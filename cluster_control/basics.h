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
#define OUTPUT_XML_NAME "output.xml"
#define INPUT_XML_NAME "input.xml"
#define CHECK_OKAY 0
#define CHECK_FAILED 1
#define WORK_THREAD_CANCELED 100

#define KEEP_ALIVE_SIGNAL 'k'
#define IDENTIFY_SIGNAL 'i'

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
#define PINGS_PER_IDENTIFY 6//used in master_main defines how long the intervals between network updates are
#define TIMEOUT 3 //how often a non-block-receive returns with nothing

//Port defines
#define UDP_NODE_LISTEN_PORT 50001 //used for general commands to Nodes
#define UDP_N2M_PORT 50002 //slave to master
#define UDP_ELECT_M_PORT 50003
#define UDP_OPEN_TCP_CONNECTION_FOR_DATA_TRANSFER 51000
#define TCP_GET_DATA 51001
#define TCP_RECV_ARCHIVE_PORT 50010 //port for the archive with all needed data
#define TCP_RECV_INFO_PORT 50011 //used for sending  administrative data to slaves
#define TCP_RECV_DATA_PORT 40001 //receive the data that has to be processed

//this has to be adjusted for the FPGA in use
#define FPGATYPE ZYNQ7000
//everyone with the same number shall be in the same subgroup (not active)
#define CLUSTERGROUP 0

#define FILENAME_SIZE 30

///struct for storing a variable with a corresponding mutex
struct var_mtx
{
	volatile int var;
	pthread_mutex_t mtx;
};

///struct for thread creating arguments master_ptr is also used with the mutex
struct slave_args
{
	struct var_mtx *timeout_count;
	int *master_ptr;
	uint8_t *subgroup_ptr;
};

/** \brief  struct in for sending the file_info over ethernet, shall be copied into the buffer
 *
 * WARNING this experimental and might not work
 */
struct fileInfo_bufferformat
{
	size_t file_size;
	char workname[FILENAME_SIZE], scriptname[FILENAME_SIZE];
};

/// struct for list of nodes
struct node_data
{
	///IP in networkorder
	uint32_t ip_u32;
	/// type number according to defines
	uint8_t type_u8;
	/// last alive counter number when it answered
	uint8_t lastAlive_u8;
	/// does it have a job running
	uint8_t nowActive_u8;
	/// subgroup number(not used at the moment)
	uint8_t group_u8;
};

struct cluster_info
{
	//uint16_t *connection_values_ptr;
	/// list of the nodes
	struct node_data *node_data_list_ptr;
	/// amount of nodes for which memory is allocated
	size_t size;
	///actual number of nodes stored
	size_t numNodes_size;
	/// incrementing number for comparison with lastAlive
	uint8_t alive_count_u8;
	pthread_mutex_t mtx;
};

///function to exit the program and return an error description
void critErr(char *s);

///fills the sockaddr_in struct for Broadcasts
void fillSockaddrBroad(struct sockaddr_in *broad_addr, uint16_t port);

///fills the sockaddr_in struct for receiving from any IP
void fillSockaddrAny(struct sockaddr_in *any_addr, uint16_t port);

///fills the sockaddr_in struct with the target of the loopback address
void fillSockaddrLoop(struct sockaddr_in *loop_addr, uint16_t port);

///reads the IP with getifaddrs and returns the networf format of the IP address, designed for one active IF
uint32_t getIP();

/**\\brief reads the MAC from "/sys/class/net/eth0/address"
 *
 * returns the MAC in 6 Byte chunks to the pointer
 **mac has to point to an array with at least 6 Byte
 */
void getMAC(uint8_t *mac);

/**\brief takes a MAC in the 6 unsigned char array format and converts it to uint64_t
 *
 *  Generally the bits stay the same only the type should change, this is done by bit shifts
 *  of the array components to the correct position
 */
uint64_t MACtoDecimal(uint8_t *mac);

/**\ brief compare function for sorting algorithm
 *
 *  conforms to the compare standard format
 */
int compareNodes(const void * a, const void * b);

/** returns a pointer to char array with the dotted notation of the IP
 *  pointer has to be freed
 */
char* networkToDottedIP(uint32_t ip);

// BASICS_H_INCLUDED
#endif

/** \brief
 *
 */
long int fsize(char* file);
