/*
 * XML.c
 *
 *  Created on: Aug 10, 2015
 *      Author: Christian Konrad
 */

#include "XML.h"

extern uint32_t ownIP;

static void print_element_names(xmlNode * a_node)
{
	xmlNode *cur_node = NULL;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			printf("node type: Element, name: %s\n", cur_node->name);
		}
		print_element_names(cur_node->children);
	}
}

int *XMLGetMinNodeAndTotalWeight(xmlDocPtr doc)
{
	int *values_ptr = malloc(sizeof(int) * 2);
	xmlNodePtr node = NULL;
	xmlNodePtr child = NULL;
	xmlChar *value;
	node = xmlDocGetRootElement(doc);
	node = node->children;
	int convert_i;
	values_ptr[MIN_SHIFT] = 0;
	values_ptr[WEIGHT_SHIFT] = 0;
	uint8_t found_u8_bit;

	while (node) //node!=NULL
	{

		if (node->type == XML_ELEMENT_NODE)
		{
			if (!xmlStrcmp(node->name, (const xmlChar *) "master")
					|| !xmlStrcmp(node->name, (const xmlChar *) "node"))
				values_ptr[MIN_SHIFT]++;
			else
			{
				if (!xmlStrcmp(node->name, (const xmlChar *) "parallel"))
				{
					found_u8_bit = 0;

					child = node->children;
					while (child && (found_u8_bit < 3))
					{
						//printf("XMLCalculateMinNode:search min: current node =%s \n",child->name);
						if (!(found_u8_bit & (1 << MIN_SHIFT)))
						{
							if (!xmlStrcmp(child->name,
									(const xmlChar *) "min"))
							{
								found_u8_bit = found_u8_bit | (1 << MIN_SHIFT);
								value = xmlNodeGetContent(child);
								convert_i = (int) strtol((char*) value, NULL,
										10);
								printf("convert_i = %d\n", convert_i);
								values_ptr[MIN_SHIFT] = values_ptr[MIN_SHIFT]
										+ convert_i;
								xmlFree(value);
							}
						}
						if (!(found_u8_bit & (1 << WEIGHT_SHIFT)))
						{
							if (!xmlStrcmp(child->name,
									(const xmlChar *) "weight"))
							{
								found_u8_bit = found_u8_bit
										| (1 << WEIGHT_SHIFT);
								value = xmlNodeGetContent(child);
								convert_i = (int) strtol((char*) value, NULL,
										10);
								printf("convert_i = %d\n", convert_i);
								values_ptr[WEIGHT_SHIFT] =
										values_ptr[WEIGHT_SHIFT] + convert_i;
								xmlFree(value);
							}
						}
						child = child->next;
					} // if there are no values_ptr it assumes a value of 1
					if (!(found_u8_bit & 1 << MIN_SHIFT))
						values_ptr[MIN_SHIFT]++;
					if (!(found_u8_bit & 1 << WEIGHT_SHIFT))
						values_ptr[WEIGHT_SHIFT]++;

				}
				else
				{
					printf("XMLGetMinNodeAndTotalWeight: unknown Node type");
					return -1;
				}
			}

		}
		node = node->next;

	}
	printf("XMLCalculateMinNode:count = %d\n", values_ptr[MIN_SHIFT]);
	printf("XMLCalculateMinNode:weight = %d\n", values_ptr[WEIGHT_SHIFT]);

	return values_ptr;
}

void get_id(xmlDocPtr doc, xmlNodePtr cur)
{

	xmlChar *uri;
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "node")))
		{
			uri = xmlGetProp(cur, (const xmlChar *) "id");
			printf("uri: %s\n", uri);
			xmlFree(uri);
		}
		cur = cur->next;
	}
}

xmlDocPtr buildCompleteXML(xmlDocPtr docOld,
		struct cluster_info *clusterInfo_ptr, int *values)
{

	xmlNodePtr rootOld = NULL, rootNew = NULL, curNew = NULL, curOld = NULL,
			masterOld = NULL, child = NULL;
	xmlChar *value_str;
	int restNodes, usedRestNodes, convert_i;
	xmlDocPtr docNew = xmlNewDoc((const xmlChar*) "1.0");
	rootNew = xmlNewNode(NULL, (const xmlChar*) "Nodes");
	xmlDocSetRootElement(docNew, rootNew);
	rootOld = xmlDocGetRootElement(docOld);

	curOld = rootOld->children;

	printf("new Doc generated\n");
	char IP_str[11];
	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	restNodes = clusterInfo_ptr->num_nodes_i - values[MIN_SHIFT];
	printf("numnodes = %d\n", clusterInfo_ptr->num_nodes_i);

	for (int i = 0; i < clusterInfo_ptr->num_nodes_i; i++)
	{
		printf("%d", i);
		sprintf(IP_str, "IP_%u", clusterInfo_ptr->node_data_list_ptr[i].ip_u32);
		printf("listip=%u\townIP=%u\n",
				clusterInfo_ptr->node_data_list_ptr[i].ip_u32, ownIP);
		curNew = xmlNewChild(rootNew, NULL, (xmlChar *) IP_str, NULL);
		if (clusterInfo_ptr->node_data_list_ptr[i].ip_u32 == ownIP)
		{
			xmlNewProp(curNew, (xmlChar*) "id", (xmlChar*) "master");
			masterOld = rootOld->children;
			while (xmlStrcmp(masterOld->name, (const xmlChar *) "master")
					&& masterOld != NULL)
			{
				masterOld = masterOld->next;
				printf("searching new master:%p\n", masterOld->name);
			}
			curNew->children = masterOld->children;
		}
	}
	printf("forloop finished \n");
	pthread_mutex_unlock(&clusterInfo_ptr->mtx);
	curNew = rootNew->children;
	while (curNew->type != XML_ELEMENT_NODE && curNew != NULL)
		curNew = curNew->next;
	while (curOld != NULL)
	{
		if (curOld->type == XML_ELEMENT_NODE)
		{
			if (!xmlStrcmp(xmlGetProp(curNew, (xmlChar *) "id"),
					(xmlChar*) "master"))
			{
				do
				{
					curNew = curNew->next;
				} while (curNew->type != XML_ELEMENT_NODE && curNew != NULL);
			}
			if (curNew != NULL)
			{
				if (!xmlStrcmp(curOld->name, (const xmlChar *) "node"))
				{
					xmlSetProp(curNew, (xmlChar *) "id",
							xmlGetProp(curOld, (xmlChar *) "id"));
					curNew->children = curOld->children;

					do
					{
						curNew = curNew->next;
					} while (curNew->type != XML_ELEMENT_NODE && curNew != NULL);

				}
				else
				{
					if (!xmlStrcmp(curOld->name, (const xmlChar *) "parallel"))
					{   //TODO 1 continue
						XMLsearchElementAndGetInt(curOld,(xmlChar *)"min");
						XMLsearchElementAndGetInt(curOld,(xmlChar *)"weight");

					}
				}
			}
		}
		curOld = curOld->next;
	}

	print_element_names(rootNew);

	printf("saving file");
	xmlSaveFile("newfile.xml", docNew);
	return docNew;
}

xmlDocPtr XMLread(char *filename)
{
	xmlDocPtr XMLdoc = xmlParseFile(filename);
	if (XMLdoc == NULL)
		critErr("XML File not correct");
	return XMLdoc;
}
void XMLCleanup(xmlDocPtr doc, xmlDocPtr doc2, int *values)
{
	free(values);
	xmlFreeDoc(doc2);
	xmlFreeDoc(doc); // free document
	xmlCleanupParser(); // Free globals
}

int XMLsearchElementAndGetInt(xmlNodePtr cur, xmlChar *ElementName)
{
	int convert_i;
	xmlChar* value_str;
	xmlNodePtr child = cur->children;
	while (xmlStrcmp(child->name, ElementName) && child != NULL)
	{
		child = child->next;
		printf("searching %s:%s\n", ElementName, child->name);
	}
	value_str = xmlNodeGetContent(child);
	convert_i = (int) strtol((char*) value_str, NULL, 10);
	printf("convert_i = %d\n", convert_i);
	xmlFree(value_str);

	return convert_i;
}
