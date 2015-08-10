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

#include "basics.h"

struct send_info
{
	size_t work_size, bit_size, driver_size;
	uint32_t IP; //converted format for sockaddr_in
};

struct send_file
{
	char filename[20];
	int filetype_i;
	uint32_t IP; //converted format for sockaddr_in
};

int master_control(int mastBroad_sock);

void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32,
		uint8_t *typeAndGroup);

void readIdentifyAnswers(int receive_sock, struct cluster_info *clusterInfo_ptr,
		uint8_t newList_u8);

void updateClusterInfo(struct cluster_info *clusterInfo_ptr, int receive_sock);

void *send_info(void *send_info_args);

void *send_file(void *send_file_args);
#endif /* MASTER_H_ */
