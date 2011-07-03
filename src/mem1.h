
/* Function to delete a timer structure */
void free_mem(void*);

/* Function to malloc new pages and to initialise them in the form of linked list of free memory blocks
   Return Value : 0 = successful
               -1 = error
*/
int init_mem(int size);


/* Function to obtain free block of memory
   Return Value : Successful = address of the allocated memory
                  Error      = NULL
*/
void *get_memory(void);
