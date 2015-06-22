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

//define the different Boards/controllers priority
#define ZYNQ7000 1
#define KINTEX7  2
#define ARTIX7	 3
#define VIRTEX7  4
#define VIRTEX6  5
#define VIRTEX5  6
#define VIRTEX4  7

//this block can be changed for tweaking make sure every node has the same
#define EST_NUM_BOARD 10 //estimated number of boards/FPGAs in the cluster used for allocation of arrays too few means more reallocs() too many allocates more unused space
#define PING_PERIOD 2 ///in sec
#define TIMEOUT_PERIODS 5
#define UDP_NODE_LISTEN_PORT 12345 //used for general commands to Nodes
#define UDP_N2M_PORT 12346 //slave to master
#define UDP_ELECT_M_PORT 12347

//this has to be adjusted for the FPGA in use
#define FPGATYPE ZYNQ7000

//function to exit the program and return an error description
void critErr(char *s);

//*mac has to point to an array with at least 6 Byte
void getMAC(uint8_t *mac);

uint64_t MACtoDecimal(uint8_t *mac);

// struct for storing a variable with a corresponding mutex
struct var_mtx
{
	volatile int var;
	pthread_mutex_t mtx;
};

struct thread_args
{
	struct var_mtx *timeout_count;
	int *am_I_master;
};

// BASICS_H_INCLUDED
#endif
