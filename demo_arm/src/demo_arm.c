/* **************************************************
 * oledwrite.c
 * (with ZedboardOLED_v1.0 IP core from tamu.edu )
 * write to oled display at OLED
 * 1 parameter - wrapping at end of line
 * LC, 1.10.2015
 * *********************************************** */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define OLED 0x43C00000
#define BUFFERSIZE 64

int write_oled(char * in);

int main(int argc, char *argv[])
{
	uint32_t IP; //network
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
		IP = htonl((uint32_t) strtoul( (argv[1]), NULL, 10));
	send_addr.sin_family =AF_INET;
	send_addr.sin_port = htons(50505);
	send_addr.sin_addr.s_addr=IP;
	}
	char recvBuff[BUFFERSIZE];
	memset(recvBuff, 0, sizeof(recvBuff));

	char sendBuff[BUFFERSIZE+10];
	memset(sendBuff, 0, sizeof(sendBuff));

	listen(recv_sock, 1);

	printf("accept waiting\n");
	int return_accept = accept(recv_sock, NULL,NULL);
	if (return_accept < 0)
		printf("accepterror:%s\n", strerror(errno));
	printf("AAAASDASDASDASD: dest addr=%u",send_addr.sin_addr.s_addr);
	int return_connect = connect(send_sock, (struct sockaddr*) &send_addr,sizeof(send_addr));
	if (return_connect < 0)
			printf("connecterror:%s\n", strerror(errno));
	do
	{
		return_recv = recv(return_accept, recvBuff, BUFFERSIZE, 0);
		if (return_recv < 0)
			printf("recverror:%s\n", strerror(errno));
		else
			printf("recv data = %d", return_recv);

		write_oled(recvBuff);

		strcpy(sendBuff,recvBuff);
		strcat(sendBuff," ack\n");
		return_send = send(send_sock,sendBuff,BUFFERSIZE,0);
		printf("%s",sendBuff);
	} while (strcmp(recvBuff,"end"));

	//for (i = 0;i<10;i++) printf("%c,%c\n",sendBuff[i],recvBuff[i]);

	close(return_accept);
	close(recv_sock);
	close(send_sock);
	return 0;

}
int write_oled(char * in)
{
	char emptyChar = 0;
	char *disp = malloc(65 * sizeof(char));
	int i = 0;

	int reg_addr = OLED;

	size_t page_size = (size_t) sysconf(_SC_PAGESIZE);
	int fd = open("/dev/mem", O_RDWR);

	void *ptr = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, reg_addr);

	int inlen = strlen(in);
	for (i = 0; i < 64; i++)
	{
		if (i >= inlen)
		{
			disp[i] = emptyChar;
		}
		else
		{
			disp[i] = in[i];
		}
	}

	unsigned int reg = 0;
	unsigned int addr = 0;
	int rblk = 0;
	int ltr = 0;
	for (rblk = 0; rblk < 16; rblk++)
	{
		for (ltr = 0; ltr < 4; ltr++)
		{
			reg += (unsigned) disp[((rblk * 4) + (3 - ltr))];
			if (ltr != 3)
				reg = reg << 8;
		}
		addr = OLED + (rblk * 4);
		*((unsigned *) (ptr + (rblk * 4))) = reg;
		reg = 0;
	}

	int cnt = 0;
	int delay = 10000;
	for (cnt = 0; cnt <= delay; cnt++)
	{
		*((unsigned *) (ptr + 64)) = 1;
	}
	for (cnt = 0; cnt <= delay; cnt++)
	{
		*((unsigned *) (ptr + 64)) = 0;
	}

	return 0;
}
