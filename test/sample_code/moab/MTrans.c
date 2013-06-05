#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"



mtrans_t MTrans[MMAX_TRANS + 10];


/**
 * Set attribute w/in specified transaction.
 *
 * @param TID (I)
 * @param AIndex (I)
 * @param Val (I)
 */



/**
 * Set attribute w/in specified transaction.
 *
 * @param TID (I)
 * @param AIndex (I)
 * @param Val (I)
 */

int MTransSet(

  int                  TID,    /* I */
  enum MTransAttrEnum  AIndex, /* I */
  const char                *Val)    /* I */

  {
  int tindex;

  if ((TID <= 0) || (AIndex == mtransaNONE))
    {
    return(FAILURE);
    }

  for (tindex = 0;tindex < MMAX_TRANS;tindex++)
    {
    if (MTrans[tindex].ID == -1)
      {
      continue;
      }

    if (MTrans[tindex].ID == 0)
      {
      /* end of list located */

      break;
      }

    if (MTrans[tindex].ID == TID)
      {
      switch (AIndex)
        {
        case mtransaCost:
        case mtransaDependentTIDList:
        case mtransaMShowCmd:
        case mtransaVariables:
        default:

          MUStrDup(&MTrans[tindex].Val[AIndex],Val);

          break;
        }  /* END switch (AIndex) */

      return(SUCCESS);
      }
    }    /* END for (tindex) */

  return(FAILURE);
  }  /* END MTransSet() */





/**
 * Locate specified transaction.
 *
 * @see MTransAdd() - peer
 * 
 * @param TID (I)
 * @param Requestor (I) [optional] 
 * @param Trans (O)
 */

int MTransFind(

  int         TID,       /* I */
  char       *Requestor, /* I (optional) */
  mtrans_t  **Trans)     /* O */

  {
  int tindex;

  if (TID <= 0) 
    {
    return(FAILURE);
    }

  for (tindex = 0;tindex < MMAX_TRANS;tindex++)
    {
    if (MTrans[tindex].ID == -1)
      {
      continue;
      }

    if (MTrans[tindex].ID == 0)
      {
      /* end of list located */

      break;
      }

    if (MTrans[tindex].ID == TID)
      {
      /* match located */

      if ((Requestor != NULL) && 
          (MTrans[tindex].Requestor != NULL) &&
          (strcmp(Requestor,MTrans[tindex].Requestor)))
        {
        mbitmap_t TmpFlags;
        /* Check if Requestor is an Admin1 user */
 
        MSysGetAuth(Requestor,mcsNONE,0,&TmpFlags);

        if (!bmisset(&TmpFlags,mcalAdmin1))
          {
          /* requestor not authorized to access TID */

          return(FAILURE);
          }
        }

      *Trans = &MTrans[tindex];

      return(SUCCESS);
      }  /* END if (MTID[tindex] == TID) */
    }    /* END for (tindex) */

  return(FAILURE);
  }  /* END MTransFind() */





/**
 * Add new transaction to transaction table.
 *
 * @see MTransFind() - peer
 * @see MTransDestroy() - peer
 *
 * @param Trans            (I) [optional]
 * @param Requestor        (I) [optional]
 * @param NodeList         (I)
 * @param Duration         (I) [optional]
 * @param StartTime        (I) [optional]
 * @param Flags            (I) [optional]
 * @param OpSys            (I) [optional]
 * @param DRes             (I) [optional]
 * @param RsvProfile       (I) [optional]
 * @param Label            (I) [optional]
 * @param VCDescription    (I) [optional] - Name of a VC to create
 * @param VCMaster         (I) [optional] - VC to bind multiple VCs in an mshow
 * @param DependentTIDList (I) [optional]
 * @param MShowCmd         (I) [optional]
 * @param VPCProfile       (I) [optional]
 * @param ACLLine          (I) [optional]
 * @param FeatureLine      (I) [optional]
 * @param TID              (O) [optional]
 */

int MTransAdd(

  mtrans_t *Trans,
  char     *Requestor,
  char     *NodeList,
  char     *Duration,
  char     *StartTime,
  char     *Flags,
  char     *OpSys,
  char     *DRes,
  char     *RsvProfile,
  char     *Label,
  char     *VCDescription,
  char     *VCMaster,
  char     *DependentTIDList,
  char     *MShowCmd,
  char     *VPCProfile,
  char     *ACLLine,
  char     *FeatureLine,
  int      *TID)

  {
  char Name[MMAX_NAME];

  int tindex;

  const char *FName = "MTransAdd";

  MDB(7,fCORE) MLog("%s(T,'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')\n",
    FName, 
    (Requestor != NULL) ? Requestor: "NULL",
    (NodeList != NULL) ? NodeList: "NULL",
    (Duration != NULL) ? Duration: "NULL",
    (StartTime != NULL) ? StartTime: "NULL",
    (Flags != NULL) ? Flags: "NULL",
    (OpSys != NULL) ? OpSys: "NULL",
    (DRes != NULL) ? DRes: "NULL",
    (RsvProfile != NULL) ? RsvProfile: "NULL",
    (Label != NULL) ? Label: "NULL",
    (VCDescription != NULL) ? VCDescription: "NULL",
    (VCMaster != NULL) ? VCMaster: "NULL",
    (DependentTIDList != NULL) ? DependentTIDList: "NULL",
    (MShowCmd != NULL) ? MShowCmd: "NULL",
    (VPCProfile != NULL) ? VPCProfile: "NULL",
    (ACLLine != NULL) ? ACLLine: "NULL",
    (FeatureLine != NULL) ? FeatureLine : "NULL");

  if (NodeList == NULL)
    { 
    /* at least one data spec required */

    return(FAILURE);
    }

  /* always generate unique transaction id */

  /* NOTE:  expire transaction more than MMAX_TRANS transactions old */

  for (tindex = 0;tindex < MMAX_TRANS;tindex++)
    {
    if (MTrans[tindex].ID > 0)
      {
      if (((MSched.TransCounter + 1) - MTrans[tindex].ID) <= MMAX_TRANS) /* the "+ 1" accounts for TIDs starting at 1 (not 0) */
        {
        continue;
        }
      else
        {
        /* slot is populated */

        MTransFree(&MTrans[tindex]);
        }
      }  /* END if (MTrans[tindex].ID > 0) */

    /* add transaction data */

    MTrans[tindex].ID = MSched.TransCounter;

    snprintf(Name,sizeof(Name),"%d",MTrans[tindex].ID);

    MUStrDup(&MTrans[tindex].Requestor,Requestor);

    MUStrDup(&MTrans[tindex].Val[mtransaNodeList],NodeList);
    MUStrDup(&MTrans[tindex].Val[mtransaDuration],Duration);
    MUStrDup(&MTrans[tindex].Val[mtransaStartTime],StartTime);
    MUStrDup(&MTrans[tindex].Val[mtransaFlags],Flags);
    MUStrDup(&MTrans[tindex].Val[mtransaOpSys],OpSys);
    MUStrDup(&MTrans[tindex].Val[mtransaDRes],DRes);
    MUStrDup(&MTrans[tindex].Val[mtransaRsvProfile],RsvProfile);
    MUStrDup(&MTrans[tindex].Val[mtransaVPCProfile],VPCProfile);
    MUStrDup(&MTrans[tindex].Val[mtransaLabel],Label);
    MUStrDup(&MTrans[tindex].Val[mtransaVCDescription],VCDescription);
    MUStrDup(&MTrans[tindex].Val[mtransaVCMaster],VCMaster);
    MUStrDup(&MTrans[tindex].Val[mtransaDependentTIDList],DependentTIDList);
    MUStrDup(&MTrans[tindex].Val[mtransaMShowCmd],MShowCmd);
    MUStrDup(&MTrans[tindex].Val[mtransaACL],ACLLine);
    MUStrDup(&MTrans[tindex].Val[mtransaNodeFeatures],FeatureLine);
    MUStrDup(&MTrans[tindex].Val[mtransaName],Name);

    /* alloc NL */

    MNLInit(&MTrans[tindex].NL);

    MNLFromString(MTrans[tindex].Val[mtransaNodeList],&MTrans[tindex].NL,-1,FALSE);

    if (TID != NULL)
      *TID = MSched.TransCounter;

    MDB(7,fFS) MLog("INFO:     new TID '%d' created\n",MSched.TransCounter);

    MSched.TransCounter++;

    return(SUCCESS);
    }    /* END for (tindex) */

  /* no free slots available */

  return(FAILURE);
  }  /* END MTransAdd() */



/**
 * Report if specified TID is valid for use.
 *
 * @see MUIRsvCreate() - parent
 * @see MTransSetLastCommittedTID() - peer
 *
 * @param TID         (I)
 * @param Placeholder (I)
 */

mbool_t MTransIsValid(

  int      TID,
  mbool_t *Placeholder)

  {
  mtrans_t *Trans;

  if (Placeholder != NULL)
    *Placeholder = FALSE;

  if (MTransFind(TID,NULL,&Trans) == FAILURE)
    {
    return(FALSE);
    }

#ifdef MNEWTIDSYSTEM
  if (Trans->IsInvalid == TRUE)
    {
    return(FALSE);
    }

  return(TRUE);
#else /* MNEWTIDSYSTEM */

  if (Trans->RsvID != NULL)
    {
    mrsv_t *R = NULL;

    /* if the TID has a placeholder reservation then it is always valid */

    if ((MRsvFind(Trans->RsvID,&R,mraNONE) == SUCCESS) &&
        (bmisset(&R->Flags,mrfPlaceholder)))
      {
      if (Placeholder != NULL)
        *Placeholder = TRUE;

      return(TRUE);
      }
    }

  if ((MSched.LastTransCommitted <= -1) || 
      (TID > MSched.LastTransCommitted) ||
      ((TID > MSched.LastTransCommitted) &&
       (MSched.TransCounter < (unsigned int)MSched.LastTransCommitted)))
    {
    return(TRUE);
    }

  return(FALSE);
#endif /* else */
  }  /* END MTransIsValid() */



/**
 * Determine whether a particular node is part of a transaction.
 *
 * @param Trans - transaction to evaluate
 * @param N     - name of node to match
 */

mbool_t MTransHasNode(

  mtrans_t *Trans,
  mnode_t  *N)

  {
  /* NOTE: transactions could store the nodelist in a better format for improved search speed */

  /* do a quick search */

  if (strstr((const char *)Trans->Val[mtransaNodeList],(const char *)N->Name) == NULL)
    {
    return(FALSE);
    }

  if (MNLFind(&Trans->NL,N,NULL) == SUCCESS)
    return(TRUE);

  return(FALSE);
  }  /* END MTransHasNode() */



/**
 * Invalidate other transactions based on node use of specified TID.
 *
 * @param CTrans - committed transaction
 */

int MTransInvalidateOthers(
  
  mtrans_t *CTrans)

  {
  mnode_t *N;

  int nindex;
  int tindex;

  for (nindex = 0;MNLGetNodeAtIndex(&CTrans->NL,nindex,&N) == SUCCESS;nindex++)
    {
    for (tindex = 0;tindex < MMAX_TRANS;tindex++)
      {
      if (MTrans[tindex].ID == 0)
        {
        /* end of list located */

        break;
        }

      if (MTrans[tindex].IsInvalid == TRUE)
        {
        continue;
        }

      if (MTransHasNode(&MTrans[tindex],N))
        {
        MTrans[tindex].IsInvalid = TRUE;
        }
      }  /* END for (tindex) */
    }  /* END for (nindex) */

  return(SUCCESS);
  }  /* END MTransInvalidate() */


/**
 * Set last committed TID value.
 *
 */

int MTransSetLastCommittedTID()

  {
#ifdef MNEWTIDSYSTEM
  mtrans_t *Trans;

  MTransFind(TID,NULL,&Trans);

  MTransInvalidateOthers(Trans);
#else  /* MNEWTIDSYSTEM */
  MSched.LastTransCommitted = MSched.TransCounter - 1;
#endif /* ELSE */

  return(SUCCESS);
  }  /* END MTransSetLastCommittedTID() */






/**
 * Apply a transaction template to a reservation.
 *
 * @param Trans (I)
 * @param R     (I/O) modified
 */

int MTransApplyToRsv(

  mtrans_t *Trans,  /* I */
  mrsv_t   *R)      /* I (modified) */

  {
  mnode_t *N;

  char tmpLine[MMAX_BUFFER];
  char tmpHExp[MMAX_BUFFER];

  char *ptr;
  char *TokPtr;

  char *BPtr;
  int   BSpace;

  int   nindex;

  mnl_t *NL;

  /* NOTE:  set TSpec1 to exact host list expression */

  if ((Trans->Val[mtransaRsvProfile] != NULL) && 
      (Trans->Val[mtransaRsvProfile][0] != '\0'))
    {
    mrsv_t *RProf = NULL;

    if (MRsvProfileFind(Trans->Val[mtransaRsvProfile],&RProf))
      {
      MRsvFromProfile(R,RProf);
      }
    }

  /* FORMAT:  <HOST>[:<TC>][,<HOST>[:<TC>]]... */

  bmset(&R->IFlags,mrifTIDRsv);

  MUStrCpy(tmpLine,Trans->Val[mtransaNodeList],sizeof(tmpLine));

  MRsvSetAttr(R,mraReqNodeList,(void *)tmpLine,mdfString,mSet);

  ptr = MUStrTok(tmpLine,",",&TokPtr);

  MUSNInit(&BPtr,&BSpace,tmpHExp,sizeof(tmpHExp));

  if (MNLIsEmpty(&R->ReqNL))
    {
    return(FAILURE); 
    }

  NL = &R->ReqNL;

  /* generate host expression */

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (nindex == 0)
      {
      MUSNPrintF(&BPtr,&BSpace,"^%s$",
        N->Name);
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"|^%s$",
        N->Name);
      }
    }  /* END for (nindex) */

  MRsvSetAttr(R,mraHostExp,(void *)tmpHExp,mdfString,mSet);

  R->HostExpIsSpecified = TRUE;

  R->ReqHLMode = mhlmExactSet;

  if ((Trans->Val[mtransaStartTime] != NULL) && 
      (Trans->Val[mtransaStartTime][0] != '\0'))
    {
    R->StartTime = strtol(Trans->Val[mtransaStartTime],NULL,10);
    }

  if ((Trans->Val[mtransaDuration] != NULL) && 
      (Trans->Val[mtransaDuration][0] != '\0'))
    {
    R->Duration = strtol(Trans->Val[mtransaDuration],NULL,10);
    R->EndTime  = R->StartTime + R->Duration;
    }

  if ((Trans->Val[mtransaFlags] != NULL) && 
      (Trans->Val[mtransaFlags][0] != '\0'))
    {
    /* process rsv flag */

    if (strstr(Trans->Val[mtransaFlags],"TIMEFLEX"))
      {
      bmset(&R->Flags,mrfTimeFlex);
      }
    }

  if ((Trans->Val[mtransaOpSys] != NULL) && 
      (Trans->Val[mtransaOpSys][0] != '\0'))
    {
    mnode_t *N;

    char tmpBuffer[MMAX_BUFFER];

    MRsvSetAttr(R,mraReqOS,(void *)Trans->Val[mtransaOpSys],mdfString,mSet);

    /* NOTE:  VLP:  set OS only if ACL is set to shared */

    if ((Trans->Val[mtransaFlags] != NULL) && 
        (Trans->Val[mtransaFlags][0] != '\0'))
      {
      /* set OS on all requested resources */

      MUStrCpy(tmpBuffer,Trans->Val[mtransaFlags],sizeof(tmpBuffer));

      ptr = MUStrTok(tmpBuffer,",",&TokPtr);

      while (ptr != NULL)
        {
        if (MNodeFind(ptr,&N) == FAILURE)
          {
          char *ptr2;
          char *TokPtr2;

          if (strchr(ptr,':') == NULL)
            {
            ptr = MUStrTok(NULL,",",&TokPtr);

            continue;
            }

          ptr2 = MUStrTok(ptr,":",&TokPtr2); 

          if ((ptr2 == NULL) || (MNodeFind(ptr2,&N) == FAILURE))
            {
            ptr = MUStrTok(NULL,",",&TokPtr);

            continue;
            }

          MNodeSetAttr(N,mnaOS,(void **)Trans->Val[mtransaOpSys],mdfString,mSet);

          ptr = MUStrTok(NULL,",",&TokPtr);

          continue;
          }

        MNodeSetAttr(N,mnaOS,(void **)Trans->Val[mtransaOpSys],mdfString,mSet);

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END if (TSpec4[0] != '\0') */
    }      /* END if (Tspec5 != NULL) */

  if ((Trans->Val[mtransaDRes] != NULL) && (Trans->Val[mtransaDRes][0] != '\0'))
    {
    /* only allocate explicitly requested resources */

    /* MCResFromString() does not initialize DRes */

    MCResClear(&R->DRes);

    MCResFromString(&R->DRes,Trans->Val[mtransaDRes]);

    R->ReqTPN = 1;

    /* FIXME:  when only reserving generic resources must specify a full taskmap */

    if ((R->DRes.Procs == 0) && 
        (!MSNLIsEmpty(&R->DRes.GenericRes)) &&
        (R->ReqTC <= 1))
      {
      /* determine rsv taskcount */

      R->ReqTC = 0;

      if (!MNLIsEmpty(&R->ReqNL))
        {
        /* extract TC from req hostlist */

        R->ReqTC = MNLSumTC(&R->ReqNL);
        }    /* END if (R->ReqNL != NULL) */

      R->ReqTC = MAX(1,R->ReqTC);
      }
    }    /* END if ((Trans->DRes != NULL) && (Trans->DRes[0] != '\0')) */

  if ((Trans->Val[mtransaLabel] != NULL) && 
      (Trans->Val[mtransaLabel][0] != '\0'))
    {
    MUStrDup(&R->Label,Trans->Val[mtransaLabel]);
    }

  if ((Trans->Val[mtransaACL] != NULL) &&
      (Trans->Val[mtransaACL][0] != '\0'))
    {
    MRsvSetAttr(R,mraACL,Trans->Val[mtransaACL],mdfString,mAdd);
    }

  if ((Trans->Val[mtransaVariables] != NULL) &&
      (Trans->Val[mtransaVariables][0] != '\0'))
    {
    MRsvSetAttr(R,mraVariables,Trans->Val[mtransaVariables],mdfString,mAdd);
    }

  if ((Trans->Val[mtransaVMUsage] != NULL) &&
      (Trans->Val[mtransaVMUsage][0] != '\0'))
    {
    MRsvSetAttr(R,mraVMUsagePolicy,Trans->Val[mtransaVMUsage],mdfString,mAdd);

    if ((R->VMUsagePolicy == mvmupVMCreate) ||
        (R->VMUsagePolicy == mvmupPMOnly))
      {
      MRsvSetAttr(R,mraFlags,(char *)MRsvFlags[mrfProvision],mdfString,mAdd);
      }
    }

  if ((Trans->Val[mtransaNodeFeatures] != NULL) &&
      (Trans->Val[mtransaNodeFeatures][0] != '\0'))
    {
    MRsvSetAttr(R,mraReqFeatureList,Trans->Val[mtransaNodeFeatures],mdfString,mAdd);
    }

  return(SUCCESS);
  }  /* END MTransApplyToRsv() */




/*
 * Free any unused transactions.
 *
 */

int MTransCheckStatus(void)

  {
  int tindex;

  for (tindex = 0;tindex < MMAX_TRANS;tindex++)
    {
    if (MTrans[tindex].ID == -1)
      {
      /* free spot */

      continue;
      }

    if (MTrans[tindex].ID == 0)
      {
      /* end of list located */

      break;
      }

    if (MTransIsValid(MTrans[tindex].ID,NULL) == TRUE)
      {
      continue;
      }

    /* free transaction's memory use */

    MTransFree(&MTrans[tindex]);
    }

  return(SUCCESS);
  }  /* END MTransCheckStatus() */




/**
 * Converts mtrans_t structure to corresponding XML.
 * 
 * @param T (I) required
 * @param EP (I/0) required (should be NULL, this routine will allocate memory)
 */

int MTransToXML(

  mtrans_t  *T,
  mxml_t   **EP)

  {
  mxml_t *E;

  int     aindex;

  if ((T == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  if (MXMLCreateE(EP,(char*)MXO[mxoxTrans]) == FAILURE)
    {
    return(FAILURE);
    }

  E = *EP;

  mstring_t Attr(MMAX_LINE);

  for (aindex = 1;aindex < mtransaLAST;aindex++)
    {
    MTransAToString(T,(enum MTransAttrEnum)aindex,&Attr,0);

    if (Attr.empty())
      continue;

    MXMLSetAttr(E,(char *)MTransAttr[aindex],(void **)Attr.c_str(),mdfString);

    Attr.clear(); /* reset string */
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MTransToXML() */



/**
 * Converts the specified attribute to a string.
 * 
 * @param T      (I) required
 * @param AIndex (I) required
 * @param Attr   (I) required, should be initialized.
 * @param Mode   (I) not used
 */

int MTransAToString(

  mtrans_t *T,
  enum MTransAttrEnum AIndex,
  mstring_t *Attr,
  int        Mode)

  {
  switch (AIndex)
    {
    case mtransaACL:
    case mtransaNodeList:
    case mtransaDuration:
    case mtransaStartTime:
    case mtransaFlags:
    case mtransaOpSys:
    case mtransaDRes:
    case mtransaRsvProfile:
    case mtransaLabel:
    case mtransaDependentTIDList:
    case mtransaMShowCmd:
    case mtransaCost:
    case mtransaVPCProfile:
    case mtransaVariables:
    case mtransaVMUsage:
    case mtransaNodeFeatures:
    case mtransaName:

      MStringAppend(Attr,T->Val[AIndex]);

      break;

    case mtransaRsvID:

      MStringAppend(Attr,T->RsvID);

      break;

    case mtransaIsValid:

      MStringAppend(Attr,MBool[MTransIsValid(T->ID,FALSE)]);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MTransAToString() */

  

/**
 * Loads a transaction from the checkpoint file.
 * 
 * @see MTransFromXML() - child
 * 
 * @param TP   (I) optional - pointer to transaction to populate, will populate global table if NULL
 * @param Line (I) required
 */

int MTransLoadCP(

  mtrans_t *TP,
  const char     *Line)

  {
  mtrans_t *T;

  char junk[MMAX_NAME + 1];
  char Name[MMAX_NAME + 1];

  const char *ptr;

  long CkTime;

  mxml_t *E = NULL;

  sscanf(Line,"%64s %64s %ld",
    junk,
    Name,
    &CkTime);
 
  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  if (TP == NULL)
    {
    int index;

    if (MTransFindEmptySlot(&index) == FAILURE)
      {
      return(FAILURE);
      }
 
    T = &MTrans[index];
    }
  else
    {
    T = TP;
    }

  if ((ptr = strchr(Line,'<')) == NULL)
    {
    return(FAILURE);
    }
 
  MXMLFromString(&E,ptr,NULL,NULL);

  MTransFromXML(T,E);

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MTransLoadCP() */



/**
 * Finds an empty slot in the global transaction table.
 *
 * @param Slot (I/O) required
 */

int MTransFindEmptySlot(

  int *Slot)

  {
  int tindex;

  if (Slot == NULL)
    {
    return(FAILURE);
    }

  *Slot = 0;

  for (tindex = 0;tindex < MMAX_TRANS;tindex++)
    {
    if ((MTrans[tindex].ID != 0) && 
       (((MSched.TransCounter + 1) - MTrans[tindex].ID) <= MMAX_TRANS)) /* the "+ 1" accounts for TIDs starting at 1 (not 0) */
      {
      /* slot is populated */

      continue;
      }
 
    *Slot = tindex;

    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MTransFindEmptySlot() */



/**
 * Builds a transaction from XML and inserts it into the global transaction table.
 *
 * NOTE: transaction is verified as complete/valid
 *
 * @param T (I) required
 * @param E (I) required
 */

int MTransFromXML(

  mtrans_t *T,
  mxml_t   *E)

  {
  int aindex;

  enum MTransAttrEnum tindex;

  if ((T == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    tindex = (enum MTransAttrEnum)MUGetIndexCI(E->AName[aindex],(const char **)MTransAttr,FALSE,mtransaNONE);

    if (tindex == mtransaNONE)
      {
      MDB(4,fCORE) MLog("WARNING:  cannot process unknown trans attribute '%s'",
          E->AName[aindex]);
      continue;
      }

    switch (tindex)
      {
      case mtransaACL:
      case mtransaNodeList:
      case mtransaDuration:
      case mtransaStartTime:
      case mtransaFlags:
      case mtransaOpSys:
      case mtransaDRes:
      case mtransaRsvProfile:
      case mtransaLabel:
      case mtransaDependentTIDList:
      case mtransaMShowCmd:
      case mtransaCost:
      case mtransaVPCProfile:
      case mtransaVariables:
      case mtransaVMUsage:
      case mtransaNodeFeatures:

        MUStrDup(&T->Val[tindex],E->AVal[aindex]);

        break;

      case mtransaName:

        MUStrDup(&T->Val[tindex],E->AVal[aindex]);

        T->ID = strtol(T->Val[tindex],NULL,10);

        break;

      case mtransaRsvID:
     
        MUStrDup(&T->RsvID,E->AVal[aindex]);

        break;

      default:

        /* NO-OP */

        break;
      }
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MTransFromXML() */





/**
 * Free transaction's memory.
 *
 */

int MTransFree(
 
  mtrans_t *T)

  {
  if ((T->ID == -1) || (T->ID == 0))
    {
    return(SUCCESS);
    }

  MUFree(&T->Requestor);
  MUFree(&T->Val[mtransaNodeList]);
  MUFree(&T->Val[mtransaDuration]);
  MUFree(&T->Val[mtransaStartTime]);
  MUFree(&T->Val[mtransaFlags]);
  MUFree(&T->Val[mtransaOpSys]);
  MUFree(&T->Val[mtransaDRes]);
  MUFree(&T->Val[mtransaRsvProfile]);
  MUFree(&T->Val[mtransaVPCProfile]);
  MUFree(&T->Val[mtransaLabel]);
  MUFree(&T->Val[mtransaDependentTIDList]);
  MUFree(&T->Val[mtransaMShowCmd]);
  MUFree(&T->Val[mtransaACL]);
  MUFree((char **)&T->NL);

  T->ID = -1;

  return(SUCCESS);
  }  /* END MTransFree() */




/**
 * Mark a transaction and its dependents as invalid.
 *
 * NOTE: sets IsInvalid boolean and removes the RsvID.
 *       called when a transaction moves from placeholder rsv to vpc.
 *
 * @param T (I) required
 */

int MTransInvalidate(

  mtrans_t *T)

  {
  char *ptr;
  char *TokPtr = NULL;

  char *Dependents = NULL;

  int TID;

  mtrans_t *tmpT = NULL;

  if (T == NULL)
    {
    return(FAILURE);
    }

  T->IsInvalid = TRUE;
  MUFree(&T->RsvID);

  if ((T->Val[mtransaDependentTIDList] == NULL) ||
      (T->Val[mtransaDependentTIDList][0] == '\0'))
    {
    return(SUCCESS);
    }

  MUStrDup(&Dependents,T->Val[mtransaDependentTIDList]);

  ptr = MUStrTok(Dependents,",",&TokPtr);

  while ((ptr != NULL) && (ptr[0] != '\0'))
    {
    TID = (int)strtol(ptr,NULL,10);

    if ((TID > 0) && 
        (MTransFind(TID,NULL,&tmpT) == SUCCESS))
      {
      tmpT->IsInvalid = TRUE;
      MUFree(&tmpT->RsvID);
      }

    ptr = MUStrTok(NULL,",",&TokPtr);
    }

  MUFree(&Dependents);

  return(SUCCESS);
  }  /* END MTransInvalidate() */

/* END MTrans.c */
