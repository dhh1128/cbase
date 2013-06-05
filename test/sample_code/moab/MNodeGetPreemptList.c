/* HEADER */

      
/**
 * @file MNodeGetPreemptList.c
 *
 * Contains: MNodeGetPreemptList()
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Removes a mjob_t pointer from the given array.
 *
 * @param Originals (I) Array of null-terminated job pointers.
 * @param RemoveJ (I) Job to remove from list of jobs.
 */

int MURemoveJobFromArray(

  mjob_t **Originals,
  mjob_t  *RemoveJ)

  {
  int origIndex;
  int offset = 0;

  mjob_t *origJ = NULL;
  
  if ((Originals == NULL) || (RemoveJ == NULL))
    return(FAILURE);

  for (origIndex = 0;Originals[origIndex] != NULL;origIndex++)
    {
    origJ = Originals[origIndex];

    if (RemoveJ == origJ)
      {
      offset++;
      }
    else if (offset > 0)
      {
      Originals[origIndex - offset] = origJ;
      }
    } /* END for (origIndex) */

  Originals[origIndex - offset] = NULL; 

  return(SUCCESS);
  }  /* END MURemoveJobFromArray() */


/**
 * Removes the list of mjob_t pointers from the orignal list.
 *
 * Assumes the list is NULL terminated.
 *
 * @param Originals (I) NULL terminated list of mjob_t *s to be remove from.
 * @param Removals (I) NULL terminated list of mjob_t *s to remove.
 */

int MURemoveJobListFromArray(

  mjob_t **Originals,
  mjob_t **Removals)

  {
  int removeIndex;
  mjob_t *removeJ = NULL;
  
  if ((Originals == NULL) || (Removals == NULL))
    return(FAILURE);

  for (removeIndex = 0;Removals[removeIndex] != NULL;removeIndex++)
    {
    removeJ = Removals[removeIndex];

    MURemoveJobFromArray(Originals,removeJ);
    } /* END for (removeIndex) */

  return(SUCCESS);
  }  /* END MURemoveJobListFromArray() */



/**
 * Get list of preemptible nodes which have resources satisfying job PreemptorJ.
 *
 * @see MJobSelectMNL() - parent
 * @see MJobCanPreempt() - child - determine if PreemptorJ can preempt preemptee
 * @see MJobSelectPJobList() - peer - selects best jobs to preempt
 *
 * @param PreemptorJ (I)
 * @param RQ (I)
 * @param FNL (I) [optional]
 * @param PNL (O)
 * @param ExcludedPreempteeList (I)
 * @param PJL (O) [optional,minsize=MDEF_MAXJOB] preemptible jobs
 * @param Priority (I) preemptor's priority
 * @param P (I) desired partition
 * @param IsConditional (I)
 * @param ForceCancel (I)
 * @param NodeCount (O) [optional]
 * @param TaskCount (O) [optional]
 */

int MNodeGetPreemptList(

  mjob_t       *PreemptorJ,
  const mreq_t *RQ,
  mnl_t        *FNL,
  mnl_t        *PNL,
  mjob_t      **ExcludedPreempteeList,
  mjob_t      **PJL,
  long          Priority,
  const mpar_t *P,
  mbool_t       IsConditional,
  mbool_t       ForceCancel,
  int          *NodeCount,
  int          *TaskCount)
 
  {
  static mulong     PreemptListMTime[MMAX_PAR];
  static int        ActiveJobCount[MMAX_PAR];

  static int        PTC[MMAX_PAR];
  static int        PNC[MMAX_PAR];
  static mjob_t    *PreemptJobCache[MMAX_PAR][MDEF_MAXJOB + 2];

  static long       CachePriority;
 
  static mbool_t    InitializeTimeStamp = TRUE;

  static mnl_t     *PreemptNodeCache[MMAX_PAR];
  static mbool_t    PreemptNodeCacheInit = FALSE;

  int njstartindex; /* index of first newly added preemptible job */

  int index;
  int index2;

  int nindex;
  int jindex;
  int jindex2;
 
  int tmpNC;
  int tmpTC;

  int TC;

  int MaxPJC;   /* maximum feasible preemptible jobs to consider */

  mbool_t JobIsPreemptible;

  int      TA;  /* tasks available */
  long     TD;
 
  mnode_t *tmpN;
  mnode_t *N;
  mjob_t  *J;

  mjob_t  *tJ;

  int      rc;

  int      RATail;
  mbool_t  RAListIsEmpty;
 
  mjob_t  *tmpJ[MMAX_PJOB + 1];

  mcres_t  PreemptRes;

  char    *tmpRAList[MMAX_PJOB + 1];

  const char *FName = "MNodeGetPreemptList";

  MDB(4,fSCHED) MLog("%s(J,%s,PNode,PJob,%ld,%s,%s,NC,TC)\n",
    FName,
    (FNL != NULL) ? "FNL" : "NULL",
    Priority, 
    (P != NULL) ? P->Name : "NULL",
    MBool[IsConditional]);

  /* verify parameters */

  if ((P == NULL) || (PNL == NULL) || (PreemptorJ == NULL))
    {
    return(FAILURE);
    }

  if (PreemptNodeCacheInit == FALSE)
    {
    MNLMultiInit(PreemptNodeCache);

    PreemptNodeCacheInit = TRUE;
    }

  tmpJ[0] = NULL;

  MaxPJC = MDEF_MAXJOB;

  if ((PreemptorJ != NULL) && (PreemptorJ->Request.TC == 1))
    {
    if (MSched.SerialJobPreemptSearchDepth > 0)
      {
      MaxPJC = MIN(MSched.SerialJobPreemptSearchDepth,MaxPJC);
      }
    }

  if (PJL != NULL)
    PJL[0] = NULL;

  if (InitializeTimeStamp == TRUE)
    {
    memset(PreemptListMTime,0,sizeof(PreemptListMTime));
    memset(ActiveJobCount,0,sizeof(ActiveJobCount));

    CachePriority = -1;
 
    InitializeTimeStamp = FALSE;
    }

  /* NOTE:  MUST process conditional requirements (NYI) */

  if (IsConditional == TRUE)
    {
    int nindex2;

    mre_t  *RE;
    mrsv_t *R;

    mbool_t NodeIsPreemptee;

    int PJCount = 0;
    int PNCount = 0;
    int PTCount = 0;
   
    /* locate active 'owned' reservations */

    /* all active jobs within reservation are preemptible */

    index = 0;

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      if (FNL == NULL)
        N = MNode[nindex];
      else
        MNLGetNodeAtIndex(FNL,nindex,&N);

      if (N == NULL)
        break;

      if ((PreemptorJ->ReqRID != NULL) &&
          (MNodeRsvFind(N,PreemptorJ->ReqRID,TRUE,NULL,NULL) == FAILURE))
        {
        MDB(5,fSCHED) MLog("INFO:     job %s requires rsv %s missing from idle node %s\n",
          PreemptorJ->Name,
          PreemptorJ->ReqRID,
          N->Name);

        continue;
        }

      if (MNodeCannotRunJobNow(N,PreemptorJ) == TRUE)
        continue;

      NodeIsPreemptee = FALSE;

      R = NULL;  /* NOTE: for compiler warnings only */

      for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
        {
        R = MREGetRsv(RE);

        if ((bmisset(&R->Flags,mrfOwnerPreempt) == FALSE) ||
            (bmisset(&R->Flags,mrfIsActive) == FALSE))
          continue;

        if (MCredIsMatch(&PreemptorJ->Credential,R->O,R->OType) == FALSE)
          continue;

        NodeIsPreemptee = TRUE;

        break;
        }  /* END for (rindex) */

      if (NodeIsPreemptee == FALSE)
        continue;

      /* select all non-owner jobs */ 

      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        {
        tJ = N->JList[jindex];

        if (tJ == NULL)
          break;

        if (tJ == (mjob_t *)1)
          continue;

        if (MCredIsMatch(&tJ->Credential,R->O,R->OType) == TRUE)
          continue;

        /* is job unique? */

        for (jindex2 = 0;jindex2 < PJCount;jindex2++)
          {
          if ((PJL != NULL) && (PJL[jindex2] == tJ))
            break;
          }  /* END for (jindex2) */

        if (jindex2 >= PJCount)
          {
          /* add job if unique AND it can be preempted */

          /* if the OwnerPreemptIgnoreMinTime is set on R, or if tJ has met it's
           * min preempt time, add it to the list */

          /* NOTE:  This replaced a call to MJobCanPreempt to determine if tJ
                    could go in the list. We still want OwnerPreempt to trump
                    higher priority, non-owner jobs, but this allows site
                    admins to determine which of PreemptMinTime or
                    OwnerPreempt is their desired behavior */

          if ((tJ->State != mjsSuspended) &&
              ((bmisset(&R->Flags,mrfOwnerPreemptIgnoreMinTime)) ||
              (MJobReachedMinPreemptTime(tJ,NULL) == TRUE)))
            {
            MDB(7,fSCHED) MLog("INFO:     adding job %s to preempt list (OwnerPreemptIgnoreMinTime set or job PreemptMinTime past)\n",tJ->Name);
              
            if (PJL != NULL) 
              PJL[jindex2] = tJ;

            PJCount++;
            }
          else
            {
            MDB(7,fSCHED) MLog("INFO:     not adding job %s to preempt list (OwnerPreemptIgnoreMinTime not set or job PreemptMinTime not past)\n", tJ->Name);
            }
          }  /* END if (PJL[jindex2] != tJ) */

        /* add new resources to PNL */

        for (nindex2 = 0;nindex2 < PNCount;nindex2++)
          {
          MNLGetNodeAtIndex(PNL,nindex2,&tmpN);

          if (tmpN == N)
            break;
          }  /* END for (nindex2) */

        PTCount += N->JTC[jindex]; /* Add the taskcount from this job */
        MNLSetTCAtIndex(PNL,nindex2,N->JTC[jindex]);

        if (nindex2 == PNCount)
          {
          /* unique node located, add node info */

          /* NOTE:  currently only adding node evaluated */

          MNLSetNodeAtIndex(PNL,nindex2,N);

          PNCount++;
          }    /* END if (nindex2 == PNCount) */
        }      /* END for (jindex) */
      }        /* END for (nindex) */

    /* terminate list */

    MNLTerminateAtIndex(PNL,PNCount);
    if (PJL != NULL)
      PJL[PJCount]   = NULL;

    MDB(4,fSCHED) MLog("INFO:     preemptible resources found.  (%d nodes/%d tasks)\n",
      PNCount,
      PTCount);

    if (TaskCount != NULL)
      *TaskCount = PTCount;

    if (NodeCount != NULL)
      *NodeCount = PNCount;

    return(SUCCESS);
    }  /* END if (IsConditional == TRUE) */

  /* NOTE:  should determine list of potential preemptors */

  /* step through reservation list */

  /* if not owner of active job, return empty list */
 
  /* process feasible list if provided */
 
  MCResInit(&PreemptRes);

  if (FNL == NULL)
    {
    /* update PreemptList on new iteration or if new job has been started */
 
    if ((MSched.Time > PreemptListMTime[P->Index]) ||
        (ActiveJobCount[P->Index] != MStat.ActiveJobs) ||
        (Priority != CachePriority))
      {
      MDB(5,fSCHED) MLog("INFO:     refreshing PreemptList for partition %s (prio: %ld)\n",
        MPar[P->Index].Name,
        Priority);

      PreemptJobCache[P->Index][0] = NULL;
 
      nindex = 0;
 
      tmpTC = 0;
 
      for (index = 0;index < MSched.M[mxoNode];index++)
        {
        N = MNode[index];

        if ((N == NULL) || (N->Name[0] == '\0'))
          break;
 
        if (N->Name[0] == '\1')
          continue;
 
        if ((N->PtIndex != P->Index) && (P->Index != 0))
          {
          continue;
          }

        /* node is preemptible if              */
        /* - node is busy or running           */
        /* - active jobs are preemptible       */
        /* - active jobs are of lower priority */
 
        if (MNODEISACTIVE(N) == FALSE)
          {
          continue;
          }

        for (jindex = 0;jindex < N->MaxJCount;jindex++)
          {
          if (N->JList[jindex] == NULL)
            break;

          if (N->JList[jindex] == (mjob_t *)1)
            continue;

          J  = N->JList[jindex];
          TC = N->JTC[jindex];

          /* NOTE:  Why are we not using MJobCanPreempt()? - see usage below */

          if (MJOBISACTIVE(J) == FALSE)
            continue;

          if ((bmisset(&J->Flags,mjfPreemptee)) && 
              (MJobReachedMinPreemptTime(J,NULL) == TRUE))
            {
            JobIsPreemptible = TRUE;
            }
          else
            {
            JobIsPreemptible = FALSE;
            }

          if ((JobIsPreemptible == TRUE) &&
              ((MSched.IgnorePreempteePriority == TRUE) || 
               (J->PStartPriority[P->Index] < Priority)))
            {
            MCResAdd(&PreemptRes,&N->CRes,&RQ->DRes,TC,FALSE);  /* FIXME */

            for (jindex2 = 0;jindex2 < MaxPJC;jindex2++)
              {
              if (PreemptJobCache[P->Index][jindex2] != NULL)
                break;

              if (PreemptJobCache[P->Index][jindex2] == J)
                break; 
              }  /* END for (jindex2) */

            if (PreemptJobCache[P->Index][jindex2] == NULL)
              {
              PreemptJobCache[P->Index][jindex2]     = J;
              PreemptJobCache[P->Index][jindex2 + 1] = NULL;
              }

            MDBO(6,J,fSCHED) MLog("INFO:     job %s on node %s added to preempt list (Proc: %d)\n",
              J->Name,
              N->Name,
              PreemptRes.Procs);
            }
          else
            {
            MDB(7,fSCHED) MLog("INFO:     cannot preempt job %s on node %s (%s) P: %ld < %ld partition %s\n",
              J->Name,
              N->Name,
              bmisset(&J->Flags,mjfPreemptee) ? "priority too high" : "not preemptible",
              Priority,
              J->PStartPriority[P->Index],
              P->Name);
            }
          }  /* END for (jindex) */

        if (PreemptRes.Procs <= 0)
          continue;

        MNLSetNodeAtIndex(PreemptNodeCache[P->Index],nindex,N);
        MNLSetTCAtIndex(PreemptNodeCache[P->Index],nindex,PreemptRes.Procs);
 
        tmpTC += MNLGetTCAtIndex(PreemptNodeCache[P->Index],nindex);

        nindex++;
        }  /* END for (index) */

      MNLTerminateAtIndex(PreemptNodeCache[P->Index],nindex);
 
      MDB(5,fSCHED) MLog("INFO:     nodes placed in PNL[%d] list: %3d (procs: %d)\n",
        P->Index,
        nindex,
        tmpTC);
 
      PreemptListMTime[P->Index] = MSched.Time;
 
      ActiveJobCount[P->Index]   = MStat.ActiveJobs;
 
      PTC[P->Index]      = tmpTC;
      PNC[P->Index]      = nindex;
      }  /* END if (MSched.Time > PreemptListMTime[P->Index])) */
 
    /* copy cached data */

    for (nindex = 0;nindex < PNC[P->Index];nindex++)
      {
      MNLCopyIndex(PNL,nindex,PreemptNodeCache[P->Index],nindex);

      memcpy(
        &PNL[nindex],
        &PreemptNodeCache[P->Index][nindex],
        sizeof(PNL[nindex]));
      }  /* END for (nindex) */

    if (PJL != NULL)
      {
      for (jindex = 0;jindex < MaxPJC;jindex++)
        {
        if (PreemptJobCache[P->Index][jindex] == NULL)
          break;

        PJL[jindex] = PreemptJobCache[P->Index][jindex];
        }

      PJL[jindex] = NULL;
      }

    MNLTerminateAtIndex(PNL,nindex);
 
    tmpTC = PTC[P->Index];
    tmpNC = PNC[P->Index];
    }    /* END if (FNL == NULL) */
  else
    {
    /* process feasible node list */

    char EMsg[MMAX_LINE];
 
    tmpTC  = 0;
    nindex = 0;

    /* sort feasible node list based on NODEALLOCATIONPOLICY */

    if ((MPar[0].NAllocPolicy == mnalMachinePrio) ||
        (MPar[0].NAllocPolicy == mnalCustomPriority))
      {
      MNLSort(FNL,PreemptorJ,MSched.Time,mnalMachinePrio,NULL,FALSE);
      }
 
    for (index = 0;MNLGetNodeAtIndex(FNL,index,&N) == SUCCESS;index++)
      {
      MDB(9,fSCHED) MLog("INFO:     evaluating feasible node %s \n",
        N->Name);

      if ((N->PtIndex != P->Index) && (P->Index != 0) &&
          (N->PtIndex != MSched.SharedPtIndex))
        {
        continue;
        }
 
      /* node is preemptible if:             */
      /* - node is busy or running           */
      /* - active jobs are preemptible       */
      /* - active jobs are of lower priority */
 
      if ((N->State != mnsActive) && (N->State != mnsBusy))
        {
        continue;
        }
 
      index2 = 0;
      njstartindex = -1;
 
      J = NULL;

      /* loop through list of active jobs on node */
 
      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        {
        if (N->JList[jindex] == NULL)
          break;

        if (N->JList[jindex] == (mjob_t *)1)
          continue;

        J = N->JList[jindex];

        MDB(9,fSCHED) MLog("INFO:     evaluating job %s\n",
          J->Name);

        TC = N->JTC[jindex];
 
        if (MJobIsInList(ExcludedPreempteeList,J) == TRUE)
          {
          continue;
          } 

        if ((MJobCanPreempt(PreemptorJ,J,Priority,&MPar[N->PtIndex],EMsg) == TRUE) ||
            (ForceCancel == TRUE))
          {
          tmpJ[index2++] = J;
          tmpJ[index2]   = NULL;

          /* sum up preemptible job resources found on node */

          /* NOTE:  where do we validate that preemptee duration is long enough
                    to allow preeptor to start? */

          /* NOTE:  MCResAdd does not adjust for node access policy impact on 
                    resource allocation */

          /* NOTE:  Using TA below, PreemptRes is no longer used and can be removed */

          MCResAdd(&PreemptRes,&N->CRes,&RQ->DRes,TC,FALSE);  /* FIXME */

          if (PJL != NULL)
            {
            for (jindex2 = 0;PJL[jindex2] != NULL;jindex2++)
              {
              if (PJL[jindex2] == J)
                break;
              }  /* END for (jindex2) */
 
            if (PJL[jindex2] == NULL)
              {
              MDB(5,fSCHED) MLog("INFO:     new preemptible job %s located on node %s (priority: %ld > %ld) partition %s\n",
                J->Name,
                N->Name,
                Priority,
                J->PStartPriority[P->Index],
                P->Name);

              PJL[jindex2]     = J;
              PJL[jindex2 + 1] = NULL;

              if (njstartindex == -1)
                njstartindex = jindex2;
              }
            else
              {
              MDB(5,fSCHED) MLog("INFO:     preemptible job %s located on node %s\n",
                J->Name,
                N->Name);
              }
            }    /* END if (PJL != NULL) */

          if (index2 >= MMAX_PJOB)
            {
            /* buffer is full */
 
            break;
            }
          }    /* END if (MJobCanPreempt(PreemptorJ,J,Priority,NULL) == TRUE) */
        else
          {
          MDB(5,fSCHED) MLog("INFO:     job '%s' is not preemptible by job '%s' (%s)\n",
            J->Name,
            PreemptorJ->Name,
            EMsg);
          }
        }      /* END for (jindex) */

      if ((J == NULL) || 
         ((RQ->DRes.Procs != 0) && (PreemptRes.Procs <= 0)))
        {
        /* no procs on node can be preempted, do not add to node list, 
           continue to next node */

        /* clear newly added jobs */

        if (njstartindex != -1)
          {
          if (PJL != NULL)
            PJL[njstartindex] = NULL;
          }

        continue;
        }

      /* adequate resources available if all preemptible resources are */
      /* preempted.  Must now verify that other reservations do not block */
      /* use of node */

      /* add preemptible rsvs to job access list */

      if (PreemptorJ->RsvAList == NULL)
        { 
        RAListIsEmpty = TRUE;

        PreemptorJ->RsvAList = tmpRAList;

        RATail = 0;
        }
      else
        {
        RAListIsEmpty = FALSE;

        for (jindex = 0;jindex < MMAX_PJOB;jindex++)
          {
          if (PreemptorJ->RsvAList[jindex] == NULL)
            break;
          }

        RATail = jindex;
        }

      /* place all preemptee's into RsvAList for use by MJobGetSNRange() */

      for (jindex = 0;jindex < MMAX_PJOB;jindex++)
        {
        if (jindex > MIN(index2,MMAX_PJOB))
          break;

        if (jindex + RATail >= MMAX_PJOB)
          break;

        PreemptorJ->RsvAList[jindex + RATail] = tmpJ[jindex]->Name;
        }  /* END for (jindex) */
 
      /* terminate list */
 
      PreemptorJ->RsvAList[MIN(MMAX_PJOB,jindex + RATail)] = NULL;
 
      if (MSched.TrackSuspendedJobUsage)
        bmset(&PreemptorJ->IFlags,mjifIsPreempting);

      /* NOTE: MJobGetNRange() will report failure if preemptor WCLimit is 
               > preemptee->RemainingTime.  Preemptee resources should be 
               considered if no subsequent job/admin reservations exist
               which would block preemptor.  FIXME */

      /* look for reservation timeframe conflicts only */

      MSched.IsPreemptTest = TRUE;  /* FIXME - route in as parameter in 5.3+ */

      rc = MJobGetNRange(
             PreemptorJ,
             RQ,
             N,
             MSched.Time + 1,  /* set to Now + 1 to look at preemptible resources */
             &TA,              /* O */ 
             &TD,              /* O */
             NULL,
             NULL,
             FALSE,
             NULL);

      MSched.IsPreemptTest = FALSE;  /* FIXME - route in as parameter in 5.3+ */

      if (MSched.TrackSuspendedJobUsage)
        bmunset(&PreemptorJ->IFlags,mjifIsPreempting);

      /* erase temporary access list data */

      if (RAListIsEmpty == TRUE)
        {
        PreemptorJ->RsvAList = NULL;
        }
      else
        {
        for (jindex = 0;jindex < MMAX_PJOB;jindex++)
          {
          if (jindex >= MIN(index2,MMAX_PJOB))
            break;

          PreemptorJ->RsvAList[jindex + RATail] = NULL;
          }  /* END for (jindex) */
        }

      if (rc == FAILURE) 
        {
        /* must remove all jobs in tmpJ from PJL as a job might preemptbile on
         * one node but not on another (ex. reservation). */

#if 0
        /**
          * 10/6/11 DRW MOAB-2326 and MOAB-2714
          *
          * A big change was made in MJobSelectMNL to use PNL rather than LocalFNL.
          * This change as always intended but had never been implemented.  It was
          * finally implemented to fix a bug where a job would run on resources
          * it did not have access to (credlocked).  
          *
          * This code was implemented as a temporary fix but now that the bug is really 
          * fixed it won't be needed.
          *
          * Basically this routine will return 2 things: list of nodes where the job can
          * run and a list of jobs that are preemptible.  When these two things are 
          * "and'ed" together (in MJobSelectPreemptees) then we get the real list of jobs
          * that we should preempt.
          */

        MURemoveJobListFromArray(PJL,tmpJ);
#endif

        continue;
        }

      /* NOTE: use TA, not PreemptRes.Procs because TA reflects impact of node
               access policy on resource availability */

      /* NOTE: DRW wonders RE. RT5871, is it possible that TA gets double 
               counted with ITC in MJobSelectMNL? Who knows? */
               
      MNLSetNodeAtIndex(PNL,nindex,N);
      MNLSetTCAtIndex(PNL,nindex,TA);

      tmpTC += TA;
 
      nindex++;

      MDB(5,fSCHED) MLog("INFO:     node %s contains preemptible resources (T: %d)\n",
        N->Name,
        TA);
      }  /* END for (nindex) */

    /* terminate list */

    MNLTerminateAtIndex(PNL,nindex);
 
    tmpNC = nindex;
    }    /* END else (FNL == NULL) */

  MCResFree(&PreemptRes);

  MDB(4,fSCHED) MLog("INFO:     %d/%d preemptible nodes/tasks found.\n",
    tmpNC,
    tmpTC);
   
  if (TaskCount != NULL)
    *TaskCount = tmpTC;
 
  if (NodeCount != NULL)
    *NodeCount = tmpNC;

  if ((PJL != NULL) && (PJL[0] == NULL))
    {
    /* no preemptible jobs located */

    MDB(2,fSCHED) MLog("INFO:     no preemptible resources found for job %s (tc: %d  class: '%s'  qos: %s  priority: %ld) partition %s\n",
      PreemptorJ->Name,
      PreemptorJ->Request.TC,
      (PreemptorJ->Credential.C != NULL) ? PreemptorJ->Credential.C->Name : "",
      (PreemptorJ->Credential.Q != NULL) ? PreemptorJ->Credential.Q->Name : "",
      PreemptorJ->PStartPriority[P->Index],
      P->Name);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNodeGetPreemptList() */
/* END MNodeGetPreemptList.c */
