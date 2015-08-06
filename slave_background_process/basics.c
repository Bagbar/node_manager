/*
 * basics.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */
#include "basics.h"

void critErr(char *s)
{
	perror(s);
	exit(1);
}

void fillSockaddrBroad(struct sockaddr_in *broad_addr, uint16_t port)
{
	broad_addr->sin_family = AF_INET;
	broad_addr->sin_port = htons(port);
	broad_addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
}

void fillSockaddrAny(struct sockaddr_in *any_addr, uint16_t port)
{
	any_addr->sin_family = AF_INET;
	any_addr->sin_port = htons(port);
	any_addr->sin_addr.s_addr = htonl(INADDR_ANY);
}

void fillSockaddrLoop(struct sockaddr_in *loop_addr, uint16_t port)
{
	loop_addr->sin_family = AF_INET;
	loop_addr->sin_port = htons(port);
	loop_addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
}

void getMAC(uint8_t *mac)
{
	char chr[18];

	FILE *pFile;
	if ((pFile = fopen("/sys/class/net/eth0/address", "r")) == NULL)
		critErr("getMAC: fopen:");
	int ret = fscanf(pFile, "%s", chr);
	fclose(pFile);
	//	printf("%s\n", chr);

	sscanf(chr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2],
			&mac[3], &mac[4], &mac[5]);

//	printf("%u", imac);

}

uint64_t MACtoDecimal(uint8_t *mac)
{
	return ((uint64_t) mac[5]) + (((uint64_t) mac[4]) << 8)
			+ (((uint64_t) mac[3]) << 16) + (((uint64_t) mac[2]) << 24)
			+ (((uint64_t) mac[1]) << 32) + (((uint64_t) mac[0]) << 40);
}

int compareNodes(const void * a, const void * b)
{
	if ((*(struct node_data*) a).ip_u32 < (*(struct node_data*) b).ip_u32)
		return -1;
	if ((*(struct node_data*) a).ip_u32 == (*(struct node_data*) b).ip_u32)
		return 0;
	if ((*(struct node_data*) a).ip_u32 > (*(struct node_data*) b).ip_u32)
		return 1;

	//alternativly
	//return ( (*(struct node_data*)a).ip_u32 - (*(struct node_data*)b).ip_u32 );
}
