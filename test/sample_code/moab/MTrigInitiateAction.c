/* HEADER */

      
/**
 * @file MTrigInitiateAction.c
 *
 * Contains: MTrigInitiateAction
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


int __InitiateChangeParam(mtrig_t *,char *,char *);
int __InitiateInternal(mtrig_t *,char *,char *);
int __InitiateMail(mtrig_t *,char *,char *);
int __InitiateExec(mtrig_t *,char *,char *);
int __InitiateModify(mtrig_t *,char *,char *);
int __InitiateQuery(mtrig_t *,char *,char *);
int __InitiateJobPreempt(mtrig_t *,char *,char *);
int __InitiateReserve(mtrig_t *,char *,char *);
int __InitiateSubmit(mtrig_t *,char *,char *);





/**
 * This function will return several variables that can be
 * used in a trigger action string.
 *
 * NOTE:  isubsequent processing of VarName[] is case sensitive.
 *
 * @see MOGetCommonVarList() - peer
 * @see MTrigInitiateAction() - parent
 *
 * @param T (I)
 * @param VarList (I/O) [modified]
 */

int __MTrigGetVarList(

  mtrig_t  *T,
  mln_t   **VarList)  /* I/O */

  {
  if ((T == NULL) || (VarList == NULL))
    {
    return(FAILURE);
    }

  MUAddVarLL(VarList,"ETYPE",(char *)MTrigType[T->EType]);

  if (T->EType == mttThreshold)
    {
    /* add threshold-specific variables */

    if (T->ThresholdGMetricType != NULL)
      {
      MUAddVarLL(VarList,"METRICTYPE",T->ThresholdGMetricType);
      }
    }

  return(SUCCESS);
  }  /* END __MTrigGetVarList() */


char  tmpVarValBuf[MDEF_VARVALBUFCOUNT][MMAX_BUFFER3];  /* hold tmp var values */

/**
 * Helper function that sets up the variable list for the trigger.
 *
 * @param T                (I) - The trigger to setup the varlist for
 * @param ActionString
 * @param ActionStringSize
 */

int SetupTrigVarList(

  mtrig_t *T,
  char    *ActionString,
  int      ActionStringSize)

  {
  char tmpObjectXML[MMAX_BUFFER << 3];
  char tmpHostList[MMAX_BUFFER << 3];
  char tmpRetry[MMAX_NAME];
  char tmpOwner[MMAX_NAME];
  char tmpStartTime[MMAX_LINE << 2];
  char tmpEndTime[MMAX_LINE << 2];
  char tmpActive[MMAX_NAME];

  char *TaskList = NULL; /* alloc'ed */
  int   tmpVarCount;
  mln_t *VarList = NULL;
  marray_t HashList;

  if (T == NULL)
    return(FAILURE);

  tmpHostList[0] = '\0';
  memset(&HashList,0x0,sizeof(marray_t));  /* set everything to NULL */

  if (T->OType != mxoJob)
    {
    mgcred_t *U = NULL;

    tmpVarCount = MDEF_VARVALBUFCOUNT - 1;

    MTrigGetOwner(T,&U);

    MOGetCommonVarList(
      T->OType,
      T->OID,
      U,
      &VarList,      /* O */
      tmpVarValBuf,
      &tmpVarCount); /* O */
    }  /* END if (T->OType != mxoJob) */

  __MTrigGetVarList(
    T,
    &VarList);   /* I/O */

  /* add object-specific trigger variables */

  switch (T->OType)
    {
    case mxoRsv:

      {
      mstring_t TaskString(MMAX_LINE);

      mln_t *tmpL;

      mrsv_t *R;
      void   *tmpR;

      if (T->O == NULL)
        break;

      R = (mrsv_t *)T->O;

      if (MUStrIsEmpty(R->Name) || (R->Name[0] == '\1'))
        break;

      /* gather hash lists that contain variables */

      MUArrayListCreate(&HashList,sizeof(mhash_t *),4);

      snprintf(tmpRetry,sizeof(tmpRetry),"%d",T->RetryCount);
      MUAddVarLL(&VarList,"RETRYCOUNT",tmpRetry);

      snprintf(tmpOwner,sizeof(tmpOwner),"%s:%s",
        MXOC[R->OType],
        MOGetName(R->O,R->OType,NULL));

      MUAddVarLL(&VarList,"OWNER",tmpOwner);

      snprintf(tmpStartTime,sizeof(tmpStartTime),"%ld",R->StartTime);
      MUAddVarLL(&VarList,"STARTTIME",tmpStartTime);

      snprintf(tmpEndTime,sizeof(tmpEndTime),"%ld",R->EndTime);
      MUAddVarLL(&VarList,"ENDTIME",tmpEndTime);

      MNLToMString(&R->NL,TRUE,",",'\0',R->DRes.Procs,&TaskString);
      MUAddVarLL(&VarList,"TASKLIST",TaskString.c_str());

      if (bmisset(&R->Flags,mrfIsActive))
        {
        snprintf(tmpActive,sizeof(tmpActive),"TRUE");
        MUAddVarLL(&VarList,"ACTIVE",tmpActive);
        }

      if (R->RsvGroup != NULL)
        {
        mpar_t *C;

        if (MVPCFind(R->RsvGroup,&C,TRUE) == SUCCESS)
          {
          MUAddVarLL(&VarList,"VPCID",C->Name);

          if (C->P != NULL)
            {
            if (C->P->Name[0] != '\0')
              {
              MUAddVarLL(&VarList,"CLIENTNAME",C->P->Name); 
              }

            if (C->P->CSKey != NULL)
              {
              MUAddVarLL(&VarList,"CLIENTKEY",C->P->CSKey);
              }
            }
          } 

        MUAddVarLL(&VarList,"RSVGROUP",R->RsvGroup);
        }    /* END if (R->RsvGroup != NULL) */

      if (R->ParentVCs != NULL)
        {
        /* Add in variables from any VCs */

        mln_t *tmpL;
        mvc_t *tmpVC;

        for (tmpL = R->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
          {
          tmpVC = (mvc_t *)tmpL->Ptr;

          MVCAddVarsToLLRecursive(tmpVC,&VarList,NULL);
          }
        }

      /* generate hostlist */

      if ((MNLIsEmpty(&R->NL)) &&
         ((R->AllocNC > 0) ||
         ((R->HostExp != NULL) && (strstr(R->HostExp,"GLOBAL") != NULL))))
        {
        MRsvCreateNL(
          &R->NL, /* O (alloc) */
          R->HostExp,
          0,
          0,
          NULL,
          &R->DRes,
          NULL);
        }

      if (!MNLIsEmpty(&R->NL))
        {
        mnode_t *tmpN;

        MNLGetNodeAtIndex(&R->NL,0,&tmpN);
        MUAddVarLL(&VarList,"OS",(char *)MAList[meOpsys][tmpN->ActiveOS]);
        }

      if (!MNLIsEmpty(&R->NL))
        {
        mnode_t *tmpN;

        char *BPtr;
        int   BSpace;

        int  nindex;

        mnl_t *NL = &R->NL;

        MUSNInit(&BPtr,&BSpace,tmpHostList,sizeof(tmpHostList));

        for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&tmpN) == SUCCESS;nindex++)
          {
          if (nindex > 0)
            MUSNPrintF(&BPtr,&BSpace,",%s",
              tmpN->Name);
          else
            MUSNCat(&BPtr,&BSpace,tmpN->Name);
          }
        }   /* END if (R->NL != NULL) */

      MUAddVarLL(&VarList,"HOSTLIST",tmpHostList);

      MRsvToString(R,NULL,tmpObjectXML,sizeof(tmpObjectXML),NULL);

      MUAddVarLL(&VarList,"OBJECTXML",tmpObjectXML);

      tmpL = ((mrsv_t *)T->O)->Variables;

      while (tmpL != NULL)
        {
        MUAddVarLL(&VarList,tmpL->Name,(tmpL->Ptr != NULL) ? (char *)tmpL->Ptr : (char *)"NULL");

        tmpL = tmpL->Next;
        }

      if (bmisset(&R->Flags,mrfStanding) &&
          (MSRFind(R->RsvGroup,(msrsv_t **)&tmpR,NULL) == SUCCESS))
        {
        tmpL = ((msrsv_t *)tmpR)->Variables;
        }
      else if ((R->RsvGroup != NULL) &&
          (MRsvFind(R->RsvGroup,(mrsv_t **)&tmpR,mraNONE) == SUCCESS))
        {
        tmpL = ((mrsv_t *)tmpR)->Variables;
        }
      else
        {
        tmpL = NULL;
        }

      while (tmpL != NULL)
        {
        MUAddVarLL(&VarList,tmpL->Name,(tmpL->Ptr != NULL) ? (char *)tmpL->Ptr : (char *)"NULL");

        tmpL = tmpL->Next;
        }  /* END while (tmpL != NULL) */

      if (bmisset(&T->SFlags,mtfGlobalVars))
        {
        char *tmpVarName;
        char *tmpVarValue;

        mhashiter_t HTI;

        mnode_t *N;

        if (T->GetVars != NULL)
          {
          MNodeFind(T->GetVars,&N);

          if (N != NULL)
            {
            MUHTIterInit(&HTI);

            while (MUHTIterate(&N->Variables,&tmpVarName,(void **)&tmpVarValue,NULL,&HTI) == SUCCESS)
              {
              MUAddVarLL(&VarList,tmpVarName,tmpVarValue); 
              }
            }
          }    /* END if (T->GetVars != NULL) */
        else
          {
          int nindex;
     
          for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
            {
            N = MNode[nindex];
     
            if ((N == NULL) || (N->Name[0] == '\0'))
              break;
     
            if (N->Name[0] == '\1')
              continue;
     
            if (!bmisset(&N->Flags,mnfGlobalVars))
              continue;
     
            MUHTIterInit(&HTI);

            while (MUHTIterate(&N->Variables,&tmpVarName,(void **)&tmpVarValue,NULL,&HTI) == SUCCESS)
              {
              MUAddVarLL(&VarList,tmpVarName,tmpVarValue); 
              }
            }  /* END for (nindex) */
          }    /* END else */
        }      /* END if (bmisset(&T->SFlags,mtfGlobalVars)) */
      }        /* END BLOCK (case mxoRsv) */

      break;

    case mxoJob:

      {
      mjob_t *J;

      if (T->O == NULL)
        break;

      J = (mjob_t *)T->O;

      /* gather hash lists that contain variables */

      MUArrayListCreate(&HashList,sizeof(mhash_t *),4);

      if (J->Variables.Table != NULL)
        {
        MUArrayListAppendPtr(&HashList,&J->Variables);
        }

      if (J->JGroup != NULL)
        {
        mjob_t *tmpJ = NULL;

        if (MJobFind(J->JGroup->Name,&tmpJ,mjsmExtended) == SUCCESS)
          {
          if (tmpJ->Variables.Table != NULL)
            {
            MUArrayListAppendPtr(&HashList,&tmpJ->Variables);
            }
          }
        }

      if (J->ParentVCs != NULL)
        {
        /* Add in variables from any VCs */

        char *Namespace = NULL;
        mln_t *tmpL;
        mvc_t *tmpVC;

        MJobGetDynamicAttr(J,mjaTrigNamespace,(void **)&Namespace,mdfString);

        for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
          {
          tmpVC = (mvc_t *)tmpL->Ptr;

          MVCAddVarsToLLRecursive(tmpVC,&VarList,Namespace);
          }

        MUFree(&Namespace);
        }

      /* gather variables */

      tmpVarCount = MDEF_VARVALBUFCOUNT - 1;

      MJobGetVarList(
        J,
        &VarList,
        tmpVarValBuf,
        &tmpVarCount);

      snprintf(tmpRetry,sizeof(tmpRetry),"%d",T->RetryCount);
      MUAddVarLL(&VarList,"RETRYCOUNT",tmpRetry);

      if (J->ReqVPC != NULL)
        {
        mln_t  *tmpL = NULL;
        mpar_t *C;
        mrsv_t *R;

        /* extract variables from rsv group leader */
        
        MVPCFind(J->ReqVPC,&C,FALSE);

        if (C != NULL)
          {
          MRsvFind(C->RsvGroup,&R,mraNONE);

          if (R != NULL)
            tmpL = R->Variables;
          }

        while (tmpL != NULL)
          {
          MUAddVarLL(&VarList,tmpL->Name,(tmpL->Ptr != NULL) ? (char *)tmpL->Ptr : (char *)"NULL");

          tmpL = tmpL->Next;
          }  /* END while (tmpL != NULL) */
        }

      /* NOTE: global node trigger variables not set in MJobGetVarList() */

      if (bmisset(&T->SFlags,mtfGlobalVars))
        {
        char *tmpVarName;
        char *tmpVarValue;

        mhashiter_t HTI;

        mnode_t *N;

        if (T->GetVars != NULL)
          {
          MNodeFind(T->GetVars,&N);

          if (N != NULL)
            {
            MUHTIterInit(&HTI);

            while (MUHTIterate(&N->Variables,&tmpVarName,(void **)&tmpVarValue,NULL,&HTI) == SUCCESS)
              {
              MUAddVarLL(&VarList,tmpVarName,tmpVarValue); 
              }
            }
          }    /* END if (T->GetVars != NULL) */
        else
          {
          int nindex;
     
          for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
            {
            N = MNode[nindex];
     
            if ((N == NULL) || (N->Name[0] == '\0'))
              break;
     
            if (N->Name[0] == '\1')
              continue;
     
            if (!bmisset(&N->Flags,mnfGlobalVars))
              continue;
     
            MUHTIterInit(&HTI);

            while (MUHTIterate(&N->Variables,&tmpVarName,(void **)&tmpVarValue,NULL,&HTI) == SUCCESS)
              {
              MUAddVarLL(&VarList,tmpVarName,tmpVarValue); 
              }
            }  /* END for (nindex) */
          }    /* END else */
        }      /* END if (bmisset(&T->SFlags,mtfGlobalVars)) */
      }        /* END BLOCK (case mxoJob) */

      break;

    case mxoxVM:

      {
      mvm_t *V;

      V = (mvm_t *)T->O;

      if (V->N != NULL)
        {
        MUAddVarLL(&VarList,"HOSTLIST",V->N->Name);
        }

      if (V->Variables.NumItems > 0)
        {
        /* Iterate through variables */

        mhashiter_t VarIter;
        char *tmpVarName;
        char *tmpVarValue;

        MUHTIterInit(&VarIter);

        while (MUHTIterate(&V->Variables,&tmpVarName,(void **)&tmpVarValue,NULL,&VarIter) == SUCCESS)
          {
          MUAddVarLL(&VarList,tmpVarName,tmpVarValue);
          }    /* END while (MUHTIterate(&V->Variables,...) == SUCCESS) */
        }      /* END if (V->Variables.NumItems > 0) */
      }        /* END BLOCK (case mxoxVM) */

      break;

    case mxoNode:

      {
      mnode_t *N;

      mhashiter_t VarIter;
      char *tmpVarName;
      char *tmpVarValue;

      N = (mnode_t *)T->O;

      MUHTIterInit(&VarIter);

      while (MUHTIterate(&N->Variables,&tmpVarName,(void **)&tmpVarValue,NULL,&VarIter) == SUCCESS)
        {
        MUAddVarLL(&VarList,tmpVarName,tmpVarValue);
        }    /* END while (MUHTIterate(&N->Variables,...) == SUCCESS) */
      }        /* END BLOCK (case mxoNode) */

      break;


    default:

      /* not supported */

      break;
    }  /* END switch (T->OType) */

  /* Insert EType */
  MUAddVarLL(&VarList,"ETYPE",(char *)MTrigType[T->EType]);

  MUInsertVarList(
    T->Action,
    VarList,
    (mhash_t **)HashList.Array,
    (T->AType == mtatExec) ? (char *)"UNDEFINED" : NULL,
    ActionString,              /* O */
    ActionStringSize,
    FALSE);

  MUArrayListFree(&HashList);

  MUFree(&TaskList);
  MULLFree(&VarList,MUFREE);

  return(SUCCESS);
  }  /* END SetupTrigVarList() */



/**
 * Helper function to launch ChangeParam trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateChangeParam(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  /* NOTE:  changeparam action may result in other triggers being fired - set
            state to active before internal action launch to prevent recursion */

  T->State = mtsActive;

  MOWriteEvent((void *)T,mxoTrig,mrelTrigStart,NULL,NULL,NULL);

  if (MTrigLaunchChangeParam(T,ActionString,tmpMsg) == SUCCESS)
    {
    T->State = mtsSuccessful;
    }
  else
    {
    T->State = mtsFailure;
    }

  return(SUCCESS);
  }  /* END __InitiateChangeParam() */



/**
 * Helper function to launch Internal trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateInternal(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  mbool_t Complete = TRUE;
  int rc;

  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  /* NOTE:  internal action may result in other triggers being fired - set 
            state to active before internal action launch to prevent recursion */

  T->State = mtsActive;

  MOWriteEvent((void *)T,mxoTrig,mrelTrigStart,NULL,NULL,NULL);

  rc = MTrigLaunchInternalAction(T,ActionString,&Complete,tmpMsg);

  if ((rc == SUCCESS) && (Complete == TRUE))
    {
    T->State = mtsSuccessful;
    }
  else if (rc == FAILURE)
    {
    T->State = mtsFailure;
    }

  return(SUCCESS);
  }  /* END __InitiateInternal() */



/**
 * Helper function to launch Mail trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateMail(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  char ToList[MMAX_LINE];

  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  ToList[0] = '\0';
  tmpMsg[0] = '\0';

  if (MSched.Action[mactMail][0] == '\0')
    {
    return(FAILURE);
    }

  /* locate object owner */

  switch (T->OType)
    {
    case mxoNode:
    case mxoSched:

      {
      mgcred_t *U = NULL;  /* warning: declaration of ‘U’ shadows a previous local */

      /* send to primary admin */
      
      MUserFind(MSched.Admin[1].UName[0],&U);

      if ((U != NULL) && (U->EMailAddress != NULL))
        MUStrCpy(ToList,U->EMailAddress,sizeof(ToList));
      else 
        MUStrCpy(ToList,MSched.Admin[1].UName[0],sizeof(ToList));
      }

      break;

    case mxoJob:

      /* send to user */

      if ((T->O != NULL) && (((mjob_t *)T->O)->Credential.U != NULL))
        {
        mgcred_t *U;  /* warning: declaration of ‘U’ shadows a previous local */

        U = ((mjob_t *)T->O)->Credential.U;

        if (U->EMailAddress != NULL)
          MUStrCpy(ToList,U->EMailAddress,sizeof(ToList));
        else
          MUStrCpy(ToList,U->Name,sizeof(ToList));
        }

      break;

    case mxoRsv:

      /* send to owner */

      if (T->O != NULL)
        {
        mrsv_t *R;
        mgcred_t *U = NULL;  /* warning: declaration of ‘U’ shadows a previous local */

        R = (mrsv_t *)T->O;

        /* FORMAT:  <TYPE>:<OID> */

        if ((R->O != NULL) &&
            (R->OType == mxoUser) && 
           ((mgcred_t *)R->O)->EMailAddress != NULL)
          {
          MUStrCpy(ToList,((mgcred_t *)R->O)->EMailAddress,sizeof(ToList));
          }
        else
          {
          /* send to primary admin */
      
          MUserFind(MSched.Admin[1].UName[0],&U);

          if ((U != NULL) && (U->EMailAddress != NULL))
            MUStrCpy(ToList,U->EMailAddress,sizeof(ToList));
          else 
           MUStrCpy(ToList,MSched.Admin[1].UName[0],sizeof(ToList));
          }
        }   /* END if (T->O != NULL) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (T->OType) */

  if (ToList[0] == '\0')
    {
    MTrigSetState(T,mtsFailure);

    return(FAILURE);
    }

  if (MSched.DisableTriggers == TRUE)
    {
    MDB(1,fCORE) MLog("INFO:     triggers disabled\n");
    }
  else
    {
    MOWriteEvent((void *)T,mxoTrig,mrelTrigStart,NULL,NULL,NULL);

    MSysSendMail(
      ToList,
      NULL,
      "moab update",
      MSched.Action[mactMail],
      ActionString);

    /* FIXME - why are we building up a tmpMsg and then never using it? */

    snprintf(tmpMsg,sizeof(tmpMsg),"trigger %s completed on %s %s\n\n%s",
      T->TName,
      MXO[T->OType],
      (T->OID != NULL) ? T->OID : "",
      (T->Action != NULL) ? T->Action : "");
    }

  T->State = mtsSuccessful;

  return(SUCCESS);
  }  /* END __InitiateMail() */


/**
 * Helper function to launch Exec trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateExec(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  char EMsg[MMAX_LINE];

  char *Exec;
  char *Args;

  mgcred_t *U = NULL;
  mstring_t tmpObjectXMLStr(MMAX_LINE);
  char *StdIn;

  int   UID = -1;
  int   GID = -1;

  int   rc;

  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  Exec = MUStrTok(ActionString," ",&Args);

  if (MSched.DisableTriggers == TRUE)
    {
    MDB(1,fCORE) MLog("INFO:     triggers disabled\n");

    rc = SUCCESS;
    }
  else
    {
    int SC;  /* exit status for trigger */

    if (bmisset(&T->SFlags,mtfUser))
      {
      /* get trigger owner */

      if ((T->UserName != NULL) && 
          (MUserAdd(T->UserName,&U) == FAILURE) &&
          (MUserGetUID(U,&UID,&GID) == FAILURE))
        {
        snprintf(EMsg,sizeof(EMsg),"exec trigger %s failed, could not get info for user '%s'",
          T->TName, 
          T->UserName);

        MDB(3,fSTRUCT) MLog("ALERT:    %s\n",
          EMsg);

        bmset(&T->InternalFlags,mtifFailureDetected);

        T->State = mtsFailure;

        MTrigSetAttr(T,mtaMessage,EMsg,mdfString,mAdd);

        return(SUCCESS);
        }
      else if ((U == NULL) ||
               (MUserGetUID(U,&UID,&GID) == FAILURE))
        {
        snprintf(EMsg,sizeof(EMsg),"exec trigger %s failed, could not get user info",
          T->TName);

        MDB(3,fSTRUCT) MLog("ALERT:    %s\n",
          EMsg);

        bmset(&T->InternalFlags,mtifFailureDetected);

        T->State = mtsFailure;

        MTrigSetAttr(T,mtaMessage,EMsg,mdfString,mAdd);

        return(SUCCESS);
        }
      }    /* END if (bmisset(&T->SFlags,mtfUser)) */

    /* Find StdIn */

    if ((bmisset(&T->SFlags,mtfObjectXMLStdin)) &&
         ((T->OType == mxoRsv) ||
          (T->OType == mxoJob)))
      {
      /* Set StdIn to parent object XML */

      mxml_t *tmpE = NULL;

      /* Currently for RSV or JOB parent objects */

      if (T->OType == mxoRsv)
        {
        mrsv_t *R;

        R = (mrsv_t *)T->O;

        MRsvToXML(R,&tmpE,NULL,NULL,FALSE,mcmNONE);
        }
      else if (T->OType == mxoJob)
        {
        mjob_t *J;

        enum MJobAttrEnum JAList[] = {
          mjaAccount,
          mjaAllocNodeList,  /* special grid treatment like mrqaAllocNodeList */
          mjaAllocVMList,
          mjaArgs,           /* job executable arguments */
          mjaAWDuration,     /* active wall time consumed */
          mjaBlockReason,
          mjaBypass,
          mjaClass,
          mjaCmdFile,        /* command file path */
          mjaCompletionCode, 
          mjaCompletionTime,
          mjaCost,
          mjaCPULimit,       /* job CPU limit */
          mjaDepend,         /* dependencies on status of other jobs */
          mjaDescription,    /* old-style message - char* */
          mjaDRM,            /* master destination job rm */
          mjaDRMJID,         /* destination rm job id */
          mjaEEWDuration,    /* eff eligible duration:  duration job has been eligible for scheduling */
          mjaEffPAL,
          mjaEnv,            /* job execution environment variables */
          mjaEpilogueScript,
          mjaEState,
          mjaExcHList,       /* exclusion host list */
          mjaFlags,
          mjaGAttr,          /* requested generic attributes */
          mjaGJID,           /* global job id */
          mjaGroup,
          mjaHold,           /* hold list */
          mjaHoldTime,
          mjaReqHostList,    /* requested hosts */
          mjaHoldTime,       /* time since job had hold placed on it */
          mjaInteractiveScript,
          mjaIsInteractive,  /* job is interactive */
          mjaIsRestartable,  /* job is restartable (NOTE: map into specflags) */
          mjaIsSuspendable,  /* job is suspendable (NOTE: map into specflags) */
          mjaIWD,            /* execution directory */
          mjaJobID,          /* job batch id */
          mjaJobName,        /* user specified job name */
          mjaJobGroup,       /* job group id */
          mjaLogLevel,
          mjaMasterHost,     /* host to run primary task on */
          mjaMinPreemptTime, /* earliest duration after job start at which job can be preempted */
          mjaMinWCLimit,
          mjaMessages,       /* new style message - mmb_t */
          mjaNodeAllocationPolicy,
          mjaNotification,   /* notify user of specified events */
          mjaNotificationAddress, /* the address to send user notifications to */
          mjaOwner,
          mjaPAL,
          mjaPrologueScript,
          mjaQOS,
          mjaQOSReq,
          mjaReqAWDuration,  /* req active walltime duration */
          mjaReqFeatures,
          mjaReqMem,
          mjaReqNodes,       /* number of nodes requested (obsolete) */
          mjaReqProcs,       /* obsolete */
          mjaReqReservation, /* required reservation */
          mjaReqReservationPeer, /* required reservation originally received from peer */
          mjaReqSMinTime,    /* req earliest start time */
          mjaRM,
          mjaRMSubmitFlags,
          mjaRMXString,      /* RM extension string */
          mjaRsvAccess,      /* list of reservations accessible by job */
          mjaRsvStartTime,   /* reservation start time */
          mjaRunPriority,    /* effective job priority */
          mjaSessionID,
          mjaShell,
          mjaSize,           /* job's computational size */
          mjaSRMJID,         /* source rm job id */
          mjaStartCount,
          mjaStartPriority,  /* effective job priority */
          mjaStartTime,      /* most recent time job started execution */
          mjaState,
          mjaStatMSUtl,      /* memory seconds utilized */
          mjaStatPSDed,
          mjaStatPSUtl,
          mjaStdErr,         /* path of stderr output */
          mjaStdIn,          /* path of stdin */
          mjaStdOut,         /* path of stdout output */
          mjaStepID,
          mjaSubmitArgs,
          mjaSubmitDir,
          mjaSubmitLanguage, /* resource manager language of submission request */
          mjaSubmitTime,
          mjaSubState,
          mjaSuspendDuration, /* duration job has been suspended */
          mjaSysPrio,        /* admin specified system priority */
          mjaSysSMinTime,    /* system specified min start time */
          mjaTaskMap,        /* nodes allocated to job */
          mjaTemplateSetList,
          mjaTemplateFlags,
          mjaTrigger,
          mjaTrigNamespace,
          mjaTTC,
          mjaUMask,          /* umask of user submitting the job */
          mjaUser,
          mjaUserPrio,       /* user specified job priority */
          mjaUtlMem,
          mjaUtlProcs,
          mjaVariables,
          mjaVM,
          mjaVMUsagePolicy,
          mjaVWCLimit,       /* virtual wc limit */
          mjaNONE };

        enum MReqAttrEnum JRQList[] = {
          mrqaIndex,
          mrqaNodeAccess,
          mrqaNodeSet,
          mrqaPref,
          mrqaReqArch,          /* architecture */
          mrqaReqDiskPerTask,
          mrqaReqMemPerTask,
          mrqaReqNodeDisk,
          mrqaReqNodeFeature,
          mrqaReqNodeMem,
          mrqaReqNodeProc,
          mrqaReqNodeSwap,
          mrqaReqOpsys,
          mrqaReqPartition,
          mrqaReqProcPerTask,
          mrqaReqSwapPerTask,
          mrqaNCReqMin,
          mrqaTCReqMin,
          mrqaTPN,
          mrqaGRes,
          mrqaCGRes,
          mrqaNONE };

        J = (mjob_t *)T->O;

        MJobToXML(J,&tmpE,JAList,JRQList,NULL,FALSE,FALSE);
        }

      MXMLToMString(tmpE,&tmpObjectXMLStr,(char const **) NULL,TRUE);
      MXMLDestroyE(&tmpE);

      StdIn = tmpObjectXMLStr.c_str();

      MXMLDestroyE(&tmpE);
      }
    else if (T->StdIn != NULL)
      {
      StdIn = T->StdIn;
      }
    else
      {
      StdIn = NULL;
      }

    MOWriteEvent((void *)T,mxoTrig,mrelTrigStart,NULL,NULL,NULL);

    rc = MUSpawnChild(
      Exec,                    /* exec */
      Exec,                    /* name */
      Args,                    /* args */
      NULL,                    /* shell */
      UID,                     /* UID */
      GID,                     /* GID */
      NULL,                    /* cwd */
      T->Env,                  /* env */
      NULL,
      NULL,                    /* limits */
      StdIn,                   /* IBuf */
      &T->IBuf,                /* IBuf filename */
      &T->OBuf,                /* OBuf */
      &T->EBuf,                /* EBuf */
      &T->PID,                 /* pid */
      &SC,                     /* exit code */
      0,                       /* timelimit */
      T->BlockTime,            /* blocktime */
      (void *)T,               /* object */
      mxoTrig,                 /* OType */
      FALSE,                   /* check env */  
      FALSE,                   /* ignore default env */  
      EMsg);
    }  /* END else */

  /* NOTE:  MUSpawnChild will directly update T->State to mtsSuccessful/mtsFailure upon bad exit code */

  /* NOTE:  may want to adjust FAIL/SUCCESS based upon SC value (NYI) */

  if (rc == FAILURE)
    {
    MDB(3,fSTRUCT) MLog("ALERT:    exec trigger %s failed, '%s'\n",
      T->TName,
      EMsg);

    bmset(&T->InternalFlags,mtifFailureDetected);

    T->State = mtsFailure;

    MTrigSetAttr(T,mtaMessage,EMsg,mdfString,mAdd);
    }

  if (bmisset(&T->SFlags,mtfAsynchronous))
    {
    /* drop the PID and wait for somebody to tell us the trigger is done (via mschedctl) */

    T->PID = 0;
    }
  else if ((rc == SUCCESS) && (MSched.DisableTriggers == FALSE))
    {
    mpid_t *TrigPID;

    /* trigger succeeded */

    MDB(7,fSTRUCT) MLog("INFO:     trigger %s launched child with PID %d\n",
      (T->Name == NULL) ? T->TName : T->Name,
      T->PID);

    /* add to trigger/PID table */

    TrigPID = (mpid_t *)MUCalloc(1,sizeof(mpid_t));

    TrigPID->Object = T;

    MUHTAddInt(&TrigPIDs,T->PID,TrigPID,NULL,MUFREE);

    MDB(7,fSTRUCT) MLog("INFO:    added trigger '%s' with PID '%d' to hashtable\n",
      (T->Name == NULL) ? T->TName : T->Name,
      T->PID);
    }

  return(SUCCESS);
  }  /* END __InitiateExec() */

/**
 * Helper function to launch Modify trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateModify(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  char tmpLine[MMAX_LINE];

  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  /* NOTE:  modify action may result in other triggers being fired - set
            state to active before internal action launch to prevent recursion */

  T->State = mtsActive;

  MOWriteEvent((void *)T,mxoTrig,mrelTrigStart,NULL,NULL,NULL);

  if (MTrigDoModifyAction(T,ActionString,tmpLine) == SUCCESS)
    {
    T->State = mtsSuccessful;
    }
  else
    {
    T->State = mtsFailure;
    }

  return(SUCCESS);
  }  /* END __InitiateModify() */



/**
 * Helper function to launch Query trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateQuery(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  MOWriteEvent((void *)T,mxoTrig,mrelTrigStart,NULL,NULL,NULL);

  if (MTrigQuery(T,T->Action,TRUE) == SUCCESS)
    {
    T->State = mtsSuccessful;
    }
  else
    {
    T->State = mtsFailure;
    }

  return(SUCCESS);
  }  /* END __InitiateQuery() */


/**
 * Helper function to launch JobPreempt trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateJobPreempt(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  enum MPreemptPolicyEnum PPolicy;

  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  /* NOTE:  preempt action may result in other triggers being fired - set
            state to active before internal action launch to prevent recursion */

  T->State = mtsActive;

  MOWriteEvent((void *)T,mxoTrig,mrelTrigStart,NULL,NULL,NULL);

  switch (T->OType)
    {
    case mxoRsv:

      {
      mrsv_t *R = (mrsv_t *)T->O;

      if (!bmisset(&T->InternalFlags,mtifMultiFire))
        {
        T->EndTime    = MSched.Time;
        bmset(&T->InternalFlags,mtifIsComplete);
        }

      if (T->Action != NULL)
        {
        PPolicy = (enum MPreemptPolicyEnum)MUGetIndexCI(
          T->Action,
          MPreemptPolicy,
          FALSE,
          mppNONE);
        }
      else
        {
        PPolicy = mppNONE;
        }

      if (MNLPreemptJobs(&R->NL,"trigger fired",PPolicy,TRUE) == SUCCESS)
        {
        T->State = mtsSuccessful;
        }
      else
        {
        T->State = mtsFailure;
        }
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;

    }  /* END switch (T->OType) */

  return(SUCCESS);
  }  /* END __InitiateJobPreempt() */



/**
 * Helper function to launch Reserve trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateReserve(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  mrsv_t    *R;
  mulong     Duration;
  mnl_t     *NL = NULL;

  char       tmpLine[MMAX_LINE];

  char      *ptr;
  char      *TokPtr = NULL;

  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  /* NOTE:  reserve action may result in other triggers being fired - set
            state to active before internal action launch to prevent recursion */

  T->State = mtsActive;

  /* initialize defaults */

  Duration = MCONST_HOURLEN;

  /* Action Format:  <DURATION>[|profile=<PROFILE>][|subtype=<TYPE>]... */

  if (T->Action != NULL)
    {
    MUStrCpy(tmpLine,T->Action,sizeof(tmpLine));

    ptr = MUStrTok(tmpLine,"|",&TokPtr);

    if (isdigit(T->Action[0])) 
      {
      Duration = MUTimeFromString(ptr);

      ptr = MUStrTok(NULL,"|",&TokPtr);
      }
    }
  else
    {
    ptr = NULL;
    }

  switch (T->OType)
    {
    case mxoJob:

      {
      mbitmap_t Flags;

      mjob_t *J;

      char tmpName[MMAX_NAME];

      J = (mjob_t *)T->O;

      if (J != NULL)
        {
        NL = &((mjob_t *)T->O)->NodeList;

        snprintf(tmpName,sizeof(tmpName),"trig.%s.%d",
          J->Name,
          MSched.RsvCounter++);
        }
      else
        {
        snprintf(tmpName,sizeof(tmpName),"trig.job.%d",
          MSched.RsvCounter++);

        NL = NULL;
        }

      if (MRsvCreate(
            mrtUser,
            NULL,
            NULL,
            &Flags,
            NL,
            MSched.Time,
            MSched.Time + Duration,
            1,
            -1,
            tmpName,
            &R,
            NULL,
            NULL,
            NULL,
            TRUE,
            TRUE) == FAILURE)
        {
        MDB(1,fCORE) MLog("WARNING:  trigger rsv creation failed\n");

        bmclear(&Flags);

        return(FAILURE);
        }

      bmclear(&Flags);

      while (ptr != NULL)
        {
        if (!strncasecmp(ptr,"profile=",strlen("profile=")))
          {
          /* apply rsv profile */

          /* NYI */
          }
        else if (!strncasecmp(ptr,"subtype=",strlen("subtype=")))
          {
          /* apply rsv subtype */

          ptr += strlen("subtype=");

          MRsvSetAttr(R,mraSubType,(void *)ptr,mdfString,mSet);
          }

        ptr = MUStrTok(NULL,";",&TokPtr);
        }  /* END while (ptr != NULL) */ 
      }    /* END (case mxoJob) */

      /* Mark state as successful so trigger doesn't continue to fire */
      T->State = mtsSuccessful;

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (T->OType) */

  return(SUCCESS);
  }  /* END __InitiateReserve() */



/**
 * Helper function to launch Submit trigger actions.
 *
 * @param T            [I] - The trigger that is launching
 * @param ActionString [I] - ActionString
 * @param tmpMsg       [O] - Temporary message string
 */

int __InitiateSubmit(

  mtrig_t *T,
  char    *ActionString,
  char    *tmpMsg)

  {
  int   rc;
  char *SubmitExe = NULL;
  char  EMsg[MMAX_LINE];

  if ((T == NULL) ||
      (ActionString == NULL) ||
      (tmpMsg == NULL))
    return(FAILURE);

  /* NOTE:  submit action may result in other triggers being fired - set
            state to active before internal action launch to prevent recursion */

  T->State = mtsActive;

  /* NOTE:  generalize for all trigger types (NYI) */

  switch (MRM[0].Type)
    {
    case mrmtPBS:
    default:

      SubmitExe = MRM[0].PBS.SubmitExe;

      break;
    }  /* END switch (MRM[0].Type) */

  /* submit request */

  /* if no rsv owner specified, launch as admin */

  if (MSched.DisableTriggers == TRUE)
    {
    MDB(1,fCORE) MLog("INFO:     triggers disabled\n");

    rc = SUCCESS;
    }
  else
    {
    int SC;  /* exit code for trigger */

    rc = MUSpawnChild(
      SubmitExe,               /* exec */
      SubmitExe,               /* name */
      T->Action,               /* args */
      NULL,                    /* shell */
      -1,                      /* UID */
      -1,                      /* GID */
      NULL,                    /* cwd */
      NULL,                    /* env */
      NULL,
      NULL,                    /* limits */
      NULL,                    /* IBuf */
      NULL,
      NULL,                    /* OBuf */
      NULL,                    /* EBuf */
      NULL,                    /* pid */
      &SC,                     /* exit code */
      0,                       /* timelimit */
      T->BlockTime,            /* blocktime */
      (void *)T,               /* object */
      mxoTrig,                 /* OType */
      FALSE,                   /* check env */
      FALSE,                   /* ignore default env */
      EMsg);
    }

  /* NOTE:  MUSpawnChild will directly update T->State to mtsSuccessful/mtsFailure upon bad exit code */

  /* NOTE:  may want to adjust FAIL/SUCCESS based upon SC value (NYI) */

  if (rc == FAILURE)
    {
    MDB(3,fSTRUCT) MLog("ALERT:    submit trigger %s failed, '%s'\n",
      T->TName,
      EMsg);

    bmset(&T->InternalFlags,mtifFailureDetected);

    T->State = mtsFailure;

    MUStrDup(&T->Msg,EMsg);
    }

  return(SUCCESS);
  }  /* END __InitiateSubmit() */


/**
 * Launch internal/external trigger action. 
 *
 * @see MSchedCheckTriggers() - parent
 * @see MOGetCommonVarList() - child - load trigger environment
 * @see MTrigDoModifyAction() - child - modify object
 * @see MTrigLaunchInternalAction() - child
 *
 * NOTE: tmpVarValBuf is too large to be local.  This routine is not 
 *       thread safe.
 *
 * @param T (I)
 */


int MTrigInitiateAction(

  mtrig_t *T) /* I */

  {
  char ActionString[MMAX_BUFFER << 1];

  char  tmpMsg[MMAX_LINE];

  const char *FName = "MTrigInitiateAction";

  MDB(3,fSTRUCT) MLog("%s(%s)\n",
    FName,
    ((T != NULL) && (T->Name != NULL)) ? T->Name : "NULL");

#ifndef __MOPT
  if ((T == NULL) || (T->Action == NULL))
    {
    return(FAILURE);
    }
#endif /* !__MOPT */

  ActionString[0] = '\0';

  if ((T->EType == mttThreshold) && (bmisset(&MSched.RecordEventList,mrelTrigThreshold)))
    {
    /* If DisableThresholdTrigs is set, so will mrelTrigThreshold */

    char EventMsg[MMAX_LINE];

    snprintf(EventMsg,sizeof(EventMsg),"%s threshold trigger %s, %s threshold%s%s is %f",
      (MSched.DisableThresholdTrigs) ? "Intended: fire" : "firing",
      (T->Name != NULL) ? T->Name : T->TName,
      MTrigThresholdType[T->ThresholdType],
      (T->ThresholdType == mtttGMetric) ? " for gmetric " : "",
      (T->ThresholdType == mtttGMetric) ? T->ThresholdGMetricType : "",
      T->Threshold);

    MOWriteEvent((void *)T,mxoTrig,mrelTrigThreshold,EventMsg,NULL,NULL);

    if (MSched.DisableThresholdTrigs == TRUE)
      {
      return(FAILURE);
      }
    }

  if (T->O != NULL)
    {
    /* register trigger launch */

    if (T->OType == mxoRsv)
      {
      bmset(&((mrsv_t *)T->O)->Flags,mrfTrigHasFired);

      if ((T->EType == mttEnd) && 
          (!bmisset(&T->InternalFlags,mtifMultiFire)) && 
          !bmisset(&T->SFlags,mtfResetOnModify))
        {
        bmset(&((mrsv_t *)T->O)->Flags,mrfEndTrigHasFired);
        }
      }
    }    /* END if (T->O != NULL) */

  if ((T->Action != NULL) && 
      (T->AType != mtatReserve) && 
      (T->AType != mtatJobPreempt))
    {
    /* set up variable list */

    SetupTrigVarList(T,ActionString,sizeof(ActionString));
    }   /* END if ((T->Action != NULL) && ...) */

  MDB(6,fSTRUCT) MLog("INFO:     taking action %s:%s for %s %s trigger %s\n",
    MTrigActionType[T->AType],
    ActionString,
    MXO[T->OType],
    MTrigType[T->EType],
    (T->Name == NULL) ? "NULL" : T->Name);

  T->LaunchTime = MSched.Time;

  MTrigSetState(T,mtsLaunched);

  switch (T->AType)
    {
    case mtatChangeParam:

      __InitiateChangeParam(T,ActionString,tmpMsg);

      break;

    case mtatInternal:

      __InitiateInternal(T,ActionString,tmpMsg);

      break;

    case mtatMail:

      __InitiateMail(T,ActionString,tmpMsg);

      break;

    case mtatExec:

      __InitiateExec(T,ActionString,tmpMsg);

      break;

    case mtatModify:

      __InitiateModify(T,ActionString,tmpMsg);

      break;

    case mtatQuery:

      __InitiateQuery(T,ActionString,tmpMsg);

      break;

    case mtatJobPreempt:

      __InitiateJobPreempt(T,ActionString,tmpMsg);

      break;

    case mtatReserve:

      __InitiateReserve(T,ActionString,tmpMsg);

      break;

    case mtatSubmit:

      __InitiateSubmit(T,ActionString,tmpMsg);

      break;

    default:

      /* NOT SUPPORTED */

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (AType) */

  MTrigCheckState(T);

  MSched.TrigStats.NumTrigStartsThisIteration++;

  /* allow a job to start once a trigger is launched on it */

  if ((T->OType == mxoJob) &&
      (T->O != NULL))
    {
    mjob_t *J;

    J = (mjob_t *)T->O;

    bmunset(&J->IFlags,mjifWaitForTrigLaunch);
    }

  MDB(7,fSTRUCT) MLog("INFO:     trigger %s launched %s %s %s\n",
    T->TName,
    MXO[T->OType],
    (T->OID != NULL) ? T->OID : "",
    (T->Action != NULL) ? T->Action : "");

  CreateAndSendEventLog(meltTrigStart,T->TName,mxoTrig,(void *)T);

  return(SUCCESS);
  }  /* END MTrigInitiateAction() */
/* END MTrigInitiateAction.c */
