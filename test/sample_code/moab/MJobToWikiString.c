/* HEADER */

      
/**
 * @
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Converts a job to a WIKI string.
 *
 * @param J      (I) - the job to translate
 * @param Msg    (I) [optional] - optional message
 * @param String (O) - output mstring_t. Must be construct'ed prior to call.
 */

int MJobToWikiString(
  mjob_t     *J,
  const char *Msg,
  mstring_t  *String)

  {
  mreq_t *RQ[MMAX_REQ_PER_JOB];

  int TC;

  if ((J == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  RQ[0] = J->Req[0];

  if (RQ[0] == NULL)
    {
    return(FAILURE);
    }

  TC = J->Request.TC;

  if (TC == 0)
    TC = 1;

  /* NAME */

  MStringSet(String,J->Name);

  /* REQUESTED NODE COUNT */

  if (J->Request.NC > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaRequestedNC],
      J->Request.NC);
    }

  /* REQUESTED TASK COUNT */

  if (J->Request.TC > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaRequestedTC],
      J->Request.TC);
    }

  /* USER */

  if (J->Credential.U != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaUName],
      J->Credential.U->Name);
    }

  /* GROUP */

  if (J->Credential.G != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaGName],
      J->Credential.G->Name);
    }

  /* WALLTIME */

  if (J->SpecWCLimit[0] > 0)
    {
    MStringAppendF(String," %s=%ld",
      MWikiJobAttr[mwjaWCLimit],
      J->SpecWCLimit[0]);
    }

  /* JOB STATE */

  MStringAppendF(String," %s=%s",
    MWikiJobAttr[mwjaState],
    MJobState[J->State]);

  /* CLASSES */

  if (J->Credential.C != NULL)
    {
    MStringAppendF(String," %s=[%s]",
      MWikiJobAttr[mwjaRClass],
      J->Credential.C->Name);
    }

  /* SUBMIT TIME */

  if (J->SubmitTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaSubmitTime],
      J->SubmitTime);
    }

  /* DISPATCH TIME */

  if (J->DispatchTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaDispatchTime],
      J->DispatchTime);
    }

  /* CHECKPOINT START TIME */

  if (J->Ckpt != NULL)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaCheckpointStartTime],
      J->Ckpt->StoredCPDuration);
    }

  /* START TIME */

  if (J->StartTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaStartTime],
      J->StartTime);
    }

  /* COMPLETION TIME */

  if (J->CompletionTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaCompletionTime],
      J->CompletionTime);
    } 

  /* ARCHITECTURE */

  if (RQ[0]->Arch > 0)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaRArch],
      MAList[meArch][RQ[0]->Arch]);
    }

  /* OS */

  if (RQ[0]->Opsys > 0)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaROpsys],
      MAList[meOpsys][RQ[0]->Opsys]);
    }

  /* MEM COMPARATOR */

  if ((MComp[(int)RQ[0]->ReqNRC[mrMem]] != NULL) &&
      (strcmp(MComp[(int)RQ[0]->ReqNRC[mrMem]],"NC")))
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaRMemCmp],
      MComp[(int)RQ[0]->ReqNRC[mrMem]]);
    }

  /* MEM */

  if (RQ[0]->ReqNR[mrMem] > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaRMem],
      RQ[0]->ReqNR[mrMem]);
    }

  /* DISK COMPARATOR */

  if ((MComp[(int)RQ[0]->ReqNRC[mrDisk]] != NULL) &&
      (strcmp(MComp[(int)RQ[0]->ReqNRC[mrDisk]],"NC")))
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaRDiskCmp],
      MComp[(int)RQ[0]->ReqNRC[mrDisk]]);
    }

  /* DISK */

  if (RQ[0]->ReqNR[mrDisk] > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaRDisk],
      RQ[0]->ReqNR[mrDisk]);
    }

  /* FEATURES */

  if (!bmisclear(&RQ[0]->ReqFBM))
    {
    char Features[MMAX_LINE];

    MUNodeFeaturesToString('\0',&RQ[0]->ReqFBM,Features);

    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaRFeatures],
      Features);
    }

  /* SYSTEM QUEUE TIME */

  if (J->SystemQueueTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaSystemQueueTime],
      J->SystemQueueTime);
    }

  /* TASKS PER NODE */

  if (RQ[0]->TasksPerNode > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaTaskPerNode],
      RQ[0]->TasksPerNode);
    }

  /* REQUESTED QOS */

  if (J->QOSRequested != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaRequestedQOS],
      J->QOSRequested->Name);
    }

  /* QOS */

  if (J->Credential.Q != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaQOS],
      J->Credential.Q->Name);
    }

  /* FLAGS */

  mstring_t tmpString(MMAX_LINE);

  MJobFlagsToMString(NULL,&J->Flags,&tmpString);

  if (!tmpString.empty())
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaFlags],
      tmpString.c_str());
    }

  /* ACCOUNT */

  if (J->Credential.A != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaAccount],
      J->Credential.A->Name);
    }

  /* COMMAND (quoted) */

  if (J->Env.Cmd != NULL)
    {
    MStringAppendF(String," %s=\"%s\"",
      MWikiJobAttr[mwjaCommand],
      J->Env.Cmd);
    }

  /* RMX STRING (quoted) */

  MStringSet(&tmpString,"");
  MRMXToMString(J,&tmpString,&J->RMXSetAttrs);

  if (!tmpString.empty())
    {
    MStringAppendF(String," %s=\"%s\"",
      MWikiJobAttr[mwjaRMXString],
      tmpString.c_str());
    }

  /* BYPASS COUNT */

  if (J->BypassCount > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaBypassCount],
      J->BypassCount);
    }

  /* PS UTILIZED */

  if (J->PSUtilized > 0)
    {
    MStringAppendF(String," %s=%.2f",
      MWikiJobAttr[mwjaPSUtilized],
      J->PSUtilized);
    }

  /* PARTITION */

  if (RQ[0]->PtIndex > 0)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaPartition],
      MPar[RQ[0]->PtIndex].Name);
    }

  /* PROCS */

  if (RQ[0]->DRes.Procs > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaDProcs],
      RQ[0]->DRes.Procs * RQ[0]->TaskCount);
    }

  /* MEM */

  if (RQ[0]->DRes.Mem > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaDMem],
      RQ[0]->DRes.Mem * RQ[0]->TaskCount);
    }

  /* DISK */

  if (RQ[0]->DRes.Disk > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaDDisk],
      RQ[0]->DRes.Disk * RQ[0]->TaskCount);
    }

  /* SWAP */

  if (RQ[0]->DRes.Swap > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaDSwap],
      RQ[0]->DRes.Swap * RQ[0]->TaskCount);
    }

  /* STARTDATE */

  if (J->SpecSMinTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaStartDate],
      J->SpecSMinTime);
    }

  /* ENDDATE */

  if (J->CMaxDate > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiJobAttr[mwjaEndDate],
      J->CMaxDate);
    }

  /* TASKMAP */

  if (((J->DestinationRM != NULL) && bmisset(&J->DestinationRM->Flags,mrmfCompressHostList)) ||
      ((J->SubmitRM != NULL) && bmisset(&J->SubmitRM->Flags,mrmfCompressHostList)))
    {
    mnl_t     tmpNodeList;
    mstring_t tmpNLString(MMAX_LINE);

    MNLInit(&tmpNodeList);

    MNLCopy(&tmpNodeList,&J->NodeList);

    /* sort the list lexicographically */

    MUNLSortLex(&tmpNodeList,MNLCount(&J->NodeList));

    MUNLToRangeString(
      &tmpNodeList,
      NULL,
      -1,
      TRUE,
      TRUE,
      &tmpNLString);

    MNLFree(&tmpNodeList);

    if (!tmpNLString.empty())
      {
      MStringAppendF(String," %s=%s",
        MWikiJobAttr[mwjaTaskMap],
        tmpNLString.c_str());
      }
    } /* END if (((J->DRM != NULL) && bmisset(&J->DRM->Flags,mrmfCompressHostList)) ... */
  else if (J->TaskMap != NULL)
    {
    int tindex;
    mbool_t TMPrinted = FALSE;

    for (tindex = 0;J->TaskMap[tindex] != -1;tindex++)
      {
      if (TMPrinted == FALSE)
        {
        MStringAppendF(String," %s=%s",
          MWikiJobAttr[mwjaTaskMap],
          MNode[J->TaskMap[tindex]]->Name);

        TMPrinted = TRUE;
        }
      else
        {
        MStringAppend(String,",");
        MStringAppend(String,MNode[J->TaskMap[tindex]]->Name);
        }
      } /* END for (tindex = 0;J->TaskMap[tindex] != -1;tindex++) */
    } /* END else if (J->TaskMap != NULL) */

  /* RMID */

  if (J->SubmitRM != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaSRM],
      J->SubmitRM->Name);
    }

  /* HOSTLIST */

  if (bmisset(&J->IFlags,mjifHostList) &&
      (!MNLIsEmpty(&J->ReqHList)))
    {
    mstring_t tmpHLString(MMAX_LINE);

    MNLToMString(&J->ReqHList,MBNOTSET,",",'\0',-1,&tmpHLString);

    MStringAppendF(String, " %s=%s",
      MWikiJobAttr[mwjaHostList],
      tmpHLString.c_str());
    } /* END if (bmisset(&J->IFlags,mjifHostList) && ... */

  /* RSVID */

  if (J->ReqRID != NULL)
    {
    MStringAppendF(String," %s=%s%s",
      MWikiJobAttr[mwjaReqRsv],
      (bmisset(&J->IFlags,mjifNotAdvres)) ? "!" : "",
      J->ReqRID);
    }

  /* TEMPLATES */

  if (J->Credential.Templates != NULL)
    {
    int jindex;

    MStringAppendF(String," %s=",
      MWikiJobAttr[mwjaTemplate]);

    for (jindex = 0;J->Credential.Templates[jindex] != NULL;jindex++)
      {
      if (jindex >= MSched.M[mxoxTJob])
        {
        MStringAppendF(String,"%s%s",
          (jindex > 0) ? "," : "",
          J->Credential.Templates[jindex]);
        }
      }
    } /* END if (J->Cred.Templates != NULL) */

  /* MESSAGES */

  if ((J->MessageBuffer != NULL) || (Msg != NULL))
    {
    char tmpLine[MMAX_LINE << 2];
    mbool_t MMBPrinted = FALSE;

    /* We need a MMBPrintMessagesToMString to not have to use tmpLine */

    MMBPrintMessages(
      J->MessageBuffer,
      mfmNONE,
      TRUE,        /* verbose */
      -1,
      tmpLine,
      sizeof(tmpLine));

    if (tmpLine[0] != '\0')
      {
      mstring_t packString(MMAX_BUFFER);

      MUMStringPack(tmpLine,&packString);

      MStringSet(&tmpString,packString.c_str());

      MStringAppendF(String," %s=\"%s",
        MWikiJobAttr[mwjaMessage],
        tmpString.c_str());

      MMBPrinted = TRUE;
      }

    if (Msg != NULL)
      {
      mstring_t packString(MMAX_BUFFER);

      MUMStringPack(Msg,&packString);

      MStringSet(&tmpString,packString.c_str());

      if (MMBPrinted == FALSE)
        {
        MStringAppendF(String," %s=\"",
          MWikiJobAttr[mwjaMessage]);
        }
      else
        {
        MStringAppend(String,",");
        }

      MStringAppend(String,tmpString.c_str());
      }

    MStringAppend(String,"\"");
    }  /* END if ((J->MB != NULL) || (Msg != NULL)) */

  /* COMPLETION CODE */

  if (MJOBISCOMPLETE(J))
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaExitCode],
      J->CompletionCode);
    } 

  /* SESSION ID */

  if (J->SessionID > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiJobAttr[mwjaSID],
      J->SessionID);
    }

  /* VARIABLES */

  if (J->Variables.Table != NULL)
    {
    MStringSet(&tmpString,"");

    MJobAToMString(
      J,
      mjaVariables,
      &tmpString);

    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaVariables],
      tmpString.c_str());
    }

  /* NODE ACCESS POLICY */

  if ((J->Req[0] != NULL) &&
      (J->Req[0]->PtIndex >= 0) &&
      (J->TaskMap != NULL) &&
      (J->TaskMap[0] >= 0))
    {
    enum MNodeAccessPolicyEnum NAccessPolicy;

    MJobGetNAPolicy(J,NULL,&MPar[J->Req[0]->PtIndex],MNode[J->TaskMap[0]],&NAccessPolicy);

    if (NAccessPolicy != mnacNONE)
      {
      MStringAppendF(String," %s=%s",
        MWikiJobAttr[mwjaNodeAllocationPolicy],
        MNAccessPolicy[NAccessPolicy]);
      }
    }

  /* GMETRICS */

  if (RQ[0]->XURes != NULL)
    {
    int gmindex;
    double AvgFactor;

    for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
      {
      if (RQ[0]->XURes->GMetric[gmindex] == NULL)
        continue;

      if ((RQ[0]->XURes->GMetric[gmindex]->Min <= 0.0) ||
          (RQ[0]->XURes->GMetric[gmindex]->Max <= 0.0) ||
          (RQ[0]->XURes->GMetric[gmindex]->Avg <= 0.0))
        continue;

      AvgFactor = (double)(RQ[0]->XURes->GMetric[gmindex]->Avg * J->TotalProcCount /
        MAX(1,J->PSDedicated));

      MStringAppendF(String," %s[%s]=%.2f",
        MWikiJobAttr[mwjaGMetric],
        MGMetric.Name[gmindex],
        AvgFactor / 100.00);
      }
    } /* END if (RQ[0]->XURes != NULL) */

  /* EFFECTIVE QUEUE DURATION */

  if (J->EffQueueDuration > 0)
    {
    MStringAppendF(String," %s=%ld",
      MWikiJobAttr[mwjaEffQDuration],
      J->EffQueueDuration);
    }

  /* RM SUBMIT STRING */

  if ((MSched.StoreSubmission == TRUE) && (J->RMSubmitString != NULL))
    {
    MStringSet(&tmpString,"");

    MUMStringPack(J->RMSubmitString,&tmpString);

    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaRMSubmitStr],
      tmpString.c_str());
    }

  /* System Info */

  if (J->System != NULL)
    {
    mstring_t systemInfo(MMAX_LINE);

    /* VM ID STRING */

    if (J->System->VM)
      MStringAppendF(String," %s=%s",MWikiJobAttr[mwjaVMID],J->System->VM);

    /* Get all System information from MSysJobToWikiString */

    MSysJobToWikiString(J,J->System,&systemInfo);
 
    MStringAppendF(String," %s",systemInfo.c_str());
    }

  /* WIKI is a separate line for each job, so put newline on */

  MStringAppend(String,"\n");

  return(SUCCESS);
  } /* END MJobToWikiString() */

/* END MJobToWikiString.c */
