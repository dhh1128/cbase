/* HEADER */

      
/**
 * @file MRsvXML.c
 *
 * Contains: Rsv XML functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report reservation state as XML.
 *
 * NOTE:  used to checkpoint/report reservation state.
 *
 * NOTE:  message and ACL attributes stored as XML children, not XML attributes
 *
 * @see MCPStoreRsvList() - parent - checkpoint rsv's (Mode=mcmCP)
 * @see MUIRsvDiagnose()->MRsvShowState() - parent
 * @see MRsvAToMString() - child
 *
 * @param R (I)
 * @param EP (O) [alloc]
 * @param SAList (I) [optional]
 * @param SC (O) [optional]
 * @param VerboseACL (I) store ACL affinity regardless of its value
 * @param Mode (I) [mcmCP]
 */

int MRsvToXML(

  mrsv_t             *R,          /* I */
  mxml_t            **EP,         /* O (alloc) */
  enum MRsvAttrEnum  *SAList,     /* I (optional) specified attribute list */
  int                *SC,         /* O (optional) */
  mbool_t             VerboseACL, /* I */
  enum MCModeEnum     Mode)       /* I (mcmCP) */

  {
  mbool_t PrintTriggers = FALSE;
  mbool_t PrintVars     = FALSE;

  /* default rsv attribute list */

  const enum MRsvAttrEnum DAList[] = {
    mraName,
    mraLabel,
    mraAllocNodeList,
    mraType,
    mraStartTime,
    mraEndTime,
    mraCTime,
    mraDuration,
    mraACL,       /* NOTE: remove when MACLFromXML is supported */
    mraComment,
    mraCost,
    mraExcludeRsv,
    mraExpireTime,
    mraFlags,
    mraIsTracked,
    mraLastChargeTime,
    mraLogLevel,
    mraAAccount,
    mraAGroup,
    mraAUser,
    mraAQOS,
    mraPartition,  /* NOTE: has no effect on CP, partitions are created AFTER rsv */
    mraPriority,
    mraResources,
    mraHostExp,
    mraHostExpIsSpecified,
    mraReqArch,
    mraReqFeatureList,
    mraReqFeatureMode,
    mraReqMemory,
    mraReqNodeCount,
    mraReqNodeList,
    mraReqOS,
    mraReqTaskCount,
    mraRsvGroup,
    mraRsvParent,
    mraSpecName,
    mraStatCAPS,
    mraStatCIPS,
    mraStatTAPS,
    mraStatTIPS,
    mraSubType,
    mraSysJobID,
    mraOwner,
    mraVariables,
    mraGlobalID,
    mraSID,
    mraNONE };

  /* NOTE: do NOT checkpoint trigger, handled by checkpointing trigger itself */

  int  aindex;

  enum MRsvAttrEnum *AList;

  /* allow LARGE hostlist up to MMAX_NODE */

  mxml_t *E;

  if ((R == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  if (*EP == NULL)
    {
    if (MXMLCreateE(EP,(char*) MXO[mxoRsv]) == FAILURE)
      {
      return(FAILURE);
      }
    }

  E = *EP;

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MRsvAttrEnum *)DAList;

  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mraNONE;aindex++)
    {
    if (AList[aindex] == mraVariables)
      {
      /* variables are handled as children XML */

      PrintVars = TRUE;

      continue;
      }

    if (AList[aindex] == mraTrigger)
      {
      /* triggers are handled as children XML */

      PrintTriggers = TRUE;

      continue;
      }

    if ((AList[aindex] == mraACL) || ( AList[aindex] == mraCL))
      {
      /* ACL/CL are handled with a special case after the loop */

      continue;
      }

    if (AList[aindex] == mraMessages)
      {
      /* messages are handled with a special case after the loop */

      continue;
      }

    if ((AList[aindex] == mraHostExp) &&
        (R->HostExpIsSpecified == MBNOTSET) &&
        (!MNLIsEmpty(&R->NL)))
      continue;

    if (MRsvAToMString(
          R,
          AList[aindex],
          &tmpString,
          Mode) == FAILURE)
      {
      MDB(7,fSTRUCT) MLog("WARNING:  could not convert rsv '%s' attr '%s' to string\n",
        R->Name,
        MRsvAttr[AList[aindex]]);

      continue;
      }

    if (tmpString.empty())
      {
      continue;
      }

    MXMLSetAttr(E,(char *)MRsvAttr[AList[aindex]],tmpString.c_str(),mdfString);
    }  /* END for (aindex) */

  if (R->MB != NULL)
    {
    mxml_t *ME = NULL;

    MRsvAToMString(R,mraMessages,&tmpString,Mode);

    if (MXMLFromString(&ME,tmpString.c_str(),NULL,NULL) == SUCCESS)
      {
      MXMLAddE(E,ME);
      }
    }

  if (!MACLIsEmpty(R->ACL))
    {
    macl_t *tACL;
    mxml_t *AE = NULL;

    for (tACL = R->ACL;tACL != NULL;tACL = tACL->Next)
      {
      AE = NULL;

      MACLToXML(tACL,&AE,NULL,FALSE);

      MXMLAddE(E,AE);
      }
    }    /* END if (!MACLIsEmpty(R->ACL)) */

  /* treat CL like ACL and create child elements */

  if (!MACLIsEmpty(R->CL))
    {
    macl_t *tCL;
    mxml_t *CLE = NULL;

    for (tCL = R->CL;tCL != NULL;tCL = tCL->Next)
      {
      CLE = NULL;

      MACLToXML(tCL,&CLE,NULL,TRUE);

      MXMLAddE(E,CLE);
      }
    } /* END if if (!MACLIsEmpty(R->CL)) */

  if (R->History != NULL)
    {
    mxml_t *HE = NULL;

    MHistoryToXML(R,mxoRsv,&HE);

    MXMLAddE(E,HE);
    }

  if ((PrintVars == TRUE) && (R->Variables != NULL))
    {
    MUAddVarsLLToXML(E,R->Variables);
    }

  if ((PrintTriggers == TRUE) && (R->T != NULL))
    {
    mtrig_t *T = NULL;
    mxml_t *TE = NULL;

    int tindex;

    for (tindex = 0; tindex < R->T->NumItems;tindex++)
      {
      /* Check for any active triggers */

      T = (mtrig_t *)MUArrayListGetPtr(R->T,tindex);

      /* If it is a reservation profile, we will just print everything */

      if ((MTrigIsReal(T) == FALSE) &&
          (!bmisset(&R->IFlags,mrifReservationProfile)))
        continue;

      TE = NULL;

      if (MXMLCreateE(&TE,(char *)MXO[mxoTrig]) == FAILURE)
        {
        /* cannot create object */

        continue;
        }

      MTrigToXML(T,TE,NULL);

      MXMLAddE(E,TE);
      }  /* END for (tindex) */
    }    /* END if (DoTriggerAsChild == TRUE) */

  return(SUCCESS);
  }  /* END MRsvToXML() */



/**
 * Update reservation R with XML attributes.
 *
 * @see MRsvLoadCP() - parent - load XML from ckpt file
 * @see MRsvCreateFromXML() - parent - load XML from peer
 *
 * @param R (I) [modified]
 * @param E (I)
 * @param EMsg (O) [optional]
 */

int MRsvFromXML(

  mrsv_t *R,    /* I (modified) */
  mxml_t *E,    /* I */
  char   *EMsg) /* O (optional) */

  {
  int aindex;

  mxml_t *ME;

  int     CTok;

  enum MRsvAttrEnum raindex;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((R == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  do not initialize.  may be overlaying data */

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    raindex = (enum MRsvAttrEnum)MUGetIndexCI(E->AName[aindex],MRsvAttr,FALSE,mraNONE);

    if (raindex == mraNONE)
      {
      /* NOTE:  change in Moab 5.3 to not silently ignore bad attributes */

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        snprintf(EMsg,MMAX_LINE,"cannot process unknown rsv attribute '%s'",
          E->AName[aindex]);
        }

      continue;
      }

    if (raindex == mraPartition)
      {
      /* If partition doesn't exist, add it.  This is needed so that 
         reservation hostlists can be correctly evaluated */

      MParAdd(E->AVal[aindex],NULL);
      }
 
    if (MRsvSetAttr(
          R,
          raindex,
          (void **)E->AVal[aindex],
          mdfString,
          mSet) == FAILURE)
      {
      /* NOTE:  change in Moab 5.3 to not silently ignore bad attributes */

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        snprintf(EMsg,MMAX_LINE,"cannot set rsv attribute %s to '%s'",
          E->AName[aindex],
          (E->AVal[aindex] != NULL) ? E->AVal[aindex] : "");
        }

      continue;
      }
    }  /* END for (aindex) */

  if (MXMLGetChild(E,(char *)MRsvAttr[mraMessages],NULL,&ME) == SUCCESS)
    {
    MMBFromXML(&R->MB,ME);
    }

  CTok = -1;

  while (MXMLGetChild(E,(char *)MRsvAttr[mraACL],&CTok,&ME) == SUCCESS)
    {
    MACLFromXML(&R->ACL,ME,TRUE);
    }

  while (MXMLGetChild(E,(char *)MRsvAttr[mraCL],&CTok,&ME) == SUCCESS)
    {
    MACLFromXML(&R->CL,ME,TRUE);
    }

  if (MXMLGetChild(E,(char *)MRsvAttr[mraHistory],NULL,&ME) == SUCCESS)
    {
    MHistoryFromXML(R,mxoRsv,ME);
    }

  /* Parse any 'Variables' entries */

  if (MXMLGetChild(E,"Variables",NULL,&ME) == SUCCESS)
    {
    mxml_t *VarE = NULL;
    int VarETok = -1;
    char VarName[MMAX_LINE];

    /* get each "Variable" child and add to linked list */

    while (MXMLGetChildCI(ME,(char*)MRsvAttr[mraVariables],&VarETok,&VarE) == SUCCESS)
      {
      char *VarValue = NULL;

      /* Get the value for the "name" attribute. If there is one, attempt to add it to variables */
      if (MXMLGetAttr(VarE,"name",NULL,VarName,sizeof(VarName) == SUCCESS))
        {
        /* If an attribute was present, then add it */
        if ((VarE->Val != NULL) && (VarE->Val[0] != '\0'))
          {
          MUStrDup(&VarValue,VarE->Val);    /* Create a new string object */

          /* Now verify that we have a valid 'VarValue', and if so, add it to the R variable */
          if (VarValue != NULL)
            MULLAdd(&R->Variables,VarName,strdup(VarValue),NULL,MUFREE);

          MUFree(&VarValue);   /* Free the intermediate buffer */
          }
        }
      }
    }

  return(SUCCESS);
  }  /* END MRsvFromXML() */




/**
 *
 *
 * @param R (I)
 * @param EP (O) [alloc]
 * @param SAList (I) [optional]
 * @param PR (I) [bitmap of enum MCModeEnum]
 */

int MRsvToExportXML(

  mrsv_t             *R,      /* I */
  mxml_t            **EP,     /* O (alloc) */
  enum MRsvAttrEnum  *SAList, /* I (optional) */
  mrm_t              *PR)     /* I (peer) */

  {
  int  aindex;
  int   index = 0;

  enum MRsvAttrEnum *AList;

  enum MRsvAttrEnum  DAList[] = {
    mraACL,
    mraHostExp,
    mraReqNodeList,
    mraOwner };

  char tmpLine[MMAX_LINE];

  mxml_t *E;

  if ((R == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  if (*EP == NULL)
    {
    if (MXMLCreateE(EP,(char*) MXO[mxoRsv]) == FAILURE)
      {
      return(FAILURE);
      }
    }

  E = *EP;

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MRsvAttrEnum *)DAList;

  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mraNONE;aindex++)
    {
    tmpString.clear();

    switch (AList[aindex])
      {
      case mraACL:

        {
        char Line[MMAX_LINE];

        macl_t *tmpACL;

        tmpACL = R->ACL;

        while (tmpACL != NULL)
          {
          if (tmpString.empty())
            MStringAppendF(&tmpString,";%s",
              MACLShow(tmpACL,NULL,TRUE,(PR == NULL) ? NULL : PR->OMap,FALSE,Line));
          else
            MStringAppendF(&tmpString,"%s",
              MACLShow(tmpACL,NULL,TRUE,(PR == NULL) ? NULL : PR->OMap,FALSE,Line));

          tmpACL = tmpACL->Next;
          }  /* END for (cindex) */

        if (tmpString.empty())
          MXMLSetAttr(E,(char *)MRsvAttr[AList[aindex]],tmpString.c_str(),mdfString);
        }

        break;

      case mraHostExp:

        /* NOTE: for the reservation to be succesfully created must also
                 export R->ReqNC, R->ReqTPN, R->ReqTC */

        if (!MNLIsEmpty(&R->NL))
          {
          int      NC = 0;
          int      TC = 0;

          mnode_t *N;

          for (index = 0;MNLGetNodeAtIndex(&R->NL,index,&N) == SUCCESS;index++)
            {
            MOMap(
              (PR == NULL) ? NULL : PR->OMap,
              mxoNode,
              N->Name,
              FALSE,
              FALSE,
              tmpLine);

            MStringAppendF(&tmpString,"%s:%d,",tmpLine,MNLGetTCAtIndex(&R->NL,index));

            NC++;
            }

          if (!tmpString.empty())
            {
            MXMLSetAttr(E,(char *)MRsvAttr[mraHostExp],tmpString.c_str(),mdfString);
            MXMLSetAttr(E,(char *)MRsvAttr[mraReqNodeCount],&NC,mdfInt);
            MXMLSetAttr(E,(char *)MRsvAttr[mraReqTaskCount],&TC,mdfInt);
            MXMLSetAttr(E,(char *)MRsvAttr[mraReqTPN],(char *)"1",mdfString);
            }
          }

        break;

      case mraOwner:

        MRsvAToMString(R,mraOwner,&tmpString,0);

        if (!tmpString.empty())
          MXMLSetAttr(E,(char *)MRsvAttr[mraHostExp],tmpString.c_str(),mdfString);

        break;

      case mraReqNodeList:

        MRsvAToMString(R,mraReqNodeList,&tmpString,0);

        if (!tmpString.empty())
          {
          MXMLSetAttr(E,(char *)MRsvAttr[mraReqNodeList],tmpString.c_str(),mdfString);
          }

        break;

      default:

        /* NO-OP */

        break;
      }
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MRsvToExportXML() */





/**
 * Create reservation from XML.
 *
 * NOTE: this routine is only used to create reservations imported from peers
 *       it is not used to create reservations from checkpoint files
 *
 * NOTE: this routine will assign a locally unique ID to the reservation
 *
 * NOTE: rsv attributes imported from peers must be explicitly handled within
 *       this routine.
 *
 * @see MCPStoreRsvList()
 * @see MRsvLoadCP() - peer - loads rsv attributes from checkpoint files.
 * @see MRsvCreate() - child
 * @see MMRsvQueryPostLoad() - parent
 * @see MRsvFromXML() - child
 *
 * @param RS (I) [optional]
 * @param RsvID (I)
 * @param RE (I)
 * @param RP (O) [optional]
 * @param PR (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MRsvCreateFromXML(

  mrsv_t  *RS,     /* I (optional) */
  char    *RsvID,  /* I */
  mxml_t  *RE,     /* I */
  mrsv_t **RP,     /* O (optional) */
  mrm_t   *PR,     /* I (optional) */
  char    *EMsg)   /* O (optional,minsize=MMAX_LINE) */

  {
  mrsv_t  *R;
  mrsv_t  *RsvP = NULL;

  mrsv_t   tmpR;

  int      rc;

  mnl_t NodeList = {0};

  char tmpBuf[MMAX_BUFFER];
  char tmpName[MMAX_LINE];
  char tmpEMsg[MMAX_LINE];

  mbool_t   RIsLocal = FALSE;

  const char *FName = "MRsvCreateFromXML";

  MDB(4,fCKPT) MLog("%s(RS,%s,RE,RP,%s,EMsg)\n",
    FName,
    (RsvID != NULL) ? RsvID : "NULL",
    (PR != NULL) ? PR->Name : "NULL");

  if (RP != NULL)
    *RP = NULL;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((RE == NULL) || (RsvID == NULL) || (RsvID[0] == '\0'))
    {
    return(FAILURE);
    }

  /* if RS is specified, update RS.  if not, load rsv ckpt line into new rsv */

  if (RS == NULL)
    {
    if (MRsvFind(RsvID,&R,mraNONE) != SUCCESS)
      {
      RIsLocal = TRUE;

      R = &tmpR;

      MRsvInitialize(&R);
      }
    }
  else
    {
    R = RS;
    }

  rc = MRsvFromXML(R,RE,tmpEMsg);

  if (rc == FAILURE)
    {
    MDB(2,fCKPT) MLog("WARNING:  cannot update reservation %s from XML - %s\n",
      (RS != NULL) ? RS->Name : "",
      EMsg);

    if (RIsLocal == TRUE)
      MRsvDestroy(&R,FALSE,FALSE);

    if (EMsg != NULL)
      sprintf(EMsg,"cannot parse XML into rsv");

    return(FAILURE);
    }
  else if (tmpEMsg[0] != '\0')
    {
    /* XML warning received */

    if (EMsg != NULL)
      MUStrCpy(EMsg,tmpEMsg,MMAX_LINE);

    return(FAILURE);
    }

  if ((R->RsvParent != NULL) &&
      (MSRFind(R->RsvParent,NULL,NULL) != SUCCESS) &&
      (MRsvFind(R->RsvParent,NULL,mraNONE) != SUCCESS) &&
      !strcmp(R->RsvParent,R->Name))
    {
    /* DOUG:  Why can't it match other admin rsv's in the same rsv group? */

    /* reservation specifies reservation group and rsvgroup does not match
       existing SRsv, Rsv, or self. */

    MDB(5,fCKPT) MLog("INFO:     parent reservation not located - destroying reservation %s\n",
      R->RsvParent);

    if (EMsg != NULL)
      sprintf(EMsg,"cannot locate rsv parent");

    return(FAILURE);
    }

  MNLInit(&NodeList);

  if ((PR != NULL) && (PR->OMap != NULL))
    {
    /* map ACL, HostExp, Owner */

    mstring_t mappedHostExp(MMAX_BUFFER);

    if (!MACLIsEmpty(R->ACL))
      {
      if (MXMLGetAttr(RE,(char *)MRsvAttr[mraACL],NULL,tmpBuf,sizeof(tmpBuf)) == SUCCESS)
        {
        MACLLoadConfigLine(&R->ACL,tmpBuf,mSet,PR->OMap,TRUE);
        }
      }

    if ((R->HostExp != NULL) &&
        (MOMapList(PR->OMap,mxoNode,R->HostExp,TRUE,FALSE,&mappedHostExp) == SUCCESS))
      {
      char  tmpHostExp[MMAX_BUFFER];

      char *BPtr = NULL;
      int   BSpace = 0;

      char *ptr;
      char *TokPtr = NULL;

      char *ptr2;
      char *TokPtr2 = NULL;

      int   nindex;

      mnode_t *N;

      /* need a mutable char array from the mstring */

      char *mutableHostString = NULL;
      MUStrDup(&mutableHostString,mappedHostExp.c_str());

      /* generate nodelist for MRsvCreate() */

      /* FIXME: should support "<node>:<tc>[,<node>:<tc>]..." */

      ptr = MUStrTok(mutableHostString,",",&TokPtr);

      nindex = 0;

      MUSNInit(&BPtr,&BSpace,tmpHostExp,sizeof(tmpHostExp));

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,":",&TokPtr2);

        if (MNodeFind(ptr2,&N) == SUCCESS)
          {
          MNLSetNodeAtIndex(&NodeList,nindex,N);
          MNLSetTCAtIndex(&NodeList,nindex,1);

          if (nindex == 0)
            MUSNPrintF(&BPtr,&BSpace,"%s",ptr2);
          else
            MUSNPrintF(&BPtr,&BSpace,",%s",ptr2);
          }

        if ((TokPtr2 != NULL) && (TokPtr2[0] != '\0'))
          {
          MNLSetTCAtIndex(&NodeList,nindex,(int)strtol(TokPtr2,NULL,0));
          }

        nindex++;

        ptr = MUStrTok(NULL,",",&TokPtr);
        }

      MNLTerminateAtIndex(&NodeList,nindex);

      MUStrDup(&R->HostExp,tmpHostExp);

      /* Free the tmp mutable host string */
      MUFree(&mutableHostString);
      }

    if (R->OType != mxoNONE)
      {
      /* NO-OP? */
      }
    }
  else if (R->HostExp != NULL)
    {
    char  tmpHostExp[MMAX_BUFFER];

    char *BPtr;
    int   BSpace;

    char *ptr;
    char *TokPtr;

    char *ptr2;
    char *TokPtr2=NULL;

    int   nindex;

    mnode_t *N;

    /* generate nodelist for MRsvCreate() */

    /* FIXME: should support "<node>:<tc>[,<node>:<tc>]..." */

    MUStrCpy(tmpBuf,R->HostExp,sizeof(tmpBuf));

    ptr = MUStrTok(tmpBuf,",",&TokPtr);

    nindex = 0;

    MUSNInit(&BPtr,&BSpace,tmpHostExp,sizeof(tmpHostExp));

    while (ptr != NULL)
      {
      ptr2 = MUStrTok(ptr,":",&TokPtr2);

      if (MNodeFind(ptr2,&N) == SUCCESS)
        {
        MNLSetNodeAtIndex(&NodeList,nindex,N);
        MNLSetTCAtIndex(&NodeList,nindex,1);

        if (nindex == 0)
          MUSNPrintF(&BPtr,&BSpace,"%s",ptr2);
        else
          MUSNPrintF(&BPtr,&BSpace,",%s",ptr2);
        }

      if ((TokPtr2 != NULL) && (TokPtr2[0] != '\0'))
        {
        MNLSetTCAtIndex(&NodeList,nindex,(int)strtol(TokPtr2,NULL,0));
        }

      nindex++;

      ptr = MUStrTok(NULL,",",&TokPtr);
      }

    MNLTerminateAtIndex(&NodeList,nindex);

    MUStrDup(&R->HostExp,tmpHostExp);
    }  /* END else if (R->HostExp != NULL) */

  if (RIsLocal == TRUE)
    {
    /* create new rsv */

    MRsvGetUID(R->Name,-1,NULL,tmpName);

    if (MRsvCreate(
         R->Type,
         R->ACL,
         R->CL,
         &R->Flags,
         &NodeList,     /* ReqNL is NULL for most reservations */
         R->StartTime,
         R->EndTime,
         R->ReqNC,
         R->ReqTC,
         tmpName,
         &RsvP,       /* O - pointer to created rsv */
         R->HostExp,
         &R->DRes,
         EMsg,        /* O */
         FALSE,
         TRUE) == FAILURE)
      {
      /* cannot create reservation */

      MDB(2,fCKPT) MLog("WARNING:  cannot create new reservation from checkpoint XML for reservation %s\n",
        (RS != NULL) ? RS->Name : "");

      if (RIsLocal == TRUE)
        MRsvDestroy(&R,FALSE,FALSE);

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        sprintf(EMsg,"cannot create new rsv");

      MNLFree(&NodeList);

      return(FAILURE);
      }

    MNLFree(&NodeList);

    if (R->Label != NULL)
      {
      MRsvSetAttr(RsvP,mraLabel,(void **)R->Label,mdfString,mSet);
      }

    RsvP->CAPS = R->CAPS;
    RsvP->CIPS = R->CIPS;

    RsvP->TAPS = R->TAPS;
    RsvP->TIPS = R->TIPS;

    RsvP->LienCost = R->LienCost;

    /* NOTE:  do not load R->AllocTC and R->AllocNC from checkpoint, these should be
              regenerated */

    if (!bmisclear(&R->ReqFBM))
      MRsvSetAttr(RsvP,mraReqFeatureList,&R->ReqFBM,mdfOther,mSet);

    RsvP->ReqFMode = R->ReqFMode;

    RsvP->CTime    = R->CTime;

    RsvP->CIteration = MSched.Iteration;

    RsvP->IsTracked = R->IsTracked;

    RsvP->LogLevel  = R->LogLevel;

    RsvP->SubType   = R->SubType;

    if (R->A != NULL)
      MRsvSetAttr(RsvP,mraAAccount,(void *)R->A,mdfOther,mSet);

    if (R->G != NULL)
      MRsvSetAttr(RsvP,mraAGroup,(void *)R->G,mdfOther,mSet);

    if (R->U != NULL)
      MRsvSetAttr(RsvP,mraAUser,(void *)R->U,mdfOther,mSet);

    if (R->Q != NULL)
      MRsvSetAttr(RsvP,mraAQOS,(void *)R->Q,mdfOther,mSet);

    if (R->ReqTC > 0)
      MRsvSetAttr(RsvP,mraReqTaskCount,(void *)&R->ReqTC,mdfInt,mSet);

    if (R->HostExpIsSpecified == TRUE)
      RsvP->HostExpIsSpecified = TRUE;

    if (R->MB != NULL)
      {
      MMovePtr((char **)&R->MB,(char **)&RsvP->MB);
      }

    if (R->SystemID != NULL)
      {
      MMovePtr(&R->SystemID,&RsvP->SystemID);
      }

    if (R->SystemRID != NULL)
      {
      MMovePtr(&R->SystemRID,&RsvP->SystemRID);
      }

    if (R->Comment != NULL)
      {
      MMovePtr(&R->Comment,&RsvP->Comment);
      }

    if (R->RsvGroup != NULL)
      {
      MMovePtr(&R->RsvGroup,&RsvP->RsvGroup);
      }

    if ((!MNLIsEmpty(&R->ReqNL)) && (MNLIsEmpty(&RsvP->ReqNL)))
      {
      /* must preserve ReqNL because it holds the TaskCount information (6-15-05) */

      MNLCopy(&RsvP->ReqNL,&R->ReqNL);
      }

    if (R->T != NULL)
      {
      MMovePtr((char **)&R->T,(char **)&RsvP->T);
      }

    if (R->Variables != NULL)
      {
      MMovePtr((char **)&R->Variables,(char **)&RsvP->Variables);
      }

    if (!bmisclear(&R->ReqFBM))
      {
      bmcopy(&RsvP->ReqFBM,&R->ReqFBM);

      bmclear(&R->ReqFBM);
      }

    if (R->RsvParent != NULL)
      {
      /* NOTE:  this section should sync checkpointed rsvs against the parent SR */

      msrsv_t *SR;

      MMovePtr(&R->RsvParent,&RsvP->RsvParent);

      /* If RsvGroup not explicitly specified, RsvGroup should be set to RsvParent */

      if (RsvP->RsvGroup == NULL)
        MUStrDup(&RsvP->RsvGroup,RsvP->RsvParent);

      if (MSRFind(RsvP->RsvParent,&SR,NULL) == SUCCESS)
        {
        mulong StartTime;
        mulong EndTime;

        long Offset;
        int  dindex;

        /* reservation is SR child, link to parent */

        /* determine time slot */

        Offset = RsvP->StartTime - MSched.Time;

        switch (SR->Period)
          {
          case mpHour:

            dindex = (Offset / MCONST_HOURLEN);

            if (Offset % MCONST_HOURLEN > 0)
              dindex++;

            break;

          case mpDay:

            dindex = (Offset / MCONST_DAYLEN);

            if (Offset % MCONST_DAYLEN > 0)
              dindex++;

            break;

          case mpWeek:

            dindex = (Offset / MCONST_WEEKLEN);

            if (Offset % MCONST_WEEKLEN > 0)
              dindex++;

            break;

          case mpMonth:

            dindex = (Offset / MCONST_MONTHLEN);

            if (Offset % MCONST_MONTHLEN > 0)
              dindex++;

            break;

          default:
          case mpInfinity:

            dindex = 0;

            break;
          }  /* END switch (SR->Period) */

        /* NOTE:  if the SR has changed ANY aspect of time (period,starttime,endtime,days)
                  delete this child rsv and rebuild from scratch */

        if (dindex >= SR->Depth)
          {
          /* this depth is not needed */

          MRsvDestroy(&RsvP,TRUE,TRUE);

          MDB(5,fCKPT) MLog("INFO:     reservation for specified period no longer needed - destroying reservation %s\n",
            (RS != NULL) ? RS->Name : "");

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          if (EMsg != NULL)
            sprintf(EMsg,"rsv has expired");

          return(FAILURE);
          }  /* END if (dindex >= SR->Depth) */

        MSRGetAttributes(SR,dindex,&StartTime,&EndTime);

        if (((RsvP->StartTime > StartTime) && (StartTime != MSched.Time)) ||
            (RsvP->EndTime != StartTime + EndTime))
          {
          /* time settings on SR have changed, delete CP rsv and start from scratch */

          MRsvDestroy(&RsvP,TRUE,TRUE);

          MDB(5,fCKPT) MLog("INFO:     reservation time constraints have changed - destroying reservation %s\n",
            (RS != NULL) ? RS->Name : "");

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          if (EMsg != NULL)
            sprintf(EMsg,"time constraints have changed");

          return(FAILURE);
          }

        SR->R[dindex] = RsvP;

        if (SR->ReqTC != RsvP->ReqTC)
          RsvP->ReqTC = SR->ReqTC;

        if (SR->DRes.Procs != RsvP->DRes.Procs)
          RsvP->DRes.Procs = SR->DRes.Procs;
        } /* END if (MSRFind() == SUCCESS) */
      }
    }  /* END if (RIsLocal == TRUE) */

  if (RIsLocal == TRUE)
    {
    /* DOUG:  Should parameter 3 be FALSE? */

    MRsvDestroy(&R,FALSE,TRUE);
    }

  if (RP != NULL)
    *RP = RsvP;

  MNLFree(&NodeList);
  return(SUCCESS);
  }  /* END MRsvCreateFromXML() */
/* END MRsvXML.c */
