#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFERSIZE 64


int main(int argc, char *argv[])
{
	int i;
	uint32_t IP;//network
	int return_recv, return_send;
	//struct sockaddr_in sender_addr;
	//socklen_t len;

	int recv_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (recv_sock < 0)
		perror("recv_sockerror:%s\n");

	struct sockaddr_in recv_addr,send_addr;
	recv_addr.sin_family = AF_INET;
	recv_addr.sin_port = htons(50505);
	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("192.168.75.1");
	int return_bind = bind(recv_sock, (struct sockaddr*) &recv_addr, sizeof(recv_addr));
	if (return_bind < 0)
		printf("binderror:%s\n", strerror(errno));

	int send_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (send_sock < 0)
			perror("send_sockerror:%s\n");


	if(argc >1){
		IP = htonl((uint32_t) strtol( (argv[1]), NULL, 10));
	send_addr.sin_family =AF_INET;
	send_addr.sin_port = htons(50505);
	send_addr.sin_addr.s_addr=IP;
	}
	char recvBuff[BUFFERSIZE+10];
	memset(recvBuff, '0', sizeof(recvBuff));

	char sendBuff[BUFFERSIZE];
	memset(sendBuff, 0, sizeof(sendBuff));

	FILE * pFile;

		pFile = fopen("return.file", "wb	");
		if (pFile == NULL)
		{
			fputs("File error", stderr);
			exit(1);
		}



	listen(recv_sock, 1);
	sleep(2);
	int return_connect = connect(send_sock, (struct sockaddr*) &send_addr,sizeof(send_addr));
		if (return_connect < 0)
				printf("work:connecterror:%s\n", strerror(errno));

	int return_accept = accept(recv_sock, NULL,NULL);
	if (return_accept < 0)
		printf("accepterror:%s\n", strerror(errno));
		do
	{

		printf("write message\n");
		scanf("%s",sendBuff);
		i=0;
		while(sendBuff[i])
			i++;

		return_send = send(send_sock,sendBuff,i,0);

		return_recv = recv(return_accept, recvBuff, BUFFERSIZE+10, 0);
				if (return_recv < 0)
					printf("recverror:%s\n", strerror(errno));
				else
					printf("recv data = %d\n", return_recv);

				fwrite(recvBuff,1,return_recv,pFile);
				printf("%s \t i=%d\n",sendBuff,i);
	} while (strcmp(sendBuff,"end"));

	//for (i = 0;i<10;i++) printf("%c,%c\n",sendBuff[i],recvBuff[i]);

	fclose(pFile);
	close(return_accept);
	close(recv_sock);
	close(send_sock);
	return 0;

}
