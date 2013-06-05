/* HEADER */

      
/**
 * @file MSysMainLoop.c
 *
 * Contains: MSysMainLoop
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


/**
 */

int MSysStateReset(void)

  {
  /* NOTE: this routine will not be run before iteration 0 */

  /* NOTE: jobs may come in from both RM and UI (i.e. msub), so some
   * variables must be set accordingly here */

  return(SUCCESS);
  }  /* END MSysStateReset() */


/**
 * Checks the given mount point for free space and file nodes. If either of these
 * is low, a message is attached to the scheduler output and other actions may be taken
 * (scheduler paused, etc.)
 *
 * @see MSched.MountPointUsageThreshold
 * @see MSched.MountPointCheckInterval
 *
 * @param MountPoint (I)
 */

int MSysCheckMount(

  char *MountPoint)

  {
  /* defines to handle differences in operating systems: */
#if defined(__AIX) || defined(__FREEBSD)
 #define STRUCT_STATFS statfs
 #define STATFS(x,y) statfs((x),(y))
#else
 #define STRUCT_STATFS statvfs
 #define STATFS(x,y) statvfs((x),(y))
#endif /* defined(__AIX) || defined(__FREEBSD) */

  struct STRUCT_STATFS FSStat;
  int rc;
  char tmpLine[MMAX_LINE];
  mbool_t FoundProblem = FALSE;
  float PercentUsed = 0.0f;

  rc = STATFS(MountPoint,&FSStat);

  if (rc < 0)
    {
    /* error occurred */

    return(FAILURE);
    }

  PercentUsed = (1.0f - ((float)FSStat.f_ffree / (float)FSStat.f_files));

  if (PercentUsed >= MSched.MountPointUsageThreshold)
    {
    snprintf(tmpLine,sizeof(tmpLine),"number of free inodes on mount '%s' was too low (%.1f%% used > %.1f%% threshold)",
      MountPoint,
      PercentUsed * 100,
      MSched.MountPointUsageThreshold * 100);

    MMBAdd(
      &MSched.MB,
      tmpLine,
      (char *)"N/A",
      mmbtOther,
      0,
      1,
      NULL);

    FoundProblem = TRUE;
    }

  PercentUsed = (1.0f - ((float)FSStat.f_bavail / (float)FSStat.f_blocks));

  if (PercentUsed >= MSched.MountPointUsageThreshold)
    {
    snprintf(tmpLine,sizeof(tmpLine),"number of bytes available for mount '%s' was too low (%.1f%% used > %.1f%% threshold)",
      MountPoint,
      PercentUsed * 100,
      MSched.MountPointUsageThreshold * 100);

    MMBAdd(
      &MSched.MB,
      tmpLine,
      (char *)"N/A",
      mmbtOther,
      0,
      1,
      NULL);

    FoundProblem = TRUE;
    }

  if (FoundProblem == TRUE)
    {
    if (MSched.State == mssmRunning)
      {
      /* pause scheduler (but only if no one else has
       * already paused/stopped it) */

      MSchedSetState(mssmPaused,"[mountcheck]");
      }
    
    return(FAILURE);
    }
  
  return(SUCCESS);
  }  /* END MSysCheckMount() */


/**
 * Checks important mount points periodically. If any mount points are used up
 * beyond a certain threshold, emergency actions may be taken.
 *
 * WARNING: Not thread safe due to single static variable!
 *
 * @see MSysCheckMount()
 *
 * Checks following mount locations: Moab home dir, var dir, log dir, checkpoint dir,
 * and spool dir.
 */

int MSysCheckMountSpace()

  {
  static mulong LastCheckTime = 0;
  mbool_t AllMountsSuccessful = TRUE;

  /* MSysCheckMount checks each mount point and decides 
   * if any action needs to be taken! */

  if ((MSched.MountPointCheckInterval <= 0) || (MSched.MountPointUsageThreshold <= 0))
    {
    /* mount checks are disabled */

    return(SUCCESS);
    }

  if ((MSched.Time - LastCheckTime) < (mulong)MSched.MountPointCheckInterval)
    {
    /* too soon to check again */

    return(SUCCESS);
    }

  LastCheckTime = MSched.Time;

  /* note that CfgDir/VarDir and SpoolDir/CheckpointDir are
   * the same by default */

  /* NOTE: it doesn't appear that this is the case any more... AE */

  if (MSysCheckMount(MSched.CfgDir) == FAILURE)
    AllMountsSuccessful = FALSE;

  if (strcmp(MSched.CfgDir,MSched.VarDir))
    {
    if (MSysCheckMount(MSched.VarDir) == FAILURE)
      AllMountsSuccessful = FALSE;
    }

  if (MSysCheckMount(MSched.LogDir) == FAILURE)
    AllMountsSuccessful = FALSE;

  if (MSysCheckMount(MSched.ToolsDir) == FAILURE)
    AllMountsSuccessful = FALSE;

  if (MSysCheckMount(MSched.SpoolDir) == FAILURE)
    AllMountsSuccessful = FALSE;

  if (strcmp(MSched.SpoolDir,MSched.CheckpointDir))
    {
    if (MSysCheckMount(MSched.CheckpointDir) == FAILURE)
      AllMountsSuccessful = FALSE;
    }

  if (AllMountsSuccessful == TRUE)
    {
    /* unpause the scheduler if all mounts successfully checked out
     * and this function paused it earlier */

    if ((MSched.State == mssmPaused) &&
        !strcmp(MSched.StateMAdmin,"[mountcheck]"))
      {
      MSchedSetState(mssmRunning,"[mountcheck]");
      }
    }

  return(SUCCESS);
  }  /* END MSysCheckMountSpace() */


/**
 * 
 *
 */

void MSysUIChildAcceptRequests()
  {
  msocket_t tmpS;
  msocket_t tmpC;

  while (TRUE)
    {
    if (getppid() == 1)
      {
      /* if the parent went away then the child is inherited by the init process (1)
       * and in this case the child should exit.  */

#ifdef __UICHILDDEBUG
      sprintf(TestLogMessage,"child %d exiting because it was orphaned\n", MSched.ClientUIPort);
      ChildTestLog(TestLogMessage);

      fclose(fpTest);
#endif

      exit(0);
      }

    memset(&tmpS,0,sizeof(tmpS));
    tmpS.sd = MSched.ServerS.sd;

    memset(&tmpC,0,sizeof(tmpC));

    if (MSUAcceptClient(&tmpS,&tmpC,NULL) == FAILURE)
      {
      MUSleep(100000,TRUE);
      continue;
      }
                                        
    if (MSURecvData(&tmpC,MSched.SocketWaitTime,TRUE,NULL,NULL) == FAILURE)
      {
      close(tmpC.sd);

      MSUFree(&tmpC);
      MUFree((char **)&tmpC);

      continue;
      }

    if (/* valid command */TRUE)
      {
      if (MSysProcessRequest(&tmpC,tmpC.IsLoaded) == FAILURE)
        {
        }
      }

    MSUFree(&tmpC);
    MUFree((char **)&tmpC);
    } /* END while(TRUE) */

  return;
  }  /* END  MSysUIChildAcceptRequests */




/**
 * 
 *
 */

int MSysUIChildProcessing()
  {
  int rmindex = 0;
  char EMsg[MMAX_LINE];

#ifdef __UICHILDDEBUG
  char FileName[80];
#endif

  /* close files, sockets etc that were inherited from the parent */

  fclose(stdin);

  fclose(stdout);

  fclose(stderr);

#ifdef __UICHILDDEBUG
  sprintf(FileName,"%s%d",
    "/tmp/child",
    getpid());

  fpTest = fopen(FileName,"w+");

  sprintf(TestLogMessage,"child %d starting\n", getpid());
  ChildTestLog(TestLogMessage);
#endif

  /* reset signal handlers */

  signal(SIGUSR1,SIG_DFL);
  signal(SIGUSR2,SIG_DFL);
  signal(SIGHUP,SIG_DFL);
  signal(SIGTERM,SIG_DFL);
  signal(SIGQUIT,SIG_DFL);
  signal(SIGIO,SIG_DFL);
  signal(SIGURG,SIG_DFL);
  signal(SIGSEGV,SIG_DFL);
  signal(SIGBUS,SIG_DFL);
  signal(SIGFPE,SIG_DFL);
  signal(SIGILL,SIG_DFL);


  /* close connection to the log file */

  if (mlog.logfp != NULL)
    fclose(mlog.logfp);

  /* perhaps use a separate child log file? NYI */

  mlog.logfp = NULL;

  if (MSched.HALockFD >= 0)
    close(MSched.HALockFD);

  MSched.HALockFD = 0;

#if 0
  if (MSched.MDB.DBType != mdbNONE)
    {
    MDBFree(&MSched.MDB);
    }
#endif

#if 0
  /* close msub journal file */

  if (MSubJournal.FP != NULL)
    fclose(MSubJournal.FP);

  /* close checkpoint journal file */

  if (MCP.JournalFP != NULL)
    fclose(MCP.JournalFP);
#endif

  /* close event file */

  if (MStat.eventfp != NULL)
    fclose(MStat.eventfp);

  MStat.eventfp = NULL;

  /* close client sockets */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    if (MRM[rmindex].Name[0] == '\0')
      break;

    if ((MRM[rmindex].Type == mrmtNONE) || (MRM[rmindex].Name[0] == '\1'))
      continue;

    if (MRM[rmindex].PBS.SchedS.sd >= 0)
      close(MRM[rmindex].PBS.SchedS.sd);
    }  /* END for (rmindex) */

  /* close the user interface socket */

  if (MSched.ServerS.sd >= 0)
    close(MSched.ServerS.sd);

  /* flush any UI transactions */

  MUIProcessTransactions(TRUE,NULL);

  /* reopen a new user interface socket with a new port */

  MSUInitialize(
    &MSched.ServerS,
    NULL,                  /* remote host */
    MSched.ClientUIPort,   /* remote port */
    MSched.ClientTimeout);
           
  EMsg[0] = '\0';

  if (MSUListen(&MSched.ServerS,EMsg) == FAILURE)
    {

#ifdef __UICHILDDEBUG
    sprintf(TestLogMessage,"cannot open child user interface port %d (%s)\n",
      MSched.ClientUIPort,
      EMsg);

    ChildTestLog(TestLogMessage);
#endif

    return(FAILURE);
    }

  /* service user interface requests*/

  MSysUIChildAcceptRequests();

  if (MSched.ServerS.sd >= 0)
    close(MSched.ServerS.sd);

  return(SUCCESS);
  }




/**
 * Process calendar-based system updates
 *
 * @see MSysMainLoop() - parent
 *
 * NOTE:  updates the following systems:
 *  - checkpointing
 *  - fairshare
 *  - profile statistics
 *  - logging
 */

int MSysProcessEvents()

  {
  mfsc_t *FC;

  const char *FName = "MSysProcessEvents";

  MDB(2,fCORE) MLog("%s()\n",
    FName);

  FC = &MPar[0].FSC;

#ifdef MNOT  /* this is unsafe and unsupported code */
  /* reload config file if necessary */

  if (MSched.Reload == TRUE)
    {
    int OldIteration;
    int OldStopIteration;

    OldIteration     = MSched.Iteration;
    OldStopIteration = MSched.StopIteration;

    if (MCP.EnableTestWrite == TRUE) 
      {
      MCPCreate(MCP.CPFileName);

      MCP.LastCPTime = MSched.Time;
      }

    MSysInitialize(TRUE);

    MSysHandleSignals();

    MDB(1,fCONFIG) MLog("INFO:     reloading config files\n");

    if (MSysLoadConfig(MSched.ConfigFile,mcmForce) == FAILURE)
      {
        MDB(3,fALL) MLog("WARNING:  cannot locate configuration file '%s' in '%s' or in current directory\n",
          MSched.ConfigFile,
          MSched.CfgDir);
      }

    MStatProfInitialize(&MStat.P);

    if (FC->FSPolicy != mfspNONE)
      {
      MFSInitialize(FC);
      }

    if (MCP.EnableTestRead == TRUE)
      {
      MCPLoad(MCP.CPFileName,mckptRsv);
      }

    MSched.Iteration   = OldIteration;
    MSched.StopIteration = OldStopIteration;
    }  /* END if ((MSched.Reload == TRUE) && ...) */
#endif /* MNOT */

  /* update logs */

  MLogRoll(
    NULL,
    FALSE,
    MSched.Iteration,
    MSched.LogFileRollDepth);

  if ((MSched.Iteration > 0) && 
      (MSched.Iteration == MSched.OLogLevelIteration))
    {
    if (MSched.OLogFile != NULL)
      {
      MLogShutdown();

      MLogInitialize(MSched.OLogFile,-1,MSched.Iteration);

      MUFree(&MSched.OLogFile);
      }

    mlog.Threshold = MSched.OLogLevel;
    }

  if (((MSched.Time - MCP.LastCPTime) > MCP.CPInterval))
    {
    /* update CP */

    MCPCreate(MCP.CPFileName);

    MCP.LastCPTime = MSched.Time;

    /* prune completed jobs from the cache */

    MCachePruneCompletedJobs();

    /* update FS */

    if (FC->FSPolicy != mfspNONE)
      MFSUpdateData(FC,MSched.CurrentFSInterval,TRUE,FALSE,FALSE);
    }

  /* update fairshare */

  if ((FC->FSPolicy != mfspNONE) && (FC->FSInterval > 0))
    {
    long NewFSInterval;

    MFSTreeUpdateUsage();

    /* if FS interval reached */

    NewFSInterval = MSched.Time - (MSched.Time % FC->FSInterval);

    if (NewFSInterval != MSched.CurrentFSInterval)
      {
      if (MSched.CurrentFSInterval != 0)
        {
        MFSUpdateData(FC,MSched.CurrentFSInterval,FALSE,FALSE,TRUE);
        }

      MSched.CurrentFSInterval = NewFSInterval;

      MDB(1,fFS) MLog("INFO:     FS rolled to interval %ld\n",
        MSched.CurrentFSInterval);
      }
    }    /* END if ((FC->FSPolicy != mfspNONE) && ...) */

  /* Is this right?  resume time always takes you to running (if you were stopped or paused) */

  if (MSched.State != mssmRunning)
    {
    if ((MSched.ResumeTime > 0) && (MSched.ResumeTime < MSched.Time))
      {
      MSched.State      = mssmRunning;
      MSched.ResumeTime = 0;

      MDB(1,fSTRUCT) MLog("INFO:     scheduling re-enabled (resume time reached)\n");
      }
    }

  if (((MSched.RestartTime > 0) && (MSched.RestartTime <= MSched.Time)) ||
      ((MSched.RestartJobCount > 0) && (MSched.TotalJobQueryCount > MSched.RestartJobCount)))
    {
    MSched.Recycle  = TRUE;

    MCacheLockAll();

    MSched.Shutdown = TRUE;  /* NOTE:  must be set to preserve job ckpt records */

    MDB(1,fSTRUCT) MLog("INFO:     scheduler restart time reached (scheduler will restart)\n");

    MSysShutdown(0,-1);
    }
  else if ((MSched.Shutdown == TRUE) && (MSched.ShutdownSigno != 0))
    {
    /* a "shutdown signal" occurred and we are handling it asynchronously now */

    MSysShutdown(MSched.ShutdownSigno,0);
    }

  /* update reservations */

  MRsvAdjust(NULL,0,0,FALSE);

  MRsvDumpNL(NULL);

  MMBPurge(&MSched.MB,NULL,-1,MSched.Time);

  return(SUCCESS);
  }  /* END MSysProcessEvents() */



/**
 * Start high-level services.
 *
 */

int MSysStartServer()

  {
  char SubmitJournalFileName[MMAX_PATH];
  char DString[MMAX_LINE];

  const char *FName = "MSysStartServer";

  MDB(3,fALL) MLog("%s()\n",
    FName);

  MULToDString(&MSched.Time,DString);

  MDB(0,fALL) MLog("starting %s version %s (PID: %d) on %s",
    MOAB_VERSION,
    MSCHED_NAME,
    MSched.PID,
    DString);

  MSysMemCheck();

  /* set up user interface socket */

  if (MSched.ServerS.Version == 0)
    {
    char EMsg[MMAX_LINE];

    /* set up user interface socket */

    MSUInitialize(
      &MSched.ServerS,
      NULL,              /* remote host */
      MSched.ServerPort, /* remote port */
      MSched.ClientTimeout);
 
    if (MSUListen(&MSched.ServerS,EMsg) == FAILURE)
      {
      MDB(0,fALL) MLog("ERROR:    cannot open user interface socket on port %d (moab already running?)\n",
        MSched.ServerPort);

      if (EMsg[0] != '\0')
        {
        fprintf(stderr,"ERROR:    cannot open port %d (%s) - moab already running?\n",
          MSched.ServerPort,
          EMsg);
        }
      else
        {
        fprintf(stderr,"ERROR:    moab already running? (cannot open user interface socket on port %d)\n",
          MSched.ServerPort);
        }

      exit(1);
      }
    }    /* END if (MSched.ServerS.Version == 0) */

  /* open files shared by both threads */

  snprintf(SubmitJournalFileName,sizeof(SubmitJournalFileName),"%s/%s",
    MSched.CheckpointDir,
    ".msub.cp.journal");

  MCPSubmitJournalOpen(SubmitJournalFileName);

  /* setup peer interface socket */
  
  MSysStartCommThread();

  if (MSched.HTTPServerPort != 0)
    {
    /* enable extension interface */

    MSUInitialize(
      &MSched.ServerSH,
      NULL,
      MSched.HTTPServerPort,
      MSched.ClientTimeout);

    if (MSUListen(&MSched.ServerSH,NULL) == FAILURE)
      {
      MDB(7,fALL) MLog("ERROR:    cannot open extension interface socket on port %d\n",
        MSched.HTTPServerPort);
      }
    }    /* END if (MSched.HTTPServerPort != 0) */

  /* initialize accounting manager */

  if (MAM[0].Type != mamtNONE)
    {
    MAMInitialize(NULL);

    MAMActivate(&MAM[0]);
    }

  /* open events file */
  MStatPreInitialize(&MStat.P);

  /* load checkpoint records */
  if (MCPLoad(MCP.CPFileName,mckptRsv) == FAILURE)
    {
    char tmpLine[MMAX_LINE];

    MUStrCpy(tmpLine,MCP.CPFileName,sizeof(tmpLine));

    strcat(tmpLine,".1");

    if (MCPLoad(tmpLine,mckptRsv) == FAILURE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"cannot load checkpoint file");

      /* create SCHEDFAIL event (NYI) */

      MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedFailure,
        NULL,
        tmpLine,
        NULL);

      /* create empty checkpoint file */
 
      snprintf(tmpLine,sizeof(tmpLine),"%s",MCONST_CPTERMINATOR);

      MFUCreate(MCP.CPFileName,NULL,tmpLine,strlen(tmpLine) + 1,0600,-1,-1,TRUE,NULL);
      }
    }    /* END if (MCPLoad(MCP.CPFileName,mckptRsv) == FAILURE) */


  MSysGetLastEventID(&MSched);

  /* initialize resource manager interfaces */

  MRMInitialize(NULL,NULL,NULL);

  /* NOTE:  simulation may adjust stat timeframe */

  MStatInitialize();

  return(SUCCESS);
  }  /* END MSysStartServer() */




/**
 * Main primary scheduling loop.
 *
 * @see main - parent
 */

int MSysMainLoop()

  {
  int      OldDay;  /* day of week index */

  int      iindex;
  int      pindex;

  char     tmpLine[MMAX_LINE];
  char     DString[MMAX_LINE];

  double   Factor;

  pid_t    pid;

  const char *FName = "MSysMainLoop";

  MDB(5,fCORE) MLog("%s()\n",
    FName);

  MSysRegEvent("starting primary moab server",mactMail,1);

  if (MSched.UseSyslog == TRUE)
    {
    MOSSyslog(LOG_INFO,"starting primary moab server");
    }

  MSysStartServer();

  Factor = MPar[0].FSC.FSDecay;

  MPar[0].FSC.FSTreeConstant = 1.0;

  for (iindex = 0;iindex < MPar[0].FSC.FSDepth;iindex++)
    {
    MPar[0].FSC.FSTreeConstant += (1.0 * Factor);

    Factor *= MPar[0].FSC.FSDecay;
    }

  MOReportEvent((void *)&MSched,MSched.Name,mxoSched,mttStart,MSched.Time,TRUE);

  if ((MAM[0].Type != mamtNONE) && (MAM[0].UseDisaChargePolicy == TRUE))
    {
    MSchedGetLastCPTime();
    }

  while (TRUE)
    {
    mbool_t BreakEarly = FALSE;
    mbool_t InTransaction = FALSE;
    /* main scheduling loop */

    MSched.IntervalStart = MSched.Time;

    MStatInitializeActiveSysUsage();

    MSysCheckMountSpace();

    OldDay = MSched.Day;

    if ((MSched.MDB.DBType != mdbNONE))
      {
      if (MDBInTransaction(&MSched.MDB))
        {
        MDB(1,fCORE) MLog("ALERT:    Primary database handle is in a transaction "
            "state at the beginning of a scheduling iteration\n");
        /* transaction already started. Attempt to end it at the end of the iteration. */
        InTransaction = TRUE; 
        }
      else
        {
        /*Attempt to start a new transaction */
        InTransaction = (MDBBeginTransaction(&MSched.MDB,NULL) != FAILURE);
        }
      }

    /* Can this be moved to after MSchedProcessJobs()?
       when first starting up, no node information has been loaded and thus
       no nodes stats are written to the stats files */

    MServerUpdate();

    MStatPAdjust(); 

    snprintf(tmpLine,sizeof(tmpLine),"starting iteration %d - loglevel=%d  version=%s %s%s %s%s",
      MSched.Iteration,
      mlog.Threshold,
      MOAB_VERSION,
#ifdef MOAB_CHANGESET
      "changeset=",
      MOAB_CHANGESET,
#else /* MOAB_CHANGESET */
      "",
      "",
#endif /* MOAB_CHANGESET */
#ifdef MOAB_REVISION
      "revision=",
      MOAB_REVISION
#else /* MOAB_REVISION */
      "",
      ""
#endif /* MOAB_REVISION */
      );

#ifdef MDEBUGEVENTS
    MOWriteEvent((void *)&MSched,mxoSched,mrelNote,tmpLine,NULL,NULL);
#endif /* MDEBUGEVENTS */

    MDB(2,fALL) MLog("INFO:     %s\n",
      tmpLine);

    MUGetMS(NULL,&MSched.LoadStartTime[mschpTotal]);

    MSchedProcessJobs(OldDay,MGlobalSQ,MGlobalHQ);

    if (MSched.Iteration == 0)
      {
      MSched.ReadyForThreadedCommands = TRUE;
      }

    MQueueCheckStatus();

    MNodeCheckStatus(NULL);

    /* we can roll MSysCheck into MQueueCheckStatus...I believe */

    MSysCheck();

    MParUpdate(&MPar[0],FALSE,FALSE);

    HardPolicyRsvIsActive = FALSE;

    MRsvCheckStatus(NULL);

#if 0
    /* this routine is note fully implemented and should be turned
       on by parameter */

    MVPCCheckStatus(NULL);
#endif

    MDB(0,fSCHED)
      {
      /* give heartbeat */

      if (!(MSched.Iteration % 100) && (MSched.Mode == msmSim))
        {
        fprintf(mlog.logfp,".");

        fflush(mlog.logfp);
        }
      }

    MDB(1,fSCHED) MLog("INFO:     scheduling complete.  sleeping %ld seconds\n",
      MSched.MaxRMPollInterval);

    if ((MSched.Iteration > 0) && (MSched.Iteration == MSched.StopIteration))
      {
      /* NOTE:  enable schedctl {-s <X> | -S <X>} in normal mode */

      MSched.StopIteration = -1;

      MSched.State = mssmStopped;
      }

    if ((MSched.Iteration % MSched.MaxSleepIteration) == 0)
      {
      MSched.EnvChanged = TRUE;
      }
    else
      {
      MSched.EnvChanged = FALSE;
      }

    /* reset trigger timing */

    MSched.LoadStartTime[mschpTriggers] = 0;
    MSched.LoadEndTime[mschpTriggers] = 0;

    MSchedCheckTriggers(NULL,MSched.MaxTrigCheckSingleTime,&MSched.LastTrigIndex);

    /* update statistics */

    MNodeUpdateProfileStats(NULL);

    MSchedUpdateStats();

    MSysStateReset();

    MSysStoreNonBlockingStrings(); /* cache info for non-blocking calls */

    /* fork off a child moab process to handle user interface requests */

    if ((MSched.ForkIterations > 0) && 
        (MSched.Iteration % MSched.ForkIterations == 0))
      {
      int StatLoc;

      /* kill old child process if it exists */
      if (MSched.UIChildPid != 0)
        {
        int rc;

        MDB(7,fCORE) MLog("INFO:      killing child process %d\n",
          MSched.UIChildPid);

        rc = kill(MSched.UIChildPid,SIGKILL);

        if (rc != -1)
          {
          MDB(7,fCORE) MLog("INFO:      waiting for child process %d to die\n",
            MSched.UIChildPid);

          waitpid(MSched.UIChildPid,&StatLoc,0);
          }
        else
          {
          MDB(7,fCORE) MLog("INFO:      child process %d already dead\n",
            MSched.UIChildPid);
          }
        }

      if ((pid = fork()) == -1)
        {
        MDB(0,fCORE) MLog("ALERT:    cannot fork UI child process, errno: %d (%s)\n",
          errno,
          strerror(errno));
        }
      else if (pid == 0)
        {
        /* child context */

        /* allow the previous child process time to die before we attach to the socket */
        /* sleep(2); */

        MSysUIChildProcessing();

#ifdef __UICHILDDEBUG
        fclose(fpTest);
#endif

        if (MSched.ServerS.sd >= 0)
          close(MSched.ServerS.sd);

        exit(0);
        }
      else
        {
        /* parent context */

        MDB(7,fCORE) MLog("INFO:      forked new UI child process %d\n",
           pid);

        MSched.UIChildPid = pid;
        }
      } /* END if (UIChildForked ==FALSE) */

    MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedCycleEnd,
        NULL,
        NULL,
        NULL);

    CreateAndSendEventLog(meltSchedCycleEnd,MSched.Name,mxoSched,(void *)&MSched);

    MUIAcceptRequests(&MSched.ServerS,MSched.MaxRMPollInterval);

    MSysProcessEvents();

    memcpy(MUILastQ,MUIQ,sizeof(MUILastQ));
    MStat.LastQueuedPH = MStat.QueuedPH;

    MOQueueDestroy(MUIQ,TRUE);
    MOQueueDestroy(MGlobalSQ,FALSE);
    MOQueueDestroy(MGlobalHQ,FALSE);

    if ((MSched.Mode == msmSingleStep) && (MSched.StepCount <= 1))
      {
      MDB(4,fSCHED) MLog("INFO:     scheduler has completed single step scheduling\n");

      BreakEarly = TRUE;
      }
    else if ((MSched.Mode == msmSingleStep) && (MSched.Iteration >= MSched.StepCount))
      {
      MDB(4,fSCHED) MLog("INFO:     scheduler has completed multi-step scheduling (%d steps)\n",
        MSched.StepCount);

      BreakEarly = TRUE;
      }

    if (BreakEarly)
      {
      enum MStatusCodeEnum SC;

      if (InTransaction)
        MDBEndTransaction(&MSched.MDB,NULL,&SC);

      break;
      }

    fflush(mlog.logfp);

    MUGetMS(NULL,&MSched.LoadEndTime[mschpTotal]);

    /* TODO: put the below stats calculations into a separate funciton
     * to keep this loop trim and clean looking */

    /* reset trigger stats */

    MSched.TrigStats.NumAllTrigsEvaluatedLastIteration = MSched.TrigStats.NumAllTrigsEvaluatedThisIteration;
    MSched.TrigStats.NumTrigStartsLastIteration = MSched.TrigStats.NumTrigStartsThisIteration;
    MSched.TrigStats.NumTrigEvalsLastIteration = MSched.TrigStats.NumTrigEvalsThisIteration;
    MSched.TrigStats.NumTrigChecksLastIteration = MSched.TrigStats.NumTrigChecksThisIteration;
    MSched.TrigStats.NumTrigCheckInterruptsLastIteration = MSched.TrigStats.NumTrigCheckInterruptsThisIteration;

    MSched.TrigStats.TimeSpentEvaluatingTriggersLastIteration = MSched.LoadEndTime[mschpTriggers] - MSched.LoadStartTime[mschpTriggers];

    MSched.TrigStats.NumTrigStartsThisIteration = 0;
    MSched.TrigStats.NumTrigEvalsThisIteration = 0;
    MSched.TrigStats.NumTrigChecksThisIteration = 0;
    MSched.TrigStats.NumTrigCheckInterruptsThisIteration = 0;
    MSched.TrigStats.NumAllTrigsEvaluatedThisIteration = 0;

    /* adjust phase profiling stats */

    /* 24h */

    if (MSched.Time > MSched.Load24HStart + MCONST_HOURLEN)
      {
      /* roll intervals */

      for (iindex = MCONST_HOURSPERDAY - 1;iindex > 0;iindex--)
        {
        memcpy(
          MSched.Load24HDI[iindex],
          MSched.Load24HDI[iindex - 1],
          sizeof(MSched.Load24HDI[0]));
        }  /* END for (iindex) */

      memset(MSched.Load24HDI[0],0,sizeof(MSched.Load24HDI[0]));

      MSched.Load24HStart = MSched.Time;
      }  /* END if (MSched.Time > MSched.Load24HStart + MCONST_HOURLEN) */

    /* 5m */

    if (MSched.Time > MSched.Load5MStart + MCONST_MINUTELEN)
      {
      /* roll 1 minute interval for 5 minute sliding interval */

      for (iindex = 5 - 1;iindex > 0;iindex--)
        {
        memcpy(
          MSched.Load5MDI[iindex],      /* dest */
          MSched.Load5MDI[iindex - 1],  /* src */
          sizeof(MSched.Load5MDI[0]));
        }  /* END for (iindex) */

      /* clear current 1 minute interval located at index 0 */

      memset(MSched.Load5MDI[0],0,sizeof(MSched.Load5MDI[0]));

      MSched.Load5MStart = MSched.Time;
      }

    /* update all phases */

    for (pindex = 1;pindex < mschpLAST;pindex++)
      {
      /* TODO:  must track down possible underflow situation */

      MSched.Load24HDI[0][pindex] += 
        MAX(0,MSched.LoadEndTime[pindex] - MSched.LoadStartTime[pindex]);

      MSched.Load5MDI[0][pindex] += 
        MAX(0,MSched.LoadEndTime[pindex] - MSched.LoadStartTime[pindex]);

      MSched.LoadLastIteration[pindex] = MAX(0,MSched.LoadEndTime[pindex] - MSched.LoadStartTime[pindex]);

      if (MSched.LoadEndTime[pindex] - MSched.LoadStartTime[pindex] < 0)
        {
        MDB(2,fSCHED) MLog("ALERT:    current iteration load for %s is negative (underflow detected) %ld < %ld)\n",
          MSchedPhase[pindex],
          MSched.LoadEndTime[pindex],
          MSched.LoadStartTime[pindex]);
        }
      else
        {
        MDB(3,fSCHED) MLog("INFO:     current iteration load for %s: %ld\n",
          MSchedPhase[pindex],
          MAX(0,MSched.LoadEndTime[pindex] - MSched.LoadStartTime[pindex]));
        }
      }  /* END for (pindex) */

    /* calculate effective load */

    /* clear and recalculate 24 hour stats for each phase */

    memset(MSched.Load24HD,0,sizeof(MSched.Load24HD));

    for (iindex = MCONST_HOURSPERDAY - 1;iindex >= 0;iindex--)
      {
      for (pindex = 1;pindex < mschpLAST;pindex++)
        {
        MSched.Load24HD[pindex] += MSched.Load24HDI[iindex][pindex];
        }
      } 

    /* clear and recalculate 5 minute stats for each phase */

    memset(MSched.Load5MD,0,sizeof(MSched.Load5MD));

    for (iindex = 5 - 1;iindex >= 0;iindex--)
      {
      for (pindex = 1;pindex < mschpLAST;pindex++)
        {
        MSched.Load5MD[pindex] += MSched.Load5MDI[iindex][pindex];
        }
      }

    MSched.LoadLastIteration[mschpIdle] = 
      MSched.LoadLastIteration[mschpTotal] -
      MSched.LoadLastIteration[mschpSched] -
      MSched.LoadLastIteration[mschpRMAction] -
      MSched.LoadLastIteration[mschpRMLoad] -
      MSched.LoadLastIteration[mschpRMProcess] -
      MSched.LoadLastIteration[mschpTriggers] -
      MSched.LoadLastIteration[mschpUI];

    MSched.Load5MD[mschpIdle] = 
      MSched.Load5MD[mschpTotal] -
      MSched.Load5MD[mschpSched] -
      MSched.Load5MD[mschpRMAction] -
      MSched.Load5MD[mschpRMLoad] -
      MSched.Load5MD[mschpRMProcess] -
      MSched.Load5MD[mschpTriggers] -
      MSched.Load5MD[mschpUI];

    MSched.Load24HD[mschpIdle] =
      MSched.Load24HD[mschpTotal] -
      MSched.Load24HD[mschpSched] -
      MSched.Load24HD[mschpRMAction] -
      MSched.Load24HD[mschpRMLoad] -
      MSched.Load24HD[mschpRMProcess] -
      MSched.Load24HD[mschpTriggers] -
      MSched.Load24HD[mschpUI];

    if (InTransaction)
      {
      enum MStatusCodeEnum SC;

      MDBEndTransaction(&MSched.MDB,NULL,&SC);
      }

    /* Make sure that we've loaded in VM's, set up their reservations */

    if (MSched.Iteration == 1)
      {
      MVMSynchronizeStorageRsvs();

      MUFree((char **)&MSched.StartUpRacks);
      }
    }  /* END while (TRUE) */

  MULToDString(&MSched.Time,DString);

  MDB(1,fSCHED) MLog("INFO:     scheduling completed on %s\n",
    DString);

  exit(0);

  /*NOTREACHED*/

  return(0);
  }  /* END MSysMainLoop() */
/* END MSysMainLoop.c */
