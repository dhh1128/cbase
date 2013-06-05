/* HEADER */

/**
 * @file MUI.c
 *
 * Moab User Interface.
 *
 * The functions in this file are mainly for carrying out command-line requests.
 * Any time the user does an "mjobctl" for example, the functions in this file are used.
 */

#include "moab.h"
#include "moab-proto.h"  
#include "mcom-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/* prototypes */

int MUIMiscShow(msocket_t *,mxml_t *,mxml_t **,char *);

int MUIOShowAccess(void *,enum MXMLOTypeEnum,char *,mbool_t,int,mxtree_t *,mxml_t *);
int MUIExportRsvQuery(mrsv_t *,mrm_t *,char *,mxml_t *,mulong);

int __MUIXMLPerJobLimits(mxml_t *,mcredl_t *,int);
int __MUIXMLPolicyLimits(mxml_t *,mpu_t *,enum MLimitTypeEnum,int,char *);
int __MUIXMLActivePolicyLimits(mxml_t *,mcredl_t *,int);



/* END prototypes */



/**
 * Report miscellaneous object tables (node features, job features, etc):.
 *
 * @param S (I)
 * @param ReqE (I)
 * @param RDE (O)
 * @param TypeList (I)
 */

int MUIMiscShow(

  msocket_t *S,        /* I */
  mxml_t    *ReqE,     /* I */
  mxml_t   **RDE,      /* O */
  char      *TypeList) /* I */

  {
  mbitmap_t BM;

  mxml_t *DE;
  mxml_t *CE;

  char tmpLine[MMAX_LINE];

  const char *FName = "MUIMiscShow";

  MDB(2,fUI) MLog("%s(S,ReqE,RDE,%s)\n",
    FName,
    (TypeList != NULL) ? TypeList : "NULL");

  if ((S == NULL) || (ReqE == NULL) || (RDE == NULL))
    {
    return(FAILURE);
    }

  if (*RDE == NULL)
    {
    MUISAddData(S,"ERROR:    internal failure\n");

    return(FAILURE);
    }

  DE = *RDE;

  MXMLSetAttr(DE,"object",(void *)MXO[mxoxCP],mdfString);

  /* add experienced values */

  CE = NULL;

  if (MXMLCreateE(&CE,(char *)"EValues") == FAILURE)
    {
    MUISAddData(S,"ERROR:    internal failure\n");

    return(FAILURE);
    }

  MXMLAddE(DE,CE);

  /* display comma delimited list of detected node features */

  bmsetall(&BM,MMAX_ATTR);

  MUNodeFeaturesToString(',',&BM,tmpLine);

  if (tmpLine[0] != '\0')
    MXMLSetAttr(CE,"nodefeatures",(void *)&tmpLine,mdfString);

  /* display comma delimited list of detected job features */

  MUJobAttributesToString(&BM,tmpLine);

  if (tmpLine[0] != '\0')
    MXMLSetAttr(CE,"jobfeatures",(void *)&tmpLine,mdfString);

  return(SUCCESS);
  }  /* END MUIMiscShow() */






/**
 * Figure out the dependency type based on String and RmType. If no dependency 
 *   type can be inferred, the function Returns FAILURE and sets Out to DefaultValue
 *
 * @param String       (I)
 * @param RmType       (I)
 * @param DefaultValue (I)
 * @param Out          (O)
 */

int MJobParseDependType(

  char const       *String,          /* I */
  enum MRMTypeEnum  RmType,          /* I */
  enum MDependEnum  DefaultValue,    /* I */
  enum MDependEnum *Out)             /* O */

  {
  int rc = SUCCESS;

  enum MDependEnum DType = mdNONE;
  int ListIndex;

  void *DependTypeLists[] = {
    MDependType,       /* moab types */
    SDependType,       /* slurm types */
    TDependType,       /* torque types */
    TriggerDependType, /* trigger types */
    NULL };

  /* don't bother to check if in dependency list if the first character is numeric -
     if it is numeric then it is probably a job id or job name,
     so it will not be a depend type. */

  if (isalpha(*String))  
    {
    for (ListIndex = 0; DependTypeLists[ListIndex] != NULL; ListIndex++)
      {
      DType = (enum MDependEnum)MUGetIndexCI((char *)String,(const char **)DependTypeLists[ListIndex],FALSE,0);
   
      if (DType != mdNONE)
        {
        /* found a matching type so we are done */
   
        break;
        }
      }
    }    /* END for (ListIndex) */

  if (DType == mdNONE)
    {
    DType = DefaultValue;

    rc = FAILURE;
    }

  *Out = DType;

  return(rc);
  }  /* END MJobParseDependType() */








/**
 * Report credential statistics.
 *
 * @see MUIShowStats() - parent
 *
 * @param AuthBM (I) [bitmap of enum MRoleEnum]
 * @param ReqE (I)
 * @param CName (I)
 * @param CIndex (I)
 * @param FlagBM (I)
 * @param AName (I)
 * @param E (O)
 * @param STime (I)
 * @param ETime (I)
 * @param EMsg (O) [minsize=MMAX_LINE]
 */

int MUIShowCStats(

  mbitmap_t           *AuthBM,
  mxml_t              *ReqE,
  char                *CName,
  enum MXMLOTypeEnum   CIndex,
  mbitmap_t           *FlagBM,
  char                *AName,
  mxml_t              *E,
  long                 STime,
  long                 ETime, 
  char                *EMsg)

  {
  void   *O;
  void   *OP;

  void   *OE;

  char   *NameP;

  mxml_t *CE;

  int     CCount;

  mhashiter_t HTI;

  mgcred_t *U;
  
  mbitmap_t BM;

  char *BPtr;
  int   BSpace;

  char tmpLine[MMAX_LINE];
 
  const enum MCredAttrEnum CAList[] = {
    mcaID,
    mcaNONE };

  const enum MXMLOTypeEnum CCList[] = {
    mxoxStats,
    mxoxLimits,
    mxoxFS,
    mxoNONE };

  const enum MXMLOTypeEnum CCListIdle[] = {
    mxoxStats,
    mxoNONE };

  void *AList[1];

  const enum MStatAttrEnum CCAListIdle[] = {
    mstaTQueuedJC,
    mstaTQueuedPH,
    mstaNONE };

  const char *FName = "MUIShowCStats";

  MDB(2,fUI) MLog("%s(%ld,ReqE,'%s',%s,%d,AName,E,%ld,%ld,EMsg)\n",
    FName,
    AuthBM,
    CName,
    MXO[CIndex],
    FlagBM,
    STime,
    ETime);

  if (EMsg != NULL)
    {
    MUSNInit(&BPtr,&BSpace,EMsg,MMAX_LINE);
    }
  else
    {
    return(FAILURE);
    }

  if (E == NULL)
    {
    return(FAILURE);
    }

   if ((AName == NULL) || (AName[0] == '\0'))
     {
     return(FAILURE);
     }
  
  AList[0] = (void *)CCAListIdle;

  CCount = 0;

  if (MUserFind(AName,&U) == FAILURE)
    {
    return(FAILURE);
    }

  if ((CName != NULL) && (CName[0] != '\0') && strcmp(CName,NONE))
    {
    /* if CName specified */

    MDB(4,fUI) MLog("INFO:     searching for %s '%s'\n",
      MXO[CIndex],
      CName);

    if (MOGetObject(CIndex,CName,&O,mVerify) == FAILURE)
      {
      MDB(3,fUI) MLog("INFO:     cannot locate %s %s in Moab's historical data\n",
        MXO[CIndex],
        CName);

      MUSNPrintF(&BPtr,&BSpace,"cannot locate %s '%s' in Moab's historical data\n",
        MXO[CIndex],
        CName);

      return(FAILURE);
      }

    /* NOTE: authentication is already handled in MUIShowStats() */

    if (MUICheckAuthorization(
          U,
          NULL,
          (void *)O,
          CIndex,
          mcsStatShow,
          mccmQuery,
          NULL,
          tmpLine,
          sizeof(tmpLine)) == FAILURE)
      {
      MUSNCat(&BPtr,&BSpace,tmpLine);
  
      return(FAILURE);
      }
    }   /* END if ((CName != NULL) && ...) */

  /* display global statistics */

  MSchedStatToString(
    &MSched,
    mwpXML,
    E,
    0,
    TRUE);

  MOINITLOOP(&OP,CIndex,&OE,&HTI);

  bmset(&BM,mcmVerbose);

  while ((O = MOGetNextObject(&OP,CIndex,OE,&HTI,&NameP)) != NULL)
    {
    if ((CName != NULL) && (CName[0] != '\0') && strcmp(CName,NONE))
      {
      if (strcmp(CName,NameP))
        continue;
      }

    if (!strcmp(NameP,NONE) || !strcmp(NameP,ALL) || !strcmp(NameP,"DEFAULT"))
      continue;

    if (MUICheckAuthorization(
          U,
          NULL,
          (void *)O,
          CIndex,
          mcsStatShow,
          mccmQuery,
          NULL,
          tmpLine,
          sizeof(tmpLine)) == FAILURE)
      {
      MUSNCat(&BPtr,&BSpace,tmpLine);

      continue;
      }

    CE = NULL;

    MCOToXML(
      O,
      CIndex,
      &CE,
      (void *)CAList,
      (bmisset(FlagBM,mjstEligible)) ? 
         (enum MXMLOTypeEnum *)CCListIdle : 
         (enum MXMLOTypeEnum *)CCList,
      (bmisset(FlagBM,mjstEligible)) ? AList : NULL,
      TRUE,
      &BM);

    MXMLAddE(E,CE);

    CCount++;
    }  /* END while ((O = MOGetNextObject()) != NULL) */

  return(SUCCESS);
  }  /* END MUIShowCStats() */





/**
 * Report credential configuration.
 *
 * @see __MUICredCtlQuery() - parent
 *
 * @param CName (I)
 * @param CIndex (I)
 * @param Buf (I/O)
 * @param CEP (O) [optional]
 * @param LList (I)
 */

int MUIShowCConfig(

  char                 *CName,    /* I */
  enum MXMLOTypeEnum    CIndex,   /* I */
  mxml_t               *Buf,      /* I/O */
  mxml_t              **CEP,      /* O (optional) */
  enum MLimitAttrEnum  *LList)    /* I */

  {
  mbitmap_t BM;

  void   *O = NULL;
  void   *OP;

  void   *OE;

  char   *NameP;

  mxml_t *CE;
  mxml_t *DE;

  mhashiter_t HTI;

  char    tmpLine[MMAX_LINE];

  const enum MCredAttrEnum CAList[] = {
    mcaID,
    mcaADef,
    mcaAList,
    mcaCDef,
    mcaCSpecific,
    mcaDefWCLimit,
    mcaEMailAddress,
    mcaEnableProfiling,
/*  mcaFSCap,
    mcaFSTarget, */
    mcaHold,
    mcaIsDeleted,
    mcaJobFlags,
    mcaMaxIWC,
    mcaMinWCLimitPerJob,
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

  const enum MLimitAttrEnum LAList[] = {
    mlaHLAJobs,
    mlaHLAProcs,
    mlaHLAPS,
    mlaHLANodes,
    mlaSLAJobs,
    mlaSLAProcs,
    mlaSLAPS,
    mlaSLANodes,
    mlaNONE };

  const enum MFSAttrEnum FAList[] = {
    mfsaTarget,
    mfsaCap,
    mfsaNONE };

  void *CCAList[16];

  const enum MXMLOTypeEnum CCList[] = {
    mxoxLimits,
    mxoxFS, 
    mxoNONE };

  const char *FName = "MUIShowCConfig";

  MDB(2,fUI) MLog("%s('%s',%s,Buf,CEP)\n",
    FName,
    (CName != NULL) ? CName : "NULL",
    MXO[CIndex]);

  /* initialize */

  if (CEP != NULL)
    *CEP = NULL;

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  /* initialize data */

  CCAList[0] = (void *)((LList != NULL) ? LList : LAList);
  CCAList[1] = (void *)FAList;
  CCAList[2] = NULL;

  DE = (mxml_t *)Buf;

  if ((CName != NULL) && 
      (CName[0] != '\0') && 
      strcmp(CName,NONE))
    {
    /* if CName specified */

    MDB(4,fUI) MLog("INFO:     searching for %s '%s'\n",
      MXO[CIndex],
      CName);

    if (MOGetObject(CIndex,CName,&O,mVerify) == FAILURE)
      {
      MDB(3,fUI) MLog("INFO:     cannot locate %s %s in Moab's historical data\n",
        MXO[CIndex],
        CName);

      sprintf(tmpLine,"cannot locate %s '%s' in Moab's historical data\n",
        MXO[CIndex],
        CName);

      MXMLSetVal(DE,(void *)tmpLine,mdfString);

      return(FAILURE);
      }
    }   /* END if ((CName != NULL) && ...) */

  bmset(&BM,mcmVerbose);

  if ((CIndex == mxoQOS) && 
      (CName != NULL) && 
      (!strcmp(CName,"DEFAULT")))
    {
    /* display QOS default config */

    CE = NULL;

    MCOToXML(
      O,
      CIndex,
      &CE,
      (void *)CAList,
      (enum MXMLOTypeEnum *)CCList,
      CCAList,
      TRUE,
      &BM);

    MXMLAddE(DE,CE);

    if (CEP != NULL)
      *CEP = CE;
    }

  MOINITLOOP(&OP,CIndex,&OE,&HTI);

  while ((O = MOGetNextObject(&OP,CIndex,OE,&HTI,&NameP)) != NULL)
    {
    if ((CName != NULL) && 
        (CName[0] != '\0') && 
         strcmp(CName,NONE))
      {
      if (strcmp(CName,NameP))
        continue;
      }

    if ((NameP != NULL) &&
       (!strcmp(NameP,NONE) || 
        !strcmp(NameP,ALL)))
      {
      continue;
      }

    CE = NULL;

    MCOToXML(
      O,
      CIndex,
      &CE,
      (void *)CAList,
      (enum MXMLOTypeEnum *)CCList,
      CCAList,
      TRUE,
      &BM);

    MXMLAddE(DE,CE);

    if (CEP != NULL)
      *CEP = CE;
    }  /* END while ((O = MOGetNextObject()) != NULL) */

  return(SUCCESS);
  }  /* END MUIShowCConfig() */





/**
 * Reports the Moab scheduler's general configuration options. This
 * method is invoked by the "showconfig" command.
 *
 * @param Auth (I) [optional]
 * @param PIndex (I)
 * @param Vflag (I)
 * @param SBuffer (O)
 * @param BufSize (I)
 */

int MUIConfigShow(

  char *Auth,    /* I requestor (optional) */
  enum MCfgParamEnum PIndex,  /* I */
  int   Vflag,   /* I */
  char *SBuffer, /* O */
  int   BufSize) /* I */

  {
  mstring_t Response(MMAX_LINE);
  mstring_t tmpString(MMAX_LINE);

  int  pindex;
  int  qindex;
  int  cindex;

  mpar_t   *P;
  mqos_t   *Q;
  mclass_t *C;
  mgcred_t *U;
  mgcred_t *G;
  mgcred_t *A;

  mhashiter_t Iter;

  const char *FName = "MUIConfigShow";

  MDB(2,fUI) MLog("%s('%s',%d,%d,SBuffer,%d)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL",
    PIndex,
    Vflag,
    BufSize);


  if (PIndex == mcoNONE)
    {
    /* display header */

    MStringAppendF(&Response,"# %s version %s  PID: %d\n# requestor: %s\n",
      MSCHED_NAME,
      MOAB_VERSION,
      MOSGetPID(),
      (Auth != NULL) ? Auth : "N/A");

    if (MSched.InRecovery == TRUE)
      {
      MStringAppendF(&Response,"# recovery mode initiated\n\n");
      }
    }  /* END if (PIndex == mcoNONE) */

  if ((PIndex == mcoNONE) || (PIndex == mcoSchedCfg))
    {
    /* show scheduler config */

    MSchedConfigShow(
      &MSched,
      (Vflag > 0) ? TRUE : FALSE,
      &tmpString);

    MStringAppendF(&Response,"%s\n",tmpString.c_str());

    MSchedOConfigShow(PIndex,Vflag,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }  /* END if ((PIndex == mcoNONE) || (PIndex == mcoSchedCfg)) */

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    MParConfigShow(P,Vflag,PIndex,&tmpString);

    MStringAppend(&Response,tmpString.c_str());

    MPrioConfigShow(Vflag,P,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }   /* END for (pindex) */

  /* policies */

  P = &MPar[0];

  MDB(4,fUI) MLog("INFO:     policy parameters displayed\n");

  /* standing reservations */

  MSRConfigShow(NULL,Vflag,PIndex,&tmpString);

  MStringAppend(&Response,tmpString.c_str());

  MRsvProfileConfigShow(NULL,Vflag,PIndex,&tmpString);

  MStringAppend(&Response,tmpString.c_str());

  MNodeConfigShow(&MSched.DefaultN,Vflag,P,&tmpString);

  MStringAppend(&Response,tmpString.c_str());

  MNodeConfigShow(NULL,Vflag,P,&tmpString);

  MStringAppend(&Response,tmpString.c_str());

  /* class values */

  MClassConfigShow(NULL,Vflag,&tmpString);

  MStringAppend(&Response,tmpString.c_str());

  {
  enum MCredAttrEnum AList[] = {
    mcaDefWCLimit,
    mcaMaxNodePerJob,
    mcaMaxProcPerJob,
    mcaMaxProcPerUser,
    mcaMaxWCLimitPerJob,
    mcaPriority,
    mcaPref,
    mcaNONE };

  for (cindex = MCLASS_DEFAULT_INDEX;cindex < MSched.M[mxoClass];cindex++)
    {
    C = &MClass[cindex];

    if ((C->Name[0] == '\0') || (C->Name[1] == '\1'))
      continue;

    MCredConfigShow((void *)C,mxoClass,AList,Vflag,PIndex,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }  /* END for (cindex) */

  if (MSched.DefaultC != NULL)
    {
    MCredConfigShow(
      (void *)MSched.DefaultC,
      mxoClass,
      AList,
      Vflag,
      PIndex,
      &tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }

  /* QOS values */

  for (qindex = 0;qindex < MSched.M[mxoQOS];qindex++)
    {
    Q = &MQOS[qindex];

    if (Q->Name[0] == '\0')
      break;

    MQOSConfigShow(Q,Vflag,PIndex,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }  /* END for (qindex) */

  /* user values */

  MUHTIterInit(&Iter);
  while (MUHTIterate(&MUserHT,NULL,(void **)&U,NULL,&Iter) == SUCCESS)
    {
    MCredConfigShow(
      (void *)U,
      mxoUser,
      AList,
      Vflag,
      PIndex,
      &tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }  /* END for (uindex) */

  if (MSched.DefaultU != NULL)
    {
    MCredConfigShow((void *)MSched.DefaultU,mxoUser,AList,Vflag,PIndex,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }

  /* group values */

  MUHTIterInit(&Iter);
  while (MUHTIterate(&MGroupHT,NULL,(void **)&G,NULL,&Iter) == SUCCESS)
    {
    MCredConfigShow((void *)G,mxoGroup,AList,Vflag,PIndex,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }  /* END for (gindex) */

  if (MSched.DefaultG != NULL)
    {
    MCredConfigShow((void *)MSched.DefaultG,mxoGroup,AList,Vflag,PIndex,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }

  /* account values */

  MUHTIterInit(&Iter);
  while (MUHTIterate(&MAcctHT,NULL,(void **)&A,NULL,&Iter) == SUCCESS)
    {
    MCredConfigShow((void *)A,mxoAcct,AList,Vflag,PIndex,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }  /* END for (aindex) */
  }    /* END BLOCK */

  MSysConfigShow(&tmpString,PIndex,Vflag);

  MStringAppend(&Response,tmpString.c_str());

  MDB(4,fUI) MLog("INFO:     file/directory parameters displayed\n");

  MAMConfigShow(NULL,Vflag,&tmpString);

  MStringAppend(&Response,tmpString.c_str());

  MRMConfigShow(NULL,Vflag,&tmpString);

  MStringAppend(&Response,tmpString.c_str());

  MDB(4,fUI) MLog("INFO:     miscellaneous parameters displayed\n");

  /* simulation parameters */

  if (MSched.Mode == msmSim)
    {
    /* NOTE:  no bounds checking */

    MSimShow(&MSim,Vflag,&tmpString);

    MStringAppend(&Response,tmpString.c_str());
    }

  MUStrCpy(SBuffer,Response.c_str(),BufSize);

  return(SUCCESS);
  }  /* END MUIConfigShow() */



/*
 * Write scheduler status information to XML.
 *
 * WARNING: Although this function is a big improvement over the old method
 * of reporting scheduler status, it is still not completely thread-safe! If P is passed
 * in the call to MRMFind() is not thread-safe and is dangerous, especially
 * in environments where RM's could change!
 *
 * @param OEP (O) XML is created using this pointer--not freed! [alloc] 
 * @param P (I) If passed in, this function is no longer thread-safe! [optional]
 * @param Auth (I) [optional]
 */
 
int MUIGenerateSchedStatusXML(

  mxml_t **OEP,   /* O (not freed!) */
  mpsi_t  *P,     /* I (optional) */
  char    *Auth)  /* I (optional) */

  {
  mxml_t *OE = NULL; 

  if (OEP == NULL)
    {
    return(FAILURE);
    }

  *OEP = NULL;

  MXMLCreateE(&OE,(char *)MXO[mxoSched]);
  MXMLSetAttr(OE,"Status",(void *)"active",mdfString);
  MXMLSetAttr(OE,"time",(void *)&MSched.Time,mdfLong);
  MXMLSetAttr(OE,"iteration",(void *)&MSched.Iteration,mdfInt);

  MXMLSetAttr(OE,"moabversion",(void *)MOAB_VERSION,mdfString);

  MXMLSetAttr(
    OE,
    "moabrevision",
#ifdef MOAB_REVISION
    (void *)MOAB_REVISION,
#else /* MOAB_REVISION */
    (void *)"NA",
#endif /* MOAB_REVISION */
    mdfString);

  /* add simple diagnostics */

  if (MSched.State == mssmStopped)
    MXMLSetAttr(OE,"stopped",(void *)"true",mdfString);

  if (MSched.SRsvIsActive == TRUE)
    MXMLSetAttr(OE,"sreserved",(void *)"true",mdfString);

  if (Auth != NULL)
    {
    MXMLSetAttr(OE,"requester",(void *)Auth,mdfString);
    }

  MXMLSetAttr(OE,"cfgdir",(void *)MSched.CfgDir,mdfString);

  if ((MSched.Activity != macUIWait) && (MSched.Activity != macUIProcess))
    MXMLSetAttr(OE,"activity",(void *)MActivity[MSched.Activity],mdfString);

  *OEP = OE;

  return(SUCCESS);
  }  /* END MUIGenerateSchedStatusXML() */





/**
 * Report partition diagnostics (mdiag -t)
 *
 * @see MUIDiagnose() - parent
 * @see MParShow() - child
 *
 * @param Auth (I) (FIXME: not used but should be) 
 * @param RE (O)
 * @param String (O)
 * @param DiagOpt (I) [optional]
 * @param IFlags (I) [enum mcm*]
 * @param DFormat (I)
 */

int MUIParDiagnose(

  char                 *Auth,
  mxml_t              **RE,
  mstring_t            *String,
  char                 *DiagOpt,
  mbitmap_t            *IFlags,
  enum MFormatModeEnum  DFormat)

  {
  int rc;

  const char *FName = "MUIParDiagnose";

  MDB(3,fUI) MLog("%s(SBuffer,SBufSize,%s,%ld,%s)\n",
    FName,
    (DiagOpt != NULL) ? DiagOpt : "NULL",
    IFlags,
    MFormatMode[DFormat]);

  if (DFormat == mfmXML)
    {
    mxml_t *DE;

    DE = NULL;

    MXMLCreateE(&DE,(char *)MSON[msonData]);

    rc = MParShow(
           DiagOpt,
           DE,
           NULL,
           DFormat,
           IFlags,  /* I */
           TRUE);

    if (String == NULL)
      { 
      *RE = DE;
      }
    else
      {    
      MXMLToMString(
        DE,
        String,
        NULL,
        TRUE);

      MXMLDestroyE(&DE);
      }
    }    /* END if (DFormat == mfmXML) */
  else
    {
    rc = MParShow(DiagOpt,NULL,String,DFormat,IFlags,TRUE);
    }

  return(rc);
  }  /* END MUIParDiagnose() */



/**
 * Process matrix stats (showstats -f) requests.
 *
 * @param RBuffer (I)
 * @param SBuffer (O)
 * @param DFlags (I)
 * @param Cred (I)
 * @param SBufSize (I)
 */

int MUIStatShowMatrix(

  char      *RBuffer,
  char      *SBuffer,
  mbitmap_t *DFlags,
  char      *Cred,
  int        SBufSize)

  {
  enum MMStatTypeEnum SType;

  char SName[MMAX_NAME];

  enum MFormatModeEnum DFormat = mfmNONE;

  mgcred_t *U = NULL;

  const char *FName = "MUIStatShowMatrix";

  MDB(3,fUI) MLog("%s(RBuffer,SBuffer,%d,%s,SBufSize)\n",
    FName,
    DFlags,
    Cred);

  MUserFind(Cred,&U);

  if (MUICheckAuthorization(
        U,
        NULL,
        NULL,
        mxoSched,
        mcsStatShow,
        mgscQuery,
        NULL,
        NULL,
        0) == FAILURE)
    {
    sprintf(SBuffer,"          user '%s' not authorized to run command\n",
      Cred);

    return(FAILURE);
    }

  MUSScanF(RBuffer,"%x%s",
    sizeof(SName),
    SName);

  /* determine requested statistics type */

  if ((SType = (enum MMStatTypeEnum)MUGetIndexCI(SName,MStatType,FALSE,mgstNONE)) == mgstNONE)
    {
    MDB(2,fUI) MLog("INFO:     invalid stat type '%s' requested\n",
      SName);

    sprintf(SBuffer,"ERROR:  invalid statistics type requested\n");

    return(FAILURE);
    }  /* END if (MUGetIndexCI() == mgstNONE) */

  if (bmisset(DFlags,mcmXML))
    DFormat = mfmXML;

  MStatBuildMatrix(SType,SBuffer,SBufSize,DFlags,DFormat,NULL);

  return(SUCCESS);
  }  /* END MUIStatShowMatrix() */





/**
 *
 *
 * @param Auth (I)
 * @param SP (I) [optional]
 * @param SBuffer (O)
 * @param SBufSize (I)
 */

int MUIParShowStats(

  char   *Auth,     /* I */
  mpar_t *SP,       /* I (optional) */
  char   *SBuffer,  /* O */
  int     SBufSize) /* I */

  {
  mgcred_t *U;

  const char *FName = "MUIParShowStats";

  MDB(3,fUI) MLog("%s(%s,%s,SBuffer,SBufSize)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL",
    (SP != NULL) ? SP->Name : "NULL");

  if (SBuffer == NULL)
    {
    return(FAILURE);
    }

  if (MUserFind(Auth,&U) == FAILURE)
    {
    return(FAILURE);
    }
  
  if (MUICheckAuthorization(
        U,
        NULL,
        (void *)SP,
        mxoPar,
        mcsStatShow,
        mgscQuery,
        NULL,
        SBuffer,
        SBufSize) == FAILURE)
    {
    return(FAILURE);
    }

  snprintf(SBuffer,SBufSize,"internal error - cannot generate requested partition statistics\n");

  return(SUCCESS);
  }  /* END MUIParShowStats() */



/**
 *
 *
 * @param PName (I)
 * @param EMsg (O) [optional]
 * @param Size (I)
 */

int MUIConfigPurge(

  char *PName,  /* I */
  char *EMsg,   /* O (optional) */
  int   Size)   /* I */

  {
  char  tmpLine[MMAX_BUFFER];
  char  tmpPLine[MMAX_LINE];

  char  IName[MMAX_NAME];

  char *PVal;

  enum MCfgParamEnum CIndex;

  char *tmpCfgBuf = NULL;

  char  CfgFName[MMAX_LINE];
  char  DatFName[MMAX_LINE];

  char *TokPtr;

  char *AName;

  char *btail;

  mbool_t ExtractParam;
  mbool_t BufModified;

  char  Name[MMAX_NAME];       /* store name of config */
  enum MCfgParamEnum Index;    /* store type of config */
  void *O;

  CfgFName[0] = '\0';

  /* remove parameter from config files */

  if ((PName == NULL) || (PName[0] == '\0'))
    {
    return(SUCCESS);
    }

  /* NOTE:  PLine is single parameter specification */

  MUStrCpy(tmpPLine,PName,sizeof(tmpPLine));
  MUStrCpy(tmpLine,PName,sizeof(tmpLine));

  MUGetWord(tmpPLine,&PName,&PVal);

  if (MCfgGetIndex(mcoLAST,PName,FALSE,&CIndex) == FAILURE)
    {
    return(FAILURE);
    }

  if (MCfgGetParameter(
        tmpLine,
        MCfg[CIndex].Name,
        &CIndex,
        IName,    /* O */
        NULL,
        -1,
        FALSE,
        NULL) == FAILURE)
    {
    MDB(2,fCONFIG) MLog("WARNING:  config line '%s' cannot be processed\n",
      PName);

    if (EMsg != NULL)
      sprintf(EMsg,"ERROR:  specified parameter cannot be purged");

    return(FAILURE);
    }

  /* store variables for later object deletion */

  MUStrCpy(Name,IName,sizeof(Name));

  Index = CIndex;

  sprintf(DatFName,"%s%s%s",
    (MSched.CfgDir[0] != '\0') ? MSched.CfgDir : "",
    (MSched.CfgDir[strlen(MSched.CfgDir) - 1] != '/') ? "/" : "",
    MDEF_DATFILE);

  if ((MSched.CfgDir[0] != '\0') && !strstr(MSched.ConfigFile,MSched.CfgDir))
    {
    strcpy(CfgFName,MSched.CfgDir);

    if (MSched.CfgDir[strlen(MSched.CfgDir) - 1] != '/')
      {
      strcat(CfgFName,"/");
      }
    }

  strcat(CfgFName,MSched.ConfigFile);

  /* re-load config buffer to reflect most recent user changes */

  if ((tmpCfgBuf = MFULoad(CfgFName,1,macmWrite,NULL,NULL,NULL)) == NULL)
    {
    MDB(2,fCONFIG) MLog("WARNING:  cannot load config file '%s'\n",
      CfgFName);

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot load config file '%s'",
        CfgFName);
      }

    return(FAILURE);
    }

  /* NOTE:  maintain raw copy for writing */

  MUStrDup(&MSched.ConfigBuffer,tmpCfgBuf);
  MCfgAdjustBuffer(&MSched.ConfigBuffer,FALSE,&btail,FALSE,TRUE,FALSE);

  /* remove user specified parameters */

  MUStrCpy(tmpLine,PName,sizeof(tmpLine));

  /* extract attribute name */

  /* FORMAT:  <PARAM>[<INAME>] <ANAME>=<AVAL> */

  if (strchr(tmpLine,'=') == NULL)
    {
    AName = NULL;
    }
  else
    {
    /* parse parameter name */

    MUStrTok(tmpLine," \t\n",&TokPtr);

    /* parse attribute name */

    AName = MUStrTok(NULL," \t\n=",&TokPtr);
    }

  ExtractParam = TRUE;

  BufModified = FALSE;

  while (ExtractParam == TRUE)
    {
    mbool_t tmpBufModified;

    if (MCfgExtractAttr(
          MSched.ConfigBuffer,
          &tmpCfgBuf,
          CIndex,  /* I */
          IName,   /* I */
          AName,   /* I */
          TRUE,
          &tmpBufModified) == FAILURE)
      {
      MUFree(&tmpCfgBuf);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot extract attribute '%s' from parameter '%s' in config database",
          IName,
          AName);
        }

      return(FAILURE);
      }

    if (tmpBufModified == TRUE)
      BufModified = TRUE;

    if (AName != NULL)
      {
      /* parse next attribute value */

      MUStrTok(NULL," \t\n",&TokPtr);

      /* parse next attribute name */

      AName = MUStrTok(NULL," \t\n",&TokPtr);
      }

    if (AName == NULL)
      ExtractParam = FALSE;
    }  /* END while (ExtractParam == TRUE) */

  if (BufModified == TRUE)
    {
    MFUCreate(
      CfgFName,
      NULL,
      tmpCfgBuf,
      strlen(tmpCfgBuf),
      -1,
      -1,
      -1,
      FALSE,
      NULL);
    }

  MUFree(&tmpCfgBuf);

  /* set/overwrite parameters in dat config */

  /* locate parameters in dat config */

  if ((tmpCfgBuf = MFULoad(DatFName,1,macmWrite,NULL,NULL,NULL)) == NULL)
    {
    MDB(2,fCONFIG) MLog("WARNING:  cannot load config file '%s'\n",
      DatFName);

    sprintf(tmpLine,"# DAT File --- Do Not Modify --- parameters in moab.cfg will override parameters here\n# This file is automatically modified by moab\n");

    MUStrDup(&MSched.DatBuffer,tmpLine);

    MUStrDup(&tmpCfgBuf,tmpLine);
    }
  else
    {
    /* refresh dat buffer */

    MUStrDup(&MSched.DatBuffer,tmpCfgBuf);
    }
  /* remove dat parameters */

  MUStrCpy(tmpLine,PName,sizeof(tmpLine));

  /* extract attribute name */

  /* FORMAT:  <PARAM>[<INAME>] <ANAME>=<AVAL> */

  if (strchr(tmpLine,'=') == NULL)
    {
    AName = NULL;
    }
  else
    {
    /* parse parameter name */

    MUStrTok(tmpLine," \t\n",&TokPtr);

    /* parse attribute name */

    AName = MUStrTok(NULL," \t\n=",&TokPtr);
    }

  ExtractParam = TRUE;

  BufModified = FALSE;

  while (ExtractParam == TRUE)
    {
    mbool_t tmpBufModified;

    if (MCfgExtractAttr(
          MSched.DatBuffer,
          &tmpCfgBuf,
          CIndex,  /* I */
          IName,   /* I */
          AName,   /* I */
          TRUE,
          &tmpBufModified) == FAILURE)
      {
      MUFree(&tmpCfgBuf);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot extract attribute '%s' from parameter '%s' in config database",
          IName,
          AName);
        }

      return(FAILURE);
      }

    if (tmpBufModified == TRUE)
      BufModified = TRUE;

    if (AName != NULL)
      {
      /* parse next attribute value */

      MUStrTok(NULL," \t\n",&TokPtr);

      /* parse next attribute name */

      AName = MUStrTok(NULL," \t\n",&TokPtr);
      }

    if (AName == NULL)
      ExtractParam = FALSE;
    }  /* END while (ExtractParam == TRUE) */

  MCfgCompressBuf(tmpCfgBuf);

  MFUCreate(
    DatFName,
    NULL,
    tmpCfgBuf,
    strlen(tmpCfgBuf),
    -1,
    -1,
    -1,
    FALSE,
    NULL);

  if ((tmpCfgBuf = MFULoad(DatFName,1,macmWrite,NULL,NULL,NULL)) == NULL)
    {
    MDB(2,fCONFIG) MLog("WARNING:  cannot load dat file '%s'\n",
      DatFName);
    }

  MUFree(&MSched.DatBuffer);

  MSched.DatBuffer = tmpCfgBuf;

  tmpCfgBuf = NULL;

  MUFree(&tmpCfgBuf);

  MSched.ConfigSerialNumber++;

  MSched.EnvChanged = TRUE;

  /* mark object as deleted */

  switch (MCfg[Index].PIndex)
    {
    case mcoUserCfg:

      if (MOGetObject(mxoUser,Name,&O,mVerify) == SUCCESS)
        {
        ((mgcred_t *)O)->IsDeleted = TRUE;
        }

      break;

    case mcoClassCfg:

      if (MOGetObject(mxoClass,Name,&O,mVerify) == SUCCESS)
        {
        ((mclass_t *)O)->IsDeleted = TRUE;
        }

      break;

    case mcoAcctCfg:

      if (MOGetObject(mxoAcct,Name,&O,mVerify) == SUCCESS)
        {
        ((mgcred_t *)O)->IsDeleted = TRUE;
        }

      break;

    case mcoQOSCfg:

      if (MOGetObject(mxoQOS,Name,&O,mVerify) == SUCCESS)
        {
        ((mgcred_t *)O)->IsDeleted = TRUE;
        }

      break;

    case mcoGroupCfg:

      if (MOGetObject(mxoGroup,Name,&O,mVerify) == SUCCESS)
        {
        ((mgcred_t *)O)->IsDeleted = TRUE;
        }

      break;

    case mcoNodeCfg:

      if (MOGetObject(mxoNode,Name,&O,mVerify) == SUCCESS)
        {
        bmset(&((mnode_t *)O)->IFlags,mnifIsDeleted);
        }

      break;

    case mcoSRCfg:

      if (MSRFind(Name,(msrsv_t **)&O,NULL) == SUCCESS)
        {
        ((msrsv_t *)O)->IsDeleted = TRUE;
        }

      break;

    case mcoRMCfg:

      if (MRMFind(Name,(mrm_t **)&O) == SUCCESS)
        {
        ((mrm_t *)O)->IsDeleted = TRUE;
        }

      break;

    default:

      /* NO-OP */

      break;
    }

  return(SUCCESS);
  }  /* END MUIConfigPurge() */




/**
 *
 *
 * @param RBuffer
 * @param Buffer
 * @param FLAGS
 * @param Auth
 * @param BufSize
 */

int MUIShowEstStartTime(

  const char *RBuffer,
  char       *Buffer,
  mbitmap_t  *FLAGS,
  char       *Auth,
  long       *BufSize)

  {
  const char *FName = "MUIShowEstStartTime";

  MDB(2,fUI) MLog("%s(RBuffer,Buffer,%d,%s,BufSize)\n",
    FName,
    FLAGS,
    Auth);

  /* show estimated expected and worst-case job start time */

  /* NYI */

  return(SUCCESS);
  }  /* END MUIShowEstStartTime() */




/**
 * Diagnose moab's object size and dynamic/static limits.
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param String (O)
 * @param Flags (I)
 */

int MUILimitDiagnose( 

  char      *Auth,
  mstring_t *String,
  mbitmap_t *Flags)

  {
  enum MFormatModeEnum DFormat;

  int   OSize;
  int   TSize;

  int   oindex;

  mxml_t *DE;

  enum MXMLOTypeEnum OList[] = {
    mxoNode,
    mxoRsv,
    mxoJob,
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoQOS,
    mxoClass,
    mxoNONE };

  const char *FName = "MUILimitDiagnose";

  MDB(3,fUI) MLog("%s(String)\n",
    FName);

  if (String == NULL)
    {
    return(FAILURE);
    }

  /* show object table overview */

  MDB(4,fUI) MLog("INFO:     evaluating object table (%d slots)\n",
    MSched.M[mxoNode]);

  if (bmisset(Flags,mcmXML))
    {
    DFormat = mfmXML;
    }
  else
    {
    DFormat = mfmNONE;
    }

  if (DFormat == mfmXML)
    {
    DE = NULL;

    MXMLCreateE(&DE,(char *)MSON[msonData]);
    }
  else
    {
    MStringAppendF(String,"object table summary\n");

    /* create summary */

    MStringAppendF(String,"%-8.8s %6.6s/%-6.6s  %15.15s %18.18s\n",
      "object",
      "active",
      "max",
      "table size (MB)",
      "object size (byte)");

    MStringAppendF(String,"%-8.8s %6.6s/%-6.6s  %15.15s %18.18s\n",
      "----------------------",
      "----------------------",
      "----------------------",
      "----------------------",
      "----------------------");

    TSize = 0;

    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      OSize = (MSched.AS[OList[oindex]] > 0) ? 
        MSched.AS[OList[oindex]] : MSched.Size[OList[oindex]];

      TSize += MSched.A[OList[oindex]] * OSize;

      if ((OList[oindex] == mxoJob))
        {
        MStringAppendF(String,"%-8.8s %6d/%-6d  %15.2f %18d\n",
          MXO[mxoJob],
          MJobHT.size(),
          MSched.M[mxoJob],
          (double)MJobHT.size() * OSize / 1024 / 1024,
          OSize);
        }
      else if ((OList[oindex] == mxoRsv) ||
          (OList[oindex] == mxoUser) ||
          (OList[oindex] == mxoGroup) ||
          (OList[oindex] == mxoAcct))
        {
        /* Rsv, User, Group, Acct have no max limit values */

        MStringAppendF(String,"%-8.8s %6d/ -      %15.2f %18d\n",
          MXO[OList[oindex]],
          MSched.A[OList[oindex]],
          (double)MSched.A[OList[oindex]] * OSize / 1024 / 1024,
          OSize);
        }
      else
        {
        /* All others (node,rsv,job,qos,class) have max limits */

        MStringAppendF(String,"%-8.8s %6d/%-6d  %15.2f %18d\n",
          MXO[OList[oindex]],
          MSched.A[OList[oindex]],
          MSched.M[OList[oindex]],
          (double)MSched.A[OList[oindex]] * OSize / 1024 / 1024,
          OSize);
        }
      }  /* END for (oindex) */

    MStringAppendF(String,"%-8.8s %6.6s/%-6.6s  %15.2f %18.18s\n",
      "total",
      "------",
      "------",
      (double)TSize / 1024 / 1024,
      "----------------------");
    }  /* END else (Format == mfmXML) */

  if (DFormat == mfmXML)
    {
    MXMLToMString(DE,String,NULL,TRUE);

    MXMLDestroyE(&DE);
    }

  return(SUCCESS);
  }  /* END MUILimitDiagnose() */





/**
 *
 *
 * @param J (I)
 */

int MUIQRemoveJob(

  mjob_t *J)   /* I */

  {
  int jindex;

  mjob_t *tmpJ;

#ifndef __MOPT
  if (J == NULL)
    {
    return(FAILURE);
    }
#endif  /* !__MOPT */

  for (jindex = 0;MUIQ[jindex] != NULL;jindex++)
    {
    tmpJ = MUIQ[jindex];

    if (J != tmpJ)
      continue;

    /* remove job from MUIQ and adjust MUIQ */

    while (MUIQ[jindex] != NULL)
      {
      MUIQ[jindex] = MUIQ[jindex+1];

      jindex++;
      }

    return(SUCCESS);
    }

  /* job never found */

  return(SUCCESS);
  }  /* END MUIQRemoveJob() */




/**
 *
 *
 * @param LHead   (I)
 * @param S       (I) [object representing transaction]
 * @param DPtr    (I)
 * @param Arg2    (I)
 * @param Handler (I)
 */

int MUITransactionAdd(

  mln_t     **LHead,
  msocket_t  *S,
  void       *DPtr,
  void       *Arg2,
  int (*Handler)(msocket_t *,void *,void *,void *,void *))
  
  {
  static int counter = 0;
  char tmpName[MMAX_NAME];
  int rc;

  if ((LHead == NULL) ||
      (S == NULL) ||
      (DPtr == NULL) ||
      (Handler == NULL))
    {
    return(FAILURE);
    }

  /* increment unique id */

  counter++;

  S->ResponseContextList[0] = DPtr;
  S->ResponseContextList[1] = Arg2;
  S->ResponseHandler        = Handler;
  S->RequestTime            = MSched.Time;
 
  snprintf(tmpName,sizeof(tmpName),"%d",
    counter);

  rc = MULLAdd(LHead,tmpName,S,NULL,(int(*)(void **))MSUFree);

  return(rc);
  }  /* END MUITransactionAdd() */




/**
 * Transaction has been processed, remove from transaction list.
 *
 * @param LHead (I)
 * @param Target (I)
 */

int MUITransactionRemove(

  mln_t **LHead,  /* I */
  mln_t  *Target) /* I */

  {
  char tmpName[MMAX_NAME];
  int rc;

  const char *FName = "MUITransactionRemove";

  MDB(7,fUI) MLog("%s(LHead,%s)\n",
    FName,
    (Target != NULL) ? Target->Name : "NULL");

  if ((LHead == NULL) || (Target == NULL))
    {
    return(FAILURE);
    }

  MUStrCpy(tmpName,Target->Name,sizeof(tmpName));

  rc = MULLRemove(LHead,tmpName,NULL);

  return(rc);
  }  /* END MUITransactionRemove() */






/**
 * Process non-blocking transactions (called from MUIAcceptRequest())
 *
 * @param FlushTransactions (I)
 * @param EMsg (O) [optional]
 */

int MUIProcessTransactions(

  mbool_t FlushTransactions,    
  char *EMsg) /* O (optional,minsize=MMAX_LINE) */

  {
  mln_t      *Trans;
  mln_t      *next;
  msocket_t  *S;

  enum MStatusCodeEnum SC;

  const char *FName = "MUIProcessTransactions";

  MDB(9,fUI) MLog("%s(EMsg)\n",
    FName);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  Trans = MUITransactions;

  /* go through list, check each transaction for data */

  while (Trans != NULL)
    {
    S = (msocket_t *)Trans->Ptr;

    /* poll for data */
   
    MXMLDestroyE(&S->RE);
    MXMLDestroyE(&S->RDE);

    MUFree(&S->RBuffer);
    S->RPtr = NULL;

    if (FlushTransactions == TRUE)
      {
      /* flushing all entries, disconnect and remove from list */

      next = Trans->Next;

      MSUFree(S);

      MUITransactionRemove(&MUITransactions,Trans);

      MUFree((char **)&S);
      } 
    else if (MSURecvData(S,0,TRUE,&SC,EMsg) == SUCCESS)
      {
      /* process data and update objects involved in transaction */

      if (S->ResponseHandler != NULL)
        {
        MDB(7,fUI) MLog("INFO:     calling transaction handler for socket '%d' (TID: %ld)\n",
          S->sd,
          S->LocalTID);

        S->ResponseHandler(S,
          S->ResponseContextList[0],
          S->ResponseContextList[1],
          NULL,
          NULL);

        MDB(6,fUI) MLog("INFO:     transaction processed - removing transaction for socket '%d' (TID: %ld)\n",
          S->sd,
          S->LocalTID);
        }
      else
        {
        /* should never happen */

        return(FAILURE);
        }

      /* disconnect and remove from list */

      next = Trans->Next;

      MSUFree(S);

      MUITransactionRemove(&MUITransactions,Trans);

      MUFree((char **)&S);
      }
    else
      {
      /* detect if socket is closed, and if so, remove from list */

      switch (SC)
        {        
        case mscNoEnt:

          /* socket has problems */

          MDB(4,fUI) MLog("INFO:     socket error occured - removing transaction for socket '%d' (TID: %ld) SC=%d EMsg=%s\n",
            S->sd,
            S->LocalTID,
            SC,
            (EMsg != NULL) ? EMsg : "NULL");

          next = Trans->Next;

          MSUFree(S);

          MUITransactionRemove(&MUITransactions,Trans);

          MUFree((char **)&S);

          break;

        case mscNoData:  /* poll found no data waiting */

          /* NO-OP */

          MDB(8,fUI) MLog("INFO:     no socket data available for transaction for socket '%d' (TID: %ld) - polling will continue\n",
            S->sd,
            S->LocalTID);

          next = Trans->Next;

          break;

        default:

          MDB(4,fUI) MLog("INFO:     unknown socket error occured - removing transaction for socket '%d' (TID: %ld) SC=%d EMsg=%s\n",
            S->sd,
            S->LocalTID,
            SC,
            (EMsg != NULL) ? EMsg : "NULL");

          next = Trans->Next;

          MSUFree(S);

          MUITransactionRemove(&MUITransactions,Trans);

          MUFree((char **)&S);

          break;
        }
      }    /* END else () */

    Trans = next;
    }  /* END while (Trans != NULL) */ 

  return(SUCCESS);  
  }  /* END MUIProcessTransactions() */





/**
 * Searches the socket transaction list managed by the commnication thread for 
 * a specific transaction type.
 *
 * NOTE: This function must remain thread safe!
 *
 * @param Handler (I)
 * @param R (I) [optional - constrain search to transactions from this RM]
 * @param SP (O) [optional]
 */

int MUITransactionFindType(

  int       (*Handler)(msocket_t *,void *,void *,void *,void *),  /* I */
  mrm_t      *R,   /* I (optional - constrain search to transactions from this RM) */
  msocket_t **SP)  /* O (optional) */

  {
  msocket_t *S;
  mln_t     *ptr;

  mulong ETime;  /* transaction expire time */

  if (Handler == NULL)
    {
    return(FAILURE);
    }

  for (ptr = MUITransactions;ptr != NULL;ptr = ptr->Next)
    {
    if (ptr->Name == NULL)
      break;

    S = (msocket_t *)ptr->Ptr;

    ETime = S->RequestTime + ((S->Timeout > 0) ? 
      S->Timeout / MDEF_USPERSECOND : 
      MCONST_MINUTELEN);

    if (MSched.Time > ETime)
      {
      /* transaction has expired, purge */

      /* NYI - JOSH */

      continue;
      }

    if (S->ResponseHandler == Handler)
      {
      if ((R == NULL) || (S->ResponseContextList[0] == (void *)R))
        {
        /* match of same type found */

        if (SP != NULL)
          {
          *SP = S;
          }

        return(SUCCESS);
        }
      }
    }  /* END for (ptr) */

  return(FAILURE);
  }  /* END MUITransactionFindType() */



/**
 * Report local reservations to peer (grid).
 *
 * @param RP    (I)
 * @param PR    (I)
 * @param Auth  (I)
 * @param DE    (I) [modified]
 * @param Flags (I)
 */

int MUIExportRsvQuery(

  mrsv_t  *RP,
  mrm_t   *PR,
  char    *Auth,
  mxml_t  *DE,
  mulong   Flags)

  {
  rsv_iter RTI;

  mrsv_t *R;

  mxml_t *RE;

  char *PName = NULL;

  enum MRsvAttrEnum RAList[] = {
    mraComment,
    mraEndTime,      
    mraFlags,
    mraGlobalID,
    mraName,
    mraResources,
    mraRsvGroup,
    mraSID,
    mraSpecName,
    mraStartTime,
    mraType,
    mraNONE };

  enum MRsvAttrEnum RAList2[] = {
    mraHostExp,
    mraACL,
    mraOwner,
    mraNONE };

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    PName = Auth + strlen("peer:");
  else 
    PName = Auth;

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((RP != NULL) && (R != RP))
      continue;

    if ((PR != NULL) && !bmisset(&PR->Flags,mrmfLocalRsvExport) && 
        ((R->SystemID == NULL) || strcmp(R->SystemID,PName)))
      {
      /* don't export reservation if localrsvexport is not set and
         there is no system id */

      continue;
      }

    RE = NULL;
 
    MRsvToXML(R,&RE,RAList,NULL,FALSE,mcmNONE);
  
    MRsvToExportXML(R,&RE,RAList2,PR);

    if (R->SystemRID == NULL)
      {
      MXMLSetAttr(RE,(char *)MRsvAttr[mraGlobalID],R->Name,mdfString);
      }

    MXMLAddE(DE,RE);
    }

  return(SUCCESS);
  }    /* END MUIExportRsvQuery() */





/**
 * compare job transition objects by job ID, ordered low to high
 *
 * @param A (I)
 * @param B (I)
 */

int __MTransJobNameCompLToH(

  mtransjob_t **A,  /* I */
  mtransjob_t **B)  /* I */

  {
  char *ANumIndex;
  char *BNumIndex;

  mbool_t HaveNumericID = FALSE;

  int ANum = -1;
  int BNum = -1;

  char *AName = (*A)->Name;
  char *BName = (*B)->Name;

  /* check for jobID of the type 'Moab.xx' -- these need to be sorted
   * numerically by the xx */

  ANumIndex = strrchr(AName,'.');
  BNumIndex = strrchr(BName,'.');

  if ((ANumIndex != NULL) && (BNumIndex != NULL))
    {
    ANum = strtol(++ANumIndex,NULL,10);
    BNum = strtol(++BNumIndex,NULL,10);

    HaveNumericID = TRUE;
    }
  else if ((isdigit(AName[0])) && (isdigit(BName[0]))) 
    {
    ANum = strtol(AName,NULL,10);
    BNum = strtol(BName,NULL,10);

    HaveNumericID = TRUE;
    }

  if (HaveNumericID)
    {
    if (ANum < BNum)
      return(-1);
    else if (ANum == BNum)
      return(0);
    else
      return(1);
    } /* END if(HaveNumericID) */

  else
    return(strcmp(AName,BName));
  } /* END __MTransJobNameCompLToH() */












#ifdef JOSH
/**
 *
 *
 * @param MinDate (I) [optional]
 * @param OBuf (I) [realloc]
 * @param OBufSize (I)
 */

int __MUIPackageSpoolFiles(

  long   MinDate,  /* I (optional) */
  char **OBuf,     /* I (realloc) */
  long  *OBufSize) /* I */

  {
  DIR *DirHandle;
  struct dirent *File;
  mulong FMTime;
  char *SpoolBuffer;
  int count;

  if (OBuf == NULL)
    {
    return(FAILURE);
    }

  if (*OBuf == NULL)
    {
    *OBufSize = MMAX_BUFFER;

    *OBuf = (char *)MUMalloc(sizeof(char) * *OBufSize);
    }

  DirHandle = opendir(MSched.SpoolDir);

  if (DirHandle == NULL)
    {
    return(FAILURE);
    }

  File = readdir(DirHandle);

  while (File != NULL)
    {
    /* attempt to send spool file */

    if (MFUGetInfo(File->d_name,&FMTime,NULL,NULL,NULL) == FAILURE)
      {
      MDB(5,fALL) MLog("ALERT:    cannot determine modify time for file '%s'\n",
        File->d_name);

      FMTime = 0;
      }

    if (FMTime <= MinDate)
      {
      /* local spool file is old */

      continue;
      }
    else if ((SpoolBuffer = MFULoad(File->d_name,1,macmRead,&count,NULL,NULL)) == NULL)
      {
      /* cannot load spool file */

      MDB(1,fCKPT) MLog("WARNING:  cannot load checkpoint file '%s'\n",
        File->d_name);

      continue;
      }
    else
      {
      /* local spool file is newer than MinDate */

      /* send spool file */

      while (MUStringPack(SpoolBuffer,*OBuf,*OBufSize) == FAILURE)
        {
        *OBufSize += MMAX_BUFFER;

        *OBuf = (char *)realloc(*OBuf,*OBufSize);

        if (*OBuf == NULL)
          {
          MDB(1,fCKPT) MLog("ALERT:    memory realloc failure in mschedctl\n");

          return(FAILURE);
          }
        }
      }

    /* PUT FILES INTO BUFFER IN SEQUENTIAL ORDER--NYI */
    }

  return(SUCCESS);
  }  /* END __MUIPackageSpoolFiles() */

#endif /* JOSH */





/**
 *
 *
 * @param Auth (I) [optional]
 * @param FE   (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MUISchedCtlRecvFile(

  char                 *Auth,  /* I (optional) */
  mxml_t               *FE,    /* I */
  char                 *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  int rc;
  int index;

  char  tmpPath[MMAX_PATH];
  char  FilePath[MMAX_PATH];

  char  FileName[MMAX_LINE];
  char  DirName[MMAX_LINE];
  char *FileBuf = NULL;
  int   FileBufSize = MMAX_BUFFER >> 1;

  mbool_t IsAdmin   = FALSE;

  mpsi_t   *P = NULL;
  mgcred_t *U = NULL;

  const char *FName = "__MUISchedCtlRecvFile";

  MDB(3,fUI) MLog("%s(%s,FE,EMsg)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (FE == NULL)
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Auth != NULL)
    {
    if (!strncasecmp(Auth,"peer:",strlen("peer:")))
      {
      if (MPeerFind(Auth,FALSE,&P,FALSE) == FAILURE)
        {
        /* NYI */
        }
      }
    else
      {
      MUserAdd(Auth,&U);
      }

    if (MUICheckAuthorization(
          U,
          P,
          NULL,
          mxoSched,
          mcsMSchedCtl,
          msctlCopyFile,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      snprintf(EMsg,MMAX_BUFFER,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }
    }

  FileBuf = (char *)MUMalloc(FileBufSize * sizeof(char));

  MXMLGetAttr(FE,"file",NULL,tmpPath,sizeof(tmpPath));

  /* extract file name and relative dir */

  FileName[0] = '\0';

  for (index = strlen(tmpPath);index >= 0;index--)
    {
    if ((index == 0) || (tmpPath[index] == '/'))
      {
      MUStrCpy(FileName,&tmpPath[index+1],sizeof(FileName));

      break;
      }
    }

  tmpPath[index] = '\0';

  DirName[0] = '\0';

  for (index = strlen(tmpPath);index >= 0;index--)
    {
    if ((index == 0) || (tmpPath[index] == '/'))
      {
      MUStrCpy(FileName,&tmpPath[index+1],sizeof(FileName));

      break;
      }
    }

  if (DirName[0] == '\0')
    MUStrCpy(DirName,"spool",sizeof(DirName));

  /* for now all files must be relative to Moab's var dir */

  snprintf(FilePath,sizeof(FilePath),"%s/%s/%s",
    MSched.VarDir,
    DirName,
    FileName);

  /* must convert string before writing out to file */

  while ((MUStringUnpack(FE->Val,FileBuf,FileBufSize,&rc) != NULL) && (rc == FAILURE))
    {
    FileBufSize += MMAX_BUFFER;

    FileBuf = (char *)realloc(FileBuf,FileBufSize);

    if (FileBuf == NULL)
      {
      return(FAILURE);
      }
    }

  if (MFUCreate(
        FilePath,
        NULL,
        FileBuf,
        strlen(FileBuf),
        -1,
        -1,
        -1,
        TRUE,
        NULL) == FAILURE)
    {
    return(FAILURE);
    }

  MUFree(&FileBuf);

  return(SUCCESS);
  }  /* END __MUISchedCtlRecvFile() */




/**
 * Adds job template info to the already existing XML.
 *  JE should have already been populated from MJobToXML.  This function merely adds in
 *  info that is specific to job templates.
 *
 * @param J  (I) - The job template
 * @param JE (O) - The already-initialized XML element for the JOB - a new XML element
 *                    for tx will be added as a child to JE
 */

int MUIJTXToXML(

  mjob_t *J,  /* I */
  mxml_t *JE) /* O */

  {
  mxml_t *TE;
  mjtx_t *TX;
    
  if ((J == NULL) || (J->TemplateExtensions == NULL) || (JE == NULL))
    {
    return(FAILURE);
    }

  /* sync XML name w/ MJobFromXML */

  if (MXMLCreateE(&TE,(char *)"tx") == FAILURE)
    {
    return(FAILURE);
    }

  TX = J->TemplateExtensions;

  if (bmisset(&TX->TemplateFlags,mtjfTemplateIsSelectable))
    {
    MXMLSetAttr(TE,(char *)MJobCfgAttr[mjcaSelect],(void **)MBool[TRUE],mdfString);
    }

  if (MJobTemplateIsValidGenSysJob(J))
    {
    MXMLSetAttr(TE,(char *)MJobCfgAttr[mjcaGenSysJob],(void **)MBool[TRUE],mdfString);
    }

  if (bmisset(&J->IFlags,mjifInheritResources))
    {
    MXMLSetAttr(TE,(char *)MJobCfgAttr[mjcaInheritResources],(void **)MBool[TRUE],mdfString);
    }

  if (TX->JobReceivingAction != NULL)
    {
    MXMLSetAttr(TE,(char *)"JobReceivingAction",(void **)TX->JobReceivingAction,mdfString);
    }

  if (TX->TJobAction != mtjatNONE)
    {
    MXMLSetAttr(TE,(char *)"TJobAction",(void **)MTJobActionTemplate[TX->TJobAction],mdfString);
    }

  if (TX->Depends != NULL)
    {
    mln_t    *tmpL;
    mstring_t Deps(MMAX_LINE);
    mjob_t   *TJob;

    enum MDependEnum DependType;

    tmpL = TX->Depends;

    while (tmpL != NULL)
      {
      if (MTJobFind(tmpL->Name,&TJob) == SUCCESS)
        {
        if (!Deps.empty())
          {
          MStringAppend(&Deps,",");
          }

        if (bmisset(&tmpL->BM,mdJobSuccessfulCompletion))
          {
          DependType = mdJobSuccessfulCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobCompletion))
          {
          DependType = mdJobCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobFailedCompletion))
          {
          DependType = mdJobFailedCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobBeforeSuccessful))
          {
          DependType = mdJobBeforeSuccessful;
          }
        else if (bmisset(&tmpL->BM,mdJobBefore))
          {
          DependType = mdJobBefore;
          }
        else if (bmisset(&tmpL->BM,mdJobBeforeFailed))
          {
          DependType = mdJobBeforeFailed;
          }
        else if (bmisset(&tmpL->BM,mdJobSyncMaster))
          {
          DependType = mdJobSyncMaster;
          }
        else
          {
          DependType = mdJobSuccessfulCompletion;
          }

        MStringAppendF(&Deps,"%s:%s",MDependType[DependType],TJob->Name);
        }

      tmpL = tmpL->Next;
      }

    MXMLSetAttr(TE,(char *)MJobCfgAttr[mjcaTemplateDepend],Deps.c_str(),mdfString);
    }

  MXMLAddE(JE,TE);

  return(SUCCESS);
  }  /* END __MUIJTXToXML() */

/* END MUI.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
