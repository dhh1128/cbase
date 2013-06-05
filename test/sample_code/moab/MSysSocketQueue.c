/* HEADER */

      
/**
 * @file MSysSocketQueue.c
 *
 * Contains: accessor function to the MSocketQueue - thread safely
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* Mutex to gate  access to the Moab socket 'MSocketQueue' */
mmutex_t     MSocketQueueMutex;

/* Moab Socket Queue list */
mdllist_t   *MSocketQueue;


/**
 * Create the Moab Socket queue 
 */

int MSysSocketQueueCreate()
  {
  MSocketQueue = MUDLListCreate();
  return(SUCCESS);
  }

/**
 * Deconstruct the MSocketQueue
 */

int MSysSocketQueueFree()
  {
  MUDLListFree(MSocketQueue);
  return(SUCCESS);
  }

/**
 * initialize the mutex for the MSocketQueue
 *
 */

int MSysSocketQueueMutexInit()
  {
  pthread_mutex_init(&MSocketQueueMutex,NULL);
  return(SUCCESS);
  }


/**
 * Convienence function which adds the given socket, S, to the transaction
 * queue used between the communication and scheduling threads. This function
 * is thread safe, as it uses mutexes to lock the socket queue and
 * increases the transaction counters.
 *
 * @param S (I)
 */

int MSysAddSocketToQueue(

  msocket_t *S)

  {
  MUMutexLock(&MSocketQueueMutex);

  /* place S into queue */

  MDB(7,fSOCK) MLog("INFO:     socket '%d' being enqueued\n",
    S->sd);

  MUDLListAppend(MSocketQueue,S);

  MSched.TransactionCount++;

  MSched.HistMaxTransactionCount = MAX(MSched.TransactionCount,MSched.HistMaxTransactionCount);

  MUMutexUnlock(&MSocketQueueMutex);

  return(SUCCESS);
  }  /* END MSysAddSocketToQueue() */


/**
 * Removes a socket from the queue being shared by the communication
 * and scheduling threads.
 *
 * @see MSysEnqueueSocket() - peer
 * @see MUIAcceptRequests() - parent - process socket returned by this routine
 *
 * @param S (O) The socket removed from the queue.
 *
 * @return FAILURE if the queue is empty or if the S param is NULL.
 */

int MSysDequeueSocket(
  
  msocket_t **S)  /* O */

  {
  if (S == NULL)
    {
    return(FAILURE);
    }

  if (MUDLListSize(MSocketQueue) == 0)
    {
    /* queue is empty */
    
    return(FAILURE);
    }

  MUMutexLock(&MSocketQueueMutex);

  *S = (msocket_t *)MUDLListRemoveFirst(MSocketQueue);

  MSched.TransactionCount--;

  MUMutexUnlock(&MSocketQueueMutex);

  MUGetMS(NULL,(long *)&(*S)->ProcessTime);

  MDB(7,fSOCK) MLog("INFO:     socket %d being serviced after %lu milli-sec wait\n",
    (*S)->sd,
    (*S)->ProcessTime - (*S)->CreateTime);
   
  return(SUCCESS);    
  }  /* END MSysDequeueSocket() */
 

/* END MSysSocketQueue.c */
