/* HEADER */

      
/**
 * @file MNodeJob.c
 *
 * Contains: MNode and Job relationship functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Populate "Out" with the resources that are not being consumed by jobs right now.
 *
 * NOTE: does not consider admin/standing reservations
 *
 * @param N (I)
 * @param Out (O)
 */

int MNodeSumJobUsage(

  mnode_t *N,
  mcres_t *Out)

  {
  mrsv_t *R;
  mre_t  *RE;
 

  if ((N == NULL) || (Out == NULL) || (N->RE == NULL))
    {
    return(FAILURE);
    }

  MCResClear(Out);

  MCResCopy(Out,&N->CRes);

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if (RE->Type != mreEnd)
      continue;

    if ((R == (mrsv_t *)1) || 
        (R->StartTime > MSched.Time) || 
        (R->Type != mrtJob))
      continue;
    
    MCResRemove(Out,&N->CRes,&R->DRes,RE->TC,FALSE);
    }  /* END for (rindex) */

  return(SUCCESS);
  }  /* END MNodeSumJobUsage() */






/**
 * Get next job in node job list.
 *
 * NOTE:  initialize JTok to -1 before first call.
 *
 * @see MNodeGetJobCount() - peer
 * @see MNodeGetRsv() - peer
 * @see MNodeGetSysJob() - peer
 * @see MNodeGetRE() - peer
 * @see MNodeGetNextJob() - peer - report jobs in time-sorted order 
 *
 * @return SUCCESS only if job found
 *
 * @param N    (I)
 * @param JP   (O)
 * @param JTok (IO) [optional]
 */


int MNodeGetJob(

  mnode_t  *N,
  mjob_t  **JP,
  int      *JTok)

  {
  int jindex;

  if (JP != NULL)
    *JP = NULL;

  if ((N == NULL) || (N->JList == NULL))
    {
    return(FAILURE);
    }

  if (JTok != NULL)
    jindex = *JTok + 1;
  else
    jindex = 0;

  for (;jindex < N->MaxJCount;jindex++)
    {
    if (N->JList[jindex] == NULL)
      break;

    if (N->JList[jindex] == (mjob_t *)1)
      continue;

    if (JP != NULL)
      {
      *JP = N->JList[jindex];
      }
 
    if (JTok != NULL)
      *JTok = jindex;

    return(SUCCESS);
    }  /* END for (jindex) */

  if (JTok != NULL)
    *JTok = jindex;

  return(FAILURE);
  }  /* END MNodeGetJob() */





/**
 * Report node's jobs in time-sorted order
 *
 * @see MNodeGetJob() - peer
 *
 * @param N (I)
 * @param MinStartTime (I)
 * @param RE (I/O) [optional] - current reservation event (should be initialized to first RE)
 * @param JP (O) [optional] - pointer to job located
 */

int MNodeGetNextJob(

  mnode_t  *N,             /* I */
  mulong    MinStartTime,  /* I earliest start time */
  mre_t   **RE,            /* I/O */
  mjob_t  **JP)            /* O */

  {
  mrsv_t *R;

  if (JP != NULL)
    *JP = NULL;

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* locate idle job reservations */

  /* NOTE:  walk node RE table to process reservations in time order */

  if ((RE == NULL) || (*RE == NULL))
    {
    return(FAILURE);
    }

  for (;*RE != NULL;MREGetNext(*RE,RE))
    {
    if ((*RE)->Type != mreStart)
      continue;

    R = MREGetRsv(*RE);

    if ((R->J == NULL) || (R->StartTime <= MinStartTime))
      continue;

    if (JP != NULL)
      *JP = R->J;

    MREGetNext(*RE,RE);

    return(SUCCESS);
    }  /* END for (reindex) */

  /* no jobs located */

  return(FAILURE);
  }  /* END MNodeGetNextJob() */



/**
 * Determine whether the node is busy from untracked VMs (see MOAB-4375).
 *
 * NOTE: when there are untracked VMs on nodes that consume the entire node
 *       then we must update the node state to busy so that we don't try to 
 *       schedule anything else on the node.
 *
 * @see MNodePostUpdate() - parent
 * @see MNodePostLoad() - parent
 *
 * @param N (I)
 */

int MNodeUpdateStateFromVMs(

  mnode_t *N)

  {
  mcres_t VMCRes;
  mcres_t VMDRes;

  if ((N == NULL) || (N->VMList == NULL))
    {
    return(SUCCESS);
    }

  MCResInit(&VMCRes);
  MCResInit(&VMDRes);

  MNodeGetVMRes(N,NULL,FALSE,&VMCRes,&VMDRes,NULL);

  if (N->CRes.Procs - VMCRes.Procs <= 0)
    {
    MNodeSetState(N,mnsBusy,TRUE);
    }

  MCResFree(&VMCRes);
  MCResFree(&VMDRes);

  /* we could optionally set N->ARes.Procs = tmpI on the node too */

  return(SUCCESS);
  }  /* END MNodeUpdateAvailableProcs() */






/**
 * Recalculates numbers for job slots
 *
 * @param N (I)
 */

int MNodeUpdateAvailableGRes(

  mnode_t *N)  /* I */

  {
  int gindex;

  /* process generic resource availability */

  if (!MSNLIsEmpty(&N->CRes.GenericRes))
    {
    int CGRes;
    int XGRes;
    int DGRes;
    int AGRes;

    for (gindex = 1;gindex <= MSched.GResCount;gindex++)
      {
      CGRes = MSNLGetIndexCount(&N->CRes.GenericRes,gindex);
      XGRes = (N->XRes != NULL) ? MSNLGetIndexCount(&N->XRes->GenericRes,gindex) : 0;
      DGRes = 0;
      AGRes = 0;
     
      if (MGRes.GRes[gindex]->StartDelayDate > 0)
        {
        if (MGRes.GRes[gindex]->StartDelayDate <= MSched.Time)
          {
          /* delay has expired */

          MGRes.GRes[gindex]->StartDelayDate = 0;
          }
        else
          {
          /* this GRes is not available during the "StartDelay" */

          MSNLSetCount(&N->ARes.GenericRes,gindex,0);

          continue;
          }
        }

      /* this loop has been changed in 6.1, it now resets sets ARes equal to 
         CRes - XRes for every GRes whose ARes is not reported by an RM */
   
      if ((CGRes > 0) &&
          ((MSched.AResInfoAvail == NULL) ||
           (MSched.AResInfoAvail[N->Index] == NULL) ||
           (MSched.AResInfoAvail[N->Index][gindex] == FALSE)))
        {
        /* node has cgres and no RM is controlling them */
   
        /* FIXME: this also happens, for every job, in MJobAddToNL() when ARes info is not reported */
#if 0
        mcres_t *DRes;
   
        if (N->JList != NULL)
          {
          int jIndex;
          for (jIndex = 0;jIndex < N->MaxJCount;jIndex++)
            {
            if (N->JList[jIndex] == NULL)
              break;
   
            if (N->JList[jIndex] == (mjob_t *)1)
              continue;
   
            if (N->JList[jIndex]->Req != NULL)
              {
              int rIndex;
              for (rIndex = 0;rIndex < MMAX_REQ_PER_JOB;rIndex++)
                {
                if (N->JList[jIndex]->Req[rIndex] == NULL)
                  break;
                DRes = &N->JList[jIndex]->Req[rIndex]->DRes;
                DGRes += MSNLGetIndexCount(&DRes->GenericRes,gindex);
                }
              }
            }
          }
#endif

        AGRes = CGRes - DGRes - XGRes;

        MSNLSetCount(&N->ARes.GenericRes,gindex,AGRes);
        }
      }  /* END for (gindex) */
    }    /* END if (!MSNLIsEmpty(&N->CRes.GenericRes)) */

  return(SUCCESS);
  }  /* END MNodeUpdateAvailableGRes() */






/**
 * Preempt all jobs running on specified.
 *
 * @see MJobPreempt() - child
 *
 * @param N (I)
 * @param Message (I) reason jobs are preempted [optional]
 * @param SPreemptPolicy (I) [optional]
 * @param DoBestEffort (I) [continue preemption on partial failure]
 */

int MNodePreemptJobs(

  mnode_t                 *N,
  const char              *Message,
  enum MPreemptPolicyEnum  SPreemptPolicy,
  mbool_t                  DoBestEffort)

  {
  int jindex;

  mjob_t *J;

  char EMsg[MMAX_LINE];

  mnl_t    tmpNL;    /* temp node list for node-specific migration/preemption */

  mbool_t FailureDetected;

  enum MPreemptPolicyEnum PreemptPolicy;

  const char *FName = "MNodePreemptJobs";

  if (N == NULL) 
    {
    return(SUCCESS);
    }

  MNLInit(&tmpNL);

  PreemptPolicy = (SPreemptPolicy != mppNONE) ? SPreemptPolicy : MSched.PreemptPolicy;

  FailureDetected = FALSE;

  for (jindex = 0;jindex < N->MaxJCount;jindex++)
    {
    J = N->JList[jindex];

    if (J == NULL)
      break;

    if (J == (mjob_t *)1)
      continue;

    if (MJobPreempt(
          J,
          NULL,
          NULL,
          NULL,
          Message,
          PreemptPolicy,
          TRUE,  /* FIXME:  must determine if request is admin or non-admin */
          NULL,
          EMsg,
          NULL) == FAILURE)
      {
      MDB(2,fCORE) MLog("ALERT:    cannot preempt job '%s' in %s (EMsg='%s')\n",
        J->Name,
        FName,
        EMsg);

      if (DoBestEffort == FALSE)
        {
        MNLFree(&tmpNL);

        return(FAILURE);
        }

      FailureDetected = TRUE;
      }
    }  /* END for (jindex) */

  MNLFree(&tmpNL);

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNodePreemptJobs() */





/**
 *
 *
 * @param N (I)
 * @param Action (I)
 * @param DoBestEffort (I) [continue signalling on partial failure]
 */

int MNodeSignalJobs(

  mnode_t  *N,             /* I */
  char     *Action,        /* I */
  mbool_t   DoBestEffort)  /* I (continue signalling on partial failure) */

  {
  int signal;
  int jindex;

  mjob_t *J;

  char EMsg[MMAX_LINE];
  int  SC;

  mbool_t FailureDetected;

  const char *FName = "MNodeSignalJobs";

  if (N == NULL)
    {
    return(SUCCESS);
    }

  FailureDetected = FALSE;

  if ((Action != NULL) && (isdigit(Action[0])))
    {
    signal = (int)strtol(Action,NULL,10);
    }
  else
    {
    signal = -1;
    }

  for (jindex = 0;jindex < N->MaxJCount;jindex++)
    {
    J = N->JList[jindex];

    if (J == NULL)
      break;

    if (J == (mjob_t *)1)
      continue;

    if (MRMJobSignal(
          J,
          signal,
          (Action != NULL) ? Action : (char *)"sigint",
          EMsg,
          &SC) == FAILURE)
      {
      MDB(2,fCORE) MLog("ALERT:    cannot signal job '%s' in %s (EMsg='%s')\n",
        J->Name,
        FName,
        EMsg);

      if (DoBestEffort == FALSE)
        {
        return(FAILURE);
        }

      FailureDetected = TRUE;
      }
    }  /* END for (jindex) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNodeSignalJobs() */





/**
 * Report number of active jobs currently located on node.
 *
 * @see MNodeGetJob()
 *
 * @param N (I)
 */

int MNodeGetJobCount(

  mnode_t *N)  /* I */

  {
  int jindex;
  int jc;

  if (N == NULL)
    {
    return(0);
    }

  if (N->JList == NULL)
    {
    return(0);
    }

  jc = 0;

  for (jindex = 0;jindex < N->MaxJCount;jindex++)
    {
    if (N->JList[jindex] == NULL)
      break;
   
    if (N->JList[jindex] == (mjob_t *)1)
      continue;

    jc++;
    }  /* END for (jindex) */

  return(jc);
  }  /* END MNodeGetJobCount() */






/**
 * Report Active or Reserved System Job Located on Specified Node.
 *
 * @see MNodeGetJob() - peer
 *
 * @param N (I)
 * @param JType (I)
 * @param IsActive (I)
 * @param JP (O) [optional]
 * 
 * @return SUCCESS if matching system job located.
 *
 * NOTE:  will only return first matching job.
 */

int MNodeGetSysJob(

  const mnode_t            *N,
  enum MSystemJobTypeEnum   JType,
  mbool_t                   IsActive,
  mjob_t                  **JP)

  {
  mjob_t *J;
  mrsv_t *R;

  mre_t  *RE;

  if (JP != NULL)
    *JP = NULL;

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  should look at reservations - NYI */

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if (R->Type != mrtJob)
      continue;

    J = R->J;

    if ((J == NULL) || (J->System == NULL) || !(bmisset(&J->Flags,mjfSystemJob)))
      continue;

    if (J->System->JobType != JType)
      continue;

    if ((IsActive == FALSE) &&
        (MJOBISACTIVE(J)))
      continue;

    if ((IsActive == TRUE) &&
        (!MJOBISACTIVE(J)))
      continue;

    if (JP != NULL)
      {
      *JP = J;
      }

    return(SUCCESS);
    }  /* END for (rindex) */

  /* matching system job not located */

  return(FAILURE);
  }  /* END MNodeGetSysJob() */





/**
 * Returns TRUE if job cannot run on node right now (MSched.Time).
 *
 * NOTE: this only checks if an admin/system reservation is on the node right now
 *       consuming all procs, it doesn't check other types of reservations (jobs)
 *
 * @param N
 * @param J
 */

mbool_t MNodeCannotRunJobNow(

  const mnode_t *N,
  const mjob_t  *J)

  {
  mre_t *RE;

  mrsv_t *R;

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if (R->StartTime > MSched.Time + J->WCLimit)
      continue;

    if (R->Type == mrtJob)
      continue;

    if ((R->DRes.Procs == -1) ||
        (R->DRes.Procs * RE->TC == N->CRes.Procs))
      {
      if (MRsvCheckJAccess(R,J,-1,NULL,FALSE,NULL,NULL,NULL,NULL) == FALSE)
        {
        return(TRUE);
        }
      }
    }  /* END for (rindex) */

  return(FALSE);
  }  /* END MNodeCannotRunJobNow() */




/**
 * Return TRUE if node has any suspended jobs.
 *
 * @param N
 */

mbool_t MNodeHasSuspendedJobs(

  const mnode_t *N)

  {
  int jindex;

  if ((N == NULL) || (N->JList == NULL))
    {
    return(FALSE);
    }

  for (jindex = 0;jindex < N->MaxJCount;jindex++)
    {
    if (N->JList[jindex] == NULL)
      break;

    if (N->JList[jindex] == (mjob_t *)1)
      continue;

    if (MJOBISSUSPEND(N->JList[jindex]))
      {
      return(TRUE);
      }
    }  /* END for (jindex) */

  return(FALSE);
  }  /* END MNodeHasSuspendedJobs() */



/**
 * Return TRUE if job can run on node NOW against suspended jobs.
 *
 * @param J
 * @param N
 * @param EMsg
 */

mbool_t MNodeCheckSuspendedJobs(

  const mjob_t  *J,
  const mnode_t *N,
  char          *EMsg)

  {
  int jindex;

  if ((J == NULL) || (N == NULL) || (N->JList == NULL))
    {
    return(TRUE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (MNodeHasSuspendedJobs(N) == FALSE)
    {
    return(TRUE); 
    }

  for (jindex = 0;jindex < N->MaxJCount;jindex++)
    {
    if (N->JList[jindex] == NULL)
      break;

    if (N->JList[jindex] == (mjob_t *)1)
      continue;

    if ((MJOBISSUSPEND(N->JList[jindex])) &&
        (J->CurrentStartPriority < N->JList[jindex]->RunPriority))
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"job '%s' blocks job '%s' on node '%s' (%ld > %ld)",
          N->JList[jindex]->Name,
          J->Name,
          N->Name,
          N->JList[jindex]->RunPriority,
          J->CurrentStartPriority);
        }

      /* job should not run on node as there is a suspended job of higher priority */

      return(FALSE);
      }
    }  /* END for (jindex) */

  return(TRUE);
  }  /* END MNodeCanRunJobOnSuspendedResources() */
/* END MNodeJob.c */
