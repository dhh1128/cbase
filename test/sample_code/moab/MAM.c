/* HEADER */

/**
 * @file MAM.c
 *
 * Moab Accounting Manager
 */

/* contains:                                    *
 *                                              *
 * int MAMInitialize(AS)                        *
 * int MIDLoadCred(I,IDBufP,SOType,SO)          *
 *                                              */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"
#include "mcom-proto.h"


#ifdef __MLDAP
#include "ldap.h"
#endif /* __MLDAP */
 


#define MAM_FILEFOOTER "BANKEND\n"

/* Globals */

mbool_t MAMTestMode;

const char *MAMCommands[] = {
  NONE,
  "make_withdrawal",
  NULL };

enum MAMCmdTypes {
  mamcNone = 0,
  mamcJobDebitAlloc };
 

typedef struct
  {
  char *Attr;
  char  Value[MMAX_LINE];
  } mgavplist_t;


/* internal prototypes */

int __MAMStartFunc(mam_t *,int);
int __MAMEndFunc(mam_t *,int);
int MAMGoldDoCommand(mxml_t *,mpsi_t *,char **,enum MS3CodeDecadeEnum *,char *);
int __MAMOAddXMLChild(mxml_t *,const char *,void *,enum MDataFormatEnum);
int __MAMOAddXMLVariables(mxml_t *,mln_t *);
int __MAMOAddXMLVariablesFromHT(mxml_t *,mhash_t *);
int __MAMOAddXMLGRes(mxml_t *,msnl_t *);

/* END internal prototypes */





/**
 * Initialize accounting manager interface.
 *
 * @see MAMPing() - child
 * @see MSysStartServer() - parent
 *
 * @param AS (I) [modified]
 */

int MAMInitialize(

  mam_t *AS)  /* I (modified) */

  {
  int aindex;

  mam_t *A;

  const char *FName = "MAMInitialize";

  MDB(3,fAM) MLog("%s(%s)\n",
    FName,
    (AS != NULL) ? AS->Name : "NULL");

  if (MSched.Mode != msmNormal)
    {
    return(SUCCESS);
    }

  /* NOTE:  must distinguish between multiple AM's */

  for (aindex = 0;aindex < MMAX_AM;aindex++)
    {
    A = &MAM[aindex];

    if (A->Type == mamtNONE) 
      continue;

    if ((AS != NULL) && (A != AS))
      continue;

    if (A->State != mrmsNONE)
      {
      MAMShutdown(A);
      }

    /* determine AM version */

    switch (A->Type)
      {
      case mamtNative:

        MSched.NativeAM = TRUE;

        A->State = mrmsActive;

        break;

      case mamtMAM:
      case mamtGOLD:

        {
        /* dynamic version detection not supported */

        /* version can be specified via config file */

        A->P.SocketProtocol = mspHTTP;
        A->P.WireProtocol   = mwpS32;

        A->P.HostName       = A->Host;
        A->P.Port           = A->Port;

        A->P.CSAlgo         = A->CSAlgo;
        A->P.CSKey          = A->CSKey;

        A->P.Timeout        = A->Timeout;

        A->P.Type           = mpstAM;

        A->P.TimeStampIsOptional = TRUE;

        /* create socket */

        MSUCreate(&A->P.S);

        /* no initialization phase */

        if (MAMPing(A,NULL) == SUCCESS)
          {
          A->State = mrmsActive;

          if (A->CreateCred == TRUE)
            {
            MAMAccountGetDefault(NULL,NULL,NULL);
            }
          }
        else
          {
          A->State = mrmsDown;
          }
        }  /* END BLOCK */

        break;

      case mamtFILE:

        A->State = mrmsActive;

        if (A->ChargePolicy == mamcpNONE)
          A->ChargePolicy = mamcpDebitSuccessfulWC;

        break;

      default:

        break;
      }  /* END switch (A->Type) */
    }      /* END for (aindex) */

  return(SUCCESS);
  }  /* END MAMInitialize() */





/**
 * Send 'are you alive?' ping message to accounting manager.
 *
 * NOTE:  sends Gold-style pings only
 *
 * @see MS3DoCommand() - child
 *
 * @param A (I)
 * @param SC (O) [optional]
 */

int MAMPing(

  mam_t                *A,   /* I */
  enum MStatusCodeEnum *SC)  /* O (optional) */

  {
  int     rc;
  static  mxml_t *ReqE = NULL;
  static  mxml_t *DE   = NULL;

  mxml_t *tSE;
  mxml_t *tVE;

  char    EMsg[MMAX_LINE];

  char    tmpLine[MMAX_LINE];

  if (SC != NULL)
    *SC = mscNoError;

  if (A == NULL)
    {
    return(FAILURE);
    }

  /* verify accounting manager is alive */

  /* FORMAT:  <Request action="Query" actor="adaptive" chunking="true"><Object>System</Object><Get name="Version"></Get></Request>

<Response actor="adaptive"><Status><Value>Success</Value><Code>000</Code></Status><Count>1</Count><Data><System><Version>2.0.b1.0</Version></System></Data></Response>
  */

  MXMLCreateE(&ReqE,(char*)MSON[msonRequest]);

  MXMLSetAttr(ReqE,MSAN[msanAction],(void **)"Query",mdfString);

  MS3SetObject(ReqE,"System",NULL);

  MS3AddGet(ReqE,"Version",NULL);

  if (MXMLToString(ReqE,tmpLine,sizeof(tmpLine),NULL,TRUE) == FAILURE)
    {
    return(FAILURE);
    }

  MXMLDestroyE(&ReqE);

  rc = MS3DoCommand(
     &A->P,
     tmpLine,
     NULL,
     &DE,   /* O */
     NULL,
     EMsg); /* O */

  if (rc == FAILURE)
    {
    if (!strcasecmp(EMsg,"timestamp"))
      {
      MMBAdd(&A->MB,"cannot authenticate request - check key and authentication algorithm",NULL,mmbtNONE,0,0,NULL);
      }
    else
      {
      MMBAdd(&A->MB,EMsg,NULL,mmbtNONE,0,0,NULL);
      }
    }

  if (DE == NULL)
    {
    MSUFree(A->P.S);

    return(FAILURE);
    }

  if ((MXMLGetChild(DE,"System",NULL,&tSE) == FAILURE) ||
      (MXMLGetChild(tSE,"Version",NULL,&tVE) == FAILURE))
    {
    MSUFree(A->P.S);

    return(FAILURE);
    }

  /* SUCCESS:  version reported */

  MSUFree(A->P.S);

  return(SUCCESS);
  }  /* END MAMPing() */



 
 
/**
 * Add new accounting manager interface.
 *
 * @see MAMSetDefaults() - child
 *
 * @param AMName (I)
 * @param AP (O)
 */

int MAMAdd(
 
  char   *AMName,  /* I */
  mam_t **AP)      /* O */
 
  {
  int     amindex;
 
  mam_t *A; 

  const char *FName = "MAMAdd";
 
  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (AMName != NULL) ? "AMName" : "NULL",
    (AP != NULL) ? "AP" : "NULL");
 
  if ((AMName == NULL) || (AMName[0] == '\0'))
    {
    return(FAILURE);
    }
 
  if (AP != NULL)
    *AP = NULL;
 
  for (amindex = 0;amindex < MMAX_AM;amindex++)
    {
    A = &MAM[amindex];
 
    if ((A != NULL) && !strcmp(A->Name,AMName))
      {
      /* AM already exists */
 
      if (AP != NULL)
        *AP = A;
 
      return(SUCCESS);
      }
 
    if ((A != NULL) &&
        (A->Name[0] != '\0') &&
        (A->Name[0] != '\1'))
      {
      continue;
      }
 
    /* empty slot found */
 
    /* create/initialize new record */
 
    if (A == NULL)
      {
      A = &MAM[amindex];

      if (MAMCreate(AMName,&A) == FAILURE)
        {
        MDB(1,fALL) MLog("ERROR:    cannot alloc memory for AM '%s'\n",
          AMName);
 
        return(FAILURE); 
        }
      }
    else if (A->Name[0] == '\0')
      {
      MUStrCpy(A->Name,AMName,sizeof(A->Name));
      }

    MAMSetDefaults(A);

    MOLoadPvtConfig((void **)A,mxoAM,A->Name,NULL,NULL,NULL);

    if (AP != NULL)
      *AP = A;
 
    A->Index = amindex;
 
    /* update AM record */
 
    MCPRestore(mcpAM,AMName,(void *)A,NULL);
 
    MDB(5,fSTRUCT) MLog("INFO:     AM %s added\n",
      AMName);
 
    return(SUCCESS);
    }    /* END for (amindex) */
 
  /* end of table reached */
 
  MDB(1,fSTRUCT) MLog("ALERT:    AM table overflow.  cannot add %s\n",
    AMName);
 
  return(SUCCESS);
  }  /* END MAMAdd() */
 
 
 

 
/**
 * Create new accounting manager object 
 *
 * @see MAMInitialize()
 * @see MAMAdd()
 *
 * @param AMName (I)
 * @param AP (O)
 */

int MAMCreate(
 
  char   *AMName,  /* I */
  mam_t **AP)      /* O */
 
  {
  mam_t *A;

  if (AP == NULL)
    {
    return(FAILURE);
    }

  A = *AP;
 
  /* use static memory for now */ 
 
  memset(A,0,sizeof(mam_t));
 
  if ((AMName != NULL) && (AMName[0] != '\0'))
    MUStrCpy(A->Name,AMName,sizeof(A->Name));
 
  return(SUCCESS);
  }  /* END MAMCreate() */

 
 
 
 
/**
 *
 *
 * @param AMName (I)
 * @param AP (O)
 */

int MAMFind(
 
  char   *AMName,  /* I */
  mam_t **AP)      /* O */
 
  {
  /* if found, return success with AP pointing to AM */
 
  int    amindex;
  mam_t *A;
 
  if (AP != NULL)
    *AP = NULL;
 
  if ((AMName == NULL) ||
      (AMName[0] == '\0'))
    {
    return(FAILURE);
    }
 
  for (amindex = 0;amindex < MMAX_AM;amindex++)
    {
    A = &MAM[amindex];
 
    if ((A == NULL) || (A->Name[0] == '\0') || (A->Name[0] == '\1'))
      {
      break;
      }
 
    if (strcmp(A->Name,AMName) != 0)
      continue; 
 
    /* AM found */
 
    if (AP != NULL)
      *AP = A;
 
    return(SUCCESS);
    }  /* END for (amindex) */
 
  /* entire table searched */
 
  return(FAILURE);
  }  /* END MAMFind() */
 
 
 

 
/**
 * @see MAMInitialize()
 *
 * @param AM (I) [modified]
 */

int MAMSetDefaults(
 
  mam_t *AM) /* I (modified) */
 
  {
  const char *FName = "MAMSetDefaults";

  MDB(1,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL");

  if (AM == NULL)
    {
    return(FAILURE);
    }

  switch (AM->Type)
    {
    case mamtNONE:

      /* set general defaults */

      AM->Port                  = MDEF_AMPORT;   
      strcpy(AM->Host,MDEF_AMHOST); 
 
      AM->Type                  = MDEF_AMTYPE;
      AM->ChargePolicy          = mamcpDebitSuccessfulWC;
      AM->CreateFailureAction   = MDEF_AMCREATEFAILUREACTION;
      AM->CreateFailureNFAction = MDEF_AMCREATEFAILUREACTION;
      AM->ResumeFailureAction   = MDEF_AMRESUMEFAILUREACTION;
      AM->ResumeFailureNFAction = MDEF_AMRESUMEFAILUREACTION;
      AM->StartFailureAction    = MDEF_AMSTARTFAILUREACTION;
      AM->StartFailureNFAction  = MDEF_AMSTARTFAILUREACTION;
      AM->UpdateFailureAction   = MDEF_AMUPDATEFAILUREACTION;
      AM->UpdateFailureNFAction = MDEF_AMUPDATEFAILUREACTION;
      AM->ValidateJobSubmission = FALSE;
      AM->FlushTime             = 0;
      AM->FlushInterval         = MDEF_AMFLUSHINTERVAL;  
      AM->Timeout               = MDEF_AMTIMEOUT * MDEF_USPERSECOND; /* in microseconds */
 
      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case mamtMAM:
    case mamtGOLD:

      if (AM->WireProtocol == mwpNONE)
        AM->WireProtocol = mwpXML;

      if (AM->SocketProtocol == mspNONE)
        AM->SocketProtocol = mspHalfSocket;

      if (AM->Port <= 0)
        AM->Port = MDEF_AMGOLDPORT;

/*    
      NOTE:  if no ALGO specified disable authentication

      if (AM->CSAlgo == mcsaNONE)
        AM->CSAlgo = mcsaHMAC;
*/

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AM->Type) */
 
  return(SUCCESS);
  }  /* END MAMSetDefaults() */

 
 
 
 
/**
 * Remove/free accounting manager object.
 *
 * @param A (I) [modified]
 */

int MAMDestroy(
 
  mam_t **A)  /* I (modified) */
 
  {
  if (A == NULL)
    {
    return(SUCCESS);
    }
 
  if (*A == NULL)
    {
    return(SUCCESS);
    }
 
  memset(*A,0,sizeof(mam_t));
 
  return(SUCCESS);
  }  /* END MAMDestroy() */ 

 
 
 
 
/**
 * Report accounting manager state/configuration.
 *
 * NOTE:  process 'mdiag -R -v' for accounting manager output.
 *
 * @param AS (I)
 * @param RE (O)
 * @param String (O)
 * @param DFormat (I)
 */

int MAMShow(
 
  mam_t     *AS,
  mxml_t    *RE,
  mstring_t *String,
  enum MFormatModeEnum DFormat)
 
  {
  int   amindex;

  int   index;
  int   findex;

  mbool_t FailureRecorded;

  mbool_t ShowHeader = TRUE;

  char tmpLine[MMAX_LINE];
 
  mam_t  *AM;
  mpsi_t *P;

  mxml_t *DE = NULL;
  mxml_t *AE;
  mxml_t *PE;
 
  if (DFormat == mfmXML)
    {
    if (RE == NULL)
      {
      return(FAILURE);
      }

    DE = RE;
    }
  else
    {
    if (String == NULL)
      {
      return(FAILURE);
      }

    MStringSet(String,"\0");
    }

  /* NOTE:  allow mode to specify verbose, diagnostics, etc */
 
  for (amindex = 0;amindex < MMAX_AM;amindex++)
    {
    AM = &MAM[amindex];

    if (AM->Name[0] == '\0')
      break;
 
    if ((AS != NULL) && (AS != AM))
      continue; 

    P = &AM->P;
 
    if (ShowHeader == TRUE)
      {
      ShowHeader = FALSE;
      }

    tmpLine[0] = '\0';

    if (DFormat == mfmXML)
      {
      AE = NULL;

      MXMLCreateE(&AE,(char *)MXO[mxoAM]);

      MXMLAddE(DE,AE);

      MXMLSetAttr(AE,(char *)MAMAttr[mamaName],(void *)AM->Name,mdfString);
      MXMLSetAttr(AE,(char *)MAMAttr[mamaType],(void *)MAMType[AM->Type],mdfString);

      MXMLSetAttr(AE,(char *)MAMAttr[mamaState],(void *)MAMState[AM->State],mdfString);

      MXMLSetAttr(AE,(char *)MAMAttr[mamaHost],(void *)AM->Host,mdfString);
      MXMLSetAttr(AE,(char *)MAMAttr[mamaPort],(void *)&AM->Port,mdfInt);
      }
    else
      {
      MStringAppendF(String,"AM[%s]  Type: %s  State: '%s'\n",
        AM->Name,
        MAMType[AM->Type],
        MAMState[AM->State]);
      }

    if (AM->Type == mamtNONE)
      {
      /* type not specified, AM is corrupt - show only limited information */

      if (AM->MB != NULL)
        {
        if (DFormat == mfmXML)
          {
          mxml_t *ME = NULL;

          MXMLCreateE(&ME,(char *)MAMAttr[mamaMessages]);

          MMBPrintMessages(AM->MB,mfmXML,TRUE,-1,(char *)ME,0);
          }
        else
          {
          if (AM->MB != NULL)
            {
            char tmpBuf[MMAX_BUFFER];

            MStringAppendF(String,"\n");

            MMBPrintMessages(AM->MB,mfmHuman,FALSE,-1,tmpBuf,sizeof(tmpBuf));

            MStringAppend(String,tmpBuf);
            }
          }
        }

      continue;
      }  /* END if (AM->Type == mamtNONE) */

    if (DFormat == mfmXML)
      {
      if (AM->WireProtocol != 0)
        MXMLSetAttr(AE,(char *)MAMAttr[mamaWireProtocol],(void *)MWireProtocol[AM->WireProtocol],mdfString);

      if (AM->SocketProtocol != 0)
        MXMLSetAttr(AE,(char *)MAMAttr[mamaSocketProtocol],(void *)MSockProtocol[AM->SocketProtocol],mdfString);

      /* show AM policies */

      if (AM->FallbackAccount[0] != '\0')
        {
        MXMLSetAttr(AE,(char *)MAMAttr[mamaFallbackAccount],(void *)AM->FallbackAccount,mdfString);
        }

      if (AM->FallbackQOS[0] != '\0')
        {
        MXMLSetAttr(AE,(char *)MAMAttr[mamaFallbackQOS],(void *)AM->FallbackQOS,mdfString);
        }

      if (AM->CreateFailureAction != mamjfaNONE)
        {
        char tmpVal[MMAX_LINE]={0};
        char tmpNFVal[MMAX_LINE]={0};

        if (AM->CreateFailureNFAction != mamjfaNONE)
          {
          sprintf(tmpNFVal,",%s",MAMJFActionType[AM->CreateFailureNFAction]);
          }

        sprintf(tmpVal,"%s%s",
          MAMJFActionType[AM->CreateFailureAction],
          (tmpNFVal[0] != '\0') ? tmpNFVal : "");

        MXMLSetAttr(AE,(char *)MAMAttr[mamaCreateFailureAction],(void *)tmpVal,mdfString);
        }

      if (AM->StartFailureAction != mamjfaNONE)
        {
        char tmpVal[MMAX_LINE];
        char tmpNFVal[MMAX_LINE];
        tmpVal[0] = '\0';

        if (AM->StartFailureNFAction != mamjfaNONE)
          {
          sprintf(tmpNFVal,",%s",MAMJFActionType[AM->StartFailureNFAction]);
          }

        sprintf(tmpVal,"%s%s",
          MAMJFActionType[AM->StartFailureAction],
          (tmpNFVal[0] != '\0') ? tmpNFVal : "");

        MXMLSetAttr(AE,(char *)MAMAttr[mamaStartFailureAction],(void *)tmpVal,mdfString);
        }

      if (AM->UpdateFailureAction != mamjfaNONE)
        {
        char tmpVal[MMAX_LINE];
        char tmpNFVal[MMAX_LINE];
        tmpVal[0] = '\0';

        if (AM->UpdateFailureNFAction != mamjfaNONE)
          {
          sprintf(tmpNFVal,",%s",MAMJFActionType[AM->UpdateFailureNFAction]);
          }

        sprintf(tmpVal,"%s%s",
          MAMJFActionType[AM->UpdateFailureAction],
          (tmpNFVal[0] != '\0') ? tmpNFVal : "");

        MXMLSetAttr(AE,(char *)MAMAttr[mamaUpdateFailureAction],(void *)tmpVal,mdfString);
        }

/* Comment out until functionally implemented */
#ifdef MNOT
      if (AM->ResumeFailureAction != mamjfaNONE)
        {
        char tmpVal[MMAX_LINE];
        char tmpNFVal[MMAX_LINE];
        tmpVal[0] = '\0';

        if (AM->ResumeFailureNFAction != mamjfaNONE)
          {
          sprintf(tmpNFVal,",%s",MAMJFActionType[AM->ResumeFailureNFAction]);
          }

        sprintf(tmpVal,"%s%s",
          MAMJFActionType[AM->ResumeFailureAction],
          (tmpNFVal[0] != '\0') ? tmpNFVal : "");

        MXMLSetAttr(AE,(char *)MAMAttr[mamaResumeFailureAction],(void *)tmpVal,mdfString);
        }
#endif /* MNOT */

      if (AM->NodeChargePolicy != mamncpNONE)
        {
        MXMLSetAttr(
          AE,
          (char *)MAMAttr[mamaNodeChargePolicy],
          (void *)MAMNodeChargePolicy[AM->NodeChargePolicy],mdfString);
        }

      if (AM->FlushInterval != 0)
        {
        MXMLSetAttr(AE,(char *)MAMAttr[mamaFlushInterval],(void *)&AM->FlushInterval,mdfLong);
        }

      if (AM->ChargePolicy != 0)
        {
        MXMLSetAttr(
          AE,
          (char *)MAMAttr[mamaChargePolicy],
          (void *)MAMChargeMetricPolicy[AM->ChargePolicy],
          mdfString);
        }

      PE = NULL;

      MXMLCreateE(&PE,"psi");

      if (MPSIToXML(&AM->P,PE) == SUCCESS)
        {
        MXMLAddE(AE,PE);
        }
      }  /* END if (DFormat == mfmXML */
    else
      {
      if (AM->Host[0] != '\0')
        {
        switch (AM->Type)
          {
          case mamtFILE:

            MStringAppendF(String,"  Destination:    %s\n",
              AM->Host);

            break;

          default:

            MStringAppendF(String,"  Host:           %s\n",
              AM->Host);

            break;
          }
        }  /* END if (AM->Host != NULL) */

      if ((AM->Type == mamtMAM) || (AM->Type == mamtGOLD))
        {
        if (AM->CSAlgo == mcsaNONE)
          {
          MStringAppendF(String,"  ALERT:  no security algorithm specified (see moab-private.cfg)\n");
          }
        else
          {
          MStringAppendF(String,"  CS Algorithm:   %s\n",
            MCSAlgoType[AM->CSAlgo]);
          }

        if (AM->CSKey[0] == '\0')
          {
          MStringAppendF(String,"  ALERT:  no secret key specified (see moab-private.cfg)\n");
          }
        }    /* END if (AM->Type == mamtMAM) */
      
      if ((AM->WireProtocol != mwpNONE) || (AM->SocketProtocol != mspNONE))
        {
        MStringAppendF(String,"  socketprotocol: %s  wireprotocol: %s\n",
          MSockProtocol[AM->SocketProtocol],
          MWireProtocol[AM->WireProtocol]);
        }

      /* NOTE:  display optional AM version, failures, and fault tolerance config */

      if (AM->CreateCred == TRUE)
        {
        MStringAppendF(String,"  CreateCred:     %s\n",
          MBool[AM->CreateCred]);
        }

      if (AM->ValidateJobSubmission != MBNOTSET)
        {
        MStringAppendF(String,"  ValidateJobSubmission:     %s\n",
          MBool[AM->ValidateJobSubmission]);
        }

      if (!bmisclear(&AM->Flags))
        {
        char tmpLine[MMAX_LINE];

        MStringAppendF(String,"  Flags:         %s\n",
          bmtostring(&AM->Flags,MAMFlags,tmpLine));
        }

      if (AM->FlushPeriod != mpNONE)
        {
        MStringAppendF(String,"  FlushPeriod:    %s\n",
          MCalPeriodType[AM->FlushPeriod]);
        }
      else if (AM->FlushInterval > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(AM->FlushInterval,TString);

        MStringAppendF(String,"  FlushInterval:  %s\n",
          TString);
        }

      if (AM->ChargePolicy != mamcpNONE)
        {
        MStringAppendF(String,"  ChargePolicy:   %s\n",
          MAMChargeMetricPolicy[AM->ChargePolicy]);
        }

      if (MUMemCCmp((char *)AM->IsChargeExempt,0,sizeof(AM->IsChargeExempt)) == FAILURE)
        {
        int oindex;

        mbool_t FirstShown = FALSE;

        MStringAppendF(String,"  ChargeObjects:  ");

        for (oindex = 0;oindex < mxoLAST;oindex++)
          {
          if (AM->IsChargeExempt[oindex] == FALSE)
            {
            MStringAppendF(String,"%s%s",
              (FirstShown == TRUE) ? "," : "",
              MXO[oindex]);

            FirstShown = TRUE;
            }
          }    /* END for (oindex) */

        MStringAppendF(String,"\n");
        }

      if (AM->Type == mamtNative)
        {
        if (MAMAToString(AM,mamaQuoteURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  Quote URL:      %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnQuote] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in QuoteURL (file://)\n");
            }
          }

        if (MAMAToString(AM,mamaCreateURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  Create URL:     %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnCreate] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in CreateURL (file://)\n");
            }
          }

        if (MAMAToString(AM,mamaStartURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  Start URL:      %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnStart] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in StartURL (file://)\n");
            }
          }

        if (MAMAToString(AM,mamaUpdateURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  Update URL:     %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnUpdate] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in UpdateURL (file://)\n");
            }
          }

        if (MAMAToString(AM,mamaPauseURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  Pause URL:      %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnPause] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in PauseURL (file://)\n");
            }
          }

/* Comment out until functionally implemented */
#ifdef MNOT
        if (MAMAToString(AM,mamaResumeURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  Resume URL:     %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnResume] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in ResumeURL (file://)\n");
            }
          }
#endif /* MNOT */

        if (MAMAToString(AM,mamaEndURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  End URL:        %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnEnd] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in EndURL (file://)\n");
            }
          }

        if (MAMAToString(AM,mamaDeleteURL,tmpLine,sizeof(tmpLine),0) == SUCCESS)
          {
          MStringAppendF(String,"  Delete URL:     %s\n",
            tmpLine);

          if (AM->ND.Protocol[mamnDelete] == mbpFile)
            {
            MStringAppend(String,"  WARNING:  invalid protocol specified in DeleteURL (file://)\n");
            }
          }
        }   /* END if (AM->Type == mamtNative) */

      if (AM->ChargeRate != 0.0)
        {
        MStringAppendF(String,"  ChargeRate:     %s%.2lf/%s\n",
          (AM->ChargePolicyIsLocal == TRUE) ? "local:" : "",
          AM->ChargeRate,
          (AM->ChargePeriod != mpNONE) ? MCalPeriodType[AM->ChargePeriod] : MCalPeriodType[AM->FlushPeriod]);
        }

      /* NOTE:  stats in ms */

      if (P->RespTotalCount[0] > 0)
        {
        MStringAppendF(String,"  AM Performance:  Avg Time: %.2fs  Max Time:  %.2fs  (%d samples)\n",
          (double)P->RespTotalTime[0] / P->RespTotalCount[0] / 1000,
          (double)P->RespMaxTime[0] / 1000,
          P->RespTotalCount[0]);
        }

      FailureRecorded = FALSE;

      for (index = 0;index < MMAX_PSFAILURE;index++)
        {
        char DString[MMAX_LINE];

        findex = (index + P->FailIndex) % MMAX_PSFAILURE;

        if (P->FailTime[findex] <= 0)
          continue;

        if (FailureRecorded == FALSE)
          {
          MStringAppendF(String,"\nAM[%s] Failures: \n",
            AM->Name);

          FailureRecorded = TRUE;
          }

        MULToDString((mulong *)&P->FailTime[findex],DString);

        MStringAppendF(String,"  %19.19s  %-15s  '%s'\n",
          DString,
          MAMFuncType[P->FailType[findex]],
          (P->FailMsg[findex] != NULL) ? P->FailMsg[findex] : "no msg");
        }  /* END for (index) */

      if (AM->MB != NULL)
        {
        char tmpBuf[MMAX_BUFFER];

        MStringAppendF(String,"\n");

        MMBPrintMessages(AM->MB,mfmHuman,FALSE,-1,tmpBuf,sizeof(tmpBuf));

        MStringAppend(String,tmpBuf);
        }

      MStringAppendF(String,"\n");
      }  /* END else (DFormat == mfmXML) */
    }    /* END for (amindex) */
 
  return(SUCCESS);
  }  /* END MAMShow() */





/**
 * Report accounting manager configuration.
 *
 * @see MUIConfigShow()
 *
 * @param AS (I)
 * @param Mode (I)
 * @param String (O)
 */

int MAMConfigShow(
 
  mam_t     *AS,
  int        Mode,
  mstring_t *String)
 
  {
  int  amindex;
  int  aindex;

  char tmpVal[MMAX_LINE];

  mam_t *AM;
 
  if (String == NULL)
    {
    return(FAILURE);
    }

  mstring_t tmpString(MMAX_LINE);;

  *String = "";

  /* NOTE:  allow mode to specify verbose, etc */
 
  for (amindex = 0;MAM[amindex].Type != mamtNONE;amindex++)
    {
    AM = &MAM[amindex];
 
    if ((AS != NULL) && (AS != AM))
      continue;
 
    tmpString = "";

    if (AM->Type == mamtNONE)
      continue;

    tmpString += MCredCfgParm[mxoAM];
    tmpString += '[';
    tmpString +=  AM->Name;
    tmpString += ']';

    for (aindex = 0;MAMAttr[aindex] != NULL;aindex++)
      {
      tmpVal[0] = '\0';

      switch (aindex)
        {
        case mamaCreateFailureAction:

          if (AM->CreateFailureAction != mamjfaNONE)
            {
            char tmpLine[MMAX_LINE];

            if (AM->CreateFailureNFAction != mamjfaNONE)
              {
              sprintf(tmpLine,",%s",MAMJFActionType[AM->CreateFailureNFAction]);
              }

            sprintf(tmpVal,"%s%s",
              MAMJFActionType[AM->CreateFailureAction],
              (tmpLine[0] != '\0') ? tmpLine : "");
            }

          break;

        case mamaChargePolicy:

          if (AM->ChargePolicy != 0)
            strcpy(tmpVal,MAMChargeMetricPolicy[AM->ChargePolicy]);

          break;

        case mamaFallbackAccount:

          if (AM->FallbackAccount[0] != '\0')
            strcpy(tmpVal,AM->FallbackAccount);

          break;

        case mamaFallbackQOS:

          if (AM->FallbackQOS[0] != '\0')
            strcpy(tmpVal,AM->FallbackQOS);

          break;

        case mamaFlushInterval:

          if (AM->FlushInterval != 0)
            {
            char TString[MMAX_LINE];

            MULToTString(AM->FlushInterval,TString);
            strcpy(tmpVal,TString);
            }

          break;

        case mamaHost:

          if (AM->Host[0] != '\0')
            strcpy(tmpVal,AM->Host);

          break;

        case mamaPort:

          if (AM->Port != 0)
            sprintf(tmpVal,"%d",AM->Port);

          break;

/* Comment out until functionally implemented */
#ifdef MNOT
        case mamaResumeFailureAction:

          if (AM->ResumeFailureAction != mamjfaNONE)
            {
            char tmpLine[MMAX_LINE];

            if (AM->ResumeFailureNFAction != mamjfaNONE)
              {
              sprintf(tmpLine,",%s",MAMJFActionType[AM->ResumeFailureNFAction]);
              }

            sprintf(tmpVal,"%s%s",
              MAMJFActionType[AM->ResumeFailureAction],
              (tmpLine[0] != '\0') ? tmpLine : "");
            }

          break;
#endif /* MNOT */

        case mamaStartFailureAction:

          if (AM->StartFailureAction != mamjfaNONE)
            {
            char tmpLine[MMAX_LINE];

            if (AM->StartFailureNFAction != mamjfaNONE)
              {
              sprintf(tmpLine,",%s",MAMJFActionType[AM->StartFailureNFAction]);
              }

            sprintf(tmpVal,"%s%s",
              MAMJFActionType[AM->StartFailureAction],
              (tmpLine[0] != '\0') ? tmpLine : "");
            }

          break;

        case mamaTimeout:

          /* timeout is in micro seconds */

          if (AM->Timeout != 0)
            {
            char TString[MMAX_LINE];

            MULToTString(AM->Timeout / MDEF_USPERSECOND,TString);
            strcpy(tmpVal,TString);
            }

          break;
 
        case mamaType:

          if (AM->Type != mamtNONE)
            strcpy(tmpVal,MAMType[AM->Type]);

          break;

        case mamaUpdateFailureAction:

          if (AM->UpdateFailureAction != mamjfaNONE)
            {
            char tmpLine[MMAX_LINE];

            if (AM->UpdateFailureNFAction != mamjfaNONE)
              {
              sprintf(tmpLine,",%s",MAMJFActionType[AM->UpdateFailureNFAction]);
              }

            sprintf(tmpVal,"%s%s",
              MAMJFActionType[AM->UpdateFailureAction],
              (tmpLine[0] != '\0') ? tmpLine : "");
            }

          break;

        case mamaValidateJobSubmission:

          if (AM->ValidateJobSubmission != MBNOTSET)
            {
            strcpy(tmpVal,MBool[AM->ValidateJobSubmission]);
            }

          break;

        default:

          /* not handled */

          break;
        }  /* END switch (aindex) */

      if (tmpVal[0] != '\0')
        {
        /* " %s=%s" */
        tmpString += ' ';
        tmpString += MAMAttr[aindex];
        tmpString += '='; 
        tmpString +=  tmpVal;
        }
      }    /* END for (aindex) */

    *String += tmpString.c_str();
    *String += '\n';
    }  /* END for (amindex) */

  return(SUCCESS);
  }  /* END MAMConfigShow() */





/**
 * Activate accounting manager interface
 *
 * @param A (I)
 */

int MAMActivate(
 
  mam_t *A)  /* I */
 
  {
  char FileName[MMAX_PATH_LEN];
  char PathName[MMAX_PATH_LEN];

  const char *FName = "MAMActivate";
 
  MDB(2,fAM) MLog("%s(%s)\n",
    FName,
    (A != NULL) ? A->Name : "NULL");
 
  if ((A == NULL) || (A->Type == mamtNONE))
    {
    return(SUCCESS);
    }
 
  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    return(SUCCESS);
    }
 
  switch (A->Type)
    {
    case mamtMAM:
    case mamtGOLD:

      /* NO-OP */

      break;
 
    case mamtFILE:
 
      /* open transaction log file */ 

      if (A->Host[0] == '\0')
        {
        strcpy(FileName,"am.log");
        }
      else
        {
        strcpy(FileName,A->Host);
        }
 
      if ((FileName[0] != '/') && (FileName[0] != '~'))
        {
        if (MSched.VarDir[strlen(MSched.VarDir) - 1] == '/')
          {
          sprintf(PathName,"%s%s",
            MSched.VarDir,
            FileName);
          }
        else
          {
          sprintf(PathName,"%s/%s",
            MSched.VarDir,
            FileName);
          }
        }
      else
        {
        strcpy(PathName,FileName);
        }
 
      if ((A->FP = fopen(PathName,"a+")) == NULL)
        {
        MDB(0,fAM) MLog("WARNING:  cannot open AM file '%s', errno: %d (%s)\n",
          PathName,
          errno,
          strerror(errno));
 
        /* dump AM records to logfile */
 
        A->FP = mlog.logfp;
 
        return(FAILURE);
        }
      else
        { 
        fprintf(A->FP,"AMSTART TIME=%ld\n",
          MSched.Time);
 
        fflush(A->FP);
        }
 
      MDB(3,fAM) MLog("INFO:     AM type '%s' initialized (file '%s' opened)\n",
        MAMType[A->Type],
        PathName);
 
      break;
 
    case mamtNONE:
 
      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;
 
    default:
 
      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;
    }  /* END switch (A->Type) */

  /* check fallback account */
 
  if (A->FallbackAccount[0] != '\0')
    {
    /* initialize fallback account */
 
    if (MAcctAdd(A->FallbackAccount,NULL) == FAILURE)
      {
      MDB(1,fAM) MLog("ALERT:    cannot add fallback account '%s'\n",
        A->FallbackAccount);
 
      return(FAILURE);
      }
    }

  if (A->FallbackQOS[0] != '\0')
    {
    /* initialize fallback qos */

    if (MQOSAdd(A->FallbackQOS,NULL) == FAILURE)
      {
      MDB(1,fAM) MLog("ALERT:    cannot add fallback qos '%s'\n",
        A->FallbackQOS);

      return(FAILURE);
      }
    }
 
  return(SUCCESS);
  }  /* END MAMActivate() */





/**
 *
 *
 * @param A (I)
 * @param U (I) [modified]
 * @param RIndex (O) [optional]
 */

int MAMUserUpdateAcctAccessList(

  mam_t    *A, /* I */
  mgcred_t *U, /* I (modified) */
  enum MHoldReasonEnum *RIndex) /* O (optional) */

  {
  const char *FName = "MAMUserUpdateAcctAccessList";

  MDB(3,fAM) MLog("%s(%s,%s,RIndex)\n",
    FName,
    (A != NULL) ? A->Name : "NULL",
    (U != NULL) ? U->Name : "NULL");

  if (RIndex != NULL)
    *RIndex = mhrNONE;

  if ((A == NULL) || (U == NULL))
    {
    return(FAILURE);
    }

  if (A->State != mrmsActive)
    {
    return(FAILURE);
    }
 
  switch (A->Type)
    {
    default:

      /* NOT SUPPORTED */

      return(FAILURE);
  
      /*NOTREACHED*/

      break;
    }  /* END switch (A->Type) */
 
  return(SUCCESS);
  }  /* END MAMUserUpdateAcctAccessList() */


/**
 * Close connection to accounting manager.
 *
 * @param A (I) [modified]
 */

int MAMClose(

  mam_t *A)   /* I (modified) */
 
  {
  const char *FName = "MAMClose";

  MDB(2,fAM) MLog("%s(%s)\n",
    FName,
    (A != NULL) ? A->Name : "NULL");

  if (A == NULL)
    {
    return(FAILURE);
    }
 
  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    return(SUCCESS);
    }
 
  switch (A->Type)
    {
    case mamtNONE:
 
      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;

    case mamtFILE:
 
      if (A->FP != NULL)
        {
        fprintf(A->FP,MAM_FILEFOOTER);

        if (A->FP != mlog.logfp) 
          fclose(A->FP);
 
        A->FP = NULL;
        }
 
      break;
 
    case mamtMAM:
    case mamtGOLD:

      /* NYI */
 
      return(SUCCESS); 
 
      /*NOTREACHED*/
 
      break;
 
    default:
 
      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;
    }  /* END switch (A->Type) */
 
  return(SUCCESS);
  }  /* END MAMClose() */


/**
 *
 */

int MAMAToString(

  mam_t            *A,       /* I */
  enum MAMAttrEnum  AIndex,  /* I */ 
  char             *SVal,    /* O (minsize=MMAX_LINE) */
  int               BufSize, /* I */
  int               Mode)    /* I (not used) */

  {
  if (SVal != NULL)
    SVal[0] = '\0';

  if ((A == NULL) || (SVal == NULL))
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mamaEndURL:

      if (A->ND.AMURL[mamnEnd] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnEnd],BufSize);
        }

      break;

    case mamaCreateURL:

      if (A->ND.AMURL[mamnCreate] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnCreate],BufSize);
        }

      break;

    case mamaDeleteURL:

      if (A->ND.AMURL[mamnDelete] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnDelete],BufSize);
        }

      break;

    case mamaModifyURL:

      if (A->ND.AMURL[mamnModify] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnModify],BufSize);
        }

      break;

    case mamaPauseURL:

      if (A->ND.AMURL[mamnPause] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnPause],BufSize);
        }

      break;

    case mamaQueryURL:

      if (A->ND.AMURL[mamnQuery] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnQuery],BufSize);
        }

      break;

    case mamaQuoteURL:

      if (A->ND.AMURL[mamnQuote] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnQuote],BufSize);
        }

      break;

/* Comment out until functionally implemented */
#ifdef MNOT
    case mamaResumeURL:

      if (A->ND.AMURL[mamnResume] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnResume],BufSize);
        }

      break;
#endif /* MNOT */

    case mamaStartURL:

      if (A->ND.AMURL[mamnStart] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnStart],BufSize);
        }

      break;

    case mamaUpdateURL:

      if (A->ND.AMURL[mamnUpdate] != NULL)
        {
        MUStrCpy(SVal,A->ND.AMURL[mamnUpdate],BufSize);
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MAMAToString() */


/**
 * Set specified accounting manager attribute.
 *
 * @see MAMAToString() - peer - report specified accounting manager attribute
 *
 * @param A (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode
 */

int MAMSetAttr(

  mam_t  *A,      /* I (modified) */
  int     AIndex, /* I */
  void  **Value,  /* I */
  int     Format, /* I */
  int     Mode)

  {
  const char *FName = "MAMSetAttr";

  MDB(7,fSCHED) MLog("%s(%s,%d,Value,%d)\n",
    FName,
    (A != NULL) ? A->Name : "NULL",
    AIndex,
    Mode);

  if (A == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mamaCSAlgo:

      A->CSAlgo = (enum MChecksumAlgoEnum)MUGetIndexCI((char *)Value,MCSAlgoType,FALSE,A->CSAlgo);

      break;

    case mamaCSKey:

      MUStrCpy(A->CSKey,(char *)Value,sizeof(A->CSKey));

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MAMSetAttr() */




/**
 *
 *
 * @param A (I)
 */

int MAMShutdown(

  mam_t *A)  /* I */

  {
  if (A == NULL)
    {
    return(FAILURE);
    }

  switch (A->Type)
    {
    case mamtNative:

      /* This will flush, but also clear the queues */

      MAMFlushNAMIQueues(A);

      return(SUCCESS);

      break;

    case mamtNONE:

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case mamtMAM:
    case mamtGOLD:

      MSUFree(A->P.S);
      MUFree((char **)&A->P.S);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (A->Type) */

  /* NOTE:  temp until AM can unregister itself */

  A->State = mrmsDown;

  MAMClose(A);

  return(SUCCESS);
  }  /* END MAMShutdown() */





/**
 *
 *
 * @param A (I)
 * @param FType (I)
 */

int __MAMStartFunc(

  mam_t *A,     /* I */
  int    FType) /* I */

  {
  MUGetMS(NULL,&A->P.RespStartTime[FType]);

  return(SUCCESS);
  }  /* END __MAMStartFunc() */





/**
 *
 *
 * @param A (I)
 * @param FType (I)
 */

int __MAMEndFunc(

  mam_t *A,     /* I */
  int    FType) /* I */

  {
  long NowMS;
  long Interval;

  mpsi_t *P;

  P = &A->P;

  if (P->RespStartTime[FType] <= 0)
    {
    /* invalid time */

    return(FAILURE);
    }

  MUGetMS(NULL,&NowMS);

  if (NowMS < P->RespStartTime[FType])
    Interval = P->RespStartTime[FType] - NowMS;
  else
    Interval = NowMS - P->RespStartTime[FType];

  P->RespTotalTime[FType] += Interval;
  P->RespMaxTime[FType] = MAX(P->RespMaxTime[FType],Interval);
  P->RespTotalCount[FType]++;

  if (FType != 0)
    {
    P->RespTotalTime[0] += Interval;
    P->RespMaxTime[0] = MAX(P->RespMaxTime[0],Interval);
    P->RespTotalCount[0]++;
    }

  /* reset start time */

  P->RespStartTime[FType] = -1;

  return(SUCCESS);
  }  /* EMD __MAMEndFunc() */






/**
 *
 *
 * @param A (I)
 * @param FunctionType (I)
 * @param FailureType (I)
 * @param FMsg (I)
 */

int MAMSetFailure(

  mam_t       *A,
  int          FunctionType,
  enum MStatusCodeEnum FailureType,
  const char  *FMsg)

  {
  int findex;

  mpsi_t *P;

  if (A == NULL)
    {
    return(FAILURE);
    }

  P = &A->P; 

  /* record failure */

  switch (FailureType)
    {
    case mscBadParam:

      /* NO-OP */

      break;

    default:

      A->FailCount++;

      break;
    }  /* END switch (FailureType) */

  switch (FailureType)
    {
    case mscBadParam:
    case mscRemoteFailure:
    case mscNoError:

      /* NO-OP */

      break;

    case mscSysFailure:
    default:

      /* widespread AM failure, prevent further calls this iteration */

      A->FailIteration = MSched.Iteration;

      break;
    }  /* END switch (FailureType) */

  findex = P->FailIndex;

  P->FailTime[findex] = MSched.Time;
  P->FailType[findex] = FunctionType;

  P->RespFailCount[FunctionType]++;

  if (FMsg != NULL)
    MUStrDup(&P->FailMsg[findex],FMsg);
  else
    MUFree(&P->FailMsg[findex]);

  P->FailIndex = (findex + 1) % MMAX_PSFAILURE;

  /* adjust interface state */

  switch (FunctionType)
    {
    default:

      /* NO-OP */

      break;
    }  /* END switch (FunctionType) */

  return(SUCCESS);
  }  /* END MAMSetFailure() */


/**
 * Aggregates the actions in each queue and submits them to the AM.
 *   Called every flush interval
 * 
 * Queue order is important!
 *
 * Queue order:
 *   Charge
 *   Delete
 *
 * @param A (I) - The accounting manager that will have its queues flushed
 */

int MAMFlushNAMIQueues(
  mam_t *A)

  {
  if (A == NULL)
    {
    return(FAILURE);
    }

  mstring_t Action(MMAX_LINE);

  MStringSet(&Action,"");
  if (MXMLToMString(A->NAMIChargeQueueE,&Action,NULL,TRUE) == SUCCESS)
    {
    MAMNativeDoCommandWithString(A,Action.c_str(),mamnEnd);
    }

  MStringSet(&Action,"");
  if (MXMLToMString(A->NAMIDeleteQueueE,&Action,NULL,TRUE) == SUCCESS)
    {
    MAMNativeDoCommandWithString(A,Action.c_str(),mamnDelete);
    }

/*
  MStringSet(&Action,"");
  if (MXMLToMString(A->NAMIPauseQueueE,&Action,NULL,TRUE) == SUCCESS)
    {
    MAMNativeDoCommandWithString(A,Action.c_str(),mamnPause);
    }
*/

  MXMLDestroyE(&A->NAMIChargeQueueE);
  MXMLDestroyE(&A->NAMIDeleteQueueE);
/* MXMLDestroyE(&A->NAMIPauseQueueE); */

  return(SUCCESS);
  }  /* END MAMFlushNAMIQueues() */




/**
 * Adds the given action XML (object description) to the 
 *  given queue on the AM.
 *
 * @param A      [I] - The AM to add to
 * @param Action [I] - The XML to add
 * @param Queue  [I] - Specific queue type
 */

int MAMAddToNAMIQueue(
  mam_t *A,
  mxml_t *Action,
  enum MAMNativeFuncTypeEnum Queue)

  {
  if ((A == NULL) ||
      (Action == NULL))
    return(FAILURE);

  switch (Queue)
    {
    case mamnEnd:

      {
      if ((A->NAMIChargeQueueE == NULL) &&
          (MXMLCreateE(&A->NAMIChargeQueueE,"OBJECTS") == FAILURE))
        {
        return(FAILURE);
        }

      MXMLAddE(A->NAMIChargeQueueE,Action);
      }

      break;

    case mamnDelete:

      {
      if ((A->NAMIDeleteQueueE == NULL) &&
          (MXMLCreateE(&A->NAMIDeleteQueueE,"OBJECTS") == FAILURE))
        {
        return(FAILURE);
        }

      MXMLAddE(A->NAMIDeleteQueueE,Action);
      }

      break;

/*
    case mamnPause:

      {
      if ((A->NAMIPauseQueueE == NULL) &&
          (MXMLCreateE(&A->NAMIPauseQueueE,"OBJECTS") == FAILURE))
        {
        return(FAILURE);
        }

      MXMLAddE(A->NAMIPauseQueueE,Action);
      }

      break;
*/

    default:

      return(FAILURE);

      break;
    } /* END switch (Queue) */
  
  return(SUCCESS);
  }  /* MAMAddToNAMIQueue() */

/* END MAM.c */
