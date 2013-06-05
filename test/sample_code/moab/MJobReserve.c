/* HEADER */

/**
 * @file MJobReserve.c
 *
 * Contains Job Reservation functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Set the job reservation to the earliest reservation between the current reservation and the saved reservation.
 *
 * @param J        (I) [modified]
 * @param P        (I)
 * @param SavedRsv (I) 
 */

int __MJobSelectReservation(

  mjob_t *J,
  mpar_t *P,
  mrsv_t *SavedRsv)

  {
  char          SRsvStartTime[MMAX_NAME];
  char          JRsvStartTime[MMAX_NAME];
  char          TString[MMAX_LINE];

  if (J == NULL)
    return(FAILURE);

  if (SavedRsv == NULL)
    return(SUCCESS);

  MULToTString(SavedRsv->EndTime - MSched.Time,TString);
  strcpy(SRsvStartTime,TString);

  if (J->Rsv == NULL)
    {
    MDB(7,fSCHED) MLog("INFO:     keeping old rsv for job %s to start %s in partition %s - scheduling partition %s\n",
      J->Name,
      SRsvStartTime,
      MPar[SavedRsv->PtIndex].Name, 
      (P != NULL) ? P->Name : MPar[0].Name);

    J->Rsv = SavedRsv;

    return(SUCCESS);
    }

  if (J->Rsv != NULL)
    {
    MULToTString(J->Rsv->EndTime - MSched.Time,TString);
    strcpy(JRsvStartTime,TString);
    }

  if (J->Rsv->StartTime > SavedRsv->StartTime)
    {
    MDB(7,fSCHED) MLog("INFO:     releasing new rsv for job %s to start %s in partition %s - scheduling partition %s\n",
      J->Name,
      JRsvStartTime,
      MPar[J->Rsv->PtIndex].Name, 
      (P != NULL) ? P->Name : MPar[0].Name);

    MJobReleaseRsv(J,TRUE,TRUE);

    MDB(7,fSCHED) MLog("INFO:     keeping old rsv for job %s to start %s in partition %s - scheduling partition %s\n",
      J->Name,
      SRsvStartTime,
      MPar[SavedRsv->PtIndex].Name, 
      (P != NULL) ? P->Name : MPar[0].Name);

    J->Rsv = SavedRsv;
    }
  else
    {
    mrsv_t *tmpRsv;

    MDB(7,fSCHED) MLog("INFO:     releasing old rsv for job %s to start %s in partition %s - scheduling partition %s\n",
      J->Name,
      SRsvStartTime,
      MPar[SavedRsv->PtIndex].Name, 
      (P != NULL) ? P->Name : MPar[0].Name);

    tmpRsv = J->Rsv;
    J->Rsv = SavedRsv;
    MJobReleaseRsv(J,TRUE,TRUE);
    J->Rsv = tmpRsv;

    MDB(7,fSCHED) MLog("INFO:     keeping new rsv for job %s to start %s in partition %s - scheduling partition %s\n",
      J->Name,
      JRsvStartTime,
      MPar[J->Rsv->PtIndex].Name, 
      (P != NULL) ? P->Name : MPar[0].Name);
    }

  return(SUCCESS);
  } /* END __MJobSelectReservation */




/**
 * Identify best time/resources for job reservation and then create said reservation.
 *
 * NOTE: P specifies partition in which to run
 * 
 * NOTE: fixme to NOT destroy J on failure - schedule destruction for end of 
 *       scheduling cycle.
 *
 * NOTE: if J->IFlags has mjifReserveAlways set and job is system job, create 
 *       placeholder reservation out at infinity on failure.
 *
 * @param JP (I) [modified/possibly freed on failure]
 * @param P (I) [optional]
 * @param EStartTime (I) [absolute,optional,set <=0 for not set]
 * @param RsvType (I)
 * @param NoRsvTransition (I)
 *
 * @see MJobGetEStartTime() - child
 */

int MJobReserve(
 
  mjob_t                **JP,           /* I (modified/possibly freed on failure) */
  mpar_t                 *P,            /* I (optional) */
  mulong                  EStartTime,   /* I earliest start time (absolute,optional,set <=0 for not set) */
  enum MJRsvSubTypeEnum   RsvType,      /* I */ 
  mbool_t                 NoRsvTransition) /* I  don't transition */
 
  {
  long          BestTime = 0;
  mpar_t       *BestP;
 
  mnl_t        *MNodeList[MMAX_REQ_PER_JOB];
 
  int           NodeCount;
 
  mbool_t       MultiReqJob = FALSE;

  mulong        InitialCompletionTime;
  char          IStringTime[MMAX_NAME];
  char          Message[MMAX_LINE];
  char          TString[MMAX_LINE];
  char          DString[MMAX_LINE];
  char         *MBPtr;
  int           MBSpace;
 
  int           sindex;
 
  int           BestSIndex;
  unsigned long BestSTime;
 
  unsigned long DefaultSpecWCLimit;
  int           DefaultTaskCount;

  int           rc;

  mjob_t       *J; 
  mreq_t       *RQ;

  mbool_t      PerPartitionScheduling = FALSE;
  mrsv_t       *SavedRsv = NULL;

  const char *FName = "MJobReserve";

  MDB(3,fSCHED) MLog("%s(%s,%s,%ld,%s)\n",
    FName,
    ((JP != NULL) && (*JP != NULL)) ? (*JP)->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    EStartTime,
    MJRType[RsvType]);
 
  if ((JP == NULL) || (*JP == NULL) || ((*JP)->Req[0] == NULL))
    {
    return(FAILURE);
    }

  if ((P != NULL) && 
      (MSched.PerPartitionScheduling == TRUE) &&         
      (P->Index != 0))
    {
    PerPartitionScheduling = TRUE; 
    }

  J = *JP;

  if (MJobIsPreempting(J) == TRUE)
    {
    return(SUCCESS);
    }

  /* if job already reserved, release (or save) rsv and try rsv again */
 
  if (J->Rsv != NULL)
    {
    char TString[MMAX_LINE];
    InitialCompletionTime = J->Rsv->EndTime;
 
    MULToTString(InitialCompletionTime - MSched.Time,TString);
    strcpy(IStringTime,TString);

    if ((PerPartitionScheduling == TRUE) && (J->Rsv->PtIndex != P->Index))
      {
      /* we are doing a priority per partition reservation.
         save the current reservation and see if we can get a better reservation.
         be sure and release either this reservation or the new reservation
         before leaving this routine. */

      SavedRsv = J->Rsv;
      J->Rsv = NULL;
      }
    else
      {     
      MJobReleaseRsv(J,TRUE,TRUE);
      }
    }
  else
    {
    InitialCompletionTime = 0;
    }

  if (J->Depend != NULL)
    {
    mdepend_t *D;
    mjob_t    *DJ;

    /* if job has prereq dependency, first reserve prereq job and adjust EStartTime */

    for (D = J->Depend;D != NULL;D = D->NextAnd)
      {
      if ((D->Type != mdJobCompletion) && 
          (D->Type != mdJobSuccessfulCompletion))
        {
        /* ignore non-completion based job dependencies for now */

        continue;
        }

      if ((D->Value == NULL) || 
          (MJobFind(D->Value,&DJ,mjsmExtended) == FAILURE))
        {
        MDB(2,fSCHED) MLog("ALERT:    cannot locate prereq job '%s'\n",
          (D->Value != NULL) ? D->Value : "NULL");

        /* cannot create/validate reservation for prereq job */

        continue;
        }
         
      if (DJ->Rsv != NULL)
        {
        /* job is active/idle and reservation already exists */

        EStartTime = MAX(EStartTime,DJ->Rsv->EndTime);
        }
      else if (MJobReserve(&DJ,NULL,EStartTime,RsvType,NoRsvTransition) == SUCCESS)
        {
        /* new reservation successfully created */

        /* get a reservation for this job even though it may have a low priority? */

        EStartTime = MAX(EStartTime,DJ->Rsv->EndTime);
        }
      else
        {
        /* could not create reservation for prereq job */

        if (DJ != NULL)
          {
          MDB(2,fSCHED) MLog("ALERT:    cannot create reservation for prereq job '%s'\n",
            DJ->Name);
          }

        /* NOTE:  create reservation for dependent job anyway */

        /* NO-OP */
        }
      }    /* END for (D) */
    }      /* END if (J->Depend != NULL) */

  MultiReqJob = MJobIsMultiReq(J);

  /* NOTE:  supports only one malleable req */
 
  RQ = J->Req[0];
 
  DefaultSpecWCLimit = J->SpecWCLimit[0];
  DefaultTaskCount   = RQ->TaskRequestList[0];
 
  BestSIndex = 0;
 
  BestSTime  = MMAX_TIME;
  BestP      = P;

  MUSNInit(&MBPtr,&MBSpace,Message,sizeof(Message));

  MNLMultiInit(MNodeList);

  for (sindex = 1;RQ->TaskRequestList[sindex] > 0;sindex++)
    {
    /* find earliest completion time for each shape */
 
    /* If rsv type is priority, then attempt to start immediately has failed.
       Make certain resource rsv is in the future. */

    if (RsvType == mjrPriority)
      {
      if ((MSched.Mode == msmTest) || (MSched.Mode == msmMonitor))
        {
        BestTime = MAX(MSched.Time + MSched.MaxRMPollInterval,J->SMinTime);
        }
      else
        {
        /* NOTE:  job cannot run now, do not attempt to retry job for
                  at least MIN_FUTURERSVTIME seconds */

        BestTime = MAX(MSched.Time + MSched.JobRsvRetryInterval,J->SMinTime);
        }
      }
    else
      {
      BestTime = MAX(MSched.Time,J->SMinTime);
      }
 
    if (EStartTime > 0)
      BestTime = MAX(BestTime,(long)EStartTime);

    if (MSched.EnableHighThroughput == FALSE)
      {
      /* identify timeframe when all job policies can be satisfied */

      if (MPolicyGetEStartTime(
          (void *)J,
          mxoJob,
          (P != NULL) ? P : &MPar[0],
          mptSoft,
          &BestTime,  /* I/O */
          NULL) == FAILURE)
        {
        /* return failure */

        /* NYI */
        }
      }

    /* FIXME:  single req */ 

    J->SpecWCLimit[0]      = J->SpecWCLimit[sindex];

    J->Request.TC += RQ->TaskRequestList[sindex] - RQ->TaskCount;
 
    RQ->TaskRequestList[0] = RQ->TaskRequestList[sindex];
    RQ->TaskCount          = RQ->TaskRequestList[sindex];

    /* determine when and where reservation can be created */

    if (MultiReqJob == TRUE)
      {
      int  rindex = 0;

      long NextETime = BestTime;

      rc = FAILURE;

      while ((rc == FAILURE) && (NextETime < MMAX_TIME))
        {
        rc = MJobGetEStartTime(
               J,
               &BestP,      /* I/O */
               &NodeCount,
               NULL,
               MNodeList,   /* populated */
               &BestTime,   /* I/O */
               &NextETime,  /* O */
               FALSE,
               FALSE,
               NULL);

        rindex ++;

        if (rc == FAILURE)
          BestTime = NextETime;

        if (rc >= MMAX_RANGE)
          break;
        }  /* END while ((rc == FAILURE) && (NextETime < MMAX_TIME)) */
      }    /* END if (MultiReqJob == TRUE) */
    else
      {
      rc = MJobGetEStartTime(
             J,
             &BestP,      /* I/O */
             &NodeCount,
             NULL,
             MNodeList,   /* populated */
             &BestTime,   /* I/O */
             NULL,        /* O */
             TRUE,        /* look in remote partitions */
             FALSE,
             NULL);
      }
 
    if (rc == FAILURE)
      {
      /* job cannot be run on selected partition with selected shape */
 
      MUSNPrintF(&MBPtr,&MBSpace,"cannot create reservation for job '%s' on partition %s",
        J->Name,
        (P != NULL) ? P->Name : MPar[0].Name);
 
      if (InitialCompletionTime > 0)
        {
        MDB(1,fSCHED) MLog("ALERT:    cannot create rsv (job '%s' previously reserved to start in %s)\n",
          J->Name,
          IStringTime);
 
        MUSNPrintF(&MBPtr,&MBSpace," (job previously reserved to start in %s)\n",
          IStringTime);
        }
      else
        {
        MDB(1,fSCHED) MLog("ALERT:    cannot create new rsv for job %s (shape[%d] %d)\n",
          J->Name,
          sindex,
          RQ->TaskRequestList[sindex]);
 
        MUSNPrintF(&MBPtr,&MBSpace," (requested resources not available at any time)\n");
        }

      continue;
      }    /* END if (MJobGetEStartTime() == FAILURE) */

    /* valid timeframe located */

    if (BestTime + J->SpecWCLimit[sindex] < BestSTime)
      {
      BestSTime = BestTime + J->SpecWCLimit[sindex];
      BestSIndex = sindex;
      }
    }      /* END for (sindex) */

  J->SpecWCLimit[0]      = DefaultSpecWCLimit;
  J->Alloc.TC            = DefaultTaskCount;
  RQ->TaskRequestList[0] = DefaultTaskCount;
  RQ->TaskCount          = DefaultTaskCount;
 
  if ((BestSTime == MMAX_TIME) && (J->SpecWCLimit[0] != MMAX_TIME))
    {
    /* job cannot be run on any (or requested) partition */

    if (bmisset(&J->IFlags,mjifReserveAlways) && (J->System != NULL))
      {
      int rc;

      rc = MJobForceReserve(J,P);

      if (SavedRsv != NULL)
        __MJobSelectReservation(J,P,SavedRsv);

      MNLMultiFree(MNodeList);

      return(rc);
      }
 
    MUSNPrintF(&MBPtr,&MBSpace,"cannot create reservation for job '%s' on partition %s",
      J->Name,
      (P != NULL) ? P->Name : MPar[0].Name);
 
    if (InitialCompletionTime > 0)
      {
      MDB(1,fSCHED) MLog("ALERT:    cannot create reservation (job '%s' previously reserved to start in %s)\n",
        J->Name,
        IStringTime);
 
      MUSNPrintF(&MBPtr,&MBSpace," (job previously reserved to start in %s)\n",
        IStringTime);

      /* NOTE:  should walk original node list and determine which nodes are not available (NYI) */
      }
    else
      {
      MDB(1,fSCHED) MLog("ALERT:    cannot create new reservation for job %s in partition %s\n",
        J->Name, 
        (P != NULL) ? P->Name : MPar[0].Name);
 
      MUSNPrintF(&MBPtr,&MBSpace," (requested resources not available at any time)\n");
      }

    if (PerPartitionScheduling == TRUE)
      {
      /* take no action if resources not available for specific partition */

      /* NO-OP */
      }
    else if (RsvType == mjrDeadline)
      {
      /* set holds on jobs that had a deadline but didn't make it */

      /* take action at parent level */

      /* NO-OP */
      }
    else if ((InHardPolicySchedulingCycle == FALSE) && 
             (MSched.HPEnabledRsvExists == TRUE))
      {
      /* don't defer jobs during soft policy scheduling if an HPEnabled rsv exists */

      /* NO-OP */
      }
    else if ((bmisset(&MSched.Flags,mschedfAllowInfWallTimeJobs)) &&
              ((J->SpecWCLimit[0] == MMAX_TIME) ||
                (J->WCLimit == MMAX_TIME)))
      {
      /* If there are infinite walltime jobs, we may have no resources because
          the system is full, not because there is no feasible place to run */

      /* NO-OP */
      }
    else if ((MSched.Mode != msmMonitor) || 
             (getenv("MOABENABLESYSLOG") != NULL))
      {
      if (MSched.NoJobHoldNoResources == FALSE)
        {
        MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,Message);
        }

      MOSSyslog(LOG_NOTICE,Message);
      }
 
    if (SavedRsv != NULL)
      __MJobSelectReservation(J,P,SavedRsv);

    MNLMultiFree(MNodeList);

    return(FAILURE);
    }  /* END if (BestTime == MMAX_TIME) */
 
  /* FIXME:  does not handle multi-req jobs */
 
  J->SpecWCLimit[0]      = J->SpecWCLimit[BestSIndex];

  J->Request.TC += RQ->TaskRequestList[BestSIndex] - RQ->TaskCount;
 
  RQ->TaskRequestList[0] = RQ->TaskRequestList[BestSIndex];
  RQ->TaskCount          = RQ->TaskRequestList[0];

  /* NodeCount could be lingering with an incorrect value from a requeue and a
   * specified trl. ex. a job is running on 3 nodes, then requeued and those 3 
   * nodes aren't available anymore, only one is, which it can run on. The 
   * RQ->NodeCount is 3 which messes up the distribution of tasks in MJobDistributeTasks */

  /* Should this set differently or higher? */

  /* 10/6/2008: this is breaking the case where when MNODEMATCHPOLICY EXACTNODE is set,
   * this line destroys the previous value of NodeCount computed to satisfy the 
   * requirement of this parameter. 
  RQ->NodeCount = 0; */
 
  J->SpecWCLimit[BestSIndex]      = DefaultSpecWCLimit;

  /* When job is requeued, and taskreqlist is 4,8,12 and 12 was being used,
   * the taskreqlist gets set to 12,8,12 */
  /* RQ->TaskRequestList[BestSIndex] = DefaultTaskCount; */
 
  /* must call 'MJobDistributeTasks()' to get proper task count */

  /* NOTE:  must check locality constraints */

  /* NYI */

  /* JOSH: this is where we will compare our BestTime to our RemoteBestTime */
  /* if RemoteBestTime is better, then determine remote hostlist and adjust MNodeList */
 
  if (J->Req[1] == NULL)
    {
    int *tmpTaskList;

    mnl_t     tmpNodeList = {0};
    mnl_t     CopyOfJNodeList = {0};
 
    MNLInit(&CopyOfJNodeList);
    MNLInit(&tmpNodeList);

    /* NOTE:  make temporary copy of original MNodeList[0] so it doesn't 
              get modified by MJobDistributeTasks(). tmpNodeList will be pointed
              to by J->Req[0]->NodeList which is modified in MJobDistributeTasks
              as it is used as the basis for distributing nodes. */

    MNLCopy(&CopyOfJNodeList,&J->Req[0]->NodeList);
    MNLCopy(&tmpNodeList,MNodeList[0]);
 
    MNLCopy(&RQ->NodeList,&tmpNodeList);

    /* NOTE:  build node/task list for all RM's */
 
    tmpTaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

    MJobDistributeTasks(
      J,
      &MRM[RQ->RMIndex],  /* RQ->RMIndex used as DRM */
      FALSE,
      MNodeList[0],       /* O */
      tmpTaskList,        /* O */
      MSched.JobMaxTaskCount);

    MUFree((char **)&tmpTaskList);

    /* tmpNodeList points to RQ->NodeList which was modified in MJobDistributeTasks.
     * It's possible that RQ->NodeList will come out like: [node5:4][node6:0]. 
     * tmpNodeList should not be copied to MNodeList[0] since MNodeList[0] is the 
     * distributed result of MJobDistributeTasks and is used in MRsvJCreate */
 
    MNLCopy(&tmpNodeList,MNodeList[0]);

    /* NOTE:  MNodeList[1...] are empty */

    rc = MRsvJCreate(J,MNodeList,BestTime,RsvType,NULL,NoRsvTransition);

    /* restore original node list pointer */

    MNLCopy(&RQ->NodeList,&CopyOfJNodeList);

    MNLFree(&tmpNodeList);
    MNLFree(&CopyOfJNodeList);
    }    /* END if (J->Req[1] == NULL) */
  else
    {
    /* NOTE:  need multi-req distribution handling */
 
    if (!bmisset(&J->IFlags,mjifMasterSyncJob))
      {
      rc = MRsvJCreate(J,MNodeList,BestTime,RsvType,NULL,NoRsvTransition);
      }
    else
      {
      /* this routine calls MRsvJCreate() -- no duplicate functionality */

      rc = MRsvSyncJCreate(J,MNodeList,BestTime,RsvType,NULL,NoRsvTransition);
      }
    }

  if (rc == FAILURE)
    {
    MDB(1,fSCHED) MLog("ALERT:    cannot create reservation in %s\n",
      FName);

    if (SavedRsv != NULL)
      __MJobSelectReservation(J,P,SavedRsv);

    MNLMultiFree(MNodeList);

    return(FAILURE);
    }

  if ((MSched.PerPartitionScheduling == TRUE) && (SavedRsv != NULL))
    __MJobSelectReservation(J,P,SavedRsv);

  MULToTString(BestTime - MSched.Time,TString);
  MULToDString((mulong *)&BestTime,DString);

  MDB(2,fSCHED) MLog("INFO:     job '%s' reserved %d tasks (partition %s) to start in %s on %s (WC: %ld)\n",
    J->Name,
    J->Request.TC,
    BestP->Name,
    TString,
    DString,
    J->SpecWCLimit[BestSIndex]);
  
  MDB(8,fSCHED)
    {
    if ((J->Rsv != NULL) && (!MNLIsEmpty(&J->Rsv->NL)))
      {
      char tmpBuf[MMAX_BUFFER];

      MNLToString(&J->Rsv->NL,TRUE,",",'\0',tmpBuf,sizeof(tmpBuf));

      MLog("INFO:     job %s's reserved nodes: %s\n",
        J->Name,
        tmpBuf);
      }
    }
 
  if ((InitialCompletionTime > 0) &&
      (J->Rsv != NULL) &&
      (J->Rsv->EndTime > InitialCompletionTime))
    {
    MULToTString(J->Rsv->EndTime - MSched.Time,TString);

    MUSNPrintF(&MBPtr,&MBSpace,"reservation completion for job '%s' delayed from %s to %s",
      J->Name,
      IStringTime,
      TString);

    MDB(1,fSCHED) MLog("ALERT:    %s\n",
      Message);

    MUSNPrintF(&MBPtr,&MBSpace,"reservation completion for job '%s' delayed from %s to %s\n",
      J->Name,
      IStringTime,
      TString);

    if ((MSched.Mode != msmMonitor) || (getenv("MOABENABLESYSLOG") != NULL))
      MOSSyslog(LOG_NOTICE,Message);

    if (getenv("MOABADJUSTLOGLEVEL"))
      {
      /* increase loglevel to 6 for 5 iterations */

      MSched.OLogLevel = mlog.Threshold;
      MSched.OLogLevelIteration = MSched.Iteration + 5;

      mlog.Threshold = 6;
      }
    }    /* END if ((InitialCompletionTime > 0) && ...) */

  if ((J->Credential.Q != NULL) &&
      (J->Credential.Q->QTTriggerThreshold > 0) &&
      (J->Rsv != NULL) &&
      (J->Rsv->StartTime - J->SystemQueueTime > (mulong)J->Credential.Q->QTTriggerThreshold))
    {
    /* trigger is attached to job not QOS */

    MOReportEvent(J,J->Name,mxoJob,mttFailure,MSched.Time,TRUE);
    }

  MNLMultiFree(MNodeList);

  return(SUCCESS);
  }  /* END MJobReserve() */






/**
 * Attempts to create a reservation for a job based on priority.
 *
 * The priority of the job is important since policy allows the X highest priority jobs to have reservations created.
 * 
 * Incorporate QoS bucket reservation depth info to determine if job is eligible for a reservation.
 * look for available resources to reserve in all partitions.
 * NOTE:  does not yet take into account job dependencies
 * 
 * @param J (I, Modified) The job that the reservation should be created for (if possible).
 * @param P (I) The Partition to reserve the job on.
 * @param EStartTime (I) The Earliest start time this reservation can have (<=0 means not considered). 
 * @param RsvCountRej (O, Optional) whether job was rejected because no space in bucket or not
 * @param LockPartition (I) 
 */

int MJobPReserve(
 
  mjob_t  *J,           /* I (modified) */
  mpar_t  *P,           /* I (only used for reservation policy,optional) */
  mbool_t  LockPartition,  /* I lock rsv to specified partition */
  mulong   EStartTime,  /* I earliest time this rsv can start (optional) */
  mbool_t *RsvCountRej) /* O (optional) */

  {
  enum MRsvPolicyEnum GRsvPolicy;
  enum MRsvPolicyEnum RsvPolicy;
 
  mbool_t DoReserve;
  mbool_t ThresholdReached = FALSE;
 
  double XFactor;
 
  int bindex;
 
  const char *FName = "MJobPReserve";

  MDB(1,fSCHED) MLog("%s(%s,%s,%s,%ld,RsvCountRej)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    MBool[LockPartition],
    EStartTime);

  if (RsvCountRej != NULL)
    *RsvCountRej = FALSE;

  if (J == NULL) 
    {
    return(FAILURE);
    }
 
  if (!bmisclear(&J->Hold))
    {
    /* job has hold set */

    MDB(1,fSCHED) MLog("INFO:     reservation not allowed for job %s in %s (hold)\n",
      J->Name,
      FName);

    return(FAILURE);
    }
 
  if ((J->Credential.Q != NULL) && (bmisset(&J->Credential.Q->Flags,mqfNoReservation)))
    {
    MDB(1,fSCHED) MLog("INFO:     reservation not allowed for job %s in %s (QOS)\n",
      J->Name,
      FName);

    return(FAILURE);
    }
 
  if (MPar[0].BFPolicy == mbfPreempt)
    {
    MDB(1,fSCHED) MLog("INFO:     no priority reservations created (preempt based backfill enabled)\n");

    return(FAILURE);
    }

  GRsvPolicy = (MPar[0].RsvPolicy == mrpDefault) ?
    MDEF_RSVPOLICY : MPar[0].RsvPolicy;
 
  RsvPolicy = ((P != NULL) && (P->RsvPolicy != mrpDefault)) ?
    P->RsvPolicy : GRsvPolicy;
 
  if ((MPar[0].BFPolicy == mbfNONE) ||
      (RsvPolicy == mrpNever))
    {
    /* no reservations required */
 
    /* verify job can run */

    if (MSched.DisableJobFeasibilityCheck == FALSE)
      { 
      /* create temporary reservation so don't transition it */

      if (MJobReserve(
            &J,
            (LockPartition == TRUE) ? P : NULL,
            EStartTime,
            mjrPriority,
            TRUE) == SUCCESS)
        {
        /* release temporary reservation */
 
        MJobReleaseRsv(J,TRUE,TRUE);
        }
      else
        {
        /* job can never run */
 
        MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,"cannot create reservation");
        }
      }

    if (RsvCountRej != NULL)
      *RsvCountRej = TRUE;

    MDB(1,fSCHED) MLog("INFO:     no priority reservation created for job %s (bf/rsv policy)\n",
      J->Name);
 
    return(FAILURE);
    }  /* END if ((MPar[0].BFPolicy == mbfNONE) || (RsvPolicy == mrpNever)) */
 
  if ((J->Rsv != NULL) && (MSched.Time - J->Rsv->MTime < 120))
    {
    /* job has current reservation (should be non-priority) */

    MDB(1,fSCHED) MLog("INFO:     no priority reservations created (existing rsv)\n");
 
    return(FAILURE);
    }
 
  /* reserve nodes for highest priority idle job */
 
  DoReserve = FALSE;
 
  if (MPar[0].BFPolicy != mbfNONE)
    {
    switch (RsvPolicy)
      {
      case mrpNever:
 
        /* do not reserve nodes */
 
        DoReserve = FALSE;
 
        break;
 
      case mrpHighest:
      case mrpCurrentHighest:
 
        if ((J->Credential.Q != NULL) && 
           ((J->Credential.Q->QTRsvThreshold > 0) || (J->Credential.Q->XFRsvThreshold > 0.0)))
          {
          XFactor = ((double)(MSched.Time - J->SystemQueueTime) + J->SpecWCLimit[0]) /
                      J->SpecWCLimit[0];

          if ((J->Credential.Q->QTRsvThreshold > 0) && 
             ((long)(MSched.Time - J->SystemQueueTime) > J->Credential.Q->QTRsvThreshold))
            {
            DoReserve = TRUE;
            ThresholdReached = TRUE;
            }
          else if ((J->Credential.Q->XFRsvThreshold > 0.0) &&
                   (XFactor > J->Credential.Q->XFRsvThreshold))
            {
            DoReserve = TRUE;
            ThresholdReached = TRUE;
            }
          }

        /* If a threshold was not reached, then apply normal constraints */

        if (ThresholdReached != TRUE)
          {
          switch (MPar[0].RsvTType)
            {
            case mrttNONE:
 
              DoReserve = TRUE; 
 
              break;
 
            case mrttBypass:
 
              if (J->BypassCount > MPar[0].RsvTValue)
                DoReserve = TRUE;
 
              break;
 
            case mrttQueueTime:
 
              if ((long)(MSched.Time - J->SystemQueueTime) > MPar[0].RsvTValue)
                {
                DoReserve = TRUE;
                }
               
              break;
 
            case mrttXFactor:
 
              XFactor = ((double)(MSched.Time - J->SystemQueueTime) + J->SpecWCLimit[0]) /
                          J->SpecWCLimit[0];
 
              if (XFactor > (double)MPar[0].RsvTValue)
                {
                DoReserve = TRUE;
                }
 
              break;
 
            default:
 
              DoReserve = TRUE;
 
              break;
            }  /* END switch (MPar[0].RsvTType) */
          }    /* END else ((J->Cred.Q != NULL) && ...) */
 
        /* FUTURE WORK: */ 
 
        /* determine cost of previous backfill */
 
        /* could job start if backfill jobs were not running? */
 
        break;
 
      default:

        /* NO-OP */
 
        break;
      }  /* END switch (RsvPolicy) */
    }    /* END if (MPar[0].BFPolicy != bfNONE) */
 
  /* locate QOS group */
  
  MJobGetQOSRsvBucketIndex(J,&bindex);

  if (bindex == MMAX_QOSBUCKET)
    { 
    DoReserve = FALSE;
    }

  if ((DoReserve == TRUE) &&
      ((MSchedGetQOSRsvCount(bindex) < MPar[0].QOSBucket[bindex].RsvDepth) ||
       (ThresholdReached == TRUE) || 
       ((P != NULL) &&
        (P->ParBucketRsvDepth > 0) &&
        (MSchedGetParRsvCount(P->Index) < P->ParBucketRsvDepth))))
    {
    MDB(6,fSCHED) MLog("INFO:     attempting reservation of job '%s' into slot %d of %d in bucket %s\n",
      J->Name,
      MSchedGetQOSRsvCount(bindex),
      MPar[0].QOSBucket[bindex].RsvDepth,
      MPar[0].QOSBucket[bindex].Name);

    if (MJobReserve(
          &J,
          (LockPartition == TRUE) ? P : NULL,
          EStartTime,
          (ThresholdReached == TRUE) ? mjrQOSReserved : mjrPriority,
          FALSE) == FAILURE)
      {
      /* NOTE:  MJobReserve() may destroy J */

      MDB(2,fSCHED) MLog("WARNING:  cannot reserve priority job '%s'\n",
        ((J != NULL) && (J->Name[0] > '\1')) ? J->Name : "NULL");

      if (RsvCountRej != NULL)
        *RsvCountRej = FALSE;

      return(FAILURE);
      }

    if (J->Rsv != NULL)
      {
      J->Rsv->Priority = J->PStartPriority[(P != NULL) ? P->Index : 0];
 
      bmset(&J->Rsv->Flags,mrfPreemptible);
 
      if (ThresholdReached != TRUE)
        {
        MSchedIncrementQOSRsvCount(bindex);

        if (P != NULL)
          MSchedIncrementParRsvCount(P->Index);
        }
      }
    else
      {
      MDB(1,fCORE) MLog("ALERT:    cannot create reservation for priority job %s\n",
        J->Name);
      }
    }    /* END if ((DoReserve == TRUE) && ...) */
  else
    {
    MDB(3,fSCHED) MLog("INFO:     rsv bucket is full - no reservation created\n");

    if (RsvCountRej != NULL)
      *RsvCountRej = TRUE;

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobPReserve() */




/**
 * Create a reservation for J which should succeed regardless of any currently
 * existing reservations
 *
 * NOTE: only support single req jobs with hostlists
 *
 * @param J (I) 
 * @param P (I) [optional]
 */

int MJobForceReserve(

  mjob_t *J,
  mpar_t *P)

  {
  mnl_t  tmpFNL;
  mnl_t *MNodeList[MMAX_REQ_PER_JOB];
  mnl_t *MNodeList2[MMAX_REQ_PER_JOB];

  char  *NodeMap = NULL;

  enum MNodeAllocPolicyEnum NAPolicy;

  const char *FName = "MJobForceReserve";

  MDB(4,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((MNLIsEmpty(&J->ReqHList)) || 
      (J->Req[0] == NULL) || 
      (J->Req[1] != NULL))
    {
    /* MReqGetFNL() will return all feasible nodes UNLESS ReqHList is specified */

    return(FAILURE);
    }

  MNLInit(&tmpFNL);
  MNLMultiInit(MNodeList);

  if (MReqGetFNL(
       J,
       J->Req[0],
       (P != NULL) ? P : &MPar[0],
       NULL,
       &tmpFNL, /* O */
       NULL,
       NULL,
       MMAX_TIME,
       0,
       NULL) == FAILURE)
    {
    MNLFree(&tmpFNL);
    MNLMultiFree(MNodeList);

    return(FAILURE);
    }

  MNodeMapInit(&NodeMap);

  if (MJobSelectMNL(
        J,
        (P != NULL) ? P : &MPar[0],
        &tmpFNL,
        MNodeList,
        NodeMap,
        FALSE,
        FALSE,
        FALSE,
        NULL,
        NULL) == FAILURE)
    {
    MNLFree(&tmpFNL);
    MNLMultiFree(MNodeList);
    MUFree(&NodeMap);

    return(FAILURE);
    }

  MNLMultiInit(MNodeList2);

  NAPolicy = (J->Req[0]->NAllocPolicy != mnalNONE) ?
    J->Req[0]->NAllocPolicy:
    (P != NULL) ? P->NAllocPolicy : MPar[0].NAllocPolicy;

  if (MJobAllocMNL(
        J,
        MNodeList,
        NodeMap,
        MNodeList2,
        NAPolicy,
        MSched.Time,
        NULL,
        NULL) == FAILURE)
    {
    MNLFree(&tmpFNL);
    MUFree(&NodeMap);
    MNLMultiFree(MNodeList);
    MNLMultiFree(MNodeList2);

    return(FAILURE);
    }

  MUFree(&NodeMap);
  MNLFree(&tmpFNL);
  MNLMultiFree(MNodeList);

  if (MRsvJCreate(J,MNodeList2,MSched.Time,mjrQOSReserved,NULL,FALSE) == FAILURE)
    {
    MNLMultiFree(MNodeList2);

    return(FAILURE);
    }

  MNLMultiFree(MNodeList2);

  return(SUCCESS);
  }  /* END MJobForceReserve() */



