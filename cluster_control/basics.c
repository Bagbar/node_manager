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

uint32_t getIP()
{
	uint32_t IP =0;
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST], found=0;

	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	 can free list later */

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		/* For an AF_INET* interface address, display the address */

		if (!strcmp(ifa->ifa_name, "eth0") && family == AF_INET)
		{
			printf("%-8s %s (%d)\n", ifa->ifa_name,
					(family == AF_INET) ? "AF_INET" : "???", family);
			s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
					NI_MAXHOST,
					NULL, 0, NI_NUMERICHOST);
			if (s != 0)
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}

			printf("\t\taddress: <%s>\n", host);
			IP = inet_addr(host);

		}
		//TODO check if this does not activate if eth1 is not active
		if (!strcmp(ifa->ifa_name, "eth1") && family == AF_INET)
				{
					printf("%-8s %s (%d)\n", ifa->ifa_name,
							(family == AF_INET) ? "AF_INET" : "???", family);
					s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
							NI_MAXHOST,
							NULL, 0, NI_NUMERICHOST);
					if (s != 0)
					{
						printf("getnameinfo() failed: %s\n", gai_strerror(s));
						exit(EXIT_FAILURE);
					}

					printf("\t\taddress: <%s>\n", host);
					IP = inet_addr(host);

				}

	}

	freeifaddrs(ifaddr);
	return IP;
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

char* networkToDottedIP(uint32_t ip)
{

    unsigned char bytes[4];
    char *IP = malloc(16);
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    sprintf(IP,"%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
    return IP;
}
