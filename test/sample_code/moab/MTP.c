/* HEADER */

/**
 * @file MTP.c
 *
 * Moab Thread Pool
 */

/* Contains:                                    *
 *                                              *
 * int MTPCreateThreadPool()                    *
 *                                              */
 
#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"

typedef struct mtprequest_t mtprequest_t;
#ifndef __NOMCOMMTHREAD
#include <pthread.h>     /* pthread functions and data structures     */

 
/* threadpool request data structure */

struct mtprequest_t {
  int RequestId;                  /* id of request for debug */
  mthread_handler_t FunctionPtr;  /* moab function to be called */  
  void *FunctionDataPtr;          /* pointer to data for the moab function */
  struct mtprequest_t *Next;      /* pointer to next request, NULL if none. */
};

/* info kept for each thread */

typedef struct mtpthreadinfo_t {
  int        ThreadId;              /* thread id         */
  pthread_t  ThreadHandle;          /* thread handle     */
  pthread_key_t      DBHandleKey;   /* thread-specific database handle thread key*/
  mdb_t              DBHandle;      /* thread-specific database handle */
} mtpthreadinfo_t;

/* thread pool data */

typedef struct mtpthreadpool_t {
  mtpthreadinfo_t ThreadInfo[MMAX_TP_HANDLER_THREADS];
  int NumThreadsCreated;

  /* Requests for a thread from the thread pool are kept in a linked list*/

  mtprequest_t *RequestsHead;     /* head of linked list of requests. */
  mtprequest_t *RequestsTail;     /* pointer to last request.         */
} mtpthreadpool_t;

/* For some reason the compiler does not like having these itmes in the mpthreadpool_t structure */

pthread_mutex_t MTPRequestMutex = PTHREAD_MUTEX_INITIALIZER;   /* thread pool mutex */
pthread_cond_t  MTPPendingRequestSignal = PTHREAD_COND_INITIALIZER; /* global condition variable, each thread in the pool blocks 
                                                                       on this condition waiting for a request  */

/* Global Thread Pool data */

mtpthreadpool_t MTPThreadPool;
mtpthreadpool_t *TP = &MTPThreadPool;


/**
 * This function places a request on the pending request linked list
 * for a thread to pick up and handle.
 *
 * @param RequestId (I)  Optional parameter for logging
 * @param FunctionPtr (I) moab function 
 * @param *Data (I)
 */

int MTPAddRequest(

  int RequestId,
  mthread_handler_t FunctionPtr,
  void *Data)

  {
  mtprequest_t *NewRequest;      /* pointer to newly added request.     */

  /* create structure with new request */

  NewRequest = (mtprequest_t *)malloc(sizeof(mtprequest_t));
  if (NewRequest == NULL) 
    { 
    /* TBD - add error handling here */
    }

  NewRequest->RequestId = RequestId;
  NewRequest->Next = NULL;
  NewRequest->FunctionPtr = FunctionPtr;
  NewRequest->FunctionDataPtr = Data;

  /* lock the mutex, to assure exclusive access to the request linked list */

  pthread_mutex_lock(&MTPRequestMutex);

  /* add new request to the end of the list, updating list pointers as required */

  if (TP->RequestsHead == NULL) 
    { 
    /* special case - list is empty */

    TP->RequestsHead = NewRequest;
    TP->RequestsTail = NewRequest;
    }
  else 
    {
    /* Add new request to the tail of the request list */

    TP->RequestsTail->Next = NewRequest;
    TP->RequestsTail = NewRequest;
    }

  /* unlock mutex */

  pthread_mutex_unlock(&MTPRequestMutex);

  /* signal the condition variable - let the blocked threads know that a new request is pending */

  pthread_cond_signal(&MTPPendingRequestSignal);

  return(SUCCESS);
  } /* END MTPAddRequest() */





/**
 * This routine is called by a thread to get a request from the request queue.
 *
 * Note that the returned request should be freed by the caller of this routine.
 *
 * @param ThreadId (I)   id of the thread handling the function (for debug) 
 */

mtprequest_t *MTPGetRequest(

  int ThreadId)

  {
  mtprequest_t *Request = NULL;      /* pointer to request.                 */
  int rc;

  if (TP->RequestsHead == NULL)
    {
    /* no pending requests so block waiting for a pending request signal and the mutex lock */

    rc = pthread_cond_wait(&MTPPendingRequestSignal,&MTPRequestMutex);

    if (rc != 0)
      {
      pthread_mutex_unlock(&MTPRequestMutex); /* just in case */

      return(NULL);
      }
    }
  else /* pending request - race the other threads to get it */
    {
    /* lock the mutex, to assure exclusive access to the list */

    pthread_mutex_lock(&MTPRequestMutex);
    }

  /* We have the lock, check to see if there is still a pending request 
   * or did another thread beat us to it. */

  Request = TP->RequestsHead;

  if (Request != NULL) 
    {
    /* Take this request off the list */

    TP->RequestsHead = Request->Next;

    if (TP->RequestsHead == NULL) 
      { 
      /* this was the last request on the list */

      TP->RequestsTail = NULL;
      }
    }

  /* unlock mutex */

  pthread_mutex_unlock(&MTPRequestMutex);

  return(Request);
  } /* END MTPGetRequest() */





/**
 * Processing loop for each thread in the thread pool
 *
 * Attempt to get a pending request.
 * If the thread gets a request, call the request handler routine and
 * then free the request.
 *
 * @param Data (I) id of the thread (for debug) 
 */

void *MTPHandleRequestsLoop(

  void *Data)

  {
  struct mtprequest_t *Request;/* pointer to a request.               */
  mtpthreadinfo_t *TI = (mtpthreadinfo_t *)Data;
  int ThreadId = TI->ThreadId;

  MSysInitDB(&TI->DBHandle);

  /* do forever.... */

  while (1) 
    {
    /* The MTPGetRequest() routine will block on the request queue mutex
     * lock, or on a pthread_cond_wait() if no requests currently in the
     * request queue */

    Request = MTPGetRequest(ThreadId); 

    if (Request != NULL) 
      { 
      /* got a request - handle it and free it */

      if (Request->FunctionPtr != NULL)
        Request->FunctionPtr(Request->FunctionDataPtr);

      free(Request);
      }
    }   /* END while(1) */

  return(NULL);
  } /* END MTPHandleRequestsLoop() */





/**
 * Create Thread Pool
 *
 * @param NumThreads (I)
 */

int MTPCreateThreadPool(

  int NumThreads)     /* I */

  {
  int tindex;
  mtpthreadinfo_t *TI;
  pthread_attr_t attr;

  /* Initialize thread pool information */

  TP->RequestsHead = NULL;
  TP->RequestsTail = NULL;
  TP->NumThreadsCreated = 0;

  /* create the threads in the thread pool */

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

  for (tindex = 0;tindex < NumThreads;tindex++) 
    {
    int rc;
    TI = &TP->ThreadInfo[tindex];
    TI->ThreadId = tindex;

    /* create each thread */

    rc = pthread_create(&TI->ThreadHandle,&attr,MTPHandleRequestsLoop,TI);

    if (rc != 0)
      {
      MLog("ERROR:    pthread_create number %d returned non-zero exit code %d\n",
        tindex+1,
        rc);

      return(FAILURE);
      }    

    TP->NumThreadsCreated++;
    }

  pthread_attr_destroy(&attr);

  return(SUCCESS);
  }  /* END MTPCreateThreadPool() */





/**
 * Destroy Thread Pool
 *
 */

void MTPDestroyThreadPool()

  {
  mtprequest_t *Request;
  int tindex;

  /* cancel the threads */

  for (tindex = 0;tindex < TP->NumThreadsCreated;tindex++) 
    {
    pthread_cancel(TP->ThreadInfo[tindex].ThreadHandle);
    }

  while (TP->RequestsHead != NULL)
    {
    Request = TP->RequestsHead;

    TP->RequestsHead = Request->Next;

    free(Request);
    }

  return;
  } /* END MTPDestroyThreadPool() */




#else /* ifndef __NOMCOMMTHREAD */
int MTPAddRequest(

  int                RequestId,
  mthread_handler_t  FunctionPtr,
  void              *Data)

  {
  return(FAILURE);
  }




mtprequest_t * MTPGetRequest(

  int  ThreadId)

  {
  return(FAILURE);
  }




void * MTPHandleRequestsLoop(

  void *Data)

  {
  return(FAILURE);
  }




int MTPCreateThreadPool(

  int  NumThreads)

  {
  return(FAILURE);
  }




void MTPDestroyThreadPool(

)

  {
  }

#endif /*ifndef __NOMCOMMTHREAD */

/* END MTP.c */


