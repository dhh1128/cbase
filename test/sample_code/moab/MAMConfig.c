/* HEADER */

      
/**
 * @file MAMConfig.c
 *
 * Contains: MAM Configuration fucntions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

 
 
 
/**
 *
 *
 * @param A (I)
 */

int MAMCheckConfig(
 
  mam_t *AM)  /* I */
 
  {
  if (AM == NULL)
    {
    return(FAILURE);
    }

  MAMSetDefaults(AM);
 
  /* NYI */
 
  return(SUCCESS);
  }  /* END MAMCheckConfig() */

 
 
/**
 * Process 'AMCFG' attributes.
 *
 * @see MAMLoadConfig() - parent
 * @see MAMSetAttr() - child
 *
 * @param AM (I) [modified]
 * @param Value (I)
 */

int MAMProcessConfig(
 
  mam_t *AM,     /* I (modified) */
  char  *Value) /* I */
 
  {
  int   aindex;
 
  char *ptr;
  char *TokPtr;
 
  char  ValLine[MMAX_LINE];
 
  if ((AM == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }
 
  /* process value line */
 
  ptr = MUStrTok(Value," \t\n",&TokPtr);
 
  while (ptr != NULL)
    {
    /* parse name-value pairs */
 
    /* FORMAT:  <VALUE>[,<VALUE>] */
 
    if (MUGetPair(
          ptr, 
          (const char **)MAMAttr,
          &aindex,
          NULL,
          TRUE,
          NULL,
          ValLine,
          MMAX_NAME) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%.128s' specified",
        ptr);

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for AM %s\n",
        ptr,
        (AM != NULL) ? AM->Name : "NULL");

      MMBAdd(&AM->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }
 
    switch (aindex)
      {
      case mamaChargeObjects:

        /* FORMAT:  <OBJECT>[,<OBJECT>] */

        {
        int   tmpI;

        char *ptr2;
        char *TokPtr2;

        char tmpLine[MMAX_LINE];

        memset(AM->IsChargeExempt,TRUE,sizeof(AM->IsChargeExempt));

        MUStrCpy(tmpLine,ValLine,sizeof(tmpLine));

        ptr2 = MUStrTok(tmpLine,", :",&TokPtr2);

        while (ptr2 != NULL)
          {
          tmpI = MUGetIndexCI(ptr2,(const char **)MXO,FALSE,0);

          AM->IsChargeExempt[tmpI] = FALSE;

          ptr2 = MUStrTok(NULL,", :",&TokPtr2);
          }
        }    /* END BLOCK */

        break;

      case mamaChargePolicy:

        AM->ChargePolicy = 
          (enum MAMConsumptionPolicyEnum)MUGetIndexCI(
            ValLine,
            (const char **)MAMChargeMetricPolicy,
            FALSE,
            MDEF_AMCHARGEPOLICY);

        break;

      case mamaChargeRate:

        {
        char *ptr;

        /* FORMAT:  [local:]<VALUE>[/<INTERVAL>][*] */

        if (!strcasecmp(ValLine,"disa"))
          {
          MDISASetDefaults(AM);

          break;
          }

        if (!strncasecmp(ValLine,"local:",strlen("local:")))
          {
          AM->ChargePolicyIsLocal = TRUE;

          ptr = ValLine + strlen("local:");
          }
        else
          {
          ptr = ValLine;
          }

        AM->ChargeRate = strtod(ptr,&ptr);

        if ((ptr != NULL) && (ptr[0] == '/'))
          {
          ptr++;

          AM->ChargePeriod = (enum MCalPeriodEnum)MUGetIndexCI(
            ptr,
            (const char **)MCalPeriodType,
            TRUE,
            mpNONE);
          }
        }  /* END BLOCK (case mamaChargeRate) */

        break;

      case mamaCreateCred:

        AM->CreateCred = MUBoolFromString(ValLine,FALSE);

        break;

      case mamaCreateFailureAction:

        {
        char *ptr;
        char *TokPtr;

        /* FORMAT:  <AMFailureAction>[,<NoFundsAction>] */

        ptr = MUStrTok(ValLine,", \t\n",&TokPtr);

        AM->CreateFailureAction = (enum MAMJFActionEnum)MUGetIndexCI(
          ptr,
          MAMJFActionType,
          TRUE,
          mamjfaNONE);

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);

        if (ptr != NULL)
          {
          AM->CreateFailureNFAction = (enum MAMJFActionEnum)MUGetIndexCI(
            ptr,
            MAMJFActionType,
            TRUE,
            mamjfaNONE);
          }
        else
          {
          AM->CreateFailureNFAction = AM->CreateFailureAction;
          }
        }    /* END BLOCK */

        break;

      case mamaFallbackAccount:

        MUStrCpy(AM->FallbackAccount,ValLine,sizeof(AM->FallbackAccount));

        break;

      case mamaFallbackQOS:

        MUStrCpy(AM->FallbackQOS,ValLine,sizeof(AM->FallbackQOS));

        break;

      case mamaFlags:

        bmfromstring(ValLine,MAMFlags,&AM->Flags);

        break;

      case mamaFlushInterval:

        /* NOTE:  supports only period */

        /* FORMAT:  {minute|hour|day|week|month} */

        if (!isalpha(ValLine[0]))
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"invalid flush interval specified '%.128s'",
            ptr);

          MDB(3,fSTRUCT) MLog("ERROR:  %s for AM %s\n",
            tmpLine,
            (AM != NULL) ? AM->Name : "NULL");

          MMBAdd(&AM->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

          break;
          }
        else
          {
          /* period specified */

          AM->FlushPeriod = (enum MCalPeriodEnum)MUGetIndexCI(ValLine,MCalPeriodType,FALSE,mpNONE);

          if (AM->FlushPeriod == mpNONE)
            {
            AM->FlushInterval = MMAX_TIME;
            }
          else
            {
            AM->FlushInterval = MCalPeriodDuration[AM->FlushPeriod];

            AM->FlushTime = (MSched.Time - (MSched.Time % AM->FlushInterval)) + AM->FlushInterval;
            }
          }

        break;

      case mamaHost:

        {
        char *ptr;
        char *TokPtr;

        /* FORMAT:  [<HOSTNAME>][*] */
 
        ptr = MUStrTok(ValLine,"*",&TokPtr);

        AM->UseDirectoryService = FALSE;

        if (ptr == NULL)
          break;

        if (ptr[0] == '*')
          {
          AM->UseDirectoryService = TRUE;
          }
        else
          {     
          MUStrCpy(AM->Host,ptr,sizeof(AM->Host));
          MUStrCpy(AM->SpecHost,ptr,sizeof(AM->SpecHost));
          }

        ptr = MUStrTok(NULL,"*",&TokPtr);

        if (ptr == NULL)
          break;

        if (ptr[0] == '*')
          {
          AM->UseDirectoryService = TRUE;
          }
        else
          {
          MUStrCpy(AM->Host,ptr,sizeof(AM->Host));
          MUStrCpy(AM->SpecHost,ptr,sizeof(AM->SpecHost));
          }
        }  /* END BLOCK */

        break;

      case mamaNodeChargePolicy:

        AM->NodeChargePolicy = (enum MAMNodeChargePolicyEnum)MUGetIndexCI( 
          ValLine,
          MAMNodeChargePolicy,
          FALSE,
          mamncpNONE);

        break;

      case mamaPort:

        AM->Port     = (int)strtol(ValLine,NULL,10);
        AM->SpecPort = AM->Port;

        break;

/* Comment out until functionally implemented */
#ifdef MNOT
      case mamaResumeFailureAction:

        {
        char *ptr;
        char *TokPtr;

        /* FORMAT:  <AMFailureAction>[,<NoFundsAction>] */

        ptr = MUStrTok(ValLine,", \t\n",&TokPtr);

        AM->ResumeFailureAction = (enum MAMJFActionEnum)MUGetIndexCI(
          ptr,
          MAMJFActionType,
          TRUE,
          mamjfaNONE);

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);

        if (ptr != NULL)
          {
          AM->ResumeFailureNFAction = (enum MAMJFActionEnum)MUGetIndexCI(
            ptr,
            MAMJFActionType,
            TRUE,
            mamjfaNONE);
          }
        else
          {
          AM->ResumeFailureNFAction = AM->ResumeFailureAction;
          }
        }    /* END BLOCK */

        break;
#endif /* MNOT */

      case mamaStartFailureAction:
      case mamaJFAction:

        {
        char *ptr;
        char *TokPtr;

        /* FORMAT:  <AMFailureAction>[,<NoFundsAction>] */

        ptr = MUStrTok(ValLine,", \t\n",&TokPtr);

        AM->StartFailureAction = (enum MAMJFActionEnum)MUGetIndexCI(
          ptr,
          MAMJFActionType,
          TRUE,
          mamjfaNONE);

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);

        if (ptr != NULL)
          {
          AM->StartFailureNFAction = (enum MAMJFActionEnum)MUGetIndexCI(
            ptr,
            MAMJFActionType,
            TRUE,
            mamjfaNONE);
          }
        else
          {
          AM->StartFailureNFAction = AM->StartFailureAction;
          }

        /* use of JOBFAILUREACTION is deprecated */
        if (aindex == mamaJFAction)
          {
          char message[] = "Use of the JOBFAILUREACTION parameter is deprecated. Use STARTFAILUREACTION instead.";

          MDB(3,fSTRUCT) MLog("WARNING:  %s\n",message);

          MMBAdd(&AM->MB,message,NULL,mmbtNONE,0,0,NULL);
          }
        }    /* END BLOCK */

        break;

      case mamaServer:

        {
        char tmpHost[MMAX_LINE];
        char tmpDir[MMAX_LINE];
        char tmpProtocol[MMAX_LINE];

        if (!strcmp(ValLine,"ANY"))
          {
          AM->UseDirectoryService = TRUE;
          
          break;
          }

        /* FORMAT:  [<PROTO>://][<HOST>[:<PORT>]][/<DIR>] */

        if (MUURLParse(
              ValLine,
              tmpProtocol,
              tmpHost,
              tmpDir,
              sizeof(tmpDir),
              &AM->Port,
              TRUE,
              TRUE) == FAILURE)
          {
          /* cannot parse string */

          break;
          }

        if (tmpProtocol[0] != '\0')
          {
          AM->Type = (enum MAMTypeEnum)MUGetIndexCI(tmpProtocol,MAMType,FALSE,0);
  
          if (AM->Type == mamtNONE)
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"invalid protocol '%.128s' specified",
              tmpProtocol);

            MDB(3,fSTRUCT) MLog("WARNING:  invalid protocol '%s' specified for AM %s\n",
              tmpProtocol,
              (AM != NULL) ? AM->Name : "NULL");

            MMBAdd(&AM->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
            }
          }    /* END if (tmpProtocol[0] != '\0') */

        AM->SpecPort = AM->Port;

        AM->UseDirectoryService = FALSE;

        if (AM->Type != mamtFILE)
          {
          MUStrCpy(AM->Host,tmpHost,sizeof(AM->Host));
          MUStrCpy(AM->SpecHost,tmpHost,sizeof(AM->SpecHost));
          }
        else
          {
          MUStrCpy(AM->Host,tmpDir,sizeof(AM->Host));
          MUStrCpy(AM->SpecHost,tmpDir,sizeof(AM->SpecHost));
          }
        }  /* END BLOCK */
 
        break;

      case mamaSocketProtocol:

        AM->SocketProtocol = (enum MSocketProtocolEnum)MUGetIndexCI(ValLine,MSockProtocol,FALSE,0);

        /* NOTE:  if 'HTTP' specified, use 'HTTPClient' */

        break;

      case mamaTimeout:

        /* timeout specified in seconds/microseconds - convert to microseconds */

        AM->Timeout = MUTimeFromString(ValLine);

        if (AM->Timeout < 1000)
          AM->Timeout *= MDEF_USPERSECOND;
 
        break;

      case mamaType:

        AM->Type = (enum MAMTypeEnum)MUGetIndexCI(ValLine,MAMType,FALSE,0);

        break;

      case mamaUpdateFailureAction:

        {
        char *ptr;
        char *TokPtr;

        /* FORMAT:  <AMFailureAction>[,<NoFundsAction>] */

        ptr = MUStrTok(ValLine,", \t\n",&TokPtr);

        AM->UpdateFailureAction = (enum MAMJFActionEnum)MUGetIndexCI(
          ptr,
          MAMJFActionType,
          TRUE,
          mamjfaNONE);

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);

        if (ptr != NULL)
          {
          AM->UpdateFailureNFAction = (enum MAMJFActionEnum)MUGetIndexCI(
            ptr,
            MAMJFActionType,
            TRUE,
            mamjfaNONE);
          }
        else
          {
          AM->UpdateFailureNFAction = AM->UpdateFailureAction;
          }
        }    /* END BLOCK */

        break;

      case mamaWireProtocol:

        AM->WireProtocol = (enum MWireProtocolEnum)MUGetIndexCI(ValLine,MWireProtocol,FALSE,0);

        break;

      case mamaValidateJobSubmission:

        AM->ValidateJobSubmission = MUBoolFromString(ValLine,TRUE);

        break;

      case mamaCreateURL:
      case mamaDeleteURL:
      case mamaEndURL:
      case mamaModifyURL:
      case mamaPauseURL:
      case mamaQueryURL:
      case mamaQuoteURL:
/*    case mamaResumeURL: * Comment out until functionally implemented */
      case mamaStartURL:
      case mamaUpdateURL:

        {
        char tmpProtoString[MMAX_NAME];
        char tmpHost[MMAX_NAME];
        char tmpDir[MMAX_PATH_LEN];
        int  tmpPort;
   
        enum MBaseProtocolEnum tmpProtocol;
   
        enum MAMNativeFuncTypeEnum AFIndex = mamnNONE;
   
        mnat_t *ND;
   
        ND = &AM->ND;
   
        switch (aindex)
          {
          case mamaCreateURL:         AFIndex = mamnCreate; break;        
          case mamaDeleteURL:         AFIndex = mamnDelete; break;        
          case mamaEndURL:            AFIndex = mamnEnd; break;        
          case mamaModifyURL:         AFIndex = mamnModify; break;        
          case mamaPauseURL:          AFIndex = mamnPause; break;        
          case mamaQueryURL:          AFIndex = mamnQuery; break;        
          case mamaQuoteURL:          AFIndex = mamnQuote; break;        
/*        case mamaResumeURL:         AFIndex = mamnResume; break; */ 
          case mamaStartURL:          AFIndex = mamnStart; break;        
          case mamaUpdateURL:         AFIndex = mamnUpdate; break;        
          default:                    return(FAILURE); break;
          }  /* END switch (aindex) */
   
        if (Value == NULL)
          {
          /* unset URL */
   
          switch (aindex)
            {
            case mamaCreateURL:
            case mamaDeleteURL:
            case mamaEndURL:
            case mamaModifyURL:
            case mamaPauseURL:
            case mamaQueryURL:
            case mamaQuoteURL:
/*          case mamaResumeURL: * Comment out until functionally implemented */
            case mamaStartURL:
            case mamaUpdateURL:
   
              ND->AMProtocol[AFIndex] = mbpNONE;
   
              MUFree(&ND->AMPath[AFIndex]);
              MUFree(&ND->AMURL[AFIndex]);
   
              break;
   
            default:
   
              assert(FALSE);
   
              /* NO-OP */
   
              break;
            }  /* END switch (aindex) */
   
          break;
          }  /* END if (Value == NULL) */
        else
          {
          /* set URL */
   
          switch (aindex)
            {
            case mamaCreateURL:
            case mamaDeleteURL:
            case mamaEndURL:
            case mamaModifyURL:
            case mamaPauseURL:
            case mamaQueryURL:
            case mamaQuoteURL:
/*          case mamaResumeURL: * Comment out until functionally implemented */
            case mamaStartURL:
            case mamaUpdateURL:
 
              MUStrDup(&ND->AMURL[AFIndex],(char *)Value);
   
              break;
   
            default:
   
              assert(FALSE);
              /* NO-OP */
   
              break;
            }  /* END switch (aindex) */
          }    /* END if (Value == NULL) */
   
        /* parse URL */
   
        if (MUURLParse(
             (char *)Value,
             tmpProtoString,
             tmpHost,            /* O */
             tmpDir,             /* O */
             sizeof(tmpDir),
             &tmpPort,           /* O */
             TRUE,
             TRUE) == SUCCESS)
          {
          ND->AMPort[AFIndex] = tmpPort;
   
          /* NOTE:  high-level serverport was previously set based on last processed URL.
           *        this attribute should only be set via the RM 'port' config option */
   
          /* ND->ServerPort = tmpPort; */
   
          MUStrDup(&ND->AMHost[AFIndex],tmpHost);
   
          tmpProtocol = (enum MBaseProtocolEnum)MUGetIndexCI(
            tmpProtoString,
            MBaseProtocol,
            FALSE,
            mbpNONE);
   
          if (tmpProtocol == mbpNONE)
            {
            /* set default */
   
            tmpProtocol = mbpExec;
            }
          }
        else
          {
          tmpProtocol = mbpExec;
   
          MUStrCpy(tmpDir,(char *)Value,sizeof(tmpDir));
          }
   
        /* sanity check URL */
   
        if ((tmpDir != NULL) &&
           ((tmpProtocol == mbpExec) || (tmpProtocol == mbpFile)))
          {
          char *ptr;
   
          mbool_t IsExe;
   
          int     rc;
   
          /* remove args for file check */
   
          if ((ptr = strchr(tmpDir,'?')) != NULL)
            *ptr = '\0';
   
          rc = MFUGetInfo(tmpDir,NULL,NULL,&IsExe,NULL);
   
          if (ptr != NULL)
            *ptr = '?';
   
          /* if there is a failure don't RETURN, still attach it so that
             the error will propogate to jobs and other objects. */

          if (rc == FAILURE)
            {
            char tmpLine[MMAX_LINE];
   
            /* warn and continue */
   
            MDB(3,fNAT) MLog("WARNING:  invalid value '%s' specified for %s (cannot locate path)\n",
              (char *)Value,
              MAMAttr[aindex]);
   
            snprintf(tmpLine,sizeof(tmpLine),"file %s for %s does not exist",
              tmpDir,
              MAMAttr[aindex]);
   
            MMBAdd(&AM->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
            }
          else if ((tmpProtocol == mbpExec) && (IsExe != TRUE))
            {
            char tmpLine[MMAX_LINE];
   
            MDB(3,fNAT) MLog("WARNING:  invalid value '%s' specified for '%s' (path not executable)\n",
              (char *)Value,
              MAMAttr[aindex]);
   
            snprintf(tmpLine,sizeof(tmpLine),"file %s for %s is not executable",
              tmpDir,
              MAMAttr[aindex]);
   
            MMBAdd(&AM->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
            }
          }    /* END if ((tmpDir != NULL) && ...) */
   
        /* set values */
   
        switch (aindex)
          {
          case mamaCreateURL:
          case mamaDeleteURL:
          case mamaEndURL:
          case mamaModifyURL:
          case mamaPauseURL:
          case mamaQueryURL:
          case mamaQuoteURL:
/*        case mamaResumeURL: * Comment out until functionally implemented */
          case mamaStartURL:
          case mamaUpdateURL:

            ND->AMProtocol[AFIndex] = tmpProtocol;
   
            MUStrDup(&ND->AMPath[AFIndex],tmpDir);
   
            break;
   
          default:
   
            /* NO-OP */
   
            break;
          }  /* END switch (aindex) */
   
        /* NOTE:  only override, no support for add */
        }  /* END BLOCK (case mrmaClusterQueryCmd, ...) */


        break;

      default:
 
        MDB(4,fAM) MLog("WARNING:  AM attribute '%s' not handled\n",
          MAMAttr[aindex]);
 
        break;
      }  /* END switch (aindex) */
 
    ptr = MUStrTok(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  /* NOTE:  completed job purge time important for track allocation of jobs
            which complete while Moab if shutdown. */

  if (MSched.JobCPurgeTimeSpecified == FALSE)
    MSched.JobCPurgeTime = MAX(MSched.JobCPurgeTime,MCONST_MINUTELEN * 10);
 
  return(SUCCESS);
  }  /* END MAMProcessConfig() */





/**
 *
 *
 * @param AMName (I) [optional]
 * @param Buf (I) [optional]
 */

int MAMLoadConfig(
 
  char *AMName,  /* I (optional) */
  char *Buf)     /* I (optional) */ 

  {
  char   IndexName[MMAX_NAME];
 
  char   Value[MMAX_LINE];
 
  char  *ptr;
  char  *head;
 
  mam_t *A;
 
  /* FORMAT:  <KEY>=<VAL>[<WS><KEY>=<VAL>]...         */
  /*          <VAL> -> <ATTR>=<VAL>[:<ATTR>=<VAL>]... */
 
  /* load all/specified AM config info */

  head = (Buf != NULL) ? Buf : MSched.ConfigBuffer;
 
  if (head == NULL)
    {
    return(FAILURE);
    }
 
  if ((AMName == NULL) || (AMName[0] == '\0'))
    {
    /* load ALL AM config info */
 
    ptr = head;
 
    IndexName[0] = '\0';
 
    while (MCfgGetSVal(
             head,
             &ptr,
             MCredCfgParm[mxoAM],
             IndexName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) != FAILURE)
      {
      if (MAMFind(IndexName,&A) == FAILURE)
        {
        if (MAMAdd(IndexName,&A) == FAILURE) 
          {
          /* unable to locate/create AM */
 
          IndexName[0] = '\0';
 
          continue;
          }
 
        MAMSetDefaults(A);
        }
 
      /* load AM specific attributes */
 
      MAMProcessConfig(A,Value);

      IndexName[0] = '\0';
      }  /* END while (MCfgGetSVal() != FAILURE) */

    A = &MAM[0];  /* NOTE:  only supports single AM per scheduler */

    if (MAMCheckConfig(A) == FAILURE)
      {
      MAMDestroy(&A);

      /* invalid AM destroyed */

      return(FAILURE);
      }
    }    /* END if ((AMName == NULL) || (AMName[0] == '\0')) */
  else
    {
    /* load specified AM config info */
 
    ptr = MSched.ConfigBuffer;
 
    A = NULL;
 
    while (MCfgGetSVal(
             head,
             &ptr,
             MCredCfgParm[mxoAM],
             AMName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) == SUCCESS)
      {
      if ((A == NULL) &&
          (MAMFind(AMName,&A) == FAILURE))
        {
        if (MAMAdd(AMName,&A) == FAILURE)
          {
          /* unable to add AM */ 
 
          return(FAILURE);
          }
 
        MAMSetDefaults(A);
        }
 
      /* load AM attributes */
 
      MAMProcessConfig(A,Value);
      }  /* END while (MCfgGetSVal() == SUCCESS) */
 
    if (A == NULL)
      {
      return(FAILURE);
      }
 
    if (MAMCheckConfig(A) == FAILURE)
      {
      MAMDestroy(&A);
 
      /* invalid AM destroyed */
 
      return(FAILURE);
      }
    }  /* END else ((AMName == NULL) || (AMName[0] == '\0')) */
 
  return(SUCCESS);
  }  /* END MAMLoadConfig() */

/*END MAMConfig.c */ 
