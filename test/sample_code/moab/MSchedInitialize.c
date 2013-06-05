/* HEADER */

      
/**
 * @file MSchedInitialize.c
 *
 * Contains: MSchedInitialize functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


#define MVM_STALEITERATIONS 5
#define MDEF_GEVENTTIMEOUT 300 * 6    /* 30 minutes */

/**
 * Set default scheduler attributes.
 *
 * @see MSchedInitialize() - parent
 *
 * @param S (I) [modified]
 */

int MSchedSetDefaults(

  msched_t *S)  /* I (modified) */

  {
  int    index;

  struct timezone  tzp;    /* NOTE: 'struct timezone' is obsolete (see man page) */

  char  *ptr;
  char  *TokPtr;

  int    tmpI;

  char   tmpLine[MMAX_LINE];

  if (S == NULL)
    {
    return(FAILURE);
    }

#ifdef MSCHED_SNAME
  MUStrCpy(S->Name,MSCHED_SNAME,sizeof(S->Name));
#else /* MSCHED_SNAME */
  MUStrCpy(S->Name,"moab",sizeof(S->Name));
#endif /* MSCHED_SNAME */

  S->ServerPort             = MDEF_SERVERPORT;
  S->ClientUIPort           = S->ServerPort+1;
  S->Mode                   = MDEF_SERVERMODE;

  S->DefaultCSAlgo          = MDEF_CSALGO;

  S->MaxRsvPerNode          = MDEF_RSV_DEPTH;
  S->MaxRsvPerGNode         = MDEF_RSV_GDEPTH;

  /* maximum number of nodes which can be allocated to any job, Max nodes set in MSchedInitialize() */

  S->JobMaxNodeCount        = MIN(MSched.M[mxoNode],MDEF_JOBMAXNODECOUNT);
  S->JobMaxTaskCount        = MMAX_TASK;

  /* initial/default size of all node tables attached to any job */

  S->JobDefNodeCount        = MIN(S->JobMaxNodeCount,MDEF_JOBDEFNODECOUNT);

  S->ResourceQueryDepth     = MDEF_RESOURCEQUERYDEPTH;

  S->IgnoreToTime           = 0;
  S->IgnoreToIteration      = 0;

  S->StopIteration          = -1;

  S->MinVMOpDelay           = MDEF_MINVMOPDELAY;
  S->FutureVMLoadFactor     = MDEF_FUTUREVMLOADFACTOR;

  S->PreemptPrioWeight      = MDEF_PREEMPTPRIOWEIGHT;
  S->PreemptRTimeWeight     = MDEF_PREEMPTRTIMEWIGHT;

  S->NTRJob                 = NULL;

  S->ClockSkew              = MMAX_CLIENTSKEW;

  S->TransactionIDCounter   = MMIN_TRANSACTIONID;

  S->DefProvDuration        = MDEF_MINPROVDURATION;

  S->ARFDuration            = MDEF_ARFDURATION;

  strcpy(S->ServerHost,MDEF_SERVERHOST);

  strcpy(S->DefaultDomain,DEFAULT_DOMAIN);

  strcpy(S->Action[mactJobFB],MDEF_JOBFBACTIONPROGRAM);
  
  /* NOTE:  disable mail by default */

  /* strcpy(S->Action[mactMail],MDEF_MAILACTION); */

  S->SingleJobMaxPreemptee  = MMAX_PJOB;

  S->DefaultNAccessPolicy   = MDEF_NACCESSPOLICY;
  S->OESearchOrder          = MDEF_OESEARCHORDER;
  S->OSCredLookup           = MDEF_OSCREDLOOKUP;

  S->StepCount              = MDEF_SCHEDSTEPCOUNT;
  S->LogFileMaxSize         = MDEF_LOGFILEMAXSIZE;
  S->LogFileRollDepth       = MDEF_LOGFILEROLLDEPTH;

  S->SPVJobIsPreemptible    = MDEF_SPVJOBISPREEMPTIBLE;
  S->AdjustUtlRes           = MDEF_ADJUSTUTLRESOURCES;

  S->AggregateNodeActionsTime = MDEF_AGGREGATENODEACTIONSTIME;

  if (S->DefaultJ != NULL)
    S->DefaultJ->SpecWCLimit[1] = MDEF_SYSDEFJOBWCLIMIT;

  S->FilterCmdFile          = TRUE;

  S->UseCPRsvNodeList       = TRUE;

  S->DisableUICompression   = TRUE;

  S->RsvCounter             = MAX(1,S->RsvCounter);

  S->RsvGroupCounter        = MAX(1,S->RsvGroupCounter);

  S->TransCounter           = MAX(1,S->TransCounter);

  S->Iteration              = -1;
  S->State                  = mssmRunning;
  S->Interval               = 0;

  S->JobRsvRetryInterval    = MDEF_JOBRSVRETRYINTERVAL;

  S->AuthTimeout            = MDEF_AUTHTIMEOUT;

  S->DefaultRackSize        = MCONST_DEFRACKSIZE;

  S->NodeFailureReserveTime = MDEF_NODEFAILURERSVTIME;

  S->SpoolDirKeepTime       = MCONST_YEARLEN;

  S->GEventTimeout          = MDEF_GEVENTTIMEOUT;

  gettimeofday(&S->SchedTime,&tzp);

  S->GreenwichOffset        = - (tzp.tz_minuteswest * 60); 

  for (index = 0;index <= MMAX_ADMINTYPE;index++)
    {
    S->Admin[index].UName[0][0]       = '\0';

    S->Admin[index].Index = index;
    }  /* END for (index) */

  strcpy(S->Admin[1].Name,"admin");
  strcpy(S->Admin[2].Name,"operator");
  strcpy(S->Admin[3].Name,"helpdesk");
  strcpy(S->Admin[4].Name,"other1");
  strcpy(S->Admin[5].Name,"other2");

  strcpy(S->Admin[1].UName[0],MDEF_PRIMARYADMIN);
  S->Admin[1].UName[1][0]   = '\0';

  strcpy(S->SubmitHosts[0],"ALL");
  S->SubmitHosts[1][0]      = '\0';

  S->CfgMem[0]              = -1;

  S->DeferTime              = MDEF_DEFERTIME;
  S->DeferCount             = MDEF_DEFERCOUNT;
  S->DeferStartCount        = MDEF_DEFERSTARTCOUNT;

  S->MaxJobStartPerIteration = -1;
  S->MaxPrioJobStartPerIteration = -1;

  S->MinJobCounter          = 0;
  S->MaxJobCounter          = MMAX_INT;

  S->MaxRecordedCJobID      = MMAX_INT;

  S->JobPurgeTime           = MDEF_JOBPURGETIME;
  S->JobCPurgeTime          = MDEF_JOBCPURGETIME;
  S->RsvCPurgeTime          = MDEF_RSVCPURGETIME;
  S->VMCPurgeTime           = MDEF_VMCPURGETIME;

  S->VMStaleIterations      = MVM_STALEITERATIONS;

  S->NodeDownTime           = MMAX_TIME;
  S->NodePurgeTime          = MDEF_NODEPURGETIME;
  S->NodePurgeIteration     = -1;

  S->FreeTimeLookAheadDuration = MDEF_FREETIMELOOKAHEADDURATION;
  S->NonBlockingCacheInterval = MDEF_NONBLOCKINGCACHEINTERVAL;

  S->APIFailureThreshold   = MDEF_APIFAILURETHRESHOLD;

  S->NodeSyncDeadline       = MDEF_NODESYNCDEADLINE;
  S->JobSyncDeadline        = MDEF_JOBSYNCDEADLINE;
  S->JobHMaxOverrun         = MDEF_JOBMAXOVERRUN;
  S->JobSMaxOverrun         = 0;

  S->MaxGreenStandbyPoolSize = 0;

  S->MaxOSCPULoad           = MAX_OS_CPU_OVERHEAD;

  S->StartTime              = S->Time;

  S->RMMsgIgnore            = MDEF_RMMSGIGNORE;
  S->MaxRMPollInterval      = MDEF_RMPOLLINTERVAL;
  S->MinRMPollInterval      = 0;

  S->MaxTrigCheckSingleTime = MDEF_TRIGCHECKTIME;
  S->MaxTrigCheckTotalTime  = MCONST_EFFINF;
  S->MaxTrigCheckInterval   = MDEF_TRIGINTERVAL;
  S->TrigEvalLimit          = MDEF_TRIGEVALLIMIT;

  S->SocketWaitTime         = MDEF_SOCKETWAIT;
  S->SocketLingerVal        = MDEF_SOCKETLINGERVAL;

  S->HALockCheckTime        = MDEF_HALOCKCHECKTIME;
  S->HALockUpdateTime       = MDEF_HALOCKUPDATETIME;

  S->MaxSleepIteration      = MDEF_MAXSLEEPITERATION;
  S->PreemptPolicy          = MDEF_PREEMPTPOLICY;
  S->Mode                   = MDEF_SERVERMODE;

  S->SMatrixPeriod          = MDEF_SMATRIXPERIOD;

  S->ReferenceProcSpeed     = MMAX_INT;

  S->SharedPtIndex          = -1;

  S->AdminEventAggregationTime = -1;  /* unlimited aggregation */

  bmclear(&S->RecordEventList);

  MUStrCpy(tmpLine,MDEF_RELISTBITMAP,sizeof(tmpLine));

  ptr = MUStrTok(tmpLine,",",&TokPtr);

  while (ptr != NULL)
    {
    tmpI = MUGetIndexCI(ptr,MRecordEventType,FALSE,0);

    bmset(&S->RecordEventList,tmpI);

    ptr = MUStrTok(NULL,",",&TokPtr);
    }  /* END while (ptr != NULL) */

  strcpy(S->DefaultClassList,MDEF_CLASSLIST);

  S->RemapC                 = NULL;

  strcpy(S->Action[mactAdminEvent],MDEF_ADMINEACTIONPROGRAM);

  S->DefaultJobOREnd        = MDEF_JOBOREND;

  S->FSTreeACLPolicy        = mfsapFull;

  S->MissingDependencyAction = mmdaHold;

  /* load env values */

  MSched.EnableInternalCharging = MDEF_ENABLEINTERNALCHARGING;

  MUStrCpy(MSched.TrustedCharList,MDEF_TRUSTEDCHARLIST,sizeof(MSched.TrustedCharList));
  MUStrCpy(MSched.TrustedCharListX,MDEF_TRUSTEDCHARLISTX,sizeof(MSched.TrustedCharListX));

  bmsetall(&S->RealTimeDBObjects,mxoLAST);

  if (getenv("MOABDISABLEJOBFEASIBILITYCHECK") != NULL)
    {
    MDB(1,fCORE) MLog("INFO:     job feasibility check disabled (env)\n");

    MSched.DisableJobFeasibilityCheck = TRUE;
    }

  ptr = getenv("MOABPARCLEANUP");

  if (ptr != NULL)
    {
    if (!strcasecmp(ptr,"FULL"))
      MSched.AllocPartitionCleanup = TRUE;
    else
      MSched.AllocPartitionCleanup = MBNOTSET;
    }
  else
    {
    MSched.AllocPartitionCleanup = FALSE;
    }

  if (getenv("MOABUSENODEINDEX") != NULL)
    {
    MDB(1,fCORE) MLog("INFO:     node index table enabled (env)\n");

    MSched.UseNodeIndex = TRUE;
    }

  MSched.DisableTriggers    = FALSE;

  MSched.AggregateNodeActions = TRUE;

  if (getenv(MSCHED_ENVDISABLETRIGGER) != NULL)
    {
    MDB(1,fCORE) MLog("INFO:     triggers disabled (env)\n");

    MSched.DisableTriggers = TRUE;
    }

  if (getenv("MOABTRACKSUSPEND") != NULL)
    {
    MDB(1,fCORE) MLog("INFO:     preempt usage tracking enabled (env)\n");

    MSched.TrackSuspendedJobUsage = TRUE;
    }

  ptr = getenv(MSCHED_ENVCKTESTVAR);

  if (ptr != NULL)
    {
    MDB(1,fCORE) MLog("INFO:     checkpoint test enabled (env)\n");

    MCP.EnableTestRead = TRUE;

    if (strcasecmp(ptr,"read"))
      MCP.EnableTestWrite = TRUE; 
    }

  MSched.EnableImplicitStageOut = MDEF_ENABLEIMPLICITSTAGEOUT;

  MSched.ShowMigratedJobsAsIdle = FALSE;

  MSched.MWikiSubmitTimeout = 10; /* 10 seconds */

  MSched.IgnoreRMDataStaging = FALSE;

  MSched.LastTransCommitted = -1;

  MSched.DisableRegExCaching = FALSE;

  /* the below params have been tested enough @
   * Yahoo and should always be turned on to
   * improve performance */

  MSched.EnableFastSpawn = TRUE;
  S->EnableFastNativeRM = TRUE;

  S->VMCreateThrottle = MDEF_VMCREATETHROTTLE;
  S->VMMigrateThrottle = MDEF_VMMIGRATETHROTTLE;
  S->AllowVMMigration = TRUE;
  S->VMTracking = TRUE;

#ifdef MSCYLD
  MUStrDup(&MSched.LicMsg,"  Contact Penguin Computing for assistance at:\n\n888-PENGUIN (888-736-4846)\nsupport@penguincomputing.com\n");
#else /* MSCYLD */
  MUStrDup(&MSched.LicMsg,"  Please contact Adaptive Computing for assistance\n   (http://www.adaptivecomputing.com/contact.shtml)\n");
#endif /* MSCYLD */

  MSched.ClientMaxConnections = MDEF_MAXCLIENT;
  MSched.VMStorageNodeThreshold = MDEFAULT_STORAGE_NODE_THRESHOLD;

  MSched.DontEnforcePeerJobLimits = FALSE;
  MSched.DisplayProxyUserAsUser = FALSE;
  MSched.VMCalculateLoadByVMSum = FALSE;

  MSched.TPSize = MDEF_TP_HANDLER_THREADS;

  MSched.GuaranteedPreemption = TRUE;
  MSched.CheckSuspendedJobPriority = TRUE;

  MTJobAdd("DEFAULT",&MSched.DefaultJ);

  return(SUCCESS);
  }  /* END MSchedSetDefaults() */


/**
 * Initialize core scheduler data structure.
 *
 * @see struct msched_t
 * @see MSysInitialize() - peer
 *
 * @param S (I)
 */

int MSchedInitialize(

  msched_t *S)  /* I */

  {
  char            *ptr;

  int              count;
  char            *buf;

  char             tmpLine[MMAX_LINE];

  const char *FName = "MSchedInitialize";

  MDB(4,fSCHED) MLog("%s(S)\n",
    FName);

  S->IsServer   = TRUE;

  /* set scheduler defaults */

  S->PID = MOSGetPID();    

  strcpy(S->Version,MOAB_VERSION);

#ifdef MOAB_REVISION
  MUStrCpy(S->Revision,MOAB_REVISION,sizeof(S->Revision));
#endif /* MOAB_REVISION */

  /* set default umask */

  S->UMask = MDEF_UMASK;

  umask(S->UMask);

#ifdef MOAB_CHANGESET
  MUStrCpy(S->Changeset,MOAB_CHANGESET,sizeof(S->Changeset));
#endif /* MOAB_CHANGESET */

  sprintf(S->MFullRevision,"Version: %s server %s (revision %s, changeset %s)\n",
            MSCHED_SNAME,
            MOAB_VERSION,
#ifdef MOAB_REVISION
            MOAB_REVISION,
#else /* MOAB_REVISION */
            "NA",
#endif /* MOAB_REVISION */
#ifdef MOAB_CHANGESET
            MOAB_CHANGESET);
#else /* MOAB_CHANGESET */
            "NA");
#endif /* MOAB_CHANGESET */

  /* set repeatable random seed */

  srand(1);
 
  /* srand(S->PID); */

  S->UnsupportedDependencies[0] = '\0';

  /* setup home directory */

  if (getcwd(tmpLine,sizeof(tmpLine)) != NULL)
    {
    MUStrDup(&S->LaunchDir,tmpLine);
    }

  if ((ptr = getenv(MSCHED_ENVHOMEVAR)) != NULL)
    {
    MDB(4,fCONFIG) MLog("INFO:     using %s environment variable (value: %s)\n",
      MSCHED_ENVHOMEVAR,
      ptr);

    MSchedSetAttr(S,msaHomeDir,(void **)ptr,mdfString,mSet);
    }
  else if ((ptr = getenv(MParam[mcoMServerHomeDir])) != NULL)
    {
    MDB(4,fCONFIG) MLog("INFO:     using %s environment variable (value: %s)\n",
      MParam[mcoMServerHomeDir],
      ptr);

    MSchedSetAttr(S,msaHomeDir,(void **)ptr,mdfString,mSet);
    }
  else
    {
    mbool_t HomeDirSet = FALSE;

    int rc;

    /* check master configfile */
 
    if ((buf = MFULoad(MCONST_MASTERCONFIGFILE,1,macmRead,&count,NULL,NULL)) != NULL)
      {
      if ((ptr = strstr(buf,MParam[mcoMServerHomeDir])) != NULL)
        {
        ptr += (strlen(MParam[mcoMServerHomeDir]) + 1);

        rc = sscanf(ptr,"%128s",
          S->CfgDir);

        if (rc == 1)
          {
          HomeDirSet = TRUE;
          MUStrCpy(S->VarDir,S->CfgDir,sizeof(S->VarDir));
          }
        }
      }

    if (HomeDirSet == FALSE)
      {
#ifdef MBUILD_ETCDIR
      MSchedSetAttr(S,msaHomeDir,(void **)MBUILD_ETCDIR,mdfString,mSet);
#else /* MBUILD_ETCDIR */
      MSchedSetAttr(S,msaHomeDir,(void **)"/opt/moab/",mdfString,mSet);
#endif  /* MBUILD_ETCDIR */

#ifdef MBUILD_VARDIR
      strcpy(S->VarDir,MBUILD_VARDIR "/");
#else /* MBUILD_VARDIR */
      strcpy(S->VarDir,"/opt/moab/");
#endif /* MBUILD_VARDIR */
      }  /* END if (HomeDirSet == FALSE) */
    }    /* END else */

  if ((ptr = getenv("PATH")) != NULL)
    {
    /* Initialize the PathList object */
    MUInitPathList(ptr);

    MUStrDup(&MSched.ServerPath,ptr);
    }    /* END if ((ptr = getenv("PATH")) != NULL) */
 
  if (S->CfgDir[strlen(S->CfgDir) - 1] != '/')
    strcat(S->CfgDir,"/");

  if (S->VarDir[0] == '\0')
    strcpy(S->VarDir,S->CfgDir);

  sprintf(tmpLine,"%s%s",
    S->VarDir,
    "spool");

  MSchedSetAttr(S,msaSpoolDir,(void **)tmpLine,mdfString,mSet);

  MSchedSetDefaults(S);

  MCP.CPInterval          = MDEF_CPINTERVAL;
  MCP.CPExpirationTime    = MDEF_CPEXPIRATIONTIME;
  MCP.CPJobExpirationTime = MDEF_CPJOBEXPIRATIONTIME;
 
  /* setup lock file */

  sprintf(S->LockFile,"%s.%s%s",
    S->VarDir,
    MSCHED_SNAME,
    MDEF_LOCKSUFFIX);
 
  MDB(5,fCONFIG) MLog("INFO:     default LockFile: '%s'\n",
    S->LockFile);
 
  /* setup checkpoint file */

  sprintf(MCP.CPFileName,"%s.%s%s",
    S->VarDir,
    MSCHED_SNAME,
    MDEF_CHECKPOINTSUFFIX);

  MDB(5,fCONFIG) MLog("INFO:     default CPFile: '%s'\n",
    MCP.CPFileName);
 
  /* setup log directory */
 
  if (!strstr(S->VarDir,MDEF_LOGDIR))
    {
    sprintf(S->LogDir,"%s%s",
      S->VarDir,
      MDEF_LOGDIR);
    }
  else
    {
    strcpy(S->LogDir,MDEF_LOGDIR);
    }

  /* setup tools directory */
 
#ifdef MBUILD_INSTDIR
  sprintf(S->InstDir,"%s/",
    MBUILD_INSTDIR);
#else
  sprintf(S->InstDir,"%s",
    MDEF_INSTDIR);
#endif /* MBUILD_INSTDIR */

  sprintf(S->ToolsDir,"%s%s",
    S->InstDir,
    MDEF_TOOLSDIR);

  /* setup config file */

  sprintf(S->ConfigFile,"%s%s%s",
    S->CfgDir,
    MSCHED_SNAME,
    MDEF_CONFIGSUFFIX);

  MDB(5,fCONFIG) MLog("INFO:     default ConfigFile: '%s'\n",
    S->ConfigFile);
 
  if ((ptr = getenv(MParam[mcoSchedConfigFile])) != NULL)
    {
    MDB(4,fCORE)
      {
      fprintf(stderr,"INFO:     using %s environment variable (value: %s)\n",
        MParam[mcoSchedConfigFile],
        ptr);
      }
 
    MUStrCpy(S->ConfigFile,ptr,sizeof(S->ConfigFile));
    }

  /* setup web services failover file */

  sprintf(S->WSFailOverFile,"%s/%s",
    S->SpoolDir,
    MSCHED_WSFAILOVER);

  sprintf(S->MsgErrHeader,"%s_ERROR",
    MSCHED_SNAME);

  sprintf(S->MsgInfoHeader,"%s_INFO",
    MSCHED_SNAME);

  /* initialize global variable space */

  MUHTCreate(&MSched.Variables,-1);
  MUHTCreate(&MDBHandles,-1);
  MUHTCreate(&MAQ,-1);

#ifdef __MCOMMTHREAD
  pthread_mutex_init(&MDBHandleHashLock,NULL);
#endif

  /* set up default scheduler values */
 
  S->GP = &MPar[0];

  /* set vpc defaults */

  S->VPCMigrationPolicy = mvpcmpNONE;

  S->JobMigratePolicy = mjmpAuto;

  S->IgnorePreempteePriority = FALSE;
  S->IgnoreLLMaxStarters = FALSE;

  S->DisableExcHList = FALSE;

  S->MDiagXmlJobNotes = FALSE;

  S->RMRetryTimeCap = MCONST_HOURLEN; /* Set default to 1 hour, RMRETRYTIMECAP to configure */

  if (getenv(MSCHED_ENVDBGVAR) != NULL)
    {
    S->DebugMode = TRUE;
    }

  if (getenv("MOABRECORDEVENTTOSYSLOG") != NULL)
    {
    S->RecordEventToSyslog = TRUE;
    }

  if (getenv("MOABRESUMEONFAILEDPREEMPT") != NULL)
    {
    S->ResumeOnFailedPreempt = TRUE;
    }

  MSched.DynamicRMFailureWaitTime = MDEF_DYNAMIC_RM_FAILURE_WAIT_TIME;

  MSched.NodeIdlePowerThreshold = MDEF_NODEIDLEPOWERTHRESHOLD;

  memset(MSched.LoadEndTime,0,sizeof(MSched.LoadEndTime));
  memset(MSched.LoadStartTime,0,sizeof(MSched.LoadStartTime));

  MSched.DataStageHoldType = mhDefer;

  MSched.RunVMOvercommitMigration = FALSE;
  MSched.RunVMConsolidationMigration = FALSE;

#ifdef MUSEWEBSERVICES
  MSched.WebServicesConfigured = TRUE;
#else
  MSched.WebServicesConfigured = FALSE;
#endif

  return(SUCCESS);
  }  /* END MSchedInitialize() */
/* END MSchedInitialize.c */
