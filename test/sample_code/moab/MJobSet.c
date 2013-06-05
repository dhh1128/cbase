/* HEADER */

/**
 * @file MJobSet.c
 *
 * Contains Job Setter functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


/**
 * Set job QoS.
 *
 * NOTE:  NULL QOS is allowed.
 *
 * @param J (I) [modified]
 * @param Q (I)
 * @param Mode (I) [not used]
 */

int MJobSetQOS(

  mjob_t *J,    /* I (modified) */
  mqos_t *Q,    /* I */
  int     Mode) /* I (not used) */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->Credential.Q == Q)
    {
    return(SUCCESS);
    }

  if (Q == &MQOS[0])
    {
    /* cannot use DEFAULT QOS */

    /* set a message on the job */

    MJobSetHold(J,mhDefer,MSched.DeferTime,mhrCredAccess,"DEFAULT credential for template use only");

    return(FAILURE);
    }

  /* qos has changed */

  if (J->Credential.Q != NULL)
    {
    /* disable special privileges of existing QOS */

    if (bmisset(&J->Credential.Q->Flags,mqfNTR) == TRUE)
      J->SystemPrio = 0;

    if ((J->Credential.Q->F.ReqRsv != NULL) && (J->ReqRID != NULL))
      {
      char tmpReqRID[MMAX_NAME];

      /* F.ReqRsv will be different if J->IFlags - mjifNotAdvres is set since J->ReqRID doesn't have ! but ReqRsv does. */

      snprintf(tmpReqRID,sizeof(tmpReqRID),"%s%s",
          (bmisset(&J->IFlags,mjifNotAdvres)) ? "!" : "",
          J->ReqRID);

      if (!strcmp(J->Credential.Q->F.ReqRsv,tmpReqRID))
        {
        MUFree(&J->ReqRID);

        bmunset(&J->SpecFlags,mjfAdvRsv);
        bmunset(&J->Flags,mjfAdvRsv);
        }
      }
    }  /* END if (J->Cred.Q != NULL) */

  if (J->QOSRequested == NULL)
    bmset(&J->IFlags,mjifQOSSetByDefault);

  J->Credential.Q = Q;

  MJobUpdateFlags(J);

  MJobBuildCL(J);

  /* QOS access can change partition access, update PAL */

  MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);

  if ((J->Credential.Q != NULL) && bmisset(&J->Credential.Q->Flags,mqfProvision) && (J->Req[0]->Opsys != 0))
    {
    bmset(&J->IFlags,mjifMaster);
    }  /* END if (bmisset(&J->Cred.Q->Flags,mqfProvision) && ...) */
  else if ((J->Credential.Q != NULL) && 
           (bmisset(&J->Credential.Q->Flags,mqfProvision)) && 
           (MSched.OnDemandProvisioningEnabled) &&
          ((J->Req[0]->Opsys == 0) || 
           (strcmp(MAList[meOpsys][J->Req[0]->Opsys],ANY) == 0)))
    {
    /* NOTE:  job can provision but this may cover provisioning VM's */

    /* singlejob server access should only be enfroced for server level 
       provisioning once provisioning is known to be required */

    bmset(&J->IFlags,mjifMaster);
    }

  /* Set system priority, if applicable */

  if ((Q != NULL) &&      
      (Q->SystemJobPriority >= 0) &&      
      (Q->SystemJobPriority <= 1000) &&
      (Q->SystemJobPriority > J->SystemPrio))

    {
    MJobSetAttr(J,mjaSysPrio,(void **)&Q->SystemJobPriority,mdfLong,mSet);
    }

  return(SUCCESS);
  }  /* END MJobSetQOS() */





mbool_t MJobHasCreatedEnvironment(

  mjob_t *J)  /* I */

  {
  if ((J == NULL) || (bmisset(&J->IFlags,mjifHasCreatedEnvironment)))
    {
    return(FALSE);
    }

  return(TRUE);
  }  /* END MJobHasCreatedEnvironment() */




/**
 * Apply user, system, batch, or defer hold to job.
 *
 * NOTE: if simulation purge job is TRUE, set job state to completed.
 *
 * @param J (I) [NOTE:  modified, possibly freed]
 * @param HoldTypeBM (I) [bitmap of enum mh*]
 * @param HoldDuration (I)
 * @param HoldReason (I)
 * @param HoldMsg (I) [optional]
 */

int MJobSetHold(
 
  mjob_t               *J,
  enum MHoldTypeEnum    HoldType,
  long                  HoldDuration,
  enum MHoldReasonEnum  HoldReason,
  const char           *HoldMsg)
 
  {
  mrm_t   *R;
  char     Line[MMAX_LINE];
  mbitmap_t NewHoldBM;
  
  char TString[MMAX_LINE];
  const char *FName = "MJobSetHold";

  MULToTString(HoldDuration,TString);
 
  MDB(3,fSCHED) MLog("%s(%s,%d,%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MHoldType[HoldType],
    TString,
    MHoldReason[HoldReason],
    (HoldMsg != NULL) ? HoldMsg : "NULL");
 
  if (!MJobPtrIsValid(J))
    {
    /* job has been removed or is a job array master */
 
    return(SUCCESS);
    }

  if ((MSched.DisableBatchHolds == TRUE) &&
      (HoldType == mhBatch))
    {
    /* only attach HoldMsg and ignore hold request */

    MMBAdd(
      &J->MessageBuffer,
      HoldMsg,
      NULL,
      mmbtHold,
      MSched.Time + MCONST_DAYLEN,
      0,
      NULL);

    return(SUCCESS);
    }

  if (bmisclear(&J->Hold))
    J->HoldTime = MSched.Time;

  if ((J->IsInteractive == TRUE) && (MSched.DontCancelInteractiveHJobs != TRUE))
    {
    snprintf(Line,sizeof(Line),"MOAB_INFO:  interactive job can never run - %s",
      (HoldMsg != NULL) ? HoldMsg : "no resources");

    bmset(&J->IFlags,mjifCancel);

    return(SUCCESS);
    }  /* END if (J->IsInteractive == TRUE) */

  /* NOTE:  hold reason attached to job as message for all hold types */

  if ((J->SubmitRM != NULL) &&
      (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue)) &&
      (HoldReason == mhrNoResources) &&
      (J->MigrateBlockReason == mjneNONE))
    {
    J->MigrateBlockReason = mjneNoResource;

    MUStrDup(&J->MigrateBlockMsg,HoldMsg);
    }
    
  if ((J->ReqRID != NULL) && 
      (HoldReason == mhrNoResources))
    {
    if ((MRsvFind(J->ReqRID,NULL,mraNONE) == SUCCESS) ||
        (MRsvFind(J->ReqRID,NULL,mraRsvGroup) == SUCCESS))
      {
      MDB(2,fSCHED) MLog("INFO:     job %s requests reservation %s.  (not deferring)\n",
        J->Name, 
        J->ReqRID);
 
      return(SUCCESS);
      }

    HoldType = mhBatch;
    }

  R = (J->SubmitRM != NULL) ? J->SubmitRM : &MRM[0];
 
  if ((R->FailIteration == MSched.Iteration) &&
      (HoldReason == mhrNoResources))
    {
    MDB(2,fSCHED) MLog("INFO:     connection to RM %s failed.  Not deferring job %s for reason %s\n",
      R->Name,
      J->Name,
      MHoldReason[HoldReason]);
 
    return(SUCCESS);
    }
 
  if (HoldType == mhDefer)
    {
    if (bmisset(&MSched.Flags,mschedfUnmigrateOnDefer))
      {
      MJobUnMigrate(J,FALSE,NULL,NULL);
      }

    if (MSched.DeferTime <= 0)
      {
      MDB(2,fSCHED) MLog("INFO:     defer disabled\n");

      if (!bmisset(&J->IFlags,mjifTemporaryJob))
        {
        snprintf(Line,sizeof(Line),"hold:%s",
          HoldMsg);

        MMBAdd(
          &J->MessageBuffer,
          Line,
          NULL,
          mmbtHold,
          MSched.Time + MCONST_DAYLEN,
          0,
          NULL);
        }
 
      return(SUCCESS);
      }
    }    /* END if (HoldType == mhDefer) */
  else
    {
    if (MPar[0].JobPrioAccrualPolicy == mjpapReset)
      {
      J->SystemQueueTime = MSched.Time;

      if (J->EffQueueDuration > 0)
        J->EffQueueDuration = 0;
      }
    }

  if (bmisset(&J->Hold,mhBatch))
    {
    MDB(2,fSCHED) MLog("INFO:     job '%s' already on batch hold\n",
      J->Name);
 
    return(SUCCESS);
    }

  if (HoldType == mhDefer)
    {
    if (!bmisset(&J->Hold,mhDefer))
      {
      MDB(2,fSCHED) MLog("ALERT:    job '%s' cannot run (deferring job for %ld seconds)\n",
        J->Name,
        HoldDuration);

      if ((HoldMsg != NULL) && (HoldMsg[0] != '\0'))
        {
        /* annotate job object */
 
        if (MRMJobModify(
             J,
             "comment",
             NULL,
             HoldMsg,
             mSet,
             NULL,
             NULL,
             NULL) == FAILURE)
          {
          MDB(2,fSCHED) MLog("INFO:     cannot annotate job '%s' with message '%s'\n",
            J->Name,
            HoldMsg);
          }
        }
      }
    }     /* END if (HoldType == mhDefer) */

  if (!bmisset(&J->IFlags,mjifTemporaryJob))
    {
    MMBAdd(
      &J->MessageBuffer,
      HoldMsg,
      NULL,
      mmbtHold,
      MSched.Time + MCONST_DAYLEN,
      0,
      NULL);
    }
 
  MDB(2,fSCHED)
    MJobShow(J,0,NULL);
 
  if ((MSched.Mode == msmSim) &&
      (MSim.PurgeBlockedJobs == TRUE) &&
      (HoldDuration != MSched.MaxRMPollInterval))
    {
    MDB(0,fSCHED) MLog("INFO:     job '%s' removed on iteration %d (cannot run in simulated environment), reason: %s\n",
      J->Name,
      MSched.Iteration,
      MHoldReason[HoldReason]);

    if (!bmisset(&J->IFlags,mjifTemporaryJob))
      {
      /* only adjust usage if job has been 'officially' submitted */

      MStatAdjustEJob(J,NULL,-1);
      }

    /* NOTE:  remove should be moved to end of iteration processing */

    /* mark job completed */

    MSim.RejectJC++;
    MSim.RejReason[marHold]++;
 
    MJobSetState(J,mjsCompleted);

    /* MJobRemove(&J); */
 
    /* return(SUCCESS); */
    }

  /* apply hold to job */

  MJobReleaseRsv(J,TRUE,TRUE);

  if (!bmisset(&J->Hold,HoldType))  /* see if a new hold is being applied (JMB) */
    {
    if (J->Triggers == NULL)
      bmset(&J->IFlags,mjifNewBatchHold);

    MOReportEvent(J,J->Name,mxoJob,mttHold,MSched.Time,TRUE);

    MOWriteEvent((void *)J,mxoJob,mrelJobHold,NULL,MStat.eventfp,NULL);

    CreateAndSendEventLog(meltJobHold,J->Name,mxoJob,(void *)J);
    }

  bmcopy(&NewHoldBM,&J->Hold);

  if (HoldType == mhNONE)
    bmclear(&NewHoldBM);
  else 
    bmset(&NewHoldBM,HoldType);

  J->HoldReason = HoldReason;    

  if (HoldType == mhDefer)
    {
    J->EState       = mjsDeferred;

    J->DeferCount++;

    MDB(3,fSCHED) MLog("INFO:     job %s is deferred, DeferCount=%d  Reason=%s  Message='%s'\n",
      J->Name,
      J->DeferCount,
      MHoldReason[HoldReason],
      (HoldMsg != NULL) ? HoldMsg : "NULL");
    }
 
  /* if job has failed too many times */
 
  if ((MSched.DeferCount > 0) && 
      (J->DeferCount > MSched.DeferCount))
    {
    char CData[MMAX_LINE];  /* failure context data */

    MDB(1,fSCHED) MLog("INFO:     batch hold placed on job '%s', reason: '%s'\n",
      J->Name, 
      MHoldReason[HoldReason]);
 
    bmset(&NewHoldBM,mhBatch);

    CData[0] = '\0';

    switch (HoldReason)
      {
      case mhrRMReject:

        if (!MNLIsEmpty(&J->NodeList))
          {
          mnode_t *N;

          MNLGetNodeAtIndex(&J->NodeList,0,&N);

          sprintf(CData,"(master node: %s)",
            N->Name);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (HoldReason) */

    sprintf(Line,"JOBHOLD:  batch hold placed on job '%s'.  defercount: %d  reason: '%s'%s",
      J->Name,
      J->DeferCount,
      MHoldReason[HoldReason],
      CData);

    MSysRegEvent(Line,mactNONE,1); 
    }
  else
    {
    if ((HoldReason == mhrRMReject) || (HoldReason == mhrAPIFailure))
      {
      sprintf(Line,"JOBDEFER:  defer hold placed on job '%s'.   reason: '%s'",
        J->Name,
        MHoldReason[HoldReason]);
      
      MSysRegEvent(Line,mactNONE,1);
      }
    }

  /* commit changes to hold */

  MJobSetAttr(J,mjaHold,(void **)&NewHoldBM,mdfOther,mSet);

  return(SUCCESS);
  }  /* END MJobSetHold() */




/**
 * Sets the state of the job.
 *
 * @param J (I) [modified]
 * @param NewState (I)
 */

int MJobSetState(

  mjob_t             *J,         /* I (modified) */
  enum MJobStateEnum  NewState)  /* I */

  {
  enum MJobStateEnum oldState;

  const char *FName = "MJobSetState";

  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MJobState[NewState]);
  
  /* keep track of old state to determine if to transition new state or not. */

  if (J == NULL)
    {
    return(FAILURE);
    }

  oldState = J->State;

  switch (NewState)
    {
    case mjsIdle:

      J->State  = mjsIdle;
      J->EState = mjsIdle;

      break;

    case mjsStarting:
    case mjsRunning:

      /*
       * Why do we want to prevent Moab from assigning a state of "Starting" ? 
       * Josh commented this code on Aug. 15, 2007
      if (MSched.EnableJITServiceProvisioning == TRUE)
        {
        J->State  = NewState;
        J->EState = NewState;
        }
      else
        {
        J->State  = mjsRunning;
        J->EState = mjsRunning;
        }
        */

      J->State = NewState;
      J->EState = NewState;

      MJobUpdateResourceCache(J,NULL,0,NULL);

      /* NOTE:  incorporate checkpoint/suspend time */

      if (J->StartTime > 0)
        {
        if (J->AWallTime > 0)
          {
          J->RemainingTime = 
            ((long)J->WCLimit > J->AWallTime) ? (J->WCLimit - J->AWallTime) : 0;
          }
        else
          {
          J->RemainingTime = J->WCLimit + J->SWallTime;

          if (MSched.Time > J->StartTime)
            {
            long tmpL;

            tmpL = J->RemainingTime - (MSched.Time - J->StartTime);

            J->RemainingTime = (tmpL > 0) ? tmpL : 0;
            }
          }
        }
      else
        {
        J->RemainingTime = J->WCLimit;
        }

      break;

    case mjsSuspended:

      J->State  = mjsSuspended;
      J->EState = mjsSuspended;

      break;

    default:

      J->State  = NewState;
      J->EState = NewState;
 
      break;
    }  /* END switch (NewState) */

  if (oldState != NewState)
    {
    /* only mark state modified if it actually was */

    J->StateMTime = MSched.Time;
  
    MJobTransition(J,FALSE,FALSE);
    }
 
  return(SUCCESS);
  }  /* END MJobSetState() */



/**
 * Set job defaults including default and set job templates.
 *
 * NOTE:  has dependency on J->SRM (may require J->RMXString to be processed first)
 *
 * @see MRMJobPostLoad() - parent
 * @see MJobApplyDefTemplate() - child - apply default templates
 * @see MJobApplySetTemplate() - child - apply set templates
 * @see MJobGetTemplate() - child - locate matching templates
 *
 * @param J (I) [modified]
 * @param ApplySetTemplates (I)
 */

int MJobSetDefaults(

  mjob_t  *J,                  /* I (modified) */
  mbool_t  ApplySetTemplates)  /* I */

  {
  int rqindex;
  int jindex;

  mreq_t *RQ;

  mjob_t *CJDef = NULL;
  mjob_t *QJDef = NULL;

  mjob_t *CJSet = NULL;

  mjtmatch_t *JTM;

  mjob_t *DJ = NULL;       /* default job template */
  mjob_t *SJ = NULL;       /* set job template */
  mjob_t *UJ = NULL;       /* unset job template */
  mjob_t **TStats = NULL;  /* template statistics */

  long tmpWallTime;

  mbool_t TemplateAlreadyApplied = FALSE;

  mrm_t  *SRM;

  int     JTok;

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* If J was loaded from CP file, templates have already been applied */

  if (bmisset(&J->IFlags,mjifWasLoadedFromCP))
    {
    TemplateAlreadyApplied = TRUE;
    }

  /* get credential defaults */

  if (J->Credential.Q != NULL)
    QJDef = J->Credential.Q->L.JobDefaults;

  if (J->Credential.C != NULL)
    CJDef = J->Credential.C->L.JobDefaults;

  /* set the walltime based on class/user defaults */

  if ((J->SpecWCLimit[0] <= 0) || (J->SpecWCLimit[0] > MMAX_TIME))
    {
    MJobGetDefaultWalltime(J,&tmpWallTime);

    if (tmpWallTime != 0)
      {
      char TString[MMAX_LINE];

      MULToTString(tmpWallTime,TString);

      MDB(4,fRM) MLog("INFO:     wallclock limit set to %s on job %s\n",
        TString,
        J->Name);

      J->SpecWCLimit[0] = tmpWallTime;
      J->SpecWCLimit[1] = tmpWallTime;
      } /* END if (tmpWallTime != 0) */
    }   /* END if (J->SpecWCLimit[0] <= 0) */

  /* process req level job defaults */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ->DRes.Procs == 0)
      {
      /* do not apply defaults for virtual resource reqs */

      continue; 
      }

    if (RQ->TasksPerNode <= 0)
      {
      /* check QOS and class defaults only */

      /* NOTE:  JDef->FNC contains per req TPN info */

      if ((CJDef != NULL) && (CJDef->FloatingNodeCount > 0))
        {
        RQ->TasksPerNode = CJDef->FloatingNodeCount;
        }
      else if ((QJDef != NULL) && (QJDef->FloatingNodeCount > 0))
        {
        RQ->TasksPerNode = QJDef->FloatingNodeCount;
        }
      }

    if (CJDef != NULL)
      {
      if (RQ->DRes.Mem <= 0)
        {
        if (MJobWantsSharedMem(J) && (CJDef->Req[0]->DRes.Mem > 0))
          {
          RQ->ReqMem = CJDef->Req[0]->DRes.Mem;
          }
        else if ((CJDef->Req[0] != NULL) && (CJDef->Req[0]->DRes.Mem > 0))
          {
          RQ->DRes.Mem = CJDef->Req[0]->DRes.Mem;
          }
        }

      if (RQ->DRes.Disk <= 0)
        {
        if ((CJDef->Req[0] != NULL) && (CJDef->Req[0]->DRes.Disk > 0))
          {
          RQ->DRes.Disk = CJDef->Req[0]->DRes.Disk;
          }
        }

      if ((RQ->NAccessPolicy == mnacNONE) &&
          (CJDef->Req[0] != NULL) && 
          (CJDef->Req[0]->NAccessPolicy != mnacNONE))
        {
        RQ->NAccessPolicy = CJDef->Req[0]->NAccessPolicy;

        MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from defaults\n",
          J->Name,
          MNAccessPolicy[RQ->NAccessPolicy]);
        }
      }

    /* Question - is CJSet ever set to anything other than NULL? */

    if (CJSet != NULL)
      {
      if ((CJSet->Req[0] != NULL) && (CJSet->Req[0]->NAccessPolicy != mnacNONE))
        {
        RQ->NAccessPolicy = CJSet->Req[0]->NAccessPolicy;

        MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from defaults\n",
          J->Name,
          MNAccessPolicy[RQ->NAccessPolicy]);
        }
      }
    }    /* END for (rqindex) */

  /* NOTE:  for 'internal' jobs, must determine effective source RM */

  if (J->SubmitRM != NULL)
    {
    SRM = J->SubmitRM;

    if (bmisset(&SRM->IFlags,mrmifLocalQueue))
      {
      if (J->SystemID != NULL)
        {
        mrm_t *tRM;

        if ((MRMFind(J->SystemID,&tRM) == SUCCESS) && 
           ((bmisset(&tRM->Flags,mrmfClient)) || (strstr(J->SystemID,".inbound") != NULL)))
          {
          /* specified RM is client interface */

          SRM = tRM;
          }
        else
          {
          char tmpLine[MMAX_LINE];

          /* search for client interface */

          sprintf(tmpLine,"%s.inbound",
            J->SystemID);

          if (MRMFind(tmpLine,&tRM) == SUCCESS)
            SRM = tRM;
          }
        }
      }    /* END if (bmisset(&SRM->IFlags,mrmifLocalQueue)) */ 
    }      /* END if (J->SRM != NULL) */
  else
    {
    SRM = NULL;
    }

  /* apply RM default/set template */

  if (TemplateAlreadyApplied == FALSE)
    {
    if ((SRM != NULL) && (SRM->JDef != NULL))
      {
      MJobApplyDefTemplate(J,SRM->JDef);
      }

    if ((SRM != NULL) && (SRM->JSet != NULL))
      {
      MJobApplySetTemplate(J,SRM->JSet,NULL);
      }
    } /* END if (TemplateAlreadyApplied == FALSE) */

  JTok   = -1;
  jindex =  0;

  if ((J->System == NULL) && (MSched.DefaultJ != NULL))
    {
    /* MSched.DefaultJ is JDef template set via 'JOBCFG[DEFAULT]' */

    DJ     = MSched.DefaultJ;
    SJ     = NULL;
    UJ     = NULL;
    TStats = NULL;

    /* set JTM to allow processing in loop below */

    JTM = (mjtmatch_t *)1;
    }
  else if ((J->System != NULL) && (MTJobFind("SYSTEM",&DJ) == SUCCESS))
    {
    SJ     = NULL;
    UJ     = NULL;
    TStats = NULL;

    /* set JTM to allow processing in loop below */

    JTM = (mjtmatch_t *)1;
    }
  else
    {
    /* get first match specified via JOBMATCHCFG[] */

    JTM = MJobGetTemplate(J,MJTMatch,MMAX_TJOB,&JTok);  /* will return NULL on failure */

    if (JTM != NULL)
      {
      DJ     = (mjob_t *)JTM->JDef;
      SJ     = (mjob_t *)JTM->JSet;
      UJ     = (mjob_t *)JTM->JUnset;

      TStats = (mjob_t **)JTM->JStat;

      if (J->Credential.Templates == NULL)
        {
        J->Credential.Templates = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoxTJob]);
        }

      J->Credential.Templates[jindex++] = JTM->Name;
      }
    }

  if (bmisset(&J->IFlags,mjifCreatedByTemplateDepend))
    {
    /* Don't apply job matching to jobs that were created
        by job template dependencies */

    JTM = NULL;
    }

  while (JTM != NULL)
    {
    if (TStats != NULL)
      {
      int tindex;

      for (tindex = 0;tindex < MMAX_JTSTAT;tindex++)
        {
        if (TStats[tindex] == NULL)
          break;

        MULLAdd(&J->TStats,TStats[tindex]->Name,(void *)TStats[tindex],NULL,NULL);
        }  /* END for (tindex) */
      }    /* END if (TStats != NULL) */

    /* NOTE: Default and set templates may require use of TStat information */

    if (DJ != NULL)
      {
      /* we don't want to apply the template twice */

      if (TemplateAlreadyApplied == FALSE)
        {
        MJobApplyDefTemplate(J,DJ);
        }

      /* we do want the template name applied */

      MULLAdd(&J->TSets,DJ->Name,(void *)DJ,NULL,NULL);
      }          /* END if (DJ != NULL) */

    if (ApplySetTemplates == TRUE)
      {
      if (SJ != NULL)
        {
        /* only apply it the template if we haven't already... */

        if (TemplateAlreadyApplied == FALSE)
          {
          MJobApplySetTemplate(J,SJ,NULL);
          }

        /* we want the template name applied regardless */

        MULLAdd(&J->TSets,SJ->Name,(void*)DJ,NULL,NULL);
        } /* END if(SJ != NULL) */

      if (UJ != NULL)
        {
        /* only apply it the template if we haven't already... */

        if (TemplateAlreadyApplied == FALSE)
          {
          MJobApplyUnsetTemplate(J,UJ);
          }

        /* we want the template name applied regardless */
        
        MULLAdd(&J->TSets,UJ->Name,(void*)UJ,NULL,NULL);
        } /* END if (UJ != NULL) */

      }  /* END if (ApplySetTemplates == TRUE) */

    JTM = MJobGetTemplate(J,MJTMatch,MMAX_TJOB,&JTok);

    if (JTM != NULL)
      {
      DJ     = (mjob_t *)JTM->JDef;
      SJ     = (mjob_t *)JTM->JSet;
      TStats = (mjob_t **)JTM->JStat;

      if (J->Credential.Templates == NULL)
        {
        J->Credential.Templates = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoxTJob]);
        }

      if (jindex < MSched.M[mxoxTJob] - 1)
        {
        J->Credential.Templates[jindex++] = JTM->Name;

        J->Credential.Templates[jindex] = NULL;
        }
      }    /* END if (JTM != NULL) */
    }      /* END while (DJ != NULL) */

  /* apply RM set template */

  if ((SRM != NULL) && (SRM->JSet != NULL) && (TemplateAlreadyApplied == FALSE))
    {
    MJobApplySetTemplate(J,SRM->JSet,NULL);
    }

  /* NOTE: this line disabled any recovery of queueduration from checkpoint file */
  /*       disabling  7/17/07 drw */

#ifdef  MNOT
  J->EffQueueDuration = -1;  /* NOTE: this may have side affects (Feb 23, 2006) */
#endif

  return(SUCCESS);
  }  /* END MJobSetDefaults() */





/**
 *
 *
 * @param J (I) [modified]
 * @param SDRQ (I) [optional]
 * @param TaskList (I/O) [modified,minsize=MSched.JobMaxTaskCount,terminated w/-1]
 */

int MJobSetAllocTaskList(

  mjob_t *J,        /* I (modified) */
  mreq_t *SDRQ,     /* I default req (optional) */
  int    *TaskList) /* I/O (modified,minsize=MSched.JobMaxTaskCount,terminated w/-1) */

  {
  int tindex;
  int rqindex;

  mbool_t IsMultiReq;

  mreq_t *RQ;

  mreq_t *DefReq;

  const char *FName = "MJobSetAllocTaskList";

  if ((J == NULL) || (TaskList == NULL) || (J->Req[0] == NULL))
    {
    return(FAILURE);
    }

  DefReq = SDRQ;

  tindex = 0;

  IsMultiReq = (J->Req[1] == NULL) ? FALSE : TRUE;

  if (IsMultiReq == TRUE)
    {
    /* multi-req job detected */

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ->DRes.Procs == 0)
        {
        /* handle local resources */

        MReqAllocateLocalRes(J,RQ);
        }
      else if (DefReq == NULL)
        {
        /* assign tasks to first compute req */

        DefReq = RQ;
        }
      }  /* END for (rqindex) */

    for (tindex = 0;tindex < MSched.JobMaxTaskCount;tindex++)
      {
      if (TaskList[tindex] == -1)
        break;
      }  /* END for (tindex) */

    MJobGetLocalTL(J,FALSE,&TaskList[tindex],MSched.JobMaxTaskCount - tindex);
    }  /* END if (J->Req[1] != NULL) */
  else
    {
    if (bmisset(&J->IFlags,mjifPBSGPUSSpecified))
      MJobTranslateNCPUTaskList(J,TaskList);

    /* single req job */

    for (tindex = 0;tindex < MSched.JobMaxTaskCount;tindex++)
      {
      /* count up number of tasks in TaskList */

      if (TaskList[tindex] == -1)
        break;
      }  /* END for (tindex) */
    }

  if ((tindex >= J->TaskMapSize) && 
      (MJobGrowTaskMap(J,tindex - J->TaskMapSize + 16) == FAILURE))
    {
    MDB(1,fRM) MLog("ALERT:    cannot allocate job tasklist for job %s in %s\n",
      J->Name,
      FName);

    return(FAILURE);
    }

  if (MJobNLFromTL(J,&J->NodeList,TaskList,&J->Alloc.NC) == FAILURE)
    {
    MDB(1,fRM) MLog("ALERT:    cannot translate tasklist to nodelist for job %s in %s\n",
      J->Name,
      FName);

    return(FAILURE);
    }

  /* The job's taskcount is now known, set Alloc.TC. In a grid, Alloc.TC wasn't being set
   * until MRMJobPostUpdate at which point the the NodeList's were cleared. This caused
   * the job to show more tasks than allocated in showq because the hostlist was cleared. */

  J->Alloc.TC = tindex;

  if (J->TaskMap != NULL)
    memcpy(J->TaskMap,TaskList,sizeof(int) * J->TaskMapSize);

  if (DefReq == NULL)
    DefReq = J->Req[0];

  if (bmisset(&J->IFlags,mjifHostList) && (J->Req[J->HLIndex] != NULL))
    {
    /* tasklist specified for hostlist req only */

    rqindex = J->HLIndex;

    MNLCopy(&J->Req[rqindex]->NodeList,&J->NodeList);
    }  /* END if (bmisset(&J->IFlags,mjifHostList) && (J->Req[rqindex] != NULL)) */
  else if ((MNLIsEmpty(&DefReq->NodeList)) ||
          ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtMoab)))
    {
    int tmpT;

    /* assign compute tasks (tasks 0-tindex) to primary req */

    tmpT = TaskList[tindex];
    TaskList[tindex] = -1;

    MJobNLFromTL(J,&DefReq->NodeList,TaskList,NULL);

    TaskList[tindex] = tmpT;
    }

  return(SUCCESS);
  }  /* END MJobSetAllocTaskList() */




/**
 * Parses and sets the nodeset for a given job.
 *
 * FORMAT:  <SETTYPE>:<SETATTR>[:<SETLIST>]
 *
 * NOTE: puts the nodeset requirement on ALL COMPUTE reqs of the job
 *
 * @param J
 * @param NString
 * @param ApplyToAll
 */

int MJobSetNodeSet(

  mjob_t *J,
  char   *NString,
  mbool_t ApplyToAll)

  {
  int rqindex;
  int index;

  char *tmpLine = NULL;
  char *ptr;
  char *TokPtr = NULL;

  mreq_t *RQ;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    if ((ApplyToAll == FALSE) && (MReqIsGlobalOnly(RQ)))
      continue;

    MUStrDup(&tmpLine,NString);

    /* FORMAT:  <SETTYPE>:<SETATTR>[:<SETLIST>] */

    if ((ptr = MUStrTok(tmpLine,":",&TokPtr)) != NULL)
      {
      RQ->SetSelection = (enum MResourceSetSelectionEnum)MUGetIndexCI(
        ptr,
        MResSetSelectionType,
        FALSE,
        mrssNONE);

      bmset(&J->IFlags,mjifSpecNodeSetPolicy);

      if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
        {
        RQ->SetType = (enum MResourceSetTypeEnum)MUGetIndexCI(
          ptr,
          MResSetAttrType,
          FALSE,
          mrstNONE);

        index = 0;

        RQ->SetList = (char **)MUCalloc(1,sizeof(char *) * MMAX_ATTR);

        while ((ptr = MUStrTok(NULL,":,",&TokPtr)) != NULL)
          {
          MUStrDup(&RQ->SetList[index],ptr);

          index++;
          }
        }
      }   /* END if ((ptr = MUStrTok(Value,":",&TokPtr)) != NULL) */
    }     /* END for (rqindex) */

  MUFree(&tmpLine);

  return(SUCCESS);
  }  /* END MJobSetNodeSet() */


/**
 * Sets an dynamic attribute (minternal_attr_t)
 *  There is no mode because these are simple types (char *,int,etc).
 *
 * Format technically refers to the format of Value, but this function will
 *  return FAILURE if it is not the same as the expected type for the given
 *  index.  Only indexes that use the dynamic attributes should be supported.
 *
 * Returns SUCCESS if the var was set.
 *
 * @param J      [I] (modified) - The job to set the attribute on
 * @param AIndex [I] - The attribute to modify
 * @param Value  [I] - Value to set attribute to
 * @param Format [I] - Format of the attribute
 */

int MJobSetDynamicAttr(
  mjob_t              *J,      /* I (modified) */
  enum MJobAttrEnum    AIndex, /* I */
  void               **Value,  /* I */
  enum MDataFormatEnum Format) /* I */

  {
  if ((J == NULL) || 
      (Value == NULL) ||
      (AIndex == mjaNONE))
    return(FAILURE);

  switch (Format)
    {
    case mdfString:

      {
      switch (AIndex)
        {
        /* Put all accepted char * attrs here */

        case mjaTrigNamespace:
        case mjaCancelExitCodes:

          MUDynamicAttrSet(AIndex,&J->DynAttrs,Value,Format);

          break;

        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfString */

      break;

    case mdfInt:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfInt */

      break;

    case mdfDouble:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfDouble */

      break;

    case mdfLong:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfLong */

      break;

    default:

      /* Type not supported */

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Format) */

  return(SUCCESS);
  }  /* END MJobSetDynamicAttr() */



/**
 * Set job start priority.
 *
 * @param J (I)
 * @param pindex (I)
 * @param Priority (I)
 */

void MJobSetStartPriority(

  mjob_t *J,        /* I */
  int     pindex,   /* I */
  long    Priority) /* I */

  {
  J->CurrentStartPriority = Priority;
  J->PStartPriority[pindex] = Priority;

  if (MSched.PerPartitionScheduling != TRUE)
    {
    int PIndex = 0;

    /* set it in all the partitions just in case it is set now for partition 0
       and later on we look for the priority in the "real" partition for this job. */

    for (PIndex = 0; PIndex < MMAX_PAR; PIndex++)
      {
      J->PStartPriority[PIndex] = Priority;
      }
    }
  } /* END MJobSetStartPriority() */


