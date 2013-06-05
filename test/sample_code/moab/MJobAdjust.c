/* HEADER */

      
/**
 * @file MJobAdjust.c
 *
 * Contains: Mob Ajustment functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param J (I)
 * @param R (I)
 */

int MJobAdjustETimes(

  mjob_t  *J,  /* I */
  mrm_t   *R)  /* I */

  {
  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if (J->StartTime != 0)
    J->StartTime += R->ClockSkewOffset;

  if (J->DispatchTime != 0)
    J->DispatchTime += R->ClockSkewOffset;

  /*  Commented out by jgardner, 1-16-08
    We shouldn't be changing the submission time on a job.
    There was a problem where if a job migrated, the job 
    submission time would change after it had migrated (*/
/*  if (J->SubmitTime != 0)
    J->SubmitTime += R->ClockSkewOffset;*/

  if (J->CompletionTime != 0)
    J->CompletionTime += R->ClockSkewOffset;

  if (J->SpecSMinTime != 0)
    J->SMinTime = (R->ClockSkewOffset + J->SpecSMinTime);

  /* adjust suspend time info */

  /* NYI */

  return(SUCCESS);
  }  /* END MJobAdjustETimes() */





/**
 * Apply credential-based holds.
 *
 * @see MRMJobPostLoad() - parent
 *
 * @param J (I) [modified]
 */

int MJobAdjustHold(

  mjob_t *J)  /* I (modified) */

  {
  mbitmap_t BM;

  mbool_t DoHold;

  if (J == NULL)
    {
    return(FAILURE);
    }

  DoHold = MBNOTSET;

  /* NOTE:  check credentials followed by default credentials */

  if ((J->Credential.U != NULL) && (J->Credential.U->DoHold != MBNOTSET))
    DoHold = J->Credential.U->DoHold;
  else if ((J->Credential.G != NULL) && (J->Credential.G->DoHold != MBNOTSET))
    DoHold = J->Credential.G->DoHold;
  else if ((J->Credential.A != NULL) && (J->Credential.A->DoHold != MBNOTSET))
    DoHold = J->Credential.A->DoHold;
  else if ((J->Credential.C != NULL) && (J->Credential.C->DoHold != MBNOTSET))
    DoHold = J->Credential.C->DoHold;
  else if ((J->Credential.Q != NULL) && (J->Credential.Q->DoHold != MBNOTSET))
    DoHold = J->Credential.Q->DoHold;
  else if ((MSched.DefaultU != NULL) && (MSched.DefaultU->DoHold != MBNOTSET))
    DoHold = MSched.DefaultU->DoHold;
  else if ((MSched.DefaultG != NULL) && (MSched.DefaultG->DoHold != MBNOTSET))
    DoHold = MSched.DefaultG->DoHold;
  else if ((MSched.DefaultA != NULL) && (MSched.DefaultA->DoHold != MBNOTSET))
    DoHold = MSched.DefaultA->DoHold;
  else if ((MSched.DefaultC != NULL) && (MSched.DefaultC->DoHold != MBNOTSET))
    DoHold = MSched.DefaultC->DoHold;
  else if ((MSched.DefaultQ != NULL) && (MSched.DefaultQ->DoHold != MBNOTSET))
    DoHold = MSched.DefaultQ->DoHold;

  if ((DoHold == MBNOTSET) || (DoHold == FALSE))
    {
    /* release hold if credhold type */

    if (bmisset(&J->Hold,mhBatch) && (J->HoldReason == mhrCredHold))
      {
      bmunset(&J->Hold,mhBatch);

      J->HoldReason = mhrNONE;     
      }

    MMBRemoveMessage(&J->MessageBuffer,"credential hold applied",mmbtNONE);

    return(SUCCESS);
    }

  /* set hold */

  if (!bmisclear(&J->Hold))
    {
    /* hold already on job */

    return(SUCCESS); 
    }

  /* NOTE:  even active jobs should have hold applied to block them from restarting when they 
            are requeued/suspended */

  MJobSetHold(J,mhBatch,MMAX_TIME,mhrCredHold,"credential hold applied");

  return(SUCCESS); 
  }  /* END MJobAdjustHold() */


/*
 * Return the QOS rsv bucket index for the given job.
 *
 * NOTE: if job has no QOS OBIndex == 0
 * NOTE: 
 *
 * @param J (I)
 * @param OBIndex (O)
 */

int MJobGetQOSRsvBucketIndex(

  mjob_t *J,
  int    *OBIndex)

  {
  int bindex;
  int qindex;

  if (OBIndex != NULL)
    *OBIndex = MMAX_QOSBUCKET;

  if ((J == NULL) || (OBIndex == NULL))
    {
    return(FAILURE);
    }

  if (J->Credential.Q == NULL)
    {
    *OBIndex = 0;
    }

  for (bindex = 1;bindex < MMAX_QOSBUCKET;bindex++)
    {
    if (MPar[0].QOSBucket[bindex].Name[0] == '\0')
      {
      /* reservation bucket not found, use DEFAULT */

      *OBIndex = 0;

      return(SUCCESS);
      }

    if (MPar[0].QOSBucket[bindex].Name[0] == '\1')
      {
      /* bucket is empty */

      continue;
      }

    for (qindex = 0;qindex < MMAX_QOS;qindex++)
      {
      if (MPar[0].QOSBucket[bindex].List[qindex] == NULL)
        {
        break;
        }

      if ((MPar[0].QOSBucket[bindex].List[qindex] == J->Credential.Q) ||
          (MPar[0].QOSBucket[bindex].List[qindex] == (mqos_t *)MMAX_QOS))
        {
        break;
        }
      }  /* END for (qindex) */
 
    /* check if could not find item */
    if (qindex >= MMAX_QOS)
      break;

    if ((MPar[0].QOSBucket[bindex].List[qindex] == J->Credential.Q) ||
        (MPar[0].QOSBucket[bindex].List[qindex] == (mqos_t *)MMAX_QOS))
      {
      *OBIndex = bindex;

      return(SUCCESS);
      }
    }    /* END for (bindex) */

  *OBIndex = MMAX_QOSBUCKET;

  return(SUCCESS);
  }  /* END MJobGetQOSRsvBucketIndex() */


/**
 * Enforce reservation policy by releasing/optimizing priority job reservations.
 *
 * @param J (I) [modified]
 * @param SP (I) [optional] 
 * @param DoRelease (I)
 */

int MJobAdjustRsvStatus(

  mjob_t  *J,         /* I (modified) */
  mpar_t  *SP,        /* I optional */
  mbool_t  DoRelease) /* I */

  {
  static int CacheIteration = -1;

  static enum MRsvPolicyEnum RsvPolicy[MMAX_PAR];

  mpar_t *P;

  const char *FName = "MJobAdjustRsvStatus";

  MDB(8,fSCHED) MLog("%s(%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (SP != NULL) ? SP->Name : "NULL",
    MBool[DoRelease]);

  if (J == NULL)
    {
    return(FAILURE);
    }  

  if (MJobIsPreempting(J) == TRUE)
    {
    return(SUCCESS);
    }

  if ((J->Rsv != NULL) && (SP != NULL) && (SP->Index > 0) && (J->Rsv->PtIndex != SP->Index))
    {
    MDB(7,fSCHED) MLog("INFO:     reservation adjustment for job %s in partition %s does not match requested partition %s\n",
      J->Name,
      MPar[J->Rsv->PtIndex].Name,
      SP->Name);

    return(SUCCESS);
    }

  /* release rsv if DoRelease == TRUE, else create rsv */

  if ((DoRelease == TRUE) && (J->Rsv == NULL))
    { 
    /* no reservation to release */

    MDB(8,fSCHED) MLog("INFO:     no rsv to release on job %s\n",
      J->Name);

    return(SUCCESS);
    }

  /* release rsv if DoRelease == TRUE, else create rsv */

  if (MSched.Iteration != CacheIteration)
    {
    int pindex;
    mpar_t *GP;

    /* once per iteration, update cache */

    /* determine per partition rsv policy */

    GP = &MPar[0];

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\1')
        continue;

      if (P->Name[0] == '\0')
        break;

      if (P->RsvPolicy != mrpDefault)
        RsvPolicy[pindex] = P->RsvPolicy;
      else if (GP->RsvPolicy != mrpDefault)
        RsvPolicy[pindex] = GP->RsvPolicy;
      else
        RsvPolicy[pindex] = MDEF_RSVPOLICY;
      }  /* END for (pindex) */

    CacheIteration = MSched.Iteration;
    }    /* END if (MSched.Iteration != CacheIteration) */

  P = (SP != NULL) ? SP : &MPar[0];

  MDB(5,fSCHED) MLog("INFO:     checking rsv status for job %s with policy %s in partition %s in %s()\n",
    J->Name,
    MRsvPolicy[RsvPolicy[P->Index]],
    P->Name,
    FName);

  switch (J->RType)
    {
    case mjrPriority:

      switch (RsvPolicy[P->Index])
        {
        case mrpCurrentHighest:

          /* release all 'current highest' reservations */

          if (DoRelease == TRUE)
            {
            MDB(6,fSCHED) MLog("INFO:     releasing reservation '%s' for rsv policy '%s'\n",
              J->Rsv->Name,
              MRsvPolicy[RsvPolicy[P->Index]]);

            MJobReleaseRsv(J,TRUE,TRUE);
            }

          /* appropriate reservations will be re-created in MQueueScheduleIJobs() */

          break;

        case mrpHighest:

          if (DoRelease == TRUE)
            {
            /* 'highest' priority reservations are persistent */

            /* NO-OP */
            }
          else
            {
            /* re-schedule reservation forward (ignore bucket rsvdepth) */

            if (MJobReserve(&J,SP,0,mjrPriority,FALSE) == SUCCESS)
              {

              J->Rsv->Priority = J->PStartPriority[P->Index];

              bmset(&J->Rsv->Flags,mrfPreemptible);

/*
              int bindex;

              if ((MJobGetQOSRsvBucketIndex(J,&bindex) == SUCCESS) && (bindex > 0) && (bindex != MMAX_QOSBUCKET))
                {
                MSchedIncrementQOSRsvCount(bindex);
                }
*/
              }
            }

          break;

        case mrpNever:
        default:

          if (DoRelease == TRUE)
            {
            MJobReleaseRsv(J,TRUE,TRUE);
            }

          break;
        }  /* END switch (RsvPolicy[P->Index])  */

      break;

    case mjrDeadline:

      if (DoRelease == TRUE)
        {
        /* remove all deadline reservations */

        MJobReleaseRsv(J,TRUE,TRUE);

        J->RType = mjrDeadline;  /* allow job to be restored */
        }
      else
        {
        if ((J->Credential.Q == NULL) || !bmisset(&J->Credential.Q->Flags,mqfDeadline))
          {
          /* no reservation required */

          /* NO-OP */

          return(SUCCESS);
          }

        /* create reservation at latest possible time which meets deadline */

        /* NYI */

        if (MJobReserve(&J,SP,0,mjrDeadline,FALSE) == FAILURE)
          {
          MDB(6,fSCHED) MLog("INFO:     cannot restore deadline reservation for job '%s' in partition %s\n",
            J->Name,
            P->Name);
          }
        }

      break;

    case mjrQOSReserved:

      if (DoRelease == TRUE)
        {
        /* qos-based job reservations are persistent */

        /* NOTE: qos job reservations with expiretime protect preemptor jobs */

        if (J->Rsv->ExpireTime > 0)
          {
          if (J->Rsv->ExpireTime < MSched.Time)
            {
            /* preemptor job has not been able to start within specified timeframe (expiretime) */

            MJobReleaseRsv(J,TRUE,TRUE);
            }
          else
            {
            if (MJobReserve(&J,SP,0,mjrPriority,FALSE) == FAILURE)
              {
              /* attempt to optimize preemption reservation failed */

              MDB(6,fSCHED) MLog("INFO:     cannot reschedule preemptor reservation for job '%s' in partition %s\n",
                J->Name,
                P->Name);
              }
            }
          }    /* END if (J->R->ExpireTime > 0) */
        }      /* END if (DoRelease == TRUE) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (J->RType) */

  return(SUCCESS);
  }  /* END MJobAdjustRsvStatus() */





/**
 *
 *
 * @param J (I) [modified]
 */

int MJobAdjustVirtualTime(

  mjob_t *J)  /* I (modified) */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  if (MJOBISACTIVE(J) == FALSE)
    {
    /* NOTE:  virtual walltime will only be in effect for active jobs */

    return(SUCCESS);
    }

  /* NOTE:  does not support malleable jobs or node-speed walltime scaling */

  if (J->VWCLimit <= 0)
    {
    /* virtual walltime not set */

    return(SUCCESS);
    }

  if ((long)((J->StartTime + J->VWCLimit) - MSched.Time) > (long)MSched.MaxRMPollInterval)
    {
    /* restore virtual job settings */

    J->WCLimit        = J->VWCLimit;
    J->SpecWCLimit[0] = J->VWCLimit;
    J->SpecWCLimit[1] = J->VWCLimit;
    }

  if (MJobAdjustWallTime(
        J,
        J->OrigWCLimit,
        J->OrigWCLimit,
        FALSE,
        FALSE,
        TRUE,
        TRUE,
        NULL) == SUCCESS)
    {
    /* clear virtual wclimit values */

    J->VWCLimit    = 0;
    J->OrigWCLimit = 0;
    }
  else
    {
    /* collision detected */

    switch (MPar[0].VWConflictPolicy)
      {
      case mvwcpPreempt:

        MJobPreempt(
          J,
          NULL,
          NULL,
          NULL,
          "virtual walltime conflict",
          MSched.PreemptPolicy,
          FALSE,
          NULL,
          NULL,
          NULL);

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (MPar[0].VWCollisionPolicy) */

    J->VWCLimit = 0;
    } /* END else (collision detected) */

  return(SUCCESS);
  }  /* END MJobAdjustVirtualTime() */





/**
 * Adjust job walltime in Moab, if ModifyRM is set, push change to RM.
 *
 * @param J (I) [modified]
 * @param MinWallTime (I) [minimum new walltime]
 * @param MaxWallTime (I) [maximum new walltime]
 * @param ModifyRM (I)
 * @param AllowPreemption (I) [NYI]
 * @param FailOnConflict (I) [fail if walltime modification results in conflict]
 * @param DoIncrIncrease (I) [increase walltime by MinWallTime increments]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobAdjustWallTime(

  mjob_t  *J,                /* I (modified) */
  long     MinWallTime,      /* I (minimum new walltime) */
  long     MaxWallTime,      /* I (maximum new walltime) */
  mbool_t  ModifyRM,         /* I */
  mbool_t  AllowPreemption,  /* I allow preemption of reservations (NYI) */
  mbool_t  FailOnConflict,   /* I (fail if walltime modification results in conflict) */
  mbool_t  DoIncrIncrease,   /* I (increase walltime by MinWallTime increments) */
  char    *EMsg)             /* O (optional,minsize=MMAX_LINE) */

  {
  long   OrigWallTime;

  char tmpLine[MMAX_LINE];

  mbool_t AdjustReservation = FALSE;

  long    WallTime = MaxWallTime;

  mnode_t *N;

  const char *FName = "MJobAdjustWallTime";

  MDB(8,fSCHED) MLog("%s(%s,%ld,%ld,%s,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MinWallTime,
    MaxWallTime,
    MBool[ModifyRM],
    MBool[AllowPreemption],
    MBool[FailOnConflict]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (J == NULL)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  if ((J->VWCLimit > 0) && (WallTime == (long)J->VWCLimit))
    {
    return(SUCCESS);
    }
  else if (WallTime == (long)J->WCLimit)
    {
    /* no change */

    return(SUCCESS);
    }

  if (MinWallTime > MaxWallTime)
    {
    /* no change--invalid inputs */

    return(SUCCESS);
    }

  OrigWallTime = J->WCLimit;

  if (/* (MJOBISACTIVE(J)) && */(J->Rsv != NULL) && (WallTime < OrigWallTime))
    {
    /* We are shortening the walltime of a job so there is no need to 
       check to see if the job reservation is still viable.
       We just need to adjust the existing reservation for the shortened walltime. */

    AdjustReservation = TRUE;

    MDB(7,fSCHED) MLog("INFO:     job %s - modify will reduce reservation walltime\n",
      J->Name);
    }
  else if ((MJOBISACTIVE(J)) && (FailOnConflict == TRUE))
    {
    mbool_t ExtensionIsBlocked = FALSE;
    mnl_t     *NLP = NULL;
    mulong OrigWCLimit = J->SpecWCLimit[0];

    MDB(7,fSCHED) MLog("INFO:     job %s - checking to see if modify can extend existing reservation\n",
      J->Name);

    /* search through walltime range from min to max */

    /* NOTE: WallTime represents the total walltime limit--not just how much is left */

    if (DoIncrIncrease == TRUE)
      {
      /* increment walltime little-by-little until we reach max */

      WallTime = MinWallTime;
      }

    if (WallTime > MaxWallTime)
      {
      /* cannot extend wallclock anymore--upper bound has been reached */

      MDB(2,fSCHED) MLog("INFO:     cannot modify walltime for job %s - cannot increase walltime (upper bound reached)\n",
        J->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"cannot modify walltime - upper walltime bound reached");

      return(FAILURE);
      }

    if (!MNLIsEmpty(&J->NodeList))
      {
      NLP = &J->NodeList;
      }
    else if (J->Rsv != NULL)
      {
      NLP = &J->Rsv->NL;
      }

    for (;WallTime > 0;WallTime -= MinWallTime)
      {
      int rqindex;
      enum MRsvTypeEnum Type;
      mrange_t RRange[MMAX_RANGE];

      ExtensionIsBlocked = FALSE;

      RRange[0].STime = MSched.Time;
      RRange[0].ETime = MSched.Time + (WallTime - (MSched.Time - J->StartTime));

      RRange[1].ETime = 0;

      Type = mrtJob;

      OrigWCLimit       = J->SpecWCLimit[0];
      J->SpecWCLimit[0] = WallTime - (MSched.Time - J->StartTime);

      /* for each allocated/reserved node, determine if extension is possible */

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        int nindex;
        mreq_t  *RQ;

        RQ = J->Req[rqindex];

        if (NLP == NULL)
          break;

        for (nindex = 0;MNLGetNodeAtIndex(NLP,nindex,&N) == SUCCESS;nindex++)
          {
          mrange_t ARange[MMAX_RANGE];

          RRange[0].TC = MNLGetTCAtIndex(NLP,nindex);

          if (MJobGetSNRange(
                J,
                RQ,
                N,
                RRange,
                1,
                NULL,
                &Type,
                ARange,
                NULL,
                MSched.SeekLong,
                NULL) == FAILURE)
            {
            ExtensionIsBlocked = TRUE;

            break;
            }

          if (ARange[0].STime > MSched.Time)
            {
            ExtensionIsBlocked = TRUE;

            break;
            }
          }  /* END for (nindex) */

        if (ExtensionIsBlocked == TRUE)
          break;
        }  /* END for (rqindex) */

      if (ExtensionIsBlocked == FALSE)
        break;
      }    /* END for (WallTime) */

    if (ExtensionIsBlocked == TRUE)
      {
      /* if extension is not possible across all nodes, return failure */

      MDB(2,fSCHED) MLog("INFO:     cannot modify walltime for job %s - cannot increase walltime (reservation/job is blocking)\n",
        J->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"cannot modify walltime - reservation/job is blocking increase");

      J->SpecWCLimit[0] = OrigWCLimit;

      return(FAILURE);
      }
    }    /* END if (MJOBISACTIVE(J)) */

  /* update job */

  /* modify rm if required */

  if (ModifyRM == TRUE)
    {
    sprintf(tmpLine,"%ld",
      WallTime);

    if (MRMJobModify(
       J,
       "Resource_List",
       "walltime",
       tmpLine,
       mSet,
       NULL,
       EMsg,
       NULL) == FAILURE)
      {
      MDB(2,fSCHED) MLog("INFO:     cannot modify walltime for job %s via RM - '%s'\n",
        J->Name,
        (EMsg != NULL) ? EMsg : "NULL");

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"RM failure attempting to change walltime via RM");
 
      return(FAILURE);
      }
    }

  /* if new job walltime is less than old walltime do the following:

     - reduce job walltime via MJobSetAttr(J,mjaSpecWCLimit)
     - reduce J->R->EndTime via MRsvSetAttr(J->R,mraEndTime)
     - call MRsvAdjustTimeframe(R)
  */


  MJobSetAttr(J,mjaReqAWDuration,(void **)&WallTime,mdfLong,mSet);

  J->WCLimit = WallTime;

  /* update reservation */

  if (J->Rsv != NULL)
    {
    if (AdjustReservation == TRUE)
      {
      mulong TimeDifference;

      TimeDifference = OrigWallTime - WallTime;

      /* Note that MRsvSetAttr calls the routine to adjust the time frame. */

      MRsvSetAttr(J->Rsv,mraEndTime,(void **)&TimeDifference,mdfLong,mDecr);

      MRsvAdjustTimeframe(J->Rsv);

      MDB(7,fSCHED) MLog("INFO:     job %s - adjusted reservation with wallclock time difference\n",
        J->Name);

      } /* END if (AdjustReservation == TRUE) */
    else
      {
      long StartTime = (J->StartTime != 0) ? J->StartTime : MSched.Time;

      MJobReleaseRsv(J,TRUE,TRUE);

      MDB(7,fSCHED) MLog("INFO:     job %s - increased wall time so release old reservation\n",
        J->Name);

      if (MJOBISACTIVE(J))
        {
        /* if the job is active then it is safe to create a new reservation.
           if the job has an idle reservation then it loses the reservation until next iteration */

        MDB(7,fSCHED) MLog("INFO:     job %s - job is active so create new reservation\n",
          J->Name);

        if (MRsvJCreate(J,NULL,StartTime,mjrActiveJob,NULL,FALSE) == FAILURE)
          {
          MDB(0,fSCHED) MLog("ERROR:    cannot create reservation for job '%s'\n",
            J->Name);
  
          if (EMsg != NULL)
            strcpy(EMsg,"cannot create new reservation for job");
  
          return(FAILURE);
          }
        } /* END if (MJOBISACTIVE(J) */
      } /* END else if (AdjustReservation == TRUE) */
    } /* END if (J->R != NULL) */

  return(SUCCESS);
  }  /* END MJobAdjustWallTime() */
/* END MJobAdjust.c */
