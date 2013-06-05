/* HEADER */

      
/**
 * @file MSchedTriggers.c
 *
 * Contains: MSchedCheckTriggers
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Will cycle through all global triggers (or all triggers in the given list)
 * and check their dependencies, launch needed actions based on events, etc.
 *
 * @see MTrigInitiateAction() - child
 *
 * WARNING: Not thread-safe due to static NumTrigsChecked var!
 *
 * @param ST (I) List of triggers to check. [optional]
 * @param TimeLimit (I) How long this function should execute before it returns (in milliseconds). [optional]
 * @param StartTIndex (I) An index specifying which trigger in the list to start on. If the function
 * leaves early (due to TimeLimit) this is also set to the last trigger index that was inspected. [modified,optional]
 */

int MSchedCheckTriggers(

  marray_t *ST,          /* I (optional) */
  long      TimeLimit,   /* I - in milliseconds (optional) */
  long     *StartTIndex) /* I (optional) */

  {
  mtrig_t *T = NULL;

  double Usage;
  int    tindex = 0;

  /* used to determine how much time is spend in a single invocation of MSchedCheckTrigger */
  long   LocalStartTime = 0;
  long   LocalEndTime = 0;

  /* used to determine how much time is spent in MSchedCheckTrigger per iteration */
  long   CheckStartTime = 0;
  long   CheckEndTime = 0;

  static int NumTrigsChecked = 0;  /* how many triggers we've found in the list thus far (since we last went through the list entirely) */
  int STSize = -1;          /* how many triggers are in the list (only used if ST==NULL at this point) */

  mbool_t ForceIdleTrigJobs = FALSE;
    
  mbool_t AllTrigsWereEvaluated = FALSE;  /* says if all triggers were evaluated, either in a single call
                                             to this function or many calls over time */

  const char *FName = "MSchedCheckTriggers";

  MDB(5,fCORE) MLog("%s(%s,%ld,StartTIndex)\n",
    FName,
    (ST == NULL) ? "NULL" : "ST",
    TimeLimit);

  if (MSched.State == mssmStopped)
    {
    /* Do not check triggers if scheduling is stopped */

    return(SUCCESS);
    }

  if (MSched.LoadStartTime[mschpTriggers] == 0)
    MUGetMS(NULL,&MSched.LoadStartTime[mschpTriggers]);
  else
    MUGetMS(NULL,&CheckStartTime);

  MUGetMS(NULL,&LocalStartTime);

  if (MSched.ForceTrigJobsIdle == TRUE)
    ForceIdleTrigJobs = TRUE;

  if ((StartTIndex != NULL) &&
      (*StartTIndex > 0))
    {
    tindex = *StartTIndex;
    }
  else
    {
    /* since we are starting over at the beginning, reset
     * how many triggers we've checked thus far */

    NumTrigsChecked = 0;
    }

  if (ST == NULL)
    STSize = MTList.NumItems;

  MSched.TrigStats.NumTrigChecksThisIteration++;

  for (;tindex < MTList.NumItems;tindex++)
    {
    if (ST != NULL)
      {
      if (tindex >= MTList.NumItems)
        {
        /* we hit the bottom of the specified list */

        /* sync below time checks with code at bottom of function */

        if (CheckStartTime > 0)
          {
          MUGetMS(NULL,&CheckEndTime);

          MSched.LoadEndTime[mschpTriggers] += (CheckEndTime - CheckStartTime);
          }
        else
          {
          MUGetMS(NULL,&MSched.LoadEndTime[mschpTriggers]);
          }

        return(SUCCESS);
        }

      T = (mtrig_t *)MUArrayListGetPtr(ST,tindex);
      }
    else
      {
      T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);
      }

    if (MTrigIsReal(T) == FALSE)
      continue;

    /* see if we need to stop processing triggers because we've reached our time limit */

    if (TimeLimit > 0)
      {
      MUGetMS(NULL,&LocalEndTime);

      if ((LocalEndTime - LocalStartTime) > TimeLimit)
        {
        if (StartTIndex != NULL)
          *StartTIndex = tindex;

        MDB(7,fCORE) MLog("INFO:     stopping processing of triggers early\n");

        MSched.TrigStats.NumTrigCheckInterruptsThisIteration++;

        break;
        }
      }

    /* check if we have already seen all triggers */

    if ((STSize > -1) && (NumTrigsChecked >= STSize))
      {
      /* we've seen all available triggers in the system */

      AllTrigsWereEvaluated = TRUE;

      MUGetMS(NULL,&LocalEndTime);

      break;
      }

    /* check for empty slots in trig list */

    if (T->IsExtant == FALSE)
      continue;

    if (T->TName == NULL)
      continue;

    if (bmisset(&T->InternalFlags,mtifIsTemplate))
      continue;

    NumTrigsChecked++;  /* this needs to be here--disabled triggers are NOT empty slots and should be counted! */

    if (bmisset(&T->InternalFlags,mtifDisabled))
      continue;

    if (bmisset(&T->InternalFlags,mtifOutstandingTrigDeps))
      continue;

    MDB(5,fCORE) MLog("INFO:     checking trigger '%s'\n",
      T->TName);

    MSched.TrigStats.NumTrigEvalsThisIteration++;

    if (bmisset(&T->InternalFlags,mtifIsComplete))
      {
      if ((T->O == NULL) && (bmisset(&T->InternalFlags,mtifOIsRemoved)))
        {
        /* purge completed trigger */

        MDB(7,fCORE) MLog("INFO:     removing completed trigger '%s' whose object has been removed\n",
          T->TName);

        MTrigRemove(T,NULL,FALSE);

        continue;
        }

      if (!bmisset(&T->InternalFlags,mtifMultiFire))
        {
        /* trigger cannot fire again - no action required */

        MDB(7,fCORE) MLog("INFO:     trigger '%s' is complete\n",
          T->TName);

        continue;
        }

      if (T->EType == mttStanding)
        {
        /* reset standing triggers as soon as they are complete */

        MTrigReset(T);
        }
        
      if (T->RearmTime > 0)
        {
        mulong ETime = 0;

        /* trigger is complete and rearm time is set */

        /* NOTE:  trigger should be based on event time, not launch time */

        ETime = MAX(ETime,T->LastETime);
        ETime = MAX(ETime,T->ETime);
        ETime = MAX(ETime,T->LaunchTime);

        if (MSched.Time < ETime + T->RearmTime)
          {
          MDB(7,fCORE) MLog("INFO:     trigger '%s' has not yet rearmed\n",
            T->TName);

          continue;
          }
        }    /* END if (T->RearmTime > 0) */

      /* Trigger has rearmed, has to go another iteration before can fire. */

      if (T->EType != mttStanding)
        MTrigReset(T);
      }    /* END if (T->IsComplete == TRUE) */
    else if (T->State == mtsNONE)
      {
      if ((T->ExpireTime > 0) && (MSched.Time >= T->ExpireTime))
        {
        /* trigger has not yet fired and expiration time has been reached */

        MDB(7,fCORE) MLog("INFO:     trigger '%s' has expired\n",
          T->TName);

        MTrigRemove(T,NULL,FALSE);

        continue;
        }
      }

    if ((T->O == NULL) && (!bmisset(&T->InternalFlags,mtifOIsRemoved)))
      {
      /* initialize trigger */

      if ((T->OID == NULL) || (T->OType == mxoNONE))
        {
        MDB(7,fCORE) MLog("INFO:     trigger '%s' has no object, removing\n",
          T->TName);

        MTrigRemove(T,NULL,FALSE);

        continue;
        }

      if (MOGetObject(T->OType,(char *)T->OID,&T->O,mVerify) == FAILURE)
        {
        MDB(7,fCORE) MLog("INFO:     could not find object for trigger '%s', removing\n",
          T->TName);

        MTrigRemove(T,NULL,FALSE);

        continue;
        }
      }  /* END if ((T->O == NULL) && (T->OIsRemoved == FALSE)) */
    else if ((T->O == NULL) && 
             (bmisset(&T->InternalFlags,mtifOIsRemoved)) && 
             (T->State == mtsNONE))
      {
      MDB(7,fCORE) MLog("INFO:     trigger '%s' object has been removed, removing\n",
        T->TName);

      MTrigRemove(T,NULL,FALSE);

      continue;
      }
    else if ((T->O == NULL) && (!bmisset(&T->InternalFlags,mtifOIsRemoved)))
      {
      MDB(7,fCORE) MLog("INFO:     trigger '%s' is corrupt (no object), removing\n",
        T->TName);

      MTrigRemove(T,NULL,FALSE);

      continue;
      }

    if (MTRIGISRUNNING(T) &&
        ((T->PID != 0) || (T->AType == mtatInternal) || (bmisset(&T->SFlags,mtfAsynchronous))))
      {
      /* check trigger */

      MDB(7,fCORE) MLog("INFO:     trigger '%s' is active, checking state\n",
        T->TName);

      MTrigCheckState(T);

      continue;
      }  /* END if (MTRIGISRUNNING(T) && (T->PID != 0)) */

    if ((ForceIdleTrigJobs == TRUE) &&
        (T->ETime == 0))
      {
      /* since trigger jobs are now idle by default, we need to fake a start
       * event to activate the triggers */

      MOReportEvent(T->O,NULL,mxoJob,mttStart,MSched.Time,FALSE);
      }

    if (((T->ETime > MSched.Time) || (T->ETime == 0)) &&
         (T->EType != mttThreshold))
      {
      /* trigger event time not reached or not set */

      MDB(7,fCORE) MLog("INFO:     trigger '%s' on %s:%s event not yet reached (%ld >= %ld)\n",
        T->TName,
        MXO[T->OType],
        T->OID,
        T->ETime,
        MSched.Time);

      continue;
      }

    /* TODO: JOBS  -> Usage/GMetric
             NODES -> Usage
             RM    -> Down Nodes
             RSV   -> GMetric */

    if (((T->EType == mttThreshold) || (T->Threshold > 0.0)) && 
        (!bmisset(&T->InternalFlags,mtifOIsRemoved)) &&
        (MTrigCheckThresholdUsage(T,&Usage,NULL) == FAILURE))
      {
      MDB(7,fCORE) MLog("INFO:     trigger '%s' threshold not reached\n",
        T->TName);

      continue;
      }    /* END if (T->Threshold > 0.0)... */

    /* do not proceed with dependency check or trigger starts when we are shutting down--we'll
     * most likely lose the trigger and shutdown/startup in a bad state (exception - scheduler end triggers MOAB-1462) */

    if ((MSched.Shutdown == TRUE) && ((T->OType != mxoSched) || (T->EType != mttEnd)))
      continue;

    /* check dependencies, and if they are clear, execute the trigger action */

    if (MTrigCheckDependencies(T,0,NULL) == FAILURE)
      {
      MDB(7,fCORE) MLog("INFO:     trigger '%s' dependencies not satisfied\n",
        T->TName);

      continue;
      }

    /* EXECUTE TRIGGER! */

    MDB(3,fCORE) MLog("INFO:     trigger '%s' passes all policies\n",
      T->TName);

    switch (T->OType)
      {
      case mxoJob:
      case mxoNode:
      case mxoQOS:
      case mxoRM:

        MTrigInitiateAction(T);

        break;

      case mxoRsv:

        switch (T->AType)
          {
          case mtatCancel:
          
            {
            mrsv_t *R = (mrsv_t *)T->O;

            if (R->CancelIsPending == FALSE)
              {
              /* Only cancel rsv if it is not already trying to cancel.
                  This is to prevent infinite loops. */

              MRsvDestroy(&R,TRUE,TRUE);
              }
            }

            break;

          case mtatSubmit:
          case mtatExec:
          case mtatMail:
          case mtatQuery:
          default:

            MTrigInitiateAction(T);

            if (T->AType == mtatModify)
              MRsvTransition((mrsv_t *)T->O,FALSE);

            break;
          }  /* END switch (T->AType) */

        break;

      default:

        MTrigInitiateAction(T);

        break;
      } /* END switch (T->OType) */
    }   /* END for (tindex) */

  if (tindex >= MTList.NumItems)
    {
    /* we hit the bottom of the list and saw all triggers */

    AllTrigsWereEvaluated = TRUE;
    }

  /* all triggers were evaluated--we've completed one full pass! */

  if (AllTrigsWereEvaluated == TRUE)
    {
    mhashiter_t HIter;
    char *TrigName = NULL;
    char *PostAction = NULL;
    mtrig_t *tmpT = NULL;
    char tmpLine[MMAX_LINE];

    NumTrigsChecked = 0;  /* reset local count (may not need to do this,
                             because we also reset it at the top of the func) */

    if (StartTIndex != NULL)
      *StartTIndex = 0;

    MSched.TrigStats.NumAllTrigsEvaluatedThisIteration++;

    /* execute post actions */

    MUHTIterInit(&HIter);

    while (MUHTIterate(&MTPostActions,&TrigName,(void **)&PostAction,NULL,&HIter) == SUCCESS)
      {
      if (MTrigFind(TrigName,&tmpT) == SUCCESS)
        {
        snprintf(tmpLine,sizeof(tmpLine),"POSTACTION launched: %s",
          PostAction);

        MOWriteEvent((void *)tmpT,mxoTrig,mrelNote,tmpLine,NULL,NULL);

        MTrigLaunchInternalAction(tmpT,PostAction,NULL,NULL);
        }
      }

    /* remove post action regardless of success/failure */

    MUHTClear(&MTPostActions,FALSE,NULL);
    }

  /* sync below time checks with code at top of function */

  if (CheckStartTime > 0)
    {
    MUGetMS(NULL,&CheckEndTime);

    MSched.LoadEndTime[mschpTriggers] += (CheckEndTime - CheckStartTime);
    }
  else
    {
    MUGetMS(NULL,&MSched.LoadEndTime[mschpTriggers]);
    }

  return(SUCCESS);
  }  /* END MSchedCheckTriggers() */
/* END MSchedTriggers,c */
