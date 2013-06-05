/* HEADER */

      
/**
 * @file MIdentity.c
 *
 * Contains: Identiy functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Load/Process MIDCFG[] parameters.
 *
 * @see MIDProcessConfig() - child
 *
 * @param IDName (I) [optional]
 * @param Buf (I) [optional]
 */

int MIDLoadConfig(

  char *IDName,  /* I (optional) */
  char *Buf)     /* I (optional) */

  {
  char   IndexName[MMAX_NAME];

  char   Value[MMAX_LINE];

  char  *ptr;
  char  *head;

  midm_t *I;

  /* FORMAT:  <KEY>=<VAL>[<WS><KEY>=<VAL>]...         */
  /*          <VAL> -> <ATTR>=<VAL>[:<ATTR>=<VAL>]... */

  /* load all/specified ID config info */

  head = (Buf != NULL) ? Buf : MSched.ConfigBuffer;

  if (head == NULL)
    {
    return(FAILURE);
    }

  if ((IDName == NULL) || (IDName[0] == '\0'))
    {
    /* load ALL ID config info */

    ptr = head;

    IndexName[0] = '\0';

    while (MCfgGetSVal(
             head,
             &ptr,
             MParam[mcoIDCfg],
             IndexName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) != FAILURE)
      {
      if (MIDFind(IndexName,&I) == FAILURE)
        {
        if (MIDAdd(IndexName,&I) == FAILURE)
          {
          /* unable to locate/create AM */

          IndexName[0] = '\0';

          continue;
          }

        MIDSetDefaults(I);
        }

      /* load ID specific attributes */

      MIDProcessConfig(I,Value);

      IndexName[0] = '\0';
      }  /* END while (MCfgGetSVal() != FAILURE) */

    I = &MID[0];  /* NOTE:  only support single ID manager per scheduler */

    if (MIDCheckConfig(I) == FAILURE)
      {
      MIDDestroy(&I);

      /* invalid ID destroyed */

      return(FAILURE);
      }

    /* NOTE:  default state to active if configured */

    I->State = mrmsActive;
    }  /* END else ((IDName == NULL) || (IDName[0] == '\0')) */

  return(SUCCESS);
  }  /* END MIDLoadConfig() */





/**
 * Process MIDCFG[] parameters
 * 
 * @see MIDLoadConfig() - parent
 *
 * @param I (I) [modified]
 * @param Value (I)
 */

int MIDProcessConfig(

  midm_t *I,     /* I (modified) */
  char   *Value) /* I */

  {
  int   aindex;

  char *ptr;
  char *TokPtr;

  char  ValLine[MMAX_LINE];

  char  tmpLine[MMAX_LINE];

  int  tmpPort;

  if ((I == NULL) ||
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

    /* FOAMAT:  <VALUE>[,<VALUE>] */

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
      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%s' specified",
        ptr);

      MMBAdd(&I->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      ptr = MUStrTok(NULL," \t\n",&TokPtr);

      continue;
      }

    switch (aindex)
      {
      case mamaBlockCredList:

        bmfromstring(ValLine,MXO,&I->CredBlockList);

        break;

      case mamaCreateCred:

        I->CreateCred = MUBoolFromString(ValLine,FALSE);

        break;

      case mamaCreateCredURL:
      case mamaDestroyCredURL:
      case mamaResetCredURL:

        {
        char tmpProtoString[MMAX_NAME];
        char tmpHost[MMAX_NAME];
        char tmpDir[MMAX_PATH_LEN];

        enum MBaseProtocolEnum tmpProtocol;

        if (ValLine[0] == '\0')
          {
          /* unset URL */

          switch (aindex)
            {
            case mamaCreateCredURL:

              I->CredCreateProtocol = mbpNONE;

              MUFree(&I->CredCreatePath);
              MUFree(&I->CredCreateURL);

              break;

            case mamaDestroyCredURL:

              I->CredDestroyProtocol = mbpNONE;

              MUFree(&I->CredDestroyPath);
              MUFree(&I->CredDestroyURL);

              break;

            case mamaResetCredURL:

              I->CredResetProtocol = mbpNONE;

              MUFree(&I->CredResetPath);
              MUFree(&I->CredResetURL);

              break;

            default:

              snprintf(tmpLine,sizeof(tmpLine),"attribute '%s' not supported",
                ptr);

              MMBAdd(&I->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

              break;
            }  /* END switch (AIndex) */

          break;
          }  /* END if (ValLine[0] == '\0') */

        /* set URL */

        switch (aindex)
          {
          case mamaCreateCredURL:

            MUStrDup(&I->CredCreateURL,(char *)ValLine);

            break;

          case mamaDestroyCredURL:

            MUStrDup(&I->CredDestroyURL,(char *)ValLine);

            break;

          case mamaResetCredURL:

            MUStrDup(&I->CredResetURL,(char *)ValLine);

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (AIndex) */

        /* parse URL */

        if (MUURLParse(
             (char *)ValLine,
             tmpProtoString,
             tmpHost,
             tmpDir,
             sizeof(tmpDir),
             &tmpPort,
             TRUE,
             TRUE) == SUCCESS)
          {
          /* MUStrDup(&I->ServerHost,tmpHost); */

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

          if (rc == FAILURE)
            {
            char tmpLine[MMAX_LINE];

            /* warn and continue */

            MDB(3,fNAT) MLog("WARNING:  invalid value '%s' specified for %s (cannot locate path)\n",
              (char *)Value,
              MAMAttr[aindex]);

            snprintf(tmpLine,sizeof(tmpLine),"WARNING:  invalid value '%s' specified for %s (cannot locate path)\n",
              (char *)Value,
              MAMAttr[aindex]);

            MMBAdd(&I->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

            /*
            MAMSetAttr(
              R,
              mamaMessages,
              (void **)"cluster query path is not executable",
              mdfString,
              mIncr);
            */
            }

          if ((tmpProtocol == mbpExec) && (IsExe != TRUE))
            {
            MDB(3,fNAT) MLog("WARNING:  invalid value '%s' specified for '%s' (path not executable)\n",
              (char *)Value,
              MAMAttr[aindex]);

            /*
            MAMSetAttr(
              R,
              mamaMessages,
              (void **)"cluster query path is not executable",
              mdfString,
              mIncr);
            */

            return(FAILURE);
            }
          }    /* END if ((tmpDir != NULL) && ...) */

        /* set values */

        switch (aindex)
          {
          case mamaCreateCredURL:

            I->CredCreateProtocol = tmpProtocol;

            if (!MUStrIsEmpty(tmpDir))
              {
              MUStrDup(&I->CredCreatePath,tmpDir);
              }

            break;

          case mamaDestroyCredURL:

            I->CredDestroyProtocol = tmpProtocol;

            if (!MUStrIsEmpty(tmpDir))
              {
              MUStrDup(&I->CredDestroyPath,tmpDir);
              }

            break;

          case mamaResetCredURL:

            I->CredResetProtocol = tmpProtocol;

            if (!MUStrIsEmpty(tmpDir))
              {
              MUStrDup(&I->CredResetPath,tmpDir);
              }

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (aindex) */
        }    /* END (BLOCK) */

        break;

      case mamaRefreshPeriod:

        I->RefreshPeriod = (enum MCalPeriodEnum)MUGetIndexCI(
          ValLine,
          MCalPeriodType,
          FALSE,
          mpNONE);

        break;

      case mamaResetCredList:

        bmfromstring(ValLine,MXO,&I->CredResetList);

        break;

      case mamaServer:

        {
        char tmpHost[MMAX_LINE];
        char tmpProtocol[MMAX_LINE];
        char tmpPath[MMAX_LINE];

        if (!strcmp(ValLine,"ANY"))
          {
          I->UseDirectoryService = TRUE;

          break;
          }

        /* FORMAT:  [<PROTO>://][<HOST>[:<PORT>]][/<DIR>] */

        if (MUURLParse(
              ValLine,
              tmpProtocol,
              tmpHost,
              tmpPath,
              sizeof(tmpPath),
              &I->P.Port,
              TRUE,
              TRUE) == FAILURE)
          {
          /* cannot parse string */

          break;
          }

        I->Type = (enum MIDTypeEnum)MUGetIndexCI(tmpProtocol,MIDType,FALSE,0);

        I->UseDirectoryService = FALSE;

        if (tmpHost[0] != '\0')
          MUStrDup(&I->P.HostName,tmpHost);

        if (tmpPath[0] != '\0')
          MUStrDup(&I->P.Path,tmpPath);
        }  /* END BLOCK */

        break;

      case mamaTimeout:

        /* timeout specified in seconds/microseconds - convert to microseconds */

        I->P.Timeout = MUTimeFromString(ValLine);

        if (I->P.Timeout < 1000)
          I->P.Timeout *= MDEF_USPERSECOND;

        break;

      case mamaUpdateRefreshOnFailure:

        if (!strcasecmp(ValLine,"TRUE"))
          {
          I->UpdateRefreshOnFailure = TRUE;
          }

        break;

      default:

        MDB(4,fAM) MLog("WARNING:  ID attribute '%s' not handled\n",
          MAMAttr[aindex]);

        break;
      }  /* END switch (aindex) */

    ptr = MUStrTok(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MIDProcessConfig() */



/* 
-----
# sample idmgr.txt

user:john     alist=account1,account2 var=key=34wwd5
user:steve    alist=account3
acct:account1 qlist=mg1
acct:account2 qlist=mg2
acct:account3 qlist=mg2
-----
*/

/**
 * Load identity manager attribute/value pairs.
 *
 * NOTE:  Support for credential attributes must be explicitly added on an
 *        attribute by attribute basis in the 'switch (aindex)' stanza below.
 *
 * NOTE:  ID info is authoritative.
 *
 * @see MServerUpdate() - parent
 * @see MFSTreeLoadConfig() - child - process FSTree XML if specified
 *
 * @param I (I)
 * @param String (I/O) [optional,alloc]
 * @param SOType (I) [optional]  if specified, only process lines with matching cred object
 * @param SO (I) [optional]  if specified, only process lines with matching cred object
 */

int MIDLoadCred(

  midm_t              *I,
  enum MXMLOTypeEnum   SOType,
  void                *SO)

  {
  char *cline;
  char *ptr;
  char *TokPtr = NULL;
  char *TokPtr2 = NULL;
  char *tail;

  int   tmpI;
  char  AttrArray[MMAX_NAME];
  char  ValLine[MMAX_LINE];
  char  tmpLine[MMAX_LINE];

  enum MCredAttrEnum aindex;

  char *NameP = NULL;

  char *IBuf;  /* buffer containing full id data buffer */
  char *FSData = NULL; /* (alloc) */

  enum MXMLOTypeEnum   OType;
  void                *O;

  mfs_t               *F;

  mpsi_t              *P;

  void                *OP;

  void                *OE;

  char *ctype = NULL;
  char *cid   = NULL;

  mbool_t              FSTreeReload = FALSE;

  const char *FName = "MIDLoadCred";

  MDB(3,fAM) MLog("%s(%s,AName,%s,SO)\n",
    FName,
    (I != NULL) ? I->Name : "NULL",
    MXO[SOType]);

  if (I == NULL)
    {
    return(FAILURE);
    }

  if (I->Type == midtNONE)
    {
    return(FAILURE);
    }

  if (I->State == mrmsDown)
    {
    MDB(3,fAM) MLog("WARNING:  idmanager %s is down\n",
      I->Name);

    return(FAILURE);
    }

  if (SOType != mxoNONE)
    {
    if (MOGetName(SO,SOType,&NameP) == FAILURE)
      {
      MDB(3,fAM) MLog("WARNING:  cannot locate specified object\n");
  
      return(FAILURE);
      }
    }

  /* support LDAP/SQL/local */

  /* load attributes including email, priority, security access, *
   * permissions, cred mappings, preferences */

  IBuf = NULL;

  mstring_t String(MMAX_BUFFER);

  MRMStartFunc(NULL,&I->P,mrmInfoQuery);

  /* contact identity manager */

  switch (I->Type)
    {
    case midtFile:
    case midtExec:

      if ((I->P.Path == NULL) || (I->P.Path[0] == '\0'))
        {
        MMBAdd(
          &I->MB,
          "no path specified in 'SERVER' parameter",
          NULL,
          mmbtNONE,
          0,
          0,
          NULL);

        return(FAILURE);
        }

      /* FORMAT:  <CREDTYPE>:<CREDID> <ATTR>=<VAL>[ <ATTR>=<VAL>]... */

      if (I->Type == midtFile) 
        {
        char EMsg[MMAX_LINE];

        if ((ptr = MFULoad(I->P.Path,1,macmWrite,NULL,EMsg,NULL)) == NULL)
          {
          char tmpLine[MMAX_LINE];

          MDB(2,fCONFIG) MLog("WARNING:  cannot load id file '%s'\n",
            I->P.Path);

          snprintf(tmpLine,sizeof(tmpLine),"cannot load id file '%s' - %s",
            I->P.Path,
            (EMsg[0] != '\0') ? EMsg : "N/A");

          MMBAdd(&I->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

          return(FAILURE);
          }

        String = ptr;

        MUFree(&ptr);
        }  /* END if (I->Type == midtFile) */
      else 
        {
        char EMsg[MMAX_LINE];

        enum MStatusCodeEnum SC;

        /* assume exec mode */

        if (NameP != NULL)
          {
          sprintf(tmpLine,"%s %s %s",
            I->P.Path,
            MXO[SOType],
            NameP);
          }
        else
          {
          sprintf(tmpLine,"%s %s",
            I->P.Path,
            "all");
          }

        I->P.Timeout = 30 * MDEF_USPERSECOND; /* NYI make this configurable */

        mstring_t EBuf(MMAX_LINE);

        if (MUReadPipe2(
              tmpLine,
              NULL,
              &String,
              &EBuf,
              &I->P,
              NULL,
              EMsg,      /* O */
              &SC) == FAILURE)
          { 
          if (!EBuf.empty())
            {
            MDB(1,fNAT) MLog("ALERT:    cannot load IDMgr data from command '%s' - stderr %s\n",
              tmpLine,
              EBuf.c_str());

            MMBAdd(&I->MB,EBuf.c_str(),NULL,mmbtNONE,0,0,NULL);
            }
          else if (EMsg[0] != '\0')
            {
            MDB(1,fNAT) MLog("ALERT:    cannot load IDMgr data from command '%s' - %s\n",
              tmpLine,
              EBuf.c_str());

            MMBAdd(&I->MB,EMsg,NULL,mmbtNONE,0,0,NULL);
            }

          if (SC == mscNoMemory)
            {
            MDB(1,fNAT) MLog("ALERT:    cannot read output of id command '%s' - buffer too small\n",
              tmpLine);
            }
          else
            {
            MDB(1,fNAT) MLog("ALERT:    cannot read output of id command '%s', SC=%d\n",
              tmpLine,
              SC);
            }

          return(FAILURE);
          }
        }  /* END else (I->Type == midtFile) */

      break;

    case midtLDAP:

      {
      int rc;

      rc = MIDLDAPLoadCred(I,SOType,SO,NULL);

      return(rc);
      }  /* END BLOCK */

      /*NOTREACHED*/

    default:

      /* NO-OP */

      break;
    }  /* END switch (I->Type) */

  IBuf = String.c_str();

  if ((IBuf == NULL) || (IBuf[0] == '\0'))
    {
    /* no data to process */

    return(FAILURE);
    }

  memset(I->OCount,0,sizeof(I->OCount));

  if (strstr(IBuf,"FSTREE") != NULL)
    {
    /* NOTE:  only update share tree on initial start up */

    /* NOTE:  change to destroy/re-create share tree periodically */

    MUStrDup(&FSData,IBuf);
   
    /* NOTE:  process share tree info after all creds are loaded */
    }

  if (!bmisclear(&I->CredResetList))
    {
    mhashiter_t HTI;

    if (bmisset(&I->CredResetList,mxoUser))
      {
      mgcred_t *U;

      MOINITLOOP(&OP,mxoUser,&OE,&HTI);

      while ((U = (mgcred_t *)MOGetNextObject(&OP,mxoUser,OE,&HTI,&NameP)) != NULL)
        {
        if (!strcmp(U->Name,"DEFAULT"))
          continue;

        U->F.FSTarget = 0.0;
        U->F.FSCap    = 0.0;

        /* clear AList */

        if (U->F.AAL != NULL)
          MULLFree(&U->F.AAL,NULL);

        U->F.ADef = NULL;
        }
      }

    if (bmisset(&I->CredResetList,mxoGroup))
      {
      mgcred_t *G;

      MOINITLOOP(&OP,mxoGroup,&OE,&HTI);

      while ((G = (mgcred_t *)MOGetNextObject(&OP,mxoGroup,OE,&HTI,&NameP)) != NULL)
        {
        if (!strcmp(G->Name,"DEFAULT"))
          continue;

        G->F.FSTarget = 0.0;
        G->F.FSCap    = 0.0;

        /* clear AList */

        if (G->F.AAL != NULL)
          MULLFree(&G->F.AAL,NULL);

        G->F.ADef = NULL;
        }
      }

    if (bmisset(&I->CredResetList,mxoAcct))
      {
      mgcred_t *A;

      MOINITLOOP(&OP,mxoAcct,&OE,&HTI);

      while ((A = (mgcred_t *)MOGetNextObject(&OP,mxoAcct,OE,&HTI,&NameP)) != NULL)
        {
        if (!strcmp(A->Name,"DEFAULT"))
          continue;

        A->F.FSTarget = 0.0;
        A->F.FSCap    = 0.0;
        }
      }
    }    /* END if (I->CredResetList != 0) */

  /* locate specified credential data */

  mstring_t Response(MMAX_LINE);

  if (NameP != NULL)
    {
    char scratch[MMAX_LINE+1];

    sprintf(tmpLine,"%s:%s",
      MXO[SOType],
      NameP);

    if (MUBufGetMatchToken(
          IBuf,
          tmpLine,
          "\n",
          NULL,
          scratch,
          sizeof(scratch)) == NULL)
      {
      /* NOTE:  if cred not located, continue */

      MDB(1,fAM) MLog("INFO:     cannot locate id information for cred '%s'\n",
        tmpLine);

      MUFree(&FSData);

      return(SUCCESS);
      }

      Response = scratch;
    }    /* END if (NameP != NULL) */
  else
    {
    /* load first line */

    cline = MUStrTok(IBuf,"\n",&TokPtr);

    Response = cline;
    }

  cline = Response.c_str();

  while (cline != NULL)
    {
    if ((cline[0] == '#') || (cline[0] == '\0'))
      {
      /* ignore empty lines and comments */

      cline = MUStrTok(NULL,"\n",&TokPtr);
  
      continue;
      }

    /* parse primary key */

    ptr = MUStrTok(cline," \t",&TokPtr2);

    if ((ptr == NULL) || (ptr[0] == '\0'))
      {
      /* ignore lines with no object */

      cline = MUStrTokE(NULL,"\n",&TokPtr);

      continue;
      }

    if (SOType == mxoNONE)
      {
      char tmpLine[MMAX_LINE];

      char *TokPtr3;
  
      /* object type not specified, load all objects */
    
      /* process primary key (<CREDTYPE>:<CREDID>) */

      strncpy(tmpLine,ptr,sizeof(tmpLine));

      ctype = MUStrTok(tmpLine,":",&TokPtr3);
      cid = MUStrTok(NULL,":",&TokPtr3);

      OType = (enum MXMLOTypeEnum)MUGetIndexCI(ctype,MXO,FALSE,mxoNONE);

      if (OType == mxoNONE)
        {
        if (!strcasecmp(ctype,"account"))
          {
          OType = mxoAcct;
          }
        else if (strstr(ctype,"FSTREE") == NULL)
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"cannot load data for %s:%s (unknown object type '%s')",
            ctype,
            (cid != NULL) ? cid : "NULL",
            ctype);

          MMBAdd(&I->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
          }
        else /* fairshare tree */
          {
          if ((I->Type == midtFile) || (I->Type == midtExec))
            {  
            OType = mxoxFSC;

            if ((MPar[0].FSC.ShareTree != NULL) || (MSched.ParHasFSTree != NULL))
              {
              /* The fairshare tree has been previously loaded so this must be a reload */

              FSTreeReload = TRUE;
              }
            }
          }
        }    /* END if (OType == mxoNONE) */

      /* increment object type counters */

      I->OCount[mxoNONE]++;  /* bump the total counter */

      if (OType != mxoNONE)
        {
        I->OCount[OType]++;
        }
      else
        {
        MDB(3,fAM) MLog("WARNING:  cannot process ID line '%.64s...;\n",
          ptr);

        I->OCount[mxoLAST]++;
        }

      if (OType == mxoRM)
        {
        /* do not add RM, create peer object */

        if (MPeerAdd(cid,&P) == FAILURE)
          {
          MDB(1,fAM) MLog("ALERT:    cannot add peer '%s' specified via identity manager interface\n",
            cid);

          continue;
          }
  
        P->Type = mpstNM;
        }
      else if (OType == mxoxFSC)
        {
        /* don't process any FSTREE data here (we do that later), just treat the fstree data like comments and skip over them */

        /* TokPtr2 has any data on the same line following FSTREE[xxx] */

        if ((TokPtr2 != NULL) && (strstr(TokPtr2,"</fstree>") != NULL))
          {
          /* found the xml terminator so all of the FSTREE XML is on one line. 
             just skip this line */

          cline = MUStrTokE(NULL,"\n",&TokPtr);
          }
        else /* New format where XML can be spread out over multiple lines - (e.g. tidy --xml format) */
          {
          /* eat all of the lines between FSTREE[xxx] and </fstree> 
             since we do not process any of the fstree info in this loop */

          while ((cline != NULL) && (strstr(cline,"</fstree>") == NULL))
            cline = MUStrTokE(NULL,"\n",&TokPtr);

           /* eat the </fstree> line also if on a separate line*/

          if ((cline != NULL) && (strstr(cline,"</fstree>") != NULL))
            cline = MUStrTokE(NULL,"\n",&TokPtr);
          }

        continue;
        }
      else if (MOGetObject(
            OType,
            cid,
            &O,   /* O */
            (I->CreateCred == TRUE) ? mAdd : mVerify) == FAILURE)
        {
        cline = MUStrTokE(NULL,"\n",&TokPtr);

        if (cline != NULL)
          {
          MStringSet(&Response,cline);

          cline = Response.c_str();
          }

        continue;
        }
      }    /* END if (SOType == mxoNONE) */
    else
      {
      OType = SOType;
      O     = SO;
      } 

    if ((OType == mxoUser) || (OType == mxoGroup) || (OType == mxoAcct))
      {
      ((mgcred_t *)O)->AMUpdateTime = MSched.Time;
      }

    /* get first attr-value pair */

    ptr = MUStrTokE(NULL," \t\n",&TokPtr2);

    MOGetComponent(O,OType,(void **)&F,mxoxFS);

    while (ptr != NULL)
      {
      /* parse name-value pairs */
  
      if (MUGetPair(
          ptr,       /* I FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */
          (const char **)MCredAttr,
          &tmpI,     /* O */
          AttrArray, /* O */
          TRUE,
          NULL,
          ValLine,   /* O */
          sizeof(ValLine)) == FAILURE)
        {
        enum MQOSAttrEnum qaindex;

        /* unrecognized value pair */

        /* See if it is a QOS attribute */
        if ((OType == mxoQOS) &&
            (MUGetPair(
              ptr,       /* I FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */
              (const char **)MQOSAttr,
              &tmpI,     /* O */ 
              AttrArray, /* O */ 
              TRUE,
              NULL,
              ValLine,   /* O */ 
              sizeof(ValLine)) == SUCCESS))
          {
          qaindex = (enum MQOSAttrEnum)tmpI;

          switch(qaindex)
            {
            case mqaFlags:

              MQOSFlagsFromString((mqos_t *)O,ValLine);

              break;

            default:

              /* NOT SUPPORTED */

              break; 
            }
          }
        else if ((OType == mxoUser) &&
                 (MUGetPair(
                  ptr,       /* I FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */
                  (const char **)MUserAttr,
                  &tmpI,     /* O */
                  AttrArray, /* O */
                  TRUE,
                  NULL,
                  ValLine,   /* O */
                  sizeof(ValLine)) == SUCCESS))
          {
          enum MUserAttrEnum uaindex;

          uaindex = (enum MUserAttrEnum)tmpI;

          /* Check if they are declaring admin level for the user */

          if (uaindex == muaAdminLevel)
            {
            int AdminIndex; /* Iterate through admin users */
            int ListIndex;  /* Iterate through admin lists (level 1, 2, etc.) */
            madmin_t *AdminList; /* Specific admin list */
            char *UserName;

            int NewRole = ((int)strtol(ValLine,NULL,10));

            UserName = ((mgcred_t *)O)->Name;

            if ((NewRole < 1) || (NewRole > 5))
              {
              MDB(3,fAM) MLog("ALERT:    admin level for user '%s' must be a number between 1 and 5 (given %d)\n",
                UserName,
                NewRole);
              }
            else
              {
              ((mgcred_t *)O)->Role = (enum MRoleEnum)NewRole;
              }

            AdminList = &MSched.Admin[NewRole];

            for (AdminIndex = 0;AdminIndex < MMAX_ADMINUSERS + 1;AdminIndex++)
              {
              if (AdminList->UName[AdminIndex][0] == '\0')
                {
                /* Found an open spot */

                MUStrCpy(
                  AdminList->UName[AdminIndex],
                  UserName,
                  MMAX_NAME);                                                       
        
                MDB(3,fCONFIG) MLog("INFO:     admin %d user '%s' added\n",
                  AdminList->Index,
                  UserName);

                break;
                }
              else if (strcmp(AdminList->UName[AdminIndex],UserName) == 0)
                {
                /* User already admin */

                break;
                }
              }

            for (ListIndex = 1;ListIndex < MMAX_ADMINTYPE + 1;ListIndex++)
              {
              int CurUserIndex = -1;
              int SwitchUserIndex = -1;

              /* Only need to check levels lower than NewRole */

              if (ListIndex == NewRole)
                break;

              AdminList = &MSched.Admin[ListIndex];

              for (AdminIndex = 0;AdminIndex < MMAX_ADMINUSERS + 1;AdminIndex++)
                {
                if (AdminList->UName[AdminIndex][0] == '\0')
                  break;

                SwitchUserIndex = AdminIndex;

                if (strcmp(AdminList->UName[AdminIndex],UserName) == 0)
                  {
                  /* Need to remove user from this admin list - Find last user and switch*/

                  CurUserIndex = AdminIndex; 
                  }
                }

              if (CurUserIndex != -1)
                {
                /* Switch users, remove current user */

                MUStrCpy(AdminList->UName[CurUserIndex],AdminList->UName[SwitchUserIndex],MMAX_NAME);
                memset(AdminList->UName[SwitchUserIndex],0,MMAX_NAME);
                }
              }
            }
          }
        else if (tmpI == -1)
          {
          /* NOTE:  add opaque credential value */

          if (!strncasecmp(ptr,"key",strlen("key")))
            {
            if (OType == mxoRM)
              {
              MUStrDup(&P->CSKey,ValLine);
              }
            }
          }

        ptr = MUStrTokE(NULL," \t\n",&TokPtr2);

        continue;
        }

      aindex = (enum MCredAttrEnum)tmpI;

      switch (aindex)
        {
        case mcaADef:

          if (F == NULL)
            break;

          if (strcmp(ValLine,DEFAULT) && strcmp(ValLine,"DEFAULT"))
            {
            if ((MAcctFind(ValLine,(mgcred_t **)&F->ADef) == SUCCESS) ||
                (MAcctAdd(ValLine,(mgcred_t **)&F->ADef) == SUCCESS))
              {
              /* add account to AList */

              MULLAdd(
                &F->AAL,
                ValLine,
                (void *)F->ADef,
                NULL,
                NULL);  /* no free routine */
              }
            }

          break;

        case mcaAList:

          if (F == NULL)
            break;

          /* determine if change occurred (NYI) */

          {
          char *aptr;
          char *ATokPtr;

          mgcred_t *A;

          /* NOTE: for now, force value to set (no incr/decr supported) */

          enum MCompEnum ValCmp = mcmpEQ; 

          /* FORMAT:  <AID>[{,:}<AID>]... */

          aptr = MUStrTok(ValLine,",: \n\t",&ATokPtr);

          if ((ValCmp != mcmpINCR) && (ValCmp != mcmpDECR))
            {
            /* clear AAL */

            MULLFree(&F->AAL,NULL);
            }

          /* NOTE:  mcmpDECR not supported */

          while (aptr != NULL)
            {
            if (strcmp(aptr,DEFAULT) && strcmp(aptr,"DEFAULT"))
              {
              if ((MAcctFind(aptr,(mgcred_t **)&A) == SUCCESS) ||
                  (MAcctAdd(aptr,(mgcred_t **)&A) == SUCCESS))
                {
                /* add account to AList */
  
                MULLAdd(&F->AAL,aptr,(void *)A,NULL,NULL);  /* no free routine */
                }
              }

            aptr = MUStrTok(NULL,",: \n\t",&ATokPtr);
            }  /* END while (aptr != NULL) */

          if (F->AAL != NULL)
            {
            if (F->ADef == NULL)
              F->ADef = (mgcred_t *)F->AAL->Ptr;
            }
          else
            {
            F->ADef = NULL;
            }
          }    /* END BLOCK (case mcaAList) */

          break;

        case mcaChargeRate:

          switch (OType)
            {
            case mxoUser:
            case mxoGroup:
            case mxoAcct:

              ((mgcred_t *)O)->ChargeRate = strtod(ValLine,NULL);

              break;

            case mxoQOS:

              ((mqos_t *)O)->DedResCost = strtod(ValLine,NULL);

              break;

            case mxoClass:

              /* NOT SUPPORTED */

              break;

            default:

              /* NOT SUPPORTED */

              break;
            }  /* END switch (OType) */

          break;

        case mcaComment:

          /* NOTE:  allow only one comment from ID manager */

          {
          mmb_t **MBP;

          switch (OType)
            {
            case mxoUser:
            case mxoGroup:
            case mxoAcct:
            default:

              MBP = &((mgcred_t *)O)->MB;

              break;

            case mxoQOS:

              MBP = &((mqos_t *)O)->MB;

              break;

            case mxoClass:

              MBP = &((mclass_t *)O)->MB;

              break;
            }  /* END switch (OIndex) */

          if (MBP != NULL)
            {
            MMBRemoveMessage(MBP,NULL,mmbtAnnotation);
            }

          if (ValLine[0] != '\0')
            { 
            MCredSetAttr(
              O,
              OType,
              aindex,
              NULL,
              (void **)ValLine,
              NULL,
              mdfString,
              mSet);
            }
          }    /* END BLOCK (case mcaComment) */

          break;

        case mcaJobFlags:

          switch (OType)
            {
            case mxoQOS:

              MJobFlagsFromString(NULL,NULL,&((mqos_t *)O)->F.JobFlags,NULL,":,|",ValLine,FALSE);

              break;

            default:

              /* NOT SUPPORTED */

              break;
            }  /* END switch (OType) */

          break;

        case mcaManagers:
        case mcaMaxMem:
        case mcaMaxGRes:
        case mcaMaxIGRes:
        case mcaMaxIJob:
        case mcaMaxJob:
        case mcaMaxNode:
        case mcaMaxProc:
        case mcaMaxPS:
        case mcaMaxPE:
        case mcaMaxWC:
        case mcaMaxWCLimitPerJob:
        case mcaMinNodePerJob:
        case mcaMaxNodePerJob:
        case mcaMaxJobPerUser:
        case mcaMaxJobPerGroup:
        case mcaMaxProcPerJob:
        case mcaMaxProcPerUser:
        case mcaMaxNodePerUser:
        case mcaMaxSubmitJobs:
        case mcaPList:

          MCredSetAttr(
            O,
            OType,
            aindex,
            NULL,
            (void **)ValLine,
            AttrArray,
            mdfString,
            mSet);

          break;

        case mcaEMailAddress:

          switch (OType)
            {
            case mxoUser:
            case mxoGroup:
            case mxoAcct:

              MUStrDup(&((mgcred_t *)O)->EMailAddress,ValLine);

              break;

            default:

              /* NOT SUPPORTED */

              break;
            }  /* END switch (OType) */

          break;

        case mcaFSCap:

          if (F == NULL)
            break;

          if (ValLine[0] != '\0')
            {
            F->FSCap = strtod(ValLine,NULL);

            if (strchr(ValLine,'%') != NULL)
              {
              F->FSCapMode = mfstCapRel;
              }
            else if (strchr(ValLine,'^') != NULL)
              {
              F->FSCapMode = mfstCapAbs;
              }
            else
              {
              MDB(3,fFS) MLog("INFO:     No type specified for FSCap, defaulting to Relative\n");

              F->FSCapMode = mfstCapRel;
              }
            }
          else
            {
            F->FSCap = 0.0;
            }

          break;

        case mcaFSTarget:

          if (F == NULL)
            break;

          if (ValLine[0] != '\0')
            {
            F->FSTarget = strtod(ValLine,NULL);
            }
          else
            {
            F->FSTarget = 0.0;
            }

          break;

        case mcaGFSTarget:

          if (F == NULL)
            break;

          if (ValLine[0] != '\0')
            {
            F->GFSTarget = strtod(ValLine,NULL);
            }
          else
            {
            F->GFSTarget = 0.0;
            }

          break;

        case mcaGFSUsage:

          if (F == NULL)
            break;

          if (ValLine[0] != '\0')
            {
            F->GFSUsage = strtod(ValLine,NULL);
            }
          else
            {
            F->GFSUsage = 0.0;
            }

          break;

        case mcaPref:

          /* NYI */

          break;

        case mcaPriority:

          if ((F != NULL) && (ValLine[0] != '\0'))
            {
            F->Priority = strtol(ValLine,NULL,10);
            }

          break;

        case mcaQDef:

          if (F == NULL)
            break;

          /* FORMAT:  <QOS>[&^] */

          if (!strcmp(ValLine,DEFAULT) || !strcmp(ValLine,"DEFAULT"))
            {
            break;
            }

          if ((tail = strchr(ValLine,'&')) != NULL)
            F->QALType = malAND;
          else if ((tail = strchr(ValLine,'^')) != NULL)
            F->QALType = malOnly;
          else
            F->QALType = malOR;

          if (tail != NULL)
            *tail = '\0';

          if (MQOSFind(ValLine,(mqos_t **)&F->QDef[0]) == FAILURE)
            {
            if (F->QDef[0] != NULL)
              {
              /* empty slot located */

              MQOSAdd(ValLine,(mqos_t **)&F->QDef[0]);
              }
            else
              {
              F->QDef[0] = &MQOS[MDEF_SYSQDEF];
              }
            }

          bmset(&F->QAL,((mqos_t *)F->QDef[0])->Index);

          bmset(&F->Flags,mffQDefSpecified);

          break;

        case mcaQList:

          if ((F == NULL) || (ValLine[0] == '\0'))
            {
            /* NO-OP */

            break;
            }
 
          {
          char QALLine[MMAX_LINE];

          mbitmap_t tmpQAL;

          int ValCmp = mcmpEQ;

          mqos_t *FQ;

          if (strchr(ValLine,'&') != NULL)
            F->QALType = malAND;
          else if (strchr(ValLine,'^') != NULL)
            F->QALType = malOnly;
          else
            F->QALType = malOR;

          /* support '=' and '+=' */

          /* FORMAT:  <QOS>[,<QOS>]... */

          if (MQOSListBMFromString(ValLine,&tmpQAL,&FQ,mAdd) == FAILURE)
            {
            MDB(5,fFS) MLog("ALERT:    cannot parse string '%s' as rangelist\n",
              ValLine);
            }

          /* support '=' and '+=' */

          if (ValCmp == mcmpINCR)
            {
            bmor(&F->QAL,&tmpQAL);
            }
          else if (ValCmp == mcmpDECR)
            {
            bmnot(&tmpQAL,MMAX_QOS);

            bmand(&F->QAL,&tmpQAL);
            }
          else
            {
            bmcopy(&F->QAL,&tmpQAL);
            }

          MDB(2,fFS) MLog("INFO:     QOS access list set to %s\n",
            MQOSBMToString(&F->QAL,QALLine));

          if (F->QDef[0] != NULL)
            {
            /* if set, add QDef to QList */

            bmset(&F->QAL,((mqos_t *)F->QDef[0])->Index);
            }
          else if ((FQ != NULL) && (MSched.QOSIsOptional == FALSE))
            {
            /* if not set, add first QList QOS as QDef */

            F->QDef[0] = FQ;
            }

          bmclear(&tmpQAL);
          }  /* END BLOCK (case mcaQList) */

          break;

        case mcaRole:

          /* NYI */

          break;
 
        case mcaVariables:

          MCredSetAttr(O,OType,aindex,NULL,(void **)ValLine,NULL,mdfString,mSet);

          break;
 
        default:

          /* unsupported attribute */
  
          MDB(3,fRM) MLog("ALERT:    cannot add attribute '%s' to %s %s via ID manager\n",
            MCredAttr[aindex],
            MXO[OType],
            (cid != NULL) ? cid : "");

          break;
        }  /* END switch (aindex) */

      ptr = MUStrTokE(NULL," \t\n",&TokPtr2);
      }    /* END while (ptr != NULL) */

    if (SO != NULL)
      break;

    cline = MUStrTokE(NULL,"\n",&TokPtr);

    if (cline != NULL)
      {
      MStringSet(&Response,cline);

      cline = Response.c_str();
      }
    }      /* END while (cline != NULL) */

  if (!bmisclear(&I->CredBlockList))
    {
    mhashiter_t HTI;

    if (bmisset(&I->CredBlockList,mxoUser))
      {
      mgcred_t *U;

      MOINITLOOP(&OP,mxoUser,&OE,&HTI);

      while ((U = (mgcred_t *)MOGetNextObject(&OP,mxoUser,OE,&HTI,&NameP)) != NULL)
        {
        if (!strcmp(U->Name,"DEFAULT"))
          continue;

        if (U->AMUpdateTime <= I->LastRefresh)
          {
          /* user is stale - block resource access */

          U->F.FSCap = -1.0;
          }
        else
          {
          /* user is updated - enable resource access */

          if (U->F.FSCap == -1.0)
            U->F.FSCap = 0.0;
          }
        }
      }

    if (bmisset(&I->CredBlockList,mxoGroup))
      {
      mgcred_t *G;

      MOINITLOOP(&OP,mxoGroup,&OE,&HTI);

      while ((G = (mgcred_t *)MOGetNextObject(&OP,mxoGroup,OE,&HTI,&NameP)) != NULL)
        {
        if (!strcmp(G->Name,"DEFAULT"))
          continue;

        if (G->AMUpdateTime <= I->LastRefresh)
          {
          /* group is stale - block resource access */

          G->F.FSCap = -1.0;
          }
        else
          {
          /* group is updated - enable resource access */

          if (G->F.FSCap == -1.0)
            G->F.FSCap = 0.0;
          } 
        }
      }  /* END if (bmisset(&I->CredBlockList,mxoGroup)) */

    if (bmisset(&I->CredBlockList,mxoAcct))
      {
      mgcred_t *A;

      MOINITLOOP(&OP,mxoAcct,&OE,&HTI);

      while ((A = (mgcred_t *)MOGetNextObject(&OP,mxoAcct,OE,&HTI,&NameP)) != NULL)
        {
        if (!strcmp(A->Name,"DEFAULT"))
          continue;

        if (A->AMUpdateTime <= I->LastRefresh)
          {
          /* account is stale - block resource access */

          A->F.FSCap = -1.0;
          }
        else
          {
          /* account is updated - enable resource access */

          if (A->F.FSCap == -1.0)
            A->F.FSCap = 0.0;
          }
        }
      }    /* END if (bmisset(&I->CredBlockList,mxoAcct)) */
    }      /* END if (I->CredBlockList != 0) */

  /* process the fairshare tree data */

  if (FSData != NULL)
    {
    int   pindex;

    char *StartFSTree = NULL;
    mxml_t *tmpFSTree[MMAX_PAR];

    if (FSTreeReload == TRUE)
      {
      /* save pointers to the current fairshare trees and NULL tree pointers in MPar[] */

      memset(tmpFSTree,0x0,sizeof(tmpFSTree));

      MSched.FSTreeDepth = 0;
      MUFree((char **)&MSched.ParHasFSTree);

      for (pindex = 0;pindex < MMAX_PAR;pindex++)
        {
        if (MPar[pindex].Name[0] == '\0')
          break;

        if (MPar[pindex].Name[1] == '\1')
          continue;

        if (MPar[pindex].FSC.ShareTree != NULL)
          {
          tmpFSTree[pindex] = MPar[pindex].FSC.ShareTree;

          MPar[pindex].FSC.ShareTree = NULL;
          }
        } /* END for pindex */
      } /* END if FSTreeReload */

    /* if the tree data is in the new XML tidy format then do some pre-processing of the buffer */

    StartFSTree = MFSTreeReformatXML(FSData);

    /* load the tree data into the MPar[x].FSC tree structures */

    MFSTreeLoadConfig((StartFSTree != NULL) ? StartFSTree : FSData);

    /* if we are doing a reload then merge the old tree data (saved off above) into the new tree and 
       reset the job fstree pointers */

    if (FSTreeReload == TRUE)
      {
      job_iter JTI;

      mjob_t *J;

      /* merge the old trees with the new ones */

      for (pindex = 0;pindex < MMAX_PAR;pindex++)
        {
        if (MPar[pindex].Name[0] == '\0')
          break;

        if (MPar[pindex].Name[1] == '\1')
          continue;

        if ((MPar[pindex].FSC.ShareTree != NULL) && (tmpFSTree[pindex] != NULL))
          {
          /* we are reloading the tree for this partition so merge the old data into the new tree */

          MFSTreeLoadFSData(MPar[pindex].FSC.ShareTree,tmpFSTree[pindex],0,TRUE);
          }
        } /* END for pindex */

      /* recalculate the usage since our trees may have changed */

      MFSTreeUpdateUsage();

      /* free the old fairshare trees now that we have copied the data over to the new trees */

      for (pindex = 0;pindex < MMAX_PAR;pindex++)
        {
        if (tmpFSTree[pindex] != NULL)
          {
          MFSTreeFree(&tmpFSTree[pindex]); 
          }
        } /* END for pindex */

      MJobIterInit(&JTI);

      /* reset the fairshare tree pointers for all jobs */

      while (MJobTableIterate(&JTI,&J) == SUCCESS)
        {
        if (J->FairShareTree == NULL)
          continue;

        MJobGetFSTree(J,NULL,TRUE);
        } /* END for J->Next */
      } /* END if FSTreeReload */

    MUFree(&FSData);
    } /* END if FSData != NULL */

  MRMEndFunc(NULL,&I->P,mrmInfoQuery,NULL);

  I->LastRefresh = MSched.Time;

  return(SUCCESS);
  }  /* END MIDLoadCred() */





/**
 * Destroy/free identity manager object.
 *
 * @param I (I) [modified]
 */

int MIDDestroy(

  midm_t **I)  /* I (modified) */

  {
  if (I == NULL)
    {
    return(SUCCESS);
    }

  if (*I == NULL)
    {
    return(SUCCESS);
    }

  memset(*I,0,sizeof(midm_t));

  return(SUCCESS);
  }  /* END MIDDestroy() */




/**
 *
 *
 * @param I (I)
 */

int MIDCheckConfig(

  midm_t *I)  /* I */

  {
  if (I == NULL)
    {
    return(FAILURE);
    }

  /* NYI */

  return(SUCCESS);
  }  /* END MIDCheckConfig() */





/**
 *
 *
 * @param IDName (I)
 * @param IP (O)
 */

int MIDAdd(

  char    *IDName,  /* I */
  midm_t **IP)      /* O */

  {
  int     idindex;

  midm_t *I;

  const char *FName = "MIDAdd";

  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (IDName != NULL) ? "IDName" : "NULL",
    (IP != NULL) ? "IP" : "NULL");

  if ((IDName == NULL) || (IDName[0] == '\0'))
    {
    return(FAILURE);
    }

  if (IP != NULL)
    *IP = NULL;

  for (idindex = 0;idindex < MMAX_ID;idindex++)
    {
    I = &MID[idindex];

    if ((I != NULL) && !strcmp(I->Name,IDName))
      {
      /* ID already exists */

      if (IP != NULL)
        *IP = I;

      return(SUCCESS);
      }

    if ((I != NULL) &&
        (I->Name[0] != '\0') &&
        (I->Name[0] != '\1'))
      {
      continue;
      }

    /* empty slot found */

    /* create/initialize new record */

    if (I == NULL)
      {
      I = &MID[idindex];

      if (MIDCreate(IDName,&I) == FAILURE)
        {
        MDB(1,fALL) MLog("ERROR:    cannot alloc memory for ID '%s'\n",
          IDName);

        return(FAILURE);
        }
      }
    else if (I->Name[0] == '\0')
      {
      MUStrCpy(I->Name,IDName,sizeof(I->Name));
      }

    MIDSetDefaults(I);

    MOLoadPvtConfig((void **)I,mxoAM,I->Name,NULL,NULL,NULL);

    if (IP != NULL)
      *IP = I;

    I->Index = idindex;

    /* update ID record */

    /* NOTE:  no checkpoint data for id manager */

    MDB(5,fSTRUCT) MLog("INFO:     ID %s added\n",
      IDName);

    return(SUCCESS);
    }    /* END for (idindex) */

  /* end of table reached */

  MDB(1,fSTRUCT) MLog("ALERT:    ID table overflow.  cannot add %s\n",
    IDName);

  return(SUCCESS);
  }  /* END MIDAdd() */




/**
 *
 *
 * @param IDName (I)
 * @param IP (O)
 */

int MIDCreate(

  char    *IDName,  /* I */
  midm_t **IP)      /* O */

  {
  midm_t *I;

  if (IP == NULL)
    {
    return(FAILURE);
    }

  I = *IP;

  /* use static memory for now */

  memset(I,0,sizeof(midm_t));

  if ((IDName != NULL) && (IDName[0] != '\0'))
    MUStrCpy(I->Name,IDName,sizeof(I->Name));

  return(SUCCESS);
  }  /* END MIDCreate() */




/**
 *
 *
 * @param IDName (I)
 * @param IP (O)
 */

int MIDFind(

  char    *IDName,  /* I */
  midm_t **IP)      /* O */

  {
  /* if found, return success with IP pointing to ID */

  int     idindex;
  midm_t *I;

  if (IP != NULL)
    *IP = NULL;

  if ((IDName == NULL) ||
      (IDName[0] == '\0'))
    {
    return(FAILURE);
    }

  for (idindex = 0;idindex < MMAX_ID;idindex++)
    {
    I = &MID[idindex];

    if ((I == NULL) || (I->Name[0] == '\0') || (I->Name[0] == '\1'))
      {
      break;
      }

    if (strcmp(I->Name,IDName) != 0)
      continue;

    /* ID found */

    if (IP != NULL)
      *IP = I;

    return(SUCCESS);
    }  /* END for (idindex) */

  /* entire table searched */

  return(FAILURE);
  }  /* END MIDFind() */




/**
 *
 *
 * @param I (I) [modified]
 */

int MIDSetDefaults(

  midm_t *I)  /* I (modified) */

  {
  const char *FName = "MIDSetDefaults";

  MDB(1,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (I != NULL) ? I->Name : "NULL");

  if (I == NULL)
    {
    return(FAILURE);
    }

  switch (I->Type)
    {
    default:

      I->RefreshPeriod = mpInfinity;
      I->UpdateRefreshOnFailure = FALSE;

      break;
    }  /* END switch (I->Type) */

  return(SUCCESS);
  }  /* END MIDSetDefaults() */




/**
 *
 *
 * @param I (I)
 * @param U (I)
 * @param G (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MIDUserCreate(

  midm_t   *I,     /* I */
  mgcred_t *U,     /* I */
  mgcred_t *G,     /* I (optional) */
  char     *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((I == NULL) || (U == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid parameters - no user");

    return(FAILURE);
    }

  if (I->CredCreateURL == NULL)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"createcred method not specified");

    return(FAILURE);
    }

  if (I->CredCreateProtocol == mbpNONE)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid createcred method specified");

    return(FAILURE);
    }

  switch (I->CredCreateProtocol)
    {
    case mbpExec:

      if (I->CredCreatePath == NULL)
        {
        if (EMsg != NULL)
          strcpy(EMsg,"cred create URL misconfigured");

        return(FAILURE);
        }
      else
        {
        char tmpLine[MMAX_LINE];
        char Response[MMAX_LINE];

        char UID[MMAX_NAME];
        char GID[MMAX_NAME];
        char HomeDir[MMAX_LINE];

        if (U->OID > 0)
          sprintf(UID,"%d",
            (int)U->OID);
        else
          strcpy(UID,"-");

        if ((G != NULL) && (G->OID > 0))
          sprintf(GID,"%d",(int)G->OID);
        else
          strcpy(GID,"-");
 
        if ((U->HomeDir == NULL) || (U->HomeDir[0] == '\0'))
          sprintf(HomeDir,"-");
        else
          sprintf(HomeDir,"%s",U->HomeDir);
         
        /* FORMAT:  <UNAME> <UID> <GNAME> <GID> <HOMEDIR> */

        snprintf(tmpLine,sizeof(tmpLine),"%s %s %s %s %s %s",
          I->CredCreatePath,
          U->Name,
          UID,
          (G != NULL) ? G->Name : "-",
          GID,
          HomeDir);
 
        if (MUReadPipe(tmpLine,NULL,Response,sizeof(Response),NULL) == FAILURE)
          {
          MDB(1,fNAT) MLog("ALERT:    cannot read output of command '%s'\n",
            tmpLine);

          if (EMsg != NULL)
            strcpy(EMsg,"cannot read output of command");

          return(FAILURE);
          }

        /* NOTE:  process response to obtain UID/GID info */

        U->OID = MUUIDFromName(U->Name,HomeDir,NULL);

        if (U->OID > 0)
          MUStrDup(&U->HomeDir,HomeDir);

        if (G != NULL)
          G->OID = MUGIDFromName(G->Name);
        }  /* END else (I->CredCreatePath == NULL) */

      break;

    default:

      if (EMsg != NULL)
        sprintf(EMsg,"cred creation protocol %s not supported",
          MBaseProtocol[I->CredCreateProtocol]);

      return(FAILURE);

      /* NO-OP */

      break;
    }  /* END switch (I->CredCreateProtocol) */  

  U->ETime = MMAX_TIME;

  if (G != NULL)
    G->ETime = MMAX_TIME;

  return(SUCCESS);
  }  /* END MIDUserCreate() */




/**
 *
 *
 * @param IS (I)
 * @param RE
 * @param String
 * @param DFormat (I)
 * @param Mode (I) [not used]
 */

int MIDShow(

  midm_t    *IS,
  mxml_t    *RE,
  mstring_t *String,
  enum MFormatModeEnum DFormat,
  mbitmap_t *Mode)

  {
  int   idindex;

  int   index;
  int   findex;

  mbool_t FailureRecorded;

  mbool_t ShowHeader = TRUE;

  midm_t *I;
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

  for (idindex = 0;MID[idindex].Name[0] != '\0';idindex++)
    {
    I = &MID[idindex];

    if ((IS != NULL) && (IS != I))
      continue;

    if (I->Name[0] == '\0')
      break;

    P = &I->P;

    if (ShowHeader == TRUE)
      {
      ShowHeader = FALSE;
      }

    /* NOTE:  id managers are valid with no type specified */

    if (DFormat == mfmXML)
      {
      AE = NULL;

      MXMLCreateE(&AE,(char *)MXO[mxoAM]);

      MXMLAddE(DE,AE);

      MXMLSetAttr(AE,(char *)MAMAttr[mamaName],(void *)I->Name,mdfString);
      MXMLSetAttr(AE,(char *)MAMAttr[mamaType],(void *)MAMType[I->Type],mdfString);
      MXMLSetAttr(AE,(char *)MAMAttr[mamaState],(void *)MAMState[I->State],mdfString);
      MXMLSetAttr(AE,(char *)MAMAttr[mamaVersion],(void *)&I->Version,mdfInt);

      MXMLSetAttr(AE,(char *)MAMAttr[mamaHost],(void *)I->Host,mdfString);
      MXMLSetAttr(AE,(char *)MAMAttr[mamaPort],(void *)&I->Port,mdfInt);

      /* show ID policies */

      PE = NULL;

      MXMLCreateE(&PE,"psi");

      if (MPSIToXML(&I->P,PE) == SUCCESS)
        {
        MXMLAddE(AE,PE);
        }
      }
    else
      {
      char Buf[MMAX_LINE];

      MStringAppendF(String,"ID[%s]  State: '%s'  Server: %s:%s\n",
        I->Name,
        MAMState[I->State],
        MIDType[I->Type],
        (I->P.Path != NULL) ? I->P.Path : "---");

      /* NOTE:  display optional ID version, failures, and fault tolerance config */

      if (I->Version > 0)
        {
        MStringAppendF(String,"  Version: %d\n",
          I->Version);
        }

      if (!bmisclear(&I->CredResetList))
        {
        MStringAppendF(String,"  CredResetList: %s\n",
          bmtostring(&I->CredResetList,MXO,Buf));
        }

      if (!bmisclear(&I->CredBlockList))
        {
        MStringAppendF(String,"  CredBlockList: %s\n",
          bmtostring(&I->CredBlockList,MXO,Buf));
        }

      if (I->RefreshPeriod != mpNONE)
        {
        char DString[MMAX_LINE];
        mulong tmpL;

        MStringAppendF(String,"  RefreshPeriod: %s\n",
          MCalPeriodType[I->RefreshPeriod]);

        MULToDString(&I->LastRefresh,DString);

        MStringAppendF(String,"  Last Refresh:  %s",
          DString);

        MStringAppendF(String,"  Next Refresh: ");

        switch (I->RefreshPeriod)
          {
          case mpMinute:

             tmpL = I->LastRefresh + MCONST_MINUTELEN;

             MULToDString(&tmpL,DString);

             MStringAppendF(String," %s\n",
               DString);

             break;

          case mpHour:

             tmpL = I->LastRefresh + MCONST_HOURLEN;

             MULToDString(&tmpL,DString);

             MStringAppendF(String," %s\n",
               DString);

             break;

          case mpDay:

             MStringAppendF(String," Midnight\n");

             break;

          case mpInfinity:
          default:

            MStringAppendF(String," --- (updated on restart only)\n");

            break;
          }
        }

      if (I->CredCreateURL != NULL)
        {
        MStringAppendF(String,"  CreateCredURL: %s\n",
          I->CredCreateURL);

        if (I->CredCreateProtocol == mbpNONE)
          {
          MStringAppendF(String,"  WARNING:  invalid credcreate protocol specified\n");
          }
        else if (I->CredCreatePath == NULL)
          {
          mbool_t IsExe;

          if (MFUGetInfo(I->CredCreatePath,NULL,NULL,&IsExe,NULL) == FAILURE)
            {
            MStringAppendF(String,"  WARNING:  cannot locate credcreate path '%s'\n",
              I->CredCreatePath);
            }
          else if ((I->CredCreateProtocol == mbpExec) && (IsExe == FALSE))
            {
            MStringAppendF(String,"  WARNING:  credcreate path '%s' is not executable\n",
              I->CredCreatePath);
            }
          }
        }    /* END if (I->CredCreateURL != NULL) */

      if (I->OCount[mxoNONE] > 0)
        {
        int oindex;

        MStringAppendF(String,"  Objects Reported:  Total: %d",
          I->OCount[mxoNONE]);

        for (oindex = mxoNONE + 1;oindex < mxoLAST;oindex++)
          {
          if (I->OCount[oindex] != 0)
            {
            MStringAppendF(String,"  %s: %d",
              MXO[oindex],
              I->OCount[oindex]);
            }
          }    /* END for (oindex) */

        MStringAppend(String,"\n");

        if (I->OCount[mxoLAST] > 0)
          {
          MStringAppendF(String,"  WARNING:  %d lines could not be processed\n",
            I->OCount[mxoLAST]);
          }
        }  /* END if (I->OCount[mxoNONE] > 0) */

      /* NOTE:  stats in ms */

      if (P->RespTotalCount[0] > 0)
        {
        MStringAppendF(String,"  ID Performance:  Avg Time: %.2fs  Max Time:  %.2fs  (%d samples)\n",
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
          MStringAppendF(String,"\nID[%s] Failures: \n",
            I->Name);

          FailureRecorded = TRUE;
          }

        MULToDString((mulong *)&P->FailTime[findex],DString);

        MStringAppendF(String,"  %19.19s  %-15s  '%s'\n",
          DString,
          MAMFuncType[P->FailType[findex]],
          (P->FailMsg[findex] != NULL) ? P->FailMsg[findex] : "no msg");
        }  /* END for (index) */

      if (I->MB != NULL)
        {
        char tmpBuf[MMAX_BUFFER];

        MMBPrintMessages(I->MB,mfmHuman,FALSE,-1,tmpBuf,sizeof(tmpBuf));

        MStringAppendF(String,"\n%s",tmpBuf);
        }

      MStringAppendF(String,"\n");
      }  /* END else (DFormat == mfmXML) */
    }    /* END for (idindex) */

  return(SUCCESS);
  }  /* END MIDShow() */




/**
 *
 *
 * @param I (I)
 * @param SOType (I) [optional]
 * @param SO (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MIDLDAPLoadCred(

  midm_t             *I,       /* I */
  enum MXMLOTypeEnum  SOType,  /* I (optional) */
  void               *SO,      /* I (optional) */
  char               *EMsg)    /* O (optional,minsize=MMAX_LINE) */

  {
#ifdef __MLDAP
  static LDAP  *ld = NULL; 

  LDAPMessage  *result, *e; 
  BerElement   *ber; 
  char         *a; 
  char         **vals; 
  int          i, rc; 

  char        *NameP;
  char         SLine[MMAX_LINE];
#endif /* __MLDAP */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (I == NULL)
    {
    return(FAILURE);
    }

#ifdef __MLDAP
  /* NOTE:  attempt to use persistent LDAP connection */

  if (ld == NULL)
    {
    /* get a handle to an LDAP connection. */

    if ((ld = ldap_init(I->ServerName,I->ServerPort)) == NULL ) 
      { 
      if (EMsg != NULL)
        strcpy(EMsg,"cannot initialize LDAP interface");

      return(FAILURE); 
      } 

    /* bind anonymously to the LDAP server. */

    rc = ldap_simple_bind_s(ld,NULL,NULL);

    if (rc != LDAP_SUCCESS) 
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"cannot bind to LDAP interface - %s",
          ldap_err2string(rc));

      return(FAILURE);
      }
    }    /* END if (ld == NULL) */

  /* generate search string */

  NameP = NULL;

  switch (SOType)
    {
    case mxoUser:

      MOGetName(SO,SOType,&NameP);
  
      break;

    default:
    
      /* NYI */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (SOType) */

  /* NOTE:  ou should be case sensitive based on SOType (NYI) */
 
  sprintf(SLine,"uid=%s, ou=People, dc=example,dc=com",
    (NameP == NULL) ? NameP : "*");

  /* search for the entry */
 
  if ((rc = ldap_search_ext_s(
        ld,
        SLine,
        LDAP_SCOPE_BASE, 
        "(objectclass=*)", 
        NULL, 
        0, 
        NULL, 
        NULL, 
        LDAP_NO_LIMIT, 
        LDAP_NO_LIMIT, 
        &result)) != LDAP_SUCCESS)  /* O */
    { 
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"LDAP search failed - %s",
        ldap_err2string(rc)); 

    return(FAILURE); 
    } 

  /* since we are doing a base search, there should be only one matching entry */ 

  e = ldap_first_entry(ld,result); 

  if (e != NULL) 
    { 
    MDB(5,fNAT) MLog("INFO:     found LDAP response for search '%s'\n",
      SLine);

    /* iterate through each attribute in the entry. */ 

    a = ldap_first_attribute(ld,e,&ber);

    for (;a != NULL;a = ldap_next_attribute(ld,e,ber)) 
      { 
      /* for each attribute, print the attribute name and values */ 

      if ((vals = ldap_get_values(ld,e,a)) != NULL) 
        { 
        for (i = 0;vals[i] != NULL;i++) 
          { 
          MDB(7,fNAT) MLog("INFO:     %s: %s\n",
            a, 
            vals[i]); 

          /* process results, create/adjust internal creds */

          /* NYI */
          } 

        ldap_value_free(vals); 
        } 

      ldap_memfree(a); 
      }  /* END for (a) */
 
    if (ber != NULL) 
      { 
      ber_free(ber,0); 
      } 
    }    /* END if (e != NULL) */ 

  if (result != NULL)
    ldap_msgfree(result); 

  /* NOTE:  uncomment below to disable persistent connections */

  /* 
  ldap_unbind(ld);

  ld = NULL;
  */ 
#endif /* __MLDAP */

  return(SUCCESS); 
  }  /* END MIDLDAPLoadCred() */


/* END MIdentity.c */
