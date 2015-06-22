/*
 * mastercom.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#include "slave_listen.h"

#define BUFFERSIZE 10

//FIXME is this okay??
extern uint8_t mac[6];

void *listen_for_master(void *args_struct)
///  network I/O function for the control communication with the master
{

	//Variable declarations
	// TODO test this
	struct var_mtx *timeout_counter =
			((struct thread_args*) args_struct)->timeout_count;
	int *am_I_master = ((struct thread_args*) args_struct)->am_I_master;
	unsigned char board_type = FPGATYPE;

	char recvBuff[BUFFERSIZE];
	memset(recvBuff, '0', BUFFERSIZE);

	int recv_mast_sock, elect_recv_sock;

	struct sockaddr_in serv_addr, cli_addr, elect_recv_addr;

	socklen_t cli_len = sizeof(cli_addr);

	//create UDP-Socket receiver for control commands
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

	//create UDP-Socket receiver for fetching the type_and_MAC broadcasts
	if ((elect_recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:elect_recv_sock=");
	}

	//listen socket address TODO corresponding master
	elect_recv_addr.sin_family = AF_INET;
	elect_recv_addr.sin_port = htons(UDP_ELECT_M_PORT);
	elect_recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((bind(elect_recv_sock, (struct sockaddr*) &elect_recv_addr,
			sizeof(elect_recv_addr))) < 0)
	{
		critErr("listen:bind elect_recv_sock:");
	}

	fcntl(elect_recv_sock, F_SETFL, O_NONBLOCK);

	// waiting for the master and reseting the timeout counter and if needed answering the request
	//TODO put this where it belongs and check if multiple DGRAMs wait to be called.

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
			// TODO check if this is the only call of am_I_master
			*am_I_master = elect_master(elect_recv_sock);
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

int elect_master(int elect_recv_sock)
{
	unsigned char type_and_MAC[7];
	type_and_MAC[0] = FPGATYPE;
	int i;
	for (i = 0; i < 6; i++)
	{
		type_and_MAC[i + 1] = mac[i];
	}

	unsigned char typeMAC_other;
	//unsigned char *typeMacArray = (unsigned char*) malloc(8 * EST_NUM_BOARD);

	int elect_send_sock;
	struct sockaddr_in elect_addr, cli_addr;
	socklen_t cli_len, elect_send_len = sizeof(elect_addr);
//create UDP-Socket Server
	if ((elect_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:send_master_socket=");
	}
	//Broadcast socket address TODO corresponding master
	elect_addr.sin_family = AF_INET;
	elect_addr.sin_port = htons(UDP_ELECT_M_PORT);
	elect_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	if ((bind(elect_send_sock, (struct sockaddr*) &elect_addr,
			sizeof(elect_addr))) < 0)
	{
		critErr("main:bind elect_sock:");
	}

	sendto(elect_send_sock, &type_and_MAC, sizeof(type_and_MAC), 0,
			(struct sockaddr*) &elect_addr, elect_send_len);
	close(elect_send_sock);

	int i, return_recv, biggest = 1, timeout = 3;
	while (biggest == 1 && timeout > 0)
	{
		// Receive msg from other boards TODO (kami#1#) check timeout
		return_recv = recvfrom(elect_recv_sock, typeMAC_other,
				sizeof(type_and_MAC), 0, NULL, NULL);
		if (return_recv != sizeof(type_and_MAC))
		{
			if (return_recv == -1)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same leaving both if there should be a problem
				{
					timeout--;
					if (timeout > 0)
						sleep(1);
				}
				else
					criterr("elect:read broadcast socket: timeout:%d:",
							timeout);
			}
			else
			{
				printf("typeMAC_other received has the wrong size");
				exit(3); //should be exclusive for this
			}
		}

		i = 0;
		while (biggest == 1 && i < 7)
		{
			if (typeMAC_other[i] > type_and_MAC[i])
				biggest = 0;

			i++;
		}
		//flush the socket
		while(return_recv>=0){
		return_recv = recvfrom(elect_recv_sock, typeMAC_other,
						sizeof(type_and_MAC), 0, NULL, NULL);
		printf("flush\n");}

	}

	/*
	 Wait up to five seconds.
	 struct timeval tv;
	 tv.tv_sec = 5;
	 tv.tv_usec = 0;
	 //set filedescriptors
	 fd_set rfds;
	 FD_ZERO(&rfds);
	 FD_SET(0, &rfds);
	 int retval = select(1, &rfds, NULL, NULL, &tv);
	 // Don't rely on the value of tv now!

	 if (retval == -1)
	 perror("select()");
	 */

	return biggest;
}
