/* HEADER */ 
/**
 * @file MLimit.c
 *
 * Moab Limits
 */
        
/* Contains:                                 *
 *  int MLimitEnforceAll(PLimit)             *
 *                                           */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  



/**
 * cancel a job that has violated it's limits
 *
 * @param J         (I) modified, canceled
 * @param CancelMsg
 * @param EMsg      (O) optional, minsize=MMAX_LINE
 */

int MLimitCancelJob(

  mjob_t     *J,
  const char *CancelMsg,
  char       *EMsg)

  {
  if (bmisset(&J->Flags,mjfDestroyTemplateSubmitted))
    return(SUCCESS);

  J->CompletionCode = MSched.WCViolCCode;

  if (bmisset(&J->IFlags,mjifGenericSysJob))
    {
    /* system job being cancelled */

    char tmpMsg[MMAX_LINE];
    mjob_t *JG;

    snprintf(tmpMsg,sizeof(tmpMsg),"system job exceeded %ld second timeout",
      J->WCLimit);

    MMBAdd(&J->MessageBuffer,tmpMsg,NULL,mmbtNONE,0,0,NULL);

    if ((J->JGroup != NULL) &&
        (MJobFind(J->JGroup->Name,&JG,mjsmExtended) == SUCCESS))
      {
      snprintf(tmpMsg,sizeof(tmpMsg),"system job dependency '%s' exceeded %ld second timeout",
        J->Name,
        J->WCLimit);

      MMBAdd(&JG->MessageBuffer,tmpMsg,NULL,mmbtNONE,0,0,NULL);
      }

    /* if sysjob cancelled, due to WCLimit or other limit, consider job to have failed
       explicitly set J->CompletionCode since there is no guarantee that T->ExitCode will be set
       appropriately */

    if (J->CompletionCode == 0)
      {
      J->CompletionCode = 1;

      MDB(4,fRM) MLog("INFO:     system job %s cancelled with no completion code specified - setting ccode=1\n",
        J->Name);
      }
    }

  if (MJobCancel(
       J,
       CancelMsg,
       FALSE,
       EMsg,
       NULL) == FAILURE)
    {
    /* extend job rsv duration by JobWCXH */

    if (J->Rsv != NULL)
      {
      J->Rsv->EndTime = MSched.Time + MSched.MaxRMPollInterval;

      MRsvAdjustTimeframe(J->Rsv);
      }
    }

  return (SUCCESS);
  } /* END MoabFunction() */




/**
 * Enforce all job usage limits and trigger violation actions.
 *
 * @see MSysCheck() - parent
 *
 * @param P (I)
 */

int MLimitEnforceAll(

  mpar_t *P) /* I */

  {
  long    JobWCXH;  /* hard overrun */
  long    JobWCXS;  /* soft overrun */

  job_iter JTI;

  mjob_t *J;
  mreq_t *RQ;

  mbool_t ResourceLimitsExceeded;  
  mbool_t JobExceedsLimits;
  mbool_t IsMinLimit;              /* limit is min, not max */

  enum MResLimitTypeEnum VRes;

  int     VLimit = -1;
  int     VVal   = -1;

  int     rc = SUCCESS;

  int     pindex;
  int     lindex;

  char    EMsg[MMAX_LINE];
  char    DString[MMAX_LINE];
  char    LimitString[MMAX_LINE];

  mnode_t *N;
  
  int     nindex;
  int     aindex;

  const char *FName = "MLimitEnforceAll";

  MDB(4,fSCHED) MLog("%s(%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL");

#ifndef __MOPT
  if (P == NULL)
    {
    return(FAILURE);
    }
#endif /* __MOPT */

  MJobIterInit(&JTI);

  mstring_t Msg(MMAX_LINE);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if ((J->State != mjsStarting) &&
        (J->State != mjsRunning))
      {
      continue;
      }

#ifdef MYAHOO
    /* only remove jobs if they are not using the default wallclock time */

    if (J->WCLimit >= MDEF_SYSDEFJOBWCLIMIT)
      continue;
#endif /* MYAHOO */

    /* Only check peer job limits started by this scheduler */

    if (MSched.DontEnforcePeerJobLimits == TRUE)
      {  
      if ((bmisset(&J->Flags,mjfNoRMStart) && bmisset(&J->Flags,mjfClusterLock)) ||
          (MJOBWASMIGRATEDTOREMOTEMOAB(J)))
        continue;
      }

    /* enforce wallclock limits */

    if (bmisset(&J->Flags,mjfNoRMStart))
      {
      JobWCXH = 0;

      JobWCXS = 0;
      }
    else if (MSched.JobPercentOverrun != 0.0)
      {
      JobWCXH = (long) (J->WCLimit * MSched.JobPercentOverrun);

      JobWCXS = 1;
      }
    else
      {
      JobWCXH = MSched.JobHMaxOverrun;
      JobWCXS = MSched.JobSMaxOverrun;

      if (JobWCXS == JobWCXH)
        JobWCXS = -1;
      }
 
    /* set overrun */

    if ((J->Credential.U != NULL) && (J->Credential.U->F.Overrun > 0))
      JobWCXH = J->Credential.U->F.Overrun;
    else if ((MSched.DefaultU != NULL) && (MSched.DefaultU->F.Overrun > 0))
      JobWCXH = MSched.DefaultU->F.Overrun;
    else if ((J->Credential.C != NULL) && (J->Credential.C->F.Overrun > 0))
      JobWCXH = J->Credential.C->F.Overrun;
    else if ((MSched.DefaultC != NULL) && (MSched.DefaultC->F.Overrun > 0))
      JobWCXH = MSched.DefaultC->F.Overrun;
 
    if ((JobWCXH >= 0) &&
        (MSched.Time > J->StartTime) &&
       ((unsigned long)(MSched.Time - J->StartTime) > (J->WCLimit + J->SWallTime + JobWCXH)))
      {
      char TString[MMAX_LINE];
      char NCTime[MMAX_LINE];

      MULToTString((MSched.Time - J->StartTime) - (J->WCLimit + J->SWallTime),TString);

      MDB(2,fCORE) MLog("ALERT:    job '%s' in state '%s' has exceeded its wallclock limit (%ld+S:%ld) by %s (job will be cancelled)\n",
        J->Name,
        MJobState[J->State],
        J->WCLimit,
        J->SWallTime,
        TString);
 
      MULToTString(MSched.Time - J->StartTime - J->WCLimit,TString);
      MUSNCTime(&J->StartTime,NCTime);

      MStringSetF(&Msg,"JOBWCVIOLATION:  job '%s' in state '%s' has exceeded its wallclock limit (%ld) by %s (job will be cancelled)  job start time: %s",
        J->Name,
        MJobState[J->State],
        J->WCLimit,
        TString,
        NCTime);

      MStringAppend(&Msg,"\n\n========== Output of Checkjob ==========\n\n");

      MUICheckJob(J,&Msg,mptHard,NULL,NULL,NULL,NULL,0);
 
      if (JobWCXS > 0)
        {
        /* if we sent out an email for soft violation we should follow it up with an email for cancel */

        MSysRegEvent(Msg.c_str(),mactMail,1);
        }

      MSysRegEvent(Msg.c_str(),mactNONE,1);

      if (MSched.WCViolAction == mwcvaPreempt)
        {
        if (MJobPreempt(
              J,
              NULL,
              NULL,
              NULL,
              "wallclock limit exceeded",
              mppNONE,
              TRUE,
              NULL,
              NULL,
              NULL) == FAILURE)
          {
          MDB(2,fCORE) MLog("ALERT:    cannot preempt job '%s' (job exceeded wallclock limit)\n",
            J->Name);
          }
        }    /* END if (MSched.WCViolAction == mwcvaPreempt) */
      else if ((J->System != NULL))
        {
        switch (J->System->CompletionPolicy)
          {
          case mjcpOSProvision:
          case mjcpOSProvision2:
          case mjcpPowerOff:
          case mjcpPowerOn:
          case mjcpReset:
          case mjcpVMCreate:
          case mjcpVMMap:
          case mjcpVMMigrate:

          /* ALERT:  We can not allow VMTracking jobs to expire and destroy the associated VM, 
                     which may cause data loss and create severe liability issues with customers */

          case mjcpVMTracking:

            /* 8-31-09 BOC RT4812 - Let the rm report that the job is completed. 
             * MJobSetState(J,mjsCompleted); */

            /* do not cancel these system jobs based on limits */

            break;

          default:

            MLimitCancelJob(J,"MOAB_INFO:  job exceeded wallclock limit",EMsg);

            break;
          } /* END switch */
        }
      else
        {
        MLimitCancelJob(J,"MOAB_INFO:  job exceeded wallclock limit",EMsg);
        }
      }    /* END if ((JobWCXH >= 0) && ...) */

    if ((JobWCXS > 0) &&
        (MSched.Time > J->StartTime) &&
        ((unsigned long)(MSched.Time - J->StartTime) > (J->WCLimit + J->SWallTime + JobWCXS)) &&
        (J->SoftOverrunNotified == FALSE))
      {
      char EMsg[MMAX_LINE];
      char TString[MMAX_LINE];
      int  SC;

      MULToTString(MSched.Time - J->StartTime - J->WCLimit,TString);
      MULToDString(&J->StartTime,DString);

      MStringSetF(&Msg,"JOBWCVIOLATION:  job '%s' in state '%s' has exceeded its wallclock limit (%ld) by %s (soft limit notify) job start time: %s",
        J->Name,
        MJobState[J->State],
        J->WCLimit,
        TString,
        DString);

      MStringAppend(&Msg,"\n\n========== output of checkjob ==========\n\n");

      MUICheckJob(J,&Msg,mptHard,NULL,NULL,NULL,NULL,0);
 
      MSysRegEvent(Msg.c_str(),mactMail,1);

      /* TODO:  send a "soft-signal" to the job */

      if (J->TermSig != NULL)
        MRMJobSignal(J,-1,J->TermSig,EMsg,&SC);
      else if (J->Credential.C->TermSig != NULL)
        MRMJobSignal(J,-1,J->Credential.C->TermSig,EMsg,&SC);
      else
        MRMJobSignal(J,-1,J->DestinationRM->SoftTermSig,EMsg,&SC);

      J->SoftOverrunNotified = TRUE;
      }

    /* enforce CRes utilization limits */

    RQ = J->Req[0];  /* FIXME */

    if (RQ == NULL)
      continue;

    JobExceedsLimits = FALSE;

    for (pindex = MRESUTLSOFT;pindex <= MRESUTLHARD;pindex++)
      {
      ResourceLimitsExceeded = FALSE;

      VRes = mrlNONE;

      for (lindex = 0;lindex < mrlLAST;lindex++)
        {
        IsMinLimit = FALSE;

        switch (lindex)
          {
          case mrlDisk:

            VLimit = RQ->DRes.Disk;
            VVal   = (RQ->URes != NULL) ? RQ->URes->Disk : 0;

            break;

          case mrlJobMem:

            if (J->MaxJobMem > 0)
              {
              VLimit = J->MaxJobMem;
              VVal   = (RQ->URes != NULL) ? RQ->URes->Mem * J->Request.TC : 0;
              }
            else
              {
              VLimit = 1;
              VVal   = 0;
              }

            break;

          case mrlJobProc:

            if (J->MaxJobProc > 0)
              {
              VLimit = 100 * J->MaxJobProc;
              VVal   = (RQ->URes != NULL) ? RQ->URes->Procs * J->Request.TC : 0; 
              }
            else
              {
              VLimit = 1;
              VVal   = 0;
              }

            break;

          case mrlMem:

            VLimit = RQ->DRes.Mem;
            VVal   = (RQ->URes != NULL) ? RQ->URes->Mem : 0;

            break;

          case mrlProc:

            /* cpu time */

            VLimit = 100 * RQ->DRes.Procs;
            VVal   = (RQ->URes != NULL) ? RQ->URes->Procs : 0;

            break;

          case mrlSwap:

            VLimit = RQ->DRes.Swap;
            VVal   = (RQ->URes != NULL) ? RQ->URes->Swap : 0;

            break;

          case mrlWallTime:

            VLimit = (int)J->WCLimit;
            VVal   = (int)J->AWallTime;

            break;

          case mrlCPUTime:

            VLimit = J->CPULimit;
            VVal   = 0;
            
            {
            int  rqindex;

            for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
              {
              if (J->Req[rqindex] == NULL)
                break;

              VVal += (J->Req[rqindex]->LURes != NULL) ? J->Req[rqindex]->LURes->Procs / 100 : 0;
              }
            }

            break;

          case mrlMinJobProc:

            /* trigger exception if job efficiency < 5% */

            /* 5% of potential cpu usage */

            VLimit = (int) (.05 * J->AWallTime * J->TotalProcCount);
         
            VVal   = (RQ->URes != NULL) ? RQ->URes->Procs : 0;

            IsMinLimit = TRUE;

            break;

          case mrlTemp:

            /* NYI: need to look this up off the gmetric structure */

            VLimit = 1;

            if ((MSched.TempGMetricIndex > 0) &&
                (RQ->XURes != NULL) && 
                (RQ->XURes->GMetric[MSched.TempGMetricIndex] != NULL))
              VVal = (int) (RQ->XURes->GMetric[MSched.TempGMetricIndex]->IntervalLoad);
            else
              VVal = 0;

            break;

          default:

            VLimit = 1;
            VVal   = 0;

            break;
          }  /* END switch (lindex) */
 
        if (P->ResourceLimitMultiplier[lindex] != 0.0)
          { 
          if ((pindex == MRESUTLHARD) && (P->ResourceLimitMultiplier[lindex] > 1.0))
            VLimit = (int)(VLimit * P->ResourceLimitMultiplier[lindex]);
          else if ((pindex == MRESUTLSOFT) && (P->ResourceLimitMultiplier[lindex] < 1.0))
            VLimit = (int)(VLimit * P->ResourceLimitMultiplier[lindex]);
          }

        MDB(7,fSCHED) MLog("INFO:     Checking job %s for limit violations. %s %s limit (%d %c %d)\n",
          J->Name,
          MResLimitType[lindex],
          (pindex == MRESUTLSOFT) ? "soft" : "hard",
          VVal,
          (IsMinLimit == TRUE) ? '<' : '>',
          VLimit);

        MDB(7,fSCHED) MLog("INFO:     Checking job %s for limit violations. %s %s limit (%d %c %d)\n",
          J->Name,
          MResLimitType[lindex],
          (pindex == MRESUTLSOFT) ? "soft" : "hard",
          VVal,
          (IsMinLimit == TRUE) ? '<' : '>',
          VLimit);

        if ((P->ResourceLimitPolicy[pindex][lindex] != mrlpNONE) &&
            (((IsMinLimit == FALSE) && (VVal > VLimit) && (VLimit > 0)) ||
             ((IsMinLimit == TRUE) && (VVal < VLimit))))
          {
          MDB(3,fSCHED) MLog("INFO:     job %s violates requested %s %s limit (%d %c %d)\n",
            J->Name,
            MResLimitType[lindex],
            (pindex == MRESUTLSOFT) ? "soft" : "hard",
            VVal,
            (IsMinLimit == TRUE) ? '<' : '>',
            VLimit);

          VRes = (enum MResLimitTypeEnum)lindex;

          ResourceLimitsExceeded = TRUE;
          JobExceedsLimits       = TRUE;

          break;
          }  /* END if ((P->ResourceLimitPolicy[pindex][lindex] != mrlpNONE) && ...) */
        }    /* END for (lindex) */

      if ((ResourceLimitsExceeded == FALSE) || 
         ((pindex == MRESUTLSOFT) && bmisset(&J->IFlags,mjifRULSPActionTaken)) ||
         ((pindex == MRESUTLHARD) && bmisset(&J->IFlags,mjifRULHPActionTaken)))
        {
        /* limit not violated or already handled */

        continue;
        }
  
      /* job is using more resources than requested */
 
      /* round off interval specified in 100th's of second */
 
      J->RULVTime += (mulong)((MSched.Interval + 50) / 100);
    
      switch (P->ResourceLimitPolicy[pindex][VRes])
        {
        case mrlpAlways:
  
          /* check limited resources */
  
          break;
  
        case mrlpExtendedViolation:
  
          /* determine length of violation */
  
          if (J->RULVTime < (mulong)P->ResourceLimitMaxViolationTime[pindex][VRes])
            {
            /* ignore violation */
  
            ResourceLimitsExceeded = FALSE;
            }
           
          break;
  
        case mrlpBlockedWorkloadOnly:

          ResourceLimitsExceeded = FALSE;

          /* walk all tasks allocated to job */

          if (MNLIsEmpty(&J->NodeList))
            {
            MDB(3,fSCHED) MLog("INFO:     cannot evaluate nodelist for job %s\n",
              J->Name);

            break;
            }

          for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
            {
            if (VRes == mrlWallTime)
              { 
              /* determine if eligible job is blocked, ie does job reservation */
              /* exist at Now + MSched.Iteration for node utilized by job? */

              if (MNodeRangeIsExclusive(N,J,MSched.Time,MSched.MaxRMPollInterval) == TRUE)
                {
                ResourceLimitsExceeded = TRUE;
                JobExceedsLimits       = TRUE;

                break;
                }
              }
            else if (VRes == mrlJobProc)
              {
              if (N->Load > N->CRes.Procs)
                {
                ResourceLimitsExceeded = TRUE;
                JobExceedsLimits       = TRUE;
                }
              }
            else
              {
              /* walk all tasks allocated to job */

              /* determine if resource usage impacts resources dedicated to other jobs */

              /* ie, if (N->CRes[VRes].X < J->Req[0]->URes[VRes].X.N - SUM(N->J->Req[0]->DRes[VRes].X.N) */

              /* NYI */
              }
            }    /* END for (nindex) */

          break;
  
        default:
  
          MDB(1,fSCHED) MLog("ALERT:    unexpected limit violation policy[%d][%d] %d\n",
            pindex,
            VRes,
            P->ResourceLimitPolicy[pindex][VRes]);
  
          ResourceLimitsExceeded = FALSE;
  
          break;
        }  /* END switch (P->ResourceUtilizationPolicy[pindex]) */
  
      if (ResourceLimitsExceeded == FALSE)
        {
        continue;
        }

      MULToDString(&J->StartTime,DString);

      /* job violates resource utilization policy */
  
      char emptyString[] = {0};

      char *str =  bmtostring(&P->ResourceLimitViolationActionBM[pindex][VRes],MPolicyAction,LimitString,"+");

      if (NULL == str)
        str = emptyString;

      MStringSetF(&Msg,"JOBRESVIOLATION:  job '%s' in state %s has exceeded %s resource %s limit (%d > %d) (action %s will be taken)  job start time: %s",
        J->Name,
        MJobState[J->State],
        MResLimitType[VRes],
        (pindex == MRESUTLSOFT) ? "soft" : "hard", 
        VVal,
        VLimit,
        str,
        DString);
  
      MSysRegEvent(Msg.c_str(),mactNONE,1);
 
      EMsg[0] = '\0';

      for (aindex = 1;aindex < mrlaLAST;aindex++)
        {
        if (!bmisset(&P->ResourceLimitViolationActionBM[pindex][VRes],aindex))
          continue;

        switch (aindex)
          {
          case mrlaCancel:

            MStringSetF(&Msg,"job %s exceeded %s usage %s limit (%d %c %d)",
              J->Name,
              MResLimitType[VRes],
              (pindex == MRESUTLSOFT) ? "soft" : "hard",
              VVal,
              (IsMinLimit == TRUE) ? '<' : '>',
              VLimit);

            if (MLimitCancelJob(J,Msg.c_str(),EMsg) == SUCCESS)
              {
              if (pindex == MRESUTLSOFT)
                bmset(&J->IFlags,mjifRULSPActionTaken);
              else
                bmset(&J->IFlags,mjifRULHPActionTaken);

              J->CompletionCode = MSched.WCViolCCode;
              }

            break;

          case mrlaCheckpoint:

            rc = MRMJobCheckpoint(
                   J,
                   TRUE,
                   "job violates resource utilization policies",
                   EMsg,
                   NULL);

            if (rc == SUCCESS)
              {
              if (pindex == MRESUTLSOFT)
                bmset(&J->IFlags,mjifRULSPActionTaken);
              else
                bmset(&J->IFlags,mjifRULHPActionTaken);
              }

            break;

          case mrlaNotify:

            {
            char userEmailAddr[MMAX_LINE];
            char ToList[MMAX_LINE];

            /* Code to check for default mail domain */

            if ((J->NotifyAddress != NULL) && (J->NotifyAddress[0] != '\0'))
              {
              strcpy(userEmailAddr,J->NotifyAddress);
              }
            else if ((J->Credential.U->EMailAddress != NULL) && (J->Credential.U->EMailAddress[0] != '\0'))
              {
              strcpy(userEmailAddr,J->Credential.U->EMailAddress);
              }
            else
              {
              sprintf(userEmailAddr,"%s@%s",J->Credential.U->Name,
                (MSched.DefaultMailDomain != NULL) ? MSched.DefaultMailDomain : "localhost");
              }

            sprintf(ToList,"%s,%s",MSysGetAdminMailList(1),userEmailAddr);

            MStringSetF(&Msg,"job %s exceeded %s usage %s limit",
              J->Name,
              MResLimitType[VRes],
              (pindex == MRESUTLSOFT) ? "soft" : "hard");

            if (J->Credential.U->NoEmail != TRUE)
              {
              rc = MSysSendMail(
                    ToList,
                    NULL,
                    "moab job resource violation",
                    NULL,
                    Msg.c_str());
              }
            else
              {
              rc = SUCCESS;
              }

           if (rc == SUCCESS)
              {
              if (pindex == MRESUTLSOFT)
                bmset(&J->IFlags,mjifRULSPActionTaken);
              else
                bmset(&J->IFlags,mjifRULHPActionTaken);
              }
            }  /* END BLOCK (case mrlaNotify) */

            break;

          case mrlaPreempt:

            rc = MJobPreempt(
                   J,
                   NULL,
                   NULL,
                   NULL,
                   "job violated resource utilization limits",
                   mppNONE,
                   TRUE,
                   NULL,
                   EMsg,
                   NULL);

           if (rc == SUCCESS)
              {
              if (pindex == MRESUTLSOFT)
                bmset(&J->IFlags,mjifRULSPActionTaken);
              else
                bmset(&J->IFlags,mjifRULHPActionTaken);
              }

            break;

          case mrlaRequeue:
  
            rc = MJobRequeue(J,NULL,"job violated resource utilization limits",EMsg,NULL);

#if 0
            /* do not set action taken flags, we want this to occur
             * as many times as it is reached - kc rt5599 */

            if (rc == SUCCESS)
              {
              if (pindex == MRESUTLSOFT)
                bmset(&J->IFlags,mjifRULSPActionTaken);
              else
                bmset(&J->IFlags,mjifRULHPActionTaken);
              }
#endif
  
            break;
  
          case mrlaSuspend:
  
            if ((rc = MRMJobSuspend(J,"job violated resource utilization limits",EMsg,NULL)) == SUCCESS)
              {
              J->SMinTime = MSched.Time + P->AdminMinSTime;
              }

            if (rc == SUCCESS)
              {
              if (pindex == MRESUTLSOFT)
                bmset(&J->IFlags,mjifRULSPActionTaken);
              else
                bmset(&J->IFlags,mjifRULHPActionTaken);
              }
  
            break;

          case mrlaUser:

            if (bmisset(&J->IFlags,mjifWCRequeue))
              {
              rc = MJobRequeue(J,NULL,"job violated resource utilization limits",EMsg,NULL);
              }
            else
              {
              MStringSetF(&Msg,"job %s exceeded %s usage %s limit (%d %c %d)",
                J->Name,
                MResLimitType[VRes],
                (pindex == MRESUTLSOFT) ? "soft" : "hard",
                VVal,
                (IsMinLimit == TRUE) ? '<' : '>',
                VLimit);

              rc = MJobCancel(J,Msg.c_str(),FALSE,EMsg,NULL);

              if (rc == SUCCESS)
                {
                if (pindex == MRESUTLSOFT)
                  bmset(&J->IFlags,mjifRULSPActionTaken);
                else
                  bmset(&J->IFlags,mjifRULHPActionTaken);
                }
              }

            break;

          default:
  
            rc = FAILURE;
  
            MDB(1,fSCHED) MLog("ALERT:    unexpected limit violation action %u\n",
              aindex);
  
            break;
          }  /* END switch (aindex) */


        MDB(1,fSCHED) 
          {
          char emptyString[] = {0};
          char *str = bmtostring(&P->ResourceLimitViolationActionBM[pindex][VRes],MPolicyAction,LimitString,"+");

          if (NULL == str)
            str = emptyString;

          MLog("ALERT:    %s limit violation action %s %s%s%s\n",
          (pindex == MRESUTLSOFT) ? "soft" : "hard",     
          str,
          (rc == SUCCESS) ? "succeeded" : "failed",
          (EMsg[0] != '\0') ? " - " : "",
          (EMsg[0] != '\0') ? EMsg : "");
          }
        }  /* END for (aindex) */
      }    /* END for (pindex) */

    if (JobExceedsLimits == FALSE)
      {
      /* clear job violation time */

      J->RULVTime = 0;
      }
    }    /* END for (jindex) */

  return(SUCCESS);
  }  /* END MLimitEnforceAll() */ 



int MLimitGetMPU(

  mcredl_t           *L,
  void               *O,
  enum MXMLOTypeEnum  OType,
  enum MLimitTypeEnum LimitType,
  mpu_t             **P)

  {
  char *OName = NULL;
  
  mhash_t **HTP = NULL;
  mhash_t  *HT  = NULL;

  mpu_t    *PP = NULL;

  switch (LimitType)
    {
    case mlActive:

      switch (OType)
        {
        case mxoUser:

          HTP = &L->UserActivePolicy;

          OName  = ((mgcred_t *)O)->Name;

          break;

        case mxoAcct:

          HTP = &L->AcctActivePolicy;

          OName  = ((mgcred_t *)O)->Name;

          break;

        case mxoGroup:

          HTP = &L->GroupActivePolicy;

          OName  = ((mgcred_t *)O)->Name;

          break;

        case mxoClass:

          HTP = &L->ClassActivePolicy;

          OName  = ((mclass_t *)O)->Name;

          break;

        case mxoQOS:

          HTP = &L->QOSActivePolicy;

          OName  = ((mqos_t *)O)->Name;

          break;

        default:

          break;
        }  /* END switch (OType) */

      break;

    case mlIdle:

      switch (OType)
        {
        case mxoUser:

          HTP = &L->UserIdlePolicy;

          OName  = ((mgcred_t *)O)->Name;

          break;

        case mxoAcct:

          HTP = &L->AcctIdlePolicy;

          OName  = ((mgcred_t *)O)->Name;

          break;

        case mxoGroup:

          HTP = &L->GroupIdlePolicy;

          OName  = ((mgcred_t *)O)->Name;

          break;

        case mxoClass:

          HTP = &L->ClassIdlePolicy;

          OName  = ((mclass_t *)O)->Name;

          break;

        case mxoQOS:

          HTP = &L->QOSIdlePolicy;

          OName  = ((mqos_t *)O)->Name;

          break;

        default:

          break;
        }  /* END switch (OType) */

      break;

    default:

      break;
    }  /* END switch (LimitType) */

  if (*HTP == NULL)
    {
    HT = (mhash_t *)MUCalloc(1,sizeof(mhash_t));

    MUHTCreate(HT,-1);

    *HTP = HT;
    }
  else
    {
    HT = *HTP;
    }

  if (MUHTGet(HT,OName,(void **)P,NULL) == SUCCESS)
    {
    return(SUCCESS);
    }

  PP = (mpu_t *)MUCalloc(1,sizeof(mpu_t));

  MPUInitialize(PP);

  MUHTAdd(HT,OName,(void *)PP,NULL,MUFREE);

  PP->Ptr = O;

  *P = PP;

  return(SUCCESS);
  }  /* END MLimitGetMPU() */




/**
 * Verify all needed limit/policy structures are instantiated.
 *
 * When a limit is read in on the default object only the memory for the 
 * default object is initialized: MClass[batch].L->APU[DEFAULT]
 * In order to apply limits, as jobs are read in we must populate the
 * structures with the default limits: MClass[batch].L->APU[user]
 * then the rest of moab's routines will automatically work and
 * we will only allocate memory for valid credential combinations that 
 * are used
 *
 * @param J
 */

int MJobVerifyPolicyStructures(

  mjob_t *J)

  {
  mgcred_t *U = NULL;
  mgcred_t *G = NULL;
  mgcred_t *A = NULL;

  mclass_t *C = NULL;

  mqos_t   *Q = NULL;

  mpu_t    *P = NULL;

  mpu_t    *DefaultP = NULL;

  /* verify that policy structures exist and that they inherit default policies */

  /**
   * All the following must eventually exist:

   USER:
   J->Cred.U->L->APG
   J->Cred.U->L->APC
   J->Cred.U->L->APQ
   J->Cred.U->L->APA

   J->Cred.U->L->IPG
   J->Cred.U->L->IPC
   J->Cred.U->L->IPQ
   J->Cred.U->L->IPA

   CLASS:
   J->Cred.C->L->APG
   J->Cred.C->L->APU
   J->Cred.C->L->APQ
   J->Cred.C->L->APA

   J->Cred.C->L->IPG
   J->Cred.C->L->IPU
   J->Cred.C->L->IPQ
   J->Cred.C->L->IPA

   GROUP:
   J->Cred.G->L->APC
   J->Cred.G->L->APU
   J->Cred.G->L->APQ
   J->Cred.G->L->APA

   J->Cred.G->L->IPC
   J->Cred.G->L->IPU
   J->Cred.G->L->IPQ
   J->Cred.G->L->IPA

   ACCOUNT:
   J->Cred.A->L->APC
   J->Cred.A->L->APU
   J->Cred.A->L->APQ
   J->Cred.A->L->APG

   J->Cred.A->L->IPC
   J->Cred.A->L->IPU
   J->Cred.A->L->IPQ
   J->Cred.A->L->IPG

   QOS:
   J->Cred.Q->L->APC
   J->Cred.Q->L->APU
   J->Cred.Q->L->APG
   J->Cred.Q->L->APA

   J->Cred.Q->L->IPC
   J->Cred.Q->L->IPU
   J->Cred.Q->L->IPG
   J->Cred.Q->L->IPA

   *
   * For now only user is being tried. 
   */

  U = J->Credential.U;
  G = J->Credential.G;
  A = J->Credential.A;
  C = J->Credential.C;
  Q = J->Credential.Q;

  /**
   * check X to user mappings (*CFG[] MAX*[USER])
   * 
   * handle: J->Cred.G->L.APU GROUPCFG[]   MAX*[USER]
   *         J->Cred.C->L.APU CLASSCFG[]   MAX*[USER]
   *         J->Cred.A->L.APU ACCOUNTCFG[] MAX*[USER]
   *         J->Cred.Q->L.APU QOSCFG[]     MAX*[USER] 
   *
   */
  
  if (U != NULL)
    {
    /*****
     * CHECK ACTIVE POLICIES
     */

    if (MSched.DefaultU->L.ActivePolicy.GLimit != NULL)
      {
      /* check USERCFG[DEFAULT] MAXGRES */

      P = &U->L.ActivePolicy;

      MPUCopyGLimit(P,&MSched.DefaultU->L.ActivePolicy);
      }

    if ((MSched.DefaultU->L.IdlePolicy != NULL) && (MSched.DefaultU->L.IdlePolicy->GLimit != NULL))
      {
      /* check USERCFG[DEFAULT] MAXIGRES */

      if (U->L.IdlePolicy == NULL)
        {
        MPUCreate(&U->L.IdlePolicy);
        }

      P = U->L.IdlePolicy;

      MPUCopyGLimit(P,MSched.DefaultU->L.IdlePolicy);
      }

    /* check group to user mappings (GROUPCFG[] MAX*[USER]) */

    if ((G != NULL) && 
        (G->L.UserActivePolicy != NULL) &&
        (MUHTGet(G->L.UserActivePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.UserActivePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,U->Name,mxoUser,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.APU != NULL)) */

    /* check class to user mappings (CLASSCFG[] MAX*[USER]) */

    if ((C != NULL) && 
        (C->L.UserActivePolicy != NULL) &&
        (MUHTGet(C->L.UserActivePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.UserActivePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,U->Name,mxoUser,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (C->L.APU != NULL)) */

    /* check account to user mappings (ACCOUNTCFG[] MAX*[USER]) */

    if ((A != NULL) && 
        (A->L.UserActivePolicy != NULL) &&
        (MUHTGet(A->L.UserActivePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.UserActivePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,U->Name,mxoUser,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.APU != NULL)) */

    /* check qos to user mappings (QOSCFG[] MAX*[USER]) */

    if ((Q != NULL) && 
        (Q->L.UserActivePolicy != NULL) &&
        (MUHTGet(Q->L.UserActivePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.UserActivePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,U->Name,mxoUser,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.APU != NULL)) */

    /*****
     * CHECK IDLE POLICIES
     */

    /* check group to user mappings (GROUPCFG[] MAX*[USER]) */

    if ((G != NULL) && 
        (G->L.UserIdlePolicy != NULL) &&
        (MUHTGet(G->L.UserIdlePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.UserIdlePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,U->Name,mxoUser,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.IPU != NULL)) */

    /* check class to user mappings (CLASSCFG[] MAX*[USER]) */

    if ((C != NULL) && 
        (C->L.UserIdlePolicy != NULL) &&
        (MUHTGet(C->L.UserIdlePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.UserIdlePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,U->Name,mxoUser,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (C->L.IPU != NULL)) */

    /* check account to user mappings (ACCOUNTCFG[] MAX*[USER]) */

    if ((A != NULL) && 
        (A->L.UserIdlePolicy != NULL) &&
        (MUHTGet(A->L.UserIdlePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.UserIdlePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,U->Name,mxoUser,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.IPU != NULL)) */

    /* check qos to user mappings (QOSCFG[] MAX*[USER]) */

    if ((Q != NULL) && 
        (Q->L.UserIdlePolicy != NULL) &&
        (MUHTGet(Q->L.UserIdlePolicy,MSched.DefaultU->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.UserIdlePolicy,U->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,U->Name,mxoUser,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.IPU != NULL)) */
    }      /* END if (U != NULL) */

  /**
   * check X to group mappings (*CFG[] MAX*[GROUP])
   * 
   * handle: J->Cred.U->L.APG USERCFG[]    MAX*[GROUP]
   *         J->Cred.C->L.APG CLASSCFG[]   MAX*[GROUP]
   *         J->Cred.A->L.APG ACCOUNTCFG[] MAX*[GROUP]
   *         J->Cred.Q->L.APG QOSCFG[]     MAX*[GROUP] 
   *
   */
 
  if (G != NULL)
    {
    /*****
     * CHECK ACTIVE POLICIES
     */

    if (MSched.DefaultG->L.ActivePolicy.GLimit != NULL)
      {
      /* check GROUPCFG[DEFAULT] MAXGRES */

      P = &G->L.ActivePolicy;

      MPUCopyGLimit(P,&MSched.DefaultG->L.ActivePolicy);
      }

    if ((MSched.DefaultG->L.IdlePolicy != NULL) && (MSched.DefaultG->L.IdlePolicy->GLimit != NULL))
      {
      /* check GROUPCFG[DEFAULT] MAXIGRES */

      if (G->L.IdlePolicy == NULL)
        {
        MPUCreate(&G->L.IdlePolicy);
        }

      P = G->L.IdlePolicy;

      MPUCopyGLimit(P,MSched.DefaultG->L.IdlePolicy);
      }

    /* check user to group mappings (USERCFG[] MAX*[GROUP]) */

    if ((U != NULL) && 
        (U->L.GroupActivePolicy != NULL) &&
        (MUHTGet(U->L.GroupActivePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.GroupActivePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,G->Name,mxoGroup,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.APG != NULL)) */

    /* check class to user mappings (CLASSCFG[] MAX*[GROUP]) */

    if ((C != NULL) && 
        (C->L.GroupActivePolicy != NULL) &&
        (MUHTGet(C->L.GroupActivePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.GroupActivePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,G->Name,mxoGroup,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (C->L.APG != NULL)) */

    /* check account to user mappings (ACCOUNTCFG[] MAX*[GROUP]) */

    if ((A != NULL) && 
        (A->L.GroupActivePolicy != NULL) &&
        (MUHTGet(A->L.GroupActivePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.GroupActivePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,G->Name,mxoGroup,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.APG != NULL)) */

    /* check qos to user mappings (QOSCFG[] MAX*[GROUP]) */

    if ((Q != NULL) && 
        (Q->L.GroupActivePolicy != NULL) &&
        (MUHTGet(Q->L.GroupActivePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.GroupActivePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,G->Name,mxoGroup,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.APG != NULL)) */

    /*****
     * CHECK IDLE POLICIES
     */

    /* check user to group mappings (USERCFG[] MAX*[GROUP]) */

    if ((U != NULL) && 
        (U->L.GroupIdlePolicy != NULL) &&
        (MUHTGet(U->L.GroupIdlePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.GroupIdlePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,G->Name,mxoGroup,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.IPG != NULL)) */

    /* check class to user mappings (CLASSCFG[] MAX*[GROUP]) */

    if ((C != NULL) && 
        (C->L.GroupIdlePolicy != NULL) &&
        (MUHTGet(C->L.GroupIdlePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.GroupIdlePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,G->Name,mxoGroup,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (C->L.IPG != NULL)) */

    /* check account to user mappings (ACCOUNTCFG[] MAX*[GROUP]) */

    if ((A != NULL) && 
        (A->L.GroupIdlePolicy != NULL) &&
        (MUHTGet(A->L.GroupIdlePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.GroupIdlePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,G->Name,mxoGroup,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.IPG != NULL)) */

    /* check qos to user mappings (QOSCFG[] MAX*[GROUP]) */

    if ((Q != NULL) && 
        (Q->L.GroupIdlePolicy != NULL) &&
        (MUHTGet(Q->L.GroupIdlePolicy,MSched.DefaultG->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.GroupIdlePolicy,G->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,G->Name,mxoGroup,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.IPG != NULL)) */
    }      /* END if (U != NULL) */

  /**
   * check X to class mappings (*CFG[] MAX*[CLASS])
   * 
   * handle: J->Cred.U->L.APC USERCFG[]    MAX*[CLASS]
   *         J->Cred.G->L.APC GROUPCFG[]   MAX*[CLASS]
   *         J->Cred.A->L.APC ACCOUNTCFG[] MAX*[CLASS]
   *         J->Cred.Q->L.APC QOSCFG[]     MAX*[CLASS] 
   *
   */
 
  if (C != NULL)
    {
    /*****
     * CHECK ACTIVE POLICIES
     */

    if (MSched.DefaultC->L.ActivePolicy.GLimit != NULL)
      {
      /* check CLASSCFG[DEFAULT] MAXGRES */

      P = &C->L.ActivePolicy;

      MPUCopyGLimit(P,&MSched.DefaultC->L.ActivePolicy);
      }

    if ((MSched.DefaultC->L.IdlePolicy != NULL) && (MSched.DefaultC->L.IdlePolicy->GLimit != NULL))
      {
      /* check CLASSCFG[DEFAULT] MAXIGRES */

      if (C->L.IdlePolicy == NULL)
        {
        MPUCreate(&C->L.IdlePolicy);
        }

      P = C->L.IdlePolicy;

      MPUCopyGLimit(P,MSched.DefaultC->L.IdlePolicy);
      }

    /* check group to class mappings (GROUPCFG[] MAX*[CLASS]) */

    if ((G != NULL) && 
        (G->L.ClassActivePolicy != NULL) &&
        (MUHTGet(G->L.ClassActivePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.ClassActivePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,C->Name,mxoClass,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.APC != NULL)) */

    /* check user to class mappings (USERCFG[] MAX*[CLASS]) */

    if ((U != NULL) && 
        (U->L.ClassActivePolicy != NULL) &&
        (MUHTGet(U->L.ClassActivePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.ClassActivePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,C->Name,mxoClass,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((U != NULL) && (U->L.APC != NULL)) */

    /* check account to class mappings (ACCOUNTCFG[] MAX*[CLASS]) */

    if ((A != NULL) && 
        (A->L.ClassActivePolicy != NULL) &&
        (MUHTGet(A->L.ClassActivePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.ClassActivePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,C->Name,mxoClass,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.APC != NULL)) */

    /* check qos to class mappings (QOSCFG[] MAX*[CLASS]) */

    if ((Q != NULL) && 
        (Q->L.ClassActivePolicy != NULL) &&
        (MUHTGet(Q->L.ClassActivePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.ClassActivePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,C->Name,mxoClass,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.APC != NULL)) */

    /*****
     * CHECK IDLE POLICIES
     */

    /* check group to class mappings (GROUPCFG[] MAX*[CLASS]) */

    if ((G != NULL) && 
        (G->L.ClassIdlePolicy != NULL) &&
        (MUHTGet(G->L.ClassIdlePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.ClassIdlePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,C->Name,mxoClass,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.IPC != NULL)) */

    /* check user to class mappings (USERCFG[] MAX*[CLASS]) */

    if ((U != NULL) && 
        (U->L.ClassIdlePolicy != NULL) &&
        (MUHTGet(U->L.ClassIdlePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.ClassIdlePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,C->Name,mxoClass,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((U != NULL) && (U->L.IPC != NULL)) */

    /* check account to class mappings (ACCOUNTCFG[] MAX*[CLASS]) */

    if ((A != NULL) && 
        (A->L.ClassIdlePolicy != NULL) &&
        (MUHTGet(A->L.ClassIdlePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.ClassIdlePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,C->Name,mxoClass,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.IPC != NULL)) */

    /* check qos to class mappings (QOSCFG[] MAX*[CLASS]) */

    if ((Q != NULL) && 
        (Q->L.ClassIdlePolicy != NULL) &&
        (MUHTGet(Q->L.ClassIdlePolicy,MSched.DefaultC->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.ClassIdlePolicy,C->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,C->Name,mxoClass,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.IPC != NULL)) */
    }      /* END if (C != NULL) */

  /**
   * check X to qos mappings (*CFG[] MAX*[QOS])
   * 
   * handle: J->Cred.U->L.APQ USERCFG[]    MAX*[QOS]
   *         J->Cred.G->L.APQ GROUPCFG[]   MAX*[QOS]
   *         J->Cred.A->L.APQ ACCOUNTCFG[] MAX*[QOS]
   *         J->Cred.C->L.APQ CLASSCFG[]   MAX*[QOS] 
   *
   */
 
  if (Q != NULL)
    {
    /*****
     * CHECK ACTIVE POLICIES
     */

    if (MSched.DefaultQ->L.ActivePolicy.GLimit != NULL)
      {
      /* check QOSCFG[DEFAULT] MAXGRES */

      P = &Q->L.ActivePolicy;

      MPUCopyGLimit(P,&MSched.DefaultQ->L.ActivePolicy);
      }

    if ((MSched.DefaultQ->L.IdlePolicy != NULL) && (MSched.DefaultQ->L.IdlePolicy->GLimit != NULL))
      {
      /* check QOSCFG[DEFAULT] MAXIGRES */

      if (Q->L.IdlePolicy == NULL)
        {
        MPUCreate(&Q->L.IdlePolicy);
        }

      P = Q->L.IdlePolicy;

      MPUCopyGLimit(P,MSched.DefaultQ->L.IdlePolicy);
      }

    /* check group to qos mappings (GROUPCFG[] MAX*[QOS]) */

    if ((G != NULL) && 
        (G->L.QOSActivePolicy != NULL) &&
        (MUHTGet(G->L.QOSActivePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.QOSActivePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,Q->Name,mxoQOS,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.APQ != NULL)) */

    /* check user to qos mappings (USERCFG[] MAX*[QOS]) */

    if ((U != NULL) && 
        (U->L.QOSActivePolicy != NULL) &&
        (MUHTGet(U->L.QOSActivePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.QOSActivePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,Q->Name,mxoQOS,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((U != NULL) && (U->L.APQ != NULL)) */

    /* check account to qos mappings (ACCOUNTCFG[] MAX*[QOS]) */

    if ((A != NULL) && 
        (A->L.QOSActivePolicy != NULL) &&
        (MUHTGet(A->L.QOSActivePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.QOSActivePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,Q->Name,mxoQOS,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.APQ != NULL)) */

    /* check class to qos mappings (CLASSCFG[] MAX*[QOS]) */

    if ((C != NULL) && 
        (Q->L.QOSActivePolicy != NULL) &&
        (MUHTGet(C->L.QOSActivePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.QOSActivePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,Q->Name,mxoQOS,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (Q->L.APQ != NULL)) */

    /*****
     * CHECK IDLE POLICIES
     */

    /* check group to qos mappings (GROUPCFG[] MAX*[QOS]) */

    if ((G != NULL) && 
        (G->L.QOSIdlePolicy != NULL) &&
        (MUHTGet(G->L.QOSIdlePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.QOSIdlePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,Q->Name,mxoQOS,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.IPQ != NULL)) */

    /* check user to qos mappings (USERCFG[] MAX*[QOS]) */

    if ((U != NULL) && 
        (U->L.QOSIdlePolicy != NULL) &&
        (MUHTGet(U->L.QOSIdlePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.QOSIdlePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,Q->Name,mxoQOS,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((U != NULL) && (U->L.IPQ != NULL)) */

    /* check account to qos mappings (ACCOUNTCFG[] MAX*[QOS]) */

    if ((A != NULL) && 
        (A->L.QOSIdlePolicy != NULL) &&
        (MUHTGet(A->L.QOSIdlePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(A->L.QOSIdlePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&A->L,Q->Name,mxoQOS,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((A != NULL) && (A->L.IPQ != NULL)) */

    /* check class to qos mappings (CLASSCFG[] MAX*[QOS]) */

    if ((C != NULL) && 
        (Q->L.QOSIdlePolicy != NULL) &&
        (MUHTGet(C->L.QOSIdlePolicy,MSched.DefaultQ->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.QOSIdlePolicy,Q->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,Q->Name,mxoQOS,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (Q->L.IPQ != NULL)) */
    }      /* END if (Q != NULL) */

  /**
   * check X to account mappings (*CFG[] MAX*[ACCT])
   * 
   * handle: J->Cred.U->L.APQ USERCFG[]    MAX*[ACCT]
   *         J->Cred.G->L.APQ GROUPCFG[]   MAX*[ACCT]
   *         J->Cred.Q->L.APQ QOSCFG[]     MAX*[ACCT]
   *         J->Cred.C->L.APQ CLASSCFG[]   MAX*[ACCT] 
   *
   */

  if (A != NULL)
    {
    /*****
     * CHECK ACTIVE POLICIES
     */

    if (MSched.DefaultA->L.ActivePolicy.GLimit != NULL)
      {
      /* check ACCOUNTCFG[DEFAULT] MAXGRES */

      P = &A->L.ActivePolicy;

      MPUCopyGLimit(P,&MSched.DefaultA->L.ActivePolicy);
      }

    if ((MSched.DefaultA->L.IdlePolicy != NULL) && (MSched.DefaultA->L.IdlePolicy->GLimit != NULL))
      {
      /* check ACCOUNTCFG[DEFAULT] MAXIGRES */

      if (A->L.IdlePolicy == NULL)
        {
        MPUCreate(&A->L.IdlePolicy);
        }

      P = A->L.IdlePolicy;

      MPUCopyGLimit(P,MSched.DefaultA->L.IdlePolicy);
      }

    /* check group to account mappings (GROUPCFG[] MAX*[ACCOUNT]) */

    if ((G != NULL) && 
        (G->L.AcctActivePolicy != NULL) &&
        (MUHTGet(G->L.AcctActivePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.AcctActivePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,A->Name,mxoAcct,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.APA != NULL)) */

    /* check user to account mappings (USERCFG[] MAX*[ACCOUNT]) */

    if ((U != NULL) && 
        (U->L.AcctActivePolicy != NULL) &&
        (MUHTGet(U->L.AcctActivePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.AcctActivePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,A->Name,mxoAcct,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((U != NULL) && (U->L.APA != NULL)) */

    /* check qos to account mappings (ACCOUNTCFG[] MAX*[ACCOUNT]) */

    if ((Q != NULL) && 
        (Q->L.AcctActivePolicy != NULL) &&
        (MUHTGet(Q->L.AcctActivePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.AcctActivePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,A->Name,mxoAcct,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.APA != NULL)) */

    /* check class to account mappings (CLASSCFG[] MAX*[ACCOUNT]) */

    if ((C != NULL) && 
        (C->L.AcctActivePolicy != NULL) &&
        (MUHTGet(C->L.AcctActivePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.AcctActivePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,A->Name,mxoAcct,mlActive,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (C->L.APA != NULL)) */

    /*****
     * CHECK IDLE POLICIES
     */

    /* check group to account mappings (GROUPCFG[] MAX*[ACCOUNT]) */

    if ((G != NULL) && 
        (G->L.AcctIdlePolicy != NULL) &&
        (MUHTGet(G->L.AcctIdlePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(G->L.AcctIdlePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&G->L,A->Name,mxoAcct,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((G != NULL) && (G->L.IPA != NULL)) */

    /* check user to account mappings (USERCFG[] MAX*[ACCOUNT]) */

    if ((U != NULL) && 
        (U->L.AcctIdlePolicy != NULL) &&
        (MUHTGet(U->L.AcctIdlePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(U->L.AcctIdlePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&U->L,A->Name,mxoAcct,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((U != NULL) && (U->L.IPA != NULL)) */

    /* check qos to account mappings (ACCOUNTCFG[] MAX*[ACCOUNT]) */

    if ((Q != NULL) && 
        (Q->L.AcctIdlePolicy != NULL) &&
        (MUHTGet(Q->L.AcctIdlePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(Q->L.AcctIdlePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&Q->L,A->Name,mxoAcct,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((Q != NULL) && (Q->L.IPA != NULL)) */

    /* check class to account mappings (CLASSCFG[] MAX*[ACCOUNT]) */

    if ((C != NULL) && 
        (C->L.AcctIdlePolicy != NULL) &&
        (MUHTGet(C->L.AcctIdlePolicy,MSched.DefaultA->Name,(void **)&DefaultP,NULL) == SUCCESS) &&
        (MUHTGet(C->L.AcctIdlePolicy,A->Name,(void **)&P,NULL) == FAILURE))
      {
      MLimitGetMPU(&C->L,A->Name,mxoAcct,mlIdle,&P);

      MPUCopy(P,DefaultP);
      }    /* END if ((C != NULL) && (C->L.IPA != NULL)) */
    }      /* END if (A != NULL) */

  return(SUCCESS);
  }  /* END MJobVerifyPolicyStructures() */



/*
 * Allocates and initializes an mpu_t structure.
 *
 * @ param PUP
 */

int MPUCreate(

  mpu_t **PUP)

  {
  mpu_t *PU;

  if (PUP == NULL)
    {
    return(FAILURE);
    }

  *PUP = NULL;

  PU = (mpu_t *)MUCalloc(1,sizeof(mpu_t));

  if (PU == NULL)
    {
    return(FAILURE);
    }

  MPUInitialize(PU);

  *PUP = PU;

  return(SUCCESS);
  }  /* END MPUCreate() */


/**
 * Initializes an mpu_t structure.
 *
 * @param PU
 */

int MPUInitialize(

  mpu_t *PU)

  {
  if (PU == NULL)
    {
    return(FAILURE);
    }

  MPUClearUsage(PU);

  memset(PU->SLimit,-1,sizeof(PU->SLimit));
  memset(PU->HLimit,-1,sizeof(PU->HLimit));

  return(SUCCESS);
  }  /* END MPUInitialize() */


/**
 * Initializes an mgpu_t structure.
 *
 * @param GPU
 */

int MGPUInitialize(

  mgpu_t *GPU)

  {
  if (GPU == NULL)
    {
    return(FAILURE);
    }

  MGPUClearUsage(GPU);

  memset(GPU->SLimit,-1,sizeof(GPU->SLimit));
  memset(GPU->HLimit,-1,sizeof(GPU->HLimit));

  return(SUCCESS);
  }  /* END MGPUInitialize() */




/**
 * Initializes an mpu_t structure for use as override.
 *
 * NOTE: overrides are set to 0 (not -1) this means you can't
 *       set override limits to 0, you can only "increase" the 
 *       override limit
 *
 * @param PU
 */

int MPUInitializeOverride(

  mpu_t *PU)

  {
  if (PU == NULL)
    {
    return(FAILURE);
    }

  MPUClearUsage(PU);

  memset(PU->SLimit,0,sizeof(PU->SLimit));
  memset(PU->HLimit,0,sizeof(PU->HLimit));

  return(SUCCESS);
  }  /* END MPUInitializeOverride() */





/**
 * Clear usage on mgpu_t structre.
 *
 * @param P
 */

int MGPUClearUsage(

  mgpu_t *P)

  {
  memset(P->Usage,0,sizeof(P->Usage));

  return(SUCCESS);
  }  /* END MGPUClearUsage() */


/**
 * Clear usage on mpu_t structre.
 *
 * @param P
 */

int MPUClearUsage(

  mpu_t *P)

  {
  memset(P->Usage,0,sizeof(P->Usage));

  return(SUCCESS);
  }  /* END MPUClearUsage() */




/**
 * Copy limits from source to destination.
 *
 * NOTE: this only copies hard and soft limits, not usage.
 *
 * @param D
 * @param S
 */

int MGPUCopy(

  mgpu_t *D,
  mgpu_t *S)

  {
  memcpy(D->SLimit,S->SLimit,sizeof(D->SLimit));
  memcpy(D->HLimit,S->HLimit,sizeof(D->HLimit));

  return(SUCCESS);
  }  /* END MGPUCopy() */





/**
 * Copy limits from source to destination.
 *
 * NOTE: this only copies hard and soft limits, not usage.
 *
 * @param D
 * @param S
 */

int MPUCopy(

  mpu_t *D,
  mpu_t *S)

  {
  memcpy(D->SLimit,S->SLimit,sizeof(D->SLimit));
  memcpy(D->HLimit,S->HLimit,sizeof(D->HLimit));

  if (S->GLimit != NULL)
    MPUCopyGLimit(D,S);

  return(SUCCESS);
  }  /* END MPUCopy() */


/**
 * Copy gres limits from source to destination.
 *
 * NOTE: this only copies hard and soft limits, not usage.
 *
 * @param D
 * @param S
 */

int MPUCopyGLimit(

  mpu_t *D,
  mpu_t *S)

  {
  mgpu_t *DP;
  mgpu_t *SP;

  mhashiter_t HTIter;

  char *VarName;

  MUHTIterInit(&HTIter);

  if (D->GLimit == NULL)
    {
    D->GLimit = (mhash_t *)MUCalloc(1,sizeof(mhash_t));
         
    MUHTCreate(D->GLimit,-1);
    }

  while (MUHTIterate(S->GLimit,&VarName,(void **)&SP,NULL,&HTIter) == SUCCESS)
    {
    /* don't override existing limits */

    if (MUHTGet(D->GLimit,VarName,NULL,NULL) == FAILURE)
      {
      DP = (mgpu_t *)MUCalloc(1,sizeof(mgpu_t));

      MGPUCopy(DP,SP);

      MUHTAdd(D->GLimit,VarName,(void *)DP,NULL,MUFREE);
      }
    }

  return(SUCCESS);
  }  /* END MPUCopyGLimit() */

/* END MLimit.c */
