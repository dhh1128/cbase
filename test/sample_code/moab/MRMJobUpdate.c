/* HEADER */

      
/**
 * @file MRMJobUpdate.c
 *
 * Contains: MJobPreUpdate and MJobPostUpdate
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

/**
 * Update common job attributes.
 *
 * @param J (I)
 */

int MRMJobPreUpdate(
 
  mjob_t *J)  /* I */
 
  {
  int     rqindex;
  int     gmindex;
  mreq_t *RQ;

  const char *FName = "MRMJobPreUpdate";
 
  MDB(5,fRM) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  J->UIteration = MSched.Iteration;

  J->MTime      = MSched.Time;
  J->ATime      = MSched.Time;

  bmunset(&J->IFlags,mjifBillingReserveFailure);
  bmunset(&J->IFlags,mjifIsNew);
  bmunset(&J->IFlags,mjifIsEligible);

  /* Now reset the GMetric IntervalLoad values to default value:
   *
   * For each of the Req's for this job:
   *     For each of the GMETRICs for this job
   */
  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ->XURes != NULL)
      {
      for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
        {
        RQ->XURes->GMetric[gmindex]->IntervalLoad = MDEF_GMETRIC_VALUE;
        } /* END for (gmindex) */
      }   /* END if (RQ->XURes != NULL) */
    }     /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MRMJobPreUpdate() */






/**
 * Handle common job tasks after RM update.
 *
 * @see MRMJobPostLoad() - peer
 *
 * NOTE: OldState is actually CurrentState, 5.2 should fix this
 *
 * @param J (I) [modified]
 * @param TaskList (I) [minsize=MSched.JobMaxTaskCount] (potentially modified)
 * @param OldState (I)
 * @param R (I)
 *
 * NOTE:  This routine performs actions in the following order:
 *
 *  Step 1:  Initialize
 *    Step 1.1:  Update Job Flags, Access Time, TTC
 *    Step 1.2:  Adjust Job Attributes Range-Based Duration and Dynamic Jobs 
 *    Step 1.3:  Synchronize Job State Transitions 
 *      Step 1.3.1:  Determine if Job was Rejected 
 *      Step 1.3.2:  Handle Standard State Transitions 
 *    Step 1.4:  Restore Default WCLimit if Unset 
 *    Step 1.5:  Check Class Constraints 
 *    Step 1.6:  Scale Job WCLimit by Node Speed 
 *    Step 1.7:  Pre-Eval/Adjust TaskList 
 *  Step 2:  Process Active Jobs 
 *    Step 2.1:  Clear Holds 
 *    Step 2.2:  Rebuild Taskmap 
 *    Step 2.3:  Adjust Job History 
 *    Step 2.4:  Add Job to Active Queue 
 *    Step 2.5:  Update Job Reservation 
 *  Step 3:  Process Suspended Jobs 
 *  Step 4:  General Job Synchronization 
 *    Step 4.1:  Restore Alloc/Requested Resource Counts 
 *    Step 4.2:  Adjust Job Statistics/state for Completed Jobs 
 *    Step 4.3:  Process Rejected Jobs 
 *    Step 4.4:  Check/Apply Credential-Based Holds 
 *    Step 4.5:  Update Job Resource/GMetric Usage 
 *    Step 4.6:  Update Job Reservation
 *    Step 4.7:  Handle External Job Start 
 *    Step 4.8:  Handle Job Checkpointing 
 *    Step 4.9:  Check Job Termination Time 
 */

int MRMJobPostUpdate(

  mjob_t             *J,        /* I (modified) */
  int                *TaskList, /* I (optional,minsize=MSched.JobMaxTaskCount) */
  enum MJobStateEnum  OldState, /* I */
  mrm_t              *R)        /* I */

  {
  mreq_t   *RQ;

  int       index = 0;
  int       nindex = 0;
  int       rqindex = 0;
  int       tindex = 0;
  int       jnindex = 0;
  mulong    DETime;

  long      DeferTime;

  mbool_t   BuildTaskMap;
  mbool_t   TCHasChanged;
  mbool_t   JobRejected;         /* job started by scheduler but rejected by RM */
  mbool_t   JobRequeued;         /* job checkpointed/requeued by user/admin */

  mbool_t   TaskListHasExternalResources;

  int      *tmpTaskList = NULL;

  mnode_t  *N = NULL;

  char      IdleNodeMessage[MMAX_LINE << 4];

  char     *INPtr;
  int       INSpace;

  char      tmpLine[MMAX_LINE];

  mnode_t  *LNode = NULL;  /* locally allocated virtual node */
 
  /* NOTE: TaskList only contains tasks from rm 'R' */

  const char *FName = "MRMJobPostUpdate";

  MDB(5,fRM) MLog("%s(%s,TaskList,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MJobState[OldState],
    (R != NULL) ? R->Name : "NULL");

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  /* Step 1:  Initialize */

  MUSNInit(&INPtr,&INSpace,IdleNodeMessage,sizeof(IdleNodeMessage));

  /* Step 1.1:  Update Job Flags, Access Time, TTC */

  J->ATime = MSched.Time;

  if ((MJobDetermineTTC(J) == SUCCESS) &&
      (TaskList != NULL))
    {
    /* modify TaskList to reflect TTC */

    TaskList[1] = -1;
    }

  if (J->Credential.C != NULL)
    {
    if (J->Credential.C->FType == mqftRouting)
      MJobSetState(J,mjsNotQueued);
    }

  if (bmisclear(&J->Flags))
    bmcopy(&J->Flags,&J->SpecFlags);

  MJobUpdateFlags(J);

  if (MJobWantsSharedMem(J) == TRUE)
    MRMUpdateSharedMemJob(J);

  /* Step 1.2:  Adjust Job Attributes for Range-Based Duration and Dynamic Jobs */

  /* slurm may send us a min walltime for the job in the workload query since
   * it is still in the extension string on slurm (and we currently have no
   * way of changing the extension string in slurm. If we extended the
   * job walltime on start then the minwclimit no longer applies so reset it
   * for this job on moab */

  if ((J->MinWCLimit > 0) && (bmisset(&J->Flags,mjfExtendStartWallTime)))
    {
    J->MinWCLimit = 0;
    J->OrigMinWCLimit = 0;
    }
  else if (J->MinWCLimit > 0)
    {
    /* scale-down walltime according to J->MinWCLimit */

    if (J->OrigWCLimit <= 0)
      J->OrigWCLimit  = J->WCLimit;

    J->WCLimit        = J->MinWCLimit;

    J->SpecWCLimit[0] = J->WCLimit;
    J->SpecWCLimit[1] = J->WCLimit;
    }

  /* Step 1.3:  Synchronize Job State Transitions */

  if ((J->EState == mjsStarting) && (J->State == mjsRunning))
    {
    /* starting job is now running */

    J->EState       = mjsRunning;
    }

  /* Step 1.3.1:  Determine if Job was Rejected */

  JobRejected = FALSE;
  JobRequeued = FALSE;

  if (((J->EState == mjsRunning) || (J->EState == mjsStarting)) &&  /* job should be active */
      ((J->State == mjsIdle) || (J->State == mjsHold)) &&           /* job is not active */
       (J->PrologTList == NULL) &&                                  /* job not waiting on prolog */
       (R->MaxJobStartDelay + J->StartTime < MSched.Time) &&        /* job is not trying to start */
       (!bmisset(&J->IFlags,mjifWasRequeued)))                 /* job not requeued w/in Moab */
    {
    /* job was requeued/rejected */

    if (J->CheckpointStartTime >= J->StartTime)
      {
      JobRequeued = TRUE;
      }
    else
      {
      /* job rejected by RM */

      JobRejected = TRUE;
      }

    if (bmisset(&J->IFlags,mjifAllocPartitionCreated))
      MJobDestroyAllocPartition(J,R,"job rejected");

    MOWriteEvent((void *)J,mxoJob,mrelJobReject,NULL,NULL,NULL);

    CreateAndSendEventLog(meltJobReject,J->Name,mxoJob,(void *)J);
    }

  /* Step 1.3.2:  Handle Standard State Transitions */

  switch (OldState)
    {
    case mjsSuspended:

      /* J->SWallTime += (long)((MSched.Interval + 50) / 100); */
      J->SWallTime = MAX(0,(long)(MSched.Time - J->StartTime - J->AWallTime));

      break;

    case mjsStarting:
    case mjsRunning:

      if (J->AWallTime <= 0)
        {
        J->AWallTime = MAX(0,(long)(MSched.Time - J->StartTime - J->SWallTime));

        MDB(7,fSCHED) MLog("INFO:     job %s setting walltime to %ld (was negative or zero) in function %s\n",
          J->Name,
          J->AWallTime,
          __FUNCTION__);
        }
      else if (MSched.Time > J->StartTime)
        {
        J->AWallTime = MAX(0,(long)(MSched.Time - J->StartTime - J->SWallTime));

        MDB(7,fSCHED) MLog("INFO:     job %s setting walltime to %ld in function %s\n",
          J->Name,
          J->AWallTime,
          __FUNCTION__);
        }

      if ((J->EState == mjsIdle) &&
          (MSched.SideBySide == TRUE))
        {
        /* FIXME:  temporary kluge until OldState really is OldState */
 
        int rqindex;

        /* running side-by-side - other RM has started job elsewhere */

        MDB(1,fRM) MLog("ALERT:    job '%s' was started externally\n",
          J->Name);

        MMBAdd(&J->MessageBuffer,"job was started externally",NULL,mmbtNONE,0,0,NULL);

        MJobSetState(J,mjsRunning);

        MNLClear(&J->NodeList);

        for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
          {
          if (J->Req[rqindex] == NULL)
            break;
     
          MNLClear(&J->Req[rqindex]->NodeList);
          }

        bmset(&J->SpecFlags,mjfNoResources);
        bmset(&J->Flags,mjfNoResources);

        J->EState = mjsRunning;
 
        MJobReleaseRsv(J,TRUE,TRUE);
        }

      break;

    case mjsDeferred:

      J->EState       = J->State;

      break;

    case mjsIdle:
    case mjsNotQueued:
    case mjsHold:
    case mjsSystemHold:

      if ((J->State == mjsIdle) ||
          (J->State == mjsNotQueued) ||
          (J->State == mjsHold) ||
          (J->State == mjsSystemHold))
        {
        if ((J->EState != mjsDeferred) &&
            !bmisset(&J->Hold,mhDefer))
          {
          /* syncronize state with expected state */

          J->EState       = J->State;
          }

        if ((J->State != OldState) && (J->State == mjsIdle))
          {
          if (OldState != mjsSystemHold)
            J->SystemQueueTime = MSched.Time;
          }

        if ((MSched.GuaranteedPreemption == TRUE) &&
            (bmisset(&J->IFlags,mjifWasRequeued)))
          {
          /* we need to clear out the reservation for jobs that were preempted 
             and took a long time to cleanup. */

          MJobReleaseRsv(J,FALSE,FALSE);

          bmunset(&J->IFlags,mjifWasRequeued);
          }
        }
      else if (J->State == mjsRemoved)
        {
        /* handle cancelled jobs */

        MDB(1,fRM) MLog("ALERT:    job '%s' was cancelled externally\n",
          J->Name);

        return(SUCCESS);
        }
      else if (MJOBISACTIVE(J) && 
              (J->EState != mjsStarting) && 
              (J->EState != mjsRunning))
        {
        /* handle remotely started jobs */

        if (MSched.SideBySide == TRUE)
          {
          int rqindex;

          /* running side-by-side - other RM has started job elsewhere */

          MDB(1,fRM) MLog("ALERT:    job '%s' was started externally\n",
            J->Name);

          MMBAdd(&J->MessageBuffer,"job was started externally",NULL,mmbtNONE,0,0,NULL);

          MJobSetState(J,mjsRunning);

          MNLClear(&J->NodeList);

          for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
            {
            if (J->Req[rqindex] == NULL)
              break;
     
            MNLClear(&J->Req[rqindex]->NodeList);
            }

          bmset(&J->SpecFlags,mjfNoResources);
 
          MJobReleaseRsv(J,TRUE,TRUE);
          }
        else if (MSched.Mode == msmNormal)
          {
          MDB(1,fRM) MLog("ALERT:    job '%s' was started externally\n",
            J->Name);

          MMBAdd(&J->MessageBuffer,"job was started externally",NULL,mmbtNONE,0,0,NULL);
          }

        /* record start event */

        if (J->StartTime == 0)
          J->StartTime = MSched.Time;

        MOWriteEvent(
          (void *)J,
          mxoJob,
          mrelJobStart,
          "job was started externally",
          MStat.eventfp,
          NULL);

        if (MSched.PushEventsToWebService == TRUE)
          {
          MEventLog *Log = new MEventLog(meltJobStart);
          Log->SetCategory(melcStart);
          Log->SetFacility(melfJob);
          Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
          Log->AddDetail("msg","job was started externally");

          MEventLogExportToWebServices(Log);

          delete Log;
          }
        }    /* END else if (MJOBISACTIVE(J) && ...) */

      break;

    default:

      /* NOT HANDLED */

      /* NO-OP */

      break;
    }  /* END switch (OldState) */

  /* Step 1.4:  Restore Default WCLimit if Unset */

  if (J->SpecWCLimit[1] == (unsigned long)-1)
    {
    /* NOTE:  apply global default limits before checking class/queue limits */

    MDB(4,fRM) MLog("INFO:     wallclock limit set to unlimited to job %s\n",
      J->Name);

    J->SpecWCLimit[1] = MPar[0].L.JobPolicy->HLimit[mptMaxWC][0];
    }

  /* Step 1.5:  Check Class Constraints */

  if ((!bmisset(&J->Flags,mjfNoRMStart) || !bmisset(&J->Flags,mjfClusterLock)) &&
      (MJobCheckClassJLimits(J,J->Credential.C,FALSE,0,tmpLine,MMAX_LINE) == FAILURE))
    {
    char tmpMsg[MMAX_LINE];

    sprintf(tmpMsg,"job violates class configuration '%s'",
      tmpLine);

    /* job does not meet class constraints */

    MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,tmpLine);

    if (J == NULL)
      {
      return(FAILURE);
      }
    }

  /* Step 1.6:  Scale Job WCLimit by Node Speed */

  J->SpecWCLimit[0] = J->SpecWCLimit[1];

  if ((!bmisset(&MPar[0].Flags,mpfUseMachineSpeed)) ||
     ((J->State != mjsRunning) && (J->State != mjsStarting)))
    {
    J->WCLimit = J->SpecWCLimit[0];
    }
  else
    {
    double tmpD;

    MUNLGetMinAVal(&J->NodeList,mnaSpeed,NULL,(void **)&tmpD);

    J->WCLimit = (long)((tmpD > 0.0) ? 
      (long)((double)J->SpecWCLimit[0]) / tmpD :
      J->SpecWCLimit[0]);
    }

  if (J->VWCLimit > 0)
    MJobAdjustVirtualTime(J);

  /* Step 1.7:  Pre-Eval/Adjust TaskList */

  /* adjust active job statistics/state */

  if (bmisset(&J->IFlags,mjifCopyTaskList) ||
      (J->SubState == mjsstProlog))
    {
    /* moab prolog modified job - RM may not yet have resource information */

    if (TaskList == NULL)
      {
      tmpTaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

      TaskList = tmpTaskList;
      }

    memcpy(TaskList,J->TaskMap,sizeof(int) * (J->TaskMapSize));

    TaskList[J->TaskMapSize] = -1;

    bmunset(&J->IFlags,mjifCopyTaskList);
    }

  if ((TaskList != NULL) && (TaskList[0] != -1))
    TaskListHasExternalResources = TRUE;
  else 
    TaskListHasExternalResources = FALSE;

  /* Step 2:  Process Active Jobs */

  if (MJOBISACTIVE(J))
    {
    mbool_t MultiRMJob = FALSE;  /* if TRUE, job has resources allocated from multiple RMs */

    /* check for multi-rm'ed jobs */

    if (J->Req[1] != NULL)
      {
      mnode_t *tmpN;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        if (MReqIsGlobalOnly(RQ))
          {
          MultiRMJob = TRUE;

          break;
          }

        if (MNLGetNodeAtIndex(&RQ->NodeList,0,&tmpN) == FAILURE)
          continue;

        if (tmpN->RM == NULL)
          continue;

        if ((MNLGetNodeAtIndex(&J->Req[0]->NodeList,0,&N) == SUCCESS) &&
            (N != tmpN))
          {
          MultiRMJob = TRUE;

          break;
          }
        }
      }    /* END if (J->Req[1] != NULL) */

    /* Step 2.1:  Clear Holds */

    /* if job is active, unset all holds */

    bmclear(&J->Hold);

    if (J->EState == mjsDeferred)
      J->EState = J->State;

    /* Step 2.2:  Rebuild Taskmap */

    /* J->TaskMap guaranteed to be allocated by MRMJobPreLoad() */

    if ((OldState == mjsHold) ||
        (OldState == mjsIdle) ||
        (OldState == mjsNotRun) ||
        (MNLIsEmpty(&J->NodeList)))
      {
      /* build new task list - may still have stale task/node info from previous 
         job start/execution */

      if ((R->NoAllocMaster == FALSE) &&
         ((R->SubType == mrmstXT3) ||
          (R->SubType == mrmstXT4) ||
          (R->SubType == mrmstX1E)))
        {
        /* NOTE:  store locally allocated resources */

        if (MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS)
          {
          /* Moab has already pre-populated J->NodeList */

          if (N->CRes.Procs == 0)
            {
            /* save local virtual resource assignment */

            LNode = N;
            }
          }
        else if ((TaskListHasExternalResources == TRUE) &&
                 (MNode[TaskList[0]]->CRes.Procs != 0) &&
                 (J->Req[0]->DRes.Procs == 0))
          {
          mnl_t tmpNL;

          /* job requires local virtual resources but these have not been 
             assigned to the job's tasklist */

          /* ie, monitor mode */

          /* find available virtual node and assign to LNode */

          MNLInit(&tmpNL);

          if ((MNLGetNodeAtIndex(&J->Req[0]->NodeList,0,&N) == SUCCESS) && 
              (N->CRes.Procs == 0))
            {
            /* local node already assigned to req, but not task list */

            LNode = MNLReturnNodeAtIndex(&tmpNL,0);
            }          
          else if (MReqGetFNL(
                J,
                J->Req[0],
                &MPar[0],
                NULL,
                &tmpNL,
                NULL,
                NULL,
                MSched.Time,
                0,
                NULL) == FAILURE)
            {
            MDB(2,fRM) MLog("ALERT:    cannot locate feasible virtual resources for %s:%d\n",
              J->Name,
              J->Req[0]->Index);
            }
          else
            {
            /* allocate first available node from FNL list */

            LNode = MNLReturnNodeAtIndex(&tmpNL,0);
            }

          MNLFree(&tmpNL);
          }
        }    /* END if ((R->NoAllocMaster == FALSE) && ...) */

      /* tasklist info is lost or job is transitioning to active state */

      /* rebuild may not be required but force rebuild just in case */

      MNLClear(&J->NodeList);

      if (J->TaskMapSize > 0)
        J->TaskMap[0] = -1;
      }  /* END if ((OldState == mjsHold) || ...) */
    else if (MultiRMJob == FALSE)
      {
      /* check task index ordering */

      if (TaskList != NULL)
        {
        mbool_t JobHasMap;
        int     tmindex;

        if ((J->TaskMapSize == 0) || (J->TaskMap[0] == -1))
          JobHasMap = FALSE;
        else
          JobHasMap = TRUE;

        if (J->Req[1] != NULL)
          {
          for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
            {
            RQ = J->Req[rqindex];

            if (RQ->DRes.Procs == 0)
              {
              /* handle local resources */

              MReqAllocateLocalRes(J,RQ);
              }
            }  /* END for (rqindex) */

          for (tindex = 0;tindex < MSched.JobMaxTaskCount;tindex++)
            {
            if (TaskList[tindex] == -1)
              break;
            }  /* END for (tindex) */
 
          if (JobHasMap == TRUE)
            {
            MJobGetLocalTL(J,TRUE,TaskList,MSched.JobMaxTaskCount);
            }
          }  /* END if (J->Req[1] != NULL) */
        else if ((JobHasMap == TRUE) && (bmisset(&J->IFlags,mjifPBSGPUSSpecified)))
          {
          MJobTranslateNCPUTaskList(J,TaskList);
          }

        /* NOTE:  taskmap may change for multi-req allocation */
        /*        change is acceptable but must verify resource 
                  quantity has not changed */

        /* NOTE:  some resource managers will lose task ordering info */

        tmindex = 0;

        if ((R->NoAllocMaster == FALSE) &&
           ((R->SubType == mrmstXT3) ||
            (R->SubType == mrmstXT4) ||
            (R->SubType == mrmstX1E)))
          {
          /* if TaskList does not include master task, skip it */

          if (J->TaskMap[tmindex] != TaskList[0])
            tmindex++;
          }

        /* walk new tasklist info, locate if/where TaskList and J->TaskMap disagree */

        for (tindex = 0;TaskList[tindex] != -1;tindex++)
          {
          if (tmindex >= J->TaskMapSize)
            {
            if (MJobGrowTaskMap(J,0) == FAILURE)
              break;
            }

          if (J->TaskMap[tmindex] == -1)
            {
            /* end of J->TaskMap reached - new TaskList is longer or
               job just started and J->TaskMap not yet populated */

            if ((R->NoAllocMaster == FALSE) &&
               ((R->SubType == mrmstXT3) ||
                (R->SubType == mrmstXT4) ||
                (R->SubType == mrmstX1E)))
              {
              /* non-compute virtual nodes will be added to the end 
                 by MJobGetLocalTL() - at this point, end comparison */

              /* is tindex incremented as a bad way to make TaskList[tindex] != -1?  who uses it? */

              /* NOTE:  tindex value is not used outside of 'if (TaskList != NULL)' loop */

              /* NOTE:  tindex incr is safe and will not step off end of TaskList[] array - see for loop above */

              tindex++;
              }

            break;
            }

          if (MNode[J->TaskMap[tmindex]] == NULL)
            {
            MDB(6,fRM) MLog("ALERT:    tasklist is corrupt for job %s at index %d\n",
              J->Name,
              tindex);
 
            break;
            }

          /* NOTE:  J->TaskMap[] does not include locally allocated non-compute tasks */

          if (J->TaskMap[tmindex] != TaskList[tindex])
            {
            if (JobHasMap == TRUE) 
              {
              MDB(6,fRM) MLog("ALERT:    task %d changed from '%s' to '%s' for active job '%s'\n",
                tindex,
                MNode[J->TaskMap[tmindex]]->Name,
                MNode[TaskList[tindex]]->Name,
                J->Name);
              }

            break;
            }

          /* TaskList[] and J->TaskMap[] match at this location, check next task */

          tmindex++;
          }    /* END for (tindex) */

        if (TaskList[tindex] != -1) 
          {
          if (bmisset(&R->IFlags,mrmifNoTaskOrdering) &&
             (MTaskMapIsEquivalent(J->TaskMap,TaskList) == TRUE))
            {
            /* task info is equivalent */

            /* NO-OP */
            }
          else
            {
            int rqindex;
 
            /* rebuild map - clear all req nodeslists */

            MNLClear(&J->NodeList);

            for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
              {
              if (J->Req[rqindex] == NULL)
                break;

              MNLClear(&J->Req[rqindex]->NodeList);
              }
            }
          }    /* END if (TaskList[tindex] != -1) */
        }      /* END if (TaskList != NULL)    */
      }        /* END else if (MultiRMJob == FALSE) */

    if ((TaskListHasExternalResources == TRUE) && (MultiRMJob == FALSE))
      {
      int tmindex;

      if (MNLIsEmpty(&J->NodeList))
        BuildTaskMap = TRUE;
      else
        BuildTaskMap = FALSE;

      tmindex = 0;

      /* NOTE:  restore locally allocated resources */

      if ((R->NoAllocMaster == FALSE) &&
         ((R->SubType == mrmstXT3) ||
          (R->SubType == mrmstXT4) ||
          (R->SubType == mrmstX1E)))
        {
        /* NOTE:  restore locally allocated resources */
     
        if (LNode != NULL)
          {
          if (BuildTaskMap == TRUE)
            {
            J->TaskMap[tmindex]        = LNode->Index;

            MNLSetNodeAtIndex(&J->NodeList,tmindex,LNode);
            MNLSetTCAtIndex(&J->NodeList,tmindex,1);

            MNLTerminateAtIndex(&J->NodeList,tmindex + 1);

            tmindex++;
            }
          }
        }

      for (tindex = 0;TaskList[tindex] != -1;tindex++)
        {
        if (tindex >= MSched.JobMaxTaskCount)
          {
          MDB(0,fRM) MLog("WARNING:  job '%s' size exceeds internal maximum job size (%d)\n",
            J->Name,
            MSched.JobMaxTaskCount);

          break;
          }

        if ((TaskList[tindex] < 0) || (TaskList[tindex] > MSched.M[mxoNode]))
          {
          MDB(0,fRM) MLog("WARNING:  job '%s' node index (%d) at task list index (%d) out of range\n",
            J->Name,
            TaskList[index],
            tindex);

          break;
          } 

        N = MNode[TaskList[tindex]];

        if (N == NULL)
          {
          MDB(0,fRM) MLog("WARNING:  job '%s' node at node index (%d) from task list index (%d) is NULL\n",
            J->Name,
            TaskList[index],
            tindex);

          break;
          } 

        if (BuildTaskMap == TRUE)
          {
          mnode_t *tmpN;

          /* build new node list */

          if (tmindex >= J->TaskMapSize)
            {
            if (MJobGrowTaskMap(J,0) == FAILURE)
              {
              break;
              }
            }

          if (J->TaskMap[tmindex] == -1)
            {
            /* this could be a problem if the terminator is not restored later on */

            MDB(7,fRM) MLog("INFO:     job '%s' taskmap terminator overwritten at index %d\n",
              J->Name,
              tmindex);
            }

          J->TaskMap[tmindex++] = TaskList[tindex];

          for (jnindex = 0;MNLGetNodeAtIndex(&J->NodeList,jnindex,&tmpN) == SUCCESS;jnindex++)
            {
            if (tmpN->Index == TaskList[tindex])
              {
              MNLAddTCAtIndex(&J->NodeList,jnindex,1);

              break;
              }
            }    /* END for (jnindex) */

          if (MNLGetNodeAtIndex(&J->NodeList,jnindex,NULL) == FAILURE)
            {
            MNLSetNodeAtIndex(&J->NodeList,jnindex,MNode[TaskList[tindex]]);
            MNLSetTCAtIndex(&J->NodeList,jnindex,1);

            MNLTerminateAtIndex(&J->NodeList,jnindex + 1);
            }
          }   /* END if (BuildTaskMap == TRUE) */

        MDB(6,fRM) MLog("INFO:     task %d assigned to job '%s'\n",
          tindex,
          J->Name);

        if ((N->State == mnsIdle) && 
            (N->RM != NULL) && 
            (N->RM->Type != mrmtPBS))
          {
          /* NOTE:  PBS RM's only report 'free' and 'busy'.  There is no info
                    on 'active' status (Moab which generate this info later in
                    the scheduling iteration) */

          if ((MSched.Time - J->StartTime > MCONST_MINUTELEN * 5) && (N->CRes.Procs > 0))
            {
            MDB(3,fRM) MLog("ALERT:    RM state corruption.  job '%s' has idle node '%s' allocated (node forced to active state)\n",
              J->Name,
              N->Name);

            if (IdleNodeMessage[0] == '\0')
              {
              MUSNPrintF(&INPtr,&INSpace,"JOBCORRUPTION:  job '%s' has the following idle node(s) allocated:\n",
                J->Name);
              }

            MUSNPrintF(&INPtr,&INSpace," '%s' ",
              N->Name);
            }    /* END if ((MSched.Time - J->StartTime > 300) && ...) */

          /* adjust node state */

          if ((N->DRes.Procs >= N->CRes.Procs) && (N->CRes.Procs > 0))
            N->EState = mnsBusy;
          else
            N->EState = mnsActive;
          }  /* END else if (N->State == mnsIdle) */
        }          /* END for (tindex) */

      /* terminate lists */

      if (BuildTaskMap == TRUE)
        J->TaskMap[tmindex] = -1;

      /* register failure events, if any */

      if ((OldState != mjsHold) &&
          (OldState != mjsIdle) &&
          (OldState != mjsNotRun))
        {
        /* job was in active state on last iteration */

        if ((IdleNodeMessage[0] != '\0') &&
            (MSched.Time - J->StartTime > 150))
          {
          /* indicate idle node allocated to active job */

          MSysRegEvent(IdleNodeMessage,mactNONE,1);
          }
        }

      MJobPopulateReqNodeLists(J,&TCHasChanged);

      if (TCHasChanged == TRUE)
        {
        mbool_t NCSet = FALSE;

        /* update job level node and task count */

        J->Request.TC = 0;

        if (J->Request.NC > 0)
          {
          NCSet = TRUE;

          J->Request.NC = 0;
          }

        for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
          {
          RQ = J->Req[rqindex];

          J->Request.TC += RQ->TaskCount;

          if (NCSet == TRUE)
            J->Request.NC += RQ->NodeCount; 
          }  /* END for (rqindex) */
        }    /* END if (TCHasChanged) */
      }      /* END if ((TaskList != NULL) && ...) */

    /* Step 2.3:  Adjust Job History */

    RQ = J->Req[0];

    J->StartTime = MAX(J->StartTime,J->SubmitTime);

    if (J->StartTime == 0)
      {
      MDB(0,fRM) MLog("ERROR:    job '%s' updated in active state '%s' with NULL start time\n",
        J->Name,
        MJobState[J->State]);

      J->StartTime    = MSched.Time;
      J->DispatchTime = MSched.Time;
      }

    if (J->SpecWCLimit[0] < MMAX_TIME)
      J->RemainingTime = ((long)J->WCLimit > (long)J->AWallTime) ? 
        (long)J->WCLimit - (long)J->AWallTime : 0;

    if ((J->Alloc.TC <= 0) &&
        !bmisset(&J->Flags,mjfNoResources))
      {
      MDB(2,fRM) MLog("WARNING:  job '%s' loaded in active state with no tasks allocated\n",
        J->Name);

      MDB(2,fRM) MLog("INFO:     adjusting allocated tasks for job '%s'\n",
        J->Name);

      J->Alloc.TC = J->Request.TC;

      MNLClear(&J->NodeList);

      if (RQ != NULL)
        MNLClear(&RQ->NodeList);
      }  /* END if ((J->Alloc.TC <= 0) && ...) */
   
    /* Update Job Resource/GMetric Usage BEFORE adding to active queue, otherwise statistics are lost */

    MJobUpdateResourceUsage(J);

    /* Step 2.4:  Add Job to Active Queue */

    MQueueAddAJob(J);

    /* Step 2.5:  Update Job Reservation */

    /* only make reservation on previously active jobs */

    if ((OldState == mjsStarting) || (OldState == mjsRunning) ||
        (OldState == mjsHold) || (OldState == mjsIdle) ||
        (OldState == mjsNotRun))
      {
      RQ = J->Req[0];  /* FIXME:  if req info is lost */

      if (MNLIsEmpty(&RQ->NodeList))
        {
        MNLCopy(&RQ->NodeList,&J->NodeList);
        }

      if (J->Rsv != NULL)
        {
        if ((R->JobRsvRecreate == TRUE) && (J->System == NULL) &&
            (MRsvCheckJobMatch(J,J->Rsv) == FAILURE))
          {
          /* FIXME: when MRsvCheckJobMatch() works, change to TRUE */

          MJobReleaseRsv(J,TRUE,FALSE);

          MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);
          }
        }
      else if (!bmisset(&J->SpecFlags,mjfRsvMap) &&
          !bmisset(&J->SpecFlags,mjfNoResources))
        {
        MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);
        }
      }
    }   /* END if (MJOBISACTIVE(J)) */

  /* Step 3:  Process Suspended Jobs */

  if (J->State == mjsSuspended)
    {
    RQ = J->Req[0];

    if (MNLIsEmpty(&RQ->NodeList))
      {
      /* NOTE:  address primary req only */

      MNLCopy(&RQ->NodeList,&J->NodeList);
      }

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      N->SuspendedJCount++;
      }
    }    /* END else if (J->State == mjsSuspended) */

  /* Step 4:  General Job Synchronization */

  if (J->Req[1] != NULL)
    {
    /* multi-req job */

    /* Step 4.1:  Restore Alloc/Requested Resource Counts */

    if (MJOBISACTIVE(J) || MJOBISCOMPLETE(J))
      {
      int nindex;

      J->Request.NC = 0;
      J->Request.TC = 0;

      J->Alloc.TC = 0;
      J->Alloc.NC = 0;

      J->FloatingNodeCount = 0;

      if (!MNLIsEmpty(&J->NodeList))
        {
        for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          J->Request.NC++;
          J->Request.TC += MNLGetTCAtIndex(&J->NodeList,nindex);

          if (N == MSched.GN)
            J->FloatingNodeCount++;

          if (MJOBISACTIVE(J))
            {
            J->Alloc.TC += MNLGetTCAtIndex(&J->NodeList,nindex);
            J->Alloc.NC ++;
            }
          }    /* END for (nindex) */
        }      /* END if (J->NodeList != NULL) */
      }        /* END if (MJOBISACTIVE(J) || MJOBISCOMPLETE(J)) */
    }          /* END if (J->Req[1] != NULL) */

  /* Step 4.2:  Adjust Job Statistics/state for Completed Jobs */

  if (MJOBISCOMPLETE(J))
    {
    if ((J->StartTime == 0) && !bmisset(&J->IFlags,mjifWasCanceled))
      {
      MDB(0,fRM) MLog("ERROR:    job '%s' updated to completed state '%s' with no start time\n",
        J->Name,
        MJobState[J->State]);

      /* make best guess */

      J->StartTime    = MSched.Time;
      J->DispatchTime = MSched.Time;
      }

    if (J->CompletionTime <= J->StartTime)
      {
      /* NOTE:  if J->SWallTime and J->AWallTime are both 0,
                J->CompletionTime will be the same as J->StartTime, which
                results in an effective job duration of 0, which probably
                isn't correct... */

      if ((J->StartTime == 0) || (J->SWallTime > 0) || (J->AWallTime > 0))
        {
        J->CompletionTime = MIN(
          MSched.Time,
          J->StartTime + J->SWallTime + MAX(J->AWallTime,0));

        MDB(7,fSCHED) MLog("INFO:     CompletionTime for %s was set to MIN(%d,%d + %d + MAX(%d,0))\n",
            J->Name,
            MSched.Time,
            J->StartTime,
            J->SWallTime,
            J->AWallTime);
        }
      else
        {
        J->CompletionTime = MSched.Time;

        MDB(7,fSCHED) MLog("INFO:     CompletionTime for %s was set to MSched.Time (%d)\n", 
          J->Name,
          MSched.Time);
        }

      }

    if (J->Alloc.TC <= 0)
      J->Alloc.TC = J->Request.TC;
    }  /* END if (MJOBISCOMPLETE(J)) */

  /* Step 4.3:  Process Rejected Jobs */

  if ((JobRejected == TRUE) || (JobRequeued == TRUE))
    {
    /* handle rejected/requeued jobs */

    char tmpLine[MMAX_LINE];
    char tmpNList[MMAX_LINE];
    char tmpTime[MMAX_LINE];

    char *BPtr;
    int   BSpace;

    /* For now, if job is rejected, assume something is wrong with the node.  
       Add that node to an exclusion host list.  For now, only enable this for 
       serial jobs.
     */

    if ((JobRequeued == FALSE) && 
        (MNLGetNodeAtIndex(&J->NodeList,0,NULL) == SUCCESS) &&
        (MNLGetNodeAtIndex(&J->NodeList,1,NULL) == FAILURE))
      {
      char tmpLine[MMAX_LINE];

      if ((J->Credential.C != NULL) &&
          (J->Credential.C->CancelFailedJobs == TRUE))
        {
        /* job is serial, it failed to start, we're going to cancel it */

        MDB(3,fRM) MLog("INFO:     job '%s' failed to start, cancelling job (see class policies)\n",
          J->Name,
          MJobState[J->State]);

        MMBAdd(&MSched.MB,"job was rejected by RM - cancelling",NULL,mmbtNONE,0,0,NULL);

        MJobCancel(J,"job was rejected by RM - cancelling",FALSE,NULL,NULL);
        }
      else if (MPar[0].JobRetryTime == 0)
        {
        /* assume node failure is permanent - add node to job's rejection hostlist */

        MNLGetNodeAtIndex(&J->NodeList,0,&N);

        MJobSetAttr(J,mjaExcHList,(void **)N->Name,mdfString,mAdd);

        sprintf(tmpLine,"job failed to execute on node %s",
          MNode[J->TaskMap[0]]->Name);

        MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }
      }    /* END if ((J->NodeList != NULL) && ...) */

    if ((J->DestinationRM != NULL) && 
        (J->DestinationRM->Type == mrmtNative) && 
       ((J->DestinationRM->SubType == mrmstXT3) || (J->DestinationRM->SubType == mrmstXT4)))
      {
      char tEMsg[MMAX_LINE];
      char requeueMsg[MMAX_LINE];

      /* NOTE:  On XT3 systems, a rejected job may be retried by the RM */
      /*        requeue job to make certain RM does not restart job */

      if (JobRejected == TRUE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"job %s rejected by RM - requeuing",
          J->Name);
       
        MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }

      MJobDestroyAllocPartition(J,J->DestinationRM,"job rejected");

      snprintf(requeueMsg,sizeof(requeueMsg),"%s by RM",
        (J->CheckpointStartTime > J->StartTime) ? "checkpointed" : "rejected");

      if (MJobRequeue(J,NULL,requeueMsg,tEMsg,NULL) == FAILURE)
        {
        MDB(1,fRM) MLog("ALERT:    cannot requeue rejected job %s - %s\n",
          J->Name,
          tEMsg);
        }
      }    /* END if ((J->DRM != NULL) && ...) */

    /* Cancel lien if necessary */
    if (J->Credential.A != NULL)
      {
      if (MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,NULL) == FAILURE)
        {
        MDB(3,fSCHED) MLog("WARNING:  Unable to register job deletion with accounting manager for job '%s'\n",
          J->Name);
        }
      }

    DeferTime = MSched.DeferTime * (1 + J->StartCount);

    J->Alloc.NC   = 0;
    J->Alloc.TC   = 0;

    /* Only if we actually have a TaskMap, then clear the map */
    if (J->TaskMapSize > 0)
      J->TaskMap[0] = -1;

    MDB(2,fRM) MLog("ALERT:    job '%s' was in state '%s' but is now in state '%s' (job was rejected/requeued)\n",
      J->Name,
      MJobState[J->EState],
      MJobState[J->State]);

#ifdef __NCSA
    sprintf(tmpLine,"WARNING:  job '%s' changed states from running to idle\n",
      J->Name);

    MSysRegEvent(tmpLine,mactMail,0,1);
#endif /* __NCSA */

    MDB(4,fRM)
      {
      if (!MNLIsEmpty(&J->NodeList))
        {
        MDB(4,fRM) MLog("INFO:     job '%s' nodelist: ",
          J->Name);

        for (index = 0;MNLGetNodeAtIndex(&J->NodeList,index,&N) == SUCCESS;index++)
          {
          MLogNH("[%16s]",
            N->Name);
          }  /* END for (index) */

        MLogNH("\n");
        }
      }

    MUSNInit(&BPtr,&BSpace,tmpNList,sizeof(tmpNList));

    RQ = J->Req[0];

    if ((RQ != NULL) && (!MNLIsEmpty(&RQ->NodeList)))
      {
      for (index = 0;index < 4;index++)
        {
        if (MNLGetNodeAtIndex(&RQ->NodeList,index,&N) == FAILURE)
          break;

        MUSNPrintF(&BPtr,&BSpace,"%s%s",
          (tmpNList[0] != '\0') ? "," : "",
          N->Name);
        }  /* END for (index) */

      if (MNLGetNodeAtIndex(&RQ->NodeList,index,NULL) == SUCCESS)
        MUSNCat(&BPtr,&BSpace,"...");
      }

    RQ = J->Req[1];

    if ((RQ != NULL) && (!MNLIsEmpty(&RQ->NodeList)))
      {
      for (index = 0;index < 4;index++)
        {
        if (MNLGetNodeAtIndex(&RQ->NodeList,index,&N) == FAILURE)
          break;

        MUSNPrintF(&BPtr,&BSpace,"%s%s",
          (tmpNList[0] != '\0') ? "," : "",
          N->Name);
        }  /* END for (index) */

      if (MNLGetNodeAtIndex(&RQ->NodeList,index,NULL) == SUCCESS)
        MUSNCat(&BPtr,&BSpace,"...");
      }

    if (JobRejected == TRUE)
      {
      MUStrCpy(tmpTime,MULToATString(J->StartTime,NULL),sizeof(tmpTime));

      snprintf(tmpLine,sizeof(tmpLine),"job rejected by RM '%s' - job started on hostlist %s at time %s, job reported idle at time %s (see RM logs for details)\n",
        R->Name,
        tmpNList,
        tmpTime,
        MULToATString(MSched.Time,NULL));

      if (J->StartCount >= MSched.DeferStartCount) 
        {
        if (!bmisset(&J->Flags,mjfNoRMStart) || 
            !bmisset(&J->Flags,mjfClusterLock))
          {
          MDB(2,fRM) MLog("ALERT:    job '%s' is being deferred for %ld seconds\n",
            J->Name,
            DeferTime);

          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrRMReject,tmpLine);
          }
        }
      }
    }  /* END if ((JobRejected == TRUE) || (JobRequeued == TRUE)) */

  /* Step 4.4:  Check/Apply Credential-Based Holds */

  if (MJOBISIDLE(J))
    MJobAdjustHold(J);

  /* Step 4.7:  Handle External Job Start */

  if (((OldState == mjsHold) ||
       (OldState == mjsIdle) ||
       (OldState == mjsNotRun)) &&
      ((J->State == mjsRemoved) ||
       (J->State == mjsCompleted) ||
       (J->State == mjsVacated)))
    {
    MDB(4,fRM) MLog("INFO:     job '%s' state transition (%s --> %s)\n",
      J->Name,
      MJobState[OldState],
      MJobState[J->State]);

    /* if job starts and completes while scheduler is sleeping (cannot happen with x scheduling) */

    if (J->EState != mjsRunning)
      {
      MDB(0,fRM) MLog("WARNING:  scheduler cannot handle completion in 1 iteration on job '%s'\n",
        J->Name);

      /* set job to active so it can be properly removed */

      if (MAMHandleStart(&MAM[0],J,mxoJob,NULL,NULL,NULL) == FAILURE)
        {
        MDB(3,fRM) MLog("WARNING:  Unable to register job start with accounting manager for job '%s'\n",
          J->Name);
        }

      MQueueAddAJob(J);

      if (J->DispatchTime == 0)
        {
        if (J->State == mjsCompleted)
          {
          MDB(0,fRM) MLog("ERROR:    Job '%s' updated to completed state '%s' with NULL dispatch time\n",
            J->Name,
            MJobState[J->State]);
          }

        J->DispatchTime = MSched.Time;
        }

      RQ = J->Req[0];  /* if req info is lost */

      if (MNLIsEmpty(&RQ->NodeList))
        {
        MNLCopy(&RQ->NodeList,&J->NodeList);
        }

      MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);

      /* complete job record */

      MDB(1,fRM) MLog("INFO:     completing job '%s'\n",
        J->Name);
      }
    }     /* END if (((OldState == mjsHold) || ... */

  /* Step 4.8:  Handle Job Checkpointing */

  if (J->CheckpointStartTime > 0)
    {
    /* job is checkpointing */

    if ((R->CheckpointTimeout > 0) &&
        (MSched.Time > J->CheckpointStartTime + R->CheckpointTimeout))
      {
      int rc;

      /* cancel/requeue job */

      MDB(3,fRM) MLog("INFO:     checkpoint timeout for job '%s' (%ld)\n",
        J->Name,
        R->CheckpointTimeout);

      if (bmisset(&R->Flags,mrmfCkptRequeue))
        {
        rc = MJobRequeue(J,NULL,"checkpoint requested",NULL,NULL);
        }
      else
        {
        rc = MJobCancel(J,"MOAB_INFO:  checkpoint timeout",FALSE,NULL,NULL);
        }

      if (rc == SUCCESS)
        {
        J->CheckpointStartTime = 0;
        }
      else
        {
        MDB(3,fRM) MLog("ALERT:    checkpoint timeout for job '%s' reached but job cannot be terminated, will retry\n",
          J->Name);
        }
      }
    }    /* END if (J->CheckpointStartTime > 0) */

  /* Step 4.9:  Check Job Termination Time */

  if ((J->State != mjsCompleted) && (J->State != mjsRemoved))
    {
    if ((J->TermTime > 0) && (J->TermTime < MSched.Time))
      {
      /* cancel job */

      MMBAdd(&J->MessageBuffer,"termination time reached",NULL,mmbtNONE,0,0,NULL);

      MDB(3,fRM) MLog("INFO:     termination time reached for job '%s' (%ld)\n",
        J->Name,
        J->TermTime);

      MJobCancel(J,"MOAB_INFO:  termination time reached",FALSE,NULL,NULL);
      }
    }    /* END if ((J->State != mjsCompleted) && (J->State != mjsRemoved)) */

  /* check job synchronization */

  if ((J->State != J->EState) || bmisset(&J->Hold,mhDefer))
    {
    MDB(5,fRM) MLog("INFO:     checking job '%s' defer time (is %ld > %ld)\n",
      J->Name,
      MSched.Time,
      J->HoldTime + MSched.DeferTime);

    if ((long)MSched.Time > (long)(J->HoldTime + MSched.DeferTime))
      {
      /* check if DeferTime is expired */

      if ((J->EState == mjsDeferred) || bmisset(&J->Hold,mhDefer))
        {
        mbitmap_t BM;

        MDB(2,fRM) MLog("INFO:     restoring job '%s' from deferred state\n",
          J->Name);

        J->EState = J->State;

        bmset(&BM,mhDefer);

        MJobSetAttr(J,mjaHold,(void **)&BM,mdfOther,mUnset);

        J->SystemQueueTime = MSched.Time;
        }
      }
    }      /* END if (J->State != J->EState) */

  /* set assigned req partition index (not J->PAL) */
  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    RQ->TasksPerNode = MIN(RQ->TasksPerNode,RQ->TaskCount);

    if (MNLGetNodeAtIndex(&RQ->NodeList,0,&N) == SUCCESS)
      {
      RQ->PtIndex = N->PtIndex;
      }
    else
      {
      RQ->PtIndex = 0;
      }
    }    /* END for (rqindex) */

  if (J->CPULimit == 0)
    {
    if ((MPar[J->Req[0]->PtIndex].L.JobDefaults != NULL) &&
        (MPar[J->Req[0]->PtIndex].L.JobDefaults->CPULimit > 0))
      {
      J->CPULimit = MPar[J->Req[0]->PtIndex].L.JobDefaults->CPULimit;
      }
    else if ((MPar[1].L.JobDefaults != NULL) &&
             (MPar[1].L.JobDefaults->CPULimit > 0))
      {
      J->CPULimit = MPar[1].L.JobDefaults->CPULimit;
      }
    }

  /* monitor mode patch for unexpected state changes */

  RQ = J->Req[0];

  if ((MNLIsEmpty(&J->NodeList)) &&
      (!MNLIsEmpty(&RQ->NodeList)))
    {
    /* update job level node list - assumes single req job */

    MNLCopy(&J->NodeList,&RQ->NodeList);
    }

  MJobUpdateResourceCache(J,NULL,0,NULL);

  /* will automatically update J->SIData's file sizes/times */
  
  if (MSDUpdateStageInStatus(J,&DETime) == SUCCESS)
    {
    /* see Josh for details */

    /* TODO: only set SysSMinTime if we know we are only overriding previous DS SysSMinTime changes */

    /*
    if (bmisset(&J->IFlags,mjifDataDependency) && (DETime != MMAX_TIME))
      {
      MJobSetAttr(J,mjaSysSMinTime,(void **)&DETime,mdfLong,mSet);
      }
    */
    }

  if (MJOBISACTIVE(J))
    {
    /* we need to report the "start" and "end" events of a job each iteration
     * in order to properly support the "offset" feature of triggers as they currently
     * stand */

    MOReportEvent(J,J->Name,mxoJob,mttStart,J->StartTime,TRUE);
    MOReportEvent(J,J->Name,mxoJob,mttEnd,J->StartTime + J->SpecWCLimit[0],TRUE);
    }

  /* NOTE: hopefully, this can be done here:
           Update EffQueueDuration based on whether the job was in the idle queue
           during the last iteration */

  MJobAddEffQueueDuration(J,NULL);

  /* check job XFactor with new effective queue duration and if it has passed preemptxfthreshold */

  if ((J->Credential.Q != NULL) &&
      (J->Credential.Q->XFPreemptThreshold > 0) &&
       MJOBISIDLE(J))
    {
    if (J->EffXFactor >= J->Credential.Q->XFPreemptThreshold)
      {
      bmset(&J->Flags,mjfPreemptor);
      }
    }

  bmunset(&J->IFlags,mjifUIQ);
  bmunset(&J->IFlags,mjifNoViolation);

  if (J->Array != NULL)
    {
    MJobArrayUpdate(J);
    }

  if ((MSched.LimitedJobCP == FALSE) && (MSched.EnableHighThroughput == FALSE))
    MJobTransition(J,TRUE,FALSE);

  if ((J->Array != NULL) && bmisset(&J->Flags,mjfArrayMaster))
    {
    int       aindex = 0;

    mbool_t   UnfinishedJobs = FALSE;
 
    /* loop through subjobs to make sure they're
    all completed.  If they are, set the CompletionTime on the master job so
    it will be pruned out of the cache */
  
    for (aindex = 0;aindex < J->Array->Count;aindex++)
      {
      if (J->Array->JPtrs != NULL)
        {
        UnfinishedJobs = TRUE;
        break;
        }
      }
  
    if (!UnfinishedJobs)
      {
      J->CompletionTime = MSched.Time;
      }
    }

  MUFree((char **)tmpTaskList);

  MJobVerifyPolicyStructures(J);

  return(SUCCESS);
  }  /* END MRMJobPostUpdate() */

/* END MRMJobUpdate.c */
