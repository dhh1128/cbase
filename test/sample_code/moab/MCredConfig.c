/* HEADER */

      
/**
 * @file MCredConfig.c
 *
 * Contains: Credential Configuration functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Parses a credential attribute's soft and hard limits.
 *
 * WARNING: SLimit and HLimit should be initialized BEFORE this routine is called.
 *          This routine does not set default values (it can't, it doesn't know what
 *          the default should be 0 or -1, depends on the limit).
 *
 * NOTE:  If ':' detected, assume value is specified in format [[[DD:]HH:]MM:]SS 
 *
 * @param ValLine (I)
 * @param SLimit (I)
 * @param HLimit (I)
 */

int __MCredParseLimit(

  char  *ValLine,  /* I */
  long  *SLimit,   /* I */
  long  *HLimit)   /* I */

  {
  char *tail;

  if (MUStrIsEmpty(ValLine))
    {
    return(SUCCESS);
    }

  /* FORMAT:  <SOFT>[,<HARD>] */

  if (strchr(ValLine,':') != NULL)
    {
    char *ptr;
    char *TokPtr = NULL;

    ptr = MUStrTok(ValLine,",",&TokPtr);

    if (!strncasecmp(ptr,"unl",3))
      *SLimit = -1;
    else
      *SLimit = MUTimeFromString(ptr);

    if (TokPtr[0] != '\0')
      {
      if (!strncasecmp(TokPtr,"unl",3))
        *HLimit = -1;
      else
        *HLimit = MUTimeFromString(TokPtr);
      }
    else
      {
      *HLimit = *SLimit;
      }
    }
  else
    {
    if (!strncasecmp(ValLine,"unl",3))
      {
      *SLimit = -1;
      tail = ValLine + strlen("unl");
      }
    else
      *SLimit = strtol(ValLine,&tail,10);
 
    if (*tail == ',')
      {
      if (!strncasecmp(tail + 1,"unl",3))
        *HLimit = -1;
      else
        *HLimit = strtol(tail + 1,NULL,10);
      }
    else
      {
      *HLimit = *SLimit;
      }
    }

  return(SUCCESS);
  }  /* END __MCredParseLimit() */



/**
 * Load credential config attributes.
 *
 * @see MCredProcessConfig() - child - process individual cred config lines
 *
 * @param OType (I)
 * @param CName (I)
 * @param P (I) optional
 * @param ABuf (I) config source data [optional]
 * @param IsPar (I) optional
 * @param EMsg (O) [optiona,minsize=MMAX_LINE)
 */

int MCredLoadConfig(

  enum MXMLOTypeEnum  OType,
  char               *CName,
  mpar_t             *P,
  char               *ABuf,
  mbool_t             IsPar,
  char               *EMsg)

  {
  char   IndexName[MMAX_NAME];

  char   Value[MMAX_BUFFER];

  char   tEMsg[MMAX_BUFFER];

  char  *ptr;
  char  *head;

  void  *C;

  int    rc = SUCCESS;

  mbool_t LoopedOnce = FALSE;

  mfs_t    *CFSPtr = NULL;
  mcredl_t *CLPtr  = NULL;
 
  /* FORMAT:  <KEY>=<VAL>[<WS><KEY>=<VAL>]...         */
  /*          <VAL> -> <ATTR>=<VAL>[:<ATTR>=<VAL>]... */

  /* load all/specified cred config info */

  head = (ABuf != NULL) ? ABuf : MSched.ConfigBuffer;

  if (head == NULL)
    {
    return(FAILURE);
    }

  if ((MCredCfgParm[OType] == NULL) || (strcasestr(head,MCredCfgParm[OType]) == NULL))
    {
    /* nothing to find */

    return(FAILURE);
    }

  if ((CName == NULL) || (CName[0] == '\0'))
    {
    /* load ALL cred config info */

    ptr = head;
 
    IndexName[0] = '\0';

    if (EMsg != NULL)
      EMsg[0] = '\0';
 
    while (MCfgGetSVal(
             head,
             &ptr,
             MCredCfgParm[OType],
             IndexName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) != FAILURE)
      {
      if (MOGetObject(OType,IndexName,&C,mAdd) == FAILURE) 
        {
        /* unable to locate credential */

        IndexName[0] = '\0';     
 
        continue; 
        }

      if (OType == mxoClass)
        {
        /* set local attribute since class defined in local config file */

        bmunset(&((mclass_t *)C)->Flags,mcfRemote);
        }

      if ((MOGetComponent(C,OType,(void **)&CFSPtr,mxoxFS) == FAILURE) ||
          (MOGetComponent(C,OType,(void **)&CLPtr,mxoxLimits) == FAILURE))
        {
        /* cannot get components */

        IndexName[0] = '\0';
 
        continue; 
        }

      tEMsg[0] = '\0';

      rc &= MCredProcessConfig(C,OType,P,Value,CLPtr,CFSPtr,IsPar,tEMsg);

      if ((EMsg != NULL) && 
          (tEMsg[0] != '\0'))
        {
        sprintf(EMsg,"%s%s%s",
            EMsg,
            (LoopedOnce == TRUE) ? "\n" : "",
            tEMsg);

        LoopedOnce = TRUE;
        }

      IndexName[0] = '\0';
      }  /* END while (MCfgGetSVal() != FAILURE) */ 
    }    /* END if ((CName == NULL) || (CName[0] == '\0')) */
  else
    {
    /* load specified cred config info */

    if (MCfgGetSVal(
          head,
          NULL,
          MCredCfgParm[OType],         
          CName,
          NULL,
          Value,
          sizeof(Value),
          0,
          NULL) == FAILURE)
      {
      /* cannot locate config info for specified cred */ 

      return(FAILURE);
      }
    
    if (MOGetObject(OType,CName,&C,mAdd) == FAILURE)
      {
      /* unable to add user */

      return(FAILURE);
      }

    if ((MOGetComponent(C,OType,(void **)&CFSPtr,mxoxFS) == FAILURE) ||
        (MOGetComponent(C,OType,(void **)&CLPtr,mxoxLimits) == FAILURE))
      {
      /* cannot get components */

      IndexName[0] = '\0';

      return(FAILURE);
      }

    if (MCredProcessConfig(C,OType,P,Value,CLPtr,CFSPtr,IsPar,EMsg) == FAILURE)
      {
      return(FAILURE);
      }
    }  /* END else ((CName == NULL) || (CName[0] == '\0')) */
   
  return(rc); 
  }  /* END MCredLoadConfig() */





/**
 * 
 *
 * @param OIndex (I)
 * @param C (I)
 * @param IsTemplate (I)
 */

int MCredAdjustConfig(

  enum MXMLOTypeEnum  OIndex,     /* I */
  void               *C,          /* I */
  mbool_t             IsTemplate) /* I */

  {
  mgcred_t *U;
  mqos_t   *Q;

  mbool_t DoProfile = FALSE;

  if (C == NULL)
    {
    return(FAILURE);
    }

  switch (OIndex)
    {
    case mxoUser:
    case mxoAcct:
    case mxoGroup:

      {
      mgcred_t *Default;

      mgcred_t  *Cred = (mgcred_t *)C;

      if (OIndex == mxoUser)
        {
        Default = MSched.DefaultU;
        }
      else if (OIndex == mxoAcct)
        {
        Default = MSched.DefaultA;
        }
      else 
        {
        Default = MSched.DefaultG;
        }

      Cred->DoHold = MBNOTSET;

      if ((Default != NULL) && !bmisclear(&Default->F.PAL))
        {
        bmcopy(&Cred->F.PAL,&Default->F.PAL);

        Cred->F.PALType = Default->F.PALType;
        }
      }

      break;

    case mxoClass:

      {
      mclass_t *Class = (mclass_t *)C;

      Class->DoHold = MBNOTSET;

      if ((MSched.DefaultC != NULL) && !bmisclear(&MSched.DefaultC->F.PAL))
        {
        bmcopy(&Class->F.PAL,&MSched.DefaultC->F.PAL);

        Class->F.PALType = MSched.DefaultC->F.PALType;
        }
      }

      break;

    case mxoQOS:

      {
      Q = (mqos_t *)C;

      Q->DoHold = MBNOTSET;

      if ((MSched.DefaultQ != NULL) && !bmisclear(&MSched.DefaultQ->F.PAL))
        {
        bmcopy(&Q->F.PAL,&MSched.DefaultQ->F.PAL);

        Q->F.PALType = MSched.DefaultQ->F.PALType;
        }
      }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (OIndex) */

  /* If template credential then no further processing needed. */

  if (IsTemplate == TRUE)
    {
    return(SUCCESS);
    }

  switch (OIndex)
    {
    case mxoUser:

      U = (mgcred_t *)C;

      if ((U->F.QDef[0] == NULL) || (U->F.QDef[0] == &MQOS[0]))
        {
        if ((MSched.DefaultU != NULL) && 
            (MSched.DefaultU->F.QDef[0] != NULL))
          {
          Q = (mqos_t *)MSched.DefaultU->F.QDef[0];

          bmcopy(&U->F.QAL,&MSched.DefaultU->F.QAL);

          if (Q != NULL)
            bmset(&U->F.QAL,Q->Index);     
          }
        }   /* END if ((U->F.QDef[0] == NULL) || (U->F.QDef[0] == &MQOS[0])) */

      if ((MSched.DefaultU != NULL) && (MSched.DefaultU->ProfilingIsEnabled == TRUE))
        DoProfile = TRUE;

      break;
   
    case mxoAcct:

      if ((MSched.DefaultA != NULL) && (MSched.DefaultA->ProfilingIsEnabled == TRUE))
        DoProfile = TRUE;

      break;

    case mxoGroup:

      if ((MSched.DefaultG != NULL) && (MSched.DefaultG->ProfilingIsEnabled == TRUE))
        DoProfile = TRUE;

      break;

    case mxoClass:

      if ((MSched.DefaultC != NULL) && (MSched.DefaultC->ProfilingIsEnabled == TRUE))
        DoProfile = TRUE;

      break;

    case mxoQOS:

      if ((MSched.DefaultQ != NULL) && (MSched.DefaultQ->ProfilingIsEnabled == TRUE))
        DoProfile = TRUE;

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (OIndex) */

  if (DoProfile == TRUE)
    {
    must_t *SPtr = NULL;

    if ((MOGetComponent(C,OIndex,(void **)&SPtr,mxoxStats) == SUCCESS) &&
        (SPtr->IStat == NULL))
      {
      /* NOTE:  handle 'default' profiling specification */

      MStatPInitialize((void *)SPtr,FALSE,msoCred);

      /* must allocate O->L.IP */

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:

          if (((mgcred_t *)C)->L.IdlePolicy == NULL)
            {
            MPUCreate(&((mgcred_t *)C)->L.IdlePolicy);
            }

          break;

        case mxoClass:

          if (((mqos_t *)C)->L.IdlePolicy == NULL)
            {
            MPUCreate(&((mclass_t *)C)->L.IdlePolicy);
            }

          break;

        case mxoQOS:

          if (((mqos_t *)C)->L.IdlePolicy == NULL)
            {
            MPUCreate(&((mqos_t *)C)->L.IdlePolicy);
            }

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */
      }    /* END if (MOGetComponent() == SUCCESS) */
    }      /* END if (DoProfile == TRUE) */

  return(SUCCESS);
  }  /* END MCredAdjustConfig() */




/**
 * Process general credential config parameters.
 *
 * @see MCredLoadConfig() - parent
 * @see MQOSProcessConfig() - peer
 *
 * @param O (I) [optional,modified]
 * @param OIndex (I)
 * @param P (I) optional
 * @param Value (I) [FORMAT:  <ATTR>=<VAL>]
 * @param L (I) partition limit struct [modified]
 * @param F (I) (optional in some cases)
 * @param IsPar (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCredProcessConfig(

  void          *O,
  enum MXMLOTypeEnum OIndex,
  mpar_t        *P,
  const char    *Value,
  mcredl_t      *L,
  mfs_t         *F,
  mbool_t        IsPar,
  char          *EMsg)

  {
  int ParIndex;

  mbitmap_t ValidBM;

  char *TokPtr = NULL;
  char *tail;

  int   aindex;

  enum MCredAttrEnum AIndex;

  char  ValBuf[MMAX_BUFFER];
  char  ArrayLine[MMAX_NAME];

  int   tmpI;

  enum  MCompEnum ValCmp;

  char *BPtr;
  int   BSpace;

  int   rc = SUCCESS;

  const char *FName = "MCredProcessConfig";

  MDB(4,fSTRUCT) MLog("%s(O,%s,%s,L,F,EMsg)\n",
    FName,
    MXO[OIndex],
    Value);

  /* fail if invalid parameters are specified or invalid attributes are requested */

  if (EMsg != NULL)
    MUSNInit(&BPtr,&BSpace,EMsg,MMAX_LINE);

  if ((Value == NULL) ||
      (Value[0] == '\0') ||
      (L == NULL)) 
    {
    return(FAILURE);
    }

  if (IsPar == TRUE)
    {
    if (P == NULL)
      {
      return(FAILURE);
      }

    MParSetupRestrictedCfgBMs(NULL,&ValidBM,NULL,NULL,NULL,NULL,NULL);

    ParIndex = P->Index;
    }
  else
    {
    ParIndex = 0;
    }

  /* process value line */

  char *mutableString=NULL;

  MUStrDup(&mutableString,Value);

  char *ptrVal = MUStrTokE(mutableString," \t\n",&TokPtr);

  while (ptrVal != NULL)
    {
    /* parse name-value pairs */

    /* FORMAT:  <ATTR>[INDEXNAME]=<VALUE>[,<VALUE>] */
    /*          <INDEXNAME> -> {[}<OTYPE>[:<OID>]{]} */

    if (MUGetPair(
          ptrVal,
          (const char **)MCredAttr,
          &aindex,
          ArrayLine,  /* O (attribute array index) */
          FALSE,      /* (use relative) */
          &tmpI,      /* O (comparison) */
          ValBuf,     /* O (value) */
          sizeof(ValBuf)) == FAILURE)
      {
      if (aindex == -1)
        {
        /* value pair not located in standard cred set */

        /* NOTE:  attempt cred-specific flags */

        switch (OIndex)
          {
          case mxoClass:

            rc &= MClassProcessConfig((mclass_t *)O,ptrVal,IsPar);

            break;

          case mxoGroup:

            rc &= MGroupProcessConfig((mgcred_t *)O,ptrVal,IsPar);

            break;

          case mxoQOS:

            rc &= MQOSProcessConfig((mqos_t *)O,ptrVal,IsPar);

            break;

          case mxoUser:

            rc &= MUserProcessConfig((mgcred_t *)O,ptrVal,IsPar);

            break;

          case mxoAcct:

            rc &= MAcctProcessConfig((mgcred_t *)O,ptrVal,IsPar);

            break;

          default:

            /* pass through with rc = FAILURE */

            rc = FAILURE;

            break;
          }  /* END switch (OIndex) */

        if (rc == FAILURE)
          {
          if (EMsg != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"unknown attribute '%s' specified\n",
              ptrVal);
            }
          }
        }    /* END if (aindex == -1) */
 
      ptrVal = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }  /* END if (MUGetPair() == FAILURE) */
  
    ValCmp = (enum MCompEnum)tmpI;

    AIndex = (enum MCredAttrEnum)aindex;
 
    if ((IsPar == TRUE) && (!bmisset(&ValidBM,aindex)))
      {
      ptrVal = MUStrTokE(NULL," \t\n",&TokPtr);

      /* This cred attr hasn't been added to the list of supported attrs in MParSetupRestrictedCfgBMs() */

      MDB(3,fFS) MLog("INFO:     '%s' not in bitmap of valid cred attributes for cred processing\n",
        MCredAttr[AIndex]);

      continue;
      }

    switch (AIndex)
      {
      case mcaADef:

        if (F == NULL)
          break;

        {
        if (strcmp(ValBuf,DEFAULT) && strcmp(ValBuf,"DEFAULT"))
          {
          if ((MAcctFind(ValBuf,(mgcred_t **)&F->ADef) == SUCCESS) ||
              (MAcctAdd(ValBuf,(mgcred_t **)&F->ADef) == SUCCESS))
            {
            /* add account to AList */

            MULLAdd(&F->AAL,ValBuf,(void *)F->ADef,NULL,NULL);  /* no free routine */
            }
          else
            {
            if (EMsg != NULL)
              MUSNPrintF(&BPtr,&BSpace,"invalid cred value '%s'\n",
                ValBuf);

            MDB(4,fFS) MLog("WARNING:  invalid cred value '%s'\n",
              ValBuf);

            rc = FAILURE;
            }
          }
        }

        break;

      case mcaAList:

        if (F == NULL)
          break;

        {
        char *ptr;
        char *TokPtr;

        mgcred_t *A;

        /* FORMAT:  <AID>[{,:}<AID>]... */

        ptr = MUStrTok(ValBuf,",: \n\t",&TokPtr);

        if ((ValCmp != mcmpINCR) && (ValCmp != mcmpDECR))
          {
          /* clear AAL */

          MULLFree(&F->AAL,NULL);
          }

        /* NOTE:  mcmpDECR not supported */

        while (ptr != NULL)
          {
          if (strcmp(ptr,DEFAULT) && strcmp(ptr,"DEFAULT"))
            {
            if ((MAcctFind(ptr,(mgcred_t **)&A) == SUCCESS) ||
                (MAcctAdd(ptr,(mgcred_t **)&A) == SUCCESS))
              {
              /* add account to AList */

              MULLAdd(&F->AAL,ptr,(void *)A,NULL,NULL);  /* no free routine */
              }
            }

          ptr = MUStrTok(NULL,",: \n\t",&TokPtr);
          }  /* END while (ptr != NULL) */

        if ((F->AAL != NULL) && (F->ADef == NULL))
          {
          F->ADef = (mgcred_t *)F->AAL->Ptr;
          }

        /* if this is not the first iteration then we set a flag
         * saying that we set the value at runtime */

        if (MSched.Iteration > 0)
          {
          switch(OIndex)
            {
            case mxoUser:
            case mxoGroup:
            case mxoAcct:
              {
              mgcred_t *Cred = (mgcred_t *)O;
              bmset(&Cred->UserSetAttrs,mcaAList);
              }

            case mxoClass:
            case mxoQOS:
              /*NYI*/

            default:
              break;
            }



          }
        }    /* END BLOCK (case mcaAList) */
 
        break;

      case mcaCDef:

        if (F != NULL)
          {
          if (strcmp(ValBuf,DEFAULT) && strcmp(ValBuf,"DEFAULT"))
            {
            if ((MClassFind(ValBuf,&F->CDef) == SUCCESS) ||
                (MClassAdd(ValBuf,&F->CDef) == SUCCESS))
              {
              /* add class to CList */
              /* not yet supported */
              /* NYI */
              }
            }
          }

        break;

      case mcaChargeRate:

        if ((O == NULL) || (ValBuf[0] == '\0'))
          break;

        switch (OIndex)
          {
          case mxoAcct:

            ((mgcred_t *)O)->ChargeRate = strtod(ValBuf,NULL);

            break;

          default:

            /* NO-OP */

            break;

          }  /* END switch (OIndex) */

        break;

      /* case mcaComment: (see mcaMessages) */

      case mcaEMailAddress:

        if (O == NULL)
          break;

        switch (OIndex)
          {
          case mxoUser:

            MUStrDup(&((mgcred_t *)O)->EMailAddress,ValBuf);

            break;

          default:

            /* NO-OP */

            break;
          }

        break;

      case mcaFlags:

        if (O == NULL)
          break;

        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:
          case mxoQOS:
          default:

            /* NOT SUPPORTED */

            break;

          case mxoClass:

            bmfromstring(ValBuf,MClassFlags,&((mclass_t *)O)->Flags);

            break;
          }  /* END switch (OIndex) */

        break;

      case mcaFSTarget:

        if (F != NULL)
          {
          if ((MPar[0].FSC.FSPolicy == mfspNONE) && (MPar[0].FSC.FSPolicySpecified == FALSE))
            MPar[0].FSC.FSPolicy = MDEF_FSPOLICY;

          MFSTargetFromString(F,ValBuf,TRUE);

          switch (OIndex)
            {
            case mxoUser:  MPar[0].FSC.PSCIsActive[mpsFU] = TRUE; break;
            case mxoGroup: MPar[0].FSC.PSCIsActive[mpsFG] = TRUE; break;
            case mxoAcct:  MPar[0].FSC.PSCIsActive[mpsFA] = TRUE; break;  
            case mxoQOS:   MPar[0].FSC.PSCIsActive[mpsFQ] = TRUE; break;
            case mxoClass: MPar[0].FSC.PSCIsActive[mpsFC] = TRUE; break;

              break;

            default:

              /* NO-OP */

              break;
            }  /* END switch (OIndex) */
          }    /* END if (F != NULL) */

        break;

      case mcaFSCap:

        if (F != NULL)
          MFSTargetFromString(F,ValBuf,FALSE);

        break;

      case mcaHold:

        if (O == NULL)
          break;

        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:

            /* NOTE:  hold attribute supported as config setting */

            ((mgcred_t *)O)->DoHold = MUBoolFromString(ValBuf,FALSE); 
  
            break;

          case mxoQOS:

            ((mqos_t *)O)->DoHold = MUBoolFromString(ValBuf,FALSE);

            break;

          case mxoClass:

            ((mclass_t *)O)->DoHold = MUBoolFromString(ValBuf,FALSE);

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */

        break;

      case mcaManagers:
      case mcaNoSubmit:

        if (O == NULL)
          break;

        switch (OIndex)
          {
          case mxoAcct:
          case mxoClass:
          case mxoGroup:
          case mxoQOS:
          case mxoUser:

            if (ValBuf != NULL)
              MCredSetAttr(O,OIndex,AIndex,NULL,(void **)ValBuf,NULL,mdfString,mSet);

            break;

          default:

            /* NO-OP */

            break;            
          }  /* END switch (OIndex) */

        break;

      case mcaMaxIJob:
      case mcaMaxIProc:
      case mcaMaxIPS:
      case mcaMaxGRes:
      case mcaMaxIGRes:
      case mcaMaxJob:
      case mcaMaxArrayJob:
      case mcaMaxIArrayJob:
      case mcaMaxNode:
      case mcaMaxPE:
      case mcaMaxProc:
      case mcaMaxWC:
      case mcaMaxPS:
      case mcaMaxMem:
      case mcaRMaxCount:
      case mcaRMaxProc:
      case mcaRMaxPS:
      case mcaRMaxDuration:
      case mcaRMaxTotalProc:
      case mcaRMaxTotalPS:
      case mcaRMaxTotalDuration:

        /* 6.1 change: parsing moved to MCredSetAttr() */

        MCredSetAttr(O,OIndex,AIndex,P,(void **)ValBuf,ArrayLine,mdfString,mSet);

        break;

      case mcaFSCWeight:

        if (O == NULL)
          break;

        {
        long tmpL;

        tmpL = strtol(ValBuf,NULL,10);

        switch (OIndex)
          {
          case mxoAcct:
          case mxoGroup:
          case mxoUser:

            ((mgcred_t *)O)->FSCWeight = tmpL;

            break;

          case mxoQOS:

            ((mqos_t *)O)->FSCWeight = tmpL;

            break;

          default:

            /* ignore, not supported */

            break;
          }  /* END switch (OIndex) */
        }    /* END BLOCK */

        break;

      case mcaJobFlags:

        if (F != NULL)
          {
          MJobFlagsFromString(NULL,NULL,&F->JobFlags,&F->ReqRsv," \t\n:,|",ValBuf,TRUE);
          }    /* END if (F != NULL) */

        break;

      case mcaMaxJobPerUser:

        switch (OIndex)
          {
          case mxoClass:
          case mxoQOS:
 
            {
            mpu_t *P = NULL;

            MLimitGetMPU(L,(void *)MSched.DefaultU,mxoUser,mlActive,&P);

            __MCredParseLimit(
              ValBuf,
              &P->SLimit[mptMaxJob][ParIndex],
              &P->HLimit[mptMaxJob][ParIndex]);

            break;
            }  /* END BLOCK */
        
          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */

        break;

      case mcaMaxJobPerGroup:

        switch (OIndex)
          {
          case mxoClass:
          case mxoQOS:
 
            {
            mpu_t *P = NULL;

            MLimitGetMPU(L,O,mxoGroup,mlActive,&P);

            __MCredParseLimit(
              ValBuf,
              &P->SLimit[mptMaxJob][ParIndex],
              &P->HLimit[mptMaxJob][ParIndex]);  
            }  /* END BLOCK */

            break;
        
          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */

        break;

      case mcaMaxNodePerUser:
 
        switch (OIndex)
          {
          case mxoClass:
          case mxoQOS:
 
            {
            mpu_t *P = NULL;

            MLimitGetMPU(L,(void *)MSched.DefaultU,mxoUser,mlActive,&P);

            __MCredParseLimit(
              ValBuf,
              &P->SLimit[mptMaxNode][ParIndex],
              &P->HLimit[mptMaxNode][ParIndex]);
 
            break;
            }  /* END BLOCK */
 
          default:

            /* NO-OP */
 
            break;
          }  /* END switch (Oindex) */
 
        break;

      case mcaMinProc:

        if ((L->JobPolicy == NULL) && (MPUCreate(&L->JobPolicy) == FAILURE))
          {
          break;
          }

        __MCredParseLimit(
          ValBuf,
          &L->JobPolicy->SLimit[mptMinProc][ParIndex],
          &L->JobPolicy->HLimit[mptMinProc][ParIndex]);
 
        break;

      case mcaMaxProcPerUser:
 
        switch (OIndex)
          {
          case mxoClass:
          case mxoQOS:
  
            {
            mpu_t *P = NULL;

            MLimitGetMPU(L,(void *)MSched.DefaultU,mxoUser,mlActive,&P);

            __MCredParseLimit(
              ValBuf,
              &P->SLimit[mptMaxProc][ParIndex],
              &P->HLimit[mptMaxProc][ParIndex]);
 
            break;
            }  /* END BLOCK */
 
          default:

            /* NO-OP */
 
            break;
          }  /* END switch (OIndex) */
 
        break;

      case mcaMaxProcPerNode:

        if ((O == NULL) || (ValBuf[0] == '\0'))
          break;

        switch (OIndex)
          {
          case mxoClass:

            ((mclass_t *)O)->MaxProcPerNode = strtol(ValBuf,NULL,10);

            break;

          default:

            /* NO-OP */

            break;

          }  /* END switch (OIndex) */

        break;

      case mcaMaxINode:

        if (L->IdlePolicy == NULL)
          {
          MPUCreate(&L->IdlePolicy);
          }

        if (L->IdlePolicy != NULL)       
          {
          __MCredParseLimit(
            ValBuf,
            &L->IdlePolicy->SLimit[mptMaxNode][ParIndex],
            &L->IdlePolicy->HLimit[mptMaxNode][ParIndex]);
          }
 
        break;
 
      case mcaMaxIPE:

        if (L->IdlePolicy == NULL)
          {
          MPUCreate(&L->IdlePolicy);
          }
 
        if (L->IdlePolicy != NULL)
          {
          __MCredParseLimit(
            ValBuf,
            &L->IdlePolicy->SLimit[mptMaxPE][ParIndex],
            &L->IdlePolicy->HLimit[mptMaxPE][ParIndex]);
          }

        break;
 
      case mcaMaxIWC:

        if (L->IdlePolicy == NULL)
          {
          MPUCreate(&L->IdlePolicy);
          }

        if (L->IdlePolicy != NULL)
          {
          __MCredParseLimit(
            ValBuf,
            &L->IdlePolicy->SLimit[mptMaxWC][ParIndex],
            &L->IdlePolicy->HLimit[mptMaxWC][ParIndex]);
          }

        break;
 
      case mcaMaxIMem:

        if (L->IdlePolicy == NULL)
          {
          MPUCreate(&L->IdlePolicy);
          }

        if (L->IdlePolicy != NULL)
          {
          __MCredParseLimit(
            ValBuf,
            &L->IdlePolicy->SLimit[mptMaxMem][ParIndex],
            &L->IdlePolicy->HLimit[mptMaxMem][ParIndex]);
          }

        break;

      case mcaOMaxJob:
 
        if (L->OverrideActivePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideActivePolicy->SLimit[mptMaxJob][ParIndex],
          &L->OverrideActivePolicy->HLimit[mptMaxJob][ParIndex]);

        if (ArrayLine[0] != '\0')
          {
          int oindex;

          /* add override specifier */

          if ((oindex = MUGetIndexCI(ArrayLine,MXOC,TRUE,mxoNONE)) != mxoNONE)
            {
            bmset(&L->OverrideActiveCredBM,oindex);
            }
          }
 
        break;
 
      case mcaOMaxNode:
 
        if (L->OverrideActivePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideActivePolicy->SLimit[mptMaxNode][ParIndex],
          &L->OverrideActivePolicy->HLimit[mptMaxNode][ParIndex]);
 
        break;
 
      case mcaOMaxPE:
 
        if (L->OverrideActivePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideActivePolicy->SLimit[mptMaxPE][ParIndex],
          &L->OverrideActivePolicy->HLimit[mptMaxPE][ParIndex]);
 
        break;
 
      case mcaOMaxProc:
 
        if (L->OverrideActivePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideActivePolicy->SLimit[mptMaxProc][ParIndex],
          &L->OverrideActivePolicy->HLimit[mptMaxProc][ParIndex]);
 
        break;
 
      case mcaOMaxPS:
 
        if (L->OverrideActivePolicy != NULL)
          {
          __MCredParseLimit(
            ValBuf,
            &L->OverrideActivePolicy->SLimit[mptMaxPS][ParIndex],
            &L->OverrideActivePolicy->HLimit[mptMaxPS][ParIndex]);
          }
 
        break;
 
      case mcaOMaxWC:
 
        if (L->OverrideActivePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideActivePolicy->SLimit[mptMaxWC][ParIndex],
          &L->OverrideActivePolicy->HLimit[mptMaxWC][ParIndex]);

        if (OIndex == mxoQOS)
          {
          if (L->OverriceJobPolicy == NULL)
            break;

          __MCredParseLimit(
            ValBuf,
            &L->OverriceJobPolicy->SLimit[mptMaxWC][ParIndex],
            &L->OverriceJobPolicy->HLimit[mptMaxWC][ParIndex]);
          }
 
        break;
 
      case mcaOMaxMem:
 
        if (L->OverrideActivePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideActivePolicy->SLimit[mptMaxMem][ParIndex],
          &L->OverrideActivePolicy->HLimit[mptMaxMem][ParIndex]);
 
        break;

      case mcaOMaxIJob:
 
        if (L->OverrideIdlePolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverrideIdlePolicy->SLimit[mptMaxJob][ParIndex],&L->OverrideIdlePolicy->HLimit[mptMaxJob][ParIndex]);
 
        break;
 
      case mcaOMaxINode:
 
        if (L->OverrideIdlePolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverrideIdlePolicy->SLimit[mptMaxNode][ParIndex],&L->OverrideIdlePolicy->HLimit[mptMaxNode][ParIndex]);
 
        break;
 
      case mcaOMaxIPE:
 
        if (L->OverrideIdlePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideIdlePolicy->SLimit[mptMaxPE][ParIndex],
          &L->OverrideIdlePolicy->HLimit[mptMaxPE][ParIndex]);
 
        break;
 
      case mcaOMaxIProc:
 
        if (L->OverrideIdlePolicy == NULL)
          break;

        __MCredParseLimit(
          ValBuf,
          &L->OverrideIdlePolicy->SLimit[mptMaxProc][ParIndex],
          &L->OverrideIdlePolicy->HLimit[mptMaxProc][ParIndex]);
 
        break;
 
      case mcaOMaxIPS:
 
        if (L->OverrideIdlePolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverrideIdlePolicy->SLimit[mptMaxPS][ParIndex],&L->OverrideIdlePolicy->HLimit[mptMaxPS][ParIndex]);
 
        break;
 
      case mcaOMaxIWC:
 
        if (L->OverrideIdlePolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverrideIdlePolicy->SLimit[mptMaxWC][ParIndex],&L->OverrideIdlePolicy->HLimit[mptMaxWC][ParIndex]);
 
        break;
 
      case mcaOMaxIMem:
 
        if (L->OverrideIdlePolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverrideIdlePolicy->SLimit[mptMaxMem][ParIndex],&L->OverrideIdlePolicy->HLimit[mptMaxMem][ParIndex]);
 
        break;

      case mcaOMaxJNode:
 
        if (L->OverriceJobPolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverriceJobPolicy->SLimit[mptMaxNode][ParIndex],&L->OverriceJobPolicy->HLimit[mptMaxNode][ParIndex]);
 
        break;
 
      case mcaOMaxJPE:
 
        if (L->OverriceJobPolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverriceJobPolicy->SLimit[mptMaxPE][ParIndex],&L->OverriceJobPolicy->HLimit[mptMaxPE][ParIndex]);
 
        break;
 
      case mcaOMaxJProc:
 
        if (L->OverriceJobPolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverriceJobPolicy->SLimit[mptMaxProc][ParIndex],&L->OverriceJobPolicy->HLimit[mptMaxProc][ParIndex]);
 
        break;
 
      case mcaOMaxJPS:
 
        if (L->OverriceJobPolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverriceJobPolicy->SLimit[mptMaxPS][ParIndex],&L->OverriceJobPolicy->HLimit[mptMaxPS][ParIndex]);
 
        break;
 
      case mcaOMaxJWC:
 
        if (L->OverriceJobPolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverriceJobPolicy->SLimit[mptMaxWC][ParIndex],&L->OverriceJobPolicy->HLimit[mptMaxWC][ParIndex]);
 
        break;
 
      case mcaOMaxJMem:
 
        if (L->OverriceJobPolicy != NULL)
          __MCredParseLimit(ValBuf,&L->OverriceJobPolicy->SLimit[mptMaxMem][ParIndex],&L->OverriceJobPolicy->HLimit[mptMaxMem][ParIndex]);
 
        break;

      case mcaOverrun:

        if (F == NULL)
          break;

        /* cred job overrun */

        F->Overrun = MUTimeFromString(ValBuf);

        break;

      case mcaPriority:

        if (F == NULL)
          break;

        F->Priority = strtol(ValBuf,NULL,10);

        switch (OIndex)
          {
          case mxoUser:  MPar[0].FSC.PSCIsActive[mpsCU] = TRUE; break;
          case mxoGroup: MPar[0].FSC.PSCIsActive[mpsCG] = TRUE; break;
          case mxoAcct:  MPar[0].FSC.PSCIsActive[mpsCA] = TRUE; break;
          case mxoQOS:   MPar[0].FSC.PSCIsActive[mpsCQ] = TRUE; break;
          case mxoClass: MPar[0].FSC.PSCIsActive[mpsCC] = TRUE; break;

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */

        bmset(&F->Flags,mffIsLocalPriority);

        MDB(4,fFS) MLog("INFO:     cred priority set to %ld\n",
          F->Priority);

        break;

      case mcaPDef:

        if (F == NULL)
          break;

        /* default partition specification */

        /* NOTE:  PALType only set with PList */

        /* remove partition modifier */

        if ((tail = strchr(ValBuf,'&')) != NULL)
          {
          /* NO-OP */
          }
        else if ((tail = strchr(ValBuf,'^')) != NULL)
          {
          /* NO-OP */
          }
 
        if (tail != NULL)
          *tail = '\0';
 
        if (MParFind(ValBuf,&F->PDef) == FAILURE)
          {
          if (MParAdd(ValBuf,&F->PDef) == FAILURE)
            {
            /* empty slot located */
 
            MDB(4,fFS) MLog("ERROR:    Error adding new partition %s\n",
              ValBuf);
            }
          else
            {
            F->PDef = &MPar[MDEF_SYSPDEF];
            }
          }
 
        bmset(&F->PAL,((mpar_t  *)F->PDef)->Index);

        F->PDefSet = TRUE;
 
        MDB(4,fFS) MLog("INFO:     default partition set to %s\n",
          ((mpar_t  *)F->PDef)->Name);

        break;

      case mcaPList:

        {
        char tmpLine[MMAX_LINE];

        if (F == NULL)
          break;

        /* partition access list */

        if (strchr(ValBuf,'&') != NULL)
          F->PALType = malAND;
        else if (strchr(ValBuf,'^') != NULL)
          F->PALType = malOnly;
        else
          F->PALType = malOR;
 
        if (strstr(ValBuf,MCONST_SHAREDPARNAME) == NULL)
          {
          /* SHARED should ALWAYS be in the PAL */

          MUStrAppendStatic(ValBuf,MCONST_SHAREDPARNAME,',',sizeof(ValBuf));
          }

        if (MPALFromString(ValBuf,mAdd,&F->PAL) == FAILURE)
          {
          MDB(5,fFS) MLog("ALERT:    cannot parse string '%s' as rangelist\n",
            ValBuf);
          }
 
        MDB(2,fFS) MLog("INFO:     partition access list set to %s\n",
          MPALToString(&F->PAL,",",tmpLine));
 
        if ((OIndex == mxoSys) && (ValBuf[0] != '\0') && (F->PDefSet == FALSE))
          {
          int pindex;

          /* clear current F.PDef */

          /* set F.PDef = first member in F->PAL */

          F->PDef = NULL;

          for (pindex = 1;pindex < MMAX_PAR;pindex++)
            {
            if (MUStrIsEmpty(MPar[pindex].Name))
              break;

            if (bmisset(&F->PAL,pindex))
              {
              F->PDef = &MPar[pindex];

              break;
              }
            }
          }
        else if ((F->PDef != NULL) && (ValBuf[0] != '\0'))
          {
          bmset(&F->PAL,F->PDef->Index);
          }
        } /* END block */
 
        break;

      case mcaQDef:

        if (F == NULL)
          break;

        {
        char *ptr;
        char *TokPtr;

        int   qindex;

        /* FORMAT:  <QOS>[&^][,<QOS>[&^]]... */

        ptr = MUStrTok(ValBuf,", \t\n",&TokPtr);

        qindex = 0;

        while (ptr != NULL)
          {
          if (!strcmp(ptr,DEFAULT) || !strcmp(ptr,"DEFAULT"))
            {
            /* do not allow assignment to DEFAULT QOS */

            ptr = MUStrTok(NULL,", \t\n",&TokPtr);

            continue;
            }

          if ((tail = strchr(ptr,'&')) != NULL)
            F->QALType = malAND;
          else if ((tail = strchr(ptr,'^')) != NULL)
            F->QALType = malOnly;
          else
            F->QALType = malOR;

          if (tail != NULL)
            *tail = '\0';

          if (MQOSFind(ptr,(mqos_t **)&F->QDef[qindex]) == FAILURE)
            {
            if (F->QDef[qindex] != NULL)
              {
              /* empty slot located */

              MQOSAdd(ptr,(mqos_t **)&F->QDef[qindex]);
              }
            else
              {
              F->QDef[qindex] = &MQOS[MDEF_SYSQDEF];
              }
            }

          bmset(&F->QAL,((mqos_t *)F->QDef[qindex])->Index);

          qindex++;

          ptr = MUStrTok(NULL,", \t\n",&TokPtr);
          }  /* END while (ptr != NULL) */

        bmset(&F->Flags,mffQDefSpecified);
        }    /* END BLOCK (case mcaQDef) */

        break;

      case mcaQList:

        if (F == NULL)
          break;

        {
        char QALLine[MMAX_LINE];

        mbitmap_t tmpQAL;  /* QOS access list */

        mqos_t *FQ;

        if (strchr(ValBuf,'&') != NULL)
          F->QALType = malAND;
        else if (strchr(ValBuf,'^') != NULL)
          F->QALType = malOnly;
        else
          F->QALType = malOR;

        /* support '=' and '+=' */

        /* FORMAT:  <QOS>[,<QOS>]... */

        if (MQOSListBMFromString(ValBuf,&tmpQAL,&FQ,mAdd) == FAILURE)
          {
          MDB(5,fFS) MLog("ALERT:    cannot parse string '%s' as rangelist\n",
            ValBuf);
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
          /* check to see if F->QAL has a value */

          if (!bmisclear(&F->QAL))
            {
            bmor(&F->QAL,&tmpQAL);
            }
          else
            {
            bmcopy(&F->QAL,&tmpQAL);
            }
          }

        MDB(2,fFS) MLog("INFO:     QOS access list set to %s\n",
          MQOSBMToString(&F->QAL,QALLine));

        if (F->QDef[0] != NULL)
          {
          /* if set, add QDef to QList */

          bmset(&F->QAL,(F->QDef[0])->Index);
          }
        else if ((FQ != NULL) && (MSched.QOSIsOptional == FALSE))
          {
          /* if not set, add first QList QOS as QDef */

          F->QDef[0] = FQ;
          }
        }  /* END BLOCK (case mcaQList) */

        break;

      case mcaReqRID:

        if (O == NULL)
          break;

        /* credential to rsv binding */

        MUStrDup(&F->ReqRsv,ValBuf);

        bmset(&F->JobFlags,mjfAdvRsv);

        break;

      case mcaMessages:
      case mcaComment:

        if (O == NULL)
          break;

        {
        mmb_t *MB;
        mmb_t *tMB;

        MCredSetAttr(
          O,
          OIndex,
          AIndex,
          NULL,
          (void **)ValBuf,
          NULL,
          mdfString,
          mSet);

        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:
          default:

            MB = ((mgcred_t *)O)->MB;

            break;

          case mxoQOS:

            MB = ((mqos_t *)O)->MB;

            break;

          case mxoClass:

            MB = ((mclass_t *)O)->MB;

            break;
          }  /* END switch (OIndex) */

        if (MB == NULL)
          break;

        /* do not checkpoint config messages */

        for (tMB = MB;tMB != NULL;tMB = (mmb_t *)tMB->Next)
          {
          tMB->Priority = -1;
          }  /* END for (tMB) */
        }    /* END BLOCK (case mcaMessages) */

        break;

      case mcaDefTPN:
      case mcaDefWCLimit:
      case mcaRole:
      case mcaTrigger:
      case mcaVariables:

        if (O == NULL)
          break;

        MCredSetAttr(
          O,
          OIndex,
          AIndex,
          NULL,
          (void **)ValBuf,
          NULL,
          mdfString,
          mSet);

        break;

      case mcaMinWCLimitPerJob:
      case mcaMaxWCLimitPerJob:
      case mcaMinNodePerJob:
      case mcaMaxNodePerJob:
      case mcaMaxProcPerJob:

        if (O == NULL)
          break;

        MCredSetAttr(
          O,
          OIndex,
          AIndex,
          (IsPar == TRUE) ? P : NULL,
          (void **)ValBuf,
          NULL,
          mdfString,
          mSet);

        /* Save the original value from the config file to act as a limit to what 
           can be reset by a change propogated from the RM.  MOAB-3420, MOAB-3560 */
        if ((bmisset(&MSched.Flags,mschedfBoundedRMLimits) == TRUE) &&
            (OIndex == mxoClass))
          {
          int pindex = (IsPar == TRUE) ? P->Index : 0;
          switch (AIndex)
            {
            case mcaMaxWCLimitPerJob:
              {
              mjob_t *J;
              mjob_t **JP;
        
              JP = &((mclass_t*)O)->RMLimit.JobMaximums[pindex];
              if ((*JP == NULL) &&
                  (MJobAllocBase(JP,NULL) == FAILURE))
                {
                return(FAILURE);
                }

              J = *JP;
        
              bmset(&J->IFlags,mjifIsTemplate);
              J->WCLimit = MUTimeFromString((char *)ValBuf);

              }  /* END BLOCK */
              break;

            case mcaMinNodePerJob:
            case mcaMaxNodePerJob:
              {
              mjob_t *J;
              mjob_t **JP;
        
              JP = (AIndex == mcaMinNodePerJob) ?&((mclass_t*)O)->RMLimit.JobMinimums[pindex] :
                                                 &((mclass_t*)O)->RMLimit.JobMaximums[pindex];
              if ((*JP == NULL) &&
                  (MJobAllocBase(JP,NULL) == FAILURE))
                {
                return(FAILURE);
                }

              J = *JP;
        
              if (ValBuf == NULL)
                J->Request.NC = 0;
              else
                J->Request.NC = strtol((char *)ValBuf,NULL,10);

              }  /* END BLOCK */
              break;

            default:
              break;
            }
          }

        break;

      case mcaMaxSubmitJobs:

        if (O == NULL)
          break;

        MCredSetAttr(
          O,
          OIndex,
          AIndex,
          (IsPar == TRUE) ? P : NULL,
          (void **)ValBuf,
          NULL,
          mdfString,
          mSet);

        break;

      case mcaMemberUList:

        /* FORMAT:  <USER>[{,:+}<USER>]... */

        if (O == NULL)
          break;

        /* NOTE:  should handle 'all' (NYI) */

        {
        /* list of users granted access to this credential */

        char *TokPtr;
        char *ptr;

        char *NameP;

        mgcred_t *U;

        if (MOGetName(O,OIndex,&NameP) == NULL)
          break;

        ptr = MUStrTok(ValBuf,",+:",&TokPtr);

        /* NOTE:  MemberUList is a pseudo-attribute and maintains
                  no persistent info about existing state.  Thus,
                  incr, and decr cannot be supported   
        */

        if ((ValCmp == mcmpINCR) || (ValCmp == mcmpDECR))
          {
          /* NO-OP */
          }
        else
          {
          /* clear old value */

          /* NOTE:  MemberUList is a pseudo-attribute and maintains
                    no persistent info about existing state.  Thus,
                    incr, and decr cannot be supported
          */

          /* NO-OP */
          }    /* END else */

        while (ptr != NULL)
          {
          if (MUserAdd(ptr,&U) == FAILURE)
            {
            ptr = MUStrTok(NULL,",+:",&TokPtr);

            /* cannot locate/add specified user */

            continue;
            }
 
          switch (OIndex)
            {
            case mxoGroup:

              if (ValCmp == mcmpDECR)
                {
                MULLRemove(&U->F.GAL,NameP,NULL);
                }
              else
                {
                MULLAdd(&U->F.GAL,NameP,NULL,NULL,NULL);  /* no free routine */

                if (U->F.GDef == NULL)
                  U->F.GDef = (mgcred_t *)O;
                }

              break;

            case mxoAcct:

              if (ValCmp == mcmpDECR)
                {
                MULLRemove(&U->F.AAL,NameP,NULL);
                }
              else
                {
                MULLAdd(&U->F.AAL,NameP,NULL,NULL,NULL);  /* no free routine */

                if (U->F.ADef == NULL)
                  U->F.ADef = (mgcred_t *)O;
                }

              break;

            case mxoQOS:

              if (ValCmp == mcmpDECR)
                {
                bmunset(&U->F.QAL,((mqos_t *)O)->Index);
                }
              else
                {
                bmset(&U->F.QAL,((mqos_t *)O)->Index);
                }

              break;

            case mxoClass:
            default:

              /* use required user list */

              /* not supported */

              /* NO-OP */

              break;
            }  /* END switch (OIndex) */

          ptr = MUStrTok(NULL,",+:",&TokPtr);
          }    /* END while (ptr != NULL) */
        }      /* END BLOCK (case mcaMemberUList) */

        break;

      case mcaPref:

        if (O == NULL)
          break;

        {
        mgcred_t *C;

        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:

            C = (mgcred_t *)O;

            MUStrDup(&C->Pref,(char *)ValBuf);
   
            break;
 
          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */
        }    /* END BLOCK (case mcaPref) */

        break;

      case mcaEnableProfiling:

        if (O == NULL)
          break;

        {
        must_t *SPtr;

        MSched.ProfilingIsEnabled = TRUE;

        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:

            ((mgcred_t *)O)->ProfilingIsEnabled = TRUE;

            break;

          case mxoQOS:

            ((mqos_t *)O)->ProfilingIsEnabled = TRUE;

            break;

          case mxoClass:

            ((mclass_t *)O)->ProfilingIsEnabled = TRUE;

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */

        if ((MOGetComponent(O,OIndex,(void **)&SPtr,mxoxStats) == FAILURE) ||
            (SPtr->IStat != NULL))
          {
          break;
          }

        /* NOTE:  must handle 'default' profiling specification */

        MStatPInitialize((void *)SPtr,FALSE,msoCred);
        }  /* END BLOCK (case mcaEnableProfiling) */

        break;

      case mcaRsvCreationCost:

        if (O == NULL)
          break;

        switch (OIndex)
          {
          case mxoAcct:

            ((mgcred_t *)O)->RsvCreationCost = strtod(ValBuf,NULL); 

            break;

          default:

            break;
          }

        break;

      default:

        if (EMsg != NULL)
          MUSNPrintF(&BPtr,&BSpace,"cred attribute '%s' not handled\n",
            MCredAttr[aindex]);

        MDB(4,fFS) MLog("WARNING:  cred attribute '%s' not handled\n",
          MCredAttr[aindex]);

        rc = FAILURE;

        break;
      }  /* END switch (AIndex) */

    ptrVal = MUStrTokE(NULL," \t\n",&TokPtr);       
    }  /* END while (ptrVal != NULL) */

  MUFree(&mutableString);

  return(rc);
  }  /* END MCredProcessConfig() */





/**
 *
 *
 * @param O (I)
 * @param OIndex (I)
 * @param AList (I)
 * @param VFlag (I)
 * @param PIndex (I)
 * @param String (O)
 */

int MCredConfigShow(

  void *O,
  enum MXMLOTypeEnum OIndex,
  enum MCredAttrEnum *AList,
  int   VFlag,
  int   PIndex,
  mstring_t *String)

  {
  char *OName = NULL;

  if ((O == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  *String = "";

  mstring_t tmpString(MMAX_LINE);

  /* show common attributes */

  MCredConfigAShow(O,OIndex,AList,VFlag,PIndex,&tmpString);

  /* show cred specific attributes */

  switch (OIndex)
    {
    case mxoQOS:

      MQOSConfigLShow((mqos_t *)O,VFlag,PIndex,&tmpString);

      break;

    case mxoClass:

      MClassConfigLShow((mclass_t *)O,VFlag,PIndex,&tmpString);

      break;

    default:

      /* no-op */

      break;
    }  /* END switch (OIndex) */

  if (!tmpString.empty())
    {
    /* display parameter */

    MOGetName(O,OIndex,&OName);

    /* append "%s[%s] %s\n" */

    *String += MCredCfgParm[OIndex];
    *String += '[';
    *String += OName;
    *String += "] ";
    *String += tmpString;
    }

  return(SUCCESS);
  }  /* END MCredConfigShow() */




/**
 * 
 *
 * NOTE: does NOT clear String before putting contents in
 *
 * @param O (I)
 * @param OIndex (I)
 * @param SAList (I) [optional]
 * @param VFlag (I)
 * @param PIndex (I)
 * @param String (O)
 */

int MCredConfigAShow(

  void *O,
  enum MXMLOTypeEnum  OIndex,
  enum MCredAttrEnum *SAList,
  int          VFlag,
  int          PIndex,
  mstring_t   *String)

  {
  enum MCredAttrEnum DAList[] = {
    mcaDefTPN,
    mcaDefWCLimit,
    mcaMaxNodePerJob,
    mcaMaxProcPerJob,
    mcaMaxProcPerUser,
    mcaMinWCLimitPerJob,
    mcaMaxWCLimitPerJob,
    mcaNONE };

  enum MCredAttrEnum *AList;

  int aindex;

  if ((O == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  AList = (SAList != NULL) ? SAList : DAList;

  /* don't clear buffer */

  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mcaNONE;aindex++)
    {
    tmpString = "";

    if ((MCredAToMString(O,OIndex,AList[aindex],&tmpString,mfmNONE,0) == FAILURE) ||
        (tmpString.empty()))
      {
      continue;
      }

    /* Append ' %s=%s' */
    *String += ' ';
    *String += MCredAttr[AList[aindex]];
    *String += '=';
    *String += tmpString;
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MCredConfigAShow() */





int MCredSyncDefaultConfig()

  {
  void   *O;
  void   *OP;

  void   *OE;

  int     oindex;

  enum MXMLOTypeEnum   OIndex;

  enum MXMLOTypeEnum OList[] = { mxoUser, mxoGroup, mxoAcct, mxoQOS, mxoClass, mxoNONE };

  must_t *SPtr;

  mbool_t DoProfile;

  char   *NameP;

  mhashiter_t HTI;

  for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
    {
    OIndex = OList[oindex];

    DoProfile = FALSE;

    switch (OIndex)
      {
      case mxoUser:

        if (MSched.DefaultU != NULL)
          {
          DoProfile = MSched.DefaultU->ProfilingIsEnabled;
          }

        break;

      case mxoGroup:

        if (MSched.DefaultG != NULL)
          {
          DoProfile = MSched.DefaultG->ProfilingIsEnabled;
          }

        break;

      case mxoAcct:

        if (MSched.DefaultA != NULL)
          {
          DoProfile = MSched.DefaultA->ProfilingIsEnabled;
          }

        break;

      case mxoQOS:

        if (MSched.DefaultQ != NULL)
          {
          DoProfile = MSched.DefaultQ->ProfilingIsEnabled;
          }

        break;

      case mxoClass:

        if (MSched.DefaultC != NULL)
          {
          DoProfile = MSched.DefaultC->ProfilingIsEnabled;
          }

        break;
 
      default:

        /* NO-OP */

        break;
      }  /* END switch (OIndex) */
 
    if (DoProfile == TRUE)
      {
      /* walk through object to enable their profiling */

      MOINITLOOP(&OP,OIndex,&OE,&HTI);

      while ((O = MOGetNextObject(&OP,OIndex,OE,&HTI,&NameP)) != NULL)
        {
        if ((NameP == NULL) ||
            !strcmp(NameP,NONE) ||
            !strcmp(NameP,ALL) ||
            !strcmp(NameP,"ALL") ||
            !strcmp(NameP,"DEFAULT") ||
            !strcmp(NameP,"NOGROUP"))
          {
          continue;
          }

        if ((MOGetComponent(O,OIndex,(void **)&SPtr,mxoxStats) == FAILURE) ||
            (SPtr->IStat != NULL))
          {
          continue;
          }

        MStatPInitialize((void *)SPtr,FALSE,msoCred);

        /* must allocate O->L.IP */

        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:

            if (((mgcred_t *)O)->L.IdlePolicy == NULL)
              {
              MPUCreate(&((mgcred_t *)O)->L.IdlePolicy);
              }

            break;

          case mxoClass:

            if (((mqos_t *)O)->L.IdlePolicy == NULL)
              {
              MPUCreate(&((mclass_t *)O)->L.IdlePolicy);
              }

            break;

          case mxoQOS:

            if (((mqos_t *)O)->L.IdlePolicy == NULL)
              {
              MPUCreate(&((mqos_t *)O)->L.IdlePolicy);
              }

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */
        }    /* END while (MOGetNextObject()) != NULL) */
      }      /* END if (DoProfile == TRUE) ... */
    }        /* END for (oindex) */

  return(SUCCESS);
  }  /* END MCredSyncDefaultConfig() */
/* END MCredConfig.c */
