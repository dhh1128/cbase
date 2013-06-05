/* HEADER */

/**
 * @file MUIDiagnose.c
 *
 * Contains MUI Diagnose function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report credential state/diagnostics.
 *
 * handle mdiag -u 
 *        mdiag -c
 *        mdiag -g
 *        mdiag -q
 *        mdiag -a
 *
 * @see MUIDiagnose() - parent
 * @see __MUICredCtlQuery() - parent
 * @see MAcctShow() - child
 * @see MUIShowCConfig() - child
 *
 * @param Auth   (I) (FIXME: not used but should be)
 * @param ReqE   (I) [optional] 
 * @param Buffer (O)
 * @param OIndex (I)
 * @param OID    (I) [optional]
 * @param IFlags (I) [bitmap of enum MCModeEnum]
 */

int MUICredDiagnose(

  char                *Auth,
  mxml_t              *ReqE,
  mstring_t           *Buffer,
  enum MXMLOTypeEnum   OIndex,
  char                *OID,
  mbool_t              Verbose)

  {
  void   *O;
  void   *OP;

  void   *OE;

  char   *NameP;
  char   Partition[MMAX_LINE];

  int   (*FPtr)(mgcred_t *,mstring_t *,mbool_t,char *);

  enum MXMLOTypeEnum FilterOType = mxoNONE;

  void   *FO = NULL;  /* filter object */

  mcredl_t *L = NULL;

  macl_t   *RCL = NULL;

  mhashiter_t HTI;

  char    tmpSVal[MMAX_LINE];
  char    tmpDVal[MMAX_LINE];

  int     CTok;

  const char *FName = "MUICredDiagnose";

  MDB(3,fUI) MLog("%s(ReqE,Buffer,BufSize,%s,%s)\n",
    FName,
    MXO[OIndex],
    (OID != NULL) ? OID : "NULL");

  if ((Buffer == NULL) || (OIndex == mxoNONE))
    {
    return(FAILURE);
    }

  MStringAppendF(Buffer,"evaluating %s information\n",
    MXO[OIndex]);

  /* create header */

  FPtr = NULL;

  switch (OIndex)
    {
    case mxoUser:

      FPtr = MUserShow;

      break;

    case mxoGroup:

      FPtr = MGroupShow;

      break;

    case mxoAcct:

      FPtr = MAcctShow;

      break;

    default:

      /* NOT HANDLED */

      break;
    }  /* END switch (OIndex) */

  if (FPtr == NULL)
    {
    return(FAILURE);
    }

  /* process all constraints */

  CTok = -1;

  while (MS3GetWhere(
      ReqE,
      NULL,
      &CTok,
      tmpSVal,         /* O */
      sizeof(tmpSVal),
      tmpDVal,         /* O */
      sizeof(tmpDVal)) == SUCCESS)
    {
    FilterOType = (enum MXMLOTypeEnum)MUGetIndexCI(tmpSVal,MXO,FALSE,0);

    switch (FilterOType)
      {
      case mxoUser:
      case mxoGroup:
      case mxoAcct:
      case mxoClass:
      case mxoQOS:

        if (MOGetObject(FilterOType,tmpDVal,(void **)&FO,mVerify) == FAILURE)
          {
          MDB(5,fUI) MLog("INFO:     cannot locate %s %s for constraint in %s\n",
            MXO[FilterOType],
            tmpDVal,
            FName);

          return(FAILURE);
          }

        if (MOGetComponent(FO,FilterOType,(void **)&L,mxoxLimits) == FAILURE)
          {
          return(FAILURE);
          }

        if ((L->JobMinimums[0] == NULL) || (L->JobMinimums[0]->RequiredCredList == NULL))
          {
          return(FAILURE);
          }

        RCL = L->JobMinimums[0]->RequiredCredList;

        break;

      default:

        /* constraint ignored */

        break;
      }  /* END switch (OIndex) */
    }    /* END while () */

  MXMLGetAttr(ReqE,"partition",NULL,Partition,sizeof(Partition));

  if ((!MUStrIsEmpty(Partition)) && (MUStrNCmpCI(Partition,"ALL",sizeof("ALL")) == 0))
    {
    MStringAppendF(Buffer,"Filtered by partition %s\n",
      Partition);
    }

  /* create header */

  (*FPtr)(NULL,Buffer,Verbose,NULL);

  /* display credential information */

  MOINITLOOP(&OP,OIndex,&OE,&HTI);

  while ((O = MOGetNextObject(&OP,OIndex,OE,&HTI,&NameP)) != NULL)
    {
    if (!strcmp(NameP,ALL) || !strcmp(NameP,"NOGROUP"))
      continue;

    if ((!MUStrIsEmpty(OID)) && strcmp(OID,NameP))
      continue;

    if (RCL != NULL)
      {
      macl_t *tACL;

      /* look through MemberUList */

      for (tACL = RCL;tACL != NULL;tACL = tACL->Next)
        {
        if (tACL->Type != maUser)
          continue;

        if (!strcmp(tACL->Name,NameP))
          break;
        }  /* END for (tACL) */

      if (tACL == NULL)
        {
        /* object not found in membership list */

        continue;
        }
      }    /* END if (RCL != NULL) */

    MDB(8,fUI) MLog("INFO:     checking %s '%s'\n",
      MXO[OIndex],
      NameP);

    MXMLGetAttr(ReqE,"partition",NULL,Partition,sizeof(Partition));

    (*FPtr)((mgcred_t *)O,Buffer,Verbose,Partition);
    }  /* END while (O) */

  return(SUCCESS);
  }  /* END MUICredDiagnose() */



/**
 * Return object in XML format.
 *
 * @see MUIDiagnose - parent
 *
 * @param Auth  (I) (FIXME: not used but should be)
 * @param OType (I)
 * @param OSpec (I)
 * @param RE    (O) must already be initialized
 */

int MUIGCredToXML(

  char              *Auth,
  enum MXMLOTypeEnum OType,
  char              *OSpec,
  mxml_t            *RE)

  {
  mgcred_t *Cred = NULL;
  mxml_t *CE = NULL;
  mhashiter_t HIter;

  enum MCredAttrEnum AList[] = {
    mcaID,
    mcaADef,
    mcaAList,
    mcaCDef,
    mcaCSpecific,
    mcaDefWCLimit,
    mcaEMailAddress,
    mcaEnableProfiling,
    mcaFlags,
    mcaFSCap,
    mcaFSTarget,
    mcaHold,
    mcaIsDeleted,
    mcaJobFlags,
    mcaMaxIWC,
    mcaMinWCLimitPerJob,
    mcaManagers,
    mcaMaxWCLimitPerJob,
    mcaMaxJob,
    mcaMaxNode,
    mcaMaxPE,
    mcaMaxProc,
    mcaMaxPS,
    mcaPriority,
    mcaQList,
    mcaQDef,
    mcaPList,
    mcaPDef,
    mcaOverrun,
    mcaReqRID,
    mcaRole,
    mcaNONE };

  if ((MUStrIsEmpty(Auth)) || (RE == NULL))
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoUser:

      if (MUStrIsEmpty(OSpec))
        {
        /* show all users */

        MUHTIterInit(&HIter);
        while (MUHTIterate(&MUserHT,NULL,(void **)&Cred,NULL,&HIter) == SUCCESS)
          {
          MCOToXML(
              (void *)Cred,
              OType,
              &CE,
              AList,
              NULL,
              NULL,
              TRUE,
              0);

          MXMLAddE(RE,CE);

          CE = NULL;
          }
        }
      else
        {
        MUserFind(OSpec,&Cred);

        MCOToXML(
            (void *)Cred,
            OType,
            &CE,
            AList,
            NULL,
            NULL,
            TRUE,
            0);

        MXMLAddE(RE,CE);

        CE = NULL;
        }

      break;

    case mxoGroup:

      if (MUStrIsEmpty(OSpec))
        {
        /* show all Groups */

        MUHTIterInit(&HIter);
        while (MUHTIterate(&MGroupHT,NULL,(void **)&Cred,NULL,&HIter) == SUCCESS)
          {
          MCOToXML(
              (void *)Cred,
              OType,
              &CE,
              AList,
              NULL,
              NULL,
              TRUE,
              0);

          MXMLAddE(RE,CE);

          CE = NULL;
          }
        }
      else
        {
        MGroupFind(OSpec,&Cred);

        MCOToXML(
            (void *)Cred,
            OType,
            &CE,
            AList,
            NULL,
            NULL,
            TRUE,
            0);

        MXMLAddE(RE,CE);

        CE = NULL;
        }

      break;

    case mxoAcct:

      if (MUStrIsEmpty(OSpec))
        {
        /* show all users */

        MUHTIterInit(&HIter);
        while (MUHTIterate(&MAcctHT,NULL,(void **)&Cred,NULL,&HIter) == SUCCESS)
          {
          MAcctToXML(Cred,&CE,AList);

          MXMLAddE(RE,CE);

          CE = NULL;
          }
        }
      else
        {
        MAcctFind(OSpec,&Cred);

        MAcctToXML(Cred,&CE,AList);

        MXMLAddE(RE,CE);

        CE = NULL;
        }

      break;

    default:

      return(FAILURE);

      break;
    }

  return(SUCCESS);
  }  /* END MUICredToXML() */


/**
 * Handle all mdiag requests.
 *
 * @see MCDiagnoseCreateRequest() - client routine to create request
 * @see MCDiagnose() - client routine to report diagnose results 
 * @see MQOSShow() - child - report QoS state ('mdiag -q')
 * @see MClassShow() - child - report class/queue state ('mdiag -c')
 * @see MUIShowStats() - peer - process 'showstats' request
 * @see MUIJobDiagnose() - child - report queue state ('mdiag -j')
 * @see MUINodeDiagnose() - child - report node state ('mdiag -n')
 * @see MUIRsvDiagnose() - child - report rsv state ('mdiag -r')
 * @see MUICredDiagnose() - child - report cred state 
 * @see MRMShow() - child - report RM state
 * @see MTrigDiagnose() - child - report trigger state
 *
 * @param S (I)
 * @param CFlags (I) [bitmap of enum MRoleEnum]
 * @param Auth (I)
 */

int MUIDiagnose(

  msocket_t *S,      /* I */
  mbitmap_t *CFlags, /* I (bitmap of enum MRoleEnum) */
  char      *Auth)   /* I */

  {
  mpsi_t *Peer = NULL;
  mgcred_t *U = NULL;

  int  WTok;

  char OString[MMAX_NAME];

  enum MXMLOTypeEnum OType;
  enum MXMLOTypeEnum ObjectType = mxoNONE;

  char tmpLine[MMAX_LINE];
  char tmpName[MMAX_NAME];
  char OID[MMAX_NAME];
  char tmpVal[MMAX_LINE];

  char OFilterID[MMAX_LINE];
  enum MXMLOTypeEnum OFilterType = mxoNONE;

  char FlagString[MMAX_NAME];
  char AccessString[MMAX_LINE];
  char PolicyLevel[MMAX_NAME];

  mpar_t *P;

  char PName[MMAX_NAME];
  char DiagOpt[MMAX_NAME];

  mbitmap_t IFlags;  /* bitmap of enum MCModeEnum */
  mbitmap_t AuthBM;   /* bitmap of enum MRoleEnum */

  enum MFormatModeEnum DFormat;
  mbool_t Verbose = FALSE;

  mxml_t *ReqE;
  mxml_t *WE;

  const char *FName = "MUIDiagnose";

  MDB(2,fUI) MLog("%s(S,%s)\n",
    FName,
    Auth);

  if (S == NULL)
    {
    return(FAILURE);
    }

  AccessString[0] = '\0';
  OID[0]          = '\0';
  PolicyLevel[0]  = '\0';
  PName[0]        = '\0';
  OFilterID[0]    = '\0';
  DiagOpt[0]      = '\0';

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    if (MPeerFind(Auth,FALSE,&Peer,FALSE) == FAILURE)
      {
      return(FAILURE);
      }      
     
    U = NULL;
    }
  else
    {
    MUserAdd(Auth,&U);
     
    Peer = NULL;
    }

  /* FORMAT:  XML -> A:object, A:partition, A:op, C:where, A:policy, A:flags */

  ReqE = S->RDE;

  MS3GetObject(ReqE,OString);

  /* OType is the object type to diagnose */
  /* OFilterType is the object filter, ie, diagnose jobs (OType) where user=X (OFilterType) */

  OType = (enum MXMLOTypeEnum)MUGetIndexCI(OString,MXO,FALSE,mxoNONE);

  MXMLGetAttr(ReqE,"partition",NULL,PName,sizeof(PName));
  MXMLGetAttr(ReqE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString));
  MXMLGetAttr(ReqE,"policy",NULL,PolicyLevel,sizeof(PolicyLevel));
  MXMLGetAttr(ReqE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt));

  if (!MUStrIsEmpty(DiagOpt))
    {
    if (strchr(DiagOpt,':') != NULL)
      {
      char tmpLine[MMAX_LINE];

      char *ptr;
      char *TokPtr = NULL;

      /* FORMAT:  [<OTYPE>:]OID */

      MUStrCpy(tmpLine,DiagOpt,sizeof(tmpLine));

      if ((ptr = MUStrTok(tmpLine,":",&TokPtr)) != NULL)
        {
        ObjectType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);
        }

      if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
        {
        MUStrCpy(OID,ptr,sizeof(OID));
        }
      }    /* END else if (strchr(DiagOpt,':') != NULL) */
    else
      {
      MUStrCpy(OID,DiagOpt,sizeof(OID));

      /* ObjectType = OType; */
      }
    }  /* END if (!MUStrIsEmpty(DiagOpt)) */

  WTok = -1;

  while (MS3GetWhere(
        ReqE,
        &WE,
        &WTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcasecmp(tmpName,"access"))
      {
      MUStrCpy(AccessString,tmpVal,sizeof(AccessString));
      }
    else if (!strcasecmp(tmpName,"otype"))
      {
      /* what is this otype?  how is it set? */

      ObjectType = (enum MXMLOTypeEnum)MUGetIndexCI(tmpVal,MXO,FALSE,mxoNONE);
      }
    else if (!strcasecmp(tmpName,"oid"))
      {
      MUStrCpy(OID,tmpVal,sizeof(OID));
      }
    else if (!strncasecmp(tmpName,"par",strlen("par")))
      {
      MUStrCpy(PName,tmpVal,sizeof(PName));
      }
    else if (!strcasecmp(tmpName,"class"))
      {
      OFilterType = mxoClass;

      MUStrCpy(OFilterID,tmpVal,sizeof(OFilterID));
      }
    else if (!strcasecmp(tmpName,"jobspec"))
      {
      OFilterType = mxoxTJob;

      MUStrCpy(OFilterID,tmpVal,sizeof(OFilterID));
      }
    }    /* END while (MS3GetWhere() == SUCCESS) */

  /* NOTE:  diagnose object type 'ObjectType' with optional ID 'OID' */

  if (OFilterID[0] == '\0')
    {
    /* allow object managers to diagnose their own objects */

    /* ie, class manager for batch can run 'mdiag -c batch' */

    if (OID[0] != '\0')
      {
      MUStrCpy(OFilterID,OID,sizeof(OFilterID));
 
      OFilterType = OType;
      }
    }

  /* check permissions */

  MSysGetAuth(Auth,mcsDiagnose,0,&AuthBM);

  if (MUICheckAuthorization(U,Peer,NULL,OType,mcsDiagnose,0,NULL,NULL,0) == SUCCESS)
    {
    bmset(&AuthBM,mcalGranted);
    }

  if ((!bmisset(&AuthBM,mcalGranted)) && (OFilterID[0] != '\0'))
    {
    switch (OFilterType)
      {
      case mxoAcct:

        {
        mgcred_t *A;

        if ((MAcctFind(OFilterID,&A) == SUCCESS) &&
            (A->F.ManagerU != NULL) &&
            (MCredFindManager(Auth,A->F.ManagerU,NULL) == SUCCESS))
          {
          switch (OType)
            {
            case mxoAcct:

              bmset(&AuthBM,mcalGranted);

              break;

            default:

              /* no access granted */

              break;
            }
          }
        }    /* END BLOCK (case mxoAcct) */

        break;

      case mxoClass:

        {
        mclass_t *C;

        if ((MClassFind(OFilterID,&C) == SUCCESS) &&
            (C->F.ManagerU != NULL) &&
            (MCredFindManager(Auth,C->F.ManagerU,NULL) == SUCCESS))
          {
          /* requestor is queue manager */

          switch (OType)
            {
            case mxoJob:

              /* NOTE:  by default, queue manager is manager of queue jobs */

              MS3AddWhere(ReqE,(char *)MXO[mxoClass],C->Name,NULL);

              bmset(&AuthBM,mcalOwner);

              break;

            case mxoTrig:

              MS3AddWhere(ReqE,(char *)MXO[mxoClass],C->Name,NULL);

              bmset(&AuthBM,mcalOwner);

              break;

            case mxoClass:
 
              bmset(&AuthBM,mcalGranted);

              break;

            default:

              /* NO ACCESS ALLOWED */

              break;
            }
          }
        }    /* END BLOCK (case mxoClass) */

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OFilterType) */
    }    /* END if ((!bmisset(&AuthBM,mcalGranted)) && (OID[0] != '\0')) */

  switch (OType)
    {
    case mxoSched:

      {
      mgcred_t *U;
      mpsi_t   *P;

      if (!strncasecmp(Auth,"peer:",strlen("peer:")))
        {
        if (MPeerFind(Auth,FALSE,&P,FALSE) == FAILURE)
          {
          /* NYI */
          }      
     
        U = NULL;
        }
      else
        {
        MUserAdd(Auth,&U);
     
        P = NULL;
        }

      if (!bmisset(&AuthBM,mcalOwner) && 
          !bmisset(&AuthBM,mcalGranted) &&
          (MUICheckAuthorization(U,P,NULL,mxoSched,mcsMSchedCtl,msctlQuery,NULL,NULL,0) == FAILURE))
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    user '%s' is not authorized to run command '%s'\n",
          Auth,
          MUI[mcsDiagnose].SName);
     
        MUISAddData(S,tmpLine);
     
        return(FAILURE);
        }
      }

      break;
 
    case mxoQueue:

      /* No Auth checking for "mdiag -b", this is handled at a lower level */

      /* NO-OP */

      break;

    default:

      if (!bmisset(&AuthBM,mcalOwner) && !bmisset(&AuthBM,mcalGranted))
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    user '%s' is not authorized to run command '%s'\n",
          Auth,
          MUI[mcsDiagnose].SName);
     
        MUISAddData(S,tmpLine);
     
        return(FAILURE);
        }

      break;
    }  /* END switch (OType) */

  bmfromstring(FlagString,MClientMode,&IFlags);

  if (bmisset(&IFlags,mcmXML))
    DFormat = mfmXML;
  else
    DFormat = mfmHuman;

  if (bmisset(&IFlags,mcmVerbose))
    Verbose = TRUE;

  if (MParFind(PName,&P) == FAILURE)
    {
    if ((PName[0] != '\0') && (strcmp(PName,ALL)) && (strcmp(PName,NONE)))
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid partition '%s' specified\n",
        PName);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }

    P = &MPar[0];
    }

  /* create SDE structure for functions that need it below */

  if ((DFormat == mfmXML) && (S->SDE == NULL))
    {
    if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    cannot create response\n");

      return(FAILURE);
      }
    }

  mstring_t Response(MMAX_LINE);

  switch (OType)
    {
    case mxoUser:
    case mxoGroup:
    case mxoAcct:

      /* handle 'mdiag -u' */
      /* handle 'mdiag -g' */
      /* handle 'mdiag -a' */

      if (DFormat == mfmXML)
        {
        MUIGCredToXML(
          Auth,
          OType,
          DiagOpt,
          S->SDE);
        }
      else
        {
        MUICredDiagnose(
          Auth,
          ReqE,
          &Response,
          OType,
          DiagOpt,
          Verbose);

        MUISAddData(S,Response.c_str());
        }

      break;

    case mxoClass:

      /* handle 'mdiag -c' */

      MClassShow(
        Auth,
        DiagOpt,
        (DFormat == mfmXML) ? S->SDE : NULL,
        (DFormat == mfmXML) ? NULL : &Response,
        &IFlags,
        FALSE,
        DFormat);

      if (DFormat != mfmXML)
        MUISAddData(S,Response.c_str());

      break;

    case mxoxFS:

      /* handle 'mdiag -f' */

      MFSShow(
        Auth,
        &Response,
        ObjectType,  /* I object type reported */
        P,           /* I partition */
        &IFlags,      /* I (hierarchical if mcmRelative set) */
        DFormat,
        Verbose);

      MUISAddData(S,Response.c_str());

      break;

    case mxoxGreen:

      /* handle 'mdiag -G' */

      MSysDiagGreen(Auth,&Response,&IFlags);

      MUISAddData(S,Response.c_str());

      break;

    case mxoJob:

      /* handle 'mdiag -j' */

      MUIJobDiagnose(
        Auth,
        (DFormat == mfmXML) ? S->SDE : NULL,
        (DFormat == mfmXML) ? NULL : &Response,
        P,
        DiagOpt,
        ReqE,           /* I */
        &IFlags);

      if (DFormat != mfmXML)
        MUISAddData(S,Response.c_str());

      break;

    case mxoxLimits:

      /* handle 'mdiag -L' */

      MUILimitDiagnose(Auth,&Response,&IFlags);

      MUISAddData(S,Response.c_str());

      break;

    case mxoNode:

      /* handle 'mdiag -n' */

      /* how are 'where' constraints routed in? */

      MUINodeDiagnose(
        Auth,
        ReqE,                /* I */
        OFilterType,
        OFilterID,
        (DFormat == mfmXML) ? &S->SDE : NULL,  /* O */
        (DFormat == mfmXML) ? NULL : &Response,
        P,
        AccessString,
        DiagOpt,
        &IFlags);

      if (DFormat != mfmXML)
        MUISAddData(S,Response.c_str());

      break;

    case mxoPar:

      /* handle 'mdiag -t' */

      MUIParDiagnose(
        Auth,
        NULL,
        &Response,
        DiagOpt,
        &IFlags,
        DFormat);

      MUISAddData(S,Response.c_str());

      break;

    case mxoxPriority:

      /* handle 'mdiag -p' */

      MUIPrioDiagnose(
        Auth,
        &Response,
        P,
        ReqE,
        &IFlags,
        DFormat);

      MUISAddData(S,Response.c_str());

      break;

    case mxoQOS:

      /* handle 'mdiag -q' */

      MQOSShow(Auth,DiagOpt,&Response,&IFlags,DFormat);

      MUISAddData(S,Response.c_str());

      break;

    case mxoQueue:

      {
      enum MPolicyTypeEnum Mode = (enum MPolicyTypeEnum)MUGetIndexCI(PolicyLevel,MPolicyMode,FALSE,mptDefault);

      /* handle 'mdiag -b' */

      /* NOTE:  pass PLevel via IFlags */
 
      MUIQueueDiagnose(
        Auth,
        MUIQ,
        Mode,
        P,
        &Response);

      MUISAddData(S,Response.c_str());
      }

      break;

    case mxoRsv:

      /* handle 'mdiag -r' */

      MUIRsvDiagnoseXML(S,CFlags,Auth);

      break;

    case mxoRM:

      /* handle 'mdiag -R' */

      MRMShow(
        Auth,
        DiagOpt,
        (DFormat == mfmXML) ? S->SDE : NULL,
        (DFormat == mfmXML) ? NULL : &Response,
        DFormat,
        &IFlags);  

      if (DFormat != mfmXML)
        MUISAddData(S,Response.c_str());

      break;

    case mxoSched:

      /* handle 'mdiag -S' */

      MSchedDiag(
        Auth,
        &MSched,
        (DFormat == mfmXML) ? S->SDE : NULL,
        (DFormat == mfmXML) ? NULL : &Response,
        DFormat,
        &IFlags);

      if (DFormat != mfmXML)
        MUISAddData(S,Response.c_str());

      break;

    case mxoSRsv:

      /* handle 'mdiag -s' */

      MSRDiagXML(S,CFlags,Auth);

      break;

    case mxoTrig:

      /* handle 'mdiag -T' */

      if (bmisset(&IFlags,mcmParse))
        {
        MTrigDisplayDependencies(Auth,&Response);
        }
      else
        {
        MTrigDiagnose(
          Auth,
          ReqE,
          (DFormat == mfmXML) ? S->SDE : NULL, /* O */
          (DFormat == mfmXML) ? NULL : &Response,
          DFormat,
          &IFlags,
          NULL,
          mSet);
        }

      if (DFormat != mfmXML)
        MUISAddData(S,Response.c_str());

      break;

    default:

      MDB(2,fUI) MLog("ALERT:    unexpected diag type, '%s'\n",
        MXO[OType]);

      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  unexpected diag type, '%s'\n",
        MXO[OType]);

      MUISAddData(S,tmpLine);

      S->SBufSize = strlen(S->SBuffer);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (OType) */

  if ((MSched.DisableUICompression != TRUE) && (strlen(S->SBuffer) > (MMAX_BUFFER >> 1)))
    {
    MSecCompress(
      (unsigned char *)S->SBuffer,
      strlen(S->SBuffer),
      NULL,
      (MSched.EnableEncryption == TRUE) ? (char *)MCONST_CKEY : NULL);
    }

  S->SBufSize = strlen(S->SBuffer);

  return(SUCCESS);
  }  /* END MUIDiagnose() */
/* END MUIDiagnose.c */
