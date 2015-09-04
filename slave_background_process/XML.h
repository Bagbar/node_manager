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

/** \brief Gets  the total weight and the minimum amount of nodes needed in an array
 *
 * returns a pointer with the elements for weight and minimum nodes needed can be accessed with WEIGHT_SHIFT and MIN_SHIFT
 *
 */
int *XMLGetMinNodeAndTotalWeight(xmlDocPtr doc);

struct id_weight{
	xmlChar *id;
	int weight;
};


xmlDocPtr XMLread(char *filename);

/**\brief clears up all the Pointers used in XML procession
 *
 *	this is a rather specific function for a personal usecase
 */
void XMLCleanup(xmlDocPtr doc,xmlDocPtr doc2, int *values);

static void print_element_names(xmlNode * a_node);


/** \brief Creates a new xmlDoc which contains all data for the master
 *
 *	Generates an element for every node that will be in use, copies the relevant data from the oldDoc
 *	and adds the own IP and target IP to the info so it can be used to make the data distribution
 *
 *	Is not thread-safe all accesses to clusterInfo are without mutex.
 *
 *	returns a pointer that has to be freed
 */
xmlDocPtr buildCompleteXML(xmlDocPtr docOld, struct cluster_info *clusterInfo_ptr,
		int *values);

/** \brief searches through all children of an element for a child with the given name
 *
 * 	does not search for beyond children
 * 	returns -1 if element is not found
 */
int XMLsearchElementAndGetInt(xmlNodePtr cur, xmlChar *ElementName);

#endif /* XML_H_ */
