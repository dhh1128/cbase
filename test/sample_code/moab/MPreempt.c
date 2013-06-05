/* HEADER */

/**
 * @file MPreempt.c
 *
 * Moab Preempts
 */
 
 
#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
 

typedef struct
  {
  mjob_t     *J;
  double      Cost;
  mcres_t     Res;
  int         Nodes;
  mnl_t       TL;

  mbool_t     JobWillBeUsed;
  } __mpreempt_prio_t;

/* internal prototypes */

int __MJobPreemptPrioComp(__mpreempt_prio_t *,__mpreempt_prio_t *);
int __MJobPreemptTCComp(__mpreempt_prio_t *,__mpreempt_prio_t *);
int __MJobSelectBGPFindExactSize(mjob_t *,mnl_t *,__mpreempt_prio_t *);
int __MJobSelectBGPFindSmallSize(mjob_t *,mnl_t *,mnl_t *,__mpreempt_prio_t *);
mulong __MJobGetLatestPreempteeEndTime(mnl_t *);
double __MJobGetPreempteeCost(mjob_t *,int);
void __MJobSortBGPJobList(mjob_t *,mjob_t **,int);

/* END internal prototypes */


#define MMAX_PPLIST    32 /* partial preempt list (number of nodes to consider) */
  

/**
 * Select list of jobs which are preemptible by job 'PreemptorJ'.
 *
 * NOTE: Only return up to MDEF_MAXJOB possible solutions 
 * 
 * NOTE: Allow preemption if:
 *
 *  A) preemptee duration meets or exceeds preemptor duration
 *  B) preemptee duration is less than preemptor duration and subsequent node 
 *     reservations are also preemptible.  However, in this case, do not take 
 *     any immediately action on subsequent reservation.
 *
 * NOTE:  process list of potential feasible jobs, prioritizing list and then
 *        aggregating preemptees into buckets to satisfy TPN requirements of
 *        the preemptor.
 *
 * DESIGN:  PREEMPTION
 *
 *   MJobSelectMNL() 
 *     - selects available idle resources
 *     - determines if preemptible resources are required
 *    MNodeGetPreemptList() 
 *      - determines which jobs are preemptible and what resources can be provided by each
 *    MJobSelectPreemptees()
 *      - determines optimal combination of preemptees required to allow preemptor to start
 *    MJobPreempt() 
 *      - send preempt request to RM
 *
 * NOTE:  To be valid preemption target, resource must be in FNL[] AND owned by 
 *        job in FeasibleJobList[].
 *
 * NOTE:  Report all tasks which become available if job is preempted even if
 *        tasks exceed requirements of preemptor - use XXX to select tasks from
 *        those available.
 *                           
 *                           
 * @see MJobSelectMNL() - parent
 * @see MJobGetRunPriority() - child
 *
 * @param PreemptorJ        (I)
 * @param RQ                (I)
 * @param ReqTC             (I)
 * @param ReqNC             (I)
 * @param FeasibleJobList   (I) [terminated w/NULL]
 * @param FNL               (I)
 * @param INL               (I)
 * @param PreempteeJList    (O) [minsize=MDEF_MAXJOB] list of preemptible jobs
 * @param PreempteeTCList   (O) [minsize=MDEF_MAXJOB] proc count
 * @param PreempteeNCList   (O) [minsize=MDEF_MAXJOB] 
 * @param PreempteeTaskList (O) [minsize=MDEF_MAXJOB,returns array of alloc nodelists]
 */

int MPreemptorSelectPreempteesOld(

  mjob_t      *PreemptorJ,
  mreq_t      *RQ,
  int          ReqTC,
  int          ReqNC,
  mjob_t     **FeasibleJobList,
  const mnl_t *FNL,
  mnl_t      **INL,
  mjob_t     **PreempteeJList,
  int         *PreempteeTCList,
  int         *PreempteeNCList,
  mnl_t      **PreempteeTaskList)

  {
  mjob_t     *J;
  mnode_t    *N;
  mrsv_t     *R;

  mjob_t     *PreempteeJ;

  int         TC;
  int         JC;

  static __mpreempt_prio_t pJ[MDEF_MAXJOB + 1];  /* only consider up to MDEF_MAXJOB jobs for preemption */

  mnl_t       tmpTaskList = {0};
  mcres_t     PreemptRes;

  int index;
  int jindex;
  int jindex2;
  int nindex;
  int tindex;

  int ppnindex;
  int ppjindex;

  int ReqTPN;                                         /* required tasks per node */
  int TotalTC;
  int TotalNC;

  double TotalCost;

  int NC;
  int PreemptPC;                                      /* number of procs required for preemption */

  mbool_t IsPreemptor;

  int    PJCount;                                     /* preemptible job count */

  double CostPerTask;
  double MaxCostPerTask;

  int     PPNIndex[MMAX_PPLIST + 1];               /* index of node on which partial preempt resources located */
  mjob_t *PPJList[MMAX_PPLIST + 1][MMAX_PJOB + 1]; /* list of jobs which provide partial preempt resources */
  int     PPBJTC[MMAX_PPLIST + 1][MMAX_PJOB + 1];  /* partial preempt taskcount blocked from correspnding job 
                                                         on specified node (lost due to ppn) */

  int     ATC;   /* available task count */
  int     BTC;   /* preemptible tasks which cannot be utilized due to job TPN constraints */

  const char *FName = "MPreemptorSelectPreempteesOld";

  MDB(4,fSCHED) MLog("%s(%s,%d,%d,%s,%s,%s,%s,%s,PTL)\n",
    FName,
    (PreemptorJ != NULL) ? PreemptorJ->Name : "NULL",
    ReqTC,
    ReqNC,
    (FeasibleJobList != NULL) ? "FJobList" : "NULL",
    (FNL != NULL) ? "FNL" : "NULL",
    (PreempteeJList != NULL)  ? "PJList" : "NULL",
    (PreempteeTCList != NULL) ? "PTCList" : "NULL",
    (PreempteeNCList != NULL) ? "PNCList" : "NULL");

  if ((PreemptorJ == NULL) ||
      (FeasibleJobList == NULL) ||
      (FeasibleJobList[0] == NULL) ||
      (FNL == NULL) ||
      (PreempteeJList == NULL) ||
      (PreempteeTCList == NULL) ||
      (PreempteeNCList == NULL))
    {
    if ((FeasibleJobList != NULL) && (FeasibleJobList[0] == NULL))
      {
      MDB(4,fSCHED) MLog("INFO:     empty feasible job list for job %s\n",
        PreemptorJ->Name);
      }
    else
      {
      MDB(1,fSCHED) MLog("ALERT:    invalid parameters passed in %s\n",
        FName);
      }

    return(FAILURE);
    }

  /* NOTE:  select 'best' list of jobs to preempt so as to provide */
  /*        needed tasks/nodes for preemptorJ on feasiblenodelist  */
  /*        lower 'run' priority means better preempt candidate    */

  /* determine number of available tasks associated with each job */

  index = 0;

  MCResInit(&PreemptRes);
  MNLInit(&tmpTaskList);

  /* NOTE:  currently only support single req preemption */

  for (jindex = 0;FeasibleJobList[jindex] != NULL;jindex++)
    {
    if (jindex >= MSched.SingleJobMaxPreemptee)
      {
      MDB(1,fSCHED) MLog("INFO:     single job max preemptee limit (%d) reached\n",
        MSched.SingleJobMaxPreemptee);

      break;
      }

    PreempteeJ = FeasibleJobList[jindex];

    MCResClear(&PreemptRes);

    NC = 0;

    tindex = 0;

    for (nindex = 0;MNLGetNodeAtIndex(FNL,nindex,&N) == SUCCESS;nindex++)
      {
      if (bmisset(&PreemptorJ->Flags,mjfPreemptor) != TRUE)
        {
        mre_t *RE;

        /* job is not global preemptor but may have resource-specific preemption 
           rights */

        IsPreemptor = FALSE;

        /* determine if 'ownerpreempt' is active */

        for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
          {
          R = MREGetRsv(RE);

          if ((bmisset(&R->Flags,mrfOwnerPreempt) == FALSE) || 
              (bmisset(&R->Flags,mrfIsActive) == FALSE))
            continue;

          if (MCredIsMatch(&PreemptorJ->Credential,R->O,R->OType) == FALSE)
            continue;

          IsPreemptor = TRUE;

          break;
          }  /* END for (rindex) */

        if (IsPreemptor == FALSE)
          {
          continue;
          }
        }    /* END if (bmisset(&PreemptorJ->Flags,mjfPreemptor) != TRUE) */

      for (jindex2 = 0;jindex2 < MMAX_JOB_PER_NODE;jindex2++)
        {
        J = (mjob_t *)N->JList[jindex2];

        if (J == NULL)
          break;

        if (J == (mjob_t *)1)
          continue;

        if (J != PreempteeJ)
          continue;

        TC = N->JTC[jindex2];

        if (J->RemainingTime < PreemptorJ->SpecWCLimit[0])
          {
          /* verify node does not possess non-preemptible rsvs which would block
             preemptor from executing */

          /* NYI */
          }

        if (bmisset(&J->Flags,mjfIgnIdleJobRsv))
          {
          mre_t *RE;

          long PreemptorETime;  /* preemptor end time */

          mbool_t IsExclusiveRsv = FALSE;

          /* NOTE:  The following only works with the assumption that node is
                    dedicated
    
             NOTE:  This capability only used by LLNL 
          */

          /* do not consider preemptee if any job reservations are detected 
             within Preemptor WCLimit, and job reservation is not preemptible
          */

          PreemptorETime = MSched.Time + PreemptorJ->SpecWCLimit[0];

          for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
            {
            if ((RE->Type == mreStart) && 
                (RE->Time < PreemptorETime))
              {
              mrsv_t *R = MREGetRsv(RE);

              /* rsv located which overlaps with preemptor */

              if ((R->Type == mrtJob) &&
                  (R->J == J))
                {
                /* rsv event is preemptee, ignore */

                continue;
                }

              /* NOTE:  this is pessimistic - idle job rsv's should be ignore-able */
           
              IsExclusiveRsv = TRUE;

              break;
              }

            if (RE->Time > PreemptorETime)
              {
              /* all remaining rsv's are after preemptor completion */

              break;
              }
            }  /* END for (reindex) */

          if (IsExclusiveRsv == TRUE)
            {
            /* no resources available from this job */

            continue;
            }
          }    /* END if (bmisset(&J->Flags,mjfIgnIdleJobRsv)) */

        /* job is preemptible if 
             'preemptee' flag is set 
             or job w/in 'ownerpreempt' reservation */

        /* NOTE:  feasible job list should only contain preemptible jobs 
         * which meet priority requirements */

        if ((N->JList != NULL) &&
            (N->JList[0] != NULL) &&
            (N->JList[1] == NULL) &&
           ((N->EffNAPolicy == mnacSingleJob) ||
            (N->EffNAPolicy == mnacSingleTask)))
          {
          /* if single job running on node and node access policy blocks
             non-dedicated resource access, assume all node resources become
             available when job is preempted */

          /* NOTE:  if node has user/system reservation in place over subset of
                    resources, issues may arise (not yet handled) */

          MCResAdd(&PreemptRes,&N->CRes,&N->CRes,TC,FALSE);
          }
        else if ((N->JList != NULL) &&
                 (N->JList[0] != NULL) &&
                 (N->JList[1] == NULL) &&
               (((N->EffNAPolicy == mnacSingleUser) &&
                 (J->Credential.U != PreemptorJ->Credential.U)) ||
                ((N->EffNAPolicy == mnacSingleGroup) &&
                 (J->Credential.G != PreemptorJ->Credential.G)) ||
                ((N->EffNAPolicy == mnacSingleAccount) &&
                 (J->Credential.A != PreemptorJ->Credential.A))))
          {
          MCResAdd(&PreemptRes,&N->CRes,&N->CRes,TC,FALSE);
          }
        else
          {
          /* job dedicated resources are available */

          MCResAdd(&PreemptRes,&N->CRes,&RQ->DRes,TC,FALSE);
          }

        NC += 1;

        MNLSetNodeAtIndex(&tmpTaskList,tindex,N);
        MNLSetTCAtIndex(&tmpTaskList,tindex,TC);

        tindex++;
        }  /* END for (jindex2) */
      }    /* END for (nindex) */

    /* NOTE:  restore to loglevel 6 once MSIC bug is isolated */

    MDB(2,fSCHED) MLog("INFO:     preemptible job %s provides %d/%d tasks/nodes\n",
      PreempteeJ->Name,
      PreemptRes.Procs,
      NC); 

    if (PreemptRes.Procs <= 0)
      {
      /* job provides no suitable compute (proc) resources, ignore */

      continue;
      }

    MNLTerminateAtIndex(&tmpTaskList,tindex);

    /* determine 'cost per task' associated with job */

    pJ[index].J     = PreempteeJ;
    pJ[index].Nodes = NC;

    MCResInit(&pJ[index].Res);

    MCResCopy(&pJ[index].Res,&PreemptRes);

    PreemptPC = MIN(ReqTC,PreemptRes.Procs);

    pJ[index].Cost = __MJobGetPreempteeCost(pJ[index].J,PreemptPC);

    /* + RTimeWeight favors jobs that will end first - preempt those with shorter remaining times */
    /* - RTimeWeight favors jobs that will end last  - preempt those with longer remaining times */
    /* Larger remaining time = larger cost */

    if (MSched.PreemptRTimeWeight != 0.0)
      pJ[index].Cost += (MSched.PreemptRTimeWeight * (pJ[index].J->RemainingTime / MCONST_MINUTELEN));

    if (PreempteeTaskList != NULL)
      {
      MNLInit(&pJ[index].TL);

      MNLCopy(&pJ[index].TL,&tmpTaskList);

#if defined(MPREEMPTDEBUG)
      fprintf(stderr,"allocated %lu at pJ[%d] for possible preemptee '%s' (%d)\n",
        (unsigned long)&pJ[index].TL.Array,
        index,
        pJ[index].J->Name,
        __LINE__);
#endif /* MPREEMPTDEBUG */
      }

    index++; 

    if (index >= MDEF_MAXJOB)
      break;
    }    /* END for (jindex) */

  MNLFree(&tmpTaskList);
  MCResFree(&PreemptRes);

  /* terminate list */

  pJ[index].J = NULL;

  JC = index;  /* JC is total preemptible jobs available to preemptor */

  /* sort job list by cost - low to high */

  if (index > 1)
    {
    qsort(
      (void *)&pJ[0],
      index,
      sizeof(pJ[0]),
      (int(*)(const void *,const void *))__MJobPreemptPrioComp);
    }

  /* create initial preemption 'schedule' */

  TotalNC = 0;
  TotalTC = 0;
  TotalCost = 0;

  MaxCostPerTask = 0.0;

  ReqTPN = RQ->TasksPerNode;

  PPNIndex[0] = -1;

  /* search job list, lowest cost job first */

  for (jindex = 0;jindex < index;jindex++)
    {
    if ((TotalTC >= ReqTC) && (TotalNC >= ReqNC))
      break;

    PreempteeJList[jindex]  = pJ[jindex].J;
    PreempteeTCList[jindex] = pJ[jindex].Res.Procs;
    PreempteeNCList[jindex] = pJ[jindex].Nodes;

    if (PreempteeTaskList != NULL)
      {
      PreempteeTaskList[jindex] = &pJ[jindex].TL;

#if defined(MPREEMPTDEBUG)
        fprintf(stderr,"setting PreempteeTaskList[%d] to pJ[%d] (%lu) (%d)\n",jindex,jindex,(unsigned long)&pJ[jindex].TL.Array,__LINE__);
#endif /* MPREEMPTDEBUG */
      }

    if (ReqTPN > 1)
      {
      int nindex;

      mnl_t *NL;

      /* search list of nodes in job */

      /* NOTE:  must handle situation where TPN results in job X too small and job Y too small 
                but X+Y is adequate (NYI) */

      /* NOTE: pJ->TL does not take TPN into account */

      NL = &pJ[jindex].TL;

      PreempteeTCList[jindex] = 0;
      PreempteeNCList[jindex] = 0;
 
      for (nindex = 0;nindex < pJ[jindex].Nodes;nindex++)
        {
        MNLGetNodeAtIndex(NL,nindex,&N);

        BTC = MNLGetTCAtIndex(NL,nindex) % ReqTPN;
        ATC = MNLGetTCAtIndex(NL,nindex) - BTC;
       
        PreempteeTCList[jindex] += ATC;

        if (ATC > 0)
          PreempteeNCList[jindex] ++;
 
        if (BTC == 0)
          {
          /* all tasks available from job can be applied to preemptor */

          /* if adequate idle procs available... will already be loaded 
             elsewhere...continue (TPN) */

          continue;
          }

        /* due to TPN, only partial available tasks can be applied */

        /* idle tasks are not considered until further below (Search for "Consider Idle" and "Add Idle") */
        /* considering idle tasks in this location could lead to double-counting as we are looking at nodes
           on a per job basis */
      
        /* MNOT added 9/5/08 by DRW to address following situation:
           4 proc node
           job1: nodes=1
           job2: nodes=1
           preemptorjob: nodes=1:ppn=4
        */

#ifdef MNOT
        if ((RQ->DRes.Procs == 0) || (N->ARes.Procs > 0))
          {
          /* add idle tasks to preemptible tasks to determine if TPN can be 
             satisfied */

          TC = MNodeGetTC(
            N,
            &N->ARes,
            &N->CRes,
            &N->DRes,
            &RQ->DRes,
            MSched.Time,
            1,
            NULL);
          }
        else
          {
          TC = 0;
          }

        if (TC > 0)
          {
          EffectiveNTC = NL[nindex].TC + TC -
                         (NL[nindex].TC + TC) % ReqTPN;

          /* NOTE:  line below will result in double counting of available 
                    preemptible tasks (FIXME) */

          /*        modify PreempteeTCList[] by ATC first?  other? */

          TotalTC += EffectiveNTC;


          /* INL[0][nindex2].TC -= EffectiveNTC - NL[nindex].TC; */

          /* NL[nindex].TC = EffectiveNTC; */

          /* NOTE:  too optimistic, one idle proc may be applied to multiple 
                    preemptible jobs */


          /**********************************************
           * new code added 08/05/08 by DRW
           **********************************************/

          /* To address the case of a 4 proc node with 2 procs 
             being used by 1 preemptee job and a 4 proc preemptor 
             job being scheduled.  In that case the availble 2 procs
             were not being counted as available */

          PreempteeTCList[jindex] += TotalTC;
          }  /* END if (TC > 0) */
        else
#endif  /* MNOT */
          {
          /* assign job to partial preempt list */

          for (ppnindex = 0;ppnindex < MMAX_PPLIST;ppnindex++)
            {
            if (N->Index == PPNIndex[ppnindex])
              {
              /* match located */

              for (ppjindex = 0;ppjindex < MMAX_PJOB;ppjindex++)
                {
                if (PPJList[ppnindex][ppjindex] == NULL)
                  {
                  PPJList[ppnindex][ppjindex] = pJ[jindex].J;
                  PPBJTC[ppnindex][ppjindex]  = BTC;

                  PPJList[ppnindex][ppjindex + 1] = NULL;
                  
                  break;
                  }
                }

              break;
              }  /* END if (N->Index == PPNIndex[ppnindex]) */

            if (PPNIndex[ppnindex] == -1)
              {
              /* end of list reached */

              PPNIndex[ppnindex] = N->Index;
              PPNIndex[ppnindex + 1] = -1;

              PPJList[ppnindex][0] = pJ[jindex].J;
              PPJList[ppnindex][1] = NULL;

              PPBJTC[ppnindex][0]  = BTC;

              break;
              }
            }    /* END for (ppnindex) */
          }      /* END else */
        }        /* END for (nindex) */

      TotalTC += PreempteeTCList[jindex];
      TotalNC += PreempteeNCList[jindex];
      }  /* END if (ReqTPN > 1) */
    else
      {
      TotalTC += PreempteeTCList[jindex];
      TotalNC += PreempteeNCList[jindex];
      }

    TotalCost += pJ[jindex].Cost;


    CostPerTask = pJ[jindex].Cost / pJ[jindex].Res.Procs;

    if (CostPerTask > MaxCostPerTask)
      {
      MaxCostPerTask = CostPerTask;
      }
    }  /* END for (jindex) */

  /* terminate lists */

  PJCount = jindex;

  if (PreempteeTaskList != NULL)
    {
    /* free all tasklists which are not used */

    for (jindex = PJCount;jindex < JC;jindex++)
      {
#if defined(MPREEMPTDEBUG)
      fprintf(stderr,"freeing %lu at pJ[%d] (%d)\n",(unsigned long)&pJ[jindex].TL.Array,jindex,__LINE__);
#endif /* MPREEMPTDEBUG */

      MNLFree(&pJ[jindex].TL);
      }
    
#if defined(MPREEMPTDEBUG)
      fprintf(stderr,"setting PreempteeTaskList[%d] to NULL (%d)\n",PJCount,__LINE__);
#endif /* MPREEMPTDEBUG */

    PreempteeTaskList[PJCount] = NULL;
    }

  PreempteeTCList[PJCount] = 0;
  PreempteeNCList[PJCount] = 0;
  PreempteeJList[PJCount]  = NULL;

  if ((PJCount > 0) && bmisset(&PreemptorJ->Flags,mjfBestEffort) && (TotalTC > 0))
    {
    MDB(2,fSCHED) MLog("INFO:     inadequate preempt jobs (%d) located for best effort job (P: %d of %d,N: %d of %d)\n",
      PJCount,
      TotalTC,
      ReqTC,
      TotalNC,
      ReqNC);
    }
  else if ((PJCount == 0) || 
      (ReqTC > TotalTC) || 
      (ReqNC > TotalNC))
    {
    /* inadequate resources located */

    /* if multiple partial jobs detected, determine if any partials share node */

    if ((PPNIndex[0] != -1) && (ReqTPN > 1))
      {
      int jindex2;

      /* on per node basis, sum resources available from each preemptee job */

      for (nindex = 0;nindex < MMAX_PPLIST;nindex++)
        {
        if (PPNIndex[nindex] == -1) 
          {
          /* end of list */

          break;
          }

        if ((TotalTC >= ReqTC) && (TotalNC >= ReqNC))
          {
          break;
          }

        if (PPJList[nindex][0] == NULL)
          {
          /* empty list, ignore - if there is just one job, keep it because we
             still need to add idle tasks */

          continue;
          }

        N = MNode[PPNIndex[nindex]];

        /* Compiler warning - N may not be initialized */

        TC = MNodeGetTC(
          N,
          &N->ARes,
          &N->CRes,
          &N->DRes,
          &PreemptorJ->Req[0]->DRes,
          MSched.Time,
          1,
          NULL);

        /* "Consider Idle" tasks here */

        ATC = TC;

        /* append possible joint solutions */

        for (jindex = 0;jindex < PJCount;jindex++)
          {
          if (PPJList[nindex][jindex] == NULL)
            break;

          ATC += PPBJTC[nindex][jindex];

          /* if possible joint solution found, append to list */
          }  /* END for (jindex) */

        if (ATC < ReqTPN)
          {
          /* all partials together cannot add a full task */

          continue;
          }

        /*  partial + idle is sufficient for preemption */

        /* "Add Idle" tasks here */

        TotalTC += TC;

        /* add preemptible tasks to master preemptible job list */

        for (jindex = 0;jindex < MMAX_PJOB;jindex++)
          {
          if (PPJList[nindex][jindex] == NULL)
            break;

          for (jindex2 = 0;jindex2 < MDEF_MAXJOB;jindex2++)
            {
            if (PreempteeJList[jindex2] == NULL)
              {
              /* end of list reached */

              break;
              }

            if (PreempteeJList[jindex2] == PPJList[nindex][jindex])
              {
              /* job tasks were added */

              PreempteeTCList[jindex2] += PPBJTC[nindex][jindex];
              PreempteeNCList[jindex2] ++;

              TotalTC   += PPBJTC[nindex][jindex];

              TotalNC++;

              ATC += PPBJTC[nindex][jindex];

              /* can we break here? */
              }
            }    /* END for (jindex2) */
          }      /* END for (jindex) */
        }        /* END for (nindex) */

      /* terminate new list */

      if (PreempteeTaskList != NULL)
        {
        /* free all tasklists which are not used */

        for (jindex = PJCount;jindex < JC;jindex++)
          {
#if defined(MPREEMPTDEBUG)
          fprintf(stderr,"freeing %lu at pJ[%d] (%d)\n",(unsigned long)pJ[jindex].TL.Array,jindex,__LINE__);
#endif /* MPREEMPTDEBUG */

          MNLFree(&pJ[jindex].TL);
          }

#if defined(MPREEMPTDEBUG)
      fprintf(stderr,"setting PreempteeTaskList[%d] to NULL (%d)\n",PJCount,__LINE__);
#endif /* MPREEMPTDEBUG */

        PreempteeTaskList[PJCount] = NULL;
        }  /* END if (PreempteeTaskList != NULL) */

      PreempteeTCList[PJCount] = 0;
      PreempteeNCList[PJCount] = 0;
      PreempteeJList[PJCount]  = NULL;
      }  /* END (PPNIndex[0] != -1) */

    if ((PJCount == 0) ||
        (ReqTC > TotalTC) ||
        (ReqNC > TotalNC))
      {
      /* inadequate preemptible resources located */

      MDB(2,fSCHED) MLog("INFO:     inadequate preempt jobs (%d) located (P: %d of %d,N: %d of %d)\n",
        jindex,
        TotalTC,
        ReqTC,
        TotalNC,
        ReqNC);

      /* free alloc memory */

      if (PreempteeTaskList != NULL)
        {
        for (jindex = 0;jindex < JC;jindex++)
          {
#if defined(MPREEMPTDEBUG)
          fprintf(stderr,"freeing %lu at pJ[%d] (%d)\n",(unsigned long)pJ[jindex].TL.Array,jindex,__LINE__);
#endif /* MPREEMPTDEBUG */

          MNLFree(&pJ[jindex].TL);
          }  /* END for (jindex) */

#if defined(MPREEMPTDEBUG)
      fprintf(stderr,"setting PreempteeTaskList[0] to NULL (returning failure %d)\n",__LINE__);
#endif /* MPREEMPTDEBUG */

	PreempteeTaskList[0] = NULL;
        }

      return(FAILURE);
      }  /* END if ((PJCount == 0) || ...) */
    }    /* END if ((PJCount == 0) || ...) */

  if ((jindex <= MSched.SingleJobMaxPreemptee) && (jindex < JC))
    {
    /* optimize only reasonably small schedules and schedules where 
       additional job options are available */

    mbool_t OptSchedLocated = FALSE;

    int     PSched[MMAX_PJOB + 1];

    int     jindex2;

    /* NOTE:  determine lowest cost schedule */

    /* jindex is total number of jobs in current solution */

    for (jindex2 = jindex;jindex2 < JC;jindex2++)
      {
      CostPerTask = pJ[jindex2].Cost / pJ[jindex2].Res.Procs;

      if (CostPerTask > MaxCostPerTask)
        {
        /* no improved solution can be located */

        break;
        }
      }  /* END for (jindex2) */

    if ((jindex > 1) && (pJ[1].Res.Procs >= ReqTC))
      {
      /* NOTE:  temporary hack */

      OptSchedLocated = TRUE;

      PSched[0] = 1;
      PSched[1] = -1;
      }

    if (OptSchedLocated == TRUE)
      {
      int offset;

      /* overwrite base schedule */

      offset = 0;

      for (jindex2 = 0;jindex2 < MSched.SingleJobMaxPreemptee;jindex2++)
        {
        if (PSched[jindex2] == -1)
          break;

        if (pJ[PSched[jindex2]].Res.Procs <= 0)
          {
          /* remove preemptees which cannot contribute due to Preemptor TPN
             constraints */
#if defined(MPREEMPTDEBUG)
          fprintf(stderr,"freeing %lu at pJ[%d] (%d)\n",(unsigned long)pJ[PSched[jindex2]].TL.Array,PSched[jindex2],__LINE__);
#endif /* MPREEMPTDEBUG */

          MNLFree(&pJ[PSched[jindex2]].TL);

          offset++;

          continue;
          }

        PreempteeJList[jindex2 - offset]  = pJ[PSched[jindex2]].J;
        PreempteeTCList[jindex2 - offset] = pJ[PSched[jindex2]].Res.Procs;
        PreempteeNCList[jindex2 - offset] = pJ[PSched[jindex2]].Nodes;

        if (PreempteeTaskList != NULL)
          {
#if defined(MPREEMPTDEBUG)
          fprintf(stderr,"setting PreempteeTaskList[%d] to PJ[%d] (%d)\n",jindex2 - offset, PSched[jindex2],__LINE__);
#endif /* MPREEMPTDEBUG */

          PreempteeTaskList[jindex2 - offset] = &pJ[PSched[jindex2]].TL;
          }
        }  /* END for (jindex2) */

      if (PreempteeTaskList != NULL)
        {
        int oindex;

        for (oindex = 1;oindex <= offset;oindex++)
          {
#if defined(MPREEMPTDEBUG)
          fprintf(stderr,"freeing %lu at PreempteeTaskList[%d] (%d)\n",(unsigned long)PreempteeTaskList[jindex2 - oindex],jindex2 - oindex,__LINE__);
#endif /* MPREEMPTDEBUG */

          MNLFree(PreempteeTaskList[jindex2 - oindex]);
          }
        }

      PreempteeTCList[jindex2 - offset] = 0;
      PreempteeNCList[jindex2 - offset] = 0;
      PreempteeJList[jindex2 - offset]  = NULL;
      }  /* END if (OptSchedLocated == TRUE) */
    }    /* END if ((jindex <= MSched.SingleJobMaxPreemptee) && ...) */ 
  else if (ReqTPN > 1)
    {
    int offset;

    /* remove preemptees which cannot contribute due to Preemptor TPN constraints */

    offset = 0;

    for (jindex2 = 0;jindex2 < MSched.SingleJobMaxPreemptee;jindex2++)
      {
#if defined(MPREEMPTDEBUG)
      fprintf(stderr,"loop info: MAX(%d) jindex2(%d) offset(%d) (%d)\n",
        MSched.SingleJobMaxPreemptee,
        jindex2,
        offset,
        __LINE__);
#endif /* MPREEMPTDEBUG */

      if (PreempteeJList[jindex2] == NULL)
        {
        /* end of list reached */

        break;
        }

      if (PreempteeTCList[jindex2] <= 0)
        {
        /* remove preemptees which cannot contribute due to Preemptor TPN
           constraints */

#if defined(MPREEMPTDEBUG)
        fprintf(stderr,"freeing %lu at PreempteeTaskList[%d] (%d)\n",
          (unsigned long)PreempteeTaskList[jindex2],
          jindex2,
          __LINE__);
#endif /* MPREEMPTDEBUG */


        MNLFree(PreempteeTaskList[jindex2]);

        offset++;

        continue;
        }

      if (offset > 0)
        {
        PreempteeJList[jindex2 - offset]  = PreempteeJList[jindex2];
        PreempteeTCList[jindex2 - offset] = PreempteeTCList[jindex2];
        PreempteeNCList[jindex2 - offset] = PreempteeNCList[jindex2];

        if (PreempteeTaskList != NULL)
          {
#if defined(MPREEMPTDEBUG)
          fprintf(stderr,"setting PreempteeTaskList[%d](%lu) to PreempteeTaskList[%d](%lu) (%d)\n",
            jindex2 - offset,
            (unsigned long)PreempteeTaskList[jindex2 - offset],
            jindex2,
            (unsigned long)PreempteeTaskList[jindex2],
            __LINE__);
#endif /* MPREEMPTDEBUG */

          PreempteeTaskList[jindex2 - offset] = PreempteeTaskList[jindex2];

          PreempteeTaskList[jindex2] = NULL;
          }
        }
      }  /* END for (jindex2) */

    /* terminate list */

    if (PreempteeTaskList != NULL)
      {
      /* free unused tasklists */

      int oindex;

      for (oindex = 1;oindex <= offset;oindex++)
        {
#if defined(MPREEMPTDEBUG)
        fprintf(stderr,"freeing %lu at PreempteeTaskList[%d] (%d)\n",(unsigned long)PreempteeTaskList[jindex2 - oindex],jindex2 - oindex,__LINE__);
#endif /* MPREEMPTDEBUG */

        MNLFree(PreempteeTaskList[jindex2 - oindex]);
        }
      }

#if defined(MPREEMPTDEBUG)
    {
    int oindex;

    for (oindex = 0;oindex <= 20;oindex++)
      {
      fprintf(stderr,"PreempteeTaskList[%d] = %lu (%d)\n",oindex,(unsigned long)PreempteeTaskList[oindex],__LINE__);
      }
    }
#endif /* MPREEMPTDEBUG */

    PreempteeTCList[jindex2 - offset] = 0;
    PreempteeNCList[jindex2 - offset] = 0;
    PreempteeJList[jindex2 - offset]  = NULL;
    }  /* END else if (ReqTPN > 1) */
  else
    {
    /* solution is exactly the set of preemptees available - copy over needed
       variables */

    for (jindex2 = 0;jindex2 < JC;jindex2++)
      {
      if (PreempteeTaskList != NULL)
        {
        PreempteeTaskList[jindex2] = &pJ[jindex2].TL;
        }
      }  /* END for (jindex2) */

    PreempteeTCList[jindex2] = 0;
    PreempteeNCList[jindex2] = 0;
    PreempteeJList[jindex2]  = NULL;

    if (PreempteeTaskList != NULL)
      {
      PreempteeTaskList[jindex2] = NULL;
      }
    }  /* END else */

  for (jindex = 0;jindex <  MDEF_MAXJOB + 1;jindex++)
    {
    MCResFree(&pJ[jindex].Res);
    }

  return(SUCCESS);
  }  /* END MPreemptorSelectPreempteesOld() */


/**
 * Get cost of preemptee job.
 *
 * @param Preemptee (I)
 * @param PreemptProcCount (I)
 */

double __MJobGetPreempteeCost(

  mjob_t *Preemptee,        /* I */
  int     PreemptProcCount) /* I*/

  {
  double cost = 0.0;
  double JobRunPriority = 0.0;

  MJobGetRunPriority(
    Preemptee,
    0,
    &JobRunPriority,
    NULL,
    NULL);

  if (MSched.PreemptPrioWeight == 0.0)
    {
    /* cost is size of job */

    cost = Preemptee->Request.TC;
    }
  else if (MSched.PreemptPrioWeight == -1.0)
    {
    /* cost is wasted resource percentage for job */

    cost = 
      ((MPar[0].UpRes.Procs == 0) || (PreemptProcCount == 0)) ? 0 :
      (1.0 + (double)(MAX(0,Preemptee->Request.TC - PreemptProcCount)) / 
      MPar[0].UpRes.Procs) /
      PreemptProcCount;
    }
  else
    {
    /* cost is increased by job priority, is increased by number of procs 
       currently allocated to preemptee which cannot be used by preemptor, 
       and is decreased by number of procs provided to preemptor */

    cost = 
      ((MPar[0].UpRes.Procs == 0) || (PreemptProcCount == 0)) ? 0 :
      (double)JobRunPriority * MSched.PreemptPrioWeight * 
      (1.0 + (double)(MAX(0,Preemptee->Request.TC - PreemptProcCount)) / MPar[0].UpRes.Procs) / 
      PreemptProcCount;
    }

  return(cost);
  } /* END __MJobGetPreempteeCost() */



/**
 * Slurm expects the preemptee job list to be sorted according to those
 * that should be preempted first. So, sort the preemptees according to same size
 * and then small size first.
 *
 * @param PreemptorJ (I)
 * @param FeasibleJobList (I)
 * @param INL Idle (I/O) nodes + preemptee nodes
 * @param PreempteeJList (O)
 * @param PreempteeTCList (O)
 * @param PreempteeNCList (O)
 * @param PreempteeTaskList (O)
 */

int MJobSelectBGPJobList(

  mjob_t      *PreemptorJ,        /* I */
  mjob_t     **FeasibleJobList,   /* I (terminated w/NULL) */
  mnl_t       *INL,               /* I/O */
  mjob_t     **PreempteeJList,    /* O list of preemptible jobs (minsize=MDEF_MAXJOB) */
  int         *PreempteeTCList,   /* O proc count (minsize=MDEF_MAXJOB) */
  int         *PreempteeNCList,   /* O (minsize=MDEF_MAXJOB) */
  mnl_t      **PreempteeTaskList) /* O (minsize=MDEF_MAXJOB,returns array of alloc nodelists) */

  {
  int jIndex;
  char tmpEMsg[MMAX_LINE];
  mnl_t FNL = {0}; /* Idle + Preemptee nodes */

  MDB(7,fWIKI) MLog("INFO:     determining which bluegene jobs to preempt\n");

  MNLInit(&FNL);

  MNLMerge(&FNL,&FNL,INL,NULL,NULL);

  /* copy feasible list which will be finalized by MJobWillRun */

  for (jIndex = 0;jIndex < (MMAX_JOB - 1);jIndex++)
    {
    if (FeasibleJobList[jIndex] == NULL)
      break;

    PreempteeJList[jIndex] = FeasibleJobList[jIndex];

    MNLMerge(&FNL,&FNL,&FeasibleJobList[jIndex]->NodeList,NULL,NULL);
    }

  PreempteeJList[jIndex] = NULL;

  __MJobSortBGPJobList(PreemptorJ,PreempteeJList,jIndex);

  if (MJobWillRun(
      PreemptorJ,
      &FNL,                    /* I/O */
      PreempteeJList,          /* I/O */
      MSched.Time + MMAX_TIME, /* I */
      NULL,
      tmpEMsg) == FAILURE)
    {
    MNLFree(&FNL);

    MDB(7,fWIKI) MLog("INFO:     preemptor %s unable to run now.\n",PreemptorJ->Name);

    return(FAILURE);
    }

  for (jIndex = 0;PreempteeJList[jIndex] != NULL;jIndex++)
    {
    PreempteeTCList[jIndex] = PreempteeJList[jIndex]->Req[0]->TaskCount;
    PreempteeNCList[jIndex] = PreempteeJList[jIndex]->Req[0]->NodeCount;
    PreempteeTaskList[jIndex] = (mnl_t *)MUMalloc(sizeof(mnl_t));
    MNLCopy(PreempteeTaskList[jIndex],&PreempteeJList[jIndex]->NodeList);
    }

  MNLCopy(INL,&FNL);

  PreempteeTCList[jIndex] = -1;
  PreempteeNCList[jIndex] = -1;
  PreempteeTaskList[jIndex] = NULL;

  MNLFree(&FNL);

  return(SUCCESS);
  }


/**
 * Sort bluegene preemptee job list.
 *
 * Sorts first according to same sized jobs and then from smallest to largest.
 *
 * @param PreemptorJ (I)
 * @param Preemptees (I)
 * @param NumPreemptees (I) Used to determine size of arrays and save space.
 */

void __MJobSortBGPJobList(

  mjob_t  *PreemptorJ,    /* I */
  mjob_t **Preemptees,    /* I/O (modified) */
  int      NumPreemptees) /* I */

  {
  int     sortedIndex = 0;
  int     jindex = 0;
  int     sameIndex = 0;
  int     diffIndex = 0;
  mjob_t *preempteeJ = NULL;

  if ((PreemptorJ == NULL) || (Preemptees == NULL))
    return;

  __mpreempt_prio_t *sameSizeJobs = (__mpreempt_prio_t *)malloc(sizeof(__mpreempt_prio_t) * (NumPreemptees + 1));
  __mpreempt_prio_t *diffSizeJobs = (__mpreempt_prio_t *)malloc(sizeof(__mpreempt_prio_t) * (NumPreemptees + 1));

  /* sort low priority to high - preempt lower priority first */

  for (jindex = 0;Preemptees[jindex] != NULL;jindex++)
    {
    preempteeJ = Preemptees[jindex];

    if (preempteeJ->Request.TC == PreemptorJ->Request.TC)
      {
      sameSizeJobs[sameIndex].J       = preempteeJ;
      sameSizeJobs[sameIndex].Res.Procs   = preempteeJ->Request.TC;
      sameSizeJobs[sameIndex++].Cost  = __MJobGetPreempteeCost(preempteeJ,preempteeJ->Request.TC);
      }
    else
      {
      diffSizeJobs[diffIndex].J       = preempteeJ;
      diffSizeJobs[diffIndex].Res.Procs   = preempteeJ->Request.TC;
      diffSizeJobs[diffIndex++].Cost  = __MJobGetPreempteeCost(preempteeJ,preempteeJ->Request.TC);
      }
    }

  sameSizeJobs[sameIndex].J = NULL;
  diffSizeJobs[diffIndex].J = NULL;
    
  /* Sort same sized jobs */

  if (sameIndex > 1)
    {
    qsort(
      (void *)&sameSizeJobs[0],
      sameIndex,
      sizeof(sameSizeJobs[0]),
      (int(*)(const void *,const void *))__MJobPreemptPrioComp);
    }

  /* Sort different sized jobs by priority and then by size. */

  if (diffIndex > 1)
    {
    qsort(
      (void *)&diffSizeJobs[0],
      diffIndex,
      sizeof(diffSizeJobs[0]),
      (int(*)(const void *,const void *))__MJobPreemptPrioComp);

    qsort(
      (void *)&diffSizeJobs[0],
      diffIndex,
      sizeof(diffSizeJobs[0]),
      (int(*)(const void *,const void *))__MJobPreemptTCComp);
    }

  /* First, put all same sized jobs in front of list.
   * Second, look for preemptee jobs, starting with smallest size. */

  for (sameIndex = 0;sameSizeJobs[sameIndex].J != NULL;sameIndex++)
    Preemptees[sortedIndex++] = sameSizeJobs[sameIndex].J;

  for (diffIndex = 0;diffSizeJobs[diffIndex].J != NULL;diffIndex++)
    Preemptees[sortedIndex++] = diffSizeJobs[diffIndex].J;

  MUFree((char **)&sameSizeJobs);
  MUFree((char **)&diffSizeJobs);

  Preemptees[sortedIndex] = NULL;
  } /* END __MJobSortBGPJobList() */



/**
 * Build list of idle and preemtable nodes that a bluegene job can run on. 
 *
 * Create list of preemptable nodes and jobs by first seeing if the preemptee is the same size
 * as the preemptor and then seeing if the preemptor can run on the preemptee's nodes. If that 
 * doesn't work start by taking the idle nodes and adding the preemptees', sorted 
 * smallest to largest, nodelists one at a time and seeing if the job can run on the list of nodes. 
 *
 * Jobs are first sorted by priority and then by size.
 *
 * @param PreemptorJ (I)
 * @param INL        (I) Idle nodes.
 * @param ONL        (O) Output nodes.
 * @param Preemptees (I/O) [modified] List of jobs preemptee jobs.
 */

int MJobSelectBGPList(

  mjob_t     *PreemptorJ,
  mnl_t      *INL,
  mnl_t      *ONL,
  mjob_t    **Preemptees)

  {
  int     jindex;
  mjob_t *preempteeJ;

  __mpreempt_prio_t pJ[MDEF_MAXJOB + 1];  /* only consider up to MDEF_MAXJOB jobs for preemption */

  const char *FName = "MJobSelectBGPJobList";

  MDB(7,fSCHED) MLog("%s(%s,INL,ONL,ONLSize,Preemptees)\n",
    FName,
    (PreemptorJ != NULL) ? PreemptorJ->Name : "NULL");

  if ((PreemptorJ == NULL) || (INL == NULL) || (ONL == NULL) || (Preemptees == NULL))
    return(FAILURE);

  /* sort low priority to high - preempt lower priority first */

  for (jindex = 0;Preemptees[jindex] != NULL;jindex++)
    {
    if (jindex >= MDEF_MAXJOB)
      break;

    preempteeJ = Preemptees[jindex];

    pJ[jindex].J         = preempteeJ;
    pJ[jindex].Res.Procs = preempteeJ->Request.TC;
    pJ[jindex].Cost      = __MJobGetPreempteeCost(preempteeJ,preempteeJ->Request.TC);
    }

  pJ[jindex].J = NULL;
    
  if (jindex > 1)
    {
    qsort(
      (void *)&pJ[0],
      jindex,
      sizeof(pJ[0]),
      (int(*)(const void *,const void *))__MJobPreemptPrioComp);
    }

  /* First look for exact size preemptee jobs. */

  if (__MJobSelectBGPFindExactSize(PreemptorJ,ONL,pJ) == SUCCESS)
    {
    Preemptees[0] = pJ[0].J; /* Force MJobSelectPreemptees to only look at this job. */
    Preemptees[1] = NULL;

    return(SUCCESS);
    }

  /* Second, look for preemptee jobs, starting with smallest size, that fullfills preemptor's needs. */
  /* Even though preemptees may be sorted by priority as well, it doesn't guarantee that all lower preemptees
   * will be preempted because slurm has a hand which ones will be chosen. */

  if (jindex > 1)
    {
    /* sort jobs by size (requested tasks) */

    qsort(
      (void *)&pJ[0],
      jindex,
      sizeof(pJ[0]),
      (int(*)(const void *,const void *))__MJobPreemptTCComp);
    }

  if (__MJobSelectBGPFindSmallSize(PreemptorJ,INL,ONL,pJ) == SUCCESS)
    return(SUCCESS);

  return(FAILURE);
  } /* END MJobSelectBGPList() */




/**
 * Find a preemptee of the same size.
 *
 * Only looks at jobs with same size and (NYI) same connection type.
 * Preemptees is passed in sorted according to exact same size jobs from low priority to high.
 *
 * Preempt low priority jobs first.
 *
 * @param PreemptorJ (I) Preemptor
 * @param ONL (O)
 * @param Preemptees (I/O) [modified] Preemptees.
 */

int __MJobSelectBGPFindExactSize(

  mjob_t            *PreemptorJ, /* I */
  mnl_t             *ONL,        /* O */
  __mpreempt_prio_t *Preemptees) /* I/O (modified) */

  {
  int        jindex;
  mjob_t    *preempteeJ;

  for (jindex = 0;Preemptees[jindex].J != NULL;jindex++)
    {
    preempteeJ = Preemptees[jindex].J;

    /* only look at jobs that are the same size. */

    if (PreemptorJ->Request.TC != preempteeJ->Request.TC)
      continue;

    /* NYI: continue if Preemptor and Preemptee connection type are different. ex. torus vs. mesh.
     * slurm isn't reporting this information back to moab yet. */

    /* don't need to ask slurm if the job can run because the block and wiring is already 
     * setup for preemptor's size. */

    /* copy preemptee's nodelist to tmpNL to check if job can run on it */

    MNLCopy(ONL,&preempteeJ->NodeList);

    MDB(7,fSCHED) MLog("INFO:     preemptor able to run on job %s's exact nodelist.\n",preempteeJ->Name);

    Preemptees[0].J = preempteeJ; /* force MJobSelectPList to look at this job for resources. */
    Preemptees[1].J = NULL;

    return(SUCCESS);
    }
      
  MDB(7,fSCHED) MLog("INFO:     no preemptee with the exact same size and connection type.\n");

  return(FAILURE);
  } /* END __MJobSelectBGPFindExactSize() */




/**
 * Find a list of preemptee's starting with small jobs first.
 *
 * @param PreemptorJ (I)
 * @param INL (I) Idle nodes.
 * @param ONL (O)
 * @param Preemptees (I/O) Passed in sorted according to requested task count from low to high.
 */

int __MJobSelectBGPFindSmallSize(

  mjob_t            *PreemptorJ, /* I */
  mnl_t             *INL,        /* I */
  mnl_t             *ONL,        /* O */
  __mpreempt_prio_t *Preemptees) /* I/O (modified) */

  {
  int jindex;
  int tmpTC = 0;
  mulong startTime;
  mulong latestEndTime;
  mnl_t tmpNL = {0};

  /* copy INL to ONL - start with Idle node list */

  MNLCopy(ONL,INL);

  for (jindex = 0;Preemptees[jindex].J != NULL;jindex++)
    {
    /* add to ONL */

    MDB(7,fSCHED) MLog("INFO:     adding preemptee %s's nodes to list.\n",Preemptees[jindex].J->Name);

    MNLMerge(ONL,ONL,&Preemptees[jindex].J->NodeList,&tmpTC,NULL);

    /* get enough tasks in ONL before asking willrun */

    if (tmpTC < PreemptorJ->Request.TC)
      continue;

    /* Check with slurm if the job can run on the list of nodes. 
     * The available start time returned from slurm must be less than the 
     * latest preemptee's endtime, which indicates that the wiring is available
     * to create the block for the preemptor. If not, it means that the wiring 
     * is being used by another block and won't be available until that block is done.*/

    MNLInit(&tmpNL);

    MNLCopy(&tmpNL,ONL);

    if (MJobWillRun(PreemptorJ,&tmpNL,NULL,MSched.Time + MMAX_TIME,&startTime,NULL) == SUCCESS)
      {
      latestEndTime = __MJobGetLatestPreempteeEndTime(&tmpNL);

      if (PreemptorJ->WillrunStartTime > latestEndTime)
        {
        MDB(7,fSCHED) MLog("INFO:     preemptor's start time is greater than the latest preemptee's end time. Won't preempt now\n");

        return(FAILURE);
        }

      MDB(7,fSCHED) MLog("INFO:     nodes and wiring available for preemptor to run (starttime=%ld endtime=%ld).\n",
          startTime,
          latestEndTime);

      MNLCopy(ONL,&tmpNL); /* tmpNL modified with nodes chosen by slurm */

      MNLFree(&tmpNL);

      return(SUCCESS);
      }
    else
      {
      MDB(7,fSCHED) MLog("INFO:     preemptor not able to run on previous willrun nodes.\n");
      }

    MNLFree(&tmpNL);
    }
  
  return(FAILURE);
  } /* END __MJobSelectBGPFindSmallSize() */



/**
 * Get the latest end time of the preemptee jobs.
 *
 * @param NL (I)
 */

mulong __MJobGetLatestPreempteeEndTime(

  mnl_t     *NL)
  
  {
  int nindex;
  int jindex;
  mulong endTime = 0;
  mjob_t *J;
  mjob_t *endJ = NULL;
  mnode_t *N;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    for (jindex = 0;jindex < MMAX_JOB_PER_NODE;jindex++)
      {
      J = (mjob_t *)N->JList[jindex];

      if (J == NULL)
        break;

      if (J == (mjob_t *)1)
        continue;

      if ((MSched.Time + J->RemainingTime) > endTime)
        {
        endTime = MSched.Time + J->RemainingTime;

        endJ = J;
        }
      } /* END for (jindex = 0; */
    } /* END for (nindex = 0; */
      
  MDB(7,fSCHED) MLog("INFO:     latest ending preemptee %s ends at %ld\n",endJ->Name,endTime);

  return(endTime);
  } /* END __MJobGetLatestPreempteeEndTime() */




/**
 * Sort jobs by Request.TC from small to large.
 *
 * @param A (I)
 * @param B (I)
 */

int __MJobPreemptTCComp(

  __mpreempt_prio_t *A,  /* I */
  __mpreempt_prio_t *B)  /* I */

  {
  /* order small to large */

  return(A->Res.Procs - B->Res.Procs);
  }  /* END __MJobEndCompLToH() */


 
/**
 * Routine for qsort based sorting in selecting optimal mix of preemptees.
 *
 * @see MJobSelectPreemptees()
 *
 * @param A (I)
 * @param B (I)
 */

int __MJobPreemptPrioComp(
 
  __mpreempt_prio_t *A,  /* I */
  __mpreempt_prio_t *B)  /* I */
 
  {
  static int tmp;

  /* order low to high */   
 
  tmp = (int)((A->Cost - B->Cost));
 
  return(tmp);
  }  /* END __MJobPreemptPrioComp() */

/* END MPreempt.c */




#define MMAX_PPLIST    32 /* partial preempt list (number of nodes to consider) */
  

/**
 * Select list of jobs which are preemptible by job 'PreemptorJ'.
 *
 * NOTE: Only return up to MDEF_MAXJOB possible solutions 
 * 
 * NOTE: Allow preemption if:
 *
 *  A) preemptee duration meets or exceeds preemptor duration
 *  B) preemptee duration is less than preemptor duration and subsequent node 
 *     reservations are also preemptible.  However, in this case, do not take 
 *     any immediately action on subsequent reservation.
 *
 * NOTE:  process list of potential feasible jobs, prioritizing list and then
 *        aggregating preemptees into buckets to satisfy TPN requirements of
 *        the preemptor.
 *
 * DESIGN:  PREEMPTION
 *
 *   MJobSelectMNL() 
 *     - selects available idle resources
 *     - determines if preemptible resources are required
 *    MNodeGetPreemptList() 
 *      - determines which jobs are preemptible and what resources can be provided by each
 *    MJobSelectPreemptees()
 *      - determines optimal combination of preemptees required to allow preemptor to start
 *    MJobPreempt() 
 *      - send preempt request to RM
 *
 * NOTE:  To be valid preemption target, resource must be in FNL[] AND owned by 
 *        job in FeasibleJobList[].
 *
 * NOTE:  Report all tasks which become available if job is preempted even if
 *        tasks exceed requirements of preemptor - use XXX to select tasks from
 *        those available.
 *                           
 *
 * This routine is divided into 3 main sections:
 * Section 1: the first section builds up a list of how many resources are gained by 
 *            doing preemption of all the jobs in FeasibleJobList.
 *
 * Section 2: The next section builds up the list of how many preemptee tasks are on each node.
 * 
 * Section 3: The final section then determines which jobs need to be preempted in order to get
 *            a task.
 *                           
 * @see MJobSelectMNL() - parent
 * @see MJobGetRunPriority() - child
 *
 * @param PreemptorJ        (I)
 * @param RQ                (I)
 * @param ReqTC             (I)
 * @param ReqNC             (I)
 * @param FeasibleJobList   (I) [terminated w/NULL]
 * @param FNL               (I)
 * @param INL               (I)
 * @param PreempteeJList    (O) [minsize=MDEF_MAXJOB]
 * @param PreempteeTCList   (O) [minsize=MDEF_MAXJOB]
 * @param PreempteeNCList   (O) [minsize=MDEF_MAXJOB]
 * @param PreempteeTaskList (O) [minsize=MDEF_MAXJOB,returns array of alloc nodelists]
 */

int MPreemptorSelectPreemptees(

  mjob_t      *PreemptorJ,
  mreq_t      *RQ,
  int          ReqTC,
  int          ReqNC,
  mjob_t     **FeasibleJobList,
  const mnl_t *FNL,
  mnl_t      **INL,
  mjob_t     **PreempteeJList,
  int         *PreempteeTCList,
  int         *PreempteeNCList,
  mnl_t      **PreempteeTaskList)

  {
  mjob_t     *J;
  mnode_t    *N;
  mrsv_t     *R;

  mjob_t     *PreempteeJ;

  int         TC;
  int         JC;

  static __mpreempt_prio_t pJ[MDEF_MAXJOB + 1];  /* only consider up to MDEF_MAXJOB jobs for preemption */

  mnl_t       tmpTaskList;
  mcres_t     PreemptRes;

  mcres_t     PerNodeRes;

  int index;
  int jindex;
  int jindex2;
  int InsertIndex;
  int nindex;
  int tindex;

  int ppnindex;
  int ppjindex;

  int ReqTPN;                                         /* required tasks per node */
  int TotalTC;
  int TotalNC;

  int rqindex;

  double TotalCost;

  mnl_t *NL;

  int NC;
  int PreemptPC;                                      /* number of procs required for preemption */

  mbool_t IsPreemptor;

  int    PJCount;                                     /* preemptible job count */

  double CostPerTask;
  double MaxCostPerTask;

  int     PreemptibleNodeIndex[MMAX_PPLIST + 1];               /* index of node on which partial preempt resources located */
  mjob_t *PreemptibleJobNodeIndex[MMAX_PPLIST + 1][MMAX_PJOB + 1] = {{NULL}}; /* list of jobs which provide partial preempt resources */
  int     PreemptibleJobNodeTaskCount[MMAX_PPLIST + 1][MMAX_PJOB + 1];  /* partial preempt taskcount available from corresponding job on specified node */
  int     PreemptibleReqNodeIndex[MMAX_PPLIST + 1][MMAX_PJOB + 1]; /* which req from the job will contribute resources (assumes multi-req jobs are node exclusive */

  int JobNodeTC; /* how many tasks of preemptor are on a particular node */
  int NodeTC;

  /* This routine will differ from MPreemptorSelectPreempteesOld() in that it will assume the task structures of the
     preemptor and preemptee are different.  It will build up a list of the resources gained by each
     preemptor on each node */

  const char *FName = "MPreemptorSelectPreemptees";

  MDB(4,fSCHED) MLog("%s(%s,%d,%d,%s,%s,%s,%s,%s,PTL)\n",
    FName,
    (PreemptorJ != NULL) ? PreemptorJ->Name : "NULL",
    ReqTC,
    ReqNC,
    (FeasibleJobList != NULL) ? "FJobList" : "NULL",
    (FNL != NULL) ? "FNL" : "NULL",
    (PreempteeJList != NULL)  ? "PJList" : "NULL",
    (PreempteeTCList != NULL) ? "PTCList" : "NULL",
    (PreempteeNCList != NULL) ? "PNCList" : "NULL");

  if ((PreemptorJ == NULL) ||
      (FeasibleJobList == NULL) ||
      (FeasibleJobList[0] == NULL) ||
      (FNL == NULL) ||
      (PreempteeJList == NULL) ||
      (PreempteeTCList == NULL) ||
      (PreempteeNCList == NULL))
    {
    if ((FeasibleJobList != NULL) && (FeasibleJobList[0] == NULL))
      {
      MDB(4,fSCHED) MLog("INFO:     empty feasible job list for job %s\n",
        PreemptorJ->Name);
      }
    else
      {
      MDB(1,fSCHED) MLog("ALERT:    invalid parameters passed in %s\n",
        FName);
      }

    return(FAILURE);
    }

  if ((ReqTC == 0) && (ReqNC == 0))
    {
    return(SUCCESS);
    }

  /* NOTE:  select 'best' list of jobs to preempt so as to provide */
  /*        needed tasks/nodes for preemptorJ on feasiblenodelist  */
  /*        lower 'run' priority means better preempt candidate    */

  /* determine number of available tasks associated with each job */

  index = 0;

  MCResInit(&PreemptRes);

  /* NOTE:  currently only support single req preemption */

  MNLInit(&tmpTaskList);

  for (jindex = 0;FeasibleJobList[jindex] != NULL;jindex++)
    {
    if (jindex >= MSched.SingleJobMaxPreemptee)
      {
      MDB(1,fSCHED) MLog("INFO:     single job max preemptee limit (%d) reached\n",
        MSched.SingleJobMaxPreemptee);

      break;
      }

    PreempteeJ = FeasibleJobList[jindex];

    MCResClear(&PreemptRes);

    NC = 0;

    tindex = 0;

    for (rqindex = 0;PreempteeJ->Req[rqindex] != NULL;rqindex++)
      {
      if (MCResHasOverlap(&RQ->DRes,&PreempteeJ->Req[rqindex]->DRes) == TRUE)
        {
        break;
        }
      }

    if (PreempteeJ->Req[rqindex] == NULL)
      {
      /* preemptee cannot contribute any resources to preemptor */

      continue;
      }

    for (nindex = 0;MNLGetNodeAtIndex(FNL,nindex,&N) == SUCCESS;nindex++)
      {
      /* find out if the job/node combination works */

      if (bmisset(&PreemptorJ->Flags,mjfPreemptor) != TRUE)
        {
        mre_t *RE;

        /* job is not global preemptor but may have resource-specific preemption 
           rights */

        IsPreemptor = FALSE;

        /* determine if 'ownerpreempt' is active */

        for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
          {
          R = MREGetRsv(RE);

          if ((bmisset(&R->Flags,mrfOwnerPreempt) == FALSE) || 
              (bmisset(&R->Flags,mrfIsActive) == FALSE))
            continue;

          if (MCredIsMatch(&PreemptorJ->Credential,R->O,R->OType) == FALSE)
            continue;

          IsPreemptor = TRUE;

          break;
          }  /* END for (rindex) */

        if (IsPreemptor == FALSE)
          {
          continue;
          }
        }    /* END if (bmisset(&PreemptorJ->Flags,mjfPreemptor) != TRUE) */

      for (jindex2 = 0;jindex2 < MMAX_JOB_PER_NODE;jindex2++)
        {
        J = (mjob_t *)N->JList[jindex2];

        if (J == NULL)
          break;

        if (J == (mjob_t *)1)
          continue;

        if (J != PreempteeJ)
          continue;

        TC = N->JTC[jindex2];

        if (J->RemainingTime < PreemptorJ->SpecWCLimit[0])
          {
          /* verify node does not possess non-preemptible rsvs which would block
             preemptor from executing */

          /* NYI */
          }

        /* job is preemptible if 
             'preemptee' flag is set 
             or job w/in 'ownerpreempt' reservation */

        /* NOTE:  feasible job list should only contain preemptible jobs 
         * which meet priority requirements */

        if ((N->JList != NULL) &&
            (N->JList[0] != NULL) &&
            (N->JList[1] == NULL) &&
           ((N->EffNAPolicy == mnacSingleJob) ||
            (N->EffNAPolicy == mnacSingleTask)))
          {
          /* if single job running on node and node access policy blocks
             non-dedicated resource access, assume all node resources become
             available when job is preempted */

          /* NOTE:  if node has user/system reservation in place over subset of
                    resources, issues may arise (not yet handled) */

          MCResAdd(&PreemptRes,&N->CRes,&N->CRes,TC,FALSE);
          }
        else
          {
          rqindex = -1;

          MUJobNLGetReqIndex(J,N,&rqindex);

          /* job dedicated resources are available */

          MCResAdd(&PreemptRes,&N->CRes,&PreempteeJ->Req[rqindex]->DRes,TC,FALSE);
          }

        NC += 1;

        MNLSetNodeAtIndex(&tmpTaskList,tindex,N);
        MNLSetTCAtIndex(&tmpTaskList,tindex,TC);

        tindex++;
        }  /* END for (jindex2) */
      }    /* END for (nindex) */

    MDB(6,fSCHED) MLog("INFO:     preemptible job %s provides %d/%d tasks/nodes\n",
      PreempteeJ->Name,
      PreemptRes.Procs,
      NC); 

    MNLTerminateAtIndex(&tmpTaskList,tindex);

    /* determine 'cost per task' associated with job */

    pJ[index].J     = PreempteeJ;
    pJ[index].Nodes = NC;

    MCResInit(&pJ[index].Res);

    MCResCopy(&pJ[index].Res,&PreemptRes);

    PreemptPC = MIN(ReqTC,PreemptRes.Procs);

    pJ[index].Cost = __MJobGetPreempteeCost(pJ[index].J,PreemptPC);

    /* + RTimeWeight favors jobs that will end first - preempt those with shorter remaining times */
    /* - RTimeWeight favors jobs that will end last  - preempt those with longer remaining times */
    /* Larger remaining time = larger cost */

    if (MSched.PreemptRTimeWeight != 0.0)
      pJ[index].Cost += (MSched.PreemptRTimeWeight * (pJ[index].J->RemainingTime / MCONST_MINUTELEN));

    if (PreempteeTaskList != NULL)
      {
      MNLInit(&pJ[index].TL);

      MNLCopy(&pJ[index].TL,&tmpTaskList);
      }

    index++; 

    if (index >= MDEF_MAXJOB)
      break;
    }    /* END for (jindex) */

  MNLFree(&tmpTaskList);

  MCResFree(&PreemptRes);

  /* terminate list */

  pJ[index].J  = NULL;

  JC = index;  /* JC is total preemptible jobs available to preemptor */

  /* sort job list by cost - low to high */

  if (index > 1)
    {
    qsort(
      (void *)&pJ[0],
      index,
      sizeof(pJ[0]),
      (int(*)(const void *,const void *))__MJobPreemptPrioComp);
    }

  /* create initial preemption 'schedule' */

  TotalNC = 0;
  TotalTC = 0;
  TotalCost = 0;

  MaxCostPerTask = 0.0;

  ReqTPN = RQ->TasksPerNode;

  PreemptibleNodeIndex[0] = -1;

  /* search job list, lowest cost job first */

  for (jindex = 0;jindex < index;jindex++)
    {
    /* search list of nodes in job */

    NL = &pJ[jindex].TL;

    for (nindex = 0;nindex < pJ[jindex].Nodes;nindex++)
      {
      MNLGetNodeAtIndex(NL,nindex,&N);

      JobNodeTC = MNLGetTCAtIndex(NL,nindex);

      /* idle tasks are not considered until further below (Search for "Consider Idle" and "Add Idle") */
      /* considering idle tasks in this location could lead to double-counting as we are looking at nodes
         on a per job basis */
    
      /* assign job to partial preempt list */

      rqindex = -1;

      MUJobNLGetReqIndex(pJ[jindex].J,N,&rqindex);

      for (ppnindex = 0;ppnindex < MMAX_PPLIST;ppnindex++)
        {
        if (N->Index == PreemptibleNodeIndex[ppnindex])
          {
          /* match located */

          for (ppjindex = 0;ppjindex < MMAX_PJOB;ppjindex++)
            {
            if (PreemptibleJobNodeIndex[ppnindex][ppjindex] == NULL)
              {
              PreemptibleJobNodeIndex[ppnindex][ppjindex] = pJ[jindex].J;
              PreemptibleJobNodeTaskCount[ppnindex][ppjindex]  = JobNodeTC;

              PreemptibleReqNodeIndex[ppnindex][ppjindex] = rqindex;

              PreemptibleJobNodeIndex[ppnindex][ppjindex + 1] = NULL;
              
              break;
              }
            }

          break;
          }  /* END if (N->Index == PreemptibleNodeIndex[ppnindex]) */

        if (PreemptibleNodeIndex[ppnindex] == -1)
          {
          /* end of list reached */

          PreemptibleNodeIndex[ppnindex] = N->Index;
          PreemptibleNodeIndex[ppnindex + 1] = -1;

          PreemptibleJobNodeIndex[ppnindex][0] = pJ[jindex].J;
          PreemptibleJobNodeIndex[ppnindex][1] = NULL;

          PreemptibleReqNodeIndex[ppnindex][0] = rqindex;

          PreemptibleJobNodeTaskCount[ppnindex][0]  = JobNodeTC;

          break;
          }
        }    /* END for (ppnindex) */
      }        /* END for (nindex) */

    TotalCost += pJ[jindex].Cost;

    CostPerTask = pJ[jindex].Cost / pJ[jindex].Res.Procs;

    if (CostPerTask > MaxCostPerTask)
      {
      MaxCostPerTask = CostPerTask;
      }
    }  /* END for (jindex) */

  TotalTC = 0;
  TotalNC = 0;

  /* on per node basis, sum resources available from each preemptee job */

  PJCount = 0;

  MCResInit(&PerNodeRes);

  for (nindex = 0;nindex < MMAX_PPLIST;nindex++)
    {
    if (PreemptibleNodeIndex[nindex] == -1) 
      {
      /* end of list */

      break;
      }

    if ((TotalTC >= ReqTC) && ((ReqNC == 0) || (TotalNC >= ReqNC)))
      {
      break;
      }

    if (PreemptibleJobNodeIndex[nindex][0] == NULL)
      {
      /* empty list, ignore - if there is just one job, keep it because we
         still need to add idle tasks */

      continue;
      }

    N = MNode[PreemptibleNodeIndex[nindex]];

    /* Compiler warning - N may not be initialized */

    MCResClear(&PerNodeRes);

    /* "Consider Idle" tasks here */

    MCResAddNodeIdleResources(N,&PerNodeRes);

    /* append possible joint solutions */

    for (jindex = 0;PreemptibleJobNodeIndex[nindex][jindex] != NULL;jindex++)
      {
      /* FIXME: as we add the resources from each preemptee we need to see
         how many preemptor tasks we get with the addition of each preemptor */

      JobNodeTC = PreemptibleJobNodeTaskCount[nindex][jindex];

      MCResAddDefTimesTC(
        &PerNodeRes,
        &PreemptibleJobNodeIndex[nindex][jindex]->Req[PreemptibleReqNodeIndex[nindex][jindex]]->DRes,
        JobNodeTC);
      }  /* END for (jindex) */

    /* this routine does the conversion.  Up till now we've added in the resources
       in the generic mcres_t format.  This will calculate how many preemptor tasks
       we get from all the summed resources */

    MCResGetTC(&PerNodeRes,1,&RQ->DRes,&NodeTC);

    if (NodeTC < ReqTPN)
      {
      /* all partials together cannot add a full task */

      continue;
      }

    /*  partial + idle is sufficient for preemption */

    TotalTC += NodeTC;
    TotalNC++;

    if (PJCount == 0)
      {
      /* PreempteeTCList and PreempteeNCList are not initialized anywhere
         so we need to initialize each entry as we add them.  Initialize
         the first entry here, later entries are initialized as we add 
         jobs to the list */

      PreempteeTCList[PJCount] = 0;
      PreempteeNCList[PJCount] = 0;
      }

    for (jindex = 0;PreemptibleJobNodeIndex[nindex][jindex] != NULL;jindex++)
      {
      /* this is tricky, we need to keep track of the number of tasks
         each preemptee provides to the preemptor, but a single
         job may provide less than 1 task */

      /* HACK: for now, dump all the tasks this node can provide with
         all of its preemptees on the last job on the node */
 
      for (jindex2 = 0;jindex2 < PJCount;jindex2++)
        {
        /* this for loop looks to see if this preemptee was already added from
           another node or if this is the first time we're adding this job as a
           preemptee */

        if (PreempteeJList[jindex2] == NULL)
          break;

        if (PreempteeJList[jindex2] == PreemptibleJobNodeIndex[nindex][jindex])
          break;
        }

      InsertIndex = jindex2;

      PreempteeJList[InsertIndex] = PreemptibleJobNodeIndex[nindex][jindex];
     
      for (jindex2 = 0;jindex2 < JC;jindex2++)
        {
        /* find the right tasklist to copy */

        if (PreempteeJList[InsertIndex] == pJ[jindex2].J)
          {
          PreempteeTaskList[InsertIndex] = &pJ[jindex2].TL;

          pJ[jindex2].JobWillBeUsed = TRUE;

          break;
          }
        }

      if (PreemptibleJobNodeIndex[nindex][jindex + 1] == NULL)
        {
        PreempteeTCList[InsertIndex]+=NodeTC;
        PreempteeNCList[InsertIndex]++;
        }
      else if (InsertIndex == PJCount)
        {
        /* we're adding at the end of the list and there are more
           jobs on this node so we're going to initialize the lists
           here */

        PreempteeTCList[PJCount] = 0;
        PreempteeNCList[PJCount] = 0;
        }

      if (InsertIndex == PJCount)
        {
        /* just added one to the end */
        PJCount++;

        PreempteeJList[PJCount]    = NULL;
        PreempteeTaskList[PJCount] = NULL;
        PreempteeTCList[PJCount]   = 0;
        PreempteeNCList[PJCount]   = 0;
        }
      }
    }        /* END for (nindex) */

  MCResFree(&PerNodeRes);

  if (PreempteeTaskList != NULL)
    {
    /* free all tasklists which are not used */

    for (jindex = 0;pJ[jindex].J != NULL;jindex++)
      {
      MCResFree(&pJ[jindex].Res);

      if (pJ[jindex].JobWillBeUsed == FALSE)
        MNLFree(&pJ[jindex].TL);
      }
    }

  if ((PJCount == 0) ||
      (ReqTC > TotalTC) ||
      (ReqNC > TotalNC))
    {
    /* inadequate preemptible resources located */

    MDB(2,fSCHED) MLog("INFO:     inadequate preempt jobs (%d) located (P: %d of %d,N: %d of %d)\n",
      jindex,
      TotalTC,
      ReqTC,
      TotalNC,
      ReqNC);

    /* free alloc memory */

    if (PreempteeTaskList != NULL)
      {
      for (jindex = 0;jindex < JC;jindex++)
        {
        MCResFree(&pJ[jindex].Res);
        MNLFree(&pJ[jindex].TL);
        }  /* END for (jindex) */

      PreempteeTaskList[0] = NULL;
      }

    return(FAILURE);
    }  /* END if ((PJCount == 0) || ...) */

  return(SUCCESS);
  }  /* END MPreemptorSelectPreemptees() */




/**
 * Wrapper to determine which preemption routine to call.
 *
 * NOTE: any req that requests anything besides just procs will
 *       use the new routine.  See moabdocs.h
 *
 * @param PreemptorJ        (I)
 * @param RQ                (I)
 * @param ReqTC             (I)
 * @param ReqNC             (I)
 * @param FeasibleJobList   (I) (terminated w/NULL)
 * @param FNL               (I)
 * @param INL               (I)
 * @param PreempteeJList    (O) list of preemptible jobs (minsize=MDEF_MAXJOB)
 * @param PreempteeTCList   (O) proc count (minsize=MDEF_MAXJOB)
 * @param PreempteeNCList   (O) (minsize=MDEF_MAXJOB)
 * @param PreempteeTaskList (O) (minsize=MDEF_MAXJOB,returns array of alloc nodelists)
 */

int MJobSelectPreemptees(

  mjob_t        *PreemptorJ,       
  mreq_t        *RQ,               
  int            ReqTC,            
  int            ReqNC,            
  mjob_t       **FeasibleJobList,  
  const mnl_t   *FNL,              
  mnl_t        **INL,              
  mjob_t       **PreempteeJList,   
  int           *PreempteeTCList,  
  int           *PreempteeNCList,  
  mnl_t        **PreempteeTaskList)

  {
  if ((ReqTC == 0) && (ReqNC == 0))
    {
    return(SUCCESS);
    }

  if (!bmisset(&MSched.Flags,mschedfForceProcBasedPreemption) &&
      ((RQ->DRes.Mem > 0) ||
       (RQ->DRes.Disk > 0) ||
       (RQ->DRes.Swap > 0) ||
       (!MSNLIsEmpty(&RQ->DRes.GenericRes))))
    {
    return(MPreemptorSelectPreemptees(
             PreemptorJ,
             RQ,
             ReqTC,
             ReqNC,
             FeasibleJobList,
             FNL,
             INL,
             PreempteeJList,
             PreempteeTCList,
             PreempteeNCList,
             PreempteeTaskList));
    }

  return(MPreemptorSelectPreempteesOld(
           PreemptorJ,
           RQ,
           ReqTC,
           ReqNC,
           FeasibleJobList,
           FNL,
           INL,
           PreempteeJList,
           PreempteeTCList,
           PreempteeNCList,
           PreempteeTaskList));
  }  /* END MJobSelectPreemptees() */



/**
 * This function will attempt to "Preempt" an active job (and only an active job) 
 * based on the input policy.
 * 
 * The preemption policy can be
 *   - Cancel - Cancel the job
 *   - Checkpoint - Checkpoint the job by sending a signal to the job's controlling RM (not applicable to many RMs)
 *   - Migrate - Currently unimplemented (no case statement). (6/26/07)
 *   - Overcommit - Currently does nothing.
 *   - Requeue - This policy will stop the job from executing and put it back into the idle queue 
 *         for rescheduling. 
 *       If this policy is set, but the job flag "Restartable" isn't set, the
 *         policy will be changed to "Cancel".
 *   - Suspend - Stops the job from executing, but keeps the job resident in memory. 
 *       i.e. the job won't use any CPU time, but will keep all of its resources.
 *       If this policy is set, but the job flag "Suspendable" isn't set, the policy will
 *         be changed to "Requeue"
 * 
 * NOTE:  need preempt reason (NYI)
 * NOTE:  should specify 'allowescalation' parameter (NYI) 
 * 
 * @see MNodePreemptJobs()
 * @see __MUIJobPreempt() - parent - handle admin request based preemption
 *
 * @param J (I) The job to preempt
 * @param PreemptorJ (I) [optional]
 * @param JPeer (I) [optional]
 * @param NL (I) [optional]
 * @param Message (I) reason job was preempted [optional]
 * @param PreemptPolicy (I) one of the preemption policies
 * @param IsAdminRequest (I) request is based on policy violation or admin request
 * @param Args (I) [optional] 
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MJobPreempt(

  mjob_t                   *J,
  mjob_t                   *PreemptorJ,
  mjob_t                  **JPeer,
  mnl_t                    *NL,
  const char               *Message,
  enum MPreemptPolicyEnum   PreemptPolicy,
  mbool_t                   IsAdminRequest,
  char                     *Args,
  char                     *EMsg,
  int                      *SC)

  {
  int nindex;
  int TC;
  
  mnode_t *N;
  mreq_t  *RQ;

  enum MPreemptPolicyEnum PPolicy;

  char     tmpLine[MMAX_LINE];
  char     PreemptOverride[MMAX_LINE];

  mbool_t  AllowEscalation;

  mulong JobStartTime;
  mulong JobRequeueTime;
  mulong SaveStartTime;

  const char *FName = "MJobPreempt";

  MDB(4,fSCHED) MLog("%s(%s,%s,JPeer,NL,%s,%s,%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (PreemptorJ != NULL) ? PreemptorJ->Name : "NULL",
    (Message != NULL) ? Message : "NULL",
    MBool[IsAdminRequest],
    MPreemptPolicy[PreemptPolicy],
    (Args != NULL) ? Args : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  tmpLine[0] = '\0';
  PreemptOverride[0] = '\0';

  AllowEscalation = TRUE;

  if (PreemptPolicy != mppNONE)
    PPolicy = PreemptPolicy;
  else if ((J->Credential.Q != NULL) && (J->Credential.Q->PreemptPolicy != mppNONE))
    PPolicy = J->Credential.Q->PreemptPolicy;
  else
    PPolicy = MSched.PreemptPolicy;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* verify job state */

  if (MJOBISACTIVE(J) == FALSE)
    {
    MDB(4,fSCHED) MLog("WARNING:  cannot preempt non-active job '%s' (state: '%s' estate: '%s')\n",
      J->Name,
      MJobState[J->State],
      MJobState[J->EState]);

    if (EMsg != NULL)
      sprintf(EMsg,"cannot preempt non-active job");

    return(FAILURE);
    }

  /* escalate preemption policy if required */

  /* NOTE:  should check R->FnList for support of PreemptAction (NYI) */

  /*
  if ((PPolicy == mppSuspend) && 
     ((bmisset(&J->Flags,mjfSuspendable) == FALSE) || (!bmisset(&R->FnList,mrmJobRequeue))))
  */

  if ((PPolicy == mppSuspend) &&
     (bmisset(&J->Flags,mjfSuspendable) == FALSE))
    {
    if (AllowEscalation == TRUE)
      {
      PPolicy = mppRequeue;

      sprintf(PreemptOverride,"job %s not suspendable, preempt policy temporarily changed to requeue\n",
        J->Name);
      }
    else
      {
      if (EMsg != NULL)
        sprintf(EMsg,"job cannot be suspended");

      return(FAILURE);
      }
    }

  if ((PPolicy == mppRequeue) && !bmisset(&J->Flags,mjfRestartable))
    {
    if (AllowEscalation == TRUE)
      {
      PPolicy = mppCancel;

      MDB(2,fSCHED) MLog("WARNING:  cannot requeue non-restartable job '%s' (canceling instead)\n",
        J->Name);

      if (PreemptOverride[0] != '\0')
        {
        sprintf(PreemptOverride,"job %s not suspendable or requeueable, preempt policy temporarily changed to cancel\n",
          J->Name);
        }
      else
        {
        sprintf(PreemptOverride,"job %s not requeueable, preempt policy temporarily changed to cancel\n",
          J->Name);
        }
      }
    else
      {
      if (EMsg != NULL)
        sprintf(EMsg,"job cannot be requeued");

      return(FAILURE);
      }
    }    /* END if ((PPolicy == mppRequeue) && ...) */

  /* SubState preempted is used to prevent immediate use of preempted resources */

  J->SubState = mjsstPreempted;

  if ((J->DestinationRM != NULL) &&
      (bmisset(&J->DestinationRM->Flags,mrmfPreemptionTesting)))
    {
    return(SUCCESS);
    }

  /* save job run time information to create the charge after requeueing */

  JobStartTime = J->StartTime;
  JobRequeueTime = MSched.Time;

  switch (PPolicy)
    {
    case mppRequeue:
      
      if (MJobRequeue(J,JPeer,Message,EMsg,SC) == SUCCESS)
        {
        MJobRemoveFromNL(J,NULL);
  
        /* remove job from active job list */
  
        MQueueRemoveAJob(J,mstForce);
  
        /* change job state */
  
        MJobSetState(J,mjsIdle);
  
        /* release reservation */
  
        if (J->Rsv != NULL)
          {
          MJobReleaseRsv(J,TRUE,TRUE);
          }
  
        /* reset job start time */
  
        J->StartTime       = 0;
        J->SystemQueueTime = MSched.Time;
        J->AWallTime       = 0;
        J->SWallTime       = 0;
  
        /* reset job stats */
  
        J->Alloc.TC   = 0;
        J->PSUtilized = 0.0;
  
        /* adjust job mode */
  
        bmunset(&J->SysFlags,mjfBackfill);
  
        /* NOTE:  we only want to un-migrate if we're manually requeueing the
         *        job, and only if the job was previously migrated to another peer/slave. */
  
        if ((IsAdminRequest == TRUE) &&
            (strcmp(Message, "admin request") == 0) &&
            ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtMoab)))
          {
          MJobUnMigrate(J,TRUE,EMsg,SC);
          }
  
        break;
        }

      /* follow through to cancel on requeue failure */
      
    case mppCancel:
      {
      mbool_t  CancelJobWithTrigger = FALSE;
      mtrig_t *T = NULL;

      snprintf(tmpLine,sizeof(tmpLine),"job preempted (%s)",
        Message);

      /* if the preemptee job has a termination signal trigger then we should fire the
         trigger and we should not send a job cancel until after the gracetime has expired */

      if (MTrigFindByName("termsignal",J->Triggers,&T) == SUCCESS)
        {
        int  Gracetime = abs(T->Offset);
        char TrigString[MMAX_LINE];

        if ((T->State != mtsSuccessful))
          {
          MDB(7,fSCHED) MLog("INFO:    firing termination signal trigger for preemptee %s\n",
            J->Name);

          MTrigInitiateAction(T);

          MTrigRemove(T,NULL,FALSE);
          }

        /* if there is a gracetime then add a trigger (if it does not have one already)
           to the preemptee job so that it is canceled at the end of the gracetime */

        if ((Gracetime > 0) &&
            (MTrigFindByName("termpreempt",J->Triggers,&T) != SUCCESS))
          {
          MDB(7,fSCHED) MLog("INFO:    preemptee %s has a termination trigger with gracetime %d\n",
            J->Name,
            Gracetime);

          /* set the was cancelled flag so that guaranteed preemption will see that
             the preemptee job is in the process of cleaning up */

          MDB(3,fSCHED) MLog("INFO:    job '%s' will be force-canceled after signal and gracetime\n",
            J->Name);

          bmset(&J->IFlags,mjifWasCanceled);

          /* set the force cancel flag so that MJobCancel will send the cancel job
             message when this trigger fires (since we are setting WasCanceled flag but 
             not really sending the cancel message now. */

          bmset(&J->IFlags,mjifRequestForceCancel);

          /* adjust the WCLimit - not sure if we need to adjust SpecWCLimit, MinWCLimit, OrigWCLimit, etc. */

          J->WCLimit = MIN((mutime)Gracetime,J->WCLimit);

          snprintf(TrigString,sizeof(TrigString),"atype=internal,action=job:-:cancel,etype=preempt,offset=%d,name=termpreempt",
            Gracetime);

          MTrigLoadString(
            &J->Triggers,
            (char *)TrigString,
            TRUE,
            FALSE,
            mxoJob,
            J->Name,
            NULL,
            NULL);

          if (MTrigFindByName("termpreempt",J->Triggers,&T) == SUCCESS)
            MTrigInitialize(T);
          }
        }

      if (MTrigFindByName("termpreempt",J->Triggers,&T) == SUCCESS)
        {
        if (T->State == mtsSuccessful)
          MTrigRemove(T,NULL,FALSE);
        else        
          CancelJobWithTrigger = TRUE;
        }

      /* if we are canceling the preemptee job with a trigger then pass in a ForceUpdate to
         MOReportEvent of FALSE so that the event time is not updated if we call this
         routine more than once before the trigger fires. */

      MOReportEvent((void *)J,J->Name,mxoJob,mttPreempt,MSched.Time,(CancelJobWithTrigger == FALSE));

      MStat.TotalPHPreempted += J->PSDedicated;

      if ((CancelJobWithTrigger == FALSE) && 
          (MJobCancel(J,tmpLine,FALSE,EMsg,SC) == FAILURE))        
        {
        /* cannot cancel job */

        bmset(&J->IFlags,mjifPreemptFailed);

        return(FAILURE);
        }

      /* update preemption stats */

      MStat.TotalPreemptJobs++;
      MSched.JobPreempteePerIterationCounter++;

      /* If we succeeded but changed the preemption policy, then set SC and
       * fill in the description of the preemption change in EMsg */

      if ((EMsg != NULL) && (SC != NULL) && (PreemptOverride[0] != '\0'))
        {
        *SC = mscPartial;

        MUStrCpy(EMsg,PreemptOverride,MMAX_LINE);
        }

      /* 5-18-09 RT4411 BOC
       * release reservation - if preemptor fails to start because the job is taking 
       * a long time to start, then the preemptor will get a reservation that goes out 
       * past the preempted job's walltime which allows for a smaller jobs to jump ahead
       * of the preemptor job. If the reservation is released then the preemptor will 
       * get a reservation with the nodes that it tried to start with. */

      if (J->Rsv != NULL)
        {
        MJobReleaseRsv(J,TRUE,TRUE);
        }

      if (PreemptorJ != NULL)
        PreemptorJ->PreemptIteration = MSched.Iteration;

      return(SUCCESS);

      } /* END block */

      break;

    case mppCheckpoint:

      /* NOTE:  checkpoint can either ckpt and resume or ckpt and terminate */

      if (MRMJobCheckpoint(J,TRUE,Message,EMsg,SC) == FAILURE)
        {
        /* cannot checkpoint job */

        bmset(&J->IFlags,mjifPreemptFailed);
 
        return(FAILURE);
        }

      break;

    case mppOvercommit:

      /* do nothing */

      /* let RM/OS handle all preemption */

      break;

    case mppSuspend:
 
      if (MRMJobSuspend(J,Message,EMsg,SC) == FAILURE)
        {
        /* cannot suspend job */

        bmset(&J->IFlags,mjifPreemptFailed);
 
        return(FAILURE);
        }

      MJobSetState(J,mjsSuspended);

      if (IsAdminRequest == TRUE)
        {
        if (MPar[0].AdminMinSTime > 0)
          {
          /* NOTE:  to prevent thrashing, by default, suspended jobs should not *
           *        resume for MDEF_MINSUSPENDTIME */

          long tmpL;

          tmpL = MSched.Time + MPar[0].AdminMinSTime;

          MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mSet);
          }
        }  /* END if (IsAdminRequest == TRUE) */
      else
        {
        /* job preemption - non-admin request */

        if (MPar[0].JobMinSTime > 0)
          {
          long tmpL;

          tmpL = MSched.Time + MPar[0].JobMinSTime;
  
          MJobSetAttr(J,mjaSysSMinTime,(void **)&tmpL,mdfLong,mSet);
          }
        }

      if (J->Rsv != NULL)
        {
        MJobReleaseRsv(J,TRUE,TRUE);
        }
 
      break;

    default:

      /* no policy specified */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (MSched.PreemptPolicy) */

  if (bmisset(&J->IFlags,mjifRanProlog))
    {
    /* Anytime the prolog is run, need to run epilog -
       epilog may tear down setup from prolog */

    MJobLaunchEpilog(J);

    bmset(&J->IFlags,mjifRanEpilog);
    }

  RQ = J->Req[0];  /* FIXME */

  if (!MNLIsEmpty(&J->NodeList))
    { 
    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      if ((RQ->NAccessPolicy == mnacSingleJob) ||
          (RQ->NAccessPolicy == mnacSingleTask))
        {
        MCResRemove(&N->DRes,&N->CRes,&N->CRes,1,TRUE);

        MCResAdd(&N->ARes,&N->CRes,&N->CRes,1,FALSE);
        }
      else
        {
        TC = MNLGetTCAtIndex(&J->NodeList,nindex);

        /* FIXME:  if MSched.TrackSuspendedJobUsage == TRUE then we need to remove everything
                   but mem, swap, and disk */
 
        if (MSched.TrackSuspendedJobUsage)
          {
          mbitmap_t BM;

          bmset(&BM,mrProc);
          bmset(&BM,mrGRes);

          MCResRemoveSpec(&N->DRes,&N->CRes,&RQ->DRes,TC,TRUE,&BM);

          MCResAddSpec(&N->ARes,&N->CRes,&RQ->DRes,TC,FALSE,&BM);
          }
        else
          {
          MCResRemove(&N->DRes,&N->CRes,&RQ->DRes,TC,TRUE);

          MCResAdd(&N->ARes,&N->CRes,&RQ->DRes,TC,FALSE);
          }
        }

      if (N->CRes.Procs > 0)
        { 
        /* NOTE:  only adjust expected node state for compute nodes */
        /* NOTE:  above comment does not apply 
                  CHANGED by drw 11/30/06 
                  so that MJobGetINL successfully reports idle node the same iteration */

        if (MNODEISUP(N) == TRUE)
          {
          if (N->ARes.Procs == 0)
            {
            N->EState = mnsBusy;
            N->State  = mnsBusy;
            }
          else if (N->ARes.Procs == N->CRes.Procs)
            {
            N->State  = mnsIdle;
            N->EState = mnsIdle;
            } 
          else
            {
            N->State  = mnsActive;
            N->EState = mnsActive;
            }
          }
        }
      }    /* END for (nindex) */
    }      /* END if (J->NodeList != NULL) */
 
  /* debit according to charge policy */
 
  if ((J->Credential.A != NULL) &&
      ((MAM[0].ChargePolicy == mamcpDebitAllWC) ||
       (MAM[0].ChargePolicy == mamcpDebitAllBlocked) ||
       (MAM[0].ChargePolicy == mamcpDebitAllCPU) ||
       (MAM[0].ChargePolicy == mamcpDebitAllPE)))
    {
    SaveStartTime = J->StartTime;

    /* create the charge based on the previously saved run time */
    J->StartTime = JobStartTime;
    J->CompletionTime = JobRequeueTime;

    if (MAMHandlePause(&MAM[0],(void *)J,mxoJob,NULL,NULL) == FAILURE)
      {
    MDB(3,fSCHED) MLog("WARNING:  Unable to register job pause with accounting manager for job '%s'\n",
      J->Name);
      }

    /* restore the saved start time and reset the completion time */
    J->StartTime = SaveStartTime;
    J->CompletionTime = 0;
    }
 
  /* update preemption stats */
 
  MStat.TotalPreemptJobs++;
  MStat.TotalPHPreempted += J->PSDedicated;

  J->PreemptCount ++;

  MSched.JobPreempteePerIterationCounter++;
 
  J->PSDedicated = 0.0;
  J->PSUtilized  = 0.0;
  J->MSUtilized  = 0.0;
  J->MSDedicated = 0.0;

  MOReportEvent((void *)J,J->Name,mxoJob,mttPreempt,MSched.Time,TRUE);
 
  MJobUpdateFlags(J);
 
  /* If we succeeded but changed the preemption policy, then set SC and
   * fill in the description of the preemption change in EMsg */

  if ((EMsg != NULL) && (SC != NULL) && (PreemptOverride[0] != '\0'))
    {
    *SC = mscPartial;

    MUStrCpy(EMsg,PreemptOverride,MMAX_LINE);
    }

  if (PreemptorJ != NULL)
    PreemptorJ->PreemptIteration = MSched.Iteration;

  return(SUCCESS);
  }  /* END MJobPreempt() */



/**
 * Return list of jobs with idle rsv's on N which start within J->WCLimit.
 *
 * @see MJobCanPreempt() - child
 * @see MQueueScheduleSJobs() - parent - determine if suspended job can be resumed
 * @see MNodeGetPreemptList() - peer - determine which jobs can be preempted on specified node
 *
 * @param J (I)
 * @param N (I)
 * @param RsvAList (O) [minsize=MMAX_PJOB+1]
 */

int MJobGetNodePreemptList(

  mjob_t   *J,         /* I */
  mnode_t  *N,         /* I */
  char    **RsvAList)  /* O (minsize=MMAX_PJOB+1) */

  {
  int JIndex;

  mre_t *RE;

  mrsv_t *R;
  mjob_t *RJ;

  const char *FName = "MJobGetNodePreemptList";

  MDB(8,fRM) MLog("%s(%s,%s,%s)\n",
      FName,
      (J != NULL) ? J->Name : "NULL",
      (N != NULL) ? N->Name : "NULL",
      "RsvAList");

  if ((J == NULL) || (N == NULL) || (RsvAList == NULL))
    {
    return(FAILURE);
    }

  JIndex = 0;

  /* loop through all events on node */

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    if (RE->Type != mreStart)
      continue;

    R = MREGetRsv(RE);

    if (R->Type != mrtJob)
      continue;

    RJ = R->J;

    if (bmisset(&J->IFlags,mjifIsResuming) &&
        (RJ->State == mjsIdle))
      {
      /* NO-OP */
      }
    else if (R->StartTime <= MSched.Time)
      {
      /* ignore running jobs */

      continue;
      }

    if (R->StartTime > MSched.Time + J->WCLimit)
      {
      /* reservation too far in future to impact job resume */

      break;
      }

    if (RJ == NULL)
      continue;

    if (MJobCanPreempt(J,RJ,J->PStartPriority[N->PtIndex],&MPar[N->PtIndex],NULL) == FALSE)
      {
      /* reservation is not preemptible */

      continue;
      }

    RsvAList[JIndex++] = RJ->Name;

    if (JIndex >= MMAX_PJOB)
      break;
    }    /* END for (eindex) */

  RsvAList[JIndex] = NULL;

  if (JIndex == 0)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobGetNodePreemptList() */



/**
 * Returns the job effective time before the job can be preempted.
 *
 * @param J (I)
 */

mulong MJobGetMinPreemptTime(

  const mjob_t *J)

  {
  mulong tmpL;

  if (J == NULL)
    return(0);

  if (J->MinPreemptTime > 0)
    tmpL = J->MinPreemptTime;
  else if ((J->Credential.Q != NULL) && 
           (J->Credential.Q->PreemptMinTime > 0))
    tmpL = J->Credential.Q->PreemptMinTime;
  else 
    tmpL = MSched.JobPreemptMinActiveTime;

  return(tmpL);
  } /* END MJobGetMinPreemptTime() */



/**
 * Determines if preemptee has reached min preempt time.
 *
 * @see MJobCanPreempt
 *
 * @param J (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

mbool_t MJobReachedMinPreemptTime(

  const mjob_t *J,
  char   *EMsg)

  {
  long tmpL;

  MASSERT(J != NULL,"null job in checking min preemption time");

  tmpL = MJobGetMinPreemptTime(J);

  if ((tmpL > 0 ) && 
     (((long)(MSched.Time - J->StartTime)) < tmpL))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"job will not reach preempt min time for %ld seconds",
        tmpL - (MSched.Time - J->StartTime));

    return(FALSE);
    }

  return(TRUE);
  } /* END MJobReachedMinPreemptTime() */



/**
 * Returns the job effective time after which a job can't be preempted anymore.
 *
 * @param J (I)
 */

mulong MJobGetMaxPreemptTime(

  const mjob_t *J)

  {
  mulong tmpL;

  if (J == NULL)
    return(0);

  if ((J->Credential.Q != NULL) && 
      (J->Credential.Q->PreemptMaxTime > 0))
    tmpL = J->Credential.Q->PreemptMaxTime;
  else 
    tmpL = MSched.JobPreemptMaxActiveTime;

  return(tmpL);
  } /* MJobGetMaxPreemptTime() */



/**
 * Determines if preemptee has passed max preempt time.
 *
 * @see MJobCanPreempt
 *
 * @param J (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

mbool_t MJobReachedMaxPreemptTime(

  const mjob_t *J,
  char   *EMsg)

  {
  long tmpL;

  MASSERT(J != NULL,"null job in checking max preemption time");

  tmpL = MJobGetMaxPreemptTime(J);

  if ((tmpL > 0 ) && 
      (((long)(MSched.Time - J->StartTime)) > tmpL))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"job has passed max preempt time by %ld seconds",
        (MSched.Time - J->StartTime) - tmpL);

    return(TRUE);
    }

  return(FALSE);
  } /* END MJobReachedMaxPreemptTime() */

/**
 * Report if Preemptor can preempt Preemptee by QOS.
 *
 * @see MJobCanPreempt() - parent
 *
 * @param PreemptorJ (I)
 * @param PreempteeJ (I) 
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * 
 * @return TRUE if PreempteeJ is eligible to be preempted by PreemptorJ
 */

int __MJobCanPreemptQOS(

  mjob_t       *PreemptorJ,
  const mjob_t *PreempteeJ,
  char         *EMsg)

  {
  int     qindex;

  /* If preemptor has a QOS, and there is a QOS preemtees list */
  MASSERT(PreemptorJ != NULL, "Unexpected NULL PreemptorJ searching preemptees list.");
  MASSERT(PreempteeJ != NULL, "Unexpected NULL PreempteeJ searching preemptees list.");

  if ((PreemptorJ->Credential.Q != NULL) &&
      (PreemptorJ->Credential.Q->Preemptees[0]) != NULL)
    {
    mbool_t foundOne = FALSE;

    /* If preemptee has a QOS*/
    if (PreempteeJ->Credential.Q != NULL)
      {

      /* Make sure this preemptee is on the list. */
      for (qindex = 0;((qindex < MMAX_QOS) && (PreemptorJ->Credential.Q->Preemptees[qindex] != NULL));qindex++)
        {
        if (PreemptorJ->Credential.Q->Preemptees[qindex] ==
            PreempteeJ->Credential.Q)
          {

          /* This preemptee is on the list */
          foundOne = TRUE;
          break;
          }
        }
      }

    /* Return FALSE if this preemptee is not on the list */
    if (!foundOne)
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"job %s cannot preempt job %s - preemptee is not in QOS %s's PREEMPTEES list.\n",
                 PreemptorJ->Name,
                 PreempteeJ->Name,
                 PreemptorJ->Credential.Q->Name);

      return(FALSE);
      }
    }

  return(TRUE);
  }


/**
 * Report if Preemptor can preempt Preemptee.
 *
 * @see MJobGetNodePreemptList() - parent
 * @see MNodeGetPreemptList() - parent
 *
 * @param PreemptorJ (I)
 * @param PreempteeJ (I) 
 * @param PreemptorPriority (I) 
 * @param P (I) 
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * 
 * @return TRUE if PreempteeJ is eligible to be preempted by PreemptorJ
 */

mbool_t MJobCanPreempt(

  mjob_t        *PreemptorJ,
  const mjob_t  *PreempteeJ,
  const long     PreemptorPriority,
  const mpar_t  *P,
  char          *EMsg)

  {
  mbool_t IgnorePreempteePriority;
  int     PIndex = 0;

  const char *FName = "MJobCanPreempt";

  MDB(8,fRM) MLog("%s(%s,%s,%ld,%s,EMsg)\n",      
      FName,
      (PreemptorJ != NULL) ? PreemptorJ->Name : "NULL",
      (PreempteeJ != NULL) ? PreempteeJ->Name : "NULL",
      PreemptorPriority,
      (P != NULL) ? P->Name : "NULL");

  if ((NULL == PreemptorJ) || (NULL == PreempteeJ))
    return(FALSE);

  if (P != NULL)
    PIndex = P->Index;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* check preemptee flag */

  if (!bmisset(&PreempteeJ->Flags,mjfPreemptee))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"preemptee flag not set");

    return(FALSE);
    }

  if (bmisset(&PreempteeJ->IFlags,mjifWasCanceled))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"preemptee has been canceled");

    return(FALSE);
    }

  if (bmisset(&PreempteeJ->IFlags,mjifWasCanceled))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"preemptee has been canceled");

    return(FALSE);
    }

  if (PreempteeJ->State == mjsSuspended)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"job is suspended");

    return(FALSE);
    }

  if (bmisset(&PreempteeJ->IFlags,mjifPreemptFailed))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"previous preempt attempt failed");

    return(FALSE);
    }

  if (MJobReachedMinPreemptTime(PreempteeJ,EMsg) == FALSE)
    return(FALSE);

  if (MJobReachedMaxPreemptTime(PreempteeJ,EMsg) == TRUE)
    return(FALSE);

  if ((MSched.JobPreemptCompletionTime > 0) &&
      (PreempteeJ->RemainingTime <= MSched.JobPreemptCompletionTime))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"job is too close to completion to preempt - %ld seconds left",
        PreempteeJ->RemainingTime);

    return(FALSE);
    }

  if (bmisset(&MSched.DisableSameCredPreemptionBM,maQOS) &&
      (PreempteeJ->Credential.Q != NULL) &&
      (PreemptorJ->Credential.Q != NULL) &&
      (PreempteeJ->Credential.Q == PreemptorJ->Credential.Q))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"cannot preempt jobs of same QOS");

    return(FALSE);
    }

  if (bmisset(&MSched.DisableSameCredPreemptionBM,maClass) &&
      (PreempteeJ->Credential.C != NULL) &&
      (PreemptorJ->Credential.C != NULL) &&
      (PreempteeJ->Credential.C == PreemptorJ->Credential.C))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"cannot preempt jobs of same Class");

    return(FALSE);
    }

  if (bmisset(&MSched.DisableSameCredPreemptionBM,maGroup) &&
      (PreempteeJ->Credential.G != NULL) &&
      (PreemptorJ->Credential.G != NULL) &&
      (PreempteeJ->Credential.G == PreemptorJ->Credential.G))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"cannot preempt jobs of same Group");

    return(FALSE);
    }

  if (bmisset(&MSched.DisableSameCredPreemptionBM,maAcct) &&
      (PreempteeJ->Credential.A != NULL) &&
      (PreemptorJ->Credential.A != NULL) &&
      (PreempteeJ->Credential.A == PreemptorJ->Credential.A))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"cannot preempt jobs of same Acct");

    return(FALSE);
    }

  if (bmisset(&MSched.DisableSameCredPreemptionBM,maUser) &&
      (PreempteeJ->Credential.U != NULL) &&
      (PreemptorJ->Credential.U != NULL) &&
      (PreempteeJ->Credential.U == PreemptorJ->Credential.U))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"cannot preempt jobs of same User");

    return(FALSE);
    }

  IgnorePreempteePriority = MSched.IgnorePreempteePriority;

/*
  TODO: need special case for missile command that checks preemption (getenv?)
*/

  if (!bmisset(&PreempteeJ->SpecFlags,mjfPreemptee))
    {
    mbitmap_t BM;

    bmset(&BM,mlActive);

    /* job became preemptee via QOS or system based SP/FS violation */

    if ((bmisset(&PreempteeJ->SysFlags,mjfSPViolation)) &&
       ((PreempteeJ->Credential.Q != NULL) && 
        (bmisset(&PreempteeJ->Credential.Q->Flags,mqfPreemptSPV)))) 
      {
      /* QOS based SPV preemption allowed */

      if (MJobCheckLimits(
            PreemptorJ,
            mptSoft,
            &MPar[0],
            &BM,
            NULL,
            NULL) == FAILURE)
        {
        /* job violates 'soft' active policy */

        bmset(&PreemptorJ->SysFlags,mjfSPViolation);
        bmset(&PreemptorJ->Flags,mjfSPViolation);

        MJobUpdateFlags(PreemptorJ);

        if (EMsg != NULL)
          sprintf(EMsg,"job violates soft policies");

        return(FALSE);
        }

      /* NO-OP */
      }
    else if ((bmisset(&PreempteeJ->SysFlags,mjfFSViolation)) &&
            ((PreempteeJ->Credential.Q != NULL) &&
             (bmisset(&PreempteeJ->Credential.Q->Flags,mqfPreemptFSV))))
      {
      /* QOS based FSV preemption allowed */

      /* NO-OP */
      }
    else if ((bmisset(&PreempteeJ->SysFlags,mjfSPViolation)) &&
             (MSched.EnableSPViolationPreemption == TRUE))
      {
      if (MJobCheckLimits(
            PreemptorJ,
            mptSoft,
            &MPar[0],
            &BM,
            NULL,
            NULL) == FAILURE)
        {
        /* job violates 'soft' active policy */

        bmset(&PreemptorJ->SysFlags,mjfSPViolation);
        bmset(&PreemptorJ->Flags,mjfSPViolation);

        MJobUpdateFlags(PreemptorJ);

        if (EMsg != NULL)
          sprintf(EMsg,"job violates soft policies");

        return(FALSE);
        }

      if (PreempteeJ->Credential.C != PreemptorJ->Credential.C)
        {
        if (EMsg != NULL)
          sprintf(EMsg,"cannot preempt SP violation jobs in same class");

        return(FALSE);
        }

      if (bmisset(&PreemptorJ->SysFlags,mjfSPViolation))
        {
        if (EMsg != NULL)
          sprintf(EMsg,"preemptor violates soft policies");

        return(FALSE);
        }

      if (!bmisset(&PreemptorJ->Flags,mjfSPViolation))
        {
        IgnorePreempteePriority = TRUE;
        }
      }
    else if ((bmisset(&PreempteeJ->SysFlags,mjfFSViolation)) &&
             (MSched.EnableFSViolationPreemption == TRUE))
      {
      if (PreempteeJ->Credential.C != PreemptorJ->Credential.C)
        {
        if (EMsg != NULL)
          sprintf(EMsg,"cannot preempt FS violation jobs in same class");

        return(FALSE);
        }

      if (bmisset(&PreemptorJ->SysFlags,mjfFSViolation))
        {
        if (EMsg != NULL)
          sprintf(EMsg,"preemptor violates fairshare");

        return(FALSE);
        }

      if (!bmisset(&PreemptorJ->Flags,mjfFSViolation))
        {
        IgnorePreempteePriority = TRUE;
        }
      }
    }    /* END if (!bmisset(&PreempteeJ->SpecFlags,mjfPreemptee)) */
  else
    {
    IgnorePreempteePriority = FALSE;
    }

  if (IgnorePreempteePriority == FALSE)
    {
    if ((PreempteeJ->PStartPriority[PIndex] == PreemptorPriority) &&
        !bmisset(&PreemptorJ->IFlags,mjifIsResuming))
      {
      if (EMsg != NULL)
        sprintf(EMsg,"preemptor priority is too low in partition %s", MPar[PIndex].Name);

      return(FALSE);
      }

    if (PreempteeJ->PStartPriority[PIndex] > PreemptorPriority)
      {
      if (EMsg != NULL)
        sprintf(EMsg,"preemptor priority is too low in partition %s", MPar[PIndex].Name);

      return(FALSE);
      }
    }

  /* Check the Preemptor's PREEMPTEES list for this job's QOS */

  if (__MJobCanPreemptQOS(PreemptorJ, PreempteeJ, EMsg) == FALSE)
    {
    return(FALSE);
    }

  /* job can preempt */

  return(TRUE);
  }  /* END MJobCanPreempt() */



/**
 * Wrapper to MJobCanPreempt in order to accomodate ignIdleJobRsv jobs.
 *
 * IgnIdleJobRsv are checked in MQueueCheckStatus. They need to be checked 
 * against things like JobPreemptMinActiveTime and DisableSameCredPreemption
 * options, which MJobCanPreempt checks.
 *
 * IgnIdleJobRsv don't have the preempt flag by default, so this flag will
 * place it on the job temporarily if it doesn't already exist. Their priority
 * doesn't matter so MSched.IgnorePreempteePriority will be set to TRUE
 * temporarily as well.
 *
 * @see MJobCanPreempt
 *
 * @param PreemptorJ (I)
 * @param PreempteeJ (I)
 * @param PreemptorPriority (I)
 * @param EMsg (O) [optiona,minsize=MMAX_LINE]
 */

mbool_t MJobCanPreemptIgnIdleJobRsv(


  mjob_t  *PreemptorJ,         /* I */
  mjob_t  *PreempteeJ,         /* I */
  long     PreemptorPriority,  /* I */
  char    *EMsg)               /* O (optional,minsize=MMAX_LINE) */

  {
  int rc;
  mbool_t removePreempteeFlag = FALSE;
  mbool_t removePreempteeSpecFlag = FALSE;
  mbool_t origIgnorePreempteePriority = MSched.IgnorePreempteePriority;

  MASSERT(PreemptorJ != NULL,"null preemptor when checking preemption against idle rsv job");
  MASSERT(PreempteeJ != NULL,"null preemptee when checking preemption against idle rsv job");
  
  MSched.IgnorePreempteePriority = TRUE;

  if (!bmisset(&PreempteeJ->Flags,mjfPreemptee))
    {
    removePreempteeFlag = TRUE;
    bmset(&PreempteeJ->Flags,mjfPreemptee);
    }

  /* don't need to check policy violations in MJobCanPreempt */
  if (!bmisset(&PreempteeJ->SpecFlags,mjfPreemptee)) 
    {
    removePreempteeSpecFlag = TRUE;
    bmset(&PreempteeJ->Flags,mjfPreemptee);
    }

  rc = MJobCanPreempt(PreemptorJ,PreempteeJ,PreemptorPriority,NULL,EMsg);

  if (removePreempteeFlag)
    bmunset(&PreempteeJ->Flags,mjfPreemptee);

  if (removePreempteeSpecFlag)
    bmunset(&PreempteeJ->SpecFlags,mjfPreemptee);

  MSched.IgnorePreempteePriority = origIgnorePreempteePriority;

  return(rc);
  } /* END MJobCanPreemptIgnIdleJobRsv() */



/**
 * Return TRUE is job is currently preempting.
 *
 * NOTE: uses JobRetryTime
 *
 * @param J
 */

mbool_t MJobIsPreempting(

  mjob_t *J)

  {
  if (MSched.GuaranteedPreemption == FALSE)
    {
    return(FALSE);
    }

  if (!bmisset(&J->Flags,mjfPreempted)) /* preemptor */
    {
    return(FALSE);
    }

  if (MPar[0].JobRetryTime <= 1)
    {
    return(FALSE);
    }

  if (J->InRetry == FALSE)
    {
    return(FALSE);
    }

  if (J->Rsv == NULL)
    {
    return(FALSE);
    }

  MDB(7,fRM) MLog("INFO:     job %s has preempted and is waiting to start\n",J->Name);

  return(TRUE);
  }  /* END MJobIsPreempting() */



/* END MPreempt.c */
