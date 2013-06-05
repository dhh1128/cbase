/* HEADER */

/**
 * @file MVPC.c
 *
 * Moab Virtual Private Clouds
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  
 

/*
 *  Virtual Private Cluster Routines
 */

/**
 * Locate VPC by name.
 *
 * @see MVPCAdd() - peer
 * @see XXX() - peer - locate VPC Profile 
 *
 * @param PName (I)
 * @param PP (O) - found partition or next available partition slot [optional]
 * @param Extended (I)
 */

int MVPCFind(

  char    *PName,    /* I */
  mpar_t **PP,       /* O (optional) - found partition or next available partition slot */
  mbool_t  Extended) /* I */

  {
  /* If found, return success with P pointing to partition.     */
  /* If not found, return failure with P pointing to            */
  /* first free partition if available, P set to NULL otherwise */

  int pindex;

  mpar_t *P;

  if (PP != NULL)
    *PP = NULL;

  if ((PName == NULL) ||
      (PName[0] == '\0'))
    {
    return(FAILURE);
    }

  for (pindex = 0;pindex < MVPC.NumItems;pindex++)
    {
    P = (mpar_t *)MUArrayListGet(&MVPC,pindex);

    if ((P == NULL) || (P->Name[0] == '\0') || (P->Name[0] == '\1'))
      {
      continue;
      }

    if (strcmp(P->Name,PName) != 0)
      {
      if ((Extended == FALSE) || (P->RsvGroup == NULL) || (strcmp(P->RsvGroup,PName) != 0))
        continue;
      }

    /* partition found */

    if (PP != NULL)
      *PP = P;

    return(SUCCESS);
    }  /* END for (pindex) */

  return(FAILURE);
  }  /* END MVPCFind() */




/**
 * Add new VPC (virtual private cluster)
 *
 * @see MVPCFind() - peer
 * @see MVPCInitialize() - child
 *
 * @param PName (I)
 * @param PP (O) [optional]
 */

int MVPCAdd(

  char     *PName,
  mpar_t  **PP)

  {
  int     vindex;

  mpar_t *P;

  const char *FName = "MVPCAdd";

  MDB(4,fSTRUCT) MLog("%s(%s,PP)\n",
    FName,
    (PName != NULL) ? PName : "NULL");

  if ((PName == NULL) ||
      (PName[0] == '\0'))
    {
    return(FAILURE);
    }

  if (!MOLDISSET(MSched.LicensedFeatures,mlfVPC))
    {
    MMBAdd(&MSched.MB,"ERROR:  license does not allow VPCs, please contact Adaptive Computing\n",NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }

  for (vindex = 0;vindex < MVPC.NumItems;vindex++)
    {
    P = (mpar_t *)MUArrayListGet(&MVPC,vindex);

    if ((P != NULL) && !strcmp(P->Name,PName))
      {
      /* VPC already exists */

      if (PP != NULL)
        *PP = P;
 
      return(SUCCESS);
      }
 
    if ((P != NULL) &&
        ((P->Name[0] == '\0') ||
         (P->Name[0] == '\1')))
      {
      /* found cleared-out standing VPC slot */

      break;
      }
    }  /* END for(vindex) */

  if (vindex >= MVPC.NumItems)
    {
    mpar_t NewVPC = {{0}};

    /* append new VPC */

    if (MVPCCreate(PName,&NewVPC) == FAILURE)
      {
      MDB(1,fALL) MLog("ERROR:    cannot clear memory for VPC '%s'\n",
        PName);

      return(FAILURE);
      }

    MUArrayListAppend(&MVPC,&NewVPC);

    /* get pointer to newly added VPC */

    vindex = MVPC.NumItems - 1;
    P = (mpar_t *)MUArrayListGet(&MVPC,vindex);
    }

  /* available partition slot located */

  MUFree((char **)&P->L.IdlePolicy);
  MUFree((char **)&P->L.JobPolicy);

  if (P->S.IStat != NULL)
    {
    MStatPDestroy((void ***)&P->S.IStat,msoCred,MStat.P.MaxIStatCount);
    }

  MVPCInitialize(P,PName);

  if (P->L.IdlePolicy == NULL)
    MPUCreate(&P->L.IdlePolicy);

  if (P->L.JobPolicy == NULL)
    MPUCreate(&P->L.JobPolicy);

  if (PP != NULL)
    *PP = P;

  if (MSched.OptimizedCheckpointing == TRUE)
    {
    MOCheckpoint(mxoxVPC,(void *)P,TRUE,NULL);
    }

  return(SUCCESS);
  }  /* END MVPCAdd() */




/**
 * Create a vpc
 *
 * @param Name
 * @param VPC
 */

int MVPCCreate(

  char   *Name,
  mpar_t *VPC)

  {
  if (VPC == NULL)
    {
    return(FAILURE);
    }

  /* use static memory for now */

  memset(VPC,0,sizeof(mpar_t));

  if ((Name != NULL) && (Name[0] != '\0'))
    {
    MUStrCpy(VPC->Name,Name,sizeof(VPC->Name));
    }

  return(SUCCESS);
  }  /* END MVPCCreate() */


/**
 * Initialize VPC.
 *
 * @see MVPCAdd() - peer
 * @see MParSetDefaults() - child
 *
 * @param P (I)
 * @param PName (I)
 */

int MVPCInitialize(

  mpar_t  *P,     /* I */
  char    *PName) /* I */

  {
  mpar_t *tmpP;

  int index;

  const char *FName = "MVPCInitialize";

  MDB(4,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (PName != NULL) ? PName : "NULL");

  if (P == NULL)
    {
    return(FAILURE);
    }

  if (PName != NULL)
    memset(P,0,sizeof(mpar_t));

  if (PName != NULL)
    MUStrCpy(P->Name,PName,sizeof(P->Name));

  P->S.OType = mxoPar;
  P->S.Index = -1;

  P->IsVPC   = TRUE;

  for (index = 0;index < MVPC.NumItems;index++)
    {
    tmpP = (mpar_t *)MUArrayListGet(&MVPC,index);

    if (tmpP == P)
      {
      P->Index = index;

      MParSetDefaults(P);

      break;
      }
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MVPCInitialize() */





/**
 * Iterate through all VPC SRsvs and update the corresponding VPC partition.
 *
 * @param SR (I) [optional]
 */

int MVPCUpdate(

  msrsv_t *SR) /* I (optional) */

  {
  int        cindex;

  const char *FName = "MVPCUpdate";

  MDB(4,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (SR != NULL) ? SR->Name : "NULL");

  if (SR == NULL)
    {
    mpar_t *C;

    for (cindex = 0;cindex < MVPC.NumItems;cindex++)
      {
      C = (mpar_t *)MUArrayListGet(&MVPC,cindex);

      if (C->Name[0] == '\0')
        break;

      if (C->Name[1] == '\1')
        continue;

      if (!bmisset(&C->Flags,mpfIsDynamic))
        continue;

      if (C->PurgeTime <= 0)
        {
        mrsv_t *R;

        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if (MRsvGroupGetList(C->RsvGroup,&RList,NULL,0) == SUCCESS)
          {
          int rindex;

          /* determine purge time.  NOTE:  must be adjusted if rsv modified/extended */

          C->ProvStartTime = MMAX_TIME;
          C->ProvEndTime   = 0;


          for (rindex = 0;rindex < RList.NumItems;rindex++)
            {
            R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

            if (rindex == 0)
              C->VPCMasterRsv = R;

            C->ProvStartTime = MIN(R->StartTime,C->ProvStartTime);

            C->ProvEndTime = MAX(R->EndTime,C->ProvEndTime);
            }  /* END for (rindex) */

          if ((C->VPCProfile != NULL) && (C->VPCProfile->PurgeDuration > 0))
            C->PurgeTime = C->ProvEndTime + C->VPCProfile->PurgeDuration;
          else
            C->PurgeTime = C->ProvEndTime + MDEF_VPCPURGETIME;
          }

        MUArrayListFree(&RList);
        }    /* END if (C->PurgeTime <= 0) */

      if (MSched.Time < C->PurgeTime)
        {
        /* update state */

        /* STATES:  |--------|++++++++++++|ACTIVE|-------|+++++++++| */
        /*            Pending|Initializing|Active|Cleanup|Completed  */

        if (MSched.Time < C->ProvStartTime)
          {
          C->State = mvpcsPending;
          }
        else if (MSched.Time < C->ReqStartTime)
          {
          C->State = mvpcsInitializing;
          }
        else if (MSched.Time < C->ReqStartTime + C->ReqDuration)
          {
          C->State = mvpcsActive;
          }
        else if (MSched.Time < C->ProvEndTime)
          {
          C->State = mvpcsCleanup;
          }
        else
          {
          C->State = mvpcsCompleted;
          }

        continue;
        }

      /* if rsv group cannot be located */

      if (MRsvGroupGetList(C->RsvGroup,NULL,NULL,0) == SUCCESS)
        {
        /* resources still associated with VPC */

        continue;
        }

      /* destroy expired VPC */

      MVPCFree(&C);
      }  /* END for (cindex) */
    }    /* END if (SR == NULL) */

  return(SUCCESS);
  }   /* END int MVPCUpdate() */






/**
 * Update charge/costing associated with VPC.
 *
 * NOTE:  Generally called after dynamic modification of VPC attributes or
 *        resources.
 *
 * @see __MUISchedCtlModify() - parent
 *
 * @param V (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MVPCUpdateCharge(

  mpar_t *V,    /* I */
  char   *EMsg) /* O (optional,minsize=MMAX_LINE) */

  {
  int TC;
  int PC;
  int NC;

  mnode_t *N;

  mvpcp_t *P;

  marray_t  RList;

  mrsv_t  *R;

  double   Cost = 0.0;

  mulong   StartTime;
  mulong   Duration;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (V == NULL)
    {
    return(FAILURE);
    }

  MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

  if ((MRsvGroupGetList(V->RsvGroup,&RList,NULL,0) == SUCCESS) &&
      (RList.NumItems > 0))
    {
    R  = (mrsv_t *)MUArrayListGetPtr(&RList,0);

    TC = R->AllocTC;
    PC = R->AllocPC;
    NC = R->AllocNC;

    N = MNLReturnNodeAtIndex(&R->NL,0);
    }
  else
    {
    TC = 1;
    PC = 1;
    NC = 1;

    N = NULL;
 
    R = NULL;
    }

  P = V->VPCProfile;

  StartTime = V->ReqStartTime;

  if (V->ReqDuration > 0)
    Duration = V->ReqDuration;
  else if (R != NULL && P != NULL)
    Duration = (R->EndTime - P->ReqEndPad) - (R->StartTime + P->ReqStartPad);
  else
    Duration = 1;

  MRsvGetCost(
    NULL,
    P,
    StartTime,
    Duration,
    TC,
    PC,
    NC,
    NULL,
    N,
    FALSE,   /* query is for full rsv, not partial */
    NULL,
    &Cost);  /* O */

  if (V->Cost > 0.0)
    V->Cost = Cost;

  MUArrayListFree(&RList);

  return(SUCCESS);
  }  /* END MVPCUpdateCharge() */




int MVPCUpdateResourceUsage(

  mpar_t  *VPC,
  mcres_t *Usage)

  {
  mrsv_t *R;
  marray_t RList;

  /* must update VPC->R[0]->DRes and any system jobs */

  /* only support GRes on the first RSV right now */

  MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

  MRsvGroupGetList(VPC->RsvGroup,&RList,NULL,0);

  R = (mrsv_t *)MUArrayListGetPtr(&RList,0);

  MUArrayListFree(&RList);
 
  if (R != NULL)
    {
    MSNLCopy(&R->DRes.GenericRes,&Usage->GenericRes);

    if (R->J != NULL)
      MSNLCopy(&R->J->Req[0]->DRes.GenericRes,&Usage->GenericRes);
    }
   
  return(SUCCESS);
  }   /* END MVPCUpdateResourceUsage() */


/**
 * Add a TID to an existing VPC.
 *
 * NOTE: if the TID has a placeholder reservation this routine will NOT
 *       destroy the placeholder reservations, that needs to be handled
 *       by the parent function.
 *
 * @see MUIVPCModify() - parent
 * @see MUIVPCCreate() - parent
 *
 * @param VPC        (I/O) required
 * @param RsvProfile (I) optional
 * @param Auth       (I) optional
 * @param TID        (I) optional
 * @param RList      (I) optional
 * @param Response   (I) required
 */

int MVPCAddTID(

  mpar_t              *VPC,
  char                *RsvProfile,
  char                *Auth,
  char                *TID,
  marray_t            *RList,
  char                *Response)

  {
  char    tmpVal[MMAX_LINE];
  char    tmpLine[MMAX_LINE + 1];
  char    RsvGroup[MMAX_LINE + 1];
  char    OName[MMAX_LINE];

  mrsv_t *R;

  mxml_t *tRE;

  tRE = NULL;

  if ((VPC == NULL) ||
      (TID == NULL) ||
      (Response == NULL))
    {
    return(FAILURE);
    }

  /* create the XML RsvCreate request to hand to MUIRsvCreate() */

  MXMLCreateE(&tRE,(char*)MSON[msonRequest]);

  MS3AddSet(tRE,(char *)MRsvAttr[mraResources],TID,NULL);

  if ((RsvProfile != NULL) && (RsvProfile[0] != '\0'))
    MS3AddSet(tRE,(char *)MRsvAttr[mraProfile],RsvProfile,NULL);

  if (VPC->VPCProfile != NULL)
    MS3AddSet(tRE,(char *)MRsvAttr[mraSpecName],VPC->VPCProfile->Name,NULL);

  if (VPC->RsvGroup != NULL)
    MS3AddSet(tRE,(char *)MRsvAttr[mraRsvGroup],VPC->RsvGroup,NULL);

  if (VPC->ReqStartTime > 0)
    {
    char StartTime[MMAX_LINE];

    snprintf(StartTime,sizeof(StartTime),"%ld",VPC->ReqStartTime);

    MS3AddSet(tRE,(char *)MRsvAttr[mraStartTime],StartTime,NULL);
    }

  if (VPC->ReqDuration > 0)
    {
    char Duration[MMAX_LINE];

    snprintf(Duration,sizeof(Duration),"%ld",VPC->ReqDuration);

    MS3AddSet(tRE,(char *)MRsvAttr[mraDuration],Duration,NULL);
    }

  if (VPC->U != NULL)
    MS3AddSet(tRE,(char *)MRsvAttr[mraAUser],VPC->U->Name,NULL);

  if (VPC->G != NULL)
    MS3AddSet(tRE,(char *)MRsvAttr[mraAGroup],VPC->G->Name,NULL);

  if (VPC->A != NULL)
    MS3AddSet(tRE,(char *)MRsvAttr[mraAAccount],VPC->A->Name,NULL);

  if (VPC->Q != NULL)
    MS3AddSet(tRE,(char *)MRsvAttr[mraAQOS],VPC->A->Name,NULL);

  snprintf(OName,sizeof(OName),"%s:%s",
    MXO[VPC->OType],
    (VPC->OType == mxoQOS) ? ((mqos_t *)VPC->O)->Name : ((mgcred_t *)VPC->O)->Name);

  MS3AddSet(tRE,(char *)MRsvAttr[mraOwner],OName,NULL);

  /* disable MUICheckAuthorization in MUIRsvCreate() */

  if (MUIRsvCreate(
        tRE,
        Auth,
        tmpVal,                      /* O */
        sizeof(tmpVal),
        FALSE,
        TRUE,
        TRUE,
        RList) == FAILURE)  
    {
    /* cannot reserve VPC resources */

    MXMLDestroyE(&tRE);

    snprintf(Response,MMAX_BUFFER,"%s",tmpVal);

    return(FAILURE);
    }  /* END if (MUIRsvCreate() == FAILURE) */

  MXMLDestroyE(&tRE);

  /* NOTE:  tmpVal -> 'NOTE:  reservation system.1 created' */

  sscanf(tmpVal,"%1024s %1024s %64s %1024s",
    tmpLine,
    tmpLine,
    RsvGroup,
    tmpLine);

  if (MRsvFind(RsvGroup,&R,mraNONE) == FAILURE)
    {
    snprintf(Response,MMAX_BUFFER,"could not find reservation %s",RsvGroup);

    return(FAILURE);
    }

  MUStrDup(
    &VPC->RsvGroup,
    (R->RsvGroup != NULL) ? R->RsvGroup : R->Name);

  return(SUCCESS);
  }  /* END MVPCAddTID() */




int MVPCUpdateReservations(

  mpar_t   *VPC,
  marray_t *RList,
  mbool_t   DoCharge,
  char     *EMsg)

  {
  int rindex;

  char *tBPtr;
  int   tBSpace;

  char  VPCHostList[MMAX_BUFFER];
  char  tmpLine[MMAX_NAME];

  mrsv_t *R;

  /* determine purge time.  NOTE:  must be adjusted if rsv modified/extended */

  VPC->ProvStartTime = MMAX_TIME;
  VPC->ProvEndTime   = 0;

  MUSNInit(&tBPtr,&tBSpace,VPCHostList,sizeof(VPCHostList));

  for (rindex = 0;rindex < RList->NumItems;rindex++)
    {
    R = (mrsv_t *)MUArrayListGetPtr(RList,rindex);

    R->SubType = mrsvstVPC;

    VPC->ProvStartTime = MIN(R->StartTime,VPC->ProvStartTime);

    VPC->ProvEndTime = MAX(R->EndTime,VPC->ProvEndTime);

    bmset(&R->Flags,mrfIsVPC);

    if (VPC->A != NULL)
      {
      MRsvSetAttr(
        R,
        mraAAccount,
        (void *)VPC->A->Name,
        mdfString,
        mSet);
      }

#if 0  /* make this configurable, but for DISA, allow modifications */
    /* disallow direct removal of VPC reservations */

    bmset(&R->Flags,mrfParentLock);

    /* disallow direct modification/removal of VPC reservations */

    bmset(&R->Flags,mrfStatic);
#endif

    if (VPC->U != NULL)
      {
      char line[MMAX_LINE];

      MRsvSetAttr(
        R,
        mraAUser,
        (void *)VPC->U->Name,
        mdfString,
        mSet);

      snprintf(line,sizeof(line),"USER:%s",
        VPC->U->Name);

      MRsvSetAttr(
        R,
        mraOwner,
        (void *)line,
        mdfString,
        mSet);

      if (VPC->VPCProfile != NULL)
        {
        if (bmisset(&VPC->VPCProfile->Flags,mvpcfAutoAccess))
          {
          snprintf(line,sizeof(line),"USER==%s",
            VPC->U->Name);

          /* add requestor to rsv acl */

          MRsvSetAttr(
            R,
            mraACL,
            (void *)line,
            mdfString,
            mIncr);
          }
        }
      }  /* END if (VPC->U != NULL) */

    /* concatenate the HostLists */

    if (!MNLIsEmpty(&R->NL))
      {
      mnode_t *N;

      int nindex;

      mnl_t *NL = &R->NL;

      if (rindex == 0)
        {
        MUSNCat(&tBPtr,&tBSpace,"VPCHOSTLIST=");
        }
      else
        {
        MUSNCat(&tBPtr,&tBSpace,",");
        }

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        if (MNLGetNodeAtIndex(NL,nindex + 1,NULL) == FAILURE)
          {
          MUSNCat(&tBPtr,&tBSpace,N->Name);
          }
        else
          {
          MUSNPrintF(&tBPtr,&tBSpace,"%s,",
            N->Name);
          }
        }
      }    /* END if (RList[rindex]->NL != NULL) */

    if (R->T != NULL)
      { 
      mtrig_t *T;
      int      tindex;

      /* walk all triggers, adjust relative timeouts, add variables to T->Env */

      for (tindex = 0;tindex < R->T->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(R->T,tindex);

        if (MTrigIsValid(T) == FALSE)
          continue;

        if (bmisset(&T->InternalFlags,mtifTimeoutIsRelative))
          T->Timeout = R->EndTime - R->StartTime + T->Timeout;
        }
      }    /* END if (R->T != NULL) */

    MRsvSetAttr(
      R,
      mraVariables,
      (void *)VPCHostList,
      mdfString,
      mAdd);

    snprintf(tmpLine,sizeof(tmpLine),"VPCID=%s",VPC->Name);

    MRsvSetAttr(
      R,
      mraVariables,
      (void *)tmpLine,
      mdfString,
      mAdd);

    /* enforce VPC setup accounting manager charge */

    R->AllocResPending = TRUE;

    if (VPC->VPCProfile != NULL)
      {
      R->AllocPolicy = VPC->VPCProfile->ReAllocPolicy;
      }
    }      /* END for (rindex) */

  /* loop through all reservations and update global variables, including
     allocated resources */

  MVPCUpdateVariables(VPC);

  return(SUCCESS);
  }  /* END MVPCUpdateReservations() */



/**
 * Update VPC specific variables VPCHOSTLIST and VPCID
 *
 */

int MVPCUpdateVariables(

  mpar_t *VPC)

  {
  marray_t RList;

  MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

  if ((MRsvGroupGetList(VPC->RsvGroup,&RList,NULL,0) == SUCCESS) &&
      (RList.NumItems > 0))
    {
    char VPCID[MMAX_LINE];

    mnl_t NodeList;

    mstring_t NodeString(MMAX_LINE);

    mrsv_t *R;

    int rindex;

    MNLInit(&NodeList);

    for (rindex = 0;rindex < RList.NumItems;rindex++)
      {
      R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

      MNLMerge(&NodeList,&NodeList,&R->NL,NULL,NULL);
      } /* END for (rindex) */

    MStringAppend(&NodeString,"VPCHOSTLIST=");

    MNLToMString(&NodeList,FALSE,",",'\0',0,&NodeString);

    MNLFree(&NodeList);

    snprintf(VPCID,sizeof(VPCID),"VPCID=%s",VPC->Name);

    for (rindex = 0;rindex < RList.NumItems;rindex++)
      {
      R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

      MRsvSetAttr(R,mraVariables,NodeString.c_str(),mdfString,mAdd);
      MRsvSetAttr(R,mraVariables,VPCID,mdfString,mAdd);
      }
    }  /* END if (MRsvGroupGetList() == SUCCESS) */

  MUArrayListFree(&RList);

  return(SUCCESS);
  }  /* END MVPCUpdateVariables() */


/**
 * Decides whether one vpc can preempt another vpc.
 *
 * @param Preemptor
 * @param PreemptorJ (usually a psuedo job being scheduling during "mshow -a")
 * @param Preemptee
 * @param PreempteeR (the reservation that represents the VPC) 
 * @param EMsg
 */

mbool_t MVPCCanPreempt(

  const mvpcp_t *Preemptor,
  const mjob_t  *PreemptorJ,
  const mpar_t  *Preemptee,
  const mrsv_t  *PreempteeR,
  char          *EMsg)

  {
  if ((Preemptor  == NULL) ||
      (PreemptorJ == NULL) ||
      (Preemptee  == NULL) ||
      (PreempteeR == NULL)) 
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"invalid parameters to determine vpc preemption\n");

    return(FALSE);
    }

  if (bmisset(&PreempteeR->Flags,mrfIsActive))
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"VPC is active and cannot be preempted\n");

    return(FALSE); 
    } 

  if (Preemptee->Priority >= Preemptor->Priority)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"VPC has a higher priority and cannot be preempted\n");

    return(FALSE); 
    }
  
  if (EMsg != NULL)
    snprintf(EMsg,MMAX_LINE,"VPC preemption is possible\n");

  return(TRUE);
  }  /* END MVPCCanPreempt() */




/**
 * Modify a vpcs parameter and handle all ramifications of the change
 *
 * NOTE: these modifications can be user-driven or scheduler-driven
 *
 * @see __MUISchedCtlModify() - parent
 *
 * @param C        (I) [modified]
 * @param AIndex   (I)
 * @param AVal     (I)
 * @param Format   (I)
 * @param Mode     (I) [mSet,mIncr,mAdd,mDecr,mClear]
 * @param FlagBM   (I)
 * @param Msg      (O) [optional,minsize=MMAX_LINE]
 */

int MVPCModify(

  mpar_t                  *C,
  enum MParAttrEnum        AIndex,
  void                    *AVal,
  enum MDataFormatEnum     Format,
  enum MObjectSetModeEnum  Mode,
  mbitmap_t               *FlagBM,
  char                    *Msg)

  {
  if (Msg != NULL)
    Msg[0] = '\0';

  switch (AIndex)
    {
    case mpaDuration:

      {
      mrsv_t *R;

      marray_t RList;

      int rindex;

      mxml_t *E;
      mxml_t *SE;

      mulong tmpTime;
      mulong NewTime = 0;
      enum MObjectSetModeEnum Op;

      long StartTimes[MMAX_REQ_PER_JOB];
      long Durations[MMAX_REQ_PER_JOB];

      char TimeString[MMAX_NAME];

      switch (Mode)
        {
        case mAdd:
        case mDecr:

          if (Format == mdfString)
            tmpTime  = MUTimeFromString((char *)AVal);
          else
            tmpTime  = *(long *)AVal;

          Op = Mode;

          break;

        default:

          if (Msg != NULL)
            strcpy(Msg,"invalid operation");

          return(FAILURE);

          /* NOTREACHED */

          break;
        }  /* END switch (Mode) */

      MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

      MRsvGroupGetList(C->RsvGroup,&RList,NULL,0);

      snprintf(TimeString,sizeof(TimeString),"%ld",
        tmpTime);

      for (rindex = 0;rindex < RList.NumItems;rindex++)
        {
        E = NULL;

        R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

        StartTimes[rindex] = R->StartTime;
        Durations[rindex]  = R->EndTime - R->StartTime;

        MXMLCreateE(&E,"request");

        MXMLSetAttr(
          E,
          MSAN[msanAction],
          (void *)MRsvCtlCmds[mrcmModify],
          mdfString);

        if (bmisset(FlagBM,mcmForce))
          {
          MXMLSetAttr(E,MSAN[msanFlags],(void *)"force",mdfString);
          }
        
        MS3AddSet(E,(char *)MRsvAttr[mraDuration],TimeString,&SE);

        MXMLSetAttr(SE,MSAN[msanOp],(char *)MObjOpType[Op],mdfString);

        MS3AddWhere(E,(char *)MRsvAttr[mraName],R->Name,NULL);

        /* FIXME:  determine whether triggers should prevent this rsv from being modified */

        if (MUIRsvCtlModify(R,E,Msg) == FAILURE)
          {
          int bindex;

          /* need to roll back all other reservations in rsv group */

          for (bindex = rindex - 1;bindex >= 0;bindex--)
            {
            MRsvSetAttr(R,mraStartTime,&StartTimes[bindex],mdfLong,mSet);
            MRsvSetAttr(R,mraDuration,&Durations[bindex],mdfLong,mSet);
            }

          MXMLDestroyE(&E);

          return(FAILURE);
          }

        MXMLDestroyE(&E);

        NewTime = MAX(NewTime,R->EndTime - R->StartTime);

        if (R->T != NULL)
          {
          MOReportEvent((void *)R,R->Name,mxoRsv,mttCreate,R->CTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttStart,R->StartTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttEnd,R->EndTime,TRUE);
          } /* END if (R->T != NULL) */

        MRsvSyncSystemJob(R);
        }   /* END for (rindex) */

      C->ReqDuration = NewTime;

      if (C->ProvEndTime > 0)
        {
        C->ProvEndTime = C->ProvStartTime + C->ReqDuration;
        }

      if ((C->VPCProfile != NULL) && (C->VPCProfile->PurgeDuration > 0))
        C->PurgeTime = C->ProvEndTime + C->VPCProfile->PurgeDuration;
      else
        C->PurgeTime = C->ProvEndTime + MDEF_VPCPURGETIME;

      MUArrayListFree(&RList);
      }     /* END BLOCK (case mpaDuration) */

      break;

    case mpaProvStartTime:

      {
      mrsv_t *R;

      marray_t RList;

      int rindex;

      mxml_t *E;
      mxml_t *SE;

      mulong tmpTime;
      mulong NewTime = 0;
      enum MObjectSetModeEnum Op;

      long StartTimes[MMAX_REQ_PER_JOB];
      long Durations[MMAX_REQ_PER_JOB];

      char TimeString[MMAX_NAME];

      char EMsg[MMAX_LINE];

      if (C->ProvStartTime <= MSched.Time)
        {
        if (Msg != NULL)
          {
          snprintf(Msg,MMAX_LINE,"ERROR:    VPC '%s' is active\n",
            C->Name);
          }

        return(FAILURE);
        }

      switch (Mode)
        {
        case mAdd:
        case mDecr:

          /* tmpTime is time delta */

          if (Format == mdfString)
            tmpTime = MUTimeFromString((char *)AVal);
          else
            tmpTime = *(long *)AVal;

          if (C->ProvStartTime > 0)
            NewTime = C->ProvStartTime + tmpTime;

          Op = Mode;

          if (Mode == mAdd)
            NewTime = C->ProvStartTime + tmpTime;
          else
            NewTime = C->ProvStartTime - tmpTime;

          break;

        case mSet:
        default:

          if (Format == mdfString)
            tmpTime = MUStringToE((char *)AVal,(long int *)&tmpTime,TRUE);
          else
            tmpTime = *(long *)AVal;

          /* store absolute time */

          NewTime = tmpTime;

          /* find relative time */

          if (tmpTime < C->ProvStartTime)
            {
            tmpTime = C->ProvStartTime - tmpTime;

            Op = mDecr;
            }
          else if (tmpTime > C->ProvStartTime)
            {
            tmpTime = tmpTime - C->ProvStartTime;

            Op = mAdd;
            }
          else
            {
            return(SUCCESS);
            } 

          break;
        }  /* END switch (Mode) */

      MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

      MRsvGroupGetList(C->RsvGroup,&RList,NULL,0);

      snprintf(TimeString,sizeof(TimeString),"%ld",
        tmpTime);

      for (rindex = 0;rindex < RList.NumItems;rindex++)
        {
        E = NULL;

        R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

        StartTimes[rindex] = R->StartTime;
        Durations[rindex]  = R->EndTime - R->StartTime;

        MXMLCreateE(&E,"request");

        MXMLSetAttr(E,MSAN[msanAction],(void *)MRsvCtlCmds[mrcmModify],mdfString);
        
        MS3AddSet(E,(char *)MRsvAttr[mraStartTime],TimeString,&SE);

        MXMLSetAttr(SE,MSAN[msanOp],(char *)MObjOpType[Op],mdfString);

        MS3AddWhere(E,(char *)MRsvAttr[mraName],R->Name,NULL);

        /* TODO:  determine whether triggers should prevent this rsv from being modified */

        if (bmisset(&R->Flags,mrfTrigHasFired))
          {
          if (Msg != NULL)
            {
            snprintf(Msg,MMAX_LINE,"ERROR:    setup for VPC '%s' has already started\n",
              C->Name);
            }

          return(FAILURE);
          }

        if (MUIRsvCtlModify(R,E,EMsg) == FAILURE)
          {
          int bindex;

          /* need to roll back all other reservations in rsv group */

          for (bindex = rindex - 1;bindex >= 0;bindex--)
            {
            MRsvSetAttr(R,mraStartTime,&StartTimes[bindex],mdfLong,mSet);
            MRsvSetAttr(R,mraDuration,&Durations[bindex],mdfLong,mSet);
            }

          break;
          }

        if (R->T != NULL)
          {
          MOReportEvent((void *)R,R->Name,mxoRsv,mttCreate,R->CTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttStart,R->StartTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttEnd,R->EndTime,TRUE);
          } /* END if (R->T != NULL) */

        MRsvSyncSystemJob(R);
        }   /* END for (rindex) */

      if (NewTime > 0)
        C->ProvStartTime = NewTime;

      MUArrayListFree(&RList);
      }     /* END BLOCK (case mpaProvStartTime) */

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (aindex) */

  if (MSched.OptimizedCheckpointing == TRUE)
    {
    MOCheckpoint(mxoxVPC,(void *)C,TRUE,NULL);
    }

  return(SUCCESS);
  }  /* END MVPCModify() */

  
  
 



/**
 * Verify status of virtual private cluster (VPC) 
 *
 * @param SVPC
 */

int MVPCCheckStatus(

  mpar_t *SVPC)

  {
  int pindex;

  int CTok;

  mrange_t RRange[MMAX_REQ_PER_JOB];

  mxml_t  *AE;
  mxml_t  *RE;
  mxml_t  *PE;
  mxml_t  *CE;

  char    tmpLine[MMAX_LINE];
  char    tmpSVal[MMAX_LINE];
  char    tmpDVal[MMAX_LINE];
  char    TID[MMAX_NAME];
  char    RsvList[MMAX_LINE];
  char    EMsg[MMAX_LINE];

  mulong  STime;

  mpar_t *P;

  mbitmap_t Flags;
  
  bmset(&Flags,mcmVerbose);
  bmset(&Flags,mcmFuture);
  bmset(&Flags,mcmUseTID);
  bmset(&Flags,mcmSummary);

  for (pindex = 0;pindex < MVPC.NumItems;pindex++)
    {
    P = (mpar_t *)MUArrayListGet(&MVPC,pindex);

    if (P == NULL)
      break;

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if ((SVPC != NULL) && (SVPC != P))
      continue;

    if (P->MShowCmd == NULL)
      continue;

    if (P->ProvStartTime <= MSched.Time)
      continue;

    memset(RRange,0,sizeof(RRange));

    AE = NULL;
    RE = NULL;

    tmpLine[0] = '\0'; 

    EMsg[0] = '\0';

    MRsvGroupGetList(P->RsvGroup,NULL,RsvList,sizeof(RsvList));

    MXMLFromString(&RE,P->MShowCmd,NULL,NULL);

    snprintf(tmpLine,sizeof(tmpLine),"RSVACCESSLIST:%s",
      RsvList);

    MXMLAppendAttr(RE,(char *)MSAN[msanOption],tmpLine,',');

    MXMLCreateE(&AE,(char *)MSON[msonData]);

    if (MClusterShowARes(
          MSched.Admin[1].UName[0],
          mwpXML,          /* response format */
          &Flags,           /* query flags */
          RRange,          /* I (modified) */
          mwpXML,          /* source format */
          (void *)RE,      /* source data */
          (void **)&AE,    /* response data */
          0,
          0,
          NULL,
          EMsg) == FAILURE)
      {
      MDB(3,fSTRUCT) MLog("INFO:     Could not re-evaulate vpc '%s' (%s)\n",
        P->Name,
        EMsg);

      MXMLDestroyE(&AE);

      continue;
      }

    MDB(7,fSTRUCT)
      {
      mstring_t Result(MMAX_LINE);

      if (MXMLToMString(AE,&Result,(char const **) NULL,TRUE) == SUCCESS)
        {
        MLog("INFO:     current VPC starttime '%ld' current time '%ld\n",P->ProvStartTime,MSched.Time);
        MLog("INFO:     result from 'mshow -a' -- %s\n",Result.c_str());
        }
      }

    CTok = -1;

    if ((MXMLGetChild(AE,(char *)MXO[mxoPar],&CTok,&PE) == FAILURE) ||
        (MXMLGetChild(AE,(char *)MXO[mxoPar],&CTok,&PE) == FAILURE))
      {
      MXMLDestroyE(&AE);

      continue;
      }

    CTok = -1;

    STime = 0;

    while (MXMLGetChild(PE,"range",&CTok,&CE) == SUCCESS)
      {
      if ((MXMLGetAttr(CE,"starttime",NULL,tmpSVal,sizeof(tmpSVal)) == FAILURE) ||
          (MXMLGetAttr(CE,"duration",NULL,tmpDVal,sizeof(tmpDVal)) == FAILURE) ||
          (MXMLGetAttr(CE,"proccount",NULL,tmpLine,sizeof(tmpLine)) == FAILURE) ||
          (MXMLGetAttr(CE,"tid",NULL,TID,sizeof(TID)) == FAILURE))
        {
        /* invalid range */

        MXMLDestroyE(&AE);

        continue;
        }

      if (STime == 0)
        STime = strtol(tmpSVal,NULL,10);
      else
        STime = MIN(STime,(mulong)strtol(tmpSVal,NULL,10));

      }  /* END while (MXMLGetChild() == SUCCESS) */

    if (MSched.EnableVPCPreemption == TRUE)
      {
      if (STime == P->ProvStartTime)
        {
        MXMLDestroyE(&RE);
        MXMLDestroyE(&AE);

        continue;
        }
      }
    else if (STime >= P->ProvStartTime)
      {
      MXMLDestroyE(&RE);
      MXMLDestroyE(&AE);

      continue;
      }

    MDB(3,fSTRUCT) MLog("INFO:     Found new time for vpc '%s' (currently: %ld, new: %ld)\n",
      P->Name,
      P->ProvStartTime,
      STime);
      
    if (MVPCModify(P,mpaProvStartTime,&STime,mdfLong,mSet,0,EMsg) == FAILURE)
      {
      MDB(3,fSTRUCT) MLog("ALERT:    could not migrate vpc '%s' to new time (currently: %ld, new: %ld)\n",
        P->Name,
        P->ProvStartTime,
        STime);
      }

    MXMLDestroyE(&AE);
    MXMLDestroyE(&RE);
    }    /* END for (pindex) */

  return(SUCCESS);
  }  /* END MVPCCheckStatus() */








/**
 * Report virtual private cluster as XML.
 *
 * NOTE:  ACL/CL attributes reported as XML child elements.
 *
 * @see MVPCShow() - peer - report VPC in human readable format
 * @see MUISchedCtl() - parent - handle 'mschedctl -l vpc' request
 * @see MParAToString() - child
 *
 * @param P (I)
 * @param E (O)
 * @param SAList (I) [optional]
 */

int MVPCToXML(

  mpar_t            *P,      /* I */
  mxml_t            *E,      /* O */
  enum MParAttrEnum *SAList) /* I (optional) */

  {
  const enum MParAttrEnum DAList[] = {
    mpaID,
    mpaNONE };

  int  aindex;

  enum MParAttrEnum *AList;

  if ((P == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MParAttrEnum *)DAList;

  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mpaNONE;aindex++)
    {
    if (AList[aindex] == mpaVariables)
      {
      mrsv_t *R;

      /* extract variables from rsv group leader */

      if (MRsvFind(P->RsvGroup,&R,mraNONE) == FAILURE)
        break;

      MUAddVarsLLToXML(E,R->Variables);

      continue;
      }  /* END if (AList[aindex] == mpaVariables) */

    if (AList[aindex] == mpaACL)
      {
      marray_t RList;
   
      MUArrayListCreate(&RList,sizeof(mrsv_t *),10);
   
      if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
          (RList.NumItems > 0))
        {
        mxml_t *AE;
   
        macl_t *tACL;
   
        mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,0);
   
        for (tACL = R->CL;tACL != NULL;tACL = tACL->Next)
          {
          AE = NULL;
   
          MACLToXML(tACL,&AE,NULL,TRUE);
   
          MXMLAddE(E,AE);
          }  /* END for (aindex) */
        }    /* END if ((MRsvGroupGetList() == SUCCESS) && ...) */
   
      MUArrayListFree(&RList);

      continue;
      }  /* END if (List[aindex] == mpaACL) */

    if ((MParAToMString(P,AList[aindex],&tmpString,0) == FAILURE) ||
        (tmpString.empty()))
      {
      continue;
      }

    MXMLSetAttr(E,(char *)MParAttr[AList[aindex]],tmpString.c_str(),mdfString);
    }  /* END for (aindex) */

  if (P->MB != NULL)
    {
    mxml_t *ME = NULL;

    MXMLCreateE(&ME,(char *)MParAttr[mpaMessages]);

    MMBPrintMessages(P->MB,mfmXML,TRUE,-1,(char *)ME,0);

    MXMLAddE(E,ME);
    }

  return(SUCCESS);
  }  /* END MVPCToXML() */





/**
 * Report virtual private clusters in human-readable form
 *
 * NOTE:  Processes 'mschedctl -l vpc' request.
 *
 * @see MVPCToXML() - peer - report VPC as XML
 * @see MVPCProfileToString() - peer - report VPC Profile
 *  
 * @param P     (I)
 * @param Buf   (O) (must be initialized, will only append)
 * @param Flags (I) [bitmap of enum mcm*]
 */

int MVPCShow(

  mpar_t    *P,
  mstring_t *Buf)

  {
  mrsv_t *R = NULL;

  marray_t RList;

  if ((P == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MStringAppendF(Buf,"\nVPC %s (%s) ---------\n",
    P->Name,
    MClusterState[P->State]);

  MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

  if (MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS)
    {
    R = (mrsv_t *)MUArrayListGetPtr(&RList,0);
    }

  if (R != NULL)
    {
    MStringAppendF(Buf,"User:                  %s",
      (R->U != NULL) ? R->U->Name : "N/A");
    }

  if ((R != NULL) && (R->A != NULL))
    {
    MStringAppendF(Buf,"  (Account: %s)\n",
      R->A->Name);
    }
  else
    {
    MStringAppendF(Buf,"\n");
    }

  if (P->OType != mxoNONE)
    {
    MStringAppendF(Buf,"Owner:                 %s:%s\n",
      MXO[P->OType],
      (P->OID != NULL) ? P->OID : "NULL");
    }

  if (P->Priority != 0)
    {
    MStringAppendF(Buf,"Priority:              %ld\n",
      P->Priority);
    }

  {
  mstring_t tmp(MMAX_LINE);

  MParAToMString(P,mpaACL,&tmp,0);

  MStringAppendF(Buf,"ACL:                   %s\n",
    tmp.c_str());
  }

  if (R != NULL)
    {
    MStringAppendF(Buf,"Size:                  %d\n",
      R->AllocTC);

    MStringAppendF(Buf,"Task Resources:        %s\n",
      MCResToString(&R->DRes,0,mfmHuman,NULL));
    }

  if (P->ReqStartTime > 0)
    {
    char TString[MMAX_LINE];

    MULToTString(P->ReqStartTime - MSched.Time,TString);

    MStringAppendF(Buf,"Available Time:        %s (%s)\n",
      MULToATString(P->ReqStartTime,NULL),
      TString);
    }

  if (P->ReqDuration > 0)
    {
    char TString[MMAX_LINE];

    MULToTString(P->ReqDuration,TString);

    MStringAppendF(Buf,"Available Duration:    %s\n",
      TString);
    }

  if (P->ReqResources != NULL)
    {
    MStringAppendF(Buf,"Requested Resources:   %s\n",
      P->ReqResources);
    }

  if (P->ProvStartTime > 0)
    {
    char TString[MMAX_LINE];

    MULToTString(P->ProvStartTime - MSched.Time,TString);

    MStringAppendF(Buf,"Provision Start Time:  %s (%s)\n",
      MULToATString(P->ProvStartTime,NULL),
      TString);
    }

  if (P->ProvEndTime > 0)
    {
    char TString[MMAX_LINE];

    MULToTString(P->ProvEndTime - MSched.Time,TString);

    MStringAppendF(Buf,"Cleanup End Time:      %s (%s)\n",
      MULToATString(P->ProvEndTime,NULL),
      TString);
    }

  if (P->PurgeTime > 0)
    {
    MStringAppendF(Buf,"PurgeTime:             %s\n",
      MULToATString(P->PurgeTime,NULL));
    }

  if (P->MShowCmd != NULL)
    {
    MStringAppendF(Buf,"CmdLine:               %s\n",
      P->MShowCmd);
    }

  if (RList.NumItems > 0)
    {
    mrsv_t *tmpR;

    int     rindex;

    char    RLine[MMAX_LINE];

    /* list rsvs in group */

    RLine[0] = '\0';

    for (rindex = 0;rindex < RList.NumItems;rindex++)
      {
      tmpR = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

      if (rindex == 0)
        {
        strcat(RLine,"(");
        strcat(RLine,tmpR->Name);
        }
      else
        {
        strcat(RLine,",");
        strcat(RLine,tmpR->Name);
        }

      if ((P->RsvGroup != NULL) && !strcmp(tmpR->Name,P->RsvGroup))
        {
        mstring_t Vars(MMAX_LINE);

        /* examine master rsv */

        mln_t *L;

        MULLToMString(tmpR->Variables,TRUE,",",&Vars);

        MStringAppendF(Buf,"Variables:             %s\n",
          (!Vars.empty()) ? Vars.c_str() : "---");

        /* display access variables (ip,login,passwd) */

        for (L = tmpR->Variables;L != NULL;L = L->Next)
          {
          if (!strcmp(L->Name,"ip"))
            {
            MStringAppendF(Buf,"Access IP Address: %s",
              (char *)L->Ptr);
            }
          else if (!strcmp(L->Name,"login"))
            {
            MStringAppendF(Buf,"Access Login: %s",
              (char *)L->Ptr);
            }
          else if (!strcmp(L->Name,"passwd"))
            {
            MStringAppendF(Buf,"Access Password: %s",
              (char *)L->Ptr);
            }
          }    /* END for (L) */
        }      /* END if ((P->RsvGroup != NULL) && !strcmp(tmpR->Name,P->RsvGroup)) */
      }        /* END for (rindex) */ 

    if (RLine[0] != '\0')
      strcat(RLine,")");

    MStringAppendF(Buf,"Rsv Group:             %s %s\n",
      (P->RsvGroup != NULL) ? P->RsvGroup : "-",
      RLine);
    }    /* END if (RList.NumItems > 0) */

  if ((R != NULL) && (R->AllocPolicy != mralpNONE))
    {
    MStringAppendF(Buf,"Rsv ReAlloc Policy:    %s\n",
      MRsvAllocPolicy[R->AllocPolicy]);
    }

  if (P->VPCProfile != NULL)
    {
    MStringAppendF(Buf,"VPC Profile:           %s\n",
      P->VPCProfile->Name);

    /* NOTE:  explicitly requested resources are currently always located in the
              reservation group leader */

    if ((MAM[0].Type != mamtNONE) && (MAM[0].UseDisaChargePolicy == TRUE))
      {
      mrsv_t *tmpR;

      int     rindex;

      double tmpCost = 0.0;
      double Cost = 0.0;

      Cost += P->VPCProfile->SetUpCost;

      for (rindex = 0;rindex < RList.NumItems;rindex++)
        {
        tmpR = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

        MRsvGetCost(tmpR,P->VPCProfile,0,0,0,0,0,NULL,NULL,FALSE,NULL,&tmpCost);

        Cost += tmpCost;
        }

      MStringAppendF(Buf,"Total Charge:          %.2f\n",
        Cost);
      }
    else if (P->Cost > 0)
      {
      if ((R != NULL) && (R->LienCost > 0.0))
        {
        MStringAppendF(Buf,"Total Charge:          %.2f (Lien:  %.2lf)\n",
          P->Cost,
          R->LienCost);
        }
      else
        {
        MStringAppendF(Buf,"Total Charge:          %.2f\n",
          P->Cost);
        }
      }
    else if ((R != NULL) && (P->VPCProfile != NULL))
      {
      double cost;

      cost = P->VPCProfile->SetUpCost +
             P->VPCProfile->NodeSetupCost * R->AllocTC +
             P->VPCProfile->NodeHourAllocChargeRate * 
               R->AllocTC * 
               MAX(1,R->EndTime - R->StartTime) / MCONST_HOURLEN;

      MStringAppendF(Buf,"Total Charge:          %.2f\n",
        cost);
      }
    }    /* END if (P->ProfileID[0] != '\0') */

  if (P->MB != NULL)
    {
    char tmpBuf[MMAX_BUFFER];

    MMBPrintMessages(P->MB,mfmHuman,TRUE,-1,tmpBuf,sizeof(tmpBuf));

    MStringAppendF(Buf,tmpBuf);
    }

  MStringAppend(Buf,"\n\n");

  MUArrayListFree(&RList);

  return(SUCCESS);
  }  /* END MVPCShow() */








/**
 * Destroy/free existing VPC
 *
 * NOTE:  destroy associated reservation group after VPC is destroyed because 
 *        reservation may contain information (ie triggers) VPC is dependent 
 *        upon.
 *
 * @see MUISchedCtl() - parent
 * @see MRsvDestroyGroup() - child
 * @see MVPCFree() - child
 *
 * @param CP (I) [modified,cleared]
 * @param ArgString (I) - support 'hard' or 'soft'
 */

int MVPCDestroy(

  mpar_t     **CP,
  const char  *ArgString)

  {
  mpar_t  *C;

  long  tmpTime = 0;

  char     RsvGroupID[MMAX_NAME];

  C = *CP;

  /* if C is NULL, every path below dereferences it, so short circuit it now */
  if (C == NULL)
    return(FAILURE);

  if (C->RsvGroup != NULL)
    MUStrCpy(RsvGroupID,C->RsvGroup,sizeof(RsvGroupID));
  else
    RsvGroupID[0] = '\0';

  if (ArgString[0] != '\0')
    {
    if (strstr(ArgString,"soft"))
      {
      /* schedule deletion around REQENDPAD */

      if ((C->VPCProfile != NULL) &&
          (C->VPCProfile->ReqEndPad > 0))
        {
        tmpTime = MSched.Time + C->VPCProfile->ReqEndPad;

        MRsvGroupSetAttr(RsvGroupID,mraEndTime,(void *)&tmpTime,mdfLong,mSet);

        MRsvGroupSendSignal(RsvGroupID,mttEnd);

        MParSetAttr(C,mpaProvEndTime,(void **)&tmpTime,mdfLong,mSet);

        tmpTime = MSched.Time - C->ReqStartTime + C->VPCProfile->ReqEndPad;

        MParSetAttr(C,mpaDuration,(void **)&tmpTime,mdfLong,mSet);

        MSchedCheckTriggers(NULL,-1,NULL);
        }
      else
        {
        /* free reserved resources */

        MRsvGroupSendSignal(RsvGroupID,mttEnd);

        MSchedCheckTriggers(NULL,-1,NULL);

        /* remove associated rsv group */

        MRsvDestroyGroup(RsvGroupID,TRUE);

        MORemoveCheckpoint(mxoPar,C->Name,0,NULL);

        /* free VPC */

        MVPCFree(&C);
        }
      }    /* END if (strstr(ArgString,"soft")) */
    else if (strstr(ArgString,"hard"))
      {
      /* schedule cancellation around REQENDPAD */

      if ((C->VPCProfile != NULL) &&
          (C->VPCProfile->ReqEndPad > 0))
        {
        tmpTime = MSched.Time + C->VPCProfile->ReqEndPad;

        MRsvGroupSetAttr(RsvGroupID,mraEndTime,(void *)&tmpTime,mdfLong,mSet);

        MRsvGroupSendSignal(RsvGroupID,mttCancel);

        MParSetAttr(C,mpaProvEndTime,(void **)&tmpTime,mdfLong,mSet);

        tmpTime = MSched.Time - C->ReqStartTime + C->VPCProfile->ReqEndPad;

        MParSetAttr(C,mpaDuration,(void **)&tmpTime,mdfLong,mSet);

        MSchedCheckTriggers(NULL,-1,NULL);
        }
      else
        {
        /* free reserved resources */

        MRsvGroupSendSignal(RsvGroupID,mttCancel);

        MSchedCheckTriggers(NULL,-1,NULL);
 
        MRsvDestroyGroup(RsvGroupID,TRUE);

        MORemoveCheckpoint(mxoPar,C->Name,0,NULL);

        /* destroy VPC */

        MVPCFree(&C);
        }
      }    /* END else if (strstr(ArgString,"hard")) */
    }      /* END if (ArgString[0] != '\0') */
  else
    {
    /* free reserved resources */

    MRsvGroupSendSignal(RsvGroupID,mttCancel);

    MSchedCheckTriggers(NULL,-1,NULL);

    MRsvDestroyGroup(RsvGroupID,TRUE);

    MORemoveCheckpoint(mxoPar,C->Name,0,NULL);

    /* destroy VPC */

    MVPCFree(&C);
    }

  return(SUCCESS);
  }  /* END MVPCDestroy() */





/**
 * Free memory and destroy virtual private cluster (VPC).
 *
 * @see MVPCDestroy() - parent
 *
 * @param CP (I) [modified, contents freed]
 */

int MVPCFree(

  mpar_t **CP)  /* I (modified, contents freed) */

  {
  mpar_t *C;

  const char *FName = "MVPCFree";

  MDB(4,fSTRUCT) MLog("%s(CP)\n",
    FName);

  if ((CP == NULL) || (*CP == NULL))
    {
    return(FAILURE);
    }

  C = *CP;

  /* free dynamic attributes */

  MUFree(&C->Message);

  MUFree((char **)&C->L.IdlePolicy);
  MUFree((char **)&C->L.JobPolicy);
  MUFree((char **)&C->MShowCmd);

  if (C->S.IStat != NULL)
    {
    MStatPDestroy((void ***)&C->S.IStat,msoCred,MStat.P.MaxIStatCount);
    }

  /* mark VPC as free */

  C->Name[0] = '\1';

  return(SUCCESS);
  }  /* END MVPCFree() */









