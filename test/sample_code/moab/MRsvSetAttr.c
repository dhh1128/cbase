/* HEADER */

      
/**
 * @file MRsvSetAttr.c
 *
 * Contains: MRsvSetAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Set specified reservation attribute.
 *
 * @see __MUIRsvCtlModify() - parent - allow external modification of rsv attribute
 * @see MRsvAToMString() - peer
 *
 * @param R (I) [modified]
 * @param AIndex (I)
 * @param AVal (I)
 * @param Format (I)
 * @param Mode (I) [mSet,mIncr,mAdd,mDecr,mClear]
 */

int MRsvSetAttr(

  mrsv_t                 *R,      /* I (modified) */
  enum MRsvAttrEnum       AIndex, /* I */
  void                   *AVal,   /* I */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I (mSet,mIncr,mAdd,mDecr,mClear) */

  {
  const char *FName = "MRsvSetAttr";

  MDB(6,fSTRUCT) MLog("%s(%s,%s,%s,%s,%d)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MRsvAttr[AIndex],
    (AVal != NULL) ? "AVal" : "NULL",
    MDFormat[Format],
    Mode);

  if (R == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mraACL:
    case mraCL:

      {
      macl_t **tmpACL;

      if (AIndex == mraACL)
        tmpACL = &R->ACL;
      else
        tmpACL = &R->CL;

      if ((AVal == NULL) ||
          (strcmp((char *)AVal,NONE) == 0))
        {
        /* clear */

        if ((Mode == mSet) || (Mode == mNONE2))
          MACLFreeList(tmpACL);

        break;
        }
      else if (Format != mdfString)
        {
        if ((Mode == mIncr) || (Mode == mAdd))
          {
          MACLMerge(tmpACL,(macl_t *)AVal);
          }
        else
          {
          MACLCopy(tmpACL,(macl_t *)AVal);
          }
        }  /* END else if (Format != mdfString) */
      else if (strcmp((char *)AVal,NONE))
        {
        if (((char *)AVal)[0] == '\0')
          {
          /* clear */

          if ((Mode == mSet) || (Mode == mNONE2))
            MACLFreeList(tmpACL);
          }
        else if ((Mode == mUnset) || (Mode == mDecr))
          {
          macl_t *tPtr = NULL;

          /* remove specified ACL's */

          char *tmpBuffer = NULL;

          MUStrDup(&tmpBuffer,(char *)AVal);

          MACLLoadConfigLine(&tPtr,tmpBuffer,mSet,NULL,FALSE);

          MUFree(&tmpBuffer);

          MACLSubtract(tmpACL,tPtr);

          MACLRemoveCredLock(tPtr,R->Name);
          }    /* END else if ((Mode == mUnset) || (Mode == mDecr)) */
        else
          {
          char *tmpBuffer = NULL;

          MUStrDup(&tmpBuffer,(char *)AVal);

          /* FORMAT:  <ACLTYPE>==<ACLVALUE>[,<ACLTYPE>==<ACLVALUE>]... */

          if ((Mode == mIncr) || (Mode == mAdd))
            {
            MACLLoadConfigLine(tmpACL,tmpBuffer,Mode,NULL,FALSE);
            }
          else
            {
            MACLFreeList(tmpACL);

            MACLLoadConfigLine(tmpACL,tmpBuffer,Mode,NULL,FALSE);

            MACLSet(tmpACL,maRsv,(void *)R->Name,mcmpSEQ,mnmNeutralAffinity,0,FALSE);
            }

          MUFree(&tmpBuffer);
          }
        }    /* END else if (strcmp((char *)AVal,NONE)) */
      }      /* END case mraCL and mraACL */

      break;

    case mraAAccount:

      if ((Format == mdfString) && (AVal != NULL))
        {
        if (strcmp((char *)AVal,NONE))
          {
          if (MAcctAdd((char *)AVal,&R->A) == FAILURE)
            {
            MDB(1,fSTRUCT) MLog("ALERT:    cannot add account %s for reservation %s\n",
              (char *)AVal,
              R->Name);

            return(FAILURE);
            }
          }
        }
      else
        {
        R->A = (mgcred_t *)AVal;
        }

      MACLSet(&R->CL,maAcct,R->A->Name,mcmpSEQ,0,0,FALSE);

      break;

    case mraAGroup:

      if ((Format == mdfString) && (AVal != NULL))
        {
        if (strcmp((char *)AVal,NONE))
          {
          if (MGroupAdd((char *)AVal,&R->G) == FAILURE)
            {
            MDB(1,fSTRUCT) MLog("ALERT:    cannot add group %s for reservation %s\n",
              (char *)AVal,
              R->Name);

            return(FAILURE);
            }
          }
        }
      else
        {
        R->G = (mgcred_t *)AVal;
        }

      MACLSet(&R->CL,maGroup,R->G->Name,mcmpSEQ,0,0,FALSE);

      break;

    case mraAQOS:

      if ((Format == mdfString) && (AVal != NULL))
        {
        if (strcmp((char *)AVal,NONE))
          {
          if (MQOSAdd((char *)AVal,&R->Q) == FAILURE)
            {
            MDB(1,fSTRUCT) MLog("ALERT:    cannot add qos %s for reservation %s\n",
              (char *)AVal,
              R->Name);

            return(FAILURE);
            }
          }
        }
      else
        {
        R->Q = (mqos_t *)AVal;
        }

      MACLSet(&R->CL,maQOS,R->Q->Name,mcmpSEQ,0,0,FALSE);

      break;

    case mraAUser:

      if ((Format == mdfString) && (AVal != NULL))
        {
        if (strcmp((char *)AVal,NONE))
          {
          if (MUserAdd((char *)AVal,&R->U) == FAILURE)
            {
            MDB(1,fSTRUCT) MLog("ALERT:    cannot add user %s for reservation %s\n",
              (char *)AVal,
              R->Name);

            return(FAILURE);
            }
          }
        }
      else
        {
        R->U = (mgcred_t *)AVal;
        }

      MACLSet(&R->CL,maUser,R->U->Name,mcmpSEQ,0,0,FALSE);

      break;

    case mraAllocNodeCount:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->AllocNC = strtol((char *)AVal,NULL,10);
      else
        R->AllocNC = *(long *)AVal;

      break;

    case mraAllocProcCount:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->AllocPC = strtol((char *)AVal,NULL,10);
      else
        R->AllocPC = *(long *)AVal;

      break;

    case mraAllocTaskCount:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->AllocTC = strtol((char *)AVal,NULL,10);
      else
        R->AllocTC = *(long *)AVal;

      break;

    case mraComment:

      if (AVal == NULL)
        {
        MUFree(&R->Comment);
        }
      else
        {
        MUStrDup(&R->Comment,(char *)AVal);
        }

      break;

    case mraCost:

      if (AVal == NULL)
        return(FAILURE);

      R->LienCost = strtod((char *)AVal,NULL);

      break;

    case mraCTime:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        R->CIteration = MSched.Iteration;

        R->CTime = strtol((char *)AVal,NULL,10);
        }

      break;

    case mraExcludeRsv:

      {
      int   index;
      char *TokPtr;
      char *ptr;

      /* clear old values */

      if (R->RsvExcludeList != NULL)
        {
        for (index = 0;index < MMAX_PJOB;index++)
          {
          if (R->RsvExcludeList[index] == NULL)
            break;

          MUFree(&R->RsvExcludeList[index]);
          }  /* END for (index) */
        }

      if ((AVal == NULL) ||
          (((char *)AVal)[0] == '\0') ||
          !strcmp((char *)AVal,NONE))
        {
        /* empty list specified */

        return(FAILURE);
        }

      if (R->RsvExcludeList == NULL)
        R->RsvExcludeList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

      ptr = MUStrTok((char *)AVal,", \t\n",&TokPtr);

      index = 0;

      while ((ptr != NULL) && (ptr[0] != '\0'))
        {
        MUStrDup(&R->RsvExcludeList[index],ptr);

        index++;

        if (index >= MMAX_PJOB)
          break;

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mraExcludeRsv) */

      break;

    case mraExpireTime:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        long tmpL;

        tmpL = MUTimeFromString((char *)AVal);

        if (tmpL > MCONST_EFFINF)
          R->ExpireTime = tmpL;
        else
          R->ExpireTime = MSched.Time + tmpL;
        }

      break;

    case mraFlags:

      {
      mbitmap_t tmpL;

      /* WARNING:  may overwrite flags set thru alternate methods */
      /*           only supports mdfOther, mdfString */

      if (AVal == NULL)
        {
        bmclear(&tmpL);
        }
      else if (Format == mdfString)
        {
        bmfromstring((char *)AVal,MRsvFlags,&tmpL);
        }
      else if (Format == mdfOther)
        {
        bmcopy(&tmpL,(mbitmap_t *)AVal);
        }
      else
        {
        /* format not supported */

        return(FAILURE);
        }

      if ((Mode == mAdd) || (Mode == mIncr))
        {
        bmor(&R->Flags,&tmpL);
        }
      else if ((Mode == mDecr) || (Mode == mUnset))
        {
        int index;

        for (index = 0;index < mrfLAST;index++)
          {
          if (bmisset(&tmpL,index))
            bmunset(&R->Flags,index);
          }  /* END for (index) */
        }
      else   /* default is set */
        {
        bmcopy(&R->Flags,&tmpL);
        }
      }  /* END BLOCK (case mraFlags) */

      break;

    case mraGlobalID:

      if (AVal == NULL)
        {
        MUFree(&R->SystemRID);
        }
      else
        {
        MUStrDup(&R->SystemRID,(char *)AVal);
        }

      break;

    case mraHostExp:

      if ((AVal == NULL) ||
          (((char *)AVal)[0] == '\0') ||
          !strcmp((char *)AVal,NONE))
        {
        MUFree(&R->HostExp);

        break;
        }

      if (R->HostExp != NULL)
        {
        MUFree(&R->HostExp);
        }

      if ((R->HostExp == NULL) &&
         ((R->HostExp = (char *)MUCalloc(1,strlen((char *)AVal) + 1)) == NULL))
        {
        break;
        }

      MUStrCpy(R->HostExp,(char *)AVal,strlen((char *)AVal) + 1);

      break;

    case mraHostExpIsSpecified:

      if (AVal == NULL)
        return(FAILURE);

      R->HostExpIsSpecified = MUBoolFromString((char *)AVal,MBNOTSET);

      break;

    case mraIsTracked:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->IsTracked = MUBoolFromString((char *)AVal,FALSE);

      break;

    case mraLabel:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        if (R->Label != NULL)
          MUFree(&R->Label);

        MUStrDup(&R->Label,(char *)AVal);
        }

      break;

    case mraLastChargeTime:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        R->LastChargeTime = strtol((char *)AVal,NULL,10);
        }

      break;

    case mraLogLevel:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        R->LogLevel = (int)strtol((char *)AVal,NULL,10);
        }
      else
        {
        R->LogLevel = *(int *)AVal;
        }

      break;

    case mraMaxJob:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        R->MaxJob = (int)strtol((char *)AVal,NULL,10);
        }
      else
        {
        R->MaxJob = *(int *)AVal;
        }

      break;

    case mraMessages:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfOther)
        {
        mmb_t *MP = (mmb_t *)AVal;

        MMBAdd(&R->MB,MP->Data,MP->Owner,MP->Type,MP->ExpireTime,MP->Priority,NULL);
        }

      break;

    case mraName:

      /* NOTE:  verify no whitespace in rsv name */

      {
      int   namelen;
      char *name;

      int aindex;

      if (AVal == NULL)
        return(FAILURE);

      name = (char *)AVal;
      namelen = strlen(name);

      for (aindex = 0;aindex < namelen;aindex++)
        {
        if (!isprint(name[aindex]) || isspace(name[aindex]))
          {
          return(FAILURE);
          }
        }

      MUStrCpy(R->Name,(char *)AVal,sizeof(R->Name));
      }  /* END BLOCK */

      break;

    case mraReqArch:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfInt)
        {
        R->ReqArch = *(int *)AVal;
        }
      else
        {
        R->ReqArch = MUMAGetIndex(meArch,(char *)AVal,mAdd);
        }

      break;

    case mraReqMemory:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfInt)
        {
        R->ReqMemory = *(int *)AVal;
        }
      else
        {
        R->ReqMemory = (int)strtol((char *)AVal,NULL,10);
        }

      break;

    case mraReqNodeList:

      if (Format == mdfString)
        {
        if (MNLFromString(
             (char *)AVal,
             &R->ReqNL,
             -1,
             TRUE) == FAILURE)
          {
          return(FAILURE);
          }
        }
      else if (Format == mdfOther)
        {
        int index = 0;

        mnode_t *tmpN;

        mnl_t *NL = (mnl_t *)AVal;
        mnl_t *RNL;

        RNL = &R->ReqNL;

        while (MNLGetNodeAtIndex(NL,index,&tmpN) == SUCCESS)
          {
          MNLSetNodeAtIndex(RNL,index,tmpN);
          MNLSetTCAtIndex(RNL,index,MNLGetTCAtIndex(NL,index));

          index++;
          }

        MNLTerminateAtIndex(RNL,index);
        }

      break;

    case mraReqNodeCount:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->ReqNC = (int)strtol((char *)AVal,NULL,10);
      else
        R->ReqNC = *(int *)AVal;

      break;

    case mraReqTaskCount:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        if (strcasecmp((char *)AVal,"all"))
          {
          char *ptr;

          if (strchr((char *)AVal,'-') != NULL) /* check for range */
            {
            R->MinReqTC = (int)strtol((char *)AVal,&ptr,10);

            if (R->MinReqTC > 0)
              {
              R->ReqTC = (int)strtol((char *)&ptr[1],NULL,10);

              if (R->ReqTC < R->MinReqTC)
                R->ReqTC = 0;
              }
            }
          else
            {
            R->ReqTC = (int)strtol((char *)AVal,NULL,10);
            }
          }
        else
          {
          /* AVal is ALL */

          R->HostExpIsSpecified = TRUE;

          MRsvSetAttr(R,mraHostExp,AVal,mdfString,mSet);
          }
        }
      else
        {
        R->ReqTC = *(int *)AVal;
        }

      break;

    case mraReqTPN:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        R->ReqTPN = (int)strtol((char *)AVal,NULL,10);
        }
      else
        {
        R->ReqTPN = *(int *)AVal;
        }

      break;

    case mraAllocNodeList:

      if (Format != mdfString)
        {
        int         NIndex;
        int         nindex;

        int         NCount;

        mnode_t    *N;
        mnode_t    *NPtr;

        mnl_t *NL;

        if (AVal == NULL)
          {
          return(FAILURE);
          }

        /* assume ordered list */

        NL = (mnl_t *)AVal;

        NIndex = 0;

        if (MNLIsEmpty(NL))
          {
          NPtr   = NULL;
          NCount = 0;
          }
        else
          {
          MNLGetNodeAtIndex(NL,0,&NPtr);
          NCount = MNLGetTCAtIndex(NL,0);
          }

        /* locate nodes */

        for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
          {
          N = MNode[nindex];

          if ((N == NULL) || (N->Name[0] == '\0'))
            break;

          if (N->Name[0] == '\1')
            continue;

          switch (Mode)
            {
            case mDecr:

              if (NPtr == N)
                MRsvAdjustNode(R,N,NCount,-1,FALSE);

              break;

            case mSet:

              if (NPtr == N)
                MRsvAdjustNode(R,N,NCount,0,FALSE);
              else
                MRsvAdjustNode(R,N,0,0,FALSE);

              break;

            case mIncr:

              if (NPtr == N)
                MRsvAdjustNode(R,N,NCount,1,FALSE);

              break;

            default:

              /* NOT HANDLED */

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }  /* END switch (Mode) */

          if (N == NPtr)
            {
            NIndex++;

            if (MNLGetNodeAtIndex(NL,NIndex,NULL) == FAILURE)
              break;

            MNLGetNodeAtIndex(NL,NIndex,&NPtr);
            NCount = MNLGetTCAtIndex(NL,NIndex);
            }
          }    /* END for (nindex) */
        }      /* END if (Format != mdfString) */
      else
        {
        MNLFromString((char *)AVal,&R->ReqNL,-1,TRUE);
        }  /* END else if (Format != mdfString) */

      break;

    case mraNodeSetPolicy:

      if (AVal == NULL)
        return(FAILURE);

      MUStrDup(&R->NodeSetPolicy,(char *)AVal);

      break;

    case mraOwner:

      if (Format == mdfString)
        {
        char tmpLine[MMAX_LINE];

        enum MXMLOTypeEnum oindex;

        char *TokPtr;
        char *ptr;

        void *optr;

        /* FORMAT:  <CREDTYPE>:<CREDID> */

        MUStrCpy(tmpLine,(char *)AVal,sizeof(tmpLine));

        ptr = MUStrTok(tmpLine,": \t\n",&TokPtr);

        if ((ptr == NULL) ||
           ((oindex = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXOC,FALSE,mxoNONE)) == mxoNONE))
          {
          /* invalid format */

          return(FAILURE);
          }

        ptr = MUStrTok(NULL,": \t\n",&TokPtr);

        if (MOGetObject(oindex,ptr,&optr,mAdd) == FAILURE)
          {
          return(FAILURE);
          }

        R->O     = optr;
        R->OType = oindex;
        }
      else
        {
        /* NYI */

        return(FAILURE);
        }

      break;

    case mraPartition:

      {
      mpar_t *P;

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        if (!strcmp((char *)AVal,"ALL"))
          {
          R->PtIndex = -1;
          }
        else
          {
          if (MParFind((char *)AVal,&P) == FAILURE)
            {
            return(FAILURE);
            }

          R->PtIndex = P->Index;
          }
        }    /* END if (Format == mdfString) */
      else
        {
        R->PtIndex = ((mpar_t *)AVal)->Index;
        }
      }    /* END BLOCK mraPartition */

      break;

    case mraPriority:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->Priority = strtol((char *)AVal,NULL,10);
      else
        R->Priority = *(long *)AVal;

      break;

    case mraProfile:

      MRsvProfileFind((char *)AVal,&R->P);

      break;

    case mraReqFeatureList:

      if (Format == mdfOther)
        {
        bmcopy(&R->ReqFBM,(mbitmap_t *)AVal);
        }
      else
        {
        /* FORMAT:  <FEATURE>[{,:|}<FEATURE>]... */

        if ((AVal != NULL) &&
            (strchr((char *)AVal,'|') != NULL))
          {
          int tmpI;

          tmpI = (int)mclOR;

          MRsvSetAttr(
            R,
            mraReqFeatureMode,
            (void *)&tmpI,
            mdfInt,
            mSet);
          }

        /* extract from MUMAToString */

        if (MUNodeFeaturesFromString((char *)AVal,(Mode == mVerify) ? mVerify : mAdd,&R->ReqFBM) == FAILURE)
          {
          return(FAILURE);
          }
        }  /* END else (Format == mdfOther) */

      break;

    case mraReqFeatureMode:

      if ((Format == mdfInt) && (AVal != NULL))
        {
        int tmpI;

        tmpI = *(int *)AVal;

        R->ReqFMode = (enum MCLogicEnum)tmpI;
        }
      else
        {
        R->ReqFMode = (enum MCLogicEnum)MUGetIndex((char *)AVal,MCLogic,FALSE,0);
        }

      break;

    case mraReqOS:

      if ((Format == mdfInt) && (AVal != NULL))
        {
        R->ReqOS = *(int *)AVal;
        }
      else
        {
        R->ReqOS = MUMAGetIndex(meOpsys,(char *)AVal,mAdd);
        }

      break;

    case mraResources:

      /* if TID specified, may set both HostExp and rsv NL */

      if (((Format == mdfString) || (Format == mdfInt)) &&
          (AVal != NULL))
        {
        /* FORMAT:  {<TID>|<CRESSPEC>} */

        if (isdigit(((char *)AVal)[0]) || (Format == mdfInt))
          {
          /* TID detected */

          int   TID;

          mtrans_t *Trans = NULL;

          if (Format == mdfInt)
            TID = *(int *)AVal;
          else
            TID = (int)strtol((char *)AVal,NULL,0);

          if (MTransFind(
                TID,
                NULL,
                &Trans) == SUCCESS)
            {
            MTransApplyToRsv(Trans,R);
            }      /* END if (MTransFind()) */
          }        /* END if (isdigit(((char *)AVal)[0]) || (Format == mdfInt)) */
        else
          {
          char *ptr = NULL;

          /* FORMAT:  {<CRESSPEC>} */

          /* MCResFromString() does not initialize DRes */

          MCResClear(&R->DRes);

          /* by default allocate all procs */

          R->DRes.Procs = -1;

          MUStrDup(&ptr,(char *)AVal);

          if (MCResFromString(&R->DRes,ptr) == FAILURE)
            {
            MUFree(&ptr);

            return(FAILURE);
            }

          MUFree(&ptr);
          }
        }  /* END if ((Format == mdfString) || (Format == mdfInt)) */
      else
        {
        MCResCopy(&R->DRes,(mcres_t *)AVal);
        }

      break;

    case mraRsvAccessList:

      {
      int   index;
      char *TokPtr;
      char *ptr;

      /* clear old values */

      if (R->RsvAList != NULL)
        {
        for (index = 0;index < MMAX_PJOB;index++)
          {
          if (R->RsvAList[index] == NULL)
            break;

          MUFree(&R->RsvAList[index]);
          }  /* END for (index) */
        }

      if ((AVal == NULL) ||
          (((char *)AVal)[0] == '\0') ||
          !strcmp((char *)AVal,NONE))
        {
        /* empty list specified */

        break;
        }

      if (R->RsvAList == NULL)
        R->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

      ptr = MUStrTok((char *)AVal,", \t\n",&TokPtr);

      index = 0;

      while ((ptr != NULL) && (ptr[0] != '\0'))
        {
        MUStrDup(&R->RsvAList[index],ptr);

        index++;

        if (index >= MMAX_PJOB)
          break;

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mraRsvAccessList) */

      break;

    case mraRsvGroup:

      if ((AVal == NULL) ||
          (((char *)AVal)[0] == '\0') ||
          !strcmp((char *)AVal,NONE))
        {
        MUFree(&R->RsvGroup);

        break;
        }

      MUStrDup(&R->RsvGroup,(char *)AVal);

      break;

    case mraRsvParent:

      if ((AVal == NULL) ||
          (((char *)AVal)[0] == '\0') ||
          !strcmp((char *)AVal,NONE))
        {
        MUFree(&R->RsvParent);

        break;
        }

      MUStrDup(&R->RsvParent,(char *)AVal);

      break;

    case mraSID:

      MUStrDup(&R->SystemID,(char *)AVal);

      break;

    case mraSpecName:

      {
      int   namelen;
      char *name;

      int aindex;

      if (AVal == NULL)
        return(FAILURE);

      name = (char *)AVal;
      namelen = strlen(name);

      for (aindex = 0;aindex < namelen;aindex++)
        {
        if (!isprint(name[aindex]) || isspace(name[aindex]))
          {
          return(FAILURE);
          }
        }

      MUStrDup(&R->SpecName,(char *)AVal);
      }  /* END BLOCK */

      break;

    case mraEndTime:
    case mraStartTime:
    case mraDuration:

      {
      long tmpL = 0;

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        if (!strcasecmp((char *)AVal,"INFINITY"))
          tmpL = MMAX_TIME - 1;
        else
          tmpL = strtol((char *)AVal,NULL,10);
        }
      else
        {
        tmpL = *((long *)AVal);
        }

      MRsvSetTimeframe(
        R,
        AIndex,
        Mode,
        tmpL,
        NULL);

      MRsvAdjustTimeframe(R);
      }

      break;

    case mraStatCAPS:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->CAPS = strtod((char *)AVal,NULL);
      else
        R->CAPS = *(double *)AVal;

      break;

    case mraStatCIPS:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->CIPS = strtod((char *)AVal,NULL);
      else
        R->CIPS = *(double *)AVal;

      break;

    case mraStatTAPS:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->TAPS = strtod((char *)AVal,NULL);
      else
        R->TAPS = *(double *)AVal;

      break;

    case mraStatTIPS:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        R->TIPS = strtod((char *)AVal,NULL);
      else
        R->TIPS = *(double *)AVal;

      break;

    case mraSysJobID:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        MUStrDup(&R->SysJobID,(char *)AVal);

      break;

    case mraSysJobTemplate:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        MUStrDup(&R->SysJobTemplate,(char *)AVal);

      break;

    case mraTrigger:

      /* load trigger list */

      return(MTrigLoadString(&R->T,(char *)AVal,TRUE,FALSE,mxoRsv,R->Name,NULL,NULL));

      break;

    case mraType:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        if (isdigit(((char *)AVal)[0]))
          R->Type = (enum MRsvTypeEnum)strtol((char *)AVal,NULL,0);
        else
          R->Type = (enum MRsvTypeEnum)MUGetIndex((char *)AVal,MRsvType,FALSE,mrtNONE);
        }
      else
        {
        int tmpI;

        tmpI = *(int *)AVal;

        R->Type = (enum MRsvTypeEnum)tmpI;
        }

      break;

    case mraSubType:

      if (AVal == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        if (isdigit(((char *)AVal)[0]))
          {
          R->SubType = (enum MRsvSubTypeEnum)strtol((char *)AVal,NULL,0);
          }
        else
          {
          R->SubType = (enum MRsvSubTypeEnum)MUGetIndexCI((char *)AVal,MRsvSubType,FALSE,mrsvstNONE);

          if (R->SubType == mrsvstNONE)
            {
            /* subtype does not match full name - try shortcut */

            R->SubType = (enum MRsvSubTypeEnum)MUGetIndexCI((char *)AVal,MRsvSSubType,FALSE,mrsvstNONE);
            }
          }
        }
      else
        {
        int tmpI;

        tmpI = *(int *)AVal;

        R->SubType = (enum MRsvSubTypeEnum)tmpI;
        }

      break;

    case mraVariables:

      /* FORMAT:  <name>[=<value>][[;<name[=<value]]...] */

      /* NOTE:  only supports mSet and mIncr (add support for mDecr - NYI) */

      {
      mln_t *tmpL;

      char *ptr;
      char *TokPtr;

      char *TokPtr2 = NULL;
      char tmpVal[MMAX_BUFFER << 3];

      if (Mode == mSet)
        {
        /* clear initial data */

        MULLFree(&R->Variables,MUFREE);
        }

      /* add specified data */

      MUStrCpy(tmpVal,(char *)AVal,sizeof(tmpVal));

      ptr = MUStrTok((char *)tmpVal,";",&TokPtr);

      while (ptr != NULL)
        {
        MUStrTok(ptr,"=",&TokPtr2);

        /* if mSet or mAdd, add the new variable to the list */

        if  ((Mode == mSet) || (Mode == mAdd) || (Mode == mSetOnEmpty))
          {
          MULLAdd(&R->Variables,ptr,NULL,&tmpL,MUFREE);

          if (TokPtr2[0] != '\0')
            {
            MUStrDup((char **)&tmpL->Ptr,TokPtr2);
            }
          }

        /* if mDecr, remove the given variable from the list */

        if (Mode == mDecr)
          {
          MULLRemove(&R->Variables,ptr,NULL);
          }

        ptr = MUStrTok(NULL,";",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mraVariables) */

      break;

    case mraVMUsagePolicy:

      if (Format == mdfString)
        {
        R->VMUsagePolicy = (enum MVMUsagePolicyEnum)MUGetIndexCI(
          (char *)AVal,
          MVMUsagePolicy,
          FALSE,
          0);

        if (R->VMUsagePolicy == mvmupNONE)
          {
          return(FAILURE);
          }

        switch (R->VMUsagePolicy)
          {
          case mvmupPMOnly:

            if ((R->VMUsagePolicy == mvmupPMOnly) && 
                (R->VMTaskMap != NULL))
              {
              /* other code assumes R wants VMs if VMTaskMap is allocated */

              MUFree((char **)&R->VMTaskMap);
              }

            break;

          case mvmupAny:
          case mvmupPMPref:
          case mvmupVMCreate:
          case mvmupVMOnly:
          case mvmupVMPref:

            /* NO-OP */

            break;

          default:

            assert(FALSE);

            break;
          }  /* END switch (R->VMUsagePolicy) */
        }    /* END if (Format == mdfString) */

      break;

    default:

      /* not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MRsvSetAttr() */
/* END MRsvSetAttr.c */
