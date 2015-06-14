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
#include <stdint.h>

//dont change any of these
#define ZYNQ7000 "z7"
#define KINTEX7  "k7"
#define ARTIX7	 "a7"
#define VIRTEX7  "v7"
#define VIRTEX6  "v6"
#define VIRTEX5  "v5"
#define VIRTEX4  "v4"

//this can be changed for tweaking make sure every node has the same
#define PING_PERIOD 2 ///in sek
#define TIMEOUT_PERIODS 5
#define UDP_NODE_LISTEN_PORT 12345 //used for general comands to Nodes
#define UDP_N2M_PORT 12346 //slave to master
#define UDP_ELECT_M_PORT 12347

//this has to be adjusted for the FPGA in use
#define FPGATYPE ZYNQ7000


//function to exit the program and return an error description
void critErr(char *s);

uint64_t getDecimalFromMAC();


// struct for storing a variable with a corresponding mutex
struct var_mtx{
volatile int var;
pthread_mutex_t mtx;
};

struct thread_args{
	struct var_mtx *timeout_count;
	int *am_I_master;
};

// BASICS_H_INCLUDED
#endif
