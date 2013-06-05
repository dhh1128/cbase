/* HEADER */

      
/**
 * @file MUGenerateSystemJobs.c
 *
 * Contains: MUGenerateSystemJobs
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



#define MDEF_POWERDURATION  600

/**
 * Generates system jobs of the specified type.
 *
 * @see MSysScheduleProvisioning() - parent
 * @see MSysSchedulePower() - parent
 * @see MUTCCreateSystemJobs() - child
 * @see MUNLCreateJobs() - child
 * @see MRsvSyncSystemJob() - peer
 * @see MJobPopulateSystemJobData() - child
 *
 * NOTE: either J or NL must be specified
 * NOTE: if OrigJ is specified then the system job will get a walltime of DURATION += OrigJ->WallTime
 *       and if JType == Provision then only nodes that don't have the right ActiveOS are switched
 *  
 * @param Auth     (I) [optional] Authenticated User Name.  NULL defaults to root.
 * @param OrigJ    (I) [optional]
 * @param NL       (I) [optional]
 * @param JType    (I)
 * @param Name     (I)
 * @param CPIVar   (I)
 * @param OpInfo   (I) [optional] information
 * @param Data     (I) [optional] information that doesn't force one to use a string
 * @param IsProlog (I)
 * @param MultiJob (I)
 * @param MCMFlags (I) [optional] BM of enum MCModeEnum
 * @param JobsOut  (O) [optional, must be pre-initialized], output array of type mjob_t *
 */

int MUGenerateSystemJobs(

  const char              *Auth,
  mjob_t                  *OrigJ,
  mnl_t                   *NL,
  enum MSystemJobTypeEnum  JType,
  char const              *Name,
  int                      CPIVar,
  char                    *OpInfo,
  void                    *Data,
  mbool_t                  IsProlog,
  mbool_t                  MultiJob,
  mbitmap_t               *MCMFlags,
  marray_t                *JobsOut)

  {
  int rc;
  mnode_t  *tmpN;

  marray_t *JArray;
  marray_t tmpJArray;

  mulong JDuration = MDEF_SJOBDURATION;

  char tmpBuf[MMAX_BUFFER];

  mbitmap_t tmpL;
  mbitmap_t tmpMCMFlags;

  mjob_t *J;

  enum    MAMJFActionEnum AMFailureAction;

  int     jindex = 0;

  const char *FName = "MUGenerateSystemJobs";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s,%s,%d,OpInfo,Data,Flags,%s,MCMFlags,JobsOut)\n",
    FName,
    (OrigJ != NULL) ? OrigJ->Name : "",
    (NL != NULL) ? "NL" : "",
    MSysJobType[JType],
    (Name != NULL) ? Name : "",
    CPIVar,
    MBool[MultiJob]);

  if ((NL != NULL) && (MNLIsEmpty(NL)))
    {
    MDB(5,fSTRUCT) MLog("ALERT:    empty nodelist specified for %s system job in %s\n",
      MSysJobType[JType],
      FName);

    return(FAILURE);
    }

  if (MCMFlags == NULL)
    {
    MCMFlags = &tmpMCMFlags;
    }

  switch (JType)
    {
    case msjtOSProvision:
    case msjtOSProvision2:

      /* the first stage of OSProvision2 is the same as OSProvision */

      if (MSched.DefProvDuration > 0)
        JDuration = MSched.DefProvDuration;
      else
        JDuration = MDEF_PROVDURATION;

      break;

    case msjtVMMap:

      /* consume VM resources within node, rsv and job structures */

      /* NYI */

      break;

    case msjtVMMigrate:

      /* use MUSubmitVMMigrationJob() to submit a single migration job. */

      return(FAILURE);

      /*NOTREACHED*/

      break;

    case msjtPowerOn:
    case msjtPowerOff:

      if (NL != NULL)
        {
        mpar_t *P = NULL;

        tmpN = MNLReturnNodeAtIndex(NL,0);

        P = &MPar[tmpN->PtIndex];

        JDuration = (JType == msjtPowerOn) ?
          P->NodePowerOnDuration :
          P->NodePowerOffDuration;

        if (bmisset(&MSched.RecordEventList,(JType == msjtPowerOn) ? mrelNodePowerOn : mrelNodePowerOff))
          {
          int tmpNIndex;

          for (tmpNIndex = 0;MNLGetNodeAtIndex(NL,tmpNIndex,&tmpN) == SUCCESS;tmpNIndex++)
            {
            char tmpEvent[MMAX_LINE];

            snprintf(tmpEvent,sizeof(tmpEvent),"powering %s node %s",
              (JType == msjtPowerOn) ? "on" : "off",
              tmpN->Name);

            MOWriteEvent((void *)tmpN,mxoNode,(JType == msjtPowerOn) ? mrelNodePowerOn : mrelNodePowerOff,tmpEvent,NULL,NULL);
            }
          } /* END if (mUBMFLAGISSET(MSched.RecordEventList,(JType == )...)) */
        }

      if (JDuration == 0)
        {
        JDuration = (JType == msjtPowerOn) ?
          MPar[0].NodePowerOnDuration :
          MPar[0].NodePowerOffDuration;
        }

      if (JDuration == 0)
        JDuration = MDEF_POWERDURATION;

      break;

    case msjtReset:

      /* NOTE:  for now, assume all resets take 5 minutes (FIXME) */

      JDuration = 5 * MCONST_MINUTELEN;

      break;

#if 0
    case msjtVMCreate:
    case msjtVMDestroy:
    case msjtVMStorage:

      {
      long LPVal = 0;
      mvm_t  *VM = NULL;
      
      if (NL != NULL)
        {
        mpar_t *tmpP;

        tmpN = MNLReturnNodeAtIndex(NL,0);

        tmpP = &MPar[tmpN->PtIndex];

        LPVal = (JType == msjtVMCreate) ? tmpP->VMCreationDuration :
          tmpP->VMDeletionDuration;
        }

      if (LPVal == 0)
        {
        long GPVal = (JType == msjtVMCreate) ? MPar[0].VMCreationDuration :
          MPar[0].VMDeletionDuration;

        if (GPVal != 0)
          JDuration = GPVal;
        }
      else
        {
        JDuration = LPVal;
        }

      if (JType == msjtVMStorage)
        {
        mvm_req_create_t *VMCR = ((mvm_req_create_t *)Data) + jindex;

        if (VMCR == NULL)
          return(FAILURE);

        if (VMCR->Storage[0] == '\0')
          return(FAILURE);
        }

      /* Check if vmdestroy job has been already created. */

      if (JType == msjtVMDestroy)
        {
        VM = (mvm_t *)Data;

        if ((VM != NULL) && bmisset(&VM->Flags,mvmfDestroySubmitted))
          return(FAILURE);
        }

      }  /* END BLOCK */

      break;

    case msjtVMTracking:

      {
      mvm_req_create_t *VMCR = ((mvm_req_create_t *)Data) + jindex;

      if (VMCR == NULL)
        return(FAILURE);

      if (VMCR->Walltime > 0)
        JDuration = VMCR->Walltime;
      else
        JDuration = MDEF_VMTTTL;
      }

      break;
#endif

    case msjtStorage:

      break;

    default:

      /* invalid job type specified */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (JType) */

  bmset(&tmpL,mjfSystemJob);
  bmset(&tmpL,mjfNoRMStart);

  if (JobsOut == NULL)
    {
    MUArrayListCreate(&tmpJArray,sizeof(mjob_t *),4);
    JArray = &tmpJArray;
    }
  else
    {
    JArray = JobsOut;
    }

  if ((NL != NULL) || ((OrigJ != NULL) && (!MNLIsEmpty(&OrigJ->ReqHList))))
    {
    rc = MUNLCreateJobs(
      Auth,
      (NL == NULL) ? &OrigJ->ReqHList : NL,
      JType,
      JDuration,
      Name,
      &tmpL,
      JArray,  /* O */
      MultiJob);
    }
  else
    {
    rc = MUTCCreateJobs(
      Auth,
      OrigJ->Request.TC,
      JType,
      JDuration,
      Name,
      &tmpL,
      JArray,   /* O */
      MultiJob);
    }

  bmclear(&tmpL);

  /* process all jobs submitted */

  for (jindex = 0;jindex < JArray->NumItems;jindex++)
    {
    J = *(mjob_t **)MUArrayListGet(JArray,jindex);

    if (J == NULL)
      continue;

    if (MJobCreateSystemData(J) == FAILURE)
      continue;

    bmcopy(&J->System->MCMFlags,MCMFlags);

    /* set job action */

    J->System->JobType = JType;

    J->System->WCLimit = JDuration;

    /* perform global config applicable to all system jobs */

    /* NOTE:  make system jobs shared to allow VMMap+VMCreate or VMMap+
              VMMigrate, etc */

    J->Req[0]->NAccessPolicy = mnacShared;

    /* perform system job type-specific config */

    switch (JType)
      {
      case msjtOSProvision:
      case msjtOSProvision2:

        {
        mstring_t MString(MMAX_LINE);
        mreq_t *RQ;

        MSysJobAddQOSCredForProvision(J);

        /* build up HOSTLIST for Action string */

        MStringAppend(&MString,"node:");

        if (OrigJ != NULL)
          {
          int nindex;
          int nodecount;

          nodecount = 0;

          /* when OrigJ is specified, only provision nodes that actually need to be provisioned */
          /* UPDATE: this is now handled on the trigger level */

          for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&tmpN) == SUCCESS;nindex++)
            {
/*
            if (((CPIVar > 0) && (NL[nindex].N->ActiveOS != CPIVar)) ||
                (NL[nindex].N->PowerSelectState == mpowOff))
              {
*/
              if (nodecount++ > 0)
                MStringAppend(&MString,",");

              MStringAppend(&MString,tmpN->Name);

              MDB(5,fSTRUCT) MLog("INFO:     provisioning node %s for job %s with OS %s (activeos: %s)\n",
                tmpN->Name,
                J->Name,
                MAList[meOpsys][CPIVar],
                MAList[meOpsys][tmpN->ActiveOS]);
/*
              }
*/
            } /* END for (nindex) */
          }
        else
          {
          MStringAppend(&MString,"$(HOSTLIST)");
          }

        MStringAppendF(&MString,":modify:os=%s",
          MAList[meOpsys][CPIVar]);

        MUStrDup(&J->System->Action,MString.c_str());

        if (JType == msjtOSProvision)
          J->System->CompletionPolicy = mjcpOSProvision;
        else
          J->System->CompletionPolicy = mjcpOSProvision2;

        J->Req[0]->Opsys = CPIVar;
        J->System->ICPVar1          = CPIVar;
        MUStrDup(&J->System->SCPVar1,MAList[meOpsys][CPIVar]);
        J->System->VCPVar1          = Data;     /* partition to manage DynPendingNL on */

        if ((JType == msjtOSProvision2) && (OpInfo != NULL) && (OpInfo[0] != '\0'))
          {
          MUStrDup(&J->System->SCPVar1,OpInfo);
          }

        J->SystemPrio = 1;

        RQ = J->Req[0];

        if (RQ != NULL)
          {
          /* provision resources which will complete compute jobs first */

          RQ->NAllocPolicy = mnalFirstAvailable;

          if (OrigJ != NULL)
            RQ->Opsys = OrigJ->Req[0]->Opsys;

          /* remove proc restriction for provision jobs */
          /* why would we do that?  */
#ifdef MNOT
          RQ->DRes.Procs = 0;
#endif /* MNOT */           
          }  /* END if (RQ != NULL) */

        bmset(&J->IFlags,mjifReserveAlways);

        if (bmisset(MCMFlags,mcmForce))
          bmset(&J->IFlags,mjifRunAlways);
        }  /* END BLOCK (case mjstOSProvision,mjstOSProvision2) */

        break;

      case msjtPowerOn:
      case msjtPowerOff:

        {
        mnode_t *N;

        mreq_t *RQ;

        char const *PowerType;
        enum MJobCompletionPolicyEnum CompletionPolicy;
        mpoweroff_req_t *PReq = (mpoweroff_req_t *)Data;
        mln_t *DependLink = (PReq == NULL) ? NULL : ((mln_t **)PReq->Dependencies.Array)[jindex];

        if (JType == msjtPowerOn)
          {
          PowerType = "on";
          CompletionPolicy = mjcpPowerOn;
          }
        else
          {
          PowerType = "off";
          CompletionPolicy = mjcpPowerOff;
          }

        while (DependLink != NULL)
          {
          MJobSetDependency(J,mdJobSuccessfulCompletion,DependLink->Name,NULL,0,FALSE,NULL);
          DependLink = DependLink->Next;
          }

        if (bmisset(MCMFlags,mcmForce))
          bmset(&J->RsvAccessBM,mjapAllowAll);

        snprintf(tmpBuf,sizeof(tmpBuf),"node:$(HOSTLIST):modify:power=%s",
          PowerType);

        MUStrDup(&J->System->Action,tmpBuf);

        J->System->CompletionPolicy = CompletionPolicy;

        J->SystemPrio               = 1;

        /* force power on job to run on physical machine */

        J->VMUsagePolicy = mvmupPMOnly;

        RQ = J->Req[0];

        if (RQ != NULL)
          {
          /* reset target resources */

          RQ->NAllocPolicy = mnalFirstAvailable;

#if 0     /* NOTE: should not be needed as MNLSetAttr() can compact the node range */

          RQ->NAccessPolicy = mnacSingleJob;
#endif

          /* remove proc restriction on poweroff jobs */
          /* why would we do that?  */
#ifdef MNOT
          if (JType == msjtPowerOff) 
            RQ->DRes.Procs = 0;
#endif /* MNOT */           
          }  /* END if (RQ != NULL) */

        /* power on jobs should run immediately, power off jobs should
           immediately reserve resources */

        if (JType == msjtPowerOn)
          bmset(&J->IFlags,mjifRunAlways);
        else
          bmset(&J->IFlags,mjifReserveAlways);

#ifdef MFORCE
        bmset(&J->IFlags,mjifRunAlways);
#endif /* MFORCE */

        if (OpInfo != NULL)
          {
          mjob_t *PJ;

          /* OpInfo contains pointer to parent power job which is reserving resources */

          PJ = (mjob_t *)OpInfo;

          /* NOTE:  AllowAllNonJob is a BIG STICK - this will allow the power-on
                    job to run in spite of maintenance reservations etc.  This
                    should be ok because Moab will not create the job and lock
                    it to a node unless the job should be able to run. */

          bmset(&J->RsvAccessBM,mjapAllowAllNonJob);

          /* point void data to power job to indicate this is 'green' based action */

          J->System->VCPVar1 = OpInfo;

          MUAddAction(&PJ->TemplateExtensions->Action,J->Name,J);
          }  /* END if (OpInfo != NULL) */

        N = MNLReturnNodeAtIndex(&J->ReqHList,0);
        MUAddAction(&N->Action,J->Name,J);
        }  /* END BLOCK (case mjstPowerOn,msjtPowerOff) */

        break;

      case msjtReset:

        {
        mreq_t *RQ;

        snprintf(tmpBuf,sizeof(tmpBuf),"node:$(HOSTLIST):modify:power=reset");

        MUStrDup(&J->System->Action,tmpBuf);

        J->System->CompletionPolicy = mjcpOSProvision;

        J->SystemPrio               = 1;

        RQ = J->Req[0];

        if (RQ != NULL)
          {
          /* reset target resources */

          RQ->NAllocPolicy = mnalFirstAvailable;

          RQ->NAccessPolicy = mnacSingleJob;
          }  /* END if (RQ != NULL) */
        }  /* END BLOCK (case mjstReset) */

        break;

      case msjtStorage:

        {
        mtrig_t *T;

        if (J->System->TList == NULL)
          {
          if ((J->System->TList = (marray_t *)MUCalloc(1,sizeof(marray_t))) == NULL)
            {
            return(FAILURE);
            }

          MUArrayListCreate(J->System->TList,sizeof(mtrig_t *),10);
          }

        T = (mtrig_t *)MUCalloc(1,sizeof(mtrig_t));

        MUArrayListAppendPtr(J->System->TList,T);
 
        assert(OpInfo != NULL);
        MTrigCreate(T,mttModify,mtatExec,OpInfo);
        J->System->CompletionPolicy = mjcpStorage;
        J->System->ModifyRMJob = (mbool_t)CPIVar;
        T->Timeout = J->SpecWCLimit[0];

        bmset(&J->IFlags,mjifReserveAlways);
        bmset(&J->IFlags,mjifRunAlways);
        }  /* END case msjtStorage */

        break;

#if 0
      case msjtVMStorage:

        {
        mvm_t *VM = NULL;
        mvm_req_create_t *VMCR = ((mvm_req_create_t *)Data) + jindex;

        J->System->CompletionPolicy = mjcpVMStorage;

        //bmset(&J->SpecFlags,mjfNoResources);
        bmset(&J->IFlags,mjifReserveAlways);
        bmset(&J->IFlags,mjifRunAlways);

        if (MVMAdd(VMCR->VMID,&VM) == SUCCESS)
          {
          /* Save the VM ID for event tracking. */

          MUStrDup(&J->System->VM,VM->VMID);

          /* Storage mounts (locations) */

          if (VMCR->Storage[0] != '\0')
            {
            /* Parse original storage string into the resulting structures */

            char *STok1;
            char *STok2;
            char *SType;
            char *SVal;
            char *SPtr;
            char *SpecMountOpts;

            int   MCounter = 0;

            SPtr = MUStrTok(VMCR->Storage,"%",&STok1);
            while (SPtr != NULL)
              {
              int Size;
              mvm_storage_t *StorageMount;
              char StoreName[MMAX_LINE];

              SVal = NULL;
              SType = MUStrTok(SPtr,":",&STok2);
              SVal = MUStrTok(NULL,"@",&STok2);

              StorageMount = (mvm_storage_t *)MUCalloc(1,sizeof(mvm_storage_t));

              if (SVal != NULL)
                {
                Size = (int)strtol(SVal,NULL,10);

                if (Size <= 0)
                  {
                  /* Other storage mounts will be freed by the VM */
                  MUFree((char **)&StorageMount);

                  return(FAILURE);
                  }
                }
              else
                {
                Size = 1;
                }

              snprintf(StoreName,sizeof(StoreName),"Storage%d",
                MCounter++);

              MUStrCpy(StorageMount->Name,StoreName,sizeof(StorageMount->Name));
              MUStrCpy(StorageMount->Type,SType,sizeof(StorageMount->Type));
              StorageMount->Size = Size;

              SVal = MUStrTok(NULL,"#",&STok2);
              if (SVal != NULL)
                {
                /* User also specified the mount point */

                MUStrCpy(StorageMount->MountPoint,SVal,sizeof(StorageMount->MountPoint));
                }

              /* check to see if mount options have been specified */

              SpecMountOpts = MUStrTok(NULL,"#",&STok2);
              if (SpecMountOpts != NULL)
                {
                /* User also specified mount point options */

                MUStrCpy(StorageMount->MountOpts,SpecMountOpts,sizeof(StorageMount->MountOpts));
                }

              /* Location will be filled in by the resulting triggers */
              StorageMount->Location[0] = '\0';

              MULLAdd(&VM->Storage,StoreName,(void *)StorageMount,NULL,NULL);

              SPtr = MUStrTok(NULL,"%",&STok1);
              } /* END while (SPtr != NULL) */
            } /* END if (VMCR->Storage[0] != '\0') */
          } /* END if (MVMAdd(VMCR->VMID,&VM) == SUCCESS) */
        else
          {
          return(FAILURE);
          }

        /* set the proxy user on the storage job from the vm tracking job */

        if ((VM->TrackingJ != NULL) &&
            (VM->TrackingJ->System != NULL) &&
            (VM->TrackingJ->System->ProxyUser != NULL) &&
            (J->System->ProxyUser == NULL))
          {
          MUStrDup(&J->System->ProxyUser,VM->TrackingJ->System->ProxyUser);
          }

        } /* END BLOCK (case msjtVMStorage) */

        break;

      case msjtVMCreate:

        {
        mreq_t *RQ;
        mvm_t *VM;
        char tmpVar[MMAX_LINE];
        mvm_req_create_t *VMCR = ((mvm_req_create_t *)Data) + jindex;

        assert(VMCR != NULL);

        if (MJobPopulateSystemJobData(J,msjtVMCreate,VMCR) == FAILURE)
          {
          return(FAILURE);
          }

        RQ = J->Req[0];

        if (RQ != NULL)
          {
          MCResClear(&RQ->DRes);

          MCResPlus(&RQ->DRes,&VMCR->CRes);

          RQ->DRes.Procs = MAX(1,RQ->DRes.Procs);

          RQ->NAllocPolicy = mnalFirstAvailable;

          RQ->NAccessPolicy = mnacShared;

          if (!bmisclear(&VMCR->FBM))
            {
            bmor(&RQ->ReqFBM,&VMCR->FBM);
            }
          }  /* END if (RQ != NULL) */

        /* allow all VM create jobs to reserve resources */

        bmset(&J->IFlags,mjifReserveAlways);

        if (bmisset(MCMFlags,mcmForce))
          bmset(&J->IFlags,mjifRunAlways);

        /* need to pre-create VM and specify whether or not it is a consumer */

        if (MVMAdd(VMCR->VMID,&VM) == SUCCESS)
          {
          mnode_t *N = (NL == NULL) ? MNLReturnNodeAtIndex(&OrigJ->ReqHList,0) : MNLReturnNodeAtIndex(NL,0);

          if (N != NULL)
            {
            MVMSetParent(VM,N);
            bmset(&VM->IFlags,mvmifUnreported);
            }

          VM->CreateJob = J;

          /* Copy the job variables into the VM variables */

          if ((OrigJ != NULL) && (OrigJ->Variables.Table != NULL))
            {
            MUHTCopy(&VM->Variables,&OrigJ->Variables);
            }

          if (VMCR->IsSovereign == TRUE)
            bmset(&VM->Flags,mvmfSovereign);
          else
            bmunset(&VM->Flags,mvmfSovereign);

          if (VMCR->IsOneTimeUse == TRUE)
            bmset(&VM->Flags,mvmfOneTimeUse);
          else
            bmunset(&VM->Flags,mvmfOneTimeUse);

          if (VMCR->OwnerJob != NULL)
            MUStrDup(&VM->OwnerJob,VMCR->OwnerJob->Name);

          if (VMCR->Vars[0] != '\0')
            {
            MVMSetAttr(VM,mvmaVariables,VMCR->Vars,mdfString,mSet);
            }

          /* Storage mounts (reservations) */

          VM->StorageRsvs = NULL;

          if (VMCR->StorageRsvs != NULL)
            {
            VM->StorageRsvs = VMCR->StorageRsvs;
            }

          /* Set VM time to live */

          if (VMCR->Walltime > 0) 
            {
            VM->SpecifiedTTL = VMCR->Walltime;
            VM->EffectiveTTL = VM->SpecifiedTTL;
            }
          else
            {
            VM->EffectiveTTL = MDEF_VMTTTL;
            }

          if (VMCR->Aliases[0] != '\0')
            {
            MVMSetAttr(VM,mvmaAlias,VMCR->Aliases,mdfString,mSet);
            }

          if (VMCR->Triggers[0] != '\0')
            {
            /* Parse triggers (comma separated) */

            char *TPtr;
            char *TTok;

            /* Triggers are comma separated internally, but specified as
                mvmctl -m trigger=x&y,trigger=a&b,hypervisor=node,image=os 

               Saved format will be x&y,a&b */

            TPtr = MUStrTok(VMCR->Triggers,",",&TTok);

            while (TPtr != NULL)
              {
              MVMSetAttr(VM,mvmaTrigger,TPtr,mdfString,mAdd);

              TPtr = MUStrTok(NULL,",",&TTok);
              }
            }

          if (VMCR->TrackingJ[0] != '\0')
            {
            /* Point VM back to VMTracking job */

            MJobFind(VMCR->TrackingJ,&VM->TrackingJ,mjsmExtended);
            }

          if (VMCR->VC[0] != '\0')
            {
            mvc_t *VC;

            /* VC ownership was checked at VM submit time */

            /* Note: it is possible that a user could destroy the VC
                      and a different user could create a VC with the
                      same name.  This is a corner case and not worth
                      fixing right now. */

            if (MVCFind(VMCR->VC,&VC) == SUCCESS)
              {
              MVCAddObject(VC,(void *)VM,mxoxVM);
              }
            }

          /* Set VMID variable */

          snprintf(tmpVar,MMAX_LINE,"VMID=%s",
            VMCR->VMID);

          MJobSetAttr(J,mjaVariables,(void **)tmpVar,mdfString,mAdd);

          /* set the proxy user on the create job from the vm tracking job */

          if ((VM->TrackingJ != NULL) &&
              (VM->TrackingJ->System != NULL) &&
              (VM->TrackingJ->System->ProxyUser != NULL) &&
              (J->System->ProxyUser == NULL))
            {
            MUStrDup(&J->System->ProxyUser,VM->TrackingJ->System->ProxyUser);
            }

          /* VM inherits variables from Tracking Job*/

          if ((VM->TrackingJ != NULL) && (VM->TrackingJ->Variables.Table))
            {
            MUHTCopy(&VM->Variables,&VM->TrackingJ->Variables);
            }
          } /* END if (MVMAdd(VMCR->VMID,&VM) == SUCCESS) */
        else
          {
          return(FAILURE);
          }

        MUAddAction(&VM->Action,J->Name,J);
        }  /* END BLOCK (case msjtVMCreate) */

        break;

      case msjtVMDestroy:

        {
        /* currently only support creating one job that destroys one VM */

        mreq_t *RQ;
        char    tmpVar[MMAX_LINE];
        mvm_t  *VM = (mvm_t *)Data;

        if (VM == NULL)
          {
          assert(OpInfo != NULL);

          MVMFind(OpInfo,&VM);

          if (VM == NULL)
            {
            return(FAILURE);
            }
          }

				/* Set destroy submitted to prevent double destroy jobs. */

        bmset(&VM->Flags,mvmfDestroySubmitted);

        J->System->CompletionPolicy = mjcpVMDestroy;

        MUStrDup(&J->System->VM,VM->VMID);
        MUStrDup(&J->System->VMSourceNode,MNLGetNodeName(&J->NodeList,0,NULL));

        /* set the proxy user on the destroy job from the vm tracking job */

        if ((VM->TrackingJ != NULL) &&
            (VM->TrackingJ->System != NULL) &&
            (VM->TrackingJ->System->ProxyUser != NULL) &&
            (J->System->ProxyUser == NULL))
          {
          MUStrDup(&J->System->ProxyUser,VM->TrackingJ->System->ProxyUser);
          }

        /* Save the name of the VM tracking job so it can be check-pointed. */

        if (VM->TrackingJ != NULL)
          MUStrDup(&J->System->VMJobToStart,VM->TrackingJ->Name);

        RQ = J->Req[0];

        if (RQ != NULL)
          {
          RQ->NAllocPolicy = mnalFirstAvailable;

          RQ->NAccessPolicy = mnacShared;

          MCResClear(&RQ->DRes);

          /* VMDestroy system job does not need to consume resources */

          /* MCResPlus(&RQ->DRes,&VM->CRes); */

          /* RQ->DRes.Procs = MAX(1,RQ->DRes.Procs); */

          bmset(&J->SpecFlags,mjfNoResources);
          bmset(&J->Flags,mjfNoResources);
          bmset(&J->Flags,mjfIgnState);
          bmset(&J->IFlags,mjifIgnRsv);
          bmset(&J->IFlags,mjifRunAlways);

          }  /* END if (RQ != NULL) */

        /* this is too dangerous--we should NEVER force a destroy
         * job to run!
         *
        if (bmisset(MCMFlags,mcmForce) ||
           ((VM != NULL) && (bmisset(&VM->Flags,mvmfSovereign))))
          {
          bmset(&J->RsvAccessBM,mjapAllowAll);
          bmset(&J->IFlags,mjifRunAlways);
          }
        */

        if (VM->Storage != NULL)
          {
          mln_t *StorePtr = NULL;
          mrm_t *StoreRM = NULL;
          mstring_t TrigAction(MMAX_LINE);

          while (MULLIterate(VM->Storage,&StorePtr) == SUCCESS)
            {
            marray_t  TList;

            mtrig_t *T = NULL;

            mvm_storage_t *StoreMount = (mvm_storage_t *)StorePtr->Ptr;

            /* Find RM -> check for each StoreMount */

            if ((MNodeGetResourceRM(NULL,StoreMount->Hostname,mrmrtStorage,&StoreRM) == FAILURE) ||
                (StoreRM == NULL))
              {
              /* Could not find storage RM */

              continue;
              }

            TrigAction.clear();

            if (StoreMount->Location[0] == '\0')
              continue;

            MStringSetF(&TrigAction,"atype=exec,etype=start,action=\"%s destroystorage STORAGESERVER=%s STORAGEID=%s HOSTLIST=%s\"",
              StoreRM->ND.Path[mrmSystemModify],
              StoreMount->Hostname, /*StoreMount->R->NL[0].N->Name,*/
              StoreMount->Location,
              VM->VMID);
             
            MUArrayListCreate(&TList,sizeof(mtrig_t *),1);

            if (MTrigLoadString(&J->Triggers,TrigAction.c_str(),TRUE,FALSE,mxoJob,J->Name,&TList,NULL) == SUCCESS)
              {
              T = (mtrig_t *)MUArrayListGetPtr(&TList,0);

              /* Set the trigger env */

              if ((StoreRM->Env != NULL) && (StoreRM->Env[0] != '\0'))
                MUStrDup(&T->Env,StoreRM->Env);

              MTrigInitialize(T);

              /* Set the trigger checkpoint flag to force checkpoint. */

              bmset(&T->SFlags,mtfCheckpoint);
              }

            MUArrayListFree(&TList);
            }    /* while (MULLIterate(VM->Storage,&StorePtr) == SUCCESS) */
          }    /* END if (VM->Storage != NULL) */

        snprintf(tmpVar,MMAX_LINE,"VMID=%s",
          VM->VMID);

        MJobSetAttr(J,mjaVariables,(void **)tmpVar,mdfString,mAdd);
        }    /* END BLOCK (msjtVMDestroy) */

        break;
#endif

      case msjtVMMap:

        {
        long tmpWC = MCONST_EFFINF;
        mvm_t *V;

        V = (mvm_t *)Data;

        if (V == NULL)
          {
          return(FAILURE);
          }

        if (MJobPopulateSystemJobData(J,msjtVMMap,Data) == FAILURE)
          {
          return(FAILURE);
          }

        J->System->CompletionPolicy = mjcpVMMap;

        MUStrDup(&J->System->SCPVar1,V->VMID);

        MJobSetAttr(J,mjaReqAWDuration,(void **)&tmpWC,mdfLong,mSet);

        bmset(&J->IFlags,mjifRunAlways);
        }  /* END BLOCK (case msjtVMMap) */

        break;

      case msjtVMMigrate:

        {
        /* NYI, currently use MUSubmitVMMigrationJob() to submit a single migration job. */
        }

        break;

#if 0
      case msjtVMTracking:

        {
        mreq_t *RQ;
        char tmpVar[MMAX_LINE];
        mvm_req_create_t *VMCR = ((mvm_req_create_t *)Data) + jindex;

        if ((VMCR == NULL) || (VMCR->VMID[0] == '\0'))
          break;

        J->Credential.U = VMCR->U;

        J->System->CompletionPolicy = mjcpVMTracking;

        RQ = J->Req[0];

        if (RQ != NULL)
          {
          /* There will already be one proc from submitting the job -
             set to 0 so MCResPlus below won't give too many */

          RQ->DRes.Procs = 0;

          MCResPlus(&RQ->DRes,&VMCR->CRes);
          RQ->DRes.Procs = MAX(1,RQ->DRes.Procs);
          RQ->NAllocPolicy = MPar[0].NAllocPolicy;
          RQ->NAccessPolicy = mnacShared;

          RQ->Opsys = VMCR->OSIndex;

          if (!bmisclear(&VMCR->FBM))
            {
            bmor(&RQ->ReqFBM,&VMCR->FBM);
            }

          if (VMCR->N != NULL)
            {
            /* specific hypervisor requested */

            bmset(&J->IFlags,mjifHostList);
            J->ReqHLMode = mhlmNONE;

            MJobUpdateFlags(J);

            MNLClear(&J->ReqHList);

            MNLSetNodeAtIndex(&J->ReqHList,0,VMCR->N);
            MNLSetTCAtIndex(&J->ReqHList,0,1);
            MNLTerminateAtIndex(&J->ReqHList,1);
            }  /* END if (VMCR->N != NULL) */

          if (VMCR->Storage[0] !='\0')
            {
            /* Storage specified, need to add to job so that it will be scheduled as well */

            char  tmpStorage[MMAX_LINE];
            char *GResInst = NULL;
            char *Tok1 = NULL;

            MUStrCpy(tmpStorage,VMCR->Storage,sizeof(tmpStorage));

            GResInst = MUStrTok(tmpStorage,"%",&Tok1);

            while (GResInst != NULL)
              {
              char *GPtr = NULL;
              char *GTokPtr = NULL;
              char *GPtrCount = NULL;
              int GCount;
              int GIndex;
              int RIndex;
              mreq_t *StoreReq = NULL;

              GPtr = MUStrTok(GResInst,":",&GTokPtr);
              GPtrCount = MUStrTok(NULL,"@",&GTokPtr);

              if (GPtrCount == NULL)
                {
                MJobSetState(J,mjsCompleted);
                return(FAILURE);
                }
              GCount = strtol(GPtrCount,NULL,10);
              

              if (GPtr != NULL)
                {
                /* Verify the storage node exists by using the GRes. */

                GIndex = MUMAGetIndex(meGRes,GPtr,mVerify);
                if (GIndex <= 0)
                  {
                  MJobSetState(J,mjsCompleted);
                  return(FAILURE);
                  }

                /* Find the req (if there is one) for specified storage type */

                for (RIndex = 1;RIndex < MMAX_REQ_PER_JOB;RIndex++)
                  {
                  if (J->Req[RIndex] == NULL)
                    {
                    /* Req for requested storage not found, create one */

                    MReqCreate(J,NULL,NULL,TRUE);
                    J->Req[RIndex]->TaskCount = 1;
                    J->Req[RIndex]->TaskRequestList[0] = 1;
                    J->Req[RIndex]->TaskRequestList[1] = 1;

                    StoreReq = J->Req[RIndex];

                    break;
                    }
                  else if (MSNLGetIndexCount(&J->Req[RIndex]->DRes.GenericRes,GIndex) > 0)
                    {
                    /* Found a previous req for the same storage type */

                    StoreReq = J->Req[RIndex];

                    break;
                    }
                  }

                if (StoreReq == NULL)
                  {
                  return(FAILURE);
                  }

                if (GIndex > 0)
                  {
                  MSNLSetCount(&StoreReq->DRes.GenericRes,GIndex,MSNLGetIndexCount(&StoreReq->DRes.GenericRes,GIndex) + GCount);
                  }
                }

              GResInst = MUStrTok(NULL,"%",&Tok1);
              }  /* END while (GResInst != NULL) */
            }  /* END if (VMCR->Storage[0] !='\0') */
          }  /* END if (RQ != NULL) */

        if (J->VMCreateRequest == NULL)
          {
          J->VMCreateRequest = (mvm_req_create_t *)MUCalloc(1,sizeof(mvm_req_create_t));
          }

        memcpy(J->VMCreateRequest,VMCR,sizeof(mvm_req_create_t));

        J->VMUsagePolicy = mvmupVMCreate;

        /* Point the VM back to this job */

        MUStrCpy(J->VMCreateRequest->TrackingJ,J->Name,sizeof(J->VMCreateRequest->TrackingJ));

        bmset(&J->SpecFlags,mjfSystemJob);
        bmset(&J->Flags,mjfSystemJob);
        bmset(&J->SpecFlags,mjfNoRMStart);
        bmset(&J->Flags,mjfNoRMStart);
        bmset(&J->SpecFlags,mjfMeta);      /* Job reserves but does not consume resources */
        bmset(&J->Flags,mjfMeta);      /* Job reserves but does not consume resources */

        /* Set VMID variable so that VMID's won't be duplicated */

        snprintf(tmpVar,MMAX_LINE,"VMID=%s",
          VMCR->VMID);

        MJobSetAttr(J,mjaVariables,(void **)tmpVar,mdfString,mAdd);

        MULLAdd(&MSched.VMTrackingSubmitted,J->Name,NULL,NULL,NULL);

        /* Need to transition the job again because of the user */

        MJobTransition(J,FALSE,TRUE);
        }  /* END BLOCK msjtVMTracking */

        break;
#endif

      default:

        /* NO-OP */

        break;
      }  /* END switch (JType) */

    if ((IsProlog == TRUE) &&
        (J->System != NULL) &&
        (OrigJ != NULL))
      {
      bmset(&J->System->CFlags,msjcaJobStart);
      }

    /* NAMI billing - Create */

    if ((MAM[0].ValidateJobSubmission == TRUE) && (MAMHandleCreate(&MAM[0],
          (void *)J,
          mxoJob,
          &AMFailureAction,
          NULL,
          NULL) == FAILURE))
      {
      /* do not apply failure policies to system jobs */

      MDB(3,fSTRUCT) MLog("WARNING:  Unable to register job creation with accounting manager for job '%s', taking action '%s'\n",
        J->Name,
        MAMJFActionType[AMFailureAction]);

      if ((!bmisset(&J->IFlags,mjifRunAlways)) &&
          (!bmisset(&J->IFlags,mjifRunNow)))
        {
        /* Failure action */

        switch (AMFailureAction)
          {
          case mamjfaCancel:

            MJobCancel(J,"Unable to register job creation with accounting manager",FALSE,NULL,NULL);

            return(FAILURE);

            /* NOTREACHED */

            break;

          case mamjfaHold:

            MJobSetHold(J,mhSystem,MSched.DeferTime,mhrAMFailure,NULL);

            return(FAILURE);

            /* NOTREACHED */

            break;

          default:

            /* NO-OP, ignore */

            break;
          }  /* END switch (AMFailureAction) */
        }  /* END if ((!bmisset(&J->IFlags,mjifRunAlways)) && ...) */
      }  /* if ((MAM[0].ValidateJobSubmission == TRUE) && (MAMHandleCreate */
    }  /* END for (jindex) */

  if (JArray == &tmpJArray)
    MUArrayListFree(JArray);

  return(rc);
  }  /* END MUGenerateSystemJobs() */
/* END MUGenerateSystemJobs.c */
