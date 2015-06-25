/*
 * masteunr.c
 *
 *  Created on: Jun 23, 2015
 *      Author: christian
 */

#include "master.h"

void master_control(int mast_broad_sock)
{
	while(1)
	{
		printf("I am master");
		sleep(1);
	}
	/*uint32_t *node_list = (unsigned char *)malloc(2*4*EST_NUM_BOARD);
	int node_list_size = EST_NUM_BOARD;
	int num_nodes = 0;
	unsigned char board_type;
	struct sockaddr_in broad_addr, response_addr;
	socklen_t broad_len = sizeof(broad_addr), response_len;
	fillSockaddrBroad(broad_addr, UDP_NODE_LISTEN_PORT);

	if ((bind(mast_broad_sock, (struct sockaddr*) &broad_addr, broad_len)) < 0)
	{
		critErr("master:bind mast_broad_sock:");
	}

	char identify_node = 's';
	sendto(mast_broad_sock, &identify_node, 1, 0,
			(struct sockaddr*) &broad_addr, broad_len);


	int return_recv,  timeout = 3;
		while ( timeout > 0)
		{
			// Receive msg from other boards
			return_recv = recvfrom(mast_broad_sock, &board_type,
					1, 0, (struct sockaddr*) &response_addr, &response_len);
			if (return_recv != 1)
			{
				if (return_recv == -1)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same leaving both if there should be a problem
					{
						timeout--;
						if (timeout > 0)
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

		}*/
}



