/* HEADER */

/**
 * @file MJobSubmitPBS.c
 *
 * Contains  MPBSJobSubmit()
 *
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Submit job to PBS RM using PBS submission language.
 * 
 * NOTE:  at this point, fork/exec qsub, do not use PBS submit API (reconsider)
 *
 * @see MRMJobSubmit() - parent
 *
 * @param SubmitString (I) [optional]
 * @param R (I)
 * @param JP (I/O)
 * @param FlagBM (I) [type=enum MJobSubmitFlagsEnum] stage cmd file, do not submit job to RM
 * @param JobName (O) [minsize=MMAX_NAME]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MPBSJobSubmit(

  const char           *SubmitString,  /* I (optional) */
  mrm_t                *R,             /* I */
  mjob_t              **JP,            /* I/O */
  mbitmap_t            *FlagBM,        /* I (type=enum MJobSubmitFlagsEnum) stage cmd file, do not submit job to RM */
  char                 *JobName,       /* O (minsize=MMAX_NAME) */
  char                 *EMsg,          /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)            /* O (optional) */

  {
  int     rqindex;
  int     RMTasks;  /* total tasks requested for specified RM */

  mnode_t *N;

  char    CommandBuffer[MMAX_SCRIPT];
  char    SpoolDir[MMAX_PATH_LEN];
  char    CmdFile[MMAX_PATH_LEN];

  char    tmpBuffer[MMAX_BUFFER];
  char    tmpLine[MMAX_BUFFER << 1];
  char    tmpFlagLine[MMAX_LINE];

  mcred_t tmpCreds;

  char   *ptr;

  char   *BPtr;
  int     BSpace;

  char   *tBPtr;
  int     tBSpace;

  char    ParBuf[MMAX_LINE];
  char    PartitionReqLine[MMAX_LINE];

  char   *OBuf = NULL;
  char   *EBuf = NULL;

  char    SubmitExe[MMAX_LINE];
  char    SubmitDir[MMAX_LINE];
  char    Cmd[MMAX_LINE];
  char    OFile[MMAX_LINE];
  char    EFile[MMAX_LINE];

  mjob_t *J;
  mreq_t *RQ;

  int     fd;

  mbool_t DataOnly;
  mbool_t SubmitDirIsLocal = TRUE;  /* job submission directory exists on Moab server host */

  const char *FName = "MPBSJobSubmit";
  ParBuf[0] = '\0';

  MDB(1,fPBS) MLog("%s(JobDesc,%s,%s,%s,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    ((JP != NULL) && (*JP != NULL)) ? (*JP)->Name : "NULL",
    (JobName != NULL) ? "JobName" : "NULL");

  if (JobName != NULL)
    JobName[0] = '\0';

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  /* NOTE:  fork, set uid/gid, send stderr back to parent process via pipe */

  /* NOTE:  launch environment info passed via job */

  if ((JP == NULL) || (*JP == NULL))
    {
    MDB(1,fPBS) MLog("ERROR:    invalid job pointer in %s()\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error: bad job pointer");

    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  J  = *JP;

  DataOnly = (((FlagBM != NULL) && (bmisset(FlagBM,mjsufDataOnly))) || bmisset(&J->IFlags,mjifDataOnly));

  RMTasks = 0;

  RQ = NULL;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    if ((&MRM[J->Req[rqindex]->RMIndex] == R) &&
        (J->Req[rqindex]->PtIndex != MSched.SharedPtIndex) &&
        (((MRM[J->Req[rqindex]->RMIndex].SubType == mrmstXT4) &&  /* size 0 job */
          (bmisset(&J->IFlags,mjifDProcsSpecified) == TRUE)) || 
         (J->Req[rqindex]->DRes.Procs != 0)))  /* check to make sure we don't count GLOBAL resources */
      {
      if (RQ == NULL)
        RQ = J->Req[rqindex];

      RMTasks += J->Req[rqindex]->TaskCount;
      }
    }    /* END for (rqindex) */

  if (RMTasks == 0)
    RMTasks = 1;

  if (RQ == NULL)
    {
    /* job is corrupt */

    MDB(1,fPBS) MLog("ERROR:    job is corrupt in %s() - no req\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error: corrupt job");

    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  memcpy(&tmpCreds,&J->Credential,sizeof(tmpCreds));

  /* NOTE:  PBS executes from the home directory unless '-d' is specified */

  MJobGetIWD(J,SubmitDir,sizeof(SubmitDir),TRUE);
  MJobGetCmd(J,Cmd,sizeof(Cmd));
  MJobResolveHome(J,J->Env.Output,OFile,sizeof(OFile));
  MJobResolveHome(J,J->Env.Error,EFile,sizeof(EFile));

  MUSNInit(&BPtr,&BSpace,CommandBuffer,sizeof(CommandBuffer));

  if ((SubmitString == NULL) &&
     ((J->RMSubmitString == NULL) || 
      (J->RMSubmitType != mrmtPBS) ||
     ((J->Req[1] != NULL) && bmisset(&J->Flags,mjfCoAlloc))))
    {
    int GPUIndex;
    mstring_t gpus(MMAX_LINE);

    /* No SubmitString available, use J->E.Cmd and create command file string */

    MUSNPrintF(&BPtr,&BSpace,"%s\n",
      MURMScriptExtractShell(J->RMSubmitString,tmpLine));
  
    MUSNPrintF(&BPtr,&BSpace,"# Moab-PBS staging file\n");

    if (J->AName != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -N %s\n",
        J->AName);
      }

    if ((SubmitDir[0] != '\0') &&
       ((J->Credential.U->HomeDir == NULL) || strcasecmp(J->Credential.U->HomeDir,SubmitDir)))
      {
      /* NOTE:  PBS executes from the home directory unless '-d' is specified */

      if (R->Version < 500)
        {
        /* HACK:  assume TORQUE if version < 500, PBSPro if version >= 500 */

        /* -d option only supported by TORQUE */

        MUSNPrintF(&BPtr,&BSpace,"#PBS -d %s\n",
          SubmitDir);
        }
      }

    if (bmisset(&J->Flags,mjfInteractive))
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -I\n");

      if (!MUStrIsEmpty(J->InteractiveScript))
        {
        MUSNPrintF(&BPtr,&BSpace,"#PBS -x %s\n",J->InteractiveScript);
        }
      }

    if (J->Credential.C != NULL)
      {
      /* NOTE:  currently only allow one class per job */

      MUSNPrintF(&BPtr,&BSpace,"#PBS -q %s\n",
        J->Credential.C->Name);
      }

    if ((J->Credential.A != NULL) && (strcmp(J->Credential.A->Name,NONE)))
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -A %s\n",
        J->Credential.A->Name);
      }

    /* generate gpus string to append to nodes */

    GPUIndex = MUMAGetIndex(meGRes,MCONST_GPUS,mVerify);

    if ((GPUIndex == 0) || (GPUIndex > MMAX_GRES))
      {
      /* gpus exist on the system, see if requested */

      if (MSNLGetIndexCount(&RQ->DRes.GenericRes,GPUIndex) > 0)
        {
        MStringAppendF(&gpus,":gpus=%d",
          MSNLGetIndexCount(&RQ->DRes.GenericRes,GPUIndex));
        }
      }

    MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

    if (bmisset(&J->IFlags,mjifHostList) && (!MNLIsEmpty(&J->ReqHList)))
      {
      /* job requests hostlist */

      MUSNPrintF(&tBPtr,&tBSpace,"%snodes=",
        (tmpLine[0] != '\0') ? "," : "");

      MNLToString(&J->ReqHList,FALSE,"+",'\0',tBPtr,tBSpace);

      tBSpace -= strlen(tBPtr);
      tBPtr   += strlen(tBPtr);

      MUSNPrintF(&tBPtr,&tBSpace,gpus.c_str());
      } 
    else if (RQ->TasksPerNode > 0)
      {
      /* NOTE:  assume common ppn per req for all reqs */
      /* NOTE:  assume 1 proc per task for PBS */

      int ppn;
      int NC;

      ppn = RQ->TasksPerNode;

      NC = J->Request.NC;

      if (NC <= 0)
        {
        NC = (int)(RMTasks/ppn);
        }

      NC = MAX(1,NC);

      MUSNPrintF(&tBPtr,&tBSpace,"%snodes=%d:ppn=%d",
        (tmpLine[0] != '\0') ? "," : "",
        NC,
        ppn);

      MUSNPrintF(&tBPtr,&tBSpace,gpus.c_str());
      }  /* END else if (RQ->TasksPerNode > 0) */
    else
      {
      char Line[MMAX_LINE];

      mstring_t FeaturesList(MMAX_LINE);
      mstring_t ExcludedFeaturesList(MMAX_LINE);

      /* NOTE:  should not report nodes if mjifTasksSpecified is not set,
                and submission queue defaults exist for nodes */

      /* NYI */

      /* NOTE:  assume 1 proc per task for PBS */

      if (RQ->NodeCount > 0)
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%snodes=%d:ppn=%d",
          (tmpLine[0] != '\0') ? "," : "",
          (int)RQ->NodeCount,
          (int)(RMTasks/RQ->NodeCount));
        }
      else
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%snodes=%d",
          (tmpLine[0] != '\0') ? "," : "",
          RMTasks);
        }

      MUSNPrintF(&tBPtr,&tBSpace,gpus.c_str());

      /* Build included feature string */

      ptr = MUNodeFeaturesToString(':',&RQ->ReqFBM,Line);

      if ((ptr != NULL) && (ptr[0] != '\0') && (strcmp(ptr,NONE))) 
        {
        MStringAppend(&FeaturesList,ptr);
        }

      /* Build excluded feature string */

      ptr = MUNodeFeaturesToString(':',&RQ->ExclFBM,Line);

      if ((ptr != NULL) && (ptr[0] != '\0') && (strcmp(ptr,NONE))) 
        {
        MStringAppend(&ExcludedFeaturesList,ptr);
        }
      
      if ((R->Version >= 500) && strcmp(FeaturesList.c_str(),""))
        {
        /* apply features to PBSPro 
         * excluded features NYI */

        MUSNPrintF(&tBPtr,&tBSpace,":%s",
          FeaturesList.c_str());
        }
      else if (strcmp(FeaturesList.c_str(),"") || strcmp(ExcludedFeaturesList.c_str(),""))
        {
        mstring_t TorqueString(MMAX_LINE);
        char* TPtr;
        char* TokPtr;

        /* apply features to TORQUE - use 'feature' in case features only
           specified w/in Moab */

        /* feature string syntax: feature='(FEATURE)?|(((!)?FEATURE)([:\|&](!)?FEATURE))*' 
         * where a feature preceded by ! is an excluded feature and ":" is taken
         * to be the default logic mode for features (AND) or excluded features (OR) */

        if (strcmp(FeaturesList.c_str(),"")) 
          {
          char *mutableString = NULL;
          MUStrDup(&mutableString,FeaturesList.c_str());

          TPtr = MUStrTok(mutableString,":",&TokPtr);

          MStringAppend(&TorqueString,TPtr);

          while ((TPtr = MUStrTok(NULL,":",&TokPtr)))
            {
            MStringAppend(&TorqueString,(RQ->ReqFMode == mclAND ? (char *)"&" : (char *)"|"));
            MStringAppend(&TorqueString,TPtr);
            }

          FeaturesList = mutableString;

          MUFree(&mutableString);
          }
    
        if (strcmp(ExcludedFeaturesList.c_str(),"")) 
          {
          char *mutableString = NULL;
          MUStrDup(&mutableString,ExcludedFeaturesList.c_str());

          TPtr = MUStrTok(mutableString,":",&TokPtr);

          if (strcmp(TorqueString.c_str(),""))
            {
            MStringAppend(&TorqueString,":");
            }

          MStringAppend(&TorqueString,"!");
          MStringAppend(&TorqueString,TPtr);

          while ((TPtr = MUStrTok(NULL,":",&TokPtr)))
            {
            MStringAppend(&TorqueString,(RQ->ExclFMode == mclAND ? (char *)"&!" : (char *)"|!"));
            MStringAppend(&TorqueString,TPtr);
            }

          ExcludedFeaturesList = mutableString;

          MUFree(&mutableString);
          }

        MUSNPrintF(&tBPtr,&tBSpace,"%sfeature='%s'",
          (tmpLine[0] != '\0') ? "," : "",
          TorqueString.c_str());
        }
      }    /* END else (bmisset(&J->IFlags,mjifHostList) && (J->ReqHList != NULL)) */


    /* per-job prologue and epiloge attributes supported in Torque 2.4.1 */
    if (R->Version >= 241 && R->Version < 500)
      {
      if (J->PrologueScript != NULL) 
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%sprologue='%s'",
          ((tmpLine[0] != '\0') ? "," : ""),
          J->PrologueScript);
        }

      if (J->EpilogueScript != NULL) 
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%sepilogue='%s'",
          ((tmpLine[0] != '\0') ? "," : ""),
          J->EpilogueScript);
        }
      }

    if (bmisset(&J->IFlags,mjifWCNotSpecified))
      {
      mjob_t *CJDef = NULL;

      mulong tmpL = 0;

      if (J->Credential.C != NULL)
        CJDef = J->Credential.C->L.JobDefaults;

      if ((CJDef != NULL) && (CJDef->SpecWCLimit[0] > 0))
        tmpL = CJDef->SpecWCLimit[0];
      else if (R->DefWCLimit > 0)
        tmpL = R->DefWCLimit;

      if (tmpL > 0)
        MJobSetAttr(J,mjaReqAWDuration,(void **)&tmpL,mdfLong,mSet);
      }
    
    if (J->WCLimit >= MCONST_DAYLEN)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%swalltime=%ld",
        (tmpLine[0] != '\0') ? "," : "",
        J->WCLimit);
      }
    else if (J->WCLimit > 0)
      {
      char TString[MMAX_LINE];

      MULToTString(J->WCLimit,TString);

      MUSNPrintF(&tBPtr,&tBSpace,"%swalltime=%s",
        (tmpLine[0] != '\0') ? "," : "",
        TString);
      }

    /* file */

    if (RQ->DRes.Disk > 0)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sfile=%dmb",
        (tmpLine[0] != '\0') ? "," : "",
        RQ->DRes.Disk);
      }

    /* mem */

    if (RQ->DRes.Mem > 0)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%smem=%dmb",
        (tmpLine[0] != '\0') ? "," : "",
        RQ->DRes.Mem);
      }

    /* vmem */

    if (RQ->DRes.Swap > 0)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%svmem=%dmb",
        (tmpLine[0] != '\0') ? "," : "",
        RQ->DRes.Swap);
      }

    /* cput */

    if (J->CPULimit >= MCONST_DAYLEN)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%scput=%ld",
        (tmpLine[0] != '\0') ? "," : "",
        J->CPULimit);
      }
    else if (J->CPULimit > 0)
      {
      char TString[MMAX_LINE];

      MULToTString(J->CPULimit,TString);

      MUSNPrintF(&tBPtr,&tBSpace,"%scput=%s",
        (tmpLine[0] != '\0') ? "," : "",
        TString);
      }

    /* arch */

    if ((RQ->Arch != 0) && strcmp(MAList[meArch][RQ->Arch],NONE))
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sarch=%s",
        (tmpLine[0] != '\0') ? "," : "",
        MAList[meArch][RQ->Arch]);
      }

    if ((RQ->Opsys != 0) && strcmp(MAList[meOpsys][RQ->Arch],NONE))
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sopsys=%s",
        (tmpLine[0] != '\0') ? "," : "",
        MAList[meOpsys][RQ->Opsys]);
      }

    if (tmpLine[0] != '\0')
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -l %s\n",
        tmpLine);
      }

    if (J->AName != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -N %s\n",
        J->AName);
      }

    if (J->SpecNotification != NULL)
      {
      char tmpLine[MMAX_LINE];

      /* FORMAT: all,JobStart,JobEnd,JobFail */

      if (J->RMSubmitType == mrmtPBS)
        {
        MUStrCpy(tmpLine,J->SpecNotification,sizeof(tmpLine));
        }
      else
        {
        tmpLine[0] = '\0';

        if (bmisset(&J->NotifyBM,mntAll))
          {
          strcat(tmpLine,"abe");
          }
        else
          {
          if (bmisset(&J->NotifyBM,mntJobStart))
            strcat(tmpLine,"b");

          if (bmisset(&J->NotifyBM,mntJobEnd))
            strcat(tmpLine,"e");

          if (bmisset(&J->NotifyBM,mntJobFail)) 
            strcat(tmpLine,"a");
          }
        }    /* END else (J->SubmitLanguage == mrmtPBS) */

      if (tmpLine[0] != '\0')
        {
        MUSNPrintF(&BPtr,&BSpace,"#PBS -m %s\n",
          tmpLine);
        }
      }

    if (J->NotifyAddress != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -M %s\n",
        J->NotifyAddress);
      }

    /* disable checkpointing */

    MUSNCat(&BPtr,&BSpace,"#PBS -c n\n");

    if (OFile[0] != '\0')
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -o %s\n",
        OFile);
      }

    if (EFile[0] != '\0')
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -e %s\n",
        EFile);
      }
    else if (OFile[0] != '\0')
      {
      /* join stderr with stdout */

      MUSNCat(&BPtr,&BSpace,"#PBS -joe\n");
      }

    if (J->UPriority != 0)
      {
      /* NOTE:  PBS user priorities may be in the range -1024 to +1024 */
      /* NOTE:  Moab constrains user priorities -infinity to 0 */

      MUSNPrintF(&BPtr,&BSpace,"#PBS -p %ld\n",
        MAX(-1024,J->UPriority));
      }

    if (J->SMinTime > 0)
      {
      char tmpRMString[MMAX_LINE];

      MTimeToRMString(J->SMinTime,mrmtPBS,tmpRMString);

      MUSNPrintF(&BPtr,&BSpace,"#PBS -a %s\n",
        tmpRMString);
      }

    if (J->Env.Shell != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -S %s\n",
        J->Env.Shell);
      }

    if ((J->Env.BaseEnv != NULL) && bmisset(&J->IFlags,mjifEnvSpecified))
      {
      char *tmpBuf = NULL;

      /* Duplicate job environment, strip out 0x36 chars, and replace with comma's. */

      MUStrDup(&tmpBuf,J->Env.BaseEnv);
      MUStrReplaceChar(tmpBuf,ENVRS_ENCODED_CHAR,',',TRUE);
      MUStrRemoveFromEnd(tmpBuf,",");

      /* Build PBS -v command. */

      MUSNPrintF(&BPtr,&BSpace,"#PBS -v %s\n",tmpBuf);

      MUFree(&tmpBuf);
      }

    /* apply restartable & service attributes in torque */

    if (R->Version < 500)
      {
      MUSNPrintF(&BPtr,&BSpace,"#PBS -r %c\n",
        (bmisset(&J->Flags,mjfRestartable) ? 'y' : 'n')); 

      if (bmisset(&J->IFlags,mjifIsServiceJob))
        {
        MUSNPrintF(&BPtr,&BSpace,"#PBS -s y\n");
        }
      }

    /* add rm extensions */

    MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

    /* system ID */

    if (J->SystemID != NULL)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sSID%c%s",
        (tmpLine[0] != '\0') ? ";" : "",
        (R->Version > 500) ? '=' : ':',
        J->SystemID);
      }

    /* system JID */

    if (J->SystemJID != NULL)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sSJID%c%s",
        (tmpLine[0] != '\0') ? ";" : "",
        (R->Version > 500) ? '=' : ':',
        J->SystemJID);
      }
 
    if (J->SRMJID != NULL)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sSRMJID%c%s",
        (tmpLine[0] != '\0') ? ";" : "",
        (R->Version > 500) ? '=' : ':',
        J->SRMJID);
      }

    /* reservation constraints */

    if (bmisset(&J->Flags,mjfAdvRsv))
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sADVRES%c%s",
        (tmpLine[0] != '\0') ? ";" : "",
        (R->Version > 500) ? '=' : ':',
        (J->ReqRID != NULL) ? J->ReqRID : "ALL");
      }

    /* QOS request */

    if (J->QOSRequested != NULL)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sQOS%c%s",
        (tmpLine[0] != '\0') ? ";" : "",
        (R->Version > 500) ? '=' : ':',
        J->QOSRequested->Name);
      }

    /* trigger request */

    if (J->Triggers != NULL)
      {
      mtrig_t *T;

      int tindex;

      for (tindex = 0;tindex < J->Triggers->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,tindex);

        if (MTrigIsReal(T) == FALSE)
          continue;

        MTrigToAVString(T,'&',tmpBuffer,sizeof(tmpBuffer));

        MUSNPrintF(&tBPtr,&tBSpace,"%strig%c%s",
          (tmpLine[0] != '\0') ? ";" : "",
          (R->Version > 500) ? '=' : ':',
          tmpBuffer);
        }
      }  /* END if (J->T != NULL) */

    /* NOTE:  no flag overflow checking */

    tmpFlagLine[0] = '\0';

    if (bmisset(&J->SpecFlags,mjfRestartable) && (R->Version >= 500))
      {
      /* no direct support in PBS, use rm extensions */

      if (tmpFlagLine[0] != '\0')
        strcat(tmpFlagLine,",");

      strcat(tmpFlagLine,MJobFlags[mjfRestartable]);
      }

    if (bmisset(&J->SpecFlags,mjfPreemptee))
      {
      /* no direct support in PBS, use rm extensions */

      if (tmpFlagLine[0] != '\0')
        strcat(tmpFlagLine,",");

      strcat(tmpFlagLine,MJobFlags[mjfPreemptee]);
      }

    if (tmpFlagLine[0] != '\0')
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sFLAGS%c%s",
        (tmpLine[0] != '\0') ? ";" : "",
        (R->Version > 500) ? '=' : ':',
        tmpFlagLine);
      }

    if (bmisset(&J->IFlags,mjifEnvRequested) == TRUE) /* if -E was specified */
      {
      /* if -E was specified, put EnvRequested rm xattr on job to get info back. */

      MUSNPrintF(&tBPtr,&tBSpace,"%s%s%c%s",
        (tmpLine[0] != '\0') ? ";" : "",
        MRMXAttr[mxaEnvRequested],
        (R->Version > 500) ? '=' : ':',
        MBool[TRUE]);
      }

    if (tmpLine[0] != '\0')
      {
      /* NOTE:  TORQUE 'may' require '-W x=<ARGS>' below, but tmpLine[] 
                created locally w/o 'x=' prefix */

      /* NOTE: PBSPro uses resources defined in 'server_priv/resourcedef' */

      MUSNPrintF(&BPtr,&BSpace,"#PBS %s%s\n",
        (R->Version >= 500) ? (char *)"-l " : (char *)"-W x=",
        tmpLine);
      }

    /* end of PBS args */

    if (J->RMSubmitString == NULL)
      {
      char ExecutableLine[MMAX_LINE];

      MUSNInit(&tBPtr,&tBSpace,ExecutableLine,sizeof(ExecutableLine));

      if (J->Env.Input != NULL)
        {
        MUSNPrintF(&tBPtr,&tBSpace,"cat %s | ",
          J->Env.Input);
        }

      if (J->Env.Cmd == NULL)
        {
        /* cmd file is mandatory */

        /* write submit string to spool file */

        if (EMsg != NULL)
          strcpy(EMsg,"internal error: no cmd");

        if (SC != NULL)
          *SC = mscBadParam;

        return(FAILURE);
        }

      if (Cmd[0] != '\0')
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%s%s",
          (ExecutableLine[0] != '\0') ? " " : "",
          Cmd);
        }
      else if (J->RMSubmitString != NULL)
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%s%s",
          (ExecutableLine[0] != '\0') ? " " : "",
          J->RMSubmitString);
        }
      else 
        {
        mbool_t IsExe;

        if (MFUGetInfo(J->Env.Cmd,NULL,NULL,&IsExe,NULL) == FAILURE)
          {
          if (EMsg != NULL)
            snprintf(EMsg,MMAX_LINE,"internal error: cmd '%s' does not exist",
              J->Env.Cmd);

          if (SC != NULL)
            *SC = mscBadParam;

          return(FAILURE);
          }

        /* NOTE: PBS cmd files do not need to be executable - remove MNOT below after testing (5/29/07) */       

#ifdef MNOT 
        if (IsExe == FALSE)
          {
          if (EMsg != NULL)
            snprintf(EMsg,MMAX_LINE,"internal error: cmd '%s' is not executable",
              J->Env.Cmd);

          if (SC != NULL)
            *SC = mscBadParam;

          return(FAILURE);
          }
#endif /* MNOT */
 
        MUSNPrintF(&tBPtr,&tBSpace,"%s%s",
          (ExecutableLine[0] != '\0') ? " " : "",
          J->Env.Cmd);
        } 

      if (J->Env.Args != NULL)
        {
        MUSNPrintF(&tBPtr,&tBSpace," %s",
          J->Env.Args);
        }

      if (ExecutableLine[0] != '\0')
        {
        MUSNPrintF(&BPtr,&BSpace,"%s\n",
          ExecutableLine);
        }
      }
    else
      {
      char tScript[MMAX_BUFFER];

      MUSNPrintF(&BPtr,&BSpace,"\n%s",
        MURMScriptFilter(J->RMSubmitString,J->RMSubmitType,tScript,sizeof(tScript)));
      }  /* END if (J->RMSubmitString == NULL) */

    MDB(3,fRM) MLog("INFO:     submit job command buffer created: '%s'\n",
      CommandBuffer);
    }    /* END if ((SubmitString == NULL) && ...) */
  else
    {
    char tmpVal[MMAX_BUFFER];

    if (SubmitString != NULL)
      MUStrCpy(CommandBuffer,SubmitString,sizeof(CommandBuffer));
    else 
      MUStrCpy(CommandBuffer,J->RMSubmitString,sizeof(CommandBuffer));

    /* apply not-restartable attribute in torque */

    if ((R->Version < 500) && (!bmisset(&J->Flags,mjfRestartable)))
      {
      MJobPBSInsertArg(
        "-r",
        "n",
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }
    else
      {
      MJobPBSInsertArg(
        "-r",
        "y",
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }

    /* apply service attribute in torque */

    if ((R->Version < 500) && (bmisset(&J->IFlags,mjifIsServiceJob)))
      {
      MJobPBSInsertArg(
        "-s",
        "y",
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }

    if ((J->Env.BaseEnv != NULL) && (bmisset(&J->IFlags,mjifEnvSpecified)))
      {
      /* add -V, even if one is already present */

      MJobPBSInsertArg(
        "-V",
        NULL,
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }

    /* if queue locally specified, roll into command buffer */

    if (J->Credential.C != NULL)
      {
      MJobPBSExtractArg(
        "-q",
        NULL,
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        FALSE);

      if (tmpVal[0] == '\0')
        {
        MJobPBSInsertArg(
          "-q",
          J->Credential.C->Name,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }
      else if ((MUStrNCmpCI(J->Credential.C->Name,tmpVal,-1)) == FAILURE)
        {
        /* if J->Cred.C->Name is different than what we pulled from
         * the CommandBuffer, then J->Cred.C->Name gets preference (tmpVal
         * could have been a bogus #PBS directive in the script) RT 4016 */
   
        /* NOTE:  we'll only do this if the MIGRATEALLJOBATTRIBUTES flag is set
                  on the destination RM */
   

        /* get rid of the bogus request */

        MJobPBSExtractArg(
            "-q",
            NULL,
            CommandBuffer,
            tmpVal,
            sizeof(tmpVal),
            TRUE);

        /* insert the correct class name from the job */

        MJobPBSInsertArg(
            "-q",
            J->Credential.C->Name,
            CommandBuffer,
            CommandBuffer,
            sizeof(CommandBuffer));
        }
      }    /* END if (J->Cred.C != NULL) */

    MJobPBSExtractArg(
      "-d",
      NULL,
      CommandBuffer,
      tmpVal,
      sizeof(tmpVal),
      FALSE);

    /* wckey shouldn't be passed to torque with a -l, only 
     * with -W x= so that it will be ignored by torque */

    MJobPBSExtractArg(
      "-l",
      "wckey",
      CommandBuffer,
      NULL,
      0,
      TRUE);

    if ((J->TSets != NULL) && ((R->Version > 500) || (R->Version < 220)))
      {
      /* NOTE:  TORQUE 2.1 and earlier do not support job template specification */

      MJobPBSExtractArg(
        "-l",
        "template",
        CommandBuffer,
        NULL,
        0,
        TRUE);
      }

    if ((J->Req[0]->ReqNR[mrMem] != 0) && ((R->Version > 500) || (R->Version < 240)))
      {
      /* NOTE:  TORQUE 2.3 and earlier do not support job template specification */

      MJobPBSExtractArg(
        "-l",
        "nodemem",
        CommandBuffer,
        NULL,
        0,
        TRUE);
      }

    MJobPBSExtractArg(
      "-l",
      "cgres",
      CommandBuffer,
      NULL,
      0,
      TRUE);

    /* insert SID, SJID, SRMJID for grid workload */
    /* NOTE: not enabled for heirarchical yet */

    if ((J->Grid != NULL) &&
        (J->Grid->SID[0] != NULL) &&
        (J->Grid->SJID[0] != NULL) &&
        (J->SRMJID != NULL))
      {
      char tmpLine[MMAX_LINE];

      snprintf(tmpLine,sizeof(tmpLine),"x=SID:%s;SJID:%s;SRMJID:%s",
        J->Grid->SID[0],
        J->Grid->SJID[0],
        J->SRMJID);

      /* PBSPro uses resources defined in 'server_priv/resourcedef' */

      MJobPBSInsertArg(
        (R->Version >= 500) ? (char *)"-l" : (char *)"-W",
        tmpLine,
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }

    if (!bmisset(&J->IFlags,mjifWCNotSpecified))
      {
      /* insert walltime */

      MJobPBSExtractArg(
        "-l",
        "walltime",
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        FALSE);
    
      if (tmpVal[0] == '\0')
        {
        sprintf(tmpVal,"walltime=%ld",
          J->SpecWCLimit[0]);

        MJobPBSInsertArg(
          "-l",
          tmpVal,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }
      }

    /* if I'm an array job replace -o and -e args with E.Output and E.Error */
    if (bmisset(&J->Flags,mjfArrayJob))
      {
      if ((J->Env.Output != NULL) && !MUStrIsEmpty(J->Env.Output))
        {
        MJobPBSExtractArg(
          "-o",
          NULL,
          CommandBuffer,
          NULL,
          0,
          TRUE);

        MJobPBSInsertArg(
          "-o",
          J->Env.Output,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }

      if ((J->Env.Error != NULL) && !MUStrIsEmpty(J->Env.Error))
        {
        MJobPBSExtractArg(
          "-e",
          NULL,
          CommandBuffer,
          NULL,
          0,
          TRUE);

        MJobPBSInsertArg(
          "-e",
          J->Env.Error,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }
      }

    /* NoLocalUserEnv deprecated 6.0 in favor of FSISREMOTE flag. */

    if ((MSched.NoLocalUserEnv == TRUE) ||
        (bmisset(&R->Flags,mrmfFSIsRemote)))
      {
      SubmitDirIsLocal = FALSE;
      }
    else
      {
      int rc;

      mbool_t IsDir;

      rc = MFUGetInfo(SubmitDir,NULL,NULL,NULL,&IsDir);

      if ((rc == FAILURE) || (IsDir == FALSE))
        SubmitDirIsLocal = FALSE;
      else
        SubmitDirIsLocal = TRUE;
      }
 
    if (SubmitDirIsLocal == FALSE)  
      {
      /* NOTE:  only set working dir if not already set */

      char tmpLine[MMAX_LINE];

      MJobPBSExtractArg(
        "-d",
        NULL,
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        FALSE);

      if (tmpVal[0] == '\0')
        {
        MJobPBSInsertArg(
          "-d",
          SubmitDir,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }

      if (R->Version >= 241 && R->Version < 500)
        {
        /* HACK:  assume TORQUE if version < 500, PBSPro if version >= 500 */

        MJobPBSExtractArg(
          "-w",
          NULL,
          CommandBuffer,
          tmpVal,
          sizeof(tmpVal),
          FALSE);
  
        if (tmpVal[0] == '\0')
          {
          MJobPBSInsertArg(
            "-w",
            SubmitDir,
            CommandBuffer,
            CommandBuffer,
            sizeof(CommandBuffer));
          }
        }

      /* -o needs to have a host in all cases. 
       * in the case that there is no host prepend host
       * case that there is a host and dir insert original back into CommandBuffer 
       * case that hostname: only - append submitdir
       * cast that no -o specified build -o as submithost:submitdir */

      MJobPBSExtractArg(
        "-o",
        NULL,
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        TRUE);

      if (tmpVal[0] != '\0') /* -o was set */
        {
        if (strstr(tmpVal,":") == NULL) 
          {
          /* no hostname: prepend submit host 
           * In the format hostname:patch_name, if the path_name is not 
           * absolute, then qsub will *not* expand the path relative to the
           * current working directory but to the user's home directory. 
           * If the path is not absolute build hostname:submitdir/tmpval */
          
          if (tmpVal[0] == '/')
            {
            sprintf(tmpLine,"%s:%s",
                J->SubmitHost,
                tmpVal);
            }
          else
            {
            sprintf(tmpLine,"%s:%s/%s",
                J->SubmitHost,
                SubmitDir,
                tmpVal);
            }
          }
        else if (tmpVal[strlen(tmpVal) - 1] == ':')
          {
          /* hostname: only - append submitdir */

          sprintf(tmpLine,"%s%s/",
              tmpVal,
              SubmitDir);
          }
        else
          {
          /* use original */

          sprintf(tmpLine,"%s",tmpVal);
          }
        }
      else
        {
        /* build hostname:submitdir */

        sprintf(tmpLine,"%s:%s/",
            J->SubmitHost,
            SubmitDir);
        }

      MJobPBSInsertArg(
        "-o",
        tmpLine,
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));

      /* -e needs to have a host in all cases. 
       * in the case that there is no host prepend host
       * case that there is a host and dir insert original back into CommandBuffer 
       * case that hostname: only - append submitdir
       * cast that no -e specified build -e as submithost:submitdir */

      MJobPBSExtractArg(
        "-e",
        NULL,
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        TRUE);

      if (tmpVal[0] != '\0') /* was in there */
        {
        if (strstr(tmpVal,":") == NULL) 
          {
          /* no hostname: prepend submit host 
           * In the format hostname:patch_name, if the path_name is not 
           * absolute, then qsub will *not* expand the path relative to the
           * current working directory but to the user's home directory. 
           * If the path is not absolute build hostname:submitdir/tmpval */
          
          if (tmpVal[0] == '/')
            {
            sprintf(tmpLine,"%s:%s",
                J->SubmitHost,
                tmpVal);
            }
          else
            {
            sprintf(tmpLine,"%s:%s/%s",
                J->SubmitHost,
                SubmitDir,
                tmpVal);
            }
          }
        else if (tmpVal[strlen(tmpVal) - 1] == ':')
          {
          /* hostname: only - append submitdir */

          sprintf(tmpLine,"%s%s/",
              tmpVal,
              SubmitDir);
          }
        else
          {
          /* use original */

          sprintf(tmpLine,"%s",tmpVal);
          }
        }
      else
        {
        /* build hostname:submitdir */

        sprintf(tmpLine,"%s:%s/",
            J->SubmitHost,
            SubmitDir);
        }

      MJobPBSInsertArg(
        "-e",
        tmpLine,
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }    /* END if (SubmitDirIsLocal == FALSE) */
     
    /* insert nodes (hostlist or proccount) */
    /* NOTE: the specification of -l nodes overrides -l ncpus in all cases */
    /* must also handle case where user puts in both nodes and ncpus */

    MJobPBSExtractArg(
      "-l",
      "nodes",
      CommandBuffer,
      tmpVal,  /* O */
      sizeof(tmpVal),
      FALSE);

    /* If there is no -l nodes= and mjifProcsSpecified is set then -l procs
     * was specified on the commandline and -l nodes is not needed as it will
     * cause issues with interactive jobs (ex. -l procs=5,nodes=5). It's 
     * possible to do -l procs=12,nodes=2 but is the same as nodes=2:ppn=6. */

    if ((tmpVal[0] == '\0') &&
        (bmisset(&J->Flags,mjfProcsSpecified) == FALSE))
      {
      /* no -l nodes is present, check for -l ncpus */

      MJobPBSExtractArg(
        "-l",
        "ncpus",
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        FALSE);

      if (tmpVal[0] == '\0')
        {
        /* no nodes or ncpus, check for -l size */

        MJobPBSExtractArg(
          "-l",
          "size",
          CommandBuffer,
          tmpVal,
          sizeof(tmpVal),
          FALSE);

        if (tmpVal[0] == '\0')
          {
          /* this is a fun game of nested ifs */

          MJobPBSExtractArg(
            "-l",
            "procs",
            CommandBuffer,
            tmpVal,
            sizeof(tmpVal),
            FALSE);
 
          if (tmpVal[0] == '\0')
            {
            /* if none of -l nodes, -l ncpus, and -l size is present, insert -l nodes */

            if (!MNLIsEmpty(&J->ReqHList))
              {
              int nindex;
              char tmpList[MMAX_BUFFER];
              char Name[MMAX_NAME];

              tmpList[0] = '\0';

              for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N) == SUCCESS;nindex++)
                {
                MOMap(R->OMap,mxoNode,N->Name,FALSE,FALSE,Name);

                MUStrAppendStatic(
                  tmpList,
                  Name,
                  '+',
                  sizeof(tmpList));
                }

              snprintf(tmpVal,sizeof(tmpVal),"nodes=%s",
                tmpList);
              }
            else if (!MNLIsEmpty(&J->NodeList))
              {
              mnl_t NL;

              MNLInit(&NL);

              MNLCopy(&NL,&J->NodeList);
 
              MNLRMFilter(&NL,R);

              sprintf(tmpVal,"nodes=%d",
                MNLCount(&NL));

              MNLFree(&NL);
              }
            else
              {
              sprintf(tmpVal,"nodes=%d",
                J->Request.TC);
              }
  
            MJobPBSInsertArg(
              "-l",
              tmpVal,
              CommandBuffer,
              CommandBuffer,
              sizeof(CommandBuffer));
            } /* END if (tmpVal[0] == '\0') */
          }   /* END if (tmpVal[0] == '\0') */
        }     /* END if (tmpVal[0] == '\0') */
      }       /* END if (tmpVal[0] == '\0') */
    else if ((R->OMap != NULL) && (!MNLIsEmpty(&J->ReqHList)))
      {
      mstring_t tmpList(MMAX_LINE);

      int nindex;
      char Name[MMAX_NAME];

      tmpList += "nodes=";

      for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N) == SUCCESS;nindex++)
        {
        MOMap(R->OMap,mxoNode,N->Name,FALSE,FALSE,Name);

        if (nindex > 0)
          tmpList += "+";

        tmpList += Name;
        }

      MJobPBSExtractArg(
        "-l",
        "nodes",
        CommandBuffer,
        tmpVal,  /* O */
        sizeof(tmpVal),
        TRUE);

      MJobPBSInsertArg(
        "-l",
        tmpList.c_str(),
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }  /* END else if ((R->OMap != NULL) && (!MNLIsEmpty(&J->ReqHList))) */

    /* Process the email notify address. */

    if (J->NotifyAddress != NULL)
      {
      MJobPBSExtractArg(
        "-M",
        NULL,
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        FALSE);
      
      if (tmpVal[0] == '\0')
        {
        MJobPBSInsertArg(
          "-M",
          J->NotifyAddress,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }
      }    /* END if (J->NotifyAddress[0] != NULL) */

    /* get the account from the job and insert to the command buffer */
    if ((J->Credential.A != NULL) &&
        (J->Credential.A->Name != NULL) && 
        (J->Credential.A->Name[0] != '\0'))
      {
      /* NOTE:  this is a no-op if the job script doesn't have a -A
                request, but is necessary to make sure that a -A on the
                command-line trumps any -A in the job script. */

      MJobPBSExtractArg(
        "-A",
        NULL,
        CommandBuffer,
        tmpVal,
        sizeof(CommandBuffer),
        TRUE);

      /* (re)insert the "real" account request */

      MJobPBSInsertArg(
        "-A",
        J->Credential.A->Name,
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }
    
    /* check for migrateAllJobAttributes flag on destination RM */
    if (bmisset(&R->Flags,mrmfMigrateAllJobAttrbutes))
      {
      char    PALList[MMAX_LINE];
      char    JobPALString[MMAX_LINE];

      char    FeatureList[MMAX_LINE];
      char    FeatureReq[MMAX_LINE];

      /* get J->SpecPAL and insert the string version to the command buffer */

      MPALToString(&J->SpecPAL,NULL,PALList);
      
      if (!MUStrIsEmpty(PALList) && strcmp(PALList,NONE))
        {
        sprintf(JobPALString, "partition=%s", PALList);

        MJobPBSInsertArg(
          "-l",
          JobPALString,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }

      /* Get the features from J->Req[0] and insert them into the command
       * buffer. 
       *
       * These should have been set to contain the class default features in
       * MJobApplyClassDefaults.
       *
       * NOTE: If the job was submitted with a feature request attached to the
       * resource list for Req[0] these features will now be applied to the
       * job globally!! */
      
      if (!bmisclear(&J->Req[0]->ReqFBM))
        {
        MUNodeFeaturesToString(':',&J->Req[0]->ReqFBM,FeatureList);

        if ((!MUStrIsEmpty(FeatureList)) && (strstr(FeatureList,"[NONE]") == NULL))
          {
          snprintf(FeatureReq,sizeof(FeatureReq),"feature=%s",FeatureList);

          MJobPBSInsertArg(
            "-l",
            FeatureReq,
            CommandBuffer,
            CommandBuffer,
            sizeof(CommandBuffer));
          } /* End if(FeatureList[0] != '\0' */
        } /* END if(!bmisclear(&J->Req[0]->ReqFBM)) */

      } /* END if (bmisset(&R->Flags, mrmfMigrateAllJobAttrbutes) */

    }  /* END else */

  /* stage in executable for template workflow job */

  if (bmisset(&J->IFlags,mjifStageInExec) && (J->Env.Cmd != NULL))
    {
    char tmpVal[MMAX_BUFFER];

    snprintf(tmpVal,sizeof(tmpVal),"stagein=%s@%s:%s\n",
      J->Env.Cmd,
      MSched.ServerHost,
      J->Env.Cmd);

    MJobPBSInsertArg(
      (R->Version >= 500) ? (char *)"-l" : (char *)"-W",
      tmpVal,
      CommandBuffer,
      CommandBuffer,
      sizeof(CommandBuffer));
    }

  /* check for duplicate partition requests and only send the last one */

  MJobRemovePBSDirective(
      NULL,
      "partition",
      CommandBuffer,
      ParBuf,
      MMAX_LINE);

  if (ParBuf[0] != '\0')
    {
    snprintf(PartitionReqLine,MMAX_LINE -1,"partition=%s",ParBuf);

    MJobPBSInsertArg(
      "-l",
      PartitionReqLine,
      CommandBuffer,
      CommandBuffer,
      sizeof(CommandBuffer));
    }

  /* In PBSPro 9 (and earlier?) Resource_List.partition can't be set, because
   * read-only permissions 8.
   * The Submission Script has -l partition=pname and -W x=partition:pname.
   * PBSPro will fail both because it doesn't like -l partition= and -W x= */

  if (R->Version >= 900)
    {
    char tmpVal[MMAX_BUFFER];

    /* "features type=string" should be added to server_priv/resourcedef for features */

    MJobPBSExtractArg(
      "-l",
      "partition=",
      CommandBuffer,
      tmpVal,
      sizeof(tmpVal),
      TRUE);

    MJobPBSExtractArg(
      "-W",
      "x=",
      CommandBuffer,
      tmpVal,
      sizeof(tmpVal),
      TRUE);
      
    if (tmpVal[0] != '\0')
      {
      char tmpExt[MMAX_BUFFER];

      /* replace -W x= with -l x= */

      sprintf(tmpExt,"x=%s",tmpVal);

      MJobPBSExtractArg(
        "-l",
        "x=",
        CommandBuffer,
        tmpVal,
        sizeof(tmpVal),
        TRUE);
       
      if (tmpVal[0] != '\0')
        {
        MUStrCat(tmpExt,";",sizeof(tmpExt));
        MUStrCat(tmpExt,tmpVal,sizeof(tmpExt));
        }

      MJobPBSInsertArg(
        "-l",
        tmpExt,
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
      }
    } /* END if (R->Version >= 900) */

  /* Route job to specific pbs_server and port */

  if ((R->P[0].HostName != NULL) ||
      (R->P[0].Port != 0))
    {
    char *BPtr;
    int   BSpace;
    char  destQueue[MMAX_LINE];
    char  prevQueue[MMAX_LINE];

    destQueue[0] = '\0';
    prevQueue[0] = '\0';
    
    /* server spec format:  @<HOST>[:<PORT>] */

    MUSNInit(&BPtr,&BSpace,destQueue,sizeof(destQueue));

    /* If a queue was specified it should already be added to the CommandBuffer
     * by now. If it was added, the existing information will be preserved and 
     * the destination hostname and port will be appended. */

    MJobPBSExtractArg(
        "-q",
        NULL,
        CommandBuffer,
        prevQueue,
        sizeof(prevQueue),
        TRUE);

    if (R->P[0].HostName != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s@%s",
        (prevQueue[0] == '\0') ? "" : prevQueue,
        R->P[0].HostName);
      }

    /* If the RM is a moab peer then the port number is the port that the peer
     * moab is listening on -- NOT the pbs_server's port. Curreently in a grid
     * setup with interactive jobs the pbs_server must reside on the same host
     * as moab. */

    if ((R->Type != mrmtMoab) && (R->P[0].Port != 0))
      { 
      if (destQueue[0] == '\0')
        {
        MUSNPrintF(&BPtr,&BSpace,"%s@%s:%d",
          (prevQueue[0] == '\0') ? "" : prevQueue,
          MSched.ServerHost,
          R->P[0].Port);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,":%d",
          R->P[0].Port);
        }
      }

    /* Add new destination queue. */

    MJobPBSInsertArg(
        "-q",
        destQueue,
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));
    }    /* END BLOCK */

  /* create submission command */

  if (bmisset(&J->Flags,mjfInteractive))
    {
    char tmpBuffer[MMAX_BUFFER];

    /* NOTE:  should use MFUGetFullPath() and short name for qsub (NYI) */

    MUStrCpy(
      SubmitExe,
      (R->SubmitCmd != NULL) ? R->SubmitCmd : (char *)MDEF_PBSSUBMITEXE,
      sizeof(SubmitExe));

    /* convert to cmdline args, save in J->SubmitString, and return */

    if (__MPBSCmdFileToArgs(
          J,
          R,
          SubmitExe,
          CommandBuffer,
          tmpBuffer,
          sizeof(tmpBuffer),
          EMsg) == FAILURE)
      {
      return(FAILURE);
      }

    MUStrDup(&J->RMSubmitString,tmpBuffer);

    MDB(8,fPBS) MLog("INFO:     interactive cmdline: '%s'\n",tmpBuffer);

    return(SUCCESS);
    }  /* END if (bmisset(&J->Flags,mjfInteractive)) */

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

    if (SC != NULL)
      *SC = mscNoFile;

    return(FAILURE);
    }

  close(fd);

  MUStrDup(&J->Env.Cmd,CmdFile);

  if (DataOnly == TRUE)
    {
    /* write file to spool directory */

    if (MFUCreate(
         CmdFile,
         NULL,
         CommandBuffer,
         strlen(CommandBuffer),
         S_IXUSR|S_IRUSR,
         -1,
         -1,
         FALSE,
         NULL) == FAILURE)
      {
      MFURemove(CmdFile);

      if (SC != NULL)
        *SC = mscNoFile;

      return(FAILURE);
      }

    return(SUCCESS);
    }  /* END if (DataOnly == TRUE) */

  /* NOTE:  for non-data only, should remove CmdFile on failures or 
            once job submission is complete */

  /* NOTE:  should use MFUGetFullPath() with shortname for qsub (NYI) */

  MUStrCpy(
    SubmitExe,
    (R->SubmitCmd != NULL) ? R->SubmitCmd : (char *)MDEF_PBSSUBMITEXE,
    sizeof(SubmitExe));

  MJobPBSExtractArg(
    "-l",
    "nodesetcount",
    CommandBuffer,
    NULL,
    0,
    TRUE);

  if (bmisset(&MPar[0].JobNodeMatchBM,mnmpExactNodeMatch))
    {
    char  tmpVal[MMAX_LINE];
    char  newVal[MMAX_LINE];
    int   tmpNC;

    /* must handle test cases -l nodes=2 nodes=2:ppn=2 */

    if ((MJobPBSExtractArg(
          "-l",
          "nodes",
          CommandBuffer,
          tmpVal,            /* O */
          sizeof(tmpVal),
          TRUE) == SUCCESS) && (tmpVal[0] != '\0'))
      {
      /* FORMAT:  <NODECT>[:ppn=<TPN>] */

      if (MUIsNum(tmpVal) == FALSE)
        {
        snprintf(newVal,sizeof(newVal),"nodes=%s", tmpVal);
        MJobPBSInsertArg(
          "-l",
          newVal,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }
      else
        {
        /* this has the potential to lose attributes */
        /* doesn't work for multi-req jobs */

        tmpNC = (int)strtol(tmpVal,NULL,10);

        if (strstr(tmpVal,":ppn=") != NULL)
          {
          int tmpTPN = J->Req[0]->TasksPerNode;

          if  ((MPar[0].MaxPPN > 1) && 
               (tmpTPN > MPar[0].MaxPPN) && 
               (tmpTPN % MPar[0].MaxPPN == 0))
            {
            snprintf(newVal,sizeof(newVal),"nodes=%d:ppn=%d",
              tmpNC / MPar[0].MaxPPN,
              MPar[0].MaxPPN);

            /* clear cached node usage info */

            J->Request.NC = 0;

            if (J->Req[0] != NULL)
              J->Req[0]->NodeCount = 0;
            }
          else
            {
            snprintf(newVal,sizeof(newVal),"nodes=%d:ppn=%d",
              tmpNC,
              MAX(1,tmpTPN));
            }
          }
        else
          {
          snprintf(newVal,sizeof(newVal),"nodes=%s",
            tmpVal);
          }

        MJobPBSInsertArg(
          "-l",
          newVal,
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
        }
      }  /* END if ((MJobPBSExtractArg() == SUCCESS) && ...) */
    }    /* END if (bmisset(&MPar[0].JobNodeMatchBM,mnmpExactNodeMatch)) */

  /* Remove what -l depend args there on in the command buffer, and replace with one depend. 
   * If there are multiple -l depends, torque will only take the first. Also adds dependencies 
   * if a job is submitted without a submit string (bsub.lsf.pl only submits xml). It's 
   * possible that the job has dependencies * but they aren't in the command buffer. */

  if (J->Depend != NULL) 
    {
    mstring_t  tmpDepStr("depend=");
    char       tmpVal[MMAX_LINE];

    MDependToString(&tmpDepStr,J->Depend,mrmtPBS);
    
    /* remove all -l depends and an W x = depend.  We're only modifing the -l depends */
    MJobRemovePBSDirective(NULL,"depend",CommandBuffer,tmpVal,sizeof(tmpVal));

    MJobPBSInsertArg("-l",tmpDepStr.c_str(),CommandBuffer,CommandBuffer,sizeof(CommandBuffer));
    }  /* END if (J->Depend != NULL) */

  if (bmisset(&R->Flags,mrmfProxyJobSubmission) == TRUE)
    {
    char tmpCreds[MMAX_LINE];

    if (R->Version < 248)
      {
      char tmpLine[MMAX_LINE];

      snprintf(tmpLine,sizeof(tmpLine),"RM flag SUBMITJOBSASROOT not supported with this version, %d. Must be >= 2.4.8.",
          R->Version);

      MDB(1,fSTRUCT) MLog("ERROR:    %s\n",tmpLine);

      MMBAdd(
        &MSched.MB,
        tmpLine,
        NULL,
        mmbtNONE,
        0,
        0,
        NULL);

      if (EMsg != NULL)
        strcpy(EMsg,tmpLine);

      return(FAILURE);
      }

    snprintf(tmpCreds,sizeof(tmpCreds),"%s:%s",
        J->Credential.U->Name,
        J->Credential.G->Name);

    MJobPBSInsertArg(
        "-P",
        tmpCreds, 
        CommandBuffer,
        CommandBuffer,
        sizeof(CommandBuffer));

    if (R->SyncJobID == TRUE)
      {
      char *ptr;

      /* FORMAT:  <jobindex>|<schedname>.<jobindex> */

      /* NOTE:  jobid must be >= 1 */

      ptr = strchr(J->Name,'.');

      if (ptr == NULL)
        ptr = J->Name;
      else
        ptr++;

      MJobPBSInsertArg(
          "-J",
          ptr, 
          CommandBuffer,
          CommandBuffer,
          sizeof(CommandBuffer));
      }  /* END if (R->SyncJobID == TRUE) */
    }

  if ((J->Env.BaseEnv != NULL) && (MSched.JobRemoveEnvVarList != NULL))
    {
    /* Remove specified env variables before migrating job */

    MJobRemoveEnvVarList(J,MSched.JobRemoveEnvVarList);
    }

  MDB(3,fPBS) MLog("INFO:     submitting script as user %s%s with command '%s' - script: '%s'\n",
    J->Credential.U->Name,
    (bmisset(&R->Flags,mrmfProxyJobSubmission) == TRUE) ? " by proxy" : "",
    SubmitExe,
    CommandBuffer);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  MUSpawnChild(
    SubmitExe,
    J->Name,
    NULL,
    NULL,
    (bmisset(&R->Flags,mrmfProxyJobSubmission)) ? -1 : J->Credential.U->OID,
    (bmisset(&R->Flags,mrmfProxyJobSubmission)) ? -1 : J->Credential.G->OID,
    (SubmitDirIsLocal == TRUE) ? SubmitDir : MSched.SpoolDir,
    J->Env.BaseEnv,
    NULL,
    NULL,
    CommandBuffer,     /* I - input buffer */
    NULL,
    &OBuf,
    &EBuf,
    NULL,
    NULL,
    10000,             /* timeout (child is killed after this time) */
    0,                 /* blocktime */
    NULL,
    mxoNONE,
    FALSE,
    FALSE,
    EMsg);

  if ((EBuf != NULL) && (EBuf[0] != '\0'))
    {
    char    *ptr;

    mbool_t SubmitFailed = TRUE;

    /* NOTE:  OBuf may contain additional filter info */

    if (!strcmp(EBuf,"cannot set GID"))
      {
      strcpy(EMsg,"cannot launch command - no permission to change UID");

      if (SC != NULL)
        *SC = mscNoAuth;
      }
    else if ((ptr = strstr(EBuf,"qsub:")) != NULL)
      {
      /* NOTE:  all qsub errors will begin with the string 'qsub:' written to stderr */

      if (strstr(EBuf,"Unauthorized"))
        {
        snprintf(EMsg,MMAX_LINE,"%s - user=%s group=%s",
          ptr,
          J->Credential.U->Name,
          J->Credential.G->Name);
        }
      else
        {
        MUStrCpy(EMsg,ptr,MMAX_LINE);
        }

      /* most likely cred or file mapping error */

      if (SC != NULL)
        *SC = mscBadRequest;
      }
    else if (strstr(EBuf,"Permission denied") != NULL)
      {
      /* bad permissions on executable */

      MUStrCpy(EMsg,EBuf,MMAX_LINE);
      }
    else 
      {
      /* ignore message, probably from filter */

      SubmitFailed = FALSE;
      }

    if (SubmitFailed == TRUE)
      {
      MUFree(&EBuf);

      MFURemove(CmdFile);

      return(FAILURE);
      }
    }    /* END if ((EBuf != NULL) && (EBuf[0] != '\0')) */

  if ((OBuf == NULL) || (OBuf[0] == '\0') || strstr(OBuf,"ERROR"))
    {
    MDB(0,fRM) MLog("ALERT:    no job ID detected\n");

    if (EMsg[0] == '\0')
      {
      strcpy(EMsg,"unknown failure - no job ID detected");
      }

    MFURemove(CmdFile);

    if (SC != NULL)
      *SC = mscSysFailure;

    return(FAILURE);
    }

  /* NOTE:  OBuf may contain additional filter info */

  MUStrCpy(JobName,OBuf,MMAX_NAME);

  /* remove newline */

  JobName[strlen(JobName) - 1] = '\0';

  MUFree(&OBuf);
  MUFree(&EBuf);

  MDB(1,fPBS) MLog("INFO:     successfully submitted job %s\n",
    JobName);

  remove(CmdFile);

  return(SUCCESS);
  }  /* END MPBSJobSubmit() */





/**
 * Convert command file to command line args.
 *
 * NOTE:  used to create job scripts to be exec'd by msub.
 *
 * NOTE:  currently only used in conjunction w/interactive jobs 
 *
 * @see MPBSJobSubmit() - parent
 *
 * @param J (I)
 * @param R (I)
 * @param SubmitExe (I) [modified as side-affect]
 * @param CmdFile (I)
 * @param CmdArgs (O)
 * @param CmdArgSize (I)
 * @param EMsg (O) [optional]
 */

int __MPBSCmdFileToArgs(

  mjob_t *J,          /* I */
  mrm_t  *R,          /* I */
  char   *SubmitExe,  /* I (modified as side-affect) */
  char   *CmdFile,    /* I */
  char   *CmdArgs,    /* O */
  int     CmdArgSize, /* I */
  char   *EMsg)       /* O (optional) */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  char *ptr;
  char *TokPtr;

  if (CmdArgs != NULL)
    CmdArgs[0] = '\0';

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((SubmitExe == NULL) || (CmdFile == NULL) || (CmdArgs == NULL))
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,CmdArgs,CmdArgSize);

  if (SubmitExe[0] != '/')
    {
    char tmpPath[MMAX_LINE];

    if (MFUGetFullPath(
          SubmitExe,
          MUGetPathList(),
          (R != NULL) ? R->PBS.SBinDir : NULL,
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

    MUSNCat(&BPtr,&BSpace,tmpPath);
    }
  else
    {
    MUSNCat(&BPtr,&BSpace,SubmitExe);
    }

  ptr = MUStrTok(CmdFile,"\n",&TokPtr);

  while (ptr != NULL)
    {
    if (!strncasecmp(ptr,"#PBS",strlen("#PBS")))
      {
      MUSNCat(&BPtr,&BSpace,ptr + strlen("#PBS"));
      }

    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (ptr != NULL) */

#if 0
  /* don't know why this code is here, removed for RT6967
     by adding a separate -W here we override any other
     -W that might already be on the job, the two should
     be merged.  Nevertheless, if the system already has
     EXACTNODE why do we need to explicitly set it on the job
     unless this job is going to a system that does not
     have EXACTNODE */

  if ((J != NULL) && bmisset(&J->NodeMatchBM,mnmpExactNodeMatch))
    {
    /* if job node match policy set, explictly apply to job submit script */

    if ((R != NULL) && (R->Version >= 220) && (R->Version < 500))
      {
      MUSNCat(&BPtr,&BSpace," -W x=nmatchpolicy:exactnode");
      }
    }
#endif

  if ((R != NULL) && (R->Version >= 241) && (J->Env.Cmd != NULL))
    {
    char *ArgBuf = NULL;

   if (J->Env.Args != NULL)
     {
     int   ArgBufSize = 0;
     
     /* Preserve quotes by escaping them */
     ArgBufSize = strlen(J->Env.Args) + 100;
     ArgBuf = (char *)MUMalloc(ArgBufSize);
     MUStrReplaceStr(J->Env.Args,"\"", "\\\"", ArgBuf,ArgBufSize);
     }

    MUSNPrintF(&BPtr,&BSpace," %s %s",
      J->Env.Cmd,
      (ArgBuf != NULL) ? ArgBuf : "");

    if (ArgBuf != NULL)
      MUFree(&ArgBuf);
    }

  return(SUCCESS);
  }  /* END __MPBSCmdFileToArgs() */

