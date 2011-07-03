/* timer.h : This header file includes the signatures of the functions that can be invoked for using the timer utility. */
#define RECURSIVE_TIMER '1'
#define NON_RECURSIVE_TIMER '0'


/* Function Pointer */
typedef void (*fnptr_t)(void *arg1, void *arg2);


/* Function Signatures */
/* 1. Function to initialize the timer utility 
   Parameters : none
   Return Value : 0  = successful
                  -1 = error
*/
int init_timer(void);

/* 2. Function to start the timer 
   Parameters:
   1) duration : Time duration in milisec after which user wants the timer to expire 
   2) handler  : Function pointer to callback funtion
   3) arg1     : Argument 1 to callback function
   4) arg2     : Argument 2 to callback function
   5) recursion : '1' ----> recursive timer
                  '0' ----> non-recursive timer
   Return Value : On success, address of the timer structure(void *)
                  On failure, NULL.
*/
void *start_timer(unsigned long long duration, fnptr_t handler, void *arg1, void *arg2,char recursion);


/* 3. Function to stop timer
   Parameter :
   1) handle : Address of timer structure returned by start_timer 
*/
void stop_timer(void *handle);

/* 4. Function to restart timer
   Parameter :
   1) handle : Address of timer structure returned by start_timer
*/ 
void *reset_timer(void* handle);
