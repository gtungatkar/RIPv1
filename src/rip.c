 
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<pthread.h>
#include<sys/ipc.h>
#include<semaphore.h>
#include<pthread.h>
#include "log.h"
#include "timer.h"
#include "rip.h"

byte packet_type(char *m)
{
  byte command;
  command = *(byte *)m;
  return command;
} /* packet_type */

void print_rip_header(struct rip_header *rh)
{
  LOG("\nRIP HEADER: command = %d \n",rh->command);
} 


void print_rip_entry(struct rip_entry *rte)
{
  LOG("\nRIP ENTRY: AF = %d\tdestination = %s\t metric = %d\n",rte->addrfamily,inet_ntoa(rte->destination),rte->metric);
}

struct route_entry * alloc_route_entry()
{
  struct route_entry *temp = NULL;
  LOG_FLOW("\nentering alloc_route_entry\n");
  if((temp = (struct route_entry *)malloc(sizeof(struct route_entry))) == NULL) {
    WARN("\nmalloc failed in alloc_route_entry()\n");
    exit(-1);
  }
  LOG_FLOW("\nexiting alloc_route_entry\n");
  return temp;
} /* alloc_route_entry */

struct neighbour * alloc_neighbour()
{
  struct neighbour *temp = NULL;
  LOG_FLOW("\nentering alloc_neighbour\n");
  if((temp = (struct neighbour *)malloc(sizeof(struct neighbour))) == NULL) {
    WARN("\nmalloc failed in alloc_neighbour()\n");
    exit(-1);
  }
  LOG_FLOW("\nexiting alloc_route_entry\n");
  return temp;
} /* alloc_neighbour */

/* Purpose: read one ip address at a time from the neighbours file */
int read_neighbour_ip(FILE *neighbour_file, struct in_addr *ip) 
{
  char buff[20];
  char c = 0;
  int i = 0;
  LOG_FLOW("entering read_neighbour_ip\n");
  fflush(stdin);
  while(1) {
    c = (unsigned char)fgetc(neighbour_file);
    if(c == EOF)
      break;
    if(c == '$') { 		/* buff now contains one ip addr */
      buff[i] = '\0';
      i = 0;
      LOG("\nip address read from neighbour file  = %s\n", buff);
      inet_aton(buff,ip);
      LOG_FLOW("exiting read_neighbour_ip successfully\n");
      return 0;
    }
    else {
      if(i > 20)
	break;
      buff[i++] = c;
    }
  }
  WARN("\nNo IP address read from neighbour file\n");
  LOG_FLOW("exiting read_neighbour_ip :failed\n");
  return -1;

} /* read_neighbour_ip */

void set_route_entry(struct route_entry *re, struct in_addr destination, uint32_t metric, struct in_addr nexthop)
{
  LOG_FLOW("entering set_route_entry\n");
  re->destination.s_addr = destination.s_addr;
  if(metric >=  0) 
    re->metric = metric;
  re->nexthop.s_addr = nexthop.s_addr;
  LOG_FLOW("exiting set_route_entry\n");
} /* set_route_entry */

void set_rip_entry(struct rip_entry *rte, short addrfamily, struct in_addr destination, uint32_t metric)
{
  LOG_FLOW("entering set_rip_entry\n");
  rte->addrfamily = addrfamily;
  rte->_zero = 0;
  rte->destination.s_addr = destination.s_addr;
  rte->_zero1 = 0;
  rte->_zero2 = 0;
  if(metric >= 0)
    rte->metric = metric;
  LOG_FLOW("exiting set_rip_entry\n");

} /* set_rip_entry */

int sendmesg( unsigned short int addrfamily, int port,  struct in_addr send_to_ip, int sock,char *message, int len)
{
  struct sockaddr_in send_to;
  int num_bytes = 0;
  send_to.sin_family = addrfamily;
  send_to.sin_port = htons(port);
  send_to.sin_addr.s_addr = send_to_ip.s_addr;
  memset(send_to.sin_zero, '\0', sizeof(send_to.sin_zero));
  if((num_bytes = sendto(sock, message, len, 0, (struct sockaddr *)&send_to, sizeof(struct sockaddr))) == -1) {
    WARN("\nsend socket error \n");perror("");
    return -1;
  }
  return num_bytes;
} /* sendmesg */

void print_route_table(int flag)
{
  listnode *l = NULL;
  struct route_entry *re = NULL;
  FILE *temp;
  LOG_FLOW("entering print_route_table\n");
  if(flag)
    temp = logfile;
  else
    temp = stdout;
  l = (listnode *)route_table;
  fprintf(temp,"\nROUTE TABLE------>");
  while((l = list_next(l)) != NULL) {
    re = (struct route_entry *)l;
    fprintf(temp,"\ndestination: %s  metric: %d ", inet_ntoa(re->destination),re->metric);
    fprintf(temp,"nexthop: %s\n", inet_ntoa(re->nexthop));
    fflush(temp);
     // fclose(logfile);
      /*if((logfile = fopen(fname, "a")) == NULL) {
	printf("\nError in opening file:%s\n",fname);
	return;
	}*/
  }
  LOG_FLOW("exiting print_route_table\n");
} /* print_route_table */

/* init routine- initialise neighbour list,send request msg, send msg with self_entry */
int init(char *fname)
{
  FILE *ifile;
  int num_bytes_sent;
  struct neighbour *new;
  listnode *curr;
  struct rip_header rh;
  struct rip_entry rte, self_entry;
  char message[1024] = {'\0'};
  char *tempmsg;
  struct route_entry * re;  
  struct in_addr ip, dummy;
  
  LOG_FLOW("\nentering init\n");
 
  if((ifile = fopen(fname, "r+")) == NULL) {
    printf("\nError in opening file:%s\n",fname);
    return -1;
  }
  tempmsg = message;
  while(read_neighbour_ip(ifile, &ip) != -1) {
    tempmsg = message;
    memset(tempmsg, '\0', 1024);
    new = alloc_neighbour();
    new->addr.s_addr = ip.s_addr;
    
    curr = (listnode *)new;
    
    if(!list_exist((listnode *)neighbours)) {
      neighbours = alloc_neighbour();
    }
    list_append((listnode *)neighbours, curr);
 
   /* construct request message and send to neighbours */
    rh.command = REQUEST;
    rh.version = IPVER;
    rh._zero = 0;
 
    tempmsg = add_rip_header_to_packet(tempmsg, rh);
    set_rip_entry(&rte, 0, dummy, INFINITY);
    rte.destination.s_addr = 0;
    
    tempmsg = add_rip_entry_to_packet(tempmsg, rte);

    if((num_bytes_sent = sendmesg(AF_INET, RIP_PORT, ip, sock, message, 24)) == -1) {
      perror("");
      return -1;
      }
    
    /* send entry of own ip to neighbours */
    memset(message, '\0', 1024);
    tempmsg = message;
    rh.command = RESPONSE;
    rh.version = IPVER;
    rh._zero = 0;
    tempmsg = add_rip_header_to_packet(tempmsg, rh);
    set_rip_entry(&self_entry, AF_INET, ownip, 0);
    tempmsg = add_rip_entry_to_packet(tempmsg, self_entry);
    
    if((num_bytes_sent = sendmesg(AF_INET, RIP_PORT, ip, sock, message, 24)) == -1) {
      perror("");
      return -1;
    }
   
    re = alloc_route_entry();
    set_route_entry(re, ip, 1, ip);
    
    add_route_entry(re);
    re->timeout_tmr = start_timer(35000ULL, timeout_callback, re, NULL, '0');	
    
  }

  print_route_table(1);
  LOG_FLOW("\nexiting init_neighbour\n");
  return 0;
} /* init */

int is_neighbour(struct in_addr s)
{
  struct neighbour *n;
  listnode * p = (listnode*)neighbours;
  while((p = list_next(p)) != NULL) {
    n = (struct neighbour*)p;
    if(n->addr.s_addr == s.s_addr)
      return 1;
  }
  return 0;
} /* is_neighbour */

int validate_message(struct sockaddr_in from)
{
  /* validate port */
  if(ntohs(from.sin_port) != RIP_PORT) {
    WARN("\nValidate_message() failed:Message not from RIP_PORT:%d\n",ntohs(from.sin_port));
    return -1;
  }
  /* validate that message is from neighbour */
  if(!is_neighbour(from.sin_addr)) {
    WARN("\nValidate_message() failed:Message not from neighbour\n");
    return -1;
  }
  return 0;
} /* validate_message */

int validate_rip_entry(struct rip_entry e)
{
  LOG_FLOW("\nentering validate_rip_entry\n");
  if(e.metric < 0 || e.metric > 16) {
    LOG_FLOW("\exiting validate_rip_entry with return val= -1\n");
    return -1;
  }
  LOG_FLOW("\nexiting validate_rip_entry with return val= 0\n");
  return 0;
} /* validate_rip_entry */

void input()
{
  int msgsize;
  struct sockaddr_in from;
  socklen_t fromlen;
  char message[1024];
  byte pkt_type;
  fromlen = sizeof(from);
  LOG_FLOW("\nentering input\n");

  /*receive message  */
  msgsize = recvfrom(sock, message, 1024, 0, (struct sockaddr *)&from, &fromlen);
  if(validate_message(from) != 0) {
    WARN("\ninvalid message received in input:dropped\n");
    return;
  }
 
 /* check command */
  pkt_type = packet_type(message);
   
  if(pkt_type == RESPONSE) {
    decode_response(from.sin_addr, message,fromlen);
  }
  else if(pkt_type == REQUEST) {
    decode_request(from.sin_addr, message, fromlen);
  }
  
  LOG_FLOW("\nexiting input\n");
  return;
} /* input */

int min(unsigned int x, unsigned int y)
{
  return (x<y)?x:y;
}


/* checks if route to destination exists.
if yes, returns pointer to that route_entry
if no, returns NULL */
struct route_entry * is_route_available(struct in_addr destination)
{
  struct route_entry *re;
  listnode *l;
  LOG_FLOW("\nentering is_route_available\n");
  l = (listnode *)route_table;
  if(is_list_empty((listnode*)route_table) || !list_exist((listnode*)route_table)) {
    LOG_FLOW("\nexiting is_route_available with return val= NULL\n");
    return NULL;
  }
  while(( l = list_next(l)) != NULL) {
    re = (struct route_entry *)l;
    if((re->destination.s_addr == destination.s_addr)) {
      LOG_FLOW("\nexiting is_route_available with re:: destination=%s; metric=%d; nexthop=%s \n",inet_ntoa(re->destination), re->metric, inet_ntoa(re->nexthop));
      return re;
    }
  }
  LOG_FLOW("\nexiting is_route_available with return val= NULL\n");
  return NULL;
 
} /* is_route_available */

/* timeout timer callback function */
void timeout_callback(void *r, void *dummy)
{
  LOG_FLOW("\nentering timeout_callback\n");
  struct route_entry *re = (struct route_entry *)r;
  re->metric = INFINITY;
  fprintf(logfile,"\nRoute timed out:marked invalid::dest:%s ; metric:%d ; ",inet_ntoa(re->destination), re->metric);
  fprintf(logfile,"nexthop:%s ;\n ",inet_ntoa(re->nexthop));
  fflush(logfile);
  re->garbage_tmr = start_timer(GARBAGE, garbage_callback, r, NULL, '0');
  LOG_FLOW("\nexiting timeout_callback\n");
 
} /* timeout_callback */

int delete_route(struct route_entry *re)
{
  LOG_FLOW("\nentering delete_route\n");
  list_delete((listnode *)route_table, (listnode *)re);
  fprintf(logfile,"\nRoute deleted::dest:%s ; metric:%d ; ",inet_ntoa(re->destination), re->metric);
  fprintf(logfile,"nexthop:%s ;\n ",inet_ntoa(re->nexthop));
  fflush(logfile);
  free(re);
  LOG_FLOW("\nexiting delete_route\n");
  return 0;
} /* delete_route */

void garbage_callback(void *r, void *dummy)
{
  LOG_FLOW("\nentering garbage_callback\n");
  struct route_entry *re = (struct route_entry *)r;
  delete_route(re);
  LOG_FLOW("\nexiting garbage_callback\n");

} /* garbage_callback */

void add_route_entry(struct route_entry *re)
{
  listnode * l;
  LOG_FLOW("\nEntering add_route_entry\n");
  if(!list_exist((listnode *)route_table)) {
    route_table = alloc_route_entry();
    l = (listnode *)route_table;
    l->next = NULL;
  }
  list_append((listnode *)route_table, (listnode *)re);
  LOG_FLOW("\nExiting add_route_entry\n");
} /* add_route_entry */

int decode_request(struct in_addr from, char *data, int len)
{
  struct rip_header rh;
  struct rip_entry rte[25];
  int entry_count;
  char message[1024];
  LOG_FLOW("\nentering decode_request\n");
  entry_count = decode_rip_message(data, &rh, rte);
  /* request message requesting entire routing table */
  if(((rte[0].addrfamily == 0) && (rte[0].metric == INFINITY))||1) {
    if(construct_regular_update(from, message) == 0) { /* no errors,update message constructed */
      sendmesg(AF_INET, RIP_PORT, from, sock, message,sizeof(message));
   
    }
  }
  LOG_FLOW("\nexiting decode request\n");
  return 0;
} /* decode_request */


/* decodes the rip message and populates the rip_entry array with entries from the message.
 returns the count of rip_entries in the message */

int decode_rip_message(char *message, struct rip_header *rh, struct rip_entry *rte )
{
  LOG_FLOW("\n entering dec_rip_msg,msglen = %d",sizeof(message));
  *rh = *(struct rip_header *)message;
  print_rip_header(rh);
 
  message += sizeof(struct rip_header);
  int entry_count = 0;
  if(rh->command == REQUEST) {
    rte[0] = *(struct rip_entry *)message;
    print_rip_entry(&rte[0]);
    return 1;
  }
  while(*message != '\0') {
    rte[entry_count++] = *(struct rip_entry *)message;
    message += sizeof(struct rip_entry);
  }
  LOG_FLOW("\nexiting dec_rip_msg\n");
  return entry_count;
} /* decode_rip_message */

int reset_neighbour_entry_timer(struct in_addr from)
{
  listnode *l;
  struct route_entry *re;
  LOG_FLOW("Entering reset_neighbour_entry_timer\n");
  if(is_neighbour(from)) {
    if(route_table == NULL) {
      return -1;
    }
    l = (listnode*)route_table;
    while((l = list_next(l)) != NULL) {
      re = (struct route_entry *)l;
      if((re->destination.s_addr == from.s_addr) && (re->nexthop.s_addr == from.s_addr)) {
	re->timeout_tmr = reset_timer(re->timeout_tmr);
      }
    }
    
  }
  LOG_FLOW("\nExiting reset_neighbour_entry_timer\n");
  return 0;
} /* reset_neighbour_entry_timer */

/* input processing-decode response and update route table*/
int decode_response(struct in_addr from, char *data, int len)
{
  struct rip_header rh;
  struct rip_entry rte[25], e;
  struct route_entry *re;
  char *tempdata;
  int entry_count , i = 0;
  tempdata = data;
  LOG_FLOW("\nentering decode response\n");
  LOG("\nin decode_response:from:%s\n",inet_ntoa(from));
  reset_neighbour_entry_timer(from);
  entry_count = decode_rip_message(data, &rh, rte);
  
  while(i < entry_count) {
    e = rte[i++];
    print_rip_entry(&e);
    if(validate_rip_entry(e) == 0) {
      print_rip_entry(&e);
      e.metric = min(e.metric + 1, INFINITY);
      
      /* if route is not available and metric of new route is < INFINITY
	 then add this route. else if route exists, update route */
      re = is_route_available(e.destination);
      
      /* route not available..make new entry */

      if(re == NULL && e.metric <= INFINITY) {
	if(e.destination.s_addr == ownip.s_addr)
	  continue;
	re = alloc_route_entry();
	/* construct the route entry */
	set_route_entry(re, e.destination, e.metric, from);
	re->timeout_tmr = start_timer(TIMEOUT, timeout_callback, re, NULL, '0');	
	re->routechanged = SET_ROUTE_CHANGE_FLAG;
	
	add_route_entry(re);
      }	

      /* route exists */
      else {
	  
	if(e.metric > INFINITY) {
	  delete_route(re);
	  continue;
	}

	/* if message from same router as existing route */
	if(from.s_addr == re->nexthop.s_addr) {
	  /* reinitialize timeout timer */
	  re->timeout_tmr = reset_timer(re->timeout_tmr);
	  
	  /* if metric is different, assign it to rte */
	  if(re->metric != e.metric) {
	    re->routechanged = SET_ROUTE_CHANGE_FLAG;
	    re->metric = e.metric;
	  }
	}

	/* if from a diff router, but new metric is less than existing metric */
	else if(e.metric < re->metric) {
	  re->metric = e.metric;
	  re->nexthop.s_addr = from.s_addr;
	  re->routechanged = SET_ROUTE_CHANGE_FLAG;
	  re->timeout_tmr=reset_timer(re->timeout_tmr);
	}
      }
    }
  }
  LOG_FLOW("\nexiting decode response\n");
  return 0;
} /* decode_response */

char* add_rip_header_to_packet(char *message, struct rip_header rh)
{
  memcpy(message, (void *)&rh, sizeof(rh));
  message += sizeof(rh);
  return message;
}


char* add_rip_entry_to_packet(char *message, struct rip_entry rte)
{
  memcpy(message, (void *)&rte, sizeof(rte));
  message += sizeof(rte);
  return message;
}

/* 
pack message with appropriate rip entries
returns 0 : no error, update message constructed
returns -1 : no update message constructed */
int construct_regular_update(struct in_addr send_to, char *message)
{
  struct rip_header rh;
  struct rip_entry rte;
  struct route_entry *re;
  listnode * l;
  LOG_FLOW("\n entering construct_regular_update\n");
  rh.command = RESPONSE;
  rh.version = IPVER;
  memset(message, '\0', 1024);
  message = add_rip_header_to_packet(message, rh);

  l = (listnode *)route_table;
  if(!list_exist(l) || is_list_empty(l)) {
    return -1; 			/* no entries in routing table */
  }
  /* scan through routing table */
  while((l = list_next(l)) != NULL) {
    re = (struct route_entry *)l;
    /* split horizon: do not send entries that are learned from a router,
       to that router again */
    if(re->nexthop.s_addr != send_to.s_addr) {
      set_rip_entry(&rte, AF_INET, re->destination, re->metric);
      print_rip_entry(&rte);
      message = add_rip_entry_to_packet(message, rte);
    }
   
  }
  LOG_FLOW("\nexiting construct_regular_update\n");
  return 0;
} /* construct_regular_update */

/* send update pass update type: TRIGUPDATE/REGUPDATE */
int send_update(int updatetype)
{
  struct neighbour *neibr;
  listnode * l;
  char message[1024];
  LOG_FLOW("\nentering genupdate\n");
  
  l = (listnode *)neighbours;
  if(!list_exist(l) || is_list_empty(l)) {
    return 0; 			/* no entries in neighbour table */
  }

  while((l = list_next(l)) != NULL) {
    neibr = (struct neighbour *)l;

    if(construct_regular_update(neibr->addr, message) == 0) {
      sendmesg(AF_INET, RIP_PORT, neibr->addr, sock, message,sizeof(message));
    }
  }
  LOG_FLOW("\nexiting genupdate\n");
  return 0;
} /* send_update */

/* output processing function */
void output(void * dummy1, void * dummy2) 
{
  LOG_FLOW("\nentering output\n");
  send_update(REGUPDATE);
  LOG_FLOW("\nexiting output\n");
}

/* cli thread executes in this function */
void * cli(void *dummy)
{
  struct route_entry *re;
  char destination[20] = {'\0'};
  char nexthop[20] = {'\0'};
  struct in_addr dest, nhop;
  int metric;
  int option;
  while(1) {
    sleep(2);
    printf("\n****RIP CLI****\n");
    printf("1.add route\n2.show table\n3.quit\n");
    scanf("%d",&option);
    switch(option) {
    case 1:
      re = alloc_route_entry();
      printf("destination ip:");
      scanf("%s", destination);
      inet_aton(destination, &dest);
      printf("metric:");
      scanf("%d",&metric);
      printf("nexthop ip:");
      scanf("%s", nexthop);
      inet_aton(nexthop, &nhop);
      set_route_entry(re, dest, metric, nhop);
      re->timeout_tmr = start_timer(TIMEOUT, timeout_callback, re, NULL, '0');	 
      add_route_entry(re);
      break;
    case 2: print_route_table(0);
      break;
    case 3: exit(0);
      break;
    default:
      printf("\nEnter Correct Option\n");
    }
  }

} /* cli */

#ifdef _NOTMAIN_
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


#endif
