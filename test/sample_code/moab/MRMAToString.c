/* HEADER */

      
/**
 * @file MRMAToString.c
 * 
 * Contains: MRMAToString
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Convert RM attribute to string.
 *
 * @see MRMSetAttr() - peer
 *
 * @param R (I)
 * @param AIndex (I)
 * @param SVal (O) [minsize=MMAX_LINE]
 * @param BufSize (I)
 * @param Mode (I) [not used]
 */

int MRMAToString(

  mrm_t            *R,       /* I */
  enum MRMAttrEnum  AIndex,  /* I */ 
  char             *SVal,    /* O (minsize=MMAX_LINE) */
  int               BufSize, /* I */
  int               Mode)    /* I (not used) */

  {
  if (SVal != NULL)
    SVal[0] = '\0';

  if ((R == NULL) || (SVal == NULL))
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mrmaAuthType:
 
      if (R->AuthType != 0)
        strcpy(SVal,MRMAuthType[R->AuthType]);
 
      break;

    case mrmaAuthAList:

      if (R->AuthAServer != NULL)
        {
        MUStrCpy(SVal,R->AuthAServer,BufSize);
        }

      break;

    case mrmaAuthCList:

      if (R->AuthCServer != NULL)
        {
        MUStrCpy(SVal,R->AuthCServer,BufSize);
        }

      break;

    case mrmaAuthGList:

      if (R->AuthGServer != NULL)
        {
        MUStrCpy(SVal,R->AuthGServer,BufSize);
        }

      break;

    case mrmaAuthQList:

      if (R->AuthQServer != NULL)
        {
        MUStrCpy(SVal,R->AuthQServer,BufSize);
        }

      break;

    case mrmaAuthUList:

      if (R->AuthUServer != NULL)
        {
        MUStrCpy(SVal,R->AuthUServer,BufSize);
        }

      break;

    case mrmaBW:

      if (R->MaxStageBW > 0.0)
        {
        sprintf(SVal,"%f",
          R->MaxStageBW);
        }

      break;

    case mrmaCheckpointSig:

      if (R->CheckpointSig[0] != '\0')
        strcpy(SVal,R->CheckpointSig);

      break;

    case mrmaCheckpointTimeout:

      if (R->CheckpointTimeout > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(R->CheckpointTimeout,TString);

        strcpy(SVal,TString);
        }

      break;

    case mrmaClient:

      if (R->ClientName[0] != '\0')
        strcpy(SVal,R->ClientName);

      break;

    case mrmaClockSkew:

      if (R->ClockSkewOffset != 0)
        {
        sprintf(SVal,"%ld",
          R->ClockSkewOffset);
        }

      break;

    case mrmaClusterQueryURL:

      if (R->ND.URL[mrmClusterQuery] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmClusterQuery],BufSize);
        }

      break;

    case mrmaConfigFile:
 
      if (R->ConfigFile[0] != '\0')
        MUStrCpy(SVal,R->ConfigFile,BufSize);
 
      break;

    case mrmaCredCtlURL:

      if (R->ND.URL[mrmCredCtl] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmCredCtl],BufSize);
        }

      break;

    case mrmaCSKey:

      /* NOTE:  only report P[0] */

      /* disabled */

      /*
      if (R->P[0].CSKey != NULL)
        MUStrCpy(SVal,R->P[0].CSKey,BufSize);
      */

      break;

    case mrmaDataRM:

      if (R->DataRM == NULL)
        break;

      if (R->DataRM[0] == NULL)
        break;

      MUStrCpy(SVal,R->DataRM[0]->Name,BufSize);
      
      break;

    case mrmaDefClass:

      if (R->DefaultC != NULL)
        strcpy(SVal,R->DefaultC->Name);

      break;

    case mrmaDefDStageInTime:

      if (R->DefDStageInTime > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(R->DefDStageInTime,TString);

        strcpy(SVal,TString);
        }

      break;

    case mrmaDefJob:

      if (R->JDef != NULL)
        strcpy(SVal,R->JDef->Name);

      break;

    case mrmaDefOS:

      if (R->DefOS > 0)
        strcpy(SVal,MAList[meOpsys][R->DefOS]);

      break;

    case mrmaEnv:

      if (R->Env != NULL)
        MUStrCpy(SVal,R->Env,BufSize);

      break;

    case mrmaEPort:

      if (R->EPort > 0)
        {
        sprintf(SVal,"%d",
          R->EPort);
        }

      break;

    case mrmaExtHost:

      if (R->ExtHost != NULL)
        MUStrCpy(SVal,R->ExtHost,BufSize);

      break;

    case mrmaFeatureList:

      {
      char Line[MMAX_LINE];

      strcpy(SVal,MUNodeFeaturesToString(',',&R->FBM,Line));
      }
 
      break;

    case mrmaFlags:

      if (!bmisclear(&R->Flags))
        {
        mstring_t Flags(MMAX_LINE);

        bmtostring(&R->Flags,MRMFlags,&Flags);

        MUStrCpy(SVal,Flags.c_str(),BufSize);
        }

      if (bmisset(&R->Flags,mrmfNoCreateAll))
        {
        char  tmpName[MMAX_NAME];

        char *ptr;

        /* look for format 'slave:<MASTER-RM>' */

        snprintf(tmpName,sizeof(tmpName),"%s:",
          MRMFlags[mrmfNoCreateAll]);

        if ((ptr = strstr(SVal,tmpName)) != NULL)
          { 
          ptr += strlen(tmpName);

          /* rm name bounded by comma or '\0' */

          MUStrCpy(tmpName,ptr,sizeof(tmpName));

          if ((ptr = strchr(tmpName,',')) != NULL)
            *ptr = '\0';

          if (MRMFind(tmpName,&R->MRM) == FAILURE)
            {
            /* cannot locate master rm */
            }
          }
        }    /* END if (bmisset(&R->Flags,mrmfNoCreateAll)) */

      break;

    case mrmaFnList:

      if (!bmisclear(&R->FnList))
        {
        mstring_t FnList(MMAX_LINE);

        bmtostring(&R->FnList,MRMFuncType,&FnList);

        MUStrCpy(SVal,FnList.c_str(),BufSize);
        }

      break;

    case mrmaHost:
 
     if (R->P[0].HostName != NULL)
       strcpy(SVal,R->P[0].HostName);
 
      break;

    case mrmaIgnHNodes:

      if (R->IgnHNodes == TRUE)
        sprintf(SVal,"%s",
          MBool[R->IgnHNodes]);

      break;

    case mrmaJobCancelURL:

      if (R->ND.URL[mrmJobCancel] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobCancel],BufSize);
        }

      break;

    case mrmaJobCount:

      if (R->JobCount > 0)
        sprintf(SVal,"%d",R->JobCount);

      break;

    case mrmaJobCounter:

      if (R->JobCounter > 0)
        {
        sprintf(SVal,"%d",
          R->JobCounter);
        }

      break;

    case mrmaJobExportPolicy:

      if (!bmisclear(&R->LocalJobExportPolicy))
        {
        bmtostring(&R->LocalJobExportPolicy,MExportPolicy,SVal);
        }

      break;

    case mrmaJobMigrateURL:

      if (R->ND.URL[mrmJobMigrate] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobMigrate],BufSize);
        }

      break;

    case mrmaJobModifyURL:

      if (R->ND.URL[mrmJobModify] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobModify],BufSize);
        }

      break;

    case mrmaJobRequeueURL:

      if (R->ND.URL[mrmJobRequeue] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobRequeue],BufSize);
        }

      break;

    case mrmaJobResumeURL:

      if (R->ND.URL[mrmJobResume] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobResume],BufSize);
        }

      break;

    case mrmaJobStartCount:

      if (R->JobStartCount > 0)
        sprintf(SVal,"%d",R->JobStartCount);

      break;

    case mrmaJobStartURL:

      if (R->ND.URL[mrmJobStart] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobStart],BufSize);
        }

      break;

    case mrmaJobSubmitURL:

      if (R->ND.URL[mrmJobSubmit] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobSubmit],BufSize);
        }

      break;

    case mrmaJobSuspendURL:

      if (R->ND.URL[mrmJobSuspend] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmJobSuspend],BufSize);
        }

      break;

    case mrmaJobValidateURL:

      if (R->ND.URL[mrmXJobValidate] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmXJobValidate],BufSize);
        }

      break;
 
    case mrmaLocalDiskFS:

      /* NYI */
 
      break;

    case mrmaMasterRM:

      if (R->MRM != NULL)
        {
        MUStrCpy(SVal,R->MRM->Name,BufSize);
        }

      break;

    case mrmaMaxJobPerMinute:

      if (R->MaxJobPerMinute > 0)
        {
        sprintf(SVal,"%d",
          R->MaxJobPerMinute);
        }

      break;

    case mrmaMaxJobs:

      if (R->MaxJobsAllowed > 0)
        {
        sprintf(SVal,"%d",
          R->MaxJobsAllowed);
        }

      break;

    case mrmaMaxTransferRate:

      if (R->MaxTransferRate > 0)
        {
        sprintf(SVal,"%f",
          R->MaxTransferRate);
        }

      break;

    case mrmaMessages:

      {
      char *BPtr;
      int   BSpace;
      char  DString[MMAX_LINE];

      int index;
      int findex;

      mpsi_t *P;

      /* NOTE:  only show messages for primary peer interface */

      P = &R->P[0];

      MUSNInit(&BPtr,&BSpace,SVal,MMAX_LINE);

      for (index = 0;index < MMAX_PSFAILURE;index++)
        {
        findex = (index + P->FailIndex) % MMAX_PSFAILURE;
  
        if (P->FailTime[findex] <= 0)
          continue;

        MULToDString((mulong *)&P->FailTime[findex],DString);

        MUSNPrintF(&BPtr,&BSpace,"failure %19.19s  %-15s  '%s'\n",
          DString,
          MRMFuncType[P->FailType[findex]],
          (P->FailMsg[findex] != NULL) ? P->FailMsg[findex] : "no msg");
        }  /* END for (index) */
      }    /* END BLOCK (case mrmaMessages) */

      break;

    case mrmaNC:

      if (R->NC > 0)
        sprintf(SVal,"%d",R->NC);

      break;

    case mrmaName:

     if (R->Name[0] != '\0')
       strcpy(SVal,R->Name);

      break;

    case mrmaNodeFailureRsvProfile:

      if (R->NodeFailureRsvProfile != NULL)
        strcpy(SVal,R->NodeFailureRsvProfile);

      break;

    case mrmaNodeList:

      if (!MNLIsEmpty(&R->NL))
        {
        mnode_t *N;

        if (MNLGetNodeAtIndex(&R->NL,0,&N) == SUCCESS)
          {
          MUStrCpy(SVal,N->Name,BufSize);
          }
        }

      break;

    case mrmaNodeMigrateURL:

      if (R->ND.URL[mrmXNodeMigrate] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmXNodeMigrate],BufSize);
        }

      break;

    case mrmaNodeModifyURL:

      if (R->ND.URL[mrmNodeModify] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmNodeModify],BufSize);
        }

      break;

    case mrmaNodePowerURL:

      if (R->ND.URL[mrmXNodePower] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmXNodePower],BufSize);
        }

      break;

    case mrmaNodeVirtualizeURL:

      if (R->ND.URL[mrmXNodeVirtualize] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmXNodeVirtualize],BufSize);
        }

      break;
 
    case mrmaNMPort:
 
      if (R->NMPort != 0)
        {
        /* NOTE:  NMPort affect connection to BOTH global node monitor and local node manager */

        sprintf(SVal,"%d",
          R->NMPort);
        }
      
      break;

    case mrmaNMServer:

      if (R->P[1].HostName != NULL)
        strcpy(SVal,R->P[1].HostName);

      break;

    case mrmaNodeStatePolicy:

      if (R->NodeStatePolicy != mrnspNONE)
        strcpy(SVal,MRMNodeStatePolicy[R->NodeStatePolicy]);

      break;

    case mrmaOMap:

      if (R->OMapServer != NULL)
        {
        MUStrCpy(SVal,R->OMapServer,BufSize);
        }

      break;

    case mrmaParCreateURL:

      if (R->ND.URL[mrmXParCreate] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmXParCreate],BufSize);
        }

      break;

    case mrmaParDestroyURL:

      if (R->ND.URL[mrmXParDestroy] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmXParDestroy],BufSize);
        }

      break;

    case mrmaPreemptDelay:

      if (R->PreemptDelay > 0)
        {
        sprintf(SVal,"%ld",
          R->PreemptDelay);
        }

      break;

    case mrmaProcs:

      if (MPar[R->PtIndex].CRes.Procs > 0)
        sprintf(SVal,"%d",MPar[R->PtIndex].CRes.Procs);

      break;

    case mrmaProfile:

      if (R->Profile != NULL)
        {
        MUStrCpy(SVal,R->Profile,BufSize);
        }

      break;

    case mrmaPartition:

      if (R->PtIndex > 0)
        {
        MUStrCpy(SVal,MPar[R->PtIndex].Name,BufSize);
        }
    
      break;

    case mrmaPort:
 
      if (R->P[0].Port > 0)
        {
        sprintf(SVal,"%d",
          R->P[0].Port);
        }
 
      break;

    case mrmaUseVNodes:

      if (R->UseVNodes == TRUE)
        sprintf(SVal,"%s",
          MBool[R->UseVNodes]);

      break;

    case mrmaQueueQueryURL:

      if (R->ND.URL[mrmQueueQuery] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmQueueQuery],BufSize);
        }

      break;

    case mrmaReqNC:

      if (R->ReqNC > 0)
        {
        sprintf(SVal,"%d",
          R->ReqNC);
        }

      break;

    case mrmaResourceCreateURL:

      /* NO-OP */

      break;

    case mrmaResourceType:

      if (!bmisclear(&R->RTypes))
        {
        bmtostring(&R->RTypes,MRMResourceType,SVal);
        }

      break;

    case mrmaRMInitializeURL:

      if (R->ND.URL[mrmRMInitialize] != NULL)
        MUStrCpy(SVal,R->ND.URL[mrmRMInitialize],BufSize);

      break;

    case mrmaRMStartURL:

      if (R->ND.URL[mrmRMStart] != NULL)
        MUStrCpy(SVal,R->ND.URL[mrmRMStart],BufSize);

      break;

    case mrmaRMStopURL:
                
      if (R->ND.URL[mrmRMStop] != NULL)
        MUStrCpy(SVal,R->ND.URL[mrmRMStop],BufSize);
               
      break;

    case mrmaRsvCtlURL:
                
      if (R->ND.URL[mrmRsvCtl] != NULL)
        MUStrCpy(SVal,R->ND.URL[mrmRsvCtl],BufSize);
               
      break;

    case mrmaSetJob:

      if (R->JSet != NULL)
        strcpy(SVal,R->JSet->Name);

      break;

    case mrmaSocketProtocol:

      if (R->P[0].SocketProtocol != mspNONE)
        strcpy(SVal,MSockProtocol[R->P[0].SocketProtocol]);
 
      break;

    case mrmaSQLData:

      if ((R->ND.S != NULL) && (R->ND.S->Data != NULL))
        MUStrCpy(SVal,R->ND.S->Data,BufSize);

      break;

    case mrmaStageThreshold:

      if (R->StageThreshold > 0)
        {
        sprintf(SVal,"%ld",
          R->StageThreshold);
        }

      break;

    case mrmaStartCmd:

      if (R->StartCmd != NULL)
        strcpy(SVal,R->StartCmd);

      break;

    case mrmaState:

      strcpy(SVal,MRMState[R->State]);

      break;

    case mrmaStats:

      {
      mpsi_t *P;

      double tmpD;

      P = &R->P[0];  /* primary peer only */

      if (P->RespTotalCount[0] > 0)
        tmpD = (double)P->RespTotalTime[0] / P->RespTotalCount[0] / 1000;
      else
        tmpD = 0.0;

      sprintf(SVal,"%.2f,%.2f,%d",
        tmpD,
        (double)P->RespMaxTime[0] / 1000,
        P->RespTotalCount[0]);
      }  /* END BLOCK (case mrmaStats) */

      break;

    case mrmaSubmitCmd:

      if (R->SubmitCmd != NULL)
        strcpy(SVal,R->SubmitCmd);

      break;

    case mrmaSubmitPolicy:

      if (R->SubmitPolicy == mrmspNONE)
        break;

      strcpy(SVal,MRMSubmitPolicy[R->SubmitPolicy]);

      break;

    case mrmaSuspendSig:

      if (R->SuspendSig[0] != '\0')
        strcpy(SVal,R->SuspendSig);

      break;

    case mrmaSyncJobID:

      if (R->SyncJobID == TRUE)
        sprintf(SVal,"%s",
          MBool[R->SyncJobID]);

      break;

    case mrmaSystemModifyURL:

      if (R->ND.URL[mrmSystemModify] != NULL)
        MUStrCpy(SVal,R->ND.URL[mrmSystemModify],BufSize);

      break;

    case mrmaSystemQueryURL:

      if (R->ND.URL[mrmSystemQuery] != NULL)
        MUStrCpy(SVal,R->ND.URL[mrmSystemQuery],BufSize);

      break;

    case mrmaTargetUsage:
 
      if (R->TargetUsage > 0.0)
        sprintf(SVal,"%.2f",
          R->TargetUsage);
 
      break;

    case mrmaTimeout:
 
      {
      char *VTString = NULL;

      if (MULToVTString((R->P[0].Timeout/MDEF_USPERSECOND),&VTString) == SUCCESS)
          strcpy(SVal,VTString);
    
      MUFree(&VTString);
      }
 
      break;
 
    case mrmaType:

      /* FORMAT:  <TYPE>[:<SUBTYPE>] */
 
      if (R->Type != mrmtNONE)
        {
        strcpy(SVal,MRMType[R->Type]);

        if (R->SubType != mrmstNONE)
          {
          sprintf(SVal,"%s:%s",
            SVal,
            MRMSubType[R->SubType]);
          }
        }    /* END if (R->Type != mrmtNONE) */

      break;

    case mrmaUpdateTime:

      if (R->UTime > 0)
        {
        sprintf(SVal,"%ld",
          R->UTime);
        }

      break;

    case mrmaVariables:
        
      if (R->Variables != NULL)
        {
        char *BPtr = NULL;
        int   BSpace = 0;
      
        mln_t *tmpL;
       
        tmpL = R->Variables;
        
        MUSNInit(&BPtr,&BSpace,SVal,BufSize);
     
        while (tmpL != NULL)
          {
          MUSNCat(&BPtr,&BSpace,tmpL->Name);
    
          if (tmpL->Ptr != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"=%s",(char *)tmpL->Ptr);
            }
   
          if (tmpL->Next != NULL)
            {
            MUSNCat(&BPtr,&BSpace,",");
            }
            
          tmpL = tmpL->Next;
          }  /* END while (tmpL != NULL) */
        }    /* END BLOCK (case mrmaVariables) */
           
      break;

    case mrmaVersion:

      if (R->APIVersion[0] != '\0')
        strcpy(SVal,R->APIVersion);

      if (R->Version == 0)
        R->Version = (int)strtol(R->APIVersion,NULL,10);

      break;

    case mrmaVMOwnerRM:

      if (R->VMOwnerRM != NULL)
        strcpy(SVal,R->VMOwnerRM->Name);

      break;

    case mrmaWireProtocol:

      if (R->P[0].WireProtocol != mwpNONE)         
        strcpy(SVal,MWireProtocol[R->P[0].WireProtocol]);
 
      break;

    case mrmaWorkloadQueryURL:

      if (R->ND.URL[mrmWorkloadQuery] != NULL)
        {
        MUStrCpy(SVal,R->ND.URL[mrmWorkloadQuery],BufSize);
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  if (SVal[0] == '\0')
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMAToString() */
/* END MRMAToString.c */
