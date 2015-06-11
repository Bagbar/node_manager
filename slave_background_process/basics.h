/*
 * basics.h
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */
#ifndef BASICS_H_INCLUDED
#define BASICS_H_INCLUDED


#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>


#define ZYNQ7000 "z7"
#define KINTEX7  "k7"
#define ARTIX7	 "a7"
#define VIRTEX7  "v7"
#define VIRTEX6  "v6"
#define VIRTEX5  "v5"
#define VIRTEX4  "v4"

#define PING_PERIOD 2 ///in sek
#define TIMEOUT_PERIODS 5
#define MAST_2_CLI_PORT 12345
#define CLI_2_MAST_PORT	12346

//this has to be adjusted for the FPGA in use
#define FPGATYPE ZYNQ7000

void criterr(char *s);


struct thread_param{
int var;
pthread_mutex_t mtx;
};

// BASICS_H_INCLUDED
#endif
