/* HEADER */

/**
 * @file MGroup.c
 *
 * Moab Groups
 */
 
/* Contains:                                         *
 *  int MGroupLoadCP(GS,Buf)                         *
 *  int MGroupAdd(GP,GName)                          *
 *  int MGroupFind(G,GroupName)                      *
 *                                                   */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  



/**
 *
 *
 * @param GS (I)
 * @param Buf (O) [minsize=MMAX_BUFFER]
 */

int MGroupLoadCP(
 
  mgcred_t *GS,  /* I */
  const char     *Buf) /* O (minsize=MMAX_BUFFER) */
 
  {
  char    tmpHeader[MMAX_NAME + 1];
  char    GName[MMAX_NAME + 1];
 
  const char   *ptr;
 
  mgcred_t *G;
 
  long    CkTime = 0;
 
  mxml_t *E = NULL;

  const char *FName = "MGroupLoadCP";
 
  MDB(4,fCKPT) MLog("%s(GS,%s)\n",
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
    GName,
    &CkTime);
 
  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }
 
  if (GS == NULL)
    {
    if (MGroupAdd(GName,&G) != SUCCESS)
      {
      MDB(5,fCKPT) MLog("ALERT:    cannot load CP group '%s'\n", 
        GName);
 
      return(FAILURE);
      }
    }
  else
    {
    G = GS;
    }
 
  if ((ptr = strchr(Buf,'<')) == NULL)
    {
    return(FAILURE);
    }
 
  MXMLFromString(&E,ptr,NULL,NULL);
 
  MOFromXML((void *)G,mxoGroup,E,NULL);
 
  MXMLDestroyE(&E);
 
  return(SUCCESS);
  }  /* END MGroupLoadCP() */




/**
 *
 *
 * @param GName (I)
 * @param GP (O) [optional]
 */

int MGroupAdd(
 
  const char  *GName,
  mgcred_t   **GP)
 
  {
  mgcred_t      *G;
  unsigned long hashkey;
 
  const char *FName = "MGroupAdd";
 
  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (GName != NULL) ? GName : "NULL",
    (GP != NULL) ? "GP" : "NULL");

  /* Validate parameters */

  if ((GName == NULL) || (GName[0] == '\0') || (GP == NULL))
    return(FAILURE);
 
  /* Find existing group */

  if (MGroupFind(GName,GP) == SUCCESS)
    return(SUCCESS);

  /* Create group */

  if ((G = (mgcred_t *)MUCalloc(1,sizeof(mgcred_t))) == NULL)
    {
    *GP = NULL;

    MDB(1,fALL) MLog("ERROR:    cannot alloc memory for group '%s'\n",GName);
 
    return(FAILURE);
    }

  MUHTAdd(&MGroupHT,GName,G,NULL,NULL);
  hashkey = MAX(1,MUGetHash(GName) % MMAX_CREDS);

  *GP = G;
 
  G->Key = hashkey;

  MUStrCpy(G->Name,GName,sizeof(G->Name));

  MCredAdjustConfig(mxoGroup,G,TRUE);

  G->Stat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

  MPUInitialize(&G->L.ActivePolicy);

  if (strcmp(GName,ALL) && 
      strcmp(GName,NONE) && 
      strcmp(GName,DEFAULT) && 
      strcmp(GName,"ALL") &&
      strcmp(GName,"NOGROUP") &&
      strcmp(GName,"DEFAULT"))
    {
    /* update group record */

    MCredAdjustConfig(mxoGroup,G,FALSE);

    MCPRestore(mcpGroup,GName,(void *)G,NULL);

    G->OID = MUGIDFromName(G->Name);

    MDB(5,fSTRUCT) MLog("INFO:     group %s added\n",GName);
    }

  MSched.A[mxoGroup]++;

  return(SUCCESS);
  }  /* END MGroupAdd() */

/**
 *
 *
 * @param GName (I)
 * @param GP (O)
 */

int MGroupFind(

  const char  *GName,  /* I */
  mgcred_t   **GP)     /* O */

  {

  const char *FName = "MGroupFind";

  MDB(7,fSTRUCT) MLog("%s(%s,G)\n",
    FName,
    GName);

  /* Get the group from the hash table */

  if ((GName != NULL) && (GName[0] != '\0'))
    return MUHTGet(&MGroupHT,GName,(void**)GP,NULL); 
 
  return(FAILURE);
  }  /* END MGroupFind() */





int MGroupShow(  

  mgcred_t  *G,         /* I */
  mstring_t *Buffer,    /* O (optional) */
  mbool_t    Verbose,   /* I */
  char      *Partition) /* I (optional) */

  {
  char QALLine[MMAX_LINE];
  char QALChar;

  char GLLine[MMAX_LINE];

  char tmpLine[MMAX_LINE];

  const char *FName = "MGroupShow";

  MDB(3,fUI) MLog("%s(%s,Buf,BufSize)\n",
    FName,
    (G != NULL) ? G->Name : "NULL");

  if (Buffer == NULL)
    {
    return(FAILURE);
    }

  if (G == NULL)
    {
    /* build header */

    /*            NAME PRI FLAG QDEF QLST * PLST TRG LIMITS */

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
    mstring_t FlagLine(MMAX_LINE);

    /* build job info line */

    MJobFlagsToMString(NULL,&G->F.JobFlags,&FlagLine);

    MQOSBMToString(&G->F.QAL,QALLine);

    if (G->F.QALType == malAND)
      QALChar = '&';
    else if (G->F.QALType == malOnly)
      QALChar = '^';
    else
      QALChar = ' ';

    MCredShowAttrs(
      &G->L,
      &G->L.ActivePolicy,
      G->L.IdlePolicy,
      G->L.OverrideActivePolicy,
      G->L.OverrideIdlePolicy,
      G->L.OverriceJobPolicy,
      G->L.RsvPolicy,
      G->L.RsvActivePolicy,
      &G->F,
      0,
      FALSE,
      TRUE,
      GLLine);

    /*            NAME PRIO FLAG QDEF QLST * PLST FSTARG LIMITS */

    MStringAppendF(Buffer,"%-12s %8ld %12s %12s %12s%c %20s %6.2f %7s\n",
      G->Name,
      G->F.Priority,
      (FlagLine.empty()) ? "-" : FlagLine.c_str(),
      (G->F.QDef[0]) != NULL ?  (G->F.QDef[0])->Name  : "-",
      (QALLine[0] != '\0') ? QALLine : "-",
      QALChar,
      (bmisclear(&G->F.PAL)) ?  "-" : MPALToString(&G->F.PAL,NULL,tmpLine),
      G->F.FSTarget,
      (GLLine[0] != '\0') ? GLLine : "-");

    /* add group attributes */

    if (G->F.FSCap != 0.0)
      {
      MStringAppendF(Buffer,"  FSCAP=%f\n",
        G->F.FSCap);
      }

    if (G->L.ClassActivePolicy != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      while (MUHTIterate(G->L.ClassActivePolicy,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        if (P->SLimit[mptMaxProc][0] > 0)
          {
          MStringAppendF(Buffer,"  MAXPROC[CLASS:%s]=%d,%d\n",
            MOGetName(P->Ptr,mxoClass,NULL),
            P->SLimit[mptMaxProc][0],
            P->HLimit[mptMaxProc][0]);
          }
        }  /* while (MUHTIterate()) */
      }    /* END if (G->L.APC != NULL) */

    if (G->L.QOSActivePolicy != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      while (MUHTIterate(G->L.QOSActivePolicy,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        if (P->SLimit[mptMaxProc][0] > 0)
          {
          MStringAppendF(Buffer,"  MAXPROC[QOS:%s]=%d,%d\n",
            MOGetName(P->Ptr,mxoClass,NULL),
            P->SLimit[mptMaxProc][0],
            P->HLimit[mptMaxProc][0]);
          }
        }  /* while (MUHTIterate()) */
      }    /* END if (G->L.APQ != NULL) */

    if (G->F.ReqRsv != NULL)
      {
      MStringAppendF(Buffer,"  NOTE:  group %s is tied to rsv '%s'\n",
        G->Name,
        G->F.ReqRsv);
      }

    if (G->Variables != NULL)
      {
      /* display variables */

      MStringAppendF(Buffer,"  VARIABLE=%s\n",
        G->Variables->Name);
      MStringAppendF(Buffer,"\n");
      }

    if (G->MB != NULL)
      {
      char tmpBuf[MMAX_BUFFER];

      /* display messages */

      tmpBuf[0] = '\0';

      MMBPrintMessages(
        G->MB,
        mfmHuman,
        FALSE,
        -1,
        tmpBuf,
        sizeof(tmpBuf));

      MStringAppend(Buffer,tmpBuf);
      }

    if (Verbose == TRUE)
      {
      mstring_t tmpString(MMAX_LINE);

      /* display additional attributes */

      MCredConfigAShow(
        (void *)G,
        mxoGroup,
        NULL,
        TRUE,
        -1,
        &tmpString);

      if (!tmpString.empty())
        {
        /* add "  %s\n" */
        *Buffer += "  ";
        *Buffer += tmpString;
        *Buffer += '\n';
        }

      MStringAppendF(Buffer,"  GID=%ld\n",
        G->OID);
      }    /* END if (Mode == Verbose) */
    }      /* END else (G == NULL) */

  return(SUCCESS);
  }  /* END MGroupShow() */




/**
 * process group-specific configuration attributes
 *
 * @param G (I)
 * @param Value (I)
 * @param IsPar (I)
 */

int MGroupProcessConfig(

  mgcred_t *G,
  char     *Value,
  mbool_t   IsPar)

  {
  mbitmap_t ValidBM;

  char *ptr;
  char *TokPtr;

  int   aindex;

  char  ValLine[MMAX_LINE];
  char  tmpLine[MMAX_LINE];

  mbool_t AttrIsValid = MBNOTSET;

  const char *FName = "MGroupProcessConfig";

  MDB(5,fCONFIG) MLog("%s(%s,%s)\n",
    FName,
    (G != NULL) ? G->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((G == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  /* process value line */

  if (IsPar == TRUE)
    {
    MParSetupRestrictedCfgBMs(NULL,NULL,NULL,&ValidBM,NULL,NULL,NULL);
    }

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    if (MUGetPair(
          ptr,
          (const char **)MGroupAttr,
          &aindex, /* O */
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

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for group %s\n",
        ptr,
        G->Name);

      if (MMBAdd(&G->MB,tmpLine,NULL,mmbtNONE,0,0,&M) == SUCCESS)
        {
        M->Priority = -1;  /* annotation, do not checkpoint */
        }

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
      case mgaClassWeight:

        G->ClassSWeight = strtol(ValLine,NULL,10);

        break;

      default:

        /* not handled */

        bmclear(&ValidBM);

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
  }  /* END MGroupProcessConfig() */

/* END MGroup.c */

