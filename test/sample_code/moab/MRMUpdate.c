/* HEADER */

      
/**
 * @file MRMUpdate.c
 *
 * Contains: MRMUpdate
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Function to check for and act on stale VMs (VMs that have not been reported
 *  for MSched.VMStaleIterations).
 *
 * Stale VMs will be acted on according to MSched.VMStaleAction.
 */

void MRMCheckStaleVMs()
  {
  if (MSched.ManageTransparentVMs == TRUE)
    {
    mvm_t *V;

    /* reset usage across all VM's */

    mhashiter_t HTI;
    MUHTIterInit(&HTI);

    while (MUHTIterate(&MSched.VMTable,NULL,(void **)&V,NULL,&HTI) == SUCCESS)
      {
      if (V != NULL)
        {
        if (V->JobID[0] == '\0')
          {
          /* VM is not dedicated - assume available for use until job is
             detected which is actively using VM resources 
          */

          if (V->State == mnsActive)
            V->State = mnsIdle;
          }

        if (bmisset(&V->Flags,mvmfDestroyed) && (MSched.Iteration - V->UpdateIteration > 1))
          {
          /* VM not reported last iteration */

          MVMComplete(V);

          continue;
          }

        if (bmisset(&V->IFlags,mvmifUnreported) &&
            (MSched.Iteration - V->UpdateIteration > MSched.VMStaleIterations))
          {
          /* VM has been unreported for too long - just log this fact
           * DO NOT DELETE ANY VMS--this is too risky in production systems!!! */

          MDB(1,fSTRUCT) MLog("INFO:     VM '%s' has not been reported in %d iterations\n",
            V->VMID,
            MSched.Iteration - V->UpdateIteration);

          switch (MSched.VMStaleAction)
            {
            case mvmsaDestroy:

              if (MSched.EnableVMDestroy == TRUE)
                {
                if (V->TrackingJ != NULL)
                  {
                  /* Cancelling the VMTracking job will take care of removing the VM */

                  MJobCancel(V->TrackingJ,NULL,FALSE,NULL,NULL);
                  }
                else
                  {
                  /* ? */
                  }
                }  /* END if (MSched.EnableVMDestroy == TRUE) */

              break;

            case mvmsaCancelTracking:

              /* Remove VMTracking job, but not the VM */

              if (V->TrackingJ != NULL)
                {
                int tmpRC = 0;

                tmpRC = MRMJobCancel(V->TrackingJ,NULL,FALSE,NULL,NULL);

                if (tmpRC == SUCCESS)
                  V->TrackingJ = NULL;
                }

              break;

            case mvmsaIgnore:
            default:

              /* Ignore */
 
              break;
            }  /* END switch (MSched.VMStaleAction) */

          continue;
          } /* END if (bmisset(&V->IFlags,mvmifUnreported) ... */

        V->ARes.Procs = V->CRes.Procs; 
        }  /* END if (V != NULL) */
      }    /* END while (MUHTIterate() == SUCCESS) */

    MUHTIterInit(&HTI);

    /* Purge old completed VMs */

    while (MUHTIterate(&MSched.VMCompletedTable,NULL,(void **)&V,NULL,&HTI) == SUCCESS)
      {
      if ((V != NULL) && (MSched.Time - V->UpdateTime > (mulong)MSched.VMCPurgeTime))
        {
        MVMDestroy(&V,NULL);

        /* Reset the iterator because we just changed the hash table */

        MUHTIterInit(&HTI);
        }
      }  /* END while (MUHTIterate(&MSched.VMCompletedTable,...) == SUCCESS) */
    }  /* END if (MSched.ManageTransparentVMs == TRUE) */
  }  /* END MRMCheckStaleVMs() */


/**
 * Reset stats, update queue, workload, and cluster info.
 *
 * @see MSchedProcessJobs() - parent
 * @see MClusterClearUsage() - child
 */
 
int MRMUpdate(void)
 
  {
  job_iter JTI;

  mjob_t  *J;

  int JobCount;
  int NodeCount;
  int QueueCount;

  mrm_t *R;
  int    rmindex;
  int    nindex;

  int    qindex;

  char   EMsg[MMAX_LINE];
  char   tmpLine[MMAX_LINE];
  enum MStatusCodeEnum SC;

  mbool_t RMIsNew = FALSE;  /* RM has been newly initializated (i.e. successfully ran MRMInitialize() this iteration) */

  const char *FName = "MRMUpdate";

  MDB(2,fRM) MLog("%s()\n",
    FName);

  MUGetMS(NULL,&MSched.LoadStartTime[mschpRMProcess]);

  MSched.LoadStartTime[mschpRMLoad] = 0;
  MSched.LoadEndTime[mschpRMLoad] = 0;

  MSched.LoadStartTime[mschpRMAction] = 0;
  MSched.LoadEndTime[mschpRMAction] = 0;

  MSched.ActiveLocalRMCount = 0;

  MSched.MinJobAName[0] = '\0';

  MStat.ActiveJobs = 0;

  MSysRecordEvent(
    mxoSched,
    MSched.Name,
    mrelRMPollStart,
    NULL,
    NULL,
    NULL);

  MRMCheckStaleVMs();

  if ((MSched.Mode == msmTest) || (MSched.Mode == msmMonitor))
    {
    /* clear message buffer which will hold this iteration's scheduling actions */

    if (MSched.TestModeBuffer != NULL)
      {
      MUSNInit(
        &MSched.TBPtr,
        &MSched.TBSpace,
        MSched.TestModeBuffer,
        MSched.TestModeBufferSize);
      }
    }

  /* new RM cycle - reset data state */

  memset(MSched.QOSPreempteeCount,0,sizeof(MSched.QOSPreempteeCount));

  for (qindex = 0;qindex < MSched.M[mxoQOS];qindex++)
    {
    if (MQOS[qindex].Name[0] == '\0')
      break;

    if (MQOS[qindex].Name[0] == '\1')
      continue;

    MQOS[qindex].Backlog = 0;
    MQOS[qindex].MaxQT = 0;
    MQOS[qindex].MaxXF = 0;
    }  /* END for (qindex) */
 
  if ((MSched.DefStorageRM != NULL) && (!MNLIsEmpty(&MSched.DefStorageRM->NL)))
    {
    mnode_t *N;

    int dsindex;

    MNLGetNodeAtIndex(&MSched.DefStorageRM->NL,0,&N);
   
    if (N != NULL)
      { 
      N->DRes.Disk = 0;

      dsindex = MUMAGetIndex(meGRes,"dsop",mVerify);
      if (dsindex > 0)
        {
        MSNLSetCount(&N->DRes.GenericRes,dsindex,0);
        }
      }
    }  /* END if (MSched.DefStorageRM != NULL) */
 
  /* clear runtime statistics */

  MStat.EligibleJobs = 0;

  MUHTClear(&MAQ,FALSE,NULL);

  MClusterClearUsage();

  /* load resource data */

  if (MSched.Iteration == 0) 
    {
    MSched.InitialLoad = TRUE;
    }

  /* establish/restore RM interfaces */

  MRMRestore();

  /* perform per-cluster query node processing, variable resetting, etc */

  if (MSched.ManageTransparentVMs == TRUE)
    {
    for (nindex = 0;MNode[nindex] != NULL;nindex++)
      {
      /* Clear hypervisor status on each node */

      mnode_t *N = MNode[nindex];

      if (N == (mnode_t *)1)
        continue;

      if (N->Name[0] == 0)
        continue;

      N->HVType = mhvtNONE;

      MNodeFreeOSList(&N->VMOSList,MMAX_NODEOSTYPE);
      }
    }

  /* check for pending information service requests */

  /* NOTE:  To avoid race conditions, should process all master RM's first
            followed by RM's with mrmfNoCreateAll or mrmfNoCreateResource
            (NYI) */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((R->State == mrmsDown) && (R->State == mrmsCorrupt))
      continue;

    if (R->IQBuffer == NULL)
      continue;

    /* submit information query register request */

    if (MRMInfoQuery(
          R,
          "register",
          R->IQBuffer,
          tmpLine,  /* O */
          EMsg,
          &SC) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      MDB(1,fALL) MLog("ALERT:    cannot register query request with RM '%s'\n",
        R->Name);

      snprintf(tmpLine,sizeof(tmpLine),"cannot register query request '%.16s...' with RM %s - %s",
        R->IQBuffer,
        R->Name,
        EMsg);

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
      }

    MUFree(&R->IQBuffer);
    }  /* END for (rmindex) */
 
  MRMPreClusterQuery();

  /* load RM node information */

  if (MRMClusterQuery(&NodeCount,NULL,NULL) == SUCCESS)
    {
    MDB(1,fRM) MLog("INFO:     resources detected: %d\n",
      NodeCount);

    if (MSched.Iteration == 0)
      MStatPInitialize(NULL,FALSE,msoNode);
    }
  else
    {
    MDB(1,fRM) MLog("WARNING:  no resources detected\n");
    }

  /* load queue data */

  if (MRMQueueQuery(&QueueCount,NULL,NULL) == SUCCESS)
    {
    MDB(1,fALL) MLog("INFO:     queues detected: %d\n",
      QueueCount);
    }
  else
    {
    MDB(1,fALL) MLog("INFO:     no queues detected\n");
    }

  MRMCheckDefaultClass();

  /* check for any newly init'd RM's */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((R->State == mrmsDown) && (R->State == mrmsCorrupt))
      continue;

    if (R->InitIteration == MSched.Iteration)
      {
      RMIsNew = TRUE;  /* this RM was just newly initialized! */
      }

    if (MSched.ProvRM == NULL)
      {
      /* provisioning RM not specified, evaluate RM for use as default */

      if ((R->InitIteration == MSched.Iteration) || (MSched.Iteration <= 0))
        {
        if ((R->ND.Path[mrmJobMigrate] != NULL) &&
            (R->ND.Path[mrmJobMigrate][0] != '\0'))
          {
          /* provisioning RM not explicitly specified and this RM provides
             provisioning services,  use this RM as default ProvRM */

          MSched.ProvRM = R;

          MDB(4,fALL) MLog("INFO:     RM '%s' selected as default provisioning RM\n",
            R->Name);
          }
        }
      }
    }  /* END for (rmindex) */

  if ((MSched.Iteration <= 0) || (RMIsNew == TRUE))
    {
    /* need to update partition information because Moab just started up OR
     * a RM's resources may have changed due to a new RM initialization */

    /* NOTE:  basic cluster info required for job evaluation */

    MParUpdate(&MPar[0],TRUE,FALSE);

    if ((bmisset(&MSched.Flags,mschedfFastRsvStartup) == TRUE) && 
        (MSched.Iteration == 0))
      {
      mrsv_t *R;

      rsv_iter RTI;

      /* Now that all nodes have been loaded, update checkpointed reservations. */

      MRsvIterInit(&RTI);

      while (MRsvTableIterate(&RTI,&R) == SUCCESS)
        {
        MRsvPopulateNL(R);
        }
      }

    /* load completed job info after node info loaded to minimize race 
       conditions associated with node discovery */

    MCPLoad(MCP.CPFileName,mckptCJob);

    if ((MSched.NodeIDBase[0] == '\0') && (MNode[0] != NULL))
      {
      if (MNode[0]->Name[0] > '\1')
        {
        /* store node id base */

        int cindex;

        for (cindex = 0;cindex < MMAX_NAME;cindex++)
          {
          if (!isalpha(MNode[0]->Name[cindex]))
            break;

          MSched.NodeIDBase[cindex] = MNode[0]->Name[cindex];
          }

        MSched.NodeIDBase[MIN(cindex,MMAX_NAME - 1)] = '\0'; 
        } 
      }
    }    /* if ((MSched.Iteration <= 0) || (RMIsNew == TRUE)) */

  if (MSched.Mode == msmSim)
    {
    MSimGetWorkload();
    }

  /* load workload data */

  if (MRMWorkloadQuery(&JobCount,NULL,NULL,NULL) == SUCCESS)
    {
    MDB(1,fALL) MLog("INFO:     jobs detected: %d\n",
      JobCount);

    MSched.TotalJobQueryCount += JobCount;
    }
  else
    {
    MDB(1,fALL) MLog("INFO:     no workload reported by any RM\n");
    }

 /* Array master jobs are checkpointed with it's members indecies but the
  * indecies can't be relied on being the same after restarting so they 
  * must be updated after a restart. */

  if (MSched.Iteration == 0)
    MJobUpdateJobArrayIndices();

  MSched.InitialLoad = FALSE;

  MStatClearUsage(mxoNode,TRUE,TRUE,FALSE,FALSE,FALSE);

  MClusterUpdateNodeState(); 
 
  /* all node/resource and all job info loaded - update statistics */

  /* modify dynamic partitions before starting/scheduling new jobs */

  /* NOTE:  sys provisioning should not happen until after standing 
            reservations are instantiated in case rsv tracking is
            activated */

  MParUpdate(&MPar[0],TRUE,TRUE);

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (MSched.Iteration == J->UIteration) 
      {
      /* job is not stale - was updated this iteration */

      continue;
      }

    /* job not recently updated */

    /* handle potential RM load delays */

    if (MJOBISACTIVE(J) == FALSE)
      {
      if (J->AName != NULL)
        {
        if ((MSched.MinJobAName[0] == '\0') ||
            (strcmp(J->AName,MSched.MinJobAName) < 0))
          {
          MUStrCpy(MSched.MinJobAName,J->AName,sizeof(MSched.MinJobAName));
          }
        }
          
      continue;
      }

    /* add 'stale' active jobs to MAQ[] */

    MQueueAddAJob(J);
    }    /* END for (J) */

  /* sort active job table */

#if 0 /* sorting of MAQ disabled because of the cache */
  qsort(
    (void *)&MAQ[0],
    MAQLastIndex,
    sizeof(int),
    (int(*)(const void *,const void *))MJobCTimeComp);
#endif

  MDB(4,fRM)
    {
    MDB(4,fRM) MLog("INFO:     jobs in queue\n");
 
    MJobIterInit(&JTI);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      MJobShow(J,0,NULL);
      }  /* END for (J) */
    }

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (bmisset(&R->RTypes,mrmrtLicense))
      {
      int ARes;
      int DRes;
      int CRes;

      if (R->U.Nat.N != NULL)
        {
        int lindex;
        mnode_t *GN;

        GN = (mnode_t *)R->U.Nat.N;

        for (lindex = 1;lindex < MSched.M[mxoxGRes];lindex++)
          {
          if (MGRes.Name[lindex][0] == '\0')
            break;

          if (MSNLGetIndexCount(&R->U.Nat.ResConfigCount,lindex) == 0)
            continue;

          ARes = MSNLGetIndexCount(&GN->ARes.GenericRes,lindex);
          CRes = MSNLGetIndexCount(&GN->CRes.GenericRes,lindex);
          DRes = MSNLGetIndexCount(&GN->DRes.GenericRes,lindex);


          MDB(3,fSCHED)  MLog("INFO:     License %-16s  %3d of %3d available  (Idle: %.2f%%  Active: %.2f%%)\n",
            R->U.Nat.ResName[lindex],
            (ARes - DRes < 0) ? 0 : (ARes - DRes),
            CRes,
            CRes == 0 ? 0 : ((double)ARes / CRes * 100.0),
            CRes == 0 ? 0 : ((double)(CRes - ARes) / CRes * 100.0));
          }  /* END for (lindex) */
        }
      }

    if ((R->State != mrmsDown) && (R->State != mrmsCorrupt))
      continue;

    if (R->StateMTime + R->TrigFailDuration > MSched.Time)
      {
      MOReportEvent((void *)R,R->Name,mxoRM,mttFailure,MSched.Time,FALSE);
      }
    }    /* END for (rmindex) */

  MUGetMS(NULL,&MSched.LoadEndTime[mschpRMProcess]);

  MSysRecordEvent(
    mxoSched,
    MSched.Name,
    mrelRMPollEnd,
    NULL,
    NULL,
    NULL);
 
#ifdef MCACHEDEBUG
    MCacheDumpNodes();
#endif /* MCACHEDEBUG */

  return(SUCCESS);
  }  /* END MRMUpdate() */
/* END MRMUpdate.c */
