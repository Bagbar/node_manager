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
		printf("I am master");
		sleep(1);
	}

}

void mstr_ctrl(int mast_broad_sock)
{
	void* new_ip_ptr, new_type_ptr;
	int size_i;
	struct node_info node_info_str;
	node_info_str.ip_ptr = (uint32_t*) malloc(
			EST_NUM_BOARD * sizeof *node_info_str.ip_ptr);
	node_info_str.type_ptr = (uint8_t*) malloc(
			EST_NUM_BOARD * sizeof *node_info_str.type_ptr);
	if (node_info_str.ip_ptr == NULL || node_info_str.type_ptr == NULL)
	{
		free(node_info_str.ip_ptr);
		free(node_info_str.type_ptr);
		printf("couldn't malloc ip or type list")
		exit(5);
	}
	node_info_str.num_nodes_i = 0;
	node_info_str.size_i = EST_NUM_BOARD;

	uint8_t board_type_uc;
	struct sockaddr_in broad_addr, response_addr;
	socklen_t broad_len = sizeof(broad_addr), response_len;
	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);

	if ((bind(mast_broad_sock, (struct sockaddr*) &broad_addr, broad_len)) < 0)
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
				size_i = node_info_str.size_i + REALLOC_STEPSIZE;
			new_ip_ptr = realloc(node_info_str.ip_ptr,
					size_i * sizeof *node_info_str.ip_ptr);
			new_type_ptr = realloc(node_info_str.type_ptr,
					size_i * sizeof *node_info_str.type_ptr);
			if (new_ip_ptr != NULL && new_type_ptr != NULL)
			{
				node_info_str.ip_ptr = new_ip_ptr;
				node_info_str.type_ptr = new_type_ptr;
				node_info_str.size_i = size_i;
			}
			else
			{
				free(new_ip_ptr);
				free(new_type_ptr);
				free(node_info_str.ip_ptr);
				free(node_info_str.type_ptr);
				printf("couldn't realloc ip or type list");
				exit(5);
			}
			node_info_str.ip_ptr[node_info_str.num_nodes_i]=ntohl(response_addr.sin_addr);
			node_info_str.type_ptr[node_info_str.num_nodes_i]=board_type_uc;
			node_info_str.num_nodes_i++;


		}

	}
	free(node_info_str.ip_ptr);
	free(node_info_str.type_ptr);
}

