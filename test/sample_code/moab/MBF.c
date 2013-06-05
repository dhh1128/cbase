/* HEADER */

/**
 * @file MBF.c
 *
 * Moab Backfill
 */
 
/*                                           *
 * Contains:                                 *
 *                                           */
 
#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"
 

/* local prototypes */

int __MBFStoreClusterState(enum MNodeStateEnum *);
int __MBFRestoreClusterState(enum MNodeStateEnum *);
int __MBFReserveNodes(mnl_t *);
int __MBFReleaseNodes(enum MNodeStateEnum *,mnl_t *);



/**
 * Perform preemption-based backfill scheduling.
 *
 * @param JobList (I)
 * @param PolicyLevel (I)
 * @param NodeList (I)
 * @param MaxDuration (I)
 * @param NodesAvailable (I)
 * @param ProcsAvailable (I)
 * @param P (I)
 */

int MBFPreempt(

  mjob_t    **JobList,              /* I */
  enum MPolicyTypeEnum PolicyLevel, /* I */
  mnl_t      *NodeList,             /* I */
  mulong      MaxDuration,          /* I */
  int         NodesAvailable,       /* I */
  int         ProcsAvailable,       /* I */
  mpar_t     *P)                    /* I */

  {
  int           jindex;
  mnl_t        *MNodeList[MMAX_REQ_PER_JOB];

  double        BestBFPriority;
  int           BestJIndex;

  char         *NodeMap = NULL;
 
  int           CurrentProcCount;
  int           CurrentNodeCount;

  int           ReqPC;
  int           ReqNC;
 
  mjob_t       *J;

  double        JobBFPriority;

  mbitmap_t     BM;

  const char *FName = "MBFPreempt";
 
  MDB(2,fSCHED) MLog("%s(%s,%s,%ld,%d,%d,%s)\n",
    FName,
    (JobList != NULL) ? "JobList" : "NULL",
    (NodeList != NULL) ? "NodeList" : "NULL", 
    MaxDuration,
    NodesAvailable,
    ProcsAvailable,
    P->Name);
 
  CurrentNodeCount = NodesAvailable;
  CurrentProcCount = ProcsAvailable;

  MNLMultiInit(MNodeList);

  if (MNodeMapInit(&NodeMap) == FAILURE)
    {
    return(FAILURE);
    }
 
  MNLMultiInit(MNodeList);

  while (1)
    {
    BestJIndex     = -1;
    BestBFPriority = 0.0;

    /* find highest priority backfill job */
 
    for (jindex = 0;JobList[jindex] != NULL;jindex++)
      {
      J = JobList[jindex];

      if (J == (mjob_t *)1)
        continue;

      if (bmisset(&J->IFlags,mjifBillingReserveFailure))
        {
        /* Billing failed already failed this iteration, wait until next iteration */

        continue;
        }

      if (bmisset(&J->IFlags,mjifReqTimeLock))
        {
        MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (time locked)\n",
          J->Name,
          P->Name);

        continue;
        }
 
      ReqPC = J->TotalProcCount;
      ReqNC = J->Request.NC;

      if ((ReqPC > CurrentProcCount) ||
          (ReqNC > CurrentNodeCount))
        {
        /* inadequate resources remaining */

        JobList[jindex] = (mjob_t *)1;

        continue;
        }

      MJobGetBackfillPriority(
        J,
        MaxDuration,
        0,
        &JobBFPriority,
        NULL);

      if (JobBFPriority > BestBFPriority)
        {
        BestBFPriority = JobBFPriority;
        BestJIndex     = jindex;
        }
      }    /* END for (jindex) */

    if (BestJIndex == -1)
      {
      MNLMultiFree(MNodeList);

      /* no feasible job found */

      return(SUCCESS);
      }

    MStat.EvalJC++;

    J = JobList[BestJIndex];

    bmset(&BM,mlActive);

    if (MJobCheckLimits(
          J,
          PolicyLevel,
          P,
          &BM,
          NULL,
          NULL) == FAILURE)
      {
      MDB(4,fSCHED) MLog("INFO:     job '%s' rejected by active policy\n",
        J->Name);

      JobList[BestJIndex] = (mjob_t *)1;       
 
      continue;
      }    /* END if (MJobCheckLimits() == FAILURE) */
 
    if (MJobSelectMNL(
         J,
         P,
         NodeList,
         MNodeList,
         NodeMap,
         MBNOTSET,
         FALSE,
         NULL,
         NULL,
         NULL) == FAILURE)
      {
      MDB(4,fSCHED) MLog("INFO:     cannot select procs for job '%s'\n",
        J->Name);

      JobList[BestJIndex] = (mjob_t *)1;     
 
      continue;
      }    /* END if (MJobSelectMNL() == FAILURE) */

    if (MJobAllocMNL(
         J,
         MNodeList,
         NodeMap,
         0,
         P->NAllocPolicy,
         MSched.Time,
         NULL,
         NULL) == FAILURE)
      {
      MDB(4,fSCHED) MLog("INFO:     cannot allocate resources for job '%s'\n",
        J->Name);
 
      JobList[BestJIndex] = (mjob_t *)1;
 
      continue;
      }    /* END if (MJobAllocMNL() == FAILURE) */

    if (MJobStart(J,NULL,NULL,"job preempted") == FAILURE)
      {
      MDB(4,fSCHED) MLog("INFO:     cannot start job '%s'\n",
        J->Name);
 
      JobList[BestJIndex] = (mjob_t *)1;
 
      continue;
      }  /* END if (MJobStart(J,NULL,NULL) == FAILURE) */

    /* job successfully started */

    bmset(&J->SysFlags,mjfBackfill);

    /* if job is backfilled, mark job as preemptible */
 
    bmset(&J->SpecFlags,mjfPreemptee);

    MJobUpdateFlags(J);
 
    MStatUpdateBFUsage(J);

    ReqPC = J->TotalProcCount;
    ReqNC = J->Request.NC;

    CurrentNodeCount -= ReqNC;
    CurrentProcCount -= ReqPC;

    JobList[BestJIndex] = (mjob_t *)1;
    }  /* END while (1) */

  MNLMultiFree(MNodeList);
  MUFree(&NodeMap);

  return(SUCCESS);
  }  /* END MBFPreempt() */




/* move to heap to avoid X1E compiler issue */


/**
 * Perform 'Firstfit' backfill.
 *
 * @param BFQueue (I)
 * @param PolicyLevel (I)
 * @param BFNodeList (I)
 * @param Duration (I)
 * @param BFNodeCount (I) [number of eligible nodes in BFNodeList]
 * @param BFProcCount (I)
 * @param P (I)
 */

int MBFFirstFit(
 
  mjob_t               **BFQueue,      /* I */
  enum MPolicyTypeEnum   PolicyLevel,  /* I */
  mnl_t                 *BFNodeList,   /* I */
  mulong                 Duration,     /* I */
  int                    BFNodeCount,  /* I (number of eligible nodes in BFNodeList) */
  int                    BFProcCount,  /* I */
  mpar_t                *P)            /* I */
 
  {
  static mbool_t     Initialized = FALSE;
  static mnl_t  *FFMNodeList[MMAX_REQ_PER_JOB];
  static mnl_t  *FFBestMNodeList[MMAX_REQ_PER_JOB];
 
  int           jindex;
  int           sindex;

  mjob_t       *J;
  mreq_t       *RQ;
 
  static char  *tmpNodeMap = NULL;
  static char  *NodeMap = NULL;
  static char  *BestNodeMap = NULL;
 
  int           PC;
  mulong        SpecWCLimit;
 
  int           BestSIndex;
  int           BestSVal;
 
  int           DefaultJobTaskCount;
  int           DefaultReqTaskCount;
  mulong        DefaultSpecWCLimit;

  mbool_t       ChunkingEnabled = FALSE;  /* chunking is enabled in the moab.cfg file */
  mbool_t       ChunkingActive = FALSE;   /* chunking is active (have not reached BFCHUNKDURATION threshold) */

  mbool_t       PReq;

  mnl_t         tmpNL;

  int           rc;

  mbitmap_t     BM;
  mbitmap_t     OptimizedBF;
  
  const char *FName = "MBFFirstFit";

  MDB(2,fSCHED) MLog("%s(BFQueue,%d,BFNodeList,%ld,%d,%d,%s)\n",
    FName,
    PolicyLevel,
    Duration, 
    BFNodeCount,
    BFProcCount,
    P->Name);

  J = NULL;
  PC = 0;
  
  if (bmisset(&MSched.Flags,mschedfOptimizedBackfill))
    {
    bmset(&OptimizedBF,1);
    }

  if (Initialized == FALSE)
    {
    MNLMultiInit(FFMNodeList);
    MNLMultiInit(FFBestMNodeList);

    if ((MNodeMapInit(&tmpNodeMap) == FAILURE) ||
        (MNodeMapInit(&NodeMap) == FAILURE) ||
        (MNodeMapInit(&BestNodeMap) == FAILURE))
      {
      return(FAILURE);
      }

    Initialized = TRUE;
    }

  if ((MPar[0].BFChunkSize > 0) && 
      (MPar[0].BFChunkDuration > 0))
    {
    ChunkingEnabled = TRUE;
    }
  else
    {
    ChunkingEnabled = FALSE;
    }

  if ((ChunkingEnabled == TRUE) &&
      (MPar[0].BFChunkBlockTime > 0) &&
      ((mulong)MPar[0].BFChunkBlockTime < MSched.Time))
    {
    /* if chunking is enabled but the block time has passed then disabled chunking */

    ChunkingEnabled = FALSE;
    }

  ChunkingActive = FALSE;

  MNLInit(&tmpNL);

  for (jindex = 0;BFQueue[jindex] != NULL;jindex++)
    {
    if ((P->RM != NULL) &&
        (P->RM->FailIteration == MSched.Iteration) &&
        (P->RM->FailCount >= P->RM->MaxFailCount))
      {
      /* this check also happens in MQueueBackFill, __MSchedScheduleJobs */

      /* have to check with every job or we may end up swamping a failed RM */

      continue;
      }

    if ((ChunkingEnabled == TRUE) && (ChunkingActive == FALSE))
      {
      /* evaluate previous job */

      /* if chunking is enabled and job is chunksize or larger, */
      /* and job is blocked, activate chunking */

      if ((J != NULL) && 
          (MJOBISACTIVE(J) == FALSE) &&
          (PC >= MPar[0].BFChunkSize))
        {
        if (MPar[0].BFChunkBlockTime == 0)
          {
          /* if the block time has not been set then set it to now */

          MPar[0].BFChunkBlockTime = MSched.Time + MPar[0].BFChunkDuration;
          }

        ChunkingActive = TRUE;
        }
      }

    J = BFQueue[jindex];

    if ((J->Depend != NULL) &&
        (J->Depend->Type == mdJobSyncMaster))
      {
      if (MJobCombine(J,&J) == FAILURE)
        {
        MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (dependency)\n",
          J->Name,
          P->Name);

        continue;
        }
      }

    if (bmisset(&J->IFlags,mjifBillingReserveFailure))
      {
      /* Billing failed already failed this iteration, wait until next iteration */

      continue;
      }

    if (bmisset(&J->IFlags,mjifReqTimeLock))
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (time locked)\n",
        J->Name,
        P->Name);

      continue;
      }
 
    RQ = J->Req[0];
 
    if ((J->State != mjsIdle) || (J->EState != mjsIdle))
      {
      continue;
      }
 
    MStat.EvalJC++;
 
    /* try all proc count/wclimit combinations */
 
    BestSIndex = -1;
    BestSVal   = 0;
 
    /* FIXME:  handles only one req */
 
    DefaultSpecWCLimit  = J->SpecWCLimit[0];
    DefaultReqTaskCount = RQ->TaskRequestList[0];
    DefaultJobTaskCount = J->Request.TC;
 
    for (sindex = 0;RQ->TaskRequestList[sindex] > 0;sindex++)
      {
      if ((sindex != 0) &&
          (RQ->TaskRequestList[sindex] == RQ->TaskRequestList[0]))
        {
        /* ignore duplicate shapes */
 
        continue;
        } 
 
      SpecWCLimit = (J->SpecWCLimit[sindex] > 0) ?
        J->SpecWCLimit[sindex] : J->SpecWCLimit[0];
 
      if (sindex != 0)
        MJobUpdateResourceCache(J,NULL,sindex,NULL);
 
      PC = J->TotalProcCount;

      if ((ChunkingActive == TRUE) && (PC < MPar[0].BFChunkSize))
        {
        /* job blocked by chunking */

        MDB(5,fSCHED) MLog("INFO:     %s:  job %s.%d blocked by chunking (P: %d T: %ld)\n",
          FName,
          J->Name,
          sindex,
          PC,
          SpecWCLimit);

        continue;
        }

      if (PC > BFProcCount)
        {
        /* NOTE:  re-evaluate/BF may have adjusted initial availability */

        continue;
        }
 
      MDB(5,fSCHED) MLog("INFO:     %s:  evaluating job[%d] %s.%d (P: %3d T: %ld)\n",
        FName,
        jindex,
        J->Name,
        sindex,
        PC,
        SpecWCLimit);

      /* NOTE:  try all shapes, select shape with best job turnaround time */

      J->Request.TC         += 
        RQ->TaskRequestList[sindex] - RQ->TaskCount;

      J->SpecWCLimit[0]      = SpecWCLimit;
      RQ->TaskRequestList[0] = RQ->TaskRequestList[sindex];
      RQ->TaskCount          = RQ->TaskRequestList[sindex];

      bmset(&BM,mlActive);

      if (MJobCheckLimits(
            J,
            PolicyLevel,
            P,
            &BM,
            NULL,
            NULL) == FAILURE)
        { 
        MDB(8,fSCHED) MLog("INFO:     job %s.%d does not meet active fairness policies\n",
          J->Name,
          sindex);
 
        continue;
        }

      /* NOTE:  tmpNL must include available global resources */

      rc = MJobGetFNL(
            J,
            P,
            BFNodeList,  /* I */
            &tmpNL,      /* O */
            NULL,
            NULL,
            MMAX_TIME,
            &OptimizedBF,
            NULL);

      if (rc == FAILURE)
        {
        /* insufficient feasible nodes found */

        MDB(4,fSCHED) MLog("INFO:     cannot locate adequate feasible tasks for job %s:%d in partition %s\n",
          J->Name,
          0,
          P->Name);

        continue;
        }

      if (MJobSelectMNL(
            J,
            P,
            &tmpNL,       /* I */
            FFMNodeList,  /* O */
            NodeMap,
            (J->OrigWCLimit <= 0) ? MBNOTSET : FALSE,
            FALSE,
            &PReq,
            NULL,
            NULL) == FAILURE)
        {
        MDB(8,fSCHED) MLog("INFO:     cannot locate available resources for job %s.%d\n",
          J->Name,
          sindex);
 
        continue;
        }
 
      MNodeMapCopy(tmpNodeMap,NodeMap);
 
      if (MJobAllocMNL(
            J,
            FFMNodeList,
            NodeMap,
            0,
            P->NAllocPolicy,
            MSched.Time,
            NULL,
            NULL) == FAILURE)
        {
        mnl_t *MNodeList[MMAX_REQ_PER_JOB];

        long        StartTime = (long)MSched.Time;

        MNLMultiInit(MNodeList);

        MSched.BFNSDelayJobs = TRUE;

        /* cannot enforce distribution policy */

        if ((MSched.NodeSetPlus != mnspSpanEvenly) || 
            (MSched.BFNSDelayJobs != TRUE))
          {
          MNLMultiFree(MNodeList);

          continue;
          }

        /* determine start time */

        if ((MJobGetEStartTime(J,&P,NULL,NULL,MNodeList,&StartTime,NULL,FALSE,FALSE,NULL) == FAILURE) ||
            (StartTime > (long)MSched.Time))
          {
          MNLMultiFree(MNodeList);

          continue;
          }

        MNLCopy(&J->Req[0]->NodeList,MNodeList[0]);

        MNLMultiFree(MNodeList);
        }    /* END if (MJobAllocMNL() == FAILURE) */
 
      /* job is feasible */
 
      if (PC > BestSVal)
        {
        /* select largest mshape */

        BestSVal   = PC; 
 
        BestSIndex = sindex;
 
        MNodeMapCopy(BestNodeMap,tmpNodeMap);

        MNLMultiCopy(FFBestMNodeList,FFMNodeList);
        }
      }  /* END for (sindex) */
 
    /* FIXME:  only handles one req per job */
 
    MJobUpdateResourceCache(J,NULL,0,NULL);
 
    J->SpecWCLimit[0]      = DefaultSpecWCLimit;
    RQ->TaskRequestList[0] = DefaultReqTaskCount;
    J->Request.TC          = DefaultJobTaskCount;
    RQ->TaskCount          = DefaultReqTaskCount;
 
    if (BestSIndex >= 0)
      {
      /* valid shape found */

      J->SpecWCLimit[0]      = J->SpecWCLimit[BestSIndex];
      RQ->TaskRequestList[0] = RQ->TaskRequestList[BestSIndex];
      RQ->TaskCount          = RQ->TaskRequestList[BestSIndex];
      J->Request.TC         += 
        RQ->TaskRequestList[BestSIndex] - DefaultReqTaskCount;

      if (J->OrigWCLimit > 0)
        {
        /* already evaluated job at scaled walltime */
        /* restore orig walltime and verify no collisions with non-job reservations occur */

        /* restore orig walltime */

        J->SpecWCLimit[0] = J->OrigWCLimit;

        bmset(&J->Flags,mjfIgnIdleJobRsv);

        MNLCopy(FFMNodeList[0],FFBestMNodeList[0]);

        if (MJobSelectMNL(
            J,
            P,
            FFMNodeList[0],     /* I */
            FFBestMNodeList,    /* O */
            BestNodeMap,
            FALSE,
            FALSE,
            &PReq,
            NULL,
            NULL) == FAILURE)
          {
          MDB(8,fSCHED) MLog("INFO:     cannot locate available resources for job %s.%d\n",
            J->Name,
            sindex);

          bmunset(&J->Flags,mjfIgnIdleJobRsv);

          continue;
          }
        }    /* END if (J->OrigWCLimit > 0) */

      bmunset(&J->Flags,mjfIgnIdleJobRsv);

      if (BestSIndex > 1)
        {
        if (MJobAllocMNL(
              J,
              FFBestMNodeList,
              BestNodeMap,
              0,
              P->NAllocPolicy,
              MSched.Time,
              NULL,
              NULL) == FAILURE)
          {
          MDB(0,fSCHED) MLog("ERROR:    %s:  cannot allocate nodes for job %s.%d\n",
            FName,
            J->Name,
            BestSIndex);
 
          J->SpecWCLimit[0]      = DefaultSpecWCLimit;
          RQ->TaskRequestList[0] = DefaultReqTaskCount;
          J->Request.TC          = DefaultJobTaskCount; 
 
          continue;
          }
        }    /* END if (sindex > 1) */

      if (J->VWCLimit > 0)
        {
        /* job passes all checks, launch using requested virtual time */

        /* NOTE:  does not support malleable jobs or node-speed scaling */

        J->WCLimit        = J->VWCLimit;
        J->SpecWCLimit[0] = J->VWCLimit;
        J->SpecWCLimit[1] = J->VWCLimit;
        }
 
      if (MJobStart(J,NULL,NULL,"job backfilled") == FAILURE)
        {
        if (PReq == TRUE)
          {
          /* create rsv for preempted resources */

          if ((MJobReserve(&J,NULL,0,mjrQOSReserved,FALSE) == SUCCESS) && (J->Rsv != NULL))
            J->Rsv->ExpireTime = MSched.Time + MDEF_PREEMPTRSVTIME;

          continue;
          }

        if (MSched.Mode == msmNormal)
          {
          MDB(4,fSCHED) MLog("ERROR:    %s:  cannot start job %s.%d\n",
            FName,
            J->Name,
            BestSIndex);
          }

        J->SpecWCLimit[0]      = DefaultSpecWCLimit;
        RQ->TaskRequestList[0] = DefaultReqTaskCount;
        J->Request.TC          = DefaultJobTaskCount;
        J->Alloc.TC            = DefaultJobTaskCount;
        }
      else if (J->Array == NULL)
        {
        MDB(4,fSCHED) MLog("INFO:     %s:  job %s.%d started\n",
          FName,
          J->Name,
          BestSIndex);
 
        bmset(&J->SysFlags,mjfBackfill);
 
        MJobUpdateFlags(J);
 
        MStatUpdateBFUsage(J);
        }
      }   /* END if (BestSIndex >= 0) */
    }     /* END for (jindex)         */
 
  MNLFree(&tmpNL);

  MDB(2,fSCHED) MLog("INFO:     partition %s nodes/procs available after %s: %d/%d (%d jobs examined)\n",
    P->Name,
    FName,
    P->IdleNodes,
    P->ARes.Procs,
    jindex); 
 
  return(SUCCESS);
  }  /* END MBFFirstFit() */




/**
 *
 *
 * @param NSList
 */

int __MBFStoreClusterState(
 
  enum MNodeStateEnum *NSList)
 
  {
  int nindex;

  mnode_t *N;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if (N != NULL)    
      NSList[nindex] = N->State;
    else
      NSList[nindex] = mnsNONE;
    }  /* END for (nindex) */
 
  return(SUCCESS);
  }  /* END __MBFStoreClusterState() */
 
 
 
 
/**
 *
 *
 * @param NSList
 */

int __MBFRestoreClusterState(
 
  enum MNodeStateEnum *NSList)
 
  {
  int nindex;

  mnode_t *N;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if (N != NULL)
      N->State = NSList[nindex];
    }  /* END for (nindex) */
 
  return(SUCCESS);
  }  /* END __MBFRestoreClusterState() */ 



 
/**
 *
 *
 * @param NL
 */

int __MBFReserveNodes(
 
  mnl_t *NL)
 
  {
  int nindex;

  mnode_t *N;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    N->State = mnsReserved;
    }  /* END for (nindex) */
 
  return(SUCCESS);
  }  /* END __MBFReserveNodes() */
 
 
 
 
 
/**
 *
 *
 * @param NSList
 * @param NL
 */

int __MBFReleaseNodes(
 
  enum MNodeStateEnum   *NSList,
  mnl_t *NL)
 
  {
  int nindex = 0;

  mnode_t *N;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    N->State = NSList[N->Index];
    }  /* END for (nindex) */
 
  return(SUCCESS);
  }  /* END __MBFReleaseNodes() */





/**
 * Identify number of nodes/procs, specific nodes, and next duration available 
 * for BF window.
 *
 * @see MQueueBackFill() - parent
 *
 * NOTE:  allow all resources in shared partition into backfill window 
 *
 * @param TJ (I) template job
 * @param SMinTime (I) [may be max or min depending on __MLONGESTBFWINDOWFIRST]
 * @param ANC (O) [optional]
 * @param ATC (O) [optional]
 * @param ANL (O) nodelist available [optional,minsize=MMAX_NODE + 1]
 * @param ADuration (O) duration available
 * @param SeekLong (I)
 * @param Msg (O) descriptive message [optional,minsize=MMAX_BUFFER]
 */

int MBFGetWindow(

  mjob_t    *TJ,          /* I template job         */
  long       SMinTime,    /* I duration limit (may be max or min depending on __MLONGESTBFWINDOWFIRST) */
  int       *ANC,         /* O nodes available (optional) */
  int       *ATC,         /* O tasks available (optional) */
  mnl_t     *ANL,         /* O (optional,minsize=MMAX_NODE + 1) nodelist available */
  long      *ADuration,   /* O duration available   */
  mbool_t    SeekLong,    /* I                      */
  char      *Msg)         /* O (optional,minsize=MMAX_BUFFER) descriptive message */

  {
  int  nindex;

  long ETime;

  int  NC;
  int  TTC;

  enum MAllocRejEnum RIndex;

  long AvailableTime;

  char Affinity;

  char ReqRID[MMAX_NAME];
  char TString[MMAX_LINE];

  int  TC;

  char *BPtr;
  int   BSpace;

  /* NOTE: should switch to use TJ rather than making tmpJ (NYI) */

  mnode_t *N;
  mjob_t  *J;
  mreq_t  *RQ;

  mpar_t  *P;

  char     BRsvID[MMAX_NAME];

  const char *FName = "MBFGetWindow";

  MDB(3,fSCHED) MLog("%s(%s,%ld,ANC,ATC,ANL,ADuration,%s,%s)\n",
    FName,
    (TJ != NULL) ? TJ->Name : "NULL",
    SMinTime,
    MBool[SeekLong],
    (Msg == NULL) ? "NULL" : "Msg");

#ifndef __MOPT
  if (TJ == NULL)
    {
    return(FAILURE);
    }
#endif /* __MOPT */

#ifdef __MLONGESTBFWINDOWFIRST
  if (SMinTime <= 0)
#else /* __MLONGESTBFWINDOWFIRST */
  if (SMinTime >= MMAX_TIME)
#endif /* __MLONGESTBFWINDOWFIRST */
    {
    return(FAILURE);
    }

  if (Msg != NULL)
    MUSNInit(&BPtr,&BSpace,Msg,MMAX_BUFFER);

  /* build single req pseudo job */

  MJobMakeTemp(&J);

  RQ = J->Req[0];

  strcpy(J->Name,MCONST_BFJOBNAME);  /* NOTE:  must set to enable correct window traversal */

  if ((MSched.LocalQueueIsActive == TRUE) &&
      (MRMFind("internal",&J->SubmitRM) == SUCCESS))
    {
    /* associate BF job with local queue */

    J->DestinationRM = NULL;  /* no destination has been selected */

    RQ->RMIndex = J->SubmitRM->Index;  /* only single req in temp job */
    }
  else if (MRM[1].Type != mrmtNONE)
    {
    /* multiple rm's defined - allow resources from any RM to be located */

    J->SubmitRM = NULL;
    }
  else
    {
    /* only one rm exists */

    J->SubmitRM = &MRM[0];
    }

  strcpy(BRsvID,"UNKNOWN");

  if (TJ->ReqRID != NULL)
    {
    MUStrCpy(ReqRID,TJ->ReqRID,sizeof(ReqRID));

    J->ReqRID = ReqRID;

    bmcopy(&J->SpecFlags,&TJ->SpecFlags);
    bmcopy(&J->Flags,&J->SpecFlags);
    }

  bmcopy(&J->IFlags,&TJ->IFlags);

  if (TJ->Req[0]->Opsys != 0)
    RQ->Opsys = TJ->Req[0]->Opsys;

  if (MJobSetCreds(
        J,
        (J->Credential.U == NULL) ? (char *)ALL : J->Credential.U->Name,
        (J->Credential.G == NULL) ? (char *)ALL : J->Credential.G->Name,
        (J->Credential.A == NULL) ? (char *)ALL : J->Credential.A->Name,
        0,
        NULL) == FAILURE)
    {
    MDB(1,fCORE) MLog("ALERT:    cannot setup credentials for pseudo-job (U: %s  G: %s  A: %s)\n",
      (J->Credential.U == NULL) ? ALL : J->Credential.U->Name,
      (J->Credential.G == NULL) ? ALL : J->Credential.G->Name,
      (J->Credential.A == NULL) ? ALL : J->Credential.A->Name);

    MJobFreeTemp(&J);

    return(FAILURE);
    }

  MCResCopy(&RQ->DRes,&TJ->Req[0]->DRes);

  MCredCopy(&J->Credential,&TJ->Credential);

  J->WCLimit        = TJ->WCLimit;
  J->SpecWCLimit[0] = TJ->WCLimit;

#ifndef __MLONGESTBFWINDOWFIRST
  J->WCLimit        = MAX((mulong)SMinTime,TJ->WCLimit);
  J->SpecWCLimit[0] = MAX((mulong)SMinTime,TJ->WCLimit);
#endif /* !__MLONGESTBFWINDOWFIRST */

  J->Request.TC = TJ->Request.TC;

  if (TJ->Credential.Q == NULL)
    {
    MQOSFind(ALL,&J->QOSRequested);
    }
  else
    {
    J->QOSRequested = TJ->Credential.Q;
    }

  MJobSetQOS(J,J->QOSRequested,0);

  if (TJ->Credential.C != NULL)
    {
    J->Credential.C = TJ->Credential.C;
    }

  bmcopy(&J->AttrBM,&TJ->AttrBM);

  MJobBuildCL(J);

  bmcopy(&RQ->ReqFBM,&TJ->Req[0]->ReqFBM);

  memcpy(RQ->ReqNR,TJ->Req[0]->ReqNR,sizeof(TJ->Req[0]->ReqNR));
  memcpy(RQ->ReqNRC,TJ->Req[0]->ReqNRC,sizeof(TJ->Req[0]->ReqNRC));

  RQ->Index = 0;

  if (TJ->Req[0]->DRes.Procs == 0)
    {
    /* non-proc resources requested */

    RQ->TaskCount = MAX(1,TJ->Req[0]->TaskCount);
    }
  else if (TJ->Req[0]->DRes.Procs == -1)
    {
    /* dedicated node requested */

    RQ->TaskCount = MAX(1,TJ->Req[0]->TaskCount);
    }
  else
    { 
    /* map tasks to processors */

    RQ->TaskCount = TJ->Req[0]->DRes.Procs;
    }

  RQ->TasksPerNode = 0;

  P                = &MPar[TJ->Req[0]->PtIndex];

  ETime            = MMAX_TIME;

  NC               = 0;
  TTC              = 0;

  J->VMUsagePolicy = TJ->VMUsagePolicy;

  /* locate all idle nodes with AvailableTime >/< SMinTime */

  MDB(5,fSCHED) MLog("INFO:     searching for backfill nodes available for at least %ld seconds\n",
    J->WCLimit);

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    int MaxRsv;

    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    /* determine node availability */
  
    if (N->PtIndex == MSched.SharedPtIndex)
      MaxRsv = MSched.MaxRsvPerGNode;
    else
      MaxRsv = MSched.MaxRsvPerNode;

    /* if node has max rsv already, it isn't available */
    if (MNodeGetRsvCount(N,FALSE,FALSE) >= MaxRsv)
      {
      MDB(6,fSCHED) MLog("INFO:     node %s not considered - exceeds max number of reservations per node.\n",
        N->Name);
      
      continue;
      }

    if (((N->State != mnsIdle) && (N->State != mnsActive)) ||
        ((N->EState != mnsIdle) && (N->EState != mnsActive)))
      {
      MDB(6,fSCHED) MLog("INFO:     node %s not considered (state: %s/estate: %s)\n",
        N->Name,
        MNodeState[N->State],
        MNodeState[N->EState]);
      
      if (Msg != NULL)
        {
        if ((N->State != mnsIdle) && (N->State != mnsActive))
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is unavailable (state '%s')\n",
            N->Name,
            MNodeState[N->State]);
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is unavailable (expected state '%s')\n",
            N->Name,
            MNodeState[N->EState]);
          }
        }

      continue;
      }  /* END if (N->State ...) */

    /* allow shared partition nodes into list */

    if ((N->PtIndex != P->Index) && 
        (P->Index != 0) && 
        (N->PtIndex != MSched.SharedPtIndex) && 
        (N->PtIndex != 0))
      {
      MDB(6,fSCHED) MLog("INFO:     node %s not considered (invalid partition)\n",
        N->Name);

      if (Msg != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"node %s is unavailable (partition '%s')\n",
          N->Name,
          MPar[N->PtIndex].Name);
        }

      continue;
      }

    if ((N->PtIndex == MSched.SharedPtIndex) &&
        (N->PtIndex != P->Index))
      {
      /* all shared partition nodes are available to every job in every partition */
      /* this allows compute jobs with license requirements to be backfilled */

      MDB(5,fSCHED) MLog("INFO:     shared node %s found\n",
        N->Name);

      TC = MSNLGetIndexCount(&N->ARes.GenericRes,0);

      if (ANL != NULL)
        {
        MNLSetNodeAtIndex(ANL,NC,N);
        MNLSetTCAtIndex(ANL,NC,TC);

        NC++;
        TTC += TC;
        MNLTerminateAtIndex(ANL,NC);
        }

      continue;
      }

    /* allow all non-compute virtual nodes to be considered in backfill */

    if (bmisset(&J->IFlags,mjifIsTemplate) || (N->CRes.Procs != 0))
      {
      if (MReqCheckResourceMatch(J,RQ,N,&RIndex,MSched.Time,FALSE,NULL,NULL,NULL) == FAILURE)
        {
        MDB(6,fSCHED) MLog("INFO:     node '%s' rejected.  (%s)\n",
          N->Name,
          MAllocRejType[RIndex]);

        if (Msg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is unavailable (%s)\n",
            N->Name,
            MAllocRejType[RIndex]);
          }

        continue;
        }    /* END if (MReqCheckResourceMatch(J,RQ,N,&RIndex,MSched.Time,NULL,NULL) == FAILURE) */

      /* check reservations */

      if (MReqCheckNRes(
            J,
            N,
            RQ,
            MSched.Time,
            &TC,       /* O */
            1.0,
            0,
            &RIndex,   /* O */
            &Affinity, /* O */
            NULL,
            TRUE,
            FALSE,
            NULL) == FAILURE)
        {
        /* node does not meet time/resource requirements */

        if (Msg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s does not meet requirements (%s)\n",
            N->Name,
            MAllocRejType[RIndex]);
          }

        continue;
        }

      if (MJobGetNRange(
            J,
            RQ,
            N,
            MSched.Time,
            &TC,  /* O */
            &AvailableTime,
            &Affinity,
            NULL,
            SeekLong,
            BRsvID) == FAILURE)
        {
        /* node does not have resources immediately available */

        if (Msg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is blocked immediately\n",
            N->Name);
          }

        continue;
        }

#ifdef __MLONGESTBFWINDOWFIRST
      if (AvailableTime >= SMinTime)
#else /* __MLONGESTBFWINDOWFIRST */
      if (AvailableTime <= SMinTime)
#endif /* __MLONGESTBFWINDOWFIRST */
        {
        if (Msg != NULL)
          {
          if (AvailableTime < (MMAX_TIME >> 1))
            {
            MULToTString(AvailableTime,TString);

            MUSNPrintF(&BPtr,&BSpace,"node %s is blocked by reservation %s in %s\n",
              N->Name,
              BRsvID,
              TString);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"node %sx%d is available with no timelimit\n",
              N->Name,
              TC);
            }
          }

        continue;
        }
      else
        {
        if (Msg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %sx%d is available with no timelimit\n",
            N->Name,
            TC);
          }
        }

#ifdef __MLONGESTBFWINDOWFIRST
      if (AvailableTime > ETime)
#else /* __MLONGESTBFWINDOWFIRST */
      if (AvailableTime < ETime)
#endif /* __MLONGESTBFWINDOWFIRST */
        {
        MDB(5,fSCHED) MLog("INFO:     node %s found with best time %ld\n",
          N->Name,
          AvailableTime);

        ETime = AvailableTime;

        if ((mulong)ETime == (MMAX_TIME - MSched.Time))
          {
          /* End time is INFINITY, set AvailableTime to INFINITY
              so that infinite walltime jobs can be backfilled
              when the top priority jobs can't run (but there is
              still space) */

          ETime = MMAX_TIME;
          }
        }
      else
        {
        MDB(5,fSCHED) MLog("INFO:     node %s found with available time %ld\n",
          N->Name,
          AvailableTime);
        }
      }    /* END if ((N->CRes.Procs != 0) || ...) */
    else
      {
      /* node has no procs - node not considered */

      continue;
      }

    if (ANL != NULL)
      {
      MNLSetNodeAtIndex(ANL,NC,N);
      MNLSetTCAtIndex(ANL,NC,TC);

      NC++;
      TTC += TC;
      MNLTerminateAtIndex(ANL,NC);
      }
    }       /* END for (nindex) */

  /* locate ETime bounded by reservation max time */

  {
  macl_t *tACL;

  rsv_iter RTI;

  mrsv_t *R;

  long    tmpL;

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (R->Type == mrtJob)
      continue;

    if (MACLGet(R->ACL,maDuration,&tACL) == FAILURE)
      continue;

    tmpL = tACL->LVal;

#ifdef __MLONGESTBFWINDOWFIRST
    {
    if (tmpL >= SMinTime)
      continue;

    if (tmpL > ETime)
      {
      MDB(5,fSCHED) MLog("INFO:     reservation %s found with best time %ld\n",
        R->Name,
        tmpL);

      ETime = tmpL;
      }
    }  /* END BLOCK */
#else /* __MLONGESTBFWINDOWFIRST */
    {
    if (tmpL <= SMinTime)
      continue;

    if (tmpL < ETime)
      {
      MDB(5,fSCHED) MLog("INFO:     reservation %s found with best time %ld\n",
        R->Name,
        tmpL);

      ETime = tmpL;
      }
    }  /* END BLOCK */
#endif /* __MLONGESTBFWINDOWFIRST */
    }  /* END while (MRsvTableIterate()) */
  }    /* END BLOCK */

  if (ADuration != NULL)
    *ADuration = ETime;

  if (ANC != NULL)
    *ANC = NC;

  if (ATC != NULL)
    *ATC = TTC;

  MULToTString(ETime,TString);

  MDB(3,fSCHED) MLog("INFO:     resource window:  time: %s  nodes: %3d  tasks: %3d  mintime: %5ld (idle nodes: %d)\n",
    TString,
    NC,
    TTC,
    SMinTime,
    MPar[0].IdleNodes);

  MJobFreeTemp(&J);

  if ((NC == 0) || (ETime == 0))
    {
    /* no BF window located */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MBFGetWindow() */





/**
 * Backfill jobs in bestfit order where fit is determined by procs/duration.
 *
 * NOTE:  does not support malleable jobs.
 *
 * @param BFQueue (I)
 * @param PolicyLevel (I)
 * @param BFNodeList
 * @param Duration (I)
 * @param BFNodeCount (I)
 * @param BFProcCount (I)
 * @param P (I)
 */

int MBFBestFit(

  mjob_t     **BFQueue,      /* I */
  enum MPolicyTypeEnum PolicyLevel,  /* I */
  mnl_t       *BFNodeList,   /* I */
  mulong       Duration,     /* I */
  int          BFNodeCount,  /* I */
  int          BFProcCount,  /* I */
  mpar_t      *P)            /* I */

  {
  int          jindex;
  mnl_t   *MNodeList[MMAX_REQ_PER_JOB];

  int          JECount;
  int          SmallJobPC;

  mulong       BFValue;
  mulong       tmpBFValue;

  mjob_t      *BFJob;
  int          BFIndex;

  char        *NodeMap = NULL;

  int          APC;

  int          PC;

  enum MBFMetricEnum BFMetric;

  mnl_t   tmpNL;

  static mjob_t      **LBFQueue = NULL;

  mjob_t      *J;

  mpar_t      *GP = &MPar[0];

  mbool_t      PReq;

  mbitmap_t    BM;

  const char *FName = "MBFBestFit";

  MDB(2,fSCHED) MLog("%s(BFQueue,%s,BFNodeList,%ld,%d,%d,%s)\n",
    FName,
    MPolicyMode[PolicyLevel],
    Duration,
    BFNodeCount,
    BFProcCount,
    P->Name);

  if ((LBFQueue == NULL) &&
      (MJobListAlloc(&LBFQueue) == FAILURE))
    {
    return(FAILURE);
    }

  memcpy(LBFQueue,BFQueue,sizeof(mjob_t *) * MSched.M[mxoJob]);

  APC = BFProcCount;
 
  BFMetric = (enum MBFMetricEnum)(P->BFMetric != mbfmNONE) ? P->BFMetric : GP->BFMetric;

  JECount = 0;

  MNLMultiInit(MNodeList);
  MNLInit(&tmpNL);

  if (MNodeMapInit(&NodeMap) == FAILURE)
    {
    return(SUCCESS);
    }

  while (1)
    {
    BFJob   = NULL;
    BFValue = 0;
    BFIndex = 0;

    SmallJobPC = MSched.M[mxoNode];

    for (jindex = 0;LBFQueue[jindex] != NULL;jindex++)
      {
      if (LBFQueue[jindex] == (mjob_t *)1)
        {
        /* job is not eligible */

        continue;
        }

      J = LBFQueue[jindex];

      if ((J->Depend != NULL) &&
          (J->Depend->Type == mdJobSyncMaster))
        {
        if (MJobCombine(J,&J) == FAILURE)
          {
          MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (dependency)\n",
            J->Name,
            P->Name);

          continue;
          }
        }

      if (bmisset(&J->IFlags,mjifBillingReserveFailure))
        {
        /* Billing failed already failed this iteration, wait until next iteration */

        continue;
        }

      if (bmisset(&J->IFlags,mjifReqTimeLock))
        {
        MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (time locked)\n",
          J->Name,
          P->Name);

        continue;
        }

      PC = J->TotalProcCount;

      if ((J->State != mjsIdle) || (J->EState != mjsIdle))
        {
        LBFQueue[jindex] = (mjob_t *)1;

        continue;
        }

      if (PC > APC)
        {
        /* job is too big to fit in window - ignore */

        LBFQueue[jindex] = (mjob_t *)1;

        continue;
        }

      /* determine job utility metric */

      switch (BFMetric)
        {
        case mbfmPS:

          tmpBFValue = PC * J->WCLimit;

          break;

        case mbfmWalltime:

          tmpBFValue = J->WCLimit;

          break;

        case mbfmProcs:
        default:
 
          tmpBFValue = PC;
 
          break;
        }  /* END switch (BFMetric) */

      if (tmpBFValue <= BFValue)
        {
        /* better job already located */

        MDB(5,fSCHED) MLog("INFO:     ignoring BF job '%s' (p: %d t: %ld), %s is better\n",
          J->Name,
          PC,
          J->WCLimit,
          (BFJob != NULL) ? BFJob->Name : "NULL");

        continue;
        }

      MDB(3,fSCHED) MLog("INFO:     attempting BF backfill with job '%s' (p: %d t: %ld)\n",
        J->Name,
        PC,
        J->WCLimit);

      JECount++;

      MStat.EvalJC++;

      SmallJobPC = MIN(SmallJobPC,PC);

      bmset(&BM,mlActive);

      if (MJobCheckLimits(
            J,
            PolicyLevel,
            P,
            &BM,
            NULL,
            NULL) == FAILURE)
        {
        MDB(8,fSCHED) MLog("INFO:     job %s does not meet active fairness policies\n",
          J->Name);

        LBFQueue[jindex] = (mjob_t *)1;    
 
        continue;
        }  /* END if (MJobCheckLimits() == FAILURE) */

      if (MReqGetFNL(
            J,
            J->Req[0],  /* FIXME:  support multi-req */
            P,
            BFNodeList,
            &tmpNL,      /* O */
            NULL,
            NULL,
            MMAX_TIME,
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

      if (MJobSelectMNL(
           J,
           P,
           &tmpNL,      /* I */
           MNodeList,  /* O */
           NodeMap,    /* O */
           MBNOTSET,
           FALSE,
           NULL,
           NULL,
           NULL) != SUCCESS)
        {
        LBFQueue[jindex] = (mjob_t *)1;    

        MDB(4,fSCHED) MLog("INFO:     cannot select tasks for job '%s'\n",
          J->Name);

        continue;
        }    /* END if (MJobSelectMNL() != SUCCESS) */

      /* located potential job */

      BFValue = tmpBFValue;
      BFJob   = LBFQueue[jindex];
      BFIndex = jindex;

      MDB(2,fSCHED) MLog("INFO:     located bestfit job '%s' in %s (size: %d  duration: %ld)\n",
        J->Name,
        FName,
        PC,
        J->WCLimit);
      }  /* END for (jindex) */

    if (BFJob == NULL)
      {
      /* no eligible jobs found */
 
      MDB(5,fSCHED) MLog("INFO:     no jobs found to backfill\n");
 
      break;
      }

    J  = BFJob;

    PC = J->TotalProcCount;

    LBFQueue[BFIndex] = (mjob_t *)1;

    if (MJobSelectMNL(
         J,
         P,
         BFNodeList,
         MNodeList,
         NodeMap,
         MBNOTSET,
         FALSE,
         &PReq,
         NULL,
         NULL) == FAILURE)
      {
      MDB(0,fSCHED) MLog("ERROR:    cannot select tasks for job %s in %s\n",
        FName,
        J->Name);

      continue;
      }

    if (MJobAllocMNL(
          J,
          MNodeList,
          NodeMap,
          0,
          P->NAllocPolicy,
          MSched.Time,
          NULL,
          NULL) == FAILURE)
      {
      MDB(1,fSCHED) MLog("ERROR:    cannot allocate nodes for job '%s' in %s\n",
        J->Name,
        FName);

      continue;
      }

    if (MJobStart(J,NULL,NULL,"job backfilled") == FAILURE)
      {
      if (PReq == TRUE)
        {
        /* create rsv for preempted resources */

        if ((MJobReserve(&J,NULL,0,mjrQOSReserved,FALSE) == SUCCESS) && (J->Rsv != NULL))
          J->Rsv->ExpireTime = MSched.Time + MDEF_PREEMPTRSVTIME;

        continue;
        }

      MDB(1,fSCHED) MLog("ERROR:    cannot start job '%s' in %s\n",
        J->Name,
        FName);

      continue;
      }

    /* job successfully started */

    bmset(&J->SysFlags,mjfBackfill);

    MJobUpdateFlags(J);          

    MStatUpdateBFUsage(J);

    APC -= PC;
    }  /* END while (1) */

  MNLMultiFree(MNodeList);
  MNLFree(&tmpNL);
  MUFree(&NodeMap);

  MDB(2,fSCHED) MLog("INFO:     partition %s has %d/%d nodes/procs available after backfill (%d jobs, %d evals)\n",
    P->Name,
    P->IdleNodes,
    P->ARes.Procs,
    jindex,
    JECount);

  return(SUCCESS);
  }  /* END MBFBestFit() */





#define MMAX_GREEDY_JOBS 100

/**
 * Perform 'greedy' search of possible backfill jobs.
 *
 * NOTE:  does not support malleable jobs
 *
 * @param BFQueue (I)
 * @param PolicyLevel (I)
 * @param BFNodeList (I)
 * @param Duration (I)
 * @param BFNodeCount (I)
 * @param BFProcCount (I)
 * @param P (I)
 */

int MBFGreedy(

  mjob_t    **BFQueue,               /* I */
  enum MPolicyTypeEnum PolicyLevel,  /* I */
  mnl_t      *BFNodeList,            /* I */
  mulong      Duration,              /* I */
  int         BFNodeCount,           /* I */
  int         BFProcCount,           /* I */
  mpar_t     *P)                     /* I */

  {
  mjob_t      *BestList[MMAX_GREEDY_JOBS];
  int          BestValue;

  mjob_t      *BFList[MMAX_GREEDY_JOBS];
  int          BFValue;
  int          BFIndex[MMAX_GREEDY_JOBS];        

  mjob_t      *LBFQueue[MMAX_GREEDY_JOBS];      

  int          SPC;

  mjob_t      *J;

  int          StartIndex;

  int          index;
  int          sindex;
  int          jindex;

  int          scount;

  mbool_t      Failure;

  char        *NodeMap = NULL;

  mnl_t   *MNodeList[MMAX_REQ_PER_JOB];
  mnl_t    NodeList;
  mnl_t    tmpNL;

  enum MNodeStateEnum *NSList = NULL;

  static int   ReservationMisses = 0;

  int          PC;

  mpar_t      *GP;

  mbitmap_t    BM;

  enum MBFMetricEnum BFMetric;

  const char *FName = "MBFGreedy";

  MDB(1,fSCHED) MLog("%s(BFQueue,%s,BFNodeList,%ld,%d,%d,%s)\n",
    FName,
    MPolicyMode[PolicyLevel],
    Duration,
    BFNodeCount,
    BFProcCount,
    P->Name);

  GP = &MPar[0];

  BFMetric = (P->BFMetric != mbfmNONE) ? P->BFMetric : GP->BFMetric;

  /* initialize state */

  StartIndex  = 0;

  BFValue     = 0;
  BFList[0]   = NULL;
  
  BestValue   = 0;
  BestList[0] = NULL;

  sindex = 0;
  scount = 0;

  memcpy(LBFQueue,BFQueue,sizeof(LBFQueue));
  memset(BFIndex,0,sizeof(BFIndex));    

  SPC = 0;

  NSList = (enum MNodeStateEnum *)MUCalloc(1,sizeof(enum MNodeStateEnum) * MSched.M[mxoNode]);
  __MBFStoreClusterState(NSList);

  MDB(4,fSCHED) MLog("INFO:     attempting greedy backfill w/%d procs, %ld time\n",
    BFProcCount,
    Duration);

  MDB(4,fSCHED)
    {
    char tmpLine[MMAX_BUFFER];

    char *BPtr;
    int   BSpace;

    MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

    MUSNPrintF(&BPtr,&BSpace,"INFO:     backfill jobs: ");

    for (index = 0;LBFQueue[index] != NULL;index++)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        (index > 0) ? "," : "",
        LBFQueue[index]->Name);
      }

    MUSNCat(&BPtr,&BSpace,"\n");

    MLog("%s",
      tmpLine);
    }

  if (MNodeMapInit(&NodeMap) == FAILURE)
    {
    MUFree((char **)&NSList);

    return(FAILURE);
    }

  bmset(&BM,mlActive);

  MNLMultiInit(MNodeList);
  MNLInit(&NodeList);
  MNLInit(&tmpNL);

  while (scount++ < GP->BFMaxSchedules)
    {
    BFValue        = 0;
    BFList[sindex] = NULL;

    if ((BestValue > 0) && (BestList[0] == NULL))
      {
      MDB(0,fSCHED) MLog("ERROR:    BestVal %d achieved but schedule is empty (%p, %p, ...)\n",
        BestValue,
        (void *)BestList[0],
        (void *)BestList[1]);
  
      __MBFRestoreClusterState(NSList);
 
      MNLMultiFree(MNodeList);
      MNLFree(&NodeList);
      MNLFree(&tmpNL);

      MUFree((char **)&NSList);
      MUFree(&NodeMap);

      return(FAILURE);
      }

    /* locate next job for current schedule */

    for (jindex = StartIndex;jindex < MMAX_GREEDY_JOBS;jindex++)
      {

      if (LBFQueue[jindex] == NULL)
        break;

      J = LBFQueue[jindex];

      if (!MJobPtrIsValid(J))
        {
        /* job is not eligible */

        MDB(6,fSCHED) MLog("INFO:     BFQueue[%d] is not eligible\n",
          jindex);

        continue;
        }

      if ((J->Depend != NULL) &&
          (J->Depend->Type == mdJobSyncMaster))
        {
        if (MJobCombine(J,&J) == FAILURE)
          {
          MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (dependency)\n",
            J->Name,
            P->Name);

          continue;
          }
        }

      if (bmisset(&J->IFlags,mjifBillingReserveFailure))
        {
        /* Billing failed already failed this iteration, wait until next iteration */

        continue;
        }

      if (bmisset(&J->IFlags,mjifReqTimeLock))
        {
        MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (time locked)\n",
          J->Name,
          P->Name);

        continue;
        }

      PC = J->TotalProcCount;

      if (scount >= GP->BFMaxSchedules)
        {
        MDB(1,fSCHED) MLog("ALERT:    max backfill schedules reached in %s (%d) on iteration %d\n",
          FName,
          GP->BFMaxSchedules,
          MSched.Iteration);

        break;
        }

      MDB(5,fSCHED)
        {
        if (sindex > 0)
          {
          MDB(4,fSCHED) MLog("INFO:     checking  ");

          for (index = 0;index < sindex;index++)
            {
            MLogNH("[%s]",
              BFList[index]->Name);
            }  /* END for (index) */

          MLogNH(" (%03d)\n",
            SPC);
          }
        else
          {
          MDB(4,fSCHED) MLog("INFO:     checking empty list\n");
          }
        
        MDB(4,fSCHED) MLog("INFO:     checking S[%02d] '%s'  (p: %03d t: %06ld)\n",
          jindex,
          J->Name,
          PC,
          J->WCLimit);
        }

      if ((J->State != mjsIdle) || (J->EState != mjsIdle))
        {
        MDB(6,fSCHED) MLog("INFO:     job %s is not idle\n",
          J->Name);

        LBFQueue[jindex] = (mjob_t *)1;

        continue;
        }

      if (J->SpecWCLimit[0] > Duration)
        {
        /* NOTE:  check should incorporate machine speed (NYI) */

        MDB(6,fSCHED) MLog("INFO:     job %s is too long\n",
          J->Name);

        LBFQueue[jindex] = (mjob_t *)1;        

        continue;
        }

      if (PC > BFProcCount)
        {
        /* job will not fit in backfill window */

        MDB(6,fSCHED) MLog("INFO:     job %s is too large\n",
          J->Name);

        LBFQueue[jindex] = (mjob_t *)1;        

        continue;
        }

      if ((PC + SPC) > BFProcCount)
        {
        /* job will not fit in existing schedule */

        MDB(6,fSCHED) MLog("INFO:     job %s does not fit in schedule\n",
          J->Name);

        continue;
        }

      MDB(4,fSCHED) MLog("INFO:     feasible backfill %s (P: %3d T: %ld)\n",
        J->Name,
        PC,
        J->WCLimit);

      /* NYI:  must reserve and unreserve nodes */

      MStat.EvalJC++;

      if (MJobCheckLimits(
            J,
            PolicyLevel,
            P,
            &BM,
            NULL,
            NULL) == FAILURE)
        {
        MDB(8,fSCHED) MLog("INFO:     job %s does not meet active fairness policies\n",
          J->Name);

        /* NOTE: de-activate when res/release enabled */

        if (sindex == 0)
          LBFQueue[jindex] = (mjob_t *)1;        
 
        continue;
        }  /* END if (MJobCheckLimits() == FAILURE) */

      if (MReqGetFNL(
            J,
            J->Req[0],
            P,
            BFNodeList,
            &tmpNL,
            NULL,
            NULL,
            MMAX_TIME,
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

      if (MJobSelectMNL(
            J,
            P,
            &tmpNL,
            MNodeList,
            NodeMap,
            MBNOTSET,
            FALSE,
            NULL,
            NULL,
            NULL) == FAILURE)
        {
        /* resources not available with current schedule in place */

        if (sindex == 0)
          LBFQueue[jindex] = (mjob_t *)1;          

        continue;
        }

      if (MJobAllocMNL(
            J,
            MNodeList,
            NodeMap,
            0,
            P->NAllocPolicy,
            MSched.Time,
            NULL,
            NULL) == FAILURE)
        {
        /* cannot allocate resources with current schedule */
 
        if (sindex == 0)
          LBFQueue[jindex] = (mjob_t *)1;
 
        continue;
        }

      /* add job to list */

      BFList[sindex]  = J;
      BFIndex[sindex] = jindex;

      SPC             += PC;

      sindex++;   

      MJobGetNL(J,&NodeList);

      __MBFReserveNodes(&NodeList);

      MDB(6,fSCHED) MLog("INFO:     reservation added for Job[%03d] '%s'\n",
        sindex,
        J->Name);

      StartIndex = BFIndex[sindex] + 1;

      MDB(4,fSCHED) MLog("INFO:     located job '%s' for greedy backfill (size: %d length: %ld)\n",
        J->Name,
        PC,
        J->WCLimit);
      }  /* END for (jindex) */

    /* end of selected jobs list reached.  (current schedule complete) */

    /* determine utility of current schedule */

    BFValue = 0;

    switch (BFMetric)
      {
      case mbfmPS:

        for (index = 0;index < sindex;index++)
          {
          BFValue += 
            BFList[index]->TotalProcCount * BFList[index]->WCLimit;
          }

        break;

      case mbfmWalltime:

        for (index = 0;index < sindex;index++)
          {
          BFValue += BFList[index]->WCLimit;
          }

        break;

      case mbfmProcs:
      default:
 
        for (index = 0;index < sindex;index++)
          {
          BFValue += BFList[index]->TotalProcCount;
          }
 
        break;
      }  /* END switch (BFMetric) */

    if (BFValue > BestValue)
      {
      /* copy current list to best list */

      for (index = 0;index < sindex;index++)
        {
        BestList[index] = BFList[index];
        }

      BestList[index] = NULL;
       
      BestValue       = BFValue;

      MDB(1,fSCHED)
        {
        MDB(1,fSCHED) MLog("INFO:     improved list found by greedy in %d searches (utility: %d  procs available: %d)\n",
          scount,
          BestValue,
          BFProcCount - SPC);

        for (index = 0;BestList[index] != NULL;index++)
          {
          J = BestList[index];

          MDB(1,fSCHED) MLog("INFO:     %02d:  job '%s' procs: %03d time: %06ld\n",
            index,
            J->Name,
            J->TotalProcCount,
            J->WCLimit);
          }
        }

      if ((BFMetric == mbfmProcs) && (BFProcCount == SPC))
        {
        /* break out if perfect schedule is found */

        break;
        }
      }     /* END if (BFValue > BestValue) */ 

    /* backtrack if at limit */

    if (sindex == 0)
      {
      /* if schedule is empty */

      break;
      }
    else 
      {
      sindex--;

      J = BFList[sindex];

      SPC -= J->TotalProcCount;

      /* release reservation */

      MDB(6,fSCHED) MLog("INFO:     releasing reservation for Job %d '%s'\n",
        sindex,
        J->Name);

      MJobGetNL(J,&NodeList);

      __MBFReleaseNodes(NSList,&NodeList);
      }    /* END else if (sindex == 0) */

    StartIndex = BFIndex[sindex] + 1;
    }   /* END  while(scount++ < GP->BFMaxSchedules) */

  __MBFRestoreClusterState(NSList);

  if (BestValue == 0)
    {
    /* no schedule found */

    MDB(5,fSCHED) MLog("INFO:     no jobs found to backfill\n");

    MNLMultiFree(MNodeList);
    MNLFree(&NodeList);
    MNLFree(&tmpNL);

    MUFree((char **)&NSList);
    MUFree(&NodeMap);

    return(SUCCESS);
    }

  MDB(3,fSCHED)
    {
    MDB(1,fSCHED) MLog("INFO:     final list found by greedy in %d searches (%d of %d procs/utility: %d)\n",
      scount,
      SPC,
      BFProcCount,
      BestValue);

    for (index = 0;BestList[index] != NULL;index++)
      {
      J = BestList[index];

      MDB(1,fSCHED) MLog("INFO:     S[%02d]  job %16s   procs: %03d time: %06ld\n",
        index,
        J->Name,
        J->TotalProcCount,
        J->WCLimit);
      }
    }

  /* launch schedule */

  Failure = FALSE;

  for (index = 0;BestList[index] != NULL;index++)
    {
    J = BestList[index];

    if (MJobCheckLimits(
          J,
          PolicyLevel,
          P,
          &BM,
          NULL,
          NULL) == FAILURE)
      {
      MDB(1,fSCHED)
        {
        MDB(1,fSCHED) MLog("ERROR:    scheduling failure %3d in %s (policy violation/no reservation)  iteration: %4d\n",
          ++ReservationMisses,
          FName,
          MSched.Iteration);
 
        MJobShow(J,0,NULL);
        }
 
      Failure = TRUE;
 
      continue;
      }  /* END if (MJobCheckLimits() == FAILURE) */

    if (MJobSelectMNL(
          J,
          P,
          BFNodeList,
          MNodeList,
          NodeMap,
          MBNOTSET,
          FALSE,
          NULL,
          NULL,
          NULL) == FAILURE)
      {
      MDB(1,fSCHED)
        {
        MDB(1,fSCHED) MLog("ERROR:    cannot get tasks in %s on (ERR: %d/no reservation/iteration %d)\n",
          FName,
          ++ReservationMisses,
          MSched.Iteration);
 
        MJobShow(J,0,NULL);
        }
 
      Failure = TRUE;
     
      continue;
      }

    /* job evaluation successful */

    if (MJobAllocMNL(
          J,
          MNodeList,
          NodeMap,
          0,
          P->NAllocPolicy,
          MSched.Time,
          NULL,
          NULL) == FAILURE)
      {
      MDB(1,fSCHED) MLog("ERROR:    cannot allocate resources to job %s\n",
        J->Name);
 
      Failure = TRUE;

      continue;
      }  /* END if (MJobAllocMNL() == SUCCESS) */

    if (MJobStart(J,NULL,NULL,"job backfilled") == FAILURE)
      {
      MDB(1,fSCHED) MLog("ERROR:    cannot start job %s in %s()\n",
        FName,
        J->Name);
 
      Failure = TRUE;

      continue;
      }

    /* job successfully started */

    bmset(&J->SysFlags,mjfBackfill);

    MJobUpdateFlags(J);          

    MStatUpdateBFUsage(J);
    }  /* END for (index) */

  /* if reservations fail, use bestfit as fallback */

  if (Failure == TRUE)
    {
    int          tmpNodes;
    int          tmpTasks;
 
    long         tmpTime;

    mjob_t      *tmpJ = NULL;

    mnl_t    tmpNL;          

    MNLInit(&tmpNL);

    MJobMakeTemp(&tmpJ);

    tmpJ->Req[0]->DRes.Procs = 1;
    tmpJ->Req[0]->PtIndex    = P->Index;

    /* initialize gres to prevent seg fault in MBFGetWindow */
    MSNLInit(&tmpJ->Req[0]->DRes.GenericRes);

    if (MBFGetWindow(
          tmpJ, 
          0,
          &tmpNodes,
          &tmpTasks,
          &tmpNL,
          &tmpTime,
          MSched.SeekLong,
          NULL) == SUCCESS)
      {
      MBFBestFit(
        BFQueue,
        PolicyLevel,
        &tmpNL,
        tmpTime,
        tmpNodes,
        tmpTasks,
        P);
      }

    MNLFree(&tmpNL);

    /* free gres */
    MSNLFree(&tmpJ->Req[0]->DRes.GenericRes);

    MJobFreeTemp(&tmpJ);
    }    /* END if (Failure == TRUE) */

  MNLMultiFree(MNodeList);
  MNLFree(&NodeList);
  MNLFree(&tmpNL);
  MUFree(&NodeMap);
  MUFree((char **)&NSList);

  MDB(2,fSCHED) MLog("INFO:     partition %s nodes/procs available after %s: %d/%d\n",
    P->Name,
    FName,
    P->IdleNodes,
    P->ARes.Procs);

  return(SUCCESS);
  }  /* END MBFGreedy() */




/* END MBF.c */


