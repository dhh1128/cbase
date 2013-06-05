


/**
 * @file MSys.c
 *
 * This is where the main Moab loop "MSysMainLoop" resides.
 * 
 * <p>NOTE:  all routines should be ANSI, ISO 90, and ISO 99 compliant 
 */

using namespace std;

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "PluginManager.h"
#include "PluginNodeAllocate.h"


/* thread-safety Mutexes */
mmutex_t SaveNonBlockMDiagTMutex;
mmutex_t SaveNonBlockMDiagSMutex;
mmutex_t SaveNonBlockShowqMutex;


/* local prototpyes */

/* NOTE:  must sync with declaration */

int __MSysTestRLMerge();
int __MSysProcessSocket(msocket_t *);
int __MSysTestStart(void);

void *MSysUpdateHALockThread(void *);

int MServerUpdate(void);
int MSysStartUpdateHALockThread();
int MSysEnqueueSocket(char *);
int MSysStoreNonBlockingStrings(void);
int MSysQueryEventsFile(mulong,mulong,int *,enum MRecordEventTypeEnum *,enum MXMLOTypeEnum *,char A[][MMAX_LINE],mevent_itr_t *);

int MDBQueryEventsWithRetry(mdb_t *,mulong,mulong,int *,enum MRecordEventTypeEnum *,enum MXMLOTypeEnum *,char [][MMAX_LINE],mevent_itr_t *,char *);
int MDBEItrNext(mevent_itr_t *,mevent_t *);
int MDBEItrClear(mevent_itr_t *);
int MTestRMX();
int MSysBacktraceHandler(int,siginfo_t *,void *);

/* END local prototypes */

const struct mstatdata_t MStatData[] = {
  { mstaNONE,            mdfNONE },
  { mstaAvgQueueTime,    mdfNONE },    /* Job weighted average qtime, TotalQTS / JC */
  { mstaAvgBypass,       mdfNONE },    /* Job weighted average bypass count, Bypass / JC */
  { mstaAvgXFactor,      mdfNONE },    /* PS weighted average xfactor, PSXFactor / PSRun */
  { mstaTANodeCount,     mdfNONE },    /* total active node count */
  { mstaTAProcCount,     mdfNONE },    /* total active processor count */
  { mstaTCapacity,       mdfNONE },    /* nodes available of parent object (currently only set on class) */
  { mstaTJobCount,       mdfNONE },    /* total jobs finished during period,   mdfNONE }, |J| -> JC */
  { mstaTNJobCount,      mdfNONE },    /* node weighted finished job count, SUM(NC[J]) */
  { mstaTQueueTime,      mdfNONE },    /* total queuetime of finished jobs, SUM(QT[J]) */
  { mstaMQueueTime,      mdfNONE },    /* max queuetime of finished jobs,   MAX(QT[J]) */
  { mstaTReqWTime,       mdfNONE },    /* total requested walltime for finished jobs, SUM(WCLIMIT[j]) */
  { mstaTExeWTime,       mdfNONE },    /* total run walltime for finished jobs, SUM(CTIME[J] - STIME[J]) */
  { mstaTMSAvl,          mdfNONE },    /*  */
  { mstaTMSDed,          mdfNONE },    /*  */
  { mstaTMSUtl,          mdfNONE },    /*  */
  { mstaTDSAvl,          mdfNONE },    /* total disk seconds available */
  { mstaTDSDed,          mdfNONE },    /* total disk seconds dedicated */
  { mstaTDSUtl,          mdfNONE },    /* total disk seconds utilized */
  { mstaIDSUtl,          mdfNONE },    /* instant disk seconds utilized */
  { mstaTPSReq,          mdfNONE },    /* proc-seconds requested associated w/completed jobs (WallReqTime*PC) */
  { mstaTPSExe,          mdfNONE },    /* proc-seconds executed associated w/completed jobs (WallRunTime*PC) */
  { mstaTPSDed,          mdfNONE },    /* per iteration */
  { mstaTPSUtl,          mdfNONE },    /* per iteration */
  { mstaTJobAcc,         mdfNONE },
  { mstaTNJobAcc,        mdfNONE },
  { mstaTXF,             mdfNONE },
  { mstaTNXF,            mdfNONE },
  { mstaMXF,             mdfNONE },
  { mstaTBypass,         mdfNONE },
  { mstaMBypass,         mdfNONE },    /* maximum bypass count */
  { mstaTQOSAchieved,    mdfNONE },    /* total QOS achieved */
  { mstaStartTime,       mdfNONE },    /* time profile started */
  { mstaIStatCount,      mdfNONE },
  { mstaIStatDuration,   mdfNONE },
  { mstaGCEJobs,         mdfNONE },    /* current eligible jobs */
  { mstaGCIJobs,         mdfNONE },    /* current idle jobs */
  { mstaGCAJobs,         mdfNONE },    /* current active jobs */
  { mstaGPHAvl,          mdfNONE },    /* total processor hours */
  { mstaGPHUtl,          mdfNONE },    /* total processor hours used */
  { mstaGPHDed,          mdfNONE },    /* total processor hours dedicated */
  { mstaGPHSuc,          mdfNONE },    /* proc hours dedicated to jobs which have successfully completed */
  { mstaGMinEff,         mdfNONE },
  { mstaGMinEffIteration, mdfNONE },
  { mstaTPHPreempt,      mdfNONE },
  { mstaTJPreempt,       mdfNONE },
  { mstaTJEval,          mdfNONE },
  { mstaSchedDuration,   mdfNONE },
  { mstaSchedCount,      mdfNONE },
  { mstaTNC,             mdfNONE },
  { mstaTPC,             mdfNONE },
  { mstaTQueuedPH,       mdfNONE },
  { mstaTQueuedJC,       mdfNONE },
  { mstaTSubmitJC,       mdfNONE },    /* S->SubmitJC */
  { mstaTSubmitPH,       mdfNONE },    /* S->SubmitPH */
  { mstaTStartJC,        mdfNONE },    /* S->StartJC (field 50) */
  { mstaTStartPC,        mdfNONE },    /* S->StartPC */
  { mstaTStartQT,        mdfNONE },  /* S->StartQT */
  { mstaTStartXF,        mdfNONE },  /* S->StartXF */
  { mstaDuration,        mdfNONE },
  { mstaIterationCount,  mdfNONE }, /* number of iterations contributing to record */
  { mstaProfile,         mdfNONE },
  { mstaQOSCredits,      mdfNONE },
  { mstaTNL,             mdfNONE },
  { mstaTNM,             mdfNONE },
  { mstaXLoad,           mdfNONE },           /* see GMetric Overview */
  { mstaGMetric,         mdfNONE },          /* see GMetric Overview */
  { mstaLAST,            mdfNONE } };


int __MSysGetMutexLockFile(

  char *FileName,
  int   FileNameSize)

  {
  if (FileName == NULL)
    return(FAILURE);

  snprintf(FileName,FileNameSize,"%s.mutex",
      MSched.HALockFile);

  return(SUCCESS);
  }



/**
 * Scheduler initalization (including dynamic buffers such as MJob and MCJob)
 *
 * @param FullInit (I)
 */

int MSysInitialize(

  mbool_t FullInit)  /* I */

  {
  int       index;

  msched_t *S;

  time_t    tmpTime;

  const char *FName = "MSysInitialize";

  MDB(5,fALL) MLog("%s(%s)\n",
    FName,
    MBool[FullInit]);

  /* initialize all data structures */

  MSysSocketQueueCreate();        /* Create the Moab Socket queue */
  MSysJobSubmitQueueCreate();     /* Create the Job Submit queue */
  MObjectQueueInit();             /* Create the Moab Object queue */
  MWSQueueInit();                 /* Create the Web Services queue */

  M64Init(&M64);

  S = &MSched;

  memset(S,0,sizeof(MSched));

  time(&tmpTime);

  S->Time = (long)tmpTime;

  S->MaxCPU = MMAX_INT;

  MOSSyslogInit(S);

  MUBuildPList((mcfg_t *)MCfg,MParam);

  /* For arrays, point to the unused first element
   * because MOGetNextObject looks 1 past the current object */
  
  S->T[mxoAcct]  = &MAcctHT;
  S->Size[mxoAcct]  = 0;
  S->M[mxoAcct]  = 0;
  S->E[mxoAcct]  = &MAcctHT;
  S->AS[mxoAcct] = sizeof(mgcred_t) + sizeof(mpu_t);

  S->T[mxoAM]    = &MAM[0];
  S->Size[mxoAM]    = sizeof(mam_t);
  S->M[mxoAM]    = MMAX_AM;
  S->E[mxoAM]    = &MAM[MMAX_AM - 1];

  S->T[mxoClass] = &MClass[0];
  S->Size[mxoClass] = sizeof(mclass_t);
  S->M[mxoClass] = MMAX_CLASS;
  S->E[mxoClass] = &MClass[MMAX_CLASS];

  S->T[mxoxCP]   = &MCP;
  S->Size[mxoxCP]   = sizeof(mckpt_t);
  S->M[mxoxCP]   = 1;
  S->E[mxoxCP]   = &MCP;

  S->T[mxoGroup] = &MGroupHT;
  S->Size[mxoGroup] = 0;
  S->M[mxoGroup] = 0;
  S->E[mxoGroup] = &MGroupHT;
  S->AS[mxoGroup] = sizeof(mgcred_t) + sizeof(mpu_t);

  S->T[mxoTrig]  = &MTList;
  S->Size[mxoTrig]  = sizeof(mtrig_t);
  S->M[mxoTrig]  = 0;
  S->E[mxoTrig]  = &MTList;
  S->Max[mxoTrig] = 0;

  if (MSysAllocJobTable(MDEF_MAXJOB) == FAILURE)
    {
    exit(1);
    }

  if (MSysAllocCJobTable() == FAILURE)
    {
    exit(1);
    }

  MNode = NULL;

  if (MSysAllocNodeTable(MDEF_MAXNODE) == FAILURE)
    {
    exit(1);
    }

  S->Size[mxoNode]  = sizeof(mnode_t);

  S->T[mxoPar]   = &MPar[0];
  S->Size[mxoPar]   = sizeof(mpar_t);
  S->M[mxoPar]   = MMAX_PAR;
  S->E[mxoPar]   = &MPar[MMAX_PAR - 1];

  S->T[mxoQOS]   = &MQOS[0];
  S->Size[mxoQOS]   = sizeof(mqos_t);
  S->M[mxoQOS]   = MMAX_QOS;
  S->E[mxoQOS]   = &MQOS[MMAX_QOS - 1];

  S->T[mxoxRange]   = NULL;
  S->Size[mxoxRange]   = sizeof(mrange_t);
  S->M[mxoxRange]   = MMAX_RANGE;
  S->E[mxoxRange]   = NULL;
  S->Max[mxoxRange] = 0;

  S->T[mxoRM]    = &MRM[0];
  S->Size[mxoRM]    = sizeof(mrm_t);
  S->M[mxoRM]    = MMAX_RM;
  S->E[mxoRM]    = &MRM[MMAX_RM - 1];

  S->T[mxoSched] = &MSched;
  S->Size[mxoSched] = sizeof(msched_t);
  S->M[mxoSched] = 1;
  S->E[mxoSched] = &MSched;

  S->T[mxoUser]  = &MUserHT;
  S->Size[mxoUser]  = 0;   /* Switched from sizeof(mgcred_t *) to hashtable. */
  S->M[mxoUser]  = 0;   /* Will be incremented as users added. */
  S->E[mxoUser]  = &MUserHT;
  S->AS[mxoUser] = sizeof(mgcred_t) + sizeof(mpu_t);

  S->T[mxoRsv]  = &MRsvHT;
  S->Size[mxoRsv]  = 0;   /* Switched from sizeof(mrsv_t *) to hashtable. */
  S->M[mxoRsv]  = 0;      /* Will be incremented as reservations added. */
  S->E[mxoRsv]  = &MRsvHT;
  S->AS[mxoRsv] = sizeof(mrsv_t);

  S->M[mxoxDepends] = MDEF_MAXDEPENDS;
  S->M[mxoxTJob]    = MMAX_TJOB;

  S->M[mxoxGRes] = MDEF_MAXGRES;
  S->M[mxoxGMetric] = MDEF_MAXGMETRIC;

  memset(MNode,0,MNodeSize);

  MUArrayListCreate(&MSMPNodes,sizeof(msmpnode_t*),MDEF_MSMPNODECOUNT);

  MUArrayListCreate(&MSRsv,sizeof(msrsv_t),4);

  MUArrayListCreate(&MTJob,sizeof(mjob_t *),4);

  memset(MAList,0,sizeof(MAList));
  memset(MPar,0,MParSize);
  memset(MRClass,0,MRClassSize);
  memset(&MCP,0,sizeof(MCP));

  strcpy(MCP.SVersionList,MCKPT_SVERSIONLIST);
  strcpy(MCP.WVersion,MCKPT_VERSION);

  memset(MRM,0,MRMSize);

  MUArrayListCreate(&MVPCP,sizeof(mvpcp_t),4);
  MUArrayListCreate(&MVPC,sizeof(mpar_t),4);
  MUArrayListCreate(&MTList,sizeof(mtrig_t *),4);

  memset(&TrigPIDs,0,sizeof(mhash_t));

  if (FullInit == TRUE)
    {
    MRMModuleLoad();
    }

  MUIQ[0] = NULL;

  memset(MRange,0,MRangeSize);

  MFUCacheInitialize(&S->Time);

  /* initialize node cache */

  for (index = 0;index < (S->M[mxoNode] << 2) + MMAX_NHBUF;index++)
    {
    MNodeIndex[index] = -1;
    }

  /* initialize stat lookup table */

  for (index = 0;MStatData[index].AType != mstaLAST;index++)
    {
    MStatDataType[MStatData[index].AType] = MStatData[index].DType;
    }

  MSchedInitialize(S);

  MSimSetDefaults();

  MStatSetDefaults();

  memset(MNLProvBucket,0,MNLProvBucketSize);

  MNodeInitialize(S->GN,MDEF_GNNAME);

  strcpy(MAList[meJFeature][0],NONE);

  MUMAGetIndex(meJFeature,MJobFlags[mjfBackfill],mAdd);
  MUMAGetIndex(meJFeature,MJobFlags[mjfRestartable],mAdd);
  MUMAGetIndex(meJFeature,MJobFlags[mjfSuspendable],mAdd);
  MUMAGetIndex(meJFeature,MJobFlags[mjfPreemptee],mAdd);
  MUMAGetIndex(meJFeature,MJobFlags[mjfPreemptor],mAdd);
  MUMAGetIndex(meJFeature,MJobFlags[mjfInteractive],mAdd);
  MUMAGetIndex(meJFeature,MJobFlags[mjfFSViolation],mAdd);

  strcpy(MAList[meNFeature][0],NONE);
  strcpy(MAList[meOpsys][0],NONE);
  strcpy(MAList[meArch][0],NONE);

/* removed and added to MSysLoadConfig() to avoid race condition on MAXGRES 
  MParAdd(MCONST_GLOBALPARNAME,NULL);                
*/

  /* initialize rack array */

  for (index = 0;index < MMAX_RACK;index++)
    {
    MRack[index].Index = index;
    }  /* END for (index) */

  MLocalInitialize();

  S->ExtendedJobTracking = MDEF_EXTENDEDJOBTRACKING;

  S->StartUpRacks = (int *)MUCalloc(MMAX_RACK,sizeof(int));

  return(SUCCESS);
  }  /* END MSysInitialize() */



int MSysMemCheck()
 
  {
  const char *FName = "MSysMemCheck";

  MDB(3,fCORE) MLog("%s()\n",
    FName);
 
  MDB(2,fCORE) MLog("MNode[%4d]             %6.2f\n",
    MSched.M[mxoNode],
    (double)(MNodeSize) / MDEF_MBYTE);
 
  MDB(2,fCORE) MLog("MJob[%4d]              %6.2f\n",
    MSched.M[mxoJob],
    (double)(sizeof(mjob_t *) * MSched.M[mxoJob]) / MDEF_MBYTE);
 
  MDB(2,fCORE) MLog("MJobTraceBuffer[%4d]   %6.2f\n",
    MMAX_JOB_TRACE,
    (double)(sizeof(MJobTraceBuffer)) / MDEF_MBYTE);
 
  MDB(2,fCORE) MLog("MUser[%4d]             %6.2f\n",
    MSched.M[mxoUser],
    (double)((MSched.M[mxoUser] * sizeof(mgcred_t)) / MDEF_MBYTE));
 
  MDB(2,fCORE) MLog("MGroup[%4d]            %6.2f\n",
    MSched.M[mxoGroup],
    (double)((MSched.M[mxoGroup] * sizeof(mgcred_t)) / MDEF_MBYTE));
 
  MDB(2,fCORE) MLog("MAcct[%4d]             %6.2f\n",
    MSched.M[mxoAcct],
    (double)((MSched.M[mxoAcct] * sizeof(mgcred_t))) / MDEF_MBYTE);
 
  MDB(2,fCORE) MLog("MRsv[%4d]              %6.2f\n",
    MRsvHT.size(),
    (double)(MRsvHT.size() * sizeof(mrsv_t)) / MDEF_MBYTE);
 
  MDB(2,fCORE) MLog("MSRsv[%4d]             %6.2f\n",
    MSRsv.NumItems,
    (double)(MSRsv.NumItems * (MSRsv.ElementSize)) / MDEF_MBYTE);
 
  MDB(2,fCORE) MLog("MVPCP[%4d]             %6.2f\n",
    MVPCP.NumItems,
    (double)(MVPCP.NumItems * (MVPCP.ElementSize)) / MDEF_MBYTE);

  MDB(2,fCORE) MLog("MVPC[%4d]             %6.2f\n",
    MVPC.NumItems,
    (double)(MVPC.NumItems * (MVPC.ElementSize)) / MDEF_MBYTE);

  return(SUCCESS);
  }  /* END MSysMemCheck() */



/**
 * Register event with external event management system.
 *
 * @see MSysLaunchAction() - child
 * @see MSysRegExtEvent() - child - report event to external event manager
 *
 * @param Message (I)
 * @param AType (I)  action type
 * @param EFlags (I) [bitmap of enum mef*]
 * @param Prio (I)  event priority
 */

int MSysRegEvent(

  const char *Message,
  enum MSchedActionEnum AType,
  int         Prio)

  {
  char  tmpBuffer[MMAX_BUFFER];

  char *ASList[32];

  const char *FName = "MSysRegEvent";

  MDB(2,fCORE) MLog("%s(%s,%d,%d)\n",
    FName,
    (Message != NULL) ? Message : "NULL",
    AType,
    Prio);

  if (Prio > 0)
    {
    MUStrCpy(tmpBuffer,Message,sizeof(tmpBuffer));

    ASList[0] = NULL;

    MUStringChop(tmpBuffer);

    ASList[1] = tmpBuffer;
    ASList[2] = NULL;

    MSysLaunchAction(
      ASList,
      (AType != mactNONE) ? AType : mactAdminEvent);
    }

  return(SUCCESS);
  }  /* END MSysRegEvent() */





/**
 * @see MSysRegEvent() - parent
 *
 * @param ASList (I)
 * @param AType (I)
 */

int MSysLaunchAction(

  char                  **ASList,  /* I */
  enum MSchedActionEnum   AType)   /* I */

  {
  char Exec[MMAX_LINE];
  char Line[MMAX_BUFFER];

  int  pid;
  int  rc;

  const char *FName = "MSysLaunchAction";

  MDB(2,fCORE) MLog("%s(ASList,%s)\n",
    FName,
    MSched.Action[AType]);

  Line[0] = '\0';

  if (MSched.Action[AType][0] == '\0')
    {
    MDB(5,fCORE) MLog("INFO:     scheduler action %d disabled\n",
      AType);

    return(SUCCESS);
    }

  if (AType == mactMail)
    {
    MSysSendMail(
      MSysGetAdminMailList(1),
      NULL,
      "moab event",
      NULL,
      ASList[1]);

    return(SUCCESS);
    }

  if (MSched.Action[AType][0] == '/')
    {
    strcpy(Exec,MSched.Action[AType]);
    }
  else if (MSched.ToolsDir[strlen(MSched.ToolsDir) - 1] == '/')
    {
    sprintf(Exec,"%s%s",
      MSched.ToolsDir,
      MSched.Action[AType]);
    }
  else
    {
    sprintf(Exec,"%s/%s",
      MSched.ToolsDir,
      MSched.Action[AType]);
    }

  ASList[0] = Exec;

  if (ASList[1] == NULL)
    {
    sprintf(Line,"\"%s %s\"",
      MLogGetTime(),
      "NODATA");
  
    MDB(7,fCORE) MLog("INFO:     launching '%s' (AString: '%s')\n",
      Exec,
      Line);
    }
  else
    {
    MDB(7,fCORE) MLog("INFO:     launching '%s' (AString: '%s', ...)\n",
      Exec,
      ASList[1]);
    }

  /* fork process */

  if ((pid = fork()) == -1)
    {
    MDB(0,fCORE) MLog("ALERT:    cannot fork for action '%s', errno: %d (%s)\n",
      MSched.Action[AType],
      errno,
      strerror(errno));

    return(FAILURE);
    }

  if (pid == 0)
    {
    /* if child */

    if (ASList[1] == NULL)
      {
      rc = execl(Exec,Exec,Line,(char *)NULL);
      }
    else
      {
      rc = execv(Exec,ASList);
      }

    if (rc == -1)
      {
      /* child has failed */

      MDB(1,fCORE) MLog("ALERT:    cannot exec action '%s', errno: %d (%s)\n",
        Exec,
        errno,
        strerror(errno));
      }

    exit(0);
    }  /* END if (pid == 0) */

  MDB(2,fCORE) MLog("INFO:     action '%s' launched with message '%s'\n",
    MSched.Action[AType],
    (ASList[1] != NULL) ? ASList[1] : "NULL");

  return(SUCCESS);
  }  /* END MSysLaunchAction() */




/**
 * send general mail message
 *
 * @param ToList (I)
 * @param From (I) [optional]
 * @param Subject (I) [optional]
 * @param MailCmd (I) [optional]
 * @param Msg (I)
 */

int MSysSendMail(

  char       *ToList,
  char       *From,
  const char *Subject,
  char       *MailCmd,
  const char *Msg)

  {
  char tmpBuf[MMAX_BUFFER];
  char tmpCmd[MMAX_LINE];
  char tmpMCmd[MMAX_LINE];

  char *BPtr;
  int   BSpace;

  char *mptr;

  const char *FName = "MSysSendMail";

  MDB(4,fCORE) MLog("%s(%s,%s,%s,%s,%.64s)\n",
    FName,
    (ToList != NULL) ? ToList : "NULL",
    (From != NULL) ? From : "NULL",
    (Subject != NULL) ? Subject : "NULL",
    (MailCmd != NULL) ? MailCmd : "NULL",
    (Msg != NULL) ? Msg : "NULL");

  if ((ToList == NULL) || (ToList[0] == '\0') || (Msg == NULL))
    {
    return(FAILURE);
    }

  mptr = (MailCmd != NULL) ? MailCmd : MSched.Action[mactMail];

  if (!strcasecmp(mptr,"default"))
    {
    MUStrCpy(tmpMCmd,MDEF_MAILACTION,sizeof(tmpMCmd));

    mptr = tmpMCmd;
    }
  else if ((mptr == NULL) || (mptr[0] == '\0') || (!strcmp(mptr,"NONE")))
    {
    MDB(2,fCORE) MLog("ALERT:    mail command not specified\n");

    return(FAILURE);
    }

  /* create body */
 
  MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf)); 

  MUSNPrintF(&BPtr,&BSpace,"To: %s\n",
    ToList);

  MUSNPrintF(&BPtr,&BSpace,"From: %s\n",
    (From != NULL) ? From : MSCHED_MAILNAME);

  if ((Subject != NULL) && (Subject[0] != '\0'))
    {
    MUSNPrintF(&BPtr,&BSpace,"Subject: %s\n\n",
      Subject);
    }

  MUSNCat(&BPtr,&BSpace,Msg);

  snprintf(tmpCmd,sizeof(tmpCmd),"%s %s",
    mptr,
    ToList);

  /* issue mail command */

  if (MUReadPipe(tmpCmd,tmpBuf,NULL,0,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSysSendMail() */




/**
 *
 *
 * @param S (I)
 */

int MSysUpdateTime(

  msched_t *S)  /* I */

  {
  time_t          tmpT;

  struct tm      *Time;

  struct timeval  tvp;

  long            interval;

  const char *FName = "MSysUpdateTime";

  MDB(3,fALL) MLog("%s(S)\n",
    FName);

  /* update time, day, iteration, interval, and runtime */

  /* update time */

  if ((MSched.Mode != msmSim) || (MSched.Iteration != 0))
    {
    MUGetTime(&MSched.Time,mtmRefresh,S);
    }

  /* update day */

  tmpT = (time_t)MSched.Time;

  if ((Time = localtime(&tmpT)) == NULL)
    {
    char tmpLine[MMAX_LINE];

    MDB(2,fALL) MLog("ALERT:    localtime failed in %s (errno=%d)\n",
      FName,
      errno);

    snprintf(tmpLine,sizeof(tmpLine),"ALERT:    localtime failed in %s (errno=%d, '%s')\n",
      FName,
      errno,
      strerror(errno));

    MMBAdd(
      &MSched.MB,
      tmpLine,
      (char *)"N/A",
      mmbtOther,
      0,
      1,
      NULL);
    }
  else if (MSched.Day != Time->tm_yday)
    {
    char DString[MMAX_LINE];

    /* starting new day */

    MULToDString(&MSched.Time,DString);

    MDB(2,fALL) MLog("INFO:     starting new day: %s",
      DString);

    MSched.Day      = Time->tm_yday;
    MSched.DayTime  = Time->tm_hour;
    MSched.WeekDay  = Time->tm_wday;
    MSched.Month    = Time->tm_mon;
    MSched.MonthDay = Time->tm_mday - 1;
    }    /* END if (MSched.Day != Time->tm_yday) */
  else if (MSched.DayTime != Time->tm_hour)
    {
    MSched.DayTime = Time->tm_hour;
    }

  /* get exact time (update SchedTime, Interval) */

  gettimeofday(&tvp,NULL);

  /* determine time interval in 1/100's of a second */

  if ((MSched.Mode == msmSim) && (MSched.TimePolicy != mtpReal))
    {
    interval = MSched.MaxRMPollInterval * 100;
    }
  else if (MSched.SchedTime.tv_sec > 0)
    {
    interval = (tvp.tv_sec  - MSched.SchedTime.tv_sec) * 100 +
               (tvp.tv_usec - MSched.SchedTime.tv_usec) / 10000;
    }
  else
    {
    interval = 0;
    }

  if (interval < 0)
    {
    MDB(1,fSCHED) fprintf(stderr,"ALERT:    negative time interval detected (%ld)\n",
      interval);

    MSched.Interval = 0;
    }
  else if (MSched.SchedTime.tv_sec == 0)
    {
    /* first pass, time not yet initialized */

    MSched.Interval = 0;
    }
  else
    {
    /* update run time statistics */

    MSched.Interval = interval;

    MStat.SchedRunTime += MSched.Interval;
    }

  memcpy(&MSched.SchedTime,&tvp,sizeof(struct timeval));

  return(SUCCESS);
  }  /* END MSysUpdateTime() */



/**
 * Tell MSysProcessRequest() to automatically grow buffer next
 * time it is run. This function is usually called after Moab detects
 * a truncation in S->SBuffer.
 */

int MSysIncrGlobalReqBuf(void)
  {
  MIncrGlobalReqBuf = TRUE;

  return(SUCCESS);
  }  /* END MSysIncrGlobalReqBuf() */





/**
 * Determines if a user is authorized to run a specific command.
 *
 * In doing so it determines the user's admin level (ADMINCFG[level]) and 
 * then uses that level to check the level's privileges to the command.
 *
 * @param SAString (I)
 * @param CIndex (I) [optional]
 * @param SCIndex (I) [optional,0=notset]
 * @param AFlagBM (I) enum MRoleEnum bitmap
 */

int MSysGetAuth(

  const char    *SAString,
  enum MSvcEnum  CIndex,
  int            SCIndex,
  mbitmap_t     *AFlagBM)

  {
  char *name;
  char *data;
  char *ptr;

  char *TokPtr;

  int   index;

  mbitmap_t tmpL;

  char  AString[MMAX_NAME];

  int   aindex;

  enum MRoleEnum AList[] = { mcalNONE, mcalAdmin1, mcalAdmin2, mcalAdmin3, mcalAdmin4, mcalLAST };

  /* FORMAT:  <USERNAME>[:<PASSWORD>] */

  if (AFlagBM != NULL)
    bmclear(AFlagBM);

  MUStrCpy(AString,SAString,sizeof(AString));

  if ((name = MUStrTok(AString,":",&TokPtr)) == NULL)
    {
    /* no auth string available */

    return(FAILURE);
    }

  if ((data = MUStrTok(NULL,":",&TokPtr)) != NULL)
    {
    /* password string detected */

    /* NYI */
    }

  if (!strcasecmp(name,"PEER"))
    {
    name = data;
    }

  if (name == NULL)
    {
    return(FAILURE);
    }

  /* determine auth admin level */

  for (aindex = 1;AList[aindex] != mcalLAST;aindex++)
    {
    for (index = 0;MSched.Admin[aindex].UName[index][0] != '\0';index++)
      {
      ptr = MSched.Admin[aindex].UName[index];

      if (!strncasecmp(ptr,"PEER:",strlen("PEER:")))
        ptr += strlen("PEER:");

      if (!strcasecmp(ptr,name) ||
          !strcasecmp(ptr,"ALL"))
        {
        bmset(&tmpL,AList[aindex]);

        break;
        }
      }    /* END for (index) */
    }      /* END for (aindex) */

  /* all requests authorized at level 5 */

  bmset(&tmpL,mcalAdmin5);

  if (CIndex != mcsNONE)
    {
    /* determine granted vs owner vs none */

    for (index = 1;index <= MMAX_ADMINTYPE;index++)
      {
      /* AdminAccess 0 is empty */

      if (!bmisset(&tmpL,index))
        continue;

      if (MUI[CIndex].AdminAccess[index] == TRUE)
        {
        bmset(&tmpL,mcalGranted);
        }    /* END if (MUI[CIndex].AdminAccess[index] == TRUE) */

      if (MUI[CIndex].AdminAccess[index] == MBNOTSET)
        {
        bmset(&tmpL,mcalOwner);
        }
      }  /* END for (index) */
    }    /* END if (CIndex != mcsNONE) */

  if (AFlagBM != NULL)
    bmcopy(AFlagBM,&tmpL);

  return(SUCCESS);
  }  /* END MSysGetAuth() */





/**
 * Translate MClientCmdEnum to MSvcEnum
 *
 * @param CIndex (I)
 * @param SIndex (O)
 */

int MCCToMCS(

  enum MClientCmdEnum  CIndex,  /* I */
  enum MSvcEnum       *SIndex)  /* O */

  {
  if (SIndex == NULL)
    {
    return(FAILURE);
    }

  switch (CIndex)
    {
    case mccBal:

      *SIndex = mcsMBal;

      break;

    case mccNodeCtlThreadSafe:
    case mccJobCtlThreadSafe:
    case mccRsvDiagnoseThreadSafe:
    case mccTrigDiagnoseThreadSafe:
    case mccDiagnose:

      *SIndex = mcsMDiagnose;

      break;

    case mccJobCtl:

      *SIndex = mcsMJobCtl;

      break;

    case mccNodeCtl:

      *SIndex = mcsMNodeCtl;

      break;

    case mccVCCtlThreadSafe:
    case mccVCCtl:

      *SIndex = mcsMVCCtl;

      break;

    case mccVMCtlThreadSafe:
    case mccVMCtl:

      *SIndex = mcsMVMCtl;

      break;

    case mccRsvCtl:

      *SIndex = mcsMRsvCtl;

      break;

    case mccSchedCtl:

      *SIndex = mcsMSchedCtl;

      break;

    case mccShowThreadSafe:
    case mccShow:

      *SIndex = mcsMShow;

      break;

    case mccStat:

      *SIndex = mcsMStat;

      break;

    case mccJobSubmitThreadSafe:
    case mccSubmit:

      *SIndex = mcsMSubmit;

      break;

    case mccCredCtl:

      *SIndex = mcsCredCtl;

      break;

    case mccRMCtl:

      *SIndex = mcsRMCtl;

      break;

    case mccNONE:
    default:

      *SIndex = mcsNONE;

      break;
    }  /* END switch (CIndex) */

  return(SUCCESS);
  }  /* END MCCToMCS() */


/**
 * Dynamically modify in-memory/persistent scheduling parameters.
 *
 * NOTE:  handles 'changeparam'/'mschedctl -m' requests.
 *
 * @param PLine              (I) [full parameter line]
 * @param ChangeIsPersistent (I)
 * @param EvalOnly           (I)
 * @param ModifyMaster       (I)
 * @param LineNum            (I)
 * @param EMsg               (O) [optional,minsize=MMAX_LINE]
 */

int MSysReconfigure(

  char    *PLine,
  mbool_t  ChangeIsPersistent,
  mbool_t  EvalOnly,
  mbool_t  ModifyMaster,
  int      LineNum,
  char    *EMsg)

  {
  char *PName;
  char *PVal;

  char *ptr;

  enum MCfgParamEnum OrigCIndex;
  enum MCfgParamEnum CIndex;

  char  IName[MMAX_NAME];

  char  tmpPLine[MMAX_LINE];
  char  tEMsg[MMAX_LINE];
  char  PLineModified[MMAX_LINE];

  int   rc;

  mbool_t IsPrivate = FALSE;

  const char *FName = "MSysReconfigure";

  MDB(1,fCONFIG) MLog("%s(%s,%s,%s,%s,%d,EMsg)\n",
    FName,
    (PLine != NULL) ? PLine : "NULL",
    MBool[ChangeIsPersistent],
    MBool[EvalOnly],
    MBool[ModifyMaster],
    LineNum);

  if (EMsg == NULL)
    {
    EMsg = tEMsg;
    }
  EMsg[0] = '\0';

  if (PLine == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  PLine is single parameter specification */

  MUStrCpy(tmpPLine,PLine,sizeof(tmpPLine));

  mstring_t tmpLine(PLine);

  /* check to see if the parameter is invalid */

  if ((MUGetWord(tmpPLine,&PName,&PVal) == FAILURE) ||
      (MCfgGetIndex(mcoNONE,PName,FALSE,&OrigCIndex) == FAILURE))
    {
    MDB(2,fCONFIG) MLog("INFO:     invalid parameter '%s' cannot be processed\n",
      (PName != NULL) ? PName : "");

    if (EMsg != NULL)
      {
      sprintf(EMsg,"invalid parameter specified on line #%d",
        LineNum);
      }
    return(FAILURE);
    }

#ifndef MUSEWEBSERVICES
  {
  int   len;

  len = strlen("WEBSERVICESURL");
  if (!strncmp("WEBSERVICESURL",PName,len))
    {
    MDB(2,fCONFIG) MLog("ERROR:     Web Services is not enabled in this build of moab\n");

    if (EMsg != NULL)
      sprintf(EMsg,"Web Services is not enabled in this build of moab");
    else
      fprintf(stderr, "Web Services is not enabled in this build of moab\n");
    return(FAILURE);
    }
  }
#endif

  /* Check to see if the parameter has been disabled.  Note:  In EvalOnly mode, we won't
     check runtime parms here, because we'll let those get processed later */

  if ((EvalOnly == FALSE) && (!MParamCanChangeAtRuntime(MCfg[OrigCIndex].Name)))
    {
    char *pTokenTail = NULL;
    char *pToken = MUStrTok(PName," ",&pTokenTail);
    
    MDB(2,fCONFIG) MLog("INFO:     parameter '%s' cannot be changed while Moab is running.\n",
      (pToken != NULL) ? pToken : "");

    if (EMsg != NULL)
      {
      sprintf(EMsg,"parameter '%s' cannot be changed while Moab is running.",
        pToken);
      }
    return(FAILURE);
    }

  MUStringChop(PVal);

  if (MCfg[OrigCIndex].IsDeprecated == TRUE)
    {
    if (EMsg != NULL)
      {
      sprintf(EMsg,"parameter %s on line #%d is deprecated, use %s (see documentation)",
        MParam[MCfg[OrigCIndex].PIndex],
        LineNum,
        MParam[MCfg[OrigCIndex].NewPIndex]);
      }
    }    /* END if (MCfg[OrigCIndex].IsDeprecated == TRUE) */

  if ((MCfg[OrigCIndex].AttrList != NULL) && (EMsg != NULL))
    {
    mbool_t IsValid;

    IsValid = FALSE;

    if (MCfg[OrigCIndex].AttrList == (char **)MTrueString)
      {
      /* search true string and false string */

      if (MUGetIndexCI(PVal,MTrueString,FALSE,0) != 0)
        {
        IsValid = TRUE;
        }
      else if (MUGetIndexCI(PVal,MFalseString,FALSE,0) != 0)
        {
        IsValid = TRUE;
        }
      }

    if (IsValid == FALSE)
      {
      if (MUGetIndexCI(PVal,(const char **)MCfg[OrigCIndex].AttrList,FALSE,-1) != -1)
        {
        IsValid = TRUE; 
        }
      }

    if (IsValid == FALSE)
      {
      /* handle complicated strings */

      switch (MCfg[OrigCIndex].PIndex)
        {
        case mcoNodeAvailPolicy:

          {
          char  tmpLine1[MMAX_LINE];
          char *SArray[2];

          SArray[0] = PVal;
          SArray[1] = NULL;

          if (MParProcessNAvailPolicy(&MPar[0],SArray,TRUE,tmpLine1) == SUCCESS)
            {
            IsValid = TRUE;
            }
          }    /* END BLOCK */

        break;

        case mcoNodeAllocationPolicy:
          {
          enum MNodeAllocPolicyEnum policyType;
          char *allocationPolicy = NULL;
          char *policyValue = NULL;

          allocationPolicy = MUStrTok(PVal,":",&policyValue);

          policyType = (enum MNodeAllocPolicyEnum)MUGetIndexCI(allocationPolicy,MNAllocPolicy,FALSE,mnalNONE);
          if (policyType == mnalNONE)
            {
            if (EMsg != NULL)
              sprintf(EMsg,"invalid NODEALLOCATIONPOLICY");
            return(FAILURE);
            }

          /* validate plugin path */
          if (policyType == mnalPlugin)
            {
            try 
              {
              PluginManager::ValidateNodeAllocationPlugin("Eval Plugin",policyValue);
              }
            catch (exception& e) 
              {
              if (EMsg != NULL)
                sprintf(EMsg,"invalid plugin: %s", e.what());

              return(FAILURE);
              }
            }

          IsValid = TRUE;
          }

          break;

        case mcoFSPolicy:

          {
          /* Check if fspolicy is percentage based (% at end of policy)*/
          
          if ('%' == PVal[strlen(PVal) - 1])
            {
            char tmpPVal[MMAX_NAME];

            MUStrCpy(tmpPVal,PVal,strlen(PVal));  /* Copy all but % */

            if (MUGetIndexCI(tmpPVal,(const char **)MCfg[OrigCIndex].AttrList,FALSE,0) != 0)
              {
              IsValid = TRUE; 
              }
            }
          }

          break;

        case mcoJobRejectPolicy:

          IsValid = TRUE;

          break;

        default:

          /* NO-OP */

          break;
        }
      }    /* END if (IsValid == FALSE) */

    if (IsValid == FALSE)
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"parameter %s on line #%d may have invalid value '%s' (see documentation)",
          MParam[MCfg[OrigCIndex].PIndex],
          LineNum,
          PVal);
        }

      /* should we return failure at this point? */

      /* return(FAILURE); */
      }
    }    /* END if ((MCfg[OrigCIndex].AttrList != NULL) && (EMsg != NULL)) */

  CIndex = OrigCIndex;

  /* get index name */

  if (MCfgGetParameter(
        tmpLine.c_str(),
        MCfg[OrigCIndex].Name,
        &CIndex,
        IName,    /* O */
        NULL,
        -1,
        FALSE,
        NULL) == FAILURE)
    {
    MDB(2,fCONFIG) MLog("WARNING:  config line '%s' cannot be processed\n",
      PLine);

    if (EMsg != NULL)
      sprintf(EMsg,"specified parameter cannot be modified");

    return(FAILURE);
    }

  rc = SUCCESS;

  switch (MCfg[CIndex].PIndex)
    {
    case mcoAMCfg:
    case mcoIDCfg:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(MCfg[CIndex].OType,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      MAMLoadConfig(NULL,PLine);

      break;

    case mcoSysCfg:
    case mcoQOSCfg:
    case mcoUserCfg:
    case mcoGroupCfg:
    case mcoAcctCfg:
    case mcoClassCfg:
    
      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(MCfg[CIndex].OType,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      rc = MCredLoadConfig(MCfg[CIndex].OType,NULL,NULL,PLine,FALSE,EMsg);

      break;

    case mcoJobCfg:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(MCfg[CIndex].OType,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      rc = MTJobLoadConfig(NULL,PLine,EMsg);

      break;

    case mcoNodeCfg:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(mxoNode,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      {
      mnode_t *N;

      if (MNodeAdd(IName,&N) == FAILURE)
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot add node '%s'\n",
          IName);

        if (EMsg != NULL)
          sprintf(EMsg,"ERROR:  cannot add node");

        return(FAILURE);
        }

      rc = MNodeLoadConfig(N,PLine);
      }    /* END BLOCK */

      break;

    case mcoParCfg:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(mxoPar,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      {
      mpar_t *P;

      if (MParAdd(IName,&P) == FAILURE)
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot add partition '%s'\n",
          IName);

        if (EMsg != NULL)
          sprintf(EMsg,"ERROR:  cannot add partition");

        return(FAILURE);
        }

      rc = MParLoadConfig(P,PLine);
      }    /* END BLOCK */

      break;

    case mcoRMCfg:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(mxoRM,PLine,NULL,NULL,EMsg,MMAX_LINE);
        }
      else
        {
        rc = MRMLoadConfig(NULL,PLine,NULL);
        }

      break;

    case mcoSchedCfg:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(mxoSched,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      rc = MSchedLoadConfig(PLine,FALSE);

      break;

    case mcoSRCfg:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(mxoSRsv,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      rc = MSRLoadConfig(NULL,NULL,PLine);

      break;

    case mcoRsvProfile:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(mxoSRsv,PLine,"RSVPROFILE",NULL,EMsg,MMAX_LINE);

        break;
        }

      rc = MSRLoadConfig(NULL,MPARM_RSVPROFILE,PLine);

      break;

    case mcoClientCfg:

      IsPrivate = TRUE;

      if (EvalOnly == TRUE)
        {
        /* NO-OP */
        
        break;
        }
     
      /* process/modify private config file */

      rc = MOLoadPvtConfig(NULL,mxoNONE,NULL,NULL,NULL,PLine);

      /* NYI */

      break;

    case mcoVCProfile:

      if (EvalOnly == TRUE)
        {
        rc = MCfgEvalOConfig(mxoCluster,PLine,NULL,NULL,EMsg,MMAX_LINE);

        break;
        }

      rc = MClusterLoadConfig(NULL,NULL);

      break;

    case mcoVMCfg:

      rc = MVMLoadConfig(PLine);

      break;

    case mcoAdminCfg:

      rc = MAdminLoadConfig(PLine);

      break;

    default:

      /* change active parameter value */

      rc = MCfgProcessLine(CIndex,IName,NULL,PVal,EvalOnly,tEMsg);

      if ((rc == FAILURE) && (EMsg != NULL) && (EMsg[0] == '\0'))
        {
        MUStrCpy(EMsg,tEMsg,MMAX_LINE);
        }

      break;
    }  /* END switch (MCfg[CIndex].Index) */

  if (EMsg != NULL)
    {
    if (EMsg[0] == '\0')
      {
      if (rc == FAILURE)
        {
        strcpy(EMsg,"cannot modify parameter");
        }
      else 
        {
        if (EvalOnly != TRUE)
          {
          int len;

          len = strlen(PLine);

          /* remove terminating newline character */

          if (PLine[len - 1] == '\n')
            len--;

          snprintf(EMsg,MMAX_LINE,"parameter modified:  '%.*s'",
            len,
            PLine);
          }
        else
          {
          snprintf(EMsg,MMAX_LINE,"line #%d is valid:  '%s'",
            LineNum,
            PLine);
          }
        }
      }
    }      /* END if (EMsg != NULL) */

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  if (EvalOnly == TRUE)
    {
    return(SUCCESS);
    }

  MSched.EnvChanged = TRUE;

  if (ChangeIsPersistent == TRUE)
    {
    char  nchar;
    char *ntail;

    int   blen;
    int  newParamLen;

    char *tmpCfgBuf = NULL;

    char  CfgFName[MMAX_LINE]; 
    char  DatFName[MMAX_LINE];

    char *TokPtr;

    char *AName;

    char *btail;

    mbool_t ExtractParam;
    mbool_t BufModified;

    CfgFName[0] = '\0';

    sprintf(DatFName,"%s%s%s",
      (MSched.CfgDir[0] != '\0') ? MSched.CfgDir : "",
      (MSched.CfgDir[strlen(MSched.CfgDir) - 1] != '/') ? "/" : "",
      MDEF_DATFILE);

    MUStrCpy(CfgFName,MSched.ConfigFile,sizeof(CfgFName));

    if (IsPrivate == TRUE)
      strcpy(CfgFName + strlen(CfgFName) - strlen(".cfg"),"-private.cfg");

    /* re-load config buffer to reflect most recent user changes */

    if ((tmpCfgBuf = MFULoad(CfgFName,1,macmWrite,NULL,NULL,NULL)) == NULL)
      {
      MDB(2,fCONFIG) MLog("WARNING:  cannot load config file '%s'\n",
        CfgFName);

      if (IsPrivate == TRUE)
        {
        /* no private file exists, attempt to create new file */

        /*sprintf(tmpLine,"# moab-private.cfg\n\n");*/
        MStringSet(&tmpLine,"# moab-private.cfg\n\n");

        if (MFUCreate(CfgFName,NULL,tmpLine.c_str(),strlen(tmpLine.c_str()),-1,-1,-1,FALSE,NULL) == FAILURE)
          {
          /* cannot create dat file */

          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"cannot create private config file\n");
            }

          return(FAILURE);
          }

        MUStrDup(&MSched.PvtConfigBuffer,tmpLine.c_str());

        if ((tmpCfgBuf = MFULoad(CfgFName,1,macmWrite,NULL,NULL,NULL)) == NULL)
          {
          /* cannot locate private file and attempt to create new file failed */

          return(FAILURE);
          }
        }
      else
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot load config file '%s'",
            CfgFName);
          }

        return(FAILURE);
        }
      }  /* if ((tmpCfgBuf = MFULoad(CfgFName,1,macmWrite,NULL,NULL,NULL)) == NULL) */

    /* NOTE:  maintain raw copy for writing */

    if (IsPrivate == TRUE)
      {
      MUStrDup(&MSched.PvtConfigBuffer,tmpCfgBuf);

      MCfgAdjustBuffer(&MSched.PvtConfigBuffer,FALSE,&btail,FALSE,TRUE,FALSE);
      }
    else
      {
      MUStrDup(&MSched.ConfigBuffer,tmpCfgBuf);

      MCfgAdjustBuffer(&MSched.ConfigBuffer,FALSE,&btail,FALSE,TRUE,FALSE);
      }

    /* remove user-specified parameters */

    MStringSet(&tmpLine,PLine);

    /* extract attribute name */

    /* FORMAT:  <PARAM>[<INAME>] <ANAME>=<AVAL> */

    if (strchr(tmpLine.c_str(),'=') == NULL)
      {
      AName = NULL;
      }
    else
      {
      /* parse parameter name */

      MUStrTok(tmpLine.c_str()," \t\n",&TokPtr);

      /* parse attribute name */

      AName = MUStrTok(NULL," \t\n=",&TokPtr);
      }

    ExtractParam = ModifyMaster;

    BufModified = FALSE;

    while (ExtractParam == TRUE)
      {
      mbool_t tmpBufModified;

      if (MCfgExtractAttr(
            (IsPrivate == TRUE) ? MSched.PvtConfigBuffer : MSched.ConfigBuffer,
            &tmpCfgBuf,
            CIndex,  /* I */
            IName,   /* I */
            AName,   /* I */
            TRUE,
            &tmpBufModified) == FAILURE)
        {
        MUFree(&tmpCfgBuf);

        if (EMsg != NULL)
          {
          if (IName[0] != '\0')
            {
            snprintf(EMsg,MMAX_LINE,"cannot extract attribute '%s' from parameter '%s[%s]' in config database",
              AName,
              PName,
              IName);
            }
          else
            {
            snprintf(EMsg,MMAX_LINE,"cannot extract attribute '%s' from parameter '%s' in config database",
              AName,
              PName);
            }
          }

        return(FAILURE);
        }

      if (tmpBufModified == TRUE)
        BufModified = TRUE;

      if (AName != NULL)
        {
        /* parse next attribute value */

        MUStrTok(NULL," \t\n",&TokPtr);

        /* parse next attribute name */

        AName = MUStrTok(NULL," \t\n",&TokPtr);
        }

      if (AName == NULL)
        ExtractParam = FALSE;
      }  /* END while (ExtractParam == TRUE) */

    if (BufModified == TRUE)
      {
      if (MFUCreate(
            CfgFName,
            NULL,
            tmpCfgBuf,
            strlen(tmpCfgBuf),
            -1,
            -1,
            -1,
            FALSE,
            NULL) == FAILURE)
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot update config file '%s'", 
            CfgFName);
          }
        }    /* END if (MFUCreate() == FAILURE) */
      }      /* END if (BufModified == TRUE ) */

    MUFree(&tmpCfgBuf);

    /* set/overwrite parameters in dat config */

    /* locate parameters in dat config */

    if (IsPrivate == TRUE)
      {
      if ((tmpCfgBuf = MFULoad(CfgFName,1,macmWrite,NULL,NULL,NULL)) == NULL)
        {
        return(FAILURE);
        }

      MUStrDup(&MSched.PvtConfigBuffer,tmpCfgBuf);
      }
    else
      {
      if ((tmpCfgBuf = MFULoad(DatFName,1,macmWrite,NULL,NULL,NULL)) == NULL)
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot load config file '%s'\n",
          DatFName);

        MStringSet(&tmpLine,"# DAT File --- Do Not Modify --- parameters in moab.cfg will override parameters here\n# This file is automatically modified by moab\n");

        if (MFUCreate(DatFName,NULL,tmpLine.c_str(),strlen(tmpLine.c_str()),-1,-1,-1,FALSE,NULL) == FAILURE)
          {
          /* cannot create dat file */

          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"cannot create config database");
            }

          return(FAILURE);
          }

        MUStrDup(&MSched.DatBuffer,tmpLine.c_str());

        MUStrDup(&tmpCfgBuf,tmpLine.c_str());
        }
      else
        {
        /* refresh dat buffer */

        MUStrDup(&MSched.DatBuffer,tmpCfgBuf);
        }
      }   /* END else */

    /* remove dat parameters */

    MStringSet(&tmpLine,PLine);

    /* extract attribute name */

    /* FORMAT:  <PARAM>[<INAME>] <ANAME>=<AVAL> */

    if (strchr(tmpLine.c_str(),'=') == NULL)
      {
      AName = NULL;
      }
    else
      {
      /* parse parameter name */

      MUStrTok(tmpLine.c_str()," \t\n",&TokPtr);

      /* parse attribute name */

      AName = MUStrTok(NULL," \t\n=",&TokPtr);
      }

    ExtractParam = TRUE;

    BufModified = FALSE;

    while (ExtractParam == TRUE)
      {
      mbool_t tmpBufModified;

      /* Special Case: RESOURCELIMITPOLICY should be added, not replace the old one 
       * Still need a way to implement replacing the RESOURCELIMITPOLICY (NYI) */
      if (strstr(PLine,"RESOURCELIMITPOLICY") == PLine)
        {
        char  PVal[MMAX_LINE << 3];

        char LIName[MMAX_LINE];

        if (MCfgGetParameter(
              (IsPrivate == TRUE) ? MSched.PvtConfigBuffer : MSched.DatBuffer,
              MCfg[CIndex].Name,
              &CIndex,
              LIName,              /* O */
              PVal,                /* O */
              sizeof(PVal),
              FALSE,                
              NULL) == SUCCESS)
          {
          MUStrCpy(PLineModified,PLine,sizeof(PLineModified));
          MUStrCat(PLineModified," ",sizeof(PLineModified));
          MUStrCat(PLineModified,PVal,sizeof(PLineModified));
          
          PLine = PLineModified;
          } /* END if (MCfgGetParameter) */
        } /* END if (RESOURCELIMITPOLICY) special case */


      if (MCfgExtractAttr(
            (IsPrivate == TRUE) ? MSched.PvtConfigBuffer : MSched.DatBuffer,
            &tmpCfgBuf,
            CIndex,  /* I */
            IName,   /* I */
            AName,   /* I */
            FALSE,
            &tmpBufModified) == FAILURE)
        {
        MUFree(&tmpCfgBuf);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot extract attribute '%s' from parameter '%s' in config database",
            IName,
            AName);
          }

        return(FAILURE);
        }

      if (tmpBufModified == TRUE)
        BufModified = TRUE;

      if (AName != NULL)
        {
        /* parse next attribute value */

        MUStrTok(NULL," \t\n",&TokPtr);

        /* parse next attribute name */

        AName = MUStrTok(NULL," \t\n",&TokPtr);
        }

      if (AName == NULL)
        ExtractParam = FALSE;
      }  /* END while (ExtractParam == TRUE) */

    /* add new parameter to end of file */

    blen = strlen(tmpCfgBuf);

    /* NOTE:  Buffer -> <PLINE>\0[<NEXTPLINE>]... */

    /* record head of next string */

    if (CIndex != OrigCIndex)
      {
      /* dealing with deprecated param - use newer param when inserting */

      MStringSetF(&tmpLine,"%s %s",
        MCfg[CIndex].Name,
        PVal);
      }
    else
      {
      MStringSet(&tmpLine,PLine);
      }

    ntail = &tmpLine.c_str()[strlen(tmpLine.c_str()) - 1];
    nchar = *(ntail + 2);

    if (*ntail != '\n')
      {
      strncat(tmpLine.c_str(),"\n",sizeof(tmpLine.c_str()) - strlen(tmpLine.c_str()));
      }

    newParamLen = strlen(tmpLine.c_str()) + MMAX_NAME; /* Add MMAX_NAME for extra breathing room? */

    /* 
     * Extend our dynamically allocated buffer so we can append to it. 
     * It was originally created using MUStrDup and therefore has no extra room.
     */
    if ((tmpCfgBuf = (char *)realloc(tmpCfgBuf,newParamLen + blen)) == NULL)
      {
      blen = 0;
      }
    else
      {
      blen += newParamLen;
      }

    if (MUBufReplaceString(
          &tmpCfgBuf,      /* modified */
          &blen,           /* I/O */
          blen - newParamLen,            /* append to end of buffer */
          0,               /* append */
          tmpLine.c_str(),
          TRUE) == SUCCESS)
      {
      MCfgCompressBuf(tmpCfgBuf);

      if (MFUCreate(
           (IsPrivate == TRUE) ? CfgFName : DatFName,
           NULL,
           tmpCfgBuf,
           strlen(tmpCfgBuf),
           -1,
           -1,
           -1,
           FALSE,
           NULL) == FAILURE)
        {
        MDB(2,fCONFIG) MLog("ERROR:  cannot create/modify dat file '%s'\n",
          DatFName);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"changes implemented in memory only, error in creating/modifying file '%s'\n",
            (IsPrivate == TRUE) ? CfgFName : DatFName);
          }
        }     /* END if (MFUCreate() == FAILURE) */

      if ((ptr = MFULoad(
             (IsPrivate == TRUE) ? CfgFName : DatFName,
             1,
             macmWrite,
             NULL,
             NULL,
             NULL)) == NULL)
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot load file '%s'\n",
          DatFName);
        }

      /* use updated buffer as current dat buffer */

      if (IsPrivate == TRUE)
        {
        MUFree(&MSched.PvtConfigBuffer);

        MSched.PvtConfigBuffer = ptr;
        }
      else
        {
        MUFree(&MSched.DatBuffer);

        MSched.DatBuffer = ptr;
        }

      ptr = NULL;
      }  /* END if MBufReplaceString() == SUCCESS) */

    /* restore next string */

    *(ntail + 2) = nchar;

    MUFree(&tmpCfgBuf);
    }  /* END if (ChangeIsPersistent == TRUE) */

  MSched.ConfigSerialNumber++;

  return(SUCCESS);
  }  /* END MSysReconfigure() */


/**
 * Background process and disassociate from terminal, stdin, stdout, etc.
 *
 * @see MSysAuthenticate() - peer
 *
 * NOTE: Called by main() - run once at start-up before any config is loaded.
 */

int MSysDaemonize()

  {
  int     pid;

  struct rlimit rlim;

  const char *FName = "MSysDaemonize";

  FILE    *file;

  MDB(2,fALL) MLog("%s()\n",
    FName);

  /* create private process group */

#if defined(__OSF) || (defined(__FREEBSD) && !defined(__OSX5))
  if (setpgrp((pid_t)0,(pid_t)0) == -1)
#else /* __OSF */
  if (setpgrp() == -1)
#endif /* __OSF */
    {
    perror("cannot set process group");

    MDB(0,fALL) MLog("ERROR:    setpgrp() failed, errno: %d (%s)\n",
      errno,
      strerror(errno));
    }

  fflush(mlog.logfp);

  if ((MSched.DebugMode == TRUE) && (MSched.ForceBG != TRUE))
    {
    /* do not background in debug mode */
    /* command line overrides environment */

    return(SUCCESS);
    }

  switch (MSched.Mode)
    {
    case msmInteractive:

      /* NO-OP */

      if (MSched.ForceBG != TRUE)
        break;

    default:

      /* fork to disconnect from terminal */

      if (getrlimit(RLIMIT_CORE,&rlim) == 0)
        {
        if (rlim.rlim_cur != RLIM_INFINITY)
          {
          MDB(5,fALL) MLog("INFO:     system core limit set to %d (complete core files might not be generated)\n",
            (int)rlim.rlim_cur);
          }
        }

      if ((pid = fork()) == -1)
        {
        perror("cannot fork");

        MDB(0,fALL) MLog("ALERT:    cannot fork into background, errno: %d (%s)\n",
          errno,
          strerror(errno));
        }

      if (pid != 0)
        {
        /* exit if parent */

        MDB(3,fALL) MLog("INFO:     parent is exiting\n");

        fflush(mlog.logfp);

        exit(0);
        }

      /* restore core limit in child */

      setrlimit(RLIMIT_CORE,&rlim);

      /* NOTE:  setsid() disconnects from controlling-terminal */

      if (setsid() == -1)
        {
        MDB(3,fALL) MLog("INFO:     could not disconnect from controlling-terminal, errno=%d - %s\n",
          errno,
          strerror(errno));
        }
   
      /* disconnect stdin */

      fclose(stdin);
      file = fopen("/dev/null","r");

      /* disconnect stdout */

      fclose(stdout);
      file = fopen("/dev/null","w");

      /* disconnect stderr */

      fclose(stderr);
      file = fopen("/dev/null","w");

      if ((pid = fork()) == -1)
        {
        perror("cannot fork");

        MDB(0,fALL) MLog("ALERT:    cannot fork into background, errno: %d (%s)\n",
          errno,
          strerror(errno));
        }

      if (pid != 0)
        {
        /* exit if parent */
 
        MDB(3,fALL) MLog("INFO:     parent is exiting\n");

        fflush(mlog.logfp);

        exit(0);
        }

      MDB(3,fALL) MLog("INFO:     child process in background\n");
      MDB(9,fALL) MLog("INFO:     bogus statement to avoid compile warning (%d)\n",file);

      break;
    }  /* END switch (MSched.Mode) */

  MSched.PID = MOSGetPID();

  if (!bmisset(&MSched.Flags,mschedfFileLockHA) &&
      MFUAcquireLock(MSched.LockFile,&MSched.LockFD,"pid") == FAILURE)
    {
    /* Failed to acquire the pid lock file */

    fprintf(stderr,"ERROR:  cannot lock the PID file '%s' - moab already running?\n",
      MSched.LockFile);

    MDB(1,fALL) MLog("cannot lock the PID file - moab already running?\nexiting\n");

    if (!bmisset(&MSched.Flags,mschedfIgnorePIDFileLock))
      exit(1);
    }

  if (MSched.LockFD > 0)
    {
    char tmpPID[MMAX_NAME];

    /* we've already opened the lock file */
    /* write server pid into lockfile */

    snprintf(tmpPID,sizeof(tmpPID),"%d",MSched.PID);

    if (ftruncate(MSched.LockFD,0) != 0)
      {
      MDB(0,fCKPT) MLog("ALERT:  error with ftruncate of lockfile - errno: %d (%s)\n",
          errno,
          strerror(errno));
      }

    lseek(MSched.LockFD,0,SEEK_SET);

    if (write(MSched.LockFD,tmpPID,strlen(tmpPID)) < 0)
      {
      MDB(0,fCKPT) MLog("ALERT:  error with write to lockfile - errno: %d (%s)\n",
          errno,
          strerror(errno));
      }

    /* WARNING: DO NOT CLOSE LockFD as it will relinquish
     * the PID lock! */
    }

  return(SUCCESS);
  }  /* END MSysDaemonize() */





/**
 * Initialize DB based on MSched.ReqMDBType. Does nothing and returns success if
 * MSched.ReqMDBType is mdbNone;
 *
 * @param MyDB  (I);
 *
 */

int MSysInitDB(

  mdb_t *MyDB)

  {
  char tmpEMsg[MMAX_LINE];

  if (MSched.ReqMDBType == mdbNONE)
    return(SUCCESS);

  /* after extensive tests it seems that unixodbc is not that threadsafe in these calls
     we are locking them to avoid crashes/aborts. Do NOT put any "returns" in the if
     clause below */

  MUMutexLock(&MDBHandleHashLock);
  MDBInitialize(MyDB,MSched.ReqMDBType);
  MUHTAddInt(&MDBHandles,MUGetThreadID(),MyDB,NULL,(mfree_t)MDBFreeDPtr);

  if ((MDBConnect(MyDB,tmpEMsg) == FAILURE))
    {
    MyDB->DBType = mdbNONE;

    MDBFree(MyDB);

#if defined(MYAHOO) && !defined(MDISABLEODBC)
    fprintf(stderr,"ERROR:  cannot connect to DB--please check ODBC driver and DB engine (%s)!\n",
      tmpEMsg);

    exit(-1);  /* shutdown if we cannot connect to the DB */
#else
    /* If we are under Strict Configuration Check mode, and
     * we had a DB failure, so we bail now without any DB 
     * fallback operation
     */
    if (TRUE == MSched.StrictConfigCheck)
      {
      MDB(0,fCONFIG) MLog("ERROR:  StrictConfigCheck ON and cannot connect to DB--please check DB engine and configuration (%s)!\n",
        tmpEMsg);
      fprintf(stderr,"ERROR:  StrictConfigCheck ON and cannot connect to DB--please check DB engine and configuration (%s)!\n",
        tmpEMsg);
      fprintf(stderr,"moab will now exit with error\n");

      exit(-1);  /* shutdown if we cannot connect to the DB */
      }
#endif /* MYAHOO && MODBC */
    }  /* END if ((MDB->DBType != mdbNONE) && ...) */

  MUMutexUnlock(&MDBHandleHashLock);

  return (SUCCESS);
  } /* END MSysInitDB() */



/**
 * Initialize system-level logging.
 *
 * @param ArgC (I)
 * @param ArgV (I)
 * @param BaseName (I)
 */

int MSysLogInitialize(

  int          ArgC,
  const char **ArgV,
  const char  *BaseName)

  {
  char LogFile[MMAX_LINE];  /* log file name */
  char LogPath[MMAX_LINE];  /* log dir */
  char tmpLine[MMAX_LINE];

  /* set logging defaults */

  mlog.logfp        = stderr;
  mlog.Threshold    = MDEF_LOGLEVEL;

  bmset(&mlog.FacilityBM,MDEF_LOGFACILITY);

  LogPath[0] = '\0';

  /* handle command line based logging specification */

  sprintf(LogFile,"%s%s",
    BaseName,
    MDEF_LOGSUFFIX);

  if (ArgC > 1)
    {
    int aindex = 1;

    if ((ArgV[aindex][0] == '-') &&
        (ArgV[aindex][1] == 'L'))
      {
      /* process '-L <LOGLEVEL>' */

      if (ArgV[aindex][2] != '\0')
        {
        mlog.Threshold = (int)strtol(&ArgV[aindex][2],NULL,10);

        aindex++;
        }
      else if (ArgV[aindex + 1])
        {
        mlog.Threshold = (int)strtol(ArgV[aindex + 1],NULL,10);

        aindex += 2;
        }
      else
        {
        /* we are missing the "required" argument--advance the index... */
        aindex++;
        }

      MDB(3,fCONFIG) MLog("INFO:     initial LOGLEVEL set to %d\n",
        mlog.Threshold);
      }

    if ((ArgC > aindex) &&
        (ArgV[aindex] != NULL) &&
        (ArgV[aindex][0] == '-') &&
        (ArgV[aindex][1] == 'l'))
      {
      /* process '-l <LOGFILE>' */

      if (ArgV[aindex][2] != '\0')
        {
        MUStrCpy(LogFile,&ArgV[aindex][2],sizeof(LogFile));
        }
      else if (ArgV[aindex + 1])
        {
        MUStrCpy(LogFile,ArgV[aindex + 1],sizeof(LogFile));
        }

      MDB(3,fCONFIG) MLog("INFO:     initial LOGFILE set to %s\n",
        LogFile);
      }
    }

  /* logfile could be absolute or relative */

  if ((LogFile[0] != '/') && (LogFile[0] != '~'))
    {
    /* log file is relative */
 
    char *ptr;

    MUStrCpy(LogPath,MDEF_LOGDIR,sizeof(LogPath));

    if ((LogPath[0] != '/') && (LogPath[0] != '~'))
      {
      /* log path is relative - adjust by homedir */

      if ((ptr = getenv(MSCHED_ENVHOMEVAR)) != NULL)
        {
        MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

        snprintf(LogPath,sizeof(LogPath),"%s/%s",
          tmpLine,
          MDEF_LOGDIR);

        if (LogPath[strlen(LogPath) - 1] != '/')
          strcat(LogPath,"/");

        MUStrCat(LogPath,LogFile,sizeof(LogPath));
        }  /* END if ((ptr = getenv(MSCHED_ENVHOMEVAR)) != NULL) */
      else
        {
#ifdef MBUILD_VARDIR
        struct stat buf;

        sprintf(LogPath,"%s/log",
          MBUILD_VARDIR);

        if (stat(LogPath,&buf) != -1)
          {
          snprintf(LogPath,sizeof(LogPath),"%s/log/%s",
            MBUILD_VARDIR,
            LogFile);
          }
        else
          {
          snprintf(LogPath,sizeof(LogPath),"%s/%s",
            MBUILD_VARDIR,
            LogFile);
          }
#else /* MBUILD_VARDIR */
        MUStrCpy(LogPath,LogFile,sizeof(LogPath));
#endif /* MBUILD_VARDIR */
        }  /* END else ((ptr = getenv(MSCHED_ENVHOMEVAR)) != NULL) */
      }    /* END if ((LogPath[0] != '/') && (LogPath[0] != '~')) */
    else
      {
      /* LogPath is absolute, LogFile is relative */

      if (LogPath[strlen(LogPath) - 1] != '/')
        strcat(LogPath,"/");

      MUStrCat(LogPath,LogFile,sizeof(LogPath));
      }
    }  /* END if ((LogFile[0] != '/') && (LogFile[0] != '~')) */
  else
    {
    /* logfile is absolute */

    MUStrCpy(LogPath,LogFile,sizeof(LogPath));
    }

  /* LogPath is now absolute pathname */

  MLogInitialize(
    LogPath,             /* full path name of log file */
    MDEF_LOGFILEMAXSIZE,
    MSched.Iteration);

  return(SUCCESS);
  }  /* END MSysLogInitialize() */




/**
 * Report system config parameters.
 *
 * @param String (O)
 * @param PIndex (I)
 * @param VFlag (I)
 */

int MSysConfigShow(

  mstring_t *String,
  int        PIndex,
  int        VFlag)

  {
  char TString[MMAX_LINE];

  const char *FName = "MSysConfigShow";

  MDB(1,fCONFIG) MLog("%s(String,%d,%d,%d)\n",
    FName,
    PIndex,
    VFlag);

  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  /* display modules */

  MStringAppend(String,"# SERVER MODULES: ");

#ifdef __MMD5
  MStringAppend(String," MD5");
#endif /* __MMD5 */

  MStringAppend(String,"\n");

  /* display parameters */

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoSchedMode],
    MSchedMode[MSched.Mode]);

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoSchedPolicy],
    MSchedPolicy[MSched.SchedPolicy]);

  if ((VFlag || (PIndex == -1) || (PIndex == mcoServerName)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoServerName],
      MSched.Name);
    }

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoServerHost],
    MSched.ServerHost);

  MStringAppendF(String,"%-30s  %d\n",
    MParam[mcoServerPort],
    MSched.ServerPort);

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoMServerHomeDir],
    MSched.CfgDir);

  if (VFlag || (MSched.ToolsDir[0] != '\0'))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoSchedToolsDir],
      MSched.ToolsDir);
    }

  if (VFlag || (MSched.LogDir[0] != '\0'))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoLogDir],
      MSched.LogDir);
    }

  if (VFlag || (MStat.StatDir[0] != '\0'))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoStatDir],MStat.StatDir);
    }

  if (VFlag || (MSched.LockFile[0] != '\0'))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoSchedLockFile],MSched.LockFile);
    }

  if (VFlag || (MSched.ConfigFile[0] != '\0'))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoSchedConfigFile],MSched.ConfigFile);
    }

  if ((MSched.Action[mactAdminEvent][0] != '\0') || VFlag)
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoAdminEAction],MSched.Action[mactAdminEvent]);
    }

  if ((MSched.Action[mactJobFB][0] != '\0') || VFlag)
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoJobFBAction],MSched.Action[mactJobFB]);
    }

  if ((MSched.Action[mactMail][0] != '\0') || VFlag)
    {
    MStringAppendF(String,"%-30s  %s\n",MParam[mcoMailAction],MSched.Action[mactMail]);
    }

  MStringAppendF(String,"%-30s  %s\n",MParam[mcoCheckPointFile],MCP.CPFileName);
  MULToTString(MCP.CPInterval,TString);
  MStringAppendF(String,"%-30s  %s\n",MParam[mcoCheckPointInterval],TString);
  MULToTString(MCP.CPExpirationTime,TString);
  MStringAppendF(String,"%-30s  %s\n",MParam[mcoCheckPointExpirationTime],TString);

  if ((MSched.MaxRsvPerNode != MDEF_RSV_DEPTH) || (VFlag || (PIndex == -1) || (PIndex == mcoMaxRsvPerNode)))
    {
    MStringAppendF(String,"%-30s  %d\n",
      MParam[mcoMaxRsvPerNode],
      MSched.MaxRsvPerNode);
    }

  if ((MSched.VMGResMap == TRUE) || (VFlag || (PIndex == -1) || (PIndex == mcoVMGResMap)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoVMGResMap],
      MBool[MSched.VMGResMap]);
    }

  if ((MSched.ForceNodeReprovision == TRUE) || (VFlag || (PIndex == -1) || (PIndex == mcoForceNodeReprovision)))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoForceNodeReprovision],
      MBool[MSched.ForceNodeReprovision]);
    }

  if ((VFlag || (PIndex == -1) || (PIndex == mcoVMMinOpDelay)))
    {
    MStringAppendF(String,"%-30s  %ld\n",
      MParam[mcoVMMinOpDelay],
      MSched.MinVMOpDelay);
    }

  if ((VFlag || (PIndex == -1) || (PIndex == mcoVMMigrationPolicy)))
    {
    MStringAppendF(String,"%-30s ",
      MParam[mcoVMMigrationPolicy]);

    if (MSched.RunVMOvercommitMigration == TRUE)
      {
      MStringAppendF(String," %s",
        MVMMigrationPolicy[mvmmpOvercommit]);
      }

    if (MSched.RunVMConsolidationMigration == TRUE)
      {
      MStringAppendF(String," %s",
        MVMMigrationPolicy[mvmmpConsolidation]);
      }

    MStringAppendF(String,"\n");
    }  /* END if (PIndex == mcoVMMigrationPolicy) */

  MStringAppend(String,"\n");

  return(SUCCESS);
  }  /* END MSysConfigShow() */





/**
 * Sets the scheduler process' uid and gid to the first user in admincfg[1].
 *
 * @param UID (I)
 * @param GID (I)
 */

int MSysSetCreds(

  int UID,  /* I */
  int GID)  /* I */

  {
  char *logName = NULL;

  if ((MSched.LockFD > 0) &&
      (fchown(MSched.LockFD,UID,GID) == -1))
    {
    MDB(0,fSTRUCT) MLog("ERROR:    could not change ownership of lock file to uid:%d gid:%d (errno: %d:%s)\n",
      UID,
      GID,
      errno,
      strerror(errno));
    
    exit(1);
    }

  if (MSched.HALockFD > 0)
    {
    char MutexLockFile[MMAX_NAME];

    if (fchown(MSched.HALockFD,UID,GID) == -1)
      {
      MDB(0,fSTRUCT) MLog("ERROR:    could not change ownership of HA lock file to uid:%d gid:%d (errno: %d:%s)\n",
        UID,
        GID,
        errno,
        strerror(errno));
      
      exit(1);
      }

    __MSysGetMutexLockFile(MutexLockFile,sizeof(MutexLockFile));

    if (chown(MutexLockFile,UID,GID) == -1)
      {
      MDB(0,fSTRUCT) MLog("ERROR:    could not change ownership of HA mutex lock file to uid:%d gid:%d (errno: %d:%s)\n",
        UID,
        GID,
        errno,
        strerror(errno));
      }
    }

  /* change log file ownership to new uid so that on restart it won't complain about permissions. */
  MLogGetName(&logName);
  if ((logName != NULL) &&
      (chown(logName,UID,GID) == -1))
    {
    MDB(0,fSTRUCT) MLog("ALERT:    could not change ownership of log file to uid:%d gid:%d (errno: %d:%s)\n",
      UID,
      GID,
      errno,
      strerror(errno));
    }

  if (logName != NULL)
    MUFree(&logName);

  /* must set group before changing UID */

  if (MOSSetGID(GID) == -1)
    {
    char tmpGName[MMAX_NAME];

    fprintf(stderr,"ERROR:    cannot change GID to group '%s'  (GID: %d) %s\n",
      MUGIDToName(GID,tmpGName),
      GID,
      strerror(errno));

    /* non-catastrophic failure */
    }
  else
    {
    MSched.GID = GID;
    }

  if (MOSSetUID(UID) == -1)
    {
    char tmpUName[MMAX_NAME];

    fprintf(stderr,"ERROR:    cannot change UID to user '%s'  (UID : %d) %s\n",
      MUUIDToName(UID,tmpUName),
      UID,
      strerror(errno));

    exit(1);
    }
  else
    {
    MSched.UID = UID;
    }

  return(SUCCESS);
  }  /* END MSysSetCreds() */





/**
 * Process server command line arguments.
 *
 * @param ArgC (I)
 * @param ArgV (I)
 * @param PreLoad (I)
 */

int MSysProcessArgs(

  int       ArgC,     /* I */
  char    **ArgV,     /* I */
  mbool_t   PreLoad)  /* I */

  {
  char  Flag;
  int   index;

  int   tmpArgC;

  char  tmp[MMAX_LINE];

  char *OptArg;
  int   OptTok = 1;

  char  AString[MMAX_LINE];

  const char *FName = "MSysProcessArgs";

  MDB(1,fCONFIG) MLog("%s(%d,ArgV,%s)\n",
    FName,
    ArgC,
    MBool[PreLoad]);

  if (PreLoad == TRUE)
    {
    /* preserve all args */

    for (index = 0;index < MMAX_ARG;index++)
      {
      if (ArgV[index] == NULL)
        break;

      MUStrDup(&MSched.ArgV[index],ArgV[index]);
      }  /* END for (index) */

    MSched.ArgV[index] = NULL;
    MSched.ArgC        = index;

    strcpy(AString,"bc:dehH:sv?-:");
    }
  else
    {
    strcpy(AString,"a:bB:dD:ehH:i:l:L:m:n:N:p:P:R:sS:v?-:");
    }

  if ((ArgV[1] != NULL) && !strcmp(ArgV[1],"-?"))
    {
    /* handle explicit '?' request - preload request cannot be distinguished from
       bad arg */

    MSysShowUsage(ArgV[0]);

    exit(0);
    }

  /* NOTE:  '--' args should be processed in each pass (NYI) */

  tmpArgC = ArgC;

  while ((Flag = MUGetOpt(&tmpArgC,ArgV,AString,&OptArg,&OptTok,NULL)) != (char)-1)
    {
    MDB(3,fCONFIG) MLog("INFO:     processing arg '%c' (value='%s')\n",
      Flag,
      (OptArg != NULL) ? OptArg : "");

    switch (Flag)
      {
      case '-':

        {
        char *ptr;

        char *TokPtr = NULL;
        char *Value  = NULL;

        if (OptArg == NULL)
          {
          break;
          }

        if ((ptr = MUStrTok(OptArg,"=",&TokPtr)) != NULL)
          {
          Value = MUStrTok(NULL,"=",&TokPtr);
          }

        if (!strcmp(OptArg,"about"))
          {
          mccfg_t C;

          char *hptr;

          memset(&C,0,sizeof(C));

          strcpy(C.ServerHost,MDEF_SERVERHOST);
          C.ServerPort = MDEF_SERVERPORT;

          hptr = getenv(MSCHED_ENVHOMEVAR);

          #ifdef MBUILD_ETCDIR
            strcpy(C.CfgDir,MBUILD_ETCDIR);
          #else
            strcpy(C.CfgDir,"/opt/moab");
          #endif 

          sprintf(C.ConfigFile,"%s%s%s",
            C.CfgDir,
            MSCHED_SNAME,
            MDEF_CONFIGSUFFIX);

#if defined(MBUILD_VARDIR)
          strcpy(C.VarDir,MBUILD_VARDIR);
#else 
          strcpy(C.VarDir,C.CfgDir);
#endif /* MBUID_VARDIR */

#if defined(MBUILD_DIR)
          strncpy(C.BuildDir,MBUILD_DIR,sizeof(C.BuildDir));
#else
          strcpy(C.BuildDir,"NA");
#endif /* MBUILD_DIR */

#if defined(MBUILD_HOST)
          strncpy(C.BuildHost,MBUILD_HOST,sizeof(C.BuildHost));
#else
          strcpy(C.BuildHost,"NA");
#endif /* MBUILD_HOST */

#if defined(MBUILD_DATE)
          strncpy(C.BuildDate,MBUILD_DATE,sizeof(C.BuildDate));
#else
          strcpy(C.BuildDate,"NA");
#endif /* MBUILD_DATE */

#if defined(MBUILD_ARGS)
          {
          char *ptr;

          strncpy(C.BuildArgs,MBUILD_ARGS,sizeof(C.BuildArgs));

          /* hide sensitive key info */

          ptr = strstr(C.BuildArgs,"--with-key=");

          if (ptr != NULL)
            {
            ptr += strlen("--with-key=");

            while (isalnum(*ptr))
              {
              *ptr = '?';
              ptr++;
              }
            }  /* END if (ptr != NULL) */
          }
#else
          strcpy(C.BuildArgs,"NA");
#endif /* MBUILD_DATE */

          /* NOTE:  specify:  default configfile, serverhost, serverport */

          fprintf(stderr,"Defaults:   server=%s:%d  cfgdir=%s%s  vardir=%s\n",
            C.ServerHost,
            C.ServerPort,
            (hptr != NULL) ? hptr : C.CfgDir,
            (hptr != NULL) ? " (env)" : "",
            C.VarDir);

          /* NOTE:  specify build location, build date, build host       */

          fprintf(stderr,"Build dir:  %s\nBuild host: %s\nBuild date: %s\n",
            C.BuildDir,
            C.BuildHost,
            C.BuildDate);

          fprintf(stderr,"Build args: %s\n",
            C.BuildArgs);

          MSysShowEndianness();

          fprintf(stderr,"Version: %s server %s (revision %s, changeset %s)\n",
            MSCHED_SNAME,
            MOAB_VERSION,
#ifdef MOAB_REVISION
            MOAB_REVISION,
#else /* MOAB_REVISION */
            "NA",
#endif /* MOAB_REVISION */
#ifdef MOAB_CHANGESET
            MOAB_CHANGESET);
#else /* MOAB_CHANGESET */
            "NA");
#endif /* MOAB_CHANGESET */


          MSysShowCopy();

          if (MSched.ServerHost[0] == '\0')
            MOSGetHostName(NULL,MSched.ServerHost,NULL,FALSE);

          MLicenseGetLicense(FALSE,NULL,0);

          exit(0);
          }
        else if ((!strcmp(OptArg,"configfile")) && (Value != NULL))
          {
          MUStrCpy(MSched.ConfigFile,Value,sizeof(MSched.ConfigFile));

          MDB(0,fCONFIG) MLog("INFO:     server configfile set to %s\n",
            MSched.ConfigFile);
          }
        else if (!strcmp(OptArg,"help"))
          {
          MSysShowUsage(ArgV[0]);

          exit(0);
          }
        else if (!strcmp(OptArg,"keyfile"))
          {
          FILE *fp;
          char  tmpS[MMAX_LINE];
          int   rc;

          if (Value == NULL)
            {
            MSysShowUsage(ArgV[0]);

            exit(1);
            }

          if ((fp = fopen(Value,"r")) == NULL)
            {
            fprintf(stderr,"ERROR:    cannot open keyfile '%s' errno: %d, (%s)\n",
              Value,
              errno,
              strerror(errno));

            exit(1);
            }

          rc = fscanf(fp,"%64s", tmpS);

          MUStrCpy(MSched.DefaultCSKey,tmpS,sizeof(MSched.DefaultCSKey));

          fclose(fp);

          MDB(9,fALL) MLog("INFO:     scanned %d\n",rc);
          }
        else if ((!strcmp(OptArg,"loglevel")) && (Value != NULL))
          {
          mlog.Threshold = (int)strtol(Value,NULL,0);

          MDB(3,fCONFIG) MLog("INFO:     log level set to %d\n",
            mlog.Threshold);

          break;
          }
        else if ((!strcmp(OptArg,"host")) && (Value != NULL))
          {
          MUStrCpy(MSched.ServerHost,Value,sizeof(MSched.ServerHost));

          MDB(3,fCONFIG) MLog("INFO:     server host set to %s\n",
            MSched.ServerHost);
          }
        else if (!strcmp(OptArg,"noeval"))
          {
          MSched.NoEvalLicense = TRUE;

          MDB(3,fCONFIG) MLog("INFO:     eval license disabled\n");
          }
        else if ((!strcmp(OptArg,"port")) && (Value != NULL))
          {
          MSched.ServerPort = (int)strtol(Value,NULL,0);

          MDB(3,fCONFIG) MLog("INFO:     server port set to %d\n",
            MSched.ServerPort);
          }
        else if (!strcmp(OptArg,"version"))
          {
          fprintf(stderr,"%s server version %s (revision %s, changeset %s)\n",
            MSCHED_SNAME,
            MOAB_VERSION,
#ifdef MOAB_REVISION
            MOAB_REVISION,
#else
            "NA",
#endif /* MOAB_REVISION */
#ifdef MOAB_CHANGESET
            MOAB_CHANGESET);
#else
            "NA");
#endif /* MOAB_CHANGESET */


          exit(0);
          }
        }    /* END BLOCK (case '-') */

        break;

      case 'a':

        {
        enum MNodeAllocPolicyEnum tmpNAP;

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"WARNING:  expected arg not supplied for '-a' -- ignoring.\n");
          break;
          }

        tmpNAP = (enum MNodeAllocPolicyEnum)MUGetIndexCI(
          OptArg,
          MNAllocPolicy,
          FALSE,
          mnalNONE);

        if (tmpNAP == mnalNONE)
          {
          fprintf(stderr,"WARNING:  invalid node access policy '%s' specified - setting ignored\n",
            OptArg);
          }
        else
          {
          MPar[0].NAllocPolicy = tmpNAP;

          MPar[1].NAllocPolicy = tmpNAP;

          MDB(3,fCONFIG) MLog("INFO:     node allocation policy set to %s\n",
            MNAllocPolicy[tmpNAP]);
          }
        }    /* END BLOCK (case 'a') */

        break;

      case 'b':

        MSched.ForceBG = TRUE;

        break;

      case 'B':

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"WARNING:  expected arg not supplied for '-B' -- ignoring.\n");
          break;
          }

        MPar[0].BFDepth = (int)strtol(OptArg,NULL,0);

        MDB(3,fCONFIG) MLog("INFO:     backfill depth set to %d\n",
          MPar[0].BFDepth);

        break;

      case 'c':

        /* c-configfile */

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"ERROR:  invalid configfile specified\n");

          exit(1);
          }

        MUStrCpy(MSched.ConfigFile,OptArg,sizeof(MSched.ConfigFile));

        MDB(3,fCONFIG) MLog("INFO:     configfile set to %s\n",
          MSched.ConfigFile);

        break;

      case 'd':

        /* set 'debug' mode */

        MSched.DebugMode = TRUE;

        break;

      case 'e':

        /* Do not allow Moab to start if there is an error in the config. */

        MSched.StrictConfigCheck = TRUE;

        break;

      case 'h':

        MSysShowUsage(ArgV[0]);

        exit(0);

        /*NOTREACHED*/

        break;

      case 'i':

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"WARNING:  expected arg not supplied for '-i' -- ignoring.\n");
          break;
          }

        MSched.MaxRMPollInterval = strtol(OptArg,NULL,0);

        MDB(3,fCONFIG) MLog("INFO:     RMPollInterval set to %ld seconds\n",
          MSched.MaxRMPollInterval);

        break;

      case 'l':

        /* See also MSysLogInitialize -- this allows positional independent -l */
        /* TODO: remove possible redundancy  */

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"WARNING:  expected arg not supplied for '-l' -- ignoring.\n");
          break;
          }

        MUStrCpy(MSched.LogFile,OptArg,sizeof(MSched.LogFile));

        MDB(3,fCONFIG) MLog("INFO:     logfile set to '%s'\n",
          MSched.LogFile);

        if ((MSched.LogFile[0] == '/') || (MSched.LogDir[0] == '\0'))
          {
          MUStrCpy(tmp,MSched.LogFile,sizeof(tmp));
          }
        else
          {
          MUStrCpy(tmp,MSched.LogDir,sizeof(tmp));

          if (MSched.LogDir[strlen(MSched.LogDir) - 1] != '/')
            strcat(tmp,"/");

          MUStrCat(tmp,MSched.LogFile,sizeof(tmp));
          }

        MLogInitialize(tmp,-1,MSched.Iteration);

        break;

      case 'L':

        /* See also MSysLogInitialize -- this allows positional independent -L */
        /* TODO: remove possible redundancy  */

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"WARNING:  expected arg not supplied for '-L' -- ignoring.\n");
          break;
          }

        mlog.Threshold = strtol(OptArg,NULL,0);

        MDB(3,fCONFIG) MLog("INFO:     loglevel set to %d\n",
          mlog.Threshold);

        break;

      case 'm':

        if (MSched.ForceMode == msmNONE)
          {
          MSched.Mode = (enum MSchedModeEnum)MUGetIndexCI(
            OptArg,
            MSchedMode,
            FALSE,
            MSched.Mode);

          if (MSched.Mode == msmTest)
            MSched.Mode = msmMonitor;
          }

        break;

      case 'p':

        if ((OptArg != NULL) && (OptArg[0] != '\0'))
          {
          int tmpI;

          tmpI = (int)strtol(OptArg,NULL,0);
  
          if (tmpI > 0)
            {
            MSched.ServerPort = (int)strtol(OptArg,NULL,0);

            MDB(3,fCONFIG) MLog("INFO:     server port set to %d\n",
              MSched.ServerPort);
            }
          }

        break;

      case 'P':

        /* NOTE:  if scheduler is in NORMAL mode, use OptArg to indicate pause duration */

        /* disable scheduling, continue loading rm info */

        MSched.State = mssmPaused;
        MSched.StateMTime = MSched.Time;

        /* stop scheduling, freeze at initial reservation */

        /* MSched.Schedule = FALSE; */

        MDB(3,fCONFIG) MLog("INFO:     scheduler paused\n");

        if ((OptArg != NULL) && (OptArg[0] != '\0'))
          {
          /* resume scheduleing after specified duration */

          MSched.ResumeTime = MSched.Time + MUTimeFromString(OptArg);
          }

        break;

      case 'R':  /* R - recycle */

        /* FORMAT:  <TIME>[,savestate] */

        if ((OptArg != NULL) && (OptArg[0] != '\0'))
          {
          char   *ptr;
          char   *TokPtr;

          long tmpL;

          if (strstr(OptArg,"savestate"))
            {
            MUStrDup(&MSched.RecycleArgs,"savestate");
            }

          ptr = MUStrTok(OptArg,", \t\n",&TokPtr);

          tmpL = MUTimeFromString(ptr);

          if (tmpL > 0)
            {
            MSched.RestartTime = MSched.Time + tmpL;

            MDB(3,fCONFIG) MLog("INFO:     sched recycle time set to %ld seconds\n",
              tmpL);
            }
          }    /* END if ((OptArg != NULL) && (OptArg[0] != '\0')) */
        else 
          {
          fprintf(stderr,"WARNING:  expected arg not supplied for '-R' -- ignoring.\n");
          break;
          }

        break;

      case 's':

        MSched.UseCPRestartState = TRUE;

        break;

      case 'S':

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          /* stop scheduling, freeze at initial reservation */

          MSched.State = mssmStopped; 

          MDB(3,fCONFIG) MLog("INFO:     scheduler stopped at initial reservation\n");
          }
        else
          {
          /* stop at specified iteration */

          MSched.State = mssmRunning;

          MSched.StopIteration = (int)strtol(OptArg,NULL,0);
          }

        break;

      case 'v':

        fprintf(stderr,"%s version %s\n",
          MSCHED_NAME,
          MOAB_VERSION);

        exit(0);

        /*NOTREACHED*/

        break;

      case '?':

        if (PreLoad == FALSE)
          {
          MSysShowUsage(ArgV[0]);

          exit(0);
          }

        break;

      default:

        if (PreLoad == FALSE)
          {
          MDB(1,fCONFIG) MLog("WARNING:  unexpected flag '%c'\n",
            Flag);
          }

        break;
      }  /* END switch (Flag)                     */
    }    /* END while ((Flag = MUGetOpt()) != -1) */

  return(SUCCESS);
  }  /* END MSysProcessArgs() */




/**
 * Report commandline usage for server daemon.
 *
 * @param Command (I)
 */

int MSysShowUsage(

  char *Command)  /* I */

  {
  if (Command == NULL)
    {
    return(FAILURE);
    }

  fprintf(stderr,"Usage:   %s [FLAGS]\n",
    Command);

  /* fprintf(stderr,"  --configfile=<FILENAME>\n"); */
  fprintf(stderr,"  --help\n");
  /* fprintf(stderr,"  --host=<HOSTNAME>\n"); */
  /* fprintf(stderr,"  --keyfile=<FILENAME>\n"); */
  fprintf(stderr,"  --loglevel=<LOGLEVEL>\n");
  /* fprintf(stderr,"  --port=<SERVERPORT>\n"); */
  fprintf(stderr,"  --version\n");

/*
  fprintf(stderr,"  [ -a <NODE_ALLOCATION_POLICY> ]\n");
  fprintf(stderr,"  [ -b // BACKGROUND ]\n");
  fprintf(stderr,"  [ -B <BACKFILL_DEPTH> ]\n");
*/
  fprintf(stderr,"  [ -c <CONFIG_FILE> ]\n");
  fprintf(stderr,"  [ -d // DEBUG_MODE ]\n");
  fprintf(stderr,"  [ -e // STRICT STARTUP]\n");
/*
  fprintf(stderr,"  [ -L <LOGLEVEL> ]\n");
  fprintf(stderr,"  [ -f <LOGFACILITY> ]\n");
*/
  fprintf(stderr,"  [ -h ] // HELP\n");
/*
  fprintf(stderr,"  [ -H <SERVER_HOME_DIRECTORY> ]\n");
  fprintf(stderr,"  [ -i <SCHEDULING_INTERVAL> ]\n");
  fprintf(stderr,"  [ -j <WORKLOAD_TRACE_FILE> ]\n");
  fprintf(stderr,"  [ -l <LOG_FILE> ]\n");
  fprintf(stderr,"  [ -L <LOG_LEVEL> ]\n");
  fprintf(stderr,"  [ -m <SCHEDULER_MODE> ]\n");
  fprintf(stderr,"  [ -n <NODE_COUNT> ]\n");
  fprintf(stderr,"  [ -N <NODE_CONFIGURATION> ]\n");
  fprintf(stderr,"  [ -p <SERVICE_PORT> ]\n");
*/
  fprintf(stderr,"  [ -P [<PAUSEDURATION>]] // PAUSE\n");
/*
  fprintf(stderr,"  [ -r <RESOURCE_TRACE_FILE> ]\n");
*/
  fprintf(stderr,"  [ -R <RECYCLEDURATION> ]\n");
  fprintf(stderr,"  [ -s ] // Start with CP Restart State\n");
  fprintf(stderr,"  [ -S [<STOPITERATION>]] // STOP\n");
/*
  fprintf(stderr,"  [ -w <WALLCLOCK ACCURACY> ]\n");
  fprintf(stderr,"  [ -W <WC_ACCURACY_CHANGE> ]\n");
*/
  fprintf(stderr,"  [ -v ] // VERSION\n");
  fprintf(stderr,"  [ -? ] // HELP\n");

  return(SUCCESS);
  }  /* END MSysShowUsage() */





/** 
 * Authenticate user is admin1 and execution environment.
 *
 * @see MSysDaemonize() - peer
 *
 * NOTE: Called by main() - run once at start-up before any config is loaded.
 */

int MSysAuthenticate()

  {
  int uid;
  int gid;
  char tmpUName[MMAX_NAME];

  int index;

  mbool_t ValidAdmin;

  struct rlimit rlim;

  const char *FName = "MSysAuthenticate";

  MDB(2,fALL) MLog("%s()\n",
    FName);

  /* verify scheduler is being run by scheduler admin user */

  ValidAdmin = FALSE;

  uid = MOSGetUID();

  for (index = 0;MSched.Admin[1].UName[index][0] != '\0';index++)
    {
    if (!strcmp(MSched.Admin[1].UName[index],MUUIDToName(uid,tmpUName)))
      {
      ValidAdmin = TRUE;

      break;
      }
    }    /* END for (index) */

  if (ValidAdmin == FALSE)
    {
    MDB(0,fCORE) MLog("ERROR:    user %s (UID: %d) is not authorized to run this program\n",
      MUUIDToName(uid,tmpUName),
      uid);

    fprintf(stderr,"\nERROR:    user %s (UID: %d) is not authorized to run this program\n",
      MUUIDToName(uid,tmpUName),
      uid);

    exit(1);
    }

  /* run with UID of primary scheduler admin */

  uid = MUUIDFromName(MSched.Admin[1].UName[0],NULL,NULL);
  gid = MUGIDFromUser(uid,NULL,NULL);

  MSysSetCreds(uid,gid);

  MDB(3,fCORE) MLog("INFO:     executing scheduler from '%s' under UID %d GID %d\n",
    MSched.CfgDir,
    uid,
    gid);

  if (MSched.DefaultCSAlgo != mcsaMunge)
    {
    mbool_t  UseKeyFile;
    mbool_t  KeyFileExists;
    char EMsg[MMAX_LINE];

    /* get the moab key string from file/built source */
    if (MUCheckAuthFile(
          &MSched,
          MSched.DefaultCSKey,
          &KeyFileExists,
          &UseKeyFile,
          EMsg,
          TRUE) == SUCCESS)
      {
      if (UseKeyFile == TRUE)
        {
        MSched.UseKeyFile = TRUE;
        }
      else if (KeyFileExists == TRUE)
        {
        MDB(2,fCONFIG) MLog("WARNING:  .moab.key exists but has invalid ownership/permissions: %s\n",
          EMsg);

        fprintf(stderr,"WARNING:  .moab.key exists but has invalid ownership/permissions: %s\n",
          EMsg);

        MMBAdd(
          &MSched.MB,
          "WARNING:  .moab.key exists but has invalid ownership/permissions",
          (char *)"N/A",
          mmbtOther,
          0,
          1,
          NULL);
        }
      }

#ifdef __MREQKEYFILE
    if (UseKeyFile == FALSE)
      {
      fprintf(stderr,"ERROR:  .moab.key required - %s\n",
        (KeyFileExists == FALSE) ? "file does not exist" : "file has invalid ownership/permissions\n");

      exit(1);
      }  /* END if (UseKeyFile == FALSE) */
#endif /* __MREQKEYFILE */

    } /* END if (MSched.DefaultCSAlgo != mcsaMunge) */

  /* set up default umask value */
  /* MSched.UMask should always be != 0 because it is set in MSchedInitialize */

  if (MSched.UMask == 0)
    umask(MDEF_UMASK);

  /* verify system limits not enforced */

  if (getrlimit(RLIMIT_CPU,&rlim) == 0)
    {
    if (rlim.rlim_cur != RLIM_INFINITY)
      {
      fprintf(stderr,"WARNING:  OS cpu limits active (use 'ulimit' to adjust)\n");
      }
    }

  if (getrlimit(RLIMIT_STACK,&rlim) == 0)
    {
    /* limit reported in bytes */

    if ((rlim.rlim_cur != RLIM_INFINITY) && (rlim.rlim_cur < 100000000))
      {
      mulong origlimit;

      if (rlim.rlim_cur < rlim.rlim_max)
        {
        origlimit = rlim.rlim_cur;

        rlim.rlim_cur = rlim.rlim_max;

        if (setrlimit(RLIMIT_STACK,&rlim) == 0)
          {
          MDB(0,fALL) MLog("INFO:     OS stack limits increased from %ld MB to %ld MB (use 'ulimit' to adjust)\n",
            origlimit >> 20, 
            rlim.rlim_cur >> 20);
          }
        else
          {
          fprintf(stderr,"WARNING:  OS stack limits may be low (%ld MB) and cannot be increased by Moab (use 'ulimit' to adjust,errno=%d)\n",
            rlim.rlim_cur >> 20,
            errno);
          }
        }    /* END if (rlim.rlim_cur < rlim.rlim_max) */
      else
        {
        fprintf(stderr,"WARNING:  OS stack hard limits may be too low (%ld MB)\n",
          rlim.rlim_cur >> 20);
        }
      }
    }     /* END if (getrlimit(RLIMIT_STACK,&rlim) == 0) */

  if (getrlimit(RLIMIT_CORE,&rlim) == 0)
    {
    if (rlim.rlim_cur != RLIM_INFINITY)
      {
      MDB(5,fALL) MLog("INFO:     system core limit set to %d (complete core files might not be generated)\n",
        (int)rlim.rlim_cur);
      }
    }

  return(SUCCESS);
  }  /* END MSysAuthenticate() */






/**
 * Report copyright information.
 *
 * @see MUShowCopy() - peer/sync
 *
 * NOTE:  called via 'moab --about' 
 */

int MSysShowCopy(void)

  {
  mbool_t ShowLibCopy;

  fprintf(stderr,"Copyright (C) 2000-2012 by Adaptive Computing Enterprises, Inc. All Rights Reserved.\n");

  if (MSched.Copy != NULL)
    {
    fprintf(stderr,"\n%s\n",
      MSched.Copy);
    }

  /* NOTE:  MUShowCopy not displayed unless multiple 3rd party projects 
            are incorporated */

  ShowLibCopy = FALSE;

  /* locate 3rd party projects (NYI) */

  if (ShowLibCopy == TRUE)
    {
    MUShowCopy();
    }

  return(SUCCESS);
  }  /* END MSysShowCopy() */



/**
 * Reports information about what endianness Moab was
 * compiled with.
 *
 * NOTE: called via 'moab --about'
 *
 */

int MSysShowEndianness(void)

  {
#ifdef MLITTLEENDIAN
  fprintf(stderr,"Compiled as little endian.\n");
#else
  fprintf(stderr,"Compiled as big endian.\n");
#endif /* MLITTLEENDIAN */

  return(SUCCESS);
  }  /* END MSysShowEndianness() */






/**
 * Perform HTTP 'get' operation and report results.
 *
 * @param HostName (I) [optional]
 * @param Port     (I) [optional]
 * @param Path     (I) [optional]
 * @param Timeout  (I) [optional, in us, -1 for not set] 
 * @param Buffer   (O)
 */

int MSysHTTPGet(

  const char      *HostName,
  int        Port,
  const char      *Path,
  long       Timeout,
  mstring_t *Buffer)

  {
  char  tmpBuf[MMAX_NAME];
  char  tmpHost[MMAX_NAME];

  char *RspPtr = NULL;

  static const char *DefPath = "/";

  mpsi_t tmpP;

  const char *FName = "MSysHTTPGet";

  MDB(4,fSTRUCT) MLog("%s(%s,%d,%s,%ld,Buffer)\n",
    FName,
    (HostName != NULL) ? HostName : "NULL",
    Port,
    (Path != NULL) ? Path : "NULL",
    Timeout);

  memset(&tmpP,0,sizeof(tmpP));

  tmpP.Type = mpstWWW;

  if ((Path != NULL) && (Path[0] != '\0'))
    tmpP.Data = (void *)Path;
  else
    tmpP.Data = (void *)DefPath;

  tmpBuf[0] = '\0';

  if (HostName != NULL)
    MUStrCpy(tmpHost,HostName,sizeof(tmpHost));
  else
    MUStrCpy(tmpHost,"localhost",sizeof(tmpHost));

  tmpP.HostName = tmpHost;

  tmpP.Port     = (Port > 0) ? Port : MDEF_MHPORT;

  if (Timeout > 1000000)
    tmpP.Timeout = Timeout / 1000000;
  else if (Timeout >= 0)
    tmpP.Timeout = Timeout; 
  else
    tmpP.Timeout = 3;

  if (MS3DoCommand(
        &tmpP,    /* I (alloc) */
        tmpBuf,   /* O (not used) */
        &RspPtr,  /* O (alloc) */
        NULL,
        NULL,
        NULL) == FAILURE)
    {
    MDB(2,fCORE) MLog("ALERT:    cannot sync\n");

    return(FAILURE);
    }

  MSUFree(tmpP.S);

  /* response received */

  MDB(4,fCORE) MLog("INFO:     received http '%s'\n",
    RspPtr);

  MStringSet(Buffer,RspPtr);

  MUFree(&RspPtr);

  return(SUCCESS);
  }  /* END MSysHTTPGet() */




/**
 * 
 *
 */
#ifdef __UICHILDDEBUG

FILE   *fpTest;
char   TestLogMessage[200];

void ChildTestLog(
  char *logMessage)
  {
  fprintf(fpTest,"%s %s",
    MLogGetTime(),
    logMessage);

  fflush(fpTest);
  }
#endif




/**
 * Check spool directory for old files and remove them.
 *
 * This function is not used currently, it is also undocumented.
 */

void __MSysCheckOldFiles()

  {
  char SpoolDir[MMAX_PATH_LEN];

  static long LastFileRemoval = 0;

  if (LastFileRemoval == 0)
    {
    LastFileRemoval = MSched.Time;
    }

  if ((MSched.Time - LastFileRemoval) > MSched.SpoolDirKeepTime)
    {
    /* remove old files from spool directory */

    MUGetSchedSpoolDir(SpoolDir,sizeof(SpoolDir));

    MURemoveOldFilesFromDir(SpoolDir,MSched.SpoolDirKeepTime,TRUE,NULL);

    LastFileRemoval = MSched.Time;
    }
  }  /* END MSysCheckOldFiles() */






/**
 * Perform various periodic server-level operations.
 *
 * NOTE:  called before MRMUpdate() 
 *
 * @see MSchedUpdateStats() - perform limited periodic actions 
 * @see MStatWritePeriodPStats() - child
 */

int MServerUpdate()

  {
  int  OldHour;
  int  OldDay;
  int  OldWeekDay;
  int  OldMonth;
  int  rc;

  mbool_t UpdateIDManager = FALSE;
  mbool_t UpdateCPA       = FALSE;

  mulong tmpL;
  
  static int StatTestCount = -1;

  static mbool_t DoChargeAllocation = FALSE; /* static for race condition (see below) */

  const char *FName = "MServerUpdate";

  MDB(3,fALL) MLog("%s()\n",
    FName);

  OldHour    = MSched.DayTime;
  OldDay     = MSched.Day;
  OldWeekDay = MSched.WeekDay;
  OldMonth   = MSched.Month;

  /* adjust time, day, week, month, iteration */

  MSysUpdateTime(&MSched);

  MSched.Iteration++;

/* 
 * Disable obsolete MAMAccountUpdateBalance called via MAMRefresh *
 *
 *  if ((MAM[0].Type != mamtNONE) && (MAM[0].State == mrmsActive))
 *    {
 *    MAMRefresh(&MAM[0]);
 *    }
 */

  /* handle 'pre scheduling' tasks */

  /* NOTE:  for now, assume minute tasks should be executed every iteration */

  /* First of all, check to make sure host was not changed out from under Moab */

  if (MSysCheckHost() == FALSE)
    {
    /* If host was changed, recheck the license */
    /* If new host is not licensed Moab will shutdown */

    MLicenseRecheck();
    }


  if ((MID[0].State == mrmsActive) && (MID[0].RefreshPeriod == mpMinute))
    {
    MIDLoadCred(
      &MID[0],
      mxoNONE,
      NULL);
    }

  /* NOTE:  on some systems, localtime is failing, use the LastRefresh check as a fallback */

  if ((MID[0].RefreshPeriod == mpHour) &&
      (MID[0].LastRefresh + MCONST_HOURLEN < MSched.Time))
    {
    if (MID[0].State == mrmsActive)
      UpdateIDManager = TRUE;
    }

  if (MSched.DayTime != OldHour) 
    {
    if ((MID[0].State == mrmsActive) && (MID[0].RefreshPeriod == mpHour))
      UpdateIDManager = TRUE;

    if (MRM[0].SubType == mrmstXT3)
      UpdateCPA = TRUE;

    if (MSched.SMatrixPeriod == mpHour)
      {
      int mindex;

      /* roll stats matrix */

      for (mindex = MMAX_GRIDDEPTH;mindex > 0;mindex--)
        {
        memcpy(
          &MStat.SMatrix[mindex],
          &MStat.SMatrix[mindex - 1],
          sizeof(&MStat.SMatrix[mindex]));
        }  /* END for (mindex) */     
      }

    if ((MAM[0].State == mrmsActive) && (MAM[0].FlushPeriod == mpHour))
      DoChargeAllocation = TRUE;
    }      /* END if (MSched.Hour != OldHour) */
 
  if (StatTestCount == -1)
    {
    char *ptr;
   
    ptr = getenv("MOABSTATTEST");

    if (ptr != NULL)
      StatTestCount = MAX(0,(int)strtol(ptr,NULL,10));
    else
      StatTestCount = MMAX_TIME;
    }

  /* Ensure that OLD GEvents have been cleared */
  MGEventClearOldGEvents();

  if ((MSched.Day != OldDay) || (MSched.Iteration >= StatTestCount))
    {
    /* starting new day */

    tmpL = MSched.Time - (MSched.DayTime + 1) * MCONST_HOURLEN;

    if (MSched.MDB.DBType == mdbNONE)
      MStatWritePeriodPStats(tmpL,mpDay);

    /* update statistics */

    if (MSched.Iteration > 0)
      {
      /* NOTE:  stat file already opened previously on sched start up */

      MStatOpenFile(MSched.Time,NULL);
      }

    if ((MID[0].State == mrmsActive) && (MID[0].RefreshPeriod == mpDay))
      {
      UpdateIDManager = TRUE;
      }

    if (MSched.SMatrixPeriod == mpDay)
      {
      int mindex;

      /* roll stats matrix */

      for (mindex = MMAX_GRIDDEPTH;mindex > 0;mindex--)
        {
        memcpy(
          &MStat.SMatrix[mindex],
          &MStat.SMatrix[mindex - 1],
          sizeof(&MStat.SMatrix[mindex]));
        }  /* END for (mindex) */
      }

    if ((MAM[0].State == mrmsActive) && (MAM[0].FlushPeriod == mpDay))
      DoChargeAllocation = TRUE;

    if (MSched.PBSAccountingDir != NULL)
      {
      MStatOpenCompatFile(MSched.Time);
      }

    /* Perform License Recheck on a per day rollover event */
    MLicenseRecheck();

    }    /* END if ((MSched.Day != OldDay) || (getenv("MOABSTATTEST") != NULL)) */

  if (MSched.WeekDay < OldWeekDay)
    {
    /* starting new week */

    if (MSched.SMatrixPeriod == mpWeek)
      {
      int mindex;

      /* roll stats matrix */

      for (mindex = MMAX_GRIDDEPTH;mindex > 0;mindex--)
        {
        memcpy(
          &MStat.SMatrix[mindex],
          &MStat.SMatrix[mindex - 1],
          sizeof(&MStat.SMatrix[mindex]));
        }  /* END for (mindex) */
      }

    if ((MAM[0].State == mrmsActive) && (MAM[0].FlushPeriod == mpWeek))
      DoChargeAllocation = TRUE;
    }    /* END if (MSched.WeekDay != OldWeekDay) */

  if (MSched.Month != OldMonth)
    {
    /* starting new month */

    if ((MAM[0].State == mrmsActive) && (MAM[0].FlushPeriod == mpMonth))
      DoChargeAllocation = TRUE;
    }    /* END if (MSched.Month != OldMonth) */

  if (MSched.Iteration == 0)
    {
    if ((MID[0].State == mrmsActive) && (MID[0].RefreshPeriod == mpInfinity))
      {
      UpdateIDManager = TRUE;
      }
    }

  if (DoChargeAllocation == TRUE)
    {
     /* NOTE:  race condition: allocated rsv resource information required for
               charging not populated until node information loaded w/in
               MRMUpdate() on iteration 0 */
    if (MSched.Iteration > 0)
      {
      long Midnight;

      MUGetPeriodRange(MSched.Time,0,0,mpDay,&Midnight,NULL,NULL);

      MAM[0].FlushTime = Midnight +
        (((MSched.Time - Midnight) / MAM[0].FlushInterval) + 1) * MAM[0].FlushInterval;

      MRsvChargeAllocation(NULL,MBNOTSET,NULL);

      MJobDoPeriodicCharges();

      if (MSched.QueueNAMIActions == TRUE)
        {
        /* Flush anything left in the NAMI queues */

        int AMIndex;
        mam_t *A;

        for (AMIndex = 0;AMIndex < MMAX_AM;AMIndex++)
          {
          A = &MAM[AMIndex];

          if (A->Type != mamtNative)
            continue;

          MAMFlushNAMIQueues(A);
          }
        }  /* END if (MSched.QueueNAMIActions == TRUE) */

      DoChargeAllocation = FALSE;
      }
    else
      {
      /* Never charge on a moab startup since all time buckets will roll over.
         This could potentially skip a charge but that should be covered by
         the last charge time */

      DoChargeAllocation = FALSE;
      }
    }  /* END if (DoChargeAllocation == TRUE) */

  if (UpdateCPA == TRUE)
    {
    MSysCheckAllocPartitions();
    }

  if (UpdateIDManager == TRUE)
    {
    /* update id manager data */

    rc = MIDLoadCred(
      &MID[0],
      mxoNONE,
      NULL);

    if (rc == FAILURE && MID[0].UpdateRefreshOnFailure)
      {
      /* we failed to load the id cred info and will wait to try again
         until the next refresh period. otherwise we would try again the 
         next iteration */

      MID[0].LastRefresh = MSched.Time;
      }
    }

  if (MSched.Iteration == 0)
    {
    /* load the per-partition information and put it into the tree */

    MFSInitializeFSTreeData(&MPar[0].FSC);

    /* calculate the FSFactor from the information we have loaded */

    MFSInitialize(&MPar[0].FSC);

    /* organize the tree based on the information that has been loaded and calculated */

    MFSTreeUpdateUsage();
    }

  __MSysCheckOldFiles();

  MSched.DBIterationRetries = 0;

  return(SUCCESS);
  }  /* END MServerUpdate() */




/* return SUCCESS if event detected */

int MSysCheckEvents()

  {
  static mbool_t EventReceived = FALSE;

  if ((MRMCheckEvents() == SUCCESS) || (MSched.LocalEventReceived == TRUE))
    {
    EventReceived = TRUE;
    }

  MSched.LocalEventReceived = FALSE;

  if (EventReceived == FALSE)
    {
    return(FAILURE);
    }

  /* event received from RM - determine whether or not to propogate it */

  if (MSched.RMEventAggregationTime > 0)
    {
    if (MSched.FirstEventReceivedTime == 0)
      {
      /* this is the first event to occur during this event aggregation time span */

      MSched.FirstEventReceivedTime = MSched.Time;
      }

    if ((MSched.JobTermEventReceivedTime > 0) && 
       ((MSched.RMJobAggregationTime + MSched.JobTermEventReceivedTime) <= MSched.Time))
      {
      /* job 'termination' event received */

      /* NO-OP */
      }
    else if ((MSched.RMEventAggregationTime + MSched.FirstEventReceivedTime) > (long)MSched.Time)
      {
      /* aggregation time span has not yet expired - don't process events */

      return(FAILURE);
      }

    /* aggregation window has expired - reset and report events */

    MSched.FirstEventReceivedTime = 0;
    MSched.JobSubmitEventReceivedTime = 0;
    MSched.JobTermEventReceivedTime = 0;
    }  /* END if (MSched.RMEventAggregationTime > 0) */

  EventReceived = FALSE;

  return(SUCCESS);
  }  /* END MSysCheckEvents() */





/* raises a scheduling event to cause UI phase to exit sooner */

int MSysRaiseEvent()
  {
  MSched.LocalEventReceived = TRUE;

  return(SUCCESS);
  }  /* END MSysRaiseEvent() */





/**
 * Locate peer object associated with client name/host.
 *
 * @see MSURecvData() - parent
 *
 * @param SPName (I)
 * @param AllowRM (I)
 * @param PP (O) [optional]
 * @param FindByHost (I)
 */

int MPeerFind(

  char     *SPName,     /* I */
  mbool_t   AllowRM,    /* I */
  mpsi_t  **PP,         /* O (optional) */
  mbool_t   FindByHost) /* I */

  {
  mpsi_t *P;

  int pindex;
  
  mbool_t FindByPeerName;

  char PName[MMAX_NAME];

  if (PP != NULL)
    *PP = NULL;

  if ((SPName == NULL) || (PP == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT:  <CLIENTNAME> || PEER:<CLIENTNAME> */

  if (!strncasecmp(SPName,"PEER:",strlen("PEER:")))
    {
    strncpy(PName,SPName + strlen("PEER:"),sizeof(PName));

    FindByPeerName = TRUE;
    }
  else
    {
    strncpy(PName,SPName,sizeof(PName));

    FindByPeerName = FALSE;
    }

  for (pindex = 0;pindex < MMAX_PEER;pindex++)
    {
    P = &MPeer[pindex];

    if (P->Name[0] == '\0')
      break;

    if ((FindByHost == FALSE) && 
        (FindByPeerName == FALSE) && 
        (P->Type != mpstNONE))
      {
      if ((AllowRM == FALSE) || (P->Type != mpstNM))
        continue;
      }

    /* only allow lookup by host if we are servicing a peer--NOT A CLIENT */

    if ((FindByHost == TRUE) && (FindByPeerName == TRUE))
      {
      if (P->HostName == NULL)
        {
        continue;
        }

      if (strcasecmp(P->HostName,PName))
        {
        char *IPAddr;

        if (MUGetIPAddr(PName,&IPAddr) == FAILURE)
          {
          /* can't lookup hostname's IP address */

          continue;
          }

        if ((P->IPAddr == NULL) ||
            (IPAddr == NULL) ||
            strcasecmp(P->IPAddr,IPAddr))
          {
          MUFree(&IPAddr);

          continue;
          }

        MUFree(&IPAddr);
        }
      }
    else
      {
      /* check by client name */

      if (strcasecmp(P->Name,PName))
        {
        continue;
        }
      }

    if (P->IsBogus == TRUE)
      {
      /* bogus peer detected */

      return(FAILURE);
      }

    /* peer located */

    *PP = P;

    if (FindByHost == TRUE)
      {
      /* ensure admin privileges are given to new peer */

      /* NO-OP */
      }

    return(SUCCESS);
    }  /* END for (pindex) */

  if (FindByPeerName == TRUE)
    {
    /* peer may be resource manager type - load info from config */

    mpsi_t *tP = NULL;

    if (MOLoadPvtConfig(
         NULL,
         mxoRM,
         PName,
         &tP,      /* O */
         NULL,
         NULL) == SUCCESS)
      {
      if (tP == NULL)
        {
        return(FAILURE);
        }

      if (PP != NULL)
        *PP = tP;

      return(SUCCESS);
      }
    }    /* END if (FindByPeerName == TRUE) */
  
  return(FAILURE);
  }  /* END MPeerFind() */




/* NOTE:  assume prefix (ie 'RM:', 'AM:', etc) is stripped */

/**
 *
 *
 * @param PName (I)
 * @param PP (O)
 */

int MPeerAdd(

  char    *PName,  /* I */
  mpsi_t **PP)     /* O */

  {
  mpsi_t *P;

  int pindex;

  if (PP != NULL)
    *PP = NULL;

  if ((PName == NULL) || (PP == NULL))
    {
    return(FAILURE);
    }

  for (pindex = 0;pindex < MMAX_PEER;pindex++)
    {
    P = &MPeer[pindex];

    if (!strcmp(P->Name,PName))
      {
      /* peer already exists */

      *PP = P;

      return(SUCCESS);
      }

    if (P->Name[0] == '\0')
      {
      /* available slot located */

      strcpy(P->Name,PName);

      *PP = P;

      return(SUCCESS);
      }
    }  /* END for (pindex) */

  return(FAILURE);
  }  /* END MPeerAdd() */





/**
 *
 *
 * @param P
 */

int MPeerFree(

  mpsi_t *P)

  {
  if (P == NULL)
    {
    return(FAILURE);
    }

  MUFree(&P->HostName);
  MUFree(&P->IPAddr);
  MUFree(&P->CSKey);
  MUFree(&P->RID);

  return(SUCCESS);
  }  /* END MPeerFree() */





/**
 *
 *
 * @param DstP (O) [if *DstP == NULL a new peer is returned]
 * @param SrcP (I)
 */

int MPeerDup(

  mpsi_t **DstP,  /* O (if *DstP == NULL a new peer is returned) */
  mpsi_t  *SrcP)  /* I */

  {
  mpsi_t *P;
  
  if ((DstP == NULL) || (SrcP == NULL))
    {
    return(FAILURE);
    }

  if (*DstP != NULL)
    {
    P = *DstP;
    }
  else
    {
    P = (mpsi_t *)MUMalloc(sizeof(mpsi_t));
    }

  memcpy(P,SrcP,sizeof(mpsi_t));
  
  /* deep copy of strings */

  P->Version = NULL;
  MUStrDup(&P->Version,SrcP->Version);

  P->CSKey = NULL;
  MUStrDup(&P->CSKey,SrcP->CSKey);
  
  P->RID = NULL;
  MUStrDup(&P->RID,SrcP->RID);

  P->HostName = NULL;
  MUStrDup(&P->HostName,SrcP->HostName);

  P->IPAddr = NULL;
  MUStrDup(&P->IPAddr,SrcP->IPAddr);

  *DstP = P;

  return(SUCCESS);
  }  /* END MPeerDup() */



/**
 * return list of mail addresses for members of specified admin level
 *
 * @param AIndex (I)
 */

char *MSysGetAdminMailList(

  int AIndex)  /* I */

  {
  char *BPtr;
  int   BSpace;

  int aindex;

  char *NPtr;

  mgcred_t *U;

  char  tmpName[MMAX_NAME];

  static char tmpLine[MMAX_LINE];

  MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

  for (aindex = 0;aindex < MMAX_ADMINUSERS;aindex++)
    {
    if (MSched.Admin[AIndex].UName[aindex][0] == '\0')
      break;

    if (!strncasecmp(MSched.Admin[AIndex].UName[aindex],"PEER:",strlen("PEER:")))
      continue;

    if ((MUserFind(MSched.Admin[AIndex].UName[aindex],&U) == FAILURE) ||
        (U->EMailAddress == NULL))
      {
      snprintf(tmpName,sizeof(tmpName),"%s@%s",
        MSched.Admin[AIndex].UName[aindex],
        (MSched.DefaultMailDomain != NULL) ? MSched.DefaultMailDomain : "localhost");

      NPtr = tmpName;
      }
    else if (!strcasecmp(U->EMailAddress,"nomail"))
      {
      continue;
      }
    else
      {
      NPtr = U->EMailAddress;
      }
 
    if (tmpLine[0] == '\0')
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        NPtr); 
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,",%s",
        NPtr);
      }
    }  /* END for (aindex) */

  return(tmpLine);
  }  /* END MSysGetAdminMailList() */ 





/**
 * Returns an iterator over events which match timeframe/object constraints.
 *
 * @see MOWriteEvent() 
 * @see MSysRecordEvent()
 * @see MUISchedCtl() - parent - handle 'mschedctl -q events'
 *
 * @param StartTime (I)
 * @param EndTime   (I)
 * @param EID       (I) [optional,terminated w/-1]
 * @param EType     (I) [optional,terminated w/mrelLAST]
 * @param OType     (I) [optional,terminated w/mxoLAST]
 * @param OID       (I) [optional]
 * @param Itr       (O) 
 */

int MSysQueryEvents(

  mulong                     StartTime,
  mulong                     EndTime,
  int                       *EID,
  enum MRecordEventTypeEnum *EType,
  enum MXMLOTypeEnum        *OType,
  char                       OID[][MMAX_LINE],
  mevent_itr_t              *Itr)

  {
  int rc;
  mdb_t *MDBInfo;

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MDBInfo->DBType != mdbNONE)
    {
    rc = MDBQueryEventsWithRetry(MDBInfo,StartTime,EndTime,EID,EType,OType,OID,Itr,NULL);
    }
  else
    {
    rc = MSysQueryEventsFile(StartTime,EndTime,EID,EType,OType,OID,Itr);
    }

  return(rc);
  }  /* END MSysQueryEvents() */





/**
 * Gets the next event in a file-based events iterator
 * @param Itr (I)
 * @param Out (O)
 */

int MSysEFileItrNext(

  mevent_itr_t *Itr,   /* I */
  mevent_t  *Out)      /* O */
  {
  char  StatFName[MMAX_LINE];
  char   tmpETime[MMAX_LINE];
  char   EventOType[MMAX_LINE];
  char   EventEType[MMAX_LINE];

  char *ptr2;
  char *TokPtr2 = NULL;
  int index;


  int *EID = Itr->TypeUnion.FileItr.EID;
  enum MRecordEventTypeEnum *EType = Itr->TypeUnion.FileItr.EType;
  enum MXMLOTypeEnum *OType = Itr->TypeUnion.FileItr.OType;
  char (*OID)[MMAX_LINE] = Itr->TypeUnion.FileItr.OID;

  int EIDSize = Itr->TypeUnion.FileItr.EIDSize;
  int ETypeSize = Itr->TypeUnion.FileItr.ETypeSize;
  int OTypeSize = Itr->TypeUnion.FileItr.OTypeSize;
  int OIDSize = Itr->TypeUnion.FileItr.OIDSize;

  mulong StartTime = Itr->TypeUnion.FileItr.StartTime;
  mulong EndTime = Itr->TypeUnion.FileItr.EndTime;
  mulong *CurTimeP = &Itr->TypeUnion.FileItr.CurTime;
  char **ptr = &Itr->TypeUnion.FileItr.CurFilePos;
  char **TokPtr = &Itr->TypeUnion.FileItr.CurFileTokContext;

  if (Itr->TypeUnion.FileItr.FileBuf == NULL)
    {
    if (MStatGetFullNameSearch(CurTimeP,EndTime,StatFName) != SUCCESS)
      return(MITR_DONE);

    if ((Itr->TypeUnion.FileItr.FileBuf = MFULoadNoCache(StatFName,1,NULL,NULL,NULL,NULL)) == NULL)
      return(FAILURE);

    *ptr = MUStrTok(Itr->TypeUnion.FileItr.FileBuf,"\n",TokPtr);
    }

  if (*CurTimeP > EndTime)
    return(MITR_DONE);

  while (*ptr != NULL)
    {
    int StrSize;
    char const *Context;
    char const *Delims = " \t";
    mbool_t parseSucceeded;

    ptr2 = MUStrScan(*ptr,Delims,&StrSize,&Context);
    /*ignore first item */

    ptr2 = MUStrScan(NULL,Delims,&StrSize,&Context);
    MUStrCpy(tmpETime,ptr2,MIN((int)sizeof(tmpETime),StrSize + 1));

    ptr2 = MUStrScan(NULL,Delims,&StrSize,&Context);
    MUStrCpy(EventOType,ptr2,MIN((int)sizeof(EventOType),StrSize + 1));

    ptr2 = MUStrScan(NULL,Delims,&StrSize,&Context);
    MUStrCpy(Out->Name,ptr2,MIN((int)sizeof(Out->Name),StrSize + 1));

    ptr2 = MUStrScan(NULL,Delims,&StrSize,&Context);
    MUStrCpy(EventEType,ptr2,MIN((int)sizeof(EventEType),StrSize + 1));

    /*check that we succeeded at least to this point */
    parseSucceeded = ptr != NULL;

    ptr2 = MUStrScan(NULL,Delims,&StrSize,&Context);
    MUStrCpy(Out->Description,ptr2,sizeof(Out->Description));

    if (!parseSucceeded)
      continue;

    ptr2 = MUStrTok(tmpETime,":",&TokPtr2);

    if (ptr2 != NULL)
      Out->Time= strtoul(ptr2,NULL,10);
    else
      Out->Time = 0;

    ptr2 = MUStrTok(NULL,":",&TokPtr2);

    if (ptr2 != NULL)
      Out->ID = strtol(ptr2,NULL,10);
    else
      Out->ID = -1;

      /* event id constraints specified */

    for (index = 0; index < EIDSize; index++)
      {
      if (EID[index] == Out->ID)
        break;
      }

    if (index != 0 && index >= EIDSize)
      {
      *ptr = MUStrTok(NULL,"\n",TokPtr);

      continue;
      }

    Out->EType = (enum MRecordEventTypeEnum)MUGetIndexCI(EventEType,MRecordEventType,FALSE,mrelNONE);

    for (index = 0; index < ETypeSize; index++)
      {
      if (Out->EType == EType[index])
        break;
      }

    if (index != 0 && index >= ETypeSize)
      {
      *ptr = MUStrTok(NULL,"\n",TokPtr);

      continue;
      }

    Out->OType = (enum MXMLOTypeEnum)MUGetIndexCI(EventOType,MXO,FALSE,mxoLAST);


    for (index = 0;index < OTypeSize;index++)
      {
      if (Out->OType == OType[index])
        break;
      }

    if (index != 0 && index >= OTypeSize)
      {
      *ptr = MUStrTok(NULL,"\n",TokPtr);

      continue;
      }

    for (index = 0;index < OIDSize;index++)
      {
      if (!strcasecmp(Out->Name,OID[index]))
        break;
      }

    if (index != 0 && index >= OIDSize)
      {
      *ptr = MUStrTok(NULL,"\n",TokPtr);

      continue;
      }

    if (Out->Time < StartTime)
      {
      *ptr = MUStrTok(NULL,"\n",TokPtr);
      continue;
      }
    else if (Out->Time > EndTime)
      {
      return(MITR_DONE);
      }
    else 
      {
      *ptr = MUStrTok(NULL,"\n",TokPtr);
      return(SUCCESS);
      }
    }  /* END while (*ptr != NULL) */

  *CurTimeP = (*CurTimeP + MCONST_DAYLEN);

  MUFree(&Itr->TypeUnion.FileItr.FileBuf);

  return(MSysEFileItrNext(Itr,Out));
  }  /* END MEventToXML */



/**
 * Returns the next event in an events iterator
 *
 * @param Itr (I)
 * @param Out (O)
 * @return SUCCESS, FAILURE, or, if there are no more items, MITR_DONE
 */

int MSysEItrNext(

  mevent_itr_t *Itr,  /* I */
  mevent_t *Out)      /* O */

  {
  switch (Itr->Type)
    {
    case meiFile:
      return(MSysEFileItrNext(Itr,Out));

      break;

    case meiDB:
      return(MDBEItrNext(Itr,Out));

      break;

    default:
      return(FAILURE);
    }
  } /* END MSysEItrNext */




/**
 * Clears resources associated with a file-bsed events iterator
 * 
 * @param Itr (I)
 */

int MSysEFileItrClear(

    mevent_itr_t *Itr) /* I */
  {
  MUFree((char **)&Itr->TypeUnion.FileItr.EID);
  MUFree((char **)&Itr->TypeUnion.FileItr.EType);
  MUFree((char **)&Itr->TypeUnion.FileItr.OType);
  MUFree((char **)&Itr->TypeUnion.FileItr.OID);
  MUFree((char **)&Itr->TypeUnion.FileItr.FileBuf);

  return(SUCCESS);
  } /* END MSysEFileItrClear */




/**
 * Clear resources associated with an events iterator
 *
 * @param Itr (I)
 */

int MSysEItrClear(

  mevent_itr_t *Itr) /* I */

  {
  switch (Itr->Type)
    {
    case meiFile:

      return(MSysEFileItrClear(Itr));

      break;

    case meiDB:

      return(MDBEItrClear(Itr));

      break;

    default:

      /* NOT SUPPORTED */

      /* NO-OP */

      break;
    }  /* END switch (Itr->Type) */

  return(FAILURE);
  }  /* END MSysEItrClear() */





/**
 * Query events and report results in XML. All events will be populated as 
 * children of the passed in 'ParentElement' XML structure. 
 * 
 * @see MSysQueryEvents() - child
 * @see MEventToXML() - child
 *
 * @param StartTime     (I)
 * @param EndTime       (I)
 * @param EID           (I) [optional,terminated w/-1]
 * @param EType         (I) [optional,terminated w/mrelLAST]
 * @param OType         (I) [optional,terminated w/mxoLAST]
 * @param OID           (I) [optional]
 * @param ParentElement (O) receives the events as children on success
 */

int MSysQueryEventsToXML(

  mulong                     StartTime,
  mulong                     EndTime,
  int                       *EID,
  enum MRecordEventTypeEnum *EType,
  enum MXMLOTypeEnum        *OType,
  char                       OID[][MMAX_LINE],
  mxml_t                    *ParentElement)

  {
  mevent_itr_t Itr;
  mevent_t Event;
  int rc;

  rc = MSysQueryEvents(StartTime,EndTime,EID,EType,OType,OID,&Itr);

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  while (TRUE)
    {
    int fetchRC;
    mxml_t *Element = NULL;

    fetchRC = MSysEItrNext(&Itr,&Event);

    if (fetchRC == MITR_DONE)
      break;

    if (fetchRC == FAILURE)
      {
      rc = FAILURE;

      break;
      }

    /* Skip if empty event */

    if (Event.EType == mrelNONE)
      continue;

    rc = MEventToXML(Event.ID,Event.EType,Event.OType,Event.Time,Event.Name,NULL,Event.Description,&Element);

    if (rc == FAILURE)
      break;

    MXMLAddE(ParentElement,Element);
    }  /* END while (TRUE) */

  MSysEItrClear(&Itr);

  return(rc);
  }  /* END MSysQueryEventsToXML() */





/**
 * Query events to an array. 
 * 
 * @see MSysQueryEvents() - child
 *
 * @param StartTime (I)
 * @param EndTime (I)
 * @param EID (I) [optional,terminated w/-1]
 * @param EType (I) [optional,terminated w/mrelLAST]
 * @param OType (I) [optional,terminated w/mxoLAST]
 * @param OID (I) [optional]
 * @param EventArrayOut     (O) recieves the array containing the event structures
 * @param EventArrayOutSize (O) receives the size of the outputted array
 * @param EMsg              (O) [optional,minsize=MMAX_LINE]
 */

int MSysQueryEventsToArray(

  mulong                     StartTime,  /* I */
  mulong                     EndTime,    /* I */
  int                       *EID,        /* I (optional,terminated w/-1) */
  enum MRecordEventTypeEnum *EType,      /* I (optional,terminated w/mrelLAST) */
  enum MXMLOTypeEnum        *OType,      /* I (optional,terminated w/mxoLAST) */
  char                       OID[][MMAX_LINE], /* I (optional) */
  mevent_t                 **EventArrayOut,     /* O recieves the array containing the event structures */
  int                       *EventArrayOutSize, /* O receives the size of the outputted array */
  char                      *EMsg)              /* O [optional,minsize=MMAX_LINE]                      */

  {
  mevent_t *Result = NULL;
  int Capacity;
  int Size;
  int rc;
  mevent_itr_t Itr;

  rc = MSysQueryEvents(StartTime,EndTime,EID,EType,OType,OID,&Itr);

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  Capacity = 10;
  Size = 0;
  Result = (mevent_t *)MUCalloc(Capacity,sizeof(Result[0]));

  while (TRUE)
    {
    int fetchRC;
    mevent_t *Event = Result + Size;

    if (Size >= Capacity)
      {
      Capacity =  (int)( Capacity * 1.5);
      Result = (mevent_t *)realloc(Result,Capacity);
      }

    fetchRC = MSysEItrNext(&Itr,Event);

    if (fetchRC == MITR_DONE)
      break;
    else if (fetchRC == FAILURE)
      {
      rc = FAILURE;
      break;
      }

    Size++;
    }

  MSysEItrClear(&Itr);

  if (rc == FAILURE)
    {
    MUFree((char **)&Result);
    Size = 0;
    }

  if (EventArrayOut != NULL)
    *EventArrayOut = Result;
  else
    free(Result);

  if (EventArrayOutSize != NULL)
    *EventArrayOutSize = Size;

  return(rc);
  }  /* END MSysQueryEventsToArray() */





int MSysEFileItrInit(

  mevent_itr_t              *Itr,
  mulong                     StartTime,  /* I */
  mulong                     EndTime,    /* I */
  int                       *EID,        /* I (optional,terminated w/-1) */
  enum MRecordEventTypeEnum *EType,      /* I (optional,terminated w/mrelLAST) */
  enum MXMLOTypeEnum        *OType,      /* I (optional,terminated w/mxoLAST) */
  char                       OID[][MMAX_LINE]) /* I (optional) */

  {
  int *End;
  int  copySize;

  mbool_t useEID = ((EID != NULL) && (EID[0] != -1));
  mbool_t useEType = ((EType != NULL) && (EType[0] != mrelLAST));
  mbool_t useOType = ((OType != NULL) && (OType[0] != mxoLAST));
  mbool_t useOID = ((OID != NULL) && (OID[0][0] != 0));

  Itr->TypeUnion.FileItr.EIDSize = 0;
  Itr->TypeUnion.FileItr.ETypeSize = 0;
  Itr->TypeUnion.FileItr.OTypeSize = 0;
  Itr->TypeUnion.FileItr.OIDSize = 0;

  Itr->TypeUnion.FileItr.EID = NULL;
  Itr->TypeUnion.FileItr.EType = NULL;
  Itr->TypeUnion.FileItr.OType = NULL;
  Itr->TypeUnion.FileItr.OID = NULL;

  if (useEID)
    {
    for (End = EID; *End != -1; End++);

    Itr->TypeUnion.FileItr.EIDSize = End - (int *)EID;
    copySize = Itr->TypeUnion.FileItr.EIDSize * sizeof(Itr->TypeUnion.FileItr.EID[0]);
    Itr->TypeUnion.FileItr.EID = (int *)MUMalloc(copySize);
    memcpy(Itr->TypeUnion.FileItr.EID,EID,copySize);
    }

  if (useEType)
    {
    for (End = (int *)EType; *End != mrelLAST; End++);

    Itr->TypeUnion.FileItr.ETypeSize = End - (int *)EType;
    copySize = Itr->TypeUnion.FileItr.ETypeSize *sizeof(Itr->TypeUnion.FileItr.EType[0]);
    Itr->TypeUnion.FileItr.EType = (enum MRecordEventTypeEnum *)MUMalloc(copySize);
    memcpy(Itr->TypeUnion.FileItr.EType,EType,copySize);
    }

  if (useOType)
    {
    for (End = (int *)OType; *End != mxoLAST; End++);

    Itr->TypeUnion.FileItr.OTypeSize = End - (int *)OType;
    copySize = Itr->TypeUnion.FileItr.OTypeSize *sizeof(Itr->TypeUnion.FileItr.OType[0]);
    Itr->TypeUnion.FileItr.OType = (enum MXMLOTypeEnum *)MUMalloc(copySize);
    memcpy(Itr->TypeUnion.FileItr.OType,OType,copySize);
    }

  if (useOID)
    {
    char (*End)[MMAX_LINE];

    for (End = OID; (*End)[0] != 0; End++);

    Itr->TypeUnion.FileItr.OIDSize = End - OID;

    copySize = Itr->TypeUnion.FileItr.OIDSize *sizeof(Itr->TypeUnion.FileItr.OID[0]);
    Itr->TypeUnion.FileItr.OID = (char (*)[MMAX_LINE])MUMalloc(copySize);
    memcpy(Itr->TypeUnion.FileItr.OID,OID,copySize);
    }

  Itr->TypeUnion.FileItr.StartTime = StartTime;
  Itr->TypeUnion.FileItr.CurTime = StartTime;
  Itr->TypeUnion.FileItr.EndTime = EndTime;
  Itr->TypeUnion.FileItr.FileBuf = NULL;
  Itr->Type = meiFile;

  return(SUCCESS);
  }  /* END MSysEFileItrInit() */





/**
 * @see MSysQueryEvents - parent Populates an iterator for file-based events.
 *
 * @param StartTime (I)
 * @param EndTime   (I)
 * @param EID       (I) [optional,terminated w/-1]
 * @param EType     (I) [optional,terminated w/mrelLAST]
 * @param OType     (I) [optional,terminated w/mxoLAST]
 * @param OID       (I) [optional]
 * @param Itr       (O)
 */

int MSysQueryEventsFile(

  mulong                     StartTime,
  mulong                     EndTime,
  int                       *EID,
  enum MRecordEventTypeEnum *EType,
  enum MXMLOTypeEnum        *OType,
  char                       OID[][MMAX_LINE],
  mevent_itr_t              *Itr)

  {
  if (Itr == NULL)
    {
    return(FAILURE);
    }

  return(MSysEFileItrInit(Itr,StartTime,EndTime,EID,EType,OType,OID));
  }





/**
 * Render an event to XML
 *
 * @param EventID     (I)
 * @param EType       (I)
 * @param EventOType  (I)
 * @param EventETime  (I)
 * @param EventOID    (I) 
 * @param Name        (I) 
 * @param Description (I) [optional]
 * @param EOut        (O)
 */

int MEventToXML(

  int                       EventID,
  enum MRecordEventTypeEnum EType,
  enum MXMLOTypeEnum        EventOType,
  int unsigned              EventETime,
  char                     *EventOID,
  char                     *Name,
  char                     *Description,
  mxml_t                  **EOut)

  {
  /* matching event located - event passes all filters */

  mxml_t *EE = NULL;
  char EventTypeStr[MMAX_LINE];
  char EventMsg[MMAX_LINE];
  char *ptr;

  EventMsg[0] = '\0';

  if (Description != NULL)
    {
    ptr = (char *)strstr(Description,"MSG=\'");
  
    if (ptr != NULL)
      {
      /* FORMAT:  ... MSG='...' */
  
      /* @see MOWriteEvent() - formats MSG= component */
  
      ptr += strlen("MSG=\'");
  
      MUStrCpy(EventMsg,ptr,sizeof(EventMsg));
  
      ptr = strrchr(EventMsg,'\'');
  
      if (ptr != NULL)
        *ptr = '\0';
      }
    else
      {
      MUStrCpy(EventMsg,Description,sizeof(EventMsg));
      }
    }

  MXMLCreateE(EOut,"event");

  EE = *EOut;

  if (EventETime > 0)
    MXMLSetAttr(EE,"etime",(void **)&EventETime,mdfLong);

  if (EventID >= 0)
    MXMLSetAttr(EE,"eid",(void **)&EventID,mdfInt);

  if (EType != mrelGEvent)
    {
    MUStrCpy(EventTypeStr,(char *)MRecordEventType[EType],sizeof(EventTypeStr));
    }
  else
    {
    snprintf(EventTypeStr,sizeof(EventTypeStr),"%s.%s",
      MRecordEventType[EType],
      EventMsg);
    }

  MXMLSetAttr(EE,"otype",(void **)MXO[EventOType],mdfString);

  MXMLSetAttr(EE,"oid",(void **)EventOID,mdfString);

  MXMLSetAttr(EE,"name",(void **)Name,mdfString);

  if (EventMsg[0] != 0)
    MXMLSetAttr(EE,"details",(void **)EventMsg,mdfString);

  MXMLSetAttr(EE,"etype",(void **)EventTypeStr,mdfString);

  /* NOTE:  for generic events, gevent is EventEType.EventMsg */

  switch (EType)
    {
    case mrelRsvStart:
    case mrelRsvEnd:

      /* report alloc nodelist */

      {
      char tmpAllocList[MMAX_BUFFER + 1];

      char *tmpPtr = NULL;
      char *endPtr = NULL;
      char  tmpChar = '\0';
      /* Event is in wiki format, search for AllocNodeList (sync w/MRsvToWikiString) */

      /* MWikiRsvAttr[mwraAllocNodeList] */

      tmpPtr = Description;

      while(TRUE)
        {
        tmpPtr = (char *)MUStrStr(tmpPtr,(char *)MWikiRsvAttr[mwraAllocNodeList],0,FALSE,FALSE);

        if (tmpPtr == NULL)
          break;

        tmpPtr += strlen(MWikiRsvAttr[mwraAllocNodeList]);

        if (*tmpPtr == '=')
          break;
        }

      if ((tmpPtr == NULL) || (*tmpPtr == '='))
        break;

      while(*tmpPtr == '=')
        {
        tmpPtr++;
        }

      endPtr = tmpPtr;
      while (!isspace(*endPtr) && (*endPtr != '\0'))
        {
        endPtr++;
        }

      /* endPtr is now pointing at the first space or end of the string,
          and tmpPtr is at the beginning of the value */

      if (*endPtr != '\0')
        {
        /* Terminate the string for the copy */
        tmpChar = *endPtr;
        *endPtr = '\0';
        }

      MUStrCpy(tmpAllocList,tmpPtr,MMAX_BUFFER + 1);

      if (tmpChar != '\0')
        {
        *endPtr = tmpChar;
        }

      MXMLSetAttr(EE,"allocnodes",(void **)tmpAllocList,mdfString);
      }  /* END BLOCK (case mrelRsvStart) */

      break;

    case mrelJobStart:
    case mrelJobComplete:
    case mrelJobFailure:

      /* report alloc nodelist */

      {
      char tmpAllocList[MMAX_BUFFER + 1];

      if (MSched.WikiEvents == TRUE)
        {
        char *tmpPtr = NULL;
        char *endPtr = NULL;
        char  tmpChar = '\0';
        /* Event is in wiki format, search for taskmap (sync w/MJobToWikiString) */

        /* MWikiJobAttr[mwjaTaskMap] */

        tmpPtr = Description;

        while(TRUE)
          {
          tmpPtr = MUStrStr(tmpPtr,(char *)MWikiJobAttr[mwjaTaskMap],0,FALSE,FALSE);

          if (tmpPtr == NULL)
            break;

          tmpPtr += strlen(MWikiJobAttr[mwjaTaskMap]);

          if (*tmpPtr == '=')
            break;
          }

        if (tmpPtr == NULL)
          break;


        while(*tmpPtr == '=')
          {
          tmpPtr++;
          }

        endPtr = tmpPtr;
        while (!isspace(*endPtr) && (*endPtr != '\0'))
          {
          endPtr++;
          }

        /* endPtr is now pointing at the first space or end of the string,
            and tmpPtr is at the beginning of the value */

        if (*endPtr != '\0')
          {
          /* Terminate the string for the copy */
          tmpChar = *endPtr;
          *endPtr = '\0';
          }

        MUStrCpy(tmpAllocList,tmpPtr,MMAX_BUFFER + 1);

        if (tmpChar != '\0')
          {
          /* Un-terminate the string */
          *endPtr = tmpChar;
          }
        } /* END if (MSched.WikiEvents == TRUE) */
      else
        {
        /* NOTE:  AllocHosts is 37th field in object specific section (sync w/MJobToTString) */
        /* HEADER-------------------------| --> */
        if (Description != NULL)
          {
          sscanf(Description,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %65536s",
            tmpAllocList);
          }
        else
          {
          /* No description, so default to empty string in the result */
          tmpAllocList[0] = '\0';  
          }
        }


      MXMLSetAttr(EE,"allocnodes",(void **)tmpAllocList,mdfString);
      }  /* END BLOCK (case mrelJobStart) */

      break;

    case mrelJobModify:

      {
      char *tptr;

      char *VPtr;
      char *APtr;
      char *tmpTokPtr = NULL;

      /* @see MOWriteEvent() - formats MSG= component */

      if (Description != NULL)
        {
        tptr = strstr(Description,"MSG=");
        }
      else
        {
        tptr = NULL;
        }

      if (tptr != NULL)
        {
        tptr += strlen("MSG='");

        /* FORMAT:  <ATTR>[.<SUBATTR>]=<VALUE> */

        APtr = MUStrTok(tptr,"=",&tmpTokPtr);
        VPtr = MUStrTok(NULL,"= \t\n",&tmpTokPtr);
        
        if (APtr != NULL)
          MXMLSetAttr(EE,"modattr",(void **)APtr,mdfString);

        if (VPtr != NULL)
          MXMLSetAttr(EE,"modvalue",(void **)VPtr,mdfString);
        }
      }  /* END BLOCK (case mrelJobModify) */

      break;

    case mrelGEvent:

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (EIndex) */

  /* NOTE:  add object specific details (ie jobmodify attribute and node allocation) */

  return(SUCCESS);
  }  /* END MEventToXML() */






/**
 * This function can be used by dying threads to clean-up mutexes they hold. 
 *
 * This function can be made a "clean-up" function by calling
 * the pthread_cleanup_push() and its matching pthread_cleanup_pop(). When
 * done in this fashion, this function will be guaranteed to execute before
 * a thread is killed or is canceled, thus ensuring that no deadlocks can
 * occur because the thread is holding a mutex when it dies.
 *
 * @param Arg (not used)  This parameter is not used for anything--just use NULL
 */

void MSysThreadDestructor(

  void *Arg)  /* not used */

  {
#ifdef MTHREADSAFE

  /* make sure we unlock any mutexes that we may have locked down to
   * prevent deadlocks with other threads */

  MUMutexUnlock(&MLogMutex);
  /* should we also unlock the MCommMutex? */

#endif /* MTHREADSAFE */

  return;
  }  /* END MSysThreadDestructor() */



/**
 * This function is a handler called by pthread_atfork()
 * to handle unlocking mutexes in children after the fork.
 * (It is given as the "child" argument to pthread_atfork()).
 */

void MSysPostFork()

  {
#ifdef MTHREADSAFE

  /* destroy and recreate to ensure that the mutex is unlocked
   * and ready to use (we just can't unlock, because calling thread
   * may not have the mutex) */

  pthread_mutex_destroy(&MLogMutex);
  pthread_mutex_init(&MLogMutex,NULL);

#endif /* MTHREADSAFE */

  return;
  }  /* END MSysPostFork() */




/**
 * This function will return SUCCESS if we are able to sucessfully become the primary
 * Moab in an HA environment. Note that this function should work with file
 * locking, but also has a "backup" mechanism that should operate (although less
 * optimally) if locking isn't working.
 */

int MSysAcquireHALock()

  {
  mbool_t UseFLock = FALSE;
  mbool_t FilePossession = FALSE;

  char MutexLockFile[MMAX_NAME];
  int  MutexLockFD = -1;

  int NumChecks = 0;
  mbool_t FileIsMissing = FALSE;
  struct stat StatBuf;

  if (MSched.Iteration <= 0)
    {
    /* starting up--go into lock mode to prevent race conditions
     * with another Moab starting up in the case that the lock
     * file already exists */

    UseFLock = TRUE;
    }

  __MSysGetMutexLockFile(MutexLockFile,sizeof(MutexLockFile));

  while (!FilePossession)
    {
    if (NumChecks > 0)
      {
      MUSleep(MDEF_USPERSECOND*MSched.HALockCheckTime,TRUE);

      /* we only lock mutex file 1) on the first try or 2) if the below check in MSysIsHALockFileValid() fails */
      /* we also must ensure that each time we go around the loop we are releasing the mutex (if we have
       * one) */

      UseFLock = FALSE;
      if (MutexLockFD > 0)
        close(MutexLockFD);
      }

    NumChecks++;

    if (MSysIsHALockFileValid() == FALSE)
      {
      /* the mount has failed or the directory has been removed! */

      UseFLock = TRUE;
      }

    if (UseFLock == TRUE)
      {
      /* try to get a filesystem lock on the "mutex" file */

      while (MFUAcquireLock(MutexLockFile,&MutexLockFD,"HA") == FAILURE)
        {
        MDB(8,fSTRUCT) MLog("INFO:    could not acquire HA flock--trying again in 1 second\n");

        MUSleep(MDEF_USPERSECOND,TRUE);
        }
      }

    /* check if lock file exists */

    if (stat(MSched.HALockFile,&StatBuf) != -1)
      {
      /* file DOES exist--check time */

      FileIsMissing = FALSE;

      if (((mulong)StatBuf.st_mtime > MSched.Time) ||
          (MSched.Time - (mulong)StatBuf.st_mtime) < MSched.HALockCheckTime)
        {
        /* someone else probably has the lock */

        continue;
        }

      /* update the file to mark it as ours */

      utime(MSched.HALockFile,NULL);

      FilePossession =  TRUE;
      }
    else
      {
      /* file doesn't exist--wait required amount of time and check again */

      if (FileIsMissing == FALSE)
        {
        FileIsMissing = TRUE;

        if (MSched.Iteration > 0)
          continue;  /* if we are starting up, don't try and block */
        }
      
      /* this is not the first time the file has been missing--we are
       * probably safe to create it! */

      MSched.HALockFD = open(MSched.HALockFile,O_CREAT|O_EXCL|O_RDONLY,0600);

      if (MSched.HALockFD < 0)
        {
        /* could not create the lock file! */

        MOSSyslog(LOG_WARNING,"could not create HA lock file '%s'--errno %d:%s",
          MSched.HALockFile,
          errno,
          strerror(errno));

        continue;
        }

      FilePossession = TRUE;
      }

    if (FilePossession == TRUE)
      {
      /* start heartbeat thread */

      MSysStartUpdateHALockThread();
      }

    if (UseFLock == TRUE)
      close(MutexLockFD);  /* unlock file mutex */
    }  /* END while (!FilePossession) */

  /* we have the file lock--go ahead and log this fact */

  MSysRegEvent("starting Moab server--HA lock has been acquired",mactMail,1);

  if (MSched.UseSyslog == TRUE)
    {
    MOSSyslog(LOG_INFO,"starting Moab server--HA lock has been acquired");
    }

  return(SUCCESS);
  }  /* END MSysAcquireHALock() */


/**
 * "Touch" the lockfile so that if locks are failing
 * other processes can see we still have possession
 * of the file. Also, we need to check that we can still
 * acccess the file.
 *
 * @return FAILURE if we do not have possession of the lock file
 * anymore!
 */

void *MSysUpdateHALockThread(

  void *Arg)

  {
  char EMsg[MMAX_LINE];

  int rc = 0;
  struct stat statbuf;
  struct utimbuf timebuf;
  static long LastModifyTime = 0;

  if (MSched.HALockFile[0] == '\0')
    {
    /* locking HA not enabled */

    return(NULL);
    }

  EMsg[0] = '\0';

  while (TRUE)
    {
    MUSleep(MDEF_USPERSECOND*MSched.HALockUpdateTime,FALSE);

    MUMutexLock(&EUIDMutex);  /* don't update while we have wrong euid() */

    if (stat(MSched.HALockFile,&statbuf) != -1)
      {
      /* check to make sure that no other process has modified this file
       * since the last time we did */

      if ((LastModifyTime > 0) && (LastModifyTime != statbuf.st_mtime))
        {
        snprintf(EMsg,sizeof(EMsg),"update time changed unexpectedly");

        rc = -1;
        }
      else
        {
        /* no one has touched this file since we last did--continue */

        LastModifyTime = time(NULL);
        timebuf.actime = LastModifyTime;
        timebuf.modtime = LastModifyTime;

        rc = utime(MSched.HALockFile,&timebuf);
        }
      }
    else
      {
      snprintf(EMsg,sizeof(EMsg),"could not stat file");

      rc = -1;
      }

    MUMutexUnlock(&EUIDMutex);

    /* NOTE: MSched.HALockFile is emptied out when we delete the file during shutdown */

    if ((rc == -1) && !MUStrIsEmpty(MSched.HALockFile))
      {
      char ErrorString[MMAX_LINE];
      /* error occurred--immediate shutdown needed */

      if (errno != 0)
        {
#ifndef MNOREENTRANT
        strerror_r(errno,ErrorString,sizeof(ErrorString));

        MOSSyslog(LOG_WARNING,"could not update HA lock file '%s' in heartbeat thread (%s - errno %d:%s)",
          MSched.HALockFile,
          EMsg,
          errno,
          ErrorString);
#else
        MOSSyslog(LOG_WARNING,"could not update HA lock file '%s' in heartbeat thread (%s)",
          MSched.HALockFile,
          EMsg);
#endif
        }
      else
        {
        MOSSyslog(LOG_WARNING,"could not update HA lock file '%s' in heartbeat thread (%s)",
          MSched.HALockFile,
          EMsg);
        }

      MSysRestart(0);
      }
    }

  /*NOTREACHED*/
  return(NULL);
  }  /* END MSysUpdateHALockThread() */




int MSysStartUpdateHALockThread()    

  {
#if !defined(MTHREADSAFE)
  /* not compiled with threads */

  MDB(3,fALL) MLog("WARNING:  cannot create HA update thread - pthreads not enabled\n");

  return(FAILURE);
#else /* !defined(MTHREADSAFE) */

  pthread_t HALockThread;
  pthread_attr_t HALockThreadAttr;

  int rc;

  const char *FName = "MSysStartUpdateHALockThread";

  MDB(6,fALL) MLog("%s()\n",
    FName);

  pthread_attr_init(&HALockThreadAttr);

  /* int pthread_create(pthread_t * thread, pthread_attr_t * attr, void * (*start_routine)(void *), void * arg); */

  rc = pthread_create(&HALockThread,&HALockThreadAttr,MSysUpdateHALockThread,NULL);

  if (rc != 0)
    {
    /* error creating thread */

    MDB(0,fALL) MLog("ERROR:    cannot create HA update thread (error: %d '%s')\n",
      rc,
      strerror(rc));
    
    fprintf(stderr,"ERROR:    cannot create HA update thread (error: %d ' %s')\n",
      rc,
      strerror(rc));

    return(FAILURE);
    }

  MDB(4,fALL) MLog("INFO:     new HA update thread created\n");
#endif /* !defined(MTHREADSAFE) */

  return(SUCCESS);
  }  /* END MSysStartUpdateHALockThread() */



/**
 * Insert a job into the submit job queue, retreive back a unique ID.
 *
 * @param JE
 */

int MSysAddJobSubmitToJournal(

  mjob_submit_t *JE)

  {
  mstring_t JString(MMAX_LINE);

  MXMLToMString(JE->JE,&JString,NULL,TRUE);

  MCPSubmitJournalAddEntry(&JString,JE);

  return(SUCCESS);
  }  /* END MSysAddJobSubmitToJournal() */




char *MSysShowBuildInfo(void)

  {
  static char Buf[MMAX_LINE];

  char *BPtr;
  int   BSpace;

  MUSNInit(&BPtr,&BSpace,Buf,sizeof(Buf));

  #ifdef __MSLURM
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (Buf[0] != '\0') ? "," : "",
      "SLURM");
  #endif /* __MSLURM */

  #ifdef __MSMALLMEM
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (Buf[0] != '\0') ? "," : "",
      "SMALLMEM");
  #endif /* __MSMALLMEM */

  #ifdef MTHREADSAFE
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (Buf[0] != '\0') ? "," : "",
      "THREADSAFE");
  #endif /* MTHREADSAFE */

  #ifdef __M64
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (Buf[0] != '\0') ? "," : "",
      "64");
  #endif /* __M64 */

  #ifdef __MCOMMTHREAD
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (Buf[0] != '\0') ? "," : "",
      "MCOMMTHREAD");
  #endif /* __MCOMMTHREAD */

  #ifdef __MCPA
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (Buf[0] != '\0') ? "," : "",
      "CPA");
  #endif /* __MCPA */

  #ifdef __AIX
    MUSNPrintF(&BPtr,&BSpace,"%s%s",
      (Buf[0] != '\0') ? "," : "",
      "AIX");
  #endif /* __AIX */

  return(Buf);
  }  /* END MSysShowBuildInfo() */






/**
 * Dynamically reallocate node table and supporting tables
 *
 * @param MaxNode (I)
 *
 */

int MSysAllocNodeTable(

  int MaxNode)  /* I */

  {
  if ((MaxNode == MSched.M[mxoNode]) && (MNode != NULL))
    {
    return(SUCCESS);
    }

  if (MNode != NULL)
    {
    MUFree((char **)&MNode);
    }

  MNode = (mnode_t **)MUCalloc(1,sizeof(mnode_t *) * MaxNode + 1);

  MNodeIndex = (int *)MUCalloc(1,sizeof(int) * ((MaxNode << 2) + MMAX_NHBUF));

  MSched.JobMaxNodeCount = MaxNode;
  MSched.M[mxoNode] = MaxNode;

  if ((MNode == NULL) || (MNodeIndex == NULL))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSysAllocJobTable() */








/**
 * Dynamically reallocate job table and supporting tables
 *
 * @param MaxJob (I)
 *
 * @see MSysAllocCJobTable() - peer
 * @see MJobFreeTable() - peer
 */

int MSysAllocJobTable(

  int MaxJob)  /* I */

  {
  MUHTClear(&MAQ,FALSE,NULL);

  MUFree((char **)&MFQ);
  MUFree((char **)&MUIQ);
  MUFree((char **)&MUILastQ);

  MUFree((char **)&MGlobalHQ);
  MUFree((char **)&MGlobalSQ);

  MFQ = (mjob_t **)MUCalloc(
    1,
    sizeof(mjob_t *) * (MaxJob + 1));

  MUIQ = (mjob_t **)MUCalloc(
    1,
    sizeof(mjob_t *) * (MaxJob + 1));

  MUILastQ = (mjob_t **)MUCalloc(
    1,
    sizeof(mjob_t *) * (MaxJob + 1));

  MGlobalHQ = (mjob_t **)MUCalloc(
    1,
    sizeof(mjob_t *) * (MaxJob + 1));

  MGlobalSQ = (mjob_t **)MUCalloc(
    1,
    sizeof(mjob_t *) * (MaxJob + 1));

  if ((MFQ == NULL) ||
      (MUIQ == NULL) ||
      (MUILastQ == NULL) ||
      (MGlobalHQ == NULL) ||
      (MGlobalSQ == NULL))
    {
    MDB(0,fSCHED) MLog("ALERT:    Unable to allocate memory\n");

    return(FAILURE);
    }

  MSched.T[mxoJob] = NULL;
  MSched.Size[mxoJob] = sizeof(mjob_t);
  MSched.M[mxoJob] = MaxJob;
  MSched.E[mxoJob] = NULL;

  MUIQ[0]     = NULL;
  MUILastQ[0] = NULL;

  /* correct segfault in ticket Moab-3410 */
  MFQ[0] = NULL;

  return(SUCCESS);
  }  /* END MSysAllocJobTable() */





/**
 * Dynamically reallocate completed job table.
 */

int MSysAllocCJobTable(void)

  {
  return(SUCCESS);
  }  /* END MSysAllocCJobTable() */



/**
 * Attempts to retrieve the last event ID from the events files
 *
 * @param Out (O) gets the event ID on success, 0 on failure
 */

int MSysGetLastEventIDFromFile(

  int *Out) /* O gets the event ID on success, 0 on failure */

  {
  char LatestFile[MMAX_LINE] = {0};
  int rc;
  char *ptr;

  *Out = 0;

  MStatGetFullName(MSched.Time,LatestFile);

  if (MUFileExists(LatestFile))
    {
    char tmpETime[MMAX_LINE];
    long int CurPos;
    int unsigned EventID;
    FILE* FP;
    mbool_t hitColon = FALSE;
    char *ResultPtr;

    FP = fopen(LatestFile,"r");

    if (FP == NULL)
      {
      return(FAILURE);
      }

    fseek(FP,0,SEEK_END);
    CurPos = ftell(FP);

    if (CurPos == 0) /*file is empty */
      {
      fclose(FP);

      return(SUCCESS);
      }

    /** scan from the back of the file */

    while (CurPos > 0)
      {
      char c = fgetc(FP);

      /* if we hit a colon, we assume we found a line with an event ID.
       * This can be fooled if the MSG='%s' string contains both a colon and
       * a newline.
       */

      if (c == ':')
        hitColon = TRUE;

      /* if we hit a newline, we know we are now one past the end of a line,
        and if we hit a colon, we can assume that we are now in front of a line
        with an event ID
       */
      if (hitColon && c == '\n')
        break;

      CurPos--;
      /*we have to move back by two because fgetc moves us forward one */
      fseek(FP,-2,SEEK_CUR);

      }

    rc = fscanf(FP,"%*s %1024s",
      tmpETime);

    fclose(FP);

    if (rc != 1)
      {
      return(FAILURE); /* Scan failed */
      }

    /* tmpETime should have the format <timestamp>:<EID> */
    ptr = strstr(tmpETime,":");

    if (ptr == NULL)
      {
      return(FAILURE); /* we didn't find what we were expecting in the scan */
      }

    EventID = strtoul(ptr + 1,&ResultPtr,10);

    if (ResultPtr == ptr)
      {
      return(FAILURE); /*what followed the colon wasn't a number */
      }

    *Out = EventID; 

    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MSysGetLastEventIDFromFile() */





/**
 * Ensures that the event counter on Scheduler is at least 1 greater than
 * the ID of the last event written to disk
 *
 * @param Scheduler
 */

int MSysGetLastEventID(

  msched_t *Scheduler)

  {
  int lastID;
  int rc;

  if (Scheduler->MDB.DBType != mdbNONE)
    {
    rc = MDBGetLastID(&Scheduler->MDB,"Events","ID",&lastID,NULL);
    }
  else
    {
    rc = MSysGetLastEventIDFromFile(&lastID);
    }

  if ((rc == SUCCESS) && (MSched.EventCounter <= lastID))
    MSysUpdateEventCounter(lastID);

  return(rc);
  }

/**
 * Ensures that the event counter on Scheduler is at least 1 greater than
 * the ID of the last event written to disk and ensures it won't overflow
 *
 * @param lastID
 */

int MSysUpdateEventCounter(

  int lastID)

  {
  int newValue = lastID++;

  /* if it wrapped, start again at 1 */

  if (newValue > lastID)
    MSched.EventCounter = 1;
  else
    MSched.EventCounter = lastID;

  return SUCCESS;
  }




#ifdef __MCPA
#include "cpalib.h"
#endif /* __MCPA */

int MSysCheckAllocPartitions(void)

  {
#ifdef __MCPA
  /* load cpa partition list */

  /* walk through all partitions */

  /* if partition is batch and has job record and there is no associated
     job, log discrepancy and notify admin or log discrepancy and remove
     stale partition */

  /* also, should track externally created partitions and manage them as
     jobs via XT3 tools */

  /* NYI */
#endif /* __MCPA */

  return(SUCCESS);
  }  /* END MSysCheckAllocPartitions() */



mbool_t MSysUICompressionIsDisabled(void)

  {
  return(MSched.DisableUICompression);
  }  /* END MSysUICompressionIsDisabled() */



/* create job from vpc

          (MJobAllocBase(&P->PowerJ,NULL) == FAILURE))

      if (P->PowerJ->NodeList == NULL)
        {
        MJobAllocNL(P->PowerJ,0,&P->PowerJ->NodeList);
        }

      if (P->PowerJ->Req[0]->NodeList == NULL)
        {
        MJobAllocNL(P->PowerJ,0,&P->PowerJ->Req[0]->NodeList);
        }
*/


/**
 * Verify text buffer is 'safe'.
 *
 * safe is defined as 'printable' characters
 *
 * @param Buf (I)
 * @param BufSize (I)
 * @param FName (I)
 * @param OName (I)
 */

int MSysVerifyTextBuf(

  const char       *Buf,
  int         BufSize,
  const char *FName,
  char       *OName)

  {
  int bindex = 0;

  for (bindex = 0;bindex < BufSize;bindex++)
    {
    if (Buf[bindex] == '\0')
      break;

    if (isprint(Buf[bindex]) == 0)
      {
      char tmpLine[MMAX_LINE];
      int StartIndex;
      int EndIndex;
      int NumBytes;

      /* Copy up to 20 chars before and up to 20 chars after the corruption so we can log enough of the
       * context to tell where the corruption occurs. */

      StartIndex = MAX(0,bindex - 20);
      EndIndex   = MIN(BufSize -1,bindex + 20);
      NumBytes = EndIndex - StartIndex;

      strncpy(tmpLine,Buf + StartIndex,NumBytes);
      tmpLine[NumBytes + 1] = '\0';  /* Make sure that the string is terminated */

      MDB(2,fSTRUCT) MLog("ALERT:    text buffer '%.32s...' of size %d is corrupt at index %d for object '%s' in routine '%s' with value %d in context '%s'\n",
        Buf,
        BufSize,
        bindex,
        OName,
        FName,
        Buf[bindex],
        tmpLine);

      snprintf(tmpLine,sizeof(tmpLine),"text buffer '%.32s...' is corrupt at index %d for object '%s' in routine '%s'",
        Buf,
        bindex,
        OName,
        FName);

      MMBAdd(
        &MSched.MB,
        tmpLine,
        (char *)"N/A",
        mmbtOther,
        0,
        1,
        NULL);

      return(FAILURE);
      }  /* END if (isprint(Buf[bindex]) == 0) */
    }    /* END for (bindex) */
   
  return(SUCCESS);
  }  /* END MSysVerifyTextBuf() */





/**
 * Checks to see if MSched.HALockFile is defined, exists (or can be created), 
 * and can be opened, etc.
 *
 * @return FALSE if the file is inaccesible or misconfigured for Moab.
 */

int MSysIsHALockFileValid()

  {
  char LockDir[MMAX_PATH_LEN];
  char ErrorString[MMAX_LINE];
  struct stat Stat;
  mbool_t GoodPermissions = FALSE;

  if (MSched.HALockFile[0] == '\0')
    {
    return(FALSE);
    }

  MUExtractDir(MSched.HALockFile,LockDir,sizeof(LockDir));

  if (stat(LockDir,&Stat) == -1)
    {
    /* stat failed */

#ifndef MNOREENTRANT
    strerror_r(errno,ErrorString,sizeof(ErrorString));

    MOSSyslog(LOG_WARNING,"could not stat HA lock directory '%s'--errno %d:%s",
      LockDir,
      errno,     
      ErrorString);
#else
    MOSSyslog(LOG_WARNING,"could not stat HA lock directory '%s'",
      LockDir);
#endif

    return(FALSE);
    }

  /* directory must be owned by the Moab user and must have read/write/exec permissions */

  if ((Stat.st_uid == getuid()) &&
      (Stat.st_mode & (S_IRUSR | S_IWUSR | S_IXUSR)))
    {
    /* we can write to this directory */

    GoodPermissions = TRUE;
    }
  else if ((Stat.st_gid == getgid()) &&
      (Stat.st_mode & (S_IRGRP | S_IWGRP | S_IXGRP)))
    {
    GoodPermissions = TRUE;
    }
  else if (Stat.st_mode & (S_IROTH | S_IWOTH | S_IXOTH))
    {
    GoodPermissions = TRUE;
    }

  if (GoodPermissions == FALSE)
    {
    MOSSyslog(LOG_WARNING,"HA lock directory '%s' has bad permissions (cannot access, need at least 0700)",
      LockDir);
    }
     
  return(GoodPermissions);
  }  /* END MSysIsHALockFileValid() */








/*
 * Stores strings for certain calls so that they may be used by non-blocking calls.
 *
 * WARNING: Only use in one thread (not thread safe) due to static variable.
 *
 *  Currently supported:
 *    mdiag -S -v (NonBlockMDiagS)
 *    mdiag -T    (NonBlockMDiagT)
 *    showq -n    (NonBlockShowqN)
 */

int MSysStoreNonBlockingStrings(void)

  {
  static mulong LastCacheTime = 0;
  mbitmap_t SchedDiagFlags;
  mbitmap_t QueueShowFlags;

  mpar_t *P  = NULL;
  msocket_t S;

  mxml_t *RE;
  mxml_t *Where;

  return(SUCCESS);
#define NONBLOCK_MDIAGT_SIZE ((MMAX_BUFFER << 7) * sizeof(char))
#define NONBLOCK_MDIAGS_SIZE (MMAX_BUFFER * 2 * sizeof(char))

  /* We always cache mdiag -S -v as it is not resource intensive and
   * the info it provides *is* critical and is needed at all times. */

  /**** mdiag -S v ****/
  MUMutexLock(&SaveNonBlockMDiagSMutex);
  bmset(&SchedDiagFlags,mcmVerbose);

  
  MSchedDiag(
    "root",
    &MSched,
    NULL,
    &NonBlockMDiagS,
    mfmHuman,
    &SchedDiagFlags);

  MUMutexUnlock(&SaveNonBlockMDiagSMutex);

  /* The below commands are not necessarily critical and can be
   * cached less than once per iteration. */

  if (MSched.NonBlockingCacheInterval >= 0)
    {
    /* we need a separate if statement above to avoid NonBlockingCacheInterval
     * from being cast to an unsigned long when used against LastCacheTime. */

    if ((MSched.NonBlockingCacheInterval == 0) ||
        ((long)(MSched.Time - LastCacheTime) < MSched.NonBlockingCacheInterval))
      {
      return(SUCCESS);
      }
    }

  LastCacheTime = MSched.Time;

  /**** showq -n ****/
  MUMutexLock(&SaveNonBlockShowqMutex);

  MXMLDestroyE(&NonBlockShowqN);
  /* Setup the socket and XML for the showq -n command */

  MXMLCreateE(&NonBlockShowqN,"Data");

  MXMLCreateE(&RE, "Request");
  MXMLAddChild(RE,"Where","ALL",&Where);
  MXMLSetAttr(Where,MSAN[msanName],(void *)"partition",mdfString);
  P = &MPar[0];

  MXMLSetAttr(RE,MSAN[msanAction],(void *)MJobCtlCmds[mjcmQuery],mdfString);
  MXMLSetAttr(RE,"actor",(void *)(MSched.Admin[1].UName[0]),mdfString);
  MXMLSetAttr(RE,"cmdline",(void **)"\\STARTshowq\\20-n\\20--noblock",mdfString);

  bmset(&QueueShowFlags,mcalAdmin1);
  bmset(&QueueShowFlags,mcalAdmin5);
  bmset(&QueueShowFlags,mcalGranted);

  MSUInitialize(&S,NULL,0,0);

  MUIQueueShow(
    &S,
    RE,
    &NonBlockShowqN,
    P,
    NULL,
    MSched.Admin[1].UName[0],  
    &QueueShowFlags,
    0); /* Hard-coded flags */

  MSUFree(&S);

  MXMLDestroyE(&RE);

  MUMutexUnlock(&SaveNonBlockShowqMutex);

  return(SUCCESS);
  }  /* END MSysStoreNonBlockingStrings() */



/**
 * Query internal pending actions and report results in XML.
 * All pending actions will be populated as children of the passed in 'E' XML
 * structure.
 *
 * NOTE:  reviews queued, active, and completed workload
 *
 * @see MSysJobToPendingActionXML() - child
 *
 * @param StartTime (I)
 * @param EndTime (I)
 * @param E (O) receives the events as children on success
 */

int MSysQueryPendingActionToXML(

  mulong   StartTime,  /* I */
  mulong   EndTime,    /* I */
  mxml_t  *E)          /* O receives the pending actions as children on success */

  {
  mxml_t *JE;
  mjob_t *J;

  mvm_t *V;

  job_iter JTI;
  mhashiter_t Iter;

  /* loop through all active/pending system jobs */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (J->System == NULL)
      continue;

    if ((J->System->JobType == msjtVMMap) ||
        (J->System->JobType == msjtVMTracking))
      continue;

    if ((StartTime != (mulong)-1) && (J->StartTime < StartTime))
      continue;

    if ((EndTime != (mulong)-1) && (J->StartTime + J->WCLimit > EndTime))
      continue;

    JE = NULL;

    MSysJobToPendingActionXML(J,&JE);

    MXMLAddE(E,JE);
    }  /* END for (J) */

  /* loop through all completed system jobs */

  MCJobIterInit(&JTI);

  while (MCJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (J->System == NULL)
      continue;

    if ((J->System->JobType == msjtVMMap) ||
        (J->System->JobType == msjtVMTracking))
      continue;

    if ((StartTime != (mulong)-1) && (J->StartTime < StartTime))
      continue;

    if ((EndTime != (mulong)-1) && (J->StartTime + J->WCLimit > EndTime))
      continue;

    JE = NULL;

    MSysJobToPendingActionXML(J,&JE);

    MXMLAddE(E,JE);
    }  /* END for (J) */

  /* process/report non-sysjob based pending actions */
  /* we will re-architect pending actions to do this */

  MUHTIterInit(&Iter);

  while (MUHTIterate(&MSched.VMTable,NULL,(void **)&V,NULL,&Iter) == SUCCESS)
    {
    if (MVMToPendingActionXML(V,&JE) == SUCCESS)
      {
      MXMLAddE(E,JE);
      }
    }    /* END while (MUHTIterate() == SUCCESS) */

  return(SUCCESS);
  }  /* END MSysQueryPendingActionToXML() */



/**
 * Checks the incoming parameters to see if they have been 
 * disabled from running from the command line. 
 *  
 * NOTE:  All new params that shouldn't be modified after 
 * initialization should be added to 
 * MCfgParamDontModifyDuringRuntime. 
 *  
 * @see MSysReconfigure - parent 
 */

mbool_t MParamCanChangeAtRuntime(

  const char *paramStr) /* I */

  {
  int ParamIndex;

  /* Get the index of the MCfg for the parameter desired */

  if (MCfgGetIndex(mcoNONE,paramStr,FALSE,(enum MCfgParamEnum *)&ParamIndex) == SUCCESS)
    {
    /* Test the Don't Modify List for the parameter desired */

    if ((MCfg[ParamIndex].PIndex == mcoJobMaxNodeCount) ||
        (MCfg[ParamIndex].PIndex == mcoMaxGMetric) ||
        (MCfg[ParamIndex].PIndex == mcoMaxGRes) ||
        (MCfg[ParamIndex].PIndex == mcoMaxJob) ||
        (MCfg[ParamIndex].PIndex == mcoMaxNode) ||
        (MCfg[ParamIndex].PIndex == mcoMaxRsvPerNode) ||
        (MCfg[ParamIndex].PIndex == mcoStatProcMax) ||
        (MCfg[ParamIndex].PIndex == mcoStatProcMin) ||
        (MCfg[ParamIndex].PIndex == mcoStatProcStepCount) ||
        (MCfg[ParamIndex].PIndex == mcoStatProcStepSize) ||
        (MCfg[ParamIndex].PIndex == mcoStatTimeMax) ||
        (MCfg[ParamIndex].PIndex == mcoStatTimeMin) ||
        (MCfg[ParamIndex].PIndex == mcoStatTimeStepCount) ||
        (MCfg[ParamIndex].PIndex == mcoStatTimeStepSize))
      {
      return(FALSE);
      }
    }

  return(TRUE);
  } /* END MParamCanChangeAtRuntime() */



/**
 * Perform a quick check of the host Moab thinks it is running on and the host it is actually running on.
 *
 */
mbool_t MSysCheckHost(void)
  {
  char RealHostName[MMAX_LINE];

  RealHostName[0] = '\0';

  if (gethostname(RealHostName,sizeof(RealHostName)) == -1)
    {
    return(FALSE);
    }

  if (strcasecmp(RealHostName,MSched.LocalHost))
    {
    return(FALSE);
    }

  return(TRUE);
  }  /* END MSysCheckHost() */


/* END MSys.c */
