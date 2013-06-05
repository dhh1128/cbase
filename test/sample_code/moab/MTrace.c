/* HEADER */

/**
 * @file MTrace.c
 *
 * Moab Trace
 */
        
/*                                               *
 * Contains:                                     *
 *                                               *
 * int MTraceLoadWorkload(Buffer,TraceLen,J,Mode,Version) *
 * int MTraceLoadResource(Buffer,TraceLen,N,Version) *
 * int MTraceGetWorkloadVersion(Buffer,Version)  *
 * int MTraceGetResourceVersion(Buffer,Version)  *
 * int MTraceBuildResource(N,Version,Buf,BufSize) *
 * int MTraceQueryWorkload(STime,ETime,JC,JList,LSize) *
 *                                               */

/* NOTE:  for jobs, use MJobToTString() */


#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"


/* file globals */
   
char NullBuf[MMAX_BUFFER];

/* local prototypes */

int MTraceGetRsv(char *,mrsv_t *,mjobconstraint_t *,int *,int);

/* END local prototypes */



/**
 * @see MTraceLoadResource() - peer
 *
 * @param VM (I)
 * @param Buf (O)
 */

int MTraceBuildVMResource(

  mvm_t     *VM,     /* I */
  mstring_t *Buf)    /* O */

  {
  mbitmap_t BM;

  int  cindex;

  char OpSys[MMAX_LINE];
  char Vars[MMAX_LINE];

  const char *FName = "MTraceBuildVMResource";

  MDB(3,fSIM) MLog("%s(%s,Buf)\n",
    FName,
    (VM != NULL) ? VM->VMID : "NULL");

  if ((VM == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf->clear();

  for (cindex = 0;cindex < MMAX_NAME;cindex++)
    {
    if (MAList[meOpsys][VM->ActiveOS][cindex] == '\0')
      break;

    if (MAList[meOpsys][VM->ActiveOS][cindex] == ' ')
      {
      OpSys[cindex] = '_';

      continue;
      }

    OpSys[cindex] = MAList[meOpsys][VM->ActiveOS][cindex];
    }

  OpSys[cindex] = '\0';

  bmset(&BM,mcmUser);

  MVMAToString(VM,NULL,mvmaVariables,Vars,sizeof(Vars),&BM);

  if (Vars[0] == '\0')
    {
    strcpy(Vars,NONE);
    }

  /* FORMAT:  <NAME> <SOVEREIGN> <POWERSTATE> <PARENTNODE> <SWAP> <MEMORY> <DISK> <MAXPROCS> <OPSYS> [<CLASS>]* [<VARIABLES>]* */

  if (MSched.WikiEvents == TRUE)
    {

    MStringAppendF(Buf," %s=%s",
                   MVMAttr[mvmaID], /* VMID= */
                   VM->VMID);

    MStringAppendF(Buf," %s=%s",
                   MVMAttr[mvmaSovereign], /* SOVEREIGN= */
                   MBool[bmisset(&VM->Flags,mvmfSovereign)]);

    MStringAppendF(Buf," %s=%s",
                   MVMAttr[mvmaPowerState], /* POWERSTATE= */
                   MPowerState[VM->PowerState]);

    MStringAppendF(Buf," %s=%s",
                   MVMAttr[mvmaContainerNode], /* CONTAINERNODE= */
                   (VM->N == NULL) ? NONE : VM->N->Name);

    MStringAppendF(Buf," %s=%dM",
                   MVMAttr[mvmaCSwap], /* RCSWAP= */
                   VM->CRes.Swap);

    MStringAppendF(Buf," %s=%dM",
                   MVMAttr[mvmaCMem], /* RCMEM= */
                   VM->CRes.Mem);

    MStringAppendF(Buf," %s=%dM",
                   MVMAttr[mvmaCDisk], /* RCDISK= */
                   VM->CRes.Disk);

    MStringAppendF(Buf," %s=%d",
                   MVMAttr[mvmaCProcs], /* RCPROC= */
                   VM->CRes.Procs);

    MStringAppendF(Buf," %s=%s",
                   MVMAttr[mvmaActiveOS], /* OS= */
                   OpSys);

    MStringAppendF(Buf," %s=%s",
                   MVMAttr[mvmaVariables], /* VARIABLE= */
                   Vars);
    }
  else
    {
    MStringAppendF(Buf,"%s SOVEREIGN=%s %s %s %dM %dM %dM %d %s %s",
      VM->VMID,
      MBool[bmisset(&VM->Flags,mvmfSovereign)],
      MPowerState[VM->PowerState],
      (VM->N == NULL) ? NONE : VM->N->Name,
      VM->CRes.Swap,
      VM->CRes.Mem,
      VM->CRes.Disk,
      VM->CRes.Procs,
      OpSys,
      Vars);
    }


  return(SUCCESS);
  }  /* END MTraceBuildVMResource() */


/**
 * This function extracts job information from the stats files and places the 
 * data into the given job. It is worthy to note that this function only 
 * populates CANCELED or COMPLETED jobs. Also, when adding more logic to this 
 * function, please ensure that every return FAILURE is also coupled with a 
 * MJobDestroy() where appropriate.
 *
 * NOTE: calling routine is responsible for freeing alloc'd job memory.
 *
 * NOTE: translate trace string to temporary job structure
 *
 * @see MJobToTString() - peer - convert job to trace string.
 * @see MJobEval() - child
 * @see MTraceGetWorkloadMatch() - parent
 *
 * @param Buffer   (I)
 * @param TraceLen (I) [optional,modified]
 * @param J        (O) [modified]
 * @param Mode     (I)
 */

int MTraceLoadWorkload(

  char                *Buffer,
  int                 *TraceLen,
  mjob_t              *J,
  enum MSchedModeEnum  Mode)

  {
  mrm_t *R = NULL;

  char *tail;
  char *head;

  char *ptr;

  char *tok;
  char *TokPtr;

  char *TraceLinePtr;
  char TraceLine[MMAX_BUFFER];  
  char tmpLine[MMAX_LINE + 1];
  char OID[MMAX_LINE + 1];
  char AllocHList[MMAX_BUFFER + 1];  /* FIXME:  should be dynamic - must hold max alloc taskmap */
  char ReqHList[MMAX_BUFFER + 1];
  char Reservation[MMAX_NAME + 1];

  char tmpTaskRequest[MMAX_LINE + 1];
  char tmpSpecWCLimit[MMAX_LINE + 1];
  char tmpState[MMAX_NAME + 1];
  char tmpOpsys[MMAX_NAME + 1];
  char tmpArch[MMAX_NAME + 1];
  char tmpMemCmp[MMAX_NAME + 1];
  char tmpDiskCmp[MMAX_NAME + 1];
  char tmpReqNFList[MMAX_LINE + 1];
  char tmpClass[MMAX_LINE + 1];
  char tmpRMName[MMAX_LINE + 1];
  char tmpCmd[MMAX_LINE + 1];
  char tmpRMXString[MMAX_LINE >> 2];
  char tmpParName[MMAX_NAME + 1];
  char tmpJFlags[MMAX_LINE + 1];
  char tmpQReq[MMAX_NAME + 1];
  char tmpASString[MMAX_LINE + 1];
  char tmpRMemBuf[MMAX_NAME + 1];
  char tmpRDiskBuf[MMAX_NAME + 1];
  char tmpDSwapBuf[MMAX_NAME + 1];
  char tmpDMemBuf[MMAX_NAME + 1];
  char tmpDDiskBuf[MMAX_NAME + 1];
  char UName[MMAX_NAME + 1];
  char GName[MMAX_NAME + 1];
  char AName[MMAX_NAME + 1];
  char tmpMsg[MMAX_LINE];

  char SetString[MMAX_LINE + 1];

  unsigned long tmpQTime;
  unsigned long tmpDTime;
  unsigned long tmpSTime;
  unsigned long tmpCTime;

  unsigned long tmpSQTime;


  unsigned long tmpSDate;
  unsigned long tmpEDate;

  int      TPN;
  int      TasksAllocated;
  int      index;
  int      rc;

  int      tmpI;

  int      tindex;

  mqos_t  *QDef;

  mreq_t  *RQ[MMAX_REQ_PER_JOB];

  mreq_t   tmpRQ;

  mnode_t *N;

  char     OType[MMAX_NAME + 1];
  char     EType[MMAX_NAME + 1];

  long     ReqProcSpeed;

  const char *FName = "MTraceLoadWorkload";

  MDB(5,fSIM) MLog("%s(Buffer,TraceLen,J,%s)\n",
    FName,
    MSchedMode[Mode]);

  if (J == NULL)
    {
    MDB(5,fSIM) MLog("ALERT:    NULL job pointer\n");

    return(FAILURE);
    }

  if (Buffer == NULL)
    {
    return(FAILURE);
    }

  if ((tail = strchr(Buffer,'\n')) == NULL)
    {
    /* assume line is '\0' terminated */

    if (TraceLen != NULL)
      tail = Buffer + *TraceLen;
    else
      tail = Buffer + strlen(Buffer);
    }

  head = Buffer;

  MUStrCpy(TraceLine,head,MIN(tail - head + 1,(int)sizeof(TraceLine)));

  MDB(8,fSIM) MLog("INFO:     parsing trace line '%s'\n",
    TraceLine);

  if (TraceLen != NULL)
    *TraceLen = (tail - head) + 1;

  /* eliminate comments */

  if ((ptr = strchr(TraceLine,'#')) != NULL)
    {
    MDB(5,fSIM) MLog("INFO:     detected trace comment line, '%s'\n",
      TraceLine);

    *ptr = '\0';
    }

  MSim.JTLines++;

  if (TraceLine[0] == '\0')
    {
    return(FAILURE);
    }

  /* set default workload attributes */

  memset(J,0,sizeof(mjob_t));

  memset(&tmpRQ,0,sizeof(tmpRQ));

  /* Set mjifTemporaryJob so the actual job's memory won't be freed by MJobDestroy later on. */
  bmset(&J->IFlags,mjifTemporaryJob);

  /* must have the GenricRes array or not all DRes members are preserved later on */
  MSNLInit(&tmpRQ.DRes.GenericRes);

  RQ[0]     = &tmpRQ;

  tmpMsg[0] = '\0';
  EType[0]  = '\0';

  TraceLinePtr = TraceLine;

  /* FORMAT: (HUMANTIME) ETIME[:EID] OTYPE OID ETYPE ... */

  if (((ptr = strstr(TraceLinePtr,":")) != NULL) &&
      ((unsigned int)(ptr - TraceLinePtr) < strlen("XX:XX:XX")))
    {
    /* new format - need to advance to keep things in sync */

    TraceLinePtr += strlen("XX:XX:XX ");
    }

  rc = sscanf(TraceLinePtr,"%64s %64s %64s %64s",
    tmpLine,  /* O: ignored */
    OType,
    OID,
    EType);

  if (strcmp(OType,MXO[mxoSched]) == 0)
    {
    /* If the Workload trace file is an events file, the first line could be
       a sched with a SCHEDSTART */

    MDB(7,fSIM) MLog("INFO:     'sched' object type found in event file - ignoring\n"); 

    MSNLFree(&tmpRQ.DRes.GenericRes);

    return(FAILURE);
    }

  if (strcmp(OType,MXO[mxoJob]))
    {
    /* ignore event record */

    /* NOTE: at this point, we do not know the trace event type.   */
    /*       assume completion and update sim stats */

    MSim.ProcessJC++;
    MSim.RejectJC++;
    MSim.RejReason[marCorruption]++;

    MSNLFree(&tmpRQ.DRes.GenericRes);

    return(FAILURE);
    }

  if (strcmp(EType,MRecordEventType[mrelJobCancel]) &&
      strcmp(EType,MRecordEventType[mrelJobComplete]))
    {
    /* ignore non-completion event record */

    MSNLFree(&tmpRQ.DRes.GenericRes);

    return(FAILURE);
    }

  MRMAddInternal(&R);

  /* initialize job fields */

  J->Req[0] = NULL;

  MRMJobPreLoad(J,NULL,&MRM[0]);

  J->StartCount = 0;
  J->Rsv          = NULL;

  /* process trace */

  MSim.ProcessJC++;

  SetString[0] = '\0';
  
  /*                      XX   ET  OT   OID  EType NC TASK USER GRP WALL STAT CLSS QT  DT  ST  CT  ARCH OS   MEM  MEM  DSK  DSK  FEAT QT  T  TPN QREQ FLAGS */

  rc = sscanf(TraceLine,"%64s %64s %64s %64s %64s %d %64s %64s %64s %64s %64s %64s %lu %lu %lu %lu %64s %64s %64s %64s %64s %64s %64s %lu %d %d  %64s %1024s %64s %1024s %1024s %d %lf %64s %d %64s %64s %64s %lu %lu %65536s %64s %65536s %64s %64s %1024s %64s %64s",
         tmpLine,
         tmpLine,
         tmpLine,
         J->Name,
         tmpLine,
         &J->Request.NC,
         tmpTaskRequest,
         UName,
         GName,
         tmpSpecWCLimit,
         tmpState,
         tmpClass,
         &tmpQTime,
         &tmpDTime,
         &tmpSTime,
         &tmpCTime,
         tmpArch,
         tmpOpsys,
         tmpMemCmp,
         tmpRMemBuf,
         tmpDiskCmp,
         tmpRDiskBuf,
         tmpReqNFList,           /* required node features */
         &tmpSQTime,             /* SystemQueueTime */
         &TasksAllocated,        /* allocated task count */
         &TPN,                   /* min required TPN */
         tmpQReq,
         tmpJFlags,
         AName,
         tmpCmd,
         tmpRMXString,           /* RM extension string */
         &J->BypassCount,        /* Bypass          */
         &J->PSUtilized,         /* PSUtilized      */
         tmpParName,             /* partition used  */
         &RQ[0]->DRes.Procs,
         tmpDMemBuf,
         tmpDDiskBuf,
         tmpDSwapBuf,
         &tmpSDate,
         &tmpEDate,
         AllocHList,
         tmpRMName,
         ReqHList,
         Reservation,
         tmpASString,            /* application simulator name */
         SetString,              /* resource sets required by job */
         tmpLine,                /* RES1 */
         tmpLine);               /* check for too many fields */

  /* fail if all fields not read in */

  switch (rc)
    {
    case 43:

      strcpy(AllocHList,NONE);

      break;

    case 44:
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:

      /* allow reserved fields (5) to be ignored */

      /* canonical 400 trace */

      break;

    default:

      MDB(1,fSIM) MLog("ALERT:    workload trace is corrupt '%s' (%d of %d fields read) ignoring line\n",
        TraceLine,
        rc,
        44);

      MJobDestroy(&J);

      MSim.RejectJC++;
      MSim.RejReason[marCorruption]++;

      if ((MSim.CorruptJTMsg == NULL) && (rc < 50))
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"workload trace corrupt on line %d: only %d fields could be read in '%.64s...'\n",
          MSim.JTLines,
          rc,
          TraceLine);

        MUStrDup(&MSim.CorruptJTMsg,tmpLine);
        }

      MSNLFree(&tmpRQ.DRes.GenericRes);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (rc) */

  J->SubmitRM = R;

  RQ[0]->ReqNR[mrMem]  = MURSpecToL(tmpRMemBuf,mvmMega,mvmMega);
  RQ[0]->ReqNR[mrDisk] = MURSpecToL(tmpRDiskBuf,mvmMega,mvmMega);

  RQ[0]->DRes.Mem       = MURSpecToL(tmpDMemBuf,mvmMega,mvmMega);
  RQ[0]->DRes.Swap      = MURSpecToL(tmpDSwapBuf,mvmMega,mvmMega);
  RQ[0]->DRes.Disk      = MURSpecToL(tmpDDiskBuf,mvmMega,mvmMega);

  if (Mode != msmProfile)
    {
    ptr  = NULL;

    if (strcmp(tmpASString,NONE) && strcmp(tmpASString,"-"))
      {
      if ((ptr = MUStrTok(tmpASString,":",&TokPtr)) != NULL)
        {
        MUStrTok(NULL,"|",&TokPtr);
        }
      }
    }    /* END if (Mode != msmProfile) */
  
  /* FIXME: we don't properly handle cancelled jobs yet, they will not show up as cancelled, 
            but they should finish when the cancel happened */

  /* obtain job step number from LL based job name */

  /* FORMAT:  <FROM_HOST>.<CLUSTER>.<PROC> */

  MUStrCpy(tmpLine,J->Name,sizeof(tmpLine));

  /* adjust job flags */

  if (Mode == msmProfile)
    {
    /* FORMAT:  <HOST>[:<COUNT>][{:,}<HOST>[:<COUNT>]]... */

    if (strchr(AllocHList,',') == NULL)
      {
      char *ptr; 

      ptr = strchr(AllocHList,':');

      if ((ptr != NULL) && (!isdigit(*(ptr + 1))))
        {
        /* old style colon delimited list located */

        MUStrReplaceChar(AllocHList,':',',',FALSE);
        }
      }

    /* NOTE:  do not add nodes discovered from job hostlist */

    if (isdigit(tmpTaskRequest[0]))
      MJobGrowTaskMap(J,(int)strtol(tmpTaskRequest,NULL,10));
    else
      MJobGrowTaskMap(J,MSched.JobMaxTaskCount);

    MTLFromString(AllocHList,J->TaskMap,J->TaskMapSize,TRUE);

    MJobNLFromTL(J,&J->NodeList,J->TaskMap,NULL);

    if (!MNLIsEmpty(&J->NodeList))
      MUStrDup(&J->MasterHostName,MNLGetNodeName(&J->NodeList,0,NULL));

    /* NOTE:  flags may be specified as string or number */

    bmfromstring(tmpJFlags,MJobFlags,&J->SpecFlags);
    }    /* END if ((Mode == msmProfile) || (MSim.JobSubmissionPolicy == msjsReplay)) */
  else
    {
    /* simulation mode */

    /* NOTE:  flags may be specified as string or number */

    bmfromstring(tmpJFlags,MJobFlags,&J->SpecFlags);

    bmunset(&J->SpecFlags,mjfBackfill);
    }    /* END else (Mode == msmProfile) */

  /* extract arbitrary job attributes */

  /* FORMAT:  JATTR:<ATTR>[=<VALUE>] */

  {
  char *ptr;
  char *TokPtr;

  ptr = MUStrTok(tmpJFlags,",",&TokPtr);

  while (ptr != NULL)
    {
    if (!strncmp(ptr,"JATTR:",strlen("JATTR:")))
      {
      MJobSetAttr(J,mjaGAttr,(void **)(ptr + strlen("JATTR:")),mdfString,mSet);
      }

    ptr = MUStrTok(NULL,"[]",&TokPtr);
    }  /* END while (ptr != NULL) */
  }    /* END BLOCK */

  /* load task info */

  if (!strncmp(tmpTaskRequest,"PBS=",strlen("PBS=")))
    {
    char tmpLine[MMAX_LINE];

    mtrma_t tmpA;
    int    *TaskList = NULL;

    memset(&tmpA,0,sizeof(tmpA));

    sprintf(tmpLine,"Resource_List,nodes,%s",
      &tmpTaskRequest[strlen("PBS=")]);

    TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

    MRMJobSetAttr(
      &MRM[0],
      J,
      NULL,
      tmpLine,
      &tmpA,
      TaskList,
      TRUE,
      NULL);

    MUFree((char **)&TaskList);
    }
  else
    {
    ptr = MUStrTok(tmpTaskRequest,",",&TokPtr);
    tindex = 1;

    while (ptr != NULL)
      {
      if (strchr(ptr,'-') != NULL)
        {
        /* NOTE:  not handled */
        }
      else
        {
        tmpI = (int)strtol(ptr,NULL,10);

        if (tmpI < J->Request.NC)
          {
          MDB(1,fSIM) MLog("ALERT:    job %s workload trace is corrupt 'NC > TC' (fixing)\n",
            J->Name);

          tmpI = MIN(tmpI,J->Request.NC); 
          }

        RQ[0]->TaskRequestList[tindex] = tmpI;

        tindex++;
        }

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (ptr != NULL) */

    RQ[0]->TaskRequestList[tindex] = 0;
    RQ[0]->TaskRequestList[0]      = RQ[0]->TaskRequestList[1];

    RQ[0]->TaskCount    = RQ[0]->TaskRequestList[0];

    J->Request.TC       = RQ[0]->TaskRequestList[0];

    RQ[0]->TasksPerNode = MAX(0,TPN);

    if (bmisset(&MPar[0].JobNodeMatchBM,mnmpExactNodeMatch))
      {
      RQ[0]->NodeCount  = RQ[0]->TaskCount / MAX(1,RQ[0]->TasksPerNode);
      J->Request.NC     = RQ[0]->NodeCount;
      }

    if (bmisset(&MPar[0].JobNodeMatchBM,mnmpExactProcMatch))
      {
      if (RQ[0]->TasksPerNode >= 1)
        {
        RQ[0]->ReqNR[mrProc]  = RQ[0]->TasksPerNode;
        RQ[0]->ReqNRC[mrProc] = mcmpEQ;
        }

      if (RQ[0]->TasksPerNode > J->Request.TC)
        {
        RQ[0]->TasksPerNode = 0;
        }
      }
    }    /* END else (!strncmp(tmpTaskRequest,"PBS:",strlen("PBS:"))) */

  /* process WCLimit constraints */

  ptr = MUStrTok(tmpSpecWCLimit,",:",&TokPtr);
  tindex = 1;

  while (ptr != NULL)
    {
    if (strchr(ptr,'-') != NULL)
      {
      /* NOTE:  not handled */
      }
    else
      {
      J->SpecWCLimit[tindex] = strtol(ptr,NULL,10);

      tindex++;
      }

    ptr = MUStrTok(NULL,",:",&TokPtr);
    }  /* END while (ptr != NULL) */

  J->SpecWCLimit[tindex] = 0;
  J->SpecWCLimit[0]      = J->SpecWCLimit[1];

  J->SubmitTime     = tmpQTime;
  J->DispatchTime   = MAX(tmpDTime,tmpSTime);
  J->StartTime      = MAX(tmpDTime,tmpSTime);
  J->CompletionTime = tmpCTime;

  J->SystemQueueTime = tmpSQTime;

  J->SpecSMinTime   = tmpSDate;
  J->CMaxDate       = tmpEDate;

  /* determine required reservation */

  if (strcmp(Reservation,NONE) &&
      strcmp(Reservation,"-") &&
      strcmp(Reservation,J->Name))
    {
    bmset(&J->SpecFlags,mjfAdvRsv);

    if (strcmp(Reservation,ALL) != 0)
      {
      MUStrDup(&J->ReqRID,Reservation);
      }
    }

  /* determine job class */

  if (strcmp(tmpClass,"-"))
    {
    MClassAdd(tmpClass,&J->Credential.C);
    }

  if (Mode != msmProfile)
    {
    if (strcmp(ReqHList,NONE) &&
        strcmp(ReqHList,"-"))
      {
      /* FORMAT:  <HOST>[{,:}<HOST>]... */

      bmset(&J->IFlags,mjifHostList);

      ptr = MUStrTok(ReqHList,",:",&TokPtr);

      MNLClear(&J->ReqHList);

      while (ptr != NULL)
        {
        if (MNodeFind(ptr,&N) == FAILURE)
          {
          if (MSched.DefaultDomain[0] != '\0')
            {
            if ((tail = strchr(ptr,'.')) != NULL)
              {
              /* try short name */

              MUStrCpy(ReqHList,ptr,MIN((int)sizeof(ReqHList),(tail - ptr + 1)));
              }
            else
              {
              /* append default domain */

              if (MSched.DefaultDomain[0] == '.')
                {
                sprintf(ReqHList,"%s%s",
                  ptr,
                  MSched.DefaultDomain);
                }
              else
                {
                sprintf(ReqHList,"%s.%s",
                  ptr,
                  MSched.DefaultDomain);
                }
              }

            if (MNodeFind(ReqHList,&N) != SUCCESS)
              {
              MDB(1,fUI) MLog("ALERT:    cannot locate node '%s' for job '%s' hostlist\n",
                ptr,
                J->Name);

              N = NULL;
              }
            }
          else
            {
            MDB(1,fUI) MLog("ALERT:    cannot locate node '%s' for job '%s' hostlist\n",
              ptr,
              J->Name);

            N = NULL;
            }
          }  /* END if (MNodeFind() == FAILURE) */

        if (N != NULL)
          {
          MNLAddNode(&J->ReqHList,N,1,TRUE,NULL);
          }

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }    /* END while (ptr != NULL) */
      }      /* END if (strcmp(ReqHList,NONE) && ...) */
    else if (bmisset(&J->IFlags,mjifHostList) && (MNLIsEmpty(&J->ReqHList)))
      {
      MDB(1,fSIM) MLog("ALERT:    job %s requests hostlist but has no hostlist specified\n",
        J->Name);

      bmunset(&J->IFlags,mjifHostList);
      }
    }    /* END if (MODE != msmProfile) */

  /* if there is already a req, free it to prevent a memory leak */

  if (J->Req[0] != NULL)
    {
    MReqDestroy(&J->Req[0]);
    J->Req[0] = NULL;
    }

  /* alloc/initialize job req */

  if (MReqCreate(J,&tmpRQ,&J->Req[0],FALSE) == FAILURE)
    {
    MDB(0,fSIM) MLog("ALERT:    cannot create job req\n");

    MJobDestroy(&J);

    MSNLFree(&tmpRQ.DRes.GenericRes);

    MSim.RejectJC++;
    MSim.RejReason[marMemory]++;

    return(FAILURE);
    }

  MSNLFree(&tmpRQ.DRes.GenericRes);

  RQ[0]         = J->Req[0];

  RQ[0]->Index  = 0;

  /* process trace attributes */

  if (strcmp(tmpMsg,NONE) && strcmp(tmpMsg,"-"))
    {
    char tMsg[MMAX_LINE];

    MUStringUnpack(tmpMsg,tMsg,sizeof(tMsg),NULL);

    if (tMsg[0] != '\0')
      {
      MJobSetAttr(J,mjaDescription,(void **)tMsg,mdfString,mSet);
      }
    }

  if (!strcmp(AName,NONE) ||
      !strcmp(AName,"-") ||
      !strcmp(AName,"0"))
    {
    AName[0] = '\0';
    }

  if (strcmp(tmpRMXString,NONE) && 
      strcmp(tmpRMXString,"-") &&
      strcmp(tmpRMXString,"0"))
    {
    MJobSetAttr(J,mjaRMXString,(void **)tmpRMXString,mdfString,mSet);
    }

  if (RQ[0]->DRes.Procs == 0)
    RQ[0]->DRes.Procs = 1;

  if (J->SpecWCLimit[0] == (unsigned long)-1)
    J->SpecWCLimit[0] = MMAX_TIME;

  MUStrDup(&J->Env.Cmd,tmpCmd);

  if (Mode == msmSim)
    {
    J->State           = mjsIdle;
    J->EState          = mjsIdle;

    J->PSUtilized      = 0.0;

    J->SystemQueueTime = J->SubmitTime;
    J->EstWCTime       = MAX(1,J->CompletionTime - J->StartTime);

    J->BypassCount   = 0;

    J->StartTime     = 0;
    J->DispatchTime  = 0;

    J->CompletionTime  = 0;

    J->WCLimit         = J->SpecWCLimit[0];
    }
  else
    {
    int   tmpI;
    char *tail;

    /* profile mode */

    tmpI = (int)strtol(tmpParName,&tail,10);

    if ((tmpI > 0) && (*tail == '\0'))
      {
      if (tmpI > MMAX_PAR)
        {
        /* partition is invalid, use default partition */

        RQ[0]->PtIndex = 1;
        }
      else if (MPar[tmpI].Name[0] != '\0')
        {
        /* assign job directly to requiested partition */

        RQ[0]->PtIndex = tmpI;
        }
      else 
        {
        /* partition is invalid, use default partition */

        RQ[0]->PtIndex = 1;
        }
      }
    else
      {
      mpar_t *P;

      if (MParFind(tmpParName,&P) == FAILURE)
        {
        RQ[0]->PtIndex = 1;  /* assign to default partition */
        }
      else
        {
        RQ[0]->PtIndex = P->Index;
        }
      }

    J->Alloc.TC = TasksAllocated;

    J->PSDedicated = J->TotalProcCount * (J->CompletionTime - J->StartTime);

    J->WCLimit = J->SpecWCLimit[0];

    for (index = 0;MJobState[index] != NULL;index++)
      {
      if (!strcmp(tmpState,MJobState[index]))
        {
        J->State  = (enum MJobStateEnum)index;
        J->EState = (enum MJobStateEnum)index;

        break;
        }
      }    /* END for (index) */
    }      /* END else (Mode == msmSim) */

  if (J->State == mjsNotRun)
    {
    MDB(3,fSIM) MLog("ALERT:    ignoring job '%s' (job never ran, state '%s')\n",
      J->Name,
      MJobState[J->State]);

    MJobDestroy(&J);

    MSim.RejectJC++;
    MSim.RejReason[marState]++;

    return(FAILURE);
    }

  /* check for timestamp corruption */

  if (Mode == msmProfile)
    {
    /* profiler mode */

    if (J->StartTime < J->DispatchTime)
      {
      MDB(2,fSIM) MLog("WARNING:  fixing job '%s' with corrupt dispatch/start times (%ld < %ld)\n",
        J->Name,
        J->DispatchTime,
        J->StartTime);

      J->DispatchTime = J->StartTime;
      }

    if (J->SubmitTime > J->StartTime)
      {
      MDB(2,fSIM) MLog("WARNING:  fixing job '%s' with corrupt queue/start times (%ld > %ld)\n",
        J->Name,
        J->SubmitTime,
        J->StartTime);

      if (J->SystemQueueTime > 0)
        J->SubmitTime = MIN(J->StartTime,J->SystemQueueTime);
      else
        J->SubmitTime = J->StartTime;
      }

    if (J->SystemQueueTime > J->DispatchTime)
      {
      MDB(2,fSIM) MLog("WARNING:  fixing job '%s' with corrupt system queue/dispatch times (%ld > %ld)\n",
        J->Name,
        J->SystemQueueTime,
        J->DispatchTime);

      J->SystemQueueTime = J->DispatchTime;
      }

    if (J->SystemQueueTime < J->SubmitTime)
      {
      MDB(2,fSIM) MLog("WARNING:  fixing job '%s' with corrupt system queue/queue time (%ld < %ld)\n",
        J->Name,
        J->SystemQueueTime,
        J->SubmitTime);

      J->SystemQueueTime = J->SubmitTime;
      }

    if ((J->SubmitTime > J->DispatchTime) ||
        (J->DispatchTime > J->CompletionTime))
      {
      MDB(1,fSIM) MLog("ALERT:    ignoring job '%s' with corrupt queue/start/completion times (%ld > %ld > %ld)\n",
        J->Name,
        J->SubmitTime,
        J->DispatchTime,
        J->CompletionTime);

      MJobDestroy(&J);

      MSim.RejectJC++;
      MSim.RejReason[marTime]++;

      return(FAILURE);
      }
    }    /* END if (Mode != msmSim) */

  if (MJobSetCreds(J,UName,GName,AName,0,NULL) == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    ignoring job '%s' with invalid authentication info (%s:%s:%s)\n",
      J->Name,
      UName,
      GName,
      AName);

    MJobDestroy(&J);

    MSim.RejectJC++;
    MSim.RejReason[marUser]++;

    return(FAILURE);
    }

  if (J->Credential.G != NULL)
    {
    /* if group is specified by trace, it is trusted */

    MULLAdd(&J->Credential.U->F.GAL,J->Credential.G->Name,NULL,NULL,NULL); /* no free routine */
    }

  /* determine network */

  if ((strcmp(tmpOpsys,"-")) &&
      (strstr(tmpOpsys,MAList[meOpsys][0]) == NULL))
    {
    if ((RQ[0]->Opsys = MUMAGetIndex(meOpsys,tmpOpsys,mAdd)) == FAILURE)
      {
      MDB(0,fSIM) MLog("WARNING:  cannot add opsys '%s' for job '%s'\n",
        tmpOpsys,
        J->Name);
      }
    }

  if ((strcmp(tmpArch,"-")) && 
      (strstr(tmpArch,MAList[meArch][0]) == NULL))
    {
    if ((RQ[0]->Arch = MUMAGetIndex(meArch,tmpArch,mAdd)) == FAILURE)
      {
      MDB(0,fSIM) MLog("WARNING:  cannot add arch '%s' for job '%s'\n",
        tmpArch,
        J->Name);
      }
    }

  /* load feature values */

  if ((MSched.ProcSpeedFeatureHeader[0] != '\0') &&
      (MSched.ReferenceProcSpeed > 0))
    {
    sprintf(tmpLine,"[%s",
      MSched.ProcSpeedFeatureHeader);

    if ((ptr = strstr(tmpReqNFList,tmpLine)) != NULL)
      {
      ReqProcSpeed = strtol(ptr + strlen(tmpLine),NULL,10);

      J->EstWCTime *= (long)((double)ReqProcSpeed / MSched.ReferenceProcSpeed);
      }
    }

  if (strcmp(tmpReqNFList,"-") && !strstr(tmpReqNFList,MAList[meNFeature][0]))
    {
    tok = MUStrTok(tmpReqNFList,"[]",&TokPtr);

    do
      {
      MUGetNodeFeature(tok,mAdd,&RQ[0]->ReqFBM);
      }
    while ((tok = MUStrTok(NULL,"[]",&TokPtr)) != NULL);
    }

  RQ[0]->ReqNRC[mrMem]  = MUCmpFromString(tmpMemCmp,NULL);
  RQ[0]->ReqNRC[mrDisk] = MUCmpFromString(tmpDiskCmp,NULL);

  /* NOTE: this setting overrides per-node settings  */

/*
  RQ[0]->NAccessPolicy = MSched.DefaultNAccessPolicy;
*/

  RQ[0]->TaskCount = J->Request.TC;
  RQ[0]->NodeCount = J->Request.NC;

  MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,NULL);

  {
  char *ptr;
  char *TokPtr;

  ptr = MUStrTok(tmpQReq,":",&TokPtr);

  if ((ptr != NULL) &&
      (ptr[0] != '\0') &&
       strcmp(ptr,"-1") &&
       strcmp(ptr,"0") &&
       strcmp(ptr,NONE) &&
       strcmp(ptr,"-") &&
       strcmp(ptr,DEFAULT))
    {
    if (MQOSAdd(ptr,&J->QOSRequested) == FAILURE)
      {
      /* cannot locate requested QOS */

      MQOSFind(DEFAULT,&J->QOSRequested);
      }

    if (Mode == msmProfile)
      {
      if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
        {
        MQOSAdd(ptr,&J->Credential.Q);
        }
      }
    }    /* END if ((ptr != NULL) && ...) */
  }      /* END BLOCK */

  if (Mode == msmSim)
    {
    bmset(&J->Flags,mjfNoRMStart);
    bmset(&J->SpecFlags,mjfNoRMStart);

    if ((MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&QDef) == FAILURE) ||
        (J->QOSRequested == NULL) ||
        (J->QOSRequested == &MQOS[0]))
      {
      MJobSetQOS(J,QDef,0);
      }
    else
      {
      MJobSetQOS(J,J->QOSRequested,0);
      }
    }    /* END if (Mode == msmSim) */

  if ((SetString[0] != '0') && 
       strcmp(SetString,NONE) && 
       strcmp(SetString,"-"))
    {
    /* FORMAT:  ONEOF,FEATURE[,X:Y:Z] */

    if ((ptr = MUStrTok(SetString,",: \t",&TokPtr)) != NULL)
      {
      /* determine selection type */

      RQ[0]->SetSelection = (enum MResourceSetSelectionEnum)MUGetIndexCI(
        ptr,
        (const char **)MResSetSelectionType,
        FALSE,
        mrssNONE);

      if ((ptr = MUStrTok(NULL,",: \t",&TokPtr)) != NULL)
        {
        /* determine set attribute */

        RQ[0]->SetType = (enum MResourceSetTypeEnum)MUGetIndexCI(
          ptr,
          (const char **)MResSetAttrType,
          FALSE,
          mrstNONE);

        index = 0;

        if ((ptr = MUStrTok(NULL,", \t",&TokPtr)) != NULL)
          {
          /* determine set list */

          while (ptr != NULL)
            {
            MUStrDup(&RQ[0]->SetList[index],ptr);

            index++;

            ptr = MUStrTok(NULL,", \t",&TokPtr);
            }
          }

        RQ[0]->SetList[index] = NULL;
        }  /* END if ((ptr = MUStrTok(NULL)) != NULL) */
      }    /* END if ((ptr = MUStrTok(SetString)) != NULL) */
    }      /* END if ((SetString[0] != '0') && strcmp(SetString,NONE) && ...) */

  if (MJobEval(J,R,NULL,NULL,NULL) == FAILURE)
    {
    /* job not properly formed */

    MDB(1,fSTRUCT) MLog("ALERT:    ignoring job '%s' with corrupt configuration\n",
      J->Name);

    MJobDestroy(&J);

    MSim.RejectJC++;
    MSim.RejReason[marSystemLimits]++;

    return(FAILURE);
    }

  MDB(6,fSIM) MLog("INFO:     job '%s' loaded.  class: %s  opsys: %s  arch: %s\n",
    J->Name,
    (J->Credential.C != NULL) ? J->Credential.C->Name : "NONE",
    MAList[meOpsys][RQ[0]->Opsys],
    MAList[meArch][RQ[0]->Arch]);

  return(SUCCESS);
  }  /* END MTraceLoadWorkload() */





/**
 * Load workload described within simulation trace files.
 * 
 * @see MTraceGetWorkloadMatch() - child
 *
 * @param StartTime (I)
 * @param EndTime (I)
 * @param JC (I)
 * @param JList (O) [minsize=MMAX_JOB]
 * @param BufSize (I)
 */

int MTraceQueryWorkload(

  long              StartTime,  /* I */
  long              EndTime,    /* I */
  mjobconstraint_t *JC,         /* I */
  mjob_t           *JList,      /* O (minsize=MMAX_JOB) */
  int               BufSize)    /* I */

  {
  int JobCount = 0;
  int rc = SUCCESS;
  mevent_itr_t Itr;
  enum MXMLOTypeEnum OType[] = {mxoJob,mxoLAST};

  const char *FName = "MTraceQueryWorkload";

  MDB(5,fSIM) MLog("%s(%ld,%ld,JC,JList,%d)\n",
    FName,
    StartTime,
    EndTime,
    BufSize);

  if (JList != NULL)
    JList[0].Name[0] = '\0';

  if (JList == NULL)
    {
    return(FAILURE);
    }

  if (StartTime > EndTime)
    StartTime = EndTime - 1;

  if (MSysQueryEvents(StartTime,EndTime,NULL,NULL,OType,NULL,&Itr) == SUCCESS)
    {
    rc = MTraceGetWorkloadMatch(
          &Itr,
          JC,
          JList,
          &JobCount,
          BufSize - 1);

    /* terminate list */

    JList[JobCount].Name[0] = '\0';

    MSysEItrClear(&Itr);

    return(rc);
    }
  else
    {
    return(FAILURE);
    }
  }  /* END MTraceQueryWorkload() */







/**
 * Read the events files and populate JobList based on the events read.
 *
 * @see MTraceLoadWorkload() - child
 * @see MTraceQueryWorkload() - parent
 * 
 * @param Itr            (I)
 * @param ConstraintList (I)
 * @param JobList        (O) [minsize=MMAX_JOB]
 * @param JobIndex       (I/O) [optional]
 * @param MaxJobs        (I)
 */

int MTraceGetWorkloadMatch(

  mevent_itr_t      *Itr,
  mjobconstraint_t  *ConstraintList,
  mjob_t            *JobList,
  int               *JobIndex,
  int                MaxJobs)

  {
  char tmpLine[MMAX_LINE];
  char AllocHList[MMAX_BUFFER];
  char ReqHList[MMAX_BUFFER];
  char Reservation[MMAX_NAME];
  char tmpTaskRequest[MMAX_LINE];
  char tmpSpecWCLimit[MMAX_LINE];
  char tmpState[MMAX_NAME];
  char tmpOpsys[MMAX_NAME];
  char tmpArch[MMAX_NAME];
  char tmpNetwork[MMAX_NAME];
  char tmpMemCmp[MMAX_NAME];
  char tmpDiskCmp[MMAX_NAME];
  char tmpReqNFList[MMAX_LINE];
  char tmpClass[MMAX_LINE];
  char tmpRMName[MMAX_LINE];
  char tmpCmd[MMAX_LINE];
  char tmpRMXString[MMAX_LINE >> 2];
  char tmpParName[MMAX_NAME];
  char tmpJFlags[MMAX_LINE];
  char tmpQReq[MMAX_NAME];
  char tmpASString[MMAX_LINE];
  char tmpRMemBuf[MMAX_NAME];
  char tmpRDiskBuf[MMAX_NAME];
  char tmpDSwapBuf[MMAX_NAME];
  char tmpDMemBuf[MMAX_NAME];
  char tmpDDiskBuf[MMAX_NAME];
  char UName[MMAX_NAME];
  char GName[MMAX_NAME];
  char AName[MMAX_NAME];
  char SetString[MMAX_LINE];
  char tmpMsg[MMAX_LINE];

  unsigned long tmpQTime;
  unsigned long tmpDTime;
  unsigned long tmpSTime;
  unsigned long tmpCTime;
  unsigned long tmpSQTime;
  unsigned long tmpSDate;
  unsigned long tmpEDate;

  mreq_t  RQ[MMAX_REQ_PER_JOB];

  int      TPN;
  int      TasksAllocated;

  int      NodeCount;
  int      BypassCount;
  double   PSUtilized;

  mbool_t  ValMatch;

  int      aindex;
  int      vindex;

  int      JIndex;
  
  /* The duplicate job search index */
  int      DJIndex = 0;
  /* If true, a duplicate is found in the tmeporary list.*/
  mbool_t  DuplicateFound;

  int itrRC;
  mevent_t Event;
  
  mulong   tmpL;

  JIndex = (JobIndex != NULL) ? *JobIndex : 0;

  while ((itrRC = MSysEItrNext(Itr,&Event)) == SUCCESS)
    {
    if (JIndex >= MaxJobs)
      {
      MDB(3,fSTAT) MLog("INFO:     max jobs found in completed job query");

      if (JobIndex != NULL)
        *JobIndex = JIndex;

      return(SUCCESS);
      }

    sscanf(Event.Description,"%d %64s %64s %64s %64s %64s %64s %lu %lu %lu %lu %64s %64s %64s %64s %64s %64s %64s %64s %lu %d %d %64s %1024s %64s %1024s %1024s %d %lf %64s %d %64s %64s %64s %lu %lu %65536s %64s %65536s %64s %64s %1024s %1024s %1024s %1024s %1024s %1024s %1024s %1024s %1024s",
      &NodeCount,         /* %d   1 */
      tmpTaskRequest,     /* %64s 1 */
      UName,              /* %64s wightman */
      GName,              /* %64s wightman */
      tmpSpecWCLimit,     /* %64s 180 */
      tmpState,           /* %64s Vacated */
      tmpClass,           /* %64s [DEFAULT:1] */
      &tmpQTime,          /* %lu  1074791109 */
      &tmpDTime,          /* %lu  1074791109 */
      &tmpSTime,          /* %lu  1074791109 */
      &tmpCTime,          /* %lu  1074791424 */
      tmpNetwork,         /* %64s [NONE] */
      tmpArch,            /* %64s [NONE] */
      tmpOpsys,           /* %64s [NONE] */
      tmpMemCmp,          /* %64s >= */
      tmpRMemBuf,         /* %64s 0M */
      tmpDiskCmp,         /* %64s >= */
      tmpRDiskBuf,        /* %64s 0M */
      tmpReqNFList,       /* %64s [NONE] */
      &tmpSQTime,         /* %lu 1074791109 SystemQueueTime */
      &TasksAllocated,    /* %d  1 allocated task count */
      &TPN,               /* %d  0 */
      tmpQReq,            /* %64s [NONE:high] */
      tmpJFlags,          /* %1024s [RESTARTABLE] */
      AName,              /* %64s live */
      tmpCmd,             /* %1024s [NONE] */
      tmpRMXString,       /* %1024s [NONE] RM extension string */
      &BypassCount,       /* %d 0 Bypass          */
      &PSUtilized,        /* %lf 0.00 PSUtilized      */
      tmpParName,         /* %64s DEFAULT partition used  */
      &RQ[0].DRes.Procs, /* %d 1 */
      tmpDMemBuf,         /* %64s 0M */
      tmpDDiskBuf,        /* %64s 0M */
      tmpDSwapBuf,        /* %64s 0M */
      &tmpSDate,          /* %lu  0 */
      &tmpEDate,          /* %lu  2140000000 */
      AllocHList,         /* %65536s aloha */
      tmpRMName,          /* %64s base */
      ReqHList,           /* %65536s [NONE] */
      Reservation,        /* %64s [NONE] */
      SetString,          /* %64s [DEFAULT] resource sets required by job */
      tmpASString,        /* %1024s [NONE] application simulator name */
      tmpMsg,             /* %1024s */
      tmpLine,            /* cost    */
      tmpLine,            /* %1024s RES1 [NONE] */
      tmpLine,            /* %1024s RES2 [NONE] */
      tmpLine,            /* %1024s RES3 [NONE] */
      tmpLine,            /* %1024s RES4 [NONE] */
      tmpLine,            /* %1024s RES5 [NONE] */
      tmpLine);           /* check for too many fields */

    /* enforce constraints */

    ValMatch = TRUE;

    for (aindex = 0;ConstraintList[aindex].AType != mjaNONE;aindex++)
      {
      ValMatch = FALSE;

      switch (ConstraintList[aindex].AType)
        {
        case mjaJobID:

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if (!strcmp(ConstraintList[aindex].AVal[vindex],Event.Name))
              ValMatch = TRUE;
            }  /* END for (vindex) */

          break;

        case mjaClass:

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if (strstr(tmpClass,ConstraintList[aindex].AVal[vindex]))
              ValMatch = TRUE;
            }  /* END for (vindex) */

          break;

        case mjaGroup:

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if (!strcmp(ConstraintList[aindex].AVal[vindex],GName))
              ValMatch = TRUE;
            }  /* END for (vindex) */

          break;

        case mjaQOS:

          {
          char *ptr;
          char *TokPtr;

          char *QOS;

          if ((ptr = MUStrTok(tmpQReq,":",&TokPtr)) == NULL)
            {
            QOS = tmpQReq;
            }
          else
            {
            QOS = TokPtr;
            }

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if (!strcmp(ConstraintList[aindex].AVal[vindex],QOS))
              ValMatch = TRUE;
            }

          } /* END BLOCK */

          break;

        case mjaState:

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if (!strcmp(ConstraintList[aindex].AVal[vindex],tmpState))
              ValMatch = TRUE;
            }
           
          break;

        case mjaUser:

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if (!strcmp(ConstraintList[aindex].AVal[vindex],GName))
              ValMatch = TRUE;
            }  /* END for (vindex) */

          break;

        case mjaAllocNodeList:

          {
          char *ptr;

          int   tmpLength;

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if ((ptr = strstr(AllocHList,ConstraintList[aindex].AVal[vindex])) == NULL)
              continue;

            /* Potential Match -- Check for beginning, termination, or comma */

            tmpLength = strlen(ConstraintList[aindex].AVal[vindex]);

            if ((ptr > AllocHList) && (*(ptr - 1) != ','))
              continue;

            if ((ptr[tmpLength] != '\0') && (ptr[tmpLength] != ','))
              continue;

            ValMatch = TRUE;

            break;
            }     /* END for (vindex) */
          }       /* END BLOCK */

          break;

        case mjaAccount:

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            if (!strcmp(ConstraintList[aindex].AVal[vindex],AName))
              ValMatch = TRUE;
            }

          break;

        case mjaRMXString:

          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            if (ConstraintList[aindex].AVal[vindex][0] == '\0')
              break;

            /* (NYI) */

            if (strstr(tmpRMXString,ConstraintList[aindex].AVal[vindex]))
              ValMatch = TRUE;
            }

          break;

        case mjaReqAWDuration:

          /* NOTE:  also require arch, featurelist (req attrs) */

          /* NYI */

          break;

        case mjaStartTime:
	   
           for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            tmpL = ConstraintList[aindex].ALong[vindex];

            if (tmpL <= 0)
              break;

            switch (ConstraintList[aindex].ACmp)
              {
              case mcmpLE:
              default:

                if ( tmpL <= tmpSTime)
                  ValMatch = TRUE;

                break;

              case mcmpGE:

                 if (tmpL >= tmpSTime)
                  ValMatch = TRUE;

                break;
              } /* END switch (ConstraintList[aindex].ACmp) */
            }   /* END for (vindex) */

          break;

        case mjaCompletionTime:
	   
          for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
            {
            tmpL = ConstraintList[aindex].ALong[vindex];

            if (tmpL <= 0)
              break;

            switch (ConstraintList[aindex].ACmp)
              {
              case mcmpLE:
              default:

                if (tmpCTime <= tmpL)
                  ValMatch = TRUE;

                break;

              case mcmpGE:

                 if (tmpCTime >= tmpL)
                  ValMatch = TRUE;

                break;
              } /* END switch (ConstraintList[aindex].ACmp) */
            }   /* END for (vindex) */

          break;

        default:

          /* constraint ignored */

          break;
        } /* END switch (ConstraintList[aindex].AType) */

      if (ValMatch == FALSE)
        {
        break;
        }
      }   /* END for(aindex) */

    if (ValMatch == TRUE)
      {
      char TraceLine[MMAX_LINE << 2];
      struct tm *tmpT;
      char tmpTime[MMAX_NAME];
      enum MSchedModeEnum OMode;
      time_t Time;

      Time = (time_t)Event.Time;
      tmpT = localtime(&Time);

      sprintf(tmpTime,"%02d:%02d:%02d",
        tmpT->tm_hour,
        tmpT->tm_min,
        tmpT->tm_sec);


      snprintf(TraceLine,sizeof(TraceLine),"%s %ld:%d %-8s %-12s %-12s %s\n",
        tmpTime,
        Time,
        Event.ID,
        MXO[Event.OType],
        (Event.Name[0] != 0) ? Event.Name : "-",
        (Event.EType != mrelNONE) ? MRecordEventType[Event.EType] : "-",
        (Event.Description[0] != 0) ? Event.Description : "-");

      OMode = MSched.Mode;

      MSched.Mode = msmProfile;  /* preserve trace data if in simulation */

      if (MTraceLoadWorkload(
          TraceLine,
          NULL,
          &JobList[JIndex],
          MSched.Mode) == SUCCESS)
        {
        if (!MJOBISCOMPLETE(&JobList[JIndex]))
          {
          JobList[JIndex].State  = mjsCompleted;
          JobList[JIndex].EState = mjsCompleted;
          }

        /* NOTE:  does not account for suspend time */

        JobList[JIndex].AWallTime = 
          JobList[JIndex].CompletionTime - JobList[JIndex].StartTime;

        DuplicateFound = FALSE;
        
        /* Look for a duplicate job and use the DJIndex to merge the
         * cancel job information with the completed job information. 
         */
        for (DJIndex = 0;DJIndex < JIndex;DJIndex++)
          {
          if (!strcmp(JobList[DJIndex].Name,JobList[JIndex].Name))
            {
            DuplicateFound = TRUE;

            break;
            }
          }
        
        if (DuplicateFound == TRUE) 
          {
          /* If a duplicate is found, merge the duplicate's internal cancel flag. */

          if (bmisset(&JobList[DJIndex].IFlags,mjifWasCanceled))
            {
            mjob_t *DJob;

            bmset(&JobList[JIndex].IFlags,mjifWasCanceled);

            /* Move the completed job with cancelled information 
             * into the original job's slot and free the current JIndex.
             */

            DJob = &JobList[DJIndex];
            MJobDestroy(&DJob);
            memcpy(&JobList[DJIndex],&JobList[JIndex],sizeof(mjob_t));
            JobList[JIndex].Name[0] = '\0';

            /* Don't increment the JIndex for the next job in the checkpoint file.*/
            }
          else
            {
            /* Let the duplicate job through */

            JIndex++;

            MDB(1,fSTAT) MLog("ALERT:    duplicate job object found for %s", 
              JobList[DJIndex].Name);
            }
          }
        else
          {
          /* Else job not already found in temporary list, so simply increment the
           * job index.*/

          JIndex++;
          }
        }

      MSched.Mode = OMode;
      }
    }    /* END while (ptr != NULL) */

  if (JobIndex != NULL)
    *JobIndex = JIndex;

  return(itrRC != FAILURE);
  }  /* END MTraceGetWorkloadMatch() */



#define MTBMAX_JOB     10000
#define MTBMAX_ACCOUNT 32
#define MTBMAX_ATTR    16


typedef struct {
  char *Key;
  char *Val[MTBMAX_ATTR + 1];
  } mtab_t;

int MaxJob = MTBMAX_JOB;

#ifdef MNOT

mtab_t MTAcct[MTBMAX_ACCOUNT] = {
  { "FairfieldOil", { NULL } },
  { "GeneticsInc", { NULL } },
  { "NYU", { NULL } },
  { "CancerSearchLab", { NULL } },
  { "SoftTest", { NULL } },
  { NULL, { NULL } } };


mtab_t MTArch[MTBMAX_ACCOUNT] = {
  { "FairfieldOil",    { "P655", "P690", "E325", "x335","x345", NULL } },
  { "GeneticsInc",     { NULL } },
  { "NYU",             { "E325", "x335", "x345", "x345", "x345", NULL } },
  { "CancerSearchLab", { NULL } },
  { "SoftTest",        { "P655", "P690", "E325", "X335","x345", "x345", "x345", "X345", "x345", "x345", "x345", "x345", "x345", "x345", "x345", "X355", NULL } },
  { NULL, { NULL } } };


/* NOTE arch maps to OS */

mtab_t MTArchToOS[MTBMAX_ACCOUNT] = {
  { "P655",        { "AIX5L", "AIX5L", "AIX5L", "AIX5L","AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", NULL } },
  { "P690",        { "AIX5L", "AIX5L", "AIX5L", "AIX5L","AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", NULL } },
  { "E325",        { "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", NULL } },
  { "x335",        { "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", "Redhat", NULL } },
  { "x345",        { "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", NULL } },
  { NULL, { NULL } } };

mtab_t MTUser[MTBMAX_ACCOUNT] = {
  { "FairfieldOil",    { "wcarroll", "lwelch", "rshurtz", "mbentley","amadsen", "reynolds", "drahman", "npeters", "jsmith", "roberts", "cjackson", "gwolsey", "gwolsey", "gwolsey", "awok", "awok", NULL } },
  { "GeneticsInc",     { "sthompsn", "jjones", "tgates", "mwillis", "mwillis", "jjones", "jjones", "rjung", "jjones", "nsmith", "jjones", "tgates", "tgates", "tgates", "tgates",  "tgates", NULL } },
  { "NYU",             { "walfort", "allendr", "ralsop", "tamaker", "yangus", "uannan", "iannesley", "oappleby", "parbuthnot", "armitage", "sartois", "tamaker", "jbburton", "gastor", "yangus", "yangus", NULL } },
  { "CancerSearchLab", { "ibentley", "jbethune", "lbeverly", "lbeverly", "lbeverly", "lbeverly", "sam13", "sam13", "mbirch", "sam13","mshaw", "mshaw", "mshaw", "sam13", "sam13", "sam13", NULL } },
  { "SoftTest",        { "ufelton", "cfosdyke", "cfosdyke", "cfosdyke", "cfosdyke", "ufelton", "jfoote", "jfoote", "jfoote", "jfoote", "jfoote", "kforbes", "lfordham", "cfosdyke", "jfoote", "fothgill", NULL } },
  { NULL, { NULL } } };

mtab_t MTOpsys[MTBMAX_ACCOUNT] = {
  { "FairfieldOil",    { "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", NULL } },
  { "GeneticsInc",     { "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "Suse8", "AIX5L", "AIX5L", "Suse8", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "AIX5L", "Suse8", NULL } },
  { "NYU",     { "Redhat", "Suse8","Redhat", "Suse8","Redhat", "Suse8","Redhat", "Suse8","Redhat", "Suse8","Redhat", "Suse8","Redhat", "Suse8","Redhat", "Suse8", NULL } },
  { "CancerSearchLab", { "Redhat", "Suse8", "Redhat", "Suse8", "Redhat", "Suse8", "Redhat", "Suse8", "Redhat", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", "Suse8", NULL } },
  { "SoftTest",        { "AIX5L", "Redhat", "Suse8", "AIX5L", "Redhat", "Suse8", "AIX5L", "Redhat", "Suse8", "Redhat", "Suse8", "Redhat", "Suse8", "Redhat", "Suse8", "Suse8", NULL } },
  { NULL, { NULL } } };

mtab_t MTDuration[MTBMAX_ACCOUNT] = {
  { "FairfieldOil",    { "12000", "16000", "24000", "3600", "864000", "1080000", "108000", "1080000", "1080000", "108000", "1080", "108000", "12000", "16000", "16000", "32000" , NULL } },
  { "GeneticsInc",     { "120000", "16000", "24000", "3600", "86400", "10800", "108000", "10800", "10800", "10800", "10800", "10800", "12000", "1600", "1600", "32000" , NULL } },
  { "NYU",             { "50", "3600", "86400", "86400", "172800", "129600", "7032", "8888", "50", "3600", "86400", "86400", "172800", "129600", "7032", "8888", NULL } },
  { "CancerSearchLab", { "120000", "16000", "24000", "3600", "86400", "10800", "108000", "10800", "10800", "10800", "10800", "10800", "12000", "1600", "1600", "32000" , NULL } },
  { "SoftTest",        { "50", "3600", "86400", "86400", "172800", "129600", "7032", "8888", "24000", "3600", "86400", "10800", "108000", "10800", NULL } },
  { NULL, { NULL } } };



mtab_t MTQOS[MTBMAX_ACCOUNT] = {
  { "FairfieldOil",    { NULL, NULL, NULL, NULL, NULL, "FFPremium", "FFPremium", "Fast", "Fast", "FFPremium", "FFPremium", NULL, NULL, NULL, NULL, NULL, NULL } },
  { "GeneticsInc",    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL } },
  { "NYU",    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Fast", NULL, NULL } },
  { "CancerSearchLab", { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Fast", NULL, NULL } },
  { "SoftTest",    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Fast", NULL, NULL } },
  { NULL, { NULL } } };


mtab_t MTClass[MTBMAX_ACCOUNT] = {
  { "FairfieldOil", { "Bigmem", "TorqueBatch", "TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","Bigmem", "Bigmem","Bigmem", "Bigmem","Bigmem", "Bigmem","Bigmem", "Bigmem","Bigmem", "Bigmem", NULL } },
  { "GeneticsInc", { "LoadLevelerBatch", "LoadLevelerBatch", "LoadLevelerBatch", "LoadLevelerBatch","LoadLevelerBatch", "LoadLevelerBatch","LoadLevelerBatch", "LoadLevelerBatch","LoadLevelerBatch", "LoadLevelerBatch","LoadLevelerBatch", "LoadLevelerBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch", NULL } },
  { "NYU", { "TorqueBatch", "TorqueBatch", "TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","Bigmem", "Bigmem", NULL } },
  { "CancerSearchLab", { "TorqueBatch", "TorqueBatch", "TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch","TorqueBatch", "TorqueBatch", NULL } },
  { "SoftTest", { "TorqueBatch", "TorqueBatch", "TorqueBatch", "TorqueBatch","LoadLevelerBatch", "LoadLevelerBatch","LoadLevelerBatch", "LoadLevelerBatch","Bigmem", "Bigmem","Bigmem", "Bigmem","Bigmem", "Bigmem","Bigmem", "Bigmem", NULL } },
  { NULL, { NULL } } };

mtab_t MTTC[MTBMAX_ACCOUNT] = {
  { "FairfieldOil", { "8", "16", "2", "2", "2", "2", "2", "4", "4", "8", "8", "4", "4", "2", "2", "2" , NULL } },
  { "GeneticsInc", { "8", "16", "2", "2", "2", "2", "2", "4", "4", "8", "8", "4", "4", "2", "2", "2" , NULL } },
  { "NYU", { "8", "16", "2", "2", "2", "2", "2", "4", "4", "8", "8", "4", "4", "2", "2", "2" , NULL } },
  { "CancerSearchLab", { "8", "16", "2", "2", "2", "2", "2", "4", "4", "8", "8", "4", "4", "2", "2", "2" , NULL } },
  { "SoftTest", { "8", "16", "2", "1", "1", "1", "3", "4", "4", "5", "8", "7", "4", "11", "13", "20" , NULL } },
  { NULL, { NULL } } };
#endif



mtab_t MTAcct[MTBMAX_ACCOUNT];
mtab_t MTAcctWeights[MTBMAX_ACCOUNT];
mtab_t MTArch[MTBMAX_ACCOUNT];
mtab_t MTArchToOS[MTBMAX_ACCOUNT];
mtab_t MTClass[MTBMAX_ACCOUNT];
mtab_t MTDuration[MTBMAX_ACCOUNT];
mtab_t MTTPN[MTBMAX_ACCOUNT];
mtab_t MTOpsys[MTBMAX_ACCOUNT];
mtab_t MTQOS[MTBMAX_ACCOUNT];
mtab_t MTTC[MTBMAX_ACCOUNT];
mtab_t MTUser[MTBMAX_ACCOUNT];
mtab_t MTGroup[MTBMAX_ACCOUNT];


mtab_t *MTabIndex[] = {
  &MTAcct[0],
  &MTAcctWeights[0],
  &MTArch[0],
  &MTArchToOS[0],
  &MTClass[0],
  &MTDuration[0],
  &MTGroup[0],
  &MTTPN[0],
  &MTOpsys[0],
  &MTQOS[0],
  &MTTC[0],
  &MTUser[0],
  NULL };

const char *MTabName[] = {
  "ACCOUNT",
  "ACCOUNTWEIGHTS",
  "ARCH",
  "ARCHTOOS",
  "CLASS",
  "DURATION",
  "GROUP",
  "TPN",
  "OPSYS",
  "QOS",
  "TASKCOUNT",
  "USER",
  NULL };

/* a job will be assigned a random completion time between 0 and *
 * WCLimit * MDEF_COMPLETIONSCALAR */

#define MDEF_COMPLETIONSCALAR   1.2  




/**
 * NOTE: THIS ROUTINE IS BROKEN!!!
 *       See note at bottom (located by the call to MJobToTString)
 *
 * @param Buffer (O)
 * @param BufSize (I)
 */

int MTraceCreateJob(

  char *Buffer,  /* O */
  int   BufSize) /* I */

  {
  int index;
  int index2;
  int index3;

  int aindex;

  int    tmpRand;
  long   tmpRandL;

  mulong StartTime = MSched.SchedTime.tv_sec;

  mjob_t *J;
  mreq_t *RQ;

  char    tmpBuf[MMAX_BUFFER];

  static  mbool_t IsInitialized = FALSE;

  char   *ptr;

  static int JCounter = 1;

  if (Buffer == NULL)
    {
    return(FAILURE);
    }

  if (IsInitialized == FALSE)
    {
    int rc;

    char *ptr;
    char *TokPtr = NULL;

    char tmpAttr[MTBMAX_ATTR + 1][MMAX_NAME + 1];
    char tmpKey[MMAX_NAME + 1];

    char *buf;

    ptr = MUStrTok(MSim.WorkloadTraceFile,":",&TokPtr);

    ptr = MUStrTok(NULL,":",&TokPtr);

    /* load trace data */

    if ((ptr == NULL) || 
       ((buf = MFULoadNoCache(ptr,1,NULL,NULL,NULL,NULL)) == NULL))
      {
      /* no trace data specified, use defaults */
 
      /* NO-OP */
      }
    else
      {
      char *cptr;
      char *lptr;

      mtab_t *tptr;

      /* extract tables */

      for (index = 0;MTabName[index] != NULL;index++)
        {
        ptr = buf;

        while ((ptr = strstr(ptr,MTabName[index])) != NULL)
          {
          cptr = strrchr(ptr,'#');
          lptr = strrchr(ptr,'\n');

          if ((cptr != NULL) && ((cptr > lptr) || (lptr == NULL)))
            {
            /* ignore comment line */

            ptr++;
            }
          else
            { 
            break;
            }
          }

        if (ptr == NULL)
          continue;

        ptr += strlen(MTabName[index]);

        ptr = strchr(ptr,'\n');

        ptr++;

        index2 = 0;

        tptr = MTabIndex[index];

        MUStrCpy(tmpBuf,ptr,sizeof(tmpBuf));

        ptr = strstr(tmpBuf,"\n\n");

        if (ptr != NULL)
          *ptr = '\0';

        ptr = MUStrTok(tmpBuf,"\n",&TokPtr);

        while ((rc = sscanf(ptr,"%64s %64s %64s %64s %64s %64s %64s %64s %64s %64s %64s %64s %64s %64s %64s %64s %64s\n",
            tmpKey,
            tmpAttr[0],
            tmpAttr[1],
            tmpAttr[2],
            tmpAttr[3],
            tmpAttr[4],
            tmpAttr[5],
            tmpAttr[6],
            tmpAttr[7],
            tmpAttr[8],
            tmpAttr[9],
            tmpAttr[10],
            tmpAttr[11],
            tmpAttr[12],
            tmpAttr[13],
            tmpAttr[14],
            tmpAttr[15])) >= 1)
          {
          MUStrDup(&tptr[index2].Key,tmpKey);

          for (index3 = 0;index3 < MTBMAX_ATTR;index3++)
            { 
            if (index3 >= (rc - 1))  /* account for tmpKey */
              tptr[index2].Val[index3] = NULL;
            else
              MUStrDup(&tptr[index2].Val[index3],tmpAttr[index3]); 
            }

          ptr = MUStrTok(NULL,"\n",&TokPtr);

          if (ptr == NULL)
            {
            /* end of buffer reached */

            break;
            }

          index2++;

          if (index2 >= MTBMAX_ACCOUNT)
            break;
          }  /* END while ((rc = sscanf(ptr)) */
        }    /* END for (index) */

      /* count configured accounts */

      for (index = 0;index < MTBMAX_ATTR;index++)
        {
        if (MTAcct[index].Key == NULL)
          break;
        }
 
      MUFree(&buf);
      }      /* END else () */

    IsInitialized = TRUE;
    }  /* END if (IsInitialized == FALSE) */

  Buffer[0] = '\0';

  MJobMakeTemp(&J);

  RQ = J->Req[0];

  RQ->SetBlock = MBNOTSET;

  sprintf(J->Name,"%5d",
    JCounter++);

  J->SubmitTime = StartTime;
  J->StartTime  = StartTime;

  /* select account */

  ptr = MTAcctWeights[0].Val[rand() % MTBMAX_ATTR];

  if (ptr == NULL)
    {
    return(FAILURE);
    }

  for (aindex = 0; MTAcct[aindex].Key != NULL;aindex++)
    {
    if (!strcmp(MTAcct[aindex].Key,ptr))
      break;
    }

  if (MTAcct[aindex].Key == NULL)
    {
    return(FAILURE);
    }

  /* add creds */

  if (strcmp(MTAcct[aindex].Key,DEFAULT) && strcmp(MTAcct[aindex].Key,"DEFAULT"))
    {
    MAcctAdd(MTAcct[aindex].Key,&J->Credential.A);
    }

  /* to match up a user with their group */

  tmpRand = rand() % MTBMAX_ATTR; 

  if (MTUser[aindex].Val[tmpRand] == NULL)
    {
    return(FAILURE);
    }

  MUserAdd(MTUser[aindex].Val[tmpRand],&J->Credential.U);

  ptr = MTGroup[aindex].Val[tmpRand];

  if (ptr != NULL)
    MGroupAdd(ptr,&J->Credential.G);
  else
    MGroupAdd("DEFAULT",&J->Credential.G);

  MClassAdd(MTClass[aindex].Val[rand() % MTBMAX_ATTR],&J->Credential.C);

  ptr = MTQOS[aindex].Val[rand() % MTBMAX_ATTR];

  if ((ptr != NULL) && strcmp(ptr,DEFAULT) && strcmp(ptr,"DEFAULT"))
    {
    MQOSAdd(ptr,&J->QOSRequested);

    J->Credential.Q = J->QOSRequested;
    }

  /* set resource constraints */

  ptr = MTArch[aindex].Val[rand() % MTBMAX_ATTR];

  if (ptr != NULL)
    {
    RQ->Arch = MUMAGetIndex(meArch,ptr,mAdd);

    /* locate arch specific os */

    for (index = 0;MTArchToOS[index].Key != NULL;index++)
      {
      if (strcmp(MTArchToOS[index].Key,ptr))
        continue;

      ptr = MTOpsys[aindex].Val[rand() % MTBMAX_ATTR];

      if (ptr != NULL)
        {
        /* walk through the ArchToOS to see it the randomly picked OS matches */

        for (index2 = 0;MTArchToOS[index].Val[index2] != NULL;index2++)
          {
          if (!strcmp(MTArchToOS[index].Val[index2],ptr))
            {
            RQ->Opsys = MUMAGetIndex(meOpsys,ptr,mAdd);

            break;
            }
          }
        }

      break;
      }
    }
  else
    {
    /* locate general os constraint */

    ptr = MTOpsys[aindex].Val[rand() % MTBMAX_ATTR];

    if (ptr != NULL)
      RQ->Opsys = MUMAGetIndex(meOpsys,ptr,mAdd);
    }

  /* set resource quantity */

  J->Request.TC = (int)strtol(MTTC[aindex].Val[rand() % MTBMAX_ATTR],NULL,10);

  ptr = MTTPN[aindex].Val[rand() % MTBMAX_ATTR];

  if (ptr != NULL)
    {
    J->Req[0]->TasksPerNode = (int)strtol(ptr,NULL,10);
    J->Request.NC = J->Request.TC * J->Req[0]->TasksPerNode;
    }
  else
    {
    J->Req[0]->TasksPerNode = 0;
    }

  J->SpecWCLimit[0] = strtol(MTDuration[aindex].Val[rand() % MTBMAX_ATTR],NULL,10);
  J->WCLimit        = J->SpecWCLimit[0];
  J->SpecWCLimit[1] = J->SpecWCLimit[0];

  tmpRandL = labs(rand() * 10000);

  J->CompletionTime =  
    J->StartTime + 
    (tmpRandL % (int)(J->WCLimit * MDEF_COMPLETIONSCALAR));


  mstring_t JobString(MMAX_LINE);

  MStringSetF(&JobString,"%s %ld %-8s %-12s %-12s",
    "XX:XX:XX",
    0L,
    MXO[mxoJob],
    J->Name,
    MRecordEventType[mrelJobComplete]);

  MJobToTString(
      J,
      mrelJobComplete,
      NULL,
      &JobString);

  MUStrCpy(Buffer,JobString.c_str(),BufSize);

  MJobFreeTemp(&J);

  return(SUCCESS);
  }  /* END MTraceCreateJob() */

/* END MTrace.c */
