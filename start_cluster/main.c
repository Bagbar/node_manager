
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <arpa/inet.h>

#define UDP_OPEN_TCP_CONNECTION_FOR_PROGRAM_TRANSFER 51000
#define TCP_GET_PROGRAM 51001
#define TCP_SEND_SOLUTION 51002


#define BUFFERSIZE 1024

char* hostToDottedIP(uint32_t ip)
{

	unsigned char bytes[4];
	char *IP = malloc(16);
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	sprintf(IP, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
	return IP;
}

int main(int argc, const char* argv[])
{
	printf("%d\n",argc);
	if (argc > 1)
	{
		size_t bytesRead_size;
		int sendReturn_i, recvReturn_i;
		FILE * pFile;
		int broadcast_sock, sendProg_sock,sendProg_accept,recvSolution_sock,recvSolution_accept;
		char broadSendBuff[10] = "fetch", broadRecvBuff[100], fileSendBuff[BUFFERSIZE],fileRecvBuff[BUFFERSIZE];
		struct sockaddr_in broadcast_addr, sendProgram_addr, master_addr,recvSolution_addr;
				socklen_t broadcast_len = sizeof broadcast_addr, connect_len = sizeof sendProgram_addr, master_len= sizeof master_addr;

		printf("opening: %s\n",argv[1]);
		pFile = fopen(argv[1], "rb");
		if (pFile == NULL)
		{
			fputs("File error", stderr);
			exit(-1);
		}
		printf("opened: %s\n",argv[1]);

		if ((broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			fclose(pFile);
			fputs("broadcast_socket=", stderr);
						exit(-1);
		}
		if ((recvSolution_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
				{
					fclose(pFile);
					fputs("recvSolution_socket=", stderr);
								exit(-1);
				}

		if ((sendProg_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			fclose(pFile);
			fputs("transfer_socket=", stderr);
			exit(-1);
		}
		int broadcastEnable = 1;
		int ret = setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST,
					&broadcastEnable, sizeof(broadcastEnable));
			if (ret)
				fputs("main: couldn't set setsockopt:",stderr);

		broadcast_addr.sin_family = AF_INET;
		broadcast_addr.sin_port = htons(UDP_OPEN_TCP_CONNECTION_FOR_PROGRAM_TRANSFER);
		broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

		sendProgram_addr.sin_family = AF_INET;
		sendProgram_addr.sin_port = htons(TCP_GET_PROGRAM);
		sendProgram_addr.sin_addr.s_addr=htonl(INADDR_ANY);

		recvSolution_addr.sin_family = AF_INET;
		recvSolution_addr.sin_port = htons(TCP_SEND_SOLUTION);
		recvSolution_addr.sin_addr.s_addr=htonl(INADDR_ANY);

		printf("sending broadcast\n");
		sendReturn_i = sendto(broadcast_sock, &broadSendBuff[0], sizeof broadSendBuff, 0,
				(struct sockaddr*) &broadcast_addr, broadcast_len);
		printf("sent:%d\n",sendReturn_i);
		if(sendReturn_i <0)
			printf("senderror:%s", strerror(errno));
		printf("waiting for reply\n");
		recvReturn_i = recvfrom(broadcast_sock, &broadRecvBuff[0], 10, 0,
				(struct sockaddr*) &master_addr, &master_len);
		printf("received\n");

		//connect_addr.sin_addr.s_addr = master_addr.sin_addr.s_addr;
		printf("bind master IP = %s\n",hostToDottedIP( ntohl(sendProgram_addr.sin_addr.s_addr)));
		int return_bind = bind(sendProg_sock, (struct sockaddr*) &sendProgram_addr, connect_len);
		if(return_bind <0)
		{
			printf("bind error=%d",errno);
			exit(1);
		}
		listen(sendProg_sock, 1);

		printf("connection ack\n");
		if (!strcmp(broadRecvBuff, "ack"))
		{
			printf("waiting for accept\n");
			sendProg_accept = accept(sendProg_sock,NULL,NULL);
			printf("accepted\n");
			if (sendProg_accept < 0)
				printf("accepterror:%s\n", strerror(errno));


			do{
			bytesRead_size = fread(fileSendBuff, 1, BUFFERSIZE, pFile);
				if (bytesRead_size != BUFFERSIZE && feof(pFile)==0)
				{
					fputs("Reading error", stderr);
					exit(3);
				}
				sendReturn_i = send(sendProg_accept, fileSendBuff, bytesRead_size, 0);
					if (sendReturn_i < 0)
						printf("senderror:%s", strerror(errno));
					else
						printf("send data = %d\n", sendReturn_i);
				/* the whole file is now loaded in the memory buffer. */
				}while(feof(pFile)==0);
				// terminate
				fclose(pFile);

		}
		close(sendProg_accept);
		close(broadcast_sock);

		printf("finished transmission starting receive\n");



		    	pFile = fopen("return.file", "wb");
		    	if (pFile == NULL)
		    	{
		    		fputs("File error", stderr);
		    		exit(1);
		    	}


		printf("waiting for accept\n");
		sendProg_accept = accept(sendProg_sock,NULL,NULL);
					printf("accepted\n");
					if (sendProg_accept < 0)
						printf("accepterror:%s\n", strerror(errno));
					do{
					  recvReturn_i = recv(sendProg_accept,fileRecvBuff,BUFFERSIZE,0);
					    if (recvReturn_i<0)
					    	printf("recverror:%s\n",strerror(errno));
					    else
					    	printf("recv data = %d",recvReturn_i);

					    fwrite(fileRecvBuff,1,recvReturn_i,pFile);


					    	}while(recvReturn_i>0);
					fclose(pFile);
					close(sendProg_accept);
					close(sendProg_sock);

		return 0;
	}

	else
	{
		printf("no file declared");
		return -1;
	}



}
