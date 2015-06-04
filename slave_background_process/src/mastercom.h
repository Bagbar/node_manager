/*
 * mastercom.h
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#ifndef SRC_MASTERCOM_H_
#define SRC_MASTERCOM_H_
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "basics.h"

void *listen_for_master(void *timeout_struct);

#endif /* SRC_MASTERCOM_H_ */
