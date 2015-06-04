#include "basics.h"

#define BUFFERSIZE 10

// int listen_for_master(struct slave_network_param *timeout_counter) with implicit cast may work but seems dirty
void *listen_for_master(void *timeout_struct)
{
  struct slave_network_param timeout_counter =*(struct slave_network_param*)timeout_struct;
  char recvBuff[BUFFERSIZE];
  memset(recvBuff, '0' ,BUFFERSIZE);


  char board_type='1';// TODO (kami#5#): implement boardtype
  int udp_sock;

  ///create UDP-Socket Server
  if((udp_sock=socket(AF_INET,SOCK_DGRAM,0))==-1)
  {
    criterr("socket=",udp_sock);
  }

  struct sockaddr_in serv_addr,cli_addr;
  socklen_t cli_len=sizeof(cli_addr);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(50505);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if((bind(udp_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)))<0)
  {
    criterr("bind",udp_sock);
  }

  while(1)
  {
    printf("while:\n");
    /// Recieve msg from master,
    int return_recv = recvfrom(udp_sock,recvBuff,2,0,(struct sockaddr*)&cli_addr,&cli_len);
    /// TODO (kami#9#): remove printf
    printf("return_recv=%d\t recvBuff[0]=%c",return_recv,recvBuff[0]);

    // TODO (kami#5#)check if cases are all what is needed
    switch(recvBuff[0])
    {
    // TODO (kami#6#): implement case o
    /* case o : ///identify other: measure the connection to everyone and send to caller

        break;*/
    case 's' : ///identify self : send your Type to caller
    //printf("send:%d",(int)sendto(udp_sock,&board_type,1,0,(struct sockaddr*)&cli_addr,cli_len));
    //break;
    case 'k' : ///keepalive
      printf("send:%d",(int)sendto(udp_sock,&board_type,1,0,(struct sockaddr*)&cli_addr,cli_len));
      pthread_mutex_lock(&timeout_counter.mtx);
      timeout_counter.var=0;
      pthread_mutex_unlock(&timeout_counter.mtx);
      break;
    default :
      printf("Unknown symbol: recvBuff[0]=%c",recvBuff[0]);
      /// NOTE (kami#9#): remove exit in final?
      close(udp_sock);
      exit(2);
    }
  }
}


int main()
{
  struct slave_network_param timeout_counter;
  timeout_counter.var=0;

  pthread_t background_listen;
  if(pthread_create(&background_listen, NULL, listen_for_master, (void*)&timeout_counter))
  {
    criterr("pthread_create=",0);
  }

  sleep(10);
  while(1)
  {
    printf("Hallo\n");
    // NOTE May need a mutex
    if (timeout_counter.var>PING_PERIOD*TIMEOUT_PERIODS)
    {
      // TODO (kami#1#): start search for new master
      printf("over\n");
    }
    else
    {
      printf("under\n");
      pthread_mutex_lock(&timeout_counter.mtx);
      printf("%d\n",timeout_counter.var++);
      pthread_mutex_unlock(&timeout_counter.mtx);
      printf("underchanged\n");
    }
    // NOTE (kami#9#): may use PING_PERIOD here
    sleep(1);
    printf("?");
  }
  return 0;
}
