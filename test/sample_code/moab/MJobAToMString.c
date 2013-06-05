/* HEADER */

      
/**
 * @file MJobAToMString.c
 *
 * Contains:
 *     MJobAToMString()
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  





/**
 * Report job attribute as mstring_t.
 *
 * The mstring_t that is passed in (Buf) should be intialized before calling this function.
 *
 * @see MJobSetAttr() - peer
 * @see MReqAToMString() - peer
 *
 * @param J       (I)
 * @param AIndex  (I)
 * @param Buf     (O) [must be initialized by caller]
 * @param Mode    (I) [not used except by mjaVariables]
 */

int MJobAToMString(

  mjob_t              *J,
  enum MJobAttrEnum    AIndex,
  mstring_t           *Buf)

  {
  if (Buf == NULL)
    {
    return(FAILURE);
    }

  Buf->clear();   /* Clear the destination string */

  if (J == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mjaAccount:

      if (J->Credential.A != NULL)
        {
        *Buf = J->Credential.A->Name;
        }

      break;

    case mjaAllocNodeList:

      /* NYI - see mrqaAllocNodeList */

      if (MJOBISACTIVE(J) || 
          MJOBISCOMPLETE(J) ||
         (J->VMUsagePolicy == mvmupVMCreate))
        {
        /* FORMAT: <NodeName>[:<TaskCount>][,...] */

        if (MNLIsEmpty(&J->NodeList))
          break;

        MNLToMString(&J->NodeList,MBNOTSET,",",'\0',-1,Buf);
        }    /* END BLOCK (case mjaAllocNodeList) */

      break;

    case mjaAllocVMList:

      {
      if ((MUStrIsEmpty(Buf->c_str())) && (!MUHTIsEmpty(&J->Variables)))
        {
        /* Check if there is a VMID in the job variables */

        char *tmpVMID = NULL;

        if (MUHTGet(&J->Variables,"VMID",(void **)&tmpVMID,NULL) == SUCCESS)
          {
          MStringSet(Buf,tmpVMID);
          }
        }
      }      /* END case mjaAllocVMList */

      break;

    case mjaArgs:

      if (J->Env.Args != NULL)
        {
        MStringSet(Buf,J->Env.Args);
        }

      break;

    case mjaAWDuration:

      if (J->AWallTime > 0)
        {
        MStringSetF(Buf,"%ld",
          J->AWallTime);
        }

      break;

    case mjaBlockReason:

      {
      int   pindex;
      enum  MJobNonEType BlockReason = mjneNONE;

      /* copy the block reason and block message for each partition in the format
            "<blockreason>:<blockmessage>"
         or if per partition scheduling
            "<parname>:<blockreason>:<blockmessage> ~rs <parname>:<blockreason>:<blockmessage> ~rs ..." */

      MStringSet(Buf,"");

      for (pindex = 0;pindex < MMAX_PAR;pindex++)
        { 
        if (MPar[pindex].Name[0] == '\0')
          break;

        if (MPar[pindex].Name[0] == '\1')
          continue;

        /* Check PerPartitionScheduling so we don't skip over partition 0 */

        if ((MSched.PerPartitionScheduling == TRUE) &&
            (!bmisset(&J->PAL,pindex))) 
          continue;

        BlockReason = MJobGetBlockReason(J,&MPar[pindex]);

        if (BlockReason != mjneNONE)
          {
          char tmpLine[MMAX_LINE];

          tmpLine[0] = '\0';

          if (MSched.PerPartitionScheduling == TRUE)
            {
            sprintf(tmpLine,"%s%s:",
              (MUStrIsEmpty(Buf->c_str()) != TRUE) ? " ~rs " : "",
              MPar[pindex].Name);
            }

          MStringAppendF(Buf,"%s%s:%s",
            tmpLine,
            MJobBlockReason[BlockReason],
            MJobGetBlockMsg(J,NULL));
          }

        if (MSched.PerPartitionScheduling == FALSE)
          break;

        }  /* END for pindex */
      } /* END block */
  
      break;

    case mjaBypass:

      if (J->BypassCount > 0)
        {
        MStringSetF(Buf,"%d",
          J->BypassCount);
        }

      break;

    case mjaClass:

      if (J->Credential.C != NULL)
        {
        MStringSet(Buf,J->Credential.C->Name);
        }

      break;

    case mjaCmdFile:

      if ((J->Env.Cmd != NULL) && strcmp(J->Env.Cmd,NONE))
        {
        MStringSet(Buf,J->Env.Cmd);
        }
  
      break;

    case mjaCompletionCode:
    
      if (bmisset(&J->IFlags,mjifWasCanceled))
        {
        if (J->CompletionCode != 0)
          {
          MStringSetF(Buf,"CNCLD(%d)",
            J->CompletionCode);
          }
        else
          {
          MStringSet(Buf,"CNCLD");
          }
        }
      else if (J->CompletionCode != 0)
        {
        MStringSetF(Buf,"%d",J->CompletionCode);
        }

      break;

    case mjaCompletionTime:

      if (J->CompletionTime > 0)
        {
        MStringSetF(Buf,"%ld",
          J->CompletionTime);
        }

      break;

    case mjaCost:

      if (J->Cost != 0.0)
        {
        MStringSetF(Buf,"%.3f",
          J->Cost);
        }

      break;

    case mjaCPULimit:

      if (J->CPULimit > 0)
        {
        MStringSetF(Buf,"%ld",
          J->CPULimit);
        }

      break;

    case mjaDepend:

      {
      if (J->Depend != NULL)
        {
        mdepend_t *D;

        D = J->Depend;

        MStringSet(Buf,"");

        /* FORMAT:  <TYPE> <VALUE> <SPECIFIED VALUED> */

        while (D != NULL)
          {
          if (D->Satisfied == TRUE)
            {
            D = D->NextAnd;
            continue;
            }
            
          MStringAppendF(Buf,"%s %s %s",
            MDependType[D->Type],
            (D->Value != NULL) ? D->Value : "NULL",
            (D->SValue != NULL) ? D->SValue : "");

          if (D->NextAnd != NULL)
            MStringAppend(Buf,",");

          D = D->NextAnd;
          }  /* END while (D != NULL) */
        }   /* END if (J->Depend != NULL) */
      }    /* END BLOCK (case mjaDepend) */

      break;

    case mjaDependBlock:

      if (J->DependBlockReason == mdNONE)
        break;

      if (J->DependBlockMsg != NULL)
        {
        MStringSetF(Buf,"%s:%s",
          MDependType[J->DependBlockReason],
          J->DependBlockMsg);
        }
      else
        {
        MStringSet(Buf,(char *)MDependType[J->DependBlockReason]);
        }
 
      break;

    case mjaDescription:

      /* NOTE:  string should be made XML-safe (NYI) */

      if (J->Description != NULL)
        MStringSet(Buf,J->Description);

      break;

    case mjaDRM:

      /* DRM Name */

      if (J->DestinationRM != NULL && J->DestinationRM->Name != NULL)
        {
        MStringSet(Buf,J->DestinationRM->Name);
        }

      break;

    case mjaDRMJID:

      /* DRM job ID */

      if (J->DRMJID != NULL)
        {
        MStringSet(Buf,J->DRMJID);
        }

      break;

    case mjaEEWDuration:

      MStringSetF(Buf,"%ld",
        J->EffQueueDuration);

      break;

    case mjaEffPAL:

      {
      char ParLine[MMAX_LINE];

      if (!bmisclear(&J->PAL))
        MStringSet(Buf,MPALToString(&J->PAL,NULL,ParLine));
      }  /* END BLOCK */

      break;

    case mjaEFile:

      /* currently, this is tied to RMError--it should be removed
       * completely in future versions of Moab */

      if (J->Env.RMError != NULL)
        MStringSet(Buf,J->Env.RMError);

      break;

    case mjaEligibilityNotes:

      if (J->EligibilityNotes != NULL)
        MStringSet(Buf,J->EligibilityNotes);

      break;

    case mjaEnv:

      if (J->Env.BaseEnv != NULL)
        {
        mstring_t tmpEnvString(MMAX_LINE);

        /* Prepare env string to be printed. BaseEnv can contain ENVRS_ENCODED_CHAR
         * (record separator to handle nested ;'s) and '\n's which
         * are not printable characters and will cause the job to fail
         * to checkpoint in MSysVerifyTextBuf. */

        MStringReplaceStr(J->Env.BaseEnv,ENVRS_ENCODED_STR,ENVRS_SYMBOLIC_STR,&tmpEnvString);
        MUMStringPack(tmpEnvString.c_str(),Buf);
        }

      break;

    case mjaEnvOverride:

      if (J->Env.IncrEnv != NULL)
        {
        mstring_t tmpEnvString(MMAX_LINE);

        /* Prepare env string to be printed. IncrEnv can contain ENVRS_ENCODED_CHAR
         * (record separator to handle nested ;'s) and '\n's which
         * are not printable characters and will cause the job to fail
         * to checkpoint in MSysVerifyTextBuf. */

        MStringReplaceStr(J->Env.IncrEnv,ENVRS_ENCODED_STR,ENVRS_SYMBOLIC_STR,&tmpEnvString);
        MUMStringPack(tmpEnvString.c_str(),Buf);
        }

      break;

    case mjaEState:

      if (J->EState != mjsNONE)
        {
        MStringSet(Buf,(char *)MJobState[J->EState]);
        }

      break;

    case mjaExcHList:

      if (!MNLIsEmpty(&J->ExcHList))
        {
        mstring_t tmpExcHList(MMAX_LINE);

        if (MUNLToRangeString(
            &J->ExcHList,   /* I node list (optional) */ 
            FALSE,          /* I key list for node indicies (optional) */
            0,              /* I (==0 ignore Key, <0 break range on Key changes, >0 filter by Key) */
            FALSE,          /* I */
            TRUE,
            &tmpExcHList) == FAILURE)  /* O */
          {
          MDB(3,fSCHED) MLog("ERROR:    failed to print ExcHList as string for job %s\n",
              J->Name);

          return(FAILURE);
          }
        else if (MStringSet(Buf,tmpExcHList.c_str()) == FAILURE)
          {
          MDB(3,fSCHED) MLog("ERROR:    failed to copy ExcHList string for job %s\n",
              J->Name);

          return(FAILURE);
          }
        }

      break;

    case mjaFlags:

      if (bmisclear(&J->Flags))
        break;

      MJobFlagsToMString(J,&J->Flags,Buf);

      break;

    case mjaGeometry:

      if (J->Geometry != NULL)
        {
        MStringSet(Buf,J->Geometry);
        }

      break;

    case mjaGAttr:

      if (bmisclear(&J->AttrBM))
        break;

      {
      int index;

      MStringSet(Buf,"");

      for (index = 0;index < MMAX_ATTR;index++)
        {
        if (!bmisset(&J->AttrBM,index))
          continue;

        if (!Buf->empty())
          MStringAppend(Buf,",");

        MStringAppend(Buf,MAList[meJFeature][index]);
        }  /* END for (index) */
      }    /* END BLOCK */

      break;

    case mjaGJID:

      if (J->SystemJID != NULL)
        {
        MStringSet(Buf,J->SystemJID);
        }

      break;

    case mjaGroup:

      if (J->Credential.G != NULL)
        {
        MStringSet(Buf,J->Credential.G->Name);
        }

      break;

    case mjaHold:

      bmtostring(&J->Hold,MHoldType,Buf);
 
      break;

    case mjaHoldTime:

      if ((!bmisclear(&J->Hold)) && (J->HoldTime != 0))
        {
        MStringSetF(Buf,"%ld",
          J->HoldTime);
        }

      break;

    case mjaHopCount:

      if (J->Grid == NULL)
        break;

      MStringSetF(Buf,"%d",
        J->Grid->HopCount);

      break;

    case mjaHostList:

      /* FORMAT:  <HOST>[,<HOST>]... */

      if (!MNLIsEmpty(&J->NodeList))
        {
        MNLToMString(&J->NodeList,MBNOTSET,",",'\0',-1,Buf);
        }

      break;

    case mjaImmediateDepend:

      if (J->ImmediateDepend != NULL)
        {
        mln_t *tmpL;

        tmpL = J->ImmediateDepend;

        while (tmpL != NULL)
          {
          MStringAppendF(Buf,"%s %s",
            (tmpL->Ptr != NULL) ? (char *)tmpL->Ptr : MDependType[mdJobCompletion],
            tmpL->Name);

          if (tmpL->Next != NULL)
            {
            MStringAppend(Buf,",");
            }

          tmpL = tmpL->Next;
          }
        }

      break;

    case mjaReqHostList:

      /* FORMAT:  <HOST>[,<HOST>]... */

      if (!MNLIsEmpty(&J->ReqHList))
        {
        MNLToMString(&J->ReqHList,MBNOTSET,",",'\0',-1,Buf);
        }

      break;

    case mjaReqVMList:

      if (J->ReqVMList != NULL)
        {
        /* NOTE:  only support single VM in req VM joblist (FIXME) */

        MStringSet(Buf,J->ReqVMList[0]->VMID);
        }

      break;

    case mjaIsInteractive:

      if (J->IsInteractive == TRUE)
        MStringSet(Buf,(char *)MBool[J->IsInteractive]);

      break;

    case mjaIsRestartable:

      if (bmisset(&J->SpecFlags,mjfRestartable))
        MStringSet(Buf,(char *)MBool[TRUE]);

      break;

    case mjaIsSuspendable:

      if (bmisset(&J->SpecFlags,mjfSuspendable))
        MStringSet(Buf,(char *)MBool[TRUE]);

      break;

    case mjaIWD:

      if ((J->Env.IWD != NULL) && strcmp(J->Env.IWD,NONE))
        {
        MStringSet(Buf,J->Env.IWD);
        }
 
      break;

    case mjaJobGroup:

      if (J->JGroup != NULL)
        {
        MStringSet(Buf,J->JGroup->Name);
        }

      break;

    case mjaJobGroupArrayIndex:

      if (J->JGroup != NULL)
        {
        MStringSetF(Buf,"%d",J->JGroup->ArrayIndex);
        }

      break;

    case mjaJobID:

      MStringSet(Buf,J->Name);

      break;

    case mjaJobName:

      if (J->AName != NULL) 
        MStringSet(Buf,J->AName);
 
      break;

    case mjaLastChargeTime:

      if (J->LastChargeTime > 0)
        MStringSetF(Buf,"%ld",J->LastChargeTime);

      break;

    case mjaMasterHost:
      if (!MNLIsEmpty(&J->NodeList))
        {
        mnode_t *tmpN;
  
        MNLGetNodeAtIndex(&J->NodeList,0,&tmpN);
        MStringSetF(Buf,"%s",tmpN->Name);
        }
      break;

    case mjaMessages:

      if (J->MessageBuffer == NULL)
        break;

      {
      mxml_t *E = NULL;

      MXMLCreateE(&E,(char *)MJobAttr[mjaMessages]);

      MMBPrintMessages(J->MessageBuffer,mfmXML,TRUE,-1,(char *)E,0);

      if (MXMLToMString(E,Buf,NULL,TRUE) == FAILURE)
        {
        MXMLDestroyE(&E);

        return(FAILURE);
        }

      MXMLDestroyE(&E);
      }

      break;

    case mjaMinPreemptTime:

      if (J->MinPreemptTime > 0)
        MStringSetF(Buf,"%ld",
          J->MinPreemptTime);

      break;
      
    case mjaMinWCLimit:
    
      if (J->MinWCLimit > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(J->MinWCLimit,TString);

        MStringSet(Buf,TString);
        }
      
      break;

    case mjaNodeSelection:
      
      if (J->NodePriority != NULL)
        {
        MPrioFToMString(J->NodePriority,Buf);
        }

      break;
  
    case mjaNotification:

      if (!bmisclear(&J->NotifyBM))
        {
        bmtostring(&J->NotifyBM,MNotifyType,Buf); 
        }

      break;

    case mjaNotificationAddress:

      if (J->NotifyAddress != NULL)
        {
        MStringSet(Buf,J->NotifyAddress);
        }

      break;

    case mjaOFile:

      /* currently, this is tied to RMOutput--it should be removed
       * completely in future versions of Moab */

      if (J->Env.RMOutput != NULL)
        MStringSet(Buf,J->Env.RMOutput);

      break;

    case mjaOMinWCLimit:

      if (J->OrigMinWCLimit > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(J->OrigMinWCLimit,TString);

        MStringSet(Buf,TString);
        }

      break;

    case mjaOWCLimit:

      if (J->OrigWCLimit > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(J->OrigWCLimit,TString);

        MStringSet(Buf,TString);
        }

      break;

    case mjaPAL:

      {
      char ParLine[MMAX_LINE];

      if (!bmisclear(&J->SpecPAL))
        MStringSet(Buf,MPALToString(&J->SpecPAL,NULL,ParLine));
      }  /* END BLOCK */

      break;

    case mjaParentVCs:

      if (J->ParentVCs != NULL)
        {
        mln_t *VCL;
        mvc_t *VC;
   
        for (VCL = J->ParentVCs;VCL != NULL;VCL = VCL->Next)
          {
          VC = (mvc_t *)VCL->Ptr;
   
          MStringAppendF(Buf,"%s%s",(VCL != J->ParentVCs) ? "," : "",VC->Name);
          }
        }  /* END if (J->ParentVCs != NULL) */

      break;

    case mjaQueueStatus:

      if (MJOBISCOMPLETE(J))
        {
        /* NO-OP */
        }
      else if (MJOBISACTIVE(J))
        {
        MStringSet(Buf,(char *)MJobStateType[mjstActive]);
        }
      else if (bmisset(&J->IFlags,mjifIsEligible))
        {
        MStringSet(Buf,(char *)MJobStateType[mjstEligible]);
        }
      else
        {
        MStringSet(Buf,(char *)MJobStateType[mjstBlocked]);
        }

      break;

    case mjaQOS:

      if (J->Credential.Q != NULL)
        MStringSet(Buf,J->Credential.Q->Name);

      break;
       
    case mjaQOSReq:

      if (J->QOSRequested != NULL)
        MStringSet(Buf,J->QOSRequested->Name);        

      break;

    case mjaRCL:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);
      bmset(&BM,mfmVerbose);

      MACLListShowMString(J->RequiredCredList,maNONE,&BM,Buf);
      }

      break;

    case mjaReqAWDuration:

      {
      char tmpTime[20];

      if ((J->OrigMinWCLimit > 0) && 
          (J->OrigWCLimit > 0) &&
          (MSched.JobExtendStartWallTime == TRUE))
        {
        MStringSet(Buf,MULToTStringSeconds(J->OrigWCLimit,tmpTime,20));
        }
      else if (J->SpecWCLimit[0] != 0)
        {
        MStringSet(Buf,MULToTStringSeconds(J->SpecWCLimit[0],tmpTime,20));
        }
      }

      break;

    case mjaReqCMaxTime:

      if ((J->CMaxDate > 0) && (J->CMaxDate < MMAX_TIME))
        {
        MStringSetF(Buf,"%ld",
          J->CMaxDate);
        }

      break;

    case mjaReqProcs:

      {
      int PC = J->TotalProcCount;

      if ((PC == 0) && (J->Request.TC != 0))
        PC = J->Request.TC;

      if (PC != 0)
        {
        MStringSetF(Buf,"%d",
          PC);
        }
      }  /* END case mjaReqProcs */

      break;

    case mjaReqNodes:

      if ((J->Request.NC > 0) || (MSched.BluegeneRM == TRUE))
        {
        MStringSetF(Buf,"%d",
          (MSched.BluegeneRM == TRUE) ? J->Request.TC / MSched.BGNodeCPUCnt : J->Request.NC);
        }

      break;

    case mjaReqReservation:

      if (bmisset(&J->Flags,mjfAdvRsv) && (J->ReqRID != NULL))
        {
        MStringSetF(Buf,"%s%s",
          (bmisset(&J->IFlags,mjifNotAdvres)) ? "!" : "",
          J->ReqRID);
        }

      break;

    case mjaReqReservationPeer:

      if (J->PeerReqRID != NULL)
        {
        MStringSet(Buf,J->PeerReqRID);
        }

      break;

    case mjaReqSMinTime:

      if (J->SpecSMinTime > 0)
        {
        MStringSetF(Buf,"%ld",
          J->SpecSMinTime);
        }

      break;

    case mjaResFailPolicy:

      if (J->ResFailPolicy != marfNONE)
        {
        MStringSet(Buf,(char *)MARFPolicy[J->ResFailPolicy]);
        }

      break;

    case mjaRM:

      if (J->SubmitRM != NULL)
        {
        MStringSet(Buf,J->SubmitRM->Name);
        }

      break;

    case mjaRMCompletionTime:

      if (J->RMCompletionTime > 0)
        {
        MStringSetF(Buf,"%ld",
          J->RMCompletionTime);
        }

      break;

    case mjaRMError:

      if (J->Env.RMError != NULL)
        MStringSet(Buf,J->Env.RMError);

      break;

    case mjaRMOutput:

      if (J->Env.RMOutput != NULL)
        MStringSet(Buf,J->Env.RMOutput);

      break;

    case mjaRMSubmitFlags:

      if (J->RMSubmitFlags != NULL)
        {
        MStringSet(Buf,J->RMSubmitFlags);
        }

      break;

    case mjaRMXString:

      MRMXToMString(J,Buf,&J->RMXSetAttrs);

      break;

    case mjaRsvStartTime:

      if (J->Rsv != NULL)
        {
        MStringSetF(Buf,"%ld",
          J->Rsv->StartTime);
        }

      break;

    case mjaPrologueScript:
    case mjaEpilogueScript:
      {
      char** Script;

      if (AIndex == mjaPrologueScript) 
        Script = &J->PrologueScript;
      else
        Script = &J->EpilogueScript;

      if(*Script != NULL)
        MStringSetF(Buf,"%s",
          *Script);
      }
      break;

    case mjaServiceJob:

      if (bmisset(&J->IFlags,mjifServiceJob))
        {
        MStringSet(Buf,(char *)MBool[TRUE]);
        }

      break;

    case mjaSessionID:

      if (J->SessionID != 0)
        {
        MStringSetF(Buf,"%d",J->SessionID);
        }

      break;

    case mjaSID:

      if (J->SystemID != NULL)
        {
        MStringSet(Buf,J->SystemID);
        }

      break;

    case mjaShell:

      if (J->Env.Shell != NULL)
        {
        MStringSet(Buf,J->Env.Shell);
        }

      break;

    case mjaSRMJID:

      /* RM job ID */

      if (J->SRMJID != NULL)
        {
        MStringSet(Buf,J->SRMJID);
        }

      break;

    case mjaStartCount:

      if (J->StartCount > 0)
        {
        MStringSetF(Buf,"%d",
          J->StartCount);
        }

      break;

    case mjaStartPriority:

      MStringSetF(Buf,"%ld",
        J->CurrentStartPriority);

      break;

    case mjaStartTime:

      if (J->StartTime > 0)
        {
        MStringSetF(Buf,"%ld",
          J->StartTime);
        }

      break;

    case mjaState:

      if (J->State != mjsNONE)
        {
        MStringSet(Buf,(char *)MJobState[J->State]);
        }
  
      break;

    case mjaStatMSUtl:

      MStringSetF(Buf,"%.3f",
        J->MSUtilized);

      break;

    case mjaStatPSDed:

      if (J->PSDedicated != 0.0)
        {
        MStringSetF(Buf,"%.3f",
          J->PSDedicated);
        }

      break;

    case mjaStatPSUtl:

      MStringSetF(Buf,"%.3f",
        J->PSUtilized);

      break;

    case mjaStdErr:

      if (J->Env.Error != NULL)
        {
        MStringSet(Buf,J->Env.Error);
        }

      break;

    case mjaStdIn:

      if (J->Env.Input != NULL)
        {
        MStringSet(Buf,J->Env.Input);
        }

      break;

    case mjaStdOut:

      if (J->Env.Output != NULL)
        {
        char *mustableString = NULL;

        *Buf = J->Env.Output;

        /* expand $PBS_JOBID if it exists in Buf to the actual job name */

        /* Need to make a copy of the mstring into a mutable char array
         * then do some text replacement, then set it back to the mstring
         */

        MUStrDup(&mustableString,Buf->c_str());

        MUExpandPBSJobIDToJobName(mustableString,strlen(mustableString),J);

        *Buf = mustableString;

        MUFree(&mustableString);
        }

      break;

    case mjaStorage:

      if (bmisset(&J->IFlags,mjifDestroyDynamicStorage))
        MStringSet(Buf,"destroystorage");

      break;

    case mjaSubmitArgs:

      if (J->AttrString != NULL)
        {
        MStringSet(Buf,J->AttrString);
        }

      break;

    case mjaSubmitHost:

      if (J->SubmitHost != NULL)
        {
        MStringSet(Buf,J->SubmitHost);
        }

      break;

    case mjaSubmitLanguage:

      if (J->RMSubmitType != mrmtNONE)
        {
        MStringSet(Buf,(char *)MRMType[J->RMSubmitType]);
        }

      break;

    case mjaSubmitString:

      if (J->RMSubmitString != NULL)
        {
        /* pack */

        MUMStringPack(J->RMSubmitString,Buf);
        }

      break;

    case mjaSubmitTime:

      if (J->SubmitTime > 0)
        {
        MStringSetF(Buf,"%ld",
          J->SubmitTime);
        }

      break;

    case mjaSubState:

      if (J->SubState != mjsstNONE)
        {
        MStringSet(Buf,(char *)MJobSubState[J->SubState]);
        }

      break;

    case mjaSuspendDuration:

      MStringSetF(Buf,"%ld",
        J->SWallTime);

      break;

    case mjaSysPrio:

      if (J->SystemPrio > 0)
        {
        mbool_t IsRelative = FALSE;
        long AdjustedSysPrio;

        if (J->SystemPrio > MMAX_PRIO_VAL)
          {
          IsRelative = TRUE;
          AdjustedSysPrio = (long)(J->SystemPrio - (MMAX_PRIO_VAL << 1));
          }
        else
          {
          AdjustedSysPrio = (long)J->SystemPrio;
          }

        MStringSetF(Buf,"%s%ld",
          (IsRelative == TRUE) ? "+" : "",
          AdjustedSysPrio);
        }

      break;

    case mjaSysSMinTime:

      if ((J->SysSMinTime != NULL) && (J->SysSMinTime[0] != 0))
        {
        char TString[MMAX_LINE];

        MULToTString(J->SysSMinTime[0],TString);

        MStringSet(Buf,TString);
        }

      break;

    case mjaTaskMap:

      if (J->TaskMap != NULL)
        {
        int   tindex;

        MStringSet(Buf,"");
 
        for (tindex = 0;tindex < J->TaskMapSize;tindex++)
          {
          if (J->TaskMap[tindex] == -1)
            break;

          if (tindex > 0)
            MStringAppend(Buf,",");
 
          MStringAppend(Buf,MNode[J->TaskMap[tindex]]->Name);
          }  /* END for (tindex) */
        }    /* END if (J->TaskMap != NULL) */
 
      break;

    case mjaTemplateList:

      if (J->Credential.Templates != NULL)
        {
        /* FORMAT:  <TEMPLATE>[,<TEMPLATE>]... */

        int jindex;

        for (jindex = 0;J->Credential.Templates[jindex] != NULL;jindex++)
          {
          if (jindex >= MSched.M[mxoxTJob])
            break;

          if (jindex > 0)
            MStringAppendF(Buf,",%s",
              J->Credential.Templates[jindex]);
          else
            MStringSetF(Buf,"%s",
              J->Credential.Templates[jindex]);
          }  /* END for (jindex) */
        }    /* END if (J->Cred.Templates) */

      break;

    case mjaTemplateSetList:

      if (J->TSets != NULL)
        {
        /* FORMAT:  <TEMPLATE>[,<TEMPLATE>]... */

        MULLToMString(J->TSets,FALSE,NULL,Buf);
        }    /* END if (J->TSets != NULL) */

      break;

    case mjaTermTime:

      if (J->TermTime > 0)
        {
        MStringSetF(Buf,"%ld",
          J->TermTime);
        }

      break;

    case mjaTemplateFlags:

      if ((J->TemplateExtensions != NULL) && (!bmisclear(&J->TemplateExtensions->TemplateFlags)))
        {
        bmtostring(&J->TemplateExtensions->TemplateFlags,MTJobFlags,Buf);
        }

      break;

    case mjaTrigNamespace:

      {
      char *tmpAttr;

      if (MJobGetDynamicAttr(J,mjaTrigNamespace,(void **)&tmpAttr,mdfString) == FAILURE)
        return(FAILURE);

      MStringSet(Buf,tmpAttr);

      MUFree(&tmpAttr);
      }

      break;

    case mjaVariables:

      if (MUHTToMString(&J->Variables,Buf) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mjaTTC:

      if (J->TotalTaskCount > 0)
        {
        MStringSetF(Buf,"%d",
          J->TotalTaskCount);
        }

      break;

    case mjaUMask:

      if (J->Env.UMask > 0)
        {
        MStringSetF(Buf,"%d",
          J->Env.UMask);
        }

      break;

    case mjaUser:

      if (J->Credential.U != NULL)
        {
        MStringSet(Buf,J->Credential.U->Name);
        }

      break;

    case mjaUserPrio:

      if (J->UPriority != 0)
        {
        MStringSetF(Buf,"%ld",
          J->UPriority);
        }

      break;

    case mjaVM:

      if (bmisset(&J->IFlags,mjifDestroyDynamicVM))
        MStringSet(Buf,"destroyvm");

      break;

    case mjaVMUsagePolicy:

      if (J->VMUsagePolicy != mvmupNONE)
        MStringSet(Buf,(char *)MVMUsagePolicy[J->VMUsagePolicy]);

      break;

    case mjaVWCLimit:

      if (J->VWCLimit > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(J->VWCLimit,TString);

        MStringSet(Buf,TString);
        }

      break;

    case mjaWorkloadRMID:

      if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRMID))
        MStringSet(Buf,J->TemplateExtensions->WorkloadRMID);

      break;

    default:

      /* not handled */

      return(FAILURE);
 
      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MJobAToMString() */


