/* HEADER */

      
/**
 * @file MNodeGetPriority.c
 *
 * Contains: MNodeGetPriority
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * This function takes an affinity type and returns a value
 * that can be used in a node priority function.
 * @see MNodeGetPriority() - parent
 *
 * @param Affinity (I)
 *
 * @return An integer value that can be used in a node priority function.
 */

int __MNodeTranslateAffinityToPrio(

  enum MNodeAffinityTypeEnum Affinity)

  {
  int Priority = 0;

  switch (Affinity)
    {
    case mnmPositiveAffinity:
      Priority = 1;
      break;
    case mnmNeutralAffinity:
      Priority = 0;
      break;
    case mnmNegativeAffinity:
      Priority = -1;
      break;
    case mnmPreemptible:
      Priority = 5;
      break;
    default:
      Priority = 0;
      break;
    }

  return(Priority);
  }  /* END __MNodeTranslateAffinityToPrio() */


/**
 * Calculate node allocation priority for 'PRIORITY' node allocation policy.
 *
 * @see MNLSort() - parent
 *
 * @param N (I) The given node that we should calculate priority for.
 * @param J (I) [optional]
 * @param RQ (I) [optional]
 * @param SpecP (I) [optional]
 * @param RAff (I) Rsv. affinity type this request has for this given node.
 * @param JPref (I) Whether or not this node is "preferred" by this request.
 * @param Val (O) The calculated priority for this node.
 * @param StartTime (I) The time that should be used to determine if this
 *                      priority is for a node's usage now or in the future.
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MNodeGetPriority(

  mnode_t        *N,
  const mjob_t   *J,
  const mreq_t   *RQ,
  mnprio_t       *SpecP,
  int             RAff,
  mbool_t         JPref,
  double         *Val,
  long            StartTime,
  char           *Msg)

  {
  double    tmpP;
  double    tmpLP;

  double   *CW;  /* component weight */

  char     *BPtr;
  int       BSpace;

  int       NPriority;

  mbool_t OSPrio = FALSE;
  mbool_t VMPrio = FALSE;

  mnprio_t *P;

  if (Val != NULL)
    *Val = 0;

  if (Msg != NULL)
    MUSNInit(&BPtr,&BSpace,Msg,MMAX_LINE);

  if ((N == NULL) || (Val == NULL))
    {
    return(FAILURE);
    }

  if ((J != NULL) &&
      (J->Req[0]->Opsys != 0))
    {
    OSPrio = TRUE;

    if ((MJOBREQUIRESVMS(J) == TRUE) ||
        (MJOBISVMCREATEJOB(J) == TRUE)) 
      {
      VMPrio = TRUE;
      }
    }

  /* translate RAff, as otherwise a non-affinity node would be a value of 6 when it
   * should get 0! */

  RAff = __MNodeTranslateAffinityToPrio((enum MNodeAffinityTypeEnum)RAff);

  if (N->PrioritySet == FALSE)
    {
    NPriority = MSched.DefaultN.Priority;
    }
  else
    {
    NPriority = N->Priority;
    }

  /* node allocation priority function precedence:  J -> N -> C -> DefaultNode */

  if (SpecP != NULL)
    {
    P = SpecP;
    }
  else if ((J != NULL) && (J->NodePriority != NULL) && (J->NodePriority->CWIsSet == TRUE))
    {
    P = J->NodePriority;
    }
  else if ((J != NULL) && 
           (bmisset(&J->Flags,mjfRsvMap)) && 
           (MPar[0].RsvNAllocPolicy != mnalNONE) &&
           (MPar[0].RsvP != NULL) &&
           (MPar[0].RsvP->CWIsSet == TRUE))
    {
    P = MPar[0].RsvP;
    }
  else if ((N->P != NULL) && (N->P->CWIsSet == TRUE))
    {
    P  = N->P;
    }
  else if ((J != NULL) &&
           (J->Credential.C != NULL) && 
           (J->Credential.C->P != NULL) && 
           (J->Credential.C->P->CWIsSet == TRUE))
    {
    P  = J->Credential.C->P;
    }
  else if ((MSched.DefaultN.P != NULL) && 
           (MSched.DefaultN.P->CWIsSet == TRUE))
    {
    P  = MSched.DefaultN.P;
    }
  else
    {
    /* node allocation priority function not specified */

    *Val = (double)NPriority + 
      100.0 * JPref + 
      100.0 * RAff;

    return(SUCCESS);
    }

  CW = P->CW;

  /* P is guaranteed to not be NULL */

  tmpP = 0.0;

  if ((OSPrio == TRUE) &&
      (VMPrio == FALSE) &&
      (N->ActiveOS != J->Req[0]->Opsys) && 
      (CW[mnpcOS] == 0.0))
    {
    /* HACK:  disfavor nodes which do not have matching OS regardless of 
              whether we are considering nodes for immediate or future
              allocation */

    tmpP -= 1000.0;
    }

  /* NOTE:  do not use cached results if custom 'SpecP' is specified or if
            job specific priority function is used. */

  if (((J == NULL) || (J->NodePriority != P)) &&
      (SpecP != NULL) &&
      (N->P != NULL) &&
      (N->P->SPIsSet == TRUE))
    {
    tmpP += N->P->SP;
    }
  else 
    {
    double tmpSP;

    /* calculate static priority component */

    tmpSP = 0.0;

    tmpSP += CW[mnpcCDisk]     * N->CRes.Disk;
    tmpSP += CW[mnpcCMem]      * N->CRes.Mem;
    tmpSP += CW[mnpcCost]      * N->ChargeRate;
    tmpSP += CW[mnpcCProcs]    * N->CRes.Procs;
    tmpSP += CW[mnpcCSwap]     * N->CRes.Swap;
    tmpSP += CW[mnpcNodeIndex] * N->NodeIndex;
    tmpSP += CW[mnpcPriority]  * NPriority; 
    tmpSP += CW[mnpcSpeed]     * N->Speed;

    if (CW[mnpcFeature] != 0.0)
      {
      /* adjust priority if specified features are set */

      int findex;

      for (findex = 0;findex < MMAX_ATTR;findex++)
        {
        if ((P->FValues[findex] != 0.0) &&
            bmisset(&N->FBM,findex))
          {
          tmpSP += P->FValues[findex];
          }
        }
      }    /* END if (CW[mnpcFeature] != 0.0) */

    if ((CW[mnpcArch] != 0.0) && (N->Arch != 0))
      {
      if (P->AIndex == N->Arch)
        {
        /* add arch contribution */

        tmpSP += CW[mnpcArch];
        }
      }   /* END if ((CW[mnpcArch] != 0.0) && (N->Arch != 0)) */

    if (CW[mnpcProximity] != 0.0) 
      {
      /* NOTE:  enhance to support 'degrees' of proximity (NYI) */

      /* proximity ranges from 0 -> 1 with 1 being local */

      if ((N->RM != NULL) && (N->RM->Type != mrmtMoab))
        tmpSP += CW[mnpcProximity];
      }

    /* don't cache the priority on the node if custom SpecP is specified or
       when using job priority function */

    if (((J == NULL) || (J->NodePriority != P)) &&
        (SpecP == NULL) &&
       ((N->P != NULL) || 
        (MNPrioAlloc(&N->P) == SUCCESS)))
      {
      N->P->SP      = tmpSP;
      N->P->SPIsSet = TRUE;
      }

    tmpP += tmpSP;
    }  /* END else if ((N->P != NULL) && ...) */

  if (StartTime == (long)MSched.Time)
    {
    if ((N->P != NULL) && 
        (N->P->DPIsSet == TRUE) && 
        (N->P->DPIteration == MSched.Iteration) &&
        (CW[mnpcOS] == 0.0) &&
        (CW[mnpcReqOS] == 0.0))
      {
      tmpP += N->P->DP;
      }
    else
      {
      double tmpDP;
 
      /* calculate dynamic priority component */

      /* includes time and load based attributes */
 
      tmpDP = 0.0;

      /* consider disk space already allocated */
      tmpDP += CW[mnpcADisk] * (MIN(N->ARes.Disk,N->CRes.Disk - N->DRes.Disk));
      /* consider memory already allocated */
      tmpDP += CW[mnpcAMem] * (MIN(N->ARes.Mem,N->CRes.Mem - N->DRes.Mem));
      /* consider swap space already allocated */
      tmpDP += CW[mnpcASwap] * (MIN(N->ARes.Swap, N->CRes.Swap - N->DRes.Swap));
      tmpDP += CW[mnpcLoad]  * N->Load;    

      if (CW[mnpcServerLoad] != 0.0)
        {
        double tmpD;

        MNodeGetEffectiveLoad(N,marNONE,TRUE,TRUE,-1.0,&tmpD,NULL);

        tmpDP += CW[mnpcServerLoad] * tmpD;
        }

      if (N->Stat.FCurrentTime > 0)
        {
        tmpDP += CW[mnpcMTBF] * MCONST_DAYLEN / 
          MAX(1,N->Stat.OFCount[mtbf16h] + N->Stat.OFCount[mtbf8h]);
        }

      tmpDP += CW[mnpcUsage] * ((N->SUTime > 0) ? 
        ((double)N->SATime / N->SUTime) : 
        0.0);  

      if (CW[mnpcRandom] != 0.0)
        tmpDP += (double)(rand() % (int)CW[mnpcRandom]);

      if ((CW[mnpcOS] != 0.0) && (J != NULL))
        {
        /* NOTE:  this adjustment is only made if considering nodes for 
                  immediate allocation */

        /* add weight if node OS matches job */

        if (VMPrio == TRUE)
          {
          /* we need to prioritize the HyperVisor OS, not the VM OS */

          if (((N->ActiveOS > 0) &&
               (&MSched.VMOSList[N->ActiveOS] != NULL) &&
               (MUHTGet(&MSched.VMOSList[N->ActiveOS],MAList[meOpsys][J->Req[0]->Opsys],NULL,NULL) == SUCCESS)) ||
              ((N->NextOS > 0) &&
               (&MSched.VMOSList[N->NextOS] != NULL) &&
               (MUHTGet(&MSched.VMOSList[N->NextOS],MAList[meOpsys][J->Req[0]->Opsys],NULL,NULL) == SUCCESS)))
            {
            /* if node has a good HV or will soon be in the right HV then give it the boost in priority */

            tmpDP += (double)CW[mnpcOS];
            }
          }
        else if (N->ActiveOS == J->Req[0]->Opsys)
          {
          tmpDP += (double)CW[mnpcOS];
          }
        }

      if ((CW[mnpcPower] != 0.0) &&
          (N->PowerState == mpowOn))
        {
        /* add weight if node is ON */

        tmpDP += (double)CW[mnpcPower];
        }

      if ((CW[mnpcReqOS] != 0.0) && (J != NULL))
        {
        mre_t  *RE = N->RE;
        mjob_t *tJ;

        /* locate idle job reservations */

        /* NOTE:  walk node RE table to process reservations in time order */

        while (MNodeGetNextJob(N,MSched.Time,&RE,&tJ) == SUCCESS)
          {
          if ((tJ->Req[0] == NULL) || (tJ->Req[0]->Opsys == 0))
            continue;

          /* future job reservation located which requires specific OS */

          if (tJ->Req[0]->Opsys == J->Req[0]->Opsys)
            tmpDP += (double)CW[mnpcReqOS];
          else
            tmpDP -= (double)CW[mnpcReqOS];

          break;
          }  /* END for (rindex) */
        }

      if ((CW[mnpcAGRes] != 0.0) &&
          (P->AGRValues != NULL))
        {
        int rindex;

        for (rindex = 0; rindex < MSched.M[mxoxGRes]; rindex++)
          {
          if ((rindex > 0) && (MGRes.Name[rindex][0] == '\0'))
            break;

          if ((P->AGRValues[rindex] != 0.0) &&
              (MSNLGetIndexCount(&N->ARes.GenericRes,rindex) > 0))
            {
            tmpDP += P->AGRValues[rindex] * MSNLGetIndexCount(&N->ARes.GenericRes,rindex);
            }
          }
        }

      if ((CW[mnpcCGRes] != 0.0) &&
          (P->CGRValues != NULL))
        {
        int rindex;

        for (rindex = 0; rindex < MSched.M[mxoxGRes]; rindex++)
          {
          if ((rindex > 0) && (MGRes.Name[rindex][0] == '\0'))
            break;

          if ((P->CGRValues[rindex] != 0.0) &&
              (MSNLGetIndexCount(&N->CRes.GenericRes,rindex) > 0))
            {
            tmpDP += P->CGRValues[rindex] * MSNLGetIndexCount(&N->CRes.GenericRes,rindex);
            }
          }
        }

      if (N->XLoad != NULL)
        {
        /* add network load contribution */

        if ((P->GMValues != NULL) &&
            (CW[mnpcGMetric] != 0.0))
          {
          int mindex;

          for (mindex = 1;mindex < MSched.M[mxoxGMetric];mindex++)
            {
            if ((P->GMValues[mindex] != 0.0) &&
                (N->XLoad->GMetric[mindex] != NULL) &&
                (N->XLoad->GMetric[mindex]->IntervalLoad != MDEF_GMETRIC_VALUE))
              {
              tmpDP += P->GMValues[mindex] *
                N->XLoad->GMetric[mindex]->IntervalLoad;
              }
            }
          }
        }

      if ((N->P != NULL) ||
          (MNPrioAlloc(&N->P) == SUCCESS))
        {
        N->P->DP          = tmpDP;
        N->P->DPIsSet     = TRUE;
        N->P->DPIteration = MSched.Iteration;
        }
 
      tmpP += tmpDP;
      }
    }    /* END if (StartTime == MSched.Time) */
  else
    {
    /* address attributes with minor time-based influence */

    if ((CW[mnpcOS] != 0.0) && (J != NULL))
      {
      /* NOTE:  this adjustment is only made if considering nodes for
                immediate allocation */

      /* add weight if node OS matches job */

      if (N->ActiveOS == J->Req[0]->Opsys)
        {
        /* HACK:  should enable future OS weighting factor */

        tmpP += (double)CW[mnpcOS] / 4.0;
        }
      }
    }

  if (CW[mnpcFreeTime] != 0.0)
    {
    int FreeTime;

    int Duration = ((StartTime > 0) ? StartTime : MSched.Time) + MSched.FreeTimeLookAheadDuration;

    MNodeGetFreeTime(N,(StartTime > 0) ? StartTime : MSched.Time,Duration,&FreeTime);

    tmpP += CW[mnpcFreeTime] * FreeTime;
    }

  if ((CW[mnpcWindowTime] != 0.0) && (J != NULL))
    {
    int Start = (StartTime > 0) ? StartTime : MSched.Time;
    int WindowTime;

    MNodeGetWindowTime(N,Start,Start + J->WCLimit,&WindowTime);

    tmpP += CW[mnpcWindowTime] * WindowTime;
    }

  /* add intra-iteration/context-based components */

  tmpP += CW[mnpcPref]        * JPref;
  tmpP += CW[mnpcRsvAffinity] * RAff;  

  if (CW[mnpcVMCount] != 0.0)
    {
    int VMCount;

    VMCount = MULLGetCount(N->VMList);

    tmpP += CW[mnpcVMCount] * VMCount;
    }

  MLocalNodeGetPriority(N,J,&tmpLP);

  tmpP += CW[mnpcLocal] * tmpLP;

  if (StartTime <= (long)MSched.Time)
    {
    /* handle current usage weights */

    tmpP += CW[mnpcAProcs]             * N->ARes.Procs;   
    tmpP += CW[mnpcJobCount]           * MNodeGetJobCount(N);
    tmpP += CW[mnpcSuspendedJCount]    * N->SuspendedJCount;

    if (N->PtIndex > 0)
      {
      tmpP += CW[mnpcParAProcs]  * MPar[N->PtIndex].ARes.Procs;

      if (Msg != NULL)
        {
        if (CW[mnpcParAProcs] != 0)
          {
          MUSNPrintF(&BPtr,&BSpace," +%.2f*ParAProcs[%d]",
            CW[mnpcParAProcs],
            MPar[N->PtIndex].ARes.Procs);
          }
        }

      tmpP += CW[mnpcPParAProcs] * 
        MPar[N->PtIndex].ARes.Procs / MAX(1,MPar[N->PtIndex].CRes.Procs);

      if (Msg != NULL)
        {
        if (CW[mnpcPParAProcs] != 0)
          {
          MUSNPrintF(&BPtr,&BSpace," +%.2f*PParAProcs[%d]",
            CW[mnpcPParAProcs],
            MPar[N->PtIndex].ARes.Procs / MAX(1,MPar[N->PtIndex].CRes.Procs));
          }
        }
      }
    }       /* END if (StartTime <= (long)MSched.Time) */
  else
    { 
    /* NOTE:  should determine estimated available resources for specified timeframe */

    /* doing this right is computationally expensive */

    /* for now, simply use current node available resources */

    tmpP += CW[mnpcAProcs] * N->ARes.Procs;
    }

  assert(!isnan(tmpP));
#if !defined(__SOLARISX86) && !defined(__SOLARIS)
  assert(!isinf(tmpP));
#endif

  *Val = tmpP; 

  return(SUCCESS);
  }  /* END MNodeGetPriority() */
/* END MNodeGetPriority.c */
