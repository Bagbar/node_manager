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
uint64_t getDecimalFromMAC()
{

	uint64_t imac = 0;
	char chr[18];
	unsigned char mac[6];

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

	imac = ((uint64_t) mac[5]) + (((uint64_t) mac[4]) << 8)
			+ (((uint64_t) mac[3]) << 16) + (((uint64_t) mac[2]) << 24)
			+ (((uint64_t) mac[1]) << 32) + (((uint64_t) mac[0]) << 40);
//	printf("%u", imac);
	return imac;
}

