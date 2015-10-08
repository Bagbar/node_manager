/*
 ============================================================================
 Name        : udp_client_linux.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdint.h>
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

#define UDP_OPEN_TCP_CONNECTION_FOR_DATA_TRANSFER 51000
#define TCP_RECV_ARCHIVE_PORT 50010

#define BUFFERSIZE 1024

char* hostToDottedIP(uint32_t ip)
{

	unsigned char bytes[4];
	char *IP = malloc(16);
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	sprintf(IP, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
	return IP;
}

int main(int argc, const char* argv[])
{
	printf("%d\n",argc);
	if (argc > 1)
	{

		size_t file_size;
		int recv_return_i, returnSend_i, bytesRead_i;
		FILE * pFile;
		int broadcast_sock, transfer_sock, recvReturn_i;
		char broadSendBuff[10] = "fetch", broadRecvBuff[100], fileSendBuff[BUFFERSIZE];

		printf("opening: %s\n",argv[1]);
		pFile = fopen(argv[1], "rb");
		if (pFile == NULL)
		{
			fputs("File error", stderr);
			exit(-1);
		}
		printf("opened: %s\n",argv[1]);
		struct sockaddr_in broadcast_addr, connect_addr, master_addr;
		socklen_t broadcast_len = sizeof broadcast_addr, connect_len = sizeof connect_addr, master_len= sizeof master_addr;

		if ((broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			fclose(pFile);
			fputs("broadcast_socket=", stderr);
						exit(-1);
		}

		if ((transfer_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			fclose(pFile);
			fputs("transfer_socket=", stderr);
			exit(-1);
		}
		int broadcastEnable = 1;
		int ret = setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST,
					&broadcastEnable, sizeof(broadcastEnable));
			if (ret)
				fputs("main: couldn't set setsockopt:",stderr);

		broadcast_addr.sin_family = AF_INET;
		broadcast_addr.sin_port = htons(UDP_OPEN_TCP_CONNECTION_FOR_DATA_TRANSFER);
		broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

		connect_addr.sin_family = AF_INET;
		connect_addr.sin_port = htons(TCP_RECV_ARCHIVE_PORT);
		connect_addr.sin_addr.s_addr=htonl(INADDR_ANY);
		printf("sending broadcast\n");
		returnSend_i = sendto(broadcast_sock, &broadSendBuff[0], sizeof broadSendBuff, 0,
				(struct sockaddr*) &broadcast_addr, broadcast_len);
		printf("sent:%d\n",returnSend_i);
		if(returnSend_i <0)
			printf("senderror:%s", strerror(errno));
		printf("waiting for reply\n");
		recvReturn_i = recvfrom(broadcast_sock, &broadRecvBuff[0], 10, 0,
				(struct sockaddr*) &master_addr, &master_len);
		printf("received\n");

		//connect_addr.sin_addr.s_addr = master_addr.sin_addr.s_addr;
		printf("bind master IP = %s\n",hostToDottedIP( ntohl(connect_addr.sin_addr.s_addr)));
		int return_bind = bind(transfer_sock, (struct sockaddr*) &connect_addr, connect_len);
		if(return_bind <0)
		{
			printf("bind error=%d",errno);
			exit(1);
		}
		listen(transfer_sock, 1);

		printf("connection ack\n");
		if (!strcmp(broadRecvBuff, "ack"))
		{
			printf("waiting for accept\n");
			int return_accept = accept(transfer_sock,NULL,NULL);
			printf("accepted\n");
			if (return_accept < 0)
				printf("accepterror:%s\n", strerror(errno));


			do{
			bytesRead_i = fread(fileSendBuff, 1, BUFFERSIZE, pFile);
				if (bytesRead_i != BUFFERSIZE && feof(pFile)==0)
				{
					fputs("Reading error", stderr);
					exit(3);
				}
				returnSend_i = send(return_accept, fileSendBuff, bytesRead_i, 0);
					if (returnSend_i < 0)
						printf("senderror:%s", strerror(errno));
					else
						printf("send data = %d\n", returnSend_i);
				/* the whole file is now loaded in the memory buffer. */
				}while(feof(pFile)==0);
				// terminate
				fclose(pFile);

		}
		close(transfer_sock);
		close(broadcast_sock);

		printf("finished");
		return 0;
	}

	else
	{
		printf("no file declared");
		return -1;
	}
}
