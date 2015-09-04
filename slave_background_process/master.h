/*
 * master.h
 *
 *  Created on: Jun 23, 2015
 *      Author: christian
 */

#ifndef MASTER_H_
#define MASTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "XML.h"
#include "basics.h"



/// arguments for send_info function contains target IP and size of the file
struct send_info
{
	size_t file_size;
	uint32_t IP; ///network format
};

/// arguments for send_file function contains target IP and name of the file to be sent
struct send_file
{
	char filename[20];
	//	int filetype_i;
	uint32_t IP; /// network format
};

/** \brief gets called when Node is elected as master, manages network exploration, job receiving and process distrubution
 *
 */
int master_control(int mastBroad_sock);

/** \brief adds nodes to clusterInfo with the info of IP, type and group
 *
 *	adds nodes at the end and changes the relevant parameters does not sort the list
 *	clusterInfo_ptr points to the struct that shall be expanded
 *	ip_u32 is the IP of the node
 *	typeAndGroup is the value received by the node when asked for identification
 */
void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32,
		uint8_t *typeAndGroup);

/** \brief receives the previous triggered identification messages from the network and modifies the clusterIndo
 *
 * uses addNode2List when it has the needed information for the list and sorts the it
 * if newList is 0 it checks if the same IP is in the list and marks the nodes as active if it is or adds if it is new and sorts the list
 * if newList is not 0 all received information is added and the list is sorted afterwards. No check for nodes already in the list is done.
 * in any case clusterInfo has to be configured correctly before
 */
void readIdentifyAnswers(int receive_sock, struct cluster_info *clusterInfo_ptr,
		uint8_t newList_u8);

/** \brief calls readIdentifyAnswers and removes node that are not active from the list
 *
 *	this may be done in a thread
 */
void updateClusterInfo(struct cluster_info *clusterInfo_ptr, int receive_sock);

/** \brief sends information about the program files to the node
 *
 * At the moment this is only the size of the archive that will be send
 */
void *send_info(void *send_info_args);

/** \brief sends file to the target
 *
 */
void *send_file(void *send_file_args);

/**starts the distrubution and work
 * returns a pointer to an int that has to be freed.
 */
void * start(void *start_args);

void * getProgram(void * args);

void * distributeData(void * args);
#endif /* MASTER_H_ */
