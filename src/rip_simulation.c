#include<stdio.h>
#include<stdlib.h>
#include<netdb.h>
#include<pthread.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include "rip.h"
int main(int argc, char *argv[])
{
  neighbours = NULL;
  char fname[20];
  struct sockaddr_in my_addr;
  char hostname[100];
  struct hostent *he;
  int yes = 1;   
  pthread_t thr_cli;
   strcpy(fname, argv[2]);
  if(argc != 3) {
    printf("\nIncorrect number of arguments\nneighbour and log file name required\n");
    exit(-1);
  }
  
  /* open logfile */
  if((logfile = fopen(argv[2], "rw+")) == NULL) {
    printf("\nError in opening file:%s\n",argv[2]);
    exit(-1);
  }
  
  /* open udp socket */
  if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    printf("\nSocket creation error \n");
    exit(-1);
  }
  
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(RIP_PORT);
  my_addr.sin_addr.s_addr = INADDR_ANY;
  memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

  /* bind socket */
  if(bind(sock, (struct sockaddr *)&my_addr, sizeof my_addr)) {
    printf("\nSocket bind error in input()\n");
    perror("");
    exit(0);
    } 
  
  /* get own ip address */
  if(gethostname(hostname, 100) == -1)
    perror("");
  he = gethostbyname(hostname);
  ownip = *(struct in_addr *)he->h_addr;
  
  /* initialise timers */
  if(init_timer() == -1) {
    printf("\nTimer init error\n");
  }
  
  init(argv[1]);
  
  /* spawn cli thread */
  if(pthread_create(&thr_cli, NULL, cli, NULL) != 0) {
    perror("");
    return -1;
  }

  /* start update timer */
  start_timer(UPDTIME, output, NULL, NULL, '1');
  
  while(1) {
    input();
    print_route_table(1);
  }
  
  fclose(logfile);
  return 0;
}
