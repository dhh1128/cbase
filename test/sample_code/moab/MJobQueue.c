/* HEADER */

      
/**
 * @file MJobQueue.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Remove active job from active job table (MAQ).
 *
 * @param J (I)
 * @param Mode (I) [mstForce, ... ]
 *
 * @see MQueueAddAJob() - peer
 */

int MQueueRemoveAJob(

  mjob_t            *J,     /* I */
  enum MServiceType  Mode)  /* I (mstForce, ... ) */

  {
  if (J == NULL) 
    {
    return(FAILURE);
    }

  if ((Mode != mstForce) && 
      (J->State != mjsStarting) && 
      (J->State != mjsRunning))
    {
    /* job is not active - job will not be in MAQ table */

    return(SUCCESS);
    }

  MDB(8,fSTAT)
    {
    MAQDump();
    }

  /* also remove the job from the active job hash table */

  MUHTRemove(&MAQ,J->Name,NULL);

  if ((!bmisset(&J->IFlags,mjifIsTemplate)) && (MStat.ActiveJobs > 0))
    {
    MStat.ActiveJobs--;     
    }

  MDB(8,fCORE)
    {
    MAQDump();
    }

  return(SUCCESS);
  }    /* END MQueueRemoveAJob() */





/**
 * Cleanup adaptive flags/settings so job is "fresh".
 *
 * @see MJobRequeue() - parent
 *
 * @param J
 */

int MJobCleanupAdaptive(

  mjob_t *J)

  {
  if ((MJOBREQUIRESVMS(J) == FALSE) && 
      (MJOBISVMCREATEJOB(J) == FALSE) &&
      ((J->Credential.Q == NULL) || (!bmisset(&J->Credential.Q->Flags,mqfProvision))))
    {
    return(SUCCESS);
    }

  bmunset(&J->IFlags,mjifReqTimeLock);

  return(SUCCESS);
  }  /* END MJobCleanupAdaptive() */



/**
 * Requeue job.
 *
 * @see MRMJobRequeue() - child 
 *
 * @param J        (I) [modified]
 * @param JPeer [optional]
 * @param Message [optional]
 * @param EMsg [optional]
 * @param SC [optional]
 */

int MJobRequeue(

  mjob_t     *J,
  mjob_t    **JPeer,
  const char *Message,
  char       *EMsg,
  int        *SC)

  {
  const char *FName = "MJobRequeue";

  MDB(3,fSCHED) MLog("%s(%s,EMsg,SC)\n",
    FName,
    J->Name);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (bmisset(&J->IFlags,mjifRanProlog) &&
      !bmisset(&J->IFlags,mjifRanEpilog))
    {
    MJobLaunchEpilog(J);

    bmset(&J->IFlags,mjifRanEpilog);
    }

  if (MRMJobRequeue(J,JPeer,Message,EMsg,SC) == FAILURE)
    {
    /* cannot requeue job */

    return(FAILURE);
    }

  bmunset(&J->IFlags,mjifRanProlog);
  bmunset(&J->IFlags,mjifRanEpilog);

  MJobCleanupAdaptive(J);

  if (MSched.LimitedJobCP == FALSE)
    MJobTransition(J,TRUE,FALSE);

  bmunset(&J->IFlags,mjifWasCanceled);

  return(SUCCESS);
  }  /* END MJobRequeue() */





/**
 * Move job from MJobHT to MCJobHT.
 *
 * @see MCJobLoadCP() - parent - load completed job info from checkpoint
 * @see MJobProcessRemoved() - parent
 * @see MJobProcessCompleted() - parent
 *
 * NOTE: Completed job modified to prevent deallocation of memory 
 * CJ->Cred.{ACL,CL,Templates}, CJ->T, and CJ->Req[0]->ReqAttr cleared 
 *
 * NOTE: Must explicitly duplicate all dynamic job attributes which should be preserved.
 *
 * NOTE: Completed jobs purged in MQueueUpdateCJobs()
 *
 * NOTE:  CJ still evaluated further after this routine - do NOT mess it up!
 *
 * @param CJ (I) [not modified, but moved]
 */

int MQueueAddCJob(

  mjob_t *CJ) /* I (modified) */

  {
  int JobID = 0;

  const char *FName = "MQueueAddCJob";

  char NCTime[MMAX_LINE];

  MDB(4,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (CJ != NULL) ? CJ->Name : "NULL");

  if (!MJobPtrIsValid(CJ))
    {
    return(FAILURE);
    }

  /* If the scheduler limits the max completed job id that can be recorded in
     the completed job table then check to see if there is a job number in
     the job name (e.g. moab.1234) */

  if ((MSched.MaxRecordedCJobID != MMAX_INT) && 
      (MUGetJobIDFromJobName(CJ->Name,&JobID) == SUCCESS))
    {
    /* We found a job id number embedded in the name, so now make sure it
       does not exceed the limit */

    if (JobID > MSched.MaxRecordedCJobID)
      {
      MDB(3,fSTRUCT) MLog("INFO:     job '%s' not added to completed job table due to MAXRECORDEDCJOBID limit %d\n",
        CJ->Name,
        MSched.MaxRecordedCJobID);

      /*MJobTransition(J,FALSE);*/

      return(FAILURE);
      }
    }

  /* Search the completed job table for a free entry or
   * an already existing entry for this job. */

  if (MCJobTableFind(CJ->Name,NULL) == SUCCESS)
    {
    /* Found a job with the same name already in the table */

    /* why should we update the table if we found the same job? */

    /* it's possible there may be some wraparound where job 1 is in the completed table
       and we've executed so many jobs that we're back at job 1.  In this situation
       we won't replace the old job, the new job will be lost.  It is recommended that
       if we ever encounter that scenario we create a new routine and call it here. 
       Something like MQueueUpdateCJob.  We can't use this routine as it isn't
       memory safe when CJ == J */

    return(FAILURE);
    }

  MJobHT.erase(CJ->Name);

  if (CJ->DRMJID != NULL)
    MUHTRemove(&MJobDRMJIDHT,CJ->DRMJID,NULL);

  MCJobTableInsert(CJ->Name,CJ);

#if 0 /* this is not longer used */
  else
    {
    /* add new entry to hash table */
   
    J = (mjob_t *)MUCalloc(1,sizeof(mjob_t));

    MUHTAdd(&MCJobHT,CJ->Name,(void *)J,NULL,MJOBFREE);
    }

  /* call end triggers before we transfer anything over to the completed job */

  if (CJ->T != NULL)
    {
    MTListClearObject(CJ->T,FALSE);

    MUArrayListFree(CJ->T);

    MUFree((char **)&CJ->T);

    CJ->T = NULL;
    }    /* END if (CJ->T != NULL) */

  /* copy base information */

  /* NOTE:  all memory alloc'd for attributes must be freed in MQueueUpdateCJobs() */

  strcpy(J->Name,CJ->Name);
  MUStrDup(&J->AName,CJ->AName);

  MUStrDup(&J->SRMJID,CJ->SRMJID);
  MUStrDup(&J->DRMJID,CJ->DRMJID);
 
  J->State = CJ->State;
  J->EState = CJ->State;  /* this avoids seeing errors about state and estate mismatch */
  J->ATime = CJ->ATime;

  MNLClear(&J->ReqHList);  /* ReqHList can have bad info when we are reusing MCJob[] entries. */

  if (CJ->Depend != NULL)
    {
    /* dependency info required for workflow reporting */

    MJobDependCopy(CJ->Depend,&J->Depend);
    }  /* END if (CJ->Depend != NULL) */

  /* job variables required by XT systems for removing orphan partitions if
     initial request to destroy partition fails.
  */

  /* Variables will be destroyed in MJobRemove */
  MUHTCopy(&J->Variables,&CJ->Variables);

  memcpy(&J->Request,&CJ->Request,sizeof(J->Request));
  bmcopy(&J->PAL,&CJ->PAL);
  bmcopy(&J->SysPAL,&CJ->SysPAL);
  bmcopy(&J->SpecPAL,&CJ->SpecPAL);

  MUStrDup(&J->SystemID,CJ->SystemID);
  MUStrDup(&J->SystemJID,CJ->SystemJID);

  MUStrDup(&J->E.IWD,CJ->E.IWD);
  MUStrDup(&J->E.Cmd,CJ->E.Cmd);
  MUStrDup(&J->E.Error,CJ->E.Error);
  MUStrDup(&J->E.Output,CJ->E.Output);
  MUStrDup(&J->E.RMOutput,CJ->E.RMOutput);
  MUStrDup(&J->E.RMError,CJ->E.RMError);

  if (CJ->System != NULL)
    {
    J->System = (msysjob_t *)MUCalloc(1,sizeof(J->System[0]));
    MJobSystemCopy(J->System,CJ->System);
    }

  J->EffQueueDuration = CJ->EffQueueDuration;

  if (CJ->MB != NULL)
    {
    /* copy job messages into completed job buffer */

    MMBCopyAll(&J->MB,CJ->MB);
    }  /* END if (J->MB != NULL) */

  J->SRM = CJ->SRM;
  J->DRM = CJ->DRM;
  
  bmcopy(&J->IFlags,&CJ->IFlags);
  bmcopy(&J->Flags,&CJ->Flags);

  MUStrDup(&J->MasterHostName,CJ->MasterHostName);
  J->CompletionCode = CJ->CompletionCode;
  J->E.UMask = CJ->E.UMask;

  /* transfer data-staging information */

  if (CJ->DS != NULL)
    {
    MDSCreate(&J->DS);

    MMovePtr((char **)&CJ->DS->SIData,(char **)&J->DS->SIData); /* can lose orignial pointer? */

    MMovePtr((char **)&CJ->DS->SOData,(char **)&J->DS->SOData);
    }

  MJGroupDup(CJ->JGroup,J);

  /* transfer VM information */

  if (CJ->Req[0] == NULL)
    {
    /* if CJ doesn't have a req (it is missing from the XML),
     * create an empty one */

    if (MReqCreate(CJ,NULL,NULL,TRUE) == FAILURE)
      {
      MJobDestroy(&J);

      return(FAILURE);      
      }
    }

  /* only support single req */

  for (rqindex = 0; (rqindex < MMAX_REQ_PER_JOB) && (CJ->Req[rqindex] != NULL); rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      {
      /* allocate new req structure */

      MReqCreate(J,CJ->Req[rqindex],NULL,TRUE);
      }    /* END if (J->Req[rqindex] == NULL) */
    else
      {
      RQ = J->Req[rqindex];

      /* duplicate the request -- don't memcpy -- causes duplicate free problems */

      MReqDuplicate(RQ,CJ->Req[rqindex]);
      }  /* END else (J->Req[rqindex] == NULL) */

    RQ = J->Req[rqindex];

    RQ->OptReq = NULL;

    if (CJ->Req[rqindex]->XURes != NULL)
      {
      J->Req[rqindex]->XURes = NULL;
      }

    CJ->Req[rqindex]->ReqAttr = NULL;

    if ((J->DRM != NULL) && (RQ != NULL))
      RQ->RMIndex = J->DRM->Index;

    if (MNLGetNodeAtIndex(&CJ->Req[rqindex]->NodeList,0,NULL) == SUCCESS)
      {
      /* preserve allocated hostlist */

      if (!MNLIsEmpty(&CJ->Req[rqindex]->NodeList))
        {
        MNLCopy(&J->Req[rqindex]->NodeList,&CJ->Req[rqindex]->NodeList);

        if (MSched.JobCTruncateNLCP == TRUE)
          {
          /* Make the second entry a list termination entry */

          MNLTerminateAtIndex(&J->Req[rqindex]->NodeList,1);
          }
        }
      }    /* END if (MNLGetNodeAtIndex(...) == SUCCESS) */
    }    /* END for (rindex... */

  /* credentials */

  /* J->Cred.ACL/CL/Templates can already have info when the completed job is being
   * loaded from checkpoint. So they need to be free'ed' before memcpy'ing' newly 
   * malloc'ed' memory for the checkpointed job. */

  MACLFreeList(&J->Cred.CL);
  MACLFreeList(&J->Cred.ACL);
  MUFree((char **)&J->Cred.Templates);

  MJobCopyCL(J,CJ);

  /* resources */

  /* NOTE:  do not preserve completed job taskmap info */

  if (J->TaskMap != NULL)
    MUFree((char **)&J->TaskMap);

  J->TaskMap = (int *)MUCalloc(1,sizeof(int));
  J->TaskMap[0]     = -1;
  J->TaskMapSize    = 1;

  J->WCLimit        = CJ->WCLimit;
  J->SpecWCLimit[0] = CJ->SpecWCLimit[0];
  J->CPULimit       = CJ->CPULimit;

  J->C.TotalProcCount = CJ->C.TotalProcCount;

  J->Request.NC     = CJ->Request.NC;

  if (!MNLIsEmpty(&CJ->NodeList))
    {
    MNLCopy(&J->NodeList,&CJ->NodeList);

    if (MSched.JobCTruncateNLCP == TRUE)
      {
      /* Make the second entry a list termination entry */

      MNLTerminateAtIndex(&J->NodeList,1);
      }
    }    /* END if (CJ->NodeList != NULL) */

  /* usage */

  J->PSDedicated = CJ->PSDedicated;
  J->PSUtilized  = CJ->PSUtilized;

  J->AWallTime   = CJ->AWallTime;

  /* history */

  J->SubmitTime       = CJ->SubmitTime;
  J->SystemQueueTime  = CJ->SystemQueueTime;
  J->StartTime        = CJ->StartTime;
#endif

  if (CJ->CompletionTime <= 0)
    {
    CJ->CompletionTime = MSched.Time;
    }

  MOCheckpoint(mxoxCJob,(void *)CJ,FALSE,NULL);

  MDB(3,fCORE) MLog("INFO:     added completed job '%s'\n",
    CJ->Name);

  bmset(&CJ->IFlags,mjifCompletedJob);

  MUSNCTime(&CJ->CompletionTime,NCTime);

  MDB(3,fCORE) MLog("INFO:     added completed job '%s', Job Completion Time %s\n",
    CJ->Name,
    NCTime);

  MJobTransition(CJ,FALSE,FALSE);

  return(SUCCESS);
  }  /* END MQueueAddCJob() */





/**
 * Purge expired completed jobs.
 *
 * Jobs that have NOT been reported to a required master (i.e. a job is from a remote source
 * peer) will exist only 7 days in Moab's completed job queue. Once the job has been
 * reported to the source peer, the job is then eligible for purging. This time limit is
 * not currently configurable, but could be in the future.
 *
 * @see struct msched_t.JobCPurgeTime - default purge time for completed jobs
 *
 * @see MQueueAddCJob() - peer
 * @see MReqDestroy() - child
 */

int MQueueUpdateCJobs(void)

  {
  mjob_t *J;

  job_iter JTI;
  char DString[MMAX_LINE];

  const char *FName = "MQueueUpdateCJobs";

  MDB(4,fSTRUCT) MLog("%s()\n",
    FName);

  MCJobIterInit(&JTI);

  while (MCJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (J->CompletionTime + MSched.JobCPurgeTime > MSched.Time)
      {
      /* completed job info still required - do not purge */

      continue;
      }

    MULToDString(&J->CompletionTime,DString);

    MDB(3,fCORE) MLog("INFO:     removing used memory for completed job '%s', job completion time %s",
      J->Name,
      DString);

    /* before we purge the job, we need to mark that we can safely remove the job from TORQUE
     * because Moab has obviously read in the job and no longer needs it */

    if ((J->DestinationRM != NULL) &&
        (J->DestinationRM->Type == mrmtPBS))
      {
      /* record that this completed job was successfully read-in (and purged) so we can tell TORQUE
       * to remove the job from its memory */

      mrm_t *R = J->DestinationRM;

      R->PBS.LastCPCompletedJobTime = MAX(J->RMCompletionTime,R->PBS.LastCPCompletedJobTime);
      }

    MORemoveCheckpoint(mxoxCJob,J->Name,0,NULL);

    if (J->System != NULL)
      MJobFreeSystemStruct(J);

    MCacheRemoveJob(J->Name);

    MCJobHT.erase(J->Name);

    MJobDestroy(&J);
    }  /* END for (jindex) */

  return(SUCCESS);
  }  /* END MQueueUpdateCJobs() */


  


/**
 * Qsort comparision algorithm - sort mjob_t array earliest job end first.
 *
 * @param A (I)
 * @param B (I)
 */

int __MJobEndCompLToH(

  mjob_t **A,  /* I */
  mjob_t **B)  /* I */

  {
  /* order low to high */

  return((*A)->RemainingTime - (*B)->RemainingTime);
  }  /* END __MJobEndCompLToH() */





/**
 * Determine if 'cost' of requeueing is worth improved utilization.
 *
 * @param J (I) preemptor job 
 * @param P (I) partition 
 * @param DoPreempt (I) If TRUE, routine will preempt target jobs
 */

mbool_t MJobShouldRequeue(

  mjob_t  *J,         /* I */
  mpar_t  *P,
  mbool_t  DoPreempt) /* I */
  
  {
  long CyclesRequeued;
  long CyclesUnused;
  long CyclesLost;

  long NowValue;  /* lost cycles associated with requeueing all jobs now */

  long NLPH;  /* nodelist proc-hours */

  long RunTime;
  long TC;       /* job taskcount */
  long TTC;      /* total tasks in node list */
  long ATC;      /* total active job list taskcount */

  int  jindex;

  long Delta;
  long PrevTime;

  mjob_t *JList[MMAX_JOB];
  int     JCount;

  enum MPreemptPolicyEnum PPolicy;

  double RRatio;

  int pindex;

  const char *FName = "MJobShouldRequeue";

  MDB(3,fSCHED) MLog("%s(%s,%s,%s)\n",
    FName,
    J->Name,
    (P != NULL) ? P->Name : "NULL",
    MBool[DoPreempt]);

  /* assumes/requires dedicated nodes */

  if ((J == NULL) || 
      (J->Rsv == NULL) || 
      (MNLIsEmpty(&J->Rsv->NL)) || 
      (J->Credential.Q == NULL) ||
      !bmisset(&J->Credential.Q->Flags,mqfPreemptorX))
    {
    /* NOTE:  currently only checks requeue status if job has reservation */

    return(FALSE);
    }

  if ((J->Credential.Q != NULL) && (J->Credential.Q->PreemptPolicy != mppNONE))
    PPolicy = J->Credential.Q->PreemptPolicy;
  else
    PPolicy = MSched.PreemptPolicy;

  if (PPolicy != mppRequeue)
    {
    return(FALSE);
    }

  /* build list of jobs on affected nodes */

  if (MNLGetJobList(
        &J->Rsv->NL,
        &NLPH,
        JList,    /* O */
        &JCount,  /* O */
        MMAX_JOB) == FAILURE)
    {
    return(FALSE);
    }

  /* sort JList into earliest completion first order */

  qsort(
    (void *)JList,
    JCount,
    sizeof(mjob_t *),
    (int(*)(const void *,const void *))__MJobEndCompLToH);

  CyclesRequeued = 0;

  ATC = 0;

  pindex = (P != NULL) ? P->Index : 0;

  for (jindex = 0;JList[jindex] != NULL;jindex++)
    {
    if (!bmisset(&JList[jindex]->Flags,mjfPreemptee) || 
        !bmisset(&JList[jindex]->Flags,mjfRestartable) ||
       (JList[jindex]->PStartPriority[pindex] > J->PStartPriority[pindex]))
      {
      /* job cannot requeue */

      MDB(5,fSCHED) MLog("INFO:     job %s cannot requeue, blocked by job %s (%s, %s, priority=%ld) partition %s\n",
        J->Name,
        JList[jindex]->Name,
        bmisset(&JList[jindex]->Flags,mjfPreemptee) ? "preemptee" : "non-preemptee",
        bmisset(&JList[jindex]->Flags,mjfRestartable) ? "restartable" : "non-restartable",
        JList[jindex]->PStartPriority[pindex],
        (P != NULL) ? P->Name : MPar[0].Name);

      return(FALSE);
      }

    RunTime = MSched.Time - JList[jindex]->StartTime;
    TC      = (long)JList[jindex]->TotalProcCount;

    ATC    += TC;

    CyclesRequeued += TC * RunTime;
    }  /* END for (nindex) */

  TTC = J->TotalProcCount;

  CyclesUnused = TTC * (J->Rsv->StartTime - MSched.Time) - NLPH;

  RRatio = (MSched.RequeueRecoveryRatio > 0.0) ? MSched.RequeueRecoveryRatio : 1.0;

  MDB(5,fSCHED) MLog("INFO:     jobcount: %d  startduration: %ld  completedcycles: %ld  wasted cycles: %ld\n",
    JCount,
    J->Rsv->StartTime - MSched.Time,
    CyclesRequeued,
    CyclesUnused);

  /*
  Lost Cycles with no action = CyclesUnused;
  Lost cycles with immediate action = CyclesRequeued;
  */

  if (CyclesUnused < CyclesRequeued * RRatio)
    {
    /* do not take action at this time */

    return(FALSE);
    }

  /* enough gain to warrant requeue */

  /* NOTE:  goal is to minimize CyclesLost */

  NowValue = (long)(CyclesRequeued * RRatio);

  /* walk through all job time slots and determine if better requeue time exists */

  CyclesLost = CyclesRequeued;

  PrevTime = 0;

  for (jindex = 0;jindex < JCount;jindex++)
    {
    Delta = JList[jindex]->RemainingTime - PrevTime;
    TC    = JList[jindex]->TotalProcCount;

    /* cycles gained because specified job will run to completion and will not be requeued */

    CyclesLost -= 
       (long)(TC * JList[jindex]->WCLimit * RRatio); 

    /* cycles lost associated with idle resources AND with additional used cycles which will
       be requeued */

    ATC -= TC;

    CyclesLost += 
       (TTC - TC) * Delta;    

    PrevTime += Delta;

    if (CyclesLost < NowValue)
      {
      /* better decision can be made in the future - take action then */

      MDB(5,fSCHED) MLog("INFO:     in %ld seconds, %d jobs can be preempted with %ld fewer wasted cycles (better)\n",
        PrevTime,
        JCount - (jindex + 1),
        NowValue - CyclesLost);

      return(FALSE);
      }

    MDB(5,fSCHED) MLog("INFO:     in %ld seconds, %d jobs can be preempted with %ld wasted cycles (worse)\n",
      PrevTime, 
      JCount - (jindex + 1),
      CyclesLost);
    }  /* END for (rindex) */

  if (DoPreempt == TRUE)
    {
    for (jindex = 0;JList[jindex] != NULL;jindex++)
      {
      if (MJobPreempt(
            JList[jindex],
            J,
            NULL,
            NULL,
            "priority job",
            mppRequeue,
            TRUE,
            NULL,
            NULL,
            NULL) == FAILURE)
        {
        return(FALSE);
        }
      }  /* END for (jindex) */
    }

  return(TRUE);
  }  /* END MJobShouldRequeue() */
/* END MJobQueue.c */
