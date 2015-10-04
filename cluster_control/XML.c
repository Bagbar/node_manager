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
	xmlChar *value;
	node = xmlDocGetRootElement(doc);
	node = node->children;
	int min_i, weight_i;
	values_ptr[MIN_SHIFT] = 0;
	values_ptr[WEIGHT_SHIFT] = 0;

	while (node) //node!=NULL
	{

		if (node->type == XML_ELEMENT_NODE)
		{
			if (!xmlStrcmp(node->name, (const xmlChar *) "master")
					|| !xmlStrcmp(node->name, (const xmlChar *) "node"))
			{
				values_ptr[MIN_SHIFT]++;
			}
			else
			{
				if (!xmlStrcmp(node->name, (const xmlChar *) "parallel"))
				{
					min_i = XMLsearchElementAndGetInt(node, (xmlChar *) "min");
					//TODO implement weighting function and reactivate this
					//weight_i = XMLsearchElementAndGetInt(node, (xmlChar *) "weight");
					weight_i = -1;
					// if there are no values_ptr it assumes a value of 1
					if (min_i == -1)
					{
						values_ptr[MIN_SHIFT]++;
					}
					else
					{
						values_ptr[MIN_SHIFT] = values_ptr[MIN_SHIFT] + min_i;
					}

					if (weight_i == -1)
					{
						values_ptr[WEIGHT_SHIFT]++;
					}
					else
					{
						values_ptr[WEIGHT_SHIFT] = values_ptr[WEIGHT_SHIFT] + weight_i;
					}
				}
				else
				{
					printf("XMLGetMinNodeAndTotalWeight: unknown Node type");
					return NULL;
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

xmlDocPtr buildCompleteXML(xmlDocPtr docOld, struct cluster_info *clusterInfo_ptr, int *values)
{

	xmlNodePtr rootOld = NULL, rootNew = NULL, curNew = NULL, curOld = NULL, masterOld = NULL, child =
	NULL, destSearch = NULL;
	xmlChar *destination_str, *id_str;
	xmlChar partnumber_str[10];
	char isElement = 0;
	int i, restNodes_i, usedRestNodes_i, convert_i, min_i, max_i, weight_i, additionalNodes_i,
			parallelNodes_i;
	float rest, accumulatedRest = 0.0;
	xmlDocPtr docNew = xmlNewDoc((const xmlChar*) "1.0");
	rootNew = xmlNewNode(NULL, (const xmlChar*) "Nodes");
	restNodes_i = clusterInfo_ptr->numNodes_size - values[MIN_SHIFT];
	if (values[WEIGHT_SHIFT] > 0)
	{
		additionalNodes_i = restNodes_i / values[WEIGHT_SHIFT];
		rest = (float) restNodes_i / (float) values[WEIGHT_SHIFT] - additionalNodes_i;
		printf("XML:rest=%f\n", rest);
	}

	xmlDocSetRootElement(docNew, rootNew);
	rootOld = xmlDocGetRootElement(docOld);

	curOld = rootOld->children;

	printf("new Doc generated\n");
	char IP_str[11];
	//pthread_mutex_lock(&clusterInfo_ptr->mtx);
	restNodes_i = clusterInfo_ptr->numNodes_size - values[MIN_SHIFT];
	printf("numnodes = %d\n", clusterInfo_ptr->numNodes_size);

	// generate entry for every available node
	for (int i = 0; i < clusterInfo_ptr->numNodes_size; i++)
	{
		printf("%d", i);
		sprintf(IP_str, "IP_%u", clusterInfo_ptr->node_data_list_ptr[i].ip_u32);
		printf("listip=%u\townIP=%u\n", clusterInfo_ptr->node_data_list_ptr[i].ip_u32, ownIP);
		curNew = xmlNewChild(rootNew, NULL, (xmlChar *) IP_str, NULL);
		if (clusterInfo_ptr->node_data_list_ptr[i].ip_u32 == ownIP)
		{
			xmlNewProp(curNew, (xmlChar*) "id", (xmlChar*) "master");
			masterOld = rootOld->children;
			while (xmlStrcmp(masterOld->name, (const xmlChar *) "master") && masterOld != NULL)
			{
				masterOld = masterOld->next;
				printf("XML:searching master in oldDoc:%s\n", masterOld->name);
			}
			curNew->children = masterOld->children;
		}
	}
	printf("forloop finished \n");
	//pthread_mutex_unlock(&clusterInfo_ptr->mtx);
	curNew = rootNew->children;
	if (curNew == NULL)
	{
		printf("XML: something went terribly wrong the New doc has no children");
		exit(1000);
	}

	// fill every entry with the corresponding information
	while (curNew != NULL && curNew->type != XML_ELEMENT_NODE)
		curNew = curNew->next;
	//iterate through old Doc to get the information
	while (curOld != NULL && curNew != NULL)
	{
		// copy the information to new Doc
		if (curOld->type == XML_ELEMENT_NODE)
		{
			id_str = xmlGetProp(curNew, (xmlChar *) "id");
			if (!xmlStrcmp(id_str, (xmlChar*) "master"))
			{
				do
				{
					curNew = curNew->next;
				} while (curNew->type != XML_ELEMENT_NODE && curNew != NULL);
			}
			xmlFree(id_str);
			if (curNew != NULL)
			{
				if (!xmlStrcmp(curOld->name, (const xmlChar *) "node"))
				{
					id_str = xmlGetProp(curOld, (xmlChar *) "id");
					xmlSetProp(curNew, (xmlChar *) "id", id_str);
					xmlFree(id_str);
					curNew->children = curOld->children;

					do
					{
						curNew = curNew->next;
					} while (curNew->type != XML_ELEMENT_NODE && curNew != NULL);

				}
				else
				{
					// copy multiple times if element is parallel block
					if (!xmlStrcmp(curOld->name, (const xmlChar *) "parallel"))
					{
						min_i = XMLsearchElementAndGetInt(curOld, (xmlChar *) "min");
						max_i = XMLsearchElementAndGetInt(curOld, (xmlChar *) "max");
						//TODO implement weighting function (remember to reactivate the weight getter function)
//						weight_i = XMLsearchElementAndGetInt(curOld,
//								(xmlChar *) "weight");
//						weightNodes_i =  (float)weight_i/(float)values[WEIGHT_SHIFT];
//						printf("XML: weightNodes = %d\t",weightNodes_i);
						//TODO deactivate this when weighting algorithm is implemented
						accumulatedRest += rest;
						printf("XML: accumulatedRest = %f\t casted = %d\n", accumulatedRest,
								(int) accumulatedRest);
						printf("min_i = %d \t additional=%d\n", min_i, additionalNodes_i);
						if ((min_i + additionalNodes_i + (int) accumulatedRest) > max_i)
						{
							printf("sum=%d", (min_i + additionalNodes_i + (int) accumulatedRest));
							parallelNodes_i = max_i;
							usedRestNodes_i = usedRestNodes_i + parallelNodes_i - min_i;
						}
						else
						{
							parallelNodes_i = min_i + additionalNodes_i + (int) accumulatedRest;
							accumulatedRest = accumulatedRest - (int) accumulatedRest;
							usedRestNodes_i = usedRestNodes_i + parallelNodes_i - min_i;

						}

						printf("XML:restNodes=%d\tusedRestNodes_i=%d\t nodes for this block = %d\n",
								restNodes_i, usedRestNodes_i, parallelNodes_i);
						for (i = 0; i < parallelNodes_i; i++)
						{
							printf("now %s\n", curNew->name);
							id_str = xmlGetProp(curOld, (xmlChar *) "id");
							xmlSetProp(curNew, (xmlChar *) "id", id_str);

							curNew->children = curOld->children;
							sprintf((char*) partnumber_str, "%d", i);
							xmlSetProp(curNew, (xmlChar*) "part", partnumber_str);
							//if(xmlStrcmp(curNew->name, (const xmlChar *) "IP_269488144"))
							{
								do
								{
									xmlFree(id_str);
									curNew = curNew->next;
									id_str = xmlGetProp(curNew, (xmlChar *) "id");
									//printf("next %s\n", curNew->name);
								} while ((curNew != NULL && curNew->type != XML_ELEMENT_NODE)
										|| !xmlStrcmp(id_str, (const xmlChar *) "master"));
								xmlFree(id_str);
							}
						}

					}
				}
			}
		}
		curOld = curOld->next;
	}
	printf("saving file");
	xmlSaveFile("intermediate.xml", docNew);
	xmlDocPtr newXMLdoc = xmlParseFile("intermediate.xml");
	xmlNodePtr root = xmlDocGetRootElement(newXMLdoc);
	curNew = root->children;

	while (curNew != NULL)
	{
		if (curNew->type == XML_ELEMENT_NODE)
		{
			child = curNew->children;
			while (child != NULL && xmlStrcmp(child->name, (xmlChar *) "destination"))
			{
				child = child->next;
			}
			if (child)
			{
				destination_str = xmlNodeGetContent(child);
				destSearch = rootNew->children;

				//search all nodes with this id and ad their IP
				while (destSearch != NULL)
				{
					id_str = xmlGetProp(destSearch, (xmlChar *) "id");
					printf("current: node= %s \t search= %s\tcurrent_id = %s \t searched =%s \n",
							curNew->name, destSearch->name, id_str, destination_str);
					if (!xmlStrcmp(id_str, destination_str))
					{
						printf("%s", &(destSearch->name[3]));
						//exit(0);
						xmlNewChild(curNew, NULL, (xmlChar*) "dest_IP", &(destSearch->name[3]));
					}

					xmlFree(id_str);
					destSearch = destSearch->next;
				}
				xmlFree(destination_str);
			}
			/*else
			 {
			 printf("XML: no destination specified");
			 return NULL;
			 }*/

		}
		curNew = curNew->next;
	}
	XMLremoveUnusedNodes(newXMLdoc);
	xmlSaveFile(OUTPUT_XML_NAME, newXMLdoc);
	xmlFree(docNew);
	return newXMLdoc;
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
	int convert_i = -1;
	xmlChar* value_str = NULL;
	xmlNodePtr child = cur->children;
	while (xmlStrcmp(child->name, ElementName) && child != NULL)
	{
		child = child->next;
		printf("searching %s:%s\n", ElementName, child->name);
	}
	if (child)
	{
		value_str = xmlNodeGetContent(child);
		convert_i = (int) strtol((char*) value_str, NULL, 10);
		printf("convert_i = %d\n", convert_i);
		xmlFree(value_str);
	}

	return convert_i;
}

void XMLremoveUnusedNodes(xmlDocPtr doc)
{

	xmlNodePtr node = xmlDocGetRootElement(doc);
	node = node->children;
	while (node->next)
	{
		if (node->next->children == NULL)
		{
			XMLremoveNode(node->next);
		}
		else
		{
			node = node->next;
		}
	}
}

void XMLremoveNode(xmlNodePtr node)
{
	xmlUnlinkNode(node);
	xmlFreeNode(node);
}

xmlDocPtr XMLNodeToDoc(xmlNodePtr node)
{
	xmlDocPtr docNew = xmlNewDoc((const xmlChar*) "1.0");
	xmlDocSetRootElement(docNew, node);

	return docNew;
}
