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


int main(void)
{
	int i = 0;
	char cont = 'y';

	int return_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (return_socket < 0)
		printf("socketerror:%s\n", strerror(errno));

	struct sockaddr_in serv_addr;
	socklen_t serv_len = sizeof(serv_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(50505);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//	if (inet_aton("127.0.0.1", &serv_addr.sin_addr) == 0)
//	{
//		fprintf(stderr, "inet_aton() failed\n");
//		exit(1);
//	}


	char recvBuff[1024];
	memset(recvBuff, '0', sizeof(recvBuff));

	char sendBuff[1024];
	memset(sendBuff, '0', sizeof(sendBuff));
	for (i = 0; i < 2; i++)
		sendBuff[i] = 'k';

	do
	{
		int return_send = (int) sendto(return_socket, sendBuff, 1, 0,
				(struct sockaddr*) &serv_addr, serv_len);
		if (return_send < 0)
			printf("senderror:%s\n", strerror(errno));
		else
			printf("send data = %d\t send_symbol=%c\n", return_send,
					sendBuff[0]);

		int return_recv = (int) recvfrom(return_socket, recvBuff, 1, 0,
				(struct sockaddr*) &serv_addr, &serv_len);
		if (return_recv < 0)
			printf("recverror:%s\n", strerror(errno));
		else
			printf("recv data = %d\t recv_symb=%c\n", return_recv, recvBuff[0]);
		printf("continue?y/n\n");
		scanf("%c", &cont);
		fflush(stdin);

	} while (cont == 'y');

	/*int return_recv = recv(return_accept,&recvBuff,10,0);
	 if (return_recv<0) printf("recverror:%s\n",strerror(errno));
	 else printf("recv data = %d",return_recv);*/

	for (i = 0; i < 10; i++)
		printf("%c,%c\n", sendBuff[i], recvBuff[i]);

	close(return_socket);
	return 0;
}
