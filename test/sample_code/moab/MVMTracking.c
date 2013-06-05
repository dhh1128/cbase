/* HEADER */

/**
 * @file MVMTracking.c
 * 
 * Contains VM Tracking functions 
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 *  Helper function for updating the CL.  Does no error checking.
 *
 * @param J    (I) [modified] - modify the CL on this job
 * @param Type (I) - Type of CL to modify
 * @param Val  (I) - Value to set it to
 */

int __MVMUpdateVMTrackingCL(
  mjob_t *J,
  enum MAttrEnum Type,  
  int Val)
  {
  macl_t *tmpACL = NULL;
  macl_t ACL;

  tmpACL = J->Credential.CL;

  while (tmpACL != NULL)
    {
    if (tmpACL->Type != Type)
      {
      tmpACL = tmpACL->Next;

      continue;
      }

    tmpACL->LVal = (long)Val;

    return(SUCCESS);
    }  /* END while (tmpACL != NULL) */

  /* Didn't have an existing one, need to add */

  MACLInit(&ACL);

  ACL.Affinity = mnmPositiveAffinity;
  ACL.Type = Type;
  ACL.Cmp = mcmpEQ;
  ACL.LVal = (long)Val;

  MACLSetACL(&J->Credential.CL,&ACL,TRUE);

  return(SUCCESS);
  }  /* END __VMUpdateVMTrackingCL() */


/**
 * Helper function for MVMUpdateVMTracking.  Chooses how to appropriately
 *  update job procs (procs/tasks are different for VMTracking jobs
 *  submitted via workflows and jobs submitted with mvmctl).
 *
 * If the job is already multi-proc, this function will update the job
 *  in the same format that it already had procs.  If only 1 proc/task,
 *  the job will be updated according to how workflow jobs are created.
 *
 * Note that NO PARAMETER CHECKING is performed.
 *
 * @see MVMUpdateVMTracking() - parent
 *
 * @param VM (I) - The VM whose VMTracking job is being updated
 * @param J  (I) - The VMTracking job
 * @param TaskCount (O) - Resulting TaskCount
 * @param ResChange (O) - set to TRUE if procs changed
 */

int __MVMUpdateVMTrackingProcs(
  mvm_t *VM,         /* I */
  mjob_t *J,         /* I */
  int *TaskCount,    /* O */
  mbool_t *ResChange)/* O */

  {
  mvc_t *WorkflowVC = NULL;
  *TaskCount = 1;

  MVCGetJobWorkflow(J,&WorkflowVC);

  if (WorkflowVC == NULL)
    {
    /* mvmctl method */

    if (J->Req[0]->DRes.Procs != VM->CRes.Procs)
      {
      J->Req[0]->DRes.Procs = VM->CRes.Procs;

      __MVMUpdateVMTrackingCL(J,maProc,VM->CRes.Procs);

      *ResChange = TRUE;
      }

    J->Req[0]->TaskCount = 1;

    *TaskCount = 1;
    }
  else
    {
    /* workflow msub method */

    if (J->Req[0]->TaskCount != VM->CRes.Procs)
      {
      int nindex = 0;

      if (J->NodeList.Array == NULL)
        return(FAILURE);

      J->Req[0]->TaskCount = VM->CRes.Procs;
      J->Req[0]->DRes.Procs = 1;

      if (J->Req[0]->TasksPerNode > 0)
        J->Req[0]->TasksPerNode = J->Req[0]->TaskCount;

      J->Request.TC = J->Req[0]->TaskCount;
      J->Req[0]->NodeList.Array[0].TC = J->Req[0]->TaskCount;

      for (nindex = 0;(J->NodeList.Array[nindex].N != NULL) && (nindex < J->NodeList.Size);nindex++)
        {
        if (J->NodeList.Array[nindex].N != VM->N)
          continue;

        J->NodeList.Array[nindex].TC = J->Req[0]->TaskCount;
        }

      for (nindex = 0;nindex < MMAX_SHAPE + 1;nindex++)
        {
        if (J->Req[0]->TaskRequestList[nindex] == 0)
          break;

        J->Req[0]->TaskRequestList[nindex] = J->Req[0]->TaskCount;
        }

      __MVMUpdateVMTrackingCL(J,maProc,J->Req[0]->TaskCount);

      *ResChange = TRUE;
      }  /* END if (J->Req[0]->TaskCount != VM->CRes.Procs) */

    *TaskCount = J->Req[0]->TaskCount;
    }  /* END else */

  return(SUCCESS);
  }  /* END __MVMUpdateVMTrackingProcs() */



/**
 * Helper function to update the GRes on a VMTracking job based on
 *  what the VM object currently looks like.
 *
 * No parameter checking.
 *
 * @param VM [I] - VM to update the job to conform to
 * @param J  [O] (modified) - the VM Tracking job for the given VM
 * @param ResChange [O] (modified) - set to TRUE if a GRes was modified
 */

int __MVMUpdateVMTrackingGRes(
  mvm_t *VM,
  mjob_t *J,
  mbool_t *ResChange)
  {
  int ReqIndex = -1;  /* index of req w/ GRes in it */
  int CountIndex = 0;    /* index to use in for loops */
  int GResIndex = 0;  /* The index of the GRes that we're looking at */ 

  /* We have updated GRes.  Steps for deciding where to place the updated info:
      For each GRes:
        1. Find a req that uses that GRes.
        2. If #1 fails, find the first req that has a GRes on it.
        3. If #2 doesn't exist, add a new req to hold GRes. */


  for (GResIndex = 1;GResIndex < MSched.M[mxoxGRes];GResIndex++)
    {
    if (MUStrIsEmpty(MGRes.Name[GResIndex]))
      break;

    /* Need to check every GRes, even if it is 0 - VM may have been changed
        from having GRes to 0 GRes. */

    ReqIndex = -1;

    /*** STEP 1: Search for a req that has the specified GRES */

    for (CountIndex = 0;(J->Req[CountIndex] != NULL) && (CountIndex < MMAX_REQ_PER_JOB);CountIndex++)
      {
      if (MSNLGetIndexCount(&J->Req[CountIndex]->DRes.GenericRes,GResIndex) > 0)
        {
        /* We found a req that has the specified GRes, break */

        ReqIndex = CountIndex;

        break;
        }
      }  /* END for (CountIndex) */

    if (ReqIndex >= 0)
      {
      if (MSNLGetIndexCount(&J->Req[ReqIndex]->DRes.GenericRes,GResIndex) !=
          MSNLGetIndexCount(&VM->CRes.GenericRes,GResIndex))
        {
        *ResChange = TRUE;

        MSNLSetCount(&J->Req[ReqIndex]->DRes.GenericRes,
                      GResIndex,
                      MSNLGetIndexCount(&VM->CRes.GenericRes,GResIndex));
        }

      continue;
      }
    else if (MSNLGetIndexCount(&VM->CRes.GenericRes,GResIndex) <= 0)
      {
      /* Didn't find a req for this GRes, and it is 0 - no need to check further */

      continue;
      }

    /* END STEP 1 */

#if 0
    /* NYI - adding a new GRES type to the job */

    /* MOVE THIS UP TO START OF FUNCTION */
    int FirstGResReq = -1;  /* The index of the first req with GRes */

    /* Req not found for specific GRes, do steps 2 and 3 */

    /*** STEP 2: Search for the first req that uses GRes */

    if (FirstGResReq == -1)
      {
      /* First time, search for the first GRes req */

      for (CountIndex = 0;(J->Req[CountIndex] != NULL) && (CountIndex < MMAX_REQ_PER_JOB);CountIndex++)
        {
        if (MSNLGetIndexCount(&J->Req[CountIndex]->DRes.GenericRes,0) > 0)
          {
          FirstGResReq = CountIndex;
          ReqIndex = FirstGResReq;

          break;
          }
        }  /* END for (CountIndex) */
      }  /* END if (FirstGResReq == -1) */
    else
      {
      ReqIndex = FirstGResReq;
      }

    /* END STEP 2 */

    /* TODO - See about using MJobAddRequiredGRes() here */

    if ((ReqIndex == -1) && (ReqIndex < MMAX_REQ_PER_JOB))
      {
      /*** STEP 3: Create a new req to hold GRes */

      mreq_t *TmpReq = NULL;

      if (MReqCreate(J,NULL,&TmpReq,FALSE) == FAILURE)
        {
        MDB(5,fSTRUCT) MLog("ERROR:    failed to create new req for GRes on VMTracking job '%s'\n",
          J->Name);

        return(FAILURE);
        }

      MRMReqPreLoad(TmpReq,J->SRM);

      TmpReq->DRes.Procs = 0;
      TmpReq->TaskCount          = 1;
      TmpReq->TaskRequestList[0] = 1;
      TmpReq->TaskRequestList[1] = 1;

      TmpReq->NAccessPolicy = mnacShared;

      /* New GRes reqs must use the GLOBAL node */

      if (MSched.GN == NULL)
        return(FAILURE);

      MNLSetNodeAtIndex(&TmpReq->NodeList,0,MSched.GN);
      MNLSetTCAtIndex(&TmpReq->NodeList,0,1);

      J->FNC++;

      ReqIndex = TmpReq->Index;
      FirstGResReq = ReqIndex;
      }

    /* Have a req, check resources */

    if (MSNLGetIndexCount(&J->Req[ReqIndex]->DRes.GenericRes,GResIndex) !=
        MSNLGetIndexCount(&VM->CRes.GenericRes,GResIndex))
      {
      *ResChange = TRUE;

      MSNLSetCount(&J->Req[ReqIndex]->DRes.GenericRes,
                    GResIndex,
                    MSNLGetIndexCount(&VM->CRes.GenericRes,GResIndex));
/*      MJobAddRequiredGRes(J,
                          MGRes.Name[GResIndex],
                          MSNLGetIndexCount(&VM->CRes.GenericRes,GResIndex),
                          mxaGRes,
                          FALSE,
                          TRUE);*/

      }
#endif
    }  /* END for (GResIndex) */

  return(SUCCESS);
  }  /* END __MVMUpdateVMTrackingGRes() */


/**
 * Updates the job based on the VM.
 *
 * If resources have changed (procs, mem, etc.), the job's reservation
 *  will be released and recreated.
 *
 * @param VM (I) - The VM whose VMTracking job is being updated.
 */

int MVMUpdateVMTracking(

  mvm_t *VM)

  {
  mbool_t ResourceChange = FALSE;
  mbool_t OtherChange = FALSE;
  mjob_t *J = NULL;
  int TaskCount = 1;

  if (VM == NULL)
    return(FAILURE);

  if (VM->TrackingJ == NULL)
    return(SUCCESS);

  /* Only modify running VMTracking job (otherwise, how was the VM updated?) */

  if (!MJOBISACTIVE(VM->TrackingJ))
    return(SUCCESS);

  J = VM->TrackingJ;

  if (J->Req[0]->Opsys != VM->ActiveOS)
    {
    J->Req[0]->Opsys = VM->ActiveOS;

    OtherChange = TRUE;
    }

  if ((J->Req[0]->NodeList.Array[0].N == NULL) ||
      (J->Req[0]->NodeList.Array[0].N != VM->N))
    {
    /* Shouldn't hit this (done by MVMSetParent when VM's node changes) */

    J->Req[0]->NodeList.Array[0].N = VM->N;

    ResourceChange = TRUE;
    }

  if (__MVMUpdateVMTrackingProcs(VM,J,&TaskCount,&ResourceChange) == FAILURE)
    return(FAILURE);

  /* Mem, etc. are always reported as the whole VM, not per task */

  if (TaskCount > 0)
    {

    /* Check the Memory valid as recorded vs as received: If different update */

    if (J->Req[0]->DRes.Mem != (VM->CRes.Mem / TaskCount))
      {
      J->Req[0]->DRes.Mem = VM->CRes.Mem / TaskCount;

      __MVMUpdateVMTrackingCL(J,maMemory,VM->CRes.Mem);

      ResourceChange = TRUE;
      }

    /* Check the Disk valid as recorded vs as received: If different update */
    if (J->Req[0]->DRes.Disk != (VM->CRes.Disk / TaskCount))
      {
      J->Req[0]->DRes.Disk = VM->CRes.Disk / TaskCount;

      ResourceChange = TRUE;
      }

    /* Check the Swap valid as recorded vs as received: If different update */
    if (J->Req[0]->DRes.Swap != (VM->CRes.Swap / TaskCount))
      {
      J->Req[0]->DRes.Swap = VM->CRes.Swap / TaskCount;

      ResourceChange = TRUE;
      }
    }

  if (MSNLGetIndexCount(&VM->CRes.GenericRes,0) > 0)
    {
    if (__MVMUpdateVMTrackingGRes(VM,J,&ResourceChange) == FAILURE)
      return(FAILURE);
    }  /* END if (MSNLGetIndexCount(&VM->CRes.GenericRes,0) > 0) */

  if (ResourceChange == TRUE)
    {
    /* Don't need to worry about nodelist - parent node is taken care of
        by MVMSetParent, called by MVMUpdate */

    MJobReleaseRsv(J,TRUE,FALSE);

    MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);
    }

  if ((ResourceChange == TRUE) ||
      (OtherChange == TRUE))
    {
    MJobTransition(J,FALSE,TRUE);
    }

  return(SUCCESS);
  }  /* END MVMUpdateVMTracking() */


/**
 *  Updates an out-of-band VM (no VM-tracking job) to have a reservation
 *   and syncs the reservation to appropriately represent the VM in
 *   scheduling.
 *
 * @param VM - [I] The VM whose reservation is being created/updated
 */

int MVMUpdateOutOfBandRsv(

  mvm_t *VM)

  {
  job_iter JTI;

  int TaskCount = 0;
  mjob_t *TmpJ = NULL;
  mnode_t *OldNode = NULL;

  if (!bmisset(&MSched.Flags,mschedfOutOfBandVMRsv))
    return(SUCCESS);

  if (VM == NULL)
    return(FAILURE);

  if (VM->TrackingJ != NULL)
    {
    /* VMTracking job found, remove rsv and exit */

    if (VM->OutOfBandRsv != NULL)
      MRsvDestroy(&VM->OutOfBandRsv,TRUE,TRUE);

    return(SUCCESS);
    }  /* END if (VM->TrackingJ != NULL) */

  /* See if there are any VMTracking jobs representing this VM in the queue -
      if a VMTracking job exists, and this VM has appeared, we can assume that
      there is a job in the queue representing this VM, even if the VMTracking
      job is not active (the setup job created by the VMTracking job must
      be active */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&TmpJ) == SUCCESS)
    {
    char *VMIDPtr = NULL;

    /* See if job is VMTracking job */

    if ((TmpJ->System == NULL) || (TmpJ->System->JobType != msjtVMTracking))
      continue;

    /* See if VMTracking job is for this VM */

    if ((MUHTGet(&TmpJ->Variables,"VMID",(void **)&VMIDPtr,NULL) == SUCCESS) &&
        (!strcmp(VMIDPtr,VM->VMID)))
      {
      if (VM->OutOfBandRsv != NULL)
        MRsvDestroy(&VM->OutOfBandRsv,TRUE,TRUE);

      return(SUCCESS);
      }
    }  /* END for (TmpJ) */

  if (VM->OutOfBandRsv == NULL)
    {
    /* Create rsv */

    mnl_t NodeList = {0};
    mbitmap_t Flags;
    char BaseName[MMAX_NAME];

    MNLInit(&NodeList);
    MNLSetNodeAtIndex(&NodeList,0,VM->N);
    MNLSetTCAtIndex(&NodeList,0,1);
    MNLTerminateAtIndex(&NodeList,1);

    snprintf(BaseName,sizeof(BaseName),"%s.placeholder.%d",
      VM->VMID,
      MSched.RsvCounter++);

    if ((MRsvCreate(
          mrtUser,
          NULL,
          NULL,
          &Flags,
          &NodeList,
          0,
          MMAX_TIME,
          0,
          0,
          BaseName,
          &VM->OutOfBandRsv,
          NULL,
          NULL,
          NULL,
          FALSE,
          TRUE) == FAILURE) ||
        (VM->OutOfBandRsv == NULL))
      {
      MDB(5,fSTRUCT) MLog("ERROR:    failed to create reservation for out-of-band VM %s\n",
        VM->VMID);

      MNLFree(&NodeList);
      return(FAILURE);
      }
    }  /* END if (VM->OutOfBandRsv == NULL) */

  /* Update the reservation */

  /* NOTE: Unlike jobs, a task here in a user reservation should be one
      task per node, with the task being all of the resources used on
      that node.  This is because of MJobGetSNRange, where user
      reservations are assumed to work in this way */

  TaskCount = VM->CRes.Procs;

  VM->OutOfBandRsv->ReqTC = 1;
  VM->OutOfBandRsv->DRes.Procs = TaskCount;
  VM->OutOfBandRsv->DRes.Mem = VM->CRes.Mem;
  VM->OutOfBandRsv->DRes.Disk = VM->CRes.Disk;
  VM->OutOfBandRsv->DRes.Swap = VM->CRes.Swap;

  VM->OutOfBandRsv->AllocPC = TaskCount;
  VM->OutOfBandRsv->AllocTC = 1;
  VM->OutOfBandRsv->AllocNC = 1;

  /* This is to clear old resources */
  OldNode = MNLReturnNodeAtIndex(&VM->OutOfBandRsv->NL,0);
  MRsvAdjustNode(VM->OutOfBandRsv,OldNode,1,-1,FALSE);

  MNLSetNodeAtIndex(&VM->OutOfBandRsv->NL,0,VM->N);
  MNLSetTCAtIndex(&VM->OutOfBandRsv->NL,0,1);
  MNLTerminateAtIndex(&VM->OutOfBandRsv->NL,1);

  MNLSetNodeAtIndex(&VM->OutOfBandRsv->ReqNL,0,VM->N);
  MNLSetTCAtIndex(&VM->OutOfBandRsv->ReqNL,0,1);
  MNLTerminateAtIndex(&VM->OutOfBandRsv->ReqNL,1);

  /* Out-of-band VMs cannot have GRES associated with them */

  MRsvAdjustNode(VM->OutOfBandRsv,VM->N,1,0,FALSE);

  /* Out-of-band reservations should not be checkpointed */

  VM->OutOfBandRsv->DisableCheckpoint = TRUE;

  return(SUCCESS);
  }  /* END MVMUpdateOutOfBandRsv() */


/**
 * Returns TRUE if job is part of a workflow that contains a
 *  VMTracking job.  This essentially means that the job is
 *  a VM setup job, a VMTracking job, VM destroy job, or a
 *  migration job in the new workflow model.
 *
 * @param J - [I] The job we are evaluating
 */

mbool_t MJobIsPartOfVMTrackingWorkflow(

  mjob_t *J)
  {
  mvc_t *VC = NULL;
  mln_t *tmpLVCs = NULL;
  mln_t *tmpLJobs = NULL;
  mjob_t *tmpJ = NULL;

  if (J == NULL)
    return(FALSE);

  if ((J->System != NULL) &&
      (J->System->JobType == msjtVMTracking))
    { 
    /* Job itself is a VMTracking job */

    return(TRUE);
    }

  for (tmpLVCs = J->ParentVCs;tmpLVCs != NULL;tmpLVCs = tmpLVCs->Next)
    {
    VC = (mvc_t *)tmpLVCs->Ptr;

    if (VC == NULL)
      continue;

    if (!bmisset(&VC->Flags,mvcfWorkflow))
      continue;

    /* We have a workflow VC */

    for (tmpLJobs = VC->Jobs;tmpLJobs != NULL;tmpLJobs = tmpLJobs->Next)
      {
      tmpJ = (mjob_t *)tmpLJobs->Ptr;

      if (tmpJ == NULL)
        continue;

      if ((J->System != NULL) &&
      (J->System->JobType == msjtVMTracking))
        {
        /* Found a VMTracking job in the workflow */

        return(TRUE);
        }
      }  /* END for (tmpLJobs = VC->Jobs) */
    }  /* END for (tmpLVCs = J->ParentVCs) */

  /* No workflows or VMTracking jobs found */

  return(FALSE);
  }  /* END MJobIsPartOfVMTrackingWorkflow() */
/* END MVMTracking.c */
