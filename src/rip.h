#ifndef __ISINCLUDED__
#define __ISINCLUDED__
#define TRIGUPDATE 11
#define REGUPDATE 22
#define RIP_PORT 3333
#define REQUEST 1
#define RESPONSE 2
#define INFINITY 16
#define SET_ROUTE_CHANGE_FLAG 1
#define RESET_ROUTE_CHANGE_FLAG 0
#define VALID 1
#define INVALID 0
#define IPVER 4
#define UPDTIME 5000ULL
#define TIMEOUT 10000ULL
#define GARBAGE 20000ULL
#include "list.h"

typedef char byte;

/* format for routing table entry */
struct route_entry {
  listnode *lnode;		/*linked list pointer */
  struct in_addr destination;
  uint32_t metric;
  struct in_addr nexthop;       /*next router along the path to destination */
  byte routechanged;		/*route change flag */
  void *timeout_tmr;
  void *garbage_tmr;
};

/*RIP MESSAGE FORMAT */

/* RIP packet
|-----------------------------------------------|
|command (1) |version(1)|    zero(2)            |
|-----------------------------------------------|
|                RIP entry(20)                  |
~                                               ~
|-----------------------------------------------|
 */


/*
RIP entry(RTE) format 
|-------------------------------------------------
|addr family id(2)     | must be zero(2)         |
|------------------------------------------------|
|               IPv4 addr (4)                    |
|------------------------------------------------|
|                must be zero(4)                 |
|------------------------------------------------|
|                must be zero(4)                 |
|------------------------------------------------|
|               metric(4)                        |
|------------------------------------------------|

 */

/*header*/
struct rip_header {
  byte command; 		/* 1-REQUEST, 2-RESPONSE */
  byte version;
  short int _zero;
};


struct rip_entry {
  short int addrfamily;
  short int _zero;
  struct in_addr destination;
  uint32_t _zero1;
  uint32_t _zero2;
  uint32_t metric;
};

/*neighbour list - stores ip addr of this node's neighbours */
struct neighbour {
  listnode *lnode;	       	/* linked list pointer */
  struct in_addr addr;
};

struct neighbour *neighbours;	/* head of neighbour list */
struct route_entry *route_table; /* route table head */
int sock;			 /* socket id for communication over RIP_PORT */
struct in_addr ownip;
FILE *logfile;
char fname[20];

byte packet_type(char *message);
void add_route_entry(struct route_entry *re);
void set_route_entry(struct route_entry *re, struct in_addr destination, uint32_t metric, struct in_addr nexthop);

char* add_rip_entry_to_packet(char *message, struct rip_entry rte);
char* add_rip_header_to_packet(char *message, struct rip_header rh);
void set_rip_entry(struct rip_entry *rte, short addrfamily, struct in_addr destination, uint32_t metric);

int validate_rip_entry(struct rip_entry e);
int validate_message(struct sockaddr_in from);
struct neighbour * alloc_neighbour();
struct route_entry * alloc_route_entry();

void print_rip_header(struct rip_header *rh);
void print_rip_entry(struct rip_entry *rte);

struct route_entry * is_route_available(struct in_addr destination);
int is_neighbour(struct in_addr s);

int send_update(int updatetype);
int construct_regular_update(struct in_addr sendto, char *message);

int decode_response(struct in_addr from, char *data, int len);
int decode_request(struct in_addr from, char *data, int len);
int decode_rip_message(char *message, struct rip_header *rh, struct rip_entry *rte );

int init();
void input();
void output(void *, void *);

void timeout_callback(void *r, void *dummy);
void garbage_callback(void *r, void *dummy);
int reset_neighbour_entry_timer(struct in_addr from);

int sendmesg( unsigned short int addrfamily, int port,  struct in_addr send_to_ip, int sock,char *message, int len);

void print_route_table(int flag);
int delete_route(struct route_entry *re);
void* cli(void *);
#endif
