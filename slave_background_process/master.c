/*
 * master.c
 *
 *  Created on: Jun 23, 2015
 *      Author: christian
 */

#include "master.h"

#define BUFFERSIZE 1024

void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32,
		uint8_t *typeAndGroup)
{
	int size_i;
	void *newList_ptr;
	if (clusterInfo_ptr->num_nodes_i >= clusterInfo_ptr->size_i)
	{
		size_i = clusterInfo_ptr->size_i + REALLOC_STEPSIZE;
		newList_ptr = realloc(clusterInfo_ptr->node_data_list_ptr,
				size_i * sizeof(struct node_data));
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
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].ip_u32 =
			ip_u32;
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].type_u8 =
			typeAndGroup[0];
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].group_u8 =
			typeAndGroup[1];
	clusterInfo_ptr->node_data_list_ptr->lastAlive_u8 =
			clusterInfo_ptr->alive_count_u8;
	clusterInfo_ptr->node_data_list_ptr->nowActive_u8 = 0;
	printf("ip=%u \t type = %u added\n",
			clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].ip_u32,
			typeAndGroup[0]);
	clusterInfo_ptr->num_nodes_i++;
}

void readIdentifyAnswers(int receive_sock, struct cluster_info *clusterInfo_ptr,
		uint8_t newList_u8)
{
	int timeout_i = TIMEOUT, returnRecv_i;
	uint8_t typeAndGroup[2];
	struct sockaddr_in response_addr;
	socklen_t response_len;
	struct node_data *searchReturn_ptr;
	while (timeout_i > 0)
	{
		// Receive msg from other boards
		returnRecv_i = recvfrom(receive_sock, &typeAndGroup,
				sizeof typeAndGroup, 0, (struct sockaddr*) &response_addr,
				&response_len);
		if (returnRecv_i != 1)
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
				printf("master_control: recvfrom wrongsize");
				exit(4); //should be exclusive for this
			}
		}
		else
		{
			if (newList_u8)
			{
				addNode2List(clusterInfo_ptr,
						ntohl(response_addr.sin_addr.s_addr), &typeAndGroup[0]);
			}
			else
			{
				searchReturn_ptr = (struct node_data*) bsearch(
						&(response_addr.sin_addr.s_addr),
						clusterInfo_ptr->node_data_list_ptr,
						clusterInfo_ptr->num_nodes_i, sizeof(struct node_data),
						compareNodes);
				if (searchReturn_ptr == NULL)
				{
					addNode2List(clusterInfo_ptr,
							ntohl(response_addr.sin_addr.s_addr),
							&typeAndGroup[0]);
					qsort(clusterInfo_ptr->node_data_list_ptr,
							clusterInfo_ptr->num_nodes_i,
							sizeof(struct node_data), compareNodes);
				}
				else
				{
					searchReturn_ptr->lastAlive_u8 =
							clusterInfo_ptr->alive_count_u8;
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
	char keep_alive_c = 'k', identify_node_c = 'i';

	int master_i = 1, identify_counter_i = 1;

	struct cluster_info clusterInfo_sct;
	clusterInfo_sct.node_data_list_ptr = (struct node_data*) malloc(
	EST_NUM_BOARD * sizeof(struct node_data));
	clusterInfo_sct.alive_count_u8 = 1;

	struct sockaddr_in broad_addr, recv_addr;
	socklen_t broad_len = sizeof broad_addr, recv_len = sizeof recv_addr;

	if (clusterInfo_sct.node_data_list_ptr == NULL)
	{
		free(clusterInfo_sct.node_data_list_ptr);
		printf("couldn't malloc node_data_list");
		exit(5);
	}
	clusterInfo_sct.num_nodes_i = 0;
	clusterInfo_sct.size_i = EST_NUM_BOARD;

	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrAny(&recv_addr, UDP_N2M_PORT);

//	if ((bind(mastBroad_sock, (struct sockaddr*) &recv_addr, recv_len)) < 0)
//	{
//		critErr("master:bind mastBroad_sock:");
//	}

	sendto(mastBroad_sock, &identify_node_c, 1, 0,
			(struct sockaddr*) &broad_addr, broad_len);
	readIdentifyAnswers(mastBroad_sock, &clusterInfo_sct, 1);
//	while (timeout_i > 0)
//	{
//		// Receive msg from other boards
//		return_recv_i = recvfrom(mast_broad_sock, &boardtype_u8, 1, 0,
//				(struct sockaddr*) &response_addr, &response_len);
//		if (return_recv_i != 1)
//		{
//			if (return_recv_i == -1)
//			{
//				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same. leaving both if there should be a problem
//				{
//					timeout_i--;
//					if (timeout_i > 0)
//						sleep(1);
//				}
//				else
//					critErr("elect:read broadcast socket:");
//			}
//			else
//			{
//				printf("master_control: recvfrom wrongsize");
//				exit(4); //should be exclusive for this
//			}
//		}
//		else
//		{
//			addNode2List(&clusterInfo_str, ntohl(response_addr.sin_addr.s_addr),
//					boardtype_u8);
//		}
//
//	}
//
//	qsort(clusterInfo_str.node_data_list_ptr, clusterInfo_str.num_nodes_i,
//			sizeof(struct node_data), compareNodes);

	do
	{
		if (identify_counter_i == 0)
		{
			sendto(mastBroad_sock, &identify_node_c, 1, 0,
					(struct sockaddr*) &broad_addr, broad_len);
			updateClusterInfo(&clusterInfo_sct, mastBroad_sock);
		}
		else
		{

			sendto(mastBroad_sock, &keep_alive_c, 1, 0,
					(struct sockaddr*) &broad_addr, broad_len);
		}
		sleep(PING_PERIOD);
		identify_counter_i = (identify_counter_i + 1) % PINGS_PER_IDENTIFY;
		printf("identify_counter=%d\n", identify_counter_i);
	} while (master_i);
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
//	while (timeout_i > 0)
//	{
//		// Receive msg from other boards
//		returnRecv_i = recvfrom(receive_sock, &boardtype_u8, 1, 0,
//				(struct sockaddr*) &response_addr, &response_len);
//		if (returnRecv_i != 1)
//		{
//			if (returnRecv_i == -1)
//			{
//				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same. leaving both if there should be a problem
//				{
//					timeout_i--;
//					if (timeout_i > 0)
//						sleep(1);
//				}
//				else
//					critErr("elect:read broadcast socket:");
//			}
//			else
//			{
//				printf("master_control: recvfrom wrongsize");
//				exit(4); //should be exclusive for this
//			}
//		}
//		else
//		{
//			searchReturn_ptr = (struct node_data*) bsearch(
//					&(response_addr.sin_addr.s_addr), clusterInfo_ptr->node_data_list_ptr,
//					clusterInfo_ptr->num_nodes_i, sizeof(struct node_data), compareNodes);
//			if (searchReturn_ptr == NULL)
//			{
//				addNode2List(clusterInfo_ptr, ntohl(response_addr.sin_addr.s_addr),
//						boardtype_u8);
//				qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->num_nodes_i,
//						sizeof(struct node_data), compareNodes);
//			}
//			else
//			{
//				searchReturn_ptr->lastAlive_u8 = clusterInfo_ptr->alive_count_u8;
//			}
//		}
//	}

	for (i = 0; i < clusterInfo_ptr->num_nodes_i; i++)
	{
		if (clusterInfo_ptr->node_data_list_ptr[i].lastAlive_u8
				!= clusterInfo_ptr->alive_count_u8)
		{
			clusterInfo_ptr->node_data_list_ptr[i].ip_u32 = -1;
			clusterInfo_ptr->node_data_list_ptr->nowActive_u8 = 0;
			outdated_i++;
		}
	}
	qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->num_nodes_i,
			sizeof(struct node_data), compareNodes);
	clusterInfo_ptr->num_nodes_i = clusterInfo_ptr->num_nodes_i - outdated_i;

}

void *send_info(void *send_info_args)
{
	uint8_t check_u8;
	size_t sendBuff[4];
	struct send_info *send_info_ptr = send_info_args;
	//TODO 1 Parse xml file and get info about file
	sendBuff[WORK_SHIFT] = send_info_ptr->work_size;
	sendBuff[BIT_SHIFT] = send_info_ptr->bit_size;
	sendBuff[DRIVER_SHIFT] = send_info_ptr->driver_size;
	sendBuff[3] = sendBuff[WORK_SHIFT] + sendBuff[BIT_SHIFT]
			+ sendBuff[DRIVER_SHIFT];

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(TCP_RECV_INFO_PORT);
	dest_addr.sin_addr.s_addr = send_info_ptr->IP; //has to be converted already

	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		printf("slave: recv_info: socketerror:%s\n", strerror(errno));
	int return_connect = connect(return_socket, (struct sockaddr*) &dest_addr,
			sizeof(dest_addr));
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
	close(return_socket); //TODO cancel
	return NULL;

}
void *send_file(void *send_file_args)
{

	struct send_file *file_info_ptr = send_file_args;
	uint8_t check_u8;
	uint16_t port_u16;
	int i;

	switch (file_info_ptr->filetype_i)
	{
	case WORK_SHIFT:
		port_u16 = TCP_RECV_WORK_PORT;
		break;
	case BIT_SHIFT:
		port_u16 = TCP_RECV_BITSTREAM_PORT;
		break;
	case DRIVER_SHIFT:
		port_u16 = TCP_RECV_DRIVER_PORT;
		break;
	default:
		critErr("slave recv_file= filetype unknown");
	}

	char sendBuff[BUFFERSIZE];
	memset(sendBuff, '0', sizeof(sendBuff));
	int return_send;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port_u16);
	dest_addr.sin_addr.s_addr = file_info_ptr->IP;
	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		printf("socketerror:%s", strerror(errno));
	do
	{

		//TODO verify if this works
		int return_connect = connect(return_socket,
				(struct sockaddr*) &dest_addr, sizeof(dest_addr));

		if (return_connect < 0)
			printf("connecterror:%s", strerror(errno));

		FILE * pFile;

		size_t result;

		pFile = fopen(&(file_info_ptr->filename[0]), "rb");
		if (pFile == NULL)
		{
			fputs("File error", stderr);
			exit(1);
		}

		do
		{
			// copy the file into the buffer:
			result = fread(sendBuff, 1, BUFFERSIZE, pFile);
			if (result != BUFFERSIZE && feof(pFile) == 0)
			{
				critErr("master:send_file:Reading error");
			}
			return_send = send(return_socket, sendBuff, result, 0);
			if (return_send < 0)
				printf("master: send_file: senderror:%s", strerror(errno));
			//TODO catch
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
			sleep(2);
		}
	} while (check_u8 != CHECK_OKAY);
	return NULL;
}

void readXML(struct XML_args){
}
}
