/*
 * mastercom.h
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#ifndef SLAVE_LISTEN_H_
#define SLAVE_LISTEN_H_
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "basics.h"

void *listen_for_master(void *args_struct);
int elect_master();

#endif /* SLAVE_LISTEN_H_ */
