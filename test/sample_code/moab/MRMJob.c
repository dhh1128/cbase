/* HEADER */

      
/**
 * @file MRMJob.c
 *
 * Contains: RM Job functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"



/**
 * Send job/workload start request to RM.
 *
 * @param J (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobStart(

  mjob_t *J,    /* I */
  char   *EMsg, /* O (optional,minsize=MMAX_LINE) */
  int    *SC)   /* O (optional) */

  {
  int rqindex;
  int rmindex;
  int rmcount;

  int rc;

  mrm_t *R;

  int RMList[MMAX_RM];

  char  tmpLine[MMAX_LINE];

  char *MPtr;               /* message pointer */

  int   tmpSC;

  long  RMTime;

  const char *FName = "MRMJobStart";

  MDB(1,fRM) MLog("%s(%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  MDB(6,fRM) MLog("attempting to start job %s/%s\n",
    (J != NULL) ? J->Name : "NULL",
    ((J != NULL) && (J->AName != NULL)) ? J->AName : "NULL");

  MSchedSetActivity(macJobStart,J->Name);

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (J == NULL)
    {
    return(FAILURE);
    }

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmJobStart,mxoJob,(void *)J,NULL);

      sprintf(MPtr,"cannot start job - monitor mode");

      MDB(3,fRM) MLog("INFO:     cannot start job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;
 
      return(FAILURE);

      /*NOTREACHED*/
 
      break;

    case msmInteractive:

      {
      int nindex;
      int tindex;

      char Rsp[3];

      mbool_t Accept;

      mnode_t *N;

      char tmpBuf[MMAX_BUFFER];

      tmpBuf[0] = '\0';

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        for (tindex = 0;tindex < MNLGetTCAtIndex(&J->NodeList,nindex);tindex++)
          {
          if (tmpBuf[0] != '\0')
            MUStrCat(tmpBuf,",",sizeof(tmpBuf));

          MUStrCat(tmpBuf,N->Name,sizeof(tmpBuf));
          }  /* END for (tindex) */
        }    /* END for (nindex) */

      /* create message */

      fprintf(stdout,"Command:  start job %s on node list %s\n",
        J->Name,
        tmpBuf);

      /* while (fgetc(stdin) != EOF); */

      fprintf(stdout,"Accept:  (y/n) [default: n]?");

      if ((fgets(Rsp,3,stdin) != NULL) &&
         ((Rsp[0] == 'y') || (Rsp[0] == 'Y')))
        {
        Accept = TRUE;
        }
      else
        {
        Accept = FALSE;
        }

      fprintf(stdout,"\nRequest %s\n",
        (Accept == TRUE) ? "Accepted" : "Rejected");

      if (Accept == FALSE)
        {
        MDB(3,fRM) MLog("INFO:     cannot start job %s (command rejected by user)\n",
          J->Name);

        if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }
      }  /* END BLOCK */
      
      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */

  memset(RMList,0,sizeof(RMList));
 
  rmcount = 0;
 
  /* determine resource managers used */

  if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRMID != NULL))
    {
    /* workload RM explicitly specified */

    if (J->TemplateExtensions->WorkloadRM == NULL)
      MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM);

    if (J->TemplateExtensions->WorkloadRM == NULL)
      {
      sprintf(MPtr,"cannot locate target workload RM '%s'",
        J->TemplateExtensions->WorkloadRMID);

      MDB(2,fRM) MLog("INFO:     cannot start job %s - %s\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoEnt;

      return(FAILURE);
      }

    /* NOTE:  must confirm that migration to WorkloadRM has been successful */

    if (J->DestinationRM == J->TemplateExtensions->WorkloadRM)
      {
      RMList[0] = J->TemplateExtensions->WorkloadRM->Index;
      rmcount++;
      }
    }  /* END if ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */
  else
    {
    mrm_t *RQRM;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        continue;

      RQRM = &MRM[J->Req[rqindex]->RMIndex];

      if (J->Req[rqindex]->DRes.Procs == 0)
        {
        /* FIXME: Do not start a job that requires no procs */
        /*        What about utility environments and wacky jobs? */

        /* NOTE:  putting this in for licenses may want to restrict
                  check to only licenses */

        if (MReqIsSizeZero(J,J->Req[rqindex]) == TRUE)
          {
          /* allow execution for master task of 'zero proc' XT3 job */

          /* NO-OP */
          }
        else if ((bmisset(&RQRM->RTypes,mrmrtNetwork) ||
                  bmisset(&RQRM->RTypes,mrmrtStorage) ||
                  bmisset(&RQRM->RTypes,mrmrtLicense)) &&
                  (RQRM->ND.URL[mrmJobStart] != NULL))
          {
          /* extended resource required with explicitly specified job start
             interface */

          /* NO-OP */
          }
        else
          {
          continue;
          }
        }
 
      for (rmindex = 0;rmindex < rmcount;rmindex++)
        {
        if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
          {
          break;
          }
        }
 
      if (rmindex == rmcount)
        {
        RMList[rmindex] = J->Req[rqindex]->RMIndex;

        MDB(4,fRM) MLog("INFO:     will start job %s:%d utilizing RM[%d] '%s'\n",
          J->Name,
          rqindex,
          J->Req[rqindex]->RMIndex,
          MRM[RMList[rmindex]].Name);
 
        rmcount++;
        }
      }  /* END for (rqindex) */
    }    /* END else ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */

  /* perform pre-check */

  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    rmcount = 0;

    rc = SUCCESS;
    }
 
  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    if ((R->MaxJobPerMinute > 0) && 
        (R->IntervalStart + MCONST_MINUTELEN > MSched.Time))
      {
      if ((int)R->IntervalJobCount >= R->MaxJobPerMinute)
        {
        sprintf(MPtr,"cannot start job - RM max job per minute limit");

        MDB(3,fRM) MLog("INFO:     cannot start job %s (%s)\n",
          J->Name,
          MPtr);

        if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }
      }

    /* NOTE:  ignore R->FailIteration if async start is enabled */

    if ((R->FailIteration == MSched.Iteration) &&
         !bmisset(&R->Flags,mrmfASyncStart) &&
        (R->FailCount >= R->MaxFailCount))
      {
      sprintf(MPtr,"cannot start job - too many failures (%d)",
        R->FailCount);

      MDB(2,fRM) MLog("INFO:     cannot start job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;

      return(FAILURE);
      }

    if (MRMIsFuncDisabled(R,mrmJobStart) == TRUE)
      {
      return(FAILURE);
      }

    if (MRMFunc[R->Type].JobStart == NULL)
      {
      sprintf(MPtr,"cannot start job - RM '%s' does not support function '%s'",
        R->Name,
        MRMFuncType[mrmJobStart]);

      MDB(3,fRM) MLog("ALERT:    cannot start job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscSysFailure;

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  /* start job on specified RM's */

  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    mnode_t *N;
    enum MRMTypeEnum rmType;

    R = &MRM[RMList[rmindex]];

    MRMStartFunc(R,NULL,mrmJobStart);

    if ((R->SubType == mrmstXT4) &&
        (MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS) &&
        (strcmp(MPar[N->PtIndex].Name,"external") == 0))
      {
      rmType = mrmtPBS;
      }
    else
      {
      rmType = R->Type;
      }

    MUThread(
      (int (*)(void))MRMFunc[rmType].JobStart,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      4,
      J,
      R,
      MPtr,
      &tmpSC);

    MRMEndFunc(R,NULL,mrmJobStart,&RMTime);

    /* remove time from sched load and add to rm action load */

    MSched.LoadEndTime[mschpRMAction] += RMTime;
    MSched.LoadStartTime[mschpSched]  += RMTime;

    if (SC != NULL)
      *SC = tmpSC;

    if (rc != SUCCESS)
      {
      char tmpLine[MMAX_LINE];

      MDB(3,fRM) MLog("ALERT:    cannot start job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobStart]);

      if (MPtr[0] != '\0')
        {
        snprintf(tmpLine,MMAX_LINE,"job %s - %s",
          J->Name,
          MPtr);

        MRMSetFailure(R,mrmJobStart,(enum MStatusCodeEnum)tmpSC,tmpLine);
        }
      else if (rc == mscTimeout)
        {
        MRMSetFailure(R,mrmJobStart,mscSysFailure,"timeout");
        }
      else
        {
        snprintf(tmpLine,MMAX_LINE,"cannot start job %s",
          J->Name);

        MRMSetFailure(R,mrmJobStart,(enum MStatusCodeEnum)tmpSC,tmpLine);
        }

      if (rmindex > 0)
        {
        /* requeue successfully started reqs */

        MJobRequeue(J,NULL,"job start failed",NULL,NULL);
        }
 
      return(FAILURE);
      }

    /* update rm statistics */

    R->JobStartCount++;

    if (R->MaxJobPerMinute > 0)
      {
      if (R->IntervalStart + MCONST_MINUTELEN < MSched.Time)
        {
        R->IntervalStart = MSched.Time;
        R->IntervalJobCount = 0;
        }

      R->IntervalJobCount++;
      }
    }    /* END for (rmindex) */

  /* NOTE:  StartTime/DispatchTime typically updated in MJobStart() */
  /*        However, this information is required by MOWriteEvent() */

#ifdef MSIC
  {
  /* NOTE:  MSched.Time is only updated at the start of each iteration so may be
            stale by the time this particular jobs is started in environments with
            long-running scheduling cycles.  This can cause timeouts and time-based
            policies to trigger prematurely.  However, there may be negative side
            affects associated with J->StartTime being in the future relative to
            MSched.Time */

  mulong now;

  MUGetTime((mulong *)&now,mtmNONE,&MSched);

  J->StartTime     = now;
  J->DispatchTime  = now;
  }
#else /* MSIC */
  J->StartTime     = MSched.Time;
  J->DispatchTime  = MSched.Time;
#endif /* MSIC */

  bmunset(&J->IFlags,mjifWasRequeued);

  MOWriteEvent((void *)J,mxoJob,mrelJobStart,NULL,MStat.eventfp,NULL);

  CreateAndSendEventLog(meltJobStart,J->Name,mxoJob,(void *)J);

  return(SUCCESS);
  }  /* END MRMJobStart() */





/**
 * Send cancel job request to a job's resource managers (one per req). Note that
 * by specifing the RemoteOnly flag, the job will not be removed from Moab, but
 * only from any external RM queues.
 *
 * NOTE: In the future we would like to have this method accept a job and a single RM
 * to cancel on. Then MJobCancel() would cycle through each RM the job has and call this
 * method once for each RM. This would more closely match how other MRM functions operate.
 *
 * NOTE:  DANGER - J can be freed!
 *
 * @param J (I) 
 * @param Message (I) [optional]
 * @param DoRemoteOnly (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobCancel(
 
  mjob_t     *J,
  const char *Message,
  mbool_t     DoRemoteOnly,
  char       *EMsg,
  int        *SC)
 
  {
  int rqindex;
  int rmindex;
  int rmcount;
  int rc;
 
  mrm_t *R;

  int RMList[MMAX_RM];
  int tmpSC;

  char *MPtr;
  char  tmpLine[MMAX_LINE];

  long  RMTime;

  mreq_t *RQ;

  const char *FName = "MRMJobCancel";

  MDB(1,fRM) MLog("%s(%s,%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (Message != NULL) ? Message : "NULL",
    (DoRemoteOnly == TRUE) ? "TRUE" : "FALSE");

  MSchedSetActivity(macJobStart,J->Name);

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (J == NULL)
    {
    strcpy(MPtr,"invalid job");

    MDB(1,fRM) MLog("ALERT:    invalid job specified in %s\n",
      FName);

    return(FAILURE);
    }

  switch (MSched.Mode)
    {
    case msmInteractive:

      {
      char Rsp[3];

      mbool_t Accept;

      /* create message */

      fprintf(stdout,"Command:  cancel job %s in state %s (reason: '%s')\n",
        J->Name,
        MJobState[J->State],
        (Message != NULL) ? Message : "N/A");

      /* while (fgetc(stdin) != EOF); */

      fprintf(stdout,"Accept:  (y/n) [default: n]?");

      if ((fgets(Rsp,3,stdin) != NULL) && 
         ((Rsp[0] == 'y') || (Rsp[0] == 'Y')))
        {
        Accept = TRUE;
        }
      else
        {
        Accept = FALSE;
        }
          
      fprintf(stdout,"\nRequest %s\n",
        (Accept == TRUE) ? "Accepted" : "Rejected");

      if (Accept == FALSE)
        {
        MDB(3,fRM) MLog("INFO:     cannot cancel job %s (command rejected by user)\n",
          J->Name);

        sprintf(MPtr,"cannot cancel job '%s' (interactive mode)",
          J->Name);
      
         if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }
      }    /* END BLOCK */

      break;

    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmJobCancel,mxoJob,(void *)J,Message);

      MDB(3,fRM) MLog("INFO:     cannot cancel job '%s' (monitor mode)\n",
          J->Name);

      sprintf(MPtr,"cannot cancel job '%s' (monitor mode)",
        J->Name);
 
      return(FAILURE);

      break;
 
    default:

      /* NO-OP */
 
      break;
    }  /* END switch (MSched.Mode) */

  if (bmisset(&J->IFlags,mjifRequestForceCancel))
    {
    if (J->CancelCount == 0)
      {
      J->CancelReqTime = MSched.Time;
      }
    }
  else
    {
    if (bmisset(&J->IFlags,mjifIsExiting) ||
        bmisset(&J->IFlags,mjifWasCanceled))
      {
      /* job has already begun exiting */

      /* NO-OP */

      if (SC != NULL)
        *SC = mscPartial;

      strcpy(MPtr,"job previously cancelled");

      MDB(7,fRM) MLog("INFO:     job %s previously cancelled\n",
          J->Name);

      return(SUCCESS);
      }

    if (J->CancelCount == 0)
      {
      J->CancelReqTime = MSched.Time;
      }
    else if (J->CancelReqTime > MSched.Time)
      {
      char TString[MMAX_LINE];
      /* job cancel failed on previous attempt.  try again later */

      MULToTString(J->CancelReqTime - MSched.Time,TString);

      sprintf(MPtr,"previous failure - try again in %s",
        TString);

      MDB(1,fRM) MLog("ALERT:    previous failure attempting to cancel job in %s\n",
        FName);

      return(FAILURE);
      }
    else
      {
      J->CancelReqTime = 
        MSched.Time + 
        MAX(MSched.MaxRMPollInterval,(MAX(MSched.MaxRMPollInterval,90) << (J->CancelCount - 1)));

      /* escalate cancel attempt */

      bmset(&J->IFlags,mjifRequestForceCancel);
      }
    } 

  J->CancelCount++;
 
  memset(RMList,0,sizeof(RMList));
 
  rmcount = 0;
 
  /* determine resource managers used */

  if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRMID != NULL))
    {
    /* workload RM explicitly specified */

    if (J->TemplateExtensions->WorkloadRM == NULL)
      MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM);

    if (J->TemplateExtensions->WorkloadRM == NULL)
      {
      sprintf(MPtr,"cannot locate target workload RM '%s'",
        J->TemplateExtensions->WorkloadRMID);

      MDB(2,fRM) MLog("INFO:     cannot start job %s - %s\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoEnt;

      return(FAILURE);
      }

    /* NOTE:  must confirm that migration to WorkloadRM has been successful */

    if (J->DestinationRM == J->TemplateExtensions->WorkloadRM)
      {
      RMList[0] = J->TemplateExtensions->WorkloadRM->Index;
      rmcount++;
      }
    }  /* END if ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */
  else
    {
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        {
        /* end of list located */

        break;
        }
  
      if (RQ->RMIndex == -1)
        {
        /* req not attached to RM */

        continue;
        }

      if (bmisset(&J->Flags,mjfNoRMStart))
        {
        if ((J->Req[0] != NULL) &&
            (MRM[J->Req[0]->RMIndex].Type == mrmtMoab))
          {
          /* cancel migrated job on remote peer only */

          RMList[0] = J->Req[0]->RMIndex;

          if (rqindex == 0)
            rmcount++;
          }
        else
          {
          /* do not cancel a system job based on its allocated nodes' rm */

          RMList[0] = J->SubmitRM->Index;
  
          rmcount++;
  
          break;
          }
        }    /* END if (bmisset(&J->Flags,mjfNoRMStart)) */

#ifdef __MNOT
      if ((MRM[RQ->RMIndex].Type == mrmtMoab) &&
           bmisset(&J->Flags,mjfCoAlloc) &&
          (RQ->ReqJID == NULL))
       {
        /* job was not successfully staged - no remote cancel required */

        continue;
        }
#endif /* __MNOT */

      /* find duplicate RM entries in the job's reqs */

      for (rmindex = 0;rmindex < rmcount;rmindex++)
        {
        if (RMList[rmindex] == RQ->RMIndex)
          {
          break;
          }
        }    /* END for (rmindex) */
 
      /* if we didn't find a match already in the RMList, add the new RMIndex */

      if (rmindex == rmcount)
        {
        RMList[rmindex] = RQ->RMIndex;
 
        rmcount++;
        }
      }  /* END for (rqindex) */

    /* Cancel data staging requests. Should the cancelurl be called even if
     * request has already been staged? There may be one request staged and
     * another still pending. Should the cancelurl be called even if the 
     * request hasn't been made and let it be up to the cancelurl to know if a
     * request exists or not? Currently, let the datarm handle the request. */

    if (((J->DataStage != NULL) && 
         ((J->DataStage->SIData != NULL) || (J->DataStage->SOData != NULL))) && 
        (J->DestinationRM != NULL) &&
        (J->DestinationRM->DataRM != NULL) &&
        (J->DestinationRM->DataRM[0] != NULL) &&
        (J->DestinationRM->DataRM[0]->ND.Protocol[mrmJobCancel] != mbpNONE))
      {
      RMList[rmcount++] = J->DestinationRM->DataRM[0]->Index;
      }
    }

  if ((rmcount == 0) ||
     ((rmcount == 1) && bmisset(&MRM[RMList[0]].IFlags,mrmifLocalQueue)))
    {
    if ((J->DestinationRM == NULL) &&
        (DoRemoteOnly == FALSE))
      {
      /* job only exists in internal queue */

      MJobSetState(J,mjsRemoved);
      
      /* NOTE: MRMJobCancel should not remove system jobs, this causes other 
               routines to seg-fault */

      /* NOTE:  mjfSystemJob is not set on generic system jobs */

      if (!bmisset(&J->Flags,mjifGenericSysJob))
        {
        if (!bmisset(&J->IFlags,mjifWasCanceled))
          {
          /* update state modify time */

          J->StateMTime = MSched.Time;
          }

        bmset(&J->IFlags,mjifWasCanceled);

        MOWriteEvent((void *)J,mxoJob,mrelJobCancel,Message,MStat.eventfp,NULL);

        if ((Message != NULL) && (Message[0] != '\0'))
          CreateAndSendEventLogWithMsg(meltJobCancel,J->Name,mxoJob,(void *)J,Message);
        else
          CreateAndSendEventLog(meltJobCancel,J->Name,mxoJob,(void *)J);
        }
      else
        {
        /* system job being cancelled */

        /* if cancelled, due to admin action, or other, consider job to have failed */

        /* NOTE:  MLimitCancelJob() sets J->CompletionCode if job limits cause job cancel, but does
                  not cover the case of admin action or other forms of job failure */

        if (J->CompletionCode == 0)
          {
          J->CompletionCode = 1;

          MDB(4,fRM) MLog("INFO:     system job %s cancelled with no completion code specified - setting ccode=1\n",
            J->Name);
          }
        }

      MJobTransition(J,TRUE,FALSE);

      MDB(7,fRM) MLog("INFO:     job %s removed from local queue only\n",
          J->Name);

      return(SUCCESS);
      }  /* END if ((J->DRM == NULL) && ...) */
    }    /* END if ((rmcount == 0) || ...) */
 
  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    if (bmisset(&R->IFlags,mrmifLocalQueue) &&
        (DoRemoteOnly == TRUE))
      {
      /* do not remove jobs from local queues */

      continue;
      }

    if (MSched.Iteration == R->FailIteration)
      {
      char TString[MMAX_LINE];

      MDB(3,fRM) MLog("INFO:     cannot cancel job '%s' (fail iteration)\n",
        J->Name);

      MULToTString(MSched.MaxRMPollInterval,TString);

      sprintf(MPtr,"previous failure - try again in %s",
        TString);
 
      return(FAILURE);
      }

    if (R->State == mrmsDisabled)
      {
      MDB(3,fRM) MLog("INFO:     cannot cancel job '%s', RM '%s' is disabled\n",
        J->Name,
        R->Name);

      sprintf(MPtr,"cannot cancel jobs on RM '%s' because it is disabled",
        R->Name);

      return(FAILURE);
      }
 
    if (MRMFunc[R->Type].JobCancel == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot cancel job %s (RM '%s' does not support function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobCancel]);

      sprintf(MPtr,"cannot cancel job through RM %s - RM doesn't support cancel function",
        R->Name);
 
      return(FAILURE);
      }

    bmclear(&J->Hold);  /* remove any holds before cancel */

    MRMStartFunc(R,NULL,mrmJobCancel);
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobCancel,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      5,
      J,
      R,
      Message,
      MPtr,
      &tmpSC);

    if ((SC != NULL) && (*SC == mscNoError))
      *SC = tmpSC;

    MRMEndFunc(R,NULL,mrmJobCancel,&RMTime);

    /* remove time from sched load and add to rm action load */

    MSched.LoadEndTime[mschpRMAction] += RMTime;
    MSched.LoadStartTime[mschpSched]  += RMTime;

    /* unset this flag so that the next time we come around we don't force it */

    bmunset(&J->IFlags,mjifRequestForceCancel);
 
    if (rc != SUCCESS)
      {
      char tmpEMsg[MMAX_LINE];

      if (tmpSC == mscPending)
        {
        /* assume that the job will cancel and that no failure occured */

        continue;
        }

      MDB(3,fRM) MLog("ALERT:    cannot cancel job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobCancel]);

      if (MPtr[0] != '\0')
        MRMSetFailure(R,mrmJobCancel,(enum MStatusCodeEnum)tmpSC,MPtr);
      else if (rc == mscTimeout)
        MRMSetFailure(R,mrmJobCancel,mscSysFailure,"timeout");
      else
        MRMSetFailure(R,mrmJobCancel,(enum MStatusCodeEnum)tmpSC,"cannot cancel job");

      if (tmpSC == mscPartial)
        continue;

      /* force cancel if remote error occurs - in the future differentiate
       * better between "remote" failures */

      /* This code is too generic--causes major problems in grid.

      if (tmpSC == mscRemoteFailure)
        continue;
      */

      if (Message != NULL)      
        {
        snprintf(tmpEMsg,sizeof(tmpEMsg),"job cannot be cancelled, reason='%s' - %s",
          Message,
          MPtr);
        }
      else
        {
        snprintf(tmpEMsg,sizeof(tmpEMsg),"job cannot be cancelled - %s",
          MPtr);
        }

      MMBAdd(&J->MessageBuffer,tmpEMsg,NULL,mmbtNONE,0,0,NULL);
  
      return(FAILURE);
      }  /* END if (rc != SUCCESS) */
    }    /* END for (rmindex) */

  /* if we are not deleting the entire job, we do not want
   * to mark the job as dying, nor do we want to record a full cancel
   * event */

  if (DoRemoteOnly == FALSE)
    {
    bmset(&J->IFlags,mjifIsExiting);

    if (!bmisset(&J->IFlags,mjifWasCanceled))
      {
      /* update state modify time */

      J->StateMTime = MSched.Time;
      }

    bmset(&J->IFlags,mjifWasCanceled);

    if ((Message != NULL) && (Message[0] != '\0'))
      {
      char tmpMsg[MMAX_LINE];

      snprintf(tmpMsg,sizeof(tmpMsg),"job cancelled - %s",
        Message);

      MMBAdd(&J->MessageBuffer,tmpMsg,NULL,mmbtNONE,0,0,NULL);
      }

    MOWriteEvent((void *)J,mxoJob,mrelJobCancel,Message,MStat.eventfp,NULL);

    if (MSched.PushEventsToWebService == TRUE)
      { 
      MEventLog *Log = new MEventLog(meltJobCancel);
      Log->SetCategory(melcCancel);
      Log->SetFacility(melfJob);
      Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
      if ((Message != NULL) && (Message[0] != '\0'))
        {
        Log->AddDetail("msg",Message);
        }

      MEventLogExportToWebServices(Log);

      delete Log;
      }
    }  /* END if (DoRemoteOnly == FALSE) */

  if (MSched.LimitedJobCP == FALSE)
    MJobTransition(J,TRUE,FALSE);

  MDB(7,fRM) MLog("INFO:     job %s successfully cancelled - %s\n",
      J->Name,
      Message != NULL ? Message : "");
 
  return(SUCCESS);
  }  /* END MRMJobCancel() */





/**
 * Issue job modify request to resource manager.
 *
 * NOTE: if J->SRM is set, modify J->SRM, otherwise modify all RM's listed in 
 *       individual reqs
 *
 * NOTE: do not modify internal queue jobs.
 *
 * @see MNatJobModify() - child
 *
 * @param J (I)
 * @param Attr (I)
 * @param SubAttr (I)
 * @param Value (I)
 * @param Op (I)
 * @param Msg (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobModify(

  mjob_t                 *J,
  const char             *Attr,
  const char             *SubAttr,
  const char             *Value,
  enum MObjectSetModeEnum Op,
  const char             *Msg,
  char                   *EMsg,
  int                    *SC)
 
  {
  int rqindex;
  int rmindex = 0;
  int rmcount;
 
  int rc;

  mrm_t *R;
 
  int RMList[MMAX_RM];

  char *MPtr;

  char  tmpLine[MMAX_LINE];
  char tmpMsg[MMAX_LINE];

  const char *FName = "MRMJobModify";
 
  MDB(1,fRM) MLog("%s(%s,%s,%s,%.32s,%d,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (Attr != NULL) ? Attr : "NULL",
    (SubAttr != NULL) ? SubAttr : "NULL",
    (Value != NULL) ? Value : "NULL",
    Op,
    (Msg != NULL) ? Msg : "NULL");

  MPtr = (EMsg != NULL) ? EMsg : tmpLine;

  MPtr[0] = '\0';
 
  if (SC != NULL)
    *SC = 0;
 
  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((J->State == mjsCompleted) || (J->State == mjsRemoved))
    {
    MDB(3,fRM) MLog("INFO:     not modifing job '%s' through RM (job state is %s)\n",
      J->Name,
      MJobState[J->State]);

    return(SUCCESS);
    }

  if ((J->DestinationRM == NULL) && (J->SubmitRM == MSched.InternalRM))
    {
    /* job has not been migrated, still in internal RM, just return SUCCESS */

    return(SUCCESS);
    }

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmJobModify,mxoJob,(void *)J,Msg);
 
      MDB(3,fRM) MLog("INFO:     cannot modify job '%s' (monitor mode)\n",
        J->Name);
 
      return(FAILURE);
 
      /*NOTREACHED*/
 
      break;
 
    default:

      /* NO-OP */
 
      break;
    }  /* END switch (MSched.Mode) */
 
  memset(RMList,0,sizeof(RMList));
 
  rmcount = 0;

  /* determine resource managers used */

  if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRMID != NULL))
    {
    /* workload RM explicitly specified */

    if (J->TemplateExtensions->WorkloadRM == NULL)
      MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM);

    if (J->TemplateExtensions->WorkloadRM == NULL)
      {
      if (EMsg != NULL)
        sprintf(EMsg,"cannot locate target workload RM '%s'",
          J->TemplateExtensions->WorkloadRMID);

      MDB(2,fRM) MLog("INFO:     cannot modify job %s - %s\n",
        J->Name,
        (EMsg != NULL) ? EMsg : "???");

      if (SC != NULL)
        *SC = mscNoEnt;

      return(FAILURE);
      }

    /* NOTE:  must confirm that migration to WorkloadRM has been successful */

    if (J->DestinationRM == J->TemplateExtensions->WorkloadRM)
      {
      RMList[0] = J->TemplateExtensions->WorkloadRM->Index;
      rmcount++;
      }
    }  /* END if ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */
  else
    {
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        continue;

      if (J->Req[rqindex]->RMIndex == -1)
        continue;

      /* see if current Req->RMIndex has already been seen */

      for (rmindex = 0;rmindex < rmcount;rmindex++)
        {
        if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
          {
          break;
          }
        }

      /* we haven't seen this RM before -- add it to the end of our RMList */

      if ((rmindex == rmcount) && !bmisset(&MRM[J->Req[rqindex]->RMIndex].IFlags,mrmifLocalQueue))
        {
        RMList[rmindex] = J->Req[rqindex]->RMIndex;
 
        rmcount++;
        }
      }  /* END for (rqindex) */
    }    /* END else ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */

  /* if we don't have any destination RM's attached to reqs, use other
   * RM pointers to modify the job */

  if (rmcount == 0)
    {
    if (J->DestinationRM != NULL)
      {
      RMList[0] = J->DestinationRM->Index;

      rmcount = 1;
      }
    else if ((J->SubmitRM != NULL) && !bmisset(&MRM[rmindex].IFlags,mrmifLocalQueue))
      {
      /* I think that this is never a good idea - JMB */

      RMList[0] = J->SubmitRM->Index;

      rmcount = 1;
      }
    else if ((J->SubmitRM != NULL) && bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue))
      {
      /* no need to modify jobs that have not been migrated (e.g. held jobs) */
      /* any modifications to the unmigrated job should have been made before calling this routine (e.g. in __MUIJobModify()) */
      }
    }

  if ((bmisset(&J->Flags,mjfNoRMStart) == TRUE) && 
      (bmisset(&J->Flags,mjfClusterLock) == FALSE)) /* allow modification from the grid head if job is clusterlocked. */ 
    {
    /* not clusterlocked */

    rmcount = 0;

    rc = SUCCESS;
    }

  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];
 
    if ((MSched.Iteration == R->FailIteration) &&
        (R->FailCount >= R->MaxFailCount))
      {
      MDB(3,fRM) MLog("INFO:     cannot modify job '%s' (fail iteration)\n",
        J->Name);

      if (EMsg != NULL)
        strcpy(EMsg,"cannot modify job - fail iteration");
 
      return(FAILURE);
      }
 
    if (MRMFunc[R->Type].JobModify == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot modify job %s (RM '%s' does not support function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobModify]);

      if (EMsg != NULL)
        strcpy(EMsg,"cannot modify job - function not supported");
 
      return(FAILURE);
      }
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobModify,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      8,
      J,
      R,
      Attr,
      SubAttr,
      Value,
      Op,
      MPtr,    /* O */
      SC);     /* O */
 
    if (rc != SUCCESS)
      {
      char tmpMsg[MMAX_LINE];

      if (Msg != NULL)
        {
        snprintf(tmpMsg,sizeof(tmpMsg),"cannot modify job, reason='%s' - %s",
          Msg,
          MPtr);
        }
      else
        {
        snprintf(tmpMsg,sizeof(tmpMsg),"cannot modify job - %s",
          MPtr);
        }

      MMBAdd(&R->MB,tmpMsg,NULL,mmbtNONE,0,0,NULL);

      MMBAdd(&J->MessageBuffer,tmpMsg,NULL,mmbtNONE,0,0,NULL);

      MDB(3,fRM) MLog("ALERT:    cannot modify job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobModify]);
 
      return(FAILURE);
      }
    }    /* END for (rmindex) */

  snprintf(tmpMsg,sizeof(tmpMsg),"%s%s%s=%s %s",
    Attr,
    (SubAttr != NULL) ? "." : "",
    (SubAttr != NULL) ? SubAttr : "",
    (Value != NULL) ? Value : "",
    (Msg != NULL) ? Msg : "-");

  if (bmisset(&MSched.RecordEventList,mrelJobModify))
    {
    MOWriteEvent((void *)J,mxoJob,mrelJobModify,tmpMsg,MStat.eventfp,NULL);
    }

  CreateAndSendEventLogWithMsg(meltJobModify,J->Name,mxoJob,(void *)J,tmpMsg);

  return(SUCCESS);
  }  /* END MRMJobModify() */





/**
 * 'Live' migrate job tasks (ie, VM's) to specified nodelist (physical nodes).
 *
 * NOTE:  Can only be used with active jobs
 *
 * @see MJobMigrate() - used to migrate/submit job to specified RM - NOT RELATED
 * @see MNatJobMigrate() - child (used w/native RM only)
 * @see MVMScheduleMigration() - schedule system directed migration operations
 * @see __MUIJobMigrate() - schedule admin requested migration operations
 *
 * @param J (I) [optional]
 * @param SrcVMList (I) [optional]
 * @param DstNL (I)
 * @param SC (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MRMJobMigrate(

  mjob_t     *J,            /* I (optional) */
  char      **SrcVMList,    /* I (optional) */
  mnl_t      *DstNL,        /* I */
  char       *EMsg,         /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC) /* O (optional) */

  {
  int rqindex;
  int rmindex;
  int rmcount;

  int rc;

  mrm_t *R;

  int RMList[MMAX_RM];

  long RMTime = 0;

  mnode_t *N;

  const char *FName = "MRMJobMigrate";

  MDB(1,fRM) MLog("%s(%s,SrcVMList,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (DstNL != NULL) ? "DstNL" : "NULL");

  if (SC != NULL)
    *SC = mscNoError;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmJobMigrate,mxoJob,(void *)J,NULL);

      MDB(3,fRM) MLog("INFO:     cannot migrate job '%s' (monitor mode)\n",
        (J != NULL) ? J->Name : "");

      return(FAILURE);

      /*NOTREACHED*/

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */

  memset(RMList,0,sizeof(RMList));

  if ((DstNL != NULL) && (MNLGetNodeAtIndex(DstNL,0,&N) == SUCCESS) && (N->ProvRM != NULL))
    {
    /* NOTE:  should allow multiple ProvRM's per system (FIXME) */

    RMList[0] = N->ProvRM->Index;

    rmcount = 1;
    }
  else if (MSched.ProvRM != NULL)
    {
    /* NOTE:  should allow multiple ProvRM's per system (FIXME) */

    RMList[0] = MSched.ProvRM->Index;

    rmcount = 1;
    }
  else 
    {
    if (J == NULL)
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot locate provisioning RM to perform operation");
        }

      return(FAILURE);
      }

    rmcount = 0;

    /* determine resource managers used */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        continue;

      for (rmindex = 0;rmindex < rmcount;rmindex++)
        {
        if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
          {
          break;
          }
        }    /* END for (rmindex) */

      if (rmindex == rmcount)
        {
        RMList[rmindex] = J->Req[rqindex]->RMIndex;

        rmcount++;
        }
      }  /* END for (rqindex) */
    }

  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    if (MRMFunc[R->Type].JobMigrate == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot migrate job %s (RM '%s' does not support function '%s')\n",
        (J != NULL) ? J->Name : "",
        R->Name,
        MRMFuncType[mrmJobMigrate]);

      return(FAILURE);
      }

    MRMStartFunc(R,NULL,mrmJobMigrate);

    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobMigrate,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      6,
      J,
      R,
      SrcVMList,
      DstNL,
      EMsg,
      SC);

    MRMEndFunc(R,NULL,mrmJobMigrate,&RMTime);

    /* remove time from sched load and add to rm action load */

    MSched.LoadEndTime[mschpRMAction] += RMTime;
    MSched.LoadStartTime[mschpSched]  += RMTime;

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot migrate job %s (RM '%s' failed in function '%s')\n",
        (J != NULL) ? J->Name : "",
        R->Name,
        MRMFuncType[mrmJobMigrate]);
      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        snprintf(EMsg,MMAX_LINE,"cannot perform migration operation - operation failed");
        }

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        snprintf(EMsg,MMAX_LINE,"cannot perform migration operation - operation failed");
        }

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  MOWriteEvent(
    (void *)J,
    mxoJob,
    mrelJobMigrate,
    "job tasks migrated",
    MStat.eventfp,
    NULL);

  return(SUCCESS);
  }  /* END MRMJobMigrate() */





/**
 * Issue general requeue request to resource manager.
 *
 * @see MJobPreempt()
 *
 * @param J (I) [modified]
 * @param JPeer (I) [optional]
 * @param Message (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobRequeue(
 
  mjob_t     *J,
  mjob_t    **JPeer,
  const char *Message,
  char       *EMsg,
  int        *SC)
 
  {
  int rqindex;
  int rmindex;
  int rmcount;
 
  int rc;
 
  mrm_t *R = NULL;
 
  int RMList[MMAX_RM];

  char     tmpLine[MMAX_LINE];
  char    *MPtr;

  const char *FName = "MRMJobRequeue";
 
  MDB(1,fRM) MLog("%s(%s,%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (JPeer != NULL) ? "JPeer" : "NULL",
    (Message != NULL) ? Message : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  need req[x]->EState, and need force flag to MRMJobRequeue() */

  /* NYI */

  MPtr = (EMsg != NULL) ? EMsg : tmpLine;

  MPtr[0] = '\0';
  
  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmJobRequeue,mxoJob,(void *)J,Message);
 
      MDB(3,fRM) MLog("INFO:     cannot requeue job '%s' (monitor mode)\n",
        J->Name);
 
      return(FAILURE);
 
      /*NOTREACHED*/
 
      break;
 
    default:

      /* NO-OP */
 
      break;
    }  /* END switch (MSched.Mode) */

  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    MJobReleaseRsv(J,TRUE,TRUE);
   
    J->AWallTime = 0;
   
    if ((Message != NULL) && (Message[0] != '\0'))
      {
      char tmpLine[MMAX_LINE];
   
      snprintf(tmpLine,sizeof(tmpLine),"job requeued - %s",
        Message);
   
      MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
      }

    bmset(&J->IFlags,mjifWasRequeued);

    MJobSetState(J,mjsIdle);

    return(SUCCESS);
    }

  memset(RMList,0,sizeof(RMList));
 
  rmcount = 0;
 
  /* determine resource managers used */
 
  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      continue;
 
    for (rmindex = 0;rmindex < rmcount;rmindex++)
      {
      if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
        {
        break;
        }
      }
 
    if (rmindex == rmcount)
      {
      RMList[rmindex] = J->Req[rqindex]->RMIndex;
 
      rmcount++;
      }
    }  /* END for (rqindex) */
 
  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];
 
    if (MRMFunc[R->Type].JobRequeue == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot requeue job %s (RM '%s' does not support function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobRequeue]);
 
      return(FAILURE);
      }

    if (MSched.Mode == msmNormal)
      {
      MOSSyslog(LOG_ERR,"INFO:  requeuing job %s",
        J->Name);
      }
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobRequeue,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      5,
      J,
      R,
      JPeer,
      MPtr,
      SC);
 
    if (rc != SUCCESS)
      {
      char tmpLine[MMAX_LINE];

      MDB(3,fRM) MLog("ALERT:    cannot requeue job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobRequeue]);

      if ((Message != NULL) && (Message[0] != '\0'))
        {
        snprintf(tmpLine,sizeof(tmpLine),"cannot requeue job, reason='%s' - %s",
          Message,
          MPtr);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"cannot requeue job - %s",
          MPtr);
        }

      MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
 
      return(FAILURE);
      }
    
    bmset(&J->IFlags,mjifWasRequeued);
    }    /* END for (rmindex) */

  /* job requeue successful */

  /* adjust reserved resources */

  MJobReleaseRsv(J,TRUE,TRUE);

  J->AWallTime = 0;

  if ((Message != NULL) && (Message[0] != '\0'))
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"job requeued - %s",
      Message);

    MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
    }

  if (bmisset(&J->IFlags,mjifAllocPartitionCreated))
    MJobDestroyAllocPartition(J,R,"job requeued");

  MOWriteEvent((void *)J,mxoJob,mrelJobPreempt,Message,MStat.eventfp,NULL);
 
  return(SUCCESS);
  }  /* END MRMJobRequeue() */





/**
 * Issue job submission request to resource manager.
 *
 * If JobName populated, use as job id
 *
 * @see MUIJobSubmit() - parent
 *
 * @param JobDesc (I) [optional]
 * @param R (I)
 * @param JP (I/O)
 * @param FlagBM (I) [stage cmd file, do not submit job to RM]
 * @param JobName (I/O) [optional,minsize=MMAX_LINE]
 * @param EMsg (O) [optional,minsize=MMAX_LINE or MMAX_BUFFER if mjfTest]
 * @param SC (O) [optional]
 */

int MRMJobSubmit(

  char      *JobDesc,
  mrm_t     *R,
  mjob_t   **JP,
  mbitmap_t *FlagBM,
  char      *JobName,
  char      *EMsg,
  enum MStatusCodeEnum *SC)

  {
  int  rc;
  int  tmpSC = 0;

  long RMTime;

  char tEMsg[MMAX_LINE];

  const char *FName = "MRMJobSubmit";

  MDB(1,fRM) MLog("%s(%.256s,%s,JP,Flags,JobName,EMsg,SC)\n",
    FName,
    (JobDesc != NULL) ? JobDesc : "",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      if (getenv("MFORCESUBMIT") == NULL)
        {
        if (JP != NULL)
          MSchedAddTestOp(mrmJobSubmit,mxoJob,(void *)*JP,NULL);

        MDB(3,fRM) MLog("INFO:     cannot submit job (monitor mode)\n");

        if (EMsg != NULL)
          sprintf(EMsg,"cannot submit job in monitor mode\n");

        if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }

      /*NOTREACHED*/

      break;

    case msmInteractive:

      {
      char Rsp[3];

      mbool_t Accept;

      /* create message */

      if ((JP != NULL) && (*JP != NULL))
        {
        char TString[MMAX_LINE];
        mjob_t *J;

        J = *JP;

        MULToTString(J->WCLimit,TString);

        fprintf(stdout,"Command:  submit job '%s' (user: %s  procs: %d  walltime: %s)\n",
          J->Name,
          J->Credential.U->Name,
          J->TotalProcCount,
          TString);
        }
      else
        {
        fprintf(stdout,"Command:  submit job '%s' ('%.24s...')\n",
          JobDesc,
          JobDesc);
        }

      /* while (fgetc(stdin) != EOF); */

      while(TRUE)
        {
        Rsp[0] = '\0';

        if (fgets(Rsp,3,stdin) != NULL)
          {
          if (Rsp[0] == '\0')
            continue;

          Accept = ((Rsp[0] == 'y') || (Rsp[0] == 'Y')) ? TRUE : FALSE;
          break;
          }
        }

      fprintf(stdout,"\nRequest %s\n",
        (Accept == TRUE) ? "Accepted" : "Rejected");

      if (Accept == FALSE)
        {
        MDB(3,fRM) MLog("INFO:     cannot submit job (command rejected by user)\n");

        if (EMsg != NULL)
          sprintf(EMsg,"job submit rejected by admin (interactive mode)\n");

        if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */

  if (MRMFunc[R->Type].JobSubmit == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot submit job to RM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmJobSubmit]);

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"RM %s (TYPE:%s) does not support job submit",
        R->Name,
        MRMType[R->Type]);
      }

    return(FAILURE);
    }

  MRMStartFunc(R,NULL,mrmJobSubmit);

  MUThread(
    (int (*)(void))MRMFunc[R->Type].JobSubmit,
    R->P[0].Timeout,
    &rc,
    NULL,
    R,
    7,
    JobDesc,
    R,
    JP,
    FlagBM,
    JobName,  /* O */
    tEMsg,    /* O */
    &tmpSC);  /* O */

  MRMEndFunc(R,NULL,mrmJobSubmit,&RMTime);

  /* remove time from sched load and add to rm action load */

  MSched.LoadEndTime[mschpRMAction] += RMTime;
  MSched.LoadStartTime[mschpSched]  += RMTime;

  if (SC != NULL)
    *SC = (enum MStatusCodeEnum)tmpSC;

  if (rc != SUCCESS)
    {
    MDB(3,fRM) MLog("ALERT:    cannot submit job (RM '%s' failed in function '%s')\n",
      R->Name,
      MRMFuncType[mrmJobSubmit]);

    if (EMsg != NULL)
      MUStrCpy(EMsg,tEMsg,MMAX_LINE);

    if (tEMsg[0] != '\0')
      {
      char tmpLine[MMAX_LINE];

      snprintf(tmpLine,sizeof(tmpLine),"job submit failed - %s",
        tEMsg);

      MRMSetFailure(R,mrmJobSubmit,(enum MStatusCodeEnum)tmpSC,tmpLine);
      }
 
    return(FAILURE);
    }  /* END if (rc != SUCCESS) */

  return(SUCCESS);
  }  /* END MRMJobSubmit() */





/**
 * Issue job signal request to resource manager.
 *
 * @param J (I)
 * @param Sig (I) [optional, -1 = not set]
 * @param Action (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobSignal(

  mjob_t  *J,      /* I */
  int      Sig,    /* I (optional, -1 = not set) */
  char    *Action, /* I non-signal based action (optional) */
  char    *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  int     *SC)     /* O (optional) */

  {
  int rqindex;
  int rmindex;
  int rmcount;

  int rc;

  mrm_t *R;

  int    RMList[MMAX_RM];

  long   RMTime;

  char   tmpLine[MMAX_LINE];
  char  *MPtr;

  int    tmpSC;

  const char *FName = "MRMJobSignal";

  MDB(1,fRM) MLog("%s(%s,%d,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    Sig,
    (Action != NULL) ? Action : "NULL");

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (MJOBISACTIVE(J) == FALSE)
    {
    sprintf(MPtr,"cannot signal job - job is not active");

    MDB(3,fRM) MLog("INFO:     cannot signal job %s (%s)\n",
      J->Name,
      MPtr);

    if (SC != NULL)
      *SC = mscBadState;

    return(FAILURE);
    }

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmXJobSignal,mxoJob,(void *)J,NULL);

      sprintf(MPtr,"cannot signal job - monitor mode");

      MDB(3,fRM) MLog("INFO:     cannot signal job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;

      return(FAILURE);

      /*NOTREACHED*/

      break;

    case msmInteractive:

      {
      int nindex;
      int tindex;

      char Rsp[3];

      mbool_t Accept;

      mnode_t *N;

      char tmpBuf[MMAX_BUFFER];

      tmpBuf[0] = '\0';

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        for (tindex = 0;tindex < MNLGetTCAtIndex(&J->NodeList,nindex);tindex++)
          {
          if (tmpBuf[0] != '\0')
            MUStrCat(tmpBuf,",",sizeof(tmpBuf));

          MUStrCat(tmpBuf,N->Name,sizeof(tmpBuf));
          }  /* END for (tindex) */
        }    /* END for (nindex) */

      /* create message */

      fprintf(stdout,"Command:  signal job %s on node list %s\n",
        J->Name,
        tmpBuf);

      /* while (fgetc(stdin) != EOF); */

      fprintf(stdout,"Accept:  (y/n) [default: n]?");

      if ((fgets(Rsp,3,stdin) != NULL) &&
         ((Rsp[0] == 'y') || (Rsp[0] == 'Y')))
        {
        Accept = TRUE;
        }
      else
        {
        Accept = FALSE;
        }

      fprintf(stdout,"\nRequest %s\n",
        (Accept == TRUE) ? "Accepted" : "Rejected");

      if (Accept == FALSE)
        {
        MDB(3,fRM) MLog("INFO:     cannot signal job %s (command rejected by user)\n",
          J->Name);

        if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */

  memset(RMList,0,sizeof(RMList));

  rmcount = 0;

  /* determine resource managers used */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      continue;

    for (rmindex = 0;rmindex < rmcount;rmindex++)
      {
      if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
        {
        break;
        }
      }

    if (rmindex == rmcount)
      {
      RMList[rmindex] = J->Req[rqindex]->RMIndex;

      rmcount++;
      }
    }  /* END for (rqindex) */

  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    /* NOTE:  ignore R->FailIteration if async start is enabled */

    if ((R->FailIteration == MSched.Iteration) &&
        (R->FailCount >= R->MaxFailCount))
      {
      sprintf(MPtr,"cannot signal job - fail iteration");

      MDB(2,fRM) MLog("INFO:     cannot signal job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;

      return(FAILURE);
      }

    if (MRMFunc[R->Type].JobSignal == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot signal job %s (RM '%s' does not support function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmXJobSignal]);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot signal job %s (RM '%s' does not support function '%s')\n",
          J->Name,
          R->Name,
          MRMFuncType[mrmXJobSignal]);
        }

      return(FAILURE);
      }

    MRMStartFunc(R,NULL,mrmXJobSignal);

    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobSignal,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      6,
      J,
      R,
      Sig,
      Action,
      MPtr,
      &tmpSC);

    if (SC != NULL)
      *SC = tmpSC;

    MRMEndFunc(R,NULL,mrmXJobSignal,&RMTime);

    /* remove time from sched load and add to rm action load */

    MSched.LoadEndTime[mschpRMAction] += RMTime;
    MSched.LoadStartTime[mschpSched]  += RMTime;

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot signal job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmXJobSignal]);

      if (MPtr[0] != '\0')
        MRMSetFailure(R,mrmXJobSignal,(enum MStatusCodeEnum)tmpSC,MPtr);
      else if (rc == mscTimeout)
        MRMSetFailure(R,mrmXJobSignal,(enum MStatusCodeEnum)tmpSC,"timeout");
      else
        MRMSetFailure(R,mrmXJobSignal,(enum MStatusCodeEnum)tmpSC,"cannot signal job");

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  /* job signal successful */

  /* NOTE:  no record of signal action written to event log */

  return(SUCCESS);
  }  /* END MRMJobSignal() */





/**
 * Issue job suspend request to resource manager
 *
 * @see MJobPreempt()
 *
 * @param J (I)
 * @param Message (I) reason job was suspended [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobSuspend(
 
  mjob_t     *J,
  const char *Message,
  char       *EMsg,
  int        *SC)
 
  {
  int rqindex;
  int rmindex;
  int rmcount;
 
  int rc;
 
  mrm_t *R;
 
  int    RMList[MMAX_RM];

  long   RMTime;

  char   tmpLine[MMAX_LINE];
  char  *MPtr;

  int    tmpSC;

  const char *FName = "MRMJobSuspend";
 
  MDB(1,fRM) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (Message != NULL) ? Message : "NULL");

  MSchedSetActivity(macJobSuspend,J->Name);

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';
 
  if (J == NULL)
    {
    return(FAILURE);
    }
 
  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmJobSuspend,mxoJob,(void *)J,NULL);

      sprintf(MPtr,"cannot suspend job - monitor mode");

      MDB(3,fRM) MLog("INFO:     cannot suspend job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;

      return(FAILURE);

      /*NOTREACHED*/

      break;

    case msmInteractive:

      {
      int nindex;
      int tindex;

      char Rsp[3];

      mbool_t Accept;

      mnode_t *N;

      char tmpBuf[MMAX_BUFFER];

      tmpBuf[0] = '\0';

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        for (tindex = 0;tindex < MNLGetTCAtIndex(&J->NodeList,nindex);tindex++)
          {
          if (tmpBuf[0] != '\0')
            MUStrCat(tmpBuf,",",sizeof(tmpBuf));

          MUStrCat(tmpBuf,N->Name,sizeof(tmpBuf));
          }  /* END for (tindex) */
        }    /* END for (nindex) */

      /* create message */

      fprintf(stdout,"Command:  suspend job %s on node list %s\n",
        J->Name,
        tmpBuf);

      /* while (fgetc(stdin) != EOF); */

      fprintf(stdout,"Accept:  (y/n) [default: n]?");

      if ((fgets(Rsp,3,stdin) != NULL) &&
         ((Rsp[0] == 'y') || (Rsp[0] == 'Y')))
        {
        Accept = TRUE;
        }
      else
        {
        Accept = FALSE;
        }

      fprintf(stdout,"\nRequest %s\n",
        (Accept == TRUE) ? "Accepted" : "Rejected");

      if (Accept == FALSE)
        {
        MDB(3,fRM) MLog("INFO:     cannot suspend job %s (command rejected by user)\n",
          J->Name);

        if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */
 
  memset(RMList,0,sizeof(RMList));
 
  rmcount = 0;
 
  /* determine resource managers used */
 
  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      continue;
 
    for (rmindex = 0;rmindex < rmcount;rmindex++)
      {
      if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
        {
        break;
        }
      }
 
    if (rmindex == rmcount)
      {
      RMList[rmindex] = J->Req[rqindex]->RMIndex;
 
      rmcount++;
      }
    }  /* END for (rqindex) */

  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    rmcount = 0;

    rc = SUCCESS;
    }
 
  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    /* NOTE:  ignore R->FailIteration if async start is enabled */

    if ((R->FailIteration == MSched.Iteration) &&
         !bmisset(&R->Flags,mrmfASyncStart) &&
        (R->FailCount >= R->MaxFailCount))
      {
      sprintf(MPtr,"cannot suspend job - fail iteration");

      MDB(2,fRM) MLog("INFO:     cannot suspend job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;

      return(FAILURE);
      }
 
    if (MRMFunc[R->Type].JobSuspend == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot suspend job %s (RM '%s' does not support function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobSuspend]);
 
      return(FAILURE);
      }

    MRMStartFunc(R,NULL,mrmJobSuspend);
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobSuspend,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      4,
      J,
      R,
      MPtr,
      &tmpSC);

    if (SC != NULL)
      *SC = tmpSC;

    MRMEndFunc(R,NULL,mrmJobSuspend,&RMTime);

    /* remove time from sched load and add to rm action load */

    MSched.LoadEndTime[mschpRMAction] += RMTime;
    MSched.LoadStartTime[mschpSched]  += RMTime;

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot suspend job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobSuspend]);

      if (MPtr[0] != '\0')
        MRMSetFailure(R,mrmJobSuspend,(enum MStatusCodeEnum)tmpSC,MPtr);
      else if (rc == mscTimeout)
        MRMSetFailure(R,mrmJobSuspend,(enum MStatusCodeEnum)tmpSC,"timeout");
      else
        MRMSetFailure(R,mrmJobSuspend,(enum MStatusCodeEnum)tmpSC,"cannot suspend job");

      if ((Message != NULL) && (Message[0] != '\0'))
        {
        snprintf(tmpLine,sizeof(tmpLine),"cannot suspend job, reason='%s' - %s",
          Message,
          MPtr);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"cannot suspend job - %s",
          MPtr);
        }

      MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  /* job suspension successful */

  /* adjust reserved resources - should release only partial resources (NYI) */

  MJobReleaseRsv(J,TRUE,TRUE);

  if ((Message != NULL) && (Message[0] != '\0'))
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"job suspended - %s",
      Message);

    MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
    }

  MOWriteEvent((void *)J,mxoJob,mrelJobPreempt,Message,MStat.eventfp,NULL);
 
  return(SUCCESS);
  }  /* END MRMJobSuspend() */





/**
 * Issue job checkpoint request to resource manager.
 *
 * @param J (I)
 * @param TerminateJob (I)
 * @param Message (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobCheckpoint(
 
  mjob_t     *J,
  mbool_t     TerminateJob,
  const char *Message,
  char       *EMsg,
  int        *SC)
 
  {
  int rqindex;
  int rmindex;
  int rmcount;
 
  int rc;
 
  mrm_t *R;
 
  int RMList[MMAX_RM];

  const char *FName = "MRMJobCheckpoint";
 
  MDB(1,fRM) MLog("%s(%s,%s,Message,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MBool[TerminateJob]);

  if (EMsg != NULL)
    EMsg[0] = '\0';
 
  if (J == NULL)
    {
    return(FAILURE);
    }
 
  memset(RMList,0,sizeof(RMList));
 
  rmcount = 0;
 
  /* determine resource managers used */
 
  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      continue;
 
    for (rmindex = 0;rmindex < rmcount;rmindex++)
      {
      if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
        {
        break;
        }
      }
 
    if (rmindex == rmcount)
      {
      RMList[rmindex] = J->Req[rqindex]->RMIndex;
 
      rmcount++;
      }
    }  /* END for (rqindex) */
 
  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];
 
    if (MRMFunc[R->Type].JobCheckpoint == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot checkpoint job %s (RM '%s' does not support function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobCheckpoint]);
 
      return(FAILURE);
      }
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobCheckpoint,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      5,
      J,
      R,
      TerminateJob,
      EMsg,
      SC);
 
    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot checkpoint job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobCheckpoint]);
 
      return(FAILURE);
      }
    }    /* END for (rmindex) */

  if (TerminateJob == TRUE)
    bmset(&J->IFlags,mjifWasRequeued);

  MOReportEvent(J,J->Name,mxoJob,mttCheckpoint,MSched.Time,TRUE);

  MOWriteEvent((void *)J,mxoJob,mrelJobCheckpoint,Message,MStat.eventfp,NULL);
 
  if (MSched.LimitedJobCP == FALSE)
    MJobTransition(J,TRUE,FALSE);

  return(SUCCESS);
  }  /* END MRMJobCheckpoint() */





/**
 * Issue job resume request to resource manager.
 *
 * @param J (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMJobResume(
 
  mjob_t *J,     /* I */
  char   *EMsg,  /* O (optional,minsize=MMAX_LINE) */
  int    *SC)    /* O (optional) */
 
  {
  int rqindex;
  int rmindex;
  int rmcount;

  int nindex;
 
  int rc;
 
  mrm_t   *R;
  mreq_t  *RQ;
  mnode_t *N;
 
  int RMList[MMAX_RM];

  long  RMTime;

  char  tmpLine[MMAX_LINE];

  char *MPtr;

  int   tmpSC;

  const char *FName = "MRMJobResume";
 
  MDB(1,fRM) MLog("%s(%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  MSchedSetActivity(macJobResume,J->Name);

  if (EMsg != NULL)
    {
    MPtr = EMsg;
    }
  else
    {
    MPtr = tmpLine;
    }

  MPtr[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmJobResume,mxoJob,(void *)J,NULL);

      sprintf(MPtr,"cannot resume job - monitor mode");

      MDB(3,fRM) MLog("INFO:     cannot resume job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;

      return(FAILURE);

      /*NOTREACHED*/

      break;

    case msmInteractive:

      {
      int nindex;
      int tindex;

      char Rsp[3];

      mbool_t Accept;

      mnode_t *N;

      char tmpBuf[MMAX_BUFFER];

      tmpBuf[0] = '\0';

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        for (tindex = 0;tindex < MNLGetTCAtIndex(&J->NodeList,nindex);tindex++)
          {
          if (tmpBuf[0] != '\0')
            MUStrCat(tmpBuf,",",sizeof(tmpBuf));

          MUStrCat(tmpBuf,N->Name,sizeof(tmpBuf));
          }  /* END for (tindex) */
        }    /* END for (nindex) */

      /* create message */

      fprintf(stdout,"Command:  resume job %s on node list %s\n",
        J->Name,
        tmpBuf);

      /* while (fgetc(stdin) != EOF); */

      fprintf(stdout,"Accept:  (y/n) [default: n]?");

      if ((fgets(Rsp,3,stdin) != NULL) &&
         ((Rsp[0] == 'y') || (Rsp[0] == 'Y')))
        {
        Accept = TRUE;
        }
      else
        {
        Accept = FALSE;
        }

      fprintf(stdout,"\nRequest %s\n",
        (Accept == TRUE) ? "Accepted" : "Rejected");

      if (Accept == FALSE)
        {
        MDB(3,fRM) MLog("INFO:     cannot resume job %s (command rejected by user)\n",
          J->Name);

        if (SC != NULL)
          *SC = mscNoError;

        return(FAILURE);
        }
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */
 
  memset(RMList,0,sizeof(RMList));
 
  rmcount = 0;
 
  /* determine resource managers used */
 
  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      continue;
 
    for (rmindex = 0;rmindex < rmcount;rmindex++)
      {
      if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
        {
        break;
        }
      }
 
    if (rmindex == rmcount)
      {
      RMList[rmindex] = J->Req[rqindex]->RMIndex;
 
      rmcount++;
      }
    }  /* END for (rqindex) */

  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    rmcount = 0;

    rc = SUCCESS;
    }
 
  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    /* NOTE:  ignore R->FailIteration if async start is enabled */

    if ((R->FailIteration == MSched.Iteration) &&
         !bmisset(&R->Flags,mrmfASyncStart) &&
        (R->FailCount >= R->MaxFailCount))
      {
      sprintf(MPtr,"cannot resume job - fail iteration");

      MDB(2,fRM) MLog("INFO:     cannot resume job %s (%s)\n",
        J->Name,
        MPtr);

      if (SC != NULL)
        *SC = mscNoError;

      return(FAILURE);
      }
 
    if (MRMFunc[R->Type].JobResume == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot resume job %s (RM '%s' does not support function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobResume]);
 
      return(FAILURE);
      }

    MRMStartFunc(R,NULL,mrmJobResume);
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].JobResume,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      4,
      J,
      R,
      MPtr,
      &tmpSC);

    if (SC != NULL)
      *SC = tmpSC;

    MRMEndFunc(R,NULL,mrmJobResume,&RMTime);

    /* remove time from sched load and add to rm action load */

    MSched.LoadEndTime[mschpRMAction] += RMTime;
    MSched.LoadStartTime[mschpSched]  += RMTime;
 
    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot resume job %s (RM '%s' failed in function '%s')\n",
        J->Name,
        R->Name,
        MRMFuncType[mrmJobResume]);

      if (MPtr[0] != '\0')
        MRMSetFailure(R,mrmJobResume,(enum MStatusCodeEnum)tmpSC,MPtr);
      else if (rc == mscTimeout)
        MRMSetFailure(R,mrmJobResume,(enum MStatusCodeEnum)tmpSC,"timeout");
      else
        MRMSetFailure(R,mrmJobResume,(enum MStatusCodeEnum)tmpSC,"cannot resume job");

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  if (MRsvJCreate(J,NULL,MSched.Time,mjrActiveJob,NULL,FALSE) == FAILURE)
    {
    MDB(0,fSCHED) MLog("ERROR:    cannot create reservation for resumed job '%s'\n",
      J->Name);
    }

  /* adjust per-node resource consumption */

  /* WARNING:  does not properly support multi-req jobs (FIXME) */

  RQ = J->Req[0];
 
  for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    int TC = MNLGetTCAtIndex(&J->NodeList,nindex);
    /* adjust resource usage */
 
    if (RQ->SDRes != NULL)
      MCResRemove(&N->ARes,&N->CRes,RQ->SDRes,TC,TRUE);
    else
      MCResRemove(&N->ARes,&N->CRes,&RQ->DRes,TC,TRUE);

    if (RQ->SDRes != NULL)
      MCResAdd(&N->DRes,&N->CRes,RQ->SDRes,TC,TRUE);
    else
      MCResAdd(&N->DRes,&N->CRes,&RQ->DRes,TC,TRUE);
    }  /* END for (nindex) */

  MOWriteEvent((void *)J,mxoJob,mrelJobResume,NULL,MStat.eventfp,NULL);

  MDB(2,fSIM) MLog("INFO:     job %s successfully resumed on %d procs\n",
    J->Name,
    J->Request.TC);
 
  return(SUCCESS);
  }  /* END MRMJobResume() */


