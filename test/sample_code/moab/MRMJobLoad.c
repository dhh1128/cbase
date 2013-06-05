/* HEADER */

      
/**
 * @file MRMJobLoad.c
 *
 * Contains: MRMJobPreLoad and MRMJobPostLoad
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "mcom.h"  
#include "MEventLog.h"

/**
 * Perform general job initialization for any RM. 
 *
 * NOTE:  Will allocate J->TaskMap and set state to Idle, but will not allocate J->NodeList (see MJobCreate) and will not allocate J->Req[0]
 *
 * @param J (I) [modified]
 * @param JobName (I)
 * @param R (I) [optional]
 */

int MRMJobPreLoad(

  mjob_t     *J,
  const char *JobName,
  mrm_t      *R)

  {
  char EMsg[MMAX_LINE];

  const char *FName = "MRMJobPreLoad";

  MDB(5,fRM) MLog("%s(J,%s,%s)\n",
    FName,
    (JobName != NULL) ? JobName : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  MSched.EnvChanged = TRUE;

  if (R != NULL)
    J->SubmitRM = R;

  if (JobName != NULL)
    MUStrCpy(J->Name,JobName,sizeof(J->Name));

  J->CTime = MSched.Time;
  J->MTime = MSched.Time;
  J->ATime = MSched.Time;
  J->UIteration = MSched.Iteration;

  J->EffQueueDuration = -1;

  bmset(&J->IFlags,mjifIsNew);

  /* set defaults */

  J->CMaxDate       = MMAX_TIME;
  J->TermTime       = MMAX_TIME;
  bmclear(&J->Hold);
  J->SystemPrio     = 0;

  J->BypassCount    = 0;

  J->AWallTime      = 0;
  J->SWallTime      = 0;

  J->Rsv              = NULL;

  MDB(7,fCONFIG) MLog("INFO:     job %s SpecPAL (pmask) initialized to ALL in job preload\n",
    J->Name);

  bmsetall(&J->SpecPAL,MMAX_PAR);
  bmsetall(&J->SysPAL,MMAX_PAR);
  bmsetall(&J->PAL,MMAX_PAR);

  bmset(&J->IFlags,mjifShouldCheckpoint);
  
  bmset(&J->IFlags,mjifTaskCountIsDefault);

  J->Request.TC     = 1;

  J->State          = mjsIdle;

  J->SpecWCLimit[1] = 0;

  J->QOSRequested           = NULL;

  MNLClear(&J->NodeList);

  if (J->Req[0] == NULL)
    {
    /* add resource requirements information */

    if (MReqCreate(J,NULL,&J->Req[0],FALSE) == FAILURE)
      {
      MDB(1,fPBS) MLog("ALERT:    cannot add requirements to job '%s'\n",
        J->Name);

      return(FAILURE);
      }

    MRMReqPreLoad(J->Req[0],R);
    }

  /* Step 2.3  Load/Apply Checkpointed Job Status (QOS/Hold/SPrio/Bypass) */

  /* NOTE:  query the checkpoint file IF:
            First Iteration (always) OR
            Down RM has recovered and reports job */

  if (((MSched.Iteration == 0) ||
       (J->SubmitTime < MCP.LastCPTime)) &&
      (MRMLoadJobCP(R,J,EMsg) == FAILURE))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMJobPreLoad() */





/**
 * Called after the job is seen for the first time.
 *
 * NOTE:  handling of tasklist information
 *        TaskList[] contains updated task ordering information
 *        must update both J->NodeList and J->Req[X]->NodeList
 *        this call should override, not add to existing info
 *
 * @see MRMLoadJobCP() - child
 * @see MRMWorkloadQuery() - 'grandparent'
 *
 * Step 1.0  Initialize 
 * Step 2.0  Adapt Job Based on Learned Attributes and System Attributes 
 *   Step 2.2  Apply System Attributes 
 *   Step 2.4  Populate Missing Attributes 
 *   Step 2.5  Validate Job 
 *     Step 2.5.1  Check External Validation Interface 
 *     Step 2.5.2  Perform Pre-Template Internal Job Validation 
 *   Step 2.6  Adjust Job Credentials 
 *   Step 2.7  Apply Defaults/Templates 
 *   Step 2.8  Perform Post-Template Validation 
 *
 * @param J (I) [modified]
 * @param TaskList (I) optional [possibly modified]
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MRMJobPostLoad(

  mjob_t *J,
  int    *TaskList,
  mrm_t  *R,
  char   *EMsg)

  {
  int       TC;
  int       SC;

  mqos_t   *QDef;
  mbool_t   AccountFallback;

  mbitmap_t RMXBM;

  int       rqindex;

  mreq_t   *RQ;
  mnode_t  *N;

  enum MJobCtlCmdEnum FailureAction;
  enum MAllocRejEnum  JobEvalFailReason;
  enum MS3CodeDecadeEnum S3C;

  int       NReq;
  mulong    DETime;

  char      ErrMsg[MMAX_LINE];
  char      tmpLine[MMAX_LINE];
  char      tmpMsg[MMAX_LINE];
  char      vmid[MMAX_NAME];

  char     *ReqRsv;

  const char *FName = "MRMJobPostLoad";

  MDB(5,fRM) MLog("%s(%s,TaskList,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  /* Step 1.0  Initialize */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) || (R == NULL))
    {
    MDB(1,fRM) MLog("ERROR:    invalid job/RM pointer passed to %s()\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  /* Step 2.0  Adapt Job Based on Learned Attributes and System Attributes */

  /* Step 2.2  Apply System Attributes */

  bmunset(&J->IFlags,mjifUIQ);

  AccountFallback = FALSE;

  /* Checkpoint job immediately if job was just msub'ed. Job checkpoint files 
   * will be deleted once the job is migrated to the resource manager. When
   * migrating to another moad the checkpoing file is retained.
   * It's possible to submit jobs on iteration 0. Only on 
   * iteration will the checkpoint file be read and write checkpoint files. */

  if ((!bmisset(&J->IFlags,mjifWasLoadedFromCP)) &&
      (bmisset(&R->IFlags,mrmifLocalQueue) == TRUE) &&
      (!bmisset(&J->Flags,mjfNoRMStart) || !bmisset(&J->Flags,mjfClusterLock)) &&
      (MJobShouldCheckpoint(J,NULL) == TRUE))
    {
    if (MJobWriteCPToFile(J) == FAILURE)
      return(FAILURE);
    }


  
  /* what was the below code supposed to do? */

  /*
  if ((J->PeerReqRID != NULL) && (strcmp(J->ReqRID,"ALL") == 0))
    {
    MUFree(&J->ReqRID);
    MUStrDup(&J->ReqRID,J->PeerReqRID); 

    MDB(7,fRM) MLog("INFO:     job '%s' ReqRID restored from peer as '%s'\n",
      J->Name,
      J->ReqRID);
    }
  */

  /* Step 2.4  Populate Missing Attributes */

  /* NOTE:  determine req/def QOS, allocation status, resulting account/QOS,
            then create reservation */

  /* mjifWasLoadedFromCP set after checkpoint loaded */

  if ((!bmisset(&J->IFlags,mjifWasLoadedFromCP)) && (J->Req[0] == NULL))
    {
    MReqCreate(J,NULL,NULL,TRUE);
    }

  if (bmisclear(&J->Flags))
    bmcopy(&J->Flags,&J->SpecFlags);

  if (J->SubmitTime == 0)
    {
    MDB(4,fRM) MLog("WARNING:  no QueueTime on job '%s'\n",
      J->Name);

    /* attempt to guess approximate job submission time */

    if (J->StartTime > 0)
      J->SubmitTime = J->StartTime;
    else
      J->SubmitTime = MSched.Time;
    }  /* END if (J->SubmitTime == 0) */

  if (J->SystemQueueTime <= 0)
    {
    /* set system queue time if info not specified in checkpoint */

    J->SystemQueueTime = J->SubmitTime;
    }

  if ((J->Req[0] != NULL) && 
      (J->Req[0]->BlockingFactor > 0))
    {
    MJobParseBlocking(J);
    }

  if (J->Geometry != NULL)
    {
    MJobParseGeometry(J,J->Geometry);
    }

  if ((MJobDetermineTTC(J) == SUCCESS) &&
      (TaskList != NULL))
    {
    /* modify TaskList to reflect TTC */

    TaskList[1] = -1;
    }

  if (J->MinWCLimit > 0)
    {
    /* scale-down walltime according to J->MinWCLimit */

    if ((J->OrigWCLimit <= 0) &&
        (MSched.JobExtendStartWallTime == TRUE))
      {
      J->OrigWCLimit = J->WCLimit;
      }
    else if ((J->OrigWCLimit <= 0) && 
        (MSched.JobExtendStartWallTime == FALSE))
      {
      /* Must preserve OrigWCLimit. WCLimit isn't set at this point. */

      J->OrigWCLimit = J->SpecWCLimit[1];
      }

    J->WCLimit     = J->MinWCLimit;

    J->SpecWCLimit[0] = J->WCLimit;
    J->SpecWCLimit[1] = J->WCLimit;
    }

  if (bmisset(&J->Hold,mhUser) || bmisset(&J->Hold,mhSystem) || bmisset(&J->Hold,mhBatch))
    {
    MJobCheckPriorityAccrual(J,NULL);
    MJobAddEffQueueDuration(J,NULL);
    }

  J->EState = J->State;

  /* NOTE:  J->System may not be allocated at this point even if J->Flags has
            mjfSystemJob set */

  if (bmisset(&MSched.Flags,mschedfAllowMultiCompute) &&
      !bmisset(&J->Flags,mjfSystemJob) &&
     (J->System == NULL) &&
     (J->Req[0] != NULL) && 
     (J->Req[0]->Opsys == 0))
    {
    /* NOTE:  should identify feasible target node and assign
              first OS from node OSList to job (NYI) */

    if ((J->SubmitRM != NULL) && (J->SubmitRM->DefOS != 0))
      {
      MDB(5,fRM) MLog("INFO:     OS not specified by job %s - assigning default OS '%s' for RM %s\n",
        J->Name,
        MAList[meOpsys][J->SubmitRM->DefOS],
        J->SubmitRM->Name);

      J->Req[0]->Opsys = J->SubmitRM->DefOS;
      }
    else if (MSched.DefaultJobOS != 0)
      {
      MDB(5,fRM) MLog("INFO:     OS not specified by job %s - assigning default job OS '%s'\n",
        J->Name,
        MAList[meOpsys][MSched.DefaultJobOS]);

      J->Req[0]->Opsys = MSched.DefaultJobOS;
      } 
    else if (MPar[1].ReqOS != 0)
      {
      MDB(5,fRM) MLog("INFO:     OS not specified by job %s - assigning first partition req OS '%s'\n",
        J->Name,
        MAList[meOpsys][MPar[1].ReqOS]);

      J->Req[0]->Opsys = MPar[1].ReqOS;
      }
    }

  /* Step 2.5  Validate Job */

  /* Step 2.5.1  Check External Validation Interface */

  /* MRMJobValidate() needs to be called after MCPRestore() loads job variables */

  if (bmisset(&J->IFlags,mjifIsValidated) == FALSE)
    {
    /* NOTE:  User proxy list can be admin specified or can be dynamically
              discovered and modified via JobValidateURL */

    if (R->ND.URL[mrmXJobValidate] != NULL)
      {
      if (MRMJobValidate(J,R,tmpMsg,&FailureAction,&SC) == FAILURE)
        {
        char tmpLine[MMAX_LINE];

        if (EMsg != NULL)
          MUStrCpy(EMsg,tmpMsg,MMAX_LINE);

        snprintf(tmpLine,sizeof(tmpLine),"job validation failed - %s",
          tmpMsg);

        MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,tmpLine);

        if (J == NULL)
          {
          return(FAILURE);
          }
        }
      else if ((J->EUser != NULL) && (strcmp(J->Credential.U->Name,J->EUser)))
        {
        /* cache acceptable proxy user */

        if (J->Credential.U->ProxyList == NULL)
          {
          MUStrDup(&J->Credential.U->ProxyList,J->EUser);
          }
        else
          {
          /* verify user not already listed */

          if (MUCheckStringList(J->Credential.U->ProxyList,J->EUser,',') == FAILURE)
            MUStrAppend(&J->Credential.U->ProxyList,NULL,J->EUser,',');
          }
        }
      }  /* END if (R->ND.URL[mrmXJobValidate] != NULL) */
    }    /* END if (bmisset(&J->IFlags,mjifIsValidated == FALSE)) */

  /* Step 2.5.2  Perform Pre-Template Internal Job Validation */

  if (J->Req[0] == NULL)
    {
    MDB(1,fRM) MLog("ERROR:    invalid job/RM pointer passed to %s()\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  if (J->Credential.U == NULL)
    {
    /* invalid user credential */

    if (EMsg != NULL)
      strcpy(EMsg,"no user specified");

    return(FAILURE);
    }

  /* Step 2.6  Adjust Job Credentials */

  MJobUpdateResourceCache(J,NULL,0,NULL);

  MJobGetClass(J);

  if (J->Credential.G != NULL)
    {
    /* if group is specified by resource manager, it is trusted */

    MULLAdd(&J->Credential.U->F.GAL,J->Credential.G->Name,NULL,NULL,NULL); /* no free routine */
    }
  else
    {
    mgcred_t *tmpG;
      
    if (MJobGetGroup(J,NULL,&tmpG) == FAILURE)
      {
      /* invalid credentials */
        
      return(FAILURE);
      }

    J->Credential.G = tmpG;
    }

  if (!bmisset(&J->IFlags,mjifQOSLocked))
    {
    if (MSched.FSTreeDepth > 0)
      MJobGetFSTree(J,NULL,FALSE);

    if ((MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&QDef) == FAILURE) ||
        (J->QOSRequested == NULL) ||
        (J->QOSRequested == &MQOS[0]))
      {
      if (QDef != NULL)
        {
        /* Don't overwrite a [potentially] valid QOS with NULL */

        MJobSetQOS(J,QDef,0);
        }
      }
    else
      {
      MJobSetQOS(J,J->QOSRequested,0);
      }
    }

  /* Step 2.7  Apply Defaults/Templates */

  /* NOTE: MJobSetDefaults() depends on J->SystemID, QOS, etc. and so should occur after 
           MJobProcessExtensionString() */

  /* NOTE: MJobSetDefaults() applies JSet and JDef templates */
            
  MJobSetDefaults(J,bmisset(&J->IFlags,mjifNoTemplates) ? FALSE : TRUE);

  /* Step 2.8  Perform Post-Template Validation */

  if ((J->Credential.U->OID == 0) &&
      !bmisset(&J->Flags,mjfSystemJob) &&
      (MSched.AllowRootJobs == FALSE))
    {
    /* credentials do not exist locally to allow direct submission to RM */

    if (EMsg != NULL)
      {
      sprintf(EMsg,"user root cannot submit jobs");
      }

    MDB(6,fCKPT) MLog("INFO:     job %s rejected - user root cannot submit jobs\n",
      J->Name);

    return(FAILURE);
    }

  if ((J->TemplateExtensions != NULL) && 
      (J->TemplateExtensions->WorkloadRMID != NULL) &&
      (J->SubmitRM != NULL) &&
      !strcmp(J->TemplateExtensions->WorkloadRMID,J->SubmitRM->Name))
    {
    if (J->TemplateExtensions->WorkloadRM == NULL)
      {
      if (MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM) == FAILURE)
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot locate workload RM '%s'",
            J->TemplateExtensions->WorkloadRMID);
          }

        MDB(6,fCKPT) MLog("INFO:     job %s rejected - cannot locate workload RM\n",
          J->Name);

        return(FAILURE);
        }
      }

    if (MSched.DefaultComputeNodeRM != NULL)
      {
      int rqindex;

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        if (J->Req[rqindex] == NULL)
          break;

        /* FIXME:  must determine resource manager associated 
                   with compute nodes */

        J->Req[rqindex]->RMIndex = MSched.DefaultComputeNodeRM->Index;
        }  /* END for (rqindex) */

      /* NOTE:  associate job with allocated compute resource RM */

      if (J->SubmitRM != NULL)
        J->SubmitRM = MSched.DefaultComputeNodeRM;

      if (J->DestinationRM != NULL)
        J->DestinationRM = MSched.DefaultComputeNodeRM;
      }  /* END if (MSched.DefaultComputeNodeRM != NULL) */
    else if ((MJOBISACTIVE(J) == TRUE) && 
             (MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS))
      {
      if (J->SubmitRM != NULL)
        J->SubmitRM = N->RM;

      if (J->DestinationRM != NULL)
        J->DestinationRM = N->RM;
      }
    }    /* END if ((J->TX != NULL) && ...) */

  /* NOTE: ReqRsv processing must be after MJobSetDefaults() which sets 
           mjifWCNotSpecified flag */

  ReqRsv = NULL;

  if ((J->Credential.C != NULL) && (J->Credential.C->F.ReqRsv != NULL))
    ReqRsv = J->Credential.C->F.ReqRsv;

  if ((J->Credential.A != NULL) && (J->Credential.A->F.ReqRsv != NULL))
    ReqRsv = J->Credential.A->F.ReqRsv;

  if ((J->Credential.G != NULL) && (J->Credential.G->F.ReqRsv != NULL))
    ReqRsv = J->Credential.G->F.ReqRsv;

  if ((J->Credential.Q != NULL) && (J->Credential.Q->F.ReqRsv != NULL))
    ReqRsv = J->Credential.Q->F.ReqRsv;

  if ((J->Credential.U != NULL) && (J->Credential.U->F.ReqRsv != NULL))
    ReqRsv = J->Credential.U->F.ReqRsv;

  if (ReqRsv != NULL)
    {
    /* or should F.ReqRsv have the NotAdvres as well? */
    
    bmset(&J->SpecFlags,mjfAdvRsv);
    bmset(&J->Flags,mjfAdvRsv);

    if (ReqRsv[0] == '!')
      {
      bmset(&J->IFlags,mjifNotAdvres);

      ReqRsv++;
      }

    MUStrDup(&J->ReqRID,ReqRsv);

    if (bmisset(&J->IFlags,mjifWCNotSpecified))
      {
      mrsv_t *tR;

      if ((MRsvFind(ReqRsv,&tR,mraNONE) == SUCCESS) ||
          (MRsvFind(ReqRsv,&tR,mraRsvGroup) == SUCCESS))
        {
        /* job should inherit duration of bind rsv with time pad for
           one scheduling iteration */

        /*Doug says that the job shouldn't inherit duration of bind rsv.
         *This overwrites default wclimits specified, for instance, by a class - SR

        J->SpecWCLimit[1] = MAX(
          1,
          tR->EndTime - tR->StartTime - MSched.MaxRMPollInterval);
         */
        }
      }
    }    /* END if (ReqRsv != NULL) */

  /* do not enforce system limits at this point */

  /*
  if (MPar[0].L.JP->HLimit[mptMaxWC][0] > 0)
    J->SpecWCLimit[1] = MIN(J->SpecWCLimit[1],MPar[0].L.JP->HLimit[mptMaxWC][0]);
  */

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

      /* NOTE:  should search completed jobs as well */

      if ((MJobFind(ptr,&SJ,mjsmExtended) == FAILURE) &&
          (MJobCFind(ptr,&SJ,mjsmBasic) == FAILURE))
        {
        /* cannot locate specified application */

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot locate target VM application '%s'",
            ptr);
          }

        MJobSetHold(J,mhBatch,0,mhrData,"cannot locate target VM application");

        /* add dependency to parent application - NYI */
        }
      else
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"target VM application '%s' has no active VM allocation",
            ptr);
          }

        MJobSetHold(J,mhBatch,0,mhrData,"target VM application has no active VM allocation");
        }
      }
    }    /* END if (MUStringGetEnvValue(J->E.BaseEnv,"MOAB_VMID",...)) == SUCCESS) */

  J->SpecWCLimit[0] = J->SpecWCLimit[1];

  if ((!bmisset(&MPar[0].Flags,mpfUseMachineSpeed)) || (MJOBISACTIVE(J) == FALSE))
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

  /* set tasklist/nodelist */

  if ((TaskList != NULL) && (TaskList[0] != -1))
    {
    MJobSetAllocTaskList(J,NULL,TaskList);

    if (MJOBISACTIVE(J) &&
        (J->Geometry != NULL))
      {
      mbool_t TC;

      MJobPopulateReqNodeLists(J,&TC);
      }
    }

  /* apply class defaults */
  
  MJobApplyClassDefaults(J);

  if (J->Credential.C != NULL)
    {
    if ((J->Credential.C->L.JobMaximums[0] != NULL) &&
        (J->Credential.C->L.JobMaximums[0]->CPULimit > 0))
      {
      /* apply max usage limits */

      J->CPULimit = J->Credential.C->L.JobMaximums[0]->CPULimit;
      }  
    }    /* END if (J->Cred.C->JMax != NULL) */

  /* should checks be bypassed if QoS locked is set? */

  if (J->Credential.Q != NULL)
    {
    /* check QoS constraints */

    if ((!bmisset(&J->Flags,mjfNoRMStart) || !bmisset(&J->Flags,mjfClusterLock)) &&
       (MJobCheckQOSJLimits(
          J,
          J->Credential.Q,
          FALSE,
          tmpLine,
          sizeof(tmpLine)) == FAILURE))
      {
      /* job does not meet qos constraints */

      sprintf(tmpMsg,"job violates qos configuration '%s'",
        tmpLine);

      MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,tmpMsg);

      if (J == NULL)
        {
        /* job removed in MJobReject() */

        if (EMsg != NULL)
          strcpy(EMsg,tmpLine);
          
        return(FAILURE);
        }
      }    /* END if (MJobCheckQOSJLimits() == FAILURE) */
    }      /* END if (J->Cred.Q != NULL) */

  if (J->Credential.C != NULL)
    {
    /* check class constraints */

    /* NOTE:  some class limits may have QOS exemptions/dependencies, ie, OMAXWC 
              only enforce limits after job QOS credential has been assigned */

    /* should class checks be bypassed if ClassLocked is set? */

    if (!bmisset(&J->Flags,mjfNoRMStart) || !bmisset(&J->Flags,mjfClusterLock))
      {
      if (MJobCheckClassJLimits(
            J,
            J->Credential.C,
            FALSE,
            0,
            tmpLine,  /* O */
            sizeof(tmpLine)) == FAILURE)
        {
        char tmpMsg[MMAX_LINE];

        /* job does not meet class constraints */

        sprintf(tmpMsg,"job violates class configuration '%s'",
          tmpLine);

        MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,tmpMsg);

        if (J == NULL)
          {
          /* job removed in MJobReject() */

          if (EMsg != NULL)
            strcpy(EMsg,tmpLine);

          return(FAILURE);
          }

        if (!bmisset(MJobGetRejectPolicy(J),mjrpRetry))
          {
          if ((J->SubmitRM != NULL) && (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue)))
            {
            /* if job has not yet been submitted */

            if (EMsg != NULL)
              strcpy(EMsg,tmpLine);

            return(FAILURE);
            }
          }
        }    /* END if (MJobCheckClassJLimits() == FAILURE) */
      }
    }      /* END if (J->Cred.C != NULL) */

  if (J->Credential.A != NULL)
    {
    /* check account constraints */

    /* should account checks be bypassed if AccountLocked is set? */

    if (!bmisset(&J->Flags,mjfNoRMStart) || !bmisset(&J->Flags,mjfClusterLock))
      {
      if (MJobCheckAccountJLimits(
            J,
            J->Credential.A,
            FALSE,
            tmpLine,
            sizeof(tmpLine)) == FAILURE)
        {
        char tmpMsg[MMAX_LINE];

        /* job does not meet class constraints */

        sprintf(tmpMsg,"job violates account configuration '%s'",
          tmpLine);

        MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,tmpMsg);

        if (J == NULL)
          {
          /* job removed in MJobReject() */

          if (EMsg != NULL)
            strcpy(EMsg,tmpLine);

          return(FAILURE);
          }

        if (!bmisset(MJobGetRejectPolicy(J),mjrpRetry))
          {
          if ((J->SubmitRM != NULL) && (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue)))
            {
            /* if job has not yet been submitted */

            if (EMsg != NULL)
              strcpy(EMsg,tmpLine);

            return(FAILURE);
            }
          }
        }    /* END if (MJobCheckAccountJLimits() == FAILURE) */
      }
    }      /* END if (J->Cred.A != NULL) */

  /* check account allocation status */

  if ((MAM[0].FallbackAccount[0] != '\0') || (MAM[0].FallbackQOS[0] != '\0'))
    {
    if (MAMHandleCreate(&MAM[0],J,mxoJob,NULL,ErrMsg,&S3C) == FAILURE)
      {
      MDB(3,fRM) MLog("WARNING:  Unable to register job creation with accounting manager for job '%s', reason: %s\n",
        J->Name,
        (S3C > ms3cNone && S3C < ms3cLAST && MS3CodeDecade[S3C] != NULL) ? MS3CodeDecade[S3C] : "Unknown Failure");

      if (S3C == ms3cNoFunds)
        {
        mgcred_t *A = NULL;

        char tmpFBAccount[MMAX_LINE];

        if (MAM[0].FallbackAccount[0] == '+')
          {
          snprintf(tmpFBAccount,sizeof(tmpFBAccount),"%s%s",
            (J->Credential.A->Name != NULL) ? J->Credential.A->Name : "NULL",
            MAM[0].FallbackAccount + 1);
          }
        else
          {
          strcpy(tmpFBAccount,MAM[0].FallbackAccount);
          }

        MDB(3,fRM) MLog("WARNING:  Insufficient balance in primary account '%s' to run job '%s' (attempting fallback credentials)\n",
          (J->Credential.A != NULL) ? J->Credential.A->Name : "NULL",
          J->Name);

        /* look for the fallback account */

        MAcctFind(tmpFBAccount,&A);

        if ((A != NULL) && (A->Name[0] != '\0'))
          {
          J->Credential.A = A;

          AccountFallback = TRUE;

          bmset(&J->IFlags,mjifFBAcctInUse);

          MRMJobModify(J,"Account_Name",NULL,A->Name,mSet,"Fallback account.",NULL,NULL);
          }

        /* look for the fallback qos */

        if (MAM[0].FallbackQOS[0] != '\0')
          {
          MQOSAdd(MAM[0].FallbackQOS,&J->Credential.Q);

          bmset(&J->IFlags,mjifFBQOSInUse);
          }

        /* if we didn't find either fallback qos or account */
        
        if ((A == NULL) && (MAM[0].FallbackQOS[0] == '\0'))
          {
          MDB(2,fRM) MLog("ALERT:    cannot locate fallback account '%s' or qos '%s'\n",
            tmpFBAccount,
            MAM[0].FallbackQOS);
          }
        }
      }
    }      /* END if (MAM[0].FallbackAccount[0] != '\0') */

  /* reset QOS in case fallback account utilized */

  if ((MAM[0].FallbackQOS[0] == '\0') &&
      ((MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&QDef) == FAILURE) ||
       (J->QOSRequested == NULL) ||
       (J->QOSRequested == &MQOS[0])))
    {
    /* NOTE: no FBQOS configured, no access to requested QOS */

    if (AccountFallback == TRUE)
      MJobSetQOS(J,QDef,0);

    if ((J->QOSRequested != NULL) && (AccountFallback == FALSE))
      {
      char tmpLine[MMAX_LINE];

      MDB(2,fRM) MLog("ALERT:    job '%s' cannot use requested QOS %s (deferring/setting QOS to default %s)\n",
        J->Name,
        J->QOSRequested->Name,
        (QDef != NULL) ? QDef->Name : "NULL");

      /* job cannot run in simulation mode w/o admin intervention */

      snprintf(tmpLine,sizeof(tmpLine),"job cannot use requested QOS '%s'",
        J->QOSRequested->Name);

      MQOSReject(J,mhBatch,MSched.DeferTime,tmpLine);

      if (J == NULL)
        {
        /* job removed in MJobReject() */

        if (EMsg != NULL)
          strcpy(EMsg,tmpMsg);

        return(FAILURE);
        }
      }
    }    /* END if ((MAM[0].FallbackQOS[0] == '\0') && ...) */
  else if ((J->Credential.Q != NULL) &&
           (!bmisset(&J->IFlags,mjifFBQOSInUse)) &&
           (!bmisset(&J->IFlags,mjifQOSLocked)) &&
           (J->QOSRequested != NULL) &&
           (J->Credential.Q != J->QOSRequested) &&
           !bmisset(&J->IFlags,mjifFBQOSInUse) &&
           (MQOSGetAccess(J,J->QOSRequested,NULL,NULL,NULL) == SUCCESS))
    {
    MJobSetQOS(J,J->QOSRequested,0);
    }  /* END else ((MQOSGetAccess(J,J->QReq,NULL,&QDef) == FAILURE) || ... ) */

  if ((J->Credential.Q != NULL) && bmisset(&J->Credential.Q->Flags,mqfDeadline))
    {
    if ((!bmisset(&J->Flags,mjfNoRMStart) || !bmisset(&J->Flags,mjfClusterLock)) &&
        (MJobCheckDeadLine(J) == FAILURE))
      {
      /* cannot support deadline for job */

      sprintf(tmpMsg,"job cannot obtain deadline");

      MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,tmpMsg);

      if (J == NULL)
        {
        /* job removed in MJobReject() */

        if (EMsg != NULL)
          strcpy(EMsg,tmpMsg);

        return(FAILURE);
        }
      }
    }  /* END if ((J->Cred.Q != NULL) && bmisset(&J->Cred.Q->Flags,mqfDeadline)) */

  if (((J->IsInteractive == TRUE) || (bmisset(&J->Flags,mjfInteractive))) &&
      (MSched.DisableInteractiveJobs == TRUE))
    {
    MDB(1,fRM) MLog("ALERT:    interactive job %s passed in - interactive jobs are disabled\n",
      J->Name);

    if (EMsg != NULL)
      strcpy(EMsg,"interactive jobs are currently disabled");

    MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,"interactive jobs are currently disabled");

    return(FAILURE);
    }

  MJobBuildCL(J);

  MJobUpdateFlags(J);

  if (bmisset(&J->Flags,mjfGResOnly))
    {
    /* check for invalid use of this flag */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,0) <= 0)
        {
        if ((rqindex == 0) && (J->Req[1] == NULL))
          {
          MDB(1,fRM) MLog("ALERT:    gresonly flag used on non-gres job\n",
            J->Name);
     
          if (EMsg != NULL)
            strcpy(EMsg,"gresonly flag used on non-gres job");
     
          MJobReject(&J,mhBatch,MSched.DeferTime,mhrRMReject,"gresonly flag used on non-gres job");
     
          return(FAILURE);
          }
        
        /* this req has no gres requirement so it's probably just a leftover, delete it */

        MReqRemoveFromJob(J,rqindex);
        }
      else
        {
        /* zero out the other requested resources */

        J->Req[rqindex]->DRes.Procs = 0;
        J->Req[rqindex]->DRes.Mem   = 0;
        J->Req[rqindex]->DRes.Disk  = 0;
        J->Req[rqindex]->DRes.Swap  = 0;
        }
      }
    }  /* END if (bmisset(&J->Flags,mjfGResOnly)) */

  if (bmisset(&J->Flags,mjfClusterLock) &&
      !bmisset(&R->IFlags,mrmifLocalQueue))
    {
    /* lock job into current cluster/partition */

    bmclear(&J->SysPAL);

    bmset(&J->SysPAL,R->PtIndex);
    }

  /* determine partition mask */

  if (MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL) == FAILURE)
    {
    char ParLine[MMAX_LINE];

    /* cannot utilize requested PAL */
 
    MDB(2,fRM) MLog("ALERT:    cannot utilize requested PAL '%s'\n",
      MPALToString(&J->SpecPAL,NULL,ParLine));
    }

  if (MJobWantsSharedMem(J) == TRUE)
    MRMUpdateSharedMemJob(J);

  /* check credential holds */

  MJobAdjustHold(J);

  if ((J->State == mjsIdle) && (J->EState != mjsDeferred))
    {
    /* adjust idle job statistics handled in MQueueSelectAllJobs() */

    /* NO-OP */
    }
  else if (MJOBISALLOC(J) == TRUE)
    {
    long   tmpL;
    double tmpD;
    double TotalCPULoad;
    double TotalIOLoad;
    long   ComputedWallTime;

    int IOIndex;

    MDB(4,fRM) MLog("INFO:     allocated job '%s' detected\n",
      J->Name);

    /* setup allocated node count */

    if (J->Alloc.TC == 0)
      {
      MDB(2,fRM) MLog("WARNING:  job '%s' loaded in allocated state with no tasks allocated\n",
        J->Name);

      MDB(2,fRM) MLog("INFO:     adjusting allocated procs to %d for job '%s'\n",
        J->TotalProcCount,
        J->Name);

      J->Alloc.TC = J->Request.TC;
      }

    IOIndex = MUMAGetIndex(meGRes,"IOLOAD",mVerify);

    /* sanity check start time */

    J->StartTime = MAX(J->StartTime,J->SubmitTime);

    tmpL = J->StartTime + J->SWallTime + J->WCLimit - MSched.Time;

    J->RemainingTime = (tmpL > 0) ? tmpL : 0;

    /* sanity check the walltime of how long the job has been running */

    ComputedWallTime = MAX(0,(long)(MSched.Time - J->StartTime - J->SWallTime));

    if (J->AWallTime <= 0)
      {
      MDB(7,fSCHED) MLog("INFO:     job %s setting walltime to computed walltime %ld (was negative or zero)\n",
        J->Name,
        J->AWallTime);

      J->AWallTime = ComputedWallTime;
      }
    else if (J->AWallTime > ComputedWallTime)
      {
      MDB(7,fSCHED) MLog("INFO:     job %s setting walltime from %ld to computed walltime %ld (exceeded computed walltime)\n",
        J->Name,
        J->AWallTime,
        ComputedWallTime);

      J->AWallTime = ComputedWallTime;
      }
    else if ((ComputedWallTime - J->AWallTime) > 2)
      {
      /* Somehow we lost at least a couple of seconds of walltime, perhaps moab died or
         was restarted while the job was running and we read in an old walltime from the checkpoint file */

      MDB(7,fSCHED) MLog("INFO:     job %s setting walltime to %ld correcting drift of %ld seconds\n",
        J->Name,
        ComputedWallTime,
        ComputedWallTime - J->AWallTime);

      J->AWallTime = ComputedWallTime;
      }

    /* evaluate allocated nodes */

    TotalCPULoad = 0.0;
    TotalIOLoad  = 0.0;

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      int nindex;
  
      RQ = J->Req[rqindex];

      if ((RQ->DRes.Procs == 0) && (!bmisset(&J->SpecFlags,mjfNoResources)))
        {
        MReqAllocateLocalRes(J,RQ);
        }      /* END if (RQ->DRes.Procs == 0) */

      for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        TC = MNLGetTCAtIndex(&RQ->NodeList,nindex);

        N->TaskCount  += TC;
        N->EProcCount += TC * RQ->DRes.Procs;

        TotalCPULoad += N->Load;

        if ((IOIndex != 0) && (MSNLGetIndexCount(&N->CRes.GenericRes,IOIndex) != 0))
          {
          TotalIOLoad += MSNLGetIndexCount(&N->CRes.GenericRes,IOIndex) - MSNLGetIndexCount(&N->ARes.GenericRes,IOIndex);;
          }

        if (MSched.AdjustUtlRes == TRUE)
          {
          if ((RQ->URes != NULL) && (N->RM != NULL) && (N->RM->Type == mrmtPBS))
            {
            int tmpInt;

            /* modify per node utilized resources */

            tmpInt = MAX(0,N->ARes.Swap - RQ->URes->Swap);
            MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);
            }
          }

        /* NOTE:  even if node accesspolicy is dedicated blocking resources
                  from use by other jobs, adjust node dedicated resources only
                  by actual job dedicated usage */

        /* NOTE:  sync code below with DRes update code in 
                  MJobUpdateResourceUsage() - consider merging! */

        if ((RQ->NAccessPolicy == mnacSingleJob) ||
                 (RQ->NAccessPolicy == mnacSingleTask))
          {
          /* OLD BEHAVIOR: dedicate all resources on the node to the job
             NEW BEHAVIOR: only dedicate what the job asked for (changed 6/27/07 by DRW) */

          /*
          MCResAdd(&N->DRes,&N->CRes,&N->CRes,1,FALSE);
          */
          MCResAdd(&N->DRes,&N->CRes,&RQ->DRes,TC,FALSE);
          }

        /* NOTE:  when we read a job from the slave peer, we end up double
         *        counting the dedicated resources later on in
         *        MJobUpdateResourceUsage, called from MRMJobPostUpdate. */

        else if ((J->State != mjsSuspended) &&
            (!bmisset(&R->Flags,mrmfSlavePeer)))
          {
          MCResAdd(&N->DRes,&N->CRes,&RQ->DRes,TC,FALSE);
          }

        if (J->State == mjsSuspended)
          {
          N->SuspendedJCount++;
          }
        }    /* END for (nindex)  */

      /* determine partition */

      if ((RQ->PtIndex == 0) && !bmisset(&J->Flags,mjfCoAlloc))
        {
        if (MNLGetNodeAtIndex(&RQ->NodeList,0,&N) == SUCCESS)
          RQ->PtIndex = N->PtIndex;
        else
          RQ->PtIndex = 0;
        }
      }      /* END for (rqindex) */

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

    if (MSched.ExtendedJobTracking == TRUE)
      {
      mgmetric_t *XM;

      if (J->ExtLoad == NULL)
        {
        J->ExtLoad = (mxload_t *)MUCalloc(1,sizeof(mxload_t));

        MGMetricCreate(&J->ExtLoad->GMetric);
        }

      XM = J->ExtLoad->GMetric[MSched.CPUGMetricIndex];

      XM->TotalLoad += TotalCPULoad;

      if ((XM->Max == 0) || (XM->Max < TotalCPULoad))
        XM->Max = TotalCPULoad;
 
      if ((XM->Min == 0) || (XM->Min > TotalCPULoad))
        XM->Min = TotalCPULoad;
 
      XM->SampleCount++;
      } 

    RQ = J->Req[0];

    if ((J->DispatchTime == 0) && !bmisset(&J->SpecFlags,mjfNoRMStart))
      {
      MDB(6,fRM) MLog("ERROR:    job '%s' loaded in alloc state '%s' with no dispatch time\n",
        J->Name,
        MJobState[J->State]);

      J->DispatchTime = MSched.Time;

      /* hack to work around LL API bug */

      J->StartTime = J->DispatchTime;
      }

    MUNLGetMinAVal(&J->NodeList,mnaSpeed,NULL,(void **)&tmpD);

    J->WCLimit = (long)((tmpD > 0.0) ?
      (long)((double)J->SpecWCLimit[0]) / tmpD :
      J->SpecWCLimit[0]);

    if (MNLIsEmpty(&RQ->NodeList))
      {
      /* req[0] task map not loaded */

      if (J->Req[1] == NULL)
        {
        /* job is single req and req[0] node list is not populated */

        /* set req nodelist to full job nodelist */

        /* we need to do better than this */

        MNLCopy(&RQ->NodeList,&J->NodeList);
        }
      else if (RQ->DRes.Procs == 0)
        {
        /* req is generic resource based */

        /* NOTE:  always copy job nodelist onto req nodelist, why? */
        /*        is this for licenses? network resource? other? */

        /* matching node should be associated to req in MReqAllocLocalRes()  */

        /* the following memcpy() should be removed (NYI) */

        MNLCopy(&RQ->NodeList,&J->NodeList);

        MDB(0,fRM) MLog("ALERT:    req node list empty for job %s:%d in state %s (job nodelist copied to req nodelist)\n",
          J->Name,
          RQ->Index,
          MJobState[J->State]);
        }
      else
        {
        MDB(0,fRM) MLog("ALERT:    cannot locate task allocation info for job %s:%d in state %s\n",
          J->Name,
          RQ->Index,
          MJobState[J->State]);

        /* NO-OP */
        }
      }

    if (MJOBISACTIVE(J) == TRUE)
      {
      /* don't create reservations for suspended jobs */

      MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);

      MQueueAddAJob(J);
      }
    }   /* END else if (MJOBISALLOC(J) == TRUE) */

  if (J->Req[1] == NULL)
    {
    RQ = J->Req[0];

    RQ->TaskCount = J->Request.TC;
    }    /* END if (J->Req[1] == NULL) */

  /* check job deadline status */

  if ((J->Credential.Q != NULL) && bmisset(&J->Credential.Q->Flags,mqfDeadline))
    {
    /* if completion date is invalid, disable deadline */
   
    /* if completed date is not specified, determine from QOS targets */

    if ((J->CMaxDate > 0) && (J->CMaxDate < MMAX_TIME))
      {
      /* absolute deadline specified by job */

      J->CMaxDateIsActive = MBNOTSET;
      }
    else if (J->Credential.Q->QTTarget > 0)
      {
      J->CMaxDate = J->SubmitTime + J->Credential.Q->QTTarget + J->WCLimit;

      J->CMaxDateIsActive = MBNOTSET;
      }
    else if (J->Credential.Q->XFTarget > 0.0)
      {
      /* XF = (QT + WC) / WC -> QT = (XF - 1) * WC */
      /* END = SubmitTime + QT + WC -> END = SubmitTime + (XF * WC) */

      J->CMaxDate = (long)(J->SubmitTime + (J->Credential.Q->XFTarget * J->WCLimit));

      J->CMaxDateIsActive = MBNOTSET;
      }
    else
      {
      /* if completion date is invalid, disable deadline */

      J->CMaxDateIsActive = FALSE;
      }
    }    /* END if ((J->Cred.Q != NULL) && bmisset(mqfDeadline)) */

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

  if (J->SubmitRM != NULL)
    {
    if ((J->DestinationRM == NULL) && !bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue))
      {
      J->DestinationRM = J->SubmitRM;

      J->Req[0]->RMIndex = J->DestinationRM->Index;
      }
    }

  if (J->Request.NC > 0)
    {
    NReq = J->Request.NC;
    }
  else if (MPar[0].MaxPPN > 0)
    {
    NReq = (J->Request.TC / MPar[0].MaxPPN) + ((J->Request.TC % MPar[0].MaxPPN) > 0) ? 1 : 0;
    }
  else
    {
    NReq = 1;
    }

  if (NReq > MSched.JobMaxNodeCount)
    {
    char tmpLine[MMAX_LINE];

    /* current node mapping requires too many processors */

    /* other feasible node mappings may be available */

    /* NOTE:  should compare allocation against optimal possible allocation
              and only set hold if job can never run (NYI) */

    sprintf(tmpLine,"job %s requests %d nodes, %s set to %d (increase %s?)",
      J->Name,
      NReq,
      MParam[mcoJobMaxNodeCount],
      MSched.JobMaxNodeCount,
      MParam[mcoJobMaxNodeCount]);

    MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

    sprintf(tmpLine,"job requests too many nodes - %d nodes requested, %d node per job limit (NC %d TC %d PPN %d)",
      NReq,
      MSched.JobMaxNodeCount,
      J->Request.NC,
      J->Request.TC,
      MPar[0].MaxPPN);

    if (EMsg != NULL)
      strcpy(EMsg,tmpLine);

    MJobSetHold(J,mhBatch,0,mhrNoResources,tmpLine);

    if (MSched.JobMaxNodeCountOFOID == NULL)
      {
      MUStrDup(&MSched.JobMaxNodeCountOFOID,J->Name);
      }

    return(FAILURE);
    }

  MJobUpdateResourceCache(J,NULL,0,NULL);

  /* call this again in case something has changed on the job (like proc-count) */

  MJobBuildCL(J);

  /* adjust statistics */

  if (!bmisset(&J->IFlags,mjifWasLoadedFromCP))
    MStatUpdateSubmitJobUsage(J);

  /* will automatically update J->SIData's file sizes/times */
  
  if (MSDUpdateStageInStatus(J,&DETime) == SUCCESS)
    {
    /* TODO: only set SysSMinTime if we know we are only overriding previous DS SysSMinTime changes */

    /*
    if (bmisset(&J->IFlags,mjifDataDependency) && (DETime != MMAX_TIME))
      {
      MJobSetAttr(J,mjaSysSMinTime,(void **)&DETime,mdfLong,mSet);
      }
    */
    }
  
  MLocalJobPostLoad(J);

  if ((!bmisset(&J->Flags,mjfNoRMStart) || !bmisset(&J->Flags,mjfClusterLock)) &&
      (MJobEval(J,J->SubmitRM,NULL,tmpMsg,&JobEvalFailReason) == FAILURE))
    {
    if (((JobEvalFailReason != marPartition) &&
        (JobEvalFailReason != marClass)) ||
        (bmisset(&J->IFlags,mjifFairShareFail)))
      {
      MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,tmpMsg);
  
      if (J == NULL)
        {
        /* job removed in MJobReject() */
  
        if (EMsg != NULL)
          strcpy(EMsg,tmpMsg);
  
        return(FAILURE);
        }
      }
    else
      {
      /* Job could not find partition, could be because 
          partition has not reported back to Moab yet.  Set 
          to the MAX in case J->SMinTime is already set. */

      J->SMinTime = MAX(MSched.Time + 2,J->SMinTime);
      }
    } /* END if ((!bmisset(&J->Flags,mjfNoRMStart) || ... */

  /* Need to check the dependencies first so that any INHERITRES template
      workflows will also have the dependencies.  A dependency blocks the
      entire workflow. */

  bmset(&RMXBM,mxaDependency);
  bmset(&RMXBM,mxaTrigNamespace);
  bmset(&RMXBM,mxaAdvRsv);
  bmset(&RMXBM,mxaHostList);

  if (MJobProcessExtensionString(J,J->RMXString,mxaNONE,&RMXBM,tmpMsg) == FAILURE)
    {
    if (EMsg != NULL)
      strcpy(EMsg,tmpMsg);

    bmclear(&RMXBM);

    return(FAILURE);
    }

  bmclear(&RMXBM);

  if (MJobProcessRestrictedAttrs(J,EMsg) == FAILURE)
    {
    return(FAILURE);
    }
  
  /* default job trigger processing moved to MJobApplyDefTemplate() */

  RQ = J->Req[0];  /* FIXME */

  /* FORMAT:                                  T    UN GN   CN   WC   JS   UP    QT NET  OS  AR  MC MEM  DC DSK */

  MDB(1,fRM) MLog("INFO:     job '%s' loaded: TC=%3d UGC=%8s,%8s,%8s WC=%6ld ST=%10s %3ld %10ld %6s %6s %2s %6d %2s %6d %ld\n",
    J->Name,
    J->Request.TC,
    J->Credential.U->Name,
    (J->Credential.G != NULL) ? J->Credential.G->Name : "NULL",
    (J->Credential.C != NULL) ? J->Credential.C->Name : "NULL",
    J->WCLimit,
    MJobState[J->State],
    J->UPriority,
    J->SubmitTime,
    (RQ->Opsys != 0) ? MAList[meOpsys][RQ->Opsys] : "-",
    (RQ->Arch != 0) ? MAList[meArch][RQ->Arch] : "-",
    MComp[(int)RQ->ReqNRC[mrMem]],
    RQ->ReqNR[mrMem],
    MComp[(int)RQ->ReqNRC[mrDisk]],
    RQ->ReqNR[mrDisk],
    J->ATime);

  if (MSched.Mode == msmNormal)
    {
    MOSSyslog(LOG_INFO,"new job %s loaded",
      J->Name);
    }

  /* determine if peer job has a data-staging hold */

  if (J->SystemID != NULL)
    {
    enum MHoldTypeEnum Hold = mhNONE;
    char SJobID[MMAX_NAME];

    /* convert to short-name */

    MJobGetName(NULL,J->DRMJID,J->DestinationRM,SJobID,sizeof(SJobID),mjnShortName);
    
    if (MSDRemoveJobHold(SJobID,&Hold) == SUCCESS)
      {
      MJobSetHold(J,Hold,0,mhrData,"data dependencies not yet fulfilled");
      }
    }

  if (!bmisset(&J->IFlags,mjifWasLoadedFromCP))
    {
    /* NOTE:  if limited job checkpoint is enabled, job may have been processed earlier */

    if ((MSched.LimitedJobCP == FALSE) || 
        (MSched.Iteration > 0) ||
        (J->SubmitTime > MCP.LastCPTime - MCONST_MINUTELEN))
      {
      MDB(4,fRM) MLog("INFO:     cannot locate checkpoint record for job '%s' - recreating submit event record\n",
        J->Name);

      MOWriteEvent((void *)J,mxoJob,mrelJobSubmit,"checkpoint record not found",MStat.eventfp,NULL);

      if (MSched.PushEventsToWebService == TRUE)
        {
        MEventLog *Log = new MEventLog(meltJobSubmit);
        Log->SetCategory(melcSubmit);
        Log->SetFacility(melfJob);
        Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
        Log->AddDetail("msg","checkpoint record not found");

        MEventLogExportToWebServices(Log);

        delete Log;
        }
      }
    }  /* END if (!bmisset(&J->IFlags,mjifWasLoadedFromCP)) */
  else
    {
    if (MSched.Iteration <= 0)
      {
      MOWriteEvent((void *)J,mxoJob,mrelJobSubmit,"loaded job from checkpoint",MStat.eventfp,NULL);

      if (MSched.PushEventsToWebService == TRUE)
        {
        MEventLog *Log = new MEventLog(meltJobSubmit);
        Log->SetCategory(melcSubmit);
        Log->SetFacility(melfJob);
        Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
        Log->AddDetail("msg","loaded job from checkpoint");

        MEventLogExportToWebServices(Log);

        delete Log;
        }
      }  /* END if (MSched.Iteration <= 0) */
    }  /* END else */

  if (MJOBISACTIVE(J))
    {
    MOReportEvent(J,J->Name,mxoJob,mttStart,J->StartTime,TRUE);
    MOReportEvent(J,J->Name,mxoJob,mttEnd,J->StartTime + J->SpecWCLimit[0],TRUE);
    }

  if (J->EffQueueDuration == -1)
    {
    /* SystemQueueTime is the time that the job was submitted to the resource manager.
     *
     * EffQueueDuration can be larger for jobs submitted later than other jobs when moab is catching up and using J->SystemQueueTime.
     * Example: moab processes at time 5 a job submitted at time 5. It's EffQeueuDuration is 0.
     * The next iteration moab processes the next job submitted at 5, but moab is processing at time 25
     * so its EffQueueDuration is 20. USEMOABCTIME was created to handle this.
     */

    if (MSched.UseMoabCTime)
      {
      J->EffQueueDuration = MSched.Time - J->CTime;

      MDB(7,fSCHED) MLog("INFO:    (MRMJobPostLoad) job %s new EffqueueDuration %ld; MSched.Time = %ld; J->CTime = %ld\n", /* BRIAN - debug (2400) */
        J->Name,
        J->EffQueueDuration,
        MSched.Time,
        J->CTime);
      }
    else if (MJOBISACTIVE(J))
      {
      J->EffQueueDuration = J->StartTime - J->SubmitTime;
      }
    else 
      {
      J->EffQueueDuration = MSched.Time - J->SystemQueueTime;
      
      MDB(7,fSCHED) MLog("INFO:    (MRMJobPostLoad) job %s new EffqueueDuration %ld; MSched.Time = %ld; J->EffQueueDuration = %ld\n", /* BRIAN - debug (2400) */
        J->Name,
        J->EffQueueDuration,
        MSched.Time,
        J->EffQueueDuration);
      }
    }

  /* array parsing moved to MJobProcessRestrictedAttr() */
  /* array handling moved to MUIJobSubmit() */
  /* MRMJobPostLoad() is too early to process arrays when submitted via msub, not everything has been loaded */

  MJobVerifyPolicyStructures(J);

/*
  if ((MSched.LimitedJobCP == FALSE) && !bmisset(&R->IFlags,mrmifLocalQueue))
*/
    MJobTransition(J,FALSE,FALSE);

  MJobWriteCPToFile(J);

#ifndef MYAHOO
  /* don't checkpoint here for Yahoo in order to increase performance--instead
   * we checkpoint in MUIJobSubmit */

  MOCheckpoint(mxoJob,(char *)J,FALSE,NULL);
#endif /* !MYAHOO */

  return(SUCCESS);
  }  /* END MRMJobPostLoad() */

/* END MRMJobLoad.c */
