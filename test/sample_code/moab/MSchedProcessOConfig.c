/* HEADER */

      
/**
 * @file MSchedProcessOConfig.c
 *
 * Contains: MSchedProcessOConfig
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  







/**
 *  Does a MIN with longs, but logs if Requested > Limit
 *
 * @param S [I]         - Schedule object (to attach message to if Requested > Limit)
 * @param PIndex [I]    - The parameter that was requested
 * @param Requested [I] - Requested value
 * @param Limit     [I] - Predefined limit
 * @param ToSet     [I] (modified) - The Value to set to the MIN(Requested,Limit)
 */

int __MSchedConfigMINLong(

  msched_t *S,                /* I */
  enum MCfgParamEnum PIndex,  /* I */
  long  Requested,            /* I */
  long  Limit,                /* I */
  long *ToSet)                /* I (modified) */

  {
  if (ToSet == NULL)
    return(FAILURE);

  if (Requested > Limit)
    {
    char tmpMsg[MMAX_LINE];

    snprintf(tmpMsg,sizeof(tmpMsg),"ERROR:    requested %s %ld is greater than precompiled limit (%ld)\n",
      MParam[PIndex],
      Requested,
      Limit);

    MDB(1,fCONFIG) MLog(tmpMsg);

    MMBAdd(
      &S->MB,
      tmpMsg,
      NULL,
      mmbtOther,
      MSched.Time + MCONST_YEARLEN,
      0,
      NULL);

    *ToSet = Limit;

    return(SUCCESS);
    }

  *ToSet = Requested;

  return(SUCCESS);
  }  /* __MSchedConfigMINLong() */


/**
 *  Parses and adds MQOSDEFAULTORDER to MSched
 *  
 *  FORMAT:  qos[,qos]...
 *  
 *  @param QosString [I]  Input QOS order string 
 */

void __MSchedParseQOSDefaultOrder(

  char *QosString)

  {
  int qindex = 0;
  char *ptr;
  char *TokPtr;

  /* FORMAT:  qos[,qos]... */

  if (QosString == NULL)
    {
    return;
    }

  /* Tokenize the string */
  ptr = MUStrTok(QosString,",",&TokPtr);
  while (ptr != NULL)
    {
    mqos_t *Q;

    /* Make sure this won't overflow the table*/
    if (qindex >= MMAX_QOS)
      {
      MDB(3,fCONFIG) MLog("ALERT:    Default QOS Order table overflow.\n");
      break;
      }

    if (MQOSFind(ptr,&Q) == SUCCESS)
      {
      MSched.QOSDefaultOrder[qindex++] = Q;
      MSched.QOSDefaultOrder[qindex] = NULL;
      }
    else if (MQOSAdd(ptr,&MSched.QOSDefaultOrder[qindex]) == SUCCESS)
      {
      MDB(5,fCONFIG) MLog("INFO:     QOS '%s' added\n",ptr);
      qindex++;
      MSched.QOSDefaultOrder[qindex] = NULL;
      }
    else
      {
      MDB(1,fCONFIG) MLog("ALERT:    cannot add QOS '%s'\n",ptr);
      return;
      }

    ptr = MUStrTok(NULL,",",&TokPtr);
    }

  }


/**
 *  Does a MIN with ints, but logs if Requested > Limit
 *
 * @param S [I]         - Schedule object (to attach message to if Requested > Limit)
 * @param PIndex [I]    - The parameter that was requested
 * @param Requested [I] - Requested value
 * @param Limit     [I] - Predefined limit
 * @param ToSet     [I] (modified) - The Value to set to the MIN(Requested,Limit)
 */

int __MSchedConfigMINInt(

  msched_t *S,                /* I */
  enum MCfgParamEnum PIndex,  /* I */
  int  Requested,             /* I */
  int  Limit,                 /* I */
  int *ToSet)                 /* I (modified) */

  {
  if (ToSet == NULL)
    return(FAILURE);

  if (Requested > Limit)
    {
    char tmpMsg[MMAX_LINE];

    snprintf(tmpMsg,sizeof(tmpMsg),"ERROR:    requested %s %d is greater than precompiled limit (%d)\n",
      MParam[PIndex],
      Requested,
      Limit);

    MDB(1,fCONFIG) MLog(tmpMsg);

    MMBAdd(
      &S->MB,
      tmpMsg,
      NULL,
      mmbtOther,
      MSched.Time + MCONST_YEARLEN,
      0,
      NULL);

    *ToSet = Limit;

    return(SUCCESS);
    }

  *ToSet = Requested;

  return(SUCCESS);
  }  /* END __MSchedConfigMINInt() */

/**
 * Process old-style scheduler parameters.
 * 
 * @see MSchedOConfigShow() - peer - report old-style config parameters
 * @see MCfgSetVal() - parent
 *
 * NOTE:  if checking the value against a predefined limit, use
 *     __MSchedConfigMINLong() or __MSchedConfigMINInt()
 *
 * @param S (I)
 * @param PIndex (I)
 * @param IVal (I)
 * @param DVal (I)
 * @param SVal (I)
 * @param SArray (I) [optional]
 * @param IndexName (I) [optional]
 */

int MSchedProcessOConfig(

  msched_t *S,               /* I */
  enum MCfgParamEnum PIndex, /* I */
  int       IVal,            /* I */
  double    DVal,            /* I */
  char     *SVal,            /* I */
  char    **SArray,          /* I (optional) */
  char     *IndexName)       /* I (optional) */

  {
  mpar_t *P;

  if (S == NULL)
    {
    return(FAILURE);
    }

  P = &MPar[0];

  switch (PIndex)
    {
    case mcoAccountingInterfaceURL:

      if ((SVal == NULL) || (SVal[0] == '\0'))
        break;

      {
      char tmpProtocol[MMAX_NAME];
      char tmpHost[MMAX_LINE];
      char tmpDir[MMAX_LINE];
      int  tmpPort;

      MUStrDup(&S->AccountingInterfaceURL,SVal);

      if (MUURLParse(
            SVal,
            tmpProtocol,
            tmpHost,
            tmpDir,
            sizeof(tmpDir),
            &tmpPort,
            TRUE,
            TRUE) == FAILURE)
        {
        /* cannot parse string */

        break;
        }

      MSched.AccountingInterfaceProtocol = 
        (enum MBaseProtocolEnum)MUGetIndexCI(tmpProtocol,MBaseProtocol,FALSE,0);

      MUStrDup(&MSched.AccountingInterfacePath,tmpDir);

      /* verify path is valid and executable */

      if (MSched.AccountingInterfaceProtocol == mbpExec)
        {
        mbool_t IsExe;

        if ((MFUGetInfo(tmpDir,NULL,NULL,&IsExe,NULL) == FAILURE) || 
            (IsExe == FALSE))
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"  WARNING:  invalid accounting interface specified (file '%s' does not exist or is not executable)\n",
            tmpDir);

          MMBAdd(
            &MSched.MB,
            tmpLine,
            NULL,
            mmbtOther,
            MSched.Time + MCONST_DAYLEN,
            0,
            NULL);
          }
        }
      }  /* END BLOCK (case mcoAccountingInterfaceURL) */

      break;

    case mcoAllowMultiReqNodeUse:

      MSched.AllowMultiReqNodeUse = MUBoolFromString(SVal,FALSE);

      break;

    case mcoVMStorageNodeThreshold:

      if ((IVal < 0) || (IVal > 100))
        {
        return(FAILURE);
        }
      
      MSched.VMStorageNodeThreshold = IVal;

      break;
      
    case mcoVMCreateThrottle: 
  
      MSched.VMCreateThrottle = IVal;

      break;

    case mcoVMGResMap:

      S->VMGResMap = MUBoolFromString(SVal,FALSE);

      break;

    case mcoVMMigrateThrottle: 
  
      MSched.VMMigrateThrottle = IVal;

      break;

    case mcoVMMigrationPolicy:

      {
      char *ptr;
      char *TokPtr;

      /* clear existing values */

      S->RunVMConsolidationMigration = FALSE;
      S->RunVMOvercommitMigration = FALSE;

      /* FORMAT:  <POLICY>[{ ,}<POLICY>]... */

      ptr = MUStrTok(SVal," ,",&TokPtr);

      while (ptr != NULL)
        {
        if (!strncasecmp(ptr,MVMMigrationPolicy[mvmmpConsolidation],strlen(MVMMigrationPolicy[mvmmpConsolidation])))
          {
          S->RunVMConsolidationMigration = TRUE;
          }
        else if (!strncasecmp(ptr,MVMMigrationPolicy[mvmmpOvercommit],strlen(MVMMigrationPolicy[mvmmpOvercommit])))
          {
          S->RunVMOvercommitMigration = TRUE;
          }
        else if (!strncasecmp(ptr,MVMMigrationPolicy[mvmmpPerformance],strlen(MVMMigrationPolicy[mvmmpPerformance])))
          {
          /* "Performance" alias for overcommit handled here */
          S->RunVMOvercommitMigration = TRUE;
          }

        ptr = MUStrTok(NULL," ,",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mcoVMMigrationPolicy) */

      break;

    case mcoVMMinOpDelay:
  
      S->MinVMOpDelay = MUTimeFromString(SVal);

      break;

    case mcoVMOvercommitThreshold:
      /* This is the grandfathered way of setting OvercommitThreshold on a global basis
       * Use it now to set default NODECFG */

      /* NOTE:  bound all values within range 0-1.0 */
      /* Set on default NODECFG */
      MUResFactorArrayFromString(
        SVal,
        S->DefaultN.ResOvercommitThreshold,
        S->DefaultN.GMetricOvercommitThreshold,
        FALSE,
        TRUE);

      break;

    case mcoVMProvisionStatusReady:

      MUStrDup(&MSched.VMProvisionStatusReady,SVal);

      break;

    case mcoVMsAreStatic:

      S->VMsAreStatic = MUBoolFromString(SVal,S->VMsAreStatic);

      break;

    case mcoVMStorageMountDir:

      MUStrDup(&MSched.VMStorageMountDir,SVal);

      break;

    case mcoVMTracking:

      S->VMTracking = MUBoolFromString(SVal,FALSE);

    case mcoAdminEventAggregationTime:

      S->AdminEventAggregationTime = MUTimeFromString(SVal);

      break;

    case mcoAggregateNodeActions:

      S->AggregateNodeActions = MUBoolFromString(SVal,FALSE);

      break;

    case mcoAggregateNodeActionsTime:

      S->AggregateNodeActionsTime = MUTimeFromString(SVal);

      break;

    case mcoAllowRootJobs:

      MSched.AllowRootJobs = MUBoolFromString(SVal,FALSE);

      break;

    case mcoAllowVMMigration:

      MSched.AllowVMMigration = MUBoolFromString(SVal,FALSE);

      break;

    case mcoAlwaysApplyQOSLimitOverride:

      MSched.AlwaysApplyQOSLimitOverride = MUBoolFromString(SVal,FALSE);

      break;

    case mcoAlwaysEvaluateAllJobs:

      MSched.AlwaysEvaluateAllJobs = MUBoolFromString(SVal,FALSE);

      break;

    case mcoArrayJobParLock:

      S->ArrayJobParLock = MUBoolFromString(SVal,FALSE);

      break;

    case mcoAssignVLANFeatures:

      MSched.AssignVLANFeatures = MUBoolFromString(SVal,FALSE);

      break;

    case mcoClientMaxConnections:

      if (IVal >= S->ClientMaxConnections)
        {
        S->ClientMaxConnections = IVal;
        }
      else
        { 
        MDB(1,fSTRUCT) MLog("WARNING:    Reducing CLIENTMAXCONNECTIONS to %d from %d not allowed during runtime. \n",
          IVal,
          S->ClientMaxConnections);

        return(FAILURE);
        }

      break;

    case mcoDontEnforcePeerJobLimits:

      MSched.DontEnforcePeerJobLimits = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisplayProxyUserAsUser:

      MSched.DisplayProxyUserAsUser = MUBoolFromString(SVal,FALSE);

      break;

    case mcoCalculateLoadByVMSum:

      MSched.VMCalculateLoadByVMSum = MUBoolFromString(SVal,FALSE);

      break;

    case mcoMigrateToZeroLoadNodes:

      MSched.MigrateToZeroLoadNodes = MUBoolFromString(SVal,FALSE);

      break;

    case mcoAuthTimeout:

      S->AuthTimeout = MUTimeFromString(SVal);

      break;

    case mcoBGPreemption:

      MSched.BGPreemption = MUBoolFromString(SVal,FALSE);

      break;

    case mcoGuaranteedPreemption:

      MSched.GuaranteedPreemption = MUBoolFromString(SVal,FALSE);

      break;

    case mcoUseDatabase:

      {
      if (MSched.MDB.DBType == mdbNONE)
        {
        MSched.ReqMDBType = (enum MDBTypeEnum)MUGetIndex(
          SVal,
          MDBType,
          FALSE,
          mdbSQLite3);
        }
      }    /* END BLOCK (case mcoUseDatabase) */

      break;

    case mcoDataStageHoldType:

      S->DataStageHoldType = (enum MHoldTypeEnum)MUGetIndexCI(SVal,MHoldType,FALSE,mhDefer);

      break;

    case mcoDontCancelInteractiveHJobs:

      MSched.DontCancelInteractiveHJobs = MUBoolFromString(SVal,FALSE);

      break;

    case mcoCheckPointFile:

      if ((SVal[0] != '~') && (SVal[0] != '/'))
        {
        sprintf(MCP.CPFileName,"%s%s",
          S->VarDir,
          SVal);
        }
      else
        {
        MUStrCpy(MCP.CPFileName,SVal,sizeof(MCP.CPFileName));
        }

      break;

    case mcoCheckPointInterval:

      MCP.CPInterval = MUTimeFromString(SVal);

      break;

    case mcoCheckPointAllAccountLists:

      MCP.CheckPointAllAccountLists = MUBoolFromString(SVal,FALSE);

      break;

    case mcoCheckPointExpirationTime:

      if (!strcmp(SVal,"-1") || 
          strstr(SVal,"INFIN") || 
          strstr(SVal,"UNLIMITED") ||
          strstr(SVal,"NONE"))
        {
        MCP.CPExpirationTime = MMAX_TIME;
        }
      else
        {
        MCP.CPExpirationTime = MUTimeFromString(SVal);
        }

      break;

    case mcoCheckPointWithDatabase:

      /* this is causing problems (RT7273) and Doug has decided that
         checkpointing to a database is not a good idea at this point
         in moab's life, maybe later. */

      MCP.UseDatabase = MUBoolFromString(SVal,FALSE);

      break;

    case mcoCheckSuspendedJobPriority:

      MSched.CheckSuspendedJobPriority = MUBoolFromString(SVal,FALSE);

      break;

    case mcoChildStdErrCheck:

      MSched.ChildStdErrCheck = MUBoolFromString(SVal,FALSE);

      break;

    case mcoClientUIPort:

      MSched.ClientUIPort = IVal;

      break;

    case mcoCredDiscovery:

      MSched.CredDiscovery = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDefaultRackSize:

      MSched.DefaultRackSize = IVal;

      break;

    case mcoDefaultVMSovereign:

      MSched.DefaultVMSovereign = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableBatchHolds:

      S->DisableBatchHolds = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableExcHList:

      S->DisableExcHList = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableInteractiveJobs:

      S->DisableInteractiveJobs = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableRegExCaching:

      S->DisableRegExCaching = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableRequiredGResNone:

      S->DisableRequiredGResNone = MUBoolFromString(SVal,FALSE);

      break;

    case mcoIgnoreRMDataStaging:

      S->IgnoreRMDataStaging = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableSameCredPreemption:

      bmfromstring(SVal,MAttrO,&MSched.DisableSameCredPreemptionBM);

      break;

    case mcoDisableSameQOSPreemption:

      MSched.DisableSameQOSPreemption = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableThresholdTriggers:

      S->DisableThresholdTrigs = MUBoolFromString(SVal,FALSE);

      if (S->DisableThresholdTrigs == TRUE)
        {
        bmset(&S->RecordEventList,mrelTrigThreshold);
        }

      break;
 
    case mcoDisableUICompression:

      S->DisableUICompression = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDisableVMDecisions:

      S->DisableVMDecisions = MUBoolFromString(SVal,FALSE);

      if (S->DisableVMDecisions == TRUE)
        {
        bmset(&S->RecordEventList,mrelNodePowerOff);
        bmset(&S->RecordEventList,mrelNodePowerOn);
        bmset(&S->RecordEventList,mrelNodeProvision);
        bmset(&S->RecordEventList,mrelVMMigrate);
        }

      break;

    case mcoEmulationMode:

      {
      enum MRMSubTypeEnum tmpEMode;

      tmpEMode = (enum MRMSubTypeEnum)MUGetIndexCI(SVal,MRMSubType,FALSE,0);

      MSchedSetEmulationMode(tmpEMode);
      }  /* END BLOCK */

      break;

    case mcoEnableFSViolationPreemption:

      MSched.EnableFSViolationPreemption = MUBoolFromString(SVal,FALSE);

      break;

    case mcoEnableSPViolationPreemption:

      MSched.EnableSPViolationPreemption = MUBoolFromString(SVal,FALSE);

      break;

    case mcoEnableVMDestroy:

      MSched.EnableVMDestroy = MUBoolFromString(SVal,FALSE);

      break;

    case mcoEnableVMSwap:

      MSched.EnableVMSwap = MUBoolFromString(SVal,FALSE);

      break;

    case mcoEnableVPCPreemption:

      MSched.EnableVPCPreemption = MUBoolFromString(SVal,FALSE);

      break;

    case mcoSubmitEnvFileLocation:

      MUStrDup(&MSched.SubmitEnvFileLocation,SVal);

      break;

    case mcoExtendedJobTracking:

      MSched.ExtendedJobTracking = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFilterCmdFile:

      MSched.FilterCmdFile = MUBoolFromString(SVal,TRUE);

      break;

    case mcoMapFeatureToPartition:

      MSched.MapFeatureToPartition = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFreeTimeLookAheadDuration:

      MSched.FreeTimeLookAheadDuration = MUTimeFromString(SVal);

      break;

    case mcoFSRelativeUsage:

      MSched.FSRelativeUsage = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSTreeACLPolicy:

      MSched.FSTreeACLPolicy = (enum MFSTreeACLPolicyEnum)MUGetIndexCI(SVal,MFSTreeACLPolicy,FALSE,mfsapFull);

      break;

    case mcoFSTreeIsRequired:

      MSched.FSTreeIsRequired = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSTreeUserIsRequired:
    
      MSched.FSTreeUserIsRequired = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSMostSpecificLimit:

      MSched.FSMostSpecificLimit = MUBoolFromString(SVal,FALSE);

      break;

    case mcoMDiagXmlJobNotes:

      MSched.MDiagXmlJobNotes = MUBoolFromString(SVal,FALSE);

      break;

    case mcoNonBlockingCacheInterval:

      MSched.NonBlockingCacheInterval = MUTimeFromString(SVal);

      break;

    case mcoNodeBusyStateDelayTime:

      MPar[0].NodeBusyStateDelayTime = MUTimeFromString(SVal);

      break;

    case mcoNodeCatCredList:

      /* FORMAT:  <CRED>=<CAT>[{:,}<CAT>]... */
      /*     ie:  down=down-batch,down-hw,down-net idle=idle */

      if (SArray == NULL)
        break;

      {
      mgcred_t *U;
      mgcred_t *G;
      mgcred_t *A;
      mqos_t   *Q;
      mclass_t *C;

      int sindex;
      int nindex;
      int ncindex;

      char *ptr;
      char *CName;

      char *TokPtr;

      MSched.NodeCatCredEnabled = TRUE;

      for (sindex = 0;sindex < MMAX_CONFIGATTR;sindex++)
        {
        if (SArray[sindex] == NULL)
          break;

        CName = MUStrTok(SArray[sindex],"=",&TokPtr);

        if (MUserAdd(CName,&U) == SUCCESS)
          U->Role = mcalTemplate;

        if (MGroupAdd(CName,&G) == SUCCESS)
          G->Role = mcalTemplate;       

        if (MAcctAdd(CName,&A) == SUCCESS)
          A->Role = mcalTemplate;

        if (MQOSAdd(CName,&Q) == SUCCESS)
          Q->Role = mcalTemplate;

        if (MClassAdd(CName,&C) == SUCCESS)
          C->Role = mcalTemplate;

        for (nindex = 1;nindex < mncLAST;nindex++)
          {
          if (MSched.NodeCatGroup[nindex] == NULL)
            {
            MUStrDup(&MSched.NodeCatGroup[nindex],CName);

            break;
            }
          }    /* END for (nindex) */

        ptr = MUStrTok(NULL,",:",&TokPtr);

        while (ptr != NULL)
          {
          ncindex = MUGetIndexCI(ptr,MNodeCat,FALSE,0);

          if (ncindex != 0)
            {         
            MDB(3,fCONFIG) MLog("INFO:     nodecat '%s' tracked via cred %s\n",
              ptr,
              CName);

            if (ncindex == mncRsv_Job)
              MSched.TrackIdleJobPriority = TRUE;

            MSched.NodeCatU[nindex] = U;
            MSched.NodeCatG[nindex] = G;
            MSched.NodeCatA[nindex] = A;
            MSched.NodeCatQ[nindex] = Q;
            MSched.NodeCatC[nindex] = C;

            MSched.NodeCatGroupIndex[ncindex] = nindex; 
            }
 
          ptr = MUStrTok(NULL,",:",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END for (sindex) */
      }      /* END case mcoNodeCatCredList */

      break;

    case mcoNodeDownStateDelayTime:

      MPar[0].NodeDownStateDelayTime = MUTimeFromString(SVal);

      break;

    case mcoNodeDrainStateDelayTime:

      if (!strcmp(SVal,"-1"))
        MPar[0].NodeDrainStateDelayTime = MCONST_EFFINF;
      else
        MPar[0].NodeDrainStateDelayTime = MUTimeFromString(SVal);

      break;

    case mcoNodeIdlePowerThreshold:
     
      MSched.NodeIdlePowerThreshold = MUTimeFromString(SVal);
  
      break;

    case mcoNodeSetPlus:

      MSched.NodeSetPlus = (enum MNodeSetPlusPolicyEnum)MUGetIndexCI(SVal,MNodeSetPlusPolicy,FALSE,0);

      break;

    case mcoNodeSetMaxUsage:

      /* Set on the partition, but done through this function
          to not create unnecessary partitions */

      {
      if ((DVal > 1.0) || (DVal < 0.0))
        {
        MDB(1,fSTRUCT) MLog("ERROR:    ignoring NODESETMAXUSAGE because set incorrectly (%f). Valid values are 0.0 - 1.0.\n",
          DVal);

        break;
        }

      if ((IndexName == NULL) || (IndexName[0] == '\0'))
        {
        /* No node set name given, put as the general on MPar[0] */

        MPar[0].NodeSetMaxUsage = DVal;
        }
      else
        {
        /* A node set name has been given */

        int NSIndex = -1;
        int tmpI = 0;

        for (tmpI = 0;MPar[0].NodeSetList[tmpI] != NULL;tmpI++)
          {
          if (!strcmp(MPar[0].NodeSetList[tmpI],IndexName))
            {
            NSIndex = tmpI;

            break;
            }
          }

        if (NSIndex >= 0)
          {
          MPar[0].NodeSetListMaxUsage[NSIndex] = DVal; 
          MPar[0].NodeSetMaxUsageSet = TRUE;
          }
        else if (MUStrNCmpCI(IndexName,"DEFAULT",strlen("DEFAULT")) == SUCCESS)
          {
          MPar[0].NodeSetMaxUsage = DVal;
          }
        else
          {
          MDB(1,fSTRUCT) MLog("ERROR:    could not find node set type '%s' (was it declared in NODESETLIST?)\n",
            IndexName);
          }
        }
      } /* END case mcoNodeSetMaxUsage */

      break;

    case mcoNoJobHoldNoResources:

      MSched.NoJobHoldNoResources = MUBoolFromString(SVal,FALSE);

      break;

    case mcoNoLocalUserEnv:

      MSched.NoLocalUserEnv = MUBoolFromString(SVal,FALSE);

      break;

    case mcoPerPartitionScheduling:

      MSched.PerPartitionScheduling = MUBoolFromString(SVal,FALSE);

      break;

    case mcoPushCacheToWebService:

      MSched.PushCacheToWebService = MUBoolFromString(SVal,FALSE);

      break;

    case mcoPushEventsToWebService:

      MSched.PushEventsToWebService = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRecordEventList:

      {
      /* FORMAT: <EVENT>[{ ,}<EVENT>]... */

      if (!strcasecmp(SVal,"all"))
        {
        int index;

        for (index = 1;index < mrelLAST;index++)
          bmset(&S->RecordEventList,index);
        }
      else
        {
        char *ptr;
        char *TokPtr;
       
        char  tmpI;

        if (!strchr(SVal,'+'))
          bmclear(&S->RecordEventList);

        ptr = MUStrTok(SVal,"+,: \t\n ",&TokPtr);

        while (ptr != NULL)
          {
          tmpI = MUGetIndexCI(ptr,MRecordEventType,FALSE,0);

          bmset(&S->RecordEventList,tmpI);

          if (tmpI == mrelAllSchedCommand)
            {
            bmset(&S->RecordEventList,mrelSchedCommand);
            }
            
 
          ptr = MUStrTok(NULL,"+,: \t\n ",&TokPtr);
          }
        }
      }  /* END BLOCK (case mcoRecordEventList) */

      break;

    case mcoReportProfileStatsAsChild:

      MSched.ReportProfileStatsAsChild = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRegistrationURL:

      MUStrDup(&MSched.RegistrationURL,SVal);

      break;

    case mcoRejectInfeasibleJobs:

      S->RejectInfeasibleJobs = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRejectDuplicateJobs:

      S->RejectDuplicateJobs = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRelToAbsPaths:

      S->RelToAbsPaths = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRsvTimeDepth:

      S->RsvTimeDepth = MUTimeFromString(SVal);

      break;

    case mcoVPCMigrationPolicy:

      S->VPCMigrationPolicy = (enum MVPCMigrationPolicyEnum)MUGetIndex(
        SVal,
        MVPCMigrationPolicy,
        FALSE,
        S->VPCMigrationPolicy);

      break;

    case mcoFallBackClass:

      MClassAdd(SVal,&MSched.FallBackClass);

      break;

    case mcoFileRequestIsJobCentric:

      S->FileRequestIsJobCentric = MUBoolFromString(SVal,FALSE);

      break;

    case mcoForceNodeReprovision:

      S->ForceNodeReprovision = MUBoolFromString(SVal,FALSE);

      break;

    case mcoForceRsvSubType:

      S->ForceRsvSubType = MUBoolFromString(SVal,FALSE);

      break;

    case mcoForceTrigJobsIdle:

      S->ForceTrigJobsIdle = MUBoolFromString(SVal,FALSE);

      break;


    case mcoFSChargeRate:

      S->FSChargeRate = DVal;

      break;

    case mcoFullJobTrigCP:

      S->FullJobTrigCP = MUBoolFromString(SVal,FALSE);

      break;

    case mcoHideVirtualNodes:

      if (!strcasecmp(SVal,"transparent"))
        {
        S->ManageTransparentVMs = TRUE;
        }

      break;

    case mcoIgnoreClasses:

      {
      char *ptr;
      char *TokPtr;

      char **LPtr;

      int    oindex;

      if (strchr(SVal,'!') != NULL)
        {
        if ((S->SpecClasses == NULL) &&
           ((S->SpecClasses = (char **)MUCalloc(1,sizeof(char *) * (MSched.M[mxoClass] + 1))) == NULL))
          {
          return(FAILURE);
          }

        LPtr = S->SpecClasses;
        }
      else
        {
        if ((S->IgnoreClasses == NULL) &&
           ((S->IgnoreClasses = (char **)MUCalloc(1,sizeof(char *) * (MSched.M[mxoClass] + 1))) == NULL))
          {
          return(FAILURE);
          }

        LPtr = S->IgnoreClasses;
        }

      oindex = 0;

      ptr = MUStrTok(SVal," \t\n,!:",&TokPtr);

      while (ptr != NULL)
        {
        MUStrDup(&LPtr[oindex],ptr);

        oindex++;

        if (oindex >= MSched.M[mxoClass])
          break;

        ptr = MUStrTok(NULL," \t\n,!:",&TokPtr);
        }  /* END while (ptr) */

      LPtr[oindex] = NULL;
      }  /* END BLOCK (case mcoIgnoreClasses) */

      break;

    case mcoIgnoreJobs:

      {
      char *ptr;
      char *TokPtr;

      char **LPtr;

      int    oindex;

      if (strchr(SVal,'!') != NULL)
        {
        if ((S->SpecJobs == NULL) &&
           ((S->SpecJobs = (char **)MUCalloc(1,sizeof(char *) * (MMAX_IGNJOBS + 1))) == NULL))
          {
          return(FAILURE);
          }

        LPtr = S->SpecJobs;
        }
      else
        {
        if ((S->IgnoreJobs == NULL) &&
           ((S->IgnoreJobs = (char **)MUCalloc(1,sizeof(char *) * (MMAX_IGNJOBS + 1))) == NULL))
          {
          return(FAILURE);
          }

        LPtr = S->IgnoreJobs;
        }

      oindex = 0;

      ptr = MUStrTok(SVal," \t\n,!:",&TokPtr);

      while (ptr != NULL)
        {
        MUStrDup(&LPtr[oindex],ptr);

        oindex++;

        if (oindex >= MMAX_IGNJOBS)
          break;

        ptr = MUStrTok(NULL," \t\n,!:",&TokPtr);
        }  /* END while (ptr) */

      LPtr[oindex] = NULL;
      }  /* END BLOCK (case mcoIgnoreJobs) */

      break;

    case mcoIgnoreLLMaxStarters:

      S->IgnoreLLMaxStarters = MUBoolFromString(SVal,FALSE);

      break;

    case mcoIgnoreNodes:

      MSchedSetAttr(S,msaIgnoreNodes,(void **)SVal,mdfString,mSet);

      break;

    case mcoIgnorePreempteePriority:

      S->IgnorePreempteePriority = MUBoolFromString(SVal,FALSE);

      break;

    case mcoIgnoreUsers:

      {
      char *ptr;
      char *TokPtr;

      char **LPtr;

      int    oindex;

      if (strchr(SVal,'!') != NULL)
        {
        if ((S->SpecUsers == NULL) &&
           ((S->SpecUsers = (char **)MUCalloc(1,sizeof(char *) * (MSched.M[mxoUser] + 1))) == NULL))
          {
          return(FAILURE);
          }

        LPtr = S->SpecUsers;
        }
      else
        {
        if ((S->IgnoreUsers == NULL) &&
           ((S->IgnoreUsers = (char **)MUCalloc(1,sizeof(char *) * (MSched.M[mxoUser] + 1))) == NULL))
          {
          return(FAILURE);
          }

        LPtr = S->IgnoreUsers;
        }

      oindex = 0;

      ptr = MUStrTok(SVal," \t\n,!:",&TokPtr);

      while (ptr != NULL)
        {
        MUStrDup(&LPtr[oindex],ptr);

        oindex++;

        if (oindex >= MSched.M[mxoUser])
          break;

        ptr = MUStrTok(NULL," \t\n,!:",&TokPtr);
        }  /* END while (ptr) */

      LPtr[oindex] = NULL;
      }  /* END BLOCK (case mcoIgnoreUsers) */

      break;

    case mcoInstantStage:

      {
      /* deprecated - see mcoJobMigratePolicy */

      mbool_t tmpB;

      tmpB = MUBoolFromString(SVal,FALSE);

      switch (tmpB)
        {
        case TRUE:  MSched.JobMigratePolicy = mjmpImmediate; break;
        case FALSE: MSched.JobMigratePolicy = mjmpJustInTime; break;
        default:    MSched.JobMigratePolicy = mjmpAuto; break;
        }
      }

      break;

    case mcoInternalTVarPrefix:

      MUStrDup(&MSched.TVarPrefix,SVal);

      break;

    case mcoInvalidFSTreeMsg:

      MUStrDup(&S->SpecEMsg[memInvalidFSTree],SVal);

      break;

    case mcoJobCCodeFailSampleSize:

      S->JobCCodeFailSampleSize = IVal;

      break;

    case mcoJobCfg:

      /* loaded directly elsewhere */

      /* MTJobProcessConfig(IndexName,SVal,NULL); */

      break;

    case mcoJobFailRetryCount:

      S->JobFailRetryCount = IVal;

      break;

    case mcoJobMaxTaskCount:

      /* only allow update before scheduling begins */

      if (MSched.Iteration != -1)
        break;

      if (SVal != NULL)
        {
        /* FORMAT:  <MAX> */

        S->JobMaxTaskCount = strtol(SVal,NULL,10);
        S->JobMaxTaskCount += 1;  /* add one for the terminator */

        MDB(3,fSCHED) MLog("ALERT:    parameter %s set to %d\n",
          MParam[mcoJobMaxTaskCount],
          S->JobMaxTaskCount - 1);
        }

      break;

    case mcoJobMaxNodeCount:

      /* only allow update before scheduling begins */

      if (MSched.Iteration != -1)
        break;

      if (SVal != NULL)
        {
        char *ptr1;
        char *ptr2;

        char *TokPtr;

        /* FORMAT:  [<DEF>,]<MAX> */

        ptr1 = MUStrTok(SVal,",",&TokPtr);
        ptr2 = MUStrTok(NULL,",",&TokPtr);

        if (ptr2 == NULL)
          {
          S->JobMaxNodeCount = strtol(ptr1,NULL,10);
          S->JobDefNodeCount = MSched.JobMaxNodeCount;
          }
        else
          {
          S->JobDefNodeCount = strtol(ptr1,NULL,10);
          S->JobMaxNodeCount = strtol(ptr2,NULL,10);
          }
        }

      if (S->JobMaxNodeCount > MSched.M[mxoNode])
        {
        MDB(0,fSCHED) MLog("ALERT:    parameter %s cannot exceed %d\n",
          MParam[mcoJobMaxNodeCount],
          MSched.M[mxoNode]);

        S->JobDefNodeCount = MSched.M[mxoNode];
        S->JobMaxNodeCount = MSched.M[mxoNode];
        }
      else if (S->JobDefNodeCount > S->JobMaxNodeCount)
        {
        S->JobDefNodeCount = S->JobMaxNodeCount;
        }

      break;

    case mcoJobMigratePolicy:

      S->JobMigratePolicy = (enum MJobMigratePolicyEnum)MUGetIndexCI(
        SVal,
        MJobMigratePolicy,
        FALSE,
        S->JobMigratePolicy);

      break;

    case mcoLicenseChargeRate:

      S->LicenseChargeRate = DVal;

      break;

    case mcoLimitedJobCP:

      S->LimitedJobCP = MUBoolFromString(SVal,FALSE);

      break;

    case mcoLimitedNodeCP:

      S->LimitedNodeCP = MUBoolFromString(SVal,FALSE);

      break;

    case mcoLoadAllJobCP:

      S->LoadAllJobCP = MUBoolFromString(SVal,FALSE);

      break;

    case mcoNetworkChargeRate:

      S->NetworkChargeRate = DVal;

      break;

    case mcoNodeIDFormat:

      MUStrDup(&S->NodeIDFormat,SVal);

      break;

    case mcoObjectEList:

      S->OESearchOrder = (enum MOExpSearchOrderEnum)MUGetIndexCI(
        SVal,
        MOExpSearchOrder,
        FALSE,
        S->OESearchOrder);

      break;

    case mcoOptimizedCheckpointing:

      S->OptimizedCheckpointing = MUBoolFromString(SVal,FALSE);

      break;

    case mcoOptimizedJobArrays:

      S->OptimizedJobArrays = MUBoolFromString(SVal,FALSE);

      break;

    case mcoOSCredLookup:

      S->OSCredLookup = (enum MActionPolicyEnum)MUGetIndex(
        SVal,
        MActionPolicy,
        FALSE,
        S->OSCredLookup);

      break;

    case mcoQueueNAMIActions:

      S->QueueNAMIActions = MUBoolFromString(SVal,FALSE);

      break;

    case mcoReportPeerStartInfo:

      S->ReportPeerStartInfo = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRequeueRecoveryRatio:

      S->RequeueRecoveryRatio = DVal;

      break;

    case mcoResourceQueryDepth:

      if (SVal == NULL)
        break;

      S->ResourceQueryDepth = (int)strtol(SVal,NULL,10);

      break;

    case mcoMemRefreshInterval:
    case mcoRestartInterval:

      {
      char tmpLine[MMAX_LINE];

      char *ptr;
      char *TokPtr;

      long tmpL;

      MUStrCpy(tmpLine,SVal,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,":",&TokPtr);

      if ((ptr != NULL) && (!strcmp(ptr,"job") || !strcmp(ptr,"JOB")))
        {
        ptr = MUStrTok(NULL,":",&TokPtr);

        MUStrDup(&MSched.RecycleArgs,"savestate");

        MSched.RestartJobCount = strtol(ptr,NULL,10);

        MDB(3,fCONFIG) MLog("INFO:     sched recycle time set to %ld seconds\n",
          MSched.RestartJobCount);
        }
      else
        {
        tmpL = MUTimeFromString(SVal);

        if (tmpL > 0)
          {
          MUStrDup(&MSched.RecycleArgs,"savestate");

          MSched.RestartTime = MSched.Time + tmpL;

          MDB(3,fCONFIG) MLog("INFO:     sched recycle time set to %ld seconds\n",
            tmpL);
          }
        }
      }    /* END BLOCK (case mcoRestartInterval/mcoMemRefreshInterval) */

      break;
 
    case mcoDeadlinePolicy:

      if (SVal == NULL)
        break;

      S->DeadlinePolicy = (enum MDeadlinePolicyEnum)MUGetIndex(
        SVal,
        MDeadlinePolicy,
        FALSE,
        mdlpNONE);

      break;

    case mcoNAMITransVarName:

      if ((SVal == NULL) || (SVal[0] == '\0'))
        break;

      MUStrDup(&MSched.NAMITransVarName,SVal);

      break;
 
    case mcoNestedNodeSet:

      {
      /* FORMAT: <feature>[|<feature>...][:<feature>[|<feature>]]... */
 
      mln_t *tmpL;

      char Buf[MMAX_NAME];

      int  index = 0;
      int  index2;

      char *ptr;
      char *ptr2;
      char *TokPtr;
      char *TokPtr2;
      char *String = NULL;

      char **List;

      MUStrDup(&String,SVal);
      
      ptr = MUStrTok(String,":",&TokPtr);

      while (ptr != NULL)
        {
        /* Add a new nodeset, name is just the index */

        ptr2 = MUStrTok(ptr,"|",&TokPtr2);

        if (ptr2 == NULL)
          break;

        snprintf(Buf,sizeof(Buf),"%d",index++);

        MULLAdd(&MSched.NestedNodeSet,Buf,NULL,&tmpL,NULL);
 
        tmpL->Ptr = (char **)MUCalloc(1,sizeof(char *) * MMAX_ATTR);

        List = (char **)tmpL->Ptr;

        index2 = 0;

        while (ptr2 != NULL)
          {     
          MUStrDup(&List[index2++],ptr2);

          ptr2 = MUStrTok(NULL,"|",&TokPtr2);
          }         

        ptr = MUStrTok(NULL,":",&TokPtr);
        }  /* END while (ptr != NULL) */

      MPar[0].NodeSetAttribute = mrstFeature;
      MPar[0].NodeSetIsOptional = FALSE;
      MPar[0].NodeSetPolicy = mrssOneOf;

      MUFree(&String);
      }    /* END case mcoNestedNodeSet */

      break;

    case mcoMissingDependencyAction:

      if (SVal == NULL)
        break;

      S->MissingDependencyAction = (enum MMissingDependencyActionEnum)MUGetIndex(
        SVal,
        MMissingDependencyAction,
        FALSE,
        mmdaNONE);

      break;

    case mcoMongoPassword:

      if (MUStrIsEmpty(SVal))
        MUFree(&S->MongoPassword);
      else
        MUStrDup(&S->MongoPassword,SVal);

      break;

    case mcoMongoServer:

      if (MUStrIsEmpty(SVal))
        MUFree(&S->MongoServer);
      else
        {
        MUStrDup(&S->MongoServer,SVal);
        }

      break;

    case mcoMongoUser:

      if (MUStrIsEmpty(SVal))
        MUFree(&S->MongoUser);
      else
        MUStrDup(&S->MongoUser,SVal);

      break;

    case mcoDefaultDomain:

      /* NOTE:  always proceed with '.' */

      if (SVal[0] == '.')
        {
        MUStrCpy(S->DefaultDomain,SVal,sizeof(S->DefaultDomain));
        }
      else if (SVal[0] != '\0')
        {
        sprintf(S->DefaultDomain,".%s",
          SVal);
        }

      break;

    case mcoDefaultJobOS:

      S->DefaultJobOS = MUMAGetIndex(meOpsys,SVal,mAdd);

      break;

    case mcoDefaultClassList:

      {
      int index;

      S->DefaultClassList[0] = '\0';

      if (SArray != NULL)
        {
        for (index = 0;SArray[index] != NULL;index++)
          {
          sprintf(S->DefaultClassList,"%s[%s]",
            S->DefaultClassList,
            SArray[index]);

          MDB(3,fCONFIG) MLog("INFO:     default class '%s' added\n",
            SArray[index]);
          }  /* END for (index) */
        }    /* END if (SArray != NULL) */
      }      /* END BLOCK */

      break;

    case mcoDefaultQMHost:

      MUStrCpy(S->DefaultQMHost,SVal,sizeof(S->DefaultQMHost));

      break;

    case mcoDefaultTaskMigrationTime:

      S->DefaultTaskMigrationDuration = MUTimeFromString(SVal);

      break;

    case mcoDeferTime:

      S->DeferTime = MUTimeFromString(SVal);

      break;

    case mcoDeferCount:

      S->DeferCount = IVal;

      break;

    case mcoDeferStartCount:

      S->DeferStartCount = IVal;

      break;

    case mcoDeleteStageOutFiles:

      S->DeleteStageOutFiles = MUBoolFromString(SVal,FALSE);

      break;

    case mcoDependFailurePolicy:

      S->DependFailPolicy = (enum MDependFailPolicyEnum)MUGetIndexCI(SVal,MDependFailPolicy,FALSE,mdfpHold); 

      break;

    case mcoGreenPoolEvalInterval:

      S->GreenPoolEvalInterval = (enum MCalPeriodEnum)MUGetIndexCI(SVal,MCalPeriodType,FALSE,mpDay);

      break;

    case mcoHAPollInterval:

      S->HAPollInterval = MUTimeFromString(SVal);

      break;

    case mcoJobCPurgeTime:

      S->JobCPurgeTime = MUTimeFromString(SVal);
      S->JobCPurgeTimeSpecified = TRUE;

      break;

    case mcoJobCTruncateNLCP:

      S->JobCTruncateNLCP = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoJobCTruncateNLCP],
        MBool[S->JobCTruncateNLCP]);

      break;

    case mcoJobPreemptCompletionTime:

      S->JobPreemptCompletionTime = MUTimeFromString(SVal);

      break;

    case mcoMaxSuspendTime:

      S->MaxSuspendTime = MUTimeFromString(SVal);

      break;

    case mcoJobPreemptMaxActiveTime:

      S->JobPreemptMaxActiveTime = MUTimeFromString(SVal);

      break;

    case mcoJobPreemptMinActiveTime:

      S->JobPreemptMinActiveTime = MUTimeFromString(SVal);

      break;

    case mcoJobPurgeTime:

      /* with delta-based job scheduling, cannot allow scheduler purge of jobs */

      if (!strcmp(SVal,"-1"))
        {
        /* NOTE:  use effective infinity to prevent long overflow */
        S->JobPurgeTime = MCONST_EFFINF;
        }
      else
        {
        S->JobPurgeTime = MUTimeFromString(SVal);
        }

      break;

    case mcoJobRejectPolicy:

      MSchedProcessJobRejectPolicy(&S->JobRejectPolicyBM,SArray);

      break;

    case mcoJobRemoveEnvVarList:

      MUStrDup(&S->JobRemoveEnvVarList,SVal);

      break;

    case mcoNodeDownTime:

      /* FORMAT:  { [[[DD:]HH:]MM:]SS } */

      S->NodeDownTime = MUTimeFromString(SVal);

      break;

    case mcoNodeFailureReserveTime:

      S->NodeFailureReserveTime = MUTimeFromString(SVal);

      break;

    case mcoNodePollFrequency:

      S->NodePollFrequency = MAX(-3,IVal);

      break;

    case mcoNodePurgeTime:

      /* FORMAT:  { <ITERATIONCOUNT>I | [[[DD:]HH:]MM:]SS } */

      if (strchr(SVal,'I') != NULL)
        S->NodePurgeIteration = (int)strtol(SVal,NULL,10);
      else
        S->NodePurgeTime = MUTimeFromString(SVal);

      break;

    case mcoAPIFailureThreshold:

      S->APIFailureThreshold = IVal;

      break;

    case mcoNodeSyncDeadline:

      S->NodeSyncDeadline = MUTimeFromString(SVal);

      break;

    case mcoJobSyncDeadline:

      S->JobSyncDeadline = MUTimeFromString(SVal);

      break;

    case mcoParIgnQList:

      /* multi-string */

      {
      /* FORMAT:  <QNAME>[{ ,:}<QNAME>] */

      char *ptr;
      char *TokPtr;

      int   pindex;
      int   qindex;

      if (S->IgnQList == NULL)
        S->IgnQList = (char **)MUCalloc(1,sizeof(char *) * MMAX_CLASS);

      qindex = 0;

      for (pindex = 0;SArray[pindex] != NULL;pindex++)
        {
        ptr = MUStrTok(SArray[pindex],",: \t",&TokPtr);

        while (ptr != NULL)
          {
          MUStrDup(&S->IgnQList[qindex],ptr);

          ptr = MUStrTok(NULL,",: \t",&TokPtr);

          qindex++;
          }
        }    /* END for (pindex) */

      MUFree(&S->IgnQList[qindex]);
      }  /* END BLOCK */

      break;

    case mcoARFDuration:

      S->ARFDuration = IVal;

      break;

    case mcoJobActionOnNodeFailure:
    case mcoAllocResFailPolicy:

      {
      char *ptr;

      /* FORMAT:  <POLICY>[*] */

      ptr = strchr(SVal,'*');

      if (ptr != NULL)
        {
        *ptr = '\0';

        S->ARFOnMasterTaskOnly = TRUE;
        }

      S->ARFPolicy = (enum MAllocResFailurePolicyEnum)MUGetIndexCI(
        SVal,
        MARFPolicy,
        FALSE,
        0);
      }

      break;

    case mcoJobAggregationTime:

      {
      char *ptr;
      char *TokPtr;

      /* FORMAT:  <JOBSUBMIT>[,<JOBTERMINATE>] */

      ptr = MUStrTok(SVal,",",&TokPtr);

      S->RMEventAggregationTime = MUTimeFromString(ptr);

      ptr = MUStrTok(NULL,",",&TokPtr);

      if (ptr != NULL)
        {
        /* for now, also set event aggregation time */

        S->RMJobAggregationTime = MUTimeFromString(ptr);
        }
      else
        {
        /* set event aggregation time */

        S->RMJobAggregationTime = S->RMEventAggregationTime;
        }
      }    /* END BLOCK (case mcoJobAggregationTime) */

      break;

    case mcoJobRsvRetryInterval:

      S->JobRsvRetryInterval = MUTimeFromString(SVal);

      break;

    case mcoJobMaxOverrun:

      if (!strcmp(SVal,"-1"))
        {
        S->JobHMaxOverrun = -1;
        }
      else if (strchr(SVal,'%') != NULL)
        {
        char *ptr = NULL;
        char *TokPtr;

        double Limit = 0.0;

        ptr = MUStrTok(SVal," \t\n\%",&TokPtr);

        if (ptr != NULL)
          Limit = strtod(ptr,NULL);

        if ((Limit > 0) && (Limit <= 100))
          {
          S->JobPercentOverrun = Limit/100.0;
          }
        }
      else
        {
        __MCredParseLimit(SVal,&S->JobSMaxOverrun,&S->JobHMaxOverrun);
        }

      break;

    case mcoJobStartLogLevel:

     MDB(3,fCONFIG) MLog("INFO:     changing old %s value (%d)\n",
        MParam[PIndex],
        IVal);

      MSched.JobStartThreshold = IVal;

      MDB(3,fCONFIG) MLog("INFO:     new %s value (%d)\n",
        MParam[PIndex],
        IVal);

      break;

    case mcoLogFileMaxSize:

      S->LogFileMaxSize = IVal;

      /* N/A indicates unknown - do not modify */

      MLogInitialize("N/A",S->LogFileMaxSize,S->Iteration);

      break;

    case mcoLogFileRollDepth:

      S->LogFileRollDepth = IVal;

      break;

    case mcoLogLevel:

      MDB(3,fCONFIG) MLog("INFO:     changing old %s value (%d)\n",
        MParam[PIndex],
        IVal);

      mlog.Threshold = IVal;

      MDB(3,fCONFIG) MLog("INFO:     new %s value (%d)\n",
        MParam[PIndex],
        IVal);

      break;

    case mcoLogPermissions:

      /* Get the permissions as an octal. */

      S->LogPermissions = strtol(SVal,NULL,8);
 
      if (S->LogPermissions > 0)
        chmod(LogFile,S->LogPermissions);

      break;

    case mcoLogLevelOverride:

      MSched.LogLevelOverride = MUBoolFromString(SVal,FALSE);

      break;

    case mcoLogRollAction:

      {
      const char *trigName = "logrollaction";
      char  trigDesc[MMAX_LINE];

      snprintf(trigDesc,sizeof(trigDesc),"atype=exec,name=%s,etype=logrollaction,mutilfire=true,action=\"%s\"",
          trigName,
          SVal);

      MTrigLoadString(&S->TList,trigDesc,TRUE,FALSE,mxoSched,S->Name,NULL,NULL);

      if (MTrigFind(trigName,&MSched.LogRollTrigger) == SUCCESS)
        MTrigInitialize(MSched.LogRollTrigger);
      }

      break;

    case mcoLogFacility:

      {
      mstring_t Line(MMAX_LINE);

      /* FORMAT:  <FACILITY>[:<FACILITY>]... */

      bmfromstring(SVal,MLogFacilityType,&mlog.FacilityBM,":,");

      if (bmisset(&mlog.FacilityBM,fALL))
        {
        MStringSet(&Line,MLogFacilityType[fALL]);
        }
      else
        {
        bmtostring(&mlog.FacilityBM,MLogFacilityType,&Line);
        }

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[PIndex],
        Line.c_str());
      }  /* END BLOCK (mcoLogFacility) */

      break;

    case mcoMaxDepends:

      S->M[mxoxDepends] = IVal;

      break;

    case mcoMaxGreenStandbyPoolSize:

      S->MaxGreenStandbyPoolSize = IVal;

      break;

    case mcoMaxJobHoldTime:

      S->MaxJobHoldTime = MUTimeFromString(SVal);

      break;

    case mcoMaxJobStartPerIteration:

      /* FORMAT:  [<PRIOMAX>,]<TOTALMAX> */

      __MCredParseLimit(
        SVal,
        &S->MaxPrioJobStartPerIteration,
        &S->MaxJobStartPerIteration);

      break;

    case mcoMaxJobPreemptPerIteration:
     
      S->MaxJobPreemptPerIteration = IVal;
    
      break;

    case mcoMinDispatchTime:

      S->MinDispatchTime = MUTimeFromString(SVal);

      break;

    case mcoMaxSleepIteration:

      S->MaxSleepIteration = IVal;

      break;

    case mcoNodeMaxLoad:

      S->DefaultN.MaxLoad = DVal;

      break;

    case mcoNodeCPUOverCommitFactor:

      S->ResOvercommitSpecified[0] = TRUE;
      S->ResOvercommitSpecified[mrlProc] = TRUE;

      S->DefaultN.ResOvercommitFactor[mrlProc] = DVal;

      break;

    case mcoNodeMemOverCommitFactor:

      S->ResOvercommitSpecified[0] = TRUE;
      S->ResOvercommitSpecified[mrlMem] = TRUE;

      S->DefaultN.ResOvercommitFactor[mrlMem] = DVal;

      break;

    case mcoNodeNameCaseInsensitive:

      S->NodeNameCaseInsensitive = MUBoolFromString(SVal,FALSE);

      break;

    case mcoNodeUntrackedProcFactor:

      MPar[0].UntrackedProcFactor = DVal;

      break;

    case mcoNodeUntrackedResDelayTime:
 
      MPar[0].UntrackedResDelayTime = MUTimeFromString(SVal);

      break;

      break;

    case mcoPBSAccountingDir:

      MUStrDup(&S->PBSAccountingDir,SVal);

      break;

    case mcoQOSIsOptional:

      S->QOSIsOptional = MUBoolFromString(SVal,FALSE);

      break;

    case mcoQOSRejectPolicy:

      {
      int index;
      int pindex;

      char *ptr;
      char *TokPtr;

      bmclear(&S->QOSRejectPolicyBM);

      if (SArray == NULL)
        break;

      for (index = 0;SArray[index] != NULL;index++)
        {
        ptr = MUStrTok(SArray[index],",:;\t\n ",&TokPtr);

        while (ptr != NULL)
          {
          pindex = MUGetIndexCI(ptr,MJobRejectPolicy,FALSE,0);

          ptr = MUStrTok(NULL,",:;\t\n ",&TokPtr);

          if (pindex == 0)
            continue;

          bmset(&S->QOSRejectPolicyBM,pindex);
          }  /* END while (ptr != NULL) */
        }    /* END for (index) */
      }      /* END BLOCK */

      break;

    case mcoRealTimeDBObjects:

      if (!MUStrIsEmpty(SVal))
        {
        if (!strcasecmp(SVal,"all"))
          {
          bmset(&S->RealTimeDBObjects,mxoJob);
          bmset(&S->RealTimeDBObjects,mxoRsv);
          bmset(&S->RealTimeDBObjects,mxoNode);
          bmset(&S->RealTimeDBObjects,mxoTrig);
          bmset(&S->RealTimeDBObjects,mxoxVC);
          bmset(&S->RealTimeDBObjects,mxoxVM);
          }
        else if (!strcasecmp(SVal,"none"))
          {
          bmclear(&S->RealTimeDBObjects);
          }
        else
          {
          bmfromstring(SVal,MXO,&S->RealTimeDBObjects,",:");
          }
        }

      break;

    case mcoRemoveTrigOutputFiles:

      S->RemoveTrigOutputFiles = MUBoolFromString(SVal,FALSE);

      break;

    case mcoShowMigratedJobsAsIdle:

      S->ShowMigratedJobsAsIdle = MUBoolFromString(SVal,FALSE);

      break;

    case mcoStatProcMax:

      MStat.P.MaxProc = IVal;

      break;

    case mcoStatProcMin:

      MStat.P.MinProc = IVal;

      break;

    case mcoStatProcStepCount:

      __MSchedConfigMINInt(S,PIndex,IVal,MMAX_GRIDSIZECOUNT,(int *)&MStat.P.ProcStepCount);

      break;

    case mcoStatProcStepSize:

      __MSchedConfigMINInt(S,PIndex,IVal,MMAX_GRIDSIZECOUNT,(int *)&MStat.P.ProcStepSize);

      break;

    case mcoStatTimeMax:

      MStat.P.MaxTime = MUTimeFromString(SVal);

      break;

    case mcoStatTimeMin:

      MStat.P.MinTime = MUTimeFromString(SVal);

      break;

    case mcoStatTimeStepCount:

      __MSchedConfigMINInt(S,PIndex,IVal,MMAX_GRIDTIMECOUNT,(int *)&MStat.P.TimeStepCount);

      break;

    case mcoStatTimeStepSize:

      __MSchedConfigMINInt(S,PIndex,IVal,MMAX_GRIDTIMECOUNT,(int *)&MStat.P.TimeStepSize);

      break;

    case mcoSubmitFilter:

      MUStrCpy(MSched.ClientSubmitFilter,(char *)SVal,sizeof(MSched.ClientSubmitFilter));

      break;

    case mcoServerSubmitFilter:

      MUStrCpy(MSched.ServerSubmitFilter,(char *)SVal,sizeof(MSched.ServerSubmitFilter));

      break;

    case mcoUnsupportedDependencies:

      {
      char *ptr;
      char *TokPtr;

      int   dindex = 0;

      /* FORMAT:  dependency[,dependency]... */

      ptr = MUStrTok(SVal,",: \t\n|",&TokPtr);

      while ((ptr != NULL) && (dindex < MMAX_UNSUPPORTEDDEPENDLIST))
        {
        /* add ptr to S->UnsupportedDependencies[dindex] */

        MUStrDup(&S->UnsupportedDependencies[dindex++],ptr);

        S->UnsupportedDependencies[dindex] = '\0';
  
        ptr = MUStrTok(NULL,",: \t\n|",&TokPtr);
        }
      }      /* END BLOCK */

      break;

    case mcoRemapClass:

      {
      if (MClassAdd(SVal,&S->RemapC) == FAILURE)
        {
        MDB(3,fCONFIG) MLog("ALERT:    cannot add remap class '%s'\n",
          SVal);
        }
      else
        {
        MDB(3,fCONFIG) MLog("INFO:     remap class '%s' added\n",
          SVal);
        }
      }      /* END BLOCK */

      break;

    case mcoStrictProtocolCheck:

      MSched.StrictProtocolCheck = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRemapClassList:

      {
      char *ptr;
      char *TokPtr;

      int   cindex;

      /* FORMAT:  class[,class]... */

      cindex = 0;

      ptr = MUStrTok(SVal,",: \t\n|",&TokPtr);

      while (ptr != NULL)
        {
        if (cindex >= MMAX_CLASS)
          break;

        if (MClassAdd(ptr,&S->RemapCList[cindex]) == FAILURE)
          {
          MDB(3,fCONFIG) MLog("ALERT:    cannot add remap class '%s'\n",
            ptr);
          }
        else
          {
          MDB(3,fCONFIG) MLog("INFO:     remap class '%s' added\n",
            ptr);

          cindex++;
          }
  
        ptr = MUStrTok(NULL,",: \t\n|",&TokPtr);
        }

      if (cindex < MMAX_CLASS)
        S->RemapCList[cindex] = NULL;
      }      /* END BLOCK */

      break;

    case mcoRsvBucketDepth:
    case mcoRsvBucketQOSList:

      {
      int index;
      int bindex;
      int findex;
      int vindex;

      char *ptr;
      char *TokPtr;

      mqos_t *Q;

      findex = MMAX_QOS;

      if (IndexName[0] != '\0')
        {
        /* bucket name specified */

        for (bindex = 0;bindex < MMAX_QOSBUCKET;bindex++)
          {
          if (P->QOSBucket[bindex].Name[0] == '\0')
            break;

          if (P->QOSBucket[bindex].Name[0] == '\1')
            {
            findex = MIN(bindex,findex);

            continue;
            }
   
          if (!strcmp(IndexName,P->QOSBucket[bindex].Name))
            {
            /* bucket located */

            break;
            }
          }  /* END for (bindex) */
        }
      else
        {
        /* default bucket requested */

        bindex = 0;
        }

      if ((bindex >= MMAX_QOSBUCKET) && (findex >= MMAX_QOS))
        {
        MDB(0,fCONFIG) MLog("ALERT:    max QOS (%d) reached\n",
          MMAX_QOS);

        break;
        }

      if (bindex >= MMAX_QOSBUCKET)
        {
        bindex = findex;

        MUStrCpy(P->QOSBucket[bindex].Name,IndexName,sizeof(P->QOSBucket[bindex].Name));
        }

      switch (PIndex)
        {
        case mcoRsvBucketDepth:

          P->QOSBucket[bindex].RsvDepth = IVal;

          if (P->QOSBucket[bindex].Name[0] == '\0')
            strcpy(P->QOSBucket[bindex].Name,IndexName);

          break;

        case mcoRsvBucketQOSList:

          vindex = 0;

          /* FORMAT:  <QOS>[{ \t\n,:}<QOS>]... */

          for (index = 0;SArray[index] != NULL;index++)
            {
            if (!strcmp(SArray[index],ALL))
              {
              P->QOSBucket[bindex].List[vindex] = (mqos_t *)MMAX_QOS;

              vindex++;

              break;
              }

            ptr = MUStrTok(SArray[index],",: \t\n",&TokPtr);

            while (ptr != NULL)
              {
              if ((MQOSFind(ptr,&Q) == SUCCESS) ||
                  (MQOSAdd(ptr,&Q) == SUCCESS))
                {
                if (vindex >= (MMAX_QOS - 1))
                  {
                  MDB(0,fCONFIG) MLog("ALERT:    max QOS (%d) reached\n",
                    MMAX_QOS);

                  break;
                  }

                P->QOSBucket[bindex].List[vindex] = Q;

                vindex++;

                MDB(3,fCONFIG) MLog("INFO:     QOS '%s' added to %s[%d]\n",
                  Q->Name,
                  MParam[mcoRsvBucketQOSList],
                  bindex);
                }    /* END if (MQOSFind() == SUCCESS) */
              else
                {
                MDB(3,fCONFIG) MLog("WARNING:  cannot locate QOS '%s' for parameter %s\n",
                  SArray[index],
                  MParam[mcoRsvBucketQOSList]);
                }

              ptr = MUStrTok(NULL,",: \t\n",&TokPtr);
              }  /* END while (ptr != NULL) */
            }    /* END for (index) */

          P->QOSBucket[bindex].List[MIN(vindex,MMAX_QOS - 1)] = NULL;

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (PIndex) */
      }    /* END BLOCK */

      break;

    case mcoQOSDefaultOrder:

      __MSchedParseQOSDefaultOrder(SVal);

      break;

    case mcoResendCancelCommand:

      S->ResendCancelCommand = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRMMsgIgnore:

      S->RMMsgIgnore = MUBoolFromString(SVal,FALSE);

      break;

    case mcoRMPollInterval:

      {
      char *ptr;
      char *TokPtr;

      long tmpL;

      /* FORMAT:  [<MINPOLLINTERVAL>,]<MAXPOLLINTERVAL> */

      ptr = MUStrTok(SVal,",",&TokPtr);

      tmpL = MUTimeFromString(SVal);

      ptr = MUStrTok(NULL,",",&TokPtr);

      if (ptr == NULL)
        {
        S->MinRMPollInterval = 0;

        S->MaxRMPollInterval = tmpL;
        }
      else
        {
        S->MinRMPollInterval = tmpL;

        S->MaxRMPollInterval = MUTimeFromString(ptr);
        }
      }  /* END BLOCK (case mcoRMPollInterval) */

      break;

    case mcoRMRetryTimeCap:

      S->RMRetryTimeCap = MUTimeFromString(SVal);

      break;

    case mcoSchedToolsDir:

      /* prepend HomeDir if necessary */

      if ((SVal[0] != '~') && (SVal[0] != '/'))
        {
        sprintf(S->ToolsDir,"%s%s",
          S->InstDir,
          SVal);
        }
      else
        {
        MUStrCpy(S->ToolsDir,SVal,sizeof(S->ToolsDir));
        }

      if (SVal[strlen(SVal) - 1] != '/')
        {
        MUStrCat(S->ToolsDir,"/",sizeof(S->ToolsDir));
        }

      MUAddToPathList(S->ToolsDir);

      break;

    case mcoSchedLockFile:

      MUStrCpy(S->LockFile,SVal,sizeof(S->LockFile));

      break;

    case mcoSpoolDirKeepTime:

      MSched.SpoolDirKeepTime = MUTimeFromString(SVal);

      break;

    case mcoGEventTimeout:

      MSched.GEventTimeout = MUTimeFromString(SVal);

      break;

    case mcoStatDir:

      if ((SVal[0] != '~') && (SVal[0] != '/'))
        {
        sprintf(MStat.StatDir,"%s%s",
          S->VarDir,
          SVal);
        }
      else
        {
        MUStrCpy(MStat.StatDir,SVal,sizeof(MStat.StatDir));
        }

      if (SVal[strlen(SVal) - 1] != '/')
        MUStrCat(MStat.StatDir,"/",sizeof(MStat.StatDir));

      if (S->Reload == TRUE)
        {
        MStatOpenFile(S->Time,NULL);
        }

      break;

    case mcoSideBySide:

      S->SideBySide = MUBoolFromString(SVal,FALSE);

      break;

    case mcoStopIteration:

      S->StopIteration = IVal;

      break;

    case mcoSimTimePolicy:

      S->TimePolicy = (enum MTimePolicyEnum)MUGetIndexCI(
        SVal,
        MTimePolicy,
        FALSE,
        mtpNONE);

      break;

    case mcoSimTimeRatio:

      S->TimeRatio = DVal;

      break;

    case mcoAdminEAction:

      {
      char    tmpPathName[MMAX_LINE];
      mbool_t IsExe;

      MUStrCpy(S->Action[mactAdminEvent],SVal,sizeof(S->Action[mactAdminEvent]));

      if (S->Action[mactAdminEvent][0] == '/')
        {
        MUStrCpy(tmpPathName,S->Action[mactAdminEvent],sizeof(tmpPathName));
        }
      else if (S->ToolsDir[strlen(S->ToolsDir) - 1] == '/')
        {
        sprintf(tmpPathName,"%s%s",
          S->ToolsDir,
          S->Action[mactAdminEvent]);
        }
      else
        {
        sprintf(tmpPathName,"%s/%s",
          S->ToolsDir,
          S->Action[mactAdminEvent]);
        }

      if ((MFUGetInfo(tmpPathName,NULL,NULL,&IsExe,NULL) == FAILURE) ||
          (IsExe == FALSE))
        {
        MDB(2,fCONFIG) MLog("ALERT:    invalid action program '%s' requested\n",
          tmpPathName);

        MDB(2,fCONFIG) MLog("INFO:     disabling action program '%s'\n",
          S->Action[mactAdminEvent]);

        S->Action[mactAdminEvent][0] = '\0';

        MMBAdd(
          &MSched.MB,
          "notification program is invalid",
          NULL,
          mmbtAnnotation,
          MSched.Time + MCONST_DAYLEN,
          0,
          NULL);
        }
      }    /* END BLOCK */

      break;

    case mcoClientTimeout:

      __MSchedConfigMINLong(S,PIndex,MUTimeFromString(SVal),1000,&S->ClientTimeout);

      break;

    case mcoDisplayFlags:

      bmfromstring(SVal,MCDisplayType,&S->DisplayFlags);

      break;

    case mcoDisableScheduling:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        S->State = mssmPaused;

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoDisableScheduling],
        MBool[MUBoolFromString(SVal,FALSE)]);

      break;
      
    case mcoDisableSlaveJobSubmit:

      S->DisableSlaveJobSubmit = MUBoolFromString(SVal,FALSE);

      MDB(7,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoDisableSlaveJobSubmit],
        MBool[S->DisableSlaveJobSubmit]);

      break;

    case mcoEnableJobArrays:
  
      S->EnableJobArrays = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableJobArrays],
        MBool[S->EnableJobArrays]);

      break;

    case mcoEnableClusterMonitor:

      /* NYI */

      break;

    case mcoEnableCPJournal:

      MCP.UseCPJournal = MUBoolFromString(SVal,FALSE); 

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableCPJournal],
        MBool[MCP.UseCPJournal]);

      break;

    case mcoEnableEncryption:

      S->EnableEncryption = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableEncryption],
        MBool[S->EnableEncryption]);

      break;

    case mcoEnableFailureForPurgedJob:

      S->EnableFailureForPurgedJob = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableFailureForPurgedJob],
        MBool[S->EnableFailureForPurgedJob]);

      break;

    case mcoEnableFastSpawn:

      S->EnableFastSpawn = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableFastSpawn],
        MBool[S->EnableFastSpawn]);

      break;

    case mcoEnableHighThroughput:

      S->EnableHighThroughput = MUBoolFromString(SVal,FALSE);

      if (S->EnableHighThroughput == TRUE)
        {
        /* Comment these out until we've individually tested each of the
         * following parameters one at a time--they may not be safe
         * changes! */

        S->DisableMRE = TRUE;

        /* the below variables are probably safe--Yahoo has been using
         * them for months */
        S->NoEnforceFuturePolicy = TRUE;
        S->EnableFastNativeRM = TRUE;
        S->EnableFastSpawn = TRUE;

        S->UseMoabJobID = TRUE;

        if (MSched.InternalRM != NULL)
          {
          MSched.InternalRM->SyncJobID = TRUE;
          MSched.InternalRM->JobIDFormatIsInteger = TRUE;
          }
        }

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableHighThroughput],
        MBool[S->EnableHighThroughput]);

      break;

    case mcoEnableFastNodeRsv:

      S->EnableFastNodeRsv = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableFastNodeRsv],
        MBool[S->EnableFastNodeRsv]);

    case mcoForkIterations:

      S->ForkIterations = IVal;

      break;

    case mcoEnableHPAffinityScheduling:

      S->EnableHPAffinityScheduling = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableHPAffinityScheduling],
        MBool[S->EnableHPAffinityScheduling]);

      break;

    case mcoEnableImplicitStageOut:

      S->EnableImplicitStageOut = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableImplicitStageOut],
        MBool[S->EnableImplicitStageOut]);

      break;

    case mcoEnableNSNodeAddrLookup:

      S->EnableNodeAddrLookup = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableNSNodeAddrLookup],
        MBool[S->EnableNodeAddrLookup]);

      break;

    case mcoEnableJobEvalCheckStart:

      S->EnableJobEvalCheckStart = MUBoolFromString(SVal,FALSE);

      MDB(7,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableJobEvalCheckStart],
        MBool[S->EnableJobEvalCheckStart]);

      break;

    case mcoEnableUnMungeCredCheck:

      S->EnableUnMungeCredCheck = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoEnableUnMungeCredCheck],
        MBool[S->EnableUnMungeCredCheck]);

      break;

    case mcoEnforceAccountAccess:
 
      S->EnforceAccountAccess = MUBoolFromString(SVal,FALSE);

      break;

    case mcoEnforceGRESAccess:

      S->EnforceGRESAccess = MUBoolFromString(SVal,FALSE);

      break;

    case mcoEvalMetrics:

      S->EvalMetrics = 1;

      break;

    case mcoEventFileRollDepth:

      MStat.EventFileRollDepth = IVal;

      break;

    case mcoEventLogWSPassword:

      if (MSched.WebServicesConfigured == TRUE)
        MUStrDup(&S->EventLogWSPassword,SVal);
      else
        {
        MDB(2,fCONFIG) MLog("ERROR:     Web Services is not enabled in this build of moab, cannot set event log web service URL\n");
        return(FAILURE);
        }

      break;

    case mcoEventLogWSURL:

      if (MSched.WebServicesConfigured == TRUE)
        MUStrDup(&S->EventLogWSURL,SVal);
      else
        {
        MDB(2,fCONFIG) MLog("ERROR:     Web Services is not enabled in this build of moab, cannot set event log web service URL\n");
        return(FAILURE);
        }

      break;

    case mcoEventLogWSUser:

      if (MSched.WebServicesConfigured == TRUE)
        MUStrDup(&S->EventLogWSUser,SVal);
      else
        {
        MDB(2,fCONFIG) MLog("ERROR:     Web Services is not enabled in this build of moab, cannot set event log web service URL\n");
        return(FAILURE);
        }

      break;

    case mcoGEventCfg:

      if ((SArray != NULL) && (SArray[0] != '\0'))
        {
        char *ptr;

        char *TokPtr;

        char  OName[MMAX_NAME];
        char  TVal[MMAX_LINE];
        enum MCompEnum   CIndex;    /* comparison index */
 
        int   gindex = 0;
        int   sindex;

        mbool_t IsGEvent;

        /* NOTE:  can specify gevent or gmetric */

        /* FORMAT: INDEXNAME={GEVENT|GMETRIC{cmp}THRESHOLD} */

        MUParseComp(IndexName,OName,&CIndex,TVal,sizeof(TVal),TRUE);

        if (CIndex == mcmpNONE)
          {
          IsGEvent = TRUE;

          /* Add/update a GEvent Description */

          MGEventAddDesc(OName);
          }
        else
          {
          IsGEvent = FALSE;

          if ((gindex = MUMAGetIndex(meGMetrics,OName,mAdd)) <= 0)
            {
            /* cannot add requested gevent */

            break;
            }

          MGMetric.GMetricThreshold[gindex] = strtod(TVal,NULL);
          MGMetric.GMetricCmp[gindex]       = CIndex;
          }

        /* FORMAT:  GEVENTCFG[X] ACTION=<A>[:<MODIFIER>][,<A>[:<MODIFIER>]]... */

        for (sindex = 0;SArray[sindex] != NULL;sindex++)
          {
          mgevent_desc_t *GEventDesc = NULL;

          ptr = SArray[sindex];

          if (IsGEvent == TRUE)
            {
            /* By definition of 'IsGEvent' being TRUE, the entry should be in the table */
            MGEventGetDescInfo(OName,&GEventDesc);
            }

          if (!strncasecmp(ptr,"action=",strlen("action=")))
            {
            char *ptr2;
            char *TokPtr2;

            char  tmpLine[MMAX_LINE];
            
            int   aindex;

            ptr += strlen("action=");

            MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

            /* initialize gevent/gmetric */

            if (IsGEvent == TRUE)
              {
              bmclear(&GEventDesc->GEventAction);
              }
            else
              bmclear(&MGMetric.GMetricAction[gindex]);

            /* process new values */

            ptr = MUStrTok(tmpLine,",",&TokPtr);

            while (ptr != NULL)
              {
              if (IsGEvent == TRUE)
                {
                ptr2 = MUStrTok(ptr,":",&TokPtr2);

                aindex = MUGetIndexCI(ptr2,MGEAction,MBNOTSET,0);

                if (aindex > 0)
                  {
                  bmset(&GEventDesc->GEventAction,aindex);
                  }

                ptr2 = MUStrTok(NULL,",",&TokPtr2);

                if (ptr2 != NULL)
                  {
                  MUStrDup(&GEventDesc->GEventActionDetails[aindex],ptr2);
                  }
                }
              else
                {
                ptr2 = MUStrTok(ptr,":",&TokPtr2);

                aindex = MUGetIndexCI(ptr2,MGEAction,MBNOTSET,0);

                if (aindex > 0)
                  bmset(&MGMetric.GMetricAction[gindex],aindex);

                ptr2 = MUStrTok(NULL,",",&TokPtr2);

                if (ptr2 != NULL)
                  {
                  /* record action modifier */

                  MUStrDup(&MGMetric.GMetricActionDetails[gindex][aindex],ptr2);
                  }
                }   /* END else */

              ptr = MUStrTok(NULL,",",&TokPtr);
              }     /* END while (ptr2 != NULL) */
            }       /* END if (!strncasecmp(ptr,"action=",strlen("action="))) */
          else if (!strncasecmp(ptr,"rearm=",strlen("rearm=")))
            {
            ptr += strlen("rearm=");

            if (IsGEvent == TRUE)
              {
              GEventDesc->GEventReArm = MUTimeFromString(ptr);
              }
            else 
              MGMetric.GMetricReArm[gindex] = MUTimeFromString(ptr);
            }
          else if (!strncasecmp(ptr,"ecount=",strlen("ecount=")))
            {
            ptr += strlen("ecount=");

            if (IsGEvent == TRUE)
              {
              /* threshold will be compared with count to determine when to initiate action */
              GEventDesc->GEventAThreshold = strtol(ptr,NULL,10);
              }
            }
          else if (!strncasecmp(ptr,"severity=",strlen("severity=")))
            {
            ptr += strlen("severity=");

            if (IsGEvent == TRUE)
              {
              int tmpSev = strtol(ptr,NULL,10);

              if ((tmpSev > 0) && (tmpSev <= MMAX_GEVENTSEVERITY))
                {
                GEventDesc->Severity = tmpSev;
                }
              }
            }
          }  /* END for (sindex) */
        }    /* END if ((SArray != NULL) && (SArray[0] != NULL)) */

      break;

    case mcoGMetricCfg:

      {
      int gindex;  /* generic metric index */
      int sindex;

      char *ptr;
      char *ptr2;

      if ((gindex = MUMAGetIndex(meGMetrics,IndexName,mAdd)) <= 0)
        {
        /* cannot add requested gevent */

        break;
        }

      for (sindex = 0;SArray[sindex] != NULL;sindex++)
        {
        ptr = SArray[sindex];

        if (!strncasecmp(ptr,"chargerate=",strlen("chargerate=")))
          {
          ptr2 = ptr + strlen("chargerate=");

          MGMetric.GMetricChargeRate[gindex] = strtod(ptr2,NULL);
          }
        else if (!strncasecmp(ptr,"iscummulative=",strlen("iscummulative=")))
          {
          ptr2 = ptr + strlen("iscummulative=");

          MGMetric.GMetricIsCummulative[gindex] = MUBoolFromString(ptr2,TRUE);
          }
        }    /* END for (sindex) */
      }      /* END BLOCK (case mcoGMetricCfg) */

      break;

    case mcoGResCfg:

      if ((SVal != NULL) && (SVal[0] != '\0'))
        {
        int   gindex;

        if ((gindex = MUMAGetIndex(meGRes,IndexName,mAdd)) <= 0)
          {
          /* cannot add requested gres */

          break;
          }

        MGResProcessConfig(gindex,SVal);
        }    /* END if ((SVal != NULL) && (SVal[0] != '\0')) */

      break;

    case mcoGResToJobAttrMap:

      {
      int index;
      int sindex;

      char *ptr;
      char *TokPtr;

      if (SArray != NULL)
        {
        for (sindex = 0;SArray[sindex] != NULL;sindex++)
          {
          ptr = MUStrTok(SArray[sindex],", \t\n:;",&TokPtr);

          while (ptr != NULL)
            {
            if ((index = MUMAGetIndex(meGRes,ptr,mAdd)) <= 0)
              {
              /* can't add new gres, must have hit limit */
   
              MDB(3,fCONFIG) MLog("ERROR:    could not add GRESTOJOBATTRMAP '%s'\n",
                ptr);
   
              break;
              }

            MGRes.GRes[index]->JobAttrMap = TRUE;

            MDB(3,fCONFIG) MLog("INFO:     gres to job attr mapping '%s' added\n",
              ptr);

            ptr = MUStrTok(NULL,", \t\n:;",&TokPtr);
            }  /* END while (ptr != NULL) */
          }    /* END for (sindex) */
        }      /* END if (SArray != NULL) */
      }        /* END BLOCK (case mcoGResToJobAttrMap) */

      break;

    case mcoJobExtendStartWallTime:

      MSched.JobExtendStartWallTime = MUBoolFromString(SVal,FALSE);

      break;

    case mcoJobFBAction:

      {
      char    tmpPathName[MMAX_LINE];
      mbool_t IsExe;

      MUStrCpy(S->Action[mactJobFB],SVal,sizeof(S->Action[mactJobFB]));

      if (S->Action[mactJobFB][0] == '/')
        {
        MUStrCpy(tmpPathName,S->Action[mactJobFB],sizeof(tmpPathName));
        }
      else if (S->ToolsDir[strlen(S->ToolsDir) - 1] == '/')
        {
        sprintf(tmpPathName,"%s%s",
          S->ToolsDir,
          S->Action[mactJobFB]);
        }
      else
        {
        sprintf(tmpPathName,"%s/%s",
          S->ToolsDir,
          S->Action[mactJobFB]);
        }

      if ((MFUGetInfo(tmpPathName,NULL,NULL,&IsExe,NULL) == FAILURE) ||
          (IsExe == FALSE))
        {
        MDB(2,fCONFIG) MLog("ALERT:    invalid job FB program '%s' requested\n",
          tmpPathName);

        MDB(2,fCONFIG) MLog("INFO:     disabling job FB program '%s'\n",
          S->Action[mactJobFB]);

        S->Action[mactJobFB][0] = '\0';
        }
      }    /* END BLOCK */

      break;

    case mcoMailAction:

      /* FORMAT:  [<MAILPROGRAM>][@<MAILDOMAIN>] */

      if (strchr(SVal,'@') == NULL)
        {
        strcpy(S->Action[mactMail],SVal); 

        break;
        }
      else if (SVal[0] == '@')
        {
        MUStrDup(&S->DefaultMailDomain,SVal + 1);
        }
      else
        {
        char *tok1;
        char *tok2;

        char *TokPtr;
        
        tok1 = MUStrTok(SVal,"@",&TokPtr);
        tok2 = MUStrTok(NULL,"@",&TokPtr);

        strcpy(S->Action[mactMail],tok1);
        MUStrDup(&S->DefaultMailDomain,tok2);
        }

      break;

    case mcoMCSocketProtocol:

      S->DefaultMCSocketProtocol = (enum MSocketProtocolEnum)MUGetIndexCI(
        SVal,
        MSockProtocol,
        FALSE,
        0);

      break;

    case mcoRsvReallocPolicy:

      S->DefaultRsvAllocPolicy = (enum MRsvAllocPolicyEnum)MUGetIndexCI(
        SVal,
        (const char **)MRsvAllocPolicy,
        FALSE,
        0);

      break;

    case mcoRsvLimitPolicy:

      S->RsvLimitPolicy = (enum MPolicyTypeEnum)MUGetIndexCI(
        SVal,
        (const char **)MPolicyMode,
        FALSE,
        0);

      break;

    case mcoAdminUsers:
    case mcoAdmin1Users:
    case mcoAdmin2Users:
    case mcoAdmin3Users:
    case mcoAdmin4Users:

      /* deprecated - use ADMINCFG */

      {
      int aindex;
      int sindex;

      int uindex;
      int uindex2;

      char *ptr;
      char *TokPtr;

      int   AIndex;

      madmin_t *A;

      enum MRoleEnum RIndex;

      char  OldUList[MMAX_ADMINUSERS + 1][MMAX_NAME];

      switch (PIndex)
        {
        case mcoAdminUsers:
        case mcoAdmin1Users:

          RIndex = mcalAdmin1;
          AIndex = 1;

          break;

        case mcoAdmin2Users:

          RIndex = mcalAdmin2;
          AIndex = 2;

          break;

        case mcoAdmin3Users:

          RIndex = mcalAdmin3;
          AIndex = 3;

          break;

        case mcoAdmin4Users:

          RIndex = mcalAdmin4;
          AIndex = 4;

          break;

        default:

          /* unreachable */

          RIndex = mcalNONE;
          AIndex = 0;

          break;
        }  /* END switch (PIndex) */

      aindex = 0;

      /* FORMAT:  <UNAME>[{ ,\t}<UNAME>]... */

      A = &MSched.Admin[AIndex];

      memcpy(OldUList,A->UName,sizeof(OldUList));

      memset(A->UName,0,sizeof(A->UName));

      for (sindex = 0;SArray[sindex] != NULL;sindex++)
        {
        ptr = MUStrTok(SArray[sindex],", \t",&TokPtr);

        while (ptr != NULL)
          {
          mgcred_t *U;

          if (!strstr(ptr,"PEER:"))
            {
            /* update user record for non-peer admin specification */

            if (MUserAdd(ptr,&U) == SUCCESS)
              U->Role = RIndex;
            }

          MUStrCpy(
            A->UName[aindex],
            ptr,
            MMAX_NAME);

          MDB(3,fCONFIG) MLog("INFO:     %s '%s' added\n",
            MParam[PIndex],
            ptr);

          aindex++;

          if (aindex == MMAX_ADMINUSERS)
            {
            MDB(0,fCONFIG) MLog("ALERT:    max admin users (%d) reached\n",
              MMAX_ADMINUSERS);

            break;
            }

          if (aindex >= MMAX_ADMINUSERS)
            break;

          ptr = MUStrTok(NULL,", \t",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END for (sindex) */

      for (uindex2 = 0;uindex2 < MMAX_ADMINUSERS;uindex2++)
        {
        if (OldUList[uindex2][0] == '\0')
          break;

        if (strncasecmp(OldUList[uindex2],"peer:",strlen("peer:")))
          continue;

        for (uindex = 0;uindex < MMAX_ADMINUSERS;uindex++)
          {
          if (!strcasecmp(A->UName[uindex],OldUList[uindex2]))
            break;

          if (A->UName[uindex][0] == '\0')
            {
            strcpy(A->UName[uindex],OldUList[uindex2]);

            break;
            }
          }  /* END for (uindex) */
        }    /* END for (uindex2) */
      }      /* END BLOCK */

      break;

    case mcoAdjUtlRes:

      S->AdjustUtlRes = MUBoolFromString(SVal,FALSE);

      break;

    case mcoSubmitHosts:

      {
      /* NOTE:  space delimited hosts only 
         (must add support for comma delimited) */

      int aindex;

      for (aindex = 0;SArray[aindex] != NULL;aindex++)
        {
        if (aindex == MMAX_SUBMITHOSTS)
          {
          MDB(0,fCONFIG) MLog("ALERT:    max admin hosts (%d) reached\n",
            MMAX_SUBMITHOSTS);

          break;
          }

        MUStrCpy(S->SubmitHosts[aindex],SArray[aindex],sizeof(S->SubmitHosts[aindex]));

        MDB(3,fCONFIG) MLog("INFO:     submit host '%s' added\n",
          S->SubmitHosts[aindex]);
        }    /* END for (aindex) */

      S->SubmitHosts[aindex][0] = '\0';
      }  /* END BLOCK */

      break;

    case mcoMServerHomeDir:

      MSchedSetAttr(S,msaHomeDir,(void **)SVal,mdfString,mSet);

      break;

    case mcoServerHost:

      MUStrCpy(S->ServerHost,SVal,sizeof(S->ServerHost));

      break;

    case mcoServerName:

      MUStrCpy(S->Name,SVal,sizeof(S->Name));

      break;

    case mcoServerPort:

      S->ServerPort = IVal;

      break;

    case mcoMountPointCheckInterval:

      MSched.MountPointCheckInterval = MUTimeFromString(SVal);

      if (MSched.MountPointUsageThreshold <= 0)
        MSched.MountPointUsageThreshold = 0.9f;  /* default's to 90% if not already set */

      break;

    case mcoMountPointCheckThreshold:

      if ((IVal <= 0) || (IVal > 100))
        break;  /* invalid value */

      MSched.MountPointUsageThreshold = (float)IVal / 100.0f;  /* convert to percentage */

      break;

    case mcoCheckPointDir:

      MSchedSetAttr(S,msaCheckpointDir,(void **)SVal,mdfString,mSet);

      break;

    case mcoSpoolDir:

      if ((SVal[0] != '~') && (SVal[0] != '/'))
        {
        snprintf(S->SpoolDir,sizeof(S->SpoolDir),"%s/%s",
          S->VarDir,
          SVal);
        }
      else
        {
        MSchedSetAttr(S,msaSpoolDir,(void **)SVal,mdfString,mSet);
        }

      break;

    case mcoLogDir:

      {
      char tmpLine[MMAX_LINE];

      /* prepend VarDir if necessary */

      if ((SVal[0] != '~') && (SVal[0] != '/'))
        {
        snprintf(S->LogDir,sizeof(S->LogDir),"%s%s",
          S->VarDir,
          SVal);
        }
      else
        {
        MUStrCpy(S->LogDir,SVal,sizeof(S->LogDir));
        }

      if (SVal[strlen(SVal) - 1] != '/')
        {
        MUStrCat(S->LogDir,"/",sizeof(S->LogDir));
        }

      if (S->LogFile[0] == '\0')
        {
        snprintf(tmpLine,sizeof(tmpLine),"%s%s%s",
          S->LogDir,
          MSCHED_SNAME,
          MDEF_LOGSUFFIX);

        MLogInitialize(tmpLine,-1,S->Iteration);
        }
      else if (S->LogFile[0] == '/')
        {
        /* NO-OP */
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"%s%s",
          S->LogDir,
          S->LogFile);

        MLogInitialize(tmpLine,-1,S->Iteration);
        } 
      }    /* END BLOCK */
 
      break;

    case mcoLogFile:

      {
      char tmpLine[MMAX_LINE];

      MUStrCpy(S->LogFile,SVal,sizeof(S->LogFile));

      if ((S->LogFile[0] == '/') || (S->LogDir[0] == '\0'))
        {
        MUStrCpy(tmpLine,S->LogFile,sizeof(tmpLine));
        }
      else
        {
        MUStrCpy(tmpLine,S->LogDir,sizeof(tmpLine));

        if (S->LogDir[strlen(S->LogDir) - 1] != '/')
          {
          MUStrCat(tmpLine,"/",sizeof(tmpLine));
          }

        MUStrCat(tmpLine,S->LogFile,sizeof(tmpLine));
        }

      MLogInitialize(tmpLine,-1,S->Iteration);

      MDB(0,fALL) MLog("INFO:     starting %s version %s ##################\n",
        MSCHED_NAME,
        MOAB_VERSION);

      /* add moab extension info */

      MDB(3,fALL) MLog("INFO:     build info: %s\n",
        MSysShowBuildInfo());
      }  /* END BLOCK */

      break;

    case mcoMaxGMetric:

      /* only allow update before scheduling begins */

      if (MSched.Iteration != -1)
        break;

      MSched.M[mxoxGMetric] = IVal;

      break;

    case mcoMaxGRes:

#ifndef MCONSTGRES
      /* only allow update before scheduling begins */

      if (MSched.Iteration != -1)
        break;

      MSched.M[mxoxGRes] = IVal;
#endif

      break;

    case mcoMaxJob:

      /* only allow update before scheduling begins */

      if (MSched.Iteration != -1)
        break;

      if (MSysAllocJobTable(IVal) == FAILURE)
        {
        exit(1);
        }

      break;

    case mcoMaxNode:

      /* only allow update before scheduling begins */

      if (MSched.Iteration != -1)
        break;

      if (MSysAllocNodeTable(IVal) == FAILURE)
        {
        exit(1);
        }

      break;

    case mcoMaxRsvPerNode:

      /* FORMAT:  <COMPUTE_LIMIT>[,<GLOBALNODE_LIMIT>] */

      /* only allow update before scheduling begins */

      if (MSched.Iteration != -1)
        break;

      {
      char *ptr;
      char *TokPtr;

      ptr = MUStrTok(SVal,", \t\n:",&TokPtr);

      if (ptr != NULL)
        {
        __MSchedConfigMINInt(S,PIndex,(int)strtol(ptr,NULL,10),MMAX_RSV_DEPTH,&S->MaxRsvPerNode);
        }  /* END if (ptr != NULL) */

      ptr = MUStrTok(NULL,", \t\n:",&TokPtr);

      if (ptr != NULL)
        {
        __MSchedConfigMINInt(S,PIndex,(int)strtol(ptr,NULL,10),MMAX_RSV_GDEPTH,&S->MaxRsvPerGNode);
        }  /* END if (ptr != NULL) */
      }    /* END BLOCK (case mcoMaxRsvPerNode) */

      break;

    case mcoMWikiSubmitTimeout:

      /*Set the wiki job submit timeout in seconds */

      S->MWikiSubmitTimeout = IVal;

      /* enforce reasonable limits on this timeout */

      if ((S->MWikiSubmitTimeout > 60) || (S->MWikiSubmitTimeout < 5))
        {
        S->MWikiSubmitTimeout = 10;
        }

      break;

    case mcoNAPolicy:

      S->DefaultNAccessPolicy = (enum MNodeAccessPolicyEnum)MUGetIndexCI(
        SVal,
        MNAccessPolicy,
        FALSE,
        MDEF_NACCESSPOLICY);

      break;

    case mcoNodeToJobFeatureMap:

      {
      int index;
      int sindex;

      char *ptr;
      char *TokPtr;

      index = 0;

      if (SArray != NULL)
        {
        for (sindex = 0;SArray[sindex] != NULL;sindex++)
          {
          ptr = MUStrTok(SArray[sindex],", \t\n:;",&TokPtr);

          while ((ptr != NULL) && (index < MMAX_ATTR ))
            {
            MUStrCpy(
              MSched.NodeToJobAttrMap[index++],
              ptr,
              sizeof(MSched.NodeToJobAttrMap[0]));

            MDB(3,fCONFIG) MLog("INFO:     node to job feature mapping '%s' added\n",
              ptr);

            ptr = MUStrTok(NULL,", \t\n:;",&TokPtr);
            }  /* END while (ptr != NULL) */
          }    /* END for (sindex) */
        }      /* END if (SArray != NULL) */
      }        /* END BLOCK */

      break;

    case mcoNodeTypeFeatureHeader:

      MUStrCpy(
        S->NodeTypeFeatureHeader,
        SVal,
        sizeof(S->NodeTypeFeatureHeader));

      break;

    case mcoNoWaitPreemption:

      S->NoWaitPreemption = MUBoolFromString(SVal,FALSE);

      break;

    case mcoPartitionFeatureHeader:

      MUStrCpy(
        S->PartitionFeatureHeader,
        SVal,
        sizeof(S->PartitionFeatureHeader));

      break;

    case mcoPreemptJobCount:

      __MSchedConfigMINInt(S,PIndex,MAX(1,IVal),MMAX_PJOB,&S->SingleJobMaxPreemptee);

      break;

    case mcoPreemptPolicy:

      S->PreemptPolicy = (enum MPreemptPolicyEnum)MUGetIndexCI(
        SVal,
        (const char **)MPreemptPolicy,
        FALSE,
        0);

      break;

    case mcoPreemptSearchDepth:

      S->SerialJobPreemptSearchDepth = IVal;

      break;

    case mcoPreemptPrioJobSelectWeight:

      S->PreemptPrioWeight = DVal;

      break;

    case mcoPreemptRTimeWeight:

      S->PreemptRTimeWeight = DVal;

      break;

    case mcoProcSpeedFeatureHeader:

      MUStrCpy(
        S->ProcSpeedFeatureHeader,
        SVal,
        sizeof(S->ProcSpeedFeatureHeader));

      break;

    case mcoRackFeatureHeader:

      MUStrCpy(
        S->RackFeatureHeader,
        SVal,
        sizeof(S->RackFeatureHeader));

      break;

    case mcoSlotFeatureHeader:

      MUStrCpy(
        S->SlotFeatureHeader,
        SVal,
        sizeof(S->SlotFeatureHeader));

      break;

    case mcoRemoteFailTransient:

      S->RemoteFailTransient = MUBoolFromString(SVal,FALSE);

      break;

    case mcoSchedMode:

      S->Mode = (enum MSchedModeEnum)MUGetIndexCI(SVal,MSchedMode,FALSE,S->Mode);

      S->SpecMode = S->Mode;

      if (S->Mode == msmSlave)
        MSched.State = mssmPaused;

      break;

    case mcoSchedPolicy:

      S->SchedPolicy = (enum MSchedPolicyEnum)MUGetIndexCI(
        SVal,
        MSchedPolicy,
        FALSE,
        mscpNONE);

      break;

    case mcoSchedStepCount:

      S->StepCount = IVal;

      break;

    case mcoSocketExplicitNonBlock:

      S->SocketExplicitNonBlock = MUBoolFromString(SVal,FALSE);

      break;

    case mcoSocketLingerVal:

      S->SocketLingerVal = IVal;

      break;

    case mcoSocketWaitTime:

      S->SocketWaitTime = IVal;

      break;

    case mcoStatProfCount:

      if (MSched.Iteration == -1)
        {
        /* only allow parameter modification at startup */

        MStat.P.MaxIStatCount = IVal;
        }

      break;

    case mcoStatProfDuration:

      if (MSched.Iteration == -1)
        {
        mbool_t Reconfig = FALSE;

        /* only allow parameter modification at startup */

        MStat.P.IStatDuration = MUTimeFromString(SVal);

        if (MStat.P.IStatDuration == 0)
          {
          MStat.P.IStatDuration = MDEF_PSTEPDURATION;
          }

        while (MCONST_DAYLEN % MStat.P.IStatDuration != 0)
          {
          MStat.P.IStatDuration--;

          Reconfig = TRUE;
          }

        MStatPInitialize(NULL,TRUE,msoCred);

        if (Reconfig == TRUE)
          {
          MDB(1,fCONFIG) MLog("WARNING:  invalid PROFILEDURATION specified, modified internally to %ld (see documentation)\n",
            (long)MStat.P.IStatDuration);
          }
        }

      break;

    case mcoStatProfGranularity:

      /* FORMAT:  <DOUBLE>[%] */

      {
      double tmpD;

      tmpD = strtod(SVal,NULL);

      if (tmpD <= 0.0)
        tmpD = 0.0;

      /* NOTE:   1% maps to .99 -> 1.01 
                10% maps to .90 -> 1.10
               100% maps to .00 -> 2.00

         (fix low-end threshold calculation) 
      */
      
      MSched.StatMaxThreshold = 1.0 + tmpD / 100.0;
      MSched.StatMinThreshold = 1.0 - tmpD / 100.0;
      }

      break;

    case mcoStoreSubmission:

      S->StoreSubmission = MUBoolFromString(SVal,FALSE);

      if (S->StoreSubmission == TRUE)
        {
        char SubDir[MMAX_PATH_LEN];

        strncpy(SubDir,MStat.StatDir,sizeof(SubDir));

        if (SubDir[strlen(SubDir) - 1] != '/')
          strcat(SubDir,"/");

        strcat(SubDir,"jobarchive");

        MOMkDir(SubDir,MCONST_ARCHIVEDIRPERM);
        }

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoStoreSubmission],
        MBool[S->StoreSubmission]);

      break;

    case mcoTrackSuspendedJobUsage:

      S->TrackSuspendedJobUsage = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoTrackSuspendedJobUsage],
        MBool[S->TrackSuspendedJobUsage]);

      break;

    case mcoTrackUserIO:

      S->TrackUserIO = MUBoolFromString(SVal,FALSE);

      MDB(2,fALL) MLog("INFO:     %s set to %s\n",
        MParam[mcoTrackUserIO],
        MBool[S->TrackUserIO]);

      break;

    case mcoTrigCheckTime:

      {
      char *ptr;
      char *TokPtr;

      long tmpL;

      /* FORMAT:  [<MAXTRIGCHECKTOTALTIME>,]<MAXTRIGCHECKSINGLETIME> */

      ptr = MUStrTok(SVal,",",&TokPtr);

      tmpL = MUTimeFromString(SVal);

      ptr = MUStrTok(NULL,",",&TokPtr);

      if (ptr == NULL)
        {
        S->MaxTrigCheckTotalTime = MCONST_EFFINF;

        S->MaxTrigCheckSingleTime = tmpL;
        }
      else
        {
        S->MaxTrigCheckTotalTime = tmpL;

        S->MaxTrigCheckSingleTime = MUTimeFromString(ptr);
        }
      }  /* END BLOCK (case mcoTrigCheckTime) */

      break;

    case mcoTrigEvalLimit:

      S->TrigEvalLimit = MAX(0,(int)strtol(SVal,NULL,10));

      break;

    case mcoTrigFlags:

      bmfromstring((char *)SVal,MTrigPolicyFlag,&S->TrigPolicyFlags);

      break;

    case mcoThreadPoolSize:

#ifdef __NOMCOMMTHREAD
      /* no thread pools if there are no threads! */
      {
      char Msg[MMAX_LINE];

      snprintf(Msg,sizeof(Msg),
          "ERROR:    Cannot configure thread pool: Moab was built without threading.");

      MDB(1,fCONFIG) MLog("%s",
        Msg);

      MMBAdd(
        &S->MB,
        Msg,
        NULL,
        mmbtOther,
        0,
        0,
        NULL);

      return (FAILURE);
      }
#endif  /* __NOMCOMMTHREAD */


      /*Set thread pool size */

      S->TPSize = IVal;

      if(S->TPSize > MMAX_TP_HANDLER_THREADS)
        {
        char Msg[MMAX_LINE];

        /* size cannot exceed a hard-coded limit for practical reasons */

        snprintf(Msg,sizeof(Msg),"Requested thread pool size %d is too high."
          "Size cannot exceed %d.\n",
          S->TPSize,
          MMAX_TP_HANDLER_THREADS);

        MDB(1,fCONFIG) MLog("%s",
          Msg);

        MMBAdd(
          &S->MB,
          Msg,
          NULL,
          mmbtOther,
          0,
          0,
          NULL);

        S->TPSize = MMAX_TP_HANDLER_THREADS;
        }

      break;

    case mcoUMask:

      S->UMask = (int)strtol(SVal,NULL,0);  /* NOTE:  allow octal/decimal spec */

      umask(S->UMask);

      break;

    case mcoUseAnyPartitionPrio:

      MSched.UseAnyPartitionPrio = MUBoolFromString(SVal,FALSE);

      break;

    case mcoUseCPRsvNodeList:

      S->UseCPRsvNodeList = MUBoolFromString(SVal,FALSE);

      break;
 
    case mcoUseMoabJobID:

      S->UseMoabJobID = MUBoolFromString(SVal,FALSE);

      break;

    case mcoUseSyslog:
 
      {
      char *ptr;
      char *TokPtr;

      /* FORMAT:  <BOOL>:<FACILITY> */

      if ((ptr = MUStrTok(SVal,":",&TokPtr)) == NULL)
        break;

      S->UseSyslog = MUBoolFromString(ptr,FALSE);

      if ((ptr = MUStrTok(NULL,":",&TokPtr)) == NULL)
        {
        MOSSyslogInit(S);

        break;
        }

      if (!strcasecmp(ptr,"local0"))
        {
        #ifdef LOG_LOCAL0
          S->SyslogFacility = LOG_LOCAL0;
        #endif /* LOG_LOCAL0 */
        }
      else if (!strcasecmp(ptr,"local1"))
        {
        #ifdef LOG_LOCAL1
          S->SyslogFacility = LOG_LOCAL1;
        #endif /* LOG_LOCAL1 */
        }
      else if (!strcasecmp(ptr,"local2"))
        {
        #ifdef LOG_LOCAL2
          S->SyslogFacility = LOG_LOCAL2;
        #endif /* LOG_LOCAL2 */
        }
      else if (!strcasecmp(ptr,"local3"))
        {
        #ifdef LOG_LOCAL3
          S->SyslogFacility = LOG_LOCAL3;
        #endif /* LOG_LOCAL3 */
        }
      else if (!strcasecmp(ptr,"local4"))
        {
        #ifdef LOG_LOCAL4
          S->SyslogFacility = LOG_LOCAL4;
        #endif /* LOG_LOCAL4 */
        }
      else if (!strcasecmp(ptr,"local5"))
        {
        #ifdef LOG_LOCAL5
          S->SyslogFacility = LOG_LOCAL5;
        #endif /* LOG_LOCAL5 */
        }
      else if (!strcasecmp(ptr,"local6"))
        {
        #ifdef LOG_LOCAL6
          S->SyslogFacility = LOG_LOCAL6;
        #endif /* LOG_LOCAL6 */
        }
      else if (!strcasecmp(ptr,"local7"))
        {
        #ifdef LOG_LOCAL7
          S->SyslogFacility = LOG_LOCAL7;
        #endif /* LOG_LOCAL7 */
        }
      else if (!strcasecmp(ptr,"user"))
        {
        #ifdef LOG_USER
          S->SyslogFacility = LOG_USER;
        #endif /* LOG_USER */
        }
      else if (!strcasecmp(ptr,"daemon"))
        {
        #ifdef LOG_DAEMON
          S->SyslogFacility = LOG_DAEMON;
        #endif /* LOG_DAEMON */
        }
      else
        { 
        S->SyslogFacility = (int)strtol(ptr,NULL,10);
        }

      MOSSyslogInit(S);
      }  /* END BLOCK */

      break;

    case mcoUseJobRegEx:

      S->UseJobRegEx = MUBoolFromString(SVal,FALSE);

      break;

    case mcoUseUserHash:

      S->UseUserHash = MUBoolFromString(SVal,FALSE);

      break;

    case mcoVAddressList:

      {
      char *ptr;
      char *Hostname;
      char *Addr;
      char *TokPtr;
      char *TokPtr2;
      mnode_t *GN;
      
      int   NumAddrs = 0;
      char tmpName[MMAX_NAME];

      /* FORMAT: <hostname>:<addr>[,<hostname>:<addr> ...] */

      ptr = MUStrTok(SVal,",",&TokPtr);

      while (ptr != NULL)
        {
        Hostname = MUStrTok(ptr,":",&TokPtr2);
        Addr = MUStrTok(NULL,":",&TokPtr2);

        if ((Hostname == NULL) ||
            (Addr == NULL))
          {
          break;
          }

        MSchedAddToAddressList(Hostname,Addr,&MSched.AddrList,&MSched.AddrListSize);

        NumAddrs++;

        ptr = MUStrTok(NULL,",",&TokPtr);
        }

      /* add addresses as generic resource */

      if (MNodeAdd("GLOBAL",&GN) == FAILURE)
        {
        return(FAILURE);
        }

      snprintf(tmpName,sizeof(tmpName),"ip-addr=%d",
        NumAddrs);

      MNodeSetAttr(GN,mnaAvlGRes,(void **)tmpName,mdfString,mSet);
      }  /* END BLOCK */

      break;

    case mcoVMCPurgeTime:

      S->VMCPurgeTime = MUTimeFromString(SVal);

      break;

    case mcoVMStaleAction:

      S->VMStaleAction = (enum MVMStaleActionEnum)MUGetIndexCI(
        SVal,
        MVMStaleAction,
        FALSE,
        mvmsaNONE);

      break;

    case mcoVMStaleIterations:

      S->VMStaleIterations = IVal; 

      break;

    case mcoWCViolAction:

      S->WCViolAction = (enum MWCVioActEnum)MUGetIndexCI(
        SVal,
        MWCVAction,
        FALSE,
        MSched.WCViolAction);

      break;

    case mcoWCViolCCode:

      S->WCViolCCode = IVal;

      break;

    case mcoWebServicesUrl:

      if (MSched.WebServicesConfigured == TRUE)
        MUStrDup(&S->WebServicesURL,SVal);
      else
        {
        MDB(2,fCONFIG) MLog("ERROR:     Web Services is not enabled in this build of moab\n");
        return(FAILURE);
        }

      break;

    case mcoWikiEvents:

      S->WikiEvents = MUBoolFromString(SVal,FALSE);

      break;

    default:

      /* not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (PIndex) */

  S->ParamSpecified[PIndex] = TRUE;

  return(SUCCESS);
  }  /* END MSchedProcessOConfig() */
/* END MSchedProcessOConfig.c */
