/* HEADER */

      
/**
 * @file MSchedOConfigShow.c
 *
 * Contains: MSchedOConfigShow
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report config settings for old style scheduler parameters.
 *
 * @see MSchedConfigShow() - peer - show new-style SCHEDCFG[] values
 * @see MSchedProcessOConfig() - peer
 *
 * @param PIndex (I)
 * @param VFlag (I)
 * @param String (O)
 */

int MSchedOConfigShow(

  int        PIndex,
  int        VFlag,
  mstring_t *String)

  {
  msched_t *S;

  int       index;
  int       aindex;

  char  tmpBuf[MMAX_LINE];
  char  TString[MMAX_LINE];

  const char *FName = "MSchedOConfigShow";

  MDB(4,fSCHED) MLog("%s(PIndex,VFlag,String)\n",
    FName);

  if (String == NULL)
    {
    return(FAILURE);
    }

  S = &MSched;

  MStringSet(String,"\0");

  /* display logging config */

  if (S->LogFile[0] == '\0')
    {
    MStringAppendF(String,"# NO LOGFILE SPECIFIED\n");
    }

  MStringAppendF(String,"%s",
    MUShowString(MParam[mcoLogFile],S->LogFile));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoLogFileMaxSize],S->LogFileMaxSize));

  if (VFlag || (S->LogFileRollDepth != MDEF_LOGFILEROLLDEPTH))
    MStringAppendF(String,"%s",
      MUShowInt(MParam[mcoLogFileRollDepth],S->LogFileRollDepth));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoLogLevel],mlog.Threshold));

  MStringAppendF(String,"%s",
    MUShowOctal(MParam[mcoLogPermissions],S->LogPermissions));

  if (VFlag || (MSched.LogLevelOverride == TRUE))
    {
    MStringAppendF(String,"%s",
      MUShowString(MParam[mcoLogLevelOverride],MBool[MSched.LogLevelOverride]));
    }

  if (MSched.JobStartThreshold > 0)
    {
    MStringAppendF(String,"%s",
     MUShowInt(MParam[mcoJobStartLogLevel],MSched.JobStartThreshold));
    }

  if ((!bmisset(&mlog.FacilityBM,fALL)) ||
      (VFlag ||
      (PIndex == -1) ||
      (PIndex == mcoLogFacility)))
    {
    mstring_t Line(MMAX_LINE);

    if (bmisset(&mlog.FacilityBM,fALL))
      MStringSet(&Line,MLogFacilityType[fALL]);
    else
      bmtostring(&mlog.FacilityBM,MLogFacilityType,&Line);

    MStringAppend(String,
      MUShowString(MParam[mcoLogFacility],Line.c_str()));
    }

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoEventFileRollDepth],MStat.EventFileRollDepth));

  if (MSched.MinRMPollInterval > 0)
    {
    char tmpLine[MMAX_LINE];

    MULToTString(S->MinRMPollInterval,TString);
    strcpy(tmpLine,TString);

    strcat(tmpLine,",");

    MULToTString(S->MaxRMPollInterval,TString);
    strcat(tmpLine,TString);

    MStringAppend(String,
      MUShowString(MParam[mcoRMPollInterval],tmpLine));
    }
  else
    {
    MULToTString(S->MaxRMPollInterval,TString);

    MStringAppend(String,
      MUShowString(MParam[mcoRMPollInterval],TString)); 
    }

  MULToTString(S->HAPollInterval,TString);

  MStringAppend(String,
    MUShowString(MParam[mcoHAPollInterval],TString));

  if ((VFlag || (PIndex == -1) || (PIndex == mcoRMRetryTimeCap)))
    {
    MULToTString(S->RMRetryTimeCap,TString);

    MStringAppend(String,
      MUShowString(MParam[mcoRMRetryTimeCap],TString));
    }

  MStringAppend(String,
    MUShowString(MParam[mcoRMMsgIgnore],MBool[S->RMMsgIgnore]));

  if (S->SpecEMsg[memInvalidFSTree] != NULL)
    {
     MStringAppend(String,
      MUShowString(
        MParam[mcoInvalidFSTreeMsg],
        S->SpecEMsg[memInvalidFSTree]));
    }

  if ((MSched.EnableVPCPreemption == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoEnableVPCPreemption)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoEnableVPCPreemption],
        MBool[S->EnableVPCPreemption]));
    }

  if ((MSched.AllowRootJobs == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoAllowRootJobs)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoAllowRootJobs],
        MBool[S->AllowRootJobs]));
    }

  if ((MSched.AllowVMMigration == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoAllowVMMigration)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoAllowVMMigration],
        MBool[S->AllowVMMigration]));
    }

  if ((MSched.CheckSuspendedJobPriority == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoCheckSuspendedJobPriority)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoCheckSuspendedJobPriority],
        MBool[S->CheckSuspendedJobPriority]));
    }

  if ((MSched.AssignVLANFeatures == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoAssignVLANFeatures)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoAssignVLANFeatures],
        MBool[S->AssignVLANFeatures]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoVMCreateThrottle))
    {
    MStringAppend(String,
      MUShowInt(
        MParam[mcoVMCreateThrottle],
        S->VMCreateThrottle));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoVMMigrateThrottle))
    {
    MStringAppend(String,
      MUShowInt(
        MParam[mcoVMMigrateThrottle],
        S->VMMigrateThrottle));
    }

  if ((MSched.EnableVMSwap == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoEnableVMSwap)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoEnableVMSwap],
        MBool[S->EnableVMSwap]));
    }

  if ((S->NodeIdlePowerThreshold > 0) || VFlag)
    {
    MULToTString(S->NodeIdlePowerThreshold,TString);

    MStringAppend(String,
      MUShowString(
        MParam[mcoNodeIdlePowerThreshold],
        TString));    
    }

  if ((MSched.AlwaysApplyQOSLimitOverride == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoAlwaysApplyQOSLimitOverride)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoAlwaysApplyQOSLimitOverride],
        MBool[S->AlwaysApplyQOSLimitOverride]));
    }

  if ((MSched.AlwaysEvaluateAllJobs == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoAlwaysEvaluateAllJobs)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoAlwaysEvaluateAllJobs],
        MBool[S->AlwaysEvaluateAllJobs]));
    }

  if (MSched.UMask != 0)
    {
    char tmpLine[MMAX_LINE];

    sprintf(tmpLine,"%o",
      MSched.UMask);

    MStringAppendF(String,"%s",
      MUShowString(MParam[mcoUMask],tmpLine));
    }

  if (!bmisclear(&MSched.TrigPolicyFlags))
    {
    char tmpLine[MMAX_LINE];

    MStringAppend(String,
      MUShowString(
        MParam[mcoTrigFlags],
        bmtostring(&S->TrigPolicyFlags,MTrigPolicyFlag,tmpLine)));
    }

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoTrigCheckTime],S->MaxTrigCheckSingleTime));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoTrigEvalLimit],S->TrigEvalLimit));

  /* socket attributes */

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoSocketExplicitNonBlock],
      MBool[S->SocketExplicitNonBlock]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoSocketLingerVal],S->SocketLingerVal));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoSocketWaitTime],S->SocketWaitTime));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoNonBlockingCacheInterval],MSched.NonBlockingCacheInterval));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoEnableCPJournal],
      MBool[MCP.UseCPJournal]));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoRemoveTrigOutputFiles],
      MBool[MSched.RemoveTrigOutputFiles]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoMaxDepends],MSched.M[mxoxDepends]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoClientMaxConnections],MSched.ClientMaxConnections));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoDontEnforcePeerJobLimits],
      MBool[MSched.DontEnforcePeerJobLimits]));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoDisplayProxyUserAsUser],
      MBool[MSched.DisplayProxyUserAsUser]));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoCalculateLoadByVMSum],
      MBool[MSched.VMCalculateLoadByVMSum]));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoMigrateToZeroLoadNodes],
      MBool[MSched.MigrateToZeroLoadNodes]));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoForceTrigJobsIdle],
      MBool[MSched.ForceTrigJobsIdle]));
 
  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoEnableHighThroughput],
      MBool[MSched.EnableHighThroughput]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoMaxNode],MSched.M[mxoNode]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoMaxJob],MSched.M[mxoJob]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoMaxGMetric],MSched.M[mxoxGMetric]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoMaxGRes],MSched.M[mxoxGRes]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoJobActionOnNodeFailure],MSched.ARFPolicy));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoARFDuration],MSched.ARFDuration));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoEnableFailureForPurgedJob],
      MBool[MSched.EnableFailureForPurgedJob]));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoDisableUICompression],
      MBool[MSched.DisableUICompression]));
  
  MStringAppendF(String,"%s",
    MUShowString(MParam[mcoRsvSearchAlgo],(char *)MRsvSearchAlgo[MSched.RsvSearchAlgo]));

  MStringAppendF(String,"%s",
    MUShowString(
      MParam[mcoLoadAllJobCP],
      MBool[MSched.LoadAllJobCP]));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoMountPointCheckInterval],MSched.MountPointCheckInterval));

  MStringAppendF(String,"%s",
    MUShowDouble(MParam[mcoMountPointCheckThreshold],MSched.MountPointUsageThreshold));

  MStringAppendF(String,"%s",
    MUShowInt(MParam[mcoNonBlockingCacheInterval],MSched.NonBlockingCacheInterval));

  if ((S->WCViolAction != mwcvaNONE) || VFlag)
    {
    MStringAppendF(String,"%s",
      MUShowString(MParam[mcoWCViolAction],(char *)MWCVAction[S->WCViolAction]));
    }

  /* Profile attributes are partition config */

  /*
  if (S->ProfileDuration > 0)
    {
    MULToTString(S->ProfileDuration,TString);

    MStringAppend(String,
      MUShowString(
        MParam[mcoStatProfDuration],
        TString));
    }

  if (S->ProfileCount > 0)
    {
    MStringAppend(String,
      MUShowInt(MParam[mcoStatProfCount],S->ProfileCount));
    }
  */

  /* display scheduling policies */

  if ((S->RMJobAggregationTime > 0) || VFlag)
    {
    MULToTString(S->RMJobAggregationTime,TString);

    MStringAppend(String,
      MUShowString(
        MParam[mcoJobAggregationTime],
        TString));
    }

  if ((S->AdminEventAggregationTime > 0) || VFlag)
    {
    MULToTString(S->AdminEventAggregationTime,TString);

    MStringAppend(String,
      MUShowString(
        MParam[mcoAdminEventAggregationTime],
        TString));
    }

  if ((S->ARFDuration > 0) || VFlag)
    {
    MULToTString(S->ARFDuration,TString);

    MStringAppend(String,
      MUShowString(
        MParam[mcoARFDuration],
        TString));
    }

  if ((!bmisclear(&S->JobRejectPolicyBM)) || VFlag)
    {
    char tmpLine[MMAX_LINE];

    bmtostring(&S->JobRejectPolicyBM,MJobRejectPolicy,tmpLine," ");

    MStringAppend(String,
      MUShowString(
        MParam[mcoJobRejectPolicy],
        tmpLine));
    }

  if (VFlag || (PIndex == -1) || (MSched.JobRemoveEnvVarList != NULL))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoJobRemoveEnvVarList],MSched.JobRemoveEnvVarList));
    }

  if (VFlag || (PIndex == -1) || (MSched.SubmitEnvFileLocation != NULL))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoSubmitEnvFileLocation],MSched.SubmitEnvFileLocation));
    }

  if ((S->ARFPolicy != marfNONE) || VFlag)
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoAllocResFailPolicy],
        (char *)MARFPolicy[S->ARFPolicy]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoNAPolicy))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoNAPolicy],
        (char *)MNAccessPolicy[MSched.DefaultNAccessPolicy]));
    }

  if (VFlag || (PIndex == -1) || (MSched.AccountingInterfaceURL != NULL))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoAccountingInterfaceURL],
        (char *)MSched.AccountingInterfaceURL));
    }

  if (VFlag || (PIndex == -1) || (MSched.EnableHPAffinityScheduling == TRUE))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoEnableHPAffinityScheduling],
        (char *)MBool[MSched.EnableHPAffinityScheduling]));
    }

  if (VFlag || (PIndex == -1) || (MSched.FullJobTrigCP == TRUE))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoFullJobTrigCP],
        (char *)MBool[MSched.FullJobTrigCP]));
    }

  if (VFlag || (PIndex == -1) || (MSched.DontCancelInteractiveHJobs == TRUE))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoDontCancelInteractiveHJobs],
        (char *)MBool[MSched.DontCancelInteractiveHJobs]));
    }

  if (VFlag || (PIndex == -1) || (MSched.DefaultRsvAllocPolicy != mralpNONE))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoRsvReallocPolicy],
        (char *)MRsvAllocPolicy[MSched.DefaultRsvAllocPolicy]));
    }

  for (aindex = 1;aindex <= MMAX_ADMINTYPE;aindex++)
    {
    MStringAppendF(String,"%-30s  %s",
      MParam[mcoAdmin1Users + aindex - 1],
      S->Admin[1].UName[0]);

    /* This block prints out the first user of ADMIN1 for each ADMIN level,
     * but the ADMIN1 first user is the first user for all ADMIN levels.
     * We set aindex to 1 avoid duplication of ADMIN1's firstuser in showconfig output
     * otherwise we print out all ADMIN level users starting at index 0 */

    if (aindex == 1)
      {
      index = 1;
      }
    else
      {
      index = 0;
      }

    for (;S->Admin[aindex].UName[index][0] != '\0';index++)
      {
      MStringAppendF(String," %s",
        S->Admin[aindex].UName[index]);
      }  /* END for (index) */

    MStringAppend(String,"\n");
    }  /* END for (aindex) */

  if ((S->SubmitHosts[0][0] != '\0') || 
      (VFlag || 
      (PIndex == -1) || 
      (PIndex == mcoSubmitHosts)))
    {
    MStringAppendF(String,"%-30s  %s",
      MParam[mcoSubmitHosts],S->SubmitHosts[0]);

    for (index = 1;index < MMAX_SUBMITHOSTS;index++)
      {
      if (S->SubmitHosts[index][0] == '\0')
        break;

      MStringAppendF(String," %s",
        S->SubmitHosts[index]);
      }  /* END for (index) */

    MStringAppend(String,"\n");
    }

  if (VFlag || 
     (PIndex == -1) || 
     (PIndex == mcoNodePollFrequency) || 
     (S->NodePollFrequency > 0))
    {
    MStringAppend(String,
      MUShowInt(MParam[mcoNodePollFrequency],S->NodePollFrequency));
    }

  if ((PIndex == mcoDisplayFlags) || 
      (!bmisclear(&S->DisplayFlags)) || (VFlag || (PIndex == -1)))
    {
    char tmpLine[MMAX_LINE];

    tmpLine[0] = '\0';

    for (index = 0;MCDisplayType[index] != NULL;index++)
      {
      if (bmisset(&S->DisplayFlags,index))
        {
        if (tmpLine[0] != '\0')
          strcat(tmpLine," ");

        strcat(tmpLine,MCDisplayType[index]);
        }
      }

    MStringAppend(String,
      MUShowString(MParam[mcoDisplayFlags],tmpLine));
    }  /* END if ((PIndex == mcoDisplayFlags) || ...) */

  if (VFlag || (PIndex == -1) || (PIndex == mcoNodeCatCredList))
    {
    char *tBPtr;
    int   tBSpace;

    char *t2BPtr;
    int   t2BSpace;

    int index1;
    int index2;
    
    char tmpLine[MMAX_LINE];
    char tmpLine2[MMAX_LINE];

    MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

    for (index1 = 1;index1 < mncLAST;index1++)
      { 
      if (MSched.NodeCatGroup[index1] == NULL)
        continue;

      MUSNInit(&t2BPtr,&t2BSpace,tmpLine2,sizeof(tmpLine2));

      for (index2 = 1;index2 < mncLAST;index2++)
        {
        if (MSched.NodeCatGroupIndex[index2] == index1)
          {
          MUSNPrintF(&t2BPtr,&t2BSpace,"%s%s",
            (tmpLine2[0] != '\0') ? "," : "",
            MNodeCat[index2]);
          }
        }

      MUSNPrintF(&tBPtr,&tBSpace,"%s=%s ",
        MSched.NodeCatGroup[index1],
        tmpLine2);
      }

    MStringAppend(String,
      MUShowString(
        MParam[mcoNodeCatCredList],
        tmpLine));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoExtendedJobTracking))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoExtendedJobTracking],
        (char *)MBool[MSched.ExtendedJobTracking]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoEnableJobArrays))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoEnableJobArrays],
        (char *)MBool[MSched.EnableJobArrays]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoArrayJobParLock))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoArrayJobParLock],
        (char *)MBool[MSched.ArrayJobParLock]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoOptimizedJobArrays))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoOptimizedJobArrays],
        (char *)MBool[MSched.OptimizedJobArrays]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoDataStageHoldType))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoDataStageHoldType],
        (char *)MHoldType[MSched.DataStageHoldType]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoEnableEncryption))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoEnableEncryption],
        (char *)MBool[MSched.EnableEncryption]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoNodeSetPlus))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoNodeSetPlus],
        (char *)MNodeSetPlusPolicy[MSched.NodeSetPlus]));
    }

  if ((S->MDiagXmlJobNotes == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoMDiagXmlJobNotes)))
     {
     MStringAppend(String,
       MUShowString(
         MParam[mcoMDiagXmlJobNotes],
         (char *)MBool[MSched.MDiagXmlJobNotes]));
     }

/* replaced by MSched.State
  if (VFlag || (PIndex == -1) || (PIndex == mcoDisableScheduling))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoDisableScheduling],
        (char *)MBool[MSched.DisableScheduling]));
    }
*/

  for (index = 0;index < MMAX_QOSBUCKET;index++)
    {
    if (MPar[0].QOSBucket[index].List[0] != '\0')
      {
      char tmpLine[MMAX_LINE];
      char BLine[MMAX_LINE];
      int  vindex;

      sprintf(tmpLine,"%d",
        MPar[0].QOSBucket[index].RsvDepth);

      MUShowSSArray(
        MParam[mcoRsvBucketDepth],
        MPar[0].QOSBucket[index].Name,
        tmpLine,
        BLine,
        sizeof(BLine));

      MStringAppend(String,BLine);

      tmpLine[0] = '\0';

      if (MPar[0].QOSBucket[index].List[0] == (mqos_t *)MMAX_QOS)
        {
        strcpy(tmpLine,"ALL");
        }
      else
        {
        for (vindex = 0;MPar[0].QOSBucket[index].List[vindex] != NULL;vindex++)
          {
          if (vindex != 0)
            {
            strcat(tmpLine,",");
            }

          strncat(tmpLine,
            MPar[0].QOSBucket[index].List[vindex]->Name,
            sizeof(tmpLine) - strlen(tmpLine));
          }
        }

      MUShowSSArray(
        MParam[mcoRsvBucketQOSList],
        MPar[0].QOSBucket[index].Name,
        tmpLine,
        BLine,
        sizeof(BLine));

      MStringAppend(String,BLine);
      }  /* END if (MPar[0].QOSBucket[index].List[0] != '\0') */
    }    /* END for (index) */

  if (S->QOSDefaultOrder[0] != NULL) 
    {
    int        qindex;
    mqos_t    *tmpQOS = NULL;
    mstring_t  qosList(MMAX_LINE);

    for (qindex = 0;qindex < MMAX_QOS;qindex++)
      {
      tmpQOS = S->QOSDefaultOrder[qindex];

      if (tmpQOS == NULL)
        break;

      MStringAppendF(&qosList,"%s%s",
          (qindex > 0) ? "," : "", 
          tmpQOS->Name);
      }

    MStringAppend(String,
      MUShowString(
        MParam[mcoQOSDefaultOrder],
        qosList.c_str()));
    }

  if ((S->DeadlinePolicy != 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoDeadlinePolicy)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoDeadlinePolicy],
        (char *)MDeadlinePolicy[S->DeadlinePolicy]));
    }

  if ((S->MissingDependencyAction != 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoMissingDependencyAction)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoMissingDependencyAction],
        (char *)MMissingDependencyAction[S->MissingDependencyAction]));
    }

  if (!bmisclear(&S->RecordEventList) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoRecordEventList)))
    {
    mstring_t Line(MMAX_LINE);

    bmtostring(&S->RecordEventList,MRecordEventType,&Line);

    MStringAppend(String,
      MUShowString(
        MParam[mcoRecordEventList],
        Line.c_str()));
    }

  if ((S->StoreSubmission != FALSE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoStoreSubmission)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoStoreSubmission],
      MBool[S->StoreSubmission]);
    }

  if ((S->TrackSuspendedJobUsage != FALSE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoTrackSuspendedJobUsage)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoTrackSuspendedJobUsage],
      MBool[S->TrackSuspendedJobUsage]);
    }

  if ((S->DefaultDomain[0] != '\0') || 
      (VFlag || (PIndex == -1) || (PIndex == mcoDefaultDomain)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDefaultDomain],S->DefaultDomain));
    }

  if ((S->DefaultJobOS != 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoDefaultJobOS)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDefaultJobOS],MAList[meOpsys][S->DefaultJobOS]));
    }

  if ((S->DefaultQMHost[0] != '\0') ||
      (VFlag || (PIndex == -1) || (PIndex == mcoDefaultQMHost)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDefaultQMHost],S->DefaultQMHost));
    }

  /* NOTE:  enforce buffer size managment (NYI below) */

  if (!strcmp(S->DefaultClassList,MDEF_CLASSLIST) || 
     (VFlag || (PIndex == -1) || (PIndex == mcoDefaultClassList)))
    {
    MStringAppend(String,
     MUShowString(MParam[mcoDefaultClassList],S->DefaultClassList));
    }

  if (!bmisclear(&S->RealTimeDBObjects))
    {
    char tmpLine[MMAX_LINE];

    bmtostring(&S->RealTimeDBObjects,MXO,tmpLine);

    MStringAppend(String,MUShowString(MParam[mcoRealTimeDBObjects],tmpLine));
    }

  if ((S->RemapC != NULL) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoRemapClass)))
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoRemapClass],
        (S->RemapC != NULL) ? S->RemapC->Name : (char *)""));
    }

  if ((S->RemapCList[0] != NULL) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoRemapClassList)))
    {
    int RclIndex;
    mstring_t ListBuf(MMAX_LINE);

    if(S->RemapCList[0] != NULL)
      {
      MStringAppend(&ListBuf,S->RemapCList[0]->Name);
      for(RclIndex = 1; S->RemapCList[RclIndex] != NULL; RclIndex++)
        {
        MStringAppend(&ListBuf,",");
        MStringAppend(&ListBuf,S->RemapCList[RclIndex]->Name);
        }
      }

    MStringAppend(String,
      MUShowString(
        MParam[mcoRemapClassList],
        ListBuf.c_str()));
    }

  if ((S->MaxJobPreemptPerIteration > 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoMaxJobPreemptPerIteration)))
    {
    MStringAppend(String,
      MUShowInt(
        MParam[mcoMaxJobPreemptPerIteration],
        S->MaxJobPreemptPerIteration));
    }

  if ((S->AuthTimeout != MDEF_AUTHTIMEOUT) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoAuthTimeout)))
    {
    MStringAppend(String,
      MUShowInt(
        MParam[mcoAuthTimeout],
        S->AuthTimeout));
    }

  if ((S->MaxJobHoldTime > 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoMaxJobHoldTime)))
    {
    MULToTString(S->MaxJobHoldTime,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoMaxJobHoldTime],
      TString);
    }

  if ((S->RsvTimeDepth > 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoRsvTimeDepth)))
    {
    MULToTString(S->RsvTimeDepth,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoMaxJobHoldTime],
      TString);
    }

  if ((S->MaxJobStartPerIteration > 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoMaxJobStartPerIteration)))
    {
    MStringAppend(String,
      MUShowInt(
        MParam[mcoMaxJobStartPerIteration],
        S->MaxJobStartPerIteration));
    }

  if ((S->MapFeatureToPartition == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoMapFeatureToPartition)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoMapFeatureToPartition],MBool[S->MapFeatureToPartition]));
    }

  if ((S->RejectDosScripts == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoRejectDosScripts)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoRejectDosScripts],MBool[S->RejectDosScripts]));
    }

  if ((S->NodeTypeFeatureHeader[0] != '\0') || 
      (VFlag || (PIndex == -1) || (PIndex == mcoNodeTypeFeatureHeader)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoNodeTypeFeatureHeader],S->NodeTypeFeatureHeader));
    }


  if ((MGEventGetDescCount() > 0) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoGEventCfg)))

    {
    char BM[MMAX_LINE];
    char tmpLine[MMAX_LINE];
    char BLine[MMAX_LINE];

    char                  *GEName;
    mgevent_desc_t        *GEventDesc;
    mgevent_iter_t         GEIter;

    MGEventIterInit(&GEIter);

    while (MGEventDescIterate(&GEName,&GEventDesc,&GEIter) == SUCCESS)
      {
      char emptyString[] = {0};
      char *str;

      if (bmisclear(&GEventDesc->GEventAction))
        continue;

      MULToTString(GEventDesc->GEventReArm,TString);

      str =  bmtostring(&GEventDesc->GEventAction,MGEAction,BM);
      if (NULL == str)
        str = emptyString;

      sprintf(tmpLine,"event=%s rearm=%s ecount=%d\n",
        str,
        TString,
        GEventDesc->GEventCount);

      MUShowSSArray(
        MParam[mcoGEventCfg],
        GEName,
        tmpLine,
        BLine,
        sizeof(BLine));

      MStringAppend(String,BLine);
      } /* END while (MGeventDescIterate(... )) == SUCCESS) */
    }    /* END if ((GEventDescCount() > 0) || ...) */

  if ((!bmisclear(&MGMetric.GMetricAction[0])) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoGEventCfg)))
    {
    int gindex;

    char tmpLine[MMAX_LINE];
    char BLine[MMAX_LINE];
    char ILine[MMAX_LINE];

    for (gindex = 0;gindex < MSched.M[mxoxGMetric];gindex++)
      {
      if (bmisclear(&MGMetric.GMetricAction[gindex]))
        continue;

      MULToTString(MGMetric.GMetricReArm[gindex],TString);

      sprintf(tmpLine,"event=%s rearm=%s ecount=%d\n",
        bmtostring(&MGMetric.GMetricAction[gindex],MGEAction,BLine),
        TString,
        MGMetric.GMetricCount[gindex]);

      snprintf(ILine,sizeof(ILine),"%s%s%f",
        MGMetric.Name[gindex],
        MComp[MGMetric.GMetricCmp[gindex]],
        MGMetric.GMetricThreshold[gindex]);

      MUShowSSArray(
        MParam[mcoGEventCfg],
        ILine,
        tmpLine,
        BLine,
        sizeof(BLine));

      MStringAppend(String,BLine);
      }  /* END for (gindex) */
    }    /* END if ((S->GMetricAction[0] != 0) || ...) */

  if ((MGMetric.GMetricChargeRate[0] != 0.0) || (MGMetric.GMetricIsCummulative[0] == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoGMetricCfg)))
    {
    int gindex;

    char tmpLine[MMAX_LINE];
    char BLine[MMAX_LINE];

    char *tBPtr;
    int   tBSpace;

    for (gindex = 1;gindex < MSched.M[mxoxGMetric];gindex++)
      {
      if (MGMetric.GMetricChargeRate[gindex] == 0.0)
        continue;

      MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

      if (MGMetric.GMetricChargeRate[gindex] != 0.0)
        {
        MUSNPrintF(&tBPtr,&tBSpace," chargerate=%.2f",
          MGMetric.GMetricChargeRate[gindex]);
        }

      if (MGMetric.GMetricIsCummulative[gindex] == TRUE)
        {
        MUSNPrintF(&tBPtr,&tBSpace," iscummulative=%s",
          MBool[MGMetric.GMetricIsCummulative[gindex]]);
        }

      if (tmpLine[0] == '\0')
        continue;

      MUShowSSArray(
        MParam[mcoGMetricCfg],
        MGMetric.Name[gindex],
        tmpLine,
        BLine,    /* O */
        sizeof(BLine));

      MStringAppend(String,BLine);
      }  /* END for (gindex) */
    }    /* END if ((S->GMetricChargeRate[0] != 0.0) || ...) */

  if ((S->ProcSpeedFeatureHeader[0] != '\0') || 
      (VFlag || (PIndex == -1) || (PIndex == mcoProcSpeedFeatureHeader)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoProcSpeedFeatureHeader],
      S->ProcSpeedFeatureHeader));
    }

  if ((S->PartitionFeatureHeader[0] != '\0') || 
      (VFlag || (PIndex == -1) || (PIndex == mcoPartitionFeatureHeader)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoPartitionFeatureHeader],
      S->PartitionFeatureHeader));
    }

  if ((S->RackFeatureHeader[0] != '\0') ||
      (VFlag || (PIndex == -1) || (PIndex == mcoRackFeatureHeader)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoRackFeatureHeader],
      S->RackFeatureHeader));
    }

  if ((S->SlotFeatureHeader[0] != '\0') ||
      (VFlag || (PIndex == -1) || (PIndex == mcoSlotFeatureHeader)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoSlotFeatureHeader],
      S->SlotFeatureHeader));
    }

  if ((S->OSCredLookup != MDEF_OSCREDLOOKUP) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoOSCredLookup)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoOSCredLookup],
      (char *)MActionPolicy[S->OSCredLookup]));
    }

  if ((S->FSTreeACLPolicy != mfsapOff) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoFSTreeACLPolicy)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoFSTreeACLPolicy],
      (char *)MFSTreeACLPolicy[S->FSTreeACLPolicy]));
    }

  if ((S->FSTreeUserIsRequired == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoFSTreeUserIsRequired)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoFSTreeUserIsRequired],
      (char *)MBool[S->FSTreeUserIsRequired]));
    }

  if ((S->UseMoabJobID == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoUseMoabJobID)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoUseMoabJobID],
      (char *)MBool[S->UseMoabJobID]));
    }

  if ((S->FreeTimeLookAheadDuration != MDEF_FREETIMELOOKAHEADDURATION) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoFreeTimeLookAheadDuration)))
    {
    MULToTString(S->FreeTimeLookAheadDuration,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoFreeTimeLookAheadDuration],
      TString);
    }

  if ((S->FSRelativeUsage == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoFSRelativeUsage)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoFSRelativeUsage],
      (char *)MBool[S->FSRelativeUsage]));
    }

  if ((S->UseAnyPartitionPrio == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoUseAnyPartitionPrio)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoUseAnyPartitionPrio],
      (char *)MBool[S->UseAnyPartitionPrio]));
    }

  if ((S->UseCPRsvNodeList == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoUseCPRsvNodeList)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoUseCPRsvNodeList],
      (char *)MBool[S->UseCPRsvNodeList]));
    }

  if ((S->UseJobRegEx == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoUseJobRegEx)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoUseJobRegEx],
      (char *)MBool[S->UseJobRegEx]));
    } 

  if ((S->DisableBatchHolds == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoDisableBatchHolds)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDisableBatchHolds],
      (char *)MBool[S->DisableBatchHolds]));
    }

  if ((S->DisableExcHList == TRUE) || 
      (VFlag || (PIndex == -1) || (PIndex == mcoDisableExcHList)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDisableExcHList],
      (char *)MBool[S->DisableExcHList]));
    }

  if ((S->DisableInteractiveJobs == TRUE) || VFlag)
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDisableInteractiveJobs],
      (char *)MBool[S->DisableInteractiveJobs]));
    }

  if ((S->DisableRegExCaching == TRUE) || VFlag)
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDisableRegExCaching],
      (char *)MBool[S->DisableRegExCaching]));
    }

 if ((S->IgnoreRMDataStaging == TRUE) || VFlag)
    {
    MStringAppend(String,
      MUShowString(MParam[mcoIgnoreRMDataStaging],
      (char *)MBool[S->IgnoreRMDataStaging]));
    }

  if ((!bmisclear(&S->DisableSameCredPreemptionBM)) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoDisableSameCredPreemption)))
    {
    char tmpLine[MMAX_LINE];

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoDisableSameCredPreemption],
      bmtostring(&S->DisableSameCredPreemptionBM,MAttrO,tmpLine));
    }

  if ((S->DisableUICompression == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoDisableUICompression)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoDisableUICompression],
      (char *)MBool[S->DisableUICompression]));
    }

  if ((S->EnableFSViolationPreemption != FALSE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoEnableFSViolationPreemption)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoEnableFSViolationPreemption],
      MBool[S->EnableFSViolationPreemption]);
    }

  if ((S->EnableSPViolationPreemption != FALSE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoEnableSPViolationPreemption)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoEnableSPViolationPreemption],
      MBool[S->EnableSPViolationPreemption]);
    }

  if ((S->BGPreemption != FALSE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoBGPreemption)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoBGPreemption],
      MBool[S->BGPreemption]);
    }

  if ((S->GuaranteedPreemption != FALSE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoGuaranteedPreemption)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoGuaranteedPreemption],
      MBool[S->GuaranteedPreemption]);
    }

  if ((S->SideBySide == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoSideBySide)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoSideBySide],
      (char *)MBool[S->SideBySide]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoMaxGreenStandbyPoolSize))
    {
    MStringAppend(String,
      MUShowInt(
        MParam[mcoMaxGreenStandbyPoolSize],
        S->MaxGreenStandbyPoolSize));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoAggregateNodeActionsTime))
    {
    MStringAppend(String,
      MUShowInt(MParam[mcoAggregateNodeActionsTime],
      S->AggregateNodeActionsTime));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoAggregateNodeActions))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoAggregateNodeActions],
      (char *)MBool[S->AggregateNodeActions]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoReportProfileStatsAsChild))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoReportProfileStatsAsChild],
        (char *)MBool[S->ReportProfileStatsAsChild]));
    }

  if (VFlag || (PIndex == -1) || (PIndex == mcoGreenPoolEvalInterval))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoGreenPoolEvalInterval],
      (char *)MCalPeriodType[S->GreenPoolEvalInterval]));
    }

  if ((S->FSTreeIsRequired == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoFSTreeIsRequired)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoFSTreeIsRequired],
      (char *)MBool[S->FSTreeIsRequired]));
    }

  if ((S->FSMostSpecificLimit == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoFSMostSpecificLimit)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoFSMostSpecificLimit],
      (char *)MBool[S->FSMostSpecificLimit]));
    }

  /* general sched config */

  MStringAppendF(String,"%-30s  %d,%d\n",
    MParam[mcoJobMaxNodeCount],
    MSched.JobDefNodeCount,
    MSched.JobMaxNodeCount);

  MStringAppendF(String,"%-30s  %d\n",
    MParam[mcoJobMaxTaskCount],
    MSched.JobMaxTaskCount - 1);

  if (S->PreemptRTimeWeight != 0.0)
    {
    MStringAppendF(String,"%s",
      MUShowDouble(MParam[mcoPreemptRTimeWeight],S->PreemptRTimeWeight));
    }

  if (S->NodeFailureReserveTime > 0)
    {
    MULToTString(S->NodeFailureReserveTime,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoNodeFailureReserveTime],
      TString);
    }

  if (MPar[0].NodeBusyStateDelayTime > 0)
    {
    MULToTString(MPar[0].NodeBusyStateDelayTime,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoNodeBusyStateDelayTime],
      TString);
    }

  if (MPar[0].NodeDownStateDelayTime > 0)
    {
    MULToTString(MPar[0].NodeDownStateDelayTime,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoNodeDownStateDelayTime],
      TString);
    }

  if (MPar[0].NodeDrainStateDelayTime > 0)
    {
    MULToTString(MPar[0].NodeDrainStateDelayTime,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoNodeDrainStateDelayTime],
      TString);
    }

  if (MPar[0].UntrackedResDelayTime > 0)
    {
    MULToTString(MPar[0].UntrackedResDelayTime,TString);

    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoNodeUntrackedResDelayTime],
      TString);
    }

  if ((MPar[0].ARFPolicy != marfIgnore) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoNodeAllocResFailurePolicy)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoNodeAllocResFailurePolicy],
      (char *)MARFPolicy[MPar[0].ARFPolicy]));
    }

  if (VFlag)
    {
    MStringAppendF(String,"%-30s  %d\n",
      MParam[mcoJobFailRetryCount],
      S->JobFailRetryCount);
    }

  MULToTString(S->DeferTime,TString);

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoDeferTime],
    TString);

  MStringAppendF(String,"%-30s  %d\n",
    MParam[mcoDeferCount],
    S->DeferCount);

  MStringAppendF(String,"%-30s  %d\n",
    MParam[mcoDeferStartCount],
    S->DeferStartCount);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoDefaultTaskMigrationTime],
    S->DefaultTaskMigrationDuration);

  if ((MSched.RejectInfeasibleJobs == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoRejectInfeasibleJobs)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoRejectInfeasibleJobs],
      MBool[S->RejectInfeasibleJobs]);
    }

  if ((MSched.RejectDuplicateJobs == TRUE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoRejectDuplicateJobs)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoRejectDuplicateJobs],
      MBool[S->RejectDuplicateJobs]);
    }

  if ((MSched.JobMigratePolicy != mjmpNONE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoJobMigratePolicy)))
    {
    MStringAppend(String,
      MUShowString(MParam[mcoJobMigratePolicy],
      (char *)MJobMigratePolicy[MSched.JobMigratePolicy]));
    }

  if ((MSched.ShowMigratedJobsAsIdle != mjmpNONE) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoShowMigratedJobsAsIdle)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoShowMigratedJobsAsIdle],
      MBool[S->ShowMigratedJobsAsIdle]);
    }

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoReportPeerStartInfo],
    MBool[S->ReportPeerStartInfo]);

  MStringAppendF(String,"%-30s  %f\n",
    MParam[mcoRequeueRecoveryRatio],
    S->RequeueRecoveryRatio);

  if (MSched.IgnoreNodes.List != NULL)
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoIgnoreNodes],
      MUSListToString(MSched.IgnoreNodes.List,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.SpecNodes.List != NULL)
    {
    MStringAppendF(String,"%-30s  !%s\n",
      MParam[mcoIgnoreNodes],
      MUSListToString(MSched.SpecNodes.List,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.IgnoreClasses != NULL)
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoIgnoreClasses],
      MUSListToString(MSched.IgnoreClasses,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.SpecClasses != NULL)
    {
    MStringAppendF(String,"%-30s  !%s\n",
      MParam[mcoIgnoreClasses],
      MUSListToString(MSched.SpecClasses,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.IgnoreUsers != NULL)
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoIgnoreUsers],
      MUSListToString(MSched.IgnoreUsers,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.SpecUsers != NULL)
    {
    MStringAppendF(String,"%-30s  !%s\n",
      MParam[mcoIgnoreUsers],
      MUSListToString(MSched.SpecUsers,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.IgnoreJobs != NULL)
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoIgnoreJobs],
      MUSListToString(MSched.IgnoreJobs,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.SpecJobs != NULL)
    {
    MStringAppendF(String,"%-30s  !%s\n",
      MParam[mcoIgnoreJobs],
      MUSListToString(MSched.SpecJobs,tmpBuf,sizeof(tmpBuf)));
    }

  if (MSched.FallBackClass != NULL)
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoFallBackClass],
      MSched.FallBackClass->Name);

#if 0
  if (MSched.G.FallBackClass != NULL)
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoFallBackGridClass],
      MSched.G.FallBackClass->Name);

  if (MSched.G.FallBackQOS != NULL)
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoFallBackGridClass],
      MSched.G.FallBackQOS->Name);
    }
#endif

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoJobCPurgeTime],
    S->JobCPurgeTime);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoVMCPurgeTime],
    S->VMCPurgeTime);

  if (S->JobCTruncateNLCP == TRUE)
    {
    MStringAppend(String,
      MUShowString(
        MParam[mcoJobCTruncateNLCP],
        MBool[S->JobCTruncateNLCP]));
    }

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoJobPreemptCompletionTime],
    S->JobPreemptCompletionTime);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoMaxSuspendTime],
    S->MaxSuspendTime);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoJobPreemptMaxActiveTime],
    S->JobPreemptMaxActiveTime);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoJobPreemptMinActiveTime],
    S->JobPreemptMinActiveTime);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoJobPurgeTime],
    S->JobPurgeTime);

  MULToTString(S->GEventTimeout,TString);

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoGEventTimeout],
    TString);

  if (S->NodePurgeIteration >= 0)
    {
    MStringAppendF(String,"%-30s  %d\n",
      MParam[mcoNodePurgeTime],
      S->NodePurgeIteration);
    }
  else
    {
    MStringAppendF(String,"%-30s  %ld\n",
      MParam[mcoNodePurgeTime],
      S->NodePurgeTime);
    }

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoNodeDownTime],
    S->NodeDownTime);

  MStringAppendF(String,"%-30s  %d\n",
    MParam[mcoAPIFailureThreshold],
    S->APIFailureThreshold);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoNodeSyncDeadline],
    S->NodeSyncDeadline);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoJobSyncDeadline],
    S->JobSyncDeadline);

  if (S->JobPercentOverrun > 0.0)
    {
    MStringAppendF(String,"%-30s  %.02f%\n",
      MParam[mcoJobMaxOverrun],
      S->JobPercentOverrun * 100.0);
    }
  else
    {
    char tmpLine1[MMAX_LINE];
    char tmpLine2[MMAX_LINE];

    MULToTString(S->JobSMaxOverrun,tmpLine1);
    MULToTString(S->JobHMaxOverrun,tmpLine2);
   
    MStringAppendF(String,"%-30s  %s%s%s\n",
      MParam[mcoJobMaxOverrun],
      (S->JobSMaxOverrun > 0) ? tmpLine1 : "",
      (S->JobSMaxOverrun > 0) ? "," : "",
      tmpLine2);
    }

  if ((PIndex == mcoNodeMaxLoad) ||
     (S->DefaultN.MaxLoad > 0.0) || VFlag || (PIndex == -1))
    {
    MStringAppendF(String,"%-30s  %.1f\n",
      MParam[mcoNodeMaxLoad],
      S->DefaultN.MaxLoad);
    }

  if (S->TPSize > 0)
    {
    MStringAppendF(String,"%s \n",
      MUShowInt(MParam[mcoThreadPoolSize],S->TPSize));
    }
  else
    {
    MStringAppendF(String,"Thread Pools Disabled\n");
    }

  if ((VFlag || (PIndex == -1) || (PIndex == mcoMWikiSubmitTimeout)))
    {
    MStringAppendF(String,"%s \n",
        MUShowInt(MParam[mcoMWikiSubmitTimeout],S->MWikiSubmitTimeout));
    }

  MStringAppend(String,"\n");

  /* ALERT:  no Buffer bounds checking beyond this point */

  /* display stat graph parameters */

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatTimeMin],
    MStat.P.MinTime);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatProcMax],
    MStat.P.MaxProc);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatProcStepCount],
    MStat.P.ProcStepCount);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatProcStepSize],
    MStat.P.ProcStepSize);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatProcMin],
    MStat.P.MinProc);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatTimeMax],
    MStat.P.MaxTime);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatTimeStepCount],
    MStat.P.TimeStepCount);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoStatTimeStepSize],
    MStat.P.TimeStepSize);

  MDB(4,fUI) MLog("INFO:     plot parameters displayed\n");

  return(SUCCESS);
  }  /* END MSchedOConfigShow() */
/* END MSchedOConfigShow.c */
