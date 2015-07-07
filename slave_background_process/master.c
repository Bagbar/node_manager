/*
 * masteunr.c
 *
 *  Created on: Jun 23, 2015
 *      Author: christian
 */

#include "master.h"

void master_control(int mast_broad_sock)
{
	while (1)
	{
		printf("I am master\n");
		sleep(1);
	}

}

void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32,
		uint8_t boardtype_u8)
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
			boardtype_u8;
	clusterInfo_ptr->node_data_list_ptr->lastAlive_u8 =
			clusterInfo_ptr->alive_count_u8;
	printf("ip=%d \t type = %d added\n",
			clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->num_nodes_i].ip_u32,
			boardtype_u8);
	clusterInfo_ptr->num_nodes_i++;
}

void readIdentifyAnswers(int mastBroad_sock,
		struct cluster_info *clusterInfo_ptr, uint8_t newList_u8)
{
	int timeout_i = TIMEOUT, returnRecv_i;
	uint8_t boardtype_u8;
	struct sockaddr_in response_addr;
	socklen_t response_len;
	struct node_data *searchReturn_ptr;
	while (timeout_i > 0)
	{
		// Receive msg from other boards
		returnRecv_i = recvfrom(mastBroad_sock, &boardtype_u8, 1, 0,
				(struct sockaddr*) &response_addr, &response_len);
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
				addNode2List(clusterInfo_ptr, ntohl(response_addr.sin_addr.s_addr),
						boardtype_u8);
			}
			else
			{
				searchReturn_ptr = (struct node_data*) bsearch(
						&(response_addr.sin_addr.s_addr),
						clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->num_nodes_i,
						sizeof(struct node_data), compareNodes);
				if (searchReturn_ptr == NULL)
				{
					addNode2List(clusterInfo_ptr, ntohl(response_addr.sin_addr.s_addr),
							boardtype_u8);
					qsort(clusterInfo_ptr->node_data_list_ptr,
							clusterInfo_ptr->num_nodes_i, sizeof(struct node_data),
							compareNodes);
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

int mstr_ctrl(int mast_broad_sock)
{

	printf("I am master\n");
	char keep_alive_c = 'k', identify_node_c = 'i';

	int master_i = 1, identify_counter_i = 1, return_recv_i, timeout_i = TIMEOUT;

	struct cluster_info clusterInfo_str;
	clusterInfo_str.node_data_list_ptr = (struct node_data*) malloc(
	EST_NUM_BOARD * sizeof(struct node_data));
	clusterInfo_str.alive_count_u8 = 1;
	uint8_t boardtype_u8;
	struct sockaddr_in broad_addr, recv_addr, response_addr;
	socklen_t broad_len = sizeof broad_addr, recv_len = sizeof recv_addr,
			response_len;

	if (clusterInfo_str.node_data_list_ptr == NULL)
	{
		free(clusterInfo_str.node_data_list_ptr);
		printf("couldn't malloc node_data_list");
		exit(5);
	}
	clusterInfo_str.num_nodes_i = 0;
	clusterInfo_str.size_i = EST_NUM_BOARD;

	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrAny(&recv_addr, UDP_N2M_PORT);

	if ((bind(mast_broad_sock, (struct sockaddr*) &recv_addr, recv_len)) < 0)
	{
		critErr("master:bind mast_broad_sock:");
	}

	sendto(mast_broad_sock, &identify_node_c, 1, 0,
			(struct sockaddr*) &broad_addr, broad_len);
	readIdentifyAnswers(mast_broad_sock, &clusterInfo_str, 1);
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
			sendto(mast_broad_sock, &identify_node_c, 1, 0,
					(struct sockaddr*) &broad_addr, broad_len);
			updateClusterInfo(&clusterInfo_str, mast_broad_sock, response_addr);
		}
		else
		{

			sendto(mast_broad_sock, &keep_alive_c, 1, 0,
					(struct sockaddr*) &broad_addr, broad_len);
		}
		sleep(PING_PERIOD);
		identify_counter_i = (identify_counter_i + 1) % PINGS_PER_IDENTIFY;
	} while (master_i);
	free(clusterInfo_str.node_data_list_ptr);
	return 0;
}

void updateClusterInfo(struct cluster_info *clusterInfo_ptr, int receive_sock,
		struct sockaddr_in response_addr)
{
	int i;
//	int timeout_i = TIMEOUT, returnRecv_i;
//
//	struct node_data *searchReturn_ptr;
//	uint8_t boardtype_u8;
//	socklen_t response_len;
	clusterInfo_ptr->alive_count_u8++;

readIdentifyAnswers()
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

		}
	}

}
