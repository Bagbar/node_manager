/*
 * mastercom.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#include "mastercom.h"

#define BUFFERSIZE 10


void *listen_for_master(void *timeout_struct)
{
	struct thread_param *timeout_counter = (struct thread_param*) timeout_struct;
	//printf("thread var=%d",timeout_counter->var);
	char recvBuff[BUFFERSIZE];
	memset(recvBuff, '0', BUFFERSIZE);

	char board_type[2] = FPGATYPE;
	int recv_mast_sock,send_mast_broad_sock;

	///create UDP-Socket Server
	if ((recv_mast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		criterr("listen:socket=");
	}

	struct sockaddr_in serv_addr, cli_addr,broad_addr;
	socklen_t cli_len = sizeof(cli_addr);
	socklen_t broad_len = sizeof(broad_addr);

	//listen socket address TODO corresponding master
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(MAST_2_CLI_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((bind(recv_mast_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))) < 0)
		{
			criterr("listen:bind recv_mast_sock:");
		}

	///create UDP-Socket Server
		if ((send_mast_broad_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			criterr("listen:send_master_socket=");
		}
	//Broadcast socket address TODO corresponding master
	broad_addr.sin_family = AF_INET;
	broad_addr.sin_port = htons(CLI_2_MAST_PORT);

	if (inet_aton("255.255.255.255", &broad_addr.sin_addr) == 0)
		{
			fprintf(stderr, "inet_aton() failed\n");
			exit(1);
		}
	if ((bind(send_mast_broad_sock, (struct sockaddr*) &broad_addr, sizeof(broad_addr))) < 0)
			{
				criterr("listen:bind send_mast_broad_sock:");
			}
	else
	{
		char i_am_new ='n';
	sendto(send_mast_broad_sock,&i_am_new,1,0,(struct sockaddr*)&broad_addr,broad_len);
	}

	// waiting for the master and reseting the timeout counter and if needed answering the request
	while (1)
	{
		printf("while:\n");
		// Recieve msg from master,
		int return_recv = recvfrom(recv_mast_sock, recvBuff, 1, 0, (struct sockaddr*) &cli_addr, &cli_len);
		// TODO (kami#9#): remove printf
		//printf("return_recv=%d\t recvBuff[0]=%c\n", return_recv, recvBuff[0]);

		// TODO (kami#5#): check if cases are all what is needed
		switch (recvBuff[0])
		{
		// TODO (kami#6#): implement case o
		/* case o : ///identify other: measure the connection to everyone and send to caller

		 break;*/
		case 's': ///identify self : send your Type to caller
			printf("send:%d",(int)sendto(recv_mast_sock,&board_type,1,0,(struct sockaddr*)&cli_addr,cli_len));
			//no break TODO remember no brake;
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
			close(recv_mast_sock);
			exit(2);
		}
	}
}
