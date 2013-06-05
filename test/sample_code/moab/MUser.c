/* HEADER */

/**
 * @file MUser.c
 *
 * Moab Users
 */
 
/* Contains:                                      *
 *  int MOLoadCP(OS,OIndex,Buf)                   *
 *  int MUserAdd(UName,UP)                        *
 *  int MUserInitialize(U,UName)                  *
 *  int MUserFind(UName,U)                        *
 *  char *MUserShow(U,Buf,BufSize,FlagBM)         *
 *  int MCOToXML(O,OIndex,EP,SAList,SCList,SCAList,Mode) *
 *                                                */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  



/**
 * Load general object checkpoint record.
 *
 * OS (I) [optional]
 * OIndex (I)
 * Buf (I)
 */

int MOLoadCP(

  void               *OS,     /* I (optional) */
  enum MXMLOTypeEnum  OIndex, /* I */
  const char         *Buf)    /* I */

  {
  char    tmpHeader[MMAX_NAME + 1];
  char    OName[MMAX_NAME + 1];
 
  const char   *ptr;
 
  void   *O;
 
  long    CkTime;
 
  mxml_t *E = NULL;

  int     rc;

  const char *FName = "MOLoadCP";
 
  MDB(4,fCKPT) MLog("%s(OS,%s,%s)\n",
    FName,
    MXO[OIndex],
    (Buf != NULL) ? Buf : "NULL"); 
 
  if (Buf == NULL)
    {
    return(FAILURE);
    }
 
  /* FORMAT:  <HEADER> <OID> <CKTIME> <USTRING> */
 
  /* load CP header */
 
  rc = sscanf(Buf,"%64s %64s %ld",
    tmpHeader,
    OName,
    &CkTime);

  if (rc != 3)
    {
    MDB(5,fCKPT) MLog("ALERT:    CP line '%s' is corrupt\n",
      Buf);

    return(FAILURE);
    }
 
  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }
 
  if (OS == NULL)
    {
    if (MOGetObject(OIndex,OName,&O,mAdd) != SUCCESS)
      {
      MDB(5,fCKPT) MLog("ALERT:    cannot load CP %s '%s'\n",
        MXO[OIndex],
        OName);
 
      return(FAILURE);
      }
    }
  else
    {
    O = OS;
    }

  switch (OIndex)
    {
    case mxoAcct:
    case mxoGroup:
    case mxoUser:

      if (((mgcred_t *)O)->CPLoaded == TRUE)
        {
        return(SUCCESS);
        }

      break;

    default:

      /* NO-OP */

      break;
    }
 
  if (((ptr = strchr(Buf,'<')) == NULL) ||
      (MXMLFromString(&E,ptr,NULL,NULL) == FAILURE))
    {
    MDB(5,fCKPT) MLog("ALERT:    cannot process CP %s XML string '%s'\n",
      MXO[OIndex],
      OName);

    return(FAILURE);
    }

  if (MOFromXML(O,OIndex,E,NULL) == FAILURE)
    {
    MXMLDestroyE(&E);

    if (OIndex == mxoTrig)
      {
      /* trigger could not be restored, clean it up (delete) */

      MODeleteTPtr((mtrig_t *)O);

      MTrigFree((mtrig_t **)&O);
      }

    return(FAILURE);
    }

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MOLoadCP() */





/**
 * Convert credential object to corresponding XML string.
 *
 * NOTE:  To checkpoint user/group/account attributes, add mca* entry
 *        to CPAList[] below.
 *
 * @see MCOToXML() - child - convert cred object to XML
 * @see MCPStoreCredList() - parent
 *
 * @param O (I)
 * @param OIndex (I)
 * @param DoCP (I)
 * @param String
 */

int MCredToString(

  void               *O,
  enum MXMLOTypeEnum  OIndex,
  mbool_t             DoCP,
  mstring_t          *String)

  {
  int index;

  const enum MXMLOTypeEnum CPCList[] = {
    mxoxStats,
    mxoNONE };

  mxml_t *E = NULL;

  enum MCredAttrEnum CPAList[mcaLAST]; /* could potentially hold all possible attributes */
 
  if ((O == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  index = 0;

  CPAList[index++] = mcaID;

  /* determine whether or not to print out the Account access list */

  if ((MCP.CheckPointAllAccountLists == TRUE) || (MCredAttrSetByUser(O,OIndex,mcaAList) == TRUE))
    {
    CPAList[index++] = mcaAList;
    CPAList[index++] = mcaNONE;
    }
  else
    {
    CPAList[index++] = mcaNONE;
    }
 
  MCOToXML(
    (void *)O,
    OIndex,
    &E,
    (DoCP == TRUE) ? (enum MCredAttrEnum *)CPAList : NULL,
    (enum MXMLOTypeEnum *)CPCList,
    NULL,
    (DoCP == TRUE) ? 0 : (1 << mcmVerbose),
    FALSE);
 
  MXMLToMString(E,String,(char const **) NULL,TRUE);
 
  MXMLDestroyE(&E);
 
  return(SUCCESS);
  }  /* END MCredToString() */





static mmutex_t MUserWriteLock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Add new user object by name.
 *
 * @see MUserFind()
 *
 * @param UName (I)
 * @param UP (O) [optional]
 */

int MUserAdd(
 
  const char  *UName,
  mgcred_t   **UP)
 
  {
  char      tmpName[MMAX_NAME];

  int       cindex;
 
  int       rc;

  mulong    hashkey;
 
  mgcred_t *U;

  char      EMsg[MMAX_LINE];
  const char *FName = "MUserAdd";
 
  MDB(6,fSTRUCT) MLog("%s(%.16s,%s)\n",
    FName,
    (UName != NULL) ? UName : "NULL",
    (UP != NULL) ? "UP" : "NULL");

  if (UP != NULL)
    *UP = NULL;

  if ((UName == NULL) || (UName[0] == '\0'))
    {
    return(FAILURE);
    }

  MUStrCpy(tmpName,UName,sizeof(tmpName));

  /* remove spaces from user names */

  for (cindex = 0;tmpName[cindex] != '\0';cindex++)
    {
    if (isspace(tmpName[cindex]))
      tmpName[cindex] = '_';
    }

  /* NOTE:  this may be a bug - just checking for 'peer:', not 'peer:<X>' */

  if (!strcasecmp("PEER:",tmpName))
    {
    /* there is no name associated with this PEER: prefix */

    return(FAILURE);
    }
 
  if (UP != NULL)
    *UP = NULL;
 
  hashkey = MAX(1,MUGetHash(tmpName) % MMAX_CREDS);

  rc = FAILURE;
 
  MUMutexLock(&MUserWriteLock);

  /* Look for existing user */

  if ((MUserFind(tmpName,&U) == SUCCESS) && (U != NULL))
    {
    /* Found existing user, return.  */

    if (UP != NULL)
      *UP = U;

    MUMutexUnlock(&MUserWriteLock);

    return SUCCESS;
    }
  else
    {
    if (MUserCreate(tmpName,&U) == FAILURE)
      {
      MDB(1,fALL) MLog("ERROR:    cannot alloc memory for user '%s'\n",
        tmpName);

      MUMutexUnlock(&MUserWriteLock);

      return FAILURE;
      }

    /* Add user to hash table */

    MUHTAdd(&MUserHT,tmpName,U,NULL,NULL);
    }
     
  if (UP != NULL)
    *UP = U;

  U->Key = hashkey;

  MUStrCpy(U->Name,tmpName,sizeof(U->Name));

  MCredAdjustConfig(mxoUser,U,TRUE);

  if (strcmp(tmpName,ALL) && 
      strcmp(tmpName,NONE) && 
      strcmp(tmpName,DEFAULT) && 
      strcmp(tmpName,"ALL") &&
      strcmp(tmpName,"DEFAULT"))
    {
    char tmpLine[MMAX_LINE];

    /* update user record */

    MCredAdjustConfig(mxoUser,U,FALSE);

/*
    MCPRestore(mcpUser,tmpName,(void *)U,NULL);
*/

    if (U->HomeDir == NULL)
      {
      if ((U->HomeDir = (char *)MUMalloc(sizeof(char) * MMAX_LINE)) != NULL)
        U->HomeDir[0] = '\0';
      }

    if (strncasecmp(U->Name,"peer:",strlen("peer:")))
      { 
      switch (MSched.OSCredLookup)
        {
        case mapForce:

          U->OID = MUUIDFromName(U->Name,U->HomeDir,EMsg);

          if (U->OID == -1)
            {
            /* cannot locally look up UID */

            MUStrDup(&MSched.FailedOSLookupUser,tmpName);
            MUStrDup(&MSched.FailedOSLookupUserMsg,EMsg);

            MMBAdd(&U->MB,EMsg,NULL,mmbtNONE,0,0,NULL);

            MDB(1,fALL) MLog("WARNING:  cannot locate OS information for user '%s' - ignoring user - %s\n",
              tmpName,
              EMsg);

            MCredDestroy(&U);

            MUFree((char**)&U);

            rc = FAILURE;

            break;
            }

          break;

        case mapBestEffort:
        default:

          U->OID = MUUIDFromName(U->Name,U->HomeDir,EMsg);

          if (U->OID == -1)
            {
            MUStrDup(&MSched.FailedOSLookupUser,tmpName);
            MUStrDup(&MSched.FailedOSLookupUserMsg,EMsg);

            MDB(3,fALL) MLog("INFO:     cannot locate OS UID information for user '%s' - continuing - %s\n",
              tmpName,
              EMsg);
            }

          break;

        case mapNever:
   
          U->OID = -1;

          break;
        }  /* END switch (MSched.OSCredLookup) */
      }    /* END if (strncasecmp(U->Name,"peer:",strlen("peer:"))) */
    else
      {
      /* peers do not have UID */

      U->OID = -1;
      }

    /* load default group (NOTE:  can be overwritten by config) */

    /* add primary group to GDef, secondary groups to GAL */

    if (U->OID != -1) 
      {
      MDB(7,fSTRUCT) MLog("INFO:     loading group info for user %s (UID: %ld) - CredLookup: %s\n",
        U->Name,
        U->OID,
        MActionPolicy[MSched.OSCredLookup]);

      /* in the future we should default to FastGroupLookup */

      if (((bmisset(&MSched.Flags,mschedfFastGroupLookup)) && 
            (MUNameGetGroups(U->Name,-1,NULL,tmpLine,NULL,sizeof(tmpLine)) == SUCCESS)) ||
          ((!bmisset(&MSched.Flags,mschedfFastGroupLookup)) &&
            (MOSGetGListFromUName(U->Name,tmpLine,NULL,sizeof(tmpLine),NULL) == SUCCESS)))
        {
        char *ptr;
        char *TokPtr;

        char GName[MMAX_NAME];

        int GID;

        ptr = MUStrTok(tmpLine,",",&TokPtr);

        /* NOTE:  sanity check primary group */

        if ((GID = MUGIDFromUser(-1,U->Name,NULL)) < 0)
          {
          /* NOTE:  user created but cannot locate user record in local user database */

          if (MSched.OSCredLookup == mapForce)
            {
            MUStrDup(&MSched.FailedOSLookupUser,tmpName);

            MDB(1,fALL) MLog("WARNING:  cannot locate OS GID information for user '%s' - ignoring user\n",
              tmpName);

            MCredDestroy(&U);

            MUFree((char**)&U);

            MUMutexUnlock(&MUserWriteLock);
  
            return FAILURE;
            }
          }

        MUGIDToName(GID,GName);

        if ((GName[0] != '\0') && strcmp(GName,NONE) && strncmp(GName,"GID",3))
          {
          MGroupAdd(GName,(mgcred_t **)&U->F.GDef);

          MULLAdd(&U->F.GAL,GName,NULL,NULL,NULL); /* no free routine */
          }

        while (ptr != NULL)
          {
          if (bmisset(&MSched.Flags,mschedfExtendedGroupSupport))
            MGroupAdd(ptr,NULL);

          MULLAdd(&U->F.GAL,ptr,NULL,NULL,NULL); /* no free routine */

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END if (MOSGetGListFromUName() == SUCCESS) */
      else
        {
        if (MSched.OSCredLookup == mapForce)
          {
          MUStrDup(&MSched.FailedOSLookupUser,tmpName);

          MDB(1,fALL) MLog("WARNING:  cannot locate OS group list information for user '%s' - ignoring user\n",
            tmpName);

          MCredDestroy(&U);

          MUFree((char**)&U);

          MUMutexUnlock(&MUserWriteLock);

          return FAILURE;
          }
        }    /* END else (MOSGetGListFromUName() == SUCCESS) */ 
      }      /* END if (U->OID != -1) */

    MDB(5,fSTRUCT) MLog("INFO:     user %s added\n",
      tmpName);
    }  /* END if (strcmp(tmpName,ALL) && ...) */

  rc = SUCCESS;

  MUMutexUnlock(&MUserWriteLock);

  return(rc);
  }  /* END MUserAdd() */


/**
 * Locate user object by name.
 *
 * @see MUserAdd()
 *
 * @param UName (I)
 * @param UP (O) [optional]
 */

int MUserFind(

  const char *UName,
  mgcred_t  **UP)

  {
  /* if found, return success with UP pointing to user. */

  /* MUHTGet can take a NULL pointer, so we don't need to check for that */

  if ((UName != NULL) && (UName[0] != '\0'))
    return MUHTGet(&MUserHT,UName,(void**)UP,NULL);

  return(FAILURE);
  }  /* END MUserFind() */




/**
 * Report user config and diagnostics.
 *
 * NOTE:  Handles 'mdiag -u'
 *
 * @param U         (I)
 * @param Buffer    (O) [optional]
 * @param Verbose   (I)
 * @param Partition (I) [optional]
 */

int MUserShow(

  mgcred_t  *U,
  mstring_t *Buffer,
  mbool_t    Verbose,
  char      *Partition)

  {
  char QALLine[MMAX_LINE];
  char QALChar;

  char ULLine[MMAX_LINE];

  const char *FormatString;

  char  tmpLString[MMAX_LINE];

  int   aindex;

  mbool_t AIsSet;

  mxml_t *CE = NULL;  /* prep for enabling XML reporting */

  mpar_t *P = NULL;

  enum MFormatModeEnum DFormat = mfmHuman;

  enum MCredAttrEnum CAList[] = {
    mcaADef,
    mcaAList,
    mcaHold,
    mcaJobFlags,
    mcaManagers,
    mcaMaxIJob,
    mcaMaxIProc,
    mcaMaxProcPerUser,
    mcaMaxNodePerUser,
    mcaMaxJobPerUser,
    mcaDefWCLimit,
    mcaMaxWCLimitPerJob,
    mcaMaxNodePerJob,
    mcaMaxProcPerJob,
    mcaNONE };

  const char *FName = "MUserShow";

  MDB(3,fUI) MLog("%s(%s,Buf)\n",
    FName,
    (U != NULL) ? U->Name : "NULL");

  if (Buffer == NULL)
    {
    return(FAILURE);
    }

  if (MUStrNCmpCI(Partition,"ALL",sizeof("ALL")) || (MParFind(Partition,&P) == FAILURE))
    {
    /* Invalid partition */

    P = NULL;
    }

  if (U == NULL)
    {
    /* build header */

    /*                         NAME PRI FLAG QDEF QLST * PLST TRG LIMITS */
 
    MStringAppendF(Buffer,"%-25s %8s %12s %12s %12s%s %20s %6s %7s\n\n",
      "Name",
      "Priority",
      "Flags",
      "QDef",
      "QOSList",
      "*",
      "PartitionList",
      "Target",
      "Limits"); 

    /* NOTE:  moved FailedOSLookupUser info to user message buffer */

#ifdef MNOT
    if (bmisset(FlagBM,mcmVerbose))
      {
      if (MSched.FailedOSLookupUser != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"WARNING:  cannot load OS information for user %s\n",
          MSched.FailedOSLookupUser);
        }

      if (MSched.FailedOSLookupUserMsg != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"WARNING:  %s\n",
          MSched.FailedOSLookupUserMsg);
        }
      }
#endif /* MNOT */
    }
  else
    {
    mstring_t FlagLine(MMAX_LINE);

    /* build job info line */

    MJobFlagsToMString(NULL,&U->F.JobFlags,&FlagLine);

    if (MUStrIsEmpty(FlagLine.c_str()))
      MStringSet(&FlagLine,"-");
 
    MQOSBMToString(&U->F.QAL,QALLine);

    if (U->F.QALType == malAND)
      QALChar = '&';
    else if (U->F.QALType == malOnly)
      QALChar = '^';
    else
      QALChar = ' ';

    MCredShowAttrs(
      &U->L,
      &U->L.ActivePolicy,
      U->L.IdlePolicy,
      U->L.OverrideActivePolicy,
      U->L.OverrideIdlePolicy,
      U->L.OverriceJobPolicy,
      U->L.RsvPolicy,
      U->L.RsvActivePolicy,
      &U->F,
      0,
      FALSE,
      TRUE,
      ULLine);
 
    /*            NAME PRIO FLAG QDEF QLST * PLST FSTARG LIMITS */

    if (Verbose == TRUE)
      {
      FormatString = "%s %8ld %12s %12s %12s%c %20s %6.2f %7s\n";
      }
    else
      {
      FormatString = "%-25.25s %8ld %12.12s %12.12s %12.12s%c %20.20s %6.2f %7s\n";
      }
 
    MStringAppendF(Buffer,FormatString,
      U->Name,
      U->F.Priority,
      FlagLine.c_str(),
      ((mqos_t *)U->F.QDef[0]) != NULL ?
        ((mqos_t *)U->F.QDef[0])->Name  :
        "-",
      (QALLine[0] != '\0') ? QALLine : "-",
      QALChar,
      (bmisclear(&U->F.PAL)) ?
        "-" :
        MPALToString(&U->F.PAL,NULL,tmpLString),
      U->F.FSTarget,
      (ULLine[0] != '\0') ? ULLine : "-");

    /* display additional attributes */

    if (U->ProxyList != NULL)
      {
      MStringAppendF(Buffer,"  PROXYLIST=%s\n",
        U->ProxyList);
      }
  
    if (!bmisclear(&U->JobNodeMatchBM))
      {
      char tmpLine[MMAX_LINE];

      MStringAppendF(Buffer,"  JOBNODEMATCHPOLICY=%s\n",
        bmtostring(&U->JobNodeMatchBM,MJobNodeMatchType,tmpLine));
      }
 
    if (U->ValidateProxy == TRUE)
      {
      MStringAppendF(Buffer,"  VALIDATEPROXY=%s\n",
        MBool[TRUE]);
      }

    if (U->F.FSCap != 0.0)
      {
      MStringAppendF(Buffer,"  FSCAP=%f\n",
        U->F.FSCap);
      }

    if (U->F.CDef != NULL)
      {
      MStringAppendF(Buffer,"  CDEF=%s\n",
        ((mclass_t *)U->F.CDef)->Name);
      }

    if (U->F.GDef != NULL)
      {
      MStringAppendF(Buffer,"  GDEF=%s\n",
        ((mpar_t *)U->F.GDef)->Name);
      }

    if (U->F.PDef != NULL)
      {
      MStringAppendF(Buffer,"  PDEF=%s\n",
        ((mpar_t *)U->F.PDef)->Name);
      }

    if (Verbose == TRUE)
      {
      if (U->OID > 0)
        {
        MStringAppendF(Buffer,"  UID=%ld\n",
          U->OID);
        }
      }

    if (U->EMailAddress != NULL)
      {
      MStringAppendF(Buffer,"  EMAILADDRESS=%s\n",
        U->EMailAddress);
      }

    if (U->NoEmail == TRUE)
      {
      MStringAppendF(Buffer,"  (email is disabled)\n");
      } 

    if (Verbose == TRUE)
      {
      if (U->Stat.QOSSatJC > 0)
        {
        MStringAppendF(Buffer,"  QOSRate=%.2f%c\n",
          (double)U->Stat.QOSSatJC / MAX(1,U->Stat.JC) * 100.0,
          '%');
        }

      if (U->ProfilingIsEnabled == TRUE)
        {
        MStringAppendF(Buffer,"  EnableProfiling=%s\n",
          MBool[U->ProfilingIsEnabled]);
        }
      }

    if (U->F.ReqRsv != NULL)
      {
      MStringAppendF(Buffer,"  NOTE:  user %s is tied to rsv '%s'\n",
        U->Name,
        U->F.ReqRsv);
      }

    if (U->L.GroupActivePolicy != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(U->L.GroupActivePolicy,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        if (P->SLimit[mptMaxProc][0] > 0)
          {
          /* Get the group and format the data for output */

          MStringAppendF(Buffer,"  MAXPROC[GROUP:%s]=%d,%d\n",
            (P->Ptr != NULL) ? ((mgcred_t *)P->Ptr)->Name : "-",
            P->SLimit[mptMaxProc][0],
            P->HLimit[mptMaxProc][0]);
          }
        }  /* END for (gindex) */
      }    /* END if (U->L.APG != NULL) */

    /* list extended cred attributes */

    AIsSet = FALSE;

    mstring_t tmpString(MMAX_LINE);

    for (aindex = 0;CAList[aindex] != mcaNONE;aindex++)
      {
      if ((MCredAToMString(
             (void *)U,
             mxoUser,
             CAList[aindex],
             &tmpString,     /* O */
             mfmNONE,
             0) == SUCCESS) &&
          (!MUStrIsEmpty(tmpString.c_str())))
        {
        if (DFormat == mfmXML)
          {
          MXMLSetAttr(CE,(char *)MCredAttr[CAList[aindex]],(void *)tmpString.c_str(),mdfString);
          }
        else if (CAList[aindex] == mcaAList)
          {
          /* Filter Account List by access to partition P */

          if (P != NULL)
            {
            char      Buffer[MMAX_LINE]; /* build output here then move it to QALLine */
            mbool_t   First = TRUE;   /* keeps track of when to insert a comma */
            char     *Ptr;
            char     *tmp;

            memset(Buffer,'\0',sizeof(Buffer));

            /* need a mutable string to MUStrTok on */
            char *mutableString = NULL;
            MUStrDup(&mutableString,tmpString.c_str());

            Ptr = MUStrTok(mutableString,",",&tmp);

            while (Ptr != NULL)
              {
              mbool_t   FoundInFairshare = FALSE;
              int       Depth;
              mgcred_t *Acct;

              MAcctFind(Ptr,&Acct);

              /* see if the user is found under this account in the fairshare tree for this partition */

              if (P->FSC.ShareTree != NULL)
                {
                mxml_t *Element; /* place holder for partition filtering */

                /* find the account element under the given partition */

                Depth = -1; /* traverse the entire tree until a matching account is found */

                Element = MFSCredFindElement(P->FSC.ShareTree,mxoAcct,Acct->Name,Depth,&FoundInFairshare);

                if (FoundInFairshare == TRUE)
                  {
                  /* the account was found in the tree, now look for the user under that account */

                  FoundInFairshare = FALSE; /* reset the boolean to look in account for individual user */

                  /* search fairshare tree immediately below the account for the given user */

                  Depth = 1; /* search the children directly below this account for a matching user*/

                  MFSCredFindElement(Element,mxoUser,U->Name,Depth,&FoundInFairshare);
                  }
                } /* END if (P->FSC.ShareTree != NULL) */

              if (bmisset(&Acct->F.PAL,P->Index) || FoundInFairshare)
                {
                /* if not first, add a comma */

                if (!First)
                  MUStrCat(Buffer,",",sizeof(Buffer));
                else
                  First = FALSE;

                /* add Acct */

                MUStrCat(Buffer,Ptr,sizeof(Buffer));
                }

              Ptr = MUStrTok(NULL,",",&tmp);
              } /* END while (ptr != NULL) */

            MUFree(&mutableString);

            tmpString = Buffer;
            } /* END if (P != NULL) */

          if (!tmpString.empty())
            {
            MStringAppendF(Buffer,"  %s=%s",
              MCredAttr[CAList[aindex]],
              tmpString.c_str());
            }
          }
        else
          {
          MStringAppendF(Buffer,"  %s=%s",
            MCredAttr[CAList[aindex]],
            tmpString.c_str());
          }

        AIsSet = TRUE;
        }
      }    /* END for (aindex) */

    if (AIsSet == TRUE)
      {
      MStringAppendF(Buffer,"\n");
      }

    if (U->Variables != NULL)
      {
      /* display variables */

      MStringAppendF(Buffer,"  VARIABLE=%s\n",
        U->Variables->Name);
      MStringAppendF(Buffer,"\n");
      }

    if (U->MB != NULL)
      {
      char tmpBuf[MMAX_BUFFER];

      /* display messages */

      tmpBuf[0] = '\0';

      MMBPrintMessages(
        U->MB,
        mfmHuman,
        FALSE,
        -1,
        tmpBuf,
        sizeof(tmpBuf));

      MStringAppend(Buffer,tmpBuf);
      }

    if (U->Stat.IStat != NULL)
      {
      char TString[MMAX_LINE];

      MULToTString(MStat.P.IStatDuration,TString);

      MStringAppendF(Buffer,"  Message:  profiling enabled (%d of %d samples/%s interval)\n",
        U->Stat.IStatCount,
        MStat.P.MaxIStatCount,
        TString);
      }
    }      /* END else (U == NULL) */

  return(SUCCESS);
  }  /* END MUserShow() */



/**
 * Report user attribute as string.
 *
 * @see MUserSetAttr() - peer
 *
 * @param U (I)
 * @param AIndex (I)
 * @param String (O)
 * @param Format (I)
 */

int MUserAToString(

  mgcred_t            *U,
  enum MUserAttrEnum   AIndex,
  mstring_t           *String,
  int                  Format)

  {
  if (String != NULL)
    MStringSet(String,"\0");

  if ((U == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case muaViewpointProxy:

      if (U->ViewpointProxy == TRUE)
        MStringSetF(String,"%s",MBool[U->ViewpointProxy]);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MUserAToString() */








/**
 *
 *
 * @param UName (I) [optional]
 * @param UP (O)
 */

int MUserCreate(

  const char   *UName,
  mgcred_t    **UP)

  {
  mgcred_t *U;

  if ((UName == NULL) || (UName[0] == '\0') || (UP == NULL))
    {
    return(FAILURE);
    }

  *UP = NULL;

  if ((U = (mgcred_t *)MUCalloc(1,sizeof(mgcred_t))) == NULL)
    {
    MDB(1,fALL) MLog("ERROR:    cannot alloc memory for user '%s'\n",
      UName);
 
    return(FAILURE);
    }

  MSched.A[mxoUser]++;

  /* allocate dynamic memory */

  MPUInitialize(&U->L.ActivePolicy);
  MPUCreate(&U->L.IdlePolicy);

  U->Stat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

  *UP = U;

  /* NOTE:  users not destroyed */

  return(SUCCESS);
  }  /* END MUserCreate() */





/**
 * Convert credential object to XML.
 *
 * @see MCredToString() - parent
 * @see MOToXML() - child - convert limits, fs, stats to XML
 *
 * NOTE:  This routine determines which object are checkpointed 
 *        @see MCPStoreCredList()->MCredToString()
 *
 * @param O            (I)
 * @param OIndex       (I)
 * @param EP           (O)
 * @param SAList       (I) [optional]
 * @param SCList       (I) [optional]
 * @param SCAList      (I) [optional]
 * @param ClientOutput (I) 
 * @param Mode         (I) [bitmap of enum mcm*]
 */

int MCOToXML(

  void                *O,
  enum MXMLOTypeEnum   OIndex,
  mxml_t             **EP,
  void                *SAList,
  enum MXMLOTypeEnum  *SCList,
  void               **SCAList,
  mbool_t              ClientOutput,
  mbitmap_t           *Mode)

  {
  const enum MCredAttrEnum DAList[] = {
    mcaID,
    mcaNONE };

  const enum MXMLOTypeEnum DCList[] = {
    mxoxStats,
    mxoNONE };
 
  int aindex;

  int cindex;
 
  enum MCredAttrEnum *AList;
  enum MXMLOTypeEnum *CList;

  mxml_t *XC;

  void *C;
 
  if ((O == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }
 
  if (SAList != NULL)
    AList = (enum MCredAttrEnum *)SAList;
  else
    AList = (enum MCredAttrEnum *)DAList;

  if (SCList != NULL)
    CList = SCList;
  else
    CList = (enum MXMLOTypeEnum *)DCList;

  MXMLCreateE(EP,(char *)MXO[OIndex]);     

  switch (OIndex)
    {
    case mxoPar:

      {
      char *NameP = NULL;

      if (MOGetName(O,OIndex,&NameP) != NULL)
        {
        MXMLSetAttr(
          *EP,
          (char *)MParAttr[mpaID],
          NameP,
          mdfString);
        }
      }  /* END BLOCK */

      break;
      
    case mxoUser:
    case mxoGroup:
    case mxoAcct:
    case mxoClass:
    case mxoQOS:
 
      {
      mstring_t tmpString(MMAX_LINE);

      for (aindex = 0;AList[aindex] != mcaNONE;aindex++)
        {
        if (AList[aindex] == mcaCSpecific)
          {
          switch (OIndex)
            {
            case mxoUser:

              {
              int uaindex;
              int rc;

              const enum MUserAttrEnum UAList[] = {
                muaViewpointProxy,
                muaLAST };
         
              for (uaindex = 0;UAList[uaindex] != muaLAST;uaindex++)
                {
                rc = MUserAToString(
                      (mgcred_t *)O,
                      UAList[uaindex],
                      &tmpString,
                      mdfString);

                if ((rc == SUCCESS) &&
                    (!MUStrIsEmpty(tmpString.c_str())))
                  {
                  MXMLSetAttr(
                    *EP,
                    (char *)MUserAttr[UAList[uaindex]],
                    tmpString.c_str(),
                    mdfString);
                  }
                }    /* END for (uaindex) */
              }      /* END BLOCK */

              break;

            case mxoQOS:

              {
              int qaindex;
              int rc;

              const enum MQOSAttrEnum QAList[] = {
                mqaXFWeight,
                mqaQTWeight,
                mqaXFTarget,
                mqaQTTarget,
                mqaFlags,
                mqaFSTarget,
                mqaBLTriggerThreshold,
                mqaQTTriggerThreshold,
                mqaXFTriggerThreshold,
                mqaBLPowerThreshold,
                mqaQTPowerThreshold,
                mqaXFPowerThreshold,
                mqaBLPreemptThreshold,
                mqaQTPreemptThreshold,
                mqaXFPreemptThreshold,
                mqaBLRsvThreshold,
                mqaQTRsvThreshold,
                mqaXFRsvThreshold,
                mqaBLACLThreshold,
                mqaQTACLThreshold,  /* queue time based ACL threshold */
                mqaXFACLThreshold,
                mqaDedResCost,
                mqaUtlResCost,
                mqaNONE };

              for (qaindex = 0;QAList[qaindex] != mqaNONE;qaindex++)
                {
                rc = MQOSAToMString(
                      (mqos_t *)O,
                      QAList[qaindex],
                      &tmpString,
                      mdfString);

                if ((rc == SUCCESS) &&
                    (!tmpString.empty()))
                  {
                  MXMLSetAttr(
                    *EP,
                    (char *)MQOSAttr[QAList[qaindex]],
                    tmpString.c_str(),
                    mdfString);
                  }
                }    /* END for (caindex) */
              }      /* END BLOCK */

              break;

            case mxoClass:

              {
              int caindex;
              int rc;

              const enum MClassAttrEnum CAList[] = {
                mclaRM,
                mclaNONE };
         
              for (caindex = 0;CAList[caindex] != mclaNONE;caindex++)
                {
                rc = MClassAToMString(
                      (mclass_t *)O,
                      CAList[caindex],
                      &tmpString);

                if ((rc == SUCCESS) &&
                    (!tmpString.empty()))
                  {
                  MXMLSetAttr(
                    *EP,
                    (char *)MClassAttr[CAList[caindex]],
                    tmpString.c_str(),
                    mdfString);
                  }
                }    /* END for (caindex) */
              }      /* END BLOCK */

              break;
    
            default:

              /* NO-OP */

              break;
            }  /* END switch (OIndex) */
 
          continue;
          }  /* END if (AList[aindex] == mcaCSpecific) */

        if ((MCredAToMString(O,OIndex,AList[aindex],&tmpString,mfmNONE,Mode) == FAILURE) ||
            (tmpString.empty()))
          {
          continue;
          }
 
        MXMLSetAttr(*EP,(char *)MCredAttr[AList[aindex]],tmpString.c_str(),mdfString);
        }  /* END for (aindex) */

      for (cindex = 0;CList[cindex] != mxoNONE;cindex++)
        {
        XC = NULL;

        if ((MOGetComponent(O,OIndex,&C,CList[cindex]) == FAILURE) ||   
            (MOToXML(
               C,
               CList[cindex],
               (SCAList != NULL) ? SCAList[cindex] : NULL,
               ClientOutput,
               &XC) == FAILURE))
          {
          continue;
          }

        MXMLAddE(*EP,XC);
        }  /* END for (cindex) */

      /* add messages */

      if ((MCredAToMString(O,OIndex,mcaMessages,&tmpString,mfmXML,Mode) != FAILURE) &&
          (!tmpString.empty()))
        {
        mxml_t *ME = NULL;

        if (MXMLFromString(&ME,tmpString.c_str(),NULL,NULL) != FAILURE)
          {
          MXMLAddE(*EP,ME);
          }
        }

      }  /* END block */

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (OIndex) */

  return(SUCCESS);
  }  /* END MCOToXML() */


/**
 * Parse a privilege line and insert it into the list.
 *
 * @param List (should be initialized)
 * @param Line
 * @param EMsg
 */

int MPrivilegeParse(

  mln_t     **List,
  const char *Line,
  char       *EMsg)

  {
  char *String = NULL;

  mprivilege_t  New;
  mprivilege_t  doug;
  mprivilege_t *NewP;

  char *ptr;
  char *TokPtr = NULL;

  enum MObjectActionEnum Action;

  /* FORMAT: OTYPE:<create|list|modify|destroy|diagnose>[,<>] */

  if ((List == NULL) || (MUStrIsEmpty(Line)) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  memset(&New,0,sizeof(New));

  MUStrDup(&String,Line);

  ptr = MUStrTok(String,"*:",&TokPtr);

  /* Get the Object type */
  New.OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

  if (New.OType == mxoNONE)
    {
    snprintf(EMsg,MMAX_LINE,"invalid object type specified (%s)",ptr);

    MUFree(&String);

    return(FAILURE);
    }

  ptr = MUStrTok(NULL,",",&TokPtr);

  if (MUStrIsEmpty(ptr))
    {
    snprintf(EMsg,MMAX_LINE,"invalid privilege");

    MUFree(&String);

    return(FAILURE);
    }
 
  /* Lookup the Object Actions, that might be on the line */
  while (ptr != NULL)
    {
    Action = (enum MObjectActionEnum)MUGetIndexCI(ptr,MObjectAction,FALSE,moaNONE);

    if (Action == moaNONE)
      {
      snprintf(EMsg,MMAX_LINE,"invalid privilege");

      MUFree(&String);

      return(FAILURE);
      }

    bmset(&New.Perms,Action);

    ptr = MUStrTok(NULL,",",&TokPtr);
    }   /* END while (ptr != NULL) */

  /* Get a new instance of mprivilege_t of data */
  NewP = (mprivilege_t *)MUCalloc(1,sizeof(mprivilege_t));
  
  /* Copy our result into that new instance to return it */
  *NewP = New;

  MULLAdd(List,MXO[NewP->OType],NewP,NULL,MUFREE);

  MUFree(&String);

  return(SUCCESS);
  }  /* END MPrivilegeParse() */


/**
 * Process user-specific config attributes.
 *
 * @see MCredProcessConfig()
 *
 * @param U     (I)
 * @param Value (I)
 * @param IsPar (I)
 */

int MUserProcessConfig(

  mgcred_t *U,
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

  const char *FName = "MUserProcessConfig";

  MDB(5,fCONFIG) MLog("%s(%s,%s)\n",
    FName,
    (U != NULL) ? U->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((U == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  if (IsPar == TRUE)
    {
    MParSetupRestrictedCfgBMs(NULL,NULL,NULL,NULL,&ValidBM,NULL,NULL);
    }

  /* process value line */

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    if (MUGetPair(
          ptr,
          (const char **)MUserAttr,
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

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for user %s\n",
        ptr,
        U->Name);

      if (MMBAdd(&U->MB,tmpLine,NULL,mmbtNONE,0,0,&M) == SUCCESS)
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

    switch (aindex)
      {
      case muaJobNodeMatchPolicy:

        bmclear(&U->JobNodeMatchBM);

        bmfromstring((char *)ValLine,MJobNodeMatchType,&U->JobNodeMatchBM);

        break;

      case muaNoEmail:

        U->NoEmail = MUBoolFromString((char *)ValLine,FALSE);

        break;

      case muaProxyList:

        if (!strcasecmp(ValLine,"validate"))
          U->ValidateProxy = TRUE;
        else
          MUStrDup(&U->ProxyList,ValLine);

        break;

      case muaViewpointProxy:

        U->ViewpointProxy = MUBoolFromString((char *)ValLine,FALSE);

        break;

      case muaPrivileges:

        {
        char EMsg[MMAX_LINE];

        if (MPrivilegeParse(&U->Privileges,(char *)ValLine,EMsg) == FAILURE)
          {
          MDB(1,fCKPT) MLog("WARNING:   could not process PRIVILEGE parameter (%s)\n",
            EMsg);
          }
        }
       
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
  }  /* END MUserProcessConfig() */





/**
 * Get default credential associated with specified user.
 *
 * @param U (I)
 * @param G (O) [optional]
 * @param C (O) [optional]
 * @param A (O) [optional]
 * @param Q (O) [optional]
 */

int MUserGetCreds(

  mgcred_t  *U, /* I */
  mgcred_t **G, /* O (optional) */
  mgcred_t **C, /* O (optional) */
  mgcred_t **A, /* O (optional) */
  mqos_t   **Q) /* O (optional) */

  {
  if ((U == NULL) || (U->Name[0] == '\0'))
    {
    return(FAILURE);
    }

  if (G != NULL)
    {
    /* populate G with user's default group */

    if (U->F.GDef != NULL)
      {
      *G = (mgcred_t *)U->F.GDef;
      }
    else
      {
      *G = NULL;
      }
    }

  if (C != NULL)
    {
    /* populate C with user's default class */

    *C = NULL;

    /* NYI */
    }

  if (A != NULL)
    {
    /* populate A with user's default account */

    if (U->F.ADef != NULL)
      {
      *A = (mgcred_t *)U->F.ADef;
      }
    else
      {
      *A = NULL;
      }
    }

  if (Q != NULL)
    {
    /* populate Q with user's default QOS */

    if (U->F.QDef[0] != NULL)
      {
      *Q = (mqos_t *)U->F.QDef[0];
      }
    else
      {
      *Q = NULL;
      }
    }

  return(SUCCESS);
  }  /* END MUserGetCreds() */





/**
 * Verify specified user, account, generic cred is authorized as job owner.
 *
 * @see MUICheckAuthorization() - parent
 *
 * @param RequestorU (I)
 * @param U          (I) [optional]user cred of object
 * @param A          (I) [optional]account cred of object
 * @param C          (I) [optional] class/queue cred of object
 * @param O          (I) [optional] other cred of object
 * @param PAL        (I) [optional] partitions this object can access
 * @param OType      (I) [optional]
 * @param TData      (I) [optional]
 */

mbool_t MUserIsOwner(

  mgcred_t      *RequestorU,
  mgcred_t      *U,
  mgcred_t      *A,
  mclass_t      *C,
  void          *O,
  mbitmap_t     *PAL,
  enum MXMLOTypeEnum OType,
  mfst_t        *TData)

  {
  int mindex;

  if (RequestorU == NULL) 
    {
    return(FAILURE);
    }

  if (RequestorU == U)
    {
    return(TRUE);
    }

  if (A != NULL)
    {
    if (MSched.FSTreeIsRequired && (PAL != NULL))
      {
      int tmppar;

      for (tmppar = 1; tmppar < MMAX_PAR; tmppar++)
        {
        mbool_t Found = FALSE;

        if (!bmisset(PAL,tmppar))
          continue;

        if (MPar[tmppar].Name[0] == '\0')
          break;

        if (MFSIsAcctManager(RequestorU,MPar[tmppar].FSC.ShareTree,U,A,&Found) == TRUE)
          return(TRUE);
        } /* END for tmppar */
      }
    else
      {
      for (mindex = 0;mindex < MMAX_CREDMANAGER;mindex++)
        {
        if (A->F.ManagerU[mindex] == RequestorU)
          {
          return(TRUE);
          }
        }
      }
    }    /* END if (A != NULL) */

  if (C != NULL)
    {
    for (mindex = 0;mindex < MMAX_CREDMANAGER;mindex++)
      {
      if (C->F.ManagerU[mindex] == RequestorU)
        {
        return(TRUE);
        }
      }
    }    /* END if (C != NULL) */

  if (TData != NULL)
    {
    mindex = 0;

    while ((TData->F->ManagerU[mindex] != NULL) && (mindex < MMAX_CREDMANAGER))
      {
      if (!strcmp(TData->F->ManagerU[mindex]->Name,RequestorU->Name))
        return(SUCCESS);

      mindex++;
      }
    }   /* END TData != NULL */

  if (O == NULL)
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoJob:

      if (bmisset(&((mjob_t *)O)->IFlags,mjifIsTemplate))
        {
        return(TRUE);
        }

      break;

    case mxoAcct:

      for (mindex = 0;mindex < MMAX_CREDMANAGER;mindex++)
        {
        if (((mgcred_t *)O)->F.ManagerU[mindex] == RequestorU)
          {
          return(TRUE);
          }
        }

      break;

    case mxoClass:

      for (mindex = 0;mindex < MMAX_CREDMANAGER;mindex++)
        {
        if (((mclass_t *)O)->F.ManagerU[mindex] == RequestorU)
          {
          return(TRUE);
          }
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (OType) */

  return(FALSE);
  }  /* END MUserIsOwner() */





/**
 * Verify specified credential can access ACL.
 *
 * @param GCred (I) [optional]
 * @param Auth  (I) [optional]
 * @param AType (I)
 * @param ACL   (I) 
 */

int MCredCheckAccess(

  mgcred_t       *GCred,
  char           *Auth,
  enum MAttrEnum  AType,
  macl_t         *ACL)

  {
  int oindex;

  macl_t *A;

  if (((GCred == NULL) && (Auth == NULL)) || 
      (AType == maNONE) ||
      (ACL == NULL))
    {
    return(FAILURE);
    }

  if (MACLIsEmpty(ACL))
    {
    return(SUCCESS);
    }

  if ((GCred == NULL) && (AType == maUser) && (MUserAdd(Auth,&GCred) == FAILURE))
    {
    return(FAILURE);
    }

  if ((GCred == NULL) && (AType == maAcct) && (MAcctAdd(Auth,&GCred) == FAILURE))
    {
    return(FAILURE);
    }

  for (A = ACL;A != NULL;A = A->Next)
    {
    if ((A->Type == maQOS) && (A->OPtr != NULL))
      {
      /* check user QAL */

      oindex = ((mqos_t *)A->OPtr)->Index;
 
      if ((GCred != NULL) &&
          (bmisset(&GCred->F.QAL,oindex)))
        {
        return(SUCCESS);
        }
      }

    if ((A->Type == maAcct) && 
        (GCred != NULL) &&
        (GCred->F.AAL != NULL))
      {
      /* check user AAL */

      if (MULLCheck(GCred->F.AAL,A->Name,NULL) == SUCCESS)
        {
        return(SUCCESS);
        }
      }

    if ((A->Type == maGroup) && 
        (GCred != NULL) &&
        (GCred->F.GAL != NULL))
      {
      /* check user GAL */

      if (MULLCheck(GCred->F.GAL,A->Name,NULL) == SUCCESS)
        {
        return(SUCCESS);
        }
      }

    if ((A->Type == AType) &&
        (GCred != NULL) &&
        (!strcmp(A->Name,GCred->Name)))
      {
      return(SUCCESS);
      }
    }  /* END for (aindex) */

  return(FAILURE);
  }  /* END MCredCheckAccess() */


/**
 * Determines whether user can perform actions on objects of this type.
 *
 * @param U (I) user performing the action
 * @param OType (I) object type
 * @param Action (I) action being performed
 */

mbool_t MUserHasPrivilege(

  mgcred_t              *U,
  enum MXMLOTypeEnum     OType,
  enum MObjectActionEnum Action)

  {
  mln_t *tmpL = NULL;

  mprivilege_t *Perm = NULL;

  printf("U: %p  OType: %d  Action:  %d\n",U,OType,Action);

  if ((U == NULL) || (OType == mxoNONE) || (Action == moaNONE))
    {
      printf("FALSE 1\n");
    return(FALSE);
    }

  if (U->Privileges == NULL)
    {
      printf("FALSE 2\n");
    return(FALSE);
    }

  if (MULLCheck(U->Privileges,MXO[OType],&tmpL) == FAILURE)
    {
      printf("FALSE 3\n");
    return(FALSE);
    }

  Perm = (mprivilege_t *)tmpL->Ptr;

  if ((Perm == NULL) || (!bmisset(&Perm->Perms,Action)))
    {
      printf("FALSE 4  tmpL: %p  Perm(tmpL->Ptr): %p  Action: %d BM:'%s'\n", tmpL,Perm, Action,Perm->Perms.ToString().c_str());
    return(FALSE);
    }
  
  return(TRUE);
  }  /* END MUserHasPrivilege() */



/**
 * Get UID for user object.
 *
 * @param U (I)
 * @param UID (O)
 * @param GID (O)
 */

int MUserGetUID(

  mgcred_t *U,    /* I */
  int      *UID,  /* O */
  int      *GID)  /* O */

  {
  if (U == NULL)
    {
    return(FAILURE);
    }

  if (UID != NULL)
    {
    if (U->OID == -1)
      { 
      return(FAILURE);
      }

    *UID = U->OID;
    }

  if (GID != NULL)
    {
    if ((U->F.GDef == NULL) || (((mgcred_t *)U->F.GDef)->OID == -1))
      {
      if ((UID == NULL) || 
          ((*GID = MUGIDFromUser(*UID,NULL,NULL)) == -1))
        {
        return(FAILURE);
        }
      }
    else
      {
      *GID = ((mgcred_t *)U->F.GDef)->OID;
      }
    }

  return(SUCCESS);
  }  /* END MUserGetUID() */

/* END MUser.c */
