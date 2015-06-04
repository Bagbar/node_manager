/*
 * basics.h
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */
#ifndef BASICS_H_INCLUDED
#define BASICS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PING_PERIOD 2 ///in sek
#define TIMEOUT_PERIODS 5
#define ZEDBOARD 1
#define UDP_PORT 123457

void criterr(char *s, int sock);

// TODO (kami#8#): check for wrapper
struct slave_network_param{
int var;
pthread_mutex_t mtx;
int sock;
};

// BASICS_H_INCLUDED
#endif
