/* HEADER */

      
/**
 * @file MJobCRUD.c
 *
 * Contains: Job Create/Read/Update/Delete functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* Extern reference */

int __MJobRemoveVCs(mjob_t *);


/* File namespace only global: MJobFreeIndex 
 *
 * indicates first available empty slot in MJob[] array, -1 indicates
 * that MJobFreeIndex should be recalculated 
 */

int MJobFreeIndex = -1;



/**
 * Assign new job id to job.
 *
 * @param J (I) [modified]
 * @param JName (I)
 */

int MJobRename(
 
  mjob_t *J,     /* I (modified) */
  char   *JName) /* I */

  {
  if ((J == NULL) || (JName == NULL))
    {
    return(FAILURE);
    }

  if (strcmp(J->Name,JName) == 0)
    {
    /* J->Name and JName are already the same */

    return(SUCCESS);
    }

  if ((J->SubmitRM != NULL) && (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue)))
    {
    MS3JobRename(J->SubmitRM,J->Name,JName);
    }

  /* Delete the job entry in the job name hash table with the old name as key.
     NOTE:  Don't modify hash tables for temporary jobs where mjifTemporaryJob is set */
  
  if (!bmisset(&J->IFlags,mjifTemporaryJob))
    {
    MJobHT.erase(J->Name);
  
    /* Add the job to the job name hash table wth the new name as the key */
  
    MJobTableInsert(JName,J);
    }

  /* remove the job from the cache before renaming it */

  MCacheRemoveJob(J->Name);

  MUStrCpy(J->Name,JName,sizeof(J->Name));

  /* rename all triggers */

  if (J->Triggers != NULL)
    {
    mtrig_t *T;
    
    int tindex;

    for (tindex = 0;tindex < J->Triggers->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      MUStrDup(&T->OID,J->Name);
      }
    }

  MJobTransition(J,FALSE,TRUE);

  return(SUCCESS);
  }  /* END MJobRename() */






/**
 * Create/clear new job - do not allocate mreq_t.
 *
 * @param JName  (I) [optional]
 * @param AddJob (I) [if set, add job to global table]
 * @param JP     (O) [optional]
 * @param EMsg   (O) [optional]
 */

int MJobCreate(
 
  const char *JName,
  mbool_t     AddJob,
  mjob_t    **JP,
  char       *EMsg)
 
  {
  mjob_t *J;

  const char *FName = "MJobCreate";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (JName != NULL) ? JName : "NULL",
    MBool[AddJob],
    (JP != NULL) ? "JP" : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (JP != NULL)
    *JP = NULL;

  if (JP == NULL)
    {
    return(FAILURE);
    }

  if ((JName == NULL) || (JName[0] == '\0'))
    {
    /* if job is being added, JName is required */

    return(FAILURE);
    }

  if ((J = (mjob_t *)MUCalloc(1,sizeof(mjob_t))) == NULL)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot allocate memory for job %s, errno: %d (%s)\n",
      (JName != NULL) ? JName : "NULL",
      errno,
      strerror(errno));

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot allocate memory for job %s, (%s)\n",
        (JName != NULL) ? JName : "NULL",
        strerror(errno));
      }

    return(FAILURE);
    }

  MUStrCpy(J->Name,JName,sizeof(J->Name));

  /* adding new job to MJobHT table */

  if (MSched.AS[mxoJob] == 0)
    {
    MSched.AS[mxoJob] =
      sizeof(mjob_t) +
      sizeof(mnl_t) * (MSched.JobMaxNodeCount + 1); 
    }  /* END if (MSched.AS[mxoJob] == 0) */

  if (AddJob == TRUE)
    {
    if (MJobHT.size() >= (unsigned int)MSched.M[mxoJob])
      {
      MDB(1,fALL) MLog("WARNING:  maximum job count reached, increase MAXJOB (cannot add job '%s')\n",
        (JName != NULL) ? JName : NULL);

      if (MSched.OFOID[mxoJob] == NULL)
        MUStrDup(&MSched.OFOID[mxoJob],JName);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"system job limit reached (%d)\n",MSched.M[mxoJob]);
        }

      MUFree((char **)&J);

      return(FAILURE);
      }

    MJobTableInsert(J->Name,J);
    }

  *JP = J;

  return(SUCCESS);
  }  /* END MJobCreate() */

 



/**
 * Destroy job and free dynamically allocated memory.
 *
 * NOTE: this routine can handle real and temporary jobs.  It does not "cleanup"
 *       a real job (remove reservation, check completion actions), that all 
 *       happens in MJobRemove() and MJobCleanupForRemoval().
 *
 * @see MJobRemove(), MJobDestroyList()
 *
 * @param JP (I) Pointer to the job whose members should be freed. This pointer
 * will also be freed i.f.f. the mjifTemporaryJob flag is not set [modified/freed]
 */

int MJobDestroy(
 
  mjob_t **JP)  /* I (modified/freed) */
 
  {
  mbool_t TemporaryJob = FALSE;

  int     rqindex;
 
  mreq_t *RQ;

  mjob_t *J;

  const char *FName = "MJobDestroy";
 
  MDB(2,fSTRUCT) MLog("%s(%s)\n",
    FName,
    ((JP != NULL) && (*JP != NULL)) ? (*JP)->Name : "NULL");

  if ((JP == NULL) || (*JP == NULL))
    {
    MDB(0,fSTRUCT) MLog("ERROR:    invalid job pointer specified\n");

    return(FAILURE);
    }

  J = *JP;      

  /* NOTE:  most operations are done regardless of whether or not job is being 
            removed or Moab is shutting down - operations which effect external
            environment, files, rm, etc, should only be located in the non-
            shutdown clause at the end of the routine. */

  if (J->Triggers != NULL)
    {
    /* NOTE:  must launch triggers before freeing required job structures */

    MTListClearObject(J->Triggers,FALSE);

    MUArrayListFree(J->Triggers);

    MUFree((char **)&J->Triggers);

    J->Triggers = NULL;
    }

  if (J->Array != NULL)
    {
    MJobArrayFree(J);
    }

/* Don't launch prolog/epilog triggers here
   Instead we will free them below */

/*
  if (J->PrologTList != NULL)
    {
    MTListClearObject(J->PrologTList,FALSE);

    MUFree((char **)&J->PrologTList);
    }

  if (J->EpilogTList != NULL)
    {
    MTListClearObject(J->EpilogTList,FALSE);

    MUFree((char **)&J->EpilogTList);
    }
*/

  if (bmisset(&J->IFlags,mjifTemporaryJob))
    TemporaryJob = TRUE;

  if (J->PrologTList != NULL)
    {
    MTrigFreeList(J->PrologTList);

    MUArrayListFree(J->PrologTList);

    MUFree((char **)&J->PrologTList);
    } /* END if (J->PrologTList != NULL) */

  if (J->EpilogTList != NULL)
    {
    MTrigFreeList(J->EpilogTList);

    MUArrayListFree(J->EpilogTList);

    MUFree((char **)&J->EpilogTList);
    } /* END if (J->EpilogTList != NULL) */
 
  if (J->TemplateExtensions != NULL)
    {
    MJobDestroyTX(J);
    }  /* END if (J->TX != NULL) */

  if (bmisset(&J->IFlags,mjifIsTemplate))
    {
    MTJobFreeXD(J);
    }
  else
    {
    MUFree((char **)&J->ExtensionData);
    }

  /* This will *usually* be redundant, but helps for when
      temporary jobs are added to VCs and then destroyed */
  MVCRemoveObjectParentVCs((void *)J,mxoJob);

  if (J->System != NULL)
    {
    switch (J->System->JobType)
      {
      case msjtVMMap:

        /* remove VM pointer to job */
     
        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (J->System->JobType) */

    MJobFreeSystemStruct(J);
    }

  if (J->JGroup != NULL)
    {
    MJGroupFree(J);
    }

  /* release job reservations */
 
  if (J->Rsv != NULL) 
    {
    mrsv_t *R;

    /* NULL out R->J to prevent looping */

    R = (mrsv_t *)J->Rsv;

    R->J = NULL;

    MJobReleaseRsv(J,TRUE,TRUE);

    if (bmisset(&J->Flags,mjfAdvRsv) && bmisset(&J->Flags,mjfNoRMStart))
      {
      /* handle system job */

      /* NOTE:  may have multiple rsv's pointing to single job */

      /* locate additional rsv's and clear J pointer */

      if ((J->RsvAList != NULL) && 
          (J->RsvAList[0] != NULL) &&
          (MRsvFind(J->RsvAList[0],&R,mraNONE) == SUCCESS))
        {
        if (R->J == J)
          R->J = NULL;
        }
      }
    }    /* END if (J->R != NULL) */

  if (J->RsvAList != NULL)
    {
    int aindex;

    for (aindex = 0;J->RsvAList[aindex] != NULL;aindex++)
      {
      MUFree((char **)&J->RsvAList[aindex]);
      }

    MUFree((char **)&J->RsvAList);
    }

  if (J->Credential.ACL != NULL)
    MACLFreeList(&J->Credential.ACL);

  if (J->Credential.CL != NULL)
    MACLFreeList(&J->Credential.CL);

  if (J->RequiredCredList != NULL)
    {
    MACLFreeList(&J->RequiredCredList);
    }

  MUFree((char **)&J->Credential.Templates);
  MUFree((char **)&J->SysSMinTime);

  if (J->VMCreateRequest != NULL)
    {
    MUFree((char **)&J->VMCreateRequest);
    }

  if (J->MessageBuffer != NULL)
    {
    MMBFree(&J->MessageBuffer,TRUE);
    }

  MUFree((char **)&J->TaskMap);
  J->TaskMapSize = 0;

  MUFree(&J->ReqVPC);

  if (TemporaryJob == FALSE)
    {
/*  replaced with DRMJID hash table in 6.1 
    MUHTRemove(&MJobANameHT,J->AName,MUFREE);
*/

    MUHTRemove(&MJobDRMJIDHT,J->DRMJID,NULL);
    }

  MUFree(&J->InteractiveScript);
  MUFree(&J->AName); 
  MUFree(&J->SRMJID);
  MUFree(&J->DRMJID);

  MUFree(&J->Owner);
  MUFree(&J->EUser);

  MULLFree(&J->BlockInfo,(int(*)(void **))MJobBlockInfoFree);

  MUFree(&J->MigrateBlockMsg);
  MUFree(&J->DependBlockMsg);
  MUFree(&J->Description);

  MUFree(&J->SpecNotification);
  MUFree(&J->NotifyAddress);
  MUFree(&J->SubmitHost);

  MDSFree(&J->DataStage);

  MUFree(&J->Env.BaseEnv);
  MUFree(&J->Env.IncrEnv);
  MUFree(&J->Env.Input);
  MUFree(&J->Env.Output);
  MUFree(&J->Env.Error);
  MUFree(&J->Env.Args);
  MUFree(&J->Env.RMOutput);
  MUFree(&J->Env.RMError);
  MUFree(&J->Env.IWD);
  MUFree(&J->Env.Cmd);
  MUFree(&J->Env.Shell);

  if (J->Credential.Templates != NULL)
    MUFree((char **)&J->Credential.Templates);

  /* NOTE:  J->E.Cmd must be handled specially based on real/temp job status (see below) */

  MUFree(&J->Env.IWD);

  if (J->Env.SubmitDir != NULL)
    {
    MUFree(&J->Env.SubmitDir);
    }

  MUFree(&J->RMSubmitFlags);
  MUFree(&J->RMXString);
  MUFree(&J->RMSubmitString);

  MUFree(&J->MasterHostName);

  MUFree(&J->ReqRID);
  MUFree(&J->PeerReqRID);

  MNLFree(&J->ReqHList);
  MNLFree(&J->ExcHList);
  MUFree((char **)&J->ReqVMList);

  MUDynamicAttrFree(&J->DynAttrs);

  MJobDependDestroy(J);

  MXLoadDestroy(&J->ExtLoad);

  MUFree(&J->Geometry);          
  MUFree((char **)&J->FairShareTree);

  MUFree((char **)&J->RequiredCredList);

  if (J->Ckpt != NULL)
    MJobCPDestroy(J,&J->Ckpt);

  /* erase req structures */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];
 
    if (RQ == NULL)
      break;

    MReqDestroy(&J->Req[rqindex]);
    }  /* END for (rqindex) */

  /* remove job from node joblist */

  MNLFree(&J->NodeList);

  MJobFreeSID(J); /* erases J->G */

  MUFree((char **)&J->AttrString);

  MULLFree(&J->TStats,NULL);
  MULLFree(&J->TSets,NULL);

  MULLFree(&J->ImmediateDepend,MUFREE);

  MUHTFree(&J->Variables,TRUE,MUFREE);

  MUFree(&J->CompletedVMList);

  MUFree(&J->PrologueScript);

  MUFree(&J->EpilogueScript);

  MNPrioFree(&J->NodePriority);

  MUFree(&J->EligibilityNotes);

  bmclear(&J->SysPAL);
  bmclear(&J->PAL);
  bmclear(&J->EligiblePAL);
  bmclear(&J->CurrentPAL);
  bmclear(&J->SpecPAL);

  bmclear(&J->Hold);
  bmclear(&J->JobRejectPolicyBM);
  bmclear(&J->Flags);
  bmclear(&J->IFlags);
  bmclear(&J->SpecFlags);
  bmclear(&J->SysFlags);
  bmclear(&J->RMXSetAttrs);
  bmclear(&J->NodeMatchBM);
  bmclear(&J->AttrBM);
  bmclear(&J->NotifySentBM);
  bmclear(&J->NotifyBM);
  bmclear(&J->RsvAccessBM);
  bmclear(&J->ReqFeatureGResBM);

  /* erase job structure */

  if (TemporaryJob == TRUE)
    {
    MUFree(&J->Env.Cmd);
    }
  else
    {
    /* job is 'real'/non-temporary */

    /* don't erase files when shutting down */

    if (MSched.Shutdown == FALSE)
      {

      MJobRemoveCP(J);

      /* remove temporary cmd file, if it exists */

      if (J->Env.Cmd != NULL)
        {
        if (!strncasecmp(J->Env.Cmd,"moab.job.",strlen("moab.job.")))
          {
          MDB(7,fSTRUCT) MLog("INFO:     deleting temporary job command file '%s'\n",
            J->Env.Cmd);
          }

        MUFree(&J->Env.Cmd);
        }
      }

    /* remove/free job from job table */

    MJobHT.erase(J->Name);

    J->Name[0] = '\0';

    MUFree((char **)JP);

    *JP = NULL;
    }  /* END else ((J->Index <= 0) || (MSched.State == mssmShutdown)) */

  return(SUCCESS);
  }  /* END MJobDestroy() */





/**
 * Update per-iteration and cred based job flags.
 *
 * Flags is reset to SpecFlags and then or'ed with the job's credentials flags
 * and other flags.
 *
 * @param J (I) [modified]
 */

int MJobUpdateFlags(

  mjob_t *J)  /* I (modified) */

  {
  int rqindex;
  int index;

  mqos_t *Q = NULL;  /* effective QoS */

  mreq_t *RQ;

  if (J == NULL)
    {
    return(FAILURE);
    }

  RQ = J->Req[0];
  
  /* J->Flags is supposed to be reset every time MJobUpdateFlags is called. If a
   * flag is to be persistent then SpecFlags should be set or set on a cred. */

  bmcopy(&J->Flags,&J->SpecFlags);
  bmor(&J->Flags,&J->SysFlags);

  if (J->Credential.U != NULL)
    bmor(&J->Flags,&J->Credential.U->F.JobFlags);

  if (J->Credential.G != NULL)
    bmor(&J->Flags,&J->Credential.G->F.JobFlags);

  if (J->Credential.A != NULL)
    bmor(&J->Flags,&J->Credential.A->F.JobFlags);

  if (J->Credential.C != NULL)
    bmor(&J->Flags,&J->Credential.C->F.JobFlags);

  if ((MSched.DefaultU != NULL) &&
      (J->Credential.U != NULL) && 
      (bmisclear(&J->Credential.U->F.JobFlags)))
    {
    bmor(&J->Flags,&MSched.DefaultU->F.JobFlags);
    }

  if (J->ReqRID == NULL)
    {
    char *tmpPtr = NULL;

    /* determine ReqRID */

    if ((J->Credential.Q != NULL) && (J->Credential.Q->F.ReqRsv != NULL))
      tmpPtr = J->Credential.Q->F.ReqRsv;
    else if ((J->Credential.U != NULL) && (J->Credential.U->F.ReqRsv != NULL))
      tmpPtr = J->Credential.U->F.ReqRsv;
    else if ((J->Credential.A != NULL) && (J->Credential.A->F.ReqRsv != NULL))
      tmpPtr = J->Credential.A->F.ReqRsv;
    else if ((J->Credential.G != NULL) && (J->Credential.G->F.ReqRsv != NULL))
      tmpPtr = J->Credential.G->F.ReqRsv;
    else if ((J->Credential.C != NULL) && (J->Credential.C->F.ReqRsv != NULL))
      tmpPtr = J->Credential.C->F.ReqRsv;

    if (tmpPtr != NULL)
      {
      if (tmpPtr[0] == '!')
        {
        bmset(&J->IFlags,mjifNotAdvres);

        tmpPtr++;
        }

      MUStrDup(&J->ReqRID,tmpPtr);

      bmset(&J->SpecFlags,mjfAdvRsv);
      bmset(&J->Flags,mjfAdvRsv);
      }
    }  /* END if (J->ReqRID == NULL) */

  if (J->Credential.Q != NULL)
    {
    Q = J->Credential.Q;

    bmor(&J->Flags,&Q->F.JobFlags);

    if (bmisset(&Q->Flags,mqfUseReservation))
      bmset(&J->Flags,mjfAdvRsv);

    if (bmisset(&Q->Flags,mqfpreemptee))
      bmset(&J->Flags,mjfPreemptee);

    if (bmisset(&Q->Flags,mqfpreemptor))
      bmset(&J->Flags,mjfPreemptor);

    if (bmisset(&Q->Flags,mqfIgnHostList))
      bmunset(&J->IFlags,mjifHostList);

#ifdef MVIRTUAL
    if (bmisset(&Q->Flags,mqfVirtual))
      bmset(&J->IFlags,mjifRequiresVM);
#endif /* MVIRTUAL */

    if (!bmisset(&Q->Flags,mqfCoAlloc))
      bmunset(&J->Flags,mjfCoAlloc);
    else
      bmset(&J->Flags,mjfCoAlloc);

    if (bmisset(&Q->Flags,mqfRunNow))
      {
      /* set system priority */

      J->SystemPrio = 2;

      bmset(&J->IFlags,mjifRunNow);
      bmset(&J->Flags,mjfPreemptor);
      }

    if (RQ != NULL)
      {
      if (bmisset(&Q->Flags,mqfDedicated))
        {
        RQ->NAccessPolicy = mnacSingleJob;

        MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from cred based job flags\n",
          J->Name,
          MNAccessPolicy[RQ->NAccessPolicy]);
        }
      }

    /* determine preemption status */

    if (!bmisset(&J->Flags,mjfPreemptor))
      {
      if ((Q->QTPreemptThreshold > 0) && (J->EffQueueDuration >= Q->QTPreemptThreshold))
        {
        bmset(&J->Flags,mjfPreemptor);
        }
      else if ((Q->XFPreemptThreshold > 0) && (J->EffXFactor >= Q->XFPreemptThreshold))
        {
        bmset(&J->Flags,mjfPreemptor);
        }
      }  /* END if (!bmisset(&J->Flags,mjfPreemptor)) */
    }    /* END if (J->Cred.Q != NULL) */
  else if (MSched.DefaultQ != NULL)
    {
    Q = MSched.DefaultQ;

    bmor(&J->Flags,&MSched.DefaultQ->F.JobFlags);
    }
  else
    {
    /* handle no-qos-specified condition */

    Q = &MQOS[0];
    }    /* END else */

  /* handle next-to-run */

  if (bmisset(&Q->Flags,mqfNTR))
    {
    /* set system priority */

    J->SystemPrio = 1;
    }

  /* handle FS/SP violation preemption */

  if ((bmisset(&J->Flags,mjfSPViolation)) &&
     ((MSched.EnableSPViolationPreemption == TRUE) || bmisset(&Q->Flags,mqfPreemptSPV)))
    {
    bmset(&J->Flags,mjfPreemptee);
    }
  else if ((bmisset(&J->Flags,mjfFSViolation)) &&
          ((MSched.EnableFSViolationPreemption == TRUE) || bmisset(&Q->Flags,mqfPreemptFSV)))
    {
    bmset(&J->Flags,mjfPreemptee);
    }

  /* NOTE:  should move most per-job QoS handling here to process default qos specification */

  /* determine task distribution policy */

  if (J->SpecDistPolicy != 0)
    J->DistPolicy = J->SpecDistPolicy;
  else if ((J->Credential.C != NULL) && (J->Credential.C->DistPolicy != 0))
    J->DistPolicy = J->Credential.C->DistPolicy;

  /* NOTE:  does not allow per req access policy specification */

  /* NOTE: this setting overrides per-node settings  */

#ifdef __MNOT
  if ((RQ != NULL) && (RQ->NAccessPolicy == mnacNONE))
    {
    RQ->NAccessPolicy = MSched.DefaultNAccessPolicy;
    }    /* END if (RQ != NULL) */
#endif
 
  MDB(7,fSTRUCT)
    {
    mstring_t Buf(MMAX_LINE);
    
    MJobFlagsToMString(NULL,&J->Flags,&Buf);

    MLog("INFO:     job flags for job %s: %s\n",
      J->Name,
      Buf.c_str());
    }

  /* update job attributes */

  if (bmisset(&J->Flags,mjfPreemptee))
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfPreemptee],mdfString,mSet);
  else
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfPreemptee],mdfString,mUnset);

  if (bmisset(&J->Flags,mjfBackfill))
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfBackfill],mdfString,mSet);
  else
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfBackfill],mdfString,mUnset);

  if (bmisset(&J->Flags,mjfInteractive) || (J->IsInteractive == TRUE))
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfInteractive],mdfString,mSet);
  else
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfInteractive],mdfString,mUnset);

  if (bmisset(&J->Flags,mjfFSViolation))
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfFSViolation],mdfString,mSet);
  else
    MJobSetAttr(J,mjaGAttr,(void **)MJobFlags[mjfFSViolation],mdfString,mUnset);

  if (MSched.NodeToJobAttrMap[0][0] != '\0')
    {
    mbitmap_t CacheFBM;
    int index;

    /* NOTE:  only supports single req jobs */

    for (index = 0;MSched.NodeToJobAttrMap[index][0] != '\0';index++)
      {
      MUGetNodeFeature(MSched.NodeToJobAttrMap[index],mAdd,&CacheFBM);
      }  /* END for (index) */
   
    /* translate node features to job attributes */

    for (index = 0;index < MMAX_ATTR;index++)
      {
      if (MAList[meNFeature][index][0] == '\0')
        break;

      if (J->Req[0] == NULL)
        break;

      if ((bmisset(&J->Req[0]->ReqFBM,index) && 
          (bmisset(&CacheFBM,index))))
        {
        MJobSetAttr(J,mjaGAttr,(void **)MAList[meNFeature][index],mdfString,mSet);
        }
      }  /* END for (index) */
    }    /* END if (MSched.NodeToJobAttrMap[0] != 0) */

  /* translate generic resources to job attributes */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;
 
    if (MSNLIsEmpty(&J->Req[rqindex]->DRes.GenericRes))
      continue;
 
    for (index = 1;index < MSched.M[mxoxGRes];index++)
      {
      if (MGRes.Name[index][0] == '\0')
        break;
 
      if ((MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,index) > 0) && 
          (MGRes.GRes[index]->JobAttrMap == TRUE))
        {
        MJobSetAttr(J,mjaGAttr,(void **)MGRes.Name[index],mdfString,mSet);
        }
      }   /* END for (index) */
    }     /* END for (rqindex) */

  /* check special QOS flags */

  if (bmisset(&J->SpecFlags,mjfIgnPolicies) &&
      !bmisset(&J->SpecFlags,mjfAdminSetIgnPolicies) &&
      ((J->Credential.Q == NULL) ||
       !bmisset(&J->Credential.Q->F.JobFlags,mjfIgnPolicies)))
    {
    bmunset(&J->SpecFlags,mjfIgnPolicies);
    bmunset(&J->Flags,mjfIgnPolicies);
    } /* END if (bmisset(&J->SpecFlags,mjfIgnPolicies)) */

  if ((!bmisset(&J->SpecFlags,mjfRsvMap)) &&
      bmisset(&J->SpecFlags,mjfIgnIdleJobRsv) &&
      ((J->Credential.Q == NULL) ||
       !bmisset(&J->Credential.Q->F.JobFlags,mjfIgnIdleJobRsv)))
    {
    bmunset(&J->SpecFlags,mjfIgnIdleJobRsv);
    bmunset(&J->Flags,mjfIgnIdleJobRsv);
    } /* END if (bmisset(&J->SpecFlags,mjfIgnPolicies)) */

  if (bmisset(&J->SpecFlags,mjfIgnNodePolicies) &&
      ((J->Credential.Q == NULL) ||
       !bmisset(&J->Credential.Q->F.JobFlags,mjfIgnNodePolicies)))
    {
    bmunset(&J->SpecFlags,mjfIgnNodePolicies);
    bmunset(&J->Flags,mjfIgnNodePolicies);
    } /* END if (bmisset(&J->SpecFlags,mjfIgnPolicies)) */

  return(SUCCESS);
  }  /* END MJobUpdateFlags() */




/**
 *
 *
 * @param J (I)
 * @param SRQ (I) [optional]
 * @param SIndex (I) shape index
 * @param ReqPCP (O) [optional]
 */

int MJobUpdateResourceCache(

  mjob_t *J,      /* I */
  mreq_t *SRQ,    /* I (optional) */
  int     SIndex, /* I shape index */
  int    *ReqPCP) /* O (optional) */

  {
  mreq_t *RQ;

  int rqindex;
  int nindex;

  int proccount;

  int *PCP;

  if (J == NULL)
    {
    return(FAILURE);
    }

  PCP = (ReqPCP != NULL) ? ReqPCP : &J->TotalProcCount;

  /* update total proc count         */ 
  /* must handle dedicated resources */

  proccount = 0;

  if (bmisset(&J->Flags,mjfNoResources))
    {
    /* NOTE:  no resource jobs require no processors */

    *PCP = 0;

    return(SUCCESS);
    }

  if (MJOBISACTIVE(J) == TRUE)
    {
    enum MNodeAccessPolicyEnum NAPolicy;

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];

      if ((SRQ != NULL) && (RQ != SRQ))
        continue;

      MJobGetNAPolicy(J,RQ,NULL,NULL,&NAPolicy);

      if ((!bmisset(&J->Flags,mjfRsvMap)) &&
          (!bmisset(&MSched.Flags,mschedfShowRequestedProcs)) &&
         ((NAPolicy == mnacSingleJob) ||
          (NAPolicy == mnacSingleTask) ||
          (RQ->DRes.Procs == -1)))
        {
        mnode_t *N;

        /* node resources are dedicated */

        if (MNLIsEmpty(&RQ->NodeList))
          {
          /* no hostlist specified */

          /* HACK:  assumes all nodes have similar processor configurations */

          /* should NAPolicy be used instead of calling RQ->TaskCount? It's possible
           * that RQ->TaskCount is still none even though NODEACCESSPOLICY SINGLEJOB
           * is set in the moab.cfg? */

          if (RQ->NAccessPolicy == mnacSingleJob)
            {
            /* assume packing */

            proccount += RQ->TaskCount * RQ->DRes.Procs;
            }
          else
            {
            /* only one task per node */

            proccount += 
              RQ->TaskCount * ((MNode[0] != NULL) ? MNodeGetBaseValue(MNode[0],mrlProc) : 1);
            }

          continue;
          }

        /* step through node list */

        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          proccount += MNodeGetBaseValue(N,mrlProc);
          }
        }
      else
        {
        /* resources not dedicated */

        proccount += RQ->TaskCount * RQ->DRes.Procs;
        }
      }    /* END for (rqindex) */
    }      /* END if (MJOBISACTIVE(J) == TRUE) */
  else
    {
    if (bmisset(&J->IFlags,mjifHostList) && 
       (!MNLIsEmpty(&J->ReqHList)))
      {
      mnode_t *N;

      MNLGetNodeAtIndex(&J->ReqHList,0,&N);
      }

    /* final job task mapping not established */

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];

      if ((SRQ != NULL) && (RQ != SRQ))
        continue;
 
      if (!bmisset(&J->Flags,mjfRsvMap) &&
          ((RQ->NAccessPolicy == mnacSingleJob) ||
           (RQ->NAccessPolicy == mnacSingleTask) ||
           (RQ->DRes.Procs == -1)))
        {
        int TC;

        /* node resources are dedicated */

        /* NOTE:  assume homogeneous resources with MNode[0] indicative of procs/node */

        TC = (SIndex > 0) ? RQ->TaskRequestList[SIndex] : RQ->TaskCount;

        if ((RQ->NAccessPolicy == mnacSingleTask) ||
            (RQ->DRes.Procs == -1))
          {
          /* only one task per node */

          proccount += TC * ((MNode[0] != NULL) ? MNodeGetBaseValue(MNode[0],mrlProc) : 1);
          }
        else
          {
          /* assume packing */

          proccount += TC * RQ->DRes.Procs;
          }
        }
      else
        {
        /* resources not dedicated */

        if (SIndex > 0)
          proccount += RQ->TaskRequestList[SIndex] * RQ->DRes.Procs;
        else
          proccount += RQ->TaskCount * RQ->DRes.Procs;
        }
      }    /* END for (rqindex) */
    }      /* END else (MJOBISACTIVE(J) == TRUE) */

  if ((proccount > 0) || (J->Req[0] == NULL))
    {
    *PCP = proccount;
    }
  else if (SRQ == NULL) 
    {
    /* NOTE:  only consider master req */

    if (!bmisset(&J->IFlags,mjifIsTemplate))
      {
      if (!bmisset(&J->IFlags,mjifDProcsSpecified))
        {
        *PCP = J->Request.TC * 
          ((J->Req[0]->DRes.Procs > 0) ? J->Req[0]->DRes.Procs : 1);
        }
      else
        {
        *PCP = J->Request.TC * J->Req[0]->DRes.Procs;
        }

      if (!bmisset(&J->IFlags,mjifTasksSpecified))
        *PCP = MAX(1,*PCP);
      }
    else
      {
      *PCP = 0;
      }
    }
  else
    {
    *PCP = 0;
    }

  return(SUCCESS);
  }  /* END MJobUpdateResourceCache() */





/**
 * Get a job ready for removal from MJobHT.
 *
 * NOTE: helper function for MJobRemove(), MJobProcessRemoved() and MJobProcessCompleted()
 * NOTE: job should then be ready for either migration to MCJobHT or MJobDestroy
 *
 * NOTE: does the following:
 *         checks job template actions
 *         removes VMs
 *         removes reservations
 *         
 *         
 *         
 *         
 *
 * @param J (I) modified
 */

int MJobCleanupForRemoval(

  mjob_t *J)

  {
  mrm_t *RM;

  /* Remove from VCs */

  __MJobRemoveVCs(J);

  /* Check if this job is a template action */

  if ((J->TemplateExtensions != NULL) &&
      (J->TemplateExtensions->TJobAction != mtjatNONE) &&
      (J->TemplateExtensions->JobReceivingAction != NULL))
    {
    mjob_t *JobReceivingAction = NULL;

    if (MJobFind(J->TemplateExtensions->JobReceivingAction,&JobReceivingAction,mjsmBasic) == SUCCESS)
      {
      switch (J->TemplateExtensions->TJobAction)
        {
        case mtjatDestroy:

          if ((J->CompletionCode == 0) && (J->CompletionTime > 0))
            {
            char tmpMsg[MMAX_LINE];

            snprintf(tmpMsg,sizeof(tmpMsg),"%s cancelled by destroy action job %s\n",
              JobReceivingAction->Name,
              J->Name);

            bmset(&JobReceivingAction->IFlags,mjifDestroyByDestroyTemplate);
            MJobCancel(JobReceivingAction,tmpMsg,FALSE,NULL,NULL);
            }
          else
            {
            /* Job failed, need to clear the flag on job so that we can resubmit
              the destroy job */

            bmunset(&JobReceivingAction->Flags,mjfDestroyTemplateSubmitted);
            bmunset(&JobReceivingAction->SpecFlags,mjfDestroyTemplateSubmitted);
            }

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (J->TX->TJobAction) */
      }  /* END if (MJobFind(J->TX->JobReceivingAction) == SUCCESS) */
    }  /* END if ((J->TX != NULL) && ...) */

  /* release job reservation */
 
  if (MJobReleaseRsv(J,TRUE,TRUE) == FAILURE)
    {
    MDB(0,fSTRUCT) MLog("ERROR:    unable to destroy job reservation\n");
    }

  if ((J->SubmitRM != NULL) && bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue))
    {
    MS3RemoveLocalJob(J->SubmitRM,J->Name);
    }

#ifdef MYAHOO
  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    /* NOTE:  upon shutdown, this should only be called once, not once per job - FIXME */

    /* need to checkpoint RM so that this job is *not* loaded as running on restart */

    MCPStoreCluster(&MCP,NULL);
    }
#endif /* MYAHOO */

  if ((bmisset(&J->Flags,mjfAdvRsv)) && 
      (J->ReqRID != NULL) &&
      (!bmisset(&J->IFlags,mjifNotAdvres)))
    {
    mrsv_t *R;

    if ((MRsvFind(J->ReqRID,&R,mraNONE) == SUCCESS) ||
        (MRsvFind(J->ReqRID,&R,mraRsvGroup) == SUCCESS))
      {
      if (bmisset(&R->Flags,mrfSingleUse))
        {
        if (bmisset(&R->Flags,mrfStanding))
          {
          msrsv_t *SR;

          /* disable SR slot */

          if ((R->RsvParent != NULL) && (MSRFind(R->RsvParent,&SR,NULL) == SUCCESS))
            {
            int sindex;

            for (sindex = 0;sindex < MMAX_SRSV_DEPTH;sindex++)
              {
              if (SR->DisabledTimes[sindex] <= 1)
                {
                SR->DisabledTimes[0] = R->EndTime;

                break;
                }
              }
            }
          }

        if (MRsvDestroy(&R,TRUE,TRUE) == FAILURE)
          {
          MDB(0,fSTRUCT) MLog("ERROR:    unable to destroy single use required job reservation\n");
          }
        }
      }      /* END if ((MRsvFind(J->ReqRID,&R,NULL,mraNONE) == SUCCESS) || ...) */
    }        /* END if (bmisset(&J->Flags,mjfAdvRsv) && ...) */

  if (!bmisset(&J->IFlags,mjifTemporaryJob))
    {
    /* job is in scheduler job table */

    MJobRemoveFromNL(J,NULL);

    MQueueRemoveAJob(J,mstForce);

    /* remove hash table entries */

    if (J->DRMJID != NULL) 
      MUHTRemove(&MJobDRMJIDHT,J->DRMJID,NULL);

    MSim.QueueChanged = TRUE;
    }  /* END if (J->Prev != NULL) */

  if (J->System != NULL)
    {
    if (bmisset(&J->System->CFlags,msjcaJobStart) &&
        (J->System->Dependents != NULL) &&
        (!bmisset(&J->System->EFlags,msjefProvFail)))
      {
      /* start master job on system job's allocated node list */

      MJobRunImmediateDependents(J,J->System->Dependents);
      }    /* END if (bmisset(&J->System->CFlags,msjcaJobStart) ...) */

    if (J->System->JobType == msjtVMTracking)
      {
      /* Remove this job from the VM structure */

      mvm_t *TmpVM;
      char  *VMID = NULL;

      if (MVMFind(J->System->VM,&TmpVM) == SUCCESS)
        {
        TmpVM->TrackingJ = NULL;

        if (TmpVM->J == J)
          TmpVM->J = NULL;

        /* Move VM to completed queue - VMTracking job is being removed! */

        MVMComplete(TmpVM);
        }

      if (MUHTGet(&J->Variables,"VMID",(void **)&VMID,NULL) == SUCCESS)
        {
        /* Remove VMID from hast table */

        MDB(7,fSTRUCT) MLog("INFO:     removing '%s' from requested VMIDs for job '%s'\n",
          VMID,
          J->Name);

        MUHTRemove(&MVMIDHT,VMID,NULL);
        }
      }      /* END if (J->System->JobType == msjtVMTracking) */
    }      /* END if (J->System != NULL) */

  if (J->ImmediateDepend != NULL)
    {
    MJobRunImmediateDependents(J,J->ImmediateDepend);
    }

  if (J->DestinationRM != NULL)
    RM = J->DestinationRM;
  else
    RM = J->SubmitRM;

  if ((RM != NULL) && ((RM->SubType == mrmstXT3) || (RM->SubType == mrmstXT4)))
    {
    MJobDestroyAllocPartition(J,RM,"job removed");
    }

  if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->R != NULL))
    {
    MRsvDestroy(&J->TemplateExtensions->R,TRUE,TRUE);
    }

  /* Clear holds and blocks so that cache reports the job as completed. */
  bmclear(&J->Hold);
  MULLFree(&J->BlockInfo,(int(*)(void **))MJobBlockInfoFree);
  MUFree(&J->MigrateBlockMsg);
  MUFree(&J->DependBlockMsg);
  J->MigrateBlockReason = mjneNONE;
  J->DependBlockReason  = mdNONE;

  return(SUCCESS);
  }  /* END MJobCleanupForRemoval() */





/**
 * Prepare a job for removal by cleaning up reservations
 *
 *
 * NOTE: 
 *
 * @see MJobDestroy() - child
 *
 * @param JP (I) [modified/freed]
 */

int MJobRemove(
 
  mjob_t **JP)  /* I (modified/freed) */
 
  {
  mjob_t *J;

  const char *FName = "MJobRemove";

  MDB(2,fSTRUCT) MLog("%s(%s)\n",
    FName,
    ((JP != NULL) && (*JP != NULL) && (*JP != (mjob_t *)1)) ? (*JP)->Name : "NULL");
 
  /* TODO - if we find that jobs are left stranded in the job transition cache which have been removed from MJob[]
            then we may need to add a boolean to this routine to indicate whether the job should also be
            removed from the job transition cache. In that case we may need to send an MJobTransition() with
            a delete only option. */

  if ((JP == NULL) ||
      (*JP == NULL) || 
      (*JP == (mjob_t *)1))
    {
    MDB(0,fSTRUCT) MLog("ERROR:    invalid job pointer passed to %s()\n",
      FName);
 
    return(SUCCESS);
    }

  J = *JP;

  if ((J->Name[0] == '\0') ||
      (J->Name[0] == '\1'))
    {
    MDB(0,fSTRUCT) MLog("INFO:     job previously removed\n");
 
    return(SUCCESS);
    }
 
  MJobCleanupForRemoval(J);

  /* MJobsReadyToRun can have this job in it's list and will segfault when
   * processed in MSchedProcessJobs. */

  MULLRemove(&MJobsReadyToRun,J->Name,NULL);
    
  MJobDestroy(&J);

  *JP = NULL;
 
  return(SUCCESS);
  }  /* END MJobRemove() */



