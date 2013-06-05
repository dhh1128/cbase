/* HEADER */

      
/**
 * @file MJobGetSNRange.c
 */

/* Contains:                                 *
 *  int MJobGetSNRange                       *
 *                                           */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


extern int __MNodeGetPEPerTask(const mcres_t *,const mcres_t *);


/**
 * Evaluate per-node per-req resource access for specified time range.
 *
 * @see MJobGetNRange() - parent
 * @see MJobGetRange()
 * @see MRLMergeTime() - child
 *
 * return up to <RCOUNT> ranges starting within the range <RRANGE> *
 * NOTE:  BRsv may be in format <RSVID>/policy if node policies in effect *
 *
 * NOTE:  can we determine if J is a preemptor attemting to access preemptee R's resources?
 *
 * NOTE:  does not consider nodes available resources (N->ARes) unless there are
 *        no reservations on node during relevant timeframe
 *
 * @param J (I) job
 * @param RQ (I) requirement checked
 * @param N (I) node
 * @param RRange (I) required range for resource availability
 * @param RCount (I) number of ranges requested
 * @param Affinity (O) [optional]
 * @param Type (O) [optional,not set]
 * @param ARange (O) available range list
 * @param DRes (I) [optional]
 * @param SeekLong (I)
 * @param BRsvID (O) [optional,minsize=MMAX_NAME] - Name of the reservation that is blocking the job
 */

int MJobGetSNRange(

  const mjob_t      *J,              /* I job                                      */
  const mreq_t      *RQ,             /* I requirement checked                      */
  const mnode_t     *N,              /* I node                                     */
  mrange_t          *RRange,         /* I required range for resource availability */
  int                RCount,         /* I number of ranges requested               */
  char              *Affinity,       /* O node affinity for job (optional)         */
  enum MRsvTypeEnum *Type,           /* O type of reservation located which blocks usage (optional,not set) */
  mrange_t          *ARange,         /* O available range list                     */
  mcres_t           *DRes,           /* I dedicated resources required (optional)  */
  mbool_t            SeekLong,       /* I                                          */
  char              *BRsvID)         /* O blocking rsvid (optional,minsize=MMAX_NAME) */

  {
  /* state variables */

  int RangeIndex;
  int index;

  long    JDuration;

  mcres_t AvlRes;  /* resources available to be dedicated to J - NOT N->ARes */
  mcres_t DedRes;
  mcres_t DQRes;

  int     TC;

  enum MAllocRejEnum RIndex;

  /* effective node access policy */

  enum MNodeAccessPolicyEnum NAccessPolicy;

  mrsv_t *R;
  int     RC;
  int     rc;

  int     RoutineRC = SUCCESS;

  mre_t   *RE;
  mre_t   *NextRE;

  mcres_t *BR;
  mcres_t  tmpBR;

  /* booleans */

  mbool_t  PreRes;       
  mbool_t  UseDed;      
  mbool_t  ActiveRange;

  mbool_t  ResourcesAdded;  
  mbool_t  ResourcesRemoved; 

  mbool_t  PolicyBlock;
  mbool_t  AdvResMatch;
  mbool_t  NodePolicyActive;
  mbool_t  ACLIsConditional;

  mbool_t  IsPreemptTest = FALSE;
  mbool_t  RsvAllowsJAccess;
  mbool_t  RsvNotApplicable = FALSE;

  /* temp variables */

  long   Overlap;
  mulong tmpUL;

  maclcon_t AC;

  char   WCString[MMAX_NAME];

  long   RsvOffset;

  int    InclusiveRsvCount;
  int    InclusiveRsvMaxSize;

  int    tmpTC;

  int    EffNC;  /* number of nodes represents by N - normally '1' except when MultiNode is enabled */

  /* node policy usage */

  int    JC;  /* node job count */
  int    UJC; /* 'job per user' count */
  int    UPC; /* 'proc per user' count */
  int    GJC; /* job per group' count */
  int    GPC; /* 'proc per group' count */
  int    CPC; /* proc per class' count */
  int    TPE;

  /* node policy limits */

  int    MJC;
  int    MUJC;
  int    MUPC;
  int    MGJC;
  int    MGPC;
  int    MCPC;
  int    MPE;

  char   pval;


  mjob_t *tmpJ;

  mulong RsvTimeDepth;

  mbool_t RsvIsGhost;
  char    TString[MMAX_LINE];

  const char *FName = "MJobGetSNRange";

  MULToTString(RRange[0].STime - MSched.Time,TString);

  MDBO(7,J,fSCHED) MLog("%s(%s,%d,%s,(%d@%s),%d,%s,Type,ARange,%s,BRsvID)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    N->Name,
    RRange[0].TC,
    TString,
    RCount,
    (Affinity == NULL) ? "NULL" : "Affinity",
    MBool[SeekLong]);

  if ((J == NULL) || (RQ == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  MDBO(8,J,fSCHED) MLog("INFO:     attempting to get resources for %s %d * (P: %d  M: %d  S: %d  D: %d)\n",
    J->Name,
    RRange[0].TC,
    RQ->DRes.Procs,
    RQ->DRes.Mem,
    RQ->DRes.Swap,
    RQ->DRes.Disk);

  IsPreemptTest = MSched.IsPreemptTest;

  memset(&ARange[0],0,sizeof(mrange_t));

  if (!bmisset(&J->Flags,mjfRsvMap) &&
      (MPar[0].NodeDownStateDelayTime < 0) &&
      ((N->State == mnsDrained) || (N->State == mnsDown)))
    {
    /* unlimited down state delay time specified and node is in down/bad state */

    return(FAILURE);
    }

  UseDed = FALSE;

  pval = mrapNONE;

  if ((N != NULL) && (N->NAvailPolicy != NULL))
    {
    pval = (N->NAvailPolicy[mrProc] != mrapNONE) ?
      N->NAvailPolicy[mrProc] : N->NAvailPolicy[0];
    }

  if (pval == mrapNONE)
    {
    pval = (MPar[0].NAvailPolicy[mrProc] != mrapNONE) ?
      MPar[0].NAvailPolicy[mrProc] : MPar[0].NAvailPolicy[0];
    }

  /* determine policy impact */

  if ((pval == mrapDedicated) || (pval == mrapCombined))
    {
    UseDed = TRUE;
    }

  RsvOffset = 0;

  if ((J->Credential.Q != NULL) && (J->Credential.Q->RsvTimeDepth > 0))
    RsvTimeDepth = J->Credential.Q->RsvTimeDepth;
  else
    RsvTimeDepth = MSched.RsvTimeDepth;

  /*  NOTE:  modifications needed to keep inclusive reservations using
   *         only inclusive resources on SMP nodes
   */

  if (Type != NULL)
    *Type = mrtNONE;

  if (Affinity != NULL)
    *Affinity = mnmNONE;

  if ((J->Credential.Q != NULL) &&
     ((J->Credential.Q->QTACLThreshold > 0) || 
      (J->Credential.Q->XFACLThreshold > 0.0) ||
      (J->Credential.Q->BLACLThreshold > 0.0)))
    {
    ACLIsConditional = TRUE;
    }
  else
    {
    memset(&AC,0,sizeof(AC));

    ACLIsConditional = FALSE;
    }

  ARange[0].ETime = 0;
  ARange[0].TC    = 0;
  ARange[0].NC    = 0;

  EffNC = 1;

  /* skip irrelevent node events */

  tmpUL = MAX(RRange[0].STime,MSched.Time);

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if ((R->EndTime > tmpUL) ||
        (R->EndTime >= MMAX_TIME - 1))
      break;

    MDBO(6,J,fSCHED) MLog("INFO:     skipping early N->RETable event for '%s' (E: %ld <= S: %ld) : P: %ld\n",
      R->Name,
      R->EndTime,
      RRange[0].STime,
      MSched.Time);
    }  /* END for (MREGetNext) */

  JDuration = J->SpecWCLimit[0];

  /* initialize available resources */

  /* specify MMAX_TIME to not constrain based on dedicated VM workload */

  MCResInit(&AvlRes);

  MJobGetFeasibleNodeResources(J,mvmupNONE,N,&AvlRes);

  if (MCResCanOffer(&AvlRes,&RQ->DRes,MJOBREQUIRESVMS(J),NULL) == FALSE)
    {
    /* Server cannot provide at least one task because and only because of 
       virtual machine constraints */

    ARange[0].STime = MMAX_TIME;
    ARange[0].ETime = MMAX_TIME;

    ARange[0].TC = MNodeGetTC(
      N,
      &N->CRes,
      &N->CRes,
      &N->DRes,
      &RQ->DRes,
      MAX(MSched.Time,RRange[0].STime),
      1,
      NULL);

    MCResFree(&AvlRes);

    return(SUCCESS);
    }

  /* WARNING!!!!! From here on instead of calling "return" you must set RoutineRC and use the goto "free_and_return" */

  MCResInit(&DedRes);
  MCResInit(&DQRes);
  MCResInit(&tmpBR);

  if (RE == NULL)
    {
    /* if no relevent reservations present */

    /* NOTE: node should be idle unless RRange[0].STime is in the future */
    /*       MNodeGetTC() should handle future evaluations and ignore ARes */

    if (((!MReqIsGlobalOnly(RQ)) ||
          (bmisset(&J->Flags,mjfGResOnly))) &&
        (J->ReqRID != NULL) && 
        (!bmisset(&J->IFlags,mjifNotAdvres)))
      {
      MDBO(6,J,fSCHED) MLog("INFO:     node %s has no reservations for %s:%d (reqrsv: %s)\n",
        N->Name,
        J->Name,
        RQ->Index,
        J->ReqRID);

      /* required rsv not located */

      if (Affinity != NULL)
        *Affinity = mnmUnavailable;

      RoutineRC = FAILURE;

      goto free_and_return;
      }

    ARange[0].TC = MNodeGetTC(
      N,
      (bmisset(&J->Flags,mjfRsvMap)) ? &N->CRes : &N->ARes,  /* change was made to allow rsv to pick up node with background load - side effects? 6/23/06 */
      &AvlRes,
      &N->DRes,
      &RQ->DRes,
      MAX(MSched.Time,RRange[0].STime),
      MAX(1,RRange[0].TC),
      &RIndex);

    ARange[0].NC = EffNC;

    if (ARange[0].TC < RRange[0].TC)
      {
      mcres_t tmpRes;

      MDBO(6,J,fSCHED) MLog("INFO:     node %s contains inadequate resources for %s:%d (state: %s,reason: %s)\n",
        N->Name,
        J->Name,
        RQ->Index,
        MNodeState[N->State],
        MAllocRejType[RIndex]);

      if (RRange[0].STime <= MSched.Time)
        {
        /* NOTE:  utilization spike may be impacting available resources 
                  re-calculate N->ARes as (N->CRes - N->DRes) */

        MCResInit(&tmpRes);

        MCResCopy(&tmpRes,&AvlRes);

        MCResRemove(&tmpRes,&N->CRes,&N->DRes,1,FALSE);

        tmpTC = MNodeGetTC(
          N,
          &tmpRes,
          &N->CRes,
          &N->DRes,
          &RQ->DRes,
          MAX(MSched.Time,RRange[0].STime),
          1,
          NULL);

        MCResFree(&tmpRes);

        if (tmpTC >= RRange[0].TC)
          {
          /* experiencing resource race condition */

          /* available resources do not match (cfg - ded) resources */

          /* allow reservation to proceed */

          if (IsPreemptTest == TRUE)
            {
            /* NOTE:  for preemption test, ignore available resource status since
                      act of preemption will change usage */

            RsvOffset = 1;
            }
          else
            {
            RsvOffset = MSched.MaxRMPollInterval;
            }

          ARange[0].TC = tmpTC;
          }
        else
          {
          /* resources unavailable regardless of current usage */

          if (Affinity != NULL)
            *Affinity = mnmUnavailable;

          RoutineRC = FAILURE;

          goto free_and_return;
          }
        }    /* END if (RRange[0].STime <= MSched.Time) */
      else
        {
        /* resources unavailable regardless of current usage */

        if (Affinity != NULL)
          *Affinity = mnmUnavailable;

        RoutineRC = FAILURE;
        goto free_and_return;
        }
      }    /* END if (ARange[0].TC < RRange[0].TC) */

    ARange[0].STime = MAX(RRange[0].STime,MSched.Time + RsvOffset);
    ARange[0].ETime = MMAX_TIME;

    /* enforce state based time constraints - NOTE: no reservations found
       on node, only one availability range exists */

    if (!bmisset(&J->Flags,mjfRsvMap) || !bmisset(&J->SpecFlags,mjfIgnState))
      {
      long StateDelayTime;

      MJobGetNodeStateDelay(J,RQ,N,&StateDelayTime);

      if (StateDelayTime > 0)
        {
        if (MSched.Time + StateDelayTime >= ARange[0].ETime)
          {
          /* no valid resources - terminate range */

          ARange[0].ETime = 0;
          }
        else
          {
          ARange[0].STime = MAX(
            ARange[0].STime,
            MSched.Time + StateDelayTime);
          }
        }
      else if (StateDelayTime < 0)
        {
        /* indefinite delay - no valid resources - terminate range */

        ARange[0].ETime = 0;
        }
      }    /* END if (!bmisset(&J->Flags,mjfRsvMap) || ...) */

    MDBO(8,J,fSCHED) MLog("INFO:     ARange[0] adjusted (TC: %d  S: %ld)\n",
      ARange[0].TC,
      ARange[0].STime);

    /* terminate range list */

    ARange[1].ETime = 0;

    MDBO(8,J,fSCHED)
      {
      MULToTString(ARange[0].STime - MSched.Time,TString);

      strcpy(WCString,TString);

      MULToTString(ARange[0].ETime - MSched.Time,TString);

      MLog("INFO:     closing ARange[%d] (%s -> %s : %d) (0)\n",
        0,
        WCString,
        TString,
        ARange[0].TC);
      }

    MULToTString(JDuration,TString);

    strcpy(WCString,TString);

    MULToTString(ARange[0].ETime - ARange[0].STime,TString);

    MDBO(5,J,fSCHED) MLog("INFO:     node %s supports %d task%s of job %s:%d for %s of %s (no reservation)\n",
      N->Name,
      ARange[0].TC,
      (ARange[0].TC == 1) ? "" : "s",
      J->Name,
      RQ->Index,
      TString,
      WCString);

    if (DRes != NULL)
      {
      MCResClear(DRes);

      MCResAdd(DRes,&N->CRes,&N->CRes,1,FALSE);
      }

    if (BRsvID != NULL)
      strcpy(BRsvID,"NONE");

    if (((bmisset(&J->Flags,mjfAdvRsv)) && (!MReqIsGlobalOnly(RQ))) &&
        (!bmisset(&J->IFlags,mjifNotAdvres)))
      {
      if (Affinity != NULL)
        *Affinity = mnmUnavailable;

      MDBO(5,J,fSCHED) MLog("INFO:     no reservation found for AR job %s\n",
        J->Name);

      RoutineRC = FAILURE;
      goto free_and_return;
      }

    if ((ARange[0].ETime == 0) || (ARange[0].TC < RRange[0].TC))
      {
      RoutineRC = FAILURE;
      goto free_and_return;
      }

    /* last block copied from end of MJobGetSNRange() */

    if ((Affinity != NULL) && !bmisset(&J->Flags,mjfBestEffort))
      {
      if (bmisset(&J->IFlags,mjifHostList) && (J->ReqHLMode != mhlmSuperset))
        {
        if (J->ReqHLMode != mhlmSubset)
          {
          *Affinity = mnmRequired;
          }
        else
          {
          mnode_t *tmpN;

          int hindex;
   
          /* look for match */
   
          for (hindex = 0;MNLGetNodeAtIndex(&J->ReqHList,hindex,&tmpN) == SUCCESS;hindex++)
            {
            if (tmpN == N)
              {
              *Affinity = mnmRequired;
   
              break;
              }
            }    /* END for (hindex) */
          }
        }
      }          /* END if (Affinity != NULL) */
  
    /* add required resources to min resource available */
   
    if (DRes != NULL)
      {
      MCResAdd(DRes,&N->CRes,&RQ->DRes,RRange[0].TC,FALSE);
      }

    /* NOTE: does not enforce policies if no reservations found */

    RoutineRC = SUCCESS;
    goto free_and_return;
    }  /* END if (RE->Type == mreNONE) */

  /* initialize reservation state */

  InclusiveRsvCount = 0;
  InclusiveRsvMaxSize = 0;

  ActiveRange = FALSE;
  PolicyBlock      = FALSE;

  if ((MReqIsGlobalOnly(RQ)) &&
      (bmisset(&J->Flags,mjfGResOnly)))
    {
    /* if job is gres only then we DO NOT want to enforce advres */

    AdvResMatch = FALSE;
    }
  else if ((!MReqIsGlobalOnly(RQ)) && 
      (J->ReqRID != NULL) && 
      (!bmisset(&J->IFlags,mjifNotAdvres)))
    {
    /* ignore job's ReqRID if this req is requesting global resources */

    AdvResMatch = FALSE;
    }
  else
    {
    AdvResMatch = TRUE;
    }

  PreRes      = TRUE;
  RangeIndex      = 0;

  /* initialize policies */

  NodePolicyActive = FALSE;

  JC  = 0;

  CPC = 0;
  UJC = 0;
  UPC = 0;
  GJC = 0;
  GPC = 0;

  TPE = 0;

  /* set node policy limits */

  MNodeGetEffNAPolicy(
    N,
    N->EffNAPolicy,
    MPar[N->PtIndex].NAccessPolicy,
    J->Req[0]->NAccessPolicy,
    &NAccessPolicy);

  if (bmisset(&J->IFlags,mjifIgnoreNodeAccessPolicy))
    NAccessPolicy = mnacShared;

  /* NOTE:  need mnacSingleActiveJob policy to only block sharing until current active
            jobs complete */

  /* NOTE:  should enforce mnacSingleActiveJob for non-VM jobs if image changes preclude sharing */

  if ((NAccessPolicy != mnacSingleJob) && (NAccessPolicy != mnacSingleTask))
    {
    MNodeGetAdaptiveAccessPolicy(N,J,&NAccessPolicy);

    if (NAccessPolicy == mnacSingleJob)
      {
      /* indicates server provisioning is required */

      if (MNodeHasVMs(N) == TRUE)
        {
        /* do not provision server with active job or container VMs */

        /* NOTE:  should be smart enough to migrate VMs away when needed (NYI) */

        RoutineRC = FAILURE;
        goto free_and_return;
        }
      }
    }

  if ((NAccessPolicy == mnacSingleJob) ||
      (NAccessPolicy == mnacSingleTask))
    {
    /* FIXME: SINGLETASK should limit the outgoing TC to no more than 1 */

    MJC = 1;
    }
  else if (N->AP.HLimit[mptMaxJob] > 0)
    {
    MJC = N->AP.HLimit[mptMaxJob];
    }
  else
    {
    MJC = MSched.DefaultN.AP.HLimit[mptMaxJob];
    }

  if (MJC > 0)
    NodePolicyActive = TRUE;

  if (N->AP.HLimit[mptMaxPE] > 0)
    {
    MPE = N->AP.HLimit[mptMaxPE];
    }
  else
    {
    MPE = MSched.DefaultN.AP.HLimit[mptMaxPE];
    }

  if (MPE > 0)
    NodePolicyActive = TRUE;

  if (N->NP != NULL)
    {
    if (MSched.DefaultN.NP != NULL)
      {
      /* group policies */

      if (N->NP->MaxProcPerClass > 0)
        MCPC = N->NP->MaxProcPerClass;
      else
        MCPC = MSched.DefaultN.NP->MaxProcPerClass;

      if (N->NP->MaxJobPerGroup > 0)
        MGJC = N->NP->MaxJobPerGroup;
      else
        MGJC = MSched.DefaultN.NP->MaxJobPerGroup;

      if (N->NP->MaxProcPerGroup > 0)
        MGPC = N->NP->MaxProcPerGroup;
      else
        MGPC = MSched.DefaultN.NP->MaxProcPerGroup;

      /* user policies */

      if (N->NP->MaxJobPerUser > 0)
        MUJC = N->NP->MaxJobPerUser;
      else
        MUJC = MSched.DefaultN.NP->MaxJobPerUser;

      if (N->NP->MaxProcPerUser > 0)
        MUPC = N->NP->MaxProcPerUser;
      else
        MUPC = MSched.DefaultN.NP->MaxProcPerUser;
      }
    else
      {
      MGJC = N->NP->MaxJobPerGroup;
      MGPC = N->NP->MaxProcPerGroup;
      MUJC = N->NP->MaxJobPerUser;
      MUPC = N->NP->MaxProcPerUser;
      MCPC = N->NP->MaxProcPerClass;
      }
    }    /* END if (N->NP != NULL) */
  else if (MSched.DefaultN.NP != NULL)
    {
    MGJC = MSched.DefaultN.NP->MaxJobPerGroup;
    MGPC = MSched.DefaultN.NP->MaxProcPerGroup;
    MUJC = MSched.DefaultN.NP->MaxJobPerUser;
    MUPC = MSched.DefaultN.NP->MaxProcPerUser;
    MCPC = MSched.DefaultN.NP->MaxProcPerClass;
    }
  else
    {
    /* no node policies specified */

    MUJC = 0;
    MUPC = 0;
    MGJC = 0;
    MGPC = 0;
    MCPC = 0;
    }

  if ((J->Credential.C != NULL) && (J->Credential.C->MaxProcPerNode > 0))
    MCPC = J->Credential.C->MaxProcPerNode;

  if ((MUJC > 0) || (MUPC > 0) || (MGJC > 0) || (MGPC > 0) || (MCPC > 0))
    NodePolicyActive = TRUE;

  DQRes.Procs = -1;

  tmpTC = RRange[0].TC;

  MDBO(8,J,fSCHED)
    {
    MLog("INFO:     initial resources: %s\n",
      MCResToString(&AvlRes,0,mfmHuman,NULL));

    MLog("INFO:     requested resources: %s (x%d)\n",
      MCResToString(&RQ->DRes,0,mfmHuman,NULL),
      tmpTC);
    }

  /* consume required resources */

  MCResRemove(&AvlRes,&N->CRes,&RQ->DRes,tmpTC,FALSE);

  if (MCResIsNeg(&AvlRes,&RIndex,NULL) == TRUE)
    {
    /* if inadequate resources are configured */

    MDBO(6,J,fSCHED) MLog("INFO:     inadequate resources configured for %s:%d on node %s (%s)\n",
      J->Name,
      RQ->Index,
      N->Name,
      MAllocRejType[RIndex]);

    if (Affinity != NULL)
      *Affinity = mnmUnavailable;

    RoutineRC = FAILURE;
    goto free_and_return;
    }

  if (UseDed == TRUE)
    MCResAdd(&DedRes,&N->CRes,&RQ->DRes,tmpTC,FALSE);

  if (NAccessPolicy == mnacSingleTask)
    {
    /* all resources dedicated */

    MCResClear(&AvlRes);
    }

  if (DRes != NULL)
    {
    /* set up default structures */

    MCResClear(DRes);

    MCResAdd(DRes,&AvlRes,&N->CRes,1,FALSE);
    }

  ResourcesAdded   = TRUE;
  ResourcesRemoved = TRUE;

  /* step through node events */

  for (;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if (R->EndTime <= RRange[0].STime)
      continue;

    if ((!bmisset(&J->IFlags,mjifIgnGhostRsv)) &&
        (MRSVISGHOSTRSV(R) == TRUE)) 
      RsvIsGhost = TRUE;
    else
      RsvIsGhost = FALSE;

    if ((RE->Time > (long)RRange[0].STime) &&
        (PreRes == TRUE))
      {
      /* adjust/check requirements at job start */

      PreRes = FALSE;

      if ((MCResIsNeg(&AvlRes,NULL,&RQ->DRes) == TRUE) ||
          (PolicyBlock == TRUE) ||
          (AdvResMatch == FALSE))
        {
        MULToTString(RRange[0].STime - MSched.Time,TString);

        MDBO(6,J,fSCHED) MLog("INFO:     resources unavailable at time %s (%s:%s)\n",
          TString,
          MBool[PolicyBlock],
          MBool[AdvResMatch]);

        /* no active range can exist if resource unavailable at starttime */

        ActiveRange = FALSE;

        /* 'starttime' range is not active */

        if (RE->Time >= (long)RRange[0].ETime)
          {
          mbool_t MergeAvailable = MBNOTSET;

          MREGetNext(RE,&NextRE);

          /* NOTE:  rsv may be broken into chunks spanning multiple RE spaces - consider this before giving up - NYI */

          if (NextRE == NULL)
            {
            /* end of list or time-separated RE events */

            MergeAvailable = FALSE;
            }
          else
            {
            for (;NextRE != NULL;MREGetNext(NextRE,&NextRE))
              {
              if (NextRE->Time != RE->Time)
                break;

              if (MREGetRsv(NextRE) != MREGetRsv(RE)) 
                continue;

              if (NextRE->Type == mreStart)
                MergeAvailable = TRUE;
              }  /* END for (teindex) */
            }

          if (MergeAvailable != TRUE)
            {
            /* range not active at time '0' and next event is too late to be considered */

            if (AdvResMatch == TRUE)
              {
              MDBO(8,J,fSCHED) MLog("INFO:     reservation %s extends beyond acceptable time range (%ld >= %ld)\n",
                R->Name,
                RE->Time,
                RRange[0].ETime);
              }
            else
              {
              MDBO(8,J,fSCHED) MLog("INFO:     required reservation %s extends beyond acceptable time range (%ld >= %ld)\n",
                R->Name,
                RE->Time,
                RRange[0].ETime);
              }

            break;
            }
          }    /* END if (RE->Time >= (long)RRange[0].ETime) */
        }      /* END if if ((MCResIsNeg(&AvlRes,NULL,&RQ->DRes) == TRUE) || ...) */
      else
        {
        TC = MNodeGetTC(N,&AvlRes,&N->CRes,&DedRes,&RQ->DRes,MSched.Time,1,NULL);

        if (InclusiveRsvMaxSize > 0)
          {
          /* for advrsv jobs, constrain available tasks */

          TC = MIN(
                 TC,
                 (InclusiveRsvMaxSize - RRange[0].TC * RQ->DRes.Procs) / MAX(1,RQ->DRes.Procs));
          }

        ARange[RangeIndex].TC = RRange[0].TC + TC;
        ARange[RangeIndex].NC = EffNC;
        ARange[RangeIndex].STime = MAX(RRange[0].STime,MSched.Time + RsvOffset);

        ActiveRange = TRUE;

        MDBO(8,J,fSCHED) MLog("INFO:     ARange[%d] started (TC: %d  S: %ld)\n",
          RangeIndex,
          ARange[RangeIndex].TC,
          ARange[RangeIndex].STime);

        if (DRes != NULL)
          MCResGetMin(DRes,DRes,&AvlRes);
        }
      }   /* END if ((RE->Time > RRange[0].STime) && ... */

    if ((RE->Time > (long)RRange[0].ETime) &&
        (ActiveRange == FALSE))
      {
      break;
      }

    if ((RsvTimeDepth > 0) && !bmisset(&J->Flags,mjfRsvMap))
      {
      /* if time depth set, break out of loop if no range is active, and window exceeded or if
         range if active and window + jduration is exceeded */

      if (((mulong)RE->Time > MSched.Time + MSched.MaxRMPollInterval + MSched.RsvTimeDepth) &&
          (ActiveRange == FALSE))
        break;
      
      if ((mulong)RE->Time > MSched.Time + MSched.MaxRMPollInterval + MSched.RsvTimeDepth + JDuration)
        break;
      }    /* END if (MSched.RsvTimeDepth > 0) */

    /* adjust resources */

    if (R->Type != mrtUser)
      {
      /* NOTE:  DRes represents 'per task' blocked resources */

      /* If a ghost reservation then DRes procs will be zero */
      /* check job's DRes instead during preemption */

      if (bmisset(&J->IFlags,mjifIgnGhostRsv) &&
          (R->J != NULL) &&
          (R->J->Req[0] != NULL))
        BR = &R->J->Req[0]->DRes;
      else
        BR = &R->DRes;

      RC = RE->TC;

      MDBO(8,J,fSCHED) MLog("INFO:     non-user reservation '%s'x%d resources %s\n",
        R->Name,
        RC,
        MCResToString(BR,0,mfmHuman,NULL));
      }
    else
      {
      /* NOTE:  BRes represents 'total' blocked resources */

      /* NOTE:  for preemptor jobs, BR should be BR + resources of preemptee (NYI) */

      MCResCopyBResAndAdjust(&tmpBR,&RE->BRes,&N->CRes);
      BR = &tmpBR;

      RC = RE->TC;

      MDBO(8,J,fSCHED) MLog("INFO:     user reservation '%s'x%d resources %s\n",
        R->Name,
        RC,
        MCResToString(BR,0,mfmHuman,NULL));
      }

    if (ActiveRange == TRUE)
      {
      /* be generous (ie, JDuration - (R->StartTime - ARange[RangeIndex].STime)) */

      Overlap =
        (long)MIN(R->EndTime,ARange[RangeIndex].STime + JDuration) -
        (long)MAX(R->StartTime,ARange[RangeIndex].STime);

      if (Overlap < 0)
        Overlap = R->EndTime - R->StartTime;
      }
    else
      {
      Overlap = R->EndTime - MAX(R->StartTime,RRange[0].STime);
      }

    Overlap = MIN(JDuration,Overlap);

    MULToTString(RE->Time - MSched.Time,TString);

    MDBO(7,J,fSCHED) MLog("INFO:     checking reservation %s %s at time %s (O: %ld)\n",
      R->Name,
      (RE->Type == mreStart) ? "start" : "end",
      TString,
      Overlap);

    if (RE->Type == mreStart)
      {
      /* reservation start */

      if (ACLIsConditional == TRUE)
        {
        AC.QT = MAX(MSched.Time,R->StartTime) - J->SubmitTime;

        AC.XF = (AC.QT + JDuration) / (JDuration);

        if ((J->Credential.Q != NULL) && (J->Credential.Q->L.IdlePolicy != NULL))
          AC.BL = J->Credential.Q->L.IdlePolicy->Usage[mptMaxPS][0];
        }

      RsvAllowsJAccess = MRsvCheckJAccess(R,J,Overlap,&AC,FALSE,NULL,NULL,Affinity,NULL);

      RsvNotApplicable = (MJobRsvApplies(J,N,R,NAccessPolicy) == FALSE);

      if ((RsvAllowsJAccess == TRUE) || (RsvNotApplicable == TRUE))
        {
        MDBO(8,J,fSCHED) MLog("INFO:     reservations are inclusive\n");

        InclusiveRsvCount++;

        if (((bmisset(&J->Flags,mjfAdvRsv)) && (!MReqIsGlobalOnly(RQ))) &&
            (!bmisset(&J->IFlags,mjifNotAdvres)))
          {
          int BProcs = (RE->BRes.Procs == -1) ? N->CRes.Procs : RE->BRes.Procs;

          InclusiveRsvMaxSize = MAX(InclusiveRsvMaxSize,BProcs * RC);

          ResourcesAdded = TRUE;
          }

        if (bmisset(&J->IFlags,mjifIsPreempting) && (R->Type == mrtJob))
          {
          mbitmap_t tmpL;

          bmset(&tmpL,mrMem);
          bmset(&tmpL,mrDisk);
          bmset(&tmpL,mrSwap);
          bmset(&tmpL,mrGRes);

          ResourcesRemoved = TRUE;

          /* remove non-suspendable resources from AvlRes (like mem, disk, swap) */

          MCResRemoveSpec(&AvlRes,&N->CRes,BR,RC,FALSE,&tmpL);

          if (UseDed == TRUE)
            {
            bmclear(&tmpL);

            bmset(&tmpL,mrProc);

            MCResAddSpec(&DedRes,&N->CRes,BR,RC,FALSE,&tmpL);
            }
          }
        }    /* END if (MRsvCheckJAccess(R,J,Overlap,&AC,NULL,Affinity,NULL) == TRUE) */
      else
        {
        MDBO(8,J,fSCHED) MLog("INFO:     reservations are exclusive\n");

        /* add rsv to tree */

        /* NYI */

        tmpJ = R->J;

        if ((NodePolicyActive == TRUE) && (tmpJ != NULL) && (tmpJ != J->ActualJ))
          {
          if (RsvIsGhost == FALSE)
            {
            JC++;
            }

          if (J->Credential.C == tmpJ->Credential.C)
            {
            CPC += tmpJ->TotalProcCount;
            }

          if (J->Credential.G == tmpJ->Credential.G)
            {
            GJC++;

            GPC += tmpJ->TotalProcCount;
            }

          if (J->Credential.U == tmpJ->Credential.U)
            {
            UJC++;

            UPC += tmpJ->TotalProcCount;
            }
   
          if (MPE > 0)
            {
            TPE += __MNodeGetPEPerTask(&N->CRes,&tmpJ->Req[0]->DRes);
            }
          }    /* END if ((NodePolicyActive == TRUE) && (tmpJ != NULL)) */

        ResourcesRemoved = TRUE;

        if ((NAccessPolicy == mnacSingleUser) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.U != NULL) &&
            (J->Credential.U != NULL) &&
            (tmpJ->Credential.U != J->Credential.U) &&
            (J->Credential.U != NULL) &&
            (strcmp(J->Credential.U->Name,"[ALL]")))
          {
          /* user dedicated resources removed */

          MCResRemove(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResAdd(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else if ((NAccessPolicy == mnacSingleGroup) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.G != NULL) &&
            (J->Credential.G != NULL) &&
            (tmpJ->Credential.G != J->Credential.G) &&
            (J->Credential.G != NULL) &&
            (strcmp(J->Credential.G->Name,"[ALL]")))
          {
          /* group dedicated resources removed */

          MCResRemove(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResAdd(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else if ((NAccessPolicy == mnacSingleAccount) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.A != NULL) &&
            (J->Credential.A != NULL) &&
            (tmpJ->Credential.A != J->Credential.A) &&
            (J->Credential.A != NULL) &&
            (strcmp(J->Credential.A->Name,"[ALL]")))
          {
          /* account dedicated resources removed */

          MCResRemove(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResAdd(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else if ((NAccessPolicy == mnacUniqueUser) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.U != NULL) &&
            (J->Credential.U != NULL) &&
            (tmpJ->Credential.U == J->Credential.U))
          {
          /* user dedicated resources removed */

          MCResRemove(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResAdd(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else
          {
          if ((J->RsvAList != NULL) && 
              (J->RsvAList[0] != NULL) &&
               bmisset(&J->Flags,mjfPreemptor) &&
              (R->Type == mrtUser))
            {
            int aindex;

            mrsv_t *tR;
            mjob_t *tJ;

            /* NOTE:  preemptor cannot access exclusive user reservation -
                      add preemptee job resources to blocked rsv resources */

            /* NOTE:  user reservations will only block resources not currently allocated
                      to matching jobs.  A preemptor job which is exclusive must restore
                      to the user reservation's blocked resource list those resources consumed
                      by the target preemptee */

            MCResCopyBResAndAdjust(&tmpBR,&RE->BRes,&N->CRes);

            if (tmpBR.Procs == -1) /* -1 means all the procs are blocked - translate to all procs */
              tmpBR.Procs = N->CRes.Procs;

            /* J->RsvAList should only include jobs found on this node */

            for (aindex = 0;aindex < MMAX_PJOB;aindex++)
              {
              if (J->RsvAList[aindex] == NULL)
                break;

              if ((MNodeRsvFind(N,J->RsvAList[aindex],FALSE,&tR,&tJ) == SUCCESS) &&
                  (MRsvCheckJAccess(R,tJ,Overlap,NULL,FALSE,NULL,NULL,NULL,NULL) == TRUE))
                {
                /* exclusive user rsv is allowing preemptee to use its resources.
                   add preemptee resources to rsv's blocked resources to prevent use
                   by preemptor */

                /* NOTE:  must determine how many tasks overlap rsv,
                          for now at least consume entire node for dedicated rsvs,
                          otherwise 1 (FIXME) */
 
                if (R->DRes.Procs == -1)
                  {
                  MCResAdd(&tmpBR,&N->CRes,&R->DRes,N->CRes.Procs,FALSE);
                  }
                else
                  {
                  MCResAdd(&tmpBR,&tR->DRes,&tR->DRes,1,FALSE);
                  }
                } 
              }  /* END for (aindex) */

            BR = &tmpBR;
            }  /* END if ((J->RsvAList != NULL) && ...) */

          MCResRemove(&AvlRes,&N->CRes,BR,RC,FALSE);

          if (UseDed == TRUE)
            MCResAdd(&DedRes,&N->CRes,BR,RC,FALSE);
          }

        MDBO(7,J,fSCHED)
          {
          MLog("INFO:     removed resources: %s (x%d)\n",
            MCResToString(BR,0,mfmHuman,NULL),
            RC);

          MLog("INFO:     resulting resources: %s\n",
            MCResToString(&AvlRes,0,mfmHuman,NULL));
          }

        if (BRsvID != NULL)
          {
          if (NodePolicyActive == TRUE)
            {
            snprintf(BRsvID,MMAX_NAME,"%s/policy",
              R->Name);
            }
          else
            {
            strcpy(BRsvID,R->Name);
            }
          }
        }    /* END else (MRsvCheckJAccess(R,J,Overlap,&AC,NULL,Affinity,NULL) == TRUE) */
      }      /* END if (RE->Type == mreStart) */
    else if (RE->Type == mreEnd)
      {
      /* reservation end */

      /* if rsv is in exclusive tree (NYI) */

      if (ACLIsConditional == TRUE)
        {
        AC.QT = MAX(MSched.Time,R->StartTime) - J->SubmitTime;

        AC.XF = (AC.QT + JDuration) / (JDuration);
        }

      RsvAllowsJAccess = MRsvCheckJAccess(R,J,Overlap,&AC,FALSE,NULL,NULL,Affinity,NULL);

      RsvNotApplicable = (MJobRsvApplies(J,N,R,NAccessPolicy) == FALSE);

      /* The R->EndTime check here is for checking infinite reservations and 
          infinite jobs.  The section below where we decrement InclusiveRsvCount
          means that the job ends after the reservation, and so advres cannot 
          be fulfilled.  With an infinite reservation, that is not an issue,
          though the math will still say that the job ends after the 
          reservation -->
       (MSched.Time + J->SpecWCLimit (MMAX_TIME)) > (R->EndTime (MMAX_TIME - 1))
      */

      if (((RsvAllowsJAccess == TRUE) || (RsvNotApplicable == TRUE)) &&
          ((R->EndTime < MMAX_TIME - 1) || (bmisset(&J->Flags,mjfRsvMap))))
        {
        MDBO(8,J,fSCHED) MLog("INFO:     reservations are inclusive\n");

        InclusiveRsvCount--;

        if (InclusiveRsvCount == 0)
          InclusiveRsvMaxSize = 0;

        ResourcesRemoved = TRUE;
        }
      else
        {
        MDBO(8,J,fSCHED) MLog("INFO:     reservations are exclusive\n");

        tmpJ = R->J;

        if ((NodePolicyActive == TRUE) && (tmpJ != NULL))
          {
          if (RsvIsGhost == FALSE)
            {
            JC--;
            }

          /* NOTE:  must change: *PC,*PE must be based on per node allocation, not per job */

          if (J->Credential.C == tmpJ->Credential.C)
            {
            CPC -= tmpJ->TotalProcCount;
            }

          if (J->Credential.G == tmpJ->Credential.G)
            {
            GJC--;

            GPC -= tmpJ->TotalProcCount;
            }

          if (J->Credential.U == tmpJ->Credential.U)
            {
            UJC--;
 
            UPC -= tmpJ->TotalProcCount;
            }

          if (MPE > 0)
            {
            TPE -= __MNodeGetPEPerTask(&N->CRes,&tmpJ->Req[0]->DRes);
            }
          }    /* END if ((NodePolicyActive == TRUE) && (tmpJ != NULL)) */

        ResourcesAdded = TRUE;

        if ((NAccessPolicy == mnacSingleUser) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.U != J->Credential.U))
          {
          /* user dedicated resources added */

          MCResAdd(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResRemove(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else if ((NAccessPolicy == mnacSingleGroup) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.G != J->Credential.G))
          {
          /* group dedicated resources added */

          MCResAdd(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResRemove(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else if ((NAccessPolicy == mnacSingleAccount) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.A != J->Credential.A))
          {
          /* account dedicated resources added */

          MCResAdd(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResRemove(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else if ((NAccessPolicy == mnacUniqueUser) &&
            (tmpJ != NULL) &&
            (tmpJ->Credential.U == J->Credential.U))
          {
          /* user dedicated resources added */

          MCResAdd(&AvlRes,&N->CRes,&N->CRes,RC,FALSE);

          if (UseDed == TRUE)
            MCResRemove(&DedRes,&N->CRes,&N->CRes,RC,FALSE);
          }
        else
          {
          MCResAdd(&AvlRes,&N->CRes,BR,RC,FALSE);

          if (UseDed == TRUE)
            MCResRemove(&DedRes,&N->CRes,BR,RC,FALSE);
          }

        MDBO(7,J,fSCHED)
          {
          MLog("INFO:     added resources: %s (x%d)\n",
            MCResToString(BR,0,mfmHuman,NULL),
            RC);

          MLog("INFO:     resulting resources: %s\n",
            MCResToString(&AvlRes,0,mfmHuman,NULL));
          }
        }
      }    /* END else (RE->Type == mreStart) */

    MREGetNext(RE,&NextRE);

    /* verify resources once per timestamp */

    if ((NextRE == NULL) ||
        (RE->Time != NextRE->Time))
      {
      MULToTString(RE->Time - MSched.Time,TString);

      MDBO(8,J,fSCHED) MLog("INFO:     verifying resource access at %s\n",
        TString);

      /* NOTE:  If ghost rsv is in place, do not apply job policies */
      /* NOTE:  enforce job policies if job is ghost but not if rsv is ghost */

      if ((RsvIsGhost == FALSE) &&
         (((MJC > 0)  && (JC >= MJC))   ||
          ((MPE > 0)  && (TPE > MPE))   || 
          ((MGJC > 0) && (GJC >= MGJC)) ||
          ((MGPC > 0) && (GPC >= MGPC)) ||
          ((MCPC > 0) && (CPC >= MCPC)) ||
          ((MUJC > 0) && (UJC >= MUJC)) ||
          ((MUPC > 0) && (UPC >= MUPC))))
        {
        PolicyBlock = TRUE;
        }
      else
        {
        PolicyBlock = FALSE;
        }

      /* NOTE:  AdvResMatch currently only check procs (FIXME) */

      if ((MReqIsGlobalOnly(RQ)) ||
          !bmisset(&J->Flags,mjfAdvRsv) ||
          (bmisset(&J->IFlags,mjifNotAdvres)) ||
          ((InclusiveRsvCount > 0) && (J->Req[0]->DRes.Procs <= InclusiveRsvMaxSize)))
        {
        if (((!MReqIsGlobalOnly(RQ)) && bmisset(&J->Flags,mjfAdvRsv)) && 
            (RsvNotApplicable == TRUE))
          {
          /* the job may have access but this isn't the reservation we want */
          }
        else
          {
          AdvResMatch = TRUE;
          }
        }
      else
        {
        AdvResMatch = FALSE;
        }

      if ((R->StartTime == RRange[0].STime) && (PreRes == TRUE))
        {
        /* adjust/check requirements at job start */

        PreRes = FALSE;

        if (MCResIsNeg(&AvlRes,NULL,NULL) == TRUE)
          {
          MULToTString(RRange[0].STime - MSched.Time,TString);

          MDBO(7,J,fSCHED) MLog("INFO:     resources unavailable at time %s\n",
            TString);

          if (RE->Time <= (long)RRange[0].STime)
            ActiveRange = FALSE;

          if (RE->Time >= (long)RRange[0].ETime)
            {
            /* end of required range reached */

            break;
            }
          }
        else if (PolicyBlock == TRUE)
          {
          MULToTString(RRange[0].STime - MSched.Time,TString);

          MDBO(7,J,fSCHED) MLog("INFO:     policy blocks access at time %s\n",
            TString);

          if (RE->Time <= (long)RRange[0].STime)
            ActiveRange = FALSE;

          if (RE->Time >= (long)RRange[0].ETime)
            {
            /* end of required range reached */

            break;
            }
          }
        else if (AdvResMatch == TRUE)
          {
          /* range is active unless advance reservation is required and not found */

          ActiveRange = TRUE;

          TC = MNodeGetTC(N,&AvlRes,&N->CRes,&DedRes,&RQ->DRes,MSched.Time,1,NULL);

          if (InclusiveRsvMaxSize > 0)
            {
            /* for advrsv jobs, constrain available tasks */

            TC = MIN(
                   TC,
                   (InclusiveRsvMaxSize - RRange[0].TC * RQ->DRes.Procs) / MAX(1,RQ->DRes.Procs));
            }

          ARange[RangeIndex].TC = RRange[0].TC + TC;
          ARange[RangeIndex].NC = EffNC;
          ARange[RangeIndex].STime = MAX(RRange[0].STime,MSched.Time + RsvOffset);

          MDBO(8,J,fSCHED) MLog("INFO:     ARange[%d] started (TC: %d  S: %ld)\n",
            RangeIndex,
            ARange[RangeIndex].TC,
            ARange[RangeIndex].STime);

          if (DRes != NULL)
            MCResGetMin(DRes,DRes,&AvlRes);
          }
        }   /* END if ((R->StartTime >= RRange[0].STime) && ... */

      MDBO(8,J,fSCHED) MLog("INFO:     ResourcesRemoved=%s, ActiveRange=%s, PolicyBlock=%s, AdvResMatch=%s\n",
          MBool[ResourcesRemoved],
          MBool[ActiveRange],
          MBool[PolicyBlock],
          MBool[AdvResMatch]);

      if ((ResourcesRemoved == TRUE) && (ActiveRange == TRUE))
        {
        ResourcesRemoved = FALSE;
        ResourcesAdded   = FALSE;

        if ((MCResIsNeg(&AvlRes,NULL,NULL) == TRUE) ||
            (PolicyBlock == TRUE) ||
            (AdvResMatch == FALSE))
          {
          /* terminate active range */

          MULToTString(RE->Time - MSched.Time,TString);

          MDBO(6,J,fSCHED) MLog("INFO:     resources unavailable at time %s during reservation %s %s (%s)\n",
            TString,
            R->Name,
            (RE->Type == mreStart) ? "start" : "end",
            (PolicyBlock == TRUE) ? "policy" : "resources");

          if ((((long)RE->Time - (long)ARange[RangeIndex].STime) >= JDuration) ||
              ((RangeIndex > 0) && (ARange[RangeIndex].STime == ARange[RangeIndex - 1].ETime)))
            {
            /* if reservation is long enough */

            ARange[RangeIndex].ETime = RE->Time;

            MDBO(8,J,fSCHED)
              {
              MULToTString(ARange[RangeIndex].STime - MSched.Time,TString);

              strcpy(WCString,TString);

              MULToTString(ARange[RangeIndex].ETime - MSched.Time,TString);

              MLog("INFO:     closing ARange[%d] (%s -> %s : %d) (1)\n",
                RangeIndex,
                WCString,
                TString,
                ARange[RangeIndex].TC);
              }

            RangeIndex++;

            if (RangeIndex >= MMAX_RANGE)
              {
              MDBO(6,J,fSCHED) MLog("ALERT:    range overflow in %s(%s,%s)\n",
                FName,
                J->Name,
                N->Name);

              RoutineRC = FAILURE;
              goto free_and_return;
              }
            }
          else
            {
            MDBO(8,J,fSCHED) MLog("INFO:     range too short (ignoring)\n");
            }

          ActiveRange = FALSE;

          if ((RCount == RangeIndex) ||
              (RE->Time >= (long)RRange[0].ETime))
            {
            break;
            }
          }   /* END if (MCResIsNeg(&AvlRes,NULL,NULL) || ... */
        else
          {
          /* check new taskcount */

          TC = MNodeGetTC(N,&AvlRes,&N->CRes,&DedRes,&RQ->DRes,MSched.Time,1,NULL);

          if (RE->Time >= (long)RRange[0].STime)
            {
            /* if in 'active' time */

            if (((TC + RRange[0].TC) != ARange[RangeIndex].TC) &&
                ((RQ->TaskCount == 0) ||
                 (TC + RRange[0].TC < RQ->TaskCount) ||
                 (TC + RRange[0].TC < ARange[RangeIndex].TC) ||
                 (ARange[RangeIndex].TC < RQ->TaskCount)))
              {
              if (RE->Time > (long)ARange[RangeIndex].STime)
                {
                /* create new range */

                ARange[RangeIndex].ETime = RE->Time;

                MDBO(8,J,fSCHED)
                  {
                  MULToTString(ARange[RangeIndex].STime - MSched.Time,TString);

                  strcpy(WCString,TString);

                  MULToTString(ARange[RangeIndex].ETime - MSched.Time,TString);

                  MLog("INFO:     closing ARange[%d] (%s -> %s : %d) (2)\n",
                    RangeIndex,
                    WCString,
                    TString,
                    ARange[RangeIndex].TC);
                  }

                RangeIndex++;

                if (RangeIndex >= MMAX_RANGE)
                  {
                  MDBO(1,J,fSCHED) MLog("ALERT:    range overflow in %s(%s,%s)\n",
                    FName,
                    J->Name,
                    N->Name);

                  RoutineRC = FAILURE;
                  goto free_and_return;
                  }
                }    /* END if (RE->Time > ARange[RangeIndex].STime) */

              ARange[RangeIndex].STime = MAX(RE->Time,(long)MSched.Time + RsvOffset);
              ARange[RangeIndex].TC = RRange[0].TC + TC;
              ARange[RangeIndex].NC = EffNC;

              if (DRes != NULL)
                MCResGetMin(DRes,DRes,&AvlRes);
              }  /* END if (((TC + RRange[0].TC) != ARange[RangeIndex].TC) */
            }    /* END if (RE->Time >= RRange[0].STime) */
          else if ((RRange[0].TC + TC) < ARange[RangeIndex].TC)
            {
            /* in 'pre-active' time */

            MDBO(6,J,fSCHED) MLog("INFO:     adjusting 'preactive' ARange[%d] taskcount from %d to %d\n",
              RangeIndex,
              ARange[RangeIndex].TC,
              RRange[0].TC + TC);

            ARange[RangeIndex].TC = RRange[0].TC + TC;
            }
          else
            {
            MDBO(6,J,fSCHED) MLog("INFO:     ARange[%d] taskcount not affected by reservation change\n",
              RangeIndex);
            }
          }   /* END else (MCResIsNeg(&AvlRes,NULL,NULL) || ... */
        }
      else if ((ResourcesAdded == TRUE) && (ActiveRange == TRUE))
        {
        int NewRange = FALSE;

        /* adjust taskcount, create new range */

        TC = MNodeGetTC(N,&AvlRes,&N->CRes,&DedRes,&RQ->DRes,MSched.Time,1,NULL);

        if ((TC + RRange[0].TC) > ARange[RangeIndex].TC)
          {
          /* range has grown */

          NewRange = TRUE;
          }
        else if (((TC + RRange[0].TC) < ARange[RangeIndex].TC) &&
            ((RQ->TaskCount == 0) ||
             (TC + RRange[0].TC < RQ->TaskCount) ||
             (ARange[RangeIndex].TC < RQ->TaskCount)))
          {
          /* range is too small */

          NewRange = TRUE;
          }

        if (NewRange == TRUE)
          { 
          ARange[RangeIndex].ETime = RE->Time;

          MDBO(8,J,fSCHED)
            {
            MULToTString(ARange[RangeIndex].STime - MSched.Time,TString);

            strcpy(WCString,TString);

            MULToTString(ARange[RangeIndex].ETime - MSched.Time,TString);

            MLog("INFO:     closing ARange[%d] (%s -> %s : %d) (3)\n",
              RangeIndex,
              WCString,
              TString,
              ARange[RangeIndex].TC);
            }

          RangeIndex++;

          if (RangeIndex >= MMAX_RANGE)
            {
            MDBO(6,J,fSCHED) MLog("ALERT:    range overflow in %s(%s,%s)\n",
              FName,
              J->Name,
              N->Name);


            RoutineRC = FAILURE;
            goto free_and_return;
            }

          ARange[RangeIndex].STime = MAX(RE->Time,(long)MSched.Time + RsvOffset);

          ARange[RangeIndex].TC = RRange[0].TC + TC;

          ARange[RangeIndex].NC = EffNC;
          }  /* END if (((TC + RRange[0].TC) != ...) */
        }    /* END else if ((ResourcesAdded == TRUE) && (ActiveRange == TRUE)) */
      else if ((ResourcesAdded == TRUE) && (ActiveRange == FALSE))
        {
        ResourcesAdded   = FALSE;
        ResourcesRemoved = FALSE;

        if ((MCResIsNeg(&AvlRes,NULL,NULL) == FALSE) &&
            (AdvResMatch == TRUE) &&
            (PolicyBlock == FALSE))
          {
          /* initiate active range */

          MULToTString(RE->Time - MSched.Time,TString);

          MDBO(6,J,fSCHED) MLog("INFO:     resources available at time %s during %s %s\n",
            TString,
            R->Name,
            (RE->Type == mreStart) ? "start" : "end");

          ARange[RangeIndex].STime = 
            MAX(
              MAX((long)RRange[0].STime,RE->Time),
              (long)MSched.Time + RsvOffset);

          /* adjust taskcount */

          TC = MNodeGetTC(N,&AvlRes,&N->CRes,&DedRes,&RQ->DRes,MSched.Time,1,NULL);

          if (InclusiveRsvMaxSize > 0)
            {
            /* for advrsv jobs, constrain available tasks */

            TC = MIN(
                   TC,
                   (InclusiveRsvMaxSize - RRange[0].TC * RQ->DRes.Procs) / MAX(1,RQ->DRes.Procs));
            }

          ARange[RangeIndex].TC = RRange[0].TC + TC;

          ARange[RangeIndex].NC = EffNC;

          MDBO(8,J,fSCHED) MLog("INFO:     ARange[%d] adjusted (TC: %d  S: %ld)\n",
            RangeIndex,
            ARange[RangeIndex].TC,
            ARange[RangeIndex].STime);

          ActiveRange = TRUE;

          if (DRes != NULL)
            {
            MCResGetMin(DRes,DRes,&AvlRes);
            }
          }   /* END if (MCResIsNeg())                                */
        }     /* END else if (RE->Type == mreEnd)            */
      }       /* END if (RE->Time != NextRE->Time) */
    else
      {
      /* this is copied from above, but we skip the code above if 2 reservations start
         at the same time.  In that case we lose the fact that we are in an advres (MOAB-872) */

      if ((MReqIsGlobalOnly(RQ)) ||
          !bmisset(&J->Flags,mjfAdvRsv) ||
          (bmisset(&J->IFlags,mjifNotAdvres)) ||
          ((InclusiveRsvCount > 0) && (J->Req[0]->DRes.Procs <= InclusiveRsvMaxSize)))
        {
        if (((MReqIsGlobalOnly(RQ)) &&bmisset(&J->Flags,mjfAdvRsv)) && 
            (RsvNotApplicable == TRUE))
          {
          /* the job may have access but this isn't the reservation we want */
          }
        else
          {
          AdvResMatch = TRUE;
          }
        }
      else
        {
        AdvResMatch = FALSE;
        }
      }
    }         /* END for (MREGetNext)                                      */

  if (ActiveRange == TRUE)
    {
    ARange[RangeIndex].ETime = MMAX_TIME;

    MDBO(8,J,fSCHED)
      {
      MULToTString(ARange[RangeIndex].STime - MSched.Time,TString);

      strcpy(WCString,TString);

      MULToTString(ARange[RangeIndex].ETime - MSched.Time,TString);

      MLog("INFO:     closing ARange[%d] (%s -> %s : %d) (4)\n",
        RangeIndex,
        WCString,
        TString,
        ARange[RangeIndex].TC);
      }

    RangeIndex++;

    if (RangeIndex >= MMAX_RANGE)
      {
      MDBO(6,J,fSCHED) MLog("ALERT:    range overflow in %s(%s,%s)\n",
        FName,
        J->Name,
        N->Name);

      RoutineRC = FAILURE;
      goto free_and_return;
      }
    }  /* END if (ActiveRange == TRUE) */

  /* terminate range list */

  ARange[RangeIndex].ETime = 0;

  RangeIndex = 0;

  /* NOTE:  RRange,ARange are availability ranges, not start ranges */

  for (index = 0;ARange[index].ETime != 0;index++)
    {
    /* enforce time constraints */

    if (!bmisset(&J->Flags,mjfRsvMap) || !bmisset(&J->SpecFlags,mjfIgnState))
      {
      long StateDelayTime;

      MJobGetNodeStateDelay(J,RQ,N,&StateDelayTime);

      if (StateDelayTime > 0)
        {
        if (MSched.Time + StateDelayTime >= ARange[index].ETime)
          {
          /* no valid resources - terminate range */

          MDBO(7,J,fSCHED)
            {
            MLog("INFO:     node %s in state %s/%s - state delaytime=%ld - terminating range at index %d\n",
              N->Name,
              MNodeState[N->State],
              MNodeState[N->EState],
              StateDelayTime,
              index);
            }

          ARange[index].ETime = 0;
          }
        else
          {
          MDBO(7,J,fSCHED)
            {
            MLog("INFO:     node %s in state %s/%s - state delaytime=%ld - moving range start forward at index %d\n",
              N->Name,
              MNodeState[N->State],
              MNodeState[N->EState],
              StateDelayTime,
              index);
            }

          ARange[index].STime = MAX(
            ARange[index].STime,
            MSched.Time + StateDelayTime);
          }
        }
      else if (StateDelayTime < 0)
        {
        /* indefinite delay - no valid resources - terminate range */

        ARange[index].ETime = 0;
        }
      }    /* END if (!bmisset(&J->Flags,mjfRsvMap) || ...) */

    /* verify adjusted range is still valid */

    if (ARange[index].ETime < RRange[0].STime)
      {
      MDBO(6,J,fSCHED) MLog("INFO:     ARange[%d] too early for job %s (E: %ld < S: %ld  P: %ld): removing range\n",
        index,
        J->Name,
        ARange[index].ETime,
        RRange[0].STime,
        MSched.Time);

      ARange[index].TC = 0;
      }
    else if (ARange[index].STime < RRange[0].STime)
      {
      MDBO(6,J,fSCHED) MLog("INFO:     ARange[%d] too early for job %s (S: %ld < S: %ld): truncating range\n",
        index,
        J->Name,
        ARange[index].STime,
        RRange[0].STime);

      ARange[index].STime = RRange[0].STime;
      }

    if (ARange[index].TC < MAX(RQ->TasksPerNode,RRange[0].TC))
      {
      MDBO(6,J,fSCHED) MLog("INFO:     ARange[%d] too small for job %s (%d < %d):  removing range\n",
        index,
        J->Name,
        ARange[index].TC,
        RRange[0].TC);

      continue;
      }

    /* adequate range found */

    if (RQ->TasksPerNode > 0)
      {
      if (bmisset(&J->IFlags,mjifExactTPN))
        {
        ARange[index].TC = RQ->TasksPerNode;
        }
      else if (bmisset(&J->IFlags,mjifMaxTPN))
        {
        ARange[index].TC = MIN(ARange[index].TC,RQ->TasksPerNode);
        }
      } 

    if (index != RangeIndex)
      {
      memcpy(&ARange[RangeIndex],&ARange[index],sizeof(ARange[index]));
      }

    RangeIndex++;
    }    /* END for (index) */

  ARange[RangeIndex].ETime = 0;

  /* NOTE:  passing in JDuration may result in a delayed job start under certain
            conditions */

  if ((N->PtIndex > 0) && (MPar[N->PtIndex].ConfigNodes == 1))
    {
    /* partition only contains one node - all tasks must be available in each
       range */

    tmpTC = RQ->TaskCount;
    }
  else
    {
    /* partition contains multiple nodes - each range does not need to provide
       all tasks required by the job req */

    tmpTC = 0;
    }

  if ((MSched.RsvSearchAlgo == mrsaWide) || (bmisset(&J->Flags,mjfWideRsvSearchAlgo)))
    {
    /* merge availability ranges so as to get largest proc ranges */

    rc = MRLMergeTime(
        ARange,
        RRange,
        SeekLong,
        0L,
        tmpTC);
    }
  else
    {
    /* DEFAULT */

    /* merge availability ranges so as to get longest duration ranges */

    rc = MRLMergeTime(
        ARange,
        RRange,
        SeekLong,
        (long)JDuration,
        tmpTC);
    }

  if (rc == FAILURE)
    {
    MULToTString(RRange[0].STime - MSched.Time,TString);

    MDBO(6,J,fSCHED) MLog("INFO:     node %s unavailable for job %s at %s\n",
      N->Name,
      J->Name,
      TString);

    if (Affinity != NULL)
      *Affinity = mnmUnavailable;

    RoutineRC = FAILURE;
    goto free_and_return;
    }

  if ((!MReqIsGlobalOnly(RQ)) &&
      (J->ReqRID != NULL) && 
      (!bmisset(&J->IFlags,mjifNotAdvres)))
    {
    if ((Affinity != NULL) &&
       ((*Affinity != mnmPositiveAffinity) &&
        (*Affinity != mnmNeutralAffinity) &&
        (*Affinity != mnmNegativeAffinity)))
      {
      if (Affinity != NULL)
        *Affinity = mnmUnavailable;

      RoutineRC = FAILURE;
      goto free_and_return;
      }
    }

  /* this block copied above for instances where node has no rsv */

  if ((Affinity != NULL) && !bmisset(&J->Flags,mjfBestEffort))
    {
    if (bmisset(&J->IFlags,mjifHostList) && 
        (J->ReqHLMode != mhlmSuperset))
      {
      if (J->ReqHLMode != mhlmSubset)
        {
        *Affinity = mnmRequired;
        }
      else if (!MNLIsEmpty(&J->ReqHList))
        {
        /* FIXME: can mjifHostList be set and J->ReqHList NULL? */
        /*        RT4179 had this happen but couldn't reproduce */

        mnode_t *tmpN;

        int hindex;

        /* look for match */

        for (hindex = 0;MNLGetNodeAtIndex(&J->ReqHList,hindex,&tmpN) == SUCCESS;hindex++)
          {
          if (tmpN == N)
            {
            *Affinity = mnmRequired;

            break;
            }
          }    /* END for (hindex) */
        }
      }
    }          /* END if (Affinity != NULL) */

  MDBO(6,J,fSCHED)
    {
    for (RangeIndex = 0;ARange[RangeIndex].ETime != 0;RangeIndex++)
      {
      MULToTString(ARange[RangeIndex].STime - MSched.Time,TString);

      strcpy(WCString,TString);

      MULToTString(ARange[RangeIndex].ETime - ARange[RangeIndex].STime,TString);

      MLog("INFO:     node %s supports %d task%c of job %s:%d for %s at %s\n",
        N->Name,
        ARange[RangeIndex].TC,
        (ARange[RangeIndex].TC == 1) ? ' ' : 's',
        J->Name,
        RQ->Index,
        TString,
        WCString);
      }
    }

  /* add required resources to min resource available */

  if (DRes != NULL)
    {
    MCResAdd(DRes,&N->CRes,&RQ->DRes,RRange[0].TC,FALSE);
    }

  RoutineRC = SUCCESS;

free_and_return:

  MCResFree(&tmpBR);

  MCResFree(&DedRes);

  MCResFree(&DQRes);

  MCResFree(&AvlRes);

  return(RoutineRC);
  }  /* END MJobGetSNRange() */
