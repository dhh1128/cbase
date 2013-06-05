/* HEADER */

      
/**
 * @file MJobRsv.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Start job with reservation.
 *
 * NOTE: The JOBSTARTLOGLEVEL parameter sets the log level to be used by this
 *        function and functions called by this function.  The original log 
 *        level must be put back when exiting this function.
 *
 * @see MJobStart() - peer
 *
 * @param J (I) [modified]
 */

int MJobStartRsv(

  mjob_t *J)  /* I (modified) */

  {
  int         rc;
  mpar_t     *P;

  mulong      Delta;

  mnl_t  *MNodeList[MMAX_REQ_PER_JOB];

  mutime OriginalSpecZeroTime;

  char       *NodeMap = NULL;

  mreq_t     *RQ;

  enum MNodeAllocPolicyEnum NAPolicy;

  char EMsg[MMAX_LINE];

  int OldLogLevel;

  mbitmap_t BM;

  const char *FName = "MJobStartRsv";

  OldLogLevel = mlog.Threshold;

  if (MSched.JobStartThreshold > 0)
    {
    mlog.Threshold = MSched.JobStartThreshold;
    }

  MDB(5,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if ((J == NULL) || (J->Rsv == NULL))
    {
    return(FAILURE);
    }

  /* the time to schedule has arrived... */

  if ((MSched.Time < J->Rsv->StartTime) || (J->State != mjsIdle))
    {
    char DString[MMAX_LINE];

    MULToDString((mulong *)&J->Rsv->StartTime,DString);

    MDB(2,fSCHED) MLog("INFO:     cannot start job %s, reserve time in %30s",
      J->Name,
      DString);

    mlog.Threshold = OldLogLevel;

    return(FAILURE);
    }

  /* NOTE:  for dynamic partitions (ie, hybrid), allocated nodes may change 
            partitions as nodes are provisioned */

  P = &MPar[MAX(0,J->Rsv->PtIndex)];

  if ((P->RM != NULL) && (P->RM->State == mrmsDisabled))
    {
    MDB(2,fSCHED) MLog("INFO:     cannot start job %s, RM %s disabled",
      J->Name,
      P->RM->Name);

    mlog.Threshold = OldLogLevel;

    return(FAILURE);
    }

  bmset(&BM,mlActive);

  if (MJobCheckLimits(
        J,
        mptSoft,
        P,
        &BM,
        NULL,
        NULL) == FAILURE)
    {
    /* job limits violated */

    if (MSched.Mode != msmMonitor)
      {
      if (MSched.Mode == msmNormal)
        {
        MOSSyslog(LOG_NOTICE,"cannot run reserved job '%s'.  job violates active policy",
          J->Name);
        }

      MDB(1,fSCHED) MLog("ALERT:    cannot run reserved job '%s'.  (job violates active policy)\n",
        J->Name);

      MJobSetHold(J,mhDefer,MSched.DeferTime,mhrPolicyViolation,"reserved job violates active policy");
      }

    mlog.Threshold = OldLogLevel;

    return(FAILURE);
    }  /* END if (MJobCheckLimits() == FAILURE) */

  /* locate available resources */

  Delta = MSched.Time - J->Rsv->StartTime;

  if (((long)Delta > MSched.MaxRMPollInterval) || 
       (Delta >= J->SpecWCLimit[0]))
    {
    /* NOTE:  only 'squeeze' job in if delay is less than 1 poll interval 
              and less than job duration */

    Delta = 0;
    }

  /* Save off the original specified wall clock limit from the [0] entry in the
   * array since it can be swapped with the [1] entry in the array. We will
   * restore the [0] element later on. */

  OriginalSpecZeroTime = J->SpecWCLimit[0];

  if (Delta > 0)
    {
    /* NOTE:  rsv is starting late, grant job access to resources of future job rsv's if required */

    /* squeeze job in, rsv's will auto adjust on the next iteration */

    MDB(1,fSCHED) MLog("INFO:     delayed rsv detected for reserved job '%s' (%ld seconds)  attempting squeeze\n",
      J->Name,
      Delta);

    J->WCLimit -= Delta;
    J->SpecWCLimit[0] -= Delta;
    J->SpecWCLimit[1] -= Delta;
    }

  MNodeMapInit(&NodeMap);
  MNLMultiInit(MNodeList);

  if (MJobIsPreempting(J) == TRUE)
    {
    int rqindex = 0;

    memset(NodeMap,mnmPositiveAffinity,sizeof(char) * MSched.M[mxoNode]);

#ifdef MNOT
    /* Removed this code because Req[]->NodeList was emtpy in a grid env and 
     * letting other jobs slip ahead. When a ignidlejobrsv is behind the 
     * preemptor it has a reservation and if the preemptor doesn't stop it then
     * it will start ahead of the preemptor and the preemptor will kill it. The
     * job's reservation has the nodelist so use that. */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      MNLCopy(MNodeList[rqindex],&J->Req[rqindex]->NodeList);
      }
#endif

    MNLCopy(MNodeList[rqindex],&J->Rsv->NL);

    rc = SUCCESS;
    }
  else if (bmisset(&J->IFlags,mjifAllocatedNodeSets) && (J->Rsv != NULL))
    {
    /* SPANEVENLY: in this situation the job already has a reservation, we can't rely on MJobSelectMNL() 
       to make the right choice as MJobAllocMNL will exit and just assume the job's reservation works.  
       Instead we need to call MJobGetEStartTime() and see if there is a starttime of now and then use 
       that nodelist if there is one */

    long        StartTime = (long)MSched.Time;

    /* determine if the job can start now */

    if ((MJobGetEStartTime(J,&P,NULL,NULL,MNodeList,&StartTime,NULL,FALSE,FALSE,NULL) == FAILURE) ||
        (StartTime != (long)MSched.Time))
      {
      rc = FAILURE;
      }
    else
      {
      MNLCopy(&J->Req[0]->NodeList,MNodeList[0]);

      rc = SUCCESS;
      }
    }   /* END if (bmisset(&J->IFlags,mjifAllocatedNodeSets) && (J->R != NULL)) */
  else
    {
    rc = MJobSelectMNL(
          J,
          P,
          (bmisset(&MSched.Flags,mschedfEnforceReservedNodes)) ? &J->Rsv->NL : NULL,
          MNodeList,        /* O */
          NodeMap,          /* O */
          TRUE,
          FALSE,
          NULL,
          EMsg,             /* O */
          NULL);
    }  /* END else */

  /* restore original values */

  J->WCLimit += Delta;

  /* We saved off the original so we will use that instead of trying
   * to use the delta to restore it to its original value. rt3851 */

  J->SpecWCLimit[0] = OriginalSpecZeroTime;
  J->SpecWCLimit[1] = OriginalSpecZeroTime;

  if (rc == FAILURE)
    {
    if (bmisset(&J->IFlags,mjifRanProlog))
      {
      /* If the prolog ran, always run the epilog */

      MJobLaunchEpilog(J);

      bmset(&J->IFlags,mjifRanEpilog);
      }

    MJobDiagRsvStartFailure(J,P,EMsg);

    mlog.Threshold = OldLogLevel;

    MUFree(&NodeMap);
    MNLMultiFree(MNodeList);

    return(FAILURE);
    }  /* END if (MJobSelectMNL() == FAILURE) */

  RQ = J->Req[0];

  NAPolicy = (RQ->NAllocPolicy != mnalNONE) ?
    RQ->NAllocPolicy :
    P->NAllocPolicy;

  if (MJobAllocMNL(
       J,
       MNodeList,
       NodeMap,
       NULL,
       NAPolicy,
       MSched.Time,
       NULL,
       NULL) == FAILURE)
    {
    MDB(1,fSCHED) MLog("ERROR:    cannot allocate nodes for job '%s'\n",
      J->Name);

    mlog.Threshold = OldLogLevel;

    MUFree(&NodeMap);
    MNLMultiFree(MNodeList);

    return(FAILURE);
    }

  MUFree(&NodeMap);
  MNLMultiFree(MNodeList);

  if (MJobStart(J,EMsg,NULL,"reserved job started") == FAILURE)
    {
    if (MSched.Mode == msmNormal)
      {
      char tmpLine[MMAX_LINE];

      MDB(1,fSCHED) MLog("ALERT:    cannot run reserved job '%s' - %s\n",
        J->Name,
        EMsg);

      snprintf(tmpLine,sizeof(tmpLine),"cannot start job on reserved resources - %s",
        EMsg);

      MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

      if (MJobIsPreempting(J) == TRUE)
        {
        /* need to extend reservation properly */
   
        MDB(4,fSCHED) MLog("INFO:     rsv for job '%s' blocked by RM race condition.  (retrying reservation)\n",
          J->Name);
   
        MRsvSetTimeframe(J->Rsv,mraStartTime,mSet,MSched.Time + MSched.JobRsvRetryInterval,EMsg);
        }
      }    /* END if (MSched.Mode == msmNormal) */

    mlog.Threshold = OldLogLevel;

    return(FAILURE);
    }

  J->RsvRetryTime = 0;

  /* Mark workflow VC (if exists) as started */

  if (J->ParentVCs != NULL)
    {
    mln_t *tmpVCL = NULL;
    mvc_t *tmpVC = NULL;

    for (tmpVCL = J->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
      {
      tmpVC = (mvc_t *)tmpVCL->Ptr;

      if (tmpVC == NULL)
        continue;

      if (bmisset(&tmpVC->Flags,mvcfWorkflow))
        {
        bmset(&tmpVC->Flags,mvcfHasStarted);

        break;
        }
      }  /* END for (tmpVCL) */
    }  /* END if (J->ParentVCs != NULL) */

  /* job successfully started */

  MDB(2,fSCHED) MLog("INFO:     reserved job '%s' started\n",
    J->Name);

  mlog.Threshold = OldLogLevel;

  return(SUCCESS);
  }  /* END MJobStartRsv() */





/**
 * Release job reservation(s).
 *
 * @see MRsvDestroy() - child
 * 
 * NOTE: assumes ALL job reservations are registered in MRsvHT table 
 *
 * @param J (I) [modified]
 * @param IsCancel () job is being cancelled
 * @param IsDestroy () job is being destroyed
 */

int MJobReleaseRsv(

  mjob_t  *J,         /* I (modified) */
  mbool_t  IsCancel,  /* job is being cancelled */
  mbool_t  IsDestroy) /* job is being destroyed */

  {
  int rqindex;

  const char *FName = "MJobReleaseRsv";

  MDB(7,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    (J == NULL) ? "NULL" : J->Name,
    MBool[IsCancel]);

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* Update standing reservations MAXJOB Policy */

  if ((IsDestroy == TRUE) && (MSched.Shutdown == FALSE) && (J->Rsv != NULL))
    MSRUpdateJobCount(J,FALSE);

  /* destroy all req rsv's */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    if (J->Req[rqindex]->R == NULL)
      continue;

    if (MRsvIsValid(J->Req[rqindex]->R))
      {
      /* NOTE:  assumes ALL job reservations are registered in MRsvHT table */
      MRsvDestroy(&J->Req[rqindex]->R,TRUE,IsDestroy);
      }
    }  /* END for (rqindex) */

  /* NOTE:  releasing job rsv does not patch 'blocked resource' holes in 
            overlapping user/admin reservations */

  J->Rsv = NULL;

  return(SUCCESS);
  }  /* END MJobReleaseRsv() */




/**
 *
 *
 * @param J (I)
 */

int MJobSyncStartRsv(

  mjob_t *J)  /* I */

  {
  mdepend_t *D = NULL;

  mjob_t *tJ;

  const char *FName = "MJobSyncStartRsv";

  MDB(3,fSCHED) MLog("%s(J)\n",
    FName);

  if ((J == NULL) || (J->Depend == NULL))
    {
    return(FAILURE);
    }

  /* NOTE: only process 'AND' dependencies */

  D = J->Depend->NextAnd;

  while (D != NULL)
    {
    if (MJobFind(D->Value,&tJ,mjsmBasic) == FAILURE)
      {
      return(FAILURE);
      }

    if ((tJ->Rsv == NULL) ||
        (tJ->Rsv->StartTime > J->Rsv->StartTime))
      {
      MDB(3,fSCHED) MLog("WARNING:  job '%s' not synchronized to start with job '%s'\n",
        J->Name,
        tJ->Name);

      return(FAILURE);
      }

    D = D->NextAnd;
    }

  D = J->Depend->NextAnd;

  if (MJobStartRsv(J) == FAILURE)
    {
    return(FAILURE);
    }

  while (D != NULL)
    {
    if (MJobFind(D->Value,&tJ,mjsmBasic) == FAILURE)
      {
      D = D->NextAnd;

      continue;
      }

    /* HACK: MJobStartRsv() doesn't like to start this job in a "hold" state */

    tJ->State = mjsIdle;

    if (MJobStartRsv(tJ) == FAILURE)
      {
      /* requeue all the jobs that were already started */

      MDB(3,fSCHED) MLog("WARNING:  could not start job '%s' requeuing any synchronized jobs\n",
        tJ->Name);

      D = J->Depend->NextAnd;

      if (MJOBISACTIVE(J))
        {
        MJobRequeue(J,NULL,NULL,NULL,NULL);
        }

      while (D != NULL)
        {
        if (MJobFind(D->Value,&tJ,mjsmBasic) == FAILURE)
          {
          D = D->NextAnd;

          continue;
          }

        if (MJOBISACTIVE(tJ))
          {
          MJobRequeue(tJ,NULL,NULL,NULL,NULL);
          }

        MJobSetHold(tJ,mhBatch,0,mhrAdmin,"held for dependency");

        D = D->NextAnd;
        }  /* END while (D != NULL) */

      return(FAILURE);
      }  /* END if (MJobStartRsv(tJ) == FAILURE) */

    D = D->NextAnd;
    }  /* END while (D != NULL) */

  return(SUCCESS);
  }  /* END MJobSyncStartRsv() */






/**
 * Determine and report why job cannot start within target job reservation.
 *
 * NOTE:  This routine may modify job's reservation, create a job hold, or set 
 *        J->SMinTime
 *
 * @param J (I) [modified]
 * @param P (I) 
 * @param FailMsg (I)
 */

int MJobDiagRsvStartFailure(

  mjob_t *J,        /* I */
  mpar_t *P,        /* I */
  char   *FailMsg)  /* I */

  {
  mbool_t  DoNodeStateCheck;

  int      NodeStateDelayCount;
  int      JobRsvDelayCount;
  int      OSDelayCount = 0;

  int      ReqOS = 0;

  mnode_t *NodeStateDelayN  = NULL;
  mnode_t *JobRsvDelayN     = NULL;

  mbool_t  NodeStateFailure;

  char    *DelayJID;

  mnl_t *NL = NULL;

  int      nindex;

  mnode_t *N;
  int      TC;

  char     EMsg[MMAX_LINE];

  /* inadequate nodes to start job */

  MDB(2,fSCHED) MLog("ALERT:    insufficient nodes to run reserved job '%s' on iteration %d\n",
    J->Name,
    MSched.Iteration);

  /* rsv failures typically result from race conditions or resource failures */

  DoNodeStateCheck = TRUE;

  /* NOTE: DoNodeStateCheck always true */

  NodeStateDelayCount = 0;
  JobRsvDelayCount    = 0;

  DelayJID   = NULL;

  /* only look at nodes in rsv */

  NL = &J->Rsv->NL;

  if (NL != NULL)
    {
    MReqGetReqOpsys(J,&ReqOS);
 
    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(NL,nindex);

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;
  
      if (N->Name[0] == '\1')
        continue;

      if (N->Name[0] == '\1')
        continue;

      if ((ReqOS != 0) && 
          ((N->OSList != NULL) || (MSched.DefaultN.OSList != NULL)))
        {
        if ((ReqOS != N->ActiveOS) && (ReqOS == N->NextOS))
          {
          OSDelayCount++;
          }
        else if (ReqOS != N->ActiveOS)
          {
          int oindex;
          const mnodea_t *OSList = (N->OSList != NULL) ? N->OSList : MSched.DefaultN.OSList;

          for (oindex = 0;OSList[oindex].AIndex != 0;oindex++)
            {
            if (OSList[oindex].AIndex == ReqOS)
              {
              OSDelayCount++;

              break;
              }
            }
          }
        }

      if ((N->PtIndex != P->Index) &&
          (P->Index != 0) &&
          !bmisset(&J->Flags,mjfCoAlloc))
        {
        continue;
        }

      NodeStateFailure = FALSE;
 
      if (DoNodeStateCheck == TRUE)
        { 
        if (MNodeCheckPolicies(
              J,
              N,
              MSched.Time,
              &TC,
              EMsg) == FAILURE)
          {
          MDB(6,fSCHED) MLog("INFO:     node '%s' is not available for scheduling.  (state: %s/%s  reason: %s)\n",
            N->Name,
            MNodeState[N->State],
            MNodeState[N->EState],
            EMsg);
 
          NodeStateDelayN = N;
 
          NodeStateDelayCount++;

          NodeStateFailure = TRUE;
          }
        else if (N->EState == mnsBusy)
          {
          MDB(6,fSCHED) MLog("INFO:     node '%s' is not available for scheduling.  (state: %s/%s)\n",
            N->Name,
            MNodeState[N->State],
            MNodeState[N->EState]);

          NodeStateDelayN = N;

          NodeStateDelayCount++;

          NodeStateFailure = TRUE;
          }
        }    /* END if (DoNodeStateCheck == TRUE) */

      if ((NodeStateFailure == FALSE) && 
          (N->ARes.Procs < (N->CRes.Procs - N->DRes.Procs)))
        {
        MDB(6,fSCHED) MLog("INFO:     node '%s' is not available for scheduling.  (untracked cpuload))\n",
          N->Name);

        NodeStateDelayN = N;
  
        NodeStateDelayCount++;

        NodeStateFailure = TRUE;
        }
 
      /* perform RM specific checks */
  
      if (N->RM == NULL)
        {
        continue;
        }
  
      /* check if job is blocked by extended node job rsv */
  
      if (N->RE != NULL)
        {
        mrsv_t *R;
        mre_t  *RE;
 
        for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
          {
          R = MREGetRsv(RE);
  
          if (R->Type != mrtJob)
            continue;

          if (R == J->Rsv)
            continue;

          if ((R->J != NULL) && 
              (R->J->RemainingTime > 0))
              continue;
 
          MDB(6,fSCHED) MLog("INFO:     %s node '%s' is not available for scheduling.  (blocked by job reservation)\n",
            MRMType[N->RM->Type],
            N->Name);

          JobRsvDelayN = N;

          if (DelayJID == NULL)
            DelayJID = R->Name;

          JobRsvDelayCount++;
 
          break; 
          }
        }    /* END if (N->R != NULL) */
      }      /* END for (nindex = 0;nindex < MMAX_NODE;nindex++) */
    }        /* END if (NL != NULL) */

  MDB(7,fSCHED) MLog("INFO:     job %s RsvRetryTime Time: %ld Rsv Starttime: %ld\n",
    J->Name,
    J->RsvRetryTime,
    J->Rsv->StartTime);
 
  if ((((J->RsvRetryTime == 0) && 
        ((MSched.Time - J->Rsv->StartTime) < MPar[0].RsvRetryTime)) ||
       ((J->RsvRetryTime != 0) &&
        ((MSched.Time - J->RsvRetryTime) < MPar[0].RsvRetryTime))) &&
       ((MPar[0].RestrictedRsvRetryTime == FALSE) ||
        (NodeStateDelayCount > 0) || (JobRsvDelayCount > 0) || (OSDelayCount > 0)))
    {
    long tmpL;

    if (J->RsvRetryTime == 0)
      J->RsvRetryTime = MSched.Time;

    /* need to extend reservation properly */

    if (MSched.Mode == msmNormal)
      {
      MOSSyslog(LOG_NOTICE,"extending reservation for job %s, NodeStateDelayNC=%d, OSDelayCount=%d",
        J->Name,
        NodeStateDelayCount,
        OSDelayCount);
      }

    tmpL = MSched.Time + 1;

    MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mAdd);

    MDB(4,fSCHED) MLog("INFO:     rsv for job '%s' blocked by RM race condition.  (retrying reservation)\n",
      J->Name);

    MDB(5,fSCHED) MLog("INFO:     %lu seconds left in RESERVATIONRETRYTIME\n",
      (J->RsvRetryTime + MPar[0].RsvRetryTime) - MSched.Time);

    if (MJobReserve(&J,NULL,0,mjrPriority,FALSE) == SUCCESS)
      {
      char TString[MMAX_LINE];

      MULToTString(J->Rsv->StartTime - MSched.Time,TString);

      MDB(4,fSCHED) MLog("INFO:     reservation for job '%s' recreated for time %s\n",
        J->Name,
        TString);
      }
    else
      {
      MDB(4,fSCHED) MLog("ALERT:    cannot create slide-back reservation for job '%s'\n",
        J->Name);
      }
    }  /* END if (MSched.Time...) */
  else
    {
    char tmpLine[MMAX_LINE];

    MDB(2,fSCHED) MLog("ALERT:    insufficient nodes to run reserved job '%s'  (reservation released)\n",
      J->Name);

    MDB(2,fSCHED) MLog("INFO:     job delay: %ld  rsv retry time: %ld   (StateDelayNC=%d  JobRsvDelayNC=%d)\n",
      MSched.Time - J->Rsv->StartTime,
      MPar[0].RsvRetryTime,
      NodeStateDelayCount,
      JobRsvDelayCount);

    if ((NL != NULL) && 
        (MNLGetNodeAtIndex(NL,0,&N) == SUCCESS) && 
        (N->RM != NULL) && 
        (N->RM->Type == mrmtMoab))
      {
      MDB(4,fSCHED) MLog("INFO:     load on remote RM %s prevents job %s from start immediately - will retry later\n",
        N->RM->Name,
        J->Name);

      /* NO-OP */
      }
    else if (MSched.Mode != msmMonitor)
      {
      if (JobRsvDelayCount > 0)
        {
        snprintf(tmpLine,sizeof(tmpLine),"%d nodes unavailable to start reserved job after %ld seconds (job %s has exceeded wallclock limit on node %s - check job)",
          JobRsvDelayCount,
          MSched.Time - J->Rsv->StartTime,
          (DelayJID != NULL) ? DelayJID : "???",
          (JobRsvDelayN != NULL) ? JobRsvDelayN->Name : "???");
        }
      else if (NodeStateDelayCount > 0)
        {
        snprintf(tmpLine,sizeof(tmpLine),"%d nodes unavailable to start reserved job after %ld seconds (reserved node %s is in state '%s' - check node)",
          NodeStateDelayCount,
          MSched.Time - J->Rsv->StartTime,
          (NodeStateDelayN != NULL) ? NodeStateDelayN->Name : "???",
          (NodeStateDelayN != NULL) ? MNodeState[NodeStateDelayN->State] : "???");
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"nodes unavailable to start reserved job after %ld seconds (%s)",
          MSched.Time - J->Rsv->StartTime,
          (FailMsg[0] != '\0') ? FailMsg : "reason unknown");
        }
          
      if (MSched.Mode == msmNormal)
        {
        MOSSyslog(LOG_NOTICE,"reserved job %s cannot run.  deferring - %s",
          J->Name,
          tmpLine);
        }

      MSched.MaxRsvDelay = MAX(MSched.MaxRsvDelay,MSched.Time - J->Rsv->StartTime);

      if (MSched.DeferTime <= 0)
        {
        MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }
      else
        {
        MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,tmpLine);
        }
      }  /* END else if (MSched.Mode != msmMonitor) */

    J->RsvRetryTime = 0;

    MJobReleaseRsv(J,TRUE,TRUE);
    }  /* END else ((MSched.Time - J->R->StartTime) > ...) */

  return(SUCCESS);
  }  /* END MJobDiagRsvStartFailure() */
/* END MJobRsv.c */
