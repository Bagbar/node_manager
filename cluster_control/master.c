/*
 * master.c
 *
 *  Created on: Jun 23, 2015
 *      Author: christian
 */

#include "master.h"

extern uint32_t ownIP;

char errormessage[100];

#define BUFFERSIZE 1024

void mypause ( void )
{
  printf ( "Press [Enter] to continue . . ." );
  fflush ( stdout );
  getchar();
}


int master_main(int mastBroad_sock)
{

	printf("master_main started\n");
	char keep_alive_c = KEEP_ALIVE_SIGNAL, identify_node_c = IDENTIFY_SIGNAL;

	pthread_t listenForData_thread;
	int master_i = 1, identify_counter_i = 1;

//	int dummy_sock;
//	if ((dummy_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
//		{
//			critErr("master:dummy_socket=");
//		}
//	struct sockaddr_in any_addr;
//	fillSockaddrAny(&any_addr, UDP_N2M_PORT);
//
//		if ((bind(dummy_sock, (struct sockaddr*) &any_addr, sizeof any_addr)) < 0)
//		{
//			critErr("master:bind dummy_sock:");
//		}
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
	clusterInfo_sct.numNodes_size = 0;
	clusterInfo_sct.size = EST_NUM_BOARD;

	struct get_Program getProgram_sct;
	getProgram_sct.exitSignal = 0;
	getProgram_sct.clusterInfo_ptr = &clusterInfo_sct;

	if (pthread_create(&listenForData_thread, NULL, getProgram, &getProgram_sct))
	{
		critErr("pthread_create(getProgram)=");
	}

	struct sockaddr_in broad_addr, loop_addr, recv_addr;
	socklen_t broad_len = sizeof broad_addr, recv_len = sizeof recv_addr, loop_len = sizeof loop_addr;

	fillSockaddrLoop(&loop_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);



	int dummy_sock;
	 if ((dummy_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	 		{
	 			critErr("master:dummy_socket=");
	 		}
	 fillSockaddrAny(&recv_addr, UDP_N2M_PORT);
	if ((bind(dummy_sock, (struct sockaddr*) &recv_addr, sizeof recv_addr)) < 0)
	 		{
	 			critErr("master:bind dummy_sock:");
	 		}
	fcntl(dummy_sock, F_SETFL, O_NONBLOCK);

	sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
			(struct sockaddr*) &broad_addr, broad_len);

	/*sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0, (struct sockaddr*) &loop_addr,
		loop_len);
*/

	readIdentifyAnswers(dummy_sock, &clusterInfo_sct, 1);

	printf("master:\t\tnodes in cluster = %d\n", clusterInfo_sct.numNodes_size);

	while (master_i)
	{
		if (identify_counter_i == 0)
		{

			sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
					(struct sockaddr*) &broad_addr, broad_len);

			sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
					(struct sockaddr*) &loop_addr, loop_len);
			updateClusterInfo(&clusterInfo_sct, dummy_sock);
			printf("master:\t\tnodes in cluster = %d\n", clusterInfo_sct.numNodes_size);
		}
		else
		{

			sendto(mastBroad_sock, &keep_alive_c, sizeof keep_alive_c, 0, (struct sockaddr*) &broad_addr,
					broad_len);
		}

		sleep(PING_PERIOD);
		identify_counter_i = (identify_counter_i + 1) % PINGS_PER_IDENTIFY;
		printf("master:identify_counter=%d\n", identify_counter_i);
	}
	free(clusterInfo_sct.node_data_list_ptr);
	return 0;
}
void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32, uint8_t *typeAndGroup)
{
	printf("master: addNode started\n");
	int size_i;
	void *newList_ptr;
	if (clusterInfo_ptr->numNodes_size >= clusterInfo_ptr->size)
	{
		size_i = clusterInfo_ptr->size + REALLOC_STEPSIZE;
		newList_ptr = realloc(clusterInfo_ptr->node_data_list_ptr, size_i * sizeof(struct node_data));
		if (newList_ptr != NULL)
		{
			clusterInfo_ptr->node_data_list_ptr = newList_ptr;
			clusterInfo_ptr->size = size_i;
		}
		else
		{
			free(clusterInfo_ptr->node_data_list_ptr);
			printf("couldn't realloc node_data_list\n");
			exit(5);
		}
	}
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->numNodes_size].ip_u32 = ip_u32;
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->numNodes_size].type_u8 = typeAndGroup[0];
	clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->numNodes_size].group_u8 = typeAndGroup[1];
	clusterInfo_ptr->node_data_list_ptr->lastAlive_u8 = clusterInfo_ptr->alive_count_u8;
	clusterInfo_ptr->node_data_list_ptr->nowActive_u8 = 0;
	printf("ip=%u \t type = %u added \t IP_dotted=%s\n",
			clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->numNodes_size].ip_u32, typeAndGroup[0],hostToDottedIP(clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->numNodes_size].ip_u32 ));
	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	clusterInfo_ptr->numNodes_size++;
	pthread_mutex_unlock(&clusterInfo_ptr->mtx);
}

void readIdentifyAnswers(int receive_sock, struct cluster_info *clusterInfo_ptr, uint8_t newList_u8)
{

	printf("master:readIdentifyAnswers started\n");
	int timeout_i = TIMEOUT, returnRecv_i;
	uint8_t typeAndGroup[2];
	uint32_t IP_holder;
	struct sockaddr_in response_addr;
	socklen_t response_len = sizeof response_addr;
	struct node_data *searchReturn_ptr;
	struct node_data compare_node;

	size_t typeAndGroup_size = sizeof typeAndGroup;
	while (timeout_i > 0)
	{
		// Receive msg from other boards
		returnRecv_i = recvfrom(receive_sock, &typeAndGroup[0], typeAndGroup_size, 0,
				(struct sockaddr*) &response_addr, &response_len);
		printf("master:readIdentifyAnswers: receive_return : %d\n", returnRecv_i);
		if (returnRecv_i != typeAndGroup_size)
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
					critErr("master:readIdentifyAnswers: recvfrom");
			}
			else
			{
				printf("master:readIdentifyanswers: recvfrom wrongsize\n");
				exit(4); //should be exclusive for this
			}
		}
		else
		{
			IP_holder = ntohl(response_addr.sin_addr.s_addr);
			if(IP_holder == 0 || IP_holder == 0x7F000001) //loopback
				IP_holder =ownIP;
			if (newList_u8)
			{
				addNode2List(clusterInfo_ptr, IP_holder, &typeAndGroup[0]);
				//printf("master:readIdentify: called addNode for new list\n");
			}
			else
			{
				compare_node.ip_u32=IP_holder;
				searchReturn_ptr = (struct node_data*) bsearch(&compare_node,
						clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size,
						sizeof(struct node_data), compareNodes);
				if (searchReturn_ptr == NULL)
				{
					printf("master:readIdentify: called addNode for existing list\n");
					addNode2List(clusterInfo_ptr, IP_holder, &typeAndGroup[0]);
					qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size,
							sizeof(struct node_data), compareNodes);
				}
				else
				{
					printf("master_readIdentify: node already exists ip=%u\t IPdotted=%s\n",searchReturn_ptr->ip_u32,hostToDottedIP(searchReturn_ptr->ip_u32));
					searchReturn_ptr->lastAlive_u8 = clusterInfo_ptr->alive_count_u8;
				}
			}
		}

	}

	if (newList_u8)
		{printf("master:qsort for new list started\n");
		qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size,
				sizeof(struct node_data), compareNodes);
		}
printf("master:readIdentify finished\n");
}

void updateClusterInfo(struct cluster_info *clusterInfo_ptr, int receive_sock)
{
	printf("master: updateClusterInfo: start\n");
	int i, outdated_i = 0;
//	int timeout_i = TIMEOUT, returnRecv_i;
//
//	struct node_data *searchReturn_ptr;
//	uint8_t boardtype_u8;
//	socklen_t response_len;
	clusterInfo_ptr->alive_count_u8++;

	readIdentifyAnswers(receive_sock, clusterInfo_ptr, 0);

	// remove non active nodes
	for (i = 0; i < clusterInfo_ptr->numNodes_size; i++)
	{
		if (clusterInfo_ptr->node_data_list_ptr[i].lastAlive_u8 != clusterInfo_ptr->alive_count_u8)
		{
			clusterInfo_ptr->node_data_list_ptr[i].ip_u32 = -1;
			clusterInfo_ptr->node_data_list_ptr->nowActive_u8 = 0;
			printf("master: update Info :node removed\n");
			outdated_i++;
		}
	}
	qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size, sizeof(struct node_data),
			compareNodes);
	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	clusterInfo_ptr->numNodes_size = clusterInfo_ptr->numNodes_size - outdated_i;
	pthread_mutex_unlock(&clusterInfo_ptr->mtx);

}

void *sendInfo(void *args)
{
	struct send_info *send_info_ptr = args;
	uint8_t check_u8;
	struct fileInfo_bufferformat sendBuff;
	sendBuff.file_size = send_info_ptr->file_size;
	memset(sendBuff.scriptname, 0, FILENAME_SIZE);
	memset(sendBuff.workname, 0, FILENAME_SIZE);
	strcpy(sendBuff.scriptname, send_info_ptr->scriptname);
	strcpy(sendBuff.workname, send_info_ptr->workname);

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(TCP_RECV_INFO_PORT);
	dest_addr.sin_addr.s_addr = send_info_ptr->IP; //has to be converted already

	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		printf("slave: recv_info: socketerror:%s\n", strerror(errno));
	int return_connect = connect(return_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
	if (return_connect < 0)
		critErr("master: send_info: connecterror\n");
	do
	{
		int return_send = send(return_socket, &sendBuff, sizeof(sendBuff), 0);
		if (return_send < 0)
			printf("master: send_info: senderror:%s\n", strerror(errno));

		int return_recv = recv(return_socket, &check_u8, 1, 0);
		if (return_recv < 0)
			printf("recverror:%s\n", strerror(errno));

		if (check_u8 != CHECK_OKAY)
			printf("master: send_info: check_failed ->retry\n");
	} while (check_u8 != CHECK_OKAY);
	close(return_socket);
	return NULL;

}
void *sendFile(void *args)
{
	struct send_file *sendFile_ptr = args;
	uint8_t check_u8;
	int i;

	char sendBuff[BUFFERSIZE];
	memset(sendBuff, '0', sizeof(sendBuff));
	int return_send;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(TCP_RECV_ARCHIVE_PORT);
	dest_addr.sin_addr.s_addr = sendFile_ptr->IP;
	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		critErr("master:send_info: socket error:");
	do
	{

		int return_connect = connect(return_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

		if (return_connect < 0)
			critErr("master:send_info:connect error:");

		FILE * pFile;

		size_t result;

		pFile = fopen(sendFile_ptr->filename, "rb");
		if (pFile == NULL)
			critErr("master:send_info: file open:");
		do
		{
			// copy the file into the buffer:
			result = fread(sendBuff, 1, BUFFERSIZE, pFile);
			if (result != BUFFERSIZE && feof(pFile) == 0)
			{
				printf("master:send_file:Reading error\n");
				exit(-1);
			}
			return_send = send(return_socket, &sendBuff[0], result, 0);
			if (return_send < 0)
				printf("master: send_file: senderror:%s\n", strerror(errno));
			else
				printf("send data = %d\n", return_send);
		} while (feof(pFile) == 0);
		// terminate
		fclose(pFile);
		printf("master:send_file:file_sent:%s\n", sendFile_ptr->filename);

		recv(return_socket, &check_u8, 1, 0);

		if (close(return_socket) < 0)
			printf("closeerror:%s\n", strerror(errno));
		if (check_u8 != CHECK_OKAY)
		{
			printf("master: send_info: %s check_failed ->retry\n", sendFile_ptr->filename);
			sleep(1);
		}
	} while (check_u8 != CHECK_OKAY);
	return NULL;
}

void* getFilesAndSend(void* args)
{
	xmlNodePtr node = args, child = NULL;

	struct send_info sendInfo_sct;
	struct send_file sendFile_sct;

	int endOfString_i = 0;
	char *filename = NULL, localname[FILENAME_SIZE], command[FILENAME_SIZE + 40], once = 1;

	child = node->children;
	while (child != NULL && xmlStrcmp(child->name, (xmlChar *) "files"))
	{
		child = child->next;
	}
	if (!xmlStrcmp(child->name, (xmlChar *) "files"))
	{
		child = child->children;
		while (child)
		{
			/*	not supported at the moment as the xml tree is added to the archive
			 *if (!xmlStrcmp(child->name, (xmlChar *) "archive"))
			 {
			 filename = (char*) child->content;
			 }

			 else*/
			{
				if (!xmlStrcmp(child->name, (xmlChar *) "script"))
				{
					sendInfo_sct.scriptname = (char*) child->content;
				}
				if (!xmlStrcmp(child->name, (xmlChar *) "work"))
				{
					sendInfo_sct.workname = (char*) child->content;
				}
				if (filename == NULL)
				{
					memset(&localname[0], 0, sizeof localname);
					strcpy(&localname[0], (char*) node->name);
					xmlDocPtr subDoc = xmlNewDoc((xmlChar *) "1.0");
					xmlDocSetRootElement(subDoc, node);
					char subDocName[20];
					sprintf(subDocName, "%s.xml", (char*) node->name);
					xmlSaveFile(subDocName, subDoc);
					xmlDocSetRootElement(subDoc, NULL);
					xmlFree(subDoc);
					filename = &localname[0];
					while (localname[endOfString_i])
					{
						endOfString_i++;
					}
					strcpy(&localname[endOfString_i], ".tar");
					sprintf(&command[0], "tar -cf %s %s", filename, subDocName); //OUTPUT_XML_NAME
					system(command);
				}

				sprintf(&command[0], "tar -rf %s %s", filename, (char*) child->content);
				system(command);

			}

		}

	}
	else
	{
		sprintf(errormessage, "master:distributeData:No files in %s", (char*) node->name);
		puts(errormessage);
		return errormessage; //FIXME maybe not ideal to give that address back but works for now
	}

	if (endOfString_i > 0) //i is the position of ".tar" in the filename for the archive if 0 no archive was generated
	{
		if (localname == filename)
		{
			memset(&command[0], 0, sizeof command);
			sprintf(&command[0], "gzip  %s", localname);
			system(command);
			strcpy(&localname[endOfString_i + 4], ".gz");
		}
		else
		{
			sprintf(errormessage, "master:distributeData: pointer error");
			puts(errormessage);
			return errormessage; //FIXME
		}
	}

	sendInfo_sct.file_size = fsize(filename);
	sendInfo_sct.IP = (uint32_t) strtol((char*) &(node->name[3]), NULL, 10);
	printf("nodename = %s \t recoveredIP = %d\n", (char*) node->name, sendInfo_sct.IP);

	sendFile_sct.IP = sendInfo_sct.IP;
	sendFile_sct.filename = filename;

	sendInfo(&sendInfo_sct);
	sendFile(&sendFile_sct);
	printf("master:get files and send: %s sent successfully\n", sendFile_sct.filename);

	return NULL;
}

void* createDistributionXML(void *args)
{
	struct cluster_info *clusterInfo_ptr = args;
	int *values = NULL;
	int restNodes;
	char filename[FILENAME_SIZE] = INPUT_XML_NAME;
	xmlDocPtr inputXML = xmlParseFile(filename), outputXML = NULL;
	if (inputXML == NULL)
	{
		printf("XML File not correct\n");
		return NULL;
	}
	pthread_mutex_lock(&clusterInfo_ptr->mtx);
	values = XMLGetMinNodeAndTotalWeight(inputXML);

	if (values[MIN_SHIFT] > clusterInfo_ptr->numNodes_size)
	{
		printf("too few nodes available\t available =%d\t needed =%d\n", clusterInfo_ptr->numNodes_size,
				values[MIN_SHIFT]);
		pthread_mutex_unlock(&clusterInfo_ptr->mtx);
		return NULL;
	}
	else
	{
		outputXML = buildCompleteXML(inputXML, clusterInfo_ptr, values);
		pthread_mutex_unlock(&clusterInfo_ptr->mtx);
	}

	xmlFree(inputXML);
	free(values);

	return outputXML;
}

void * getProgram(void * args)
{

	struct get_Program *getProgram_ptr = args;
	int recvReturn_i = 0, continue_i = 1;
	FILE * pFile;
	int waitForBroadcast_sock, openConnection_sock, received = 0;
	char listenBuff[10], recvBuffer[BUFFERSIZE], ack[4] = "ack";

	struct sockaddr_in listen_addr, connect_addr;
	socklen_t listen_len = sizeof listen_addr, connect_len = sizeof connect_addr;

	xmlDocPtr Distribution_ptr = NULL;

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
	fcntl(waitForBroadcast_sock, F_SETFL, O_NONBLOCK);

	do
	{
		printf("master:getProgram:waiting for broadcast\n");
		continue_i = 1;
		while (continue_i)
		{
			recvReturn_i = recvfrom(waitForBroadcast_sock, &listenBuff[0], sizeof listenBuff, 0,
					(struct sockaddr*) &connect_addr, &connect_len);

			if (getProgram_ptr->exitSignal)
			{
				close(waitForBroadcast_sock);
				return NULL;
			}
			if (recvReturn_i > 0)
				continue_i = 0;
			if (recvReturn_i == -1)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) //should be the same leaving both if there should be a problem
				{
					sleep(1);
				}
				else
				{
					continue_i = 0;
				}

			}
		}
		if (recvReturn_i < 0)
		{

			break;

		}

		printf("received:%s\n", listenBuff);
		if (!strcmp(listenBuff, "fetch"))
		{

			sendto(waitForBroadcast_sock, &ack[0], sizeof ack, 0, (struct sockaddr*) &connect_addr,
					connect_len);
			sleep(2);
			connect_addr.sin_port = htons(TCP_RECV_ARCHIVE_PORT);
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
					printf("recv data = %d\n", recvReturn_i);

				fwrite(&recvBuffer[0], 1, recvReturn_i, pFile);
			} while (recvReturn_i > 0);

			fclose(pFile);
			close(openConnection_sock);
			received = 1;
		}
	} while (received == 0);

	Distribution_ptr = createDistributionXML(getProgram_ptr->clusterInfo_ptr);

	char * ret;
	if (Distribution_ptr)
		ret = distributeData(Distribution_ptr);

	if (ret)
	{
		/*int return_connect = connect(openConnection_sock, (struct sockaddr*) &connect_addr,
		 connect_len);
		 if (return_connect < 0)
		 critErr("master: getProgram: connecterror");
		 */
		//TODO implement result return
	}

	close(waitForBroadcast_sock);

	return NULL;
}

void * distributeData(void * args)
{
	xmlDocPtr doc = args;
	char * errormarker = NULL;

	int endOfString_i, thread_i = 0, allocatedThreads_i = EST_NUM_BOARD;

	pthread_t *send_threads = malloc(allocatedThreads_i * sizeof(pthread_t));
	if (send_threads == NULL)
	{
		sprintf(errormessage, "master:distributeData: malloc failed");
		puts(errormessage);
		return errormessage;
	}

	xmlNodePtr node = NULL;

	node = xmlDocGetRootElement(doc);
	if (node == NULL)
	{
		printf("master:distributeData: wrong doc");
	}
	node = node->children;
	thread_i = 0;
	while (node) //node!=NULL
	{

		if (node->type == XML_ELEMENT_NODE)
		{
			if (thread_i >= allocatedThreads_i)
			{
				pthread_t *new_ptr;
				int newsize_i = allocatedThreads_i + REALLOC_STEPSIZE;
				new_ptr = realloc(send_threads, newsize_i * sizeof(pthread_t));
				if (new_ptr != NULL)
				{
					send_threads = new_ptr;
					allocatedThreads_i = newsize_i;
				}
				else
				{
					free(send_threads);
					printf("master: distributeData :couldn't realloc send_threads");
					exit(5);
				}
			}

			pthread_create((&send_threads[thread_i]), NULL, getFilesAndSend, (void*) node);
			thread_i++;

		}

		node = node->next;

	}
	for (int i = 0; i < allocatedThreads_i; i++)
	{
		char * ret;
		pthread_join(send_threads[i], (void **) &ret);
		if (ret)
			errormarker = ret;
	}

	free(send_threads);
	return errormarker;
}

