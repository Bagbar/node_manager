/*
 * mastercom.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#include "mastercom.h"

#define BUFFERSIZE 10

// int listen_for_master(struct slave_network_param *timeout_counter) with implicit cast may work but seems dirty
void *listen_for_master(void *timeout_struct)
{
	struct thread_param *timeout_counter = (struct thread_param*) timeout_struct;
	//printf("thread var=%d",timeout_counter->var);
	char recvBuff[BUFFERSIZE];
	memset(recvBuff, '0', BUFFERSIZE);

	char board_type[2] = FPGATYPE;
	int udp_sock;

	///create UDP-Socket Server
	if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		criterr("socket=");
	}

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t cli_len = sizeof(cli_addr);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(50505);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((bind(udp_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))) < 0)
	{
		criterr("bind");
	}

	while (1)
	{
		printf("while:\n");
		// Recieve msg from master,
		int return_recv = recvfrom(udp_sock, recvBuff, 1, 0, (struct sockaddr*) &cli_addr, &cli_len);
		// TODO (kami#9#): remove printf
		//printf("return_recv=%d\t recvBuff[0]=%c\n", return_recv, recvBuff[0]);

		// TODO (kami#5#): check if cases are all what is needed
		switch (recvBuff[0])
		{
		// TODO (kami#6#): implement case o
		/* case o : ///identify other: measure the connection to everyone and send to caller

		 break;*/
		case 's': ///identify self : send your Type to caller
			printf("send:%d",(int)sendto(udp_sock,&board_type,1,0,(struct sockaddr*)&cli_addr,cli_len));
			//break;
		case 'k': ///keepalive
			if( pthread_mutex_lock(&(timeout_counter->mtx)))
				criterr("listen: mutex_lock:");
			timeout_counter->var = 0;
			if(pthread_mutex_unlock(&(timeout_counter->mtx)))
				criterr("listen: mutex_unlock:");
			break;
		default:
			printf("listen: Unknown symbol: recvBuff[0]=%c", recvBuff[0]);
			/// TODO (kami#9#): no exit?
			close(udp_sock);
			exit(2);
		}
	}
}
