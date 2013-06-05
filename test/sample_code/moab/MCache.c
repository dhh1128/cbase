/* HEADER */

/**
  *@file MCache.c
  *
  *Contains functions for maintaining memory cache for all jobs & nodes.  This will be used to multi-thread the 
  *client request functions for faster access to data.
  */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"
#include <iostream>
#include "MBSON.h"
#include "MMongo.h"

/* The following 'global' is ONLY referenced in this file and that should be enforced into the
 * future
 */

mcache_t *MCache = NULL;



#ifdef __MCOMMTHREAD

/**
 * Initialize the MCache.
 *
 *
 */

int MCacheInitialize(

  mcache_t **Cache)

  {
  mcache_t *C;

  if (Cache == NULL)
    {
    return(FAILURE);
    }

  /* Allocate the cache memory block and init values */

  C = (mcache_t *)MUCalloc(1,sizeof(mcache_t));

  if (C != NULL)
    {
    /* Create the hash tables */

    MUHTCreate(&C->Jobs,-1);
    MUHTCreate(&C->Nodes,-1);
    MUHTCreate(&C->Rsvs,-1);
    MUHTCreate(&C->Triggers,-1);
    MUHTCreate(&C->VCs,-1);
    MUHTCreate(&C->VMs,-1);

    /* Initialize the reader - writer locks */

    pthread_rwlock_init(&C->NodeLock,NULL);
    pthread_rwlock_init(&C->JobLock,NULL);
    pthread_rwlock_init(&C->RsvLock,NULL);
    pthread_rwlock_init(&C->TriggerLock,NULL);
    pthread_rwlock_init(&C->VCLock,NULL);
    pthread_rwlock_init(&C->VMLock,NULL);

    *Cache = C;

    try
      {
      MMongoInterface::Init(MSched.MongoServer);
      }
    catch (std::exception& e)
      {
      if (MUStrIsEmpty(MSched.MongoServer))
        {
        MDB(2,fSTRUCT) MLog("ERROR:    Failed to initialize connection to Mongo server (Moab is configured to use Mongo, but no MONGOSERVER is specified\n");
        }
      else
        {
        MDB(2,fSTRUCT) MLog("ERROR:    Failed to initialize connection to Mongo server\n");
        }
      }

    return(SUCCESS);
    }

  *Cache = NULL;

  return(FAILURE);
  }  /* END MCacheInitialize() */





/**
 * Shutdown / destructor handler for MCache
 */

void MCacheDestroy(

  mcache_t *Cache)

  {
  if (Cache == NULL)
    {
    return;
    }

  /* Destroy the hash tables */

  MUHTFree(&Cache->Jobs,TRUE,(int (*)(void**))MJobTransitionFree);
  MUHTFree(&Cache->Nodes,TRUE,(int (*)(void**))MNodeTransitionFree);
  MUHTFree(&Cache->Rsvs,TRUE,(int (*)(void**))MRsvTransitionFree);
  MUHTFree(&Cache->Triggers,TRUE,(int (*)(void**))MTrigTransitionFree);
  MUHTFree(&Cache->VCs,TRUE,(int (*)(void**))MVCTransitionFree);
  MUHTFree(&Cache->VMs,TRUE,(int (*)(void**))MVMTransitionFree);

  /* Destroy the read - writer locks */

  pthread_rwlock_destroy(&Cache->NodeLock);
  pthread_rwlock_destroy(&Cache->JobLock);
  pthread_rwlock_destroy(&Cache->RsvLock);
  pthread_rwlock_destroy(&Cache->TriggerLock);
  pthread_rwlock_destroy(&Cache->VCLock);
  pthread_rwlock_destroy(&Cache->VMLock);

  MMongoInterface::Teardown();

  MUFree((char **)&Cache);
  }  /* END MCacheDestroy() */




/*
 * Moab Cache System:  This is the overall system that handles the entire client 
 *                     caching for the hot & cold swap.
 *
 * MCacheSysInialize initializes the entire system for allocating global locks,
 * individual caches, and other system resources
 */

int MCacheSysInitialize()

  {
  return(MCacheInitialize(&MCache));
  }  /* END MCacheSysInitialize() */




/* Shutdown the Cache System */

void MCacheSysShutdown()

  {
  MCacheDestroy(MCache);
  }  /* END MCacheSysShutdown() */



/* write out the names of nodes for debugging */

void MCacheDumpNodes()

  {
  mhashiter_t HTIter;
  mtransnode_t *N;

  if (MCache == NULL)
    {
    return;
    }

  MUHTIterInit(&HTIter);

  pthread_rwlock_rdlock(&MCache->NodeLock);

  while (MUHTIterate(&MCache->Nodes,NULL,(void **)&N,NULL,&HTIter) == SUCCESS)
    {
    MLogNH(",%s",N->Name);
    } /* END while (MUHTIterate) */

  pthread_rwlock_unlock(&MCache->NodeLock);

  return;
  }  /* END MCacheDumpNodes() */



#ifdef MUSEMONGODB
#define DBWRITE TRUE
#else
#define DBWRITE FALSE
#endif

/**
  * Write the given node transistion to hash table 
  *
  * NOTE: This routine is not threadsafe.  It should only be called
  *       by MOTransistionFromQueue() thread.
  *
  * @param N    (I) the node transistion object to write
  */

int MCacheWriteNode(

  mtransnode_t    *N)

  {
  int rc;

  const char *FName = "MCacheWriteNode";

  MDB(7,fTRANS) MLog("%s(%s)\n",
    FName,
    (MUStrIsEmpty(N->Name)) ? "NULL" : N->Name);

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  /* Obtain the write lock and write the object */

  pthread_rwlock_wrlock(&MCache->NodeLock);
  rc = MUHTAdd(&MCache->Nodes,N->Name,(void*)N,NULL,&MNodeTransitionFree);

  if (DBWRITE)
    {
    MMongoInterface::MTransOToMongo(N,mxoNode);
    }

  pthread_rwlock_unlock(&MCache->NodeLock);

  return(rc);
  }  /* END MCacheWriteNode() */








/**
  * Write the given job to the transisition job table.
  *
  * NOTE: This routine is not threadsafe.  It should only be called
  *       by MOTransistionFromQueue() thread.
  *
  * @param J    (I) the job transistion object to write
  */

int MCacheWriteJob(

  mtransjob_t   *J)

  {
  int rc = SUCCESS;

  const char *FName = "MCacheWriteJob";

  MDB(7,fTRANS) MLog("%s(%s)\n",
    FName,
    (MUStrIsEmpty(J->Name)) ? "NULL" : J->Name);

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->JobLock);

  /* NOTE:  we may also want to check J->GridJobID (corresponds to SystemJID
   *        in mjob_t) */

  if (!bmisset(&J->TFlags,mtransfLimitedTransition))
    {
    if ((bmisset(&J->TFlags,mtransfDeleteExisting)) && (!MUStrIsEmpty(J->SourceRMJobID)))
      {
      rc = MUHTRemove(&MCache->Jobs,J->SourceRMJobID,&MJobTransitionFree);
      }

    rc = MUHTAdd(&MCache->Jobs,J->Name,(void*)J,NULL,&MJobTransitionFree);
    }
  else
    {
    mtransjob_t *tmpJ;

    if (MUHTGet(&MCache->Jobs,J->Name,(void **)&tmpJ,NULL) == SUCCESS)
      {
      int rqindex;

      /* swap out minimal attributes 
       * All of these attributes are LIMITED TRANSTION, ensure MJobToTransitionStruct()
       * is in sync with these
       */

      tmpJ->State = J->State;
      tmpJ->ExpectedState = J->ExpectedState;
      tmpJ->SubState = J->SubState;
      tmpJ->StartTime = J->StartTime;
      tmpJ->CompletionTime = J->CompletionTime;
      tmpJ->UsedWalltime = J->UsedWalltime;
      tmpJ->MSUtilized = J->MSUtilized;
      tmpJ->PSUtilized = J->PSUtilized;
      tmpJ->PSDedicated = J->PSDedicated;
      bmcopy(&tmpJ->TFlags,&J->TFlags);
      tmpJ->Priority = J->Priority;
      tmpJ->RunPriority = J->RunPriority;
      tmpJ->EffQueueDuration = J->EffQueueDuration;
      tmpJ->EligibleTime = J->EligibleTime;
      bmcopy(&tmpJ->TFlags,&J->TFlags);
      tmpJ->HoldTime = J->HoldTime;
      tmpJ->SuspendTime = J->SuspendTime;
      tmpJ->MaxProcessorCount = J->MaxProcessorCount;
      tmpJ->CompletionCode = J->CompletionCode;
      tmpJ->MaxProcessorCount = J->MaxProcessorCount;

      tmpJ->RsvStartTime = J->RsvStartTime;
      MUStrDup(&tmpJ->ReservationAccess,J->ReservationAccess);
      if (!MUStrIsEmpty(J->PerPartitionPriority))
        MUStrDup(&tmpJ->PerPartitionPriority,J->PerPartitionPriority);

      MUStrDup(&tmpJ->BlockReason,J->BlockReason);
      MUStrDup(&tmpJ->Messages,J->Messages);
      MUStrDup(&tmpJ->Flags,J->Flags);
      MUStrDup(&tmpJ->ParentVCs,J->ParentVCs);

      bmcopy(&tmpJ->GenericAttributes,&J->GenericAttributes);

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        if ((J->Requirements[rqindex] == NULL) || (tmpJ->Requirements[rqindex] == NULL))
          {
          /* this could be a serious problem where the job in the cache only has 1 req but the
             real job has 2, and we are trying to update the second req in the cache but it
             doesn't exist, this job needs a full transition.  For now just ignore (MOAB-2018) */
          break;
          }

        /* Is the AllocNodeList mstring_t present AND 
         * Does the AllocNodeList have a buffer? NOTE: not is there a non-null string in it 
         */
        if ((J->Requirements[rqindex]->AllocNodeList != NULL) && 
            (J->Requirements[rqindex]->AllocNodeList->capacity() > 0))
          {
          /* Copy the AllocNodeList string to the tmpJ AllocNodeList */
          *(tmpJ->Requirements[rqindex]->AllocNodeList) = *(J->Requirements[rqindex]->AllocNodeList);
          }

        if (J->Requirements[rqindex]->UtilSwap > 0)
          {
          tmpJ->Requirements[rqindex]->UtilSwap = J->Requirements[rqindex]->UtilSwap;
          }

        if (J->Requirements[rqindex]->MinNodeCount > 0)
          {
          tmpJ->Requirements[rqindex]->MinNodeCount = J->Requirements[rqindex]->MinNodeCount;
          }

        if (J->Requirements[rqindex]->MinTaskCount > 0)
          {
          tmpJ->Requirements[rqindex]->MinTaskCount = J->Requirements[rqindex]->MinTaskCount;
          }
        }     
      }
    }

  if (DBWRITE)
    {
    MMongoInterface::MTransOToMongo(J,mxoJob);
    }

  pthread_rwlock_unlock(&MCache->JobLock);

  if (bmisset(&J->TFlags,mtransfLimitedTransition))
    {
    MJobTransitionFree((void **)&J);
    }

  return (rc);
  }  /* END MCacheWriteJob() */


/**
 * remove the job with the given name from the cache
 *
 * WARNING: this should only be called when renaming. Because the cache is a queue it's possible
 *          to call this routine, but have a queued up "write" behind it, resulting in a job
 *          that is in the cache but not in moab's memory.  You should "queue" up legitimate
 *          delete requests (mtransfDeleteExisting) rather than calling this routine
 * @param   JName (I)
 */

int MCacheRemoveJob(

    char *JName)

  {
  if (MUStrIsEmpty(JName))
    {
    MDB(3,fSCHED) MLog("INFO:     Blank name given. Cannot remove from cache.\n");

    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->JobLock);

  MUHTRemove(&MCache->Jobs,JName,&MJobTransitionFree);

  if (DBWRITE)
    {
    MMongoInterface::MMongoDeleteO(JName,mxoJob);
    }

  pthread_rwlock_unlock(&MCache->JobLock);

  return(SUCCESS);
  }



/**
 * remove the job with the given name from the cache
 *
 * @param   JName (I)
 */

int MCacheRemoveNode(

  char *Name)

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (MUStrIsEmpty(Name))
    {
    MDB(3,fSCHED) MLog("INFO:     Blank name given. Cannot remove from cache.\n");

    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->NodeLock);

  MUHTRemove(&MCache->Nodes,Name,&MNodeTransitionFree);

  if (DBWRITE)
    {
    MMongoInterface::MMongoDeleteO(Name,mxoNode);
    }

  pthread_rwlock_unlock(&MCache->NodeLock);

  return(SUCCESS);
  }   /* END MCacheRemoveNode */








/**
  * Write the given reservation to the transisition reservation table.
  *
  * NOTE: This routine is not threadsafe.  It should only be called
  *       by MOTransistionFromQueue() thread.
  *
  * @param R    (I) the reservation transistion object to write
  */

int MCacheWriteRsv(

  mtransrsv_t   *R)

  {
  int rc;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->RsvLock);

  if (bmisset(&R->TFlags,mtransfDeleteExisting))
    {
    MLog("INFO:      removing rsv %s from cache\n",R->Name);

    rc = MUHTRemove(&MCache->Rsvs,R->Name,MRsvTransitionFree);

    if (DBWRITE)
      {
      MMongoInterface::MMongoDeleteO(R->Name,mxoRsv);
      }

    MRsvTransitionFree((void **)&R);

    R = NULL;
    }
  else
    {
    rc = MUHTAdd(&MCache->Rsvs,R->Name,(void*)R,NULL,&MRsvTransitionFree);

    if (DBWRITE)
      {
      MMongoInterface::MTransOToMongo((mtransrsv_t *)R,mxoRsv);
      }
    }

  if (DBWRITE)
    {
    MMongoInterface::MTransOToMongo((mtransrsv_t *)R,mxoRsv);
    }

  pthread_rwlock_unlock(&MCache->RsvLock);

  return (rc);
  }  /* END MCacheWriteRsv() */




/**
  * Write the given trigger to the transisition trigger table.
  *
  * NOTE: This routine is not threadsafe.  It should only be called
  *       by MOTransistionFromQueue() thread.
  *
  * @param T    (I) the trigger transistion object to write
  */

int MCacheWriteTrigger(

  mtranstrig_t   *T) /* I */

  {
  int rc;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->TriggerLock);

  rc = MUHTAdd(&MCache->Triggers,T->TrigID,(void*)T,NULL,&MTrigTransitionFree);

  pthread_rwlock_unlock(&MCache->TriggerLock);

  return (rc);
  }  /* END MCacheWriteTrigger() */



/**
  * Write the given VC to the transisition trigger table.
  *
  * NOTE: This routine is not threadsafe.  It should only be called
  *       by MOTransistionFromQueue() thread.
  *
  * @param VC    (I) the VC transistion object to write
  */

int MCacheWriteVC(

  mtransvc_t *VC) /* I */

  {
  int rc;

  const char *FName = "MCacheWriteVC";

  MDB(8,fSCHED) MLog("%s(%s)\n",
    FName,
    (VC != NULL) ? VC->Name : NULL);

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->VCLock);

  if (VC->IsDeleteRequest == FALSE)
    rc = MUHTAdd(&MCache->VCs,VC->Name,(void *)VC,NULL,&MVCTransitionFree);
  else
    {
    rc = MUHTRemove(&MCache->VCs,VC->Name,&MVCTransitionFree);

    /* We freed the existing transvc from the hash table, but not the one
        that was just passed in */

    MVCTransitionFree((void **)&VC);
    }

  pthread_rwlock_unlock(&MCache->VCLock);

  return(rc);
  }  /* END MCacheWriteVC() */



/**
  * Write the given VM to the transisition trigger table.
  *
  * NOTE: This routine is not threadsafe.  It should only be called
  *       by MOTransistionFromQueue() thread.
  *
  * @param VM    (I) the VM transistion object to write
  */

int MCacheWriteVM(

  mtransvm_t *VM) /* I */

  {
  int rc;

  const char *FName = "MCacheWriteVM";

  MDB(8,fSCHED) MLog("%s(%s)\n",
    FName,
    (VM != NULL) ? VM->VMID : NULL);

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->VMLock);

  rc = MUHTAdd(&MCache->VMs,VM->VMID,(void *)VM,NULL,&MVMTransitionFree);

  if (DBWRITE)
    {
    MMongoInterface::MTransOToMongo(VM,mxoxVM);
    }

  pthread_rwlock_unlock(&MCache->VMLock);

  return(rc);
  }  /* END MCacheWriteVM() */


/**
 * Remove the VM with the given name from the cache.
 *
 * @param   Name (I)
 */

int MCacheRemoveVM(

    char *Name)

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (MUStrIsEmpty(Name))
    {
    MDB(3,fSCHED) MLog("INFO:     Blank name given. Cannot remove from cache.\n");

    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->VMLock);

  MUHTRemove(&MCache->VMs,Name,&MVMTransitionFree);

  if (DBWRITE)
    {
    MMongoInterface::MMongoDeleteO(Name,mxoxVM);
    }


  pthread_rwlock_unlock(&MCache->VMLock);

  return(SUCCESS);
  }





int MCacheNodeLock(

  mbool_t LockNode,
  mbool_t ReaderLock)     /* If true, issue reader lock; otherwise issue writer lock */

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (LockNode == TRUE)
    {
    if (ReaderLock == TRUE)
      pthread_rwlock_rdlock(&MCache->NodeLock);
    else
      pthread_rwlock_wrlock(&MCache->NodeLock);
    }
  else
    {
    pthread_rwlock_unlock(&MCache->NodeLock);
    }

  return(SUCCESS);
  }  /* END MCacheNodeLock() */




int MCacheJobLock(

  mbool_t LockJob,
  mbool_t ReaderLock)     /* If true, issue reader lock; otherwise issue writer lock */

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (LockJob == TRUE)
    {
    if (ReaderLock == TRUE)
      pthread_rwlock_rdlock(&MCache->JobLock);
    else
      pthread_rwlock_wrlock(&MCache->JobLock);
    }
  else
    {
    pthread_rwlock_unlock(&MCache->JobLock);
    }

  return(SUCCESS);
  }  /* END MCacheJobLock() */




int MCacheRsvLock(

  mbool_t LockRsv,
  mbool_t ReaderLock)     /* If true, issue reader lock; otherwise issue writer lock */

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (LockRsv == TRUE)
    {
    if (ReaderLock == TRUE)
      pthread_rwlock_rdlock(&MCache->RsvLock);
    else
      pthread_rwlock_wrlock(&MCache->RsvLock);
    }
  else
    {
    pthread_rwlock_unlock(&MCache->RsvLock);
    }

  return(SUCCESS);
  }  /* END MCacheRsvLock() */




int MCacheTriggerLock(

  mbool_t LockTrigger,
  mbool_t ReaderLock)     /* If true, issue reader lock; otherwise issue writer lock */

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (LockTrigger == TRUE)
    {
    if (ReaderLock == TRUE)
      pthread_rwlock_rdlock(&MCache->TriggerLock);
    else
      pthread_rwlock_wrlock(&MCache->TriggerLock);
    }
  else
    {
    pthread_rwlock_unlock(&MCache->TriggerLock);
    }

  return(SUCCESS);
  }  /* END MCacheTriggerLock() */


int MCacheVCLock(

  mbool_t LockVC,
  mbool_t ReaderLock)     /* If true, issue reader lock; otherwise issue writer lock */

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (LockVC == TRUE)
    {
    if (ReaderLock == TRUE)
      pthread_rwlock_rdlock(&MCache->VCLock);
    else
      pthread_rwlock_wrlock(&MCache->VCLock);
    }
  else
    {
    pthread_rwlock_unlock(&MCache->VCLock);
    }

  return(SUCCESS);
  }  /* END MCacheVCLock() */


int MCacheVMLock(

  mbool_t LockVM,
  mbool_t ReaderLock)     /* If true, issue reader lock; otherwise issue writer lock */

  {
  if (MCache == NULL)
    {
    return(FAILURE);
    }

  if (LockVM == TRUE)
    {
    if (ReaderLock == TRUE)
      pthread_rwlock_rdlock(&MCache->VMLock);
    else
      pthread_rwlock_wrlock(&MCache->VMLock);
    }
  else
    {
    pthread_rwlock_unlock(&MCache->VMLock);
    }

  return(SUCCESS);
  }  /* END MCacheVMLock() */


int MCacheLockAll()

  {
  if (MCache == NULL)
    return(FAILURE);

  /* Issue locks for nodes, jobs, reservations, and triggers */

  pthread_rwlock_wrlock(&MCache->NodeLock);
  pthread_rwlock_wrlock(&MCache->JobLock);
  pthread_rwlock_wrlock(&MCache->RsvLock);
  pthread_rwlock_wrlock(&MCache->TriggerLock);
  pthread_rwlock_wrlock(&MCache->VCLock);
  pthread_rwlock_wrlock(&MCache->VMLock);

  return(SUCCESS);
  }



int MCacheReadNodes(

  marray_t *NArray,
  marray_t *ConstraintList)

  {
  mhashiter_t HTIter;
  mtransnode_t *N;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&HTIter);

  pthread_rwlock_rdlock(&MCache->NodeLock);

  while (MUHTIterate(&MCache->Nodes,NULL,(void **)&N,NULL,&HTIter) == SUCCESS)
    {
    if (!MNodeTransitionFitsConstraintList(N,ConstraintList))
      continue;

    MUArrayListAppendPtr(NArray,N);
    } /* END while (MUHTIterate) */

  pthread_rwlock_unlock(&MCache->NodeLock);

  return(SUCCESS);
  }  /* END MCacheReadNodes() */




/**
 * Returns an array of mtransjob_t pointers from the cache.
 *
 * @param JArray (O)
 * @param ConstraintList (I)
 */

int MCacheReadJobs(

  marray_t *JArray,
  marray_t *ConstraintList)

  {
  mhashiter_t HTIter;
  char *Name;
  mtransjob_t *J;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&HTIter);

  pthread_rwlock_rdlock(&MCache->JobLock);

  while (MUHTIterate(&MCache->Jobs,&Name,(void **)&J,NULL,&HTIter) == SUCCESS)
    {
    if (MJobTransitionMatchesConstraints(J,ConstraintList))
      {
      MUArrayListAppendPtr(JArray,J);
      }
    else
      {
      MDB(8,fSCHED) MLog("INFO:     Job %s does not meet constraints.\n",J->Name);
      }
    } /* END while (MUHTIterate) */

  pthread_rwlock_unlock(&MCache->JobLock);

  return(SUCCESS);
  }  /* END MCacheReadJobs() */



/**
 * Returns an array of mtransrsv_t pointers from the cache.
 *
 * @param RArray (O)
 * @param ConstraintList (O)
 */

int MCacheReadRsvs(

  marray_t *RArray,
  marray_t *ConstraintList)

  {
  mhashiter_t HTIter;
  char *Name;
  mtransrsv_t *R;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&HTIter);

  pthread_rwlock_rdlock(&MCache->RsvLock);

  while (MUHTIterate(&MCache->Rsvs,&Name,(void **)&R,NULL,&HTIter) == SUCCESS)
    {
    if (MRsvTransitionMatchesConstraints(R,ConstraintList))
      {
      MUArrayListAppendPtr(RArray,R);
      }
    else
      {
      MDB(8,fSCHED) MLog("INFO:     Reservation %s does not meet constraints.\n",R->Name);
      }
    } /* END while (MUHTIterate) */

  pthread_rwlock_unlock(&MCache->RsvLock);

  return(SUCCESS);
  }  /* END MCacheReadRsvs() */




/**
 * Returns an array of mtranstrig_t pointers from the cache.
 *
 * @param RArray (O)
 * @param ConstraintList (I)
 */

int MCacheReadTriggers(

  marray_t *TArray,         /* O */
  marray_t *ConstraintList) /* I */

  {
  mhashiter_t HTIter;
  char *TrigID;
  mtranstrig_t *T;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&HTIter);

  pthread_rwlock_rdlock(&MCache->TriggerLock);

  while (MUHTIterate(&MCache->Triggers,&TrigID,(void **)&T,NULL,&HTIter) == SUCCESS)
    {
    if (MTrigTransitionMatchesConstraints(T,ConstraintList))
      {
      MUArrayListAppendPtr(TArray,T);
      }
    else
      {
      MDB(8,fSCHED) MLog("INFO:     Trigger %s does not meet constraints.\n",T->TrigID);
      }
    } /* END while (MUHTIterate) */

  pthread_rwlock_unlock(&MCache->TriggerLock);

  return(SUCCESS);
  }  /* END MCacheReadTriggers() */


/**
 * Returns an array of mtransvc_t pointers from the cache.
 *
 * @param RArray (O)
 * @param ConstraintList (I)
 */

int MCacheReadVCs(

  marray_t *VCArray,        /* O */
  marray_t *ConstraintList) /* I */

  {
  mhashiter_t HTIter;
  char *VCID;
  mtransvc_t *VC;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&HTIter);

  pthread_rwlock_rdlock(&MCache->VCLock);

  while (MUHTIterate(&MCache->VCs,&VCID,(void **)&VC,NULL,&HTIter) == SUCCESS)
    {
    if (MVCTransitionMatchesConstraints(VC,ConstraintList))
      {
      MUArrayListAppendPtr(VCArray,VC);
      }
    else
      {
      MDB(8,fSCHED) MLog("INFO:     VC %s does not meet constraints.\n",VC->Name);
      }
    } /* END while (MUHTIterate) */

  pthread_rwlock_unlock(&MCache->VCLock);

  return(SUCCESS);
  }  /* END MCacheReadVCs() */




/**
 * Returns an array of mtransvm_t pointers from the cache.
 *
 * @param RArray (O)
 * @param ConstraintList (I)
 */

int MCacheReadVMs(

  marray_t *VMArray,        /* O */
  marray_t *ConstraintList) /* I */

  {
  mhashiter_t HTIter;
  char *VMID;
  mtransvm_t *VM;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&HTIter);

  pthread_rwlock_rdlock(&MCache->VMLock);

  while (MUHTIterate(&MCache->VMs,&VMID,(void **)&VM,NULL,&HTIter) == SUCCESS)
    {
    if (MVMTransitionMatchesConstraints(VM,ConstraintList))
      {
      MUArrayListAppendPtr(VMArray,VM);
      }
    else
      {
      MDB(8,fSCHED) MLog("INFO:     VM %s does not meet constraints.\n",VM->VMID);
      }
    } /* END while (MUHTIterate) */

  pthread_rwlock_unlock(&MCache->VMLock);

  return(SUCCESS);
  }  /* END MCacheReadVMs() */




/**
 * Read jobs for showq, putting them in the appropriate bucket based on job state
 *
 * @see MCacheReadJobs
 *
 * @param StateBM           (I) which states we want to read jobs for
 * @param P                 (I) which partition we want to read jobs for [optional]
 * @param ConstraintList    (I) constraints to match
 * @param ActiveJobList     (O) the list for active jobs
 * @param BlockedJobList    (O) the list for blocked jobs
 * @param IdleJobList       (O) the list for idle jobs
 * @param CompletedJobList  (O) the list for completed jobs
 * @param ArrayMasterList   (O) the list of any array master jobs
 */

int MCacheReadJobsForShowq(

  mbitmap_t *StateBM,
  mpar_t   *P,
  marray_t *ConstraintList,
  marray_t *ActiveJobList,
  marray_t *BlockedJobList,
  marray_t *IdleJobList,
  marray_t *IdleReservedJobList,
  marray_t *CompletedJobList,
  marray_t *ArrayMasterList)

  {
  mhashiter_t   HTIterator;
  char         *Name;
  mtransjob_t  *J;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  MDB(7,fSCHED) MLog("INFO:     Reading jobs from the cache to service showq.");

  MUHTIterInit(&HTIterator);

  pthread_rwlock_rdlock(&MCache->JobLock);

  while (MUHTIterate(&MCache->Jobs,&Name,(void **)&J,NULL,&HTIterator) == SUCCESS)
    {
    if (ConstraintList != NULL)
      {
      if (MJobTransitionFitsConstraintList(J,ConstraintList) == FAILURE)
        continue;
      }

    /* add the job to the appropriate queue for its state, if the StateBM is
     * set for that state */

    /* don't show removed jobs unless we are doing a "showq -c" */
        
    if (((J->State == mjsRemoved) || (J->State == mjsCompleted)) && (!bmisset(StateBM,mjstCompleted)))
      continue; 

    if ((MJOBSISACTIVE(J->State)) && (bmisset(StateBM,mjstActive)))
      {
      MUArrayListAppendPtr(ActiveJobList,J);
      }
    else if (bmisset(&J->TFlags,mtransfArrayMasterJob))
      {
      MUArrayListAppendPtr(ArrayMasterList,J);
      }
    else if (J->State == mjsSuspended) /* suspended is still active -- add to active queue */
      {
      MUArrayListAppendPtr(ActiveJobList,J);
      }
    else if ((bmisset(StateBM,mjstBlocked)) &&
             (!MSTATEISCOMPLETE(J->State)) &&
             (J->State != mjsRunning) &&
             MJobTransitionJobIsBlocked(J,P))
      {
      MUArrayListAppendPtr(BlockedJobList,J);
      }
    else if (!(MUStrIsEmpty(J->Dependencies)) &&
        (((MJobTransitionJobIsBlocked(J,P)) ||
        (bmisset(StateBM,mjstBlocked))) && (!MSTATEISCOMPLETE(J->State))))
      {
      MUArrayListAppendPtr(BlockedJobList,J);
      }
    else if ((!bmisclear(&J->Hold)) && (bmisset(StateBM,mjstBlocked)))
      {
      MUArrayListAppendPtr(BlockedJobList,J);
      }
    else if (J->State == mjsNotQueued)
      {
      MUArrayListAppendPtr(BlockedJobList,J);
      }
    else if ((MSTATEISCOMPLETE(J->State)) && (bmisset(StateBM,mjstCompleted)))
      {
      MUArrayListAppendPtr(CompletedJobList,J);
      }
    else if ((MUStrIsEmpty(J->Dependencies)) &&
        (bmisset(StateBM,mjstEligible)) && 
        (MJOBSISIDLE(J->State)) && 
        (!MJOBSISBLOCKED(J->State)) &&
        (bmisclear(&J->Hold)) && 
        (!MJobTransitionJobIsBlocked(J,P)))
      {
      if (J->RsvStartTime > 0)
        MUArrayListAppendPtr(IdleReservedJobList,J);
      else
        MUArrayListAppendPtr(IdleJobList,J);
      }
    } /* END while(MUHTIterate...) */

  pthread_rwlock_unlock(&MCache->JobLock);

  return(SUCCESS);
  } /* END MCacheReadJobsForShowq() */




/**
 * Remove jobs from the cache after they have been completed for longer than
 * JOBCPURGETIME
 *
 */

int MCachePruneCompletedJobs()

  {
  marray_t      RemoveList;
  mtransjob_t  *tmpJob;
  mhashiter_t   HTIterator;
  char         *Name;
  int           jindex;

  if (MCache == NULL)
    {
    return(SUCCESS);
    }

  if (MCache->Jobs.NumItems == 0)
    {
    return(SUCCESS);
    }

  MUArrayListCreate(&RemoveList,sizeof(mtransjob_t *),10);

  /* loop through the hash table and put a pointer to any job that has
   * exceeded JOBCPURGETIME in the RemoveList */

  MUHTIterInit(&HTIterator);

  pthread_rwlock_rdlock(&MCache->JobLock);

  while (MUHTIterate(&MCache->Jobs,&Name,(void **)&tmpJob,NULL,&HTIterator) == SUCCESS)
    {
    if (tmpJob->CompletionTime > 0)
      {
      if ((mulong)tmpJob->CompletionTime < (MSched.Time - MSched.JobCPurgeTime))
        {
        MUArrayListAppendPtr(&RemoveList,tmpJob);
        }
      }
    } /* END while (MUHTIterate...) */

  /* remove everything in RemoveList from the cache */

  for (jindex = 0; jindex < RemoveList.NumItems; jindex++)
    {
    tmpJob = (mtransjob_t *)MUArrayListGetPtr(&RemoveList,jindex);

    MUHTRemove(&MCache->Jobs,tmpJob->Name,NULL);

    if (DBWRITE)
      {
      MMongoInterface::MMongoDeleteO(tmpJob->Name,mxoJob);
      }

    MJobTransitionFree((void **)&tmpJob);

    tmpJob = NULL;
    } /* END for (jindex...) */

  pthread_rwlock_unlock(&MCache->JobLock);

  MUArrayListFree(&RemoveList);

  return(SUCCESS);
  } /* END MCachePruneCompletedJobs() */



/**
  * Delete the given trigger From the transisition trigger table.
  *
  * @param T (I) the trigger transistion object to write
  */

int MCacheDeleteTrigger(

  mtranstrig_t *T) /* I */

  {
  int rc;

  if (MCache == NULL)
    {
    return(FAILURE);
    }

  pthread_rwlock_wrlock(&MCache->TriggerLock);

  rc = MUHTRemove(&MCache->Triggers,T->TrigID,&MTrigTransitionFree);

  pthread_rwlock_unlock(&MCache->TriggerLock);

  return (rc);
  }  /* END MCacheDeleteTrigger() */




#else /* __MCOMMTHREAD */

int MCacheRemoveJob(

    char *JName)

  {
  return(SUCCESS);
  }  /* END MCacheRemoveJob() */

int MCacheInitialize(

  mcache_t **Cache)

  {
  return(SUCCESS);
  }  /* END MCacheInitialize() */

void MCacheDestroy(

  mcache_t *Cache)

  {
  return;
  }  /* END MCacheDestroy() */

int MCacheSysInitialize()

  {
  return(MCacheInitialize(&MCache));
  }  /* END MCacheSysInitialize() */

void MCacheSysShutdown()

  {
  MCacheDestroy(MCache);
  }  /* END MCacheSysShutdown() */

int MCacheWriteNode(

  mtransnode_t    *N)

  {
  return(SUCCESS);
  }  /* END MCacheWriteNode() */

int MCacheWriteJobs(

  mtransjob_t   *J)

  {
  return(SUCCESS);
  }  /* END MCacheWriteJob() */

int MCacheNodeLock(

  mbool_t LockNode,
  mbool_t ReaderLock)

  {
  return(SUCCESS);
  }  /* END MCacheNodeLock() */

int MCacheJobLock(

  mbool_t LockJob,
  mbool_t ReaderLock)

  {
  return(SUCCESS);
  }  /* END MCacheJobLock() */

int MCacheReadNodes(

  marray_t *NArray,
  marray_t *ConstraintList)

  {
  return(SUCCESS);
  }  /* END MCacheReadNodes() */

int MCacheReadJobs(

  marray_t *JArray,
  marray_t *ConstraintList)

  {
  return(SUCCESS);
  }  /* END MCacheReadJobs() */

int MCacheReadJobsForShowq(

  mulong    StateBM,
  marray_t *ConstraintList,
  marray_t *ActiveJobList,
  marray_t *BlockedJobList,
  marray_t *IdleJobList,
  marray_t *IdleReservedJobList,
  marray_t *CompletedJobList)

  {
  return(SUCCESS);
  }

int MCachePruneCompletedJobs()

  {
  return(SUCCESS);
  }

int MCacheDeleteTrigger(

  mtranstrig_t   *T)

  {
  return(SUCCESS);
  }

#endif /* !__MCOMMTHREAD */

/* END MCache.c */
