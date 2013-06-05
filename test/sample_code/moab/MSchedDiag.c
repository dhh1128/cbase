/* HEADER */

      
/**
 * @file MSchedDiag.c
 *
 * Contains: MSchedDiag
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MMongo.h"



int MSchedLicensedFeaturesToString(

  char *Line)

  {
  if (Line == NULL)
    return(FAILURE);

  Line[0] = '\0';

  if (MOLDISSET(MSched.LicensedFeatures,mlfGrid))
    {
    strcat(Line,MLicenseFeatures[mlfGrid]);
    }

  if (MOLDISSET(MSched.LicensedFeatures,mlfGreen))
    {
    if (Line[0] != '\0')
      strcat(Line,",");

    strcat(Line,MLicenseFeatures[mlfGreen]);
    }

  if (MOLDISSET(MSched.LicensedFeatures,mlfProvision))
    {
    if (Line[0] != '\0')
      strcat(Line,",");

    strcat(Line,MLicenseFeatures[mlfProvision]);
    }

  if (MOLDISSET(MSched.LicensedFeatures,mlfVM))
    {
    if (Line[0] != '\0')
      strcat(Line,",");

    strcat(Line,MLicenseFeatures[mlfVM]);
    }

  if (MOLDISSET(MSched.LicensedFeatures,mlfMSM))
    {
    if (Line[0] != '\0')
      strcat(Line,",");

    strcat(Line,MLicenseFeatures[mlfMSM]);
    }

  if (MOLDISSET(MSched.LicensedFeatures,mlfVPC))
    {
    if (Line[0] != '\0')
      strcat(Line,",");

    strcat(Line,MLicenseFeatures[mlfVPC]);
    }

#if 0
  if (MOLDISSET(MSched.LicensedFeatures,mlfNoHPC))
    {
    if (Line[0] != '\0')
      strcat(Line,",");

    strcat(Line,MLicenseFeatures[mlfNoHPC]);
    }
#endif

  if (MOLDISSET(MSched.LicensedFeatures,mlfDynamic))
    {
    if (Line[0] != '\0')
      strcat(Line,",");

    strcat(Line,MLicenseFeatures[mlfDynamic]);
    }

  return(SUCCESS);
  }  /* END MSchedLicensedFeaturesToString() */


/**
 * Diagnose/report scheduler state and configuration.
 *
 * NOTE:  process 'mdiag -S' request.
 *
 * @see MUIDiagnose() - parent
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param S (I)
 * @param RE (O)
 * @param String (O)
 * @param DFormat (I)
 * @param IFlags (I) [bitmap of enum MCModeEnum]
 */

int MSchedDiag(

  const char          *Auth,
  msched_t            *S,
  mxml_t              *RE,
  mstring_t           *String,
  enum MFormatModeEnum DFormat,
  mbitmap_t           *IFlags)

  {
  mxml_t *DE = NULL;  /* set to avoid compiler warning */
  mxml_t *SchedE = NULL;

  double  tmpD;

  int     oindex;

  if (S == NULL)
    {
    return(FAILURE);
    }

  if (DFormat == mfmXML)
    {
    char    tmpLine[MMAX_LINE];

    mxml_t *VE;

    if (RE == NULL)
      {
      return(FAILURE);
      }

    DE = RE;

    MXMLCreateE(&SchedE,(char *)MXO[mxoSched]);

    MXMLAddE(DE,SchedE);

    MXMLSetAttr(SchedE,(char *)MSchedAttr[msaState],(void **)MSchedState[S->State],mdfString);

    if (S->Load24HD[mschpTotal] > 0)
      {
      /* add 24 hours scheduler stats to xml output */

      tmpD = (double)S->Load24HD[mschpSched] * 100.0 / S->Load24HD[mschpTotal];
      snprintf(tmpLine,MMAX_LINE,"%.2f%%",tmpD);
      MXMLSetAttr(SchedE,(char*)MSchedPhase[mschpSched],(void **)tmpLine,mdfString);

      tmpD = (double)S->Load24HD[mschpRMLoad] * 100.0 / S->Load24HD[mschpTotal];
      snprintf(tmpLine,MMAX_LINE,"%.2f%%",tmpD);
      MXMLSetAttr(SchedE,(char*)MSchedPhase[mschpRMLoad],(void **)tmpLine,mdfString);

      tmpD = (double)S->Load24HD[mschpRMProcess] * 100.0 / S->Load24HD[mschpTotal];
      snprintf(tmpLine,MMAX_LINE,"%.2f%%",tmpD);
      MXMLSetAttr(SchedE,(char*)MSchedPhase[mschpRMProcess],(void **)tmpLine,mdfString);

      tmpD = (double)S->Load24HD[mschpRMAction] * 100.0 / S->Load24HD[mschpTotal];
      snprintf(tmpLine,MMAX_LINE,"%.2f%%",tmpD);
      MXMLSetAttr(SchedE,(char*)MSchedPhase[mschpRMAction],(void **)tmpLine,mdfString);

      tmpD = (double)S->Load24HD[mschpUI] * 100.0 / S->Load24HD[mschpTotal];
      snprintf(tmpLine,MMAX_LINE,"%.2f%%",tmpD);
      MXMLSetAttr(SchedE,(char*)MSchedPhase[mschpUI],(void **)tmpLine,mdfString);
      }

    VE = NULL;

    MXMLCreateE(&VE,"Version");

    MXMLAddE(SchedE,VE);

#ifdef MOAB_VERSION
    strcpy(tmpLine,MOAB_VERSION);

    MXMLSetAttr(VE,"moabversion",(void **)tmpLine,mdfString);
#endif /* MOAB_VERSION */

#ifdef MOAB_REVISION
    strcpy(tmpLine,MOAB_REVISION);

    MXMLSetAttr(VE,"moabrevision",(void **)tmpLine,mdfString);
#endif /* MOAB_REVISION */

#ifdef MOAB_CHANGESET
    strcpy(tmpLine,MOAB_CHANGESET);

    MXMLSetAttr(VE,"moabchangeset",(void **)tmpLine,mdfString);
#endif /* MOAB_CHANGESET */

#ifdef MUSEMONGODB
    mxml_t *MongoE = NULL;

    MXMLCreateE(&MongoE,"Mongo");

    MXMLAddE(SchedE,MongoE);

    if (!MUStrIsEmpty(MSched.MongoServer))
      strcpy(tmpLine,MSched.MongoServer);
    else
      strcpy(tmpLine,"NOTSET");

    MXMLSetAttr(MongoE,"server",(void **)tmpLine,mdfString);

    if ((!MUStrIsEmpty(MSched.MongoUser)) &&
        (!MUStrIsEmpty(MSched.MongoPassword)))
      strcpy(tmpLine,"TRUE");
    else
      strcpy(tmpLine,"FALSE");

    MXMLSetAttr(MongoE,"credentialsSet",(void **)tmpLine,mdfString);

    if (MMongoInterface::ConnectionIsDown() == TRUE)
      strcpy(tmpLine,"down");
    else
      strcpy(tmpLine,"up");

    MXMLSetAttr(MongoE,"connectionStatus",(void **)tmpLine,mdfString);
#endif

    if (MSched.LicensedFeatures != 0)
      {
      MSchedLicensedFeaturesToString(tmpLine);

      MXMLSetAttr(VE,"licensedfeatures",(void **)tmpLine,mdfString);
      }
    }  /* END if (DFormat == mfmXML) */
  else
    {
    char Host[MMAX_NAME << 1];

    if ((MOSHostIsLocal(S->ServerHost,FALSE) == FALSE) &&
        (bmisset(&MSched.Flags,mschedfFileLockHA)) && 
        (MSched.AltServerHost[0] != '\0'))
      {
      MUStrCpy(Host,S->AltServerHost,sizeof(Host));
      }
    else
      {
      MUStrCpy(Host,S->ServerHost,sizeof(Host));
      }

    if (String == NULL)
      {
      return(FAILURE);
      }

    MStringSet(String,"\0");

    /* display license information */

    if (bmisset(IFlags,mcmVerbose))
      {
      char licStr[MMAX_BUFFER];

      licStr[0] = '\0';

      MLicenseGetLicense(TRUE,licStr,sizeof(licStr));

      MStringAppendF(String,"%s\n",licStr);
      }

    MStringAppendF(String,"Moab Server '%s' running on %s:%d  (Mode: %s)\n",
      S->Name,
      Host,
      S->ServerPort,
      MSchedMode[S->Mode]);

    MStringAppendF(String,"  Version: %s  (revision %s, changeset %s)\n",
      MOAB_VERSION,
#ifdef MOAB_REVISION
      MOAB_REVISION,
#else
      "NA",
#endif /* MOAB_REVISION */
#ifdef MOAB_CHANGESET
      MOAB_CHANGESET);
#else
      "NA");
#endif /* MOAB_CHANGESET */

#ifdef MBUILD_DATE
    MStringAppendF(String,"  Build date: %s\n",
      MBUILD_DATE);
#endif

    if (bmisset(IFlags,mcmVerbose))
      {
      MStringAppendF(String,"  Build Info:  %s\n",
        MSysShowBuildInfo());

      if (S->UseNodeIndex == TRUE)
        MStringAppendF(String,"  Node Index Activated\n");
      }

    if (bmisset(IFlags,mcmVerbose))
      {
      MStringAppendF(String,"  Process Info:  pid:%d uid:%d euid:%d gid:%d egid:%d\n",
        S->PID,
        getuid(),
        geteuid(),
        getgid(),
        getegid());
      }

    if (bmisset(IFlags,mcmVerbose))
      {
      char tmpLine[MMAX_LINE];

      /* Check to see if the moab log file exists and that the user has write permissions */

      snprintf(tmpLine,sizeof(tmpLine),"%s%s",
        S->LogDir,
        S->LogFile);

      if (MSchedDiagDirFileAccess(tmpLine) == FAILURE)
        {
        MStringAppendF(String,"  ALERT:  Moab cannot access Log File '%s' - check directory and file permissions\n",
          tmpLine);
        }

      /* Check to see if the stats directory exists and that the user has write permissions */

      if (MSchedDiagDirFileAccess(MStat.StatDir) == FAILURE)
        {
        MStringAppendF(String,"  ALERT:  Moab cannot access Stats Dir '%s' - check directory permissions\n",
          MStat.StatDir);
        }
      }

#ifdef MUSEMONGODB
    if (MUStrIsEmpty(MSched.MongoServer))
      {
      *String += "  ALERT:  Moab is configured to use Mongo, but no MONGOSERVER specified\n";
      }
    else
      {
      mstring_t MongoCredsUsed = "credentials are ";

      if ((!MUStrIsEmpty(MSched.MongoUser)) &&
          (!MUStrIsEmpty(MSched.MongoPassword)))
        MongoCredsUsed += "set";
      else
        MongoCredsUsed += "not set";

      if (MMongoInterface::ConnectionIsDown() == TRUE)
        {
        *String += "  ALERT:  Mongo connection (";
        *String += MSched.MongoServer;
        *String += ") is down (";
        *String += MongoCredsUsed;
        *String += ")\n";
        }
      else
        {
        *String += "  Mongo connection (";
        *String += MSched.MongoServer;
        *String += ") is up (";
        *String += MongoCredsUsed;
        *String += ")\n";
        }
      }
#endif

    if (bmisset(IFlags,mcmVerbose))
      {
      if (S->VarList != NULL)
        {
        char tmpLine[MMAX_LINE];

        MULLToString(S->VarList,TRUE,NULL,tmpLine,sizeof(tmpLine));

        MStringAppendF(String,"  Variables: %s\n",
          tmpLine);
        }
      }

    if (bmisset(IFlags,mcmVerbose))
      {
      int sindex;
      int gindex;

      MStringAppendF(String,"  Scheduler FeatureGRes: ");

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MUStrIsEmpty(MGRes.Name[gindex]))
          break;

        sindex = MUGetNodeFeature(MGRes.Name[gindex],mVerify,NULL);

        MStringAppendF(String,"%s:%s ",
          MGRes.Name[gindex],
          (bmisset(&S->FeatureGRes,sindex)) ? "on" : "off");
        }

      MStringAppend(String,"\n");
      }

    if (S->LoadLastIteration[mschpTotal] > 0)
      {
      MStringAppendF(String,"  Time(sec) Sched: %.3f  RMLoad: %.3f  RMProcess: %.3f  RMAction: %.3f\n",
        (double)S->LoadLastIteration[mschpSched] / 1000.0,
        (double)S->LoadLastIteration[mschpRMLoad] / 1000.0,
        (double)S->LoadLastIteration[mschpRMProcess] / 1000.0,
        (double)S->LoadLastIteration[mschpRMAction] / 1000.0);

      MStringAppendF(String,"            Triggers: %.3f  User: %.3f  Idle: %.3f Total: %.3f\n",
        (double)S->LoadLastIteration[mschpTriggers] / 1000.0,
        (double)S->LoadLastIteration[mschpUI] / 1000.0,
        (double)S->LoadLastIteration[mschpIdle] / 1000.0,
        (double)S->LoadLastIteration[mschpTotal] / 1000.0);

#ifdef MIDLE
      MStringAppendF(String,"            Empirical Idle: %.3f\n",
        (double)S->LoadLastIteration[mschpMeasuredIdle] / 1000.0);
#endif /* MIDLE */
      }

    if (S->Load5MD[mschpTotal] > 0)
      {
      MStringAppendF(String,"  Load(5m)  Sched: %.2f%%  RMLoad: %.2f%%  RMProcess: %.2f%%  RMAction: %.2f%%\n",
        (double)S->Load5MD[mschpSched] * 100.0 / S->Load5MD[mschpTotal],
        (double)S->Load5MD[mschpRMLoad] * 100.0 / S->Load5MD[mschpTotal],
        (double)S->Load5MD[mschpRMProcess] * 100.0 / S->Load5MD[mschpTotal],
        (double)S->Load5MD[mschpRMAction] * 100.0 / S->Load5MD[mschpTotal]);

      MStringAppendF(String,"            Triggers: %.2f%%  User: %.2f%%  Idle: %.2f%%\n",
        (double)S->Load5MD[mschpTriggers] * 100.0 / S->Load5MD[mschpTotal],
        (double)S->Load5MD[mschpUI] * 100.0 / S->Load5MD[mschpTotal],
        (double)S->Load5MD[mschpIdle] * 100.0 / S->Load5MD[mschpTotal]);

#ifdef MIDLE
      MStringAppendF(String,"            Empirical Idle: %.2f%%\n",
        (double)S->Load5MD[mschpMeasuredIdle] * 100.0 / S->Load5MD[mschpTotal]);
#endif /* MIDLE */

      }

    if (S->Load24HD[mschpTotal] > 0)
      {
      MStringAppendF(String,"  Load(24h) Sched: %.2f%%  RMLoad: %.2f%%  RMProcess: %.2f%%  RMAction: %.2f%%\n",
        (double)S->Load24HD[mschpSched] * 100.0 / S->Load24HD[mschpTotal],
        (double)S->Load24HD[mschpRMLoad] * 100.0 / S->Load24HD[mschpTotal],
        (double)S->Load24HD[mschpRMProcess] * 100.0 / S->Load24HD[mschpTotal],
        (double)S->Load24HD[mschpRMAction] * 100.0 / S->Load24HD[mschpTotal]);

      MStringAppendF(String,"            Triggers: %.2f%%  User: %.2f%%  Idle: %.2f%%\n",
        (double)S->Load24HD[mschpTriggers] * 100.0 / S->Load24HD[mschpTotal],
        (double)S->Load24HD[mschpUI] * 100.0 / S->Load24HD[mschpTotal],
        (double)S->Load24HD[mschpIdle] * 100.0 / S->Load24HD[mschpTotal]);

#ifdef MIDLE
      MStringAppendF(String,"            Empirical Idle: %.2f%%\n",
        (double)S->Load24HD[mschpMeasuredIdle] * 100.0 / S->Load24HD[mschpTotal]);
#endif /* MIDLE */
      }

    if (MSched.Activity != macNONE)
      {
      int aindex;

      if (MSched.Activity != macUIProcess)
        {
        mulong now;

        MUGetTime(&now,mtmNONE,&MSched);

        MStringAppendF(String,"  Current Activity:  %s (Duration: %ld seconds  Count: %d%s%s)\n",
          MActivity[MSched.Activity],
          now - MSched.ActivityTime,
          MSched.ActivityCount[MSched.Activity],
          (MSched.ActivityMsg != NULL) ? "  Msg: " : "",
          (MSched.ActivityMsg != NULL) ? MSched.ActivityMsg : "");
        }

      if (bmisset(IFlags,mcmVerbose))
        {
        char tmpName[MMAX_NAME];

        for (aindex = 0;aindex < macLAST;aindex++)
          {
          if (MSched.ActivityCount[aindex] <= 0)
            continue;

          sprintf(tmpName,"Activity[%s]             ",
            MActivity[aindex]);

          MStringAppendF(String,"  %24.24s   Count=%-2d  Duration=%lds\n",
            tmpName,
            MSched.ActivityCount[aindex],
            MSched.ActivityDuration[aindex]);
          }  /* END for (aindex) */
        }    /* END if (bmisset(IFlags,mcmVerbose)) */
      }      /* END if (MSched.Activity != macNONE) */

    if (S->Iteration > 0)
      {
      char tmpBuf[MMAX_NAME];
      char TString[MMAX_LINE];

      MULToTString(S->MaxRMPollInterval,TString);

      MUStrCpy(tmpBuf,TString,sizeof(tmpBuf));

      MULToTString((S->Time - S->StartTime) / S->Iteration,TString);

      MStringAppendF(String,"  PollInterval: %s  (Avg Sched Interval: %s  Iterations: %d)\n",
        tmpBuf,
        TString,
        S->Iteration);
      }

    if (bmisset(IFlags,mcmVerbose))
      {
      int tmpI;

      tmpI = S->TotalJobStartCounter + S->JobStartPerIterationCounter;

      MStringAppendF(String,"  JobStarts: %d  (Avg Starts/Iteration: %.2f  Last Iteration: %d)\n",
        tmpI,
        (double)tmpI / (S->Iteration + 1),  /* count activity on iteration 0 */
        S->JobStartPerIterationCounter);

      tmpI = S->TotalJobPreempteeCounter + S->JobPreempteePerIterationCounter;

      if (tmpI > 0)
        {
        MStringAppendF(String,"  JobPreempts: %d  (Avg Preempts/Iteration: %.2f  Last Iteration: %d)\n",
          tmpI,
          (double)tmpI / (S->Iteration + 1),  /* count activity on iteration 0 */
          S->JobPreempteePerIterationCounter);
        }
      }

    if (bmisset(IFlags,mcmVerbose))
      {
      MStringAppendF(String,"  Hist. Max Number Waiting Client Connections: %d\n",
        S->HistMaxClientCount);

      /* trigger stats */

      MStringAppendF(String,"\n");  /* space */

      MStringAppendF(String,"  Time spent processing triggers last iteration: %ld (ms)\n",
        MSched.TrigStats.TimeSpentEvaluatingTriggersLastIteration);

      MStringAppendF(String,"  Num. triggers evaluated last iteration: %d\n",
        MSched.TrigStats.NumTrigEvalsLastIteration);

      MStringAppendF(String,"  Num. triggers started last iteration: %d\n",
        MSched.TrigStats.NumTrigStartsLastIteration);

      MStringAppendF(String,"  Num. times Moab processed triggers last iteration: %d\n",
        MSched.TrigStats.NumTrigChecksLastIteration);

      MStringAppendF(String,"  Num. times Moab processed ALL triggers last iteration: %d\n",
        MSched.TrigStats.NumAllTrigsEvaluatedLastIteration);

      MStringAppendF(String,"  Num. times TrigCheckTime (%ld ms) was violated last iteration: %d\n",
        MSched.MaxTrigCheckSingleTime,
        MSched.TrigStats.NumTrigCheckInterruptsLastIteration);
      }

    MStringAppendF(String,"\n");  /* space out notes area */

    if (M64.Is64 == TRUE)
      {
      MStringAppendF(String,"  NOTE:  64-bit mode enabled\n");
      }

    if (bmisset(&MSched.Flags,mschedfFileLockHA))
      {
      if (MSched.HALockFile[0] == '\0')
        {
        MStringAppendF(String,"  NOTE:  Running in non-HA mode\n");
        }
      else
        {
        MStringAppendF(String,"  NOTE:  Running in HA mode (lock file: '%s')\n",
          MSched.HALockFile);
        }
      }  /* END if (bmisset(MSched.Flags,mschedfFileLockHA)) */

    if (bmisset(IFlags,mcmVerbose))
      {
      if (S->Action[mactMail][0] == '\0')
        {
        MStringAppendF(String,"  NOTE:  admin mail notification disabled (set MAILPROGRAM to enable)\n");
        }
      else if (S->DefaultMailDomain != NULL)
        {
        MStringAppendF(String,"  Mail Domain:  %s\n",
          S->DefaultMailDomain);
        }
      }

    if (S->UseKeyFile == TRUE)
      {
      MStringAppendF(String,"  NOTE:  using .moab.key for client authentication\n");
      }
    else
      {
      MStringAppendF(String,"  ALERT:  not using .moab.key for client authentication\n");
      }

    if (S->Mode == msmSim)
      {
      MStringAppendF(String,"  NOTE:  simulation mode enabled\n");

      if (MSim.WorkloadTraceFile[0] == '\0')
        {
        MStringAppendF(String,"  WARNING:  no workload trace file specified (use %s)\n",
          MParam[mcoSimWorkloadTraceFile]);
        }

      if (MSim.RejectJC > 0)
        {
        char tmpLine[MMAX_LINE];

        char *MPtr;
        int   MSpace;

        int rindex;

        MStringAppendF(String,"  WARNING:  %d of %d job traces rejected\n",
          MSim.RejectJC,
          MSim.ProcessJC);

        MUSNInit(&MPtr,&MSpace,tmpLine,sizeof(tmpLine));

        for (rindex = 0;rindex < marLAST;rindex++)
          {
          if (MSim.RejReason[rindex] == 0)
            continue;

          MStringAppendF(String,"[%s:%d]",
            MAllocRejType[rindex],
            MSim.RejReason[rindex]);
          }

        MStringAppendF(String,"    RejectionReasons: %s\n",
          tmpLine);
        }
      else
        {
        MStringAppendF(String,"  NOTE:  %d job traces processed\n",
          MSim.ProcessJC);
        }

      MStringAppendF(String,"  NOTE:  %d of %d job traces submitted (%d lines, current job cache: %d)\n",
        MSim.JTIndex,
        MSim.TotalJTCount,
        MSim.JTLines,
        MSim.JTCount);

      if (MSim.CorruptJTMsg != NULL)
        {
        MStringAppendF(String,"  NOTE:  %s",
          MSim.CorruptJTMsg);
        }

      if (mlog.Threshold > 2)
        {
        MStringAppendF(String,"  WARNING:  simulation loglevel is high, simulation may run slowly\n");
        }
      }  /* END if (S->Mode == msmSim) */
    else if ((S->Mode == msmTest) || (S->Mode == msmMonitor))
      {
      if ((S->TestModeBuffer != NULL) && (S->TestModeBuffer[0] != '\0'))
        {
        MStringAppendF(String,"  NOTE:  test mode operations for this iterations ------\n%s-----------------------------------\n",
          S->TestModeBuffer);
        }
      else
        {
        MStringAppendF(String,"  NOTE:  test mode indicates no scheduling operations this iteration\n");
        }
      }
    else 
      {
      if (MRM[0].Type == mrmtNONE)
        {
        MStringAppendF(String,"  WARNING:  no resource manager configured - no resource or workload information will be loaded (use %s)\n",
          MParam[mcoRMCfg]);
        }
      }

    if (S->NoJobMigrationMethod == TRUE)
      {
      MStringAppendF(String,"  WARNING:  job migration is not available\n");

      MStringAppendF(String,"      A job has been submitted with msub but no valid resource manager\n");
      MStringAppendF(String,"      destination exists.  This can be remedied by doing one of the following:\n");
      MStringAppendF(String,"      a) run Moab as root, b) interface with Globus, c) enable Moab peer-to-\n");
      MStringAppendF(String,"      peer services, or d) enable other job migration methods as described in\n");
      MStringAppendF(String,"      the 'Job Migration' section of the Moab Workload Manager Admin Manual.\n");
      }
    }    /* END else (DFormat == mfmXML) */

  /* display high availability status */

  if (S->AltServerHost[0] != '\0')
    {
    if (DFormat == mfmXML)
      {
      MXMLSetAttr(SchedE,"HAServerHost",(void **)S->AltServerHost,mdfString);
      }
    else
      {
      MStringAppendF(String,"  HA Fallback Server:  %s",
        S->AltServerHost);

      if (MOSHostIsLocal(S->AltServerHost,FALSE) == TRUE)
        {
        MStringAppendF(String,"  ALERT:  primary is DOWN, fallback is active\n");
        }
      }
    }    /* END if (S->AltServerHost[0] != '\0') */

  if (!bmisclear(&MSched.Flags))
    {
    if (DFormat != mfmXML)
      {
      mstring_t Line(MMAX_LINE);

      bmtostring(&MSched.Flags,MSchedFlags,&Line);

      MStringAppendF(String,"  Flags:               %s\n",
        Line.c_str());
      }
    }

  if (DFormat == mfmXML)
    {
    return(SUCCESS);
    }

  if (S->State != mssmRunning)
    {
    /* NOTE:  indicate duration of stop (NYI) */

    if (S->Mode == msmSlave)
      {
      MStringAppendF(String,"  NOTE:  local scheduling disabled (SLAVE mode)\n");
      }
    else
      {
      if (S->UseCPRestartState == TRUE)
        {
        MStringAppendF(String,"  NOTE:  scheduling restart state set to %s from checkpoint file\n",
          MSchedState[S->RestartState]);
        }
      else if (S->StateMAdmin != NULL)
        {
        char TString[MMAX_LINE];

        MULToTString(S->Time - S->StateMTime,TString);

        MStringAppendF(String,"  NOTE:  scheduling state was set to %s by admin %s (in current state for %s)\n",
          MSchedState[S->State],
          S->StateMAdmin,
          TString);
        }
      else
        {
        MStringAppendF(String,"  NOTE:  scheduling state set to %s\n",
          MSchedState[S->State]);
        }
      }
    }    /* END if (S->State != mssmRunning) */

  if (S->RestartTime != 0)
    {
    char TString[MMAX_LINE];

    /* NOTE:  indicate duration to restart */

    MULToTString(S->RestartTime - S->Time,TString);

    MStringAppendF(String,"  NOTE:  scheduler will restart in %s\n",
      TString);
    }

  if ((S->SRsvIsActive == TRUE) && (!MUIsGridHead()))
    {
    /* NOTE:  indicate duration of system rsv (NYI) */

    MStringAppendF(String,"  NOTE:  system reservation blocking all nodes\n");
    }

  if (S->MaxRsvDelay > 0)
    {
    char TString[MMAX_LINE];

    MULToTString(S->MaxRsvDelay,TString);

    MStringAppendF(String,"  NOTE:  one or more job reservations blocked by system issues (max delay: %s)\n",
      TString);

    if (MPar[0].RsvRetryTime > 0)
      {
      MULToTString(MPar[0].RsvRetryTime,TString);

      MStringAppendF(String,"  NOTE:  parameter %s set to %s\n",
        MParam[mcoRsvRetryTime],
        TString);
      }
    else
      {
      MStringAppendF(String,"  NOTE:  parameter %s not set\n",
        MParam[mcoRsvRetryTime]);
      }
    }

  if (S->IgnoreClasses != NULL)
    {
    int cindex;

    MStringAppendF(String,"  NOTE:  ignoring workload from classes ");

    for (cindex = MCLASS_FIRST_INDEX;cindex < S->M[mxoClass];cindex++)
      {
      if (S->IgnoreClasses[cindex] == NULL)
        break;

      if (cindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->IgnoreClasses[cindex]);
      }  /* END for (cindex) */
    }
  else if (S->SpecClasses != NULL)
    {
    int cindex;

    MStringAppendF(String,"  NOTE:  only processing workload from classes ");

    for (cindex = MCLASS_FIRST_INDEX;cindex < S->M[mxoClass];cindex++)
      {
      if (S->SpecClasses[cindex] == NULL)
        break;

      if (cindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->SpecClasses[cindex]);
      }  /* END for (cindex) */
    }

  if (S->IgnoreJobs != NULL)
    {
    int jindex;

    MStringAppendF(String,"  NOTE:  ignoring jobs ");

    for (jindex = 0;jindex < MMAX_IGNJOBS;jindex++)
      {
      if (S->IgnoreJobs[jindex] == NULL)
        break;

      if (jindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->IgnoreJobs[jindex]);
      }  /* END for (jindex) */
    }
  else if (S->SpecJobs != NULL)
    {
    int jindex;

    MStringAppendF(String,"  NOTE:  only processing jobs ");

    for (jindex = 0;jindex < MMAX_IGNJOBS;jindex++)
      {
      if (S->SpecJobs[jindex] == NULL)
        break;

      if (jindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->SpecJobs[jindex]);
      }  /* END for (jindex) */
    }

  if (S->IgnoreUsers != NULL)
    {
    int uindex;

    MStringAppendF(String,"  NOTE:  ignoring workload from users ");

    for (uindex = 0;uindex < S->M[mxoUser];uindex++)
      {
      if (S->IgnoreUsers[uindex] == NULL)
        break;

      if (uindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->IgnoreUsers[uindex]);
      }  /* END for (uindex) */

    MStringAppendF(String,"\n");
    } 
  else if (S->SpecUsers != NULL)
    {
    int uindex;

    MStringAppendF(String,"  NOTE:  only processing workload from users ");

    for (uindex = 0;uindex < S->M[mxoUser];uindex++)
      {
      if (S->SpecUsers[uindex] == NULL)
        break;

      if (uindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->SpecUsers[uindex]);
      }  /* END for (uindex) */

    MStringAppendF(String,"\n");
    }

  if (S->IgnoreNodes.List != NULL)
    {
    int nindex;

    MStringAppendF(String,"  NOTE:  ignoring nodes ");

    for (nindex = 0;nindex < S->M[mxoNode];nindex++)
      {
      if (S->IgnoreNodes.List[nindex] == NULL)
        break;

      if (nindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->IgnoreNodes.List[nindex]);
      }  /* END for (nindex) */

    MStringAppendF(String,"\n");
    }
  else if (S->SpecNodes.List != NULL)
    {
    int nindex;

    MStringAppendF(String,"  NOTE:  only processing nodes ");

    for (nindex = 0;nindex < S->M[mxoNode];nindex++)
      {
      if (S->SpecNodes.List[nindex] == NULL)
        break;

      if (nindex > 0)
        MStringAppendF(String,",");

      MStringAppendF(String,S->SpecNodes.List[nindex]);
      }  /* END for (nindex) */

    MStringAppendF(String,"\n");
    }  /* END else if (S->SpecNodes.List != NULL) */

  MStringAppendF(String,"\n");  /* add spacing between notes and object specs */

  if (bmisset(IFlags,mcmVerbose))
    {
    /* extern int MGlobalMsgBufSize;     */

    MStringAppendF(String,"  Object Specs:        Class=%d  GRes=%d  GMetric=%d  Job=%d  Node=%d  Par=%d  Range=%d  RM=%d",
      S->M[mxoClass],
      MSched.M[mxoxGRes],
      S->M[mxoxGMetric],
      S->M[mxoJob],
      S->M[mxoNode],
      S->M[mxoPar],
      MMAX_RANGE,
      S->M[mxoRM]);
    }

  if (MSched.MaxVM > 0)
    {
    MStringAppendF(String,"  VM=%d/%d",MSched.VMTable.NumItems,MSched.MaxVM);
    }

  MStringAppend(String,"\n");

#ifdef MYAHOO

  /* for Yahoo, display general stats about the various obj tables */

  MStringAppendF(String,"  General Obj. Stats (<CURRENT>|<MAX>|<PEAK>)\n");

  for (oindex = 0;oindex < mxoLAST;oindex++)
    {
    switch (oindex)
      {
      case mxoJob:
      case mxoTrig:

        MStringAppendF(String,"    %s: %d|%d|%d\n",
          MXO[oindex],
          MSched.A[oindex],
          MSched.M[oindex],
          MSched.Max[oindex]);
       
        break;

      default:
        /* NO-OP */
        break;
      }
    }

  MStringAppendF(String,"    open client sockets: %d|%d|%d\n",
    MSUClientCount,
    MSched.ClientMaxConnections,
    S->HistMaxClientCount);

  MStringAppendF(String,"    queued socket transactions: %d|inf|%d\n",
    MSched.TransactionCount,
    S->HistMaxTransactionCount);

#endif /* MYAHOO */

#ifdef MYAHOO
#  define MOVERFLOW_THRESHOLD 0.7
#else
#  define MOVERFLOW_THRESHOLD 0.9
#endif /* MYAHOO */

  /* report overflow on node, gres, par, job, user, class, and rsv tables */

  /* NOTE:  gres is not MXO object, must be handled separately (NYI) - see MWikiI.c */

  for (oindex = 0;oindex < mxoLAST;oindex++)
    {
    if (S->OFOID[oindex] == NULL)
      {
      /* table limit has not been reached, check for near-full conditions */
      /* NOTE: in the future, we should change S->M[oindex] to MDEF_CJOB_CEILING for more
       * accurate reporting */

      if (S->Max[oindex] > (double)S->M[oindex] * MOVERFLOW_THRESHOLD)
        {
        if (oindex == mxoSRsv) 
          continue; /* no limit */

        MStringAppendF(String,"  NOTE:  %s table has neared capacity - peak usage detected was %d of %d",
          MXO[oindex],
          MIN(S->M[oindex],S->Max[oindex]),
          S->M[oindex]);

        MStringAppendF(String,"\n");  
        }

      continue;
      }
   
    if (oindex == mxoxGMetric)
      {
      MStringAppendF(String,"  NOTE:  %s limit reached (%d) - could not load %s, adjust MAXGMETRIC\n",
        MXO[mxoxGMetric],
        MSched.M[mxoxGMetric],
        S->OFOID[oindex]);
      }
    else if (oindex == mxoxGRes)
      {
      MStringAppendF(String,"  NOTE:  %s limit reached (%d) - could not load %s, adjust MAXGRES\n",
        MXO[mxoxGRes],
        MSched.M[mxoxGRes],
        S->OFOID[oindex]);
      }
    else if (oindex == mxoxTasksPerJob)
      {
      MStringAppendF(String,"  NOTE:  %s limit reached (%d) - could not schedule %s, adjust JOBMAXTASKCOUNT\n",
        MXO[oindex],
        MSched.JobMaxTaskCount,
        S->OFOID[oindex]);
      }
    else if (oindex == mxoJob)
      {
      MStringAppendF(String,"  NOTE:  job limit reached (%d) - cannot create %s, increase parameter %s\n",
        S->M[mxoJob],
        S->OFOID[oindex],
        MParam[mcoMaxJob]);
      }
    else if (oindex == mxoxNodeFeature)
      {
      MStringAppendF(String,"  NOTE:  node feature limit reached (%d) - cannot create %s, use 'configure' option '--with-maxattrs=X' or contact support\n",
        MMAX_ATTR,
        S->OFOID[oindex]);
      }
    else if ((oindex == mxoAcct)  ||
             (oindex == mxoGroup) ||
             (oindex == mxoUser))
      {
      MStringAppendF(String,"  NOTE:  %s limit reached (%d) - could not load %s\n",
        MXO[oindex],
        S->Max[oindex],
        S->OFOID[oindex]);
      } 
    else
      {  
      MStringAppendF(String,"  NOTE:  %s limit reached (%d) - could not load %s\n",
        MXO[oindex],
        S->M[oindex],
        S->OFOID[oindex]);
      }
    }    /* END for (oindex) */

  if (S->JobMaxNodeCountOFOID != NULL)
    {
    MStringAppendF(String,"  NOTE:  %s limit %d reached on job '%s' - increase parameter %s\n",
      MParam[mcoJobMaxNodeCount],
      S->JobMaxNodeCount,
      S->JobMaxNodeCountOFOID,
      MParam[mcoJobMaxNodeCount]);
    }

  if (S->HistMaxClientCount > (double)MSched.ClientMaxConnections * MOVERFLOW_THRESHOLD)
    {
    MStringAppendF(String,"  NOTE:  socket table has neared capacity - peak usage detected was %d of %d\n",
      S->HistMaxClientCount,
      MSched.ClientMaxConnections);
    }

  if (S->UIBufferMsg != NULL)
    {
    MStringAppendF(String,"  NOTE:  user interface message buffer limit reached - '%s'\n",
      S->UIBufferMsg);
    }
 
  if (S->NodeREOverflowDetected == TRUE)
    {
    if (S->MaxRsvPerNode != S->MaxRsvPerGNode)
      {
      MStringAppendF(String,"  NOTE:  %s limit reached (%d,%d) - increase parameter %s\n",
        MParam[mcoMaxRsvPerNode],
        S->MaxRsvPerNode,
        S->MaxRsvPerGNode,
        MParam[mcoMaxRsvPerNode]);
      }
    else
      {
      MStringAppendF(String,"  NOTE:  %s limit reached (%d) - increase parameter %s\n",
        MParam[mcoMaxRsvPerNode],
        S->MaxRsvPerNode,
        MParam[mcoMaxRsvPerNode]);
      }
    }

  if (S->UIBadRequestMsg != NULL)
    {
    MStringAppendF(String,"\n  NOTE:  bad client message received from host %s - '%s'\n\n",
      S->UIBadRequestor,
      S->UIBadRequestMsg);
    }

  /* display profiling status */

  if (MPar[0].S.IStat != NULL)
    {
    char TString[MMAX_LINE];

    MULToTString(MStat.P.IStatDuration,TString);

    MStringAppendF(String,"  Message:  profiling enabled (%d of %d samples/%s interval)\n",
      MPar[0].S.IStatCount,
      MStat.P.MaxIStatCount,
      TString);
    }

  if (bmisset(IFlags,mcmVerbose))
    {
    char tmpBuf[MMAX_BUFFER];

    tmpBuf[0] = '\0';

    /* display system messages */

    if (S->MB != NULL)
      {
      int   tSpace;
      char *tPtr;

      MStringAppendF(String,"\nSystem Messages-------\n");

      tSpace = sizeof(tmpBuf) - strlen(tmpBuf);
      tPtr   = tmpBuf + strlen(tmpBuf);

      /* display data */

      MMBPrintMessages(
        S->MB,
        mfmVerbose,
        bmisset(IFlags,mcmVerbose2),  /* verbose */
        -1,
        tPtr,
        tSpace);

      MStringAppendF(String,tmpBuf);
      }
    }  /* END if (bmisset(IFlags,mcmVerbose)) */

  /* If memory tracker is enabled (a pointer is there), then we report its info */
  if (mlog.MemoryTracker != NULL)
    {
    char tmpBuf[MMAX_LINE];

    MUMemReport(FALSE,tmpBuf,sizeof(tmpBuf));

    MStringAppendF(String,"%s",
      tmpBuf);

    MUMemReport(TRUE,NULL,0);
    }

  if (bmisset(IFlags,mcmVerbose2))
    {
    int cindex;

    int Count;
    int FCount;
    mulong QTime;
    mulong PTime;
    mulong STime;

    MStringAppendF(String,"\nClient Requests------- (in milliseconds)\n");

    Count = 0;
    FCount = 0;
    QTime = 0;
    PTime = 0;
    STime = 0;

    MStringAppendF(String,"%-12s  %6s  %6s  %8s  %8s  %8s\n",
      "SERVICE",
      "COUNT",
      "FAIL",
      "QTIME",
      "PTIME",
      "STIME");

    /* NOTE:  should report client times as average, not total (NYI) */

    for (cindex = 0;cindex < mcsLAST;cindex++)
      {
      if (MUIStat[cindex].Count <= 0)
        continue;

      MStringAppendF(String,"%-12s  %6d  %6d  %8.3f  %8.3f  %8.3f\n",
        MUI[cindex].SName,
        MUIStat[cindex].Count,
        MUIStat[cindex].FCount,
        (double)MUIStat[cindex].QTime / MUIStat[cindex].Count / 1000.0,
        (double)MUIStat[cindex].PTime / MUIStat[cindex].Count / 1000.0,
        (double)MUIStat[cindex].STime / MUIStat[cindex].Count / 1000.0);

      Count +=  MUIStat[cindex].Count;
      FCount += MUIStat[cindex].FCount;
      QTime +=  MUIStat[cindex].QTime;
      PTime +=  MUIStat[cindex].PTime;
      STime +=  MUIStat[cindex].STime;
      }  /* END for (cindex) */

    MStringAppendF(String,"%-12s  %6d  %6d  %8.3f  %8.3f  %8.3f\n",
      "TOTAL",
      Count,
      FCount,
      (double)QTime / MAX(1,Count) / 1000.0,
      (double)PTime / MAX(1,Count) / 1000.0,
      (double)STime / MAX(1,Count) / 1000.0);

    MStringAppendF(String,"\nTotal Time: %ld seconds  (%.2lf requests/sec)\n",
      MSched.Time - MSched.StartTime,
      (double)Count / MAX(1,MSched.Time - MSched.StartTime));
    }    /* END if (bmisset(IFlags,mcmVerbose2)) */

  return(SUCCESS);
  }  /* END MSchedDiag() */
/* END MSchedDiag.c */
