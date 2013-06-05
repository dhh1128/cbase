/* HEADER */

      
/**
 * @file MRsvAToMString.c
 *
 * Contains: MRsvAToMString
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report reservation attribute as mstring.
 *
 * @see MRsvToXML() - parent
 * @see MRsvSetAttr() - peer
 *
 * @param R (I)
 * @param AIndex (I)
 * @param Buf (O)
 * @param Mode (I) [bitmap of enum MCModeEnum]
 */

int MRsvAToMString(

  mrsv_t    *R,        /* I */
  enum MRsvAttrEnum AIndex, /* I */
  mstring_t *Buf,      /* O */
  int        Mode)     /* I (bitmap of enum MCModeEnum) */

  {
  if ((Buf == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  Buf->clear();  /* Ensure the MString is cleared first */

  switch (AIndex)
    {
    case mraAllocNodeCount:

      if (R->AllocNC > 0)
        {
        MStringSetF(Buf,"%d",
          R->AllocNC);
        }

      break;

    case mraAllocProcCount:

      if (R->AllocPC > 0)
        {
        MStringSetF(Buf,"%d",
          R->AllocPC);
        }

      break;

    case mraAllocTaskCount:

      if (R->AllocTC > 0)
        {
        MStringSetF(Buf,"%d",
          R->AllocTC);
        }

      break;

    case mraAAccount:  /* accountable account */

      if (R->A != NULL)
        MStringSet(Buf,R->A->Name);

      break;

    case mraACL:

      {
      mbitmap_t BM;

      bmset(&BM,mfmAVP);
      bmset(&BM,mfmVerbose);

      MACLListShowMString(R->ACL,maNONE,&BM,Buf);

      if (!strcmp(Buf->c_str(),NONE))
        MStringSet(Buf,"");
      }

      break;

    case mraAGroup:    /* accountable group */

      if (R->G != NULL)
        MStringSet(Buf,R->G->Name);

      break;

    case mraAQOS:      /* accountable QOS */

      if (R->Q != NULL)
        MStringSet(Buf,R->Q->Name);

      break;

    case mraAUser:     /* accountable user */

      if (R->U != NULL)
        MStringSet(Buf,R->U->Name);

      break;

    case mraCL:

      {
      mbitmap_t BM;

      bmset(&BM,mfmAVP);
      bmset(&BM,mfmVerbose);

      MACLListShowMString(R->CL,maNONE,&BM,Buf);

      if (!strcmp(Buf->c_str(),NONE))
        MStringSet(Buf,"");
      }

      break;

    case mraComment:

      if (R->Comment != NULL)
        MStringSet(Buf,R->Comment);

      break;

    case mraCost:

      if (R->LienCost != 0.0)
        MStringSetF(Buf,"%f",
          R->LienCost);

      break;

    case mraCreds:


      {
      macl_t *tmpCL = R->CL;

      /* FORMAT:  <CRED>=<VAL>[,<CRED>=<VAL>]... */

      while (tmpCL != NULL)
        {
        if (!Buf->empty())
          MStringAppend(Buf,",");

        switch (tmpCL->Cmp)
          {
          case mcmpSEQ:
          case mcmpSSUB:
          case mcmpSNE:

            MStringAppendF(Buf,"%s=%s",
              MAttrO[(int)tmpCL->Type],
              tmpCL->Name);

            break;

          default:

            MStringAppendF(Buf,"%s=%ld",
              MAttrO[(int)tmpCL->Type],
              tmpCL->LVal);

            break;
          }  /* END while (tmpCL != NULL) */

        tmpCL = tmpCL->Next;
        }    /* END while() */
      }      /* END BLOCK */

      break;

    case mraCTime:

      if (R->CTime > 0)
        {
        MStringSetF(Buf,"%ld",
          R->CTime);
        }

      break;

    case mraDuration:

      if (R->EndTime - R->StartTime != 0)
        {
        MStringSetF(Buf,"%ld",
          R->EndTime - R->StartTime);
        }

      break;

    case mraEndTime:

      if (R->EndTime > 0)
        {
        MStringSetF(Buf,"%ld",
          R->EndTime);
        }

      break;

    case mraExcludeRsv:

      {
      int index;

      if (R->RsvExcludeList != NULL)
        {
        for (index = 0;index < MMAX_PJOB;index++)
          {
          if (R->RsvExcludeList[index] == NULL)
            break;

          if(!Buf->empty())
            MStringAppend(Buf, ",");
            
          MStringAppend(Buf,R->RsvExcludeList[index]);
          }
        }
      }

    case mraExpireTime:

      if (R->ExpireTime > 0)
        {
        MStringSetF(Buf,"%ld",
          R->ExpireTime);
        }

      break;

    case mraFlags:

      bmtostring(&R->Flags,MRsvFlags,Buf);

      break;

    case mraGlobalID:

      if (R->SystemRID != NULL)
        {
        MStringSet(Buf,R->SystemRID);
        }

      break;

    case mraHistory:

      if (R->History != NULL)
        {
        MHistoryToString(R,mxoRsv,Buf);
        }
    
      break;

    case mraHostExp:

      if (R->HostExp != NULL)
        {
        MStringSet(Buf,R->HostExp);
        }

      break;

    case mraHostExpIsSpecified:

      if (R->HostExpIsSpecified == TRUE)
        {
        MStringSet(Buf,"TRUE");
        }

      break;

    case mraJState:

      {
      if ((R->Type != mrtJob) || (R->J == NULL))
        break;

      MStringSet(Buf,(char *)MJobState[R->J->State]);
      }  /* END BLOCK */

      break;

    case mraLabel:

      if ((R->Label != NULL) && (R->Label[0] != '\0'))
        {
        MStringSet(Buf,R->Label);
        }

      break;

    case mraLastChargeTime:

      if (R->LastChargeTime > 0)
        {
        MStringSetF(Buf,"%ld",
          R->LastChargeTime);
        }

      break;

    case mraLogLevel:

      if (R->LogLevel > 0)
        {
        MStringSetF(Buf,"%d",
          R->LogLevel);
        }

      break;

    case mraMaxJob:

      if (R->MaxJob > 0)
        {
        MStringSetF(Buf,"%d",
          R->MaxJob);
        }

      break;

    case mraMessages:

      if (R->MB == NULL)
        break;

      {
      mxml_t *E = NULL;

      MXMLCreateE(&E,(char *)MRsvAttr[mraMessages]);

      MMBPrintMessages(R->MB,mfmXML,TRUE,-1,(char *)E,0);

      MXMLToMString(E,Buf,NULL,TRUE);

      MXMLDestroyE(&E);
      }

      break;

    case mraName:

      if (R->Name[0] != '\0')
        MStringSet(Buf,R->Name);

      break;

    case mraAllocNodeList:

      if (Mode != mcmCP)
        {
        if (MNLToMString(
              &R->NL,
              FALSE,
              ",",
              '\0',
              -1,
              Buf) == FAILURE)
          {
          return(FAILURE);
          }
        }
      else
        {
        if (MNLToMString(
              &R->NL,
              FALSE,
              NULL,
              '\0',
              -1,
              Buf) == FAILURE)
          {
          return(FAILURE);
          }
        }

      break;

    case mraIsTracked:

      if (R->IsTracked == TRUE)
        {
        MStringSet(Buf,"TRUE");
        }

      break;

    case mraOwner:

      if (R->O != NULL)
        {
        MStringSetF(Buf,"%s:%s",
          MXOC[R->OType],
          MOGetName(R->O,R->OType,NULL));
        }

      break;

    case mraPartition:

      if (R->PtIndex > 0)
        MStringSet(Buf,MPar[MAX(0,R->PtIndex)].Name);
      else
        MStringSet(Buf,"ALL");

      break;

    case mraPriority:

      if (R->Priority > 0)
        {
        MStringSetF(Buf,"%ld",
          R->Priority);
        }

      break;

    case mraProfile:

      /* NYI */

      break;

    case mraReqArch:

      if (R->ReqArch != 0)
        {
        MStringSet(Buf,MAList[meArch][R->ReqArch]);
        }

      break;

    case mraReqFeatureList:

      if (!bmisclear(&R->ReqFBM))
        {
        char Line[MMAX_LINE];

        MUNodeFeaturesToString(',',&R->ReqFBM,Line);

        MStringSet(Buf,Line);
        }

      break;

    case mraReqFeatureMode:

      if (R->ReqFMode != 0)
        MStringSet(Buf,(char *)MCLogic[R->ReqFMode]);

      break;

    case mraReqMemory:

      if (R->ReqMemory > 0)
        {
        MStringSetF(Buf,"%d",
          R->ReqMemory);
        }

      break;

    case mraReqNodeList:

      if (MNLToMString(
            &R->ReqNL,
            TRUE,
            ",",
            '\0',
            -1,
            Buf) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mraReqNodeCount:

      if (R->ReqNC > 0)
        {
        MStringSetF(Buf,"%d",
          R->ReqNC);
        }

      break;

    case mraReqOS:

      if (R->ReqOS != 0)
        {
        MStringSet(Buf,MAList[meOpsys][R->ReqOS]);
        }

      break;

    case mraReqTaskCount:

      if (R->ReqTC > 0)
        {
        MStringSetF(Buf,"%d",
          R->ReqTC);
        }

      break;

    case mraReqTPN:

      if (R->ReqTPN > 0)
        {
        MStringSetF(Buf,"%d",
          R->ReqTPN);
        }

      break;

    case mraResources:

      MCResToMString(&R->DRes,0,mfmAVP,Buf);

      if (!strcmp(Buf->c_str(),NONE))
        MStringSet(Buf,"");

      break;

    case mraRsvAccessList:

      {
      int   index;

      if (R->RsvAList != NULL)
        {
        for (index = 0;index < MMAX_PJOB;index++)
          {
          if (R->RsvAList[index] == NULL)
            break;
    
          MStringAppendF(Buf,"%s",
              R->RsvAList[index]);
          }
        }
      }    /* END BLOCK (case mraRsvAccessList) */

      break;

    case mraRsvGroup:

      if (R->RsvGroup != NULL)
        MStringSet(Buf,R->RsvGroup);

      break;

    case mraRsvParent:

      if (R->RsvParent != NULL)
        MStringSet(Buf,R->RsvParent);

      break;

    case mraSID:

      if (R->SystemID != NULL)
        {
        MStringSet(Buf,R->SystemID);
        }

      break;

    case mraSpecName:

      if (R->SpecName != NULL)
        MStringSet(Buf,R->SpecName);

      break;

    case mraStartTime:

      if (R->StartTime > 0)
        {
        MStringSetF(Buf,"%ld",
          R->StartTime);
        }

      break;

    case mraStatCAPS:

      if (R->CAPS > 0.0)
        {
        MStringSetF(Buf,"%.2f",
          R->CAPS);
        }

      break;

    case mraStatCIPS:

      if (R->CIPS > 0.0)
        {
        MStringSetF(Buf,"%.2f",
          R->CIPS);
        }

      break;

    case mraStatTAPS:

      if (R->TAPS > 0.0)
        {
        MStringSetF(Buf,"%.2f",
          R->TAPS);
        }

      break;

    case mraStatTIPS:

      if (R->TIPS > 0.0)
        {
        MStringSetF(Buf,"%.2f",
          R->TIPS);
        }

      break;


    case mraSubType:

      if (R->SubType != mrsvstNONE)
        {
        MStringSet(Buf,(char *)MRsvSubType[R->SubType]);
        }

      break;

    case mraSysJobID:

      if (R->SysJobID != NULL)
        MStringSet(Buf,R->SysJobID);

      break;

    case mraSysJobTemplate:

      if (R->SysJobTemplate != NULL)
        MStringSet(Buf,R->SysJobTemplate);

      break;

    case mraTrigger:

      if (R->T == NULL)
        break;

      MTListToMString(R->T,TRUE,Buf);

      break;

    case mraType:

      if (R->Type > 0)
        MStringSet(Buf,(char *)MRsvType[R->Type]);

      break;

    case mraVariables:

      if (R->Variables != NULL)
        {
        mln_t *tmpL;

        tmpL = R->Variables;

        while (tmpL != NULL)
          {
          MStringAppend(Buf,tmpL->Name);

          if (tmpL->Ptr != NULL)
            {
            MStringAppendF(Buf,"=%s;",(char *)tmpL->Ptr);
            }
          else
            {
            MStringAppend(Buf,";");
            }

          tmpL = tmpL->Next;
          }  /* END while (tmpL != NULL) */
        }    /* END (case mraVariables) */

      break;

    case mraVMList:

      break;

    case mraVMUsagePolicy:

      if (R->VMUsagePolicy != mvmupNONE)
        MStringSet(Buf,(char *)MVMUsagePolicy[R->VMUsagePolicy]);

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MRsvAToMString() */
/* END MRsvAToMString.c */
