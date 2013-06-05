/* HEADER */

      
/**
 * @file MUCheckAuthFile.c
 *
 * Contains: MUCheckAuthFile
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Translate ':' delimited PATH string into array of valid paths.
 * 
 * @see MSchedInitialize() - parent
 * @see MUAddPath() - peer
 *
 * @param PathList (O) [minsize=MMAX_PATH]
 * @param Path     (I)
 */

int MUSetPathList(

  char **PathList,
  const char  *Path)

  {
  char *dir;
  char *TokPtr;

  int   pindex;

  char tmpLine[MMAX_LINE << 2];

  if ((PathList == NULL) || (Path == NULL))
    {
    return(FAILURE);
    }

  MUStrCpy(tmpLine,Path,sizeof(tmpLine));

  dir = MUStrTok(tmpLine,": \t\n",&TokPtr);

  pindex = 0;

  while (dir != NULL)
    {
    if (pindex >= MMAX_PATH)
      break;

    MUStrDup(&PathList[pindex],dir);

    pindex++;

    dir = MUStrTok(NULL,": \t\n",&TokPtr);
    }  /* END while (dir != NULL) */

  return(SUCCESS);
  }  /* END MUGetPathList() */


/**
 * initialize the PathList from a PATH environment string
 *
 * @param EnvList (I)   PATH environment string from (like from getenv("PATH"))
 */

int MUInitPathList(
 
  const char     *EnvList
    
  )

  {
  return(MUSetPathList(MSched.PathList,EnvList));
  }

/**
 * return PathList reference
 */

char **MUGetPathList(void)
  {
  return(MSched.PathList);
  }



/**
 * Add specified Path to PathList
 *
 * @see MUGetPathList() - peer
 *
 * @param PathList (I) [modified]
 * @param Path (I)
 */

int MUAddPathList(

  char       **PathList,
  const char  *Path)

  {
  int pindex;

  if ((Path == NULL) || (PathList == NULL))
    {
    return(FAILURE);
    }

  for (pindex = 0;pindex < MMAX_PATH;pindex++)
    {
    if (PathList[pindex] == NULL)
      {
      MUStrDup(&PathList[pindex],Path);

      return(SUCCESS);
      }

    if (!strcmp(PathList[pindex],Path))
      {
      return(SUCCESS);
      }
    }  /* END for (pindex) */

  return(FAILURE);
  }  /* END MUAddPathList() */


/**
 * Added new path to the existing path list
 *
 * @param Path (I)
 */

int MUAddToPathList(

  const char  *Path)

  {
  return(MUAddPathList(MSched.PathList,Path));
  }



 
/**
 * Return TRUE if all elements of path are owned by root or UID and are not 
 * writable by other.
 *
 * @param Path (I)
 * @param UID (I)
 */

mbool_t MUPathIsSecure(

  const char *Path,
  int         UID)

  {
  char *ptr;

  int   PathPerm;
  int   PathUID;

  char  tmpLine[MMAX_LINE];

  if ((Path == NULL) || (UID < 0))
    {
    return(FALSE);
    }

  strncpy(tmpLine,Path,sizeof(tmpLine));

  while (tmpLine[0] != '\0')
    {
    if (MFUGetAttributes(
          tmpLine,
          &PathPerm,
          NULL,
          NULL,
          &PathUID,
          NULL,
          NULL,
          NULL) == FAILURE)
      {
      return(FALSE);
      }

    if ((PathPerm & S_IWOTH) || 
       ((PathUID != 0) && (PathUID != UID)))
      {
      return(FALSE);
      }

    ptr = strrchr(tmpLine,'/');

    if (ptr == NULL)
      break;

    *ptr = '\0';
    }  /* END while (ptr != NULL) */

  return(TRUE);
  }  /* END MUPathIsSecure() */


/**
 * Extract and return the client secret key from its source file
 * which file could be one of the following in search order:
 *
 *    Environment variable filename
 *    filename in sched structure member  'KeyFile'
 *    filename  as per sched structure member 'CfgDir'/MSCHED_KEYFILE
 *    filename  as per MSCHED_KEYFILE 
 *
 * @param S             (I) [optional]
 * @param KeyBuf        (O) [optional,minsize=MMAX_LINE+1,sync w/KeyBufSize]
 * @param FileExists    (O) [optional]
 * @param UseKeyFile    (O) [optional]
 * @param EMsg          (O) [optional,minsize=MMAX_LINE]
 * @param IsServer      (I)
 */

int MUCheckAuthFile(

  msched_t *S,         
  char     *KeyBuf,   
  mbool_t  *FileExists, 
  mbool_t  *UseKeyFile,
  char     *EMsg,     
  mbool_t   IsServer) 

  {
  char tmpFileName[MMAX_LINE];

  mbool_t AuthFileOK = FALSE;
  mbool_t IsPrivate;

  int UID;

  int KeyBufSize = MMAX_LINE + 1;

  char *ptr;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* determine file name based on:  
   *    1)  Environment 
   *    2)  Sched->KeyFile or 
   *    3)  Sched->Cfgdir/MSCHED_KEYFILE  */

  if (((ptr = getenv(MAUTH_FILE_ENV_OVERRIDE)) != NULL))
    {
    MUStrCpy(tmpFileName,ptr,sizeof(tmpFileName));
    }
  else if (S != NULL)
    {
    /* Use the S->KeyFile if non-empty */
    if (S->KeyFile[0] != '\0')
      {
      MUStrCpy(tmpFileName,S->KeyFile,sizeof(tmpFileName));
      }
    else
      {
      /* Use the Configuration directory coupled with the filename */
      sprintf(tmpFileName,"%s/%s",
        S->CfgDir,
        MSCHED_KEYFILE);
      }
    }
  else
    { 
    /* last resort, just the relative keyfile name */
    strcpy(tmpFileName,MSCHED_KEYFILE);
    }

  /* check existence */

  if (MFUGetAttributes(
        tmpFileName,
        NULL,
        NULL,
        NULL,
        &UID,
        NULL,
        &IsPrivate,
        NULL) == SUCCESS)
    {
    /* Private is OWNER ONLY accessible */
    if (IsPrivate == TRUE)
      {
      if (IsServer == TRUE)
        {
        /* if non-root primary admin, require .moab.key file to match */

        if ((S != NULL) && ((UID == S->UID) || (S->UID == 0))) 
          {
          AuthFileOK = TRUE;
          }
        else
          {
          if ((EMsg != NULL) && (S != NULL))
            {
            char tmpUName[MMAX_NAME];

            snprintf(EMsg,MMAX_LINE,"keyfile '%s' is not owned by user %s",
              tmpFileName,
              MUUIDToName(S->UID,tmpUName));
            }
          }
        }
      else
        {
        AuthFileOK = TRUE;
        }
      }
    else
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"keyfile '%s' does not have permissions owner read-only permissions (ie, 400)",
          tmpFileName);
      }

    if (AuthFileOK == TRUE)
      {
      if (MUPathIsSecure(tmpFileName,UID) == FALSE)
        {
        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"keyfile '%s' located in non-secure path",
            tmpFileName);

        AuthFileOK = FALSE;
        }
      }

    if (FileExists != NULL)
      *FileExists = TRUE;
    }    /* END if (MFUGetAttributes() == SUCCESS) */
  else
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"cannot locate keyfile '%s'",
        tmpFileName);

    if (FileExists != NULL)
      *FileExists = FALSE;
    }
  
  if (AuthFileOK == FALSE)
    {
    if (UseKeyFile != NULL)
      *UseKeyFile = FALSE;

    if (KeyBuf != NULL)
      {
      #ifdef MBUILD_SKEY
        strncpy(KeyBuf,MBUILD_SKEY,KeyBufSize);
      #else
        strncpy(KeyBuf,"m0@b",KeyBufSize);
      #endif
      }

    #ifdef __MREQKEYFILE
    return(FAILURE);
    #else /* __MREQKEYFILE */
    return(SUCCESS);
    #endif /* __MREQKEYFILE */
    }  /* END if (AuthFileOK == FALSE) */

  if (UseKeyFile != NULL)
    *UseKeyFile = TRUE;

  /* only load the key for server because key is protected by file ownership/permissions
     client will never directly access key but will create checksum via suid mauth tool */

  if ((IsServer == TRUE) && (KeyBuf != NULL))
    {
    char *ptr;

    int   i;

    /* load key file */

    if ((ptr = MFULoad(tmpFileName,1,macmRead,NULL,NULL,NULL)) == NULL)
      {
      /* cannot load data */

      MDB(1,fSTRUCT) MLog("ERROR:    cannot read key file '%s' (errno: %d '%s')\n",
        tmpFileName,
        errno,
        strerror(errno));

      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"cannot load file '%s'",
          tmpFileName);

      if (UseKeyFile != NULL)
        *UseKeyFile = FALSE;

      return(FAILURE);
      }

    MUStrCpy(KeyBuf,ptr,KeyBufSize);

    for (i = strlen(KeyBuf) - 1;i > 0;i--)
      {
      if (!isspace(KeyBuf[i]))
        break;

      KeyBuf[i] = '\0';
      }  /* END for (i) */
    }    /* END if ((IsServer == TRUE) && (KeyBuf != NULL)) */
 
  return(SUCCESS);
  }  /* END MUCheckAuthFile() */

/* END MUCheckAuthFile.c */
