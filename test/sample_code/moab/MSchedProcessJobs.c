/* HEADER */

      
/**
 * @file MSchedProcessJobs.c
 *
 * Contains: MSchedProcessJobs
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

/* Forward reference */

int __MSchedScheduleJobs(mpar_t *,mpar_t *[],mjob_t **,mjob_t **,int,int *,mbool_t);


/**
 * Schedules eligible jobs on a per job per partition basis.
 *
 * This function differs from __MSchedSchedulePerPartitionPerJob in that each
 * job looked at individually on each partition. This was done to allow 
 * parallocationpolicy to roundrobin after each job was started. In
 * __MSchedSchedulePerPartitionPerJob looks at all jobs in each partition which
 * make parallocationpolicy a per iteration policy since 10 jobs could fit all
 * fit on one partition before re-prioritizing partition list.
 *
 * The one downside to this way of scheduling is that jobs won't be prioritized
 * per partition because only one job will be considered on a partition at a
 * time.
 *
 * GlobaSQ and GlobalHQ are passed from MSchedProcessJobs which are then passed
 * __MSchedScheduleJobs which assumes the size of the queue arrays are 
 * MSched.M[mxoJob]. MGlobalSQ and MGlobalHQ are allocated at run time using
 * MSched.M[mxoJob] which can be different MMAX_JOB.
 *
 * @see __MSchedSchedulePerPartitionPerJob
 * @see MSchedProcessJobs - Parent
 *
 * @param PList (I) List of partitions to consider.
 * @param GlobalSQ (I) Must be MGlobalSQ because its size is alloc'ed at start time.
 * @param GlobalHQ (I) Must be MGlobalHQ because its size is alloc'ed at start time.
 */

void __MSchedSchedulePerJobPerPartition(

  mpar_t **PList,    /* I */
  mjob_t **GlobalSQ, /* I */
  mjob_t **GlobalHQ) /* I */

  {
  static mjob_t **PrioritizedJobs = NULL;
  int jindex;
  int pindex;
  int copyindex;
  mjob_t *J;
  mpar_t *prioritizedPList[MMAX_PAR + 1];
  mpar_t *PSList[2]; /* one for partition ptr and one for terminator */

  GlobalSQ[1] = NULL; /* Only Storing one in the queue. */
  GlobalHQ[1] = NULL; /* one for job index and one to terminate */
  PSList[1] = NULL;

  /* Don't know the final max size of jobs so must allocate. */
  if ((PrioritizedJobs == NULL) &&
      (MJobListAlloc(&PrioritizedJobs) == FAILURE))
    {
    return;
    }

  /* Prioritize all jobs according to global settings */
  MOQueueInitialize(PrioritizedJobs);
  MQueuePrioritizeJobs(PrioritizedJobs,FALSE,NULL);

  /* Prioritize partitions and attempt to schedule job them. */
  for (jindex = 0;PrioritizedJobs[jindex] != NULL;jindex++)
    {
    J = PrioritizedJobs[jindex];

    /* only transform those jobs which are eligible for scheduling */

    /* TODO: stop once reservation depth is full. */

    if (MJOBISACTIVE(J) ||
        MJOBSISBLOCKED(J->State) ||
        MJOBWASCANCELED(J) ||
        MJOBISCOMPLETE(J) ||
        !bmisclear(&J->Hold))
      continue;

    /* make copy of PList as MParPrioritizeList can remove partitions */
    for (copyindex = 0;PList[copyindex] != NULL;copyindex++)
      prioritizedPList[copyindex] = PList[copyindex];

    prioritizedPList[copyindex] = NULL;

    /* Prioritize partitions according to ParAllocationPolicy. */
    MParPrioritizeList(prioritizedPList,J);

    for (pindex = 0;prioritizedPList[pindex] != NULL;pindex++)
      {
      long tmpSystemPrio;
      mpar_t *tmpPar = prioritizedPList[pindex];

      /* ALL,DEFAULT,SHARED partitions were strippped out by MParPrioritizeList */

      /* transform the creds on each job for the partition we are scheduling */

      /* only transform the credits for jobs which may possibly run in this partition */

      if (bmisset(&J->SpecPAL,tmpPar->Index) == FAILURE)
          continue;

      /* transform creds can change the system prio - save it and restore afterwards */

      tmpSystemPrio = J->SystemPrio;

      MJobPartitionTransformCreds(J,tmpPar);

      J->SystemPrio = tmpSystemPrio;

      /* make a partition list with just this partition to pass into the scheduling routine */

      PSList[0] = tmpPar;
  
      GlobalSQ[0] = J;
      GlobalHQ[0] = J;

      MDB(5,fSCHED) MLog("INFO:     parallocation per partition job (%s) scheduling (%s) in iteration %d\n",
        J->Name,
        tmpPar->Name,
        MSched.Iteration);

      __MSchedScheduleJobs(PSList[0],PSList,GlobalSQ,GlobalHQ,2,NULL,FALSE);
      } /* END for (pindex = 0;prioritizedPList[pindex] != NULL;pindex++) */
    } /* END for (jindex = 0;PrioritizedJobs[jindex] != -1;jindex++) */

  } /* END int __MSchedSchedulePerJobPerPartition() */



/**
 * Schedules jobs by looping through all partitions and looking at every job on
 * each partition.
 *
 * @see __MSchedSchedulePerJobPerPartition
 * @see MSchedProcessJobs - Parent
 *
 * GlobaSQ and GlobalHQ are passed from MSchedProcessJobs which are then passed
 * __MSchedScheduleJobs which assumes the size of the queue arrays are 
 * MSched.M[mxoJob]. MGlobalSQ and MGlobalHQ are allocated at run time using
 * MSched.M[mxoJob] which can be different MMAX_JOB.
 *
 * @param PList (I) List of partitions to consider.
 * @param GlobalSQ (I) Must be MGlobalSQ because its size is alloc'ed at start time.
 * @param GlobalHQ (I) Must be MGlobalHQ because its size is alloc'ed at start time.
 */

void __MSchedSchedulePerPartitionPerJob(

  mpar_t **PList, /* I */
  mjob_t **GlobalSQ, /* I */
  mjob_t **GlobalHQ) /* I */

  {
  job_iter JTI;

  int pindex;
  mpar_t *PSList[2]; /* one for partition ptr and one for terminator */
  mbool_t TransformedJob = FALSE;
    
  PSList[1] = NULL;

  if (MPar[0].PAllocPolicy != mpapNONE)
    {
    MParPrioritizeList(PList,NULL);
    }

  for (pindex = 0;PList[pindex] != NULL;pindex++)
    {
    mpar_t *tmpPar = PList[pindex];
    mjob_t *J;

    /* skip partition 0 - we will do that later after scheduling on each partition */

    if (tmpPar->Index == 0)
      continue;

    if (!strcmp(tmpPar->Name,"ALL"))
      continue;

    if (!strcmp(tmpPar->Name,"SHARED"))
      continue;

    if (!strcmp(tmpPar->Name,"DEFAULT"))
      continue;

    /* transform the creds on each job for the partition we are scheduling */

    MJobIterInit(&JTI);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      long tmpSystemPrio;

      /* only transform those jobs which are eligible for scheduling */

      if (MJOBISACTIVE(J) ||
          MJOBSISBLOCKED(J->State) ||
          MJOBWASCANCELED(J) ||
          MJOBISCOMPLETE(J))
        continue;

      /* only transform the credits for jobs which may possibly run in this partition */

      if (bmisset(&J->SpecPAL,tmpPar->Index) == FAILURE)
        continue;

      /* transform creds can change the system prio - save it and restore afterwards */

      tmpSystemPrio = J->SystemPrio;

      MJobPartitionTransformCreds(J,tmpPar);

      J->SystemPrio = tmpSystemPrio;

      /* jobs on USETTC partitions can't span nodes--temporarily transform these jobs */

      if ((bmisset(&tmpPar->Flags,mpfUseTTC)) && 
          (bmisset(&J->Flags,mjfProcsSpecified)))
        {
        J->Req[0]->TasksPerNode = J->Req[0]->TaskCount; 
        TransformedJob = TRUE;
        }
      }  /* END for (J) */

    /* make a partition list with just this partition to pass into the scheduling routine */

    PSList[0] = tmpPar;

    MDB(3,fSCHED) MLog("INFO:     priority per partition job scheduling (%s) in iteration %d\n",
      tmpPar->Name,
      MSched.Iteration);

    __MSchedScheduleJobs(PSList[0],PSList,GlobalSQ,GlobalHQ,-1,NULL,TRUE);


    if (TransformedJob == TRUE)
      {
      job_iter JTI2;

      MJobIterInit(&JTI2);

      /* Un-transform USETTC jobs */

      while (MJobTableIterate(&JTI2,&J) == SUCCESS)
        {
        if ((bmisset(&tmpPar->Flags,mpfUseTTC)) && 
            (bmisset(&J->Flags,mjfProcsSpecified)))
          {
          J->Req[0]->TasksPerNode = 0; 
          }
        }  /* END for (J) */
      }  /* END if (TransformedJob) */
    }  /* END for (partition) */
  } /* END void __MSchedSchedulePerPartitionPerJob() */




/**
 *
 *
 * @param GP           (I)
 * @param PList        (I)
 * @param GlobalSQ     (I)
 * @param GlobalHQ     (I) 
 * @param QSize        (I) 
 * @param IdleJC       (I) optional > 0 triggers optimizations (per partition scheduling)
 * @param DoPrioritize (I) 
 */

int __MSchedScheduleJobs(

  mpar_t  *GP,
  mpar_t  *PList[],
  mjob_t **GlobalSQ,
  mjob_t **GlobalHQ,
  int      QSize,
  int     *IdleJC,
  mbool_t  DoPrioritize)

  {
  mpar_t  *P;
  int      PLIndex;
  int      GlobalHQCount;
  char     tmpLine[MMAX_LINE];

  static mjob_t **tmpQ = NULL;
  static mjob_t **CurrentQ = NULL;

  const char *FName = "__MSchedScheduleJobs";

  MDB(3,fSCHED) MLog("%s(%s,PList,%s,%s,%s)\n",
    FName,
    (GP != NULL) ? GP->Name : "NULL",
    (GlobalSQ != NULL) ? "GlobalSQ" : "NULL",
    (GlobalHQ != NULL) ? "GlobalHQ" : "NULL",
    MBool[DoPrioritize]);

  if ((tmpQ == NULL) &&
      (MJobListAlloc(&tmpQ) == FAILURE))
    {
    return(FAILURE);
    }

  if ((CurrentQ == NULL) &&
      (MJobListAlloc(&CurrentQ) == FAILURE))
    {
    return(FAILURE);
    }

  if (MQueueSelectAllJobs(
        mptHard,
        GP,
        GlobalHQ,             /* I/O */
        (QSize > 0) ? QSize + 1 : MSched.M[mxoJob] + 1,
        &GlobalHQCount,
        DoPrioritize,  /* do prioritize */
        (IdleJC != NULL) ? TRUE : FALSE, /* update stats */
        TRUE,  /* track idle - enforce number of idle jobs */
        TRUE,  /* record block */
        FALSE, /* diagnostic */
        NULL) == FAILURE)
    {
    return(FAILURE);
    }

  if (IdleJC != NULL)
    {
    *IdleJC = MStat.IdleJobs;
    }

  if (MQueueSelectAllJobs(
        mptSoft,
        GP,
        GlobalSQ,             /* I/O */
        (QSize > 0) ? QSize + 1 : MSched.M[mxoJob] + 1,
        NULL,
        DoPrioritize,    /* do prioritize */
        FALSE,   /* update stats (clear active and idle stats) */
        TRUE,    /* track idle */
        FALSE,   /* record blocking reason */
        FALSE,   /* diagnostic */
        NULL) == FAILURE)
    {
    return(FAILURE);
    }

  if (MSched.State != mssmRunning)
    {
    MDB(4,fSCHED) MLog("INFO:     scheduler in state %s - not starting workload this iteration\n",
      MSchedState[MSched.State]);

    return(SUCCESS);
    }
  else if (MSched.EnvChanged == FALSE)
    {
    return(SUCCESS);
    }

  /* Step 2.4.1  Select/Prioritize Jobs for Suspended/Reserved Scheduling */

  MOQueueInitialize(tmpQ);

  if (MQueueSelectJobs(
        GlobalHQ,    /* I */
        tmpQ,        /* O */
        mptHard,
        MSched.M[mxoNode],
        MSched.JobMaxTaskCount,
        MMAX_TIME,
        -1,
        NULL,
        FALSE,
        FALSE,
        FALSE,
        TRUE,
        FALSE) == SUCCESS)
    {
    /* schedule suspended/reserved jobs */

    memset(MFQ,0,sizeof(mjob_t *) * (MSched.M[mxoJob] + 1));

    memcpy(MFQ,tmpQ,sizeof(mjob_t *) * GlobalHQCount + 1);

    MQueueScheduleSJobs(tmpQ,NULL,NULL);   /* NOTE - add partition to this function call */

    MQueueScheduleRJobs(tmpQ,(PList[0]->Index == 0) ? NULL : PList[0]);

    MOQueueDestroy(tmpQ,FALSE);
    }

  /* Step 2.4.2  Update Policy Usage */

  MQueueSelectJobs(
    GlobalSQ,    /* I */
    CurrentQ,    /* O */
    mptSoft,
    MSched.M[mxoNode],
    MSched.JobMaxTaskCount,
    MMAX_TIME,
    -1,         /* clear J->CPAL */
    NULL,
    FALSE,
    TRUE,       /* update idle stats */
    FALSE,      /* record blocking reason */
    TRUE,
    FALSE);
       
  /* Step 2.4.3  Evaluate Jobs in Each Partition */
  /* schedule priority jobs */

  if (CurrentQ[0] != NULL)
    {
    /* loop through all partitions and populate J->CPAL for use in 
       MQueueScheduleIJobs() */

    /* Even though the resulting tmpQ from MQueueSelectJobs is not being used,
     * a job won't be scheduled on a given partition if it got selected out
     * in MQueueSelectJobs because the partition won't get into the job's 
     * CPAL. MQueueScheduleIJobs will skip over a partition if it isn't in the
     * job's CPAL. */

    for (PLIndex = 0;PList[PLIndex] != NULL;PLIndex++)
      {
      P = PList[PLIndex];

      if (((P->Index == 0) && (MPar[2].ConfigNodes == 0)) ||
          ((P->ConfigNodes == 0)))
        {
        /* partition is empty and is not dynamic */

        continue;
        }

      if ((P->RM != NULL) &&
          (P->RM->FailIteration == MSched.Iteration) &&
          (P->RM->FailCount >= P->RM->MaxFailCount))
        {
        /* this check also happens in MQueueBackFill, MBFFirstFirt */

        /* have to check with every job or we may end up swamping a failed RM */

        continue;
        }

      /* Step 2.4.3.1  Select Soft Policy Jobs */

      MOQueueInitialize(tmpQ);

      /* NOTE: if job in CurrentQ violates any per-partition limit, do not
               add to tmpQ AND remove from CurrentQ? */

      if (MQueueSelectJobs(
            CurrentQ,  /* I */
            tmpQ,      /* O */
            mptSoft,
            MSched.M[mxoNode],
            MSched.JobMaxTaskCount, /* NOTE:  trigger BF checks */
            MMAX_TIME,
            P->Index,
            NULL,
            FALSE,
            TRUE,      /* update idle stats */
            FALSE,
            TRUE,
            FALSE) == SUCCESS)
        {
        /* NO-OP */
        }    /* END if (MQueueSelectJobs() == SUCCESS) */

      MOQueueDestroy(tmpQ,FALSE);
      }    /* END for (PIndex) */

    sprintf(tmpLine,"scheduling priority jobs for partition (%s)",PList[0]->Name);

    MSchedSetActivity(macSchedule,tmpLine);

    /* Step 2.4.3.2  Schedule Soft Policy Jobs in Priority Order */

    MDB(7,fALL) 
      {
      int jindex;
      int count = 0;
      
      for (jindex = 0;CurrentQ[jindex] != NULL;jindex++)
        count++;

      MLog("INFO:     scheduling %d idle jobs for partition %s\n",
        count,
        PList[0]->Name);
      }

    MQueueScheduleIJobs(CurrentQ,(PList[0]->Index == 0) ? NULL : PList[0]);

    /* Step 2.4.3.3  Schedule Soft Policy Jobs with Backfill */

    /* NOTE:  backfill disabled w/in BASIC release */

    if (MPar[0].BFPolicy != mbfNONE)
      {
      /* backfill jobs using 'soft' policy constraints */

      sprintf(tmpLine,"backfilling soft-policy jobs for partition (%s)",PList[0]->Name);

      MSchedSetActivity(macSchedule,tmpLine);

      MQueueBackFill(CurrentQ,mbfNONE,mptSoft,FALSE,NULL);
      }
    }      /* END if (CurrentQ[0] != -1) */

  MOQueueDestroy(CurrentQ,FALSE);

  /* Step 2.4.3.4  Select Hard Policy Jobs */

  InHardPolicySchedulingCycle = TRUE;

  /* NOTE:  hard/soft policy support disabled for BASIC release */

  MQueueSelectJobs(
    GlobalHQ,    /* I */
    CurrentQ,    /* O */
    mptHard,
    MSched.M[mxoNode],
    MSched.JobMaxTaskCount,
    MMAX_TIME,
    -1,
    NULL,
    FALSE,
    TRUE,
    FALSE,
    TRUE,
    FALSE);

  if (CurrentQ[0] != NULL)
    {
    mbool_t DoGrid = FALSE;

    /* Step 2.4.3.5  Schedule Hard Policy Jobs with Backfill */

    /* backfill jobs using 'hard' policy constraints */

    /* NOTE:  attempt local partitions first */
    /* NOTE:  only attempt remote partitions on hard policy backfill */

    while (TRUE)
      {
      for (PLIndex = 0;PList[PLIndex] != NULL;PLIndex++)
        {
        P = PList[PLIndex];

        if (((P->Index == 0) && (MPar[2].ConfigNodes == 0)) ||
            (P->ConfigNodes == 0))
          {
          /* partition is empty and is not dynamic */

          continue;
          }

        if (P->BFPolicy == mbfNONE)
          {
          continue;
          }

        if ((P->RM != NULL) &&
            (P->RM->Type == mrmtMoab) &&
            !bmisset(&P->RM->Flags,mrmfSlavePeer))
          {
          /* partition is remote non-slave peer */

          if (DoGrid == FALSE)
            continue;
          }
        else
          {
          if (DoGrid == TRUE)
            continue;
          }

        MOQueueInitialize(tmpQ);

        if (MQueueSelectJobs(
              CurrentQ,  /* I */
              tmpQ,      /* O */
              mptHard,
              MSched.M[mxoNode],
              MSched.JobMaxTaskCount,  /* NOTE:  trigger BF checks */
              MMAX_TIME,
              P->Index,
              NULL,
              FALSE,
              TRUE,
              FALSE,
              TRUE,
              FALSE) == SUCCESS)
          {

          /* hard policy backfill */

          sprintf(tmpLine,"backfilling hard-policy jobs for partition (%s)",P->Name);

          MSchedSetActivity(macSchedule,tmpLine);

          MQueueBackFill(tmpQ,mbfNONE,mptHard,FALSE,P);

          if ((GP->VirtualWallTimeScalingFactor > 0.0) ||
              (P->VirtualWallTimeScalingFactor > 0.0))
            {
            MQueueBackFill(tmpQ,mbfNONE,mptHard,TRUE,P);
            }
          }

        MOQueueDestroy(tmpQ,FALSE);
        }  /* END for (PIndex) */

      if (DoGrid == TRUE)
        break;

      DoGrid = TRUE;
      }    /* END while (TRUE) */
    }      /* END if (GlobalHQ[0] != -1) */

  InHardPolicySchedulingCycle = FALSE;

  MOQueueDestroy(CurrentQ,FALSE);

  return(SUCCESS);
  }



/**
 * This function is called once per scheduling iteration. It queries the
 * RM's (@see MRMUpdate()), processes the jobs, calculates priorities, policies,
 * loads checkpoint files (on first iteration), etc. It also determines when a
 * new statistical day has started and manages rolling of the stats.
 *
 * @param OldDay (I)
 * @param GlobalSQ (I)
 * @param GlobalHQ (I)
 *
 * @see MSysMainLoop() - parent
 * @see MRMUpdate() - child
 * @see MSysSchedulePower() - child
 * @see MSysScheduleProvisioning() - child
 * @see MQueueSelectAllJobs() - child
 * @see MQueueSelectJobs() - child
 *
 * 1.0  Validate Parameters and Initialize Structures 
 * 2.0  Update Workload Info 
 *   2.1  If First Load, Perform Post-Load Initialization 
 *   2.2  Update Reservations 
 *   2.3  Schedule Resource Modification (Power, Provisioning, etc) 
 *   2.4  Prioritize, Sort, and Select Eligible Jobs 
 *     2.4.1  Select/Prioritize Jobs for Suspended/Reserved Scheduling 
 *     2.4.2  Update Policy Usage 
 *     2.4.3  Evaluate Jobs in Each Partition 
 *       2.4.3.1  Select Soft Policy Jobs 
 *       2.4.3.2  Schedule Soft Policy Jobs in Priority Order 
 *       2.4.3.3  Schedule Soft Policy Jobs with Backfill 
 *       2.4.3.4  Select Hard Policy Jobs 
 *       2.4.3.5  Schedule Hard Policy Jobs with Backfill 
 * 3.0  Update UI Queue and Workload Stats
 */

int MSchedProcessJobs(

  int       OldDay,   /* I */
  mjob_t **GlobalSQ, /* I */
  mjob_t **GlobalHQ) /* I */

  {
  mpar_t  *GP;

  mbool_t     ModifyJob;
  mbool_t     UseCPJournal = FALSE;

  int         IdleJC;
  mpar_t     *PList[MMAX_PAR + 1];
  int         PCount;
  int         PIndex;

  mln_t *Link;

  const char *FName = "MSchedProcessJobs";

  MDB(4,fSCHED) MLog("%s(%d,GlobalSQ,GlobalHQ)\n",
    FName,
    OldDay);

  /* Step 1.0  Validate Parameters and Initialize Structures */

  MSchedSetActivity(macSchedule,"pre-update prepartion");

  if ((GlobalSQ == NULL) || (GlobalHQ == NULL))
    {
    return(FAILURE);
    }

  MStat.IJobsStarted = 0;

  GP = &MPar[0];

  if ((MSched.Iteration == 0) || (MSched.Reload == TRUE))
    {
    char CPJournalFileName[MMAX_PATH_LEN];

    snprintf(CPJournalFileName,sizeof(CPJournalFileName),"%s/%s",
      MSched.CheckpointDir,
      ".moab.cp.journal");

    MCPJournalOpen(CPJournalFileName);  /* open here to mark MSched.HadCleanStart */

    UseCPJournal = MCP.UseCPJournal;
    MCP.UseCPJournal = FALSE;  /* temporarily disable */

    if (bmisset(&MSched.Flags,mschedfFileLockHA))
      {
      if (MSched.LoadAllJobCP == TRUE)
        {
        /* check spool directory for *.cp files and load them before
         * loading any other jobs! */

        MCPLoadAllSpoolCP();
        }
      }  /* END if (bmisset(&MSched.Flags,mschedfFileLockHA)) */
    }

  /* Step 2.0  Update Workload Info */

  CreateAndSendEventLog(meltSchedCycleStart,MSched.Name,mxoSched,(void *)&MSched);

  if (MRMUpdate() == SUCCESS)
    {
    MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedCycleStart,
        NULL,
        NULL,
        NULL);

    MSchedSetActivity(macSchedule,"processing jobs");

    MUGetMS(NULL,&MSched.LoadStartTime[mschpSched]);

#ifdef MNYI
    {
    long        Value;

    /* NOTE:  value not used (should this be removed?) */

    MQueueGetBestRQTime(MAQ,&Value);
    }
#endif /* MNYI */

    /* Step 2.1  If First Load, Perform Post-Load Initialization */

    if ((MSched.Iteration == 0) || (MSched.Reload == TRUE))
      {
      MSched.EnvChanged = TRUE;

      if (MCPLoad(MCP.CPFileName,mckptGeneral) == FAILURE)
        {
        char tmpLine[MMAX_LINE];

        MUStrCpy(tmpLine,MCP.CPFileName,sizeof(tmpLine));

        strcat(tmpLine,".1");

        MCPLoad(tmpLine,mckptGeneral);
        }

      if (!bmisset(&MSched.Flags,mschedfFileLockHA) && (MSched.LoadAllJobCP == TRUE))
        {
        /* check spool directory for *.cp files and load them as well */

        MCPLoadAllSpoolCP();
        }

      MCPSecondPass();

      /* we should have loaded all jobs from the CP files at this point
       * load in journal data */

      MCP.UseCPJournal = UseCPJournal; /* restore setting */
      MCPJournalLoad();
      MCPSubmitJournalLoad();
      MTrigFailIncomplete();
      }  /* END if ((MSched.Iteration == 0) || (MSched.Reload == TRUE)) */

    MSched.Reload = FALSE;

    /* Step 2.1.5 start jobs that should be started immediately */

    /*
     * Jobs are populated here because they were previously being started
     * directly in MJobRemove while iterating over all jobs in MS3WorkloadQuery.
     * We cannot add to a hash table while iterating over it and this was
     * causing SIGSEGVs.
     */
    for (Link = MJobsReadyToRun;Link != NULL;Link = Link->Next)
      {
      mjob_t *J = (mjob_t *)Link->Ptr;

      MJobStart(J,NULL,NULL,"Starting immediately");
      }

    MULLFree(&MJobsReadyToRun,NULL);

    /* Step 2.2  Update Reservations */

    MRsvRefresh();

    /* determine whether jobs should be preempted */

    if (OldDay != MSched.Day)
      {
      /* new day */

      MSRPeriodRefresh(NULL,NULL);
      }    /* END if (OldDay != MSched.Day) */
    else
      {
      /* adjust all floating/incomplete reservations */

      MSRIterationRefresh(NULL,NULL,NULL);
      }    /* END else (OldDay != MSched.Day) */

    /* Step 2.3  Schedule Pre-Job Resource Modification (Migration, Provisioning, etc) */

    /* NOTE:  provisioning actions based on backlog and other workload metrics
              calculated at the end of XXX during the previous iteration.
     * NOTE: completion of provisioning jobs, hence node OS changes, are
     *       processed within MRMUpdate

     * PROs:
     * - Scheduling provisioning before scheduling jobs allows moab to intelligently
     *     place jobs according to upcoming provisioning changes
     * Cons:
     *  - Cannot schedule jobs to take advantage of newly provisioned nodes
     *     before a new and potentially conflicting round of provisioning jobs
     *     begins.
     *  - None. Our design is perfect!
     */


    MSched.Reload = FALSE;

    /* Step 2.4  Prioritize, Sort, and Select Eligible Jobs */

    MOQueueInitialize(GlobalSQ);
    MOQueueInitialize(GlobalHQ);

    MSchedSetActivity(macSchedule,"selecting jobs");

    /* make a list of partitions available for scheduling */

    PCount = 0;

    for (PIndex = 0;PIndex < MMAX_PAR;PIndex++)
      {
      if (MPar[PIndex].Name[0] == '\1')
        continue;

      if (MPar[PIndex].Name[0] == '\0')
        break;

      if ((MPar[PIndex].RM != NULL) && (MPar[PIndex].RM->State != mrmsActive))
        continue;

      PList[PCount++] = &MPar[PIndex];
      }

    /* terminate the partition list with -1 */

    PList[PCount] = NULL;

    /* Reset per iteration priority reservations counts which will be updated
     * in MQueueScheduleIJobs and MJobPReseve called in __MSchedScheduleJobs */
    MSchedResetPriorityRsvCounts();

    /* if we are doing partition scheduling then schedule one partition at a time */

    if (MSched.PerPartitionScheduling == TRUE)
      {
      /* call MQueueSelectAllJobs to update stats

         Note - stat processing is only a part of MQueueScheduleAllJobs()
                but we are calling it here just to update the stats. */

      MQueueSelectAllJobs(
        mptHard,
        GP,
        GlobalHQ,             /* O */
        MSched.M[mxoJob] + 1,
        NULL,
        TRUE,/* prioritize queue */
        TRUE, /* update stats */
        TRUE, /* track idle */
        FALSE,/* record block */
        TRUE, /* diagnostic */
        NULL);

      IdleJC = MStat.IdleJobs;

      __MSchedSchedulePerPartitionPerJob(PList,GlobalSQ,GlobalHQ);

      /* select all jobs for the global partition so that all eligible jobs show up
         in the idle queue when we do MUIQ processing later in this routine.*/

      MQueueSelectAllJobs(
        mptHard,
        GP,
        GlobalHQ,             /* O */
        MSched.M[mxoJob] + 1,
        NULL,
        TRUE,/* prioritize queue */
        FALSE, /* update stats */
        FALSE, /* track idle */
        FALSE,/* record block */
        TRUE, /* diagnostic */
        NULL);

      } /* END if (MSched.PerPartitionScheduling == TRUE) */
    else
      {  
      /* schedule on the general partition and pass in a list of all eligible partitions */
  
      MDB(3,fSCHED) MLog("INFO:     priority job scheduling for general partition in iteration %d\n",
        MSched.Iteration);
  
      __MSchedScheduleJobs(GP,PList,GlobalSQ,GlobalHQ,-1,&IdleJC,TRUE);
      }

    /* MRMFinalize cycle should be called for xt4 slaves so that 
     * MRMCheckAllocPartition is called. */

    if (((MSched.Mode != msmSlave) && (MSched.State != mssmRunning)) ||
        ((MSched.Mode == msmSlave) && (MSched.State != mssmPaused)) ||
        (MSched.EnvChanged == FALSE))
      {
      /* no need to finalize cycle */
      }
    else
      {
      MRMFinalizeCycle();
      }

    IdleJC = MStat.IdleJobs;

    /* Step 2.5  Schedule Post-Job Resource Modification (Migration, Provisioning, etc) */

    /* Handle resource adjustments which are best performed after latest workload 
       info loaded and scheduled */

    /* NOTE: Migration  needs vmmap jobs to be reserved and started beforehand,
     * which happens in the scheduling phase.
     */

    if (MOLDISSET(MSched.LicensedFeatures,mlfVM) && (MSched.AllowVMMigration == TRUE))
      {
      mln_t *IntendedMigrations = NULL;  /* Migrations to perform (freed by PerformVMMigrations) */

      MDB(7,fMIGRATE) MLog("INFO:     evaluating continuous migrations\n");

      mstring_t MigrateReasons(MMAX_NAME);

      if (MSched.RunVMOvercommitMigration == TRUE)
        {
        PlanVMMigrations(mvmmpOvercommit,FALSE,&IntendedMigrations);

        MStringAppendF(&MigrateReasons,"%s",MVMMigrationPolicy[mvmmpOvercommit]);
        }

      if (MSched.RunVMConsolidationMigration == TRUE)
        {
        PlanVMMigrations(mvmmpConsolidation,FALSE,&IntendedMigrations);

        MStringAppendF(&MigrateReasons,
            "%s%s",
            (!MigrateReasons.empty()) ? "," : "",
            MVMMigrationPolicy[mvmmpConsolidation]);
        }

      if (IntendedMigrations != NULL)
        {
        int NumberPerformed = 0;

        PerformVMMigrations(&IntendedMigrations,&NumberPerformed,MigrateReasons.c_str());

        MDB(7,fMIGRATE) MLog("INFO:     %d migrations performed\n",
          NumberPerformed);
        }
      }  /* END if (bmisset(&MSched.LicensedFeatures,mlfVM) && (MSched.AllowVMMigration == TRUE)) */

    if ((MSched.OnDemandProvisioningEnabled == TRUE) ||
        (MSched.OnDemandGreenEnabled == TRUE))
      {
      MSysScheduleOnDemandPower();
      }

    MUGetMS(NULL,&MSched.LoadEndTime[mschpSched]);
    }        /* END if (MRMUpdate() == SUCCESS) */
  else
    {
    /* RM API failed */

    MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedCycleStart,
        NULL,
        NULL,
        NULL);

    IdleJC = MStat.IdleJobs;

    /* prioritize all jobs, sort, select idle jobs meeting queue policies */

    MQueueSelectAllJobs(
      mptHard,  
      GP,
      GlobalHQ,  /* O */
      MSched.M[mxoJob] + 1,
      NULL,
      TRUE,
      TRUE,
      TRUE,
      TRUE,
      FALSE,
      NULL);
    }    /* END else (MRMUpdate() == SUCCESS) */

  /* Step 3.0  Update UI Queue and Workload Stats */

  /* NOTE:  UI queue provides updated information for client commands */

  ModifyJob = TRUE;

  MQueueSelectJobs(
    GlobalHQ,
    MUIQ,
    mptSoft,
    MSched.M[mxoNode],
    MSched.JobMaxTaskCount,
    MMAX_TIME,
    -1,
    NULL,
    TRUE,
    TRUE,
    TRUE,
    ModifyJob,
    TRUE);

  MStatUpdateBacklog(MUIQ);

  /* must sort/order MUIQ */

  MQueuePrioritizeJobs(MUIQ,TRUE,NULL);

  MStat.IdleJobs = IdleJC - MStat.IJobsStarted;

  /* process data staging requests */

  MSDProcessStageInReqs(NULL);

  if (MSched.MDiagXmlJobNotes == TRUE)
    {
    /* run job eligibility checks and save NOTES for mdiag -j -v --xml commands */

    MQueueDiagnoseEligibility();
    }

  MNLProvisionFlushBuckets();

  /* reset NTRJob ptr */

  MSched.NTRJob = NULL;

  return(SUCCESS);
  }  /* END MSchedProcessJobs() */
/* END MSchedProcessJobs.c */
