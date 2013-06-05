/* HEADER */

/**
 * @file MUIJobDiagnose.c
 *
 * Contains MUI Job Diagnose
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  






/**
 * Process 'mdiag -j' client requests.
 *
 * @see MUIDiagnose() - parent
 * @see __MUIJobToXML() - child
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param RE (O)
 * @param String (O)
 * @param P (I) [optional]
 * @param DiagOpt (I)
 * @param ReqE (I) [optional]
 * @param FlagBM (I) [bitmap of enum mcm*]
 */

int MUIJobDiagnose(

  char      *Auth,
  mxml_t    *RE,
  mstring_t *String,
  mpar_t    *P,
  char      *DiagOpt,
  mxml_t    *ReqE,
  mbitmap_t *FlagBM)

  {
  job_iter JTI;

  int jindex;
  int nindex;
  int rqindex;
  int index;

  mnode_t *N;
  mjob_t  *J;

  int    PC;
  int    NC;

  int    TotalJC;
  int    ActiveJC;
  int    MAQJC;

  mbool_t IsFirstFlag = TRUE;

  char   QueuedTime[MMAX_NAME];
  char   WCLimit[MMAX_NAME];

  char   MemLine[MMAX_NAME];
  char   DiskLine[MMAX_NAME];
  char   ProcLine[MMAX_NAME];

  char   ClassLine[MMAX_NAME];
  char   TString[MMAX_LINE];

  mreq_t *RQ;

  mbool_t ShowTemplate;

  int    MinJob;
  int    MaxJob;

  void *O;

  mgcred_t *JU = NULL;
  mgcred_t *JG = NULL;
  mgcred_t *JA = NULL;
  mclass_t *JC = NULL;
  mqos_t   *JQ = NULL;

  int       CTok;
  enum MXMLOTypeEnum OIndex;

  char tmpSVal[MMAX_LINE];
  char tmpDVal[MMAX_LINE];

  mbool_t ShowCompleted = FALSE;
  mbool_t ShowSystem = FALSE;
  mbool_t ShowNormalJobs = TRUE;

  const char *FName = "MUIJobDiagnose";

  MDB(2,fUI) MLog("%s(Buffer,BufSize,%s,%s,ReqE,%d)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (DiagOpt != NULL) ? DiagOpt : "NULL",
    FlagBM);

  if (bmisset(FlagBM,mcmPolicy))
    {
    ShowTemplate = TRUE;
    }
  else
    {
    /* show master job list */

    ShowTemplate = FALSE;
    }

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
          MDB(5,fUI) MLog("INFO:     cannot locate %s %s for constraint in %s\n",
            MXO[OIndex],
            tmpDVal,
            FName);

          if (!bmisset(FlagBM,mcmXML))
            {
            MStringAppendF(String,"ERROR:    cannot locate %s %s in Moab's historical data\n",
              MXO[OIndex],
              tmpDVal);
            }

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

      case mxoALL:
      case mxoNONE:

      /* NOTE:  Currently, these only work with the --xml format options.
       *        Example:  mdiag -j --xml -w [completed=true|system=true|ALL=true]  (MOAB-3315) */

      if (!strcasecmp(tmpDVal,"true"))
        {
        if (!strcasecmp(tmpSVal,"completed"))
          {
          ShowCompleted = TRUE;
          ShowNormalJobs = FALSE;
          }
        else if (!strcasecmp(tmpSVal,"system"))
          {
          ShowSystem = TRUE;
          ShowNormalJobs = FALSE;
          }
        else if (!strcasecmp(tmpSVal,"ALL"))
          {
          ShowSystem = TRUE;
          ShowCompleted = TRUE;
          }
        }

      break;

      default:

        /* silently ignore */

        /* NO-OP */

        break;
      }  /* END switch (OIndex) */
    }    /* END while (MS3GetWhere() == SUCCESS) */

  if (bmisset(FlagBM,mcmXML))
    {
    job_iter JTI;

    mxml_t *DE;
    mxml_t *JE;

    DE = RE;

    MJobIterInit(&JTI);

    /* report XML format */

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if (J == NULL)
        break;

      if ((!MUStrIsEmpty(DiagOpt)) && 
          (strcmp(DiagOpt,J->Name)) && 
          ((J->AName == NULL) || strcmp(DiagOpt,J->AName)))
        {
        /* job does not match specified job name/id */

        continue;
        }

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          ((JC != NULL) && (J->Credential.C != JC)) ||
          ((P->Index != 0) && (bmisset(&J->PAL,P->Index) == FALSE)))
        {
        /* job rejected by request constraints */

        continue;
        }

      if (bmisset(&J->IFlags,mjifIsHidden))
        {
        if (!bmisset(FlagBM,mcmVerbose))
          continue;
        }

      if ((J->System != NULL) && (ShowSystem == TRUE))
       {
       /* Its a system job & they want system info */

       if (__MUIJobToXML(J,FlagBM,&JE) == SUCCESS)
         MXMLAddE(DE,JE);
        continue;
        }
      else
        {
        /* It's a normal job & they want normal info */

        if (ShowNormalJobs == TRUE)
          {
          if (__MUIJobToXML(J,FlagBM,&JE) == SUCCESS)
            MXMLAddE(DE,JE);
          continue;
          }
        }
      }  /* END for (jindex) */

    if ((ShowCompleted == TRUE) && (MCJobHT.size() > 0))
      {
      job_iter JTI;
      int tjindex;
      mjob_t **tmpMCJob = NULL;

      tmpMCJob = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * MCJobHT.size()); 

      if (tmpMCJob == NULL)
        {
        MDB(1,fUI) MLog("INFO:     cannot allocate temp memory for (%d) completed jobs\n",
          MCJobHT.size());

        return(FAILURE);
        } /* END if (tmpMCJob == NULL) */

      /* NOTE:  sort completed jobs into earliest-completion-first order */

      tjindex = 0;

      MCJobIterInit(&JTI);

      while (MCJobTableIterate(&JTI,&J) == SUCCESS)
        {
        if ((DiagOpt != NULL) &&
             strcmp(DiagOpt,NONE) &&
             (!MUStrIsEmpty(DiagOpt)) &&
             strcmp(DiagOpt,J->Name) &&
            ((J->AName == NULL) ||
             strcmp(DiagOpt,J->AName)))
          {
          /* job does not match specified job name/id */

          continue;
          }

        if (((JU != NULL) && (J->Credential.U != JU)) ||
            ((JG != NULL) && (J->Credential.G != JG)) ||
            ((JA != NULL) && (J->Credential.A != JA)) ||
            ((JQ != NULL) && (J->Credential.Q != JQ)) ||
            ((JC != NULL) && (J->Credential.C != JC)))
          {
          /* job rejected by request constraints */

          continue;
          }

        if (bmisset(&J->IFlags,mjifIsHidden))
          {
          if (!bmisset(FlagBM,mcmVerbose))
            continue;
          }

        tmpMCJob[tjindex++] = J;
        }    /* END for (jindex) */

      qsort(
        (void *)tmpMCJob,
        tjindex,
        sizeof(mjob_t *),
        (int(*)(const void *,const void *))MJobCompletionTimeCompLToH);

      for (jindex = 0;jindex < tjindex;jindex++)
        {
        J = tmpMCJob[jindex];

        if (__MUIJobToXML(J,FlagBM,&JE) == FAILURE)
          continue;

        MXMLAddE(DE,JE);
        }  /* END for (jindex) */

      MUFree((char **)&tmpMCJob);
      }    /* END if (ShowCompleted == TRUE) */

    /*
    if (MXMLToString(DE,Buffer,*BufSize,NULL,TRUE) == FAILURE)
      {
      MDB(2,fUI) MLog("ALERT:    buffer truncated in %s (%ld)",
        FName,
        *BufSize);
      }

    MXMLDestroyE(&DE);
    */

    return(SUCCESS);
    }  /* END if (bmisset(FlagBM,mcmXML)) */

  /* create header */

  if (bmisset(FlagBM,mcmVerbose))
    {
    /* FORMAT:                 NAME STATE PARTI PROCS QOS WCLI RESER MPR USR GRP ACC QTIM OPS ARC MEM DSK CLAS FEAT */

    MStringAppendF(String,"%-18s %8.8s %8.8s %4.4s %8s %11s %1.1s %4s %8s %8s %8s %11s %6s %6s %6s %6s %6s %11s %s\n\n",
      "JobID",
      "State",
      "Par",
      "Proc",
      "QOS",
      "WCLimit",
      "Reservation",
      "Min",
      "User",
      "Group",
      "Account",
      "QueuedTime",
      "Opsys",
      "Arch",
      "Mem",
      "Disk",
      "Procs",
      "Class",
      "Features");
    }
  else
    {
    /* FORMAT:                 NAME STATE PROCS WCLI USR OPS CLS FEAT */

    MStringAppendF(String,"%-18s %8.8s %4.4s %11s %8s %6s %6s %s\n\n",
      "JobID",
      "State",
      "Proc",
      "WCLimit",
      "User",
      "Opsys",
      "Class",
      "Features");
    }

  TotalJC = 0;
  ActiveJC = 0;

  if (ShowTemplate == TRUE)
    {
    MinJob = 0;
    MaxJob = MMAX_TJOB;
    }
  else
    {
    MinJob = 0;
    MaxJob = MSched.M[mxoJob];
    }

  MDB(4,fUI) MLog("INFO:     diagnosing job table (%d job slots)\n",
    MaxJob);

  MJobIterInit(&JTI);

  for (jindex = MinJob;jindex < MaxJob;jindex++)
    {
    if (ShowTemplate == TRUE)
      {
      if (MUArrayListGet(&MTJob,jindex) == NULL)
        {
        J = NULL;
        }
      else
        {
        J = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));
        }

      if (J == NULL)
        break;
      }
    else
      {
      if (MJobTableIterate(&JTI,&J) == FAILURE)
        break;
      }

    if ((!MUStrIsEmpty(DiagOpt)) &&
         strcmp(DiagOpt,J->Name) &&
        ((J->AName == NULL) || strcmp(DiagOpt,J->AName)))
      {
      /* job does not match specified job name/id */

      continue;
      }

    if (((JU != NULL) && (J->Credential.U != JU)) ||
        ((JG != NULL) && (J->Credential.G != JG)) ||
        ((JA != NULL) && (J->Credential.A != JA)) ||
        ((JQ != NULL) && (J->Credential.Q != JQ)) ||
        ((JC != NULL) && (J->Credential.C != JC)))
      {
      /* job rejected by request constraints */

      continue;
      }

    /* check partition constraints */
    if ((P != NULL) && (P->Index != 0))
      {
      enum MJobStateEnum tmpState = mjsNONE;

      if (bmisset(&J->Hold,mhBatch))
        tmpState = mjsBatchHold;
      else if (bmisset(&J->Hold,mhSystem))
        tmpState = mjsSystemHold;
      else if (bmisset(&J->Hold,mhUser))
        tmpState = mjsUserHold;
      else if (bmisset(&J->Hold,mhDefer))
        tmpState = mjsDeferred;
      else
        tmpState = (J->IState != mjsNONE) ? J->IState : J->State;

      if (tmpState == mjsRunning)
        {
        /* since the job is running, we expect a single partition and use J->Req[0]->PtIndex */

        if (J->Req[0]->PtIndex != P->Index)
          continue;
        }
      else
        {
        /* job not running, see if requested partition is in J->PAL */

        if (!bmisset(&J->PAL,P->Index))
          {
          /* partition not in J->PAL, do not show */

          continue;
          }
        }
      } /* END if (P != NULL) */

    RQ = J->Req[0]; /* FIXME */

    if ((bmisset(&MPar[0].Flags,mpfUseCPUTime)) && (J->CPULimit > 0))
      {
      MULToTString(J->CPULimit,TString);
      strcpy(WCLimit,TString);
      }
    else
      {
      MULToTString(J->SpecWCLimit[0],TString);
      strcpy(WCLimit,TString);
      }

    MULToTString(J->EffQueueDuration,TString);
    strcpy(QueuedTime,TString);

    if (RQ->ReqNR[mrProc] != 0)
      {
      sprintf(ProcLine,"%s%d",
        MComp[(int)RQ->ReqNRC[mrProc]],
        RQ->ReqNR[mrProc]);
      }
    else
      {
      strcpy(ProcLine,"-");
      }

    if (RQ->ReqNR[mrMem] != 0)
      {
      sprintf(MemLine,"%s%d",
        MComp[(int)RQ->ReqNRC[mrMem]],
        RQ->ReqNR[mrMem]);
      }
    else
      {
      strcpy(MemLine,"-");
      }

    if (RQ->ReqNR[mrDisk] != 0)
      {
      sprintf(DiskLine,"%s%d",
        MComp[(int)RQ->ReqNRC[mrDisk]],
        RQ->ReqNR[mrDisk]);
      }
    else
      {
      strcpy(DiskLine,"-");
      }

    if (J->Credential.C != NULL)
      strcpy(ClassLine,J->Credential.C->Name);
    else
      strcpy(ClassLine,"-");

    if (bmisset(FlagBM,mcmVerbose))
      {
      /* NOTE:  expand fields and do not truncate any data */

      MStringAppendF(String,"%-18s %8.8s %8s %4d %8s %11s %1d %4d %8s %8s %8.8s %11s %6s %6s %6s %6s %6s %11s ",
        J->Name,
        (J->State > mjsNONE) ? MJobState[J->State] : "-",
        MPar[RQ->PtIndex].Name,
        J->TotalProcCount,
        (J->Credential.Q != NULL) ? J->Credential.Q->Name : "-",
        WCLimit,
        J->Rsv ? 1 : 0,
        J->TotalProcCount,
        (J->Credential.U != NULL) ? J->Credential.U->Name : "-",
        (J->Credential.G != NULL) ? J->Credential.G->Name : "-",
        (J->Credential.A != NULL) ? J->Credential.A->Name : "-",
        QueuedTime,
        (RQ->Opsys != 0) ? MAList[meOpsys][RQ->Opsys] : "-",
        (RQ->Arch != 0) ? MAList[meArch][RQ->Arch] : "-",
        MemLine,
        DiskLine,
        ProcLine,
        ClassLine);

      MStringAppendF(String,"%-10s",
        (!bmisclear(&RQ->ReqFBM)) ? MUNodeFeaturesToString(',',&RQ->ReqFBM,tmpSVal) : "-");

      if (!MSNLIsEmpty(&RQ->DRes.GenericRes))
        {
        mstring_t tmp(MMAX_LINE);

        MSNLToMString(&RQ->DRes.GenericRes,NULL,",",&tmp,meGRes);

        MStringAppendF(String," GRES=%s",
          tmp.c_str());
        }

      if ((J->Req[1] != NULL) && (!MSNLIsEmpty(&J->Req[1]->DRes.GenericRes)))
        {
        mstring_t tmp(MMAX_LINE);

        MSNLToMString(&J->Req[1]->DRes.GenericRes,NULL,",",&tmp,meGRes);

        MStringAppendF(String," GRES=%s",
          tmp.c_str());
        }

    for (index = 0;MJobFlags[index] != NULL;index++)
      {
      if (bmisset(&J->Flags,index))
        { 
        if (IsFirstFlag == FALSE)
          {
          MStringAppendF(String,",%s",
            MJobFlags[index]);
          }
        else
          {
          MStringAppendF(String," FLAGS=%s",
            MJobFlags[index]);

          IsFirstFlag = FALSE;
          }

        if ((index == mjfAdvRsv) && (J->ReqRID != NULL))
          {
          MStringAppend(String,":");
          MStringAppend(String,J->ReqRID);
          }
        }
      }    /* END for (index) */

      IsFirstFlag = TRUE;

      MStringAppend(String,"\n");
      }
    else
      {
      /* FORMAT:                 NAME STATE PRC WCLI USER  OPSYS CLASS FEAT */

      MStringAppendF(String,"%-18s %8.8s %4d %11s %8.8s %6.6s %6.6s ",
        J->Name,
        (J->State > mjsNONE) ? MJobState[J->State] : "-",
        J->TotalProcCount,
        WCLimit,
        (J->Credential.U != NULL) ? J->Credential.U->Name : "-",
        (RQ->Opsys != 0) ? MAList[meOpsys][RQ->Opsys] : "-",
        (J->Credential.C != NULL) ? J->Credential.C->Name : "-");

      MStringAppendF(String,"%s\n",
        (!bmisclear(&RQ->ReqFBM)) ?  MUNodeFeaturesToString(',',&RQ->ReqFBM,tmpSVal) : "-");
      }

    TotalJC++;

    if (ShowTemplate == TRUE)
      {
      if (J->TemplateExtensions != NULL)
        {
        if (!bmisclear(&J->Flags))
          {
          mbool_t IsFirstFlag = TRUE;

          MStringAppendF(String,"Flags:          ");

          for (index = 0;MJobFlags[index] != NULL;index++)
            {
            if (bmisset(&J->Flags,index))
              {
              if (IsFirstFlag == FALSE)
                {
                MStringAppendF(String,",%s",
                  MJobFlags[index]);
                }
              else
                {
                MStringAppendF(String,"%s",
                  MJobFlags[index]);

                IsFirstFlag = FALSE;
                }

              if ((index == mjfAdvRsv) && (J->ReqRID != NULL))
                {
                MStringAppend(String,":");
                MStringAppend(String,J->ReqRID);
                }
              }
            }    /* END for (index) */

          MStringAppend(String,"\n");
          }  /* END if (!bmisclear(&J->Flags)) */
        }    /* END if (J->TX != NULL) */ 
 
      continue;
      }  /* END if (ShowTemplate == TRUE) */

    MDB(5,fUI) MLog("INFO:     '%s'  TotalJobCount: %3d\n",
      J->Name,
      TotalJC);

    if (J->Credential.U == NULL)
      {
      MStringAppendF(String,"  WARNING:  job '%s' has invalid credentials (user is null)\n",
        J->Name);
      }

    /* verify resource utilization */

    if (MJOBISACTIVE(J) == TRUE)
      {
      if ((RQ->DRes.Procs > 0) &&
          (RQ->URes != NULL) &&
          ((double)RQ->URes->Procs / 100.0 > ((double)RQ->DRes.Procs * RQ->TaskCount)))
        {
        MStringAppendF(String,"  WARNING:  job '%s' utilizes more procs than dedicated (%.2f > %d)\n",
          J->Name,
          RQ->URes->Procs / 100.0,
          RQ->DRes.Procs);
        }

      if ((RQ->URes != NULL) && (RQ->URes->Mem > RQ->DRes.Mem) && (RQ->DRes.Mem > 0))
        {
        MStringAppendF(String,"  WARNING:  job '%s' utilizes more memory than dedicated (%d > %d)\n",
          J->Name,
          RQ->URes->Mem,
          RQ->DRes.Mem);
        }

      /* Report if allocated nodes are idle. - RT135 
       * Report unknown state of job if job is running and one node is down. */

      if ((bmisset(FlagBM,mcmVerbose)) && 
          (!MNLIsEmpty(&RQ->NodeList)))
        {
        mbool_t UnknownState = FALSE;
        mbool_t LoadIsLow   = TRUE;

        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          if (N->State == mnsDown)
            UnknownState = TRUE;

          if (((N->Load) / MAX(1,N->DRes.Procs)) > 0.2)
            LoadIsLow = FALSE;

          if ((UnknownState == TRUE) && (LoadIsLow == FALSE))
            break;
          }
            
        if (UnknownState == TRUE)
          MStringAppendF(String,"  WARNING:  job '%s' is in an unknown state due to downed node(s)\n",
            J->Name);

        if (LoadIsLow == TRUE)
          MStringAppendF(String,"  WARNING:  job '%s' is idle on all allocated nodes\n",
            J->Name);

        } /* END if (RQ->NodeList != NULL) */
      }    /* END if ((J->State == mjsRunning) || (J->State == mjsStarting)) */

    /* verify job state consistency */

    if ((J->State != J->EState) &&
        (J->EState != mjsDeferred))
      {
      if ((J->State != mjsRunning) || (J->EState != mjsStarting))
        {
        MStringAppendF(String,"  WARNING:  job '%s' state '%s' does not match expected state '%s'\n",
          J->Name,
          MJobState[J->State],
          MJobState[J->EState]);
        }
      }

    /* verify pmask */

    if (bmisclear(&J->PAL))
      {
      MStringAppendF(String,"  WARNING:  job '%s' has invalid pmask\n",
        J->Name);
      }

    /* if job is active... */

    if (MJOBISACTIVE(J) == TRUE)
      {
      ActiveJC++;

      /* check reservation */

      if (!bmisset(&J->SpecFlags,mjfRsvMap) && 
          !bmisset(&J->SpecFlags,mjfNoResources) &&
         (J->Rsv == NULL))
        {
        MStringAppendF(String,"  WARNING:  active job '%s' has no reservation\n",
          J->Name);
        }

      /* check if preempt attempt failed */

      if (bmisset(&J->IFlags,mjifPreemptFailed))
        {
        MStringAppendF(String,"  WARNING:  attempt to preempt job '%s' failed this iteration\n",
          J->Name);
        }

      /* search for active job in MAQ table */

      if (MUHTGet(&MAQ,J->Name,NULL,NULL) == FAILURE)
        {
        MStringAppendF(String,"  WARNING:  active job '%s' is not in MAQ table\n",
          J->Name);
        }

      /* verify WallClock limit */

      if ((MSched.Time - J->StartTime) > J->WCLimit)
        {
        MULToTString(J->WCLimit,TString);

        strcpy(WCLimit,TString);

        MULToTString(MSched.Time - J->StartTime,TString);

        MStringAppendF(String,"  WARNING:  job %s has exceeded wallclock limit (%s > %s) \n",
          J->Name,
          TString,
          WCLimit);
        }    /* END if (MSched.Time) */

      if (J->CancelCount > 0)
        {
        MStringAppendF(String,"ALERT:  job %s cannot be cancelled (%d attempts)\n",
          J->Name,
          J->CancelCount);
        }
      }      /* END if (MJOBISACTIVE(J) == TRUE) */
    else if ((MSched.Mode != msmMonitor) && (MSched.Mode != msmTest))
      {
      /* check start count on idle jobs */

      if (J->StartCount >= 4)
        {
        MStringAppendF(String,"  WARNING:  job %s has failed to start %d times\n",
          J->Name,
          J->StartCount);
        }    /* END if (J->StartCount >= 4) */
      }      /* END else if (MJOBISACTIVE(J) == TRUE) */
    }   /* END for jindex */

  if ((DiagOpt != NULL) &&
      (!strcmp(DiagOpt,NONE)) && 
      (bmisset(FlagBM,mcmVerbose2)))
    {
    mhashiter_t HTI;

    /* diagnose linked list consistency */

    /* diagnose MAQ */

    MDB(4,fUI) MLog("INFO:     diagnosing MAQ table\n");

    if (bmisset(FlagBM,mcmVerbose))
      {
      MStringAppendF(String,"\n\nActive Jobs  (max=%d)\n",
        MSched.M[mxoJob]);

      MStringAppendF(String,"%-18s %8.8s %9.9s %5s %11s %11s\n\n",
        "JobID",
        "State",
        "User",
        "Proc",
        "WCLimit",
        "StartTime");
      }

    MAQJC = 0;

    MUHTIterInit(&HTI);

    while (MUHTIterate(&MAQ,NULL,(void **)&J,NULL,&HTI) == SUCCESS)
      {
      if (bmisset(FlagBM,mcmVerbose))
        {
        MULToTString(J->WCLimit,TString);

        strcpy(WCLimit,TString);

        MULToTString(J->StartTime - MSched.Time,TString);

        MStringAppendF(String,"%-18s %8.8s %9.9s %5d %11s %11s\n\n",
          J->Name,
          (J->State > mjsNONE) ? MJobState[J->State] : "-",
          (J->Credential.U != NULL) ? J->Credential.U->Name : "-",
          J->TotalProcCount,
          WCLimit,
          TString);
        }

      /* verify node consistency */

      MDB(4,fUI) MLog("INFO:     diagnosing node consistency of active job '%s'\n",
        J->Name);

      PC = 0;
      NC = 0;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          if (!bmisset(&MSched.Flags,mschedfShowRequestedProcs) &&
             ((RQ->NAccessPolicy == mnacSingleJob) ||
              (RQ->NAccessPolicy == mnacSingleTask) ||
              (RQ->DRes.Procs == -1)))
            {
            PC += N->CRes.Procs;
            }
          else
            {
            PC += (MNLGetTCAtIndex(&RQ->NodeList,nindex) * RQ->DRes.Procs);
            }

          if (bmisset(&J->PAL,N->PtIndex) == FAILURE)
            {
            char ParLine[MMAX_LINE];

            MStringAppendF(String,"  WARNING:  job '%s' with partition mask %s has node %s allocated from partition %s\n",
              J->Name,
              (bmisclear(&J->PAL)) ?
                ALL : MPALToString(&J->PAL,NULL,ParLine),
              N->Name,
              MPar[N->PtIndex].Name);
            }

          if ((RQ->DRes.Procs != 0) &&
              (N->State != mnsBusy) &&
              (N->State != mnsActive) &&
              (N->State != mnsDraining) &&
              (N != MSched.GN) &&
             ((MSched.Time - J->StartTime) > 300))
            {
            MULToTString(MSched.Time - J->StartTime,TString);

            MStringAppendF(String,"  WARNING:  active job '%s' has inactive node %s allocated for %s (node state: '%s')\n",
              J->Name,
              N->Name,
              TString,
              MNodeState[N->State]);
            }    /* END if ((N->State != mnsBusy) && ...) */
          }      /* END for (nindex)            */

        NC += nindex;
        }        /* END for (rqindex)           */

      if (NC != J->Alloc.NC)
        {
        MStringAppendF(String,"  WARNING:  active job '%s' has inconsistent node allocation  (nodes: %d  nodelist size: %d)\n",
          J->Name,
          J->Alloc.NC,
          NC);
        }

      if (PC != J->TotalProcCount)
        {
        MStringAppendF(String,"  WARNING:  active job %s has inconsistent proc allocation  (procs: %d  nodelist procs: %d)\n",
          J->Name,
          J->TotalProcCount,
          PC);
        }

      MAQJC++;
      }  /* END for (jindex) */

    if (ShowTemplate == TRUE)
      {
      MStringAppendF(String,"\n\nTotal Jobs: %d\n",
        TotalJC);
      }
    else
      {
      MStringAppendF(String,"\n\nTotal Jobs: %d  Active Jobs: %d\n",
        TotalJC,
        ActiveJC);

      if (ActiveJC != MAQJC)
        {
        MStringAppendF(String,"  WARNING:  active job table is corrupt (active jobs (%d) != active queue size (%d))\n",
          ActiveJC,
          MAQJC);
        }

      /* verify reservations ??? */
      }  /* END else (ShowTemplate == TRUE) */
    }    /* END if (!strcmp(DiagOpt,NONE)) */

  return(SUCCESS);
  }  /* END MUIJobDiagnose() */


/* END MUIJobDiagnose.c */
