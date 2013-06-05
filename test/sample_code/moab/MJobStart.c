/* HEADER */

      
/**
 * @file MJobStart.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "mcom.h"


enum MJobStartEnum {
  mjstfNONE = 0,
  mjstfDynamicClass,
  mjstfLien,
  mjstfNodeAllocation,
  mjstfParCreate,
  mjstfProvisioning,
  mjstfVMCreate,
  mjstfLAST };

/**
 * Start a single job.
 *
 * This function will attempt to populate J->NodeList and J->TaskMap.
 * If the job is a DVC (DVC.h) job, it will allocate IP addresses and hostnames
 * from GRES.
 * It will attempt to reserve allocations for the job through the
 * accounting manager. (TODO: exempt system jobs)
 *
 * It will check if the job requires co-allocation or needs to be migrated
 * to another RM.
 *
 * It will eventually call MRMJobStart() to start the job.
 * 
 * If in multi-partition environment and MSched.SchedPolicy
 * == mcpEarlistCompletion MJobStart may be called when evaluating an
 * early partition. If called, Moab should create a job reservation, set a
 * flag indicating job start is pending, and defer job start until the end of
 * the scheduling iteration.  If a subsequent partition can start the job with
 * an early completion time, this partition should release the
 * current reservation, and create a new reservation when all partitions
 * have been evaluated, jobs with the jobstartpending flag should be started.
 *
 * NOTE:  J->Req[X]->NodeList must be populated
 * NOTE: This routine does not check for overcommit
 *
 * @see MJobDistributeTasks() - child
 * @see MUGenerateSystemJobs() - child
 * @see MAMHandleStart() - child
 * @see MRMJobStart() - child - start job via RM
 * @see MJobProcessFailedStart() - child - resets job state if failure occurs
 *
 * NOTE:  On failure, MJobStart() should either hold the job or attach a job 
 *        message.
 *
 * @param J (I) [modified] The job to start
 * @param SEMsg (O) [optional,minsize=MMAX_LINE] Error message holder
 * @param SSC (O) [optional] Status code
 * @param Msg (I) reason job was started
 *
 * Step 1.0  Initialize 
 *   Step 1.1  Validate Job 
 *   Step 1.2  Handle 'syncwith' dependency pseudo-coalloc jobs  (optional)
 *   Step 1.3  Handle pre-start job initialization 
 *   Step 1.4  Distribute tasks 
 *     Step 1.4.1  Distribute physical compute resources 
 *     Step 1.4.2  Distribute virtual compute resources (optional) 
 *     Step 1.4.3  Create system jobs for creation of ondemand VM's (optional) 
 *   Step 1.5  Provision Compute Resources
 *     Step 1.5.1  Handle ondemand compute provisioning (optional) 
 *     Step 1.5.2  Handle Old-Style Service Provisioning (deprecated,optional) 
 *   Step 1.6  Verify AM credit/allocations exist for account (optional) 
 *   Step 1.7  Migrate jobs
 *     Step 1.7.1  Handle co-allocation job migration (optional) 
 *     Step 1.7.2  Handle standard RM migration (optional) 
 *   Step 1.8  Provision Non-Compute Environment
 *     Step 1.8.1  Handle network provisioning (optional) 
 *     Step 1.8.2  Handle storage provisioning (optional) 
 *     Step 1.8.3  Handle resource partition provisioning (optional) 
 *   Step 1.9  Enable dynamic job modification based on allocation (optional) 
 *   Step 1.10 Update job start statistics 
 * Step 2.0  Start Job 
 *   Step 2.1  Launch job via RM 
 *   Step 2.2  Launch system jobs (optional) 
 *   Step 2.3  Adjust job state and handle post-start job customization 
 *   Step 2.4  Move job to 'active' queue 
 *   Step 2.5  Reserve resources 
 *   Step 2.6  Update internal stats 
 * Step 3.0  Report start 
 *
 * NOTE: recommendations:  move 1.6 before 1.5
 *       consolidate 1.4, 1.5, 1.6, 1.8, and 1.9 into separate routines
 */

int MJobStart(
 
  mjob_t     *J,
  char       *SEMsg,
  int        *SSC,
  const char *Msg)

  {
  int      nindex;
 
  int      JTC;
  int      JNC;

  int      ReqTC;
  int      ReqNC;

  int      ReqOS = 0;

  mnode_t *N;

  enum MS3CodeDecadeEnum tmpS3C = ms3cNone;
  enum MAMJFActionEnum AMFailureAction;
 
  mqos_t   *QDef;
  mreq_t   *RQ;
  mgcred_t *A; 
  mrm_t    *R = NULL;
 
  int      SC;
 
  int      rqindex;
 
  double   tmpD;
 
  char     tmpEMsg[MMAX_LINE];
 
  double   SumNodeLoad;

  char    *EMsg;

  mbool_t  CoAllocMigrateRequired = FALSE;
  mbool_t  DoRetry = FALSE;

  mbitmap_t JStartBM;  /* bitmap of job start state */

  const char *FName = "MJobStart";

  MDB(2,fSCHED) MLog("%s(%s,%p,%p,%s)\n",
    FName,
    SEMsg,
    SSC,
    (J != NULL) ? J->Name : "NULL",
    (Msg != NULL) ? Msg : "NULL");

  /* Step 1.0  Initialize */

  EMsg = (SEMsg != NULL) ? SEMsg : tmpEMsg;
  EMsg[0] = '\0';

  if (SSC != NULL)
    *SSC = 0;

  ReqTC = 0;
  ReqNC = 0;

  /* Step 1.1  Validate Job */
 
  if (!MJobPtrIsValid(J))
    {
    MDB(2,fSCHED) MLog("ERROR:    invalid job pointer received\n");

    strcpy(EMsg,"invalid job");
 
    return(FAILURE);
    }

  if ((J->Array != NULL) && !bmisset(&J->Array->Flags,mjafIgnoreArray))
    {
    /* never start the master job of a job array */

    return(SUCCESS);
    }

  if (((J->Credential.U == NULL) ||
       (J->Credential.U->OID == 0)) && 
      !bmisset(&J->Flags,mjfSystemJob) && 
      (MSched.AllowRootJobs == FALSE))
    {
    strcpy(EMsg,"invalid job user");

    return(FAILURE);
    }

  if (J->OrigMinWCLimit > 0)
    {
    MJobExtendStartWallTime(J);
    }

  if (MLocalJobPostStart(J) == FAILURE)
    {
    MDB(2,fSCHED) MLog("INFO:     local constraints reject start of job %s\n",
      J->Name);

    strcpy(EMsg,"rejected by local job policies");

    return(FAILURE);
    }

  /* Step 1.2  Handle 'syncwith' dependency pseudo-coalloc jobs (optional) */

  if (bmisset(&J->IFlags,mjifMasterSyncJob))
    {
    /* MJobSyncStart() calls MJobStart() for each sync'ed job */

    MJobSyncStart(J);

    return(SUCCESS);
    }

  /* Step 1.3  Handle pre-start job initialization */

  J->SubState = mjsstNONE;

  bmset(&J->IFlags,mjifFailedNodeAllocated);
 
  RQ = J->Req[0];  /* FIXME */

  if (J->TaskMapSize < J->Request.TC)
    {
    /* Add plus 1 to account for possible GLOBAL node and to minimize
     * growing the taskmap. */
    if (MJobGrowTaskMap(J,(J->Request.TC - J->TaskMapSize) + 1) == FAILURE)
      {
      strcpy(EMsg,"cannot allocate memory for tasklist");

      return(FAILURE);
      }
    }

  /* Step 1.4  Distribute tasks across allocated resources */

  /* NOTE:  MJobDistributeTasks() populates J->NodeList and J->TaskMap */

  /* NOTE:  RQ->RMIndex should be set in ??? */

  if (RQ->RMIndex == -1)
    {
    if (MNLGetNodeAtIndex(&RQ->NodeList,0,&N) == SUCCESS)
      {
      RQ->RMIndex = N->RM->Index;
      }
    else
      {
      /* cannot determine req RM index, default to RM ')' */

      MDB(3,fSCHED) MLog("WARNING:  cannot determine RM for %s:%d\n",
        J->Name,
        RQ->Index);

      RQ->RMIndex = 0;
      }
    }

  /* Step 1.4.1  Distribute physical compute resources */
 
  if (bmisset(&J->Flags,mjfNoResources))
    {
    J->Request.TC = 0;
    J->Request.NC = 0;
    }
  else if (MJobDistributeTasks(
       J,
       &MRM[RQ->RMIndex],
       TRUE,
       &J->NodeList,  /* O - populated on success */
       NULL,
       0) == FAILURE)
    { 
    MDB(3,fSCHED) MLog("WARNING:  cannot distribute allocated tasks for job '%s'\n",
      J->Name);

    strcpy(EMsg,"cannot distribute tasks");

    MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

    return(FAILURE);
    }    /* END if (MJobDistributeTasks() == FAILURE) */

  /*  Check to see if we are starting a grid job which does not have a class - ticket 7447 */

  if((J->Credential.C == NULL) &&
     (MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS) &&
     (N->RM != NULL) &&
     (N->RM->Type == mrmtMoab) &&
     (N->RM->DefaultC != NULL))
    {
    char tmpLine[MMAX_LINE];

    /* only worry about setting the class if multiple partitions were requested */

    MPALToString(&J->SpecPAL,",",tmpLine);

    if ((strchr(tmpLine,',') == NULL) &&
        (strstr(tmpLine,"ALL") == NULL))
      {
      /* NO-OP - only one partition requested */
      }
    else
      {
      /* multiple partitions requested */

      J->Credential.C = N->RM->DefaultC;

      if(MJobCheckClassJLimits(J,J->Credential.C,FALSE,0,NULL,0) == FAILURE)
        {
        MDB(3,fSCHED) MLog("WARNING:    Job '%s' cannot run with default class '%s'\n",
          J->Name,
          J->Credential.C->Name);

        J->Credential.C = NULL;

        strcpy(EMsg,"cannot set default class");

        return(FAILURE);
        } /* END if MJobCheckClassJLimits() */
      } /* END if strchr() && strstr() */
    } /* END if J->Cred.C == NULL && ... */

  bmset(&JStartBM,mjstfNodeAllocation);

  /* Step 1.4.2  Distribute virtual compute resources (optional) */

  if (((J->System == NULL) && ((J->VMUsagePolicy == mvmupVMCreate) ||
        (bmisset(&J->IFlags,mjifMaster)))) ||
      ((J->VMUsagePolicy == mvmupVMCreate) &&
        (J->System != NULL) &&
        (J->System->JobType == msjtVMTracking)))
    {
    char *Name;
    char *RsvName;

    mhashiter_t HTI;

    int rc;
    mbool_t StartJob = FALSE;

#if 0
    /* limit the number of simultaneous vmcreate jobs being generated */

    if (J->VMUsagePolicy == mvmupVMCreate)
      {
      if (MSched.VMCreateThrottle > -1)
        {
        if (MJobCountVMCreateJobs(NULL) >= MSched.VMCreateThrottle)
          {
          MDB(3,fSCHED) MLog("WARNING:  vm create throttle reached for job %s (%d >= %d)\n",
            J->Name,
            MJobCountVMCreateJobs(NULL),
            MSched.VMCreateThrottle);

          snprintf(EMsg,MMAX_LINE,"vmcreate throttle reached");

          return(FAILURE);
          }
        else
          {
          MDB(3,fSCHED) MLog("WARNING:  vm create throttle enabled on jobstart for job %s (%d of %d)\n",
            J->Name,
            MJobCountVMCreateJobs(NULL),
            MSched.VMCreateThrottle);
          }
        }
      }
#endif

    rc = MAdaptiveJobStart(J,&StartJob,EMsg);

    if (StartJob == FALSE)
      {
      /* This means that the VMCreate job was just created but
          has not run yet.  When that job finishes, StartJob 
          will be true and we will continue past this statement */

      return(rc);
      }

    /* Check for any storage placeholder reservations */

    MUHTIterInit(&HTI);

    while (MUHTIterate(&J->Variables,&Name,(void **)&RsvName,NULL,&HTI) == SUCCESS)
      {
      if (!strncmp(Name,"STORAGERSV",strlen("STORAGERSV")))
        {
        mrsv_t *DeadRsv;

        /* placeholder reservation found, release the rsv */

        if (MRsvFind(RsvName,&DeadRsv,mraNONE) == FAILURE)
          {
          MRsvDestroy(&DeadRsv,TRUE,TRUE);
          }
        }
      }

    if ((J->System != NULL) &&
        (J->System->JobType == msjtVMTracking) &&
        (J->VMCreateRequest != NULL))
      {
      mvm_t *tmpVM = NULL;

      if (MVMFind(J->VMCreateRequest->VMID,&tmpVM) == SUCCESS)
        {
        tmpVM->J = J;
        }
      }
    }    /* END adaptive stuff */

  /* Step 1.5  Provision Ondemand Compute Resources */

  /* Step 1.5.1  Handle ondemand compute provisioning (optional) */

  /* Step 1.5.2  Handle Old-Style Service Provisioning (deprecated,optional) */

  if (bmisset(&J->IFlags,mjifMaster))
    {
    MReqGetReqOpsys(J,&ReqOS);
    }

  if ((MSched.OnDemandGreenEnabled == TRUE) &&
      (MSched.OnDemandProvisioningEnabled == FALSE) &&
      (!bmisset(&J->IFlags,mjifMaster)) &&
      (MUNLNeedsPower(&J->NodeList)))
    {
    marray_t JArray;

    mjob_t **JList;

    /* create one system job for all nodes that need to be provisioned */

    MUArrayListCreate(&JArray,sizeof(mjob_t *),4);

    /* determine target operating system */

    MUGenerateSystemJobs(
      (J->Credential.U != NULL) ? J->Credential.U->Name : NULL,
      J,
      &J->NodeList,
      msjtPowerOn,
      "power",
      -1,
      NULL,
      NULL,
      TRUE,
      FALSE,
      FALSE, /* force execution */
      &JArray);

    JList = (mjob_t **)JArray.Array;

    if ((JList == NULL) || (JList[0] == NULL))
      {
      strcpy(EMsg,"error in provisioning");

      return(FAILURE);
      }

    MJobTransferAllocation(JList[0],J);

    MULLAdd(&JList[0]->System->Dependents,J->Name,NULL,NULL,NULL);

    /* Apply templates for learning */

    MJobSetDefaults(JList[0],TRUE);

    /* swap out J and J->JGroup->SubJobs */

    MJobSetDependency(J,mdJobSuccessfulCompletion,JList[0]->Name,NULL,0,FALSE,NULL);

    /* create adjacent reservation */

    bmset(&J->IFlags,mjifReqTimeLock);

    MRsvJCreate(J,NULL,MSched.Time + JList[0]->SpecWCLimit[0],mjrTimeLock,NULL,FALSE);

    J = JList[0];

    MUArrayListFree(&JArray);
    }

  if (bmisset(&J->IFlags,mjifMaster) &&
      (J->System == NULL) &&
      (J->Credential.Q != NULL) &&
      (bmisset(&J->Credential.Q->Flags,mqfProvision)) &&
      (MUNLNeedsOSProvision(ReqOS,FALSE,&J->NodeList,NULL)))
    {
    marray_t JArray;

    mjob_t **JList;

    /* create one system job for all nodes that need to be provisioned */

    MUArrayListCreate(&JArray,sizeof(mjob_t *),4);

    if (ReqOS <= 0)
      {
      /* hold job */

      MJobSetHold(J,mhBatch,0,mhrAdmin,"job is missing requested operating system");

      return(FAILURE);
      }

    /* determine target operating system */

    MUGenerateSystemJobs(
      (J->Credential.U != NULL) ? J->Credential.U->Name : NULL,
      J,
      &J->NodeList,
      msjtOSProvision,
      "provision",
      ReqOS,
      NULL,
      NULL,
      TRUE,
      FALSE,
      FALSE, /* force execution */
      &JArray);

    JList = (mjob_t **)JArray.Array;

    if ((JList == NULL) || (JList[0] == NULL))
      {
      strcpy(EMsg,"error in provisioning");

      return(FAILURE);
      }

    MJobTransferAllocation(JList[0],J);

    MULLAdd(&JList[0]->System->Dependents,J->Name,NULL,NULL,NULL);

    /* Apply templates for learning */

    MJobSetDefaults(JList[0],TRUE);

    /* swap out J and J->JGroup->SubJobs */

    MJobSetDependency(J,mdJobSuccessfulCompletion,JList[0]->Name,NULL,0,FALSE,NULL);

    /* create adjacent reservation */

    bmset(&J->IFlags,mjifReqTimeLock);

    MRsvJCreate(J,NULL,MSched.Time + JList[0]->SpecWCLimit[0],mjrTimeLock,NULL,FALSE);

    J = JList[0];

    MUArrayListFree(&JArray);
    }

  /* Step 1.6  Verify AM credit/allocations exist for account (optional) */

  if (MAMHandleStart(&MAM[0],
        (void *)J,
        mxoJob,
        &AMFailureAction,
        EMsg,
        &tmpS3C) == FAILURE)
    {
    MDB(3,fSTRUCT) MLog("WARNING:  Unable to register job start with accounting manager for job '%s', reason: '%s', message: '%s'\n",
      J->Name,
      (tmpS3C > ms3cNone && tmpS3C < ms3cLAST && MS3CodeDecade[tmpS3C] != NULL)
        ? MS3CodeDecade[tmpS3C]
        : "Unknown Failure",
      EMsg[0] != '\0' ? EMsg : "");

    /* 'runalways' jobs should never fail due to accounting manager issues */
    if ((!bmisset(&J->IFlags,mjifRunAlways)) &&
        (!bmisset(&J->IFlags,mjifRunNow)))
      {
      enum MHoldReasonEnum  HoldReason;

      /* Log failure action */
      MDB(3,fSCHED) MLog("INFO:     Taking AM failure action: %s\n",
        MAMJFActionType[AMFailureAction]);

      /* Apply Fallback Account or QOS */
      if (tmpS3C == ms3cNoFunds)
        {
        mqos_t *Q;
        mbool_t RetryWithFallbackInfo = FALSE;

        if ((MAM[0].FallbackAccount[0] != '\0') &&
            (MAcctFind(MAM[0].FallbackAccount,&A) == SUCCESS) &&
            (J->Credential.A != A))
          {
          /* insufficient balance in primary account, try fallback account */

          if (AMFailureAction == mamjfaNONE)
            AMFailureAction = mamjfaRetry;
 
          J->Credential.A = A;

          RetryWithFallbackInfo = TRUE;

          bmset(&J->IFlags,mjifFBAcctInUse);

          MRMJobModify(J,"Account_Name",NULL,A->Name,mSet,"Fallback account.",NULL,NULL);

          /* CHANGE: don't clear tmpS3C as we need to completely reevaluate
                     the job based on its new credentials (DRW 8/28/07) */
          /*tmpS3C = ms3cNone;*/
 
          if ((MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&QDef) == FAILURE) ||
              (J->QOSRequested == NULL))
            {
            MJobSetQOS(J,QDef,0);
            }
 
          sprintf(EMsg,"Primary account adjusted to fallback account");
          }
        else if ((MAM[0].FallbackQOS[0] != '\0') &&
                 (MQOSFind(MAM[0].FallbackQOS,&Q) == SUCCESS) &&
                 (J->Credential.Q != Q))
          {
          if (AMFailureAction == mamjfaNONE)
            AMFailureAction = mamjfaRetry;

          /* insufficient allocations in primary account, assign fallback qos */

          J->Credential.Q = Q;

          RetryWithFallbackInfo = TRUE;

          bmset(&J->IFlags,mjifFBQOSInUse);

          /* CHANGE: don't clear tmpS3C as we need to completely reevaluate
                     the job based on its new credentials (DRW 8/28/07) */
          /*tmpS3C = ms3cNone;*/
          }

        if (EMsg[0] == '\0')
          strcpy(EMsg,"Unable to register job start with accounting manager -- Insufficient funds");

        /* If we are now using the fallback account or QOS,
         * do not allow the job to be put on hold or canceled (see below),
         * and schedule the job for re-evaluation right away. */
        if (RetryWithFallbackInfo == TRUE)
          {
          mulong tmpL;

          tmpL = MSched.Time + 1;

          MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mSet);

          MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

          return(FAILURE);
          }

        /* NOTE:  fall through and handle non-fallback 'no funds' case below */
        }  /* END if (tmpS3C == ms3cNoFunds) */
  
      /* Set hold reason */
      if (tmpS3C == ms3cNoFunds)
        {
        HoldReason = mhrNoFunds;
        }
      else if (tmpS3C == ms3cServerBusinessLogic)
        {
        HoldReason = mhrAMReject;
        }
      else
        {
        HoldReason = mhrAMFailure;
        }

      switch (AMFailureAction)
        {
        case mamjfaCancel:

          MJobCancel(J,EMsg,FALSE,NULL,NULL);

          MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

          return(FAILURE);

          /* NOTREACHED */

          break;

        case mamjfaDefer:

          MJobSetHold(J,mhDefer,MSched.DeferTime,HoldReason,EMsg);

          MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

          /* Transition to reflect hold in non-blocking calls */

          MJobTransition(J,FALSE,FALSE);

          return(FAILURE);

          /* NOTREACHED */

          break;

        case mamjfaHold:

          MJobSetHold(J,mhSystem,MMAX_TIME,HoldReason,EMsg);

          MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

          /* Transition to reflect hold in non-blocking calls */

          MJobTransition(J,FALSE,FALSE);

          return(FAILURE);

          /* NOTREACHED */

          break;

        case mamjfaRetry:

          {
          mulong tmpL;

          tmpL = MSched.Time + MSched.MaxRMPollInterval;
          MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mSet);

          MJobReleaseRsv(J,FALSE,FALSE);

          MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

          MJobTransition(J,FALSE,FALSE);

          return(FAILURE);
          }

          /* NOTREACHED */

          break;

        default:

          /* NO-OP, ignore */

          break;
        }  /* END switch (AMFailureAction) */
      }  /* END if ((!bmisset(&J->IFlags,mjifRunAlways)) ...) */
    }  /* END if (MAMHandleStart() == FAILURE) */

  if (bmisset(&J->Flags,mjfGResOnly))
    {
    MRMAddInternal(&R);
    }
  else if (MNLGetNodeAtIndex(&RQ->NodeList,0,&N) == SUCCESS)
    {
    R = N->RM;
    }
  else
    {
    MRMAddInternal(&R);
    }

  /* Step 1.7  Migrate Jobs */

  /* Step 1.7.1  Handle co-allocation job migration (optional) */

  if ((bmisset(&J->Flags,mjfCoAlloc)) && (J->System == NULL))
    {
    /* must migrate co-alloc job if multi-req job created which spans RM's */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      if ((MNLGetNodeAtIndex(&RQ->NodeList,0,&N) == FAILURE) ||
          (N->RM == NULL))
        {
        continue;
        }

      if ((N->RM != J->SubmitRM) &&
          (J->DestinationRM == NULL))
        {
        CoAllocMigrateRequired = TRUE;

        break;
        }
      }  /* END for (rqindex) */
    }    /* END if (bmisset(&J->Flags,mjfCoAlloc) && ...) */

  /* Step 1.7.2  Handle standard RM migration (optional) */

  /* 3 requirements:  1) must be submitted to local queue
                      2) must require coallocation or not yet be migrated
                      3) must not be NoRMStart job or must be grid job 
  
     NOTE:  grid exception to NoRMStart job allows grid simulation (see Josh)
   */

  if (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) &&
     ((CoAllocMigrateRequired == TRUE) || (J->DestinationRM == NULL)) &&
     (!bmisset(&J->Flags,mjfNoRMStart) || ((R != NULL) && (R->Type == mrmtMoab))))
    {
    char tEMsg[MMAX_LINE];
    enum MStatusCodeEnum tSC = mscNoError;

    /* NOTE:  MJobMigrate() will migrate to all co-alloc RM's */

    if (J->DestinationRM == NULL)
      {
      if ((R == NULL) || 
          (MJobMigrate(&J,R,FALSE,NULL,tEMsg,&tSC) == FAILURE))
        {
        MJobProcessMigrateFailure(J,R,tEMsg,tSC);

        if (tEMsg[0] != '\0')
          strcpy(EMsg,tEMsg);
        else
          strcpy(EMsg,"cannot migrate job");

        MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

        return(FAILURE);
        }  /* END if (MJobMigrate == FAILURE) */
      }
    }    /* END if (bmisset(&J->SRM->IFlags,mrmifLocalQueue) && ...) */

  if ((J->TemplateExtensions != NULL) && 
      !bmisset(&J->Flags,mjfNoRMStart) &&
      (J->TemplateExtensions->WorkloadRM != NULL) && 
      (J->System == NULL) &&
      (J->DestinationRM != J->TemplateExtensions->WorkloadRM))
    {
    /* NOTE: must migrate job to configured workload manager */
    /*       WorkloadRM overrides any scheduler decision */

    char tEMsg[MMAX_LINE];
    enum MStatusCodeEnum tSC = mscNoError;

    /* NOTE:  MJobMigrate() will migrate to all co-alloc RM's */

    if (MJobMigrate(&J,J->TemplateExtensions->WorkloadRM,FALSE,NULL,tEMsg,&tSC) == FAILURE)
      {
      if (tEMsg[0] != '\0')
        strcpy(EMsg,tEMsg);
      else
        strcpy(EMsg,"cannot migrate job");

      MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

      return(FAILURE);
      }    /* END if (MJobMigrate == FAILURE) */
    }      /* END if ((J->TX != NULL) ...) */

  /* Step 1.8.3  Handle resource partition provisioning (optional) */

  if ((R != NULL) &&
      ((R->SubType == mrmstXT3) ||
       (R->SubType == mrmstXT4) || 
       bmisset(&R->IFlags,mrmifSetTaskmapEnv)))
    {
    char     tEMsg[MMAX_LINE];
    mbool_t  createAllocPartition = TRUE;
    mnode_t *externalN;

    if ((R->SubType == mrmstXT4) &&
        (MNLGetNodeAtIndex(&J->NodeList,0,&externalN) == SUCCESS) &&
        (strcmp(MPar[externalN->PtIndex].Name,"external") == 0))
      {
      /* Treat job as normal torque job. */
      createAllocPartition = FALSE;
      }

    if (createAllocPartition == TRUE)
      {
      /* job is starting, create allocation partition */
      if (MJobCreateAllocPartition(J,R,tEMsg,&SC) == FAILURE)
        {  
        MDB(3,fSCHED) MLog("WARNING:  cannot create allocation partition for job %s - %s\n",
          J->Name,
          tEMsg);

        if (tEMsg[0] != '\0')
          MJobSetAttr(J,mjaMessages,(void **)tEMsg,mdfString,mSet);

        if (EMsg != NULL)
          strncpy(EMsg,tEMsg,MMAX_LINE);

        /* if job is preempting, alloc partition may not be ready */

        if (J->DispatchTime <= 0)
          {
          /* NOTE: DispatchTime normally set in MRMJobStart() */

          J->DispatchTime = MSched.Time;
          }

        if (MSched.Time < J->DispatchTime + MPar[0].JobRetryTime)
          {
          /* NOTE:  job should maintain existing reservation and continue to retry
                    use of reserved resources via MJobScheduleRJobs() until 
                    JOBRETRYTIME is reached */

          J->InRetry = TRUE;

          if (SSC != NULL)
            *SSC = SC;

          return(FAILURE);
          }

        MJobProcessFailedStart(J,&JStartBM,mhrRMReject,tEMsg);

        return(FAILURE);
        }  /* END if (MJobCreateAllocPartition() == FAILURE) */

      bmset(&JStartBM,mjstfParCreate);
      }
    }    /* END if ((R->SubType == mrmstXT4) || ...) */

  /* Step 1.9  Enable dynamic job modification based on allocation (optional) */

  if ((!bmisset(&J->IFlags,mjifClassSpecified) || (J->Credential.C == NULL)) && 
      (J->Req[0]->SetType == mrstClass) &&
      (J->Req[0]->SetIndex > 0))
    {
    if (MRMJobModify(
         J,
         "class",
         NULL,
         MClass[J->Req[0]->SetIndex].Name,
         mSet,
         NULL,
         NULL,
         NULL) == FAILURE)
      {
      MJobProcessFailedStart(J,&JStartBM,mhrRMReject,"cannot set dynamic job class");

      return(FAILURE);
      }

    MJobSetAttr(J,mjaClass,(void **)MClass[J->Req[0]->SetIndex].Name,mdfString,mSet);

    bmset(&JStartBM,mjstfDynamicClass);
    }  /* END if ((J->Cred.C == NULL) && ...) */

  /* full environment now provisioned */

  /* Step 1.10  Update job start statistics */

  if ((MSched.Mode != msmTest) && (MSched.Mode != msmMonitor))
    {
    J->StartCount++;

    /* NOTE:  currently call routine each time job is started even on retry */

    /* should this be chnaged */

    MStatUpdateStartJobUsage(J);

    MSched.JobStartPerIterationCounter++;
    }

  /* Lock job array into partition */

  if ((bmisset(&J->Flags,mjfArrayJob)) && (MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS))
    {
    if (((bmisset(&J->Flags,mjfArrayJobParLock)) || 
         (MSched.ArrayJobParLock == TRUE)) && 
         (!bmisset(&J->Flags,mjfArrayJobParSpan)))
      {
      MJobArrayLockToPartition(J,&MPar[N->PtIndex]);
      }
    }

  /* Step 2.0  Start Job */

  /* Step 2.1  Launch job via RM */

#ifdef __MDEBUG
  fprintf(stdout,"%ld INFO:  Starting Job '%s', Iteration '%d'\n",
    MSched.Time,
    J->Name,
    MSched.Iteration);
#endif  /* __MDEBUG */

  if (!bmisset(&J->IFlags,mjifProlog) &&
      !bmisset(&J->IFlags,mjifDataDependency) &&
      (MRMJobStart(J,EMsg,&SC) == FAILURE))
    {
    mbool_t JobHoldSent = FALSE;

    if ((MSched.Mode != msmMonitor) && (MSched.Mode != msmTest))
      {
      MDB(3,fSCHED) MLog("WARNING:  cannot start job %s through resource manager\n",
        J->Name);
      }

    DoRetry = FALSE;

    if (MPar[0].JobRetryTime > 0)
      {
      if (J->DispatchTime == 0)
        J->DispatchTime = MSched.Time;

      if (MSched.Time < J->DispatchTime + MPar[0].JobRetryTime)
        {
        /* verify failure is transient */

        /* NYI */

        /* continue to retry job launch */

        MDB(5,fSCHED) MLog("INFO:     %ld seconds left in JOBRETRYTIME\n",
          (J->DispatchTime + MPar[0].JobRetryTime) - MSched.Time);

        DoRetry = TRUE;

        J->InRetry = TRUE;

        if (EMsg[0] != '\0')
          {
          if ((J->DestinationRM == NULL) || 
              (J->DestinationRM->Type != mrmtPBS) || 
              (J->PreemptIteration < MSched.Iteration))
            {
            /* race conditions can occur in PBS/TORQUE RM's if preemption is 
               initiated on the same iteration as job started */

            MJobSetAttr(J,mjaMessages,(void **)EMsg,mdfString,mSet);
            }
          }

        if ((J->Rsv == NULL) &&
            (bmisset(&J->Flags,mjfPreempted)) &&
            (MSched.GuaranteedPreemption == TRUE))
          {
          /* normally MRsvJCreate() only happens happen we have successfully started 
             the job, but if we are doing guaranteed preemption then we need to 
             lock the job to the already scheduled (and preempted) nodes and keep
             it there during JobRetryTime */

          MRsvJCreate(J,NULL,MSched.Time,mjrPriority,NULL,FALSE);
          }
        }
      else
        {
        bmunset(&J->Flags,mjfPreempted);
        bmunset(&J->SpecFlags,mjfPreempted);

        J->DispatchTime = 0;

        J->InRetry = FALSE;

        MDB(2,fSCHED) MLog("WARNING:  cannot start job %s through resource manager (giving up after trying for %ld seconds)\n",
          J->Name,
          MPar[0].JobRetryTime);

        if ((JobHoldSent == FALSE) && (J->StartCount >= MSched.DeferStartCount))
          {
          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrAPIFailure,EMsg);

          JobHoldSent = TRUE;
          }
        }  /* END else */
      }    /* END if (MPar[0].JobRetryTime > 0) */

    /* MSched.DeferStartCount remains the same but J->StartCount continues to
       increment if the job cannot start.  Therefore we must calculate the upper
       bound for J->StartCount using MSched.DeferStartCount and J->DeferCount */

    if ((DoRetry == FALSE) && 
        (SC == mscRemoteFailure) && 
       ((J->StartCount >= (MSched.DeferStartCount * (J->DeferCount+1))) || 
         bmisset(&J->Flags,mjfNoQueue)))
      {
      MDB(2,fSCHED) MLog("ALERT:    job '%s' deferred after %d failed start attempts (API failure on last attempt)\n",
        J->Name,
        J->StartCount);
 
      if ((MSched.Mode != msmMonitor) && (JobHoldSent == FALSE))
        {
        MJobSetHold(J,mhDefer,MSched.DeferTime,mhrAPIFailure,EMsg);

        JobHoldSent = TRUE;
        }
      }
 
    if (DoRetry == FALSE) 
      {
      /* release resource allocations */

      if ((R != NULL) && 
          ((R->SubType == mrmstXT3) || (R->SubType == mrmstXT4)))
        {
        MJobDestroyAllocPartition(J,R,"job removed");
        }
      }    /* END if (DoRetry == FALSE) */

    /* NOTE:  should rebuild rsv table to 'fill' hole */

    /* NYI */

    /* EMsg populated by MRMJobStart() */

    MJobProcessFailedStart(J,&JStartBM,mhrNONE,EMsg);

    if (SSC != NULL)
      *SSC = SC;
 
    return(FAILURE);
    }  /* END if ((J->Prolog == NULL) && ...) */

  /* Step 2.2  Launch system jobs (optional) */

  if (J->System != NULL)
    {
#if 0
    if (MSysJobCheckThrottles(J) == FALSE)
      {
      /* cannot start job due to throttle limit */

      return(FAILURE);
      }
#endif

    if (MSysJobStart(J,NULL,EMsg) == FAILURE)
      {
      switch (J->System->CompletionPolicy)
        {
        case mjcpPowerOff:
        case mjcpPowerOn:
        case mjcpReset:

          bmset(&J->System->EFlags,msjefPowerRMFail);

          MSysJobProcessFailure(J);
 
          return(FAILURE);

          break;  /* not reached */
          
        case mjcpVMCreate:

          /* is this the correct way to handle this? */
          /* we can expand this later to do smarter things like retry */

          bmset(&J->IFlags,mjifCancel);

          return(FAILURE);

          break;  /* not reached */

        case mjcpOSProvision: 
        case mjcpOSProvision2:
  
          {
          mln_t *tmpL;
          mjob_t *tmpJ;
          
          bmset(&J->System->EFlags,msjefProvRMFail);

          if (J->System->Dependents != NULL) 
            {
            /* assume that if job has a dependent, it has only one dependent
             * and that dependent is a job with the on-demand OS provisioning
             * flag set */

            assert(J->System->Dependents->Name != NULL);
            assert(J->System->Dependents->Next == NULL);
    
            tmpL = &J->System->Dependents[0];

            if (MJobFind(tmpL->Name,&tmpJ,mjsmJobName) == SUCCESS)
              {
              MJobTransferAllocation(tmpJ,J);

              MJobReleaseRsv(tmpJ,FALSE,FALSE);
              }
           }

          bmset(&J->IFlags,mjifCancel);

          /* NOTE: don't remove jobs in this routine 

          MS3RemoveLocalJob(MSched.InternalRM,J->Name);

          MJobProcessRemoved(&J);
          */

          return(FAILURE);
          }

          break;  /* not reached */

        case mjcpVMTracking:

          break;

        default:

          return(FAILURE);

          /*NOTREACHED*/

          break;  
        }  /* END switch (J->System->CompletionPolicy) */
      }    /* END if (MJobStartSystemJob() == FAILURE)*/
    }      /* END if (J->System == NULL)*/

  /* Step 2.3  Adjust job state and handle post-start job customization */

  if (bmisset(&J->Flags,mjfArrayJob))
    {
    MJobArrayUpdateJobInfo(J,mjsRunning);
    }

  J->InRetry = FALSE;

  if (bmisset(&J->IFlags,mjifProlog))
    {
    /* launch prologue and set appropriate state on job */

    MJobCheckProlog(J,NULL);
    }

  if (bmisset(&J->IFlags,mjifIsDynReq))
    {
    /* issue MRMSystemModify() */

    /* NYI */
    }

#ifdef MSIC
  {
  /* NOTE:  MSched.Time is only updated at the start of each iteration so may be
            stale by the time this particular jobs is started in environments with
            long-running scheduling cycles.  This can cause timeouts and time-based
            policies to trigger prematurely.  However, there may be negative side
            affects associated with J->StartTime being in the future relative to
            MSched.Time */

  mulong now;

  MUGetTime((mulong *)&now,mtmNONE,&MSched);

  J->StartTime     = now;
  J->DispatchTime  = now;
  }
#else /* MSIC */
  J->StartTime     = MSched.Time;
  J->DispatchTime  = MSched.Time;
#endif /* MSIC */

  J->RemainingTime = J->WCLimit;
 
  /* release all nodes reserved by job */
 
  MJobReleaseRsv(J,TRUE,TRUE);
 
  /* determine number of nodes selected */
 
  JTC = 0;

  J->FloatingNodeCount = 0;

  if (bmisset(&J->IFlags,mjifDataDependency))
    {
    MJobSetState(J,mjsStaging);
    }
  else if ((R != NULL) && (R->Type == mrmtMoab))
    {
    /* for grid, do not assume that the job successfully started remotely
       and is running -- mark job Starting until a workload query confirms 
       the job has started and is running, but don't start master of job array */

    if (J->Array == NULL)
      {
      MJobSetState(J,mjsStarting);
      }

    if (J->SubState == mjsstMigrated)
      J->SubState = mjsstNONE;
    }
  else
    {
    MJobSetState(J,mjsRunning);

    /* at this point it is safe to show user that job is attempting to run */

    if (J->SubState == mjsstMigrated)
      J->SubState = mjsstNONE;
    }

  if (!bmisset(&J->SpecFlags,mjfNoResources))
    {
    enum MNodeAccessPolicyEnum NAPolicy;
    mcres_t tmpDRes = {0};
    
    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];
   
      if (bmisset(&J->Flags,mjfCoAlloc))
        {
        /* NO-OP */
        }
      else if (!MNLIsEmpty(&RQ->NodeList))
        {
        MNLGetNodeAtIndex(&RQ->NodeList,0,&N);

        RQ->PtIndex = N->PtIndex; 
        }
      else
        {
        mpar_t *P;
   
        /* no nodes located */
   
        if ((J->DestinationRM != NULL) && (MParAdd(J->DestinationRM->Name,&P) == SUCCESS))
          {
          RQ->PtIndex = P->Index;
          }
        else
          {
          strcpy(EMsg,"cannot assign partition");
   
          /* job started within RM but cannot assign partition, what to do 
             with job? */
   
          /* should job be cancelled? should J->NodeList be cleared? */
   
          return(FAILURE);
          }
   
        RQ->PtIndex = 1;
        }
        
      MCResCopy(&tmpDRes,&RQ->DRes);

      if (bmisset(&MPar[RQ->PtIndex].Flags,mpfSharedMem))
        {
        MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);

        /* Shared jobs have to fit on one node, so remove dedicated memory,
         * singlejob job's span nodes and use all resources. */

        if (NAPolicy == mnacShared)
          tmpDRes.Mem = RQ->ReqMem / RQ->TaskCount;
        }
      else
        {
        NAPolicy = RQ->NAccessPolicy;
        }

      /* modify node usage */
   
      if (!bmisset(&J->Flags,mjfMeta))
        {
        /* META jobs are just containers for other jobs, they have reservations but
           their usage will not show up on the node and the node will be available for 
           other jobs to run inside this one */

        MNLAdjustResources(
          &RQ->NodeList,
          NAPolicy,
          tmpDRes,
          &ReqTC,
          &ReqNC,
          &SumNodeLoad);
        }
      else
        {
        for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,NULL) == SUCCESS;nindex++)
          {
          ReqTC += MNLGetTCAtIndex(&J->NodeList,nindex);
          }       
        }
  
      /* Update SMPNode stats with the newly started job */

      if ((bmisset(&MPar[RQ->PtIndex].Flags,mpfSharedMem)) &&
          (MPar[0].NodeSetPriorityType == mrspMinLoss))
        {
        if (J->Req[0] != NULL)
          MSMPNodeUpdateWithNL(J,&J->Req[0]->NodeList);

        MDB(7,fSCHED) MSMPPrintAll();
        }
   
      JTC += ReqTC;
   
      RQ->TaskCount = ReqTC;

      /* determine possible impact of job start on other workload */

      if (!MSNLIsEmpty(&RQ->DRes.GenericRes))
        {
        int grindex;

        for (grindex = 1;grindex < MSched.M[mxoxGRes];grindex++)
          {
          if (MGRes.Name[grindex][0] == '\0')
            break;

          if ((MSNLGetIndexCount(&RQ->DRes.GenericRes,grindex) > 0) && 
              (MGRes.GRes[grindex]->StartDelay > 0))
            {
            MQueueApplyGResStartDelay(MGRes.GRes[grindex]->StartDelay,grindex);
            }
          }    /* END for (grindex) */
        }
      }  /* END for (rqindex) */
    }    /* END if (!bmisset(&J->SpecFlags,mjfNoResources)) */

  /* Step 2.4  Move job to 'active' queue */

  MJobTransition(J,FALSE,FALSE);

  /* remove job from eligible stats */

  MStatAdjustEJob(J,NULL,-1);

  MStat.IdleJobs--;

  /* NOTE:  job nodecount != sum of reqnc */

  JNC = 0;

  for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,NULL) == SUCCESS;nindex++)
    {
    JNC ++;
    }

  /* add job to active job list, update usage stats */

  J->Alloc.NC = JNC;
  J->Alloc.TC = JTC;

  if (!bmisset(&J->IFlags,mjifIsDynReq))
    {
    MQueueAddAJob(J);
    }
  else
    {
    /* merge job with master */

    /* NYI */

    /* adjust J to point to master J */
    }

  MUNLGetMinAVal(&J->NodeList,mnaSpeed,NULL,(void **)&tmpD);

  J->WCLimit = (long)((double)J->SpecWCLimit[0] / tmpD);
 
  /* Step 2.5:  Reserve resources */
 
  if (!bmisset(&J->SpecFlags,mjfRsvMap) && 
      !bmisset(&J->Flags,mjfNoResources) &&
      !bmisset(&J->SpecFlags,mjfNoResources) &&
      (MRsvJCreate(J,NULL,MSched.Time,mjrActiveJob,NULL,FALSE) == FAILURE))
    {
    MDB(0,fSCHED) MLog("ERROR:    cannot create reservation for job '%s'\n",
      J->Name);
 
    J->EState = mjsIdle;

    strcpy(EMsg,"cannot create job reservation");

    /* job started within RM but cannot create rsv, what to do with job? */

    /* should job be cancelled? should J->NodeList be cleared? */

    /* NOTE:  do not call MJobProcessFailedStart() because job started */
 
    return(FAILURE);
    } 

#if 0
  MJobAllocateGResType(J,EMsg);
#endif

  /* Step 2.6:  Update internal stats */

  /* Update standing reservations MAXJOB Policy */

  MSRUpdateJobCount(J,TRUE);

  RQ = J->Req[0];  /* require single partition per job */

/*
  MParUpdate(&MPar[RQ->PtIndex],FALSE,FALSE);
*/

  /* job successfully started - update stats */

  MJobUpdateResourceCache(J,NULL,0,NULL);
 
  MJobAddToNL(J,NULL,TRUE);

  MLocalAdjustActiveJobStats(J);

  /* Step 3.0  Report start */

  if (J->Triggers != NULL)
    {
    MOReportEvent((void *)J,J->Name,mxoJob,mttStart,J->StartTime,TRUE);
    MOReportEvent((void *)J,J->Name,mxoJob,mttEnd,J->StartTime + J->SpecWCLimit[0],TRUE);
    }

  MLocalJobPostStart(J);

  if (J->Depend != NULL)
    {
    /* check any before dependencies */

    mdepend_t *JobDep = J->Depend;
    mjob_t *OtherJob;

    while (JobDep != NULL)
      {
      if (JobDep->Type != mdJobBefore)
        {
        JobDep = JobDep->NextAnd;

        continue;
        }

      OtherJob = NULL;

      if (MJobFind(JobDep->Value,&OtherJob,mjsmExtended) == FAILURE)
        {
        MDB(4,fSTAT) MLog("WARNING:  could not find job '%s' to update %s dependency for job '%s'\n",
          JobDep->Value,
          MDependType[JobDep->Type],
          J->Name);
        }
      else
        {
        mdepend_t *OtherDep;

        OtherDep = OtherJob->Depend;

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
        } /* END else */

      JobDep = JobDep->NextAnd;
      } /* END while (JobDep != NULL) */
    } /* END if (J->Depend != NULL) */

  /* Mark workflow VC (if exists) as started */

  if (J->ParentVCs != NULL)
    {
    mln_t *tmpVCL = NULL;
    mvc_t *tmpVC = NULL;

    for (tmpVCL = J->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
      {
      tmpVC = (mvc_t *)tmpVCL->Ptr;

      if (tmpVC == NULL)
        continue;

      if (bmisset(&tmpVC->Flags,mvcfWorkflow))
        {
        bmset(&tmpVC->Flags,mvcfHasStarted);

        break;
        }
      }  /* END for (tmpVCL) */
    }  /* END if (J->ParentVCs != NULL) */

  MDB(2,fSCHED) MLog("INFO:     starting job '%s'\n",
    J->Name);
 
  MDB(2,fSCHED)
    MJobShow(J,0,NULL);
 
  return(SUCCESS);
  }  /* END MJobStart() */


/**
 * Process failed job start by undoing system level changes associated with 
 * job and restoring job state where appropriate.
 *
 * @see MJobStart() - parent 
 * 
 * NOTE:  should handle job reject as well inside MRMJobPostUpdate() - NYI
 */

int MJobProcessFailedStart(

  mjob_t               *J,          /* I (modified) */
  mbitmap_t            *JobStartBM, /* I */
  enum MHoldReasonEnum  RIndex,     /* I */
  const char           *EMsg)       /* I */

  {
   enum MS3CodeDecadeEnum tmpS3C = ms3cNone;
 
  if (J == NULL)
    {
    return(FAILURE);
    }

  if (bmisset(JobStartBM,mjstfNodeAllocation))
    {
    MNLClear(&J->NodeList);
    J->TaskMap[0]    = -1;
    }

  if (bmisset(JobStartBM,mjstfProvisioning))
    {
    /* NYI */
    }

  if (bmisset(JobStartBM,mjstfVMCreate))
    {
    /* NYI */
    }

  if (bmisset(JobStartBM,mjstfParCreate))
    {
    /* dynamic RM allocation partition created - ie XT4 */

    /* NYI */
    }

  if (bmisset(JobStartBM,mjstfLien))
    {
    /* NYI */
    }

   /* if the job has an AM reservation, remove it */
 
   if (bmisset(&J->IFlags,mjifAMReserved))
     {
     MDB(3,fSCHED) MLog("INFO:     Job %s failed to start, removing lien.\n", J->Name);
 
     if (MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,&tmpS3C) == FAILURE)
       {
       MDB(3,fSCHED) MLog("WARNING:  Unable to register job deletion with accounting manager for job '%s', reason: '%s'\n",
         J->Name,
         (tmpS3C > ms3cNone && tmpS3C < ms3cLAST && MS3CodeDecade[tmpS3C] != NULL) ? MS3CodeDecade[tmpS3C] : "Unknown Failure");
       }
     }
 
  if (RIndex != mhrNONE)
    {
    char tmpEMsg[MMAX_LINE];

    /* apply appropriate hold */

    switch (RIndex)
      {
      case mhrPreReq:

        /* create batch hold */

        MJobSetHold(J,mhBatch,0,mhrPreReq,EMsg);

        break;

      case mhrAMFailure:

        switch (MAM[0].StartFailureAction)
          {
          case mamjfaHold:
          default:

            /* set hold on all non-server AM failures or if policy set to hold */

            sprintf(tmpEMsg,"Unable to charge funds - AM failure");
 
            MJobSetHold(J,mhDefer,MSched.DeferTime,RIndex,tmpEMsg);

            MNLClear(&J->NodeList);

            J->TaskMap[0]    = -1;

            break;

          case mamjfaCancel:

            MJobCancel(
              J,
              "MOAB_INFO:  Unable to charge funds\n",
              FALSE,
              NULL,
              NULL);

            sprintf(tmpEMsg,"Unable to charge funds -- Job cancelled");

            MNLClear(&J->NodeList);

            J->TaskMap[0]    = -1;
  
            break;
          }  /* END switch (MAM[0].StartFailureAction) */

        break;

      case mhrNoFunds:

        switch (MAM[0].StartFailureNFAction)
          {
          case mamjfaHold:
          default:

            /* set hold on all non-server AM failures or if policy set to hold */

            sprintf(tmpEMsg,"Unable to charge funds -- Insufficient balance");

            MJobSetHold(J,mhDefer,MSched.DeferTime,RIndex,tmpEMsg);

            break;

          case mamjfaCancel:

            MJobCancel(
              J,
              "MOAB_INFO:  Unable to charge funds\n",
              FALSE,
              NULL,
              NULL);

            sprintf(tmpEMsg,"Unable to charge funds -- job cancelled");

            MNLClear(&J->NodeList);

            J->TaskMap[0]    = -1;

            break;

          case mamjfaRetry:
          
            /* job start failed, return failure even if action is NONE */

            MNLClear(&J->NodeList);

            J->TaskMap[0]    = -1;
 
            break;

          case mamjfaIgnore:

            /* ignore failure, continue and run job */

            /* NO-OP */

            break;
          }  /* END switch (MAM[0].StartFailureNFAction) */

        break;

      default:

        /* defer job */

        MJobSetHold(J,mhDefer,MSched.DeferTime,RIndex,EMsg);
 
        break;
      }
    }

  return(SUCCESS);
  }  /* END MJobProcessFailedStart() */


