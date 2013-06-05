/* HEADER */

      
/**
 * @file MSysJobSubmitQueue.c
 *
 * Contains: MJobSumitQueue accesses functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/* Mutex to gate access to the MJobSumitQueue */
mmutex_t    MJobSubmitQueueMutex;

/* Moab JobSubmitQueue */
mdllist_t  *MJobSubmitQueue;


/**
 * Create the MJobSubmitQueue list head
 */

int MSysJobSubmitQueueCreate()
  {
  MJobSubmitQueue = MUDLListCreate();
  return(SUCCESS);
  }

/**
 * Deconstruct the MJobSubmitQueue 
 */

int MSysJobSubmitQueueFree()
  {
  MUDLListFree(MJobSubmitQueue);
  return(SUCCESS);
  }

/**
 * Initialize the mutext for the Job Submit Queue
 */

int MSysJobSubmitQueueMutexInit()
  {
  pthread_mutex_init(&MJobSubmitQueueMutex,NULL);
  return(SUCCESS);
  }


/**
 * Insert a job into the submit job queue, retreive back a unique ID.
 *
 * @param JE
 * @param ID
 * @param PopulateID - if TRUE then this routine will create a new ID and populate it to ID
 *                     if FALSE then this routine will leave ID alone and won't create a new ID
 */

int MSysAddJobSubmitToQueue(

  mjob_submit_t *JE,
  int           *ID,
  mbool_t        PopulateID)

  {
  MDB(7,fSCHED)
    {
    mstring_t JString(MMAX_LINE);

    MXMLToMString(JE->JE,&JString,NULL,TRUE);

    MLog("INFO:     Thread: %d, Enqueuing job with XML '%s'\n",MUGetThreadID(),JString.c_str());
    }

  MUMutexLock(&MJobSubmitQueueMutex);

  /* place S into queue */

  MDB(7,fSOCK) MLog("INFO:     Thead: %d, job being enqueued\n",MUGetThreadID());

  MUDLListAppend(MJobSubmitQueue,JE);

  if (PopulateID == TRUE)
    {
    *ID = MRMJobCounterIncrement(MSched.InternalRM);

    JE->ID = *ID;
    }

  MSched.TransactionCount++;

  MSched.HistMaxTransactionCount = MAX(MSched.TransactionCount,MSched.HistMaxTransactionCount);

  MUMutexUnlock(&MJobSubmitQueueMutex);

  MDB(7,fSOCK) MLog("INFO:     Thead: %d, job %d enqueued\n",MUGetThreadID(),*ID);

  return(SUCCESS);
  }  /* END MSysAddJobSubmitToQueue() */





/**
 * Removes a job from the queue being shared by the communication
 * and scheduling threads.
 *
 * @see MSysAddJobSubmitToQueue() - peer
 * @see MUIJobSubmitThreadSafe() - peer
 * @see MS3ProcessSubmitQueue() - parent
 * @see MJobSubmitAllocate() - peer
 *
 * @return FAILURE if the queue is empty or if the S param is NULL.
 */

int MSysDequeueJobSubmit(

  mjob_submit_t **JS)

  {
  if (JS == NULL)
    {
    return(FAILURE);
    }

  if (MUDLListSize(MJobSubmitQueue) == 0)
    {
    /* queue is empty */
    
    return(FAILURE);
    }

  MUMutexLock(&MJobSubmitQueueMutex);

  *JS = (mjob_submit_t *)MUDLListRemoveFirst(MJobSubmitQueue);

  MSched.TransactionCount--;

  MUMutexUnlock(&MJobSubmitQueueMutex);

  return(SUCCESS);
  }  /* END MSysDequeueJobSubmit() */

/* END MSysJobSubmitQueue.c */
