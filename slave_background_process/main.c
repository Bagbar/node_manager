
/* Zedboard Cluster management process
 *
 * all defines and options are in basics.h
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "basics.h"
#include "slave.h"
#include "master.h"

uint8_t mac[6];

int main()
{

	getMAC(mac);
	int am_I_master = 0; // 0=no, 1=yes
	uint64_t MAC = MACtoDecimal(mac);
	srand((unsigned int) MAC);
	struct timespec rnd_time =
	{ 0, 0 };
//	struct node_info node_list =
//	{
//			malloc(EST_NUM_BOARD*sizeof(short)),
//			malloc(EST_NUM_BOARD),
//			malloc(EST_NUM_BOARD*4),
//			EST_NUM_BOARD,
//			0
//	};

	int mast_broad_sock;

	struct sockaddr_in broad_addr,loop_addr;
	socklen_t broad_len = sizeof broad_addr, loop_len = sizeof loop_addr ;

	struct var_mtx time_count =
	{ 1, PTHREAD_MUTEX_INITIALIZER };
	struct thread_args bg_listen_args =
	{ &time_count, &am_I_master };

	pthread_t background_listen;
	if (pthread_create(&background_listen, NULL, listen_for_master,
			(void*) &bg_listen_args))
	{
		critErr("pthread_create(listen)=");
	}

	//create UDP-Socket Server
	if ((mast_broad_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("listen:send_master_socket=");
	}fcntl(mast_broad_sock, F_SETFL, O_NONBLOCK);
	int broadcastEnable=1;
	int ret=setsockopt(mast_broad_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

	//Broadcast socket address TODO corresponding master
	fillSockaddrBroad(&broad_addr,UDP_NODE_LISTEN_PORT);
	fillSockaddrLoop(&loop_addr,UDP_NODE_LISTEN_PORT);

	/*if ((bind(mast_broad_sock, (struct sockaddr*) &broad_addr,
			sizeof(broad_addr))) < 0)
	{
		critErr("main:bind mast_broad_sock:");
	}*/

	// Timeout-counter for master communication
	while (1)
	{

		if (pthread_mutex_lock(&time_count.mtx))
			critErr("main: mutex_lock:");
		printf("<");
		if (am_I_master)
		{
			printf("I am master and start control function mutex is locked\n");
			master_control(mast_broad_sock);
		}
		if (time_count.var > PING_PERIOD * TIMEOUT_PERIODS)
		{
			printf("main:timecount_over_limit:%d\n", time_count.var);
			// TODO check for outsourcing unlock together with else
			if (pthread_mutex_unlock(&time_count.mtx))
				critErr("main: over_mutex_unlock:");
			printf(">\n");

			//wait for 0-990ms(10ms spacing) to prevent broadcast flood
			rnd_time.tv_nsec = ((long) (rand() % 100)) * 10000000L;
			nanosleep(&rnd_time, NULL);

			//still no master?
			if (pthread_mutex_lock(&time_count.mtx))
				critErr("main:no_master mutex_lock:");
			printf("<");
			if (time_count.var != 0) //TODO has this to be volatile? and this expression should suffice
			{
				if (pthread_mutex_unlock(&time_count.mtx))
					critErr("main:no_master mutex_unlock:");
				printf(">");

				char timeout_detected = 't';
				sendto(mast_broad_sock, &timeout_detected, 1, 0,
						(struct sockaddr*) &broad_addr, broad_len);
				//sendto(mast_broad_sock, &timeout_detected, 1, 0,
				//						(struct sockaddr*) &loop_addr, loop_len);
				printf("timeout signal sent\n");
				//TODO check if sender receives his broadcast

			}
			else
			{if (pthread_mutex_unlock(&time_count.mtx))
				critErr("main:no_master mutex_unlock:");
			printf(">\n");
			}



		}
		else
		{
			printf("main:increase time_count to:%d\n", ++time_count.var);
			if (pthread_mutex_unlock(&time_count.mtx))
				critErr("main: under_mutex_unlock:");
			printf(">\n");
		}
		// TODO (kami#9#): may use PING_PERIOD here
		sleep(1);
	}

	return 0;
}
