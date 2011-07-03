#ifndef __TIMER_H
#define __TIMER_H

#include<time.h>
/* Maximum number of buckets in hash table */
#define MAX 50

#define TIME_GRANULARITY 50
#define DEBUG 1

/* Function Pointer */
typedef void (*fnptr)(void *arg1, void *arg2);

/* Typedefs */
typedef struct timer_info     timer_info_t;
typedef struct timer_header   timer_header_t;
typedef struct timer_info*    timer_info_ptr_t;
typedef struct timer_header*  timer_header_ptr_t;
typedef unsigned long long    timer_time_t;

/* Data structure for storing timer information */

struct timer_info {
  int duration;                        // Duration after which to callback
  fnptr handler;                       // Function Pointer to callback function
  void *arg1;                          // Argument to callback function
  void *arg2;                          // Argument to callback function
  timer_info_ptr_t next;                 // Doubly-linked list
  timer_info_ptr_t prev;
  timer_header_ptr_t up;                 // Pointer to header of the list
  char recursive:1;
};


/* Data structure for storing end time information and maintaining link list of timers*/

struct timer_header {
  unsigned int number_of_timers;       // Number of timers with this end time
  timer_time_t  end_time;        // Time when the timers associated with this header will expire
  timer_header_ptr_t prev;               // Pointer to prev timer header
  timer_header_ptr_t next;               // Pointer to next timer header
  timer_info_ptr_t down;                 // Pointer to list of timer_info structures
  int bucket_number;
};


struct timer_data_struct{
/* Variable to maintain current bucket number */
int current_bucket;

/* Variable to store current time (granularity of 50 ms) */
timer_time_t current_time;

/* Mutexes */
pthread_mutex_t currenttime_mutex;
pthread_mutex_t timer_mutex ;
pthread_cond_t timer_cond;

/* Hash Table */
timer_header_ptr_t hash_table[MAX];         // Array of struct timer_header pointers
}timer_ds;


#endif
