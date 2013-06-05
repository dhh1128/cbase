/* HEADER */

/**
 * @file MUSpawnChild.c
 * 
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/**
 *
 *
 * @param IFD
 * @param OFD
 * @param EFD
 * @param IFName (I) [optional]
 * @param OFName (I) [optional]
 * @param EFName (I) [optional]
 */

int MUChildCleanUp(

  int   IFD, 
  int   OFD,
  int   EFD, 
  char *IFName,  /* I (optional) */
  char *OFName,  /* I (optional) */
  char *EFName)  /* I (optional) */

  {
  int  rc;

  if (IFD >= 0)
    close(IFD);

  if (OFD > 0)
    close(OFD);

  if (EFD > 0)
    close(EFD);

  if ((IFName != NULL) && (IFName[0] != '\0'))
    {
    unlink(IFName);
    }

  if ((OFName != NULL) && (OFName[0] != '\0'))
    {
    unlink(OFName);
    }

  if ((EFName != NULL) && (EFName[0] != '\0'))
    {
    unlink(EFName);
    }
 
  umask(MSched.UMask);

  /* must set GID first */

  MOSSetGID(MSched.GID);

  MOSSetUID(MSched.UID);

  rc = chdir(MSched.CfgDir);

  MDB(9,fALL) MLog("INFO:     chdir returned %d\n",rc);

  return(FAILURE);
  }  /* END MUChildCleanUp() */



/**
 * Launch child task with specified environment.
 *
 * NOTE: Not threadsafe!
 *
 * @see MUReadPipe()
 * @see MUReadPipe2()
 *
 * @param SExec     (I) [required]
 * @param Name      (I) [optional]
 * @param Args      (I) [optional]
 * @param Prefix    (I) [optional,exec launch environment, ie shell + env vars]
 * @param UID       (I) [optional,-1 if not set]
 * @param GID       (I) [optional,-1 if not set]
 * @param CWD       (I) [optional]
 * @param Env       (I) [optional/modified,FORMAT=<NAME>=<VALUE>[;<NAME>=<VALUE>]...]
 * @param EnvPipe   (I) [optional] Initialized pipes to write EnvBuf to. execve will use a null environment.
 * @param Limits    (I) [optional,not yet supported]
 * @param IBuf      (I) input buffer, not input filename [optional]
 * @param IFileName (O) (optional) input buffer filename (like OBuf and EBuf below)
 * @param OBuf      (O) (see NOTE) [optional,alloc]
 * @param EBuf      (O) (see NOTE) [optional,alloc]
 * @param PID       (O) [optional]
 * @param SC        (O) exit/status code [optional]
 * @param TimeLimit (I) time child is allowed to execute in background (if child runs over, it is killed) [in ms]
 * @param BlockTime (I) [optional,<=0 for no blocktime,in ms]
 * @param O         (I) [optional]
 * @param OType     (I) [optional]
 * @param CheckEnv  (I) perform security check on environment
 * @param IgnoreDefaultEnv (I)  ignore default environment
 * @param EMsg      (O) [optional,minsize=MMAX_LINE]
 */

int MUSpawnChild(

  const char    *SExec,             
  const char    *Name,             
  const char    *Args,            
  const char    *Prefix,         
  int      UID,           
  int      GID,          
  const char    *CWD,         
  const char    *Env,        
  int     *EnvPipe,
  char    *Limits,    
  const char    *IBuf,     
  char   **IFileName,         
  char   **OBuf,             
  char   **EBuf,            
  int     *PID,            
  int     *SC,            
  long     TimeLimit,    
  long     BlockTime,   
  void    *O,          
  enum     MXMLOTypeEnum OType,
  mbool_t  CheckEnv,        
  mbool_t  IgnoreDefaultEnv,
  char    *EMsg)            

  {
  pid_t  pid;

  int    ifd = -1;
  int    ofd = -1;
  int    efd = -1;

  int    aindex;

  muenv_t E;

  char *AList[256];
  char *tmpArgs = NULL;

  char  UMaskStr[MMAX_LINE];
  char  tmpPrefix[MMAX_NAME];
  char  tmpExec[MMAX_LINE];

  char *ptr;
  char *TokPtr;

  char  exec[MMAX_LINE];

  char  tmpIFName[MMAX_LINE];
  char  tmpOFName[MMAX_LINE];
  char  tmpEFName[MMAX_LINE];
  const char *NameP;

  gid_t GList[MMAX_USERGROUPCOUNT];

  mbool_t DefaultPathRequired;

  pid_t (*ForkFuncPtr)(void) = NULL;  /* used to perform fork/vfork */

  const char *FName = "MUSpawnChild";

  MDB(3,fSTRUCT) MLog("%s(%s,%s,%.64s,%.32s,%d,%d,%.32s,%.32s,Limits,IBuf,OBuf,EBuf,PID,SC,%ld,%ld,O,OType,%s,EMsg)\n",
    FName,
    (SExec != NULL) ? SExec : "NULL",
    (Name != NULL) ? Name : "NULL",
    (Args != NULL) ? Args : "NULL",
    (Prefix != NULL) ? Prefix : "NULL",
    UID,
    GID,
    (CWD != NULL) ? CWD : "NULL",
    (Env != NULL) ? Env : "NULL",
    TimeLimit,
    BlockTime,
    MBool[CheckEnv]);

  MDB(8,fSTRUCT) MLog("%s(%s,%s,%s,%s,%d,%d,%s,%s,Limits,IBuf,OBuf,EBuf,PID,SC,%ld,%ld,O,OType,%s,EMsg)\n",
    FName,
    (SExec != NULL) ? SExec : "NULL",
    (Name != NULL) ? Name : "NULL",
    (Args != NULL) ? Args : "NULL",
    (Prefix != NULL) ? Prefix : "NULL",
    UID,
    GID,
    (CWD != NULL) ? CWD : "NULL",
    (Env != NULL) ? Env : "NULL",
    TimeLimit,
    BlockTime,
    MBool[CheckEnv]);

  /* NOTE:  stdout/stderr, if TimeLimit not specified, file name */
  /*        if TimeLimit specified, return char buffer (alloc) */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = 0;

  if (Name != NULL)
    {
    if ((NameP = (char *)strrchr(Name,'/')) == NULL)
      {
      NameP = Name;
      }
    else
      {
      NameP++;
      }
    }
  else
    {
    NameP = NULL;
    }

  if (SExec == NULL)
    {
    /* executable not specified */

    MDB(1,fSTRUCT) MLog("ALERT:    invalid executable specified '%s'\n",
      (SExec != NULL) ? SExec : "NULL");

    if (EMsg != NULL)
      sprintf(EMsg,"NULL executable specified");

    return(FAILURE);
    }  /* END if (SExec == NULL) */

  if (CheckEnv == TRUE)
    {
    /* check attr security */

    /* must change to only enforce check on non-job submit (NYI) */

    if ((MUCheckString(SExec,MSched.TrustedCharListX) == FAILURE) ||
       ((Args != NULL) && (MUCheckString(Args,MSched.TrustedCharListX) == FAILURE)) ||
       ((Prefix != NULL) && (MUCheckString(Prefix,MSched.TrustedCharListX) == FAILURE)) ||
       ((CWD != NULL) && (MUCheckString(CWD,MSched.TrustedCharListX) == FAILURE)))
       /*((Env != NULL) && (MUCheckString(Env,MSched.TrustedCharList) == FAILURE))) */
      {
      /* non-trusted characters detected in child environment */

      MDB(1,fSTRUCT) MLog("ALERT:    non-trusted characters detected in child environment\n");

      if (EMsg != NULL)
        sprintf(EMsg,"non-trusted characters detected in child environment");

      return(FAILURE); 
      }  /* END if (exec == NULL) */
    }    /* END if (CheckEnv == TRUE) */

  /* verify exec file is executable */

  if (MFUGetFullPath(SExec,MUGetPathList(),NULL,NULL,TRUE,exec,sizeof(exec)) == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    exec '%s' cannot be located or is not executable (errno: %d '%s')\n",
      SExec,
      errno,
      strerror(errno));

    if (EMsg != NULL)
      {
      sprintf(EMsg,"exec '%s' cannot be located or is not executable",
        SExec);
      }

    return(FAILURE);
    }    /* END if (MFUGetFullPath() == FAILURE) */

  if (Prefix != NULL)
    {
    if (MFUGetFullPath(Prefix,MUGetPathList(),NULL,NULL,TRUE,exec,sizeof(exec)) == FAILURE)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    prefix '%s' cannot be located or is not executable (errno: %d '%s')\n",
        Prefix,
        errno,
        strerror(errno));

      if (EMsg != NULL)
        {
        sprintf(EMsg,"prefix '%s' cannot be located or is not executable",
          Prefix);
        }

      return(FAILURE);
      }    /* END if (MFUGetFullPath() == FAILURE) */

    /* NOTE: add '-c <EXEC>' to arg stack below */
    }      /* END if (Prefix != NULL) */

  /* must set GID first */
  /* see man page on setgid() - because of CAP_SETGID not every user can switch groups, must switch group first then user */

  if ((GID != -1) && 
      (GID != MSched.GID) &&
      (MOSSetGID(GID) == -1))
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot set gid to '%d' for child %s (errno: %d '%s')\n",
      GID,
      (NameP != NULL) ? NameP : "N/A",
      errno,
      strerror(errno));

    if (EMsg != NULL)
      sprintf(EMsg,"cannot set GID");

    return(FAILURE);
    }

  if ((GID != -1) && (GID != MSched.GID))
    {
    int NumGroups = 1;

    /* set child grouplist */

    if (!bmisset(&MSched.Flags,mschedfDoNotApplySecondaryGroupToChild))
      {
      char tmpUName[MMAX_NAME];

      if (UID == -1)
        {
        MUNameGetGroups(
          MSched.Admin[1].UName[0],
          GID,
          (int *)GList,      /* O */
          NULL,
          &NumGroups,        /* O */
          MMAX_USERGROUPCOUNT);
        }
      else
        {
        MUNameGetGroups(
          MUUIDToName(UID,tmpUName),
          GID,
          (int *)GList,      /* O */
          NULL,
          &NumGroups,        /* O */
          MMAX_USERGROUPCOUNT);
        }
      }
    else
      {
      GList[0] = GID;
      }

    if (setgroups(NumGroups,GList) < 0)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot setgroups to '%d' for child %s (errno: %d '%s')\n",
        GID,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        sprintf(EMsg,"cannot set grouplist, err=%s",
          strerror(errno));

      return(FAILURE);
      }
    }

  if ((UID != -1) && 
      (UID != MSched.UID) &&
      (MOSSetEUID(UID) == -1))
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot set uid to '%d' for child %s (errno: %d '%s')\n",
      UID,
      (NameP != NULL) ? NameP : "N/A",
      errno,
      strerror(errno));

    if (EMsg != NULL)
      sprintf(EMsg,"cannot set UID");

    return(FAILURE);
    }

  tmpIFName[0] = '\0';
  tmpOFName[0] = '\0';
  tmpEFName[0] = '\0';

  /* if TimeLimit specified, block until process completes */

  if (IBuf != NULL)
    {
    /* determine temporary input filename */

    sprintf(tmpIFName,"%s/%s.iXXXXXX",
      MSched.SpoolDir,
      (NameP != NULL) ? NameP : "moab");

    if ((MUMksTemp(tmpIFName,&ifd) == FAILURE) ||
        (ifd == -1))
      {
      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MDB(1,fSTRUCT) MLog("ALERT:    cannot create input file '%s' for UID %d for child %s (errno: %d '%s')\n",
        tmpIFName,
        UID,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot create input file '%s' for UID %d (errno: %d '%s')",
          tmpIFName,
          UID,
          errno,
          strerror(errno));
        }

      return(FAILURE);
      }  /* if ((MUMksTemp(tmpIFName,&ifd)) == FAILURE) */

    /* make file r by owner */

    if (fchmod(ifd,S_IRUSR) == -1)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot change mode on input file '%s' for child %s (errno: %d '%s')\n",
        tmpIFName,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        sprintf(EMsg,"cannot set ownership on input file, err=%s",
          strerror(errno));

      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MUChildCleanUp(ifd,-1,-1,tmpIFName,NULL,NULL);

      return(FAILURE);
      }

    /* write data to file */

    if (write(ifd,(void *)IBuf,strlen(IBuf)) == -1)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot write data to input file '%s' for child %s (errno: %d '%s')\n",
        tmpIFName,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        {
        sprintf(EMsg,"cannot write data to input file '%s', err=%s",
          tmpIFName,
          strerror(errno));
        }

      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MUChildCleanUp(ifd,-1,-1,tmpIFName,NULL,NULL);

      return(FAILURE);
      }  /* END if (write(ifd) == -1) */

    close(ifd);

    ifd = open(tmpIFName,O_RDONLY,0);
    }  /* END if (IBuf != NULL) */

  if (OBuf != NULL)
    {
    *OBuf = NULL;

    /* determine temporary output filename */

    sprintf(tmpOFName,"%s/%s.oXXXXXX",
      MSched.SpoolDir,
      (NameP != NULL) ? NameP : "moab");

    if ((MUMksTemp(tmpOFName,&ofd) == FAILURE) ||
        (ofd == -1))
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot open stdout file '%s' for child %s (errno: %d '%s')\n",
        tmpOFName,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        {
        sprintf(EMsg,"cannot open output file '%s', err=%s",
          tmpOFName,
          strerror(errno));
        }

      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MUChildCleanUp(ifd,-1,-1,tmpIFName,NULL,NULL);

      return(FAILURE);
      }

    /* make file r/w by owner */

    if (fchmod(ofd,S_IWUSR|S_IRUSR) == -1)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot change mode for stdout file '%s' for child %s (errno: %d '%s')\n",
        tmpOFName,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        sprintf(EMsg,"cannot change ownership of output file, err=%s",
          strerror(errno));

      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MUChildCleanUp(ifd,ofd,-1,tmpIFName,tmpOFName,NULL);

      return(FAILURE);
      } 
    }    /* END if (OBuf != NULL) */

  if (EBuf != NULL)
    {
    *EBuf = NULL;

    /* determine temporary error filename */

    sprintf(tmpEFName,"%s/%s.eXXXXXX",
      MSched.SpoolDir,
      (NameP != NULL) ? NameP : "moab");

    if ((MUMksTemp(tmpEFName,&efd) == FAILURE) ||
        (efd == -1))
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot open stderr file %s for child %s (errno: %d '%s')\n",
        tmpEFName,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        sprintf(EMsg,"cannot open error file, err=%s",
          strerror(errno));

      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MUChildCleanUp(ifd,ofd,-1,tmpIFName,tmpOFName,NULL);

      return(FAILURE);
      }

    if (fchmod(efd,S_IWUSR|S_IRUSR) == -1)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot change mode for stderr file %s for child %s (errno: %d '%s')\n",
        tmpEFName,
        (NameP != NULL) ? NameP : "N/A",
        errno,
        strerror(errno));

      if (EMsg != NULL)
        sprintf(EMsg,"cannot change ownership of error file, errno=%d",
          errno);

      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MUChildCleanUp(ifd,ofd,efd,tmpIFName,tmpOFName,tmpEFName);

      return(FAILURE);
      }
    }    /* END if (EBuf != NULL) */

  MUEnvSetDefaults(&E,UID);

  DefaultPathRequired = TRUE;

  if ((Env != NULL) && (Env[0] != '\0'))
    {
    char *ptr;
    char *TokPtr;

    char *tmpe = NULL;

    MUStrDup(&tmpe,Env);

    if (strstr(Env,"PATH=") != NULL)
      {
      /* User specified a path in Env so we do not need to setup a default path */

      DefaultPathRequired = FALSE;
      }

    /* FORMAT:  <NAME>=<VALUE>[<ENVRS_ENCODED_CHAR><NAME>=<VALUE>]... */

    /* NOTE:  name value pairs are delimited with the ENVRS_ENCODED_CHAR character */

    ptr = MUStrTok(tmpe,ENVRS_ENCODED_STR,&TokPtr);

    while (ptr != NULL)
      {
      MUEnvAddValue(&E,ptr,NULL);

      ptr = MUStrTok(NULL,ENVRS_ENCODED_STR,&TokPtr);
      }  /* END while (ptr != NULL) */

    MUFree(&tmpe);
    }    /* END if (Env != NULL) */

  /* Setup a default path if not specified by the user and 
   * if we are not ignoring the default environment */

  if ((IgnoreDefaultEnv == FALSE) && (DefaultPathRequired == TRUE))
    {
    /* set default environment */

    if ((MSched.ServerPath != NULL) && (MSched.ServerPath[0] != '\0'))
      {
      MUEnvAddValue(&E,"PATH",MSched.ServerPath);
      }
    else
      {
      MUEnvAddValue(&E,"PATH","/bin:/usr/bin:/usr/local/bin");
      }
    }

  /* NOTE:  due to NFS root squash, chdir() may fail as root but be successful as user 
            (NYI - should change ordering) */

  if ((CWD != NULL) && strcmp(CWD,NONE))
    {
    /* if CWD[0] == '\0', chdir to $HOME */

    if (MOChangeDir(CWD,&E,TRUE) == FAILURE)
      {
      /* determine permissions on directory */

      /* apply secondary groups */

      /* attempt directory again */

      /* NYI */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"cannot change to directory '%s', err=%s",
          CWD,
          strerror(errno));
        }

      if (MOSSetEUID(MSched.UID) == -1)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
          MSched.UID,
          errno,
          strerror(errno));

        exit(1);
        }

      MUChildCleanUp(ifd,ofd,efd,tmpIFName,tmpOFName,tmpEFName);

      return(FAILURE);
      }  /* END if (MOChangeDir() == FAILURE) */

    MDB(1,fSTRUCT) MLog("INFO:     successfully changed directories to '%s' in %s\n",
      CWD,
      FName);
    }    /* END if (CWD != NULL) */

  /* restore permissions */

  if (MOSSetEUID(MSched.UID) == -1)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot restore EUID to '%d' for server (errno: %d '%s')\n",
      MSched.UID,
      errno,
      strerror(errno));

    exit(1);
    }

  if (MOSSetGID(MSched.GID) == -1)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot restore GID to '%d' for server (errno: %d '%s')\n",
      MSched.GID,
      errno,
      strerror(errno));

    exit(1);
    }

  /* prepare arguments for use in child */

  aindex = 0;

  AList[aindex++] = exec;

  if (Prefix != NULL)
    {
    MUStrCpy(tmpPrefix,"-c",sizeof(tmpPrefix));
    AList[aindex++] = tmpPrefix;
    MUStrCpy(tmpExec,SExec,sizeof(tmpExec));
    AList[aindex++] = tmpExec;
    }

  if ((Args != NULL) && (Args[0] != '\0'))
    {
    /* FORMAT:  <ARG>[{ \t}<ARG>]... */

    MUStrDup(&tmpArgs,Args);  /* free'd after fork() */

    if (tmpArgs != NULL)
      {
      /* Anything quoted should be handled as a single arg, rather than
       * delimited by space. The previous implementation used MUStrTok
       * which broke quoted arguments with spaces into multiple arguments.
       * A #define was used to opt-in to use MUStrTokE which treated the
       * quoted args correctly as a single argument.
       * On 7/30/09 scottmo made it default to use MUStrTokE. */

      ptr = MUStrTokE(tmpArgs," \t\n",&TokPtr);

      while (ptr != NULL)
        {
        AList[aindex++] = ptr;

        ptr = MUStrTokE(NULL," \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }
    }    /* END if (Args != NULL) */

  AList[aindex] = NULL;

  /* prepare possible umask changes */

  UMaskStr[0] = '\0';

  /* If this is a SLURM job and the UMask environment variable has been set, 
   * then set the UMask for the child process accordingly */

  if (Limits != NULL)
    {
    char *tmpPtr = NULL;
    int   UMaskLen = 0;

    tmpPtr = MUStrStr(Limits,"umask=",0,TRUE,FALSE);

    if (tmpPtr != NULL)
      {
      tmpPtr += strlen("umask=");

      UMaskLen = strspn(tmpPtr,"0123456789");

      strncpy(UMaskStr,tmpPtr,UMaskLen);

      UMaskStr[UMaskLen] = '\0';  /* terminate string--used below in child code*/
      }
    }

  /* perform the actual fork()!!!! */

  if (MSched.EnableFastSpawn == TRUE)
    {
    ForkFuncPtr = vfork;
    }
  else
    {
    ForkFuncPtr = fork;
    }

  if (((pid = ForkFuncPtr()) == -1))
    {
    /* fork failed */

    MDB(1,fSTRUCT) MLog("ALERT:    cannot fork child (errno: %d '%s')\n",
      errno,
      strerror(errno));

    if (EMsg != NULL)
      sprintf(EMsg,"cannot fork child, errno=%d - %s",
        errno,
        strerror(errno));

    MUChildCleanUp(ifd,ofd,efd,tmpIFName,tmpOFName,tmpEFName);

    return(FAILURE);
    }  /* END if (((pid = fork()) == -1)) */

  if (pid == 0)
    {
    /* child */
    /* WARNING--keep the amount of code in child block to a minimum--we could
     * be using vfork() which is very sensitive to what you do in the child (see manpage
     * for vfork()) */

    if (((GID != -1) && (setgid(GID) == -1)) ||
        ((UID != -1) && (setuid(UID) == -1)))
      {
      
      fprintf(stderr,"ERROR:  cannot set permissions to UID:%d/GID:%d (errno: %d  '%s')\n",
        UID,
        GID,
        errno,
        strerror(errno));

      _exit(1);  /* use _exit() instead of exit() to avoid flushing buffers twice */
      }

    if (EnvPipe != NULL)
      {
      close(EnvPipe[1]); /* close write end of pipe. */
      }

    if ((tmpIFName[0] != '\0') && (ifd != 0))
      {
      close(0);

      if (dup(ifd) == -1)
        {
        /* print an error message? */
        }

      close(ifd); 
      }  /* END if ((tmpIFName[0] != '\0') && (ifd != 0)) */

    if (tmpOFName[0] == '\0')
      {
      /* stdout not required, close stdout (NYI) */

      /* close(1); */
      }
    else if ((tmpOFName[0] != '\0') && (ofd != 1))
      {
      close(1);

      if (dup(ofd) == -1)
        {
        /* print an error message? */
        }

      close(ofd);
      }

    if (tmpEFName[0] == '\0')
      {
      /* stderr not required, close stderr (NYI) */

      /* close(2); */
      }
    else if ((tmpEFName[0] != '\0') && (efd != 2))
      {
      close(2);

      if (dup(efd) == -1)
        {
        /* print an error message? */
        }

      close(efd);
      }

    /* NOTE:  setsid() disconnects from controlling-terminal */

    if (setsid() == -1)
      {
      fprintf(stderr,"ERROR:  could not disconnect from controlling-terminal, errno=%d - %s\n",
        errno,
        strerror(errno));
      }

    if (UMaskStr[0] != '\0')
      {
      /* Note that the UMaskStr should be in decimal, not octal for atoi to work */

      umask(atoi(UMaskStr));
      }

#ifdef __MNOT
    /* TBD remove this log once disappearing env info bug is fixed RT3304 */ 
    MDB(3,fGRID)
     {
     if (E.List != NULL)
       {
       char tmpBuf[MMAX_BUFFER << 1] = {0};
       char *BPtr;
       int BSpace;
       int index;

       MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

       for (index = 0;E.List[index] != NULL;index++)
         {
         MUSNPrintF(&BPtr,&BSpace,"%s:",E.List[index]);
         }

       MLog("INFO:     environment of length %d passed in to fork/execve\n",
         strlen(tmpBuf));
       }
     else
       {
       MLog("INFO:     no environment passed into fork/execve\n");
       }
     }
#endif

    execve(exec,AList,(EnvPipe == NULL) ? E.List : NULL);

    fprintf(stderr,"ERROR:    cannot execute process '%s'  (errno: %d  '%s')\n",
      exec,
      errno,
      strerror(errno));

    _exit(1);
    }  /* END if (pid == 0) */

  /* parent */ 

  if ((EnvPipe != NULL) && (E.List != NULL))
    {
    int eindex;
    ssize_t  rc = 0;

    close(EnvPipe[0]); /* close read of pipe */

    for (eindex = 0;E.List[eindex] != NULL;eindex++)
      rc = write(EnvPipe[1],E.List[eindex],strlen(E.List[eindex]) + 1); /* +1 to get '\0' */

    close(EnvPipe[1]); /* close write pipe */

    MDB(9,fALL) MLog("INFO:    EnvPipe returned %ld\n",rc);
    }

  if (MSched.EnableFastSpawn == FALSE)
    {
    /* sleep for 0.1 seconds - just in case it finishes quickly */

    MUSleep(100000,TRUE);
    }

  /* parent, close files, clean-up */

  MUChildCleanUp(ifd,ofd,efd,NULL,NULL,NULL);

  MUEnvDestroy(&E);

  MUFree(&tmpArgs);

  if (PID != NULL)
    {
    *PID = pid;
    }

  if (BlockTime > 0)
    {
    /* wait for child to complete (up to BlockTime) */

    long timestep;

    int StatLoc;

    pid_t rc;

    mbool_t WaitError = FALSE;

    MDB(7,fSOCK) MLog("INFO:     waiting for child %d to process command '%s'\n",
      pid,
      exec);

    /* BlockTime is in milliseconds */

    for (timestep = 0;timestep < BlockTime;timestep+=10)
      {
      rc = waitpid(pid,&StatLoc,WNOHANG);

      if (rc == pid)
        {
        MDB(3,fSOCK) MLog("INFO:     command '%s' finished\n",
          exec);

        break;
        }
      else if (rc < 0)
        {
        if (errno == ECHILD) /* no child found with requested pid */
          {
          /* the child must have finished and the other thread reaped its info in another wait() call */
  
          /* we will assume that the exit status was a 0 - may not be a correct assumption
             we may want to create a hash table where both threads can share info on reaped pids NYI */
  
          StatLoc = 0;
  
          MDB(3,fSOCK) MLog("INFO:     command '%s' assumed finished\n",
            exec);

           }
        else
          {
          MDB(3,fSOCK) MLog("WARNING:  wait for child pid %d failed with errno: %d (%s)\n",
            pid,
            errno,
            strerror(errno));
  
          WaitError = TRUE;
          }

        break;
        } /* END (rc < 0) */

      /* wait 10 milliseconds */

      MUSleep(10*1000,TRUE); /* 1000 micro-seconds = 1 milli-second*/
      }  /* END for (timestep) */

    if (tmpOFName[0] != '\0')
      MUStrDup(OBuf,tmpOFName);

    if (tmpEFName[0] != '\0')
      MUStrDup(EBuf,tmpEFName);

    if (tmpIFName[0] != '\0')
      MUStrDup(IFileName,tmpIFName);

    if ((timestep < BlockTime) && (WaitError == FALSE))
      {
      /* child finished */

      if ((IBuf != NULL) && (tmpIFName[0] != '\0'))
        unlink(tmpIFName);

      if (SC != NULL)
        {
        *SC = WEXITSTATUS(StatLoc);
        }

      if (OType == mxoTrig)
        {
        mtrig_t *T;

        T = (mtrig_t *)O;

        T->ExitCode = WEXITSTATUS(StatLoc);

        if (T->ExitCode > 127)
          T->ExitCode = 127 - T->ExitCode;

        if (WIFEXITED(StatLoc))
          { 
          /* process exited normally */

          switch (T->ECPolicy)
            {
            case mtecpIgnore:

              T->State = mtsSuccessful;

              break;

            default:
            case mtecpNONE:
            case mtecpFailNeg:

              T->State = (T->ExitCode >= 0) ? mtsSuccessful : mtsFailure;

              break;

            case mtecpFailPos:

              T->State = (T->ExitCode <= 0) ? mtsSuccessful : mtsFailure;

              break;

            case mtecpFailNonZero:

              T->State = (T->ExitCode == 0) ? mtsSuccessful : mtsFailure;

              break;
            }  /* END switch (T->ECPolicy) */
          }
        else
          {
          /* process exited abnormally */

          T->State = mtsFailure;
          }
        }

      return(SUCCESS);
      }  /* END if (timestep < BlockTime) */

    /* timeout - child did not finish */

    if (OType == mxoTrig)
      {
      ((mtrig_t *)O)->State = mtsActive;
      }

    /* block time indicates wait N seconds for completion but
       allow process to continue in the background if not completed
       by blocktime - hence, if blocktime not satisfied, this does
       not constitute a failure */

    /* SC set time Timeout but return SUCCESS */

    if (SC != NULL)
      *SC = mscTimeout;

    MDB(3,fSOCK) MLog("WARNING:  command '%s' timed out or wait failed after %.2f seconds\n",
      exec,
      (double)timestep / 1000.0);

    return(SUCCESS);
    }  /* END if (BlockTime > 0) */
  else if (TimeLimit > 0)
    {
    int StatLoc;

    long timestep;

    pid_t rc;

    mbool_t WaitError = FALSE;

    /* wait for child to complete */

    for (timestep = 0;timestep < TimeLimit;timestep += 10)
      {
      rc = waitpid(pid,&StatLoc,WNOHANG);

      if (rc == pid)
        {
        MDB(3,fSOCK) MLog("INFO:     command '%s' completed\n",
          exec);

        break;
        }
      else if (rc < 0) 
        {
        if (errno == ECHILD) /* no child found with requested pid */
          {
          /* the child must have finished and the other thread reaped its info in another wait() call */

          /* we will assume that the exit status was a 0 - may not be a correct assumption
             we may want to create a hash table where both threads can share info on reaped pids NYI */

          StatLoc = 0;

          MDB(3,fSOCK) MLog("INFO:     command '%s' assumed completed\n",
            exec);
          }
        else
          {
          MDB(3,fSOCK) MLog("WARNING:  wait for child pid %d failed with errno: %d (%s)\n",
            pid,
            errno,
            strerror(errno));

          WaitError = TRUE;
          }
        break;
        }

      MUSleep(10000,TRUE);
      }  /* END for (timestep) */

    if (timestep >= TimeLimit)
      {
      /* TimeLimit expired, kill child */

      if (kill(pid,9) == -1)
        {
        MDB(0,fSOCK) MLog("ERROR:    cannot kill process %d (%s)\n",
          pid,
          SExec);
        }
      else
        {
        /* clear defunct child processes */

        MUClearChild(NULL);
        }

      MUChildCleanUp(-1,-1,-1,tmpIFName,tmpOFName,tmpEFName);

      if (EMsg != NULL)
        {
        /* sync text with error handling code in MMI.c */

        snprintf(EMsg,MMAX_LINE,"child process (%s) timed out and was killed", 
          SExec);
        }

      if (SC != NULL)
        *SC = -9;

      return(FAILURE);
      }
    else if (WaitError == TRUE)
      {
      if (SC != NULL)
        *SC = mscTimeout;

      MDB(3,fSOCK) MLog("WARNING:  command '%s' timed out or wait failed after %.2f seconds\n",
        exec,
        (double)timestep / 1000.0);

      return(FAILURE);
      }

    /* child successfully completed */

    if (SC != NULL)
      {
      *SC = WEXITSTATUS(StatLoc);
      }

    /* load stdout/stderr */

    if ((tmpOFName[0] != '\0') &&
       ((ptr = MFULoadNoCache(tmpOFName,1,NULL,NULL,NULL,NULL)) != NULL))
      {
      *OBuf = ptr;
      }
    else
      {
      if (EMsg != NULL)
        sprintf(EMsg,"cannot load output file %s", 
          tmpOFName);
      }

    if ((tmpEFName[0] != '\0') &&
       ((ptr = MFULoadNoCache(tmpEFName,1,NULL,NULL,NULL,NULL)) != NULL))
      {
      *EBuf = ptr;
      }
    else
      {
      if (EMsg != NULL)
        sprintf(EMsg,"cannot load error file %s",
          tmpEFName);
      }

    /* remove stdout/stderr temp files */

    MUChildCleanUp(-1,-1,-1,tmpIFName,tmpOFName,tmpEFName);
    }  /* END if (TimeLimit > 0) */
  else
    {
    /* child launched in background */

    if (tmpOFName[0] != '\0')
      MUStrDup(OBuf,tmpOFName);

    if (tmpEFName[0] != '\0')
      MUStrDup(EBuf,tmpEFName);

    if (tmpIFName[0] != '\0')
      MUStrDup(IFileName,tmpIFName);
    }

  return(SUCCESS);
  }  /* END MUSpawnChild() */

/* END MUSpawnChild.c */
