/* HEADER */

/**
 * @file MVMStorage.c
 * 
 * Contains various functions for VM Storage
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


#if 0

/**
 * Takes the VMCR object and uses the Storage on it to generate storage for a VM.
 *  Called at VM create submit time (only for msub requests--mvmctl requests are handled
 *  by MVMCreateStorageJob()).
 *
 *
 * @see MUIVMCreate - parent
 *
 * @param VMCR         (I) - The VM create request info
 * @param StoreRsv     (O) - Returned reservation that holds all of the storage gres
 * @param StoreJ       (O) - Returned Storage job with triggers attached
 * @param tmpLine      (O) [optional] - Error message string
 * @param tmpLineSize  (I) - Size of tmpLine
 * @param Auth         (I) [optional] - Name of user who requested VM
 *
 */

int MVMCreateStorage(

  mvm_req_create_t *VMCR,        /* I */
  mrsv_t          **StoreRsv,    /* O */
  mjob_t          **StoreJ,      /* O */
  char             *tmpLine,     /* O (optional) */
  int               tmpLineSize, /* I */
  char             *Auth)        /* I (optional) */

  {
  long    CalculatedDuration; /* Duration of the VM ttl + 
            VM setup/takedown time + Storage mount setup time */
  int rc;

  mrsv_t  *R = NULL;

  mxml_t  *AE = NULL;
  mxml_t  *RE = NULL;
  mxml_t  *PE = NULL;
  mxml_t  *CE = NULL;

  char  StoreEMsg[MMAX_LINE];
  char  AdminName[MMAX_NAME];
  char  TmpStorage[MMAX_LINE];
  mrsv_t  StorageRsv;
  mrsv_t *tmpStoreRsv;
  marray_t StorageArray;

  char *Tok1;
  char *GResInst;

  mrange_t RRange[MMAX_REQ_PER_JOB];
  int TmpRQD;

  int TID;
  char TIDStr[MMAX_NAME];
  mbool_t TIDFound = FALSE;

  int CTok;

  long tmpStartTime;
  long tmpEndTime;

  char HypVLAN[MMAX_LINE];

  mbool_t HasVLAN = FALSE;

  mbitmap_t StoreFlags;

  mrsv_t *AdvRsvR = NULL;
  mjob_t *StorageJ = NULL;

  if ((StoreRsv == NULL) || (StoreJ == NULL))
    return(FAILURE);

  if (tmpLine != NULL)
    tmpLine[0] = '\0';

  bmset(&StoreFlags,mcmVerbose);
  bmset(&StoreFlags,mcmFuture);
  bmset(&StoreFlags,mcmUseTID);
  bmset(&StoreFlags,mcmSummary);

  /* Set the walltime */

  if (VMCR->Walltime > 0)
    {
    /* Add an hour for storage setup/teardown and VM setup/teardown */

    CalculatedDuration = VMCR->Walltime + MCONST_HOURLEN;
    }
  else
    {
    CalculatedDuration = MDEF_VMTTTL + MCONST_HOURLEN;
    }

  /* Set up XML to be passed to MClusterShowARes */

  mstring_t  tmpXML(MMAX_LINE);

  MStringSet(&tmpXML,"<Request action=\"query\"");
  MStringAppend(&tmpXML," flags=\"EXCLUSIVE,summary,future,tid,policy\"");
  MStringAppend(&tmpXML," option=\"intersection,noaggregate\"><Object>");
  MStringAppend(&tmpXML,"cluster</Object>");

  HasVLAN = MNodeGetVLAN(VMCR->N, HypVLAN);

  /* Add in gres to XML */

  MUStrCpy(TmpStorage,VMCR->Storage,sizeof(TmpStorage));

  /* THIS APPROACH WORKS FOR GRES ON MULTIPLE NODES (1 TYPE PER NODE) */

  GResInst = MUStrTok(TmpStorage,"%",&Tok1);

  while (GResInst != NULL)
    {
    MStringAppendF(&tmpXML,"<Set><Where name=\"GRES\">%s</Where>",
      GResInst);

    MStringAppendF(&tmpXML,"<Where name=\"duration\">%ld</Where>",
      CalculatedDuration);

    if (HasVLAN)
      {
      MStringAppendF(&tmpXML,"<Where name=\"nodefeature\">%s</Where>",
        HypVLAN);
      }

    /* vmusage so that gres will be set in MJobGetFeasibleNodeResources */
    MStringAppend(&tmpXML,"<Where name=\"vmusage\">requirepm</Where>");
    MStringAppend(&tmpXML,"<Where name=\"policylevel\">off</Where></Set>");

    GResInst = MUStrTok(NULL,"%",&Tok1);
    }

  /* THIS APPROACH WORKS FOR ALL GRES ON ONE NODE */

/*
  MUSNPrintF(&BPtr,&BSpace,"<Set><Where name=\"mintasks\">1@");
  GResInst = MUStrTok(TmpStorage,"%",&Tok1);

  while (GResInst != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"GRES=%s",
      GResInst);

    GResInst = MUStrTok(NULL,"%",&Tok1);

    if (GResInst != NULL)
      MUSNPrintF(&BPtr,&BSpace,"+");
    }

  MUSNPrintF(&BPtr,&BSpace,"</Where><Where name=\"policylevel\">off</Where><Where name=\"duration\">2678400</Where></Set>");
*/

  MStringAppend(&tmpXML,"</Request>");

  /* Create output XML structure */
  MXMLCreateE(&AE,(char *)MSON[msonData]);
  MXMLAddChild(AE,"Object","cluster",NULL);

  /* Convert XML string to XML structure */
  MXMLFromString(&RE,tmpXML.c_str(),NULL,NULL);

  /* We want MClusterShowARes to give us just the first option */
  TmpRQD = MSched.ResourceQueryDepth;
  MSched.ResourceQueryDepth = 1;

  if (MClusterShowARes(
        MSched.Admin[1].UName[0],
        mwpXML,          /* response format */
        &StoreFlags,     /* query flags */
        RRange,          /* I (modified) */
        mwpXML,          /* source format */
        (void *)RE,      /* source data */
        (void **)&AE,    /* response data */
        0,
        0,
        NULL,
        StoreEMsg) == FAILURE)
    {
    MSched.ResourceQueryDepth = TmpRQD;

    if (tmpLine != NULL)
      snprintf(tmpLine,tmpLineSize,"ERROR:  could not find requested storage at any time\n");

    return(FAILURE);
    }

  MSched.ResourceQueryDepth = TmpRQD;

  MDB(8,fSTRUCT)
    {
    mstring_t Result(MMAX_LINE);

    if (MXMLToMString(AE,&Result,NULL,TRUE) == SUCCESS)
      {
      MLog("INFO:     result from 'mshow -a' for VM Storage request -- %s\n",Result.c_str());
      }
    }

  /* Response for the tid will be the second "par" child */

  CTok = -1;

  if ((MXMLGetChild(AE,(char *)MXO[mxoPar],&CTok,&PE) == FAILURE) ||
      (MXMLGetChild(AE,(char *)MXO[mxoPar],&CTok,&PE) == FAILURE))
    {
    MXMLDestroyE(&AE);

    if (tmpLine != NULL)
      snprintf(tmpLine,tmpLineSize,"ERROR:  could not find requested storage (no valid results)\n");

    return(FAILURE);
    }

  MULLCreate(&VMCR->StorageRsvs);

  snprintf(AdminName,sizeof(AdminName),"USER==%s",
      MSched.Admin[1].UName[0]);

  /* Get the TID */
  CTok = -1;

  while (MXMLGetChild(PE,"range",&CTok,&CE) == SUCCESS)
    {
    R = NULL;
    TIDStr[0] = '\0';

    if (MXMLGetAttr(CE,"tid",NULL,TIDStr,sizeof(TIDStr)) == FAILURE)
      {
      break;
      }

    /* Set up reservation request */

    TID = (int)strtol(TIDStr,NULL,10);

    TIDFound = TRUE;

    tmpStoreRsv = &StorageRsv;

    MRsvInitialize(&tmpStoreRsv);
    MRsvSetAttr(tmpStoreRsv,mraResources,(void *)&TID,mdfInt,mSet);

    tmpStartTime = tmpStoreRsv->StartTime;
    tmpEndTime = tmpStoreRsv->EndTime;

    if ((mulong)tmpStartTime < MSched.Time)
      {
      tmpStartTime = MAX(MSched.Time,(mulong)tmpStartTime);
      }

    MRsvSetTimeframe(tmpStoreRsv,mraStartTime,mSet,tmpStartTime,StoreEMsg);
    MRsvSetAttr(tmpStoreRsv,mraEndTime,(void *)&tmpEndTime,mdfLong,mSet);

    MRsvSetAttr(tmpStoreRsv,mraFlags,(void *)"NoVMMigrations",mdfString,mAdd);

    /* Create reservation */

    if (MRsvConfigure(
          NULL,
          tmpStoreRsv,
          0,
          0,
          StoreEMsg,
          &R,
          NULL,
          FALSE) == FAILURE)
      {
      if (tmpLine != NULL)
        snprintf(tmpLine,tmpLineSize,"ERROR:  could not reserve storage\n");

      return(FAILURE);
      }

    if ((tmpStoreRsv->ReqTC > 0) &&
        (tmpStoreRsv->MinReqTC > 0) &&
        (R->AllocTC < tmpStoreRsv->MinReqTC))
      {
      if (tmpLine != NULL)
        snprintf(tmpLine,tmpLineSize,"ERROR:  could not reserve sufficient storage tasks\n");

      return(FAILURE);
      }

    /* Free up resources from tmpStoreRsv */

    MRsvDestroy(&tmpStoreRsv,FALSE,FALSE);

    /* Set the BYNAME flag, other setup */

    bmset(&R->Flags,mrfByName);
    bmset(&R->Flags,mrfParentLock);

    bmunset(&R->Flags,mrfSystemJob);

    MRsvSetAttr(R,mraACL,(void *)AdminName,mdfString,mSet);

    MULLAdd(&VMCR->StorageRsvs,R->Name,(void *)R,NULL,NULL);

    if (AdvRsvR == NULL)
      {
      AdvRsvR = R;
      }
    } /* END while (MXMLGetChild(PE,"range",&CTok,&CE) == SUCCESS) */

  MXMLDestroyE(&RE);
  MXMLDestroyE(&AE);

  if (TIDFound == FALSE)
    {
    if (tmpLine != NULL)
      snprintf(tmpLine,tmpLineSize,"ERROR:  could not locate TID for storage request\n");

    return(FAILURE);
    }

  MUArrayListCreate(&StorageArray,sizeof(mjob_t *),1);

  rc = MUGenerateSystemJobs(
        ((VMCR != NULL) && (VMCR->U != NULL)) ? VMCR->U->Name : NULL,
        NULL,
        &AdvRsvR->NL,
        msjtVMStorage,
        "vmstorage",
        0,
        NULL,
        VMCR,
        FALSE,
        FALSE,
        NULL,
        &StorageArray);

  if (rc == SUCCESS)
    {
    mvm_t *VM = NULL;

    StorageJ = *(mjob_t **)MUArrayListGet(&StorageArray,0);

    StorageJ->System->RecordEventInfo = MUStrFormat("requested by %s",
      (Auth == NULL) ? "UNKNOWN" : Auth);

    StorageJ->Credential.U = VMCR->U;

    if (AdvRsvR != NULL)
      {
      StorageJ->SMinTime = AdvRsvR->StartTime;
      }

    /* Make storage creation requests */

    if (MVMFind(VMCR->VMID,&VM) == SUCCESS)
      {
      /* Pull storage info off of the structures on the VM */

      if (VM->Storage != NULL)
        {
        mln_t *StorePtr = NULL;
        mrm_t *StoreRM = NULL;

        while (MULLIterate(VM->Storage,&StorePtr) == SUCCESS)
          {
          /* Create a trigger for each storage mount */

          mvm_storage_t *StoreMount = (mvm_storage_t *)StorePtr->Ptr;

          mln_t *RsvPtr = NULL;

          /* Associate storage mount with a reservation */

          while (MULLIterate(VMCR->StorageRsvs,&RsvPtr) == SUCCESS)
            {
            mrsv_t *TmpR = (mrsv_t *)RsvPtr->Ptr;

            if (bmisset(&TmpR->IFlags,mrifUsedByVMStorage))
              continue;

            /* If same type and size, take it */

            /* ASSUMPTION: Each storage node only reports one type of storage */
            /* If you have gold:3%gold:2, MClusterShowARes will give two separate reservations */

            if (MSNLGetIndexCount(&TmpR->DRes.GenericRes,MUMAGetIndex(meGRes,StoreMount->Type,mVerify)) == StoreMount->Size)
              {
              StoreMount->R = TmpR;
              MUStrCpy(StoreMount->Hostname,MNLGetNodeName(&TmpR->NL,0,NULL),sizeof(StoreMount->Hostname));

              bmset(&TmpR->IFlags,mrifUsedByVMStorage);

              break;
              }
            } /* END (MULLIterate(VMCR->StorageRsvs...) == SUCCESS) */

          if ((StoreMount->R != NULL) && (!MNLIsEmpty(&StoreMount->R->NL)))
            {
            marray_t  TList;

            char TrigVar[MMAX_NAME];
            char *TrigVar2 = NULL; /* T->SSets requires a pointer that won't be blown away */

            mtrig_t *T = NULL;

            StoreRM = NULL;

            if (MSched.TVarPrefix != NULL)
              {
              char *PlusPrefix;

              MUStrCpy(TrigVar,MSched.TVarPrefix,MMAX_NAME);
              PlusPrefix = TrigVar + strlen(MSched.TVarPrefix);

              MUGenRandomStr(PlusPrefix,MMAX_NAME - (strlen(MSched.TVarPrefix)),8);
              }
            else
              {
              MUGenRandomStr(TrigVar,MMAX_NAME,8);
              }

            if ((MNodeGetResourceRM(NULL,StoreMount->Hostname,mrmrtStorage,&StoreRM) == FAILURE) ||
                (StoreRM == NULL))
              {
              if (tmpLine != NULL)
                snprintf(tmpLine,tmpLineSize,"ERROR:  failed to find storage RM\n");

              MUArrayListFree(&StorageArray);

              return(FAILURE);
              }

            mstring_t TrigAction(MMAX_LINE);

            MStringSetF(&TrigAction,"atype=exec,etype=start,action=\"%s createstorage STORAGESERVER=%s VARIABLE=%s HOSTLIST=%s HYPERVISOR=%s OPERATIONID=%s %s=%d%s%s\"",
              StoreRM->ND.Path[mrmSystemModify],
              StoreMount->Hostname,   /* StoreMount->Hostname = StoreMount->R->NL[0].N->Name,*/
              TrigVar,
              VM->VMID,
              (!MUStrIsEmpty(VMCR->HVName) ? VMCR->HVName : "NULL"),
              StorageJ->Name,
              StoreMount->Type,
              StoreMount->Size,
              (StoreMount->MountPoint[0] != '\0') ? "@" : "",
              (StoreMount->MountPoint[0] != '\0') ? StoreMount->MountPoint : "");

            MUArrayListCreate(&TList,sizeof(mtrig_t *),2);

            if (MTrigLoadString(&StorageJ->Triggers,TrigAction.c_str(),TRUE,FALSE,mxoJob,StorageJ->Name,&TList,NULL) == SUCCESS)
              {
              T = (mtrig_t *)MUArrayListGetPtr(&TList,0);

              /* Set max retry count to avoid one-time failures (make configurable in future) */
              T->MaxRetryCount = 3;
 
              /* Tell trigger to set variables */
              MUStrDup(&TrigVar2,TrigVar);
              MTrigInitSets(&T->SSets,FALSE);
              MUArrayListAppendPtr(T->SSets,TrigVar2);

              /* Set the trigger env */
              if ((StoreRM->Env != NULL) && (StoreRM->Env[0] != '\0'))
                MUStrDup(&T->Env,StoreRM->Env);

              /* Attach this trigger to the job */

              MTrigInitialize(T);

              StoreMount->T = T;

              MUStrCpy(StoreMount->TVar,TrigVar,sizeof(StoreMount->TVar));

              /* Set the trigger checkpoint flag to force checkpoint. */

              bmset(&T->SFlags,mtfCheckpoint);
              }   /* END if (MTrigLoadString() == SUCCESS) */

            MUArrayListFree(&TList);
            }
          } /* END while(MULLIterate(VM->Storage,&StorePtr) == SUCCESS) */

        /* Put a variable on the job so we know what VM it is for */

        MUHTAdd(&StorageJ->Variables,"VMID",strdup(VMCR->VMID),NULL,MUFREE);
        } /* END if (MVMFind() == SUCCESS) */
      else
        {
        if (tmpLine != NULL)
          snprintf(tmpLine,tmpLineSize,"ERROR:  failed to parse storage onto VM\n");

        MUArrayListFree(&StorageArray);

        return(FAILURE);
        } /* END else (VM->Storage != NULL) */
      } /* END if (MVMFind() == SUCCESS) */
    } /* END if (rc == SUCCESS) */
  else
    {
    /* Failed to create storage job, remove VM */

    mvm_t *VM = NULL;

    if (MVMFind(VMCR->VMID,&VM) == SUCCESS)
      {
      MVMDestroy(&VM,NULL);
      }

    if (tmpLine != NULL)
      snprintf(tmpLine,tmpLineSize,"ERROR:  could not create storage system job\n");

    MUArrayListFree(&StorageArray);

    return(FAILURE);
    }

  *StoreRsv = AdvRsvR;
  *StoreJ = StorageJ;

  MUArrayListFree(&StorageArray);

  return(SUCCESS);
  } /* END MVMCreateStorage() */
#endif

#if 0
/* Harvests the variables and such from successful
 *  storage creation triggers.  Note that this is called
 *  whether all of the storage triggers succeeded or not!
 *
 * We must find all successful triggers if the job fails so that
 *  we can remove any allocated storage.
 *
 * @param VM    (I) [modified] - The VM to attach storage mounts to
 * @param J     (I) - The storage job which allocated storage
 * @param State (O) - The state for the VMStorage job
 */

int MVMHarvestStorageVars(
  mvm_t *VM, /* I (modified) */
  mjob_t *J,  /* I */
  enum MJobStateEnum *State) /* O */

  {
  mln_t *StorePtr = NULL;
  int PathCount = 0; /* Just for mount path generation */

  if ((VM == NULL) ||
      (J == NULL) ||
      (J->System == NULL) ||
      (J->System->JobType != msjtVMStorage))
    {
    return(FAILURE);
    }

  /* Right now, VM->Action[0] is the name of the VM create job */

  if ((VM->CreateJob == NULL) ||
      (VM->CreateJob->System == NULL))
    {
    /* Problem with VM create job? */

    MDB(3,fSTRUCT) MLog("ERROR:    failed to find VM create job for VM '%s'\n",
      VM->VMID);

    *State = mjsRemoved;

    return(FAILURE);
    }

  while (MULLIterate(VM->Storage,&StorePtr) == SUCCESS)
    {
    mvm_storage_t *StoreMount = (mvm_storage_t *)StorePtr->Ptr;

    char *Path;
    char MountPath[MMAX_LINE * 2]; /* Location on the VM to place the mount point */

    if (MUHTGet(&J->Variables,StoreMount->TVar,(void **)&Path,NULL) == FAILURE)
      {
      /* Couldn't find variable, kill the job */

      MDB(3,fSTRUCT) MLog("ERROR:    failed to find trigger variable '%s' for system job '%s'\n",
        StoreMount->TVar,
        J->Name);

      *State = mjsRemoved;

      /* Don't return here, need to find all successful triggers */
      continue;
      }

    MUStrCpy(StoreMount->Location,Path,sizeof(StoreMount->Location));

    /* Add storage into the create job action */

    /* MSM will treat any parameter that begins with "STORAGE" as storage mount info */

    if (StoreMount->MountPoint[0] != '\0')
      {
      snprintf(MountPath,sizeof(MountPath),"%s:STORAGE%s=\"%s@%s\"",
        VM->CreateJob->System->Action,
        StoreMount->Name,
        StoreMount->Location,
        StoreMount->MountPoint);

      MUStrDup(&VM->CreateJob->System->Action,MountPath);
      }
    else if (MSched.VMStorageMountDir != NULL)
      {
      snprintf(StoreMount->MountPoint,sizeof(StoreMount->MountPoint),"%s/VMStorage%d",
        MSched.VMStorageMountDir,
        PathCount);

      snprintf(MountPath,sizeof(MountPath),"%s:STORAGE%s=\"%s@%s\"",
        VM->CreateJob->System->Action,
        StoreMount->Name,
        StoreMount->Location,
        StoreMount->MountPoint);

      PathCount++;

      MUStrDup(&VM->CreateJob->System->Action,MountPath);
      }
    else
      {
      MDB(7,fSTRUCT) MLog("ERROR:    No VM storage mount point specified for VM '%s'\n",
        VM->VMID);

      *State = mjsRemoved;

      continue;
      }
    } /* END while (MULLIterate(VM->Storage,...) == SUCCESS) */

  if (*State != mjsRemoved)
    {
    *State = mjsCompleted;
    }
  else
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MVMHarvestStorageVars() */
#endif

  
#if 0

/*
 * Creates a storage job with associated triggers based on the given mvm_req_create_t.
 *  It will find the associated storage nodes in NodeList.
 *
 * @param NodeList [I] - List of nodes that includes storage nodes
 * @param VMCR     [I] - VM definition w/ storage
 * @param ResultJ  [O] - The storage job with triggers
 */

int MVMCreateStorageJob(

  mnl_t *NodeList,          /* I */
  mvm_req_create_t *VMCR,   /* I */
  mjob_t **ResultJ)         /* O */

  {
  marray_t StorageArray;
  mjob_t  *StorageJ = NULL;
  int rc;

  const char *FName = "MVMCreateStorageJob";
  MDB(5,fSTRUCT) MLog("%s(NodeList,VMCR,ResultJ)\n",FName);

  if ((ResultJ == NULL) || (VMCR == NULL))
    return(FAILURE);

  *ResultJ = NULL;

  if (VMCR->Storage[0] == '\0')
    {
    return(SUCCESS);
    }

  MUArrayListCreate(&StorageArray,sizeof(mjob_t *),1);

  rc = MUGenerateSystemJobs(
         (VMCR->U != NULL) ? VMCR->U->Name : NULL,
         NULL,
         NodeList,
         msjtVMStorage,
         "vmstorage",
         0,
         NULL,
         VMCR,
         FALSE,
         FALSE,
         NULL,
         &StorageArray);

  if (rc == SUCCESS)
    {
    mvm_t *VM = NULL;
    char tmpLine[MMAX_LINE];
    mjob_t *TrackingJ;

    snprintf(tmpLine,MMAX_LINE,"VMID=%s",
      VMCR->VMID);

    StorageJ = *(mjob_t **)MUArrayListGet(&StorageArray,0);

    MJobSetAttr(StorageJ,mjaVariables,(void **)&tmpLine,mdfString,mAdd);

    /* if vm create request has a proxy then set it in the storage job */

    if (VMCR->TrackingJ[0] != '\0')
      {
      
      /* find the vm tracking job */

      if (MJobFind(VMCR->TrackingJ,&TrackingJ,mjsmExtended) == SUCCESS)
        {
        /* give the storage job the Proxy User */

        if ((TrackingJ->System != NULL) &&
            (StorageJ->System != NULL) &&
            (TrackingJ->System->ProxyUser != NULL))
          {
          MUStrDup(&StorageJ->System->ProxyUser,TrackingJ->System->ProxyUser);
          }
        } /* END if MJobFind() */
      } /* END if (VMCR->Tracking[0] != '\0') */

    /* Make storage creation requests */

    if (MVMFind(VMCR->VMID,&VM) == SUCCESS)
      {
      if (VM->Storage != NULL)
        {
        mln_t *StorePtr = NULL;

        while (MULLIterate(VM->Storage,&StorePtr) == SUCCESS)
          {
          /* Create a trigger for each storage mount */

          mnode_t *StorageN = NULL;
          mtrig_t *TrigPtr = NULL;
          mvm_storage_t *StoreMount = (mvm_storage_t *)StorePtr->Ptr;
          mrm_t *StoreRM = NULL;
          char TrigAction[MMAX_LINE];
          char TrigVar[MMAX_NAME];
          char *TrigVar2 = NULL; /* T->SSets requires a pointer that won't be blown away */

          mtrig_t *OldTrig = NULL;

          if (MSched.TVarPrefix != NULL)
            {
            char *PlusPrefix;

            MUStrCpy(TrigVar,MSched.TVarPrefix,MMAX_NAME);
            PlusPrefix = TrigVar + strlen(MSched.TVarPrefix);

            MUGenRandomStr(PlusPrefix,MMAX_NAME - (strlen(MSched.TVarPrefix)),8);
            }
          else
            {
            MUGenRandomStr(TrigVar,MMAX_NAME,8);
            }

          if ((MVMGetStorageNode(NodeList,StoreMount->Type,&StorageN) == FAILURE) ||
              (StorageN == NULL))
            {
            return(FAILURE);
            }

          MUStrCpy(StoreMount->Hostname,StorageN->Name,sizeof(StoreMount->Hostname));

          if ((MNodeGetResourceRM(NULL,StoreMount->Hostname,mrmrtStorage,&StoreRM) == FAILURE) ||
                (StoreRM == NULL))
            {
            MDB(5,fSTRUCT) MLog("ERROR:     failed to find storage RM\n");

            return(FAILURE);
            }

          MDB(7,fSTRUCT) MLog("INFO:     creating trigger to request storage: type=%s size=%d\n",
            StoreMount->Type,
            StoreMount->Size);

          /* Storage reservations are owned by the parent job */

          snprintf(TrigAction,sizeof(TrigAction),"%s createstorage STORAGESERVER=%s VARIABLE=%s HOSTLIST=%s HYPERVISOR=%s OPERATIONID=%s %s=%d%s%s",
            StoreRM->ND.Path[mrmSystemModify],
            StoreMount->Hostname,   /* StoreMount->Hostname = StoreMount->R->NL[0].N->Name,*/
            TrigVar,
            VM->VMID,
            (!MUStrIsEmpty(VMCR->HVName) ? VMCR->HVName : "NULL"),
            StorageJ->Name,
            StoreMount->Type,
            StoreMount->Size,
            (StoreMount->MountPoint[0] != '\0') ? "@" : "",
            (StoreMount->MountPoint[0] != '\0') ? StoreMount->MountPoint : "");

          if (StoreMount->MountOpts[0] != '\0')
            {
            MUStrAppendStatic(TrigAction,StoreMount->MountOpts,'#',sizeof(TrigAction));
            }

          /* Alloc trigger memory & create it */

          TrigPtr = (mtrig_t *)MUCalloc(1,sizeof(mtrig_t));
          if (MTrigCreate(TrigPtr,mttStart,mtatExec,TrigAction) == FALSE)
            {
            MDB(5,fSTRUCT) MLog("ERROR:    failed to create storage trigger for storage job '%s'\n",
              StorageJ->Name);

            MUFree((char**)&TrigPtr);

            MUArrayListFree(&StorageArray);

            return(FAILURE);
            }

          /* Set trigger max retry count */
          TrigPtr->MaxRetryCount = 3;

          /* Tell trigger to set variables */
          MUStrDup(&TrigVar2,TrigVar);
          MTrigInitSets(&TrigPtr->SSets,FALSE);
          MUArrayListAppendPtr(TrigPtr->SSets,TrigVar2);

          /* Set the trigger env */
          if ((StoreRM->Env != NULL) && (StoreRM->Env[0] != '\0'))
            MUStrDup(&TrigPtr->Env,StoreRM->Env);

          /* Attach this trigger to the job */
          TrigPtr->OType = mxoJob;
          MUStrDup(&TrigPtr->OID,StorageJ->Name);

          /* MTrigAdd just copies the trigger over, so we need to free the old one */
          OldTrig = TrigPtr;

          MTrigAdd(TrigPtr->TName,TrigPtr,&TrigPtr);
          MOAddTrigPtr(&StorageJ->Triggers,TrigPtr);
          StoreMount->T = TrigPtr;

          /* Free old trigger and name
              Don't do a deep clean because it was copied */

          MUFree(&OldTrig->TName);
          MUFree((char **)&OldTrig);

          MUStrCpy(StoreMount->TVar,TrigVar,sizeof(StoreMount->TVar));
          } /* while (MULLIterate(VM->Storage,&StorePtr) == SUCCESS) */
        } /* if (VM->Storage != NULL) */
      } /* END if (MVMFind(VMCR->VMID,&VM) == SUCCESS) */
    } /* END if (rc == SUCCESS) */

  if (StorageJ != NULL)
    *ResultJ = StorageJ;

  MUArrayListFree(&StorageArray);

  return(rc);
  } /* END MVMCreateStorageJob() */

#endif


/*
 * Finds which storage node in the NodeList for the given GRES type
 *
 * @param NodeList [I] - List of nodes to select from
 * @param Type     [I] - Type of storage to search for
 * @param N        [O] - Resulting storage node (NULL if not found)
 */

int MVMGetStorageNode(
  mnl_t *NodeList,  /* I */
  char *Type,           /* I */
  mnode_t **N)          /* O */

  {
  int NIndex;
  int GResIndex;

  mnode_t *tmpN;

  if ((NodeList == NULL) ||
      (Type == NULL) ||
      (N == NULL))
    {
    return(FAILURE);
    }

  *N = NULL;

  GResIndex = MUMAGetIndex(meGRes,Type,mVerify);

  if (GResIndex <= 0)
    {
    MDB(3,fSTRUCT) MLog("ERROR:    Could not find storage type '%s' to retrieve storage node\n",
      Type);

    return(FAILURE);
    }

  for (NIndex = 0;MNLGetNodeAtIndex(NodeList,NIndex,&tmpN) == SUCCESS;NIndex++)
    {
    if (!bmisset(&tmpN->RM->RTypes,mrmrtStorage))
      continue;

    if (MSNLGetIndexCount(&tmpN->CRes.GenericRes,GResIndex) <= 0)
      continue;

    *N = tmpN;

    return(SUCCESS);
    }

  return(FAILURE);
  } /* END MVMGetStorageNode() */




/*
 * When a vmtracking/vmcreate job is created and has storage, we need to
 *   reserve the resources to keep another job from taking the procs
 *   and storage from the master job while storage is being created.
 *
 * @param J [I] (modified) -> the VMtracking/msubbed job
 */

int MVMCreateTempStorageRsvs(
  mjob_t *J,
  mnl_t *StorageNL,
  mrsv_t **ReturnRsv,
  char *ErrorMsg)

  {
  int RQIndex = 1;
  char tmpEMsg[MMAX_LINE];
  char *EMsg = NULL;
  mrsv_t *ProcRsv = NULL;

  if ((J == NULL) ||
      (StorageNL == NULL) ||
      (ReturnRsv == NULL))
    {
    return(FAILURE);
    }

  if (ErrorMsg != NULL)
    EMsg = ErrorMsg;
  else
    EMsg = tmpEMsg;

  *ReturnRsv = NULL;

  while (J->Req[RQIndex] != NULL)
    {
    mnode_t  *tmpN;
    mbitmap_t tmpFlags;
    mnl_t StoreNL = {0};
    mulong    StoreEndTime;
    mrsv_t   *R = NULL;
    macl_t   *pACL = NULL;
    char      RName[MMAX_NAME];
    int       GRIndex = 0;
    char      StorageVar[MMAX_LINE];

    /* NL is all of the nodes, need to find correct one */

    GRIndex = 1;

    while (MSNLGetIndexCount(&J->Req[RQIndex]->DRes.GenericRes,GRIndex) <= 0)
      {
      GRIndex++;
      }

    if (MVMGetStorageNode(StorageNL,MGRes.Name[GRIndex],&tmpN) == FAILURE)
      {
      strcpy(EMsg,"Unable to find specified storage node.");

      return(FAILURE);
      }

    MNLInit(&StoreNL);
    MNLSetNodeAtIndex(&StoreNL,0,tmpN);
    MNLSetTCAtIndex(&StoreNL,0,1);

    StoreEndTime = MSched.Time + MPar[0].VMCreationDuration + MCONST_HOURLEN;

    bmset(&tmpFlags,mrfNoVMMigrations);

    snprintf(RName,sizeof(RName),"tmpvmstorage.%d",
      MSched.RsvCounter++);

    /* Make sure that the VMTracking job is in ACL */

    pACL = (macl_t *)MUCalloc(1,sizeof(macl_t));
    if (pACL)
      {
      MACLInit(pACL);
      MACLSet(&pACL,maJob,J->Name,mcmpSEQ,mnmNeutralAffinity,0,0);
      }

    /* This reservation will be destroyed when the VMTracking job begins */

    MRsvCreate(
      mrtUser,
      pACL,
      NULL,
      &tmpFlags,
      &StoreNL,
      MSched.Time,
      StoreEndTime,
      1,
      -1,
      RName,
      &R,
      NULL,
      &J->Req[RQIndex]->DRes,
      NULL,
      TRUE,
      TRUE);

    bmclear(&tmpFlags);

    if (R == NULL)
      {
      strcpy(EMsg,"failed to create reservation for storage");

      MNLFree(&StoreNL);
      MACLFreeList(&pACL);

      return(FAILURE);
      }

    snprintf(StorageVar,sizeof(StorageVar),"STORAGERSV%d=%s",
      RQIndex,
      R->Name);

    MJobSetAttr(J,mjaVariables,(void **)StorageVar,mdfString,mAdd);

    RQIndex++;

    MACLFreeList(&pACL);
    MNLFree(&StoreNL);
    }  /* END while (J->Req[RQIndex] != NULL) */

  /* Also create a reservation to guard the procs */
  /*  It will also be called "STORAGERSV" */

  if (RQIndex > 1)
    {
    mbitmap_t tmpFlags;
    mnl_t StoreNL;
    mulong    StoreEndTime = 0;
    macl_t   *pACL = NULL;
    char      RName[MMAX_NAME];
    char      StorageVar[MMAX_LINE];

    StoreEndTime = MSched.Time + MPar[0].VMCreationDuration + MCONST_HOURLEN;

    MNLInit(&StoreNL);

    MNLSetNodeAtIndex(&StoreNL,0,MNLReturnNodeAtIndex(&J->Req[0]->NodeList,0));
    MNLSetTCAtIndex(&StoreNL,0,1);

    snprintf(RName,sizeof(RName),"tmpvmstorage.%d",
      MSched.RsvCounter++);

    bmset(&tmpFlags,mrfNoVMMigrations);

    /* Make sure that the VMTracking job is in ACL */

    pACL = (macl_t *)MUCalloc(1,sizeof(macl_t));

    if (pACL)
      {
      MACLInit(pACL);
      MACLSet(&pACL,maJob,J->Name,mcmpSEQ,mnmNeutralAffinity,0,0);
      }

    /* This reservation will be destroyed when the VMTracking job begins */

    if ((MRsvCreate(
        mrtUser,
        pACL,
        NULL,
        &tmpFlags,
        &StoreNL,
        MSched.Time,
        StoreEndTime,
        1,
        -1,
        RName,
        &ProcRsv,
        NULL,
        &J->Req[0]->DRes,
        NULL,
        TRUE,
        TRUE) == FAILURE) ||
        (ProcRsv == NULL))
      {
      bmclear(&tmpFlags);

      ProcRsv = NULL;

      strcpy(EMsg,"failed to create reservation for storage");

      MNLFree(&StoreNL);
      MACLFreeList(&pACL);

      return(FAILURE);
      }

    bmclear(&tmpFlags);

    snprintf(StorageVar,sizeof(StorageVar),"STORAGERSV%d=%s",
      RQIndex,
      ProcRsv->Name);

    MJobSetAttr(J,mjaVariables,(void **)StorageVar,mdfString,mAdd);

    MACLFreeList(&pACL);
    MNLFree(&StoreNL);
    }  /* if (RQIndex > 1) */

  if (ProcRsv != NULL)
    *ReturnRsv = ProcRsv;

  return(SUCCESS);
  } /* END MVMCreateTempStorageRsvs() */

/*
 *  Releases all the reservations for a tracking job as specified by the job
 *  variables "STORAGERSV%d".
 *
 * @param J [I] 
*/

int MVMReleaseTempStorageRsvs(

  mjob_t *J)

  {
  int index = 1;
  int rc = SUCCESS;

	if (J == NULL)
		return(FAILURE);

  while(rc == SUCCESS)
    {
    char StorageVar[MMAX_LINE];
    char *VarVal;
    mrsv_t *R;

    snprintf(StorageVar,sizeof(StorageVar),"STORAGERSV%d",index++);

    rc = MUHTGet(&J->Variables,StorageVar,(void **)&VarVal,NULL);

    if ((rc == SUCCESS) && (MRsvFind(VarVal,&R,mraNONE) == SUCCESS))
      {
      MRsvDestroy(&R,TRUE,TRUE);
      }
    }
  return(SUCCESS);
  } /* END MVMReleaseTempStorageRsvs */


/* Traverses all nodes and gets a list of all storage mounts in the 
 * form of mvm_storage_t pointers.
 *
 * NOTE:  This function initializes the marray_t and the caller must 
 *        release the resouces by calling MUArrayListFree(&StorageArray);
 *
 * @param StorageArray (O) - Array that contains list of mvm_storage pointers.
*/

int MVMGetStorageMounts(

  marray_t *StorageArray)

{
  mnode_t *N;
  mln_t *P;
  mvm_t *V;
  int nindex;

  MUArrayListCreate(StorageArray,sizeof(mvm_storage_t*),-1);

  /* Get list of all storage nodes */

  for (nindex=0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    for (P = N->VMList;P != NULL;P = P->Next)
      {
      V = (mvm_t *)P->Ptr;
  
      if (V->Storage != NULL)
        {
        mln_t *StorePtr = NULL;
    
        while (MULLIterate(V->Storage,&StorePtr) == SUCCESS)
          MUArrayListAppendPtr(StorageArray,StorePtr->Ptr);
        } /* END if (V->Storage != NULL) */
      }  /* END for (P) */
    }

  return(SUCCESS);
}

/* Gets a list of storage associated with the storage mounts.  The
 * nodes returned are mnode_t pointers.  
 *
 * The main difference between a normal node and a storage node is that 
 * normal nodes have processors that can run a job and a storage node doesn't 
 * have processors.  The VM's have a storage mount (mvm_storage_t*) that points to the node.
 *
 * NOTE:  This function initializes the marray_t and the caller must 
 *        release the resouces by calling MUArrayListFree(&StorageArray);
 *
 * @param StorageArray (I) - Array that contains list of mvm_storage pointers.
 * @param NodeArray (O) - Array of all nodes that have storage mounts pointed at them.
*/
int MVMGetStorageNodes(

  marray_t *StorageArray,
  marray_t *NodeArray)

  {
  int nindex;
  int sindex;
  mvm_storage_t *S;
  mnode_t *N;

  MUArrayListCreate(NodeArray,sizeof(mnode_t*),-1);

  /* Get list of all host nodes */

  for (sindex = 0;sindex < StorageArray->NumItems;sindex++)
    {
    S = (mvm_storage_t*) MUArrayListGetPtr(StorageArray,sindex);

    if ((S == NULL) || (S->Hostname[0] == '\0'))
      continue;
      
    /* Get host Node */

    if (MNodeFind(S->Hostname,&N) == SUCCESS)
      {
      mnode_t *tmpNode;

      /* Find duplicates */

      for (nindex = 0;nindex < NodeArray->NumItems;nindex++)
        {
        tmpNode = (mnode_t*)MUArrayListGetPtr(NodeArray,nindex);
        if (tmpNode == N)
          break;
        }

      /* Add Node, if not duplicate. */

      if (nindex >= NodeArray->NumItems)
        MUArrayListAppendPtr(NodeArray,N);
      }
    }

  return(TRUE);
  }

/* Traverses all nodes looking for storage nodes that checks for 
 * storage mounts that have storage node assigned.
 *
 * NOTE:  MSched.VMStorageNodeThreshold should be checked prior to 
 *        calling this function.  Zero means disabled feature.
 *
 * The results are messages posted on the Scheduler & Node.
*/

int MVMCheckStorage()

  {
  mnode_t *N;
  marray_t StorageArray;
  marray_t NodeArray;
  int nindex;
  int sindex;
  mvm_storage_t *S;

  /* Get a list of all storage mounts */

  MVMGetStorageMounts(&StorageArray);

  /* Get a list of all storage nodes associated with storage mounts */

  MVMGetStorageNodes(&StorageArray,&NodeArray);
  
  /* Search through each node, find storage nodes and sum disk space. */

  for (nindex = 0;nindex < NodeArray.NumItems;nindex++)
    {
    int sum[MMAX_ATTR];
    int percentage[MMAX_ATTR];
    int tindex;

    memset(&sum,0,sizeof(sum));
    memset(&percentage,0,sizeof(percentage));

    N = (mnode_t*)MUArrayListGetPtr(&NodeArray,nindex);

    /* Sum the storage nodes */

    for (sindex = 0;sindex < StorageArray.NumItems;sindex++)
      {
      S = (mvm_storage_t*) MUArrayListGetPtr(&StorageArray,sindex);

      /* Get the type index & sum storage if we've got a matching host name */

      if (strcmp(N->Name,S->Hostname) == 0)
        {
        for (tindex = 1;tindex < MSched.M[mxoxGRes];tindex++)
          {
          /* Check the storage type */

          if (strcmp(MGRes.Name[tindex], S->Type) == 0)
            {
            sum[tindex] += S->Size;
            break;
            }
          }
        }
      } /* END for (sindex = 0; .... */

    /* Calculate the threshold & post alerts. */

    for (sindex=1; sindex < MSched.M[mxoxGRes];sindex++)
      {
      if (MUStrIsEmpty(MGRes.Name[sindex]))
        break;

      if (MSNLGetIndexCount(&N->CRes.GenericRes,sindex) > 0)
        {
        /* Calculate the percentage */

        percentage[sindex] = (sum[sindex] * 100) / MSNLGetIndexCount(&N->CRes.GenericRes,sindex);

        /* Check the threshold against configured.  Defaults at 80 percent.
         * Refer to VMSTORAGENODETHRESHOLD as configurable parameter. */

        if (percentage[sindex] > MSched.VMStorageNodeThreshold)
          {
          char tmpLine[MMAX_LINE];
    
          /* Post threshold alert in message log, node, and scheduler */

          snprintf(tmpLine,sizeof(tmpLine),"ALERT:   Node %s, type %s, exceeded disk threshold of %d at %d percent.  Use mdiag -n -v -v for detailed information.\n",
            N->Name,
            MAList[meGRes][sindex],
            MSched.VMStorageNodeThreshold,
            percentage[sindex]);
    
          MDB(5,fALL) MLog(tmpLine);
          MMBAdd(&N->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
          MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
          }
        } /* END if (N->CRes.GRes[sindex].count > 0) */
      } /* END for (sindex=1; sindex < MMAX_ATTR &&*/
    } /* END for (nindex = 0;nindex < NodeArray.NumItems;nindex++) */

  MUArrayListFree(&StorageArray);
  MUArrayListFree(&NodeArray);

  return(SUCCESS);
  }



int MVMSynchronizeStorageRsvs()

  {
  mhashiter_t  VMIter;
  mvm_t *VM;

  MUHTIterInit(&VMIter);

  while (MUHTIterate(&MSched.VMTable,NULL,(void **)&VM,NULL,&VMIter) == SUCCESS)
    {
    if ((VM != NULL) && (VM->StorageRsvNames != NULL))
      {
      /* parse the storage names and get the reservation pointers */

      char *ptr = NULL;
      char *TokPtr = NULL;

      char tmpLine[MMAX_LINE];

      MUStrCpy(tmpLine,VM->StorageRsvNames,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,",",&TokPtr);

      while (ptr != NULL)
        {
        mrsv_t *R = NULL;

        if (MRsvFind(ptr,&R,mraNONE) == SUCCESS)
          {
          MULLAdd(&VM->StorageRsvs,R->Name,(void *)R,NULL,NULL);
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        } /* END while (ptr != NULL) */
      } /* END if ((VM != NULL) && (VM->StorageRsvNames != NULL)) */
    } /* END while (MUHTIterate(&MSched.VMTable...) == SUCCESS) */

  return(SUCCESS);
  } /* END MVMSynchronizeStorageRsvs() */

/* END MVMStorage.c */

/**
 * Report VM List as comma-delimited string.
 *
 * @param VL     (I)
 * @param String (O)
 */

int MVMLToMString(

  mvm_t      **VL,
  mstring_t   *String)

  {
  int vindex;

  if (VL == NULL)
    {
    return(FAILURE);
    }

  for (vindex = 0;VL[vindex] != NULL;vindex++)
    {
    MStringAppendF(String,"%s%s",
      (vindex > 0) ? "," : "",
      VL[vindex]->VMID);
    }  /* END for (vindex) */

  return(SUCCESS);
  }  /* END MVMLToString() */




/**
 * Report VM List as comma-delimited string.
 *
 * @param VL (I)
 * @param Buf (O)
 * @param BufSize (I) 
 */

int MVMLToString(

  mvm_t **VL,      /* I */
  char   *Buf,     /* O */
  int     BufSize) /* I */

  {
  int vindex;

  char *BPtr = NULL;
  int   BSpace = 0;

  MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if (VL == NULL)
    {
    return(FAILURE);
    }

  for (vindex = 0;VL[vindex] != NULL;vindex++)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (vindex > 0) ? "," : "",
      VL[vindex]->VMID);
    }  /* END for (vindex) */

  return(SUCCESS);
  }  /* END MVMLToString() */




#if 0
/*
 * Populate the given VMCR from the string
 *
 * @param VMString (I) - The string that describes the VM (modified)
 * @param VMCR     (O) - The vm req create structure to be populated (not created here)
 * @param EMsg     (O) - The error message buffer
 * @param EMsgSize (I) - Size of EMsg 
 */

int MVMCRFromString(

  char *VMString,          /* I */
  mvm_req_create_t *VMCR,  /* O */
  char *EMsg,              /* O */
  int   EMsgSize)          /* I */

  {
  char *ptr;
  char *TokPtr;
  char *TokPtr2;
  char *aptr;
  char *vptr;
  char tmpVMID[MMAX_NAME];

  if ((VMString == NULL) || (VMCR == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  MVMCreateReqInit(VMCR);

  TokPtr = VMString;

  VMCR->CRes.Procs = 1;

  while ((ptr = MUStrTokEPlusLeaveQuotes(NULL,",",&TokPtr)) != NULL)
    {
    aptr = MUStrTokEPlusLeaveQuotes(ptr,"=",&TokPtr2);
    vptr = TokPtr2;

    if ((aptr == NULL) || (vptr == NULL))
      continue;

    if (!strcasecmp(aptr,"disk"))
      VMCR->CRes.Disk = (int)strtol(vptr,NULL,10);
    else if (!strcasecmp(aptr,"mem"))
      VMCR->CRes.Mem = (int)strtol(vptr,NULL,10);
    else if (!strcasecmp(aptr,"procs") || !strcasecmp(aptr,"cpus"))
      VMCR->CRes.Procs = (int)strtol(vptr,NULL,10);
    else if (!strcasecmp(aptr,"sovereign"))
      {
      VMCR->IsSovereign = MUBoolFromString(vptr,FALSE);
      }
    else if (!strcasecmp(aptr,"feature"))
      {
      char *tmpPtr = NULL;
      char *tmpTokPtr = NULL;
      char tmpFeatureList[sizeof(VMCR->Vars)];
      mbool_t firstFeature = TRUE;

      tmpPtr = MUStrTok((char *)vptr,"|:",&tmpTokPtr);

      while (tmpPtr != NULL)
        {
        MUGetNodeFeature(tmpPtr,mVerify,&VMCR->FBM); 

        if (firstFeature)
          {
          tmpFeatureList[0] = '\0';
          MUStrCat(tmpFeatureList,"\"",sizeof(tmpFeatureList));
          firstFeature = FALSE;
          }
        else
          {
          MUStrCat(tmpFeatureList,",",sizeof(tmpFeatureList));
          }

        MUStrCat(tmpFeatureList,tmpPtr,sizeof(tmpFeatureList));

        tmpPtr = MUStrTok(NULL,"|:",&tmpTokPtr);
        }

      MUStrCat(tmpFeatureList,"\"",sizeof(tmpFeatureList));

      /* add feature values to vars as well. MOAB-2245 */

      if (VMCR->Vars[0])
        {
        MUStrCat(VMCR->Vars,"+",sizeof(VMCR->Vars));
        }

      MUStrCat(VMCR->Vars,"VMFEATURES:",sizeof(VMCR->Vars));
      MUStrCat(VMCR->Vars,tmpFeatureList,sizeof(VMCR->Vars));
      }
    else if (!strcasecmp(aptr,"hypervisor"))
      {
      /* hypervisor has been specified in arg list */

      mnode_t *tmpN = NULL;

      if (MNodeFind(vptr,&tmpN) == FAILURE)
        {
        snprintf(EMsg,EMsgSize,"ERROR:  cannot locate hypervisor '%s'\n",
          vptr);

        return(FAILURE);
        }

      if ((tmpN->HVType == mhvtNONE) || !bmisset(&tmpN->IFlags,mnifCanCreateVM))
        {
        snprintf(EMsg,EMsgSize,"ERROR:  specified server '%s' cannot create/manage VM's\n",
          vptr);

        return(FAILURE);
        }

      VMCR->N = tmpN;  /* use as hypervisor */
      }
    else if (!strcasecmp(aptr,"id"))
      {
      MUStrCpy(tmpVMID,vptr,sizeof(tmpVMID));
      MUStrCpy(VMCR->VMID,vptr,sizeof(VMCR->VMID));
      }
    else if (!strcasecmp(aptr,"image"))
      {
      VMCR->OSIndex = MUMAGetIndex(meOpsys,vptr,mVerify);

      if (VMCR->OSIndex <= 0)
        {
        snprintf(EMsg,EMsgSize,"ERROR:  unknown image specified\n");

        return(FAILURE);
        }
      }
    else if (!strcasecmp(aptr,"template"))
      {
      if (MTJobFind(vptr,&VMCR->JT) == FAILURE)
        {
        snprintf(EMsg,EMsgSize,"ERROR:  cannot locate specified template\n");

        return(FAILURE);
        }
      }
    else if (!strcasecmp(aptr,"variable"))
      {
      if (!MUStrIsEmpty(VMCR->Vars))  /* "feature" might have already added a variable */
        {
        MUStrCat(VMCR->Vars,"+",sizeof(VMCR->Vars));
        }

      MUStrCat(VMCR->Vars,vptr,sizeof(VMCR->Vars));
      }
    else if (!strcasecmp(aptr,"storage"))
      {
      MUStrCpy(VMCR->Storage,vptr,sizeof(VMCR->Storage));
      }
    else if (!strcasecmp(aptr,"ttl"))
      {
      /* Time to live */

      VMCR->Walltime = MUTimeFromString(vptr);
      }
    else if (!strcasecmp(aptr,"alias"))
      {
      /* Aliases (passed in, assumed to be valid on the network) */

      if (MUHTGet(&MNetworkAliasesHT,vptr,NULL,NULL) == SUCCESS)
        {
        /* Specified alias is already in use.  Do not allow */

        snprintf(EMsg,EMsgSize,"ERROR:  requested alias already in use\n");

        return(FAILURE);
        }

      /* Alias will be added to global table later on */

      MUStrCpy(VMCR->Aliases,vptr,sizeof(VMCR->Aliases));
      }
    else if (!strcasecmp(aptr,"trigger"))
      {
      /* Triggers are specified as 
          mvmctl -c trigger=x+y,trigger=a+b,hypervisor=node,image=os

         Saved internally as x+y,a+b */

      if (VMCR->Triggers[0] != '\0')
        MUStrCat(VMCR->Triggers,",",sizeof(VMCR->Triggers));

      MUStrCat(VMCR->Triggers,vptr,sizeof(VMCR->Triggers));
      }
    else if (!strcasecmp(aptr,"vc"))
      {
      mvc_t *tmpVC = NULL;

      if (MVCFind(vptr,&tmpVC) == FAILURE)
        {
        /* VC does not exist */

        snprintf(EMsg,EMsgSize,"ERROR:  VC '%s' does not exist\n",
          vptr);

        return(FAILURE);
        }

      if (bmisset(&tmpVC->Flags,mvcfDeleting) == TRUE)
        {
        snprintf(EMsg,EMsgSize,"ERROR:  VC '%s' is being deleted\n",
          tmpVC->Name);

        return(FAILURE);
        }

      MUStrCpy(VMCR->VC,vptr,sizeof(VMCR->VC));
      }
    } /* END while ((ptr = MUStrTok(VMString,",",&TokPtr)) != NULL) */

  return(SUCCESS);
  }  /* END MVMCRFromString() */
#endif

