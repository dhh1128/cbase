/* HEADER */

/**
 * @file MVMTransition.c
 * 
 * Contains various functions on VM Transition
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "MBSON.h"

/**
 * Allocate memory for the given VM transition object
 *
 * @param VMP (O) [alloc-ed] point to newly allocated VM 
 *          transition object
 */

int MVMTransitionAllocate(

  mtransvm_t **VMP)

  {
  mtransvm_t *VM;

  if (VMP == NULL)
    return(FAILURE);

  VM = (mtransvm_t *)MUCalloc(1,sizeof(mtransvm_t));

  /* catch case where we couldn't allocate any memory */

  if (VM == NULL)
    return(FAILURE);

  *VMP = VM;

  return(SUCCESS);
  }  /* END MVMTransitionAllocateMOExportTransition() */




int MVMTransitionFree(

  void **VMP)

  {
  mtransvm_t *VM;

  const char *FName = "MVMTransitionFree";

  MDB(8,fSTRUCT) MLog("%s(%s)\n",
    FName,
    ((VMP != NULL) && (*VMP != NULL)) ? (*((mtransvm_t **)VMP))->VMID : "NULL");

  if (VMP == NULL)
    return(FAILURE);

  VM = (mtransvm_t *)*VMP;

  MUFree(&VM->JobID);
  MUFree(&VM->SubState);
  MUFree(&VM->LastSubState);
  MUFree(&VM->ContainerNodeName);
  MUFree(&VM->Description);
  MUFree(&VM->StorageRsvNames);
  MUFree(&VM->NetAddr);
  MXMLDestroyE(&VM->Variables);
  MUFree(&VM->Aliases);
  MUFree(&VM->GMetric);
  MUFree(&VM->TrackingJob);

  bmclear(&VM->Flags);
  bmclear(&VM->IFlags);

#ifdef MUSEMONGODB
  if (VM->BSON != NULL)
    {
    delete VM->BSON;
    VM->BSON = NULL;
    }
#endif

  MUFree((char **)VMP);

  return(SUCCESS);
  }  /* END MVMTransitionFree() */




int MVMTransition(

  mvm_t *VM)

  {
  mtransvm_t *TVM = NULL;

  if (VM == NULL)
    return(FAILURE);

#if defined (__NOMCOMMTHREAD)
  return(SUCCESS);
#endif

  MVMTransitionAllocate(&TVM);

  MVMToTransitionStruct(VM,TVM);

  MOExportTransition(mxoxVM,TVM);

  return(SUCCESS);
  }  /* END MVMTransition() */




/**
 * Store VM in transition structure for storage in database.
 *
 * @param SVM (I) The VM to store
 * @param DVM (O) The transition object to store it in
 */

int MVMToTransitionStruct(

  mvm_t      *SVM,  /* I */
  mtransvm_t *DVM)  /* O */

  {
  mbitmap_t BM;

  char tmpLine[MMAX_LINE];

  if ((SVM == NULL) || (DVM == NULL))
    return(FAILURE);

  MVMAToString(SVM,NULL,mvmaID,DVM->VMID,MMAX_LINE,NULL);
  
  MVMAToString(SVM,NULL,mvmaJobID,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->JobID,tmpLine);
  MVMAToString(SVM,NULL,mvmaSubState,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->SubState,tmpLine);
  MVMAToString(SVM,NULL,mvmaLastSubState,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->LastSubState,tmpLine);
  MVMAToString(SVM,NULL,mvmaContainerNode,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->ContainerNodeName,tmpLine);
  MVMAToString(SVM,NULL,mvmaDescription,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->Description,tmpLine);

  if (SVM->Variables.NumItems > 0)
    {
    MUVarsToXML(&DVM->Variables,&SVM->Variables);
    }

  MVMAToString(SVM,NULL,mvmaStorageRsvNames,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->StorageRsvNames,tmpLine);
  MVMAToString(SVM,NULL,mvmaNetAddr,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->NetAddr,tmpLine);
  MVMAToString(SVM,NULL,mvmaTrackingJob,tmpLine,sizeof(tmpLine),NULL);
  MUStrDup(&DVM->TrackingJob,tmpLine);

  MULLToString(SVM->Aliases,FALSE,NULL,tmpLine,MMAX_LINE);
  MUStrDup(&DVM->Aliases,tmpLine);

  bmset(&BM,mcmUser);

  MVMGMetricAToString(SVM->GMetric,tmpLine,MMAX_LINE,&BM);
  MUStrDup(&DVM->GMetric,tmpLine);

  bmcopy(&DVM->Flags,&SVM->Flags);
  bmcopy(&DVM->IFlags,&SVM->IFlags);
  
  DVM->State = SVM->State;

  DVM->LastSubStateMTime = SVM->LastSubStateMTime;

  DVM->RADisk = SVM->ARes.Disk;
  DVM->RAMem = SVM->ARes.Mem;
  DVM->RAProc = SVM->ARes.Procs;

  DVM->RCDisk = SVM->CRes.Disk;
  DVM->RCMem = SVM->CRes.Mem;
  DVM->RCProc = SVM->CRes.Procs;

  DVM->RDDisk = SVM->DRes.Disk;
  DVM->RDMem = SVM->DRes.Mem;
  DVM->RDProc = SVM->DRes.Procs;

  DVM->CPULoad = SVM->CPULoad;
  DVM->NetLoad = SVM->NetLoad;

  DVM->Rack = SVM->Rack;
  DVM->Slot = SVM->Slot;

  DVM->ActiveOS = SVM->ActiveOS;
  DVM->NextOS = SVM->NextOS;
  DVM->LastOSModRequestTime = SVM->LastOSModRequestTime;
  DVM->LastOSModIteration = SVM->LastOSModIteration;

  DVM->LastMigrationTime = SVM->LastMigrationTime;
  DVM->MigrationCount = SVM->MigrationCount;

  DVM->PowerState = SVM->PowerState;

  DVM->LastUpdateTime = SVM->UpdateTime;

  DVM->SpecifiedTTL = SVM->SpecifiedTTL;
  DVM->EffectiveTTL = SVM->EffectiveTTL;
  DVM->StartTime = SVM->StartTime;

#ifdef MUSEMONGODB
  MVMTransitionToBSON(SVM,&DVM->BSON);
#endif

  return(SUCCESS);
  }  /* END MVMToTransitionStruct() */



/**
 * Report specified Transition VM attribute as string
 *
 * @param V       (I)
 * @param AIndex  (I)
 * @param Buf     (O) [minsize=MMAX_LINE]
 * @param BufSize (I) [optional,-1 if not set,default=MMAX_LINE]
 */

int MVMTransitionAToString(

  mtransvm_t      *V,
  enum MVMAttrEnum AIndex,
  char            *Buf,
  int              BufSize)

  {
  if (BufSize <= 0)
    BufSize = MMAX_LINE;

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  Buf[0] = '\0';

  if (V == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mvmaActiveOS:

      if (V->ActiveOS != 0)
        MUStrCpy(Buf,MAList[meOpsys][V->ActiveOS],BufSize);

      break;

    case mvmaADisk:

      if (V->RADisk >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RADisk);
        }

      break;

    case mvmaAlias:

      if ((V->Aliases != NULL) &&
        (V->Aliases[0] != '\0'))
        MUStrCpy(Buf,V->Aliases,BufSize);

      break;

    case mvmaAMem:

      if (V->RAMem >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RAMem);
        }

      break;

    case mvmaAProcs:

      if (V->RAProc >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RAProc);
        }

      break;

    case mvmaCDisk:

      if (V->RCDisk >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RCDisk);
        }

      break;

    case mvmaCPULoad:

      snprintf(Buf,BufSize,"%f",V->CPULoad);

      break;

    case mvmaCMem:

      if (V->RCMem >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RCMem);
        }

      break;

    case mvmaCProcs:

      if (V->RCProc >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RCProc);
        }

      break;

    case mvmaContainerNode:

      if ((V->ContainerNodeName != NULL) &&
        (V->ContainerNodeName[0] != '\0'))
        {
        MUStrCpy(Buf,V->ContainerNodeName,BufSize);
        }

      break;

    case mvmaDDisk:

      if (V->RDDisk >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RDDisk);
        }

      break;

    case mvmaDMem:

      if (V->RDMem >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RDMem);
        }

      break;

    case mvmaDProcs:

      if (V->RDProc >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->RDProc);
        }

      break;

    case mvmaDescription:

      if ((V->Description != NULL) &&
          (V->Description[0] != '\0'))
        {
        MUStrCpy(Buf,V->Description,BufSize);
        }

      break;

    case mvmaEffectiveTTL:

      if (V->EffectiveTTL >= 0)
      {
      snprintf(Buf,BufSize,"%ld",V->EffectiveTTL);
      }

      break;

    case mvmaFlags:

      {
      mstring_t tmp(MMAX_LINE);

      bmtostring(&V->Flags,MVMFlags,&tmp);

      MUStrCpy(Buf,tmp.c_str(),BufSize);
      }

      break;

    case mvmaGMetric:

      if ((V->GMetric != NULL) &&
          (V->GMetric[0] != '\0'))
        {
        MUStrCpy(Buf,V->GMetric,BufSize);
        }

      break;

    case mvmaID:

      if (V->VMID[0] != '\0')
        MUStrCpy(Buf,V->VMID,BufSize);
       
      break;

    case mvmaJobID:

      if ((V->JobID != NULL) &&
          (V->JobID[0] != '\0'))
        {
        MUStrCpy(Buf,V->JobID,BufSize);
        }

      break;

    case mvmaLastMigrateTime:

      if (V->LastMigrationTime > 0)
        snprintf(Buf,BufSize,"%lu",V->LastMigrationTime);

      break;

    case mvmaLastSubState:

      if ((V->LastSubState != NULL) &&
        (V->LastSubState[0] != '\0'))
        MUStrCpy(Buf,V->LastSubState,BufSize);

      break;

    case mvmaLastSubStateMTime:

      if (V->LastSubStateMTime > 0)
        snprintf(Buf,BufSize,"%lu",V->LastSubStateMTime);

      break;

    case mvmaLastUpdateTime:

      if (V->LastUpdateTime > 0)
        {
        snprintf(Buf,BufSize,"%lu",V->LastUpdateTime);
        }

      break;

    case mvmaMigrateCount:

      if (V->MigrationCount > 0)
        snprintf(Buf,BufSize,"%d",V->MigrationCount);

      break;

    case mvmaNextOS:

      if ((V->NextOS > 0) && 
          (V->NextOS != V->ActiveOS))
        {
        MUStrCpy(Buf,MAList[meOpsys][V->NextOS],BufSize);
        }

      break;

    case mvmaPowerIsEnabled:

      {
      mbool_t IsOn = TRUE;

      switch (V->PowerState)
        {
        case mpowOff:

          IsOn = FALSE;

          break;

        default:

          break;
        }

      strcpy(Buf,MBool[IsOn]);
      }  /* END BLOCK */

      break;

    case mvmaPowerState:

      if (V->PowerState != mpowNONE)
        {
        MUStrCpy(Buf,MPowerState[V->PowerState],BufSize);
        }

      break;

    case mvmaRackIndex:

      if(V->Rack >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->Rack);
        }

      break;

    case mvmaSlotIndex:

      if(V->Slot >= 0)
        {
        snprintf(Buf,BufSize,"%d",V->Slot);
        }

      break;

    case mvmaSpecifiedTTL:

      snprintf(Buf,BufSize,"%ld",V->SpecifiedTTL);

      break;

    case mvmaStartTime:

      snprintf(Buf,BufSize,"%lu",V->StartTime);

      break;

    case mvmaState:

      if (bmisset(&V->Flags,mvmfInitializing))
        {
        /* NOTE:  sync w/VM bootstrap state reported via MSM */

        MUStrCpy(Buf,MNodeState[mnsDrained],BufSize);
        }
      else if (V->State != mnsNONE)
        {
        MUStrCpy(Buf,MNodeState[V->State],BufSize);
        }

      break;

    case mvmaSubState:

      if ((V->SubState != NULL) &&
        (V->SubState[0] != '\0'))
        {
        MUStrCpy(Buf,V->SubState,BufSize);
        }

      break;

    case mvmaStorageRsvNames:

      if ((V->StorageRsvNames != NULL) &&
        (V->StorageRsvNames[0] != '\0'))
        {
        MUStrCpy(Buf,V->StorageRsvNames,BufSize);
        }

      break;

    case mvmaVariables:

      /* not handled, shouldn't be done this way as it is XML (see MVMTransitionToXML for the right way) */

      /* NO-OP */

      break;

    case mvmaNetAddr:

      if ((V->NetAddr != NULL) &&
        (V->NetAddr[0] != '\0'))
        MUStrCpy(Buf,V->NetAddr,BufSize);

      break;

    case mvmaTrackingJob:

      if ((V->TrackingJob != NULL) &&
        (V->TrackingJob[0] != '\0'))
        MUStrCpy(Buf,V->TrackingJob,BufSize);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MVMTransitionAToString() */
/* END MVMTransition.c */

/**
 * check to see if a given VM matches a list of constraints
 *
 * @param VM               (I)
 * @param VMConstraintList (I)
 */

mbool_t MVMTransitionMatchesConstraints(

  mtransvm_t   *VM,               /* I */
  marray_t     *VMConstraintList) /* I */

  {
  int cindex;
  mvmconstraint_t   *VMConstraint;
  mbool_t            DisplayOverride = FALSE;
  mbool_t            DisplayAllOverride = FALSE;

  if (VM == NULL)
    return(FALSE);

  if ((VMConstraintList == NULL) || (VMConstraintList->NumItems == 0))
    return(TRUE);

  MDB(7,fSCHED) MLog("INFO:     Checking VM %s against constraint list.\n",
    VM->VMID);

  /* Iterate over the list of Constraints */
  for (cindex = 0; cindex < VMConstraintList->NumItems; cindex++)
    {
    VMConstraint = (mvmconstraint_t *)MUArrayListGet(VMConstraintList,cindex);

    switch((int) VMConstraint->AType)
      {
        /* Match current VM against the Current Constraint */
        case mvmaContainerNode:
          if ((VM->ContainerNodeName == NULL) ||
              (strcmp(VM->ContainerNodeName,VMConstraint->AVal[0]) != 0))
            {
            return(FALSE);
            }
          break;

        case mvmaID:
          if ((strcmp(VM->VMID,VMConstraint->AVal[0]) != 0) &&
              (VMConstraint->AVal[0][0] != '\0') &&
              (strcmp(VMConstraint->AVal[0],"ALL")))
            {
            return(FALSE);
            }
          break;

        /* We have a where clause on VM State */
        case mvmaState:
          /* If 'ALL' constraint is set, then we match it by default */
          if (!strcmp(VMConstraint->AVal[0],"ALL"))
            {
            DisplayAllOverride = TRUE;
            continue;
            }

          /* See if we have a state of 'Drained', if so, then set override flag */
          if (MUGetIndexCI(VMConstraint->AVal[0],MNodeState,FALSE,mnsNONE) == mnsDrained)
            {
            /* Mark that this Node is in a drained state and 
             * we have an override
             */
            DisplayOverride = TRUE;
            continue;
            }

          break;

        default:
          break;

      } /* END switch(VMConstraint->AType) */
  }  /* END for (cindex) */

  /* If -w STATE=ALL set, then display all VMs */
  if (DisplayAllOverride == TRUE)
    return(TRUE);

  /* If DISPLAYFLAGS is set with HIDEDRAINED and the ALL-OVERRIDE is NOT set,
   *    we need to check current VM State and the DisplayOverride flag for further results
   */
  if (bmisset(&MSched.DisplayFlags,mdspfHideDrained) == TRUE)
    {
      /* Check this Node's effective state. 
       * Display only non-drained VMs, 
       *     depending on state of the VM and the Displayoverride
       */
      if (VM->State == mnsDrained)
        {
        /* yet if the DisplayOverride is TRUE, then display this node 
         *     if it is FALSE, we short-circuit now and don't display this node 
         */
        if (DisplayOverride == FALSE)
          return(FALSE);
        }
      else
        {
        /* VM is not in DRAINED state, and if the DisplayOveride is ON, should be skipped */
        if (DisplayOverride == TRUE)
          return(FALSE);
        }
    }
  else
    {
      /* If -w NODESTATE=DRAINED is requested, Nodes in the Drained state are NOT displayed */
      if ((DisplayOverride == TRUE) && (VM->State != mnsDrained))
        return(FALSE);
    }

  return(TRUE);
  }  /* END MVMTransitionMatchesConstraints() */
/* END MVMTransition.c */
