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

void get_id(xmlDocPtr doc, xmlNodePtr cur) {


	xmlChar *uri;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"reference"))) {
		    uri = xmlGetProp(cur, "uri");
		    printf("uri: %s\n", uri);
		    xmlFree(uri);
	    }
	    cur = cur->next;
	}
	return;
}
void XMLtest(char*filename)
{

	//IBXML_TEST_VERSION
	xmlNode *root_element = NULL;
	xmlDoc *doc = xmlParseFile("test.xml");
	xmlChar *key;
	if (doc == NULL)
		critErr("XML File not correct");
	if(!xmlStrcmp((const xmlChar *)"hallo",(const xmlChar *)"hallo"))
	{

	}
	root_element = xmlDocGetRootElement(doc);
	print_element_names(root_element);
	xmlFreeDoc(doc); // free document
	xmlCleanupParser(); // Free globals
}
