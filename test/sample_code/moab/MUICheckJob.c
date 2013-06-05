/* HEADER */

/**
 * @file MUICheckJob.c
 *
 * Contains MUI Check Job Function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




#define MMAX_NODE_PER_LINE 6



/**
 * Populates Buffer with job diagnostics in human readable format.
 *
 * @param J (I)
 * @param Buffer - should already be initialized, will be populated
 * @param PLevel - policy level for evaluation
 * @param IgnBM - which warnings to ignore (optional)
 * @param RsvList - reservation access (optional)
 * @param QOSList - qos access (optional)
 * @param NodeName - node access (optional)
 * @param Flags
 */

int MUICheckJob(

  mjob_t               *J,
  mstring_t            *Buffer,
  enum MPolicyTypeEnum  PLevel,
  mbitmap_t            *IgnBM,
  char                 *RsvList,
  char                 *QOSList,
  char                 *NodeName,
  mbitmap_t            *Flags)

  {
  int      VerboseLevel = 0;

  char     Line[MMAX_LINE];
  char     tmpBuffer[MMAX_BUFFER];
  char     Duration[MMAX_NAME];
  char     ProcLine[MMAX_NAME];
  char     StartTime[MMAX_NAME];
  char     EndTime[MMAX_NAME];
  char     WCLimit[MMAX_LINE];
  char     TString[MMAX_LINE];
  char     DString[MMAX_LINE];

  msdata_t *SD;
  mreq_t   *RQ;
  mnode_t  *N;
  mjob_t   *JA = NULL;

  double PE;

  char    *ptr;

  int      TaskCount;
  int      index;
  int      nindex;
  int      rqindex;
  int      tindex;

  mbool_t IsCanceling = FALSE;

  Line[0]     = '\0';

  if (bmisset(Flags,mcmVerbose2))
    VerboseLevel = 2;
  else if (bmisset(Flags,mcmVerbose))
    VerboseLevel = 1;
  else
    VerboseLevel = 0;

  if ((VerboseLevel >= 1) &&
      (J->DRMJID != NULL) &&
      (strcmp(J->Name,J->DRMJID)))
    {
    MStringAppendF(Buffer,"job %s (RM job '%s')\n\n",
        J->Name,
        J->DRMJID);
    }
  else
    {
    MStringAppendF(Buffer,"job %s\n\n",
        J->Name);
    }

  RQ = J->Req[0];

  /* NOTE:  tabbing off from here to end of while loop - move to 
     separate routine */

  /* NOTE:  previously, is cancelling was dependent upon the job being 
     staged and active, why?  ie !MJOBISCOMPLETE(J) && J->DRM != NULL */

  if ((MJOBISCOMPLETE(J) == FALSE) && bmisset(&J->IFlags,mjifWasCanceled))
    {
    IsCanceling = TRUE;
    }

  if (J->State != J->EState)
    {
    MULToDString((mulong *)&J->SubmitTime,DString);

    MDB(1,fUI) MLog("job '%s'  State: %11s  QueueTime: %26s",
        J->Name,
        (IsCanceling == FALSE) ? MJobState[J->State] : "Canceling",
        DString);
    }
  else
    {
    MULToDString((mulong *)&J->SubmitTime,DString);

    MDB(1,fUI) MLog("job '%s'  State: %11s  Expected State:  %9s   QueueTime: %26s",
        J->Name,
        (IsCanceling == FALSE) ? MJobState[J->State] : "Canceling",
        MJobState[J->EState],
        DString);
    }

  if (J->AName != NULL)
    {
    /* user specified name */

    MStringAppendF(Buffer,"AName: %s\n",
        J->AName);
    }

  if (J->State == mjsStaged)
    {
    sprintf(Line,"(destination: %s)",
        (J->DestinationRM != NULL) ? J->DestinationRM->Name : "???");
    }

  /* process array jobs */

  if ((J->Array != NULL) || 
      (bmisset(&J->Flags,mjfArrayJob) && 
       (VerboseLevel >= 1)))
    {
    int jindex;

    int BlockedCount = 0;
    int EligibleCount = 0;
    int TotalSubJobs = 0;

    enum MJobStateEnum SubJobState;

    mjob_t *JMaster = NULL;

    mjarray_t *JA = J->Array;

    if (JA == NULL)
      {
      /* find the array master of the job */

      if (((J->JGroup != NULL) && 
            (MJobFind(J->JGroup->Name,&JMaster,mjsmExtended) == FAILURE)) ||     
          (JMaster == NULL))
        {
        MDB(1,fSCHED) MLog("ERROR:    cannot find master job (%s) for job '%s' for checkjob \n",
            J->JGroup->Name,
            J->Name);
        }
      else
        {
        JA = JMaster->Array;
        }
      }

    /* show job array info */
    if (JA != NULL)
      {

      MStringAppendF(Buffer,"Job Array Info:\n");

      MStringAppendF(Buffer,"  Name: %s\n",
          J->Name);

      if (JA->Limit > 0)
        {
        MStringAppendF(Buffer,"  Slot Limit: %d\n\n",
            JA->Limit);
        }

      if (JA->PLimit > 0)
        {
        MStringAppendF(Buffer,"  Partition Limit: %d\n\n",
            JA->PLimit);
        }

      if (JA->Members != NULL)
        {
        MJobArrayGetArrayCount(JA,&TotalSubJobs);

        for (jindex = 0;jindex < MMAX_JOBARRAYSIZE;jindex++)
          {
          if (JA->Members[jindex] == -1)
            break;

          /* get proper job state here... */

          SubJobState = JA->JState[jindex];

          if (SubJobState == mjsIdle)
            {
            if (MJobIsBlocked(JA->JPtrs[jindex]))
              {
              BlockedCount++;
              SubJobState = mjsBlocked;
              }
            else
              {
              EligibleCount++;
              }
            } /* END if (SubJobState == mjsIdle) */

          if (VerboseLevel >= 1)
            {
            MStringAppendF(Buffer,"  %d : %s : %s\n",
                JA->Members[jindex],
                JA->JName[jindex],
                MJobState[SubJobState]);
            }
          }  /* END for (jindex) */
        }  /* END if (JA->Members != NULL) */

      MStringAppendF(Buffer,"\n  Sub-jobs:    %8d\n",TotalSubJobs);
      MStringAppendF(Buffer,"    Active:    %8d ( %03.1f%% )\n",
          JA->Active,
          ((float)JA->Active / (float)TotalSubJobs) * 100);
      MStringAppendF(Buffer,"    Eligible:  %8d ( %03.1f%% )\n",
          EligibleCount,
          ((float)EligibleCount / (float)TotalSubJobs) * 100);
      MStringAppendF(Buffer,"    Blocked:   %8d ( %03.1f%% )\n",
          BlockedCount,
          ((float)BlockedCount / (float)TotalSubJobs) * 100);
      MStringAppendF(Buffer,"    Completed: %8d ( %03.1f%% )\n\n",
          JA->Complete,
          ((float)JA->Complete / (float)TotalSubJobs) * 100);
      }
    }  /* END if (J->Array != NULL) */

  if (!bmisset(&J->IFlags,mjifIsTemplate))
    {
    MStringAppendF(Buffer,"State: %s%s%s %s\n",
        (IsCanceling == FALSE) ? MJobState[J->State] : "Canceling",
        (J->EState != J->State) ? "  EState: ": "",
        (J->EState != J->State) ? MJobState[J->EState] : "",
        Line);
    }
  else
    {
    MStringAppendF(Buffer,"JOB TEMPLATE\n");
    }

  if (MJOBISCOMPLETE(J))
    {
    MULToDString((mulong *)&J->CompletionTime,DString);

    MStringAppendF(Buffer,"Completion Code: %d  Time: %s",
        J->CompletionCode,
        DString);
    }

  if (J->SubState == mjsstProlog)
    {
    MStringAppendF(Buffer,"  NOTE: job is currently in prolog\n");
    }

  if (J->SubState == mjsstEpilog)
    {
    MStringAppendF(Buffer,"  NOTE: job is currently in epilog\n");
    }

  if (bmisset(&J->RsvAccessBM,mjapAllowAll))
    {
    MStringAppendF(Buffer,"  NOTE: job can access all reservations\n");
    }

  if (bmisset(&J->RsvAccessBM,mjapAllowJob))
    {
    MStringAppendF(Buffer,"  NOTE: job can access all job reservations\n");
    }

  if (bmisset(&J->RsvAccessBM,mjapAllowAllNonJob))
    {
    MStringAppendF(Buffer,"  NOTE: job can access all non-job reservations\n");
    }

  snprintf(tmpBuffer,sizeof(tmpBuffer),"%s%s%s%s%s%s%s%s%s%s",
      (J->Credential.U != NULL) ? "  user:" : "",
      (J->Credential.U != NULL) ? J->Credential.U->Name : "",
      ((J->Credential.G != NULL) && strcmp(J->Credential.G->Name,DEFAULT)) ? "  group:" : "",
      ((J->Credential.G != NULL) && strcmp(J->Credential.G->Name,DEFAULT)) ? J->Credential.G->Name : "",
      (J->Credential.A != NULL) ? "  account:" : "",
      (J->Credential.A != NULL) ? J->Credential.A->Name : "",
      (J->Credential.C != NULL) ? "  class:" : "",
      (J->Credential.C != NULL) ? J->Credential.C->Name : "",
      (J->Credential.Q != NULL) ? "  qos:" : "",
      (J->Credential.Q != NULL) ? J->Credential.Q->Name : "");

  if (tmpBuffer[0] != '\0')
    {
    MStringAppendF(Buffer,"Creds:%s\n",
        tmpBuffer);
    }

  if ((J->QOSRequested != NULL) && (J->Credential.Q != J->QOSRequested))
    {
    MStringAppendF(Buffer,"  (Requested QOS: %s)\n",
        J->QOSRequested->Name);
    }

  if (J->EUser != NULL)
    {
    MStringAppendF(Buffer,"  (Execution User: %s)\n",
        J->EUser);
    }

  MULToTString(J->AWallTime,TString);

  strcpy(Duration,TString);

  Line[0] = '\0';

  if ((!bmisset(&J->IFlags,mjifIsTemplate)) ||
      (J->SpecWCLimit[0] != 0))
    {
    if ((J->OrigMinWCLimit != 0) && (J->OrigWCLimit != 0))
      {
      char tmpWC[MMAX_NAME];
      MULToTString(J->OrigMinWCLimit,TString);

      strcpy(tmpWC,TString);
      MULToTString(J->OrigWCLimit,TString);

      sprintf(WCLimit,"%s-%s",
          tmpWC,
          TString);
      }
    else
      {
      MULToTString(J->SpecWCLimit[0],TString);

      strcpy(WCLimit,TString);
      }

    MStringAppendF(Buffer,"WallTime:   %s of %s%s\n",
        Duration,
        WCLimit,
        Line);

    if (J->WCLimit != J->SpecWCLimit[0])
      {
      MULToTString(J->WCLimit,TString);

      MStringAppendF(Buffer,"Adjusted WCLimit: %s\n",
          TString);
      }

    if ((J->OrigWCLimit > 0) && (J->SpecWCLimit[0] != J->OrigWCLimit))
      {
      MULToTString(J->OrigWCLimit,TString);

      MStringAppendF(Buffer,"Original (Non-Virtual) WCLimit: %s\n",
          TString);
      }

    if (J->SWallTime > 0)
      {
      MULToTString(J->SWallTime,TString);

      MStringAppendF(Buffer,"Suspended Wall Time: %s\n",
          TString);
      }

    if (J->SpecWCLimit[2] > 0)
      {
      /* job is malleable - show all specified times */

      strcpy(Line,"SpecWCLimit: ");

      for (tindex = 1;J->SpecWCLimit[tindex] > 0;tindex++)
        {
        if (tindex > 1)
          strcat(Line,",");

        sprintf(Line,"%s %ld",
            Line,
            J->SpecWCLimit[tindex]);
        }  /* END for (tindex) */

      MStringAppendF(Buffer,"%s  ",
          Line);
      }  /* END if (J->SpecWCLimit[2] > 0) */
    }    /* END if (!bmisset(&J->IFlags,mjifIsTemplate) || (J->SpecWCLimit[0] != 0)) */

  if ((J->CPULimit > 0) && (J->Req[0]->LURes != NULL))
    {
    int rqindex;
    int CPUUsage;

    char UsageLine[MMAX_NAME];

    CPUUsage = 0;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      CPUUsage += J->Req[rqindex]->LURes->Procs / 100;
      }

    MULToTString(CPUUsage,TString);

    strcpy(UsageLine,TString);
    MULToTString(J->CPULimit,TString);

    MStringAppendF(Buffer,"CPUTime:    %s of %s\n",
        UsageLine,
        TString);
    }  /* END if (J->CPULimit > 0) */

  if (!bmisset(&J->IFlags,mjifIsTemplate))
    {
    long TQDuration;
    long EQDuration;

    char tmpLine[MMAX_NAME];

    if ((J->StartTime > 0) && 
        ((MJOBISACTIVE(J) == TRUE) || (MJOBISCOMPLETE(J) == TRUE)))
      TQDuration = J->StartTime - J->SubmitTime;
    else
      TQDuration = MSched.Time - J->SubmitTime;

    if (J->EffQueueDuration >= 0)
      {
      EQDuration = J->EffQueueDuration;
      }
    else if (bmisclear(&J->Hold))
      {
      /* EffQueueDuration not set, use SystemQueueTime */

      if ((MJOBISACTIVE(J) == TRUE))
        {
        EQDuration = J->StartTime - J->SystemQueueTime;
        }
      else
        {
        EQDuration = MSched.Time - J->SystemQueueTime;
        }
      }
    else
      {
      EQDuration = 0;
      }

    MULToTString(TQDuration,TString);

    strcpy(tmpLine,TString);

    if (J->EligibleTime > 0)
      {
      MULToDString((mulong *)&J->EligibleTime,DString);

      MStringAppendF(Buffer,"BecameEligible: %s",
          DString);
      }

    MULToTString(MAX(0,EQDuration),TString);

    MULToDString((mulong *)&J->SubmitTime,DString);

    MStringAppendF(Buffer,"SubmitTime: %s  (Time Queued  Total: %s  Eligible: %s)\n\n",
        DString,
        tmpLine,
        TString);

    if (MJOBISACTIVE(J) == TRUE)
      {
      MULToDString((mulong *)&J->StartTime,DString);

      MStringAppendF(Buffer,"StartTime: %s",
          DString);
      }

    if ((VerboseLevel >= 2) && (J->CheckpointStartTime > 0))
      {
      MULToDString((mulong *)&J->CheckpointStartTime,DString);

      MStringAppendF(Buffer,"CheckpointStartTime: %s",
          DString);
      }
    }  /* END if (!bmisset(&J->IFlags,mjifIsTemplate)) */

  if (J->SMinTime > MSched.Time)
    {
    MULToTString(J->SMinTime - MSched.Time,TString);
    MULToDString((mulong *)&J->SMinTime,DString);

    MStringAppendF(Buffer,"StartDate: %s  %s",
        TString,
        DString);
    }

  if ((J->CMaxDate > MSched.Time) && (J->CMaxDate < MMAX_TIME))
    {
    char *tmpPtr = NULL;

    /* MULToDString returns a string with a newline */

    MULToDString((mulong *)&J->CMaxDate,DString);
    tmpPtr = DString;

    tmpPtr[strlen(tmpPtr) - 1] = '\0';

    MULToTString(J->CMaxDate - MSched.Time,TString);

    MStringAppendF(Buffer,"Deadline:  %s  (%s)\n",
        TString,
        tmpPtr);
    }
  else
    {
    /* Check for failed deadline where current time greater than expected completion time */

    /* NOTE:  Once job completes, CMaxDate is cleared, which destroys the deadline info.
              This also means you won't see the failure message after the job completes.  */

    if ((J->CMaxDate > 0) && (MSched.Time > J->CMaxDate))
      {
      char buffer[MMAX_LINE];

      MULToDString((mulong *)&J->CMaxDate,buffer);
      buffer[strlen(buffer) - 1] = '\0';
                 
      MULToTString(J->CMaxDate - MSched.Time,TString);
      
      MStringAppendF(Buffer,"Deadline Failed:  Expected at '%s' (%s)\n",buffer,TString);
      }
    }

  if ((J->TermTime != 0) && (J->TermTime != MMAX_TIME))
    {
    char *tmpPtr = NULL;

    /* MULToDString returns a string with a newline */

    MULToDString((mulong *)&J->TermTime,DString);
    tmpPtr = DString;

    tmpPtr[strlen(tmpPtr) - 1] = '\0';

    MULToTString(J->TermTime - MSched.Time,TString);

    MStringAppendF(Buffer,"TerminationDate: %s  (%s)\n",
        TString,
        tmpPtr);
    }

  if (J->MinPreemptTime != 0)
    {
    MULToTString(J->MinPreemptTime,TString);

    MStringAppendF(Buffer,"MinPreemptTime: %s\n",
        TString);
    }

  if (J->Credential.Templates != NULL)
    {
    /* Templates are 'template matches' (see TStats/TSets below) */

    int jindex;

    MStringAppend(Buffer,"Job Templates: ");

    for (jindex = 0;J->Credential.Templates[jindex] != NULL;jindex++)
      {
      if (jindex >= MSched.M[mxoxTJob])
        break;

      if (jindex > 0)
        MStringAppendF(Buffer,",%s",
            J->Credential.Templates[jindex]);
      else
        MStringAppendF(Buffer,"%s",
            J->Credential.Templates[jindex]);
      }

    MStringAppend(Buffer,"\n");
    }  /* END if (J->Cred.Templates != NULL) */

  if (J->TSets != NULL)
    {
    char tmpLine[MMAX_LINE];

    MULLToString(J->TSets,FALSE,NULL,tmpLine,sizeof(tmpLine));

    MStringAppendF(Buffer,"TemplateSets:  %s\n",
        tmpLine);
    }

  if (J->TStats != NULL)
    {
    char tmpLine[MMAX_LINE];

    MULLToString(J->TStats,FALSE,NULL,tmpLine,sizeof(tmpLine));

    MStringAppendF(Buffer,"TemplateStats: %s\n",
        tmpLine);
    }

  if (J->MaxJobMem > 0)
    {
    MStringAppendF(Buffer,"MaxJobMem: %s %s",
        MULToRSpec(J->MaxJobMem,mvmMega,NULL),
        (J->MaxJobProc > 0) ? "" : "\n");
    } 

  if (J->MaxJobProc > 0)
    {
    MStringAppendF(Buffer,"MaxJobProc: %d\n",
        J->MaxJobProc);
    }

  if (!bmisclear(&J->ReqFeatureGResBM))
    {
    char Line[MMAX_LINE];

    MStringAppendF(Buffer,"Feature GRes: %s\n",
        MUNodeFeaturesToString(',',&J->ReqFeatureGResBM,Line));
    }

  if (J->Geometry != NULL)
    {
    MStringAppendF(Buffer,"Geometry: %s\n",
        J->Geometry);
    }

  if (!bmisclear(&J->NodeMatchBM))
    {
    char *s;
    char Policy[MMAX_LINE];

    s = bmtostring(&J->NodeMatchBM,MJobNodeMatchType,Policy);
    if (NULL != s)
      MStringAppendF(Buffer,"NodeMatchPolicy: %s\n",s);
    else
      MStringAppendF(Buffer,"NodeMatchPolicy: \n");
    }

  if (J->RMSubmitFlags != NULL)
    {
    MStringAppendF(Buffer,"RMSubmitFlags: %s\n",J->RMSubmitFlags);
    }

  if (J->Triggers != NULL)
    {
    mstring_t tmp(MMAX_LINE);

    /* display job triggers */

    MTListToMString(J->Triggers,TRUE,&tmp);

    *Buffer += "Triggers: ";
    *Buffer += tmp;
    *Buffer += '\n';
    }  /* END if (J->T != NULL) */

  if (J->DataStage != NULL)
    {
    SD = J->DataStage->SIData;
 
    if ((SD != NULL) &&
        (J->DestinationRM != NULL) &&
        ((J->DestinationRM->DataRM == NULL) ||
         (J->DestinationRM->DataRM[0] == NULL)))
      {
      MStringAppendF(Buffer,"  WARNING:  job has required stage-in data, but RM '%s' has no associated data RM configured for data staging (assuming RM '%s' will handle data staging)\n",
          J->DestinationRM->Name,
          J->DestinationRM->Name);
      }
 
    /* display stage-in reqs */
 
    if (SD != NULL)
      {
      MStringAppendF(Buffer,"Stage-In Requirements:\n");
      }
 
    while (SD != NULL)
      {
      MStringAppendF(Buffer,"\n");
 
      if ((MJOBISACTIVE(J) == TRUE) && (SD->State != mdssStaged))
        {
        MStringAppendF(Buffer,"  WARNING:  job is active but required stage-in data '%s' is not available\n",
            (SD->DstFileName != NULL) ? SD->DstFileName : ((SD->SrcFileName != NULL) ? SD->SrcFileName : "???"));
        }

      MULToTString(SD->ESDuration,TString);

      MStringAppendF(Buffer,"  %s:%s => %s:%s  size:%luB  status:%s  remaining:%s\n",
          ((SD->SrcHostName != NULL) && (SD->SrcHostName[0] != '\0')) ? SD->SrcHostName : "localhost",
          (SD->SrcFileName != NULL) ? SD->SrcFileName : "???",
          ((SD->DstHostName != NULL) && (SD->DstHostName[0] != '\0')) ? SD->DstHostName : "localhost",
          (SD->DstFileName != NULL) ? SD->DstFileName : ((SD->SrcFileName != NULL) ? SD->SrcFileName : "???"),
          SD->SrcFileSize,
          MDataStageState[SD->State],
          ((SD->ESDuration == MMAX_TIME) ? "unknown" : TString));
 
      if (VerboseLevel >= 1)
        {
        MStringAppendF(Buffer,"    Transfer URL: %s,%s\n",
            (SD->SrcLocation != NULL) ? SD->SrcLocation : "-",
            (SD->DstLocation != NULL) ? SD->DstLocation : "-");
 
        if (SD->EMsg != NULL)
          {
          MStringAppendF(Buffer,"    Errors: \"%s\"\n",
              SD->EMsg);
          }
        }
 
      SD = SD->Next;
      }  /* END while (SD != NULL) */
 
    if (J->DataStage->SIData != NULL)
      MStringAppendF(Buffer,"\n");
 
    /* display stage-out reqs */
 
    SD = J->DataStage->SOData;
 
    if ((SD != NULL) &&
        (J->DestinationRM != NULL) &&
        (J->DestinationRM->Type == mrmtMoab) &&
        ((J->DestinationRM->DataRM == NULL) ||
         (J->DestinationRM->DataRM[0] == NULL)))
      {
      MStringAppendF(Buffer,"  WARNING:  job has required stage-out data, but RM '%s' is not properly configured for data staging\n",
          J->DestinationRM->Name);
      }
 
    if (SD != NULL)
      {
      MStringAppendF(Buffer,"Stage-Out Requirements:\n");
      }
 
    while (SD != NULL)
      {
      MStringAppendF(Buffer,"\n");
 
      MULToTString(SD->ESDuration,TString);

      MStringAppendF(Buffer,"  %s:%s => %s:%s  size:%ldB  status:%s  remaining:%s\n",
          ((SD->SrcHostName != NULL) && (SD->SrcHostName[0] != '\0')) ? SD->SrcHostName : "localhost",
          (SD->SrcFileName != NULL) ? SD->SrcFileName : "???",
          ((SD->DstHostName != NULL) && (SD->DstHostName[0] != '\0')) ? SD->DstHostName : "localhost",
          (SD->DstFileName != NULL) ? SD->DstFileName : ((SD->SrcFileName != NULL) ? SD->SrcFileName : "???"),
          SD->SrcFileSize,
          MDataStageState[SD->State],
          ((SD->ESDuration == MMAX_TIME) ? "unknown" : TString));
 
      if (VerboseLevel >= 1)
        {
        MStringAppendF(Buffer,"    Transfer URL: %s,%s\n",
            SD->SrcLocation,
            SD->DstLocation);
 
        if (SD->EMsg != NULL)
          {
          MStringAppendF(Buffer,"    Errors: \"%s\"\n",
              SD->EMsg);
          }
        }
 
      SD = SD->Next;
      }  /* END while (SD != NULL) */
 
    if (J->DataStage->SOData != NULL)
      MStringAppendF(Buffer,"\n");
    }  /* END if (J->DS != NULL) */

  if (!bmisset(&J->IFlags,mjifIsTemplate) || (J->Request.TC != 0))
    {
    if (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (J->Request.TC % 1024 == 0))
      {
      MStringAppendF(Buffer,"Total Requested Tasks: %dk\n",
          J->Request.TC >> 10);
      }
    else
      {
      MStringAppendF(Buffer,"Total Requested Tasks: %d\n",
          J->Request.TC);
      }
    }

  if (!bmisset(&J->IFlags,mjifIsTemplate) && !bmisset(&J->Flags,mjfNoResources)) 
    {
    if (J->Request.TC <= 0)
      {
      MStringAppendF(Buffer,"  WARNING:  job requests no tasks\n");
      }
    }

  if (VerboseLevel >= 1)
    {

    if ((VerboseLevel >= 2) || (J->Request.NC > 0) || (MSched.BluegeneRM == TRUE))
      {
      if (MSched.BluegeneRM == TRUE) 
        {
        if ((bmisset(&MSched.DisplayFlags,mdspfCondensed)) && (((J->Request.TC / MSched.BGNodeCPUCnt) % 1024) == 0))
          MStringAppendF(Buffer,"Total Requested Nodes: %dk\n",(J->Request.TC / MSched.BGNodeCPUCnt) >> 10);
        else
          MStringAppendF(Buffer,"Total Requested Nodes: %d\n",J->Request.TC / MSched.BGNodeCPUCnt);
        }
      else
        MStringAppendF(Buffer,"Total Requested Nodes: %d\n",J->Request.NC);
      }

    if (J->TotalTaskCount > 0)
      {
      MStringAppendF(Buffer,"Requested Total Task Count: %d\n",
          J->TotalTaskCount);
      }

    mstring_t tmpLine(MMAX_LINE);

    MJobCheckDependency(J,NULL,TRUE,NULL,&tmpLine);

    if (!tmpLine.empty())
      {
      MStringAppendF(Buffer,"Depend:    %s\n",
          tmpLine.c_str());
      }

    if (!MNLIsEmpty(&J->ExcHList))
      {
      mstring_t tmp(MMAX_LINE);

      if (MJobAToMString(J,mjaExcHList,&tmp) == SUCCESS)
        {
        *Buffer += "Exclude HostList: ";
        *Buffer += tmp;
        *Buffer += '\n';
        }
      }
    }    /* END if (VerboseLevel >= 1) */

  if (bmisset(&J->Flags,mjfClusterLock) &&
      bmisset(&J->Flags,mjfNoRMStart))
    {
    MStringAppendF(Buffer,"NOTE: job is being scheduled/run on a remote peer\n");
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    MStringAppendF(Buffer,"\nReq[%d]  TaskCount: %d  Partition: %s",
        rqindex,
        RQ->TaskCount,
        MPar[RQ->PtIndex].Name);

    if ((J->Req[1] != NULL) &&
        bmisset(&J->Flags,mjfCoAlloc))
      {        
      if ((MRM[1].Type != mrmtNONE) && 
          (MJOBISACTIVE(J) == TRUE))
        {
        MStringAppendF(Buffer,"  RM: %s",
            MRM[RQ->RMIndex].Name);
        }

      if (RQ->ReqJID != NULL)
        {
        MStringAppendF(Buffer,"  ReqJID: %s",
            RQ->ReqJID);
        }

      if (RQ->State != mjsNONE)
        {
        MStringAppendF(Buffer,"\nState: %s",
            MJobState[RQ->State]);
        }
      }

    MStringAppend(Buffer,"\n");

    if (MJobWantsSharedMem(J))
      {
      MStringAppendF(Buffer,"Memory Requirement: %s\n",
          MULToRSpec((long)RQ->ReqMem,mvmMega,NULL));
      }

    if (RQ->TaskRequestList[2] > 0)
      {
      strcpy(Line,"TaskRequestList: ");

      for (tindex = 1;RQ->TaskRequestList[tindex] > 0;tindex++)
        {
        if (tindex > 1)
          strcat(Line,",");

        sprintf(Line,"%s%d",
            Line,
            RQ->TaskRequestList[tindex]);
        }  /* END for (tindex) */

      MStringAppendF(Buffer,"%s  ",
          Line);
      }  /* END if (RQ->TaskRequestList[2] > 0) */

    if (RQ->ReqNR[mrProc] > 0)
      {
      sprintf(ProcLine,"Procs %s%d  ",
          MComp[(int)RQ->ReqNRC[mrProc]],
          RQ->ReqNR[mrProc]);
      }
    else
      {
      ProcLine[0] = '\0';
      }

    if ((ProcLine[0] != '\0') ||
        (RQ->ReqNR[mrMem] > 0) || 
        (RQ->ReqNR[mrDisk] > 0) ||
        (RQ->ReqNR[mrSwap] > 0))
      {
      char tmpBuf1[MMAX_NAME];
      char tmpBuf2[MMAX_NAME];
      char tmpBuf3[MMAX_NAME];

      /* NOTE: ReqNRC set to mcmpGE by default */

      MStringAppendF(Buffer,"%sMemory %s %s  Disk %s %s  Swap %s %s\n",
          ProcLine,
          (RQ->ReqNRC[mrMem] > 0) ? MComp[(int)RQ->ReqNRC[mrMem]] : ">=",
          MULToRSpec((long)RQ->ReqNR[mrMem],mvmMega,tmpBuf1),
          (RQ->ReqNRC[mrDisk] > 0) ? MComp[(int)RQ->ReqNRC[mrDisk]] : ">=",
          MULToRSpec((long)RQ->ReqNR[mrDisk],mvmMega,tmpBuf2),
          (RQ->ReqNRC[mrSwap] > 0) ? MComp[(int)RQ->ReqNRC[mrSwap]] : ">=",
          MULToRSpec((long)RQ->ReqNR[mrSwap],mvmMega,tmpBuf3));
      }  /* END if ((ProcLine[0] != '\0') || ...)  */



    if ((VerboseLevel >= 2) ||
        (RQ->ReqNRC[mrAMem] != mcmpGE) || (RQ->ReqNR[mrAMem] > 0) ||
        (RQ->ReqNRC[mrASwap] != mcmpGE) || (RQ->ReqNR[mrASwap] > 0))
      {
      char tmpBuf1[MMAX_NAME];
      char tmpBuf2[MMAX_NAME];

      MStringAppendF(Buffer,"Available Memory %s %s  Available Swap %s %s\n",
          (RQ->ReqNRC[mrAMem] > 0) ? MComp[(int)RQ->ReqNRC[mrAMem]] : ">=",
          MULToRSpec((long)RQ->ReqNR[mrAMem],mvmMega,tmpBuf1),
          (RQ->ReqNRC[mrASwap] > 0) ? MComp[(int)RQ->ReqNRC[mrASwap]] : ">=",
          MULToRSpec((long)RQ->ReqNR[mrASwap],mvmMega,tmpBuf2));

      }  /* END if ((VerboseLevel >= 2) || ...) */

    /* required features */
    MRMXFeaturesToString(RQ,Line);

    if ((RQ->Opsys > 0) || (RQ->Arch > 0) || (!MUStrIsEmpty(Line)))
      {
      MStringAppendF(Buffer,"Opsys: %s  Arch: %s  Features: %s\n",
          (RQ->Opsys > 0) ? MAList[meOpsys][RQ->Opsys] : "---",
          (RQ->Arch > 0)  ? MAList[meArch][RQ->Arch] : "---",
          (!MUStrIsEmpty(Line)) ? Line : "---");
      }

    if (bmisset(&J->IFlags,mjifNoGRes))
      {
      MStringAppendF(Buffer,"GRes: NONE\n");
      }

    if (RQ->Pref != NULL)
      {
      MStringAppendF(Buffer,"Preferences:");

      MUNodeFeaturesToString(',',&RQ->Pref->FeatureBM,Line);

      if (!MUStrIsEmpty(Line))
        {
        MStringAppendF(Buffer,"  FEATURE=%s",
            Line);
        }

      if (RQ->Pref->Variables != NULL)
        {
        char tmpLine[MMAX_LINE];

        MULLToString(RQ->Pref->Variables,FALSE,NULL,tmpLine,sizeof(tmpLine));

        if (tmpLine[0] != '\0')
          {
          MStringAppendF(Buffer,"  VARIABLE=%s",
              tmpLine);
          }
        }

      /* NOTE:  report preferred resources (NYI) */

      MStringAppendF(Buffer,"\n");
      }

    if ((VerboseLevel >= 2) ||
        (bmisset(&J->IFlags,mjifIsTemplate) && (RQ->DRes.Procs != 0)) ||
        (!bmisset(&J->IFlags,mjifIsTemplate) && (RQ->DRes.Procs != 1)) ||
        (RQ->DRes.Mem > 0) ||
        (!MSNLIsEmpty(&RQ->DRes.GenericRes)) ||
        (RQ->DRes.Disk > 0) ||
        (RQ->DRes.Swap > 0))
      {
      ptr = MCResToString(&RQ->DRes,0,mfmNONE,NULL);

      MStringAppendF(Buffer,"Dedicated Resources Per Task: %s\n",
          strcmp(ptr,NONE) ? ptr : "---");
      }

    if ((RQ->CRes != NULL) &&
        ((VerboseLevel >= 2) ||
         (!MSNLIsEmpty(&RQ->CRes->GenericRes))))
      {
      ptr = MCResToString(RQ->CRes,0,mfmNONE,NULL);

      MStringAppendF(Buffer,"Configured Resources Per Task: %s\n",
          strcmp(ptr,NONE) ? ptr : "---");
      }

    if ((MJOBISACTIVE(J) == TRUE) &&
        (RQ->URes != NULL) &&
        (RQ->LURes != NULL) &&
        (RQ->MURes != NULL) &&
        (VerboseLevel >= 1))
      {
      ptr = MUUResToString(RQ->URes,0,mfmHuman,NULL);

      if ((ptr[0] != '\0') && strcmp(ptr,NONE))
        {
        MStringAppendF(Buffer,"Utilized Resources Per Task:  %s\n",
            ptr);
        }

      ptr = MUUResToString(RQ->LURes,RQ->RMWTime,mfmHuman,NULL);

      if ((ptr[0] != '\0') && strcmp(ptr,NONE))
        {
        MStringAppendF(Buffer,"Avg Util Resources Per Task:  %s\n",
            ptr);
        }

      ptr = MUUResToString(RQ->MURes,0,mfmHuman,NULL);

      if ((ptr[0] != '\0') && strcmp(ptr,NONE))
        {
        MStringAppendF(Buffer,"Max Util Resources Per Task:  %s\n",
            ptr);
        }
      }    /* END if (MJOBISACTIVE(J) == TRUE) || ...) */

    if (RQ->XURes != NULL)
      {
      int gmindex;

      for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
        {
        double AvgFactor;

        if (RQ->XURes->GMetric[gmindex] == NULL)
          continue;

        if (RQ->XURes->GMetric[gmindex]->IntervalLoad == MDEF_GMETRIC_VALUE)
          continue;

        /* NOTE:  GMetric Avg is SUM(<GMETRICVAL>*<MSched.Interval>/J->Alloc.NC) */

        AvgFactor = (double)((RQ->XURes->GMetric[gmindex]->Avg) * J->TotalProcCount / 
            MAX(1,J->PSDedicated));  

        MStringAppendF(Buffer,"GMetric[%s]  Current: %.2f  Min: %.2f  Max: %.2f  Avg: %.2f Total: %.2f\n",
            MGMetric.Name[gmindex],
            RQ->XURes->GMetric[gmindex]->IntervalLoad,
            RQ->XURes->GMetric[gmindex]->Min,
            RQ->XURes->GMetric[gmindex]->Max,
            AvgFactor / 100.0,
            RQ->XURes->GMetric[gmindex]->TotalLoad);
        }  /* END for (gmindex) */
      }    /* END if (RQ->XURes != NULL) */

    if (RQ->ReqAttr.size() != 0)
      {
      mstring_t Attr(MMAX_LINE);

      MReqAttrToString(RQ->ReqAttr,&Attr);

      MStringAppendF(Buffer,"ReqAttr:  %s\n",Attr.c_str());
      }

    if ((J->AWallTime > 0) && (VerboseLevel >= 1))
      {
      if (J->MSUtilized > 0.01)
        {
        MStringAppendF(Buffer,"Average Utilized Memory: %.2f MB\n",
            J->MSUtilized / J->AWallTime);
        }

      if (J->PSUtilized > 0.01)
        {
        MStringAppendF(Buffer,"Average Utilized Procs: %.2f\n",
            J->PSUtilized / J->AWallTime);
        }
      }

    if ((MJOBISACTIVE(J) == TRUE) &&
        (VerboseLevel >= 1) &&
        (J->Ckpt != NULL))
      {
      char tmpLine[MMAX_LINE];

      MULToTString(J->Ckpt->ActiveCPDuration,TString);

      strcpy(tmpLine,TString);

      MULToTString(J->Ckpt->StoredCPDuration,TString);

      MStringAppendF(Buffer,"Ckpt Walltime:  current: %s   stored: %s\n",
          tmpLine,
          TString);
      }

    if ((RQ->SetSelection != mrssNONE) ||
        (RQ->SetType != mrstNONE) ||
        ((RQ->SetList != NULL) && (RQ->SetList[0] != NULL)))
      {
      int      sindex = 0;

      MStringAppendF(Buffer,"NodeSet=%s%s%s:",
          (RQ->SetSelection != mrssNONE) ? MResSetSelectionType[RQ->SetSelection] : "",
          (RQ->SetSelection != mrssNONE) ? ":" : "",
          MResSetAttrType[RQ->SetType]);

      if (RQ->SetList != NULL)
        {
        for (sindex = 0;RQ->SetList[sindex] != NULL;sindex++)
          {
          if (sindex > 0)
            MStringAppend(Buffer,":");

          MStringAppend(Buffer,RQ->SetList[sindex]);
          }
        }

      if (sindex == 0)
        MStringAppend(Buffer,NONE);

      MStringAppend(Buffer,"\n");

      if (RQ->NodeSetCount > 0)
        {
        MStringAppendF(Buffer,"NodeSetCount=%d\n",
            RQ->NodeSetCount);
        }
      }  /* END if ((RQ->SetSelection != mrssNONE) || ...) */

    if (VerboseLevel >= 1)
      {
      if (RQ->NAccessPolicy != mnacNONE)
        {
        MStringAppendF(Buffer,"NodeAccess: %s\n",
            MNAccessPolicy[RQ->NAccessPolicy]);
        }

      if (J->SpecDistPolicy != 0)
        {
        MStringAppendF(Buffer,"TaskDistributionPolicy: %s\n",
            MTaskDistributionPolicy[J->SpecDistPolicy]);
        }

      if (J->ResFailPolicy != marfNONE)
        {
        MStringAppendF(Buffer,"ResourceFailurePolicy:  %s\n",
            MARFPolicy[J->ResFailPolicy]);
        }

      if (RQ->MinProcSpeed > 0)
        {
        MStringAppendF(Buffer,"MinProcSpeed: %d ",
            RQ->MinProcSpeed);
        }

      if (RQ->TasksPerNode > 0)
        {
        if (bmisset(&J->IFlags,mjifExactTPN))
          {
          MStringAppendF(Buffer,"TasksPerNode: %s%d  ",
              "==",
              RQ->TasksPerNode);
          }
        else if (bmisset(&J->IFlags,mjifMaxTPN))
          {
          MStringAppendF(Buffer,"TasksPerNode: %s%d  ",
              "<=",
              RQ->TasksPerNode);
          }
        else
          {
          MStringAppendF(Buffer,"TasksPerNode: %d  ",
              RQ->TasksPerNode);
          }
        }

      if ((RQ->OptReq != NULL) && (RQ->OptReq->ReqImage != NULL))
        {
        MStringAppendF(Buffer,"Req. Image: %s  ",
            RQ->OptReq->ReqImage);
        }

      if ((RQ->OptReq != NULL) && (RQ->OptReq->GMReq != NULL))
        {
        mln_t *gmptr;
        mgmreq_t *G;

        MStringAppendF(Buffer,"GMetric: ");

        for (gmptr = RQ->OptReq->GMReq;gmptr != NULL;gmptr = gmptr->Next)
          {
          G = (mgmreq_t *)gmptr->Ptr;

          if ((G == NULL) || (G->GMIndex <= 0))
            continue;

          MStringAppendF(Buffer,"%s%s%s  ",
              MGMetric.Name[G->GMIndex],
              MComp[G->GMComp],
              (G->GMVal[0] != '\0') ? G->GMVal : "ANY");
          }  /* END for (rindex) */
        }
      }      /* END if (VerboseLevel >= 1) */

    if ((RQ->NodeCount > 0) &&
        (((RQ->NodeCount * RQ->TasksPerNode) != RQ->TaskCount) ||
         (VerboseLevel >= 1)))
      {
      MStringAppendF(Buffer,"NodeCount:  %d\n",
          (MSched.BluegeneRM == TRUE) ? RQ->TaskCount / MSched.BGNodeCPUCnt : RQ->NodeCount);         
      }

    if (bmisset(&J->IFlags,mjifHostList) && 
        (RQ->Index == J->HLIndex))
      {
      if (MNLIsEmpty(&J->ReqHList))
        {
        MStringAppend(Buffer,"  WARNING:  empty hostlist specified\n");
        }
      else
        {
        MStringAppend(Buffer,"Required HostList: ");

        if (J->ReqHLMode == mhlmSubset)
          MStringAppend(Buffer," (SUBSET) ");
        else if (J->ReqHLMode == mhlmSuperset)
          MStringAppend(Buffer," (SUPERSET) ");

        if (J->HLIndex != 0)
          {
          MStringAppendF(Buffer," (Req %d) ",
              J->HLIndex);
          }

        MStringAppend(Buffer,"\n");

        for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N) == SUCCESS;nindex++)
          {
          MStringAppendF(Buffer,"[%.20s:%d]",
              N->Name,
              MNLGetTCAtIndex(&J->ReqHList,nindex));

          if ((nindex % MMAX_NODE_PER_LINE) == (MMAX_NODE_PER_LINE - 1))
            MStringAppend(Buffer,"\n  ");
          }  /* END for (nindex) */

        MStringAppend(Buffer,"\n");
        }
      }      /* END if (bmisset(&J->IFlags,mjifHostList) && ...) */

    if ((MJOBISALLOC(J) == TRUE) &&
        !bmisset(&J->SpecFlags,mjfNoResources))
      {
      int nodeperrow;

      MNLGetNodeAtIndex(&RQ->NodeList,0,&N);

      if ((RQ->PtIndex < 1) && (N != MSched.GN))
        {
        /* NOTE:  partition 0 is allowed if 'coalloc' QOS flag set */

        if ((RQ->PtIndex < 0) || 
            ((J->Credential.Q != NULL) && !bmisset(&J->Credential.Q->Flags,mqfCoAlloc)))
          {
          MStringAppendF(Buffer,"ALERT:    partition %d is invalid\n",
              RQ->PtIndex);
          }
        }

      MStringAppend(Buffer,"\nAllocated Nodes:\n");

      if ((N != NULL) && (strlen(N->Name) <= 6))
        nodeperrow = 8;
      else
        nodeperrow = 4;

      if (J->Request.TC > 64)
        {
        mstring_t tmpRange(MMAX_BUFFER);

        if (MUNLToRangeString(
              &RQ->NodeList,
              NULL,
              0,
              TRUE,
              TRUE,
              &tmpRange) == SUCCESS)
          {
          MStringAppendF(Buffer,"%s\n",
              tmpRange.c_str());
          }
        }
      else
        {
        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          MStringAppendF(Buffer,"[%.20s:%d]",
              N->Name,
              MNLGetTCAtIndex(&RQ->NodeList,nindex));

          if ((nindex % nodeperrow) == (nodeperrow - 1))
            MStringAppend(Buffer,"\n");
          }    /* END for (nindex)               */
        }

      MStringAppend(Buffer,"\n");

      /* evaluate nodes */

      TaskCount = 0;

      for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        TaskCount += MNLGetTCAtIndex(&RQ->NodeList,nindex);

        if (N->State == mnsDown)
          {
          MStringAppendF(Buffer,"  WARNING:  allocated node %s is in state %s\n",
              N->Name,
              MNodeState[N->State]);
          }
        else if ((N->State == mnsIdle) && 
            (J->State != mjsSuspended) &&
            (MSched.Time - J->StartTime > 120) &&
            (RQ->DRes.Procs != 0) && 
            ((N->RM == NULL) || (bmisclear(&N->RM->RTypes)) || bmisset(&N->RM->RTypes,mrmrtCompute)))
          {
          MStringAppendF(Buffer,"  WARNING:  allocated node %s is in state %s\n",
              N->Name,
              MNodeState[N->State]);
          }
        }    /* END for (nindex)  */

      if ((TaskCount != RQ->TaskCount) && (TaskCount != RQ->DRes.Procs))
        {
        MStringAppendF(Buffer,"  WARNING:  allocated tasks do not match requested tasks (%d != %d)\n",
            TaskCount,
            RQ->TaskCount);
        }
      }      /* END if (MJOBISALLOC(J) == TRUE) */
    else if ((MJOBISIDLE(J) == TRUE) && (J->Rsv != NULL) && (!MNLIsEmpty(&J->Rsv->NL)))
      {
      mrsv_t *R;

      R = ((RQ->R != NULL) && (!MNLIsEmpty(&RQ->R->NL))) ? RQ->R : J->Rsv;

      /* display reservation */

      MULToTString(R->StartTime - MSched.Time,TString);
      strcpy(StartTime,TString);
      MULToTString(R->EndTime - MSched.Time,TString);
      strcpy(EndTime,TString);
      MULToTString(R->EndTime - R->StartTime,TString);
      strcpy(Duration,TString);

      MStringAppendF(Buffer,"\nReserved Nodes:  (%s -> %s  Duration: %s)\n",
          StartTime,
          EndTime,
          Duration);

      if (J->Request.TC > 64)
        {
        mstring_t tmpRange(MMAX_BUFFER);

        if (MUNLToRangeString(
              &R->NL,
              NULL,
              0,
              TRUE,
              TRUE,
              &tmpRange) == SUCCESS)
          {
          MStringAppendF(Buffer,"%s\n",
              tmpRange.c_str());
          }
        }
      else
        {
        for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,&N) == SUCCESS;nindex++)
          {
          MStringAppendF(Buffer,"[%.24s:%d]",
              N->Name,
              MNLGetTCAtIndex(&R->NL,nindex));

          if ((nindex % MMAX_NODE_PER_LINE) == (MMAX_NODE_PER_LINE - 1))
            MStringAppend(Buffer,"\n");
          }    /* END for (nindex) */
        }

      MStringAppend(Buffer,"\n");
      }
    else if ((MJOBISCOMPLETE(J) == TRUE) && 
        (!MNLIsEmpty(&RQ->NodeList)))
      {
      MStringAppend(Buffer,"\nAllocated Nodes:\n");

      for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        MStringAppendF(Buffer,"[%.20s:%d]",
            N->Name,
            MNLGetTCAtIndex(&RQ->NodeList,nindex));

        if ((nindex % MMAX_NODE_PER_LINE) == (MMAX_NODE_PER_LINE - 1))
          MStringAppend(Buffer,"\n");
        }    /* END for (nindex) */

      MStringAppend(Buffer,"\n");
      }
    }        /* END for (rqindex) */

  /* display nodeset allocation */

  if (VerboseLevel >= 2)
    {
    RQ = J->Req[0];

    if (RQ->SetType == mrstFeature)
      {
      if (RQ->SetIndex > 0)
        MStringAppendF(Buffer,"Applied Nodeset: %s\n",MAList[meNFeature][RQ->SetIndex]);
      }
    }

  MStringAppend(Buffer,"\n");

  /* END of per req display */

  /* node selection criteria */
  if (J->NodePriority != NULL)
    {
    mstring_t tmp(MMAX_LINE);

    MPrioFToMString(J->NodePriority,&tmp);
    MStringAppendF(Buffer,"Node Selection Priority Function: %s\n",tmp.c_str());
    }

  MStringAppend(Buffer,"\n");

  /* job peer attributes */

  if (J->JGroup != NULL)
    {
    if (bmisset(&J->Flags, mjfArrayJob))
      {
      /* Job Group */
      MStringAppendF(Buffer,"Job Group:        %s\n",
          J->JGroup->Name);

      /* Parent Array ID*/
      MStringAppendF(Buffer,"Parent Array ID:  %s\n",
          J->JGroup->Name);

      /* get the array index of the job and print it out */

      /* find master job */
      if ((MJobFind(J->JGroup->Name,&JA,mjsmExtended) == SUCCESS) && 
          (JA->Array != NULL))
        { 
        /* display the size of the array */
        MStringAppendF(Buffer,"Array Range:      %d\n",
            JA->Array->Count);
        }
      }
    else
      {
      MStringAppendF(Buffer,"Job Group:  %s\n",
          J->JGroup->Name);
      }
    }

  if (bmisset(&J->Flags,mjfArrayMaster))
    {
    if (J->MessageBuffer != NULL)
      {
      /* display data */

      char *tmpPtr;

      if (VerboseLevel >= 2)
        {
        /* display header */

        MStringAppendF(Buffer,"\nJob Messages -----\n");

        /* display column headings */

        tmpPtr = MMBPrintMessages(
            NULL,
            mfmHuman,
            TRUE,
            -1,
            NULL,
            0);

        MStringAppendF(Buffer,"%s",
            tmpPtr);
        }

      /* display job messages */

      tmpPtr = MMBPrintMessages(
          J->MessageBuffer,
          mfmHuman,
          (VerboseLevel >= 2) ? TRUE : FALSE,
          -1,
          NULL,
          0);

      MStringAppendF(Buffer,"%s",
          tmpPtr);
      }

    return(SUCCESS);
    }

  if (J->SystemID != NULL)
    {
    MStringAppendF(Buffer,"SystemID:   %s\n",
        J->SystemID);
    }

  if (J->SystemJID != NULL)
    {
    MStringAppendF(Buffer,"SystemJID:  %s\n",
        J->SystemJID);
    }

  if (!bmisclear(&J->NotifyBM))
    {
    char tmpLine[MMAX_LINE];

    if (J->NotifyAddress != NULL)
      {
      MStringAppendF(Buffer,"Notification Events: %s  Notification Address: %s\n",
        bmtostring(&J->NotifyBM,MNotifyType,tmpLine),
        J->NotifyAddress);
      }
    else
      {
      MStringAppendF(Buffer,"Notification Events: %s\n",
        bmtostring(&J->NotifyBM,MNotifyType,tmpLine));
      }
    }

  if ((J->Grid != NULL) && (J->Grid->HopCount > 0))
    {
    MStringAppendF(Buffer,"Hop Count:  %d\n",
        J->Grid->HopCount);
    }

  if (J->MigrateBlockReason != mjneNONE)
    {
    MStringAppendF(Buffer,"MigrateBlockReason:  %s - %s\n",
        MJobBlockReason[J->MigrateBlockReason],
        (J->MigrateBlockMsg != NULL) ? J->MigrateBlockMsg : "N/A");
    }

  if ((J->ReqVMList != NULL) && (J->ReqVMList[0] != NULL))
    {
    MStringAppendF(Buffer,"Req VMList: %s\n",
        J->ReqVMList[0]->VMID);
    }

  /* show taskmap (don't show on bluegene systems) */

  if ((VerboseLevel >= 1) && (MSched.BluegeneRM == FALSE))
    {
    if ((J->TaskMapSize > 0) && (J->TaskMap != NULL) && (J->TaskMap[0] != -1))
      {
      MStringAppend(Buffer,"Task Distribution: ");

      for (tindex = 0;J->TaskMap[tindex] != -1;tindex++)
        {
        if (tindex > 10)
          {
          MStringAppendF(Buffer,",...");

          break;
          }

        if (tindex > 0)
          {
          MStringAppendF(Buffer,",%.20s",
              MNode[J->TaskMap[tindex]]->Name);
          }
        else
          {
          MStringAppendF(Buffer,"%.20s",
              MNode[J->TaskMap[tindex]]->Name);
          }
        }  /* END for (tindex) */

      MStringAppend(Buffer,"\n");
      }    /* END if (J->TaskMap != NULL) */

    if (J->CompletedVMList != NULL)
      {
      MStringAppendF(Buffer,"VM Distribution:   %s\n",J->CompletedVMList);
      }
    }    /* END if (VerboseLevel >= 1) */

  MStringAppend(Buffer,"\n");

  /* display job executable information */

  if ((J->Env.Shell != NULL) && (VerboseLevel >= 1))
    {
    MStringAppendF(Buffer,"Shell:          %s\n",
        J->Env.Shell);
    }

  if (J->Env.IWD != NULL)
    {
    MStringAppendF(Buffer,"IWD:            %s\n",
        (J->Env.IWD != NULL) ? J->Env.IWD : "---");
    }

  if (J->Env.SubmitDir != NULL)
    {
    MStringAppendF(Buffer,"SubmitDir:      %s\n",
        J->Env.SubmitDir);
    }

  if ((VerboseLevel >= 1) && 
      (J->System == NULL) &&
      (!bmisset(&J->IFlags,mjifIsTemplate) || (J->Env.UMask != 0)))
    {
    MStringAppendF(Buffer,"UMask:          %04o \n",
        J->Env.UMask);
    }

  if ((J->Env.Cmd != NULL) && (J->System == NULL))
    {
    MStringAppendF(Buffer,"Executable:     %s",
        J->Env.Cmd);

    MStringAppend(Buffer,"\n");

    if (J->Env.Args != NULL)
      {
      MStringAppendF(Buffer,"Arguments:    %s",
          J->Env.Args);
      }

    MStringAppend(Buffer,"\n");
    }

  if ((J->PrologueScript != NULL) || (J->EpilogueScript != NULL))  
    {
    if (J->PrologueScript != NULL) 
      MStringAppendF(Buffer,"Prologue Script: %s\n",
          J->PrologueScript);

    if (J->EpilogueScript != NULL) 
      MStringAppendF(Buffer,"Epilogue Script: %s\n",
          J->EpilogueScript);

    MStringAppend(Buffer,"\n");
    }

  if (VerboseLevel >= 1)
    {
    if ((J->Env.Output != NULL) || (J->Env.RMOutput != NULL))
      {
      if (J->Env.Output == NULL)
        {
        MStringAppendF(Buffer,"OutputFile:     %s\n",
            J->Env.RMOutput);
        }
      else
        {
        MStringAppendF(Buffer,"OutputFile:     %s (%s)\n",
            J->Env.Output,
            (J->Env.RMOutput != NULL) ? J->Env.RMOutput : "-");
        }
      }

    if (J->Env.Input != NULL)
      {
      MStringAppendF(Buffer,"InputFile:      %s\n",
          J->Env.Input);
      }

    if ((J->Env.Error != NULL) || (J->Env.RMError != NULL))
      {
      if (J->Env.Error == NULL)
        {
        MStringAppendF(Buffer,"ErrorFile:      %s\n",
            J->Env.RMError);
        }
      else
        {
        MStringAppendF(Buffer,"ErrorFile:      %s (%s)\n",
            J->Env.Error,
            (J->Env.RMError != NULL) ? J->Env.RMError : "-");
        }
      }

    if (bmisset(&J->IFlags,mjifMergeJobOutput))
      {
      MStringAppendF(Buffer,"NOTE:  stdout/stderr will be merged\n");
      }

    if (VerboseLevel >= 2)
      {
      if (J->Env.BaseEnv != NULL)
        {
        MStringAppendF(Buffer,"EnvVariables:   %s\n",
            J->Env.BaseEnv);
        }
      }
    }    /* END if (VerboseLevel >= 1)) */

  if (J->StartCount != 0)
    {
    MStringAppendF(Buffer,"StartCount:     %d\n",
        J->StartCount);
    }

  if (J->BypassCount != 0)
    {
    MStringAppendF(Buffer,"BypassCount:    %d\n",
        J->BypassCount);
    }

  if (J->ParentVCs != NULL)
    {
    mstring_t tmp(MMAX_LINE);

    MJobAToMString(J,mjaParentVCs,&tmp);

    MStringAppendF(Buffer,"Parent VCs:     %s\n",
        tmp.c_str());
    }

  if (bmisset(&J->IFlags,mjifIsTemplate))
    {
    mstring_t tmp(MMAX_LINE);

    MJobAToMString(J,mjaRCL,&tmp);

    if (!MUStrIsEmpty(tmp.c_str()))
      {
      MStringAppendF(Buffer,"RCL:      %s\n",
          tmp.c_str());
      }
    }

  if (MJOBISCOMPLETE(J) == TRUE)
    {
    MStringAppendF(Buffer,"Execution Partition:  %s\n",
        MPar[J->Req[0]->PtIndex].Name);
    }
  else
    {
    char ParLine[MMAX_LINE];

    if (VerboseLevel >= 1)
      {
      if ((!bmisclear(&J->SpecPAL)) && (!bmissetall(&J->SpecPAL,MMAX_PAR)))
        {
        MStringAppendF(Buffer,"User Specified Partition List:   %s\n",
            MPALToString(&J->SpecPAL,NULL,ParLine));
        }

      if ((!bmisclear(&J->SysPAL)) && (!bmissetall(&J->SysPAL,MMAX_PAR)))
        {
        MStringAppendF(Buffer,"System Available Partition List: %s\n",
            MPALToString(&J->SysPAL,NULL,ParLine));
        }
      else if (bmisclear(&J->SysPAL))
        {
        if (!bmisset(&J->IFlags,mjifIsTemplate))
          MStringAppendF(Buffer,"  WARNING:  job has empty sys-avail partition list\n");
        }
      }

    if (((!bmisclear(&J->PAL)) && (!bmissetall(&J->PAL,MMAX_PAR))) ||
        (VerboseLevel >= 1))
      {
      if (bmisclear(&J->PAL))
        {
        if (!bmisset(&J->IFlags,mjifIsTemplate))
          MStringAppendF(Buffer,"  WARNING:  job has empty partition list\n");
        }
      else
        {
        MStringAppendF(Buffer,"Partition List: %s\n",
            (bmissetall(&J->PAL,MMAX_PAR)) ?
            ALL : MPALToString(&J->PAL,NULL,ParLine));
        }
      }
    else if (bmisclear(&J->PAL))
      {
      if (!bmisset(&J->IFlags,mjifIsTemplate))
        MStringAppendF(Buffer,"  WARNING:  job has empty partition list\n");
      }
    }    /* END else (MJOBISCOMPLETE(J) == TRUE) */

  if (VerboseLevel >= 1)
    {
    if ((MRM[1].Type != mrmtNONE) && (J->SubmitRM != NULL))
      {
      int rqindex;

      MStringAppendF(Buffer,"SrcRM:          %s",
          J->SubmitRM->Name);

      if (J->DestinationRM != NULL)
        {
        MStringAppendF(Buffer,"  DstRM: %s",
            J->DestinationRM->Name);
        }

      if (J->DRMJID != NULL)
        {
        MStringAppendF(Buffer,"  DstRMJID: %s",
            J->DRMJID);
        }

      MStringAppendF(Buffer,"\n");

      if (!bmisset(&J->Flags,mjfCoAlloc))
        {
        mreq_t *RQ;
        mrm_t  *RQRM;

        for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
          {
          RQ = J->Req[rqindex];

          if (RQ == NULL)
            break;

          if (RQ->RMIndex >= 0)
            {
            RQRM = &MRM[RQ->RMIndex];

            if ((!bmisclear(&RQRM->RTypes)) && !bmisset(&RQRM->RTypes,mrmrtCompute))
              continue;
            }
          else
            {
            /* NOTE:  req rm not specified */

            continue;
            }

          if ((J->DestinationRM != NULL) && (RQRM != J->DestinationRM))
            {
            MStringAppendF(Buffer,"  WARNING:  req %d RM (%s) does not match job destination RM\n",
                rqindex,
                RQRM->Name);
            }
          else if ((J->DestinationRM == NULL) && (RQRM->Index != J->SubmitRM->Index))
            {
            MStringAppendF(Buffer,"  WARNING:  req %d RM (%s) does not match job source RM\n",
                rqindex,
                RQRM->Name);
            }
          }    /* END for (rqindex) */
        }      /* END if (!bmisset(&J->Flags,mjfCoAlloc)) */
      }        /* END if ((MRM[1].Type != mrmtNONE) && ...) */

    if (J->AttrString != NULL)
      {
      MStringAppendF(Buffer,"Submit Args:    %s\n",
          J->AttrString);
      }

    if (!bmisclear(&J->JobRejectPolicyBM))
      {
      char tmpPolicy[MMAX_LINE];
      bmtostring(&J->JobRejectPolicyBM,MJobRejectPolicy,tmpPolicy);
      MStringAppendF(Buffer,"Job Reject Policy:    %s\n",tmpPolicy);
      }
    }    /* END if (VerboseLevel >= 1) */

  if ((J->VMUsagePolicy != mvmupNONE) && (VerboseLevel >= 1))
    {
    /* NOTE:  mjifDestroyDynamicVM attr not set until job is started */

    if ((J->VMUsagePolicy == mvmupVMCreate) && 
        MJOBISACTIVE(J) &&
        !bmisset(&J->IFlags,mjifDestroyDynamicVM))
      {
      MStringAppendF(Buffer,"VMUsagePolicy:  %s (vm is persistent)\n",
          MVMUsagePolicy[J->VMUsagePolicy]);
      }
    else
      {
      MStringAppendF(Buffer,"VMUsagePolicy:  %s\n",
          MVMUsagePolicy[J->VMUsagePolicy]);
      }
    }

  if (!bmisclear(&J->Flags))
    {
    mbool_t IsFirstFlag = TRUE;

    MStringAppend(Buffer,"Flags:          ");

    if (bmisset(&J->IFlags,mjifNoImplicitCancel))
      {
      MStringAppendF(Buffer,"%s",
          "NOIMPLICITCANCEL");

      IsFirstFlag = FALSE;
      }

    for (index = 0;MJobFlags[index] != NULL;index++)
      {
      if (bmisset(&J->Flags,index))
        { 
        if (IsFirstFlag == FALSE)
          {
          MStringAppendF(Buffer,",%s",
              MJobFlags[index]);
          }
        else
          {
          MStringAppendF(Buffer,"%s",
              MJobFlags[index]);

          IsFirstFlag = FALSE;
          }

        if ((index == mjfAdvRsv) && (J->ReqRID != NULL))
          {
          MStringAppend(Buffer,":");
          if (bmisset(&J->IFlags,mjifNotAdvres))
            MStringAppend(Buffer,"!");
          MStringAppend(Buffer,J->ReqRID);
          }
        }
      }    /* END for (index) */

    MStringAppend(Buffer,"\n");

    if (J->ReqRID != NULL)
      {
      if ((MRsvFind(J->ReqRID,NULL,mraNONE) == FAILURE) &&
          (MRsvFind(J->ReqRID,NULL,mraRsvGroup) == FAILURE))
        {
        MStringAppendF(Buffer,"  WARNING:  required reservation '%s' not found\n",
            J->ReqRID);
        }
      }

    if (!bmisclear(&J->AttrBM))
      {
      mbool_t First = TRUE;

      MStringAppend(Buffer,"Attr:           ");

      for (index = 0;index < MMAX_ATTR;index++)
        {
        if (MAList[meJFeature][index][0] == '\0')
          break;

        if (bmisset(&J->AttrBM,index))
          {
          MStringAppendF(Buffer,"%s%s",
              (First == TRUE) ? "" : ",",
              MAList[meJFeature][index]);

          First = FALSE;
          }
        }  /* END for (index) */

      MStringAppend(Buffer,"\n");
      }    /* if (!bmisclear(&J->AttrBM)) */
    }     /* END if (!bmisclear(&J->Flags,mjfLAST)) */

  if (bmisset(&J->IFlags,mjifIsTemplate))
    {
    /* display Job Template IFlags */

    mbool_t FoundIFlag = FALSE;
    char tmpLine[MMAX_LINE];

    char *tBPtr;
    int   tBSpace;

    MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

    if (bmisset(&J->IFlags,mjifIsHidden))
      {
      MStringAppendF(Buffer,"HIDDEN");

      FoundIFlag = TRUE;
      }

    if (bmisset(&J->IFlags,mjifIsHWJob))
      {
      MStringAppendF(Buffer,"%sHWJOB",
          (FoundIFlag == TRUE) ? "," : "");

      FoundIFlag = TRUE;
      }

    if (bmisset(&J->IFlags,mjifGlobalRsvAccess))
      {
      MStringAppendF(Buffer,"%sGLOBALRSVACCESS",
          (FoundIFlag == TRUE) ? "," : "");

      FoundIFlag = TRUE;
      }

    if (bmisset(&J->IFlags,mjifSyncJobID))
      {
      MStringAppendF(Buffer,"%sSYNCJOBID",
          (FoundIFlag == TRUE) ? "," : "");

      FoundIFlag = TRUE;
      }

    if (FoundIFlag == TRUE)
      {
      MStringAppendF(Buffer,"IFlags:         %s\n",
          tmpLine);
      }

    if (J->TemplateExtensions != NULL)
      {
      MStringAppendF(Buffer,"Selectable:     %s\n",
          (bmisset(&J->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable)) ? "TRUE" : "FALSE");
      }
    else
      {
      MStringAppendF(Buffer,"Selectable:     %s\n",
          "FALSE");
      }

    } /* END if (bmisset(&J->IFlags,mjifIsTemplate)) */

  /* evaluate QOS access */

  if ((QOSList != NULL) && (QOSList[0] != '\0') && (strcmp(QOSList,NONE)))
    {
    char *ptr;
    char *TokPtr;

    char *QName;

    mqos_t *Q;

    ptr = MUStrTok(QOSList,",",&TokPtr);

    while (ptr != NULL)
      {
      QName = ptr;

      MQOSFind(QName,&Q);

      ptr = MUStrTok(NULL,",",&TokPtr);

      if (MQOSGetAccess(J,Q,NULL,NULL,NULL) == SUCCESS)
        {
        MStringAppendF(Buffer,"\nNOTE:  job granted access to QOS '%s'\n",
            QName);
        }
      else
        {
        MStringAppendF(Buffer,"\nNOTE:  job denied access to QOS '%s'\n",
            QName);
        }
      }
    }  /* END if ((QOSList[0] != '\0') && (strcmp(QOSList,NONE))) */

  /* evaluate reservation access */

  if ((RsvList != NULL) && (strcmp(RsvList,NONE)) && (RsvList[0] != '\0'))
    {
    char *ptr;
    char *TokPtr;

    char *RName;

    mrsv_t *R;

    ptr = MUStrTok(RsvList,",",&TokPtr);

    while (ptr != NULL)
      {
      char Msg[MMAX_LINE];

      char Affinity;

      RName = ptr;

      MRsvFind(RName,&R,mraNONE);

      ptr = MUStrTok(NULL,",",&TokPtr);

      if (MRsvCheckJAccess(R,J,J->WCLimit,NULL,FALSE,NULL,NULL,&Affinity,Msg) == SUCCESS)
        {
        MStringAppendF(Buffer,"\nNOTE:  job granted access to reservation '%s' w/affinity '%s' (%s)\n",
            RName,
            MAffinityType[(int)Affinity],
            Msg);
        }
      else
        {
        MStringAppendF(Buffer,"\nNOTE:  job denied access to reservation '%s' (%s)\n",
            RName,
            Msg);
        }
      }
    }  /* END if (strcmp(RsvList,NONE) && (RsvList[0] != '\0')) */

  /* show job variables */

  if (J->Variables.Table != NULL)
    {
    mstring_t VarBuf(MMAX_LINE);

    MUHTToMString(&J->Variables,&VarBuf);

    MStringAppendF(Buffer,"Variables:      %s\n",VarBuf.c_str());
    }

  /* display priority info */

  if (J->SystemPrio != 0)
    {
    if (J->SystemPrio > MMAX_PRIO_VAL)
      {
      sprintf(Line,"  SystemPriority:  %s%ld\n",
          (J->SystemPrio > (MMAX_PRIO_VAL << 1)) ? "+" : "",
          (J->SystemPrio - (MMAX_PRIO_VAL << 1)));
      }
    else
      {
      sprintf(Line,"  SystemPriority:  %ld\n",
          J->SystemPrio);
      }
    }
  else
    {
    Line[0] = '\0';
    }

  if (bmisset(&J->IFlags,mjifIsTemplate) && (J->SystemPrio != 0))
    {
    if (J->SystemPrio > MMAX_PRIO_VAL)
      {
      MStringAppendF(Buffer,"Priority:       %s%ld\n",
          (J->SystemPrio > (MMAX_PRIO_VAL << 1)) ? "+" : "",
          (J->SystemPrio - (MMAX_PRIO_VAL << 1)));
      }
    else
      {
      MStringAppendF(Buffer,"Priority:       %ld\n",
          J->SystemPrio);
      }
    }

  if (!bmisset(&J->IFlags,mjifIsTemplate) || (J->CurrentStartPriority != 0))
    {
    MStringAppendF(Buffer,"StartPriority:  %ld%s\n",
        J->CurrentStartPriority,
        Line);

    if (VerboseLevel >= 2)
      {
      int   pindex;

      /* per partition priorities */

      if (MSched.PerPartitionScheduling == TRUE)
        {  
        MStringAppendF(Buffer,"Per Partition Priority:\n");

        /* create headers */

        MStringAppendF(Buffer,"%-10s %10s\n","Partition","Priority");

        /* for valid partition pindex in J->PAL */

        for (pindex = 1; pindex < MMAX_PAR; pindex++)
          {
          if (bmisset(&J->PAL,pindex) == FAILURE)
            continue;

          MStringAppendF(Buffer,"%-10s %10ld\n",MPar[pindex].Name,J->PStartPriority[pindex]);
          }
        }

      /* initialize priority statistics and build header */

      MStringAppend(Buffer,"Priority Analysis:\n");

      MJobCalcStartPriority(
          NULL,
          &MPar[0],
          NULL,
          mpdHeader,
          NULL,
          Buffer,
          mfmHuman,
          FALSE);

      MJobCalcStartPriority(
          J,
          &MPar[0],
          NULL,    /* O */
          mpdJob,
          NULL,
          Buffer,
          mfmHuman,
          FALSE);
      }
    }

  if (bmisset(&J->IFlags,mjifFBAcctInUse))
    {
    MStringAppendF(Buffer,"NOTE:  Using Fallback Account\n");
    }

  if (bmisset(&J->IFlags,mjifFBQOSInUse))
    {
    MStringAppendF(Buffer,"NOTE:  Using Fallback QOS\n");
    }

  if (J->System != NULL)
    {
    MJobShowSystemAttrs(J,VerboseLevel,Buffer);
    }    /* END if (J->System != NULL) */

  MJobGetPE(J,&MPar[0],&PE);

  if (!bmisset(&J->IFlags,mjifIsTemplate))
    {
    if ((VerboseLevel >= 1) || ((int)PE != J->Request.TC))
      {
      MStringAppendF(Buffer,"PE:             %.2f\n",
          PE);
      }
    }

  /* display reservation */

  if (J->Rsv != NULL)
    {
    MULToTString(J->Rsv->StartTime - MSched.Time,TString);
    strcpy(StartTime,TString);
    MULToTString(J->Rsv->EndTime - MSched.Time,TString);
    strcpy(EndTime,TString);
    MULToTString(J->Rsv->EndTime - J->Rsv->StartTime,TString);
    strcpy(Duration,TString);

    MStringAppendF(Buffer,"Reservation '%s' (%s -> %s  Duration: %s)\n",
        J->Rsv->Name,
        StartTime,
        EndTime,
        Duration);

    if ((MJOBISACTIVE(J) == TRUE) &&
        (J->StartTime > 0) &&
        ((J->StartTime < J->Rsv->StartTime) ||
         (J->StartTime > J->Rsv->StartTime + 60)))
      {
      MStringAppendF(Buffer,"  WARNING:  reservation does not match job start time (%ld != %ld)\n",
          J->Rsv->StartTime,
          J->StartTime);
      }
    }
  else if ((MJOBISACTIVE(J) == TRUE) && 
      !bmisset(&J->SpecFlags,mjfNoResources) &&
      !bmisset(&J->SpecFlags,mjfRsvMap))
    {
    MStringAppendF(Buffer,"  WARNING:  active job has no reservation\n");
    }  /* END else if (MJOBISACTIVE(J) == TRUE) */

  /* check update time */

  if (!bmisset(&J->IFlags,mjifIsTemplate) &&
      ((MSched.Time - J->ATime) > MAX(300,2 * (mulong)MSched.MaxRMPollInterval)) && 
      (MJOBISCOMPLETE(J) == FALSE))
    {
    MULToTString(MSched.Time - J->ATime,TString);

    MStringAppendF(Buffer,"  WARNING:  job has not been updated in %s\n",
        TString);
    }

  MNodeFind(NodeName,&N);

  MJobDiagnoseState(
      J,
      N,
      PLevel,
      Flags,
      ((IgnBM == NULL) || (bmisclear(IgnBM))) ? NULL : IgnBM,
      Buffer);

  if ((MJOBISACTIVE(J) == FALSE) && (MJobGetBlockMsg(J,NULL) != NULL))
    {
    if ((MJobGetBlockReason(J,NULL) == mjneEState) && (J->EState == mjsDeferred))
      {
      /* NOTE:  hold information provides all needed details for holds */

      /* NO-OP */
      }
    else 
      {
      MStringAppendF(Buffer,"BLOCK MSG: %s (recorded at last scheduling iteration)\n",
          MJobGetBlockMsg(J,NULL));
      }
    }

  if ((MJOBISACTIVE(J) == FALSE) &&
      (MJobGetBlockMsg(J,NULL) == NULL) &&
      (VerboseLevel >= 1) &&
      (MPar[0].BFChunkSize > 0) &&
      (MPar[0].BFChunkDuration > 0) &&
      (J->TotalProcCount < MPar[0].BFChunkSize) &&
      ((MPar[0].BFChunkBlockTime > 0) &&
       ((mulong)MPar[0].BFChunkBlockTime > MSched.Time)))
    {
    MStringAppendF(Buffer,"\n  WARNING: job may be blocked by backfill chunking (%d < %d )\n",
        J->TotalProcCount,
        MPar[0].BFChunkSize);
    }

  if (VerboseLevel >= 1)
    {
    if (J->MessageBuffer != NULL)
      {
      /* display data */

      char *tmpPtr;

      if (VerboseLevel >= 2)
        {
        /* display header */

        MStringAppendF(Buffer,"\nJob Messages -----\n");

        /* display column headings */

        tmpPtr = MMBPrintMessages(
            NULL,
            mfmHuman,
            TRUE,
            -1,
            NULL,
            0);

        MStringAppendF(Buffer,"%s",
            tmpPtr);
        }

      /* display job messages */

      tmpPtr = MMBPrintMessages(
          J->MessageBuffer,
          mfmHuman,
          (VerboseLevel >= 2) ? TRUE : FALSE,
          -1,
          NULL,
          0);

      MStringAppendF(Buffer,"%s",
          tmpPtr);
      }

    if (J->PrologTList != NULL)
      {
      mstring_t String(MMAX_BUFFER);

      MStringAppendF(Buffer,"\nPrologue Triggers\n-----------------\n");

      MTrigDiagnose(
          NULL,
          NULL,
          NULL,
          &String,
          mfmHuman,
          Flags,
          J->PrologTList,
          mAdd);

      MStringAppendF(Buffer,"%s",String.c_str());
      }

    if (J->EpilogTList != NULL)
      {
      mstring_t String(MMAX_BUFFER);

      MStringAppendF(Buffer,"\nEpilogue Triggers\n-----------------\n");

      MTrigDiagnose(
          NULL,
          NULL,
          NULL,
          &String,
          mfmHuman,
          Flags,
          J->EpilogTList,
          mAdd);

      MStringAppendF(Buffer,"%s",String.c_str());
      }
    }  /* END if (VerboseLevel >= 1) */

  if ((VerboseLevel >= 2) && J->RMSubmitString != NULL)
    {
    MStringAppendF(Buffer,"\nJob Submission\n-----------------\n%s\n", J->RMSubmitString);
    }

  return(SUCCESS);
  } /* END MUICheckJob() */
/* END MUICheckJob.c */
