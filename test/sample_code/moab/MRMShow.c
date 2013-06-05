/* HEADER */

      
/**
 * @file MRMShow.c
 *
 * Contains: MRMShow
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


 
/**
 * Report RM state, config, history, etc (process 'mdiag -R')
 *
 * NOTE:  will report accounting manager interfaces if RMID not
 *        explicitly specified
 *
 * NOTE:  support both '-v' and '-v -v' options
 *
 * @see MUIDiagnose() - parent
 * @see MAMShow() - child
 * @see MRMToXML() - child 
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param SpecRName (I) [optional]
 * @param RE (O)
 * @param String (O)
 * @param DFormat (I)
 * @param DModeBM (I) [mcmVerbose]
 */

int MRMShow(
  
  char                *Auth,
  char                *SpecRName,
  mxml_t              *RE,
  mstring_t           *String,
  enum MFormatModeEnum DFormat,
  mbitmap_t           *DModeBM)

  {
  int rmindex;
  int index;
  int findex;

  mbool_t ShowHeader = TRUE;
  mbool_t FailureRecorded;

  mbool_t RMDetected;

  char  tmpLine[MMAX_LINE];
  char  tmpName[MMAX_NAME];

  mxml_t *DE = NULL;
  mxml_t *RME;

  mrm_t  *SpecR;
  mrm_t  *R;
  mpsi_t *P;

  mbool_t ShowFlush = FALSE;

  mbool_t FTypeDetected[mrmLAST];

  const char *FName = "MRMShow";

  MDB(3,fSTRUCT) MLog("%s(%s,RE,String,%d)\n",
    FName,
    (SpecRName != NULL) ? SpecRName : "NULL",
    DFormat);

  /* initialize */

  if (DFormat != mfmXML)
    {
    if (String == NULL)
      {
      return(FAILURE);
      }

    MStringSet(String,"\0");

    MStringAppendF(String,"diagnosing resource managers\n\n");
    }
  else
    {
    DE = RE;
    }

  /* display global data */

  if (DFormat != mfmXML)
    {
    /* no-op */
    }
  else
    {
    mstring_t tmp(MMAX_LINE);

    RME = NULL;

    MXMLCreateE(&RME,(char *)MXO[mxoRM]);

    MXMLAddE(DE,RME);

    MXMLSetAttr(
      RME,
      (char *)MRMAttr[mrmaName],
      (void *)"GLOBAL",
      mdfString);

    MXMLSetAttr(
      RME,
      (char *)MRMAttr[mrmaType],
      (void *)tmp.c_str(),
      mdfString);
    }

  if (SpecRName != NULL)
    {
    MRMFind(SpecRName,&SpecR);
    }

  /* NOTE:  allow mode to specify verbose, diagnostics, etc */

  RMDetected = FALSE;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((!MUStrIsEmpty(SpecRName))  && (SpecR != R))
      continue;

    if (ShowHeader == TRUE)
      {
      ShowHeader = FALSE;
      }

    if (!bmisset(DModeBM,mcmVerbose) && !strcmp(R->Name,"internal"))
      {
      continue;
      }

    RMDetected = TRUE;

    if (DFormat == mfmXML)
      {
      RME = NULL;

      MRMToXML(R,&RME,NULL);

      /* NOTE:  incorporate mrmaMessage, diagnostics (NYI) */

      MXMLAddE(DE,RME);
      }

    if (DFormat != mfmXML)
      {
      enum MRMTypeEnum EffType;

      char tmpState[MMAX_LINE];

      /* display human readable text */

      snprintf(tmpName,sizeof(tmpName),"RM[%s]",
        R->Name);

      tmpLine[0] = '\0';

      snprintf(tmpState,sizeof(tmpState),"State: %s",
        (R->State != mrmsNONE) ? MRMState[R->State] : "---");

      MStringAppendF(String,"%-12s  %-10s",
        tmpName,
        tmpState);

      if ((R->Type == mrmtPBS) && (bmisset(&R->IFlags,mrmifIsTORQUE)))
        EffType = mrmtTORQUE;
      else
        EffType = R->Type;
       
      if (R->SubType != mrmstNONE)
        {
        MStringAppendF(String,"  Type: %s:%s%s",
          MRMType[EffType],
          MRMSubType[R->SubType],
          tmpLine);
        }
      else
        {
        MStringAppendF(String,"  Type: %s%s",
          MRMType[EffType],
          tmpLine);
        }

      if (!bmisclear(&R->RTypes))
        {
        if (bmisset(DModeBM,mcmVerbose) || (bmisset(&R->RTypes,mrmrtCompute)))
          {
          char tmpLine[MMAX_LINE];

          bmtostring(&R->RTypes,MRMResourceType,tmpLine);

          MStringAppendF(String,"  ResourceType: %s",
            tmpLine);
          }
        }

      MStringAppend(String,"\n");
      }  /* END if (DFormat != mfmXML) */

    if (R->P[0].HostName != NULL)
      {
      if (DFormat != mfmXML)
        {
        if (R->P[0].Port != 0)
          {
          MStringAppendF(String,"  Server:             %s:%d\n",
            R->P[0].HostName,
            R->P[0].Port);
          }
        else
          {
          MStringAppendF(String,"  Server:             %s\n",
            R->P[0].HostName);
          }

        if (R->ExtHost != NULL)
          {
          MStringAppendF(String,"  ExtHost:            %s\n",
            R->ExtHost);
          }
        }
      }    /* END if (R->P[0].HostName != NULL) */
    else if ((R->Type == mrmtMoab) && !bmisset(&R->Flags,mrmfClient))
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  WARNING:  SERVER attribute must be specified for peer interfaces\n");
        }
      else
        {
        MXMLAddChild(
          RME,
          "DIAGMSG",
          "SERVER attribute must be specified for peer interfaces",
          NULL);
        }
      }

    if (R->P[1].HostName != NULL)
      {
      /* more than one server defined */

      if (DFormat != mfmXML)
        {
        if (R->P[1].Port != 0)
          {
          MStringAppendF(String,"  FallBack Server:    %s:%d\n",
            R->P[1].HostName,
            R->P[1].Port);
          }
        else
          {
          MStringAppendF(String,"  Fallback Server:    %s\n",
            R->P[1].HostName);
          }
        }

      if (R->ActingMaster != 0)
        {
        if (DFormat != mfmXML)
          {
          MStringAppendF(String,"  WARNING:  primary server has failed.  fallback server '%s' is acting primary\n",
            R->P[R->ActingMaster].HostName);
          }
        else
          {
          MXMLAddChild(
            RME,
            "DIAGMSG",
            "primary server has failed.  fallback is acting primary",
            NULL);
          }
        }
      }    /* END if (R->P[1].HostName != NULL) */

    if (!bmisset(DModeBM,mcmVerbose))
      {
      continue;
      }

    if ((R->State == mrmsCorrupt) || (R->State == mrmsDown))
      {
      if (R->RestoreFailureCount > 0)
        {
        long UpdateTime;

        UpdateTime = 
          R->StateMTime +
          MAX(MSched.MaxRMPollInterval,
              MIN((MCONST_MINUTELEN << (R->RestoreFailureCount - 1)),MSched.RMRetryTimeCap)) - MSched.Time;

        /* NOTE: Exponetial backoff is only enabled for corrupt states. */

        if ((UpdateTime <= 0) || (R->State == mrmsDown))
          {
          if (DFormat != mfmXML)
            MStringAppendF(String,"  NOTE: will retry connection at next scheduling iteration (%d sequential failures)\n",
              R->RestoreFailureCount);
          }
        else
          {
          if (DFormat != mfmXML)
            {
            char TString[MMAX_LINE];

            MULToTString(UpdateTime,TString);

            MStringAppendF(String,"  NOTE: will retry connection in %s (%d sequential failures)\n",
              TString,
              R->RestoreFailureCount);

            MStringAppendF(String,"        use 'mrmctl -m state=enabled %s' to retry immediately\n",
              R->Name);
            }
          }
        }
      }    /* END if ((R->State == mrmsCorrupt) || (R->State == mrmsDown)) */

    if (DFormat != mfmXML)
      {
      if ((R->Type == mrmtPBS) && (R->Version > 910))
        {
        MStringAppendF(String,"  NoNeedNodes:          %s\n",
          MBool[R->PBS.NoNN]);
        }

      if (R->P[0].Timeout > 0)
        {
        if (R->P[0].Timeout > 1000)
          {
          MStringAppendF(String,"  Timeout:            %.2f ms\n",
            (double)R->P[0].Timeout / 1000.0);
          }
        else
          {
          MStringAppendF(String,"  Timeout:            %.2f sec\n",
            (double)R->P[0].Timeout / 1000.0);
          }
        }

      if (R->Description != NULL)
        {
        MStringAppendF(String,"  Description:        %s\n",
          R->Description);
        }

      if (R->MaxFailCount != MMAX_RMFAILCOUNT)
        {
        MStringAppendF(String,"  Max Fail/Iteration: %d\n",
          R->MaxFailCount);
        }

      if (R->JobIDFormatIsInteger == TRUE)
        {
        MStringAppendF(String,"  JobID Format:       %s\n",
          "integer");
        }

      if (bmisset(&R->IFlags,mrmifLocalQueue))
        {
        MStringAppendF(String,"  JobCounter:         %d\n",
          R->JobCounter);
        }

      if (R->Profile != NULL)
        {
        MStringAppendF(String,"  Profile:            %s\n",
          R->Profile);
        }
      }  /* END if (DFormat != mfmXML) */

    if (MRMFunc[R->Type].IsInitialized == FALSE)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  ALERT:  Moab not enabled for RM type '%s' (rebuild or contact support)\n",
          MRMType[R->Type]);
        }
      }

    if (DFormat != mfmXML)
      {
      /* NOTE:  display optional RM version, failures, and fault tolerance config */
  
      if (R->APIVersion[0] != '\0')
        {
        MStringAppendF(String,"  Version:            '%s'\n",
        R->APIVersion);

        if (R->Version == 0)
          {
          switch (R->Type)
            {
            case mrmtPBS:

              MStringAppendF(String,"  NOTE:  version info not correctly parsed\n");

              break;

            default:

              /* NO-OP */

              break;
            }
          }
        }
      else if (R->Version != 0)
        {
        MStringAppendF(String,"  Version:            %d\n",
          R->Version);
        }

      if (MRMAToString(R,mrmaClusterQueryURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Cluster Query URL:  %s\n",
          tmpLine);
        }

      if (MRMAToString(R,mrmaWorkloadQueryURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Workload Query URL: %s\n",
          tmpLine);
        }

      if (MRMAToString(R,mrmaCredCtlURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Cred Control URL:   %s\n",
          tmpLine);
        }
      }    /* END if (DFormat != mfmXML) */

    if (DFormat != mfmXML)
      {
      if (MRMAToString(R,mrmaJobStartURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Start URL:      %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmJobStart] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobStartURL (file://)\n");
          }
        }
      else if (MRMAToString(R,mrmaStartCmd,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Start Cmd:      %s\n",
          tmpLine);
        }

      if (MRMAToString(R,mrmaJobCancelURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Cancel URL:     %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmJobCancel] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobCancelURL (file://)\n");
          }
        }

      if (MRMAToString(R,mrmaJobModifyURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Modify URL:     %s\n",
          tmpLine);
   
        if (R->ND.Protocol[mrmJobModify] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobModifyURL (file://)\n");
          }
        }
   
      if (MRMAToString(R,mrmaJobSuspendURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Suspend URL:    %s\n",
          tmpLine);
   
        if (R->ND.Protocol[mrmJobSuspend] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobSuspendURL (file://)\n");
          }
        }
   
      if (MRMAToString(R,mrmaJobValidateURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Validate URL:   %s\n",
          tmpLine);
   
        if (R->ND.Protocol[mrmXJobValidate] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobValidateURL (file://)\n");
          }
        }
   
      if (MRMAToString(R,mrmaJobMigrateURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Migrate URL:    %s\n",
          tmpLine);
   
        if (R->ND.Protocol[mrmJobMigrate] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobMigrateURL (file://)\n");
          }
        }
   
      if (MRMAToString(R,mrmaJobRequeueURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Requeue URL:    %s\n",
          tmpLine);
   
        if (R->ND.Protocol[mrmJobRequeue] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobRequeueURL (file://)\n");
          }
        }
   
      if (MRMAToString(R,mrmaJobResumeURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Resume URL:     %s\n",
          tmpLine);
   
        if (R->ND.Protocol[mrmJobResume] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobResumeURL (file://)\n");
          }
        }
   
      if (MRMAToString(R,mrmaJobSubmitURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
        {
        MStringAppendF(String,"  Job Submit URL:     %s\n",
          tmpLine);
   
        if (R->ND.Protocol[mrmJobSubmit] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in JobSubmitURL (file://)\n");
          }
        }
      }  /* END if (DFormat != mfmXML) */

    if (MRMAToString(R,mrmaNodeMigrateURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  Node Migrate URL:    %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmXNodeMigrate] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in NodeMigrateURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaNodeModifyURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  Node Modify URL:    %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmNodeModify] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in NodeModifyURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaNodePowerURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  Node Power URL:     %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmXNodePower] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in NodePowerURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaNodeVirtualizeURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  Node Virtualize URL:     %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmXNodeVirtualize] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in NodeVirtualizeURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaParCreateURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  Partition Create URL:    %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmXParCreate] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in ParCreateURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaParDestroyURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  Partition Destroy URL:   %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmXParDestroy] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in ParDestroyURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaQueueQueryURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  Queue Query URL:    %s\n",
          tmpLine);
        }
      }

    if (MRMAToString(R,mrmaRMInitializeURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  RM Initialize URL:  %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmRMInitialize] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in RMInitializeURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaRMStartURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  RM Start URL:       %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmRMStart] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in RMStartURL (file://)\n");
          }

        if (!bmisset(&R->Flags,mrmfAutoSync))
          {
          MStringAppend(String,"  WARNING:  AUTOSYNC flag required to activate RMStartURL\n");
          }
        }
      }

    if (MRMAToString(R,mrmaRMStopURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  RM Stop URL:        %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmRMStop] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in RMStopURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaRsvCtlURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  RsvCtl URL:         %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmRsvCtl] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in RSVCTLURL (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaSystemModifyURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  System Modify URL:  %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmSystemModify] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in SystemModifyProtocol (file://)\n");
          }
        }
      }

    if (MRMAToString(R,mrmaSystemQueryURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  System Query URL:   %s\n",
          tmpLine);
        }
      }

    if (MRMAToString(R,mrmaResourceCreateURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"  ResourceCreateURL:  %s\n",
          tmpLine);

        if (R->ND.Protocol[mrmResourceCreate] == mbpFile)
          {
          MStringAppend(String,"  WARNING:  invalid protocol specified in ResourceCreateProtocol (file://)\n");
          }
        }
      }

    if (R->Env != NULL)
      {
      if (DFormat != mfmXML)
        {
        char tmpLine[MMAX_LINE];

        /* need to convert special env. delimeter into something human readable */

        MUStrCpy(tmpLine,R->Env,sizeof(tmpLine));

        MUStrReplaceChar(tmpLine,ENVRS_ENCODED_CHAR,';',FALSE);

        MStringAppendF(String,"  Environment:        %s\n",
          tmpLine);
        }
      }

    if (DFormat == mfmXML)
      {
      if (R->MB != NULL)
        {
        mxml_t *ME = NULL;

        MXMLCreateE(&ME,(char *)MRMAttr[mrmaMessages]);

        MMBPrintMessages(R->MB,mfmXML,TRUE,-1,(char *)ME,0);

        MXMLAddE(RME,ME);
        }

      /* NOTE:  RM resource-type specific load information reported w/in MRMToXML() */

      if (bmisset(&R->RTypes,mrmrtLicense) ||
          bmisset(&R->RTypes,mrmrtStorage) ||
          bmisset(&R->RTypes,mrmrtNetwork))
        {
        if (MRMAToString(R,mrmaClusterQueryURL,tmpLine,sizeof(tmpLine),0) == FAILURE)
          {
          MXMLAddChild(
            RME,
            "DIAGMSG",
            "cluster query interface required but not specified",
            NULL);
          }
        }

      continue;
      }  /* END if (DFormat == mfmXML) */

    /* NOTE:  no xml reported beyond this point */

    if (R->UseNodeCache == TRUE)
      {
      MStringAppend(String,"  INFO:  node cache enabled\n");
      }

    if (R->NC > 0)
      {
      if ((R->NC != 1) || 
          (bmisclear(&R->RTypes)) || 
          bmisset(&R->RTypes,mrmrtCompute))
        {
        MStringAppendF(String,"  Objects Reported:   Nodes=%d (%d procs)  Jobs=%d\n",
          R->NC,
          (R->PtIndex > 0) ? MPar[R->PtIndex].CRes.Procs : 0,
          R->JobCount);
        }

      if (bmisset(&R->RTypes,mrmrtLicense))
        {
        MStringAppendF(String,"  Licenses Reported:  %d types (%d of %d available)\n",
          R->U.Nat.CurrentUResCount,
          R->U.Nat.CurrentResAvailCount,
          R->U.Nat.CurrentResConfigCount);
 
        if (MSched.OFOID[mxoxGRes] != NULL)
          {
          MStringAppendF(String,"  WARNING:  license table is full, cannot add license '%s' - increase MAXGRES\n",
            MSched.OFOID[mxoxGRes]);
          }
        }  /* END else if (bmisset(&R->RTypes,mrmrtLicense)) */
      else
        {
        mnode_t *N;

        if (MNLGetNodeAtIndex(&R->NL,0,&N) == SUCCESS)
          {
          MStringAppendF(String,"  Nodes Reported:     %d (%s)\n",
            R->NC,
            N->Name);
          }
        else
          {
          MStringAppendF(String,"  Nodes Reported:     %d (N/A)\n",
            R->NC);
          }
        }
      }    /* END if (R->NC > 0) */
    else if (R->NC == -1)
      {
      /* still haven't gotten response back from cluster */

      MStringAppendF(String,"  NOTE:  nodes not yet reported (request pending)\n");
      }
    else if (R->JobCount > 0)
      {
      MStringAppendF(String,"  Objects Reported:   Jobs=%d\n",
        R->JobCount);
      }

    if (R->MaxJobsAllowed > 0)
      {
      MStringAppendF(String,"  NOTE: only %d jobs are being loaded from RM (see RMCFG[] MAXJOBS)\n",
          R->MaxJobsAllowed);
      }

    if (MRMAToString(R,mrmaFlags,tmpLine,sizeof(tmpLine),0) == SUCCESS)
      {
      MStringAppendF(String,"  Flags:              %s\n",
        tmpLine);
      }

    if (R->DefHighSpeedAdapter != NULL)
      {
      MStringAppendF(String,"  Default Adapter:    %s\n",
        R->DefHighSpeedAdapter);
      }

    if (R->MRM != NULL)
      {
      MStringAppendF(String,"  Master RM:          %s\n",
        R->MRM->Name);
      }

    if (R->PtIndex > 0)
      {
      MStringAppendF(String,"  Partition:          %s\n",
        MPar[R->PtIndex].Name);
      }

    if (bmisset(DModeBM,mcmVerbose) && (R->FnListSpecified == TRUE))
      {
      mstring_t FnList(MMAX_LINE);

      bmtostring(&R->FnList,MRMFuncType,&FnList);
      MStringAppendF(String,"  FnList:             %s\n",FnList.c_str());
      }

    if (R->JMax != NULL)
      {
      MStringAppendF(String,"  Max Template:       %s\n",
        R->JMax->Name);
      }

    if (R->JMin != NULL)
      {
      MStringAppendF(String,"  Min Template:       %s\n",
        R->JMin->Name);
      }

    if (R->JSet != NULL)
      {
      MStringAppendF(String,"  Set Template:       %s\n",
        R->JSet->Name);
      }

    if (R->JDef != NULL)
      {
      MStringAppendF(String,"  Default Template:   %s\n",
        R->JDef->Name);
      }

    if (R->NodeFailureRsvProfile != NULL)
      {
      MStringAppendF(String,"  NodeFailureRsvProfile: %s\n",
        R->NodeFailureRsvProfile);
      }

    if (R->StageThreshold > 0)
      {
      char TString[MMAX_LINE];

      MULToTString(R->StageThreshold,TString);

      MStringAppendF(String,"  Staging Threshold:  %s\n",
        TString);
      }

    if (!bmisclear(&R->LocalJobExportPolicy))
      {
      char tmpLine[MMAX_LINE];

      MRMAToString(R,mrmaJobExportPolicy,tmpLine,sizeof(tmpLine),0);

      MStringAppendF(String,"  Job Export Policy:  %s\n",
        tmpLine);
      }

    if ((R->TypeIsExplicit == FALSE) &&
        (R->Type != mrmtNative) && 
        (R->Type != mrmtMoab) &&
        (R->Type != mrmtS3) &&
        (!bmisset(&R->Flags,mrmfClient)))
      {
      int rmindex2;

      for (rmindex2 = 0;rmindex2 < MSched.M[mxoRM];rmindex2++)
        {
        if (MRM[rmindex2].Name[0] == '\0')
          break;

        if (MRMIsReal(&MRM[rmindex2]) == FALSE)
          continue;

        if (rmindex2 == rmindex)
          continue;

        if (MRM[rmindex2].Type != mrmtPBS)
          continue;

        MStringAppendF(String,"  WARNING:  TYPE attribute not explicitly specified and PBS interface configured elsewhere.  Conflicts may occur.\n");

        break;
        }  /* END for (rmindex2) */
      }    /* END if (R->TypeIsExplicit == FALSE) && ...) */

    if (R->State == mrmsActive)
      {
      if ((R->PtIndex > 0) && 
          (MPar[R->PtIndex].ConfigNodes > 0) &&
          (bmisclear(&MPar[R->PtIndex].Classes)))
        {
        /* no classes configured w/in partition */

        if ((R->Type != mrmtMoab) && 
            (R->Type != mrmtNative) && 
            (!bmisset(&R->IFlags,mrmifLocalQueue)))
          {
          MStringAppendF(String,"  WARNING:  no classes configured within partition.  (check resource manager configuration?)\n");
          }
        }

      if ((R->ClockSkewOffset > 30) || (R->ClockSkewOffset < -30))
        {
        char TString[MMAX_LINE];

        MULToTString(R->ClockSkewOffset,TString);

        MStringAppendF(String,"  WARNING:  clock skew detected (RM/Sched clocks offset by %s)\n",
          TString);
        }
      }    /* END if (R->State == mrmsActive) */

    if (bmisset(&R->RTypes,mrmrtLicense))
      {
      int IC;
      int ARes;
      int DRes;
      int CRes;

      IC = MAX(1,R->U.Nat.ICount);

      MStringAppendF(String,"  License Stats:      Avg License Avail:   %.2f  (%d iterations)\n",
        ((double)(MSNLGetIndexCount(&R->U.Nat.ResAvailCount,0)) / IC),
        R->U.Nat.ICount);

      MStringAppendF(String,"  Iteration Summary:  Idle: %.2f  Active: %.2f  Busy: %.2f\n",
        (double)(MSNLGetIndexCount(&R->U.Nat.ResFreeICount,0)) / IC * 100.0,
        (double)(MSNLGetIndexCount(&R->U.Nat.ResAvailICount,0) - MSNLGetIndexCount(&R->U.Nat.ResFreeICount,0)) / IC * 100.0,
        (double)(R->U.Nat.ICount - MSNLGetIndexCount(&R->U.Nat.ResAvailICount,0)) / IC * 100.0);

      if (bmisset(DModeBM,mcmVerbose) && (R->U.Nat.N != NULL))
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

          MStringAppendF(String,"  License %-16s  %3d of %3d available  (Idle: %.2f%%  Active: %.2f%%)\n",
            R->U.Nat.ResName[lindex],
            (ARes - DRes < 0) ? 0 : (ARes - DRes),
            CRes,
            CRes == 0 ? 0.0 : ((double)ARes / CRes * 100.0),
            CRes == 0 ? 0.0 : ((double)(CRes - ARes) / CRes * 100.0));
          }  /* END for (lindex) */
        }
      }      /* END if (bmisset(&R->RTypes,mrmrtLicense)) */

    if (bmisset(&R->RTypes,mrmrtNetwork) || bmisset(&R->RTypes,mrmrtStorage))
      {
      /* validate 'clusterquery' interface */

      if (MRMAToString(R,mrmaClusterQueryURL,tmpLine,sizeof(tmpLine),0) == FAILURE)
        {
        MStringAppendF(String,"  WARNING:  cluster query interface MUST be specified for %s rm\n",
          (bmisset(&R->RTypes,mrmrtNetwork)) ? "network" : "storage");
        }

      /* report RM resources */

      if (MNLIsEmpty(&R->NL))
        {
        MStringAppendF(String,"  WARNING:  no resources reported for %s rm\n",
          (bmisset(&R->RTypes,mrmrtNetwork)) ? "network" : "storage");
        }
      else
        {
        MStringAppendF(String,"  INFO:  logical node '%s' reported for %s resources\n",
          MNLGetNodeName(&R->NL,0,NULL),
          (bmisset(&R->RTypes,mrmrtNetwork)) ? "network" : "storage");
        }
      }  /* END if (bmisset(R->RTypes,mrmrtNetwork) || ...) */
 
    if (R->P[0].WireProtocol != mwpNONE)
      {
      MStringAppendF(String,"  P[0]: WireProtocol: '%s'  SocketProtocol: '%s'\n",
        MWireProtocol[R->P[0].WireProtocol],
        MSockProtocol[R->P[0].SocketProtocol]);
      }

    if (bmisset(DModeBM,mcmVerbose) || (R->P[0].HostName != NULL))
      {
      /* displayed above */

      /*
      MStringAppendF(String,"  P[0]: Host: '%s'  Port: %d\n",
        (R->P[0].HostName != NULL) ? R->P[0].HostName : "",
        R->P[0].Port);
      */
      }

    if ((R->P[0].CSAlgo != mcsaNONE) || 
        (R->P[0].WireProtocol == mwpS32) || 
        (R->Type == mrmtMoab))
      {
      /* key should be set */

      if (R->P[0].CSKey == NULL)
        {
        if (R->Type == mrmtMoab)
          {
          MStringAppendF(String,"  WARNING:  secret key not set, using default server key - check moab-private.cfg\n");
          }
        else
          {
          MStringAppendF(String,"  NOTE:  secret key not set (algo: %s)\n",
            MCSAlgoType[R->P[0].CSAlgo]);
          }        
        }
      else if (bmisset(DModeBM,mcmVerbose))
        {
        MStringAppendF(String,"  NOTE:  secret key set (algo: %s)\n",
          MCSAlgoType[(R->P[0].CSAlgo != mcsaNONE) ? R->P[0].CSAlgo : MSched.DefaultCSAlgo]);
        }
      }

    if (R->AuthType == mrmapCheckSum) 
      {
      if (R->P[0].CSKey == NULL)
        {
        if (MSched.PvtConfigBuffer == NULL)
          MStringAppendF(String,"  WARNING:  checksum authorization set but secret key not specified - moab-private.cfg file does not exist\n");
        else
          MStringAppendF(String,"  WARNING:  checksum authorization set but secret key not specified - check CLIENTCFG parameter in moab-private.cfg\n");
        }
      else
        {
        MStringAppendF(String,"  NOTE:  checksum authorization set (secret key: %.1s...)\n",
          R->P[0].CSKey);
        }
      }

    if (R->P[1].WireProtocol != mwpNONE)
      {
      MStringAppendF(String,"  P[1]: WireProtocol: '%s'  SocketProtocol: '%s'\n",
        MWireProtocol[R->P[1].WireProtocol],
        MSockProtocol[R->P[1].SocketProtocol]);

      MStringAppendF(String,"  P[1]: Host: '%s'  Port: %d\n",
        (R->P[1].HostName != NULL) ? R->P[1].HostName : "",
        R->P[1].Port);

      if ((R->P[1].CSAlgo != mcsaNONE) || (R->P[1].WireProtocol == mwpS32))
        {
        /* key should be set */
 
        if (R->P[0].CSKey == NULL)
          {
          MStringAppendF(String,"  P[1]: secret key not set (algo: %s)\n",
            MCSAlgoType[R->P[1].CSAlgo]);
          }
        else if (bmisset(DModeBM,mcmVerbose))
          {
          MStringAppendF(String,"  P[1]: secret key set (algo: %s)\n",
            MCSAlgoType[(R->P[1].CSAlgo != mcsaNONE) ? R->P[1].CSAlgo : MSched.DefaultCSAlgo]);
          }
        }
      }

    if (R->EPort != 0)
      {
      if (R->EventTime > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(MSched.Time - R->EventTime,TString);

        MStringAppendF(String,"  Event Management:   EPORT=%d  (last event: %s)\n",
          R->EPort,
          TString);
        }
      else
        {
        MStringAppendF(String,"  Event Management:   EPORT=%d  (no events received)\n",
          R->EPort);
        }
      }
    else if (bmisset(DModeBM,mcmVerbose))
      {
      if (!bmisset(&R->IFlags,mrmifLocalQueue))
        {
        MStringAppendF(String,"  Event Management:   (event interface disabled)\n");
        }
      }

#ifdef MFORCERMTHREADS
    MStringAppendF(String,"  NOTE:  interface is multi-threaded, timeout=%.2f seconds\n",
      (double)R->P[0].Timeout / MDEF_USPERSECOND);
#endif /* MFORCERMTHREADS */

    if (R->ConfigFile[0] != '\0')
      {
      MStringAppendF(String,"  Config File:        %s\n",
        R->ConfigFile);
      }

    /* NOTE: spooldir used in grid, by msub, triggers, and native RM tools */

    if (MSched.SpoolDir[0] != '\0')
      {
      int     Perm;
      int     RequestedPerm;

      if (bmisset(&MSched.Flags,mschedfStrictSpoolDirPermissions))
        RequestedPerm = MCONST_STRICTSPOOLDIRPERM;
      else
        RequestedPerm = MCONST_SPOOLDIRPERM;
  
      if (MFUGetAttributes(MSched.SpoolDir,&Perm,NULL,NULL,NULL,NULL,NULL,NULL) == FAILURE)
        {
        MStringAppendF(String,"  WARNING:  spool directory '%s' does not exist\n",
          MSched.SpoolDir);
        }
      else if ((Perm & RequestedPerm) != RequestedPerm)
        {
        MStringAppendF(String,"  WARNING:  spool directory '%s' has incorrect permissions\n",
          MSched.SpoolDir);
        }
      }

    /* Compute RM's can have data rm's too. Only print out that its not configured if the type is moab. */

    if (R->DataRM != NULL) 
      {
      MStringAppendF(String,"  Data RM:            %s\n",
        R->DataRM[0]->Name);

      if (MSched.UID != 0)
        {
        MStringAppendF(String,"  INFO:     most data managers require admin privileges - currently running with UID %d\n",
          MSched.UID);
        }
      }

    /* display RM-specific attributes */

    switch (R->Type)
      {
      case mrmtPBS:

        if (R->PBS.PBS5IsEnabled == TRUE)
          {
          MStringAppendF(String,"  NOTE:  PBSv5 protocol enabled\n");
          }

        if (R->PBS.S3IsEnabled == TRUE)
          {
          MStringAppendF(String,"  NOTE:  SSS protocol enabled\n");
          }

        break;

      case mrmtLL:

        /* NO-OP */

        break;

      case mrmtWiki:

        if (bmisset(&R->Wiki.Flags,mslfNodeDeltaQuery))
          MStringAppendF(String,"  NOTE:  delta node query enabled\n");

        if (bmisset(&R->Wiki.Flags,mslfJobDeltaQuery))
          MStringAppendF(String,"  NOTE:  delta job query enabled\n");

        if (bmisset(&R->Wiki.Flags,mslfCompressOutput))
          MStringAppendF(String,"  NOTE:  range-based tasklists enabled\n");

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (R->Type) */

    if (!bmisset(&R->Flags,mrmfClient))
      {
      /* NOTE:  currently do not diagnose 'relative' paths */

      if ((R->SubmitCmd != NULL) && (R->SubmitCmd[0] == '/'))
        {
        mbool_t IsExe;

        MStringAppendF(String,"  Submit Command:     %s\n",
          R->SubmitCmd);

        if (MFUGetInfo(R->SubmitCmd,NULL,NULL,&IsExe,NULL) == FAILURE)
          {
          MStringAppendF(String,"  WARNING:  invalid submit command specified (file does not exist)\n");
          }
        else if (IsExe == FALSE)
          {
          MStringAppendF(String,"  WARNING:  invalid submit command specified (file is not executable)\n");
          }
        }    /* END if (R->SubmitCmd != NULL) */
      else if ((MSched.LocalQueueIsActive == TRUE) && (R->Type == mrmtPBS))
        {
        char exec[MMAX_LINE];

        if (MFUGetFullPath(
              MDEF_PBSQSUBPATHNAME,
              MUGetPathList(),
              NULL,
              NULL,
              TRUE,
              exec,
              sizeof(exec)) == FAILURE)
          {
          MDB(1,fSTRUCT) MLog("  ALERT:    exec '%s' cannot be located or is not executable (errno: %d '%s')\n",
            MDEF_PBSQSUBPATHNAME,
            errno,
            strerror(errno));

          MStringAppendF(String,"  WARNING:  invalid default submit command - %s (file does not exist)\n",
            MDEF_PBSQSUBPATHNAME);
          }    /* END if (MFUGetFullPath() == FAILURE) */

        MUStrDup(&R->SubmitCmd,exec);
        
        MStringAppendF(String,"  Submit Command:     %s\n",
          R->SubmitCmd);
        }
      }    /* END if (!bmisset(R->Flags,mrmfClient)) */
    else  
      {
      /* client */

      if (R->GridSandboxExists == TRUE)
        {
        MStringAppendF(String,"  NOTE:     grid sandbox applied to this RM\n");
        }

      /* look for Peer authentication */
      /* NYI: lookup peer in moab-private to see if it is there to differentiate between peername and RM:peername */

      if (R->P[0].CSKey == NULL)
        {
        /* peer not rm type */

        MStringAppendF(String,"  WARNING:  peer not located in moab-private.cfg\n");
        } 
      }    /* END else (!bmisset(R->Flags,mrmfClient)) */

    if (R->SubmitPolicy != mrmspNONE)
      {
      MStringAppendF(String,"  Submit Policy:      %s\n",
        MRMSubmitPolicy[R->SubmitPolicy]);
      }

    if (R->MaxJobPerMinute > 0)
      {
      MStringAppendF(String,"  MaxJobPerMinute:    %d\n",
        R->MaxJobPerMinute);
      }

    if (R->DefaultC != NULL)
      {
      MStringAppendF(String,"  DefaultClass:       %s\n",
        R->DefaultC->Name);
      }

    if (R->DefOS > 0)
      {
      MStringAppendF(String,"  DefaultOpSys:       %s\n",
        MAList[meOpsys][R->DefOS]);
      }

    if (R->JDef != NULL)
      {
      MStringAppendF(String,"  JobTemplate:        %s\n",
        R->JDef->Name);
      }

    if (R->OMapServer != NULL)
      {
      MStringAppendF(String,"  OMap Server:        %s\n",
        R->OMapServer);

      /* check for possible errors */

      if ((R->OMapMsg != NULL) && (R->OMapMsg[0] != '\0'))
        {
        MStringAppendF(String,"  WARNING:  object map is corrupt - %s\n",
          R->OMapMsg);
        }

      if ((R->OMap == NULL) ||
          (R->OMap[0].TransName[0] == '\0'))
        {
        MStringAppendF(String,"  WARNING:  object map %s cannot be loaded\n",
          R->OMapServer);
        }   
      }    /* END if (R->OMapServer != NULL) */

    if (R->CMapEMsg != NULL)
      {
      MStringAppendF(String,"  WARNING:  credential mapping failure - %s\n",
        R->CMapEMsg);
      }

    if (R->FMapEMsg != NULL)
      {
      MStringAppendF(String,"  WARNING:  file mapping failure - %s\n",
        R->FMapEMsg);
      }

    /* display access list info */

    if (R->AuthU != NULL)
      MStringAppendF(String,"  NOTE:  authorized user list explicitly specified\n");

    if (R->AuthG != NULL)
      MStringAppendF(String,"  NOTE:  authorized group list explicitly specified\n");

    if (R->AuthA != NULL)
      MStringAppendF(String,"  NOTE:  authorized account list explicitly specified\n");

    if (R->AuthQ != NULL)
      MStringAppendF(String,"  NOTE:  authorized QOS list explicitly specified\n");

    if (R->AuthC != NULL)
      MStringAppendF(String,"  NOTE:  authorized class list explicitly specified\n");

    if (R->MaxStageBW > 0.0)
      {
      MStringAppendF(String,"  Bandwidth:          %.2fM\n",
        R->MaxStageBW);
      }

    if (R->NoAllocMaster == TRUE)
      MStringAppendF(String,"  NoAllocMaster       TRUE\n");

    /* NOTE:  stats in ms */

    /* NOTE:  only report performance/availability for primary peer */

    P = &R->P[0];  

    if (R->JobStartCount > 0)
      {
      MStringAppendF(String,"  Total Jobs Started: %d\n",
        R->JobStartCount);
      }

    if (R->Variables != NULL)
      {
      char tmpLine[MMAX_LINE];

      MRMAToString(R,mrmaVariables,tmpLine,sizeof(tmpLine),0);

      if (tmpLine[0] != '\0')
        {
        MStringAppendF(String,"  Variables:          %s\n",
          tmpLine);
        }
      }   /* END if (R->Variables != NULL) */

    if (P->RespTotalCount[0] > 0)
      {
      MStringAppendF(String,"  RM Performance:     AvgTime=%.2fs  MaxTime=%.2fs  (%d samples)\n",
        (double)P->RespTotalTime[0] / P->RespTotalCount[0] / 1000,
        (double)P->RespMaxTime[0] / 1000,
        P->RespTotalCount[0]);

      if (bmisset(DModeBM,mcmVerbose2))
        {
        int findex;

        for (findex = 1;findex < MMAX_PSFUNC;findex++)
          {
          if (P->RespTotalCount[findex] <= 0)
            continue;

          MStringAppendF(String,"    %16s:   AvgTime=%.2fs  MaxTime=%.2fs  (%d samples)\n",
            MRMFuncType[findex],
            (double)P->RespTotalTime[findex] / P->RespTotalCount[findex] / 1000,
            (double)P->RespMaxTime[findex] / 1000,
            P->RespTotalCount[findex]);
          }  /* END for (findex) */
        }
      }

    /* rm languages supported */

    if (!bmisset(&R->Flags,mrmfClient))
      {
      char *s;

      MStringAppendF(String,"  RM Languages:       ");

      s = bmtostring(&R->Languages,MRMType,tmpLine,",","-");
      if (NULL != s)
        MStringAppendF(String,"%s\n",s);
      else
        MStringAppendF(String,"\n");

      MStringAppendF(String,"  RM Sub-Languages:   ");

      s = bmtostring(&R->Languages,MRMType,tmpLine,",","-");
      if (NULL != s)
        MStringAppendF(String,"%s\n",s);
      else
        MStringAppendF(String,"\n");
      }

    /* authorized users */

    if (R->AuthU != NULL)
      {
      int uindex;

      MStringAppendF(String,"  Authorized Users:   ");

      for (uindex = 0;uindex < MMAX_CREDS;uindex++)
        {
        if (R->AuthU[uindex] == NULL)
          break;

        if (uindex == 0)
          {
          MStringAppendF(String,"%s",
            R->AuthU[uindex]);
          }
        else
          {
          MStringAppendF(String,",%s",
            R->AuthU[uindex]);
          }
        }

      MStringAppendF(String,"\n");
      }  /* END if (R->AuthU != NULL) */

    /* messages */

    if (R->MB != NULL)
      {
      char tmpBuf[MMAX_BUFFER];

      MStringAppendF(String,"\n");

      MMBPrintMessages(R->MB,mfmHuman,FALSE,-1,tmpBuf,sizeof(tmpBuf));

      MStringAppend(String,tmpBuf);
      }

    FailureRecorded = FALSE;

    memset(FTypeDetected,0,sizeof(FTypeDetected));

    for (index = 0;index < MMAX_PSFAILURE;index++)
      {
      int index2;
      int findex2;

      findex = (index + P->FailIndex) % MMAX_PSFAILURE;

      if (P->FailTime[findex] <= 0)
        continue;

      if (FailureRecorded == FALSE)
        {
        MStringAppendF(String,"\nRM[%s] Failures:\n",
          R->Name);

        FailureRecorded = TRUE;
        }

      if (FTypeDetected[P->FailType[findex]] == TRUE)
        {
        continue;
        }

      FTypeDetected[P->FailType[findex]] = TRUE;

      /* display failure information */

      MStringAppendF(String,"  %-15s  (%d of %d failed)\n",
        MRMFuncType[P->FailType[findex]],
        P->RespFailCount[P->FailType[findex]],
        P->RespTotalCount[P->FailType[findex]]);
    
      /* NOTE:  FailMsg[] rolls, P->FailIndex points to next available slot */
      /*        findex2 points to rolled index */
 
      for (index2 = index;index2 < MMAX_PSFAILURE;index2++)
        {
        char TString[MMAX_LINE];

        findex2 = (index2 + P->FailIndex) % MMAX_PSFAILURE;

        if (P->FailTime[findex2] <= 0)
          continue;

        if (P->FailType[findex2] != P->FailType[findex])
          continue;

        /* display failure information */

        MULToTString(P->FailTime[findex2] - (long)MSched.Time,TString);

        MStringAppendF(String,"    %11.11s  '%s'\n",
          TString,
          (P->FailMsg[findex2] != NULL) ? P->FailMsg[findex2] : "no msg");
        }  /* END for (index2) */ 
      }    /* END for (index) */

    if ((FailureRecorded == TRUE) || (P->RespTotalCount[0]))
      {
      ShowFlush = TRUE;
      }

    if (bmisset(&R->Flags,mrmfClient))
      {
      if (R->UTime == 0)
        {
        MStringAppendF(String,"  WARNING:  no contact from peer resource manager\n");
        }
      else
        {
        char TString[MMAX_LINE];

        MULToTString(MSched.Time - R->UTime,TString);

        MStringAppendF(String,"  Last Contact:       %s (%s)\n",
          TString,
          MULToATString((mulong)R->UTime,NULL));
        }
      }

    MStringAppendF(String,"\n");
    }  /* END for (rmindex) */

  if ((DFormat != mfmXML) && (ShowFlush == TRUE))
    {
    MStringAppendF(String,"\nNOTE:  use 'mrmctl -f messages <RMID>' to clear stats/failures\n");
    }

  if (MSched.ParBufOverflow == TRUE)
    {
    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"ALERT:    inadequate partitions available.  Requested RM/partition may not be created.\n       Increase partition count or contact support@adaptivecomputing.com\n");
      }
    else
      {
      /* NYI */
      }
    }

  /* NOTE:  temp hack.  add AM diagnostics to output */

  if (MUStrIsEmpty(SpecRName))
    {
    /* RM ID not specified */

    if (DFormat != mfmXML)
      {
      mstring_t tmp(MMAX_LINE);

      MAMShow(NULL,NULL,&tmp,DFormat);

      MStringAppendF(String,"\n%s",tmp.c_str());
      }
    else
      {
      MAMShow(NULL,DE,NULL,DFormat);
      }

    if (DFormat != mfmXML)
      {
      mstring_t tmp(MMAX_LINE);
 
      MIDShow(NULL,NULL,&tmp,DFormat,DModeBM);

      MStringAppendF(String,"\n%s",tmp.c_str());
      }
    else
      {
      MIDShow(NULL,DE,NULL,DFormat,DModeBM);
      }
    }

  if (RMDetected == FALSE)
    {
    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"ALERT:  no resource manager configured\n"); 
      }
    }

  return(SUCCESS);
  }  /* END MRMShow() */
/* END MRMShow.c */
