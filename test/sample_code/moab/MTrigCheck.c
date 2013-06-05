/* HEADER */

      
/**
 * @file MTrigCheck.c
 *
 * Contains: Trigger Check functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/* Local prototypes */
int __SetTriggerStateByECPolicy(mtrig_t *);
int __KillTimedOutTrigger(mtrig_t *,int);

/**
 * Check the state of a given trigger. This function is only run on a trigger
 * after it has been launched!
 *
 * TODO:  handle restart where trigger has completed during moab down 
 *
 * @param T (I)
 */

int MTrigCheckState(

  mtrig_t *T) /* I */

  {
  int StatLoc = 0;
  int Flags;
  int pid = 0;
  mpid_t *TrigPID;

  enum MTrigStateEnum ExitCode;

  mbool_t TriggerExited = FALSE;

  Flags = WNOHANG;

  if (T->LaunchTime == 0)       
    {
    /* trigger was never launched--this function is only for triggers
     * that have been launched! */

    return(FAILURE);
    }

  if (bmisset(&T->InternalFlags,mtifIsHarvested))
    {
    if (MUHTGetInt(&TrigPIDs,T->PID,(void **)&TrigPID,NULL) == SUCCESS)
      {
      pid = TrigPID->PID;
      StatLoc = TrigPID->StatLoc;
      }
    else
      {
      pid = T->PID;
      StatLoc = T->StatLoc;
      }

    MDB(7,fSTRUCT) MLog("INFO:    processed harvesting of trigger '%s' with PID '%d'\n",
      (T->Name == NULL) ? T->TName : T->Name,
      T->PID);
    }
  else if (T->AType == mtatInternal)
    {
    mbool_t Complete = FALSE;

    if ((MTrigCheckInternalAction(T,T->CompleteAction,&Complete,NULL) == SUCCESS) &&
        (Complete == TRUE))
      {
      /* internal action has completed */

      pid = 1;
   
      T->State = mtsSuccessful;
      }
    else
      {
      pid = 0;
      }
    }
  else if (bmisset(&T->SFlags,mtfAsynchronous))
    {
    /* only continue in this routine if the trigger's state has been updated (via mschedctl) */

    if ((T->State != mtsSuccessful) && (T->State != mtsFailure))
      {
      T->State = mtsActive;

      return(SUCCESS);
      }
    }
  else
    {
    pid = waitpid(T->PID,&StatLoc,Flags);
    }

  /* remove from hash table just in case */

  if (pid > 0)
    {
    MDB(7,fSTRUCT) MLog("INFO:    processed trigger '%s' with PID %d via waitpid()\n",
      (T->Name == NULL) ? T->TName : T->Name,
      T->PID);

    MUHTRemoveInt(&TrigPIDs,pid,MUFREE);
    }

  if ((T->State == mtsSuccessful) || (T->State == mtsFailure))
    {
    /* trigger has completed--mark it as done and clean-up */

    TriggerExited = TRUE;
    bmset(&T->InternalFlags,mtifIsComplete);

    T->LastExecStatus = T->State;

    T->EndTime    = MSched.Time;

    if ((T->OBuf != NULL) && (MTrigLoadExitValue(T,&ExitCode) == SUCCESS))
      T->State = ExitCode;

    MDB(7,fSTRUCT) MLog("trigger '%s' with PID '%d' has completed without waitpid()\n",
      T->Name,
      T->PID);

    MTrigRegisterEvent(T);
    }
  else if ((pid == 0) &&
           (T->Timeout > 0) &&
           (MSched.Time - T->LaunchTime > (unsigned int)T->Timeout))
    {
    /* trigger has run too long, kill and cleanup */

    __KillTimedOutTrigger(T,pid);

    TriggerExited = TRUE;
    }  /* END if ((pid == 0) ...) */
  else if (pid == T->PID)
    {
    /* trigger completed - check exit status */

    TriggerExited = TRUE;

    if ((T->OBuf != NULL) && (MTrigLoadExitValue(T,&ExitCode) == SUCCESS))
      {
      /* exit code and last execution trigger information is displayed on
         the job with the checkjob command */

      T->State = ExitCode;

      /* code added for RT 6911*/

      bmset(&T->InternalFlags,mtifIsComplete);

      if (WIFEXITED(StatLoc) == TRUE)
        {
        T->ExitCode = WEXITSTATUS(StatLoc);

        if (T->ExitCode > 127)
          T->ExitCode = 127 - T->ExitCode;
        }

      T->LastExecStatus = T->State;
      T->EndTime    = MSched.Time;
      }
    else if (WIFEXITED(StatLoc) == TRUE)
      {
      T->ExitCode = WEXITSTATUS(StatLoc);

      __SetTriggerStateByECPolicy(T);

      if (T->State == mtsFailure)
        {
        /* trigger returned failure exitcode */

        /* NOTE:  returned value is unsigned char, thus negative values > 128 */

        MDB(4,fSTRUCT) MLog("INFO:      trigger '%s' exited with code '%d' (WIFEXITED==TRUE)  StatLoc=%d\n",
          T->TName,
          WEXITSTATUS(StatLoc),
          StatLoc);
        }

      bmset(&T->InternalFlags,mtifIsComplete);
      T->LastExecStatus = T->State;

      T->EndTime    = MSched.Time;
      }  /* END else if (WIFEXITED(StatLoc) == TRUE) */
    else
      {
      /* NOTE:  on some systems, jobs can exit abnormally 
           (ie, WIFEXITED() == TRUE) and the job is still successful */

      /*  thus, WIFEXITED alone is not an adequate test */

      if ((T->OBuf != NULL) && (MTrigLoadExitValue(T,&ExitCode) == SUCCESS))
        {
        T->State = ExitCode;
        }
      else 
        {
        T->ExitCode = WEXITSTATUS(StatLoc);

        if ((T->ExitCode == 0) && (WIFSIGNALED(StatLoc) == TRUE))
          {
          T->ExitCode = WTERMSIG(StatLoc);
          }

        __SetTriggerStateByECPolicy(T);

        if (T->State == mtsFailure)
          {
          /* trigger returned failure exitcode */

          /* NOTE:  returned value is unsigned char, thus negative values > 128 */

          MDB(4,fSTRUCT) MLog("INFO:      trigger '%s' exited with code '%d' (WIFEXITED==FALSE)  StatLoc=%d\n",
            T->TName,
            WEXITSTATUS(StatLoc),
            StatLoc);
          }
        }    /* END else ((T->OBuf != NULL) && (MTrigLoadExitValue(T,&ExitCode) == SUCCESS)) */

      bmset(&T->InternalFlags,mtifIsComplete);
      T->LastExecStatus = T->State;

      T->EndTime    = MSched.Time;
      }

    MUClearChild(NULL);

    MDB(7,fSTRUCT) MLog("INFO:    trigger '%s' with PID '%d' is finalizing completion\n",
      (T->Name != NULL) ? T->Name : "NULL",
      T->PID);

    MTrigRegisterEvent(T);
    }  /* END else if (pid == T->PID) */
  else if ((pid == -1) && (errno != ECHILD))
    {
    /* error finding trigger process--check that it exists */

    MDB(3,fSTRUCT) MLog("ALERT:    waitpid returned error (errno: %d '%s') for child process %d, checking if process exists\n",
      errno,
      strerror(errno),
      T->PID);

    if (kill(T->PID,0) == -1)
      {
      /* Process does not exist, assume completed */

      TriggerExited = TRUE;

      T->State = mtsSuccessful;

      bmset(&T->InternalFlags,mtifIsComplete);
      T->LastExecStatus = T->State;

      T->EndTime    = MSched.Time;

      MDB(1,fSTRUCT) MLog("ALERT:   trigger '%s' with PID '%d' returned error in waitpid()--completing!\n",
        T->Name,
        T->PID);

      MTrigRegisterEvent(T);
      }
    }    /* END else if (pid == -1) */
  else
    {
    /* trigger must be active if it hasn't finished and there
     * is not error finding its process (we only call
     * this function for triggers that have been launched) */

    T->State = mtsActive;

    /* check if process exists */

    if ((T->AType != mtatInternal) && (kill(T->PID,0) == -1))
      {
      /* process does not exist, assume completed */

      TriggerExited = TRUE;

      if ((T->OBuf != NULL) && (MTrigLoadExitValue(T,&ExitCode) == SUCCESS))
        T->State = ExitCode;
      else
        T->State = mtsSuccessful;

      bmset(&T->InternalFlags,mtifIsComplete);
      T->LastExecStatus = T->State;

      T->EndTime    = MSched.Time;

      MDB(3,fSTRUCT) MLog("WARNING:  trigger '%s' with PID '%d' does not exist--completing!\n",
        T->Name,
        T->PID);

      MTrigRegisterEvent(T);
      }
    }     /* END else */

  if ((!bmisset(&T->InternalFlags,mtifIsComplete)) && 
      (TriggerExited == FALSE))
    {
    if ((T->OBuf != NULL) && (MTrigLoadExitValue(T,&ExitCode) == SUCCESS))
      T->State = ExitCode;

    MTrigRegisterEvent(T);
    }

  return(SUCCESS);
  }  /* END MTrigCheckState() */



/**
 * Check internal trigger action.
 *
 * @see MSchedCheckTriggers() - parent
 * @see MTrigCheckInternalAction() - sibling
 *
 * @param T (I)
 * @param ActionString (I)
 * @param Complete (O) whether the trigger is complete
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MTrigCheckInternalAction(

  mtrig_t *T,
  char    *ActionString,
  mbool_t *Complete,
  char    *Msg)

  {
  char ArgLine[MMAX_LINE];
  char OID[MMAX_BUFFER];
  char Action[MMAX_LINE];

  enum MXMLOTypeEnum OType;

  const char *FName = "MTrigCheckInternalAction";

  MDB(4,fSCHED) MLog("%s(%s,%s,Msg)\n",
    FName,
    ((T != NULL) && (T->TName != NULL)) ? T->TName : "NULL",
    (ActionString != NULL) ? ActionString : "NULL");

  if (Msg != NULL)
    Msg[0] = '\0';

  if ((ActionString == NULL) || (Complete == NULL))
    {
    if (Msg != NULL)
      strcpy(Msg,"internal error");

    return(FAILURE);
    }

  *Complete = FALSE;

  if (MTrigParseInternalAction(ActionString,&OType,OID,Action,ArgLine,Msg) == FAILURE)
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoNode:

      {
      enum MNodeCtlCmdEnum atype;

      atype = (enum MNodeCtlCmdEnum)MUGetIndexCI(Action,MNodeCtlCmds,FALSE,mncmNONE);

      if (atype == mncmNONE)
        {
        return(FAILURE);
        }

      switch (atype)
        {
        case mncmModify:
 
          {
          char *ptr;
          char *TokPtr;
 
          mnode_t *N;
 
          char *Attr;
          char *Value;
 
          enum MNodeStateEnum State;
          enum MNodeAttrEnum  aindex;

          /* FORMAT:  <ARG>=<VALUE> */
 
          Attr  = MUStrTok(ArgLine,"=",&TokPtr);
          Value = MUStrTok(NULL,"= \t\n",&TokPtr);
 
          aindex = (enum MNodeAttrEnum)MUGetIndexCI(Attr,MNodeAttr,FALSE,mnaNONE);

          /* FORMAT: OID = node[,node]... */
 
          ptr = MUStrTok(OID,",",&TokPtr);
 
          if ((ptr == NULL) || (aindex == mnaNONE))
            {
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
              case mnaPowerIsEnabled:

                if (!strcasecmp(Value,"on+"))
                  {
                  if ((N->PowerState != mpowOn) ||
                      (N->PowerSelectState != mpowOn) ||
                      (MNodeGetRMState(N,mrmrtCompute,&State) == FAILURE) ||
                      (!MNSISUP(State)))
                    {
                    return(SUCCESS);
                    }
                  }
                else if (!strcasecmp(Value,"off+"))
                  {
                  if ((N->PowerState != mpowOff) ||
                      (N->PowerSelectState != mpowOff) ||
                      (MNodeGetRMState(N,mrmrtCompute,&State) == FAILURE) ||
                      (MNSISUP(State)))
                    {
                    return(SUCCESS);
                    }
                  }
 
                break;
    
              default:
 
                return(FAILURE);
 
                /*NOTREACHED*/
 
                break;
              }   /* END switch (aindex) */
 
            ptr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (ptr != NULL) */

          /* only reached when all nodes have reached their intended state */

          *Complete = TRUE;
          }  /* END if (MSched.AggregateNodeActions == FALSE) */

          break;

        default:

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }
      }   /* END BLOCK (case mxoNode) */

      break;

    default:

      if (Msg != NULL)
        strcpy(Msg,"unsupported trigger object type");

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (OType) */

  return(SUCCESS);
  }  /* END MTrigCheckInternalAction() */


/**
 * Check usage/performance threshold for a MVC trigger.
 *
 * Report SUCCESS if trigger threshold is satisfied.
 *
 * @param T (I)
 * @param UsageP (O) [required]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */
int __MTrigCheckThresholdUsageMVC(

  mtrig_t *T,      /* I */
  double  *UsageP, /* O (required) */
  char    *EMsg)   /* O (optional,minsize=MMAX_LINE) */

  {
  mvc_t *VC;
  int    rc = SUCCESS;
  
  if ((NULL == T) || (NULL == UsageP))
    return(FAILURE);

  /* cast the generic object pointer to a MVC pointer */
  VC = (mvc_t *)T->O;

  /* Ensure we have a valid object pointer for this Trigger */
  if (NULL == VC)
    {
    if (NULL != EMsg)
      {
      snprintf(EMsg,MMAX_LINE,"Triger %s does NOT have a valid object pointer\n",
            NULL == T->TName  ? "<NULL>" : T->TName);
      }
    return(FAILURE);
    }


  /* What type of Trigger Type? */
  switch (T->EType)
    {
    case mttThreshold:

      /* what type of Threshold Type */
      switch (T->ThresholdType)
        {
        case mtttGMetric:
          /* T->ThresholdGMetricType has the GMETRIC 'name' */

          rc = MVCGMetricHarvestAverageUsage(VC,T->ThresholdGMetricType,UsageP,EMsg);

          break;

        default:
          break;
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (T->EType) */

  return(rc);
  }  /* END __MTrigCheckThresholdUsageMVC() */


/**
 * Check usage/performance threshold for trigger.
 *
 * Report SUCCESS if trigger threshold is satisfied.
 *
 * @param T (I)
 * @param UsageP (O) [required]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MTrigCheckThresholdUsage(

  mtrig_t *T,      /* I */
  double  *UsageP, /* O (required) */
  char    *EMsg)   /* O (optional,minsize=MMAX_LINE) */

  {
  int rc;

  if (UsageP != NULL)
    *UsageP = 0.0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((UsageP == NULL) || (T == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  T->ThresholdUsage = 0.0;

  /* Process based on the Object type */
  switch (T->OType)
    {
    case mxoxVC:

      /* Go Process the Threshold usage for the specified MVC */
      rc = __MTrigCheckThresholdUsageMVC(T,UsageP,EMsg);
      if (FAILURE == rc)
        {
        return(FAILURE);
        }

      break;

    case mxoAcct:
    case mxoGroup:
    case mxoUser:

      {
      mgcred_t *C = (mgcred_t *)T->O;

      switch (T->EType)
        {
        case mttStart:
        case mttEnd:

          if (C == NULL)
            break;

          switch (T->ThresholdType)
            {
            case mtttBacklog:

              if (C->L.IdlePolicy == NULL)
                {
                return(FAILURE);
                }

              *UsageP = (double)C->L.IdlePolicy->Usage[mptMaxWC][0];

              break;

            case mtttQueueTime:

              *UsageP = ((double)C->Stat.TotalQTS / C->Stat.JC / 3600);

              break;

            case mtttXFactor:

              *UsageP = MIN(999.99,((double)C->Stat.XFactor / C->Stat.JC));

              break;

            case mtttUsage:
            case mtttAvailability:
            case mtttGMetric:
            default:

              if (EMsg != NULL)
                {
                strcpy(EMsg,"threshold type not supported");
                }

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }  /* END switch (T->ThresholdType) */

            break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (T->EType) */
      }    /* END case mxoAcct */

      break;     

    case mxoClass:

      {
      mclass_t *C = (mclass_t *)T->O;

      switch (T->EType)
        {
        case mttThreshold:
        case mttStart:
        case mttEnd:

          if (C == NULL)
            break;

          switch (T->ThresholdType)
            {
            case mtttBacklog:

              if (C->L.IdlePolicy == NULL)
                {
                if (EMsg != NULL)
                  {
                  strcpy(EMsg,"no idle stats available");
                  }

                return(FAILURE);
                }

              *UsageP = (double)C->L.IdlePolicy->Usage[mptMaxWC][0];

              break;

            case mtttQueueTime:

               *UsageP = ((double)C->Stat.TotalQTS / C->Stat.JC / 3600);

               break;

            case mtttXFactor:

              *UsageP = MIN(999.99,((double)C->Stat.XFactor / C->Stat.JC));

              break;

            case mtttUsage:

              /* total jobs jobs */

              *UsageP  = (double)C->L.ActivePolicy.Usage[mptMaxJob][0];

              if (C->L.IdlePolicy != NULL)
                *UsageP += (double)C->L.IdlePolicy->Usage[mptMaxJob][0];

              break;

            case mtttAvailability:
            case mtttGMetric:
            default:

              if (EMsg != NULL)
                {
                strcpy(EMsg,"threshold metric not yet supported");
                }

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }

          break;

        default:

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (T->EType) */
      }    /* END case mxoClass */

      break;

    case mxoJob:

      {
      mjob_t *J = (mjob_t *)T->O;

      switch (T->EType)
        {
        case mttStart:
        case mttEnd:

          if (J == NULL)
            break;

          switch (T->ThresholdType)
            {
            case mtttQueueTime:

               if (MJOBISACTIVE(J) == FALSE)
                 *UsageP = MSched.Time - J->SubmitTime;

               break;

            case mtttXFactor:

              if (MJOBISACTIVE(J) == FALSE)
                *UsageP = 1 + (double)(MSched.Time - J->SubmitTime) / MAX(1,J->WCLimit);

              break;

            case mtttUsage:
            case mtttAvailability:
            case mtttGMetric:
            default:

              if (EMsg != NULL)
                {
                strcpy(EMsg,"threshold metric not yet supported");
                }

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (T->EType) */
      }    /* END BLOCK (case mxoJob) */

      break;

    case mxoNode:

      {
      mnode_t *N = (mnode_t *)T->O;

      switch (T->ThresholdType)
        {
        case mtttGMetric:
          {
          int gmindex;

          if ((T->ThresholdGMetricType == NULL) || (N->XLoad == NULL))
            {
            if (EMsg != NULL)
              strcpy(EMsg,"gmetric value not tracked");

            return(FAILURE);
            }

          for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
            {
            if ((N->XLoad->GMetric[gmindex] == NULL) ||
                (N->XLoad->GMetric[gmindex]->IntervalLoad == MDEF_GMETRIC_VALUE))
              {
              continue;
              }

            if (strcmp(MGMetric.Name[gmindex],T->ThresholdGMetricType))
              {
              continue;
              }

            *UsageP = N->XLoad->GMetric[gmindex]->IntervalLoad;

            break;
            }  /* END for (gmindex) */

          if ((gmindex >= MSched.M[mxoxGMetric]) ||
              (MGMetric.Name[gmindex] == NULL))
            {
            if (EMsg != NULL)
              strcpy(EMsg,"could not find requested gmetric");

            return(FAILURE);
            }
          }    /* END case mtttGMetric */
          
          break;

        case mtttStatistic:

          {
          char tmpLine[MMAX_LINE];

          enum MNodeStatTypeEnum aindex;

          aindex = (enum MNodeStatTypeEnum)MUGetIndexCI(T->ThresholdGMetricType,MNodeStatType,FALSE,0);

          MStatAToString(&N->Stat,msoNode,aindex,0,tmpLine,mfmHuman);

          *UsageP = strtod(tmpLine,NULL);
          }

          break;

        default:

          /* NOT SUPPORTED */

          if (EMsg != NULL)
            strcpy(EMsg,"threshold type not supported");

          return(FAILURE);

          /* NOTREACHED */

          break;
        }  /* END switch (T->ThresholdType) */
      }    /* END BLOCK (case mxoNode) */

      break;

    case mxoQOS:

      {
      mqos_t *Q = (mqos_t *)T->O;

      switch (T->EType)
        {
        case mttStart:
        case mttEnd:

          if (Q == NULL)
            break;

          switch (T->ThresholdType)
            {
            case mtttBacklog:

              if (Q->L.IdlePolicy == NULL)
                {
                if (EMsg != NULL)
                  {
                  strcpy(EMsg,"no idle stats available");
                  }

                return(FAILURE);
                }

              *UsageP = (double)Q->L.IdlePolicy->Usage[mptMaxWC][0];

              break;

            case mtttQueueTime:

              *UsageP = ((double)Q->Stat.TotalQTS / Q->Stat.JC / 3600);

              break;

            case mtttXFactor:

              *UsageP = MIN(999.99,((double)Q->Stat.XFactor / Q->Stat.JC));

              break;

            case mtttUsage:
            case mtttAvailability:
            case mtttGMetric:
            default:

              if (EMsg != NULL)
                strcpy(EMsg,"threshold type not supported");

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }  /* END switch (T->ThresholdType) */

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (T->EType) */
      }    /* END case mxoAcct */

      break;   

    case mxoRsv:

      {
      mrsv_t *R;

      R = (mrsv_t *)T->O;

      switch (T->EType)
        {
        case mttStart:
        case mttEnd:
        case mttThreshold:

          if (R == NULL)
            break;

          if (R->CIPS + R->CAPS + R->TIPS + R->TAPS < 0)
            {
            if (EMsg != NULL)
              strcpy(EMsg,"no reservation usage recorded");

            return(FAILURE);
            }

          *UsageP = (R->CAPS + R->TAPS) / (R->CIPS + R->CAPS + R->TIPS + R->TAPS);

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (T->EType) */
      }    /* END case mxoRsv */

      break;

    case mxoRM:

      {
      mrm_t  *R = (mrm_t *)T->O;

      mpar_t *P = &MPar[R->PtIndex];

      switch (T->EType)
        {
        case mttThreshold:
        case mttStart:
        case mttEnd:

          if (R == NULL)
            break;

          switch (T->ThresholdType)
            {
            case mtttAvailability:

              if (bmisset(&T->InternalFlags,mtifThresholdIsPercent))
                {
                *UsageP = ((double)P->UpNodes / (double)P->ConfigNodes);
                }
              else
                {
                *UsageP = P->ConfigNodes - (P->ConfigNodes - P->UpNodes);
                }

              break;

            case mtttBacklog:

              *UsageP = P->BacklogDuration;

              break;

            case mtttQueueTime:

              /* only supported on internal RM */

              if (bmisset(&R->IFlags,mrmifLocalQueue))
                {
                mjob_t *J;
                mhashiter_t JobListIter;
                char *JobName;
             
                MUHTIterInit(&JobListIter);
             
                /* add up backlog for non-migrated jobs */

                while (MUHTIterate(&R->U.S3.JobListIndex,&JobName,NULL,NULL,&JobListIter) == SUCCESS)
                  {
                  if (MJobFind(JobName,&J,mjsmExtended) == FAILURE)
                    {
                    continue;
                    }

                  if (MJOBISACTIVE(J) == TRUE)
                    continue;

                  *UsageP += (J->SubmitTime > MSched.Time) ? 0 : MSched.Time - J->SubmitTime;
                  }  
                }    /* END if (bmisset(&R->Flags,mrmfInternalQueue)) */
 
              break;

            case mtttUsage:

              /* FIXME: checks usage for entire system, not just this resource manager */

              if (MJobHT.size() == 0)
                *UsageP = 0.0;
              else
                *UsageP = 1.0;

              break;

            case mtttXFactor:
            case mtttGMetric:
            default:

              if (EMsg != NULL)
                strcpy(EMsg,"threshold type not supported");

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }

          break;

        case mttFailure:

          /* NYI (DOUG?) */

          break;

        default:

          return(FAILURE);

          /* NOTREACHED */

          break;
        }  /* END switch (T->EType) */
      }    /* END BLOCK (case mxoRM) */

      break;

    case mxoSched:

      switch (T->EType)
        {
        case mttThreshold:
        case mttStart:
        case mttEnd:

          switch (T->ThresholdType)
            {
            case mtttUsage:

              if (bmisset(&T->InternalFlags,mtifThresholdIsPercent))
                {
                *UsageP = (((double)(MPar[0].UpRes.Procs - MIN(MPar[0].ARes.Procs,(MPar[0].UpRes.Procs - MPar[0].DRes.Procs)))) / (double)MPar[0].UpRes.Procs);
                }
              else
                {
                *UsageP = MPar[0].UpRes.Procs -  MIN(MPar[0].ARes.Procs,(MPar[0].UpRes.Procs - MPar[0].DRes.Procs));
                }

              break;

            case mtttBacklog:
            case mtttAvailability:
            case mtttQueueTime:
            case mtttXFactor:
            case mtttGMetric:
            default:

              if (EMsg != NULL)
                strcpy(EMsg,"threshold type not supported");

              return(FAILURE);

              /*NOTREACHED*/

              break;
            }

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (T->EType) */

      break;

    default:

      /* NOT SUPPORTED */

      if (EMsg != NULL)
        strcpy(EMsg,"event type not supported");

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (T->EType) */

  /* At this point we should have the harvested 'Usage'. Take that
   * with the CMP operation and the Trigger 'Threshold and perform
   * the expression evaulation.
   * If the threshold not met, then FAIL - do not fire the trigger
   * If met, then indicate that we should fire the trigger
   */
  if (!MUDCompare(*UsageP,T->ThresholdCmp,T->Threshold))
    {
    /* threshold not satisfied */

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"threshold not satisfied - requires usage %f %s %f",
        *UsageP,
        MComp[T->ThresholdCmp],
        T->Threshold); 
      }

    T->FailTime = 0;  /* reset threshold failure time */

    return(FAILURE);
    }

  if (T->FailTime == 0)
    {
    T->FailTime = MSched.Time;
    }

  /* check to see if we have crossed threshold for the needed amount of time */

  if ((long)(MSched.Time - T->FailTime) < (long)T->FailOffset)
    {
    if (EMsg != NULL)
      {
      char tmpDuration[MMAX_NAME];
      char TString[MMAX_LINE];

      MULToTString((long)T->FailOffset,TString);

      strcpy(tmpDuration,TString);

      MULToTString(MSched.Time - T->FailTime,TString);

      snprintf(EMsg,MMAX_LINE,"threshold duration not met - currently %s of %s",
        TString,
        tmpDuration);
      }

    return(FAILURE);
    }

  /* usage has crossed threshold, proceed to trigger action by returning success */

  T->ThresholdUsage = *UsageP;  /* record in the trigger what the threshold value is at */

  return(SUCCESS);
  }  /* END MTrigCheckThresholdUsage() */




/**
 * Check all trigger and data/variable dependencies.
 *
 * FIXME: this routine really needs to be refactored to only use hash tables and no linked lists
 * 
 * @param T         (I)
 * @param EMsgFlags (I) Flags that specify how verbose of an EMsg to create [enum MCModeEnum bitmap]
 * @param EMsg      (O) [optional,minsize=MMAX_LINE]
 */

int MTrigCheckDependencies(

  mtrig_t   *T,
  mbitmap_t *EMsgFlags,
  mstring_t *EMsg)

  {
  int VIndex;
  int HIndex;

  mln_t **Var = NULL;    /* local variable buffer */
  int    *VarFlags = NULL;

  mhash_t *HTs[MMAX_NAME];
  int     HTFlags[MMAX_NAME];

  mbool_t DoExtended = FALSE;
  mbool_t ExtSearch = FALSE;

  mdepend_t **DArray;

#ifdef MYAHOO3
  char   *ptr;
  char   *TokPtr;
#endif /* MYAHOO3 */

  if (EMsg != NULL)
    MStringSet(EMsg,"\0");

  Var = (mln_t **)MUCalloc(1,sizeof(mln_t *) * MSched.M[mxoNode]);
  VarFlags = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);

  memset(HTs,0,sizeof(HTs));
  memset(HTFlags,0,sizeof(HTFlags));

#ifdef MYAHOO3
  /* ensure all of the trigger dependencies are fulfilled first */

  if ((T->TrigDepString != NULL) &&
      (T->TrigDepString->size() > 0) &&
      (T->TrigDeps == NULL))
    {
    int index;
    mln_t *tmpLL = NULL;

    /* need to lookup/register triggers we depend on */

    MULLCreate(&T->TrigDeps);

    ptr = MUStrTok(T->TrigDepString->c_str()),",.",&TokPtr);

    for (index = 0;ptr != NULL;index++)
      {
      MULLAdd(&T->TrigDeps,ptr,NULL,&tmpLL,NULL);

      ptr = MUStrTok(NULL,",.",&TokPtr);
      }

    /* we no longer need this string */

    delete T->TrigDepString;  
    T->TrigDepString = NULL;
    }

  if ((T->TrigDeps != NULL) && (T->TrigDeps->ListSize > 0))
    {
    mln_t   *LLPtr;
    mtrig_t *tmpTrig;
    mbool_t  DisableTrig = TRUE;

    /* we need to ensure we've already registered with all
     * of the triggers T depends on */

    LLPtr = T->TrigDeps;

    while (LLPtr != NULL)
      {
      if (bmisset(&LLPtr->BM,TRUE))
        {
        /* already registered with this trig */

        LLPtr = LLPtr->Next;

        continue;
        }

      if (MTrigFind(LLPtr->Name,&tmpTrig) == SUCCESS)
        {
        /* NOTE: we do not store a pointer to the trigger in case the
         * trigger is free'd without us knowing it */

        if (tmpTrig->Dependents == NULL)
          MULLCreate(&tmpTrig->Dependents);

        MULLAdd(&tmpTrig->Dependents,T->Name,NULL,NULL,NULL);

        MSET(LLPtr->BM,TRUE);  /* mark that the trigger exists! */
        }
      else
        {
        /* we can't disable the trig. until we've registered
         * with all of trig. dependencies */

        DisableTrig = FALSE;
        }

      LLPtr = LLPtr->Next;
      }

    if (DisableTrig == TRUE)
      {
      T->OutstandingTrigDeps = TRUE; /* don't bail out of this function yet--it is possible that all deps really are satisfied */
      }
    }
#endif /* MYAHOO3 */

  if (T->Depend.Array != NULL)
    {
    DArray = (mdepend_t **)T->Depend.Array;

    for (VIndex = 0;VIndex < T->Depend.NumItems;VIndex++)
      {
      if (DArray[VIndex] == NULL)
        break;

      if (bmisset(&DArray[VIndex]->BM,mdbmExtSearch))
        {
        DoExtended = TRUE;

        break;
        }
      }  /* END for (VIndex) */
    }

  /* build list of accessible variables */

  VIndex = 0;
  HIndex = 0;

  if (T->O != NULL)
    {
    switch (T->OType)
      {
      case mxoJob:

        {
        mjob_t *J;

        J = (mjob_t *)T->O;

        HTs[HIndex++] = &J->Variables;

        /* we use J->JGroupIndex for faster lookups */

        while (J != NULL)
          {
          mjob_t *tmpJ = NULL;

          if (J->JGroup != NULL)
            MJobFind(J->JGroup->Name,&tmpJ,mjsmExtended);

          if (tmpJ != NULL)
            {
            HTs[HIndex] = &tmpJ->Variables;
 
            if (ExtSearch == TRUE)
              HTFlags[HIndex] = 1;  /* this denotes that HTs[HIndex] is a set of vars in global scope */

            HIndex++;
            }

          if (DoExtended == FALSE)
            break;

          ExtSearch = TRUE;

          J = tmpJ;
          }
        }    /* END BLOCK (case mxoJob) */

        break;

      case mxoNode:

        if (!MUHTIsEmpty(&((mnode_t *)T->O)->Variables))
          {
          HTs[HIndex] = &((mnode_t *)T->O)->Variables;

          HIndex++;
          }

        break;

      case mxoRM:

        if (((mrm_t *)T->O)->Variables != NULL)
          Var[VIndex++] = ((mrm_t *)T->O)->Variables;

        break;

      case mxoRsv:

        {
        mrsv_t *R;
        mrsv_t *tmpR;

        R = (mrsv_t *)T->O;

        if (R->Variables != NULL)
          Var[VIndex++] = R->Variables;

        if ((R->RsvGroup != NULL) && 
            (MRsvFind(R->RsvGroup,&tmpR,mraNONE) == SUCCESS))
          {
          if (tmpR->Variables != NULL)
            {
            VarFlags[VIndex] = 1; /* this denotes that the variables are from a global space */
            Var[VIndex++]    = tmpR->Variables;
            }
          }
        }    /* END if (T->O != NULL) */

        break;

      case mxoQOS:

        if (((mqos_t *)T->O)->Variables != NULL)
          Var[VIndex++] = ((mqos_t *)T->O)->Variables;

        break;

      case mxoClass:

        if (((mclass_t *)T->O)->Variables != NULL)
          Var[VIndex++] = ((mclass_t *)T->O)->Variables;

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (T->OType) */
    }    /* END if (T->O != NULL) */

  if (bmisset(&T->SFlags,mtfGlobalVars))
    {
    if (T->GetVars != NULL)
      {
      mnode_t *N;

      if ((MNodeFind(T->GetVars,&N) == SUCCESS) &&
          (!MUHTIsEmpty(&N->Variables)))
        {
        HTs[HIndex] = &N->Variables;
 
        if (ExtSearch == TRUE)
          HTFlags[HIndex] = 1;  /* this denotes that HTs[HIndex] is a set of vars in global scope */

        HIndex++;
        }
      }
    else
      {
      int nindex;

      mnode_t *N;

      for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
        {
        N = MNode[nindex];

        if ((N == NULL) || (N->Name[0] == '\0'))
          break;

        if (N->Name[0] == '\1')
          continue;

        if (!bmisset(&N->Flags,mnfGlobalVars))
          continue;

        if (!MUHTIsEmpty(&N->Variables))
          {
          HTs[HIndex] = &N->Variables;
 
          if (ExtSearch == TRUE)
            HTFlags[HIndex] = 1;  /* this denotes that HTs[HIndex] is a set of vars in global scope */

          HIndex++;

          if (HIndex >= MMAX_NAME - 1)
            break;
          }
        }  /* END for (nindex) */
      }
    }      /* END if (bmisset(&T->Flags,mtfGlobalVars)) */

  Var[VIndex] = NULL;

  if (T->OType == mxoJob)
    {
    /* right now only looks at job/job group vars */

    if (MDAGCheckMultiWithHT(&T->Depend,NULL,(mhash_t **)HTs,HTFlags,EMsgFlags,EMsg,NULL) == FAILURE)
      {
      MUFree((char **)&Var);
      MUFree((char **)&VarFlags);

      return(FAILURE);
      }
    }
  else
    {
    mdepend_t *DHead = NULL;

    /* we only used linked-list variables for non-job triggers! */

    DHead = (mdepend_t *)MUArrayListGetPtr(&T->Depend,0);

    if ((MDAGCheckMulti(DHead,(mln_t **)Var,VarFlags,EMsg,NULL) == FAILURE) &&
        (MDAGCheckMultiWithHT(&T->Depend,NULL,(mhash_t **)HTs,HTFlags,EMsgFlags,EMsg,NULL) == FAILURE))
      {
      MUFree((char **)&Var);
      MUFree((char **)&VarFlags);

      return(FAILURE);
      }
    }

  MUFree((char **)&Var);
  MUFree((char **)&VarFlags);

  /* mark that we really don't have any outstanding trigger deps! */

  bmunset(&T->InternalFlags,mtifOutstandingTrigDeps);

  return(SUCCESS);
  }  /* END MTrigCheckDependencies() */



/**
 *  Helper function to set trigger state based on ECPolicy.
 *
 * @param T [I] - The trigger to evaluate
 */

int __SetTriggerStateByECPolicy(

  mtrig_t *T)

  {
  if (T == NULL)
    return(FAILURE);

  if (T->ExitCode > 127)
    T->ExitCode = 127 - T->ExitCode;

  switch (T->ECPolicy)
    {
    case mtecpIgnore:

      T->State = mtsSuccessful;

      break;

    case mtecpFailNeg:

      T->State = (T->ExitCode >= 0) ? mtsSuccessful : mtsFailure;

      break;

    case mtecpFailPos:

      T->State = (T->ExitCode <= 0) ? mtsSuccessful : mtsFailure;

      break;

    default:
    case mtecpNONE:
    case mtecpFailNonZero:

      T->State = (T->ExitCode == 0) ? mtsSuccessful : mtsFailure;

      break;
    }  /* END switch (T->ECPolicy) */

  return(SUCCESS);
  }  /* END __SetTriggerStateByECPolicy() */


int __KillTimedOutTrigger(

  mtrig_t *T,
  int pid)

  {
  int KillLevel = 9;
  enum MTrigStateEnum ExitCode;

  if (T == NULL)
    return(FAILURE);

  /* trigger has run too long, kill and cleanup */

  MDB(3,fSOCK) MLog("INFO:     trigger '%s' has run past timeout of %ld seconds, killing process '%d'\n",
    T->TName,
    T->Timeout,
    T->PID);

  if (bmisset(&T->SFlags,mtfSoftKill))
    KillLevel = 15;

  if (kill(T->PID,KillLevel) == -1)
    {
    MDB(0,fSOCK) MLog("ERROR:    cannot kill process %d\n",
      pid);
    }
  else
    {
    /* clear defunct child processes */

    MUClearChild(NULL);
    }

  if ((T->OBuf != NULL) && (MTrigLoadExitValue(T,&ExitCode) == SUCCESS))
    T->State = ExitCode;
  else
    T->State = mtsFailure;

  bmset(&T->InternalFlags,mtifIsComplete);
  T->LastExecStatus = T->State;

  T->EndTime    = MSched.Time;

  MDB(7,fSTRUCT) MLog("INFO:    trigger '%s' with PID '%d' has run too long--killing\n",
    T->Name,
    T->PID);

  MTrigRegisterEvent(T);

  return(SUCCESS);
  }  /* __KillTimedOutTrigger() */

/* END MTrigCheck.c */
