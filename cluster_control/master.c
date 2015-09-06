/*
 * master.c
 *
 *  Created on: Jun 23, 2015
 *      Author: christian
 */

#include "master.h"

extern uint32_t ownIP;

#define BUFFERSIZE 1024

void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32, uint8_t *typeAndGroup)
{
	int size_i;
	void *newList_ptr;
	if (clusterInfo_ptr->num_nodes_i >= clusterInfo_ptr->size_i)
	{
		size_i = clusterInfo_ptr->size_i + REALLOC_STEPSIZE;
		newList_ptr = realloc(clusterInfo_ptr->node_data_list_ptr, size_i * sizeof(struct node_data));
		if (newList_ptr != NULL)
		{
			clusterInfo_ptr->node_data_list_ptr = newList_ptr;
			clusterInfo_ptr->size_i = size_i;
		}
		else
		{
			free(clusterInfo_ptr->node_data_list_ptr);
			printf("couldn't realloc node_data_list");
			exit(5);
		}
	}
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].ip_u32 = ip_u32;
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].type_u8 = typeAndGroup[0];
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].group_u8 = typeAndGroup[1];
	clusterInfo_ptr->node_data_list_ptr->lastAlive_u8 = clusterInfo_ptr->alive_count_u8;
	clusterInfo_ptr->node_data_list_ptr->nowActive_u8 = 0;
	printf("ip=%u \t type = %u added\n",
			clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].ip_u32, typeAndGroup[0]);
	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	clusterInfo_ptr->num_nodes_i++;
	pthread_mutex_unlock(&clusterInfo_ptr->mtx);
}

void readIdentifyAnswers(int receive_sock, struct cluster_info *clusterInfo_ptr, uint8_t newList_u8)
{
	int timeout_i = TIMEOUT, returnRecv_i;
	uint8_t typeAndGroup[2];
	struct sockaddr_in response_addr;
	socklen_t response_len;
	struct node_data *searchReturn_ptr;
	while (timeout_i > 0)
	{
		// Receive msg from other boards
		returnRecv_i = recvfrom(receive_sock, &typeAndGroup, sizeof typeAndGroup, 0,
				(struct sockaddr*) &response_addr, &response_len);
		if (returnRecv_i != 2)
		{
			if (returnRecv_i == -1)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same. leaving both if there should be a problem
				{
					timeout_i--;
					if (timeout_i > 0)
						sleep(1);
				}
				else
					critErr("elect:read broadcast socket:");
			}
			else
			{
				printf("master:readIdentifyanswers: recvfrom wrongsize");
				exit(4); //should be exclusive for this
			}
		}
		else
		{
			if (newList_u8)
			{
				addNode2List(clusterInfo_ptr, ntohl(response_addr.sin_addr.s_addr), &typeAndGroup[0]);
			}
			else
			{
				searchReturn_ptr = (struct node_data*) bsearch(&(response_addr.sin_addr.s_addr),
						clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->num_nodes_i,
						sizeof(struct node_data), compareNodes);
				if (searchReturn_ptr == NULL)
				{
					addNode2List(clusterInfo_ptr, ntohl(response_addr.sin_addr.s_addr), &typeAndGroup[0]);
					qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->num_nodes_i,
							sizeof(struct node_data), compareNodes);
				}
				else
				{
					searchReturn_ptr->lastAlive_u8 = clusterInfo_ptr->alive_count_u8;
				}
			}
		}

	}
	if (newList_u8)
		qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->num_nodes_i,
				sizeof(struct node_data), compareNodes);

}

int master_control(int mastBroad_sock)
{

	printf("I am master\n");
	char keep_alive_c = KEEP_ALIVE_SIGNAL, identify_node_c = IDENTIFY_SIGNAL;

	pthread_t listenForData_thread, sendDataToSlaves_thread;
	int master_i = 1, identify_counter_i = 1;

	struct cluster_info clusterInfo_sct;
	clusterInfo_sct.node_data_list_ptr = (struct node_data*) malloc(
	EST_NUM_BOARD * sizeof(struct node_data));
	clusterInfo_sct.alive_count_u8 = 1;
	pthread_mutex_init(&clusterInfo_sct.mtx, NULL);

	if (clusterInfo_sct.node_data_list_ptr == NULL)
	{
		free(clusterInfo_sct.node_data_list_ptr);
		printf("couldn't malloc node_data_list");
		exit(5);
	}
	clusterInfo_sct.num_nodes_i = 0;
	clusterInfo_sct.size_i = EST_NUM_BOARD;

	if (pthread_create(&listenForData_thread, NULL, getProgram,
	NULL))
	{
		critErr("pthread_create(getProgram)=");
	}

	//TODO make parallel in the future
	if (pthread_create(&sendDataToSlaves_thread, NULL, distributeData,
	NULL))
	{
		critErr("pthread_create(slave)=");
	}

	struct sockaddr_in broad_addr, recv_addr;
	socklen_t broad_len = sizeof broad_addr, recv_len = sizeof recv_addr;

	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrAny(&recv_addr, UDP_N2M_PORT);

	sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
			(struct sockaddr*) &broad_addr, broad_len);
	readIdentifyAnswers(mastBroad_sock, &clusterInfo_sct, 1);

	while (master_i)
	{
		if (identify_counter_i == 0)
		{
			sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
					(struct sockaddr*) &broad_addr, broad_len);
			updateClusterInfo(&clusterInfo_sct, mastBroad_sock);
		}
		else
		{

			sendto(mastBroad_sock, &keep_alive_c, sizeof keep_alive_c, 0, (struct sockaddr*) &broad_addr,
					broad_len);
		}

		sleep(PING_PERIOD);
		identify_counter_i = (identify_counter_i + 1) % PINGS_PER_IDENTIFY;
		printf("identify_counter=%d\n", identify_counter_i);
	}
	free(clusterInfo_sct.node_data_list_ptr);
	return 0;
}

void updateClusterInfo(struct cluster_info *clusterInfo_ptr, int receive_sock)
{
	printf("starting update\n");
	int i, outdated_i = 0;
//	int timeout_i = TIMEOUT, returnRecv_i;
//
//	struct node_data *searchReturn_ptr;
//	uint8_t boardtype_u8;
//	socklen_t response_len;
	clusterInfo_ptr->alive_count_u8++;

	readIdentifyAnswers(receive_sock, clusterInfo_ptr, 0);

	for (i = 0; i < clusterInfo_ptr->num_nodes_i; i++)
	{
		if (clusterInfo_ptr->node_data_list_ptr[i].lastAlive_u8 != clusterInfo_ptr->alive_count_u8)
		{
			clusterInfo_ptr->node_data_list_ptr[i].ip_u32 = -1;
			clusterInfo_ptr->node_data_list_ptr->nowActive_u8 = 0;
			outdated_i++;
		}
	}
	qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->num_nodes_i, sizeof(struct node_data),
			compareNodes);
	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	clusterInfo_ptr->num_nodes_i = clusterInfo_ptr->num_nodes_i - outdated_i;
	pthread_mutex_unlock(&clusterInfo_ptr->mtx);

}

void *send_info(void *send_info_args)
{
	struct send_info *send_info_ptr = send_info_args;
	uint8_t check_u8;
	size_t sendBuff = send_info_ptr->file_size;

	//TODO 1 Parse xml file and get info about file

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(TCP_RECV_INFO_PORT);
	dest_addr.sin_addr.s_addr = send_info_ptr->IP; //has to be converted already

	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		printf("slave: recv_info: socketerror:%s\n", strerror(errno));
	int return_connect = connect(return_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
	if (return_connect < 0)
		critErr("master: send_info: connecterror");
	do
	{
		int return_send = send(return_socket, &sendBuff, sizeof(sendBuff), 0);
		if (return_send < 0)
			printf("master: send_info: senderror:%s", strerror(errno));

		int return_recv = recv(return_socket, &check_u8, 1, 0);
		if (return_recv < 0)
			printf("recverror:%s", strerror(errno));

		if (check_u8 != CHECK_OKAY)
			printf("master: send_info: check_failed ->retry");
	} while (check_u8 != CHECK_OKAY);
	close(return_socket);
	return NULL;

}
void *send_file(void *send_file_args)
{
	uint32_t *IP = send_file_args;
	struct send_file *file_info_ptr = send_file_args;
	uint8_t check_u8;
	int i;

	char sendBuff[BUFFERSIZE];
	memset(sendBuff, '0', sizeof(sendBuff));
	int return_send;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(TCP_RECV_ARCHIVE_PORT);
	dest_addr.sin_addr.s_addr = *IP;
	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		critErr("master:send_info: socket error:");
	do
	{

		//TODO verify if this works
		int return_connect = connect(return_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

		if (return_connect < 0)
			critErr("master:send_info:connect error:");

		FILE * pFile;

		size_t result;

		pFile = fopen(ARCHIVE_NAME, "rb");
		if (pFile == NULL)
			critErr("master:send_info: file open:");
		do
		{
			// copy the file into the buffer:
			result = fread(sendBuff, 1, BUFFERSIZE, pFile);
			if (result != BUFFERSIZE && feof(pFile) == 0)
			{
				printf("master:send_file:Reading error");
				exit(-1);
			}
			return_send = send(return_socket, sendBuff, result, 0);
			if (return_send < 0)
				printf("master: send_file: senderror:%s", strerror(errno));
			else
				printf("send data = %d\n", return_send);
		} while (feof(pFile) == 0);
		// terminate
		fclose(pFile);
		printf("master:send_file:file_sent:%s", file_info_ptr->filename);

		recv(return_socket, &check_u8, 1, 0);

		if (close(return_socket) < 0)
			printf("closeerror:%s", strerror(errno));
		if (check_u8 != CHECK_OKAY)
		{
			printf("master: send_info: check_failed ->retry");
			sleep(1);
		}
	} while (check_u8 != CHECK_OKAY);
	return NULL;
}

void* start(void *start_args)
{
	//TODO free this pointer
	int *return_ptr = malloc(sizeof(int));
	struct cluster_info *clusterInfo_ptr = start_args;
	int *values;
	int restNodes;
	char filename[10] = "test.xml";
	xmlDocPtr inputXML = xmlParseFile(filename), outputXML = NULL;
	if (inputXML == NULL)
	{
		printf("XML File not correct");
		*return_ptr = -1;
		return return_ptr;
	}
	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	*values = XMLGetMinNodeAndTotalWeight(inputXML);

	if (values[MIN_SHIFT] > clusterInfo_ptr->num_nodes_i)
	{
		printf("too few nodes available\t available =%d\t needed =%d\n", clusterInfo_ptr->num_nodes_i,
				values[MIN_SHIFT]);
		pthread_mutex_unlock(&clusterInfo_ptr->mtx);
		*return_ptr = -1;
		return return_ptr;
	}
	else
	{
		outputXML = buildCompleteXML(inputXML, clusterInfo_ptr, values);
		pthread_mutex_unlock(&clusterInfo_ptr->mtx);
	}

	XMLCleanup(inputXML, outputXML, values);

	return NULL;
}

void * getProgram(void * args)
{
	int recvReturn_i;
	FILE * pFile;
	int waitForBroadcast_sock, openConnection_sock, received = 0;
	char listenBuff[10], recvBuffer[BUFFERSIZE], ack[4] = "ack";

	struct sockaddr_in listen_addr, connect_addr;
	socklen_t listen_len = sizeof listen_addr, connect_len = sizeof connect_addr;

	if ((waitForBroadcast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("master:getProgram:wait_socket=");
	}

	if ((openConnection_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		critErr("master:getProgram:connect_socket=");
	}
	fillSockaddrAny(&listen_addr, UDP_OPEN_TCP_CONNECTION_FOR_DATA_TRANSFER);
	if ((bind(waitForBroadcast_sock, (struct sockaddr*) &listen_addr, sizeof(listen_addr))) < 0)
	{
		critErr("master:getProgram:bind elect_recv_sock:");
	}
	do
	{
		printf("waiting for conection\n");
		recvReturn_i = recvfrom(waitForBroadcast_sock, &listenBuff[0], sizeof listenBuff, 0,
				(struct sockaddr*) &connect_addr, &connect_len);
		printf("received:%s",listenBuff);
		if (!strcmp(listenBuff, "fetch"))
		{
			sendto(waitForBroadcast_sock, &ack, sizeof ack, 0, (struct sockaddr*) &connect_addr,
					connect_len);
			printf("connecting\n");
			int return_connect = connect(openConnection_sock, (struct sockaddr*) &connect_addr,
					connect_len);
			if (return_connect < 0)
				critErr("master: getProgram: connecterror");
			pFile = fopen("data.tar.gz", "wb");
			if (pFile == NULL)
			{
				fputs("File error", stderr);
				exit(1);
			}
			printf("receiving file\n");
			do
			{
				recvReturn_i = recv(openConnection_sock, &recvBuffer[0], BUFFERSIZE, 0);
				if (recvReturn_i < 0)
					printf("recverror:%s\n", strerror(errno));
				else
					printf("recv data = %d", recvReturn_i);

				fwrite(&recvBuffer[0], 1, recvReturn_i, pFile);
			} while (recvReturn_i == BUFFERSIZE && feof(pFile) == 0); //TODO maybe change to EOF check

			fclose(pFile);
			close(openConnection_sock);
			received =1;
		}
	} while (received);
	close(waitForBroadcast_sock);

	// w8 for broadcast and open tcp connection to sender to fetch the data archive, decompress archive
	return NULL;
}
void * distributeData(void * args)
{
	xmlDocPtr doc = args;

	//go through the XML doc to distribute the data to the nodes in new packed archives
	return NULL;
}
