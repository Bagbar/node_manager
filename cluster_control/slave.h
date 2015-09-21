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

///  network I/O function for the control communication with the master
void *slave_main(void *args_struct);

int elect_master();

void recvMasterControlMsg(struct slave_args *slaveArgs_ptr, int recvMast_sock,
		int electRecv_sock);



void *receive_info(void * transfer_args);

void *receive_file(void * recv_file_args);

void *execute_work(void *args);

void *fetch_data(void *args);

#endif /* SLAVE_H_ */
