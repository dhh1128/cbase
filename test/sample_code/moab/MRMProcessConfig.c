/* HEADER */

      
/**
 * @file MRMProcessConfig.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Process RM-config attributes.
 *
 * @see MRMLoadConfig() - parent
 * @see MRMSetAttr() - child
 *
 * @param R (I)
 * @param Value (I)
 */

int MRMProcessConfig(
 
  mrm_t *R,     /* I */
  char  *Value) /* I */
 
  {
  int   aindex;

  enum MRMAttrEnum AIndex;
 
  char *ptr;
  char *TokPtr;
 
  char  ValLine[MMAX_LINE];

  int   rc;

  const char *FName = "MRMProcessConfig";

  MDB(3,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (Value != NULL) ? Value : "NULL");
 
  if ((R == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }
 
  /* process value line */

  ptr = MUStrTokE(Value," \t\n",&TokPtr);
 
  while (ptr != NULL)
    {
    /* parse name-value pairs */
 
    /* FORMAT:  <VALUE>[,<VALUE>] */
 
    if (MUGetPair(
          ptr,
          (const char **)MRMAttr,
          &aindex,   /* O */
          NULL,
          TRUE,
          NULL,
          ValLine,   /* O */
          sizeof(ValLine)) == FAILURE) 
      {
      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%.128s' specified",
        ptr);

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for RM %s\n",
        ptr,
        (R != NULL) ? R->Name : "NULL");

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);
 
      continue;
      }
 
    AIndex = (enum MRMAttrEnum)aindex;

    rc = SUCCESS;
 
    switch (AIndex)
      {       
      case mrmaAuthType:

        {
        enum MRMAuthEnum tmpA;

        tmpA = (enum MRMAuthEnum)MUGetIndexCI(
          ValLine,
          MRMAuthType,
          FALSE,
          mrmapNONE); 

        if (tmpA == mrmapNONE)
          {
          if (strcasecmp(MRMAuthType[mrmapNONE],ValLine) != 0)
            {
            /* NONE was not explicitly specified, invalid value specified */

            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"invalid %s value '%.128s' specified",
              MRMAttr[mrmaAuthType],
              ValLine);
 
            MDB(3,fSTRUCT) MLog("WARNING:  invalid %s value '%s' specified for RM %s\n",
              MRMAttr[mrmaAuthType],
              ValLine,
              (R != NULL) ? R->Name : "NULL");

            MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
            }
          }    /* END if (tmpA == mrmapNONE) */

        R->AuthType = tmpA;
        }  /* END BLOCK */

        break;

      case mrmaAdminExec:
      case mrmaApplyGResToAllReqs:
      case mrmaAuthAList:
      case mrmaAuthCList:
      case mrmaAuthGList:
      case mrmaAuthQList:
      case mrmaAuthUList:
      case mrmaBW:
      case mrmaCancelFailureExtendTime:
      case mrmaCheckpointSig:
      case mrmaCheckpointTimeout:
      case mrmaDefClass:
      case mrmaDefDStageInTime:
      case mrmaDefOS:
      case mrmaDefHighSpeedAdapter:
      case mrmaDescription:
      case mrmaExtHost:
      case mrmaFailTime:
      case mrmaFBServer:
      case mrmaFlags:
      case mrmaFnList:
      case mrmaIgnHNodes:
      case mrmaJobExportPolicy:
      case mrmaJobExtendDuration:
      case mrmaJobIDFormat:
      case mrmaMasterRM:
      case mrmaMaxJobStartDelay:
      case mrmaMaxTransferRate:
      case mrmaNodeDestroyPolicy:
      case mrmaNodeFailureRsvProfile:
      case mrmaPreemptDelay:
      case mrmaProvDuration:
      case mrmaPtyString:
      case mrmaUseVNodes:
      case mrmaJobRsvRecreate:
      case mrmaSBinDir:
      case mrmaServer:
      case mrmaSLURMFlags:
      case mrmaStageThreshold:
      case mrmaSyncJobID:
      case mrmaTrigger:
      case mrmaUpdateJobRMCreds:
      case mrmaVariables:
      case mrmaMaxJobPerMinute:
      case mrmaMaxJobs:
      case mrmaNoNeedNodes:
      case mrmaCompletedJobPurge:
      case mrmaOMap:
      case mrmaLanguages:
      case mrmaNoAllocMaster:
      case mrmaMaxFailCount:

        rc = MRMSetAttr(R,AIndex,(void **)ValLine,mdfString,mSet);

        break;

      case mrmaConfigFile:

        switch (R->Type)
          {
          case mrmtLL:
          case mrmtMoab:
          case mrmtNative:
          case mrmtWiki:

            MUStrCpy(R->ConfigFile,ValLine,sizeof(R->ConfigFile));   

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (R->Type) */

        break;

      case mrmaDataRM:

        if (ValLine != NULL)
          {
          /* NOTE: in future support multiple RM's here? */
          
          if (R->DataRM == NULL)
            {
            /* allocate array (just 1 slot for now) */

            R->DataRM = (mrm_t **)calloc(1,1 * sizeof(mrm_t *));  
            }
          
          if (MRMAdd(ValLine,&R->DataRM[0]) == FAILURE)
            {
            return(FAILURE);
            }
          }

        break;

      case mrmaDefJob:

        MTJobAdd(ValLine,(&R->JDef));

        break;

      case mrmaEncryption:

        R->P[0].DoEncryption = MUBoolFromString(ValLine,FALSE);

        break;

      case mrmaEnv:

        /* when reading in from the config file, we must take any ';' delimeters
         * and convert them into Moab's internal ENVRS_ENCODED_CHAR value */

        MUStrReplaceChar(ValLine,';',ENVRS_ENCODED_CHAR,TRUE);

        MUStrDup(&R->Env,ValLine);

        break;

      case mrmaEPort:

        R->EPort = (int)strtol(ValLine,NULL,10);

        break;
    
      case mrmaFeatureList:

        {
        char *ptr2;
        char *TokPtr2;

        /* FORMAT:  <FEATURE>[{ ,:}<FEATURE>]... */

        ptr2 = MUStrTok(ValLine,":, \t\n",&TokPtr2);

        while (ptr2 != NULL)
          {
          MUGetNodeFeature(ptr2,mAdd,&R->FBM);

          ptr2 = MUStrTok(NULL,":, \t\n",&TokPtr2);
          }  /* END while (ptr2 != NULL) */
        }    /* END (case mrmaFeatureList) */

        break;
  
      case mrmaHost:

        MUStrDup(&R->P[0].HostName,ValLine);      

        break;

      case mrmaJobCounter:

        R->JobCounter = MAX(R->JobCounter,(int)strtol(ValLine,NULL,0));

        if (R->JobCounter > MSched.MaxJobCounter)
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"specified job counter '%d' exceeds max job id '%d'",
            R->JobCounter,
            MSched.MaxJobCounter);
 
          MDB(3,fSTRUCT) MLog("WARNING:  invalid %s value '%s' specified for RM %s\n",
            MRMAttr[mrmaJobCounter],
            ValLine,
            (R != NULL) ? R->Name : "NULL");

          MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
          
          R->JobCounter = MSched.MaxJobCounter;
          } 

        break;

      case mrmaLocalDiskFS:

        MUStrCpy(R->PBS.LocalDiskFS,ValLine,sizeof(R->PBS.LocalDiskFS));    

        break;

      case mrmaMinETime:

        R->EMinTime = MUTimeFromString(ValLine);

        break;

      case mrmaMaxJob:

        MTJobAdd(ValLine,(&R->JMax));

        break;

      case mrmaMinJob:

        MTJobAdd(ValLine,(&R->JMin));

        break;

      case mrmaNMPort:

        R->NMPort    = (int)strtol(ValLine,NULL,10);
        R->P[1].Port = (int)strtol(ValLine,NULL,10);

        R->P[1].Type = mpstNM;

        break;

      case mrmaNMHost:
      case mrmaNMServer:

        MUStrDup(&R->P[1].HostName,ValLine);    

        R->P[1].Type = mpstNM;

        break;

      case mrmaNodeStatePolicy:

        R->NodeStatePolicy = (enum MRMNodeStatePolicyEnum)MUGetIndexCI(
          ValLine,
          MRMNodeStatePolicy,
          FALSE,
          0);

        break;

      case mrmaPartition:

        {
        mpar_t *P;

        if (MParAdd(ValLine,&P) == SUCCESS)
          R->PtIndex = P->Index;
        }  /* END BLOCK */

        break;

      case mrmaPollInterval:

        R->PollInterval = MUTimeFromString(ValLine);

        break;

      case mrmaPollTimeIsRigid:

        R->PollTimeIsRigid = MUBoolFromString(ValLine,TRUE);

        break;
 
      case mrmaPort:

        R->P[0].Port = (int)strtol(ValLine,NULL,10);          

        break;

      case mrmaResourceType:

        rc = MRMSetAttr(R,AIndex,(void **)ValLine,mdfString,mSet);

        break;

      case mrmaSetJob:

        MTJobAdd(ValLine,(&R->JSet));

        break;

      case mrmaSocketProtocol:

        R->P[0].SocketProtocol = (enum MSocketProtocolEnum)MUGetIndexCI(
          ValLine,
          MSockProtocol,
          FALSE,
          0);        

        R->P[1].SocketProtocol = R->P[0].SocketProtocol;

        break;

      case mrmaStartCmd:

        if (ValLine != NULL)
          MUStrDup(&R->StartCmd,ValLine);

        break;

      case mrmaSubmitCmd:

        if (ValLine != NULL)
          {
          MUStrDup(&R->SubmitCmd,ValLine);

          if (R->ND.URL[mrmJobSubmit] == NULL)
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"exec://%s%s",
              (ValLine[0] == '/') ? "" : "$HOME",
              ValLine);

            MUStrDup(&R->ND.URL[mrmJobSubmit],tmpLine);

            MUStrDup(&R->ND.Path[mrmJobSubmit],ValLine);

            R->ND.Protocol[mrmJobSubmit] = mbpExec;
            }
          }

        break;

      case mrmaSubmitPolicy:

        if (ValLine == NULL)
          break;

        R->SubmitPolicy = (enum MRMSubmitPolicyEnum)MUGetIndexCI(
          ValLine,
          MRMSubmitPolicy,
          FALSE,
          0);

        break;

      case mrmaSuspendSig:

        rc = MRMSetAttr(R,AIndex,(void **)ValLine,mdfString,mSet);

        break;

      case mrmaTargetUsage:

        R->TargetUsage = strtod(ValLine,NULL);

        break;

      case mrmaTimeout:

        {
        /* NOTE:  R->P[0].Timeout is in microsconds */

        mulong tmpL;

        /* value should be specified in seconds */

        if (strchr(ValLine,':') == NULL)
          {
          /* value specified as float, ie 1.5, or .004 */

          tmpL = (mulong)(MDEF_USPERSECOND * strtod(ValLine,NULL));

          if (tmpL < 50000)
            {
            /* NOTE:  assume value incorrectly specified in microseconds */

            /* convert from seconds to microseconds */

            tmpL *= MDEF_USPERSECOND;
            }
          }
        else
          {
          tmpL = MUTimeFromString(ValLine);
          }

        bmset(&R->IFlags,mrmifTimeoutSpecified);

        R->P[0].Timeout = tmpL;
        R->P[1].Timeout = tmpL;
        }  /* END BLOCK (case mrmaTimeout) */

        break;

      case mrmaVersion:

        /* NOTE:  allow decimal, octal, and hex versions */

        MUStrCpy(R->APIVersion,ValLine,sizeof(R->APIVersion));

        R->Version = (int)strtol(ValLine,NULL,0);

        break;

      case mrmaVMOwnerRM:

        MRMSetAttr(R,mrmaVMOwnerRM,(void **)ValLine,mdfString,mSet);

        break;

      case mrmaVPGRes:

        {  
        char  tmpLine[MMAX_LINE];

        char *ptr;
        char *TokPtr;

        /* FORMAT:  <GRESID>[*<GRESMULTIPLIER>] */

        MUStrCpy(tmpLine,ValLine,sizeof(tmpLine));

        ptr = MUStrTok(tmpLine,"*",&TokPtr);

        R->VPGResIndex = MUMAGetIndex(meGRes,ptr,mAdd);

        ptr = MUStrTok(NULL,"*",&TokPtr);

        if (ptr != NULL)
          R->VPGResMultiplier = (int)strtol(ptr,NULL,10);
        else
          R->VPGResMultiplier = 1;
        }  /* END BLOCK (case mrmaVPGRes) */

        break;

      case mrmaWireProtocol:

        R->P[0].WireProtocol = (enum MWireProtocolEnum)MUGetIndexCI(
          ValLine,
          MWireProtocol,
          FALSE,
          0);       

        R->P[1].WireProtocol = R->P[0].WireProtocol;

        break;

      case mrmaType:

        rc = MRMSetAttr(R,AIndex,(void **)ValLine,mdfString,mSet);

        /* hack - should indicate if AuthType was explicitly specified or inherited 
           as default using RM bitmap */

        if (R->Type == mrmtNative)
          R->AuthType = mrmapNONE;

        break;

      case mrmaClient:
      case mrmaClusterQueryURL:
      case mrmaCredCtlURL:
      case mrmaNodeMigrateURL:
      case mrmaNodeModifyURL:
      case mrmaNodePowerURL:
      case mrmaNodeVirtualizeURL:
      case mrmaQueueQueryURL:
      case mrmaWorkloadQueryURL:
      case mrmaInfoQueryURL:
      case mrmaJobCancelURL:
      case mrmaJobMigrateURL:
      case mrmaJobModifyURL:
      case mrmaJobRequeueURL:
      case mrmaJobResumeURL:
      case mrmaJobStartURL:
      case mrmaJobSubmitURL:
      case mrmaJobSuspendURL:
      case mrmaJobValidateURL:
      case mrmaParCreateURL:
      case mrmaParDestroyURL:
      case mrmaRMInitializeURL:
      case mrmaRMStartURL:
      case mrmaRMStopURL:
      case mrmaRsvCtlURL:
      case mrmaSystemModifyURL:
      case mrmaSystemQueryURL:
      case mrmaResourceCreateURL:

        rc = MRMSetAttr(R,AIndex,(void **)ValLine,mdfString,mSet);

        break;
 
      default:
 
        MDB(4,fRM) MLog("WARNING:  RM attribute '%s' not handled\n",
          MRMAttr[AIndex]);

        rc = FAILURE;
 
        break;
      }  /* END switch (AIndex) */

    if (rc == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"failed to process attribute '%s' (Value: '%.128s')",
        MRMAttr[AIndex],
        ValLine);

      MDB(3,fSTRUCT) MLog("WARNING:  cannot process attribute '%s' specified for RM %s\n",
        ptr,
        (R != NULL) ? R->Name : "NULL");

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
      }  /* END if (rc == FAILURE) */
 
    ptr = MUStrTokE(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */
 
  return(SUCCESS);
  }  /* END MRMProcessConfig() */
/* END MRMProcessConfig.c */
