/* HEADER */

      
/**
 * @file MWikiJobSubmit.c
 *
 * Contains: MWikiJobSubmit
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"


/**
 * Writes the given environment variables (\036 delimited) to a file.
 *
 * @param J Job with credentials to write file.
 * @param EnvBuf (I) List of variables that are \036 delimited.
 * @param EnvFile (O) The name of the EnvFile to be returned.
 * @param EnvFileSize (I) The size of the EnvFile buffer.
 * @param EMsg (I) Error message to return.
 */

int __WriteEnvToFile(

  mjob_t *J,
  char   *EnvBuf,
  char   *EnvFile, 
  int     EnvFileSize,
  char   *EMsg)

  {
  char *envPtr = NULL;
  char *tmpEnv = NULL;
  char *TokPtr = NULL;
  char  spoolDir[MMAX_LINE];

  int fd;
  int rc;
  ssize_t count = 0;

  MASSERT(J != NULL,"null j when writing env to file");
  MASSERT(J->Credential.U != NULL,"null user when writing env to file");
  MASSERT(J->Credential.G != NULL,"null group when writing env to file");
  MASSERT(EnvBuf != NULL,"env buf is null when writing env to file");
  MASSERT(EnvFile != NULL,"env file is null when writing env to file");

  if (MUStrIsEmpty(EnvBuf))
    return(FAILURE);
    
  MUGetSchedSpoolDir(spoolDir,sizeof(spoolDir));

  snprintf(EnvFile,EnvFileSize,"%s/%s",spoolDir,"moab.env.XXXXXX");

  errno = 0;
  if ((fd = mkstemp(EnvFile)) < 0)
    {
    if (EMsg != NULL)
      {
      sprintf(EMsg,"cannot create temp env file '%s' (errno: %d, '%s')",
        EnvFile,
        errno,
        strerror(errno));
      }

    return(FAILURE);
    }

  fchmod(fd,0600);
  rc = fchown(fd,J->Credential.U->OID,J->Credential.G->OID);

  MDB(9,fALL) MLog("INFO:     chown returned %d\n",rc);

  MUStrDup(&tmpEnv,EnvBuf);

  envPtr = MUStrTok(tmpEnv,ENVRS_ENCODED_STR,&TokPtr);

  while (envPtr != NULL)
    {
    count = write(fd,envPtr,strlen(envPtr) + 1); /* NULL terminated */

    envPtr = MUStrTok(NULL,ENVRS_ENCODED_STR,&TokPtr);
    }  /* END while (envPtr != NULL) */

  MDB(9,fALL) MLog("INFO:     write returned %ld\n",count);

  close(fd);

  return(SUCCESS);
  } /* END int __WriteEnvToFile() */


/**
 * Stores the job's environment and any SLURM specific variables into EnvBuf.
 *
 * @param J (I)
 * @param R (I)
 * @param EnvBuf (O) Must be constructed by caller 
 */

void __BuildJobEnv(

  mjob_t *J,
  mrm_t  *R,
  mstring_t *EnvBuf)

  {
  MStringAppendF(EnvBuf,"%s%s%s",
    (J->Env.BaseEnv != NULL) ? J->Env.BaseEnv : "",
    (J->Env.BaseEnv != NULL) ? ENVRS_ENCODED_STR : "",
    (J->Env.IncrEnv != NULL) ? J->Env.IncrEnv : "");

  if (R->ConfigFile[0] != '\0')
    {
    if (!bmisset(&J->Flags,mjfInteractive))
      {
      MStringAppendF(EnvBuf,"%sSLURM_CONF=%s",
        (!EnvBuf->empty()) ? ENVRS_ENCODED_STR : "",
        R->ConfigFile);
      }
    }

#ifdef __MHAVEPSUB
  {
  char *ptr;
  char *tail;

  char tmpDir[MMAX_LINE];
  char SubmitDir[MMAX_LINE];

  MStringAppendF(EnvBuf,"%sPSUB_JOBID=%s",
    (!EnvBuf->empty()) ? ENVRS_ENCODED_STR : "",
    J->Name);

  ptr = strstr(EnvBuf->c_str(),"PCS_TMPDIR");

  if (ptr != NULL)
    {
    ptr += strlen("PCS_TMPDIR=");

    tail = strchr(ptr,ENVRS_ENCODED_CHAR);

    if (tail != NULL)
      *tail = '\0';

    MUStrCpy(tmpDir,ptr,sizeof(tmpDir));

    if (tail != NULL)
      *tail = ENVRS_ENCODED_CHAR;
    }
  else
    {
    tmpDir[0] = '\0';
    }

  MJobGetIWD(J,SubmitDir,sizeof(SubmitDir),TRUE);

  MStringAppendF(EnvBuf,"%sPSUB_WORKDIR=%s",
    (!EnvBuf->empty()) ? ENVRS_ENCODED_STR : "",
    (tmpDir[0] != '\0') ? tmpDir : SubmitDir);
  }
#endif /* __MHAVEPSUB */

  if (J->Env.SubmitDir != NULL)
    {
    MStringAppendF(EnvBuf,"%sSLURM_SUBMIT_DIR=%s",
      (!EnvBuf->empty()) ? ENVRS_ENCODED_STR : "",
      J->Env.SubmitDir);
    }

  if (bmisset(&J->IFlags,mjifEnvRequested) == TRUE) /* if -E was specified */
    {
    /* Put moab job environment into job submission. Later when slurm adds functionality
     * to modify a submitted job's environment variables, then do this work after 
     * submission and before starting so that all the environment variables can be populated.*/

    mstring_t tmpEnvVars(MMAX_BUFFER);

    if (MJobGetEnvVars(J,ENVRS_ENCODED_CHAR,&tmpEnvVars) == FAILURE)
      {
      MDB(7,fPBS) MLog("ERROR:    failed getting job's environment variables\n");
      }
    else
      {
      MStringAppendF(EnvBuf,"%s%s",
        (!EnvBuf->empty()) ? ENVRS_ENCODED_STR : "",
        tmpEnvVars.c_str());
      }
    }
  } /* END void __BuildJobEnv() */


#define WIKISUBMIT_TIMEOUT 10000  /* in milliseconds */

/**
 * Submits a job via the Wiki interface to a Wiki-compatible resource manager. SLURM is one
 * example of such a Wiki-enabled resource manager.
 *
 * @see MRMJobSubmit()
 * @see MPBSJobSubmit()
 * @see MS3JobSubmit()
 * 
 * NOTE:  If job is interactive, populate J->RMSubmitString and return
 *
 * @param SubmitString (I) [optional]  [unused]
 * @param R (I)
 * @param JP (I/O)
 * @param FlagBM (I) [stage command file, do not submit job to RM]  [unused]
 * @param JobID (O) [minsize=MMAX_NAME]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]  [unused]
 */

int MWikiJobSubmit(

  const char           *SubmitString,
  mrm_t                *R,
  mjob_t              **JP,
  mbitmap_t            *FlagBM,
  char                 *JobID,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  int  rc;

  char SubmitExe[MMAX_LINE];     /* full path of submit command */


  const char *WikiDelim = "?";
  const char *WikiEqStr = ":";
  char SubmitDir[MMAX_LINE];
  char tmpLine[MMAX_LINE];
  char PTYString[MMAX_LINE];
  char SpoolDir[MMAX_LINE];
  char CmdFile[MMAX_LINE];
  char tmpFlagLine[MMAX_LINE];
  char Limits[MMAX_LINE];
  char tmpStr[MMAX_LINE];
  char EnvFile[MMAX_LINE] = {0};
  int  EnvPipe[2]; /* used to pipe environment variables to sbatch */
  mbool_t usePipe = FALSE;

  char tmpJobSearchLine[MMAX_NAME];  /* search string to match when srun/sbatch reports job id */

  char   *OBuf = NULL;
  char   *EBuf = NULL;

  int     fd;

  mjob_t *J;
  mreq_t *RQ;

  int     tmpSC;

  enum MNodeAccessPolicyEnum NAPolicy;
  mbool_t NAShared = FALSE;

  char   *TokPtr;
  char   *ptr;
  mbool_t SubmitCmdSalloc = FALSE;
  mbool_t SubmitCmdSbatch = FALSE;
  mbool_t TcshFOptionFound = FALSE;
  char   *WCKeyPtr;

  char   *RspOutput;

  mbitmap_t tmpRMXSetAttrs;

  const char *FName = "MWikiJobSubmit";

  MDB(1,fPBS) MLog("%s(SubmitString,%s,%s,%ld,%s,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    ((JP != NULL) && (*JP != NULL)) ? (*JP)->Name : "NULL",
    FlagBM,
    (JobID != NULL) ? "JobID" : "NULL");

  if (JobID != NULL)
    JobID[0] = '\0';

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  Limits[0] = '\0';

  if ((JP == NULL) || (*JP == NULL) || (R == NULL))
    return(FAILURE);

  J = *JP;

  if ((R->Version > 10200) && !bmisset(&J->Flags,mjfInteractive))
    {
    /* use sbatch */

    /* SLURM 1.3 and higher no longer supports srun */

    if ((R->Version >= 10300) && (R->SubmitCmd != NULL))
      {
      char *tmpPtr = strrchr(R->SubmitCmd,'/');

      if ((strcmp(R->SubmitCmd,"srun") == 0) || 
          ((tmpPtr != NULL) && (strcmp(tmpPtr,"/srun") == 0)))
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"use sbatch instead of srun for slurm version 1.3.x - modify submit cmd '%s' in moab.cfg",
            R->SubmitCmd);
          }

        return(FAILURE);
        }
      }

    MUStrCpy(
      tmpLine,
      (R->SubmitCmd != NULL) ? R->SubmitCmd : (char *)MDEF_SLURMSBATCHPATHNAME,
      sizeof(tmpLine));

    if (strstr(tmpLine,"sbatch") != NULL)
      {
      SubmitCmdSbatch = TRUE;
      }
    }
  else if ((R->Version >= 10225) && bmisset(&J->Flags,mjfInteractive))
    {
    /* use salloc for interactive jobs since it only starts one shell for the user */

    char  tmpPTY[MMAX_LINE];
    char *tmpPtr = NULL;

    MUStrCpy(
      tmpLine,
      (R->SubmitCmd != NULL) ? R->SubmitCmd : (char *)MDEF_SLURMSALLOCPATHNAME,
      sizeof(tmpLine));
    
    /* If srun or sbatch is at the end of the submit command then replace it with salloc */

    if ((strcmp(tmpLine,"srun") == 0) || (strcmp(tmpLine,"sbatch") == 0))
      {
      tmpPtr = tmpLine;
      }
    else
      {
      tmpPtr = strrchr(tmpLine,'/');

      if ((tmpPtr != NULL) && 
          ((strcmp(tmpPtr,"/srun") == 0) || (strcmp(tmpPtr,"/sbatch") == 0)))
        {
        tmpPtr++; /* increment tmpPtr to the start of "srun"  or "sbatch" */
        }
      else
        {
        tmpPtr = NULL;
        }
      }

    if (tmpPtr != NULL)
      {
      MUStrCpy(
        tmpPtr,
        "salloc",
        sizeof(tmpLine) - (tmpPtr - tmpLine));

      SubmitCmdSalloc = TRUE;
      }
    else if (strstr(tmpLine,"salloc") != NULL)
      {
      SubmitCmdSalloc = TRUE;
      }

    if (R->PTYString != NULL)
      {
      if (strcasecmp(R->PTYString,"$salloc") == 0) /* SallocDefaultCommand specfied in slurm.conf */
        PTYString[0] = '\0';
      else
        MUStrCpy(PTYString,R->PTYString,sizeof(PTYString));
      }
    else
      {
      /* build pty string */

      MUStrCpy(tmpPTY,tmpLine,sizeof(tmpPTY));
      MUStrRemoveFromEnd(tmpPTY,"salloc");
      MUStrCat(tmpPTY,"srun",sizeof(tmpPTY));

      if (MFUGetFullPath(
          tmpPTY,
          MUGetPathList(),
          R->Wiki.SBinDir, 
          NULL,
          TRUE,
          PTYString,  /* O */
          sizeof(PTYString)) == FAILURE)
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot locate path to srun for interactive jobs '%s'",
            tmpPTY);
          }

        return(FAILURE);
        }

      MUStrCat(PTYString," -n1 -N1 --pty ",sizeof(PTYString));
      }

    } /* Interactive Job */
  else
    {
    /* use srun */

    MUStrCpy(
      tmpLine,
      (R->SubmitCmd != NULL) ? R->SubmitCmd : (char *)MDEF_SLURMSRUNPATHNAME,
      sizeof(tmpLine));
    }

  ptr = strrchr(tmpLine,'/');

  if (R->Version >= 20100)
    {
    snprintf(tmpJobSearchLine,sizeof(tmpJobSearchLine),"Submitted batch job");
    }
  else if (R->Version >= 10300)
    {
    snprintf(tmpJobSearchLine,sizeof(tmpJobSearchLine),"%s: Submitted batch job",
      (ptr != NULL) ? ptr + 1 : tmpLine);
    }
  else
    {
    snprintf(tmpJobSearchLine,sizeof(tmpJobSearchLine),"%s: jobid",
      (ptr != NULL) ? ptr + 1 : tmpLine);
    }

  if (MFUGetFullPath(
       tmpLine,
       MUGetPathList(),
       R->Wiki.SBinDir, 
       NULL,
       TRUE,
       SubmitExe,  /* O */
       sizeof(SubmitExe)) == FAILURE)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot locate path to submit client '%s'",
        tmpLine);
      }

    return(FAILURE);
    }

  MJobGetIWD(J,SubmitDir,sizeof(SubmitDir),TRUE);

  if ((!bmisset(&J->Flags,mjfInteractive)) && (J->RMSubmitString != NULL))
    {
    /* create command script */

    MUGetSchedSpoolDir(SpoolDir,sizeof(SpoolDir));

    errno = 0;

    snprintf(CmdFile,sizeof(CmdFile),"%s/%s",
      SpoolDir,
      "moab.job.XXXXXX");

    if ((fd = mkstemp(CmdFile)) < 0)
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"cannot create temp file in spool directory '%s' (errno: %d, '%s')",
          SpoolDir,
          errno,
          strerror(errno));
        }

      return(FAILURE);
      }

    close(fd);

    /* write file to spool directory */

    MUStrDup(&J->Env.Cmd,CmdFile);

    if (J->RMSubmitString != NULL)
      {
      mstring_t CmdBuf(MMAX_LINE);
      char *ptr;
      char *tcshPtr = NULL;


      if ((J->Env.Shell != NULL) && (strncmp(J->RMSubmitString,"#!",strlen("#!"))))
        {
        /* shell specified and magic not specified - insert shell */

        MStringAppendF(&CmdBuf,"#!%s\n%s",
          J->Env.Shell,
          J->RMSubmitString);

        ptr = CmdBuf.c_str();
        }
      else if ((J->Env.Shell == NULL) && (strncmp(J->RMSubmitString,"#!",strlen("#!"))) && (R->Version >= 10300))
        {
        /* shell not specified and magic not specified - insert /bin/sh shell */
        /* Note that with slurm version 1.3 we are using sbatch instead of srun and sbatch
         * requires that we provide a shell unlike srun */

        MStringAppendF(&CmdBuf,"#!/bin/sh\n%s",
          J->RMSubmitString);

        ptr = CmdBuf.c_str();
        }
      else
        {
        if ((J->Env.Shell != NULL) && (strncmp(J->RMSubmitString,"#!",strlen("#!"))))
          {
          /* shell specified and magic not specified - insert shell */

          MStringAppendF(&CmdBuf,"#!%s\n%s",
            J->Env.Shell,
            J->RMSubmitString);

          ptr = CmdBuf.c_str();
          }
        else if ((J->Env.Shell == NULL) && (strncmp(J->RMSubmitString,"#!",strlen("#!"))) && (R->Version >= 10300))
          {
          /* shell not specified and magic not specified - insert /bin/sh shell */
          /* Note that with slurm version 1.3 we are using sbatch instead of srun and sbatch
           * requires that we provide a shell unlike srun */

          MStringAppendF(&CmdBuf,"#!/bin/sh\n%s",
            J->RMSubmitString);

          ptr = CmdBuf.c_str();
          }
        else
          {
          ptr = J->RMSubmitString;
          }
        }

      if ((tcshPtr = strstr(ptr,"/tcsh ")) != NULL)
        {
        tcshPtr += strlen("/tcsh ");

        while (isspace(*tcshPtr))
          tcshPtr++;

        if ((tcshPtr[0] == '-') && (tcshPtr[1] == 'f'))
          TcshFOptionFound = TRUE;
        }

      /* strip out rm directives with slurm */

      if (R->Version > 10200)
        {
        MURemoveRMDirectives(ptr,mrmtPBS);
        }

      if (MFUCreate(
           J->Env.Cmd,
           NULL,
           (void *)ptr,
           strlen(ptr),
           S_IXUSR|S_IRUSR,
           -1,
           -1,
           TRUE,
           NULL) == FAILURE)
        {
        MFURemove(CmdFile);

        if (EMsg != NULL)
          strcpy(EMsg,"cannot create command file");

        return(FAILURE);
        }
      }    /* END if (J->RMSubmitString != NULL) */
    }      /* END if (!bmisset(&J->Flags,mjfInteractive)) */

  mstring_t ArgLine(MMAX_LINE);

  if (J->RMSubmitFlags != NULL)
    {
    /* pass opaque resource manager flags to SLURM job submit */

    MStringAppendF(&ArgLine," %s",
      J->RMSubmitFlags);
    }

  /* NOTE:  should we reject migration if UID or GID is unknown? */

  if ((J->Credential.U != NULL) && (J->Credential.U->OID > 0))
    { 
    if (!bmisset(&J->Flags,mjfInteractive))
      {
      MStringAppendF(&ArgLine," --uid=%d",
        (int)J->Credential.U->OID);
      }
    }
  else
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid uid for user");

    MFURemove(CmdFile);

    return(FAILURE);
    }

  /* Import user environment unless full env is required or if tcsh -f was
   * specified. --get-user-env runs su - <username> -c /usr/bin/env which
   * bypasses the -f request. */

  if ((R->Version >= 10125) && 
      (!bmisset(&J->IFlags,mjifEnvSpecified)) && 
      (TcshFOptionFound == FALSE))
    {
    if ((R->Version >= 10219) && (SubmitCmdSbatch == TRUE) && (MSched.MWikiSubmitTimeout > 2))
      {
      /* llnl recommends that we make the get user env timeout 2 seconds less than the submit timeout */

      MStringAppendF(&ArgLine," --get-user-env=%d",
                 MSched.MWikiSubmitTimeout - 2);
      }
    else if (SubmitCmdSalloc == FALSE)
      {
      MStringAppend(&ArgLine," --get-user-env");
      }
    }

  if ((J->Credential.G != NULL) && (J->Credential.G->OID > 0))
    {
    if (!bmisset(&J->Flags,mjfInteractive))
      {
      MStringAppendF(&ArgLine," --gid=%d",
        (int)J->Credential.G->OID);
      }
    }

  if (!bmisset(&J->Flags,mjfInteractive))
    {
    if ((R->Version <= 10200) || (strstr(tmpLine,"srun") != NULL))
      {
      /* NOTE:  srun -b is deprecated - use sbatch in newer SLURM releases */

      MStringAppend(&ArgLine," -b");
      }
    }
  else if (SubmitCmdSalloc == FALSE)
    {
    /* for srun interactive jobs, route info unbuffered */

    MStringAppend(&ArgLine," -u");
    }

  if (!bmisset(&J->Flags,mjfInteractive) && (R->Version >= 10200) &&
      (SubmitCmdSalloc == FALSE))
    {
    /* salloc does not support --no-requeue and --requeue */

    if (J->ResFailPolicy == marfCancel)
      {
      MStringAppend(&ArgLine," --no-requeue");
      }
    else if ((J->ResFailPolicy == marfRequeue) && (R->Version >= 10300))
      {
      MStringAppend(&ArgLine," --requeue");
      }
    }

  if ((!bmisset(&J->Flags,mjfInteractive)) && (R->Version >= 10300) && (J->Depend != NULL))
    {
    mstring_t  dependStr(MMAX_LINE);
    
    if (((MDependToString(&dependStr,J->Depend,mrmtWiki)) != FAILURE) &&
        (!dependStr.empty())) /* dependees completed / not found */
      {
      ArgLine += " -P ";
      ArgLine += dependStr;

      /* remove dependeny line from RMXString because the dependency could 
       * have been a job name which was already expanded to jobids. If 
       * the job name is still in the extension string, it will be 
       * processed again and add later jobs with the same name. */

      if (J->RMXString != NULL)
        {
        if ((J->RMXString[0] == 'x') && (J->RMXString[1] == '='))
          MUStrRemoveFromList(J->RMXString + 2,"depend",';',TRUE);
        else
          MUStrRemoveFromList(J->RMXString,"depend",';',TRUE);
        }
      }
    else
      {
      MDB(2,fWIKI) MLog("WARNING:  failed to add dependecies to job %s's submission\n",J->Name);
      }
    }

  /* this will trump whatever what just done, won't it? */

  if ((!bmisset(&J->Flags,mjfInteractive) || !bmisset(&J->IFlags,mjifWCNotSpecified)) &&
      (R->Version > 10200) &&
      (SubmitCmdSalloc == FALSE))
    {
    if (bmisset(&J->Flags,mjfRestartable))
      {
      MStringAppend(&ArgLine," --requeue");
      }
    else 
      {
      /* by default jobs will not be requeued */

      MStringAppend(&ArgLine," --no-requeue");
      }
    }

  if (!bmisset(&J->Flags,mjfInteractive) || !bmisset(&J->IFlags,mjifWCNotSpecified))
    {
    /* set time if job is not interactive or wclimit explicitly specified. 
     * mjifWCNotSpecifed is unset, in MJobSetDefaults, when CLASSCFG[] DEFAULT.WCLIMIT is 
     * specified so that the interactive jobs will have the default wclimit. If DEFAULT.WCLIMIT 
     * is not set then the interactive job will get slurm's partition's default wclimit. */

    MStringAppendF(&ArgLine," -t %ld",
      MAX(1,(J->WCLimit + MCONST_MINUTELEN - 1) / MCONST_MINUTELEN));
    }

  MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);

  if ((NAPolicy == mnacNONE) || (NAPolicy == mnacShared) || (NAPolicy == mnacSharedOnly))
    {
    NAShared = TRUE;
    }

  MDB(7,fWIKI) MLog("INFO:     job '%s' NC %d, TC %d, TPN %d, TTC %d, NAPolicy '%s', Exact Node Match '%s'\n",
    J->Name,
    J->Request.NC,
    J->Request.TC,
    J->Req[0]->TasksPerNode,
    J->TotalTaskCount,
    (NAShared == TRUE) ? "shared" : "not shared",
    (bmisset(&MPar[0].JobNodeMatchBM,mnmpExactNodeMatch)) ? "set" : "not set");

  if (R->IsBluegene)
    {
    /* msub -l nodes=2 specifies 2 c-nodes (sbatch -N 2) in SLURM which is translated as procs in moab */
    
    if (!bmisset(&J->IFlags,mjifNoResourceRequest))
      MStringAppendF(&ArgLine," -N %d",J->Request.TC);
    }
#ifdef MNOT
  /* 7-31-09 RT5277 BOC - LANL was running into issues with --cpus-per-task setting the SLURM_CPUS_PER_TASK env 
   * variable which caused mpi to handle the job wrong. --cpus-per-task is now removed and -n is used in it's place.
   * Jobs that request ppn now look like: msub -l nodes=2:ppn=4 => salloc -N 2-2 -n 8 --mincpus=4 */ 

  else if ((SubmitCmdSalloc == TRUE) && (J->Req[0]->TasksPerNode > 1) && (J->Request.NC != 0))
    {
    /* Note that we only use the "-N" (no "-n") if we are going to add --cpus-per-task 
     * for salloc commands with TasksPerNode > 1 (see below in the code) */

    /* example  msub -I -l nodes=2:ppn=4 */

    MStringAppendF(&ArgLine," -N %d",  
      J->Request.NC);
    }
#endif
  else if (NAShared == TRUE)
    {
    /* Note that J->Request.NC is not set if we are not ExactNodeMatch */

    /* If mjifProcsSpecified is set then -l procs requested which is treated
     * as procs instead of nodes. */

    if (((bmisset(&MPar[0].JobNodeMatchBM,mnmpExactNodeMatch)) && 
         (bmisset(&J->Flags,mjfProcsSpecified) == FALSE)) && /* -l procs overrides extactnode */
        (J->Request.NC > 0))
      {
      if (J->TotalTaskCount > 0)
        {
        /* example msub -l nodes=1,ttc=7 */

        MDB(7,fWIKI) MLog("INFO:     shared job '%s' requested %d node(s) and ttc %d\n",
          J->Name,
          J->Request.NC,
          J->TotalTaskCount);

        MStringAppendF(&ArgLine," -N %d -n %d",  
          J->Request.NC,
          J->TotalTaskCount);
        }
      else if (J->Req[0]->TasksPerNode == 0)
        {
        /* example msub -l nodes=5 */

        MDB(7,fWIKI) MLog("INFO:     shared job '%s' requested %d node(s)\n",
          J->Name,
          J->Request.NC);

        MStringAppendF(&ArgLine," -N %d",  
          J->Request.NC);
        }
      else
        {
        /* example msub -l nodes=1:ppn=3 */
        /*         msub -l nodes=1,tpn=3 */

        MDB(7,fWIKI) MLog("INFO:     shared job '%s' requested %d nodes and %d tasks per node\n",
          J->Name,
          J->Request.NC,
          J->Req[0]->TasksPerNode);

        MStringAppendF(&ArgLine," -N %d-%d -n %d",
          J->Request.NC,
          J->Request.NC,
          J->Request.TC);
        }
      } /* END if (J->Request.NC > 0) */
    else if (J->TotalTaskCount > 0)
      {
      /* example msub -l ttc=5 */

      MDB(7,fWIKI) MLog("INFO:     shared job '%s' requested ttc %d\n",
        J->Name,
        J->TotalTaskCount);

      MStringAppendF(&ArgLine," -N 1 -n %d",  
        J->TotalTaskCount);
      }
    else
      {
      /* example msub -l nodes=5 */

      MDB(7,fWIKI) MLog("INFO:     shared job '%s' requested tasks %d\n",
        J->Name,
        J->Request.TC);

      MStringAppendF(&ArgLine," -n %d",  
        J->Request.TC);
      }
    } /* END if (NAShared == TRUE) */
  else if ((NAShared != TRUE) && 
           ((bmisset(&MPar[0].JobNodeMatchBM,mnmpExactNodeMatch)) &&
            (bmisset(&J->Flags,mjfProcsSpecified) == FALSE))) /* -l procs override exactnode */
    {
    MDB(7,fWIKI) MLog("INFO:     job '%s' is ExactNodeMatch and single node access policy\n",
      J->Name);

    /* -N specifies nodes, -n specifies processors */

    if (J->Request.NC > 0)
      {
      MStringAppendF(&ArgLine," -N %d-%d",  
        J->Request.NC,
        J->Request.NC);

      if (J->TotalTaskCount != 0)
        {
        MStringAppendF(&ArgLine," -n %d",  
          J->TotalTaskCount);
        }
      else if ((J->Req[0]->TasksPerNode > 0) && (J->Request.TC > 0)) 
        {
        MStringAppendF(&ArgLine," -n %d",  
          J->Request.TC);
        }
      }
    else if (!MNLIsEmpty(&J->ReqHList)) /* a hostlist was specified so NC is 0 */
      {
      if (J->Req[0]->TasksPerNode > 0)  /* e.g. nodes=2:ppn=8,hostlist=node1+node2 */
        {
        MStringAppendF(&ArgLine," -N %d",
          J->Request.TC / J->Req[0]->TasksPerNode); /* from the above example TC will be 16 and TasksPerNode will be 8 */
        }
      else  /* e.g. nodes=2,hostlist=node1+node2 */
        {
        MStringAppendF(&ArgLine," -N %d",
          J->Request.TC);
        }
      }
    else if (J->Req[0]->TasksPerNode > 1)
      {
      MStringAppendF(&ArgLine," -N %d",
        J->Request.TC / J->Req[0]->TasksPerNode);
      }
    else if (MPar[0].MaxPPN > 0)
      {
      MStringAppendF(&ArgLine," -N %d",
        J->Request.TC / MPar[0].MaxPPN);
      }
    else
      {
      /* as fallback, request one node per task */

      MStringAppendF(&ArgLine," -N %d", 
        J->Request.TC);
      }
    }    /* END if (bmisset(&MPar[0].JobNodeMatchBM,mnmpExactNodeMatch)) */
  else
    {
    /* request procs */

    MDB(7,fWIKI) MLog("INFO:     job '%s' is single node access policy\n",
      J->Name);

    MStringAppendF(&ArgLine," -n %d",  
      J->Request.TC);
    }

  if (J->Env.Output != NULL)
    {
    char tmpDir[MMAX_LINE];
    char OFile[MMAX_LINE];

    MJobResolveHome(J,J->Env.Output,OFile,sizeof(OFile));

    if ((MSched.RelToAbsPaths) && (OFile[0] != '/'))
      {
      /* 4-29-09 BOC PNNL RT3894 - slurm 1.2.30 has a bug with relative paths.
       * With FSREMOTE and if the directory that moab is started from exists on the compute nodes
       * a relative path error/output file is written to the directory that moab was started on.
       * If the directory doesn't exist the error/output files are written in the IWD as
       * slurm-XXX.out/err. To get around this append the IWD to the relative path. */

      char tmpIWD[MMAX_LINE];

      MJobGetIWD(J,tmpIWD,sizeof(tmpDir),TRUE);

      sprintf(tmpDir,"%s%s%s",
        tmpIWD,
        (tmpIWD[strlen(tmpIWD) - 1] == '/') ? "" : "/",
        OFile);
      }
    else
      {
      /* if (Version == 10230) absolute path */

      sprintf(tmpDir,"%s",OFile);
      }
      
    MStringAppendF(&ArgLine," -o %s",tmpDir);
    }

  if (J->Env.Error != NULL)
    {
    char tmpDir[MMAX_LINE];
    char EFile[MMAX_LINE];
    
    MJobResolveHome(J,J->Env.Error,EFile,sizeof(EFile));

    if ((MSched.RelToAbsPaths == TRUE) && (EFile[0] != '/'))
      {
      /* 4-29-09 BOC PNNL RT3894 - slurm 1.2.30 has a bug with relative paths.
       * With FSREMOTE and if the directory that moab is started from exists on the compute nodes
       * a relative path error/output file is written to the directory that moab was started on.
       * If the directory doesn't exist the error/output files are written in the IWD as
       * slurm-XXX.out/err. To get around this append the IWD to the relative path. */

      char tmpIWD[MMAX_LINE];

      MJobGetIWD(J,tmpIWD,sizeof(tmpDir),TRUE);

      sprintf(tmpDir,"%s%s%s",
        tmpIWD,
        (tmpIWD[strlen(tmpIWD) - 1] == '/') ? "" : "/",
        EFile);
      }
    else
      {
      /* if (Version == 10230) absolute path */

      sprintf(tmpDir,"%s",EFile);
      }

    MStringAppendF(&ArgLine," -e %s",tmpDir);
    }

  if (J->Env.IWD != NULL)
    {
    char tmpDir[MMAX_LINE];

    MJobGetIWD(J,tmpDir,sizeof(tmpDir),TRUE);

    MStringAppendF(&ArgLine," -D %s",
      tmpDir);
    }

  if (J->AName != NULL)
    {
    if (strchr(J->AName,' ') != NULL)
      {
      /* If there are spaces in the name then encapsulate the name with single spaces */

      MStringAppendF(&ArgLine," -J '%s'",
        J->AName);
      }
    else
      {
      MStringAppendF(&ArgLine," -J %s",
        J->AName);
      }
    }

  RQ = J->Req[0];

#ifdef _MNOT
  /*
   * This feature has been removed per LLNL request RT1532
   */

  if (bmisclear(&RQ->ReqFBM) == FALSE)
    {
    char Delim;
    char tmpLine[MMAX_LINE];

    Delim = (RQ->ReqFMode == mclOR) ? '|' : '&';

    MUStrCpy(
      tmpLine,
      MUMAToString(meNFeature,Delim,RQ->ReqFBM,sizeof(RQ->ReqFBM)),
      sizeof(tmpLine));

    if ((tmpLine[0] != '\0') && (strcmp(tmpLine,NONE) != 0))
      {
      MStringAppendF(&ArgLine,"  -C %s",
        tmpLine);
      }
    }    /* END if (bmisclear(&RQ->ReqFBM) == FALSE) */
#endif /* __MNOT */

  if (RQ->TasksPerNode > 1)
    {
    MStringAppendF(&ArgLine," --mincpus=%d",
      RQ->TasksPerNode);
    }

  if (RQ->DRes.Mem > 0)
    {
    /* NYI - Note that in Slurm v1.3 the option will change to --task-mem=<MB> and
     * applies on a per task basis (see comment in RT1941). */

    MStringAppendF(&ArgLine," --mem=%d",
      RQ->DRes.Mem);
    }

#ifdef _MNOT
  /*
   * --vmem not available with srun, salloc, or sbatch
   */

  if (RQ->DRes.Swap > 0)
    {
    MStringAppendF(&ArgLine," --vmem=%d",
      RQ->DRes.Swap);
    }
#endif /* __MNOT */

  if (RQ->DRes.Disk > 0)
    {
    MStringAppendF(&ArgLine," --tmp=%d",
      RQ->DRes.Disk);
    }

  if ((J->SpecSMinTime > 0) && (J->SpecSMinTime > MSched.Time))
    {
    mulong Time;

    Time = (J->SpecSMinTime - MSched.Time) / MCONST_MINUTELEN;

    MStringAppendF(&ArgLine," --begin=now+%ldminutes",
      Time);
    }

  /* Do not send moab node features to srun because slurm will reject the jobs */

  /* disable constaints */

  /*

  MUStrCpy(
    tmpLine,
    MUMAToString(meNFeature,',',RQ->ReqFBM,sizeof(RQ->ReqFBM)),
    sizeof(tmpLine));

  if ((tmpLine[0] != '\0') && strcmp(tmpLine,NONE))
    {
    MUSNPrintF(&BPtr,&BSpace," --constraint=%s",
      tmpLine);
    }

  */

  if (!MNLIsEmpty(&J->ReqHList))
    {
    mstring_t tmpHList(MMAX_LINE);
    int       nindex;

    for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,NULL) == SUCCESS;nindex++)
      {
      MStringAppendF(&tmpHList,"%s%s",
        (nindex != 0) ? "," : "",
        MNLGetNodeName(&J->ReqHList,nindex,NULL));
      }  /* END for (nindex) */

    MStringAppendF(&ArgLine," -w %s",
      tmpHList.c_str());
    }  /* END if (J->ReqHList != NULL) */


  if (NAShared == TRUE)
    {
    /* mark job as shared */

    MStringAppend(&ArgLine," -s");
    }

  if (!MNLIsEmpty(&J->ExcHList))
    {
    mstring_t tmp(MMAX_LINE);

    if (MJobAToMString(
        J,
        mjaExcHList,
        &tmp) == FAILURE)
      {
      MDB(3,fPBS) MLog("ERROR:    failed to add exclude nodelist to submission\n");
      }
    else
      {
      MStringAppendF(&ArgLine," -x %s",tmp.c_str());
      }
    }

  /* NOTE:  if local job is going to be destroyed, all needed job info including QoS,
            exemptions, nokill, etc  must be pushed into '--comment' */

  /* NYI */

  /* add extension flags */

  mstring_t RMXBuf(MMAX_LINE);
  mstring_t CommentLine(MMAX_LINE);

  /* NOTE:  since this is routing into WIKI which uses ';' as a field delimiter, the RMXString
            must be delimited using another character */

  /* For a period of time this function, MRMXToString, didn't work because a
   * key part was missing in MJobProccessExtenstionString. The key part was 
   * that the attrs * weren't being added to the J->RMXSetAttrs bitmap. Once 
   * this was added all rmxattrs that were set would be printed out. This is 
   * good but can cause issues when slurm is holding the extension natively 
   * and we it is stored in the comment, for ex. dependendencies (-P) and 
   * excludenodes (-x). Problems occur when the attr is modified. It is 
   * modified in slurm but not in the comment. In order to avoid this, the 
   * rm extentions that exist natively in slurm shouldn't be printed out 
   * in MRMXToString. */

  bmcopy(&tmpRMXSetAttrs,&J->RMXSetAttrs);

  bmunset(&tmpRMXSetAttrs,mxaExcHList);
  bmunset(&tmpRMXSetAttrs,mxaDependency);
   
  MRMXToMString(J,&RMXBuf,&tmpRMXSetAttrs); 

  bmclear(&tmpRMXSetAttrs);

  if (!RMXBuf.empty())
    {
    /* concatenate specified resource manager extensions */

    MUStrReplaceChar(RMXBuf.c_str(),';','?',FALSE);
    MStringSet(&CommentLine,RMXBuf.c_str());
    }  /* END if (rmxBuf != 0) */

  /* Check for features that did not come through the RMXString (e.g. psub -c) */

  if (strstr(RMXBuf.c_str(),"feature") == NULL)
    {
    /* Do we have any features? */

    if (!bmisclear(&RQ->ReqFBM))
      {
      char Delim;
      char tmpBuf[MMAX_LINE];

      /* features may have been entered using the psub -c <feat>,<feat> command 
         and do not come in via the RMXString. Note that our psub script
         only supports comma-delimited features which are in logical-AND mode. */

      /* check in case someday we support OR mode */

      Delim = (RQ->ReqFMode == mclOR) ? '|' : ':'; 

      MUNodeFeaturesToString(Delim,&RQ->ReqFBM,tmpBuf);

      if (!MUStrIsEmpty(tmpBuf))
        {
        /* Feature is added to the comment in the format "feature:abc:xyz?" */

        MStringInsertArg(&CommentLine,"feature",tmpBuf,WikiEqStr,WikiDelim);
        }
      } /* END if (!bmisclear(!RQ->ReqFBM)) */
    }   /* END if (strstr(rmxBuf,"feature") == NULL) */

  /* With jobmigratepolicy=immediate all job information will be lossed including umask. 
   * UMASK will be parsed as an RM extension string only to get the umask back after 
   * deleting the job's information. */
  
  if ((strstr(CommentLine.c_str(),"UMASK") == NULL) &&
      (MSched.JobMigratePolicy == mjmpImmediate) && (J->Env.UMask != 0))
    {
    char tmpMask[5]; /* 4 digits + NULL */

    snprintf(tmpMask,sizeof(tmpMask),"%d",J->Env.UMask);

    MStringInsertArg(&CommentLine,"UMASK",tmpMask,WikiEqStr,WikiDelim);
    }

  if ((!bmisset(&J->Flags,mjfInteractive)) || 
      ((MSched.UseMoabJobID == TRUE) && (J->SystemJID != NULL)))
    {
    /* submit with sid/sjid pointers to reconnect with parent job */

    /* system JID */

    if (J->SystemJID != NULL)
      {
      MStringInsertArg(&CommentLine,"SJID",J->SystemJID,WikiEqStr,WikiDelim);

      MStringInsertArg(&CommentLine,"SID",J->SystemID,WikiEqStr,WikiDelim);
      }
    else if (!bmisset(&J->IFlags,mjifUseDRMJobID))
      {
      MStringInsertArg(&CommentLine,"SJID",J->Name,WikiEqStr,WikiDelim);

      MStringInsertArg(&CommentLine,"SID","moab",WikiEqStr,WikiDelim);
      }
    }    /* END if (!bmisset(&J->Flags,mjfInteractive)) */
  else
    {
    /* job is interactive - msub will perform direct job submission and no
       parent job will be created */

    /* NO-OP */
    }

  if ((strstr(CommentLine.c_str(),MRMXAttr[mxaResFailPolicy]) == NULL) &&
      (J->ResFailPolicy != marfNONE))
    {
    /* set nokill */

    MStringInsertArg(
      &CommentLine,
      (char *)MRMXAttr[mxaResFailPolicy],
      (char *)MARFPolicy[J->ResFailPolicy],
      WikiEqStr,
      WikiDelim);
    }

  if (bmisset(&J->IFlags,mjifMergeJobOutput))
    {
    /* set stdout/stderr join */

    /* NYI */
    }

  /* reservation constraints */

  if ((strstr(CommentLine.c_str(),"ADVRES") == NULL) && 
      (bmisset(&J->Flags,mjfAdvRsv)))
    {
    MStringInsertArg(
      &CommentLine,
      (char *)"ADVRES",
      (J->ReqRID != NULL) ? J->ReqRID : (char *)"ALL",
      WikiEqStr,
      WikiDelim);
    }

  /* QOS request */

  if (strstr(CommentLine.c_str(),MRMXAttr[mxaQOS]) == NULL) 
    {
    /* J->QReq should already be in CommentLine from MRMXToString if it was requested. */

    if (J->QOSRequested != NULL)
      {
      MStringInsertArg(
        &CommentLine,
        "QOS",
        J->QOSRequested->Name,
        WikiEqStr,
        WikiDelim);
      }  /* END if (J->QReq != NULL) */
    else if (J->Credential.Q != NULL)
      {
      /* Assigned QOS preserved w/in SLURM comment field - see RT1771 */

      MStringInsertArg(
        &CommentLine,
        "QOS",
        J->Credential.Q->Name,
        WikiEqStr,
        WikiDelim);
      }  /* END else if (J-.Cred.Q != NULL) */
    }
   

  /* NOTE:  no flag overflow checking */

  tmpFlagLine[0] = '\0';

  if (bmisset(&J->SpecFlags,mjfPreemptee))
    {
    /* no direct support in WIKI, use RM extensions */

    if (tmpFlagLine[0] != '\0')
      strcat(tmpFlagLine,",");

    strcat(tmpFlagLine,MJobFlags[mjfPreemptee]);
    }

  if (tmpFlagLine[0] != '\0')
    {
    MStringInsertArg(
      &CommentLine,
      "FLAGS",
      tmpFlagLine,
      WikiEqStr,
      WikiDelim);
    }

  /* include GRes info in the comment field */

  if ((J->Req[0] != NULL) && (!MSNLIsEmpty(&J->Req[0]->DRes.GenericRes)))
    {
    int  gindex;
    mstring_t tmpBuf(MMAX_LINE);

    for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
      {
      if (MGRes.Name[gindex][0] == '\0')
        break;

      if (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,gindex) == 0)
        continue;

      MStringAppendF(&tmpBuf,"%s%s:%d",
        (!tmpBuf.empty()) ? "%" : "", 
        MGRes.Name[gindex],
        MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,gindex));
      } /* END for ... */

    MStringInsertArg(
      &CommentLine,
      "gres",
      tmpBuf.c_str(),
      "=",
      WikiDelim);
    } /* END  if ((J->Req[0] != NULL) && (J->Req[0]->DRes.GRes[0].count > 0)) */ 

  if (!CommentLine.empty())
    {
    /* NOTE:  no support for SLURM RM extensions (use account) */

    if (MWikiHasComment == TRUE)
      {
      MStringAppendF(&ArgLine," --comment='%s'",
        CommentLine.c_str());
      }
    else 
      {
      /* NOTE:  some SLURM instances do not support 'comments' for SLURM RM 
                extensions (use account) */

      MStringAppendF(&ArgLine," -U '%s'",
        CommentLine.c_str());
      }
    }  /* END if (!CommentLine.empty()) */


  /* extract wckey if present in the RMXString and add it to slurm command */

  if ((R->Version >= 10312) && 
      (J->RMXString != NULL) && 
      ((WCKeyPtr = strstr(J->RMXString,"wckey:")) != NULL))
    {
    WCKeyPtr += strlen("wckey:");
    MUStrCpy(tmpStr,WCKeyPtr,sizeof(tmpStr));
    MUStrTok(tmpStr,";",&TokPtr);

    MStringAppendF(&ArgLine," --wckey=%s",
      tmpStr);
    }

  if (J->ResFailPolicy != marfNONE)
    {
    /* set nokill */

    MStringAppendF(&ArgLine," --no-kill");
    }

  if (!bmisclear(&J->NotifyBM))
    {
    if (bmisset(&J->NotifyBM,mntAll))
      {
      MStringAppendF(&ArgLine," --mail-type=%s",
        "all");
      }
    else 
      {
      if (bmisset(&J->NotifyBM,mntJobStart))
        {
        MStringAppendF(&ArgLine," --mail-type=%s",
          "begin");
        }

      if (bmisset(&J->NotifyBM,mntJobEnd))
        {
        MStringAppendF(&ArgLine," --mail-type=%s",
          "end");
        }

      if (bmisset(&J->NotifyBM,mntJobFail))
        {
        MStringAppendF(&ArgLine," --mail-type=%s",
          "fail");
        }
      }

    if (J->NotifyAddress != NULL)
      {
      MStringAppendF(&ArgLine," --mail-user=%s",
        J->NotifyAddress);
      }
    }    /* END if (J->NotifyBM != 0) */

  if (!bmisset(&J->Flags,mjfInteractive))
    {
    if (R->SyncJobID == TRUE)
      {
      char *jptr;

      /* FORMAT:  <jobindex>|<schedname>.<jobindex> */

      /* NOTE:  jobid must be >= 1 */

      jptr = strchr(J->Name,'.');

      if (jptr == NULL)
        jptr = J->Name;
      else
        jptr++;

      MStringAppendF(&ArgLine," --jobid=%s",
        jptr);
      }  /* END if (R->SyncJobID == TRUE) */
    }    /* END if (!bmisset(&J->Flags,mjfInteractive)) */

  if (J->Credential.C != NULL)
    {
    /* map class back to partition */

    MStringAppendF(&ArgLine," --partition=%s",
      J->Credential.C->Name);
    }

  if ((J->Credential.A != NULL) && (MWikiHasComment == TRUE))
    {
    MStringAppendF(&ArgLine," --account=%s",
      J->Credential.A->Name);
    }

  /* put the job's and other envrionment variables in EnvBuf */
  mstring_t EnvBuf(MMAX_LINE);              /* env settings */

  __BuildJobEnv(J,R,&EnvBuf);

  /* write env to file to give to slurm because sbatch is exec'ed as root. */
  if ((!bmisset(&J->Flags,mjfInteractive)) &&
      (MSched.SubmitEnvFileLocation != NULL) &&
      (!MUStrIsEmpty(EnvBuf.c_str())))
    {
    if (strcmp(MSched.SubmitEnvFileLocation,"PIPE") == 0)
      {
      if (pipe(EnvPipe) == -1)
        {
        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"ERROR:    failed to open pipe for sbatch environment variables: %s\n",strerror(errno));

        return(FAILURE);
        }

      usePipe = TRUE;
      
      MStringAppendF(&ArgLine," --export-file=%d",EnvPipe[0]);

      /* leave EnvBuf intact. MUSpawnChild will write envbuf to the pipe. */
      }
    else  /* default to FILE in spooldir */
      {
      if (__WriteEnvToFile(J,EnvBuf.c_str(),EnvFile,sizeof(EnvFile),EMsg) == FAILURE)
        {
        MFURemove(CmdFile);

        return(FAILURE);
        }
      else
        {
        MStringAppendF(&ArgLine," --export-file=%s", EnvFile);

        /* null out EnvBuf so the env isn't passed to execve() */
        EnvBuf.clear();  /* reset string */
        }
      }
    }

  /* NOTE:  specify '--pty' for shell access to route stdout/stdin to/from user terminal */

  if ((!bmisset(&J->Flags,mjfInteractive)) && (J->RMSubmitString != NULL))
    {
    /* NOTE:  interactive jobs do not require command file */

    MStringAppendF(&ArgLine," %s",
      CmdFile);
    }
  else if (J->Env.Cmd != NULL)
    {
    char cmdPath[MMAX_LINE];
    
    MJobResolveHome(J,J->Env.Cmd,cmdPath,sizeof(cmdPath));

    MStringAppendF(&ArgLine," %s",
      cmdPath);
    }
  else if (R->Version <= 10300)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"interactive shell access not supported in this %s",
        (R->SubType == mrmstSLURM) ? "version of SLURM" : "RM");

    MFURemove(CmdFile);
    MFURemove(EnvFile);

    if (usePipe == TRUE)
      {
      close(EnvPipe[0]);
      close(EnvPipe[1]);
      }

    return(FAILURE);
    }
  else if ((J->Env.Shell != NULL) && (R->PTYString == NULL))
    {
    /* execute shell */

    /* NOTE:  Only SLURM 1.3.0 and higher provides 'tty' support */

    if ((strstr(J->Env.Shell,"/bash") != NULL) || (strstr(MDEF_SHELL,"/sh") != NULL))
      {
      MStringAppendF(&ArgLine," %s %s%s",
        PTYString,
        J->Env.Shell,
        (SubmitCmdSalloc == FALSE) ? " -i":"");
      }
    else
      {
      MStringAppendF(&ArgLine," %s %s",
        PTYString,
        J->Env.Shell);
      }
    }
  else if (R->PTYString == NULL)
    {
    if ((strstr(MDEF_SHELL,"/bash") != NULL) || (strstr(MDEF_SHELL,"/sh") != NULL))
      {
      MStringAppendF(&ArgLine," %s %s%s",
        PTYString,
        MDEF_SHELL,
        (SubmitCmdSalloc == FALSE) ? " -i":"");
      }
    else
      {
      MStringAppendF(&ArgLine," %s %s",
        PTYString,
        MDEF_SHELL);
      }
    }
  else if (R->PTYString != NULL) /* PTYString specified in moab.cfg */
    {
    char *tmpShellPtr = NULL;

    if ((J->Env.Shell != NULL) && ((tmpShellPtr = strstr(R->PTYString,"$SHELL")) != NULL))
      {
      char *tmpPTYStr = NULL;
      int   tmpPTYSize;

      /* $SHELL exists in the pty string. Replace it with the requested shell. */

      MUStrDup(&tmpPTYStr,R->PTYString);

      tmpPTYSize = strlen(tmpPTYStr) + 1;

      MUBufReplaceString(
        &tmpPTYStr,
        &tmpPTYSize,
        tmpShellPtr - R->PTYString,
        strlen("$SHELL"),
        J->Env.Shell,
        TRUE);

      MStringAppendF(&ArgLine," %s",
        tmpPTYStr);

      MUFree(&tmpPTYStr);
      }
    else
      {
      /* $SHELL exists in the PTYString. It will be replaced by the process' $SHELL env 
       * variable in MClient.c just before executing the jobs interactive shell. */

      MStringAppendF(&ArgLine," %s",
        PTYString);
      }
    }

  if (J->Env.Args != NULL)
    {
    MStringAppendF(&ArgLine," %s",
      J->Env.Args);
    }

  if (J->Env.BaseEnv == NULL)
    {
    char tmpEnv[MMAX_LINE];

    /* no env specified */

    /* only set path if env is specified and not passed, otherwise, SLURM will extract
       PATH from the execution user's environment on the compute node */
  
    if (bmisset(&J->IFlags,mjifEnvSpecified))
      {
      if ((MSched.ServerPath != NULL) && (MSched.ServerPath[0] != '\0'))
        {
        snprintf(tmpEnv,sizeof(tmpEnv),"PATH=%s",
          MSched.ServerPath);
        }
      else
        {
        snprintf(tmpEnv,sizeof(tmpEnv),"PATH=/bin/:/usr/bin:/usr/local/bin");
        }

      MUStrDup(&J->Env.BaseEnv,tmpEnv);
      }
    }    /* END if (J->E.BaseEnv == NULL) */

  if (J->Env.UMask != 0)
    {
    char  UMaskLimit[MMAX_LINE];

    sprintf(UMaskLimit,"%sumask=%d",
      (Limits[0] != '\0') ? "," : "",
      J->Env.UMask);

    MUStrCat(Limits,UMaskLimit,sizeof(Limits));
    }

  if (!bmisset(&J->IFlags,mjifEnvSpecified))
    {
    /* get-user-env is set, unset key submission values so SLURM can apply local values */

    /* NYI */
    }

  if (bmisset(&J->Flags,mjfInteractive))
    {
    mstring_t  tmpCmd(MMAX_LINE);

    if (R->ConfigFile[0] != '\0')
      {
      MStringAppendF(&tmpCmd,"SLURM_CONF=%s ",
        R->ConfigFile);
      }

    if (SubmitExe[0] != '/')
      {
      char tmpPath[MMAX_LINE];

      if (MFUGetFullPath(
            SubmitExe,
            MUGetPathList(),
            R->Wiki.SBinDir,
            NULL,
            TRUE,
            tmpPath,
            sizeof(tmpPath)) == FAILURE)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    exec '%s' cannot be located or is not executable (errno: %d '%s')\n",
          SubmitExe,
          errno,
          strerror(errno));

        if (EMsg != NULL)
          {
          sprintf(EMsg,"exec '%s' cannot be located or is not executable",
            SubmitExe);
          }

        return(FAILURE);
        }    /* END if (MFUGetFullPath() == FAILURE) */

      MStringAppend(&tmpCmd,tmpPath);
      }
    else
      {
      MStringAppend(&tmpCmd,SubmitExe);
      }

    /* job is interactive - record submitstring and return */

    MStringAppend(&tmpCmd,ArgLine.c_str());

    MUStrDup(&J->RMSubmitString,tmpCmd.c_str());
  
    MDB(3,fSTRUCT) MLog("INFO:     Interactive submit string=%s\n",tmpCmd.c_str());

    return(SUCCESS);
    }  /* END if (bmisset(&J->Flags,mjfInteractive)) */

  MDB(3,fSTRUCT) MLog("INFO:     ArgLine=%s\n",ArgLine.c_str());

  /* TBD remove this log once disappearing env info bug is fixed RT3304 */ 

  MDB(7,fGRID) MLog("INFO:     job %s final environment cstring buffer is of length %d\n",
    J->Name,
    EnvBuf.capacity());

  rc = MUSpawnChild(
    SubmitExe,
    J->Name,
    ArgLine.c_str(),  /* args */
    NULL,
    -1,     /* use admin UID */
    -1,     /* use admin GID */
    (!bmisset(&R->Flags,mrmfFSIsRemote)) ? SubmitDir : NULL,
    EnvBuf.c_str(),
    (usePipe == TRUE) ? EnvPipe : NULL,
    Limits,
    NULL,   /* input buffer */
    NULL,
    &OBuf,  /* O */
    &EBuf,  /* O */
    NULL,
    &tmpSC, /* O - exit code */
    MSched.MWikiSubmitTimeout * 1000,  /* timeout (in milliseconds) */
    0,
    NULL,
    mxoNONE,
    FALSE,
    bmisset(&R->IFlags,mrmifIgnoreDefaultEnv),
    EMsg);  /* O */

  MFURemove(CmdFile);
  MFURemove(EnvFile);

  if (R->Version >= 20100)
    {
    /* In SLURM 2.1 they made a correction to put errors on standard error and command output on stdout */

    RspOutput = OBuf;
    }
  else
    {
    /* Pre SLURM 2.1 errors were put in stdout and the command output was on stderr */

    RspOutput = EBuf;
    }

  if (RspOutput == NULL)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    no jobid returned by exec '%s' (errno: %d '%s')\n",
      SubmitExe,
      errno,
      strerror(errno));

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      strcpy(EMsg,"cannot get output from submit command");
      }

    MUFree(&EBuf);
    MUFree(&OBuf);

    return(FAILURE);
    }

  /* Pre SLURM 2.1 output */
  /* FORMAT (stderr): srun: jobid <JOBID> submitted\n[srun: Warning: XXX\n] */  
  /* FORMAT: Unable ... */
  /* FORMAT: srun: error: Unable to submit batch job: ...\n */
  /* SLURM 2.1 output */
  /* FORMAT (stdout): jobid <JOBID> submitted\n[Warning: XXX\n] */  
  /* FORMAT: Unable ... */
  /* FORMAT: error: Unable to submit batch job: ...\n */

  /* check for error messages */

  /* NOTE:  'error: X' message may occur even on a successful job submission */

  if (tmpSC != 0) 
    {
    /* submit request failed - non-zero exit code always indicates job submission failed */

    MDB(1,fSTRUCT) MLog("ALERT:    exec '%s' failed - SC: %d  RspOutput: '%s' Err: '%s'\n",
      SubmitExe,
      tmpSC,
      RspOutput,
      EBuf);

    snprintf(tmpLine,sizeof(tmpLine),"cannot migrate job to RM %s - %s",
      R->Name,
      EBuf);

    MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
    MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

    if (EMsg != NULL)
      MUStrCpy(EMsg,EBuf,MMAX_LINE);

    if (bmisset(&J->NotifyBM,mntJobFail))
      {
      MSysSendMail(
        MSysGetAdminMailList(1),
        NULL,   
        "Could not migrate job to Wiki RM",
        NULL,
        tmpLine);
      }

    MUFree(&OBuf);
    MUFree(&EBuf);

    return(FAILURE);
    }

  ptr = MUStrTok(RspOutput,"\n",&TokPtr);

  if (ptr == NULL)
    {
    MDB(1,fSTRUCT) MLog("ALERT:     exec '%s' did not report jobid\n",
      SubmitExe);

    if (EMsg != NULL)
      strcpy(EMsg,"cannot extract jobid - no data returned");

    MUFree(&OBuf);
    MUFree(&EBuf);

    return(FAILURE);
    }

  while (ptr != NULL)
    {
    if (!strncasecmp(ptr,tmpJobSearchLine,strlen(tmpJobSearchLine)))
      {
      /* extract jobid */
   
      if (R->Version >= 20100)
        {
        rc = sscanf(ptr,"%32s %32s %32s %32s",
          tmpLine,
          tmpLine,
          tmpLine,
          JobID);

        if (rc != 4)
          {
          MDB(1,fSTRUCT) MLog("ALERT:    exec '%s' did not report jobid, %d args read from '%s'\n",
            SubmitExe,
            rc,
            ptr);

          if (EMsg != NULL)
            strcpy(EMsg,"cannot extract jobid");

          MUFree(&OBuf);
          MUFree(&EBuf);

          return(FAILURE);
          }
        }
      else if (R->Version >= 10300)
        {
        rc = sscanf(ptr,"%32s %32s %32s %32s %32s",
          tmpLine,
          tmpLine,
          tmpLine,
          tmpLine,
          JobID);
   
        if (rc != 5)
          {
          MDB(1,fSTRUCT) MLog("ALERT:    exec '%s' did not report jobid, %d args read from '%s'\n",
            SubmitExe,
            rc,
            ptr);
     
          if (EMsg != NULL)
            strcpy(EMsg,"cannot extract jobid");
   
          MUFree(&OBuf);
          MUFree(&EBuf);
     
          return(FAILURE);
          }
        }
      else
        {
        rc = sscanf(ptr,"%32s %32s %32s",
          tmpLine,
          tmpLine,
          JobID);
   
        if (rc != 3)
          {
          MDB(1,fSTRUCT) MLog("ALERT:    exec '%s' did not report jobid, %d args read from '%s'\n",
            SubmitExe,
            rc,
            ptr);
     
          if (EMsg != NULL)
            strcpy(EMsg,"cannot extract jobid");
   
          MUFree(&OBuf);
          MUFree(&EBuf);
          
          return(FAILURE);
          }
        }

      /* NOTE:  should generate ALERT if --with-jobid is used and reported 
                jobid is different (NYI) */
      }
    else if (strstr(ptr,"Warning:") != NULL)
      {
      /* check for warning messages */

      if (strstr(ptr,"pending scheduler release") != NULL)
        {
        /* ignore default SLURM external scheduler message */

        /* NO-OP */
        }
      else
        { 
        snprintf(tmpLine,sizeof(tmpLine),"warning received when migrating job to RM %s - %s",
          R->Name,
          ptr);
   
        MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }
      }
    else if (strstr(ptr,"error:") != NULL)
      {
      /* check for error messages */

      snprintf(tmpLine,sizeof(tmpLine),"error received when migrating job to RM %s - %s",
        R->Name,
        ptr);

      MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
      }
    else
      {
      /* Not sure what kind of message this is */

      MDB(7,fSTRUCT) MLog("INFO:     Unrecognized response message to job submit '%s'\n",
        ptr);
      }

    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  MUFree(&OBuf);
  MUFree(&EBuf);
          
  return(SUCCESS);
  }  /* END MWikiJobSubmit() */

/* END MWikiJobSubmit.c */
