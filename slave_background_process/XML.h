/*
 * XML.h
 *
 *  Created on: Aug 10, 2015
 *      Author: xubuntu
 */

#ifndef XML_H_
#define XML_H_

#include "basics.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlstring.h>

#define MIN_SHIFT 0
#define WEIGHT_SHIFT 1

/** \brief Gets  the total weight of all parallel blocks and checks if there are enough nodes available
 *
 * if no errors occurred it returns the total weight
 * 0 = no parallel blocks
 * -1 = not enough nodes available
 */
int *XMLGetMinNodeAndTotalWeight(xmlDocPtr doc);

struct id_weight{
	xmlChar *id;
	int weight;
};

/**
 *
 */
xmlDocPtr XMLread(char *filename);

/**\brief clears up all the Pointers used in XML procession
 *
 */
void XMLCleanup(xmlDocPtr doc,xmlDocPtr doc2, int *values);

static void print_element_names(xmlNode * a_node);

xmlDocPtr buildCompleteXML(xmlDocPtr docOld, struct cluster_info *clusterInfo_ptr,
		int *values);
int XMLsearchElementAndGetInt(xmlNodePtr cur, xmlChar *ElementName);

#endif /* XML_H_ */
