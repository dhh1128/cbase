/* HEADER */

      
/**
 * @file MNodePost.c
 *
 * Contains: Node Post Load and Update functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Perform initialization on newly discovered nodes after
 * all RM's have reported.
 *
 * @param N (I) [modified]
 *
 * @see MRMNodePostLoad() - peer
 * @see MNodePostUpdate() - peer
 */

int MNodePostLoad(

  mnode_t *N)  /* I (modified) */

  {
  const char *FName = "MNodePostLoad";
 
  MDB(7,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  if ((MPar[0].NAvailPolicy[0] == mrapUtilized) ||
      (MPar[0].NAvailPolicy[0] == mrapCombined))
    {
    /* use utilized resource info */

    if ((MSched.MaxOSCPULoad > 0.0) &&
        (N->DRes.Procs == 0) &&
        (N->ARes.Procs < N->CRes.Procs))
      {
      N->ARes.Procs = MAX(0,N->ARes.Procs - (int)(MSched.MaxOSCPULoad * N->CRes.Procs));
      }
    }  /* END if (((MPar[0].NAvailPolicy[pindex] == mrapUtilized) ... */

  /* restore checkpoint node information */

  MCPRestore(mcpNode,N->Name,(void *)N,NULL);

  /* Setting default triggers must be after adding the node to the hash 
     table and loading in the checkpoint file, or it will not work */

  MNodeSetDefaultTriggers(N);

  MNodeLoadConfig(N,NULL);

  /* connect to existing classes/reservations */

  /* each RM may provide partial resource info, so must wait         */
  /* until last node RM is loaded before adding node to rsv or class */
  /* NOTE:  adding to class requires N->ARes.Procs to be populated   */

  /* Don't check if node is in reservation at startup. Previously, as each
   * node came online, it was checked to see if it existed in a reservation.
   * This was time consuming and slow. Now, at startup, once all nodes are 
   * loaded, each reservation is configured in MRMUpdate(). If a node comes
   * online after startup then MNodeUpdateResExpression will be called. */

  if ((bmisset(&MSched.Flags,mschedfFastRsvStartup) == FALSE) || 
      (MSched.Iteration != 0))
    MNodeUpdateResExpression(N,TRUE,TRUE); 

  if (N->ProcSpeed > 0)
    MSched.ReferenceProcSpeed = MIN(MSched.ReferenceProcSpeed,N->ProcSpeed);

  if (N->Speed <= 0.0)
    {
    if (MSched.DefaultN.Speed != 0.0)
      N->Speed = MSched.DefaultN.Speed;
    else
      N->Speed = 1.0;
    }

  MNodeGetLocation(N);

  MNodeUpdateStateFromVMs(N);

  MNodeSetAdaptiveState(N);

  if ((N->RMList != NULL) && (N->RMList[0] != NULL))
    {
    int rindex1;
    int rindex2;

    /* search all RM's, see if N->RMList[0] is master RM for other RM's */

    for (rindex1 = 0;rindex1 < MSched.M[mxoRM];rindex1++)
      {
      if (MRM[rindex1].Name[0] == '\0')
        break;

      if (MRMIsReal(&MRM[rindex1]) == FALSE)
        continue;

      if (MRM[rindex1].MRM != N->RMList[0])
        continue;

      /* node's master RM requires slave report from MRM[rindex1] */

      /* verify node has record from required slave */

      /* NOTE:  node RM state for RM which does not report must be set to NONE.
                Otherwise, after first iteration, node state integration policies apply 
                and node without required slave RM may migrate from state down to idle.  */

      for (rindex2 = 1;rindex2 < MSched.M[mxoRM];rindex2++)
        {
        if (N->RMList[rindex2] == NULL)
          break;

        if (N->RMList[rindex2] == &MRM[rindex1])
          break;
        }  /* END for (rindex2) */

      if (N->RMList[rindex2] != &MRM[rindex1])
        {
        char tmpLine[MMAX_LINE];

        /* required report not received */

        MNodeAddRM(N,&MRM[rindex1]);

        N->RMState[rindex1] = mnsNONE;

        MNodeSetState(N,mnsDown,FALSE);

        sprintf(tmpLine,"no report received from rm %s",
          MRM[rindex1].Name);

        MMBAdd(&N->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }
      else if (N->RMState[rindex1] == mnsNONE)
        {
        char tmpLine[MMAX_LINE];

        MNodeSetState(N,mnsDown,FALSE);

        sprintf(tmpLine,"no report received from rm %s",
          MRM[rindex1].Name);

        MMBAdd(&N->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }
      }    /* END for (rindex1) */
    }      /* END if ((N->RMList != NULL) && (N->RMList[0] != NULL)) */

  MNodeSetEffNAPolicy(N);

  if (bmisset(&N->IFlags,mnifIsNew))
    {
    MNodeUpdateOperations(N);

    bmunset(&N->IFlags,mnifIsNew);
    }

  MSched.TotalSystemProcs += N->CRes.Procs;
  MSchedUpdateCfgRes(N);

  MNodeUpdateAvailableGRes(N);

  MNodeTransition(N);

  return(SUCCESS);
  }  /* END MNodePostLoad() */


#define MDEF_NODEREWIGGLEROOM 4

#define MCONST_FAILINTERVAL 900  /* 15 minutes */

/**
 * Perform updates on existing nodes after all RM's have reported.
 *
 * @see MRMNodePostUpdate() - perform node post update functions once per RM
 * @see MNodePostLoad() - peer
 *
 * @param N (I) [modified]
 */

int MNodePostUpdate(

  mnode_t *N)  /* I (modified) */

  {
  const char *FName = "MNodePostUpdate";
 
  MDB(7,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  if ((N->RMList != NULL) && (N->RMList[0] != NULL))
    {
    int rindex1;
    int rindex2;

    /* search all RM's, see if N->RMList[0] is master RM for other RM's */

    for (rindex1 = 0;rindex1 < MSched.M[mxoRM];rindex1++)
      {
      if (MRM[rindex1].Name[0] == '\0')
        break;

      if (MRMIsReal(&MRM[rindex1]) == FALSE)
        continue;

      if (MRM[rindex1].MRM != N->RMList[0])
        continue;

      /* node's master rm requires slave report from MRM[rindex1] */

      /* verify node has record from required slave */

      for (rindex2 = 1;rindex2 < MMAX_RM;rindex2++)
        {
        if (N->RMList[rindex2] == NULL)
          break;

        if (N->RMList[rindex2] == &MRM[rindex1])
          break;
        }  /* END for (rindex2) */

      if (N->RMState[rindex1] == mnsNONE)
        {
        char tmpLine[MMAX_LINE];

        MNodeSetState(N,mnsDown,FALSE);

        sprintf(tmpLine,"no report received from rm %s",
          MRM[rindex1].Name);

        MMBAdd(&N->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }
      }    /* END for (rindex1) */
    }      /* END if ((N->RMList != NULL) && (N->RMList[0] != NULL)) */

  if ((MPar[0].NAvailPolicy[0] == mrapUtilized) ||
      (MPar[0].NAvailPolicy[0] == mrapCombined))
    {
    /* use utilized resource info */

    if ((MSched.MaxOSCPULoad > 0.0) &&
        (N->DRes.Procs == 0) &&
        (N->ARes.Procs < N->CRes.Procs))
      {
      N->ARes.Procs = MAX(0,N->ARes.Procs - (int)(MSched.MaxOSCPULoad * N->CRes.Procs));
      }
    }  /* END if (((MPar[0].NAvailPolicy[pindex] == mrapUtilized) ... */

  if (N->NP != NULL)
    {
    if ((N->NP->PreemptMaxCPULoad != 0.0) && (N->Load >= N->NP->PreemptMaxCPULoad))
      {
      MNodePreemptJobs(
        N,
        "node cpu load too high",
        N->NP->PreemptPolicy,
        TRUE);
      }
    /* NYI: implement preemption based on any generic metric */
    else if ((N->NP->PreemptMinMemAvail != 0.0) && 
             (N->ARes.Mem <= N->NP->PreemptMinMemAvail))
      {
      MNodePreemptJobs(
        N,
        "node memory too low",
        N->NP->PreemptPolicy,
        TRUE);
      }
    }    /* END if (N->NP != NULL) */

  /* if node RE/R table is full, node is not available */

  if (bmisset(&N->IFlags,mnifRTableFull))
    MNodeSetState(N,mnsDrained,FALSE);

  MNodeUpdateStateFromVMs(N);

  MNodeSetAdaptiveState(N);

  /* handle node state changes */

  if (N->State != N->OldState)
    {
    MDB(5,fRM) MLog("INFO:     node '%s' changed states from %s to %s\n",
      N->Name,
      MNodeState[N->OldState],
      MNodeState[N->State]);

    MSched.EnvChanged = TRUE;

    N->StateMTime = MSched.Time;

    /* handle <ANYSTATE> to 'Down/Drain/Flush' state transitions */

    if (!MNODEISUP(N))
      {
      N->EState       = N->State;
      N->SyncDeadLine = MMAX_TIME;

      /* NOTE: Triggers on default node are propogated to individual nodes  */

      if (MNSISUP(N->OldState))
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"changed states from %s to %s",
          MNodeState[N->OldState],
          MNodeState[N->State]);

        MOWriteEvent((void *)N,mxoNode,mrelNodeDown,tmpLine,MStat.eventfp,NULL);

        MOReportEvent((void *)N,N->Name,mxoNode,mttFailure,MSched.Time,TRUE);
        }
      }

    /* handle Down/Drain/Flush state to <any state> transitions */

    if (!MNSISUP(N->OldState))
      {
      if (MNODEISUP(N) == TRUE)
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"changed states from %s to %s",
          MNodeState[N->OldState],
          MNodeState[N->State]);

        MOWriteEvent((void *)N,mxoNode,mrelNodeUp,tmpLine,MStat.eventfp,NULL);
        }
 
      N->EState       = N->State;
      N->SyncDeadLine = MMAX_TIME;
      }  /* END if (!MNSISUP(N->OldState)) */

    if ((N->OldState == mnsIdle) && (N->State == mnsBusy))
      {
      if ((MSched.Mode != msmMonitor) && (MSched.Mode != msmTest))
        {
        MDB(1,fRM) MLog("ALERT:    unexpected node transition on node '%s'  Idle -> Busy\n",
          N->Name);
        }

      N->EState       = N->State;
      N->SyncDeadLine = MMAX_TIME;
      }
    }   /* END if (N->State != N->OldState) */

  if (MSched.FStatInitTime == 0)
    MSched.FStatInitTime = MSched.Time;

  if (N->Stat.FCurrentTime == 0)
    {
    memset(N->Stat.FCount,0,sizeof(N->Stat.FCount));

    N->Stat.FCurrentTime = MSched.Time - (MSched.Time % MCONST_FAILINTERVAL);
    }

  /* may require multiple roll's during this iteration to synchronize */

  while (MSched.Time >= N->Stat.FCurrentTime + MCONST_FAILINTERVAL)
    {
    /* roll failure stats */

    mulong TInterval;

    int tindex;
    int TMax;

    /* determine interval scope */

    TInterval = MSched.FStatInitTime - N->Stat.FCurrentTime / MCONST_FAILINTERVAL;

    for (TMax = 0;TMax < MMAX_BCAT;TMax++)
      {
      if (TInterval % (1 << TMax))
        break;
      }

    /* limit TMax to a maximum of MMAX_BCAT - 1 */
    if (TMax >= MMAX_BCAT)
      TMax = MMAX_BCAT - 1;

    /* roll current stats to prev stats */

    for (tindex = TMax;tindex >= 0;tindex--)
      {
      N->Stat.OFCount[tindex] = N->Stat.FCount[tindex];
      N->Stat.FCount[tindex] = 0;
      }

    N->Stat.FCurrentTime += MCONST_FAILINTERVAL;
    }    /* END while (MSched.Time >= N->Stat.CurrentTime + MCONST_FAILINTERVAL) */

  if ((N->OldState != mnsDown) && (N->State == mnsDown))
    {
    int tindex;

    /* update all current failure slots */

    for (tindex = 0;tindex < MMAX_BCAT;tindex++)
      N->Stat.FCount[tindex]++;
    }

  /* handle 'down to non-down' state transitions */

  if (((N->OldState == mnsDown) || (N->OldState == mnsDrained)) && 
       (N->State != N->OldState))
    {
    /* adjust class/rsv access */

    MNodeUpdateResExpression(N,TRUE,FALSE);
    }

  if (((N->OldState == mnsBusy) || (N->OldState == mnsActive)) && 
       (N->State == mnsIdle))
    {
    MOReportEvent((void *)N,N->Name,mxoNode,mttEnd,MSched.Time,TRUE);
    }

  /* check internal triggers' status */

  if (N->T != NULL)
    MSchedCheckTriggers(N->T,-1,NULL);

  /* update/clear hypervisor resource utilization information */

  switch (N->HVType)
    {
    case mhvtESX:
    case mhvtESX35:
    case mhvtESX35i:
    case mhvtESX40:
    case mhvtESX40i:

      if (MSched.VMCalculateLoadByVMSum)
        N->Load = 0.0;  /* reset load if we are going to use VM summation as load */

      N->ARes.Procs = MNodeGetBaseValue(N,mrlProc);

      break;

    default:

      /* NO-OP */

      break;
    }

  MNodeUpdateAvailableGRes(N);

  MNodeSetEffNAPolicy(N);

  MNodeTransition(N);

  return(SUCCESS);
  }  /* END MNodePostUpdate() */
/* END MNodePost.c */
