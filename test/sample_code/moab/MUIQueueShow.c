/* HEADER */

/**
 * @file MUIQueueShow.c
 *
 * Contains MUI Queue Show functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  





/**
 * compare jobs by completion time, ordered low to high
 *
 * @param A (I)
 * @param B (I)
 */

int MJobCompletionTimeCompLToH(

  mjob_t **A,  /* I */
  mjob_t **B)  /* I */

  {
  /* order low to high */

  return((*A)->CompletionTime - (*B)->CompletionTime);
  }  /* END MJobCompletionTimeCompLToH() */




/**
 * Get comments/notes for showq based on scheduler/RM state and 
 * add each note as a child xml element. 
 *
 * @param   IsAdmin   (I)
 * @param   PE        (O)
 */

int MUIGetShowqNotes(

    mbool_t     IsAdmin,  /* I */
    mxml_t     *PE)       /* O */

  {
  int rmindex;

  mxml_t   *CE;
  mrm_t    *DownRM = NULL;
  mbool_t   ActiveRMFound = FALSE; 

  char     tmpLine[MMAX_LINE];


  if (PE == NULL)
    return(FAILURE);

  tmpLine[0] = '\0';

  if (MSched.State == mssmStopped)
    {
    /* NOTE:  indicate duration of stop (NYI) */

    if (IsAdmin == TRUE)
      {
      char TString[MMAX_LINE];

      MULToTString(MSched.Time - MSched.StateMTime,TString);

      sprintf(tmpLine,"NOTE:  scheduling has been stopped by admin %s for %s",
        (MSched.StateMAdmin != NULL) ? MSched.StateMAdmin : "",
        TString);
      }
    else
      {
      sprintf(tmpLine,"NOTE:  scheduling is stopped");
      }
    }
  else if (MSched.State == mssmPaused)
    {
    if (MSched.Mode == msmSlave)
      {
      sprintf(tmpLine,"NOTE:  local scheduling is disabled (%s mode)",
        MSchedMode[MSched.Mode]);
      }
    else if (MSched.ResumeTime < MSched.Time)
      {
      if (IsAdmin == TRUE)
        {
        char TString[MMAX_LINE];

        MULToTString(MSched.Time - MSched.StateMTime,TString);

        sprintf(tmpLine,"NOTE:  scheduling has been paused by admin %s for %s",
          (MSched.StateMAdmin != NULL) ? MSched.StateMAdmin : "N/A",
          TString);
        }
      else
        {
        sprintf(tmpLine,"NOTE:  scheduling is paused");
        }
      }
    else
      {
      char TString[MMAX_LINE];
      /* NOTE:  indicate duration of pause */

      MULToTString(MSched.ResumeTime - MSched.Time,TString);

      sprintf(tmpLine,"NOTE:  scheduling is paused for %s",
        TString);
      }
    }
  else if ((MSched.Mode == msmTest) || (MSched.Mode == msmMonitor))
    {
    sprintf(tmpLine,"NOTE:  scheduling is disabled (%s mode)",
      MSchedMode[MSched.Mode]);
    }

  if (!MUStrIsEmpty(tmpLine))
    {
    MXMLAddChild(PE,"comment",NULL,&CE);
    MXMLSetAttr(CE,"note",(void *)tmpLine,mdfString);
    }

  if ((MSched.SRsvIsActive == TRUE) && (!MUIsGridHead()))
    {
    /* NOTE:  indicate duration of system rsv (NYI) */

    sprintf(tmpLine,"NOTE:  system reservation blocking all nodes");

    MXMLAddChild(PE,"comment",NULL,&CE);
    MXMLSetAttr(CE,"note",(void *)tmpLine,mdfString);
    }

  ActiveRMFound = FALSE;
  DownRM = NULL;

  for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
    {
    if (MRM[rmindex].Name[0] == '\0')
      break;

    if (((!bmisclear(&MRM[rmindex].RTypes)) && !bmisset(&MRM[rmindex].RTypes,mrmrtCompute))
        || (bmisset(&MRM[rmindex].IFlags,mrmifLocalQueue)))
      {
      /* skip invalid destinations (non-compute resources and the
       * internal queue) */

      continue;
      }

    if (MRM[rmindex].State == mrmsActive)
      {
      ActiveRMFound = TRUE;

      break;
      }

    if (DownRM == NULL)
      DownRM = &MRM[rmindex];
    }  /* END for (rmindex) */

  if ((ActiveRMFound == FALSE) && (!bmisset(&MSched.Flags,mschedfDynamicRM)))
    {
    if (DownRM != NULL)
      {
      sprintf(tmpLine,"NOTE:  %s resource manager is down",
        (DownRM->Name != NULL) ? DownRM->Name : MRMType[DownRM->Type]);
      }
    else
      {
      sprintf(tmpLine,"NOTE:  no active compute resource manager found");
      }

    MXMLAddChild(PE,"comment",NULL,&CE);
    MXMLSetAttr(CE,"note",(void *)tmpLine,mdfString);
    }    /* END if (ActiveRMFound == FALSE) */
 

  /* Fetch License Expiration Date Information and 
   * if any data (something is in the buffer) send it back to caller 
   */
  MLicenseGetExpirationDateInfo(tmpLine,sizeof(tmpLine));

  if (!MUStrIsEmpty(tmpLine))
    {
    MXMLAddChild(PE,"comment",NULL,&CE);
    MXMLSetAttr(CE,"note",(void *)tmpLine,mdfString);
    }

  /* If any Message buffer, send it as well */
  if (MSched.MB != NULL) 
    {
    mmb_t *M;

    /* find public messages */

    if (MMBGetIndex(MSched.MB,"public",NULL,mmbtNONE,NULL,&M) == SUCCESS)
      {
      while (M != NULL)
        {
        if (M->Data != NULL)
          {
          sprintf(tmpLine,"NOTE:  %s",
            M->Data);

          MXMLAddChild(PE,"comment",NULL,&CE);
          MXMLSetAttr(CE,"note",(void *)tmpLine,mdfString);
          }

        MMBGetIndex((mmb_t *)M->Next,"public",NULL,mmbtNONE,NULL,&M);
        }  /* END while (M != NULL) */
      }
    }      /* END if (MSched.MB != NULL) */

  return(SUCCESS);
  } /* END __MUIGetShowqNotes */




/**
 * Report job queue status (for 'showq' command).
 *
 * @see MCQueueShow() for client processing of output
 * @see MJobBaseToXML() - child - generate XML for individual jobs
 * @see MUIDiagnose()->MClassShow() - peer - process 'mdiag -c'
 *
 * @param S (I)
 * @param ReqE (I)
 * @param RDE (O)
 * @param SP (I) [optional]
 * @param TypeList (I)
 * @param Auth (I) requestor
 * @param CFlagBM (I) [bitmap of enum mcal*]
 * @param DFlagBM (I) [bitmap of enum mcm*]
 */

int MUIQueueShow(

  msocket_t   *S,
  mxml_t      *ReqE,
  mxml_t     **RDE,
  mpar_t      *SP,
  char        *TypeList,
  char        *Auth,
  mbitmap_t   *CFlagBM,
  mbitmap_t   *DFlagBM)

  {
  mhashiter_t HTI;

  char    FlagString[MMAX_LINE];

  int     jindex;

  int     sindex;

  int     LocalAllocPC;
  int     RemoteAllocPC;

  int     RemoteActiveNodes;
  int     RemoteIdleNodes;
  int     RemoteConfigNodes;
  int     RemoteUpNodes;
  int     RemoteAProcs;
  int     RemoteUpProcs;

  int     LocalActiveNodes;
  int     LocalIdleNodes;
  int     LocalConfigNodes;
  int     LocalUpNodes;
  int     LocalAProcs;
  int     LocalUProcs;

  mbitmap_t    Flags;  /* bitmap of enum mcm* */

  mjob_t *J;

  mpar_t *P;

  int     JCount;

  mreq_t *RQ;

  mxml_t *DE;

  mxml_t *CE = NULL;
  mxml_t *QE = NULL;
  mxml_t *JE = NULL;
  mxml_t *EQE = NULL;

  char    tmpLine[MMAX_LINE];

  mbool_t DoSummary = FALSE;
  mbool_t ShowSystemJobs = bmisset(&MSched.DisplayFlags,mdspShowSystemJobs);

  mbitmap_t JTBM;

  mgcred_t *U = NULL;

  void     *O;

  mgcred_t *JU = NULL;
  mgcred_t *JG = NULL;
  mgcred_t *JA = NULL;
  mqos_t   *JQ = NULL;
  mclass_t *JC = NULL;

  mrsv_t   *R = NULL;

  mbool_t   AllowPartial = TRUE;
  mbool_t   IsAdmin      = FALSE;

  mpar_t   *VPCP = NULL;
  mpar_t   *ParPtr;

  int       CTok;

  char      tmpSVal[MMAX_NAME];
  char      tmpDVal[MMAX_NAME];
 
  enum MXMLOTypeEnum OIndex;

  const char *FName = "MUIQueueShow";

  MDB(2,fUI) MLog("%s(S,ReqE,RDE,SP,%s,%s)\n",
    FName,
    (TypeList != NULL) ? TypeList : "NULL",
    Auth);

  if ((S == NULL) || (ReqE == NULL) || (RDE == NULL))
    {
    return(FAILURE);
    }

  if (*RDE == NULL)
    {
    MUISAddData(S,"ERROR:    internal failure\n");

    return(FAILURE);
    }

  P = (SP != NULL) ? SP : &MPar[0];

  DE = *RDE;

  if (bmisset(CFlagBM,mcalAdmin1) ||
      bmisset(CFlagBM,mcalAdmin2) ||
      bmisset(CFlagBM,mcalAdmin3))
    IsAdmin = TRUE;

  /* process all constraints */

  CTok = -1;

  while (MS3GetWhere(
      ReqE,
      NULL,
      &CTok,
      tmpSVal,         /* O */
      sizeof(tmpSVal),
      tmpDVal,         /* O */
      sizeof(tmpDVal)) == SUCCESS)
    {
    OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(tmpSVal,MXO,FALSE,0);

    switch (OIndex)
      {
      case mxoUser:
      case mxoGroup:
      case mxoAcct:
      case mxoClass:
      case mxoQOS:

        if (MOGetObject(OIndex,tmpDVal,(void **)&O,mVerify) == FAILURE)
          {
          char tmpLine[MMAX_LINE];

          sprintf(tmpLine,"ERROR:    cannot locate %s %s in Moab's historical data\n",
            MXO[OIndex],
            tmpDVal);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (OIndex == mxoUser)
          JU = (mgcred_t *)O;
        else if (OIndex == mxoGroup)
          JG = (mgcred_t *)O;
        else if (OIndex == mxoAcct)
          JA = (mgcred_t *)O;
        else if (OIndex == mxoClass)
          JC = (mclass_t *)O;
        else if (OIndex == mxoQOS)
          JQ = (mqos_t *)O;

        break;

      case mxoRsv:

        {
        /* FORMAT:  <RSVID>[=] */

        char *ptr;

        if ((ptr = strchr(tmpDVal,'=')) != NULL)
          {
          *ptr = '\0';

          AllowPartial = FALSE;
          }

        if (MRsvFind(tmpDVal,&R,mraNONE) == FAILURE)
          {
          MUISAddData(S,"ERROR:    cannot locate reservation\n");

          return(FAILURE);
          }
        }    /* END BLOCK (case mxoRsv) */

        break;

      case mxoPar:

        /* handled earlier */

        /* NO-OP */

        break;
        
      default:

        /* NOTE: partition (not par) may be specified as a showq constraint */

        if (strcmp(tmpSVal,"partition") != 0)
          {
          sprintf(tmpLine,"ERROR:    invalid object type %s specified\n",
            tmpSVal);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        break;
      }  /* END switch (OIndex) */
    }    /* END while (MS3GetWhere() == SUCCESS) */

  bmcopy(&Flags,DFlagBM);

  bmset(&Flags,mcmFuture);

  if ((TypeList != NULL) && (strstr(TypeList,"verbose") != NULL))
    bmset(&Flags,mcmVerbose);
  
  /* display standard job queue information */

  bmfromstring(TypeList,MJobStateType,&JTBM);

  if ((MXMLGetAttr(ReqE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS) &&
      (strstr(FlagString,"summary") != NULL))
    {
    DoSummary = TRUE;
    }

  if (MXMLGetAttr(ReqE,"SHOWSYSTEMJOBS",NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    {
    ShowSystemJobs = MUBoolFromString(FlagString,FALSE);
    }

  if (bmisclear(&JTBM))
    {
    bmset(&JTBM,mjstActive);
    bmset(&JTBM,mjstEligible);
    bmset(&JTBM,mjstBlocked);
    }

  if (MS3GetObject(DE,NULL) == FAILURE)
    MS3SetObject(DE,(char *)MXO[mxoQueue],NULL);

  /* add cluster stats */

  CE = NULL;

  if (MXMLCreateE(&CE,(char *)MXO[mxoCluster]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    internal failure\n");

    return(FAILURE);
    }

  MXMLAddE(DE,CE);

  LocalAllocPC = 0;
  RemoteAllocPC = 0;

  MUHTIterInit(&HTI);

  while (MUHTIterate(&MAQ,NULL,(void **)&J,NULL,&HTI) == SUCCESS)
    {
    RQ = J->Req[0];  /* handle only first req of job */

    if ((P->Index > 0) && (RQ->PtIndex != P->Index))
      continue;

    if (MJOBISCOMPLETE(J) == TRUE)
      continue;

#ifdef __MNOTFULLYIMPLEMENTED

      /* virtual private cluster check */

      if ((U != NULL) && (J->Credential.U != U))
        {
        /* NYI - check some future policy to determine if account members can see other account members jobs */

        if (MCredCheckAcctAccess(U,NULL,J->Credential.A) == FAILURE)
          {
          continue;
          }
        }
#endif /* __MNOTFULLYIMPLEMENTED */

    if (((JU != NULL) && (J->Credential.U != JU)) ||
        ((JG != NULL) && (J->Credential.G != JG)) ||
        ((JA != NULL) && (J->Credential.A != JA)) ||
        ((JQ != NULL) && (J->Credential.Q != JQ)) ||
        ((JC != NULL) && (J->Credential.C != JC)))
      {
      /* job rejected by showq constraints */

      continue;
      }

    if (bmisset(&J->IFlags,mjifIsHidden))
      {
      if ((IsAdmin == FALSE) || !bmisset(&Flags,mcmVerbose))
        continue;
      }

    if ((R != NULL) && 
        (MRsvHasOverlap(J->Rsv,R,AllowPartial,&J->NodeList,NULL,NULL) == FALSE))
      {
      continue;
      }  /* END if ((R != NULL) && ...) */

    if ((J->DestinationRM == NULL) ||
        (J->DestinationRM->Type != mrmtMoab))
      {
      LocalAllocPC += J->TotalProcCount;
      }
    else
      {
      RemoteAllocPC += J->TotalProcCount;
      }
    }  /* END for (aindex) */

  /* provide cluster stats */

  MXMLSetAttr(CE,"time",(void *)&MSched.Time,mdfLong);
  MXMLSetAttr(CE,"LocalAllocProcs",(void *)&LocalAllocPC,mdfInt);
  MXMLSetAttr(CE,"RemoteAllocProcs",(void *)&RemoteAllocPC,mdfInt);

  MUIGetShowqNotes(IsAdmin,CE);

  RemoteConfigNodes = 0;
  RemoteActiveNodes = 0;
  RemoteIdleNodes   = 0;
  RemoteUpNodes     = 0;
  RemoteAProcs      = 0;
  RemoteUpProcs     = 0;

  LocalConfigNodes  = 0;
  LocalActiveNodes  = 0;
  LocalIdleNodes    = 0;
  LocalUpNodes      = 0;
  LocalAProcs       = 0;
  LocalUProcs       = 0;

  if ((VPCP == NULL) && (P == &MPar[0]))
    {
    int pindex;

    /* loop through all partitions - separate remote from local */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      ParPtr = &MPar[pindex];

      if (ParPtr == NULL)
        break;
       
      if ((ParPtr->RM != NULL) && (ParPtr->RM->Type != mrmtMoab))
        {
        LocalUProcs      += ParPtr->UpRes.Procs;
        LocalAProcs      += ParPtr->ARes.Procs;
        LocalUpNodes     += ParPtr->UpNodes;
        LocalIdleNodes   += ParPtr->IdleNodes;
        LocalActiveNodes += ParPtr->ActiveNodes;
        LocalConfigNodes += ParPtr->ConfigNodes;
        }
      else
        {
        RemoteUpProcs     += ParPtr->UpRes.Procs;
        RemoteAProcs      += ParPtr->ARes.Procs;
        RemoteUpNodes     += ParPtr->UpNodes;
        RemoteIdleNodes   += ParPtr->IdleNodes;
        RemoteActiveNodes += ParPtr->ActiveNodes;
        RemoteConfigNodes += ParPtr->ConfigNodes;
        }
      }
    }  /* END if ((VPCP == NULL) && (P == &MPar[0])) */
  else if (VPCP != NULL)
    {
    ParPtr = VPCP;

    LocalUProcs      += ParPtr->UpRes.Procs;
    LocalAProcs      += ParPtr->ARes.Procs;
    LocalUpNodes     += ParPtr->UpNodes;
    LocalIdleNodes   += ParPtr->IdleNodes;
    LocalActiveNodes += ParPtr->ActiveNodes;
    LocalConfigNodes += ParPtr->ConfigNodes;
    }
  else
    {
    ParPtr = P;

    LocalUProcs      += ParPtr->UpRes.Procs;
    LocalAProcs      += ParPtr->ARes.Procs;
    LocalUpNodes     += ParPtr->UpNodes;
    LocalIdleNodes   += ParPtr->IdleNodes;
    LocalActiveNodes += ParPtr->ActiveNodes;
    LocalConfigNodes += ParPtr->ConfigNodes;
    }

  MXMLSetAttr(CE,"LocalUpProcs",(void *)&LocalUProcs,mdfInt);
  MXMLSetAttr(CE,"LocalIdleProcs",(void *)&LocalAProcs,mdfInt);

  if (MSched.BluegeneRM == TRUE)
    {
    int tmpInt;

    tmpInt = LocalUProcs / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"LocalUpNodes",(void *)&tmpInt,mdfInt);

    tmpInt = LocalAllocPC / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"LocalActiveNodes",(void *)&tmpInt,mdfInt);

    tmpInt = RemoteUpProcs / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"RemoteUpNodes",(void *)&tmpInt,mdfInt);

    tmpInt = RemoteAllocPC / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"RemoteActiveNodes",(void *)&tmpInt,mdfInt);
    }
  else
    {
    MXMLSetAttr(CE,"LocalUpNodes",(void *)&LocalUpNodes,mdfInt);
    MXMLSetAttr(CE,"LocalActiveNodes",(void *)&LocalActiveNodes,mdfInt);
    MXMLSetAttr(CE,"RemoteUpNodes",(void *)&RemoteUpNodes,mdfInt);
    MXMLSetAttr(CE,"RemoteActiveNodes",(void *)&RemoteActiveNodes,mdfInt);
    }

  MXMLSetAttr(CE,"LocalIdleNodes",(void *)&LocalIdleNodes,mdfInt);
  MXMLSetAttr(CE,"LocalConfigNodes",(void *)&LocalConfigNodes,mdfInt);

  MXMLSetAttr(CE,"RemoteUpProcs",(void *)&RemoteUpProcs,mdfInt);
  MXMLSetAttr(CE,"RemoteIdleProcs",(void *)&RemoteAProcs,mdfInt);
  MXMLSetAttr(CE,"RemoteIdleNodes",(void *)&RemoteIdleNodes,mdfInt);
  MXMLSetAttr(CE,"RemoteConfigNodes",(void *)&RemoteConfigNodes,mdfInt);

  if (bmisset(&JTBM,mjstActive))
    {
    mjob_t **SuspQ = NULL;

    job_iter JTI;
    mhashiter_t HTI;

    QE = NULL;

    if (MXMLCreateE(&QE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");
  
      return(FAILURE);
      }
  
    MXMLAddE(DE,QE);

    JCount = 0;

    MXMLSetAttr(
      QE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstActive],
      mdfString);

    /* locate suspended jobs */

    sindex = 0;

    MJobIterInit(&JTI);

    MJobListAlloc(&SuspQ);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if ((J->State == mjsSuspended) && (MUHTGet(&MAQ,J->Name,NULL,NULL) == FAILURE))
        SuspQ[sindex++] = J;
      }  /* END for (J) */

    SuspQ[sindex] = NULL;

    /* list active jobs */

    sindex = 0;

    MUHTIterInit(&HTI);

    while ((MUHTIterate(&MAQ,NULL,(void **)&J,NULL,&HTI) == SUCCESS) || (SuspQ[sindex] != NULL))
      {
      if ((J == NULL) && (SuspQ[sindex] == NULL))
        break;

      if (J == NULL)
        {
        J = SuspQ[sindex++];
        }

      RQ = J->Req[0];  /* handle only first req of job */

      if ((P->Index > 0) && (RQ->PtIndex != P->Index))
        continue;

      if ((J->System != NULL) && (ShowSystemJobs == FALSE))
        continue;

      if (MJOBISCOMPLETE(J) == TRUE)
        continue;

#ifdef __MNOTFULLYIMPLEMENTED

      /* virtual private cluster check */

      if ((U != NULL) && (J->Credential.U != U))
        {
        /* NYI - check some future policy to determine if account members can see other account members jobs */

        if (MCredCheckAcctAccess(U,NULL,J->Credential.A) == FAILURE)
          {
          continue;
          }
        }
#endif /* __MNOTFULLYIMPLEMENTED */

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          ((JC != NULL) && (J->Credential.C != JC)))
        {
        /* job rejected by showq constraints */

        continue;
        }

      if (bmisset(&J->IFlags,mjifIsHidden))
        {
        if ((IsAdmin == FALSE) || !bmisset(&Flags,mcmVerbose))
          continue;
        }

      if ((R != NULL) &&
          (MRsvHasOverlap(J->Rsv,R,AllowPartial,&J->NodeList,NULL,NULL) == FALSE))
        {
        continue;
        }  /* END if ((R != NULL) && ...) */

      if (DoSummary == FALSE)
        {
        if (MJobBaseToXML(J,&JE,mjstActive,FALSE,bmisset(DFlagBM,mcmVerbose)) == FAILURE)
          continue;
        
        MDB(3,fUI) MLog("INFO:     %s:  adding active job '%s' to buffer\n",
          FName,
          J->Name);

        MXMLAddE(QE,JE);
        }

      JCount++;
      }  /* END while (while ((MAQ[aindex] != 0) || (SuspQ[sindex] != -1)) */

    MXMLSetAttr(QE,"count",(void **)&JCount,mdfInt);

    MUFree((char **)&SuspQ);
    }    /* END if (bmisset(&JTBM(mjstActive) */

  if (bmisset(&JTBM,mjstEligible))
    {
    /* list eligible jobs */

    JCount = 0;

    QE = NULL;

    if (MXMLCreateE(&QE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");

      return(FAILURE);
      }

    EQE = QE;

    MXMLAddE(DE,QE);

    MXMLSetAttr(
      QE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstEligible],
      mdfString);

    for (jindex = 0;MUIQ[jindex] != NULL;jindex++)
      {
      J = MUIQ[jindex];

      if (!MJobPtrIsValid(J))
        continue;

      if (MJOBISACTIVE(J) == TRUE)
        {
        /* NOTE:  dynamic jobs may be active and eligible */

        continue;
        }

      if ((J->System != NULL) && (ShowSystemJobs == FALSE))
        continue;

      RQ = J->Req[0];  /* handle only first req of job */

      if ((RQ == NULL) || (J->Credential.U == NULL))
        {
        /* corrupt job record */

        continue;
        }

      if ((P->Index > 0) && (bmisset(&J->PAL,P->Index) == FAILURE))
        continue;

      if ((J->State == mjsRemoved) || (J->State == mjsSuspended))
        continue;

#ifdef __MNOTFULLYIMPLEMENTED

      /* virtual private cluster check */

      if ((U != NULL) && (J->Credential.U != U))
        {
        /* NYI - check some future policy to determine if account members can see other account members jobs */

        if (MCredCheckAcctAccess(U,NULL,J->Credential.A) == FAILURE)
          {
          continue;
          }
        }
#endif /* __MNOTFULLYIMPLEMENTED */

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          ((JC != NULL) && (J->Credential.C != JC)))
        {
        /* job rejected by showq constraints */

        continue;
        }

      if (bmisset(&J->IFlags,mjifIsHidden))
        {
        if ((IsAdmin == FALSE) || !bmisset(&Flags,mcmVerbose))
          continue;
        }

      if ((R != NULL) &&
          (MRsvHasOverlap(J->Rsv,R,AllowPartial,&J->NodeList,NULL,NULL) == FALSE))
        {
        continue;
        }  /* END if ((R != NULL) && ...) */

      if (DoSummary == FALSE)
        {
        if (MJobBaseToXML(J,&JE,mjstEligible,FALSE,bmisset(DFlagBM,mcmVerbose)) == FAILURE)
          continue;

        MDB(3,fUI) MLog("INFO:     %s:  adding eligible job '%s' to buffer\n",
          FName,
          J->Name);

        MXMLAddE(QE,JE);
        }

      if (bmisset(&Flags,mcmVerbose))
        {
        if (J->DRMJID != NULL)
          {
          MXMLSetAttr(
            JE,
            (char *)MJobAttr[mjaDRMJID],
            (void **)J->DRMJID,
            mdfString);
          }
        }

      JCount++;
      }  /* END for (jindex = 0;UIQ[jindex] != -1;jindex++) */

    MXMLSetAttr(QE,"count",(void **)&JCount,mdfInt);
    }    /* END if (bmisset(&JTBM,mjstEligible)) */

  if (bmisset(&JTBM,mjstBlocked))
    {
    job_iter JTI;

    /* list blocked jobs */

    QE = NULL;

    if (MXMLCreateE(&QE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");

      return(FAILURE);
      }

    JCount = 0;

    MXMLAddE(DE,QE);

    MXMLSetAttr(
      QE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstBlocked],
      mdfString);

    MJobIterInit(&JTI);

    /* list blocked jobs */

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if ((P->Index > 0) && (bmisset(&J->PAL,P->Index) == FAILURE))
        continue;

      /* continue if alloc job found */

      if ((MJOBISALLOC(J) == TRUE) ||
          (J->IState != mjsNONE))
        {
        continue;
        }

      /* Prevent some jobs from being displayed in both the active and
       * blocked queues. 
       * This can happen in the case where a job is requeueing */

      if (J->EState == mjsStarting)
        {
        continue;
        }

      if (MJOBISCOMPLETE(J) == TRUE)
        continue;

      if ((J->System != NULL) && (ShowSystemJobs == FALSE))
        continue;

      if (!MJobIsBlocked(J))
        continue;

      if ((J->State == mjsStaged) && (EQE == NULL))
        continue;

      /* virtual private cluster check */

      /* Show all jobs regardless if explicity asked for through showq -b */

      if ((IsAdmin == FALSE) && 
          bmisset(&MSched.DisplayFlags,mdspfHideBlocked) &&
          (TypeList != NULL) &&
          (strcmp(TypeList,"blocked") != 0))
        {
        if (U == NULL)
          MUserFind(Auth,&U);

        /* hide non-owned blocked jobs from non-admins */

        if (J->Credential.U != U)
          {
          /* Report the number of blocked jobs though */

          JCount++;

          continue;
          }  
        }

#ifdef __MNOTFULLYIMPLEMENTED
      if ((U != NULL) && (J->Credential.U != U))
        {
        /* NYI - check some future policy to determine if account members can
         * see other account members jobs */

        if (MCredCheckAcctAccess(U,NULL,J->Credential.A) == FAILURE)
          {
          continue;
          }
        }
#endif /* __MNOTFULLYIMPLEMENTED */

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          (J->Array != NULL) ||
          ((JC != NULL) && (J->Credential.C != JC)))
        {
        /* job rejected by showq constraints */

        continue;
        }

      if (bmisset(&J->IFlags,mjifIsHidden))
        {
        if ((IsAdmin == FALSE) || !bmisset(&Flags,mcmVerbose))
          continue;
        }

      if (DoSummary == FALSE)
        {
        if (MJobBaseToXML(J,&JE,mjstBlocked,FALSE,bmisset(DFlagBM,mcmVerbose)) == FAILURE)
          continue;

        if (J->State == mjsStaged)
          {
          if (EQE != NULL)
            MXMLAddE(EQE,JE);
          }
        else
          {
          MXMLAddE(QE,JE);
          }
        }

      if (bmisset(&Flags,mcmVerbose))
        {
        if (J->DRMJID != NULL)
          {
          MXMLSetAttr(
            JE,
            (char *)MJobAttr[mjaDRMJID],
            (void **)J->DRMJID,
            mdfString);
          }
        }

      if ((MJobGetBlockReason(J,SP) == mjneHold) && (J->HoldReason != mhrNONE))
        {
        MXMLSetAttr(
          JE,
          (char *)"HoldReason",
          (void **)MHoldReason[J->HoldReason],
          mdfString);
        }

      MDB(3,fUI) MLog("INFO:     %s:  adding blocked job '%s' to buffer\n",
        FName,
        J->Name);

      JCount ++;

      }  /* END for (J = MJob[0]->Next;(J != NULL) && (J != MJob[0]);J = J->Next) */

    MXMLSetAttr(QE,"count",(void **)&JCount,mdfInt);
    }    /* END if (bmisset(&JTBM,mjstBlocked)) */

  if ((bmisset(&JTBM,mjstCompleted)) && (MCJobHT.size() > 0))
    {
    job_iter JTI;

    int tjindex;
    mjob_t **tmpMCJob = NULL;

    /* list completed jobs */

    QE = NULL;

    JCount = 0;

    if (MXMLCreateE(&QE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");

      return(FAILURE);
      }

    MXMLAddE(DE,QE);

    MXMLSetAttr(
      QE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstCompleted],
      mdfString);

    tjindex = 0;

    /* filter, sort, then add completed jobs */

    tmpMCJob = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * MCJobHT.size()); 

    if (tmpMCJob == NULL)
      {
      MDB(1,fUI) MLog("INFO:     cannot allocate temp memory for (%d) completed jobs\n",
        MCJobHT.size());

      MUISAddData(S,"ERROR:    cannot allocate temp memory for completed jobs\n");

      return(FAILURE);
      } /* END (tmpMCJob == NULL) */

    MCJobIterInit(&JTI);

    while (MCJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if ((J->System != NULL) && (ShowSystemJobs == FALSE))
        continue;

      RQ = J->Req[0]; /* FIXME */

      if ((P->Index != 0) && (RQ->PtIndex != P->Index))
        {
        continue;
        }

      /* virtual private cluster check */

      if ((U != NULL) && (J->Credential.U != U))
        {
        /* not the user's job */

        /* NYI - check some future policy to determine if account members can see other account members jobs */

        if (MCredCheckAcctAccess(U,NULL,J->Credential.A) == FAILURE)
          {
          continue;
          }
        }

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          ((JC != NULL) && (J->Credential.C != JC)))
        {
        /* job rejected by showq constraints */

        continue;
        }

      if (bmisset(&J->IFlags,mjifIsHidden))
        {
        if ((IsAdmin == FALSE) || !bmisset(&Flags,mcmVerbose))
          continue;
        }
 
      tmpMCJob[tjindex++] = J;
      }  /* END for (jindex) */

    qsort(
      (void *)tmpMCJob,
      tjindex,
      sizeof(mjob_t *),
      (int(*)(const void *,const void *))MJobCompletionTimeCompLToH);

    for (jindex = 0;jindex < tjindex;jindex++)
      {
      J = tmpMCJob[jindex];

      if (DoSummary == FALSE)
        {
        if (MJobBaseToXML(J,&JE,mjstCompleted,FALSE,bmisset(DFlagBM,mcmVerbose)) == FAILURE)
          continue;

        MDB(3,fUI) MLog("INFO:     %s:  adding completed job '%s' to buffer\n",
          FName,
          J->Name);

        MXMLAddE(QE,JE);
        }

      if (bmisset(&Flags,mcmVerbose))
        {
        if (J->DRMJID != NULL)
          {
          MXMLSetAttr(JE,(char *)MJobAttr[mjaDRMJID],(void **)J->DRMJID,mdfString);
          }
        }

      JCount++;
      }  /* END for (jindex) */

    MXMLSetAttr(QE,"purgetime",(void **)&MSched.JobCPurgeTime,mdfLong);

    MXMLSetAttr(QE,"count",(void **)&JCount,mdfInt);

    MUFree((char **)&tmpMCJob);
    }    /* END if (bmisset(&JTBM,mjstCompleted)) */

  return(SUCCESS);
  }  /* END MUIQueueShow() */
/* END MUIQueueShow.c */
