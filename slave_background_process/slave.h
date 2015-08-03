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

#define WORK_SHIFT 2
#define BIT_SHIFT 1
#define DRIVER_SHIFT 0

struct transfer_info{
	size_t work_size,bit_size,driver_size;
		//uint8_t status_okay;
		pthread_mutex_t mtx;
};

void *slave_main(void *args_struct);
int elect_master();
void recvMasterControlMsg(struct slave_args *slaveArgs_ptr,int recvMast_sock,int electRecv_sock);

struct recv_args{
	size_t file_size;
	pthread_mutex_t *mtx_ptr;
	int filetype_i;
};

void *receive_info(void * transfer_args);

void *receive_file(void * recv_args);

#endif /* SLAVE_H_ */
