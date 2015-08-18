/*
 * XML.c
 *
 *  Created on: Aug 10, 2015
 *      Author: Christian Konrad
 */

#include "XML.h"

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

int XMLGetMinNodeAndTotalWeight(xmlDocPtr doc)
{
	xmlNodePtr node = NULL;
	xmlNodePtr child = NULL;
	xmlChar *min;
	node = xmlDocGetRootElement(doc);
	node=node->children;
	int convert_i, count_i=0, totalweight_i=0;
	
	while(node)//node!=NULL
	{
		if(node->type==XML_ELEMENT_NODE)
		{
			if(!xmlStrcmp(node->name,(const xmlChar *) "master") || !xmlStrcmp(node->name,(const xmlChar *)"node"))
				count_i++;
			else
			{
				if(!xmlStrcmp(node->name,(const xmlChar *)"parallel"))
				{
					child = node->children;
					while(xmlStrcmp(child->name,(const xmlChar *) "min"))
					{
						//printf("XMLCalculateMinNode:search min: current node =%s \n",child->name);
						child=child->next;
						if(child==NULL)
						{
							printf("XMLCalculateMinNode: No minimum value in parallel block");
						return -1;}
					}
					min = xmlNodeGetContent(child);

					convert_i = (int) strtol((char*)min,NULL,10);
					printf("convert_i = %d\n",convert_i);
					count_i = count_i + convert_i;
					xmlFree(min);
				}
			}

		}
		node=node->next;


	}
	printf("XMLCalculateMinNode:count = %d",count_i);
;	return count_i;
}

void get_id(xmlDocPtr doc, xmlNodePtr cur)
{

	xmlChar *uri;
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "node")))
		{
			uri = xmlGetProp(cur,(const xmlChar *) "id");
			printf("uri: %s\n", uri);
			xmlFree(uri);
		}
		cur = cur->next;
	}
}

xmlDocPtr buildCompleteXML(xmlDocPtr doc, struct cluster_info *clusterInfo_ptr,int minNodes)
{

	xmlNodePtr root_element = NULL;
	xmlChar *key;
int restNodes;
	xmlDocPtr newdoc = xmlNewDoc((const xmlChar*)"1.0");
	root_element = xmlNewNode(NULL, (const xmlChar*) "Nodes");
	xmlDocSetRootElement(doc, root_element);

	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	restNodes = clusterInfo_ptr->num_nodes_i - minNodes;
	for(int i = 0;i<clusterInfo_ptr->num_nodes_i;i++)
	{
		// TODO 1 add conversion to string
	xmlNewChild(root_element, NULL, clusterInfo_ptr->node_data_list_ptr[i].ip_u32 , const xmlChar * c);
	}
	pthread_mutex_unlock(&clusterInfo_ptr->mtx);

	xmlNodePtr node = NULL;
		xmlNodePtr child = NULL;
		xmlChar *weight_str;
		node = xmlDocGetRootElement(doc);
		node=node->children;
		int convert_i, totalWeight_i=0;
		while(node)//node!=NULL
		{
			if(node->type==XML_ELEMENT_NODE)
			{
				if(!xmlStrcmp(node->name,(const xmlChar *) "master") || !xmlStrcmp(node->name,(const xmlChar *)"node"))
					totalWeight_i++;
				else
				{
					if(!xmlStrcmp(node->name,(const xmlChar *)"parallel"))
					{
						child = node->children;
						while(xmlStrcmp(child->name,(const xmlChar *) "min"))
						{
							child=child->next;
							if(child==NULL)
							{
								printf("XMLCalculateMinNode: No minimum value in parallel block");
							return -1;}
						}
						weight_str = xmlNodeGetContent(child);

						convert_i = (int) strtol((char*)weight_str,NULL,10);
						printf("convert_i = %d\n",convert_i);
						totalWeight_i = totalWeight_i + convert_i;
						xmlFree(weight_str);
					}
				}

			}
			node=node->next;


		}





	xmlSaveFile(BAD_CAST "newfile.xml", newdoc);

	
	return newdoc;
}

xmlDocPtr XMLread(char *filename)
{
	xmlDocPtr XMLdoc = xmlParseFile(filename);
	if (XMLdoc == NULL)
		critErr("XML File not correct");
	return XMLdoc;
}
void XMLCleanup(xmlDocPtr doc,xmlDocPtr doc2)
{
	xmlFreeDoc(doc2);
	xmlFreeDoc(doc); // free document
	xmlCleanupParser(); // Free globals
}


