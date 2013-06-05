/* HEADER */

/**
 * @file MVCJointSchedule.c
 *
 * Contains Virtual Container (VC) joint scheduling and
 *     guaranteed starttime functions.
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"

/* Local prototypes */

int __MVCJointScheduleCreateJob(mjob_t **,mjob_t *,mbool_t *,mvc_t *);
int __MVCJointScheduleAddReqs(mjob_t *,mjob_t *);
mbool_t __MVCJobCanBeJointScheduled(mjob_t *);
int __MVCGutCombinationJob(mjob_t *,mvc_t *);
int __MVCGetJointWalltime(mvc_t *,mulong *);
int __MVCBuildFlatJobList(mvc_t *, mvc_t **);
int __MVCAddObjectToFlatVC(mvc_t *,mvc_t *);
void __MVCFreeFlatVC(mvc_t **);

/**
 *
 * Helper function that will do a duplicate from tmpJ to J if J has not
 *  already been allocated.
 *
 * @see MVCJointScheduleJobs - parent 
 *
 * @param NewJob   [O] - The job to duplicate (if not already done)
 * @param tmpJ     [I] - The job to duplicate from
 * @param JobAdded [O] - Set to TRUE if the job was added
 * @param VC       [I] - The VC that the joint job is being created for
 */

int __MVCJointScheduleCreateJob(

  mjob_t **NewJob,   /* O */
  mjob_t *tmpJ,      /* I */
  mbool_t *JobAdded, /* O */
  mvc_t   *VC)       /* I */

  {
  int ReqIndex = 0;
  mjob_t *J = NULL;
  mulong Walltime = 0;
  int ScheduleCount = 1;  /* Instance counter for generated job name */

  if ((NewJob == NULL) || (tmpJ == NULL) || (JobAdded == NULL) || (VC == NULL))
    return(FAILURE);

  if (*JobAdded == TRUE)
    return(SUCCESS);

  if (*NewJob != NULL)
    {
    /* JobAdded is FALSE, but NewJob isn't NULL? */

    return(FAILURE);
    }

  if ((__MVCGetJointWalltime(VC,&Walltime) == FAILURE) ||
      (Walltime == 0))
    {
    /* Could not find how long to set the walltime for */

    return(FAILURE);
    }

  /* Have to create a name or MJobCreate will fail */

  mstring_t tmpName(MMAX_NAME);

  /* Need to generate a new job name so that if this VC was previously scheduled,
      the new batch of jobs won't conflict with the old batch */

  MStringSetF(&tmpName,"%s.%d.Schedule",VC->Name,ScheduleCount);

  while (MRsvFind(tmpName.c_str(),NULL,mraNONE) == SUCCESS)
    {
    ScheduleCount++;

    MStringSetF(&tmpName,"%s.%d.Schedule",VC->Name,ScheduleCount);
    }

  if (MJobCreate(tmpName.c_str(),TRUE,&J,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  if (MJobDuplicate(J,tmpJ,FALSE) == FAILURE)
    {
    MJobDestroy(&J);

    return(FAILURE);
    }

  MUStrCpy(J->Name,tmpName.c_str(),sizeof(J->Name));

  /* Clear reqs from the new job because they will be added later */

  for (ReqIndex = 0;ReqIndex < MMAX_REQ_PER_JOB + 1;ReqIndex++)
    {
    if (J->Req[ReqIndex] == NULL)
      break;

    MReqDestroy(&J->Req[ReqIndex]);

    J->Req[ReqIndex] = NULL;
    }

  J->SpecWCLimit[0] = Walltime;
  J->SpecWCLimit[1] = Walltime;

  /* Clear any dependencies, otherwise if the first job is a workflow, it
      will add in the time for the setup jobs */

  MJobDependDestroy(J);

  /* Clear hold from the duplicate */

  bmunset(&J->Hold,mhUser);

  /* Add new job to VC so that it will pick up reqnodeset, if any */

  MVCAddObject(VC,(void *)J,mxoJob);

  *NewJob = J;

  *JobAdded = TRUE;

  return(SUCCESS);
  }  /* END __MVCJointScheduleCreateJob() */


/**
 * Helper function that will copy the reqs from tmpJ to NewJob
 *
 * @see MVCJointScheduleJobs - parent
 *
 * @param NewJob [O] - Job to add reqs to
 * @param tmpJ   [I] - Job to copy reqs from 
 */

int __MVCJointScheduleAddReqs(

  mjob_t *NewJob, /* O (modified) */
  mjob_t *tmpJ)   /* I */

  {
  int ReqIndex = 0;
  mreq_t *NewReq;
  int NewReqIndex = 0;

  if ((NewJob == NULL) || (tmpJ == NULL))
    return(FAILURE);

  for (ReqIndex = 0;ReqIndex < MMAX_REQ_PER_JOB + 1;ReqIndex++)
    {
    if (NewJob->Req[ReqIndex] == NULL)
      break;

    NewReqIndex++;
    }

  for (ReqIndex = 0;ReqIndex < MMAX_REQ_PER_JOB + 1;ReqIndex++)
    {
    if (tmpJ->Req[ReqIndex] == NULL)
      break;

    if (NewReqIndex > MMAX_REQ_PER_JOB)
      return(FAILURE);

    if (MReqCreate(NewJob,NULL,&NewReq,FALSE) == FAILURE)
      {
      return(FAILURE);
      }

    MReqDuplicate(NewReq,tmpJ->Req[ReqIndex]);

    /* Link NewReq back to original so that reservations/resources can be sent back */

    MUStrDup(&NewReq->ParentJobName,tmpJ->Name);
    NewReq->ParentReqIndex = ReqIndex;
    NewReq->Index = NewReqIndex;
    NewReqIndex++;
    }  /* END for (ReqIndex) */

  return(SUCCESS);
  }  /* END __MVCJointScheduleAddReqs() */


/**
 * Helper function to determine if a job can be part of a VC's 
 *  joint scheduling operation.
 *
 * @see MVCJointScheduleJobs() - parent
 *
 * @param J (I) - The job to evaluate
 */

mbool_t __MVCJobCanBeJointScheduled(

  mjob_t *J)

  {
  mbool_t rcB = TRUE;

  if (J == NULL)
    return(FALSE);

  if (!bmisset(&J->Hold,mhUser))
    return(FALSE);

  /* Check that user is the only hold set.
      Unset user hold, see if J->Hold is 0, then reset user hold. */

  bmunset(&J->Hold,mhUser);

  if (!bmisclear(&J->Hold))
    rcB = FALSE;

  bmset(&J->Hold,mhUser);

  if (MJOBISACTIVE(J))
    rcB = FALSE;

  return(rcB);  
  }  /* END __MVCJobCanBeJointScheduled() */


/**
 * Helper function to find the required walltime for a combination job.
 *
 * @param VC       [I] - the VC we're getting the walltime for
 * @param Walltime [O] - output mulong (the minimum needed walltime)
 */

int __MVCGetJointWalltime(

  mvc_t *VC,
  mulong *Walltime)

  {
  mln_t *tmpL = NULL;
  mln_t *tmpL2 = NULL;
  mvc_t *tmpVC = NULL;
  mjob_t *tmpJ = NULL;

  mulong ExpectedWallTime = 0;
  mulong WorkflowWallTime = 0;  /* Temp time for summing workflow job times */

  if ((VC == NULL) || (Walltime == NULL))
    {
    return(FAILURE);
    }

  /* Iterate through all jobs and workflows, get the MAX time needed */

  for (tmpL = VC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpJ = (mjob_t *)tmpL->Ptr;

    if (tmpJ == NULL)
      continue;

    if (__MVCJobCanBeJointScheduled(tmpJ) == FALSE)
      continue;

    ExpectedWallTime = MAX(ExpectedWallTime,tmpJ->SpecWCLimit[0]);
    }  /* END for (tmpL = VC->Jobs) */

  for (tmpL = VC->VCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    if ((!bmisset(&tmpVC->Flags,mvcfWorkflow)) ||
      (bmisset(&tmpVC->Flags,mvcfHasStarted)))
      continue;

    WorkflowWallTime = 0;

    for (tmpL2 = tmpVC->Jobs;tmpL2 != NULL;tmpL2 = tmpL2->Next)
      {
      tmpJ = (mjob_t *)tmpL2->Ptr;

      if (tmpJ == NULL)
        continue;

      if (__MVCJobCanBeJointScheduled(tmpJ) == FALSE)
        continue;

      WorkflowWallTime += tmpJ->SpecWCLimit[0];
      }  /* END for (tmpL2 = tmpVC->Jobs) */

    /* Don't allow to go over MMAX_TIME (infinite walltime) */

    WorkflowWallTime = MIN(WorkflowWallTime,MMAX_TIME);

    ExpectedWallTime = MAX(ExpectedWallTime,WorkflowWallTime); 
    }  /* END for (tmpL = VC->VCs) */

  *Walltime = ExpectedWallTime;

  return(SUCCESS);
  }  /* END __MVCGetJointWalltime() */


/**
 * Takes all of the held jobs/workflows in a VC (first level only, no recursion),
 *  and creates a pseudo-job that is the combination of all of the sub-jobs.
 *  This combination job will then be scheduled.  When the combination job is started,
 *  it will be gutted and it's resources passed to the original jobs.
 *
 * Steps:
 *  1.  Create empty job
 *  2.  Iterate through VC's jobs and immediate workflows
 *    a.  For each held job/workflow, copy reqs to the new job
 *  3.  Return SUCCESS if no errors found (may still return SUCCESS if no job
 *      was created if there were no jobs to combine)
 *
 * NOTE:  this means that you can only combine up to MMAX_REQ_PER_JOB reqs.  This is
 *    checked by MReqCreate.  MReqCreate also logs its own failures.
 *
 * @param VC [I] (Optional) - The VC whose jobs are to be scheduled
 * @param J  [O] - Job that is the combination of VC job/workflow reqs
 */

int MVCJointScheduleJobs(

  mvc_t *VC,
  mjob_t **J)

  {
  mjob_t *NewJob = NULL;  /* Combination job */
  mjob_t *tmpJ   = NULL;
  mvc_t  *tmpVC  = NULL;
  mln_t  *tmpL   = NULL;
  mln_t  *tmpL2  = NULL;

  mbool_t JobAdded = FALSE;

  int rc = SUCCESS;
  int ReqIndex;

  const char *FName = "MVCJointScheduleJobs";

  MDB(6,fSCHED) MLog("%s()\n",
    FName,
    (VC != NULL) ? VC->Name : "NULL",
    (J != NULL) ? "J" : "NULL");

  if (J != NULL)
    *J = NULL;

  if (VC == NULL)
    return(FAILURE); 

  /* Go through jobs */

  for (tmpL = VC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
    {
    if (rc == FAILURE)
      break;

    tmpJ = (mjob_t *)tmpL->Ptr;

    if (__MVCJobCanBeJointScheduled(tmpJ) == FALSE)
      continue;

    if (__MVCJointScheduleCreateJob(&NewJob,tmpJ,&JobAdded,VC) == FAILURE)
      {
      rc = FAILURE;

      break;
      }

    /* Have a valid job - add to combination job */

    if (__MVCJointScheduleAddReqs(NewJob,tmpJ) == FAILURE)
      {
      rc = FAILURE;

      break;
      }
    }  /* END for (tmpL = VC->Jobs) */

  /* Go through workflows */

  for (tmpL = VC->VCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    mbool_t WorkflowValid = TRUE;

    if (rc == FAILURE)
      break;

    tmpVC = (mvc_t *)tmpL->Ptr;

    if (!bmisset(&tmpVC->Flags,mvcfWorkflow))
      continue;

    if (bmisset(&tmpVC->Flags,mvcfHasStarted))
      continue;

    /* Workflow VC, check that all jobs within it are eligible */

    for (tmpL2 = tmpVC->Jobs;tmpL2 != NULL;tmpL2 = tmpL2->Next)
      {
      tmpJ = (mjob_t *)tmpL2->Ptr;

      if (__MVCJobCanBeJointScheduled(tmpJ) == FALSE)
        {
        WorkflowValid = FALSE;

        break;
        }
      }  /* END for (tmpL2 = tmpVC->Jobs) */

    if (WorkflowValid == FALSE)
      continue;

    /* Have a valid workflow VC */

    /* Find the main job and add it's reqs */

    if (tmpVC->CreateJob != NULL)
      {
      tmpJ = NULL;

      MJobFind(tmpVC->CreateJob,&tmpJ,mjsmBasic);

      if (tmpJ == NULL)
        {
        MDB(7,fSTRUCT) MLog("ERROR:  VC '%s' was marked as workflow, could not find job that created it\n",
          tmpVC->Name);

        rc = FAILURE;

        break;
        }

      /* Have the correct job, add its reqs to combination job */

      if (__MVCJointScheduleCreateJob(&NewJob,tmpJ,&JobAdded,VC) == FAILURE)
        {
        rc = FAILURE;

        break;
        }

      if (__MVCJointScheduleAddReqs(NewJob,tmpJ) == FAILURE)
        {
        rc = FAILURE;

        break;
        }
      }  /* END if (VC->CreateJob != NULL) */
    else
      {
      /* ERROR: workflow VC should have a CreateJob */

      MDB(7,fSTRUCT) MLog("ERROR:  VC '%s' was marked as workflow, could not find job that created it\n",
        tmpVC->Name); 

      rc = FAILURE;

      break;
      }
    }  /* END for (tmpL = VC->VCs) */

  if ((rc == FAILURE) && (JobAdded == TRUE))
    {
    /* Free all created reqs and job */
    for (ReqIndex = 0;ReqIndex < MMAX_REQ_PER_JOB + 1;ReqIndex++)
      {
      if (NewJob->Req[ReqIndex] == NULL)
        break;

      MReqDestroy(&NewJob->Req[ReqIndex]);
      }

    MJobDestroy(&NewJob);

    NewJob = NULL;
    }  /* END if ((rc == FAILURE) && (JobAdded == TRUE)) */

  if (J != NULL)
    *J = NewJob;

  return(rc);
  }  /* END MVCJointScheduleJobs() */


/**
 * Guts the reservation from a scheduled combination job and
 *  passes them to their original jobs/reqs.
 *
 * @param J  [I] - The job to gut
 * @param VC [I] (modified) - The VC that governs the guaranteed start time
 */

int __MVCGutCombinationJob(

  mjob_t *J,  /* I */
  mvc_t  *VC) /* I [modified] */

  {
  int rc = SUCCESS;
  int ReqIndex = 0;
  mjob_t *DstJ = NULL;

  if ((J == NULL) || (VC == NULL))
    return(FAILURE);

  /* Do a pre-check of all the reqs.  If no reservation, or destination
      job isn't found, fail */

  for (ReqIndex = 0;ReqIndex < MMAX_REQ_PER_JOB + 1;ReqIndex++)
    {
    if (J->Req[ReqIndex] == NULL)
      break;

    if (J->Req[ReqIndex]->ParentJobName == NULL)
      {
      /* Should not get here - malformed combination job */

      rc = FAILURE;

      break;
      }

    if (J->Req[ReqIndex]->R == NULL)
      {
      rc = FAILURE;

      break;
      }

    if (MJobFind(J->Req[ReqIndex]->ParentJobName,&DstJ,mjsmBasic) == FAILURE)
      {
      rc = FAILURE;

      break;
      }

    if ((J->Req[ReqIndex]->ParentReqIndex < 0) ||
        (J->Req[ReqIndex]->ParentReqIndex > MMAX_REQ_PER_JOB) ||
        (DstJ->Req[J->Req[ReqIndex]->ParentReqIndex] == NULL))
      {
      rc = FAILURE;

      break;
      }
    }  /* END for (ReqIndex) */

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  /* Everything checks out, pass reservations across */

  for (ReqIndex = 0;ReqIndex < MMAX_REQ_PER_JOB + 1;ReqIndex++)
    {
    mvc_t *WorkflowVC = NULL;

    if (J->Req[ReqIndex] == NULL)
      break;

    MJobFind(J->Req[ReqIndex]->ParentJobName,&DstJ,mjsmBasic);

    /*
      For each job, there are actions to do.  If the job is part of a
        workflow, these actions are done on each job in the workflow.

      1.  Add job/workflow to the reservation ACL
      2.  Set the reservation as the ReqRID for job/workflow
      3.  Remove holds on destination job(s).
      4.  Add reservation to the VC.

    About setting the advres on jobs.  This means that guaranteed start times
        currently only work with single-req jobs.  We can't set
        DstJ->Req[X]->R because then the reservation is blown away when
        the job completes, which wouldn't work for workflow jobs because
        the first setup job would blow away the reservation 
    */

    /* Rework the ACL on the reservation to allow DstJ in */

    MVCGetJobWorkflow(DstJ,&WorkflowVC);

    if (WorkflowVC != NULL)
      {
      mln_t *tmpL = NULL;
      mjob_t *ChildJ = NULL;

      /* Add the workflow to the reservation ACL */

      MACLSet(&J->Req[ReqIndex]->R->ACL,maVC,(void *)WorkflowVC->Name,mcmpSEQ,mnmPositiveAffinity,0,TRUE);

      /* Set ReqRID on each job in the workflow */

      for (tmpL = WorkflowVC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
        {
        ChildJ = (mjob_t *)tmpL->Ptr;

        if (ChildJ == NULL)
          continue;

        MUStrDup(&ChildJ->ReqRID,J->Req[ReqIndex]->R->Name);
        bmset(&ChildJ->Flags,mjfAdvRsv);
        bmset(&ChildJ->SpecFlags,mjfAdvRsv);

        bmunset(&ChildJ->Hold,mhUser);
        }
      }  /* END if (WorkflowVC != NULL) */
    else
      {
      /* Job is not part of a workflow, just add the job to the ACL */

      MACLSet(&J->Req[ReqIndex]->R->ACL,maJob,(void *)DstJ->Name,mcmpSEQ,mnmPositiveAffinity,0,TRUE);

      MUStrDup(&DstJ->ReqRID,J->Req[ReqIndex]->R->Name);
      bmset(&J->Flags,mjfAdvRsv);
      bmset(&J->SpecFlags,mjfAdvRsv);

      bmunset(&DstJ->Hold,mhUser);
      }    

    /* Add Rsv to VC so that they can be cleaned up if necessary if jobs
        are destroyed early and so that they are reflected as belonging to
        the environment. */

    MVCAddObject(VC,(void **)J->Req[ReqIndex]->R,mxoRsv);

    /* Destroy link to combination job */
    J->Req[ReqIndex]->R->Type = mrtUser;
    J->Req[ReqIndex]->R->SubType = mrsvstUserRsv;

    J->Req[ReqIndex]->R->J = NULL;
    J->Req[ReqIndex]->R = NULL;
    }  /* END for (ReqIndex) */

  if (J->Rsv != NULL)
    {
    J->Rsv->J = NULL;
    J->Rsv = NULL;
    }

  return(SUCCESS);
  }  /* END __MVCGutCombinationJob() */


/**
 * Takes a VC and will create reservations for the jobs/workflows
 *  inside.  Returns TRUE if reservations were created for the VC
 *  jobs, FALSE otherwise.
 *
 * The check for if Msg is NULL is done inside of MStringSetF - it will
 *  just return FAILURE if Msg is NULL.
 *
 * @see MVCJointScheduleJobs() - child
 *
 * @param VC  [I] - The VC to get reservations for
 * @param StartDate [O] - The starttime that was determined
 * @param Msg [O] (optional) - Output buffer.  If not NULL, must be pre-initialized.
 */

int MVCGetGuaranteedStartTime(

  mvc_t     *VC,
  mulong    *StartDate,
  mstring_t *Msg)

  {
  mulong EStartTime = 0;
  mjob_t *CombinationJob = NULL;
  int rc = SUCCESS;
  mvc_t *flatVC = NULL;

  const char *FName = "MVCGetGuaranteedStartTime";

  MDB(6,fSCHED) MLog("%s(%s,Msg)\n",
    FName,
    (VC != NULL) ? VC->Name : "NULL");

  if ((VC == NULL) || (StartDate == NULL))
    return(FAILURE);

  *StartDate = 0;

  /* Search existing VC for all jobs & workflows.  Create a flat VC (without sub-containers)
     to call MVCJointScheduleJobs to build a job that contains all jobs so we can get
     reservations for all resources.  */

  __MVCBuildFlatJobList(VC,&flatVC);

  if (MVCJointScheduleJobs((flatVC != NULL) ? flatVC : VC,&CombinationJob) == FAILURE)
    {
    MStringSetF(Msg,"Failed to combine jobs in VC '%s'",
      VC->Name);

    MDB(6,fSCHED) MLog("ERROR:  Failed to combine jobs in VC '%s'\n",
      VC->Name);

    __MVCFreeFlatVC(&flatVC);

    return(FAILURE);
    }

  if (CombinationJob == NULL)
    {
    /* No eligible jobs in the VC to schedule */

    MStringSetF(Msg,"No eligible jobs found for scheduling in VC '%s'",
      VC->Name);

    MDB(6,fSCHED) MLog("INFO:     No eligible jobs found for scheduling in VC '%s'\n",
      VC->Name);

    __MVCFreeFlatVC(&flatVC);

    return(SUCCESS);
    }

  MVCGetDynamicAttr(VC,mvcaReqStartTime,(void **)&EStartTime,mdfMulong);

  /* We now have a job that represents all of the requirements of the jobs
       and workflows in the VC */

  if (MJobReserve(&CombinationJob,
                NULL,
                EStartTime,
                mjrUser,
                TRUE) == FAILURE)
    {
    __MVCFreeFlatVC(&flatVC);
    MJobDestroy(&CombinationJob);

    MStringSetF(Msg,"Failed to schedule VC '%s'",
      VC->Name);

    MDB(6,fSCHED) MLog("ERROR:  Failed to schedule VC '%s'\n",
      VC->Name);

    __MVCFreeFlatVC(&flatVC);

    return(FAILURE);
    }

  if (CombinationJob->Req[0]->R == NULL)
    {
    rc = FAILURE;

    MStringSetF(Msg,"Failed to find valid reservation for VC '%s'",
      VC->Name);

    MDB(6,fSCHED) MLog("ERROR:  Failed to find valid reservation for VC '%s'\n",
      VC->Name);
    }

  /* Check requested start time */

  if ((rc != FAILURE) && (EStartTime > 0))
    {
    /* We had a starttime that needed to be fulfilled.  If the reservation
        starttimes are greater than requested starttime, return FAILURE */

    if (CombinationJob->Req[0]->R->StartTime != EStartTime)
      {
      rc = FAILURE;

      MStringSetF(Msg,"Could not schedule VC '%s' for requested time",
        VC->Name);

      MDB(6,fSCHED) MLog("ERROR:  Could not schedule VC '%s' for requested time\n",
        VC->Name);
      }
    }  /* END if (EStartTime > 0) */

  /* Check start time is not INFINITY */

  if ((rc != FAILURE) && (CombinationJob->Req[0]->R->StartTime >= MMAX_TIME))
    {
    /* Resources not available - first valid time is INFINITY */

    rc = FAILURE;

    MStringSetF(Msg,"Resources not available at any time for VC '%s'",
      VC->Name);

    MDB(6,fSCHED) MLog("ERROR:  Resources not available at any time for VC '%s'\n",
      VC->Name);
    }  /* END if (CombinationJob->Req[0]->R->StartTime >= MMAX_TIME) */

  if (rc != FAILURE)
    *StartDate = CombinationJob->Req[0]->R->StartTime;

  /* Have reservations, need to gut the reservation */

  if ((rc != FAILURE) && (__MVCGutCombinationJob(CombinationJob,VC) == FAILURE))
    {
    *StartDate = 0;

    rc = FAILURE;

    MStringSetF(Msg,"Failed to create reservations for jobs in VC '%s'",
      VC->Name);

    MDB(6,fSCHED) MLog("ERROR:  Failed to create reservations for jobs in VC '%s'\n",
      VC->Name);
    }

  /* Free the temp combination job & flat VC */

  __MVCFreeFlatVC(&flatVC);
  MJobDestroy(&CombinationJob);

  return(rc);
  }  /* END MVCGetGuaranteedStartTime() */


/**
 * Takes a flat VC and drops all JOB & VC pointers the frees the 
 *  VC.  
 *
 *
 * @param flatVC  [I] - The flat VC pointer.
 */

void __MVCFreeFlatVC(mvc_t **flatVC)
  {
  mvc_t *tmp;
  /* Free tmpVC.  Remove all job & VC pointers */

  if ((flatVC == NULL) || (*flatVC == NULL))
    return;
  tmp = *flatVC;

  while((volatile mln_t*)tmp->Jobs != NULL)
    MVCRemoveObject(tmp,tmp->Jobs->Ptr,mxoJob);

  while((volatile mln_t*)tmp->VCs != NULL)
    MVCRemoveObject(tmp,tmp->VCs->Ptr,mxoxVC);

  MVCFree(flatVC);
  }  /* END __MVCFreeFlatVC */

/**
 * Build a flat VC of all jobs and workflow VC's.  This flat 
 *  VC is a temporary VC that is NOT reported to Moab.  This
 *  function is called recursively for each VC in the chain.
 *
 *
 * @param VC  [I] - The Parent flat VC pointer. 
 * @param flatVC [0] - The output VC that contains all jobs & 
 *               workflows.
 * @return SUCCESS / FAILURE 
 */

int __MVCBuildFlatJobList(mvc_t *VC, mvc_t **flatVC)
  {
  mvc_t *tmp;
  mln_t *Iter = NULL;

  if ((VC == NULL) || (flatVC == NULL))
    return(FAILURE);
  tmp = *flatVC;

  /* Create a temp VC, if not already specified */

  if (tmp == NULL)
    {

    /* Alocate the VC memory */

    if ((tmp = (mvc_t *)MUCalloc(1,sizeof(mvc_t))) == NULL)
      return(FAILURE);
    sprintf(tmp->Name,"%s.schedule_group",VC->Name);
    
    *flatVC = tmp;

    /* Check if VC has a reqstarttime */

    mulong EStartTime = 0;
    MVCGetDynamicAttr(VC,mvcaReqStartTime,(void **)&EStartTime,mdfMulong);

    if (EStartTime > 0)
      {
      /* VC had a required start time, this needs to be preserved */
      if (MVCSetDynamicAttr(*flatVC,mvcaReqStartTime,(void **)&EStartTime,mdfMulong) == FAILURE)
        {
        MUFree((char **)flatVC);

        return(FAILURE);
        }
      }  /* END if (EStartTime > 0) */
    }  /* END if (tmp == NULL) */


  /* Iterate through all VC's */

  while(MULLIteratePayload(VC->VCs,&Iter,NULL) == SUCCESS)
    {
    mvc_t *v = (mvc_t*)Iter->Ptr;
    
    if (v->VCs != NULL)
      __MVCBuildFlatJobList(v,flatVC);

    /* Add jobs/workflow VC from child VC to tmpVC */

    __MVCAddObjectToFlatVC(tmp,v);
    }

  /* Add jobs/workflow VC from parent VC to tmpVC */

  __MVCAddObjectToFlatVC(tmp,VC);
    
  return(SUCCESS);
  }  /* END  __MVCBuildFlatJobList */

/**
 * Adds all jobs and workflow VC's to the flat VC. 
 *
 * @param flatVC  [I] - The flat VC pointer. 
 * @param VC [0] - The VC that contains jobs and 
 *               workflows.
 * @return SUCCESS / FAILURE 
 */

int __MVCAddObjectToFlatVC(mvc_t *flatVC,mvc_t *VC)
  {
  mln_t *tmpL = NULL;

  /* Check for a workflow VC */
  if(bmisset(&VC->Flags,mvcfWorkflow))
    {
    /* Workflow VC -- Add the entire VC */
    MVCAddObject(flatVC,(void*)VC,mxoxVC);
    }
  else
    {
    /* Non-workflow VC -- Add each job */
    for (tmpL = VC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
      {
      MVCAddObject(flatVC,(void*)tmpL->Ptr,mxoJob);
      }
    }
  return(SUCCESS);
  } /* END - __MVCAddObjectToFlatVC */
