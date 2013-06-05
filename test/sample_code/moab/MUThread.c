/* HEADER */

/**
 * @file MUThread.c
 * 
 * Contains various functions dealing with threading
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


#ifdef MTHREADSAFE
extern void MSysThreadDestructor(void *);
#endif /* MTHREADSAFE */


/* Signal number toString() lookup table */

const char * MSigName[] = {
  NONE,
  "hup",
  "int",
  "quit",
  "ill",
  "trap",
  "abrt",
  "bus",
  "fpe",
  "kill",
  "usr1",
  "segv",
  "usr2",
  "pipe",
  "alrm",
  "term",
  "stkflt",
  "chld",
  "cont",
  "stop",
  "tstp",
  "ttin",
  "ttou",
  "urg",
  "xcpu",
  "xfsz",
  "vtalrm",
  "prof",
  "winch",
  "io",
  "pwr",
  "sys",
  "tmin",
  NULL };


/* NOTE:  must sync MMAX_UARG w/MUThread(), MUTFuncThread(), and __MUTFunc() */

#define MMAX_UARG  12
 
typedef struct {
    int (*Func)();
    void *Arg[MMAX_UARG+1];
    long  TimeOut;
    int  *RC;
    int  *Lock;
    } mut_t;

 
/* prototypes */
 
int __MUTFunc(void *);

#ifdef MTHREADSAFE
int __MUTFuncThread(void *);
#endif /* MTHREADSAFE */

/* END prototypes */



/**
 *
 *
 * @param SigString (I)
 * @param SignoP (O)
 */

int MUSignalFromString(

  char *SigString,  /* I */
  int  *SignoP)     /* O */

  {
  int   tmpI;
  char *ptr;

  /* NOTE: the order of these signals may not work on non-Linux/non-POSIX operating systems! */

  if (SignoP == NULL)
    {
    return(FAILURE);
    }

  *SignoP = 0;

  if ((SigString == NULL) || (SigString[0] == '\0'))
    {
    return(FAILURE);
    }

  /* FORMAT:  <INT>|SIG<SIGNAME>|<SIGNAME> */

  tmpI = (int)strtol(SigString,NULL,10);

  if (tmpI > 0)
    {
    *SignoP = tmpI;

    return(SUCCESS);
    }

  if (strncasecmp(SigString,"sig",strlen("sig")) == 0)
    {
    ptr = SigString + strlen("sig");
    }
  else
    {
    ptr = SigString;
    }

  tmpI = MUGetIndexCI(ptr,MSigName,FALSE,0);

  if (tmpI == 0)
    {
    return(FAILURE);
    }

  *SignoP = tmpI;

  return(SUCCESS);
  }  /* END MUSignalFromString() */


/**
 *
 *
 * @param SigNum (I) [optional when -1]
 * @param SigName (I) [optional]
 * @param SigString (O) [minsize=MMAX_LINE]
 */

int MUBuildSignalString(

  int   SigNum,     /* I (optional when -1) */
  char *SigName,    /* I (optional) */
  char *SigString)  /* O (minsize=MMAX_LINE) */

  {
  /* NOTE: in future we may need to pass in a RMType var in order to build string based on RM requirements */
  /* NYI */

  if (SigString == NULL)
    {
    return(FAILURE);
    }

  SigString[0] = '\0';

  if (SigNum >= 0)
    {
    snprintf(SigString,MMAX_LINE,"%d",
      SigNum);
    }
  else if ((SigName != NULL) && (SigName[0] != '\0'))
    {
    if (isdigit(SigName[0]) || !strncasecmp(SigName,"sig",strlen("sig")))
      { 
      MUStrCpy(SigString,SigName,MMAX_LINE);
      }
    else
      {
      /* some RM's only support <INT> and "SIG<X>" so we need to prepend "SIG" */
    
      snprintf(SigString,MMAX_LINE,"SIG%s",
        SigName);
      }
    }
  else
    {
    /* invalid signal & action specified */
    
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUBuildSignalString() */




/**
 * Executes the given function in a separate thread of execution. This function
 * only provides a way for Moab to launch a function that could potentially
 * take a long time and then cancel the function's operation if the given
 * TimeOut is reached. This function does not increase overall throughput or
 * performance as the code will still wait for the new thread to finish before
 * it moves on. An improved threading model is needed in order to better
 * accomodate true gains in performance with multithreading.
 *
 * NOTE: If MTHREAD is not defined (threading is not enabled), then the passed
 * in function is simply executed inline. There is one exception to this rule:
 * if an RM is passed in via R, and that RM has a timeout specified, then a
 * thread is used.
 *
 * NOTE:  For threading, calls __MUTFuncThread()
 *
 * @see __MUTFuncThread()
 *
 * @param F (I) The function to execute--possibly in a thread.
 * @param TimeOut (I) Seconds (if < 1000) or microseconds to wait for thread before killing it. [optional]
 * @param RC (O) [optional,value should be SUCCESS,FAILURE or mscNoMemory/mscTimeout FIXME!]
 * @param LockP (O) [optional]
 * @param R (I) The resource manager involved in executing the given function. [optional]
 * @param ACount (I)
 * @return On thread or general failure, 0 is returned. On success, 1 is returned.
 */

int MUThread(

  int   (*F)(),     /* I */
  long    TimeOut,  /* I (optional,seconds if < 1000 or microseconds) */
  int    *RC,       /* O (optional,value should be SUCCESS,FAILURE or mscNoMemory/mscTimeout FIXME!) */
  int    *LockP,    /* O (optional) */
  mrm_t  *R,        /* I (optional) */
  int     ACount,   /* I */
  ...)              /* I (function args) */

  {
  int rc;

  int index;

  int MyLock;

  mut_t D;

  va_list Args;

  mbool_t DoThread = FALSE;

  /* NOTE:  if Lock specified, function is non-blocking */

#ifdef MFORCERMTHREADS
  /* variables only used in threaded code */
  mulong         MicroTimeOut;
  pthread_t      T;
  pthread_attr_t TA;
  mulong DelayTime;
  mulong DelayInterval = 100000;  /* microseconds */
#endif /* MFORCERMTHREADS */

  const char *FName = "MUThread";

  MDB(7,fCORE) MLog("%s(%p,%ld,RC,%s,%s,%d,...)\n",
    FName,
    (F != NULL) ? (void *)F : "NULL",  
    TimeOut,
    (LockP != NULL) ? "LockP" : "NULL",
    (R != NULL) ? R->Name : "NULL",
    ACount);

  if (F == NULL)
    {
    if (RC != NULL)
      *RC = mscBadParam;

    return(FAILURE);
    }

  if (LockP != NULL)
    D.Lock = LockP;
  else
    D.Lock = &MyLock;

  *D.Lock = TRUE;

  D.Func = (int (*)())F;

  if (RC != NULL)
    D.RC = RC;
  else
    D.RC = &rc;

  va_start(Args,ACount);

  for (index = 0;index < MIN(ACount,MMAX_UARG);index++)
    {
    D.Arg[index] = va_arg(Args,void *);
    }  /* END for (index) */

  /* NOTE:  verify all args are initialized */

  if (index < MMAX_UARG)
    {
    for (;index <= MMAX_UARG;index++)
      {
      D.Arg[index] = NULL;
      }  /* END for (index) */
    }

  va_end(Args);

#ifdef MFORCERMTHREADS
  DoThread = TRUE;
#else  /* MFORCERMTHREADS */

  if ((R != NULL) && 
      bmisset(&R->IFlags,mrmifTimeoutSpecified) && 
      (R->Type != mrmtNative))
    {
    /* DoThread = TRUE; No longer support threaded timeouts!!! */
    }
  else
    {
    DoThread = FALSE;
    }
#endif /* MFORCERMTHREADS */

  if (DoThread == FALSE)
    {
    rc = __MUTFunc(&D);

    if (rc == FAILURE)
      {
      return(rc);
      }
    else
      {
      return(*D.RC);
      }
    }

#if !defined(MFORCERMTHREADS)

  MDB(0,fCORE) MLog("ALERT:    attempting to run in threaded model without MForceRMThread compilation!\n");
 
  return(FAILURE);
#endif /* !defined(MFORCERMTHREADS) */

#if defined(MFORCEMTHREADS) && defined(MTHREADSAFE)

  pthread_attr_init(&TA);

#ifdef __AIX
  MDB(4,fALL) MLog("INFO:    setting MUThread stack size to %d bytes\n",
    MDEF_THREADSTACKSIZE);

  pthread_attr_setstacksize(&TA,MDEF_THREADSTACKSIZE);
#endif /* __AIX */

  pthread_attr_setdetachstate(&TA,PTHREAD_CREATE_DETACHED);

  if ((rc = pthread_create(
        &T,
        &TA,
        (void *(*)(void *))__MUTFuncThread,
        (void *)&D)) != 0)
    {
    MDB(0,fCORE) MLog("ALERT:    cannot create thread (rc: %d)\n",
      rc);
 
    /* do not retry thread on this iteration */

    if (RC != NULL)
      *RC = mscNoMemory;

    return(FAILURE);
    }

  MDB(7,fCORE) MLog("INFO:     thread %lu created\n",
    (mulong)T);

  /* poll waiting for thread to complete */

  if (TimeOut < 1000)
    MicroTimeOut = TimeOut * MDEF_USPERSECOND;
  else
    MicroTimeOut = TimeOut;

  /* min delay is 20 milliseconds */

  for (DelayTime = 0;*D.Lock == TRUE;DelayTime += DelayInterval)
    {
    if (DelayTime >= MAX(DelayInterval << 1,MicroTimeOut))
      break;

    MDB(5,fCORE) 
      {
      if ((DelayTime > 0) && ((DelayTime % MDEF_USPERSECOND) == 0))
        MLog("ALERT:    thread %lu blocking (%.2f seconds)\n",
          (mulong)T,
          (double)DelayTime / MDEF_USPERSECOND);
      }

    MUSleep(DelayInterval,TRUE);
    }  /* END for (DelayTime) */
   
  if (*D.Lock == TRUE)
    {
    /* if thread is not completed within timeout, kill thread */
 
    pthread_cancel(T);
 
    MDB(0,fCORE) MLog("ALERT:    thread %lu killed (%ld micro-second time out reached)\n",
      (mulong)T,
      TimeOut);
  
    if (RC != NULL)
      {
      /* HACK:  need alternate method to indicate timeout occurred */

      *RC = mscTimeout;
      }
 
    return(FAILURE);
    }

  if (RC != NULL)
    {
    *RC = *D.RC;
    }

  MDB(8,fCORE) MLog("INFO:     thread %lu completed after %.2f seconds\n",
    (mulong)T,
    (double)DelayTime / MDEF_USPERSECOND);
#endif /* if defined(MFORCERMTHREADS) && ... */

  return(SUCCESS);
  }    /* END MUThread() */




/**
 * Call a function based on the given mut_t structure. This struct defines
 * the function to call, the args to pass in, and allows a return code to
 * be set.
 *
 * @see struct mut_t
 *
 * @param V (I) The mut_t struct defining the function to call.
 */

int __MUTFunc(

  void *V) /* I */

  {
  mut_t *D;

  if (V == NULL)
    {
    return(FAILURE);
    }

  D = (mut_t *)V;

  *D->RC = ((int (*)(void *,void *,void *,void *,void *,void *,void *,void *,void *,void *,void *,void *))D->Func)(
     D->Arg[0],
     D->Arg[1],
     D->Arg[2],
     D->Arg[3],
     D->Arg[4],
     D->Arg[5],
     D->Arg[6],
     D->Arg[7],
     D->Arg[8],
     D->Arg[9],
     D->Arg[10],
     D->Arg[11]);

  *D->Lock = FALSE;

  return(SUCCESS);
  }  /* END __MUTFunc() */





#ifdef MTHREADSAFE
/**
 * Wrapper function used to spawn a thread (see MUThread()) that simply executes
 * a function, passing a list of given parameters into it. 
 *
 * @param V (I) A mut_t variable which contains a pointer to the function to run, its
 * parameters, etc.
 */

int __MUTFuncThread(

  void *V)  /* I */

  {
  mut_t *D;

  if (V == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  default pthread cancel type is deferred */

  /* NOTE: pthread_canceltype should only be set to one of 
           PTHREAD_CANCEL_DEFERRED or PTHREAD_CANCEL_ASYNCHRONOUS */

  /* pre-Moab 5.2.0 */

  /* pthread_setcanceltype(PTHREAD_CANCEL_ENABLE,NULL); */

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  /* NOTE:  settings cancel type to PTHREAD_CANCEL_ASYNCHRONOUS allows system 
            calls to potentially exit leaving active mutex's behind which can 
            result in Moab's master thread hanging in a subsequent system call.

            Also, with PTHREAD_CANCEL_DEFERRED, long blocks of code without
            system calls should call pthread_testcancel() to allow the 
            cancelled thread to terminate.

     NOTE:  If cancellation is enabled, and its type is deferred, the system tests for pending cancellation requests in certain defined functions (usually any blocking system function). These points are known as cancellation points.

Cancellation points may be created in user code with the pthread_testcancel() function.

Other thread-related functions that are cancellation points include pthread_cond_wait(), pthread_cond_timedwait(), pthread_join(), and sigwait(). pthread_mutex_lock() is not a cancellation point, although it may block indefinitely; making pthread_mutex_lock() a cancellation point would make writing correct cancellation handlers difficult, if not impossible.

The following functions are cancellation points: close(), creat(), fcntl(), fsync(), lockf(), nanosleep(), open(), pause(), pthread_cond_timedwait(), pthread_cond_wait(), pthread_join(), pthread_testcancel(), read(), readv(), select(), sigpause(), sigsuspend(), sigwait(), sleep(), system(), tcdrain(), usleep(), wait(), waitpid(), write(), writev().

In addition, a cancellation point may occur in the following functions: catclose(), catgets(), catopen(), closedir(), closelog(), ctermid(), dlclose(), dlopen(), fclose(), fcntl(), fflush(), fgetc(), fgetpos(), fgets(), fopen(), fprintf(), fputc(), fputs(), fread(), freopen(), fscanf(), fseek(), fsetpos(), ftell(), fwrite(), getc(), getc_unlocked() getchar(), getchar_unlocked(), getcwd(), getgrgid(), getgrgid_r(), getgrnam(), getgrnam_r(), getlogin(), getlogin_r(), getpwnam(), getpwnam_r(), getpwuid(), getpwuid_r(), gets(), getwd(), glob(), ioctl(), lseek(), mkstemp(), opendir(), openlog(), pclose(), perror(), popen(), printf(), putc(), putc_unlocked() putchar(), putchar_unlocked(), puts(), readdir(), readdir_r(), remove(), rename(), rewind(), rewinddir(), scanf(), semop(), strerror(), syslog(), tmpfile(), tmpnam(), ttyname(), ttyname_r(), ungetc(), unlink(), vfprintf(), vprintf().
  */
  
  /* pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); */

  pthread_cleanup_push(MSysThreadDestructor,NULL);

  D = (mut_t *)V;

  *D->RC = ((int (*)(void *,void *,void *,void *,void *,void *,void *,void *,void *,void *,void *,void *))D->Func)(
     D->Arg[0],
     D->Arg[1],
     D->Arg[2],
     D->Arg[3],
     D->Arg[4],
     D->Arg[5],
     D->Arg[6],
     D->Arg[7],
     D->Arg[8],
     D->Arg[9],
     D->Arg[10],
     D->Arg[11]);

  *D->Lock = FALSE;

  pthread_cleanup_pop(0);

  return(SUCCESS);
  }     /* END __MUTFuncThread() */
#endif  /* END MTHREADSAFE */

 


/**
 * Performs a fork and returns a PID as per the man page of fork().
 *
 * @return PID (as per def. for fork()) or -1 on error.
 */

pid_t MUFork()

  {
  pid_t pid;

  pid = fork();

  if (pid == -1) 
    MDB(0,fCORE) MLog("ERROR:    fork failed.\n");
 
  return(pid);
  }  /* END MUFork() */


