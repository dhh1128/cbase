/* HEADER */

      
/**
 * @file MUIRsv.c
 *
 * Contains: MUI Rsv functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


int __MUIRsvProfShow(msocket_t *,mgcred_t *,char *,mxml_t **,char *);



#define MMAX_COMPLETERSVCOUNT     256

/**
 * process mdiag -r request in a threadsafe way.
 *
 * @see MUIRsvDiagnose (sibling)
 *
 * @param S       (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth    (I) authorization credentials for request
 */

int MUIRsvDiagnoseXML(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  rsv_iter RTI;

  char DiagOpt[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char RsvID[MMAX_LINE];

  mxml_t *DE;
  mxml_t *RE;

  mrsv_t *R;

  mgcred_t *U;

  mbitmap_t CFlags;  /* bitmap of enum MCModeEnum */

  const enum MRsvAttrEnum RAList[] = {
    mraName,
    mraACL,       /**< @see also mraCL */
    mraAAccount,
    mraAGroup,
    mraAUser,
    mraAQOS,
    mraAllocNodeCount,
    mraAllocNodeList,
    mraAllocProcCount,
    mraAllocTaskCount,
    mraCL,        /**< credential list */
    mraComment,
    mraCost,      /**< rsv AM lien/charge */
    mraCTime,     /**< creation time */
    mraDuration,
    mraEndTime,
    mraExcludeRsv,
    mraExpireTime,
    mraFlags,
    mraGlobalID,
    mraHostExp,
    mraHistory,
    mraLabel,
    mraLastChargeTime, /* time rsv charge was last flushed */
    mraLogLevel,
    mraMessages,
    mraOwner,
    mraPartition,
    mraPriority,
    mraProfile,
    mraReqArch,
    mraReqFeatureList,
    mraReqMemory,
    mraReqNodeCount,
    mraReqNodeList,
    mraReqOS,
    mraReqTaskCount,
    mraReqTPN,
    mraResources,
    mraRsvAccessList, /* list of rsv's and rsv groups which can be accessed */
    mraRsvGroup,
    mraRsvParent,
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

  RsvID[0] = '\0';

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    MCacheJobLock(FALSE,TRUE);
    return(FAILURE);
    }

  DE = S->SDE;

  /* find the user who issued the request */

  MUserAdd(Auth,&U);

  /* get the argument flags for the request */

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  /* look for a specified resveration id */

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    MUStrCpy(RsvID,DiagOpt,sizeof(RsvID));
    }

  /* loop through all reservations and create an xml element for each, adding
   * it to the return data element */

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R))
    {
    if ((!MUStrIsEmpty(RsvID)) && (strcmp(RsvID,R->Name) != 0))
      continue;

    if (MUICheckAuthorization(
          U,
          NULL,
          (void *)R,
          mxoRsv,
          mcsDiagnose,
          mrcmQuery,
          NULL,
          NULL,
          0) == FAILURE)
      {
      /* no authority to diagnose reservation */

      continue;
      } /* END if (MUICheckAuthorization...) */

    MDB(6,fUI) MLog("INFO:     evaluating MRsv '%s'\n",
      R->Name);

    RE = NULL;

    MXMLCreateE(&RE,(char *)MXO[mxoRsv]);

    MXMLAddE(DE,RE);

    MRsvToXML(R,&RE,(enum MRsvAttrEnum *)RAList,NULL,TRUE,mcmNONE);
    }  /* END for (rindex) */

  return(SUCCESS);
  } /* END MUIRsvDiagnoseXML() */




/**
 *
 *
 * @param S        (I)
 * @param U        (I)
 * @param Name     (I)
 * @param RDE      (O)
 * @param TypeList (I)
 */

int __MUIRsvProfShow(

  msocket_t *S,
  mgcred_t  *U,
  char      *Name,
  mxml_t   **RDE,
  char      *TypeList)

  {
  mxml_t *DE;
  mxml_t *CE;

  int rindex;

  mrsv_t *R;

  mbool_t IsAdmin = FALSE;

  enum MRsvAttrEnum RAList[] = {
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
    mraExpireTime,
    mraFlags,
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

  const char *FName = "__MUIRsvProfShow";

  MDB(2,fUI) MLog("%s(S,ReqE,RDE,%s)\n",
    FName,
    (TypeList != NULL) ? TypeList : "NULL");

  if ((S == NULL) || (RDE == NULL))
    {
    return(FAILURE);
    }

  if (*RDE == NULL)
    {
    MUISAddData(S,"ERROR:    internal failure\n");

    return(FAILURE);
    }

  DE = *RDE;

  /* add rsv profiles */

  for (rindex = 0;rindex < MMAX_RSVPROFILE;rindex++)
    {
    R = MRsvProf[rindex];

    if (R == NULL)
      continue;

    if ((Name != NULL) && 
        (Name[0] != '\0') &&
        (strcasecmp(Name,"all")) &&
        (strcmp(Name,R->Name)))
      {
      continue;
      }

    if (MUICheckAuthorization(
          U,
          NULL,
          (void *)R,
          mxoRsv,
          mcsMRsvCtl,
          mrcmQuery,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      continue;
      }

    CE = NULL;

    if (MRsvToXML(R,&CE,RAList,NULL,FALSE,mcmNONE) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");

      return(FAILURE);
      }

    MXMLAddE(DE,CE);
    }  /* END for (rindex) */

  return(SUCCESS);
  }  /* END __MUIRsvProfShow() */




/**
 * Diagnose and report status of reservation table.
 *
 * NOTE: Handles 'mdiag -r' and 'mrsvctl -q'
 *
 * @see MSRDiag() - peer - diagnose standing reservation
 * @see MRsvShowState() - child
 * @see MRsvDiagnoseState() - child
 * @see MUIDiagnose() - parent
 *
 * @param AFlagBM (I) [enum MRoleEnum bitmap]
 * @param Auth (I)
 * @param SR (I) [optional]
 * @param String (I) [must be initalized]
 * @param P (I)
 * @param DiagOpt (I)
 * @param DFormat (I)
 * @param Flags (I) [bitmap on enum MClientModeEnum]
 * @param ResType (I) [mrtNONE means do not filter on type]
 */

int MUIRsvDiagnose(

  mbitmap_t *AFlagBM,
  char      *Auth,
  mrsv_t    *SR,
  mstring_t *String,
  mpar_t    *P,
  char      *DiagOpt,
  enum MFormatModeEnum DFormat,
  mbitmap_t *Flags,
  enum MRsvTypeEnum ResType)

  {
  rsv_iter RTI;

  int nindex;

  mnode_t *N;
  mjob_t  *J;

  int   TotalRC;
  int   TotalRsvStatePC;
  int   TotalRsvAPC;

  mrsv_t *R;

  mxml_t *DE = NULL;
  mxml_t *RE = NULL;

  mbool_t IsAdmin;

  mgcred_t *U;

  const char *FName = "MUIRsvDiagnose";

  MDB(3,fUI) MLog("%s(Auth,String,%s,%s,%s,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    DiagOpt,
    MFormatMode[DFormat],
    MRsvType[ResType]);

  if (String == NULL)
    {
    return(FAILURE);
    }

  /* check all reservations */

  MDB(4,fUI) MLog("INFO:     diagnosing rsv table (%d slots)\n",
    MSched.M[mxoRsv]);

  if (DFormat == mfmXML)
    {
    /* NOTE:  do not wrap specific rsv requests in Data element */

    if (SR == NULL)
      {
      DE = NULL;

      MXMLCreateE(&DE,(char *)MSON[msonData]);
      }
    }
  else
    {
    /* create header */

    MStringAppend(String,"Diagnosing Reservations\n");

    MRsvShowState(
      NULL,
      Flags,
      NULL,
      String,
      DFormat);
    }

  /* initialize statistics */

  TotalRC = 0;

  TotalRsvAPC     = 0;
  TotalRsvStatePC = 0;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if (N->State == mnsReserved)
      TotalRsvStatePC += N->CRes.Procs;
    }  /* END for (nindex) */

  if (bmisset(AFlagBM,mcalAdmin1) ||
      bmisset(AFlagBM,mcalAdmin2) ||
      bmisset(AFlagBM,mcalAdmin3))
    IsAdmin = TRUE;
  else
    IsAdmin = FALSE;

  MUserAdd(Auth,&U);

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((SR != NULL) && (R != SR))
      continue;

    if ((P != NULL) && (P->Index > 0) && (P->Index != R->PtIndex))
      continue;

    if (strcmp(DiagOpt,NONE) && strcmp(DiagOpt,R->Name))
      {
      if ((R->Label == NULL) || strcmp(DiagOpt,R->Label)) 
        continue;
      }

    /* If a ResType is given (-w type=ResType) and it is not equal to this reservation's type, don't print */

    if ((ResType != mrtNONE) && (R->Type != ResType))
      continue;

    if (!bmisset(AFlagBM,mcalOwner) && (IsAdmin == FALSE))
      { 
      if (MUICheckAuthorization(
            U,
            NULL,
            (void *)R,
            mxoRsv,
            mcsDiagnose,
            mrcmQuery,
            NULL,
            NULL,
            0) == FAILURE)
        {
        /* no authority to diagnose reservation */

        continue;
        }
      }

    MDB(6,fUI) MLog("INFO:     evaluating Rsv '%s'\n",
      R->Name);

    if (DFormat == mfmXML)
      {
      RE = NULL;

      MXMLCreateE(&RE,(char *)MXO[mxoRsv]);

      if (SR == NULL)
        MXMLAddE(DE,RE);

      MRsvShowState(
        R,
        Flags,
        RE,  /* O */
        NULL,
        DFormat);
      }
    else
      {
      MRsvShowState(
        R,
        Flags,
        NULL, 
        String,
        DFormat);

      MRsvDiagnoseState(
        R,
        Flags,
        NULL,
        String,
        DFormat);
      }  /* END else (DFormat == mfmXML) */

    TotalRC++;

    if ((R->J != NULL) && ((R->Type == mrtJob) || (R->Type == mrtUser)))
      {
      /* determine allocated jobs associated with system jobs (user rsv) or regular jobs */

      J = R->J;

      if (MJOBISACTIVE(J))
        TotalRsvAPC += R->AllocPC;
      }

    if (SR != NULL)
      break;
    }  /* END while (MRsvTableIterate()) */

  if ((!strcmp(DiagOpt,NONE)) && !bmisset(AFlagBM,mcalOwner))
    {
    /* set global diagnostic messages */

    if (DFormat != mfmXML)
      {
      char *BPtr;
      int   BSpace;

      char MsgBuf[MMAX_BUFFER];

      MUSNInit(&BPtr,&BSpace,MsgBuf,sizeof(MsgBuf));

      MStringAppendF(String,"\nActive Reserved Processors: %d\n",
        TotalRsvAPC);

      if ((TotalRsvAPC + TotalRsvStatePC) != MPar[0].DRes.Procs)
        {
        MStringAppendF(String,"INFO:  active procs reserved does not equal active procs utilized (%d != %d)\n",
          TotalRsvAPC + TotalRsvStatePC,
          MPar[0].DRes.Procs);
        }

      MRsvDiagGrid(BPtr,BSpace,0);

      MStringAppend(String,MsgBuf);
      }  /* END if (DFormat != mfmXML) */
    }    /* END if (!strcmp(DiagOpt,NONE) */

  if (DFormat == mfmXML)
    {
    /* make into XML encoded string */

    if (MXMLToMString(
        (SR == NULL) ? DE : RE,
        String,
        NULL,
        TRUE) == FAILURE)
      {
      MDB(2,fUI) MLog("ALERT:    could not convert XML to string in %s\n",
        FName);
      }

    MXMLDestroyE(
      (SR == NULL) ? &DE : &RE);
    }  /* END if (DFormat == mfmXML) */

  return(SUCCESS);
  }  /* END MUIRsvDiagnose() */







/**
 * Report reservation status (process 'showres' request)
 *
 * @see MUIRsvCtl() - control/modify/create reservations.
 *
 * @see MNodeShowRsv() - child - process 'showres -n'
 * @see MUIRsvList() - child - process 'showres'
 *
 * @param RBuffer (I)
 * @param Buffer (O)
 * @param CFlags (I)
 * @param Auth (I)
 * @param BufSize (I)
 */

int MUIShowRes(

  msocket_t *S,      /* I */
  mbitmap_t *CFlags, /* I (not used) */
  char      *Auth)   /* I */

  {
  enum MXMLOTypeEnum ObjectType = mxoNONE;

  char Name[MMAX_NAME];
  char tmpName[MMAX_NAME];
  char tmpVal[MMAX_LINE];
  char FlagLine[MMAX_LINE];
  char EMsg[MMAX_LINE] = {0};

  mxml_t *RE = NULL;
  mxml_t *DE;

  mpar_t *P = NULL;

  mbitmap_t  Flags;

  const char *FName = "MUIShowRes";

  MDB(3,fUI) MLog("%s(S,CFlags,%s)\n",
    FName,
    Auth);

  /* need to get ObjectType, PName, Flags, Name */

  switch (S->WireProtocol)
    {
    case mwpXML:
    case mwpS32:

      {
      mxml_t *WE;

      int WTok;
   
      /* FORMAT:  <Request action="X" object="Y"><Where/></Request> */

      char *ptr;

      if (S->RDE != NULL)
        {
        RE = S->RDE;
        }
      else if ((S->RPtr != NULL) &&
              ((ptr = strchr(S->RPtr,'<')) != NULL) &&
               (MXMLFromString(&RE,ptr,NULL,NULL) == FAILURE))
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%100.100s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      WTok = -1;

      while (MS3GetWhere(
          RE,
          &WE,
          &WTok,
          tmpName,          /* O */
          sizeof(tmpName),
          tmpVal,           /* O */
          sizeof(tmpVal)) == SUCCESS)
        {
        if (!strcasecmp(tmpName,"partition"))
          {
          if (MParFind(tmpVal,&P) == FAILURE)
            {
            snprintf(EMsg,sizeof(EMsg),"ERROR:  invalid partition \"%s\" specified\n",
              tmpVal);

            MUISAddData(S,EMsg);

            return(FAILURE);
            }
          }
        else if (!strcmp(tmpName,MSON[msonObject]))
          {
          ObjectType = (enum MXMLOTypeEnum)MUGetIndexCI(tmpVal,MXOC,FALSE,mxoNONE);
          }
        }       /* END while (MXMLGetChild() == SUCCESS) */

      if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagLine,sizeof(FlagLine)) == SUCCESS)
        {
        bmfromstring(FlagLine,MClientMode,&Flags);

        if (strcasestr(FlagLine,"showfree") != NULL)
          {
          bmset(&Flags,mcmNonBlock);
          }
        }

      if (MXMLGetAttr(RE,MSAN[msanOp],NULL,Name,sizeof(Name)) == FAILURE)
        {
        MUStrCpy(Name,NONE,sizeof(Name));
        }
      }  /* END BLOCK (case mwpXML/mwpS3) */

      break;

    default:

      /* not supported */

      MUISAddData(S,"ERROR:    corrupt command received\n");

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (S->WireProtocol) */

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  DE = S->SDE;

  switch (ObjectType)
    {
    case mxoNode:

      if (MNodeShowRsv(Auth,Name,P,&Flags,DE,EMsg) == FAILURE)
        {
        MUISAddData(S,EMsg);

        return(FAILURE);
        }

      break;

    case mxoJob:

      if (MUIRsvList(Auth,Name,P,&Flags,DE,EMsg) == FAILURE)
        {
        MUISAddData(S,EMsg);

        return(FAILURE);
        }

      break;

    default:

      MDB(0,fUI) MLog("ERROR:    reservation type '%d' not handled\n",
        ObjectType);

      snprintf(EMsg,sizeof(EMsg),"ERROR:    reservation type '%d' not handled\n",
        ObjectType);

      MUISAddData(S,EMsg);

      return(FAILURE);

      break;
    }  /* END switch (ObjectType) */

  return(SUCCESS);
  }  /* END MUIShowRes() */





/**
 * Process client request for showres.
 *
 * @see MUIRsvShow() - parent
 *
 * @param Auth (I) [optional]
 * @param RID (I) [optional]
 * @param P (I) [optional]
 * @param Flags (I) [optional,bitmap of enum MCModeEnum]
 * @param SBuffer (O) [minsize=*SBufSize or MMAX_BUFFER]
 * @param SBufSize (I)
 *
 * NOTE:  Enhance to allow specification of target rsvgroup (NYI) 
 */

int MUIRsvList(

  char      *Auth,
  char      *RID,
  mpar_t    *P,
  mbitmap_t *Flags,
  mxml_t    *DE,
  char      *EMsg)

  {
  mxml_t *RE;

  const enum MRsvAttrEnum RAList[] = {
    mraName,
    mraType,
    mraStartTime,
    mraEndTime,
    mraAllocNodeCount,
    mraAllocProcCount,
    mraCTime,
    mraNONE };
 
  int  rqindex;

  int  count;

  mjob_t *J;
  mreq_t *RQ;

  mrsv_t *R;
  mrsv_t *SR;   /* specified rsv */
  
  mgcred_t *U;
  char tmpLine[MMAX_LINE];
 
  const char *FName = "MUIRsvList";

  enum MSysAttrEnum SAList[] = {
    msysaPresentTime,
    msysaNONE };

  enum MXMLOTypeEnum SCList[] = {
    mxoNONE };

  MDB(2,fUI) MLog("%s(Auth,%s,%s,SBuffer,SBufSize)\n",
    FName,
    RID,
    (P != NULL) ? P->Name : "NULL");

  if ((DE == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  EMsg[0] = '\0';

  /* uses global MJob, MRsv tables */

  count = 0;

  if (MUserFind(Auth,&U) == FAILURE)
    {
    snprintf(EMsg,MMAX_LINE,"INFO:  no reservations located for requestor\n");

    return(SUCCESS);
    }

  if ((RID != NULL) && (RID[0] != '\0') && strcmp(RID,NONE))
    {
    if (MRsvFind(RID,&SR,mraNONE) == FAILURE)
      {
      snprintf(EMsg,MMAX_LINE,"\ncannot locate specified reservation '%s'\n",
        RID);

      return(FAILURE);
      }
    }
  else
    {
    SR = NULL;
    }

  if (MSysToXML(&MSched,&RE,SAList,SCList,TRUE,0) == SUCCESS)
    {
    MXMLAddE(DE,RE);
    }

  /* add job reservations */

  if (MJobHT.size() > 0)
    {
    job_iter JTI;

    MJobIterInit(&JTI);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if ((P != NULL) && 
          (P->Index > 0) && 
          (bmisset(&J->PAL,P->Index) == FAILURE))
        continue;

      R = J->Rsv;

      if (R == NULL)
        {
        MDB(5,fUI) MLog("INFO:     job '%s' has no reservation\n",
          J->Name);

        continue;
        }

      if (SR != NULL)
        {
        if (bmisset(Flags,mcmOverlap))
          {
          if (MRsvHasOverlap(SR,R,TRUE,&SR->NL,NULL,NULL) == FALSE)
            continue;
          }
        else if (R != SR)
          {
          continue;
          }
        }

      if (MUICheckAuthorization(
           U,
           NULL,
           (void *)R,
           mxoRsv,
           mcsRsvShow,
           mrcmList,
           NULL,
           tmpLine,
           sizeof(tmpLine)) == FAILURE)
        {
        MDB(6,fUI) MLog("%s",tmpLine);

        continue;
        }  /* END if (MUICheckAuthorization() */

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        RE = NULL;

        RQ = J->Req[rqindex];

        if (RQ == NULL)
          break;

        R = RQ->R;

        if (R == NULL)
          continue;

        if (MRsvToXML(R,&RE,(enum MRsvAttrEnum *)RAList,NULL,TRUE,mcmNONE) == SUCCESS)
          {
          MXMLAddE(DE,RE);
          }

        count++;
        }  /* END for (rqindex) */

      MDB(5,fUI) MLog("INFO:     job '%s' added to reservation list\n",
        J->Name);
      }    /* END for (J = MJob[0]->Next) */
    }    /* END if (MJob[0] != NULL) */

  if (MRsvHT.size() > 0)
    {
    rsv_iter RTI;

    /* add non-job reservations */
   
    MRsvIterInit(&RTI);
   
    while (MRsvTableIterate(&RTI,&R) == SUCCESS)
      {
      if ((P != NULL) && (P->Index > 0) && (R->PtIndex != P->Index))
        continue;
   
      /* meta reservations belong to meta jobs, already printed above */
   
      if ((R->Type == mrtJob) || (R->Type == mrtMeta))
        continue;
   
      if (SR != NULL)
        {
        if (bmisset(Flags,mcmOverlap))
          {
          if (MRsvHasOverlap(SR,R,TRUE,&SR->NL,NULL,NULL) == FALSE)
            continue;
          }
        else if (R != SR)
          {
          continue;
          }
        }
   
      /* authority check */
   
      if (MUICheckAuthorization(
           U,
           NULL,
           (void *)R,
           mxoRsv,
           mcsRsvShow,
           mrcmList,
           NULL,
           tmpLine,
           sizeof(tmpLine)) == FAILURE)
        {
        MDB(6,fUI) MLog("%s",tmpLine);
   
        continue;
        }  /* END if (MUICheckAuthorization() == FAILURE) */
   
      RE = NULL;

      /* display non-job reservation */
   
      if (MRsvToXML(R,&RE,(enum MRsvAttrEnum *)RAList,NULL,TRUE,mcmNONE) == SUCCESS)
        {
        MXMLAddE(DE,RE);
        }

      count++;
   
      MDB(5,fUI) MLog("INFO:     reservation '%s' added to reservation list\n",
        R->Name);
      }  /* END while (MRsvTableIterate()) */
    }    /* END if (MRsvHT.size() > 0) */

  return(SUCCESS);
  }  /* END MUIRsvList() */

/* END MUIRsv.c */
