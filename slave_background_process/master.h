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



void master_control(int mast_broad_sock);

void update_cluster_info(struct cluster_info *cluster_info_str, int receive_sock, struct sockaddr_in response_addr);


#endif /* MASTER_H_ */
