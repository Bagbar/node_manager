/*
 * slave.c
 *
 */

#include "slave.h"

#define BUFFERSIZE 1024
#define IDENTIFIER_LENGTH 8 // 1 for type + 6 for MAC + 1 for subgroup

extern uint8_t mac[6];
extern uint32_t ownIP;
//receives commands from master for managing the cluster and starts the threads for the actual work
void *slave_main(void *args_ptr)
{
	//Variable declarations
	struct var_mtx *timeoutCounter_ptr = ((struct slave_args*) args_ptr)->timeoutCount_ptr;
	int *master_ptr = ((struct slave_args*) args_ptr)->master_ptr;
	uint8_t boardtype_u8 = FPGATYPE;
	uint8_t *subgroup_ptr = ((struct slave_args*) args_ptr)->subgroup_ptr;
	uint8_t typeAndGroup[2]; //Group ([1]) not implemented at the moment
	int recvReturn_i, sendreturn_i;
	char recvBuff[5];
	memset(recvBuff, '0', sizeof recvBuff);

	pthread_t waitForData_thread;
	if (pthread_create(&waitForData_thread, NULL, fetchDataAndExecute,
			(void*) ((struct slave_args*) args_ptr)->workReady_ptr))
	{
		critErr("slave:pthread_create(recv_info)=");
	}

	int recvMast_sock, electRecv_sock;

	struct sockaddr_in serv_addr, elect_recv_addr, cli_addr;
	socklen_t cli_len = sizeof cli_addr;
	//cli_addr.sin_family =AF_INET;
	//create UDP-Socket receiver for control commands
	if ((recvMast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("slave_main:socket=");
	}

	//listen socket address and bind for master control messages
	fillSockaddrAny(&serv_addr, UDP_NODE_LISTEN_PORT);

	if ((bind(recvMast_sock, (struct sockaddr*) &serv_addr, sizeof serv_addr)) < 0)
	{
		critErr("slave_main:bind recv_mast_sock:");
	}
	//create UDP-Socket receiver for fetching the type_and_MAC broadcasts
	if ((electRecv_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("slave_main:elect_recv_sock=");
	}

	//listen socket address and bind for electing master
	fillSockaddrAny(&elect_recv_addr, UDP_ELECT_M_PORT);

	if ((bind(electRecv_sock, (struct sockaddr*) &elect_recv_addr, sizeof(elect_recv_addr))) < 0)
	{
		critErr("slave_main:bind elect_recv_sock:");
	}
	//non-blocking recv
	fcntl(electRecv_sock, F_SETFL, O_NONBLOCK);

	// waiting for the master and reseting the timeout counter and if needed answering the request
	// should be an endless loop
	while (1)
	{

		//printf("slave_main: wait for message:\n");
		// Receive msg from master,

		recvReturn_i = recvfrom(recvMast_sock, &recvBuff[0], (size_t) 1, 0,
				(struct sockaddr*) &cli_addr, &cli_len);

		if (recvReturn_i < 0)
			perror("slave_main: recv control msg error");

		//printf("slave_main: received : %c \t from= %s \n", recvBuff[0], hostToDottedIP(ntohl(cli_addr.sin_addr.s_addr)));

		switch (recvBuff[0])
		{
		case 'i': ///identify self : send your Type to caller
			typeAndGroup[0] = boardtype_u8;
			typeAndGroup[1] = *subgroup_ptr;

			cli_addr.sin_port = htons(UDP_N2M_PORT);
			//printf("slave:\t\tcli_addr.sinfamily = %d\n",cli_addr.sin_family);
			sendreturn_i = sendto(recvMast_sock, &typeAndGroup[0], (size_t) 2, 0,
					(struct sockaddr*) &cli_addr, cli_len);
			//printf("slave_main:send identify:%d\n", sendreturn);
			if (sendreturn_i < 0)
				perror("slave_main:send identify failed");
			//no break
		case 'k': ///keepalive
			if (pthread_mutex_lock(&(timeoutCounter_ptr->mtx)))
				critErr("slave_main: mutex_lock:");
			////printf("slave_main: MUTEX LOCKED\n");
			timeoutCounter_ptr->var = 0;
			if (pthread_mutex_unlock(&(timeoutCounter_ptr->mtx)))
				critErr("slave_main: mutex_unlock:");
			////printf("slave_main: MUTEX UNLOCKED\n");
			break;
		case 't': //timeout encountered by a node
			//printf("slave_main:timeout signal received\n");
			if (pthread_mutex_lock(&(timeoutCounter_ptr->mtx)))
				critErr("slave_main: mutex_lock:");
			////printf("slave_main: MUTEX LOCKED\n");
			if (timeoutCounter_ptr->var > 1)
			{
				timeoutCounter_ptr->var = -5;
				*master_ptr = elect_master(electRecv_sock);
			}
			//printf("slave_main:master= %d\n", *master_ptr);
			if (pthread_mutex_unlock(&(timeoutCounter_ptr->mtx)))
				critErr("slave_main: mutex_unlock:");
			////printf("slave_main: MUTEX UNLOCKED\n");
			break;
		default:
			//printf("slave_main: Unknown symbol: recvBuff[0]=%c\n", recvBuff[0]);
			close(recvMast_sock);
			exit(2);
		}

	}

	return NULL;
}

int elect_master(int electRecv_sock)
{
	printf("slave:electing master:\n");
	uint8_t typeMAC_self[IDENTIFIER_LENGTH];
	typeMAC_self[0] = FPGATYPE;
	typeMAC_self[1] = CLUSTERGROUP;
	int i;
	for (i = 0; i < 6; i++)
	{
		typeMAC_self[i + 2] = mac[i];
	}

	uint8_t typeMAC_other[IDENTIFIER_LENGTH];

	int electSend_sock;
	struct sockaddr_in elect_addr;
	socklen_t electSend_len = sizeof(elect_addr);
//create UDP-Socket Server
	if ((electSend_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("slave_main:send_master_socket=");
	}
	//Broadcast socket address
	fillSockaddrBroad(&elect_addr, UDP_ELECT_M_PORT);
	int broadcastEnable = 1;
	int ret = setsockopt(electSend_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
			sizeof(broadcastEnable));
	if (ret)
		critErr("slave: couldn't set setsockopt:");

//	if ((bind(elect_send_sock, (struct sockaddr*) &elect_addr,
//			sizeof(elect_addr))) < 0)
//	{
//		critErr("main:bind elect_sock:");
//	}

	sendto(electSend_sock, &typeMAC_self[0], sizeof(typeMAC_self), 0, (struct sockaddr*) &elect_addr,
			electSend_len);
	printf("elect: broadcast sent: type and mac\n");
	//close(elect_send_sock);

	int recvReturn_i, best_i = 1, timeout_i = 3;
	while (best_i == 1 && timeout_i > 0)
	{
		// Receive msg from other boards
		recvReturn_i = recvfrom(electRecv_sock, &typeMAC_other[0], sizeof(typeMAC_other), 0, NULL,
		NULL);

		printf("returnrecv=%d\n", recvReturn_i);
		if (recvReturn_i != sizeof(typeMAC_self))
		{
			if (recvReturn_i == -1)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same leaving both if there should be a problem
				{
					timeout_i--;
					printf("nonblock\n");
					if (timeout_i > 0)
						sleep(1);
				}
				else
					critErr("slave:elect:recvfrom:");
			}
			else
			{
				printf("typeMAC_other received has the wrong size");
				exit(3); //should be exclusive for this
			}
		}
		else
		{
			i = 0;
			while (best_i == 1 && i < IDENTIFIER_LENGTH)
			{
				if (typeMAC_other[i] < typeMAC_self[i])
				{
					best_i = 0;
					printf("I'm not the best;i=%d\t typeMAC_other[i]=%d \ttypeMAC_self[i]=%d\n", i,
							typeMAC_other[i], typeMAC_self[i]);
				}
				else
					printf("I'm the best;i=%d\t typeMAC_other[i]=%d \ttypeMAC_self[i]=%d\n", i,
							typeMAC_other[i], typeMAC_self[i]);

				if (typeMAC_other[i] > typeMAC_self[i])
					i = IDENTIFIER_LENGTH;
				i++;
			}
		}
	}
	//flush the socket
	while (recvReturn_i >= 0)
	{
		recvReturn_i = recvfrom(electRecv_sock, &typeMAC_other[0], sizeof(typeMAC_self), 0, NULL, NULL);
		printf("flush\n");
	}

	return best_i;
}

//fetches target IPaddress for processed data, size of work program
void *receive_info(void * args)
{
	printf("slave:receive info: started\n");
	struct recv_info *recvInfo_ptr = args;
	struct fileInfo_bufferformat *recvBuff = recvInfo_ptr->recvBuff;
	uint8_t check_u8;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof client_addr;

	int return_socket = recvInfo_ptr->socket;

	listen(return_socket, 1);

	while (1)
	{
		printf("slave:receiveInfo: accept waiting\n");
		int return_accept = accept(return_socket, (struct sockaddr*) &client_addr, &client_len);
		if (return_accept < 0)
		{
			printf("slave: recv_info: accepterror:%s\n", strerror(errno));
			check_u8 = CHECK_FAILED;
		}
		printf("slave:recvInfo: accepted sizeof recvBuff = %u\n", sizeof(*recvBuff));
		recv(return_accept, recvBuff, sizeof(*recvBuff), 0);
		printf("slave:recvInfo: received");
		if (recvBuff->cancel == 1)
		{
			// TODO insert cancel function
			//pthread_cancel(work_thread);
			check_u8 = WORK_THREAD_CANCELED;
		}
		else
		{
			check_u8 = CHECK_OKAY;
		}
		printf("slave:recvInfo: send check");
		send(return_accept, &check_u8, 1, 0);
		if (check_u8 == CHECK_OKAY)
		{
		}
		printf(
				"slave:receive info: finished\t cancel =%d \tsize = %llu\tworkname= %s\t scriptname= %s \n",
				recvBuff->cancel, recvBuff->file_size, recvBuff->workname, recvBuff->scriptname);
		close(return_accept);
		return NULL;
	}
	close(return_socket);
	return NULL;
}

//fetches the files
void *receive_file(void * args)
{
	struct recv_file * file_info_ptr = args;

	uint8_t check_u8;
	size_t file_size = 0;
	FILE * pFile;
	int recv_return_i, errorcount_i = 0;
	char recvBuff[BUFFERSIZE];
	memset(recvBuff, '0', sizeof(recvBuff));

	int recv_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (recv_sock < 0)
		printf("socketerror:%s\n", strerror(errno));

	struct sockaddr_in addr, client;
	socklen_t len;
	fillSockaddrAny(&addr, TCP_RECV_ARCHIVE_PORT);

	int return_bind = bind(recv_sock, (struct sockaddr*) &addr, sizeof(addr));
	if (return_bind < 0)
		perror("slave:receive file:binderror:");

	listen(recv_sock, 1);
	do
	{
		printf("slave:receive file: accept\n");
		int return_accept = accept(recv_sock, (struct sockaddr*) &client, &len);
		if (return_accept < 0)
			printf("accepterror:%s\n", strerror(errno));

		pFile = fopen(NODE_ARCHIVE_NAME, "wb");
		if (pFile == NULL)
		{
			fputs("File error", stderr);
			exit(1);
		}

		do
		{
			recv_return_i = recv(return_accept, recvBuff, BUFFERSIZE, 0);
			if (recv_return_i < 0)
				printf("recverror:%s\n", strerror(errno));
			else
				printf("recv data = %d\n", recv_return_i);

			fwrite(recvBuff, 1, recv_return_i, pFile);
			file_size += recv_return_i;
		} while (recv_return_i > 0); // == BUFFERSIZE);

		fclose(pFile);
		file_info_ptr->recv_size = file_size;
		printf("expected size= %u\t received size =%u\n", file_info_ptr->expected_size, file_size);
		if (file_info_ptr->expected_size != file_size)
		{
			errorcount_i++;
			check_u8 = CHECK_FAILED;
		}
		else
		{
			check_u8 = CHECK_OKAY;
		}
		return_accept = accept(recv_sock, (struct sockaddr*) &client, &len);
		send(return_accept, &check_u8, 1, 0);
		printf("slave_receive file : check sent and closing socket\n");
		close(return_accept);
	} while (check_u8 == CHECK_FAILED && errorcount_i < 3);
	if (errorcount_i >= 3)
	{
		printf("slave:receive_file: couldn't receive archive");
		exit(6);
	}
	printf("slave:receive file: finished\n");
	return NULL;
}

void *fetchDataAndExecute(void *args)
{
	struct cond_mtx *workReady_ptr = args;
	struct recv_info recvInfo_sct;
	struct fileInfo_bufferformat recvInfoBuff;
	recvInfo_sct.recvBuff = &recvInfoBuff;
	char xmlName[20];
	struct recv_file recvFile_sct =
	{ 0, 0 };
	char command[9 + FILENAME_SIZE];
	struct IP_list *IP_list_ptr;
	char * workcall;

	int return_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (return_socket < 0)
		printf("slave: recv_info: socketerror:%s\n", strerror(errno));

	struct sockaddr_in addr;
	fillSockaddrAny(&addr, TCP_RECV_INFO_PORT);

	int return_bind = bind(return_socket, (struct sockaddr*) &addr, sizeof(addr));
	if (return_bind < 0)
		printf("slave: fetch data and execute:: binderror:%s\n", strerror(errno));

	recvInfo_sct.socket = return_socket;

	while (1)
	{
		receive_info(&recvInfo_sct);
		recvFile_sct.expected_size = recvInfoBuff.file_size;
		receive_file(&recvFile_sct);
		printf("slave:fetch data and execute: file fetched\n");

		if (snprintf(command, sizeof command, "tar xzf %s", NODE_ARCHIVE_NAME) > sizeof command)
			critErr("slave fetch data and execute: snprint error tar xzf");
		system(command);

		if (snprintf(xmlName, sizeof xmlName, "IP_%u.xml", ownIP) > sizeof command)
			critErr("slave fetch data and execute: snprint error xmlName");
		printf("generated name of the xml file is %s\n", xmlName);
		xmlDocPtr doc = xmlParseFile(xmlName);
		IP_list_ptr = getIPfromXML(doc);
		xmlFree(doc);
		if (snprintf(command, sizeof command, "sh %s", recvInfoBuff.scriptname) > sizeof command)
			critErr("slave fetch data and execute: snprint error shell");
		puts(command);
		system(command);
		int mall_i = IP_list_ptr->amount * 13 + 3 + sizeof(recvInfoBuff.workname);
		workcall = malloc(mall_i);
		printf("slave:fetch Data And Execute: allocated = %d", mall_i);

		if (workcall == NULL)
		{
			printf("slave:fetch_data: char array workcall malloc error");
			return NULL;
		}

		if (snprintf(workcall, mall_i, "./%s", recvInfoBuff.workname) > mall_i)
			critErr("slave fetch data and execute: snprint workcall 1");
		int i = 0;
		while (workcall[i])
			i++;
		printf("slave: fetch data and execute: workcall=%s string length = %d", workcall, i);

		char singleIP_c[13];
		for (int i = 0; i < IP_list_ptr->amount; i++)
		{
			if (snprintf(singleIP_c, sizeof singleIP_c, " %u", IP_list_ptr->IP[i]) > sizeof singleIP_c)
				critErr("slave fetch data and execute: snprint single IP");
			strcat(workcall, singleIP_c);
		}
		i = 0;
		while (workcall[i])
			i++;
		printf("slave: fetch data and execute: workcall string length = %d", i);

		if (pthread_mutex_lock(&workReady_ptr->mtx))
			perror("slave:fetch data and execute: work mutex lock\n");
		printf("slave: fetch Data: condition mutex locked\n");
		printf("\nstarting work :  %s\n", workcall);

		printf("\nfinished work : %d\n", system(workcall));
		if (pthread_cond_signal(&workReady_ptr->cond))
			perror("slave:fetch data and execute: condition signal");
		printf("slave: fetch Data: condition signal\n");
		if (pthread_mutex_unlock(&workReady_ptr->mtx))
			perror("slave:fetch data and execute: work mutex unlock");
		printf("slave: fetch Data: condition mutex unlocked\n");
		free(IP_list_ptr);
		free(workcall);
	}
	return NULL;

}

struct IP_list *getIPfromXML(xmlDocPtr doc)
{
	struct IP_list *IPlist_ptr = malloc(sizeof *IPlist_ptr);
	if (IPlist_ptr == NULL)
		return NULL;
	IPlist_ptr->IP = NULL;
	IPlist_ptr->amount = 0;
	xmlNodePtr node = xmlDocGetRootElement(doc);
	xmlNodePtr child = node->children;
	char *IP_str;
	int i = 0;
	while (child)
	{
		if (!xmlStrcmp(child->name, (xmlChar*) "dest_IP"))
		{
			IPlist_ptr->amount++;
		}
		child = child->next;
	}
	child = node->children;
	IPlist_ptr->IP = malloc(sizeof(*(IPlist_ptr->IP)) * IPlist_ptr->amount);
	if (IPlist_ptr->IP == NULL)
		return NULL;
	while (child)
	{
		if (!xmlStrcmp(child->name, (xmlChar*) "dest_IP"))
		{

			IP_str = (char*) xmlNodeGetContent(child);
			IPlist_ptr->IP[i] = (uint32_t) strtoul((char*) IP_str, NULL, 10);
			printf("IP_i = %u\tIP_str=%s\n", IPlist_ptr->IP[i], IP_str);
			xmlFree(IP_str);
			i++;
		}
		child = child->next;
	}
	printf("slave:get IP from XML: number of IP addresses = %d\n", i);
	return IPlist_ptr;
}

