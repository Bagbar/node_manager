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
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/xmlmemory.h>
#include <libxml2/libxml/xmlstring.h>

int XMLGetMinNodeAndTotalWeight(xmlDocPtr doc);

struct id_weight{
	xmlChar *id;
	int weight;
};

xmlDocPtr XMLread(char *filename);

void XMLCleanup(xmlDocPtr doc,xmlDocPtr doc2);

#endif /* XML_H_ */
