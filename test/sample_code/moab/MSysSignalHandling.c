/* HEADER */

      
/**
 * @file MSysSignalHandling.c
 *
 * Contains: MSysSignalHandling
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

#if !defined(__SOLARIS)
#include <execinfo.h>
#endif


/**
 * This is a signal handler that leaves a backtrace of where the code was when
 * the signal was received.  
 * (See "man sigaction" for more details on error reasons/names)
 *
 * @param Signo   (I)
 * @param Siginfo (I)
 * @param Sigctx  (I)
 */


int MSysBacktraceHandler(

  int        Signo,    /* I */
  siginfo_t *Siginfo,  /* I */
  void      *Sigctx)   /* I */

  {
#if !defined(__SOLARIS)

#define MAX_BACKTRACE_DEPTH (32)
  void       *BackTrace[MAX_BACKTRACE_DEPTH]; 
  int         Depth;
  int         fd;
  char        Buf[MMAX_LINE];
  ssize_t     rc;
  
  fd = fileno(mlog.logfp);

  snprintf(Buf,MMAX_LINE, "----------Begin Moab Backtrace----------\n");
  rc = write(fd,Buf,strlen(Buf));
  rc = write(fd,MSched.MFullRevision,strlen(MSched.MFullRevision));
  snprintf(Buf,MMAX_LINE, "Backtrace timestamp: %s\n", MLogGetTime());
  rc = write(fd,Buf,strlen(Buf));

  snprintf(Buf,MMAX_LINE, "Caught Signal %d ", Signo);
  rc = write(fd,Buf,strlen(Buf));

  switch (Signo)
    {
    case SIGQUIT:
      snprintf(Buf,MMAX_LINE, "(SIGQUIT) Quit from keyboard\n");
      rc = write(fd,Buf,strlen(Buf));
      Buf[0] = '\0';  /* check generic si_codes below */
      break;


    case SIGILL:
      snprintf(Buf,MMAX_LINE, "(SIGILL) Illegal Instruction\n");
      rc = write(fd,Buf,strlen(Buf));
      switch (Siginfo->si_code)
        {
        case ILL_ILLOPC:
          snprintf(Buf,MMAX_LINE, "illegal opcode \n");
          break;

        case ILL_ILLOPN:
          snprintf(Buf,MMAX_LINE, "illegal operand \n");
          break;

        case ILL_ILLADR:
          snprintf(Buf,MMAX_LINE, "illegal addressing mode \n");
          break;

        case ILL_ILLTRP:
          snprintf(Buf,MMAX_LINE, "illegal trap \n");
          break;

        case ILL_PRVOPC:
          snprintf(Buf,MMAX_LINE, "privileged opcode \n");

          break;
        case ILL_PRVREG:
          snprintf(Buf,MMAX_LINE, "privileged register \n");
          break;

        case ILL_COPROC:
          snprintf(Buf,MMAX_LINE, "coprocessor error \n");
          break;

        case ILL_BADSTK:
          snprintf(Buf,MMAX_LINE, "internal stack error \n");
          break;

        default:
          Buf[0] = '\0';  /* check generic si_codes below */
          break;

        }
      rc = write(fd,Buf,strlen(Buf));
      break;

    case SIGFPE:
      snprintf(Buf,MMAX_LINE, "(SIGFPE) Floating point exception\n");
      rc = write(fd,Buf,strlen(Buf));
      switch (Siginfo->si_code)
        {
        case FPE_INTDIV:
          snprintf(Buf,MMAX_LINE, "integer divide by zero\n");
          break;

        case FPE_INTOVF:
          snprintf(Buf,MMAX_LINE, "integer overflow\n");
          break;

        case FPE_FLTDIV:
          snprintf(Buf,MMAX_LINE, "floating-point divide by zero\n");
          break;

        case FPE_FLTOVF:
          snprintf(Buf,MMAX_LINE, "floating-point overflow\n");
          break;

        case FPE_FLTUND:
          snprintf(Buf,MMAX_LINE, "floating-point underflow\n");
          break;

        case FPE_FLTRES:
          snprintf(Buf,MMAX_LINE, "floating-point inexact result\n");
          break;

        case FPE_FLTINV:
          snprintf(Buf,MMAX_LINE, "floating-point invalid operation\n");
          break;

        case FPE_FLTSUB:
          snprintf(Buf,MMAX_LINE, "subscript out of range\n");
          break;

        default:
          Buf[0] = '\0';  /* check generic si_codes below */
          break;
        }
      rc = write(fd,Buf,strlen(Buf));
      break;


    case SIGSEGV:
      snprintf(Buf,MMAX_LINE, "(SIGSEGV) Invalid memory reference\n");
      rc = write(fd,Buf,strlen(Buf));
      switch (Siginfo->si_code)
        {
        case SEGV_MAPERR:
          snprintf(Buf,MMAX_LINE, "address not mapped to object\n");
          break;

        case SEGV_ACCERR:
          snprintf(Buf,MMAX_LINE, "invalid permissions for mapped object\n");
          break;

        default:
          Buf[0] = '\0';  /* check generic si_codes below */
          break;
        }
      rc = write(fd,Buf,strlen(Buf));
      break;


    case SIGBUS:
      snprintf(Buf,MMAX_LINE, "(SIGBUS) Bus error (bad memory access)\n");
      rc = write(fd,Buf,strlen(Buf));
      switch (Siginfo->si_code)
        {
        case BUS_ADRALN:
          snprintf(Buf,MMAX_LINE, "invalid address alignment\n");
          break;

        case BUS_ADRERR:
          snprintf(Buf,MMAX_LINE, "nonexistent physical address\n");
          break;

        case BUS_OBJERR:
          snprintf(Buf,MMAX_LINE, "object-specific hardware error\n");
          break;

        default:
          Buf[0] = '\0';  /* check generic si_codes below */
          break;
        }
      rc = write(fd,Buf,strlen(Buf));
      break;

    default:
      /* We really shouldn't get here.  If we are handling a new signal 
       * a new case should be added above to deal with it. */

      snprintf(Buf,MMAX_LINE, "\n");
      rc = write(fd,Buf,strlen(Buf));
      break;
    }

  /* generic si_code could come from any signal.  Aggregate the code here... */
  if (Buf[0] == '\0') 
    {
    switch (Siginfo->si_code)
      {
      case SI_USER:
        snprintf(Buf,MMAX_LINE, "kill(2) or raise(3). Sent by pid:%d uid:%d\n", Siginfo->si_pid, Siginfo->si_uid);
        break;

      case SI_KERNEL:
        snprintf(Buf,MMAX_LINE, "Sent by the kernel.\n");
        break;

      case SI_QUEUE:
        snprintf(Buf,MMAX_LINE, "sigqueue(2)\n");
        break;

      case SI_TIMER:
        snprintf(Buf,MMAX_LINE, "POSIX timer expired\n");
        break;

      case SI_ASYNCIO:
        snprintf(Buf,MMAX_LINE, "AIO completed\n");
        break;

      case SI_MESGQ:
        snprintf(Buf,MMAX_LINE, "POSIX message queue state changed (since Linux 2.6.6); see mq_notify(3)\n");
        break;

      case SI_SIGIO:
        snprintf(Buf,MMAX_LINE, "queued SIGIO\n");
        break;

      case SI_TKILL:
        snprintf(Buf,MMAX_LINE, "tkill(2) or tgkill(2) (since Linux 2.4.19)\n");
        break;

      default:
        snprintf(Buf,MMAX_LINE, "unspecified si_code error %d\n",Siginfo->si_code);
        break;
      }
      rc = write(fd,Buf,strlen(Buf));
    }

  Depth = backtrace(BackTrace, MAX_BACKTRACE_DEPTH);

  backtrace_symbols_fd(BackTrace, Depth, fd);

  snprintf(Buf,MMAX_LINE, "----------End Moab Backtrace----------\n");
  rc = write(fd,Buf,strlen(Buf));

  /* resubmit the signal in order to produce a core dump */
  /* which implies that you shouldn't register this function as a handler for SIGABRT...*/
  abort();

  /* stupid thing to avoid compile warning about "set but not used" */
  MDB(9,fALL) snprintf(Buf,MMAX_LINE,"%ld\n",rc);

  return SUCCESS;  /* never reached but non-void function must return a value. */
#else
  abort();
#endif
  }  /* END MSysBacktraceHandler() */



/**
 * Loads all configuration related to the SCHEDCFG[] parameter.
 *
 * @param Signo (I)
 * @param ECode (I)
 */

void MSysAbort(

  int Signo,  /* I */
  int ECode)  /* I */

  {
  char tmpLine[MMAX_LINE];

  /* log event and generate stack trace */

  MOWriteEvent((void *)&MSched,mxoSched,mrelSchedFailure,NULL,MStat.eventfp,NULL);

  sprintf(tmpLine,"WARNING:  Moab is aborting.\n");

  MSysRegEvent(tmpLine,mactMail,1);

  abort();
  }  /* END MSysAbort() */




/**
 * MSysShutdown() should no longer be called as the result of any signals, 
 * these instead will come directly to this function which will set flags 
 * that will lead to MSysShutdown() being called in a more sychronous, 
 * controlled manner that avoids deadlocks & other errors.
 * 
 * @see MSysShutdown()
 *
 * @param Signo (I) received signal initiating shutdown
 */

void MSysInitiateShutdown( 

  int Signo)

  {
  MCacheLockAll();

  MSched.Shutdown = TRUE;
  MSched.ShutdownSigno = Signo;
  }  /* END MSysInitiateShutdown() */





/**
 * This function will shutdown Moab, freeing structures, closing sockets, etc.
 * At one time this function WAS called directly as a signal handler, but this is much
 * too dangerous, as the function is not thread-safe nor is it re-entrant safe. If we
 * interrupt the scheduling thread during a critical time, it could cause shutdown to
 * crash or even hang.
 *
 * A signal resulting in shutdown now calls MSysInitiateShutdown().
 *
 * For the time being we will leave the code in this function which disables logging. This
 * code was important when this routine could be called as a signal handler asynchronously,
 * but this is probably no longer needed.
 *
 * IMPORTANT NOTE:  If MSysShutdown() is called as the result of a signal (see
 * MSysInitiateShutdown()), logging will be immediately disabled and the shutdown
 * process will not be logged.
 *
 * @param Signo (I) received signal initiating shutdown
 * @param ECode (I) [-1 for recycle]
 */

void MSysShutdown(

  int Signo,  /* I received signal initiating shutdown */
  int ECode)  /* I exit code (-1 for recycle) */

  {
  static mbool_t DoFree = TRUE;

  char                 tmpBuf[MMAX_NAME];
  char                *GEName;
  mgevent_desc_t      *GEventDesc;
  mgevent_iter_t       GEIter;

  const char *FName = "MSysShutdown";

  if (Signo != 0)
    {
    /* this is being called as a signal-handler */
    /* set mlog.logfp to NULL to disable logging and avoid locking */
    
    /* (This is proably no longer needed since we switched over to
     * synchronous handling of signals.) */
    mlog.logfp = NULL;
    }

  MDB(2,fALL) MLog("%s(%d,%d)\n",
    FName,
    Signo,
    ECode);

  if (Signo == 0)
    {
    MDB(0,fALL) MLog("INFO:     admin shutdown request received.  shutting down server\n");
    }
  else
    {
    MDB(0,fALL) MLog("INFO:     received signal %d.  shutting down server\n",
      Signo);
    }

  MSched.RestartState = MSched.State;

  MSched.State = mssmShutdown;

  MOReportEvent((void *)&MSched,MSched.Name,mxoSched,mttEnd,MSched.Time,TRUE);

  MSUDisconnect(&MSched.ServerS);  /* disconnect socket early to prevent any deadlocks in trigger actions */

  /* need to finish up all currently running triggers before we shutdown */

  MSchedCheckTriggers(NULL,-1,NULL);

#ifdef MYAHOO
  while (TrigPIDs.NumItems > 0)
    {
    MDB(3,fSCHED) MLog("INFO:    there are still %d fork'd trigger children running--waiting to harvest\n",
      TrigPIDs.NumItems);

    MSchedCheckTriggers(NULL,-1,NULL);

    MUSleep(1000,FALSE);  /* keep from burning CPU */
    }
#endif /* MYAHOO */

  if (Signo != 0)
    ECode = 1;

  MAMShutdown(&MAM[0]);

  MFSShutdown(&MPar[0].FSC);

  MRMStop(NULL,NULL,NULL);

  /* deconstruct the socket and jobsubmit queues */
  MSysSocketQueueFree();
  MSysJobSubmitQueueFree();

  /* Do we need to free this?  I don't think it would be lost.
  MUDLListFree(MOQueue);
  */

  sprintf(tmpBuf,"%d",
    Signo);

  MSysRecordEvent(
    mxoSched,
    MSched.Name,
    mrelSchedStop,
    NULL,
    tmpBuf,
    NULL);

  if (MSched.PushEventsToWebService == TRUE)
    {
    MEventLog *Log = new MEventLog(meltSchedStop);
    Log->SetCategory(melcStop);
    Log->SetFacility(melfScheduler);
    Log->SetPrimaryObject(MSched.Name,mxoSched,(void *)&MSched);
    Log->AddDetail("signal",tmpBuf);

    MEventLogExportToWebServices(Log);

    delete Log;
    }

  MCPCreate(MCP.CPFileName);  /* updates checkpoint records */

  if (MSched.OptimizedCheckpointing == TRUE)
    {
    /* Need to checkpoint things that aren't checkpointed in MCPCreate */

    MCPStoreVPCList(&MCP,&MVPC);
    }

  MCPJournalClose();
  MCPSubmitJournalClose();

  /* handled in MFUCacheInitialize (all cases?) */
  /* MUFree(&MCP.OBuffer); */

  /* Walk the MGEvent Desc table, freeing items */
  MGEventIterInit(&GEIter);

  while(MGEventDescIterate(&GEName,&GEventDesc,&GEIter) == SUCCESS)
    {
    int Action;

    MUFree(&GEventDesc->Name);

    for (Action = mgeaNONE;Action < mgeaLAST;Action++)
      {
      MUFree(&GEventDesc->GEventActionDetails[Action]);
      }
    }

  /* Clean up the MGEvent Desc table */
  MGEventFreeDesc(TRUE,MUFREE);

  MUHTFree(&MSched.Variables,TRUE,MUFREE);
  MUHTFree(&TrigPIDs,TRUE,MUFREE);
  MVMHashFree(&MSched.VMTable);

  MStatShutdown();

  if (DoFree == TRUE)
    {
    int freeCount;
    char                  *GEName;
    mgevent_desc_t        *GEventDesc;
    mgevent_iter_t         GEIter;

    /* NOTE:  under some circumstances, memory free may fail causing sighandler 
              to re-start MSysShutdown() */

    DoFree = FALSE;

    MQOSFreeTable();
    MJobFreeTable();
    MCJobFreeTable();

    MVCFreeAll();
    MNLProvisionFreeBuckets();
    MRsvFreeTable();
    MCredFreeTable(&MUserHT);
    MCredFreeTable(&MGroupHT);
    MCredFreeTable(&MAcctHT);
    MSRFreeTable();
    MNodeFreeAResInfoAvailTable();
    MParFreeTable();
    MFUCacheInitialize(&MSched.Time);
    MSMPNodeDestroyAll(); /* Remove SMPNodes array */

    /* destroy non-blocking buffer by calling its destructor */

    NonBlockMDiagS.~mstring_t();

    MXMLDestroyE(&NonBlockShowqN);

    /* Free job templates */
    for (freeCount = 0;MUArrayListGet(&MTJob,freeCount) != NULL;freeCount++)
      {
      mjob_t **JP = (mjob_t **)(MUArrayListGet(&MTJob,freeCount));
      MJobDestroy(JP);
      MUFree((char **)JP);
      }

    /* WHY DO IT AGAIN?   Free All Desc GEvents */
    
    MGEventIterInit(&GEIter);
    while (MGEventDescIterate(&GEName,&GEventDesc,&GEIter) == SUCCESS)
      {
      MUFree((char **)&GEventDesc->Name);
      }

    /* Clear the MGEvent Desc table */
    MGEventFreeDesc(FALSE,NULL);


    MUArrayListFree(&MTJob);
    }  /* END if (DoFree == TRUE) */

  if (MSched.MDB.DBType != mdbNONE)
    {
    MDBFree(&MSched.MDB);
    }

  /* may still free MUREToList cache, MRsvAdjustDRes cache, MNodeBuildRE cache,
     MNat cache */

  /* closing of lock file MUST happen before restart! */

  if (!MUStrIsEmpty(MSched.HALockFile))
    {
    char tmpFile[MMAX_PATH_LEN];

    MUStrCpy(tmpFile,MSched.HALockFile,sizeof(tmpFile));
    MSched.HALockFile[0] = '\0';  /* empty so that update thread
                                     doesn't restart immediately when
                                     it sees file is deleted */
    remove(tmpFile);
    }

  if (MSched.LockFD > -1)
    close(MSched.LockFD);

  if (ECode < 0)
    {
    MSysRestart(Signo);
    }

  /* Dump any memory info that has not yet been freed. This becomes a memory leak reporter */
  MUMemReport(TRUE,NULL,0);

  if (MSched.ExitMsg != NULL)
    fprintf(stderr,"%s",MSched.ExitMsg);

  /* kill child user interface process if there is one */

  if (MSched.UIChildPid != 0)
    {
    kill(MSched.UIChildPid,9);
    }

  /* kill child user interface process if there is one */

  if (MSched.UIChildPid != 0)
    {
    kill(MSched.UIChildPid,SIGKILL);
    }

  MLogShutdown();

#ifdef __MCOMMTHREAD
  pthread_mutex_destroy(&MDBHandleHashLock);

  MTPDestroyThreadPool();
#endif

  /* destruct any objects in the MGlobals.c file */
  MGlobalVariablesDestructor();

  /* clean up database stuctures */
  MUHTClear(&MDBHandles,TRUE,(mfree_t)MDBFreeDPtr);

  exit(ECode);
  }  /* END MSysShutdown() */



/**
 * Restart Moab server.
 *
 * @see MSysShutdown()
 *
 * @param Signo (I)
 */

int MSysRestart(

  int Signo)  /* I */

  {
  int  rc;

  char FullCmd[MMAX_LINE];

  const char *FName = "MSysRestart";

  if (Signo != 0)
    {
    /* this is being called as a signal-handler */
    /* set mlog.fp to NULL to disabled logging and avoid locking */
    
    mlog.logfp = NULL;   
    }

  MDB(1,fALL) MLog("%s(%d)\n",
    FName,
    Signo);

  MOChangeDir(MSched.LaunchDir,NULL,FALSE);

  if (MFUGetFullPath(
       MSched.ArgV[0],
       MUGetPathList(),
       NULL,
       MSched.LaunchDir,
       TRUE,
       FullCmd,
       sizeof(FullCmd)) == FAILURE)
    {
    MDB(1,fALL) MLog("ALERT:    cannot locate full path for '%s'\n",
      MSched.ArgV[0]);

    exit(1);
    }

  MDB(1,fALL) MLog("INFO:     about to exec '%s'\n",
    FullCmd);

  MSysRecordEvent(
    mxoSched,
    MSched.Name,
    mrelSchedRecycle,
    NULL,
    (char *)((Signo == 11) ? "sigsegv" : "admin"),
    NULL);

  CreateAndSendEventLogWithMsg(meltSchedRecycle,MSched.Name,mxoSched,(void *)&MSched,(char *)((Signo == 11) ? "sigsegv" : "admin"));

  MLogShutdown();

  if ((MSched.RecycleArgs != NULL) && !strcasecmp(MSched.RecycleArgs,"savestate"))
    {
    MArgListAdd(MSched.ArgV, "-s", NULL);
    }
  else
    {
    MArgListRemove(MSched.ArgV, "-s", FALSE);
    }

  if ((rc = execv(FullCmd,MSched.ArgV)) == -1)
    {
    /* exec failed */

    MDB(1,fCORE) MLog("ERROR:    cannot restart scheduler '%s' rc: %d\n",
      FullCmd,
      rc);

    exit(1);
    }

  /*NOTREACHED*/

  exit(0);

  return(SUCCESS);
  }  /* END MSysRestart() */



/**
 * Registers the given function to be called for the given signal.
 *
 * @param Signo (I) The signal to modify.
 * @param Fn (I) The function to register.
 */

int MSysSetSignal(

  int   Signo,  /* I */
  void *Fn)     /* I */

  {
  if (Signo < 0)
    {
    return(FAILURE);
    }

  signal(Signo,(void(*)(int))Fn);

  return(SUCCESS);
  }  /* END MSysSetSignal() */





/**
 *
 *
 * @param Signo (I)
 */

int MSysRecover(

  int Signo)  /* I */

  {
  long   Time;
  time_t tmpTime;

  char Line[MMAX_LINE];
  char DString[MMAX_LINE];

  const char *FName = "MSysRecover";

  MDB(1,fALL) MLog("%s(%d)\n",
    FName,
    Signo);

#ifdef SIGXFSZ

  if (Signo == SIGXFSZ)
    {
    /* file too large, roll logs */

    MLogRoll(
      NULL,
      FALSE,
      MSched.Iteration,
      MSched.LogFileRollDepth);

    MSysSetSignal(Signo,(void *)MSysRecover);

    return(SUCCESS);
    }

#endif /* SIGXFSZ */

  MSched.State = mssmStopped;

  MSched.InRecovery = TRUE;

  time(&tmpTime);
  Time = (long)tmpTime;

  MULToDString((mulong *)&Time,DString);

  MDB(1,fALL) MLog("Recovery Mode:  failure occurred on iteration %d (loglevel=%d) time: %s\n",
    MSched.Iteration,
    mlog.Threshold,
    DString);

  mlog.Threshold = MMAX_LOGLEVEL;

  if (MSched.Mode == msmNormal)
    {
    MOSSyslog(LOG_ERR,"ERROR:  %s has failed, signal=%d",
      MSCHED_SNAME,
      Signo);
    }

  sprintf(Line,"scheduler has failed.  (Signo: %d)",
    Signo);

  MSysRecordEvent(mxoSched,MSched.Name,mrelSchedFailure,NULL,Line,NULL);

  MSysRegEvent(Line,mactNONE,1);

  fflush(mlog.logfp);

  /* save log file */

  MLogRoll(
    ".failure",
    TRUE,
    MSched.Iteration,
    MSched.LogFileRollDepth);

  fprintf(stderr,"ALERT:    created failure log file\n");

  sleep(2);

  MDB(1,fALL) MLog("RECOVER:  attempting to read socket connection\n");

  MUIAcceptRequests(
    &MSched.ServerS,
    MSched.MaxRMPollInterval);

  if ((Signo == SIGSEGV) || (Signo == SIGILL))
    {
    MSysSetSignal(Signo,(void *)MSysRecover);
    }

  MDB(0,fALL) MLog("INFO:     exiting recovery mode\n");

  return(SUCCESS);
  }  /* END MSysRecover() */


/**
 * Apply policies to set initial signal handlers.
 *
 */

int MSysHandleSignals()

  {
  char *RecoveryList;
  char *IgnList;

  enum MRecoveryActionEnum tmpA;

  /* set default signal handlers */

  IgnList = getenv("MOABIGNSIGNAL");

  signal(SIGTTIN,SIG_IGN);

  signal(SIGINT,SIG_IGN);

  #if defined(SIGRTMIN)
  signal(SIGRTMIN,SIG_IGN);
  #endif /* SIGRTMIN */

  if (getenv("MOABABORTONTERM"))
    signal(SIGTERM,(void(*)(int))MSysAbort);
  else if ((IgnList != NULL) && ((strstr(IgnList,"TERM") || strstr(IgnList,"term"))))
    signal(SIGTERM,SIG_IGN);
  else
    signal(SIGTERM,(void(*)(int))MSysInitiateShutdown);

  signal(SIGQUIT,(void(*)(int))MSysInitiateShutdown);
  signal(SIGIO,(void(*)(int))MSysInitiateShutdown);
  signal(SIGURG,(void(*)(int))MSysInitiateShutdown);

#ifdef __MCRAY
  signal(SIGHUP,(void(*)(int))MSysRestart);
#endif /* __MCRAY */

  signal(SIGHUP,(void(*)(int))SIG_IGN);
  signal(SIGPIPE,(void(*)(int))SIG_IGN);

#ifdef SIGXFSZ
  MSysSetSignal(SIGXFSZ,(void *)MSysRecover);
#endif /* SIGXFSZ */

  if ((RecoveryList = getenv(MSCHED_ENVRECOVERYVAR)) != NULL)
    {
    tmpA = (enum MRecoveryActionEnum)MUGetIndexCI(
      RecoveryList,
      MRecoveryAction,
      FALSE,
      mrecaRestart);
    }
  else
    {
    tmpA = MSched.RecoveryAction;
    }

  switch (tmpA)
    {
    case mrecaIgnore:

      signal(SIGSEGV,(void(*)(int))SIG_IGN);
      signal(SIGILL,(void(*)(int))SIG_IGN);

      break;

    case mrecaRestart:

      /* restart Moab */

      signal(SIGSEGV,(void(*)(int))MSysRestart);
      signal(SIGILL,(void(*)(int))MSysRestart);
      signal(SIGBUS,(void(*)(int))MSysRestart);
      signal(SIGFPE,(void(*)(int))MSysRestart);

      break;

    case mrecaTrap:

      signal(SIGSEGV,(void(*)(int))MSysRecover);
      signal(SIGILL,(void(*)(int))MSysRecover);
   
      break;

    case mrecaDie:
    default:

      {
      /* save backtrace and core dump */

      struct rlimit rlimit;
      struct sigaction sa;

      rlimit.rlim_cur = RLIM_INFINITY;
      rlimit.rlim_max = RLIM_INFINITY;

      setrlimit(RLIMIT_CORE,&rlimit);

      sigemptyset (&sa.sa_mask);
      sa.sa_flags = SA_SIGINFO;
      sa.sa_sigaction = (void (*)(int, siginfo_t*, void*))MSysBacktraceHandler;

      sigaction(SIGQUIT,&sa,NULL);
      sigaction(SIGILL,&sa,NULL);
      sigaction(SIGFPE,&sa,NULL);
      sigaction(SIGSEGV,&sa,NULL);
      sigaction(SIGBUS,&sa,NULL);
      }

      break;
    }  /* END switch (tmpA) */
 
  return(SUCCESS);
  }  /* END MSysHandleSignals() */



/* END MSysSignalHandling.c */
