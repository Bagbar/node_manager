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
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/xmlmemory.h>
#include <libxml2/libxml/xmlstring.h>

static void print_element_names(xmlNode * a_node);

void get_id(xmlNode * a_node);

void XMLtest(char*filename);


#endif /* XML_H_ */
