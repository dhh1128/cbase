/* HEADER */

/**
 * @file MJRMX.c
 *
 * Contains all resource manager extension functions, including parsing and rendering
 * to strings.
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"



/**
 * Render a depenency list and append to a 'string' object passed in
 *
 * Assumes String is already initialized to caller's desired state
 *
 * @param String (I)
 * @param DList  (I)
 * @param RMType (I)
 */

int MDependToString(

  mstring_t *MStr,
  mdepend_t *DList,
  enum MRMTypeEnum RMType)

  {
  mdepend_t *D;
  mjob_t *tmpJ;

  switch (RMType)
    {
    case mrmtWiki:

    /* ArgString sent to Wiki is in the format "Type:<jobid>[,Type:<jobid>]" */

    if (MJobFindDepend(DList->Value,&tmpJ,NULL) == SUCCESS &&
        (tmpJ != NULL ))
      {
      char *RealJID = (tmpJ->DRMJID == NULL) ? tmpJ->Name : tmpJ->DRMJID;

      char *JobIdPtr = !strcasecmp(RealJID,"none") ? (char *)"0" : RealJID;

      *MStr += SDependType[DList->Type];
      *MStr += ':';
      *MStr += JobIdPtr;
      }

    for (D = DList->NextAnd;D != NULL;D = D->NextAnd)
      {
      if (MJobFindDepend(D->Value,&tmpJ,NULL) == SUCCESS &&
          (tmpJ != NULL ))
        {
        char *RealJID = (tmpJ->DRMJID == NULL) ? tmpJ->Name : tmpJ->DRMJID;

        char *JobIdPtr = !strcasecmp(RealJID,"none") ? (char *)"0" : RealJID;

        *MStr += SDependType[D->Type];
        *MStr += ':';
        *MStr += JobIdPtr;
        }
      }

      break;

    default:

      /* MJobAddDependency accepts the following format:
         [<TYPE>:]<VALUE>[:<VALUE>...][{,|&}[<TYPE>:]<VALUE>]... */

      *MStr += MDependType[DList->Type];
      *MStr += ':';
      *MStr += DList->Value;

      for (D = DList->NextAnd;D != NULL;D = D->NextAnd)
        {
        *MStr += MDependType[DList->Type];
        *MStr += ':';
        *MStr += DList->Value;
        }

      break;
    }

  return(SUCCESS);
  } /* END MDependToString() */






/**
 * render an msdata_t struct to String
 *
 * @param SData  (I)
 * @param Out    (O)
 */

int MStagingDataToString(

  msdata_t  *SData,
  mstring_t *Out)

  {
  msdata_t *Link = SData;

  if (Link == NULL)
    {
    return(FAILURE);
    }

  for (Link = SData;Link != NULL;Link = Link->Next)
    {
    MStringAppendF(Out,"%s%%%s&",Link->SrcLocation,Link->DstLocation);
    }

  return(SUCCESS);
  } /* END MStagingDataToString() */





/**
 * Print an RMX attribute to string
 *
 * @see MRMXToString() - parent
 * @see MJobProcessExtensionString() - peer
 *
 * NOTE:  sync reporting format with parsing format found in
 *        MJobProcessExtensionString()
 *
 * @param J       (I)
 * @param Attr    (I)
 * @param Out     (O)
 *
 */

int MRMXAttrToMString(

  mjob_t           *J,
  enum MXAttrType   Attr,
  mstring_t        *Out)

  {
  mreq_t    *FirstReq = J->Req[0];
  moptreq_t *FirstOptReq = (FirstReq == NULL) ? NULL : FirstReq->OptReq;
  mbool_t    HaveFirstReq = (FirstReq != NULL);
  mbool_t    HaveFirstOptReq = (FirstOptReq != NULL);
  int        index;
  int        rc;

  if (Out == NULL)
    {
    return(FAILURE);
    }

  switch (Attr)
    {
    case mxaAdvRsv:

      if (bmisset(&J->IFlags,mjifNotAdvres))
        MStringAppendF(Out,"!%s",J->ReqRID);
      else if (bmisset(&J->IFlags,mjfAdvRsv))
        MStringAppend(Out,J->ReqRID);

      break;

    case mxaArray:
      {
      mjarray_t *Array = J->Array;

      if (Array == NULL)
        {
        return(FAILURE);
        }

      if (Array->Count == 1)
        {
        MStringAppendF(Out,"%s[%d]",
          Array->Name,Array->Members[0]);
        }
      else
        {
        int Start = Array->Members[0];
        int End = Array->Count + Start - 1; /* - 1 because the range is inclusive */

        MStringAppendF(Out,"%s[%d-%d]",
          Array->Name,Start,End);

        if (Array->Limit != 0)
          MStringAppendF(Out,"%%%d",Array->Limit);
        }
      }

      break;

    case mxaBlocking:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      if (J->Req[0]->BlockingFactor != 0)
        {
        MStringAppendF(Out,"%d",J->Req[0]->BlockingFactor);
        }

      break;

    case mxaDeadline:

      MStringAppendF(Out,"%lu",
        J->CMaxDate);

      break;

    case mxaDependency:

      {
      enum MRMTypeEnum RMType = (J->DestinationRM == NULL) ? mrmtNONE : J->DestinationRM->Type;

      if (J->Depend == NULL)
        {
        return(FAILURE);
        }

      MDependToString(Out,J->Depend,RMType);
      }

      break;

    case mxaDDisk:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppendF(Out,"%dM",
        FirstReq->DRes.Disk);

      break;

    case mxaDMem:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppendF(Out,"%dM",
        FirstReq->DRes.Mem);

      break;

    case mxaEnvRequested:

      MStringAppend(Out,MBool[bmisset(&J->IFlags,mjifEnvRequested)]);

      break;

    case mxaExcHList:

      if (MNLIsEmpty(&J->ExcHList))
        {
        return(FAILURE);
        }

      MNLToMString(&J->ExcHList,FALSE,",",'\0',-1,Out);

      break;

    case mxaFeature:

      {
      char String[MMAX_LINE];

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MRMXFeaturesToString(FirstReq,String);

      MStringAppend(Out,String);
      }

      break;

    case mxaFlags:
    case mxaJobFlags:

      MJobFlagsToMString(J,&J->SpecFlags,Out);

      break;

    case mxaGAttr:

      if (!bmisclear(&J->AttrBM))
        {
        char tmpLine[MMAX_LINE];

        MUJobAttributesToString(&J->AttrBM,tmpLine);

        MStringAppend(Out,tmpLine);
        }

      break;

    case mxaGeometry:

      if (J->Geometry == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->Geometry);

      break;

    case mxaGMetric:

      if (!HaveFirstOptReq || 
         (FirstOptReq->GMReq == NULL) || 
         (FirstOptReq->GMReq->Ptr == NULL))
        {
        return(FAILURE);
        }
      else
        {
        mgmreq_t *G;

        G = (mgmreq_t *)FirstOptReq->GMReq->Ptr;

        /* NOTE:  only report first generic metric requirement (FIXME) */

        if (G->GMVal[0] != '\0')
          {
          MStringAppendF(Out,"%s:%s:%s",
            MGMetric.Name[G->GMIndex],
            MAMOCmp[G->GMComp],
            G->GMVal);
          }
        else
          {
          MStringAppendF(Out,"%s:%s",
            MGMetric.Name[G->GMIndex],
            MAMOCmp[G->GMComp]);
          }
        }

      break;

    case mxaGRes:
    case mxaVRes:

      {
      /* NO-OP */
      }

      break;

    case mxaHostList:

      {
      char const *HLMode;

      if (MNLIsEmpty(&J->ReqHList))
        {
        return(FAILURE);
        }

      switch (J->ReqHLMode)
        {
        case mhlmSubset:

          HLMode = "*";
          break;

        case mhlmSuperset:

          HLMode = "^";
          break;

        default:

          HLMode = NULL;
          break;
        }

      if (HLMode != NULL)
        MStringAppend(Out,"*");

      MNLToMString(&J->ReqHList,FALSE,",",'\0',-1,Out);
      }

      break;

    case mxaImage:

      if (!HaveFirstOptReq)
        {
        return(FAILURE);
        }

      MStringAppend(Out,FirstOptReq->ReqImage);

      break;

    case mxaJobGroup:

      if (J->JGroup == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->JGroup->Name);

      break;

    case mxaLocalDataStageHead:

      if ((J->DataStage == NULL) || (J->DataStage->LocalDataStageHead == NULL))
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->DataStage->LocalDataStageHead);

      break;

    case mxaLogLevel:

      MStringAppendF(Out,"%d",
        J->LogLevel);

      break;

    case mxaMaxJobMem:

      MStringAppendF(Out,"%dM",
        J->MaxJobMem);

      break;

    case mxaMaxJobProc:

      MStringAppendF(Out,"%d",
        J->MaxJobProc);

      break;

    case mxaMinPreemptTime:

      if ((J->TemplateExtensions == NULL) || (J->TemplateExtensions->SpecMinPreemptTime == NULL))
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->TemplateExtensions->SpecMinPreemptTime);

      break;

    case mxaMinProcSpeed:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppendF(Out,"%d",
        FirstReq->MinProcSpeed);

      break;

    case mxaMinWCLimit:

      MStringAppendF(Out,"%lu",
        J->OrigMinWCLimit);

      break;

    case mxaMoabStageIn:
    case mxaStageIn:

      if (J->DataStage == NULL)
        {
        return(FAILURE);
        }

      return(MStagingDataToString(J->DataStage->SIData,Out));

      break;

    case mxaNAccessPolicy:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppend(Out,MNAccessPolicy[FirstReq->NAccessPolicy]);

      break;

    case mxaNAllocPolicy:

      if (!HaveFirstReq || (FirstReq->NAllocPolicy == mnalNONE))
        {
        return(FAILURE);
        }

      MStringAppend(Out,MNAllocPolicy[FirstReq->NAllocPolicy]);

      break;

    case mxaNMatchPolicy:

      bmtostring(&J->NodeMatchBM,MJobNodeMatchType,Out);

      break;

    case mxaNodeMem:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppendF(Out,"%d",
        FirstReq->ReqNR[mrMem]);

      break;

    case mxaNodeScaling:

      MStringAppend(Out,MBool[bmisset(&J->IFlags,mjifNodeScaling)]);

      break;

    case mxaNodeSet:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppend(Out,MResSetSelectionType[FirstReq->SetSelection]);

      if (FirstReq->SetType != mrstNONE)
        {
        int FSize;

        MStringAppendF(Out,":%s",MResSetAttrType[FirstReq->SetType]);

        FSize = (int)(sizeof(FirstReq->SetList) / sizeof(FirstReq->SetList[0]));

        for (index = 0;index < FSize;index++)
          {
          char *Str = FirstReq->SetList[index];

          if (Str == NULL)
            break;

          MStringAppendF(Out,":%s",Str);
          }
        }

      break;

    case mxaNodeSetDelay:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppendF(Out,"%ld",FirstReq->SetDelay);

      break;

    case mxaNodeSetCount:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppendF(Out,"%ld",FirstReq->NodeSetCount);

      break;

    case mxaOS:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppend(Out,MAList[meOpsys][FirstReq->Opsys]);

      break;

    case mxaPMask:

      {
      mbool_t Print = FALSE;

      for (index = 0;index < (MMAX_PAR + 1);index++)
        {
        if (!bmisset(&J->SpecPAL,index))
          continue;

        if (Print == TRUE)
          {
          MStringAppend(Out,",");
          }

        Print = TRUE;
        MStringAppend(Out,MPar[index].Name);
        }
      }

      break;

    case mxaPostStageAction:

      if ((J->DataStage == NULL) || (J->DataStage->PostStagePath == NULL))
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->DataStage->PostStagePath);

      break;

    case mxaPostStageAfterType:

      if (J->DataStage != NULL)
        {
        bmtostring(&J->DataStage->PostStageAfterType,MPostDSFile,Out,NULL,NULL);
        }
      else
        {
        return(FAILURE);
        }

      break;

    case mxaPref:

      if (!HaveFirstReq || (FirstReq->Pref == NULL))
        {
        return(FAILURE);
        }

      rc = MRMXPrefToString(FirstReq->Pref,Out);

      if ((rc == FAILURE))
        {
        return(FAILURE);
        }

      break;

    case mxaPrologueScript:
    case mxaEpilogueScript:

      if ((Attr == mxaPrologueScript) &&
          (J->PrologueScript != NULL))
        {
        MStringAppend(Out,J->PrologueScript);
        }
      else if (J->EpilogueScript != NULL)
        {
        MStringAppend(Out,J->EpilogueScript);
        }

      break;

    case mxaProcs:

      /* NYI need new var */

      return(FAILURE);

      break;

    case mxaQOS:

      if (J->QOSRequested == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->QOSRequested->Name);

      break;

    case mxaQueueJob:

      MStringAppend(Out,MBool[!bmisset(&J->SpecFlags,mjfNoQueue)]);

      break;

    case mxaReqAttr:

      /* not supported, this will print out in a req's xml (see MReqToXML) */

      break;

    case mxaResFailPolicy:

      MStringAppend(Out,MARFPolicy[J->ResFailPolicy]);

      break;

    case mxaRMType:

      /* NYI (at all) */

      return(FAILURE);

      break;

    case mxaSelect:

      /* NYI (need new data structure */

      return(FAILURE);

      break;

    case mxaSGE:

      {
      int gresIndex = MUMAGetIndex(meGRes,"SGE",mVerify);
      mbool_t FoundGRes = FALSE;

      if (gresIndex <= 0)
        {
        return(FAILURE);
        }

      for (index = 0;J->Req[index] != NULL;index++)
        {
        int count = MSNLGetIndexCount(&J->Req[index]->DRes.GenericRes,gresIndex);

        if (count > 0)
          {
          MStringAppendF(Out,"%d",count);
          FoundGRes = TRUE;

          break;
          }
        }

      return(FoundGRes);
      }

      break;

    case mxaSID:

      if (J->SystemID == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->SystemID);

      break;

    case mxaSignal:

      {
      mreqsignal_t *tmpSignal;

      int sindex;

      if (MJobCheckAllocTX(J) == SUCCESS)
        {
        for (sindex = 0;sindex < J->TemplateExtensions->Signals.NumItems;sindex++)
          {
          tmpSignal = (mreqsignal_t *)MUArrayListGet(&J->TemplateExtensions->Signals,0);

          MStringAppendF(Out,"sig%s@%lu",MSigName[tmpSignal->Signo],tmpSignal->TimeOffset);
          }
        }
      }

      break;

    case mxaSJID:

      if (J->SystemJID == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->SystemJID);

      break;

    case mxaSRMJID:

      if (J->SRMJID == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->SRMJID);

      break;

    case mxaStageOut:
    case mxaMoabStageOut:

      if (J->DataStage == NULL)
        {
        return(FAILURE);
        }

      return(MStagingDataToString(J->DataStage->SOData,Out));

      break;

    case mxaStdErr:

      if (J->Env.Error == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->Env.Error);

      break;

    case mxaStdOut:

      if (J->Env.Output == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->Env.Output);

      break;

    case mxaTaskDistPolicy:

      if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->BlockingTaskDistPolicySpecified == TRUE))
        {
        if (!HaveFirstReq)
          {
          return(FAILURE);
          }

        MStringAppendF(Out,"blocking:%d",
          FirstReq->TasksPerNode);
        }
      else
        {
        if (J->SpecDistPolicy == mtdNONE)
          {
          return(FAILURE);
          }

        MStringAppend(Out,MTaskDistributionPolicy[J->SpecDistPolicy]);
        }

      break;

    case mxaTemplate:

      if (J->TemplateExtensions == NULL)
        break;

      if (bmisset(&J->IFlags,mjifNoTemplates))
        MStringAppend(Out,"^");

      MStringAppend(Out,J->TemplateExtensions->ReqSetTemplates.c_str());

      break;

    case mxaTermSig:

      if (J->TermSig == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->TermSig);

      break;

    case mxaTermTime:

      MStringAppendF(Out,"%lu",J->TermTime);

      break;

    case mxaTID:

      /* NYI (This is ignored in MJobProcessExtensionString) */

      return(FAILURE);

      break;

    case mxaTPN:

      {
      char const *extraChar = NULL;

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      MStringAppendF(Out,"%d",
        FirstReq->TasksPerNode);

      if (bmisset(&J->IFlags,mjifExactTPN))
        extraChar = "-";
      else if (!bmisset(&J->IFlags,mjifMaxTPN))
        extraChar = "+";

      if (extraChar != NULL)
        MStringAppend(Out,extraChar);
      }

      break;

    case mxaTrig:

      {
      int Size;

      mjtx_t *TX = J->TemplateExtensions;

      if (TX == NULL)
        {
        return(FAILURE);
        }

      for (Size = 0;TX->SpecTrig[Size] != NULL;Size++)
        {
        MStringAppend(Out,TX->SpecTrig[Size]);
        }
      }

      break;

    case mxaTrigNamespace:

      {
      char *tmpS = NULL;

      if (MJobGetDynamicAttr(J,mjaTrigNamespace,(void **)&tmpS,mdfString) == FAILURE)
        {
        return(FAILURE);
        }

      MStringAppend(Out,tmpS);

      MUFree(&tmpS);
      }

      break;

    case mxaTRL:

      if (!HaveFirstReq)
        {
        return(FAILURE);
        }

      if (FirstReq->TaskRequestList[1] <= 0)
        {
        return(SUCCESS);
        }

      MStringAppendF(Out,"%d@%lu",
        FirstReq->TaskRequestList[1],
        J->SpecWCLimit[1]);

      for (index = 2;FirstReq->TaskRequestList[index] > 0;index++)
        {
        MStringAppendF(Out,":%d@%lu",
          FirstReq->TaskRequestList[index],
          J->SpecWCLimit[index]);
        }

      break;

    case mxaTTC:

      MStringAppendF(Out,"%d",
        J->TotalTaskCount);

      break;

    case mxaUMask:

      MStringAppendF(Out,"0%o",
        J->Env.UMask);

      break;

    case mxaSPrio:

      MStringAppendF(Out,"%ld",
        J->SystemPrio);

      break;

    case mxaVariables:

      {
      mstring_t Var(MMAX_LINE);

      MUHTToMStringDelim(&J->Variables,&Var,";");

      *Out += Var;
      }

      break;

    case mxaVMUsage:

      if (J->VMUsagePolicy != mvmupNONE)
        MStringAppend(Out,MVMUsagePolicy[J->VMUsagePolicy]);

      break;

    case mxaVPC:

      if (J->ReqVPC == NULL)
        {
        return(FAILURE);
        }

      MStringAppend(Out,J->ReqVPC);

      break;

    case mxaWCKey:

      {
      char *wckey = NULL;

      if (MUHTGet(&J->Variables,"wckey",(void **)&wckey,NULL) == SUCCESS)
        MStringAppend(Out,wckey);
      }

      break;

    case mxaWCRequeue:

      MStringAppend(Out,MBool[bmisset(&J->IFlags,mjifWCRequeue)]);

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (Attr) */

  return(SUCCESS);
  } /* END MRMXAttrToMString() */




/**
 * Split a string list into multiple RMX Attribute pairs, one pair for each
 * element in the list. The list must use ';' as a delimiter. Sets an mstring (already init'd).
 *
 * @param BPtr   (I/O)
 * @param Type    (I)
 * @param ValList (I)
 */

int MRMXAttrValListSplitMString(

  mstring_t      *BPtr,
  enum MXAttrType Type,
  char const     *ValList)

  {
  char *ScanPtr;
  char const *ScanContext = NULL;
  int  ScanLength;

  ScanPtr = MUStrScan(ValList,";",&ScanLength,&ScanContext);

  MStringAppendF(BPtr,"%s=%.*s",
    MRMXAttr[Type],ScanLength,ScanPtr);
  
  while ((ScanPtr = MUStrScan(ScanContext,";",&ScanLength,&ScanContext)) != NULL)
    {
    MStringAppendF(BPtr,";%s=%.*s",
      MRMXAttr[Type],ScanLength,ScanPtr);
    }

  return(SUCCESS);
  }  /* END MRMXAttrValListSplitMString() */



/**
 * Render all resource manager extensions specified in BM to String. if BM is NULL,
 * then all attributes will be rendered.
 *
 * NOTE:  There is also an MRMXToMString()
 *
 * @see MRMXAttrToMString() - child
 *
 * @param J    (I)
 * @param Out (I/O)
 * @param BM (I) [optional]
 */

int MRMXToMString(

  mjob_t               *J,
  mstring_t            *Out,
  const mbitmap_t      *BM)

  {
  enum MXAttrType AttrIndex;

  mbitmap_t allBM;

  int rc;

  if (BM == NULL)
    {
    bmsetall(&allBM,mxaLAST);

    /* mask out bits that duplicate the meaning other bits. This is not
       true of mxaGRes and mxaVRes because they behave slightly differently */
    bmunset(&allBM,mxaFlags);
    bmunset(&allBM,mxaMoabStageIn);
    bmunset(&allBM,mxaMoabStageOut);
    BM = &allBM;
    }

  mstring_t  Attr(MMAX_LINE);

  for (AttrIndex = (enum MXAttrType)(mxaNONE + 1);
       AttrIndex < mxaLAST;
       AttrIndex = (enum MXAttrType)(AttrIndex + 1))
    {
    if (!bmisset(BM,AttrIndex))
      continue;

    MStringSet(&Attr,"\0");

    rc = MRMXAttrToMString(J,AttrIndex,&Attr);

    if (rc == FAILURE)
      {
      continue;
      }

    switch (AttrIndex)
      {
      case mxaSignal:

        /* Attribute can be specified many times, i.e. SIGNAL=sighup;SIGNAL=sigterm.
         * MRMXAttrToMString will convert each request into a ';' delimited list.
         * This list needs to be split back into multiple attribute pairs.
         */
        MRMXAttrValListSplitMString(Out,AttrIndex,Attr.c_str());

        break;

      case mxaVariables:

        /* Attribute can be specified many times, i.e. SIGNAL=sighup;SIGNAL=sigterm.
         * MRMXAttrToMString will convert each request into a ';' delimited list.
         * This list needs to be split back into multiple attribute pairs.
         */
        MRMXAttrValListSplitMString(Out,AttrIndex,Attr.c_str());

        break;

      default:

        MStringAppendF(Out,"%s=%s",MRMXAttr[AttrIndex],Attr.c_str());

        break;
      }

    *Out += ';';
    }

  bmclear(&allBM);

  return (SUCCESS);
  } /* END MRMXToString() */

static const char *DelimList = ";?\n";  /* field delimiter */





#define MMAX_XATTR  32

/**
 * Process RM extensions.
 *
 * @see MRMXAttrToMString() - peer
 * 
 * NOTE:  sync parsing format with reporting format found in
 *        MRMXAttrToMString()
 *
 * @param J (I) [modified]
 * @param XString (I)
 * @param XType (I) process XString as specified type [optional]
 * @param XList (I) ignore all attrs in XString except this one [optional]
 * @param SEMsg (O) [optional,minsize=MMAX_LINE] 
 */

int MJobProcessExtensionString(

  mjob_t          *J,        /* I (modified) */
  char            *XString,  /* I */
  enum MXAttrType  XType,    /* I (optional) process XString as specified type */
  mbitmap_t       *XList,    /* I (optional) ignore all attrs in XString except these (bitmap of MXAttrType) */
  char            *SEMsg)    /* O (optional,minsize=MMAX_LINE) */
 
  {
  char   *ptr;
  char   *tail;
  char   *key;
 
  enum MXAttrType aindex;
  int     xindex;

  /* Using marray_t for XAttr and XVal to avoid segfault by overwriting an array */

  marray_t  XAttr;
  marray_t  XVal;
  
  int     index;  
  int     nindex;
  int     tindex;
  int     StrLen;
 
  char    tmpLine[MMAX_BUFFER << 1];  /* NOTE:  must be large enough to support superset hostlist */

  char    tmpBuf[MMAX_BUFFER];
 
  mreq_t  *tmpRQPtr;
  mreq_t  *RQ;
 
  mnode_t *N;
 
  char   *Value;
 
  char   *TokPtr;
  char   *TokPtr2;

  char    tmpEMsg[MMAX_LINE];
  char   *EMsg;

  const char *FName = "MJobProcessExtensionString";

  MDB(6,fCONFIG) MLog("%s(%s,%s,%s,XList,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : NULL,
    (XString != NULL) ? XString : "NULL",
    MRMXAttr[XType]);

  EMsg = (SEMsg != NULL) ? SEMsg : tmpEMsg;

  EMsg[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (MUStrIsEmpty(XString))
    {
    MDB(6,fCONFIG) MLog("INFO:     extension string is empty\n");
 
    return(SUCCESS);
    } 

  /* Initialize array lists (sizeof enum is sizeof int) */

  MUArrayListCreate(&XAttr,sizeof(int),MMAX_XATTR);
  MUArrayListCreate(&XVal,sizeof(char *),MMAX_XATTR);

  /* FORMAT:  [x=]<RMSTRING> */

  /* NOTE:  'x=' may be specified w/ PBS/TORQUE compatible jobs */

  ptr = &XString[0];

  if (!strncasecmp(ptr,"x=",strlen("x=")))
    {
    ptr += strlen("x=");
    }

  MUStrCpy(tmpBuf,ptr,sizeof(tmpBuf));

  /* FORMAT:  [<WS>]<ATTR>=<VALUE>[<WS>]{;?}... */
  /* FORMAT:  <ATTR>:<VALUE>[{;?}<ATTR>:<VALUE>]... */

  /* NOTE:  handle '=' delimiters for backwards compatibility */

  /* set defaults */
 
  RQ = J->Req[0];

  /* NOTE:  is character removal dangerous?  is anything using field level quoting? */

  /* remove trailing whitespace and quote characters */

  ptr = tmpBuf + strlen(tmpBuf) - 1;

  while (isspace(*ptr) || (*ptr == '\'') || (*ptr == '\"'))
    {
    *ptr = '\0';

    ptr--;
    }

  /* remove leading whitespace and quote characters */

  ptr = tmpBuf;

  while (isspace(*ptr) || (*ptr == '\'') || (*ptr == '\"'))
    {
    ptr++;
    }

  /* remove trailing whitespace and quote characters */


  /* NOTE:  both the WIKI interface and the RM extension string interface use ';' as record delimiter */
  /*        this causes issues as a RMX string is a single field inside of a WIKI record */

  /* NOTE:  '?' added as wiki delimiter */

  key = MUStrTok(ptr,DelimList,&TokPtr);

  xindex = 0;

  while (key != NULL)
    {
    if (!strncmp(key,"x=",strlen("x=")))
      {
      key += strlen("x=");
      }
      
    if (XType == mxaNONE)
      {
      aindex = (enum MXAttrType)MUGetIndexCI(key,MRMXAttr,MBNOTSET,mxaNONE);

      if (aindex == mxaNONE)
        {
        sprintf(tmpLine,"cannot parse extension attribute '%s'",
          key);

        MMBAdd(
          &J->MessageBuffer,
          tmpLine,
          NULL,
          mmbtOther,
          MMAX_TIME,
          0,
          NULL);
 
        key = MUStrTok(NULL,DelimList,&TokPtr);

        continue;
        }

      if ((XList != NULL) && (bmisset(XList,aindex) == FALSE))
        {
        /* attribute not included in list */

        key = MUStrTok(NULL,DelimList,&TokPtr);

        continue;
        }

      Value = key + strlen(MRMXAttr[aindex]);

      /* remove leading whitespace */

      while (isspace(*Value) || (*Value == '\'') || (*Value == '\"'))
        {
        Value++;
        }

      /* make sure that the extension matches exactly with the assumed attribute, (i.e. featurethatisvoid=... is rejected)*/

      if ((*Value != '=') && (*Value != ':'))
        {
        MUArrayListFree(&XVal);
        MUArrayListFree(&XAttr);

        return(FAILURE);
        }

      /* remove separation character */

      while ((*Value == '=') || (*Value == ':'))
        {
        Value++;
        }

      while (isspace(*Value) || (*Value == '\'') || (*Value == '\"'))
        {
        Value++;
        }
      }    /* END if (XType == mxaNONE) */
    else
      {
      aindex = XType;

      Value = key;
      }    /* END else (XType == mxaNONE) */

    /* remove trailing whitespace */

    for (index = strlen(Value) - 1;index > 0;index--)
      {
      if ((!isspace(Value[index])) && (Value[index] != '\'') && (Value[index] != '\"'))
        break;

      Value[index] = '\0';
      }  /* END for (index) */

    MUArrayListAppend(&XAttr,(void *)&aindex);
    MUArrayListAppendPtr(&XVal,(void *)Value);

    key = MUStrTok(NULL,DelimList,&TokPtr);        
    }    /* END while (key != NULL) */

  xindex = mxaLAST;
  MUArrayListAppend(&XAttr,(void *)&xindex);

  for (xindex = 0; MUArrayListGet(&XAttr,xindex) != NULL;xindex++)
    {
    enum MXAttrType *TypePtr;
    char **ValuePtr;

    TypePtr = (enum MXAttrType *)MUArrayListGet(&XAttr,xindex);
    aindex = *TypePtr;

    if (aindex == mxaLAST)
      break;

    ValuePtr = (char **)MUArrayListGet(&XVal,xindex);
    Value = *ValuePtr;

    if ((Value == NULL) || (Value[0] == '\0'))
      continue;

    switch (aindex)
      {
      case mxaAdvRsv:

        {
        char *tmpPtr;

        /* advance reservation flag/value */

        /* FORMAT:  [<RSVID>] */

        tmpPtr = Value;

        if (tmpPtr[0] == '!')
          {
          bmset(&J->IFlags,mjifNotAdvres);

          tmpPtr++;
          }

        bmset(&J->SpecFlags,mjfAdvRsv);

        if (tmpPtr[0] != '\0')
          MUStrDup(&J->ReqRID,tmpPtr);
        }

        break;

      case mxaArray:

        /* FORMAT:  <name>[<indexlist]%<limit> */
        /*          myarray[1-1000]%5 */
        /*          myarray[1,2,3,4]  */
        /*          myarray[1-5,7,10] */

        /* NOTE: only load array info if NULL, do not overwrite */

        if ((J->SubmitRM != NULL) && (J->SubmitRM->Type != mrmtS3))
          {
          break;
          }

        if (MSched.EnableJobArrays == FALSE)
          {
          MUStrCpy(EMsg,"job arrays are not enabled\n",MMAX_LINE);

          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);
          return(FAILURE);
          }

        /* processed in MJobProcessRestrictedAttrs() */

        MJobCheckAllocTX(J);
        MUStrDup(&J->TemplateExtensions->SpecArray,Value);

        break;

      case mxaBlocking:

        RQ->BlockingFactor = (int)strtol(Value,NULL,10);

        MJobParseBlocking(J);

        break;

      case mxaDDisk:

        /* per task dedicated disk */

        RQ->DRes.Disk = (int)MURSpecToL(Value,mvmMega,mvmMega);

        if (Value[0] != '0')
          RQ->DRes.Disk = MAX(RQ->DRes.Disk,1);

        MDB(5,fCONFIG) MLog("INFO:     per task dedicated disk set to %d MB\n",
          RQ->DRes.Disk);

        break;

      case mxaDeadline:

        {
        mulong tmpL;

        /* job completion deadline */

        /* FORMAT:  <TIME> */

        tmpL = MUTimeFromString(Value);
        
        if (tmpL < MCONST_EFFINF)  /* is tmpL < ~6.0 years after 1970? */
          {
          /* specification is relative */

          J->CMaxDate = J->SubmitTime + tmpL;
          }
        else
          {
          /* absolute time */

          J->CMaxDate = tmpL;
          }

        J->CMaxDateIsActive = MBNOTSET;
        }  /* END BLOCK (case mxaDeadline) */

        break;

      case mxaDependency:

        {
        /* job dependency */

        /* FORMAT: [<TYPE>:]<SPECSTRING>[&&[<TYPE>:]<SPECSTRING>]... */

        if (MJobAddDependency(J,Value) == FAILURE)
          {
          char tmpLine[MMAX_LINE];

          /* cannot create requested dependency */

          MDB(3,fCONFIG) MLog("INFO:     cannot create requested dependency '%s' for job %s\n",
            Value,
            J->Name);

          snprintf(tmpLine,sizeof(tmpLine),"cannot create requested dependency '%s'\n",
            Value);

          MJobSetHold(J,mhBatch,0,mhrNoResources,tmpLine);

          MUStrCpy(EMsg,tmpLine,MMAX_LINE);

          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);

          return(FAILURE);
          }
        }  /* END BLOCK (case mxaDependency) */

        break;

      case mxaDMem:

        /* per task dedicated real memory */

        RQ->DRes.Mem = (int)MURSpecToL(Value,mvmMega,mvmByte);

        if (Value[0] != '0')
          RQ->DRes.Mem = MAX(RQ->DRes.Mem,1);

        MDB(5,fCONFIG) MLog("INFO:     per task dedicated memory set to %d MB\n",
          RQ->DRes.Mem);

        break;

      case mxaEnvRequested:

        /* msub -E specified */

        if (MUBoolFromString((char *)Value,FALSE) == TRUE)
          bmset(&J->IFlags,mjifEnvRequested);

        break;

      case mxaExcHList:

        /* specify the exclusion host list */

        MJobSetAttr(J,mjaExcHList,(void **)Value,mdfString,mSet); 

        break; 

      case mxaFeature:

        MReqSetFBM(J,RQ,mAdd,Value);

        break;

      case mxaFlags:
      case mxaJobFlags:

        {
        /* job flags */

        /* FORMAT:  <JOBFLAG>[{,:}<JOBFLAG>]... */

        MJobFlagsFromString(J,RQ,NULL,NULL,NULL,Value,FALSE);

        MJobUpdateFlags(J);
        }  /* END BLOCK (case mxaFlags) */

        break;

      case mxaJobRejectPolicy:

        {
        char *Array[2];

        Array[0] = Value;
        Array[1] = NULL;

        MSchedProcessJobRejectPolicy(&J->JobRejectPolicyBM,Array);
        }

        break;

      case mxaGAttr:

        /* generic job attribute (assigned to job) */

        MJobSetAttr(J,mjaGAttr,(void **)Value,mdfString,mSet);

        break;

      case mxaGeometry:

        MUStrDup(&J->Geometry,Value);

        J->DistPolicy     = mtdArbGeo;
        J->SpecDistPolicy = mtdArbGeo;

        if (strchr(J->Geometry,'(') != NULL)
          MJobParseGeometry(J,J->Geometry);

        break;

      case mxaGMetric:

        /* FORMAT:  gmetric:<NAME><CMP><VAL>   */
        /* FORMAT:  gmetric:<NAME>:<CMP>:<VAL> */
        /* Value            ^----------------^ */

        MJobAddGMReq(J,RQ,Value);
 
        break;

      case mxaGRes:
      case mxaVRes:
      case mxaCGRes:

        {
        char *ptr;
        char *TokPtr;

        char *ptr2;
        char *TokPtr2;

        int   tmpS;
        
        long  TimeFrame;

        /* generic node resource (required/consumed by job) */

        /* FORMAT:  <ATTR>[{:+}<COUNT>][@<TIME>][,<ATTR>[{+:}<COUNT>][@<TIME>]... */

        ptr = MUStrTok(Value,",%",&TokPtr);

        while (ptr != NULL)
          {
          if (((ptr2 = strchr(ptr,'+')) != NULL) || ((ptr2 = strchr(ptr,':')) != NULL))
            {
            tmpS = (unsigned int)strtol(&ptr2[1],NULL,10);
            }
          else
            {
            tmpS = 1;
            }
          
          if ((ptr2 = strchr(ptr,'@')) != NULL)
            {
            TimeFrame = MUTimeFromString(&ptr2[1]);
            }
          else
            {
            TimeFrame = -1;
            }

          ptr2 = MUStrTok(ptr,"+:@",&TokPtr2);

          if (TimeFrame > 0)
            {
            if (MJOBISACTIVE(J) && 
                (MSched.Time - J->StartTime > (unsigned long)TimeFrame))
              {
              /* ignore the gres requirement */

              break;
              }
            else
              {
              mtrig_t *T;

              marray_t TList;

              char tmpLine[MMAX_LINE];

              MUArrayListCreate(&TList,sizeof(mtrig_t *),10);

              snprintf(tmpLine,sizeof(tmpLine),"atype=internal,etype=start,offset=%ld,action=\"job:-:modify:gres-=%s\"",
                TimeFrame,
                ptr);

              if (MTrigLoadString(
                    &J->Triggers,
                    tmpLine,
                    TRUE,
                    FALSE,
                    mxoJob,
                    J->Name,
                    &TList,
                    EMsg) == FAILURE)
                {
                /* NOTE:  should failure be reported? */

                break;
                }

              T = (mtrig_t *)MUArrayListGetPtr(&TList,0);

              T->O = (void *)J;
              T->OType = mxoJob;
     
              MUStrDup(&T->OID,J->Name);
     
              MTrigInitialize(T);
     
              MUArrayListFree(&TList);
              }   /* END else (TimeFrame > 0) */
            }     /* END if ((TimeFrame > 0) && MJOBISACTIVE(J)) */

          if (MJobAddRequiredGRes(
                J,
                ptr,
                tmpS,
                (enum MXAttrType)aindex,
                FALSE,
                FALSE) == FAILURE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"cannot locate required resource '%s'",
              ptr);

            MJobSetHold(J,mhBatch,0,mhrNoResources,tmpLine);

            MUArrayListFree(&XVal);
            MUArrayListFree(&XAttr);

            MUStrCpy(EMsg,tmpLine,MMAX_LINE);

            return(FAILURE);
            }

          ptr = MUStrTok(NULL,",%",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END BLOCK */

        break;

      case mxaHostList:

        {
        char *ptr;

        /* HOSTLIST */

        /* FORMAT:  <HOSTNAME>[{,|:|+}<HOSTNAME>][^]... */

        if (MSched.Mode == msmProfile)
          break;

        /* clear/initialize job hostlist attributes */

        bmset(&J->IFlags,mjifHostList);
        J->ReqHLMode = mhlmNONE;

        MJobUpdateFlags(J);
 
        MNLClear(&J->ReqHList);

        MUStrCpy(tmpLine,Value,sizeof(tmpLine));

        if ((ptr = strchr(tmpLine,'^')) != NULL)
          {
          /* superset mode forced */

          /* setting ptr to '\0' makes MUStrTok think it is at end of the string */

          *ptr = '+';

          J->ReqHLMode = mhlmSuperset;
          }
        else if ((ptr = strchr(tmpLine,'*')) != NULL)
          {
          /* subset mode forced */

          *ptr = '+';

          J->ReqHLMode = mhlmSubset;
          }
 
        ptr = MUStrTokEArray(tmpLine,",:+",&TokPtr2,FALSE);
 
        nindex = 0;

        MNLTerminateAtIndex(&J->ReqHList,nindex);

        J->HLIndex = 0;

        while (ptr != NULL)
          {
          if (strchr(ptr,'[') == NULL)
            {
            /* single node token */

            if (MNodeFind(ptr,&N) == FAILURE)
              {
              MDB(1,fSTRUCT) MLog("ALERT:    cannot locate node '%s' for job '%s' hostlist\n",
                ptr,
                J->Name);

              ptr = MUStrTok(NULL,",:+",&TokPtr2);              

              continue;
              }
 
            if (MNLReturnNodeAtIndex(&J->ReqHList,nindex) == N)
              {
              /* duplicate node detected */

              MNLAddTCAtIndex(&J->ReqHList,nindex,1);
              }
            else
              {
              /* new node found */

              if (MNLGetNodeAtIndex(&J->ReqHList,nindex,NULL) == SUCCESS)
                nindex++;

              MNLSetNodeAtIndex(&J->ReqHList,nindex,N);
              MNLSetTCAtIndex(&J->ReqHList,nindex,1);
              }
            } /* END if (strchr(ptr,'[') == NULL) */
          else
            {
            mnode_t *N1;
            mnode_t *N2;

            mnl_t tmpNL;
            int nindex2 = 0;
            
            /* token contains a node array e.g. blah[1-8,9] */

            /* expand hostlist */

            MNLInit(&tmpNL);

            if (MUNLFromRangeString(
                  ptr,
                  NULL,
                  &tmpNL,
                  NULL,
                  0,
                  0,
                  TRUE,
                  FALSE) == FAILURE)
              {
              MNLFree(&tmpNL);

              MUArrayListFree(&XVal);
              MUArrayListFree(&XAttr);

              return(FAILURE);
              }

            while (MNLGetNodeAtIndex(&tmpNL,nindex2,&N2) == SUCCESS)
              {
              MNLGetNodeAtIndex(&J->ReqHList,nindex,&N1);

              if (N2 == N1)
                {
                MNLAddTCAtIndex(&J->ReqHList,nindex,MNLGetTCAtIndex(&tmpNL,nindex2));
                }
              else
                {
                if (N1 != NULL)
                  nindex++;

                MNLSetNodeAtIndex(&J->ReqHList,nindex,N2);
                MNLSetTCAtIndex(&J->ReqHList,nindex,MNLGetTCAtIndex(&tmpNL,nindex2));
                }

              nindex2++;
              } /* END while (tmpNL[].N != NULL) */
            } /* END else ... */
 
          ptr = MUStrTokEArray(NULL,",:+",&TokPtr2,FALSE);
          }  /* END while (ptr != NULL) */

        MNLTerminateAtIndex(&J->ReqHList,nindex + 1);

        if (J->ReqHLMode == mhlmNONE)
          {
          /* Why are we setting the ReqHLMode to Superset or Subset if they 
              didn't specify it?  Also, if they ask for two nodes in the 
              hostlist (both with 20 procs), and nodes=2:ppn=20, then
              J->Request.TC will be 40, and nindex will be 1, so it will
              still get put as subset even though they put a hostlist that
              was exactly what they needed. */

          /* Possibly because with hostlist if you request nodes=1:ppn=8,hostlist=node1
           * the job will only get 1 proc on node1 because the hostlist's TC is 
           * only 1. Otherwise you would have to request -l hostlist=node1+node1+... */

          if (J->Request.TC > 0)
            {
            if (J->Request.TC > nindex)
              J->ReqHLMode = mhlmSubset;
            else if (J->Request.TC < nindex)
              J->ReqHLMode = mhlmSuperset;
            }
          }
        }    /* END BLOCK (case mxaHostList) */

        break;

      case mxaImage:

        MReqSetAttr(
          J,
          J->Req[0],
          mrqaReqImage,
          (void **)Value,
          mdfString,
          mSet);

        break;

      case mxaJobGroup:

        /* NOTE:  jobgroup can be jobid or jobname */

        /* FORMAT:  <GROUPID> */

        MJobSetAttr(J,mjaJobGroup,(void **)Value,mdfString,mSet);

        break;

      case mxaLocalDataStageHead:

        if (J->DataStage == NULL)
          {
          MDSCreate(&J->DataStage);
          }
        
        MUStrDup(&J->DataStage->LocalDataStageHead,Value);

        break;

      case mxaLogLevel:

        J->LogLevel = (int)strtol(Value,NULL,10);

        break;

      case mxaMaxJobMem:

        J->MaxJobMem = MURSpecToL(Value,mvmMega,mvmMega);

        break;

      case mxaMaxJobProc:

        J->MaxJobProc = (int)strtol(Value,NULL,0);

        break;

/*
      case mxaMinDuration:

        MJobSetAttr(J,mjaMinPreemptTime,(void **)Value,mdfString,mSet);

        bmset(&J->IFlags,mjifFlexDuration);

        break;
*/

      case mxaMinPreemptTime:

        /* NOTE:  process in MRMJobPostLoad() after QOS is loaded */

        if (J->TemplateExtensions == NULL) 
          {
          MJobAllocTX(J);
          }

        if (J->TemplateExtensions != NULL)
          MUStrDup(&J->TemplateExtensions->SpecMinPreemptTime,Value);

        break;

      case mxaMinProcSpeed:

        RQ->MinProcSpeed = (int)strtol(Value,NULL,10);
    
        break;

      case mxaMinWCLimit:

        /* Only change if not already set */
 
        if (J->MinWCLimit == 0)
          {
          J->OrigMinWCLimit = MUTimeFromString(Value);
          J->MinWCLimit     = J->OrigMinWCLimit;
          }

        break;

      case mxaNAccessPolicy:

        if (J->Credential.Q != NULL)
          { 
          /* if the job's QOS (which may not even be set at this point) has specified attrs
             and mjdaNAPolicy is set then allow the modification, otherwise don't */

          if ((J->Credential.Q->SpecAttrsSpecified == TRUE) &&
              (!bmisset(&J->Credential.Q->SpecAttrs,mjdaNAPolicy)))
            {
            /* specattrs was specified and NAPolicy was not, which means we don't allow NAPolicy */

            break;
            }
          }
        else if (MSched.DefaultQ != NULL)
          {
          /* if the scheduler's QOS (which may not even be set at this point) has specified attrs
             and mjdaNAPolicy is set then allow the modification, otherwise don't */

          if ((MSched.DefaultQ->SpecAttrsSpecified == TRUE) &&
              (!bmisset(&MSched.DefaultQ->SpecAttrs,mjdaNAPolicy)))
            {
            /* specattrs was specified and NAPolicy was not, which means we don't allow NAPolicy */

            break;
            }
          }

        /* get node access mode */

        RQ->NAccessPolicy = (enum MNodeAccessPolicyEnum)MUGetIndexCI(
          Value,
          MNAccessPolicy,
          FALSE,
          mnacNONE);

        MDB(5,fCONFIG) MLog("INFO:     job %s node access policy set to %s from extension string\n",
          J->Name,
          MNAccessPolicy[RQ->NAccessPolicy]);

        break;

      case mxaNAllocPolicy:

        /* FORMAT:  ??? */

        RQ->NAllocPolicy= (enum MNodeAllocPolicyEnum)MUGetIndexCI(Value,MNAllocPolicy,FALSE,0);
 
        break;

      case mxaNMatchPolicy:

        {
        enum MNodeMatchPolicyEnum tmpNMP;
        TokPtr2 = tmpLine;

        strncpy(tmpLine,Value,sizeof(tmpLine));


        while ((ptr = MUStrTok(TokPtr2,":,",&TokPtr2)) != NULL)
          {
          tmpNMP = (enum MNodeMatchPolicyEnum)MUGetIndexCI(
            ptr,
            MJobNodeMatchType,
            FALSE,
            0);

          if (tmpNMP != mnmpNONE)
            {
            bmset(&J->NodeMatchBM,tmpNMP);
            }
          }
        }


        break;

      case mxaNodeMem:

        /* required node memory configuration */

        /* FORMAT:  <MEM> (in MB) */

        /* NOTE:  support >=, ==, and <= (NYI) */

        RQ->ReqNR[mrMem]  = (int)strtol(Value,NULL,10);
        RQ->ReqNRC[mrMem] = mcmpGE;

        break;

      case mxaNodeScaling:

        if (MUBoolFromString(Value,FALSE) == TRUE)
          {
          bmset(&J->IFlags,mjifNodeScaling);
          }
 
        break;

      case mxaNodeSet:

        if (bmisset(&J->IFlags,mjifIgnoreNodeSets) ||
            bmisset(&MSched.Flags,mschedfDisablePerJobNodesets))
          {
          break;
          }

        /* node set info */

        /* NOTE:  applies to all compute reqs */

        MJobSetNodeSet(J,Value,FALSE);

        break;

      case mxaNodeSetIsOptional:

        if (bmisset(&J->IFlags,mjifIgnoreNodeSets))
          {
          break;
          }

        if (bmisset(&MSched.Flags,mschedfAllowPerJobNodeSetIsOptional))
          {
          mbool_t Optional = MUBoolFromString(Value,TRUE); 

          RQ->SetBlock = !Optional;
          }

	break;

      case mxaNodeSetDelay:

        if (bmisset(&J->IFlags,mjifIgnoreNodeSets))
          {
          break;
          }

        if (bmisset(&MSched.Flags,mschedfAllowPerJobNodeSetIsOptional))
          {
          RQ->SetDelay = MUTimeFromString(Value);

          if (RQ->SetDelay > 0)
            RQ->SetBlock = TRUE;
          else
            RQ->SetBlock = FALSE;

          if (!bmisset(&J->IFlags,mjifSpecNodeSetPolicy))
            RQ->SetSelection = MPar[0].NodeSetPolicy;
          }

        break;

      case mxaNodeSetCount:

        if (bmisset(&J->IFlags,mjifIgnoreNodeSets))
          {
          break;
          }

        RQ->NodeSetCount = (int)strtol(Value,NULL,10);

        break;

      case mxaOS:

        if ((RQ->Opsys = MUMAGetIndex(meOpsys,Value,mAdd)) == FAILURE)
          {
          MDB(1,fSCHED) MLog("WARNING:  job '%s' does not have valid Opsys value '%s'\n",
            J->Name,
            Value);
          }

        break;

      case mxaPMask:

        /* partition mask */

        /* FORMAT:  <PARTITION>[{:|,}<PARTITION>]... */

        MUStrCpy(tmpLine,Value,sizeof(tmpLine));           
 
        if (strstr(tmpLine,MPar[0].Name) == NULL)   /* if not "ALL" */
          {
          /* don't over write SpecPAL from the extension string if we loaded it in from the checkpoint file */

          mbool_t  AllPartitions = FALSE;

          if (bmisset(&J->IFlags,mjifWasLoadedFromCP))
            {
            char ParLine[MMAX_LINE];

            MPALToString(&J->SpecPAL,NULL,ParLine);
  
            if ((!MUStrIsEmpty(ParLine)) && (strstr(ParLine,"internal")))
              {
              /* we have a problem where some jobs somehow incorrectly get all of the partitions
                 in the PAL (including the internal partition) in the checkpoint file so in that
                 case go ahead and get the correct partition list from the extension string.  rt5097
                 Remove this code if we correct the problem with the bad PAL. */
  
              AllPartitions = TRUE;
  
              MDB(3,fSCHED) MLog("INFO:     job '%s' overriding requested SpecPAL (pmask) partition list (%s) with (%s) in process extension string\n",
                J->Name,
                ParLine,
                Value);
              }
            }

          /* don't over write SpecPAL from the extension string if we loaded it in from the checkpoint file ]
            (except in the case of the AllPartitions fix) */

          if ((!bmisset(&J->IFlags,mjifWasLoadedFromCP)) ||
              (AllPartitions == TRUE))
            {
            mbitmap_t tmpPAL;

            char     ParLine[MMAX_LINE];
  
            /* with Verify, will add only valid partitions */
  
            MPALFromString(tmpLine,mVerify,&tmpPAL);
  
            if (bmisclear(&tmpPAL))
              {
              char tmpMsg[MMAX_LINE];
  
              /* no valid partitions specified */
  
              if ((strchr(tmpLine,':') != NULL) || (strchr(tmpLine,',') != NULL))
                {
                snprintf(tmpMsg,sizeof(tmpMsg),"one or more specified partitions do not exist - %s",
                  tmpLine);
                }
              else
                {
                snprintf(tmpMsg,sizeof(tmpMsg),"partition '%s' does not exist",
                  tmpLine);
                }
  
              MJobSetHold(J,mhBatch,0,mhrNoResources,tmpMsg);

              MUStrCpy(EMsg,tmpMsg,MMAX_LINE);

              MUArrayListFree(&XVal);
              MUArrayListFree(&XAttr); 

              return(FAILURE);
              }
  
            bmcopy(&J->SpecPAL,&tmpPAL);

            MDB(3,fCONFIG) MLog("INFO:     job %s SpecPAL (pmask) set to %s (%d) from extension string\n",
              J->Name,
              MPALToString(&J->SpecPAL,NULL,ParLine),
              AllPartitions);         

            bmclear(&tmpPAL);
            }

          /* if non-global pmask specified */

          if (bmisclear(&J->SpecPAL))
            {
            MDB(1,fSTRUCT) MLog("ALERT:    NULL pmask set for job '%s'.  (job cannot run)\n",
              J->Name);
            }
 
          MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
          }    /* END if (strstr() == NULL) */

        break;

      case mxaPref:

        if (!MUStrStr(Value,"PREF",0,TRUE,FALSE))
          {
          MReqSetAttr(J,RQ,mrqaPref,(void **)Value,mdfString,mSet);
          }
        else
          {
          /* FORMAT:  PREF(FEATURE:X,FEATURE:Y,VARIABLE:Z) */

          /* translate to canonical pref spec */

          char *ptr;
          int   index;

          char  tmpLine[MMAX_LINE];

          index = 0;

          tmpLine[0] = '\0';

          for (ptr = Value + strlen("PREF");*ptr != '\0';ptr++)
            {
            switch (*ptr)
              {
              case '(':
              case ')':

                /* ignore */
 
                break;

              case ':':
          
                tmpLine[index++] = '=';
                tmpLine[index++] = '=';

                break;

              case ',':

                tmpLine[index++] = '&';
                tmpLine[index++] = '&';

                break;
             
              default:

                tmpLine[index++] = *ptr;

                break;
              }  /* END switch (*ptr) */ 
            }    /* END for (ptr) */

          tmpLine[index] = '\0';

          MReqRResFromString(J,J->Req[0],tmpLine,mrmtWiki,TRUE);
          }  /* END BLOCK */

        MDB(3,fCONFIG) MLog("INFO:     pref '%s' specified\n",
          Value);

        MSched.ResourcePrefIsActive = TRUE;

        break;

      case mxaTTC:

        /* Treat ttc as procs if just a -l ttc submission. If the submission
         * is -l nodes=4,ttc=4 then ignore ttc. In per-partition scheduling, 
         * if the partition has the UseTTC flag set then Req[0]->TasksPerNode 
         * will be set to Req[0]->TaskCount to lock the job to one node. */
        if (J->Request.TC > 1)
          break;

      case mxaProcs:
 
        {
        char *ptr;
        char *TokPtr;

        int   tmpI;

        /* FORMAT:  <PROCCOUNT>[:ppn=<NCOUNT>] */

        ptr = MUStrTok(Value,":",&TokPtr);

        /* -l procs=0 becomes -l procs=1*/
        if (strcmp(ptr, "0")==0)
          {
          strcpy(ptr, "1");
          }
        
        tmpI = (int)strtol(ptr,NULL,10);

        /* we don't want to call MReqSetAttr here... */
        /* procs overrides everything else */
        
        J->Req[0]->TaskRequestList[0] = tmpI;
        J->Req[0]->TaskRequestList[1] = tmpI;
        J->Req[0]->TaskRequestList[2] = 0;

        J->Req[0]->TaskCount = tmpI;

        J->Request.TC = tmpI;

        /* update job cache so that MJobGetPC will get correct number in
         * MJobBuildCL() */

        J->TotalProcCount = tmpI;

        ptr = MUStrTok(NULL,":",&TokPtr);

        if (ptr != NULL)
          J->Req[0]->TasksPerNode = (int)strtol(ptr,NULL,10);

        /* rebuild CL info since we've changed the resources */

        MJobBuildCL(J);

        bmset(&J->Flags,mjfProcsSpecified);
        bmset(&J->SpecFlags,mjfProcsSpecified); /* make setting this flag persistent */
        }

        break;

      case mxaPrologueScript:
      case mxaEpilogueScript:

        if (aindex == mxaPrologueScript)
          {
          MUStrDup(&J->PrologueScript,Value);
          }
        else
          {
          MUStrDup(&J->EpilogueScript,Value);
          }

        break;

      case mxaProcsBitmap:

        {
        J->Req[0]->NAccessPolicy = mnacSingleJob;
        } /* END BLOCK (case mxaProcsBitmap) */

        break;  

      case mxaQueueJob:

        /* get QUEUEJOB */

        if (MUBoolFromString(Value,TRUE) == FALSE)
          {
          bmset(&J->SpecFlags,mjfNoQueue);

          MJobUpdateFlags(J);

          MDB(4,fSTRUCT) MLog("INFO:     NoQueue flag set for job %s\n",
            J->Name);
          }

        break;

      case mxaQOS:

        /* Don't load QOS if its been set by a job template */

        if (bmisset(&J->IFlags,mjifQOSSetByTemplate))
          {
          MDB(3,fCONFIG) MLog("INFO:     QOS '%s' requested by job %s rejected, due to previously set job template.\n",
            Value,
            J->Name);

          break;
          }

        /* load requested QOS */

        if (MQOSFind(Value,&J->QOSRequested) == SUCCESS)
          {
          MDB(3,fCONFIG) MLog("INFO:     QOS '%s' requested by job %s\n",
            J->QOSRequested->Name,
            J->Name);
          }
        else
          {
          char tmpLine[MMAX_LINE];

          MDB(3,fCONFIG) MLog("WARNING:  job %s cannot request QOS '%s'\n",
            J->Name,
            Value);
     
          J->QOSRequested = NULL;

          snprintf(tmpLine,sizeof(tmpLine),"job requests invalid QoS '%s'",
            Value);

          MQOSReject(J,mhBatch,MSched.DeferTime,tmpLine);

          /* Can't return failure here because it will cause the job to removed,
           * must follow qosreject policy. If policy is to cancel, the job
           * will canceled later in MRMWorkLoadQuery. */
          }

        break;

      case mxaReqAttr:

        MReqAttrFromString(&RQ->ReqAttr,Value);

        break;

      case mxaResFailPolicy:

        J->ResFailPolicy = (enum MAllocResFailurePolicyEnum)MUGetIndexCI(
          Value,
          MARFPolicy,
          FALSE,
          marfNONE);

        MDB(5,fCONFIG) MLog("INFO:     resource failure policy '%s' requested by job %s %s\n",
          Value,
          J->Name,
          (J->ResFailPolicy == marfNONE) ? " (request is invalid)" : "");

        break;

      case mxaRMType:

        /* NYI */

        break;

      case mxaSelect:

        MJobGetSelectPBSTaskList(J,Value,NULL,FALSE,FALSE,NULL);

        break;

      case mxaSGE:

        /* get SGE request */

        /* FORMAT:  SGE=<WINDOWCOUNT>:<DISPLAY> */

        /* add requirement */

        if (MReqCreate(J,NULL,&tmpRQPtr,FALSE) == FAILURE)
          {
          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);

          sprintf(EMsg,"cannot create req for SGE");

          return(FAILURE);
          }

        index = MUMAGetIndex(meGRes,"SGE",mAdd);

        MSNLSetCount(&tmpRQPtr->DRes.GenericRes,index,(unsigned int)strtol(Value,&tail,10));

        tmpRQPtr->TaskCount = 1;

        /* process display name */

        /* NYI */

        break;

      case mxaSID:

        /* set System ID */

        MJobSetAttr(J,mjaSID,(void **)Value,mdfString,mSet);

        MDB(3,fCONFIG) MLog("INFO:     system ID '%s' set for job %s\n",
          (J->SystemID != NULL) ? J->SystemID : "NULL",
          J->Name);

        if (J->ExtensionData != NULL)
          {
          /* note:  set extension job attribute */

          /* NYI */
          }

        break;

      case mxaSignal:

        {
        char  *sptr;
        char  *optr;

        char   TrigString[MMAX_LINE];

        char  *TokPtr;

        mreqsignal_t  Signal;
        mreqsignal_t *tmpSignal;

        mbool_t AddSignal = TRUE;

        int sindex;

        mtrig_t *T;

        /* FORMAT:  <SIGNAL>[@<OFFSET>] */

        sptr = MUStrTok(Value,"@",&TokPtr);
        optr = MUStrTok(NULL,"@",&TokPtr);

        if (MUSignalFromString(sptr,&Signal.Signo) == FAILURE)
          {
          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);

          snprintf(EMsg,MMAX_LINE,"cannot parse signal request '%s'",
            sptr);

          return(FAILURE);
          }

        if (optr != NULL)
          Signal.TimeOffset = MUTimeFromString(optr);
        else
          Signal.TimeOffset = MCONST_MINUTELEN;

        /* create pre-completion trigger */

        if (MJobCheckAllocTX(J) == FAILURE)
          {
          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);

          snprintf(EMsg,MMAX_LINE,"could not create signal structure");

          return(FAILURE);
          }

        for (sindex = 0;sindex < J->TemplateExtensions->Signals.NumItems;sindex++)
          {
          tmpSignal = (mreqsignal_t *)MUArrayListGet(&J->TemplateExtensions->Signals,0);

          if (tmpSignal == NULL)
            continue;

          if ((tmpSignal->TimeOffset == Signal.TimeOffset) &&
              (tmpSignal->Signo == Signal.Signo))
            {
            AddSignal = FALSE;

            break;
            }
          }
        
        if (AddSignal == TRUE) 
          {
          marray_t OTList;

          MUArrayListCreate(&OTList,sizeof(mtrig_t *),10);

          MUArrayListAppend(&J->TemplateExtensions->Signals,&Signal);

          snprintf(TrigString,sizeof(TrigString),"atype=internal,action=job:-:signal:%s,etype=end,offset=-%lu,name=termsignal",
            sptr,
            Signal.TimeOffset);

          MTrigLoadString(
            &J->Triggers,
            (char *)TrigString,
            TRUE,
            FALSE,
            mxoJob,
            J->Name,
            &OTList,
            NULL);

          for (tindex = 0;tindex < OTList.NumItems;tindex++)
            {
            T = (mtrig_t *)MUArrayListGetPtr(&OTList,tindex);

            if (MTrigIsValid(T) == FALSE)
              continue;

            MTrigInitialize(T);
            }   /* END for (tindex) */

          if (MJOBISACTIVE(J))
            {
            MOReportEvent((void *)J,J->Name,mxoJob,mttEnd,J->StartTime + J->SpecWCLimit[0],TRUE);
            }
          } /* END if (AddSignal == TRUE) */
        }  /* END BLOCK (case mxaSignal) */

        break;

      case mxaSize:

        MJobSetAttr(J,mjaSize,(void **)Value,mdfString,mSet);

        break;

      case mxaSJID:

        /* set system job ID */

        MJobSetAttr(J,mjaGJID,(void **)Value,mdfString,mSet);

        MDB(3,fCONFIG) MLog("INFO:     system JID '%s' set for job %s\n",
          (J->SystemJID != NULL) ? J->SystemJID : "NULL",
          J->Name);

        break;

      case mxaSPrio:

        {
        mbitmap_t  AuthBM;

        /* set job system priority */

        /* only admins may submit jobs with sys priority set */

        if ((J->Credential.U == NULL) || 
            (MSysGetAuth(J->Credential.U->Name,mcsMJobCtl,0,&AuthBM) == FAILURE))
          break;

        J->SystemPrio = strtol(Value,NULL,10);
              
        MDB(3,fCONFIG) MLog("INFO:     system priority '%ld' set for job %s\n",
          J->SystemPrio,
          J->Name);
        }  /* END (case mxaSPrio) */

        break;

      case mxaSRMJID:

        /* set source resource manager job ID */

        MJobSetAttr(J,mjaSRMJID,(void **)Value,mdfString,mSet);

        MDB(3,fCONFIG) MLog("INFO:     source RMJID '%s' set for job %s\n",
          (J->SRMJID != NULL) ? J->SRMJID : "NULL",
          J->Name);

        break;

      case mxaMoabStageIn:
      case mxaStageIn:

        /* If this gets requested multiple times, it looks like this function
         * will cause a memory leak */

        /* ignore this extension string if a storage RM doesn't exist */

        if (MSched.DefStorageRM != NULL)
          {
          if (J->DataStage == NULL)
            MDSCreate(&J->DataStage);

          if (MSDParseURL(Value,&J->DataStage->SIData,mdstStageIn,EMsg) == FAILURE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"WARNING:  failure parsing STAGEIN extension (%s)",
              EMsg);

            MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

            /* should routine fail? */
            }

          bmset(&J->IFlags,mjifDataDependency);
          }

        break;

      case mxaMoabStageOut:
      case mxaStageOut:

        /* ignore this extension string if a storage RM doesn't exist */

        if (MSched.DefStorageRM != NULL)
          {
          if (J->DataStage == NULL)
            MDSCreate(&J->DataStage);

          if (MSDParseURL(Value,&J->DataStage->SOData,mdstStageOut,EMsg) == FAILURE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"WARNING:  failure parsing STAGEOUT extension (%s)",
              EMsg);

            MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
            }
          }

        break;

      case mxaMPPmem:
      case mxaMPPhost:
      case mxaMPParch:
      case mxaMPPwidth:
      case mxaMPPnppn:
      case mxaMPPdepth:
      case mxaMPPnodes:
      case mxaMPPlabels:
        
        MPBSSetMPPJobAttr(J,MRMXAttr[aindex],Value,RQ);

        break;

      case mxaPostStageAction:

        /* ignore this extension string if a storage RM doesn't exist */

        if (MSched.DefStorageRM != NULL)
          {
          if (J->DataStage == NULL)
            MDSCreate(&J->DataStage);

          MUStrDup(&J->DataStage->PostStagePath,Value); /* Save off the path to the script */
          }

        break;

      case mxaPostStageAfterType:

        /* ignore this extension string if a storage RM doesn't exist */

        if (MSched.DefStorageRM != NULL)
          {
          if (J->DataStage == NULL)
            MDSCreate(&J->DataStage);

          /* Save the file type to do the post stage action after staging */

          bmfromstring(Value,MPostDSFile,&J->DataStage->PostStageAfterType);
          }

        break;

      case mxaStdErr:

        MUStrDup(&J->Env.Error,Value);

        break;

      case mxaStdOut:

        MUStrDup(&J->Env.Output,Value);

        break;

      case mxaTaskDistPolicy:

        /* task distribution policy */

        MJobCheckAllocTX(J);

        if (!strncasecmp(Value,"blocking:",strlen("blocking:")))
          {
          int Offset;

          RQ->TasksPerNode = (int)strtol(Value + strlen("blocking:"),NULL,10);

          bmset(&J->IFlags,mjifMaxTPN);

          Offset = RQ->TaskCount % MAX(RQ->TasksPerNode,1);

          RQ->NodeCount = RQ->TaskCount / RQ->TasksPerNode;

          if (Offset > 1)
            RQ->NodeCount++;

          J->TemplateExtensions->BlockingTaskDistPolicySpecified = TRUE;

          break;
          }

        J->TemplateExtensions->BlockingTaskDistPolicySpecified = FALSE;

        J->SpecDistPolicy = (enum MTaskDistEnum)MUGetIndexCI(
          Value,
          MTaskDistributionPolicy,
          FALSE,
          J->SpecDistPolicy);

        break;

      case mxaTemplate:

        /* apply job 'set' template */

        {
        MJobCheckAllocTX(J);
        MUStrDup(&J->TemplateExtensions->SpecTemplates,Value);

        break;

        /* Applying templates is now handled in MJobProcessRestrictedAttrs */

#if 0
        if (bmisset(&J->Flags,mjfTemplatesApplied))
          {
          /* Job's templates have already been applied */

          break;
          }

        if (Value[0] == '^')
          {
          bmset(&J->IFlags,mjifNoTemplates);
          Value++;
          }
        
        TokPtr2 = Value;
        strncpy(tmpLine,Value,sizeof(tmpLine));

        MJobCheckAllocTX(J);

        /* race condition, for template dependencies to function correctly the job group must
           be known at this point */

        while ((ptr = MUStrTok(TokPtr2,":,",&TokPtr2)) != NULL)
          {
          if ((MTJobFind(ptr,&SJ) == SUCCESS) && 
              (SJ->TX != NULL) && 
              (bmisset(&SJ->TX->TemplateFlags,mtjfTemplateIsSelectable)))
            {
            MJobApplySetTemplate(J,SJ,NULL);
            }
          else
            {
            char errMsg[MMAX_LINE];

            snprintf(errMsg,sizeof(errMsg),"WARNING:  cannot apply template '%s'",
              ptr);

            MMBAdd(&J->MB,errMsg,NULL,mmbtNONE,0,0,NULL);
            }

          if (!J->TX->ReqSetTemplates.empty())
            MStringAppend(&J->TX->ReqSetTemplates,",");

          MStringAppend(&J->TX->ReqSetTemplates,ptr);
          }

        bmset(&J->Flags,mjfTemplatesApplied);
        bmset(&J->SysFlags,mjfTemplatesApplied);
#endif
        }    /* END BLOCK (case mxaTemplate) */

        break;

      case mxaTermSig:

        MUStrDup(&J->TermSig,Value);

        break;

      case mxaTermTime:

        /* job termination time */

        MJobSetAttr(J,mjaTermTime,(void **)Value,mdfString,mSet);

        break;

      case mxaTID:

        /* ignore - process after all attributes have been processed */

        break;

      case mxaTPN:

        {
        char *tail;

        /* get TaskPerNode, FORMAT:  <X>[+|-] */
 
        RQ->TasksPerNode = (int)strtol(Value,&tail,0);

        MDB(5,fCONFIG) MLog("INFO:     tasks per node set to %d\n",
          RQ->TasksPerNode);

        if (tail == NULL)
          bmset(&J->IFlags,mjifExactTPN);
        else if (*tail == '-')
          bmset(&J->IFlags,mjifMaxTPN);
        else if (*tail != '+')
          bmset(&J->IFlags,mjifExactTPN);
        }  /* END BLOCK */

        break;

      case mxaTrig:

        /* NOTE:  at this point, duplicate string in J->TX->SpecTrig[].  Later,
                  process in MRMJobPostLoad() after QOS is loaded */

        {
        TokPtr2 = Value;
        tindex = 0;

        if (J->TemplateExtensions == NULL)
          {
          MJobAllocTX(J);

          if (J->TemplateExtensions == NULL)
            {
            /* FAILURE - out of memory */

            break;
            }
          }

        /* NOTE:  tokenizing on ':' can fragment trigger spec */

        while ((ptr = MUStrScan(TokPtr2,"%",&StrLen,(char const **)&TokPtr2)) != NULL)
          {
          while ((tindex < MMAX_SPECTRIG) && (J->TemplateExtensions->SpecTrig[tindex] != NULL))
            tindex++;

          if (tindex == MMAX_SPECTRIG)
            {
            /* FAILURE - buffer full */

            break;
            }

          MUStrNDup(&J->TemplateExtensions->SpecTrig[tindex],ptr,StrLen);
          }
        }    /* END BLOCK (case mxaTrig) */

        break;

      case mxaTrigNamespace:

        {
        /* Namespaces are VC names.  Make sure each VC exists */

        char *VCPtr;
        char *VCTokPtr;
        char tmpVCLine[MMAX_LINE << 2];

        MUStrCpy(tmpVCLine,Value,sizeof(tmpVCLine));

        VCPtr = MUStrTok(tmpVCLine,",",&VCTokPtr);

        if (MSched.Iteration > 0)
          {
          while (VCPtr != NULL)
            {
            if (MVCFind(VCPtr,NULL) == FAILURE)
              {
              /* VC not found - invalid namespace */

              snprintf(EMsg,MMAX_LINE,"invalid trigger namespace '%s'\n",
                VCPtr);

              return(FAILURE);
              }

            VCPtr = MUStrTok(NULL,",",&VCTokPtr);
            }  /* END while (VCPtr != NULL) */
          }  /* END if (MSched.Iteration > 0) */

        if (MJobSetDynamicAttr(J,mjaTrigNamespace,(void **)Value,mdfString) == FAILURE)
          {
          return(FAILURE);
          }
        }  /* END BLOCK (case mxaTrigNamespace) */

        break;

      case mxaTRL:

        /* TRL has no effect on active jobs */

        if (MJOBISACTIVE(J) == TRUE)
          break;          

        {
        char *ptr;
        char *TokPtr;

        char *wptr;
        char *TokPtr2;

        /* load task request list */

        /* FORMAT:  TRL{:|=}<TASKCOUNT>[@<WCLIMIT>][{:|,}<TASKCOUNT>[@<WCLIMIT>]]... */

        MUStrCpy(tmpLine,Value,sizeof(tmpLine));

        if (strchr(tmpLine,'-') == NULL)
          {
          ptr = MUStrTok(tmpLine,":,",&TokPtr);

          tindex = 1;
 
          while (ptr != NULL)
            {
            if (tindex >= MMAX_SHAPE)
              {
              MMBAdd(&J->MessageBuffer,"invalid trl requested",NULL,mmbtNONE,0,0,NULL);

              return(FAILURE);
              }

            /* split taskcount, walltime */

            ptr  = MUStrTok(ptr,"@",&TokPtr2);
            wptr = MUStrTok(NULL,"@",&TokPtr2);
  
            RQ->TaskRequestList[tindex] = (int)strtol(ptr,NULL,10);

            /* if no wclimit was specified set it to the default (SpecWCLimit[0]) */

            if (wptr != NULL)
              J->SpecWCLimit[tindex] = strtol(wptr,NULL,10);
            else
              J->SpecWCLimit[tindex] = J->SpecWCLimit[0];

            tindex++;
 
            ptr = MUStrTok(NULL,":,",&TokPtr);
            }  /* END while (ptr != NULL) */
 
          RQ->TaskRequestList[0] = RQ->TaskRequestList[1];
 
          RQ->TaskRequestList[tindex] = 0;

          /* Don't set TC on job if active because it could be different on a restart */

          if (MJOBISACTIVE(J) == FALSE)
            J->Request.TC = RQ->TaskRequestList[0];

          MDB(3,fCONFIG) MLog("INFO:     task request list loaded (%d,%d,...)\n",
            RQ->TaskRequestList[1],
            RQ->TaskRequestList[2]);

          bmset(&J->IFlags,mjifTRLRequest);
          }  /* END if (strchr(tmpLine,'-') == NULL) */
        else
          {
          int Start;
          int End;
          int Offset;

          /* FORMAT:  <TCSTART>-<TCEND> */

          ptr = MUStrTok(tmpLine,"-",&TokPtr);

          Start = (int)strtol(ptr,NULL,10);

          End = (int)strtol(TokPtr,NULL,10);

          Offset = Start + ((End - Start) / 4);

          RQ->TaskRequestList[0] = Start;
          RQ->TaskRequestList[1] = Start;

          index = 1;

          while (Start + (Offset * index) < End)
            {
            RQ->TaskRequestList[index + 1] = Start + (Offset * index);
            J->SpecWCLimit[index + 1]      = J->SpecWCLimit[0];

            index++;
            }

          RQ->TaskRequestList[index + 1] = End;
          J->SpecWCLimit[index + 1 ]     = J->SpecWCLimit[0];
          RQ->TaskRequestList[index + 2] = 0;

          /* Don't set TC on job if active because it could be different on a restart */

          if (MJOBISACTIVE(J) == FALSE)
            J->Request.TC = RQ->TaskRequestList[0];

          MDB(3,fCONFIG) MLog("INFO:     task request list loaded (%d-%d)\n",
            RQ->TaskRequestList[1],
            RQ->TaskRequestList[2]);

          bmset(&J->IFlags,mjifTRLRequest);
          }
        }  /* END case mxaTRL */

        break;

      case mxaUMask:

        MJobSetAttr(J,mjaUMask,(void **)Value,mdfString,mSet);

        break;

      case mxaVariables:

        for (index = 0;Value[index] != '\0';index++)
          {
          if (Value[index] == ':')
            {
            Value[index] = '=';
            break;
            }
          }

        /* Only set vars that aren't coming from CP file (unless we are using a CP journal).
         * This is important for triggers
         * who initially need a var set (via submission), but later this var is removed for
         * various reasons. We we re-load from the CP file, this var is set regardless of
         * previous unsets. This flag helps overcome this problem, but is also kind of a hack. (JB) */

        if (!bmisset(&J->IFlags,mjifWasLoadedFromCP) ||
            ((MCP.UseCPJournal == TRUE) && (MSched.HadCleanStart == FALSE)))
          {
          MJobSetAttr(J,mjaVariables,(void **)Value,mdfString,mSet);
          }
 
        break;

      case mxaVC:

        {
        mvc_t *VC = NULL;
        mln_t *tmpVCL = NULL;
        mvc_t *tmpVC = NULL;

        if (MVCFind(Value,&VC) == FAILURE)
          {
          MDB(5,fSTRUCT) MLog("ERROR:    VC '%s' does not exist\n",
            Value);

          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);

          return(FAILURE);
          }

        if (MVCUserHasAccess(VC,J->Credential.U) == FALSE)
          {
          MDB(5,fSTRUCT) MLog("ERROR:    job user '%s' does not have access to VC '%s'\n",
            (J->Credential.U != NULL) ? J->Credential.U->Name : "",
            VC->Name);

          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);

          return(FAILURE);
          }

        if (bmisset(&VC->Flags,mvcfDeleting) == TRUE)
          {
          MDB(5,fSTRUCT) MLog("ERROR:    VC '%s' is being deleted, cannot add job '%s'\n",
            VC->Name,
            J->Name);

          MUArrayListFree(&XVal);
          MUArrayListFree(&XAttr);

          return(FAILURE);
          }

        /* If job has a workflow VC, add the workflow VC instead
            of adding the job directly */

        for (tmpVCL = J->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
          {
          tmpVC = (mvc_t *)tmpVCL->Ptr;

          if ((tmpVC == NULL) ||
              (tmpVC == VC) ||
              (!bmisset(&tmpVC->Flags,mvcfWorkflow)) ||
              (tmpVC->CreateJob == NULL) ||
              (strcmp(tmpVC->CreateJob,J->Name)))
            {
            tmpVC = NULL;

            continue;
            }

          /* Found the workflow VC */

          MVCAddObject(VC,(void *)tmpVC,mxoxVC);

          if (MVCIsContainerHold(VC))
            {
            /* For a workflow, put a hold on all jobs in the workflow */

            mln_t *tmpL;

            for (tmpL = tmpVC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
              {
              mjob_t *tmpJ = (mjob_t *)tmpL->Ptr;

              if (tmpJ == NULL)
                continue;

              MJobSetHold(tmpJ,mhUser,MSched.DeferTime,mhrCredAccess,"Hold set by VC");
              }
            }  /* END if (bmisset(&tmpVC->Flags,mvcfHoldJobs)) */

          break;
          }  /* END for (tmpVCL = J->ParentVCs;) */

        if (tmpVC == NULL)
          {
          /* Job is not part of a workflow */

          MVCAddObject(VC,(void *)J,mxoJob);

          if (MVCIsContainerHold(VC))
            {
            /* VC sets holds on jobs, set a user hold so that it can be released */

            MJobSetHold(J,mhUser,MSched.DeferTime,mhrCredAccess,"Hold set by VC");
            }
          }  /* END if (tmpVC == NULL) */
        }  /* END CASE mxaVC */

        break;

      case mxaVMUsage:

        MJobSetAttr(J,mjaVMUsagePolicy,(void **)Value,mdfString,mSet);

        MReqSetImage(J,J->Req[0],J->Req[0]->Opsys);
 
        break;

      case mxaVPC:

        {
        mrsv_t *R;
        mpar_t *VPC;

        if (MVPCFind(Value,&VPC,TRUE) == FAILURE)
          {
          MDB(3,fCONFIG) MLog("WARNING:  could not locate VPC '%s' for job '%s'\n",
            Value,
            J->Name);

          break;
          }

        if (MRsvFind(VPC->RsvGroup,&R,mraNONE) == FAILURE)
          {
          MDB(3,fCONFIG) MLog("WARNING:  no valid rsv for VPC '%s' requested by job '%s'\n",
            Value,
            J->Name);

          break;
          }

        MUStrDup(&J->ReqVPC,Value);

        if (R->RsvGroup != NULL)
          {
          MUStrDup(&J->ReqRID,R->RsvGroup);

          bmset(&J->SpecFlags,mjfAdvRsv);
          bmset(&J->Flags,mjfAdvRsv);
          }
        }  /* END case mxaVPC */

        break;

      case mxaWCKey:

        {
        char  tmpLine[MMAX_LINE];

        /* store the wckey as a variable job attribute */

        snprintf(tmpLine,sizeof(tmpLine),"wckey=%s",
          Value);

        MJobSetAttr(J,mjaVariables,(void **)tmpLine,mdfString,mSet);
        }

        break;

      case mxaWCRequeue:

        {
        mbool_t Val = MUBoolFromString(Value,FALSE);

        if (Val == TRUE)
          bmset(&J->IFlags,mjifWCRequeue);
        }

        break;

      default:

        /* not handled */

        break;
      }  /* END switch (aindex) */

    bmset(&J->RMXSetAttrs,aindex);
    }    /* END while (key != NULL) */

  MUArrayListFree(&XVal);
  MUArrayListFree(&XAttr);

  return(SUCCESS);
  }  /* END MJobProcessExtensionString() */





/**
 * Render a request's feature bitmap  and excluded feature bitmap to a
 * heap-allocated string.
 *
 * @param RQ (I)
 */

int MRMXFeaturesToString(

  mreq_t *RQ,
  char   *String)

  {
  char tmpLine[MMAX_LINE];

  if ((RQ == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  String[0] = '\0';

  mstring_t FeaturesList(MMAX_LINE);

  MUNodeFeaturesToString((RQ->ReqFMode == mclOR) ? '|' : ',',&RQ->ReqFBM,tmpLine);

  if (!MUStrIsEmpty(tmpLine))
    {
    MStringAppend(&FeaturesList,tmpLine);
    }

  /* excluded features */

  MUNodeFeaturesToString((RQ->ExclFMode == mclAND) ? '&' : ',',&RQ->ExclFBM,tmpLine);

  if (!MUStrIsEmpty(tmpLine))
    {
    char *ptr;
    char* TokPtr = NULL;
    ptr = MUStrTok(tmpLine,"&,|",&TokPtr);
    MStringAppend(&FeaturesList,(strcmp(FeaturesList.c_str(),"") ? (char *)",!" : (char *)"!"));
    MStringAppend(&FeaturesList,ptr);

    while((ptr = MUStrTok(NULL,"&,",&TokPtr)))
      {
      MStringAppend(&FeaturesList,(RQ->ExclFMode == mclAND ? (char *)"&!" : (char *)",!"));
      MStringAppend(&FeaturesList,ptr);
      }
    }

  MUStrCpy(String,FeaturesList.c_str(),MMAX_LINE);

  return(SUCCESS);
  } /* END MRMXFeaturesToString */




/**
 * render an mpref_t object to String
 *
 * @param Pref   (I)
 * @param Out    (O)
 */

int MRMXPrefToString(

  mpref_t   *Pref,
  mstring_t *Out)

  {
  char *ptr;
  char *TokPtr = NULL;

  char  Prefs[MMAX_BUFFER];
  char  Vars[MMAX_BUFFER];

  Prefs[0] = '\0';
  Vars[0] = '\0';

  if ((Pref == NULL) || (Out == NULL))
    {
    return(FAILURE);
    }

  MStringSet(Out,"\0");

  MUNodeFeaturesToString(',',&Pref->FeatureBM,Prefs);

  MULLToString(Pref->Variables,FALSE,",",Vars,sizeof(Vars));

  if ((MUStrIsEmpty(Prefs)) && (MUStrIsEmpty(Vars)))
    return(SUCCESS);

  MStringAppendF(Out,"PREF(");

  ptr = MUStrTok(Prefs,",",&TokPtr);

  while (!MUStrIsEmpty(ptr))
    {
    MStringAppendF(Out,"FEATURE:%s,",ptr);
    ptr = MUStrTok(NULL,",",&TokPtr);
    }

  ptr = MUStrTok(Vars,",",&TokPtr);

  while (!MUStrIsEmpty(ptr))
    {
    MStringAppendF(Out,"VARIABLE:%s,",ptr);
    ptr = MUStrTok(NULL,",",&TokPtr);
    }
    
  MStringAppendF(Out,")");

  return(SUCCESS);
  } /* END MRMXPrefToString() */

/* END MJRMX.c */
