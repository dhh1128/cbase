/* HEADER */

      
/**
 * @file MUtilReadPipe2.c
 *
 * Contains: MUReadPipe2()
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



#ifndef __SANSI__


/**
 * Runs the given command and stores the output.
 *
 * NOTE:  MUReadPipe() will not capture stderr 
 * NOTE:  MUReadPipe() will block indefinitely 
 * @see MUReadPipe2() 

 * BLOCKS - no timeout 
 *
 * @param Command (I) - command plus args
 * @param SBuffer (I) [optional]
 * @param OBuffer (O) [optional]
 * @param OBufSize (I)
 * @param SC (O) [optional]
 */

int MUReadPipe(

  const char           *Command,   /* I - command plus args */
  char                 *SBuffer,   /* I (optional) */
  char                 *OBuffer,   /* O (optional) */
  int                   OBufSize,  /* I */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  FILE *fp;
  int   rc;
  int   CmdRC;

  char  tmpModeStr[MMAX_NAME];

  mbool_t Overflow = FALSE;

  const char *FName = "MUReadPipe";

  MDB(5,fSOCK) MLog("%s(%s,%.64s,OBuffer,%d,SC)\n",
    FName,
    (Command != NULL) ? Command : "NULL",
    (SBuffer != NULL) ? SBuffer : "NULL",
    OBufSize);

  /* initialize output */

  if (SC != NULL)
    *SC = mscNoError;

  if (OBuffer != NULL)
    {
    OBuffer[0] = '\0';
    }

  /* check parameters */

  if (Command == NULL)
    {
    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  /* NOTE:  stderr may be merged with stdout into OBuffer */

  tmpModeStr[0] = '\0';

  if (OBuffer != NULL)
    strcat(tmpModeStr,"r");

  if (SBuffer != NULL)
    strcat(tmpModeStr,"w");

  if (tmpModeStr[0] == '\0')
    strcat(tmpModeStr,"r");

  if ((OBuffer != NULL) && (SBuffer != NULL))
    {
    MDB(0,fSOCK) MLog("ERROR:    cannot open bi-directional pipe in MUReadPipe\n");

    return(FAILURE);
    }

  if ((fp = popen(Command,tmpModeStr)) == NULL)
    {
    MDB(0,fSOCK) MLog("ERROR:    cannot open pipe on command '%s', errno: %d (%s)\n",
      Command,
      errno,
      strerror(errno));

    if (SC != NULL)
      {
      /* NOTE:  should look at errno (NYI) */

      *SC = mscNoEnt;
      }

    return(FAILURE);
    }

  if (SBuffer != NULL) 
    {
    fprintf(fp,"%s",
      SBuffer);
    }

  if (OBuffer != NULL)
    {
    OBuffer[OBufSize - 1] = '\0';

    if ((rc = fread(OBuffer,1,OBufSize,fp)) == -1)
      {
      MDB(0,fSOCK) MLog("ERROR:    cannot read pipe on command '%s', errno: %d (%s)\n",
        Command,
        errno,
        strerror(errno));

      pclose(fp);

      if (SC != NULL)
        {
        /* NOTE:  should look at errno (NYI) */

        *SC = mscSysFailure;
        }

      return(FAILURE);
      }

    /* terminate buffer */

    if (OBuffer[OBufSize - 1] != '\0')
      {
      /* overflow detected */

      Overflow = TRUE;
      }

    OBuffer[MIN(OBufSize - 1,rc)] = '\0';

    MDB(5,fSOCK) MLog("INFO:     pipe(%s) -> '%s'\n",
      Command,
      OBuffer);
    }  /* END if (OBuffer != NULL) */

  CmdRC = pclose(fp);

  if (SC != NULL)
    {
    if (CmdRC != 0)
      {
      *SC = mscRemoteFailure;
      }
    }

  if (Overflow == TRUE)
    {
    if (SC != NULL)
      *SC = mscNoMemory;

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUReadPipe() */

#endif /* __SANSI__ */




#define MDEF_PIPETIMEOUT 3000  /* 3 seconds (in milliseconds) */

/**
 * Execute command and report resulting output/error.
 * 
 * NOTE:  Will use P->Timeout (milliseconds) for Timeout if specified, otherwise
 *        Timeout is MDEF_PIPETIMEOUT.
 * NOTE:  Will use P->R->Env to extract exec environment
 *
 * NOTE: Not threadsafe!
 *
 * @see MUSpawnChild() - child
 * @see MUReadPipe() - peer
 *
 * @param Command  (I) - command plus args
 * @param SBuffer  (I) [optional]
 * @param OBuffer  (O) [optional]
 * @param EBuffer  (O) [optional]
 * @param P        (I) [optional]
 * @param RC       (O) [optional] process exit code
 * @param EMsg     (O) [optional,minsize=MMAX_LINE]
 * @param SC       (O) [optional]
 *
 * @return FAILURE if the underlying command returns a non-zero exit code or if
 *  MSched.ChildStderrCheck is TRUE and stderr contains they keyword ERROR. Returns
 *  SUCCESS otherwise.
 */

int MUReadPipe2(

  const char           *Command,   /* I - command plus args */
  const char           *SBuffer,   /* I (optional) */
  mstring_t            *OBuffer,   /* O (optional) */
  mstring_t            *EBuffer,   /* O (optional) */
  mpsi_t               *P,         /* I (optional) */
  int                  *RC,        /* O (optional) */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  const char *FName = "MUReadPipe2";

  int rc;

  int tmpSC;

  int fsize;

  char *ArgBuf;
  char *EnvBuf;

  char *TokPtr = NULL;

  char *SubmitCmd = NULL;

  char *OBuf = NULL;
  char *EBuf = NULL;

  char  tEMsg[MMAX_LINE];
  char *EMsgP;

  mbool_t DoFail = FALSE;
  mbool_t IsURL = FALSE;

  mulong  BlockTime;

  MDB(4,fSTRUCT) MLog("%s(%s,%.16s,OBuffer,%s,P,EMsg,SC)\n",
    FName,
    (Command != NULL) ? Command : "",
    (SBuffer != NULL) ? SBuffer : "NULL",
    (EBuffer != NULL) ? "EBuffer" : "");

  if (OBuffer != NULL)
    MStringSet(OBuffer,"\0");

  if (EBuffer != NULL)
    MStringSet(EBuffer,"\0");

  if (RC != NULL)
    *RC = 0;

  if (SC != NULL)
    *SC = mscNoError;

  mstring_t tmpLine(MMAX_LINE);

  EMsgP = (EMsg != NULL) ? EMsg : tEMsg;
   
  EMsgP[0] = '\0';

  MStringSet(&tmpLine,Command);

  /* FORMAT:  '<EXEC>[ <ARG>][ <ARG>]...' or '<EXEC>[?<ARG>][&<ARG>]...' */

  if (strchr(tmpLine.c_str(),'?') != NULL)
    IsURL = TRUE;

  SubmitCmd = MUStrTok(tmpLine.c_str()," ?\t\n",&TokPtr);

  ArgBuf = MUStrTok(NULL,"\n",&TokPtr);

  if (IsURL == TRUE)
    {
    MUStrReplaceChar(ArgBuf,'&',' ',TRUE);
    }

  /* MUSpawnChild wants blocktime in milliseconds */
  /* P->Timeout is microseconds? (at least when called from MSecGetChecksum() for "munge" code */

  if ((P != NULL) && (P->Timeout > 0))
    BlockTime = P->Timeout / 1000;   /* convert timeout in microseconds to BlockTime in milliseconds */
  else
    BlockTime = MDEF_PIPETIMEOUT;

  if ((P != NULL) && (P->R != NULL))
    EnvBuf = ((mrm_t *)P->R)->Env;
  else
    EnvBuf = NULL;

  /* Perform the fork of the target command */
  rc = MUSpawnChild(
          SubmitCmd,
          NULL,           /* job name */
          ArgBuf,         /* args */
          NULL,           /* no prefix */
          MSched.UID,     /* UID */
          MSched.GID,     /* GID */
          NULL,           /* cwd */
          EnvBuf,         /* env */
          NULL,
          NULL,           /* no limits */
          SBuffer,        /* stdin */
          NULL,           /* no input file name */
          (OBuffer != NULL) ? &OBuf : NULL,  /* stdout */
          (EBuffer != NULL) ? &EBuf : NULL,  /* stderr */
          NULL,           /* no PID  */
          &tmpSC,         /* child exit code */
          0,              /* background completion time */
          BlockTime,      /* (milliseconds) */
          NULL,           /* no Object  */
          mxoNONE,        /* no Otype  */
          FALSE,          /* CheckEnv no */
          FALSE,          /* IgnoreDefault Env no */
          EMsgP);         /* O - errors */

  /* NOTE:  if BlockTime is specified, OBuf and EBuf are returned with name of temp files 
            containing stdout and stderr */

  if ((OBuffer != NULL) && (OBuf != NULL))
    {
    char *ptr;

    ptr = MFULoadNoCache(OBuf,1,NULL,EMsgP,NULL,&fsize);

    MDB(5,fSTRUCT) MLog("INFO:     file %s %s loaded (%d bytes)  %s\n",
      OBuf,
      (ptr != NULL) ? "successfully" : "unsuccessfully",
      fsize,
      EMsgP);
      
    MFURemove(OBuf);

    if (ptr != NULL)
      {
      MStringSet(OBuffer,ptr);

      MUFree(&ptr);
      }
    else
      {
      MDB(3,fSOCK) MLog("ERROR:    cannot load stdout file %s\n",
        OBuf);
      }

    MUFree(&OBuf);
    }  /* END if ((OBuffer != NULL) && (OBuf != NULL)) */

  if ((EBuffer != NULL) && (EBuf != NULL))
    {
    char *ptr;

    ptr = MFULoadNoCache(EBuf,1,NULL,NULL,NULL,NULL);

    MFURemove(EBuf);

    if (ptr != NULL)
      {
      MStringSet(EBuffer,ptr);

      MUFree(&ptr);
      }
    else
      {
      MDB(3,fSOCK) MLog("ERROR:    cannot load stderr file %s\n",
        EBuf);
      }

    MUFree(&EBuf);
    }  /* END if ((EBuffer != NULL) && (EBuf != NULL)) */
  
  if ((EBuffer != NULL) && (EBuffer->empty()) && (EMsgP[0] != '\0'))
    {
    MStringSet(EBuffer,EMsgP);
    }
  else if ((EBuffer != NULL) && (EMsgP != NULL) && (!EBuffer->empty()))
    {
    MUStrCpy(EMsgP,EBuffer->c_str(),MMAX_LINE);
    }

  if (RC != NULL)
    *RC = tmpSC;

  /* convert tmpSC to SC */

  if (tmpSC != 0)
    {
    if (tmpSC == 4)
      {
      /* service needs to be restarted */

      if ((P != NULL) && (P->R != NULL))
        {
        bmset(&P->R->IFlags,mrmifRMStartRequired);
        }
      }

    if (SC != NULL)
      *SC = mscRemoteFailure;

    return(FAILURE);
    }

  if ((rc == FAILURE) || (DoFail == TRUE))
    {
    if (EMsgP[0] != '\0')
      {
      MDB(2,fSTRUCT) MLog("INFO:     failure in %s, '%s'\n",
        FName,
        EMsgP);
      }

    if (SC != NULL)
      *SC = mscRemoteFailure;

    return(FAILURE);
    }

  if ((MSched.ChildStdErrCheck == TRUE) && (EBuffer != NULL) && 
      (strstr(EBuffer->c_str(),"ERROR") != NULL))
    {
    MDB(2,fSTRUCT) MLog("INFO:     failure in %s detected by the presence of "
      "ERROR in stderr, '%s'\n",
      FName,
      EMsgP);

    if (SC != NULL)
      *SC = mscRemoteFailure;

    return(FAILURE);
    }

  if ((OBuffer != NULL) && (OBuffer->empty()) && 
      (EBuffer != NULL) && (!EBuffer->empty()))
    {
    /* only stderr reported */

    MDB(5,fRM) MLog("WARNING:  request succeeded with no stdout but stderr='%s'\n",
      (EBuf != NULL) ? EBuf : "");
    }

  if (tmpSC == mscTimeout) 
    {
    if (SC != NULL)
      *SC = (enum MStatusCodeEnum)tmpSC;

    snprintf(EMsgP,MMAX_LINE,"timeout executing '%s' after %.2f seconds",
      SubmitCmd,
      (double)BlockTime / 1000.0);

    MDB(5,fSTRUCT) MLog("INFO:     failure in %s, timeout executing '%s' after %.2f seconds",
      FName,
      SubmitCmd,
      (double)BlockTime / 1000.0);
    
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUReadPipe2() */

/* END MUtilReadPipe2.c */
