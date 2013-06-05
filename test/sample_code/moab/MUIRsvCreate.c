/* HEADER */

/**
 * @file MUIRsvCreate.c
 *
 * Contains MUI Reservation Create
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

/**
 * Create requested admin/user reservation based on XML spec 
 *   (process 'mrsvctl -c').
 *
 * @see MUIRsvCtl() - parent
 * @see MUIVPCCreate() - parent
 * @see MRsvConfigure() - child - create new reservation
 *
 * NOTE: only supports human readable output
 *
 * Step 1.0  Initialize
 *   Step 1.1  Load 'Set' Attributes 
 *   Step 1.2  Load TID Attributes 
 *   Step 1.3  Check Validity of Each TID 
 *   Step 1.4  Load Profiles if Specified 
 * Step 2.0  Create all reservations (and possibly group them together) 
 *
 * @param RE        (I)
 * @param Auth      (I)
 * @param Buf       (O) [modified]
 * @param BufSize   (I)
 * @param CheckAuth (I)
 * @param AllowTrig (I)
 * @param IsVPC     (I)
 * @param RList     (I)
 */

int MUIRsvCreate(

  mxml_t              *RE,
  char                *Auth,
  char                *Buf,
  int                  BufSize,
  mbool_t              CheckAuth,
  mbool_t              AllowTrig,
  mbool_t              IsVPC,
  marray_t            *RList)

  {
  int STok;
  int rc;

  char *ptr;
  char *TokPtr;

  int  RCount;
  int  RIndex;

  char *BPtr;
  int   BSpace;

  char AName[MMAX_NAME];
  char AVal[MMAX_BUFFER];
  char GName[MMAX_NAME];

  int  TID[MMAX_REQ_PER_JOB];
  int  HighestTID = -1;

  mrsv_t *NewRsv[MMAX_REQ_PER_JOB];

  char TIDSpecs[MMAX_BUFFER];
  char ProfSpecs[MMAX_LINE];
  char Profiles[MMAX_REQ_PER_JOB][MMAX_NAME];
  mvc_t *VCMaster = NULL;
  mvc_t *VCs[MMAX_REQ_PER_JOB];

  enum MRsvAttrEnum AIndex;

  mbool_t IsAdmin = FALSE;

  mrsv_t tmpR;
  mrsv_t *R;

  mrsv_t *tR;
  mrsv_t *RProf = NULL;

  long    tmpDuration;
  long    tmpStartTime;
  long    tmpEndTime;

  char    tmpUName[MMAX_NAME];
  char    EMsg[MMAX_LINE] = {0};
  char    tmpLine[MMAX_LINE];

  char    SpecStartTime[MMAX_LINE]; /* user specified starttime (not inherited from TID or Profile) */
  char    SpecEndTime[MMAX_LINE];   /* user specified endtime (not inherited from TID or Profile) */
  char    SpecRsvGroup[MMAX_LINE];  /* user specified rsvgroup (not inherited from TID or Profile) */

  mbool_t UsingPlaceholder; /* if TRUE, this reservation group is removing placeholder reservations and creating real reservations */
  mbool_t ReqIsInvalid;
  mbool_t UseBestEffort;
  mbool_t IsPlaceholder;
  mbool_t ChangedStartTime = FALSE;
  mbool_t ResourceOnly = FALSE;      /* if true, only apply resource aspects
                                        of TIDs and RsvProfiles, (no triggers,
                                        etc) */
  mtrans_t *Trans;

  const char *FName = "MUIRsvCreate";

  MDB(3,fUI) MLog("%s(RE,Auth,Buf,%d,%s,%s,%s)\n",
    FName,
    BufSize,
    MBool[CheckAuth],
    MBool[AllowTrig],
    MBool[IsVPC]);

  MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if (MSched.Mode == msmSlave)
    {
    MUSNPrintF(&BPtr,&BSpace,"ERROR:  reservations cannot be created on slave cluster - re-issue command on master\n");

    return(FAILURE);
    }

  /* must search all Set children for the possibility of multiple profiles 
     and TIDs.  If any TID is specified, create RsvGroup */

  /* Step 1.0  Initialize */

  RCount = 0;

  TIDSpecs[0]  = '\0';
  ProfSpecs[0] = '\0';

  UsingPlaceholder = FALSE;

  for (RIndex = 0;RIndex < MMAX_REQ_PER_JOB;RIndex++)
    {
    TID[RIndex] = -1;
    Profiles[RIndex][0] = '\0';
    NewRsv[RIndex] = NULL;
    VCs[RIndex] = NULL;
    }  /* END for (RIndex) */

  /* Step 1.1  Load 'Set' Attributes */

  STok = -1;

  while (MS3GetSet(
      RE,
      NULL,
      &STok,
      AName,          /* O */
      sizeof(AName),
      AVal,           /* O */
      sizeof(AVal)) == SUCCESS)
    {
    if (strlen(AVal) >= sizeof(AVal) - 1)
      {
      /* value too large to fit in buffer */

      MUSNPrintF(&BPtr,&BSpace,"ERROR:    value specified for attribute %s is too long - max length=%d\n", 
        AName,
        (int)sizeof(AVal));

      return(FAILURE);
      }

    if (!strcmp(AName,MRsvAttr[mraResources]))
      {
      MUStrCpy(TIDSpecs,AVal,sizeof(TIDSpecs));

      continue;
      }

    if (!strcmp(AName,MRsvAttr[mraProfile]))
      {
      MUStrCpy(ProfSpecs,AVal,sizeof(ProfSpecs));

      continue;
      }
    }    /* END while (MS3GetSet() == SUCCESS) */

  /* Step 1.2  Load TID Attributes */

  if (isdigit(TIDSpecs[0]))
    {
    mbool_t Placeholder = FALSE;

    int rindex;

    char *tptr;

    Trans = NULL;

    /* FORMAT:  <TID>[:resources][,<TID>[:resources]]... */

    /* NOTE:  multiple TID's can be specified */

    ptr = MUStrTok(TIDSpecs,",",&TokPtr);

    while ((ptr != NULL) && (RCount < MMAX_REQ_PER_JOB))
      {
      if (isdigit(ptr[0]))
        {
        TID[RCount++] = strtol(ptr,&tptr,10);

        if ((tptr != NULL) && !strcasecmp(tptr,":resources"))
          {
          ResourceOnly = TRUE;
          }
        }

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while ((ptr != NULL) && (RCount < MMAX_REQ_PER_JOB)) */

    /* NOTE:  only support transaction ID's off of primary specified TID */

    /* NOTE:  if transaction specifies dependent TID's, ignore user-
              specified multiple TID's */

    if ((MTransFind(TID[0],Auth,&Trans) == SUCCESS) &&
        (Trans->Val[mtransaDependentTIDList] != NULL) &&
        (Trans->Val[mtransaDependentTIDList][0] != '\0'))
      {
      /* walk through all dependent TID's */

      MUStrCpy(tmpLine,Trans->Val[mtransaDependentTIDList],sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,",",&TokPtr);

      while ((ptr != NULL) && (RCount < MMAX_REQ_PER_JOB))
        {
        if (isdigit(ptr[0]))
          {
          TID[RCount++] = strtol(ptr,NULL,10);
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }

      if (RCount >= MMAX_REQ_PER_JOB)
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:    too many reservations per group (Max = %d)\n",
          MMAX_REQ_PER_JOB);

        return(FAILURE);
        }
      }    /* END if ((MTransFind() == SUCCESS) && ...) */
    else
      {
      /* TID is invalid */

      /* silently ignore? */
      }

    /* Step 1.3  Check Validity of Each TID */

    for (rindex = 0;rindex < RCount;rindex++)
      {
      if (MTransIsValid(TID[rindex],&Placeholder) == FALSE)
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:   TID '%d' is no longer valid, please query for available resources\n",
          TID[rindex]);

        return(FAILURE);
        }

      if (Placeholder == TRUE)
        UsingPlaceholder = TRUE;

      if (TID[rindex] > HighestTID)
        HighestTID = TID[rindex];
      }    /* END for (rindex) */
    }      /* END if (isdigit(TIDSpecs[0])) */
  else
    {
    RCount = 1;
    }      /* END else (isdigit(TIDSpecs[0])) */

  /* Step 1.4  Load Profiles if Specified */

  if (ProfSpecs[0] != '\0')
    {
    RIndex = 0;

    ptr = MUStrTok(ProfSpecs,",",&TokPtr);

    while (ptr != NULL)
      {
      MUStrCpy(
        Profiles[RIndex],
        (ptr == NULL) ? (char *)NONE : ptr,
        sizeof(Profiles[RIndex]));

      RIndex++;

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (ptr != NULL) */
    }    /* END if (ProfSpecs[0] != '\0') */

  GName[0] = '\0';

  /* Step 2.0  Create All Reservations (and possibly group them together) */

  for (RIndex = 0;RIndex < RCount;RIndex++)
    {
    tR = &tmpR;

    if ((Profiles[RIndex][0] != '\0') && 
        (MRsvProfileFind(Profiles[RIndex],&RProf) == FAILURE))
      {
      MDB(3,fUI) MLog("WARNING:  cannot locate RSVPROFILE '%s'\n",
        Profiles[RIndex]);

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot locate RSVPROFILE '%s'\n",
        Profiles[RIndex]);

      if (RCount > 1)
        {
        int rindex;

        for (rindex = 0;rindex < RIndex;rindex++)
          {
          MUSNPrintF(&BPtr,&BSpace,"          reservation %s destroyed\n",
            NewRsv[rindex]->Name);

          MRsvDestroy(&NewRsv[rindex],TRUE,TRUE);
          }  /* END for (rindex) */

        MUSNCat(&BPtr,&BSpace,"          all partial reservations destroyed\n");
        }  /* END if (RCount > 1) */

      return(FAILURE);
      }  /* END if ((Profiles[RIndex][0] != '\0') && ...) */

    MUIRsvInitialize(&tR,RProf,ResourceOnly);

    /* set all T->IsExtant == FALSE */
 
    if (tR->T != NULL)
      {
      mtrig_t *T;

      int tindex;

      for (tindex = 0;tindex < tR->T->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(tR->T,tindex);
 
        if (MTrigIsReal(T) == FALSE)
          continue;

        bmunset(&T->SFlags,mtfGlobalTrig);
        }
      }

    /* set admin rsv defaults */

    tR->Type = mrtUser;
    tR->DRes.Procs = -1;

    if (ResourceOnly == TRUE)
      bmset(&tR->Flags,mrfApplyProfResources);

    strcpy(tR->Name,"create_rsv");

    /* get attributes */

    AName[0] = '\0';

    tmpDuration  = -1;
    tmpStartTime = -1;
    tmpEndTime   = -1;

    UseBestEffort = FALSE;
    IsPlaceholder = FALSE;

    tmpUName[0]      = '\0';
    SpecStartTime[0] = '\0';
    SpecEndTime[0]   = '\0';
    SpecRsvGroup[0]  = '\0';

    MUserAdd(Auth,&tR->U);

    ReqIsInvalid = FALSE;

    STok = -1;

    /* First loop only checks for specific flags */

    while (MS3GetSet(
             RE,
             NULL,
             &STok,
             AName,
             sizeof(AName),
             AVal,
             sizeof(AVal)) == SUCCESS)
      {
      /* NOTE:  set size already checked previously */

      if ((AIndex = (enum MRsvAttrEnum)MUGetIndexCI(
             AName,
             MRsvAttr,
             FALSE,
             mraNONE)) == mraNONE)
        {
        /* unexpected rsv attribute */

        AName[0] = '\0';

        continue;
        }

      AName[0] = '\0';

      switch (AIndex)
        {
        case mraFlags:

          if (MUStrStr(AVal,"besteffort",0,TRUE,FALSE) != NULL)
            {
            UseBestEffort = TRUE;
            }

          if (MUStrStr(AVal,"placeholder",0,TRUE,FALSE) != NULL)
            {
            IsPlaceholder = TRUE;
            }

          break;

        default:

          /* NO-OP */

          break;
        }
      }   /* END while (MS3GetSet()) */

    STok = -1;

    while (MS3GetSet(
             RE,
             NULL,
             &STok,
             AName,
             sizeof(AName),
             AVal,
             sizeof(AVal)) == SUCCESS)
      {
      /* NOTE:  set size already checked previously */

      if ((AIndex = (enum MRsvAttrEnum)MUGetIndexCI(
             AName,
             MRsvAttr,
             FALSE,
             mraNONE)) == mraNONE)
        {
        /* unexpected rsv attribute */

        AName[0] = '\0';

        continue;
        }

      AName[0] = '\0';

      switch (AIndex)
        {
        case mraAUser:

          if (IsPlaceholder == TRUE)
            break;

          MUStrCpy(tmpUName,AVal,sizeof(tmpUName));

          tR->IsTracked = TRUE;

          break;

        case mraAQOS:
        case mraAGroup:
        case mraAAccount:

          if (IsPlaceholder == TRUE)
            break;

          MRsvSetAttr(tR,AIndex,(void *)AVal,mdfString,mSet);

          tR->IsTracked = TRUE;

          break;

        case mraDuration:

          /* NOTE:  starttime must be specified first */

          tmpDuration = MUTimeFromString(AVal);

          break;

        case mraEndTime:

          MUStrCpy(SpecEndTime,AVal,sizeof(SpecEndTime));

          /* in order to allow command line to override TID and Profile
             EndTime and StartTime are processed after everything else */

          break;

        case mraExcludeRsv:

          if (MRsvSetAttr(tR,mraExcludeRsv,(void *)AVal,mdfString,mAdd) == FAILURE)
            {

            MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot create requested reservation - no jobs listed to exclude\n",
                MRsvAttr[AIndex]);

            return(FAILURE);
            }

          break;

        case mraFlags:

          MRsvSetAttr(tR,mraFlags,(void *)AVal,mdfString,mAdd);

          if (IsPlaceholder == TRUE)
            bmset(&tR->Flags,mrfPlaceholder);

          break;

        case mraGlobalID:
 
          {
          mrsv_t *tmpRP;

          if (MRsvFind((char *)AVal,&tmpRP,mraGlobalID) == SUCCESS)
            {
            MUSNPrintF(&BPtr,&BSpace,"NOTE:  reservation %s created\n",
              (char *)AVal);

            return(SUCCESS);
            }
          else
            {
            MRsvSetAttr(tR,mraGlobalID,(void *)AVal,mdfString,mSet);
            }
          }

          break;

        case mraHostExp:

          {
          char *ptr;

          /* FORMAT:  <HOST>[,<HOST>]...||TASKS==<TASKCOUNT>||CLASS:<CLASSID> */

          /* make 'tasks' uppercase */

          if ((ptr = strstr(AVal,"tasks")) != NULL)
            strncpy(ptr,"TASKS",strlen("TASKS"));

          MRsvSetAttr(tR,mraHostExp,(void *)AVal,mdfString,mSet);

          if (strncasecmp(AVal,"TASKS==",strlen("TASKS==")) != 0)
            tR->HostExpIsSpecified = TRUE;
          }  /* END BLOCK */

          break;

        case mraPartition:

          if (MRsvSetAttr(tR,mraPartition,(void *)AVal,mdfString,mSet) == FAILURE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    error processing partition '%s'\n",
              AVal);

            ReqIsInvalid = TRUE;

            return(FAILURE);
            }

          break;

        case mraProfile:

          /* NO-OP (handled earlier) */

          break;

        case mraReqArch:

          MRsvSetAttr(tR,mraReqArch,(void *)AVal,mdfString,mVerify);

          break;
 
        case mraReqMemory:

          MRsvSetAttr(tR,mraReqMemory,(void *)AVal,mdfString,mVerify);

          break;

        case mraReqOS:

          MRsvSetAttr(tR,mraReqOS,(void *)AVal,mdfString,mVerify);

          break;
 
        case mraReqFeatureList:

          if (MRsvSetAttr(tR,mraReqFeatureList,(void *)AVal,mdfString,mVerify) == FAILURE)
            {
            if (RCount > 1)
              {
              MUSNPrintF(&BPtr,&BSpace,"requested feature '%s' does not exist (index = %d)\n",
                AVal,
                RIndex);
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"requested feature '%s' does not exist\n",
                AVal);
              }

            ReqIsInvalid = TRUE;

            continue;
            }

          /* note if feature was not added, return failure (feature does not exist anywhere) */

          break;

        case mraReqTaskCount:

          MRsvSetAttr(tR,mraReqTaskCount,(void *)AVal,mdfString,mSet);

          if (tR->MinReqTC == 0)
            bmset(&tR->Flags,mrfReqFull);
          else
            UseBestEffort = TRUE;

          break;

        case mraReqVC:

          {
          mvc_t *VC;

          if (MVCFind(AVal,&VC) == FAILURE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    requested VC '%s' does not exist\n",
              AVal);

            return(FAILURE);
            }

          if (MVCUserHasAccess(VC,tR->U) == FALSE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    user '%s' does not have access to VC '%s'\n",
              tR->U->Name,
              VC->Name);

            return(FAILURE);
            }

          if (bmisset(&VC->Flags,mvcfDeleting) == TRUE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    VC '%s' is currently being deleted\n",
              VC->Name);

            return(FAILURE);
            }

          MVCAddObject(VC,(void *)tR,mxoRsv);
          }

          break;

        case mraResources:

          if (TID[RIndex] != -1)
            {
            mtrans_t *Trans;

            /* 'transaction' based resource specified */

            MRsvSetAttr(tR,mraResources,&TID[RIndex],mdfInt,mSet);

            /* set ignstate here to avoid problems (RT7088) */

            bmset(&tR->Flags,mrfIgnState);

            tmpStartTime = tR->StartTime;
            tmpEndTime   = tR->EndTime;

            /* set all T->IsExtant == FALSE */
           
            if (tR->T != NULL)
              {
              mtrig_t *T;

              int tindex;
           
              for (tindex = 0;tindex < tR->T->NumItems;tindex++)
                {
                T = (mtrig_t *)MUArrayListGetPtr(tR->T,tindex);

                if (MTrigIsReal(T) == FALSE)
                  continue;
           
                bmunset(&T->SFlags,mtfGlobalTrig);
                }
              }


            if (MTransFind(TID[RIndex],NULL,&Trans) == SUCCESS)
              {
              /* NOTE: if a placeholder reservation already exists, it 
                        will be removed further down (search for 
                        MRsvDestroyGroup(Trans->RsvID,FALSE)) */

              if (!IsPlaceholder)
                {
                /* Search for VC info (but not for placeholder rsvs) */
                mstring_t OwnerS(MMAX_LINE);

                if ((Trans->Requestor != NULL) &&
                    (Trans->Requestor[0] != '\0'))
                  {
                  MStringSetF(&OwnerS,"USER:%s",Trans->Requestor);
                  }

                if ((Trans->Val[mtransaVCDescription] != NULL) &&
                    (Trans->Val[mtransaVCDescription][0] != '\0'))
                  {
                  /* Should be wrapped in a VC (can have nested layers) */

                  /* NOTE:  if the VCs are passed in out of order (one where
                      clause says a:b:c, another says c:b:a, the VC structure
                      will conform to the first one it found.  The VCs are 
                      only hooked up to parents when they are created. */

                  int vcindex;
                  char *VCPtr;
                  char *VCTok;

                  /* Multiple VC pointers because of the possibility of nested VCs */

                  mvc_t *tmpVC = NULL; /* VC to add object to */
                  mvc_t *topVC = NULL; /* VC to be added to the VCMaster (if there is one) */
                  mvc_t *lastVC = NULL;/* Last VC that was created */

                  /* Destructive parsing, but this value should not be used elsewhere */

                  VCPtr = MUStrTok(Trans->Val[mtransaVCDescription],":",&VCTok);

                  while (VCPtr != NULL)
                    {
                    for (vcindex = 0;vcindex < MMAX_REQ_PER_JOB;vcindex++)
                      {
                      if (VCs[vcindex] == NULL)
                        {
                        /* Found a spot, create it */

                        MVCAllocate(&VCs[vcindex],VCPtr,NULL);
                        tmpVC = VCs[vcindex];

                        /* VC was created automatically, remove when empty */

                        MVCSetAttr(tmpVC,mvcaFlags,(void **)MVCFlags[mvcfDestroyWhenEmpty],mdfString,mAdd);
                        MVCSetAttr(tmpVC,mvcaFlags,(void **)MVCFlags[mvcfDestroyObjects],mdfString,mAdd);

                        MVCSetAttr(tmpVC,mvcaOwner,(void **)OwnerS.c_str(),mdfString,mSet);

                        if (lastVC != NULL)
                          {
                          /* There was more than one VC, nest this VC in the last one (top-down order) */

                          MVCAddObject(lastVC,(void *)tmpVC,mxoxVC);
                          }

                        break;
                        }
                      else if (!strcmp(VCPtr,VCs[vcindex]->Description))
                        {
                        /* Found the VC */

                        tmpVC = VCs[vcindex];

                        break;
                        }
                      }  /* END for (vcindex) */

                    if (topVC == NULL)
                      topVC = tmpVC;

                    lastVC = tmpVC;

                    VCPtr = MUStrTok(NULL,":",&VCTok);
                    }  /* END while (VCPtr != NULL) */

                  MVCAddObject(tmpVC,(void *)tR,mxoRsv);

                  if (Trans->Val[mtransaVCMaster] != NULL)
                    {
                    /* add VC to master VC - there should be only one master VC, so
                        taking the first is fine */

                    if (VCMaster == NULL)
                      {
                      MVCAllocate(&VCMaster,Trans->Val[mtransaVCMaster],NULL);

                      MVCSetAttr(VCMaster,mvcaFlags,(void **)MVCFlags[mvcfDestroyWhenEmpty],mdfString,mAdd);
                      MVCSetAttr(VCMaster,mvcaFlags,(void **)MVCFlags[mvcfDestroyObjects],mdfString,mAdd);

                      MVCSetAttr(VCMaster,mvcaOwner,(void **)OwnerS.c_str(),mdfString,mSet);
                      }

                    MVCAddObject(VCMaster,(void *)topVC,mxoxVC);
                    }
                  }  /* END if (Trans->Val[mtransaVCDescription][0] != '\0') */
                else if (Trans->Val[mtransaVCMaster] != NULL)
                  {
                  /* rsv should be added directly to the master VC */

                  if (VCMaster == NULL)
                    {
                    MVCAllocate(&VCMaster,Trans->Val[mtransaVCMaster],NULL);

                    MVCSetAttr(VCMaster,mvcaFlags,(void **)MVCFlags[mvcfDestroyWhenEmpty],mdfString,mAdd);
                    MVCSetAttr(VCMaster,mvcaFlags,(void **)MVCFlags[mvcfDestroyObjects],mdfString,mAdd);

                    MVCSetAttr(VCMaster,mvcaOwner,(void **)OwnerS.c_str(),mdfString,mSet);
                    }

                  MVCAddObject(VCMaster,(void *)tR,mxoRsv);
                  }
                }  /* END if (!IsPlaceholder) */
              }  /* END if (MTransFind(TID[RIndex],NULL,&Trans) == SUCCESS) */

            if (IsPlaceholder == TRUE)
              bmset(&tR->Flags,mrfPlaceholder);
            }
          else
            {
            char *ptr;

            /* modify AVal to support lowercase resource spec */

            if ((ptr = strstr(AVal,"node")) != NULL)
              strncpy(ptr,"NODE",strlen("NODE"));
            
            if ((ptr = strstr(AVal,"procs")) != NULL)
              strncpy(ptr,"PROCS",strlen("PROCS"));

            if ((ptr = strstr(AVal,"swap")) != NULL)
              strncpy(ptr,"SWAP",strlen("SWAP"));

            if ((ptr = strstr(AVal,"mem")) != NULL)
              strncpy(ptr,"MEM",strlen("MEM"));

            if ((ptr = strstr(AVal,"disk")) != NULL)
              strncpy(ptr,"DISK",strlen("DISK"));

            if (MRsvSetAttr(tR,AIndex,(void *)AVal,mdfString,mSet) == FAILURE)
              {
              /* Failed to set the attribute, may have been GRES -
                   if MSched.EnforceGRESAccess is set, can't create GRES
                   via creating a reservation */

              MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot set attribute '%s' to '%s'\n",
                MRsvAttr[AIndex],
                AVal);

              ReqIsInvalid = TRUE;
              }
            }

          break;

        case mraRsvGroup:

          /* in order to allow command line to override TID and Profile
             rsvgroup is processed after everything else */

          MUStrCpy(SpecRsvGroup,AVal,sizeof(SpecRsvGroup));

          break;

        case mraRsvAccessList:
        case mraVMUsagePolicy:

          MRsvSetAttr(tR,AIndex,(void *)AVal,mdfString,mSet);

          break;

        case mraStartTime:

          MUStrCpy(SpecStartTime,AVal,sizeof(SpecStartTime));

          /* in order to allow command line to override TID and Profile
             EndTime and StartTime are processed after everything else */

          break;

        case mraVariables:

          if (MRsvSetAttr(tR,AIndex,(void *)AVal,mdfString,mAdd) == FAILURE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot set attribute '%s' to '%s'\n",
              MRsvAttr[AIndex],
              AVal);

            ReqIsInvalid = TRUE;
            }

          break;

        default:

          /* Items such as ACL can be passed down as
              separate "where" clauses, do mAdd */

          if (MRsvSetAttr(tR,AIndex,(void *)AVal,mdfString,mAdd) == FAILURE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot set attribute '%s' to '%s'\n",
              MRsvAttr[AIndex],
              AVal);

            ReqIsInvalid = TRUE;
            }

          break;
        }  /* END switch (AIndex) */
      }    /* END while (MXMLGetAttr() == SUCCESS) */

    if (SpecRsvGroup[0] != '\0')
      {
      MRsvSetAttr(tR,mraRsvGroup,(void *)SpecRsvGroup,mdfString,mSet);
      }

    if ((SpecStartTime[0] != '\0') &&
        (MUStringToE(SpecStartTime,&tmpStartTime,TRUE) != SUCCESS))
      {
      if (RCount > 1)
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:    invalid starttime specification (index = %d)\n",
          RIndex);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:    invalid starttime specification\n");
        }
 
      ReqIsInvalid = TRUE;
      }

    if ((SpecEndTime[0] != '\0') &&
        (MUStringToE(SpecEndTime,&tmpEndTime,TRUE) != SUCCESS))
      {
      if (RCount > 1)
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:    invalid endtime specification (index = %d)\n",
          RIndex);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:    invalid endtime specification\n");
        }

      ReqIsInvalid = TRUE;
      }

    if (ReqIsInvalid == TRUE)
      {
      return(FAILURE);
      }

    if ((!bmisclear(&tR->ReqFBM)) && (tR->HostExp == NULL) && (tR->ReqTC == 0))
      {
      /* grab all nodes with requested features */

      MUStrDup(&tR->HostExp,"ALL");
      }

    /* get authorization */

    if ((MSched.ForceRsvSubType == TRUE) && (tR->SubType == mrsvstNONE))
      {
      MUSNPrintF(&BPtr,&BSpace,"rsv subtype not specified/valid\n");

      return(FAILURE);
      }

    if ((CheckAuth == TRUE) && 
        (MUICheckAuthorization(
          tR->U,
          NULL,
          (void *)tR,
          mxoRsv,
          mcsMRsvCtl,
          mrcmCreate,
          &IsAdmin,
          NULL,
          0) == FAILURE))
      {
      MUSNPrintF(&BPtr,&BSpace,"user '%s' does not have access to reservation creation\n",
        Auth);

      return(FAILURE);
      }

    if (CheckAuth == FALSE)
      {
      IsAdmin = TRUE;
      }

    if ((IsAdmin == FALSE) && 
        (AllowTrig == FALSE) &&
        (tR->T != NULL))
      {
      /* triggers only allowed for admin users */

      MTListClearObject(tR->T,FALSE);

      MUFree((char **)&tR->T);
      }  /* END if ((IsAdmin == FALSE) && (tR->T != NULL)) */
    else if (IsPlaceholder == TRUE)
      {
      /* placeholder reservations should not have triggers */

      MTListClearObject(tR->T,FALSE);

      MUFree((char **)&tR->T);
      }

    if ((IsAdmin == TRUE) && (!MUStrIsEmpty(tmpUName)))
      {
      MRsvSetAttr(tR,mraAUser,tmpUName,mdfString,mSet);
      }
    else if (IsAdmin == FALSE)
      {
      bmset(&tR->Flags,mrfExcludeAll);

      MRsvSetAttr(tR,mraAUser,tR->U->Name,mdfString,mSet);

      MACLSet(&tR->ACL,maUser,tR->U->Name,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
      }

    if ((IsAdmin == FALSE) && (MCredCheckAcctAccess(tR->U,tR->G,tR->A) == FAILURE))
      {
      MUSNPrintF(&BPtr,&BSpace,"user '%s' does not have access to reservation credentials\n",
        Auth);

      return(FAILURE);
      }

    if (IsAdmin == FALSE)
      bmset(&tR->Flags,mrfPRsv);

    /* set rsv timeframe */

    if (tmpStartTime == -1)
      {
      tmpStartTime = MSched.Time;
      }
    else
      {
      if ((mulong)tmpStartTime < MSched.Time)
        {
        ChangedStartTime = TRUE;
        }
      
      tmpStartTime = MAX(MSched.Time,(mulong)tmpStartTime);
      }

    MRsvSetTimeframe(tR,mraStartTime,mSet,tmpStartTime,EMsg);

    /*
    NOTE: MRsvSetAttr(mraStartTime) calls MRsvAdjustTimeframe()
          which in turn modifies the RE table on nodes. This is 
          all handled by MRsvConfigure (further down) and SHOULD
          NOT happen here for a temporary reservation. 2/26/09 DRW

    MRsvSetAttr(tR,mraStartTime,(void *)&tmpStartTime,mdfLong,mSet);
    */

    if (tmpEndTime == -1)
      {
      if (tmpDuration == MMAX_TIME)
        tmpEndTime = MMAX_TIME;
      else if (tmpDuration != -1)
        tmpEndTime = tmpStartTime + tmpDuration;
      else
        tmpEndTime = tmpStartTime + MCONST_EFFINF;
      }

    MRsvSetAttr(tR,mraEndTime,(void *)&tmpEndTime,mdfLong,mSet);

    /* handle deadline-based reservation creation */

    if (bmisset(&tR->Flags,mrfDeadline))
      {
      if (MRsvCreateDeadlineRsv(tR,tmpDuration,tmpEndTime,EMsg) == FAILURE)
        {
        MUSNPrintF(&BPtr,&BSpace,"failure when attempting to schedule deadline reservation: '%s'\n",
          EMsg);

        return(FAILURE);
        }
      }

    R = NULL;

    /* by default, requires full resource allocation - 
       enable partial allocation using ??? */

    rc = MRsvConfigure(
          NULL,
          tR,    /* I (template - may contain ReqNL) */
          0,
          0,
          EMsg,  /* O */
          &R,    /* O (pointer to created rsv) */
          NULL,
          (IsAdmin == TRUE) ? FALSE : TRUE); /* check policies */

    if ((rc == SUCCESS) && (R != NULL))
      {
      if (IsVPC == TRUE)
        bmset(&R->Flags,mrfIsVPC);

      if ((tR->ReqTC > 0) && 
          (R->AllocTC < tR->ReqTC) && 
          (UseBestEffort == FALSE))
        {
        /* cannot allocate all requested tasks */

        sprintf(EMsg,"insufficient resources available (%d available)",
          R->AllocTC);

        rc = FAILURE;

        MRsvDestroy(&R,TRUE,TRUE);
        }
      else if ((tR->ReqTC > 0) &&
               (tR->MinReqTC > 0) &&
               (R->AllocTC < tR->MinReqTC))
        {
        /* cannot allocate all requested tasks */

        sprintf(EMsg,"insufficient resources available (%d available)",
          R->AllocTC);

        rc = FAILURE;

        MRsvDestroy(&R,TRUE,TRUE);
        }
      else
        {
        if ((long)R->StartTime < MUIDeadLine)
          {
          /* adjust UI phase wake up time */

          MUIDeadLine = MIN(MUIDeadLine,(long)R->StartTime);

          if (MSched.AdminEventAggregationTime >= 0)
            MUIDeadLine = MIN(MUIDeadLine,(long)MSched.Time + MSched.AdminEventAggregationTime);
          }
        }
      }    /* END if ((rc == SUCCESS) && (R != NULL)) */

    if (rc == FAILURE)
      {
      MRsvDestroy(&tR,FALSE,TRUE);

      MDB(3,fUI) MLog("WARNING:  cannot create requested reservation (%s)\n",
        EMsg);

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      if (strchr(EMsg,'\n'))
        {
        /* multi-line error message reported */

        MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot create requested reservation - could not allocate requested resources\n%s\n",
          EMsg);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot create requested reservation - %s\n",
          EMsg);
        }

      if (RCount > 1)
        {
        int rindex;

        for (rindex = 0;rindex < RIndex;rindex++)
          {
          MUSNPrintF(&BPtr,&BSpace,"          reservation %s destroyed\n",
            NewRsv[rindex]->Name);

          MRsvDestroy(&NewRsv[rindex],TRUE,TRUE);
          }  /* END for (rindex) */

        MUSNCat(&BPtr,&BSpace,"          all partial reservations destroyed\n");
        }

      return(FAILURE);
      }  /* END if (rc == FAILURE) */

    if (tR->ParentVCs != NULL)
      {
      /* Add to VC */

      mln_t *VCL;
      mvc_t *VC;

      for (VCL = tR->ParentVCs;VCL != NULL;VCL = VCL->Next)
        {
        VC = (mvc_t *)VCL->Ptr;

        MVCAddObject(VC,(void *)R,mxoRsv);
        }
      }

    MOSSyslog(LOG_INFO,"reservation %s created by %s",
      R->Name,
      Auth);

    NewRsv[RIndex] = R;

    if (MNLIsEmpty(&R->ReqNL))
      {
      /* move required node list to real rsv */

      MRsvSetAttr(R,mraReqNodeList,&R->NL,mdfOther,mSet);

      /*
      R->ReqNL = tR->ReqNL;
      tR->ReqNL = NULL;
      */
      }

    MTrigFreeList(tR->T);

    MRsvDestroy(&tR,FALSE,TRUE);

    if (R->StartTime <= MSched.Time)
      {
      MSysRaiseEvent();
      }
    else if (((long)R->StartTime - (long)MSched.Time) < MSched.MaxRMPollInterval)
      {
      /* update UI deadline */

      MUIDeadLine = MIN(MUIDeadLine,(long)R->StartTime);
      }

    /* set extended attributes */

    if ((TID[0] != -1) && (GName[0] == '\0'))
      {
      /* for rsv-groups, groupname is name of first reservation */

      if (R->RsvGroup == NULL)
        MUStrCpy(GName,R->Name,sizeof(GName));
      else
        MUStrCpy(GName,R->RsvGroup,sizeof(GName));
      }

    if ((ResourceOnly == FALSE) && bmisset(&R->Flags,mrfSystemJob))
      {
      MRsvSyncSystemJob(R);
      }

    if (TID[0] != -1)
      MUStrDup(&R->RsvGroup,GName);

    if ((R->T != NULL) && (IsPlaceholder == FALSE))
      {
      mtrig_t *T;

      int tindex;

      for (tindex = 0; tindex < R->T->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(R->T,tindex);

        if (MTrigIsValid(T) == FALSE)
          continue;

        /* update trigger OID with new RID */

        T->O = (void *)R;
        T->OType = mxoRsv;

        MUStrDup(&T->OID,R->Name);

        MTrigInitialize(T);
        }    /* END for (tindex) */

      /* report events for admin reservations */

      MOReportEvent((void *)R,R->Name,mxoRsv,mttCreate,R->CTime,TRUE);
      MOReportEvent((void *)R,R->Name,mxoRsv,mttStart,R->StartTime,TRUE);
      MOReportEvent((void *)R,R->Name,mxoRsv,mttEnd,R->EndTime,TRUE);
      }      /* END if (R->T != NULL) */

      /*
      MRsvCreateCredLock(R);
      */

    /* save cmdline args to print in events file and show in mdiag -r -v -v. */

    MXMLGetAttr(RE,"cmdline",NULL,AVal,sizeof(AVal));

    if (AVal[0] != '\0')
      MUStrDup(&R->CmdLineArgs,MUStringUnpack(AVal,NULL,-1,NULL));

    if (RList != NULL)
      {
      MUArrayListAppendPtr(RList,R);
      }

    MHistoryAddEvent(R,mxoRsv);

    snprintf(tmpLine,sizeof(tmpLine),"reservation %s successfully created by %s\n",
      R->Name,
      Auth);

    MOWriteEvent(R,mxoRsv,mrelRsvCreate,tmpLine,MStat.eventfp,NULL);

    CreateAndSendEventLogWithMsg(meltRsvCreate,R->Name,mxoRsv,(void *)R,tmpLine);

    MUSNPrintF(&BPtr,&BSpace,"NOTE:     reservation %s created\n",
      R->Name);

    if (ChangedStartTime == TRUE)
      {
      MUSNPrintF(&BPtr,&BSpace,"WARNING:  specified starttime is in the past and will be changed to now\n");

      ChangedStartTime = FALSE;
      }

    Trans = NULL;

    if ((TID[RIndex] != -1) &&
        (IsVPC == FALSE) &&
        (bmisset(&R->Flags,mrfPlaceholder)) &&
        (MTransFind(TID[RIndex],Auth,&Trans) == SUCCESS))
      {
      MUStrDup(&Trans->RsvID,R->Name);
      }

#if 0  /* not ready yet */
    if ((IsVPC == FALSE) &&
        (bmisset(&R->Flags,mrfIsVPC)))
      {
      MRsvToVPC(R,NULL);
      }
#endif

    MRsvTransition(R,FALSE);
    }  /* END for (RIndex) */

  /* append newly created rsv's to checkpoint file */

  /* we don't want to invalidate other transactions if we 
     created a VPC from a placeholder, invalidate in all
     other cases */

  if (UsingPlaceholder == TRUE)
    {
    /* NO-OP */

    if ((IsVPC == FALSE) &&
        (MTransFind(TID[0],NULL,&Trans) == SUCCESS))
      {
      /* we are consuming a placeholder reservation
         and we are not creating a VPC so we need to 
         remove the placeholder reservations here,
         for VPCs this is handled in MUIVPCCreate() */

      if (Trans->RsvID != NULL)
        {
        MRsvDestroyGroup(Trans->RsvID,FALSE);
 
        /* remove the RsvIDs for this Trans and all dependents */
 
        MTransInvalidate(Trans);
        }
      }
    }    /* END if (UsingPlaceholder == TRUE) */
  else if (HighestTID > -1)
    {
    MTransSetLastCommittedTID();
    }    /* else if (HighestTID > -1) */

  return(SUCCESS);
  }  /* END MUIRsvCreate() */
/* END MUIRsvCreate.c */
