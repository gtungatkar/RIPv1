#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<pthread.h>
#include<errno.h>
#include<sys/timeb.h>
#include<math.h>
#include<time.h>
#include<sys/time.h>
#include "timer2.h"
#include "mem1.h"
#include "log.h"

/* -----------------------------------------------  Function to obtain the current time ----------------------------------------------------- */
void get_current_time()
{
  struct timeval timestruct;
    
  /* Get current time in the struct timeval. It stores time in two members tv_sec = seconds, tv_usec = microseconds */
  gettimeofday(&timestruct,NULL);
  
  /* Convert the time in struct timeval into milliseconds */
  timer_ds.current_time = timestruct.tv_sec*1000ULL + timestruct.tv_usec/1000;

  /* Round up the current time to the time granularity */
  timer_ds.current_time = (timer_time_t)ceill((long double)timer_ds.current_time/(long double)50)*50ULL;
}

/* ----------------------------------------------   Function to allocate and initialise a new timer ------------------------------------------ */
timer_info_ptr_t alloc_new_timer(timer_time_t duration, fnptr handler, void *arg1, void *arg2, char recursion)
{
  timer_info_ptr_t new_timer; 

  /* Allocate memory for the new timer */
  new_timer = (timer_info_ptr_t)get_memory();
  
  if(new_timer == NULL)
    return(NULL);

  /* Initialise the new timer */
  new_timer->duration = duration;
  new_timer->handler = handler;
  new_timer->arg1 = arg1;
  new_timer->arg2 = arg2;
  new_timer->recursive = recursion;
  
  /* Return address of the new timer */
  return(new_timer);

}

/* -----------------------------------------------  Function to find the position where the new timer should be inserted --------------------- */
int get_position(timer_time_t duration, timer_time_t *current_endtime)
{
  /* dval tells how many rounds of hash table are required for timer to expire */
  unsigned long long dval = (unsigned long long)ceill((long double)duration/(long double)TIME_GRANULARITY) - 1;

  /* position is in which bucket should the new timer go into */
  int position = (timer_ds.current_bucket + dval)%MAX;
    
  /* When this timer will expire */
  *current_endtime = timer_ds.current_time + TIME_GRANULARITY * dval;
  
  /* Return the bucket number */
  return(position);
}

/* -----------------------------------------------  Function to find the proper header under which to insert the timer ----------------------- */
timer_header_ptr_t get_header(int position, timer_time_t current_endtime, timer_header_ptr_t prev_header)
{
  timer_header_ptr_t temp, beforetemp;
  timer_header_ptr_t next_header = prev_header;

  beforetemp = temp = timer_ds.hash_table[position];

  while(temp!=NULL){

    /*If there is a match for header with the same endtime */
    if(temp->end_time == current_endtime){
      next_header = temp;
      break;
    }

    /*If a header is encountered, whose endtime is greater than this endtime */
    if(temp->end_time > current_endtime){
      
      if(next_header == NULL)
	next_header = (timer_header_ptr_t)get_memory();

      if(next_header==NULL)
	return(NULL);
      
      next_header->bucket_number = position;
      next_header->next = temp;
      next_header->prev = temp->prev;
      temp->prev = next_header;

      if(beforetemp!=temp)
	beforetemp->next = next_header;
      else
	/* If the header is to be inserted at the head of the list then the hash_table[position] should be updated */
	timer_ds.hash_table[position] = next_header;

      next_header->down = NULL;
      next_header->end_time = current_endtime;
      next_header->number_of_timers = 0;

      break;
    }

    /* beforetemp saves the address of node just before temp */
    beforetemp = temp;
    temp = temp->next;
  }
  
  /* If reached end of the list or if no list present */
  if(temp==NULL) {
    
    if(next_header == NULL)
      next_header = (timer_header_ptr_t)get_memory();

    next_header->bucket_number = position;
    next_header->next = NULL;
    next_header->prev = beforetemp;
    next_header->down = NULL;

    /*If no list present */
    if(beforetemp == NULL)
      timer_ds.hash_table[position] = next_header;
    else
      /*If reached end of list */
      beforetemp->next = next_header;

    next_header->end_time = current_endtime;
    next_header->number_of_timers = 0;
  }
  return(next_header);
}

/* ------------------------------------------------  Function to add the new timer to the link list of the header --------------------------- */
void attach_timer_to_header(timer_info_ptr_t new_timer, timer_header_ptr_t new_header)
{
  new_timer->next = new_header->down;
  
  /* If the new timer is not the first timer under this header */
  if(new_header->down != NULL)
    new_timer->next->prev = new_timer;
  
  new_timer->prev = NULL;
  new_timer->up = new_header;
  
  new_header->down = new_timer;
  new_header->number_of_timers++;
}

timer_info_ptr_t reset_timer(timer_info_ptr_t);

/* ------------------------------------------------  Function for per tick processing -------------------------------------------------------- */
/* This function is called every tick ( = 50ms ), it does the per tick processing
   It checks if any timer has expired and if yes, calls its respective callback function 
*/
void* per_tick_processing(void *arg)
{
  struct timespec timeout;
  struct timeval now;

  timer_time_t old_current_time,temp_time;
  unsigned long no_of_buckets;
  timer_header_ptr_t temp;
  timer_info_ptr_t timer_expire,timer_expire_old;
  
  fnptr handler;

  /* Mutex locked to wait for condition variable (used just to obtain the effect of the sleep)*/
  pthread_mutex_lock(&timer_ds.timer_mutex);

    while(1)
    {      
      /* Obtain current time */
      gettimeofday(&now,NULL);

      /* The thread needs to wait for 50ms */
      timeout.tv_sec = now.tv_sec;
      timeout.tv_nsec = (now.tv_usec +  50*1000ULL) * 1000ULL;

      /* This function is used instead of sleep, to make the thread wait for 50ms and then perform the per_tick_processing */
	pthread_cond_timedwait(&timer_ds.timer_cond,&timer_ds.timer_mutex, &timeout);

      /* Mutex for operating on hash table and current_bucket pointer */
      pthread_mutex_lock(&timer_ds.currenttime_mutex);

      /* save previous current time and get current time */
      old_current_time = timer_ds.current_time;
      get_current_time();

      /* How many buckets to scan depending on previous current time and this current time */
      no_of_buckets = ceill((long double)(timer_ds.current_time - old_current_time)/(long double)50);
      
      while(no_of_buckets > 0){
	temp =  timer_ds.hash_table[timer_ds.current_bucket];
	
	/* If there is a list associated with the current bucket and if the first header has an end time less than the current_time, then process */
	if(temp!=NULL && temp->end_time < timer_ds.current_time){
	  
	  timer_expire = temp->down;
	  timer_ds.hash_table[timer_ds.current_bucket] = temp->next;
	  
	  /* Scan whole list associated with the header to make a callback for that timer */
	  while(timer_expire!=NULL){
	    handler = timer_expire->handler;
	    
	    pthread_mutex_unlock(&timer_ds.currenttime_mutex);
	    /* Calling the callback function using the functin pointer handler */
	    handler(timer_expire->arg1,timer_expire->arg2); 
	    pthread_mutex_lock(&timer_ds.currenttime_mutex);

	    timer_expire_old = timer_expire;
	    timer_expire = timer_expire->next;
	    
	    if(timer_expire_old->recursive == 0){
	      /* Free the memory of the timer, if it is non recursive */
	      free_mem(timer_expire_old);
	      temp->number_of_timers -- ;
	    }
	    else if (timer_expire_old->recursive != 0 ){
	      temp_time = timer_ds.current_time;
	      timer_ds.current_time = old_current_time;
	      pthread_mutex_unlock(&timer_ds.currenttime_mutex);
	      timer_expire_old = reset_timer(timer_expire_old);
	      pthread_mutex_lock(&timer_ds.currenttime_mutex);
	      timer_ds.current_time = temp_time;
	      if(timer_expire_old == NULL)
		printf("\nRecursion Timer error:Memory management failed\n");
	      no_of_buckets = ceill((long double)(timer_ds.current_time - old_current_time)/(long double)50);
	    }
	  }
	  if(temp->number_of_timers == 0)
	    /* Free the header */
	    free_mem(temp);
	}
	
	no_of_buckets--;
       
	timer_ds.current_bucket = (timer_ds.current_bucket+1)%MAX;
      }

      /* Release the lock */
      pthread_mutex_unlock(&timer_ds.currenttime_mutex);      
    }
    
    return(NULL);
}

/* ---------------------------------------- APIs -----------------------------------------------------------*/

/* Function to initialize the timer utility (**V.IMP.** Use this function only once, else all previous timers will be lost and also will cause memory leak)
   Parameters : none
   Return Value : 0  = successful
                  -1 = error
*/
int init_timer(void)
{
  pthread_t tick_proc_thread;
  int count;

  /* Initialise the hash table */
  for(count=0;count<MAX;count++)
    timer_ds.hash_table[count] = NULL;

  /* Initialise present bucket as bucket number 0 */
  timer_ds.current_bucket = 0;

  /* Obtain current time with time rounded up to time granularity */
  get_current_time();

  /* Initialize mutexes and condition variable */
  pthread_mutex_init(&timer_ds.currenttime_mutex, NULL);
  pthread_mutex_init(&timer_ds.timer_mutex, NULL);
  pthread_cond_init(&timer_ds.timer_cond,NULL);


  /* Create a new thread to perform per tick processing */
  if( pthread_create(&tick_proc_thread, NULL, per_tick_processing, NULL)!= 0 ){
    return(-1);
  }
  
  /* Initialise memory and return whether the operation was successful or not */
  return(init_mem(sizeof(timer_info_t)));
}



/* Function to start the timer 
   Parameters:
   1) duration  : Time duration after which user wants the timer to expire (in ms)
   2) handler   : Function pointer to callback funtion
   3) arg1      : Argument 1 to callback function
   4) arg2      : Argument 2 to callback function
   5) recursion : If the timer is RECURSIVE_TIMER or NON_RECURSIVE_TIMER
   Return Value : On success, address of the timer structure(void *)
                  On failure, NULL.
*/
void *start_timer(timer_time_t duration, fnptr handler, void *arg1, void *arg2,char recursion)
{
  timer_time_t current_endtime;
  timer_info_ptr_t new_timer;
  timer_header_ptr_t new_header;

  /* Mutex to make changes to hash table and other data structures related to timer */
  pthread_mutex_lock(&timer_ds.currenttime_mutex);

  /* Calculating which bucket to insert into */
  int position = get_position(duration,&current_endtime);
  
  /* Allocating memory for a new timer and initializing it*/
  new_timer = alloc_new_timer(duration, handler, arg1, arg2, recursion);
  if(new_timer == NULL)
    return(NULL);
 
  /* Scanning the linked list of headers corresponding to the position in hash table */
  new_header =  get_header(position,current_endtime,NULL);
  if(new_header == NULL)
    return(NULL);

  /* Attach the new timer to link list of the header */
  attach_timer_to_header(new_timer,new_header);

  /* Release the lock */
  pthread_mutex_unlock(&timer_ds.currenttime_mutex);

  /*Return the address of the newly created timer */
  return(new_timer);
}


/* Function to stop timer
   Parameters:
   1) handle  : Address of timer structure returned by start_timer 
*/
void stop_timer(timer_info_ptr_t handle)
{
  /* Header under which this timer exists */
  timer_header_ptr_t header = handle->up;
  
  /* Obtain the lock to perform timer operations */
  pthread_mutex_lock(&timer_ds.currenttime_mutex);


  /* Update the link list of the timers */
  if(handle->next!=NULL)
    handle->next->prev = handle->prev;
  if(handle->prev!=NULL)
    handle->prev->next = handle->next;

  /* If the timer to be stopped is the first timer under the header */
  if(header->down == handle)
    header->down = handle->next;
  
  /* Reduce the count of timers in the header */
  header->number_of_timers --;

  /* If the timer is the only timer under this header, free this header too */
  if(header->number_of_timers == 0){
    /* Update the link list of headers */
    if(header->next!=NULL)
      header->next->prev = header->prev;
    if(header->prev!=NULL)
      header->prev->next = header->next;

    /* If this header is the first in the link list of headers, update the hash table entry also */
    if(timer_ds.hash_table[header->bucket_number] == header)
      timer_ds.hash_table[header->bucket_number] = header->next;
    
    /* Free the memory of the header */
    free_mem(header);

  }
  /* Free the memory of the timer */
  free_mem(handle);

  /* Release the lock */
  pthread_mutex_unlock(&timer_ds.currenttime_mutex);
  
}

/* Funtion to reset the timer.
   Parameter : 
   1) handle  : Address of timer structure returned by start_timer 
*/
timer_info_ptr_t reset_timer(timer_info_ptr_t handle)
{
  timer_time_t current_endtime;
  timer_header_ptr_t new_header;
  timer_header_ptr_t curr_header;
  int position;

  /* Mutex to make changes to hash table and other data structures related to timer */
  pthread_mutex_lock(&timer_ds.currenttime_mutex);

  /* Calculating which bucket to insert into */
  position = get_position(handle->duration,&current_endtime);

  /* Remove the timer from present link list */
  if(handle->next != NULL)
    handle->next->prev = handle->prev;
  if(handle->prev != NULL)
    handle->prev->next = handle->next;

  /* Obtain the header of this timer */
  curr_header = handle->up;
  curr_header->number_of_timers--;
  if(curr_header->down == handle)
    curr_header->down = handle->next;
  
  /* If the timer is the only timer under its header */
  if(curr_header->number_of_timers == 0){
   
    /* If the header is the first in the list */
    if(timer_ds.hash_table[curr_header->bucket_number] == curr_header){
      timer_ds.hash_table[curr_header->bucket_number] = curr_header->next;  
    }
    
    if(curr_header->next != NULL)
      curr_header->next->prev = curr_header->prev;
    if(curr_header->prev != NULL)
      curr_header->prev->next = curr_header->next;
   
    new_header = get_header(position, current_endtime, curr_header);
    if(new_header != curr_header)
      free_mem(curr_header);
  }
  else
    /* Scanning the linked list of headers corresponding to the position in hash table */
    new_header =  get_header(position,current_endtime,NULL);
  
  if(new_header == NULL)
    return(NULL);

  /* Attach the new timer to link list of the header */
  attach_timer_to_header(handle,new_header);

  /* Release the lock */
  pthread_mutex_unlock(&timer_ds.currenttime_mutex);

  return(handle);
}
