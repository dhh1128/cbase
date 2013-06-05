/* HEADER */

/**
 * @file MOSInterface.c
 *
 * Contains OS Interface functions
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


mbool_t *MGUSyslogActive = NULL;

int MOSGetEUID()

  {
  return(geteuid());
  }  /* END MOSGetEUID() */




int MOSGetUID()

  {
  return(getuid());
  }  /* END MOSGetUID() */




int MOSGetGID()

  {
  return(getgid());
  }  /* END MOSGetGID() */




/**
 * Sets the UID of this process.
 *
 * @param UID (I)
 */

int MOSSetUID(

  int UID)  /* I */

  {
  if (MOSGetEUID() == UID)
    {
    return(0);
    }

  return(setuid(UID));
  }  /* END MOSSetUID() */




/**
 * Sets the EUID (effective UID) for this process.
 *
 * @param UID (I)
 */

int MOSSetEUID(

  int UID)  /* I */

  {
  int rc = 0;
  mbool_t DoLock = TRUE;

  if ((UID == -1) || (MOSGetEUID() == UID))
    {
    return(0);
    }

  /* prevent other threads from doing operations
   * that are sensitive to changes in the EUID */

  if (UID == MSched.UID)
    {
    /* we are restoring back to our original EUID,
     * so we can unlock the mutex */

    DoLock = FALSE;
    }
  else
    {
    DoLock = TRUE;
    }

  if (DoLock == TRUE)
    MUMutexLock(&EUIDMutex);  /* must lock BEFORE running seteuid() */

#ifndef __HPUX
  rc = seteuid(UID);
#else /* __HPUX */
  rc = setuid(UID);
#endif /* __HPUX */

  if (DoLock == FALSE)
    MUMutexUnlock(&EUIDMutex);  /* must unlock AFTER running seteuid() */

  return(rc);
  }  /* END MOSSetEUID() */




/**
 *
 *
 * @param GID (I)
 */

int MOSSetGID(

  int GID)  /* I */

  {
  if ((GID == -1) || ((int)getgid() == GID))
    {
    return(0);
    }

  return(setgid(GID));
  }  /* END MOSSetGID() */





/**
 * Report current process id.
 */

int MOSGetPID()

  {
  return(getpid());
  }  /* END MOSGetPID() */

/**
 * Return Thread ID of current thread
 */
int MUGetThreadID()

  {
  /* returns an unsigned int representing the currently running thread */

  int id = 0;

#ifdef MTHREADSAFE

  /* NOTE: the cast to u_int is necessary as there is no guarantee that pthread_t (what
   * pthread_self() returns) is - only that it represents the thread. By casting/extracting the first
   * 32-bits/64-bits (the size of an int) we should be safe in representing this structure no matter what
   * its actual implementation underneath. */

  id = (int)pthread_self();

#endif /* MTHREADSAFE */

  return (id);
  }  /* END MUGetThreadID() */


/**
 * Translate IP address to hostname
 *
 * @param S (I)
 * @param S (I)
 * @param HostName (O) [optional,minsize=MMAX_NAME]
 */

int MOSGetHostByAddr(

#ifdef __MIPV6
  struct sockaddr_in6 *S,       /* I */
#else
  struct sockaddr_in *S,        /* I */
#endif
  char               *HostName) /* O (optional,minsize=MMAX_NAME) */

  {
  int rc = 0;

#ifdef __MIPV6
  struct sockaddr_in6 sockaddr;
#else
  struct sockaddr_in sockaddr;
#endif

  const char *FName = "MOSGetHostByAddr";
  
  MDB(8,fSOCK) MLog("%s(S)\n",
    FName);

  if (S == NULL)
    {
    return(FAILURE);
    }

  if (HostName == NULL)
    {
    return(SUCCESS);
    }

  sockaddr = *S;

  if ((rc = getnameinfo((struct sockaddr *)&sockaddr,sizeof(sockaddr),HostName,MMAX_NAME,NULL,0,0)) != 0)
    {
    MDB(2,fSOCK) MLog("WARNING:  cannot get hostname of client, errcode=%d (%s)\n",
      rc,
      gai_strerror(rc));

    /* convert IP address to string */

#ifdef __MIPV6
    sprintf(HostName,"%ld.%ld.%ld.%ld",
      (sockaddr.sin6_addr.s6_addr32[0] & 0x000000ff),
      (sockaddr.sin6_addr.s6_addr32[0] & 0x0000ff00) >> 8,
      (sockaddr.sin6_addr.s6_addr32[0] & 0x00ff0000) >> 16,
      (sockaddr.sin6_addr.s6_addr32[0] & 0xff000000) >> 24);
#else /* __MIPV6 */
    sprintf(HostName,"%u.%u.%u.%u",
      (sockaddr.sin_addr.s_addr & 0x000000ff),
      (sockaddr.sin_addr.s_addr & 0x0000ff00) >> 8,
      (sockaddr.sin_addr.s_addr & 0x00ff0000) >> 16,
      (sockaddr.sin_addr.s_addr & 0xff000000) >> 24);
#endif /* __MIPV6 */

    if (HostName[0] == '\0')
      MUStrCpy(HostName,"[UNKNOWN]",MMAX_LINE);

    return(FAILURE);
    }  
  else
    {
    MDB(2,fSOCK) MLog("INFO:     received service request from host '%s'\n",
      HostName);
    }

  return(SUCCESS);
  }  /* END MOSGetHostByAddr() */






/**
 * Initialize syslog usage.
 *
 * @see MOSSyslog() - peer.
 *
 * @param S (I)
 */

int MOSSyslogInit(
 
  msched_t *S)  /* I */

  {
  if (S == NULL)
    {
    return(FAILURE);
    }

  MGUSyslogActive = &S->SyslogActive;

  if ((S->SyslogActive == FALSE) && (S->UseSyslog == TRUE))
    {
    openlog(
      MSCHED_SNAME,
      LOG_PID|LOG_NDELAY,
      (S->SyslogFacility > 0) ? 
        S->SyslogFacility : 
        LOG_DAEMON);

    S->SyslogActive = TRUE;

    if (S->Mode == msmNormal)
      {
      MOSSyslog(LOG_INFO,"%s initialized",
        MSCHED_SNAME);
      }
    }

  return(SUCCESS);
  }  /* END MSyslogInitialize() */





/**
 * Report message to syslog facility.
 *
 * @see MSyslogInit() - peer
 *
 * @param Priority (I)
 * @param Format (I)
 */

int MOSSyslog(

  int         Priority,
  const char *Format,
  ...)

  {
  va_list Args;

  char    tmpBuf[MMAX_BUFFER];

  if ((MGUSyslogActive == NULL) ||
      (*MGUSyslogActive == FALSE))
    {
    return(SUCCESS);
    }

  va_start(Args,Format);

  vsnprintf(tmpBuf,sizeof(tmpBuf),Format,Args);

  va_end(Args);

  syslog(Priority,"%s",tmpBuf);

  return(SUCCESS);
  }  /* END MOSSyslog() */



/**
 * Create directory with specified name and permissions.
 *
 * @see MFUCreate() - create file with specified name and permissions.
 *
 * @param PathName (I)
 * @param Mode (I)
 */

int MOMkDir(

  char *PathName,  /* I */
  int   Mode)      /* I */

  {
  if ((PathName == NULL) || (PathName[0] == '\0'))
    {
    return(FAILURE);
    }

  if (mkdir(
        PathName,
        (Mode > 0) ? Mode : MDEF_UMASK) != 0)
    {
    MDB(5,fSOCK) MLog("INFO:     cannot mkdir '%s' errno: %d (%s)\n",
      PathName,
      errno,
      strerror(errno));

    return(FAILURE);
    }

  /* directory successfully created */

  return(SUCCESS);
  }  /* END MOMkDir() */



/**
 * returns true if FileName exists as a file
 */

mbool_t MUFileExists(

 char const * FileName)

  {
  return (access(FileName,F_OK) == 0);
  }



/**
 * Change working directory.
 *
 * @param Dir (I) [optional]
 * @param EP (I) [optional]
 * @param CheckEnvVar (I)
 */

int MOChangeDir(

  const char    *Dir,          /* I (optional) */
  muenv_t *EP,           /* I (optional) */
  mbool_t  CheckEnvVar)  /* I */

  {
  char *hptr;
  char  tmpDir[MMAX_LINE];

  tmpDir[0] = '\0';

  if ((Dir == NULL) || (Dir[0] == '\0'))
    {
    /* directory not set */

    if (EP != NULL)
      hptr = MUEnvGetVal(EP,"HOME");
    else
      hptr = getenv("HOME");

    if ((hptr == NULL) || (hptr[0] == '\0'))
      {
      return(SUCCESS);
      }

    MUStrCpy(tmpDir,hptr,sizeof(tmpDir));
    }
  else 
    {
    if (CheckEnvVar == TRUE)
      {
      MUInsertEnvVar(Dir,EP,tmpDir);
      }
    else
      {
      MUStrCpy(tmpDir,Dir,sizeof(tmpDir));
      }
    }

  if (chdir(tmpDir) == -1)
    {
    MDB(5,fSOCK) MLog("INFO:     cannot chdir to '%s' errno: %d (%s)\n",
      tmpDir,
      errno,
      strerror(errno));

    return(FAILURE);
    }  /* END if (chdir(ptr) == -1) */

  return(SUCCESS); 
  }  /* END MOChangeDir() */



/**
 *Retrieves the environment variable MSCHED_ENVHOMEVAR and returns
 *the default value of /opt/moab if that variable is not defined
 */

const char *MUGetHomeDir(void)

  {
  const char *result = getenv(MSCHED_ENVHOMEVAR);

  if (result != NULL)
    {
    return result;
    }

  return MBUILD_ETCDIR;
  }  /* END MUGetHomeDir() */
/* END MOSInterface.c */
