/* HEADER */

      
/**
 * @file MRsvShowState.c
 *
 * Contains: MRsvShowState
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report reservation attributes and state.
 *
 * NOTE: handles "mdiag -r"
 *
 * @see MRsvDiagnoseState() - peer
 * @see MUIRsvDiagnose() - parent
 *
 * @param R (I)
 * @param Flags (I) [bitmap of enum mcm*]
 * @param SRE (I) [ either SRE or String must be set]
 * @param String (I) [either SRE or String must be set]
 * @param DFormat (I)
 */

int MRsvShowState(

  mrsv_t                  *R,
  mbitmap_t               *Flags,
  mxml_t                  *SRE,
  mstring_t               *String,
  enum MFormatModeEnum     DFormat)

  {
  mxml_t *RE;

  mbitmap_t BM;

  const char *FName = "MRsvShowState";

  MDB(5,fSTRUCT) MLog("%s(%s,XML,String,%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MFormatMode[DFormat]);

  if (DFormat == mfmXML)
    {
    if (SRE == NULL)
      {
      return(FAILURE);
      }

    RE = SRE;
    }
  else
    {
    if (String == NULL)
      {
      return(FAILURE);
      }
    }

  mstring_t tmpLine(MMAX_LINE);

  if (R == NULL)
    {
    /* create header */

    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"%-20s %10s %3.3s %11s %11s  %11s %4s %4s %4s\n",
        "RsvID",
        "Type",
        "Par",
        "StartTime",
        "EndTime",
        "Duration",
        "Node",
        "Task",
        "Proc");

      MStringAppendF(String,"%-20s %10s %3.3s %11s %11s  %11s %4s %4s %4s\n",
        "-----",
        "----",
        "---",
        "---------",
        "-------",
        "--------",
        "----",
        "----",
        "----");
      }

    return(SUCCESS);
    }  /* END if (R == NULL) */

  /* show base data */

  if (DFormat != mfmXML)
    {
    char StartTime[MMAX_NAME];
    char EndTime[MMAX_NAME];
    char Duration[MMAX_NAME];
    char TString[MMAX_LINE];

    MULToTString(R->StartTime - MSched.Time,TString);
    strcpy(StartTime,TString);
    MULToTString(R->EndTime - MSched.Time,TString);
    strcpy(EndTime,TString);
    MULToTString(R->EndTime - R->StartTime,TString);
    strcpy(Duration,TString);

    MStringAppendF(String,"%-20s %10s %3.3s %11s %11s  %11s %4d %4d %4d\n",
      R->Name,
      MRsvType[R->Type],
      (R->PtIndex >= 0) ? MPar[R->PtIndex].Name : "ALL",
      StartTime,
      EndTime,
      Duration,
      R->AllocNC,
      R->AllocTC,
      R->AllocPC);
    }
  else
    {
    const enum MRsvAttrEnum RAList[] = {
      mraName,
      mraAAccount,
      mraAGroup,
      mraAUser,
      mraAQOS,
      mraAllocNodeCount,
      mraAllocNodeList,
      mraAllocProcCount,
      mraAllocTaskCount,
      mraComment,
      mraEndTime,
      mraExcludeRsv,
      mraExpireTime,
      mraGlobalID,
      mraLabel,
      mraLogLevel,
      mraPartition,
      mraProfile,
      mraReqArch,
      mraReqFeatureList,
      mraReqMemory,
      mraReqNodeCount,
      mraReqNodeList,
      mraReqOS,
      mraReqTaskCount,
      mraRsvGroup,
      mraSID,
      mraStartTime,
      mraStatCAPS,
      mraStatCIPS,
      mraStatTAPS,
      mraStatTIPS,
      mraSubType,
      mraTrigger,
      mraType,
      mraVariables,
      mraVMList,
      mraNONE };

    MRsvToXML(R,&RE,(enum MRsvAttrEnum *)RAList,NULL,FALSE,mcmNONE);
    }  /* END else (DFormat != mfmXML) */

  if (R->Label != NULL)
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(RE,(char *)MRsvAttr[mraLabel],(void *)R->Label,mdfString);
      }
    else
      {
      MStringAppendF(String,"    Label:  %s\n",
        R->Label);
      }
    }

  /* display rsv flags */

  if ((!bmisclear(&R->Flags)) || bmisset(Flags,mcmVerbose))
    {
    bmtostring(&R->Flags,MRsvFlags,&tmpLine);

    if (DFormat == mfmXML)
      {
      MXMLSetAttr(RE,(char *)MRsvAttr[mraFlags],(void *)tmpLine.c_str(),mdfString);
      }
    else
      {
      MStringAppendF(String,"    Flags: %s\n",
        tmpLine.c_str());
      }
    }

  /* display reservation ACL/CL */

  tmpLine.clear();

  bmset(&BM,mfmHuman);
  bmset(&BM,mfmVerbose);

  MACLListShowMString(R->ACL,maNONE,&BM,&tmpLine);

  if (!tmpLine.empty())
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(RE,(char *)MRsvAttr[mraACL],(void *)tmpLine.c_str(),mdfString);
      }
    else
      {
      MStringAppendF(String,"    ACL:   %s\n",
        tmpLine.c_str());
      }
    }

  tmpLine.clear();

  MACLListShowMString(R->CL,maNONE,&BM,&tmpLine);

  if (!tmpLine.empty())
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(RE,(char *)MRsvAttr[mraCreds],(void *)tmpLine.c_str(),mdfString);
      }
    else
      {
      MStringAppendF(String,"    CL:    %s\n",
        tmpLine.c_str());
      }
    }

  MRsvAToMString(R,mraOwner,&tmpLine,0);

  if (!tmpLine.empty())
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(RE,(char *)MRsvAttr[mraOwner],(void *)tmpLine.c_str(),mdfString);
      }
    else
      {
      MStringAppendF(String,"    Owner: %s\n",
        tmpLine.c_str());
      }
    }

  if (R->T != NULL)
    {
    if (DFormat != mfmXML)
      {
      /* xml data displayed in MXMLToString() */

      MRsvAToMString(R,mraTrigger,&tmpLine,0);

      if (!tmpLine.empty())
        {
        MStringAppendF(String,"    Trigger:  %s\n",
          tmpLine.c_str());
        }
      }
    }


  if (!bmisclear(&R->ReqFBM))
    {
    char Line[MMAX_LINE];

    if (DFormat == mfmXML)
      {
      MXMLSetAttr(
        RE,
        (char *)MRsvAttr[mraReqFeatureList],
        (void *)MUNodeFeaturesToString(',',&R->ReqFBM,Line),
        mdfString);

      MXMLSetAttr(
        RE,
        (char *)MRsvAttr[mraReqFeatureMode],
        (void *)MCLogic[R->ReqFMode],
        mdfString);
      }
    else
      {
      MStringAppendF(String,"    Features: %s",
        MUNodeFeaturesToString(',',&R->ReqFBM,Line));

      if (R->ReqFMode != 0)
        {
        MStringAppendF(String,"    Feature Mode: %s",
          MCLogic[R->ReqFMode]);
        }

      MStringAppendF(String,"\n");
      }
    }  /* END if (R->ReqFBM != NULL) */

  /* display accounting creds */

  if (R->Type == mrtUser)
    {
    if ((R->A != NULL) ||
        (R->G != NULL) ||
        (R->U != NULL) ||
        (R->Q != NULL) ||
        bmisset(Flags,mcmVerbose))
      {
      if (DFormat == mfmXML)
        {
        /* NYI */
        }
      else
        {
        MStringAppendF(String,"    Accounting Creds: %s%s%s%s%s%s%s%s\n",
          (R->A != NULL) ? " Account:" : "",
          (R->A != NULL) ? R->A->Name : "",
          (R->G != NULL) ? " Group:" : "",
          (R->G != NULL) ? R->G->Name : "",
          (R->U != NULL) ? " User:" : "",
          (R->U != NULL) ? R->U->Name : "",
          (R->Q != NULL) ? " QOS:" : "",
          (R->Q != NULL) ? R->Q->Name : "");
        }
      }  /* END if (R->A != NULL) ... ) */
    }    /* END if (R->Type == mrtUser) */

  /* display per task dedicated resources */

  if ((R->Type == mrtUser) ||
     ((R->Type == mrtJob) && (R->DRes.Procs != 1)))
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(
        RE,
        (char *)MRsvAttr[mraResources],
        (void *)MCResToString(&R->DRes,0,mfmAVP,NULL),
        mdfString);
      }
    else
      {
      MStringAppendF(String,"    Task Resources: %s\n",
      MCResToString(&R->DRes,0,mfmHuman,NULL));
      }
    }

  if (R->SubType != mrsvstNONE)
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(RE,(char *)MRsvAttr[mraSubType],(void *)MRsvSubType[R->SubType],mdfString);
      }
    else
      {
      MStringAppendF(String,"    SubType: %s\n",
        MRsvSubType[R->SubType]);
      }
    }

  if (R->ExpireTime > 0)
    {
    if (DFormat != mfmXML)
      {
      char TString[MMAX_LINE];

      MULToTString(R->ExpireTime - MSched.Time,TString);

      MStringAppendF(String,"    ExpireTime: %s\n",
        TString);
      }
    }

  if (R->AllocPolicy != mralpNONE)
    {
    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"    ReallocPolicy: %s\n",
        MRsvAllocPolicy[R->AllocPolicy]);
      }
    }

  /* display alloc node list */

  if (!MNLIsEmpty(&R->NL))
    {
    if (DFormat == mfmXML)
      {
      /* handled in MRsvToXML() */
      }
    else
      {
      if (bmisset(Flags,mcmVerbose))
        {
        MRsvAToMString(
          R,
          mraAllocNodeList,
          &tmpLine,
          0);

        if (!MUStrIsEmpty(tmpLine.c_str()))
          {
          MStringAppendF(String,"    Nodes='%s'\n",
            tmpLine.c_str());
          }
        }
      }
    }   /* END if (R->NL != NULL) */

  /* display the excluded job list */

  if (R->RsvExcludeList != NULL)
    {
    if (DFormat == mfmXML)
      {
      /* handled in MRsvToXML() */
      }
    else
      {
      if (bmisset(Flags,mcmVerbose))
        {
        MRsvAToMString(
          R,
          mraExcludeRsv,
          &tmpLine,
          0);
    
        if (!tmpLine.empty())
          {
          MStringAppendF(String,"    Excluded Jobs='%s'\n",
            tmpLine.c_str());
          }
        }
      }
    }   /* END if (R->RsvExcludeList != NULL) */

  if (R->Comment != NULL)
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(
        RE,
        (char *)MRsvAttr[mraComment],
        (void *)R->Comment,
        mdfString);
      }
    else
      {
      MStringAppendF(String,"    Comment: '%s'\n",
        R->Comment);
      }
    }

  /* display reservation attributes */

  tmpLine.clear();

  /* display host exp */

  if (R->HostExp != NULL)
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(
        RE,
        (char *)MRsvAttr[mraHostExp],
        (void *)R->HostExp,
        mdfString);
      }
    else
      {
      MStringAppendF(&tmpLine,"  HostExp='%.64s%s'",
        R->HostExp,
        (strlen(R->HostExp) >= 64 - 1) ? "..." : "");
      }
    }    /* END if (R->HostExp != NULL) */

  if (DFormat != mfmXML)
    {
    if (R->ReqOS > 0)
      MStringAppendF(&tmpLine,"  OS=%s",
        MAList[meOpsys][R->ReqOS]);

    if (R->ReqArch > 0)
      MStringAppendF(&tmpLine,"  ARCH=%s",
        MAList[meArch][R->ReqArch]);

    if (R->ReqMemory > 0)
      MStringAppendF(&tmpLine,"  MINMEM=%d",
        R->ReqMemory);
    }

  if (R->LogLevel > 0)
    {
    if (DFormat == mfmXML)
      {
      /* NYI */
      }
    else
      {
      MStringAppendF(&tmpLine,"  LogLevel=%d",
        R->LogLevel);
      }
    }

  if (R->Priority > 0)
    {
    if (DFormat == mfmXML)
      {
      /* NYI */
      }
    else
      {
      MStringAppendF(&tmpLine,"  Priority=%ld",
        R->Priority);
      }
    }

  if (R->SystemID != NULL)
    {
    if (DFormat == mfmXML)
      {
      /* NYI */
      }
    else
      {
      MStringAppendF(&tmpLine,"  SystemID=%s",
        R->SystemID);
      }
    }

  if (!tmpLine.empty())
    {
    /* NOTE:  skip leading whitespace */

    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"    Attributes (%s)\n",
        &tmpLine.c_str()[2]);
      }
    }

  /* display statistics */

  if ((R->Type != mrtJob) &&
     ((R->TAPS + R->CAPS + R->TIPS + R->CIPS) > 0.0))
    {
    double E;

    if ((R->TAPS + R->CAPS) > 0.0)
      {
      E = (1.0 - (double)(R->TIPS + R->CIPS) /
          (R->TAPS + R->CAPS + R->TIPS + R->CIPS)) * 100.0;
      }
    else
      {
      E = 0.0;
      }

    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"    Active PH: %.2f/%-.2f (%.2f%c)\n",
        (R->TAPS + R->CAPS) / 3600.0,
        (R->TAPS + R->CAPS + R->TIPS + R->CIPS) / 3600.0,
        E,
        '%');
      }
    }  /* END if (R->Type != mrtJob) && ...) */

  /* display SR attributes */

  if (bmisset(&R->Flags,mrfStanding))
    {
    int srindex;
    int dindex;

    char tmpLine2[MMAX_LINE];
    char tmpTString[MMAX_NAME];

    msrsv_t *S;

    for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
      {
      S = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);

      if (S == NULL)
        break;

      if (strncmp(S->Name,R->Name,strlen(S->Name)))
        continue;

      for (dindex = 0;dindex < MMAX_SRSV_DEPTH;dindex++)
        {
        if (S->R[dindex] == R)
          {
          char TString[MMAX_LINE];

          if (S->StartTime == (mulong)-1)   /* FIXME */
            {
            MULToTString(0,TString);

            strcpy(tmpTString,TString);
            }
          else
            {
            MULToTString(S->StartTime,TString);

            strcpy(tmpTString,TString);
            }

          if (bmisset(&S->Days,MCONST_ALLDAYS))
            {
            strcpy(tmpLine2,"ALL");
            }
          else
            {
            int index;

            tmpLine2[0] = '\0';

            for (index = 0;MWeekDay[index] != NULL;index++)
              {
              if (bmisset(&S->Days,index))
                {
                if (tmpLine2[0] != '\0')
                  strcat(tmpLine2,",");

                strcat(tmpLine2,MWeekDay[index]);
                }
              }    /* END for (index) */
            }      /* END else (S->Days == MCONST_ALLDAYS) */

          if (DFormat != mfmXML)
            {
            if (S->EndTime == (mulong)-1)
              {
              MULToTString(86400,TString);
              }
            else if (S->EndTime > 0)
              {
              MULToTString(S->EndTime,TString);
              }
            else
              {
              MULToTString(86400,TString);
              }

            MStringAppendF(String,"    SRAttributes (TaskCount: %d  StartTime: %s  EndTime: %s  Days: %s)\n",
              S->ReqTC,
              tmpTString,
              TString,
              (tmpLine2[0] != '\0') ? tmpLine2 : ALL);

            if (R->MaxJob > 0)
              MStringAppendF(String,"    MaxJob:   %d\n",R->MaxJob);

            if (bmisset(Flags,mcmVerbose))
              MStringAppendF(String,"    JobCount: %d\n",R->JCount);
            }

          break;
          }  /* END if (MSRsv[srindex] == R) */
        }    /* END for (dindex)            */
      }      /* END for (srindex)           */
    }        /* END if (bmisset(R->Flags,mrfStanding)) */

  /* display extension attributes */

  if (R->RsvGroup != NULL)
    {
    if (DFormat == mfmXML)
      {
      /* handled by MRsvToXML */
      
      /* No-Op */
      }
    else
      {
      MStringAppendF(String,"    Rsv-Group: %s\n",
        R->RsvGroup);
      }
    }

  if (R->RsvParent != NULL)
    {
    if (DFormat == mfmXML)
      {
      /* NOT SUPPORTED */

      /* NO-OP */
      }
    else
      {
      MStringAppendF(String,"    Rsv-Parent: %s\n",
        R->RsvParent);
      }
    }

  if (R->Variables != NULL)
    {
    MRsvAToMString(R,mraVariables,&tmpLine,mfmHuman);

    if (DFormat == mfmXML)
      {
      /* NOT SUPPORTED */

      /* NO-OP */
      }
    else
      {
      MStringAppendF(String,"    Variables: %s\n",
        tmpLine.c_str());
      }
    }

  if ((R->History != NULL) &&
      (DFormat != mfmXML))
    {
    tmpLine.clear();

    MHistoryToString(R,mxoRsv,&tmpLine);

    if (!tmpLine.empty())
      {
      MStringAppendF(String,"    History: %s\n",
        tmpLine.c_str());
      }
    }

  if (bmisset(Flags,mcmVerbose2) && 
      (DFormat != mfmXML) &&
      (R->CmdLineArgs != NULL))
    {
    MStringAppendF(String,"    Command Line: %s\n",
      R->CmdLineArgs);
    }

  if (R->MB != NULL)
    {
    if (DFormat == mfmXML)
      {
      /* NOT SUPPORTED */

      /* NO-OP */
      }
    else
      {
      char *BPtr;
      int   BSpace;

      char MsgBuf[MMAX_BUFFER];

      MsgBuf[0] = '\0';

      /* generate header */

      MStringAppendF(String,"\n    Messages\n");

      MUSNInit(&BPtr,&BSpace,MsgBuf,sizeof(MsgBuf));

      MMBPrintMessages(
        NULL,
        mfmHuman,
        TRUE,
        -1,
        BPtr,
        BSpace);

      /* display data */

      BSpace -= strlen(BPtr);
      BPtr   += strlen(BPtr);

      MMBPrintMessages(
        R->MB,
        mfmHuman,
        TRUE,
        -1,
        BPtr,
        BSpace);

      BSpace -= strlen(BPtr);
      BPtr   += strlen(BPtr);

      MStringAppend(String,MsgBuf);
      }
    }    /* END if (R->MB != NULL) */

  if (DFormat != mfmXML)
    {
    MStringAppendF(String,"\n");
    }

  return(SUCCESS);
  }  /* END MRsvShowState() */
/* END MRsvShowState.c */
