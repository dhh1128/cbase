/* HEADER */

/**
 * @file MNodeAToString.c
 * 
 * Contains all node related functions that have unit tests.
 */



#include <assert.h>

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/*
 * Report specified node attribute as string
 * 
 * @see MNodeSetAttr() - peer
 *
 * @param N (I)
 * @param AIndex (I)
 * @param String (O)
 * @param DModeBM (I) [bitmap of enum MCModeEnum - mcmVerbose,mcmXML,mcmUser=human_readable]
 */

int MNodeAToString( 
 
  mnode_t           *N,
  enum MNodeAttrEnum AIndex,
  mstring_t         *String,
  mbitmap_t         *DModeBM)

  {
  if (String == NULL)
    {
    return(FAILURE);
    }
 
  String->clear();
 
  if (N == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mnaAccess:

      if (N->SpecNAPolicy != mnacNONE)
        MStringSetF(String,"%s",MNAccessPolicy[N->SpecNAPolicy]);

      break;

    case mnaAvlGRes:

      if (!MSNLIsEmpty(&N->ARes.GenericRes))
        {
        if (bmisset(DModeBM,mcmXML))
          MSNLToMString(&N->ARes.GenericRes,NULL,";",String,meGRes);
        else
          MSNLToMString(&N->ARes.GenericRes,NULL,NULL,String,meGRes);
        }

      break;

    case mnaAllocRes:

      if (N->AllocRes == NULL)
        break;

      MULLToMString(N->AllocRes,FALSE,NULL,String);

      break;

    case mnaArch:

      if (N->Arch != 0)
        MStringSetF(String,"%s",MAList[meArch][N->Arch]);

      break;

    case mnaAvlClass:

      MClassListToMString(&N->Classes,String,NULL);

      break;

    case mnaCCodeFailure:

      {
      char tmpCCode[MMAX_LINE];
      char tmpLine[MMAX_LINE];
      int tmpCCIndex;

      tmpCCode[0] = '\0';
      tmpLine[0] = '\0';

      if (N->JobCCodeFailureCount != NULL)
        {
        for (tmpCCIndex = 0;tmpCCIndex < MMAX_CCODEARRAY;tmpCCIndex++)
          {
          sprintf(tmpCCode,"%d",
            N->JobCCodeFailureCount[tmpCCIndex]);

          strcat(tmpLine,tmpCCode);

          if (tmpCCIndex < MMAX_CCODEARRAY - 1)
            strcat(tmpLine,",");
          }
        }

      if (tmpLine[0] != '\0')
        {
        MStringAppendF(String,"%s",
          tmpLine);
        }
      }  /* END BLOCK */

      break;

    case mnaCCodeSample:

      {
      char tmpCCode[MMAX_LINE];
      char tmpLine[MMAX_LINE];
      int tmpCCIndex;

      tmpCCode[0] = '\0';
      tmpLine[0] = '\0';

      if (N->JobCCodeSampleCount != NULL)
        {
        for (tmpCCIndex = 0;tmpCCIndex < MMAX_CCODEARRAY;tmpCCIndex++)
          {
          sprintf(tmpCCode,"%d",
            N->JobCCodeSampleCount[tmpCCIndex]);

          strcat(tmpLine,tmpCCode);

          if (tmpCCIndex < MMAX_CCODEARRAY - 1)
            strcat(tmpLine,",");
          }
        }

      if (tmpLine[0] != '\0')
        {
        MStringAppendF(String,"%s",
          tmpLine);
        }
      }  /* END BLOCK */

      break;

    case mnaCfgClass:

      MClassListToMString(&N->Classes,String,NULL);

      break;

    case mnaCfgGRes:

      if (!MSNLIsEmpty(&N->CRes.GenericRes))
        {
        if (bmisset(DModeBM,mcmXML))
          MSNLToMString(&N->CRes.GenericRes,NULL,";",String,meGRes);
        else
          MSNLToMString(&N->CRes.GenericRes,NULL,NULL,String,meGRes);
        }

      break;

    case mnaChargeRate:

      if (N->ChargeRate != 0.0)
        {
        MStringAppendF(String,"%.2f",
          N->ChargeRate);
        }

      break;

    case mnaDedGRes:

      if (!MSNLIsEmpty(&N->DRes.GenericRes))
        {
        if (bmisset(DModeBM,mcmXML))
          MSNLToMString(&N->DRes.GenericRes,NULL,";",String,meGRes);
        else
          MSNLToMString(&N->DRes.GenericRes,NULL,NULL,String,meGRes);
        }

      break;

    case mnaDynamicPriority:

      if (N->P != NULL)
        {
        MStringAppendF(String,"%.2f",
          (N->P->SP + N->P->DP));
        }

      break;

    case mnaEnableProfiling:

      if (N->ProfilingEnabled == TRUE)
        MStringSetF(String,"%s",MBool[N->ProfilingEnabled]);

      break;

    case mnaFeatures:

      {
      char tmpLine[MMAX_LINE];

      MStringSetF(String,"%s",MUNodeFeaturesToString(',',&N->FBM,tmpLine));
      }

      break;

    case mnaFlags:

      bmtostring(&N->Flags,MNodeFlags,String);

      break;

    case mnaGEvent:

      {
      char              *GEName;
      mgevent_obj_t     *GEvent;
      mgevent_iter_t     GEIter;

      if (N->XLoad == NULL) 
        break;

      if (MGEventGetItemCount(&N->GEventsList) == 0)
        break;

      if (bmisset(DModeBM,mcmXML))
        {
        mxml_t *GE = NULL;
        
        MXMLCreateE(&GE,(char *)"GEvents");

        MGEventListToXML(GE,&N->GEventsList);

        MXMLToMString(GE,String,NULL,TRUE);

        MXMLDestroyE(&GE);
        }
      else
        {
        MGEventIterInit(&GEIter);

        while (MGEventItemIterate(&N->GEventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
          {
          if (!MUStrIsEmpty(String->c_str()))
            MStringAppend(String,",");

          if (bmisset(DModeBM,mcmVerbose))
            {
            /* FORMAT:  <GEVENT>:<GTIME>=<GMSG> */

            /* NOTE:  must address XML violation characters in generic event message (NYI) */

            MStringAppendF(String,"%s:%ld%s%s",
              GEName,
              GEvent->GEventMTime,
              (GEvent->GEventMsg != NULL) ? "=" : "",
              (GEvent->GEventMsg != NULL) ? GEvent->GEventMsg : "");
            }
          else
            {
            MStringAppend(String,GEName);
            }
          }
        } /* END if (bmisset(DModeBM,mcmXML)) */
      }    /* END BLOCK (case mnaGEvent) */

      break;

    case mnaGMetric:

      {
      int   gmindex;
      const char *format;
      double value;

      /* FORMAT:  GMETRIC=<GMNAME>:<VAL>[,<GMNAME>:<VAL>]... */

      if (N->XLoad == NULL)
        break;

      /* Make pre-loop assignments:
       * Assume non-user type of output
       */
      format = "%s:%.2f";
      if (bmisset(DModeBM,mcmUser))
        {
        /* if user type of output, switch format type */
        format = "%s=%.2f";
        }

      /* NOTE:  0th entry in MAList is always "NONE" */
      for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
        {
        if (MGMetric.Name[gmindex][0] == '\0')      /* if no name, we are done */
          break;

        if (N->XLoad->GMetric[gmindex] == NULL)     /* safety check the pointer */
          {
          continue;
          }

        value = N->XLoad->GMetric[gmindex]->IntervalLoad;

        if (value == MDEF_GMETRIC_VALUE)
          continue;

        if (!MUStrIsEmpty(String->c_str()))     /* If prior items, add a comment */
          MStringAppend(String,",");

        MStringAppendF(String,format, MGMetric.Name[gmindex], value);
        }
      }  /* END BLOCK (case mnaGMetric) */

      break;

    case mnaHopCount:

      if (N->HopCount > 0)
        {
        MStringAppendF(String,"%d",
          N->HopCount);
        }

      break;

    case mnaHVType:

      if (N->HVType != mhvtNONE)
        {
        MStringAppendF(String,"%s",MHVType[N->HVType]);
        }

      break;

    case mnaIsDeleted:

      if (bmisset(&N->IFlags,mnifIsDeleted))
        MStringSetF(String,"%s",MBool[TRUE]);

      break;

    case mnaJobList:

      if (N->JList != NULL)
        {
        int jindex;

        for (jindex = 0;jindex < N->MaxJCount;jindex++)
          {
          if (N->JList[jindex] == NULL)
            break;

          if (N->JList[jindex] == (mjob_t *)1)
            continue;

          if (!MUStrIsEmpty(String->c_str()))
            MStringAppend(String,",");
        
          MStringAppend(String,N->JList[jindex]->Name); 
          }  /* END for (jindex) */
        }    /* END if (N->JList != NULL) */

      break;

    case mnaKbdDetectPolicy:

      if (N->KbdDetectPolicy != mnkdpNONE)
        {
        MStringSetF(String,"%s",MNodeKbdDetectPolicy[N->KbdDetectPolicy]);
        }

      break;

    case mnaKbdIdleTime:

      if (N->KbdIdleTime > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(N->KbdIdleTime,TString);

        MStringSetF(String,"%s",TString);
        }

      break;

    case mnaLastUpdateTime:

      if (N->ATime <= 0)
        {
        /* What happens in this case? */
        }
      else
        {
        MStringAppendF(String,"%lu",N->ATime);
        }

      break;

    case mnaLoad:

      if (N->Load > 0.0)
        {
        MStringAppendF(String,"%f",
          N->Load);
        }

      break;

    case mnaMaxJob:

      if ((bmisset(DModeBM,mcmVerbose)) || (N->AP.HLimit[mptMaxJob] > 0))
        {
        MStringAppendF(String,"%ld",
          N->AP.HLimit[mptMaxJob]);
        }

      break;

    case mnaMaxJobPerUser:

      if ((bmisset(DModeBM,mcmVerbose)) || 
         ((N->NP != NULL) && (N->NP->MaxJobPerUser > 0)))
        {
        MStringAppendF(String,"%d",
          (N->NP != NULL) ? N->NP->MaxJobPerUser : 0);
        }

      break;

    case mnaMaxLoad:

      if (bmisset(DModeBM,mcmVerbose) || (N->MaxLoad > 0.0))
        {
        MStringAppendF(String,"%f",
          N->MaxLoad);
        }

      break;

    case mnaMaxPE:

      if (bmisset(DModeBM,mcmVerbose) || (N->AP.HLimit[mptMaxPE] > 0))
        {
        MStringAppendF(String,"%ld",
          N->AP.HLimit[mptMaxPE]);
        }

      break;

    case mnaMaxPEPerJob:

      if (bmisset(DModeBM,mcmVerbose) || 
         ((N->NP != NULL) && (N->NP->MaxPEPerJob > 0.0)))
        {
        MStringAppendF(String,"%f",
          (N->NP != NULL) ? N->NP->MaxPEPerJob : 0.0);
        }

      break;

    case mnaMessages:

      if (N->MB != NULL)
        {
        mxml_t *E = NULL;

        MXMLCreateE(&E,(char *)MNodeAttr[mnaMessages]);

        MMBPrintMessages(N->MB,mfmXML,TRUE,-1,(char *)E,0);

        MXMLToMString(E,String,NULL,TRUE);

        MXMLDestroyE(&E);
        }

      break;

    case mnaMinPreemptLoad:

      if (N->MinPreemptLoad > 0.0)
        {
        MStringAppendF(String,"%.2f",
          N->MinPreemptLoad);
        }

      break;

    case mnaMinResumeKbdIdleTime:

      if (N->MinResumeKbdIdleTime > 0)
        {
        MStringAppendF(String,"%ld",
          N->MinResumeKbdIdleTime);
        }

      break;

    case mnaNetAddr:

      if (N->NetAddr != NULL)
        MStringAppendF(String,"%s",N->NetAddr);

      break;

    case mnaNodeIndex:

      /* Note - We currently can have a node with Node Index 0. */
      /*        We do not display the information for node index 0. */
      /*        To address this bug we need to use a bitmap  */
      /*        for Node Indexes and change this test to call an */
      /*        isset function instead  - NYI */

      if (N->NodeIndex > 0)
        {
        MStringAppendF(String,"%d",
          N->NodeIndex);
        }
  
      break;

    case mnaNodeID:

      if (N->Name[0] == '\0')
        break;

      MStringSetF(String,"%s",N->Name);

      break;

    case mnaNodeState:

      if (N->State != mnsNONE)
        MStringSetF(String,"%s",MNodeState[N->State]);

      break;

    case mnaNodeSubState:

      if (N->SubState != NULL)
        MStringAppendF(String,"%s",N->SubState);

      break;

    case mnaOldMessages:

      if (N->Message == NULL)
        break;

      MStringSetF(String,"%s",N->Message);

      break;

    case mnaOperations:

      {
      if (N->HVType != mhvtNONE)
        {
        if (bmisset(&N->IFlags,mnifCanCreateVM))
          {
          MStringAppendF(String,"%s%s",
            (!MUStrIsEmpty(String->c_str())) ? "," : "",
            "vmcreate");
          }

        if (bmisset(&N->IFlags,mnifCanMigrateVM))
          {
          MStringAppendF(String,"%s%s",
            (!MUStrIsEmpty(String->c_str())) ? "," : "",
            "vmmigrate");
          }
        }

      if (bmisset(&N->IFlags,mnifCanModifyOS))
        {
        MStringAppendF(String,"%s%s",
          (!MUStrIsEmpty(String->c_str())) ? "," : "",
          "modifyos");
        }

      if (bmisset(&N->IFlags,mnifCanModifyPower))
        {
        MStringAppendF(String,"%s%s",
          (!MUStrIsEmpty(String->c_str())) ? "," : "",
          "poweron,poweroff,reboot");
        }
      }    /* END BLOCK (case mnaOperations) */

      break;

    case mnaPowerPolicy:

      if (N->PowerPolicy != mpowpNONE)
        MStringSetF(String,"%s",MPowerPolicy[N->PowerPolicy]);

      break;

    case mnaProvRM:

      if (N->ProvRM != NULL)
        MStringSetF(String,"%s",N->ProvRM->Name);

      break;
 
    case mnaRMMessage:

      if (N->RMMsg == NULL)
        break;

      MStringAppendF(String,"%s",N->RMMsg[0]);

      break;

    case mnaOS:

      if (N->ActiveOS != 0)
        MStringSetF(String,"%s",MAList[meOpsys][N->ActiveOS]);

      break;

    case mnaOSList:
    case mnaVMOSList:
      {
      mnodea_t *OSList = (AIndex == mnaOSList) ? N->OSList : N->VMOSList;

      if (OSList != NULL)
        {
        int   osindex;

        for (osindex = 0;OSList[osindex].AIndex > 0;osindex++)
          {
          if (!MUStrIsEmpty(String->c_str()))
            {
            MStringAppendF(String,",%s",
              MAList[meOpsys][OSList[osindex].AIndex]);
            }
          else
            {
            MStringAppend(String,MAList[meOpsys][OSList[osindex].AIndex]);
            }
          }  /* END for (osindex) */
        }    /* END if (N->OSList != NULL) */
      else if ((AIndex == mnaOSList) && (N->ActiveOS != 0))
        {
        MStringSetF(String,"%s",MAList[meOpsys][N->ActiveOS]);
        }
      }

      break;

    case mnaResOvercommitFactor:
    case mnaAllocationLimits:

      if (N->ResOvercommitFactor != NULL)
        {
        MUResFactorArrayToString(N->ResOvercommitFactor,NULL,String);
        }

      break;

    case mnaResOvercommitThreshold:
    case mnaUtilizationThresholds:

      if (N->ResOvercommitThreshold != NULL)
        {
        MUResFactorArrayToString(N->ResOvercommitThreshold,N->GMetricOvercommitThreshold,String);
        }

      break;

    case mnaOwner:

      /* FORMAT:  <CREDTYPE>:<CREDID>[,<CREDTYPE>:<CREDID>]... */

      if (N->Cred.U != NULL)
        {
        MStringAppendF(String,"%s:%s",
          MXO[mxoUser],
          N->Cred.U->Name);
        }

      if (N->Cred.G != NULL)
        {
        MStringAppendF(String,"%s%s:%s",
          (!MUStrIsEmpty(String->c_str())) ? "," : "",
          MXO[mxoGroup],
          N->Cred.G->Name);
        }

      if (N->Cred.A != NULL)
        {
        MStringAppendF(String,"%s%s:%s",
          (!MUStrIsEmpty(String->c_str())) ? "," : "",
          MXO[mxoAcct],
          N->Cred.A->Name);
        }

      if (N->Cred.Q != NULL)
        {
        MStringAppendF(String,"%s%s:%s",
          (!MUStrIsEmpty(String->c_str())) ? "," : "",
          MXO[mxoQOS],
          N->Cred.Q->Name);
        }

      if (N->Cred.C != NULL)
        {
        MStringAppendF(String,"%s%s:%s",
          (!MUStrIsEmpty(String->c_str())) ? "," : "",
          MXO[mxoClass],
          N->Cred.C->Name);
        }

      break;

    case mnaPartition:

      if (N->PtIndex >= 0)
        MStringSetF(String,"%s",MPar[N->PtIndex].Name);

      break;

    case mnaPowerIsEnabled:

      if (N->PowerIsEnabled != MBNOTSET)
        MStringSetF(String,"%s",MBool[N->PowerIsEnabled]);

      break;

    case mnaPowerSelectState:

      if (N->PowerSelectState != mpowNONE)
        MStringSetF(String,"%s",MPowerState[N->PowerSelectState]);

      break;

    case mnaPowerState:

      if (N->PowerState != mpowNONE)
        MStringSetF(String,"%s",MPowerState[N->PowerState]);

      break;

    case mnaPrioF:

      {
      mstring_t tmp(MMAX_LINE);

      MPrioFToMString(N->P,&tmp);

      MStringSetF(String,"%s",tmp.c_str());
      }

      break;

    case mnaPriority:

      if ((N->PrioritySet == FALSE) && (MSched.DefaultN.PrioritySet == TRUE))
        {
        /* if a node's priority is not set use the default node priority */
        MStringAppendF(String,"%d",
          MSched.DefaultN.Priority);
        }
      else if (bmisset(DModeBM,mcmVerbose) || (N->Priority != 0))
        {
        MStringAppendF(String,"%d",
          N->Priority);
        }

      break;

    case mnaProcSpeed:

      if ((bmisset(DModeBM,mcmVerbose)) || (N->ProcSpeed > 0))
        {
        MStringAppendF(String,"%d",
          N->ProcSpeed);
        }

      break;

    case mnaProvData:

      /* sync with MVMSetAttr#mvmaProvData */

      if ((N->NextOS != 0) && (N->NextOS != N->ActiveOS))
        {
        int oindex;
        const mnodea_t *OSList = (N->OSList != NULL) ? N->OSList : MSched.DefaultN.OSList;

        mulong ProvDuration;
        mulong EstEndTime;

        assert((N->OSList != NULL) || (MSched.DefaultN.OSList != NULL));

        ProvDuration = MSched.DefProvDuration;

        for (oindex = 0;OSList[oindex].AIndex != 0;oindex++)
          {
          if (OSList[oindex].AIndex == N->NextOS)
            {
            if (OSList[oindex].CfgTime > 0)
              {
              ProvDuration = OSList[oindex].CfgTime;
              }

            break;
            }
          }    /* END for (oindex) */

        if (N->LastOSModRequestTime + ProvDuration > MSched.Time)
          EstEndTime = N->LastOSModRequestTime + ProvDuration;
        else
          EstEndTime = MSched.Time + 1;

        MStringAppendF(String,"%s,%ld,%ld",
          MAList[meOpsys][N->NextOS],
          N->LastOSModRequestTime,
          EstEndTime);
        }  /* END if ((N->NextOS != 0) && (N->NextOS != N->ActiveOS)) */

      break;

    case mnaRack:

      MStringAppendF(String,"%d",
        N->RackIndex);

      break;

    case mnaRADisk:

      if ((N->ARes.Disk >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        MStringAppendF(String,"%d",
          N->ARes.Disk);
        }

      break;

    case mnaRAMem:

      if ((N->ARes.Mem >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        MStringAppendF(String,"%d",
          MIN(N->ARes.Mem, N->CRes.Mem - N->DRes.Mem));
        }
 
      break;

    case mnaRAProc:

      if ((N->ARes.Procs >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        MStringAppendF(String,"%d",
          N->ARes.Procs);
        }
 
      break;

    case mnaRASwap:

      if ((N->ARes.Swap >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        MStringAppendF(String,"%d",
          N->ARes.Swap);
        }
 
      break;

    case mnaRCDisk:

      if (N->CRes.Disk != 0)
        {
        MStringAppendF(String,"%d",
          N->CRes.Disk);
        }
 
      break;
 
    case mnaRCMem:

      if (N->CRes.Mem != 0)
        {
        MStringAppendF(String,"%d",
          N->CRes.Mem);
        }
 
      break;
 
    case mnaRCProc:

      /* removed for MCM (Nate 7/13/2007)
        if (N->CRes.Procs != 0)
      */
      MStringAppendF(String,"%d",
        N->CRes.Procs);

      break;
 
    case mnaRCSwap:

      if (N->CRes.Swap != 0)
        {
        MStringAppendF(String,"%d",
          N->CRes.Swap);
        }
 
      break;

    case mnaRDDisk:

      if (N->DRes.Disk != 0)
        {
        MStringAppendF(String,"%d",
          N->DRes.Disk);
        }
 
      break;
 
    case mnaRDMem:

      if (N->DRes.Mem != 0)
        {
        MStringAppendF(String,"%d",
          N->DRes.Mem);
        }
 
      break;
 
    case mnaRDProc:

      if (N->DRes.Procs != 0)
        {
        MStringAppendF(String,"%d",
          N->DRes.Procs);
        }
 
      break;
 
    case mnaRDSwap:

      if (N->DRes.Swap != 0)
        {
        MStringAppendF(String,"%d",
          N->DRes.Swap);
        }
 
      break;

    case mnaReleasePriority:

      if (N->ReleasePriority != 0)
        MStringAppendF(String,"%d",
          N->ReleasePriority);

      break;

    case mnaRMList:

      if (N->RMList != NULL)
        {
        int rmindex;

        for (rmindex = 0;N->RMList[rmindex] != NULL;rmindex++)
          {
          if (!MUStrIsEmpty(String->c_str()))
            {
            MStringAppendF(String,",%s",
              N->RMList[rmindex]->Name);
            }
          else
            {
            MStringAppend(String,N->RMList[rmindex]->Name);
            }
          }  /* END for (rmindex) */
        }    /* END if (N->RMList != NULL) */

      break;

    case mnaRsvCount:

      {
      mrsv_t *R;

      RsvTable printedRsvs;

      mre_t *RE;

      int RC = 0;

      for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
        {
        R = MREGetRsv(RE);

        if (printedRsvs.find(R->Name) == printedRsvs.end())
          {
          RC++;

          printedRsvs.insert(std::pair<std::string,mrsv_t *>(R->Name,R));
          }
        }  /* END for (rindex) */

      if (RC > 0)
        {
        MStringAppendF(String,"%d",RC);
        }
      }  /* END BLOCK */

      break;

    case mnaRsvList:

      {
      RsvTable printedRsvs;

      mrsv_t *R;
      mre_t  *RE;

      for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
        {
        R = MREGetRsv(RE);

        if (printedRsvs.find(R->Name) == printedRsvs.end())
          {
          if (MUStrIsEmpty(String->c_str()))
            {
            MStringAppendF(String,"%s",
              R->Name);
            }
          else
            {
            MStringAppendF(String,",%s",
              R->Name);
            }

          printedRsvs.insert(std::pair<std::string,mrsv_t *>(R->Name,R));
          }
        }  /* END for (rindex) */
      }    /* END BLOCK */

      break;  

    case mnaSlot:

      MStringAppendF(String,"%d",
        N->SlotIndex);

      break;

    case mnaSpeed:

      if (N->Speed > 0)
        {
        MStringAppendF(String,"%f",
          N->Speed);
        }

      break;

    case mnaStatATime:

      if (N->SATime > 0)
        {
        MStringAppendF(String,"%ld",
          N->SATime);
        }

      break;

    case mnaStatMTime:

      if (N->StateMTime > 0)
        {
        MStringAppendF(String,"%ld",
          N->StateMTime);
        }

      break;

    case mnaStatTTime:

      if (N->STTime > 0)
        { 
        MStringAppendF(String,"%ld",
          N->STTime);
        }

      break;

    case mnaStatUTime:

      if (N->SUTime > 0)
        { 
        MStringAppendF(String,"%ld",
          N->SUTime);
        }

      break;

    case mnaTrigger:

      if (N->T != NULL)
        {
        MTListToMString(N->T,TRUE,String);
        }

      break;

    case mnaVariables:

      if (!MUHTIsEmpty(&N->Variables))
        {
        MVarToMString(&N->Variables,String);
        }

      break;

    case mnaVarAttr:

      if (N->AttrList.size() != 0)
        {
        for (attr_iter it = N->AttrList.begin();it != N->AttrList.end();++it)
          {
          MStringAppendF(String,"%s%s%s%s",
            (String->empty()) ? "" : ",",
            (*it).first.c_str(),
            ((*it).second.empty()) ? "" : "=",
            ((*it).second.empty()) ? "" : (*it).second.c_str()); 
          } 
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MNodeAToString() */




/**
 * Return the window size on the node.
 *
 *
 * @param N (node to examine)
 * @param StartTime I (when the job/rsv starts) 
 * @param StopTime I (when the job/rsv ends)
 * @param FreeTime (O) 
 */

int MNodeGetWindowTime(

  mnode_t *N,
  mulong   RsvStart,
  mulong   RsvEnd,
  int     *WindowTime)

  {
  int eindex;
  int MaxRsv;
  int RCount;

  mulong StartWindow;
  mulong EndWindow;

  if (WindowTime != NULL)
    *WindowTime = 0;

  if ((N == NULL) || (RsvEnd <= MSched.Time) || (WindowTime == NULL))
    {
    return(FAILURE);
    }

  if (N->PtIndex == MSched.SharedPtIndex)
    MaxRsv = MSched.MaxRsvPerGNode;
  else
    MaxRsv = MSched.MaxRsvPerNode;

  RCount = 0;
  StartWindow = MSched.Time;
  EndWindow =  MMAX_TIME;

  for (eindex = 0;eindex < MaxRsv;eindex++)
    {
    if (N->RE[eindex].Type == mreNONE)
      {
      /* no more reservation events on the node */
      break;
      }

    if ((RCount == 0) &&
        (N->RE[eindex].Type == mreStart)  &&
        ((mulong)N->RE[eindex].Time >= RsvEnd))
      {
      /* node is free at the start of this reservation */

      EndWindow = N->RE[eindex].Time;

      break;
      }
    else if ((RCount == 1) &&
        (N->RE[eindex].Type == mreEnd) &&
        ((mulong)N->RE[eindex].Time <= RsvStart))
      {
      /* node is free after the end of this reservation */

      StartWindow = N->RE[eindex].Time;
      }

    if (N->RE[eindex].Type == mreStart)
      RCount++; /* reservation is starting, increment counter */
    else if (N->RE[eindex].Type == mreEnd)
      RCount--; /* reservation has ended, decrement counter */
    }

  /* the smallest possible window is the exact length of the job/rsv */

  *WindowTime = EndWindow - StartWindow;

  return(SUCCESS);
  }  /* END MNodeGetWindowTime() */



/**
 * Return the free time available on the node within the specified timeframe.
 *
 * Uses a simple stack to determine whether the node is free or not.  Starts at now
 * and goes to StopTime.
 *
 * @param N (node to examine)
 * @param StartTime (I) when to start calculating
 * @param StopTime (I) when to stop calculating
 * @param FreeTime (O) 
 */

int MNodeGetFreeTime(

  mnode_t *N,
  mulong   StartTime,
  mulong   StopTime,
  int     *FreeTime)

  {
  mre_t *RE;

  int TotalFreeTime;
  int RCount = 0;

  mulong LastEventTime;

  if (FreeTime != NULL)
    *FreeTime = 0;

  if ((N == NULL) || (StopTime <= MSched.Time) || (FreeTime == NULL))
    {
    return(FAILURE);
    }

  TotalFreeTime = 0;
  LastEventTime = StartTime;

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    if (RE->Time > (long)StopTime)
      {
      break;
      }

    if ((RCount == 0) && (((mulong)RE->Time) > StartTime))
      TotalFreeTime += RE->Time - LastEventTime;

    if (RE->Type == mreStart)
      RCount++;
    else if (RE->Type == mreEnd)
      RCount--;

    LastEventTime = RE->Time;
    }

  if (RCount == 0)
    {
    TotalFreeTime += StopTime - LastEventTime;
    }

  *FreeTime = TotalFreeTime;

  return(SUCCESS);
  }  /* END MNodeGetFreeTime() */

