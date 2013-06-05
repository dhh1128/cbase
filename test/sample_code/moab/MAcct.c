/* HEADER */

/**
 * @file MAcct.c
 *
 * Moab Accounts
 */

/* Contains:                                         *
 *  int MAcctLoadCP(AS,Buf)                          *
 *  int MAcctFind(A,AccountName)                     *
 *                                                   */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"


/**
 *
 *
 * @param AS (I) [modified]
 * @param Buf (O) [minsize=MMAX_BUFFER]
 */


int MAcctLoadCP(
 
  mgcred_t *AS,  /* I (modified) */
  const char     *Buf) /* O (minsize=MMAX_BUFFER) */
 
  {
  char    tmpHeader[MMAX_NAME + 1];
  char    AName[MMAX_NAME + 1];
 
  const char   *ptr;
 
  mgcred_t *A;
 
  long    CkTime;
 
  mxml_t *E = NULL;

  const char *FName = "MAcctLoadCP";
 
  MDB(4,fCKPT) MLog("%s(AS,%s)\n",
    FName,
    (Buf != NULL) ? Buf : "NULL");
 
  if (Buf == NULL)
    {
    return(FAILURE);
    }
 
  /* FORMAT:  <HEADER> <GID> <CKTIME> <GSTRING> */
 
  /* load CP header */
 
  sscanf(Buf,"%64s %64s %ld",
    tmpHeader,
    AName,
    &CkTime);
 
  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime) 
    {
    return(SUCCESS);
    }
 
  if (AS == NULL)
    {
    if (MAcctAdd(AName,&A) != SUCCESS)
      {
      MDB(5,fCKPT) MLog("ALERT:    cannot load CP account '%s'\n",
        AName);
 
      return(FAILURE);
      }
    }
  else
    {
    A = AS;
    }

  if (A->CPLoaded == TRUE)
    {
    return(SUCCESS);
    }

  if ((ptr = strchr(Buf,'<')) == NULL)
    {
    return(FAILURE);
    }
 
  MXMLFromString(&E,ptr,NULL,NULL);
 
  MOFromXML((void *)A,mxoAcct,E,NULL);

  A->CPLoaded = TRUE;
 
  MXMLDestroyE(&E);
 
  return(SUCCESS);
  }  /* END MAcctLoadCP() */ 
 

 
 
 
/**
 * Add new account to MAcct table.
 *
 * @param AName (I)
 * @param AP    (O) [optional]
 */

int MAcctAdd(
 
  const char *AName,
  mgcred_t  **AP)
 
  {
  int   Key;
 
  mgcred_t *A;

  const char *FName = "MAcctAdd";

  MDB(5,fSTRUCT) MLog("%s(%s,AP)\n",
    FName,
    (AName != NULL) ? AName : "NULL");
 
  if (AP != NULL)
    *AP = NULL;

  if ((AName == NULL) || (AName[0] == '\0'))
    {
    return(FAILURE);
    }

  /* check for valid account name */

  if (MUStrChr(AName,',') != NULL)
    {
    MDB(5,fSTRUCT) MLog("INFO:     invalid account name: %s\n",
        AName);

    return(FAILURE);
    }

  /* Check to see if account already exists */   

  if (MAcctFind(AName,&A) == SUCCESS)
    {
    if (AP != NULL)
      *AP = A;
 
    return(SUCCESS);
    }
 
  /* setup new record */

  if ((A = (mgcred_t *)MUCalloc(1,sizeof(mgcred_t))) == NULL)
    {
    MDB(1,fALL) MLog("ERROR:    cannot alloc memory for account '%s'\n",AName);
 
    return(FAILURE);
    }

  Key = (int)(MUGetHash(AName) % MMAX_CREDS);
 
  if (AP != NULL)
    *AP = A;

  if (!strcmp(AName,NONE))
    A->Key = 0;
  else
    A->Key = Key;
 
  MUStrCpy(A->Name,AName,sizeof(A->Name));
 
  MUHTAdd(&MAcctHT,AName,A,NULL,NULL);

  A->Stat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
  MPUInitialize(&A->L.ActivePolicy);

  MCredAdjustConfig(mxoAcct,A,TRUE);

  if (strcmp(AName,ALL) && 
      strcmp(AName,NONE) && 
      strcmp(AName,DEFAULT) && 
      strcmp(AName,"ALL") &&
      strcmp(AName,"DEFAULT"))
    {
    /* update account record */

    MCredAdjustConfig(mxoAcct,A,FALSE);

    MCPRestore(mcpAcct,A->Name,(void *)A,NULL);

    MDB(5,fSTRUCT) MLog("INFO:     account %s added\n",
      AName);
    }

  /* NOTE:  accounts not destroyed */

  MSched.A[mxoAcct]++;

  return(SUCCESS);
  }  /* END MAcctAdd() */





/**
 * find account in MAcct table by account name
 *
 * @param AName (I)
 * @param AP (O)
 */

int MAcctFind(

  const char  *AName,
  mgcred_t   **AP)

  {
  /* If found, return success with AP pointing to Account.     */
  /* If not found, return failure with AP pointing to          */
  /* first free Account if available, AP set to NULL otherwise */

  const char *FName = "MAcctFind";

  MDB(5,fSTRUCT) MLog("%s(%s,AP)\n",
    FName,
    (AName == NULL) ? "NULL" : AName);

  /* Get data from hash table & return results */

  if ((AName != NULL) && (AName[0] != '\0') && (AP != NULL))
    return MUHTGet(&MAcctHT,AName,(void**)AP,NULL); 
 
  return(FAILURE);
  }  /* END MAcctFind() */




/**
 * Convert account to XML.
 *
 * @see MAcctShow() - peer
 *
 * @param A (I)
 * @param EP (O) [alloc] 
 * @param SAList (I) [optional,terminated w/mcaNONE]
 */

int MAcctToXML(
 
  mgcred_t            *A,       /* I */
  mxml_t             **EP,      /* O (alloc) */
  enum MCredAttrEnum  *SAList)  /* I (optional,terminated w/mcaNONE) */
 
  {
  enum MCredAttrEnum DAList[] = {
    mcaID,
    mcaNONE };
 
  int  aindex;
 
  enum MCredAttrEnum *AList;
 
  if ((A == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }
 
  if (SAList != NULL)
    AList = SAList;
  else
    AList = DAList; 

  MXMLCreateE(EP,(char *)MXO[mxoAcct]);         
 
  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mcaNONE;aindex++)
    {
    tmpString = "";

    if ((MCredAToMString((void *)A,mxoAcct,AList[aindex],&tmpString,mfmNONE,0) == FAILURE) ||
        (MUStrIsEmpty(tmpString.c_str())))
      {
      continue;
      }
 
    MXMLSetAttr(*EP,(char *)MCredAttr[AList[aindex]],tmpString.c_str(),mdfString);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MAcctToXML() */




/**
 * process account-specific configuration attributes
 *
 * @param A     (I)
 * @param Value (I)
 * @param IsPar (I)
 */

int MAcctProcessConfig(

  mgcred_t *A,     /* I */
  char     *Value, /* I */
  mbool_t   IsPar) /* I */

  {
  mbitmap_t ValidBM;

  char *ptr;
  char *TokPtr;

  int   aindex;

  char  ValLine[MMAX_LINE];
  char  tmpLine[MMAX_LINE];

  mbool_t AttrIsValid = MBNOTSET;

  const char *FName = "MAcctProcessConfig";

  MDB(5,fCONFIG) MLog("%s(%s,%s)\n",
    FName,
    (A != NULL) ? A->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((A == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  if (IsPar == TRUE)
    {
    MParSetupRestrictedCfgBMs(NULL,NULL,NULL,NULL,NULL,&ValidBM,NULL);
    }

  /* process value line */

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    if (MUGetPair(
          ptr,
          (const char **)MAcctAttr,
          &aindex,  /* O */
          NULL,
          TRUE,
          NULL,
          ValLine,  /* O */
          sizeof(ValLine)) == FAILURE)
      {
      mmb_t *M;

      /* cannot parse value pair */

      if (AttrIsValid == MBNOTSET)
        AttrIsValid = FALSE;

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%.128s' specified",
        ptr);

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for account %s\n",
        ptr,
        A->Name);

      if (MMBAdd(&A->MB,tmpLine,NULL,mmbtNONE,0,0,&M) == SUCCESS)
        {
        M->Priority = -1;  /* annotation, do not checkpoint */
        }

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    if ((IsPar == TRUE) && (!bmisset(&ValidBM,aindex)))
      {
      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    AttrIsValid = TRUE;

    switch (aindex)
      {
      case maaVDef:

        MUStrDup(&A->VDef,ValLine);

        break;

      default:

        /* not handled */

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (AIndex) */

    ptr = MUStrTok(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  bmclear(&ValidBM);

  if (AttrIsValid == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MAcctProcessConfig() */





/**
 * Report account state in human-readable form.
 *
 * NOTE:  process 'mdiag -a' requst.
 *
 * @see MUICredDiagnose() - parent
 * @see MAcctToXML() - peer
 *
 * @param A         (I)
 * @param Buffer    (O) [optional]
 * @param DFlagBM   (I) [bitmap of enum mcm*]
 * @param Partition (I) [optional]
 */

int MAcctShow(

  mgcred_t  *A,
  mstring_t *Buffer,
  mbool_t    Verbose,
  char      *Partition)

  {
  char QALChar;

  char ALLine[MMAX_LINE];

  mgcred_t *U = NULL;

  mhashiter_t Iter;

  const char *FName = "MAcctShow";

  MDB(3,fUI) MLog("%s(%s,SBuf,SBufSize)\n",
    FName,
    (A != NULL) ? A->Name : "NULL");

  if (Buffer == NULL)
    {
    return(SUCCESS);
    }

  if (A == NULL)
    {
    /* build header */

    /*                         NAME PRI FLAG QDEF QLST * PLST TRG LIMITS */

    MStringAppendF(Buffer,"%-12s %8s %12s %12s %12s%s %20s %6s %7s\n\n",
      "Name",
      "Priority",
      "Flags",
      "QDef",
      "QOSList",
      "*",
      "PartitionList",
      "Target",
      "Limits");
    }
  else
    {
    char ParLine[MMAX_LINE];
    char QALLine[MMAX_LINE];
    mstring_t FlagLine(MMAX_LINE);

    /* build job info line */

    MJobFlagsToMString(NULL,&A->F.JobFlags,&FlagLine);
    
    if (A->F.QALType == malAND)
      QALChar = '&';
    else if (A->F.QALType == malOnly)
      QALChar = '^';
    else
      QALChar = ' ';

    MCredShowAttrs(
      &A->L,
      &A->L.ActivePolicy,
      A->L.IdlePolicy,
      A->L.OverrideActivePolicy,
      A->L.OverrideIdlePolicy,
      A->L.OverriceJobPolicy,
      A->L.RsvPolicy,
      A->L.RsvActivePolicy,
      &A->F,
      0,
      FALSE,
      TRUE,
      ALLine);

    /*                         NAME PRIO FLAG QDEF QLST * PLST FSTARG LIMITS */

    MStringAppendF(Buffer,"%-12s %8ld %12s %12s %12s%c %20s %6.2f %7s\n",
      A->Name,
      A->F.Priority,
      (!MUStrIsEmpty(FlagLine.c_str())) ? FlagLine.c_str() : "-",
      (A->F.QDef[0]) != NULL ?  (A->F.QDef[0])->Name  : "-",
      (bmisclear(&A->F.QAL)) ? "-" : MQOSBMToString(&A->F.QAL,QALLine),
      QALChar,
      (bmisclear(&A->F.PAL)) ?  "-" : MPALToString(&A->F.PAL,NULL,ParLine),
      A->F.FSTarget,
      (ALLine[0] != '\0') ? ALLine : "-");


    /* Tmp string buffer */
    mstring_t String(MMAX_LINE);

    /* check and display user access */

    MUHTIterInit(&Iter);
    while (MUHTIterate(&MUserHT,NULL,(void **)&U,NULL,&Iter) == SUCCESS)
      {
      if (!strcmp(U->Name,ALL) || 
          !strcmp(U->Name,NONE) || 
          !strcmp(U->Name,DEFAULT) || 
          !strcmp(U->Name,"ALL") ||
          !strcmp(U->Name,"DEFAULT"))
        {
        continue;
        }

      if (MCredCheckAcctAccess(U,NULL,A) == SUCCESS)
        {
        MStringAppendF(&String,"%s%s",
          (!String.empty()) ? "," : "",
          U->Name);
        }
      }  /* END for (index) */
 
    if (!String.empty()) 
       {
       MStringAppendF(Buffer,"  Users:    %s\n",
         String.c_str());
       }

    if (A->F.GDef != NULL)
      {
      MStringAppendF(Buffer,"  GDEF=%s\n",
        A->F.GDef->Name);
      }

    if (A->F.PDef != NULL)
      {
      MStringAppendF(Buffer,"  PDEF=%s\n",
        A->F.PDef->Name);
      }

    if (A->F.CDef != NULL)
      {
      MStringAppendF(Buffer,"  CDEF=%s\n",
        A->F.CDef->Name);
      }

    if ((A->F.ManagerU[0] != NULL) && 
        (A->F.ManagerU[0]->Name[0] != '\0'))
      {
      int index = 1;

      MStringAppendF(Buffer,"  Managers=%s",
          A->F.ManagerU[0]->Name);

      while ((A->F.ManagerU[index] != NULL) && 
             (A->F.ManagerU[index]->Name[0] != '\0'))
        {
        MStringAppendF(Buffer,",%s",A->F.ManagerU[index]->Name);
 
        index++;
        }

      MStringAppendF(Buffer,"\n");
      }

    if (A->FSCWeight != 0)
      {
      MStringAppendF(Buffer,"  FSWEIGHT=%ld\n",
        A->FSCWeight);
      }

    if (A->DoHold != MBNOTSET)
      {
      MStringAppendF(Buffer,"  HOLD=%s\n",
        MBool[A->DoHold]);
      }

    if (A->VDef != NULL)
      {
      MStringAppendF(Buffer,"  VDEF=%s\n",
        A->VDef);
      }

    if (Verbose == TRUE)
      {
      if (A->Stat.QOSSatJC > 0)
        {
        MStringAppendF(Buffer,"  QOSRate=%.2f%c\n",
          (double)A->Stat.QOSSatJC / MAX(1,A->Stat.JC) * 100.0,
          '%');
        }
      }

    if (A->F.ReqRsv != NULL)
      {
      MStringAppendF(Buffer,"  NOTE:  account %s is tied to rsv '%s'\n",
        A->Name,
        A->F.ReqRsv);
      }

    if (A->ChargeRate != 0.0)
      {
      MStringAppendF(Buffer,"  ChargeRate: %.2f\n",
        A->ChargeRate);
      }

    if (A->Variables != NULL)
      {
      /* display variables */

      MStringAppendF(Buffer,"  VARIABLE=%s\n",
        A->Variables->Name);
      MStringAppendF(Buffer,"\n");
      }

    if (A->MB != NULL)
      {
      char tmpBuf[MMAX_BUFFER];

      /* display messages */

      tmpBuf[0] = '\0';

      MMBPrintMessages(
        A->MB,
        mfmHuman,
        (Verbose == TRUE) ? TRUE : FALSE,
        -1,
        tmpBuf,
        sizeof(tmpBuf));

      MStringAppendF(Buffer,tmpBuf);
      }
    }    /* END else (A == NULL) */

  return(SUCCESS);
  }      /* END MAcctShow() */

/* END MAcct.c */

