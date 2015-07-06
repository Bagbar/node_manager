/*
 * mastercom.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#include "slave.h"

#define BUFFERSIZE 10
#define TYPE_MAC_LENGTH 7

//FIXME is this okay??
extern uint8_t mac[6];

void *listen_for_master(void *args_struct)
///  network I/O function for the control communication with the master
{
	//Variable declarations
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

	//listen socket address
	fillSockaddrAny(&serv_addr, UDP_NODE_LISTEN_PORT);

	if ((bind(recv_mast_sock, (struct sockaddr*) &serv_addr, sizeof serv_addr))
			< 0)
	{
		critErr("listen:bind recv_mast_sock:");
	}
	//create UDP-Socket receiver for fetching the type_and_MAC broadcasts
	if ((elect_recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:elect_recv_sock=");
	}

	//listen socket address
	fillSockaddrAny(&elect_recv_addr, UDP_ELECT_M_PORT);

	if ((bind(elect_recv_sock, (struct sockaddr*) &elect_recv_addr,
			sizeof(elect_recv_addr))) < 0)
	{
		critErr("listen:bind elect_recv_sock:");
	}

	fcntl(elect_recv_sock, F_SETFL, O_NONBLOCK);


	// waiting for the master and reseting the timeout counter and if needed answering the request
	while (1)
	{
		printf("listen: wait for message:\n");
		// Recieve msg from master,
		int return_recv = recvfrom(recv_mast_sock, &recvBuff[0], 1, 0,
				(struct sockaddr*) &cli_addr, &cli_len);
		printf("listen: received : %c \n", recvBuff[0]);

		// TODO (kami#5#): check if cases are all what is needed
		switch (recvBuff[0])
		{
		case 'i': ///identify self : send your Type to caller
			printf("send:%d",
					(int) sendto(recv_mast_sock, &board_type, 1, 0,
							(struct sockaddr*) &cli_addr, cli_len));
			//no break TODO remember no brake;
		case 'k': ///keepalive
			if (pthread_mutex_lock(&(timeout_counter->mtx)))
				critErr("listen: mutex_lock:");
			printf("+");
			timeout_counter->var = 0;
			if (pthread_mutex_unlock(&(timeout_counter->mtx)))
				critErr("listen: mutex_unlock:");
			printf("-\n");
			break;
		case 't': //timeout encountered by a node
			printf("timeout signal received");
			if (pthread_mutex_lock(&(timeout_counter->mtx)))
				critErr("listen: mutex_lock:");
			printf("+");
			timeout_counter->var = 0;
			// TODO check if this is the only call of am_I_master
			*am_I_master = elect_master(elect_recv_sock);
			printf("master= %d", *am_I_master);
			if (pthread_mutex_unlock(&(timeout_counter->mtx)))
				critErr("listen: mutex_unlock:");
			printf("-\n");
			break;
		default:
			printf("listen: Unknown symbol: recvBuff[0]=%c", recvBuff[0]);
			close(recv_mast_sock);
			exit(2);
		}
	}
	return 0;
}

int elect_master(int elect_recv_sock)
{
	printf("electing master");
	unsigned char typeMAC_self[TYPE_MAC_LENGTH];
	typeMAC_self[0] = FPGATYPE;
	int i;
	for (i = 0; i < 6; i++)
	{
		typeMAC_self[i + 1] = mac[i];
	}

	unsigned char typeMAC_other[TYPE_MAC_LENGTH];

	int elect_send_sock;
	struct sockaddr_in elect_addr;
	socklen_t elect_send_len = sizeof(elect_addr);
//create UDP-Socket Server
	if ((elect_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:send_master_socket=");
	}
	//Broadcast socket address
	fillSockaddrBroad(&elect_addr, UDP_ELECT_M_PORT);
	int broadcastEnable=1;
	int ret=setsockopt(elect_send_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

//	if ((bind(elect_send_sock, (struct sockaddr*) &elect_addr,
//			sizeof(elect_addr))) < 0)
//	{
//		critErr("main:bind elect_sock:");
//	}

	sendto(elect_send_sock, &typeMAC_self[0], sizeof(typeMAC_self), 0,
			(struct sockaddr*) &elect_addr, elect_send_len);
	printf("elect: broadcast sent: type and mac\n");
	//close(elect_send_sock);

	int return_recv, best = 1, timeout = 3;
	while (best == 1 && timeout > 0)
	{
		// Receive msg from other boards
		return_recv = recvfrom(elect_recv_sock, &typeMAC_other[0],
				sizeof(typeMAC_self), 0, NULL, NULL);

		printf("returnrecv=%d\t",return_recv);
		if (return_recv != sizeof(typeMAC_self))
		{
			if (return_recv == -1)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same leaving both if there should be a problem
				{
					timeout--;
					printf("nonblock\n");
					if (timeout > 0)
						sleep(1);
				}
				else
					critErr("elect:read broadcast socket:");
			}
			else
			{
				printf("typeMAC_other received has the wrong size");
				exit(3); //should be exclusive for this
			}
		}
		else
		{
			i = 0;
			while (best == 1 && i < TYPE_MAC_LENGTH)
			{
				if (typeMAC_other[i] < typeMAC_self[i])
				{
					best = 0;
					printf(
							"I'm not the best;i=%d\t typeMAC_other[i]=%d \ttypeMAC_self[i]=%d\n",
							i, typeMAC_other[i], typeMAC_self[i]);
				}
				else
				printf("I'm the best;i=%d\t typeMAC_other[i]=%d \ttypeMAC_self[i]=%d\n",
											i, typeMAC_other[i], typeMAC_self[i]);
				i++;
			}
		}
	}
	//flush the socket
	while (return_recv >= 0)
	{
		return_recv = recvfrom(elect_recv_sock, &typeMAC_other[0],
				sizeof(typeMAC_self), 0, NULL, NULL);
		printf("flush\n");
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

return best;
}
