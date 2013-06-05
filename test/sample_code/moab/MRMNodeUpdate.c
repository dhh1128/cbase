/* HEADER */

      
/**
 * @file MRMNodeUpdate.c
 *
 * Contains: RM Node Update functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Perform common RM node update services.
 *
 * NOTE:  called once per iteration per native RM
 *
 * @see MRMNodePostLoad() - peer
 * @see MRMNodePostUpdate() - child - update N->EffNAPolicy
 * @see MNodePostUpdate() - peer - called once per iteration AFTER all RM's have reported
 *
 * @param N (I) [modified]
 * @param R (I) [optional]
 */

int MRMNodePostUpdate(

  mnode_t *N, /* I (modified) */
  mrm_t   *R) /* I (optional) */

  {
  const char *FName = "MRMNodePostUpdate";

  MDB(5,fRM) MLog("%s(%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  N->ATime = MSched.Time;

  /* NOTE:  assume modification on each iteration */

  N->MTime = MSched.Time;
  
  /* set default values */
  
  MRMAdjustNodeRes(N);

  if (N->CRes.Procs == -1)
    N->CRes.Procs = 1;

  if (N->RM != NULL)
    {
    if ((R != NULL) && (R != N->RM)) 
      {
      MNodeAddRM(N,R);

      if (bmisset(&R->Flags,mrmfBecomeMaster) && 
         !bmisset(&N->RM->Flags,mrmfBecomeMaster))
        {
        /* re-parent node */

        N->RM = R;

        if (R->PtIndex != 0)
          MNodeSetPartition(N,&MPar[R->PtIndex],NULL);
        }
      }
    }   /* END if (N->RM != NULL) */

  MNodeDetermineEState(N);

  return(SUCCESS);
  }  /* END MRMNodePostUpdate() */




/**
 * Perform common node customization tasks before RM-specific update.
 *
 * Potential Problem: this routine clears ARes and is performed for every RM
 *                    if 2 RMs report the same node AND report ARes then the 
 *                    first could be overwritten.
 *
 * @param N (I)
 * @param NState (I) [new state]
 * @param R (I)
 */

int MRMNodePreUpdate(

  mnode_t             *N,       /* I */
  enum MNodeStateEnum  NState,  /* I (new state) */
  mrm_t               *R)       /* I */

  {
  int tmpInt;

  const char *FName = "MRMNodePreUpdate";

  MDB(4,fRM) MLog("%s(%s,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    MNodeState[NState],
    (R != NULL) ? R->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  if (N->MIteration != MSched.Iteration)
    {
    memset(N->RMState,0,sizeof(N->RMState));
    }

  if (R != NULL)
    N->RMState[R->Index] = NState;

  if (N->MIteration == MSched.Iteration)
    {
    /* only perform pre-update once per iteration */

    return(SUCCESS);
    }

  N->OldState = N->State;

  N->MIteration = MSched.Iteration;

  /* update node statistics */

  N->STTime += (MSched.Interval + 50) / 100;

  if (MNODEISUP(N))
    {
    N->SUTime += (MSched.Interval + 50) / 100;

    if (N->State != mnsIdle)
      {
      N->SATime += (MSched.Interval + 50) / 100;
      }
    }   /* END if (N->RM != NULL) */

  /* update node utilization */

  MCResClear(&N->DRes);

  N->TaskCount      = 0;
  N->EProcCount     = 0;

  N->Load = 0.0;

  MCResClear(&N->ARes);

  N->ARes.Procs     = -1;
    
  tmpInt = -1;
  MNodeSetAttr(N,mnaRAMem,(void **)&tmpInt,mdfInt,mSet);
  MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);

  N->ARes.Disk      = -1;

  /* if node state (active -> idle) transition, verify node is idle */

  if (((N->EState == mnsActive) || (N->EState == mnsBusy)) &&
       (N->State == mnsIdle))
    {
    /* job completed within a single iteration.  synchronize node */

    if (MNodeCheckAllocation(N) != SUCCESS)
      {
      MDB(4,fRM) MLog("ALERT:    node '%s' expected active but is idle and not allocated (estate forced to idle)\n",
        N->Name);

      N->EState = mnsIdle;
      }
    else
      {
      MDB(4,fRM) MLog("ALERT:    node '%s' expected active but is idle and allocated\n",
        N->Name);
      }
    }

  N->SuspendedJCount = 0;

  if ((R != NULL) && (MNODEISUP(N) == TRUE))
    {
    if ((R->JSet != NULL) && (R->JSet->Req[0] != NULL) && (R->JSet->Req[0]->Opsys > 0))
      {
      N->ActiveOS = R->JSet->Req[0]->Opsys;
      }
    else if (R->DefOS > 0)
      {
      N->ActiveOS = R->DefOS;
      }
    }

  return(SUCCESS);
  }  /* END MRMNodePreUpdate() */
/* END MRMNodeUpdate.c */
