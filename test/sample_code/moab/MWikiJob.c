/* HEADER */

      
/**
 * @file MWikiJob.c
 *
 * Contains: MWikiJob
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"


/* Local structures/globals */

typedef struct {
  enum MJobAttrEnum  JAttr;    /* job attribute */
  const char              *SpecName;
  const char              *WikiJAttr;
  } mwjattr_t;


typedef struct {
  enum MReqAttrEnum  ReqAttr;    /* job attribute */
  const char              *SpecName;
  const char              *WikiJAttr;
  } mwreqattr_t;


static const mwreqattr_t MReqModAttr[] = {
  { mrqaSpecNodeFeature,        "feature",      "RFEATURES" },
  { mrqaReqNodeFeature,        "feature",      "RFEATURES" },
  { mrqaLAST,        NULL,      NULL }
  };



/**
 * Issue job requirement modify request via WIKI interface.
 *
 * @see MWikiJobModify() - parent
 **/

static inline mbool_t IsJobReqAtt(const int idx) 
  {
  return MReqModAttr[idx].SpecName != NULL;
  }

/**
 *
 * @param Name (I)
 */

static int FindReqAttrIndex(const char* Name) 
  {
  int aindex = 0;
  for (aindex = 0;MReqModAttr[aindex].SpecName != NULL;aindex++)
    {
    if (!strcasecmp(Name,MReqModAttr[aindex].SpecName))
      {
      /* match located */

      break;
      }
    }  /* END for (aindex) */
  return aindex;
  }


/**
 * Send signal to job via WIKI interface.
 *
 * @see MRMJobSignal() - parent
 *
 * @param J (I) [modified]
 * @param R (I)
 * @param Sig (I) [optional, -1=notset]
 * @param Action (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiJobSignal(

  mjob_t *J,      /* I (modified) */
  mrm_t  *R,      /* I */
  int     Sig,    /* I (optional, -1=notset) */
  char   *Action, /* I non-signal based action or <ACTION>=<VAL> (optional) */
  char   *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  int    *SC)     /* O (optional) */

  {
  int   tmpSC;
  char *Response;

  char  Command[MMAX_LINE];
  char  tmpSig[MMAX_LINE];

  const char *FName = "MWikiJobSignal";

  MDB(1,fWIKI) MLog("%s(%s,%s,%d,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    Sig,
    (Action != NULL) ? Action : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != 0)
    *SC = 0;

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if (MUBuildSignalString(Sig,Action,tmpSig) == FAILURE)
    {
    return(FAILURE);
    }

  /* SLURM appears to only support <INT> for SIGNAL */

  MUSignalFromString(tmpSig,&Sig);

  sprintf(Command,"%s%s %s%s %s%s %s%d",
    MCKeyword[mckCommand],
    MWMCommandList[mwcmdSignalJob],
    MCKeyword[mckArgs],
    (J->DRMJID != NULL) ? J->DRMJID : J->Name,
    "ACTION=",
    "SIGNAL",
    "VALUE=",
    Sig);

  if (MWikiDoCommand(
        R,
        mrmXJobSignal,
        &R->P[R->ActingMaster],
        FALSE,
        R->AuthType,
        Command,
        &Response,  /* O (alloc) */
        NULL,
        FALSE,
        EMsg,       /* O */
        &tmpSC) == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot requeue job '%s' through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      strcpy(EMsg,"rm failure");
      }

    R->FailIteration = MSched.Iteration;

    return(FAILURE);
    }

  MUFree(&Response);

  if (tmpSC < 0)
    {
    mbool_t IsFailure;

    MDB(2,fWIKI) MLog("ALERT:    cannot requeue job '%s' through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    switch (tmpSC)
      {
      case -2021:  /* job finished */

        IsFailure = FALSE;

        break;

      case -1:     /* general error */
      case -2017:  /* bad job id */
      case -2010:  /* job has no user id */
      case -2009:  /* job script missing */
      case -2020:  /* bad state */
      default:

        IsFailure = TRUE;

        break;
      }

    if (IsFailure == TRUE)
      {
      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        sprintf(EMsg,"rm failed with rc=%d",
          tmpSC);
        }

      R->FailIteration = MSched.Iteration;

      return(FAILURE);
      }
    }    /* END if (tmpSC < 0) */

  MDB(1,fPBS) MLog("INFO:     job '%s' successfully signalled\n",
    J->Name);

  return(SUCCESS);
  }  /* END MWikiJobSignal() */



/**
 * Issue job modify request via WIKI interface.
 *
 * @see MRMJobModify() - parent
 *
 * NOTE:  job modify support must be added on an attribute by attribute
 *        basis.
 *
 * @param J (I) [required]
 * @param R (I)
 * @param Name (I)
 * @param Resource (I) [optional]
 * @param Value (I) [optional]
 * @param Op (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiJobModify(

  mjob_t               *J,        /* I (required) */
  mrm_t                *R,        /* I */
  const char           *Name,     /* I */
  const char           *Resource, /* I (optional) */
  const char           *Value,    /* I (optional) */
  enum MObjectSetModeEnum Op,     /* I */
  char                 *EMsg,     /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)       /* O (optional) */

  {
  char    Command[MMAX_LINE];
  char    tmpLine[MMAX_LINE];

  int     tmpSC;
  char   *Response;
  const char   *NamePtr;

  int     aindex;

  const mwjattr_t MJModAttr[] = {
    { mjaAccount,        "account",      "BANK" },
    { mjaAccount,        "Account_Name", "BANK" },
    { mjaDepend,         "depend",       "DEPEND" },
    { mjaDescription,    "comment",      NULL },
    { mjaJobName,        "jobname",      "JOBNAME" },
    { mjaReqSMinTime,    "minstarttime", "MINSTARTTIME" },
    { mjaPAL,            "partition",    NULL },
    { mjaClass,          "queue",        "PARTITION" },
    { mjaUserPrio,       "userprio",     NULL },
    { mjaReqAWDuration,  "walltime",     "TIMELIMIT" },
    { mjaLAST,           NULL,           NULL } };

  const char *FName = "MWikiJobModify";

  MDB(1,fWIKI) MLog("%s(%s,R,%s,%s,%s,%d,EMsg,SC)\n",
    FName,
    (J    != NULL) ? J->Name : "NULL",
    (Name != NULL) ? Name : "NULL",
    (Resource != NULL) ? Resource : "NULL",
    (Value != NULL) ? Value : "NULL",
    Op);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((J == NULL) || (R == NULL))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"invalid job specified");

    return(FAILURE);
    }

  if ((Name == NULL) || (Name[0] == '\0'))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"invalid attribute specified");

    return(FAILURE);
    }

  /* NOTE:  do not yet support incr/decr */

 if ((MSched.Iteration == R->FailIteration) || (MSched.Mode == msmMonitor))
    {
    if (MSched.Mode == msmMonitor)
      {
      MDB(3,fWIKI) MLog("INFO:     cannot modify job '%s' (monitor mode)\n",
        J->Name);

      sleep(2);
      }
    else
      {
      MDB(3,fWIKI) MLog("INFO:     cannot modify job '%s' (fail iteration)\n",
        J->Name);
      }

    return(FAILURE);
    }

  /* discover which variable holds the attribute to modify */

  if (!strcasecmp(Name,"Resource_List") && (Resource != NULL))
    {
    NamePtr = Resource;
    }
  else
    {
    NamePtr = Name;
    }

  if (NamePtr == NULL)
    {
    return(FAILURE);
    }

  for (aindex = 0;MJModAttr[aindex].SpecName != NULL;aindex++)
    {
    if (!strcasecmp(NamePtr,MJModAttr[aindex].SpecName))
      {
      /* match located */

      break;
      }
    }  /* END for (aindex) */

  if (MJModAttr[aindex].SpecName == NULL)
    {
    const int ReqAttrIdx = FindReqAttrIndex(NamePtr);

    if(!IsJobReqAtt(ReqAttrIdx)) 
      {
      MDB(3,fWIKI) MLog("INFO:     cannot modify attribute '%s' of job '%s' (unknown attribute)\n",
        NamePtr,
        J->Name);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot modify attribute '%s' - unknown attribute",
          NamePtr);
        }

      return(FAILURE);
      }
    else 
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s=%s",
        MReqModAttr[ReqAttrIdx].WikiJAttr,
        Value);
      }
    }  /* END if (MJModAttr[aindex].SpecName == NULL) */
  else 
    {

    /* FORMAT:  CMD=JOBMODIFY ARG=<jobid> [PARTITION=<name>] [NODES=<nodes>] [BANK=<name>]
          [TIMELIMIT=<minutes>] [DEPEND=[afterany:]<jobid>] */

    /* NYI Slurm will be adding PROCS which we will send whenever we modify NODES */

    switch (MJModAttr[aindex].JAttr)
      {
      case mjaAccount:
      case mjaClass:
      case mjaDepend:
      case mjaJobName:
      case mjaReqSMinTime:

        snprintf(tmpLine,sizeof(tmpLine),"%s=%s",
          MJModAttr[aindex].WikiJAttr,
          Value);

        break;

      case mjaDescription:
      case mjaPAL:

        /* Moab's concept of "partition" is incompatible with SLURM's;
         * in other words, ignore this request */

        return(SUCCESS);
   
        /*NOTREACHED*/

        break;

      case mjaReqAWDuration:

        {
        long tmpTime;

        tmpTime = strtol(Value,NULL,10) / MCONST_MINUTELEN;

        tmpTime = MAX(1,tmpTime);  /* must be at least 1 or greater, otherwise SLURM will ignore change request */

        snprintf(tmpLine,sizeof(tmpLine),"%s=%ld",
          MJModAttr[aindex].WikiJAttr,
          tmpTime);
        }

        break;

      case mjaUserPrio:

        /* NYI - priority modification not yet supported in SLURM 1.2.3 */

        return(SUCCESS);

        /*NOTREACHED*/

        break;

      default:

        MDB(3,fWIKI) MLog("INFO:     cannot modify attribute '%s' of job '%s' (attribute not handled)\n",
          NamePtr,
          J->Name);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot modify attribute '%s' - not handled",
            NamePtr);
          }

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (MJModAttr[aindex].JAttr) */
    } /* END else (MJModAttr[aindex].SpecName == NULL) */

  snprintf(Command,sizeof(Command),"%s%s %s%s %s",
    MCKeyword[mckCommand],
    MWMCommandList[mwcmdModifyJob],
    MCKeyword[mckArgs],
    (J->DRMJID != NULL) ? J->DRMJID : J->Name,
    tmpLine);

  if (MWikiDoCommand(
        R,
        mrmJobModify,
        &R->P[R->ActingMaster],
        FALSE,
        R->AuthType,
        Command,
        &Response,  /* O (alloc) */
        NULL,
        FALSE,
        EMsg,       /* O */
        &tmpSC) == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot modify job '%s' through %s RM (WIKI failure)\n",
      J->Name,
      MRMType[R->Type]);

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      strcpy(EMsg,"RM failure");
      }

    R->FailIteration = MSched.Iteration;

    return(FAILURE);
    }

  MUFree(&Response);

  if (tmpSC < 0)
    {
    mbool_t IsFailure;

    MDB(2,fWIKI) MLog("ALERT:    cannot modify job '%s' through %s RM (errnum: %d)\n",
      J->Name,
      MRMType[R->Type],
      tmpSC);

    switch (tmpSC)
      {
      case -2021:  /* job finished */

        IsFailure = FALSE;

        break;

      case -1:     /* general error */
      case -2017:  /* bad job id */
      case -2010:  /* job has no user id */
      case -2009:  /* job script missing */
      case -2020:  /* bad state */
      default:

        IsFailure = TRUE;

        break;
      }

    if (IsFailure == TRUE)
      {
      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        sprintf(EMsg,"rm failed with rc=%d",
          tmpSC);
        }

      R->FailIteration = MSched.Iteration;

      return(FAILURE);
      }
    }    /* END if (tmpSC < 0) */

  MDB(2,fWIKI) MLog("INFO:     job '%s' modified through %s RM\n",
    J->Name,
    MRMType[R->Type]);

  return(SUCCESS);
  }  /* END MWikiJobModify() */




/**
 *
 *
 * @param J (I)
 * @param R (I)
 * @param JPeer (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiJobRequeue(

  mjob_t  *J,      /* I */
  mrm_t   *R,      /* I */
  mjob_t **JPeer,  /* I */
  char    *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  int     *SC)     /* O (optional) */

  {
  char    Command[MMAX_LINE];

  int     tmpSC;
  char   *Response;

  char    tEMsg[MMAX_LINE];

  const char *FName = "MWikiJobRequeue";

  MDB(2,fWIKI) MLog("%s(%s,%s,JPeer,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if (R->Version < 10200)
    {
    MDB(3,fWIKI) MLog("INFO:     cannot requeue job '%s' (SLURM version does not support requeue operation)\n",
      J->Name);

    return(FAILURE);
    }

  /* fail in TEST mode */

  if ((MSched.Iteration == R->FailIteration) || (MSched.Mode == msmMonitor))
    {
    if (MSched.Mode == msmMonitor)
      {
      MDB(3,fWIKI) MLog("INFO:     cannot requeue job '%s' (monitor mode)\n",
        J->Name);

      sleep(2);
      }
    else
      {
      MDB(3,fWIKI) MLog("INFO:     cannot requeue job '%s' (fail iteration)\n",
        J->Name);
      }

    return(FAILURE);
    }

  sprintf(Command,"%s%s %s%s",
    MCKeyword[mckCommand],
    MWMCommandList[mwcmdRequeueJob],
    MCKeyword[mckArgs],
    (J->DRMJID != NULL) ? J->DRMJID : J->Name);

  if (MWikiDoCommand(
        R,
        mrmJobRequeue,
        &R->P[R->ActingMaster],
        FALSE,
        R->AuthType,
        Command,
        &Response,  /* O (alloc) */
        NULL,
        FALSE,
        tEMsg,      /* O */
        &tmpSC) == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot requeue job '%s' through %s RM (%s)\n",
      J->Name,
      MRMType[R->Type],
      (tEMsg[0] != '\0') ? tEMsg : "???");

    if ((EMsg != NULL) && (tEMsg[0] == '\0'))
      {
      strcpy(EMsg,"rm failure");
      }
    else
      {
      MUStrCpy(EMsg,tEMsg,MMAX_LINE);
      }

    R->FailIteration = MSched.Iteration;

    return(FAILURE);
    }

  MUFree(&Response);

  if (tmpSC < 0)
    {
    mbool_t IsFailure;

    MDB(2,fWIKI) MLog("ALERT:    cannot requeue job '%s' through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    switch (tmpSC)
      {
      case -2021:  /* job finished */

        IsFailure = FALSE;

        break;

      case -1:     /* general error */
      case -2017:  /* bad job id */
      case -2010:  /* job has no user id */
      case -2009:  /* job script missing */
      case -2020:  /* bad state */
      default:

        IsFailure = TRUE;

        break;
      }

    if (IsFailure == TRUE)
      {
      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        sprintf(EMsg,"rm failed with rc=%d",
          tmpSC);
        }

      R->FailIteration = MSched.Iteration;

      return(FAILURE);
      }
    }    /* END if (tmpSC < 0) */

  MDB(2,fWIKI) MLog("INFO:     job '%s' requeued through %s RM\n",
    J->Name,
    MRMType[R->Type]);

  return(SUCCESS);
  }  /* END MWikiJobRequeue() */






/**
 * This function accepts a list of Wiki attributes, parses them, and then loads
 * them into the given job structure.
 *
 * @param JobID (I)
 * @param AList (I) attribute list
 * @param J (I) NOTE: this job is updated, not initialized [modified]
 * @param DRQ (I) [optional]
 * @param TaskList (O) [optional,minsize=MSched.JobMaxTaskCount]
 * @param R (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MWikiJobLoad(

  const char *JobID,
  char       *AList,
  mjob_t     *J,
  mreq_t     *DRQ,
  int        *TaskList,
  mrm_t      *R,
  char       *EMsg)

  {
  char    *ptr;
  char    *JobAttr;

  mreq_t  *RQ;

  int      rqindex;
  char     AccountName[MMAX_NAME];
  char    *TokPtr = NULL;

  const char *FName = "MWikiJobLoad";

  MDB(2,fWIKI) MLog("%s(%s,%s,J,%d,TaskList,%s,EMsg)\n",
    FName,
    JobID,
    AList,
    (DRQ != NULL) ? DRQ->Index : -1,
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (J == NULL)
    {
    MDB(2,fWIKI) MLog("ALERT:    invalid job pointer specified\n");

    return(FAILURE);
    }

  if ((R == NULL) && !bmisset(&J->IFlags,mjifIsTemplate))
    {
    MDB(2,fWIKI) MLog("ALERT:    invalid rm pointer specified\n");

    return(FAILURE);
    }

  /* NOTE:  if SLURM, must load/support --contiguous (NYI) */

  MUStrCpy(J->Name,JobID,sizeof(J->Name));

  J->CTime = MSched.Time;
  J->MTime = MSched.Time;
  J->ATime = MSched.Time;
  J->UIteration = MSched.Iteration;

  if (R != NULL)
    {
    /* NOTE:  constrain job to only run on submission rm */

    if (J->SubmitRM == NULL)  /* May have been previously loaded by checkpoint. */
      {
      J->SubmitRM = &MRM[R->Index];
      }
    J->DestinationRM = &MRM[R->Index];
    }

  /* NOTE:  if not specified, set wclimit to max allowed */

  J->SpecWCLimit[0] = 0;
  J->SpecWCLimit[1] = 0;

  if (!bmisset(&MPar[0].Flags,mpfUseMachineSpeed))
    {
    J->WCLimit = J->SpecWCLimit[0];
    }

  /* add resource requirements information */

  if (J->Req[0] == NULL)
    {
    MReqCreate(J,NULL,&J->Req[0],FALSE);

    MRMReqPreLoad(J->Req[0],R);
    }

  RQ = (DRQ != NULL) ? DRQ : J->Req[0];  /* convenience variable */

  /* set req defaults */

  if (J->Credential.G == NULL)
    {
    if (MGroupAdd("NOGROUP",&J->Credential.G) == FAILURE)
      {
      MDB(1,fWIKI) MLog("ERROR:    cannot add default group for job %s\n",
        J->Name);
      }
    }

  bmclear(&RQ->ReqFBM);

  memset(RQ->ReqNR,0,sizeof(RQ->ReqNR));
  memset(RQ->ReqNRC,MDEF_RESCMP,sizeof(RQ->ReqNRC));

  RQ->TaskCount = 1;

  RQ->Opsys   = 0;
  RQ->Arch    = 0;

  if (TaskList != NULL)
    TaskList[0] = -1;

  if (R != NULL)
    {
    RQ->RMIndex  = R->Index;
    }

  ptr = AList;

  /* FORMAT:  <FIELD>=<VALUE>;[<FIELD>=<VALUE>;]... */

  JobAttr= MUStrTokE(ptr,";\n",&TokPtr);

  while (JobAttr != NULL)
    {
    while (isspace(*JobAttr) && (*JobAttr != '\0'))
      JobAttr++;

    /* MUPurgeEscape(JobAttr); */

    if (!strncasecmp(JobAttr,"TASKLIST=",strlen("TASKLIST=")))
      {
      JobAttr += strlen("TASKLIST=");

      if ((TaskList != NULL) && (TaskList[0] != '\0'))
        {
        MWikiAttrToTaskList(J,TaskList,JobAttr,&J->Alloc.TC);
        }
      }        /* END if (!strncasecmp(JobAttr,"TASKLIST=",strlen("TASKLIST="))) */
    else
      {
      if (MWikiJobUpdateAttr(JobAttr,J,DRQ,R,EMsg) == FAILURE)
        {
        MDB(1,fWIKI) MLog("ERROR:    cannot parse wiki string for job '%s'\n",
          J->Name);

        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          {
          snprintf(EMsg,MMAX_LINE,"cannot parse '%s'",
            JobAttr);
          }

        return(FAILURE);
        } 
      }        /* END else (!strncmp(JobAttr,"TASKLIST=",strlen("TASKLIST="))) */

    JobAttr = MUStrTokE(NULL,";\n",&TokPtr);
    }  /* END while (JobAttr != NULL) */

  if ((J->RMXString != NULL) &&
      (MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,NULL) == FAILURE))
    {
    MDB(1,fWIKI) MLog("ALERT:    cannot process extension line '%s' for job %s\n",
      (J->RMXString != NULL) ? J->RMXString : "NULL",
      J->Name);

   /* 4-22-10 DCH - In cases where we have a failure processing the extension string
    *               (e.g. a job dependency for a file that does not exist) we still
    *               want to load the job and not return a failure.
    *  
    return(FAILURE);
    */
    }

#ifdef MNOT
  /* 3-4-10 BOC RT7119 - Interactive jobs will now push SID,SJID at submission
   * if UseMoabJobID is TRUE. The job name will be synced with the SJID when 
   * SJID is processed. */

  /* rename job if required */

  if ((J->SystemJID != NULL) && (J->SystemID != NULL) && !strcmp(J->SystemID,"moab"))
    {
    MJobSetAttr(J,mjaDRMJID,(void **)J->Name,mdfString,mSet);

    MJobRename(J,J->SystemJID);

    MUFree(&J->SystemID);
    MUFree(&J->SystemJID);
    }
#endif

  /* authenticate job */

  if (!bmisset(&J->IFlags,mjifIsTemplate))
    {
    if ((J->Credential.U == NULL) || (J->Credential.G == NULL))
      {
      char tmpLine[MMAX_LINE];

      MDB(1,fWIKI) MLog("ERROR:    cannot authenticate job %s (no user/no group)\n",
        J->Name);

      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"cannot authenticate job - no user/no group");

      if (J->Credential.U == NULL)
        {
        snprintf(tmpLine,sizeof(tmpLine),"user must be specified for job %s - job will be destroyed",
          J->Name);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"group must be specified for job %s - job will be destroyed",
          J->Name);
        }

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(FAILURE);
      }  /* END if ((J->Cred.U == NULL) || (J->Cred.G == NULL)) */

    if (J->Credential.A != NULL)
      strcpy(AccountName,J->Credential.A->Name);
    else
      AccountName[0] = '\0';

    if (MJobSetCreds(
          J,
          J->Credential.U->Name,
          J->Credential.G->Name,
          AccountName,
          (1 << mxoAcct),  /* on load from SLURM, allow violation in account name */
          NULL) == FAILURE)
      {
      MDB(1,fWIKI) MLog("ERROR:    cannot set credentials for job %s (U: %s  G: %s  A: '%s')\n",
        J->Name,
        J->Credential.U->Name,
        J->Credential.G->Name,
        AccountName);

      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"cannot set creds");

      return(FAILURE);
      }

    if (J->Request.TC == 0)
      {
      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        J->Request.TC += RQ->TaskCount;
        }  /* END for (rqindex) */

      if ((J->State == mjsStarting) || (J->State == mjsRunning))
        {
        J->Alloc.TC = J->Request.TC;
        }
   
      if ((J->Request.TC == 0) && !bmisset(&J->IFlags,mjifTaskCountIsDefault))
        {
        MDB(1,fWIKI) MLog("ERROR:    no taskcount specified for job '%s'\n",
          J->Name);

        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"no taskcount specified");

        return(FAILURE);
        }
      }

    J->EState = J->State;

    J->SystemQueueTime = J->SubmitTime;

    if ((J->Credential.G == NULL) || !strcmp(J->Credential.G->Name,"NOGROUP"))
      {
      int   GID;
      char *ptr;
      char  tmpGName[MMAX_NAME];

      if ((GID = MUGIDFromUser(J->Credential.U->OID,NULL,NULL)) > 0)
        {
        ptr = MUGIDToName(GID,tmpGName);

        if (strncmp(ptr,"GID",strlen("GID")))
          {
          MGroupAdd(ptr,&J->Credential.G);

          MDB(1,fWIKI) MLog("INFO:     job '%s' assigned default group '%s'\n",
            J->Name,
            MUGIDToName(GID,tmpGName));
          }
        }
      }  /* END if ((J->Cred.G == NULL) || ... ) */
    }    /* END if (!bmisset(&J->IFlags,mjifIsTemplate)) */

  /* perform RM-specific customizations */

  if (((J->Credential.C != NULL) && (J->Credential.C->NAIsDefaultDedicated == TRUE)) || 
       ((R != NULL) && bmisset(&R->IFlags,mrmifDefaultNADedicated)))
    {
    /* jobs are dedicated unless explicitly specifed as shared */

    if (J->Req[0]->NAccessPolicy == mnacNONE)
      {
      J->Req[0]->NAccessPolicy = mnacSingleJob;

      MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s in job load\n",
        J->Name,
        MNAccessPolicy[J->Req[0]->NAccessPolicy]);
      }
    }

  return(SUCCESS);
  }  /* END MWikiJobLoad() */




/**
 * Update wiki job using WIKI attribute string.
 *
 * @see MWikiJobUpdateAttr() - child
 *
 * @param AList (I) [wiki formatted attr=val list]
 * @param J (I) [modified]
 * @param DRQ (I) [optional]
 * @param R (I)
 */

int MWikiJobUpdate(

  const char  *AList,   /* I (wiki formatted attr=val list) */
  mjob_t      *J,       /* I (modified) */
  mreq_t      *DRQ,     /* I default req (optional) */
  mrm_t       *R)       /* I */

  {
  char *ptr;
  char *jptr;

  char *tail;

  int *TaskList = NULL;

  enum MJobStateEnum OldState;

  const char *FName = "MWikiJobUpdate";

  MDB(2,fWIKI) MLog("%s(AList,%s,DRQ,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  J->ATime = MSched.Time;
  J->UIteration = MSched.Iteration;

  OldState = J->State;

  char *mutableAList=NULL;
  MUStrDup(&mutableAList,AList);

  ptr = mutableAList;

  TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

  TaskList[0] = -1;

  /* FORMAT:  <FIELD>=<VALUE>;[<FIELD>=<VALUE>;]... */

  mstring_t  JobString(MMAX_LINE);

  while ((tail = MUStrChr(ptr,';')) != NULL)
    {
    *tail = '\0';

    MStringSet(&JobString,ptr);

    *tail = ';';

    /* MUPurgeEscape(JobAttr); */

    /* JobAttr FORMAT:  <ATTR>=<VALUE> */

    if (!strncasecmp(JobString.c_str(),"TASKLIST=",strlen("TASKLIST="))) 
      {
      jptr = &JobString[strlen("TASKLIST=")];

      MWikiAttrToTaskList(J,TaskList,jptr,&J->Alloc.TC);
      }
    else
      {
      MWikiJobUpdateAttr(JobString.c_str(),J,DRQ,R,NULL);
      }

    ptr = tail + 1;
    }  /* END while() */

  if (!bmisset(&R->Flags,mrmfNoCreateAll))
    {
    /* reporting RM is a discovery RM - perform full RM attribute update */

    /* NOTE: this is causing jobs to be added to the MAQ twice, because
     * this function is already called in MNatWorkloadQuery. This is leading
     * to SIGSEGVs.
     *
     * UPDATE: 12-11-09 BOC - Uncommented out MRMJobPostUpdate in 5.4 to 
     * put it back to the way it was in 5.3. When commented out the wiki
     * tasklist is not updated and slurm jobs the jobs aren't added to
     * the MAQ table so the job disappears in showq, but shows in mdiag -j. */

    MRMJobPostUpdate(J,TaskList,OldState,R);
    }

  MUFree((char **)&TaskList);
  MUFree(&mutableAList);

  return(SUCCESS);
  }  /* END MWikiJobUpdate() */

/* END MWikiJob.c */
