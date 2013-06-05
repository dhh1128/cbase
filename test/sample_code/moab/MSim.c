/* HEADER */

/**
 * @file MSim.c
 *
 * Moab Simulation
 */
 
/* Contains:                                                              *
 *   int MSimJobStart(J)                                                  *
 *   int MSimRMGetInfo()                                                  *
 *   int MSimGetResources(Directory,TraceFile,Buffer)                     *
 *   int MSimGetWorkload()                                                *
 *   int MSimJobRequeue(J)                                                *
 *                                                                        */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  
 

int __MSimJobCustomize(mjob_t *);
int __MSimNodeUpdate(mnode_t *);
int __MSimJobUpdate(mjob_t *);


#define MMAX_SIMEVENT  2048

typedef struct msimevent_t {
  mulong             ETime;
  enum MXMLOTypeEnum OType;
  char               OID[MMAX_NAME];
  char               Attribute[MMAX_NAME];
  char               Value[MMAX_LINE];
  } msimevent_t;

msimevent_t MSimEvent[MMAX_SIMEVENT];

int MSimEventIndex = 0;



/**
 * Load workload from trace file and submit jobs to job queue according to job
 * submission policy.
 *
 * @see MSimGetResources() - peer - load simulation resources
 *
 * NOTE:  run every iteration from MRMUpdate()->MSimRMGetInfo() 
 */

int MSimGetWorkload()
 
  {
  job_iter JTI;

  mjob_t  *JTrace[MMAX_JOB_TRACE + 1];
 
  int      jindex;
  int      index;

  long     QueueTime = 0;  /* latest submission detected in current job queue */
  long     NewTime;        /* time to which scheduler time should be set */
  long     TraceTime = 0;  /* effective submission time of current trace record */

  long     RealSubmitTime; /* recorded submit time of current trace */

  mjob_t  *J;

  const char *FName = "MSimGetWorkload";

  MDB(3,fSCHED) MLog("%s()\n",
    FName);

  /* NOTE:  if MSimJobSubmit() is passed -1 as the first arg, 
            it will set job submit time to this value */ 

  NewTime = (MSim.StartTime == 0) ? MSched.Time : MSim.StartTime;
 
  if (MJobTraceBuffer == NULL)
    {
    if ((MJobTraceBuffer = (mjob_t *)MUCalloc(
            1,
            sizeof(mjob_t) * MMAX_JOB_TRACE)) == NULL)
      {
      /* cannot allocate workload trace buffer */
 
      fprintf(stderr,"ERROR:     cannot allocate workload trace buffer, %d (%s)\n",
        errno,
        strerror(errno));
 
      exit(1);
      }
    }    /* END if (MJobTraceBuffer == NULL) */
 
  /* initialize job trace buffer */
 
  jindex = 0;
 
  memset(JTrace,0,sizeof(JTrace));

  /* NOTE:  MSimJobSubmit() will load raw jobs from trace source and populate 
            JTrace[] */

  /* submit jobs */
 
  switch (MSim.JobSubmissionPolicy)
    {
    case msjsNONE:
    case msjsTraceSubmit:
 
      if (MSched.Iteration == -1)
        {
        char DString[MMAX_LINE];

        /* establish initial job queue */
 
        index = 0;
 
        QueueTime = 0;

        /* submit jobs until backlog reaches initial queue depth */
 
        while (index < (int)MSim.InitialQueueDepth)
          {
          if (MSimJobSubmit(
                -1,
                &JTrace[jindex],
                &RealSubmitTime) == FAILURE)
            {
            MDB(3,fSIM) MLog("INFO:     job traces exhausted\n");

            break;
            }

          MDB(3,fSIM) MLog("INFO:     submitted job '%s'\n",
            JTrace[jindex]->Name);
 
          index++;
 
          QueueTime = MAX(QueueTime,RealSubmitTime);
 
          jindex++;
 
          if (jindex >= MMAX_JOB_TRACE)
            break;
          }    /* END while (index < (int)MSim.InitialQueueDepth) */

        TraceTime = QueueTime;

        if (MSched.TimePolicy == mtpReal)
          MSched.TimeOffset = MSched.Time - TraceTime;

        MULToDString((mulong *)&TraceTime,DString);
 
        MDB(5,fSIM) MLog("INFO:     new trace time detected (%s)\n",
          DString);
 
        MDB(2,fSIM) MLog("INFO:     initial queue depth %d set (%d requested)\n",
          index,
          MSim.InitialQueueDepth);
        }    /* END if (MSched.Iteration == -1) */
      else
        {
        while (MSimJobSubmit(
            NewTime,
            &JTrace[jindex],  /* O */
            NULL) == SUCCESS)
          {
          /* submit jobs until backlog threshold reached */

          MDB(3,fSIM) MLog("INFO:     job '%s' submitted\n",
            JTrace[jindex]->Name);
 
          jindex++;
 
          if (jindex >= MMAX_JOB_TRACE)
            break;
          }
        }    /* END else (MSched.Iteration == -1) */
 
      break;
 
    case msjsConstantJob:

      if (MSched.TimePolicy != mtpReal) 
        QueueTime = MSim.StartTime;
      else
        QueueTime = 0;
 
      /* adjust idle job statistics */

      MJobIterInit(&JTI);

      while (MJobTableIterate(&JTI,&J) == SUCCESS)
        {
        if (J->State != mjsIdle)
          continue;

        MPolicyAdjustUsage(J,NULL,NULL,mlIdle,MPar[0].L.IdlePolicy,mxoALL,1,NULL,NULL);
        }   /* END for (J = MJob[0]->Next) */

      jindex = 0;
 
      while (MPar[0].L.IdlePolicy->Usage[mptMaxJob][0] < (mulong)MSim.InitialQueueDepth)
        {
        if (MSimJobSubmit(
              (MSched.Iteration == -1) ? NewTime : -1,
              &JTrace[jindex],
              NULL) == FAILURE)
          {
          MDB(3,fSIM) MLog("INFO:     job traces exhausted\n");
 
          break;
          }

        MPolicyAdjustUsage(JTrace[jindex],NULL,NULL,mlIdle,MPar[0].L.IdlePolicy,mxoALL,1,NULL,NULL);     

        MDB(3,fSIM) MLog("INFO:     job '%s' submitted\n",
          JTrace[jindex]->Name);
 
        if ((MSim.StartTime <= 0) || (MSched.TimePolicy == mtpReal))
          {
          QueueTime = MAX(QueueTime,(long)JTrace[jindex]->SubmitTime);
          }
 
        jindex++;
 
        if (jindex >= MMAX_JOB_TRACE)
          break;
        }   /* END while (MPar[0].IP.Usage[mptMaxJob]) */
 
      if (MSched.Iteration == -1)
        {
        /* set tracetime to time at which <initialqueuedepth> 
           jobs are available */

        TraceTime = QueueTime;
        }
 
      break;
 
    default:
 
      MDB(0,fSIM) MLog("ERROR:    unexpected JobSubmissionPolicy detected (%d)\n",
        MSim.JobSubmissionPolicy);
 
      break;
    }  /* END switch (MSim.JobSubmissionPolicy) */

  if (((MSched.TimePolicy != mtpReal) || (MSched.TimeRatio > 0)) && 
       (MSched.Iteration == -1))
    { 
    NewTime = (MSim.StartTime != 0) ? (long)MSim.StartTime : TraceTime;

    /* adjust SubmitTime/SystemQueueTime to NewTime */
 
    for (jindex = 0;jindex < MMAX_JOB_TRACE;jindex++)
      {
      J = JTrace[jindex];

      if (J == NULL)
        break;
    
      J->ATime           = NewTime;

      J->SubmitTime      = NewTime;
      J->SystemQueueTime = NewTime;
      }   /* END for (jindex) */
    }     /* END if (MSched.TimePolicy != mtpReal) */
 
  if ((NewTime != (long)MSched.Time) && (MSim.StartTime == 0))
    {
    char TString[MMAX_LINE];

    MULToTString(NewTime - MSched.Time,TString);

    MDB(1,fSCHED) MLog("NOTE:     adjusting scheduler time to job submission time (%s)\n",
      TString);

    MMBAdd(&MSched.MB,"scheduler time adjusted for simulation",NULL,mmbtNONE,0,0,NULL);

    MRsvAdjustTime(NewTime - MSched.Time);
 
    MSched.Time = NewTime;

    /* FIXME:  must reinitialize the profile statistics to the new time */
    }  /* END if (NewTime != (long)MSched.Time) */
 
  return(SUCCESS);
  }  /* END MSimGetWorkload() */



/**
 * Submit individual jobs from internal simulation workload cache.
 *
 * @see MSimGetWorkload() - parent
 *
 * @param Now             (I)
 * @param JPtr            (O)
 * @param TraceSubmitTime (I) [optional]
 */

int MSimJobSubmit(
 
  long       Now,
  mjob_t   **JPtr,
  long      *TraceSubmitTime)
 
  {
  mjob_t *tmpJ;
  mjob_t *J;
 
  static mbool_t NoTraces = FALSE;
 
  char       tmpName[MMAX_NAME];

  const char *FName = "MSimJobSubmit";

  MDB(4,fSIM) MLog("%s(%ld,J,%s)\n",
    FName,
    Now,
    (TraceSubmitTime != NULL) ? "TraceSubmitTime" : "NULL");

  if (JPtr != NULL)
    *JPtr = NULL;
 
  /* load job from workload cache */

  if (NoTraces == TRUE)
    {
    /* job queue exhausted */

    return(FAILURE);
    }

  if (MSim.JTIndex >= MSim.JTCount)
    {
    if (strncasecmp(MSim.WorkloadTraceFile,"local",strlen("local")))
      {
      /* load from tracefile */

      MDB(4,fSIM) MLog("INFO:     job trace cache is empty, re-populating\n");

      if (MSimLoadWorkloadCache(
            MSched.CfgDir,
            MSim.WorkloadTraceFile,
            NULL,
            &MSim.JTCount) == FAILURE)  /* JTCount is reset to new quantity read */
        {
        /* job queue exhausted */

        NoTraces = TRUE;

        MSim.AutoStop = TRUE;

        return(FAILURE);
        }

      MSim.TotalJTCount += MSim.JTCount;

      MDB(4,fSIM) MLog("INFO:     job trace cache populated with %d new jobs\n",
       MSim.JTCount);

      MSim.JTIndex = 0;
      }
    else
      {
      static char tmpBuf[MMAX_BUFFER];

      mnl_t NodeList;

      int JCount = 0;

      /* dynamically generate new job traces */

      MNLInit(&NodeList);

      while (1)
        {
        JCount++;

        if (JCount > 1000)
          {
          MDB(0,fSIM) MLog("ERROR:    cannot generate job records (1000 attempts)\n");

          MNLFree(&NodeList);

          return(FAILURE);
          }

        if (MTraceCreateJob(tmpBuf,sizeof(tmpBuf)) == FAILURE)
          {
          continue;
          }

        if (MSimLoadWorkloadCache(
              NULL,
              NULL,
              tmpBuf,
              &MSim.JTCount) == FAILURE)
          {
          /* job queue exhausted */

          NoTraces = TRUE;

          MSim.AutoStop = TRUE;

          MNLFree(&NodeList);

          return(FAILURE);
          }

        MSim.TotalJTCount += MSim.JTCount;

        if (MReqGetFNL(
             &MJobTraceBuffer[0],
             MJobTraceBuffer[0].Req[0],
             &MPar[0],
             NULL,
             &NodeList,  /* O */
             NULL,
             NULL,
             MMAX_TIME,
             0,
             NULL) == SUCCESS)
          {
          /* feasible job created */

          break;
          }

        MNLFree(&NodeList);
        }    /* END while (1) */

      MSim.JTIndex = 0;

      MDB(4,fSIM) MLog("INFO:     refreshed job cache with %d job traces\n",
        MSim.JTCount);

      MNLFree(&NodeList);
      }  /* END else (strcasecmp() */
    }    /* END if (MSim.JTIndex >= MSim.JTCount) */

  tmpJ = &MJobTraceBuffer[MSim.JTIndex];

  if ((tmpJ->Req[0] == NULL) && MReqCreate(tmpJ,NULL,NULL,FALSE) == FAILURE)
    {
    MJobDestroy(&tmpJ);
    
    return(FAILURE);
    }

  if ((Now > 0) && ((long)(tmpJ->SubmitTime + MSched.TimeOffset) > Now))
    {
    /* job release time not reached */

    MJobDestroy(&tmpJ);

    return(FAILURE);
    }
 
  if (tmpJ->EstWCTime <= 0)
    {
    /* set sim execution time if not specified */

    tmpJ->EstWCTime = tmpJ->SpecWCLimit[0];
    }

  if (MUStrIsEmpty(tmpJ->Name))
    {
    /* set job name if not specified */

    MSimJobCreateName(tmpJ->Name,&MRM[0]);
    }

  if (MSim.JTIndex >= MSim.JTCount)
    {
    /* job cache is exhausted */

    MMBAdd(&MSched.MB,"job cache is empty",NULL,mmbtNONE,0,0,NULL);

    NoTraces = TRUE;

    /* pause scheduing when last job trace is processed */

    MSim.AutoStop = TRUE;

    MJobDestroy(&tmpJ);

    return(FAILURE);
    }

  /* process new sim job */

  MJobGetName(tmpJ,NULL,&MRM[0],tmpName,sizeof(tmpName),mjnShortName);

  MUStrCpy(tmpJ->Name,tmpName,sizeof(tmpJ->Name));

  J = NULL;

  if (MJobCreate(tmpJ->Name,TRUE,&J,NULL) == FAILURE)
    {
    /* cannot create new job */

    return(FAILURE);
    }

  /* migrate new job into local structure */

  if (JPtr != NULL)
    *JPtr = J;

  MJobDuplicate(J,tmpJ,FALSE);

  MJobTransferAllocation(J,tmpJ);

  bmset(&tmpJ->IFlags,mjifTemporaryJob);
  
  MJobDestroy(&tmpJ);

  J->ATime = MSched.Time;

  if (MRMJobPostLoad(J,NULL,J->SubmitRM,NULL) == FAILURE)
    { 
    /* job was removed */

    /* NO-OP */
    }

  if (TraceSubmitTime != NULL)
    *TraceSubmitTime = J->SubmitTime;

  if ((Now <= 0) || 
     ((MSched.TimePolicy == mtpReal) && 
      (MSched.TimeRatio <= 0.0) && 
      (MSim.JobSubmissionPolicy != msjsNONE) &&
      (MSim.JobSubmissionPolicy != msjsTraceSubmit)))
    {
    /* adjust job queue time */

    J->SubmitTime = MSched.Time;
    }

  MS3AddLocalJob(J->SubmitRM,J->Name);

  J->SystemQueueTime = J->SubmitTime;

  /* adjust job record */

  MJobSetState(J,mjsIdle);
 
  J->CTime = J->SubmitTime;
  J->MTime = J->SubmitTime;
  J->ATime = J->SubmitTime;

  MDB(5,fSIM) MLog("INFO:     job '%s' submitted\n",
    J->Name);

  MDB(3,fSIM)
    MJobShow(J,0,NULL);

  MSim.JTIndex++;
 
  MSim.QueueChanged = TRUE;
 
  return(SUCCESS);
  }  /* END MSimJobSubmit() */





/**
 *
 *
 * @param JName (O) [minsize=MMAX_NAME]
 * @param R (I)
 */

int MSimJobCreateName(

  char  *JName,  /* O (minsize=MMAX_NAME) */
  mrm_t *R)      /* I */

  {
  if ((JName == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  switch (R->Type)
    {
    case mrmtLL:

      /* FORMAT:  <HOSTID>.<JOBID>.<STEPID> */

      sprintf(JName,"%s.%d.0",
        R->Name,
        MRMJobCounterIncrement(R));

      break;

    case mrmtPBS:
    default:

      /* FORMAT:  <JOBID>.<SERVER> */

      sprintf(JName,"%d.%s",
        MRMJobCounterIncrement(R),
        R->Name);

      break;
    }  /* END switch (R->Type) */

  return(SUCCESS);
  }  /* END MSimJobCreateName() */





/**
 * Load workload simulation traces into internal simulation queue.
 *
 * @see MJobTraceBuffer[] - stores job traces
 * @see MSimJobSubmit() - parent
 *
 * @param Directory (I) [optional]
 * @param TraceFile (I) [optional]
 * @param Buffer (I) [optional]
 * @param TraceCount (O) [optional]
 */

int MSimLoadWorkloadCache(
 
  char *Directory,  /* I (optional) */
  char *TraceFile,  /* I (optional) */
  char *Buffer,     /* I (optional) */
  int  *TraceCount) /* O (optional) */
 
  {
  int    JobCount;
  long   EarliestQueueTime;
 
  int    count;
  int    index;
  int    jindex;
 
  mbool_t Found;
 
  char   Name[MMAX_LINE] = {0};
  char   DString[MMAX_LINE];

  char  *buf = NULL;
  char  *ptr;
 
  char  *head;
  char  *tail;
 
  int    buflen;
 
  mjob_t *J;
  mjob_t *tmpJ = NULL;

  int    Offset;

  const char *FName = "MSimLoadWorkloadCache";

  MDB(3,fSIM) MLog("%s(%s,%s,%.128s,TraceCount)\n",
    FName,
    (Directory != NULL) ? Directory : "NULL",
    (TraceFile != NULL) ? TraceFile : "NULL",
    (Buffer != NULL) ? Buffer : "NULL");

  if (TraceCount != NULL)
    *TraceCount = 0;

  if (Buffer != NULL)
    {
    MUStrDup(&buf,Buffer);

    head = Buffer;

    Offset = 0;
    }
  else
    { 
    if ((TraceFile != NULL) && (TraceFile[0] == '\0'))
      {
      char tmpLine[MMAX_LINE];

      sprintf(tmpLine,"ERROR:  no workload tracefile specified - set %s\n",
        MParam[mcoSimWorkloadTraceFile]);

      MDB(0,fSIM) MLog("%s",
        tmpLine);

      MUStrDup(&MSched.ExitMsg,tmpLine);

      MSysShutdown(0,1);

      /*NOTREACHED*/
      }
 
    if ((TraceFile != NULL) && ((TraceFile[0] == '/') || (TraceFile[0] == '~')))
      {
      strcpy(Name,TraceFile);
      }
    else if ((Directory != NULL) && (TraceFile != NULL))
      {
      if (Directory[strlen(Directory) - 1] == '/')
        {
        sprintf(Name,"%s%s",
          Directory,
          TraceFile);
        }
      else
        {
        sprintf(Name,"%s/%s",
          Directory,
          TraceFile); 
        }
      }
 
    if ((buf = MFULoad(Name,1,macmWrite,&count,NULL,NULL)) == NULL)
      {
      char tmpLine[MMAX_LINE];

      sprintf(tmpLine,"WARNING:  cannot open workload tracefile %s - check file or parameter %s\n",
        Name,
        MParam[mcoSimWorkloadTraceFile]);

      MDB(0,fSIM) MLog("%s",
        tmpLine);

      MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(FAILURE);
      }

    /* set head to first line after marker */

    head = buf + MSim.TraceOffset;

    Offset = MSim.TraceOffset;

    MDB(3,fSIM) MLog("INFO:     reading workload tracefile at %d byte offset\n",
      MSim.TraceOffset);
    }    /* END else (Buffer != NULL) */

 
  /* terminate string at end delimiter */
 
  buflen = strlen(head);
 
  ptr  = head;
  tail = head + buflen;
 
  MDB(6,fSIM) MLog("INFO:     workload buffer size: %d bytes\n",
    buflen);
 
  /* clear workload cache */
 
  for (jindex = 0;jindex < MMAX_JOB_TRACE;jindex++)
    {
    J = &MJobTraceBuffer[jindex];
 
    if (!MJobPtrIsValid(J))
      continue;
 
    MDB(6,fSIM) MLog("INFO:     freeing memory for trace record %d '%s'\n", 
      jindex,
      (J->Name[0] != '\0') ? J->Name : NONE);
 
    bmset(&J->IFlags,mjifTemporaryJob);

    MJobDestroy(&J);
    }  /* END for (jindex) */
 
  memset(MJobTraceBuffer,0,sizeof(MJobTraceBuffer[0]) * MMAX_JOB_TRACE);
 
  /* load job traces */
 
  MDB(3,fSIM) MLog("INFO:     loading job traces\n");
 
  JobCount = 0;
 
  EarliestQueueTime = MMAX_TIME;
 
  while (ptr < tail)
    {
    char tc = '\0';  /* initialize to avoid compiler warning */

    if (tail - ptr > MMAX_BUFFER)
      {
      tc = ptr[MMAX_BUFFER];

      ptr[MMAX_BUFFER] = '\0';
      }

    MJobMakeTemp(&tmpJ);

    if (MTraceLoadWorkload(
          ptr,
          &Offset,
          tmpJ,
          MSched.Mode) == SUCCESS)
      {
      Found = FALSE;
 
      /* search job trace cache */
 
      for (index = 0;index < JobCount;index++)
        {
        if (!strcmp(tmpJ->Name,MJobTraceBuffer[index].Name))
          {
          Found = TRUE;
 
          break;
          }
        }    /* END for (index) */
 
      if ((Found == TRUE) || (MJobFind(tmpJ->Name,&J,mjsmBasic) == SUCCESS))
        {
        if (Found == TRUE)
          { 
          MDB(1,fSIM) MLog("WARNING:  job '%s' previously detected in tracefile (MJobTraceBuffer[%d]/JC: %d  IT: %d)\n",
            tmpJ->Name,
            index,
            JobCount,
            MSched.Iteration);
          }
        else
          {
          MDB(1,fSIM) MLog("WARNING:  job '%s' previously detected in tracefile (Job/JC: %d  IT: %d)\n",
            tmpJ->Name,
            JobCount,
            MSched.Iteration);
          }

        /* NOTE:  duplicate job ignored */

        MSim.RejectJC++;
        MSim.RejReason[marLocality]++;
        }
      else
        {
        mjob_t *JP = tmpJ;

        J = &MJobTraceBuffer[JobCount];
 
        MJobDuplicate(J,JP,FALSE);

        MJobTransferAllocation(J,JP);
 
        /* determine earliest job queue time */
 
        EarliestQueueTime = MIN((long)J->SubmitTime,EarliestQueueTime);
 
        JobCount++;
 
        if (JobCount >= MMAX_JOB_TRACE)
          {
          MDB(1,fSIM) MLog("INFO:     job cache is full\n");
 
          break;
          }
        }    /* END else if (MJobFind(tmpJ->Name,&J,NULL,mjsmBasic) == SUCCESS) */
      }      /* END if (MTraceLoadWorkload() == SUCCESS) */
    else
      {
      if (Offset == 0)
        {
        MDB(6,fSIM) MLog("INFO:     empty buffer detected in %s\n",
          "MTraceLoadWorkload");
        }
      }

    if (tail - ptr > MMAX_BUFFER)
      {
      ptr[MMAX_BUFFER] = tc;
      }
 
    MJobFreeTemp(&tmpJ);

    if (Offset == 0)
      {
      /* corruption detected or buffer empty */

      MDB(6,fSIM) MLog("INFO:     corruption detected in %s\n",
        "MTraceLoadWorkload");
 
      break;
      }
 
    ptr += Offset;
    }        /* END while (ptr < tail) */
 
  if (Offset != 0)
    {
    MSim.TraceOffset += (ptr - head);
    } 

  free(buf);

  if (JobCount == 0)
    {
    MDB(1,fSIM) MLog("WARNING:  no jobs loaded in %s\n",
      FName);

    return(FAILURE);
    }

  MULToDString((mulong *)&EarliestQueueTime,DString);

  MDB(3,fSIM) MLog("INFO:     jobs loaded: %d  earliest start: %ld  %s",
    JobCount,
    EarliestQueueTime,
    DString);
 
  if (TraceCount != NULL)
    *TraceCount = JobCount;
 
  return(SUCCESS);
  }  /* END MSimLoadWorkloadCache() */


int MSimSummarize()
 
  {
  long   SchedRunTime;
  int    IdleQueueSize;
  int    EligibleJobs;
  int    ActiveJC;
  int    UpNodes;
  int    IdleNC;
  int    CompletedJC;
  int    SuccessfulJobsCompleted;
  double TotalProcHours;
  double RunningProcHours;
  double SuccessfulProcHours;
  mulong QueuePS;
  double WeightedCpuAccuracy;
  double CpuAccuracy;
  double XFactor;
  int    Iteration;
  long   RMPollInterval; 
  double SchedEfficiency;
  int    ActiveNC;
  double MinEff;
  int    MinEffIteration;
 
  mpar_t *GP;

  char   tmpBuffer[MMAX_BUFFER];
  char   DString[MMAX_LINE];

  const char *FName = "MSimSummarize";
 
  MDB(1,fSIM) MLog("%s()\n",
    FName);

  GP = &MPar[0];
 
  MDB(1,fSIM) MLog("INFO:     efficiency:  %3d/%-3d  (%.2f percent) [total: %.2f]  jobs: %3d of %3d [total: %d]\n",
    (GP->UpNodes - GP->IdleNodes),
    GP->UpNodes,
    (double)(GP->UpNodes > 0) ? (GP->UpNodes - GP->IdleNodes) / GP->UpNodes * 100.0 : 0.0,
    (double)(MStat.TotalPHAvailable > 0.0) ? MStat.TotalPHBusy / MStat.TotalPHAvailable * 100.0 : 0.0,
    MStat.ActiveJobs,
    MStat.EligibleJobs,
    GP->S.JC);
 
  SchedRunTime            = MSched.Iteration * MSched.MaxRMPollInterval * 100;
  IdleQueueSize           = GP->L.IdlePolicy->Usage[mptMaxJob][0];
  EligibleJobs            = MStat.EligibleJobs;
  ActiveJC                = MStat.ActiveJobs;
  UpNodes                 = GP->UpNodes;
  IdleNC                  = GP->IdleNodes;
  CompletedJC             = GP->S.JC;
  SuccessfulJobsCompleted = MStat.SuccessfulJobsCompleted;
  TotalProcHours          = MStat.TotalPHAvailable;
  RunningProcHours        = MStat.TotalPHBusy;
  SuccessfulProcHours     = MStat.SuccessfulPH;
  QueuePS                 = GP->L.IdlePolicy->Usage[mptMaxPS][0];
  WeightedCpuAccuracy     = GP->S.NJobAcc;
  CpuAccuracy             = GP->S.JobAcc;
  XFactor                 = GP->S.XFactor;
  Iteration               = MSched.Iteration;
  RMPollInterval          = MSched.MaxRMPollInterval; 
  MinEff                  = MStat.MinEff;
  MinEffIteration         = MStat.MinEffIteration;
 
  /* show statistics */
 
  ActiveNC = UpNodes - IdleNC;
 
  MLogNH("\n\n");

  MULToDString(&MSched.Time,DString);

  MDB(0,fSIM) MLog("SUM:  Statistics for iteration %d: (Poll Interval: %ld) %s\n",
    Iteration,
    RMPollInterval,
    DString);

  MULToDString((mulong *)&MStat.InitTime,DString);

  MDB(0,fSIM) MLog("SUM:  scheduler running for %7ld seconds (%6.2f hours) initialized on %s\n",
    SchedRunTime / 100,
    (double)SchedRunTime / 360000,
    DString);
 
  MDB(0,fSIM) MLog("SUM:  active jobs:          %8d  eligible jobs:       %8d  idle jobs:      %5d\n",
    ActiveJC,
    EligibleJobs,
    IdleQueueSize);
 
  if (CompletedJC != 0)
    {
    MDB(0,fSIM) MLog("SUM:  completed jobs:       %8d  successful jobs:     %8d  avg XFactor:    %7.2f\n",
      CompletedJC,
      SuccessfulJobsCompleted,
      XFactor / CompletedJC);
    }
  else
    {
    MDB(0,fSIM) MLog("SUM:  completed jobs:           NONE\n");
    }
 
  if (TotalProcHours > 0.0) 
    {
    SchedEfficiency = RunningProcHours / TotalProcHours;
 
    MDB(0,fSIM) MLog("SUM:  available proc hours:%9.2f  running proc hours: %9.2f  efficiency:     %7.3f\n",
      TotalProcHours,
      RunningProcHours,
      SchedEfficiency * 100);

    MDB(0,fSIM) MLog("SUM:  preempt proc hours:  %9.2f  preempt jobs:      %d         preempt loss:   %7.3f\n",
      (double)MStat.TotalPHPreempted / 3600.0,
      MStat.TotalPreemptJobs,
      (double)MStat.TotalPHPreempted / 3600.0 / TotalProcHours * 100);

    MDB(0,fSIM) MLog("SUM:        min efficiency: %8.2f  iteration:           %8d\n",
      MinEff,
      MinEffIteration);
    }
  else
    {
    SchedEfficiency = 1.0;
 
    MDB(0,fSIM) MLog("SUM:  available proc hours:     <N/A>\n");
    }
 
  MDB(0,fSIM) MLog("SUM:  available nodes:      %8d  active nodes:         %8d  efficiency:     %7.3f\n",
    UpNodes,
    ActiveNC,
    (double)ActiveNC / UpNodes * 100.0);
 
  if (SuccessfulProcHours != 0.0)
    {
    MDB(0,fSIM) MLog("SUM:  wallclock accuracy:    %7.3f  weighted wc accuracy: %7.3f\n",
      (double)CpuAccuracy / CompletedJC * 100.0,
      (double)WeightedCpuAccuracy / SuccessfulProcHours * 100.0 / 36.0);
    }
  else
    {
    MDB(0,fSIM) MLog("SUM:  WC limit accuracy:       <N/A>\n");
    }
 
  if ((TotalProcHours > 0.0) && (CompletedJC > 0) && (UpNodes > 0))
    {
    MDB(0,fSIM) MLog("SUM:  est backlog hours:     %7.3f  avg backlog:         %8.3f\n",
      (double)SchedEfficiency * (CpuAccuracy / CompletedJC) * 
      QueuePS / 3600.0 / UpNodes,
      (double)SchedEfficiency * (CpuAccuracy / CompletedJC) *
      MStat.TQueuedPH / MAX(1,MSched.Iteration) / UpNodes);
    }
  else
    {
    MDB(0,fSIM) MLog("SUM:  estimated backlog: <N/A>\n");
    }

  tmpBuffer[0] = '\0';

  MSchedStatToString(
    &MSched,
    mwpNONE,
    tmpBuffer,
    sizeof(tmpBuffer),
    FALSE);

  MDB(0,fSIM) MLog("SUM:  %s\n",
    tmpBuffer);
 
  /* show configuration summary */
 
  MDB(0,fSIM) MLog("SUM:  scheduler configuration:\n");
 
  MDB(0,fSIM) MLog("SUM:  backfill policy:      %8s (%d)  BFDepth:            %3d\n",
    MBFPolicy[GP->BFPolicy],
    MStat.EvalJC,
    GP->BFDepth);
 
  MDB(0,fSIM) MLog("SUM:  queuedepth:          %9d  submitpolicy: %14s\n",
    MSim.InitialQueueDepth,
    MSimQPolicy[MSim.JobSubmissionPolicy]); 
 
  exit(0);

  /*NOTREACHED*/
 
  return(SUCCESS);
  }  /* END MSimSummarize() */




int MSimSetDefaults()

  {
  const char *FName = "MSimSetDefaults";

  MDB(5,fSIM) MLog("%s()\n",
    FName);

  memset(&MSim,0,sizeof(MSim));  

  /* set default sim values */
 
  MSim.JobSubmissionPolicy  = MDEF_SIMJSPOLICY;
  MSim.InitialQueueDepth    = MDEF_SIMINITIALQUEUEDEPTH;
 
  MSim.AutoShutdown         = MDEF_SIMAUTOSHUTDOWNMODE;
  MSim.AutoStop             = FALSE;

  MSim.PurgeBlockedJobs     = MDEF_SIMPURGEBLOCKEDJOBS;
 
  strcpy(MSim.StatFileName,MDEF_SIMSTATFILENAME);
  strcpy(MSim.WorkloadTraceFile,MDEF_SIMWORKLOADTRACEFILE);

  return(SUCCESS);
  }  /* END MSimSetDefaults() */




/**
 * Report simulation config parameters.
 *
 * @see MUIConfigShow() - parent
 *
 * @param S (I)
 * @param DoVerbose (I)
 * @param String (O)
 */

int MSimShow(
 
  msim_t    *S,
  mbool_t    DoVerbose,
  mstring_t *String)
 
  {
  if ((S == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoSimWorkloadTraceFile],
    S->WorkloadTraceFile);

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoSimAutoShutdown],
    MBool[S->AutoShutdown]);

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoSimPurgeBlockedJobs],
    MBool[S->PurgeBlockedJobs]);

  MStringAppendF(String,"%-30s  %ld\n",
    MParam[mcoSimStartTime],
    S->StartTime);

  MStringAppendF(String,"%-30s  %s\n",
    MParam[mcoSimJobSubmissionPolicy],
    MSimQPolicy[S->JobSubmissionPolicy]);

  MStringAppendF(String,"%-30s  %d\n",
    MParam[mcoSimInitialQueueDepth],
    S->InitialQueueDepth);

  if ((DoVerbose == TRUE) || (MSched.TimePolicy != mtpNONE))
    {
    MStringAppendF(String,"%-30s  %s\n",
      MParam[mcoSimTimePolicy],
      MTimePolicy[MSched.TimePolicy]);

    MStringAppendF(String,"%-30s  %.2f\n",
      MParam[mcoSimTimeRatio],
      MSched.TimeRatio);
    }

  MStringAppend(String,"\n");
 
  return(SUCCESS);
  }  /* END MSimShow() */






/**
 * Process old-style simulation parameters.
 *
 * @param S (I) [modified]
 * @param PIndex (I)
 * @param IVal (I)
 * @param DVal (I)
 * @param SVal (I)
 * @param SArray (I)
 */

int MSimProcessOConfig(

  msim_t *S,       /* I (modified) */
  enum MCfgParamEnum PIndex,  /* I */
  int     IVal,    /* I */
  double  DVal,    /* I */
  char   *SVal,    /* I */
  char  **SArray)  /* I */

  {
  if (S == NULL)
    {
    return(FAILURE);
    }

  switch (PIndex)
    {
    case mcoSimRandomizeJobSize:

      S->RandomizeJobSize = MUBoolFromString(SVal,S->RandomizeJobSize);

      break;

    case mcoSimAutoShutdown:

      S->AutoShutdown = MUBoolFromString(SVal,TRUE);

      break;

    case mcoSimInitialQueueDepth:

      S->InitialQueueDepth = IVal;

      break;

    case mcoSimJobSubmissionPolicy:

      if ((S->JobSubmissionPolicy = (enum MSimJobSubmissionPolicyEnum)MUGetIndexCI(
             SVal,
             MSimQPolicy,
             FALSE,
             S->JobSubmissionPolicy)) == 0)
        {
        MDB(1,fCONFIG) MLog("ALERT:    invalid %s parameter specified '%s'\n",
          MParam[mcoSimJobSubmissionPolicy],
          SVal);
        }

      break;

    case mcoSimPurgeBlockedJobs:

      S->PurgeBlockedJobs = MUBoolFromString(SVal,TRUE);

      break;

    case mcoSimStartTime:

      {
      mulong tmpL;
      char TString[MMAX_LINE];

      if (!strcasecmp(SVal,"now"))
        {
        MUGetTime(&tmpL,mtmNONE,NULL);
        }
      else
        {
        tmpL = MUTimeFromString(SVal);
        }

      MULToTString(tmpL - MSched.Time,TString);

      MDB(1,fSCHED) MLog("NOTE:     adjusting scheduler time to sim start time (%s)\n",
        TString);

      S->SpecStartTime = tmpL;

      if (MSched.Mode == msmSim)
        {
        S->StartTime = tmpL;

        MRsvAdjustTime(tmpL - MSched.Time);

        MSched.Time = tmpL;
        }
      }  /* END BLOCK */

      break;

    case mcoSimTimePolicy:

      MSched.TimePolicy = (enum MTimePolicyEnum)MUGetIndexCI(
        SVal,
        MTimePolicy,
        FALSE,
        MSched.TimePolicy);

      break;

    case mcoSimTimeRatio:

      MSched.TimeRatio = DVal;

      if (MSched.TimePolicy == mtpNONE)
        MSched.TimePolicy = mtpReal;

      break;

    case mcoSimWorkloadTraceFile:

      MUStrCpy(S->WorkloadTraceFile,SVal,sizeof(S->WorkloadTraceFile));

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (PIndex) */

  return(SUCCESS);
  }  /* END MSimProcessOConfig() */



/* END MSim.c */
