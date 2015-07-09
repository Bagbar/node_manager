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

int master_control(int mastBroad_sock);

void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32,
		const uint8_t *typeAndGroup);

void readIdentifyAnswers(int receive_sock, struct cluster_info *clusterInfo_ptr,
		uint8_t newList_u8);

void updateClusterInfo(struct cluster_info *clusterInfo_ptr, int receive_sock);



#endif /* MASTER_H_ */
