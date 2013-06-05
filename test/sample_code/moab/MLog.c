/* HEADER */

/**
 * @file MLog.c
 *
 * Moab Logs
 */
        
/* Contains:                                    *
 *                                              *
 * #define MDB(X,F)                             *
 * int MLog(Message,...)                        *
 * int MLogInitialize(NewLogFile,NewMaxFileSize,Iteration) *
 * int MLogOpen(Iteration)                      *
 * int MLogRoll(Suffix,DoForce,Iteration,Depth) *
 * char *MLogGetTime()                          *
 * void MLogLevelAdjust(signo)                  *
 *                                              */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"
#include "moab-global.h"  


#define MAX_LOGNAME  256
#define MAX_MDEBUG    10

#ifndef SUCCESS
#define SUCCESS        1
#endif

#ifndef FAILURE
#define FAILURE        0
#endif


mlog_t mlog;

mmutex_t MLogMutex;

char  LogFile[MAX_LOGNAME + 1];  /* global name of Moab Log */
int   MaxLogFileSize = -1;

char *MLogGetTime(void);
void  MLogLevelAdjust(int);
int   MLogClose(void);



/**
 * Report current log file name.
 *
 * @param LogFileP (O) [alloc]
 */

int MLogGetName(

  char **LogFileP)  /* O (alloc) */

  {
  if (LogFileP != NULL)
    MUStrDup(LogFileP,LogFile);

  return(SUCCESS);
  }  /* END MLogGetName() */





/**
 * Initialize log file and structure.
 *
 * @see MLogOpen() - child
 *
 * @param NewLogFile (I) [optional]
 * @param NewMaxFileSize (I) [optional, -1->do not change]
 * @param Iteration (I)
 */

int MLogInitialize(

  const char *NewLogFile,
  int         NewMaxFileSize,
  int         Iteration)

  {
  static int SigSet = 0;

#ifdef MTHREADSAFE
  pthread_mutex_init(&MLogMutex,NULL);
#endif /* MTHREADSAFE */

  if (NewMaxFileSize != -1)
    MaxLogFileSize = NewMaxFileSize;

  if (NewLogFile == NULL)
    {
    LogFile[0] = '\0';

    MLogOpen(Iteration);
    }
  else if ((strcmp(LogFile,"N/A") != 0) &&
      (strcmp(NewLogFile,"N/A") != 0) &&
      (strcmp(LogFile,NewLogFile) != 0))
    {
    strncpy(LogFile,NewLogFile,MAX_LOGNAME);
    LogFile[MAX_LOGNAME] = '\0';

    MLogOpen(Iteration);
    }

  if (SigSet == 0)
    {
    signal(SIGUSR1,(void(*)(int))MLogLevelAdjust);
    signal(SIGUSR2,(void(*)(int))MLogLevelAdjust);

    SigSet = 1;
    }

  /* Initialize the memory portion of the mlog structure */
  mlog.MemoryTracker = MUMemoryInitialize();

  return(SUCCESS);
  }  /* END MLogInitialize() */ 





/**
 * Open logfile and store log file handle.
 *
 * @param Iteration (I)
 */

int MLogOpen(

  int   Iteration) /* I */

  {
  mbool_t IsStatSync = FALSE;

  int fd;
  int flags;

  int oldMask;

  const char *FName = "MLogOpen";

  MDB(4,fCORE) MLog("%s(%d)\n",
    FName,
    Iteration);

  if ((mlog.logfp != NULL) && 
      (mlog.logfp != stderr))
    {
    if (mlog.logfp == MStat.eventfp)
      IsStatSync = TRUE;

    MLogClose();
    }

  /* if null logfile, send logs to stderr */

  if ((LogFile == NULL) || (LogFile[0] == '\0'))
    {
    mlog.logfp = stderr;

    if (IsStatSync == TRUE)
      MStat.eventfp = mlog.logfp;

    return(SUCCESS);
    }

  if ((getenv(MSCHED_ENVLOGSTDERRVAR) != NULL) && (Iteration == 0))
    {
    fclose(stderr);
    }

  oldMask = umask(0022); /* save mask and restore it */

  if (umask(0022) != 0022)
    {
    MDB(2,fCORE) MLog("ERROR:    cannot set umask on logfile '%s', errno: %d (%s)\n",
      LogFile,
      errno,
      strerror(errno));
    }

  if ((mlog.logfp = fopen(LogFile,"a+")) == NULL)
    {
    MDB(0,fALL) fprintf(stderr,"WARNING:  cannot open log file '%s', errno: %d (%s) - logging to terminal\n",
      LogFile,
      errno,
      strerror(errno));

    /* dump logs to stderr */

    mlog.logfp = stderr;

    if (IsStatSync == TRUE)
      MStat.eventfp = mlog.logfp;

    umask(oldMask); /* set mask to what is was */

    return(FAILURE);
    }

  if (MSched.LogPermissions != 0)
    chmod(LogFile,MSched.LogPermissions);

  umask(oldMask); /* set mask to what is was */

  fd = fileno(mlog.logfp);

  flags = fcntl(fd,F_GETFD,0);

  if (flags >= 0)
    {
    flags |= FD_CLOEXEC;

    fcntl(fd,F_SETFD,flags);
    }

  if (IsStatSync == TRUE)
    MStat.eventfp = mlog.logfp;

  /* use smallest of line and 4K buffering */

  /* make configurable */

  if (!getenv("MOABENABLELOGBUFFERING"))
    {
    if (setvbuf(mlog.logfp,NULL,_IOLBF,4097) != 0)
      {
      fprintf(stderr,"WARING:  cannot setup line mode buffering on logfile\n");
      }
    }

  mlog.State = mlsOpen;

  return(SUCCESS);
  }  /* END MLogOpen() */





/**
 * Close logfile and clear log file handle.
 */

int MLogClose()

  {
  /* overcomes problems with signal handlers shutting down logs */

  if ((mlog.logfp == NULL) ||
      (mlog.State == mlsClosed))
    {
    return(SUCCESS);
    }

  MUMutexLockSilent(&MLogMutex);

  mlog.State = mlsClosed;

  fclose(mlog.logfp);

  mlog.logfp = NULL;

  MUMutexUnlockSilent(&MLogMutex);

  return(SUCCESS);
  }  /* END MLogClose() */





/**
 * Shutdown logging facility and close logfile.
 */

int MLogShutdown()

  {
  if (mlog.logfp != NULL)
    {
    MLogClose();
    }

  return(SUCCESS);
  }  /* END MLogShutdown() */




#ifndef __MTEST

/**
 * Write log message to logfile with timestamp header.
 *
 * NOTE: Must remain thread-safe!
 *
 * @param Format (I)
 */

int MLog(

  const char *Format,   /* I */
  ...)

  {
  va_list Args;

  /* overcomes problems with signal handlers logging */

  if ((mlog.logfp == NULL) ||
      (mlog.State == mlsClosed))
    {
    return(SUCCESS);
    }

  MUMutexLockSilent(&MLogMutex);
  
  if ((mlog.logfp != NULL) && (mlog.State != mlsClosed))
    {
    if (mlog.logfp != stderr)
      {
      fprintf(mlog.logfp,"%s ",
        MLogGetTime());
      }

#if defined(MTHREADSAFE) && defined(MLOGTHREADID)
    fprintf(mlog.logfp,"%u ",
      (MUINT4)MUGetThreadID());
#endif /* MCOMMTHREAD && MLOGTHREADID */

    va_start(Args,Format);

    vfprintf(mlog.logfp,Format,
      Args);

    va_end(Args);
    }  /* END if ((mlog.logfp != NULL) && (mlog.State != mlsClosed)) */

  MUMutexUnlockSilent(&MLogMutex);

  return(SUCCESS);
  }  /* END MLog() */

#endif /* __MTEST */






/**
 * Write log message to logfile without timestamp header.
 *
 * @param Format (I)
 */

int MLogNH(

  const char *Format,
  ...)

  {
  va_list Args;

  /* NOTE: NYI support static buffer for caching failed log entries */

  /* overcomes problems with signal handlers logging */

  if ((mlog.logfp == NULL) ||
      (mlog.State == mlsClosed))
    {
    return(SUCCESS);
    }

  MUMutexLockSilent(&MLogMutex);

  if ((mlog.logfp != NULL) && (mlog.State != mlsClosed))
    {
    va_start(Args,Format);

    vfprintf(mlog.logfp,Format,
      Args);

    va_end(Args);
    }  /* END if ((mlog.logfp != NULL) && (mlog.State != mlsClosed)) */

  MUMutexUnlockSilent(&MLogMutex);

  return(SUCCESS);
  }  /* END MLogNH() */





/**
 * Rotate/roll logfiles.
 *
 * @param Suffix (I) [optional]
 * @param DoForce (I)
 * @param Iteration (I)
 * @param Depth (I)
 */

int MLogRoll(

  const char *Suffix,
  mbool_t     DoForce,
  int         Iteration,
  int         Depth)

  {
  struct stat buf;
  int         dindex;

  mbool_t     RollFailed = FALSE;

  char OldFile[MAX_LOGNAME] = {0};
  char NewFile[MAX_LOGNAME];

  const char *FName = "MLogRoll";

  MDB(5,fCORE) MLog("%s(%s,%s,%d,%d)\n",
    FName,
    (Suffix != NULL) ? Suffix : "NULL",
    (DoForce == TRUE) ? "TRUE" : "FALSE",
    Iteration,
    Depth);

  if ((LogFile[0] == '\0') || (MSched.OLogFile != NULL))
    {
    /* writing log data to stderr or temp log file */

    /* do not roll */

    return(SUCCESS);
    }

  memset(&buf,0,sizeof(buf));

  if (DoForce == FALSE)
    {
    if (MaxLogFileSize == -1)
      {
      /* no file size limit specified */

      return(SUCCESS);
      }

    if (stat(LogFile,&buf) == -1)
      {
      MDB(0,fCORE) MLog("WARNING:  cannot get stats on file '%s', errno: %d (%s)\n",
        LogFile,
        errno,
        strerror(errno));

      /* re-open log file if necessary */

      if (errno == ENOENT)
        {
        MDEBUG(1)
          {
          fprintf(stderr,"WARNING:  logfile '%s' was removed  (re-opening file)\n",
            LogFile);
          }

        MLogOpen(Iteration);
        }

      return(SUCCESS);
      }
    }    /* END if (DoForce == FALSE) */

  if (((LogFile[0] != '\0') || (mlog.logfp != stderr)) &&
      ((buf.st_size > MaxLogFileSize) || (DoForce == TRUE)))
    {
    if ((Suffix != NULL) && (Suffix[0] == '\0'))
      { 
      sprintf(NewFile,"%s%s",
        LogFile,
        Suffix);

      MDB(2,fCORE) MLog("INFO:     rolling logfile '%s' to '%s'\n",
        LogFile,
        NewFile);

      if ((rename(LogFile,NewFile) == -1) && (errno != ENOENT))
        {
        MDB(3,fCORE) MLog("ERROR:    cannot rename logfile '%s' to '%s' errno: %d (%s)\n",
          LogFile,
          NewFile,
          errno,
          strerror(errno));

        RollFailed = TRUE;

        /* remove old log file */

        MFURemove(OldFile);
        }
      }    /* END if ((Suffix != NULL) && (Suffix[0] == '\0')) */
    else
      {
      for (dindex = Depth;dindex > 0;dindex--)
        {
        sprintf(NewFile,"%s.%d",
          LogFile,
          dindex);
 
        if (dindex == 1)
          strcpy(OldFile,LogFile);
        else
          sprintf(OldFile,"%s.%d",
            LogFile,
            dindex - 1);
 
        MDB(2,fCORE) MLog("INFO:     rolling logfile '%s' to '%s'\n",
          OldFile,
          NewFile);
 
        if ((rename(OldFile,NewFile) == -1) && (errno != ENOENT))
          {
          MDB(2,fCORE) MLog("ERROR:    cannot rename logfile '%s' to '%s' errno: %d (%s)\n",
            OldFile,
            NewFile,
            errno,
            strerror(errno));

          RollFailed = TRUE;

          /* remove old log file */

          MFURemove(OldFile);
          }
        }    /* END for (dindex) */      
      }      /* END else ((Suffix != NULL) && (Suffix[0] == '\0')) */

    if (MLogOpen(Iteration) == SUCCESS)
      {
      if (RollFailed == TRUE)
        {
        MDB(3,fCORE) MLog("ALERT:    log roll failed (old log removed)\n");
        }

      /* Trigger logrotate event and force starting of trigger since triggers
       * need an iteration to rearm themselves. */

      if (MSched.LogRollTrigger != NULL)
        {
        MTrigReset(MSched.LogRollTrigger);
        MOReportEvent(&MSched,NULL,mxoSched,mttLogRoll,MSched.Time,TRUE); /* Set Action Date */
        MTrigInitiateAction(MSched.LogRollTrigger);
        }
      }
    }    /* END if (((LogFile[0] != '\0') || (mlog.logfp != stderr)) && ...) */
  else
    {
    MDB(3,fCORE) MLog("INFO:     not rolling logs (%lu < %d)\n",
      buf.st_size,
      MaxLogFileSize);
    }

  return(SUCCESS);
  }  /* END MLogRoll() */


char *MLogGetTimeNewFormat()

  {
  time_t        epoch_time = 0;
  struct tm    *present_time;
  static char   line[MMAX_LINE];

  MUGetTime((mulong *)&epoch_time,mtmNONE,NULL);

  if ((present_time = localtime(&epoch_time)) != NULL)
    {
    sprintf(line,"%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d ",
      present_time->tm_year + 1900,
      present_time->tm_mon + 1,
      present_time->tm_mday,
      present_time->tm_hour,
      present_time->tm_min,
      present_time->tm_sec);
    }
  else
    {
    sprintf(line,"%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d ", 
      0,0,0,0,0,0);
    }

  return(line);
  }  /* END MLogGetTime() */






char *MLogGetTime()

  {
  time_t        epoch_time = 0;
  struct tm    *present_time;
  static char   line[MMAX_LINE];
  static time_t now = 0;

#ifdef MDEBUGLOG
  long milliseconds = 0;

  MUGetMS(NULL,&milliseconds);

  sprintf(line,"%ld ",
    milliseconds);

  return(line);
#endif /* MDEBUGLOG */

  MUGetTime((mulong *)&epoch_time,mtmNONE,NULL);

  if (epoch_time != now)
    {
    now = epoch_time;

    if ((present_time = localtime(&epoch_time)) != NULL)
      {
#ifdef __MLOGYEAR
      sprintf(line,"%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d ",
        present_time->tm_mon + 1,
        present_time->tm_mday,
        present_time->tm_year + 1900,
        present_time->tm_hour,
        present_time->tm_min,
        present_time->tm_sec);
#else /* __MLOGYEAR */
      sprintf(line,"%2.2d/%2.2d %2.2d:%2.2d:%2.2d ",
        present_time->tm_mon + 1,
        present_time->tm_mday,
        present_time->tm_hour,
        present_time->tm_min,
        present_time->tm_sec);
#endif /* __MLOGYEAR */
      }
    else
      {
      sprintf(line,"%2.2d/%2.2d %2.2d:%2.2d:%2.2d ", 
        0,0,0,0,0);
      }
    }    /* END if (epoch_time != now) */

  return(line);
  }  /* END MLogGetTime() */





/**
 *
 *
 * @param signo (I)
 */

void MLogLevelAdjust(

  int signo)  /* I */

  {
  const char *FName = "MLogLevelAdjust";

  MDB(1,fCONFIG) MLog("%s(%d)\n",
    FName,
    signo);

  if (signo == SIGUSR1)
    {
    if (mlog.Threshold < MAX_MDEBUG)
      {
      mlog.Threshold++;

      MDB(0,fCONFIG) MLog("INFO:     received signal %d.  increasing LOGLEVEL to %d\n",
        signo,
        mlog.Threshold);
      }
    else
      {
      MDB(0,fCONFIG) MLog("INFO:     received signal %d.  LOGLEVEL is already at max value %d\n",
        signo,
        mlog.Threshold);
      }

    signal(SIGUSR1,(void(*)(int))MLogLevelAdjust);
    }
  else if (signo == SIGUSR2)
    {
    if (mlog.Threshold > 0)
      {
      mlog.Threshold--;

      MDB(0,fCONFIG) MLog("INFO:     received signal %d.  decreasing LOGLEVEL to %d\n",
        signo,
        mlog.Threshold);
      }
    else
      {
      MDB(0,fCONFIG) MLog("INFO:     received signal %d.  LOGLEVEL is already at min value %d\n",
        signo,
        mlog.Threshold);
      }

    signal(SIGUSR2,(void(*)(int))MLogLevelAdjust);
    }

  fflush(mlog.logfp);

  return;
  }  /* END MLogLevelAdjust() */


int MLogJob(

  enum MLogLevelEnum  Level,
  const char         *File,
  int                 Line,
  mjob_t             *J,
  char               *Format,
  ...)

  {
  va_list Args;

  va_start(Args,Format);

  MLogNew(Level,mxoJob,J->Name,File,Line,Format,Args);

  va_end(Args);

  return(SUCCESS);
  }  /* END MLogJob() */

/**
 * Log a line using the new log format.
 *
 * FORMAT: YYYY-MM-DD HH:MM:SS:DDD LEVEL CATEGORY SUBJECTID SOURCEFILE/LINENUMBER MESSAGE RESULT
 *
 * @param Level
 * @param Category
 * @param ID
 * @param File
 * @param Line
 * @param Format
 * @param Args
 */

int MLogNew(

  enum MLogLevelEnum  Level,
  enum MXMLOTypeEnum  Category,
  char               *ID,
  const char         *File,
  int                 Line,
  char               *Format,
  va_list             Args)

  {
  if ((mlog.logfp == NULL) ||
      (mlog.State == mlsClosed))
    {
    return(SUCCESS);
    }

  MUMutexLockSilent(&MLogMutex);
  
  if ((mlog.logfp != NULL) && (mlog.State != mlsClosed))
    {
    if (mlog.logfp != stderr)
      {
      fprintf(mlog.logfp,"%s %s %s %s %s/%d ",
        MLogGetTimeNewFormat(),
        MLogLevel[Level],
        MXO[Category],
        ID,
        File,
        Line);
      }

#if defined(MTHREADSAFE) && defined(MLOGTHREADID)
    fprintf(mlog.logfp,"%u ",
      (MUINT4)MUGetThreadID());
#endif /* MCOMMTHREAD && MLOGTHREADID */

    vfprintf(mlog.logfp,Format,Args);
    }  /* END if ((mlog.logfp != NULL) && (mlog.State != mlsClosed)) */

  MUMutexUnlockSilent(&MLogMutex);

  return(SUCCESS);
  }  /* END mlog() */


/* END MLog.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
