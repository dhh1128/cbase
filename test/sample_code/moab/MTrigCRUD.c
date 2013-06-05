/* HEADER */

      
/**
 * @file MTrigCRUD.c
 *
 * Contains: Create/Read/Update/Delete functions for Triggers
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


/* global variables */

mhash_t TrigPIDs;




/**
 * Gives the trigger a name, links trigger and object, sets trigger to non-template.
 *
 * @param T (I) [modified]
 */

int MTrigInitialize(

  mtrig_t *T)  /* I (modified) */

  {
  if (T == NULL)
    {
    return(FAILURE);
    }

  /* set unique trigger id, add object pointer if required */

  if (T->TName == NULL)
    {
    char tmpTID[MMAX_LINE];
      
    /* load unique trigger id */

    if (T->Name != NULL)
      {
      snprintf(tmpTID,sizeof(tmpTID),"%16s.%d",
        T->Name,
        MSched.TrigCounter++);
      }
    else
      { 
      sprintf(tmpTID,"%d",
        MSched.TrigCounter++);
      }

    MUStrDup(&T->TName,tmpTID);
    }  /* END if (T->TName == NULL) */

  if (!bmisset(&T->InternalFlags,mtifOIsRemoved))
    {
    /* locate object pointer */

    if ((T->O == NULL) && (MOGetObject(T->OType,T->OID,&T->O,mVerify) == FAILURE))
      {
      /* cannot locate object */

      return(FAILURE);
      }

    /* link object back to trigger */

    switch (T->OType)
      {
      case mxoJob:

        MOAddTrigPtr(&((mjob_t *)T->O)->Triggers,T);

        break;

      case mxoNode:

        MOAddTrigPtr(&((mnode_t *)T->O)->T,T);

        break;

      case mxoRsv:

        MOAddTrigPtr(&((mrsv_t *)T->O)->T,T);

        /* verify object name matches trigger oid */

        if (strcmp(T->OID,((mrsv_t *)T->O)->Name))
          {
          MUStrDup(&T->OID,((mrsv_t *)T->O)->Name);
          }

        break;

      case mxoSched:

        T->O = (void *)&MSched;

        break;

      case mxoRM:

        MOAddTrigPtr(&((mrm_t *)T->O)->T,T);

        break;

      case mxoxVM:

        MOAddTrigPtr(&((mvm_t *)T->O)->T,T);

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (T->OType) */
    }    /* END if (T->O == NULL) */

  if (T->EType == mttStanding)
    {
    MTrigInitStanding(T);
    }

  bmunset(&T->InternalFlags,mtifIsTemplate);

  return(SUCCESS);
  }  /* END MTrigInitialize() */




/**
 * Create new trigger.
 *
 * NOTE: Trigger memory must already be alloc'ed
 *
 * NOTE: this routine does NOT add triggers to the global table and it does not
 *       add triggers to the object or anything of the sort.  It is really only
 *       useful for system jobs currently.  System job triggers are evaluated
 *       on their own (outside MSchedCheckTriggers) and thus can be handled
 *       differently.  Unless you're using a system job you should probably use
 *       another routine like MTrigLoadString() followed by MTrigInitialize().
 * 
 * @see moabdocs.h
 *
 * @see MTrigFree() - free/destroy existing trigger
 *
 * @param T      (O)
 * @param EType  (I)
 * @param AIndex (I)
 * @param Action (I)
 */

int MTrigCreate(

  mtrig_t                 *T,
  enum MTrigTypeEnum       EType,
  enum MTrigActionTypeEnum AIndex,
  char                    *Action)

  {
  char tmpName[MMAX_NAME];

  if (T == NULL)
    {
    return(FAILURE);
    }

  memset(T,0,sizeof(mtrig_t));

  T->EType     = EType;
  T->AType     = AIndex;

  sprintf(tmpName,"%d",
    MSched.TrigCounter++);

  MUStrDup(&T->TName,tmpName);

  if ((Action != NULL) && (Action[0] != '\0'))
    { 
    char *ptr;

    MUStrDup(&T->Action,Action);

    /* remove all white space from action */

    for (ptr = T->Action + strlen(T->Action) - 1;ptr >= T->Action;ptr++)
      {
      if (!isspace(*ptr))
        break;

      *ptr = '\0';
      }  /* END for (ptr) */
    }
  else
    {
    MUFree(&T->Action);
    }
 
  T->IsExtant = TRUE;

  /* log trigger creation in event file */

  MOWriteEvent((void *)T,mxoTrig,mrelTrigCreate,NULL,MStat.eventfp,NULL);

  CreateAndSendEventLog(meltTrigCreate,T->TName,mxoTrig,(void *)T);

  return(SUCCESS);
  }  /* END MTrigCreate() */





/**
 * Frees all of the memory associated with the given trigger.
 *
 * @param TP (I) [modified]
 */

int MTrigFree(

  mtrig_t **TP)  /* I (modified) */

  {
  mtrig_t *T;
  mbool_t IsAllocated = FALSE;

  if (TP == NULL)
    {
    return(SUCCESS);
    }

  T = *TP;

  MDB(3,fCORE) MLog("INFO:     freeing trigger '%s' (%s)\n",
    (T->TName != NULL) ? T->TName : "NULL",
    (T->Name != NULL) ? T->Name : "NULL");

  /* if trigger is still active, kill it */

  if (MTRIGISRUNNING(T) && bmisset(&T->SFlags,mtfCleanup))
    {
    T->Timeout = 1;

    MTrigCheckState(T);  /* WARNING: post actions will not be processed for this trigger! (should we process post actions in here as well?) */

    MTrigReset(T);
    }

  if (T->PID > 0)
    {
    MUHTRemoveInt(&TrigPIDs,T->PID,MUFREE);
    }

  if (T->PostAction != NULL)
    {
    MUHTRemove(&MTPostActions,T->TName,NULL);
    }

  /* free allocated memory */

  MDB(7,fCORE) MLog("INFO:     freeing trigger at %x\n",T);
  MDB(7,fCORE) MLog("INFO:     freeing trigger action at %x\n",T->Action);

  MUFree(&T->Action);
  MUFree(&T->CompleteAction);
  MUFree(&T->StdIn);
  MUFree(&T->OID);
  MUFree(&T->UserName);
  /* MUFree(&T->TName); -- free later down below because we still need to use this */
  /* MUFree(&T->Name); -- free later down below because we still need to use this */
  MUFree(&T->Description);
  MUFree(&T->GetVars);

  /* If there is a "Trig Depends On String", free it */
  if (T->TrigDepString != NULL)
    {
    delete T->TrigDepString;
    T->TrigDepString = NULL;
    }

  MUFree(&T->Env);
  MUFree(&T->PostAction);

  MUFree(&T->Msg);

  MDAGDestroy(&T->Depend);

  MULLFree(&T->TrigDeps,MUFREE);

  if (T->SSets != NULL)
    {
    MUArrayListFreePtrElements(T->SSets);
    MUArrayListFree(T->SSets);
    MUFree((char **)&T->SSets);
    }
  if (T->SSetsFlags != NULL)
    {
    MUArrayListFree(T->SSetsFlags);
    MUFree((char **)&T->SSetsFlags);
    }

  if (T->FSets != NULL)
    {
    MUArrayListFreePtrElements(T->FSets);
    MUArrayListFree(T->FSets);
    MUFree((char **)&T->FSets);
    }
  if (T->FSetsFlags != NULL)
    {
    MUArrayListFree(T->FSetsFlags);
    MUFree((char **)&T->FSetsFlags);
    }

  if (T->FUnsets != NULL)
    {
    MUArrayListFreePtrElements(T->FUnsets);
    MUArrayListFree(T->FUnsets);
    MUFree((char **)&T->FUnsets);
    }

  if (T->Unsets != NULL)
    {
    MUArrayListFreePtrElements(T->Unsets);
    MUArrayListFree(T->Unsets);
    MUFree((char **)&T->Unsets);
    }

  if (T->OBuf != NULL)
    { 
    if (MTRIGISRUNNING(T))
      {
      if ((MTrigIsGenericSysJob(T) &&
            (MSched.State == mssmShutdown)) ||
          (bmisset(&T->SFlags,mtfLeaveFiles)))
        {
        /* Don't remove generic sys job triggers on shutdown, or trigs with LeaveFiles */

        /* NO-OP */
        }
      else
        {
        remove(T->OBuf);
        }  /* END if (MTRIGISRUNNING(T)) */
      }

    MUFree(&T->OBuf);
    }  /* END if (T->OBuf != NULL) */

  if (T->EBuf != NULL)
    {
    if (MTRIGISRUNNING(T))
      {
      if ((MTrigIsGenericSysJob(T) &&
            (MSched.State == mssmShutdown)) ||
          (bmisset(&T->SFlags,mtfLeaveFiles)))
        {
        /* Don't remove generic sys job triggers on shutdown, or trigs with LeaveFiles */

        /* NO-OP */
        }
      else
        {
        remove(T->EBuf);
        }
      }  /* END if (MTRIGISRUNNING(T)) */

    MUFree(&T->EBuf);
    }  /* END if (T->EBuf != NULL) */

  if (T->IBuf != NULL)
    {
    if (MTRIGISRUNNING(T))
      {
      if ((MTrigIsGenericSysJob(T) &&
            (MSched.State == mssmShutdown)) ||
          (bmisset(&T->SFlags,mtfLeaveFiles)))
        {
        /* Don't remove generic sys job triggers on shutdown, or trigs with LeaveFiles */

        /* NO-OP */
        }
      else
        {
        remove(T->IBuf);
        }
      }  /* END if (MTRIGISRUNNING(T)) */

    MUFree(&T->IBuf);
    }  /* END if (T->EBuf != NULL) */

  T->O = NULL;

  /* remove trigger from global table */

  T->IsExtant = FALSE;

  if (bmisset(&T->SFlags,mtfGlobalTrig))
    {
    MUHTRemove(&MTUniqueNameHash,T->Name,NULL);
    MUHTRemove(&MTSpecNameHash,T->TName,NULL);
    }

  MUFree(&T->UserName);
  MUFree(&T->Name);
  MUFree(&T->TName);

  IsAllocated = bmisset(&T->InternalFlags,mtifIsAlloc);
  bmclear(&T->InternalFlags);
  bmclear(&T->SFlags);

  if (IsAllocated == TRUE)
    MUFree((char **)TP);

  return(SUCCESS);
  }  /* END MTrigFree() */





/**
 * Clear state and re-initialize standing trigger event time.
 *
 * @param T (I) [modified]
 */

int MTrigReset(

  mtrig_t *T)  /* I (modified) */

  {
  MDB(3,fCORE) MLog("INFO:     resetting trigger '%s''\n",
      T->TName);

  if (!bmisset(&T->InternalFlags,mtifIsInterval))
    {
    T->LaunchTime = 0;
    }

  MTrigResetETime(T);

  MTrigSetState(T,mtsNONE);

  bmunset(&T->InternalFlags,mtifIsComplete);

  if (T->PID > 0)
    {
    MUHTRemoveInt(&TrigPIDs,T->PID,MUFREE);
    }

  T->PID = 0;
  T->StatLoc = 0;
  T->FailTime = 0;
  T->ThresholdUsage = 0;

  if (bmisset(&T->SFlags,mtfRemoveStdFiles))
    {
    /* remove std out/err files */

    if (T->OBuf != NULL)
      {
      remove(T->OBuf);
      }

    if (T->EBuf != NULL)
      {
      remove(T->EBuf);
      }
    }

  MUFree(&T->OBuf);
  MUFree(&T->EBuf);

  if (T->EType == mttStanding)
    {
    MTrigInitStanding(T);
    }

  return(SUCCESS);
  } /* END MTrigReset() */






/**
 * Add trigger to global trigger table.
 *
 * NOTE: this routine should not be called outside of MTrigLoadString() and MTListCopy()
 * 
 * @see moabdocs.h
 *
 * @see MTrigLoadString() - parent
 *
 * @param TName (I) Force trigger name [optional]
 * @param ST (I) [optional/cleared]
 * @param TP (O) [optional]
 */

int MTrigAdd(

  const char *TName,
  mtrig_t    *ST,
  mtrig_t   **TP)

  {
  int tindex;

  mtrig_t *T;

  char tmpTID[MMAX_LINE];  /* globally uniq trig id */

  const char *FName = "MTrigAdd";

  MDB(5,fSCHED) MLog("%s(%s,%s,TP)\n",
    FName,
    (TName != NULL) ? TName : "NULL",
    (ST != NULL) ? "ST" : "NULL");

  if (TP != NULL)
    {
    *TP = NULL;
    }

  if (TName != NULL)
    {
    /* adding real trigger to global list */

    if (ST != NULL) 
      ST->CTime = MSched.Time;

    MUStrCpy(tmpTID,TName,sizeof(tmpTID));

    /* search global hashtable index for existing trigger */

    if (MUHTGet(&MTSpecNameHash,TName,(void **)TP,NULL) == SUCCESS)
      {
      return(SUCCESS);
      }
    }
  else if ((ST != NULL) && (ST->Name != NULL))
    {
    /* FORMAT:  [<TName>.]<TID> */

    snprintf(tmpTID,sizeof(tmpTID),"%.16s.%d",
      ST->Name,
      MSched.TrigCounter++);
    }
  else
    {
    snprintf(tmpTID,sizeof(tmpTID),"%d",
      MSched.TrigCounter++);
    }

  /* searching above has already established that this trigger is new */

  for (tindex = 0;tindex < MTList.NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

    if (MTrigIsValid(T) == FALSE)
      break;
    }    /* END for (tindex) */

  if (tindex >= MTList.NumItems)
    {
    T = (mtrig_t *)MUCalloc(1,sizeof(mtrig_t));

    MUArrayListAppendPtr(&MTList,T);

    T = (mtrig_t *)MUArrayListGetPtr(&MTList,MTList.NumItems - 1);
    }

  if (T->TName != NULL)
    {
    MTrigFree(&T);
    }

  memset(T,0,sizeof(mtrig_t));

  /* Don't do a memcpy of ST - this can lead to shared memory problems */

  if (ST != NULL)
    {
    MTrigCopy(T,ST);
    }  /* END if (ST != NULL) */

  /* mark that we've added a trigger */

  /* set trigger name */

  T->TName = NULL;

  MUStrDup(&T->TName,tmpTID);

  bmset(&T->SFlags,mtfGlobalTrig);

  MUHTAdd(&MTUniqueNameHash,T->Name,T,NULL,NULL);
  MUHTAdd(&MTSpecNameHash,T->TName,T,NULL,NULL);

  T->IsExtant = TRUE;
  bmunset(&T->InternalFlags,mtifIsAlloc);

  /* mark flags depending on global scheduler settings */

  if (MSched.RemoveTrigOutputFiles == TRUE)
    {
    bmset(&T->SFlags,mtfRemoveStdFiles);
    }

  /* log trigger creation in event file (right now there is no separate event for "adding" and
   * creating a trigger) */

  MOWriteEvent((void *)T,mxoTrig,mrelTrigCreate,NULL,MStat.eventfp,NULL);

  CreateAndSendEventLog(meltTrigCreate,T->TName,mxoTrig,(void *)T);

  if (TP != NULL)
    *TP = T;

  return(SUCCESS);
  }  /* END MTrigAdd() */




/**
 *
 *
 * @param T (I) [modified]
 */

int MTrigResetETime(

  mtrig_t *T)  /* I (modified) */

  {
  mulong OldETime;

  OldETime = T->ETime;
 
  if ((T == NULL) || (T->O == NULL))
    {
    return(FAILURE);
    }

  switch (T->OType)
    {
    case mxoRsv:

      {
      mrsv_t *R = (mrsv_t *)T->O;

      switch (T->EType)
        {
        case mttStart:
 
          if (!bmisset(&R->Flags,mrfIsActive))
            T->ETime = 0;

          break;

        default:

          T->ETime = 0;

          break;
        }  /* END switch (T->EType) */
      }

      break;

    case mxoRM:
 
      {
      mrm_t *R = (mrm_t *)T->O;

      switch (T->EType)
        {
        case mttStart:
 
          if (R->State != mrmsActive)
            T->ETime = 0;

          break;

        default:

          T->ETime = 0;

          break;
        }  /* END switch (T->EType) */
      }    /* END BLOCK (case mxoRM) */

      break;

    case mxoJob:

      switch (T->EType)
        {
        case mttCreate:

          /* NO-OP */

          break;

        default:

          T->ETime = 0;

          break;
        }  /* END switch (T->EType) */

      break;

    case mxoAcct:
    case mxoClass:
    case mxoGroup:
    case mxoQOS:
    case mxoUser:

      switch (T->EType)
        {
        case mttStart:

          /* NO-OP */

          break;

        default:

          T->ETime = 0;

          break;
        }

      break;

    default:

      T->ETime = 0;

      break;
    }  /* END switch (T->OType) */

  if (T->ETime != OldETime)
    {
    /* event time was cleared, preserve previous event time */

    T->LastETime = OldETime;
    }
 
  return(SUCCESS);
  }  /* END MTrigResetETime() */





/**
 * Copies a trigger (deep copy).
 *
 * NOTE: copies over OName and OPtr
 *
 * @param D
 * @param S
 */

int MTrigCopy(

  mtrig_t *D,
  mtrig_t *S)

  {
  if ((D == NULL) || (S == NULL))
    {
    return(FAILURE);
    }

  D->EType = S->EType;
  D->Offset = S->Offset;
  D->AType = S->AType;
  D->Threshold = S->Threshold;
  
  MUStrDup(&D->Action,S->Action);

  D->OType = S->OType;
  MUStrDup(&D->OID,S->OID);
  MUStrDup(&D->Name,S->Name);
  MUStrDup(&D->Env,S->Env);
  D->RearmTime  = S->RearmTime;
  bmcopybit(&D->InternalFlags,&S->InternalFlags,mtifMultiFire);
  D->ECPolicy   = S->ECPolicy;
  bmcopy(&D->SFlags,&S->SFlags);
  bmcopybit(&D->InternalFlags,&S->InternalFlags,mtifTimeoutIsRelative);
  MUStrDup(&D->Description,S->Description);
  D->ECPolicy   = S->ECPolicy;
  bmcopybit(&D->InternalFlags,&S->InternalFlags,mtifDisabled);
  D->FailOffset = S->FailOffset;
  D->MaxRetryCount = S->MaxRetryCount;
  bmcopybit(&D->InternalFlags,&S->InternalFlags,mtifThresholdIsPercent);
  D->Period = S->Period;
  
  if (S->ThresholdGMetricType != NULL)
    MUStrDup(&D->ThresholdGMetricType,S->ThresholdGMetricType);

  if (S->GetVars != NULL)
    MUStrDup(&D->GetVars,S->GetVars);

  if (S->UserName != NULL)
    MUStrDup(&D->UserName,S->UserName);

  D->ThresholdType = S->ThresholdType;
  D->ThresholdCmp  = S->ThresholdCmp;

  if (S->SSets != NULL)
    {
    MTrigInitSets(&D->SSets,FALSE);

    MUArrayListCopyStrElements(D->SSets,S->SSets);
    }

  if (S->SSetsFlags != NULL)
    {
    MTrigInitSets(&D->SSetsFlags,TRUE);

    MUArrayListCopy(D->SSetsFlags,S->SSetsFlags);
    }

  if (S->FSets != NULL)
    {
    MTrigInitSets(&D->FSets,FALSE);

    MUArrayListCopyStrElements(D->FSets,S->FSets);
    }

  if (S->FSetsFlags != NULL)
    {
    MTrigInitSets(&D->FSetsFlags,TRUE);

    MUArrayListCopy(D->FSetsFlags,S->FSetsFlags);
    }

  if (S->FUnsets != NULL)
    {
    MTrigInitSets(&D->FUnsets,FALSE);

    MUArrayListCopyStrElements(D->FUnsets,S->FUnsets);
    }

  if (S->Unsets != NULL)
    {
    MTrigInitSets(&D->Unsets,FALSE);

    MUArrayListCopyStrElements(D->Unsets,S->Unsets);
    }

  MDAGCopy(&D->Depend,&S->Depend);

  D->Timeout = S->Timeout;
  D->BlockTime = S->BlockTime;

  D->IsExtant = TRUE;

  return(SUCCESS);
  }  /* END MTrigCopy() */




/**
 * Marks a trigger as extant (empty/completed) in the global trigger list.
 * Also handles any post-trigger events (like setting a job to complete, etc.).
 *
 * @param ST (I) [optional]
 * @param TName (I) [optional]
 * @param ClearOList (I)
 */

int MTrigRemove(

  mtrig_t *ST,         /* I (optional) */
  char    *TName,      /* I (optional) */
  mbool_t  ClearOList) /* I */

  {
  int      tindex;
  mtrig_t *T;

  const char *FName = "MTrigRemove";

  MDB(2,fSCHED) MLog("%s(%s,%s,%s)\n",
    FName,
    (ST != NULL) ? "ST" : "NULL",
    (TName != NULL) ? TName : "NULL",
    (ClearOList == TRUE) ? "TRUE" : "FALSE");

  if ((ST == NULL) && (TName == NULL))
    {
    return(FAILURE);
    }

  if (ST != NULL)
    {
    T = ST;
    }
  else
    {
    /* locate trigger in list */

    for (tindex = 0;tindex < MTList.NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

      if (MTrigIsValid(T) == FALSE)
        {
        /* empty trigger */

        continue;
        }

      if (!strcmp(TName,T->TName))
        {
        /* trigger matches */

        break;
        }
      }    /* END for (tindex) */

    if ((tindex >= MTList.NumItems) || (T->TName == NULL))
      {
      MDB(3,fSCHED) MLog("INFO:     no trigger found\n");

      return(SUCCESS);
      }
    }      /* END else (ST != NULL) */

  /* set IsExtant to false */

  MDB(3,fSCHED) MLog("INFO:     marking trigger '%s' as deleted\n",
    T->TName);

  T->IsExtant = FALSE;

  if (T->PID > 0)
    {
    MUHTRemoveInt(&TrigPIDs,T->PID,MUFREE);
    }

  /* should we be removing the PostAction!? */

  if (T->PostAction != NULL)
    {
    MUHTRemove(&MTPostActions,T->TName,NULL);
    }

  if (T->O != NULL)
    {
    if (T->OType == mxoJob)
      {
      mjob_t *J;

      J = (mjob_t *)T->O;

      if (bmisset(&J->IFlags,mjifExitOnTrigCompletion))
        MJobSetState(J,mjsCompleted);
      }

    if (ClearOList == TRUE)
      {
      /* remove T from O->T list */

      MODeleteTPtr(T);

      T->O = NULL;
      }
    }    /* END if (T->O != NULL) */

  return(SUCCESS);
  }  /* END MTrigRemove() */
/* END MTrigCRUD.c */
