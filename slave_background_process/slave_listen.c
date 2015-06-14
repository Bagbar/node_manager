/*
 * mastercom.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#include "slave_listen.h"

#define BUFFERSIZE 10

void *listen_for_master(void *args_struct)
///  network I/O function for the control communication with the master
{

	//Variable declarations
	// TODO test this
	struct var_mtx *timeout_counter = ((struct thread_args*) args_struct)->timeout_count;
	int *am_I_master = ((struct thread_args*) args_struct)->am_I_master;
	char board_type[2] = FPGATYPE;

	char recvBuff[BUFFERSIZE];
	memset(recvBuff, '0', BUFFERSIZE);

	int recv_mast_sock;

	struct sockaddr_in serv_addr, cli_addr;

	socklen_t cli_len = sizeof(cli_addr);
	;

	//create UDP-Socket Server
	if ((recv_mast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:socket=");
	}

	//listen socket address TODO corresponding master
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(UDP_NODE_LISTEN_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((bind(recv_mast_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)))
			< 0)
	{
		critErr("listen:bind recv_mast_sock:");
	}

	// waiting for the master and reseting the timeout counter and if needed answering the request
	while (1)
	{
		// Recieve msg from master,
		int return_recv = recvfrom(recv_mast_sock, recvBuff, 1, 0,
				(struct sockaddr*) &cli_addr, &cli_len);
		// TODO (kami#9#): remove printf
		//printf("return_recv=%d\t recvBuff[0]=%c\n", return_recv, recvBuff[0]);

		// TODO (kami#5#): check if cases are all what is needed
		switch (recvBuff[0])
		{
		// TODO (kami#6#): implement case o
		/* case o : ///identify other: measure the connection to everyone and send to caller

		 break;*/
		case 's': ///identify self : send your Type to caller
			printf("send:%d",
					(int) sendto(recv_mast_sock, &board_type, 1, 0,
							(struct sockaddr*) &cli_addr, cli_len));
			//no break TODO remember no brake;
		case 'k': ///keepalive
			if (pthread_mutex_lock(&(timeout_counter->mtx)))
				critErr("listen: mutex_lock:");
			timeout_counter->var = 0;
			if (pthread_mutex_unlock(&(timeout_counter->mtx)))
				critErr("listen: mutex_unlock:");
			break;
		case 't': //timeout encountered by a node
			if (pthread_mutex_lock(&(timeout_counter->mtx)))
				critErr("listen: mutex_lock:");
			timeout_counter->var = 0;
			*am_I_master=elect_master();
			if (pthread_mutex_unlock(&(timeout_counter->mtx)))
				critErr("listen: mutex_unlock:");
			break;
		default:
			printf("listen: Unknown symbol: recvBuff[0]=%c", recvBuff[0]);
			/// TODO (kami#9#): no exit?
			close(recv_mast_sock);
			exit(2);
		}
	}
}
