#define _MULTI_THREADED
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <qp0ztrc.h>
#define checkResults(string, val) {             \
 if (val) {                                     \
   printf("Failed with %d at %s", val, string); \
   exit(1);                                     \
 }                                              \
}

typedef struct {
   int          threadSpecific1;
   int          threadSpecific2;
} threadSpecific_data_t;

#define                 NUMTHREADS   2
pthread_key_t           threadSpecificKey;

void foo(void);
void bar(void);
void dataDestructor(void *);

void *theThread(void *parm) {
   int                     rc;
   threadSpecific_data_t  *gData;
   PTHREAD_TRACE_NP({
                    Qp0zUprintf("Thread Entered\n");
                   Qp0zDump("Global Data", parm, sizeof(threadSpecific_data_t));},
                   PTHREAD_TRACE_INFO_NP);
   gData = (threadSpecific_data_t *)parm;
   rc = pthread_setspecific(threadSpecificKey, gData);
   checkResults("pthread_setspecific()\n", rc);
   foo();
   return NULL;
}

void foo() {
   threadSpecific_data_t *gData =
      (threadSpecific_data_t *)pthread_getspecific(threadSpecificKey);
   PTHREAD_TRACE_NP(Qp0zUprintf("foo(), threadSpecific data=%d %d\n",
                                gData->threadSpecific1, gData->threadSpecific2);,
                   PTHREAD_TRACE_INFO_NP);
   bar();
   PTHREAD_TRACE_NP(Qp0zUprintf("foo(): This is an error tracepoint\n");,
                   PTHREAD_TRACE_ERROR_NP);
}

void bar() {
   threadSpecific_data_t *gData =
      (threadSpecific_data_t *)pthread_getspecific(threadSpecificKey);
   PTHREAD_TRACE_NP(Qp0zUprintf("bar(), threadSpecific data=%d %d\n",
                                gData->threadSpecific1, gData->threadSpecific2);,
                   PTHREAD_TRACE_INFO_NP);
   PTHREAD_TRACE_NP(Qp0zUprintf("bar(): This is an error tracepoint\n");
                   Qp0zDumpStack("This thread's stack at time of error in bar()");,
                   PTHREAD_TRACE_ERROR_NP);
   return;
}

void dataDestructor(void *data) {
   PTHREAD_TRACE_NP(Qp0zUprintf("dataDestructor: Free data\n");,
                   PTHREAD_TRACE_INFO_NP);
   pthread_setspecific(threadSpecificKey, NULL);   free(data);
   /* If doing verbose tracing we will even write a message to the job log */
   PTHREAD_TRACE_NP(Qp0zLprintf("Free'd the thread specific data\n");,
                   PTHREAD_TRACE_VERBOSE_NP);
}

/* Call this testcase with an optional parameter 'PTHREAD_TRACING' */
/* If the PTHREAD_TRACING parameter is specified, then the         */
/* Pthread tracing environment variable will be set, and the       */
/* pthread tracing will be re initialized from its previous value. */
/* NOTE: We set the trace level to informational, tracepoints cut  */
/*       using PTHREAD_TRACE_NP at a VERBOSE level will NOT show up*/
int main(int argc, char **argv) {
   pthread_t              thread[NUMTHREADS];
   int                    rc=0;
   int                    i;
   threadSpecific_data_t *gData;
   char                   buffer[50];

   PTHREAD_TRACE_NP(Qp0zUprintf("Enter Testcase - %s\n", argv[0]);,
                   PTHREAD_TRACE_INFO_NP);
   if (argc == 2 && !strcmp("PTHREAD_TRACING", argv[1])) {
      /* Turn on internal pthread function tracing support          */
      /* Or, use ADDENVVAR, CHGENVVAR CL commands to set this envvar*/
      sprintf(buffer, "QIBM_PTHREAD_TRACE_LEVEL=%d", PTHREAD_TRACE_INFO_NP);
      putenv(buffer);
      /* Refresh the Pthreads internal tracing with the environment */
      /* variables value.                                           */
      pthread_trace_init_np();
   }
   else {
      /* Trace only our application, not the Pthread code           */
      Qp0wTraceLevel = PTHREAD_TRACE_INFO_NP;
   }

   rc = pthread_key_create(&threadSpecificKey, dataDestructor);
   checkResults("pthread_key_create()\n", rc);

   for (i=0; i <NUMTHREADS; ++i) {
      /* Create per-thread threadSpecific data and pass it to the thread */
      gData = (threadSpecific_data_t *)malloc(sizeof(threadSpecific_data_t));
      gData->threadSpecific1 = i;
      gData->threadSpecific2 = (i+1)*2;
      rc = pthread_create( &thread[i], NULL, theThread, gData);
      checkResults("pthread_create()\n", rc);
      PTHREAD_TRACE_NP(Qp0zUprintf("Wait for the thread to complete, "
                                   "and release their resources\n");,
                      PTHREAD_TRACE_INFO_NP);
      rc = pthread_join(thread[i], NULL);
      checkResults("pthread_join()\n", rc);
   }

   pthread_key_delete(threadSpecificKey);
   PTHREAD_TRACE_NP(Qp0zUprintf("Main completed\n");,
                   PTHREAD_TRACE_INFO_NP);
   return 0;
}
