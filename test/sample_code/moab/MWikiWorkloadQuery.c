/* HEADER */

      
/**
 * @file MWikiWorkloadQuery.c
 *
 * Contains: MWikiWorkloadQuery
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"  


/**
 * Issue workload query to WIKI RM.
 *
 * NOTE:  no limit on size of data reported back.
 *
 * @see MRMWorkloadQuery() - parent
 * @see MWikiGetData() - peer - loads raw workload data
 *
 * @param R (I)
 * @param WCount (O) [optional]
 * @param NWCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiWorkloadQuery(

  mrm_t                *R,       /* I */
  int                  *WCount,  /* O (optional) */
  int                  *NWCount, /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  job_iter JTI;

  char **Data;

  int    tmpI;

  char  *ptr;
  char  *tail;

  char   Name[MMAX_LINE];
 
  mjob_t *J;

  mln_t *JobsComletedList = NULL;
  mln_t *JobsRemovedList = NULL;
  mln_t *JobListPtr = NULL;

  int    jindex;
  int    jcount;

  enum MJobStateEnum Status;

  char   Message[MMAX_LINE];

  mwikirm_t *S;

  const char *FName = "MWikiWorkloadQuery";

  MDB(2,fWIKI) MLog("%s(%s,WCount,NWCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (WCount != NULL)
    *WCount = 0;

  if (NWCount != NULL)
    *NWCount = 0;

  if (R == NULL)
    {
    return(FAILURE);
    }

  S = (mwikirm_t *)R->S;

  /* NOTE:  S->WorkloadInfoBuffer updated in MWikiClusterQuery()->MWikiGetData()  */

  if ((S == NULL) || (S->WorkloadInfoBuffer == NULL))
    {
    /* no data to process */
  
    return(SUCCESS);
    }

  Data = &S->WorkloadInfoBuffer;

  /* parse job data */

  if ((ptr = strstr(*Data,MCKeyword[mckArgs])) == NULL)
    {
    MDB(1,fWIKI) MLog("ALERT:    cannot locate ARG marker in %s()\n",
      FName);

    MUFree(Data);
 
    return(FAILURE);
    }

  ptr += strlen(MCKeyword[mckArgs]);

  /* FORMAT:  <JOBCOUNT>#<JOBID>:<FIELD>=<VALUE>;[<FIELD>=<VALUE>;]... */

  jcount = (int)strtol(ptr,&tail,10);

  if (jcount <= 0)
    {
    MDB(1,fWIKI) MLog("INFO:     no job data sent by %s RM\n",
      MRMType[R->Type]);

    if ((ptr[0] != '0') || (tail[0] != '#'))
      {
      /* wiki string is corrupt */

      MUFree(Data);

      return(FAILURE);
      }

    MUFree(Data);  /* NOTE:  ptr check above references Data */

    /* queue is empty */

    return(SUCCESS);
    }  /* END if (jcount <= 0) */

  MDB(2,fWIKI) MLog("INFO:     loading %d job(s)\n",
    jcount);

  if ((ptr = MUStrChr(ptr,'#')) == NULL)
    {
    MDB(1,fWIKI) MLog("ALERT:    cannot locate start marker for first job in %s()\n",
      FName);

    MUFree(Data);

    return(FAILURE);
    }

  ptr++;

  mstring_t   ABuf(MMAX_BUFFER); 

  for (jindex = 0;jindex < jcount;jindex++)
    {
    if (MWikiGetAttr(
          R,
          mxoJob,
          Name,
          &tmpI,
          NULL,
          &ABuf,              /* O */
          &ptr) == FAILURE)
      {
      /* failure or end of list reached */

      break;
      }

    Status = (enum MJobStateEnum)tmpI;

    MDB(2,fWIKI) MLog("INFO:     loading job '%s' in state '%s' (%d bytes)\n",
      Name,
      MJobState[Status], 
      (int)ABuf.capacity());

    switch (Status)
      {
      case mjsIdle:
      case mjsHold:
      case mjsUnknown:
      case mjsStarting:
      case mjsRunning:
      case mjsSuspended: 
      
        if (MJobFind(Name,&J,mjsmExtended) == SUCCESS)
          {
          /* update existing job */

          MRMJobPreUpdate(J);

          /* setting the DRM avoids SLURM jobs that in limbo--Moab
           * doesn't think they've been submitted, but they have been */

          if (J->DestinationRM == NULL)
            {
            MJobSetAttr(J,mjaDRM,(void **)R,mdfOther,mSet);
            }

          MWikiJobUpdate(ABuf.c_str(),J,NULL,R);
          }
        else if (MJobCreate(
                  MJobGetName(NULL,Name,R,NULL,0,mjnShortName),
                  TRUE,
                  &J,
                  NULL) == SUCCESS)
          {
          int *TaskList = NULL;

          mjob_t *SJ;
          char tmpEMsg[MMAX_LINE];

          if (NWCount != NULL)
            (*NWCount)++;

          /* if new job, load data */

          MRMJobPreLoad(J,Name,R);

          if (J->DestinationRM == NULL)
            {
            MJobSetAttr(J,mjaDRM,(void **)R,mdfOther,mSet);
            }

          TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

          MJobSetAttr(J,mjaDRMJID,(void **)Name,mdfString,mSet);

          if (MWikiJobLoad(Name,ABuf.c_str(),J,NULL,TaskList,R,tmpEMsg) == FAILURE)
            {
            MDB(1,fWIKI) MLog("ALERT:    cannot load wiki job '%s'\n",
              Name);

            /* Something failed and job should stay around so that user 
             * and admin can investigate. */

            MJobSetHold(J,mhBatch,MSched.DeferTime,mhrRMReject,tmpEMsg);

            MUFree((char **)&TaskList);

            continue;
            }

          if (MTJobFind(Name,&SJ) == SUCCESS)
            {
            /* matching job template found */

            MJobApplySetTemplate(J,SJ,NULL);
            }

          MRMJobPostLoad(J,TaskList,R,NULL);

          MDB(2,fWIKI)
            MJobShow(J,0,NULL);

          MUFree((char **)&TaskList);
          }
        else
          {
          MDB(1,fWIKI) MLog("ERROR:    could not create job (ignoring job '%s')\n",
            Name);
          }

        break;

      case mjsRemoved:
      case mjsCompleted:
      case mjsVacated:

        {
        mbool_t JobCreated = FALSE;

        if (MJobFind(Name,&J,mjsmExtended) == SUCCESS)
          {
          /* if job never ran, remove record.  job cancelled externally */

          if ((J->State != mjsRunning) && 
              (J->State != mjsStarting) && 
              (MJOBWASCANCELED(J) == FALSE))
            {
            MDB(1,fWIKI) MLog("INFO:     job '%s' was cancelled externally\n",
              J->Name);
   
            /* remove job from joblist */

            MJobProcessRemoved(&J);

            break;
            }
 
          MRMJobPreUpdate(J);

          MWikiJobUpdate(ABuf.c_str(),J,NULL,R);

          if (J->EpilogTList != NULL)
            {
            MJobDoEpilog(J, &Status);
            if (J->SubState == mjsstEpilog)
              {
              /* MWikiJobUpdate calls MRMJobPostUpdate with J->State = "completed", so we need a
               * call to MRMJobPostUpdate here with "running" status to keep the job in 
               * the MAQ   */
              MRMJobPostUpdate(J,NULL,Status,J->SubmitRM);
              }
            }
          }
        else if (MJobCFind(Name,&J,mjsmExtended) == SUCCESS)
          {
          /* already completed, don't do anything */
          /* ...unless the epilog needs to run.*/
          if (J->EpilogTList != NULL)
            {
            MJobDoEpilog(J, &Status);
            }
          break;
          }
        else if (MJobCreate(Name,TRUE,&J,NULL) == SUCCESS)
          {
          int *TaskList = NULL;

          /* temporarily add job, then mark it as completed */

          JobCreated = TRUE;

          MRMJobPreLoad(J,Name,R);

          TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

          MWikiJobLoad(Name,ABuf.c_str(),J,NULL,TaskList,R,NULL);

          if ((MSched.Time - J->CompletionTime) > (mulong)MSched.JobCPurgeTime)
            {
            /* Job is old and already purged, do not update */

            MUFree((char **)&TaskList);

            break;
            }

          MRMJobPostLoad(J,TaskList,R,NULL);

          MUFree((char **)&TaskList);
          }
        else
          {
          /* could not load job */

          MDB(6,fRM) MLog("ERROR:   could not load job '%s' (state: %s)!\n",
            Name,
            MJobState[Status]);

          break;
          }

        switch (Status)
          {
          case mjsRemoved:
          case mjsVacated:

            if (MSched.Time < (J->StartTime + J->WCLimit))
              {
              MJobProcessRemoved(&J);
              }
            else
              {
              char TString[MMAX_LINE];

              MULToTString(J->WCLimit,TString);

              sprintf(Message,"JOBWCVIOLATION:  job '%s' exceeded WC limit %s",
                J->Name,
                TString);

              MSysRegEvent(Message,mactNONE,1);

              MDB(3,fWIKI) MLog("INFO:     job '%s' exceeded wallclock limit %s\n",
                J->Name,
                TString);

              MJobProcessRemoved(&J);
              }

            break;

          case mjsCompleted:

            /* If the completed job purge time for SLURM is greater than
             * the completed job purge time for moab then moab might get jobs
             * in the workload query that it has already purged. Do not
             * process completed jobs that moab has already purged. */ 

            if (((MSched.Time - J->CompletionTime) > (unsigned long)MSched.JobCPurgeTime) &&
                (JobCreated == TRUE))
              {
              /* Destroy the temporary completed job that we created above */

              MDB(7,fWIKI) MLog("INFO:    completed job '%s' may have already been purged by moab\n",
                J->Name);

              MJobRemove(&J);
              }
            else
              {
              if ((JobCreated == TRUE) && (J->Req[0] != NULL) && (J->Req[0]->PtIndex == 0))
                {
                /* the created job does not get a partition in J->Req[0] so
                 * go ahead and set it using the partition from this rm so
                 * that showq -c shows a partition. */

                J->Req[0]->PtIndex = R->PtIndex;
                }

              MJobProcessCompleted(&J);
              }

            break;

          case mjsRunning:
            /* probably got to this point because the epilog is running.  
             * Don't do anything on this pass. */
            break;

          default:

            /* unexpected job state */

            MDB(1,fWIKI) MLog("WARNING:  unexpected job state (%d) detected for job '%s'\n",
              Status,
              J->Name);

            break;
          }   /* END switch (Status)                        */
        }

        break;

      default:

        MDB(1,fWIKI) MLog("WARNING:  job '%s' detected with unexpected state '%d'\n",
          Name,
          Status);

        break;
      }  /* END switch (Status) */
    }    /* END for (jindex) */

  /* clean up */

  MUFree(Data);

  if (WCount != NULL)
    *WCount = jcount;

  if (R->State != mrmsActive)
    {
    return(SUCCESS);
    }

  MULLCreate(&JobsComletedList);
  MULLCreate(&JobsRemovedList);

  MJobIterInit(&JTI);

  /* remove jobs not detected via Wiki interface that *should* be reported (i.e. stale jobs) */

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if ((J->SubmitRM->Index != R->Index) &&
       ((J->DestinationRM == NULL) ||
        (J->DestinationRM->Index != R->Index)))
      continue;

    if ((J->ATime <= 0) || 
        (J->UIteration == MSched.Iteration) ||
        ((MSched.Time - J->ATime) <= MSched.JobPurgeTime))
      {
      continue;
      }

    if (MJOBISACTIVE(J) == TRUE)
      {
      MDB(1,fWIKI) MLog("INFO:     active WIKI job %s has been removed from the queue (assuming successful completion)\n",
        J->Name);

      MDB(1,fWIKI) MLog("INFO:     job %s ATime %lu, update iteration %d, sched iteration %d, sched time %lu, purge time %lu\n",
        J->Name,
        J->ATime,
        J->UIteration,
        MSched.Iteration,
        MSched.Time,
        MSched.JobPurgeTime);

      MRMJobPreUpdate(J);

      if ((J->EpilogTList != NULL) &&
          ((J->StartTime > 0) ||
           (bmisset(&J->IFlags,mjifRanProlog))))
        {

        MJobDoEpilog(J, NULL);

        MRMJobPostUpdate(J,NULL,mjsRunning,J->SubmitRM);
        }
      else
        {
        enum MJobStateEnum OldState;
        /* remove job from MAQ */

        /* NYI */

        /* assume job completed successfully for now */

        OldState          = J->State;
        J->State          = mjsCompleted;
        J->CompletionTime = J->ATime;

        MRMJobPostUpdate(J,NULL,OldState,J->SubmitRM);

        MJobSetState(J,mjsCompleted);

        /* add job to completed list to be removed after exiting this loop */

        MULLAdd(&JobsComletedList,J->Name,(void *)J,NULL,NULL);
        }
      }
    else
      {
      MDB(1,fWIKI) MLog("INFO:     non-active WIKI job %s has been removed from the queue (assuming job was cancelled)\n",
        J->Name);

      /* add job to removed list to be removed after exiting this loop */

      MULLAdd(&JobsRemovedList,J->Name,(void *)J,NULL,NULL);
      }    /* END if (MJOBISACTIVE(J) == TRUE) */
    }      /* END for (J) */

  /* delete completed jobs */

  while (MULLIterate(JobsComletedList,&JobListPtr) == SUCCESS)
    MJobProcessCompleted((mjob_t **)&JobListPtr->Ptr);

  /* delete removed jobs */

  while (MULLIterate(JobsRemovedList,&JobListPtr) == SUCCESS)
    MJobProcessRemoved((mjob_t **)&JobListPtr->Ptr);

  if (JobsComletedList != NULL)
    MULLFree(&JobsComletedList,NULL);

  if (JobsRemovedList != NULL)
    MULLFree(&JobsRemovedList,NULL);

  return(SUCCESS);
  }  /* END MWikiWorkloadQuery() */

/* END MWikiWorkloadQuery.c */
