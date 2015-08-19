/*
 * basics.h
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */
#ifndef BASICS_H_INCLUDED
#define BASICS_H_INCLUDED

#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */

#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>

#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ARCHIVE_NAME "archive.tar.gz"
#define CHECK_OKAY 0
#define CHECK_FAILED 1
#define WORK_THREAD_CANCELED 100

//define the different Boards/controllers priority
#define SERVER   0
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
#define REALLOC_STEPSIZE 3
#define PINGS_PER_IDENTIFY 6//used in master_control defines how long the intervals between network updates are
#define TIMEOUT 3 //how often a non-block-receive returns with nothing

//Port defines
#define UDP_NODE_LISTEN_PORT 50001 //used for general commands to Nodes
#define UDP_N2M_PORT 50002 //slave to master
#define UDP_ELECT_M_PORT 50003
#define TCP_RECV_ARCHIVE_PORT 50010 //port for the archive with all needed data
#define TCP_RECV_INFO_PORT 50011 //used for sending  administrative data to slaves
#define TCP_RECV_DATA_PORT 40001 //receive the data that has to be processed

//this has to be adjusted for the FPGA in use
#define FPGATYPE ZYNQ7000
//everyone with the same number shall be in the same subgroup (not active)
#define CLUSTERGROUP 0

// struct for storing a variable with a corresponding mutex
struct var_mtx
{
	volatile int var;
	pthread_mutex_t mtx;
};

//struct for thread creating arguments master_ptr is also used with the mutex
struct slave_args
{
	struct var_mtx *timeout_count;
	int *master_ptr;
	uint8_t *subgroup_ptr;
};

struct node_data
{
	uint32_t ip_u32;
	uint8_t type_u8;
	uint8_t lastAlive_u8;
	uint8_t nowActive_u8;
	uint8_t group_u8;
};

struct cluster_info
{
//	uint32_t *ip_ptr;
//	uint8_t *type_ptr;
	//uint16_t *connection_values_ptr;
	struct node_data *node_data_list_ptr;
	int size_i;
	int num_nodes_i;
	uint8_t alive_count_u8;
	pthread_mutex_t mtx;
};

//function to exit the program and return an error description
void critErr(char *s);

//fills the sockaddr_in struct for Broadcasts
void fillSockaddrBroad(struct sockaddr_in *broad_addr, uint16_t port);

//fills the sockaddr_in struct for receiving from any IP
void fillSockaddrAny(struct sockaddr_in *any_addr, uint16_t port);

void fillSockaddrLoop(struct sockaddr_in *loop_addr, uint16_t port);

uint32_t getIP()

//*mac has to point to an array with at least 6 Byte
void getMAC(uint8_t *mac);

uint64_t MACtoDecimal(uint8_t *mac);

int compareNodes(const void * a, const void * b);

// BASICS_H_INCLUDED
#endif
