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

void mstr_ctrl(int mast_broad_sock)
{
	printf("I am master\n");
	void *new_list_ptr;
	int size_i;
	struct node_info node_info_str;
	node_info_str.node_data_list_ptr = (uint32_t*) malloc(
			EST_NUM_BOARD * sizeof (struct node_data));

	if (node_info_str.node_data_list_ptr == NULL )
	{
		free(node_info_str.node_data_list_ptr);
		printf("couldn't malloc node_data_list");
		exit(5);
	}
	node_info_str.num_nodes_i = 0;
	node_info_str.size_i = EST_NUM_BOARD;

	uint8_t board_type_uc;
	struct sockaddr_in broad_addr, recv_addr, response_addr;
	socklen_t broad_len = sizeof(broad_addr), recv_len, response_len;
	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrAny(&recv_addr,UDP_N2M_PORT);

	if ((bind(mast_broad_sock, (struct sockaddr*) &recv_addr, broad_len)) < 0)
	{
		critErr("master:bind mast_broad_sock:");
	}

	char identify_node_c = 's';
	sendto(mast_broad_sock, &identify_node_c, 1, 0,
			(struct sockaddr*) &broad_addr, broad_len);

	int return_recv_i, timeout_i = 3;
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
			if (node_info_str.num_nodes_i >= node_info_str.size_i)
			{	size_i = node_info_str.size_i + REALLOC_STEPSIZE;
			new_list_ptr = realloc(node_info_str.node_data_list_ptr,
					size_i * sizeof *node_info_str.node_data_list_ptr);
			if (new_list_ptr != NULL)
			{
				node_info_str.node_data_list_ptr = new_list_ptr;
				node_info_str.size_i = size_i;
			}
			else
			{
				free(new_list_ptr);
				free(node_info_str.node_data_list_ptr);
				printf("couldn't realloc node_data_list");
				exit(5);
			}
			}
			node_info_str.node_data_list_ptr[node_info_str.num_nodes_i].ip_u32=ntohl(response_addr.sin_addr.s_addr);
			node_info_str.node_data_list_ptr[node_info_str.num_nodes_i].type_u8=board_type_uc;
			printf("ip=%d \t type = %d\n",node_info_str.node_data_list_ptr[node_info_str.num_nodes_i].ip_u32,board_type_uc);
			node_info_str.num_nodes_i++;


		}

	}
	free(node_info_str.node_data_list_ptr);

}


