/* HEADER */

/**
 * @file MQueue.c
 *
 * Moab Queue
 */
        
/* Contains:                                 *
 *  int MQueueInitialize(Q,QName)            *
 *  int MQueuePrioritizeJobs(Q,JobIndex)     *
 *  int MQueueScheduleIJobs(Q,SP)            *
 *                                           */

/* NOTE:

 * int MQueueSelectJobs() located in MPolicy.c    *
 * int MQueueSelectAllJobs() located in MPolicy.c *
 * int MQueueAddCJob() located in MJob.c          *
 * int MQueueUpdateCJobs() located in MJob.c      */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

extern mbool_t InHardPolicySchedulingCycle;

/* local prototypes */

int __MBFJListFromArray(mjob_t **,mjob_t **);
void __MQueuePrintQueueWithPriority(mjob_t **,int,int);

/* END local prototypes */

int __MQueueSortCmp(

  mjob_t **JobA,
  mjob_t **JobB)

  {
  return(strverscmp((*JobA)->Name,(*JobB)->Name));
  }  /* END __MQueueSortCmp() */




/**
 * Calculate 'start' priority for either Q (if HaveQ) or ALL jobs.
 *
 * @see MQueueSelectAllJobs() - parent 
 *
 * @param Q (I/O) hi-low priority sorted array of job indexes
 * @param HaveQ (whether or not to prioritize Q or All jobs)
 * @param P [I] [optional] partition 
 */

int MQueuePrioritizeJobs(

  mjob_t **Q,
  mbool_t  HaveQ,
  const mpar_t  *P)

  {
  double  tmpD;
  int     jindex;

  long    MaxIdleStartPriority = 0;

  mjob_t *J;

  const char *FName = "MQueuePrioritizeJobs";

  int pindex = (P == NULL) ? 0 : P->Index;

  MDB(7,fSCHED) MLog("%s(Q,JobIndex,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL");

  if (Q == NULL)
    {
    return(FAILURE);
    }

  if (HaveQ == TRUE)
    {
    /* just prioritize Q, not ALL jobs */

    for (jindex = 0;Q[jindex] != NULL;jindex++) 
      { 
      J = Q[jindex]; 

      MJobCalcStartPriority(
        J,
        P,
        &tmpD,
        mpdJob,
        NULL,
        0,
        mfmHuman,
        FALSE);
 
      MJobSetStartPriority(
        J,
        pindex,
        (long)tmpD);

      if (MJOBISACTIVE(J))
        {
        int PIndex = 0;

        if (J->Req[0] != NULL)
          PIndex = J->Req[0]->PtIndex;

        MJobGetRunPriority(J,PIndex,&tmpD,NULL,NULL);
 
        J->RunPriority = (long)tmpD;
        }
      else 
        {
        MaxIdleStartPriority = MAX(MaxIdleStartPriority,J->PStartPriority[pindex]);
        } /* END else (MJOBISACTIVE(J)) */ 
      } /* END for (jindex = 0;JobIndex[jindex] != -1;jindex++) */
    } /* END if Q == NULL .... */
  else
    {
    /* prioritize ALL jobs */

    job_iter JTI;

    MJobIterInit(&JTI);

    jindex = 0;

    while (MJobTableIterate(&JTI,&J) == SUCCESS) 
      {
      if ((P != NULL) && (P->Index != 0) && (!bmisset(&J->Flags,mjfNoResources)))
        {
        /* skip jobs not in the partition we are working on */

        if (!bmisset(&J->PAL,P->Index))
          continue;
        }

      MJobCalcStartPriority(
        J,
        P,
        &tmpD,
        mpdJob,
        NULL,
        0,
        mfmHuman,
        FALSE);

      MJobSetStartPriority(
        J,
        pindex,
        (long)tmpD);

      if (MJOBISACTIVE(J) == TRUE)
        {
        int PIndex = 0;

        if (J->Req[0] != NULL)
          PIndex = J->Req[0]->PtIndex;

        MJobGetRunPriority(J,PIndex,&tmpD,NULL,NULL);

        J->RunPriority = (long)tmpD;
        }
      else
        {
        MaxIdleStartPriority = MAX(MaxIdleStartPriority,J->PStartPriority[pindex]);
        }

      Q[jindex++] = J;
      }
    }  /* END for (J) */

  if (jindex > 1)
    {      
    MDB(7,fSCHED) MLog("INFO:    Job Table BEFORE prioritization - partition (%s)\n",
       (P == NULL) ? "" : P->Name);

    MDB(7,fSCHED) __MQueuePrintQueueWithPriority(Q,jindex,7);

    /* note that the J->StartPriority was set earlier in this routine
       according to the specified partition (we do not need to use J->PStartPriority[pindex]
       since it will be the same value) */

    qsort(
      (void *)&Q[0],
      jindex,
      sizeof(Q[0]),
      (int(*)(const void *,const void *))MJobStartPrioComp);

    MDB(7,fSCHED) MLog("INFO:    Job Table AFTER prioritization - partition (%s)\n",
      (P == NULL) ? "" : P->Name);

    MDB(7,fSCHED) __MQueuePrintQueueWithPriority(Q,jindex,7);
    }

  Q[jindex] = NULL;          

#if !defined(__MALWAYSPREEMPT)
  if (MPar[0].BFPolicy == mbfPreempt)
    {
    /* adjust preemption backfill status */

    for (jindex = 0;Q[jindex] != NULL;jindex++)
      {
      J = Q[jindex];

      if ((MJOBISACTIVE(J) == TRUE) && 
          (J->PStartPriority[pindex] >= MaxIdleStartPriority) &&
          bmisset(&J->Flags,mjfBackfill) &&
          bmisset(&J->Flags,mjfPreemptee))
        {
        bmunset(&J->SpecFlags,mjfPreemptee);
     
        MJobUpdateFlags(J);

        MDB(2,fCORE) MLog("INFO:     backfill job '%s' no longer preemptible (%ld > %ld) in partition %s\n",
          J->Name,
          J->PStartPriority[pindex],
          MaxIdleStartPriority,
          MPar[pindex].Name);
        }
      }
    }  /* END if (MPar[0].BFPolicy == mbfPreempt) */
#endif /* !defined(__MALWAYSPREEMPT) */

  return(SUCCESS);
  }  /* END MQueuePrioritizeJobs() */





/**
 * Prints the job table with the start priorities to the log file.
 *   
 * @param Jobs (I) The the array of indexes into the MJob array.
 * @param JSize (I) Size of the job array.
 * @param LogLevel (I) The loglevel to print to the log at.
 */

void __MQueuePrintQueueWithPriority(

  mjob_t **Jobs,       /* I */
  int      JSize,      /* I */
  int      LogLevel)   /* I */

  {
  mjob_t *J;

  int index  = 0;

  while (index < JSize)
    {
    J = Jobs[index];

    MDB(LogLevel,fSCHED) MLog("INFO:     '%s' StartPriority: %ld\n",
      J->Name,
      J->CurrentStartPriority);

    index++;
    }  /* END while () */

  return;
  }  /* END __MQueuePrintQueueWithPriority */





/**
 *
 *
 * @param Queue (I)
 */

int MOQueueInitialize(

  mjob_t **Queue)  /* I */

  {
  if (Queue == NULL)
    {
    return(FAILURE);
    }

  Queue[0] = NULL;
  
  return(SUCCESS);
  }  /* END MOQueueInitialize() */




/**
 *
 *
 * @param Queue (I) [modified]
 * @param ModifyStats (I)
 */

int MOQueueDestroy(

  mjob_t **Queue,        /* I (modified) */
  mbool_t  ModifyStats)  /* I */

  {
  int jindex;

  mjob_t *J;

  if (Queue == NULL)
    {
    return(FAILURE);
    }

  if (ModifyStats == TRUE)
    {
    /* adjust idle job stats */            

    for (jindex = 0;Queue[jindex] != NULL;jindex++)
      {
      J = Queue[jindex];

      if ((J != NULL) && (J->State == mjsIdle))
        {
        /* remove idle job from object stats */

        MPolicyAdjustUsage(J,NULL,NULL,mlIdle,MPar[0].L.IdlePolicy,mxoALL,-1,NULL,NULL);             
        MPolicyAdjustUsage(J,NULL,NULL,mlIdle,NULL,mxoALL,-1,NULL,NULL); 

        MStat.IdleJobs--;
        }

      Queue[jindex] = NULL;
      }  /* END for (jindex) */
    }    /* END if (ModifyStats == TRUE) */

  Queue[0] = NULL;

  return(SUCCESS);
  }  /* END MOQueueDestroy() */





/**
 * Locate available resources, select eligible jobs, and perform backfill.
 *
 * @see MBFGetWindow() - child
 *
 * @param BFQueue (I) [terminated w/-1]
 * @param SBFPolicy (I) [optional]
 * @param PLevel (I)
 * @param AllowScaling (I)
 * @param SP (I) [optional]
 */

int MQueueBackFill(
 
  mjob_t             **BFQueue,       /* I array of job indicies (terminated w/-1) */
  enum MBFPolicyEnum   SBFPolicy,     /* I (optional) */
  enum MPolicyTypeEnum PLevel,        /* I */
  mbool_t              AllowScaling,  /* I */
  mpar_t              *SP)            /* I (optional) */
 
  {
  int BFNodeCount;
  int BFProcCount;
 
  long BFTime;
 
  long OBFTime;
 
  mnl_t BFNodeList;
 
  static mjob_t **tmpQ = NULL;
  static mjob_t **JList = NULL;
  static mjob_t **VQ = NULL;

  mjob_t *J;

  mpar_t *P;

  int    jindex;
  int    index;
  int    pindex;
  int    JC = 0;
 
  long AdjBFTime;

  double tmpD;

  mjob_t  *tmpJ = NULL;
  
  int  RCount[MMAX_TAPERNODE];

  char    TString[MMAX_LINE];
  mbool_t BFCompleted;
  mbool_t PreemptorLocated;

  enum MBFPolicyEnum BFPolicy;

  mpar_t *GP;

  int     rc;

  const char *FName = "MQueueBackFill";

  MDB(1,fSCHED) MLog("%s(BFQueue,%s,%s,%s,%s)\n",
    FName,
    MBFPolicy[SBFPolicy],
    MPolicyMode[PLevel],
    MBool[AllowScaling],
    (SP != NULL) ? SP->Name : "NULL");

  if (BFQueue == NULL)
    {
    return(FAILURE);
    }

  /* allocate dynamic heap variables */

  if ((tmpQ == NULL) &&
      (MJobListAlloc(&tmpQ) == FAILURE))
    {
    return(FAILURE);
    }

  if ((JList == NULL) &&
      (MJobListAlloc(&JList) == FAILURE))
    {
    return(FAILURE);
    }
 
  if ((MSched.MaxJobStartPerIteration >= 0) &&
      (MSched.JobStartPerIterationCounter >= MSched.MaxJobStartPerIteration))
    {
    MDB(4,fSCHED) MLog("ALERT:    backfill blocked by JOBMAXSTARTPERITERATION policy in %s (%d >= %d)\n",
      FName,
      MSched.JobStartPerIterationCounter,
      MSched.MaxJobStartPerIteration);

    return(SUCCESS); 
    }

  /* NOTE: BFQueue[] is terminated with a -1, and may include indicies to jobs */
  /*       that have been removed (i.e. check for MJob[index] == NULL) */

  GP = &MPar[0];

  BFPolicy = (SBFPolicy != mbfNONE) ? SBFPolicy : MPar[0].BFPolicy;

  if (BFPolicy == mbfNONE)
    {
    /* backfill not enabled */

    MDB(1,fSCHED) MLog("INFO:     backfill policy disabled in %s\n",
      FName);

    return(SUCCESS);
    }

  /* locate all potential backfill windows and launch specific backfill algo for each */

  MJobMakeTemp(&tmpJ);

  if (MJobSetCreds(tmpJ,ALL,ALL,ALL,0,NULL) == FAILURE)
    {
    MDB(3,fUI) MLog("INFO:     cannot setup template job creds\n");

    MJobFreeTemp(&tmpJ);

    return(FAILURE);
    }

  MQOSAdd(ALL,&tmpJ->Credential.Q);

  bmset(&tmpJ->IFlags,mjifBFJob);

  tmpJ->VMUsagePolicy = mvmupAny;

  /* tmpJ.Req[0]->DRes.GenericRes is init'd by MJobMakeTemp */

  /* if preemptible jobs exist, entire cluster must be considered eligible 
     for backfill workload */

  PreemptorLocated = FALSE;

  for (jindex = 0;BFQueue[jindex] != NULL;jindex++)
    {
    J = BFQueue[jindex];

    if ((J == NULL) || !bmisset(&J->Flags,mjfPreemptor))
      continue;

    PreemptorLocated = TRUE;

    break; 
    }  /* END for (jindex) */

  if (PreemptorLocated == FALSE)
    {
    PreemptorLocated = MRsvOwnerPreemptExists();
    }    /* END if (PreemptorLocated == FALSE) */

  if ((MPar[0].BFChunkSize > 0) && (MPar[0].BFChunkDuration > 0))
    {
    /* FIXME: This is a shortcut.  When backfill chunking is enabled MQueueSelectJobs() 
              needs to set a variable that says whether a job of size BFChunkSize or
              greater was not considered because of the size of the backfill window.
              This temporary hack forces the backfill window to be the entire cluster. */

    PreemptorLocated = TRUE;
    }

  /* process backfill windows */

  MNLInit(&BFNodeList);

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\1')
      continue;

    if (P->Name[0] == '\0')
      break;

    if ((SP != NULL) && (P != SP))
      continue;

    if (P->RM != NULL)
      {

      if (P->RM->State == mrmsDisabled)
        {
        continue;
        }

      if ((P->RM->FailIteration == MSched.Iteration) &&
          (P->RM->FailCount >= P->RM->MaxFailCount))
        {
        /* this check also happens in the loop for MBFFirstFit, __MSchedScheduleJobs */

        continue;
        }

      if (P->RM->StageThreshold > 0)
        {
        /* don't backfill if this partition has a stage-threshold set, is non-slave
           and local RM's exist */

        /* NOTE:  should handle StageThreshold >> MSched.RMPollInterval -- can't just backfill
                  arbitrarily (NYI) */

        if (!bmisset(&P->RM->Flags,mrmfSlavePeer) && 
           (MSched.ActiveLocalRMCount > 0) &&
           (SP != NULL))
          {
          continue;
          }
        }

      if ((P->RM->Type == mrmtMoab) && 
          bmisset(&P->RM->Flags,mrmfAutoStart))
        {
        /* don't backfill onto partitions that automatically start their own jobs */
        /* this is handled at the end of MQueueScheduleIJobs() */
        /* jobs previously migrated at the end of MQueueScheduleIJobs will 
           update P->DRes but not N->DRes */
        /* MBFGetWindow() does not look at P->DRes so do not backfill onto 
           remotely controlled partitions */

        continue;
        }
      }    /* END if (P->RM != NULL) */

    /* initiate OBFTime for each partition, see RT5670) */

#ifdef __MLONGESTBFWINDOWFIRST
    OBFTime = MMAX_TIME;
#else /* __MLONGESTBFWINDOWFIRST */
    OBFTime = 0;
#endif /* __MLONGESTBFWINDOWFIRST */

    tmpJ->Req[0]->PtIndex = P->Index;

    BFCompleted = FALSE;
 
    MDB(3,fUI) MLog("INFO:     backfilling partition '%s' policy level '%s'\n",
      P->Name,
      MPolicyMode[PLevel]);

    MNLClear(&BFNodeList);

    while (BFCompleted == FALSE)
      {
      if (PreemptorLocated == TRUE)
        { 
        int nindex;
        mnode_t *N;
 
        /* preemptors granted access to full cluster */

        BFNodeCount = MPar[0].UpNodes;
        BFProcCount = MPar[0].UpRes.Procs;
        BFTime      = MMAX_TIME;

        nindex = 0;

        for (index = 0;index < MSched.M[mxoNode];index++)
          {
          N = MNode[index];

          if ((N == NULL) || (N->Name[0] == '\0'))
            break;

          if (N->Name[0] == '\1')
            continue;

          MNLSetNodeAtIndex(&BFNodeList,nindex,N);
          MNLSetTCAtIndex(&BFNodeList,nindex,N->CRes.Procs);

          nindex++;
          }  /* END for (index) */

        MNLTerminateAtIndex(&BFNodeList,nindex);

        BFCompleted = TRUE;
        }  /* END if (PreemptorLocated == TRUE) */
      else
        {
        rc = MBFGetWindow(
             tmpJ,        /* I */
             OBFTime,      /* I - locate next window longer than this value */
             &BFNodeCount, /* O */
             &BFProcCount, /* O */
             &BFNodeList,   /* O */
             &BFTime,      /* O */
             FALSE,        /* I seeklong */
             NULL);

        if (rc == SUCCESS)
          {
          /* window located */

          /* NO-OP */
          }
        else
          {
          /* search for long range */

          rc = MBFGetWindow(
                 tmpJ,
                 OBFTime,
                 &BFNodeCount,
                 &BFProcCount,
                 &BFNodeList,   /* I */
                 &BFTime,
                 TRUE,         /* I seeklong */
                 NULL);
          }

        if (rc == FAILURE)
          {
          break;
          }
        }  /* END else (PreemptorLocated == TRUE) */

      MULToTString(BFTime,TString);

      MDB(3,fSCHED) MLog("INFO:     BF window obtained [%d nodes/%d procs : %s]\n",
        BFNodeCount,
        BFProcCount,
        TString);
 
      OBFTime = BFTime;

      MUNLGetMaxAVal(&BFNodeList,mnaSpeed,NULL,(void **)&tmpD);
 
      AdjBFTime = (int)((double)BFTime * tmpD);
 
      MOQueueInitialize(tmpQ);

      if (AllowScaling == TRUE)
        {
        double VWSFactor;
        long   VWMinDuration;

        VWSFactor = (P->VirtualWallTimeScalingFactor > 0.0) ?
          P->VirtualWallTimeScalingFactor :
          GP->VirtualWallTimeScalingFactor;

        if (VWSFactor <= 0.0)
          {
          /* scaling is disabled */

          MNLFree(&BFNodeList);
 
          MJobFreeTemp(&tmpJ);
 
          return(SUCCESS);
          }

        if ((VQ == NULL) &&
            (MJobListAlloc(&VQ) == FAILURE))
          {
          MNLFree(&BFNodeList);

          MJobFreeTemp(&tmpJ);

          return(FAILURE);
          }

        VWMinDuration = (P->MinVirtualWallTime > 0) ?
          P->MinVirtualWallTime :
          GP->MinVirtualWallTime;

        if (VWMinDuration < 0)
          VWMinDuration = 0;

        for (jindex = 0;BFQueue[jindex] != NULL;jindex++)
          {
          J = BFQueue[jindex];
  
          if ((J == NULL) || ((long)J->SpecWCLimit[0] < VWMinDuration))
            continue;

          /* if we have an NTR job, only block other jobs from running if they're going
             to a different partition than the NTR job.  Otherwise, let them run  */

          if (MSched.NTRJob != NULL)
            {
              if ((MSched.NTRJob == J) ||
                  (MAttrSubset(&MSched.NTRJob->PAL,&J->PAL,mclOR)))
                continue;
            }

          VQ[JC] = BFQueue[jindex];

          /* scale job walltime by virtualizing factor */

          if (J->OrigWCLimit <= 0)
            {
            J->OrigWCLimit = J->SpecWCLimit[0];

            J->VWCLimit    = (long)(J->SpecWCLimit[0] * VWSFactor);
            }

          J->SpecWCLimit[0] = J->VWCLimit;
  
          JC++;
          }  /* END for (jindex) */

        VQ[JC] = NULL;

        if (MQueueSelectJobs(
              VQ,          /* I */
              tmpQ,        /* O */
              PLevel,
              BFNodeCount,
              BFProcCount,
              AdjBFTime,
              P->Index,
              NULL,
              FALSE,
              FALSE,
              FALSE,
              TRUE,
              FALSE) == FAILURE)
          {
          MDB(5,fSCHED) MLog("INFO:     no jobs meet BF window criteria in partition %s\n",
            P->Name);

          for (jindex = 0;BFQueue[jindex] != NULL;jindex++)
            {
            J = BFQueue[jindex];

            if ((J == NULL) || (J->VWCLimit <= 0) || (J->OrigWCLimit <= 0))
              continue;

            J->SpecWCLimit[0] = J->OrigWCLimit;

            J->VWCLimit = 0;
            J->OrigWCLimit = 0;
            }  /* END for (jindex) */

          MOQueueDestroy(tmpQ,FALSE);

          continue;
          }   /* END if (MQueueSelectJobs() == FAILURE) */

        __MBFJListFromArray(JList,tmpQ);

        /* virtual time support requires MBFFirstFit */

        MBFFirstFit(
          JList,       /* I (scaled to virtual time) */
          PLevel,
          &BFNodeList,
          BFTime,
          BFNodeCount,
          BFProcCount,
          P);

        /* restore idle jobs */

        for (jindex = 0;VQ[jindex] != NULL;jindex++)
          {
          J = VQ[jindex];

          if (J->OrigWCLimit <= 0)
            continue;

          if (MJOBISACTIVE(J))
            continue;

          J->SpecWCLimit[0] = J->OrigWCLimit;
          J->OrigWCLimit    = 0;
          J->VWCLimit       = 0;
          }    /* END for (jindex) */

        continue;
        }    /* END if (AllowScaling == TRUE) */
      else
        {
        /* if we have an NTR job, block all other jobs from running unless
           they're going to a different partition.  The below logic is for
           non scalable jobs (AllowScaling == FALSE).  Above there is similar
           logic for jobs that are scalable.  */
  
          if ((MSched.NTRJob != NULL) && 
              (MSched.NTRJob->Rsv != NULL) &&
              (MSched.NTRJob->Rsv->PtIndex == P->Index))
          {
          if ((VQ == NULL) &&
              (MJobListAlloc(&VQ) == FAILURE))
            {
            MNLFree(&BFNodeList);
  
            MJobFreeTemp(&tmpJ);
  
            return(FAILURE);
            }
  
          for (jindex = 0;BFQueue[jindex] != NULL;jindex++)
            {
            J = BFQueue[jindex];
  
            if (J == NULL)
              continue;
            
            if ((MSched.NTRJob == J) ||
                (MAttrSubset(&MSched.NTRJob->PAL,&J->PAL,mclOR)))
              continue;
    
            VQ[JC] = BFQueue[jindex];
            JC++;
            }
    
          VQ[JC] = NULL;
  
          if (MQueueSelectJobs(
                VQ,        /* I - list of all idle/eligible jobs */
                tmpQ,           /* O */
                PLevel,
                BFNodeCount,
                BFProcCount,
                AdjBFTime,
                P->Index,
                RCount,
                FALSE,
                FALSE,
                FALSE,
                TRUE,
                FALSE) == FAILURE)
            {
            MDB(5,fSCHED) MLog("INFO:     no jobs meet BF window criteria in partition %s\n",
              P->Name);
     
            MOQueueDestroy(tmpQ,FALSE);
     
            continue;
            }   /* END if (MQueueSelectJobs() == FAILURE) */
          }
        }

      if (MQueueSelectJobs(
            BFQueue,        /* I - list of all idle/eligible jobs */
            tmpQ,           /* O */
            PLevel,
            BFNodeCount,
            BFProcCount,
            AdjBFTime,
            P->Index,
            RCount,
            FALSE,
            FALSE,
            FALSE,
            TRUE,
            FALSE) == FAILURE)
        {
        MDB(5,fSCHED) MLog("INFO:     no jobs meet BF window criteria in partition %s\n",
          P->Name);
 
        MOQueueDestroy(tmpQ,FALSE);
 
        continue;
        }   /* END if (MQueueSelectJobs() == FAILURE) */

      /* remove ineligible jobs: reserved jobs, nobf QOS jobs, etc */

      if (tmpQ[0] == NULL)
        {
        MOQueueDestroy(tmpQ,FALSE);            

        continue;
        }

      __MBFJListFromArray(JList,tmpQ);
 
      switch (BFPolicy)
        {
        case mbfPreempt:
  
          MBFPreempt(
            JList,
            PLevel,
            &BFNodeList,
            BFTime,
            BFNodeCount,
            BFProcCount,
            P); 
 
          break;
 
        case mbfFirstFit:

          MBFFirstFit(
            JList,
            PLevel,
            &BFNodeList,
            BFTime,
            BFNodeCount,
            BFProcCount,
            P);
 
          break;
 
        case mbfBestFit:

          MBFBestFit(
            JList,
            PLevel,
            &BFNodeList,
            BFTime,
            BFNodeCount,
            BFProcCount,
            P);
  
          break;
 
        case mbfGreedy:

          MBFGreedy(
            JList,
            PLevel,
            &BFNodeList,
            BFTime,
            BFNodeCount,
            BFProcCount,
            P); 
 
          break;

        case mbfNONE:

          MNLFree(&BFNodeList);

          MJobFreeTemp(&tmpJ);
 
          return(SUCCESS);

          /*NOTREACHED*/

          break;
 
        default:

          /* NOT HANDLED */

          MDB(0,fSCHED) MLog("ERROR:    unexpected backfill policy %d (using %s)\n",
            BFPolicy,
            MBFPolicy[MDEF_BFPOLICY]);

          BFPolicy = MDEF_BFPOLICY; 
 
          MBFFirstFit(
            JList,
            PLevel,
            &BFNodeList,
            BFTime,
            BFNodeCount,
            BFProcCount,
            P);
  
          break;
        } /* END switch (BFPolicy) */
 
      MOQueueDestroy(tmpQ,FALSE);
      }   /* END while (MBFGetWindow() == SUCCESS) */
    }     /* END for (pindex) */
 
  MJobFreeTemp(&tmpJ);

  MNLFree(&BFNodeList);

  return(SUCCESS);
  }  /* END MQueueBackFill() */





/**
 * Schedule idle/eligible jobs in priority order.
 *
 * @see MQueueScheduleSJobs() - peer
 * @see MQueueScheduleRJobs() - peer
 * @see MSchedProcessJobs() - parent
 *
 * NOTE:  create priority reservations inline with RESERVATIONPOLICY
 *        and RESERVATIONDEPTH
 *
 * @param Q (I) [terminated w/-1]
 * @param SP (I) [optional]
 */

int MQueueScheduleIJobs(
 
  mjob_t **Q,   /* I (terminated w/-1) */
  mpar_t  *SP)  /* I (optional) */
 
  {
  int      jindex;
 
  static mnl_t *MNodeList[MMAX_REQ_PER_JOB];
  static mnl_t *BestMNodeList[MMAX_REQ_PER_JOB]; /* used for malleable jobs */
  static mnl_t *tmpMNodeList[MMAX_REQ_PER_JOB];  /* used for node sets */
 
  static mbool_t Init = FALSE;

  int      sindex;
 
  mjob_t  *J;
  mreq_t  *RQ;
  mpar_t  *P;
  mpar_t *PList[MMAX_PAR + 1];

  int      rqindex;
 
  int      SchedCount;
 
  static char *NodeMap = NULL;
  static char *BestNodeMap = NULL;
 
  int      BestSIndex;
  int      BestSValue;

  mbool_t  RsvCountRej;
 
  mbool_t  IdleJobFound;  /* eligible job found which cannot start and
                             cannot receive rsv due to QoS rsv depth */

                          /* NOTE: once idle job is found, this function will
                                   no longer start jobs, but will create prio
                                   rsv's until all QoS rsv buckets are full */
  int      pindex;

  mbool_t  FeasibilityChecked;
  int      FeasibleLocalParCount;

  mbool_t  PReq;

  long     tmpL;

  mnl_t tmpNL;

  mulong   Now = 0;
  mpar_t  *BestRemoteP = NULL;
 
  mbool_t  AllowPreemption;
  mbool_t  IsMalleableJob;
 
  mulong   DefaultSpecWCLimit;
 
  mrsv_t **AdvRsvTable = NULL;     /* allocated, 16 slots at a time */
  int      AdvRsvTableSize = 0;    /* current size of table */

  int      DefTC[MMAX_REQ_PER_JOB];

  int      tmpSC;

  enum MNodeAllocPolicyEnum NAPolicy;

  mbool_t IsNewJobRsv;

  char     EMsg[MMAX_LINE];

  const char *FName = "MQueueScheduleIJobs";

  mbitmap_t BM;

  MDB(2,fSCHED) MLog("%s(Q,%s)\n",
    FName,
    (SP != NULL) ? SP->Name : "NULL");

  if (Q == NULL)
    {
    return(FAILURE);
    }

  bmset(&BM,mlActive);

  /*
     NOTE: 
 
     need a new job metric.  must find schedule with best completion
     times for most highest priority jobs.  NOTE:  this will significantly
     affect current resource weightings.  Do we weight by job size, ie
     V = P * TC * f(Tc)?
 
   */

  /* NOTE:  For each job, must determine earliest reserved start time in any 
            partition and earliest possible start time in current partition 
            (due to data staging and job staging requirements)

            Also, for multi-partition environment, should preclude evaluating
            every job every iteration unless local scheduler load is low and 
            remote scheduler load is low 

            Scheduler should determine load based on percentage of 
            POLLINTERVAL dedicated to RM query/control + scheduling 
  */

  if (Q[0] == NULL)
    {
    MDB(2,fSCHED) MLog("INFO:     no jobs in queue\n");
 
    return(FAILURE);
    }
 
  if (Init == FALSE)
    {
    MNodeMapInit(&NodeMap);
    MNodeMapInit(&BestNodeMap);

    MNLMultiInit(MNodeList);
    MNLMultiInit(BestMNodeList);
    MNLMultiInit(tmpMNodeList);
 
    Init = TRUE;
    }

  MNLInit(&tmpNL);

  IdleJobFound = FALSE;
 
  SchedCount = 0;

  /* attempt to start all feasible jobs */
  /* jobs listed in priority FIFO order */
  /* reserved jobs included in list     */
 
  for (jindex = 0;Q[jindex] != NULL;jindex++)
    {
    IsNewJobRsv = FALSE;
    FeasibilityChecked = FALSE;

    /* check to see if the previous job is set as NTR (next to run).  If so, don't try to run
        any other jobs after this so the NTR job will be sure to run next */

    if ((jindex > 0) && 
        (Q[jindex-1]->Rsv != NULL) &&
        (Q[jindex-1]->Credential.Q != NULL) && 
        (bmisset(&Q[jindex-1]->Credential.Q->Flags,mqfNTR)))
      {
      MDB(2,fSCHED) MLog("INFO:     found NTR job - stopping idle job scheduling\n");

      MNLFree(&tmpNL);

      if (AdvRsvTable != NULL)
        MUFree((char **)&AdvRsvTable);

      return(SUCCESS);
      }

    J = Q[jindex];

    if (!MJobPtrIsValid(J))
      continue;

    /* if we have an NTRJob set then don't schedule anything else until it's running */

    if ((MSched.NTRJob != NULL) && (MSched.NTRJob != J))
      {
      continue;
      }
 
    MDBO(2,J,fSCHED) MLog("INFO:     checking idle job '%s' (priority: %ld) partition %s\n",
      J->Name,
      J->PStartPriority[(SP == NULL) ? 0 : SP->Index],
      MPar[(SP == NULL) ? 0 : SP->Index].Name);

    if ((J->Depend != NULL) &&
        (J->Depend->Type == mdJobSyncMaster))
      {
      if (MJobCombine(J,&J) == FAILURE)
        {
        MDBO(7,J,fSCHED) MLog("INFO:     job '%s' dependency exceeds system limits (synccount)\n", 
          J->Name);
        
        continue;
        }
      }

    if ((J->EState == mjsStarting) ||
        (J->State == mjsStarting) ||
        (J->EState == mjsRunning) ||
        (J->State == mjsRunning) ||
        (J->SubState == mjsstMigrated) ||
        (J->State == mjsSuspended))
      {
      /* job is newly started, active or migrated */

      /* job started in MQueueScheduleRJobs() */
 
      MDBO(7,J,fSCHED) MLog("INFO:     job '%s' not considered for prio scheduling (State/EState)\n", 
        J->Name);
 
      continue;
      }  /* END if ((J->EState == mjsStarting) || ...) */

    if ((bmisset(&J->Flags,mjfClusterLock) && bmisset(&J->Flags,mjfNoRMStart)) || /* submitted to cluster */
        (MJOBWASMIGRATEDTOREMOTEMOAB(J))) /* submitted to head */
      {
      /* job cannot be migrated/started - skip it */
      
      MDB(7,fSCHED) MLog("INFO:     job %s is being scheduled by another system\n",
        J->Name);

      continue;
      }

    if (bmisset(&J->IFlags,mjifWaitForTrigLaunch))
      {
      /* job isn't eligible for starting until a trigger on it
       * has launched */

      continue;
      }

    if (bmisset(&J->IFlags,mjifRunAlways))
      {
      MJobForceStart(J);

      continue;
      }

    if (bmisset(&J->IFlags,mjifRunNow))
      {
      MJobRunNow(J);

      continue;
      }

    if (bmisset(&J->IFlags,mjifBillingReserveFailure))
      {
      /* Billing failed this iteration, wait until next iteration */

      continue;
      }

    if (MJobIsPreempting(J) == TRUE)
      {
      /* job is locked */
          
      MDB(7,fSCHED) MLog("INFO:     job '%s' has preempted and is retrying reservation\n",
          J->Name);

      continue;
      }

    /* gather remote peer resource availability (non-slaves) or
     * PARALLOCATIONPOLICY is set. */

    if (SP == NULL)
      {
      /* populate PList */
      for (pindex=0;pindex < MMAX_PAR;pindex++)
        {
        P = &MPar[pindex];

        if (P->Name[0] == '\0')
          break;

        if (P->Name[0] == '\1')
          continue;

        PList[pindex] = P;
        }

      PList[pindex] = NULL; /* terminate the list */

      MParPrioritizeList(PList,J);
      }
    else
      {
      PList[0] = SP;
      PList[1] = NULL;
      }

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (PList[pindex] == NULL)
        break;

      if ((PList[pindex]->RM != NULL) &&
          (PList[pindex]->RM->Type == mrmtMoab) &&
          (!bmisset(&PList[pindex]->RM->Flags,mrmfSlavePeer)))
        {
        BestRemoteP = PList[pindex];

        break;
        }
      }

    FeasibleLocalParCount = 0;

    /* examine local partitions */

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      mbool_t SkipIdleJobWithRsv;

      P = PList[pindex];

      if (P == NULL)
        break;

      MDBO(7,J,fSCHED) MLog("INFO:     checking job '%s' on local partition '%s'\n",
        J->Name,
        P->Name);

      if ((P->RM != NULL) &&
          (P->RM->State == mrmsDisabled))
        {
        MDBO(7,J,fSCHED) MLog("INFO:     Partition '%s' is disabled\n",
          P->Name);

        continue;
        }

      /* NYI: check CoAlloc flag and make sure we are using MPar[0] */

      if (P->ConfigNodes == 0)
        {
        /* partition is empty */

        continue;
        }

      if (P->Index > 0)
        {
        if (bmisset(&J->CurrentPAL,P->Index) == FALSE)
          {
          /* job cannot access partition */

          continue;
          }

        if ((P->RM != NULL) &&
            (P->RM->Type == mrmtMoab) &&
            !bmisset(&P->RM->Flags,mrmfSlavePeer))
          {
          /* NOTE:  should we add exception here to allow jobs with
                    bmisset(&P->RM->Flags,mrmfSlavePeer)? */

          /* remote peers (non-slave) have already been analyzed (above) */

          continue;
          }
        }    /* END if (P->Index > 0) */

      EMsg[0] = '\0';

      if (MJobEvaluateParAccess(J,P,NULL,EMsg) == FAILURE)
        {
        MDB(3,fSCHED) MLog("INFO:     job '%s' cannot run in partition '%s' (%s) this iteration\n",
          J->Name,
          P->Name,
          EMsg);

        continue;
        }

      if (MJobCheckParLimits(J,P,NULL,EMsg) == FAILURE)
        {
        MDB(3,fSCHED) MLog("INFO:     job '%s' cannot run in partition '%s' (%s) this iteration\n",
          J->Name,
          P->Name,
          EMsg);

        continue;
        }

      /* check if TimeLock flag is set */

      if (bmisset(&J->IFlags,mjifReqTimeLock))
        {
        MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for prio scheduling (TimeLock)\n",
          J->Name);

        if (J->Depend == NULL)
          {
          /* job is master */

          MJobReserve(&J,NULL,0,mjrTimeLock,FALSE);  /* only reserve on time-lock jobs */
          }
        else
          {
          /* job is slave */

          /* ignore job - will be scheduled w/master */

          continue;
          }
        }  /* END if (bmisset(&J->IFlags,mjifReqTimeLock)) */

      SkipIdleJobWithRsv = FALSE;

      /* if the idle job has a reservation we can skip it in some cases */

      /* If a job is in a qos with the mqfReserved flag, its reservation is 
       * created in MQueueScheduleRJobs and won't ever be considered for 
       * preemption if we skip the job. */

      if ((J->Rsv != NULL) &&
          (IsNewJobRsv != TRUE) &&
          (!MJOBISINRESERVEALWAYSQOS(J) || !bmisset(&J->Flags,mjfPreemptor))) 
        {
        if (MSched.PerPartitionScheduling == TRUE)
          {

          /* for per partition scheduling we can skip this job if it already
             has a reservation in the partition we are checking */

          /* I think we can just continue in any case. We don't want to set
           * skipidlejobrsv because that will increment reservation counters.
           * If the job has a reservation MJobPReserve will fail anyways. The
           * job will probably be released in MQueueScheduleRJobs when we look
           * at the partition anyways. The job will only ever look at one 
           * partition. */

          if (J->Rsv->PtIndex == P->Index)
            {
            SkipIdleJobWithRsv = TRUE;
            }
          }
        else if ((J->Rsv->PtIndex != P->Index) || (J->Rsv->StartTime > MSched.Time))
          {

          /* if we are not per partition scheduling then we can skip this job
             if the reservation is in a partition different than the one we are checking
             or if the reservation start time is in the future */

          SkipIdleJobWithRsv = TRUE;
          }
        } /* END if (J->R != NULL) ... */

      if (SkipIdleJobWithRsv == TRUE)
        {
        /* job already has local reservation */
   
        MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for prio scheduling (existing reservation)\n",
          J->Name);
   
        /* locate QOS group */
   
        if ((J->Rsv->CIteration == MSched.Iteration) &&
            (J->Rsv->PtIndex == P->Index))
          {
          /* check to see if the rsv was created this iteration */
          /* FIXME: should store iteration on which rsv was created rather than time */

          /* Only count each reservation once.  Do this by incrementing only when the */
          /* reservation matches the current partition.   */

          int bindex;

          MJobGetQOSRsvBucketIndex(J,&bindex);

          if (bindex != MMAX_QOSBUCKET)
            { 
            MSchedIncrementQOSRsvCount(bindex);
            }

          MSchedIncrementParRsvCount(P->Index);
          }

        /* this job already has reservation either in the future or on a different partition */
        /* this reservation will be kept */
   
        continue;
        }  /* END if (SkipIdleJobWithRsv == TRUE) */
  
      /* NOTE:  ignore IdleJobFound if considering system job */

      /* Once one priority reservation is created, IdleJobFound should be
       * set to prevent anymore jobs from being started. This will allow
       * backfill to kick in before starting any new jobs. After IdleJobFound
       * is set, the remaining jobs will get a reservation if there are 
       * available slots in the reservation buckets. */
 
      if ((IdleJobFound == TRUE) && (!bmisset(&J->Flags,mjfNoResources)))
        {
        mbool_t AdvRsvContinue = FALSE;

        if (bmisset(&J->Flags,mjfAdvRsv) && (AdvRsvTable != NULL))
          {
          mrsv_t *tR;

          int rindex;

          MRsvFind(J->ReqRID,&tR,mraNONE);

          for (rindex = 0;rindex < AdvRsvTableSize;rindex++)
            {
            if (tR == AdvRsvTable[rindex])
              {
              AdvRsvContinue = TRUE;

              break;
              }
            }    /* END for (rindex) */
          }
        else if (bmisset(&J->Flags,mjfAdvRsv) && (AdvRsvTable == NULL))
          {
          /* NO-OP */
          }
        else
          {
          AdvRsvContinue = TRUE;
          }

        if (AdvRsvContinue == TRUE)
          {
          MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for prio scheduling (Idle Job Reached)\n",
            J->Name);
   
          /* reserve job if eligible */
     
          if (MJobCheckLimits(
                J,
                mptSoft,
                P,
                &BM,
                (J->Array != NULL) ? &J->Array->PLimit : NULL,
                NULL) == FAILURE)
            {
            /* continue if jobs which have started on this iteration */
            /* cause this job to now violate active policies */
     
            MDB(5,fSCHED) MLog("INFO:     skipping job '%s'  (job now violates active policy)\n",
              J->Name);
     
            continue;
            }
     
          if ((MJobPReserve(J,P,FALSE,0,NULL) != SUCCESS) ||
              (J->Rsv == NULL))
            {
            /* reservation attempt for idle job failed */
     
            MDB(5,fSCHED) MLog("INFO:     cannot reserve job '%s'\n",
              J->Name);
     
            continue;
            }
   
          if (J->Rsv->StartTime > MSched.Time)
            {
            /* reservation attempt for idle job succeeded */

            if (MSched.EnableHPAffinityScheduling == TRUE)
              IsNewJobRsv = TRUE;
   
            if ((J->IsInteractive == TRUE) && 
                (MSched.DontCancelInteractiveHJobs != TRUE) &&
                (J->Rsv != NULL) &&
                (J->Rsv->StartTime >= MSched.Time + (MCONST_EFFINF / 2)))
              {
              char tmpLine[MMAX_LINE];
   
              snprintf(tmpLine,sizeof(tmpLine),"MOAB_INFO:  interactive job can never run");
   
              MJobCancel(J,tmpLine,FALSE,NULL,NULL);
   
              continue;
              }
   
            MDB(5,fSCHED) MLog("INFO:     future reservation found for job '%s' in %ld\n",
              J->Name,
              J->Rsv->StartTime - MSched.Time);
   
            continue;
            }
          }  /* END else */ 
          /* job reservation created for immediate use (schedule job) */
        }  /* END if ((IdleJobFound == TRUE) && ...) */

      MStat.EvalJC++;

      FeasibilityChecked = TRUE;

      if (MReqGetFNL(
           J,
           J->Req[0],
           P,
           NULL,
           &tmpNL, /* O - not used */
           NULL,
           NULL,
           MMAX_TIME,  /* locate all feasible nodes regardless of current load, state */
           0,
           NULL) == FAILURE)
        {
        /* insufficient feasible nodes found */

        MDB(4,fSCHED) MLog("INFO:     cannot locate adequate feasible tasks for job %s:%d in partition %s\n",
          J->Name,
          0,
          P->Name);

        continue;
        }

      FeasibleLocalParCount++;
   
      BestSValue = 0;
      BestSIndex = -1; 
   
      NAPolicy = (J->Req[0]->NAllocPolicy != mnalNONE) ?
        J->Req[0]->NAllocPolicy :
        P->NAllocPolicy;
   
      DefaultSpecWCLimit = J->SpecWCLimit[0];
    
      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        DefTC[rqindex] = J->Req[rqindex]->TaskRequestList[0];
        }  /* END for (rqindex) */

      /* multi-req jobs only have as many shapes as are found in the primary req */

      if (bmisset(&J->Flags,mjfNoResources))
        {
        /* only one shape allowed */

        BestSIndex = 1;
        }

      if (J->Req[0]->TaskRequestList[2] > 0)
        {
        IsMalleableJob = TRUE;
        }
      else
        {
        IsMalleableJob = FALSE;
        }
   
      for (sindex = 1;sindex < MMAX_SHAPE;sindex++)
        {
        char EMsg[MMAX_LINE];

        /* search through all malleable shapes */

        if (((J->Req[0]->TaskRequestList[sindex] <= 0) && 
             !bmisset(&J->Flags,mjfNoResources)) ||
            ((sindex > 1) && bmisset(&J->Flags,mjfNoResources)))
          {
          /* end of list located */

          break;
          }

        J->SpecWCLimit[0] = J->SpecWCLimit[sindex];

        J->Request.TC = 0;

        for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
          {
          RQ = J->Req[rqindex];

          RQ->TaskRequestList[0]  = RQ->TaskRequestList[sindex];
          RQ->TaskCount           = RQ->TaskRequestList[sindex];
          J->Request.TC          += RQ->TaskRequestList[sindex];
          
          /* NodeCount could be lingering with an incorrect value from a requeue and a
           * specified trl. ex. a job is running on 3 nodes, then requeued and those 3 
           * nodes aren't available anymore, only one is, which it can run on. The 
           * RQ->NodeCount is 3 which messes up the distribution of tasks in MJobDistributeTasks */

          /* Should this be set differently or higher? */

          /* Though this fixes the trl issue, it causes a problem for NODEMATCHPOLICY EXACTNODE where
           * MJobAllocMNL relies on RQ->NodeCount to get the correct number of nodes. */

          /* RQ->NodeCount = 0; */
          }  /* END for (rqindex) */
   
        /* is the best strategy widest fit?  ie, local greedy? */
   
        MJobUpdateResourceCache(J,NULL,sindex,NULL);
   
        if (MJobCheckLimits(
              J,
              mptSoft,
              P,
              &BM,
              (J->Array != NULL) ? &J->Array->PLimit : NULL,
              NULL) == FAILURE)
          {
          /* continue if jobs started on this iteration    */
          /* cause this job to now violate active policies */
   
          MDB(5,fSCHED) MLog("INFO:     skipping job '%s'  (job violates active policy')\n",
            J->Name);
   
          continue;
          }
   
        MDB(4,fSCHED) MLog("INFO:     checking job %s(%d)  state: %s (ex: %s)\n",
          J->Name,
          sindex,
          MJobState[J->State],
          MJobState[J->EState]);
   
        /* select resources for job */ 
   
        if (MPar[0].BFPolicy == mbfPreempt)
          {
          /* all priority jobs are preemptors */
   
          AllowPreemption = TRUE;
          }
        else 
          {
          AllowPreemption = MBNOTSET;
          }

        /* check priority job start limits */

        if ((MSched.MaxPrioJobStartPerIteration >= 0) &&
            (MSched.JobStartPerIterationCounter >= MSched.MaxPrioJobStartPerIteration))
          {
          /* NOTE:  there is not a separate counter for priority jobs - this 
                    works only because priority jobs start prior to backfill 
                    jobs - also, because there is one counter, suspended job
                    and reserved job starts this iteration will be counted 
                    against the limit */

          MDB(6,fSCHED) MLog("ALERT:    job '%s' blocked by JOBMAXSTARTPERITERATION policy in %s (%d >= %d)\n",
            J->Name,
            FName,
            MSched.JobStartPerIterationCounter,
            MSched.MaxPrioJobStartPerIteration);

          continue;
          }

        /* check available resources on 'per shape' basis */
   
        if (MJobSelectMNL(
              J,
              P,
              NULL,      /* I - provide no feasible node list */
              MNodeList, /* O */
              NodeMap,   /* O */
              AllowPreemption,
              FALSE,
              &PReq,     /* O */
              EMsg,
              NULL) == FAILURE)
          {
          MDB(6,fSCHED) MLog("INFO:     cannot locate %d tasks for job '%s' in partition %s.  Msg: %s\n",
            J->Request.TC,
            J->Name,
            P->Name,
            EMsg);
   
          continue;
          }  /* END:  if (MJobSelectMNL() == FAILURE) */

        BestSValue = 1;
        BestSIndex = sindex;

        if (IsMalleableJob == TRUE)
          {
          MNLMultiCopy(BestMNodeList,MNodeList);
          MNodeMapCopy(BestNodeMap,NodeMap);
          }
        }    /* END for (sindex) */

      /* restore default malleable shape */
 
      J->SpecWCLimit[0] = DefaultSpecWCLimit;
      J->Request.TC = 0;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        RQ->TaskRequestList[0]  = DefTC[rqindex]; 
        RQ->TaskCount           = DefTC[rqindex];
        J->Request.TC          += DefTC[rqindex];
        }  /* END for (rqindex) */
   
      MJobUpdateResourceCache(J,NULL,0,NULL);
   
      if (BestSValue <= 0)
        {
        /* no available slot located */

        if (!bmisset(&J->Flags,mjfRsvMap) &&
            ((MJobPReserve(J,P,(SP != NULL) ? TRUE : FALSE,0,&RsvCountRej) == SUCCESS) ||
            (RsvCountRej == FALSE)))
          {
          if (MSched.EnableHPAffinityScheduling == TRUE)
            IsNewJobRsv = TRUE;

          if (MSched.AlwaysEvaluateAllJobs == FALSE)
            IdleJobFound = TRUE;

          if ((J->IsInteractive == TRUE) && 
              (MSched.DontCancelInteractiveHJobs != TRUE) &&
              (J->Rsv != NULL) &&
              (J->Rsv->StartTime >= MSched.Time + (MCONST_EFFINF / 2)))
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"MOAB_INFO:  interactive job can never run");

            MJobCancel(J,tmpLine,FALSE,NULL,NULL);

            continue;
            }

          continue;
          }

        /* skip job - cannot create rsv: rsv depth reached */
   
        if (bmisset(&J->Flags,mjfAdvRsv))
          {
          int rindex;

          mrsv_t *tR;

          MRsvFind(J->ReqRID,&tR,mraNONE);

          if (AdvRsvTable == NULL)
            {
            AdvRsvTableSize = 16;

            AdvRsvTable = (mrsv_t **)MUCalloc(1,sizeof(mrsv_t *) * AdvRsvTableSize);
            }

          for (rindex = 0;rindex < AdvRsvTableSize;rindex++)
            {
            if (AdvRsvTable[rindex] == NULL)
              {
              AdvRsvTable[rindex] = tR;

              if (MSched.AlwaysEvaluateAllJobs == FALSE)
                IdleJobFound = TRUE;

              break;
              }
            }    /* END for (rindex) */

          if ((rindex == AdvRsvTableSize) && (IdleJobFound == FALSE))
            {
       
            /* Enlarge the AdvRsvTable by 16 pointers */

            AdvRsvTableSize += 16 * sizeof(mrsv_t *);

            AdvRsvTable = (mrsv_t **)realloc(AdvRsvTable,sizeof(mrsv_t *) * AdvRsvTableSize);

            if (AdvRsvTable != NULL)
              {
              AdvRsvTable[rindex] = tR;
  
              AdvRsvTable[rindex + 1] = NULL;
              }
            else
              {
              MDB(3,fSCHED) MLog("ERROR:     MQueueScheduleIJobs failed growing AdvRsvTable table to %d entries using realloc.\n",
                AdvRsvTableSize);

              AdvRsvTableSize = 0;
              MNLFree(&tmpNL);
              return(FAILURE);
              }

            if (MSched.AlwaysEvaluateAllJobs == FALSE)
              IdleJobFound = TRUE;
            }
          }
        else
          {
          /* cannot select resources for any job shape and cannot get a priority rsv */

          if (MSched.AlwaysEvaluateAllJobs == FALSE)
            IdleJobFound = TRUE;
          }

        continue;
        }    /* END if (BestSValue <= 0) */
   
      /* adequate nodes found */
   
      J->SpecWCLimit[0] = J->SpecWCLimit[BestSIndex];
      J->Request.TC     = 0;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        RQ->TaskRequestList[0]  = RQ->TaskRequestList[BestSIndex];
        RQ->TaskCount           = RQ->TaskRequestList[BestSIndex];
        J->Request.TC          += RQ->TaskRequestList[BestSIndex];

        /* RQ->TaskRequestList[BestSIndex] = DefTC[rqindex]; */
        }  /* END for (rqindex) */

      if (IsMalleableJob == TRUE)
        {
        /* restore available nodelist info */

        MNLMultiCopy(MNodeList,BestMNodeList);

        MNodeMapCopy(NodeMap,BestNodeMap);
        }

      /* NOTE:  do not store default info in TaskRequestList (DBJ-Mar02/04) */
   
      /* J->SpecWCLimit[BestSIndex] = DefaultSpecWCLimit; */
   
      if (BestSIndex > 1)
        MJobUpdateResourceCache(J,NULL,BestSIndex,NULL);

      if (MPar[0].SetBlock == TRUE)
        MNLMultiCopy(tmpMNodeList,MNodeList);
   
      /* unset flag so job will fail MJobAllocMNL. */

      if ((MSched.NodeSetPlus == mnspDelay) &&
          ((MPar[0].NodeSetDelay > 0) || ((J->Req[0] != NULL) && (J->Req[0]->SetDelay > 0))))
        {
        bmunset(&J->IFlags,mjifNodeSetDelay);
        }

      do 
        {
        if (MJobAllocMNL( 
              J,
              MNodeList,  /* I */
              NodeMap,    /* I */
              NULL,
              NAPolicy,
              MSched.Time,
              &tmpSC,     /* O */
              NULL) == FAILURE)
          {
          mulong tmpEStartTime = 0;
   
          MDB(1,fSCHED) MLog("ERROR:    cannot allocate nodes to job '%s' in partition %s\n",
            J->Name,
            P->Name);
    
          /* NOTE:  what if job can run immediately in another partition? */
   
          if (((!bmisset(&P->Flags,mpfSharedMem)) && /* Don't push sharedmem job's reservation out to MaxRMPollInterval + 1. */
              ((tmpSC == marLocality) ||
               ((tmpSC == marNodeSet) && (MSched.NodeSetPlus != mnspSpanEvenly) && (MSched.NodeSetPlus != mnspDelay)))))
            {
            /* node set cannot be satisfied in current partition */
   
            if ((J->SysSMinTime != NULL) ||
               ((J->SysSMinTime = (mulong *)MUCalloc(1,sizeof(mulong) * MMAX_PAR)) != NULL))
              {
              /* partition availability time must be more than 1 second in future to favor
                 other local partitions first */
   
              J->SysSMinTime[P->Index] = MSched.Time + MSched.MaxRMPollInterval + 1; 
              }
            else
              {
              tmpL = MSched.Time + 1;
   
              MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mAdd);
              }
   
            /* NOTE:  possible alternate solution:  try another partition and if all 
                      partitions fail, set J->SysSMinTime to MSched.Time + 1 (NYI) */
   
            /* NOTE:  MJobGetRange() should enforce node sets (NYI?) */
            }
          else if ((tmpSC == marNodeSet) && ((MSched.NodeSetPlus == mnspSpanEvenly) || (MSched.NodeSetPlus == mnspDelay)))
            {
   
            }
          else
            {
            tmpL = MSched.Time + 1;
     
            MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mAdd);
            }
   
          /* Only find ranges that start at the time slurm says the job can run. This fixes the situation
           * where isn't ready to handle a system preemptor job. Prior situation: system preemptor would
           * preempt all jobs, but slurm, through the willrun, says that it can't run for 5 seconds so 
           * moab tries to reserve it 1 second in the future. Slurm still says it isn't ready for 5 seconds
           * so moab sees thinks that the job can never run because slurm's start time is greater than the
           * start time of the only range at 1 second. */
   
          if (MSched.BluegeneRM == TRUE)
            tmpEStartTime = J->WillrunStartTime;
   
          if ((MJobPReserve(J,P,FALSE,tmpEStartTime,&RsvCountRej) == SUCCESS) ||
              (RsvCountRej == FALSE))
            {
            if (MSched.EnableHPAffinityScheduling == TRUE)
              IsNewJobRsv = TRUE;

            if (MSched.AlwaysEvaluateAllJobs == FALSE)
              IdleJobFound = TRUE;
            }
          else
            {
            /* skip job */
   
            /* cannot allocate resources for job and cannot get a priority rsv */
     
            IdleJobFound = TRUE;
            }

          break;
          }    /* END if (MJobAllocMNL() == FAILURE) */
   
        if ((MSched.MultipleComputePartitionsActive == TRUE) &&
            (MSched.SchedPolicy == mscpEarliestCompletion))
          {
          /* reserve job for immediate start */
   
          /* NOTE:  job should consume/reserve policy slots (NYI) */
   
          bmset(&J->IFlags,mjifStartPending);
   
          /* NYI */
   
          if ((MPar[0].BFPolicy == mbfPreempt) && (IdleJobFound == TRUE))
            {
            /* job was backfilled */
     
            bmset(&J->SpecFlags,mjfPreemptee);
     
            MJobUpdateFlags(J);
            }
     
          SchedCount++;
     
          if (MSched.EnableHighThroughput == FALSE)
            {
            MUGetTime(&Now,mtmNONE,&MSched);
     
            if (Now > MSched.IntervalStart + MSched.MaxRMPollInterval)
              {
              if ((MSched.ServerS.sd > 0) && (MSUSelectRead(MSched.ServerS.sd,1) == SUCCESS))
                {
                /* user request waiting */
     
                break;
                }
              }    /* END if (MSched.Time > MSched.IntervalStart + MSched.MaxRMPollInterval) */
            }
          }
        else
          {
          if (MJobStart(J,NULL,NULL,"priority job started") == FAILURE)  /* FIXME: encapsulate in a policy to control immediate staging/starting */
            {
            if (MSched.Mode == msmNormal)
              {
              MDB(1,fSCHED) MLog("ERROR:    cannot start job '%s' in partition %s\n",
                J->Name,
                P->Name);
              }
            else
              {
              MDB(1,fSCHED) MLog("INFO:     cannot start job '%s' in partition %s - scheduler mode: %s\n",
                J->Name,
                P->Name,
                MSchedMode[MSched.Mode]);
              }
   
            tmpL = MSched.Time + MAX(1,MSched.JobRsvRetryInterval);
       
            MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mAdd);
   
            if (PReq == TRUE)
              {
              /* create rsv for preempted resources */
   
              if ((MJobReserve(&J,NULL,0,mjrQOSReserved,FALSE) == SUCCESS) && (J->Rsv != NULL))
                J->Rsv->ExpireTime = MSched.Time + MDEF_PREEMPTRSVTIME;
   
              continue;
              }
   
            if ((MJobPReserve(J,P,FALSE,0,&RsvCountRej) == SUCCESS) ||
                (RsvCountRej == FALSE))
              { 
              if (MSched.EnableHPAffinityScheduling == TRUE)
                IsNewJobRsv = TRUE;
            
              if (MSched.AlwaysEvaluateAllJobs == FALSE)
                IdleJobFound = TRUE;
   
              continue;
              }
   
            /* skip job */
   
            /* job start failed and cannot get priority rsv */
       
            if (MSched.AlwaysEvaluateAllJobs == FALSE)
              IdleJobFound = TRUE;
       
            continue;
            }    /* END if (MJobStart() == FAILURE) */
          else
            {
            /* job started successfully don't check other partitions */
   
            SchedCount++;
            }
          }      /* END else ((MSched.MultipleComputePartitionsActive == TRUE) && ...) */
        }   while (MQueueGetNextJobIfSimilar(&J,&jindex,Q,P) == SUCCESS);

      break;
      }      /* END for (pindex) */

    MDB(6,fSCHED) MLog("INFO:     %d local partitions located for job %s (best remote par: %s)\n",
      FeasibleLocalParCount,
      J->Name,
      (BestRemoteP != NULL) ? BestRemoteP->RM->Name : "---");

    /* check both remote and local partition earliest start times to determine which is best to use */

    if (!MJOBISACTIVE(J) &&
        (BestRemoteP != NULL))
      {
      char tmpLine[MMAX_LINE];

      if (FeasibleLocalParCount > 0)
        {
        /* compare local with remote */

        if (J->Request.TC > BestRemoteP->ARes.Procs)
          {
          /* just a quick check to see whether the remote partition has enough
             resources to start the job now or if job has a better chance at 
             starting locally */

          /* FIXME: should get remote reservation as this technique does not
                  give local jobs a fair chance on the remote cluster */

          MDB(6,fSCHED) MLog("INFO:     remote partition %s has inadequate available resources for job %s - do not stage job at this time.\n",
            BestRemoteP->Name,
            J->Name);

          continue;
          }

        if  (J->Rsv != NULL)
          {
          /* NOTE:  For now, J->R is ALWAYS reservation on local resources */

          if (J->Rsv->StartTime <= MSched.Time)
            {
            /* assume all jobs got a local rsv if they could have */
            /* local reservation is already best - move on to next job */

            MDB(6,fSCHED) MLog("INFO:     job '%s' has a better local rsv. than can be found on remote par '%s' - no further action\n",
              J->Name,
              BestRemoteP->Name);

            continue;
            }

          if ((J->Rsv->StartTime - MSched.Time) <= 
               BestRemoteP->RM->StageThreshold)
            {
            /* don't migrate to remote - it is worth leaving locally if we save
               time below the threshold limit */

            MDB(6,fSCHED) MLog("INFO:     job '%s' has a satisfactory local rsv. - not scheduling on remote par '%s'\n",
              J->Name,
              BestRemoteP->Name);

            continue;
            }
          }    /* END if (J->R != NULL) */

        /* remote partition is better - remove local reservation */

        MJobReleaseRsv(J,TRUE,TRUE);
        }  /* END if (FeasibleLocalParCount > 0) */
      else if (J->Request.TC > BestRemoteP->ARes.Procs)
        {
        /* just a quick check to see whether the remote partition has enough 
           resources to start the job now */ 

        /* job has a better chance at starting locally */

        /* FIXME: should get remote reservation as this technique does not give
                  local jobs a fair chance on the remote cluster */

        /* NOTE:  BestRemoteP->ARes.Procs reduced below after successful migrate */

        MDB(6,fSCHED) MLog("INFO:     remote partition %s has inadequate available resources for job %s - do not stage job at this time.\n",
          BestRemoteP->Name,
          J->Name);

        continue;
        }  /* END else if (J->Request.TC > BestRemoteP->ARes.Procs) */

      if (J->DestinationRM == NULL)
        {
        enum MStatusCodeEnum tSC;

        /* If the job's SystemID isn't me, don't migrate (job was migrated
         * here. don't want to bounce). 
         *
         * J->SystemID can be NULL if the job failed to migrate. 
         * If NULL, then this cluster owns the job. */

        if ((J->SystemID != NULL) && 
            (MUStrNCmpCI(J->SystemID,MSched.Name,-1) == FAILURE))
          {
          continue;
          }

        /* immediately migrate job to remote partition */

        MDB(6,fSCHED) MLog("INFO:     attempting to migrate job '%s' to peer '%s'\n",
          J->Name,
          BestRemoteP->RM->Name);

        if (bmisset(&J->Flags, mjfArrayJob) && (J->JGroup->Name != NULL))
          {
          if (MQueueMigrateArrays(J,BestRemoteP,TRUE) == FAILURE)
            {
            /* the array cannnot run right now */
            MDB(6,fSCHED) MLog("INFO:     remote partition %s has inadequate available resources for array job %s.\n",
              BestRemoteP->Name,
              J->Name);

            }
            continue;
          }
        else if (MJobMigrate(&J,BestRemoteP->RM,FALSE,NULL,tmpLine,&tSC) == FAILURE)
          {
          /* on persistent error, will eliminate remote partition from
           * feasible list */

          MJobProcessMigrateFailure(J,BestRemoteP->RM,tmpLine,tSC);

          continue;
          }

          /* NOTE:  MJobMigrate will reduce available partition resources by the 
                  corresponding amount to prevent remote cluster from being 
                  overwhelmed within the iteration */

        BestRemoteP->ARes.Procs -= J->TotalProcCount;
        }    /* END if (J->DRM == NULL) */
      else
        {
        MDB(6,fSCHED) MLog("INFO:     job '%s' already migrated to RM '%s' - cannot migrate to peer '%s'\n",
          J->Name,
          J->DestinationRM->Name,
          BestRemoteP->RM->Name);
        }
      }      /* END if (!MJOBISACTIVE(J) && ...) */
    else if (!MJOBISACTIVE(J) &&
             (J->Rsv == NULL) &&
             (FeasibleLocalParCount  == 0) &&
             (FeasibilityChecked == TRUE) &&
             (MSched.RejectInfeasibleJobs == TRUE))
      {
      /* the job cannnot run anywhere */

      MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,NULL);
      }
    }        /* END for (jindex) */
 
  if (SchedCount != 0)
    {
    MStat.IJobsStarted = SchedCount;

    MDB(2,fSCHED) MLog("INFO:     %d jobs started for partition (%s) on iteration %d\n",
      SchedCount,
      (SP == NULL) ? "" : SP->Name,
      MSched.Iteration);
    }
 
  MDB(2,fSCHED) MLog("INFO:     resources available after scheduling: N: %d  P: %d\n", 
    MPar[0].IdleNodes,
    MPar[0].ARes.Procs);

  MNLFree(&tmpNL);

  MUFree((char **)&AdvRsvTable);
 
  return(SUCCESS);
  }  /* END MQueueScheduleIJobs() */



/**
 * Get the next job in the queue if it is simlar.
 *
 * NOTE: this routine is currently disabled.  However, in the future, for
 *       rapid scalability we will need to enable it again.  This routine
 *       relies on using cached FNLs and also a bit of code in MJobAllocMNL
 *       that weans down the available node list as jobs are rapidly
 *       scheduled.  That code in MJobAllocMNL is currently disabled because
 *       it breaks other things (MOAB-2256)
 *
 * @param JP Next job to schedule
 * @param Index
 * @param Q
 * @param P
 */

int MQueueGetNextJobIfSimilar(

  mjob_t **JP,
  int     *Index,
  mjob_t **Q,
  mpar_t  *P)

  {
  mbitmap_t BM;

  mjob_t *J;
  mjob_t *NextJ;

  if ((JP == NULL) || (Index == NULL) || (Q == NULL) || (MSched.EnableHighThroughput == FALSE))
    {
    return(FAILURE);
    }

  J = *JP;
  
  if (Q[*Index + 1] != NULL)
    NextJ = Q[*Index + 1];
  else
    return(FAILURE);

  if ((J == NULL) || (NextJ == NULL))
    {
    return(FAILURE);
    }

  if (!MNLIsEmpty(&J->ReqHList) ||
      !MNLIsEmpty(&NextJ->ReqHList))
    {
    return(FAILURE);
    }

  if (bmisset(&J->Flags,mjfPreemptor) ||
      bmisset(&NextJ->Flags,mjfPreemptor))
    {
    return(FAILURE);
    }

  if (MJobCmp(J,NextJ) == FAILURE)
    {
    return(FAILURE); 
    }

  bmset(&BM,mlActive);

  if (MJobCheckLimits(
        J,
        mptSoft,
        P,
        &BM,
        (J->Array != NULL) ? &J->Array->PLimit : NULL,
        NULL) == FAILURE)
    {
    /* job would violate a limit */

    return(FAILURE);
    }

  *JP = NextJ;

  (*Index)++;

  return(SUCCESS);
  }  /* END MQueueGetNextJobIfSimilar() */



/**
 * Report reason job is blocked at specified policy level.
 *
 * @param Auth      (I)
 * @param NBJobList (I)
 * @param PLevel    (I)
 * @param PS        (I) [optional]
 * @param String    (O)
 */

int MUIQueueDiagnose(

  char                *Auth,
  mjob_t             **NBJobList,
  enum MPolicyTypeEnum PLevel,
  mpar_t              *PS,
  mstring_t           *String)

  {
  int       pindex;
  int       index;
  int       jindex;
 
  mgcred_t *U = NULL;

  mjob_t   *J;

  enum      MDependEnum DType;
 
  char      tmpLine[MMAX_LINE];
 
  enum MPolicyRejectionEnum RIndex;

  mbool_t   IsBlocked;
 
  mpar_t   *P;
  mpar_t   *GP;
 
  job_iter JTI;

  const char *FName = "MQueueDiagnose";

  MDB(4,fSTRUCT) MLog("%s(FullQ,NBJobList,%s,%s,String)\n",
    FName,
    MPolicyMode[PLevel],
    (PS != NULL) ? PS->Name : "");
 
  if ((String == NULL) ||
      (PS == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  must be synchronized with MQueueSelectJobs() and MJobCheckPolicies() */

  MStringAppendF(String,"evaluating blocked jobs (policylevel %s  partition %s)\n\n",
    MPolicyMode[PLevel],
    PS->Name);
 
  MUserAdd(Auth,&U);

  MLocalCheckFairnessPolicy(NULL,MSched.Time,NULL);
 
  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if ((PS->Index > 0) && (bmisset(&J->PAL,PS->Index) == FAILURE))
      continue;
 
    if (MJOBISACTIVE(J) == TRUE)
      {
      continue;
      } 

    if (MUICheckAuthorization(
          U,
          NULL,
          (void *)J,
          mxoJob,
          mcsMJobCtl,
          mjcmQuery,
          NULL,
          NULL,
          0) == FAILURE)
      {
      continue;
      }
 
    MDB(4,fUI) MLog("INFO:     checking job '%s'\n",
      J->Name);

    /* look for job in NBJobList */

    for (jindex = 0;NBJobList[jindex] != NULL;jindex++)
      {
      if (J == NBJobList[jindex])
        break;
      }  /* END for (jindex) */

    if (J == NBJobList[jindex])
      {
      /* job is not blocked */

      continue;
      }

    IsBlocked = FALSE;

    /* check job state */
 
    if ((J->State != mjsIdle) && (J->State != mjsHold))
      {
      MStringAppendF(String,"job %-20s has non-idle state (state: '%s')\n",
        J->Name,
        MJobState[J->State]);
 
      continue;
      }
 
    /* check user/system/batch hold */
 
    if (!bmisclear(&J->Hold))
      {
      tmpLine[0] = '\0';
 
      for (index = 0;MHoldType[index] != NULL;index++)
        {
        if (bmisset(&J->Hold,index))
          {
          if (tmpLine[0] != '\0')
            strcat(tmpLine,",");

          strcat(tmpLine,MHoldType[index]);
          }
        }    /* END for (index) */

      if (J->HoldReason != mhrNONE)
        {
        MStringAppendF(String,"job %-20s has the following hold(s) in place: %s (%s)\n",
          J->Name,
          tmpLine,
          MHoldReason[J->HoldReason]);
        }
      else
        {
        MStringAppendF(String,"job %-20s has the following hold(s) in place: %s\n",
          J->Name,
          tmpLine);
        }

      IsBlocked = TRUE;
      }
 
    /* check if job has been previously scheduled or deferred */
 
    if (J->EState != mjsIdle)
      {
      MStringAppendF(String,"job %-20s has non-idle expected state (expected state: %s)\n",
        J->Name,
        MJobState[J->EState]);
 
      IsBlocked = TRUE;
      }
 
    /* check release time */
 
    if (J->SMinTime > MSched.Time)
      {
      char TString[MMAX_LINE];

      MULToTString(J->SMinTime - MSched.Time,TString);

      MStringAppendF(String,"job %-20s has not reached its start date (%s to startdate)\n",
        J->Name,
        TString);
 
      IsBlocked = TRUE;
      }
 
    /* check job dependencies */

    mstring_t DValue(MMAX_LINE);
 
    if (MJobCheckDependency(J,&DType,FALSE,NULL,&DValue) == FAILURE)
      {
      MStringAppendF(String,"job %-20s requires %s of job '%s'\n",
        J->Name, 
        MDependType[DType],
        DValue.c_str());
 
      IsBlocked = TRUE;
      }  /* END if (MJobCheckDependency() == FAILURE) */

    if (MLocalCheckFairnessPolicy(J,MSched.Time,NULL) == FAILURE)
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected, partition %s (violates local fairness policy)\n",
        J->Name,
        PS->Name);
 
      MStringAppendF(String,"job %-20s would violate 'local' fairness policies\n",
        J->Name);
 
      IsBlocked = TRUE;
      }
 
    GP = &MPar[0];
 
    /* determine all partitions in which job can run */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\1')
        continue;

      if (P->Name[0] == '\0')
        break;
 
      if (P->ConfigNodes == 0)
        continue;

      if (!strcmp(P->Name,"SHARED"))
        continue;
 
      if ((J->Credential.C != NULL) && !bmisset(&P->Classes,J->Credential.C->Index))
        {
        /* required classes not configured in partition */
 
        MStringAppendF(String,"job %-20s requires class not configured in partition %s (%s)\n",
          J->Name,
          P->Name,
          J->Credential.C->Name);

        IsBlocked = TRUE;
 
        continue;
        }
 
      if (bmisset(&J->PAL,P->Index) == FAILURE)
        { 
        MStringAppendF(String,"job %-20s not allowed to run in partition %s  (partitions allowed: %s)\n",
          J->Name,
          P->Name,
          MPALToString(&J->PAL,NULL,tmpLine));

        IsBlocked = TRUE;
 
        continue;
        }
 
      /* check job limits and other policies */
 
      if (MJobCheckPolicies(
            J,
            PLevel,
            0,
            P,
            &RIndex,
            tmpLine,
            MSched.Time) == FAILURE)
        {
        MStringAppendF(String,"%s\n",tmpLine);

        IsBlocked = TRUE;
 
        continue;
        }
 
      if (GP->FSC.FSPolicy != mfspNONE)
        {
        int OIndex;
        int rc;

        if (P->FSC.ShareTree != NULL)
          {
          mstring_t tmpMsg(MMAX_LINE);

          if ((rc = MFSTreeCheckCap(P->FSC.ShareTree,J,P,&tmpMsg)) == FAILURE)
            {
            MDB(5,fSCHED) MLog("INFO:     %s\n",tmpMsg.c_str());
            }
          }
        else if ((rc = MFSCheckCap(NULL,J,P,&OIndex)) == FAILURE)
          {
          MDB(5,fSCHED) MLog("INFO:     job '%s' exceeds %s FS cap\n",
            J->Name,
            MXO[OIndex]);
 
          MStringAppendF(String,"job %-20s would violate %s FS cap in partition %s\n",
            J->Name,
            MXO[OIndex],
            P->Name);
          }
        
        if (rc == FAILURE)
          {
          IsBlocked = TRUE;

          continue;
          }
        }    /* END if (GP->FSC.FSPolicy != mfspNONE) */
      }      /* END for (pindex) */

    if (IsBlocked == FALSE) 
      {
      switch (MJobGetBlockReason(J,PS))
        {
        case mjneIdlePolicy:

          MStringAppendF(String,"job %-20s is blocked by idle job policy\n",
            J->Name);

          break;

        default:

          if ((PLevel == mptHard) || (PLevel == mptOff))
            {
            MStringAppendF(String,"job %-20s is not blocked at this policy level\n",
              J->Name);
            }

          break;
        }  /* END switch (J->BlockReason) */
      }    /* END if (IsBlocked == FALSE) */
    }      /* END for (jindex) */

  return(SUCCESS);
  }  /* END MQueueDiagnose() */





/**
 * Schedule/manage/resume suspended jobs.
 *
 * @param Q (I)
 * @param RJ (O)
 * @param EMsgArray (O)  EMsgArray must have same number of 
 *                  entries as Q, not counting the terminator,
 *                  or be NULL.
 *
 * sets RJ (optional) to the number of jobs resumed
 */

int MQueueScheduleSJobs(

  mjob_t **Q,  /* I */
  int     *RJ, /* O -optional */
  char   **EMsgArray) /* O -optional */

  {
  int         jindex;

  mjob_t     *J;

  mnl_t  *MNodeList[MMAX_REQ_PER_JOB];

  char       *NodeMap = NULL;

  mbool_t     AllowPreemption;
  mbool_t     DisableFailingNode;

  mutime      OrigWCLimit;

  char        EMsg[MMAX_LINE];

  char       *RsvAList[MMAX_PJOB + 1];

  char      **OrigRsvAList;

  int         OrigLogLevel = 0;

  int         RJobs = 0;

  const char *FName = "MQueueScheduleSJobs";

  mnode_t    *N;

  MDB(2,fSCHED) MLog("%s(Q)\n",
    FName);

  if (Q == NULL)
    {
    return(FAILURE);
    }

  if (getenv("MOABDISABLENODEONFAILEDRESUME"))
    {
    DisableFailingNode = TRUE;
    }
  else
    {
    DisableFailingNode = FALSE;
    }
 
  MNodeMapInit(&NodeMap);

  MNLMultiInit(MNodeList);

  /* NOTE:  suspended job resume should be interleaves with priority job start (NYI) */

  /* NOTE:  if suspended jobs are not resumed, issue may be SMinTime, MJobSelectMNL(), 
            or the fact that MQueueSelectAllJobs() does not select the suspended job */

  /* locate suspended jobs */

  AllowPreemption = MBNOTSET;

  if (bmisset(&MSched.Flags,mschedfJobResumeLog))
    {
    OrigLogLevel   = mlog.Threshold;
    mlog.Threshold = 7;

    MDB(7,fSCHED) MLog("INFO:     increasing LOGLEVEL for suspended job scheduling\n");
    }

  for (jindex = 0;Q[jindex] != NULL;jindex++)
    {
    J = Q[jindex];

    if ((MSched.MaxJobStartPerIteration >= 0) &&
        (MSched.JobStartPerIterationCounter >= MSched.MaxJobStartPerIteration))
      {
      MDB(4,fSCHED) MLog("ALERT:    job '%s' blocked by JOBMAXSTARTPERITERATION policy in %s (%d >= %d)\n",
        J->Name,
        FName,
        MSched.JobStartPerIterationCounter,
        MSched.MaxJobStartPerIteration);

      break;
      }

    if (J->State != mjsSuspended)
      continue;

    if (J->SMinTime > MSched.Time)
      continue;

    /* NOTE:  allow job to preempt lower-priority idle job reservations */
 
    /* enable this by locating idle job reservations and adding them to the
       jobs RAList field */

    OrigRsvAList = J->RsvAList;

    bmset(&J->IFlags,mjifIsResuming);

    MNLGetNodeAtIndex(&J->NodeList,0,&N);
    MJobGetNodePreemptList(J,N,RsvAList);

    J->RsvAList = RsvAList;

    MDB(2,fSCHED) MLog("INFO:     checking suspended job %s (priority: %ld)\n",
      J->Name,
      J->CurrentStartPriority);

    if (J->RemainingTime > 0)
      {
      OrigWCLimit = J->SpecWCLimit[0];

      MJobSetAttr(J,mjaReqAWDuration,(void **)&J->RemainingTime,mdfLong,mSet);
      }

    InHardPolicySchedulingCycle = TRUE;

    if (MJobSelectMNL(
          J,
          &MPar[J->Req[0]->PtIndex],
          &J->NodeList,
          MNodeList,
          NodeMap,
          AllowPreemption,  /* I */
          FALSE,
          NULL,
          NULL,
          NULL) == FAILURE)
      {
      /* job nodes not available */

      InHardPolicySchedulingCycle = FALSE;

      MNLGetNodeAtIndex(&J->NodeList,0,&N);

      MDB(2,fSCHED) MLog("INFO:     cannot resume job %s, allocated nodes not available (node '%s' state '%s')\n",
        J->Name,
        (N != NULL) ? N->Name : "???",
        (N != NULL) ? MNodeState[N->State] : "???");

      if ((EMsgArray != NULL) && (EMsgArray[jindex] != NULL))
        {
        sprintf(EMsgArray[jindex], 
                "allocated nodes not available (node '%s' state '%s')",
                (N != NULL) ? N->Name : "???",
                (N != NULL) ? MNodeState[N->State] : "???");
        }

      J->RsvAList = OrigRsvAList;

      if (J->RemainingTime > 0)
        {
        MJobSetAttr(J,mjaReqAWDuration,(void **)&OrigWCLimit,mdfLong,mSet);
        }

      /* check if we should requeue job */

      if ((MSched.MaxSuspendTime > 0) &&
          (J->SWallTime >= (long)MSched.MaxSuspendTime) &&
          (MJobRequeue(J,NULL,NULL,NULL,NULL) == SUCCESS))
        {
        J->SWallTime = 0; /* Reset suspended time */
        }

      continue;
      }

    InHardPolicySchedulingCycle = FALSE;

    if (J->RemainingTime > 0)
      {
      MJobSetAttr(J,mjaReqAWDuration,(void **)&OrigWCLimit,mdfLong,mSet);
      }

    J->RsvAList = OrigRsvAList;

    bmunset(&J->IFlags,mjifIsResuming);

    /* all nodes support job */

    if (MJobResume(J,EMsg,NULL) == FAILURE)
      {
      MDB(2,fSCHED) MLog("INFO:     cannot resume job %s (RM rejected request - %s)\n",
        J->Name,
        EMsg);

      /* NOTE:  there is the possibility of race conditions where the highest
                priority job cannot be resumed but by the time the next highest
                priority job is considered, the RM race condition is over and the
                lower priority job is started rather than the highest priority one

                Setting the state of the job's first allocated node to Busy will
                prevent this but may have the side-affect of blocking other 
                potential jobs from properly starting
      */

      if ((EMsgArray != NULL) && (EMsgArray[jindex] != NULL) && (EMsg[0] != 0))
        {
        MUStrCpy(EMsgArray[jindex],EMsg,MMAX_LINE);
        }

      if (DisableFailingNode == TRUE)
        {
        if (MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS)
          { 
          MNodeSetState(N,mnsBusy,FALSE);
          }
        }
      }    /* END if (MJobResume(J,EMsg,NULL) == FAILURE) */
    else
      {
      RJobs++;
      }
    }      /* END for (jindex) */

  if (bmisset(&MSched.Flags,mschedfJobResumeLog))
    {
    MDB(7,fSCHED) MLog("INFO:     reducing LOGLEVEL after suspended job scheduling\n");

    mlog.Threshold = OrigLogLevel;
    }

  if(RJ != NULL)
    *RJ = RJobs;

  MUFree(&NodeMap);

  MNLMultiFree(MNodeList);

  return(SUCCESS);
  }  /* END MQueueScheduleSJobs() */





#define MDEF_GHOSTPREEMPTTIME  120

/**
 * Validate high-level queue status.
 *
 * @see MSysMainLoop() - parent
 * @see MNodeCheckStatus() - peer
 * @see MRsvCheckStatus() - peer
 * @see MQueueUpdateCJobs() - child - purge old CJob records
 *
 * NOTE:  This routine does the folloing:
 *  - purge stale jobs not reported by RM
 *  - cancel non-queue jobs which did not start
 *  - cancel jobs which can never start due to internal dependency violations
 *  - adjust walltime for time malleable jobs
 *  - process cancelled jobs which are not terminating 
 *  - purge CJob records which have expired
 */

int MQueueCheckStatus(void)
 
  {
  job_iter JTI;

  int index;
  int ReasonList[marLAST];
 
  mjob_t *SrcQ[2];
  mjob_t *tmpQ[2];
 
  mln_t *JobsComletedList = NULL;
  mln_t *JobsRemovedList = NULL;
  mln_t *JobListPtr = NULL;

  mjob_t  *J;
 
  char DeferMessage[MMAX_LINE];

  long Delta = 0;
  
  mbool_t DoPurge;

  /* hash of jobs to delete after done iterating through the job's list, 
   * and related variables 
   */
  mhash_t JobsToDelete;
  mhashiter_t Itr;
  mjob_t *ToDelete = NULL;
  mbool_t JobsToDeleteHashIsInitialized = FALSE;

  const int initialArraySize = 5;
  marray_t PreemptorJobs;

  int rc = SUCCESS;

  const char *FName = "MQueueCheckStatus";
 
  MDB(3,fSTRUCT) MLog("%s()\n",
    FName);
 
  /* initialize the jobs to delete hash table or record a failure */
  if ((MUHTCreate(&JobsToDelete,10) == SUCCESS))
    {
    if ((MUHTIterInit(&Itr) == SUCCESS))
      JobsToDeleteHashIsInitialized = TRUE;
    else
      MUHTFree(&JobsToDelete,FALSE,NULL);
    }

  if (!JobsToDeleteHashIsInitialized)
    {
    MDB(1,fSTRUCT) MLog("ERROR:    unexpected HT initialization error in MQueueCheckStatus.\n");
    }

  if (MSched.GuaranteedPreemption == TRUE)
    MUArrayListCreate(&PreemptorJobs,sizeof(mjob_t *),initialArraySize);

  MJobIterInit(&JTI);

  MULLCreate(&JobsComletedList);
  MULLCreate(&JobsRemovedList);

  /* handle data staging, purge expired jobs */

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    bmunset(&J->IFlags,mjifPreemptFailed);
    bmclear(&J->EligiblePAL);
 
    MDB(7,fSTRUCT) MLog("INFO:     checking purge criteria for job '%s'\n",
      J->Name);

    if (MSched.OptimizedJobArrays == TRUE)
      {
      if ((J->Array != NULL) && bmisset(&J->Array->Flags,mjafDynamicArray))
        {
        MJobArrayFree(J);
        MJGroupFree(J);
        }

      bmunset(&J->Flags,mjfArrayJob);
      bmunset(&J->SpecFlags,mjfArrayJob);
      bmunset(&J->IFlags,mjifIsArrayJob);
      }

    /* Step 1.0  Purge Stale and Internally Cancelled Jobs */

    DoPurge = MBNOTSET;

    if (bmisset(&J->IFlags,mjifPurge))
      {
      MDB(7,fSTRUCT) MLog("INFO:     purge flag set for job '%s'\n",
        J->Name);

      DoPurge = TRUE;
      }

    if (DoPurge == MBNOTSET)
      {
      if (J->DestinationRM != NULL)
        {
        if (J->DestinationRM->WorkloadUpdateIteration != MSched.Iteration)
          {
          /* job is NOT up-to-date */

          DoPurge = FALSE;
          }
        }
      else if (J->SubmitRM != NULL)
        {
        if (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue))
          {
          if ((MJOBISCOMPLETE(J) == TRUE) && 
              ((long)MSched.Time > J->StateMTime + 15))
            {
            /* add job to completed list to be removed after exiting this loop */

            MULLAdd(&JobsComletedList,J->Name,(void *)J,NULL,NULL);

            continue;
            }

          /* do not purge local queue jobs */

          DoPurge = FALSE;
          }
        }
      }

    if (DoPurge == MBNOTSET)
      {
      /* NOTE:  Delta should be calculated from beginning of workload query to J->ATime
                in case scheduling, job start, or other factors take an unexpected long 
                amount of time
      */

      if ((J->DestinationRM != NULL) && (J->DestinationRM->WorkloadUpdateTime > 0))
        Delta = J->DestinationRM->WorkloadUpdateTime - J->ATime;
      else 
        Delta = (long)MSched.Time - J->ATime;

      /* NOTE:  if purge time set to '-1', purge job if delta > 0 */

      if (((long)MSched.JobPurgeTime == -1) && (Delta > 0))
        {
        DoPurge = TRUE;
        }
      else 
        {
        /* llnl has had jobs prematurely purged on their grid. In these cases the Delta value was approximately twice the
         * size of the calculated purge time. To work around this problem a multiplier has been added for remote systems
         * to increase the calculated length of time before a job will be purged.*/

        int RemoteMultiplier = 1;

        if (J->DestinationRM != NULL)
          {
          RemoteMultiplier = 4;
          }

        if (Delta > MAX(MAX(15,MSched.MaxRMPollInterval) * RemoteMultiplier,(long)MSched.JobPurgeTime))
          {
          DoPurge = TRUE;
          }
        }
      }    /* END if (DoPurge == MBNOTSET) */

    /*
     * llnl has had idle jobs purged after a failed workload query. This should not happen so here is additional
     * logging to help troubleshoot the problem.
     */

    if ((DoPurge == TRUE) && (J->DestinationRM != NULL)) 
      {
      if (J->DestinationRM->State == mrmsActive)
        {
        MDB(3,fSTRUCT) MLog("INFO:     job '%s' iteration %d, WIteration %d, WorkloadTime %ld, UMWIter %d, ATime %ld, PT %ld, PI %ld\n",
          J->Name,
          MSched.Iteration,
          J->DestinationRM->WorkloadUpdateIteration,
          J->DestinationRM->WorkloadUpdateTime,
          J->DestinationRM->U.Moab.WIteration,
          J->ATime,
          MSched.JobPurgeTime,
          MSched.MaxRMPollInterval);
        }

      if (J->DestinationRM->State != mrmsActive)
        {
        DoPurge = FALSE;

        MDB(2,fSTRUCT) MLog("WARNING:  RM %s not active, job '%s' not eligible for purging\n",
          J->DestinationRM->Name,
          J->Name);
        }
      else if ((MSched.JobPurgeTime == 0) &&
               (bmisset(&J->DestinationRM->Flags,mrmfSlavePeer)) && 
               (J->DestinationRM->NC == 0) && 
               (J->DestinationRM->TC == 0))  
        {
        /* If a job purge time is not specified in the config file then do not purge jobs that are on a slave moab that
           is reporting no resources (e.g. SLURM on the slave is down). We do not purge jobs if the slave moab
           is down so we want the same behavior if the slave moab is up but the RM on the slave is down. */

        DoPurge = FALSE;

        MDB(3,fSTRUCT) MLog("WARNING:  RM %s active but reporting no resources, job '%s' not eligible for purging\n",
          J->DestinationRM->Name,
          J->Name);
        }  /* END else if (MSched.JobPurgeTime == 0) ... */
      }    /* END if ((DoPurge == TRUE) && (J->DRM != NULL)) */

    if (DoPurge == TRUE)
      {
      /* job not detected - mark it completed */

      MDB(2,fSTRUCT) MLog("WARNING:  job '%s' in state '%s' no longer detected (Last Detected %ld > PurgeTime %ld)\n",
        J->Name,
        MJobState[J->State],
        Delta,
        MAX(MAX(15,(mulong)MSched.MaxRMPollInterval),MSched.JobPurgeTime)); 
 
      MDB(2,fSTRUCT) MLog("ALERT:    purging job '%s'\n",
        J->Name);

      if (MJOBISACTIVE(J) && (J->CompletionTime <= 0))
        {
        J->CompletionTime = MSched.Time;
        }
 
      if (J->Credential.A != NULL)
        {
        if (MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,NULL) == FAILURE)
          {
          MDB(3,fSCHED) MLog("WARNING:  Unable to register job deletion with accounting manager for job '%s'\n",
            J->Name);
          }
        }
 
      /* remove job from job list */

      /* Should we call MJobProcessCompleted() in all cases? */

      if ((MJOBISACTIVE(J) == TRUE) || (J->State == mjsIdle))
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"job not reported by RM %s for %ld seconds - job purged",
          (J->DestinationRM != NULL) ? J->DestinationRM->Name : "",
          Delta);

        MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

        if (J->State == mjsIdle)
          {
          /* job was not run - must have been externally cancelled */

          bmset(&J->IFlags,mjifWasCanceled);
          }

        /* add job to completed list to be removed after exiting this loop */

        MULLAdd(&JobsComletedList,J->Name,(void *)J,NULL,NULL);
        }
      else
        { 
        /* NOTE:  should additional stats be updated beyond cancel event? */

        MOWriteEvent((void *)J,mxoJob,mrelJobCancel,"job purged",MStat.eventfp,NULL);

        if (MSched.PushEventsToWebService == TRUE)
          { 
          MEventLog *Log = new MEventLog(meltJobCancel);
          Log->SetCategory(melcCancel);
          Log->SetFacility(melfJob);
          Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
          Log->AddDetail("msg","job purged");

          MEventLogExportToWebServices(Log);

          delete Log;
          }

        /* add job to removed list to be removed after exiting this loop */

        MULLAdd(&JobsRemovedList,J->Name,(void *)J,NULL,NULL);
        }

      /* job removed */

      continue;
      }  /* END if (DoPurge == TRUE) */

    /* Step 2.0 Enforce NoQueue and InternalDepend Clean-Up */
 
    if ((J->State == mjsIdle) || 
        (J->State == mjsDeferred) || 
        (J->State == mjsHold))
      {
      if (bmisset(&J->Flags,mjfNoQueue))
        {
        /* non-queue job was held or did not start immediately - remove */

        MDB(2,fSTRUCT) MLog("INFO:     cancelling No-Queue job '%s'\n",
          J->Name);
 
        if (J->Credential.A != NULL)
          {
          if (MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,NULL) == FAILURE)
            {
            MDB(3,fSCHED) MLog("WARNING:  Unable to register job deletion with accounting manager for job '%s'\n",
              J->Name);
            }
          }
 
        /* cancel job */
 
        if (J->EState == mjsDeferred) 
          {
          sprintf(DeferMessage,"SCHED_INFO:  job cannot run.  Reason: %s\n",
            MHoldReason[J->HoldReason]);
 
          MJobCancel(J,DeferMessage,FALSE,NULL,NULL);
          }
        else
          {
          MOQueueInitialize(SrcQ);
 
          SrcQ[0] = J;
          SrcQ[1] = NULL;
 
          memset(ReasonList,0,sizeof(ReasonList));
 
          if (MQueueSelectJobs(
                SrcQ,
                tmpQ,
                mptHard,
                MSched.M[mxoJob],
                MSched.JobMaxTaskCount,
                MMAX_TIME,
                -1,
                ReasonList,
                FALSE,
                FALSE,
                FALSE,
                TRUE,
                FALSE) == FAILURE)
            {
            strcpy(DeferMessage,"SCHED_INFO:  job cannot run.  Reason: cannot select job\n");
            }
          else if (tmpQ[0] == NULL)
            {
            for (index = 0;index < marLAST;index++)
              {
              if (ReasonList[index] != 0)
                break;
              }
 
            if (index != marLAST) 
              {
              sprintf(DeferMessage,"SCHED_INFO:  job cannot run.  Reason: %s\n",
                MAllocRejType[index]);
              }
            else
              {
              strcpy(DeferMessage,"SCHED_INFO:  job cannot run.  Reason: policy violation\n");
              }
            }
          else
            {
            strcpy(DeferMessage,"SCHED_INFO:  insufficient resources to run job\n");
            }
 
          MJobCancel(J,DeferMessage,FALSE,NULL,NULL);
          }  /* END else (J->EState == mjsDeferred)   */
        }    /* END if (bmisset(&J->Flags,mjfNoQueue))  */

#if 0  /* this code is prone to errors and infinite loops, not only that but the idea
          of removing a job automatically because its dependency cannot be satisfied
          is just a really, really bad idea */
      if ((J->Depend != NULL) && !bmisset(&J->IFlags,mjifRMManagedDepend))
        {
        mbool_t DoRemove;

        if ((MJobCheckDependency(J,NULL,FALSE,&DoRemove,NULL) == FAILURE) &&
            (DoRemove == TRUE))
          {
          rc = FAILURE;
          strcpy(DeferMessage,"SCHED_INFO:  dependency cannot be satisfied\n");

          if (JobsToDeleteHashIsInitialized)
            {
            rc = MJobWorkflowToHash(&JobsToDelete,J);
            }

          if (rc == FAILURE)
            {
            MDB(1,fSTRUCT)  MLog("ALERT:    could not delete workflow rooted at %s because of hash table error\n",
              J->Name);
            }
          }
        }
#endif

      if ((J->Credential.Q != NULL) && 
          (J->Credential.Q->QTTriggerThreshold > 0) &&
          (MSched.Time - J->SystemQueueTime > (mulong)J->Credential.Q->QTTriggerThreshold))
        {
        MOReportEvent(J->Credential.Q,J->Credential.Q->Name,mxoQOS,mttFailure,MSched.Time,TRUE);
        } 

      MJobCheckPriorityAccrual(J,NULL);
      }      /* END if ((J->State == mjsIdle) || ...) */

    if (MJOBISACTIVE(J) && (J->OrigMinWCLimit > 0))
      {
      /* evaluate time-malleable jobs */

      if ((J->DestinationRM != NULL) &&
          ((J->StartTime + J->WCLimit <= MSched.Time + MSched.MaxRMPollInterval) ||
           ((J->WCLimit == J->OrigMinWCLimit) && (bmisset(&J->DestinationRM->IFlags,mrmifMaximizeExtJob)))))
        {
        mulong MaxExtension;
        mulong MinExtension;

        /* extendible job will reach wclimit within next iteration - try to extend it */

        if (J->OrigWCLimit <= 0)
          {
          J->OrigWCLimit = J->SpecWCLimit[0];
          }

        MinExtension = (J->DestinationRM->MinJobExtendDuration > 0) ?
          J->WCLimit + J->DestinationRM->MinJobExtendDuration :
          J->WCLimit + MCONST_HOURLEN;

        MaxExtension = MIN(J->OrigWCLimit,
          (J->DestinationRM->MaxJobExtendDuration > 0) ?
          J->OrigMinWCLimit + J->DestinationRM->MaxJobExtendDuration :
          J->OrigMinWCLimit + MCONST_HOURLEN);

        if (MJobAdjustWallTime(
              J,
              MinExtension,
              MaxExtension,
              (J->DestinationRM->MinJobExtendDuration > 0) ? TRUE : FALSE,
              FALSE,
              TRUE,
              (!bmisset(&J->DestinationRM->IFlags,mrmifDisableIncrExtJob)),
              NULL) == SUCCESS)
          {
          J->MinWCLimit = J->WCLimit;

          if (!bmisset(&J->DestinationRM->IFlags,mrmifDisablePreemptExtJob))
            {
            bmset(&J->SpecFlags,mjfPreemptee);
            bmset(&J->Flags,mjfPreemptee);
            }
          }
        }
      }      /* END if (MJOBISACTIVE(J) && (J->MinWCLimit > 0)) */

    if (bmisset(&J->IFlags,mjifWasCanceled) && 
       (MSched.Time - J->StateMTime > 10 * MCONST_MINUTELEN))
      {
      /* job was canceled but is not being removed */

      if (!bmisset(&J->NotifySentBM,mjnsCancelDelay))
        {
        mnode_t *N;

        char tmpLine[MMAX_LINE];
        char tmpMsg[MMAX_LINE];

        /* notify admin */

        snprintf(tmpLine,sizeof(tmpLine),"moab event - %s (%s:%s)",
          "cancelfailed",
          MXO[mxoJob],
          J->Name);

        MNLGetNodeAtIndex(&J->NodeList,0,&N);

        snprintf(tmpMsg,sizeof(tmpMsg),"job cancel request was sent to RM %s %ld seconds ago.  job has not yet been removed from the queue.  This may block other jobs from running.  Check resource manager or job's master node %s\n",
          (J->DestinationRM != NULL) ? J->DestinationRM->Name : "",
          (MSched.Time - J->StateMTime),
          (N != NULL) ? N->Name : "");

        MSysSendMail(
          MSysGetAdminMailList(1),
          NULL,
          tmpLine,
          NULL,
          tmpMsg);

        bmset(&J->NotifySentBM,mjnsCancelDelay);
        }

      if ((J->DestinationRM != NULL) && (J->DestinationRM->CancelFailureExtendTime > 0))
        {
        /* extend job */

        MJobAdjustWallTime(
          J,
          J->WCLimit + J->DestinationRM->CancelFailureExtendTime,
          J->WCLimit + J->DestinationRM->CancelFailureExtendTime,
          FALSE,  /* modify rm */
          FALSE,
          TRUE,
          TRUE,
          NULL);  /* fail on conflict */
        }
      }      /* END if (bmisset(&J->IFlags,mjifWasCanceled) && ...) */

    if ((bmisset(&J->Flags,mjfIgnIdleJobRsv)) && (J->Rsv != NULL))
      {
      mrsv_t *R;
      mjob_t *IJ;

      mnl_t tmpNL;
      rsv_iter RTI;

      /* these jobs possess 'ghost' reservations which can overlap 'real'
         idle rsvs.  If an overlapping rsv is detected and the overlap occurs
         with MDEF_GHOSTPREEMPTTIME, terminate the IgnIdleJobRsv job */

      /* NOTE:  if global MRE table has appropriate event, search job's node list 
                looking for resource overlap */

      /* NOTE:  There may be another IgnIdleJobRsv job which could possibly be a better
                candidate for preemption, however, we are only looking for overlapping reservations.
                For example, this ignidlejobrsv job may have 10 nodes and another running ignidlejobrsv job
                may have five nodes, but if a 3 node preemptor job has an overlapping reservation
                with the 10 node job, then that is the job that will be preempted in this routine. */

      MNLInit(&tmpNL);

      MRsvIterInit(&RTI);
      while (MRsvTableIterate(&RTI,&R) == SUCCESS)
        {
        if ((R->Type != mrtJob) || (R->J == NULL))
          continue;

        if (R->StartTime > MSched.Time + MDEF_GHOSTPREEMPTTIME)
          continue;

        /* idle job rsv found which overlaps in time */

        IJ = R->J;

        if (IJ == J)
          continue;     /* ignore self */

        if (MJOBISACTIVE(IJ) == TRUE)
          continue;     /* job is already active so there shouldn't be a problem */
    
        if (bmisset(&IJ->Flags,mjfIgnIdleJobRsv))
          continue;     /* ignore other ghost jobs */

        if (MJobIsPreempting(IJ) == TRUE)
          continue;     /* ignore jobs that are preempting */

        if (MJobCanPreemptIgnIdleJobRsv(IJ,J,IJ->CurrentStartPriority,NULL) == FALSE) 
          continue;
 
        /* determine if idle job rsv overlaps in space */

        MNLAND(&R->NL,&J->NodeList,&tmpNL);

        if (!MNLIsEmpty(&tmpNL))
          {
          char tmpEMsg[MMAX_LINE];

          /* active job and idle rsv overlap in space */

          if (MJobPreempt(
                J,        /* preemptee */
                IJ,       /* preemptor */
                NULL,
                NULL,
                IJ->Name, /* requestor */
                mppNONE,
                FALSE,
                NULL,
                tmpEMsg,  /* O */
                NULL) == FAILURE)
            {
            MDB(1,fSCHED) MLog("ERROR:    job %s cannot preempt 'idlejobrsv' job %s - %s\n",
              IJ->Name,
              J->Name,
              tmpEMsg);
            }
          else
            {
            MDB(4,fSCHED) MLog("INFO:     job %s preempted by 'idlejobrsv' job %s (with overlapping reservation) which will start in %ld seconds\n",
              J->Name,
              IJ->Name,
              R->StartTime - MSched.Time);

            if (MSched.GuaranteedPreemption == TRUE)
              MUArrayListAppendPtr(&PreemptorJobs,IJ);
            }
          }    /* END if (tmpNL[0].N != NULL) */
        }      /* END for (reindex) */

      MNLFree(&tmpNL);
      }        /* END if (bmisset(&J->IFlags,mjifIgnIdleJobRsv)) */
    }          /* END for (jindex) */

  if (MSched.GuaranteedPreemption == TRUE)
    {
    /* Lock preemptors into reservation so that it doesn't lose its
     * reservation and  let other jobs preempt ahead of it. MJobStartRsv
     * will keep the job on the reservation. */

    int jobIndex;
    mjob_t *preemptor;

    for (jobIndex = 0;jobIndex < PreemptorJobs.NumItems;jobIndex++)
      {
      preemptor = (mjob_t *)MUArrayListGetPtr(&PreemptorJobs,jobIndex);

      preemptor->InRetry = TRUE;

      bmset(&preemptor->Flags,mjfPreempted);
      bmset(&preemptor->SpecFlags,mjfPreempted);
      }

    MUArrayListFree(&PreemptorJobs);
    } /* END if (MSched.GuaranteedPreemption == TRUE) */

  /* delete completed jobs */

  while (MULLIterate(JobsComletedList,&JobListPtr) == SUCCESS)
    MJobProcessCompleted((mjob_t **)&JobListPtr->Ptr);

  /* delete removed jobs */

  while (MULLIterate(JobsRemovedList,&JobListPtr) == SUCCESS)
    MJobRemove((mjob_t **)&JobListPtr->Ptr);

  if (JobsComletedList != NULL)
    MULLFree(&JobsComletedList,NULL);

  if (JobsRemovedList != NULL)
    MULLFree(&JobsRemovedList,NULL);

  if (JobsToDeleteHashIsInitialized == TRUE)
    {
    while (MUHTIterate(&JobsToDelete,NULL,(void **)&ToDelete,NULL,&Itr) == SUCCESS)
      {
      switch (MSched.DependFailPolicy)
        {
        case mdfpCancel:

          rc = MJobCancel(ToDelete,"SCHED_INFO:  dependency cannot be satisfied\n",FALSE,NULL,NULL) && rc;
          break;

        /* MSched.DependFailPolicy defaults to mdfNONE.  The docs specify mdfpHold.  */

        case mdfpHold:
        default:

          rc = MJobSetHold(ToDelete,mhBatch,0,mhrMissingDependency,"DEPENDFAILUREPOLICY hold.");
          break;
        }
      }

    MUHTFree(&JobsToDelete,FALSE,NULL);
    }

  /* purge completed jobs */

  MQueueUpdateCJobs();
 
  /* purge subjobs */
 
  /* NYI */
 
  if (bmisset(&MSched.Flags,mschedfTrimTransactions))
    MTransCheckStatus();

  return(SUCCESS);
  }  /* END MQueueCheckStatus() */





/**
 * Add active job to MAQ table, adjust stats, and remove from eligible job table.
 *
 * @see MStatAdjustEJob() - peer - adds eligible job
 * @see MAQ[] table
 * @see MStatUpdateActiveJobUsage() - child
 * @see MQueueRemoveAJob() - peer - remove active job
 * @see MRMUpdate() - parent - add 'stale' jobs not updated this iteration
 * @see MRMJobPostLoad() - parent - add all active jobs
 * @see MRMJobPostUpdate() - parent - add all active jobs updated this iteration (Step 4.4/4.7)
 * @see MJobStart() - parent - add job which was started
 * @see MJobResume() - parent - add job which was resumed
 *
 * NOTE:  MAQ[] table cleared each iteration at the start of MRMUpdate()
 *
 * @param J (I)
 */

int MQueueAddAJob(
 
  mjob_t *J)  /* I */
 
  {
  const char *FName = "MQueueAddAJob";
 
  MDB(4,fSCHED) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* add the job to the active hash table so we can quickly check for duplicates */

  MUHTAdd(&MAQ,J->Name,J,NULL,NULL);

  /* only used to avoid compile errors */

  MStatUpdateActiveJobUsage(J); 
 
  /* if job is started by scheduler or is found already running */
 
  if ((J->EState == mjsIdle) || (J->CTime == (long)MSched.Time))
    {
    /* new job detected */
 
    /* if job started by scheduler set EState to Starting.      */
    /* if job discovered in active state, set to whatever is detected. */
 
    if (J->State == mjsIdle)
      J->EState = mjsStarting;
    else
      J->EState = J->State;
    }
  else
    {
    MDB(6,fSCHED) MLog("INFO:     previously detected active job '%s' added to MAQ\n",
      J->Name);
    }

  if (bmisset(&J->Flags,mjfPreemptee))
    {
    if (J->Credential.Q != NULL) 
      MSched.QOSPreempteeCount[J->Credential.Q->Index]++;
    else
      MSched.QOSPreempteeCount[0]++;
    }

  MDB(5,fSCHED) MLog("INFO:     job '%s' added to MAQ\n",
    J->Name);

  MDB(8,fSCHED)
    {
    MAQDump();
    }

  return(SUCCESS);
  }  /* END MQueueAddAJob() */





#define MMAX_RSVDEADLINEINTERVALS 4

/**
 * Schedule reserved jobs as reservations become active.
 *
 * @param Q (I) [terminated w/-1] 
 * @param P (I) partition - optional 
 */

int MQueueScheduleRJobs(
 
  mjob_t **Q,     /* I:  prioritized list of feasible jobs (terminated w/-1) */
  mpar_t  *P)     /* I;  partition - optional */

  {
  int      jindex;
 
  int      StartJC;
 
  mjob_t  *J;
 
  mrsv_t  *R;
 
  mbool_t HaveNewDeadLineJob;

  int index;

  mulong tmpLastStart;
  mulong tmpSpanUnit;

  long StartTime;

  char PartitionInfo[MMAX_LINE];

  const char *FName = "MQueueScheduleRJobs";

  MDB(2,fSCHED) MLog("%s(Q,%s)\n",
    FName,
    (P == NULL) ? "NULL" : P->Name);

  if (Q == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  jobs are evaluated in priority order */

  /* step through all jobs
       remove transient reservations (priority, deadline, threshold based, etc)
     step through all jobs
       restore job deadline reservations in priority order at latest possible time
     step through all jobs
       create priority reservations
       add new deadline requests 
       and slide forward deadline reservations  
  */

  /* initialize */

  PartitionInfo[0] = '\0';

  if (P != NULL)
    {
    sprintf(PartitionInfo,"partition %s",P->Name);
    }
  else
    {
    sprintf(PartitionInfo,"partition %s",MPar[0].Name);
    }

  /* purge transient reservations (priority, QOS, and deadline) */
 
  StartJC = 0;
 
  for (jindex = 0;Q[jindex] != NULL;jindex++)
    {
    J = Q[jindex];
 
    if ((J == NULL) || (J->State != mjsIdle))
      continue;

    if (J->Rsv == NULL)
      continue;

    if (bmisset(&J->Flags,mjfRsvMap))
      {
      continue;
      }

    if (bmisset(&J->IFlags,mjifRunAlways))
      {
      /* job will be force-started in MQueueScheduleIJobs() */

      continue;
      }

    if ((P != NULL) && (J->Rsv->PtIndex != P->Index))
      {
      /* job reservation not in the partition we are processing */

      continue;
      }

    /* only evaluate idle jobs with reservations */
 
    if ((MSched.Time >= J->Rsv->StartTime) && (J->State == mjsIdle))
      {
      if ((MSched.MaxJobStartPerIteration >= 0) &&
          (MSched.JobStartPerIterationCounter >= MSched.MaxJobStartPerIteration))
        {
        MDB(4,fSCHED) MLog("ALERT:    job '%s' blocked by JOBMAXSTARTPERITERATION policy in %s (%d >= %d)\n",
          J->Name,
          FName,
          MSched.JobStartPerIterationCounter,
          MSched.MaxJobStartPerIteration);

        break;
        }

      /* the time to schedule has arrived, and there is no unfinished data staging */

      if (!bmisset(&J->IFlags,mjifDataDependency) || (J->DestinationRM == NULL))
        {
        char DString[MMAX_LINE];

        MULToDString((mulong *)&J->Rsv->StartTime,DString);

        MDB(2,fSCHED) MLog("INFO:     located job '%s' in partition %s reserved to start %s",
          J->Name,
          PartitionInfo,
          DString);

        if ((J->Depend != NULL) &&
            (J->Depend->Type == mdJobSyncMaster))
          {
          /* NOTE: MJobSyncStartRsv() calls MJobStartRsv() */

          if (MJobSyncStartRsv(J) == SUCCESS)
            {
            StartJC ++;
            }
          }
        else
          {
          if (MJobStartRsv(J) == SUCCESS)
            {
            StartJC ++;
            }
          else
            {
            /* cannot restart, re-schedule */

            MJobAdjustRsvStatus(J,P,TRUE);
            }
          }

        continue;
        }
      else
        {
        /* data stage is still pending, so adjust system job start time */

        /* NYI */
        }
      }

    if (MSched.RequeueRecoveryRatio > 0.0)
      {
      if ((J->Credential.Q != NULL) &&
           bmisset(&J->Credential.Q->Flags,mqfPreemptorX))
        {
        MJobShouldRequeue(J,P,TRUE);
        }
      }

    /* remove future transient reservations */

    MJobAdjustRsvStatus(J,P,TRUE);
    }  /* END for (jindex) */

  /* restore latest possible deadline reservations */

  HaveNewDeadLineJob = FALSE;

  for (jindex = 0;Q[jindex] != NULL;jindex++)
    {
    J = Q[jindex];

    if ((J == NULL) || (J->State != mjsIdle))
      continue;

    if (J->RType != mjrNONE)
      {
      /* restore reservation if needed */

      MJobAdjustRsvStatus(J,P,FALSE);

      continue;
      }

    if ((J->Credential.Q == NULL) || 
        !bmisset(&J->Credential.Q->Flags,mqfDeadline))
      {
      J->CMaxDateIsActive = FALSE;

      continue;
      }

    if (J->CMaxDateIsActive == MBNOTSET)
      {
      HaveNewDeadLineJob = TRUE;

      continue;
      }

    if (J->CMaxDateIsActive != TRUE)
      {
      /* Qos does not allow deadline job, ignore deadline */

      continue;
      }

    /* restore existing deadline jobs */

    MDB(5,fSCHED) MLog("INFO:     evaluating new deadline rsv for job %s in %s() in partition %s\n",
      J->Name,
      FName,
      PartitionInfo);

    tmpLastStart = J->CMaxDate - J->SpecWCLimit[0];
    tmpSpanUnit  = (tmpLastStart - MSched.Time) / (MMAX_RSVDEADLINEINTERVALS - 1);

    if (tmpLastStart < MSched.Time)
      {
      /* Start time is in the past, no possible way to meet deadline */

      MJobApplyDeadlineFailure(J);
      }

    if ((J == NULL) || J->CMaxDateIsActive != MBNOTSET)
      {
      continue;
      }    

    for (index = 0;index < MMAX_RSVDEADLINEINTERVALS;index++)
      {
      StartTime = tmpLastStart - tmpSpanUnit * index;

      if (MJobReserve(&J,P,StartTime,mjrDeadline,FALSE) == SUCCESS)
        {
        char TString[MMAX_LINE];

        MULToTString(J->Rsv->StartTime - MSched.Time,TString);

        MDB(5,fSCHED) MLog("INFO:     create new deadline rsv for job %s in %s %s in %s()\n",
          J->Name,
          TString,
          PartitionInfo,
          FName);

        break;
        }
      }  /* END for (index) */
    }    /* END for (jindex) */

  if (HaveNewDeadLineJob == TRUE)
    {
    for (jindex = 0;Q[jindex] != NULL;jindex++)
      {
      /* attempt to add new deadline jobs */

      J = Q[jindex];
  
      if ((J == NULL) || (J->State != mjsIdle))
        continue;
  
      if ((J->Credential.Q == NULL) || 
          !bmisset(&J->Credential.Q->Flags,mqfDeadline) ||
         (J->CMaxDateIsActive != MBNOTSET))
        {
        continue;
        }

      /* attempt to add new deadline job */
  
      MDB(5,fSCHED) MLog("INFO:     evaluating new deadline rsv for job %s partition %s in %s()\n",
        J->Name,
        PartitionInfo,
        FName);

      tmpLastStart = J->CMaxDate - J->SpecWCLimit[0];
      tmpSpanUnit  = (tmpLastStart - MSched.Time) / (MMAX_RSVDEADLINEINTERVALS - 1);

      for (index = 0;index < MMAX_RSVDEADLINEINTERVALS;index++)
        {
        StartTime = tmpLastStart - tmpSpanUnit * index;

        if (MJobReserve(&J,P,StartTime,mjrDeadline,FALSE) == SUCCESS)
          {
          char TString[MMAX_LINE];

          MULToTString(J->Rsv->StartTime - MSched.Time,TString);

          MDB(5,fSCHED) MLog("INFO:     create new deadline rsv for job %s in %s %s in %s()\n",
            J->Name,
            TString,
            PartitionInfo,
            FName);

          break;
          }
        }  /* END for (index) */

      if (index == MMAX_RSVDEADLINEINTERVALS)
        {
        /* cannot create deadline w/in reservation */

        /* try again after prioritization of existing reservations */

        /* NO-OP */
        }  /* END if (index == MMAX_RSVDEADLINEINTERVALS) */
      }    /* END for (jindex) */
    }      /* END if (HaveNewDeadLineJob == TRUE) */

  /* optimize job reservations */

  for (jindex = 0;Q[jindex] != NULL;jindex++)
    {
    J = Q[jindex];

    if ((J == NULL) || (J->State != mjsIdle))
      continue;

    if (bmisset(&J->IFlags,mjifRunAlways))
      {
      /* job will be force-started in MQueueScheduleIJobs() */

      continue;
      }

    if (bmisset(&J->Flags,mjfRsvMap))
      {
      continue;
      }

    /* NOTE:  reservation restoration code broken for long time (pre Moab 4.2.0) */

    /* code below may not be correct */

    if (J->Rsv == NULL)
      {
      /* This check prevents jobs from getting special (ex. reserve always) reservations.
       * With a CurrentHighest reservation policy all reservations will be destroyed above
       * and will be NULL here. If the reservation policy is HIGHEST then the jobs could
       * pass this check. If a job does get a reservation in MQueueScheduleRJobs then it's 
       * possible that preemption won't work for the job because MQueueScheduleIJobs skips 
       * job with existing reservations. Preemptor jobs in a reserve alaways qos and handled
       * specially in this case in MQueueScheduleIJobs. */

      /* create new reservations */

      if ((bmisset(&J->IFlags,mjifReserveAlways) || (MJOBISINRESERVEALWAYSQOS(J))) && 
          (!bmisset(&J->IFlags,mjifReqTimeLock)))
        {
        /* reserve always jobs will get reservation every iteration. Preemptor jobs in a 
         * reserve always qos will get a chance to preempt in MQueueScheduleIJobs */

        if (MJobReserve(&J,P,0,mjrQOSReserved,FALSE) == FAILURE)
          {
          MDB(1,fSCHED) MLog("ALERT:    cannot create %s reservation for job %s %s\n",
            MJRType[mjrQOSReserved],
            (J != NULL) ? J->Name : "(deleted)",
            PartitionInfo);

          continue;
          }

        if ((MSched.Time >= J->Rsv->StartTime) && (J->State == mjsIdle))
          {
          if (MJobStartRsv(J) == SUCCESS)
            {
            StartJC ++;

            continue;
            }
          }
        } /* END if (bmisset(&J->IFlags,mjifReserveAlways) || ... */

      continue;
      }  /* END if (J->R == NULL) */

    MDB(5,fSCHED) MLog("INFO:     restoring transient rsvs for job %s in %s()\n",
      J->Name,
      FName);
 
    /* bind job to specified reservation */
 
    if (J->ReqRID != NULL)
      {
      if ((MRsvFind(J->ReqRID,&R,mraNONE) == SUCCESS) ||
          (MRsvFind(J->ReqRID,&R,mraRsvGroup) == SUCCESS))
        {
        /* non-preemptible */
 
        MJobReleaseRsv(J,TRUE,TRUE);
 
        if (MJobReserve(&J,P,0,mjrUser,FALSE) == SUCCESS)
          {
          J->Rsv->Priority = J->PStartPriority[(P != NULL) ? P->Index : 0];
          }
        }
      }
 
    /* create new reservations */

    if ((bmisset(&J->IFlags,mjifReserveAlways) || (MJOBISINRESERVEALWAYSQOS(J))) && 
        (!bmisset(&J->IFlags,mjifReqTimeLock)))
      {
      MJobReleaseRsv(J,TRUE,TRUE);
 
      /* non-preemptible */
 
      if (MJobReserve(&J,P,0,mjrQOSReserved,FALSE) == FAILURE)
        {
        MDB(1,fSCHED) MLog("ALERT:    cannot create %s reservation for job %s %s\n",
          MJRType[mjrQOSReserved],
          (J != NULL) ? J->Name : "(deleted)",
          PartitionInfo);
 
        continue;
        }
      }
    else if ((J->CMaxDate != MMAX_TIME) && (J->CMaxDate >= MSched.Time))
      {
      MJobReleaseRsv(J,TRUE,TRUE);
 
      /* non-preemptible */

      if (MJobReserve(&J,P,0,mjrDeadline,FALSE) == FAILURE)
        {
        MDB(1,fSCHED) MLog("ALERT:    cannot create %s reservation for job %s %s\n",
          MJRType[mjrDeadline],
          (J != NULL) ? J->Name : "(deleted)",
          PartitionInfo);

        MJobApplyDeadlineFailure(J);

        continue;
        }
     
      /* deadline reservation successfully created */
 
      J->CMaxDateIsActive = TRUE;
      }
    else if (J->RType == mjrPriority)
      {
      MJobAdjustRsvStatus(J,P,TRUE);
      }      /* END if (J->RType == mjrPriority) */
 
    /* continue if no reservation */
 
    if (J->Rsv == NULL)
      {
      continue;
      }
 
    /* if the time to schedule has arrived... */
 
    if ((MSched.Time >= J->Rsv->StartTime) && (J->State == mjsIdle))
      {
      char DString[MMAX_LINE];

      MULToDString((mulong *)&J->Rsv->StartTime,DString);

      MDB(2,fSCHED) MLog("INFO:     located job '%s' reserved to start %30s in partition %s",
        J->Name,
        DString,
        PartitionInfo);

      if ((J->Depend != NULL) &&
          (J->Depend->Type == mdJobSyncMaster))
        {
        /* NOTE: MJobSyncStartRsv() calls MJobStartRsv() */

        if (MJobSyncStartRsv(J) == SUCCESS)
          {
          StartJC ++;
          }
        }
      else
        {
        if (MJobStartRsv(J) == SUCCESS)
          {
          StartJC ++;
          }
        }

      continue;
      }
    }      /* END for (jindex)   */

  /* NOTE:  attempt to add 'pending' deadline jobs if possible */

  /* slide forward deadline jobs in priority order */

  for (jindex = 0;Q[jindex] != NULL;jindex++)
    {
    J = Q[jindex];
                                                                                
    if ((J == NULL) || (J->State != mjsIdle))
      continue;

    if (J->Rsv == NULL)
      {
      if (J->CMaxDateIsActive == MBNOTSET)
        {
        if (MJobReserve(&J,P,0,mjrDeadline,FALSE) == FAILURE)
          {
          MDB(1,fSCHED) MLog("ALERT:    cannot create %s reservation for job %s %s\n",
            MJRType[mjrDeadline],
            (J != NULL) ? J->Name : "(deleted)",
            PartitionInfo);

          if (J == NULL)
            continue;

          MJobApplyDeadlineFailure(J);

          continue;
          }    /* END if (MJobReserve(&J,NULL,0,mjrDeadline,FALSE) == FAILURE) */
        else
          {
          J->CMaxDateIsActive = TRUE;
          }
        }      /* END if (J->CMaxDateIsActive == MBNOTSET) */

      continue;
      }        /* END if (J->R == NULL) */
    }          /* END for (jindex) */

  /* report summary results */
 
  MDB(2,fSCHED) MLog("INFO:     %d reserved jobs started this iteration in partition %s\n", 
    StartJC,
    PartitionInfo);
 
  return(SUCCESS);
  }    /* END MQueueScheduleRJobs() */




/**
 * convert job index array to array of job pointers
 *
 * @param JPList (O) [populated]
 * @param JIList (I) [modified]
 */

int __MBFJListFromArray(

  mjob_t **JPList,  /* O (populated) */
  mjob_t **JIList)  /* I (modified) */

  {
  int index;
  int jindex;
 
  mjob_t *J;

  if ((JPList == NULL) || (JIList == NULL))
    {
    return(FAILURE);
    }

  index = 0;

  for (jindex = 0;JIList[jindex] != NULL;jindex++)
    {
    J = JIList[jindex];

    if ((J->Rsv != NULL) && (HardPolicyRsvIsActive == FALSE))
      {
      /* do not backfill reserved jobs */

      MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for backfill (rsv)\n", 
        J->Name);

      continue;
      }

    if (bmisset(&J->Flags,mjfClusterLock) &&
        bmisset(&J->Flags,mjfNoRMStart))
      {
      /* cannot schedule this job */

      MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for backfill (clusterlock)\n", 
        J->Name);

      continue;
      }

    if ((J->DestinationRM != NULL) &&
        bmisset(&J->DestinationRM->Flags,mrmfAutoStart))
      {
      /* job will be run by destination RM */

      MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for backfill (drm is autostart)\n", 
        J->Name);

      continue;
      }

    if ((J->Credential.Q != NULL) && (bmisset(&J->Credential.Q->Flags,mqfNoBF)))
      {
      /* job QOS disables backfill */

      MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for backfill (qos)\n", 
        J->Name);

      continue;
      }

    if (index != jindex)
      JIList[index] = JIList[jindex];

    JPList[index] = J;

    index++;

    if ((MPar[0].BFDepth > 0) &&
        (index >= MPar[0].BFDepth))
      {
      MDB(1,fSCHED) MLog("WARNING:  BFDEPTH reached, no more jobs will be backfilled this iteration\n");

      break;
      }
    }  /* END for (jindex) */

  JPList[index] = NULL;
  JIList[index] = NULL;

  MDB(5,fSTRUCT) MLog("INFO:     %d of %d jobs eligible for backfill\n",
    index,
    jindex);

  return(SUCCESS);
  }  /* END __MBFJListFromArray() */


#if 0 /* no longer needed */
/**
 * Sort a linked-list job array structure (like MJob).
 * 
 * @param J (first element in linked list)
 */

mjob_t *MQueueSort(

  mjob_t *list)

  {
  mjob_t *p, *q, *e, *tail, *oldhead;

  int insize, nmerges, psize, qsize, i;

  /*
   * Silly special case: if `list' was passed in as NULL, return
   * NULL immediately.
   */
  if (!list)
      return NULL;

  insize = 1;

  while (1) 
    {
    p = list;
    oldhead = list;                /* only used for circular linkage */
    list = NULL;
    tail = NULL;

    nmerges = 0;  /* count number of merges we do in this pass */

    while (p) 
      {
      nmerges++;  /* there exists a merge to be done */
      /* step `insize' places along from p */
      q = p;
      psize = 0;
      for (i = 0; i < insize; i++) 
        {
        psize++;

        q = (q->Next == oldhead ? NULL : q->Next);

        if (!q) break;
        }

      /* if q hasn't fallen off end, we have two lists to merge */
      qsize = insize;

      /* now we have two lists; merge them */
      while (psize > 0 || (qsize > 0 && q)) 
        {
        /* decide whether next element of merge comes from p or q */
        if (psize == 0) 
          {
          /* p is empty; e must come from q. */
          e = q; q = q->Next; qsize--;
          if (q == oldhead) 
            q = NULL;
          }
        else if (qsize == 0 || !q) 
          {
          /* q is empty; e must come from p. */
          e = p; p = p->Next; psize--;
          if (p == oldhead) 
            p = NULL;
          } 
        else if (__MQueueSortCmp(p,q) <= 0) 
          {
          /* First element of p is lower (or same);
           * e must come from p. */
          e = p; p = p->Next; psize--;
          if (p == oldhead) 
            p = NULL;
          } 
        else 
          {
          /* First element of q is lower; e must come from q. */
          e = q; q = q->Next; qsize--;
          if (q == oldhead) 
            q = NULL;
          }

        /* add the next element to the merged list */
        if (tail) 
          {
          tail->Next = e;
          } 
        else 
          {
          list = e;
          }

        /* Maintain reverse pointers in a doubly linked list. */
        e->Prev = tail;

        tail = e;
        } /* END while (psize > 0 || (qsize > 0 && q)) */

      /* now p has stepped `insize' places along, and q has too */
      p = q;
      }  /* END while (p) */

    tail->Next = list;

    list->Prev = tail;

    /* If we have done only one merge, we're finished. */
    if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
      {
      return list;
      }

    /* Otherwise repeat, merging lists twice the size */
    insize *= 2;
    }  /* END while (1) */
  }  /* END MQueueSort() */
#endif 





#define MQUEUE_COLLAPSE_THRESHOLD 2

#if 0 /* untested */
/**
 * Collapse queue if similar jobs are adjacent in the array.
 *
 * @param Q (I)
 */

int MQueueCollapse(

  mjob_t **Q)  /* I hi-low priority sorted array of job indicies */

  {
  mjob_t *J;
  mjob_t *NextJ;

  int jcount;
  int jindex;
  int qindex;
  int qstep;

  int NextJIndex;

  mjob_t **Matches = NULL;

  MJobListAlloc(&Matches);

  for (qindex = 0;Q[qindex] != NULL;qindex++)
    {
    if (Q[qindex + 1] == NULL)
      break;

    J = JobIndex[qindex];

    MJobArrayFree(J);
    MJGroupFree(J);

    Matches[0] = NULL;

    jindex = 0;

    /* step through adjacent jobs to see if they match */
    /* build array of jobs that match J */

    for (qstep = qindex + 1;JobIndex[qstep] != NULL;qstep++)
      {
      NextJ = MJob[JobIndex[qstep]];

      if (MJobIsMatch(J,NextJ) == FALSE)
        break;

      Matches[jindex] = qstep;

      jindex++;
      }  /* END for (qstep) */

    Matches[jindex] = -1;

    /* determine if we want to collapse */

    if (jindex < MQUEUE_COLLAPSE_THRESHOLD)
      {
      /* don't collapse */

      continue;
      }

    NextJIndex = qstep;

    jcount = jindex + 1; /* include self */

    jindex = 0;

    for (qstep = 0;Matches[qstep] != -1;qstep++)
      {
      NextJ = MJob[JobIndex[Matches[qstep]]];

      MJobArrayAddStep(J,NextJ,NULL,jcount,&jindex);

      /* remove NextJ from JobIndex (or just mark it for deletion) */

      JobIndex[Matches[qstep]] = 0;
      }  /* END for (qstep) */

    qindex = NextJIndex - 1;
    }  /* END for (qindex) */

  /* collapse JobIndex by removing 0s from the array */

  qindex = 0;
  qstep  = 0;

  while (JobIndex[qindex] != -1)
    {
    if (JobIndex[qindex] == 0)
      {
      qindex++;
      }
    else
      {
      JobIndex[qstep] = JobIndex[qindex];

      qstep++;
      qindex++;
      }
    }   /* END while (JobIndex[qindex] != -1) */

  JobIndex[qstep] = -1;

  MUFree((char **)&Matches);

  return(SUCCESS);
  }  /* END MQueueCollapse() */
#endif /* doesn't work */




int MAQDump()

  {
  mjob_t *J = NULL;

  mhashiter_t HTI;

  MUHTIterInit(&HTI);

  MLog("INFO:     active jobs: ");
 
  while (MUHTIterate(&MAQ,NULL,(void **)&J,NULL,&HTI) == SUCCESS)
    {
    MLogNH("[%s]",J->Name);
    }
 
  return(SUCCESS);
  }  /* END MAQDump() */




/**
 * Delay all jobs with matching GRes by specified amount
 *
 * the counter is reset in MNodeResetJobSlots
 *
 * @param DelayTime (I) [in seconds]
 * @param GRIndex (I) - generic resource index
 */

int MQueueApplyGResStartDelay(

  mulong  DelayTime,
  int     GRIndex)

  {
  long GRSMinTime;

  if ((DelayTime <= 0) || (GRIndex <= 0))
    {
    return(FAILURE);
    }

  GRSMinTime = (long)(MSched.Time + DelayTime);

  MGRes.GRes[GRIndex]->StartDelayDate = GRSMinTime;

  if ((MSched.GN != NULL) && (MSNLGetIndexCount(&MSched.GN->CRes.GenericRes,GRIndex) > 0))
    {
    MSNLSetCount(&MSched.GN->ARes.GenericRes,GRIndex,0);
    }

  return(SUCCESS);
  }  /* END MQueueApplyGResStartDelay() */

/**
 * Migrate job arrays
 *
 * @param J           (I) job
 * @param BestRemoteP (I) - best remote partition
 * @param Migrate
 */

int MQueueMigrateArrays(

  mjob_t  *J,
  mpar_t  *BestRemoteP,
  mbool_t  Migrate)

  {
  char JobName[MMAX_NAME];
  mjob_t *tmpJ; /* subjob in array */
  mjob_t *JA;   /* master of array */
  mpar_t *P;    /* partition to check available procs */
  mpar_t *BestConfiguredP = NULL;    /* partition to check configured procs */
  int jaindex;
  int pindex;
  int cindex;
  int count = 0;
  int rc = SUCCESS;
  int TC[MMAX_PAR] = {0};      /* configured tasks */
  enum MStatusCodeEnum tSC;
  char tmpLine[MMAX_LINE];
  int BestLimit = 0;
  int Limit = 0;

  /* check to see if anything further needs to be done */
  if ((J->Req[0]->PtIndex) != 0 && 
      (Migrate == FALSE) &&
      (bmisset(&BestRemoteP->RM->Flags,mrmfSlavePeer)))
    return(SUCCESS);

  /* check for limit on best partition */
  Limit = J->Credential.U->L.ActivePolicy.SLimit[mptMaxJob][BestRemoteP->Index];

  /* choose the best limit */
  if (Limit > 0)
    {
    BestLimit  = Limit;
    BestLimit -= BestRemoteP->DRes.Procs; /* subtract what is already used */
    }
  else 
    BestLimit = BestRemoteP->ARes.Procs;

  if ((J->JGroup != NULL) && 
      (MJobFind(J->JGroup->Name,&JA,mjsmExtended) != FAILURE))
    {
    /* don't migrate more than the slot limit */
    if ((JA->Array != NULL) && 
        (JA->Array->Limit != 0) && 
        ((JA->Array->Migrated >= JA->Array->Limit) ||
         (JA->Array->Active >= JA->Array->Limit) ||
         ((JA->Array->Migrated + JA->Array->Active) >= JA->Array->Limit)))
      {
      /* this job should not be migrated at this time */

      MDB(6,fSCHED) MLog("INFO:     cannot  migrate array job '%s' to peer '%s' at this time -- exceeds slot limit\n",
        J->Name,
        BestRemoteP->RM->Name);

      BestRemoteP = NULL;
      }
    else if ((JA->Array != NULL) &&
             (JA->Array->Limit != 0) && 
             (Migrate == TRUE))
      {
      for (jaindex = 0;jaindex < MMAX_JOBARRAYSIZE;jaindex++)
        {
        if (JA->Array->Members[jaindex] == -1)
          break;
  
        strcpy(JobName,JA->Array->JName[jaindex]);
  
        /* find the job */
        if (MJobFind(JobName,&tmpJ,mjsmExtended) == FAILURE)
           {
           continue;
           }

        /* check the job state */
        switch(tmpJ->State)
          {
          case mjsRunning:
          case mjsStarting:
           count++;
           break;
          case mjsIdle:
           /* set the partition access list for the job */
           bmclear(&tmpJ->PAL); /* clear current PAL */
           bmset(&tmpJ->PAL, BestRemoteP->Index);
            /* set the partition index */
           tmpJ->Req[0]->PtIndex = BestRemoteP->Index;
           /* migrate the job to the best partition */
           MJobMigrate(&tmpJ,BestRemoteP->RM,FALSE,NULL,tmpLine,&tSC);
           count++;
           break;
  
          default:
           break;
  
          }

        /* are we done yet */
        if (count == JA->Array->Limit)
          break;

        }
      }
    else if ((JA->Array != NULL) &&
             (JA->Array->Count > BestLimit)) /* not enough available procs in BestRemoteP */
      {
      P = NULL;

      /* try to find another partition where all jobs can run */
      for (pindex = 1;pindex < MMAX_PAR;pindex++)
        {
        P = &MPar[pindex];
        TC[pindex] = P->CRes.Procs;
 
        /* already checked bestremotep */
        if (P->Index == BestRemoteP->Index || P->Name[0] == '\1')
          continue;

        /* check partition limits */
        Limit = J->Credential.U->L.ActivePolicy.SLimit[mptMaxJob][P->Index];
        if (Limit > 0)
          {
          BestLimit = Limit;
          BestLimit -= P->DRes.Procs;  /* subtract what's being used */
          }
        else
          BestLimit = P->ARes.Procs;

        /* if we're at the end or have found available procs, break */
        if (P->Name[0] == '\0' || JA->Array->Count <= BestLimit)
           break;
        }
       TC[pindex] = 0;
       if (P->Name[0] != '\0')
        {
        BestRemoteP = P;
        }
       else
        {
        /* no longer the best partition */
        BestRemoteP = NULL;
        /* couldn't find available procs, look at configured procs now */
        Limit = J->Credential.U->L.ActivePolicy.SLimit[mptMaxJob][1];

        if (Limit > 0) /* if a limit has been set use it */
          {
          BestLimit = Limit;
          BestLimit -= MPar[1].DRes.Procs;
          }
        else
          {
          BestLimit = P->ARes.Procs;
          }
          BestConfiguredP = &MPar[1];

        /* choose the partition with the most procs */
        for (cindex=2; TC[cindex] != 0; cindex++)
          {
          Limit = J->Credential.U->L.ActivePolicy.SLimit[mptMaxJob][cindex];
          if (Limit > 0)
            {
            Limit -= MPar[cindex].DRes.Procs;
            }
          else
            {
            Limit = MPar[cindex].ARes.Procs;
            }
          if (BestLimit < Limit)
            {
            BestLimit = Limit;
            BestConfiguredP = &MPar[cindex];
            }
          }
        }
      }

    /* at this point, we should have a partition */
    if (((BestRemoteP !=  NULL) ||
        (BestConfiguredP != NULL)) &&
        (JA->Array != NULL) &&
        (JA->Array->Limit <= 0))
      {
      for (jaindex = 0;jaindex < MMAX_JOBARRAYSIZE;jaindex++)
        {
        if (JA->Array->Members[jaindex] == -1)
         break;

        if (JA->Array->JPtrs[jaindex] != NULL)
          {
          strcpy(JobName,JA->Array->JName[jaindex]);
 
          if (MJobFind(JobName,&tmpJ,mjsmExtended) == FAILURE)
            {
            continue;
            }

          if (BestRemoteP != NULL && BestRemoteP->Name != NULL)
            {
            /* set the partition access list for the job */
            bmclear(&tmpJ->PAL); /* clear current PAL */
            bmset(&tmpJ->PAL, BestRemoteP->Index);
            /* set the partition index */
            tmpJ->Req[0]->PtIndex = BestRemoteP->Index;
            if (Migrate == TRUE) 
              {
              /* migrate the job to the best partition */
              MJobMigrate(&tmpJ,BestRemoteP->RM,FALSE,NULL,tmpLine,&tSC);
              }
            }
          else
            {
            /* set the partition access list for the job */
            bmclear(&tmpJ->PAL); /* clear current PAL */
            bmset(&tmpJ->PAL, BestConfiguredP->Index);
            /* set the partition index */
            tmpJ->Req[0]->PtIndex = BestConfiguredP->Index;
            if (Migrate == TRUE) 
              {
              /* migrate the job to the best partition */
              MJobMigrate(&tmpJ,BestConfiguredP->RM,FALSE,NULL,tmpLine,&tSC);
              }
            }
          }
        }
      }
    }
  else
    {
    rc = FAILURE;
    }

  return(rc);
  }  /* END MQueueMigrateArrays() */

/* END MQueue.c */
