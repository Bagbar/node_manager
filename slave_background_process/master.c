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

int mstr_ctrl(int mast_broad_sock)
{

	printf("I am master\n");
	char keep_alive_c = 'k', identify_node_c = 'i';

	int master_i = 1, identify_counter_i = 1, return_recv_i, timeout_i = TIMEOUT,
			size_i;
	void *new_list_ptr;

	struct cluster_info cluster_info_str;
	uint8_t board_type_uc;
	struct sockaddr_in broad_addr, recv_addr, response_addr;
	socklen_t broad_len = sizeof(broad_addr), recv_len, response_len;
	cluster_info_str.node_data_list_ptr = (struct node_data*) malloc(
	EST_NUM_BOARD * sizeof(struct node_data));

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

	if ((bind(mast_broad_sock, (struct sockaddr*) &recv_addr, broad_len)) < 0)
	{
		critErr("master:bind mast_broad_sock:");
	}

	sendto(mast_broad_sock, &identify_node_c, 1, 0,
			(struct sockaddr*) &broad_addr, broad_len);

	while (timeout_i > 0)
	{
		// Receive msg from other boards
		return_recv_i = recvfrom(mast_broad_sock, &board_type_uc, 1, 0,
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
			if (cluster_info_str.num_nodes_i >= cluster_info_str.size_i)
			{
				size_i = cluster_info_str.size_i + REALLOC_STEPSIZE;
				new_list_ptr = realloc(cluster_info_str.node_data_list_ptr,
						size_i * sizeof *cluster_info_str.node_data_list_ptr);
				if (new_list_ptr != NULL)
				{
					cluster_info_str.node_data_list_ptr = new_list_ptr;
					cluster_info_str.size_i = size_i;
				}
				else
				{
					free(new_list_ptr);
					free(cluster_info_str.node_data_list_ptr);
					printf("couldn't realloc node_data_list");
					exit(5);
				}
			}
			cluster_info_str.node_data_list_ptr[cluster_info_str.num_nodes_i].ip_u32 =
					ntohl(response_addr.sin_addr.s_addr);
			cluster_info_str.node_data_list_ptr[cluster_info_str.num_nodes_i].type_u8 =
					board_type_uc;
			printf("ip=%d \t type = %d\n",
					cluster_info_str.node_data_list_ptr[cluster_info_str.num_nodes_i].ip_u32,
					board_type_uc);
			cluster_info_str.num_nodes_i++;

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
			update_cluster_info();
		}
		else
		{

			sendto(mast_broad_sock, &identify_node_c, 1, 0,
					(struct sockaddr*) &broad_addr, broad_len);
		}
		sleep(PING_PERIOD);
		identify_counter_i = (identify_counter_i + 1) % PINGS_PER_IDENTIFY;
	} while (master_i);
	free(cluster_info_str.node_data_list_ptr);
	return 0;
}

void update_cluster_info(struct cluster_info *cluster_info_str, int receive_sock, struct sockaddr_in response_addr)
{
	//TODO (1) anpassen
	int timeout_i=TIMEOUT, return_recv_i,size_i;
	void *new_list_ptr;
	uint8_t board_type_uc;
	socklen_t response_len;
	while (timeout_i > 0)
		{
			// Receive msg from other boards
			return_recv_i = recvfrom(receive_sock, &board_type_uc, 1, 0,
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
				if (cluster_info_str.num_nodes_i >= cluster_info_str.size_i)
				{
					size_i = cluster_info_str.size_i + REALLOC_STEPSIZE;
					new_list_ptr = realloc(cluster_info_str.node_data_list_ptr,
							size_i * sizeof *cluster_info_str.node_data_list_ptr);
					if (new_list_ptr != NULL)
					{
						cluster_info_str.node_data_list_ptr = new_list_ptr;
						cluster_info_str.size_i = size_i;
					}
					else
					{
						free(new_list_ptr);
						free(cluster_info_str.node_data_list_ptr);
						printf("couldn't realloc node_data_list");
						exit(5);
					}
				}
				cluster_info_str.node_data_list_ptr[cluster_info_str.num_nodes_i].ip_u32 =
						ntohl(response_addr.sin_addr.s_addr);
				cluster_info_str.node_data_list_ptr[cluster_info_str.num_nodes_i].type_u8 =
						board_type_uc;
				printf("ip=%d \t type = %d\n",
						cluster_info_str.node_data_list_ptr[cluster_info_str.num_nodes_i].ip_u32,
						board_type_uc);
				cluster_info_str.num_nodes_i++;

			}

		}

		qsort(cluster_info_str.node_data_list_ptr, cluster_info_str.num_nodes_i,
				sizeof(struct node_data), compareNodes);
}
