/* HEADER */

      
/**
 * @file MJobTemplates.c
 *
 * Contains:
 *    MJobApplySetTemplate
 *    MJobTemplateIsValidGenSysJob
 *    MJobGetTemplate
 *    MJobApplyDefTemplate
 *    MJobApplyUnsetTemplate
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

int __MJobTemplateApplyWorkflow(mjob_t *,mjob_t *);

/**
 * Force/apply 'set' job template attributes to specified job.
 *
 * NOTE:  For typical job discovery, called by MRMJobPostLoad()->MJobSetDefaults()
 * NOTE:  Sync w/MJobApplyDefTemplate() - peer
 *
 * TODO: Have MJobApply[Set|Def]Template call one call that does the work or both commands.
 *
 * @see MJobGetTemplate() - locate template which matches specified min/max criteria
 * @see MJobApplyDefTemplate() - apply specified attributes as defaults onto target job
 * @see MJobApplyUnsetTemplate() - remove specified attributes from target job
 * @see MJobCreateTemplateDependency() - child
 *
 * Supported Attributes:
 * Required Features (RFEATURES)
 * Partition Access List (PARTITIONMASK)
 *
 * @param J    (I) [modified] - Job to have template applied to it
 * @param SJ   (I) - The job template to apply
 * @param EMsg (O) - Optional error message (minsize = MMAX_LINE)
 */

int MJobApplySetTemplate(

  mjob_t *J,    /* I (modified) */
  mjob_t *SJ,   /* I */
  char   *EMsg) /* O (optional) */

  {
  int     rqindex;

  mbool_t WallTimeChanged = FALSE;
  
  mreq_t *RQ;
  mreq_t *SRQ;

  const char *FName = "MJobApplySetTemplate";

  MDB(4,fRM) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (SJ != NULL) ? SJ->Name : "NULL");

  if ((J == NULL) || (SJ == NULL))
    {
    return(FAILURE);
    }

  if (bmisset(&SJ->IFlags,mjifTemplateIsDeleted))
    {
    return(FAILURE);
    }

  if (bmisset(&SJ->IFlags,mjifGenericSysJob))
    {
    if (MJobTemplateIsValidGenSysJob(SJ))
      {
      /* Job is a generic system job */

      mtrig_t *GenT;

      bmset(&J->IFlags,mjifGenericSysJob);
      bmset(&J->Flags,mjfNoRMStart);
      bmset(&J->SpecFlags,mjfNoRMStart);

      if (J->System == NULL)
        J->System = (msysjob_t *)MUCalloc(1,sizeof(msysjob_t));

      if (J->System == NULL)
        return(FAILURE);

      J->System->JobType = msjtGeneric;
      J->System->CompletionPolicy = mjcpGeneric;

      /* Set walltime (unset on template if specified) */

      GenT = (mtrig_t *)MUArrayListGetPtr(SJ->Triggers,0);
      MJobSetAttr(J,mjaReqAWDuration,(void **)&GenT->Timeout,mdfLong,mSet);

      J->WCLimit = GenT->Timeout;
      J->SpecWCLimit[0] = GenT->Timeout;
      J->System->WCLimit = GenT->Timeout;

      MJobTransition(J,FALSE,TRUE);

      WallTimeChanged = TRUE;

      /* NOTE: generic system jobs require a trigger, but that will be set
                like a normal job template trigger */
      }
    else
      {
      /* Incomplete generic system job specification */

      MDB(3,fALL) MLog("ERROR:    job template '%s' has incomplete generic system job specification\n",
        SJ->Name);

      if (EMsg != NULL)
        {
        sprintf(EMsg,"job template '%s' has incomplete generic system job specification\n",
          SJ->Name);
        }

      return(FAILURE);
      }
    } /* END if (bmisset(&SJ->IFlags,mjifGenericSysJob)) */
  else if (bmisset(&SJ->Flags,mjfVMTracking))
    {
    char *VMID = NULL;
    char *EVMID = NULL;

    /* Jobs can request VM name with the VMID variable.  To create a new
        VM, you cannot request a VMID that is already in use */

    if (MUHTGet(&J->Variables,"VMID",(void **)&VMID,NULL) == SUCCESS)
      {
      /* NOTE:  only check existing IF 'ExistingVMId' NOT explicitly specified */

      if (MUHTGet(&J->Variables,"EVMID",(void **)&EVMID,NULL) == FAILURE)
        {
        if ((MVMFind(VMID,NULL) == SUCCESS) ||
            (MVMFindCompleted(VMID,NULL) == SUCCESS) ||
            (MUHTGet(&MVMIDHT,VMID,NULL,NULL) == SUCCESS))
          {
          /* VMID is already in use, reject */

          MDB(3,fALL) MLog("ERROR:    job template '%s' could not be applied because job '%s' requested an existing VMID\n",
            SJ->Name,
            J->Name);

          if (EMsg != NULL)
            {
            sprintf(EMsg,"job template '%s' could not be applied because job '%s' requested an existing VMID\n",
              SJ->Name,
              J->Name);
            }

          return(FAILURE);
          }
        }

      MDB(7,fSTRUCT) MLog("INFO:     adding '%s' to requested VMIDs for job '%s'\n",
        VMID,
        J->Name);

      MUHTAdd(&MVMIDHT,VMID,NULL,NULL,NULL);
      }  /* END if (MUHTGet(&J->Variables,"VMID",(void **)&VMID,NULL) == SUCCESS) */
    else
      {
      /* Need to choose a VMID */

      mstring_t tmpS(MMAX_LINE);
      char tmpVMID[MMAX_NAME];

      MVMGetUniqueID(tmpVMID);
      MStringSetF(&tmpS,"VMID=%s",tmpVMID);

      MJobSetAttr(J,mjaVariables,(void **)tmpS.c_str(),mdfString,mAdd);

      MDB(7,fSTRUCT) MLog("INFO:     adding '%s' to requested VMIDs for job '%s'\n",
        tmpVMID,
        J->Name);

      MUHTAdd(&MVMIDHT,tmpVMID,NULL,NULL,NULL);
      }  /* END (MUHTGet(&J->Variables,"VMID",(void **)&VMID,NULL) != SUCCESS) */

    if (J->System == NULL)
      J->System = (msysjob_t *)MUCalloc(1,sizeof(msysjob_t));

    if (J->System == NULL)
      return(FAILURE);

    J->System->JobType = msjtVMTracking;
    J->System->CompletionPolicy = mjcpVMTracking;

    J->VMUsagePolicy = mvmupVMCreate;

    bmset(&J->SpecFlags,mjfSystemJob);
    bmset(&J->Flags,mjfSystemJob);
    bmset(&J->SpecFlags,mjfNoRMStart);
    bmset(&J->Flags,mjfNoRMStart);

#if 0
    /* Removed because of guaranteed start times and reservations.
      A meta job reservation will not consume resources from another reservation, essentially
      doubling the resources that are reserved (the original reservation + the meta reservation) */

    bmset(&J->SpecFlags,mjfMeta);      /* Job reserves but does not consume resources */
    bmset(&J->Flags,mjfMeta);      /* Job reserves but does not consume resources */
#endif

    /* Set walltime for job */

    J->System->WCLimit = J->SpecWCLimit[0];

    /* The rest of the details are taken care of when the job tries to start */
    }  /* END if (bmisset(&SJ->Flags,mjfVMTracking)) */

  if ((SJ->TemplateExtensions != NULL) &&
      (SJ->TemplateExtensions->SetAction != mjtsaNONE))
    {
    if (SJ->TemplateExtensions->SetAction == mjtsaHold)
      {
      char TmpHoldReason[MMAX_LINE];

      snprintf(TmpHoldReason,sizeof(TmpHoldReason),"job held by job template '%s'",
        SJ->Name);

      MJobReject(&J,mhBatch,MSched.DeferTime,mhrPolicyViolation,TmpHoldReason);

      /* Continue processing the entire template, like the QoS's, etc. */

      }
    if (SJ->TemplateExtensions->SetAction == mjtsaCancel)
      {
      char CancelReason[MMAX_LINE];

      snprintf(CancelReason,sizeof(CancelReason),"job was canceled by job template '%s'",
        SJ->Name);

      bmset(&J->IFlags,mjifCancel);

      MMBAdd(&J->MessageBuffer,CancelReason,NULL,mmbtNONE,0,0,NULL);

      return(SUCCESS);
      }
    }

  /* apply template 'set' values here */

  MJobSetAttr(J,mjaGAttr,(void **)SJ->Name,mdfString,mIncr);

  /* apply flags, priority, and partition access list */

  bmor(&J->SpecFlags,&SJ->SpecFlags);
  bmor(&J->Flags,&SJ->SpecFlags);

  if (bmisset(&J->SpecFlags,mjfIgnPolicies) || bmisset(&J->Flags,mjfIgnPolicies))
    {
    bmset(&J->SpecFlags,mjfAdminSetIgnPolicies);
    }

  if (SJ->AName != NULL)
    MUStrDup(&J->AName,SJ->AName);

  if (bmisset(&SJ->IFlags,mjifIsServiceJob))
    bmset(&J->IFlags,mjifIsServiceJob);

  if (bmisset(&SJ->IFlags,mjifIsHidden))
    bmset(&J->IFlags,mjifIsHidden);

  if (bmisset(&SJ->IFlags,mjifIsHWJob))
    bmset(&J->IFlags,mjifIsHWJob);

  if (bmisset(&SJ->IFlags,mjifGlobalRsvAccess))
    bmset(&J->IFlags,mjifGlobalRsvAccess);

  if (bmisset(&SJ->IFlags,mjifSyncJobID))
    {
    /* rename job */

    char *ptr;

    char *APtr;

    APtr = (J->AName != NULL) ? J->AName : SJ->Name;

    if ((!strncmp(J->Name,MSched.Name,strlen(MSched.Name))) &&
        (APtr != NULL) &&
        (ptr = strchr(J->Name,'.')))
      {
      char NewJobID[MMAX_NAME];

      snprintf(NewJobID,sizeof(NewJobID),"%s.%s",
        APtr,
        ptr + 1);

      bmset(&J->IFlags,mjifUseDRMJobID);

      MJobRename(J,NewJobID);
      }
    }  /* END if (bmisset(&SJ->IFlags,mjifSyncJobID)) */

  if (SJ->SystemPrio != 0)
    J->SystemPrio = SJ->SystemPrio;

  if (SJ->UPriority != 0)
    J->UPriority = SJ->UPriority;

  bmor(&J->Hold,&SJ->Hold);

  if (SJ->VMUsagePolicy != mvmupNONE)
    {
    MJobSetAttr(J,mjaVMUsagePolicy,(void **)MVMUsagePolicy[SJ->VMUsagePolicy],mdfString,mSet);
    }

  if (!bmisclear(&SJ->SpecPAL))
    {
    char     ParLine[MMAX_LINE];

    bmcopy(&J->SpecPAL,&SJ->SpecPAL);

    MDB(3,fCONFIG) MLog("INFO:     job %s SpecPAL set to %s in set template\n",
      J->Name,
      MPALToString(&J->SpecPAL,NULL,ParLine));

    MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
    }

  if (SJ->Req[0] != NULL)
    {
    SRQ = SJ->Req[0];
   
    /* apply req attributes */

    /* taskcount, nodeaccesspolicy, reqOS, nodeset */
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      /* loop through all reqs */
      /* for each compute req if MReqIsGlobalOnly == TRUE skip it */
      /* apply NAccessPolicy to req */

      if (J->Req[rqindex] == NULL)
        break;
  
      RQ = J->Req[rqindex];

      /* Skip it if it's a GRES Req */
      if (MReqIsGlobalOnly(RQ))
        continue;
  
      if (SRQ->NAccessPolicy != mnacNONE)
        {
        RQ->NAccessPolicy = SRQ->NAccessPolicy;
        
        MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from template\n",
          J->Name,
          MNAccessPolicy[RQ->NAccessPolicy]);
        }
      }

    RQ = J->Req[0];

    if (SRQ->TaskCount > 0)
      MReqSetAttr(J,RQ,mrqaTCReqMin,(void **)&SRQ->TaskCount,mdfInt,mSet);

    if (SRQ->TasksPerNode > 0)
      MReqSetAttr(J,RQ,mrqaTPN,(void **)&SRQ->TasksPerNode,mdfInt,mSet);

    if (SRQ->Opsys != 0)
      MReqSetAttr(J,RQ,mrqaReqOpsys,(void **)&SRQ->Opsys,mdfInt,mSet);

    if (J->VMUsagePolicy != mvmupNONE)
      {
      /* should trigger not on VMUsagePolicy but anytime dynamic provisioning 
         is enabled (NYI) */

      MReqSetImage(J,RQ,RQ->Opsys);
      }

    if (SRQ->DRes.Procs > 0) 
      RQ->DRes.Procs = SRQ->DRes.Procs;

    if (SRQ->NAllocPolicy != mnalNONE)
      {
      RQ->NAllocPolicy = SRQ->NAllocPolicy;
      }

    if (SRQ->Arch > 0)
      {
      RQ->Arch = SRQ->Arch;
      }

    if (SRQ->ReqNR[mrSwap] > 0)
      {
      RQ->ReqNR[mrSwap] = SRQ->ReqNR[mrSwap];
      }

    if (SRQ->ReqNR[mrDisk] > 0)
      {
      RQ->ReqNR[mrDisk] = SRQ->ReqNR[mrDisk];
      }

    if (SRQ->SetSelection != mrssNONE)
      {
      int index;

      RQ->SetSelection = SRQ->SetSelection;
      bmset(&J->IFlags,mjifSpecNodeSetPolicy);

      RQ->SetType = SRQ->SetType;

      if ((SRQ->SetList != NULL) && (SRQ->SetList[0] != NULL))
        {
        if (RQ->SetList == NULL)
          {
          RQ->SetList = (char **)MUCalloc(1,sizeof(char *) * MMAX_ATTR);
          }

        for (index = 0;SRQ->SetList[index] != NULL;index++)
          {
          MUStrDup(&RQ->SetList[index],SRQ->SetList[index]);
          }
        }
      }

     if (bmisset(&J->Flags,mjfGResOnly))
      {
      MJobRemoveNonGResReqs(J,EMsg);
      }
    }  /* END if (SJ->Req[0] != NULL) */
    
  if (SJ->Request.NC > 0) 
    {
    /* NOTE: really only supports single-req jobs */

    J->Request.TC = SJ->Request.NC;
    J->Request.NC = SJ->Request.NC;

    RQ = J->Req[0];

    RQ->TaskCount = J->Request.TC;
    RQ->NodeCount = J->Request.NC;

    RQ->TaskRequestList[0] = J->Request.TC;
    RQ->TaskRequestList[1] = J->Request.NC;
    }

  /* apply creds */

  if (SJ->Credential.U != NULL)
    J->Credential.U = SJ->Credential.U;

  if (SJ->Credential.Q != NULL)
    MJobSetQOS(J,SJ->Credential.Q,0);

  if (SJ->QOSRequested != NULL)
    {
    J->Credential.Q = SJ->QOSRequested;

    J->QOSRequested = SJ->QOSRequested;

    /* Set mjifQOSSetByTemplate to prevent others from overwritting it. */

    bmset(&J->IFlags,mjifQOSSetByTemplate);
    }

  if (bmisset(&SJ->IFlags,mjifUseLocalGroup))
    {
    char tmpGName[MMAX_NAME];

    MUGNameFromUName(J->Credential.U->Name,tmpGName);

    MGroupAdd(tmpGName,&J->Credential.G);
    }
  else if (SJ->Credential.G != NULL)
    {
    J->Credential.G = SJ->Credential.G;
    }

  if (SJ->Credential.A != NULL)
    {
    J->Credential.A = SJ->Credential.A;

    /* Save in RM.  Job was probably submitted to RM and now changing, so RM must be notified. */

    if (MRMJobModify(J,"Account_Name",NULL,J->Credential.A->Name,mSet,NULL,NULL,NULL) == FAILURE)
      MDB(5,fSTRUCT) MLog("ERROR:     job %s account '%s' cannot be modified in RM\n",J->Name,J->Credential.A->Name);
    }

  if (SJ->Credential.C != NULL)
    {
    char tmpMsg[MMAX_LINE];

    if (MJobSetClass(
          J,
          SJ->Credential.C,
          TRUE,
          tmpMsg) == FAILURE)
      {
      MDB(2,fRM) MLog("WARNING:  cannot set job class to %s via template %s - %s\n",
        SJ->Credential.C->Name,
        SJ->Name,
        tmpMsg);

      MMBAdd(&J->MessageBuffer,tmpMsg,NULL,mmbtNONE,0,0,NULL);
      }
    }    /* END if (SJ->Cred.C != NULL) */

  if (SJ->EUser != NULL)
    {
    /* NOTE:  template EUser becomes job owner in actual job */

    mgcred_t *EC;  /* effective credential */

    char      GName[MMAX_NAME];

    if (MUserAdd(SJ->EUser,&EC) == FAILURE)
      {
      MDB(2,fRM) MLog("WARNING:  cannot set effective job user to %s via template %s\n",
        SJ->EUser,
        SJ->Name);

      MMBAdd(&J->MessageBuffer,"cannot set effective job user via template",NULL,mmbtNONE,0,0,NULL);
      }
    else
      {
      J->Credential.U = EC;

      bmset(&J->IFlags,mjifAllowProxy);
      }

    if ((MUGNameFromUName(SJ->EUser,GName) == FAILURE) || 
        (MGroupAdd(GName,&EC) == FAILURE))
      {
      MDB(2,fRM) MLog("WARNING:  cannot set matching group for effective user %s via template %s\n",
        SJ->EUser,
        SJ->Name);

      MMBAdd(&J->MessageBuffer,"cannot set matching group for effective user via template",NULL,mmbtNONE,0,0,NULL);
      }
    else
      {
      J->Credential.G = EC;
      }

    if (J->Env.IWD != NULL)
      {
      /* NOTE:  If IWD not specified, RM will use user home directory */

      /* for now, this is the desired behavior - re-evaluate??? */

      MUFree(&J->Env.IWD);
      }
    }    /* END if (SJ->EUser != NULL) */

  /* apply resources (walltime, gres, mem, swap, disk, features, ...) */

  if (SJ->SpecWCLimit[0] != 0)
    {
    MJobSetAttr(J,mjaReqAWDuration,(void **)&SJ->SpecWCLimit[0],mdfLong,mSet);

    WallTimeChanged = TRUE;
    }

  if (WallTimeChanged == TRUE)
    {
    /* make sure the job knows it now has a walltime now */
     
    bmunset(&J->IFlags,mjifWCNotSpecified);

    /* Walltime was changed, make sure that the original is not passed to the RM.
       Otherwise the walltime will just be changed back when Moab updates from 
       the RM. */

    /* if the walltime was changed and we're using SGE, make sure the
     * job.modify.sge.pl gets called so SGE gets the changes */ 

    if ((J->DestinationRM != NULL) &&
        (J->DestinationRM->Type == mrmtNative) &&
        (J->DestinationRM->SubType == mrmstSGE))
      {
      char tBuf[MMAX_LINE];

      snprintf(tBuf,MMAX_LINE,"%lu",J->SpecWCLimit[0]);

      MRMJobModify(
        J,
        "wclimit",
        "specwclimit",
        tBuf, /* need to get the wclimit as a char* here..." */
        mSet,
        NULL,
        NULL,
        NULL);
      }

    /* NOTE: needs to be implemented for each RM that takes in original args */

    if (J->RMSubmitType == mrmtPBS)
      {
      MJobPBSExtractArg(
        "-l",
        "walltime",
        J->RMSubmitString,
        NULL,
        0,
        TRUE);
      }
    }

  if (SJ->CPULimit > 0) 
    J->CPULimit = SJ->CPULimit;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (SJ->Req[rqindex] == NULL)
      break;

    SRQ = SJ->Req[rqindex];

    if (J->Req[rqindex] == NULL)
      {
      if (MReqCreate(J,NULL,&RQ,TRUE) == FAILURE)
        {
        continue;
        }
      }
    else
      {
      RQ = J->Req[rqindex];
      }

    if (!MSNLIsEmpty(&SRQ->DRes.GenericRes))
      {
      int gindex;

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        if (MSNLGetIndexCount(&SRQ->DRes.GenericRes,gindex) > 0)
          {
          MJobAddRequiredGRes(
            J,
            MGRes.Name[gindex],
            MSNLGetIndexCount(&SRQ->DRes.GenericRes,gindex),
            mxaGRes,
            FALSE,
            FALSE);
          }
        }    /* END for (gindex) */
      }      /* END if (!MSNLIsEmpty(&SRQ->DRes.GenericRes)) */
  
    if (SRQ->DRes.Mem != 0)
      {
      char tBuf[MMAX_LINE];
      char dstBuf[MMAX_BUFFER];

      dstBuf[0] = '\0';

      RQ->DRes.Mem = SRQ->DRes.Mem; 

      /* need to make sure any provided mem args are removed */

      if (J->RMSubmitType == mrmtPBS)
        {
        /* Extract any prior "-l mem=xxx" */
        MJobPBSExtractArg("-l","mem",J->RMSubmitString,NULL,0,TRUE);

        /* Insert new "-l mem=xxx" */
        sprintf(tBuf,"mem=%dmb",RQ->DRes.Mem);
        MJobPBSInsertArg("-l",tBuf,J->RMSubmitString,dstBuf,sizeof(dstBuf));
        MUStrDup(&J->RMSubmitString,dstBuf);
        }

      /* if NATIVE:SGE we need to push the change down... */

      if ((J->DestinationRM != NULL) &&
          (J->DestinationRM->Type == mrmtNative) &&
          (J->DestinationRM->SubType == mrmstSGE))
        {
        char tBuf[MMAX_LINE];

        snprintf(tBuf,sizeof(tBuf),"%d",RQ->DRes.Mem);

        MRMJobModify(
          J,
          "mem",
          "mem",
          tBuf,
          mSet,
          NULL,
          NULL,
          NULL);
        }

      /* NOTE: the above change may need to happen at some point for Swap
      * and Disk... */

      } /* END if (SRQ->DRes.Mem != 0)... */

    if (SRQ->DRes.Swap != 0)
      RQ->DRes.Swap = SRQ->DRes.Swap;

    if (SRQ->DRes.Disk != 0)
      RQ->DRes.Disk = SRQ->DRes.Disk;

    if (!bmisclear(&SRQ->ReqFBM))
      {
      if (SRQ->ReqFMode == mclOR)
        {
        MReqSetAttr(J,RQ,mrqaReqNodeFeature,(void **)&SRQ->ReqFBM,mdfOther,mAdd);
        }
      else
        {
        MReqSetAttr(J,RQ,mrqaReqNodeFeature,(void **)&SRQ->ReqFBM,mdfOther,mSet);
        }

      if (bmisset(&J->Flags,mjfGResOnly))
        {
        /* set the features on all the remaining reqs because some reqs may/will be eliminated 
         * later and we must preserve the feature for any/all remaining reqs  MOAB-3815 */
        int localrqindex = rqindex + 1;
        for (;localrqindex < MMAX_REQ_PER_JOB && J->Req[localrqindex] != NULL;localrqindex++)
          {
          if (SRQ->ReqFMode == mclOR)
            {
            MReqSetAttr(J,J->Req[localrqindex],mrqaReqNodeFeature,(void **)&SRQ->ReqFBM,mdfOther,mAdd);
            }
          else
            {
            MReqSetAttr(J,J->Req[localrqindex],mrqaReqNodeFeature,(void **)&SRQ->ReqFBM,mdfOther,mSet);
            }
          }
        }
      }

    if (SRQ->Pref != NULL)
      {
      if (!bmisclear(&SRQ->Pref->FeatureBM))
        {
        MSched.ResourcePrefIsActive = TRUE;

        if (RQ->Pref == NULL)
          {
          if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
            {
            return(FAILURE);
            }
          }

        bmcopy(&RQ->Pref->FeatureBM,&SRQ->Pref->FeatureBM);
        }

      if (SRQ->Pref->Variables != NULL)
        {
        MSched.ResourcePrefIsActive = TRUE;

        if (RQ->Pref == NULL)
          {
          if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
            {
            return(FAILURE);
            }
          }

        MULLDup(&RQ->Pref->Variables,SRQ->Pref->Variables);
        }
      }    /* END if (SRQ->Pref != NULL) */
    }      /* END for (rqindex) */

  /* apply environment attributes */

  if (SJ->Env.Cmd != NULL)
    {
    MUStrDup(&J->Env.Cmd,SJ->Env.Cmd);

    if ((J->SubmitRM != NULL) && (J->SubmitRM == MSched.InternalRM))
      {
      /* job locally submitted to internal RM - remove any non-directive
         values and apply SJ->E.CMD */

      if (bmisset(&SJ->IFlags,mjifStageInExec)) /* Stagein executable */
        bmset(&J->IFlags,mjifStageInExec);

      if (J->RMSubmitString == NULL)
        {
        MUStrDup(&J->RMSubmitString,SJ->Env.Cmd);
        }
      else if (!bmisset(&J->IFlags,mjifExecTemplateUsed))
        {
        /* Overwrite anything before */
        MUFree(&J->RMSubmitString);
        MUStrDup(&J->RMSubmitString,SJ->Env.Cmd);

        bmset(&J->IFlags,mjifExecTemplateUsed);
        }
      }
    }  /* END if (SJ->E.Cmd = NULL) */

  if (SJ->RMSubmitString != NULL)
    {
    MUStrDup(&J->RMSubmitString,SJ->RMSubmitString);
    }  /* END if (SJ->RMSubmitString != NULL) */

  if (SJ->Env.IWD != NULL)
    {
    if (!strcasecmp(SJ->Env.IWD,"$HOME"))
      {
      if ((J->Credential.U != NULL) && (J->Credential.U->HomeDir != NULL))
        {
        MUStrDup(&J->Env.IWD,J->Credential.U->HomeDir);
        }
      }
    else
      {
      MUStrDup(&J->Env.IWD,SJ->Env.IWD);
      }
    }    /* END if (SJ->E.IWD != NULL) */

  if (SJ->Env.Args != NULL)
    {
    MUStrDup(&J->Env.Args,SJ->Env.Args);
    }

  /* apply jobgroups, variables, and triggers */

  if (SJ->JGroup != NULL)
    {
    MJGroupInit(J,SJ->JGroup->Name,-1,SJ->JGroup->RealJob);
    }

  if (SJ->Variables.Table != NULL)
    {
    MUHTCopy(&J->Variables,&SJ->Variables);
    }

  if (SJ->ReqRID != NULL)
    MUStrDup(&J->ReqRID,SJ->ReqRID);

  if (SJ->Triggers != NULL)
    {
    MTrigApplyTemplate(&J->Triggers,SJ->Triggers,(void *)J,mxoJob,J->Name);

    if (bmisset(&J->IFlags,mjifGenericSysJob))
      {
      /* put flag on trigger so that we know that it is part of a 
         generic system job so that it doesn't get blown away at restart */

      mtrig_t *tmpT;

      tmpT = (mtrig_t *)MUArrayListGetPtr(J->Triggers,0);

      if (tmpT != NULL)
        {
        bmset(&tmpT->SFlags,mtfGenericSysJob);
        }
      }       /* END if (bmisset(&SJ->IFlags,mjifGenericSysJob)) */
    }      /* END if (DefJ->T != NULL) */

  if (SJ->TemplateExtensions != NULL)
    {
    if (J->TemplateExtensions == NULL)
      {
      MJobAllocTX(J);
      }

    if (SJ->TemplateExtensions->WorkloadRMID != NULL)
      {
      MUStrDup(&J->TemplateExtensions->WorkloadRMID,SJ->TemplateExtensions->WorkloadRMID);

      J->TemplateExtensions->WorkloadRM = SJ->TemplateExtensions->WorkloadRM;
      }

    if (SJ->TemplateExtensions->SpecTrig != NULL)
      {
      int tindex;

      for (tindex = 0;tindex < MMAX_SPECTRIG;tindex++)
        {
        if (SJ->TemplateExtensions->SpecTrig[tindex] == NULL)
          break;

        MUStrDup(&J->TemplateExtensions->SpecTrig[tindex],SJ->TemplateExtensions->SpecTrig[tindex]);
        }
      }
    }    /* END if (SJ->TX != NULL) */

  if (__MJobTemplateApplyWorkflow(J,SJ) == FAILURE)
    {
    return(FAILURE);
    }

  MULLAdd(&J->TSets,SJ->Name,(void *)SJ,NULL,NULL);

  if (SJ->Description != NULL)
    MUStrDup(&J->Description,SJ->Description); 

  if (SJ->NodePriority != NULL)
    {
    if ((J->NodePriority != NULL) ||
        (MNPrioAlloc(&J->NodePriority) == SUCCESS))
      {
      memcpy(J->NodePriority,SJ->NodePriority,sizeof(J->NodePriority[0]));
      }
    }

  return(SUCCESS);
  }  /* END MJobApplySetTemplate() */





/**
 * Apply workflow to job
 *
 * @param J    (I) [modified] - Job to have template applied to it
 * @param SJ   (I) - The job template to apply
 */

int __MJobTemplateApplyWorkflow(

  mjob_t *J,    /* I (modified) */
  mjob_t *SJ)   /* I */

  {
  mln_t  *tmpL;

  mbool_t IsFirstJob = FALSE;
  mbool_t CreateVC = FALSE;

  mjob_t *TJob;
  mjob_t *tmpJ;
  mvc_t  *VCGroup;

  char    JName[MMAX_NAME];

  enum MDependEnum DependType;

  mbool_t          IsBefore;

  tmpL = SJ->TemplateExtensions->Depends;

  const char *FName = "__MJobTemplateApplyWorkflow";

  MDB(4,fRM) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (SJ != NULL) ? SJ->Name : "NULL");
  
  if ((SJ->TemplateExtensions != NULL) && 
      (SJ->TemplateExtensions->Depends != NULL) && 
      !MJOBISCOMPLETE(J) &&
      !bmisset(&J->IFlags,mjifWasLoadedFromCP))
    {
    /* NOTE:  only create workflow when job is first discovered */

    VCGroup = NULL;

    if (J->JGroup == NULL)
      {
      char JGroupName[MMAX_NAME];

      /* NOTE: all jobs in a template workflow get a JGROUP of the first job */

      IsFirstJob = TRUE;

      if (SJ->TemplateExtensions->FailurePolicy != mjtfpNONE)
        {
        mjob_t *NewJ = NULL;

        mbitmap_t tmpFlags;

        bmclear(&tmpFlags);

        /* create "watcher" system job */

        bmset(&tmpFlags,mjfSystemJob);
        bmset(&tmpFlags,mjfNoResources);
        bmset(&tmpFlags,mjfNoRMStart);

        MUSubmitSysJob(
          &NewJ,
          msjtNONE,
          NULL,
          1,
          J->Credential.U->Name,
          NULL,
          &tmpFlags,
          ((MSched.DefaultJ != NULL) && (MSched.DefaultJ->SpecWCLimit[0] > 0)) ? MSched.DefaultJ->SpecWCLimit[0] : MDEF_SYSDEFJOBWCLIMIT,
          NULL);

        MUStrCpy(JGroupName,NewJ->Name,sizeof(JGroupName));

        MJGroupInit(NewJ,JGroupName,-1,NewJ);

        MJobAllocTX(NewJ);

        /* ccode variable must be set on all jobs */

        MJobSetAttr(J,mjaVariables,(void **)"ccode",mdfString,mAdd);

        NewJ->TemplateExtensions->FailurePolicy = SJ->TemplateExtensions->FailurePolicy;
        }  /* END if (SJ->TX->FailurePolicy != mjtfpNONE) */
      else
        {
        MUStrCpy(JGroupName,J->Name,sizeof(JGroupName));
        }
      
      MJGroupInit(J,JGroupName,-1,J);
      }    /* END if (J->JGroup == NULL) */

    if (J->ParentVCs != NULL)
      {
      /* Search for an existing workflow VC */

      mln_t *tmpVCL;
      mvc_t *tmpVC;

      for (tmpVCL = J->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
        {
        tmpVC = (mvc_t *)tmpVCL->Ptr;

        if (bmisset(&tmpVC->Flags,mvcfWorkflow))
          {
          VCGroup = tmpVC;

          break;
          }
        }

      if (VCGroup == NULL)
        {
        CreateVC = TRUE;
        }
      }  /* END if (J->ParentVCs != NULL) */
    else
      {
      CreateVC = TRUE;
      }

    if (CreateVC == TRUE)
      {
      /* Create a workflow VC */

      char tmpOwner[MMAX_LINE];

      if (MVCAllocate(&VCGroup,J->Name,NULL) == FAILURE)
        {
        MDB(3,fSTRUCT) MLog("ERROR:    failed to generate workflow VC for job '%s'\n",
          J->Name);
        }

      /* Workflow VCs are owned by the user that submitted the job */

      if (J->Credential.U != NULL)
        {
        snprintf(tmpOwner,sizeof(tmpOwner),"USER:%s",
          J->Credential.U->Name);

        /* If job user exists, use that as the creator */
        MVCSetAttr(VCGroup,mvcaCreator,(void **)J->Credential.U->Name,mdfString,mSet);
        }
      else
        {
        /* User could not be found, use admin */

        snprintf(tmpOwner,sizeof(tmpOwner),"USER:%s",
          MSched.Admin[1].UName[0]);
        }

      MVCSetAttr(VCGroup,mvcaCreateJob,(void **)J->Name,mdfString,mSet);

      MVCSetAttr(VCGroup,mvcaOwner,(void **)tmpOwner,mdfString,mSet);

      MVCSetAttr(VCGroup,mvcaFlags,(void **)MVCFlags[mvcfWorkflow],mdfString,mSet);
      MVCSetAttr(VCGroup,mvcaFlags,(void **)MVCFlags[mvcfDestroyWhenEmpty],mdfString,mAdd);
      MVCSetAttr(VCGroup,mvcaFlags,(void **)MVCFlags[mvcfDestroyObjects],mdfString,mAdd);

      MUHTCopy(&VCGroup->Variables,&J->Variables);

      MVCAddObject(VCGroup,(void *)J,mxoJob);
      }  /* END if (CreateVC == TRUE) */

    if (VCGroup == NULL)
      return(FAILURE);

    MUHTCopy(&VCGroup->Variables,&J->Variables);

    while (tmpL != NULL)
      {
      if (MTJobFind(tmpL->Name,&TJob) == SUCCESS)
        {
        JName[0] = '\0';
        tmpJ     = NULL;

        if (bmisset(&tmpL->BM,mdJobSuccessfulCompletion))
          {
          IsBefore = FALSE;
          DependType = mdJobSuccessfulCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobCompletion))
          {
          IsBefore = FALSE;
          DependType = mdJobCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobFailedCompletion))
          {
          IsBefore = FALSE;
          DependType = mdJobFailedCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobBeforeSuccessful))
          {
          IsBefore = TRUE;
          DependType = mdJobSuccessfulCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobBefore))
          {
          IsBefore = TRUE;
          DependType = mdJobCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobBeforeFailed))
          {
          IsBefore = TRUE;
          DependType = mdJobFailedCompletion;
          }
        else if (bmisset(&tmpL->BM,mdJobSyncMaster))
          {
          IsBefore = FALSE;
          DependType = mdJobSyncMaster;
          }
        else
          {
          IsBefore = FALSE;
          DependType = mdJobSuccessfulCompletion;
          }

        snprintf(JName,sizeof(JName),"%s.%s",
          J->JGroup->Name,
          TJob->Name);

        if (MJobFind(JName,&tmpJ,mjsmJobName) == SUCCESS)
          {
          /* job already exists - link as dependency */

          if (IsBefore == TRUE)
            {
            MJobSetDependency(tmpJ,DependType,J->Name,NULL,0,FALSE,NULL);
            }
          else if (DependType == mdJobSyncMaster)
            {
            if (MDependFind(J->Depend,mdJobSyncSlave,tmpJ->Name,NULL) == FAILURE)
              MJobSetSyncDependency(J,tmpJ);
            }
          else
            {
            MJobSetDependency(J,DependType,tmpJ->Name,NULL,0,FALSE,NULL);
            }
          }
        else
          {
          /* create/submit job to create workflow */

          if (bmisset(&TJob->IFlags,mjifInheritResources))
            {
            /* Child job should be exact copy of the parent so that it has
                all of the same resource requirements, then apply the
                template */

            mrm_t *tmpRM;
            int    tmpRQIndex;
            mreq_t *tmpRQ;
            mbool_t CreateError = FALSE;

            tmpJ = NULL;

            /* MJobCreate() will add JName to the hash but MJobDuplicate will copy J->Name
               so when we call MJobSetAttr(mjaGJID) it will think it needs to rename the job
               and remove the old name from the HT, this causes the original job's name to 
               be removed from the HT.  This is corrected by calling MUStrCpy() with the
               new name so that the old name doesn't get removed */

            if ((MJobCreate(JName,TRUE,&tmpJ,NULL) == FAILURE) ||
                (MJobDuplicate(tmpJ,J,FALSE) == FAILURE) ||
                (MUStrCpy(tmpJ->Name,JName,sizeof(tmpJ->Name)) == FAILURE) ||
                (MJobSetAttr(tmpJ,mjaSID,(void **)MSched.Name,mdfString,mSet) == FAILURE) ||
                (MJobSetAttr(tmpJ,mjaGJID,(void **)JName,mdfString,mSet) == FAILURE) ||
                (MJobSetAttr(tmpJ,mjaSRMJID,(void **)JName,mdfString,mSet) == FAILURE))
              {
              MDB(3,fSTRUCT) MLog("ERROR:    failed to create template job '%s'\n",
                JName);

              CreateError = TRUE;
              }

            if (CreateError == FALSE)
              {
              /* Change the name back */
              /* This must be done now so that any triggers will point
                  to the correct job name */

              MUStrCpy(tmpJ->Name,JName,sizeof(tmpJ->Name));

              /* Add to job group */
              /* This must be done before we apply the template so that lower
                  dependencies will also get the job group */

              MJGroupInit(tmpJ,J->JGroup->Name,-1,J->JGroup->RealJob);
              }

            if ((CreateError == FALSE) &&
                ((MRMAddInternal(&tmpRM) == FAILURE) ||
                 (MJobRemoveCP(tmpJ) == FAILURE)))
              {
              CreateError = TRUE;
              }

            if (CreateError == FALSE)
              {
              /* Add to the local RM */

              MS3AddLocalJob(tmpRM,tmpJ->Name);

              for (tmpRQIndex = 0;tmpRQIndex < MMAX_REQ_PER_JOB;tmpRQIndex++)
                {
                tmpRQ = tmpJ->Req[tmpRQIndex];

                if (tmpRQ == NULL)
                  break;

                tmpRQ->RMIndex = tmpRM->Index;
                }

              tmpRM->LastSubmissionTime = MSched.Time;
              tmpJ->SubmitTime = MSched.Time;

              /* Add to VCGroup */
              if (VCGroup != NULL)
                MVCAddObject(VCGroup,(void *)tmpJ,mxoJob);
              }

            if (CreateError == FALSE)
              {
              /* Meta and VMTracking flags should not be inherited */
  
              bmunset(&tmpJ->Flags,mjfMeta);
              bmunset(&tmpJ->SpecFlags,mjfMeta);
  
              bmunset(&tmpJ->Flags,mjfVMTracking);
              bmunset(&tmpJ->SpecFlags,mjfVMTracking);
              }

            if ((CreateError == FALSE) &&
                 ((MJobApplySetTemplate(tmpJ,TJob,NULL) == FAILURE) ||
                  (MJobSetState(tmpJ,mjsIdle) == FAILURE)))
              {
              CreateError = TRUE;
              }

            if (CreateError == TRUE)
              {
              if (tmpJ != NULL)
                {
                MJobProcessRemoved(&tmpJ);
                }

              return(FAILURE);
              }

            bmset(&tmpJ->Flags,mjfTemplatesApplied);

            /* Also need to make it so that the second job will run
                where the first one ran */

            if (IsBefore == FALSE)
              {
              MULLAdd(&tmpJ->ImmediateDepend,J->Name,(void *)strdup(MDependType[DependType]),NULL,MUFREE);
              }
            else
              {
              MULLAdd(&J->ImmediateDepend,tmpJ->Name,(void *)strdup(MDependType[DependType]),NULL,MUFREE);
              }

            /* Add new job into the cache */

            MJobTransition(tmpJ,FALSE,FALSE);
            } /* END if (bmisset(&TJob->IFlags,mjifInheritResources)) */
          else if ((MJobCreateTemplateDependency(J,TJob,JName) == FAILURE) ||
              (MJobFind(JName,&tmpJ,mjsmBasic) == FAILURE))
            {
            return(FAILURE);
            }

          bmset(&tmpJ->IFlags,mjifCreatedByTemplateDepend);

          MJobBuildCL(tmpJ);

          /* tmpJ points to newly created job */

          if (IsBefore == TRUE)
            {
            /* newly created job depends on submitted job */

            MJobSetDependency(tmpJ,DependType,J->Name,NULL,0,FALSE,NULL);
            }
          else if (DependType == mdJobSyncMaster)
            {
            MJobSetSyncDependency(J,tmpJ);
            }
          else
            {
            /* submitted job depends on newly created job */

            MJobSetDependency(J,DependType,JName,NULL,0,FALSE,NULL);
            }
          }

        if (VCGroup != NULL)
          {
          if (tmpJ->ParentVCs == NULL)
            MVCAddObject(VCGroup,(void *)tmpJ,mxoJob);
          }
        else
          {
          /* VC should never be NULL here */

          MDB(4,fSCHED) MLog("ERROR:    could not find VC for job '%s'\n",
            tmpJ->Name);
          }

        MJobWriteCPToFile(tmpJ);
        }    /* END if (MTJobFind(tmpL->Name,&TJob) == SUCCESS) */

      tmpL = tmpL->Next;
      }  /* END while (tmpL != NULL) */

    /* create system job to watch over workflow */

    if (IsFirstJob == TRUE)
      MJobWorkflowCreateTriggers(J,NULL);

    MJobWriteCPToFile(J);
    }    /* END if ((SJ->TX != NULL) && (SJ->TX->Depends != NULL)) */

  return(SUCCESS);
  }  /* END __MJobTemplateApplyWorkflow */





/**
 * Force/apply 'unset' job template attributes to specified job.
 *
 * NOTE:  For typical job discovery, called by MRMJobPostLoad()->MJobSetDefaults()
 * NOTE:  Sync w/MJobApplyDefTemplate() - peer
 *
 * @see MJobGetTemplate() - locate template which matches specified min/max criteria
 * @see MJobApplyDefTemplate() - apply specified attributes as defaults onto target job
 *
 * @param J (I) [modified]
 * @param UJ (I)
 */

int MJobApplyUnsetTemplate(

  mjob_t *J,  /* I (modified) */
  mjob_t *UJ) /* I */

  {
  const char *FName = "MJobApplyUnsetTemplate";

  MDB(4,fRM) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (UJ != NULL) ? UJ->Name : "NULL");

  if ((J == NULL) || (UJ == NULL))
    {
    return(FAILURE);
    }

  if (!bmisclear(&UJ->SpecFlags))
    {
    mbitmap_t tmpFlags;

    bmcopy(&tmpFlags,&UJ->SpecFlags);

    bmnot(&tmpFlags,mjfLAST);

    bmand(&J->SpecFlags,&tmpFlags);
    bmand(&J->Flags,&tmpFlags);
    }

  /* NYI */

  return(SUCCESS);
  }  /* END MJobApplyUnsetTemplate() */






/**
 * Apply default job template attributes to specified job if attributes not set within job.
 *
 * NOTE:  Sync w/MJobApplySetTemplate()
 
 * TODO: Have MJobApply[Set|Def]Template call one call that does the work or both commands.
 * Can AppleyDefTemplate be assured that no changes have been made to the job previous - easier for setting?
 *
 * @see MJobGetTemplate() - locate template which matches specified min/max criteria
 * @see MJobApplySetTemplate() - force specified attributes into target job
 *
 * Supported Attributes:
 * Required Features (RFEATURES)
 * Partition Access List (PARTITIONMASK)
 *
 * @param J (I) [modified]
 * @param DJ (I)
 */

int MJobApplyDefTemplate(

  mjob_t *J,  /* I (modified) */
  mjob_t *DJ) /* I */

  {
  mreq_t *RQ;
  mreq_t *DRQ;
  
  mbool_t WallTimeChanged = FALSE;

  int     rqindex;

  if ((J == NULL) || (DJ == NULL))
    {
    return(FAILURE);
    }

  /* apply template 'default' values here */

  if (strcasecmp(DJ->Name,"default"))
    MJobSetAttr(J,mjaGAttr,(void **)DJ->Name,mdfString,mIncr);

  bmor(&J->SpecFlags,&DJ->SpecFlags);
  bmor(&J->Flags,&DJ->SpecFlags);

  if ((DJ->AName != NULL) && (J->AName == NULL))
    MUStrDup(&J->AName,DJ->AName);

  if (bmisset(&DJ->IFlags,mjifSyncJobID))
    {
    /* rename job */

    char *ptr;
    char *APtr;

    APtr = (J->AName != NULL) ? J->AName : DJ->Name;

    if ((!strncmp(J->Name,MSched.Name,strlen(MSched.Name))) &&
        (APtr != NULL) &&
        (ptr = strchr(J->Name,'.')))
      {
      char NewJobID[MMAX_NAME];

      snprintf(NewJobID,sizeof(NewJobID),"%s.%s",
        APtr,
        ptr + 1);

      MJobRename(J,NewJobID);
      }
    }

  if ((DJ->UPriority != 0) && (J->UPriority == 0))
    J->UPriority = DJ->UPriority;

  if ((DJ->SystemPrio != 0) && (J->SystemPrio == 0))
    J->SystemPrio = DJ->SystemPrio;

  if (!bmisclear(&DJ->SpecPAL))
    {
    /* bmor(&J->SpecPAL,DJ->SpecPAL);--set default, do not OR */

    MJobSetAttr(J,mjaPAL,(void **)&DJ->SpecPAL,mdfInt,mSet);
    }

  if ((DJ->VMUsagePolicy != mvmupNONE) && (J->VMUsagePolicy == mvmupNONE))
    {
    MJobSetAttr(J,mjaVMUsagePolicy,(void **)MVMUsagePolicy[DJ->VMUsagePolicy],mdfString,mSet);
    }

  RQ = J->Req[0];
  DRQ = DJ->Req[0];

  if ((RQ != NULL) && (DRQ != NULL))
    {
    if (DRQ->XURes != NULL)
      {
      if (RQ->XURes == NULL)
        {
        MReqInitXLoad(RQ);
        }
      }

    if (DRQ->Pref != NULL)
      {
      if ((!bmisclear(&DRQ->Pref->FeatureBM)) && 
         ((RQ->Pref == NULL) || (bmisclear(&RQ->Pref->FeatureBM))))
        {
        if (RQ->Pref == NULL)
          {
          if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
            {
            return(FAILURE);
            }
          }

        bmcopy(&RQ->Pref->FeatureBM,&DRQ->Pref->FeatureBM);
        }

      if ((DRQ->Pref->Variables != NULL) && 
         ((RQ->Pref == NULL) || (RQ->Pref->Variables == NULL)))
        {
        if (RQ->Pref == NULL)
          {
          if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
            {
            return(FAILURE);
            }
          }  
                                                                           
        MULLDup(&RQ->Pref->Variables,DRQ->Pref->Variables);
        }
      }

    if ((!bmisclear(&DRQ->ReqFBM)) && 
        (bmisclear(&RQ->ReqFBM)))
      {
      bmcopy(&RQ->ReqFBM,&DRQ->ReqFBM);
      }

    if ((DRQ->NAccessPolicy != mnacNONE) && (RQ->NAccessPolicy == mnacNONE))
      {
      RQ->NAccessPolicy = DRQ->NAccessPolicy;

      MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from def template\n",
        J->Name,
        MNAccessPolicy[RQ->NAccessPolicy]);
      }

    /* reqOS */

    if ((DRQ->Opsys != 0) && (RQ->Opsys == 0))
      MReqSetAttr(J,J->Req[0],mrqaReqOpsys,(void **)&DRQ->Opsys,mdfInt,mSet);

    if (J->VMUsagePolicy != mvmupNONE)
      {
      /* should trigger not on VMUsagePolicy but anytime dynamic provisioning
         is enabled (NYI) */

      MReqSetImage(J,RQ,RQ->Opsys);
      }

    if ((DRQ->TaskCount > 0) && (RQ->TaskCount == 0))
      MReqSetAttr(J,RQ,mrqaTCReqMin,(void **)&DRQ->TaskCount,mdfInt,mSet);
    
    if ((DRQ->TasksPerNode > 0) && (RQ->TasksPerNode == 0))
      MReqSetAttr(J,RQ,mrqaTPN,(void **)&DRQ->TasksPerNode,mdfInt,mSet);

    if ((DRQ->DRes.Procs > 0) && (RQ->DRes.Procs == 0))
      RQ->DRes.Procs = DRQ->DRes.Procs;

    if (DRQ->NAllocPolicy != mnalNONE)
      {
      RQ->NAllocPolicy= DRQ->NAllocPolicy;
      }
    }
  
  if ((DJ->Request.NC > 0) &&
      (J->Request.NC == 0) &&
      (!(bmisset(&J->IFlags,mjifNodesSpecified))))
    {
    J->Request.NC = DJ->Request.NC;
    }

  /* apply creds */

  if ((DJ->Credential.U != NULL) && (J->Credential.U == NULL))
    J->Credential.U = DJ->Credential.U;

  if ((DJ->Credential.Q != NULL) && (J->Credential.Q == NULL))
    J->Credential.Q = DJ->Credential.Q;

  if ((DJ->QOSRequested != NULL) && (J->QOSRequested == NULL))
    {
    J->Credential.Q = DJ->QOSRequested;

    J->QOSRequested = DJ->QOSRequested;
    }

  if (bmisset(&DJ->IFlags,mjifUseLocalGroup) &&
      (J->Credential.G == NULL))
    {
    char tmpGName[MMAX_NAME];

    MUGNameFromUName(J->Credential.U->Name,tmpGName);

    MGroupAdd(tmpGName,&J->Credential.G);
    }
  else if ((DJ->Credential.G != NULL) && (J->Credential.G == NULL))
    {
    J->Credential.G = DJ->Credential.G;
    }

  if ((DJ->Credential.A != NULL) && (J->Credential.A == NULL))
    J->Credential.A = DJ->Credential.A;

  if ((DJ->Credential.C != NULL) && (J->Credential.C == NULL))
    {
    char tmpMsg[MMAX_LINE];

    if (MJobSetClass(
          J,
          DJ->Credential.C,
          TRUE,
          tmpMsg) == FAILURE)
      {
      MDB(2,fRM) MLog("WARNING:  cannot set job class to %s via template %s - %s\n",
        DJ->Credential.C->Name,
        DJ->Name,
        tmpMsg);

      MMBAdd(&J->MessageBuffer,tmpMsg,NULL,mmbtNONE,0,0,NULL);
      }
    }    /* END if (DJ->Cred.C != NULL) */

  if ((DJ->EUser != NULL) && (J->EUser == NULL))
    {
    /* NOTE:  template EUser becomes job owner in actual job */

    mgcred_t *EC;  /* effective credential */

    char      GName[MMAX_NAME];

    if (MUserAdd(DJ->EUser,&EC) == FAILURE)
      {
      MDB(2,fRM) MLog("WARNING:  cannot set effective job user to %s via template %s\n",
        DJ->EUser,
        DJ->Name);

      MMBAdd(&J->MessageBuffer,"cannot set effective job user via template",NULL,mmbtNONE,0,0,NULL);
      }
    else
      {
      J->Credential.U = EC;

      bmset(&J->IFlags,mjifAllowProxy);
      }

    if ((MUGNameFromUName(DJ->EUser,GName) == FAILURE) || 
        (MGroupAdd(GName,&EC) == FAILURE))
      {
      MDB(2,fRM) MLog("WARNING:  cannot set matching group for effective user %s via template %s\n",
        DJ->EUser,
        DJ->Name);

      MMBAdd(&J->MessageBuffer,"cannot set matching group for effective user via template",NULL,mmbtNONE,0,0,NULL);
      }
    else
      {
      J->Credential.G = EC;
      }

    if (J->Env.IWD != NULL)
      {
      /* NOTE:  If IWD not specified, RM will use user home directory */

      /* for now, this is the desired behavior - re-evaluate??? */

      MUFree(&J->Env.IWD);
      }
    }    /* END if (DJ->EUser != NULL) */

  /* apply resources (walltime, gres, mem, swap, disk, features, ...) */

  if ((DJ->SpecWCLimit[0] != 0) && (J->SpecWCLimit[0] == 0))
    {
    MJobSetAttr(J,mjaReqAWDuration,(void **)&DJ->SpecWCLimit[0],mdfLong,mSet);

    WallTimeChanged = TRUE;
    }

  if (WallTimeChanged == TRUE)
    {
    /* Walltime was changed, make sure that the original is not passed to the RM.
       Otherwise the walltime will just be changed back when Moab updates from 
       the RM. */

    /* NOTE: needs to be implemented for each RM that takes in original args */

    if (J->RMSubmitType == mrmtPBS)
      {
      MJobPBSExtractArg(
        "-l",
        "walltime",
        J->RMSubmitString,
        NULL,
        0,
        TRUE);
      }
    }

  if ((DJ->CPULimit > 0) && (J->CPULimit == 0))
    J->CPULimit = DJ->CPULimit;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (DJ->Req[rqindex] == NULL)
      break;

    DRQ = DJ->Req[rqindex];

    if (J->Req[rqindex] == NULL)
      {
      if (MReqCreate(J,NULL,&RQ,TRUE) == FAILURE)
        {
        continue;
        }
      }
    else
      {
      RQ = J->Req[rqindex];
      }

    if (!MSNLIsEmpty(&DRQ->DRes.GenericRes))
      {
      int gindex;

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        if ((MSNLGetIndexCount(&DRQ->DRes.GenericRes,gindex) > 0) &&
            (MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) == 0))
          {
          MJobAddRequiredGRes(
            J,
            MGRes.Name[gindex],
            MSNLGetIndexCount(&DRQ->DRes.GenericRes,gindex),
            mxaGRes,
            FALSE,
            FALSE);
          }
        }    /* END for (gindex) */
      }      /* END if (DRQ->DRes.GenericRes.Count[0] > 0) */
  
    if ((DRQ->DRes.Mem != 0) && (RQ->DRes.Mem == 0))
      {
      char tBuf[MMAX_LINE];
      char dstBuf[MMAX_BUFFER];

      dstBuf[0] = '\0';

      RQ->DRes.Mem = DRQ->DRes.Mem; 

      /* Insert new "-l mem=xxx" */
      sprintf(tBuf,"mem=%dmb",RQ->DRes.Mem);
      MJobPBSInsertArg("-l",tBuf,J->RMSubmitString,dstBuf,sizeof(dstBuf));
      MUStrDup(&J->RMSubmitString,dstBuf);
      }

    if ((DRQ->DRes.Swap != 0) && (RQ->DRes.Swap == 0))
      RQ->DRes.Swap = DRQ->DRes.Swap;

    if ((DRQ->DRes.Disk != 0) && (RQ->DRes.Disk == 0))
      RQ->DRes.Disk = DRQ->DRes.Disk;

    if ((!bmisclear(&DRQ->ReqFBM)) &&
        (bmisclear(&RQ->ReqFBM)))
      bmcopy(&RQ->ReqFBM,&DRQ->ReqFBM);

    if (DRQ->Pref != NULL)
      {
      if ((!bmisclear(&DRQ->Pref->FeatureBM)) &&
          ((RQ->Pref == NULL) || 
           (bmisclear(&RQ->Pref->FeatureBM))))
        {
        MSched.ResourcePrefIsActive = TRUE;

        if (RQ->Pref == NULL)
          {
          if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
            {
            return(FAILURE);
            }
          }

        bmcopy(&RQ->Pref->FeatureBM,&DRQ->Pref->FeatureBM);
        }

      if ((DRQ->Pref->Variables != NULL) &&
          ((RQ->Pref == NULL) || (RQ->Pref->Variables == NULL)))
        {
        MSched.ResourcePrefIsActive = TRUE;

        if (RQ->Pref == NULL)
          {
          if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
            {
            return(FAILURE);
            }
          }

        MULLDup(&RQ->Pref->Variables,DRQ->Pref->Variables);
        }
      }    /* END if (DRQ->Pref != NULL) */
    }      /* END for (rqindex) */

  /* apply environment attributes */

  if ((DJ->Env.Cmd != NULL) && (J->Env.Cmd == NULL))
    {
    MUStrDup(&J->Env.Cmd,DJ->Env.Cmd);
    }  /* END if (DJ->E.Cmd = NULL) */

  if ((DJ->RMSubmitString != NULL) && (J->RMSubmitString == NULL))
    {
    /* NYI */
    }  /* END if (DJ->RMSubmitString != NULL) */

  if ((DJ->Env.IWD != NULL) && (J->Env.IWD == NULL))
    {
    if (!strcasecmp(DJ->Env.IWD,"$HOME"))
      {
      if ((J->Credential.U != NULL) && (J->Credential.U->HomeDir != NULL))
        {
        MUStrDup(&J->Env.IWD,J->Credential.U->HomeDir);
        }
      }
    else
      {
      MUStrDup(&J->Env.IWD,DJ->Env.IWD);
      }
    }    /* END if (DJ->E.IWD != NULL) */

  /* apply jobgroups, variables, and triggers */

  if ((DJ->JGroup != NULL) && (J->JGroup == NULL))
    {
    MJGroupInit(J,DJ->JGroup->Name,-1,DJ->JGroup->RealJob);
    }

  if ((DJ->Variables.Table != NULL) && (J->Variables.Table == NULL))
    {
    MUHTCopy(&J->Variables,&DJ->Variables);
    }

  if ((DJ->ReqRID != NULL) && (J->ReqRID == NULL))
    MUStrDup(&J->ReqRID,DJ->ReqRID);

  /* check triggers */

  /* can this be reduced to:
   *if (DJ->T != NULL)
      {
      MTrigApplyTemplate(&J->T,DJ->T,(void *)J,mxoJob,J->Name);
      }    
    as in MJobApplySetTemplate? */

  if (DJ->Triggers != NULL)
    {
    mtrig_t *T;

    int tindex;

    MTListCopy(DJ->Triggers,&J->Triggers,TRUE);

    for (tindex = 0; tindex < J->Triggers->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      /* update trigger OID with new RID */

      T->O = (void *)J;
      T->OType = mxoJob;

      MUStrDup(&T->OID,J->Name);

      MTrigInitialize(T);

      if ((T->EType == mttHold) &&
           bmisset(&J->IFlags,mjifNewBatchHold))
        {
        T->ETime = MSched.Time;
        }
      }      /* END for (tindex) */
    }        /* END if (DJ->T != NULL) */

  if (DJ->TemplateExtensions != NULL)
    {
    if (J->TemplateExtensions == NULL)
      {
      MJobAllocTX(J);
      }

    /* set template index in job */

    /* NOTE:  TX->TIndex only records 'last' applied set template */

    if ((DJ->TemplateExtensions->SpecTrig != NULL) && (J->TemplateExtensions->SpecTrig == NULL))
      {
      int tindex;

      for (tindex = 0;tindex < MMAX_SPECTRIG;tindex++)
        {
        if (DJ->TemplateExtensions->SpecTrig[tindex] == NULL)
          break;

        MUStrDup(&J->TemplateExtensions->SpecTrig[tindex],DJ->TemplateExtensions->SpecTrig[tindex]);
        }
      }
    }  /* END if (DJ->TX != NULL) */

  if (bmisset(&J->IFlags,mjifWCNotSpecified) && (DJ->SpecWCLimit[0] > 0))
    {
    /* NOTE:  should this change be pushed to RM, should we use MJobAdjustWallTime() */

    MJobSetAttr(J,mjaReqAWDuration,(void **)&DJ->SpecWCLimit[0],mdfLong,mSet);
    }

  MULLAdd(&J->TSets,DJ->Name,(void *)DJ,NULL,NULL);

  if ((DJ->Description != NULL) && (J->Description == NULL))
    MUStrDup(&J->Description,DJ->Description); 

  if (DJ->NodePriority != NULL)
    {
    if ((J->NodePriority != NULL) ||
        (MNPrioAlloc(&J->NodePriority) == SUCCESS))
      {
      memcpy(J->NodePriority,DJ->NodePriority,sizeof(J->NodePriority[0]));
      }
    }

  return(SUCCESS);
  }  /* END MJobApplyDefTemplate() */






/**
 * Return index of next job template from specified list which matches job.
 *
 * @see MJobCheckJMaxLimits() - child
 * @see MJobCheckJMinLimits() - child
 *
 * NOTE:  always pass JTList, IndexP must be initialized to '-1' on first call 
 *
 * @param J (I)
 * @param JTList (I)
 * @param JTListSize (I)
 * @param IndexP (I/O)
 */

mjtmatch_t *MJobGetTemplate(

  mjob_t      *J,           /* I */
  mjtmatch_t **JTList,      /* I */
  int          JTListSize,  /* I */
  int         *IndexP)      /* I/O */

  {
  int jtindex;

  mjtmatch_t *JTM;

  if ((J == NULL) || (JTList == NULL) || (IndexP == NULL))
    {
    /* FAILURE */

    return(NULL);
    }

  if ((*IndexP >= 0) && (JTList[*IndexP] != NULL))
    {
    /* IndexP points to last object processed */

    if (bmisset(&JTList[*IndexP]->FlagBM,mjtmfExclusive))
      {
      /* exclusive match previously located */

      return(NULL);
      }
    }

  for (jtindex = MAX(0,*IndexP + 1);jtindex < JTListSize;jtindex++)
    {
    JTM = JTList[jtindex];

    if (JTM == NULL)
      break;

    if (JTM == (mjtmatch_t *)1)
      continue;

    if (JTM->JMax != NULL)
      {
      /* verify max requirements */

      if (MJobCheckJMaxLimits(
            J,
            mxoJob,
            "job template",
            (mjob_t *)JTM->JMax,
            NULL,
            FALSE,
            NULL,  /* O */
            0) == FAILURE)
        {
        continue;
        }
      }

    if (JTM->JMin != NULL)
      {
      /* verify min requirements */

      if (MJobCheckJMinLimits(
            J,
            mxoJob,
            "job template",
            (mjob_t *)JTM->JMin,
            NULL,
            FALSE,
            NULL,  /* O */
            0) == FAILURE)
        {
        continue;
        }
      }

    /* template matches */

    *IndexP = jtindex; 

    return(JTM);
    }  /* END for (jtindex) */

  /* no match located */

  *IndexP = jtindex;
 
  return(NULL);
  }  /* END MJobGetTemplate() */


/*
 * Checks the requirements of a job template to see if it is valid as
 *  a generic system job.
 *
 * Returns SUCCESS if it is a valid generic sysjob, FAILURE otherwise.
 *
 * @param J [I] - The job template to be evaluated
 */

int MJobTemplateIsValidGenSysJob(

  mjob_t *J)   /* I */

  {
  mtrig_t *T;

  if (J == NULL)
    return(FAILURE);

  /* Job is an active template */
  if (!bmisset(&J->IFlags,mjifIsTemplate) ||
       bmisset(&J->IFlags,mjifTemplateIsDeleted))
    return(FAILURE);

  /* Job was declared as a generic system job */
  if (!bmisset(&J->IFlags,mjifGenericSysJob))
    return(FAILURE);

  /* Job has one trigger */
  if ((J->Triggers == NULL) || (J->Triggers->NumItems != 1))
    return(FAILURE);

  T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,0);

  /* Trigger has a timeout */
  if ((T == NULL) || (T->Timeout <= 0))
    return(FAILURE);

  /* Trigger is a "start" trigger */
  if (T->EType != mttStart)
    return(FAILURE);

  /* Trigger has an executable */
  if ((T->AType != mtatExec) ||
      (T->Action == NULL) ||
      (T->Action[0] == '\0'))
    return(FAILURE);

  return(SUCCESS);
  } /* END MJobTemplateIsValidGenSysJob() */


/* END MJobTemplates.c */
