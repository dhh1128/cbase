/* HEADER */

/**
 * @file MSysJob.c
 *
 * Contains system-job related functions.
 */

/* Contains:                                 *
 *                                           */



#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "MEventLog.h"

int __MGenericJobValidateTrigger(mjob_t *);


/**
 * Alloc and initialize system job-specific data structure.
 *
 * @param J (I) [modified,alloc]
 */

int MJobCreateSystemData(

  mjob_t *J)

  {
  const char *FName = "MJobCreateSystemData";

  MDB(5,fCONFIG) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->System != NULL)
    {
    /* if System already exists, clear it */

    memset(J->System,0,sizeof(J->System[0]));
    }
  else
    {
    J->System = (msysjob_t *)MUCalloc(1,sizeof(msysjob_t));

    if (J->System == NULL)
      {
      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MJobCreateSystemData() */




/**
 * Helper function to validate that a generis system job's trigger is good to run.
 *
 * @param J [I] - The generic system job
 */

int __MGenericJobValidateTrigger(

  mjob_t *J)

  {
  mtrig_t *T = NULL;
  char *Args = NULL;
  char ExecFull[MMAX_LINE]= {0};

  int Exists = -1;
  mbool_t IsExe = FALSE;
  mbool_t IsDir = FALSE;

  if (MSched.DisableTriggers == TRUE)
    {
    MMBAdd(&J->MessageBuffer,"Triggers disabled by admin",NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }

  if (MSysJobGetGenericTrigger(J,&T) == FAILURE)
    {
    MMBAdd(&J->MessageBuffer,"Job trigger not specified",NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }

  /* Substitute $HOME if exists */
  SetupTrigVarList(T,ExecFull,sizeof(ExecFull));

  Exists = MFUGetInfo(ExecFull,NULL,NULL,&IsExe,&IsDir);

  mstring_t ErrorMsg(MMAX_LINE);

  MUStrTok(ExecFull," ",&Args);

  if (Exists == FAILURE)
    {
    MStringSetF(&ErrorMsg,"Job trigger file (%s) not found",ExecFull);
    MMBAdd(&J->MessageBuffer,ErrorMsg.c_str(),NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }
  else if (IsExe == FALSE)
    {
    MStringSetF(&ErrorMsg,"Job trigger file (%s) is not executable",ExecFull);
    MMBAdd(&J->MessageBuffer,ErrorMsg.c_str(),NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }
  else if (IsDir == TRUE)
    {
    MStringSetF(&ErrorMsg,"Job trigger (%s) is a directory",ExecFull);
    MMBAdd(&J->MessageBuffer,ErrorMsg.c_str(),NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }  /* END else */

  return(SUCCESS);
  }  /* END __MGenericJobValidateTrigger() */




/**
 * Start system job.
 *
 * @see MJobStart() - parent
 * @see MTrigLaunchInternalAction() - child
 * @see MNodeProvision() - handles provisioning system job
 * @see MSysJobUpdate() - peer - determine when system job is complete
 *
 * @param J (I) [modified]
 * @param Response (O) [optional,minsize=MMAX_LINE]
 * @param SEMsg (O) [optional,minsize=MMAX_LINE]
 */

#define MSYSJOBVARSIZE 24

int MSysJobStart(

  mjob_t    *J,         /* I (modified) */
  char      *Response,  /* O (optional,minsize=MMAX_LINE) */
  char      *SEMsg)     /* O (optional,minsize=MMAX_LINE) */

  {
  mln_t *VarList = NULL;

  char ActionString[MMAX_BUFFER << 1];
  char tmpEMsg[MMAX_LINE];

  char *EMsg;
  int rc = FAILURE;

  mbool_t   IsComplete;

  const char *FName = "MSysJobStart";

  MDB(5,fCONFIG) MLog("%s(%s,Response,SEMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (Response != NULL)
    Response[0] = '\0';

  EMsg = (SEMsg != NULL) ? SEMsg : tmpEMsg;

  EMsg[0] = '\0';

  if ((J == NULL) || (J->System == NULL))
    {
    return(FAILURE);
    }

  MDB(5,fCONFIG) MLog("INFO:     attempting to start system job %s of type %s (Action: '%s')\n",
    J->Name,
    MSysJobType[J->System->JobType],
    (J->System->Action != NULL) ? J->System->Action : "");

  switch (J->System->JobType)
    {
    case msjtVMMap:
   
      /* no action to take */

      return(SUCCESS);

      /*NOTREACHED*/

      break;

#if 0
    case msjtVMCreate:

      {
      mvm_t *V;

      /* NOTE:  V should be created when system job is created in 
                MUGenerateSystemJobs() */

      if (MVMFind(J->System->SCPVar1,&V) == SUCCESS)
        {
        bmset(&V->Flags,mvmfInitializing);
        }
      }

      break;
    case msjtVMDestroy:
    case msjtVMStorage:
#endif
    case msjtStorage:
    case msjtGeneric:  /* Trigger is the action */

      break;

    default:

      if (J->System->Action == NULL)
        {
        MDB(5,fCONFIG) MLog("WARNING:  start of system job %s failed - no action specified\n",
          J->Name);

        return(FAILURE);
        }

      break;
    }

  /* need to replace variables found in the string */

  mstring_t NodeList(MMAX_LINE);

  if (!MNLIsEmpty(&J->NodeList))
    {
    /* Remove the Global Node so it won't be included in NodeList. */

    MNLRemove(&J->NodeList,MSched.GN);

    MNLToMString(&J->NodeList,MBNOTSET,",",'\0',-1,&NodeList);

    /* always specify HOSTLIST - why? */
    MUAddVarLL(&VarList,(char *)MJVarName[mjvnHostList],NodeList.c_str());
    }   /* END if (J->NodeList != NULL) */

  switch (J->System->JobType)
    {
    case msjtGeneric:

      {
      /* Check to see that the trigger is valid (script exists, is executable, etc.) */

      rc = __MGenericJobValidateTrigger(J);

      if (rc == FAILURE)
        {
        /* In this case, Moab is the RM - RM reject hold reason */

        MJobSetHold(J,mhSystem,MSched.DeferTime,mhrRMReject,NULL);

        MULLFree(&VarList,MUFREE);

        return(FAILURE);
        }
      }  /* END case msjtGeneric */

      break;

#if 0
    case msjtVMStorage:

      rc = SUCCESS;

      break;

    case msjtVMDestroy:

      {
      mvm_t *VM;

      if (MVMFind(J->System->VM,&VM) == SUCCESS)
        {
        rc = MVMRemove(VM,J->Name,EMsg);
        }
      else
        {
        /* cannot locate VM */

        rc = SUCCESS;
        }
      }

      break;
#endif

    case msjtStorage:

      {
      mtrig_t *T;

      if ((MSysJobGetGenericTrigger(J,&T) == SUCCESS) &&
          (MTrigInitiateAction(T) == FAILURE))
        {
        /* what now? */
        MULLFree(&VarList,MUFREE);

        return(FAILURE);
        }
      }

      break;

    default:

      {
      mtrig_t tmpTrig;

      memset(&tmpTrig,0x0,sizeof(mtrig_t));

      MUInsertVarList(
        J->System->Action,
        VarList,
        NULL,
        NULL,
        ActionString,              /* O */
        sizeof(ActionString),
        FALSE);

      tmpTrig.O = (void *)J;
      tmpTrig.OType = mxoJob;

      rc = MTrigLaunchInternalAction(&tmpTrig,ActionString,&IsComplete,EMsg);
      }

      break;
    }

  /* must free after VarVal is used, because we only made a shallow copy! */

  MULLFree(&VarList,MUFREE);

  if (rc == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot launch internal action '%s' for system job '%s' (%s)\n",
      J->System->Action,
      J->Name,
      EMsg);

    MMBAdd(&J->MessageBuffer,EMsg,NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }

  if (Response != NULL)
    MUStrCpy(Response,EMsg,MMAX_LINE);

  switch (J->System->JobType)
    {
#if 0
    case msjtVMStorage:

      /* to address the problem of vmmigrations away from vmstorage jobs set the novmmigrations
         flag on the reservation */

      MRsvSetAttr(J->Rsv,mraFlags,(void *)"novmmigrations",mdfString,mAdd);

      break;
#endif

    case msjtOSProvision:

      /* NYI */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (J->System->JobType) */

  MDB(5,fCONFIG) MLog("INFO:     system job %s started - action launched\n",
    (J != NULL) ? J->Name : "NULL");

  return(SUCCESS);
  }  /* END MSysJobStart() */





/**
 * Handle system job state changes from active state to next state.
 *
 * @see MS3WorkloadQuery() - parent
 * @see MSysJobStart() - starts system jobs
 * @see MSysScheduleDynamic() - peer
 *
 * @param J     (I) [modified]
 * @param State (I) [modified] must be pre-initialized
 */

int MSysJobUpdate(

  mjob_t             *J,
  enum MJobStateEnum *State)

  {
  const char *FName = "MSysJobUpdate";

  MDB(5,fCONFIG) MLog("%s(%s,State)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if ((J == NULL) || (J->System == NULL))
    {
    return(FAILURE);
    }

  if ((J->State != mjsUnknown) &&
      ((MJOBISACTIVE(J) == FALSE) ||
      (J->StartTime <= 0)))
    {
    *State = J->State;

    return(SUCCESS);
    }

#if 0
  MSysJobUpdateRsvFlags(J);
#endif

  switch (J->System->CompletionPolicy)
    {
    case mjcpOSProvision:
    case mjcpOSProvision2:

      {
      mnode_t *N = MNLReturnNodeAtIndex(&J->NodeList,0);
      char Response[MMAX_LINE];
      mvm_req_create_t VMCR;
      int OSIndex = 0;

      mbool_t JobWantsVMOS = MJOBISVMCREATEJOB(J) || MJOBREQUIRESVMS(J);

      if (N == NULL)
        {
        /* should never happen */

        return(FAILURE);
        }

      /* add node substate support */

      /* NOTE: J->Alloc must be populated here for system jobs. If Alloc is not
               populated job's NodeList will be removed in MRMJobPostUpdate() */

      J->Alloc.TC = J->Request.TC;
      J->Alloc.NC = J->Request.NC;

      /* Use the string name of the OS to find the index.  In restarts,
        the order of OSes in MAList may change, so storing the index
        on the job is unreliable. */

      OSIndex = MUMAGetIndex(meOpsys,J->System->SCPVar1,mVerify);

      /* NOTE:  assumes system provisioning job is serial - why? */

      if (!MUNLNeedsOSProvision(OSIndex,JobWantsVMOS,&J->NodeList,NULL))
        {
        *State = mjsCompleted;

        N->LastOSModIteration = MSched.Iteration;

        MDB(4,fSCHED) MLog("INFO:     dynamic provisioning job %s completed - OS for node %s set to %s\n",
          (J != NULL) ? J->Name : "NULL",
          N->Name,
          MAList[meOpsys][N->ActiveOS]);

        if (MNODEISUP(N) && N->PowerState != mpowOn)
          {
          /* node is now available. Remove from green pool to allow immediate use
           * by jobs this scheduling iteration */

          N->PowerState = mpowOn;
          N->PowerSelectState = mpowOn;
          }
        }    /* END if (!MUNLNeedsOSProvision()) */
      else
        {
        /* check if system job has run too long */

        if ((long)(MSched.Time - J->StartTime) > J->System->WCLimit)
          {
          /* job has run too long */

          *State = mjsRemoved;

          /* J may have been deleted */

          break;
          }
        }

      if (J->System->CompletionPolicy == mjcpOSProvision)
        {
        /* break here on single stage provisioning */

        break;
        }

      *State = mjsRunning;

      /* update system job to now provision virtual machines */

      MJobFreeSystemStruct(J);
      MJobCreateSystemData(J);

      MVMCreateReqInit(&VMCR);

      VMCR.OSIndex = MUMAGetIndex(meOpsys,J->System->SCPVar1,mAdd);

#if 0
      if (MJobPopulateSystemJobData(J,msjtVMCreate,&VMCR) == FAILURE)
        {
        return(FAILURE);
        }
#endif

      if (MSysJobStart(J,Response,NULL) == FAILURE)
        {
        /* first stage of provisioning completed but second stage failed */
        /* what should we do now? */

        return(FAILURE);
        }
      }    /* END BLOCK */

#if 0
    /* NOTE: performing 2 stage provisioning, fall through to second stage */

    /* fall through to second stage of 2 stage provisioning */

    case mjcpVMCreate:

      {
      int    rc;
      mvm_t *V;
      enum MNodeStateEnum VMState = mnsNONE;

      mbool_t JobCompleted = FALSE;

      /*  is it possible for the vm to have been created and
       *  also migrated to another hypervisor by now? If not,
       *  we can simply check if the VM is on the hypervisor
       *  where the VM was requested to be created, which
       *  is J->NodeList[0].N.
      mnode_t *N = J->NodeList[0].N;
      */

      /* NOTE:  system job should persist until compute RM
                reports VM in up state */

      rc = MVMFind(J->System->SCPVar1,&V);

      if ((V != NULL) && (V->SubState != NULL))
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"VM substate '%s' detected",
          V->SubState);

        MMBAdd(
          &J->MessageBuffer,
          tmpLine,
          (char *)"N/A",
          mmbtOther,
          0,
          1,
          NULL);
        }

      if (V != NULL)
        {
        if ((V->RM != NULL) && (V->RM->VMOwnerRM != NULL))
          {
          VMState = V->RMState[V->RM->VMOwnerRM->Index];
          }
        else
          {
          VMState = V->State;
          }
        }

      if ((rc == SUCCESS) &&
          (V != NULL) &&
          (V->N != NULL) &&
          bmisset(&V->Flags,mvmfCreationCompleted) &&
          ((MSched.VMProvisionStatusReady != NULL) ||  /* if we are looking for provision status, ignore state */
           (MNODESTATEISUP(VMState))))

        {
        assert(V->RM != NULL);

        if (MSched.VMProvisionStatusReady != NULL)
          {
          char *tmpVarVal = NULL;

          if ((MUHTGet(&V->Variables,"provision_status",(void **)&tmpVarVal,NULL) == SUCCESS) &&
              (tmpVarVal != NULL) &&
              (MUIsIntValueInList(atoi(tmpVarVal),MSched.VMProvisionStatusReady,",") == SUCCESS))
            {
            JobCompleted = TRUE;
            }
          }
        else if ((V->SubState == NULL) || (strstr(V->SubState,"sshd") != NULL))
          {
          /* NOTE:  dependency on xCAT - xCAT will set V->SubState to { sshd,pbs } */
          /* if not using provision_status, only remove vm create job if 'sshd' substate has been reached */

          JobCompleted = TRUE;
          }

        if (JobCompleted == TRUE)
          {
          char *username;
          MURemoveAction(&V->Action,J->Name);

          username = J->System->ProxyUser ? J->System->ProxyUser : J->Credential.U->Name; 
          MOWriteEvent(V,mxoxVM,mrelVMCreate,J->System->RecordEventInfo,NULL,username);

          *State = mjsCompleted;

          bmunset(&V->Flags,mvmfInitializing);

          if (V->StartTime <= 0)
            V->StartTime = MSched.Time;

          /* Launch any VM start triggers */
          if (V->T != NULL)
            {
            MOReportEvent((void *)V,V->VMID,mxoxVM,mttStart,MSched.Time,TRUE);
            }
          }
        }

      if (JobCompleted == FALSE)
        {
        if (bmisset(&J->IFlags,mjifSystemJobFailure))
          {
          *State = mjsRemoved;
          }
        else if ((long)(MSched.Time - J->StartTime) > J->System->WCLimit)
          {
          /* job has run too long */

          *State = mjsRemoved;
          }
        }

      if (*State == mjsCompleted)
        {
        bmset(&V->IFlags,mvmifReadyForUse);

        if (V->TrackingJ != NULL)
          {
          /* Remove any requested host from the VMTracking job so that 
              the VM can be migrated later if needed. */

          bmunset(&V->TrackingJ->IFlags,mjifHostList);

          if (!MNLIsEmpty(&V->TrackingJ->ReqHList))
            MNLClear(&V->TrackingJ->ReqHList);

          V->TrackingJ->ReqHLMode = mhlmNONE;

          MJobUpdateFlags(V->TrackingJ);
          }
        }    /* END if (*State == mjsCompleted) */

      if (*State == mjsRemoved)
        {
        mjob_t *trackingJ = NULL;

        /* JOB has been removed due to some failure.  Find tracking job. */

        if ((V != NULL) && (V->TrackingJ != NULL))
          {
          /* In most cases, the VM will have the tracking job. */

          trackingJ = V->TrackingJ;
          }
        else
          {
          /* In the case of a system recycle, the mvm_t may not be allocated.  Look up tracking job. */

          if (J->System->VMJobToStart)
            MJobFind(J->System->VMJobToStart,&trackingJ,mjsmBasic);
          }

        /* Verify we actually have a tracking job and set a batch hold state */

        if ((trackingJ != NULL) && (trackingJ->System != NULL) && (trackingJ->System->JobType == msjtVMTracking))
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"vmcreate (%s) job failed. Tracking job %s placed on BatchHold.",J->Name,trackingJ->Name);

          MDB(3,fSCHED) MLog("INFO:     %s\n",tmpLine);

          MJobSetHold(trackingJ,mhBatch,MMAX_TIME,mhrAdmin,tmpLine);
          }

        if (V != NULL)
          V->CreateJob = NULL;
        }
      }    /* END BLOCK (case mjcpVMCreate) */

      break;

    case mjcpVMDestroy:

      {
      mbool_t VMIsGone = FALSE;
      mbool_t AllTrigsSuccess = TRUE;
      int tindex;
      mvm_t *VM;
      mjob_t *trackingJ = NULL;

      if (MVMFind(J->System->VM,&VM) == FAILURE)
        {
        VMIsGone = TRUE;
        }
      else 
        {
        /* Restore the VM->TrackingJ.  Sometimes, we can loose it on a system restart */

        trackingJ = VM->TrackingJ;

        MUStrDup(&J->CompletedVMList,VM->VMID);

        MVMDestroy(&VM,NULL);

        VMIsGone = TRUE;
        }

      /* The VM->TrackingJ can be lost on system restarts.  Attempt to restore it. */

      if ((trackingJ == NULL) && (J->System->VMJobToStart != NULL))
        MJobFind(J->System->VMJobToStart,&trackingJ,mjsmBasic);

      if (trackingJ != NULL)
        {
        /* In the case the vmtracking job has is in 
         * batchold VM->J won't be populated and will be stranded unless it is
         * removed here. */

        MJobSetState(trackingJ,mjsCompleted);
        MJobTransition(trackingJ,FALSE,FALSE);

        /* add vmtracking job to completed jobs */
        MJobProcessCompleted(&trackingJ);
        }

      if (J->Triggers != NULL)
        {
        mtrig_t *T;

        for (tindex = 0; tindex < J->Triggers->NumItems;tindex++)
          {
          T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,tindex);

          if (MTrigIsReal(T) == FALSE)
            continue;

          if (T->State != mtsSuccessful)
            {
            AllTrigsSuccess = FALSE;
            }
          }
        }

      if ((VMIsGone == TRUE) && (AllTrigsSuccess == TRUE))
        {
        /* Create an empty VM to log--follows the pattern the event logs current state */

        mvm_t tmpVM;
        char *username;

        memset(&tmpVM,0,sizeof(tmpVM));

        *State = mjsCompleted;

        MUStrCpy(tmpVM.VMID,J->System->VM,sizeof(tmpVM.VMID));
        username = J->System->ProxyUser ? J->System->ProxyUser : J->Credential.U->Name; 
        MOWriteEvent(&tmpVM,mxoxVM,mrelVMDestroy,J->System->RecordEventInfo,NULL,username);
        }
      else if ((long)(MSched.Time - J->StartTime) > J->System->WCLimit)
        {
        /* job has run too long */

        if (VM != NULL)
          {
          /* VM still exists, need to clear pending deletion flags 
             so it can be deleted later */

          bmunset(&VM->Flags,mvmfDestroyed);
          bmunset(&VM->Flags,mvmfDestroyPending);
          bmunset(&VM->Flags,mvmfDestroySubmitted);
          }

        *State = mjsRemoved;
        }
      }    /* END BLOCK (case mjcpVMDestroy) */

      break;
#endif

    case mjcpVMMap:

      {
      /* system job only completes if parent VM goes away */

      if (MVMFind(J->System->SCPVar1,NULL) == FAILURE)
        *State = mjsCompleted;
      }

      break;

    case mjcpVMMigrate:

      {
      mnode_t *DestNode;
      mln_t *VMLink;

      if ((MNodeFind(J->System->VMDestinationNode,&DestNode) == FAILURE))
        {
        *State = mjsCompleted;
        J->CompletionCode = -1;

        MDB(1,fSCHED) MLog("INFO:     migration of VM %s to node %s failed - node no longer exists\n",
          J->System->VM,
          J->System->VMDestinationNode);
        }
      else if (MULLCheck(DestNode->VMList,J->System->VM,&VMLink) == SUCCESS)
        {
        mvm_t   *V = (mvm_t *)VMLink->Ptr;
        mnode_t *SrcNode;
        char    *username;

        if (J->MessageBuffer != NULL)
          {
          char tmpMsg[MMAX_LINE];

          if (MMBPrintMessages(J->MessageBuffer,mfmHuman,FALSE,-1,tmpMsg,sizeof(tmpMsg)) != NULL)
            {
            MUStrDup(&V->LastActionDescription,tmpMsg);

            V->LastActionType      = J->System->JobType;
            V->LastActionStartTime = J->StartTime;
            V->LastActionEndTime   = MSched.Time;
            }
          }

        if ((MNodeFind(J->System->VMSourceNode,&SrcNode) == SUCCESS))
          {
          mjob_t *RealJ;

          SrcNode->VMOpTime = MSched.Time;

          MULLRemove(&SrcNode->VMList,J->System->VM,NULL);

          if (MJobFind(V->JobID,&RealJ,mjsmBasic) == SUCCESS)
            {
            MJobMigrateNode(RealJ,V,SrcNode,DestNode);
            }
          else
            {
            MDB(1,fSTRUCT) MLog("ALERT:    migration could not be finished for job %s, because job is no longer present!\n",
              V->JobID);

            /* assert(FALSE); */
            }
          }

        MDB(3,fSTRUCT) MLog("INFO:     migration job %s completed\n",
          J->Name);

        username = J->System->ProxyUser ? J->System->ProxyUser : J->Credential.U->Name; 
        MOWriteEvent(V,mxoxVM,mrelVMMigrate,J->System->RecordEventInfo,NULL,username);

        V->MigrationCount++;
        V->LastMigrationTime = MSched.Time;

        *State = mjsCompleted;

        /* Remove required host list from parent system job(s) */

        if (V->TrackingJ != NULL)
          {
          bmunset(&V->TrackingJ->IFlags,mjifHostList);

          if (!MNLIsEmpty(&V->TrackingJ->ReqHList))
            MNLClear(&V->TrackingJ->ReqHList);

          V->TrackingJ->ReqHLMode = mhlmNONE;

          MJobUpdateFlags(V->TrackingJ);
          }

        if ((V->J != NULL) && (V->J != V->TrackingJ))
          {
          bmunset(&V->J->IFlags,mjifHostList);

          if (!MNLIsEmpty(&V->J->ReqHList))
            MNLClear(&V->J->ReqHList);

          V->J->ReqHLMode = mhlmNONE;

          MJobUpdateFlags(V->J);
          }
        }
      else if ((long)(MSched.Time - J->StartTime) > J->System->WCLimit)
        {
        /* job has run too long */

        *State = mjsRemoved;
        }
      }    /* END BLOCK (case mjcpVMMigrate) */

      break;

#if 0
    case mjcpVMStorage:

      {
      if (J->Triggers != NULL)
        {
        mtrig_t *T;

        int tindex;

        mbool_t AllTrigsSuccess = TRUE;
        mbool_t FailureFound = FALSE;

        mvm_t *tmpVM = NULL;

        if (J->Variables.Table != NULL)
          {
          char *VMID;

          if ((MUHTGet(&J->Variables,"VMID",(void **)&VMID,NULL) == FAILURE) ||
              (MVMFind(VMID,&tmpVM) == FAILURE))
            {
            /* Couldn't find the VM */

            *State = mjsRemoved;

            break;
            }
          }
        else
          {
          *State = mjsRemoved;

          break;
          }

        for (tindex = 0; tindex < J->Triggers->NumItems;tindex++)
          {
          T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,tindex);

          if (MTrigIsReal(T) == FALSE)
            continue;

          if (T->State != mtsSuccessful)
            {
            AllTrigsSuccess = FALSE;
            }

          if ((T->State == mtsFailure) &&
              (T->RetryCount >= T->MaxRetryCount))
            {
            FailureFound = TRUE;

            break;
            }
          }    /* END for (tindex = 0;...) */

        if (FailureFound == TRUE)
          {
          /* One of the storage triggers failed, fail job */

          mjob_t *DestroyJ;
          marray_t JArray;
          mnl_t tmpNL;

          MDB(3,fSTRUCT) MLog("ERROR:    child trigger failed for system job '%s'\n",
                J->Name);

          MVMHarvestStorageVars(tmpVM,J,State);

          MNLInit(&tmpNL);

          MNLSetNodeAtIndex(&tmpNL,0,MNLReturnNodeAtIndex(&J->NodeList,0));
          MNLSetTCAtIndex(&tmpNL,0,MNLGetTCAtIndex(&J->NodeList,0));
          MNLTerminateAtIndex(&tmpNL,1);

          *State = mjsRemoved;

          /* Need to submit a vmdestroy job to destroy any successful storage
              and remove the vmcreate job */

          MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

          if (MUGenerateSystemJobs(
              NULL,
              NULL,
              &tmpNL,
              msjtVMDestroy,
              "vmdestroy",
              0,
              tmpVM->VMID,
              tmpVM,
              FALSE,
              FALSE,
              NULL,
              &JArray) == FAILURE)
            {
            MDB(3,fSTRUCT) MLog("ERROR:    failed to create vmdestroy job for VM '%s' after VMStorage job failure",
              tmpVM->VMID);
            }
 
          /* Get the destroy job and set some flags */

          DestroyJ = *(mjob_t **)MUArrayListGet(&JArray,0);
          if (DestroyJ != NULL)
            {
            MDB(3,fSTRUCT) MLog("INFO:     created vmdestroy job '%s'\n",
               DestroyJ->Name);
            }
 
          MUArrayListFree(&JArray);

          if (tmpVM->CreateJob != NULL)
            {
            /* We know that the create job has not started because it
                is dependent on this job */

            MRMJobCancel(tmpVM->CreateJob,NULL,FALSE,NULL,NULL);
            }

          /* place a hold on the vmtracking job, if one exists, to prevent 
           * further vmstorage and vmcreate jobs from being generated */

          if (tmpVM->TrackingJ != NULL)
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"vmstorage job '%s' failed",J->Name);

            MJobSetHold(tmpVM->TrackingJ,mhBatch,MMAX_TIME,mhrPreReq,tmpLine);

            MVMReleaseTempStorageRsvs(tmpVM->TrackingJ);
            }
 
          MNLFree(&tmpNL);

          break;
          } /* END if (FailureFound == TRUE) */
        else if (AllTrigsSuccess == TRUE)
          {
          /* Harvest the job variables into the storage mounts */

          if (MVMHarvestStorageVars(tmpVM,J,State) == FAILURE)
            break;
          }
        }  /* END if (J->T != NULL) */
      else
        {
        *State = mjsCompleted;
        }
      }  /* END BLOCK (mjcpVMStorage) */

      break;
#endif

    case mjcpVMTracking:

      {
      mvm_t *V = NULL;

      if ((J->VMCreateRequest == NULL) || (J->VMCreateRequest->VMID[0] == '\0'))
        {
        *State = mjsCompleted;

        break;
        }

      if (!MVMFind(J->VMCreateRequest->VMID,&V))
        {
        *State = mjsCompleted;
        }
      else if (bmisset(&J->IFlags,mjifSystemJobFailure))
        {
        MDB(1,fSTRUCT) MLog("ERROR:    VM '%s' reported a system job failure on VMTracking job '%s'\n",
          V->VMID,
          J->Name);

        break;
        }
      else if (((long)(MSched.Time - J->StartTime) > J->System->WCLimit) &&
                (J->System->WCLimit != MMAX_TIME))
        {
        MDB(1,fSTRUCT) MLog("ERROR:    VM '%s' exceeded it's allocated walltime.  VMTracking job '%s' (pointing to job '%s')\n",
            V->VMID,
            J->Name,
            V->TrackingJ->Name);

        /* Too risky to always automatically destroy VMs.  Admin must set EnableVMDestroy to allow */

        if (MSched.EnableVMDestroy == TRUE)
          {
          /* Need to remove the associated VM */

          if (!bmisset(&V->Flags,mvmfDestroySubmitted) &&
              !bmisset(&V->Flags,mvmfDestroyPending) &&
              !bmisset(&V->Flags,mvmfDestroyed))
            {
            mbool_t DestroyVM = TRUE;

            if (MVMCanRemove(V,NULL))
              {
              if (MSched.PushEventsToWebService == TRUE)
                {
                MEventLog *Log = new MEventLog(meltVMEnd);
                Log->SetCategory(melcEnd);
                Log->SetFacility(melfVM);
                Log->SetPrimaryObject(V->VMID,mxoxVM,(void *)V);
                Log->AddDetail("msg","VM exceeded it's allocated walltime");

                MEventLogExportToWebServices(Log);

                delete Log;
                }

              DestroyVM = MJobCancel(J,"VM exceeded it's allocated walltime",FALSE,NULL,NULL);
              }
            else
              {
              DestroyVM = FALSE;
              }

            if (DestroyVM == TRUE)
              {
              /* Don't remove job, will be removed when VM is no longer reported */

              bmset(&V->Flags,mvmfDestroySubmitted);
              }
            } /* END if (!bmisset(&V->Flags,mvmfDestroySubmitted)) */
          }  /* END if (MSched.EnableVMDestroy == TRUE) */

        break;
        } /* END else if (((long)(MSched.Time - J->StartTime) > J->System->WCLimit) && ...) */
      else
        {
        /* VM is fine, just make sure that it is still pointing to this job */

        if (V->TrackingJ == NULL)
          {
          V->TrackingJ = J;
          }
        else if (V->TrackingJ != J)
          {
          /* ERROR - this should not happen */

          MDB(1,fSTRUCT) MLog("ERROR:    VM '%s' not pointing to VMTracking job '%s' (pointing to job '%s')\n",
            V->VMID,
            J->Name,
            V->TrackingJ->Name);

          MDB(1,fSTRUCT) MLog("INFO:     setting VMTracking job for VM '%s' to job '%s'\n",
            V->VMID,
            J->Name);

          V->TrackingJ = J;
          }
        }  /* END else */

        break;
      } /* END BLOCK (mjcpVMTracking) */

      break;

    case mjcpGeneric:

      {
      mtrig_t *T = NULL;

      if (MSysJobGetGenericTrigger(J,&T) == SUCCESS)
        {
        if (T->State == mtsFailure)
          {
          /* Trigger failed - pass up debug info to parent VCs */

          *State = mjsRemoved;

          MSysJobPassDebugTrigInfo(J,T);

          MDB(8,fSTRUCT) MLog("INFO:     generic system job %s's trigger finished (CCODE: %d STATE: %s\n",
            J->Name,
            T->ExitCode,
            MTrigState[T->State]);
          }
        else if (T->State == mtsSuccessful)
          {
          *State = mjsCompleted;

          MDB(8,fSTRUCT) MLog("INFO:     generic system job %s's trigger finished (CCODE: %d STATE: %s\n",
            J->Name,
            T->ExitCode,
            MTrigState[T->State]);
          }
        }
      else
        {
        /* NOTREACHED - generic job should have one trigger */

        *State = mjsRemoved;
        }
      }   /* END case mjcpGeneric */

      break;

    case mjcpPowerOff:

      {
      int nindex;
      int rindex;

      mnode_t *N;

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        if (N == MSched.GN)
          continue;

        rindex = 0;

        if ((N->RM != NULL) && !bmisset(&N->RM->RTypes,mrmrtProvisioning))
          {
          rindex = N->RM->Index;
          }
        else if (MPowerGetPowerStateRM(&rindex) == FAILURE)
          {
          /* system job would otherwise not finish because the only RM
           * is a provisioning, MSM RM - MSysJobProcessFailure will
           * report an error when the job times out. */

          /* flag the job so checkjob output can alert the user to the
           * problem */

          bmset(&J->System->EFlags,msjefOnlyProvRM);

          break;
          }
        else
          {
          bmunset(&J->System->EFlags,msjefOnlyProvRM);
          }

        if (N->PowerState == mpowOff)
          {
          /* Go off of the power state, not the node state.  Node state will
              be set to idle so that Moab can still schedule on the node and
              bring it back up if necessary */

          MDB(3,fSCHED) MLog("INFO:     power off complete for node %s\n",
            N->Name);
          }
        else
          {
          /* system job has not finished, node is not off */

          break;
          }
        }  /* END for (nindex) */

      if (N == NULL)
        {
        if (J->System->VCPVar1 != NULL)
          {
          mjob_t *PowerJ;

          /* green power off job completed */

          PowerJ = (mjob_t *)J->System->VCPVar1;

          MURemoveAction(&PowerJ->TemplateExtensions->Action,J->Name);
          }

        *State = mjsCompleted;
        }
      else if ((long)(MSched.Time - J->StartTime) > J->System->WCLimit)
        {
        /* job has run too long */

        *State = mjsRemoved;

        break;
        }
      }    /* END BLOCK (case mjcpPowerOff) */

      break;

    case mjcpPowerOn:

      /* NOTE:  only works if RM '0' is master RM for node */

      {
      int nindex;
      int rindex;

      mnode_t *N;

      enum MNodeStateEnum NState;

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        if (N == MSched.GN)
          continue;

        rindex = 0;

        NState = mnsDown;

        if ((bmisset(&N->RMAOBM,mnaPowerIsEnabled)) &&
            (N->PowerState == mpowOn) &&
            (MNodeGetRMState(N,mrmrtAny,&NState) == SUCCESS) &&
            (MNSISUP(NState)))
          {
          /* RM reports power state */
          /* if node power is on and ANY rm reports the node as up then we're done */

          N->PowerState = mpowOn;

          continue;
          }

        if ((N->RM != NULL) && !bmisset(&N->RM->RTypes,mrmrtProvisioning))
          {
          rindex = N->RM->Index;
          }
        else if (MPowerGetPowerStateRM(&rindex) == FAILURE)
          {
          bmset(&J->System->EFlags,msjefOnlyProvRM);
          }
        else
          {
          bmset(&J->System->EFlags,msjefOnlyProvRM);
          }

        if (MNODESTATEISUP(N->RMState[rindex]))
          {
          N->PowerState = mpowOn;
          }
        else
          {
          /* system job has not finished, node is not on */

          break;
          }
        }  /* END for (nindex) */

      if (N == NULL)
        {
        if (J->System->VCPVar1 != NULL)
          {
          mjob_t *PowerJ;

          /* green power off job completed */

          PowerJ = (mjob_t *)J->System->VCPVar1;

          MURemoveAction(&PowerJ->TemplateExtensions->Action,J->Name);
          }

        *State = mjsCompleted;

        MDB(3,fSTRUCT) MLog("INFO:     poweron job %s completed\n",
          J->Name);
        }  /* END if (N == NULL) */
      else if ((long)(MSched.Time - J->StartTime) > J->System->WCLimit)
        {
        /* job has run too long */

        *State = mjsRemoved;

        break;
        }
      }    /* END BLOCK (case mjcpPowerOn) */

      break;

    case mjcpReset:

      if (MNLIsEmpty(&J->NodeList))
        {
        /* job is corrupt */

        return(FAILURE);
        }
      else
        {
        mnode_t *N = MNLReturnNodeAtIndex(&J->NodeList,0);

        if (N->State == mnsIdle)
          *State = mjsCompleted;
        }

      break;

    case mjcpStorage:
      
      {
      mtrig_t *T;

      if (MSysJobGetGenericTrigger(J,&T) == FAILURE)
        {
        return(FAILURE);
        }

      switch (T->State)
        {
        case mtsSuccessful:

          {
          char StorageVar[MMAX_LINE];
          char *OBuf;
          mjob_t *DepJob;

          if ((J->System->Dependents != NULL) && (
              MJobFind(J->System->Dependents->Name,&DepJob,mjsmBasic) == SUCCESS) &&
              ((OBuf = MFULoadNoCache(T->OBuf,1,NULL,NULL,NULL,NULL)) != NULL))
            {
            snprintf(StorageVar,MMAX_LINE,"MOAB_STORAGEID=%s",OBuf);
            MJobSetAttr(DepJob,mjaVariables,(void **)StorageVar,mdfString,mAdd);
            MJobSetAttr(DepJob,mjaEnv,(void **)StorageVar,mdfString,mAdd);

            if (DepJob->Rsv != NULL)
              DepJob->Rsv->StartTime = MSched.Time;

            if (J->System->ModifyRMJob == TRUE) 
              { 
               MJobAddEnvVar(DepJob,"MOAB_STORAGEID",OBuf,',');

              MRMJobModify(
                DepJob,
                "Variable_List",
                NULL,
                DepJob->Env.BaseEnv,
                mSet,
                NULL,
                NULL,
                NULL);
              }

            MUFree(&OBuf);
            }

          *State = mjsCompleted;
          }  /* END BLOCK */

          break;

        case mtsFailure:

          *State = mjsRemoved;

          break;

        default:

          break;
        }  /* END switch(J->System->Trig->State) */
      }    /* END case mjcpStorage */
        
      break;

    default:

      /* no info: job is done */

      *State = mjsCompleted;

      break;
    }    /* END switch (J->System->CompletionPolicy) */

  if (*State == mjsCompleted)
    {
    MSysJobProcessCompleted(J);
    }
  else if (*State == mjsRemoved)
    {
    MSysJobProcessFailure(J);
    }
  else if (bmisset(&J->IFlags,mjifSystemJobFailure))
    {
    *State = mjsRemoved;

    MSysJobProcessFailure(J);
    }

  return(SUCCESS);
  }  /* END MSysJobUpdate() */





/**
 * Perform 'deep' free of J->System
 *
 * @param J (I) [modified]
 */

int MJobFreeSystemStruct(

  mjob_t *J)  /* I (modified) */

  {
  const char *FName = "MJobFreeSystemStruct";

  MDB(7,fCONFIG) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->System == NULL)
    {
    return(SUCCESS);
    }

  /* perform 'deep' memory free */

  if (J->System->TList != NULL)
    {
    /* NOTE:  must launch triggers before freeing required job structures */

    MTListClearObject(J->System->TList,FALSE);

    MUArrayListFree(J->System->TList);

    MUFree((char **)&J->System->TList);

    J->System->TList = NULL;
    }

  if (J->System->SCPVar1 != NULL)
    MUFree(&J->System->SCPVar1);

  if (J->System->ProxyUser != NULL)
    MUFree(&J->System->ProxyUser);

  if (J->System->Action != NULL)
    MUFree(&J->System->Action);

  MUFree(&J->System->VM);
  MUFree(&J->System->VMSourceNode);
  MUFree(&J->System->VMDestinationNode);
  MUFree(&J->System->RecordEventInfo);

  MUFree(&J->System->VMJobToStart);

  MULLFree(&J->System->Dependents,NULL);

  MUFree((char **)&J->System);

  return(SUCCESS);
  }  /* END MJobFreeSystemStruct() */








/**
 * Perform post-submit processing of VM Migration system job, including
 * allocating system job information and setting appropriate flags
 *
 * @param J       (I) [modified]
 * @param V       (I)
 * @param DstNode (I)
 */

int MUVMMigratePostSubmit(

  mjob_t  *J,
  mvm_t   *V,
  mnode_t *DstNode)

  {
  if ((J == NULL) || (V == NULL))
    {
    return(FAILURE);
    }

  bmset(&J->RsvAccessBM,mjapAllowAllNonJob);
  bmset(&J->IFlags,mjifReserveAlways);

  MJobCreateSystemData(J);

  J->System->JobType          = msjtVMMigrate;
  J->System->CompletionPolicy = mjcpVMMigrate;

  MUStrDup(&J->System->VM,V->VMID);
  MUStrDup(&J->System->VMSourceNode,V->N->Name);
  MUStrDup(&J->System->VMDestinationNode,DstNode->Name);

  J->System->Action = MUStrFormat(
    "node:%s:migrate:%s:%s",
    V->N->Name,
    V->VMID,
    DstNode->Name);

  bmset(&J->IFlags,mjifReserveAlways);

  /* DO NOT set the RunAlways flag because it will disable
      throttling vmmigrate jobs with gres */

  MUAddAction(&V->Action,J->Name,J);
  MUAddAction(&V->N->Action,J->Name,J);
  MUAddAction(&DstNode->Action,J->Name,J);

  J->System->WCLimit = J->WCLimit;

  return(SUCCESS);
  }  /* END MUVMMigratePostSubmit() */




/**
 * Determines if the nodelist has a node that requires
 * a power-on job.  
 */

mbool_t MUNLNeedsPower(

  mnl_t  *NL)    /* I */

  {
  enum MNodeStateEnum State;

  int nindex;

  mnode_t *N;

  if (NL == NULL)
    {
    return(FAILURE);
    }

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++) 
    {
    if ((MNodeGetRMState(N,mrmrtCompute,&State) == FAILURE) ||
        (State == mnsDown))
      {
      break;
      }

    MDB(7,fSTRUCT) MLog("INFO:     node '%s' in state '%s' and power state '%s' does not need provisioning\n",
      N->Name,
      MNodeState[N->State],
      MPowerState[N->PowerState]);
    }  /* END for (nindex) */

  if (MNLGetNodeAtIndex(NL,nindex,NULL) == FAILURE)
    return(FALSE);

  return(TRUE);
  }  /* END MUNLNeedsPower() */




/**
 * Determines if the nodelist already has the OS
 * which is required by the job. This function will stop and return TRUE
 * once a single node is hit that requires provisioning.
 *
 * @see MJobStart() - parent
 * @see MSysJobUpdate() - parent
 *
 * @param ReqOS (I)
 * @param IsVMOS (I)
 * @param NL (I)
 * @param ProvTypeP (O) [optional]
 */

mbool_t MUNLNeedsOSProvision(

  int                      ReqOS,     /* I */
  mbool_t                  IsVMOS,    /* I */
  mnl_t                   *NL,        /* I */
  enum MSystemJobTypeEnum *ProvTypeP) /* O (optional) */

  {
  int nindex;

  mnode_t *N = NULL;
  enum MSystemJobTypeEnum ProvRequired = msjtNONE;
 
   /* NOTE: need to change assert to dump log message */

  /* assert(NL != NULL); */

  if (ProvTypeP != NULL)
    *ProvTypeP = msjtNONE;

  if (NL == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  must determine if any nodes require two-stage provisioning */

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (MNodeNeedsOSProvision(ReqOS,IsVMOS,N,&ProvRequired) == TRUE)
      {
      break;
      }
    }  /* END for (nindex) */

  if (ProvRequired == msjtNONE)
    {
    return(FALSE);
    }                   

  MDB(7,fSTRUCT) MLog("INFO:     node %s in state %s and power state %s needs %s provisioning\n",
    N->Name,
    MNodeState[N->State],
    MPowerState[N->PowerState],
    MSysJobType[ProvRequired]);

  if (ProvTypeP != NULL)
    *ProvTypeP = ProvRequired;

  return(TRUE);
  }  /* END MUNLNeedsOSProvision() */




/**
 *
 * This function checks an individual node to see if the given ReqOS
 * is already installed or if the node needs to be reprovisioned.
 *
 * @param ReqOS [I] The OS index that is required.
 * @param IsVMOS [I] Specifies whether or not we are checking for a VM operating system.
 * @param N [I] The node to check.
 * @param ProvTypeP [O] The type of provisioning required for this node.
 */

mbool_t MNodeNeedsOSProvision(

  int         ReqOS,     /* I */
  mbool_t     IsVMOS,    /* I */
  mnode_t    *N,         /* I */
  enum MSystemJobTypeEnum *ProvTypeP) /* O (optional) */

  {
  enum MNodeStateEnum State;

  enum MSystemJobTypeEnum ProvRequired = msjtNONE;

  if (ProvTypeP != NULL)
    *ProvTypeP = msjtNONE;

  if (N == NULL)
    {
    return(MBNOTSET);
    }

  if (N == MSched.GN)
    {
    MDB(7,fSTRUCT) MLog("INFO:     virtual node '%s' does not need provisioning\n",
      N->Name);

    return(FALSE);
    }

  /* ReqOS no or ReqOS "any" are not sufficient conditions to provision a node, prima facie */

  if (IsVMOS == TRUE)
    {
    mbool_t NeedsProv = FALSE;

    if (ReqOS == 0)
      {
      if (N->HVType == mhvtNONE)
        {
        int osindex;
        NeedsProv = TRUE;

        /* Look for a valid hypervisor OS */

        for (osindex = 1;osindex < MMAX_ATTR;osindex++)
          {
          char *OS = MAList[meOpsys][osindex];

          if (OS[0] == '\0')
            {
            break;
            }

          if (MSched.VMOSList[osindex].NumItems > 0)
            {
            /* Found a hypervisor OS */

            ReqOS = osindex;

            break;
            }
          }

        if (ReqOS == 0)
          {
          /* Could not find a hypervisor OS */
          /* TODO: what do we do at this point? */

          assert(FALSE);

          return(TRUE);
          }
        }
      }
    else
      {
      if (MUHTGet(&MSched.VMOSList[N->ActiveOS],MAList[meOpsys][ReqOS],NULL,NULL) == FAILURE)
        {
        NeedsProv = TRUE;
        }
      }

    if (NeedsProv == TRUE)
      {
      ProvRequired = msjtOSProvision;
      }
    }    /* END if (IsVMOS == TRUE) */
  else if ((N->ActiveOS != ReqOS) &&
           (ReqOS > 0) &&
           (strcmp(MAList[meOpsys][ReqOS],ANY) != 0))
    {
    ProvRequired = msjtOSProvision;
    }   /* END if ((N->ActiveOS != ReqOS) && ...) */

  if (MNodeGetRMState(N,mrmrtCompute,&State) == SUCCESS)
    {
    if (!MNSISUP(State))
      {
      /* node is down - provision to bring online */

      ProvRequired = msjtOSProvision;
      }
    }
  else if (MNodeGetRMState(N,mrmrtAny,&State) == SUCCESS)
    {
    if (!MNSISUP(State))
      {
      /* node is down - provision to bring online */

      ProvRequired = msjtOSProvision;
      }
    }

  if ((N->PowerSelectState == mpowOff) && (N->PowerState == mpowOff))
    {
    /* node is down, provision to bring online */

    ProvRequired = msjtOSProvision;
    }

  if (ProvRequired == msjtNONE)
    {
    return(FALSE);
    }

  if (ProvTypeP != NULL)
    *ProvTypeP = ProvRequired;

  return(TRUE);
  }  /* END MNodeNeedsOSProvision() */






/**
 * Create/submit VM migration system job
 *
 * @see MUGenerateSystemJobs() - peer - possibly merge routines?
 *
 * NOTE:  If calling this function as part of an automatic Moab decision
 *         (not a user decision to migrate), first check MSched.DisableVMDecisions.
 *         If set to true, this function should not be called, but instead log the
 *         intended migration in the event logs (grep for DisableVMDecisions for 
 *         examples).
 *
 *  @TODO change this function to assign the one task to both the source
 *  node and the destination node, instead of just the destination node.
 *
 * @param V       (I)
 * @param DstNode (I)
 * @param User    (I)
 * @param JP      (O) [optional] - set on SUCCESS
 * @param Msg     (I) [optional]
 * @param SEMsg   (O) [optional,minsize=MMAX_LINE] - set on FAILURE
 */

int MUSubmitVMMigrationJob(

  mvm_t    *V,
  mnode_t  *DstNode,
  char     *User,
  mjob_t  **JP,
  char     *Msg,
  char     *SEMsg)

  {
  mjob_t *J;
  mjob_t *tmpJ;

  mulong  JDuration;

  char    tmpName[MMAX_NAME];
  char    tmpEMsg[MMAX_LINE];
  char    tmpVar[MMAX_LINE];
  char   *EMsg = (SEMsg != NULL) ? SEMsg : tmpEMsg;

  const char *FName = "MUSubmitVMMigrationJob";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,JP,EMsg)\n",
    FName,
    (V != NULL) ? V->VMID : "",
    (DstNode != NULL) ? DstNode->Name : "");

  if ((V == NULL) || (V->N == NULL) || (DstNode == NULL) || (V->N == DstNode)) 
    {
    MDB(3,fSTRUCT) MLog("ERROR:    bad parameters passed to %s\n",
      FName);

    return(FAILURE);
    }
  if (JP != NULL)
    *JP = NULL;

  EMsg[0] = '\0';

  J = NULL;

  if (V->N->PtIndex != DstNode->PtIndex)
    {
    /* Don't allow migration across partitions */

    snprintf(EMsg,MMAX_LINE,"Cannot migrate VM %s across partitions (requested partition: %s)",
      V->VMID,
      MPar[DstNode->PtIndex].Name);

    return(FAILURE);
    }

  if (bmisset(&V->Flags,mvmfDestroyPending) ||
      bmisset(&V->Flags,mvmfDestroyed))
    {
    snprintf(EMsg,MMAX_LINE,"VM %s is already deleted or pending deletion",
      V->VMID);

    return(FAILURE);
    }

  if (MUCheckAction(&V->Action,msjtVMMigrate,NULL) == SUCCESS)
    {
    snprintf(EMsg,MMAX_LINE,"VM %s is currently being migrated",
      V->VMID);

    return(FAILURE);
    }

  if (MUCheckAction(&V->Action,msjtGeneric,&tmpJ) == SUCCESS)
    {
    if (MVMJobIsVMMigrate(tmpJ) == TRUE)
      {
      snprintf(EMsg,MMAX_LINE,"VM %s is currently being migrated",
        V->VMID);

      return(FAILURE);
      }
    }

  if (MSched.Mode == msmMonitor)
    {
    MSchedAddTestOp(mrmNodeModify,mxoxVM,(void *)V->N,Msg);

    sprintf(EMsg,"cannot migrate vm - monitor mode");

    MDB(3,fRM) MLog("INFO:     cannot migrate vm on node %s (%s)\n",
      (V->N != NULL) ? V->N->Name : "",
      EMsg);

    return(FAILURE);
    }

#if 0
  /* Removed this section because MVMCheckVLANMigrationBoundaries tries to
    schedule a temporary job, which can fail to schedule in a full system,
    which then confuses this code to think that the VLAN boundaries are bad.

    VLANs should have been checked when selecting a destination, not after
    the decision has been made.

    See MVMFindDestination()->MVMNodeIsValidDestination()*/

  if ((MPar[0].NodeSetPolicy != mrssNONE) && (MVMCheckVLANMigrationBoundaries(V->N,DstNode) == FAILURE))
		{
		snprintf(EMsg,MMAX_LINE,"Cannot migrate VM %s - target and destination node are not in the same vlan",
			V->VMID);

		return(FAILURE);
		}
#endif

  if (CalculateMaxVMMigrations(FALSE) == 0)
    {
    snprintf(EMsg,MMAX_LINE,"too many concurrent migrations are already underway");

    return(FAILURE);
    }

  snprintf(tmpName,sizeof(tmpName),"vmmigrate-%s",
    V->VMID);

  if (MPar[V->N->PtIndex].VMMigrationDuration > 0)
    JDuration = MPar[V->N->PtIndex].VMMigrationDuration;
  else if (MPar[0].VMMigrationDuration > 0)
    JDuration = MPar[0].VMMigrationDuration;
  else
    JDuration = 10 * MCONST_MINUTELEN;

  if ((V->TrackingJ != NULL) &&
      (MTJobCreateByAction(V->TrackingJ,mtjatMigrate,DstNode->Name,&J,(MUStrIsEmpty(User)) ? MSched.Admin[1].UName[0] : User) == SUCCESS))
    {
    MUVMMigratePostSubmit(J,V,DstNode);

    /* MUVMMigratePostSubmit changes job to VMMigrate, we still need it
        to be a Generic job */

    J->System->JobType          = msjtGeneric;
    J->System->CompletionPolicy = mjcpGeneric;
    }
  else
    {
    mbitmap_t tmpL;

    bmset(&tmpL,mjfSystemJob);
    bmset(&tmpL,mjfNoRMStart);

    /* should we be using both source node and destination node? */
    /* snprintf(hostNames,sizeof(hostNames),"%s,%s", DstNode->Name,V->N->Name); */

    if (MUSubmitSysJob(
         &J,  /* O */
         msjtVMMigrate,
         DstNode->Name,
         1,
         (User == NULL) ? MSched.Admin[1].UName[0] : User,
         tmpName,
         &tmpL,
         JDuration,
         EMsg) == FAILURE)
      {
      MDB(3,fSCHED) MLog("ALERT:    failed to submit VM migration job to migrate %s from %s to %s - %s\n",
        V->VMID,
        V->N->Name,
        DstNode->Name,
        EMsg);

      bmclear(&tmpL);

      return(FAILURE);
      }  /* END if (MUSubmitSysJob() == FAILURE) */

    bmclear(&tmpL);

    MDB(3,fSCHED) MLog("INFO:     VM %s migrate submitted.  %s --> %s\n",V->VMID,V->N->Name,DstNode->Name);

    MUVMMigratePostSubmit(J,V,DstNode);
    }

  if (Msg != NULL)
    {
    MMBAdd(
      &J->MessageBuffer,
      Msg,
      (char *)"N/A",
      mmbtOther,
      0,
      1,
      NULL);

    MUStrDup(&J->System->RecordEventInfo,Msg);
    }

  snprintf(tmpVar,MMAX_LINE,"VMID=%s",
    V->VMID);

  if (MSched.PushEventsToWebService == TRUE)
    {
    MEventLog *Log = new MEventLog(meltVMMigrateStart);
    Log->SetPrimaryObject(V->VMID,mxoxVM,(void *)V);
    Log->AddDetail("Source",J->System->VMSourceNode);
    Log->AddDetail("Destination",J->System->VMDestinationNode);
    Log->AddRelatedObject(J->Name,mxoJob,(void *)J);

    MEventLogExportToWebServices(Log);

    delete Log;
    }

  MJobSetAttr(J,mjaVariables,(void **)tmpVar,mdfString,mAdd);

  if (JP != NULL)
    *JP = J;

  return(SUCCESS);
  }  /* END MUSubmitVMMigrationJob() */





/**
 * Creates job(s) targeting each node in NL.
 *
 * @see MUGenerateSystemJobs() - parent
 * @see MUSubmitSysJob() - child
 *  
 * @param Auth (I) Authentication User Name. 
 * @param NL (I) list of nodes which should have associated system jobs.
 * @param SJType (I) [optional]
 * @param JDuration (I)
 * @param AName (I) job name [optional]
 * @param Flags (I)
 * @param JArray (O)
 * @param MultiJob (I)
 *
 * NOTE:  set system priority on jobs so they are always highest priority
 */

int MUNLCreateJobs(

  const char             *Auth,      /* I */
  mnl_t                  *NL,        /* I */
  enum MSystemJobTypeEnum SJType,    /* I (optional) */
  mulong                  JDuration, /* I */
  char const             *AName,     /* I (optional) */
  mbitmap_t              *Flags,     /* I */
  marray_t               *JArray,    /* O array of jobs created */
  mbool_t                 MultiJob)  /* I one job per node */

  {
  int nindex;
  int rc;
  mbool_t loopDone = FALSE;

  mnode_t *N;

  mjob_t *J;

  const char *FName = "MUNLCreateJobs";

  MDB(5,fSTRUCT) MLog("%s(NL,%s,%ld,%s,JArray,%s)\n",
    FName,
    MSysJobType[SJType],
    JDuration,
    (AName != NULL) ? AName : "",
    MBool[MultiJob]);

  if ((NL == NULL) || (MNLIsEmpty(NL)))
    {
    MDB(2,fSTRUCT) MLog("ALERT:    empty modelist passed to %s\n",
      FName);

    return(FAILURE);
    }

  nindex = 0;

  mstring_t NodeString(MMAX_LINE);

  for (nindex = 0;((!loopDone) && (MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS));nindex++)
    {
    if (MultiJob == TRUE)
      {
      /* one job for every node */

      MStringSetF(&NodeString,"%s",N->Name);
      }
    else
      {
      /* one job for all nodes in list */

      for (;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        if (nindex == 0)
          MStringAppendF(&NodeString,"%s",N->Name);
        else
          MStringAppendF(&NodeString,",%s",N->Name);
        }
      loopDone = TRUE;
      }

    J = NULL;

    rc = MUSubmitSysJob(
      &J,
      SJType,
      NodeString.c_str(),
      0,
      (Auth == NULL) ? MSched.Admin[1].UName[0] : Auth,
      AName,
      Flags,
      JDuration,
      NULL);

    if (rc == FAILURE)
      {
      MDB(2,fSTRUCT) MLog("ALERT:    cannot create system job\n");

      return(FAILURE);
      }

    if (J == NULL)
      {
      continue;
      }

    MUArrayListAppend(JArray,&J);
    }    /* END while (NL[nindex].N != NULL) */

  return(SUCCESS);
  }  /* END MUNLCreateJobs() */





/**
 * Create job(s) based on the specified tasks.
 *
 * @see MUGenerateSystemJobs() - parent
 * @see MUSubmitSysJob() - child
 *
 * @param Auth      (I) Auth User Name.
 * @param TC        (I) Total number of tasks for all jobs generated
 * @param SJType    (I) system job type
 * @param JDuration (I) duration/wclimit of job
 * @param AName     (I) job name [optional]
 * @param Flags     (I)
 * @param JArray    (O)
 * @param MultiJob  (I)
 *
 * NOTE:  set system priority on jobs so they are always highest priority
 */

int MUTCCreateJobs(

  const char              *Auth,
  int                      TC,        /* I */
  enum MSystemJobTypeEnum  SJType,    /* I */
  mulong                   JDuration, /* I */
  char const              *AName,     /* I (optional) */
  mbitmap_t               *Flags,     /* I */
  marray_t                *JArray,    /* O */
  mbool_t                  MultiJob)  /* I one job per node */

  {
  int tmpTC = 0;
  int rc;

  mjob_t *J;

  const char *FName = "MUTCCreateJobs";

  MDB(5,fSTRUCT) MLog("%s,%d,%s,%ld,%s,%ld,JArray,%s)\n",
    FName,
    TC,
    MSysJobType[SJType],
    JDuration,
    (AName != NULL) ? AName : "",
    Flags,
    MBool[MultiJob]);

  if (TC <= 0)
    {
    return(FAILURE);
    }

  while (TC > 0)
    {
    if (MultiJob == TRUE)
      {
      TC--;
      }
    else
      {
      tmpTC = TC;

      TC = 0;
      }

    J = NULL;

    rc = MUSubmitSysJob(
      &J,  /* O */
      SJType,
      NULL,
      (MultiJob == TRUE) ? 1 : tmpTC,
      (Auth == NULL) ? MSched.Admin[1].UName[0]: Auth,
      AName,
      Flags,
      JDuration,
      NULL);

    if (rc == FAILURE)
      {
      MDB(2,fSTRUCT) MLog("ALERT:    cannot create system job\n");

      return(FAILURE);
      }

    if (J == NULL)
      {
      continue;
      }

    MUArrayListAppend(JArray,&J);
    }    /* END while (NL[nindex].N != NULL) */

  return(SUCCESS);
  }  /* END MUTCCreateJobs() */





/**
 * Submits a single system job.
 *
 * @see MUNLCreateJobs() - parent
 * @see MUTaskCreateJobs() - parent
 * @see MS3JobSubmit() - child
 *
 * NOTE: either Tasks or NodeString must be specified
 *
 * @param JP         (O)
 * @param SJType     (I) [optional]
 * @param NodeString (I) [optional]
 * @param Tasks      (I) [optional]
 * @param User       (I)
 * @param JName      (I) [optional]
 * @param Flags      (I)
 * @param WCLimit    (I)
 * @param SEMsg      (O) [optional,minsize=MMAX_LINE]
 */

int MUSubmitSysJob(

  mjob_t                **JP,
  enum MSystemJobTypeEnum SJType,
  char                   *NodeString,
  int                     Tasks,
  const char             *User,
  char const             *JName,
  mbitmap_t              *Flags,
  mulong                  WCLimit,
  char                   *SEMsg)

  {
  char tmpBuf[MMAX_BUFFER];
  char SJobID[MMAX_NAME];
  char tmpEMsg[MMAX_LINE];

  mstring_t Name(MMAX_LINE);
  mstring_t Nodes(MMAX_LINE);
  mstring_t Flag(MMAX_LINE);
  mstring_t System(MMAX_LINE);

  mrm_t  *R;

  mjob_t *J;

  char   *EMsg;

  int     rc;

  const char *FName = "MUSubmitSysJob";

  MDB(5,fSTRUCT) MLog("%s(JP,%s,%s,%d,%s,%s,FLAGS,%ld,SEMsg)\n",
    FName,
    MSysJobType[SJType],
    (NodeString != NULL) ? NodeString : "",
    Tasks,
    (User != NULL) ? User : "",
    (JName != NULL) ? JName : "",
    WCLimit);

  EMsg = (SEMsg != NULL) ? SEMsg : tmpEMsg;

  EMsg[0] = '\0';

  if (JP != NULL)
    *JP = NULL;

  if (User == NULL)
    {
    return(FAILURE);
    }

  if ((JName != NULL) && (JName[0] != '\0'))
    {
    MStringSetF(&Name,"<JobName>%s</JobName>",
      JName);
    }

  if ((NodeString != NULL) && (NodeString[0] != '\0'))
    {
    MStringSetF(&Nodes,"<Nodes><Node>%s</Node></Nodes>",
      NodeString);
    }

  if ((Flags != NULL) && (!bmisclear(Flags)))
    {
    mstring_t tmp(MMAX_LINE);
 
    MJobFlagsToMString(NULL,Flags,&tmp);

    MStringSetF(&Flag,"<Extension>x=flags:%s</Extension>",
      tmp.c_str());
    }

  if (SJType != msjtNONE)
    {
    MStringSetF(&System,"<system %s=\"%s\"></system>",
      MJobSysAttr[mjsaJobType],
      MSysJobType[SJType]);
    }

  snprintf(tmpBuf,sizeof(tmpBuf),"<job>%s%s<UserId>%s</UserId><Executable>sleep 600</Executable><SubmitLanguage>PBS</SubmitLanguage><Requested>%s<Processors>%d</Processors><WallclockDuration>%ld</WallclockDuration></Requested>%s</job>",
    Name.c_str(),
    System.c_str(),
    User,
    Nodes.c_str(),
    Tasks,
    WCLimit,
    Flag.c_str());

  J = NULL;

  MRMAddInternal(&R);

  if (JName != NULL)
    {
    /* NOTE:  do not use '.' as intra-name delimiter since this can be
              a jobid-domain separator in MJobFind()
    */

    snprintf(SJobID,sizeof(SJobID),"%s-%d",
      JName,
      MRMJobCounterIncrement(R));
    }

  rc = MS3JobSubmit(
        tmpBuf,
        R,
        &J,
        NULL,
        (JName != NULL) ? SJobID : NULL,
        EMsg,                            /* O */
        NULL);

  if (rc == FAILURE)
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"cannot create system job - %s",
      EMsg);

    MMBAdd(
      &MSched.MB,
      tmpLine,
      (char *)"N/A",
      mmbtOther,
      0,
      1,
      NULL);

    if (MSched.ProvRM != NULL)
      {
      MMBAdd(
        &MSched.MB,
        tmpLine,
        (char *)"N/A",
        mmbtOther,
        0,
        1,
        NULL);
      }

    if (J != NULL)
      MJobDestroy(&J);

    return(FAILURE);
    }  /* END if (rc == FAILURE) */

  if (JP != NULL)
    *JP = J;

  return(SUCCESS);
  }  /* END MUSubmitSysJob() */






/**
 * Report system job attributes.
 *
 * @see MUIJobShow() - process 'checkjob'
 *
 * @param J            (I)
 * @param VerboseLevel (I)
 * @param Buffer       (I/O) Should already be initalized
 */

int MJobShowSystemAttrs(

  mjob_t    *J,
  int        VerboseLevel,
  mstring_t *Buffer)

  {
  struct msysjob_t *S;

  if ((J == NULL) || (J->System == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  S = J->System;

  if (S->JobType != msjtNONE)
    {
    MStringAppendF(Buffer,"  SJob Type:             %s\n",
      MSysJobType[S->JobType]);
    }

  if (S->CompletionPolicy != mjcpNONE)
    {
    MStringAppendF(Buffer,"  Completion Policy:     %s\n",
      MJobCompletionPolicy[S->CompletionPolicy]);
    }

  if (S->Action != NULL)
    {
    MStringAppendF(Buffer,"  SJob Action:           %s\n",
      S->Action);
    }

  if (S->Dependents != NULL)
    {
    char tmpLine[MMAX_LINE];

    MSysJobAToString(
      S,
      mjsaDependents,
      tmpLine,
      sizeof(tmpLine),
      mfmNONE,
      FALSE);

    MStringAppendF(Buffer,"  Dependent Jobs:        %s\n",
      tmpLine);
    }

  if (S->ProxyUser != NULL)
    {
    char tmpLine[MMAX_LINE];

    MSysJobAToString(
      S,
      mjsaProxyUser,
      tmpLine,
      sizeof(tmpLine),
      mfmNONE,
      FALSE);

    MStringAppendF(Buffer,"  Proxy User:        %s\n",
      tmpLine);
    }

  switch (S->JobType)
    {
    case msjtOSProvision:

      if (!MNLIsEmpty(&J->NodeList))
        {
        int nindex;

        mnode_t   *N;

        mnl_t     *NL = &J->NodeList;

        int totalPNodes;
        int completePNodes;

        enum MNodeStateEnum State;

        mbool_t Complete;

        MStringAppendF(Buffer,"  Provision Target:      %s\n",
          MAList[meOpsys][S->ICPVar1]);

        /* report status for each node for jobs provisioning
         * more than one node (i.e. on demand provisioning) */

        if (MNLGetNodeAtIndex(NL,1,&N) == SUCCESS)
          {
          MStringAppend(Buffer,"  Provisioning Progress:");
   
          if (VerboseLevel > 0)
            {
            MStringAppend(Buffer,"\n");
            }
   
          completePNodes = 0;
          totalPNodes = 0;
   
          for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
            {
            State = mnsNONE;
   
            Complete = ((N->ActiveOS == S->ICPVar1) &&
                        (MNodeGetRMState(N,mrmrtCompute,&State) == SUCCESS) &&
                        (MNSISUP(State)));
   
            if (VerboseLevel > 0)
              {
              MStringAppendF(Buffer, "    %s: %s\n",
                N->Name,
                (Complete ? "complete" : "in progress" ));
              }
   
            if (Complete)
              ++completePNodes;
   
            ++totalPNodes;
            }  /* END for (nindex) */
   
          MStringAppendF(Buffer, "%s %i/%i nodes provisioned\n",
            ((VerboseLevel > 0) ? "   " : ""),
            completePNodes,
            totalPNodes);
          }
        }    /* END BLOCK (case msjtOSProvision) */

      break;

    case mjcpPowerOff:
    case mjcpPowerOn:

      if (bmisset(&S->EFlags,msjefOnlyProvRM))
        {
        MStringAppendF(Buffer, "  ERROR:  the only RM is a provisioning RM which cannot determine power state.  job will fail. \n");
        }

      break;

    default:

      if (S->ICPVar1 != 0)
        {
        MStringAppendF(Buffer,"  SJob IVar:  %d\n",
          S->ICPVar1);
        }

      break;
    }  /* END switch (S->JobType) */

  return(SUCCESS);
  }  /* END MJobShowSystemAttrs() */






/**
 * Populate J->System with correct information to execute action.
 *
 * @param J             (I) [modified]
 * @param JType         (I)
 * @param SystemJobData (I)
 */

int MJobPopulateSystemJobData(

  mjob_t                  *J,
  enum MSystemJobTypeEnum  JType,
  void                    *SystemJobData)

  {
  if ((J == NULL) || (SystemJobData == NULL))
    {
    return(FAILURE);
    }

  if (J->System == NULL)
    {
    MJobCreateSystemData(J);
    }

  switch (JType)
    {
#if 0
    case msjtVMCreate:

      {
      char tmpBuf[MMAX_BUFFER];
      char *BPtr;
      int BSpace;

      mvm_req_create_t *VMCR = (mvm_req_create_t *)SystemJobData;

      if (VMCR->VMID[0] == 0)
        {
        MVMGetUniqueID(VMCR->VMID);
        }

      if ((VMCR->OSIndex <= 0) ||
         (VMCR->OSIndex >= MMAX_ATTR) ||
         (MAList[meOpsys][VMCR->OSIndex][0] == '\0'))
        {
        /* assert(FALSE); */

        /* behave more gracefully if not in debug mode */

        MDB(1,fALL) MLog("ERROR:     VM create request OSIndex is invalid: value=%d\n",
          VMCR->OSIndex);

        return(FAILURE);
        }

      MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

      /* format: <OS> */

      MUSNPrintF(&BPtr,&BSpace,"node:$(HOSTLIST):create:%s:%s=%s",
        VMCR->VMID,
        MWikiNodeAttr[mwnaOS],
        MAList[meOpsys][VMCR->OSIndex]);

      if (VMCR->CRes.Disk > 0)
        {
        MUSNPrintF(&BPtr,&BSpace,":%s=%d",
          MWikiNodeAttr[mwnaCDisk],
          VMCR->CRes.Disk);
        }

      if (VMCR->CRes.Mem > 0)
        {
        MUSNPrintF(&BPtr,&BSpace,":%s=%d",
          MWikiNodeAttr[mwnaCMemory],
          VMCR->CRes.Mem);
        }

      if (VMCR->CRes.Procs > 0)
        {
        MUSNPrintF(&BPtr,&BSpace,":%s=%d",
          MWikiNodeAttr[mwnaCProc],
          VMCR->CRes.Procs);
        }

      if (VMCR->Aliases[0] != '\0')
        {
        MUSNPrintF(&BPtr,&BSpace,":ALIAS=%s",
          VMCR->Aliases);
        }

      if (VMCR->Vars[0] != '\0')
        {
        char *ptr;
        char *TokPtr;
        char *TokPtr2;
        mbool_t VarPrinted = FALSE;
        char  tmpVars[MMAX_LINE*10];

        MUSNPrintF(&BPtr,&BSpace,":%s=",
          MWikiNodeAttr[mwnaVariables]);

        /* Copy off the variables so parsing doesn't mess up the string */

        MUStrCpy(tmpVars,VMCR->Vars,sizeof(tmpVars));

        ptr = MUStrTokEPlusLeaveQuotes(tmpVars,"+",&TokPtr);

        while (ptr != NULL)
          {
          char *AName;
          char *AVal;

          AName = MUStrTokEPlusLeaveQuotes(ptr,"=:",&TokPtr2);
          AVal = TokPtr2;

          if (VarPrinted)
            MUSNPrintF(&BPtr,&BSpace,"+");

          MUSNPrintF(&BPtr,&BSpace,"%s",
            AName);

          if (AVal != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"=%s",
              AVal);
            }

          VarPrinted = TRUE;

          ptr = MUStrTokEPlusLeaveQuotes(NULL,"+",&TokPtr);
          } /* END while (ptr != NULL) */
        } /* END if (VMCR->Vars[0] != '\0') */

      MUStrDup(&J->System->Action,tmpBuf);
      MUStrDup(&J->System->VM,VMCR->VMID);

      /* SCPVar1 will be used to check if the VM has been created yet*/

      MUStrDup(&J->System->SCPVar1,VMCR->VMID);

      J->System->CompletionPolicy = mjcpVMCreate;

      J->SystemPrio = 1;
      }  /* END BLOCK (case msjtVMCreate) */

      break;
#endif

    case msjtVMMap:

      if (J->Req[0] == NULL)
        {
        return(FAILURE);
        }

      {
      mvm_t *V = (mvm_t *)SystemJobData;

      MCResCopy(&J->Req[0]->DRes,&V->CRes);
      }  /* END BLOCK (case msjtVMMap) */

      break;


    default:

      /* NO-OP */

      break;
    }  /* END switch (JType) */

  return(SUCCESS);
  }  /* END MJobPopulateSystemJobData() */






/**
 * Populate J->System from XML
 *
 * @see MJobSystemToXML() - peer
 *
 * @param S (I) [modified]
 * @param JSE (I)
 */


int MJobSystemFromXML(

  msysjob_t *S,    /* I (modified) */
  mxml_t    *JSE)  /* I */

  {
  enum MJobSysAttrEnum AttrIndex;

  int aindex;

  if ((S == NULL) || (JSE == NULL))
    {
    return(FAILURE);
    }

  /* TODO: checkpoint CFlags */

  for (aindex = 0;aindex < JSE->ACount;aindex++)
    {
    char const *Name = JSE->AName[aindex];
    char const *Value = JSE->AVal[aindex];

    AttrIndex = (enum MJobSysAttrEnum)MUGetIndexCI(Name,MJobSysAttr,FALSE,mjsaLAST);

    if (AttrIndex == mjsaLAST)
      {
      MDB(3,fCKPT) MLog("ALERT:    cannot process system job attribute %s with value=%s\n",
        Name,
        Value);

      continue;
      }

    switch (AttrIndex)
      {
      case mjsaAction:

        MUStrDup(&S->Action,Value);

        break;

      case mjsaSAction:

        MUStrDup(&S->SAction,Value);

        break;

      case mjsaFAction:

        MUStrDup(&S->FAction,Value);

        break;

      case mjsaSCPVar1:

        MUStrDup(&S->SCPVar1,Value);

        break;

      case mjsaProxyUser:

        MUStrDup(&S->ProxyUser,Value);

        break;

      case mjsaVM:

        MUStrDup(&S->VM,Value);

        break;

      case mjsaVMToMigrate:

        MUStrDup(&S->VM,Value);

        break;

      case mjsaVMSourceNode:

        MUStrDup(&S->VMSourceNode,Value);

        break;

      case mjsaVMDestinationNode:

        MUStrDup(&S->VMDestinationNode,Value);

        break;

      case mjsaDependents:

        {
        char *ptr;
        char *TokPtr;

        char  tmpBuf[MMAX_BUFFER];

        MUStrCpy(tmpBuf,Value,sizeof(tmpBuf));

        ptr = MUStrTok(tmpBuf,",",&TokPtr);

        while (ptr != NULL)
          {
          MULLAdd(&S->Dependents,ptr,NULL,NULL,NULL);

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END BLOCK (case mjsaDependents) */

        break;

      case mjsaJobType:

        S->JobType = (enum MSystemJobTypeEnum)MUGetIndexCI(Value,MSysJobType,FALSE,mjsNONE);

        break;
      
      case mjsaJobToStart:

        MUStrDup(&S->VMJobToStart,Value);

        break;

      case mjsaCFlags:

        bmfromstring(Value,MSystemJobCompletionAction,&S->CFlags);

        break;

      case mjsaCompletionPolicy:

        S->CompletionPolicy = (enum MJobCompletionPolicyEnum)MUGetIndexCI(Value,MJobCompletionPolicy,FALSE,mjcpNONE);

        break;

      case mjsaWCLimit:

        if (!strcmp(Value,"INFINITY"))
          {
          S->WCLimit = MMAX_TIME;
          }
        else
          {
          S->WCLimit = strtol(Value,NULL,10);
          }

        break;

      case mjsaICPVar1:

        {
        char *EndPtr = NULL;

        S->ICPVar1 = strtol(Value,&EndPtr,10);

        if (EndPtr == Value)
          {
          MDB(3,fCKPT) MLog("ALERT:    cannot read integer value for attribute %s value=%s\n",
            Name,
            Value);
          }
        }  /* END case mjsaICPVar1 */

        break;

      case mjsaRetryCount:

        {
        char *EndPtr = NULL;

        S->RetryCount = strtol(Value,&EndPtr,10);

        if (EndPtr == Value)
          {
          MDB(3,fCKPT) MLog("ALERT:    cannot read integer value for attribute %s value=%s\n",
            Name,
            Value);
          }
        }  /* END case mjsaRetryCount */

        break;

      default:

        /* NOT HANDLED */

        MDB(3,fCKPT) MLog("ALERT:    cannot process attribute %s with value %s\n",
          Name,
          Value);

        break;
      }  /* END switch (AttrIndex) */
    }    /* END for (aindex) */

  return(SUCCESS);
  }  /* END MJobSystemFromXML() */




/**
 * copy msysjob_t struct
 *
 * @param Dst (O) modified
 * @param Src (I)
 */
int MJobSystemCopy(

  msysjob_t       *Dst,
  msysjob_t const *Src)

  {
  MUStrDup(&Dst->Action,Src->Action);
  MUStrDup(&Dst->FAction,Src->FAction);
  MUStrDup(&Dst->SAction,Src->SAction);
  MUStrDup(&Dst->SCPVar1,Src->SCPVar1);
  MUStrDup(&Dst->ProxyUser,Src->ProxyUser);
  MUStrDup(&Dst->VM,Src->VM);
  MUStrDup(&Dst->VMDestinationNode,Src->VMDestinationNode);
  MUStrDup(&Dst->VMSourceNode,Src->VMSourceNode);
  MUStrDup(&Dst->RecordEventInfo,Src->RecordEventInfo);

  MULLDup(&Dst->Dependents,Src->Dependents);
  MULLDup(&Dst->Variables,Src->Variables);

  bmcopy(&Dst->MCMFlags,&Src->MCMFlags);
  bmcopy(&Dst->CFlags,&Src->CFlags);
  bmcopy(&Dst->EFlags,&Src->EFlags);

  Dst->JobType = Src->JobType;
  Dst->CompletionPolicy = Src->CompletionPolicy;
  Dst->ICPVar1 = Src->ICPVar1;
  Dst->RetryCount = Src->RetryCount;
  Dst->WCLimit = Src->WCLimit;
  Dst->IsAdmin = Src->IsAdmin;
  Dst->VCPVar1 = Src->VCPVar1;

  /* TODO: how to copy SRsv? */
  return(SUCCESS);
  } /* END MJobSystemCopy() */






/**
 * Translate msysjob_t struct to mxml_t struct.
 *
 * @see MJobSystemFromXML() - peer
 * @see MSysJobAToString() - child
 *
 * @param S (I)
 * @param JSE (O) [alloc]
 */

int MJobSystemToXML(

  msysjob_t  *S,   /* I */
  mxml_t    **JSE) /* O (alloc) */

  {
  int  aindex;
  int  tmpBufSize;

  char tmpBuf[MMAX_BUFFER];

  const enum MJobSysAttrEnum AList[] = {
    mjsaAction,
    mjsaCFlags,
    mjsaCompletionPolicy,
    mjsaDependents,
    mjsaFAction,
    mjsaICPVar1,
    mjsaJobToStart,
    mjsaJobType,
    mjsaRetryCount,
    mjsaSAction,
    mjsaSCPVar1,
    mjsaProxyUser,
    mjsaVM,
    mjsaVMDestinationNode,
    mjsaVMSourceNode,
    mjsaVMToMigrate,
    mjsaWCLimit,
    mjsaLAST };

  mxml_t *E = NULL;

  tmpBufSize = sizeof(tmpBuf);

  /* TODO: checkpoint CFlags */

  if ((S == NULL) || (JSE == NULL))
    {
    return(FAILURE);
    }

  *JSE = NULL;

  if (MXMLCreateE(&E,(char *)"system") == FAILURE)
    {
    return(FAILURE);
    }

  for (aindex = 0;AList[aindex] != mjsaLAST;aindex++)
    {
    if ((MSysJobAToString(S,AList[aindex],(char *)&tmpBuf,tmpBufSize,mfmNONE,FALSE) == SUCCESS) &&
        (tmpBuf[0] != '\0'))
      {
      MXMLSetAttr(E,(char *)MJobSysAttr[AList[aindex]],tmpBuf,mdfString);
      }
    } /* END for (aindex) */

  *JSE = E;

  return(SUCCESS);
  }  /* END MJobSystemToXML() */





int MSysJobAToString(

  msysjob_t             *S,       /* I */
  enum MJobSysAttrEnum   AIndex,  /* I */
  char                  *Buf,     /* O (realloc,minsize=MMAX_LINE) */
  int                    BufSize, /* I/O (modified) */
  enum MFormatModeEnum   Mode,    /* I (not used except by mjaVariables) */
  mbool_t                WikiFormat) /* Return data in Wiki form. Example: "AttrName=<AttrValue>" */    

  {
  if ((Buf == NULL) || (S == NULL))
    return(FAILURE);

  Buf[0] = '\0';

  switch (AIndex)
    {
    case mjsaAction:

      if (S->Action != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->Action);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->Action);
        }

      break;

    case mjsaCFlags:

      if (!bmisclear(&S->CFlags))
        {
        mstring_t tmp(MMAX_LINE);

        bmtostring(&S->CFlags,MSystemJobCompletionAction,&tmp);

        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",tmp.c_str());
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],tmp.c_str());
        }

      break;

    case mjsaCompletionPolicy:

      if (S->CompletionPolicy != mjcpNONE)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",MJobCompletionPolicy[S->CompletionPolicy]);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],MJobCompletionPolicy[S->CompletionPolicy]);
        }

      break;

    case mjsaDependents:

      {
      char *BPtr = NULL;
      int   BSpace = 0;

      mln_t *tmpL;

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      tmpL = S->Dependents;

      if ((WikiFormat) && (tmpL != NULL))
        MUSNPrintF(&BPtr,&BSpace,"%s=",MJobSysAttr[AIndex]);

      while (tmpL != NULL)
        {
        if (!WikiFormat)
          MUSNPrintF(&BPtr,&BSpace,"%s%s",(tmpL != S->Dependents) ? "," : "",tmpL->Name);

        tmpL = tmpL->Next;
        }  /* END while (tmpL != NULL) */
      }    /* END BLOCK (case mjsaDependents) */

      break;

    case mjsaFAction:

      if (S->FAction != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->FAction);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->FAction);
        }

      break;

    case mjsaICPVar1:

      if (S->ICPVar1 > 0)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%d",S->ICPVar1);
        else
          snprintf(Buf,BufSize,"%s=%d",MJobSysAttr[AIndex],S->ICPVar1);
        }

      break;

    case mjsaJobType:

      if (S->JobType != msjtNONE)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",MSysJobType[S->JobType]);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],MSysJobType[S->JobType]);
        }

      break;

    case mjsaJobToStart:

      if (S->VMJobToStart != NULL)
        {
        if (!WikiFormat)
          MUStrCpy(Buf,S->VMJobToStart,BufSize);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->VMJobToStart);
        }

      break;

    case mjsaRetryCount:

      if (S->RetryCount > 0)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%d",S->RetryCount);
        else
          snprintf(Buf,BufSize,"%s=%d",MJobSysAttr[AIndex],S->RetryCount);
        }

      break;

    case mjsaSAction:

      if (S->SAction != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->SAction);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->SAction);
        }

      break;

    case mjsaSCPVar1:

      if (S->SCPVar1 != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->SCPVar1);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->SCPVar1);
        }

      break;

    case mjsaProxyUser:

      if (S->ProxyUser != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->ProxyUser);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->ProxyUser);
        }

      break;

    case mjsaVM:

      if (S->VM != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->VM);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->VM);
        }

      break;

    case mjsaVMDestinationNode:

      if (S->VMDestinationNode != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->VMDestinationNode);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->VMDestinationNode);
        }

      break;

    case mjsaVMSourceNode:

      if (S->VMSourceNode != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->VMSourceNode);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->VMSourceNode);
        }

      break;

    case mjsaVMToMigrate:

      if (S->VM != NULL)
        {
        if (!WikiFormat)
          snprintf(Buf,BufSize,"%s",S->VM);
        else
          snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],S->VM);
        }

      break;

    case mjsaWCLimit:

      if (S->WCLimit > 0)
        {
        if (S->WCLimit < MMAX_TIME)
          {
          if (!WikiFormat)
            snprintf(Buf,BufSize,"%d",S->WCLimit);
          else
            snprintf(Buf,BufSize,"%s=%d",MJobSysAttr[AIndex],S->WCLimit);
          }
        else
          {
          /* Infinite walltime */
          if (!WikiFormat)
            snprintf(Buf,BufSize,"%s","INFINITY");
          else
            snprintf(Buf,BufSize,"%s=%s",MJobSysAttr[AIndex],"INFINITY");
          }
        }

      break;

    default:

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MSysJobAToString() */

/**
 * Converts a job to a WIKI string.
 *
 * @param J      (I) - the job to translate
 * @param S      (I) - jobs system struct
 * @param String (O) - output mstring_t -> Will clear anything that is currently in the string
 *
 *  NOTE: 
 *    1) The String MUST BE INITALIZED prior to calling this function.
 *    2) This function does not add a new line at the end so it
 *    can be concatenated with another string, like the job.
 *    3) There is no parameter error checking
 */

int MSysJobToWikiString(

  mjob_t    *J,       /* I */
  msysjob_t *S,       /* I */
  mstring_t *String)  /* O */

  {
  int index;

  if ((S == NULL) || (String == NULL))
    return(FAILURE);

  String->clear();   /* reset string */

  for(index=0;index < mjsaLAST;index++)
    {
    char Buf[MMAX_LINE];

    if ((index == mjsaJobType) &&
        (MVMJobIsVMMigrate(J)))
      {
      snprintf(Buf,MMAX_LINE,"%s=vmmigrate",MJobSysAttr[index]);
      }
    else
      {
      MSysJobAToString(S,(enum MJobSysAttrEnum)index,Buf,MMAX_LINE,mfmNONE,TRUE);
      }

    if (Buf[0] != '\0')
      MStringAppendF(String,"%s ",Buf);
    }

  return(SUCCESS);
  } /* END MSysJobToWikiString() */



int MSysJobProcessFailure(

  mjob_t *J)

  {
  const char *FName = "MSysJobProcessFailure";

  MDB(5,fCONFIG) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if ((J == NULL) || (J->System == NULL))
    {
    return(FAILURE);
    }

  MDB(3,fCKPT) MLog("ALERT:    system job '%s' failed\n",
    J->Name);

  switch (J->System->JobType)
    {
    case msjtOSProvision:

      {
      int nindex;
      int index;

      mnl_t NL = {0};

      mln_t *tmpL;

      mjob_t *tmpJ;

      mnode_t *N;

      enum MNodeStateEnum State;

      tmpL = J->System->Dependents;

      while (tmpL != NULL)
        {
        if (MJobFind(tmpL->Name,&tmpJ,mjsmBasic) == SUCCESS)
          {
          MJobRemoveDependency(tmpJ,mdJobSuccessfulCompletion,J->Name,NULL,0);

          MJGroupFree(tmpJ);

          /* add a flag on the system job that will prevent the dependent job(s)
           * from being started */

          bmset(&J->System->EFlags,msjefProvFail);

          /* add message to job about provisioning job failure */

          MDB(3,fCKPT) MLog("INFO:     found job '%s' dependent on system job '%s', removing dependency\n",
            tmpJ->Name,
            J->Name);
          }

        tmpL = tmpL->Next;
        }

      index = 0;

      MNLInit(&NL);

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        State = mnsNONE;

        if ((N->ActiveOS == J->System->ICPVar1) &&
            (MNodeGetRMState(N,mrmrtCompute,&State) == SUCCESS) &&
            (MNSISUP(State)))
          {
          continue;
          }

        MNLSetNodeAtIndex(&NL,index,N);
        MNLSetTCAtIndex(&NL,index,MNLGetTCAtIndex(&J->NodeList,nindex));

        index++;
        }  /* END for (nindex) */

      MNLTerminateAtIndex(&NL,index);

      MDB(3,fCKPT)
        {
        int nindex;

        MLog("INFO:     following nodes failed to provision for system job '%s':",
          J->Name);

        for (nindex = 0;nindex < index;nindex++)
          {
          MLog("%s,",MNLGetNodeName(&NL,nindex,NULL));
          }

        MLog("\n");
        }

      /* only set a reservation if the node actually failed to provision,
         not if the RM merely failed to ask it */

      if ((!bmisset(&J->System->EFlags,msjefProvRMFail)) &&
          (MNLGetNodeAtIndex(&NL,0,&N) == SUCCESS))
        {
        MNLReserve(
          &NL,
          MSched.NodeFailureReserveTime,
          "prov-fail",
          (N->RM != NULL) ? N->RM->NodeFailureRsvProfile : NULL,
          "Provisioning failure detected");
        }

       /* attach error messages to alert sysadmin to problems with RM communication */

       if (bmisset(&J->System->EFlags,msjefProvRMFail))
         {
         char tmpLine[MMAX_LINE];

         snprintf(tmpLine, sizeof(tmpLine), "ALERT:   failure while submitting provisioning job to RM '%s'\n",
           ((MSched.ProvRM != NULL) ? MSched.ProvRM->Name : "unknown" ));

         MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

         MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

         if (MSched.ProvRM != NULL)
           {
           mrm_t* R = MSched.ProvRM;

           MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
           }
         }

      /* remove the job group info so the system job will not start the
         dependent jobs in MJobRemove */

      MJGroupFree(J);

      MNLFree(&NL);
      }  /* END BLOCK (case msjtOSProvision) */

      break;

    case mjcpPowerOn:
    case mjcpPowerOff:

      {
      mnode_t *N;

      int nindex;
      int rindex;
      int index;

      mnl_t NL = {0};

      index = 0;

      MNLInit(&NL);

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        rindex = 0;

        if ((N->RM != NULL) && !bmisset(&N->RM->RTypes,mrmrtProvisioning))
          {
          rindex = N->RM->Index;
          }
        else if ((MPowerGetPowerStateRM(&rindex)) == FAILURE)
          {
          MDB(5,fRM) MLog("WARNING:  no RM can report node '%s' power state for system job '%s'\n",
            N->Name,
            J->Name);
          }

        if ((J->System->JobType == msjtPowerOn) &&
            MNODESTATEISUP(N->RMState[rindex]))
          {
          continue;
          }

        if ((J->System->JobType == msjtPowerOff) &&
            !MNODESTATEISUP(N->RMState[rindex]))
          {
          continue;
          }

        MNLSetNodeAtIndex(&NL,index,N);
        MNLSetTCAtIndex(&NL,index,MNLGetTCAtIndex(&J->NodeList,nindex));

        index++;
        }  /* END for (nindex) */

      MNLTerminateAtIndex(&NL,index);

      MDB(7,fCKPT)
        {
        int nindex;

        MLog("INFO:     following nodes failed to change power state for system job '%s':",
          J->Name);

        for (nindex = 0;nindex < index;nindex++)
          {
          MLog("%s,",MNLGetNodeName(&NL,nindex,NULL));
          }

        MLog("\n");
        }

      if ((!MNLGetNodeAtIndex(&NL,0,&N) == SUCCESS) &&
          !bmisset(&J->System->EFlags,msjefPowerRMFail))
        {
        mjob_t *tmpJ;
   
        mln_t *tmpL;

        MNLReserve(
          &NL,
          MSched.NodeFailureReserveTime,
          "power-fail",
          (N->RM != NULL) ? N->RM->NodeFailureRsvProfile : NULL,
          "Power failure detected");

        bmset(&J->System->EFlags,msjefProvFail);

        tmpL = J->System->Dependents;
   
        while (tmpL != NULL)
          {
          if (MJobFind(tmpL->Name,&tmpJ,mjsmBasic) == SUCCESS)
            {
            MJobRemoveDependency(tmpJ,mdJobSuccessfulCompletion,J->Name,NULL,0);
   
            bmunset(&tmpJ->IFlags,mjifMaster);
   
            MJobReleaseRsv(tmpJ,TRUE,TRUE);
   
            MJGroupFree(tmpJ);
   
            /* add a flag on the system job that will prevent the dependent job(s)
             * from being started */
   
            /* add message to job about provisioning job failure */
   
            MDB(3,fCKPT) MLog("INFO:     found job '%s' dependent on system job '%s', removing dependency\n",
              tmpJ->Name,
              J->Name);
            }
   
          tmpL = tmpL->Next;
          }  /* END while (tmpL != NULL) */
        }    /* END if ((NL[0].N != NULL) && ...) */

       /* attach error messages to alert sysadmin to problems with RM communication */

       if (bmisset(&J->System->EFlags,msjefPowerRMFail))
         {
         char tmpLine[MMAX_LINE];

         snprintf(tmpLine, sizeof(tmpLine), "ALERT:   failure while submitting power state changes to RM '%s'\n",
           ((MSched.ProvRM != NULL) ? MSched.ProvRM->Name : "unknown" ));

         MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
         MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

         if (MSched.ProvRM != NULL)
           {
           mrm_t* R = MSched.ProvRM;

           MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
           }
         }

      MNLFree(&NL);
      }  /* END BLOCK (case msjtPowerOn,msjtPowerOff) */

      break;

#if 0
    case msjtVMCreate:

      {
      mln_t *tmpL;
      mvm_t *VM;

      mjob_t *tmpJ;

      tmpL = J->System->Dependents;

      while (tmpL != NULL)
        {
        if (MJobFind(tmpL->Name,&tmpJ,mjsmBasic) == SUCCESS)
          {
          MJobRemoveDependency(tmpJ,mdJobSuccessfulCompletion,J->Name,NULL,0);

          bmunset(&tmpJ->IFlags,mjifMaster);

          MJobReleaseRsv(tmpJ,TRUE,TRUE);

          MJGroupFree(tmpJ);

          /* add a flag on the system job that will prevent the dependent job(s)
           * from being started */

          bmset(&J->System->EFlags,msjefProvFail);

          /* add message to job about provisioning job failure */

          MDB(3,fCKPT) MLog("INFO:     found job '%s' dependent on system job '%s', removing dependency\n",
            tmpJ->Name,
            J->Name);
          }

        tmpL = tmpL->Next;
        }  /* END while (tmpL != NULL) */

      if (MVMFind(J->System->SCPVar1,&VM) == SUCCESS)
        {
        char tmpLine[MMAX_LINE];
        mjob_t *trackingJ = VM->TrackingJ;

        if (trackingJ != NULL)
          {
          snprintf(tmpLine,sizeof(tmpLine),"vmcreate job '%s' failed",J->Name);

          MJobSetHold(trackingJ,mhBatch,MMAX_TIME,mhrPreReq,tmpLine);

					/* The Create Job has most likely failed; therefore, leaving this only sets up a seg fault later (MOAB-1606) */

          VM->CreateJob = NULL;
          }

#ifdef MNOT
        /* Don't remove the vm as there could be storage attached.  */

        if (MVMDestroy(&VM,NULL) == FAILURE)
          {
          MDB(3,fSCHED) MLog("ERROR:    failed to remove vm%s after vmcreate job failed.\n",
              (trackingJ != NULL) ? " on tracking job" : "");
          }
#endif
        }
      }    /* END BLOCK (case msjtVMCreate) */
#endif

      break;

    default:

      /* NYI */

      break;
    }   /* END switch (JobType) */

  if (J->CompletionCode == 0)
    J->CompletionCode = -1;

  MOReportEvent((void *)J,J->Name,mxoJob,mttFailure,0,TRUE);
  MSchedCheckTriggers(J->Triggers,-1,NULL);

  MSysJobProcessCompleted(J);

  return(SUCCESS);
  }  /* END MSysJobProcessFailure() */



/**
 * Return the trigger associated with the generic system job.
 *
 * @param J
 * @param T
 */

int MSysJobGetGenericTrigger(

  mjob_t   *J,
  mtrig_t **T)

  {
  if (T != NULL)
    *T = NULL;

  if ((J == NULL) || (T == NULL))
    {
    return(FAILURE);
    }

  /* use System->TList before J->Triggers */

  if ((J->System->TList != NULL) && (J->System->TList->NumItems > 0))
    {
    *T = (mtrig_t *)MUArrayListGetPtr(J->System->TList,0);

    return(SUCCESS);
    }

  if ((J->Triggers != NULL) && (J->Triggers > 0))
    {
    *T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,0);

    return(SUCCESS);
    }
 
  return(FAILURE);
  }  /* END MSysJobGetGenericTrigger() */



/**
 * Called when a system job is completed.
 *
 * @see MSysJobProcessFailure() - parent
 * @see MSysJobUpdate() - parent
 */

int MSysJobProcessCompleted(

  mjob_t *J)

  {
  const char *FName = "MSysJobProcessCompleted";

  MDB(5,fCONFIG) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  MDB(7,fCKPT) MLog("ALERT:    system job '%s' completed\n",
    J->Name);

  if ((J == NULL) || (J->System == NULL))
    {
    return(FAILURE);
    }

  if (MJOBISCOMPLETE(J))
    return(SUCCESS);  /* prevent job from being processed again */

  switch (J->System->CompletionPolicy)
    {
    case mjcpGeneric:

      {
      mtrig_t *T;

      /* Job gets the completion code of its trigger */

      if (MSysJobGetGenericTrigger(J,&T) == FAILURE)
        {
        /* SHOULD NOT GET HERE */
        /* A generic system job should have one trigger */

        J->CompletionCode = MDEF_BADCOMPLETIONCODE;

        break;
        }

      if (J->CompletionCode == 0)
        {
        /* only set completion code if not already set */
 
        if ((T->ExitCode != 0) || (T->State != mtsFailure))
          J->CompletionCode = T->ExitCode;
        else
          J->CompletionCode = 1;
        }

      J->CompletionTime = MSched.Time;

      if ((J->System->VM != NULL) &&
          (J->System->VMSourceNode != NULL) &&
          (J->System->VMDestinationNode != NULL))
        {
        /* This job is doing a VM migration */
        mnode_t *DestNode;
        mnode_t *SrcNode;
        mvm_t *V;

        if ((MNodeFind(J->System->VMDestinationNode,&DestNode) == SUCCESS))
          {
          MURemoveAction(&DestNode->Action,J->Name);
          }

        if ((MNodeFind(J->System->VMSourceNode,&SrcNode) == SUCCESS))
          {
          MURemoveAction(&SrcNode->Action,J->Name);
          }

        if (MUHTGet(&MSched.VMTable,J->System->VM,(void **)&V,NULL) == SUCCESS)
          {
          MURemoveAction(&V->Action,J->Name);
          }
        else if (J->CompletionCode == 0)
          {
          /* Couldn't find VM - error */

          J->CompletionCode = -1;

          MMBAdd(
            &J->MessageBuffer,
            "job migration trigger returned successfully, but VM was not found",
            "N/A",
            mmbtOther,
            0,
            1,
            NULL);
          }

        /* Check to see if the VM actually migrated */

        if ((J->CompletionCode == 0) &&
            (strcmp(V->N->Name,J->System->VMDestinationNode)))
          {
          /* VM is not on the correct node! */

          J->CompletionCode = -1;

          /* Put a message on the job */

          MMBAdd(
            &J->MessageBuffer,
            "job migration trigger returned successfully, but VM was not migrated",
            "N/A",
            mmbtOther,
            0,
            1,
            NULL);
          }

        if (MSched.PushEventsToWebService == TRUE)
          {
          MEventLog *Log = new MEventLog(meltVMMigrateEnd);
          Log->SetPrimaryObject(V->VMID,mxoxVM,(void *)V);
          Log->AddDetail("Source",J->System->VMSourceNode);
          Log->AddDetail("Destination",J->System->VMDestinationNode);
          Log->AddRelatedObject(J->Name,mxoJob,(void *)J);

          if (J->CompletionCode != 0)
            {
            Log->SetStatus("failure");
            }

          MEventLogExportToWebServices(Log);

          delete Log;
          }

        }  /* END if ((J->VM != NULL) && ...) */
      }  /* END case mjcpGeneric */

      break;

    case mjcpOSProvision:
    case mjcpOSProvision2:

      /* NO-OP */

      break;

    case mjcpPowerOff:
    case mjcpPowerOn:

      {
      mnode_t *tmpN;

      if (MNLGetNodeAtIndex(&J->NodeList,0,&tmpN) == SUCCESS)
        {
        MURemoveAction(&tmpN->Action,J->Name);
        }
      }

      break;

    case mjcpVMMigrate:

      {
      mnode_t *DestNode;
      mnode_t *SrcNode;
      mvm_t *V;

      if ((MNodeFind(J->System->VMDestinationNode,&DestNode) == SUCCESS))
        {
        MURemoveAction(&DestNode->Action,J->Name);
        }

      if ((MNodeFind(J->System->VMSourceNode,&SrcNode) == SUCCESS))
        {
        MURemoveAction(&SrcNode->Action,J->Name);
        }

      if (MUHTGet(&MSched.VMTable,J->System->VM,(void **)&V,NULL) == SUCCESS)
        {
        MURemoveAction(&V->Action,J->Name);
        }
      }    /* END BLOCK (case mjcpVMMigrate) */

      break;

    default:

      {
      mvm_t *V;

      if (MUHTGet(&MSched.VMTable,J->System->VM,(void **)&V,NULL) == SUCCESS)
        {
        MURemoveAction(&V->Action,J->Name);
        }
      }

      break;
    }  /* END switch(J->System->CompletionPolicy) */

  MJobSetState(J,mjsCompleted);

  return(SUCCESS);
  }  /* END MSysJobProcessCompleted() */




/**
  * @param J (I)
 */

enum MSystemJobTypeEnum MJobGetSystemType(

  mjob_t const *J)

  {
  if (J->System != NULL)
    {
    return(J->System->JobType);
    }

  return(msjtNONE);
  } /* END MJobGetSystemType() */





/**
 * Report system job details as pending action XML.
 *
 * @see MJobShowSystemAttrs() - peer
 * @see MJobSystemToXML() - peer
 * @see MSysQueryPendingActionToXML() - parent
 *
 * <paction maxduration="120" hostlist="hv01" motivation="XXX" parent="job:13443" requester="steve" starttime="1246391032" state="Running" pactionid="mnodectl-22" type="osprovision"><osprovision targetos="linux"></osprovision><failuredetails></failuredetails></paction>
 */

int MSysJobToPendingActionXML(

  mjob_t  *J,   /* I */
  mxml_t **JEP) /* O (alloc) */

  {
  mxml_t    *JE;
  mxml_t    *CE;
  msysjob_t *S;

  char   *FailureDetails = NULL;
  mmb_t  *MB;

  if ((J == NULL) || (J->System == NULL) || (JEP == NULL))
    {
    return(FAILURE);
    }

  if (MXMLCreateE(JEP,(char *)MXO[mxoxPendingAction]) == FAILURE)
    {
    return(FAILURE);
    }

  JE = *JEP;

  S = J->System;

  /* report standard pending action attributes */

  /* NOTE:  consider changing pactionid from jobid to global pending action table record */

  MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaPendingActionID],(void *)J->Name,mdfString);
  MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaType],(void *)MSysJobType[S->JobType],mdfString);

  /* add scheduler name to pending actions */

  MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaSched],(void *)MSched.Name,mdfString);

  if (J->StartTime > 0)
    {
    MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaStartTime],(void *)&J->StartTime,mdfLong);
    }
  else 
    {
    /* NO-OP */

    /* Don't report StartTime if we don't have one.  Viewpoint wants this to 
        not be reported if there is no starttime */
    }

  MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaMaxDuration],(void *)&J->WCLimit,mdfLong);

  if (J->System != NULL && J->System->ProxyUser != NULL)
    {
    MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaRequester],(void *)J->System->ProxyUser,mdfString);
    }
  else if (J->Credential.U != NULL)
    {
    MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaRequester],(void *)&J->Credential.U->Name,mdfString);
    }

  /* host list will be an optional field in future pending actions */

  mstring_t tmpString(MMAX_LINE);

  if (!MNLIsEmpty(&J->NodeList))
    {
    MNLToMString(&J->NodeList,FALSE,",",'\0',1,&tmpString);
    
    if (!tmpString.empty())
      {
      MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaHostlist],(void *)tmpString.c_str(),mdfString);
      }
    }
  else
    {
    if (!MNLIsEmpty(&J->ReqHList))
      {
      mstring_t List(MMAX_LINE);
  
      MNLToMString(&J->ReqHList,FALSE,",",'\0',-1,&List);
  
      if (!MUStrIsEmpty(List.c_str()))
        MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaHostlist],(void *)List.c_str(),mdfString);
      }   
    }

  /* report state */
  /* NOTE:  expand state info */

  if (bmisclear(&J->Hold))
    MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaState],(void*)MJobState[J->State],mdfString);
  else
    {
    int tmpState = mjsHold;
  
    if (bmisset(&J->Hold,mhBatch))
      tmpState = mjsBatchHold;
    else if (bmisset(&J->Hold,mhSystem))
      tmpState = mjsSystemHold;
    else if (bmisset(&J->Hold,mhUser))
      tmpState = mjsUserHold;
    else if (bmisset(&J->Hold,mhDefer))
      tmpState = mjsDeferred;

    MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaState],(void*)MJobState[tmpState],mdfString);
    }

#if 0
  if ((J->System == NULL) || (J->System->JobType != msjtVMCreate))
#endif
  if (J->System == NULL)
    {
    mnode_t *tmpN;

    if (MNLGetNodeAtIndex(&J->NodeList,0,&tmpN) == SUCCESS)
      {
      if (tmpN->LastSubState != NULL)
        {
        MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaSubState],(void *)tmpN->LastSubState,mdfString);
        }
      }
    }
#if 0
  /* This is only for VMCreate jobs (removed) */
  else if (J->System->VM != NULL) 
    {
    /* vm create specific paction substate stuff */
    mvm_t *TmpVM = NULL;

    if (MVMFind(J->System->VM,&TmpVM) == SUCCESS)
      {
      mgevent_desc_t *GEventDesc = NULL;
      
      if ((MGEventGetDescInfo("provaction",&GEventDesc) == SUCCESS) && (GEventDesc->GEventActionDetails[0] != NULL)) 
        {
        MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaSubState],(void *)GEventDesc->GEventActionDetails[0],mdfString);
        }
      else if (TmpVM->LastSubState != NULL)
        {
        MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaSubState],(void *)TmpVM->LastSubState,mdfString);
        }
      } 
    }
#endif

  /* report motivation/human readable initiator */

  if (S->RecordEventInfo != NULL)
    {
    /* NOTE:  must convert to XML safe format! */

    MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaMotivation],(void *)S->RecordEventInfo,mdfString);
    }

  /* report context-sensitive info */

  switch (S->JobType)
    {
    case msjtOSProvision:  /**< reprovision OS */

      MXMLAddChild(JE,MSysJobType[S->JobType],NULL,&CE);
      MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaTargetOS],(void *)MAList[meOpsys][S->ICPVar1],mdfString);

      break;

    case msjtOSProvision2: /**< perform 2-phase (base+VM) OS reprovision */

      break;

    case msjtPowerOff:     /**< power off node */
    case msjtPowerOn:      /**< power on node */
    case msjtReset:        /**< reboot node */

      MXMLAddChild(JE,MSysJobType[S->JobType],NULL,&CE);

      break;

#if 0
    case msjtVMStorage:    /**< create storage mounts for VMs */
    case msjtVMCreate:     /**< create virtual machine */
    case msjtVMDestroy:    /**< destroy existing virtual machine */

      /* report VM description */

      MXMLAddChild(JE,MSysJobType[S->JobType],NULL,&CE);
      MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaVMID],(void *)S->VM,mdfString);

      break;
#endif

    case msjtVMMigrate:    /**< migrate virtual machine */

      MXMLAddChild(JE,MSysJobType[S->JobType],NULL,&CE);
      MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaVMID],(void *)S->VM,mdfString);

      if (S->VMSourceNode != NULL)
        MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaMigrationSource],(void *)S->VMSourceNode,mdfString);

      if (S->VMDestinationNode != NULL)
        MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaMigrationDest],(void *)S->VMDestinationNode,mdfString);

      break;

    case msjtGeneric:

      if (MVMJobIsVMMigrate(J))
        {
        MXMLAddChild(JE,MSysJobType[msjtVMMigrate],NULL,&CE);
        MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaVMID],(void *)S->VM,mdfString);

        if (S->VMSourceNode != NULL)
          MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaMigrationSource],(void *)S->VMSourceNode,mdfString);

        if (S->VMDestinationNode != NULL)
          MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaMigrationDest],(void *)S->VMDestinationNode,mdfString);
        }

      mhash_t VCVars;

      /* report variables, but only for 'generic' system jobs (to help speciate the system job) */

      if (MJobAToMString(J,mjaVariables,&tmpString) == SUCCESS)
        {
        if (!tmpString.empty())
          {
          MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaVariables],(void*)tmpString.c_str(),mdfString);
          }
        }

      /* Get VC variables */

      memset(&VCVars,0x0,sizeof(mhash_t));
      MVCGrabVarsInHierarchy(J->ParentVCs,&VCVars,TRUE);

      if (MUHTToMString(&VCVars,&tmpString) == SUCCESS)
        {
        if (!tmpString.empty())
          {
          MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaVCVariables],(void*)tmpString.c_str(),mdfString);
          }
        }

      MUHTFree(&VCVars,TRUE,MUFREE);

      /* report description/reason for 'generic' system jobs */
      if (MJobAToMString(J,mjaDescription,&tmpString) == SUCCESS)
        {
        if (!tmpString.empty())
          {
          MXMLSetAttr(JE,(char *)MPendingActionAttr[mpaaDescription],(void*)tmpString.c_str(),mdfString);
          }
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (S->JobType) */

  /* gather info about errors and report them */

  for (MB = J->MessageBuffer;MB != NULL;MB = (mmb_t *)MB->Next)
    {
    if (MB->Type == mmbtPendActionError)
      {
      FailureDetails = MB->Data;

      MXMLAddChild(JE,"failuredetails",FailureDetails,NULL);

      break;  /* The agreed upon spec only handles one 'failuredetails'. */
      }
    } /* END for (MB = J->MB; ...) */

  if ((FailureDetails == NULL) &&
      (S->VM != NULL))
    {
    mxml_t *FailureDetailsE;

    /* continue to look for failures attached to VM */

    if (MVMFailuresToXML(NULL,S->VM,&FailureDetailsE) == SUCCESS)
      MXMLAddE(JE,FailureDetailsE);
    }

  return(SUCCESS);
  }  /* END MSysJobToPendingActionXML() */



/**
 * Hooks up a VMTracking job with it's expected VM.
 *
 * Uses the variable "VMID" to know which VM to use.  The VM
 *  must already have been reported to Moab.
 *
 * @param J        [I] (modified) - The VMTracking job to attach to a VM
 * @param StartJob [I] - reports if job was successfully started
 */

int MJobLinkUpVMTrackingJob(

  mjob_t  *J,
  mbool_t *StartJob)

  {
  char VMID[MMAX_NAME];
  char *VMIDPtr = NULL;

  mvm_t *tmpVM = NULL;
  mbool_t ErrorFound = FALSE;

  if ((J == NULL) ||
      (!bmisset(&J->Flags,mjfVMTracking)) ||
      (J->System == NULL) ||
      (J->System->JobType != msjtVMTracking))
    {
    /* Job is NULL or is not a VM tracking job */

    return(FAILURE);
    }

  /* The VM should have already been set up and be reported by the RM */

  if (MUHTGet(&J->Variables,"VMID",NULL,NULL) == FAILURE)
    {
    mln_t *tmpVCL;
    mvc_t *tmpVC;
    char  *VarValue = NULL;

    if (J->ParentVCs == NULL)
      {
      ErrorFound = TRUE;
      }

    for (tmpVCL = J->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
      {
      tmpVC = (mvc_t *)tmpVCL->Ptr;

      if (tmpVC == NULL)
        continue;

      if (!bmisset(&tmpVC->Flags,mvcfWorkflow))
        continue;

      if (MUHTGet(&tmpVC->Variables,"VMID",(void **)&VarValue,NULL) == SUCCESS)
        {
        mstring_t tmpS(MMAX_LINE);

        MStringSetF(&tmpS,"VMID=%s",VarValue);
        MJobSetAttr(J,mjaVariables,(void **)tmpS.c_str(),mdfString,mAdd);

        break;
        }
      }  /* END for (tmpVCL) */
    }  /* END if (MUHTGet(J->Variables,"VMID") == FAILURE) */

  if (MUHTGet(&J->Variables,"VMID",(void **)&VMIDPtr,NULL) == FAILURE)
    {
    /* variable might exist, but no value */

    ErrorFound = TRUE;
    }

  if (ErrorFound == FALSE)
    {
    MUStrCpy(VMID,VMIDPtr,sizeof(VMID));

    if (MVMFind(VMID,&tmpVM) == FAILURE)
      {
      ErrorFound = TRUE;
      }
    }

  if (ErrorFound == TRUE)
    {
    char ErrMsg[MMAX_LINE];

    snprintf(ErrMsg,sizeof(ErrMsg),"ERROR:    could not link up VMTracking job '%s' with VM, placing hold\n",
      J->Name);

    MDB(3,fSTRUCT) MLog(ErrMsg);

    MJobSetHold(J,mhSystem,MMAX_TIME,mhrAdmin,ErrMsg);

    return(FAILURE);
    }

  /* Create VMCR, set VMID so that MSysJobUpdate can be used normally */

  if (J->VMCreateRequest == NULL)
    {
    J->VMCreateRequest = (mvm_req_create_t *)MUCalloc(1,sizeof(mvm_req_create_t));
    }

  MUStrCpy(J->VMCreateRequest->VMID,VMID,sizeof(J->VMCreateRequest->VMID));
  MUStrDup(&J->System->VM,VMID);

  /* Attach VMTracking job to VM */

  tmpVM->TrackingJ = J;

  /* Add any features on tracking job as a VMFEATURE variable */

  if (J->Req[0] != NULL)
    {
    char  tmpLine[MMAX_LINE];

    MUNodeFeaturesToString(',',&J->Req[0]->ReqFBM,tmpLine);

    MUHTAdd(&tmpVM->Variables,"VMFEATURES",(void *)strdup(tmpLine),NULL,MUFREE);
    }

  /* Write event that the VM is ready */

  CreateAndSendEventLog(meltVMReady,tmpVM->VMID,mxoxVM,(void *)tmpVM);

  if (StartJob != NULL)
    *StartJob = TRUE;

  return(SUCCESS);
  }  /* END MJobLinkUpVMTrackingJob() */



/**
 * Check requirements for adaptive job and carry out steps.
 *
 * @see MJobStart() - parent
 *
 * @param J (I)
 * @param StartJob (I) - reports if job was successfully started
 * @param Msg (O) - report reason start failed (optional,minsize=MMAX_LINE)
 */

int MAdaptiveJobStart(

  mjob_t   *J,        /* I */
  mbool_t  *StartJob, /* I */
  char     *Msg)      /* O */

  {
  char    *EMsg;

  mbitmap_t JStartBM;

  mjob_t *SubJob;

  int SubJobSubmitRC = SUCCESS;

  marray_t JobArray;
  mjob_t **JList;

  int      ReqOS = 0;       /* OS required by job */
  int      ReqHVOS = 0;     /* OS required by VM */
  int      ProvOS;          /* OS that should be provisioned on server */
 
  char     tmpMsg[MMAX_LINE];

  int      SC;

  enum MSystemJobTypeEnum PType;

  mbool_t JobWantsVMOS;

  MReqGetReqOpsys(J,&ReqOS);

  if (StartJob != NULL)
    *StartJob = FALSE;

  if (Msg != NULL)
    EMsg = Msg;
  else
    EMsg = tmpMsg;

  EMsg[0] = '\0';

  JobWantsVMOS = MJOBISVMCREATEJOB(J) || MJOBREQUIRESVMS(J);

  if ((bmisset(&J->Flags,mjfVMTracking)) &&
      (J->System != NULL) &&
      (J->System->JobType == msjtVMTracking))
    {
    /* Job is VMTracking job from job template, NOT mvmctl */

    return(MJobLinkUpVMTrackingJob(J,StartJob));
    }  /* END if ((bmisset(&J->Flags,mjfVMTracking)) .. ) */

  if ((J->VMUsagePolicy == mvmupPMOnly) && (ReqOS == 0))
    {
    char *OSName;
    mhashiter_t Iter;

    MUHTIterInit(&Iter);

    if (MUHTIterate(&MSched.OSList,&OSName,NULL,NULL,&Iter) == FAILURE)
      {
      snprintf(EMsg,MMAX_LINE,"physical machine job contains req with no specified OS");

      return(FAILURE);
      }

    ReqOS = MUMAGetIndex(meOpsys,OSName,mAdd);
    J->Req[0]->Opsys = ReqOS;
    }
  else if (J->VMUsagePolicy == mvmupVMCreate)
    {
    /* vmcreate first pass - generate/submit 'vmcreate' job */
#if 0
    return (MVMCreateVMFromJob(J,EMsg));
#endif
    return(FAILURE);
    }  /* END if ((J->VMUsagePolicy == mvmupVMCreate) && ...) */

  if ((ReqHVOS != 0) &&
      (MUNLNeedsOSProvision(ReqHVOS,FALSE,&J->NodeList,&PType)))
    {
    ProvOS = ReqHVOS;
    }
  else if (bmisset(&J->IFlags,mjifMaster) &&
          (J->System == NULL) &&
          (J->Credential.Q != NULL) &&
          (bmisset(&J->Credential.Q->Flags,mqfProvision)) &&
          (MUNLNeedsOSProvision(ReqOS,JobWantsVMOS,&J->NodeList,&PType)))
    {
    ProvOS = ReqOS;
    }
  else
    {
    ProvOS = 0;
    }

  if (ProvOS != 0)
    {
    /* create one system job for all nodes that need to be provisioned */

    MUArrayListCreate(&JobArray,sizeof(mjob_t *),4);

    if (MSched.DisableVMDecisions == TRUE)
      {
      mnode_t *tmpN;

      /* Write to the event file for each node in the node list */

      int tmpNIndex;

      for (tmpNIndex = 0;MNLGetNodeAtIndex(&J->NodeList,tmpNIndex,&tmpN) == SUCCESS;tmpNIndex++)
        {
        char tmpEvent[MMAX_LINE];

        snprintf(tmpEvent,sizeof(tmpEvent),"Intended: provision node %s to operating system '%s'",
          tmpN->Name,
          MAList[meOpsys][ProvOS]);

        MOWriteEvent((void *)tmpN,mxoNode,mrelNodeProvision,tmpEvent,NULL,NULL);
        }
      }

    /* determine target operating system */

    MUGenerateSystemJobs(
      (J->Credential.U != NULL) ? J->Credential.U->Name : NULL,
      J,
      &J->NodeList,
      PType,
      "provision",
      ProvOS,
      NULL,
      NULL,
      TRUE,
      FALSE,
      NULL,
      &JobArray);

    JList = (mjob_t **)JobArray.Array;

    if ((JList == NULL) || (JList[0] == NULL))
      {
      strcpy(EMsg,"cannot create provisioning job");

      MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

      return(FAILURE);
      }

    SubJob = JList[0];

    MJobTransferAllocation(SubJob,J);

    MULLAdd(&SubJob->System->Dependents,J->Name,NULL,NULL,NULL);

    /* Apply templates for learning */

    MJobSetDefaults(SubJob,TRUE);

    /* swap out J and J->JGroup->SubJobs */

    MJobSetDependency(J,mdJobSuccessfulCompletion,SubJob->Name,NULL,0,FALSE,NULL);

    bmset(&J->IFlags,mjifHasInternalDependencies);

    /* create adjacent reservation */

    if ((SubJobSubmitRC = MJobStart(SubJob,NULL,&SC,EMsg)) == SUCCESS)
      {
      bmset(&J->IFlags,mjifReqTimeLock);

      MRsvJCreate(
        J,
        NULL,
        MSched.Time + SubJob->SpecWCLimit[0],  /* start time */
        mjrTimeLock,
        NULL,
        FALSE);
      }

    MUArrayListFree(&JobArray);

    return(SubJobSubmitRC);
    }  /* END if (bmisset(&J->IFlags,mjifMaster) && ...) */

  if (StartJob != NULL)
    *StartJob = TRUE;

  return(SUCCESS);
  }  /* END MAdaptiveJobStart() */




/**
 * Determine node access policy for adaptive jobs.
 *
 * @see MJobGetSNRange() - parent
 *
 * NOTE: NAccessPolicy is NOT initialized as we don't want to destroy
 *       information from MJobGetSNRange() 
 *
 * @param N (I)
 * @param J (I)
 * @param NAccessPolicy (O)
 */

int MNodeGetAdaptiveAccessPolicy(

  const mnode_t *N,
  const mjob_t  *J,
  enum MNodeAccessPolicyEnum *NAccessPolicy)

  {
  enum MSystemJobTypeEnum ProvType;

  if ((N == NULL) || (J == NULL) || (NAccessPolicy == NULL))
    {
    return(FAILURE);
    }

  if ((MJOBREQUIRESVMS(J) == TRUE) ||
      (MJOBISVMCREATEJOB(J) == TRUE))
    {
    if ((MNodeCanProvision(N,J,J->Req[0]->Opsys,&ProvType,NULL) == TRUE) &&
       ((ProvType == msjtOSProvision) || (ProvType == msjtOSProvision2)))
      {
      MDBO(8,J,fSCHED) MLog("INFO:     node '%s' requires provisioning for job '%s' (SINGLEJOB)\n",
        N->Name,
        J->Name);

      /* server level provisioning is required - resources cannot be shared 
         with workload currently using OS */

      *NAccessPolicy = mnacSingleJob;
      }

    MDBO(8,J,fSCHED) MLog("INFO:     node '%s' does not require provisioning for vm job '%s'\n",
      N->Name,
      J->Name);

    return(SUCCESS);
    }

  if ((J->Credential.Q != NULL) &&
      (bmisset(&J->Credential.Q->Flags,mqfProvision)))
    {
    mjob_t *SysJ = NULL;

    int ReqOS = 0;

    MReqGetReqOpsys(J,&ReqOS);

    /* job requires physical machine */

    if ((ReqOS != 0) && (N->ActiveOS != ReqOS))
      {
      /* current OS doesn't match, check provisioning status */

      if ((MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,&SysJ) == FAILURE) ||
          (N->NextOS != ReqOS))
        {
        MDBO(8,J,fSCHED) MLog("INFO:     node '%s' requires provisioning for job '%s' (SINGLEJOB)\n",
          N->Name,
          J->Name);

        /* node is not being provisioned or NextOS is not a match
           node must be provisioned for job */

        *NAccessPolicy = mnacSingleJob;

        return(SUCCESS);
        }
      }

    if ((N->ActiveOS == ReqOS) &&
        (MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,&SysJ) == SUCCESS))
      {
      /* there are cases where the node's active os is what the node is being provisioned to
         in the case of scinet where turning on is the same as provisioning the activeos is
         also what the node is being provisioned to.  in that case we need to see of the 
         system job is provisioning to the os we want */

      if ((SysJ->System == NULL) ||
          (SysJ->System->ICPVar1 != ReqOS))
        {
        MDBO(8,J,fSCHED) MLog("INFO:     node '%s' requires provisioning for job '%s' (SINGLEJOB)\n",
          N->Name,
          J->Name);

        /* node is being provisioned away from required OS, 
           node must be provisioned for job in the future */

        *NAccessPolicy = mnacSingleJob;

        return(SUCCESS);
        }
      }

    MDBO(8,J,fSCHED) MLog("INFO:     node '%s' does not require provisioning for job '%s'\n",
      N->Name,
      J->Name);

    return(SUCCESS);
    }

  return(SUCCESS);
  }  /* END MNodeGetAdaptiveNAccessPolicy() */



#if 0
/**
 * Update the flags of a system job.
 *
 * @param J
 */

int MSysJobUpdateRsvFlags(

  mjob_t *J)

  {
  if ((J == NULL) ||
      (J->System == NULL) ||
      (J->Rsv == NULL))
    {
    return(FAILURE);
    } 

  switch (J->System->JobType)
    {
    case msjtVMStorage:

      MRsvSetAttr(J->Rsv,mraFlags,(void *)"novmmigrations",mdfString,mAdd);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (J->System->JobType) */

  return(SUCCESS);
  }  /* END MSysJobUpdateRsvFlags() */
#endif

#if 0
/**
 * report number of VMCreate procs associated with N
 *
 * NOTE:  If ShowJobs == TRUE, report number of jobs, otherwise, report number 
 *        of procs w/in associated VM's to be created
 *
 * @param N        (I)
 * @param ShowJobs (I)
 */

int MNodeGetNumVMCreate(

  const mnode_t *N,
  mbool_t        ShowJobs)

  {
  int Count = 0;

  mjob_t *J;
  mre_t  *RE;
  mrsv_t *R;

  if (N == NULL)
    {
    return(Count);
    }
  
  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    if (RE->Type != mreStart)
      continue;

    R = MREGetRsv(RE);

    if (R->Type != mrtJob)
      continue;

    J = R->J;

    if ((J == NULL) || (J->System == NULL))
      continue;

    if (J->System->JobType != msjtVMCreate)
      continue;

    /* this should add the number of procs dedicated on this node to this job
       to be more accurate NYI */

    if (ShowJobs == TRUE)
      Count++;
    else
      Count += J->TotalProcCount;
    }  /* END for (rindex) */

  return(Count);
  }  /* END MNodeGetNumVMCreate() */
#endif

#if 0
/**
 * This routine returns the number of VM create jobs
 * that are currently running in the system.
 *
 * @param NextTime (I) Returns the soonest time that a 
 * VM create job will possibly finish.
 */

int MJobCountVMCreateJobs(

  mutime *NextTime)
 
  {
  job_iter JTI;

  int Count = 0;
  mjob_t *J = NULL;

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if ((J->System != NULL) &&
        (J->System->JobType == msjtVMCreate) &&
        MJOBISACTIVE(J)) 
      {
      if (NextTime != NULL) 
        *NextTime = MIN(*NextTime,J->RemainingTime);

      ++Count;
      }
    }

  return(Count);
  }  /* END MJobCountVMCreateJobs() */
#endif

#if 0
/**
 * This function will take the given system job and ensure, based on its type,
 * that it could be successfully started considering any throttles that may apply to it.
 * For example, a VM create job has a "VMCreateThrottle" parameter which controls how
 * many can be started at any given time.
 *
 * @param J (I) The system job to check against throttles.
 * @return TRUE is the job does not violate any throttles and could start.
 */

mbool_t MSysJobCheckThrottles(

  mjob_t *J)

  {
  if (J == NULL)
    {
    return(FALSE);
    }

  switch (J->System->CompletionPolicy)
    {
    case mjcpVMCreate:

      /* ensure that we aren't reaching VM create throttle limits */

      if ((MSched.VMCreateThrottle > -1) &&
          (MJobCountVMCreateJobs(NULL) >= MSched.VMCreateThrottle))
        {
        MDB(6,fSCHED) MLog("INFO:    system job '%s' not eligible to start - VM creation throttle (%d) reached\n",
          J->Name,
          MSched.VMCreateThrottle);

        return(FALSE);
        }

      break;

    default:
      break;
    }  /* END switch(J->System->CompletionPolicy) */

  return(TRUE);
  }  /* END MSysJobCheckThrottles() */
#endif

int MSysJobPassDebugTrigInfo(

  mjob_t  *J,
  mtrig_t *T)

  {
  mln_t *tmpL = NULL;
  mvc_t *VC = NULL;
  mjob_t *JG;

  const char *FName = "MSysJobPassDebugTrigInfo";

  MDB(8,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (T != NULL) ? T->TName : "NULL");

  if ((J == NULL) || (T == NULL))
    {
    return(FAILURE);
    }

  mstring_t EMsg(MMAX_LINE);

  MStringSetF(&EMsg,"Trigger %s failed on job %s- STDIN: %s STDOUT: %s STDERR: %s EXITCODE: %d",
    T->TName,
    J->Name,
    T->IBuf,
    T->OBuf,
    T->EBuf,
    T->ExitCode);

   MDB(3,fSTRUCT) MLog("INFO:     adding message '%s' to job %s and VC's\n",
     EMsg.c_str(),
     (J != NULL) ? J->Name : "NULL");

  for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    VC = (mvc_t *)tmpL->Ptr;

    if (VC == NULL)
      continue;

    MMBAdd(
      &VC->MB,
      EMsg.c_str(),
      J->Credential.U->Name,
      mmbtOther,
      MSched.Time + MCONST_MONTHLEN,
      0,
      NULL);

    MVCTransition(VC);
    }  /* END for (tmpL = J->ParentVCs) */

  /* update system job and system job group leader */

  MMBAdd(
    &J->MessageBuffer,
    EMsg.c_str(),
    J->Credential.U->Name,
    mmbtOther,
    MSched.Time + MCONST_MONTHLEN,
    0,
    NULL);

  if ((J->JGroup != NULL) &&
      (MJobFind(J->JGroup->Name,&JG,mjsmExtended) == SUCCESS))
    {
    char tmpMsg[MMAX_LINE];

    snprintf(tmpMsg,sizeof(tmpMsg),"system job dependency '%s' failed with message '%s'",
      J->Name,
      EMsg.c_str());

    MMBAdd(&JG->MessageBuffer,tmpMsg,NULL,mmbtNONE,0,0,NULL);
    }

  if (J->CompletionCode == 0)
    {
    /* trigger has failed = T->State should be checked in MSysJobProcessCompleted() which should
       set J->CompletionCode appropriately...  this code below may not be needed unless race 
       conditions exist */
   
    /* NOTE:  system job dependency handling triggers off of ccode */

    J->CompletionCode = 1;
    }

  return(SUCCESS);
  }  /* END MSysJobPassDebugTrigInfo() */


/**
 * This function sets the default QOS credentials for a system 
 * provision job. 
 *
 * @param J (I) The system job to check against throttles.
 * @return SUCCESS / FAILURE.
 */

int MSysJobAddQOSCredForProvision(

  mjob_t *J)

  {
  mqos_t *Q;

  if ((J != NULL) && (MQOSAdd("autoprov",&Q) == SUCCESS))
    {
    /* provisioning jobs should be able to preempt other jobs and reserve
       needed resources as soon as they are created */
  
/*  Why are does this have to happen?  If the admin wants it to happen she can
    set them herself */

#if 0
    bmset(&Q->Flags,mqfpreemptor);
    bmset(&Q->Flags,mqfReserved);
#endif
    bmset(&Q->Flags,mqfProvision);
  
    J->Credential.Q = Q;
    }
  else
    return(FAILURE);

  return(SUCCESS);
  } /* END MSysJobAddQOSCredForProvision */

/* END MSysJob.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
