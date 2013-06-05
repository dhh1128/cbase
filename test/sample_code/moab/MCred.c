/* HEADER */

/**
 * @file MCred.c
 *
 * Moab Credentials
 */

/* Contains:                                         *
 *                                                   */

/* NOTE:
 *  MCredToString() - located in MUser.c
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"


/**
 * set Credential defaults
 *
 */


int MCredSetDefaults()

  {
  mgcred_t *G;

  mmb_t DefaultMsg;

  char tmpLine[MMAX_LINE];

  const char *FName = "MCredSetDefaults";

  MDB(4,fSTRUCT) MLog("%s()\n",
    FName);

  MUHTCreate(&MUserHT,-1);
  MUHTCreate(&MGroupHT,-1);
  MUHTCreate(&MAcctHT,-1);
  memset(&DefaultMsg,0,sizeof(DefaultMsg));

  strcpy(tmpLine,"template:credential for template use only");

  DefaultMsg.Data = tmpLine;
  DefaultMsg.Priority = -1;        /* do not checkpoint message */
  DefaultMsg.Type = mmbtAnnotation;
  DefaultMsg.ExpireTime = MSched.Time + MCONST_EFFINF;

  /* initialize default credentials */
 
  MQOSInitialize(&MQOS[0],"DEFAULT");
  MQOSInitialize(&MQOS[1],ALL);

  MQOS[0].Role = mcalTemplate;
  MQOS[1].Role = mcalTemplate;

  /* NOTE:  user automatically creates IP object */
 
  if (MUserAdd("DEFAULT",&MSched.DefaultU) == SUCCESS)
    MSched.DefaultU->Role = mcalTemplate;

  MOSetMessage((void *)MSched.DefaultU,mxoUser,(void **)&DefaultMsg,mdfOther,mSet);
 
  if (MGroupAdd("NOGROUP",&G) == FAILURE)
    {
    MDB(1,fRM) MLog("ERROR:    cannot add default group\n");
    }
  else
    {
    G->Role = mcalTemplate;
    }
  
  if (MGroupAdd("DEFAULT",&MSched.DefaultG) == SUCCESS)
    {
    MSched.DefaultG->Role = mcalTemplate;

    MPUCreate(&MSched.DefaultG->L.IdlePolicy);

    MOSetMessage((void *)MSched.DefaultG,mxoGroup,(void **)&DefaultMsg,mdfOther,mSet);
    }

  if (MAcctAdd("DEFAULT",&MSched.DefaultA) == SUCCESS)
    {
    MSched.DefaultA->Role = mcalTemplate;

    MPUCreate(&MSched.DefaultA->L.IdlePolicy);

    MOSetMessage((void *)MSched.DefaultA,mxoAcct,(void **)&DefaultMsg,mdfOther,mSet);
    }

  /* DEFAULT QOS already exists (from MSysInitialize) */

  if (MOGetObject(mxoQOS,"DEFAULT",(void **)&MSched.DefaultQ,mVerify) == SUCCESS)
    {
    MPUCreate(&MSched.DefaultQ->L.IdlePolicy);

    MOSetMessage((void *)MSched.DefaultQ,mxoQOS,(void **)&DefaultMsg,mdfOther,mSet);
    }
 
   if (MClassAdd("DEFAULT",&MSched.DefaultC) == SUCCESS)
    {
    MSched.DefaultC->Role = mcalTemplate;

    MPUCreate(&MSched.DefaultC->L.IdlePolicy);

    MOSetMessage((void *)MSched.DefaultC,mxoClass,(void **)&DefaultMsg,mdfOther,mSet);

    bmunset(&MSched.DefaultC->Flags,mcfRemote);
    }

  /* create ALL credentials */

  MUserAdd(ALL,NULL);
  MGroupAdd(ALL,NULL);
  MAcctAdd(ALL,NULL);

  return(SUCCESS);
  }  /* END MCredSetDefaults() */


/**
 * Report credential attribute (with per) as a string.
 *
 * @param O (I)
 * @param OIndex (I)
 * @param AIndex (I)
 * @param String (O)
 * @param DFormat (I)
 * @param DFlags (I) [bitmap of enum mcm*]
 */

int MCredPerLimitsToMString(

  void                *O,
  enum MXMLOTypeEnum   OIndex,
  enum MCredAttrEnum   AIndex,
  mstring_t           *String,
  enum MFormatModeEnum DFormat,
  mulong               DFlags)

  {
  static mfs_t    *F;
  static mcredl_t *L;
  static must_t   *S;

  static void *CO = NULL;

  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  if (O == NULL)
    {
    return(FAILURE);
    }

  if (O != CO)
    {
    if ((MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE) ||
        (MOGetComponent(O,OIndex,(void **)&L,mxoxLimits) == FAILURE) ||
        (MOGetComponent(O,OIndex,(void **)&S,mxoxStats) == FAILURE))
      {
      /* cannot get components */
 
      return(FAILURE);
      }

    CO = O;
    }

  switch (AIndex)
    {
    case mcaMaxProcPerUser:

      {
      mpu_t *P;

      if (L->UserActivePolicy != NULL)
        {
        mhashiter_t HTIter;

        MUHTIterInit(&HTIter);

        while (MUHTIterate(L->UserActivePolicy,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
          {
          if (P->SLimit[mptMaxProc][0] < 0)
            continue;

          /* Get the user and format data for output */

          MStringAppendF(String,"%sMAXPROC[USER:%s]=%d,%d",
            (MUStrIsEmpty(String->c_str())) ? "" : " ",
            (P->Ptr != NULL) ? ((mgcred_t *)P->Ptr)->Name : "-",
            P->SLimit[mptMaxProc][0],
            P->HLimit[mptMaxProc][0]);
          }
        }   /* END if (L->APU != NULL) */
      }     /* END case mcaMaxProcPerUser */

      break;

    case mcaMaxNodePerUser:

      {
      mpu_t *P;

      if (L->UserActivePolicy != NULL)
        {
        mhashiter_t HTIter;

        MUHTIterInit(&HTIter);

        while (MUHTIterate(L->UserActivePolicy,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
          {
          if (P->SLimit[mptMaxNode][0] < 0)
            continue;

          MStringAppendF(String,"%sMAXNODE[USER:%s]=%d,%d",
            (MUStrIsEmpty(String->c_str())) ? "" : " ",
            (P->Ptr != NULL) ? ((mgcred_t *)P->Ptr)->Name : "-",
            P->SLimit[mptMaxNode][0],
            P->HLimit[mptMaxNode][0]);
          }
        }   /* END if (L->APU != NULL) */
      }     /* END case mcaMaxNodePerUser */

      break;

    case mcaMaxJobPerUser:

      {
      mpu_t *P;

      if (L->UserActivePolicy != NULL)
        {
        mhashiter_t HTIter;

        MUHTIterInit(&HTIter);

        while (MUHTIterate(L->UserActivePolicy,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
          {
          if (P->SLimit[mptMaxJob][0] < 0)
            continue;

          MStringAppendF(String,"%sMAXJOB[USER:%s]=%d,%d",
            (MUStrIsEmpty(String->c_str())) ? "" : " ",
            (P->Ptr != NULL) ? ((mgcred_t *)P->Ptr)->Name : "-",
            P->SLimit[mptMaxJob][0],
            P->HLimit[mptMaxJob][0]);
          }
        }   /* END if (L->APU != NULL) */
      }     /* END case mcaMaxJobPerUser */

      break;

    case mcaMaxJobPerGroup:

      {
      mpu_t *P = NULL;

      if (L->GroupActivePolicy != NULL)
        {
        mhashiter_t HTIter;

        MUHTIterInit(&HTIter);

        while (MUHTIterate(L->GroupActivePolicy,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
          {
          MStringAppendF(String,"%sMAXJOB[GROUP:%s]=%d,%d",
            (MUStrIsEmpty(String->c_str())) ? "" : " ",
            (P->Ptr != NULL) ? ((mgcred_t *)P->Ptr)->Name : "-",
            P->SLimit[mptMaxJob][0],
            P->HLimit[mptMaxJob][0]);
          }
        }    /* END if (L->APG != NULL) */
      }      /* END case mcaMaxJobPerGroup */

      break;

    default:

      return (FAILURE);

      break;

    } /* END switch (AIndex) */

  return (SUCCESS);

  } /* END MCredPerLimitsToString() */



/**
 * Report cred statistics.
 *
 * @param OIndex       (I)
 * @param OID          (I) [optional]
 * @param AList        (I) [optional]
 * @param ShowInterval (I)
 * @param DFormat      (I)
 * @param Buf          (O)
 * @param BufSize      (I)
 * @param ClientOutput (I)
 */

int MCredShowStats(

  enum MXMLOTypeEnum   OIndex,
  char                *OID,
  enum MStatAttrEnum  *AList,
  mbool_t              ShowInterval,
  enum MFormatModeEnum DFormat,
  void               **Buf,
  int                  BufSize,
  mbool_t              ClientOutput)

  {
  mxml_t *DE = NULL;
  mxml_t *SE;
  mxml_t *E;

  void   *O;
  void   *OP;
  
  void   *OE;

  int     iindex;

  must_t *SPtr = NULL;

  mhashiter_t HTI;

  char   *NameP;

  char    tmpLine[MMAX_LINE];

  const char *FName = "MCredShowStats";

  MDB(2,fUI) MLog("%s(%s,%s,%s,%s,%s,Buf,%d)\n",
    FName,
    MXO[OIndex],
    (OID != NULL) ? OID : "NULL",
    (AList != NULL) ? "AList" : "NULL",
    (ShowInterval == TRUE) ? "TRUE" : "FALSE",
    MFormatMode[DFormat],
    BufSize);

  if ((OIndex == mxoNONE) || (Buf == NULL))
    {
    return(FAILURE);
    }

  if ((OID != NULL) && strcmp(OID,NONE))
    {
    /* specific OID requested */

    MDB(4,fUI) MLog("INFO:     searching for %s '%s'\n",
      MXO[OIndex],
      OID);

    if (MOGetObject(OIndex,OID,&O,mVerify) == FAILURE)
      {
      MDB(3,fUI) MLog("INFO:     cannot locate %s %s in Moab's historical data\n",
        MXO[OIndex],
        OID);

      switch (DFormat)
        {
        case mfmXML:

          /* NO-OP */

          break;

        default:

          sprintf((char *)Buf,"cannot locate %s '%s' in Moab's historical data\n",
            MXO[OIndex],
            OID);

          break;
        }  /* END switch (DFormat) */


      return(FAILURE);
      }
    }   /* END if (OID != NULL) */

  /* initialize, display global information */

  switch (DFormat)
    {
    case mfmXML:

      {
      const enum MStatAttrEnum AList[] = {
        mstaTJobCount,
        mstaSchedCount,
        mstaIStatCount,
        mstaNONE };

      DE = *(mxml_t **)Buf;

      if (*(mxml_t **)Buf == NULL)
        {
        MXMLCreateE((mxml_t **)Buf,"Data");
        }

      DE = *(mxml_t **)Buf;

      E = NULL;

      MStatToXML((void *)&MPar[0].S,msoCred,&E,ClientOutput,(enum MStatAttrEnum *)AList);

      MXMLAddE(DE,E);
      }  /* END BLOCK */

      break;

    default:

      Buf[0] = '\0';

      /* NYI */

      break;
    }  /* END switch (DFormat) */

  MOINITLOOP(&OP,OIndex,&OE,&HTI);

  while ((O = MOGetNextObject(&OP,OIndex,OE,&HTI,&NameP)) != NULL)
    {
    if ((OID != NULL) && strcmp(OID,NONE))
      {
      if (strcmp(OID,NameP))
        continue;
      }

    if (MOGetComponent(O,OIndex,(void **)&SPtr,mxoxStats) == FAILURE)
      {
      continue;
      }

    E = NULL;

    MXMLCreateE(&E,(char *)MXO[OIndex]);

    MXMLSetAttr(E,(char *)MCredAttr[mcaID],(void *)NameP,mdfString);

    if (ShowInterval == TRUE)
      {
      must_t *ISPtr;

      for (iindex = 0;iindex < SPtr->IStatCount;iindex++)
        {
        ISPtr = (must_t *)SPtr->IStat[iindex];

        if (ISPtr == NULL)
          continue;

        SE = NULL;

        MStatToXML((void *)ISPtr,msoCred,&SE,ClientOutput,AList);

        MXMLAddE(E,SE);
        }  /* END for (iindex) */
      }
    else
      {
      SE = NULL;

      MStatToXML((void *)SPtr,msoCred,&SE,ClientOutput,AList);

      MXMLAddE(E,SE);
      }

    switch (DFormat)
      {
      case mfmXML:

        MXMLAddE(DE,E);

        break;

      default:

        MXMLToString(E,tmpLine,MMAX_BUFFER,NULL,TRUE);

        MXMLDestroyE(&E);

        strcat((char *)Buf,tmpLine);

        break;
      }  /* END switch (DFormat) */
    }    /* END while (MOGetNextObject() != NULL) */

  return(SUCCESS);
  }  /* END MCredShowStats() */




/**
 * Return TRUE if specified cred pointer matches generic cred object.
 *
 * @param C (I)
 * @param O (I)
 * @param OType (I)
 */

mbool_t MCredIsMatch(

  const mcred_t      *C,
  void               *O,
  enum MXMLOTypeEnum  OType)

  { 
  if ((C == NULL) || (O == NULL))
    {
    return(FALSE);
    }

  switch (OType)
    {
    case mxoUser:

      if (C->U == (mgcred_t *)O)
        {
        return(TRUE);
        }

      break;

    case mxoGroup:

      if (C->G == (mgcred_t *)O)
        {
        return(TRUE);
        }

      break;

    case mxoAcct:

      if (C->A == (mgcred_t *)O)
        {
        return(TRUE);
        }

      break;

    case mxoClass:

      if (C->C == (mclass_t *)O)
        {
        return(TRUE);
        }

      break;

    case mxoQOS:

      if (C->Q == (mqos_t *)O)
        {
        return(TRUE);
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (OType) */

  return(FALSE);
  }  /* END MCredIsMatch() */




/**
 * Free allocated memory associated with specified credential and destroy.
 *
 * @param CP (I) [modified,freed]
 */

int MCredDestroy(

  mgcred_t **CP)  /* I (modified,freed) */
 
  {
  mgcred_t *C;

  mfs_t    *F;

  int pindex;

  const char *FName = "MCredDestroy";

  MDB(3,fUI) MLog("%s(CP)\n",
    FName);

  if (CP == NULL)
    {
    return(SUCCESS);
    }

  C = *CP;

  if (C->Stat.IStat != NULL)
    MStatPDestroy((void ***)&C->Stat.IStat,msoCred,-1);

  if (C->L.ActivePolicy.GLimit != NULL)
    {
    MUHTFree(C->L.ActivePolicy.GLimit,TRUE,MUFREE);
    }

  if ((C->L.IdlePolicy != NULL) &&
      (C->L.IdlePolicy->GLimit != NULL))
    {
    MUHTFree(C->L.IdlePolicy->GLimit,TRUE,MUFREE);
    }

  MUFree((char **)&C->L.IdlePolicy);
  MUFree((char **)&C->L.RsvPolicy);
  MUFree((char **)&C->L.RsvActivePolicy);

  MUFree((char **)&C->Stat.XLoad);

  for (pindex = 0;pindex < (MMAX_PAR + 1);pindex++) 
    {
    if (C->L.JobMaximums[pindex] != NULL)
      {
      MJobDestroy(&C->L.JobMaximums[pindex]);
      }

    if (C->L.JobMinimums[pindex] != NULL)
      {
      MJobDestroy(&C->L.JobMinimums[pindex]);
      }
    }

  MJobDestroy(&C->L.JobDefaults);

  MUFree(&C->Pref);

  MUFree(&C->EMailAddress);

  MULLFree(&C->Variables,MUFREE);
  MULLFree(&C->Privileges,MUFREE);

  if (C->MB != NULL)
    {
    MMBFree(&C->MB,TRUE);
    }

  F = &C->F;

  bmclear(&F->QAL);
  bmclear(&F->Flags);

  if (F->AAL != NULL)
    MULLFree(&F->AAL,NULL);

  if (F->GAL != NULL)
    MULLFree(&F->GAL,NULL);

  if (F->CAL != NULL)
    MULLFree(&F->CAL,NULL);

  if (F->ReqRsv != NULL)
    MUFree(&F->ReqRsv);

  if (C->HomeDir != NULL)
    MUFree(&C->HomeDir);

  if (C->ProxyList != NULL)
    MUFree(&C->ProxyList);

  MUFree(&C->VDef);

  /* NOTE:  clear pointer but do not free structure */

  memset(*CP,0,sizeof(mgcred_t));

  return(SUCCESS);
  }  /* END MCredDestroy() */




/**
 *
 *
 * @param UList (I)
 * @param GList
 * @param AList
 * @param QList
 * @param CList
 * @param JFList
 * @param Buf (O)
 * @param BufSize
 */

int MCredListToAString(

  char *UList,   /* I */
  char *GList,
  char *AList,
  char *QList,
  char *CList,
  char *JFList,
  char *Buf,     /* O */
  int   BufSize)

  {
  char *BPtr;
  int   BSpace;

  char *ptr;
  char *TokPtr;

  int   index;
 
  char *OList[16];

  enum MAttrEnum OIndex[] = { maUser, maGroup, maAcct, maQOS, maClass, maJAttr, maNONE };

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  /* initialize data */

  OList[0] = UList;
  OList[1] = GList;
  OList[2] = AList;
  OList[3] = QList;
  OList[4] = CList;
  OList[5] = JFList;
  OList[6] = NULL;

  BPtr = Buf;
  BSpace = BufSize;

  BPtr[0] = '\0';

  for (index = 0;OIndex[index] != maNONE;index++)
    {
    if ((OList[index] == NULL) || 
        (OList[index][0] == '\0') || 
        !strcmp(OList[index],NONE))
      {
      continue;
      }

    ptr = MUStrTok(OList[index],":|, \t\n",&TokPtr);

    while (ptr != NULL)
      {
      if (BPtr != Buf)
        {
        /* add field separator */

        MUSNCat(&BPtr,&BSpace,",");
        }

      MUSNPrintF(&BPtr,&BSpace,"%s==%s",
        MAttrO[OIndex[index]],
        ptr);

      ptr = MUStrTok(NULL,":|, \t\n",&TokPtr);
      }  /* END while (ptr != NULL) */
    }  /* END for (index) */
 
  return(SUCCESS);
  }  /* END MCredListToAString() */





/**
 * Verify that user is a member of specified group. 
 *
 * @param U (I)
 * @param G (I)
 */

int MCredCheckGroupAccess(

  mgcred_t *U,  /* I */
  mgcred_t *G)  /* I */

  {
  if ((U == NULL) || (G == NULL))
    {
    return(FAILURE);
    }

  if (MUNameGetGroups(U->Name,G->OID,NULL,NULL,NULL,0) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCredCheckGroupAccess() */



/**
 * Verify user (and group) have access to specified account.
 *
 * @param U (I) [optional]
 * @param G (I) [optional]
 * @param A (I)
 */

int MCredCheckAcctAccess(

  mgcred_t *U,  /* I (optional) */
  mgcred_t *G,  /* I (optional) */
  mgcred_t *A)  /* I */

  {
  mln_t *L;

  if (A == NULL)
    {
    return(SUCCESS);
    }

  if ((MSched.EnforceAccountAccess == FALSE) || 
     ((MSched.DefaultU != NULL) &&
      (MULLCheck(MSched.DefaultU->F.AAL,"ALL",NULL) == SUCCESS)))
    {
    /* enforcement disabled, record detected account-to-user mapping */

    /*
    if (U != NULL)
      {
      MULLAdd(&U->F.AAL,A->Name,(void *)A,NULL,NULL);
      } 
    */

    return(SUCCESS);
    }

  if (U != NULL)
    {
    L = U->F.AAL;

    if ((L != NULL) && (MULLCheck(L,A->Name,NULL) == SUCCESS))
      {
      return(SUCCESS);
      }

    if ((MAM[0].State == mrmsActive) && 
        (MSched.Time - U->AMUpdateTime > 900))
      {
      /* refresh account access info from accounting manager */

      if (MAMUserUpdateAcctAccessList(&MAM[0],U,NULL) == SUCCESS)
        {
        U->AMUpdateTime = MSched.Time;

        if ((L != NULL) && (MULLCheck(L,A->Name,NULL) == SUCCESS))
          {
          return(SUCCESS);
          }
        }
      }
    }      /* END if (U != NULL) */

  if (MSched.DefaultU != NULL)
    {
    L = MSched.DefaultU->F.AAL;

    if ((L != NULL) && (MULLCheck(L,A->Name,NULL) == SUCCESS))
      {
      return(SUCCESS);
      }
    }

  if (G != NULL)
    {
    L = G->F.AAL;

    if ((L != NULL) && (MULLCheck(L,A->Name,NULL) == SUCCESS))
      {
      return(SUCCESS);
      }
    }

  if (MSched.DefaultG != NULL)
    {
    L = MSched.DefaultG->F.AAL;

    if ((L != NULL) && (MULLCheck(L,A->Name,NULL) == SUCCESS))
      {
      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MCredCheckAcctAccess() */




/**
 *
 *
 * @param InputString (I) [modified]
 * @param OType (O)
 * @param OP (O)
 */

int MCredParseName(

  char                *InputString, /* I (modified) */
  enum MXMLOTypeEnum  *OType,       /* O */
  void               **OP)          /* O */

  {
  char *ptr;
  char *TokPtr;

#ifndef MOPT

  if ((InputString == NULL) || (OType == NULL) || (OP == NULL))
    {
    return(FAILURE);
    }

#endif /* MOPT */

  /* FORMAT:  <CTYPE>:<CNAME> */

  ptr = MUStrTok(InputString,":",&TokPtr);

  *OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

  if (*OType == mxoNONE)
    {
    return(FAILURE);
    }

  ptr = MUStrTok(NULL,":",&TokPtr);

  if (MOGetObject(*OType,ptr,OP,mAdd) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCredParseName() */





/**
 * Create new credential object
 *
 * @param OType (I)
 * @param OID (I)
 * @param UName (I) [optional]
 * @param Allocation (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCredCreate(

  enum MXMLOTypeEnum  OType,      /* I */
  char               *OID,        /* I */
  char               *UName,      /* I (optional) */
  double              Allocation, /* I (optional) */
  char               *EMsg)       /* O (optional,minsize=MMAX_LINE) */

  {
  mbool_t ObjectExists = FALSE;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((OType == mxoNONE) || (OID == NULL))
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoAcct:
     
      {
      mgcred_t *A;

      if (MAcctFind(OID,&A) == FAILURE)
        {
        if (MAcctAdd(OID,&A) == FAILURE)
          {
          if (EMsg != NULL)
            {
            MUStrCpy(EMsg,"Unable to create account",sizeof(EMsg));
            }

          return(FAILURE);
          }
        }
      else
        {
        ObjectExists = TRUE;
        }

      if ((UName != NULL) && (UName[0] != '\0'))
        {
        mgcred_t *U;

        if (MUserAdd(UName,&U) == FAILURE)
          {
          return(FAILURE);
          }

        U->F.ADef = A;
        MULLAdd(&(U->F.AAL),A->Name,(void *)A,NULL,NULL);
        }    /* END if ((UName != NULL) && (UName[0] != '\0')) */

      if (MAM[0].Type != mamtNONE)
        {
        char AName[MMAX_LINE + 1];  /* project name */

        if (ObjectExists == FALSE)
          {
          if (MAMAccountCreate(
                &MAM[0],
                A->Name,
                UName,
                Allocation,
                NULL,   /* associated RM */
                "admin request",
                AName,  /* O */
                NULL,
                EMsg) == FAILURE)
            {
            return(FAILURE);
            }
          }
        else
          {
          /* query AM for account AName */

          if (MAMAccountQuery(
                &MAM[0],
                A->Name,
                NULL,
                NULL,   /* associated RM */
                AName,  /* O */
                NULL,
                EMsg) == FAILURE)
            {
            return(FAILURE);
            }
          }
         
        /* Bug: should not be called if user is not defined */
        if (MAMAccountAddUser(
              &MAM[0],
              A->Name,
              UName,       /* I */
              AName,       /* I */
              Allocation,
              NULL, /* associated RM */
              "admin request",
              NULL,
              EMsg) == FAILURE)
          {
          return(FAILURE);
          }
        }      /* END if (MAM[0].Type != mamtNONE) */
      }        /* END BLOCK (case mxoAcct) */

      break;

    case mxoGroup:

      {
      mgcred_t *G;

      if (MGroupAdd(OID,&G) == FAILURE)
        {
        return(FAILURE);
        }
      }    /* END BLOCK (case mxoGroup) */

      break;

    case mxoUser:

      {
      mgcred_t *U;

      if (MUserAdd(OID,&U) == FAILURE)
        {
        return(FAILURE);
        }
      }    /* END BLOCK (case mxoUser) */

      break;

    default:

      return(FAILURE);
  
      /*NOTREACHED*/

      break;
    }  /* END switch (OType) */

  return(SUCCESS);
  }  /* END MCredCreate() */



/**
 *
 *
 * @param L (I)
 * @param LType (I) [mlActive or mlIdle]
 * @param PType (I)
 * @param CAttr (I)
 * @param OType (I) [optional]
 * @param ShowUsage (I)
 * @param OBuf (O)
 * @param OBufSize (I)
 */

int MCredShow2DLimit(

  mcredl_t             *L,         /* I */
  enum MLimitTypeEnum   LType,     /* I (mlActive or mlIdle) */
  enum MActivePolicyTypeEnum  PType, /* I */
  enum MCredAttrEnum    CAttr,     /* I */
  enum MXMLOTypeEnum    OType,     /* I (optional) */
  mbool_t               ShowUsage, /* I */
  char                 *OBuf,      /* O */
  int                   OBufSize)  /* I */

  {
  enum MXMLOTypeEnum OList[] = { 
    mxoUser, mxoGroup, mxoAcct, mxoClass, mxoQOS, mxoNONE };

  int  oindex;

  mhashiter_t HTIter;

  mhash_t *HT;

  mpu_t  *P;
  mpu_t  *DefaultP;

  char *NameP;

  char *BPtr = NULL;
  int   BSpace = 0;

  if (OBuf == NULL)
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,OBuf,OBufSize);
 
  if (L == NULL)
    {
    return(FAILURE);
    }

  for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
    {
    if ((OType != mxoNONE) && (OList[oindex] != OType))
      continue;
    
    HT = NULL;

    switch (OList[oindex])
      {
      case mxoUser:

        if (LType == mlActive)
          HT = L->UserActivePolicy;
        else
          HT = L->UserIdlePolicy;

        break;

      case mxoGroup:

        if (LType == mlActive)
          HT = L->GroupActivePolicy;
        else
          HT = L->GroupIdlePolicy;

        break;

      case mxoAcct:

        if (LType == mlActive)
          HT = L->AcctActivePolicy;
        else
          HT = L->AcctIdlePolicy;

        break;

      case mxoQOS:

        if (LType == mlActive)
          HT = L->QOSActivePolicy;
        else
          HT = L->QOSIdlePolicy;

        break;

      case mxoClass:

        if (LType == mlActive)
          HT = L->ClassActivePolicy;
        else
          HT = L->ClassIdlePolicy;

        break;

      default:

        /* NOT-SUPPORTED */

        break;
      }  /* END switch (OList[oindex]) */

    if (HT == NULL)
      {
      continue;
      }

    DefaultP = NULL;

    /* must detect if policy is global */

    if (MUHTGet(HT,"DEFAULT",(void **)&DefaultP,NULL) == SUCCESS)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s[%s]=%d,%d\n",
        (OBuf[0] != '\0') ? "  " : "",
        MCredAttr[CAttr],
        MXOC[OList[oindex]],
        DefaultP->SLimit[PType][0],
        DefaultP->HLimit[PType][0]);
      }

    MUHTIterInit(&HTIter);

    while (MUHTIterate(HT,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
      {
      if ((DefaultP != NULL) && (P == DefaultP))
        {
        /* skip printing out the default, we did that above */

        continue;
        }

      if ((DefaultP != NULL) && 
          (P->SLimit[PType][0] == DefaultP->SLimit[PType][0]) &&
          (P->HLimit[PType][0] == DefaultP->HLimit[PType][0]))
        {
        /* this fellows values are the same as default's so skip printing them out */

        continue;
        }

      NameP = MOGetName(P->Ptr,OList[oindex],&NameP);

      MUSNPrintF(&BPtr,&BSpace,"%s%s[%s:%s]=%d,%d\n",
        (OBuf[0] != '\0') ? "  " : "",
        MCredAttr[CAttr],
        MXOC[OList[oindex]],
        (NameP != NULL) ? NameP : "",
        P->SLimit[PType][0],
        P->HLimit[PType][0]);
      }  /* END while (MUHTIterate()) */
    }    /* END for (oindex) */

  return(SUCCESS);
  }  /* END MCredShow2DLimit() */




/**
 *
 *
 * @param E (I)
 * @param OIndex (I)
 */

int MCredDoDiscovery(

  mxml_t             *E,       /* I */
  enum MXMLOTypeEnum  OIndex)  /* I */

  {
  int oindex;
  int CTok;
  
  mxml_t *CE;

  const enum MXMLOTypeEnum OList[] = {
    mxoUser,
    mxoGroup,
    mxoQOS, 
    mxoAcct,
    mxoClass,
    mxoNONE};

  for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
    {
    CTok = -1;

    CE = NULL;

    if ((OIndex != mxoNONE) && (OList[oindex] != OIndex))
      continue;

    while (MXMLGetChild(E,(char *)MXO[OList[oindex]],&CTok,&CE))
      {
      if (CE->ACount <= 0)
        continue;

      switch (OList[oindex])
        {
        case mxoUser:

          if (MUserFind(CE->AVal[0],NULL) == FAILURE)
            {
            /* create user */

            MUserAdd(CE->AVal[0],NULL);
            }

          break;

        case mxoAcct:

          if (MAcctFind(CE->AVal[0],NULL) == FAILURE)
            {
            /* create user */

            MAcctAdd(CE->AVal[0],NULL);
            }

          break;

        case mxoQOS:

          if (MQOSFind(CE->AVal[0],NULL) == FAILURE)
            {
            /* create user */

            MQOSAdd(CE->AVal[0],NULL);
            }

          break;

        case mxoGroup:

          if (MGroupFind(CE->AVal[0],NULL) == FAILURE)
            {
            /* create user */

            MGroupAdd(CE->AVal[0],NULL);
            }

          break;

        case mxoClass:

          if (MClassFind(CE->AVal[0],NULL) == FAILURE)
            {
            /* create user */

            MClassAdd(CE->AVal[0],NULL);
            }

          break;

        default:

          /* NYI */

          break;
        }  /* END switch (OIndex) */

      continue;
      }   /* END while (MXMLGetChild(DDE,(char *)MXO[OIndex],&CTok,&CE)) */
    }     /* END for (oindex = 0;OList[oindex] != mxoNONE;oindex++) */

  return(SUCCESS);
  }  /* END MCredDoDiscovery() */


/**
 * Checks to see if the job's group is a special variable name that
 * will be mapped to a real group name at a later stage in the job's
 * lifetime.
 *
 * @param J (I) The job to check.
 */
mbool_t MCredGroupIsVariable(

  const mjob_t *J)

  {
  if (J == NULL)
    {
    return(FALSE);
    }

  if ((J->Credential.G == NULL) ||
      (strcasecmp(J->Credential.G->Name,"%default")))
    {
    return(FALSE);
    }

  return(TRUE);
  }  /* MCredGroupIsVariable() */




/**
 * Locate credential manager.
 *
 * @param UName (I)
 * @param MList (I)
 * @param Manager (O) [optional]
 */

int MCredFindManager(

  char      *UName,     /* I */
  mgcred_t **MList,     /* I */
  mgcred_t **Manager)   /* O (optional) */

  {
  int mindex;

  if (Manager != NULL)
    *Manager = NULL;
  
  if ((UName == NULL) ||
      (MList == NULL))
    {
    return(FAILURE);
    }

  mindex = 0;

  for (mindex = 0;mindex < MMAX_CREDMANAGER;mindex++)
    {
    if ((MList[mindex] == NULL) ||
        (MList[mindex]->Name[0] == '\0'))
      {
      continue;
      }

    if (!strcmp(MList[mindex]->Name,UName))
      {
      if (Manager != NULL)
        *Manager = MList[mindex];

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MCredFindManager() */
  
  


/**
 *
 *
 * @param OType (I)
 * @param O (I)
 * @param EMsg (I) [minsize=MMAX_LINE]
 */

int MCredPurge(
 
  enum MXMLOTypeEnum  OType,  /* I */
  void               *O,      /* I */
  char               *EMsg)   /* I (minsize=MMAX_LINE) */

  {
  char  tmpLine[MMAX_LINE];

  /* purges references to credential within moab.cfg and .moab.dat */
  /* NOTE: does not delete credential from memory (NYI) */

  tmpLine[0] = '\0';

  switch (OType)
    {
    case mxoAcct:
    case mxoGroup:
    case mxoUser:

      snprintf(tmpLine,sizeof(tmpLine),"%s[%s]",
        MCredCfgParm[OType],
        ((mgcred_t *)O)->Name);      

      ((mgcred_t *)O)->IsDeleted = TRUE;

      break;

    case mxoClass:

      snprintf(tmpLine,sizeof(tmpLine),"%s[%s]",
        MCredCfgParm[OType],
        ((mclass_t *)O)->Name);      

      ((mclass_t *)O)->IsDeleted = TRUE;

      break;

    case mxoQOS:

      snprintf(tmpLine,sizeof(tmpLine),"%s[%s]",
        MCredCfgParm[OType],
        ((mqos_t *)O)->Name);      

      ((mqos_t *)O)->IsDeleted = TRUE;

      break;

    default:

      /* NO-OP */

      break;
    } 

  if (tmpLine[0] == '\0')
    {
    return(FAILURE);
    }

  if (MUIConfigPurge(tmpLine,EMsg,MMAX_LINE) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCredPurge() */




/**
 * Gets the requested U,G,A,C,Q object pointer either from the cred and the scheduler default
 *
 * @param C             (I)
 * @param OIndex        (I) 
 * @param CTOPtr        (O)
 * @param DefaultCTOPtr (O)
 */

void MCredGetCredTypeObject(

  const mcred_t *C, 
  enum MXMLOTypeEnum OIndex,
  void **CTOPtr,
  void **DefaultCTOPtr)

  {
  if (CTOPtr != NULL)
    *CTOPtr = NULL;

  if (DefaultCTOPtr != NULL)
    *DefaultCTOPtr = NULL;

  if ((C == NULL) || (CTOPtr == NULL) || (DefaultCTOPtr == NULL))
    return;

  switch (OIndex)
    {
    case mxoUser:
      *CTOPtr = C->U;
      *DefaultCTOPtr = MSched.DefaultU;
      break;

    case mxoGroup:
      *CTOPtr = C->G;
      *DefaultCTOPtr = MSched.DefaultG;
      break;

    case mxoAcct:
      *CTOPtr = C->A;
      *DefaultCTOPtr = MSched.DefaultA;
      break;

    case mxoClass:
      *CTOPtr = C->C;
      *DefaultCTOPtr = MSched.DefaultC;
      break;

    case mxoQOS:
      *CTOPtr = C->Q;
      *DefaultCTOPtr = MSched.DefaultQ;
      break;

    default:
      break;
    }

  return;
  } /* END MCredGetCredTypeObject */


/**
 * Check to see if the total number of jobs for each FairShare
 * credential equals or exceeds the max allowed. 
 *
 * @param J    (I)
 * @param EMsg (I) [minsize=MMAX_LINE]
 */

int MCredCheckFSTotals(
 
  const mjob_t *J,
  char         *EMsg)

  {
  mfst_t *TData = NULL;
  int pindex;

  const char *FName = "MCredCheckFSTotals";

  MDB(3,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* Note that the J->FSTree start at the leaf nodes of the fairshare tree which are the
   * most specific (e.g. user) and as it points to the parent data it is more general.
   * (e.g. class and then account) */

  for (pindex = 0; pindex < MMAX_PAR; pindex++)
    {
    /* only check limit for requested partitions */

    if ((MSched.PerPartitionScheduling == TRUE) &&
        (pindex > 0) &&
        (bmisset(&J->SpecPAL,pindex) == FAILURE))
      continue;

    TData = NULL;
    if (J->FairShareTree != NULL)
      {
      if (J->FairShareTree[pindex] != NULL)
        {
        TData = J->FairShareTree[pindex]->TData;
        }
      }
    if (TData == NULL)
      continue;

    /* Traverse the FairShare tree and if there is a maximum
       submitted jobs, compare it to the total job count. */
    
    while (TData != NULL)
      {
      int TotalJobs;
      int MaxSubmitJobs;
      const char *LName;

      TotalJobs = TData->L->TotalJobs[0];
      MaxSubmitJobs = TData->L->MaxSubmitJobs[0];

      if ((MaxSubmitJobs != 0) &&
          (TotalJobs >= MaxSubmitJobs))
        {
        LName = MOGetName(TData->O,TData->OType,NULL);

        if (LName == NULL)
          {
          LName = "<unknown>";
          }
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"total jobs %d max allowed %d for %s %s in partition %s",
            TotalJobs,
            MaxSubmitJobs,
            MCredCfgParm[TData->OType],
            LName,
            MPar[pindex].Name);
          }

        MDB(7,fSTAT) MLog("INFO:     job '%s' total jobs %d max allowed %d for %s %s in partition %s\n",
          J->Name,
          TotalJobs,
          MaxSubmitJobs,
          MCredCfgParm[TData->OType],
          LName,
          MPar[pindex].Name);

        return(FAILURE);
        } /* END if (MaxSubmitJobs ... */
  
      /* move to the next higher leaf in the tree */
  
      if (TData->Parent != NULL)
        {
        TData = TData->Parent->TData;
        }
      else
        break;
      }
    }
  return(SUCCESS);
  }  /* END MCredCheckFSTotals() */




/**
 * Check to see if the total number of jobs for each credential equals or exceeds the max allowed.
 *
 * @param J    (I)
 * @param EMsg (I) [minsize=MMAX_LINE]
 */

int MCredCheckTotals(
 
  const mjob_t *J,
  char         *EMsg)

  {
  int OTIndex;          /* Object Type Index for OTList */ 

  /* Object Type list */

  const enum MXMLOTypeEnum OTList[] = {
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoNONE };

  const char *FName = "MCredCheckTotals";

  MDB(3,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  for (OTIndex = 0; OTList[OTIndex] != mxoNONE; OTIndex++)
    {
    void *CTO;            /* Cred Type Object e.g. U,A,G,C,Q */
    void *DefaultCTO;     /* Cred Type Object e.g. MSched.DefaultU, MSched.DefaultA, etc */
    mcredl_t *L  = NULL;
    int pindex;

    /* Get the pointer to the U,A,G,C,Q mgcredt structure, then get a pointer to the limits structure
     * from that and then check to see if a limit has been set. */

    MCredGetCredTypeObject(&J->Credential,OTList[OTIndex],&CTO,&DefaultCTO);

    MOGetComponent(CTO,OTList[OTIndex],(void **)&L,mxoxLimits);

    if (L == NULL)
      continue;

    for (pindex = 0; pindex < MMAX_PAR; pindex++)
      {
      int TotalJobs;
      int MaxSubmitJobs;
      char *LName;

      TotalJobs = L->TotalJobs[pindex];
      MaxSubmitJobs = L->MaxSubmitJobs[pindex];

      /* only check limit for requested partitions */

      if ((MSched.PerPartitionScheduling == TRUE) &&
          (pindex > 0) &&
          (bmisset(&J->SpecPAL,pindex) == FAILURE))
        continue;

      LName = MOGetName(CTO,OTList[OTIndex],NULL);

      if (MaxSubmitJobs == 0)
        {
        mcredl_t *DL = NULL;

        /* check to see if the default cred has a limit */

        MOGetComponent(DefaultCTO,OTList[OTIndex],(void **)&DL,mxoxLimits);
        if (DL != NULL)
          {
          MaxSubmitJobs = DL->MaxSubmitJobs[pindex]; 
          LName = MOGetName(DefaultCTO,OTList[OTIndex],NULL);
          }
        }

      /* See if the total current usage has reached or exceeded the max allowed */

      if ((MaxSubmitJobs != 0) &&
          (TotalJobs >= MaxSubmitJobs))
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"total jobs %d max allowed %d for %s %s in partition %s",
            TotalJobs,
            MaxSubmitJobs,
            MCredCfgParm[OTList[OTIndex]],
            LName,
            MPar[pindex].Name);
          }
  
        MDB(7,fSTAT) MLog("INFO:     job '%s' total jobs %d max allowed %d for %s %s in partition %s\n",
          J->Name,
          TotalJobs,
          MaxSubmitJobs,
          MCredCfgParm[OTList[OTIndex]],
          LName,
          MPar[pindex].Name);
  
        return(FAILURE);
        } /* END if (MaxSubmitJobs[pindex] ... */

      /* see if we only need to check pindex 0 */

      if (MSched.PerPartitionScheduling == FALSE)
        break;
      } /* END for pindex */
    } /* END for (OTIndex = 0; OTList[OTIndex] != mxoNONE; OTIndex++) */

  return(MCredCheckFSTotals(J,EMsg));
  }  /* END MCredCheckTotals() */


/**
 * Determines if a user has manually set an attribute at runtime, as opposed
 * to the user setting it via a config file parameter, or the attribute
 * being set by another process such as an identity or resource manager
 *
 * @param Object      (I) 
 * @param ObjectType  (I) 
 * @param Attribute   (I)
 */

mbool_t MCredAttrSetByUser(

  void               *Object,     /* I */
  enum MXMLOTypeEnum  ObjectType, /* I */
  enum MCredAttrEnum  Attribute)  /* I */

  {
  switch (ObjectType)
    {
    case mxoUser:
    case mxoGroup:
    case mxoAcct:

      return bmisset(&((mgcred_t *)Object)->UserSetAttrs,Attribute);

      break;

    case mxoClass:
    case mxoQOS:
    default:

      /* NYI */

      break;
    }  /* END switch (ObjectType) */

  return(FALSE);
  } /* END MCredAttrSetByUser() */

/**
 * Free's all the memory associated with a specific credential 
 * hash table.
 *
 * @param CredHT      (I) --- Credential Hash table, like 
 *                    MUserHT, MGroupHT, MAcctHT
 */

int MCredFreeTable(mhash_t *CredHT)

  {
  mgcred_t *O;
  mhashiter_t HIter;

  MUHTIterInit(&HIter);
  while (MUHTIterate(CredHT,NULL,(void **)&O,NULL,&HIter) == SUCCESS)
    { 
    /* Free the contents of the credentail */

    MCredDestroy(&O);

    /* Free the actual credential memory block */

    MUFree((char**)&O);
    }  /* END for MUHTIterate */

  MUHTClear(CredHT,FALSE,NULL);

  return(SUCCESS);
  }  /* END MCredFreeTable() */


/* END MCred.c */
