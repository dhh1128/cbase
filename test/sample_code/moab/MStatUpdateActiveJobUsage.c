/* HEADER */

      
/**
 * @file MStatUpdateActiveJobUsage.c
 *
 * Contains: MStatUpdateActiveJobUsage
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Update active usage for stat and fairshare objects associated with job.
 *
 * @see MQueueAddAJob() - parent
 *
 * Detailed description:
 *
 * update fairshare, cred, par, and job stat usage for active (running/starting) jobs
 * adjust credential statistics (iteration based delta)
 * fairshare usage (iteration based delta)
 * policy based usage (full recalculation)
 * policy usage cleared in MQueueSelectAllJobs()
 *
 * @param J (I)
 */

int MStatUpdateActiveJobUsage(

  mjob_t *J)  /* I */

  {
  int      nindex;
  int      rqindex;

  double   pesdedicated;

  double   psdedicated;
  double   psutilized;

  double   msdedicated;
  double   msutilized;

  double   fsusage;
  double   speed;
  int      procspeed;

  int      timeindex;
  int      procindex;

  int      statindex;
  int      stattotal;

  double   Load = 0;
  double   Mem  = 0;

  int      JPC;     /* job proc count */
  int      NC = 0;  /* node count (not necessarily job exclusive) */

  mgcred_t *U;
  mgcred_t *G;
  mgcred_t *A;
  mqos_t   *Q;
  mclass_t *C;

  int      TC;
  mulong   tmpUL;

  double   interval;
  double   PE;

  mnode_t *N;

  must_t  *S[MMAX_STATTARGET];
  mfs_t   *F[MMAX_STATTARGET];

  must_t  *GS;
  must_t  *tmpS;

  mreq_t  *RQ;
  mpar_t  *P;

  const char *FName = "MStatUpdateActiveJobUsage";

  MDB(4,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  JPC = J->TotalProcCount;

  if (JPC == 0)
    {
    /* 6-9-09 BOC RT 5052: though JPC == 0, the job can be a size=0 job (cray) and the MAXJOB 
     * policy stills need to be updated so call MPolicyAdjustUsage to update job usages */

    MPolicyAdjustUsage(J,NULL,NULL,mlActive,NULL,mxoALL,1,NULL,NULL);

    MDB(3,fSTAT) MLog("INFO:     no tasks associated with job '%s' (no statistics available)\n",
      J->Name);

    return(FAILURE);
    }

  MJobGetPE(J,&MPar[0],&PE);  

  if (!bmisset(&J->IFlags,mjifIsTemplate))
    {
    MStat.ActiveJobs++;     
    }

  interval = MIN(
    (double)MSched.Interval / 100.0,  /* divide by 100 to convert to seconds */
    (double)MSched.Time - J->StartTime);

  if ((MSched.Mode == msmSim) && (MSched.TimeRatio > 0.0))
    {
    /* NOTE:  must correct for time ratio impact */

    interval *= MSched.TimeRatio;
    }

  if (interval <= 0)
    {
    /* no consumed statistics, update usage and return */

    MPolicyAdjustUsage(J,NULL,NULL,mlActive,NULL,mxoALL,1,NULL,NULL);

    return(SUCCESS);
    }

  pesdedicated = PE * interval;

  GS = &MPar[0].S;

  speed     = 0.0;
  procspeed = 0;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    { 
    RQ = J->Req[rqindex];

    P  = &MPar[RQ->PtIndex];

    psdedicated = 0.0;
    psutilized  = 0.0;

    msdedicated = 0.0;
    msutilized  = 0.0;

    /* NOTE:  use of JPC within per req values is incorrect for multi-req */

    if ((J->StartTime != MSched.Time) && (J->CTime != (long)MSched.Time))
      { 
      psdedicated = interval * JPC; 

      NC      = 0;
      Load    = 0;
      Mem     = 0;

      for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        TC = MNLGetTCAtIndex(&RQ->NodeList,nindex);

        if (TC == 0)
          break;
  
        NC++;

        msdedicated += (double)(interval * TC * RQ->DRes.Mem);

        speed     += (N->Speed > 0) ? N->Speed * TC : TC;
        procspeed += (N->ProcSpeed > 0) ? N->ProcSpeed * TC : TC;

        if ((RQ->MURes != NULL) && (RQ->URes != NULL) && (RQ->URes->Procs > 0))
          {
          psutilized      += interval * TC * RQ->URes->Procs / 100.0;
          RQ->MURes->Procs  = MAX(RQ->MURes->Procs,RQ->URes->Procs);
          }
#ifdef MNOT
        /* 5/22/09 BOC RT3982
         * some rm's, slurm for instance, don't report utilized memory to moab.
         * Moab shouldn't guess and lie to the user that processors are being utilized. */

        else if (N->CRes.Procs > 1)
          {
          /* cannot properly determine efficiency with available information */
          /* make job 100% efficient                                         */

          if (RQ->DRes.Procs >= 0)
            psutilized += (interval * TC * RQ->DRes.Procs);
          else
            psutilized += (interval * TC);
          }
#endif
        else
          { 
          if (N->Load >= 1.0)
            {
            psutilized += (interval * 1.0);
            }
          else
            {
            psutilized += (interval * (double)N->Load);
            }
          }
        }     /* END for (nindex) */

      if ((RQ->URes != NULL) && (RQ->MURes != NULL) && (RQ->URes->Mem > 0))
        {
        msutilized = interval * RQ->URes->Mem;
        RQ->MURes->Mem = MAX(RQ->MURes->Mem,RQ->URes->Mem);
        }
#ifdef MNOT
        /* 5/22/09 BOC RT3982: utilized isn't dedicated */
      else
        {
        msutilized = msdedicated; /* FIXME */
        }
#endif

      if ((RQ->URes != NULL) && (RQ->MURes != NULL) && (RQ->URes->Swap > 0))
        {
        RQ->MURes->Swap = MAX(RQ->MURes->Swap,RQ->URes->Swap);
        }

      if ((RQ->URes != NULL) && (RQ->MURes != NULL) && (RQ->URes->Disk > 0))
        {
        RQ->MURes->Disk = MAX(RQ->MURes->Disk,RQ->URes->Disk);
        }

      if (psdedicated == 0.0)
        {
        char TString[MMAX_LINE];

        MULToTString(MSched.Time - J->StartTime,TString);

        MDB(1,fSTAT) MLog("ALERT:    job %s active for %s has no dedicated time on %d nodes\n",
          J->Name,
          TString,
          nindex);
        }
      else
        {
        MDB(6,fSTAT) MLog("INFO:     job '%18s'  nodes: %3d  PSDedicated: %f  PSUtilized: %f  (efficiency: %5.2f)\n",
          J->Name,
          nindex,
          psdedicated,
          psutilized,
          psutilized / psdedicated);
        } /* END else (psdedicated == 0.0) */
      }   /* END if (J->StartTime != MSched.Time) */

    /* update job specific statistics */

    J->PSUtilized += psutilized;

    if ((J->PSDedicated < 0) || (psdedicated < 0))
      {   
      MLog("ALERT:    JPSD: %f  PSD: %f\n",
        J->PSDedicated,
        psdedicated);
      }

    J->PSDedicated += psdedicated;

    J->MSUtilized  += msutilized;

    J->MSDedicated += (double)msdedicated;

    /* determine stat grid location */

    tmpUL = MIN(J->WCLimit,(mulong)MStat.P.MaxTime);

    for (timeindex = 0;MStat.P.TimeStep[timeindex] < tmpUL;timeindex++);

    tmpUL = MIN((mulong)RQ->TaskCount * RQ->DRes.Procs,MStat.P.MaxProc);

    for (procindex = 0;MStat.P.ProcStep[procindex] < tmpUL;procindex++);

    timeindex = MIN(timeindex,(int)MStat.P.TimeStepCount);

    MDB(7,fSTAT) MLog("INFO:     updating statistics for Grid[time: %d][proc: %d]\n",
      timeindex,
      procindex);

    /* determine statistics to update */

    memset(S,0,sizeof(S));
    memset(F,0,sizeof(F));

    stattotal = 0;

    /* update long-term stats */

    S[stattotal++] = &MStat.SMatrix[0].Grid[timeindex][procindex];
    S[stattotal++] = &MStat.SMatrix[0].RTotal[procindex];
    S[stattotal++] = &MStat.SMatrix[0].CTotal[timeindex];

    /* update rolling stats */

    S[stattotal++] = &MStat.SMatrix[1].Grid[timeindex][procindex];
    S[stattotal++] = &MStat.SMatrix[1].RTotal[procindex];
    S[stattotal++] = &MStat.SMatrix[1].CTotal[timeindex];

    if (MPar[0].FSC.TargetIsAbsolute == FALSE)
      {
      /* NOTE:  global usage for absolute targets specified in MParUpdate() */

      F[stattotal] = &MPar[0].F;
      }

    S[stattotal++] = &MPar[0].S;

    if ((GS->IStat != NULL) && (GS->IStatCount > 0))
      {
      S[stattotal++] = GS->IStat[GS->IStatCount - 1];
      }

    if (P->Index != 0)
      {
      F[stattotal]   = &P->F;
      S[stattotal++] = &P->S;
      }

    /* locate/adjust user stats */

    switch (MPar[0].FSC.FSPolicy)
      {
      case mfspDPES:

        fsusage = pesdedicated;

        break;

      case mfspDSPES:

        if (speed > 0.0)
          fsusage = pesdedicated * speed / JPC;
        else
          fsusage = pesdedicated;

        break;

      case mfspUPS:

        fsusage = psutilized;

        break;

      case mfspDPPS:

        if (procspeed > 0.0)
          fsusage = pesdedicated * procspeed / JPC;
        else
          fsusage = pesdedicated;

        break;

      case mfspDPS:
      default:

        fsusage = psdedicated;

        break;
      }  /* END switch (MPar[0].FSC.FSPolicy) */

    if ((J->Credential.Q != NULL) && (J->Credential.Q->FSScalingFactor != 0.0))
      {
      /* scale fs usage by QoS factor */

      fsusage *= J->Credential.Q->FSScalingFactor;
      }
      
    MPolicyAdjustUsage(J,NULL,NULL,mlActive,NULL,mxoALL,1,NULL,NULL);

    U = J->Credential.U;

    if ((U != NULL) && (U->L.ActivePolicy.HLimit[mptMaxJob][0] > 0))
      {
      /* TBD remove this log - put in for testing at Ford */

      MDB(7,fSTAT) MLog("INFO:     setting user %s max job usage %ld hlimit %ld in MStatUpdateActiveJobUsage\n",
        U->Name,
        U->L.ActivePolicy.Usage[mptMaxJob][0],
        U->L.ActivePolicy.HLimit[mptMaxJob][0]);
      }

    if (U != NULL)
      {
      MDB(7,fSTAT) MLog("INFO:     updating statistics for user %s (UID: %ld) \n",
        U->Name,   
        U->OID);

      U->MTime = MSched.Time;

      tmpS = &U->Stat;

      F[stattotal]   = &U->F;
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

      /* adjust FairShare record */

      tmpS = &G->Stat;

      F[stattotal]   = &G->F;     
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

      /* adjust FairShare record */

      tmpS = &A->Stat;

      F[stattotal]   = &A->F;     
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
 
      /* adjust FairShare record */

      tmpS = &Q->Stat;

      F[stattotal]   = &Q->F;    
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

    /* locate/adjust class stats */

    C = J->Credential.C;  
 
    if (C != NULL)
      {
      MDB(7,fSTAT) MLog("INFO:     updating statistics for class %s\n",
        C->Name);
 
      C->MTime = MSched.Time;
 
      /* adjust FairShare record */

      tmpS = &C->Stat;

      F[stattotal]   = &C->F;    
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

    if (J->FairShareTree != NULL)
      {
      mfst_t *TData = NULL;

      if ((J->Req[0]->PtIndex > 0) && 
          (J->FairShareTree[J->Req[0]->PtIndex] != NULL))
        {
        TData = J->FairShareTree[J->Req[0]->PtIndex]->TData;
        }
      else if (J->FairShareTree[0] != NULL)
        {
        TData = J->FairShareTree[0]->TData;
        }

      while (TData != NULL)
        {
        F[stattotal]   = TData->F;
        S[stattotal++] = TData->S;

        if (TData->Parent == NULL)
          break;

        TData = TData->Parent->TData;
        }
      }    /* END if (J->FSTree != NULL) */

    if (J->TStats != NULL)
      {
      must_t *tmpS;
      mln_t  *TStats;
      mjob_t *TJ;

      for (TStats = J->TStats;TStats != NULL;TStats = TStats->Next)
        {
        if (TStats->Ptr == NULL)
          continue;

        TJ = (mjob_t *)TStats->Ptr;

        if (TJ->ExtensionData == NULL)
          continue;
 
        F[stattotal]   = &((mtjobstat_t *)TJ->ExtensionData)->F;
        S[stattotal++] = &((mtjobstat_t *)TJ->ExtensionData)->S;

        tmpS = &((mtjobstat_t *)TJ->ExtensionData)->S;

        if (stattotal >= MMAX_STATTARGET)
          break;

        if ((tmpS->IStat != NULL) && (tmpS->IStatCount > 0))
          {
          S[stattotal++] = tmpS->IStat[tmpS->IStatCount - 1];
          }
        }  /* END for (TStats) */
      }    /* END if (J->TStats != NULL) */

    /* update all statistics */

    for (statindex = 0;statindex < stattotal;statindex++)
      {
      if ((S[statindex] != NULL) &&
          (S[statindex]->XLoad != NULL)) /* FIXME: this is a temporary
                                            workaround for a segfault */
        {
        /* NOTE:  should add 'scaled' ps factors (NYI) */

        S[statindex]->PSDedicated += psdedicated;
        S[statindex]->PSUtilized  += psutilized;

        S[statindex]->MSDedicated += msdedicated;
        S[statindex]->MSUtilized  += msutilized;
        S[statindex]->ActivePC    += JPC;
        S[statindex]->ActiveNC    += NC;
        S[statindex]->TNL         += Load;
        S[statindex]->TNM         += Mem;

        if ((RQ != NULL) && (RQ->XURes != NULL))
          {
          int gmindex;

          /* NOTE:  some gmetrics are cumulative (ie, watts), others are not
                    (ie, temp) */

          for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
            { 
            /* don't update Watts here, update watts
             * in MNodeUpdateProfileStats() */

            /* FIXME: this breaks the tracking of watts on all objects */

            /*
            if ((RQ->XURes->GMetric[gmindex] != NULL) &&
            */

            if ((RQ->XURes->GMetric[gmindex] != NULL) &&
                (RQ->XURes->GMetric[gmindex]->IntervalLoad != MDEF_GMETRIC_VALUE))
              {
              if (MGMetric.GMetricIsCummulative[gmindex] == TRUE)
                S[statindex]->XLoad[gmindex] += RQ->XURes->GMetric[gmindex]->IntervalLoad * J->Alloc.NC;
              else
                S[statindex]->XLoad[gmindex] += RQ->XURes->GMetric[gmindex]->IntervalLoad;
              }
            }  /* END for (gmindex) */
          }
        }    /* END if (S[statindex] != NULL) */

      /* NOTE:  do not track fairshare usage for template jobs,
                or when the Job's Resource Manager is not active. */

      if ((F[statindex] != NULL) &&
          (!bmisset(&J->IFlags,mjifIsTemplate)) &&
          ((J->DestinationRM == NULL) || 
           (J->DestinationRM->State == mrmsActive)))
        {
        F[statindex]->FSUsage[0] += fsusage;
        }
      }  /* END for (statindex) */
    }    /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MStatUpdateActiveJobUsage() */

/* END MStatUpdateActiveJobUsage.c */
