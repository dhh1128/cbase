/* HEADER */

      
/**
 * @file MTrigLaunchInternalAction.c
 *
 * Contains: MTrigLaunchInternalAction
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Launch internal trigger action.
 *
 * @see MTrigInitiateAction() - parent
 * @see MTrigCheckInternalAction() - sibling
 *
 * @param T (I)
 * @param ActionString (I)
 * @param Complete (O) whether the trigger is complete or needs to be updated
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MTrigLaunchInternalAction(

  mtrig_t *T,             /* I (optional) */
  char    *ActionString,  /* I */
  mbool_t *Complete,      /* O */
  char    *Msg)           /* O (optional,minsize=MMAX_LINE) */

  {
  /* FORMAT:  <OTYPE>:<OID>:<ACTION>:<context information> */

  char *ptr = NULL;
  char *TokPtr = NULL;
  char *tmpAString = NULL;

  char ArgLine[MMAX_LINE*10];
  char OID[MMAX_BUFFER];
  char Action[MMAX_LINE*10];

  enum MXMLOTypeEnum OType;

  const char *FName = "MTrigLaunchInternalAction";

  MDB(4,fSCHED) MLog("%s(%s,%s,Msg)\n",
    FName,
    ((T != NULL) && (T->TName != NULL)) ? T->TName : "NULL",
    (ActionString != NULL) ? ActionString : "NULL");

  if (Msg != NULL)
    Msg[0] = '\0';

  if (Complete != NULL)
    *Complete = TRUE;

  if (ActionString == NULL)
    {
    if (Msg != NULL)
      strcpy(Msg,"internal error");

    return(FAILURE);
    }

  MUStrDup(&tmpAString,ActionString);

  /* NOTE: this format and parsing is duplicated in MTrigCheckInternalAction()
           any update here must be copied to that routine */

  ptr = MUStrTok(tmpAString,":",&TokPtr);

  if (ptr == NULL)
    {
    MDB(4,fSCHED) MLog("ERROR:    invalid action");

    if (Msg != NULL)
      strcpy(Msg,"invalid action");

    MUFree(&tmpAString);

    return(FAILURE);
    }

  OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

  if (OType == mxoNONE)
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

  MUStrCpy(OID,ptr,sizeof(OID));

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

  MUStrCpy(Action,ptr,sizeof(Action));

  if ((TokPtr != NULL) && (TokPtr[0] != '\0'))
    MUStrCpy(ArgLine,TokPtr,sizeof(ArgLine));
  else
    ArgLine[0] = '\0';

  MUFree(&tmpAString);

  switch (OType)
    {
    case mxoJob:

      {
      mjob_t *J;

      char *JID;
      char *TokPtr;
 
      /* OID FORMAT:  <JID>[+<JID>]... */

      mbool_t FailureDetected;

      enum MJobCtlCmdEnum atype;

      atype = (enum MJobCtlCmdEnum)MUGetIndexCI(Action,MJobCtlCmds,FALSE,mjcmNONE);

      if (atype == mjcmNONE)
        {
        MDB(4,fSCHED) MLog("ERROR:    cannot parse job action '%s' in %s\n",
          Action,
          FName);

        return(FAILURE);
        }

      FailureDetected = FALSE;

      JID = MUStrTok(OID,"+",&TokPtr);

      while (JID != NULL)
        {
        if (!strcmp(JID,"-"))
          {
          if (T == NULL)
            return(FAILURE);

          J = (mjob_t *)T->O;
          }
        else if (MJobFind(JID,&J,mjsmExtended) == FAILURE)
          {
          MDB(4,fSCHED) MLog("ERROR:    cannot locate job '%s' in %s\n",
            JID,
            FName);

          JID = MUStrTok(NULL,"+",&TokPtr);

          FailureDetected = TRUE;

          continue;
          }

        switch (atype)
          {
          case mjcmCancel:

            {
            char *CCodeCheck;
            char UpperArgLine[MMAX_LINE];

            MDB(7,fSCHED) MLog("  INFO:  Job flaged for cancel by trigger action. '%s' in %s\n",
                JID,
                FName);

            bmset(&J->IFlags, mjifCancel);

            MUStrToUpper(ArgLine,UpperArgLine,MMAX_LINE);

            /* If specified, allow trigger to set the completion code of the job */

            if ((CCodeCheck = strstr(UpperArgLine,"CCODE=")) != NULL)
              {
              CCodeCheck += strlen("CCODE="); /* Skip the "CCODE:" part */

              if (CCodeCheck[0] != '\0')
                {
                J->CompletionCode = strtol(CCodeCheck,NULL,10);
                }
              }
            else
              {
              J->CompletionCode = 273; /* Completion code for a canceled job in TORQUE */
              }
            }  /* END case mjcmCancel */

            break;

          case mjcmComplete:
 
            {
            char *CCodeCheck;
            char UpperArgLine[MMAX_LINE];

            if ((!bmisset(&J->Flags,mjfSystemJob) &&
                 !bmisset(&J->Flags,mjfNoRMStart)))
              {
              MDB(4,fSCHED) MLog("ERROR:    job %s in bad state for complete in %s\n",
                J->Name,
                FName);

              JID = MUStrTok(NULL,"+",&TokPtr);

              FailureDetected = TRUE;

              continue;
              }

            /* the next three lines set the WCLimit to the exact runtime
             * this also causes MS3WorkloadQuery to remove the job */

            J->SpecWCLimit[0] = MSched.Time - J->StartTime;
            J->SpecWCLimit[1] = MSched.Time - J->StartTime;

            J->WCLimit = MSched.Time - J->StartTime;

            if (MJOBISACTIVE(J) == TRUE)
              {
              MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);  /* just removes active rsv? */
              }

            MUStrToUpper(ArgLine,UpperArgLine,MMAX_LINE);

            if ((CCodeCheck = strstr(UpperArgLine,"CCODE=")) != NULL)
              {
              CCodeCheck += strlen("CCODE="); /* Skip the "CCODE:" part */

              if (CCodeCheck[0] != '0')
                {
                J->CompletionCode = strtol(CCodeCheck,NULL,10);
                }
              }

            /* MSched.TrigCompletedJob = TRUE; -- not sure why we wanted to skip the UI phase here... */
            }  /* END BLOCK (case mjcmComplete) */

            break;

          case mjcmModify:

            {
            char *TokPtr;

            char *Attr = NULL;
            char *Value = NULL;
            char *Msg = NULL;

            enum MJobAttrEnum aindex;
            enum MObjectSetModeEnum Op;

            if (strstr(ArgLine,"+="))
              Op = mAdd;
            else if (strstr(ArgLine,"-="))
              Op = mDecr;
            else
              Op = mSet;

            Attr  = MUStrTok(ArgLine,"+-=",&TokPtr);
            Value = MUStrTok(NULL,"=: \t\n",&TokPtr);
            Msg = MUStrTok(NULL,":\t\n",&TokPtr);  /* extract message */

            aindex = (enum MJobAttrEnum)MUGetIndexCI(Attr,MJobAttr,FALSE,mnaNONE);

            switch (aindex)
              {
              case mjaHold:

                if (MJobSetAttr(J,mjaHold,(Value != NULL) ? (void **)Value : (void **)"batch",mdfString,mSet) == FAILURE)
                  {
                  MDB(4,fSCHED) MLog("ERROR:    cannot hold job '%s' in %s\n",
                    JID,
                    FName);

                  JID = MUStrTok(NULL,"+",&TokPtr);

                  FailureDetected = TRUE;

                  continue;
                  }

                if (Msg != NULL)
                  {
                  /* attach provided message */

                  MMBAdd(
                    &J->MessageBuffer,
                    Msg,
                    NULL,
                    mmbtHold,
                    0,
                    0,
                    NULL);
                  }

                if (MJOBISACTIVE(J))
                  {
                  /* Set to idle but don't clear DRM and DRMJID */
                  MJobToIdle(J,FALSE,FALSE);

                  J->SubState = mjsstNONE;
                  }

                /* checkpoint change (make this configurable at this point?) */

                MOCheckpoint(mxoJob,(void *)J,FALSE,NULL);
          
                break;

              case mjaVariables:

                {
                char tmpLine[MMAX_LINE*2];

                snprintf(tmpLine,sizeof(tmpLine),"%s=%s",
                  Value,
                  Msg);

                if (MJobSetAttr(J,mjaVariables,(void **)tmpLine,mdfString,Op) == FAILURE)
                  {
                  MDB(4,fSCHED) MLog("ERROR:    cannot set trigvar job '%s' in %s\n",
                    JID,
                    FName);

                  JID = MUStrTok(NULL,"+",&TokPtr);

                  FailureDetected = TRUE;

                  continue;
                  }
                }  /* END BLOCK */

                break;

              default:

                if (!strcasecmp(Attr,"gres"))
                  {
                  if ((Op == mAdd) && 
                      (MJobAddRequiredGRes(
                         J,
                         Value,
                         1,
                         mxaGRes,
                         FALSE,
                         FALSE) == SUCCESS))
                    {
                    /* SUCCESS - NO-OP */
                    }
                  else if ((Op == mDecr) &&
                           (MJobRemoveGRes(J,Value,TRUE) == SUCCESS))
                    {
                    /* SUCCESS -- Do Nothing */
                    }
                  else
                    {
                    MDB(4,fSCHED) MLog("ERROR:    cannot adjust gres for job '%s' in %s\n",
                      JID,
                      FName);

                    JID = MUStrTok(NULL,"+",&TokPtr);

                    FailureDetected = TRUE;

                    continue;
                    }
                  }
                else
                  {
                  MDB(4,fSCHED) MLog("ERROR:    cannot modify attribute '%s' for job '%s' in %s\n",
                    Attr,
                    JID,
                    FName);

                  JID = MUStrTok(NULL,"+",&TokPtr);

                  FailureDetected = TRUE;

                  continue;
                  }

                break;
              }
            }   /* END BLOCK (case mjcmModify) */

            break;

          case mjcmSignal:

            {
            char  EMsg[MMAX_LINE];
            int   SC;

            if (MRMJobSignal(J,-1,ArgLine,EMsg,&SC) == FAILURE)
              {
              MDB(4,fSCHED) MLog("ERROR:    cannot send signal '%s' to job '%s' in %s\n",
                ArgLine,
                JID,
                FName);

              JID = MUStrTok(NULL,"+",&TokPtr);

              FailureDetected = TRUE;

              continue;
              }
            }    /* END BLOCK (case mjcmSignal) */

            break;

          case mjcmStart:

            /* FIXME:  check reservation, check allocation */

            {
            J->SubState = mjsstNONE;

            /* set "copy-tasklist" flag */

            bmset(&J->IFlags,mjifCopyTaskList);

            if (MRMJobStart(J,NULL,NULL) == FAILURE)
              {
              MDB(4,fSCHED) MLog("ERROR:    cannot start job '%s' in %s\n",
                JID,
                FName);

              JID = MUStrTok(NULL,"+",&TokPtr);

              FailureDetected = TRUE;

              continue;
              }

            J->State = mjsRunning;
            }   /* END BLOCK (case mjcmStart) */

            break;

         case mjcmRequeue:

            /* FIXME:  check reservation, check allocation */

            {
            char EMsg[MMAX_LINE];

            J->SubState = mjsstNONE;

            /* set "copy-tasklist" flag */

            if (MJobRequeue(J,NULL,"workflow failure",EMsg,NULL) == FAILURE)
              {
              MDB(4,fSCHED) MLog("ERROR:    cannot requeue job '%s' in %s\n",
                JID,
                FName);

              JID = MUStrTok(NULL,"+",&TokPtr);

              FailureDetected = TRUE;

              continue;
              }

            J->State = mjsIdle;
            }   /* END BLOCK (case mjcmStart) */

          default:

            MDB(4,fSCHED) MLog("ERROR:    action '%s' not handled in %s\n",
              Action,
              FName);

            return(FAILURE);

            /*NOTREACHED*/

            break;
          }  /* END switch (atype) */
   
        JID = MUStrTok(NULL,"+",&TokPtr);
        }    /* END while (JID != NULL) */

      if (FailureDetected == TRUE)
        {
        return(FAILURE);
        }
      }  /* END BLOCK (case mxoJob) */

      break;

    case mxoNode:

      {
      enum MNodeCtlCmdEnum atype;
      char *rsvProfile = NULL;

      atype = (enum MNodeCtlCmdEnum)MUGetIndexCI(Action,MNodeCtlCmds,FALSE,mncmNONE);

      if (!strcasecmp(Action,"reserve"))
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"auto-trigger-rsv.%s",
          T->TName);

        if (ArgLine[0] != '\0')
          {
          char *Attr  = MUStrTok(ArgLine,"=",&TokPtr);
          char *Value = MUStrTok(NULL,"= \t\n",&TokPtr);

          if ((Attr != NULL) && 
              (strncasecmp(Attr,"rsvprofile",strlen("rsvprofile")) == 0))
            rsvProfile = Value;
          }

        if (T->OType != mxoRsv)
          {
          MNodeReserve(
            (mnode_t *)T->O,
            MCONST_EFFINF,
            tmpLine,
            rsvProfile,
            "Reservation created by trigger");

          return(SUCCESS);
          }
        else
          {
          return(FAILURE);
          }
        }
      else if ((!strcasecmp(Action,"evacvms")) && (MSched.AllowVMMigration == TRUE))
        {
        /* Evacuate all vms on nodes held by this reservation */

        char tmpMsg[MMAX_LINE];
        int rc = SUCCESS;

        mnl_t NL;

        MNLInit(&NL);

        MNLFromString(OID,&NL,1,FALSE);

        rc = MNodeEvacVMsMultiple(&NL);

        MNLFree(&NL);

        /* Write out events */

        snprintf(tmpMsg,sizeof(tmpMsg),"attempting to migrate vms from node '%s' for evacvms reservation '%s' (TID: %s)",
          OID,
          ((mrsv_t *)T->O)->Name,
          (T->TName != NULL) ? T->TName : "NULL");

        MDB(5,fSTRUCT) MLog("INFO:     %s\n",
          tmpMsg);

        return(rc);
        } /* END if (!strcasecmp(Action,"evacvms")) */

      if (atype == mncmNONE)
        {
        return(FAILURE);
        }

      switch (atype)
        {
        case mncmModify:
 
          if (MSched.AggregateNodeActions == FALSE)
            {
            char *ptr;
            char *TokPtr;
   
            mnode_t *N;
   
            char *Attr;
            char *Value;
   
            enum MNodeAttrEnum aindex;

            /* always look on the bright side of life */
            mbool_t NodeModifySuccessDetected;
   
            /* FORMAT:  <ARG>=<VALUE> */
   
            Attr  = MUStrTok(ArgLine,"=",&TokPtr);
            Value = MUStrTok(NULL,"= \t\n",&TokPtr);
   
            aindex = (enum MNodeAttrEnum)MUGetIndexCI(Attr,MNodeAttr,FALSE,mnaNONE);

            NodeModifySuccessDetected = FALSE;
   
            /* FORMAT: OID = node[,node]... */
   
            ptr = MUStrTok(OID,",",&TokPtr);
   
            if ((ptr == NULL) || (aindex == mnaNONE))
              {
              MDB(4,fSCHED) MLog("ERROR:    unknown attribute to modify on node '%s'\n",
                Attr);

              return(FAILURE);
              }
   
            while (ptr != NULL)
              {
              if (MNodeFind(ptr,&N) == FAILURE)
                {
                ptr = MUStrTok(NULL,",",&TokPtr);
   
                continue;
                }
   
              switch (aindex)
                {
                case mnaOS:
   
                  {
                  char EMsg[MMAX_LINE];
   
                  if (!strcmp(Value,MAList[meOpsys][0]))
                    {
                    if ((N->PowerSelectState == mpowOff) &&
                      (MNodeSetAttr(N,mnaPowerIsEnabled,(void **)"true",mdfString,mVerify) == FAILURE))
                      {
                      if (Msg != NULL)
                        MUStrCpy(Msg,EMsg,MMAX_LINE);

                      /* do not return failure just because one provisioning job fails */
                      }
                    }
                  else if (MNodeProvision(N,NULL,NULL,Value,TRUE,EMsg,NULL) == FAILURE)
                    {
                    if (Msg != NULL)
                      MUStrCpy(Msg,EMsg,MMAX_LINE);

                    /* do not return failure just because one provisioning job fails */
                    }
                  else 
                    {
                    NodeModifySuccessDetected = TRUE;
                    }
                  }
   
                  break;
   
                case mnaPowerIsEnabled:

                  /* NOTE: do not return failure just because node modify fails
                   * the routine will fail if all node modifies fail */

                  if (!strcmp(Value,"on"))
                    {
                    /* power node on */
   
                    if (MNodeSetAttr(N,mnaPowerIsEnabled,(void **)"true",mdfString,mVerify) == SUCCESS)
                      {
                      NodeModifySuccessDetected = TRUE;
                      }
                    }
                  else if (!strcasecmp(Value,"off"))
                    {
                    /* power node off */
   
                    if (MNodeSetAttr(N,mnaPowerIsEnabled,(void **)"false",mdfString,mVerify) == SUCCESS)
                      {
                      NodeModifySuccessDetected = TRUE;
                      }
                    }
                  else if (!strcasecmp(Value,"reset"))
                    {
                    char EMsg[MMAX_LINE];
   
                    /* reset node */
   
                    if (MNodeReboot(N,NULL,EMsg,NULL) == FAILURE)
                      {
                      if (Msg != NULL)
                        MUStrCpy(Msg,EMsg,MMAX_LINE);
                      }
                    else 
                      {
                      NodeModifySuccessDetected = TRUE;
                      }
                    }
                  else if (!strcasecmp(Value,"on+"))
                    {
                    /* power node on, but trigger isn't finished until action is complete */
                  
                    if (Complete != NULL)
                      *Complete = FALSE;
   
                    if (T != NULL)
                      MUStrDup(&T->CompleteAction,ActionString);
   
                    MNodeSetAttr(N,mnaPowerIsEnabled,(void **)"true",mdfString,mVerify);

                    T->PID = 1;
                    }
                  else if (!strcasecmp(Value,"off+"))
                    {
                    /* power node off, but trigger isn't finished until action is complete */
   
                    if (T != NULL)
                      MUStrDup(&T->CompleteAction,ActionString);
   
                    if (Complete != NULL)
                      *Complete = FALSE;
   
                    MNodeSetAttr(N,mnaPowerIsEnabled,(void **)"false",mdfString,mVerify);

                    T->PID = 1;
                    }
   
                  break;
      
                default:
   
                  return(FAILURE);
   
                  /*NOTREACHED*/
   
                  break;
                }   /* END switch (aindex) */
   
              ptr = MUStrTok(NULL,",",&TokPtr);
              }  /* END while (ptr != NULL) */

            /* if none of the node modify requests (non-aggregating) succeeded, 
               then return failure.  (success just means it was submitted
               at this stage) */

            if ((aindex == mnaOS) && 
                (NodeModifySuccessDetected == FALSE))
              {
              return (FAILURE);
              }
            }  /* END if (MSched.AggregateNodeActions == FALSE) */
          else
            {
            mnl_t NL;

            char *Attr;
            char *Value;
   
            enum MNodeAttrEnum aindex;

            int nindex;
   
            /* FORMAT:  <ARG>=<VALUE> */
   
            Attr  = MUStrTok(ArgLine,"=",&TokPtr);
            Value = MUStrTok(NULL,"= \t\n",&TokPtr);
   
            aindex = (enum MNodeAttrEnum)MUGetIndexCI(Attr,MNodeAttr,FALSE,mnaNONE);
   
            /* FORMAT: OID = node[,node]... */
   
            if (aindex == mnaNONE)
              {
              return(FAILURE);
              }

            MNLInit(&NL);

            MNLFromString(OID,&NL,-1,FALSE);

            /* NOTE: in the node aggregating case, return failure when there is an error 
             * performing node modify operations (e.g. communicating with the RM) */
   
            switch (aindex)
              {
              case mnaNodeState:

                {
                enum MStatusCodeEnum SC;
                char EMsg[MMAX_LINE];

                if (MNodeModify(NULL,OID,mnaNodeState,Value,EMsg,&SC) == FAILURE)
                  {
                  MDB(2,fRM) MLog("WARNING:  cannot modify state of '%s' = %s\n",
                    OID,
                    EMsg);
           
                  return(FAILURE);
                  }
                }  /* END case mnaNodeState */

                break;

              case mnaOS:
 
                {
                char EMsg[MMAX_LINE];
 
                if (MNLGetemReady(&NL,Value,EMsg) == FAILURE)
                  {
                  /* NYI: append EMsg to Msg */
 
                  if (Msg != NULL)
                    MUStrCpy(Msg,EMsg,MMAX_LINE);

                  MNLFree(&NL);

                  return (FAILURE);
                  }
                }  /* END BLOCK case mnaOS */
 
                break;
 
              case mnaPowerIsEnabled:

                {
                mnode_t *N;

                for (nindex = 0;MNLGetNodeAtIndex(&NL,nindex,&N) == SUCCESS;nindex++)
                  {
                  if (N == MSched.GN)
                    continue;

                  if ((!strcasecmp(Value,"on")) ||
                      (!strcasecmp(Value,"on+")))
                    {
                    /* power node on */
           
                    if ((N->PowerSelectState == mpowOn) ||
                        (MNodeGetSysJob(N,msjtPowerOn,MBNOTSET,NULL) == SUCCESS))
                      {
                      /* skip this node */

                      MDB(5,fSCHED) MLog("INFO:   not powering node '%s' on--power select state is already 'ON'\n",
                        N->Name);

                      MNLRemove(&NL,N);

                      continue;
                      }
                    }
                  else if ((!strcasecmp(Value,"off")) ||
                           (!strcasecmp(Value,"off+")))
                    {
                    /* power node off */
           
                    if ((N->PowerSelectState == mpowOff) ||
                        (MNodeGetSysJob(N,msjtPowerOff,TRUE,NULL) == SUCCESS))
                      {
                      /* skip this node */

                      MDB(5,fSCHED) MLog("INFO:   not powering node '%s' off--power select state is already 'OFF'\n",
                        N->Name);

                      MNLRemove(&NL,N);

                      continue;
                      }
                    }
                  }  /* END for (nindex) */
                }    /* END BLOCK */

                if (!strcmp(Value,"on"))
                  {
                  /* power node on */
 
                  if (MNLSetAttr(&NL,mnaPowerIsEnabled,(void **)"true",mdfString,mVerify) == FAILURE)
                    {
                    MNLFree(&NL);

                    return (FAILURE);
                    }
                  }
                else if (!strcasecmp(Value,"off"))
                  {
                  /* power node off */
 
                  if (MNLSetAttr(&NL,mnaPowerIsEnabled,(void **)"false",mdfString,mVerify) == FAILURE)
                    {
                    MNLFree(&NL);

                    return (FAILURE);
                    }
                  }
                else if (!strcasecmp(Value,"reset"))
                  {
                  char EMsg[MMAX_LINE];
 
                  /* reset node */
 
                  if (MNodeReboot(NULL,&NL,EMsg,NULL) == FAILURE)
                    {
                    MNLFree(&NL);

                    /* NYI: append EMsg to Msg */
 
                    if (Msg != NULL)
                      MUStrCpy(Msg,EMsg,MMAX_LINE);

                    return (FAILURE);
                    }
                  }
                else if (!strcasecmp(Value,"on+"))
                  {
                  /* power node on, but trigger isn't finished until action is complete */

                  if (MNLSetAttr(&NL,mnaPowerIsEnabled,(void **)"true",mdfString,mVerify) == FAILURE)
                    {
                    MNLFree(&NL);

                    if (Complete != NULL)
                      *Complete = TRUE;

                    return(FAILURE);                   
                    }

                    if (Complete != NULL)
                      *Complete = FALSE;
 
                  if (T != NULL)
                    MUStrDup(&T->CompleteAction,ActionString);

                  T->PID = 1;
                  }
                else if (!strcasecmp(Value,"off+"))
                  {
                  /* power node off, but trigger isn't finished until action is complete */
 
                  if (MNLSetAttr(&NL,mnaPowerIsEnabled,(void **)"false",mdfString,mVerify) == FAILURE)
                    {
                    MNLFree(&NL);

                    if (Complete != NULL)
                      *Complete = TRUE;

                    return(FAILURE);                   
                    }

                    if (Complete != NULL)
                      *Complete = FALSE;
 
                  if (T != NULL)
                    MUStrDup(&T->CompleteAction,ActionString);

                  T->PID = 1;
                  }
 
                break;
    
              default:
 
                MNLFree(&NL);

                return (FAILURE);
 
                /*NOTREACHED*/
 
                break;
              }   /* END switch (aindex) */
            }     /* END else */

          break;

        case mncmCreate:

          {
          int rc;

          mnode_t *N;


          char EMsg[MMAX_LINE];
          char tmpLine[MMAX_LINE*10];

          /* FORMAT:  COUNT:OS */

          if (MNodeFind(OID,&N) == FAILURE)
            {
            if (Msg != NULL)
              snprintf(Msg,MMAX_LINE,"cannot locate hostlist '%s'",OID);

            return(FAILURE);
            }

          /* FORMAT:  COUNT:OS */

          snprintf(tmpLine,sizeof(tmpLine),"%s:%s%s%s",
            N->Name,
            ArgLine,
            ((T->OType == mxoJob) && ((mjob_t *)T->O != NULL)) ? ":operationid=" : "",
            ((T->OType == mxoJob) && ((mjob_t *)T->O != NULL)) ? ((mjob_t *)T->O)->Name : "");

          mstring_t Output(MMAX_LINE);

          rc = MRMVMCreate(tmpLine,&Output,EMsg,NULL);

          if (rc == FAILURE)
            {
            if (Msg != NULL)
              MUStrCpy(Msg,EMsg,MMAX_LINE);

            MUStrCpy(Msg,Output.c_str(),MMAX_LINE);
            }

          return(rc);
          }  /* END BLOCK (case mncmCreate) */

          break;

        case mncmMigrate:

          {
          int rc;

          char *TokPtr = ArgLine;

          char      *VMName;
          char      *SrcNodeName;
          char      *DestNodeName;
          mnode_t   *SrcNode;
          mnode_t   *DestNode;
          mln_t     *VMLink;
          char      *VMList[2];
          mnl_t      DestNL;

          /* FORMAT:  <VM>:<DestNode>,... */

          TokPtr = ArgLine;

          VMName  = MUStrTok(NULL,":=",&TokPtr);
          DestNodeName = MUStrTok(NULL,":=",&TokPtr);

          SrcNodeName  = OID;

          if ((DestNodeName == NULL) || (VMName == NULL) ||(SrcNodeName) == NULL)
            {
            return(FAILURE);
            }

          if (MNodeFind(SrcNodeName,&SrcNode) == FAILURE)
            {
            return(FAILURE);
            }

          if (MNodeFind(DestNodeName,&DestNode) == FAILURE)
            {
            return(FAILURE);
            }

          if (MULLCheck(SrcNode->VMList,VMName,&VMLink) == FAILURE)
            {
            /* Should we be checking this here? */

            /* return(FAILURE); */
            }

          MNLInit(&DestNL);

          VMList[0] = VMName;

          MNLSetNodeAtIndex(&DestNL,0,DestNode);
          MNLSetTCAtIndex(&DestNL,0,1);
          MNLTerminateAtIndex(&DestNL,1);

          VMList[1] = NULL;

          rc = MRMJobMigrate((mjob_t *)T->O,VMList,&DestNL,Msg,NULL);

          MNLFree(&DestNL);

          return(rc);
          }  /* END BLOCK (case mncmMigrateVM) */

          break;

        default:

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }
      }   /* END BLOCK (case mxoNode) */

      break;

    case mxoxJobGroup:

      {
      job_iter JTI;

      marray_t JArray;

      int jindex;

      mjob_t *J;

      char *JID;
      char *ptr;
      char *TokPtr;
 
      /* OID FORMAT:  <JID>[+<JID>]... */

      mbool_t FailureDetected;

      enum MJobCtlCmdEnum atype;

      char *Attr = NULL;
      char *Value = NULL;
      char *Msg = NULL;

      enum MJobAttrEnum aindex = mjaNONE;

      atype = (enum MJobCtlCmdEnum)MUGetIndexCI(Action,MJobCtlCmds,FALSE,mjcmNONE);

      if (atype == mjcmNONE)
        {
        MDB(4,fSCHED) MLog("ERROR:    cannot parse job action '%s' in %s\n",
          Action,
          FName);

        return(FAILURE);
        }

      if (atype == mjcmModify)
        {
        char *TokPtr;

        Attr  = MUStrTok(ArgLine,"+-=",&TokPtr);
        Value = MUStrTok(NULL,"=: \t\n",&TokPtr);
        Msg = MUStrTok(NULL,":\t\n",&TokPtr);  /* extract message */

        aindex = (enum MJobAttrEnum)MUGetIndexCI(Attr,MJobAttr,FALSE,mnaNONE);
        }

      FailureDetected = FALSE;

      JID = MUStrTok(OID,"+",&TokPtr);

      while (JID != NULL)
        {
        MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

        if (!strcmp(JID,"-"))
          {
          if (T == NULL)
            return(FAILURE);

          ptr = T->OID;
          }
        else
          {
          ptr = JID;
          }

        MJobIterInit(&JTI);

        while (MJobTableIterate(&JTI,&J) == SUCCESS)
          {
          if ((J->JGroup != NULL) && (J->JGroup->Name != NULL) && (!strcmp(ptr,J->JGroup->Name)))
            {
            MUArrayListAppend(&JArray,&J);
            }
          }

        if (MUArrayListGet(&JArray,0) == NULL)
          {
          MDB(4,fSCHED) MLog("ERROR:    cannot locate job '%s' in %s\n",
            JID,
            FName);

          JID = MUStrTok(NULL,"+",&TokPtr);

          FailureDetected = TRUE;

          continue;
          }

        for (jindex = 0;jindex < JArray.NumItems;jindex++)
          {
          J = *(mjob_t **)MUArrayListGet(&JArray,jindex);
       
          if (J == NULL)
            continue;

          switch (atype)
            {
            case mjcmCancel:
   
              {
              char *CCodeCheck;
              char UpperArgLine[MMAX_LINE];
   
              if (MJobCancel(J,NULL,FALSE,Msg,NULL) == FAILURE)
                {
                MDB(4,fSCHED) MLog("ERROR:    cannot cancel job '%s' in %s\n",
                  JID,
                  FName);
   
                JID = MUStrTok(NULL,"+",&TokPtr);
   
                FailureDetected = TRUE;
   
                continue;
                }
   
              MUStrToUpper(ArgLine,UpperArgLine,MMAX_LINE);
   
              /* If specified, allow trigger to set the completion code of the job */
   
              if ((CCodeCheck = strstr(UpperArgLine,"CCODE=")) != NULL)
                {
                CCodeCheck += strlen("CCODE="); /* Skip the "CCODE:" part */
   
                if (CCodeCheck[0] != '\0')
                  {
                  J->CompletionCode = strtol(CCodeCheck,NULL,10);
                  }
                }
              else
                {
                J->CompletionCode = 273; /* Completion code for a canceled job in TORQUE */
                }
              }  /* END case mjcmCancel */
   
              break;
   
            case mjcmModify:

              switch (aindex)
                {
                case mjaHold:

                  if (MJobSetAttr(
                        J,
                        mjaHold,
                        (Value != NULL) ? (void **)Value : (void **)"batch",
                        mdfString,
                        mSet) == FAILURE)
                    {
                    MDB(4,fSCHED) MLog("ERROR:    cannot hold job '%s' in %s\n",
                      JID,
                      FName);

                    JID = MUStrTok(NULL,"+",&TokPtr);

                    FailureDetected = TRUE;

                    continue;
                    }

                  if (Msg != NULL)
                    {
                    /* attach provided message */

                    MMBAdd(
                      &J->MessageBuffer,
                      Msg,
                      NULL,
                      mmbtHold,
                      0,
                      0,
                      NULL);
                    }

                  if (J->SubState == mjsstProlog)
                    {
                    J->SubState = mjsstNONE;
                    }

                  /* checkpoint change (make this configurable at this point?) */

                  MOCheckpoint(mxoJob,(void *)J,FALSE,NULL);
          
                  break;

                default:

                  MDB(4,fSCHED) MLog("ERROR:    cannot modify attribute '%s' for job '%s' in %s\n",
                    Attr,
                    JID,
                    FName);

                  JID = MUStrTok(NULL,"+",&TokPtr);

                  FailureDetected = TRUE;

                  break;
                }  /* END switch (aindex) */

                break;

              default:
   
                MDB(4,fSCHED) MLog("ERROR:    action '%s' not handled in %s\n",
                  Action,
                  FName);
   
                return(FAILURE);
   
                /*NOTREACHED*/
   
                break;
            }  /* END switch (atype) */
          }    /* END for (jindex) */

        JID = MUStrTok(NULL,"+",&TokPtr);

        MUArrayListFree(&JArray);
        }    /* END while (JID != NULL) */

      if (FailureDetected == TRUE)
        {
        return(FAILURE);
        }
      }  /* END BLOCK (case mxoxJobGroup) */

      break;

    case mxoSched:

      {
      enum MSchedCtlCmdEnum atype;

      atype = (enum MSchedCtlCmdEnum)MUGetIndexCI(Action,MSchedCtlCmds,FALSE,msctlNONE);

      switch (atype)
        {
        case msctlVMMigrate:

          {
          enum MVMMigrationPolicyEnum aindex;
          aindex = (enum MVMMigrationPolicyEnum)MUGetIndexCI(ArgLine,MVMMigrationPolicy,FALSE,mvmmmNONE);

          return(MUIVMCtlPlanMigrations(aindex,FALSE,FALSE,NULL));
          }
  
        default:

          return(FAILURE);

        } /* END switch (atype) */
      }   /* END case mxoSched */

      break;

    case mxoTrig:

      {
      enum MSchedCtlCmdEnum atype;

      atype = (enum MSchedCtlCmdEnum)MUGetIndexCI(Action,MSchedCtlCmds,FALSE,msctlNONE);

      if (atype == msctlNONE)
        {
        return(FAILURE);
        }

      switch (atype)
        {
        case msctlModify:

          {
          char *TokPtr;

          mtrig_t *T;

          char *Attr;
          char *Value;

          enum MTrigAttrEnum aindex;

          Attr  = MUStrTok(ArgLine,"=",&TokPtr);
          Value = MUStrTok(NULL,"\"",&TokPtr);

          /* FORMAT: OID = trigid */

          aindex = (enum MTrigAttrEnum)MUGetIndexCI(Attr,MTrigAttr,FALSE,mtaNONE);

          if ((OID == NULL) || (OID[0] == '\0') || (aindex == mtaNONE))
            {
            return(FAILURE);
            }

          if (MTrigFind(OID,&T) == FAILURE)
            {
            return(FAILURE);
            }

          switch (aindex)
            {
            case mtaActionData:

              if (MTrigSetAttr(T,aindex,Value,mdfString,mSet) == FAILURE)
                {
                return(FAILURE);
                }

              break;

            default:

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }  /* END switch (aindex) */

          break;
          }     /* END case msctlModify */

        default:

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }       /* END switch (atype) */
      }         /* END BLOCK (case mxoTrig) */

      break;

    case mxoRM:

      {
      mrm_t  *R = NULL;
      char *RMID;
      char *TokPtr;
      mbool_t FailureDetected;

      enum MRMCtlCmdEnum atype;

      atype = (enum MRMCtlCmdEnum)MUGetIndexCI(Action,MRMCtlCmds,FALSE,mrmctlNONE);

      if (atype == mrmctlNONE)
        {
        MDB(4,fSCHED) MLog("ERROR:    cannot parse rm action '%s' in %s\n",
        Action,
        FName);

        return(FAILURE);
        }

      FailureDetected = FALSE;

      RMID = MUStrTok(OID,"+",&TokPtr);

      while (RMID != NULL)
        {
        if (!strcmp(RMID, "-"))
          {
          if (T == NULL)
            return(FAILURE);

          R = (mrm_t *)T->O;
          }
        else if (MRMFind(RMID,&R) == FAILURE) 
          {
          if (Msg != NULL)
            strcpy(Msg,"cannot locate specified RM");

          RMID = MUStrTok(NULL,"+",&TokPtr);

          FailureDetected = TRUE;

          continue;
          }

        switch (atype)
          {
          case mrmctlModify:

            {
            char   *TokPtr;

            enum MRMAttrEnum aindex;

            char *Attr;
            char *Value;

            /* FORMAT: <attr>=<value> */

            Attr  = MUStrTok(ArgLine,"=",&TokPtr);
            Value = MUStrTok(NULL,"\"",&TokPtr);

            aindex = (enum MRMAttrEnum)MUGetIndexCI(Attr,MRMAttr,FALSE,mrmaNONE);

            switch (aindex)
              {
              case mrmaState:
  
                return(MRMSetAttr(R,aindex,(void **)Value,mdfString,mSet));

                break;

              default:

                if (Msg != NULL)
                  strcpy(Msg,"cannot modify specified attribute");
   
                return(FAILURE);

                /* NOTREACHED */

                break;
              } /* END switch (aindex) */
            }     /* END BLOCK (case mrmctlModify) */

            break;

          case mrmctlDestroy:

            MDB(3,fUI) MLog("WARNING:  cannot destroy static RM\n");
                                                                                      
            if (Msg != NULL)
              strcpy(Msg,"WARNING:  cannot destroy static RM\n");
                                                                                      
            return(FAILURE);
 
            /* NOTREACHED */
  
            break;

          case mrmctlPurge:

            return(MRMPurge(R));

            break;

          default:

            if (Msg != NULL)
              strcpy(Msg,"action not supported for RM object");

            return(FAILURE);

            /*NOTREACHED*/

            break;
          } /* END switch (atype) */

        RMID = MUStrTok(NULL,"+",&TokPtr);
        } /* END while (RMID != NULL */  

      if (FailureDetected == TRUE)
        {
        return(FAILURE);
        }
      } /* END BLOCK (case mxoRM) */

      break;

    case mxoRsv:
 
      {
      enum MRsvCtlCmdEnum atype;

      atype = (enum MRsvCtlCmdEnum)MUGetIndexCI(Action,MRsvCtlCmds,FALSE,mrcmNONE);

      if (atype == mrcmNONE)
        {
        return(FAILURE);
        }

      switch (atype)
        {
        case mrcmModify:

          {
          char *ptr;
          char *TokPtr;

          enum MRsvAttrEnum attr;

          mrsv_t *R;

          if (!strcmp(OID,"-"))
            {
            if (T == NULL)
              return(FAILURE);

            R = (mrsv_t *)T->O;
            }
          else if (MRsvFind(OID,&R,mraNONE) == FAILURE)
            {
            return(FAILURE);
            }

          ptr = MUStrTok(ArgLine,":",&TokPtr);

          attr = (enum MRsvAttrEnum)MUGetIndexCI(ptr,MRsvAttr,FALSE,mraNONE);

          switch (attr)
            {
            case mraACL:

              {
              enum MObjectSetModeEnum Mode;

              if (MUStrIsEmpty(TokPtr))
                Mode = mSet;
              else if (strstr(TokPtr,"-=") != NULL)
                Mode = mDecr;
              else if (strstr(TokPtr,"+=") != NULL)
                Mode = mAdd;
              else
                Mode = mSet;
                
              MRsvSetAttr(R,mraACL,(void *)TokPtr,mdfString,Mode);
              }

              break;

            default:

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }
          }  /* END case mrcmModify */

          break;

        case mrcmDestroy:

          {
          mrsv_t *R;

          if (!strcmp(OID,"-"))
            {
            if (T == NULL)
              return(FAILURE);

            R = (mrsv_t *)T->O;
            }
          else if (MRsvFind(OID,&R,mraNONE) == FAILURE)
            {
            return(FAILURE);
            }

          R->CancelIsPending = TRUE;

          R->EndTime = MSched.Time - 1;
          R->ExpireTime = 0;
   
          /* remove reservation */
   
          MRsvCheckStatus(NULL);
   
          if ((long)R->StartTime <= MUIDeadLine)
            {
            /* adjust UI phase wake up time */
   
            if (MSched.AdminEventAggregationTime >= 0)
              MUIDeadLine = MIN(MUIDeadLine,(long)MSched.Time + MSched.AdminEventAggregationTime);
            }
          }  /* END case mrcmDestroy */

          break;

        case mrcmOther:
 
          {
          mrsv_t *R;

          if (!strcmp(OID,"-"))
            {
            if (T == NULL)
              {
              return(FAILURE);
              }

            R = (mrsv_t *)T->O;
            }
          else if (MRsvFind(OID,&R,mraNONE) == FAILURE)
            {
            return(FAILURE);
            }

          /* parse argline */

          if (strstr(ArgLine,"submit") != NULL)
            {
            MJobSubmitFromJSpec(R->SpecJ,NULL,Msg);
            }  /* END if (strstr(ArgLine,"submit") != NULL) */
          }    /* END BLOCK (case mrcmOther) */

          break;

        default:

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (atype) */
      }    /* END BLOCK (case mxoRsv) */

      break;

    case mxoxVPC:

      {
      mpar_t *VPC;

      MVPCFind(OID,&VPC,TRUE);

      if (!strcasecmp(Action,"destroy"))
        {
        MVPCDestroy(&VPC,ArgLine);
        }
      else
        {
        return(FAILURE);
        }
      }  /* END case mxoxVPC */

      break;

    default:

      if (Msg != NULL)
        strcpy(Msg,"unsupported trigger object type");

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (OType) */

  /*NOTE: return may (and does) return in above switch statement */
  return(SUCCESS);
  }  /* END MTrigLaunchInternalAction() */
/* END MTrigLaunchInternalAction.c */
