/* HEADER */

      
/**
 * @file MRMNodeLoad.c
 *
 * Contains: RM Node Load functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * @see MNodeAdjustAvailResources() - peer
 *
 *
 * @param N (I) [modified]
 */

int MRMAdjustNodeRes(

  mnode_t *N)  /* I (modified) */

  {
  int tmpInt;

  /* set defaults */

  if (N->CRes.Disk == -1)
    {
    if (MSched.DefaultN.CRes.Disk > 0)
      N->CRes.Disk = MSched.DefaultN.CRes.Disk;
    else
      N->CRes.Disk = 1;
    }

  if (N->CRes.Mem == -1)
    {
    if (MSched.DefaultN.CRes.Mem > 0)
      {
      tmpInt = MSched.DefaultN.CRes.Mem;
      MNodeSetAttr(N,mnaRCMem,(void **)&tmpInt,mdfInt,mSet);
      }
    else
      {
      tmpInt = 1;
      MNodeSetAttr(N,mnaRCMem,(void **)&tmpInt,mdfInt,mSet);
      }
    }

  if (N->CRes.Swap == -1)
    {
    if (MSched.DefaultN.CRes.Swap > 0)
      {
      tmpInt = MSched.DefaultN.CRes.Swap;
      MNodeSetAttr(N,mnaRCSwap,(void **)&tmpInt,mdfInt,mSet);
      }
    else
      {
      tmpInt = 1;
      MNodeSetAttr(N,mnaRCSwap,(void **)&tmpInt,mdfInt,mSet);
      }
    }

  /* adjust memory */

  if (N->ARes.Mem == -1)
    {
    tmpInt = MNodeGetBaseValue(N,mrlMem);
    MNodeSetAttr(N,mnaRAMem,(void **)&tmpInt,mdfInt,mSet);
    }
  else if (bmisset(&N->IFlags,mnifMemOverride) == FALSE)
    {
    tmpInt = MAX(N->ARes.Mem,N->CRes.Mem);
    MNodeSetAttr(N,mnaRCMem,(void **)&tmpInt,mdfInt,mSet);
    }

  /* adjust swap */

  /* NOTE: attempt to obtain MAX swap over time */

  if (N->ARes.Swap == -1)
    {
    tmpInt = MNodeGetBaseValue(N,mrlSwap);
    MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);
    }
  else if (bmisset(&N->IFlags,mnifSwapOverride) == FALSE)
    {
    tmpInt = MAX(N->ARes.Swap,N->CRes.Swap);
    MNodeSetAttr(N,mnaRCSwap,(void **)&tmpInt,mdfInt,mSet);
    }

  /* adjust diskspace */

  if (N->FSys != NULL)
    {
    int fsindex;
    int ADisk;
    int CDisk;

    ADisk = 0;
    CDisk = 0;

    for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
      {
      if (N->FSys->CSize[fsindex] == 0)
        break;

      ADisk += N->FSys->ASize[fsindex];
      CDisk += N->FSys->CSize[fsindex];
      }  /* END for (fsindex) */

    N->ARes.Disk = ADisk;
    N->CRes.Disk = CDisk;
    }  /* END if (N->Sys != NULL) */

  if (N->ARes.Disk == -1)
    {
    N->ARes.Disk = MNodeGetBaseValue(N,mrlDisk);
    }
  else
    {
    N->CRes.Disk = MAX(N->ARes.Disk,N->CRes.Disk);
    }

  return(SUCCESS);
  }  /* END MRMAdjustNodeRes() */


/**
 * Perform common pre-load node initialization.
 *
 * NOTE:  Only sets N->RM if not already set 
 * NOTE:  NODECFG[] attributes loaded later w/in MRMNodePostLoad()
 *
 * @see MRMNodePreUpdate()
 * @see MRMNodeLoad()
 * @see MRMNodePostLoad() - peer
 *
 * NOTE:  clears node's configured, dedicated, and available resources.
 *
 * @param N (I) [modified]
 * @param NState (I)
 * @param R (I)
 */

int MRMNodePreLoad(

  mnode_t             *N,      /* I (modified) */
  enum MNodeStateEnum  NState, /* I */
  mrm_t               *R)      /* I */

  {
  int tmpInt;
  double tmpDouble;

  const char *FName = "MRMNodePreLoad";

  MDB(5,fRM) MLog("%s(%s,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    MNodeState[NState],
    (R != NULL) ? R->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  if (N->MIteration == MSched.Iteration)
    {
    /* only perform pre-load once per iteration */

    return(SUCCESS);
    }

  MSched.EnvChanged = TRUE;

  memset(N->RMState,0,sizeof(N->RMState));

  if (R != NULL)
    N->RMState[R->Index] = NState;

  bmclear(&N->Flags);
  
  N->OldState   = mnsNONE;

  N->MIteration = MSched.Iteration;

  bmclear(&N->FBM);

  MNodeAddRM(N,R);

  if (N->PtIndex != 0)
    {
    /* verify node can access partition (one RM per partition) */

    MNodeSetPartition(N,&MPar[N->PtIndex],NULL);
    }

/* update state in MNodeApplyStatePolicy()
  N->State      = NState;
*/

  N->TaskCount  = 0;
  N->EProcCount = 0;

  MCResClear(&N->CRes);
  MCResClear(&N->DRes);
  MCResClear(&N->ARes);

  if (!MSNLIsEmpty(&MSched.DefaultN.CRes.GenericRes))
    MSNLCopy(&N->CRes.GenericRes,&MSched.DefaultN.CRes.GenericRes);

  if (!bmisclear(&MSched.DefaultN.FBM))
    bmcopy(&N->FBM,&MSched.DefaultN.FBM);

  if (MSched.DefaultN.ChargeRate >= 0.0)
    {
    tmpDouble = MSched.DefaultN.ChargeRate;
    MNodeSetAttr(N,mnaChargeRate,(void **)&tmpDouble,mdfDouble,mSet);
    }

  N->DRes.Procs = -1;
  N->CRes.Procs = -1;

  if (MSched.DefaultN.CRes.Disk > 0)
    N->CRes.Disk = MSched.DefaultN.CRes.Disk;

  if (MSched.DefaultN.CRes.Mem > 0) /* NODECFG[DEFAULT] RCMEM=#^ */
    {
    tmpInt = MSched.DefaultN.CRes.Mem;
    MNodeSetAttr(N,mnaRCMem,(void **)&tmpInt,mdfInt,mSet);

    if (bmisset(&MSched.DefaultN.IFlags,mnifMemOverride))
      bmset(&N->IFlags,mnifMemOverride);
    }

  if (MSched.DefaultN.CRes.Swap > 0) /* NODECFG[DEFAULT] RCSWAP=#^ */
    {
    tmpInt = MSched.DefaultN.CRes.Swap;
    MNodeSetAttr(N,mnaRCSwap,(void **)&tmpInt,mdfInt,mSet);

    if (bmisset(&MSched.DefaultN.IFlags,mnifSwapOverride))
      bmset(&N->IFlags,mnifSwapOverride);
    }

  N->Arch     = MSched.DefaultN.Arch;
  N->ActiveOS = MSched.DefaultN.ActiveOS;

  N->ARes.Procs = -1;

  tmpInt = -1;
  MNodeSetAttr(N,mnaRAMem,(void **)&tmpInt,mdfInt,mSet);
  MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);

  N->ARes.Disk  = -1;

  N->URes.Procs = -1;

  bmclear(&N->Classes);

  /* set default partition (map to RM) */

  if (N->PtIndex <= 0)
    {
    if (R != NULL) 
      {
      if (bmisset(&R->RTypes,mrmrtCompute))
        {
        if (R->PtIndex != 0)
          MNodeSetPartition(N,&MPar[R->PtIndex],NULL);
        else
          MNodeSetPartition(N,NULL,R->Name);
        }
      else if (bmisset(&R->RTypes,mrmrtProvisioning))
        {
        /* for provisioning RM's assign to partition matching RM even if
           RM is in shared partition */

        MNodeSetPartition(N,NULL,R->Name);
        }
      else
        {
        MNodeSetPartition(N,NULL,"SHARED");
        }
      }
    else if (N->PtIndex <= 0)
      { 
      MNodeSetPartition(N,NULL,"SHARED");
      }
    }    /* END if (N->PtIndex <= 0) */

  N->EState          = N->State;
  N->SyncDeadLine    = MMAX_TIME;
  N->StateMTime      = MSched.Time;

/* FIXME: when resetting nodes between resource managers we do not want to 
          reset the node's reservation table */

/*
  if (bmisset(&N->Flags,mnfRMDetected))
    {
    N->R[0] = NULL;
    }
*/

  N->CTime           = MSched.Time;
  N->MTime           = MSched.Time;
  N->ATime           = MSched.Time;

  N->STTime          = 0;
  N->SUTime          = 0;
  N->SATime          = 0;

  bmset(&N->IFlags,mnifIsNew);

  N->SuspendedJCount = 0;

  bmset(&N->IFlags,mnifRMDetected);

  if (MNODEISUP(N) == TRUE)
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
  }  /* END MRMNodePreLoad() */




#define MDEF_NODEREWIGGLEROOM 4

/**
 * Handle common node processing after RM has reported node attributes.
 *
 * NOTE:  called once per RM at node load.
 *
 * @see MRMNodePreLoad()
 * @see MRMNodeLoad()
 * @see MRMNodePostUpdate() - peer
 * @see MNodePostLoad() - peer - called once per node load
 * @see MRMClusterQuery() - parent
 *
 * @param N (I)
 * @param R (I) [optional]
 */

int MRMNodePostLoad(

  mnode_t *N,  /* I */
  mrm_t   *R)  /* I (optional) */

  {
  char      tmpName[MMAX_NAME];

  const char *FName = "MRMNodePostLoad";

  MDB(5,fRM) MLog("%s(%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* node configuration moved to MNodePostLoad() */

  MNodeApplyDefaults(N);

  MNodeAdjustName(N->Name,1,tmpName);

  if (strcmp(N->Name,tmpName) != 0)
    MUStrDup(&N->FullName,tmpName);

  if (N->CTime == 0)
    {
    /* initialize node timestamps */

    N->CTime = MSched.Time;
    N->MTime = MSched.Time;
    N->ATime = MSched.Time;
    }

  MRMAdjustNodeRes(N);

  /* NOTE: must properly calculate available procs */

  if (N->CRes.Procs == -1)
    N->CRes.Procs = 1;

  if (R != NULL)
    {
    /* set default values */

    bmor(&N->FBM,&R->FBM);
    }    /* END if (N->RM != NULL) */

  /* update node profiling statistics in MSysMainLoop() */

  MNodeDetermineEState(N);

  return(SUCCESS);
  }  /* END MRMNodePostLoad() */
/* END MRMNodeLoad.c */
