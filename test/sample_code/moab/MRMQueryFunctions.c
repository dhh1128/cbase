/* HEADER */

      
/**
 * @file MRMQueryFunctions.c
 *
 * Contains: RM Query functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param R
 * @param FuncType
 */

mbool_t MRMIsFuncDisabled(

  mrm_t *R,
  enum MRMFuncEnum FuncType)
  
  {
  /* returns FALSE for all non-NATIVE RM's */
  /* returns TRUE only if R is NATIVE and the function is not listed in FnList */

  if ((R->Type == mrmtNative) &&
      (!bmisclear(&R->FnList)) &&
      !bmisset(&R->FnList,FuncType))
    {
    MDB(7,fRM) MLog("INFO:     function %s disabled for rm '%s'\n",
      MRMFuncType[FuncType],
      R->Name);

    return(TRUE);
    }

  return(FALSE);
  }  /* END MRMIsFuncDisabled() */



/**
 * Load updated node/cluster/resource info via RM interface.
 *
 * @param RCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMClusterQuery(

  int                  *RCount, /* O (optional) */
  char                 *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)     /* O (optional) */
 
  {
  int rmindex;
  int rc;

  int tmpRCount;
  int TotalRCount;
  int TotalNRCount;

  int TotalCPUCount = 0;

  mhashiter_t VMIter;

  int nindex;

  mvm_t *VM;
  mnode_t *N;
 
  mrm_t   *R;

  char    *EPtr;

  char     tmpLine[MMAX_LINE];

  enum MNodeStateEnum EState;  /* final effective consensus state computed by examining states reported by each RM */

  /* approach:  first load all nodes via resource management */
  /* then process data in a node centric manner (NYI) */

  /* ie, MRMNodePreX() should be called at node discovery */
  /*     MRMNodePostX() should be called once all RM data for iteration has been processed */

  const char *FName = "MRMClusterQuery";

  MDB(2,fRM) MLog("%s(RCount,EMsg,SC)\n",
    FName);

  MSchedSetActivity(macRMResourceLoad,"starting general cluster load");

  EPtr = (EMsg != NULL) ? EMsg : tmpLine;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (!strcasecmp(R->Name,"internal"))
      continue;

    if (bmisset(&R->Flags,mrmfClient))
      continue;

    if (R->State == mrmsDisabled)
      {
      /* interface is disabled - do not attempt to query, do not attempt to restore */

      continue;
      }

    if (bmisset(&R->RTypes,mrmrtInfo))
      continue;

    if (R->Type == mrmtMoab)
      MSched.PeerConfigured = TRUE;

    if (R->State == mrmsCorrupt)
      {
      /* reset RM state */

      if (R->Type != mrmtMoab)
        {
        /* optimistically retry */

        MRMSetState(R,mrmsActive);
        }
      }

    if (MRMIsFuncDisabled(R,mrmClusterQuery) == TRUE)
      {
      continue;
      }

    if (MRMFunc[R->Type].ClusterQuery == NULL)
      {
      MDB(7,fRM) MLog("ALERT:    cannot load cluster resources on RM (RM '%s' does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmClusterQuery]);
 
      continue;
      }

    EPtr[0] = '\0';

    snprintf(tmpLine,sizeof(tmpLine),"loading cluster info for rm %s",
      R->Name);

    MSchedSetActivity(macRMResourceLoad,tmpLine);

    MRMStartFunc(R,NULL,mrmClusterQuery);

    MUThread(
      (int (*)(void))MRMFunc[R->Type].ClusterQuery,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      4,
      R,
      &tmpRCount,  /* O */
      EPtr,        /* O */
      SC);         /* O */

    MRMEndFunc(R,NULL,mrmClusterQuery,NULL);

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot load cluster resources on RM (RM '%s' failed in function '%s')\n",
        R->Name,
        MRMFuncType[mrmClusterQuery]);

      if (EPtr[0] != '\0')
        {
        MRMSetFailure(R,mrmClusterQuery,mscSysFailure,EPtr);
        }
      else if (rc == mscTimeout)
        {
        MRMSetFailure(R,mrmClusterQuery,mscSysFailure,"timeout");

        /* NOTE:  report clusterquery timeout as trigger failure */

        MOReportEvent((void *)R,R->Name,mxoRM,mttFailure,MSched.Time,TRUE);
        }
      else
        {
        MRMSetFailure(R,mrmClusterQuery,mscSysFailure,"cannot get node info");
        }

      MRMProcessFailedClusterQuery(R,(SC != NULL) ? *SC : mscNoError);

      continue;
      }  /* END if (rc != SUCCESS) */

    R->FirstContact = TRUE;

    if (R->Type != mrmtNative)
      {
      R->UTime = MSched.Time;
      }

    /* set node count and node list */

    if (tmpRCount != -1)
      {
      R->NC = tmpRCount;
      }

    MDB(1,fRM) MLog("INFO:     %d %s resources detected on RM %s\n",
      tmpRCount,
      MRMType[R->Type],
      R->Name);

    if ((R->Type == mrmtNative) && (R->ND.URL[mrmRsvCtl] != NULL))
      {
      mxml_t *DE = NULL;

      char    tmpBuffer[MMAX_BUFFER];

      MRMRsvCtl(NULL,R,mrcmQuery,(void **)tmpBuffer,NULL,SC);
 
      if (MXMLFromString(&DE,tmpBuffer,NULL,NULL) == SUCCESS)
        {
        MMRsvQueryPostLoad(DE,R,TRUE);
        }

      MXMLDestroyE(&DE);
      }
    }  /* END for (rmindex) */

  /* NOTE:  should perform MRMNodePreLoad()/MRMNodePostLoad() activities 
            only once per iteration */

  TotalRCount = 0;
  TotalNRCount = 0;

  /* Iterate the NODE structure and process them */
  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if (N->MIteration != MSched.Iteration)
      continue;

    /* Check for the case of this Node exceeding the CPU license limit */
    if (FAILURE == MLicenseCheckNodeCPULimit(N,TotalCPUCount))
      {
      /* too many cpu's on this system, NO MORE are to be accepted/added */

      /* Clear the Configured resources on this node, but leave it in the table */
      MCResClear(&N->CRes);

      continue;
      }

    /* Add this Node's CPU Count to the SUM of CPUs found so far */
    TotalCPUCount += MNodeGetBaseValue(N,mrlProc);

    /* MNodeProcessRMMsg() moved from MClusterUpdateNodeState() to 
       incorporate information from RMMsg in node state decisions */

    MNodeProcessRMMsg(N);

    if (N->SpecState == mnsNONE)
      {
      MNodeApplyStatePolicy(N,&EState);

      MNodeSetState(N,EState,FALSE);
      }

    if (bmisset(&N->IFlags,mnifIsNew))
      {
      TotalNRCount++;

      MNodePostLoad(N);
      }
    else
      {
      MNodePostUpdate(N);
      }

    TotalRCount++;
    }  /* END for (nindex) */

  /* Iterate over VMs and process them */

  MUHTIterInit(&VMIter);

  while (MUHTIterate(&MSched.VMTable,NULL,(void **)&VM,NULL,&VMIter) == SUCCESS)
    {
    MVMPostUpdate(VM);
    }

  /* All nodes have been 'visited' */

  if (MSched.GN != NULL)
    {
    MNodeTransition(MSched.GN);
    }

  if (MSched.Iteration == 0)
    {
    /* should we call full MNodePostLoad() ?? */

    MNodeUpdateResExpression(MSched.GN,TRUE,TRUE);
    }

  if (TotalNRCount > 0)
    {
    /* new nodes detected */

    int cindex;
            
    mclass_t *C;

    mstring_t tmpHostExp(MMAX_LINE);

    /* Using MCLASS_FIRST_INDEX to skip DEFAULT class */
    for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
      {
      C = &MClass[cindex];
      
      if (C->Name[0] == '\0')
        break;
     
      if (C->HostExp == NULL)
        continue;
   
      /* Nodes weren't available at startup, now update non-re hostlist */

      if (MSched.Iteration == 0)
        MClassSetAttr(C,NULL,mclaHostList,(void **)C->HostExp,mdfString,mSet);

      MStringSet(&tmpHostExp,C->HostExp);
    
      MClassUpdateHostListRE(C,tmpHostExp.c_str());
      }  /* END for (cindex) */
    }    /* END if (NodeAdded == TRUE) */

  if (RCount != NULL)
    *RCount = TotalRCount;
 
  if (TotalRCount == 0)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMClusterQuery() */



/**
 * Process a failed cluster query.
 * 
 * @param RM
 * @param SC
 */

int MRMProcessFailedClusterQuery(

  mrm_t               *RM,
  enum MStatusCodeEnum SC)

  {
  if ((bmisset(&RM->RTypes,mrmrtLicense)) && bmisset(&RM->Flags,mrmfFlushResourcesWhenDown))
    {
    int gindex;
   
    for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
      {
      /* if we failed to query a license RM then we need to 0 out the
         available licenses so that we don't try to start a job when
         we shouldn't */
   
      if (MGRes.Name[gindex][0] == '\0')
        break;
   
      if ((MSNLGetIndexCount(&RM->U.Nat.ResConfigCount,gindex) > 0) &&
          (MGRes.GRes[gindex]->RM == RM))
        {
        /* this license "belongs" to this RM so flush it from the GLOBAL node and this RM's ARes */

        MSNLSetCount(&RM->U.Nat.ResAvailCount,gindex,0);

        MSNLSetCount(&MSched.GN->ARes.GenericRes,gindex,0);
        }
      }  /* END for (gindex) */
    }    /* END if (bmisset(&RM->RTypes,mrmrtLicense)) */

  return(SUCCESS);
  }  /* END MRMProcessFailedClusterQuery() */



 
/**
 * Load updated workload/job/service data via RM interface.
 *
 * @param WCount (O) [optional]
 * @param NWCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMWorkloadQuery(
 
  int                  *WCount,  /* O total jobs reported (optional) */
  int                  *NWCount, /* O new jobs reported (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */
 
  {
  job_iter JTI;

  int rmindex;
  int rc;
 
  int tmpWCount;
  int TotalWCount;
  int TotalNWCount;

  int nindex;
  mnode_t *N;

  int pindex;

  mpar_t *P;
 
  mrm_t  *R;
  mrm_t  *InternalRM;

  mjob_t *J;

  int     tmpNewWC;
  mbool_t NewJob;

  char tmpLine[MMAX_LINE];

  char *EPtr;

  mulong UTime;

  mjob_t *MostRecentJ = NULL;

  const char *FName = "MRMWorkloadQuery";
 
  MDB(2,fRM) MLog("%s(WCount,NWCount,EMsg,SC)\n",
    FName);

  MSchedSetActivity(macRMWorkloadLoad,NULL);

  if (WCount != NULL)
    *WCount = 0;

  if (NWCount != NULL)
    *NWCount = 0;
 
  EPtr = (EMsg != NULL) ? EMsg : tmpLine;

  TotalWCount = 0;
  TotalNWCount = 0;

  /* reset dedicated resources */
 
  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];
 
    if ((N == NULL) || (N->Name[0] == '\0'))
      break;
 
    if (N->Name[0] == '\1')
      continue;
 
    MCResClear(&N->DRes);
    }

  NewJob = FALSE;
 
  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (R->State == mrmsDown)
      continue;

    if (R->State == mrmsDisabled)
      {
      /* interface is disabled - do not attempt to query, do not attempt to restore */

      continue;
      }

    if (bmisset(&R->Flags,mrmfClient))
      continue;

    if (bmisset(&R->Flags,mrmfStatic) && bmisset(&R->Flags,mrmfNoCreateAll))
      {
      /* static RM's loaded later */

      continue;
      }

    if (bmisset(&R->RTypes,mrmrtStorage) && (R->SubType != mrmstMSM))
      {
      /* update data reqs for this storage RM */

      if (MSDUpdateDataReqs(R,EPtr) == FAILURE)
        {
        if (EPtr[0] != '\0')
          MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,EPtr);
        else
          MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,"cannot get data req info");

        continue;
        }
      }

    if (MRMIsFuncDisabled(R,mrmWorkloadQuery) == TRUE)
      {
      continue;
      }
 
    if (MRMFunc[R->Type].WorkloadQuery == NULL)
      {
      MDB(7,fRM) MLog("INFO:     cannot load cluster workload on RM (RM '%s' does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmWorkloadQuery]);
 
      continue;
      }

    UTime = MSched.Time;

    MRMStartFunc(R,NULL,mrmWorkloadQuery);
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].WorkloadQuery,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      5,
      R,
      &tmpWCount,
      &tmpNewWC,
      EPtr, /* O */
      SC);

    MRMEndFunc(R,NULL,mrmWorkloadQuery,NULL);
 
    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot load cluster workload on RM (RM '%s' failed in function '%s')\n",
        R->Name,
        MRMFuncType[mrmWorkloadQuery]);

      if (EPtr[0] != '\0')
        MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,EPtr);
      else if (rc == mscTimeout)
        MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,"timeout");
      else
        MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,"cannot get workload info");
 
      continue;
      }

    if (tmpWCount == -1)
      {
      /* request is still pending - get info from last iteration */

      tmpWCount = R->JobCount;
      }
    else
      {
      /* request successful - set MR statistic */

      R->JobCount = tmpWCount;
      }

    if (tmpNewWC == -1)
      {
      tmpNewWC = R->JobNewCount;
      
      /* reset for next iteration */

      R->JobNewCount = 0;
      }

    R->WorkloadUpdateTime = UTime;
 
    MDB(1,fRM) MLog("INFO:     %d %s jobs detected on RM %s\n",
      tmpWCount,
      MRMType[R->Type],
      R->Name);

    if (tmpNewWC > 0)
      NewJob = TRUE;

    if (!bmisset(&R->Flags,mrmfNoCreateAll)) 
      {
      TotalWCount  += tmpWCount;
      TotalNWCount += tmpNewWC;
      }
    }    /* END for (rmindex) */

  if (NewJob == TRUE)
    {
    /* load information from one-time, static rm's */

    for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
      {
      R = &MRM[rmindex];

      if (R->Name[0] == '\0')
        break;

      if (MRMIsReal(R) == FALSE)
        continue;

      if (bmisset(&R->Flags,mrmfClient))
        continue;

      if (!bmisset(&R->Flags,mrmfStatic) || !bmisset(&R->Flags,mrmfNoCreateAll))
        {
        /* non-static slave RM's loaded earlier */

        continue;
        }

      if (MRMIsFuncDisabled(R,mrmWorkloadQuery) == TRUE)
        {
        continue;
        }
   
      if (MRMFunc[R->Type].WorkloadQuery == NULL)
        {
        MDB(7,fRM) MLog("INFO:     cannot load cluster workload on RM (RM '%s' does not support function '%s')\n",
          R->Name,
          MRMFuncType[mrmWorkloadQuery]);

        continue;
        }

      MRMStartFunc(R,NULL,mrmWorkloadQuery);

      MUThread(
        (int (*)(void))MRMFunc[R->Type].WorkloadQuery,
        R->P[0].Timeout,
        &rc,         /* O */
        NULL,
        R,
        5,
        R,
        &tmpWCount,  /* O */
        &tmpNewWC,   /* O */
        EPtr,
        SC);

      MRMEndFunc(R,NULL,mrmWorkloadQuery,NULL);

      if (rc != SUCCESS)
        {
        MDB(3,fRM) MLog("ALERT:    cannot load cluster workload on RM (RM '%s' failed in function '%s')\n",
          R->Name,
          MRMFuncType[mrmWorkloadQuery]);

        if (EPtr[0] != '\0')
          MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,EPtr);
        else if (rc == mscTimeout)
          MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,"timeout");
        else
          MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,"cannot get workload info");

        continue;
        }

      MDB(1,fRM) MLog("INFO:     %d %s jobs detected on RM %s\n",
        tmpWCount,
        MRMType[R->Type],
        R->Name);
      }       /* END for (rmindex) */
    }         /* END if (NewJob == TRUE) */

  /* all per-RM information loaded */

  for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
    {
    R = &MRM[rmindex];

    if (R == NULL)
      break;

    if ((R != NULL) &&
        (R->Name[0] != '\0') &&
        (R->Name[0] != '\1'))
      {
      continue;
      } 

    R->BackLog = 0;
    }

  MRMGetInternal(&InternalRM);

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    /* Save link pointer, because we need a valid point if the job gets deleted that's not stored within the freed memory. */

    /* Check to see if we need to adjust the RM JobCounter after a moab start 
     * to avoid duplicate JObIDs due to a lost or courrupted checkpoint file. */

    if ((MSched.Iteration == 0) && 
        (InternalRM != NULL) &&
        (InternalRM->JobCounter == MSched.MinJobCounter) &&
        (InternalRM->JobIDFormatIsInteger == TRUE))
      {
      /* Moab just started and we should have already read in the moab.ck file. If the RM job counter 
       * is still at the starting job id then odds are we did not read in any jobs for this RM from the checkpoint file.
       * The RM reported at least one job so either there were no jobs for the RM in the checkpoint file, or the checkpoint
       * file was lost or corrupted. Find the most recently submitted job which should have the job id of where
       * we left off and asjust the RM JobCounter */  

      int JobID = 0;
      int MostRecentJobID = 0; 

      MUGetJobIDFromJobName(J->Name, &JobID);

      if (MostRecentJ != NULL)
        MUGetJobIDFromJobName(MostRecentJ->Name, &MostRecentJobID);

      if (((MostRecentJ == NULL) || (J->SubmitTime > MostRecentJ->SubmitTime)) ||
          ((J->SubmitTime == MostRecentJ->SubmitTime) && (JobID > MostRecentJobID)))
        {
        /* wrap around for multiple jobs in the same second NYI */
        MostRecentJ = J;
        }

      } /* END if (MSched.Iteration == 0) etc. */

    if ((J->CancelCount > 0) &&
        (MSched.ResendCancelCommand == TRUE) &&
        (J->MigrateCount > 0))
      {
      /* Job has been told to cancel.  If it is a migrated job, resend the
       * cancel command.  This is because the cancel command can sometimes be
       * lost on heavy traffic systems. */

      MDB(3,fSCHED) MLog("INFO:    job '%s' has not canceled yet, will be force-canceled\n",
        J->Name);

      bmset(&J->IFlags,mjifRequestForceCancel);

      MJobCancel(J,NULL,FALSE,NULL,NULL);
      }

    if (bmisset(&J->IFlags,mjifCancel))
      {
      if (MJobGetBlockMsg(J,NULL) != NULL)
        MJobCancel(J,MJobGetBlockMsg(J,NULL),FALSE,NULL,NULL);
      else
        MJobCancel(J,"job was rejected",FALSE,NULL,NULL);

      continue;
      }

    if (MSched.Iteration == 0)
      MJobSynchronizeDependencies(J);

    if (bmisset(&J->Flags,mjfArrayMaster))
      {
      /* update job arrays active counts after all rm jobs have been loaded. */
      MJobArrayUpdate(J);
      }

    if (J->State == mjsRunning)
      continue;

    /* update Partition->BackLog */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];

      if (P == NULL)
        break;

      if (P->Name[0] == '\1')
        continue;

      if (P->Name[0] == '\0')
        break;

      if (bmisset(&J->PAL,P->Index) == FALSE)
        continue;

      if (P->RM == NULL)
        continue;

      P->RM->BackLog += J->SpecWCLimit[0];
      }
    }  /* END for (J) */

  /* If we have lost our .moab.ck file then check to see if there are any 
   * completed jobs with a more recent job id */

  if ((MSched.Iteration == 0) &&
      (InternalRM != NULL) &&
      (InternalRM->JobCounter == MSched.MinJobCounter) &&
      (InternalRM->JobIDFormatIsInteger == TRUE))
    {
    job_iter JTI;

    MCJobIterInit(&JTI);

    while (MCJobTableIterate(&JTI,&J) == SUCCESS)
      {
      int JobID = 0;
      int MostRecentJobID = 0;

      MUGetJobIDFromJobName(J->Name, &JobID);

      if (MostRecentJ != NULL)
        MUGetJobIDFromJobName(MostRecentJ->Name, &MostRecentJobID);

      if (((MostRecentJ == NULL) || (J->SubmitTime > MostRecentJ->SubmitTime)) ||
          ((J->SubmitTime == MostRecentJ->SubmitTime) && (JobID > MostRecentJobID)))
        {
        /* wrap around for multiple jobs in the same second NYI */
        MostRecentJ = J;
        }
      }  /* END while () */
    } /* END if (MSched.Iteration == 0) etc. */

  if (MostRecentJ != NULL)
    {
    /* Setting the job counter based on workload query */

    int CurrentJobCounter = InternalRM->JobCounter;

    MUGetJobIDFromJobName(MostRecentJ->Name, &InternalRM->JobCounter);
    MRMJobCounterIncrement(InternalRM);

    MDB(3,fRM) MLog("INFO:     current job counter %d set to %d from WorkloadQuery\n",
      CurrentJobCounter,
      InternalRM->JobCounter); 
    }

  if (WCount != NULL)
    *WCount = TotalWCount;

  if (NWCount != NULL)
    *NWCount = TotalNWCount;

  if (TotalWCount == 0)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMWorkloadQuery() */





/**
 * Load updated class/queue info via RM interface.
 *
 * @param QCount (O)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMQueueQuery(

  int                  *QCount, /* O */
  char                 *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)     /* O (optional) */

  {
  int rmindex;
  int rc;

  int tmpQCount;
  int TotalQCount;

  char  *EPtr;
  char   tEMsg[MMAX_LINE];

  mrm_t *R;

  const char *FName = "MRMQueueQuery";

  MDB(2,fRM) MLog("%s(QCount,EMsg,SC)\n",
    FName);

  EPtr = (EMsg != NULL) ? EMsg : tEMsg;

  TotalQCount = 0;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (R->State == mrmsDown)
      continue;

    if (bmisset(&R->Flags,mrmfClient))
      continue;

    if (MRMFunc[R->Type].QueueQuery == NULL)
      {
      MDB(7,fRM) MLog("INFO:     cannot load queue info on RM (RM '%s' does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmQueueQuery]);

      continue;
      }

    if (MRMIsFuncDisabled(R,mrmQueueQuery) == TRUE)
      {
      continue;
      }

    MRMStartFunc(R,NULL,mrmQueueQuery);

    MUThread(
      (int (*)(void))MRMFunc[R->Type].QueueQuery,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      4,
      R,
      &tmpQCount,
      EPtr,
      SC);

    MRMEndFunc(R,NULL,mrmQueueQuery,NULL);

    if (rc != SUCCESS)
      {
      char tmpLine[MMAX_LINE];

      MDB(3,fRM) MLog("ALERT:    cannot load cluster queue on RM (RM '%s' failed in function '%s')\n",
        R->Name,
        MRMFuncType[mrmQueueQuery]);

      if ((EPtr != NULL) && (EPtr[0] != '\0'))
        {
        snprintf(tmpLine,sizeof(tmpLine),"cannot get queue info - %s",
          EPtr);

        MRMSetFailure(R,mrmQueueQuery,mscSysFailure,tmpLine);
        }
      else if (rc == mscTimeout)
        {
        MRMSetFailure(R,mrmQueueQuery,mscSysFailure,"timeout");
        }
      else
        {
        MRMSetFailure(R,mrmQueueQuery,mscSysFailure,"cannot get queue info");
        }

      continue;
      }

    MDB(1,fRM) MLog("INFO:     %d %s classes/queues detected on RM %s\n",
      tmpQCount,
      MRMType[R->Type],
      R->Name);

    TotalQCount += tmpQCount;
    }    /* END for (rmindex) */

  if (QCount != NULL)
    *QCount = TotalQCount;

  if (TotalQCount == 0)
    {
    MDB(5,fRM) MLog("INFO:     no classes/queues detected\n");

    /* NOTE:  classes/queues are optional */
    }

  return(SUCCESS);
  }  /* END MRMQueueQuery() */





/**
 * Register request for subsequent info queries.
 *
 * @param SR (I) [optional - if not specified, attach to first native RM]
 * @param Attribute (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMInfoQueryRegister(

  mrm_t                *SR,        /* I (optional - if not specified, attach to first native RM) */
  char                 *Attribute, /* I */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */
  
  {
  mrm_t *R;

  const char *FName = "MRMInfoQueryRegister";

  MDB(2,fRM) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (SR != NULL) ? SR->Name : "NULL",
    (Attribute != NULL) ? Attribute : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (Attribute == NULL)
    {
    /* nothing to register */

    return(SUCCESS);
    }

  if (SR == NULL)
    {
    int rmindex;

    R = NULL;

    /* resource manager not located, find first native RM which supports infoquery */

    for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
      {
      if (MRM[rmindex].Name[0] == '\0')
        break;

      if (MRM[rmindex].Type != mrmtNative)
        continue;

      if (!bmisset(&MRM[rmindex].FnList,mrmInfoQuery))
        continue;

      /* located valid RM */

      R = &MRM[rmindex];

      break;
      }  /* END for (rmindex) */

    if (R == NULL)
      {
      return(FAILURE);
      }
    }
  else
    {
    R = SR;
    }

  MUStrAppend(&R->IQBuffer,NULL,Attribute,',');

  return(SUCCESS);
  }  /* END MRMInfoQueryRegister() */





/**
 * perform general info query via RM API
 *
 * @param R (I)
 * @param QType (I) [optional]
 * @param Attribute (I)
 * @param Value (O) [minsize=MMAX_LINE]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMInfoQuery(

  mrm_t                *R,         /* I */
  const char           *QType,     /* I (optional) */
  char                 *Attribute, /* I */
  char                 *Value,     /* O (minsize=MMAX_LINE) */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  int rc;

  const char *FName = "MRMInfoQuery";

  MDB(2,fRM) MLog("%s(%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (QType != NULL) ? QType : "NULL",
    (Attribute != NULL) ? Attribute : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((R == NULL) || (Attribute == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  if (MRMFunc[R->Type].InfoQuery == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot complete info query on RM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmInfoQuery]);

    return(FAILURE);
    }

  MRMStartFunc(R,NULL,mrmInfoQuery);

  MUThread(
    (int (*)(void))MRMFunc[R->Type].InfoQuery,
    R->P[0].Timeout,
    &rc,
    NULL,
    R,
    6,
    R,
    QType,
    Attribute,
    Value,
    EMsg,  /* O */
    SC);

  MRMEndFunc(R,NULL,mrmInfoQuery,NULL);

  if (rc != SUCCESS)
    {
    MDB(3,fRM) MLog("ALERT:    cannot complete info query on RM (RM '%s' failed in function '%s')\n",
      R->Name,
      MRMFuncType[mrmInfoQuery]);

    if (rc == mscTimeout)
      MRMSetFailure(R,mrmClusterQuery,mscSysFailure,"timeout");
    else
      MRMSetFailure(R,mrmInfoQuery,mscSysFailure,"cannot query info");

    return(FAILURE);
    }

  MDB(5,fRM) MLog("INFO:     value received for query qtype=%s attr=%s on RM %s (value: '%.64s')\n",
    (QType != NULL) ? QType : "NULL",
    Attribute,
    R->Name,
    Value);

  return(SUCCESS);
  }  /* END MRMInfoQuery() */





/**
 * Query system info via RM interface.
 *
 * WARNING: structures may be cast to char * for this routine
 *          QType, Attribute, or Value may contain non-printable characters
 *
 * @param R (I)
 * @param QType (I) [optional]
 * @param Attribute (I)
 * @param IsBlocking (I)
 * @param Value (O) [minsize=MMAX_BUFFER]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMSystemQuery(

  mrm_t                *R,
  const char           *QType,
  const char           *Attribute,
  mbool_t               IsBlocking,
  char                 *Value,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  int rc;

  const char *FName = "MRMSystemQuery";

  MDB(2,fRM) MLog("%s(%s,%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (QType != NULL) ? "QType" : "NULL",
    (Attribute != NULL) ? "Attribute" : "NULL",
    MBool[IsBlocking]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (Value != NULL)
    Value[0] = '\0';

  if ((R == NULL) || (Attribute == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  if (MRMFunc[R->Type].SystemQuery == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot complete system query on RM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmSystemQuery]);

    return(FAILURE);
    }

  MRMStartFunc(R,NULL,mrmSystemQuery);

  MUThread(
    (int (*)(void))MRMFunc[R->Type].SystemQuery,
    R->P[0].Timeout,
    &rc,
    NULL,
    R,
    6,
    R,
    QType,
    Attribute,
    IsBlocking,
    Value,
    EMsg,
    SC);

  MRMEndFunc(R,NULL,mrmSystemQuery,NULL);

  if (rc != SUCCESS)
    {
    MDB(3,fRM) MLog("ALERT:    cannot complete system query on RM (RM '%s' failed in function '%s')\n",
      R->Name,
      MRMFuncType[mrmSystemQuery]);

    if (rc == mscTimeout)
      MRMSetFailure(R,mrmSystemQuery,mscSysFailure,"timeout");
    else
      MRMSetFailure(R,mrmSystemQuery,mscSysFailure,"cannot query system");

    return(FAILURE);
    }

#ifdef MNOT
  /* because non-printable characters can be casted for this routine (MSLURMJobWillRun) we cannot print out these values */

  MDB(5,fRM) MLog("INFO:     value received for query qtype=%s attr=%s on RM %s (value: '%.64s')\n",
    (QType != NULL) ? QType : "NULL",
    Attribute,
    R->Name,
    Value);
#endif /* MNOT */

  return(SUCCESS);
  }  /* END MRMSystemQuery() */
/* END MRMQueryFunctions.c */
