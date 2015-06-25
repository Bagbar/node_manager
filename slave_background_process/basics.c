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
	broad_addr->sin_addr.s_addr = inet_addr("255.255.255.255");
}

void fillSockaddrAny(struct sockaddr_in *any_addr, uint16_t port)
{
	any_addr->sin_family = AF_INET;
	any_addr->sin_port = htons(port);
	any_addr->sin_addr.s_addr = htonl(INADDR_ANY);
}
void getMAC(uint8_t *mac)
{
	char chr[18];
	//TODO clean
//	chr[17] = 0;//maybe obsolete
//	memset(chr, 'f', 17);
//	int i = 0;
//	for (i = 0; i < 5; i++)
//		chr[i * 3 + 2] = ':';
//	printf("%s\n", chr);

// TODO check if this works on zedboard/linaro
	FILE *pFile;
	if ((pFile = fopen("/sys/class/net/eth0/address", "r")) == NULL)
		critErr("getRandomSeedFromMAC: fopen:");
	fscanf(pFile, "%s", chr);
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

