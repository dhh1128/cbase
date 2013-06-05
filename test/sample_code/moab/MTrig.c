/* HEADER */

/**
 * @file MTrig.c
 *
 * Moab Triggers
 */
        
#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "MEventLog.h"



/**
 * Updates a trigger based on its state.
 *
 * @param T (I) [modified]
 */

int MTrigRegisterEvent(

  mtrig_t *T)  /* I (modified) */

  {
  if ((T->State == mtsFailure) && (T->RetryCount < T->MaxRetryCount))
    {
    mulong ETime = T->ETime;

    MTrigReset(T);

    /* FIXME: should ETime be the original? what about T->Offset and T->RearmTime? */

    T->ETime = ETime;

    T->RetryCount++;

    return(SUCCESS);
    }

  if (T->O != NULL)
    {
    switch (T->OType)
      {
      case mxoRsv:
      case mxoJob:
      case mxoNode:
      case mxoRM:
      case mxoQOS:
      case mxoUser:
      case mxoClass:
      case mxoGroup:
      case mxoAcct:

        {
        if (T->State == mtsSuccessful)
          {
          MTrigSetVars(T,FALSE,NULL);
          MTrigProbe(T);
          MTrigUnsetVars(T,FALSE,NULL);

          /* record state in journal to mark that this trigger is completely done */

          MTrigSetState(T,T->State);  /* use same state so that we record it in journal */

#ifdef MYAHOO3
          /* notify dependents of successful completion */
          /* MTrigNotifyDependents(T); */

          if (T->Dependents != NULL)
            {
            mln_t *LLPtr;
            mtrig_t *tmpTrig;

            LLPtr = T->Dependents;

            while (LLPtr != NULL)
              {
              if (MTrigFind(LLPtr->Name,&tmpTrig) == SUCCESS)
                {
                MULLRemove(&tmpTrig->TrigDeps,T->Name,NULL);

                if ((tmpTrig->TrigDeps == NULL) ||
                    (tmpTrig->TrigDeps->ListSize == 0))
                  {
                  tmpTrig->OutstandingTrigDeps = FALSE;
                  }
                }

              LLPtr = LLPtr->Next;
              }
            }
#endif /* MYAHOO3 */

          MOWriteEvent((void *)T,mxoTrig,mrelTrigEnd,NULL,NULL,NULL);

          CreateAndSendEventLog(meltTrigEnd,T->TName,mxoTrig,(void *)T);
          }   /* END if (T->State == mtsSuccessful) */
        else if (MTRIGISRUNNING(T))
          {
          if (T->OBuf != NULL)
            MTrigProbe(T);
          }
        else
          {
          /* trigger failed */

          MOWriteEvent((void *)T,mxoTrig,mrelTrigFailure,NULL,NULL,NULL);

          MTrigSetVars(T,TRUE,NULL);
          MTrigUnsetVars(T,TRUE,NULL);

          if (bmisset(&T->SFlags,mtfAttachError))
            {
            char *EBuf;
            
            /* read in stderr and attach as message to object */

            switch (T->OType)
              {
              case mxoJob:

                {
                mjob_t *tmpCJ = NULL;

                mjob_t *J = (mjob_t *)T->O;

                if ((EBuf = MFULoadNoCache(T->EBuf,1,NULL,NULL,NULL,NULL)) == NULL)
                  {
                  MDB(3,fSCHED) MLog("INFO:     cannot load error data for trigger %s (File: %s)\n",
                    T->TName,
                    (T->EBuf == NULL) ? "NULL" : T->EBuf);
                  }

                if (EBuf != NULL)
                  {
                  if (MJOBISCOMPLETE(J) &&
                      (MJobCFind(J->Name,&tmpCJ,mjsmBasic) == SUCCESS))
                    {
                    /* In the case of a jobepilogue, this is being called after
                     * the job has already been added to the completed job
                     * table and is being destroyed from MJobDestroy. */

                    MJobSetAttr(tmpCJ,mjaMessages,(void **)EBuf,mdfString,mAdd);

                    J = tmpCJ; /* so T->Msg will be set on the completed job */
                    }
                  else
                    {
                    MJobSetAttr(J,mjaMessages,(void **)EBuf,mdfString,mAdd);
                    }
                  }

                if (T->Msg != NULL)
                  MJobSetAttr(J,mjaMessages,(void **)T->Msg,mdfString,mAdd);

                MUFree(&EBuf);
                }  /* END case mxoJob */

                break;

              case mxoNode:

                {
                mnode_t *N = (mnode_t *)T->O;

                if ((EBuf = MFULoadNoCache(T->EBuf,1,NULL,NULL,NULL,NULL)) == NULL)
                  {
                  MDB(3,fSCHED) MLog("INFO:     cannot load error data for trigger %s (File: %s)\n",
                    T->TName,
                    (T->EBuf == NULL) ? "NULL" : T->EBuf);
                  }

                if (EBuf != NULL)
                  MNodeSetAttr(N,mnaMessages,(void **)EBuf,mdfString,mAdd);

                if (T->Msg != NULL)
                  MNodeSetAttr(N,mnaMessages,(void **)T->Msg,mdfString,mAdd);

                MUFree(&EBuf);
                }  /* END case mxoNode */

                break;

              default:

                /* NO-OP */

                break;
              }
            }
          }  /* END else (T->State == mtsSuccessful) */
        }    /* END case mxoRSV */

        break;

      default:

        if (T->State == mtsSuccessful)
          {
          MOWriteEvent((void *)T,mxoTrig,mrelTrigEnd,NULL,NULL,NULL);
          CreateAndSendEventLog(meltTrigEnd,T->TName,mxoTrig,(void *)T);
          }
        else if (T->State == mtsFailure)
          {
          MOWriteEvent((void *)T,mxoTrig,mrelTrigFailure,NULL,NULL,NULL);

          if (MSched.PushEventsToWebService == TRUE)
            {
            MEventLog *Log = new MEventLog(meltTrigEnd);
            Log->SetPrimaryObject(T->TName,mxoTrig,(void *)T);
            Log->SetStatus("failure");

            MEventLogExportToWebServices(Log);

            delete Log;
            }
          }

        break;
      }  /* END switch (T->OType) */
    }    /* END if (T->O != NULL) */

  return(SUCCESS);
  }  /* END MTrigRegisterEvent() */




/**
 * Verify trigger is properly populated/configured
 *
 * @param T (I)
 */

int MTrigValidate(

  mtrig_t *T)  /* I */

  {
  const char *FName = "MTrigValidate";

  MDB(3,fRM) MLog("%s(T)\n",
    FName);

  if (T == NULL)
    {
    return(FAILURE);
    }

  if (T->AType == mtatNONE)
    {
    return(FAILURE);
    }

  if (T->EType == mttNONE)
    {
    return(FAILURE);
    }

  /* requires an action fro all types except cancel, preempt, and reserve */

  if ((T->Action == NULL) &&
      (T->AType != mtatCancel) &&
      (T->AType != mtatJobPreempt) &&
      (T->AType != mtatReserve))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MTrigValidate() */





/**
 * Perform trigger modify action.
 *
 * @see MTrigInitiateAction() - parent
 *
 * @param T (I)
 * @param ModString (I) [modified]
 * @param EMsg (I) [optional minsize==MMAX_LINE]
 */

int MTrigDoModifyAction(

  mtrig_t *T,         /* I */
  char    *ModString, /* I (modified) */
  char    *EMsg)      /* I (optional minsize==MMAX_LINE) */

  {
  void *O = NULL;
  enum MXMLOTypeEnum OType;

  char *OID = NULL;

  char *Attr;

  char *Value;

  char *ptr;
  char *TokPtr = NULL;

  int   AIndex;

  char  tmpEMsg[MMAX_LINE];

  enum MStatusCodeEnum SC;

  mbool_t OListSpecified = FALSE;

  const char *FName = "MTrigDoModifyAction";

  MDB(3,fRM) MLog("%s(%s,%s)\n",
    FName,
    (T != NULL) ? T->TName : "NULL",
    (ModString != NULL) ? ModString : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (T == NULL)
    {
    return(FAILURE);
    }

  if ((ModString == NULL) || (ModString[0] == '\0'))
    {
    return(SUCCESS);
    }

  if (strchr(ModString,'=') == NULL)
    {
    /* FORMAT:  {set|incr|decr} <OTYPE> <OID> <ATTR> <VALUE> */

    if ((ptr = MUStrTok(ModString," \t\n",&TokPtr)) == NULL)
      {
      return(FAILURE);
      }

    if ((ptr = MUStrTok(NULL," \t\n",&TokPtr)) == NULL)
      {
      return(FAILURE);
      }

    OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mSet);

    if ((OID = MUStrTok(NULL," \t\n",&TokPtr)) == NULL)
      {
      return(FAILURE);
      }

    if (strchr(OID,',') != NULL)
      OListSpecified = TRUE;

    if ((Attr = MUStrTok(NULL," \t\n",&TokPtr)) == NULL)
      {
      return(FAILURE);
      }

    Value = TokPtr;

    if (OListSpecified == FALSE)
      {
      if (MOGetObject(OType,OID,&O,mVerify) == FAILURE)
        {
        /* unable to locate object */
 
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"ERROR:  unable to locate %s:%s",
            MXO[OType],
            OID);
          } 
 
        return(FAILURE);
        }
      }
    else
      {
      O = NULL;
      }
    }
  else
    {
    /* FORMAT: <ATTR>=<VALUE> */

    OType = T->OType;

    OID   = T->OID;

    Attr = MUStrTok(ModString,"=",&TokPtr);

    Value = TokPtr;

    if (Attr == NULL)
      {
      return(FAILURE);
      }
    }

  switch (OType) 
    {
    case mxoNode:

      /* only supports OS and NodeState for now */

      AIndex = MUGetIndexCI(Attr,MNodeAttr,FALSE,mnaNONE);

      if ((AIndex != mnaOS) && (AIndex != mnaNodeState))
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"WARNING:  cannot modify node attribute %s",
            MNodeAttr[AIndex]);
          }

        return(FAILURE);
        }

      if (MNodeModify(
          (mnode_t *)O,
          (OListSpecified == TRUE) ? OID : NULL, 
          (enum MNodeAttrEnum)AIndex,
          Value,
          tmpEMsg,
          &SC) == FAILURE)
        {
        MDB(2,fRM) MLog("WARNING:  cannot modify node %s, %s\n",
          OID,
          tmpEMsg);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"WARNING:  cannot modify node %s, %s",
            OID,
            tmpEMsg);
          }

        return(FAILURE);
        }

      break;

    case mxoRsv:

      AIndex = MUGetIndexCI(Attr,MRsvAttr,FALSE,mraNONE);

      if (MRsvSetAttr(
            (mrsv_t *)T->O,
            (enum MRsvAttrEnum)AIndex,
            Value,
            mdfString,
            mSet) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mxoxVPC:

      AIndex = MUGetIndexCI(Attr,MParAttr,FALSE,mpaNONE);

      if (MParSetAttr(
            (mpar_t *)T->O,
            (enum MParAttrEnum)AIndex,
            (void **)Value,
            mdfString,
            mSet) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    default:

      /* object modification not supported */

      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"ERROR:  object modification not supported");

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (OType) */

  return(SUCCESS);
  }  /* END MTrigDoModifyAction() */





/**
 *
 *
 * @param T (I)
 * @param Attr (I)
 * @param DoGlobal (I)
 */

int MTrigQuery(

  mtrig_t *T,        /* I */
  char    *Attr,     /* I */
  mbool_t  DoGlobal) /* I */

  {
  if (T == NULL)
    {
    return(FAILURE);
    }

  switch (T->OType)
    {
    case mxoRsv:
      {
      mrsv_t *R;
      mrsv_t *tmpR;

      char *ptr;
      char *TokPtr = NULL;

      char *tmpVar;

      char *BPtr = NULL;
      int   BSpace = 0;

      char *Label;

      char  AllHostList[MMAX_BUFFER];
      char  tmpBuf[MMAX_BUFFER];

      char  tmpName[MMAX_NAME];
      char  Action[MMAX_NAME];

      marray_t  RList;

      int rindex;

      enum MRsvAttrEnum AIndex;

      R = (mrsv_t *)T->O;

      if (R == NULL)
        {
        return(FAILURE);
        }

      MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

      MRsvGroupGetList(R->RsvGroup,&RList,NULL,0);
      
      if (RList.NumItems <= 0)
        {
        T->State = mtsSuccessful;

        MUArrayListFree(&RList);

        return(SUCCESS);
        }

      MUStrCpy(Action,Attr,sizeof(Action));

      MUSNInit(&BPtr,&BSpace,AllHostList,sizeof(AllHostList));

      /* FORMAT:  <ATTR>=<VALUE> */

      Label = MUStrTok(Action,":",&TokPtr);

      ptr = MUStrTok(NULL," =\n\t",&TokPtr);

      tmpVar = MUStrTok(NULL," \n\t",&TokPtr);

      AIndex = (enum MRsvAttrEnum)MUGetIndexCI(ptr,MRsvAttr,FALSE,mraNONE);

      for (rindex = 0;rindex < RList.NumItems;rindex++)
        {
        tmpR = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

        /* all error checking done above */

        if ((tmpR->Label == NULL) || strcmp(Label,tmpR->Label))
          continue;

        switch (AIndex)
          {
          case mraAllocNodeList:

            if (MNLToString(&tmpR->NL,FALSE,",",'\0',tmpBuf,sizeof(tmpBuf)) == SUCCESS)
              {
              if (rindex == 0)
                {
                snprintf(tmpName,sizeof(tmpName),"%s-HOSTLIST",
                  Label);
                }
              else
                {
                snprintf(tmpName,sizeof(tmpName),"%s-HOSTLIST-%d",
                  Label,
                  rindex);
                }

              if (rindex == 0)
                MUSNCat(&BPtr,&BSpace,tmpBuf);
              else
                MUSNPrintF(&BPtr,&BSpace,",%s",tmpBuf);

              MTrigAddVariable(T,tmpName,0,tmpBuf,TRUE);
              }

            break;

          case mraReqOS:

            if (tmpR->ReqOS != 0)
              {
              if (rindex == 0)
                {
                snprintf(tmpName,sizeof(tmpName),"%s-OS",Label);
                }
              else
                {
                snprintf(tmpName,sizeof(tmpName),"%s-OS-%d",Label,rindex);
                }

              MUStrCpy(tmpBuf,MAList[meOpsys][tmpR->ReqOS],sizeof(tmpBuf));

              MTrigAddVariable(T,tmpName,0,tmpBuf,TRUE);
              }

            break;

          case mraVariables:

            {
            mln_t *tmpL;
            mln_t *Head;

            char *vptr;

            if (tmpVar == NULL)
              break;

            /* search variable lists for tmpVar */

            if (!strncasecmp(tmpVar,"owner.",strlen("owner.")))
              {
              switch (tmpR->OType)
                {
                case mxoUser:
                case mxoGroup:
                case mxoAcct:

                  Head = ((mgcred_t *)tmpR->O)->Variables;

                  vptr = &tmpVar[strlen("owner.")];

                  break;

                default:

                  Head = NULL;

                  vptr = NULL;

                  break;
                }
              }
            else
              {
              Head = tmpR->Variables;

              vptr = tmpVar;
              }

            if (vptr == NULL)
              {
              break;
              }

            if (MULLCheck(Head,vptr,&tmpL) == SUCCESS)
              {
              snprintf(tmpName,sizeof(tmpName),"%s-%s",Label,vptr);

              MUStrCpy(tmpBuf,(char *)tmpL->Ptr,sizeof(tmpBuf));

              MTrigAddVariable(T,tmpName,0,tmpBuf,TRUE);
              }
            else if (&tmpR->NL != NULL)
              {
              char *VarValue = NULL;

              mnode_t *N = MNLReturnNodeAtIndex(&tmpR->NL,0);

              if (MUHTGet(&N->Variables,vptr,(void **)&VarValue,NULL) == SUCCESS)
                {
                snprintf(tmpName,sizeof(tmpName),"%s-%s",Label,vptr);

                MUStrCpy(tmpBuf,VarValue,sizeof(tmpBuf));

                MTrigAddVariable(T,tmpName,0,tmpBuf,TRUE);
                }
              }    /* END else if (tmpR->NL != NULL) */
            }      /* END BLOCK mraVariables */

            break;

          default:

            /* NYI */

            break;
          }  /* END switch (AIndex) */
        }    /* END for (rindex) */

      switch (AIndex)
        {
        case mraAllocNodeList:

          snprintf(tmpName,sizeof(tmpName),"%s-HOSTLIST",Label);

          MTrigAddVariable(T,tmpName,0,AllHostList,TRUE);

          break;

        default:

          /* NYI */

          break;
        }   /* END switch (AIndex) */

      MUArrayListFree(&RList);
      }     /* END case mxoRsv */

      break;

    case mxoJob:

      break;

    default:

      break;

    }  /* END switch (T->OType) */

  T->State = mtsSuccessful;

  return(SUCCESS);
  }  /* END MTrigQuery() */



/* return User object under which to launch trigger action */

/**
 *
 *
 * @param T (I)
 * @param UP (O)
 */

int MTrigGetOwner(

  mtrig_t   *T,   /* I */
  mgcred_t **UP)  /* O */

  {
  if (UP != NULL)
    *UP = NULL;

  if (T == NULL)
    {
    return(FAILURE);
    }

  if (T->O == NULL)
    {
    return(FAILURE);
    }

  switch (T->OType)
    {
    case mxoRsv:

      {
      mrsv_t *R = (mrsv_t *)T->O;

      if (R->U == NULL)
        {
        return(FAILURE);
        }
 
      if (UP != NULL)
        *UP = R->U;

      return(SUCCESS);
      }  /* END (case mxoRsv) */

      break;

    case mxoJob:

      {
      mjob_t *J = (mjob_t *)T->O;

      if (J->Credential.U == NULL)
        {
        return(FAILURE);
        }

      if (UP != NULL)
        *UP = J->Credential.U;

      return(SUCCESS);
      }  /* END (case mxoJob) */

      break;

    case mxoSched:

      if ((T->UserName != NULL) && (MUserFind(T->UserName,UP) != FAILURE))
        {
        return(SUCCESS);
        }

      return(FAILURE);

      /*NOTREACHED*/

      break;

    default:

      return(FAILURE);
 
      /*NOTREACHED*/

      break;
    }  /* END switch (T->OType) */

  return(SUCCESS);
  }  /* END MTrigGetOwner() */





/**
 * Executed only on running triggers. This function can watch the output for the TRIGSTATUS variable
 * to determine the progress of the trigger (i.e. how much more work it has to do).
 *
 * @param T (I) [modified]
 */

int MTrigProbe(

  mtrig_t *T)  /* I (modified) */

  {
  char *ptr;

  int   index;

  char  tmpLine[MMAX_LINE];

  char *OBuf;  /* (alloc'd) */

  if (T == NULL)
    {
    return(FAILURE);
    }

  if ((OBuf = MFULoadNoCache(T->OBuf,1,NULL,NULL,NULL,NULL)) == NULL)
    {
    MDB(3,fSCHED) MLog("INFO:     cannot load output data for trigger %s (File: %s)\n",
      T->TName,
      (T->OBuf == NULL) ? "NULL" : T->OBuf);

    return(FAILURE);
    }

  tmpLine[0] = '\0';

  /* FORMAT:  {HEAD|'\n'}TRIGSTATUS[<WS>]=[<WS>]<VAL>'\n' */

  /* skip '$' marker in variable name */

  if ((ptr = MUStrStr(OBuf,"TRIGSTATUS",0,FALSE,TRUE)) == NULL)
    {
    MUFree(&OBuf);

    return(SUCCESS);
    }

  if ((ptr > OBuf) && (*(ptr - 1) != '\n'))
    {
    MUFree(&OBuf);

    return(SUCCESS);
    }

  ptr += strlen("TRIGSTATUS");

  while (isspace(*ptr) && (*ptr != '\0'))
    ptr++;

  if (*ptr != '=')
    {
    MUFree(&OBuf);

    return(SUCCESS);
    }

  ptr++;

  while (isspace(*ptr) && (*ptr != '\0'))
    ptr++;

  index = 0;

  while (index < MMAX_LINE - 1)
    {
    if ((ptr[index] == '\n') || (ptr[index] == '\0'))
      break;

    tmpLine[index] = ptr[index];

    index++;
    }

  if (index == 0)
    {
    MUFree(&OBuf);

    return(SUCCESS);
    }

  tmpLine[index] = '\0';

  if (tmpLine[0] != '\0')
    {
    T->Status = strtod(tmpLine,NULL);
    }
  else
    {
    T->Status = 0;
    }

  MUFree(&OBuf);

  return(SUCCESS);
  }  /* END MTrigProbe() */





/**
 * Searches trigger's stdout file for EXITCODE=<X>
 * and sets the trigger's exit status to X.
 *
 * @param T (I)
 * @param ExitCode (O)
 */

int MTrigLoadExitValue(
 
  mtrig_t              *T,         /* I */
  enum MTrigStateEnum  *ExitCode)  /* O */

  {
  char *ptr;

  int   index;

  char  tmpLine[MMAX_LINE];

  char *OBuf = NULL;  /* alloc'd  output buffer */
  char *EBuf = NULL;  /* alloc'd  error buffer */

  const char *FName = "MTrigLoadExitValue";

  MDB(4,fSCHED) MLog("%s(%s,ExitCode)\n",
    FName,
    ((T != NULL) && (T->TName != NULL)) ? T->TName : "NULL");

  if ((T == NULL) || (ExitCode == NULL))
    {
    return(FAILURE);
    }

  *ExitCode = mtsNONE;

  /* NOTE:  make data copy writable so it is not cached */

  if ((OBuf = MFULoadNoCache(T->OBuf,1,NULL,NULL,NULL,NULL)) == NULL)
    {
    MDB(3,fSCHED) MLog("INFO:     cannot load output data for trigger %s (File: %s)\n",
      T->TName,
      (T->OBuf == NULL) ? "NULL" : T->OBuf);

    return(FAILURE);
    }

  /* NOTE:  should search entire buffer to locate needed variables (NYI)
            (currently only check validity of first potential match) */

  tmpLine[0] = '\0';

  /* FORMAT:  {HEAD|'\n'}<VARIABLE>[<WS>]=[<WS>]<VAL>'\n' */

  if ((ptr = strstr(OBuf,"EXITCODE")) == NULL)
    {
    /* check the stderr buffer */
    if (((EBuf = MFULoadNoCache(T->EBuf,1,NULL,NULL,NULL,NULL)) == NULL) ||
        (EBuf[0] == '\0') ||
        ((ptr = strstr(EBuf,"EXITCODE")) == NULL))
      {
      MDB(6,fSCHED) MLog("INFO:     cannot locate 'EXITCODE' for trigger %s in stdout and stderr\n",
        T->TName);

      MUFree(&OBuf);
      MUFree(&EBuf);

      return(FAILURE);
      }
    }

  ptr += strlen("EXITCODE");

  /* skip whitespace */

  while (isspace(*ptr) && (*ptr != '\0'))
    ptr++;

  if (*ptr != '=')
    {
    MDB(6,fSCHED) MLog("INFO:     cannot load 'EXITCODE' for trigger %s (Buffer: %s)\n",
      T->TName,
      OBuf);

    MUFree(&OBuf);
    MUFree(&EBuf);

    return(FAILURE);
    }

  /* skip assignment '=' */

  ptr++;

  /* skip whitespace */

  while (isspace(*ptr) && (*ptr != '\0'))
    ptr++;

  /* load value (copy to end of line or end of buffer) */

  index = 0;

  while (index < MMAX_LINE - 1)
    {
    if ((ptr[index] == '\n') || (ptr[index] == '\0'))
      break;

    tmpLine[index] = ptr[index];

    index++;
    }

  if (index == 0)
    {
    MDB(6,fSCHED) MLog("INFO:     cannot load 'EXITCODE' for trigger %s (Buffer: %s)\n",
      T->TName,
      OBuf);

    MUFree(&OBuf);
    MUFree(&EBuf);

    return(FAILURE);
    }

  tmpLine[index] = '\0';

  MDB(6,fSCHED) MLog("INFO:     'EXITCODE' for trigger %s (Value: %s)\n",
    T->TName,
    tmpLine);

  index = (int)strtol(tmpLine,NULL,10);

  if (index != 0)
    {
    *ExitCode = mtsFailure;
    }
  else
    {
    *ExitCode = mtsSuccessful;
    }

  MUFree(&OBuf);
  MUFree(&EBuf);

  return(SUCCESS);
  }  /* END MTrigLoadExitValue() */
 
 
 


/**
 * Free/destroy triggers in trigger list
 *
 * @see MTListCopy() - peer
 *
 * @param TList (I) [freed]
 */

int MTrigFreeList(

  marray_t *TList)  /* I (freed) */

  {
  int tindex;

  mtrig_t *T;

  if (TList == NULL)
    {
    return(SUCCESS);
    }

  /* object trigger lists are terminated with NULL */

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    if (T == NULL)
      continue;

    if (MTrigIsReal(T) == FALSE)
      continue;

    MTrigFree(&T);
    }  /* END for (tindex) */

  return(SUCCESS);
  }  /* END MTrigFreeList() */





/**
 * Initialize Standing Trigger.
 * 
 * @see MTrigLoadString() - parent
 * @see MTrigReset() - parent
 *
 * @param T (I) [modified]
 */

int MTrigInitStanding(

  mtrig_t *T)  /* I (modified) */

  {
  long TrigStart;
  long TrigEnd;

  int  index;

  if (T->Period == mpNONE)
    {
    T->Period = mpDay;
    } 

  switch (T->Period)
    {
    case mpHour:
    case mpDay:
    case mpWeek:
    case mpMonth:

      MUGetPeriodRange(
        MSched.Time,
        0,
        0,
        T->Period,
        &TrigStart,  /* O */
        &TrigEnd,    /* O */
        NULL);

      T->ETime = TrigStart + T->Offset;

      break;

    default:

      T->ETime = MSched.Time;

      T->ETime += T->Offset;

      break;
    }  /* END switch (T->Period) */

  if ((T->ETime < MSched.Time) && (T->RearmTime != 0))
    {
    /* locate appropriate period */

    while (T->ETime < MSched.Time)
      {
      T->ETime += T->RearmTime;
      }
    }

  index = 1;

  while (T->ETime < MSched.Time)
    {
    switch (T->Period)
      {
      case mpHour:
      case mpDay:
      case mpWeek:
      case mpMonth:

        MUGetPeriodRange(
          MSched.Time,
          0,
          index++,
          T->Period,
          &TrigStart,
          &TrigEnd,
          NULL);

        T->ETime = TrigStart + T->Offset;

        break;

      default:

        T->ETime = MSched.Time;

        T->ETime += T->Offset;

        break;
      } /* END switch (T->Period) */
    }   /* END while (T->ETime < MSched.Time) */

  return(SUCCESS);
  }  /* END MTrigInitStanding() */





/**
 * Called by MTrigSetAttr() to set the flags on a trigger based
 * off of the contents of the given Buf.
 *
 * @param T (I)
 * @param Buf (I)
 */

int MTrigSetFlags(

  mtrig_t *T,
  char    *Buf)
  
  {
  char *vptr;
  int vindex;

  char UBuf[MMAX_LINE];

  if ((T == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MUStrCpy(UBuf,Buf,sizeof(UBuf));

  MUStrToLower(UBuf);
 
  for (vindex = 0;MTrigFlag[vindex] != NULL;vindex++)
    {
    if ((vptr = strstr(UBuf,MTrigFlag[vindex])) && (vptr != NULL))
      {
      switch (vindex)
        {
        case mtfUser:

          {
          char *ptr;
          char *TokPtr;

          char tmpLine[MMAX_LINE];

          bmset(&T->SFlags,vindex);

          if (strchr(vptr,'+') == NULL)
            break;

          MUStrCpy(tmpLine,vptr,sizeof(tmpLine));

          ptr = MUStrTok(tmpLine,"[] \t\n:,|.",&TokPtr);
        
          if (ptr != NULL)
            {
            vptr = strchr(ptr,'+');

            if ((vptr == NULL) || (vptr[1] == '\0'))
              break;

            MUStrDup(&T->UserName,&vptr[1]);
            }
          }       

          break;

        case mtfGlobalVars:

          {
          char *ptr;
          char *TokPtr;

          char tmpLine[MMAX_LINE];

          bmset(&T->SFlags,vindex);

          if (strchr(vptr,'+') == NULL)
            break;

          MUStrCpy(tmpLine,vptr,sizeof(tmpLine));

          ptr = MUStrTok(tmpLine,"[] \t\n:,|.",&TokPtr);
        
          if (ptr != NULL)
            {
            vptr = strchr(ptr,'+');

            if ((vptr == NULL) || (vptr[1] == '\0'))
              break;

            MUStrDup(&T->GetVars,&vptr[1]);
            }
          }       

          break;

        case mtfMultiFire:

          bmset(&T->SFlags,vindex);

          bmset(&T->InternalFlags,mtifMultiFire);

          break;

        case mtfGenericSysJob:
        case mtfSoftKill:
        case mtfCleanup:
        case mtfInterval:
        case mtfProbe:
        case mtfProbeAll:
        case mtfAttachError:
        case mtfRemoveStdFiles:
        case mtfResetOnModify:
        case mtfCheckpoint:
        case mtfObjectXMLStdin:
        case mtfAsynchronous:

          bmset(&T->SFlags,vindex);

          break;

        default:

          /* NO-OP */

          break;
        }
      }
    }  /* END for (vindex) */

  return(SUCCESS);
  }  /* END MTrigSetFlags() */



/**
 *
 *
 * @param DstTP (O) [modified/alloc]
 * @param SrcT (I)
 * @param O (I) parent object
 * @param OType () parent object type
 * @param OName () parent object name
 */

int MTrigApplyTemplate(

  marray_t **DstTP,  /* O (modified/alloc) */
  marray_t  *SrcT,   /* I */
  void      *O,      /* I parent object */
  enum MXMLOTypeEnum OType, /* parent object type */
  char      *OName)  /* parent object name */

  {
  mtrig_t *T;

  int tindex;

  marray_t *DstT;

  if ((DstTP == NULL) || (SrcT == NULL))
    {
    return(FAILURE);
    }

  MTListCopy(SrcT,DstTP,TRUE);

  DstT = *DstTP;

  for (tindex = 0; tindex < DstT->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(DstT,tindex);

    /* update trigger OID with new RID */

    T->O = O;
    T->OType = OType;

/*    FIXME: why set etime immediately?

      DstT[tindex]->ETime = MSched.Time;
*/

    MUStrDup(&T->OID,OName);

    MTrigInitialize(T);
    }    /* END for (tindex) */

  return(SUCCESS);
  }  /* END MTrigApplyTemplate() */





/**
 * Sets the "Disabled" boolean to TRUE for the given list of triggers.
 *
 * @param TList (I) The list of triggers to disable.
 */
int MTrigDisable(

  marray_t  *TList)  /* I */

  {
  mtrig_t *T;

  int tindex;

  if (TList == NULL)
    {
    return(FAILURE);
    }

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    if (MTrigIsReal(T) == FALSE)
      continue;

    bmset(&T->InternalFlags,mtifDisabled);
    }

  return(SUCCESS);
  }  /* END MTrigDisable() */



/**
 * Sets the "Disabled" boolean to FALSE for the given list of triggers.
 *
 * @param TList (I) The list of triggers to enable.
 */
int MTrigEnable(

  marray_t  *TList)  /* I */

  {
  mtrig_t *T;

  int tindex;

  if (TList == NULL)
    {
    return(FAILURE);
    }

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    if (MTrigIsReal(T) == FALSE)
      continue;

    bmunset(&T->InternalFlags,mtifDisabled);
    }

  return(SUCCESS);
  }  /* END MTrigEnable() */


/**
 * Launch internal "changeparam" action.
 *
 * @see MSysReconfigure() 
 *
 * @param T
 * @param Action
 * @param EMsg
 */

int MTrigLaunchChangeParam(

  mtrig_t *T,
  char    *Action,
  char    *EMsg)

  {
  /* TODO: place security checks on "Action" here */

  return(MSysReconfigure(Action,FALSE,FALSE,TRUE,0,EMsg));
  }  /* END MTrigLaunchChangeParam() */




/**
 * Go through the list of global triggers and mark any as failed
 * that are in state "Launched" or "Active". These triggers were
 * launched, but may not have completed due to a crash. We should
 * not re-run these triggers, as their action may be sensitive,
 * and can only run once.
 *
 * This function is designed to be run AFTER the journal log file
 * is loaded.
 */

int MTrigFailIncomplete()

  {
  int tindex = 0;
  mtrig_t *T;

  for (tindex = 0;tindex < MTList.NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

    if (MTrigIsReal(T) == FALSE)
      continue;

    if (bmisset(&T->SFlags,mtfAsynchronous))
      {
      /* let asynchronous triggers continue on */

      /* NO-OP */
      }
    else if (MTRIGISRUNNING(T))
      {
      /* need to mark trigger as failed and place block on
       * job (if job trigger) */

      if (T->OType == mxoJob)
        {
        char tmpMsg[MMAX_LINE];

        mjob_t *J;

        J = (mjob_t *)T->O;

        if (!bmisset(&T->SFlags,mtfGenericSysJob))
          {
          snprintf(tmpMsg,sizeof(tmpMsg),"trigger '%s' did not properly complete (may be due to crash)",
           T->TName);

          MJobSetHold(J,mhUser,MMAX_TIME,mhrNONE,tmpMsg);
          }
        else
          {
          /* Generic system job, trigger is longer.  Check if still running */

          if (T->PID >= 0)
            {
            if (MTrigCheckState(T) == SUCCESS)
              {
              /* We were able to check the state.  Anything special will be done there */

              continue;
              }
            }
          }
        }  /* END if (T->OType == mxoJob) */

      bmset(&T->InternalFlags,mtifIsComplete);
      T->State = mtsFailure;
      T->LastExecStatus = T->State;
      T->EndTime    = MSched.Time;
      }  /* END if (MTRIGISRUNNING(T)) */
    }  /* END for (tindex) */

  return(SUCCESS);
  }  /* END MTrigFailIncomplete() */




/**
 * Will change the state of the given trigger and also
 * record the state change in the journal log.
 *
 * Currently, this is not used everywhere we change the T->State,
 * because we do not always want to record the change in the journal.
 * This function could be changed to have a boolean to toggle journal writing,
 * and then would be suitable for all state changes.
 *
 * @param T (I)
 * @param State (I)
 */

int MTrigSetState(

  mtrig_t *T,
  enum MTrigStateEnum State)

  {
  /* set state and record in journal */

  char tmpLine[MMAX_LINE];

  if (T == NULL)
    return(FAILURE);

  snprintf(tmpLine,sizeof(tmpLine),"%s",
    MTrigState[State]);

  MCPJournalAdd(mxoTrig,T->Name,(char *)MTrigAttr[mtaState],tmpLine,mSet);

  T->State = State;

  return(SUCCESS);
  }  /* END MTrigSetState() */






int MTrigParseInternalAction(

  char               *ActionString,
  enum MXMLOTypeEnum *OType,
  char               *OID,        /* MMAX_BUFFER */
  char               *Action,     /* MMAX_LINE */
  char               *ArgLine,    /* MMAX_LINE */
  char               *Msg)        /* MMAX_LINE */

  {
  /* FORMAT:  <OTYPE>:<OID>:<ACTION>:<context information> */

  char *ptr;
  char *TokPtr = NULL;
  char *tmpAString = NULL;

  const char *FName = "MTrigPraseInternalAction";

  MDB(4,fSCHED) MLog("%s(%s,OType,OID,Action,ArgLine,,Msg)\n",
    FName,
    (ActionString != NULL) ? ActionString : "NULL");

  if (Msg != NULL)
    Msg[0] = '\0';

  if ((ActionString == NULL) || (OType == NULL) || (OID == NULL) || (Action == NULL) || (ArgLine == NULL))
    {
    if (Msg != NULL)
      strcpy(Msg,"internal error");

    return(FAILURE);
    }

  OID[0]     = '\0';
  Action[0]  = '\0';
  ArgLine[0] = '\0';

  MUStrDup(&tmpAString,ActionString);

  /* NOTE: this format and parsing is duplicated in MTrigCheckInternalAction()
           any update here must be copied to that routine */

  ptr = MUStrTok(tmpAString,":",&TokPtr);

  if (ptr == NULL)
    {
    if (Msg != NULL)
      strcpy(Msg,"invalid action");

    MUFree(&tmpAString);

    return(FAILURE);
    }

  *OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

  if (*OType == mxoNONE)
    {
    MDB(4,fSCHED) MLog("ERROR:    invalid object type '%s' in %s\n",
      ptr,
      FName);

    if (Msg != NULL)
      strcpy(Msg,"invalid object type");

    MUFree(&tmpAString);

    return(FAILURE);
    }

  ptr = MUStrTok(NULL,":",&TokPtr);

  if (ptr == NULL)
    {
    MDB(4,fSCHED) MLog("ERROR:    cannot extract object ID in %s\n",
      FName);

    if (Msg != NULL)
      strcpy(Msg,"invalid object id");

    MUFree(&tmpAString);

    return(FAILURE);
    }

  MUStrCpy(OID,ptr,MMAX_BUFFER);

  ptr = MUStrTok(NULL,":",&TokPtr);

  if (ptr == NULL)
    {
    MDB(4,fSCHED) MLog("ERROR:    cannot parse action in %s\n",
      FName);

    if (Msg != NULL)
      strcpy(Msg,"cannot locate action");

    MUFree(&tmpAString);

    return(FAILURE);
    }

  MUStrCpy(Action,ptr,MMAX_LINE);

  if ((TokPtr != NULL) && (TokPtr[0] != '\0'))
    MUStrCpy(ArgLine,TokPtr,MMAX_LINE);
  else
    ArgLine[0] = '\0';

  MUFree(&tmpAString);

  return(SUCCESS);
  }  /* END MTrigParseInternalAction() */



/*
 * If needed, alloc/instantiate the given trigger set (Sets, Unsets, FSets, etc.)
 *
 * @param Sets -> The address of the set to instantiate
 */

int MTrigInitSets(

  marray_t **Sets,  /* I */
  mbool_t    IsInt) /* I */

  {
  if (*Sets == NULL)
    {
    if ((*Sets = (marray_t *)MUCalloc(1,sizeof(marray_t))) == FAILURE)
      {
      return(FAILURE);
      }

    if (IsInt)
      {
      return(MUArrayListCreate(*Sets,sizeof(int),MDEF_DEPLIST_SIZE));
      }
    else
      {
      return(MUArrayListCreate(*Sets,sizeof(char *),MDEF_DEPLIST_SIZE));
      }
    }

  return (SUCCESS);
  }




/**
 * Encodes/decodes a trigger's env to and from a printable format.
 *
 * This function is necessary because env variables can have semicolons in them.
 *  Our env lists are semicolon delimeted, so we internally change the semicolon
 *  delimeters to the character ENVRS_ENCODED_CHAR.  This is not printable, however, so it will
 *  mess up certain functions (like checkpointing).
 *
 * So, run through this function with Encode = TRUE to make it printable, and then
 *  run through again with Encode = FALSE to change it back to the internal format
 *  (which MUSpawnChild uses).
 *
 * Semicolons are converted to &#59; (the HTML code for ';') and back
 *
 * @param String [I] (modified) - the string to encode/decode
 * @param Encode [I] - TRUE to encode (print format), FALSE to decode (put in ENVRS_ENCODED_CHAR)
 */

int MTrigTranslateEnv(

  mstring_t *String,
  mbool_t Encode)

  {
  char *ptr;
  char *Tok;

  /* This is what is used instead of semicolons */
  const char *Code = "&#59;";
  char internalC = ENVRS_ENCODED_CHAR;

  if (String == NULL)
    return(FAILURE);

  mstring_t tmpS(MMAX_LINE);

  if (Encode)
    {
    /* Encode */

    ptr = String->c_str();
    Tok = MUStrChr(ptr,internalC);

    while (Tok != NULL)
      {
      MStringAppendCount(&tmpS,ptr,Tok - ptr);

      MStringAppend(&tmpS,Code);

      ptr = Tok;
      ptr++;  /* Skip the semicolon, Tok was pointing to it */

      Tok = MUStrChr(ptr,internalC);
      }

    MStringAppend(&tmpS,ptr);
    }  /* END if (Encode) */
  else
    {
    /* Decode */

    int len = strlen(Code);
    char intC[2];

    intC[0] = internalC;
    intC[1] = '\0';

    ptr = String->c_str();
    Tok = MUStrStr(ptr,(char *)Code,FALSE,FALSE,FALSE);

    while (Tok != NULL)
      {
      MStringAppendCount(&tmpS,ptr,Tok - ptr);

      MStringAppend(&tmpS,intC);

      ptr = Tok;
      ptr += len; /* Skipping &#59; */

      Tok = MUStrStr(ptr,(char *)Code,FALSE,FALSE,FALSE);
      }

    MStringAppend(&tmpS,ptr);
    }  /* END else (Encode == FALSE) */

  MStringSet(String,tmpS.c_str());

  return(SUCCESS);
  }

/* END MTrig.c */
