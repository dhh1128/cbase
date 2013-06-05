/* HEADER */

      
/**
 * @file MRMSetAttr.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Set RM attribute.
 * 
 * @see MRMAToString() - peer
 * @see MRMProcessConfig() - parent - calls this routine to process config
 *
 * @param R (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MRMSetAttr(

  mrm_t                   *R,      /* I (modified) */
  enum MRMAttrEnum         AIndex, /* I */
  void                   **Value,  /* I */
  enum MDataFormatEnum     Format, /* I */
  enum MObjectSetModeEnum  Mode)   /* I */

  {
  const char *FName = "MRMSetAttr";

  MDB(7,fSCHED) MLog("%s(%s,%s,Value,%s,%d)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MRMAttr[AIndex],
    MFormatMode[Format],
    Mode);

  if (R == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mrmaAdminExec:

      {
      char   tmpLine[MMAX_LINE];

      char  *ptr;

      char  *TokPtr;

      int    RMIndex;

      mnat_t *ND;

      ND = (mnat_t *)&R->ND;

      /* FORMAT:  <RMFUNC>[,<RMFUNC>]... */

      MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,", \t\n",&TokPtr);

      while (ptr != NULL)
        {
        RMIndex = MUGetIndexCI(ptr,MRMFuncType,FALSE,0);

        if (RMIndex > 0)
          ND->AdminExec[RMIndex] = TRUE;
        
        ptr = MUStrTok(NULL,", \t\n",&TokPtr);
        }
      }    /* END BLOCK (case mrmaAdminExec) */
 
      break;

    case mrmaApplyGResToAllReqs:

      R->ApplyGResToAllReqs = MUBoolFromString((char *)Value,FALSE);

      break;

    case mrmaAuthAList:
    case mrmaAuthCList:
    case mrmaAuthGList:
    case mrmaAuthQList:
    case mrmaAuthUList:

      {
      char   *ptr;
      char   *TokPtr = NULL;
      char  **AList;
      char ***AListP = NULL;

      int   uindex;

      /* FORMAT: <USER>[,<USER>]... or <PROTOCOL>:<SERVER> */

      if (strchr((char *)Value,':'))
        {
        /* server specified */

        switch (AIndex)
          {
          case mrmaAuthAList: MUStrDup(&R->AuthAServer,(char *)Value); break; 
          case mrmaAuthCList: MUStrDup(&R->AuthCServer,(char *)Value); break;
          case mrmaAuthGList: MUStrDup(&R->AuthGServer,(char *)Value); break;
          case mrmaAuthQList: MUStrDup(&R->AuthQServer,(char *)Value); break;
          case mrmaAuthUList: MUStrDup(&R->AuthUServer,(char *)Value); break;
          default: break;
          }

        /* load server data (NYI) */
        }

      switch (AIndex)
        {
        case mrmaAuthAList: AListP = &R->AuthA; break;
        case mrmaAuthCList: AListP = &R->AuthC; break;
        case mrmaAuthGList: AListP = &R->AuthG; break;
        case mrmaAuthQList: AListP = &R->AuthQ; break;
        case mrmaAuthUList: AListP = &R->AuthU; break;
        default: break;
        }

      if (*AListP == NULL)
        {
        *AListP = (char **)MUCalloc(1,sizeof(char *) * MMAX_CREDS);
        }

      AList = *AListP;

      /* TODO: change to MACLLoadString() to allow cred affinity - NYI */

      ptr = MUStrTok((char *)Value," \t\n,:",&TokPtr);

      uindex = 0;

      while (ptr != NULL)
        {
        if (uindex >= MMAX_CREDS)
          break;

        MUStrDup(&AList[uindex],ptr);

        uindex++;

        ptr = MUStrTok(NULL," \t\n,:",&TokPtr);
        }
      }  /* END BLOCK (case mrmaAuthUList) */
      
      break;

    case mrmaBW:

      if (Format == mdfString)
        {
        char *tail;

        /* FORMAT:  <DOUBLE>{M|G|T} */

        R->MaxStageBW = strtod((char *)Value,&tail);

        if (tail != NULL)
          {
          switch (*tail)
            {
            case 'g':
            case 'G':

              R->MaxStageBW *= 1024;
  
              break;

            case 't':
            case 'T':

              R->MaxStageBW *= 1024 * 1024;

              break;

            case 'm':
            case 'M':
            default:

              /* NO-OP */

              break;
            }
          }
        }  /* END if (DFormat == mdfString) */

      break;

    case mrmaCancelFailureExtendTime:

      R->CancelFailureExtendTime = MUTimeFromString((char *)Value);

      break;

    case mrmaCheckpointSig:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        MUStrCpy(R->CheckpointSig,(char *)Value,sizeof(R->CheckpointSig));
        }
      else
        {
        sprintf(R->CheckpointSig,"%d",
          *(int *)Value);
        }

      break;

    case mrmaCheckpointTimeout:

      R->CheckpointTimeout = MUTimeFromString((char *)Value);

      break;

    case mrmaClient:

      MUStrCpy(R->ClientName,(char *)Value,sizeof(R->ClientName));

      break;

    case mrmaCSKey:

      if (Value == NULL)
        return(FAILURE);

      {
      char *ptr;
      char *TokPtr;

      /* FORMAT:  <KEY>[,<KEY>] */

      ptr = MUStrTok((char *)Value,",:",&TokPtr);

      if (ptr != NULL)
        {
        MUStrDup(&R->P[0].CSKey,(char *)Value);

        ptr = MUStrTok(NULL,",:",&TokPtr);

        if (ptr != NULL)
          {
          MUStrDup(&R->P[1].CSKey,(char *)Value);
          }
        }    /* END if (ptr != NULL) */
      }      /* END BLOCK */

      break;

    case mrmaDefClass:

      MClassAdd((char *)Value,&R->DefaultC);

      break;

    case mrmaDefDStageInTime:

      R->DefDStageInTime = MUTimeFromString((char *)Value);

      break;

    case mrmaDescription:

      MUStrDup(&R->Description,(char *)Value);

      break;

    case mrmaDefOS:

      R->DefOS = MUMAGetIndex(meOpsys,(char *)Value,mAdd);

      break;
 
    case mrmaExtHost:

      MUStrDup(&R->ExtHost,(char *)Value);

      break;

    case mrmaClusterQueryURL:
    case mrmaCredCtlURL:
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
    case mrmaNodeMigrateURL:
    case mrmaNodeModifyURL:
    case mrmaNodePowerURL:
    case mrmaNodeVirtualizeURL:
    case mrmaParCreateURL:
    case mrmaParDestroyURL:
    case mrmaQueueQueryURL:
    case mrmaResourceCreateURL:
    case mrmaRMInitializeURL:
    case mrmaRMStartURL:
    case mrmaRMStopURL:
    case mrmaRsvCtlURL:
    case mrmaSystemModifyURL:
    case mrmaSystemQueryURL:
    case mrmaWorkloadQueryURL:

      {
      char tmpProtoString[MMAX_NAME];
      char tmpHost[MMAX_NAME];
      char tmpDir[MMAX_PATH_LEN];
      int  tmpPort;

      enum MBaseProtocolEnum tmpProtocol;

      enum MRMFuncEnum       RFIndex = mrmNONE;

      mnat_t *ND;

      ND = (mnat_t *)&R->ND;

      switch (AIndex)
        {
        case mrmaClusterQueryURL:   RFIndex = mrmClusterQuery; break;
        case mrmaCredCtlURL:        RFIndex = mrmCredCtl; break;
        case mrmaInfoQueryURL:      RFIndex = mrmInfoQuery; break;
        case mrmaJobCancelURL:      RFIndex = mrmJobCancel; break;
        case mrmaJobMigrateURL:     RFIndex = mrmJobMigrate; break;
        case mrmaJobModifyURL:      RFIndex = mrmJobModify; break;
        case mrmaJobRequeueURL:     RFIndex = mrmJobRequeue; break;
        case mrmaJobResumeURL:      RFIndex = mrmJobResume; break;
        case mrmaJobStartURL:       RFIndex = mrmJobStart; break;
        case mrmaJobSubmitURL:      RFIndex = mrmJobSubmit; break;
        case mrmaJobSuspendURL:     RFIndex = mrmJobSuspend; break;
        case mrmaJobValidateURL:    RFIndex = mrmXJobValidate; break;
        case mrmaNodeMigrateURL:    RFIndex = mrmXNodeMigrate; break;
        case mrmaNodeModifyURL:     RFIndex = mrmNodeModify; break;
        case mrmaNodePowerURL:      RFIndex = mrmXNodePower; break;
        case mrmaNodeVirtualizeURL: RFIndex = mrmXNodeVirtualize; break;
        case mrmaParCreateURL:      RFIndex = mrmXParCreate; break;
        case mrmaParDestroyURL:     RFIndex = mrmXParDestroy; break;
        case mrmaSystemModifyURL:   RFIndex = mrmSystemModify; break;
        case mrmaSystemQueryURL:    RFIndex = mrmSystemQuery; break;
        case mrmaQueueQueryURL:     RFIndex = mrmQueueQuery; break;
        case mrmaResourceCreateURL: RFIndex = mrmResourceCreate; break; 
        case mrmaRMInitializeURL:   RFIndex = mrmRMInitialize; break;
        case mrmaRMStartURL:        RFIndex = mrmRMStart; break;
        case mrmaRMStopURL:         RFIndex = mrmRMStop; break;
        case mrmaRsvCtlURL:         RFIndex = mrmRsvCtl; break;
        case mrmaWorkloadQueryURL:  RFIndex = mrmWorkloadQuery; break;
        default:                    assert(FALSE); break;
        }  /* END switch (AIndex) */
 
      if (Value == NULL)
        {
        /* unset URL */

        switch (AIndex)
          {

          case mrmaCredCtlURL:
          case mrmaClusterQueryURL:
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
          case mrmaNodeMigrateURL:
          case mrmaNodeModifyURL:
          case mrmaNodePowerURL:
          case mrmaNodeVirtualizeURL:
          case mrmaParCreateURL:
          case mrmaParDestroyURL:
          case mrmaQueueQueryURL:
          case mrmaResourceCreateURL:
          case mrmaRMInitializeURL:
          case mrmaRMStartURL:
          case mrmaRMStopURL:
          case mrmaRsvCtlURL:
          case mrmaSystemModifyURL:
          case mrmaSystemQueryURL:
          case mrmaWorkloadQueryURL:

            ND->Protocol[RFIndex] = mbpNONE;

            MUFree(&ND->Path[RFIndex]);
            MUFree(&ND->URL[RFIndex]);

            break;

          default:

            assert(FALSE);

            /* NO-OP */

            break;
          }  /* END switch (AIndex) */

        break;
        }  /* END if (Value == NULL) */
      else
        {
        /* set URL */

        switch (AIndex)
          {

          case mrmaCredCtlURL:
          case mrmaClusterQueryURL:
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
          case mrmaNodeMigrateURL:
          case mrmaNodeModifyURL:
          case mrmaNodePowerURL:
          case mrmaNodeVirtualizeURL:
          case mrmaParCreateURL:
          case mrmaParDestroyURL:
          case mrmaQueueQueryURL:
          case mrmaResourceCreateURL:
          case mrmaRMInitializeURL:
          case mrmaRMStartURL:
          case mrmaRMStopURL:
          case mrmaRsvCtlURL:
          case mrmaSystemModifyURL:
          case mrmaSystemQueryURL:
          case mrmaWorkloadQueryURL:

            MUStrDup(&ND->URL[RFIndex],(char *)Value);

            break;

          default:

            assert(FALSE);
            /* NO-OP */

            break;
          }  /* END switch (AIndex) */

        /* NOTE:  move MSET to later in routine after URL is validated */

        /* NOTE:  disable until all URL's are properly registered, ie allow URLs use URL[x] attribute */
/*
        if ((RFIndex != mrmNONE) && (R->FnListSpecified == FALSE))
          MSET(R->FnList,RFIndex);
*/
        }    /* END if (Value == NULL) */

      /* parse URL */

      if (MUURLParse(
           (char *)Value,
           tmpProtoString,
           tmpHost,            /* O */
           tmpDir,             /* O */
           sizeof(tmpDir),
           &tmpPort,           /* O */
           TRUE,
           TRUE) == SUCCESS)
        {
        ND->Port[RFIndex] = tmpPort;

        /* NOTE:  high-level serverport was previously set based on last processed URL.
         *        this attribute should only be set via the RM 'port' config option */

        /* ND->ServerPort = tmpPort; */

        MUStrDup(&ND->Host[RFIndex],tmpHost);

        tmpProtocol = (enum MBaseProtocolEnum)MUGetIndexCI(
          tmpProtoString,
          MBaseProtocol,
          FALSE,
          mbpNONE);

        if (tmpProtocol == mbpNONE)
          {
          /* set default */

          tmpProtocol = mbpExec;
          }
        }
      else
        {
        tmpProtocol = mbpExec;

        MUStrCpy(tmpDir,(char *)Value,sizeof(tmpDir));
        }

      /* sanity check URL */

      if ((tmpDir != NULL) &&
         ((tmpProtocol == mbpExec) || (tmpProtocol == mbpFile)))
        {
        char *ptr;

        mbool_t IsExe;

        int     rc;

        /* remove args for file check */

        if ((ptr = strchr(tmpDir,'?')) != NULL)
          *ptr = '\0';

        rc = MFUGetInfo(tmpDir,NULL,NULL,&IsExe,NULL);

        if (ptr != NULL)
          *ptr = '?';

        if (rc == FAILURE)
          {
          char tmpLine[MMAX_LINE];

          /* warn and continue */

          MDB(3,fNAT) MLog("WARNING:  invalid value '%s' specified for %s (cannot locate path)\n",
            (char *)Value,
            MRMAttr[AIndex]);

          snprintf(tmpLine,sizeof(tmpLine),"file %s for %s does not exist",
            tmpDir,
            MRMAttr[AIndex]);

          MRMSetAttr(
            R,
            mrmaMessages,
            (void **)tmpLine,
            mdfString,
            mIncr);
          }
        else if ((tmpProtocol == mbpExec) && (IsExe != TRUE))
          {
          char tmpLine[MMAX_LINE];

          MDB(3,fNAT) MLog("WARNING:  invalid value '%s' specified for '%s' (path not executable)\n",
            (char *)Value,
            MRMAttr[AIndex]);

          snprintf(tmpLine,sizeof(tmpLine),"file %s for %s is not executable",
            tmpDir,
            MRMAttr[AIndex]);

          MRMSetAttr(
            R,
            mrmaMessages,
            (void **)tmpLine,
            mdfString,
            mIncr);

          return(FAILURE);
          }
        }    /* END if ((tmpDir != NULL) && ...) */

      /* set values */

      switch (AIndex)
        {
        case mrmaClusterQueryURL:

          ND->Protocol[mrmClusterQuery] = tmpProtocol;

          if (!MUStrIsEmpty(tmpDir))
            {
            MUStrDup(&ND->Path[mrmClusterQuery],tmpDir);
            }

          break;

        case mrmaCredCtlURL:
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
        case mrmaNodeMigrateURL:
        case mrmaNodeModifyURL:
        case mrmaNodePowerURL:
        case mrmaNodeVirtualizeURL:
        case mrmaParCreateURL:
        case mrmaParDestroyURL:
        case mrmaQueueQueryURL:
        case mrmaResourceCreateURL:
        case mrmaRMInitializeURL:
        case mrmaRMStartURL:
        case mrmaRMStopURL:
        case mrmaRsvCtlURL:
        case mrmaSystemModifyURL:
        case mrmaSystemQueryURL:
        case mrmaWorkloadQueryURL:

          ND->Protocol[RFIndex] = tmpProtocol;

          MUStrDup(&ND->Path[RFIndex],tmpDir);

          break;

        default:

          assert(FALSE);
          /* NO-OP */

          break;
        }  /* END switch (AIndex) */

      /* NOTE:  only override, no support for add */
      }  /* END BLOCK (case mrmaClusterQueryCmd, ...) */

      break;

    case mrmaJobIDFormat:

      if (!strcasecmp((char *)Value,"integer"))
        {
        R->JobIDFormatIsInteger = TRUE;
        }

      break;

    case mrmaCompletedJobPurge:

      R->PBS.EnableCompletedJobPurge = MUBoolFromString((char *)Value,FALSE);

      break;

    case mrmaCSAlgo:

      {
      char *ptr;
      char *TokPtr = NULL;

      ptr = MUStrTok((char *)Value,":'\"",&TokPtr);

      R->P[0].CSAlgo = (enum MChecksumAlgoEnum)MUGetIndexCI(
        ptr,
        MCSAlgoType,
        TRUE,
        R->P[0].CSAlgo);

      R->P[1].CSAlgo = (enum MChecksumAlgoEnum)MUGetIndexCI(
        ptr,
        MCSAlgoType,
        TRUE,
        R->P[1].CSAlgo);

      ptr = MUStrTok(NULL,":'\"",&TokPtr);

      MUStrDup(&R->P[0].DstAuthID,ptr);
      MUStrDup(&R->P[1].DstAuthID,ptr);
      }  /* END BLOCK */

      break;

    case mrmaDefHighSpeedAdapter:

      if (Value != NULL)
        {
        MUStrDup(&R->DefHighSpeedAdapter,(char *)Value);
        }

      break;

    case mrmaDefJob:

      MTJobAdd((char *)Value,(&R->JDef));

      break;
     
    case mrmaEnv:

      if ((Mode == mAdd) || (Mode == mIncr))
        MUStrAppend(&R->Env,NULL,(char *)Value,';');
      else
        MUStrDup(&R->Env,(char *)Value);

      break;

    case mrmaEPort:

      if (Value != NULL)
        {
        R->EPort = (int)strtol((char *)Value,NULL,10);
        }

      break;

    case mrmaFailTime:

      R->TrigFailDuration = MUTimeFromString((char *)Value);

      break;

    case mrmaFBServer:
    case mrmaServer:

        {
        mpsi_t *P;

        char tmpProtocol[MMAX_LINE];
        char tmpHost[MMAX_LINE];
        char tmpPath[MMAX_LINE];

        int  tmpPort;

        if (MUURLParse(
              (char *)Value,
              tmpProtocol,
              tmpHost,
              tmpPath,
              sizeof(tmpPath),
              &tmpPort,
              TRUE,
              TRUE) == FAILURE)
          {
          /* cannot parse string */

          break;
          }

        if (AIndex == mrmaServer)
          {
          P = &R->P[0];
          }
        else
          {
          P = &R->P[1];

          R->P[0].FallBackP = &R->P[1];
          }

        if (tmpProtocol[0] != '\0')
          {
          R->Type = (enum MRMTypeEnum)MUGetIndexCI(
            tmpProtocol,
            MRMType,
            FALSE,
            mrmtNONE);

          if (R->Type == mrmtNONE)
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"cannot create RM interface with invalid type '%s'",
              tmpProtocol);

            MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

            snprintf(tmpLine,sizeof(tmpLine),"invalid RM type '%s' specified, using default type",
              tmpProtocol);

            MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

            break;
            }

          R->TypeIsExplicit = TRUE;
          }

        if (tmpHost[0] != '\0')
          {
          MUStrDup(&P->HostName,tmpHost);
          MUGetIPAddr(P->HostName,&P->IPAddr);
          }

        /* NOTE: ignore tmpPath for now */

        if (tmpPort > 0)
          {
          P->Port = tmpPort;
          }
        else if (R->Type == mrmtMoab)
          {
          /* set Moab default port */

          P->Port = MDEF_SERVERPORT;

          /* set scheduling policy to EarliestStartTime if not already set */

          if (MSched.SchedPolicy == mscpNONE)
            {
            MSched.SchedPolicy = mscpEarliestStart;
            }
          }
        }  /* END BLOCK */

      break;

    case mrmaFlags:

      {
      mbitmap_t tmpFlag;

      mrm_t  *tMRM = NULL;

      /* WARNING:  may overwrite flags set thru alternate methods */

      if (Format == mdfString)
        {
        if (Value != NULL)
          {
          mbool_t SpecialFlagDetected = FALSE;

          if (MUStrStr((char *)Value,"imageisstateful",0,TRUE,FALSE))
            {
            /* HACK:  place on internal flags in 5.3 - move to flags for 5.4 */

            bmset(&R->IFlags,mrmifImageIsStateful);

            SpecialFlagDetected = TRUE;
            }

          if (MUStrStr((char *)Value,"twostageprovisioning",0,TRUE,FALSE))
            {
            /* HACK:  place on internal flags in 5.3 - move to flags for 5.4 */

            MSched.TwoStageProvisioningEnabled = TRUE;

            SpecialFlagDetected = TRUE;
            }

          bmfromstring((char *)Value,MRMFlags,&tmpFlag," ,:\t\n");

          if ((((char *)Value)[0] != '\0') && bmisclear(&tmpFlag))
            {
            if (SpecialFlagDetected == TRUE)
              {
              /* no standard flags detected - no further processing to do */

              return(SUCCESS);
              }

            MDB(3,fNAT) MLog("WARNING:  invalid value '%s' specified for '%s' (flags  not valid)\n",
              (char *)Value,
              MRMAttr[AIndex]);

            return(FAILURE);
            }

          if (bmisset(&tmpFlag,mrmfNoCreateAll))
            {
            char  tmpName[MMAX_NAME];

            char *ptr;

            snprintf(tmpName,sizeof(tmpName),"%s:",
              MRMFlags[mrmfNoCreateAll]);

            if ((ptr = strstr((char *)Value,tmpName)) != NULL)
              {
              ptr += strlen(tmpName);

              /* rm name bounded by comma or '\0' */

              MUStrCpy(tmpName,ptr,sizeof(tmpName));

              if ((ptr = strchr(tmpName,',')) != NULL)
                *ptr = '\0';

              if (MRMFind(tmpName,&tMRM) == FAILURE)
                {
                /* cannot locate master rm */
                }
              }
            }    /* END if (bmisset(&R->Flags,mrmfNoCreateAll)) */
          }      /* END else */
        }        /* END if (Format == mdfString) */

      if ((Mode == mAdd) || (Mode == mIncr))
        {
        bmor(&R->Flags,&tmpFlag);

        if (tMRM != NULL)
          R->MRM = tMRM;
        }
      else if ((Mode == mDecr) || (Mode == mUnset))
        {
        int index;

        for (index = 0;index < mrmfLAST;index++)
          {
          if (bmisset(&tmpFlag,index))
            bmunset(&R->Flags,index);
          }  /* END for (index) */

        if (tMRM != NULL)
          R->MRM = NULL;
        }
      else   /* default is set */
        {
        bmcopy(&R->Flags,&tmpFlag);

        R->MRM = tMRM;
        }

      /* FIXME:  if "client" flag is set, must rename RM to <name>.INBOUND */

      if (bmisset(&R->Flags,mrmfClient))
        R->Type = mrmtMoab;

      bmcopy(&R->SpecFlags,&R->Flags);

      bmclear(&tmpFlag);
      }  /* END BLOCK (case mrmaFlags) */

      break;

    case mrmaFnList:

      if (Format == mdfString)
        {
        bmfromstring((char *)Value,MRMFuncType,&R->FnList);
        }

      R->FnListSpecified = TRUE;

      break;

    case mrmaHost:

      if (Value != NULL)
        MUStrDup(&R->P[0].HostName,(char *)Value);

      break;

    case mrmaIgnHNodes:

      R->IgnHNodes = MUBoolFromString((char *)Value,FALSE);
      
      break;

    case mrmaJobCounter:

      if (Value != NULL)
        {
        /* NOTE:  allow non-decimal values (hex, octal, etc) */

        R->JobCounter = (int)strtol((char *)Value,NULL,0);
        }

      break;

    case mrmaJobExportPolicy:

      bmfromstring((char *)Value,MExportPolicy,&R->LocalJobExportPolicy," ,:\t\n");

      if (bmisset(&R->LocalJobExportPolicy,mexpInfo))
        bmset(&R->Flags,mrmfLocalWorkloadExport);

      break;

    case mrmaJobExtendDuration:

      /* FORMAT:  <MIN>[,<MAX>][!][<] */

      {
      char *ptr;
      char *TokPtr = NULL;

      ptr = strchr((char *)Value,'!');

      if (ptr != NULL)
        {
        if (*(ptr + 1) == '\0')
          {
          /* user put '!' at end of string */

          bmset(&R->IFlags,mrmifDisablePreemptExtJob);
          }
        }

      ptr = strchr((char *)Value,'<');

      if (ptr != NULL)
        {
        if (*(ptr + 1) == '\0')
          {
          /* user put '<' at end of string */

          bmset(&R->IFlags,mrmifDisableIncrExtJob);
          }
        else if (*(ptr + 1) == '<')
          {
          bmset(&R->IFlags,mrmifMaximizeExtJob);
          bmset(&R->IFlags,mrmifDisableIncrExtJob);
          }
        }

      ptr = strpbrk((char *)Value,"!<");

      if (ptr != NULL)
        {
        /* assume these characters are at the end of the string--remove them! */

        *ptr = '\0';
        }

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      R->MinJobExtendDuration = MUTimeFromString(ptr);

      ptr = MUStrTok(NULL,",",&TokPtr);

      if (ptr != NULL)
        {
        R->MaxJobExtendDuration = MAX(
          R->MinJobExtendDuration,
          (mulong)MUTimeFromString(ptr));
        }
      else
        {
        R->MaxJobExtendDuration = R->MinJobExtendDuration;
        }
      }

      break;

    case mrmaJobRsvRecreate:

      R->JobRsvRecreate = MUBoolFromString((char *)Value,TRUE);

      break;

    /* case mrmaJobList: */

      /* NOTE:  route via RM->Queue child for now (may need to expand structure
                later) */

      /* NO-OP */

      /* break; */

    case mrmaLanguages:

      {
      mbitmap_t BM;

      bmfromstring((char *)Value,MRMType,&BM);

      bmor(&R->Languages,&BM);
      }

      break;

    case mrmaMasterRM:

      if (Value != NULL)
        {
        /* NOTE:  potential race condition if rm discovery ordering changes */

        MRMFind((char *)Value,&R->MRM);
        }
      else
        {
        R->MRM = NULL;
        }

      break;

    case mrmaMaxFailCount:

      if (Value != NULL)
        R->MaxFailCount = (int)strtol((char *)Value,NULL,10);
      else
        R->MaxFailCount = 0;

      break;

    case mrmaMaxJobPerMinute:

      if (Value != NULL)
        R->MaxJobPerMinute = (int)strtol((char *)Value,NULL,10);
      else
        R->MaxJobPerMinute = 0;

      break;

    case mrmaMaxJobs:

      if (Value != NULL)
        R->MaxJobsAllowed = (int)strtol((char *)Value,NULL,10);
      else
        R->MaxJobsAllowed = 0;

      break;

    case mrmaMaxJobStartDelay:

      if (Value != NULL)
        R->MaxJobStartDelay = MUTimeFromString((char *)Value);
      else
        R->MaxJobStartDelay = 0;

      break;

    case mrmaMaxTransferRate:

      if (Value != NULL)
        {
        R->MaxTransferRate = strtol((char *)Value,NULL,10);
        }
      else
        {
        R->MaxTransferRate = 0;
        }

      break;

    case mrmaMessages:

      if (Format == mdfString)
        {
        MMBAdd(
          &R->MB,
          (char *)Value,
          NULL,
          mmbtNONE,
          0,
          0,
          NULL);
        }

      break;

    case mrmaNoAllocMaster:

      R->NoAllocMaster = MUBoolFromString((char *)Value,FALSE);
 
      break;

    case mrmaNodeDestroyPolicy:

      R->NodeDestroyPolicy = (enum MNodeDestroyPolicyEnum)MUGetIndexCI(
        (char *)Value,
        MNodeDestroyPolicy,
        FALSE,
        mndpSoft);

      break;

    case mrmaNodeFailureRsvProfile:

      MUStrDup(&R->NodeFailureRsvProfile,(char *)Value);

      break;

    case mrmaNoNeedNodes:

      if (R->Type == mrmtPBS)
        {
        R->PBS.NoNN = MUBoolFromString((char *)Value,FALSE);
        }
 
      break;

    case mrmaOMap:

      {
      char EMsg[MMAX_LINE];

      MUStrDup(&R->OMapServer,(char *)Value);

      if (MRMLoadOMap(R,(char *)Value,EMsg) == FAILURE)
        {
        MUStrDup(&R->OMapMsg,EMsg);
        }
      }    /* END BLOCK (case mrmaOMap) */

      break;

    case mrmaPreemptDelay:

      R->PreemptDelay = MUTimeFromString((char *)Value);

      break;

    case mrmaPort:

      if (Value != NULL)
        {
        if (Format == mdfString)
          {
          R->P[0].Port = (int)strtol((char *)Value,NULL,10);
          }
        else
          {
          R->P[0].Port = *(int *)Value;
          }
        }

      break;

    case mrmaProfile:

      MUStrDup(&R->Profile,(char *)Value);

      break;

    case mrmaProvDuration:

      if (Value != NULL)
        {
        if (Format == mdfString)
          {
          MSched.DefProvDuration = MUTimeFromString((char *)Value);
          }
        else
          {
          MSched.DefProvDuration = *(long *)Value;
          }
        }

      break;

    case mrmaPtyString:

      MUStrDup(&R->PTYString,(char *)Value);

      break;

    case mrmaUseVNodes:

      R->UseVNodes = MUBoolFromString((char *)Value,FALSE);
      
      break;

    case mrmaReqNC:

      if (Value != NULL)
        {
        if (Format == mdfString)
          {
          R->ReqNC = (int)strtol((char *)Value,NULL,10);
          }
        else
          {
          R->ReqNC = *(int *)Value;
          }
        }

      break;

    case mrmaResourceType:

      if (Format == mdfString)
        {
        mbitmap_t RTypes;
   
        char *ptr;
        char *TokPtr;
   
        char  tmpI;
   
        if (!strchr((char *)Value,'+'))
          bmclear(&R->RTypes);
   
        ptr = MUStrTok((char *)Value,"+,: \t\n ",&TokPtr);
   
        bmclear(&RTypes);
   
        while (ptr != NULL)
          {
          tmpI = MUGetIndexCI(ptr,MRMResourceType,FALSE,0);
   
          bmset(&RTypes,tmpI);
  
          ptr = MUStrTok(NULL,"+,: \t\n ",&TokPtr);
          }
   
        bmcopy(&R->RTypes,&RTypes);
   
        if (bmisset(&RTypes,mrmrtNetwork))
          {
          if (MSched.NetworkRM == NULL)
            {
            /* first network manager defined is default network manager for
               all requests */
   
            MSched.NetworkRM = R;
            }
          }
   
        if (bmisset(&RTypes,mrmrtProvisioning))
          {
          if (MSched.ProvRM == NULL)
            {
            /* first provisioning manager defined is default prov manager for
               all requests */
   
            MSched.ProvRM = R;
            }
          }
   
        if (bmisset(&RTypes,mrmrtLicense))
          {
          int index;
   
          /* initialize  mnatmdata_t */
   
          MSNLInit(&R->U.Nat.ResAvailICount);
          MSNLInit(&R->U.Nat.ResFreeICount);
          MSNLInit(&R->U.Nat.ResAvailCount);
          MSNLInit(&R->U.Nat.ResConfigCount);
   
          R->U.Nat.ResName = (char **)MUMalloc(sizeof(char *) * MSched.M[mxoxGRes]);
   
          for (index = 0;index < MSched.M[mxoxGRes];index++)
            {
            R->U.Nat.ResName[index] = (char *)MUMalloc(sizeof(char) * MMAX_NAME);
   
            R->U.Nat.ResName[index][0] = '\0';
            }
          } 
   
        if (bmisset(&RTypes,mrmrtStorage))
          {
          if (MSched.DefStorageRM == NULL)
            {
            /* first storage manager defined is default storage manager for
               all requests */
   
            MSched.DefStorageRM = R;
            }
   
          if (bmisclear(&R->FnList))
            {
            MRMFuncSetAll(R);
            }
   
          bmunset(&R->FnList,mrmWorkloadQuery);
          bmunset(&R->FnList,mrmJobSubmit);
          }
        }    /* END BLOCK (case mrmaResourceType) */

      break;

    case mrmaSBinDir:

      switch (R->Type)
        {
        case mrmtPBS:
        case mrmtNative:

          /* NOTE:  allow sbindir with BProc/TM native RM's */

          MUStrCpy(R->PBS.SBinDir,(char *)Value,sizeof(R->PBS.SBinDir));

          break;

        case mrmtWiki:
       
          MUStrCpy(R->Wiki.SBinDir,(char *)Value,sizeof(R->Wiki.SBinDir));

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (R->Type) */

      break;

    /* for "case mrmaServer" see mrmaFBServer above */

    case mrmaSetJob:

      MTJobAdd((char *)Value,&R->JSet);

      break;

    case mrmaSLURMFlags:

      {
      enum MSLURMFlagsEnum SLURMFlag;
      char   tmpLine[MMAX_LINE];
      char  *ptr;
      char  *TokPtr;

      /* FORMAT:  <FLAG>[,<FLAG>]... */

      MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,", \t\n",&TokPtr);

      while (ptr != NULL)
        {
        SLURMFlag = (enum MSLURMFlagsEnum)MUGetIndexCI(
          ptr,
          MSLURMFlags,
          FALSE,
          mslfNONE);

        switch(SLURMFlag)
          {
          case mslfNodeDeltaQuery:

            bmset(&R->Wiki.Flags,mslfNodeDeltaQuery);

            break;

          case mslfJobDeltaQuery:

            bmset(&R->Wiki.Flags,mslfJobDeltaQuery);

            break;

          case mslfCompressOutput:

            bmset(&R->Wiki.Flags,mslfCompressOutput);

            break;

          default:

            /* NO-OP */

            break;
          } /* END swtich (Flag) */

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);
        } /* END while (ptr !- NULL) */
      }
      break;

    case mrmaSoftTermSig:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        MUStrCpy(R->SoftTermSig,(char *)Value,sizeof(R->SoftTermSig));
        }
      else
        {
        sprintf(R->SoftTermSig,"%d",
          *(int *)Value);
        }

      break;

    case mrmaSQLData:

      if (R->ND.S == NULL)
        R->ND.S = (msql_t *)calloc(1,sizeof(msql_t));

      R->ND.S->Data = (char *)malloc(sizeof(char) * strlen((char *)Value) + 1);

      strcpy(R->ND.S->Data,(char *)Value);

      break;

    case mrmaStageThreshold:

      if (Value != NULL)
        {
        if (Format == mdfString)
          {
          R->StageThreshold = MUTimeFromString((char *)Value);
          }
        else
          {
          R->StageThreshold  = *(long *)Value;
          }

        R->StageThresholdConfigured = TRUE;
        }

      break;

    case mrmaState:

      R->State = (enum MRMStateEnum)MUGetIndexCI((char *)Value,MRMState,FALSE,mrmNONE);

      break;

    case mrmaSuspendSig:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        MUStrCpy(R->SuspendSig,(char *)Value,sizeof(R->SuspendSig));
        }
      else
        {
        sprintf(R->SuspendSig,"%d",
          *(int *)Value);
        }

      break;

    case mrmaSyncJobID:

      R->SyncJobID = MUBoolFromString((char *)Value,FALSE);

      break;

    case mrmaTrigger:

      {
      int tindex;

      mtrig_t *T;

      MTrigLoadString(
        &R->T,
        (char *)Value,
        TRUE,
        FALSE,
        mxoRM,
        R->Name,
        NULL,
        NULL);

      for (tindex = 0;tindex < R->T->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(R->T,tindex);

        MTrigInitialize(T);
        }   /* END for (tindex) */

      MOReportEvent(R,R->Name,mxoRM,mttStart,MSched.Time,TRUE);
      }     /* END BLOCK mrmaTrigger */

      break;

    case mrmaType:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        char *ptr;
        char *TokPtr;

        /* FORMAT:  <TYPE>[:<SUBTYPE>] */

        ptr = MUStrTok((char *)Value,":",&TokPtr);

        R->Type = (enum MRMTypeEnum)MUGetIndexCI(ptr,MRMType,FALSE,mrmtNONE);

        if (R->Type == mrmtTORQUE)
          {
          bmset(&R->IFlags,mrmifIsTORQUE);

          R->Type = mrmtPBS;
          }

        if (R->Type == mrmtNONE)
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"cannot create RM interface with invalid type '%s'",
            ptr);

          MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

          snprintf(tmpLine,sizeof(tmpLine),"invalid RM type '%s' specified, using default type",
            ptr);

          MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

          return(FAILURE);
          }

        bmset(&R->Languages,R->Type);

        if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
          {
          R->SubType = (enum MRMSubTypeEnum)MUGetIndexCI(
            ptr,
            MRMSubType,
            FALSE,
            mrmstNONE);

          switch (R->SubType)
            {
            case mrmstXT3:
            case mrmstXT4:
            case mrmstX1E:

              /* NOTE:  use node index must be enabled before MCPLoad() is called which 
                        can potentially create nodes */

              /* enable node index by default */

              if ((ptr = getenv("MOABUSENODEINDEX")) != NULL)
                {
                MSched.UseNodeIndex = MUBoolFromString(ptr,TRUE);
                }
              else
                {
                /* use NodeIndex if numeric hostnames specified */

                MSched.UseNodeIndex = MBNOTSET;
                }

              break;

            default:

              /* NO-OP */

              break;
            }  /* END switch (R->SubType) */
          }    /* END if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL) */

        /* native RM's must have sub-type specified in order to be considered explicit! */

        if ((R->Type != mrmtNative) || (R->SubType != mrmstNONE))
          R->TypeIsExplicit = TRUE;

        if (R->Type == mrmtMoab)
          {
          /* Set the default port if no port already specified */

          if (R->P[0].Port == 0)
            {
            R->P[0].Port = MDEF_SERVERPORT;

            if (MSched.SchedPolicy == mscpNONE)
              {
              MSched.SchedPolicy = mscpEarliestStart;
              }
            }
          }    /* END else if (R->Type == mrmtMoab) */
        }    /* END if (Format == mdfString) */

      break;

    case mrmaUpdateJobRMCreds:

      if (MUBoolFromString((char *)Value,TRUE) == FALSE)
        R->NoUpdateJobRMCreds = TRUE;

      break;

    case mrmaVariables:

      /* FORMAT:  <name>[=<value>][[,<name[=<value]]...] */

      {
      mln_t *tmpL;

      char *ptr;
      char *TokPtr;

      char *TokPtr2;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        MUStrTok(ptr,"=",&TokPtr2);

        if (MULLAdd(&R->Variables,ptr,NULL,&tmpL,MUFREE) == FAILURE)
          break;

        if (TokPtr2[0] != '\0')
          {
          MUStrDup((char **)&tmpL->Ptr,TokPtr2);
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mrmaVariables) */

      break;

    case mrmaVersion:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        MUStrCpy(R->APIVersion,(char *)Value,sizeof(R->APIVersion));
        }

      break;

    case mrmaVMOwnerRM:

      if (Format == mdfString)
        {
        char *RMName = (char *)Value;

        if (MRMFind(RMName,&R->VMOwnerRM) == FAILURE)
          MRMAdd(RMName,&R->VMOwnerRM);
        }
      else if (Format == mdfOther)
        {
        R->VMOwnerRM = (mrm_t *)Value;
        }

      break;

    default:

      /* attribute not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;    
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MRMSetAttr() */
/* END MRMSetAttr.c */
