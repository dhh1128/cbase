/* HEADER */

      
/**
 * @file MNodePolicy.c
 *
 * Contains: Node Policy functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 *
 *
 * @param Cfg (I)
 * @param Req (I)
 */

int __MNodeGetPEPerTask(

  const mcres_t *Cfg,
  const mcres_t *Req)

  {
  double PE;

  PE = (double)Req->Procs / Cfg->Procs;

  if ((Req->Mem > 0) && (Cfg->Mem > 0))
    PE = MAX(PE,(double)Req->Mem  / Cfg->Mem);

  if ((Req->Disk > 0) && (Cfg->Disk > 0))
    PE = MAX(PE,(double)Req->Disk / Cfg->Disk);

  if ((Req->Swap > 0) && (Cfg->Swap > 0))
    PE = MAX(PE,(double)Req->Swap / Cfg->Swap);

  PE *= Cfg->Procs;

  return((int)PE);
  }  /* END MNodeGetPEPerTask() */



/**
 * Check node limits, per node credential limits, and node access policy.
 *
 * NOTE: If specified, TC should contain maximum number of tasks node
 *  can support
 *
 * NOTE:  only processes NAccessPolicy within Req[0]
 *
 * @param J (I)
 * @param N (I)
 * @param StartTime (I)
 * @param TC (I/O) [optional] I maximum resources available, O maximum resources allowed by policy
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MNodeCheckPolicies(

  const mjob_t  *J,
  const mnode_t *N,
  long           StartTime,
  int           *TC,
  char          *EMsg)

  {
  int     tindex;
  int     jindex;

  int     JPC;        /* per job proc count */
  int     PCDelta;
  double  tmpTJE;

  mbool_t IsDedicated;

  enum MNodeAccessPolicyEnum EffNAPolicy;

  /* global policies */

  int     TPE;
  int     TJE; /* max pe per job */
  int     TJC;
  int     TPC;

  int     MPE;  /* NOTE:  not enabled */
  int     MJE = 0; /* max pe per job */
  int     MJC;
  int     MPC;

  /* user policies */

  int     MUJC;
  int     MUPC;

  int     UJC;
  int     UPC;

  /* group policies */

  int     MGJC;
  int     MGPC;

  int     GJC;
  int     GPC;

  mjob_t *tmpJ;
  mreq_t *RQ;

  int     rqindex;

  const char *FName = "MNodeCheckPolicies";

  MDB(6,fSTRUCT) MLog("%s(%s,%s,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (N != NULL) ? N->Name : "NULL",
    (TC != NULL) ? *TC : -1);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) || (N == NULL) || (J->Req[0] == NULL))
    {
    MDB(3,fSTRUCT) MLog("ALERT:    parameters corrupt in %s\n",
      FName);

    if (TC != NULL)
      *TC = 0;

    return(FAILURE);
    }

  /* initialize global policies */

  if ((J->Credential.Q != NULL) && (bmisset(&J->Credential.Q->Flags,mqfDedicated)))
    {
    /* dedicated jobs are exempt from resource based node policies */

    IsDedicated = TRUE;
    }
  else
    {
    IsDedicated = FALSE;
    }
 
  if (N->AP.HLimit[mptMaxJob] <= 0)
    MJC = MSched.DefaultN.AP.HLimit[mptMaxJob];
  else
    MJC = N->AP.HLimit[mptMaxJob];

  if (IsDedicated == FALSE)
    {
    if (N->AP.HLimit[mptMaxProc] <= 0)
      {
      MPC = MSched.DefaultN.AP.HLimit[mptMaxProc];
      }
    else
      {
      MPC = N->AP.HLimit[mptMaxProc];
      }

    if (N->AP.HLimit[mptMaxPE] > 0)
      MPE = N->AP.HLimit[mptMaxPE];
    else
      MPE = MSched.DefaultN.AP.HLimit[mptMaxPE];
    }
  else
    {
    /* disable both MPE and MPC policies */

    MPC = 0;
    MPE = 0;
    }  /* END if (IsDedicated == FALSE) */

  if (N->NP == NULL)
    {
    if (MSched.DefaultN.NP == NULL)
      {
      /* no node limits */

      MUJC = 0;
      MUPC = 0;
      MGJC = 0;
      MGPC = 0;
      }
    else
      {
      /* only default node limits specified */

      MUJC = MSched.DefaultN.NP->MaxJobPerUser;
      MUPC = MSched.DefaultN.NP->MaxProcPerUser;
      MGJC = MSched.DefaultN.NP->MaxJobPerGroup;
      MGPC = MSched.DefaultN.NP->MaxProcPerGroup;
      }
    }
  else if (MSched.DefaultN.NP == NULL)
    {
    /* only node limits specified */

    MUJC = N->NP->MaxJobPerUser;
    MUPC = N->NP->MaxProcPerUser;
    MGJC = N->NP->MaxJobPerGroup;
    MGPC = N->NP->MaxProcPerGroup;
    }
  else
    {  
    /* both node and default node limits specified */

    /* initialize user policies */

    if (N->NP->MaxJobPerUser <= 0)
      MUJC = MSched.DefaultN.NP->MaxJobPerUser;
    else
      MUJC = N->NP->MaxJobPerUser;

    if (N->NP->MaxProcPerUser <= 0)
      MUPC = MSched.DefaultN.NP->MaxProcPerUser;
    else
      MUPC = N->NP->MaxProcPerUser;

    /* initialize group policies */

    if (N->NP->MaxJobPerGroup <= 0)
      MGJC = MSched.DefaultN.NP->MaxJobPerGroup;
    else
      MGJC = N->NP->MaxJobPerGroup;

    if (N->NP->MaxProcPerGroup <= 0)
      MGPC = MSched.DefaultN.NP->MaxProcPerGroup;
    else
      MGPC = N->NP->MaxProcPerGroup;
    }

  JPC = J->Req[0]->DRes.Procs;

    /* initialize job pe policy */
    if (N->NP && N->NP->MaxPEPerJob > 0)
      MJE = (int) N->NP->MaxPEPerJob;

  /* initialize usage */

  if (((MJC > 0) || (MPC > 0) || (MPE > 0) || 
       (MGJC > 0) || (MGPC > 0) || (MJE > 0) ||
       (MUJC > 0) || (MUPC > 0)) && 
       (StartTime <= (long)(MSched.Time + MCONST_HOURLEN)))
    {
    TJC  = JPC;
    TPC  = 1;

    TPE = 0;
    TJE = 0;
    tmpTJE = 0.0;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      if (MPE > 0)
        TPE += __MNodeGetPEPerTask(&N->CRes,&RQ->DRes);
      }  /* END for (rqindex) */

    UJC  = 1;
    UPC  = JPC;

    GJC  = 1;
    GPC  = JPC;

    for (jindex = 0;jindex < N->MaxJCount;jindex++)
      {
      if (N->JList[jindex] == NULL)
        break;

      if (N->JList[jindex] == (mjob_t *)1)
        continue;

      tmpJ = N->JList[jindex];

      /* how to properly address suspended/preempted workload? (ignore for now) */

      if ((MJOBISACTIVE(tmpJ) == FALSE) || (tmpJ->TaskMap == NULL))
        {
        continue;
        }

      TJC++;

      if (tmpJ->Credential.U == J->Credential.U)
        UJC++;

      if (tmpJ->Credential.G == J->Credential.G)
        GJC++;

      for (tindex = 0;tmpJ->TaskMap[tindex] != -1;tindex++)
        {
        if (tmpJ->TaskMap[tindex] != N->Index)
          continue;

        /* NOTE:  must identify which req task is associated with (NYI) */

        RQ = tmpJ->Req[0];

        /* FIXME:  must adjust for multi-req jobs */

        TPC += RQ->DRes.Procs;

        if (MPE > 0)
          TPE += __MNodeGetPEPerTask(&N->CRes,&RQ->DRes);

        if (tmpJ->Credential.U == J->Credential.U)
          UPC += RQ->DRes.Procs;

        if (tmpJ->Credential.G == J->Credential.G)
          GPC += RQ->DRes.Procs;
        }  /* END for (tindex) */
      }    /* END for (jindex) */

    /* job policies */
 
    if ((MJC > 0) && (TJC > MJC))
      {
      MDB(6,fSTRUCT) MLog("INFO:     job violates MaxJobPerNode policy on node %s\n",
        N->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"violates MaxJobPerNode policy");

      return(FAILURE);
      }

    /* check for processor equivalent violations */

    if ((MPE > 0) && (TPE > MPE))
      {
      MDB(6,fSTRUCT) MLog("INFO:     job violates MaxPEPerNode policy on node %s\n",
        N->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"violates MaxPEPerNode policy");

      return(FAILURE);
      }

    /* Get Job Processor Equivalent */

    if (MJE > 0)
      {
      MJobGetPE(J,&MPar[0],&tmpTJE);
      TJE = (int)tmpTJE;
      }

    /* check Job PE against Node PE */

    if ((MJE > 0) && (TJE > MJE))
      {
      MDB(6,fSTRUCT) MLog("INFO:     job violates MaxPEPerJob policy on node %s\n",
        N->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"violates MaxPEPerJob policy");

      return(FAILURE);
      }

    if ((MGJC > 0) && (GJC > MGJC))
      {
      MDB(6,fSTRUCT) MLog("INFO:     job violates MaxJobPerGroup policy on node %s\n",
        N->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"violates MaxJobPerGroup policy");

      return(FAILURE);
      }

    if ((MUJC > 0) && (UJC > MUJC))
      {
      MDB(6,fSTRUCT) MLog("INFO:     job violates MaxJobPerUser policy on node %s\n",
        N->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"violates MaxJobPerUser policy");

      return(FAILURE);
      }

    /* proc policies */

    if (MUPC > 0)
      {
      if (UPC > MUPC)
        {
        MDB(6,fSTRUCT) MLog("INFO:     job violates MaxProcPerUser policy on node %s\n",
          N->Name);

        if (EMsg != NULL)
          strcpy(EMsg,"violates MaxProcPerUser policy");

        return(FAILURE);
        }

      if ((TC != NULL) && (JPC > 0))
        {
        PCDelta = MUPC - UPC;

        *TC = MIN(*TC,1 + PCDelta / JPC);
        }
      }  /* END if (MUPC > 0) */

    if (MGPC > 0)
      {
      if (GPC > MGPC)
        {
        MDB(6,fSTRUCT) MLog("INFO:     job violates MaxProcPerGroup policy on node %s\n",
          N->Name);

        if (EMsg != NULL)
          strcpy(EMsg,"violates MaxProcPerGroup policy");

        return(FAILURE);
        }

      if ((TC != NULL) && (JPC > 0))
        {
        PCDelta = MGPC - GPC;

        *TC = MIN(*TC,1 + PCDelta / JPC);
        }
      }  /* END if (MGPC > 0) */

    if (MPC > 0)
      {
      if (TPC > MPC)
        {
        MDB(6,fSTRUCT) MLog("INFO:     job violates MaxProc policy on node %s\n",
          N->Name);

        if (EMsg != NULL)
          strcpy(EMsg,"violates MaxProc policy");

        return(FAILURE);
        }

      if ((TC != NULL) && (JPC > 0))
        {
        PCDelta = MPC - TPC;

        *TC = MIN(*TC,1 + PCDelta / JPC);
        }
      }  /* END if (MPC > 0) */
    }    /* END if ((MJC > 0) || (MUJC > 0) || (MUPC > 0) || ...) */

  if (StartTime <= (long)MSched.Time)
    {
    /* check state */

    if ((N->State != mnsIdle) && (N->State != mnsActive))
      {
      MDB(6,fSTRUCT) MLog("INFO:     node is in non-execution state %s\n",
        MNodeState[N->State]);

      if (EMsg != NULL)
        sprintf(EMsg,"node state is %s",
          MNodeState[N->State]);

      return(FAILURE);
      }
    }

  /* check node access policy */

  if (J->System != NULL)
    {
    /* system job - do not enforce node access policy */

    EffNAPolicy = mnacNONE;
    }
  else
    {
    MNodeGetEffNAPolicy(
      N,
      (N->EffNAPolicy != mnacNONE) ? N->EffNAPolicy : N->SpecNAPolicy,
      MPar[N->PtIndex].NAccessPolicy,
      J->Req[0]->NAccessPolicy,
      &EffNAPolicy);
    }

  if (MNodePolicyAllowsJAccess(N,J,StartTime,EffNAPolicy,EMsg) == FALSE)
    return(FAILURE);

  if (TC != NULL)
    {
    double MaxPEPerJob;

    /* check effect of node task policies on TC */

    switch (EffNAPolicy)
      {
      case mnacSingleTask:

        *TC = 1;

        break;

      case mnacSingleJob:

        /* NOTE:  code below should be moved here (NYI) */

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (EffNAPolicy) */

    RQ = J->Req[0];  /* NOTE:  only process primary req for PE (FIXME) */

    if ((IsDedicated == FALSE) &&
        (RQ->NAccessPolicy == mnacSingleJob))
      {
      /* OUCH Determine most constraining NA policy (NYI) */

      if ((N->NP != NULL) && (N->NP->MaxPEPerJob > 0.0))
        MaxPEPerJob = N->NP->MaxPEPerJob;
      else if (MSched.DefaultN.NP != NULL) 
        MaxPEPerJob = MSched.DefaultN.NP->MaxPEPerJob;
      else
        MaxPEPerJob = 0.0;

      if (MaxPEPerJob > 0.0)
        {
        int PE;
        int tmpI;

        /* NOTE:  only process primary req */

        tmpI = *TC;

        PE = __MNodeGetPEPerTask(&N->CRes,&RQ->DRes);

        if (MaxPEPerJob > 1.0)
          *TC = MIN((int)MaxPEPerJob,tmpI * PE);
        else
          *TC = MIN((int)MaxPEPerJob * N->CRes.Procs,tmpI * PE);
        }  /* END if (MaxPEPerJob) */
      }    /* END if ((IsDedicated == FALSE) && (RQ->NAccessPolicy == mnacSingleJob)) */

    if (*TC <= 0)
      {
      MDB(6,fSTRUCT) MLog("INFO:     inadequate tasks on node %s in %s\n",
        N->Name,
        FName);

      if (EMsg != NULL)
        strcpy(EMsg,"inadequate tasks");

      return(FAILURE);
      }
    }  /* END if (TC != NULL) */

  return(SUCCESS);
  }  /* END MNodeCheckPolicies() */




 
/**
 * Get effective node access policy.
 *
 * NOTE:  incorporate active jobs, node policy, partition policy, and requestor job policy.
 *
 * @param N (I)
 * @param NodeEffNAP (I)
 * @param ParNAP (I)
 * @param JobNAP (I)
 * @param OP (O)
 */

int MNodeGetEffNAPolicy(

  const mnode_t              *N,
  enum MNodeAccessPolicyEnum  NodeEffNAP,
  enum MNodeAccessPolicyEnum  ParNAP,
  enum MNodeAccessPolicyEnum  JobNAP,
  enum MNodeAccessPolicyEnum *OP)

  {
  const char *FName = "MNodeGetEffNAPolicy";

  MDB(6,fSTRUCT) MLog("%s(%s,%s,%s,OP)\n",
    FName,
    MNAccessPolicy[NodeEffNAP],
    MNAccessPolicy[ParNAP],
    MNAccessPolicy[JobNAP]);

  if (OP == NULL)
    {
    return(FAILURE);
    }

  if ((N != NULL) && (N->PtIndex == MSched.SharedPtIndex))
    {
    /* handle SHARED nodes differently */

    MDB(7,fRM) MLog("INFO:     shared partition node access is always SHARED\n");

    *OP = mnacShared;

    return(SUCCESS);
    }

  if (ParNAP != mnacNONE)
    {
    *OP = (NodeEffNAP != mnacNONE) ? NodeEffNAP : ParNAP;
    }
  else
    {
    MDB(7,fRM) MLog("INFO:     partition node access policy not set, using node or scheduler policy\n");

    *OP = (NodeEffNAP != mnacNONE) ? NodeEffNAP : MSched.DefaultNAccessPolicy;
    }

  if (JobNAP == mnacNONE)
    {
    MDB(7,fRM) MLog("INFO:     job node access policy not set, effective node access policy %s\n",
      MNAccessPolicy[*OP]);

    return(SUCCESS);
    }

  switch (*OP)
    {
    case mnacNONE:
    case mnacShared:

      *OP = JobNAP;

      break;

    case mnacSharedOnly:

      *OP = NodeEffNAP;

      break;

    case mnacSingleUser:
    case mnacUniqueUser:
    case mnacSingleGroup:
    case mnacSingleAccount:

      if (JobNAP != mnacShared)
        *OP = JobNAP;

      break;

    case mnacSingleJob:

      if ((JobNAP != mnacShared) && 
          (JobNAP != mnacSingleUser) &&
          (JobNAP != mnacSingleGroup) &&
          (JobNAP != mnacSingleAccount))
        *OP = JobNAP;

      break;

    case mnacSingleTask:
    default:

      /* already maximally constrained */

      /* NO-OP */

      break;
    }  /* END switch (N->EffNAPolicy) */

  MDB(7,fRM) MLog("INFO:     node %s has effective node access policy %s\n",
    (N != NULL) ? N->Name : "",
    MNAccessPolicy[*OP]);

  return(SUCCESS);
  }   /* END MNodeGetEffNAPolicy() */




/**
 * Set effective node access policy.
 *
 * @param N (I)
 */

int MNodeSetEffNAPolicy(

  mnode_t *N)  /* I (modified) */

  {

  int jindex;

  enum MNodeAccessPolicyEnum JNAPolicy;

  /* walk thru all jobs and determine effective NA policy */

  if (N->SpecNAPolicy != mnacNONE)
    N->EffNAPolicy = N->SpecNAPolicy;
  else if (MPar[N->PtIndex].NAccessPolicy != mnacNONE)
    N->EffNAPolicy = MPar[N->PtIndex].NAccessPolicy;
  else
    N->EffNAPolicy = MSched.DefaultNAccessPolicy;

  if ((N->JList != NULL) && (N->MaxJCount > 0))
    {
    for (jindex = 0;jindex < N->MaxJCount;jindex++)
      {
      if (N->JList[jindex] == NULL)
       break;

      if (N->JList[jindex] == (mjob_t *)1)
        continue;

      JNAPolicy = N->JList[jindex]->Req[0]->NAccessPolicy;

      if (JNAPolicy == mnacNONE)
        continue;

      MNodeGetEffNAPolicy(
        N,
        N->EffNAPolicy,
        MPar[N->PtIndex].NAccessPolicy,
        JNAPolicy,
        &N->EffNAPolicy);
      }    /* END for (jindex) */
    }      /* END if ((N->JList != NULL) && (N->MaxJCount > 0)) */

  MDB(7,fRM) MLog("INFO:     node '%s' effective node access policy %s)\n",
    N->Name,
    MNAccessPolicy[N->EffNAPolicy]);

  return(SUCCESS);
  }  /* END MNodeSetEffNAPolicy() */





/**
 * @see MNodeGetRMState() - peer
 * @see MRMClusterQuery() - parent
 *
 * @param N (I)
 * @param EStateP (O) 
 */

int MNodeApplyStatePolicy(

  mnode_t             *N,        /* I */
  enum MNodeStateEnum *EStateP)  /* O */

  {
  int rmindex;

  enum MNodeStateEnum         RState;
  enum MRMNodeStatePolicyEnum NSPolicy;

  mrm_t *R;

  /* synchronize state information */

  *EStateP = mnsNONE;

  /* find the first specified node state */

  for (rmindex = 0; rmindex < MSched.M[mxoRM];rmindex++)
    {
    /* NOTE:  state reported by a provisioning RM should be overridden
              by a non-provisioning compute RM, ie handle following cases:

        Prov (MSM-xCAT+native job)     Compute (TORQUE)          Node State
        --------------------------     -----------------------   ----------
        idle                           down                      down
        idle                           idle                      idle
        active                         ---                       active
        ...
    */

    /* NYI - process for now (FIXME) */

    if (bmisset(&MRM[rmindex].RTypes,mrmrtProvisioning))
      { 
      /* don't ignore information from ProvRM, but keep looking */
      /* this information is needed for certain HVs whose information is only
         reported by MSM */

      *EStateP = N->RMState[rmindex];

      continue;
      }

    if (N->RMState[rmindex] != mnsNONE) 
      {
      *EStateP = N->RMState[rmindex];

      break;
      }
    }

  /* compare the first specified state with other reported states */

  for (rmindex++;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    /* ignore state reported by a provisioning MSM RM */

    if (bmisset(&R->RTypes,mrmrtProvisioning))
      continue;

    RState = N->RMState[rmindex];

    if ((RState == mnsNONE) || (RState == *EStateP))
      {
      /* resource manager provides redundant or no state information */

      continue; 
      }
  
      /* conflicting node state information given. Identify effective state */

    if (R->NodeStatePolicy != mrnspNONE)
      NSPolicy = R->NodeStatePolicy;
    else if (bmisset(&N->IFlags,mnifMultiComputeRM))
      NSPolicy = mrnspOptimistic;
    else
      NSPolicy = mrnspPessimistic;

    switch (NSPolicy)
      {
      case mrnspOptimistic:

        /* effective state is best of available states */
   
        if (RState == mnsIdle)
          *EStateP = RState;
        else if (MNSISACTIVE(RState) && (*EStateP != mnsIdle))
          *EStateP = RState;
        else if (MNSISUP(RState) && !MNSISUP(*EStateP))
          *EStateP = RState;
 
        break;

      case mrnspPessimistic:
      default:

        /* effective state is worst of available states */

        if (*EStateP == mnsIdle)
          *EStateP = RState;
        else if (MNSISACTIVE(*EStateP) && (RState != mnsIdle))
          *EStateP = RState;
        else if (MNSISUP(*EStateP) && !MNSISUP(RState))
          *EStateP = RState;

        break;
      }  /* END switch (R->NodeStatePolicy) */
    }    /* END for (rmindex) */

  if ((!MNSISUP(*EStateP)) &&
      (MNodeGetSysJob(N,msjtOSProvision,TRUE,NULL) == SUCCESS))
    {
    /* NOTE: BIG CHANGE!!!!
             7/29/09 moving towards sharing nodes while they are being provisioned
             node must remain "UP" while provisioning jobs are running so that
             additional jobs can be scheduled onto this node */
 
    *EStateP = mnsActive;
    }

  if (*EStateP == mnsUp)
    {
    /* Moab doesn't need to understand the "up" state.  Moab will mask this as "idle"
       and the other routines will adjust to busy/active if necessary */
    /* @see MNodeAdjustAvailResources */

    *EStateP = mnsIdle;
    }

  return(SUCCESS);
  }  /* END MNodeApplyStatePolicy() */




/**
 *
 * @param N         (I)
 * @param J         (I)
 * @param StartTime (I)
 * @param NAPolicy  (I)
 * @param EMsg      (I)
 */

mbool_t MNodePolicyAllowsJAccess(

  const mnode_t             *N,
  const mjob_t              *J,
  mulong                     StartTime,
  enum MNodeAccessPolicyEnum NAPolicy,
  char                      *EMsg)

  {
  mjob_t *tmpJ;
  int jindex;

  switch (NAPolicy)
    {
    case mnacNONE:
    case mnacShared:

      /* NO-OP */

      break;

    case mnacSharedOnly:

      if ((J->Req[0]->NAccessPolicy != mnacNONE) &&
          (J->Req[0]->NAccessPolicy != mnacShared) &&
          (J->Req[0]->NAccessPolicy != mnacSharedOnly))
        {
        MDB(6,fSTRUCT) MLog("INFO:     node only allows shared jobs (non-shared allocation requested)\n");

        if (EMsg != NULL)
          strcpy(EMsg,"shared nodeaccesspolicy");

        return(FALSE);
        }

      break;

    case mnacSingleUser:

      if (StartTime > (mulong)(MSched.Time + MCONST_HOURLEN))
        break;

      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        {
        tmpJ = N->JList[jindex];

        if (tmpJ == NULL)
          break;

        if (tmpJ == (mjob_t *)1)
          continue;

        if (tmpJ == J)
          continue;

        if (tmpJ->Credential.U != J->Credential.U)
          {
          if (EMsg != NULL)
            strcpy(EMsg,"singleuser nodeaccesspolicy");

          MDB(3,fSTRUCT) MLog("INFO:     node running job (singleuser allocation)\n");

          return(FALSE);
          }
        }   /* END for (jindex) */

      break;

    case mnacUniqueUser:

      if (StartTime > (mulong)(MSched.Time + MCONST_HOURLEN))
        break;

      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        {
        tmpJ = N->JList[jindex];

        if (tmpJ == NULL)
          break;

        if (tmpJ == (mjob_t *)1)
          continue;

        if (tmpJ == J)
          continue;

        if (tmpJ->Credential.U == J->Credential.U)
          {
          MDB(3,fSTRUCT) MLog("INFO:     node running job (uniqueuser allocation)\n");

          if (EMsg != NULL)
            strcpy(EMsg,"uniqueuser nodeaccesspolicy");

          return(FALSE);
          }
        }   /* END for (jindex) */

      break;

    case mnacSingleJob:

      if (StartTime > (mulong)(MSched.Time + MCONST_HOURLEN))
        {
        /* ignore single job policy if looking more than one hour in the future */

        break;
        }

      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        {
        tmpJ = N->JList[jindex];

        if (tmpJ == NULL)
          break;

        if (tmpJ == (mjob_t *)1)
          continue;

        if (tmpJ == J)
          continue;

        if (tmpJ == J->ActualJ)
          continue;

        if (MJOBISACTIVE(tmpJ) == FALSE)
          continue;

        MDB(3,fSTRUCT) MLog("INFO:     node running job (singlejob allocation)\n");

        if (EMsg != NULL)
          strcpy(EMsg,"singlejob nodeaccesspolicy");

        return(FALSE);
        }   /* END for (jindex) */

      break;

    case mnacSingleTask:

      if (StartTime > (mulong)(MSched.Time + MCONST_HOURLEN))
        break;

      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        {
        tmpJ = N->JList[jindex];

        if (tmpJ == NULL)
          break;

        if (tmpJ == (mjob_t *)1)
          continue;

        if (tmpJ == J)
          continue;

        if (MJOBISACTIVE(tmpJ) == FALSE)
          continue;

        MDB(3,fSTRUCT) MLog("INFO:     node running job (singletask allocation)\n");

        if (EMsg != NULL)
          strcpy(EMsg,"singletask nodeaccesspolicy");

        return(FALSE);
        }   /* END for (jindex) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (EffNAPolicy) */

  return(TRUE);
  }  /* END MNodePolicyAllowsJAccess() */


