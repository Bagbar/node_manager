/*
 * masteunr.c
 *
 *  Created on: Jun 23, 2015
 *      Author: christian
 */

#include "master.h"

#define ACTIVE_COUNTER_START 1
void master_control(int mast_broad_sock)
{
	while (1)
	{
		printf("I am master\n");
		sleep(1);
	}

}

void add_node2list(struct cluster_info *cluster_info_ptr, uint32_t ip_u32,
		uint8_t boardtype_u8)
{
	int size_i;
	void *new_list_ptr;
	if (cluster_info_ptr->num_nodes_i >= cluster_info_ptr->size_i)
	{
		size_i = cluster_info_ptr->size_i + REALLOC_STEPSIZE;
		new_list_ptr = realloc(cluster_info_ptr->node_data_list_ptr,
				size_i * sizeof(struct node_data));
		if (new_list_ptr != NULL)
		{
			cluster_info_ptr->node_data_list_ptr = new_list_ptr;
			cluster_info_ptr->size_i = size_i;
		}
		else
		{
			free(cluster_info_ptr->node_data_list_ptr);
			printf("couldn't realloc node_data_list");
			exit(5);
		}
	}
	cluster_info_ptr->node_data_list_ptr[cluster_info_ptr->num_nodes_i].ip_u32 =
			ip_u32;
	cluster_info_ptr->node_data_list_ptr[cluster_info_ptr->num_nodes_i].type_u8 =
			boardtype_u8;
	cluster_info_ptr->node_data_list_ptr->last_alive_u8 =
			cluster_info_ptr->alive_count_u8;
	printf("ip=%d \t type = %d added\n",
			cluster_info_ptr->node_data_list_ptr[cluster_info_ptr->num_nodes_i].ip_u32,
			boardtype_u8);
	cluster_info_ptr->num_nodes_i++;
}

int mstr_ctrl(int mast_broad_sock)
{

	printf("I am master\n");
	char keep_alive_c = 'k', identify_node_c = 'i';

	int master_i = 1, identify_counter_i = 1, return_recv_i, timeout_i = TIMEOUT;

	struct cluster_info cluster_info_str;
	cluster_info_str.node_data_list_ptr = (struct node_data*) malloc(
	EST_NUM_BOARD * sizeof(struct node_data));
	cluster_info_str.alive_count_u8 = ACTIVE_COUNTER_START;
	uint8_t boardtype_u8;
	struct sockaddr_in broad_addr, recv_addr, response_addr;
	socklen_t broad_len = sizeof broad_addr, recv_len = sizeof recv_addr,
			response_len;

	if (cluster_info_str.node_data_list_ptr == NULL)
	{
		free(cluster_info_str.node_data_list_ptr);
		printf("couldn't malloc node_data_list");
		exit(5);
	}
	cluster_info_str.num_nodes_i = 0;
	cluster_info_str.size_i = EST_NUM_BOARD;

	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrAny(&recv_addr, UDP_N2M_PORT);

	if ((bind(mast_broad_sock, (struct sockaddr*) &recv_addr, recv_len)) < 0)
	{
		critErr("master:bind mast_broad_sock:");
	}

	sendto(mast_broad_sock, &identify_node_c, 1, 0,
			(struct sockaddr*) &broad_addr, broad_len);
	while (timeout_i > 0)
	{
		// Receive msg from other boards
		return_recv_i = recvfrom(mast_broad_sock, &boardtype_u8, 1, 0,
				(struct sockaddr*) &response_addr, &response_len);
		if (return_recv_i != 1)
		{
			if (return_recv_i == -1)
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
			add_node2list(&cluster_info_str, ntohl(response_addr.sin_addr.s_addr),
					boardtype_u8);
		}

	}

	qsort(cluster_info_str.node_data_list_ptr, cluster_info_str.num_nodes_i,
			sizeof(struct node_data), compareNodes);

	do
	{
		if (identify_counter_i == 0)
		{
			sendto(mast_broad_sock, &identify_node_c, 1, 0,
					(struct sockaddr*) &broad_addr, broad_len);
			update_cluster_info(&cluster_info_str, mast_broad_sock, response_addr);
		}
		else
		{

			sendto(mast_broad_sock, &keep_alive_c, 1, 0,
					(struct sockaddr*) &broad_addr, broad_len);
		}
		sleep(PING_PERIOD);
		identify_counter_i = (identify_counter_i + 1) % PINGS_PER_IDENTIFY;
	} while (master_i);
	free(cluster_info_str.node_data_list_ptr);
	return 0;
}

void update_cluster_info(struct cluster_info *cluster_info_ptr,
		int receive_sock, struct sockaddr_in response_addr)
{
	int timeout_i = TIMEOUT, return_recv_i, i;

	struct node_data *search_return;
	uint8_t boardtype_u8;
	socklen_t response_len;
	cluster_info_ptr->alive_count_u8++;
	while (timeout_i > 0)
	{
		// Receive msg from other boards
		return_recv_i = recvfrom(receive_sock, &boardtype_u8, 1, 0,
				(struct sockaddr*) &response_addr, &response_len);
		if (return_recv_i != 1)
		{
			if (return_recv_i == -1)
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
			search_return = (struct node_data*) bsearch(
					&(response_addr.sin_addr.s_addr),
					cluster_info_ptr->node_data_list_ptr, cluster_info_ptr->num_nodes_i,
					sizeof(struct node_data), compareNodes);
			if (search_return == NULL)
			{
				add_node2list(cluster_info_ptr, ntohl(response_addr.sin_addr.s_addr),
						boardtype_u8);
				qsort(cluster_info_ptr->node_data_list_ptr,
						cluster_info_ptr->num_nodes_i, sizeof(struct node_data),
						compareNodes);
			}
			else
			{
				search_return->last_alive_u8 = cluster_info_ptr->alive_count_u8;
			}

		}

	}

	for (i = 0; i < cluster_info_ptr->num_nodes_i; i++)
	{
		if (cluster_info_ptr->node_data_list_ptr[i].last_alive_u8
				!= cluster_info_ptr->alive_count_u8)
		{

		}
	}

}
