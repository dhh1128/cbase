/* HEADER */

/**
 * @file MUIVM.c
 *
 * Contains MUI VM Create and Destroy
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

#if 0
/**
 * create VM's from admin request 
 *
 * @see MUINodeCtl() - parent
 * @see MUGenerateSystemJobs() - child
 */

int MUIVMCreate(

  msocket_t *S,          /* I */
  char      *Auth,       /* I */
  mnode_t   *N,          /* I (optional) */
  char      *VMID,       /* I (optional) */
  char      *ArgString)  /* I (modified as side-affect) */

  {
  char *TokPtr;

  char tmpVMID[MMAX_NAME];

  marray_t JArray;
  int      rc;

  char     tmpLine[MMAX_LINE];

  mvm_req_create_t VMCR;
  mrsv_t *AdvRsvR = NULL;
  mjob_t *StorageJ = NULL;

  mcres_t AvlRes;
  mcres_t VMCRes;
  mcres_t VMDRes;

  char    EMsg[MMAX_LINE] = {0};

  mvm_t *tV = NULL;

  mnl_t     NodeList;

  if (ArgString == NULL)
    {
    return(FAILURE);
    }

  tmpVMID[0] = '\0';

  if (MLicenseCheckVMUsage(EMsg) == FAILURE)
    {
    MDB(1,fRM) MLog(EMsg);

    MUISAddData(S,EMsg);

    return(FAILURE);
    }

  if (VMID != NULL)
    {
    MUStrCpy(tmpVMID,VMID,sizeof(tmpVMID));
    }

  MVMCreateReqInit(&VMCR);

  if (!strncasecmp(ArgString,"vm:",sizeof("vm:")))
    TokPtr = ArgString + strlen("vm:");
  else
    TokPtr = ArgString;

  /* FORMAT: ??? */

  if (MVMCRFromString(TokPtr,&VMCR,tmpLine,sizeof(tmpLine)) == FALSE)
    {
    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (MUserFind(Auth,&VMCR.U) == FAILURE)
    {
    MUISAddData(S,"ERROR:  invalid user\n");

    return(FAILURE);
    }

  if (VMCR.VC[0] != '\0')
    {
    mvc_t *tmpVC = NULL;

    /* VC was specified, need to see if user has access */

    if ((MVCFind(VMCR.VC,&tmpVC) == FAILURE) ||
        (MVCUserHasAccess(tmpVC,VMCR.U) == FALSE))
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  user '%s' does not have access to VC '%s'\n",
        (VMCR.U != NULL) ? VMCR.U->Name : "",
        VMCR.VC);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }

    if (bmisset(&tmpVC->Flags,mvcfDeleting) == TRUE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  VC '%s' is being deleted\n",
        tmpVC->Name);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }
    }  /* END if (VMCR.VC[0] != '\0') */

  if (tmpVMID[0] != '\0')
    {
    MUStrCpy(VMCR.VMID,tmpVMID,sizeof(VMCR.VMID));
    }
  else if (VMCR.VMID[0] != '\0')
    {
    MUStrCpy(tmpVMID,VMCR.VMID,sizeof(tmpVMID));
    }

  /* determine if a valid VMID was given */

  if (!MUStrIsEmpty(tmpVMID))
    {
    if (MUIsValidHostName(tmpVMID) == FALSE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot create vm '%s' - invalid hostname\n",
        tmpVMID);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }

    if (MVMFind(tmpVMID,&tV) == SUCCESS)
      {
      sprintf(tmpLine,"ERROR:  specified VMID already in use\n");

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }

    /* Check if for VMID's that have been requested but not created yet */

    if ((MSched.VMTrackingSubmitted != NULL) && (MSched.VMTracking == TRUE))
      {
      mln_t *Iter = NULL;
      mln_t *PrevIter = NULL;

      while (MULLIterate(MSched.VMTrackingSubmitted,&Iter) == SUCCESS)
        {
        mjob_t *OtherJ = NULL;

        if (MJobFind(Iter->Name,&OtherJ,mjsmExtended) == SUCCESS)
          {
          char *tmpVar;

          if ((MUHTGet(&OtherJ->Variables,"VMID",(void **)&tmpVar,NULL) == SUCCESS)  &&
              (!strcmp(tmpVar,tmpVMID)))
            {
            /* Other name has already been requested */

            sprintf(tmpLine,"ERROR:  specified VMID has already been requested\n");

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          if ((OtherJ->State == mjsRunning) ||
              (OtherJ->State == mjsCompleted))
            {
            /* Job has already started, VMID being used (not just submitted) */

            MULLRemove(&MSched.VMTrackingSubmitted,Iter->Name,NULL);

            /* NOTE: The iterator now points to freed memory caused by MULLRemove.
             *       Adjust the iterator back one element to get the next valid iteration */

            Iter = PrevIter;
            }
          }
        else
          {
          /* Job not found, remove from list */

          MULLRemove(&MSched.VMTrackingSubmitted,Iter->Name,NULL);
          Iter = PrevIter;
          }
        PrevIter = Iter;
        } /* END while (MULLIterate(MSched.VMTrackingSubmitted,&Iter) == SUCCESS) */
      } /* END if (MSched.VMTrackingSubmitted != NULL) */

    MUStrCpy(VMCR.VMID,tmpVMID,sizeof(VMCR.VMID));
    }
  else
    {
    MVMGetUniqueID(VMCR.VMID);
    }
         
  if (VMCR.JT != NULL)
    {
    if ((VMCR.CRes.Procs == 0) && (VMCR.JT->Req[0]->DRes.Procs > 0))
      VMCR.CRes.Procs = VMCR.JT->Req[0]->DRes.Procs;
             
    if ((VMCR.CRes.Mem == 0) && (VMCR.JT->Req[0]->DRes.Mem > 0))
      VMCR.CRes.Mem = VMCR.JT->Req[0]->DRes.Mem;
    }  /* END if (JT != NULL) */

  if (VMCR.OSIndex <= 0)
    {
    snprintf(tmpLine,sizeof(tmpLine),"VM creation requires that an image be specified\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (MSched.VMTracking == TRUE)
    {
    mjob_t tmpJ;
    mnl_t tmpNL;

    memset(&tmpJ,0,sizeof(tmpJ));

    MNLInit(&tmpNL);
    MNLClear(&tmpJ.ReqHList);

    if (VMCR.N != NULL)
      {
      if (MVMCRCheckNodeFeasibility(VMCR.N,&VMCR,tmpLine,sizeof(tmpLine)) == FAILURE)
        {
        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      MNLSetNodeAtIndex(&tmpNL,0,VMCR.N);
      MNLSetTCAtIndex(&tmpNL,0,1);
      MNLTerminateAtIndex(&tmpNL,1);
      }
    else if (VMCR.CRes.Procs < 1)
      {
      VMCR.CRes.Procs = 1;

      /* MUGenerateSystemJobs will need a job to get taskcount if no nodelist is given */

      tmpJ.Request.TC = 1;
      }
    else
      {
      tmpJ.Request.TC = VMCR.CRes.Procs;
      }

    /* Give tmpJ a name to not throw of valgrind (uninitialized value in MLog) */

    MUStrCpy(tmpJ.Name,"tmpTracking",MMAX_NAME);

    /* Create system job to be scheduled */

    MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

    rc = MUGenerateSystemJobs(
           (VMCR.U != NULL) ? VMCR.U->Name : NULL,
           (VMCR.N != NULL) ? NULL : &tmpJ,
           (VMCR.N != NULL) ? &tmpNL : NULL,
           msjtVMTracking,
           "vmtracking",
           0,
           NULL,
           &VMCR,
           FALSE,
           FALSE,
           NULL,
           &JArray);

    if (rc == SUCCESS)
      {
      mjob_t *J = *(mjob_t **)MUArrayListGet(&JArray,0);

      MVMChooseVLANFeature(J);

      MUStrDup(&J->System->VM,VMCR.VMID);

      snprintf(tmpLine,sizeof(tmpLine),"vm tracking job '%s' submitted\n",
        J->Name);

      MUISAddData(S,tmpLine);

      /* Adjust the procs - MUGenerateSystemJobs will create a job that is
          task based, not proc based.  If you asked for 4 procs, you would get
          4 tasks as well.  We want 1 task, 4 procs */

      J->Req[0]->TaskCount = 1;
      J->Req[0]->DRes.Procs = VMCR.CRes.Procs;
      J->Req[0]->TaskRequestList[0] = 1;
      J->Req[0]->TaskRequestList[1] = 1;

      /* determine the proxy user, if any, and annotate the system job */
        {
        char proxy[MMAX_NAME];

        if (MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy)))
          {
          MUStrDup(&J->System->ProxyUser, proxy);
          }
        } /* END block */
      }
    else
      {
      snprintf(tmpLine,sizeof(tmpLine),"Failed to create VM tracking job or dependent jobs.\n");

      MUISAddData(S,tmpLine);
      }

    MUArrayListFree(&JArray);
    MNLFree(&tmpNL);

    return(rc);
    } /* END if (MSched.VMTracking == TRUE) */
/***************************************END OF NEW VMTRACKING CODE**********************/

  /* validate request */

  N = VMCR.N;

  if (N == NULL)
    {
    snprintf(tmpLine,sizeof(tmpLine),"no valid hypervisor was specified\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if ((N->HVType == mhvtNONE) || !bmisset(&N->IFlags,mnifCanCreateVM))
    {
    snprintf(tmpLine,sizeof(tmpLine),"target hypervisor does not support VM creation\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (N->ARes.Procs < MAX(1,VMCR.CRes.Procs))
    {
    snprintf(tmpLine,sizeof(tmpLine),"inadequate available procs on server to create VM\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (MUOSListContains(N->VMOSList,VMCR.OSIndex,NULL) == FALSE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"target hypervisor does not support specified VM image\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  MCResInit(&VMCRes);
  MCResInit(&VMDRes);
  MCResInit(&AvlRes);
  MNodeGetVMRes(N,NULL,FALSE,&VMCRes,&VMDRes,NULL);

  MCResClear(&AvlRes);

  /* sync with calcuation in MJobGetINL for vmcreate and createvm jobs */

  MCResPlus(&AvlRes,&N->CRes);
  MCResMinus(&AvlRes,&N->DRes);
  MCResPlus(&AvlRes,&VMDRes);
  MCResMinus(&AvlRes,&VMCRes);

  MCResFree(&VMCRes);
  MCResFree(&VMDRes);
  if (MCResCanOffer(&AvlRes,&VMCR.CRes,TRUE,EMsg) == FALSE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"inadequate available resources on node to create VM - %s\n",
      EMsg);

    MUISAddData(S,tmpLine);

    MCResFree(&AvlRes);
    return(FAILURE);
    }
  MCResFree(&AvlRes);

  /* This is taken care of in MCResCanOffer */
  /*
  if (N->ARes.Mem <= MAX(1,VMCR.CRes.Mem))
    {
    snprintf(tmpLine,sizeof(tmpLine),"inadequate available memory on node to create VM\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }*/

  VMCR.StorageRsvs = NULL;

  /* Create storage job */

  if (VMCR.Storage[0] != '\0')
    {
    char proxy[MMAX_NAME];

    if (MVMCreateStorage(&VMCR,&AdvRsvR,&StorageJ,tmpLine,sizeof(tmpLine),Auth) == FAILURE)
      {
      if (tmpLine[0] != '\0')
        MUISAddData(S,tmpLine);

      return(FAILURE);
      }

    assert(StorageJ != NULL);

    if (MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy)))
      {
      MUStrDup(&StorageJ->System->ProxyUser, proxy);
      }
    }  /* END if (VMCR.Storage[0] != '\0') */

  /* should disk be enforced at this point as disk may be dynamically allocated? */

  /* create VM */

  MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

  MNLInit(&NodeList);

  MNLSetNodeAtIndex(&NodeList,0,N);
  MNLSetTCAtIndex(&NodeList,0,1);
  MNLTerminateAtIndex(&NodeList,1);

  rc = MUGenerateSystemJobs(
      (VMCR.U != NULL) ? VMCR.U->Name : NULL,
      NULL,
      &NodeList,
      msjtVMCreate,
      "vmcreate",
      0,
      NULL,
      &VMCR,
      FALSE,
      FALSE,
      NULL,
      &JArray);

  MNLFree(&NodeList);

  if (rc == SUCCESS)
    {
    mjob_t *J = *(mjob_t **)MUArrayListGet(&JArray,0);

    assert(J != NULL);
    assert(J->System != NULL);

    snprintf(tmpLine,sizeof(tmpLine),"VM create job '%s' submitted\n",
      J->Name);

    J->System->RecordEventInfo = MUStrFormat("requested by %s", 
      (Auth == NULL) ? "UNKNOWN" : Auth);



    /* determine the proxy user, if any, and annotate the system job */
    {
      char proxy[MMAX_NAME];

      if (MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy)))
        {
        MUStrDup(&J->System->ProxyUser, proxy);
        }
    }

    if (AdvRsvR != NULL)
      {
      /* Set advres so job only runs when storage has been reserved */

      /* Can't do advres because then the create job won't be able to run
          because it will be restricted to just the rsv resources, which 
          may be on a different node.  Just use earliest start time */

      J->SMinTime = AdvRsvR->StartTime;
      }

    if (StorageJ != NULL)
      {
      char Dep[MMAX_LINE];

      snprintf(Dep,sizeof(Dep),"afterok:%s",
        StorageJ->Name);

      MJobAddDependency(J,Dep);
      }
    } /* END if (rc == SUCCESS) */
  else
    {
    snprintf(tmpLine,sizeof(tmpLine),"VM create job submission failed\n");
    }

  MVMCreateReqFree(&VMCR);

  MUISAddData(S,tmpLine);

  MUArrayListFree(&JArray);

  return(rc);
  }  /* END MUIVMCreate() */
#endif 




/**
 * destroy specified VM
 *
 * @see MUINodeCtl() - parent
 * @see MUIVMCreate() - peer
 *
 * @param V      (I)
 * @param N      (I)
 * @param Auth   (I)
 * @param CFlags (I)
 * @param Buf    (O) [optional,minsize=MMAX_LINE]
 * @param EMsg   (O) [optional,minsize=MMAX_LINE]
 * @param SysJob (O) [optional]
 */

int MUIVMDestroy(

  mvm_t     *V,       /* I */
  mnode_t   *N,       /* I */
  char      *Auth,    /* I */
  mbitmap_t *CFlags,  /* I */
  char      *Buf,     /* O (optional,minsize=MMAX_LINE) */
  char      *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  msysjob_t **SysJob) /* O (optional) */

  {
  mbitmap_t     AuthBM;

  char     tmpLine[MMAX_LINE];
  char     tmpErr[MMAX_LINE];

  mbool_t  CanDelete;

  mbitmap_t MCMFlags;

  if (Buf != NULL)
    Buf[0] = '\0';

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SysJob != NULL)
    *SysJob = NULL;

  if (V == NULL)
    {
    return(FAILURE);
    }

  MSysGetAuth(Auth,mcsMVMCtl,0,&AuthBM);

  if (!bmisset(&AuthBM,mcalAdmin1))
    {
    snprintf(tmpErr,MMAX_LINE,"ERROR:  request not authorized");

    MUStrCpy(EMsg,tmpErr,MMAX_LINE);

    return(FAILURE);
    }

  if (bmisset(CFlags,mcmForce))
    bmset(&MCMFlags,mcmForce);

  if (bmisset(&V->Flags,mvmfDestroySubmitted))
    {
    snprintf(tmpErr,MMAX_LINE,"ERROR:  A VM destroy job has already been created for the given VM");

    MUStrCpy(EMsg,tmpErr,MMAX_LINE);

    return(FAILURE);
    }

  CanDelete = MVMCanRemove(V,tmpErr);

  if (CanDelete == TRUE)
    {
    /* determine if workload is running on VM */

    if ((V->JobID[0] != '\0') && !bmisset(&V->Flags, mvmfSovereign))
      {
      mjob_t *J = NULL;

      MJobFind(V->JobID,&J,mjsmExtended);

      if ((J != NULL) && (J->System != NULL) &&
          (J->System->JobType == msjtVMTracking))
        {
        /* NO-OP */

        /* job is VM tracking job, can destroy VM */
        }
      else if (bmisset(CFlags,mcmForce))
        {
        if (J != NULL)
          {
          /* NOTE:  potential race condition exists - should remove VM AFTER job cancellation verified from RM */

          MJobCancel(
            J,
            "MOAB_INFO:  parent VM destroyed\n",
            FALSE,
            NULL,
            NULL);
          }
        }
      else
        {
        snprintf(tmpErr,MMAX_LINE,"VM currently running workload");

        CanDelete = FALSE;
        }
      }
    }

  if (V->TrackingJ != NULL)
    {
    mjob_t *tmpDestroy = NULL;

    if ((MTJobCreateByAction(V->TrackingJ,mtjatDestroy,NULL,&tmpDestroy,Auth) == SUCCESS) &&
        (tmpDestroy != NULL))
      {
      if (Buf != NULL)
        {
        snprintf(Buf,MMAX_LINE,"successfully submitted job %s\n",
          tmpDestroy->Name);
        }

      if (SysJob != NULL)
        {
        *SysJob = tmpDestroy->System;
        }

      CreateAndSendEventLog(meltVMCancel,V->VMID,mxoxVM,(void *)V);

      return(SUCCESS);
      }
    }  /* END if (V->TrackingJ != NULL) */

  if ((V->OutOfBandRsv != NULL) && (V->TrackingJ == NULL))
    {
    char tmpID[MMAX_NAME];

    MUStrCpy(tmpID,V->VMID,sizeof(tmpID));

    /* handle VMs that have no tracking job */

    MVMDestroy(&V,NULL);

    if (Buf != NULL)
      {
      snprintf(Buf,MMAX_LINE,"out-of-band vm '%s' destroyed\n",
        tmpID);
      }

    return(SUCCESS);
    }  /* END if ((V->OutOfBandRsv != NULL) && (V->TrackingJ == NULL)) */

#if 0
  if (CanDelete == TRUE)
    {
    mnl_t tmpNL;

    MNLInit(&tmpNL);

    MNLSetNodeAtIndex(&tmpNL,0,V->N);
    MNLSetTCAtIndex(&tmpNL,0,V->CRes.Procs);
    MNLTerminateAtIndex(&tmpNL,1);

    MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

    if (MUGenerateSystemJobs(
         Auth,
         NULL,
         &tmpNL,
         msjtVMDestroy,
         "vmdestroy",
         0,
         V->VMID,
         V,
         FALSE,
         FALSE,
         &MCMFlags,
         &JArray) == FAILURE)
      {
      snprintf(tmpErr,MMAX_LINE,"cannot create vmdestroy request");

      CanDelete = FALSE;
      }

    MNLFree(&tmpNL);

    bmset(&V->Flags,mvmfDestroySubmitted);
    }

  if (CanDelete == FALSE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"cannot delete VM %s: %s.\n",
      V->VMID,
      tmpErr);

    MDB(1,fSTRUCT) MLog("ALERT:    %s",
      tmpLine);

    if (EMsg != NULL)
      MUStrCpy(EMsg,tmpLine,MMAX_LINE);

    return(FAILURE);
    }

  J = *(mjob_t **)MUArrayListGet(&JArray,0);

  assert(J != NULL);
  assert(J->System != NULL);

  if (Buf != NULL)
    {
    snprintf(Buf,MMAX_LINE,"successfully submitted job %s\n",
      J->Name);
    }

  J->System->RecordEventInfo = MUStrFormat("requested by %s",
    (Auth == NULL) ? "UNKNOWN" : Auth);

  CreateAndSendEventLog(meltVMCancel,V->VMID,mxoxVM,(void *)V);

  if (SysJob != NULL)
    {
    *SysJob = J->System;
    }

  MUArrayListFree(&JArray);
 
  return(SUCCESS);
#endif

  snprintf(tmpLine,sizeof(tmpLine),"cannot delete VM %s\n",
    V->VMID);

  return(FAILURE);
  }  /* END MUIVMDestroy() */
/* END MUIVM.c */
