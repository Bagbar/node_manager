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

void mypause(void)
{
	printf("Press [Enter] to continue . . .");
	fflush( stdout);
	getchar();
}

int master_main(int mastBroad_sock, struct cond_mtx *workReady_ptr)
{
	int betternodes_i=0;
	//printf("master_main started\n");
	char keep_alive_c = KEEP_ALIVE_SIGNAL, identify_node_c = IDENTIFY_SIGNAL;

	pthread_t listenForData_thread;
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
	clusterInfo_sct.numNodes_size = 0;
	clusterInfo_sct.size = EST_NUM_BOARD;

	struct get_Program getProgram_sct;
	getProgram_sct.working = 0;
	getProgram_sct.exitSignal = 0;
	getProgram_sct.clusterInfo_ptr = &clusterInfo_sct;
	getProgram_sct.workReady_ptr=workReady_ptr;
	if (pthread_create(&listenForData_thread, NULL, getProgram, &getProgram_sct))
	{
		critErr("pthread_create(getProgram)=");
	}

	struct sockaddr_in broad_addr, loop_addr, recv_addr;
	socklen_t broad_len = sizeof broad_addr, recv_len = sizeof recv_addr, loop_len = sizeof loop_addr;

	fillSockaddrLoop(&loop_addr, UDP_NODE_LISTEN_PORT);
	fillSockaddrBroad(&broad_addr, UDP_NODE_LISTEN_PORT);

	int recvIdent_sock;
	if ((recvIdent_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("master:receive identification socket=");
	}
	fillSockaddrAny(&recv_addr, UDP_N2M_PORT);
	if ((bind(recvIdent_sock, (struct sockaddr*) &recv_addr, sizeof recv_addr)) < 0)
	{
		critErr("master:bind receive identification socket:");
	}
	fcntl(recvIdent_sock, F_SETFL, O_NONBLOCK);

	sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
			(struct sockaddr*) &broad_addr, broad_len);

	/*sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0, (struct sockaddr*) &loop_addr,
	 loop_len);
	 */

	readIdentifyAnswers(recvIdent_sock, &clusterInfo_sct, 1);

	printf("master:\t\tnodes in cluster = %u\n", clusterInfo_sct.numNodes_size);

	while (master_i)
	{
		if (identify_counter_i == 0)
		{

			sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
					(struct sockaddr*) &broad_addr, broad_len);

			sendto(mastBroad_sock, &identify_node_c, sizeof identify_node_c, 0,
					(struct sockaddr*) &loop_addr, loop_len);
			if( updateClusterInfo(&clusterInfo_sct, recvIdent_sock) && getProgram_sct.working == 0){
				master_i = 0;printf("????????????????????????????????????");} //TODO switch: quit master when higher priority is present
			printf("master:\t\tnodes in cluster = %u\n", clusterInfo_sct.numNodes_size);
		}
		else
		{

			sendto(mastBroad_sock, &keep_alive_c, sizeof keep_alive_c, 0, (struct sockaddr*) &broad_addr,
					broad_len);
		}

		sleep(PING_PERIOD);
		identify_counter_i = (identify_counter_i + 1) % PINGS_PER_IDENTIFY;
		//printf("master:identify_counter=%d\n", identify_counter_i);
		if(master_i==0)
		{
			printf("master: canceling get Program\n\n");
			getProgram_sct.exitSignal =1;
			pthread_cancel(listenForData_thread);
		}


	}
	free(clusterInfo_sct.node_data_list_ptr);
	return 0;
}
void addNode2List(struct cluster_info *clusterInfo_ptr, uint32_t ip_u32, uint8_t *typeAndGroup)
{
	//printf("master: addNode started\n");
	int size_i;
	void *newList_ptr;
	if (pthread_mutex_lock(&clusterInfo_ptr->mtx))
			perror("master:addNode: mutex lock:");
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
	struct node_data *newNode = &clusterInfo_ptr->node_data_list_ptr[clusterInfo_ptr->numNodes_size];
	newNode->ip_u32 = ip_u32;
	newNode->type_u8 = typeAndGroup[0];
	newNode->group_u8 = typeAndGroup[1];
	newNode->lastAlive_u8 = clusterInfo_ptr->alive_count_u8;
	newNode->nowActive_u8 = 0;
	printf("ip=%u \t type = %u added \t IP_dotted=%s\n",
			newNode->ip_u32, typeAndGroup[0],
			hostToDottedIP(newNode->ip_u32));

	clusterInfo_ptr->numNodes_size++;
	if (pthread_mutex_unlock(&clusterInfo_ptr->mtx))
	perror("master:addNode: mutex unlock:");
}

int readIdentifyAnswers(int receive_sock, struct cluster_info *clusterInfo_ptr, uint8_t newList_u8)
{
	int return_value=0;
	//printf("master:readIdentifyAnswers started\n");
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
		//printf("master:readIdentifyAnswers: receive_return : %d\n", returnRecv_i);
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
			if(typeAndGroup[0]<FPGATYPE)
			{
				return_value++;
			}
			IP_holder = ntohl(response_addr.sin_addr.s_addr);
			if (IP_holder == 0x7F000001) //loopback
				IP_holder = ownIP;
			if (newList_u8)
			{
				addNode2List(clusterInfo_ptr, IP_holder, &typeAndGroup[0]);
				//printf("master:readIdentify: called addNode for new list\n");
			}
			else
			{
				compare_node.ip_u32 = IP_holder;
				searchReturn_ptr = (struct node_data*) bsearch(&compare_node,
						clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size,
						sizeof(struct node_data), compareNodes);
				if (searchReturn_ptr == NULL)
				{
					//printf("master:readIdentify: called addNode for existing list\n");
					addNode2List(clusterInfo_ptr, IP_holder, &typeAndGroup[0]);
					qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size,
							sizeof(struct node_data), compareNodes);
				}
				else
				{
					//printf("master_readIdentify: node already exists ip=%u\t IPdotted=%s\n",searchReturn_ptr->ip_u32,hostToDottedIP(searchReturn_ptr->ip_u32));
					searchReturn_ptr->lastAlive_u8 = clusterInfo_ptr->alive_count_u8;
				}
			}
		}

	}

	if (newList_u8)
	{ //printf("master:qsort for new list started\n");
		qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size,
				sizeof(struct node_data), compareNodes);
	}
//printf("master:readIdentify finished\n");
	return return_value;
}

int updateClusterInfo(struct cluster_info *clusterInfo_ptr, int receive_sock)
{
	//printf("master: updateClusterInfo: start\n");
	int i, outdated_i = 0;
//	int timeout_i = TIMEOUT, returnRecv_i;
//
//	struct node_data *searchReturn_ptr;
//	uint8_t boardtype_u8;
//	socklen_t response_len;
	clusterInfo_ptr->alive_count_u8++;

	int return_value = readIdentifyAnswers(receive_sock, clusterInfo_ptr, 0);

	// remove non active nodes
	for (i = 0; i < clusterInfo_ptr->numNodes_size; i++)
	{
		if (clusterInfo_ptr->node_data_list_ptr[i].lastAlive_u8 != clusterInfo_ptr->alive_count_u8)
		{
			printf("master: update Info : removing node counter = %d\t ip = %u \t dotted:%s\n",clusterInfo_ptr->node_data_list_ptr[i].lastAlive_u8,clusterInfo_ptr->node_data_list_ptr[i].ip_u32,hostToDottedIP(clusterInfo_ptr->node_data_list_ptr[i].ip_u32));
			clusterInfo_ptr->node_data_list_ptr[i].ip_u32 = -1;
			clusterInfo_ptr->node_data_list_ptr->nowActive_u8 = 0;

			outdated_i++;
		}
	}
	//printf("master:updateCluster:outdated = %d\n",outdated_i);
	qsort(clusterInfo_ptr->node_data_list_ptr, clusterInfo_ptr->numNodes_size,
			sizeof(struct node_data), compareNodes);
	if (pthread_mutex_lock(&clusterInfo_ptr->mtx))
		perror("master:updateCluster: mutex lock:");
	clusterInfo_ptr->numNodes_size = clusterInfo_ptr->numNodes_size - outdated_i;
	if (pthread_mutex_unlock(&clusterInfo_ptr->mtx))
		perror("master:updateCluster: mutex unlock:");

	return return_value;
}

void *sendInfo(void *args)
{
	printf("master:send_info started\n");
	struct send_info *send_info_ptr = args;
	uint8_t check_u8;
	struct fileInfo_bufferformat sendBuff;
	//printf("master:sendInfo: filesize = %d\n",sendBuff.file_size);
	sendBuff.file_size = send_info_ptr->file_size;
	sendBuff.cancel = 0;
	memset(sendBuff.scriptname, 0, FILENAME_SIZE);
	memset(sendBuff.workname, 0, FILENAME_SIZE);
	strcpy(sendBuff.scriptname, send_info_ptr->scriptname);
	strcpy(sendBuff.workname, send_info_ptr->workname);
	sendBuff.file_size = send_info_ptr->file_size;
	//printf("master:sendinfo: filesize = %d\n",sendBuff.file_size);

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(TCP_RECV_INFO_PORT);
	dest_addr.sin_addr.s_addr = send_info_ptr->IP; //has to be converted already

	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		printf("master: recv_info: socketerror:%s\n", strerror(errno));
	printf("master:send_info: connect \n");
	int return_connect = connect(return_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
	if (return_connect < 0)
		critErr("master: send_info: connecterror");
	printf("master:send_info: send buffersize =%u \n", sizeof sendBuff);
	do
	{

		int sendReturn_i = send(return_socket, &sendBuff, sizeof sendBuff, 0);
		if (sendReturn_i < 0)
			printf("master: send_info: senderror:%s\n", strerror(errno));

		int return_recv = recv(return_socket, &check_u8, 1, 0);
		if (return_recv < 0)
			printf("recverror:%s\n", strerror(errno));

		if (check_u8 != CHECK_OKAY)
			printf("master: send_info: check_failed ->retry\n");
	} while (check_u8 != CHECK_OKAY);
	close(return_socket);
	printf("master:send info:finished\n");
	return NULL;

}
void *sendFile(void *args)
{
	struct send_file *sendFile_ptr = args;
	uint8_t check_u8;
	printf("master:sending_file\n");
	char sendBuff[BUFFERSIZE];
	memset(sendBuff, '0', sizeof(sendBuff));
	int sendReturn_i;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(TCP_RECV_ARCHIVE_PORT);
	dest_addr.sin_addr.s_addr = sendFile_ptr->IP;
	int return_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (return_socket < 0)
		critErr("master:send_info: socket error:");
	do
	{
		printf("AAAASDASDASDASD: dest addr=%u",dest_addr.sin_addr.s_addr);
		int return_connect = connect(return_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
		if (return_connect < 0)
			critErr("master:send_file:connect error:");

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
			sendReturn_i = send(return_socket, &sendBuff[0], result, 0);
			if (sendReturn_i < 0)
				printf("master: send_file: senderror:%s\n", strerror(errno));
			else
				printf("send data = %d\n", sendReturn_i);
		} while (feof(pFile) == 0);
		// terminate

		fclose(pFile);
		printf("master:send_file:file_sent:%s\n", sendFile_ptr->filename);
		if (close(return_socket) < 0)
			printf("closeerror:%s\n", strerror(errno));

		return_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (return_socket < 0)
			critErr("master:send_info: socket error:");

		if (connect(return_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr)))
			critErr("master:send_file: recv check :connect error:");
		recv(return_socket, &check_u8, 1, 0);

		if (close(return_socket) < 0)
			printf("closeerror:%s\n", strerror(errno));
		if (check_u8 != CHECK_OKAY)
		{
			printf("master: send_file: %s check_failed ->retry\n", sendFile_ptr->filename);
			sleep(1);
		}
	} while (check_u8 != CHECK_OKAY);
	printf("master:send file:finished\n");
	return NULL;
}

void* getFilesAndSend(void* args)
{
	printf("getfilesandsend started \n");
	xmlNodePtr node = args, child = NULL;

	struct send_info sendInfo_sct;
	struct send_file sendFile_sct;

	int endOfString_i = 0;
	char *filename = NULL, localname[FILENAME_SIZE], command[FILENAME_SIZE + 40];

	child = node->children;
	while (child != NULL && xmlStrcmp(child->name, (xmlChar *) "files"))
	{
		child = child->next;
	}
	//printf("master:getFileandSend: files found=%s\n", child->name);
	if (!xmlStrcmp(child->name, (xmlChar *) "files"))
	{
		child = child->children;

		while (child)
		{
			printf("master:get files and send: child = %s \t type=%d \t content=%s\n", child->name,child->type, xmlNodeGetContent(child));


			if (child->type == XML_ELEMENT_NODE)
			{
				{
					if (!xmlStrcmp(child->name, (xmlChar *) "script"))
					{
						sendInfo_sct.scriptname = (char*) xmlNodeGetContent(child);
					}
					if (!xmlStrcmp(child->name, (xmlChar *) "work"))
					{
						sendInfo_sct.workname = (char*) xmlNodeGetContent(child);
					}
					if (filename == NULL)
					{
						memset(&localname[0], 0, sizeof localname);
						strcpy(&localname[0], (char*) node->name);
						xmlDocPtr subDoc = xmlNewDoc((xmlChar *) "1.0");
						//printf("master:getFilesAndSend: doc set \t \t node->next=%p\n",node->next);
						xmlNodePtr newnode = xmlCopyNode(node, 1);
						if (newnode == NULL)
							critErr("master_getfilesandsend: node copy error");
						xmlDocSetRootElement(subDoc, newnode);
						//printf("master:getFilesAndSend:doc set done \t \t node->next=%p\n",node->next);
						char subDocName[20];
						sprintf(subDocName, "%s.xml", (char*) node->name);
						xmlSaveFile(subDocName, subDoc);

						xmlFreeDoc(subDoc);

						filename = &localname[0];
						while (localname[endOfString_i])
						{
							endOfString_i++;
						}
						//printf("master:get files and send: filename =%p\t localname=%p\t *localname=%s\n \t\t subdocname=",filename, localname, localname, subDocName);
						strcpy(&localname[endOfString_i], ".tar");
						sprintf(&command[0], "tar -cf %s %s", filename, subDocName);
						puts(command);
						system(command);
					}

					sprintf(&command[0], "tar -rf %s %s", filename, (char*) xmlNodeGetContent(child));
					puts(command);
					system(command);
					//printf("master:get files and send: system = %d\t command= %s \n", system(command),command);

				}
			}
			child = child->next;
		}

	}
	else
	{
		sprintf(errormessage, "master:distributeData:No files in %s", (char*) node->name);
		puts(errormessage);
		return errormessage; //FIXME maybe not ideal to give that address back but works for now
	}
	//printf("master:getFileandSend: endOfString = %d\n", endOfString_i);
	if (endOfString_i > 0) //i is the position of ".tar" in the filename for the archive if 0 no archive was generated
	{
		if (localname == filename)
		{
			memset(&command[0], 0, sizeof command);
			sprintf(&command[0], "gzip -f %s", localname);
			puts(command);
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
	//printf("master:getFilesAndSend:generating sendFile \t \t node=%p\n",node);
	sendInfo_sct.IP = htonl((uint32_t) strtol((char*) &(node->name[3]), NULL, 10));
	//printf("nodename = %s \t recoveredIP = %u\n", (char*) node->name, ntohl(sendInfo_sct.IP));

	sendFile_sct.IP = sendInfo_sct.IP;
	sendFile_sct.filename = filename;
	//printf("master:getfilesandsend: calling send functions\n");
	sendInfo(&sendInfo_sct);
	sendFile(&sendFile_sct);
	//printf("master:get files and send: %s sent successfully\n", sendFile_sct.filename);

	return NULL;
}

void* createDistributionXML(void *args)
{
	struct cluster_info *clusterInfo_ptr = args;
	int *values = NULL;
	char filename[FILENAME_SIZE] = INPUT_XML_NAME;
	xmlDocPtr inputXML = xmlParseFile(filename), outputXML = NULL;
	if (inputXML == NULL)
	{
		printf("XML File not correct\n");
		return NULL;
	}
	if(pthread_mutex_lock(&clusterInfo_ptr->mtx))
	perror("master:create Distribution XML: mutex lock:");
	values = XMLGetMinNodeAndTotalWeight(inputXML);

	if (values[MIN_SHIFT] > clusterInfo_ptr->numNodes_size)
	{
		printf("too few nodes available\t available =%u\t needed =%d\n", clusterInfo_ptr->numNodes_size,
				values[MIN_SHIFT]);
		if(pthread_mutex_unlock(&clusterInfo_ptr->mtx))
		perror("master:create Distribution XML: mutex unlock:");
		return NULL;
	}
	else
	{
		outputXML = buildCompleteXML(inputXML, clusterInfo_ptr, values);
		if(pthread_mutex_unlock(&clusterInfo_ptr->mtx))
				perror("master:create Distribution XML: mutex unlock:");
	}

	xmlFreeDoc(inputXML);
	free(values);

	return outputXML;
}

void * getProgram(void * args)
{

	struct get_Program *getProgram_ptr = args;
	printf("master:get Program: exit signal = %d\n",getProgram_ptr->exitSignal);
	while(getProgram_ptr->exitSignal==0)
	{
	int recvReturn_i = 0, continue_i = 1, received_i = 0, result=0, sendReturn_i;
	FILE * pFile;
	int waitForBroadcast_sock, openConnection_sock,returnSolution_sock;
	char listenBuff[10], buffer[BUFFERSIZE], ack[4] = "ack";

	struct sockaddr_in listen_addr, connect_addr;
	socklen_t listen_len = sizeof listen_addr, connect_len = sizeof connect_addr;

	xmlDocPtr Distribution_ptr = NULL;

	if ((waitForBroadcast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		critErr("master:get Program:wait_socket=");
	}

	if ((openConnection_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		critErr("master:get Program:connect_socket=");
	}

	fillSockaddrAny(&listen_addr, UDP_OPEN_TCP_CONNECTION_FOR_PROGRAM_TRANSFER);
	if ((bind(waitForBroadcast_sock, (struct sockaddr*) &listen_addr, sizeof(listen_addr))) < 0)
	{
		critErr("master:get Program:bind elect_recv_sock:");
	}
	fcntl(waitForBroadcast_sock, F_SETFL, O_NONBLOCK);

	do
	{
		printf("master:get Program:waiting for broadcast\n");
		continue_i = 1;
		while (continue_i)
		{

			recvReturn_i = recvfrom(waitForBroadcast_sock, &listenBuff[0], sizeof listenBuff, 0,
					(struct sockaddr*) &connect_addr, &connect_len);
			getProgram_ptr->working = 1;
			if (getProgram_ptr->exitSignal)
			{
				printf("master:get Program:exiting \n");
				close(waitForBroadcast_sock);
				getProgram_ptr->working = 0;
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

		printf("master:get Program:received:%s\n", listenBuff);
		if (!strcmp(listenBuff, "fetch"))
		{

			sendto(waitForBroadcast_sock, &ack[0], sizeof ack, 0, (struct sockaddr*) &connect_addr,
					connect_len);
			close(waitForBroadcast_sock);
			sleep(1);
			connect_addr.sin_port = htons(TCP_GET_PROGRAM);
			//printf("master:getProgram: connecting\n");

			if (connect(openConnection_sock, (struct sockaddr*) &connect_addr,	connect_len) < 0)
				critErr("master: get Program: connecterror");
			pFile = fopen("data.tar", "wb");
			if (pFile == NULL)
			{
				fputs("master:get Program: File error", stderr);
				exit(1);
			}
			//printf("master:get Program: receiving file\n");
			do
			{
				recvReturn_i = recv(openConnection_sock, &buffer[0], BUFFERSIZE, 0);
				if (recvReturn_i < 0)
					printf("master:get Program: recverror:%s\n", strerror(errno));
				else{
					//printf("master:get Program: recv data = %d\n", recvReturn_i);
				}
				fwrite(&buffer[0], 1, recvReturn_i, pFile);
			} while (recvReturn_i > 0);

			fclose(pFile);
			close(openConnection_sock);
			received_i = 1;
		}
	} while (received_i == 0);
	system("tar xf data.tar");
	Distribution_ptr = createDistributionXML(getProgram_ptr->clusterInfo_ptr);

	char * ret = NULL;
	//printf("master:get Program: dist ptr = %p\n", Distribution_ptr);
	if(pthread_mutex_lock(&getProgram_ptr->workReady_ptr->mtx))
				perror("master:get Program: mutex lock:");
	if (Distribution_ptr)
	{
		ret = distributeData(Distribution_ptr);
		xmlFreeDoc(Distribution_ptr);
	}
	printf("master:get Program: ret = %p", ret);

	printf("master: get Program: condition mutex locked and wait\n");
	if(pthread_cond_wait(&getProgram_ptr->workReady_ptr->cond,&getProgram_ptr->workReady_ptr->mtx))
		perror("master:get Program:condition wait:");
	printf("master: get Program: condition continued\n");


	if ((returnSolution_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			critErr("master:get Program:wait_socket=");
		}
	if( 0 > connect(returnSolution_sock, (struct sockaddr*) &connect_addr,
						connect_len))
			critErr("master: get Program: solution connect error");
	if (ret)
	{
		 send(returnSolution_sock,ret,sizeof(errormessage),0);
	}
	else
	{

		pFile = fopen(RETURN_NAME, "rb");
				if (pFile == NULL)
				{
					char msg[50];
					memset(msg,0,sizeof msg);
					sprintf(msg,"master: get Program:return file error:%d",errno);
					puts(msg);
					send(returnSolution_sock,msg,sizeof(msg),0);
				}
				else
				{
					do
							{
								// copy the file into the buffer:
								result = fread(buffer, 1, BUFFERSIZE, pFile);
								if (result != BUFFERSIZE && feof(pFile) == 0)
								{
									printf("master:get Program :Reading error\n");
									exit(-1);
								}
								sendReturn_i = send(returnSolution_sock, &buffer[0], result, 0);
								if (sendReturn_i < 0)
									printf("master: get Program: senderror:%s\n", strerror(errno));
								else{
									//printf("send data = %d\n", sendReturn_i);
								}
							} while (feof(pFile) == 0);

				}
	}

	close(returnSolution_sock);
	if(pthread_mutex_unlock(&getProgram_ptr->workReady_ptr->mtx))
		perror("master:get Program: mutex unlock:");
	printf("master: get Program: condition mutex unlocked \n");
	getProgram_ptr->working = 0;
	printf("master:get Program: exit signal = %d\n",getProgram_ptr->exitSignal);
}
	return NULL;
}

void * distributeData(void * args)
{
	printf("master:distributeDAta started\n");
	xmlDocPtr doc = args;
	char * errormarker = NULL;

	int  thread_i = 0, allocatedThreads_i = EST_NUM_BOARD;

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
		printf("master:distributeData: node = %s \t %p\t node->next=%p\n", node->name, node,
				node->next);
		if (node->next)
			printf("master:distributeData: node->next->name = %s\n", node->next->name);
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
			//getFilesAndSend(node);
			pthread_create((&send_threads[thread_i]), NULL, getFilesAndSend, (void*) node);
			printf("master:distribute data:getFilesAndSend thread created : %d\n", thread_i);
			thread_i++;

		}
		printf("master:distributeData: \t \t node=%p \tnode->next=%p\n", node, node->next);
		node = node->next;

	}
	char * ret;
	for (int i = 0; i < thread_i; i++)
	{
		ret = NULL;
		pthread_join(send_threads[i], (void **) &ret);
		printf("master:distribute data:getFilesAndSend thread joined\n");
		if (ret)
			errormarker = ret;
	}

	free(send_threads);
	return errormarker;
}

