/* HEADER */

/**
 * @file MJobProcess.c
 *
 * Contains MJob Process Functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "mcom.h"
#include "MEventLog.h"

/**
 * Process job which has finished unsuccessfully.
 *
 * @see MJobProcessCompleted() - peer
 * @see MJobProcessTerminated() - child - handle actions common to both 
 *        completed and removed jobs
 *
 * @param JP (I) [freed on success]
 */

int MJobProcessRemoved(

  mjob_t **JP)  /* I (freed on success) */

  {
  int      nindex;
  int      rqindex;

  enum MS3CodeDecadeEnum tmpS3C;

  char     Line[MMAX_LINE];

  mnode_t *N;
  mreq_t  *RQ;
  mjob_t  *J; 

  char      Message[MMAX_LINE];

  const char *FName = "MJobProcessRemoved";

  MDB(2,fSTAT) MLog("%s(%s)\n",
    FName,
    ((JP != NULL) && (*JP != NULL))  ? (*JP)->Name : "NULL");

  if ((JP == NULL) || (*JP == NULL))
    {
    return(FAILURE);
    }

  J = *JP;

  /* active/inactive job was terminated without successfully completing */

  MJobProcessTerminated(J,mjsRemoved);

  MJobUpdateFailRate(J);

  if (((unsigned long)(J->CompletionTime - J->StartTime) >= J->WCLimit) ||
       (MAM[0].ChargePolicy == mamcpDebitAllWC) ||
       (MAM[0].ChargePolicy == mamcpDebitAllBlocked) ||
       (MAM[0].ChargePolicy == mamcpDebitAllCPU) ||
       (MAM[0].ChargePolicy == mamcpDebitAllPE))
    {
    if (MAMHandleEnd(&MAM[0],(void *)J,mxoJob,Message,&tmpS3C) == FAILURE)
      {
      MDB(1,fSTAT) MLog("ERROR:    Unable to register job end with accounting manager for job '%s'\n",
        J->Name);

      sprintf(Line,"AMFAILURE:  Unable to register job end with accounting manager for job %s, reason %s (%s)\n",
        J->Name,
        (tmpS3C > ms3cNone && tmpS3C < ms3cLAST && MS3CodeDecade[tmpS3C] != NULL) ? MS3CodeDecade[tmpS3C] : "Unknown Failure",
        Message);

      MSysRegEvent(Line,mactNONE,1);
      }
    }
  else
    {
    if (J->Credential.A != NULL)
      {
      if (MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,NULL) == FAILURE)
        {
        MDB(3,fSCHED) MLog("WARNING:  Unable to register job deletion with accounting manager for job '%s'\n",
          J->Name);
        }
      }
    }

  if (MJOBISCOMPLETE(J) == FALSE)
    {
    if (MJOBISACTIVE(J) == TRUE)
      MJobSetState(J,mjsRemoved);
    else
      MJobSetState(J,mjsVacated);
    }

  if (MPar[0].BFChunkDuration > 0)
    {
    MPar[0].BFChunkBlockTime = MSched.Time + MPar[0].BFChunkDuration;
    }

  /* handle statistics */

  MStatUpdateRejectedJobUsage(J,0);

  /* provide feedback info to user */

  MJobSendFB(J);

  /* create job stat record (only if canceling for first time) */
  
  if (!bmisset(&J->IFlags,mjifWasCanceled))
    {
    MOWriteEvent((void *)J,mxoJob,mrelJobCancel,NULL,MStat.eventfp,NULL);

    CreateAndSendEventLog(meltJobCancel,J->Name,mxoJob,(void *)J);

    if ((bmisset(&J->NotifyBM,mntJobFail)) &&
        (J->Credential.U->EMailAddress != NULL))
      {
      char tmpLine[MMAX_LINE];

      mstring_t Msg(MMAX_LINE);; 

      snprintf(tmpLine,sizeof(tmpLine),"Moab job '%s' was canceled",J->Name);
      
      Msg = tmpLine;

      MStringAppend(&Msg,"\n\n========== output of checkjob ==========\n\n");

      MUICheckJob(J,&Msg,mptHard,NULL,NULL,NULL,NULL,0);
 
      MSysSendMail(J->Credential.U->EMailAddress,NULL,tmpLine,NULL,Msg.c_str());
      }
    }

  /* adjust reservation */

  if (J->Rsv != NULL)
    {
    if (J->ReqRID != NULL)
      {
      /* handled in G2 */

      /* NYI */
      }

    if (bmisset(&J->Rsv->Flags,mrfSingleUse))
      MJobReleaseRsv(J,TRUE,TRUE);
    }  /* END if (J->R != NULL) */

  /* modify expected state of nodes */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      MDB(5,fSTAT) MLog("INFO:     node '%s' released from job\n",
        N->Name);

      /* determine new expected state for node */

      if (MNodeCheckAllocation(N) != SUCCESS)
        {
        N->EState = mnsIdle;
        }
      else
        {
        N->EState = mnsActive;
        }

      if (N->EState == N->State)
        N->SyncDeadLine = MMAX_TIME;
      else
        N->SyncDeadLine = MSched.Time + MSched.NodeSyncDeadline;
      }   /* END for (nindex)  */
    }     /* END for (rqindex) */

  MJobRemoveCP(J);

  /* modify expected job state */

  J->EState = J->State;

  /*MJobTransition(J,FALSE);*/

  MJobCleanupForRemoval(J);

  /* move job from MJobHT to MCJobHT */

  if (MQueueAddCJob(J) == FAILURE)
    MJobRemove(JP);

  return(SUCCESS);
  }  /* END MJobProcessRemoved() */





/**
 * Perform final processing on jobs which successfully complete. (This is not
 * always true--jobs that "fail" due to bad job scripts return a non-zero completion
 * code, but are still processed in this function.)
 *
 * @see MJobProcessRemoved() - peer - handle jobs which exit unsuccessfully
 * @see MQueueAddCJob() - child
 *
 * NOTE:  will handle final statistics, create event records, clean-up
 *        allocation partitions, update accounting manager, etc.
 *
 * @param JP (I) [freed on success, if job is not restarted]
 */

int MJobProcessCompleted(

  mjob_t **JP)  /* I (freed on success, if job is not restarted) */

  {
  int      nindex;
  int      rqindex;

  enum MS3CodeDecadeEnum tmpS3C;

  char     Line[MMAX_LINE];

  mnode_t *N;
  mreq_t  *RQ;

  mjob_t  *J;

  char     Message[MMAX_LINE];

  const char *FName = "MJobProcessCompleted";

  MDB(2,fSTAT) MLog("%s(%s)\n",
    FName,
    ((JP != NULL) && (*JP != NULL)) ? (*JP)->Name : "NULL");

  if ((JP == NULL) || (*JP == NULL))
    {
    return(FAILURE);
    }

  J = *JP;

  if ((J->StartTime > 0) && (J->CompletionTime > J->StartTime))
    {
    long ComputedWallTime = J->CompletionTime - J->StartTime - J->SWallTime;

    /* sanity check walltime */

    if (J->AWallTime != ComputedWallTime)
      {
      MDB(7,fSCHED) MLog("INFO:     job %s setting walltime to %ld from %ld in function %s\n",
        J->Name,
        ComputedWallTime,
        J->AWallTime,
        __FUNCTION__);

      J->AWallTime = ComputedWallTime;
      }

    J->AWallTime = MAX(J->AWallTime,(long)J->Req[0]->RMWTime);
    }

  MJobProcessTerminated(J,mjsCompleted);

  if (bmisset(&J->IFlags,mjifWasCanceled) &&
      bmisset(&J->NotifyBM,mntJobFail) &&
      (J->Credential.U->EMailAddress != NULL))
    {
    char tmpLine[MMAX_LINE];

    mstring_t Msg(MMAX_LINE);

    snprintf(tmpLine,sizeof(tmpLine),"Moab job '%s' was canceled",J->Name);

    Msg = tmpLine;

    MStringAppend(&Msg,"\n\n========== output of checkjob ==========\n\n");

    MUICheckJob(J,&Msg,mptHard,NULL,NULL,NULL,NULL,0);

    MSysSendMail(J->Credential.U->EMailAddress,NULL,tmpLine,NULL,Msg.c_str());
    }

  /* set/incr job variables (usually to be used by triggers) */

  MJobProcessTVariables(J);

  if (MAMHandleEnd(&MAM[0],(void *)J,mxoJob,Message,&tmpS3C) == FAILURE)
    {
    MDB(1,fSTAT) MLog("ERROR:    Unable to register job end with accounting manager for job '%s'\n",
      J->Name);

    sprintf(Line,"AMFAILURE:  Unable to register job end with accounting manager for job %s, reason %s (%s)\n",
      J->Name,
      (tmpS3C > ms3cNone && tmpS3C < ms3cLAST && MS3CodeDecade[tmpS3C] != NULL) ? MS3CodeDecade[tmpS3C] : "Unknown Failure",
      Message);

    MSysRegEvent(Line,mactNONE,1);
    }

  if ((J->TemplateExtensions != NULL) &&
      (J->TemplateExtensions->TJobAction != mtjatNONE) &&
      (J->TemplateExtensions->JobReceivingAction != NULL))
    {
    mjob_t *JobReceivingAction = NULL;

    if (MJobFind(J->TemplateExtensions->JobReceivingAction,&JobReceivingAction,mjsmBasic) == SUCCESS)
      {
      switch (J->TemplateExtensions->TJobAction)
        {
        case mtjatDestroy:

          if ((J->CompletionCode == 0) && (J->CompletionTime > 0))
            {
            char tmpMsg[MMAX_LINE];

            snprintf(tmpMsg,sizeof(tmpMsg),"%s cancelled by destroy action job %s\n",
              JobReceivingAction->Name,
              J->Name);

            bmset(&JobReceivingAction->IFlags,mjifDestroyByDestroyTemplate);
            MJobCancel(JobReceivingAction,tmpMsg,FALSE,NULL,NULL);
            }
          else
            {
            /* Job failed, need to clear the flag on job so that we can resubmit
              the destroy job */

            bmunset(&JobReceivingAction->Flags,mjfDestroyTemplateSubmitted);
            bmunset(&JobReceivingAction->SpecFlags,mjfDestroyTemplateSubmitted);
            }

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (J->TX->TJobAction) */
      }  /* END if (MJobFind(J->TX->JobReceivingAction) == SUCCESS) */
    }  /* END if ((J->TX != NULL) && ...) */

  /* determine if job should be restarted due to policies */
  /* (should other completion events happen before this occurs?) */

  if ((MSched.JobFailRetryCount > 0) &&
      (J->CompletionCode != 0) &&
      !bmisset(&J->IFlags,mjifWasCanceled))
    {
    char EMsg[MMAX_LINE];

    if (J->StartCount < MSched.JobFailRetryCount)
      {
      /* restart/requeue job */
      
      MJobSetState(J,mjsStarting);

      MJobRequeue(J,NULL,NULL,EMsg,NULL);

      return(SUCCESS);
      }
    else if (MSched.JobFailRetryCount > 1)
      {
      /* only requeue the job with a hold if the user has enabled a 
       * JobFailRetryCount > 1 */
      MJobSetState(J,mjsStarting);

      MJobRequeue(J,NULL,NULL,EMsg,NULL);

      /* update Moab's in-memory state */

      MJobSetHold(J,mhUser,MMAX_TIME,mhrPolicyViolation,"restart count violation");

      /* tell RM about change in hold */

      if ((J->DestinationRM != NULL) &&
          (J->DestinationRM->Type == mrmtPBS))
        {
        MRMJobModify(J,"hold",NULL,"user",mSet,NULL,NULL,NULL);
        }
      return(SUCCESS);
      }
    }

  if (MJOBISCOMPLETE(J) == FALSE)
    {
    if (MJOBISACTIVE(J) == TRUE)
      MJobSetState(J,mjsCompleted);
    else if ((MJobIsArrayMaster(J) == TRUE) && (bmisset(&J->IFlags,mjifWasCanceled)))
      MJobSetState(J,mjsRemoved);
    else if (MJobIsArrayMaster(J) == TRUE)
      /* Array master's state is always idle. Will need to handle case where 
       * one sub job fails so that the master fails as well in order fulfill 
       * job array dependencies. */
      MJobSetState(J,mjsCompleted);
    else
      MJobSetState(J,mjsVacated);
    }

#ifdef __NCSA
  if (J->CompletionTime == J->StartTime)
    {
    sprintf(Line,"WARNING:  job '%s' completed with a walltime of 0 seconds\n",
      J->Name);

    MSysRegEvent(Line,mactMail,0,1);
    }
#endif /* __NCSA */

  if (MPar[0].BFChunkDuration > 0)
    {
    MPar[0].BFChunkBlockTime = MSched.Time + MPar[0].BFChunkDuration; 
    }

  if (J->Triggers != NULL)
    {
    if (J->CompletionCode != 0)
      {
      MOReportEvent(
        (void *)J,
        J->Name,
        mxoJob,
        mttFailure,
        (J->CompletionTime > 0) ? J->CompletionTime : MSched.Time,
        TRUE);
      }
    else
      {
      MOReportEvent(
        (void *)J,
        J->Name,
        mxoJob,
        mttEnd,
        (J->CompletionTime > 0) ? J->CompletionTime : MSched.Time,
        TRUE);
      }

    MSchedCheckTriggers(J->Triggers,-1,NULL);
    }

  /* provide feedback info to user */

  MJobSendFB(J);

  /* handle statistics */

  MStatUpdateCompletedJobUsage(J,msmNONE,0);

  /* create job stat record */

  MOWriteEvent((void *)J,mxoJob,mrelJobComplete,NULL,MStat.eventfp,NULL);

  /* modify node expected state and dedicated resources */

  /* NOTE: for mjfRsvMap jobs should restore N->ARes (NYI) */
  /*
        TC = RQ->NodeList[nindex].TC;

        MCResRemove(&N->ARes,&N->CRes,&RQ->DRes,TC,TRUE);
  */

  nindex = 0;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      if (bmisset(&J->Flags,mjfRsvMap))
        MCResAdd(&N->ARes,&N->CRes,&RQ->DRes,MNLGetTCAtIndex(&RQ->NodeList,nindex),TRUE);

      MDB(3,fSTAT) MLog("INFO:     node '%s' released from job %s\n",
        N->Name,
        J->Name);

      /* determine new expected state for node */

      if (MNodeCheckAllocation(N) != SUCCESS)
        {
        N->EState = mnsIdle;
        }
      else
        {
        N->EState = mnsActive;
        }

      if (N->EState == N->State)
        N->SyncDeadLine = MMAX_TIME;
      else
        N->SyncDeadLine = MSched.Time + MSched.NodeSyncDeadline;
      }  /* END for (nindex)  */
    }    /* END for (rqindex) */

  MJobRemoveCP(J);

  /* modify expected job state */

  J->EState = J->State;

  MJobCleanupForRemoval(J);

  /* transition to MCJobHT table */

  if (MQueueAddCJob(J) == FAILURE)
    {
    MJobRemove(JP);
    }

  return(SUCCESS);
  }  /* END MJobProcessCompleted() */


int __MJobProcessVMDestroy(

  mjob_t *J)

  {
  mvm_t *VM = NULL;

  if (J == NULL)
    return(FAILURE);

  if (J->System == NULL)
    return(SUCCESS);

  /* See if this job is a VM destroy job.  If so, find the VM that it is trying
      to destroy */
#if 0
  if (J->System->JobType == msjtVMDestroy)
    {
    MVMFind(J->System->VM,&VM);
    }
  else if (J->System->JobType == msjtGeneric)
#endif
  if (J->System->JobType == msjtGeneric)
    {
    mjob_t *OtherJob = NULL;

    if ((J->TemplateExtensions == NULL) ||
        (J->TemplateExtensions->JobReceivingAction == NULL) ||
        (J->TemplateExtensions->TJobAction != mtjatDestroy))
      {
      /* This job is not a destroy job */

      return(SUCCESS);
      }

    MJobFind(J->TemplateExtensions->JobReceivingAction,&OtherJob,mjsmBasic);

    if ((OtherJob == NULL) ||
        (OtherJob->System == NULL) ||
        (OtherJob->System->VM == NULL))
      {
      /* Other job is not a VMTracking job */

      return(SUCCESS);
      }

    MVMFind(OtherJob->System->VM,&VM);

    /* Clear flag on the other job */

    bmunset(&OtherJob->Flags,mjfDestroyTemplateSubmitted);
    bmunset(&OtherJob->SpecFlags,mjfDestroyTemplateSubmitted);
    }
  else
    {
    return(SUCCESS);
    }

  if (VM == NULL)
    return(FAILURE);

  /* Found the VM, now clear any destroy flags with it */

  bmunset(&VM->Flags,mvmfDestroyed);
  bmunset(&VM->Flags,mvmfDestroyPending);
  bmunset(&VM->Flags,mvmfDestroySubmitted);

  return(SUCCESS);
  }  /* END __MJobProcessVMDestroy() */


/**
 * perform tasks common to all workload regardless of whether or not it completed successfully 
 *
 * @param J (I)
 * @param JState (I) 
 */

int MJobProcessTerminated(

  mjob_t             *J,      /* I (modified) */
  enum MJobStateEnum  JState) /* I */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  MSched.EnvChanged = TRUE;

  if (J->Cost == 0.0)
    MJobGetCost(J,FALSE,NULL);

  if (MSched.PushEventsToWebService == TRUE)
    {
    MEventLog *Log = new MEventLog(meltJobEnd);
    Log->SetCategory(melcEnd);
    Log->SetFacility(melfJob);
    Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
    if (JState == mjsRemoved)
      {
      /* Job was unsuccessful */
      Log->SetStatus("failure");
      }

    MEventLogExportToWebServices(Log);

    delete Log;
    }  /* END if (MSched.PushEventsToWebService == TRUE) */

  MJobUpdateFailRate(J);

  /* update array job information */
  if (bmisset(&J->Flags,mjfArrayJob))
    {
    MJobArrayUpdateJobInfo(J,JState);
    }

  /* Update any before dependencies */
  if (J->Depend != NULL)
    {
    mdepend_t *JobDep;
    mbool_t UpdateDep;

    JobDep = J->Depend;

    while (JobDep != NULL)
      {
      UpdateDep = FALSE;

      switch (JobDep->Type)
        {
        case mdJobBefore:

          /* Taken care of in MJobStart */

          break;

        case mdJobBeforeAny:

          UpdateDep = TRUE;

          break;

        case mdJobBeforeSuccessful:

          if (J->CompletionCode == 0)
            UpdateDep = TRUE;

          break;

        case mdJobBeforeFailed:

          if (J->CompletionCode != 0)
            UpdateDep = TRUE;

          break;

        default:
          break;
        }

      if (UpdateDep == TRUE)
        {
        mjob_t *OtherJ;

        if (MJobFind(JobDep->Value,&OtherJ,mjsmExtended) == FAILURE)
          {
          /* Couldn't find the dependent job */

          MDB(4,fSTAT) MLog("WARNING:  could not find job '%s' to update %s dependency for job '%s'\n",
            JobDep->Value,
            MDependType[JobDep->Type],
            J->Name);
          }
        else
          {
          /* Job Found, update any onCount dependencies */
          mdepend_t *OtherDep;

          OtherDep = OtherJ->Depend;

          while (OtherDep != NULL)
            {
            char *UsedJobs = NULL;

            if (OtherDep->Type != mdJobOnCount)
              {
              OtherDep = OtherDep->NextAnd;
              continue;
              }

            if (OtherDep->DepList == NULL)
              {
              /* ctor the mstring_t */
              OtherDep->DepList = new mstring_t(MMAX_LINE);

              OtherDep->ICount = strtol(OtherDep->Value,NULL,10);

              /* 0 is error in strtol */

              if (OtherDep->ICount == 0)
                {
                OtherDep = OtherDep->NextAnd;
                continue;
                }
              } 

            MUStrDup(&UsedJobs,OtherDep->DepList->c_str());

            if ((UsedJobs != NULL) &&
                (MUStrRemoveFromList(UsedJobs,J->Name,',',FALSE) != FAILURE))
              {
              /* Job dependency was already reported */

              OtherDep = OtherDep->NextAnd;
              continue;
              }

            *OtherDep->DepList += ',';
            *OtherDep->DepList += J->Name;

            OtherDep->ICount--; 

            if (UsedJobs != NULL)
              MUFree(&UsedJobs);

            OtherDep = OtherDep->NextAnd;
            } /* END while (OtherDep != NULL) */
          } /* END else     (MJobFind returned SUCCESS) */
        } /* END if (UpdateDep == TRUE) */

      JobDep = JobDep->NextAnd;
      }
    }

  /* If job is a VM destroy job, removed blocking flags on VM */

  __MJobProcessVMDestroy(J);

  /* Copy any VC variables to the VC.  This is so that pending actions
      will still have access to variables at the service level.  This may
      be removed later once pending actions are fully designed with the
      workflow model */

  MVCGrabVarsInHierarchy(J->ParentVCs,&J->Variables,TRUE);

  if (MSched.EnableHighThroughput == FALSE)
    MJobTransition(J,TRUE,FALSE);

  return(SUCCESS);
  }  /* END MJobProcessTerminated() */




/**
 * This function modifies the given job based on errors that
 * occur during a failed migration.
 *
 * @see MJobStart() - parent
 * @see MJobMigrate() - peer - generates failure being processed
 *
 * @param J (I) The job failed to be migrated. [modified]
 * @param R (I) The RM where the job failed to be migrated.
 * @param EMsg (I/O) [required,minsize=MMAX_LINE]
 * @param SC (I) The status code returned from the failed migration.
 */

int MJobProcessMigrateFailure(
    
  mjob_t               *J,     /* I (modified) */
  mrm_t                *R,     /* I */
  char                 *EMsg,  /* I/O (required,minsize=MMAX_LINE) */
  enum MStatusCodeEnum  SC)    /* I */

  {
  char tmpLine[MMAX_LINE];

  const char *FName = "MJobProcessMigrateFailure";

  MDB(3,fSCHED) MLog("%s(%s,%s,EMsg,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    SC);

  if ((J == NULL) ||
      (R == NULL) ||
      (EMsg == NULL))
    {
    return(FAILURE);
    }

  tmpLine[0] = '\0';

  /* NOTE:  must determine failures resulting from remote rejection vs. local rejection */

  switch (SC)
    {
    case mscNoError:

      {
      mnode_t *N;

      mrm_t *R;

      MNLGetNodeAtIndex(&J->Req[0]->NodeList,0,&N);

      if (N != NULL)
        R = N->RM;

      if (N == NULL)
        {
        sprintf(EMsg,"job %s has no requested node list",
          J->Name);
        }
      else if ((R != NULL) && (R->ND.URL[mrmJobSubmit] == NULL))
        {
        sprintf(EMsg,"no 'submit' method specified within RM %s for node %s",
          R->Name,
          N->Name);
        }
      else
        {
        sprintf(EMsg,"node %s has no RM",
          N->Name);
        }
      }    /* END BLOCK (case mscNoError) */

      break;

    case mscPartial:

      /* partial co-allocation failure detected */

      /* NOTE:  job message added in MJobMigrate() */

      /* NO-OP */

      break;

    default:

      {
      char const *name = (R == NULL) ? "NULL" : R->Name;

      MDB(3,fSCHED) MLog("WARNING:  cannot migrate job %s to RM %s - %s\n",
        J->Name,
        name,
        EMsg);

      snprintf(tmpLine,sizeof(tmpLine),"cannot migrate job to RM %s - %s",
        name,
        EMsg);
      }
   
      break;
    }  /* END switch (SC) */

  if (tmpLine[0] != '\0')
    {
    MMBAdd(      
      &J->MessageBuffer,
      tmpLine,
      NULL,
      mmbtOther,
      MSched.Time + MCONST_DAYLEN,
      0,
      NULL);
    }

  /* fine grained error handling */

  switch (SC)
    {
    case mscNoAuth:
      
      /* violates destination peer RM authorized cred list */
      
    case mscBadParam:

      /* bad parameter passed when migrating job to any peer location */
      
      MJobDisableParAccess(J,&MPar[R->PtIndex]);

      break;

    case mscRemoteFailure:

      if (MSched.RemoteFailTransient == FALSE)
        MJobDisableParAccess(J,&MPar[R->PtIndex]);

      break;

    case mscPartial:

      /* co-alloc'd job has partially failed--for now just destroy all job components (unmigrate) */
      /* DON'T do this for dynamic jobs for now--we aren't sure what will happen if we do */

      MJobDisableParAccess(J,&MPar[R->PtIndex]);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (SC) */

  return(SUCCESS);
  }  /* END MJobProcessMigrateFailure() */




#define MMAX_TEMP_VARS  64

/**
 * This routine increments or sets special variables that may be defined for the given job.
 * For example, if the job has the "ccode" variable set, this routine will find the
 * variable and set ccode equal to the job's completion code.
 *
 * This function will also set/increment relevant variables in the given job's job group.
 *
 * @param J (I) The job whose variables should be incremented/set.
 */

int MJobProcessTVariables(

  mjob_t *J)  /* I */

  {
  mjob_t *MasterJ = NULL;

  mhashiter_t HTI;
  char *VarName;
  char *VarVal;
  mbitmap_t VarBM;

  char NamesToAdd[MMAX_TEMP_VARS][MMAX_LINE];
  char ValuesToAdd[MMAX_TEMP_VARS][MMAX_LINE];

  char NamesToAddMaster[MMAX_TEMP_VARS][MMAX_LINE];
  char ValuesToAddMaster[MMAX_TEMP_VARS][MMAX_LINE];
  mbitmap_t *BMToAddMaster[MMAX_TEMP_VARS];

  int AddIndex;
  int AddIndexMaster;

  int  tmpI;
  char tmpLine[MMAX_LINE];
  char *SetVal = NULL;

  mbool_t DoVarExport = FALSE;
  mbool_t NoGroupExport = FALSE;  /* don't set the variable to export at the group level */

  const char *FName = "MJobProcessTVariables";

  MDB(3,fSCHED) MLog("%s(%s)\n",
    FName,
    J->Name);

  if (MUHTIsEmpty(&J->Variables))
    {
    return(SUCCESS);
    }

  memset(NamesToAdd,0,sizeof(NamesToAdd));
  memset(ValuesToAdd,0,sizeof(ValuesToAdd));

  AddIndex = 0;

  memset(NamesToAddMaster,0,sizeof(NamesToAddMaster));
  memset(ValuesToAddMaster,0,sizeof(ValuesToAddMaster));
  memset(BMToAddMaster,0,sizeof(BMToAddMaster));

  AddIndexMaster = 0;

  MUHTIterInit(&HTI);

  while (MUHTIterate(&J->Variables,&VarName,(void **)&VarVal,&VarBM,&HTI) == SUCCESS)
    {
    DoVarExport = FALSE;
    NoGroupExport = FALSE;

    MDB(7,fSCHED) MLog("INFO:     processing variable '%s'\n",
      VarName);

    SetVal = VarVal;

    if (bmisset(&VarBM,mtvaExport))
      DoVarExport = TRUE;

    if (bmisset(&VarBM,mtvaIncr))
      {
      if (VarVal != NULL)
        {
        tmpI = (int)strtol((char *)VarVal,NULL,10);

        tmpI++;
        }
      else
        {
        tmpI = 1;
        }

      snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

      SetVal = tmpLine;

      if (AddIndex < MMAX_TEMP_VARS)
        {
        MUStrCpy(NamesToAdd[AddIndex],VarName,MMAX_LINE);
        MUStrCpy(ValuesToAdd[AddIndex],SetVal,MMAX_LINE);

        AddIndex++;
        }

      MJobWriteVarStats(J,mSet,VarName,SetVal,NULL);
      }
    else if ((!strcasecmp(VarName,"ccode")) && 
             (MJOBISCOMPLETE(J) == TRUE))
      {
      DoVarExport = TRUE;
      NoGroupExport = TRUE;

      snprintf(tmpLine,sizeof(tmpLine),"%d",
        J->CompletionCode);

      SetVal = tmpLine;

      if (AddIndex < MMAX_TEMP_VARS)
        {
        MUStrCpy(NamesToAdd[AddIndex],VarName,MMAX_LINE);
        MUStrCpy(ValuesToAdd[AddIndex],SetVal,MMAX_LINE);

        AddIndex++;
        }

      MJobWriteVarStats(J,mSet,VarName,SetVal,NULL);

#ifdef MYAHOO
#define MPBS_STDFILE_ERROR_CODE -9
      /* check to see if we have a special completion code
       * that should result in an error message */

      if (J->CompletionCode == MPBS_STDFILE_ERROR_CODE)
        {
        mjob_t *JGroup;
        char tmpLine[MMAX_LINE];
        mstring_t tmpString(MMAX_LINE);

        MJobAToMString(
          J,
          mjaAllocNodeList,
          &tmpString,
          mfmNONE);

        snprintf(tmpLine,sizeof(tmpLine),"job '%s' failed to create stdout/stderr files on '%s'",
          J->Name,
          tmpString.c_str());

        /* there has been an error setting the stdout/stderr file */

        MMBAdd(
          &J->MessageBuffer,
          tmpLine,
          NULL,
          mmbtOther,
          MSched.Time + MCONST_DAYLEN,
          0,
          NULL);

        if (MJobGetGroupJob(J,&JGroup) == SUCCESS)
          {
          MMBAdd(
            &JGroup->MessageBuffer,
            tmpLine,
            NULL,
            mmbtOther,
            MSched.Time + MCONST_DAYLEN,
            0,
            NULL);
          }
        }
#endif /* MYAHOO */
      }
    else if (!strcmp(VarName,"HOSTLIST") &&
             (MJOBISCOMPLETE(J) == TRUE))
      {
      mstring_t tmp(MMAX_LINE);

      if (MJobAToMString(
            J,
            mjaAllocNodeList,
            &tmp) == SUCCESS)
        {
        /* can succeed, but not populate outgoing buffer */

        if (!tmp.empty())
          {
          DoVarExport = TRUE;
          NoGroupExport = TRUE;

          if (AddIndex < MMAX_TEMP_VARS)
            {
            MUStrCpy(NamesToAdd[AddIndex],VarName,MMAX_LINE);
            MUStrCpy(ValuesToAdd[AddIndex],tmp.c_str(),MMAX_LINE);

            AddIndex++;
            }

          MJobWriteVarStats(J,mSet,VarName,tmp.c_str(),NULL);
          }
        }
      }

    if (bmisset(&VarBM,mtvaNoExport))
      {
      /* override our desire to export this variable if it
       * is explicitly stated that this variable should NOT
       * be exported */

      DoVarExport = FALSE;
      }

    /* export variable to group job */

    if ((DoVarExport == TRUE) &&
        (J->JGroup != NULL)) 
      {
      if (MJobFind(J->JGroup->Name,&MasterJ,mjsmExtended) == SUCCESS)
        {
        mbitmap_t GroupBM;
        char *GroupVal;

        if ((bmisset(&VarBM,mtvaExport) && bmisset(&VarBM,mtvaIncr)) &&
            (MUHTGet(&MasterJ->Variables,VarName,(void **)&GroupVal,&GroupBM) == SUCCESS))
          {
          if (GroupVal != NULL)
            {
            tmpI = (int)strtol(GroupVal,NULL,10);

            tmpI++;
            }
          else
            {
            tmpI = 1;
            }

          snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

          SetVal = tmpLine;

          bmset(&GroupBM,mtvaExport);
          }
        else if (NoGroupExport == TRUE)
          {
          /* prevent this variable from exporting up, further
           * out of the group object */

          bmset(&GroupBM,mtvaNoExport);
          }
        else if (DoVarExport == TRUE)
          {
          bmset(&GroupBM,mtvaExport);
          }

        if (AddIndexMaster < MMAX_TEMP_VARS)
          {
          MUStrCpy(NamesToAddMaster[AddIndexMaster],VarName,MMAX_LINE);
          MUStrCpy(ValuesToAddMaster[AddIndexMaster],SetVal,MMAX_LINE);
          BMToAddMaster[AddIndexMaster] = &GroupBM;

          AddIndexMaster++;
          }

        MJobWriteVarStats(MasterJ,mSet,VarName,SetVal,NULL);
        }    /* END if ((MJobFind(J->JGroup) || ... */
      }      /* END if ((J->JGroup != NULL)... */
    }  /* END while (MUHTIterate()) */

  if (!MUStrIsEmpty(NamesToAdd[0]))
    {
    for (AddIndex = 0;AddIndex < MMAX_TEMP_VARS;AddIndex++)
      {
      if (MUStrIsEmpty(NamesToAdd[AddIndex]))
        break;

      MUHTAdd(&J->Variables,NamesToAdd[AddIndex],strdup(ValuesToAdd[AddIndex]),NULL,MUFREE);
      }
    }  /* END if (!MUStrIsEmpty(NamesToAdd[0])) */

  if ((MasterJ != NULL) && !MUStrIsEmpty(NamesToAddMaster[0]))
    {
    for (AddIndexMaster = 0;AddIndexMaster < MMAX_TEMP_VARS;AddIndexMaster++)
      {
      if (MUStrIsEmpty(NamesToAddMaster[AddIndexMaster]))
        break;

      MUHTAdd(&J->Variables,NamesToAddMaster[AddIndexMaster],strdup(ValuesToAddMaster[AddIndexMaster]),BMToAddMaster[AddIndexMaster],MUFREE);
      }
    }

  /* checkpoint jobs to ensure we don't lose newly updated variables in a crash */

  MOCheckpoint(mxoJob,(void *)J,FALSE,NULL);
  MOCheckpoint(mxoJob,(void *)MasterJ,FALSE,NULL);

  return(SUCCESS);
  }  /* END MJobProcessTVariables() */



/* move to heap to avoid valgrind errors */

char  tmpVarValBuf2[MDEF_VARVALBUFCOUNT][MMAX_BUFFER3];

/**
 * Create triggers from trigger templates and applies job set templates.
 *
 * @param J (I)
 * @param EMsg (O) [minsize=MMAX_LINE]
 */

int MJobProcessRestrictedAttrs(

  mjob_t *J,    /* I */
  char   *EMsg) /* O */

  {
  const char *FName = "MJobProcessRestrictedAttrs";

  MDB(5,fSCHED) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : NULL);

  if ((J->Triggers != NULL) && (J->TemplateExtensions == NULL))
    {
    /* triggers can come from other places (ie. CLASSCFG[] JOBTRIGGERS) */

    MOReportEvent((void *)J,J->Name,mxoJob,mttCreate,J->CTime,TRUE);

    return(SUCCESS);
    }
  else if ((J->Triggers != NULL) && (J->TemplateExtensions != NULL))
    {
    MOReportEvent((void *)J,J->Name,mxoJob,mttCreate,J->CTime,TRUE);
    }

  if (J->TemplateExtensions == NULL)
    {
    return(SUCCESS);
    }

  if (J->TemplateExtensions->SpecTrig[0] != NULL)
    {
    char tmpBuf[MMAX_BUFFER];

    int   VarValBufSize;
    int   index;

    mln_t *VarList = NULL;

    for (index = 0;index < MDEF_VARVALBUFCOUNT;index++)
      {
      tmpVarValBuf2[index][0] = '\0';
      }

    if ((J->Credential.Q == NULL) || (!bmisset(&J->Credential.Q->Flags,mqfTrigger)))
      {
      MMBAdd(
        &J->MessageBuffer,
        "trigger specified but not allowed by QOS",
        NULL,
        mmbtOther,
        MSched.Time + MCONST_DAYLEN,
        0,
        NULL);
      }
    else
      {
      int tindex;
      marray_t HashList;

      /* NOTE:  loop through all elements in linked list (NYI) */

      VarValBufSize = MDEF_VARVALBUFCOUNT - 1;

      MJobGetVarList(
        J,
        &VarList,
        tmpVarValBuf2,
        &VarValBufSize);

      MUArrayListCreate(&HashList,sizeof(mhash_t *),4);

      if (J->Variables.Table != NULL)
        {
        MUArrayListAppendPtr(&HashList,&J->Variables);
        }

      if (J->JGroup != NULL)
        {
        mjob_t *tmpJ = NULL;

        if (MJobFind(J->JGroup->Name,&tmpJ,mjsmExtended) == SUCCESS)
          {
          if (tmpJ->Variables.Table != NULL)
            {
            MUArrayListAppendPtr(&HashList,&tmpJ->Variables);
            }
          }
        }

      for (tindex = 0;tindex < MMAX_SPECTRIG;tindex++)
        {
        if (J->TemplateExtensions->SpecTrig[tindex] == NULL)
          break;

        MUInsertVarList(
          J->TemplateExtensions->SpecTrig[tindex],
          VarList,  /* var name array */
          (mhash_t **)HashList.Array, /* variables in hashtables */
          NULL,
          tmpBuf,   /* O */
          sizeof(tmpBuf),
          TRUE);
    
        MUArrayListFree(&HashList);

        if (MTrigLoadString(&J->Triggers,tmpBuf,TRUE,FALSE,mxoJob,J->Name,NULL,EMsg) == FAILURE)
          {
          return(FAILURE);
          }

        MUFree(&J->TemplateExtensions->SpecTrig[tindex]);
        }  /* END for (tindex) */

#ifdef MYAHOO
      /* make sure that holds continue to disable triggers, even after new ones are loaded */

      if (J->Hold != 0)
        {
        MTrigDisable(J->Triggers);
        }
#endif /* MYAHOO */

      if (J->Triggers != NULL)
        {
        mtrig_t *T;

        int tindex;

        for (tindex = 0; tindex < J->Triggers->NumItems;tindex++)
          {
          T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,tindex);

          if (MTrigIsValid(T) == FALSE)
            continue;

          MTrigInitialize(T);

          if (MSched.ForceTrigJobsIdle == TRUE)
            bmset(&J->IFlags,mjifWaitForTrigLaunch);
          }    /* END for (tindex) */

        /* report events for admin reservations */

        MOReportEvent((void *)J,J->Name,mxoJob,mttCreate,J->CTime,TRUE);
        }
      }    /* END else ((J->Cred.Q == NULL) || (!bmisset(&J->Cred.Q->Flags,mqfTrigger))) */

    MULLFree(&VarList,MUFREE);
    }      /* END if (J->TX->SpecTrig != NULL) */

  if (J->TemplateExtensions->SpecMinPreemptTime != NULL)
    {
    if ((J->Credential.Q == NULL) || (!bmisset(&J->Credential.Q->Flags,mqfPreemptCfg)))
      {
      MMBAdd(
        &J->MessageBuffer,
        "minpreempttime specified but not allowed by QOS",
        NULL,
        mmbtOther,
        MSched.Time + MCONST_DAYLEN,
        0,
        NULL);
      }
    else
      {
      MJobSetAttr(J,mjaMinPreemptTime,(void **)J->TemplateExtensions->SpecMinPreemptTime,mdfString,mSet);
      }

    MUFree((char **)&J->TemplateExtensions->SpecMinPreemptTime);
    }  /* if (J->TX->SpecMinPreemptTime != NULL) */

  if ((J->TemplateExtensions->SpecTemplates != NULL) && (J->TemplateExtensions->SpecTemplates[0] != '\0'))
    {
    /* Apply templates */

    mjob_t *SJ;
    char Templates[MMAX_LINE << 2];
    char *Value;
    char *ptr;
    char tmpLine[MMAX_LINE << 2];
    char *TokPtr2;

    MUStrCpy(Templates,J->TemplateExtensions->SpecTemplates,sizeof(Templates));
    Value = Templates;

    if (!bmisset(&J->Flags,mjfTemplatesApplied))
      {
      if (Value[0] == '^')
        {
        bmset(&J->IFlags,mjifNoTemplates);
        Value++;
        }

      TokPtr2 = Value;
      strncpy(tmpLine,Value,sizeof(tmpLine));

      while ((ptr = MUStrTok(TokPtr2,":,",&TokPtr2)) != NULL)
        {
        if ((MTJobFind(ptr,&SJ) == SUCCESS) && 
            (SJ->TemplateExtensions != NULL) && 
            (bmisset(&SJ->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable)))
          {
          if (MJobApplySetTemplate(J,SJ,EMsg) == FAILURE)
            {
            return(FAILURE);
            }
          }
        else
          {
          char errMsg[MMAX_LINE];

          snprintf(errMsg,sizeof(errMsg),"WARNING:  cannot apply template '%s'",
            ptr);

          MMBAdd(&J->MessageBuffer,errMsg,NULL,mmbtNONE,0,0,NULL);
          }

        if (!J->TemplateExtensions->ReqSetTemplates.empty())
          MStringAppend(&J->TemplateExtensions->ReqSetTemplates,",");

        MStringAppend(&J->TemplateExtensions->ReqSetTemplates,ptr);
        }
      }

    bmset(&J->Flags,mjfTemplatesApplied);
    bmset(&J->SysFlags,mjfTemplatesApplied);

    /* Free the memory, we don't need it anymore */

    MUFree(&J->TemplateExtensions->SpecTemplates);
    }  /* END if ((J->TX->SpecTemplates != NULL) && ...) */

  if ((J->WCLimit == MMAX_TIME) && (!bmisset(&MSched.Flags,mschedfAllowInfWallTimeJobs)))
    {
    if (EMsg != NULL)
      {
      sprintf(EMsg,"infinite walltime jobs not allowed\n");

      return(FAILURE);
      }
    }

  if ((J->TemplateExtensions->SpecArray != NULL) && 
      (J->TemplateExtensions->SpecArray[0] != '\0') &&
      (J->Array == NULL) &&
      (!bmisset(&J->IFlags,mjifWasLoadedFromCP)))
    {
    char *ptr;
    char *TokPtr = NULL;

    char *arrayStr = NULL;

    mulong BM;

    MUStrDup(&arrayStr,J->TemplateExtensions->SpecArray);

    /* Alloc the Array control container */
    if (MJobArrayAlloc(J) == FAILURE)
      {
      /* NO MEMORY ERROR */
      return(FAILURE);
      }

    ptr = MUStrTok(arrayStr,"[]",&TokPtr);

    MUStrCpy(J->Array->Name,ptr,MMAX_NAME);

    ptr = MUStrTok(NULL,"[]",&TokPtr);

    MUParseRangeString(
      ptr,
      &BM,
      J->Array->Members,
      MMAX_JOBARRAYSIZE,
      &J->Array->Count,
      NULL);

    ptr = MUStrTok(NULL,"%\n\t",&TokPtr);

    if ((ptr != NULL) && (ptr[0] != '\0'))
      {
      J->Array->Limit = strtol(ptr,NULL,10);
      }

    /* NO ERROR CHECKING ON 'NO MEMORY' CONDITION ??? */
    J->Array->JPtrs  = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * J->Array->Count + 1);
    J->Array->JName  = (char **)MUCalloc(1,sizeof(char *) * J->Array->Count + 1);
    J->Array->JState = (enum MJobStateEnum *)MUCalloc(1,sizeof(enum MJobStateEnum) * J->Array->Count + 1);

    MUFree(&arrayStr);
    }

  return(SUCCESS);
  } /* END MJobProcessRestrictedAttrs() */
/* END MJobProcess.c */
