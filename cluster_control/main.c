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
#include "XML.h"

uint8_t mac[6];
uint32_t ownIP; //in network format (htonl/inet_addr used)

int main()
{

	getMAC(mac);
	ownIP = ntohl(getIP());

	printf("ownIP = %u\n",ownIP);



	//xmlDocPtr inputXML = xmlParseFile(filename);

//	struct cluster_info clusterInfo_sct;
//	clusterInfo_sct.node_data_list_ptr = (struct node_data*) malloc(
//		EST_NUM_BOARD * sizeof(struct node_data));
//		clusterInfo_sct.alive_count_u8 = 1;
//		pthread_mutex_init(&clusterInfo_sct.mtx,NULL);
//		clusterInfo_sct.numNodes_size = 0;
//			clusterInfo_sct.size = EST_NUM_BOARD;
//			uint8_t typeAndGroup[2]= {1,1};
//			addNode2List(&clusterInfo_sct,0x11111111,typeAndGroup);
//			addNode2List(&clusterInfo_sct,0x11111112,typeAndGroup);
//			addNode2List(&clusterInfo_sct,0x11111113,typeAndGroup);
//			addNode2List(&clusterInfo_sct,0x11111114,typeAndGroup);
//			addNode2List(&clusterInfo_sct,0x11111115,typeAndGroup);
//			addNode2List(&clusterInfo_sct,0x11111116,typeAndGroup);
//			addNode2List(&clusterInfo_sct,0x11111110,typeAndGroup);
//			addNode2List(&clusterInfo_sct,ownIP,typeAndGroup);
//			addNode2List(&clusterInfo_sct,0x10101010,typeAndGroup);
//
//			struct node_data compare_node;
//			compare_node.ip_u32=0x11111110;
//			printf("%d\n",clusterInfo_sct.size);
//			printf("ownIP =%X,%p \t ipinlist=%X,%p\n",ownIP,&ownIP,clusterInfo_sct.node_data_list_ptr[7].ip_u32,&clusterInfo_sct.node_data_list_ptr[7].ip_u32);
//			printf("compare11 = %d",compareNodes(clusterInfo_sct.node_data_list_ptr,&clusterInfo_sct.node_data_list_ptr[0]));
//			printf("compare21 = %d",compareNodes(&clusterInfo_sct.node_data_list_ptr[1],&clusterInfo_sct.node_data_list_ptr[0]));
//			printf("compare12 = %d\n",compareNodes(&clusterInfo_sct.node_data_list_ptr[0],&clusterInfo_sct.node_data_list_ptr[1]));
//			qsort(clusterInfo_sct.node_data_list_ptr, clusterInfo_sct.numNodes_size, sizeof(struct node_data),
//						compareNodes);
//			printf("search = %p\n",bsearch(&compare_node,clusterInfo_sct.node_data_list_ptr,clusterInfo_sct.numNodes_size,sizeof(struct node_data),compareNodes));
//			printf("search = %p\n",bsearch(&clusterInfo_sct.node_data_list_ptr[8],clusterInfo_sct.node_data_list_ptr,clusterInfo_sct.numNodes_size,sizeof(struct node_data),compareNodes));
//			printf("finish");
//			return 0;
//
//
//	int *nodeAndWeight = XMLGetMinNodeAndTotalWeight(inputXML);
//	printf("weight = %d, minNodes = %d",nodeAndWeight[WEIGHT_SHIFT],nodeAndWeight[MIN_SHIFT]);
//
//	xmlDocPtr outputXML = buildCompleteXML(inputXML,&clusterInfo_sct,nodeAndWeight);
//	free(clusterInfo_sct.node_data_list_ptr);
//	XMLCleanup(inputXML,outputXML,nodeAndWeight);
//
//	return 0;
	int master_i = 0; // 0=no, 1=yes
	uint8_t subgroup_u8 = CLUSTERGROUP;
	uint64_t MAC = MACtoDecimal(mac);
	srand((unsigned int) MAC);
	struct timespec rnd_time =
	{ 0, 0 };

	int mastBroad_sock;

	struct sockaddr_in broad_addr, loop_addr, any_addr;
	socklen_t broad_len = sizeof broad_addr, loop_len = sizeof loop_addr;

	struct var_mtx timeCount_mtx_sct =
	{ 1, PTHREAD_MUTEX_INITIALIZER };
	struct slave_args slaveMain_args =
	{ &timeCount_mtx_sct, &master_i, &subgroup_u8 };

	pthread_t slave_thread;
	if (pthread_create(&slave_thread, NULL, slave_main, (void*) &slaveMain_args))
	{
		critErr("pthread_create(slave)=");
	}

	//create UDP-Socket Server
	if ((mastBroad_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("main:send_master_socket=");
	}

//	fillSockaddrAny(&any_addr, UDP_NODE_LISTEN_PORT);
//
//		if ((bind(mastBroad_sock, (struct sockaddr*) &any_addr, sizeof any_addr)) < 0)
//		{
//			critErr("main:bind rmastBroad_sock:");
//		}
	fcntl(mastBroad_sock, F_SETFL, O_NONBLOCK);
	int broadcastEnable = 1;
	int ret = setsockopt(mastBroad_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
			sizeof(broadcastEnable));
	if (ret)
		critErr("main: couldn't set setsockopt:");

	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrLoop(&loop_addr, UDP_NODE_LISTEN_PORT);

	// Timeout-counter for master communication
	while (1)
	{
		if (master_i)
		{
			printf("main:I am master and start control function counter mutex is not locked\n");
			master_main(mastBroad_sock);
			master_i = 0;
		}
		if (pthread_mutex_lock(&timeCount_mtx_sct.mtx))
			critErr("main: mutex_lock:");
		//printf("<");

		if (timeCount_mtx_sct.var > PING_PERIOD * TIMEOUT_PERIODS)
		{
			printf("main:timecount_over_limit:%d\n", timeCount_mtx_sct.var);
			if (pthread_mutex_unlock(&timeCount_mtx_sct.mtx))
				critErr("main: over_mutex_unlock:");
			//printf(">\n");

			//wait for 0-990ms(10ms spacing) to prevent broadcast flood
			rnd_time.tv_nsec = ((long) (rand() % 100)) * 10000000L;
			nanosleep(&rnd_time, NULL);

			//still no master?
			if (pthread_mutex_lock(&timeCount_mtx_sct.mtx))
				critErr("main:no_master mutex_lock:");
			//printf("<");
			if (timeCount_mtx_sct.var != 0) //TODO has this to be volatile? and this expression should suffice
			{
				if (pthread_mutex_unlock(&timeCount_mtx_sct.mtx))
					critErr("main:no_master mutex_unlock:");
				//printf(">");

				char timeout_detected = 't';
				sendto(mastBroad_sock, &timeout_detected, 1, 0, (struct sockaddr*) &broad_addr, broad_len);
				sendto(mastBroad_sock, &timeout_detected, 1, 0, (struct sockaddr*) &loop_addr, loop_len);
				printf("timeout signal sent\n");

			}
			else
			{
				if (pthread_mutex_unlock(&timeCount_mtx_sct.mtx))
					critErr("main:no_master mutex_unlock:");
				//printf(">\n");
			}

		}
		else
		{
			timeCount_mtx_sct.var++;
		printf("main:increase timeCount_mtx_sct to:%d\n", timeCount_mtx_sct.var);
			if (pthread_mutex_unlock(&timeCount_mtx_sct.mtx))
				critErr("main: under_mutex_unlock:");
			//printf(">\n");
		}
		// TODO (kami#9#): may use PING_PERIOD here
		sleep(1);
	}

	return 0;
}
