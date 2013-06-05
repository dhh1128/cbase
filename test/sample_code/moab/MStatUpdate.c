/* HEADER */

      
/**
 * @file MStatUpdate.c
 *
 * Contains: MStatUpdate
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Update backlog for an individual stats object
 *
 * @param S (I/O)
 * @param L (I)
 */

int MOUpdateBacklog(

  must_t *S,
  mcredl_t const *L)

  {
  mpu_t const * const IP = L->IdlePolicy;
  mulong JC;
  mulong PH;

  assert(S != NULL);
  assert(L != NULL);

  if (IP == NULL)
    {
    JC = 0;
    PH = 0;
    }
  else 
    {
    JC = IP->Usage[mptMaxJob][0];
    PH = IP->Usage[mptMaxPS][0] / MCONST_HOURLEN;
    }

  S->TQueuedJC  = JC;
  S->TQueuedPH +=  PH;
  S->IterationCount ++;

  return (SUCCESS);
  } /* END MOUpdateBacklog() */


/**
 * Update queued job statisiics.
 *
 * @param J (I)
 */

int MStatUpdateIdleJobUsage(

  mjob_t *J)  /* I */

  {
  int      rqindex;

  double   xfactor;

  mulong   qtime;

  int      rqproc;

  int      timeindex;
  int      procindex;

  int      statindex;
  int      stattotal;

  int      JPC;

  mgcred_t *U;
  mgcred_t *G;
  mgcred_t *A;
  mqos_t   *Q;
  mclass_t *C;

  double   interval;
  double   PE;

  mulong   tmpUL;

  must_t  *S[32];

  must_t  *GS;
  must_t  *tmpS;

  mreq_t  *RQ;
  mpar_t  *P;

  const char *FName = "MStatUpdateIdleJobUsage";

  MDB(4,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* update 
       adjust credential statistics (iteration based delta)
       fairshare usage (iteration based delta)
       policy based usage (full recalculation)
         policy usage cleared in MQueueSelectAllJobs()

     called from:         MQueueAddAJob()
  */

  JPC = J->TotalProcCount;

  if (JPC == 0)
    {
    MDB(3,fSTAT) MLog("INFO:     no tasks associated with job '%s' (no statistics available)\n",
      J->Name);

    return(FAILURE);
    }

  MJobGetPE(J,&MPar[0],&PE);  

  interval = MIN(
    (double)MSched.Interval / 100.0,
    (double)MSched.Time - J->StartTime);

  if ((MSched.Mode == msmSim) && (MSched.TimeRatio > 0.0))
    {
    /* NOTE:  must correct for time ratio impact */

    interval *= MSched.TimeRatio;
    }

  GS = &MPar[0].S;

  qtime   = MAX(1,MSched.Time - J->SubmitTime);
  xfactor = (double)(qtime + J->WCLimit) / J->WCLimit;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    { 
    RQ = J->Req[rqindex];

    P  = &MPar[RQ->PtIndex];

    if (RQ->DRes.Procs >= 0)
      rqproc = RQ->TaskCount * RQ->DRes.Procs;
    else
      rqproc = 1;  /* handle dedicated resources (NYI) */

    MDB(6,fSTAT) MLog("INFO:     job %-18s  xfactor: %.2f  qtime: %ld\n",
      J->Name,
      xfactor,
      qtime);

    /* determine stat grid location */

    tmpUL = MIN(J->WCLimit,MStat.P.MaxTime);

    for (timeindex = 0;MStat.P.TimeStep[timeindex] < tmpUL;timeindex++);

    tmpUL = MIN((mulong)rqproc,MStat.P.MaxProc);

    for (procindex = 0;MStat.P.ProcStep[procindex] < tmpUL;procindex++);

    timeindex = MIN(timeindex,(int)MStat.P.TimeStepCount);

    MDB(7,fSTAT) MLog("INFO:     updating statistics for Grid[time: %d][proc: %d]\n",
      timeindex,
      procindex);

    /* determine statistics to update */

    memset(S,0,sizeof(S));

    stattotal = 0;

    /* update long-term stats */

    S[stattotal++] = &MStat.SMatrix[0].Grid[timeindex][procindex];
    S[stattotal++] = &MStat.SMatrix[0].RTotal[procindex];
    S[stattotal++] = &MStat.SMatrix[0].CTotal[timeindex];

    /* update rolling stats */

    S[stattotal++] = &MStat.SMatrix[1].Grid[timeindex][procindex];
    S[stattotal++] = &MStat.SMatrix[1].RTotal[procindex];
    S[stattotal++] = &MStat.SMatrix[1].CTotal[timeindex];

    S[stattotal++] = &MPar[0].S;

    if ((GS->IStat != NULL) && (GS->IStatCount > 0))
      {
      S[stattotal++] = GS->IStat[GS->IStatCount - 1];
      }

    if (P->Index != 0)
      {
      S[stattotal++] = &P->S;
      }

    /* locate/adjust user stats */

    U = J->Credential.U;

    if (U != NULL)
      {
      MDB(7,fSTAT) MLog("INFO:     updating statistics for user %s (UID: %ld) \n",
        U->Name,   
        U->OID);

      U->MTime = MSched.Time;

      tmpS = &U->Stat;

      S[stattotal++] = tmpS;

      if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
        {
        S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
        }
      }
    else
      {
      MDB(3,fSTAT) MLog("ALERT:    cannot locate user record for job '%s'\n",
        J->Name);
      }

    /* locate/adjust group stats */

    G = J->Credential.G;

    if (G != NULL)
      {
      MDB(7,fSTAT) MLog("INFO:     updating statistics for group %s (GID %ld)\n",
        G->Name,
        G->OID);

      G->MTime = MSched.Time;

      tmpS = &G->Stat;

      S[stattotal++] = tmpS;

      if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
        {
        S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
        }
      }
    else
      {
      MDB(3,fSTAT) MLog("ALERT:    cannot locate group record for job '%s'\n",
        J->Name);
      }

    /* locate/adjust account stats */

    A = J->Credential.A;

    if (A != NULL)
      {
      MDB(7,fSTAT) MLog("INFO:     updating statistics for account %s\n",
        A->Name);

      A->MTime          = MSched.Time;

      tmpS = &A->Stat;

      S[stattotal++] = tmpS;

      if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
        {
        S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
        }
      }
    else
      {
      MDB(7,fSTAT) MLog("INFO:     job '%s' has no account assigned\n",
        J->Name);
      }

    /* locate/adjust QOS stats */

    if (J->Credential.Q != NULL)
      {
      Q = J->Credential.Q;

      MDB(7,fSTAT) MLog("INFO:     updating statistics for QOS %s\n",
        Q->Name);
 
      Q->MTime = MSched.Time;
 
      tmpS = &Q->Stat;

      S[stattotal++] = tmpS;

      if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
        {
        S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
        }
      }
    else
      {
      MDB(7,fSTAT) MLog("INFO:     job '%s' has no QOS assigned\n",
        J->Name);
      }

    /* locate/adjust Class stats */

    C = J->Credential.C;  
 
    if (C != NULL)
      {
      MDB(7,fSTAT) MLog("INFO:     updating statistics for class %s\n",
        C->Name);
 
      C->MTime = MSched.Time;
 
      tmpS = &C->Stat;

      S[stattotal++] = tmpS;

      if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
        {
        S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
        }
      }
    else
      {
      MDB(7,fSTAT) MLog("INFO:     job '%s' has no class assigned\n",
        J->Name);
      }

    /* update all statistics */

    for (statindex = 0;statindex < stattotal;statindex++)
      {
      if (S[statindex] != NULL)
        {
        S[statindex]->AvgXFactor += xfactor;
        S[statindex]->AvgQTime   += qtime;
        S[statindex]->IICount++;
        }
      }  /* END for (statindex) */
    }    /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MStatUpdateIdleJobUsage() */





#define MMAX_JOBSTATTARGET 32
#define MMAX_TEMPLATEFAILURE 10

/**
 * Update job, matrix, credential, and template statistics for completed job.
 *
 * @see MJobProcessCompleted() - parent
 * @see MStatUpdateRejectedJobUsage() - peer
 *
 * @param J (I)
 * @param EMode (I)   (msmProfile or msmNONE)
 * @param SFlagBM (I) (bitmap of enum MProfModeEnum)
 */

int MStatUpdateCompletedJobUsage(

  mjob_t             *J,
  enum MSchedModeEnum EMode,
  mbitmap_t          *SFlagBM)

  {
  mulong run;      /* actual measured run-time */ 
  mulong effrun;   /* bounded by WCLimit */
  mulong request;
  mulong queuetime;

  mulong psrun;
  mulong psrequest;
  long   psremaining;
  double accuracy;
  double xfactor;

  long  rval;
  int   timeindex;
  int   procindex;
  int   accindex;

  int   statindex;
  int   stattotal;
 
  int   rqindex;
  int   nindex;

  mbool_t QOSMet;

  int   PC;  /* processor count */
  int   NC;  /* node count */

  double  PE;

  must_t *S[MMAX_JOBSTATTARGET];

  mreq_t  *RQ;

  must_t *GS = &MPar[0].S;
  must_t *tmpS;

  const char *FName = "MStatUpdateCompletedJobUsage";

  MDB(5,fSTAT) MLog("%s(%s,%u)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    EMode);

  if (J == NULL)
    {
    return(FAILURE);
    }

  PC = J->TotalProcCount;

  MJobGetPE(J,&MPar[0],&PE);

  RQ = J->Req[0]; /* FIXME */

  /* if job failed to run, return */

  if (((J->SubmitTime == 0) && (MSched.Time > 100000)) || 
       (J->StartTime == 0) || 
       (J->CompletionTime == 0))
    {
    MDB(2,fSTAT) MLog("ALERT:    job %s did not run. (Q: %ld S: %ld: C: %ld)\n",
      J->Name,
      J->SubmitTime,
      J->StartTime,
      J->CompletionTime);

    return(FAILURE);
    }

  /* determine base statistics */

  run = (J->CompletionTime > J->StartTime) ? 
    J->CompletionTime - J->StartTime : 1;

  if (run < J->WCLimit)
    {
    effrun = run;
    }
  else
    {
    effrun = J->WCLimit;
    }

  request   = J->WCLimit;

  queuetime = (J->StartTime > J->SystemQueueTime) ?
    J->StartTime - J->SystemQueueTime : 0;

  NC = 0;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    for (nindex = 0;MNLGetNodeAtIndex(&J->Req[rqindex]->NodeList,nindex,NULL) == SUCCESS;nindex++)
      {
      if (MNLGetTCAtIndex(&J->Req[rqindex]->NodeList,nindex) == 0)
        break;

      NC++;
      }
    }

  if (J->StartTime < J->SystemQueueTime)
    {
    MDB(0,fSTAT) MLog("ALERT:    job '%18s' has invalid system queue time (SQ: %ld > ST: %ld)\n",
      J->Name,
      J->SystemQueueTime,
      J->StartTime);
    }
  
  psremaining = PC * (J->StartTime + J->WCLimit - MSched.Time);

  if ((long)psremaining < 0)
    psremaining = 0;

  /* value 0.0 - 1.0 */

  accuracy = (request == 0) ? 0.0 : (double)effrun / request;

  psrun     = run     * PC;
  psrequest = request * PC;

  /* determine XFactor */

  xfactor = bmisset(SFlagBM,mTrueXFactor) ?
    (double)(queuetime + request) / request :
    (double)(queuetime + effrun) / request;

  QOSMet = TRUE;

  if (J->Credential.Q != NULL)
    {
    mqos_t *Q;

    Q = J->Credential.Q;

    if (((Q->XFTarget > 0.0) && (xfactor > (double)Q->XFTarget)) ||
        ((Q->QTTarget > 0) && ((long)queuetime > Q->QTTarget)))
      {
      QOSMet = FALSE;
      }
    }    /* END if (J->Cred.Q != NULL) */
    
  MDB(3,fSTAT) MLog("INFO:     job '%18s' completed.  QueueTime: %6ld  RunTime: %6ld  Accuracy: %5.2f  XFactor: %5.2f\n",
    J->Name,
    queuetime,
    run,
    accuracy * 100,
    xfactor);

  MDB(4,fSTAT) MLog("INFO:     start: %8ld  complete: %8ld  SystemQueueTime: %8ld\n",
    (unsigned long)J->StartTime,
    (unsigned long)J->CompletionTime,
    (unsigned long)J->SystemQueueTime);

  MDB(3,fSTAT) MLog("INFO:     overall statistics.  Accuracy: %5.2f  XFactor: %5.2f\n",
    (GS->JC > 0) ? (GS->JobAcc / GS->JC) : 0.0,
    (GS->JC > 0) ? (GS->XFactor / GS->JC) : 0.0);

  if (!MNLIsEmpty(&J->NodeList))
    {
    mnode_t *N;

    MNLGetNodeAtIndex(&J->NodeList,0,&N);
    N->Stat.SuccessiveJobFailures = 0;
    }

  /* determine statistics grid location */

  if (bmisset(SFlagBM,mUseRunTime))
    {
    rval = (J->CompletionTime > J->StartTime) ?
      J->CompletionTime - J->StartTime : 0;
    }
  else
    {
    rval = J->WCLimit;
    }

  rval = MIN(rval,(long)MStat.P.MaxTime);

  for (timeindex = 0;timeindex < MMAX_GRIDTIMECOUNT;timeindex++)
    {
    if (rval <= (long)MStat.P.TimeStep[timeindex])
      break;
    }  /* END for (timeindex) */

  timeindex = MIN(timeindex,(int)MStat.P.TimeStepCount);

  for (procindex = 0;procindex < MMAX_GRIDSIZECOUNT;procindex++)
    {
    if ((long)MStat.P.ProcStep[procindex] >= MIN(PC,(int)MStat.P.MaxProc))
      break;
    }  /* END for (procindex) */

  for (accindex  = 0;accindex < MMAX_ACCURACY;accindex++)
    {
    if (MIN((int)(100 * accuracy),100) <= (int)MStat.P.AccuracyStep[accindex])
      break;
    }  /* END for (accindex) */

  if (accindex == MMAX_ACCURACY) accindex--; /* use last index if not found in for loop above */

  MDB(4,fSTAT) MLog("INFO:     updating statistics for Grid[time: %d][proc: %d]\n",
    timeindex,
    procindex);

  /* determine statistics to update */

  stattotal = 0;

  /* update long-term stats */

  S[stattotal++] = &MStat.SMatrix[0].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[0].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[0].CTotal[timeindex];

  /* update rolling stats */

  S[stattotal++] = &MStat.SMatrix[1].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[1].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[1].CTotal[timeindex];

  /* update global partition */

  S[stattotal++] = GS;

  /* update time profiles */

  if ((GS->IStat != NULL) && (GS->IStatCount > 0))
    S[stattotal++] = GS->IStat[GS->IStatCount - 1];

  if (RQ->PtIndex != 0)
    {
    tmpS = &MPar[RQ->PtIndex].S;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  /* locate/adjust user stats */

  if (J->Credential.U != NULL)
    {
    MDB(7,fSTAT) MLog("INFO:     updating statistics for UID %ld (user: %s)\n",
      J->Credential.U->OID,
      J->Credential.U->Name);

    tmpS = &J->Credential.U->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  /* locate/adjust group stats */

  if (J->Credential.G != NULL)
    {
    MDB(7,fSTAT) MLog("INFO:     updating statistics for GID %ld (group: %s)\n",
      J->Credential.G->OID,
      J->Credential.G->Name);

    tmpS = &J->Credential.G->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  /* locate/adjust account stats */

  if (J->Credential.A != NULL)
    {
    MDB(7,fSTAT) MLog("INFO:     updating statistics for account %s\n",
      J->Credential.A->Name);

    tmpS = &J->Credential.A->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  if (J->Credential.Q != NULL)
    {
    MDB(7,fSTAT) MLog("INFO:     updating statistics for QOS %s\n",
      J->Credential.Q->Name);

    tmpS = &J->Credential.Q->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  if (J->Credential.C != NULL)
    {
    MDB(7,fSTAT) MLog("INFO:     updating statistics for class %s\n",
      J->Credential.C->Name);

    tmpS = &J->Credential.C->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  if (J->TStats != NULL)
    {
    mln_t *JS;

    for (JS = J->TStats;JS != NULL;JS = JS->Next)
      {
      mjob_t *JT;

      mtjobstat_t *JSD;  /* job stat data */

      if (JS->Ptr != NULL)
        JT = (mjob_t *)JS->Ptr;
      else if (MTJobFind(JS->Name,&JT) == FAILURE)
        continue;

      if (JT->ExtensionData == NULL)
        continue;

      JSD = (mtjobstat_t *)JT->ExtensionData;

      tmpS = &JSD->S;

      S[stattotal++] = tmpS;

      if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
        {
        S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
        }
      }        /* END for (JS) */
    }          /* END if (J->TStats != NULL) */

  /* update all statistics */

  for (statindex = 0;statindex < stattotal;statindex++)
    {
    S[statindex]->JC++;
    S[statindex]->JAllocNC    += PC;
    S[statindex]->SuccessJC++;

    S[statindex]->TotalQTS  += queuetime;

    S[statindex]->TotalRequestTime += (double)request;
    S[statindex]->TotalRunTime     += (double)run;
    S[statindex]->PSRequest        += psrequest;
    S[statindex]->PSRun            += psrun;
    S[statindex]->PSRunSuccessful  += psrun;

    S[statindex]->JobAcc    += accuracy;
    S[statindex]->NJobAcc   += accuracy * psrun;

    S[statindex]->Accuracy[accindex]++;

    S[statindex]->XFactor   += xfactor;
    S[statindex]->NXFactor  += xfactor * PC;
    S[statindex]->PSXFactor += xfactor * psrun;

    S[statindex]->Bypass    += J->BypassCount;

    S[statindex]->QOSCredits += J->Cost;

    if (QOSMet == TRUE)
      {
      S[statindex]->QOSSatJC++;
      }

    if (EMode == msmProfile)
      {
      S[statindex]->PSDedicated += J->PSDedicated;
      S[statindex]->PSUtilized  += J->PSUtilized;

      S[statindex]->MSDedicated += J->MSDedicated;
      S[statindex]->MSUtilized  += J->MSUtilized;
      }

    if (bmisset(&J->Flags,mjfBackfill))
      {
      S[statindex]->BFCount++;
      S[statindex]->BFPSRun += psrun;
      }

    S[statindex]->MaxQTS     = MAX(S[statindex]->MaxQTS,queuetime);
    S[statindex]->MaxXFactor = MAX(S[statindex]->MaxXFactor,xfactor);
    S[statindex]->MaxBypass  = MAX(S[statindex]->MaxBypass,J->BypassCount);
    }  /* END for (statindex) */

  MDB(2,fSTAT) MLog("INFO:     job '%s' completed  X: %f  T: %ld  PS: %ld  A: %f (RM: %s/%s)\n",
    J->Name,
    xfactor,
    run,
    psrun,
    accuracy,
    (J->SubmitRM != NULL) ? J->SubmitRM->Name : "",
    (J->DestinationRM != NULL) ? J->DestinationRM->Name : "");

  /* update consumption statistics */

  MStat.SuccessfulPH += (double)psrun / 36.0;

  if ((J->State == mjsCompleted) || ((long)run >= (long)J->WCLimit))
    {
    /* job is successfull if RM says it was or it ran to its full WCLimit */

    MStat.SuccessfulJobsCompleted++;
    }

  return(SUCCESS);
  }  /* END MStatUpdateCompletedJobUsage() */





/**
 * Update job, matrix, credential, and template statistics for rejected/failed job.
 *
 * @see MJobProcessRemoved()
 * @see MStatUpdateCompletedJobUsage()
 *
 * @param J (I)
 * @param SFlagBM (I)
 */

int MStatUpdateRejectedJobUsage(

  mjob_t    *J,
  mbitmap_t *SFlagBM)

  {
  int   timeindex;
  int   procindex;

  int   stattotal;
  int   statindex;

  int   rval;
  int   PC;

  mreq_t *RQ;

  must_t  *S[MMAX_JOBSTATTARGET];

  const char *FName = "MStatUpdateRejectedJobUsage";

  MDB(5,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  MDB(1,fSTAT) MLog("INFO:     job '%s' rejected\n",
    J->Name);

  RQ = J->Req[0];  /* FIXME */

  /* add statistics to grid */

  if (bmisset(SFlagBM,mUseRunTime))
    {
    rval = (J->CompletionTime > J->StartTime) ?
      J->CompletionTime - J->StartTime : 0;
    }
  else
    {
    rval = J->WCLimit;
    }

  rval = MIN(rval,(int)MStat.P.MaxTime);

  PC = J->TotalProcCount;

  for (timeindex = 0;rval > (int)MStat.P.TimeStep[timeindex];timeindex++);
  for (procindex = 0;MIN(PC,(long)MStat.P.MaxProc) > (int)MStat.P.ProcStep[procindex];procindex++);

  timeindex = MIN(timeindex,(long)MStat.P.TimeStepCount);

  /* determine statistics to update */

  stattotal = 0;

  /* update long-term stats */

  S[stattotal++] = &MStat.SMatrix[0].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[0].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[0].CTotal[timeindex];

  /* update rolling stats */

  S[stattotal++] = &MStat.SMatrix[1].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[1].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[1].CTotal[timeindex];

  S[stattotal++] = &MPar[0].S;

  if ((RQ != NULL) && (RQ->PtIndex != 0))
    S[stattotal++] = &MPar[RQ->PtIndex].S;

  /* locate cred stats */

  if (J->Credential.U != NULL)
    {
    S[stattotal++] = &J->Credential.U->Stat;
    }

  if (J->Credential.G != NULL)
    {
    S[stattotal++] = &J->Credential.G->Stat;
    }

  if (J->Credential.A != NULL)
    {
    S[stattotal++] = &J->Credential.A->Stat;
    }

  if (J->Credential.Q != NULL)
    {
    S[stattotal++] = &J->Credential.Q->Stat;
    }

  /* update node failure rates for job's master node */

  if (!MNLIsEmpty(&J->NodeList))
    {
    mnode_t *N;

    MNLGetNodeAtIndex(&J->NodeList,0,&N);

    N->Stat.SuccessiveJobFailures++;

    N->Stat.FailureJC++;

    if (N->Stat.IStatCount > 0)
      N->Stat.IStat[N->Stat.IStatCount - 1]->StartJC++;
    }

  /* locate/adjust user/group/account stats */

  MDB(7,fSTAT) MLog("INFO:     updating statistics (user %s  group %s  account %s)\n",
    J->Credential.U->Name,
    J->Credential.G->Name,
    (J->Credential.A != NULL) ? J->Credential.A->Name : "NONE");

  /* update all statistics */

  for (statindex = 0;statindex < stattotal;statindex++)
    {
    S[statindex]->RejectJC++;
    }  /* END for (statindex) */

  return(SUCCESS);
  }  /* END MStatUpdateRejectedJobUsage() */




/**
 * Update stats for submitted jobs.
 * 
 * NOTE:  update S->SubmitJC, and S->SubmitPHRequest
 *
 * @see MRMJobPostLoad() - parent
 *
 * @param J (I) [modified]
 */

int MStatUpdateSubmitJobUsage(

  mjob_t  *J)

  {
  must_t  *S[MMAX_STATTARGET];

  must_t  *tmpS;

  int procindex;
  int timeindex;

  int stattotal;
  int statindex;

  int psrequest;
  
  int PC;
  mulong tmpUL;

  const char *FName = "MStatUpdateSubmitJobUsage";

  MDB(5,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(SUCCESS);
    }

  PC = J->TotalProcCount;

  psrequest = J->WCLimit * PC;
 
  tmpUL = (mulong)MIN((long)J->WCLimit,(long)MStat.P.MaxTime);
 
  for (timeindex = 0;MStat.P.TimeStep[timeindex] < tmpUL;timeindex++);

  tmpUL = (mulong)MIN(PC,(long)MStat.P.MaxProc);

  for (procindex = 0;MStat.P.ProcStep[procindex] < tmpUL;procindex++);

  MDB(7,fSTAT) MLog("INFO:     updating submit statistics for Grid[time: %d][proc: %d]\n",
    timeindex,
    procindex);

  /* determine statistics to update */

  stattotal = 0;

  /* update long term stats */

  S[stattotal++] = &MStat.SMatrix[0].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[0].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[0].CTotal[timeindex];

  /* update rolling stats */

  S[stattotal++] = &MStat.SMatrix[1].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[1].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[1].CTotal[timeindex];

  S[stattotal++] = &MPar[0].S;

  tmpS = &MPar[0].S;

  if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
    {
    S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  /* adjust user/group/account stats */

  MDB(7,fSTAT) MLog("INFO:     updating statistics (user %s  group %s  account %s)\n",
    J->Credential.U->Name,
    J->Credential.G->Name,
    (J->Credential.A != NULL) ? J->Credential.A->Name : "NONE");

  /* add all credential stats */

  if (J->Credential.U != NULL)
    {
    tmpS = &J->Credential.U->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.G != NULL)
    { 
    tmpS = &J->Credential.G->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.A != NULL)
    {
    tmpS = &J->Credential.A->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.Q != NULL)
    {
    tmpS = &J->Credential.Q->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.C != NULL)
    {
    tmpS = &J->Credential.C->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  /* update all statistics */

  for (statindex = 0;statindex < stattotal;statindex++)
    {
    S[statindex]->SubmitJC ++;
    S[statindex]->SubmitPHRequest += psrequest / MCONST_HOURLEN;
    }  /* END for (statindex) */

  return(SUCCESS);
  }  /* END MStatUpdateSubmitJobUsage() */





/**
 * Update stats for started jobs.
 *
 * NOTE:  update S->JobsStarted, S->StartXF, and S->StartQT
 *
 * @see MJobStart()
 *
 * @param J (I) [modified]
 */

int MStatUpdateStartJobUsage(

  mjob_t  *J) /* I (modified) */

  {
  must_t   *S[MMAX_STATTARGET];  /* FIXME: no bounds checking on this array */

  must_t   *tmpS;
  mnust_t  *tmpNS;

  int procindex;
  int timeindex;

  int stattotal;
  int statindex;

  int PC;

  int rqindex;
  int nindex;

  mnode_t *N;

  double XFactor;

  mulong QueueTime;

  mulong tmpUL;

  mulong Duration;

  const char *FName = "MStatUpdateStartJobUsage";

  MDB(5,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(SUCCESS);
    }

  PC = J->TotalProcCount;

  if (J->SubmitTime > MSched.Time)
    QueueTime = 0;
  else
    QueueTime = MSched.Time - J->SubmitTime;

  Duration = MAX(1,J->WCLimit);

  XFactor = (QueueTime + Duration) / Duration;

  tmpUL = (mulong)MIN((long)Duration,(long)MStat.P.MaxTime);

  for (timeindex = 0;MStat.P.TimeStep[timeindex] < tmpUL;timeindex++);

  tmpUL = (mulong)MIN(PC,(long)MStat.P.MaxProc);

  for (procindex = 0;MStat.P.ProcStep[procindex] < tmpUL;procindex++);

  MDB(7,fSTAT) MLog("INFO:     updating submit statistics for Grid[time: %d][proc: %d]\n",
    timeindex,
    procindex);

  /* determine statistics to update */

  stattotal = 0;

  /* update long term stats */

  S[stattotal++] = &MStat.SMatrix[0].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[0].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[0].CTotal[timeindex];

  /* update rolling stats */

  S[stattotal++] = &MStat.SMatrix[1].Grid[timeindex][procindex];
  S[stattotal++] = &MStat.SMatrix[1].RTotal[procindex];
  S[stattotal++] = &MStat.SMatrix[1].CTotal[timeindex];

  S[stattotal++] = &MPar[0].S;

  tmpS = &MPar[0].S;

  if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
    {
    S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
    }

  /* adjust user/group/account stats */

  MDB(7,fSTAT) MLog("INFO:     updating statistics (user %s  group %s  account %s)\n",
    J->Credential.U->Name,
    J->Credential.G->Name,
    (J->Credential.A != NULL) ? J->Credential.A->Name : "NONE");

  /* add all credential stats */

  if (J->Credential.U != NULL)
    {
    tmpS = &J->Credential.U->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.G != NULL)
    {
    tmpS = &J->Credential.G->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.A != NULL)
    {
    tmpS = &J->Credential.A->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.Q != NULL)
    {
    tmpS = &J->Credential.Q->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  if (J->Credential.C != NULL)
    {
    tmpS = &J->Credential.C->Stat;

    S[stattotal++] = tmpS;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      {
      S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
      }
    }

  MDB(7,fSTAT) MLog("INFO:     updating total statistics (%d)\n",
    stattotal);

  /* update all statistics */

  for (statindex = 0;statindex < stattotal;statindex++)
    {
    S[statindex]->StartJC ++;
    S[statindex]->StartPC += PC;
    S[statindex]->StartXF += XFactor;
    S[statindex]->StartQT += QueueTime;
    }  /* END for (statindex) */

  MDB(7,fSTAT) MLog("INFO:     updating node statistics\n");

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    if (MNLIsEmpty(&J->Req[rqindex]->NodeList))
      break;

    for (nindex = 0;MNLGetNodeAtIndex(&J->Req[rqindex]->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      N->Stat.Queuetime += QueueTime;
      N->Stat.XFactor   += XFactor;
      N->Stat.StartJC   ++;

      if (N->Stat.IStat == NULL)
        continue;

      /* We hit a condition on the Slave in a grid environment where
       * the IStatCount was 0 causing a SegFault below. Further investigation
       * needs to be done on what a IStatCount of 0 indicates, but we should not subtract 1 
       * from 0 and use it as an array index.
       */

      if (N->Stat.IStatCount == 0)
        continue;

      tmpNS = N->Stat.IStat[N->Stat.IStatCount - 1];

      if (tmpNS == NULL)
        continue;

      tmpNS->Queuetime += QueueTime;
      tmpNS->XFactor   += XFactor;
      tmpNS->StartJC++;
      } /* END for (nindex) */
    }   /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MStatUpdateStartJobUsage() */




/**
 * Update job backfill usage statistics.
 *
 * NOTE:  called by each backfill routine to update BypassCount for all jobs of higher priority.
 *
 * @param J (I)
 */

int MStatUpdateBFUsage(
 
  mjob_t *J)  /* I */
 
  {
  int jindex;

  const char *FName = "MStatUpdateBFUsage";
 
  MDB(7,fSCHED) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }
 
  for (jindex = 0;MFQ[jindex] != NULL;jindex++)
    {
    if (J == MFQ[jindex])
      break;
 
    if ((MFQ[jindex] != NULL) &&
        (MFQ[jindex]->State == mjsIdle))
      {
      MFQ[jindex]->BypassCount++;
      }
    }  /* END for (jindex) */
 
  return(SUCCESS);
  }  /* END MStatUpdateBFUsage() */



/**
 * Update backlog statistics for all partitions and credentials
 *
 * @param Q (I) [terminated w/-1]
 */

int MStatUpdateBacklog(

  mjob_t **Q)

  {
  must_t   *tmpS;
  mcredl_t *tmpL;

  mcredl_t *GL;
  mpar_t   *GP;

  void     *OE;
  void     *O;
  void     *OP;

  char     *OName;

  int      oindex;
  int      jindex;

  mhashiter_t HTI;

  enum MXMLOTypeEnum OList[] = {
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoNONE };

  GP = &MPar[0];
  GL = &GP->L;

  if (Q != NULL)
    {
    int pindex;
    int PC;

    mjob_t *J;

    for (jindex = 0;Q[jindex] != NULL;jindex++)
      {
      J = Q[jindex];

      /* NOTE:  only MUIQ passed in as job queue */

      bmset(&J->IFlags,mjifIsEligible);

      PC = J->TotalProcCount;

      for (pindex = 1;pindex < MSched.M[mxoPar];pindex++)
        { 
        /* only update backlog if job is eligible to run in partition */

        if (bmisset(&J->EligiblePAL,pindex))
          {
          MPar[pindex].IterationBacklogPS += J->SpecWCLimit[0] * PC;
          MPar[pindex].IterationIdleTC    += PC;

          /* record smallest job which could run in partition */

          MPar[pindex].IterationMinJobPC = MIN(MPar[pindex].IterationMinJobPC,PC);
          }
        }    /* END for (pindex) */

      MStatUpdateIdleJobUsage(Q[jindex]);
      }  /* END for (jindex) */
    }    /* END if (Q != NULL) */

  MNodeUpdatePowerUsage(NULL);

  MStat.QueuedPH = GL->IdlePolicy->Usage[mptMaxPS][0] / MCONST_HOURLEN;
  MStat.QueuedJC = GL->IdlePolicy->Usage[mptMaxJob][0];

  MStat.TQueuedPH += GL->IdlePolicy->Usage[mptMaxPS][0] / MCONST_HOURLEN;
  MStat.TQueuedJC += GL->IdlePolicy->Usage[mptMaxJob][0];

  MStat.TActivePH += GL->ActivePolicy.Usage[mptMaxPS][0] / MCONST_HOURLEN;

  if (MPar[0].S.IStatCount > 0)
    {
    MOUpdateBacklog(MPar[0].S.IStat[MPar[0].S.IStatCount - 1],&MPar[0].L);
    }
 
  /* step through all creds */

  for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
    {
    /* update base and profile stats */

    MOINITLOOP(&OP,OList[oindex],&OE,&HTI);

    while ((O = MOGetNextObject(&OP,OList[oindex],OE,&HTI,&OName)) != NULL)
      {
      if (MOGetComponent(O,OList[oindex],(void **)&tmpS,mxoxStats) == FAILURE)
        continue;

      if (MOGetComponent(O,OList[oindex],(void **)&tmpL,mxoxLimits) == FAILURE)
        continue;

      MOUpdateBacklog(tmpS,tmpL);

      if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
        MOUpdateBacklog(tmpS->IStat[tmpS->IStatCount - 1],tmpL);
      }  /* END while (O = MOPGetNextObject() != NULL) */
    }    /* END for (oindex) */

  /* step through all job templates */

  for (jindex = 0;MUArrayListGet(&MTJob,jindex) != NULL;jindex++)
    {
    mjob_t *TJ;

    TJ = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));

    if (TJ == NULL)
      break;

    if (TJ->ExtensionData == NULL)
      continue;

    tmpS = &((mtjobstat_t *)TJ->ExtensionData)->S;
    tmpL = &((mtjobstat_t *)TJ->ExtensionData)->L;

    if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
      MOUpdateBacklog(tmpS->IStat[tmpS->IStatCount - 1],tmpL);

    }   /* END for (jindex) */

  /* only calculate backlog for global partition */

  /* NOTE:  backlog info from peer partitions will be imported */
 
  GP->BacklogPS = GP->L.IdlePolicy->Usage[mptMaxPS][0] + GP->L.ActivePolicy.Usage[mptMaxPS][0];

  if (GP->UpRes.Procs > 0)
    {
    double WCAccuracy;
    double SchedEffic;

    WCAccuracy = MPar[0].S.JobAcc / MAX(1,MPar[0].S.JC);

    if (WCAccuracy <= 0.0)
      WCAccuracy = 1.0;

    SchedEffic = MStat.TotalPHDed / MAX(1,MStat.TotalPHAvailable);

    if (SchedEffic <= 0.0)
      SchedEffic = 1.0;

    GP->BacklogDuration = (long)(GP->BacklogPS * WCAccuracy /
                          SchedEffic /
                          GP->UpRes.Procs);
    }
 
  return(SUCCESS);
  }  /* END MStatUpdateBacklog() */ 



/* END MStatUpdate.c */
