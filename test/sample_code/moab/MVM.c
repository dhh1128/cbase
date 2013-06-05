/* HEADER */

/**
 * @file MVM.c
 * 
 * Contains VM related functions.
 */

/* Contains:                                 

 int MVMScheduleMigration()
 int MVMStringToPNString()
 int MNodeVMListToXML()
 int MVMAToString()
 mbool_t MNodeCheckAction()
 mbool_t MUCheckAction()
 int MVMStringToTL()
 int MVMAdd()
 int MVMDestroy()
 int MVMStart()
 int MVMStop()
 int MVMLToString()
 int MVMGetUniqueID()
 int MVMFree()
 */



#include <assert.h>

#include "moab.h"
#include "moab-proto.h"  
#include <stddef.h>
#include "moab-global.h"
#include "moab-const.h"
#include "MEventLog.h"

/* local prototypes */

int MVMActionFromXML(mvm_t *,mxml_t *);


/**
 * initialize mvm_req_create_t struct. Assumes the struct is uninitialized.
 *
 * @param VMCR (I) modified
 */

int MVMCreateReqInit(

  mvm_req_create_t *VMCR)

  {
  memset(VMCR,0,sizeof(VMCR[0]));

  VMCR->IsSovereign = MSched.DefaultVMSovereign;

  return(SUCCESS);
  }  /* END MVMCreateReqInit() */





/**
 * Free resources associated with an mvm_req_create_t struct. 
 *
 * @param VMCR (I) modified
 */

int MVMCreateReqFree(

  mvm_req_create_t *VMCR)

  {
  MULLFree(&VMCR->StorageRsvs,NULL);

  memset(VMCR,0,sizeof(mvm_req_create_t));

  return(SUCCESS);
  }  /* END MVMCreateReqFree() */






/**
 * Report if node has active VM's in place.
 *
 * @param N (I)
 */

mbool_t MNodeHasVMs(

  const mnode_t *N)  /* I */

  {
  mln_t  *P;  /* VM list pointer */
  mvm_t  *V;

  if ((N == NULL) || (N->VMList == NULL))
    {
    return(FALSE);
    }

  for (P = N->VMList;P != NULL;P = P->Next)
    {
    V = (mvm_t *)P->Ptr;

    if (V != NULL)
      {
      return(TRUE);
      }
    }  /* END for (P) */

  return(FALSE);
  }  /* END MNodeHasVMs() */



/**
 * Perform post RM update processing.
 *
 * NOTE: this is called after RMs have been processed (once per iteration)
 *
 * @param VM
 */

int MVMPostUpdate(

  mvm_t *VM)

  {
  if (VM == NULL)
    return(FAILURE);

  mrm_t *RM;

  int rindex;

  int StateTotal = 0;

  for (rindex = 0;rindex < MMAX_RM;rindex++)
    {
    RM = &MRM[rindex];

    if (RM->Name[0] == '\0')
      break;

    if (MRMIsReal(RM) == FALSE)
      continue;

    if (VM->RMState[rindex] == mnsNONE)
      continue;

    switch (VM->RMState[rindex])
      {
      /* neutral */
      default:
      case mnsNONE:
      case mnsNone:   
      case mnsUnknown:
      case mnsReserved:
      case mnsLAST:

        /* NO-OP */

        break;

      /* positive */

      case mnsIdle:   
      case mnsBusy:   
      case mnsActive: 
      case mnsUp:

        StateTotal++;

        break;

      /* negative */
      case mnsDown:
      case mnsDrained:
      case mnsDraining:
      case mnsFlush:

        StateTotal--;

        break;
      } /* END switch() */
    }   /* END for (rindex) */

  if (StateTotal > 0)
    VM->State = mnsIdle;
  else if (StateTotal < 0)
    VM->State = mnsDown;
  else
    VM->State = mnsUnknown;

  return(SUCCESS);
  }  /* END MVMPostUpdate() */




/**
 * Create VM and add VM to global VM table.
 *
 * NOTE:  if newly adding VM, will restore checkpointed VM state if available
 *
 * @see MVMLoadConfig() - parent
 * @see MVMFind() - peer
 * @see MVMLoadCP() - child
 *
 * @param VMID (I)
 * @param VP (O) [optional]
 */

int MVMAdd(

  const char  *VMID,
  mvm_t      **VP)

  {
  mvm_t *V;

  char EMsg[MMAX_LINE] = {0};

  if (VP != NULL)
    *VP = NULL;

  if (VMID == NULL)
    return(FAILURE);

  if (MVMFind(VMID,VP) == SUCCESS)
    {
    return(SUCCESS);
    }

  if (MLicenseCheckVMUsage(EMsg) == FAILURE)
    {
    MDB(1,fRM) MLog(EMsg);

    return(FAILURE);
    }

  V = (mvm_t *)MUCalloc(1,sizeof(mvm_t));

  if (V == NULL)
    {
    MDB(1,fRM) MLog("ALERT:    cannot add VM child '%s' - no memory\n",
      VMID);

    return(FAILURE);
    }

  memset(V,0,sizeof(mvm_t));

  if (MUHTAdd(&MSched.VMTable,VMID,(void *)V,NULL,NULL) == FAILURE)
    {
    MDB(1,fRM) MLog("ALERT:    cannot add VM child '%s' - global table full\n",
      VMID);

    MUFree((char **)&V);

    return(FAILURE);
    }

  MUStrCpy(V->VMID,VMID,sizeof(V->VMID));

  /* Initialize the configured, available, & dedicated resources */

  MCResInit(&V->ARes);
  MCResInit(&V->CRes);
  MCResInit(&V->DRes);
  MCResInit(&V->LURes);
  V->CRes.Procs = 1;
  V->ARes.Procs = 0;

  bmset(&V->Flags,mvmfCanMigrate);   /* all VM's are migrateable by default */

  if (MSched.DefaultVMSovereign == TRUE)
    bmset(&V->Flags,mvmfSovereign);
  else
    bmunset(&V->Flags,mvmfSovereign);

  if ((V->RMList = (mrm_t **)MUCalloc(1,sizeof(mrm_t *) * (MMAX_RM + 1))) == NULL)
    {
    MDB(1,fRM) MLog("ALERT:    cannot alloc memory for VM child '%s'\n",
      VMID);

    MUFree((char **)&V);

    return(FAILURE);
    }

  MGMetricCreate(&V->GMetric);

  /* VM successfully added */

  /* load VM checkpoint info */

  MVMLoadCP(V,NULL);

  MVMTransition(V);

  if (VP != NULL)
    *VP = V;

  return(SUCCESS);
  }  /* END MVMAdd() */



/**
 * Returns TRUE if it is legal to attempt to remove VM, FALSE otherwise.
 * EMsg will contain a verbal description of why the VM can't be deleted.
 *
 * @param VM   (I)
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 */

mbool_t MVMCanRemove(

  mvm_t *VM,
  char  *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  mbool_t CanDelete = TRUE;
  mjob_t *tmpJ = NULL;

  if (VM == NULL)
    return(FALSE);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (bmisset(&VM->Flags,mvmfDestroyPending) ||
      bmisset(&VM->Flags,mvmfDestroyed))
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"VM is already deleted or pending deletion");

    CanDelete = FALSE;
    }
  else if (MUCheckAction(&VM->Action,msjtVMMigrate,NULL) == SUCCESS)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"VM is currently being migrated");

    CanDelete = FALSE;
    }
  else if ((MUCheckAction(&VM->Action,msjtGeneric,&tmpJ) == SUCCESS) &&
           (MVMJobIsVMMigrate(tmpJ) == TRUE))
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"VM is currently being migrated");

    CanDelete = FALSE;
    }

  return(CanDelete);
  }  /* END MVMCanRemove() */





/**
 * Attempts to remove VM by calling MRMSystemModify. Will fail if
 * MVMCanRemove returns FALSE or if MRMSystemModify fails.
 * EMsg will contain a verbal description of why the VM can't be deleted.
 *
 * @param VM   (I)
 * @param OperationID (I) [optional] name of system job associated with removing the RM
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 */

int MVMRemove(

  mvm_t *VM,
  char  *OperationID,
  char  *EMsg)

  {
  mnode_t *N;
  char     OBuf[MMAX_LINE];  /* output buffer */
  int      rc;

  mrm_t *R;

  mjob_t *J = NULL;

  const char *FName = "MVMRemove";

  MDB(4,fALL) MLog("%s(%s,%s,EMsg)\n",
    FName,
    (VM != NULL) ? VM->VMID : "NULL",
    (OperationID != NULL) ? OperationID : "NULL");

  rc = FAILURE;

  if (NULL == VM)
    return(FAILURE);

  N = VM->N;

  if (OperationID != NULL)
    {
    if (MJobFind(OperationID,&J,mjsmExtended) == FAILURE)
      {
      MDB(5,fSTRUCT) MLog("ALERT:    could not find job '%s' for removing VM\n",
        OperationID);
      }
    }

  if (MVMCanRemove(VM,EMsg))
    {
    char tmpID[MMAX_NAME];

    snprintf(tmpID,sizeof(tmpID),"%s:%s%s%s",
      N->Name,
      VM->VMID,
      (J != NULL) ? " operationid=" : "",
      (J != NULL) ? J->Name : "");

    /* remove VM from node */

    if (MSched.ProvRM != NULL)
      R = MSched.ProvRM;
    else
      R = N->RM;

    rc = MRMSystemModify(
         R,
         "node:destroy",
         tmpID,
         FALSE,   /* IsBlocking */
         OBuf,    /* O */
         mfmNONE,
         EMsg,    /* O */
         NULL);
    }

  if (rc == SUCCESS)
    {
    bmset(&VM->Flags,mvmfDestroyPending);
    bmunset(&VM->Flags,mvmfCanMigrate);
    } /* END if (MRMSystemModify() == FAILURE) */

  /* NOTE:  possible race condition - make certain MRMSystemModify() 
            does not destroy V */

  return(rc);
  }  /* END MVMRemove() */



/**
 *  Moves the VM into the completed table.
 *
 * @param V [I] (modified) - The VM to be completed
 */

int MVMComplete(

  mvm_t *V)

  {
  mln_t *LLRecord = NULL;

  /* Write out event */

  CreateAndSendEventLog(meltVMDestroy,V->VMID,mxoxVM,(void *)V);

  /* Put into completed table */

  if (MUHTAdd(&MSched.VMCompletedTable,V->VMID,(void *)V,NULL,MUFREE) == FAILURE)
    {
    return(FAILURE);
    }

  /* Remove from global table, compute jobs, VCs, etc. */

  if ((V->J != NULL) && (V->J != V->TrackingJ))
    {
    MJobRemove(&V->J);
    }

  V->J = NULL;  /* Set to NULL in case it's a VMTracking job */
  V->TrackingJ = NULL;

  if (V->N != NULL)
    MVMSetParent(V,NULL);

  while (MULLIterate(V->Aliases,&LLRecord) == SUCCESS)
    {
    MUHTRemove(&MNetworkAliasesHT,LLRecord->Name,MUFREE);
    }

  MULLFree(&V->Aliases,NULL);

  if (V->ParentVCs != NULL)
    {
    MVCRemoveObjectParentVCs((void *)V,mxoxVM);
    }

  MUHTRemove(&MSched.VMTable,V->VMID,NULL);

  bmset(&V->Flags,mvmfCompleted);

  if (MSched.Shutdown == FALSE)
    MCacheRemoveVM(V->VMID);

  return(SUCCESS);
  }  /* END MVMComplete() */


/**
 *  Moves the VM back from the completed table.
 *
 * @param V [I] (modified) - The VM to be put back into action
 */

int MVMUncomplete(

  mvm_t *V)

  {
  if (MUHTAdd(&MSched.VMTable,V->VMID,(void *)V,NULL,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  MUHTRemove(&MSched.VMCompletedTable,V->VMID,MUFREE);

  bmunset(&V->Flags,mvmfCompleted);

  return(SUCCESS);
  }  /* END int MVMUncomplete() */



/**
 * Remove references to and free/destroy VM object.
 *
 * NOTE: this routine does NOT remove the VM externally, only internally.
 *
 * @param VP (O) [freed]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 *
 * @see MVMFree() - child
 */

int MVMDestroy(

  mvm_t **VP,   /* O (freed) */
  char   *EMsg) /* O (optional,minsize=MMAX_LINE) */

  {
  mvm_t            *V;
  char             *GEName;
  mgevent_obj_t    *GEvent;
  mgevent_iter_t    GEIter;
  mln_t            *LLRecord = NULL;

  const char *FName = "MVMDestroy";

  MDB(4,fALL) MLog("%s(%s,EMsg)\n",
    FName,
    ((VP != NULL) && (*VP != NULL)) ? (*VP)->VMID : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((VP == NULL) || (*VP == NULL))
    {
    return(SUCCESS);
    }

  V = *VP;

  bmset(&V->Flags,mvmfDeleted);
 
  /* Remove VM from J->TaskMap */

  /* remove associated compute jobs */
 
  if ((V->J != NULL) && (V->J != V->TrackingJ))
    {
    MJobRemove(&V->J);
    }

  /* Remove from VCs */

  if (V->ParentVCs != NULL)
    {
    MVCRemoveObjectParentVCs((void *)V,mxoxVM);
    }

  MUFreeActionTable(&V->Action,TRUE);

  MGEventIterInit(&GEIter);
  while (MGEventItemIterate(&V->GEventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
    {
    MGEventRemoveItem(&V->GEventsList,GEName);
    MUFree((char **)&GEvent->GEventMsg);
    MUFree((char **)&GEvent->Name);
    MUFree((char **)&GEvent);
    }
  MGEventFreeItemList(&V->GEventsList,FALSE,NULL);

  LLRecord = NULL;

  while (MULLIterate(V->Aliases,&LLRecord) == SUCCESS)
    {
    MUHTRemove(&MNetworkAliasesHT,LLRecord->Name,MUFREE);
    }

  MULLFree(&V->Aliases,NULL);

  /* disassociate VM from parent */

  if (V->N != NULL)
    MVMSetParent(V,NULL);

  /* remove from global table */

  MUHTRemove(&MSched.VMTable,V->VMID,NULL);
  MUHTRemove(&MSched.VMCompletedTable,V->VMID,NULL);

  LLRecord = NULL;

  while (MULLIterate(V->StorageRsvs,&LLRecord) == SUCCESS)
    {
    mrsv_t *TmpR;

    TmpR = (mrsv_t *)LLRecord->Ptr;

    bmunset(&TmpR->Flags,mrfParentLock);

    TmpR->CancelIsPending = TRUE;
    TmpR->EndTime = MSched.Time - 1;
    TmpR->ExpireTime = 0;
    }

  MULLFree(&V->StorageRsvs,NULL);

  LLRecord = NULL;

  while (MULLIterate(V->Storage,&LLRecord) == SUCCESS)
    {
    mvm_storage_t *TmpS;

    TmpS = (mvm_storage_t *)LLRecord->Ptr;

    /* Triggers already belonged to jobs, removed there */

    MUFree((char **)&TmpS);
    }

  MULLFree(&V->Storage,NULL);

  /* Cancel the reservations */
  MRsvCheckStatus(NULL);

  /* Clear gmetrics */

  MGMetricFree(&V->GMetric);

  /* Clear triggers */

  if (V->T != NULL)
    {
    MTListClearObject(V->T,FALSE);

    MUFree((char **)&V->T);
    }

  MCResFree(&V->ARes);
  MCResFree(&V->CRes);
  MCResFree(&V->DRes);
  MCResFree(&V->LURes);

  if (V->OutOfBandRsv != NULL)
    MRsvDestroy(&V->OutOfBandRsv,TRUE,TRUE);

  /* free dynamic memory */

  MUFree((char **)&V->RMList);
  MUFree(&V->NetAddr);
  MUFree(&V->OwnerJob);

  if (MSched.Shutdown == FALSE)
    MCacheRemoveVM(V->VMID);

  MVMFree(VP);

  return(SUCCESS);
  }  /* END MVMDestroy() */



/**
 * This function takes a linked list of VMs and destroys each one of them
 * using MVMDestroy()
 *
 * @param VMList (I) The linked-list of VM's to destroy.
 *
 * @see MVMDestroy()
 * @see MNodeDestroy()
 */

int MVMListDestroy(

  mln_t *VMList)

  {
  mln_t *TmpList = NULL;
  mln_t *ListNode = NULL;
  mvm_t *V = NULL;

  if (VMList == NULL)
    {
    return(FAILURE);
    }

  /* Copy the VMList because VMDestroy will remove the VM from and alter the 
      parent node VMList (which is the list which was passed in) */

  /* Can't use MULLDup() because it assumes Ptr is a string, was segfaulting */

  MULLCreate(&TmpList);

  for (ListNode = VMList;ListNode != NULL;ListNode = ListNode->Next)
    {
    MULLAdd(&TmpList,ListNode->Name,ListNode->Ptr,NULL,NULL);
    }

  for (ListNode = TmpList;ListNode != NULL;ListNode = ListNode->Next)
    {
    V = (mvm_t *)ListNode->Ptr;
    
    if (V != NULL)
      {
      MVMDestroy(&V,NULL);
      }
    }

  /* Pointers were already destroyed via MVMDestroy */

  MULLFree(&TmpList,NULL);

  return(SUCCESS);
  }  /* END MVMListDestroy() */





/**
 * Free VMs from VMHash, which requires special work because MVMDestroy
 * modifies the hash directly so that we can't iterate over it while freeing it
 *
 * @param VMHash (I) modified/freed
 */

int MVMHashFree(

  mhash_t *VMHash)

  {
  mvm_t *V;
  int NumItems = VMHash->NumItems;
  int index = 0;
  mhashiter_t Iter;

  mvm_t **VMList;

  if (VMHash->NumItems <= 0)
    /* no VM's to free */
    return(SUCCESS);

  if ((VMList = (mvm_t **)MUCalloc(NumItems,sizeof(mvm_t *))) == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&Iter);

  while (MUHTIterate(VMHash,NULL,(void **)&V,NULL,&Iter) == SUCCESS)
    {
    assert(index < NumItems);
    VMList[index] = V;
    index++;
    }

  for (index = 0;index < NumItems;index++)
    {
    MVMDestroy(VMList + index,NULL);
    }

  /* MVMDestroy should remove the nodes from VMHash */
  MAssert(VMHash->NumItems == 0,"VM hash should have been cleared",__FUNCTION__,__LINE__);

  /* since number of items is 0 we can call MUHTFree() with
     FreeObjects as FALSE and a NULL free function pointer */

  MUHTFree(VMHash,FALSE,NULL);
  MUFree((char **)&VMList);

  return (SUCCESS);
  } /* END MVMHashFree() */




/**
 * Report if VM can be 'live' migrated to specified node
 *
 * @param V                 (I)
 * @param DstN              (I)
 * @param IsManualMigration (I) - Specifies whether destination is being selected due to manual VM migration.
 * @param EMsg              (O) [optional,minsize=MMAX_LINE]
 */

mbool_t MVMNodeIsValidDestination(

  mvm_t   *V,
  mnode_t *DstN,
  mbool_t  IsManualMigration,
  char    *EMsg)

  {
  char SrcVLAN[MMAX_LINE];
  char DstVLAN[MMAX_LINE];

  int SrcInVLAN;
  int DstInVLAN;

  if ((V == NULL) || (DstN == NULL))
    return(FALSE);

  if (V->N == DstN)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"target node is same as current node");

    return(FALSE);
    }

  if ((IsManualMigration == FALSE) &&
      bmisset(&DstN->Flags,mnfNoVMMigrations))
    {
    /* only enforce NoVMMigrations when not a manual migration */

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"target node does not allow VM migrations");

    return(FALSE);
    }

  if (DstN->HVType == mhvtNONE)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"target node is not a hypervisor");

    return(FALSE);
    }

  /* NOTE:  live migration requires matching VM types */

  if (DstN->HVType != V->N->HVType)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"target node hypervisor type does not match");

    return(FALSE);
    }

  /* target node must support the OS of the VM (if the VM has an OS listed) */

  if (V->ActiveOS != 0)
    {
    if (MUHTGet(
        &MSched.VMOSList[DstN->ActiveOS],
        MAList[meOpsys][V->ActiveOS],
        NULL,NULL) == FAILURE)
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"target node does not support VM OS");

      return(FALSE);
      }

    if ((DstN->NextOS != 0) &&
        (MUHTGet(
           &MSched.VMOSList[DstN->NextOS],
           MAList[meOpsys][V->ActiveOS],
           NULL,NULL) == FAILURE))
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"target node is being provisioned to an OS "
            "that does not support VM OS");

      return(FALSE);
      }

    if (MNodeCanProvisionVMOS(DstN,FALSE,V->ActiveOS,NULL) == FAILURE)
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"target node does not support VM OS");

      return(FALSE);
      }
    }  /* END if (V->ActiveOS != 0) */

  if (MNodeGetSysJob(DstN,msjtPowerOff,MBNOTSET,NULL) == SUCCESS)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"target node is scheduled to be powered off");

    return(FALSE);
    }

  SrcInVLAN = MNodeGetVLAN(V->N,SrcVLAN);
  DstInVLAN = MNodeGetVLAN(DstN,DstVLAN);

  /* SrcInVLAN XOR DstInVLAN is bad news.
   * In other words, it's okay if they are both in a vlan or both not in a vlan,
   * but bad if one is in a VLAN and the other is not */

  if ((SrcInVLAN || DstInVLAN) && !(SrcInVLAN && DstInVLAN))
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"%s node '%s' belongs to VLAN '%s' but %s node '%s' is not in any VLAN",
        (SrcInVLAN ? "source" : "destination"),
        (SrcInVLAN ? V->N->Name : DstN->Name),
        (SrcInVLAN ? SrcVLAN : DstVLAN),
        (!SrcInVLAN ? "source" : "destination"),
        (!SrcInVLAN ? V->N->Name : DstN->Name));
      }

    return(FALSE);
    }

  /* both nodes need to be in the same VLAN */

  if ((SrcInVLAN && DstInVLAN) && 
      (strcmp(SrcVLAN,DstVLAN)))
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"VLAN mismatch - source node '%s' belongs to VLAN '%s' but destination node '%s' belongs to VLAN '%s'",
        V->N->Name,
        SrcVLAN,
        DstN->Name,
        DstVLAN);
      }

    return(FALSE);
    }

  /* MOAB-3107 -- BoA is seeing hypervisors with a load of zero or Avail Mem
     equals Config Mem.  In those conditions, either the MSM isn't reporting
     valid numbers or the hypervisor isn't healthy.  Therefore, BoA and Professional
     Services don't want those hypervisors considered as valid migration destinations. */

  if ((!MSched.MigrateToZeroLoadNodes) && (IsManualMigration == FALSE))
    {
    if ((DstN->Load == 0.0) || (DstN->CRes.Mem == DstN->ARes.Mem))
      {
      char *ptr = NULL;
      char buffer[MMAX_LINE];
      char buffer2[MMAX_LINE];
  
      snprintf(buffer2,MMAX_LINE,
               "Invalid Destination (%s). CPU load is zero or available memory equals configured memory.  Load = %f, Configured Memory = %d, Available Memory = %d",
               DstN->Name,
               DstN->Load,
               DstN->CRes.Mem,
               DstN->ARes.Mem);
  
      MDB(3,fMIGRATE) MLog("%s\n",buffer2);
  
      /* Set the GEvent ont the node */
  
      snprintf(buffer,MMAX_LINE,"migrate:%s",buffer2);
      MUStrDup(&ptr,buffer);
      MNodeSetAttr(DstN,mnaGEvent,(void **)&ptr,mdfString,mSet);
  
      if (EMsg)
        MUStrCpy(EMsg,buffer2,MMAX_LINE);

      return(FALSE);
      }
    }

  return(TRUE);
  }  /* END MVMNodeIsValidDestination() */




/**
 * Get unique VM id.
 *
 * NOTE: look for faster way to search (NYI) 
 *
 * @param VMID (O)
 */

int MVMGetUniqueID(

  char *VMID) /* O (minsize=MMAX_NAME) */

  {
  if (VMID == NULL)
    {
    return(FAILURE);
    }

  do
    {
    MSched.VMIDCounter += 1;

    snprintf(VMID,MMAX_NAME,"vm%d",
      MSched.VMIDCounter);
    } while ((MVMFind(VMID,NULL) == SUCCESS) ||
             (MVMFindCompleted(VMID,NULL) == SUCCESS) ||
             (MUHTGet(&MVMIDHT,VMID,NULL,NULL) == SUCCESS));

  return(SUCCESS);
  }  /* END MVMGetUniqueID() */





/**
 * Free an mvm_t struct and associated resources
 *
 * @param VP (I) [freed]
 *
 * @see MVMDestroy() - parent
 */

int MVMFree(

  mvm_t **VP)

  {
  mvm_t *V;

  const char *FName = "MVMFree";

  MDB(4,fALL) MLog("%s(%s)\n",
    FName,
    ((VP != NULL) && (*VP != NULL)) ? (*VP)->VMID : "NULL");

  if ((VP == NULL) || (*VP == NULL))
    {
    return(SUCCESS);
    }

  V = *VP;

  MUFree(&V->SubState);
  MUFree(&V->LastSubState);

  MNodeFreeOSList(&V->OSList,MMAX_NODEOSTYPE);
 
  MUHTFree(&V->Variables,TRUE,MUFREE);

  MUFree(&V->LastActionDescription);

  MCResFree(&V->ARes);
  MCResFree(&V->DRes);
  MCResFree(&V->ARes);

  bmclear(&V->Flags);
  bmclear(&V->IFlags);

  MUFree((char **)VP);

  return(SUCCESS);
  }  /* END MVMFree() */





#if 0
/**
 * Define an mvm_req_create_t for each node J is allocated to run on, using
 * the task definitions contained in J->Req to define each VM.
 *
 * NOTE:  currently only used to create VMs for VMCreate jobs
 *
 * @see MJobStart()/MAdaptiveJobStart() - parent
 *
 * @param J                  (I)
 * @param VMCROut            (O) mvm_req_create_t structs will be appended onto
 *                               VMCROut.
 * @param ArrayIsInitialized (I) specifies whether or not MUArrayListCreate has 
 *                               already been called on VMCROut
 * @param EMsg               (O) [optional,minsize=MMAX_LINE]
 *
 * @return SUCCESS if the array is successfully allocated and the reqs 
 *   are successfully created, FAILURE otherwise.
 *
 *  The initialization status of the array is preserved on failure, so that
 *  if the array was not initialized prior to calling this function it
 *  will be uninitialized on a failed return.
 */

int MVMCreateReqFromJob(

  mjob_t   *J,
  marray_t *VMCROut,
  mbool_t   ArrayIsInitialized,
  char     *EMsg)

  {
  int rqindex;
  int nindex;

  if (J == NULL)
    return(FAILURE);

  if (J->VMCreateRequest != NULL)
    {
    if (!ArrayIsInitialized)
      {
      MUArrayListCreate(VMCROut,sizeof(mvm_req_create_t),1);
      }

    MUArrayListAppend(VMCROut,(void *)J->VMCreateRequest);

    return(SUCCESS);
    }

  assert(J->Req[0] != NULL);

  if (!ArrayIsInitialized)
    {
    int NumNodes = 0;

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++) 
      {
      mnl_t *NL = &J->Req[rqindex]->NodeList;

      if (MNLIsEmpty(NL))
        continue;

      NumNodes = MNLCount(NL);
      }

    MUArrayListCreate(VMCROut,sizeof(mvm_req_create_t),NumNodes);
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    mreq_t    *RQ = J->Req[rqindex];
    mnl_t     *NL = &RQ->NodeList;

    if (MNLIsEmpty(NL))
      continue;

    if (RQ->Opsys == 0)
      {
      char *OSName;

      mhashiter_t Iter;

      MUHTIterInit(&Iter);

      if (MUHTIterate(&MSched.VMOSList[0],&OSName,NULL,NULL,&Iter) == FAILURE)
        {
        /* no default OS found */
        /* TODO: what about multi-req  jobs? */

        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"job contains req with no specified OS");

        if (ArrayIsInitialized == FALSE)
          MUArrayListFree(VMCROut);

        return(FAILURE);
        }

      /* use first OS found in Moab as the default OS. Should we be setting
         this on the job's req as well?*/

      RQ->Opsys = MUMAGetIndex(meOpsys,OSName,mAdd);
      }  /* END if (RQ->Opsys == 0) */

    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,NULL) == SUCCESS;nindex++)
      {
      char *VMName;

      int TC = MNLGetTCAtIndex(NL,nindex);

      mvm_req_create_t *VMCR = (mvm_req_create_t *)MUArrayListAppendEmpty(VMCROut);

      MVMCreateReqInit(VMCR);

      if (MUHTGet(&J->Variables,"VMID",(void **)&VMName,NULL) == FAILURE)
        {
        /* no vmid specified in variables, create with numbers */
        MVMGetUniqueID(VMCR->VMID);
        }
      else
        {
        /* validate specified vmid and assign to vmcr */
        mvm_t* tV = NULL;

        if (MVMFind(VMName,&tV) == SUCCESS)
          {
          if (ArrayIsInitialized == FALSE)
            MUArrayListFree(VMCROut);

          if (EMsg != NULL)
            snprintf(EMsg,MMAX_LINE,"specified VMID already exists");

          return(FAILURE);
          }

        snprintf(VMCR->VMID,MMAX_NAME,"%s",VMName);
        }

      /* a VM will use the resources specified in RQ->DRes multiplied by the task
       * count allocation on RQ->NodeList */

      VMCR->OSIndex = RQ->Opsys;
      MCResPlus(&VMCR->CRes,&RQ->DRes);
      MCResTimes(&VMCR->CRes,TC);

      VMCR->OwnerJob = J;

      /* NOTE:  currently only used to create VM's for one-time use VMCreate jobs */

      VMCR->IsOneTimeUse = TRUE;

      /* add variables to VMCR */
      MVarToQuotedString(&J->Variables,VMCR->Vars,sizeof(VMCR->Vars));
      }  /* END for (nindex) */
    }    /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MVMCreateReqFromJob() */
#endif




/**
 *
 * @param VM (I)
 *
 * @return TRUE if VM is being destroyed or is already destroyed,
 * FALSE otherwise 
 */

mbool_t MVMIsDestroyed(

  mvm_t *VM)

  {
  mbool_t IsDestroyed;

  IsDestroyed = (bmisset(&VM->Flags,mvmfDestroyPending) || 
                 bmisset(&VM->Flags,mvmfDestroyed) ||
                 bmisset(&VM->Flags,mvmfDeleted));

  return(IsDestroyed);
  }  /* END MVMIsDestroyed() */





/**
 * Copy a mcres_t struct on all VM's on N onto Out. The mcres_t structs on each
 *   VM object are found using the byte offset VMResOffset. This is an
 *   internal function only used by MNodeGetVMRes()
 *
 * Right now, GRes, PSlot, and UnspecifiedPSlots are copied verbatim from N
 * because VM's do not currently have such data, even though they might in the 
 * future.
 *
 * @param N             (I)
 * @param J             (I) [optional]
 * @param ApplyPolicies (I) 
 * @param VMResType     (I) which mcres_t to use.  valid values are:
 *                          mvmaAMem -- use ARes                                       
 *                          mvmaCMem -- use CRes                                       
 *                          mvmaDMem -- use DRes                                       
 * @param Out           (O) required
 */

int MNodeGetOneVMRes(

  const mnode_t *N,
  const mjob_t  *J,
  mbool_t  ApplyPolicies,
  enum MVMAttrEnum  VMResType,
  mcres_t *Out)

  {
  mcres_t tmpRes;
  mln_t const *Link;

  MCResInit(&tmpRes);

  for (Link = N->VMList;Link != NULL;Link = Link->Next)
    {
    mvm_t *V = (mvm_t *)Link->Ptr;
    mcres_t *VMRes = NULL;

    switch (VMResType)
      {
      case mvmaAMem:
        VMRes = &V->ARes;
        break;

      case mvmaCMem:
        VMRes = &V->CRes;
        break;

      case mvmaDMem:
        VMRes = &V->DRes;
        break;

      default:
        return(FAILURE);
      }

    if (bmisset(&V->Flags,mvmfDeleted) ||
        bmisset(&V->Flags,mvmfDestroyed) ||
        bmisset(&V->Flags,mvmfInitializing) ||
        !bmisset(&V->Flags,mvmfCreationCompleted))
      {
      continue;
      }

    if (ApplyPolicies == TRUE)
      {
      if (bmisset(&V->Flags,mvmfSovereign))
        {
        /* VM is sovereign - resources not available to other jobs */

        continue;
        }

      if ((V->OwnerJob != NULL) && (J != NULL) && strcmp(V->OwnerJob,J->Name))
        {
        /* VM not accessible by job */

        continue;
        }

      if (V->RMList != NULL)
        {
        mrm_t   *R;

        mbool_t  RMMatch = FALSE;

        int  rmindex;

        R = NULL;

        if (J != NULL)
          {
          if (J->DestinationRM != NULL)
            R = J->DestinationRM;
          else if (J->SubmitRM != NULL)
            R = J->SubmitRM;
          }

        if (R != NULL)
          {
          switch (R->Type)
            {
            case mrmtPBS:

              for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
                {
                if (V->RMList[rmindex] == NULL)
                  break;

                if (V->RMList[rmindex] == R)
                  {
                  RMMatch = TRUE;

                  break;
                  }
                }    /* END for (rmindex) */

              break;

            default:

              RMMatch = TRUE;

              break;
            }  /* END switch (R->Type) */

          if (RMMatch == FALSE)
            {
            /* RM does not match */

            continue;
            }
          }    /* END if (R != NULL) */
        }      /* END if (V->RMList != NULL) */
      }

    /* VM can provide resources */
    
    MCResPlus(&tmpRes,VMRes);
    }    /* END for (Link) */

  /* VMs currently do not have GRes or PSlot values */

  MSNLCopy(&tmpRes.GenericRes,&N->CRes.GenericRes);

  MCResCopy(Out,&tmpRes);

  MCResFree(&tmpRes);

  return(SUCCESS);
  }  /* END MNodeGetOneVMRes() */





/**
 * Compute configured and available VM resources on N, skipping sovereign VMs.
 * 
 * TODO: should we constrain the VM resources for overcommitted nodes?
 *
 * @see MNodeGetOneVMRes() - child
 *
 * @param N    (I)
 * @param J    (I) [optional]
 * @param ApplyPolicies (I) specifies whether to apply policies to selecting
 *    VMs. Only applies to ARes.
 * @param CRes (O) [optional]
 * @param DRes (O) [optional]
 * @param ARes (O) [optional]
 */

int MNodeGetVMRes(

  const mnode_t  *N,
  const mjob_t   *J,
  mbool_t         ApplyPolicies,
  mcres_t        *CRes,
  mcres_t        *DRes,
  mcres_t        *ARes)

  {
  if (CRes != NULL)
    MNodeGetOneVMRes(N,J,FALSE,mvmaCMem,CRes);

  if (ARes != NULL)
    MNodeGetOneVMRes(N,J,ApplyPolicies,mvmaAMem,ARes);

  if (DRes != NULL)
    MNodeGetOneVMRes(N,J,FALSE,mvmaDMem,DRes);

  return(SUCCESS);
  }  /* END MNodeGetVMRes() */





/**
 * Set VM parent
 *
 * @param VM (I) [modified]
 * @param ParentN (I) [modified]
 */

int MVMSetParent(

  mvm_t   *VM,        /* I (modified) */
  mnode_t *ParentN)   /* I (optional,modified) */

  {
  mln_t   *L;
  mnode_t *OrigN;

  const char *FName = "MVMSetParent";

  MDB(3,fCONFIG) MLog("%s(%s,%s)\n",
    FName,
    (VM != NULL) ? VM->VMID : "NULL",
    (ParentN != NULL) ? ParentN->Name : "NULL");

  if (VM == NULL) 
    {
    return(FAILURE);
    }

  if (VM->N == ParentN)
    {
    /* VM already child of target node */

    /* NO-OP */

    return(SUCCESS);
    }

  OrigN = VM->N;

  /* Step 1.0  Add VM to new parent */

  if (ParentN != NULL)
    {
    if (MULLCheck(ParentN->VMList,VM->VMID,&L) == FAILURE)
      {
      /* VM does not already exist on ParentN - create new record */

      if (MULLAdd(&ParentN->VMList,VM->VMID,NULL,&L,NULL) == FAILURE)
        {
        MDB(1,fRM) MLog("ALERT:    cannot add VM '%s' to node '%s' vmlist\n",
          VM->VMID,
          ParentN->Name);

        return(FAILURE);
        }
      }
    else
      {
      /* existing VM located */

      MDB(1,fRM) MLog("ALERT:    cannot add VM '%s' to node '%s' vmlist - VM already exists on node\n",
        VM->VMID,
        ParentN->Name);

      return(FAILURE);
      }

    L->Ptr = (void *)VM;

    if (MSched.VMGResMap == TRUE)
      {
      /* NOTE:  should create GRes map only if GRes is not workload locked (ie
                not one-time use, no on-demand created) */

      int gindex;

      gindex = MUMAGetIndex(meGRes,VM->VMID,mAdd);

      if ((gindex > 0) && (MSNLGetIndexCount(&ParentN->CRes.GenericRes,gindex) == 0))
        {
        /* add VM map GRes */

        MSNLSetCount(&ParentN->CRes.GenericRes,gindex,1);
        MSNLSetCount(&ParentN->ARes.GenericRes,gindex,1);
        MSNLSetCount(&ParentN->DRes.GenericRes,gindex,0);
        }
      }
    }    /* END if (ParentN != NULL) */

  /* each VM should have entry back to parent node */

  VM->N = ParentN;

  /* Step 2.0  Remove VM from previous parent */

  /* check if VM has been migrated from somewhere else */

  /* remove VM from old node's N->VMList */

  if (OrigN != NULL)
    {
    MULLRemove(&OrigN->VMList,VM->VMID,NULL);

    if (MSched.VMGResMap == TRUE)
      { 
      int gindex;

      gindex = MUMAGetIndex(meGRes,VM->VMID,mVerify);

      if (gindex > 0)
        {
        MSNLSetCount(&OrigN->CRes.GenericRes,gindex,0);
        MSNLSetCount(&OrigN->ARes.GenericRes,gindex,0);
        MSNLSetCount(&OrigN->DRes.GenericRes,gindex,0);
        }
      }
    }

  /* Step 3.0  Update VM Workload */

  if ((VM->J != NULL) && (VM->J != VM->TrackingJ) && (ParentN != NULL))
    {
    /* We don't change the job's allocation if ParentN is NULL because
        the VM is being destroyed and clearing the allocation will mess
        up billing (no procs/mem reported, etc.) */

    mnl_t tmpNL = {0};

    if (VM->J->Rsv != NULL)
      MRsvDeallocateResources(VM->J->Rsv,&VM->J->Rsv->NL);

    /* Remove the job from its current nodes */

    MJobRemoveFromNL(VM->J,NULL);

    MNLInit(&tmpNL);

    /* move system job */

    MNLSetNodeAtIndex(&tmpNL,0,ParentN);
    MNLSetTCAtIndex(&tmpNL,0,1);

    MJobSetAttr(VM->J,mjaAllocNodeList,(void **)&tmpNL,mdfOther,mSet);

    if (VM->J->Req[0] != NULL)
      {
      MReqSetAttr(
        VM->J,
        VM->J->Req[0],
        mrqaAllocNodeList,
        (void **)&tmpNL,
        mdfOther,
        mSet);
      }

    if (!MNLIsEmpty(&VM->J->ReqHList))
      {
      MJobSetAttr(VM->J,mjaReqHostList,(void **)&tmpNL,mdfOther,mSet);
      }

    MJobSetAttr(VM->J,mjaTaskMap,(void **)&tmpNL,mdfOther,mSet);

    MNLFree(&tmpNL);
    }  /* END if (VM->J) != NULL */

  if ((VM->TrackingJ != NULL) && (ParentN != NULL))
    {
    /* We don't change the job's allocation if ParentN is NULL because
        the VM is being destroyed and clearing the allocation will mess
        up billing (no procs/mem reported, etc.) */

    mnl_t tmpNL = {0};
    mbool_t UpdateRsv = FALSE;

    if (VM->TrackingJ->Rsv != NULL)
      {
      UpdateRsv = TRUE;

      MRsvDeallocateResources(VM->TrackingJ->Rsv,&VM->TrackingJ->Rsv->NL);
      }

    /* Remove the job from its current nodes */

    MJobRemoveFromNL(VM->TrackingJ,NULL);

    MNLInit(&tmpNL);

    /* move system job */

    MNLSetNodeAtIndex(&tmpNL,0,ParentN);
    MNLSetTCAtIndex(&tmpNL,0,1);

    MJobSetAttr(VM->TrackingJ,mjaAllocNodeList,(void **)&tmpNL,mdfOther,mSet);

    if (VM->TrackingJ->Req[0] != NULL)
      {
      MReqSetAttr(
        VM->TrackingJ,
        VM->TrackingJ->Req[0],
        mrqaAllocNodeList,
        (void **)&tmpNL,
        mdfOther,
        mSet);
      }

    if (!MNLIsEmpty(&VM->TrackingJ->ReqHList))
      {
      MJobSetAttr(VM->TrackingJ,mjaReqHostList,(void **)&tmpNL,mdfOther,mSet);
      }

    MJobSetAttr(VM->TrackingJ,mjaTaskMap,(void **)&tmpNL,mdfOther,mSet);

    if (UpdateRsv == TRUE)
      {
      /* Need to update the nodelist on the tracking job's reservation */

      mnl_t *MNodeList2[MMAX_REQ_PER_JOB];

      MNLMultiInit(MNodeList2);
      MNLCopy(MNodeList2[0],&tmpNL);
      MNLTerminateAtIndex(MNodeList2[1],0);

      MJobReleaseRsv(VM->TrackingJ,TRUE,FALSE);
      MRsvJCreate(VM->TrackingJ,MNodeList2,VM->TrackingJ->StartTime,mjrActiveJob,NULL,FALSE);

      MNLMultiFree(MNodeList2);
      }

    MNLFree(&tmpNL);
    }  /* END if (VM->TrackingJ) != NULL */

  /* Step 4.0  Update VM stats */

  /* NOTE:  update VM if reported either in VARATTRS or as a resource */

  VM->UpdateIteration = MSched.Iteration;
  VM->UpdateTime      = MSched.Time;

  return(SUCCESS);
  }  /* END MVMSetParent() */





/**
 * This is an application of copy-paste template meta-programming.
 *
 * @see MNodeAddRM() - sync
 *
 */

int MVMAddRM(

  mvm_t *VM,     /* I (modified) */
  mrm_t *R)      /* I */

  {
  int rmindex;

  if ((VM == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if ((VM->RMList == NULL) &&
     ((VM->RMList = (mrm_t **)MUCalloc(1,sizeof(mrm_t *) * (MMAX_RM + 1))) == NULL))
    {
    return(FAILURE);
    }

  for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
    {
    if (VM->RMList[rmindex] == NULL)
      {
      VM->RMList[rmindex] = R;

      break;
      }

    if (VM->RMList[rmindex] == R)
      break;
    }  /* END for (rmindex) */

  if (rmindex >= MMAX_RM)
    {
    return(FAILURE);
    }

  if (VM->RM == NULL)
    {
    /* only add as master RM if N->RM is not set */

    VM->RM = R;
    }

  return(SUCCESS);
  }  /* END MVMAddRM() */


#if 0

/**
 * Destroys all VMs in VMTaskMap. Handles the case of duplicate entries in
 * the task map.
 *
 *@param VMTaskMap (I) null-terminated
 */

int MVMDestroyInTaskMap(

    mvm_t **VMTaskMap)

  {
  marray_t JobArray;
  int vmindex;

  MUArrayListCreate(&JobArray,sizeof(mjob_t *),1);

  for (vmindex = 0;VMTaskMap[vmindex] != NULL;vmindex++)
    {
    mvm_t *VM = VMTaskMap[vmindex];
    mnl_t tmpNL;
    int IsDuplicate = FALSE;
    int vmindex2;

    mjob_t **JList;
    mjob_t  *tmpJ;

    for (vmindex2 = vmindex - 1;vmindex2 >= 0;vmindex2--)
      {
      if (VMTaskMap[vmindex2] == VMTaskMap[vmindex])
        {
        IsDuplicate = TRUE;

        break;
        }
      }

    if (IsDuplicate == TRUE)
      continue;

    MNLInit(&tmpNL);

    MNLSetNodeAtIndex(&tmpNL,0,VM->N);
    MNLSetTCAtIndex(&tmpNL,0,1);

    if (MUGenerateSystemJobs(
        NULL,
        NULL,
        &tmpNL,
        msjtVMDestroy,
        "vmdestroy",
        -1,
        VM->VMID,
        VM,
        FALSE,
        FALSE,
        NULL,
        &JobArray) == FAILURE)
      {
      /* cannot destroy VM */

      MNLFree(&tmpNL);

      MDB(3,fRM) MLog("ALERT:    cannot destroy VM '%s'",
        VM->VMID);

      continue;
      }

    MNLFree(&tmpNL);

    JList = (mjob_t **)JobArray.Array;
    tmpJ = JList[0];

    if (!MNLIsEmpty(&tmpJ->ReqHList))
      {
      mnode_t *N = MNLReturnNodeAtIndex(&tmpJ->ReqHList,0);

      MNLSetNodeAtIndex(&tmpJ->Req[0]->NodeList,0,N);
      MNLSetNodeAtIndex(&tmpJ->NodeList,0,N);

      MNLSetTCAtIndex(&tmpJ->NodeList,0,1);
      MNLSetTCAtIndex(&tmpJ->Req[0]->NodeList,0,1);
      }

    /* do NOT add any vmdestroy jobs to the "ready to run"
     * list as we want users to have the option of placing holds
     * on them via templates, etc. before a VM is destroyed */

    /* MULLPrepend(&MJobsReadyToRun,tmpJ->Name,tmpJ); */

    JList[0] = NULL;
    JobArray.NumItems = 0;
    }    /* END for (vmindex) */

  MUArrayListFree(&JobArray);

  return (SUCCESS);
  } /* END MVMDestroyInTaskMap() */
#endif

/**
 * Update VM utilization information
 *
 * @see MWikiVMUpdate() - parent - called once per iteration when VM info is
 *                                 updated via WIKI interface
 *
 * @param V (I) [modified]
 */

int MVMUpdate(

  mvm_t *V)

  {
  if (V == NULL)
    {
    return(FAILURE);
    }

  /* update VM state */
   
  /* ie, installing, initializing, ... */

  /* NYI */

  if (V->StartTime == 0)
    {
    /* Check if the VM is up yet, currently done by checking state */
    /* VM is marked as down until ready */

    if ((V->State == mnsIdle) ||
        (V->State == mnsActive))
      {
      V->StartTime = MSched.Time;
      }
    }

  /* update VM utilization history */

  if ((V->J != NULL) && (V->J->Req[0] != NULL) && (V->J->Req[0]->LURes != NULL))
    {
    /* NOTE: assumes 'singlejob' dedicated VM access */

    /* NOTE:  should determine number of tasks running w/in VM (NYI) */

    MCResCopy(&V->LURes,V->J->Req[0]->LURes);
    }

  if ((V->EffectiveTTL > 0) &&
      (V->StartTime > 0) &&
      ((MSched.Time - V->StartTime) >= (mulong)V->EffectiveTTL) &&
      !bmisset(&V->Flags,mvmfDestroyPending) &&
      !bmisset(&V->Flags,mvmfDestroyed) &&
      !bmisset(&V->IFlags,mvmifDeletedByTTL))
    {
    /* VM's TTL has expired.  Post notice. */

    char tmpLine[MMAX_LINE];
    char TString[MMAX_LINE];

    MULToTString(V->EffectiveTTL,TString);

    snprintf(tmpLine,sizeof(tmpLine),"WARNING:  VM '%s' has reached TTL (%s).  Must be removed manually.\n",
      TString,
      V->VMID);

    MDB(1,fSTRUCT) MLog(tmpLine);

    if (V->N)
      MMBAdd(&V->N->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

    }  /* END if ((V->EffectiveTTL > 0) && (MSched.Time - V->StartTime >= V->EffectiveTTL)) */

  return(SUCCESS);
  }  /* END MVMUpdate() */


/**
 * This function will take a "action" XML sub-tree, parse out
 * the elements, and add it to the VM.
 *
 * @param VM (I) The mvm_t object whose variables need updated.
 * @param E  (I) The XML sub-tree containing the elements.
 */

int MVMActionFromXML(
  
  mvm_t   *VM,
  mxml_t  *E)

  {
  char Attr[MMAX_LINE];

  if ((E == NULL) || (VM == NULL))
    return(FAILURE);

  if (MXMLGetAttr(E,"id",NULL,Attr,sizeof(Attr)) == SUCCESS)
    {
    return(MULLAdd(&VM->Action,Attr,NULL,NULL,MUFREE));
    }

  return(FAILURE);
  }  /* END MVMActionFromXML() */


/**
 * This function will take a "Variables" XML sub-tree, parse out the variables,
 * and add it to the variable hash table attached to given VM. WARNING: This will
 * remove any already existing variables!
 *
 * NOTE: This function *could* be generalized and used for node and reservation variables as well.
 * Job variables could also be done this way, but they have more complex parsing due to the trigger
 * modifiers that can be applied to them.
 *
 * @see MUAddVarsToXML()
 * @param VM (I) The mvm_t object whose variables need updated.
 * @param VarListE (I) The XML sub-tree containing the variables.
 */

int MVMVarsFromXML(
  
  mvm_t   *VM,
  mxml_t  *VarListE)

  {
  char VarName[MMAX_LINE];
  char *VarValue = NULL;
  mxml_t *VarE = NULL;
  int VarETok = -1;

  if ((VarListE == NULL) ||
      (VM == NULL))
    {
    return(FAILURE);
    }

  if (strcasecmp(VarListE->Name,"Variables"))
    return(FAILURE);  /* not correct element type */

  /* clear out existing variables */
  MUHTClear(&VM->Variables,TRUE,MUFREE);

  /* iterate through children and extract info to build up the variables */
  while (MXMLGetChildCI(VarListE,"Variable",&VarETok,&VarE) == SUCCESS)
    {
    VarValue = NULL;

    MXMLGetAttr(VarE,"name",NULL,VarName,sizeof(VarName));
    MUStrDup(&VarValue,VarE->Val);

    MUHTAdd(&VM->Variables,VarName,(void *)VarValue,NULL,NULL);
    }

  return(SUCCESS);
  }  /* END MVMVarsFromXML() */


#if 0
/*
 * Creates the VM creation job from the given job.  Given job may be an msub-submitted job with
 *  a VM usage policy of VMCreate, or a VMTracking system job (submitted via mvmctl -c).
 *
 * @see MAdaptiveJobStart - parent
 *
 * @param J   (I) - Job to create VMs from
 * @param Msg (O) - Msg buffer for errors
 */

int MVMCreateVMFromJob(

  mjob_t *J,
  char   *Msg)

  {
  char    *EMsg;
  char     tmpMsg[MMAX_LINE];
  char     tmpLine[MMAX_LINE];
  int      SC;
  marray_t VMCRList;
  int      RQIndex;
  int      jindex;
  int      vmindex;
  mrsv_t  *ProcRsv = NULL;
  mvm_req_create_t *VMCRArr;

  mjob_t  *SubJob;
  int      SubJobSubmitRC = SUCCESS;
  mbitmap_t   JStartBM;  /* bitmap of job start state */

  marray_t JobArray;
  mjob_t **JList;

  mjob_t  *StorageJ = NULL;

  mbool_t  ModifyRMJob = TRUE;  /* hardcode for now - route in later */

  char     vmid[MMAX_NAME];

  int      rc;

  const char *FName = "MVMCreateVMFromJob";

  MDB(5,fSTRUCT) MLog("%s(%s,Msg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  /* NOTE:  J->VMTaskMap[] should be allocated but empty on initial MJobStart()
            call for VMCreate job */

  /* check here for NULL VMTaskMap and allocated it if needed a temporary 
     hack fix for ML */

  if (J == NULL)
    return(FAILURE);

  if (Msg != NULL)
    EMsg = Msg;
  else
    EMsg = tmpMsg;


  /* TODO: per addition of MReqGetOpsys(), we may need to either change 
   * MVMCreateReqFromJob to allow one to pass in a ReqOS, change
   * MReqGetReqOpsys to modify J->Req[0]->Opsys and assign it 
   * the computed OS, or change MVMCreateReqFromJob to use MReqGetOpsys
   * separately */

  if (MVMCreateReqFromJob(J,&VMCRList,FALSE,tmpLine) == FAILURE)
    {
    snprintf(EMsg,MMAX_LINE,"ERROR:    cannot create VMs for job %s: %s\n",
      J->Name,
      tmpLine);

    MDB(7,fSTRUCT) MLog("ERROR:    cannot create VMs for job %s: %s\n",
      J->Name,
      tmpLine);

    MJobProcessFailedStart(J,&JStartBM,mhrPreReq,EMsg);

    return(FAILURE);
    }

  VMCRArr = (mvm_req_create_t *)VMCRList.Array;

  for (vmindex = 0;vmindex < VMCRList.NumItems;vmindex++)
    {
    VMCRArr[vmindex].IsSovereign = FALSE;

    if (J->Req[0]->NodeList.Array[0].N != NULL)
      {
      MUStrCpy(VMCRArr[vmindex].HVName,J->Req[0]->NodeList.Array[0].N->Name,sizeof(VMCRArr[vmindex].HVName));
      }
    }

  MUArrayListCreate(&JobArray,sizeof(mjob_t *),10);

  /* ready to launch VM create job - process optional VM creation modifiers */

  /* extract VM id */

  if (MUStringGetEnvValue(J->Env.BaseEnv,"MOAB_VMID",',',vmid,sizeof(vmid)) == SUCCESS)
    {
    char *ptr;

    /* FORMAT:  [<...>;]MOAB_VMID=<...>[;...] */

    mjob_t *SJ;


    if (!strncasecmp(vmid,"application:",strlen("application:")))
      {
      /* indirect VM referenced */

      ptr = vmid + strlen("application:");

      /* allow specification either as a job instance id of a jobname */

      if ((MJobFind(ptr,&SJ,mjsmExtended) == FAILURE) &&
          (MJobCFind(ptr,&SJ,mjsmBasic) == FAILURE))
        {
        /* cannot locate specified application */

        if (EMsg != NULL)
          strcpy(EMsg,"cannot locate specified application");

        return(FAILURE);
        }

      if (MUStringGetEnvValue(SJ->Env.BaseEnv,"MOAB_VMID",',',vmid,sizeof(vmid)) == SUCCESS)
        {
        snprintf(tmpLine,sizeof(tmpLine),"%s=%s",
          "MOAB_VMID",
          vmid);
        }
      else
        {
        /* specified application does not utilize dynamic VM */

        if (EMsg != NULL)
          strcpy(EMsg,"specified application does not utilize dynamic VM");

        return(FAILURE);
        }

      /* NOTE:  push VM ID into job's variable space and execution environment */

      MJobSetAttr(J,mjaVariables,(void **)tmpLine,mdfString,mAdd);

      MJobSetAttr(J,mjaEnv,(void **)tmpLine,mdfString,mAdd);

      if (ModifyRMJob == TRUE)
        {
        char tEMsg[MMAX_LINE];

        /* push env into RM job */

        MJobAddEnvVar(J,"MOAB_VMID",vmid,',');

        MRMJobModify(
          J,
          "Variable_List",
          NULL,
          J->Env.BaseEnv,
          mSet,
          NULL,
          tEMsg,
          NULL);
        }

      return(SUCCESS);
      }  /* END if (!strncasecmp(vmid,"application:",...)) */
    else
      {
      /* VM already specified w/in job - re-use existing VM */

      /* NOTE:  should validate referenced VM is available - NYI */

      snprintf(tmpLine,sizeof(tmpLine),"%s=%s",
        "MOAB_VMID",
        vmid);

      /* NOTE:  push VM ID into job's variable space and execution environment */

      MJobSetAttr(J,mjaVariables,(void **)tmpLine,mdfString,mAdd);

      return(SUCCESS);
      }
    }  /* END if ((MUStringGetEnvValue(J->Variables,"MOAB_VMID",&tmpL) == SUCCESS) && ...) */

    {
    char *Storage;
    mrsv_t *AdvRsvR = NULL;

    if (J->Req[1] != NULL)
      {
      mnl_t StorageNL;

      /* do storage (if any) */

      /* Combine nodelist from each req */

      RQIndex = 1;

      MNLInit(&StorageNL);

      while (J->Req[RQIndex] != NULL)
        {
        MNLMerge2(&StorageNL,&J->Req[RQIndex]->NodeList,FALSE);

        RQIndex++;
        }

      if (MVMCreateStorageJob(&StorageNL,(mvm_req_create_t *)&VMCRList.Array[0],&StorageJ) == FAILURE)
        {
        return(FAILURE);
        }

      /* Also create reservations to save the GRES and procs while creating VM */

      if (MVMCreateTempStorageRsvs(J,&StorageNL,&ProcRsv,EMsg) == FAILURE)
        {
        MJobProcessFailedStart(J,&JStartBM,mhrNoResources,EMsg);

        MNLFree(&StorageNL);

        return(FAILURE);
        }

      MNLFree(&StorageNL);
      }  /* END if (J->Req[1] != NULL) */
    else if (MUHTGet(&J->Variables,"VMSTORAGE",(void **)&Storage,NULL) == SUCCESS)
      {
      mvm_req_create_t *VMCR = NULL;

      /* this model does not accomodate multi-vm jobs */
      assert(VMCRList.NumItems == 1);

      VMCR = (mvm_req_create_t *)VMCRList.Array;

      MUStrCpyQuoted(VMCR->Storage,Storage,sizeof(VMCR->Storage));

      /* determine the hypervisor node for vlan consideration */
      if (VMCR->N == NULL) 
        {
        /* again, only single-vm jobs are supported */
        if ((MNLReturnNodeAtIndex(&J->NodeList,0) == NULL) ||
            (MNLReturnNodeAtIndex(&J->NodeList,1) != NULL))
          {
          return(FAILURE);
          }

        VMCR->N = MNLReturnNodeAtIndex(&J->NodeList,0);
        }

      if (MVMCreateStorage(
          VMCR,
          &AdvRsvR,
          &StorageJ,
          tmpLine,
          sizeof(tmpLine),
          NULL) == FAILURE)
        {
        strcpy(EMsg,"error in storage provisioning");

        MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

        return(FAILURE);
        } /* END if MVMCreateStorage(...) == FAILURE */

      if (StorageJ->System->ProxyUser == NULL)
        {
        /* if no proxy user explicitly given, use the master job's username */

        MUStrDup(&StorageJ->System->ProxyUser,J->Credential.U->Name);
        }
      }  /* END else if (MULLCheck(J->Variables,"VMSTORAGE,...) != FAILURE) */
    }

  /* create all required vmcreate system jobs */

  for (RQIndex=0;J->Req[RQIndex] != NULL;RQIndex++)
    {
    mvm_req_create_t *VMCR = (mvm_req_create_t *)VMCRList.Array;

    /* Schedule the vmcreate using the Req that has Procs.
     *
     * This maybe a multi-req job for the vmcreate and storage job.  Each Req will have a nodelist
     * at J->Req[index]->NodeList.  The vmcreate requests DRes.Procs > 0 while the storage requests zero procs. 
     * See ticket MOAB-1336.  */

    if (J->Req[RQIndex]->DRes.Procs <= 0)
      continue;

    MUArrayListClear(&JobArray);

    rc = MUGenerateSystemJobs(
      ((VMCR != NULL) && (VMCR->U != NULL)) ? VMCR->U->Name : NULL,
      J,
      &J->Req[RQIndex]->NodeList,
      msjtVMCreate,
      "vmcreate",
      J->Req[RQIndex]->Opsys,
      NULL,
      VMCRList.Array,
      FALSE,
      TRUE,
      NULL,
      &JobArray);  /* O */

    if ((rc == FAILURE) || (JobArray.NumItems == 0))
      {
      strcpy(EMsg,"error in provisioning");
  
      MDB(3,fSTRUCT) MLog("ERROR:    error in creating vmcreate job for VM %s%s%s requested by job '%s'\n",
        (J->VMCreateRequest != NULL) ? "(" : "",
        (J->VMCreateRequest != NULL) ? J->VMCreateRequest->VMID : "",
        (J->VMCreateRequest != NULL) ? ")" : "",
        J->Name);
  
      MJobProcessFailedStart(J,&JStartBM,mhrNoResources,EMsg);

      MUArrayListFree(&VMCRList);

      return(FAILURE);
      }

    /* immediately start all vmcreate system jobs */
  
    JList = (mjob_t **)JobArray.Array;
  
    for (jindex = 0;jindex < JobArray.NumItems;jindex++)
      {
      SubJob = JList[jindex];
  
      SubJob->System->VMJobToStart = strdup(J->Name);
  
      if (SubJob->System->ProxyUser == NULL)
        {
        if ((J->System != NULL) &&
            (J->System->ProxyUser != NULL))
          {
          MUStrDup(&SubJob->System->ProxyUser,J->System->ProxyUser);
          }
        else
          {
          /* if no proxy user explicitly given, use the master job's username */

          MUStrDup(&SubJob->System->ProxyUser,J->Credential.U->Name);
          }
        }
  
      bmset(&J->IFlags,mjifHasInternalDependencies);
  
      /* swap out J and J->JGroup->SubJobs */
  
      if (StorageJ != NULL)
        MJobSetDependency(SubJob,mdJobSuccessfulCompletion,StorageJ->Name,NULL,0,FALSE,NULL);
  
      if (ProcRsv != NULL)
        {
        /* Add create job to the proc reservation so that it can start */
        macl_t *Ptr = NULL;
   
        Ptr = (macl_t *)MUCalloc(1,sizeof(macl_t));
        if (Ptr)
          {
          MACLInit(Ptr);
   
          MACLSet(&Ptr,maJob,SubJob->Name,mcmpSEQ,mnmNeutralAffinity,0,0);
          MRsvSetAttr(ProcRsv,mraACL,(void *)Ptr,mdfOther,mAdd);
          }

        /* Make the VM create job use the reservation */
        bmset(&SubJob->Flags,mjfAdvRsv);
        bmset(&SubJob->SpecFlags,mjfAdvRsv);
   
        MUStrDup(&SubJob->ReqRID,ProcRsv->Name);

        MACLFreeList(&Ptr);
        }
  
      /* add dependency between compute job and vmcreate system job */
  
      MJobSetDependency(J,mdJobSuccessfulCompletion,SubJob->Name,NULL,0,FALSE,NULL);
  
      MNLSetNodeAtIndex(&SubJob->Req[0]->NodeList,0,MNLReturnNodeAtIndex(&SubJob->ReqHList,0));
      MNLSetTCAtIndex(&SubJob->Req[0]->NodeList,0,1);
  
      SubJob->System->RecordEventInfo = MUStrFormat("One time use VM for job %s",
        J->Name);
  
      if (StorageJ == NULL)
        {
        /* start vmcreate system job - can't start yet if storage present */
  
        if ((SubJobSubmitRC = MJobStart(SubJob,NULL,&SC,EMsg)) == FAILURE)
          {
          break;
          }
        }
      }    /* END for (jindex) */
  
    if (StorageJ != NULL)
      {
      MJobStart(StorageJ,NULL,&SC,EMsg);
      }
    else if (SubJobSubmitRC == SUCCESS)
      {
      /* create reservation for vmcreate compute job */
  
      bmset(&J->IFlags,mjifReqTimeLock);
      bmset(&J->IFlags,mjifHasCreatedEnvironment);
  
      MRsvJCreate(
        J,
        NULL,
        MSched.Time + JList[0]->SpecWCLimit[0],
        mjrTimeLock,
        NULL,
        FALSE);
      }
    } /* END for (RQIndex) */

  MUArrayListFree(&VMCRList);
  MUArrayListFree(&JobArray);

  return(SubJobSubmitRC);
  }  /* END MVMCreateVMFromJob() */
#endif

#if 0

/*
 * Checks if a node is feasible, given the requirements in VMCR.
 *
 * @param N        (I) - Node to check
 * @param VMCR     (I) - Description to check against
 * @param EMsg     (O) [optional] - Error message
 * @param EMsgSize (I) [optional] - Size of EMsg
 */

int MVMCRCheckNodeFeasibility(

  mnode_t *N,
  mvm_req_create_t *VMCR,
  char *EMsg,
  int   EMsgSize)

  {
  mcres_t AvlRes;
  mcres_t VMCRes;
  mcres_t VMDRes;

  char tmpLine[MMAX_LINE];

  if ((N == NULL) || (VMCR == NULL))
    return(FAILURE);

  if ((N->HVType == mhvtNONE) || !bmisset(&N->IFlags,mnifCanCreateVM))
    {
    snprintf(EMsg,EMsgSize,"target hypervisor does not support VM creation\n");

    return(FAILURE);
    }

  if (N->ARes.Procs < MAX(1,VMCR->CRes.Procs))
    {
    snprintf(EMsg,EMsgSize,"inadequate available procs on server to create VM\n");

    return(FAILURE);
    }

  if (MUOSListContains(N->VMOSList,VMCR->OSIndex,NULL) == FALSE)
    {
    snprintf(EMsg,EMsgSize,"target hypervisor does not support specified VM image\n");

    return(FAILURE);
    }

  MCResInit(&VMCRes);
  MCResInit(&VMDRes);
  MNodeGetVMRes(N,NULL,FALSE,&VMCRes,&VMDRes,NULL);

  memset(&AvlRes,0,sizeof(AvlRes));

  /* sync with calcuation in MJobGetINL for vmcreate and createvm jobs */

  MCResPlus(&AvlRes,&N->CRes);
  MCResMinus(&AvlRes,&N->DRes);
  MCResPlus(&AvlRes,&VMDRes);
  MCResMinus(&AvlRes,&VMCRes);

  MCResFree(&VMCRes);
  MCResFree(&VMDRes);

  if (MCResCanOffer(&AvlRes,&VMCR->CRes,TRUE,tmpLine) == FALSE)
    {
    snprintf(EMsg,EMsgSize,"inadequate available resources on node to create VM - %s\n",
      tmpLine);

    return(FAILURE);
    }

  if (N->ARes.Mem <= MAX(1,VMCR->CRes.Mem))
    {
    snprintf(EMsg,EMsgSize,"inadequate available memory on node to create VM\n");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MVMCRCheckNodeFeasibility() */
#endif



#if 0
/**
 * Takes care of submitting a VMDestroy job for a VM.  Returns SUCCESS if a
 *  VMDestroy job was already created or if this function succeeds in creating
 *  one.  This means that the function can return SUCCESS and J still be NULL.
 *
 * NOTE:  This is only for a VM that was created via mvmctl, not a workflow
 * NOTE:  The VM must already be up and running.
 *  
 * @param Auth [I] - Authentication User Name. 
 * @param VM [I] - The VM to be destroyed
 * @param J  [O] (optional) - the destroy job that was created
 */

int MVMSubmitDestroyJob(

  const char *Auth, /* I */
  mvm_t      *VM,  /* I */
  mjob_t    **J)   /* O (optional) */

  {
  marray_t JArray;
  mjob_t *DestroyJ;

  mnl_t tmpNL;

  if ((VM == NULL) || 
      ((!bmisset(&VM->IFlags,mvmifUnreported)) && 
       (!bmisset(&VM->IFlags,mvmifReadyForUse))))
    return(FAILURE);

  if (J != NULL)
    *J = NULL;

  if ((bmisset(&VM->Flags,mvmfDestroySubmitted)) ||
      ((VM->TrackingJ != NULL) &&
       (bmisset(&VM->TrackingJ->IFlags,mjifVMDestroyCreated))))
    {
    /* Destroy job already submitted */

    return(SUCCESS);
    }

  /* Set up node list for VMDestroy job */

  MNLInit(&tmpNL);
  MNLSetNodeAtIndex(&tmpNL,0,VM->N);
  MNLSetTCAtIndex(&tmpNL,0,VM->CRes.Procs);
  MNLTerminateAtIndex(&tmpNL,1);

  MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

  if (MUGenerateSystemJobs(
     Auth,
     NULL,
     &tmpNL,
     msjtVMDestroy,
     "vmdestroy",
     0,
     VM->VMID,
     VM,
     FALSE,
     FALSE,
     FALSE,
     &JArray) == FAILURE)
    {
    MDB(3,fSTRUCT) MLog("ERROR:    failed to create vmdestroy job for VM '%s' in vmcreate cancellation request\n",
      VM->VMID);

    MNLFree(&tmpNL);

    MUArrayListFree(&JArray);

    return(FAILURE);
    }

  DestroyJ = *(mjob_t **)MUArrayListGet(&JArray,0);

  MNLFree(&tmpNL);

  MUArrayListFree(&JArray);

  if (DestroyJ == NULL)
    return(FAILURE);

  if (VM->TrackingJ != NULL)
    bmset(&VM->TrackingJ->IFlags,mjifVMDestroyCreated);

  if (J != NULL)
    *J = DestroyJ;

  return(SUCCESS);
  }  /* END MVMSubmitDestroyJob */
#endif



/**
 * Adds a Generic Event (GEvent) for a specific VM.
 *
 * @param V (I)  Pointer to VM 
 * @param geventId (I) Pointer to event id, like 
 *                 "migrate_message", "hitemp"
 * @param severity (I) Message Severity.
 * @param msg (I)
 */

int MVMAddGEvent(

  mvm_t       *V,
  char        *geventId,
  int          severity,
  const char  *msg)

  {
  mgevent_obj_t  *GEvent;
  mgevent_desc_t *GEDesc;

  if ((V == NULL) || (MUStrIsEmpty(geventId)))
    return(FAILURE);

  /* Add the event id to the system gevents. */

  MGEventAddDesc(geventId);

  MGEventGetOrAddItem(geventId,&V->GEventsList,&GEvent);

  MGEventGetDescInfo(geventId,&GEDesc);

  GEvent->Severity = severity;

  MUStrDup(&GEvent->GEventMsg,msg);

  if (GEvent->GEventMTime + GEDesc->GEventReArm <= MSched.Time)
    {
    /* process event as new */

    GEvent->GEventMTime = MSched.Time;

    if (MGEventProcessGEvent(
        -1,
        mxoxVM,
        V->VMID,
        geventId,
        0.0,
        mrelGEvent,
        msg) == FAILURE)
      {
      MDB(7,fSTRUCT) MLog("ERROR:    MVMAddGEvent could not process event\n");

      return(FAILURE);
      }
    }

  return(SUCCESS);
  } /* END MVMAddGEvent */


/**
 * Clears all the Generic Events (GEvent) for a specific VM.
 *
 * @param V (I)  Pointer to VM 
 */

int MVMClearGEvents(

  mvm_t *V)   /* I */

  {
  char            *GEName;
  mgevent_obj_t   *GEvent;
  mgevent_iter_t   GEIter;

  if (V == NULL) 
    return(FAILURE);

  if (MGEventGetItemCount(&V->GEventsList) == 0)
    return(SUCCESS);

  MGEventIterInit(&GEIter);

  while (MGEventItemIterate(&V->GEventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
    {
    MGEventRemoveItem(&V->GEventsList,GEName);
    MUFree((char **)&GEvent->GEventMsg);
    MUFree((char **)&GEvent->Name);
    MUFree((char **)&GEvent);
    }

  return(SUCCESS);
  } /* END MVMClearGEvents */




/**
 * Load CheckPoint state
 *
 * @param   VM   (I)  [modified]
 * @param   SBuf (I)
 */

int MVMLoadCP(

  mvm_t *VM,
  const char  *SBuf)

  {
  char tmpName[MMAX_NAME + 1];
  char VMID[MMAX_NAME + 1];

  int   rc;
  const char *ptr;

  long    CkTime = 0;
  mxml_t *E = NULL;

  mvm_t  *V = NULL;

  const char *FName = "MVMLoadCP";

  MDB(4,fCKPT) MLog("%s(VM,%s)\n",
    FName,
    (SBuf != NULL) ? SBuf : "NULL");

  if (SBuf == NULL)
    return(FAILURE);

  rc = sscanf(SBuf,"%64s %64s %ld",
    tmpName,
    VMID,
    &CkTime);

  if (rc != 3)
    return(FAILURE);

  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    return(SUCCESS);

  if ((ptr = strchr(SBuf,'<')) == NULL)
    return(FAILURE);

  if (VM != NULL)
    {
    if (MVMFind(VMID,&V) != SUCCESS)
      {
      if (MVMFindCompleted(VMID,&V) != SUCCESS)
        {
        MDB(5,fCKPT) MLog("INFO:     VM '%s' no longer detected\n",
          VMID);

        return(SUCCESS);
        }
      }
    }  /* END if (VM != NULL) */
  else
    {
    if (MVMAdd(VMID,&V) != SUCCESS)
      return(FAILURE);
    }

  if ((MXMLFromString(&E,ptr,NULL,NULL) == FAILURE) ||
      (MVMFromXML(V,E) == FAILURE))
    {
    MXMLDestroyE(&E);

    if (VM == NULL)
      MVMDestroy(&V,NULL);

    return(FAILURE);
    }

  MXMLDestroyE(&E);

  if (VM == NULL)
    {
    /* Add the created VM */
    if (bmisset(&V->Flags,mvmfCompleted))
      {
      /* Add to completed table */

      MVMComplete(V);
      }
    else
      {
      /* Add to normal table */

      MUHTAdd(&MSched.VMTable,VMID,(void *)V,NULL,NULL);
      }

    /* Set update time to now */

    V->UpdateTime = MSched.Time;
    V->UpdateIteration = MSched.Iteration;
    }  /* END if (VM != NULL) */

  return(SUCCESS);
  }  /* MVMLoadCP() */



/**
 * Sets the state of a VM.
 *
 * @param V (I) Pointer to VM. 
 * @param State (I) ENum representation of the VM State. 
 * @param strState (I) String representation of the VM State. 
 *                 (Idle,Busy,Down,Drained)
 */

int MVMSetState(

  mvm_t              *V,
  enum MNodeStateEnum State,
  const char         *strState)

  {

  /* Validate VM pointer */

  if (V == NULL)
    return(FAILURE);

  /* Check to see if the State in string format. */

  if (strState != NULL)
    {
    State = (enum MNodeStateEnum)MUGetIndexCI(strState,MNodeState,FALSE,0);

    if (State == mnsNONE)
      return(FAILURE);
    }

  if (V->State != State)
    {
    V->State = State;

    MOReportEvent((void *)V,V->VMID,mxoxVM,mttModify,MSched.Time,TRUE);

    /* Clear the initializing flag for State: Idle, Busy, Active, or UP */

    switch (State)
      {
      case mnsIdle:
      case mnsBusy:
      case mnsActive:
      case mnsUp:
        bmunset(&V->Flags,mvmfInitializing);
        break;

      default:
        /* Do nothing */

        break;
      }
    }

  return(SUCCESS);
  }  /* END MVMSetState() */

/* END MVM.c */
