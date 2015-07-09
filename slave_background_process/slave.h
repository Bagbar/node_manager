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

#include "basics.h"

void *listen_for_master(void *args_struct);
int elect_master();
void recvMasterControlMsg(struct slave_args *slaveArgs_ptr,int recvMast_sock,int electRecv_sock);

#endif /* SLAVE_H_ */
