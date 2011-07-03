/* Logging :: */

/* Default logging level is 0 == No logs. */
/* Log Level = 1 Only WARNs will be displayed in this level.  */
/* Log level = 2 . WARNs and LOG_FLOWs will be showed in this level. */
/* Log level = 3 . All logs will be displayed. */

/* Logging level can be set for individual .c files. */

/*   eg: to set log level 2 for file foo.c */

/* #define LOG_LEVEL 2 */
/* #include "log.h" */
/* in foo.c */

/* and so on. */

/* If log level is not defined in .c file, default level (0) will be used. */

/* WARNs will be displayed on stdout. All other logs will be displayed on the stderr. */

/* Calling convention :: */

/* To log , follow the following convention of printf()*/
  
/*   WARN("warning message -- i == %d", i); */
/*   LOG_FLOW("found bar = %s", bar); */
/*   LOG("msg"); */
/* Use "\n" at end to make sure buffer is flushed */
 

#ifndef LOG_LEVEL    //Check if log level is set explicitly   
#define LOG_LEVEL 0  // Set default logging level
#endif

#define WARN(expr, args...) if (LOG_LEVEL >= 1) fprintf(stdout, expr, ##args)
#define LOG_FLOW(expr, args...) if (LOG_LEVEL >= 2) fprintf (stderr, expr, ##args)
#define LOG(expr, args...) if(LOG_LEVEL >= 3) fprintf(stderr, expr, ##args)
