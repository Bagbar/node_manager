/*
 * mastercom.h
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#ifndef SLAVE_H_
#define SLAVE_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include "XML.h"
#include "basics.h"

#define NODE_ARCHIVE_NAME "nodedata.tar.gz"

///container for target IP addresses
struct IP_list
{
	int amount;
	uint32_t *IP;
};

/// argument for receive_info function containing the file size
struct recv_info
{
	size_t archive_size;
//uint8_t status_okay;
};

/// argument for receive_file function
struct recv_file
{ ///the size of the received file
	size_t recv_size;
	///the size of the file as stated in receive_info
	size_t expected_size;
};

/** \brief  network I/O function for the control communication with the master
 *
 *  receives commands from UDP_NODE_LISTEN_PORT that either
 *
 *  -reset the timeout counter
 *  -triggers an identification message and reset timeout counter
 *  -tell that a timeout was detected and elect_master is called
 */
void *slave_main(void *args_struct);

/** \brief communicataes with all other notes to find a new master
 *
 *  the system with the lowest type number (defiened in basics.h) will be master
 *  when two or more are of the lowest tyoe, the loweset MAC is deciding
 *
 *  returns 0 when not master
 *  returns non-0 when it got elected
 *
 *  the result has to be written to master_i in main to take effect
 */
int elect_master();

/** \brief receives necessary information for the files that are received with receive _file
 *
 * takes struct fileInfo_bufferformat as an argument
 * returns NULL
 */
void *receive_info(void * args);

/** \brief receives and archive in tar.gz format and names it NODE_ARCHIVE_NAME
 *
 * takes struct recv_file as an argument
 * returns NULL
 *
 * contains exit() calls that should be replaced in the future for better error handling
 */
void *receive_file(void * args);

/** \brief calls the receive functions unpacks the archive and starts the script and executable
 *
 */
void *fetchDataAndExecute(void *args);

/** \brief generates an IP_list with the destination IPs
 *
 * returned pointer has to be freed after use
 *
 * the IPs are taken only from <dest_IP> nodes on the first level after root
 *
 */
struct IP_list *getIPfromXML(xmlDocPtr doc);

#endif /* SLAVE_H_ */
