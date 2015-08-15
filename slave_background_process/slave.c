/*
 * slave.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */

#include "slave.h"

#define BUFFERSIZE 10
#define IDENTIFIER_LENGTH 8 // 1 for type + 6 for MAC + 1 for subgroup
#define EVERYTHING_OKAY 7 //bit0+bit1+bit2

//FIXME is this okay??
extern uint8_t mac[6];
//receives commands from master for managing the cluster and starts the threads for the actual work
void *slave_main(void *args_ptr)
///  network I/O function for the control communication with the master
{
	//Variable declarations
	struct var_mtx *timeoutCounter_ptr =
			((struct slave_args*) args_ptr)->timeout_count;
	int *master_ptr = ((struct slave_args*) args_ptr)->master_ptr; //TODO check if better with or without cast
	uint8_t boardtype_u8 = FPGATYPE;
	uint8_t *subgroup_ptr = ((struct slave_args*) args_ptr)->subgroup_ptr;
	uint8_t typeAndGroup[2];
	int recvReturn_i;
	char recvBuff[BUFFERSIZE];
	memset(recvBuff, '0', BUFFERSIZE);

	struct recv_info recv_info_sct;
	recv_info_sct.bit_size = 0;
	recv_info_sct.driver_size = 0;
	recv_info_sct.work_size = 0;
	//trans_info_sct.status_okay = 0;

	pthread_t recv_info_thread;
	if (pthread_create(&recv_info_thread, NULL, receive_info,
			(void*) &recv_info_sct))
	{
		critErr("slave:pthread_create(recv_info)=");
	}

	int recvMast_sock, electRecv_sock;

	struct sockaddr_in serv_addr, elect_recv_addr, cli_addr;
	socklen_t cli_len;

	//create UDP-Socket receiver for control commands
	if ((recvMast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:socket=");
	}

	//listen socket address and bind for master control messages
	fillSockaddrAny(&serv_addr, UDP_NODE_LISTEN_PORT);

	if ((bind(recvMast_sock, (struct sockaddr*) &serv_addr, sizeof serv_addr))
			< 0)
	{
		critErr("listen:bind recv_mast_sock:");
	}
	//create UDP-Socket receiver for fetching the type_and_MAC broadcasts
	if ((electRecv_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:elect_recv_sock=");
	}

	//listen socket address and bind for electing master
	fillSockaddrAny(&elect_recv_addr, UDP_ELECT_M_PORT);

	if ((bind(electRecv_sock, (struct sockaddr*) &elect_recv_addr,
			sizeof(elect_recv_addr))) < 0)
	{
		critErr("listen:bind elect_recv_sock:");
	}
	//non-blocking recv
	fcntl(electRecv_sock, F_SETFL, O_NONBLOCK);

	// waiting for the master and reseting the timeout counter and if needed answering the request
	// should be an endless loop
	while (1)
	{
		printf("listen: wait for message:\n");
		// Recieve msg from master,
		recvReturn_i = recvfrom(recvMast_sock, &recvBuff, 1, 0,
				(struct sockaddr*) &cli_addr, &cli_len);
		printf("listen: received : %c \n", recvBuff[0]);

		switch (recvBuff[0])
		{
		case 'i': ///identify self : send your Type to caller
			typeAndGroup[0] = boardtype_u8;
			typeAndGroup[1] = *subgroup_ptr;
			printf("send:%d\n",
					(int) sendto(recvMast_sock, &typeAndGroup[0],
							sizeof typeAndGroup, 0,
							(struct sockaddr*) &cli_addr, cli_len));
			//no break
		case 'k': ///keepalive
			if (pthread_mutex_lock(&(timeoutCounter_ptr->mtx)))
				critErr("listen: mutex_lock:");
			printf("+");
			timeoutCounter_ptr->var = 0;
			if (pthread_mutex_unlock(&(timeoutCounter_ptr->mtx)))
				critErr("listen: mutex_unlock:");
			printf("-\n");
			break;
		case 't': //timeout encountered by a node
			printf("timeout signal received");
			if (pthread_mutex_lock(&(timeoutCounter_ptr->mtx)))
				critErr("listen: mutex_lock:");
			printf("+");
			timeoutCounter_ptr->var = 0;
			// TODO check if this is the only call of am_I_master
			*master_ptr = elect_master(electRecv_sock);
			printf("master= %d", *master_ptr);
			if (pthread_mutex_unlock(&(timeoutCounter_ptr->mtx)))
				critErr("listen: mutex_unlock:");
			printf("-\n");
			break;
		default:
			printf("listen: Unknown symbol: recvBuff[0]=%c", recvBuff[0]);
			close(recvMast_sock);
			exit(2);
		}

	}

	return NULL;
}

int elect_master(int electRecv_sock)
{
	printf("electing master");
	uint8_t typeMAC_self[IDENTIFIER_LENGTH];
	typeMAC_self[0] = FPGATYPE;
	typeMAC_self[7] = CLUSTERGROUP;
	int i;
	for (i = 0; i < 6; i++)
	{
		typeMAC_self[i + 1] = mac[i];
	}

	uint8_t typeMAC_other[IDENTIFIER_LENGTH];

	int electSend_sock;
	struct sockaddr_in elect_addr;
	socklen_t electSend_len = sizeof(elect_addr);
//create UDP-Socket Server
	if ((electSend_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:send_master_socket=");
	}
	//Broadcast socket address
	fillSockaddrBroad(&elect_addr, UDP_ELECT_M_PORT);
	int broadcastEnable = 1;
	int ret = setsockopt(electSend_sock, SOL_SOCKET, SO_BROADCAST,
			&broadcastEnable, sizeof(broadcastEnable));
	if (ret)
		critErr("slave: couldn't set setsockopt:");

//	if ((bind(elect_send_sock, (struct sockaddr*) &elect_addr,
//			sizeof(elect_addr))) < 0)
//	{
//		critErr("main:bind elect_sock:");
//	}

	sendto(electSend_sock, &typeMAC_self[0], sizeof(typeMAC_self), 0,
			(struct sockaddr*) &elect_addr, electSend_len);
	printf("elect: broadcast sent: type and mac\n");
	//close(elect_send_sock);

	int recvReturn_i, best_i = 1, timeout_i = 3;
	while (best_i == 1 && timeout_i > 0)
	{
		// Receive msg from other boards
		recvReturn_i = recvfrom(electRecv_sock, &typeMAC_other[0],
				sizeof(typeMAC_self), 0, NULL, NULL);

		printf("returnrecv=%d\t", recvReturn_i);
		if (recvReturn_i != sizeof(typeMAC_self))
		{
			if (recvReturn_i == -1)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same leaving both if there should be a problem
				{
					timeout_i--;
					printf("nonblock\n");
					if (timeout_i > 0)
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
			while (best_i == 1 && i < IDENTIFIER_LENGTH)
			{
				if (typeMAC_other[i] < typeMAC_self[i])
				{
					best_i = 0;
					printf(
							"I'm not the best;i=%d\t typeMAC_other[i]=%d \ttypeMAC_self[i]=%d\n",
							i, typeMAC_other[i], typeMAC_self[i]);
				}
				else
					printf(
							"I'm the best;i=%d\t typeMAC_other[i]=%d \ttypeMAC_self[i]=%d\n",
							i, typeMAC_other[i], typeMAC_self[i]);
				i++;
			}
		}
	}
	//flush the socket
	while (recvReturn_i >= 0)
	{
		recvReturn_i = recvfrom(electRecv_sock, &typeMAC_other[0],
				sizeof(typeMAC_self), 0, NULL, NULL);
		printf("flush\n");
	}

	return best_i;
}

//fetches target IPaddress for processed data, size of work program
void *receive_info(void * transfer_args)
{
	struct recv_info *recv_info_ptr = transfer_args;
	uint8_t check_u8;
	pthread_t  recv_archive_thread,	work_thread;
	struct recv_file recv_work_args, recv_bit_args, recv_driver_args;

	size_t recvBuff;
	int return_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (return_socket < 0)
		printf("slave: recv_info: socketerror:%s\n", strerror(errno));

	struct sockaddr_in addr, client;
	socklen_t len;
	fillSockaddrAny(&addr, TCP_RECV_INFO_PORT);

	int return_bind = bind(return_socket, (struct sockaddr*) &addr,
			sizeof(addr));
	if (return_bind < 0)
		printf("slave: recv_info: binderror:%s\n", strerror(errno));

	listen(return_socket, 1);

	while (1)
	{
		int return_accept = accept(return_socket, (struct sockaddr*) &client,
				&len);
		if (return_accept < 0){
			printf("slave: recv_info: accepterror:%s\n", strerror(errno));
			check_u8=CHECK_FAILED;}
		recv(return_accept, recvBuff, sizeof recvBuff, 0);


			if (recvBuff == 0 )
			{
				pthread_cancel(work_thread);
				check_u8 = WORK_THREAD_CANCELED;
			}
			else
			{
				check_u8 = CHECK_OKAY;
			}

			send(return_accept, &check_u8, 1, 0);
			if (check_u8 == CHECK_OKAY)
			{		}
		return NULL;
	}
}

//fetches the files
void *receive_file(void * recv_file_args)
{
	struct recv_file * file_info_ptr = recv_file_args;

	uint8_t check_u8;
	size_t file_size = 0;
	FILE * pFile;
	int recv_return_i, errorcount_i = 0;
	char recvBuff[BUFFERSIZE];
	memset(recvBuff, '0', sizeof(recvBuff));


	int return_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (return_socket < 0)
		printf("socketerror:%s\n", strerror(errno));

	struct sockaddr_in addr, client;
	socklen_t len;
	fillSockaddrAny(&addr, TCP_RECV_ARCHIVE_PORT);

	int return_bind = bind(return_socket, (struct sockaddr*) &addr,
			sizeof(addr));
	if (return_bind < 0)
		printf("binderror:%s\n", strerror(errno));

	listen(return_socket, 1);
	do
	{
		int return_accept = accept(return_socket, (struct sockaddr*) &client,
				&len);
		if (return_accept < 0)
			printf("accepterror:%s\n", strerror(errno));

		pFile = fopen("data.tar.gz", "wb");
		if (pFile == NULL)
		{
			fputs("File error", stderr);
			exit(1);
		}

		do
		{
			recv_return_i = recv(return_accept, recvBuff, BUFFERSIZE, 0);
			if (recv_return_i < 0)
				printf("recverror:%s\n", strerror(errno));
			else
				printf("recv data = %d", recv_return_i);

			fwrite(recvBuff, 1, recv_return_i, pFile);
			file_size += recv_return_i;
		} while (recv_return_i == BUFFERSIZE && feof(pFile) == 0); //TODO maybe change to EOF check

		fclose(pFile);
		file_info_ptr->recv_size = file_size;
		if (file_info_ptr->expected_size != file_size)
		{
			errorcount_i++;
			check_u8 = CHECK_FAILED;
		}
		else
		{
			check_u8 = CHECK_OKAY;
		}

		send(return_accept,&check_u8,1,0);
		close(return_accept);
	} while (check_u8 == CHECK_FAILED && errorcount_i < 3);
	if (errorcount_i >= 3)
	{
		printf("slave:receive_file: couldn't receive archive");
		exit(6);
	}
	return NULL;
}

void *execute_work(void *args)
{
	system("./work");
	return NULL;
}

