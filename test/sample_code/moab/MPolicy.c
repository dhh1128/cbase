/* HEADER */

/**
 * @file MPolicy.c
 *
 * Moab Policies
 */


/* Contains:                                                  *
 *   int MQueueSelectJobs(MJL,SJ,PL,MNC,MPC,MWCL,OPI,FR,RB,ModifyJob) *
 *   int MQueueSelectAllJobs(Q,PLevel,P,EJList,EJListSize,DoPrioritize,UpdateStats,TrackIdle,RecordBlock,Msg) *
 *   int MJobCheckPolicies(J,PL,PT,P,RIndex,Buffer,StartTime) *
 *   int MPolicyGetEStartTime(O,OIndex,P,PLevel,Time,EMsg)    *
 *   int MPolicyAdjustUsage(ResUsage,J,R,LimitType,PU,OIndex,Count,ViolationDetected) *
 *                                                            */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  


static const enum MXMLOTypeEnum MAToO[] = {
    mxoNONE,
    mxoJob,
    mxoNode,
    mxoRsv,
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoPar,
    mxoSys,
    mxoSys,
    mxoClass,
    mxoQOS,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoRack,
    mxoQueue,
    mxoCluster,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoxVC,
    mxoNONE };



int __MPolicyAdjustUsage(mjob_t *,mpu_t *,mpc_t *,int,int,enum MXMLOTypeEnum,enum MXMLOTypeEnum,mbool_t *,char *); 
int __MPolicyAdjustSingleUsage(mpu_t *,int ,enum MActivePolicyTypeEnum,int,enum MXMLOTypeEnum,enum MXMLOTypeEnum,mbool_t *,char *);
int __MPolicyAdjustSingleGUsage(mgpu_t *,int ,int,enum MXMLOTypeEnum,enum MXMLOTypeEnum,mbool_t *,char *);



/* NOTE on limits:

  -1              no limit
   0              not specified (default to no limit)
   1 - INFINITY   limit value

*/




/**
 * Run a job eligibility report on blocked jobs.
 *
 */

void MQueueDiagnoseEligibility()

  {
  job_iter JTI;

  mjob_t *J;
  char Buffer[MMAX_LINE];
  mbitmap_t Flags;

  const char *FName = "MQueueDiagnoseEligibility";

  MDB(4,fSTRUCT) MLog("%s(FullQ)\n",
    FName);

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (J->EligibilityNotes != NULL)
      {
      MUFree(&J->EligibilityNotes);
      }

    if (MJOBISACTIVE(J) == TRUE)
      {
      continue;
      }

    Buffer[0] = '\0';

    bmset(&Flags,mcmVerbose);

    MJobDiagnoseEligibility(J,NULL,NULL,mptSoft,mfmHuman,&Flags,NULL,Buffer,sizeof(Buffer),FALSE);

    /* remove carriage returns between each note */

    MUStrReplaceChar(Buffer,'\n',' ',FALSE);

    MUStrDup(&J->EligibilityNotes,Buffer);
    }      /* END for () */

  return;
  }  /* END MQueueDiagnoseEligibility() */





/**
 * Records job block information (type and description) if recording is enabled. 
 *
 * Note: Also adds block message to a linked list of block messages if we are recording all block reasons.
 *
 * @param J (I/O) job pointer 
 * @param P (I) partition [optional] 
 * @param JobBlockReason (I) enum value for block job reason 
 * @param JobBlockReasonDescription (I) string to store in the job block message
 * @param RecordBlock (I) boolean that indicates wheter or not we are recording blocked messages
 * @param BlockAlreadyRecorded (I/O) pointer to boolean which tracks whether a block message has been recorded already for this job
 */

void MQueueRecordBlock(

  mjob_t            *J,
  mpar_t            *P,
  enum MJobNonEType  JobBlockReason,
  const char        *JobBlockReasonDescription,
  mbool_t            RecordBlock,
  mbool_t           *BlockAlreadyRecorded)
 
  {
  mjblockinfo_t *BlockInfoPtr;

  const char *FName = "MQueueRecordBlock";

  /* If block message recording is disabled there is no work to do here. */

  if (RecordBlock != TRUE)
    return;

  /* We only record the BlockReason and BlockMsg once for a job in the
     queue processing loop.  Once we have set these values, we set the
     boolean so we don't overwrite them later if we are doing extra checking
     in the queue processing loop and encounter additional block reasons. */

  if ((BlockAlreadyRecorded != NULL) && (*BlockAlreadyRecorded == TRUE))
    return;

  MDB(7,fSCHED) MLog("%s(%s,%s,%s,%d,BlockAlreadyRecorded)\n",
    FName,
    J->Name,
    MJobBlockReason[JobBlockReason],
    (JobBlockReasonDescription != NULL) ? JobBlockReasonDescription : "NULL",
    RecordBlock);

  /* if there is no block message for this partition
     or the new block message is different than the existing one then
     add the new block message and update the database */

  if ((MJobGetBlockInfo(J,P,&BlockInfoPtr) != SUCCESS) ||
      (MJobCmpBlockInfo(BlockInfoPtr,JobBlockReason,JobBlockReasonDescription) == FALSE))
    {
    /* alloc and add a block record to the job for this partition */
  
    BlockInfoPtr = (mjblockinfo_t *)MUCalloc(1,sizeof(mjblockinfo_t));
  
    BlockInfoPtr->PIndex = (P == NULL) ? 0 : P->Index;
    BlockInfoPtr->BlockReason = JobBlockReason;
    MUStrDup(&BlockInfoPtr->BlockMsg,JobBlockReasonDescription);
  
    if (MJobAddBlockInfo(J,P,BlockInfoPtr) != SUCCESS)
      MJobBlockInfoFree(&BlockInfoPtr);

    if (MSched.EnableHighThroughput == FALSE)
      MJobTransition(J,TRUE,FALSE);
    }

  if (BlockAlreadyRecorded != NULL)
    *BlockAlreadyRecorded = TRUE;

  return;
  } /* END MQueueRecordBlock() */





/**
 * Select jobs which satisfy limits and partition constraints.
 *
 * NOTE:  must support idle jobs and active, dynamic jobs.
 *
 * NOTE:  no side-affects on jobs in SrcQ except:
 *
 *  1 if 'ModifyJob!=TRUE' 
 *      if 'RecordBlock==TRUE' set BlockReason, BlockMsg 
 *      set SystemQueueTime, EffQueueDuration
 *      free job rsv 
 *      if OrigPIndex == -1, clear CPAL, otherwise set CPAL
 *  2 if 'ModifyJob==TRUE' 
 *      if 'RecordBlock==TRUE' set BlockReason, BlockMsg 
 *      set SystemQueueTime, EffQueueDuration
 *      free job rsv
 *      set Hold
 *      if OrigPIndex == -1, clear CPAL, otherwise set CPAL
 *
 * NYI:  must handle effqduration
 *
 * @see designdocs JobSchedulingDoc
 * @see MQueueSelectAllJobs() - peer
 * @see MSchedProcessJobs() - parent
 *
 * @param SrcQ (I)
 * @param DstQ (O)
 * @param PLevel (I)
 * @param MaxNC (I) [-1 = unlimited,!= MMAX_NODE = ignore ds jobs]
 * @param MaxPC (I) [-1 = unlimited,MAX_TASK + 1 = use bf checks]
 * @param MaxWCLimit (I)
 * @param OrigPIndex (I)    -1 will clear the job's CPAL even if the job meets all requirements. 
 *                       != -1 will set the job's CPAL if the job meets all requirements.
 * @param FRCount (O) [optional]
 * @param DstQIsMUIQ  (I)
 * @param UpdateStats (I)
 * @param RecordBlock (I)
 * @param ModifyJob (I) [take action on blocked jobs]
 * @param SingleParCheck (I)
 */

int MQueueSelectJobs(

  mjob_t       **SrcQ,          /* I */
  mjob_t       **DstQ,          /* O */
  enum MPolicyTypeEnum PLevel,  /* I */
  int            MaxNC,         /* I (-1 = unlimited,!= MMAX_NODE = ignore ds jobs) */
  int            MaxPC,         /* I (-1 = unlimited,MAX_TASK + 1 = use bf checks) */
  mulong         MaxWCLimit,    /* I */
  int            OrigPIndex,    /* I */
  int           *FRCount,       /* O rejection reasons (optional) */
  mbool_t        DstQIsMUIQ,    /* I */
  mbool_t        UpdateStats,   /* I */
  mbool_t        RecordBlock,   /* I */
  mbool_t        ModifyJob,     /* I (take action on blocked jobs) */
  mbool_t        SingleParCheck) /* Perform partition checks if J->PAL has one partition, even if OrigPIndex is -1 */     

  {
  int      index;
  int      jindex;
  int      sindex;

  mjob_t  *J;

  enum MDependEnum DType;     /* type of failed job dependency */

  mpar_t  *P;

  int      LRCount[marLAST];

  enum MPolicyRejectionEnum PReason;

  int     *RCount;

  int      PIndex;
  int      PReq;

  int      LockPartitionIndex;

  double   PE;

  char     tmpLine[MMAX_LINE];

  long     JWCLimit;

  const char *FName = "MQueueSelectJobs";

  mbitmap_t BM;

  MDB(3,fSCHED) MLog("%s(SrcQ,DstQ,%s,%d,%d,%ld,%s,FRCount,%s,%s,%s,%s)\n",
    FName,
    MPolicyMode[PLevel],
    MaxNC,
    MaxPC,
    MaxWCLimit,
    (OrigPIndex == -1) ? "EVERY" : MPar[MAX(0,OrigPIndex)].Name,
    MBool[DstQIsMUIQ],
    MBool[UpdateStats],
    MBool[RecordBlock],
    MBool[ModifyJob]);

  PIndex = MAX(0,OrigPIndex);

  /* system policies only checked in MQueueSelectAllJobs() */

  if (FRCount != NULL)
    RCount = FRCount;
  else
    RCount = &LRCount[0];

  memset(RCount,0,sizeof(LRCount));

  if ((SrcQ == NULL) || (DstQ == NULL))
    {
    MDB(1,fSCHED) MLog("ERROR:    invalid arguments passed to %s()\n",
      FName);
 
    return(FAILURE);
    }

  /* initialize output */

  DstQ[0] = NULL;

  if (SrcQ[0] == NULL)
    {
    MDB(4,fSCHED) MLog("INFO:     idle job queue is empty on iteration %d\n",
      MSched.Iteration);

    return(FAILURE);
    }

  P  = &MPar[PIndex];

  if (UpdateStats == TRUE)
    {
    MStat.IdleJobs = 0;
    }

  bmset(&BM,mlActive);

  sindex = 0;

  /* initialize local policies */

  MLocalCheckFairnessPolicy(NULL,MSched.Time,NULL);


  mstring_t DValue(MMAX_LINE);

  for (jindex = 0;SrcQ[jindex] != NULL;jindex++)
    {
    mbool_t BlockFound = FALSE; /* have we found at least one block reason */

    J = SrcQ[jindex];

    if (MJobPtrIsValid(J))
      {
      MDB(7,fSCHED) MLog("INFO:     checking job[%d] '%s'\n",
        jindex,
        J->Name);
      }
    else
      {
      MDB(7,fSCHED) MLog("INFO:     queue job[%d] has been removed\n",
        jindex);

      RCount[marCorruption]++;

      continue;
      }

    /* NOTE:  determine job dependency failures here - use dependency info 
              later */

    if (!bmisset(&J->IFlags,mjifReqTimeLock))
      {
      DValue.clear();  /* Reset string */

      MJobCheckDependency(J,&DType,FALSE,NULL,&DValue);
      }
    else
      DType = mdNONE;

    if ((DType != mdNONE) && 
        ((DType != mdJobSyncSlave) || (J->Depend->Type != mdJobSyncMaster)))
      {
      /* Save off the current job dependency status in case it is asked for 
         later in an "mdiag -j -v --xml" */

      J->DependBlockReason = DType;

      MUStrDup(&J->DependBlockMsg,DValue.c_str());
      }
    else if (J->DependBlockMsg != NULL)
      {
      /* There is no longer a job dependency which would block this job so 
         remove the message */

      J->DependBlockReason = mdNONE;

      MUFree(&J->DependBlockMsg);
      }
    
    if (bmisset(&J->IFlags,mjifVPCMap))
      {
      /* rsvmap and vpcmap jobs are not scheduled - they just follow
         their parent object */

      continue;
      }

    if (bmisset(&J->IFlags,mjifWasCanceled))
      {
      MDBO(7,J,fSCHED) MLog("INFO:     job '%s' not scheduled, was canceled\n",
        J->Name);

      continue;
      }

    if ((J->Credential.U != NULL) &&
        (J->Credential.U->OID == 0) && 
        !bmisset(&J->Flags,mjfSystemJob) && 
        (MSched.AllowRootJobs == FALSE))
      {
      RCount[marUser]++;

      continue;
      }

    if (UpdateStats == TRUE)
      {
      if (J->State == mjsIdle)
        MStat.IdleJobs++;
      }

    PReq = J->TotalProcCount;
    MJobGetPE(J,P,&PE);

    /* check partition */

    LockPartitionIndex = -1;

    if ((SingleParCheck == TRUE) && 
        (OrigPIndex == -1) && 
        (!bmissetall(&J->PAL,MMAX_PAR)) && 
        (!bmisclear(&J->PAL)))
      {
      int          pindex;
      mbitmap_t    PAL;

      bmcopy(&PAL,&J->PAL);

      /* See if this job is locked into a single partition */

      for (pindex = 0;pindex < MMAX_PAR;pindex++) 
        {
        if (bmisset(&PAL,pindex))
          {
          bmunset(&PAL,pindex);

          if (bmisclear(&PAL))
            {
            LockPartitionIndex = pindex; /* only 1 partition in the PAL */
            P  = &MPar[pindex];
            }

          break;
          } /* END  if (bmisset(&pindex,PAL)) */
        } /* END for() loop */

      bmclear(&PAL);
      } /* END if ((SingleParCheck == TRUE) && (OrigPIndex == -1)) */

    if ((OrigPIndex != -1) || 
        (LockPartitionIndex != -1))
      {
      if ((P->Index == 0) && !bmisset(&J->Flags,mjfCoAlloc))
        {
        /* why?  what does partition '0' mean in partition mode? */

        MDB(7,fSCHED) MLog("INFO:     job %s not considered for spanning\n",
          J->Name);

        RCount[marPartition]++;

        MQueueRecordBlock(J,&MPar[PIndex],mjnePartitionAccess,"cannot span partitions",RecordBlock,&BlockFound);

        continue;
        }
      else if ((P->Index != 0) && bmisset(&J->Flags,mjfCoAlloc))
        {
        MDB(3,fSCHED) MLog("INFO:     spanning job %s not considered for partition scheduling\n",
          J->Name);

        RCount[marPartition]++;

        sprintf(tmpLine,"spanning job cannot access partition %s",
          P->Name);

        MQueueRecordBlock(J,&MPar[PIndex],mjnePartitionAccess,tmpLine,RecordBlock,&BlockFound);

        continue;
        }

      if (((P->Index > 0) && (bmisset(&J->PAL,P->Index) == FAILURE)) ||
          ((MaxNC != MSched.M[mxoNode]) && (bmisset(&J->PAL,P->Index) == FAILURE)))
        {
        MDB(7,fSCHED) MLog("INFO:     job %s not considered for partition %s (allowed %s)\n",
          J->Name,
          P->Name,
          MPALToString(&J->PAL,NULL,tmpLine));

        RCount[marPartition]++;

        sprintf(tmpLine,"cannot access partition %s",
          P->Name);

        MQueueRecordBlock(J,&MPar[PIndex],mjnePartitionAccess,tmpLine,RecordBlock,&BlockFound);

        continue;
        }

      if ((P != MSched.GP) && (MJobCheckParAccess(J,P,FALSE,NULL,NULL) == FAILURE))
        {
        MDB(7,fSCHED) MLog("INFO:     job %s not considered for partition %s (allowed %s)\n",
          J->Name,
          P->Name,
          MPALToString(&J->PAL,NULL,tmpLine));

        RCount[marPartition]++;

        sprintf(tmpLine,"cannot access partition %s",
          P->Name);

        MQueueRecordBlock(J,&MPar[PIndex],mjnePartitionAccess,tmpLine,RecordBlock,&BlockFound);

        continue;
        }
      }   /* END if (OrigPIndex != -1) */

#ifdef MNOT
    /* not sure why we are checking for data before scheduling jobs - this may be bad code */

    if ((MaxNC != MSched.M[mxoNode]) && (MaxNC != -1))
      {
      /* analysis for backfill */

      /* reject jobs that have unfinished data staging requirements */

      if (!bmisset(&J->IFlags,mjifDataDependency))
        {
        MDB(7,fSCHED) MLog("INFO:     job '%s' not considered for prio scheduling (Data Staging)\n",
          J->Name);

        RCount[marData]++;

        MQueueRecordBlock(J,&MPar[PIndex],mjneNoData,"staging data not available",RecordBlock,&BlockFound);

        continue;
        }
      }    /* END if ((MaxNC != MSched.M[mxoNode]) && (MaxNC != -1)) */
#endif /* MNOT */

    /* check job state */

    if ((J->State != mjsIdle) && 
        (J->State != mjsSuspended))
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected (job in non-idle state '%s')\n",
        J->Name,
        MJobState[J->State]);

      RCount[marState]++;

      if ((MaxNC == MSched.M[mxoNode]) && 
          (MaxWCLimit == MMAX_TIME) && 
          (J->Rsv != NULL))
        {
        if ((J->State != mjsStarting) && (J->State != mjsRunning) && (ModifyJob == TRUE))
          MJobReleaseRsv(J,TRUE,TRUE);
        }

      sprintf(tmpLine,"non-idle state '%s'",
        MJobState[J->State]);

      MQueueRecordBlock(J,&MPar[PIndex],mjneState,tmpLine,RecordBlock,&BlockFound);

      continue;
      }
    else if ((J->EState != mjsIdle) && 
             (J->EState != mjsSuspended))
      {
      /* job has been previously scheduled or deferred */

      MDB(6,fSCHED) MLog("INFO:     job %s rejected (job in non-idle expected state: '%s')\n",
        J->Name,
        MJobState[J->EState]);

      RCount[marEState]++;

      if ((MaxNC == MSched.M[mxoNode]) && (MaxWCLimit == MMAX_TIME) && (J->Rsv != NULL))
        {
        if ((J->EState != mjsStarting) && (J->EState != mjsRunning) && (ModifyJob == TRUE))
          MJobReleaseRsv(J,TRUE,TRUE);
        }

      if (J->EState == mjsDeferred)
        {
        sprintf(tmpLine,"deferred for %ld seconds",
          MAX(1,J->HoldTime + MSched.DeferTime - MSched.Time));
        }
      else
        {
        sprintf(tmpLine,"non-idle expected state '%s'",
          MJobState[J->EState]);
        }

      MQueueRecordBlock(J,&MPar[PIndex],mjneEState,tmpLine,RecordBlock,&BlockFound);

      continue;
      }  /* END else if ((J->EState != mjsIdle) && ...) */

    /* Check for hold */

    if (!bmisclear(&J->Hold))
      {
      char holdLine[MMAX_LINE];
      /* job has a hold on it */

      sprintf(tmpLine,"job hold active - %s",
        bmtostring(&J->Hold,MHoldType,holdLine,",","N/A"));

      MDB(6,fSCHED) MLog("INFO:     job %s rejected (%s)\n",
        J->Name,
        tmpLine);

      RCount[marHold]++;

      if ((P->Index <= 0) && (J->Rsv != NULL) && (ModifyJob == TRUE))
        {
        MJobReleaseRsv(J,TRUE,TRUE);
        }

      MQueueRecordBlock(J,&MPar[PIndex],mjneHold,tmpLine,RecordBlock,&BlockFound);

      continue;
      }  /* END if (!bmisclear(&J->Hold)) */

    /* if partition is not dynamic, check available procs */

    /* NOTE:  job checked against MPar[0] in MQueueSelectAllJobs() */

    if (PReq > P->CRes.Procs)
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (exceeds configured procs: %d > %d)\n",
        J->Name,
        P->Name,
        PReq,
        P->CRes.Procs);

      RCount[marNodeCount]++;

      sprintf(tmpLine,"inadequate procs in partition: (R:%d, C:%d)",
        PReq,
        P->CRes.Procs);

      MQueueRecordBlock(J,&MPar[PIndex],mjneNoResource,tmpLine,RecordBlock,&BlockFound);

      if ((P->Index <= 0) && (ModifyJob == TRUE))
        {
        MJobReleaseRsv(J,TRUE,TRUE);

        if (bmisclear(&J->Hold))
          {
          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,"exceeds partition configured procs");
          }
        }

      continue;
      }  /* END if (PReq > P->CRes.Procs) */

#if 0
    /* doug believes the only limits this section checks are: msysaActivePS and msysaQueuePS */
    /* check partition-specific limits */

    if (MJobCheckLimits(
          J,
          PLevel,
          P,
          (1 << mlSystem),
          (J->Array != NULL) ? &J->Array->PLimit : NULL,
          tmpLine) == FAILURE)
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected, partition %s (%s)\n",
        J->Name,
        P->Name,
        tmpLine);

      RCount[marSystemLimits]++;

      MQueueRecordBlock(J,&MPar[PIndex],mjneSysLimits,tmpLine,RecordBlock,&BlockFound);

      if ((P->Index <= 0) && (ModifyJob == TRUE))
        {
        if (J->R != NULL)
          MJobReleaseRsv(J,TRUE,TRUE);

        if (J->Hold == 0)
          {
          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrSystemLimits,"exceeds system proc/job limit");
          }
        }

      continue;
      }  /* END if (MJobCheckLimits() == FAILURE) */
#endif

    /*
     * 1/8/10 - enabled this code to check all class 
     * limits for ticket rt6461 - DCH
     */

    if (J->Credential.C != NULL)
      {
      if (MJobCheckClassJLimits(
            J,
            J->Credential.C,
            FALSE,
            (MSched.PerPartitionScheduling == TRUE) ? PIndex : 0,
            tmpLine,  
            sizeof(tmpLine)) == FAILURE)
        {
        MDB(7,fSCHED) MLog("INFO:     job %s rejected in class %s (%s)\n",
          J->Name,
          J->Credential.C,
          tmpLine);

        MQueueRecordBlock(J,&MPar[PIndex],mjneNoClass,tmpLine,RecordBlock,&BlockFound);
        
        RCount[marClass]++;

        continue;
        }
      }

    if (!bmisclear(&J->ReqFeatureGResBM))
      {
      mbool_t tBad = FALSE;

      for (index = 1;index < MMAX_ATTR ;index++)
        {
        if (MAList[meNFeature][index][0] == '\0')
          break;

        if ((bmisset(&MSched.FeatureGRes,index) != bmisset(&J->ReqFeatureGResBM,index) &&
            bmisset(&J->ReqFeatureGResBM,index)))
          {
          tBad = TRUE;

          break;
          }
        }  /* END for (index) */

      if (tBad == TRUE)
        {
        MDB(7,fSCHED) MLog("INFO:     job %s not considered for scheduling (requested feature gres '%s' is off)\n",
          J->Name,
          MAList[meNFeature][index]);

        RCount[marGRes]++;

        snprintf(tmpLine,MMAX_LINE,"requested feature gres '%s' is off",MAList[meNFeature][index]);

        MQueueRecordBlock(J,&MPar[PIndex],mjnePartitionAccess,tmpLine,RecordBlock,&BlockFound);

        continue;
        }
      }

    /* check job size */

    if (PReq > MaxPC)
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (exceeds window size: %d > %d)\n",
        J->Name,
        P->Name,
        PReq,
        MaxPC);

      if (MaxPC == MSched.JobMaxTaskCount)
        {
        MDB(2,fSCHED) MLog("INFO:     job %s rejected in partition %s (exceeds maximum task size: %d > %d) - adjust JOBMAXTASKCOUNT\n",
          J->Name,
          P->Name,
          PReq,
          MaxPC);

        if (MSched.OFOID[mxoxTasksPerJob] == NULL)
          MUStrDup(&MSched.OFOID[mxoxTasksPerJob],J->Name);

        if (ModifyJob == TRUE)
          {
          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrSystemLimits,"exceeds scheduler JOBMAXTASKCOUNT");
          }
        }

      RCount[marNodeCount]++;

      sprintf(tmpLine,"inadequate procs in system: (R:%d, C:%d)",
        PReq,
        P->CRes.Procs);

      MQueueRecordBlock(J,&MPar[PIndex],mjneNoResource,tmpLine,RecordBlock,&BlockFound);

      continue;
      }  /* END if (PReq > MaxPC) */

    /* check job duration */

    JWCLimit = J->SpecWCLimit[0];

    if (JWCLimit > (long)MaxWCLimit)
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected in partition %s (exceeds window time: %ld > %ld)\n",
        J->Name,
        P->Name,
        JWCLimit,
        MaxWCLimit);

      RCount[marTime]++;

      sprintf(tmpLine,"inadequate time in system: (R:%ld, C:%ld)",
        MaxWCLimit,
        JWCLimit);

      MQueueRecordBlock(J,&MPar[PIndex],mjneNoTime,tmpLine,RecordBlock,&BlockFound);

      continue;
      }

    if (!bmisset(&J->Flags,mjfNoResources))
      {
      /* partition is static */

      /* check partition class support */

      if ((J->Credential.C != NULL) && !bmisset(&P->Classes,J->Credential.C->Index))
        {
        if (bmisset(&J->Credential.C->Flags,mcfRemote))
          {
          continue;
          }

        MDB(6,fSCHED) MLog("INFO:     job %s rejected, partition %s (class not supported '%s')\n",
          J->Name,
          P->Name,
          J->Credential.C->Name);

        RCount[marClass]++;

        sprintf(tmpLine,"required class not available - %s",J->Credential.C->Name);

        MQueueRecordBlock(J,&MPar[PIndex],mjneNoClass,tmpLine,RecordBlock,&BlockFound);

        if ((P->Index <= 0) && (ModifyJob == TRUE))
          {
          MJobReleaseRsv(J,TRUE,TRUE);

          if ((bmisclear(&J->Hold)) && !MSched.NoJobHoldNoResources)
            {
            MJobSetHold(J,mhDefer,MSched.DeferTime,mhrSystemLimits,"required class not found");
            }
          }

        continue;
        }      /* END if (MUNumListGetCount() == FAILURE) */

      /* check partition OS support */

      if ((P->ReqOS != 0) && (J->Req[0]->Opsys != 0) && (P->ReqOS != J->Req[0]->Opsys))
        {
        MDB(6,fSCHED) MLog("INFO:     job %s rejected, partition %s (OS not supported %s)\n",
          J->Name,
          P->Name,
          MAList[meOpsys][J->Req[0]->Opsys]);

        RCount[marOpsys]++;

        sprintf(tmpLine,"required OS not available - %s",
          MAList[meOpsys][J->Req[0]->Opsys]);

        MQueueRecordBlock(J,&MPar[PIndex],mjneNoResource,tmpLine,RecordBlock,&BlockFound);

        if ((P->Index <= 0) && (ModifyJob == TRUE))
          {
          MJobReleaseRsv(J,TRUE,TRUE);

          if (bmisclear(&J->Hold))
            {
            MJobSetHold(J,mhDefer,MSched.DeferTime,mhrSystemLimits,"required OS not found");
            }
          }

        continue;
        }      /* END if ((P->OSList != 0) && ...) */
      }        /* END if (!bmisset(&J->Flags,mjfNoResources) */

    if ((DType != mdNONE) &&
        ((DType != mdJobSyncSlave) || (J->Depend->Type != mdJobSyncMaster)))
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected (dependent on job '%s' %s)\n",
        J->Name,
        DValue.c_str(),
        MDependType[DType]);

      RCount[marDepend]++;

      if ((MaxNC == MSched.M[mxoNode]) &&
          (MaxWCLimit == MMAX_TIME) &&
          (J->Rsv != NULL) && 
          (ModifyJob == TRUE))
        {
        MJobReleaseRsv(J,TRUE,TRUE);
        }

      sprintf(tmpLine,"dependent on job '%s' %s",
        DValue.c_str(),
        MDependType[DType]);

      MQueueRecordBlock(J,&MPar[PIndex],mjneDependency,tmpLine,RecordBlock,&BlockFound);

      continue;
      }  /* END if ((DType != mdNONE) && ...) */

    /* check partition active job policies */

    /* NOTE:  dynamic jobs should be considered even if policy limits are
              reached to allow potential job reduction.
    */

    if ((MJOBISACTIVE(J) == FALSE) && 
        (MJobCheckPolicies(
          J,
          PLevel,
          &BM,
          P,   /* NOTE:  may set to &MPar[0] */
          &PReason,
          tmpLine,
          MMAX_TIME) == FAILURE))
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected, partition %s (policy failure: '%s')\n",
        J->Name,
        P->Name,
        MPolicyRejection[PReason]);

      RCount[marPolicy]++;

      MQueueRecordBlock(J,&MPar[PIndex],mjneActivePolicy,tmpLine,RecordBlock,&BlockFound);

      if ((MaxNC == MSched.M[mxoNode]) && 
          (MaxWCLimit == MMAX_TIME) && 
          (J->Rsv != NULL) &&
          (ModifyJob == TRUE))
        {
        MJobReleaseRsv(J,TRUE,TRUE);
        }

      continue;
      }

    if (J->Credential.U != NULL)
      J->Credential.U->MTime = MSched.Time;

    if (J->Credential.G != NULL)
      J->Credential.G->MTime = MSched.Time;

    if (J->Credential.A != NULL)
      J->Credential.A->MTime = MSched.Time;

    if (MPar[0].FSC.FSPolicy != mfspNONE)
      {
      int OIndex;
      int rc = SUCCESS;
      
      mstring_t tmpString(MMAX_LINE);

      if (P->FSC.ShareTree != NULL)
        {
        mstring_t tmpMsg(MMAX_LINE);

        if ((rc = MFSTreeCheckCap(P->FSC.ShareTree,J,P,&tmpMsg)) == FAILURE)
          {
          MDB(5,fSCHED) MLog("INFO:     %s\n",tmpMsg.c_str());
        
          MMBAdd(&J->MessageBuffer,tmpMsg.c_str(),NULL,mmbtNONE,0,0,NULL);
        
          MStringSetF(&tmpString,"blocked by fairshare cap (%s)",tmpMsg.c_str());
          }
        }
      else if ((rc = MFSCheckCap(NULL,J,P,&OIndex)) == FAILURE)
        {
        MDB(5,fSCHED) MLog("INFO:     job '%s' exceeds %s FS cap\n",
          J->Name,
          (OIndex > 0) ? MXO[OIndex] : "NONE");
        
        MStringSetF(&tmpString,"blocked by fairshare cap");
        }
      
      if (rc == FAILURE)
        {
        RCount[marFairShare]++;

        MQueueRecordBlock(J,&MPar[PIndex],mjneFairShare,tmpString.c_str(),RecordBlock,&BlockFound);

        /* if we're "blocking" the job because of a fairshare cap violation
         * and it has a live reservation, release the reservation */

        if (MJobGetBlockReason(J,&MPar[PIndex]) == mjneFairShare)
          {
          MJobReleaseRsv(J,TRUE,TRUE);
          }

        continue;
        }
      }    /* END if (FS[0].FSPolicy != mfspNONE) */

    /* NOTE:  idle queue policies handled in MQueueSelectAllJobs() */

    if (MLocalCheckFairnessPolicy(J,MSched.Time,NULL) == FAILURE)
      {
      MDB(6,fSCHED) MLog("INFO:     job %s rejected, partition %s (violates local fairness policy)\n",
        J->Name,
        P->Name);

      RCount[marPolicy]++;

      MQueueRecordBlock(J,&MPar[PIndex],mjneLocalPolicy,"blocked by local policy",RecordBlock,&BlockFound);

      continue;
      }

    /* NOTE:  effective queue duration not yet properly supported */

    /* add job to destination queue */

    MDB(5,fSCHED) MLog("INFO:     job %s added to queue at slot %d\n",
      J->Name,
      sindex);

    if (OrigPIndex == -1)
      {
      bmclear(&J->CurrentPAL);
      }
    else
      {
      bmset(&J->CurrentPAL,P->Index);

      if (MSched.SharedPtIndex > 0)
        bmset(&J->CurrentPAL,MSched.SharedPtIndex);
      }

    DstQ[sindex++] = SrcQ[jindex];

    if (DstQIsMUIQ == TRUE)
      bmset(&J->IFlags,mjifUIQ);

    if (sindex >= MSched.M[mxoJob])
      break;
    }  /* END for (jindex) */

  /* terminate list */

  DstQ[sindex] = NULL;

  MDB(1,fSCHED)
    {
    MDB(1,fSCHED) MLog("INFO:     total jobs selected in partition %s: %d/%-d ",
      MPar[PIndex].Name,
      sindex,
      jindex);

    for (index = 0;index < marLAST;index++)
      {
      if (RCount[index] != 0)
        {
        MLogNH("[%s: %d]",
          MAllocRejType[index],
          RCount[index]);
        }
      }    /* END for (index) */

    MLogNH("\n");
    }

  if (sindex == 0)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MQueueSelectJobs() */


/**
 * Helper function for MQueueSelectAllJobs, determines which jobs are eligible and which aren't.
 *
 * @param J (I)
 * @param PLevel (I)
 * @param P (I)
 * @param UpdateStats (I)
 * @param TrackIdle (I)
 * @param Diagnostic (I)
 * @param RecordBlock (I)
 * @param Msg
 */

int MQueueSelectJob(

  mjob_t     *J,
  enum MPolicyTypeEnum PLevel,
  mpar_t     *P,
  mbool_t     UpdateStats,
  mbool_t     TrackIdle,
  mbool_t     Diagnostic,
  mbool_t     RecordBlock,
  char       *Msg)

  {
  mbool_t  IsIdle = FALSE;

  enum MDependEnum DType;  /* type of job dependency which cannot be satisfied */
  mbool_t NeedTransition = FALSE;  /* has the job changed? */

  char     Message[MMAX_LINE];

  mbitmap_t BM;

  MJobGetFSTree(J,P,FALSE);

  if (UpdateStats == TRUE)
    {
    bmunset(&J->SysFlags,mjfSPViolation);

    /* Update the cred total usage for creds used by this job */

    MStatIncrementCredUsage(J);
    }

  /* initialize block information */

  MQueueRecordBlock(J,P,mjneNONE,NULL,RecordBlock,NULL);

  /* Set the Depend Block Msg if any dependencies or clear it if there are no dependencies */

  mstring_t DValue(MMAX_LINE);
 
  if (!bmisset(&J->IFlags,mjifReqTimeLock))
    {
    MJobCheckDependency(J,&DType,FALSE,NULL,&DValue);

    if (J->DependBlockReason != DType)
      {
      NeedTransition = TRUE;
      }
    }
  else
    {
    DType = mdNONE;
    }

  if ((DType != mdNONE) &&
      ((DType != mdJobSyncSlave) || (J->Depend->Type != mdJobSyncMaster)))
    {
    /* Save off the current job dependency status in case it is asked for 
       later in an "mdiag -j -v --xml" */

    J->DependBlockReason = DType;

    MUStrDup(&J->DependBlockMsg,DValue.c_str());

    if (TRUE == NeedTransition)
      {
      MJobTransition(J,FALSE,FALSE);
      }

    return(FAILURE);
    }
  else if (J->DependBlockMsg != NULL)
    {
    /* There is no longer a job dependency which would block this job so remove the message */

    J->DependBlockReason = mdNONE;

    MUFree(&J->DependBlockMsg);

    if (TRUE == NeedTransition)
      {
      MJobTransition(J,FALSE,FALSE);
      }
    }

  /* check job state */

  if (((J->State != mjsIdle) && 
       (J->State != mjsSuspended)) &&
      ((J->State != mjsStaged) && 
       ((J->DestinationRM == NULL) || 
        (!bmisset(&J->DestinationRM->Flags,mrmfAutoStart)))))
    {
    /* NOTE:  do not count jobs towards local policies if it is destined for a peer with autostart (WHY?) */

    sprintf(Message,"non-idle state '%s'",
      MJobState[J->State]);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjneState,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (job in non-idle state '%s')\n",
      J->Name,
      MJobState[J->State]);

    if (MJOBISACTIVE(J) == FALSE)
      {
      mjob_t *tJ;

      /* must determine if this job has a syncwith reservation */

      if ((J->State == mjsHold) && 
          (J->Depend != NULL) && 
          (J->Depend->Value != NULL) &&
          (MJobFind(J->Depend->Value,&tJ,mjsmBasic) == SUCCESS) &&
          (tJ != NULL) &&
          (tJ->Depend != NULL) &&
          (tJ->Depend->Type == mdJobSyncMaster) &&
          (tJ->Rsv != NULL))
        {
        /* NO-OP */
        }
      else
        {
        if (Diagnostic == FALSE)
          {
          MJobReleaseRsv(J,TRUE,TRUE);
          }
        }
      }
    else if (J->EState != mjsDeferred)
      {
      /* adjust partition and credential based active use statistics */

      /* determine if job violates 'soft' policies' */
       
      if (UpdateStats == TRUE)
        {
        mbitmap_t BM;

        bmset(&BM,mlActive);

        if (MJobCheckLimits(
              J,
              mptSoft,
              P,
              &BM,
              (J->Array != NULL) ? &J->Array->PLimit : NULL,
              Message) == FAILURE)
          {
          /* job violates 'soft' active policy */

          bmset(&J->SysFlags,mjfSPViolation);
          bmset(&J->Flags,mjfSPViolation);

          MJobUpdateFlags(J);
          }

        MPolicyAdjustUsage(J,NULL,NULL,mlActive,&MPar[0].L.ActivePolicy,mxoALL,1,NULL,NULL);
        MPolicyAdjustUsage(J,NULL,NULL,mlActive,NULL,mxoALL,1,NULL,NULL);

        if ((J->Credential.U != NULL) &&
            (J->Credential.U->L.ActivePolicy.HLimit[mptMaxJob][0] > 0))
          {
          /* TBD remove this log - put in for testing at Ford */
      
          MDB(7,fSTAT) MLog("INFO:     setting user %s max job usage %ld hlimit %ld in MQueueSelectAllJobs\n",
            J->Credential.U->Name,
            J->Credential.U->L.ActivePolicy.Usage[mptMaxJob][0],
            J->Credential.U->L.ActivePolicy.HLimit[mptMaxJob][0]);        
          }
        }
      }    /* END else if (J->EState != mjsDeferred) */

    return(FAILURE);
    }      /* END if ((J->State != mjsIdle) && ...) */
  else if ((J->EState != mjsIdle) && (J->EState != mjsSuspended))
    {
    /* job has been previously scheduled or deferred */

    if (J->EState == mjsDeferred)
      {
      sprintf(Message,"deferred for %ld seconds",
        MAX(1,J->HoldTime + MSched.DeferTime - MSched.Time));
      }
    else
      {
      sprintf(Message,"non-idle expected state '%s'",
        MJobState[J->EState]);
      }

    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjneEState,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (job in non-idle expected state: '%s')\n",
      J->Name,
      MJobState[J->EState]);

    if ((J->EState != mjsStarting) && (J->EState != mjsRunning) && (Diagnostic == FALSE))
      MJobReleaseRsv(J,TRUE,TRUE);

    return(FAILURE);
    }
  else if ((P != MSched.GP) && (MJobCheckParAccess(J,P,FALSE,NULL,Message) == FAILURE))
    {
    /* partition access denied */

    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjnePartitionAccess,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (job cannot access partition '%s')\n",
      J->Name,
      P->Name);

    if ((J->EState != mjsStarting) && (J->EState != mjsRunning) && (Diagnostic == FALSE))
      MJobReleaseRsv(J,TRUE,TRUE);

    return(FAILURE);
    }  /* END else if ((P != MSched.GP) && ...) */
  else if (J->State != mjsSuspended)
    {
    IsIdle = TRUE;
    }

  if ((MSched.OptimizedJobArrays == TRUE) && (bmisset(&J->Flags,mjfArrayJob)))
    {
    sprintf(Message,"array job %s not considered for normal scheduling\n",
      J->Name);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MDB(3,fSCHED) MLog("INFO:     %s",Message);

    return(FAILURE);
    }

  if (J->Array != NULL)
    {
    sprintf(Message,"INFO:     array job %s not considered for normal scheduling\n",
      J->Name);

    MUStrCpy(Msg,Message,MMAX_LINE);

    /* master of job array not considered for scheduling */
    MDB(3,fSCHED) MLog("INFO:     %s",Message);

    return(FAILURE);
    }

  /* check to see if the job was migrated away from this peer */

  if ((MJOBWASMIGRATEDTOREMOTEMOAB(J)) &&
      (MSched.ShowMigratedJobsAsIdle == FALSE))
    {
    sprintf(Message,"INFO:     job %s was migrated to a remote peer\n",
        J->Name);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MDB(3,fSCHED) MLog("INFO:     %s",Message);

    return(FAILURE);
    }

  /* NOTE:  This might be unnecessary. It certainly doesn't work for HLRN. */
  /* check if this peer can even run this job */

  if (bmisset(&J->Flags,mjfClusterLock) &&
      bmisset(&J->Flags,mjfNoRMStart) &&
      (MSched.ShowMigratedJobsAsIdle == FALSE))
    {
    sprintf(Message,"INFO:     job %s is being scheduled by another system\n",
      J->Name);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MDB(3,fSCHED) MLog("INFO:     %s",Message);

    return(FAILURE);
    }

  /* check user/system/batch hold */

  if (!bmisclear(&J->Hold))
    {
    char Line[MMAX_LINE];

    sprintf(Message,"job hold active - %s",
      bmtostring(&J->Hold,MHoldType,Line,",","N/A"));

    MUStrCpy(Msg,Message,MMAX_LINE);

    /* only set BlockMsg if it is empty--we don't want this generic msg
     * overwriting a more specific (and helpful) one */

    MQueueRecordBlock(
      J,
      P,
      mjneHold,
      Message,
      (RecordBlock && (MJobGetBlockMsg(J,P) != NULL)),
      NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (hold on job)\n",
      J->Name);

    /*  NOTE: deprecated (use MJobAddEffQueueDuration) 
    if ((MPar[0].JobPrioAccrualPolicy == mjpapFullPolicy) ||
        (MPar[0].JobPrioAccrualPolicy == mjpapFullPolicyReset))
      {
      J->EffQueueDuration = 0;
      J->SystemQueueTime = MSched.Time;
      }
    */

    if (Diagnostic == FALSE)
      MJobReleaseRsv(J,TRUE,TRUE);

    return(FAILURE);
    }  /* END if (J->Hold != 0) */

  /* check class availability */

  if ((J->Credential.C != NULL) && 
     ((J->Credential.C->IsDisabled == TRUE) || (J->Credential.C->FType == mqftRouting)))
    {
    sprintf(Message,"class not enabled - %s",
      J->Credential.C->Name);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjneNoClass,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (class %s not enabled)\n",
      J->Name,
      J->Credential.C->Name);

    /*  NOTE: deprecated (use MJobAddEffQueueDuration) 
    if ((MPar[0].JobPrioAccrualPolicy == mjpapFullPolicy) ||
        (MPar[0].JobPrioAccrualPolicy == mjpapFullPolicyReset))
      {
      J->SystemQueueTime = MSched.Time;
      }
    */

    if (Diagnostic == FALSE)
      MJobReleaseRsv(J,TRUE,TRUE);

    return(FAILURE);
    }  /* END if ((J->Cred.C != NULL) && (J->Cred.C->IsDisabled == TRUE)) */

  if (J->Credential.C != NULL)
    {
    if (MJobCheckClassJLimits(
          J,
          J->Credential.C,
          FALSE,
          P->Index,
          Message,  
          sizeof(Message)) == FAILURE)
      {
      MDB(7,fSCHED) MLog("INFO:     job %s rejected in class %s (%s)\n",
        J->Name,
        J->Credential.C,
        Message);
    
      MUStrCpy(Msg,Message,MMAX_LINE);

      MQueueRecordBlock(J,P,mjneNoClass,Message,RecordBlock,NULL);
    
      if (Diagnostic == FALSE)
        MJobReleaseRsv(J,TRUE,TRUE);

      return(FAILURE);
      }
    }

  /* check priority */

  if ((bmisset(&MPar[0].Flags,mpfRejectNegPrioJobs)) && (J->PStartPriority[P->Index] < 0))
    {
    sprintf(Message,"invalid priority - %ld on partition %s",
      J->PStartPriority[P->Index],
      P->Name);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjnePriority,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (negative priority)\n",
      J->Name);
    
    /*  NOTE: deprecated (use MJobAddEffQueueDuration) 
    if ((MPar[0].JobPrioAccrualPolicy == mjpapFullPolicy) ||
        (MPar[0].JobPrioAccrualPolicy == mjpapFullPolicyReset))
      {
      J->SystemQueueTime = MSched.Time;
      }
    */
    
    if (Diagnostic == FALSE)
      MJobReleaseRsv(J,TRUE,TRUE);

    return(FAILURE);
    }

  /* check startdate */

  /* FIXME  jobs with time constraints should not be blocked. when blocked,
            they cannot accrue service priority or reserve resources */

  if ((J->SMinTime > MSched.Time) && (J->InRetry == FALSE))
    {
    char TString[MMAX_LINE];

    MULToTString(J->SMinTime - MSched.Time,TString);

    sprintf(Message,"startdate in '%s'",
      TString);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjneStartDate,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (release time violation: %ld > %ld)\n",
      J->Name,
      J->SMinTime,
      MSched.Time);

    if (Diagnostic == FALSE)
      MJobReleaseRsv(J,TRUE,TRUE);

    return(FAILURE);
    }  /* END if (J->SMinTime > MSched.Time) */

  if (UpdateStats == TRUE)
    {
    if (IsIdle == TRUE)
      MStat.IdleJobs++;
    }

  /* check configured procs - 

     NOTE:  job must be selected to enable reservation so cannot filter
            on available procs at this point */

  if (J->TotalProcCount > MPar[0].CRes.Procs)
    {
    sprintf(Message,"inadequate procs in system: (R:%d, C:%d)",
      J->TotalProcCount,
      MPar[0].UpRes.Procs);

    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjneNoResource,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected (exceeds configured procs in system: %d > %d)\n",
      J->Name,
      J->TotalProcCount,
      MPar[0].CRes.Procs);

    if ((MPar[0].UpRes.Procs > 0) &&
        (Diagnostic == FALSE))
      {
      /* only set hold if RM failure has not occured */

      MJobReleaseRsv(J,TRUE,TRUE);

      if (!MSched.NoJobHoldNoResources)
        {
        MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,"exceeds configured partition procs");

        MJobTransition(J,TRUE,FALSE);
        }
      }

    return(FAILURE);
    }  /* END if (J->TotalProcCount > MPar[0].CRes.Procs) */

  if (J->Request.NC != 0)
    {
    int ConfigNodes;
    /* If there's a Class, use that else use Partition */
    if (J->Credential.C != NULL)
      {
      ConfigNodes = J->Credential.C->ConfiguredNodes;
      }
    else
      {
      ConfigNodes = P->ConfigNodes;
      }

    if (J->Request.NC > ConfigNodes)
      {
      sprintf(Message,"inadequate nodes in system: (R:%d, C:%d)",
        J->Request.NC,
        ConfigNodes);

      MUStrCpy(Msg,Message,MMAX_LINE);

      MQueueRecordBlock(J,P,mjneNoResource,Message,RecordBlock,NULL);

      MDB(6,fSCHED) MLog("INFO:     job %s rejected (exceeds configured nodes in system: %d > %d)\n",
        J->Name,
        J->Request.NC,
        ConfigNodes);

      if ((MPar[0].UpRes.Procs > 0) &&
          (Diagnostic == FALSE))
        {
        /* only set hold if RM failure has not occured */

        if (J->Rsv != NULL)
          MJobReleaseRsv(J,TRUE,TRUE);

        if (!MSched.NoJobHoldNoResources)
          {
          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,"exceeds configured partition nodes");

          MJobTransition(J,TRUE,FALSE);
          }
        }

      return(FAILURE);
      }  /* END if (J->Request.NC > ConfigNodes) */
    } /* END if (J->Request.NC != 0) */

  if ((P->Index == 0) &&
     ((J->Credential.Q == NULL) || 
      !bmisset(&J->Credential.Q->Flags,mqfCoAlloc)))
    {
    int rqindex;
    int pindex = 0;

    int RQProcs;

    /* all reqs must have resources available within a single partition */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      if ((J->Req[rqindex]->DRes.Procs == 0) || (J->Req[rqindex]->TaskCount == 0))
        continue;

      RQProcs = J->Req[rqindex]->TaskCount * 
        ((J->Req[rqindex]->DRes.Procs != -1) ?  J->Req[rqindex]->DRes.Procs : 1);
        
      for (pindex = 1;pindex < MSched.M[mxoPar];pindex++)
        {
        if (MPar[pindex].Name[0] == '\1')
          continue;

        if (MPar[pindex].Name[0] == '\0')
          break;

        if (MPar[pindex].CRes.Procs >= RQProcs)
          {
          /* partition is adequately large or is dynamic */

          break;
          }
        }  /* END for (pindex) */

      if ((pindex >= MSched.M[mxoPar]) || (MPar[pindex].Name[0] == '\0'))
        {
        /* no satisfactory partition located */

        sprintf(Message,"inadequate procs in any partition for req %d: (R:%d, C:%d)",
          rqindex,
          J->TotalProcCount,
          MPar[0].UpRes.Procs);

        MUStrCpy(Msg,Message,MMAX_LINE);

        MQueueRecordBlock(J,P,mjneNoResource,Message,RecordBlock,NULL);

        MDB(6,fSCHED) MLog("INFO:     job %s rejected (cannot locate %d procs for req %d in any partition)\n",
          J->Name,
          RQProcs,
          rqindex);

        if ((MPar[0].UpRes.Procs > 0) && (Diagnostic == FALSE))
          {
          /* only set hold if RM failure has not occured */

          MJobReleaseRsv(J,TRUE,TRUE);

          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,"req exceeds configured partition procs");
          }

        break;
        }  /* END if ((pindex >= MSched.M[mxoPar]) || ...) */
      }    /* END for (rqindex) */

    if ((pindex >= MSched.M[mxoPar]) || (MPar[pindex].Name[0] == '\0'))
      {
      return(FAILURE);
      }
    }  /* END if ((P->Index == 0) && ... */

  /* check queue policies */

  /* only non-suspended/non-reserved jobs affected by
   * active/idle/system policies */

  bmset(&BM,mlSystem);
  bmset(&BM,mlActive);

  if ((TrackIdle == TRUE) && (J->State != mjsSuspended))
    {
    bmset(&BM,mlIdle);
    }

  if ((J->Rsv == NULL) && (MJobCheckLimits(
        J,
        PLevel,
        P,
        &BM,
        (J->Array != NULL) ? &J->Array->PLimit : NULL,
        Message) == FAILURE))
    {
    MUStrCpy(Msg,Message,MMAX_LINE);

    MQueueRecordBlock(J,P,mjneIdlePolicy,Message,RecordBlock,NULL);

    MDB(6,fSCHED) MLog("INFO:     job %s rejected, partition %s (%s)\n",
      J->Name,
      P->Name,
      Message);
    
    /*  NOTE: deprecated (use MJobAddEffQueueDuration) 
    if (PLevel == mptHard)
      {
      if ((MPar[0].JobPrioAccrualPolicy == mjpapFullPolicy) ||
          (MPar[0].JobPrioAccrualPolicy == mjpapFullPolicyReset) ||
          (MPar[0].JobPrioAccrualPolicy == mjpapQueuePolicy) ||
          (MPar[0].JobPrioAccrualPolicy == mjpapQueuePolicyReset))
        {
        J->SystemQueueTime = MSched.Time;
        }
      }
    */

    return(FAILURE);
    }  /* END if ((J->R == NULL) && (MJobCheckLimits() == FAILURE)) */

  /* adjust partition and credential eligible job statistics */

  if ((UpdateStats == TRUE) || (TrackIdle == TRUE))
    {
    MStatAdjustEJob(J,P,1);

    if (UpdateStats != TRUE)
      {
      /* NOTE:  MStatAdjustEJobs() updates both usage and counter, only
                usage should be updated */

      MStat.EligibleJobs--;
      }
    }

  if ((J->Credential.Q != NULL)     && 
      (MSched.NTRJob == NULL) &&
      (bmisset(&J->Credential.Q->Flags,mqfNTR)))
    {
    /* Set the highest priority eligible NTR Job */

    MSched.NTRJob = J;
    }

  return(SUCCESS);
  }  /* END MQueueSelectJob() */






/**
 * Select jobs which can potentially run within specified partition with no 
 * limits applied.
 *
 * @see MSchedProcessJobs() - parent
 * @see MQueueSelectJobs() - peer - performs hard/soft and limit-enforced job 
 *                           selection
 * @see MJobCheckLimits() - child
 * @see MStatAdjustEJob() - child
 * @see designdocs JobSchedulingDoc
 *
 * NOTE:  Unsatisfied job dependencies do not make jobs ineligible allowing 
 *        full workflows to be scheduled and reservations made.  However, 
 *        situations in which idle job usage limits are set and the dependee 
 *        is blocked out can arise. (see parameter 'BLOCKLIST DEPEND')
 * NOTE: Jobs in Q get prioritized if DoPrioritize is set
 * NOTE: This routine calculates and enforces idle job limits
 * NOTE: Updates idle job usage for all jobs that satisfy constraints
 *
 * @param PLevel (I) policy level
 * @param P
 * @param EJList (I/O) [terminated w/-1]
 * @param EJListSize (I)
 * @param ECount (O) [pointer to number of entries added to eligible job list] 
 * @param DoPrioritize (I)
 * @param UpdateStats (I) [update global eligible/idle job stats]
 * @param TrackIdle (I) [bound resulting queue by idle job limits]
 * @param RecordBlock (I) [record reason job is blocked]
 * @param Diagnostic (I) Do only diagnostics calls
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MQueueSelectAllJobs(

  enum MPolicyTypeEnum PLevel,
  mpar_t  *P,
  mjob_t **EJList,
  int      EJListSize,
  int     *ECount,
  mbool_t  DoPrioritize,
  mbool_t  UpdateStats,
  mbool_t  TrackIdle,
  mbool_t  RecordBlock,
  mbool_t  Diagnostic,
  char    *Msg)

  {
  int      JIndex;
  int      jindex;

  int      JobCount;

  mjob_t  *J;

  static mjob_t **tmpEJList = NULL;

  const char *FName = "MQueueSelectAllJobs";

  MDB(3,fSCHED) MLog("%s(%s,%s,%s,%d,%s,%s,%s,%s,Msg)\n",
    FName,
    MPolicyMode[PLevel],
    (P != NULL) ? P->Name : "NULL",
    (EJList != NULL) ? "EJList" : "NULL",
    EJListSize,
    MBool[DoPrioritize],
    MBool[UpdateStats],
    MBool[TrackIdle],
    MBool[RecordBlock]);

  if (Msg != NULL)
    Msg[0] = '\0';

  if (EJList == NULL)
    {
    return(FAILURE);
    }

  if (P == NULL)
    {
    return(FAILURE);
    }

  if ((tmpEJList == NULL) &&
      (MJobListAlloc(&tmpEJList) == FAILURE))
    {
    return(FAILURE);
    }

  if (ECount != NULL)
    {
    *ECount = 0;
    }

  /* NOTE:  prioritize all feasible idle/active jobs */
  /*        sort jobs by priority                    */
  /*        apply queue policies                     */
  /*        record eligible jobs in JobList          */
  /*        adjust system queue time                 */

  if (DoPrioritize == TRUE)
    {
    /* prioritize/sort queue */

    EJList[0] = NULL;

    MQueuePrioritizeJobs(tmpEJList,FALSE,((P->Index == 0) || !strcmp(P->Name,"DEFAULT")) ? NULL:P);

#if 0 /* this doesn't really work right now */
    if (MSched.OptimizedJobArrays == TRUE)
      MQueueCollapse(tmpEJList);
#endif
    }
  else
    {
    memcpy(tmpEJList,EJList,sizeof(mjob_t *) * MIN(MSched.M[mxoJob] + 1,EJListSize));
    }

  /* initialize throttling policy usage */

  if (UpdateStats == TRUE)
    {
    /* when not commented out, double counting occurs (ie. MAXJOB=10 will only allow 5 jobs) */
    /* when     commented out, first SPViolation not detected (ie. MAXJOB=10 will allow 11 jobs) */

    /* ***** */
    MStatClearUsage(mxoNONE,TRUE,TRUE,FALSE,FALSE,FALSE); 
    /* ***** */

    MStatClearUsage(mxoNONE,TRUE,FALSE,FALSE,FALSE,FALSE);
    }
  else if (TrackIdle == TRUE)
    {
    /* NOTE:  update idle job stats */

    MStatClearUsage(mxoNONE,FALSE,TRUE,FALSE,TRUE,FALSE);
    }

  JobCount = 0;
  JIndex   = 0;

  /* apply state/queue policies */

  for (jindex = 0;tmpEJList[jindex] != NULL;jindex++)
    {
    J = tmpEJList[jindex];

    if (!MJobPtrIsValid(J))
      continue;

    JobCount++;

    if (MQueueSelectJob(J,PLevel,P,UpdateStats,TrackIdle,Diagnostic,RecordBlock,Msg) == FAILURE)
      {
      MDB(7,fSCHED) MLog("INFO:     Job '%s' not added to master list (%s)\n",
        J->Name,
        Msg);

      continue;
      }

    MDB(7,fSCHED) MLog("INFO:     Job '%s' added to master list\n",
      J->Name);

    EJList[JIndex++] = J;
    }  /* END for (jindex) */

  EJList[JIndex] = NULL;

  if (ECount != NULL)
    {
    *ECount = JIndex;
    }

  MDB(4,fSCHED)
    {
    MDB(4,fSCHED) MLog("INFO:     jobs selected:\n");

    for (jindex = 0;EJList[jindex] != NULL;jindex++)
      {
      MLogNH("[%03d: %s]",
        jindex,
        EJList[jindex]->Name);
      }  /* END for (jindex) */

    MLogNH("\n");
    }

  return(SUCCESS);
  }  /* END MQueueSelectAllJobs() */



/**
 * Convert a job to an mpc_t structure.
 * 
 * @param J
 * @param P
 */

int MPCFromJob(

  const mjob_t *J,
  mpc_t        *P)

  {
  double PE;

  if ((J == NULL) || (P == NULL))
    {
    return(SUCCESS);
    }

  if ((J->State == mjsStarting) || (J->State == mjsRunning))
    P->WC = J->RemainingTime;
  else
    P->WC = J->WCLimit;

  P->Job  = 1;    
  P->Proc = J->TotalProcCount;

  if (MJOBISACTIVE(J) == TRUE)
    {
    P->Node = J->Alloc.NC;
    }
  else if (J->Request.NC > 0)
    {
    P->Node = J->Request.NC;
    }
  else
    {
    P->Node = MAX(1,J->Request.TC / MAX(1,MPar[0].MaxPPN));
    }

  P->Mem = J->Req[0]->DRes.Mem;  /* FIXME: add multi-req support */
 
  P->PS = P->Proc * P->WC;

  MJobGetPE(J,&MPar[0],&PE);

  P->PE = (int)PE;

  MJobGetTotalGRes(J,&P->GRes);

  if (bmisset(&J->IFlags,mjifIsArrayJob))
    P->ArrayJob = 1;

  return(SUCCESS);
  }  /* END MPCFromJob() */


/**
 * Convert a reservation to an mpc_t structure.
 * 
 * @param R
 * @param P
 */

int MPCFromRsv(

  const mrsv_t *R,
  mpc_t        *P)

  {
  if ((R == NULL) || (P == NULL))
    {
    return(SUCCESS);
    }

  P->Job  = 1;
  P->Proc = (R->AllocPC > 0) ? R->AllocPC : (R->ReqTC * (R->DRes.Procs >= 1) ? R->DRes.Procs : 1);
  P->Node = MAX(R->ReqNC,R->AllocNC);
  P->WC   = R->EndTime - R->StartTime;
  P->PS   = P->Proc * P->WC;       
  P->Mem  = R->ReqMemory;

  MSNLCopy(&P->GRes,&R->DRes.GenericRes);

  return(SUCCESS);
  }  /* END MPCFromRsv() */




/**
 * Verify job meets idle, active, and or system policies.
 *
 * NOTE:  If job already evaluated against idle policies, need changes
 *
 * @see MJobCheckLimits() - child
 *
 * @param J (I) job
 * @param PLevel (I) level of policy enforcement
 * @param PTypeBM (I) [0 == ALL]
 * @param P (I) node partition
 * @param RIndex (O) [optional]
 * @param Buffer (O) [optional]
 * @param StartTime (I) time job will start
 */

int MJobCheckPolicies(

  mjob_t       *J,
  enum MPolicyTypeEnum PLevel,
  mbitmap_t    *PTypeBM,
  mpar_t       *P,
  enum MPolicyRejectionEnum *RIndex,
  char         *Buffer,
  long          StartTime)

  {
  mbitmap_t EffPTypeBM;

  const char *FName = "MJobCheckPolicies";

  MDB(5,fSCHED) MLog("%s(%s,%s,%d,%s,RIndex,%s,%ld)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MPolicyMode[PLevel],
    PTypeBM, 
    (P != NULL) ? P->Name : "NULL",
    (Buffer == NULL) ? "NULL" : "Buffer",
    StartTime);

  if ((J == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  if (PTypeBM != NULL)
    {
    bmcopy(&EffPTypeBM,PTypeBM);
    }
  else
    {
    bmset(&EffPTypeBM,mlActive);
    bmset(&EffPTypeBM,mlIdle);
    bmset(&EffPTypeBM,mlSystem);
    }

  if (bmisset(&J->IFlags,mjifIsEligible))
    {
    /* NOTE:  If job already eligible, idle and system limits do not need to be
              re-evaluated */

    if (bmisset(&EffPTypeBM,mlIdle))
      bmunset(&EffPTypeBM,mlIdle);

    if (bmisset(&EffPTypeBM,mlSystem))
      bmunset(&EffPTypeBM,mlSystem);
    }

  if (MJobCheckLimits(
        J,
        PLevel,
        P,
        &EffPTypeBM,
        (J->Array != NULL) ? &J->Array->PLimit : NULL,
        Buffer) == FAILURE)  
    {
    /* job limit rejection */

    if (RIndex != NULL)
      *RIndex = mprMaxJobPerUserPolicy;

    return(FAILURE);
    }

  if (Buffer != NULL)
    {
    sprintf(Buffer,"job %s passes all policies in partition %s\n",
      J->Name,
      P->Name);

    MDB(4,fSCHED) MLog("INFO:     %s",
      Buffer);
    }

  return(SUCCESS);
  }  /* END MJobCheckPolicies() */







/**
 * Check if credential policy limits are being violated.
 *
 * @see MJobCheckLimits() - parent
 * @see MPolicyCheckLimit() - peer
 *
 * success if usage allows requested delta
 * precedence: QOSP -> ObjectP -> DefaultP
 *
 * @param UsageDelta (I) - additional usage requested
 * @param EvalStandAlone (I)
 * @param PLevel (I)
 * @param PtIndex (I) [partition index]
 * @param ObjectP (I)
 * @param DefaultP (I)
 * @param QOSP (I)
 * @param LimitP (O)
 * @param Remaining (O) (optional) must be initialized
 * @param Override (O) [TRUE if override was used]
 */

int MPolicyCheckGLimit(

  const mpc_t *UsageDelta,
  mbool_t      EvalStandAlone,
  enum MPolicyTypeEnum PLevel,
  int          PtIndex,
  const mpu_t *ObjectP,
  const mpu_t *DefaultP,
  const mpu_t *QOSP,
  long        *LimitP,
  long        *Remaining,
  mbool_t     *Override)

  {
  int Usage;
  int Limit;
  int gindex;

  mgpu_t *OP = NULL;
  mgpu_t *DP = NULL;
  mgpu_t *QP = NULL;

  if (Override != NULL)
    *Override = FALSE;

  if (LimitP != NULL)
    *LimitP = 0;

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    Usage = MSNLGetIndexCount(&UsageDelta->GRes,gindex);

    if (Usage == 0)
      continue;

    /* if MUHTGet() returns success then we know something was set, we don't need to check its value */

    if ((ObjectP != NULL) &&
        (ObjectP->GLimit != NULL) &&
        (MUHTGet(ObjectP->GLimit,MGRes.Name[gindex],(void **)&OP,NULL) == SUCCESS))
      {
      Limit = (PLevel == mptSoft) ?
        OP->SLimit[PtIndex] :
        OP->HLimit[PtIndex];
      }
    else if ((DefaultP != NULL) &&
             (DefaultP->GLimit != NULL) &&
             (MUHTGet(DefaultP->GLimit,MGRes.Name[gindex],(void **)&DP,NULL) == SUCCESS))
      {
      Limit = (PLevel == mptSoft) ?
        DP->SLimit[PtIndex] :
        DP->HLimit[PtIndex];
      }
    else
      {
      /* no limit specified */

      Limit = -1;
      }

    if ((QOSP != NULL) &&
        (QOSP->GLimit != NULL) &&
        (MUHTGet(QOSP->GLimit,MGRes.Name[gindex],(void **)&QP,NULL) == SUCCESS))
      {
      Limit = (PLevel == mptSoft) ?
        QP->SLimit[PtIndex] :
        QP->HLimit[PtIndex];

      if (Override != NULL)
        *Override = TRUE;
      }

    if (EvalStandAlone == FALSE)
      {
      /* incorporate external usage */
   
      if (OP != NULL)
        Usage += OP->Usage[PtIndex];
      }
   
    if (LimitP != NULL)
      *LimitP = Limit;
   
    if ((Limit > -1) && (Usage > Limit))
      {
      if (Remaining != NULL)
        *Remaining = 0;
   
      return(FAILURE);
      }
    else if (Remaining != NULL)
      {
      Limit -= Usage;
   
      *Remaining = MIN(*Remaining,(int)ceil((double)Limit / Usage)) + 1;
      }  /* END if (Limit > 0)  */
    }    /* END for (gindex) */
 
  return(SUCCESS);
  }  /* END MPolicyCheckGLimit() */








/**
 * Check if credential policy limits are being violated.
 *
 * @see MJobCheckLimits() - parent
 * @see MPolicyCheckGLimit() - peer
 *
 * @param UsageDelta (I) - additional usage requested
 * @param EvalStandAlone (I)
 * @param PlIndex (I)
 * @param PLevel (I)
 * @param PtIndex (I) [partition index]
 * @param ObjectP (I)
 * @param DefaultP (I)
 * @param QOSP (I)
 * @param LimitP (O) 
 * @param Remaining (O) 
 * @param Override (O) [TRUE if override was used]
 */

int MPolicyCheckLimit(

  const mpc_t  *UsageDelta,
  mbool_t       EvalStandAlone,
  enum MActivePolicyTypeEnum PlIndex,
  enum MPolicyTypeEnum       PLevel,
  int           PtIndex,
  const mpu_t  *ObjectP,
  const mpu_t  *DefaultP,
  const mpu_t  *QOSP,
  long         *LimitP,
  long         *Remaining,
  mbool_t      *Override)

  {
  int Usage;
  int Limit;

  /* return limit */
  /* success if usage allows requested delta */

  /* precedence: QOSP -> ObjectP -> DefaultP */

  if (PlIndex == mptMaxGRes)
    {
    return (MPolicyCheckGLimit(UsageDelta,EvalStandAlone,PLevel,PtIndex,ObjectP,DefaultP,QOSP,LimitP,Remaining,Override));
    }

  if (LimitP != NULL)
    *LimitP = 0;

  if (Override != NULL)
    *Override = FALSE;

  if ((ObjectP != NULL) && (ObjectP->HLimit[PlIndex][PtIndex] >= 0))  
    {
    /* use object */

    Limit = (PLevel == mptSoft) ?
      ObjectP->SLimit[PlIndex][PtIndex] :
      ObjectP->HLimit[PlIndex][PtIndex];
    }
  else if ((DefaultP != NULL) && (DefaultP->HLimit[PlIndex][PtIndex] >= 0))  
    {
    /* use default object */

    Limit = (PLevel == mptSoft) ?
      DefaultP->SLimit[PlIndex][PtIndex] :
      DefaultP->HLimit[PlIndex][PtIndex];
    }
  else
    {
    /* no limit specified */

    Limit = -1;
    }

  if ((QOSP != NULL) && (QOSP->HLimit[PlIndex][PtIndex] != 0))
    {
    /* use QOS */

    if ((MSched.AlwaysApplyQOSLimitOverride == TRUE) || (Limit != 0))
      {
      Limit = (PLevel == mptSoft) ?
        QOSP->SLimit[PlIndex][PtIndex] :
        QOSP->HLimit[PlIndex][PtIndex];
      }

    if (Override != NULL)
      *Override = TRUE;
    }

  if (Limit < 0)
    {
    return(SUCCESS);
    }

  if (LimitP != NULL)
    *LimitP = Limit;

  switch (PlIndex)
    {
    case mptMaxJob:       Usage = UsageDelta->Job;       break;
    case mptMaxArrayJob:  Usage = UsageDelta->ArrayJob;  break;
    case mptMaxNode:      Usage = UsageDelta->Node;      break;
    case mptMaxPE:        Usage = UsageDelta->PE;        break;
    case mptMaxProc:      Usage = UsageDelta->Proc;      break;
    case mptMaxPS:        Usage = UsageDelta->PS;        break;
    case mptMaxWC:        Usage = UsageDelta->WC;        break;
    case mptMaxMem:       Usage = UsageDelta->Mem;       break;
    case mptMinProc:      Usage = UsageDelta->Proc;      break;
    case mptMaxGRes:      Usage = MSNLGetIndexCount(&UsageDelta->GRes,0); break;
    default: Usage = 0; break;
    }

  if (EvalStandAlone == FALSE)
    {
    /* incorporate external usage */

    if (ObjectP != NULL)
      Usage += ObjectP->Usage[PlIndex][PtIndex];
    }

  switch (PlIndex)
    {
    case mptMinProc:

      if (Usage < Limit)
        {
        return(FAILURE);
        }

      break;

    default:

      if (Usage > Limit)
        {
        if (Remaining != NULL)
          *Remaining = 0;

        return(FAILURE);
        }
      else if (Remaining != NULL)
        {
        Limit -= Usage;

        *Remaining = MIN(*Remaining,(int)ceil((double)Limit / Usage)) + 1;
        }

      break;
    }  /* END switch (PlIndex) */
 
  return(SUCCESS);
  }  /* END MPolicyCheckLimit() */







/**
 * Determine earliest time all job policies can be satisfied.
 *
 * @see MRsvConfigure() - parent
 * @see MJobReserve() - parent
 * @see MClusterShowARes() - parent
 *
 * NOTE:  This routine can be expensive, looping through all policies, all jobs, and all events.
 *
 * NOTE:  This routine should not be called in high-throughput mode
 *
 * @param O (I) [requestor]
 * @param OIndex (I)
 * @param P (I)
 * @param PLevel (I)
 * @param Time (I/O)
 * @param EMsg (O) [optional,minsize=MMAX_TIME]
 */

int MPolicyGetEStartTime(

  void                 *O,      /* I (requestor) */
  enum MXMLOTypeEnum    OIndex, /* I */
  mpar_t               *P,      /* I */
  enum MPolicyTypeEnum  PLevel, /* I */
  long                 *Time,   /* I/O */
  char                 *EMsg)   /* O (optional,minsize=MMAX_TIME) */

  {
  /* Parameter:Time  I:  optional/earliest allowed start time */
  /*                 O:  required/earliest located start time */

  long    AvailStart;

  mjob_t *SJ = NULL;  /* set to avoid compiler warning */
  mrsv_t *SR = NULL;  /* set to avoid compiler warning */

  mpu_t   PAvailable[mxoALL];

  mpc_t   PConsumed;

  int     oindex;
  int     pindex;

  char   *ONameP;

  enum MXMLOTypeEnum OList[] = { 
    mxoUser, 
    mxoGroup, 
    mxoAcct, 
    mxoQOS, 
    mxoClass, 
    mxoALL };

  enum MActivePolicyTypeEnum PList[] = { 
    mptMaxJob, 
    mptMaxProc, 
    mptMaxPS, 
    mptMaxPE, 
    mptMaxNode, 
    mptMaxGRes,
    mptNONE };

  enum MActivePolicyTypeEnum PIndex;

  mbool_t ViolationDetected[mxoALL];

  char    OAttrs[mxoALL][MMAX_NAME];   /* mxoXXX based attr names */

  mpu_t  *OP;
  mpu_t  *DP;
  mpu_t  *QP;
 
  int ZList[mptLAST][MMAX_PAR];

  const char *FName = "MPolicyGetEStartTime";

  MDB(4,fSCHED) MLog("%s(%s,%s,%s,%s,%ld,EMsg)\n",
    FName,
    (O != NULL) ? "O" : "NULL",
    MXO[OIndex],
    (P != NULL) ? P->Name : "NULL",
    MPolicyMode[PLevel],
    (Time != NULL) ? *Time : -1);     

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((O == NULL) || (Time == NULL))
    {
    MDB(2,fSCHED) MLog("INFO:     invalid parameters passed to %s\n",
      FName);

    if (Time != NULL)
      *Time = MMAX_TIME;

    return(FAILURE);
    }

  if (MSched.NoEnforceFuturePolicy == TRUE)
    {
    *Time = MSched.Time;

    return(SUCCESS);
    }

  /* apply resource consumption policies */
  /* return earliest time all policies can be satisfied */

  /* NOTE:  must incorporate two-dimensional throttling policies    */
  /* NOTE:  all reservation EStart estimates based on 'soft' limits */

  /* NOTE:  must handle rsv policies */

  AvailStart  = MAX((long)MSched.Time,*Time);

  *Time = MMAX_TIME;

  memset(ViolationDetected,FALSE,sizeof(ViolationDetected));

  /* initialize credentials */

  memset(&OAttrs,0,sizeof(OAttrs));

  memset(&PAvailable,0,sizeof(PAvailable));             
  memset(&PConsumed,0,sizeof(PConsumed));       

  MSNLInit(&PConsumed.GRes);

  memset(&ZList,0,sizeof(ZList));

  /* record object consumption */

  switch (OIndex)
    {
    case mxoJob:
    default:

      SJ = (mjob_t *)O;

      ONameP = SJ->Name;

      MPCFromJob(SJ,&PConsumed);

      if (SJ->Credential.Q != NULL)
        QP = SJ->Credential.Q->L.OverrideActivePolicy;
      else
        QP = NULL;

      break;

    case mxoRsv:

      SR = (mrsv_t *)O;

      ONameP = SR->Name;

      MPCFromRsv(SR,&PConsumed);

      QP = NULL;

      break;
    }  /* END switch (OIndex) */

  /* determine all hard limits */             

  for (oindex = 0;OList[oindex] != mxoALL;oindex++)
    {
    MOGetCredUsage(O,OIndex,OList[oindex],&OP,&DP,&QP);

    for (pindex = 0;PList[pindex] != mptNONE;pindex++)
      {
      PIndex = PList[pindex];

      if (MPolicyCheckLimit(
            &PConsumed,
            TRUE,
            PIndex,
            PLevel,
            0,
            OP,
            DP,
            QP,
            &PAvailable[OList[oindex]].HLimit[PIndex][0],
            NULL,
            NULL) == FAILURE)
        {
        /* no start times possible */

        if (EMsg != NULL)
          {
          sprintf(EMsg,"job cannot meet constraints of %s %s per %s - limit=%ld",
            MPolicyMode[PLevel],
            MPolicyType[PIndex],
            MXO[OList[oindex]],
            PAvailable[OList[oindex]].HLimit[PIndex][0]);
          }

        MDB(6,fSCHED) MLog("INFO:     job cannot meet constraints of %s %s per %s - limit=%d",
          MPolicyMode[PLevel],
          MPolicyType[PIndex],
          MXO[OList[oindex]],
          PAvailable[OList[oindex]].HLimit[PIndex][0]);

        MSNLFree(&PConsumed.GRes);

        return(FAILURE);
        }
     
      /* limit loaded into PAvailable */

      if (PAvailable[OList[oindex]].HLimit[PIndex][0] > 0)
        {
        int AdjustVal;

        /* if limit set, reduce limit by job consumption */

        switch (PList[pindex])
          {
          case mptMaxJob:  AdjustVal = PConsumed.Job;  break;
          case mptMaxNode: AdjustVal = PConsumed.Node; break;
          case mptMaxPE:   AdjustVal = PConsumed.PE;   break;
          case mptMaxProc: AdjustVal = PConsumed.Proc; break;
          case mptMaxPS:   AdjustVal = PConsumed.PS;   break;
          case mptMaxWC:   AdjustVal = PConsumed.WC;   break;
          case mptMaxMem:  AdjustVal = PConsumed.Mem;  break;
          case mptMinProc: AdjustVal = PConsumed.Proc; break;
          case mptMaxGRes: AdjustVal = MSNLGetIndexCount(&PConsumed.GRes,0); break;
          default: AdjustVal = 0; break;
          }

        PAvailable[OList[oindex]].Usage[PIndex][0] =
          PAvailable[OList[oindex]].HLimit[PIndex][0] -  
          AdjustVal;
        }
      }  /* END for (pindex) */
    }    /* END for (oindex) */

  switch (OIndex)
    {
    case mxoJob:
    default:

      if (SJ->Credential.U != NULL)
        strcpy(OAttrs[mxoUser],SJ->Credential.U->Name);

      if (SJ->Credential.G != NULL)
        strcpy(OAttrs[mxoGroup],SJ->Credential.G->Name);          

      if (SJ->Credential.A != NULL)
        strcpy(OAttrs[mxoAcct],SJ->Credential.A->Name);          

      if (SJ->Credential.Q != NULL)
        strcpy(OAttrs[mxoQOS],SJ->Credential.Q->Name);          

      if (SJ->Credential.C != NULL)
        strcpy(OAttrs[mxoClass],SJ->Credential.C->Name);

      break;

    case mxoRsv:

      if (SR->U != NULL)
        strcpy(OAttrs[mxoUser],SR->U->Name);

      if (SR->G != NULL)
        strcpy(OAttrs[mxoGroup],SR->G->Name);

      if (SR->A != NULL)
        strcpy(OAttrs[mxoAcct],SR->A->Name);

      if (SR->Q != NULL)
        strcpy(OAttrs[mxoQOS],SR->Q->Name);

      break;
    }  /* END switch (OIndex) */

#if 0  /* we need to get a replacement for this code.  It is expensive but could be essential
          in determining future start-times */
  long    Duration;
  int     reindex;
  macl_t *tACL;
  mrsv_t *R;
  char    ViolationMessage[MMAX_LINE];
  enum MXMLOTypeEnum tOIndex;

  /* step through global reservation events */

  /* for each event time, determine if policy can be satisfied */

  for (reindex = 0;reindex < MMAX_RSVEVENTS;reindex++)
    {
    if (MRE[reindex].Type == mreNONE)
      break;

    if (MRE[reindex].R == NULL)
      continue;

    if ((AvailStart > 0) &&
       ((MRE[reindex].Time - AvailStart) >= Duration))
      {
      /* adequate slot located */
      char TString[MMAX_LINE];
 
      *Time = AvailStart;

      MULToTString(AvailStart - MSched.Time,TString);

      MDB(4,fSCHED) MLog("INFO:     adequate policy slot located at time %s for %s %s\n",
        TString,
        MXO[OIndex],
        ONameP);
 
      MSNLFree(&PConsumed.GRes);

      return(SUCCESS);
      }

    R = MRE[reindex].R;

    if ((R == NULL) || (R->Name[0] == '\0') || (R->Name[0] == '\1'))
      continue;

    if (R->CL == NULL)
      {
      /* no accountable creds associated w/rsv */

      continue;
      }

    if ((OIndex == mxoJob) && (R->Type != mrtJob))
      {
      /* check if R is VPC map */

      if (bmisset(&R->Flags,mrfIsVPC) && bmisset(&R->Flags,mrfSystemJob))
        {
        /* NO-OP */
        }
      else
        {
        /* rsv's only check rsv usage */

        continue;
        }
      }

    if ((OIndex == mxoRsv) && (R->Type == mrtJob))
      {
      /* jobs only check job usage */

      continue;
      }

    /* NOTE:  assumes no policy violations will occur after policy EStartTime */
    /*        must be changed! */

    /* step through CL, determine which policies must be enforced */

    for (tACL = R->CL;tACL != NULL;tACL = tACL->Next)
      {
      tOIndex = MAToO[(int)tACL->Type];

      ViolationMessage[0] = '\0';

      if (tOIndex == mxoNONE)
        {
        /* no consumption tracking */

        continue;
        }

      if ((tACL->Name == NULL) || (strcmp(tACL->Name,OAttrs[tOIndex]) != 0))
        {
        /* requestor attributes do not match reservation cred */

        continue;
        }

      if (!memcmp(PAvailable[tOIndex].HLimit,ZList,sizeof(ZList)))
        {
        /* no applicable limits set */

        continue;
        }

      if (MRE[reindex].Type == mreStart)
        {
        /* must handle dynamic limits on a per reservation */

        /* NOTE:  decrease PS usage for all jobs not completing at time T by time dT */

        MPolicyAdjustUsage(NULL,R,NULL,mlActive,PAvailable,tOIndex,-1,ViolationMessage,&ViolationDetected[tOIndex]);
        }  /* END if (MRE[reindex].Type == mreStart) */
      else
        {
        /* must handle dynamic procsecond usage tracking */            

        MPolicyAdjustUsage(NULL,R,NULL,mlActive,PAvailable,tOIndex,1,ViolationMessage,&ViolationDetected[tOIndex]);
        }  /* END else (MRE[reindex].Type == mreStart) */

      /* do we check each iteration or only when moving to new time? */

      if (memchr(ViolationDetected,TRUE,sizeof(ViolationDetected)))
        {
        /* policy violation detected */              

        AvailStart = 0;

        if (ViolationMessage[0] != '\0')
          MDB(7,fSCHED) MLog("INFO:     violation detected '%s'\n",ViolationMessage);
        }
      else 
        {
        /* no policy violation detected */

        if (AvailStart == 0)
          AvailStart = MAX((long)MSched.Time,MRE[reindex].Time);
        }  /* END else (ViolationDetected == TRUE) */
      }    /* END for (aindex) */
    }      /* END for (reindex) */
#endif

  MSNLFree(&PConsumed.GRes);

  if (AvailStart > 0)
    {
    char TString[MMAX_LINE];

    *Time = AvailStart;

    MULToTString(AvailStart - MSched.Time,TString);

    MDB(4,fSCHED) MLog("INFO:     policy start time found for %s %s in %s\n",
      MXO[OIndex],
      ONameP,
      TString);

    return(SUCCESS);
    }
 
  MDB(4,fSCHED) MLog("INFO:     no policy start time found for %s %s\n",
    MXO[OIndex],
    ONameP);
 
  return(FAILURE);
  }  /* END MPolicyGetEStartTime() */



/**
 * Adjust the usage of an entry in mgpu_t to reflect changes in workload.
 *
 * NOTE: this routine does everything twice, once for the GLOBAL partition
 *       and another for ParIndex, this keeps track of total usage and 
 *       per-partition usage.
 *
 * @see __MPolicyAdjustSingleUsage() (parallel routine)
 *
 * @param P
 * @param Usage
 * @param ParIndex
 * @param OType1
 * @param OType2
 * @param ViolationDetected (O)
 * @param EMsg (O)
 */

int __MPolicyAdjustSingleGUsage(

  mgpu_t                    *P,
  int                        Usage,
  int                        ParIndex,
  enum MXMLOTypeEnum         OType1,
  enum MXMLOTypeEnum         OType2,
  mbool_t                   *ViolationDetected,
  char                      *EMsg)

  {
  /* this is a hack, replicated several times, to handle a race condition
   * where if a job's proccount is increased after its idle usage has been 
   * added in but before it has been subtracted back out again, 
   * more idle usage will be subtracted out at this point,
   * possibly causing and unsigned integer underflow.
   * The race condition still exists and can cause other accounting
   * inaccuracies. (Sean Reque)
   */

  if (((int)P->Usage[ParIndex]) + Usage < 0)
    {
    P->Usage[ParIndex] = 0;

    if ((ViolationDetected != NULL) &&
        ((P->SLimit[ParIndex] > 0) || (P->HLimit[ParIndex] > 0)))
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"INFO:    Violates %s[%s] MAXGRES\n",
          (OType1 != mxoNONE) ? MXO[OType1] : "",
          (OType2 != mxoNONE) ? MXO[OType2] : "");
        }

      *ViolationDetected = TRUE;
      }
    }
  else
    {
    P->Usage[ParIndex] += Usage;
    }

  return(SUCCESS);
  }  /* END __MPolicyAdjustSingleGUsage() */






/**
 * Adjust the usage of an entry in mpu_t to reflect changes in workload.
 *
 * NOTE: this routine does everything twice, once for the GLOBAL partition
 *       and another for ParIndex, this keeps track of total usage and 
 *       per-partition usage.
 *
 * @see __MPolicyAdjustSingleUsage() (parallel routine)
 *
 * @param P
 * @param Usage
 * @param PIndex
 * @param ParIndex
 * @param OType1
 * @param OType2
 * @param ViolationDetected (O)
 * @param EMsg (O)
 */

int __MPolicyAdjustSingleUsage(

  mpu_t                     *P,
  int                        Usage,
  enum MActivePolicyTypeEnum PIndex,
  int                        ParIndex,
  enum MXMLOTypeEnum         OType1,
  enum MXMLOTypeEnum         OType2,
  mbool_t                   *ViolationDetected,
  char                      *EMsg)

  {
  /* this is a hack, replicated several times, to handle a race condition
   * where if a job's proccount is increased after its idle usage has been 
   * added in but before it has been subtracted back out again, 
   * more idle usage will be subtracted out at this point,
   * possibly causing and unsigned integer underflow.
   * The race condition still exists and can cause other accounting
   * inaccuracies. (Sean Reque)
   */

  if (((int)P->Usage[PIndex][ParIndex]) + Usage < 0)
    {
    P->Usage[PIndex][ParIndex] = 0;

    if ((ViolationDetected != NULL) &&
        ((P->SLimit[PIndex][ParIndex] > 0) || (P->HLimit[PIndex][ParIndex] > 0)))
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"INFO:    Violates %s[%s] %s\n",
          (OType1 != mxoNONE) ? MXO[OType1] : "",
          (OType2 != mxoNONE) ? MXO[OType2] : "",
          MPolicyType[PIndex]);
        }

      *ViolationDetected = TRUE;
      }
    }
  else
    {
    P->Usage[PIndex][ParIndex] += Usage;
    }

  return(SUCCESS);
  }  /* END __MPolicyAdjustSingleUsage() */




/**
 * Adjust the usage of an mpu_t to reflect changes in workload.
 *
 * NOTE: this routine does everything twice, once for the GLOBAL partition
 *       and another for ParIndex, this keeps track of total usage and 
 *       per-partition usage.
 *
 * @param J
 * @param P
 * @param PConsumed
 * @param Count
 * @param ParIndex
 * @param OType1
 * @param OType2
 * @param ViolationDetected (O)
 * @param EMsg (O)
 */

int __MPolicyAdjustUsage(

  mjob_t  *J,
  mpu_t   *P,
  mpc_t   *PConsumed,
  int      Count,
  int      ParIndex,
  enum     MXMLOTypeEnum OType1,
  enum     MXMLOTypeEnum OType2,
  mbool_t *ViolationDetected,
  char    *EMsg)

  {
  int pindex;

  if (PConsumed == NULL)
    {
    return(FAILURE);
    }

  for (pindex = mptNONE;pindex < mptLAST;pindex++)
    {
    int AdjustVal;

    if ((J != NULL) && 
        (J->Credential.Q != NULL) &&
        (bmisset(&J->Credential.Q->ExemptFromLimits,pindex)))
      {
      continue;
      }

    if (pindex == mptMaxGRes)
      {
      int gindex;

      mgpu_t *GP = NULL;

      if ((P->GLimit == NULL) ||
          (MSNLGetIndexCount(&PConsumed->GRes,0) == 0))
        {
        /* if the job is not using any gres or there are no gres limits then nothing to track, just move on */

        continue;
        }

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        AdjustVal = Count * MSNLGetIndexCount(&PConsumed->GRes,gindex);

        if (AdjustVal == 0)
          continue;

        if (MUHTGet(P->GLimit,MGRes.Name[gindex],(void **)&GP,NULL) == FAILURE)
          continue;

        __MPolicyAdjustSingleGUsage(GP,AdjustVal,0,mxoNONE,mxoNONE,ViolationDetected,EMsg);

        /* ParIndex update */
   
        if (ParIndex > 0)
          __MPolicyAdjustSingleGUsage(GP,AdjustVal,ParIndex,mxoNONE,mxoNONE,ViolationDetected,EMsg);
        }
      }
    else
      {
      switch (pindex)
        {
        case mptMaxJob:      AdjustVal = Count * PConsumed->Job;  break;
        case mptMaxNode:     AdjustVal = Count * PConsumed->Node; break;
        case mptMaxPE:       AdjustVal = Count * PConsumed->PE;   break;
        case mptMaxProc:     AdjustVal = Count * PConsumed->Proc; break;
        case mptMaxPS:       AdjustVal = Count * PConsumed->PS;   break;
        case mptMaxWC:       AdjustVal = Count * PConsumed->WC;   break;
        case mptMaxMem:      AdjustVal = Count * PConsumed->Mem;  break;
        case mptMinProc:     AdjustVal = Count * PConsumed->Proc; break;
        case mptMaxArrayJob: AdjustVal = Count * PConsumed->ArrayJob; break;
        default: AdjustVal = 0; break;
        }
   
      if (AdjustVal == 0)
        continue;

      /* GLOBAL update */
   
      __MPolicyAdjustSingleUsage(P,AdjustVal,(enum MActivePolicyTypeEnum)pindex,0,mxoNONE,mxoNONE,ViolationDetected,EMsg);

      /* ParIndex update */
   
      if (ParIndex > 0)
        __MPolicyAdjustSingleUsage(P,AdjustVal,(enum MActivePolicyTypeEnum)pindex,ParIndex,mxoNONE,mxoNONE,ViolationDetected,EMsg);
      }  /* END else */
    }    /* END for (pindex) */

  return(SUCCESS);
  }  /* END __MPolicyAdjustUsage() */





/**
 * Update policy usage.
 *
 * @param J                 (I/O) [optional]
 * @param R                 (I) [optional]
 * @param Partition         (I) [optional]
 * @param LimitType         (I) limit type
 * @param PU                (O) [optional]
 * @param OIndex            (I) object index
 * @param Count             (I) consumer count
 * @param EMsg              (I) [optional]
 * @param ViolationDetected (O) [optional]
 */

int MPolicyAdjustUsage(

  mjob_t  *J,
  mrsv_t  *R,
  mpar_t  *Partition,
  enum MLimitTypeEnum LimitType,
  mpu_t   *PU,
  enum MXMLOTypeEnum OIndex,
  int      Count,
  char    *EMsg,
  mbool_t *ViolationDetected)

  {
  int VCCount = 0;
  int PIndex;

  mpc_t  PConsumed;

  int oindex1;
  int oindex2;

  enum MXMLOTypeEnum OList1[] = {
    mxoNONE,
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoTNode,
    mxoALL };

  enum MXMLOTypeEnum OList2[] = {
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoPar,
    mxoALL };

  mpu_t *P;

  int tindex;

  mpu_t *tPU[mxoALL][mxoALL];

  static mpu_t **fstPU;

  mpu_t **VCPU = NULL;

  const char *FName = "MPolicyAdjustUsage";

  MDB(8,fSCHED) MLog("%s(%s,%s,%s,%s,%s,%d,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    MTLimitType[LimitType],
    (PU != NULL) ? "PU" : "NULL",
    MXO[OIndex],
    Count,
    (EMsg != NULL) ? "EMsg" : "NULL",
    (ViolationDetected != NULL) ? "ViolationDetected" : "NULL");

  if ((J != NULL) && (J->Req[0] == NULL))
    {
    return(FAILURE);
    }
  
  /* NOTE: OIndex indicates second dimension of two dimensional limit array */
  /*       OIndex==mxoALL,mxoNONE indicates update single dimensional limit usage only */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((MSched.FSTreeDepth > 0) || ((J != NULL) && (J->TStats != NULL)))
    {
    if (fstPU == NULL)
      {
      fstPU = (mpu_t **)MUCalloc(1,sizeof(mpu_t *) * (MSched.FSTreeDepth + MMAX_TJOB + 1));
      }
    else
      {
      fstPU[0] = NULL;
      }
    }

  if ((J != NULL) && (MJobGetVCThrottleCount(J,&VCCount) == SUCCESS) && (VCCount > 0))
    {
    VCPU = (mpu_t **)MUCalloc(1,sizeof(mpu_t *) * (VCCount + 1));
    }

  if (OIndex == mxoALL)
    OIndex = mxoNONE;

  /* adjust all policies */

  PIndex = 0;

  /* handle object consumption */    

  if (J != NULL)
    {
    memset(&PConsumed,0,sizeof(PConsumed));        

    MSNLInit(&PConsumed.GRes);

    MPCFromJob(J,&PConsumed);

    if (PU == NULL)
      {
      if ((Partition != NULL) && (Partition->Index > 0))
        PIndex = Partition->Index;
      else
        PIndex = J->Req[0]->PtIndex;
      }
    }
  else if (R != NULL)
    {
    memset(&PConsumed,0,sizeof(PConsumed));           

    MSNLInit(&PConsumed.GRes);

    MPCFromRsv(R,&PConsumed);
    }
  else
    {
    return(FAILURE);
    }                       

  /* add consumption state */

  memset(tPU,0,sizeof(tPU));

  if (PU == NULL)
    {
    switch (LimitType)
      {
      case mlIdle:

        if (J->Credential.U != NULL)
          {
          tPU[mxoNONE][mxoUser] = J->Credential.U->L.IdlePolicy;

          /* multi-dimensional idle policies */

          if ((J->Credential.U->L.ClassIdlePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.U->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoUser],NULL);
            }

          if ((J->Credential.U->L.AcctIdlePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.U->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoUser],NULL);
            }

          if ((J->Credential.U->L.GroupIdlePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.U->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoUser],NULL);
            }

          if ((J->Credential.U->L.QOSIdlePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.U->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoUser],NULL);
            }
          }

        if (J->Credential.G != NULL)
          {
          tPU[mxoNONE][mxoGroup] = J->Credential.G->L.IdlePolicy;

          /* multi-dimensional idle policies */

          if ((J->Credential.G->L.ClassIdlePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.G->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoGroup],NULL);
            }

          if ((J->Credential.G->L.AcctIdlePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.G->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoGroup],NULL);
            }

          if ((J->Credential.G->L.UserIdlePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.G->L.UserIdlePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoGroup],NULL);
            }

          if ((J->Credential.G->L.QOSIdlePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.G->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoGroup],NULL);
            }
          }

        if (J->Credential.A != NULL)
          {
          tPU[mxoNONE][mxoAcct] = J->Credential.A->L.IdlePolicy;

          /* multi-dimensional idle policies */

          if ((J->Credential.A->L.ClassIdlePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.A->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoAcct],NULL);
            }

          if ((J->Credential.A->L.GroupIdlePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.A->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoAcct],NULL);
            }

          if ((J->Credential.A->L.UserIdlePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.A->L.UserIdlePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoAcct],NULL);
            }

          if ((J->Credential.A->L.QOSIdlePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.A->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoAcct],NULL);
            }
          }

        if (J->Credential.Q != NULL)
          {
          tPU[mxoNONE][mxoQOS] = J->Credential.Q->L.IdlePolicy;

          /* multi-dimensional idle policies */

          if ((J->Credential.Q->L.ClassIdlePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.Q->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoQOS],NULL);
            }

          if ((J->Credential.Q->L.GroupIdlePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.Q->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoQOS],NULL);
            }

          if ((J->Credential.Q->L.UserIdlePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.Q->L.UserIdlePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoQOS],NULL);
            }

          if ((J->Credential.Q->L.AcctIdlePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.Q->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoQOS],NULL);
            }
          }

        if (J->Credential.C != NULL)
          {
          tPU[mxoNONE][mxoClass] = J->Credential.C->L.IdlePolicy;

          /* multi-dimensional idle policies */

          if ((J->Credential.C->L.ClassIdlePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.C->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoClass],NULL);
            }

          if ((J->Credential.C->L.GroupIdlePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.C->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoClass],NULL);
            }

          if ((J->Credential.C->L.UserIdlePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.C->L.UserIdlePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoClass],NULL);
            }

          if ((J->Credential.C->L.AcctIdlePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.C->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoClass],NULL);
            }
          }

        if ((J->Req[0]->PtIndex > 0) &&
            (J->FairShareTree != NULL) &&
            (J->FairShareTree[J->Req[0]->PtIndex] != NULL))
          {
          mfst_t *TData = J->FairShareTree[J->Req[0]->PtIndex]->TData;

          tindex = 0;

          while (TData != NULL)
            {
            fstPU[tindex++] = TData->L->IdlePolicy;

            if (TData->Parent == NULL)
              break;

            TData = TData->Parent->TData;
            }

          fstPU[tindex] = NULL;         
          }
        else if ((J->FairShareTree != NULL) &&
                 (J->FairShareTree[0] != NULL))
          {
          mfst_t *TData = J->FairShareTree[0]->TData;

          tindex = 0;

          while (TData != NULL)
            {
            fstPU[tindex++] = TData->L->IdlePolicy;

            if (TData->Parent == NULL)
              break;

            TData = TData->Parent->TData;
            }

          fstPU[tindex] = NULL;         
          }

        
        tindex = MSched.FSTreeDepth;

        if (J->TStats != NULL)
          {
          mln_t  *TStats;
          mjob_t *TJ;
     
          for (TStats = J->TStats;TStats != NULL;TStats = TStats->Next)
            {
            if (TStats->Ptr == NULL)
              continue;
     
            TJ = (mjob_t *)TStats->Ptr;
     
            if (TJ->ExtensionData == NULL)
              continue;
     
            fstPU[tindex++] = ((mtjobstat_t *)TJ->ExtensionData)->L.IdlePolicy;
            }  /* END for (TStats) */
          }    /* END if (J->Stat != NULL) */

        break;

      case mlActive:
      default:
 
        if (J->Credential.U != NULL)
          {
          tPU[mxoNONE][mxoUser] = &J->Credential.U->L.ActivePolicy;

          if ((J->Credential.U->L.ClassActivePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.U->L.ClassActivePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoUser],NULL);
            }

          if ((J->Credential.U->L.QOSActivePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.U->L.QOSActivePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoUser],NULL);
            }

          if ((J->Credential.U->L.GroupActivePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.U->L.GroupActivePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoUser],NULL);
            }

          if ((J->Credential.U->L.AcctActivePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.U->L.AcctActivePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoUser],NULL);
            }
          }   /* END if (J->Cred.U != NULL) */

        if (J->Credential.G != NULL)
          {
          tPU[mxoNONE][mxoGroup] = &J->Credential.G->L.ActivePolicy;

          if ((J->Credential.G->L.ClassActivePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.G->L.ClassActivePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoGroup],NULL);
            }

          if ((J->Credential.G->L.QOSActivePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.G->L.QOSActivePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoGroup],NULL);
            }

          if ((J->Credential.G->L.UserActivePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.G->L.UserActivePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoGroup],NULL);
            }

          if ((J->Credential.G->L.AcctActivePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.G->L.AcctActivePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoGroup],NULL);
            }
          }
 
        if (J->Credential.A != NULL)
          {
          tPU[mxoNONE][mxoAcct] = &J->Credential.A->L.ActivePolicy;

          if ((J->Credential.A->L.UserActivePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.A->L.UserActivePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoAcct],NULL);
            }

          if ((J->Credential.A->L.QOSActivePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.A->L.QOSActivePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoAcct],NULL);
            }

          if ((J->Credential.A->L.GroupActivePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.A->L.GroupActivePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoAcct],NULL);
            }

          if ((J->Credential.A->L.ClassActivePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.A->L.ClassActivePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoAcct],NULL);
            }
          }   /* END if (J->Cred.A != NULL) */

        if (J->Credential.Q != NULL)
          {
          tPU[mxoNONE][mxoQOS] = &J->Credential.Q->L.ActivePolicy;

          if ((J->Credential.Q->L.UserActivePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.Q->L.UserActivePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoQOS],NULL);
            }

          if ((J->Credential.Q->L.ClassActivePolicy != NULL) && (J->Credential.C != NULL))
            {
            MUHTGet(J->Credential.Q->L.ClassActivePolicy,J->Credential.C->Name,(void **)&tPU[mxoClass][mxoQOS],NULL);
            }

          if ((J->Credential.Q->L.GroupActivePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.Q->L.GroupActivePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoQOS],NULL);
            }

          if ((J->Credential.Q->L.AcctActivePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.Q->L.AcctActivePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoQOS],NULL);
            }
          }
 
        if (J->Credential.C != NULL)
          {
          tPU[mxoNONE][mxoClass] = &J->Credential.C->L.ActivePolicy;

          if ((J->Credential.C->L.AcctActivePolicy != NULL) && (J->Credential.A != NULL))
            {
            MUHTGet(J->Credential.C->L.AcctActivePolicy,J->Credential.A->Name,(void **)&tPU[mxoAcct][mxoClass],NULL);
            }

          if ((J->Credential.C->L.UserActivePolicy != NULL) && (J->Credential.U != NULL))
            {
            MUHTGet(J->Credential.C->L.UserActivePolicy,J->Credential.U->Name,(void **)&tPU[mxoUser][mxoClass],NULL);
            }

          if ((J->Credential.C->L.GroupActivePolicy != NULL) && (J->Credential.G != NULL))
            {
            MUHTGet(J->Credential.C->L.GroupActivePolicy,J->Credential.G->Name,(void **)&tPU[mxoGroup][mxoClass],NULL);
            }

          if ((J->Credential.C->L.QOSActivePolicy != NULL) && (J->Credential.Q != NULL))
            {
            MUHTGet(J->Credential.C->L.QOSActivePolicy,J->Credential.Q->Name,(void **)&tPU[mxoQOS][mxoClass],NULL);
            }
          }  /* END if (J->Cred.C != NULL) */


        /* NOTE:  update leaf node and all parent nodes with usage info */

        if ((J->Req[0]->PtIndex > 0) &&
            (J->FairShareTree != NULL) &&
            (J->FairShareTree[J->Req[0]->PtIndex] != NULL))
          {
          mfst_t *TData = J->FairShareTree[J->Req[0]->PtIndex]->TData;

          tindex = 0;

          while (TData != NULL)
            {
            fstPU[tindex++] = &TData->L->ActivePolicy;

            if (TData->Parent == NULL)
              break;

            TData = TData->Parent->TData;
            }

          fstPU[tindex] = NULL;         
          }
        else if ((J->FairShareTree != NULL) &&
                 (J->FairShareTree[0] != NULL))
          {
          mfst_t *TData = J->FairShareTree[0]->TData;

          tindex = 0;

          while (TData != NULL)
            {
            fstPU[tindex++] = &TData->L->ActivePolicy;

            if (TData->Parent == NULL)
              break;

            TData = TData->Parent->TData;
            }

          fstPU[tindex] = NULL;         
          }

        tindex = MSched.FSTreeDepth;

        if (J->TStats != NULL)
          {
          mln_t  *TStats;
          mjob_t *TJ;
     
          for (TStats = J->TStats;TStats != NULL;TStats = TStats->Next)
            {
            if (TStats->Ptr == NULL)
              continue;
     
            TJ = (mjob_t *)TStats->Ptr;
     
            if (TJ->ExtensionData == NULL)
              continue;
     
            fstPU[tindex++] = &((mtjobstat_t *)TJ->ExtensionData)->L.ActivePolicy;
            }  /* END for (TStats) */
          }    /* END if (J->Stat != NULL) */

        if (VCPU != NULL)
          {
          void  *Ptr;

          mln_t *tmpL;
          mvc_t *tmpVC;

          VCCount = 0;

          /* populate VCPU with relevant VCs */

          for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
            {
            tmpVC = (mvc_t *)tmpL->Ptr;

            if (tmpVC == NULL)
              continue;
       
            Ptr = NULL;
 
            if (MVCGetDynamicAttr(tmpVC,mvcaThrottlePolicies,&Ptr,mdfOther) == SUCCESS)
              {
              VCPU[VCCount++] = &((mcredl_t *)Ptr)->ActivePolicy;
              }
            }
          }  /* END if (VCPU != NULL) */
 
        break;
      }  /* END switch (PolicyType) */
    }    /* END if (PU == NULL) */
  else
    {
    tPU[mxoNONE][1] = &PU[OIndex];
    }  /* END else (PU == NULL) */

  if (ViolationDetected != NULL)
    {
    /* NOTE:  Violation detected set only if limit is set AND resources are negative */

    *ViolationDetected = FALSE;
    }

  /* adjust consumption state */

  /* NOTE:  THIS LOOP IS DUPLICATED BELOW FOR FAIRSHARE TREE LIMITS, JOBTEMPLATES, AND VCS */

  for (oindex1 = 0;OList1[oindex1] != mxoALL;oindex1++)
    {
    for (oindex2 = 0;OList2[oindex2] != mxoALL;oindex2++)
      {
      P = tPU[OList1[oindex1]][OList2[oindex2]];

      if (P == NULL)
        {
        continue;
        }

      __MPolicyAdjustUsage(J,P,&PConsumed,Count,PIndex,OList1[oindex1],OList2[oindex2],ViolationDetected,EMsg);
      }    /* END for (oindex2) */
    }      /* END for (oindex1) */

  /* adjust job templates */

  if ((J != NULL) && (J->TStats != NULL))
    {
    for (tindex = MSched.FSTreeDepth;tindex < MSched.FSTreeDepth + MMAX_TJOB;tindex++)
      {
      P = fstPU[tindex];
   
      if (P == NULL)
        break;
   
      __MPolicyAdjustUsage(J,P,&PConsumed,Count,PIndex,mxoNONE,mxoNONE,ViolationDetected,EMsg);
      }    /* END for (tindex) */
    }      /* END if ((J != NULL) && (J->TStats != NULL)) */

  /* adjust fairshare tree */

  for (tindex = 0;tindex < MSched.FSTreeDepth;tindex++)
    {
    P = fstPU[tindex];
 
    if (P == NULL)
      {
      break;
      }
 
    __MPolicyAdjustUsage(J,P,&PConsumed,Count,PIndex,mxoNONE,mxoNONE,ViolationDetected,EMsg);
    }    /* END for (tindex) */

  /* adjust VCs */

  if ((J != NULL) && (VCPU != NULL))
    {
    for (VCCount = 0;VCPU[VCCount] != NULL;VCCount++)
      {
      P = VCPU[VCCount];
   
      if (P == NULL)
        break;
   
      __MPolicyAdjustUsage(J,P,&PConsumed,Count,PIndex,mxoNONE,mxoNONE,ViolationDetected,EMsg);
      }    /* END for (tindex) */
    }      /* END if ((J != NULL) && (J->TStats != NULL)) */

  MSNLFree(&PConsumed.GRes);

  return(SUCCESS);
  }  /* END MPolicyAdjustUsage() */



/* END MPolicy.c */
