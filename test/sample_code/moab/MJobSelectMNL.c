/* HEADER */

      
/**
 * @file MJobSelectMNL.c
 *
 */

/* Contains:
 *  int MJobSelectMNL 
 *  int MJobCacheFNL
 *  int MJobGetCachedFNL
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * __MJobCanPreemptAny
 *
 * @param J (I)
 * @param AllowCancel (I)
 * @param AllowPreemption (I)
 */

mbool_t __MJobCanPreemptAny(

  const mjob_t  *J,
  mbool_t  AllowCancel,
  mbool_t  AllowPreemption)

  {
  int     qindex;
  mqos_t *Q;

  /* NOTE:  partition template jobs are not maintained in global job
            table and are thus not added to per QOS 'PreempteeCount'
            array used below */

  for (qindex = 0;qindex < MSched.M[mxoQOS];qindex++)
    {
    Q = &MQOS[qindex];

    if (Q->Name[0] == '\0')
      {
      /* no preemptees found */

      if (AllowCancel != TRUE)
        {
        return(FALSE);
        }

      break;
      }

    /* NOTE:  QOSPreempteeCount[] updated in MQueueAddAJob() */

    if (MSched.QOSPreempteeCount[qindex] <= 0)
      continue;

    if ((MSched.DisableSameQOSPreemption == TRUE) &&
        (J->Credential.Q == Q))
      continue;

    /* preemptee found */

    break;
    }  /* END for (qindex) */

  if (qindex >= MSched.M[mxoQOS])
    {
    return(FALSE);
    }

  return(AllowPreemption);
  }  /* END __MJobCanPreemptAny() */


/**
 * MJobCacheFNL
 */

int MJobCacheFNL(

  const mjob_t *J,
  const mreq_t *RQ,
  const mnl_t  *NL)

  {
  mjob_t *MasterJ = NULL;

  /* this needs to be disabled until we can get it going on a per-partition basis */

  return(FAILURE);

  if ((J == NULL) ||
      (RQ == NULL) ||
      (J->JGroup == NULL) || 
      (MNLIsEmpty(NL)))
    {
    return(FAILURE);
    }

  if (MSched.EnableHighThroughput == FALSE)
    {
    return(FAILURE);
    }

  if (MJobFind(J->JGroup->Name,&MasterJ,mjsmExtended) == FAILURE)
    {
    return(FAILURE);
    }

  if (MasterJ->Array == NULL)
    {
    return(FAILURE);
    }

  if (MasterJ->Array->FNL == NULL)
    {
    MasterJ->Array->FNL = (mnl_t **)MUCalloc(1,sizeof(mnl_t *) * MMAX_REQ_PER_JOB);

    MNLMultiInit(MasterJ->Array->FNL);
    }

  MNLCopy(MasterJ->Array->FNL[RQ->Index],NL);

  return(SUCCESS);
  }  /* END MJobCacheFNL() */



/**
 * MJobGetCachedFNL
 */

int MJobGetCachedFNL(

  const mjob_t *J,
  const mreq_t *RQ,
  mnl_t        *NL)

  {
  mjob_t *MasterJ = NULL;

  return(FAILURE);

  if ((J == NULL) ||
      (RQ == NULL) ||
      (J->JGroup == NULL) || 
      (NL == NULL))
    {
    return(FAILURE);
    }

  if (MJobFind(J->JGroup->Name,&MasterJ,mjsmBasic) == FAILURE)
    {
    return(FAILURE);
    }

  if ((MasterJ->Array == NULL) ||
      (MasterJ->Array->FNL == NULL) ||
      (MNLIsEmpty(MasterJ->Array->FNL[RQ->Index])))
    {
    return(FAILURE);
    }

  MNLCopy(NL,MasterJ->Array->FNL[RQ->Index]);

  return(SUCCESS);
  }  /* END MJobGetCachedFNL() */



/**
 * JobSelect Moab Node List - Select available nodes which can satisfy all constraints of specified job.
 *
 * NOTE: AMNL will report list of available nodes even if all requested 
 *       tasks cannot be located and FAILURE is returned
 *
 * NOTE: If preemption allowed, this routine will directly preempt jobs.
 *
 * NOTE: Enforce job feasibility, J->Hostlist, and J->ReqRID checks.  
 *       Check node state, node load, job reservations, and other factors.
 *
 * NOTE: Utilize job preemption if available and required.
 *
 * NOTE: only select nodes which are available immediately or take action 
 *       within this routine to make the nodes available
 * @see MQueueScheduleIJobs() - parent
 * @see MBF*() - parent
 * @see MReqGetFNL() - child
 * @see MJobGetINL() - child
 * @see MNodeGetPreemptList() - child - select feasible preemptee jobs if preemption is required
 * @see MJobSelectPJobList() - child - select best preemptee jobs from feasible preemption list
 *
 * @param J (I) job
 * @param P (I) node partition
 * @param FNL (I) [optional]
 * @param AMNL (O) array of nodes that can run job
 * @param NodeMap (O) node availability array
 * @param SAllowPreemption (I)
 * @param AllowCancel (I)
 * @param PIsReq (O) [optional,preemption is required]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 *
 * @return
 *  SUCCESS and set AMNL to list of all available nodes that can run job at MSched.Time 
 *  or 'FAILURE' if job cannot run now
 */

int MJobSelectMNL(

  mjob_t       *J,
  const mpar_t *P,
  mnl_t        *FNL,
  mnl_t       **AMNL,
  char         *NodeMap,
  mbool_t       SAllowPreemption,
  mbool_t       AllowCancel,
  mbool_t      *PIsReq,
  char         *EMsg,
  int          *SC)

  {
  int           ITC[MMAX_REQ_PER_JOB];
  int           INC[MMAX_REQ_PER_JOB];

  int           PTC[MMAX_REQ_PER_JOB];
  int           PNC[MMAX_REQ_PER_JOB];

  int           TotalAvailINC[MMAX_REQ_PER_JOB];
  int           TotalAvailITC[MMAX_REQ_PER_JOB];
  int           TotalAvailIPC[MMAX_REQ_PER_JOB];

  int           TotalAvailPNC[MMAX_REQ_PER_JOB];
  int           TotalAvailPTC[MMAX_REQ_PER_JOB];

  int           PreempteeTCList[MMAX_REQ_PER_JOB][MMAX_JOB];    /* preemptor taskcount provided by preempting */
                                              /* preemptee job - reported by MJobSelectPJobList() */
  int           PreempteeNCList[MMAX_REQ_PER_JOB][MMAX_JOB];
  mnl_t        *PreempteeTaskList[MMAX_REQ_PER_JOB][MMAX_JOB];  /* alloc */

  mbool_t       ReqCanRun[MMAX_REQ_PER_JOB];

  int           ReqNC[MMAX_REQ_PER_JOB];
  int           ReqTC[MMAX_REQ_PER_JOB];
  int           ReqPC[MMAX_REQ_PER_JOB];

  char          tmpEMsg[MMAX_LINE];

  mjob_t       *FeasiblePreempteeJobList[MMAX_REQ_PER_JOB][MMAX_JOB];
  mjob_t       *PreempteeJobList[MMAX_REQ_PER_JOB][MMAX_JOB];             
  mjob_t       *PreemptedJobList[MMAX_REQ_PER_JOB][MMAX_JOB];   /* used to resume preempted jobs in case of failure */

  mjob_t       *ExcludedPreempteeList[MMAX_JOB]; /* used to guarantee reqs do not overlap */

  mjob_t       *tJ;

  mnl_t        *TL;

  mbool_t AllowPreemption = FALSE;
  mbool_t PIsConditional;

  int     index;
  int     jindex;
  int     jindex2;
  
  int     PJCount = 0;   /* number of successfully preempted jobs */

  int     rqindex;

  int     TotalJobPC;

  int     RCount[MMAX_REQ_PER_JOB][marLAST];

  mnl_t  *NodeList = NULL;

  mbool_t    PreemptNodesRequired = FALSE;
  mbool_t    MaxPreemptionReached = FALSE;

  mreq_t    *RQ = NULL;

  mnode_t   *N;

  int rc = FAILURE;

  /* WARNING:  must be freed before returning */

  mnl_t     *IdleNode[MMAX_REQ_PER_JOB];
  mnl_t      PNL;
  mnl_t     *LocalFNL[MMAX_REQ_PER_JOB];
  mnl_t     *tmpMNL[MMAX_REQ_PER_JOB];
  mnl_t      tmpNL = {0};;

  const char *FName = "MJobSelectMNL";

  MDB(4,fSCHED) MLog("%s(%s,%s,%s,AMNL,NodeMap,MaxSpeed,%s,%s,PIsReq,EMsg,SC)\n",
    FName,
    (J !=NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    (FNL == NULL) ? "NULL" : "FNL",
    MBool[SAllowPreemption],
    MBool[AllowCancel]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = 0;

  if (PIsReq != NULL)
    *PIsReq = FALSE;

    if (AMNL != NULL)
    {
      /* NOTE:   all entries must be clear to avoid seg faults when processing nodes*/
      MNLMultiClear(AMNL);
     }

  if ((J == NULL) || (P == NULL))
    {
    MDB(4,fSCHED) MLog("ALERT:    invalid job in %s\n",
      FName);

    if (EMsg != NULL)
      sprintf(EMsg,"invalid job or partition");

    return(FAILURE);
    }

  if (MJobIsPreempting(J) == TRUE)
    {
    return(FAILURE); /* Get a reservation */
    }

  /* NOTE:  locate all idle feasible tasks */
  /*        locate preemptible resources   */

  if ((SAllowPreemption != FALSE) && (J->Credential.Q != NULL) &&
      (MRsvOwnerPreemptExists() == FALSE))
    {
    SAllowPreemption = __MJobCanPreemptAny(J,AllowCancel,SAllowPreemption);
    }  /* END if ((SAllowPreemption != FALSE) && (J->Cred.Q != NULL)) */

  if ((MSched.MaxJobStartPerIteration >= 0) &&
      (MSched.JobStartPerIterationCounter >= MSched.MaxJobStartPerIteration))
    {
    /* NOTE:  incomplete.  policy must block backfill, reserve, and suspend
              job start */

    MDB(4,fSCHED) MLog("ALERT:    job '%s' blocked by JOBMAXSTARTPERITERATION policy in %s (%d >= %d)\n",
      J->Name,
      FName,
      MSched.JobStartPerIterationCounter,
      MSched.MaxJobStartPerIteration);

    if (EMsg != NULL)
      {
      sprintf(EMsg,"maxjobstartperiteration limit reached");
      }

    return(FAILURE);
    }

  memset(NodeMap,mnmUnavailable,sizeof(char) * MSched.M[mxoNode]);

  /* NOTE:  overcommit preemption requires externally signalling that
            preemption is occuring and that dedicated resource violations
            are acceptable */

  if (bmisset(&J->Flags,mjfCoAlloc))
    P = &MPar[0];

  MNLMultiInit(IdleNode);
  MNLMultiInit(LocalFNL);
  MNLMultiInit(tmpMNL);
  MNLInit(&tmpNL);
  MNLInit(&PNL);

  /* populate local feasible node list */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    MNLClear(&tmpNL);

    ReqCanRun[rqindex] = FALSE;

    if (MJobGetCachedFNL(J,RQ,&tmpNL) == FAILURE)
      {
      if (MReqGetFNL(
           J,
           RQ, 
           P,
           FNL,     /* I */
           &tmpNL,  /* O */
           NULL,
           NULL,      
           MMAX_TIME,  /* Is MMAX_TIME passed in to allow superset nodelist to be returned for preemption? */
           0,
           NULL) == FAILURE)
        {
        /* insufficient feasible nodes found */
   
        MDB(4,fSCHED) MLog("INFO:     cannot locate adequate feasible tasks for job %s:%d\n",
          J->Name,
          rqindex);
      
        if (EMsg != NULL)
          sprintf(EMsg,"inadequate feasible tasks for job req %d",
            rqindex);
    
        rc = FAILURE;
   
        goto free_and_return;
        }
   
      MJobCacheFNL(J,RQ,&tmpNL);
      }  /* END if (MJobGetCachedFNL(J,RQ,&tmpNL) == FAILURE) */

    /* locate idle nodes which meet all job criteria */

    MJobGetINL(
      J,
      RQ,
      &tmpNL,               /* I */
      IdleNode[rqindex],    /* O */
      P->Index,
      &INC[rqindex],                 /* O */
      &ITC[rqindex]);                /* O */

    /* Note the state if MaxPreemption has been Reached */
    MaxPreemptionReached = ((MSched.MaxJobPreemptPerIteration > 0) && 
                            (MSched.JobPreempteePerIterationCounter >= MSched.MaxJobPreemptPerIteration));

    /* early feasibility check: feasible idle nodes found */

    if (ITC[rqindex] < RQ->TaskCount)
      {
      MDB(4,fSCHED) MLog("INFO:     insufficient idle tasks in partition %s for %s:%d (%d of %d available)\n",
        P->Name,
        J->Name,
        RQ->Index,
        ITC[rqindex],
        RQ->TaskCount);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"INFO:     insufficient idle tasks in partition %s for %s:%d (%d of %d available)\n",
          P->Name,
          J->Name,
          RQ->Index,
          ITC[rqindex],
          RQ->TaskCount);
        }

      if ((SAllowPreemption == FALSE) || (MaxPreemptionReached == TRUE))
        {
        /* idle tasks only allowed */

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"INFO:     insufficient idle tasks in partition %s for %s:%d (%d of %d available/nopreemption)\n",
            P->Name,
            J->Name,
            RQ->Index,
            ITC[rqindex],
            RQ->TaskCount);
          }
         
        if (MaxPreemptionReached == TRUE)
          {
          MDB(2,fSCHED) MLog("INFO:     JOBMAXPREEMPTPERITERATION reached: %d of %d\n",
            MSched.JobPreempteePerIterationCounter,
            MSched.MaxJobPreemptPerIteration);
          }

        if (AMNL != NULL)
          {
          for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
            { 
            MNLCopy(AMNL[rqindex],IdleNode[rqindex]);
            }
          }

        rc = FAILURE;

        goto free_and_return;
        }  /* END if ((SAllowPreemption == FALSE) || ...) */

      PreemptNodesRequired = TRUE;

      MDB(4,fSCHED) MLog("INFO:     will attempt to locate preempt resources for %s:%d\n",
        J->Name,
        RQ->Index);
      }    /* END if (ITC < RQ->TaskCount) */
    else if (INC[rqindex] < RQ->NodeCount)
      {
      MDB(4,fSCHED) MLog("INFO:     insufficient idle nodes in partition %s for %s:%d (%d of %d available)\n",
        P->Name,
        J->Name,
        RQ->Index,
        INC[rqindex],
        RQ->NodeCount);

      if ((SAllowPreemption == FALSE) || (MaxPreemptionReached == TRUE))
        {
        /* idle tasks only allowed */
 
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"INFO:     insufficient idle nodes in partition %s for %s:%d (%d of %d available/nopreemption)\n",
            P->Name,
            J->Name,
            RQ->Index,
            ITC[rqindex],
            RQ->TaskCount);
          }
        
        if (MaxPreemptionReached == TRUE)
          {
          MDB(2,fSCHED) MLog("INFO:     JOBMAXPREEMPTPERITERATION reached: %d of %d\n",
            MSched.JobPreempteePerIterationCounter,
            MSched.MaxJobPreemptPerIteration);
          }

        if (AMNL != NULL)
          {
          for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
            {
            MNLCopy(AMNL[rqindex],IdleNode[rqindex]);
            }
          }

        rc = FAILURE;

        goto free_and_return;
        }  /* END if ((SAllowPreemption == FALSE) || ...) */

      PreemptNodesRequired = TRUE;
      }    /* END else if (INC < RQ->NodeCount) */
    else
      {
      MDB(7,fSCHED) MLog("INFO:     adequate idle nodes/tasks located\n");
      }

    /* select idle resources */

    memset(RCount,0,sizeof(RCount));
 
    /* NOTE:  multi-req preemption not supported */
 
    TotalAvailITC[rqindex] = 0;
    TotalAvailINC[rqindex] = 0;
    TotalAvailIPC[rqindex] = 0;
    TotalAvailPNC[rqindex] = 0;
    TotalAvailPTC[rqindex] = 0;   

    /* success only if all required tasks are idle */

    if (MNodeSelectIdleTasks(
         J,
         RQ,
         IdleNode[rqindex],  /* I */
         tmpMNL,             /* O */
         &TotalAvailITC[rqindex],     /* O */
         &TotalAvailINC[rqindex],     /* O */
         NodeMap,            /* O */
         RCount) == FAILURE)
      {
      MDB(4,fSCHED) MLog("INFO:     insufficient matching resources selected in partition %s for %s:%d (%d of %d tasks/%d of %d nodes)\n",
        P->Name,
        J->Name,
        RQ->Index,
        TotalAvailITC[rqindex],
        J->Request.TC,
        TotalAvailINC[rqindex],
        J->Request.NC);
 
      if (SAllowPreemption == FALSE)
        {
        /* inadequate tasks found, only idle tasks allowed */

        MDB(8,fSCHED) MLog("INFO:     preemption not allowed for job %s\n",J->Name);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"INFO:     insufficient matching idle tasks in partition %s for %s:%d (%d of %d available)\n",
            P->Name,
            J->Name,
            RQ->Index,
            TotalAvailITC[rqindex],
            RQ->TaskCount);
          }

        /* copy tmpMNL to AMNL */

        if (AMNL != NULL)
          MNLMultiCopy(AMNL,tmpMNL);
 
        rc = FAILURE;

        goto free_and_return;
        }  /* END if (SAllowPreemption == FALSE) */

      if (MReqIsGlobalOnly(RQ) == TRUE)
        {
        int tmpRIndex;

        for (tmpRIndex = 0;tmpRIndex < rqindex;tmpRIndex++)
          {
          /* check to make sure all the previous reqs can run */

          if (ReqCanRun[tmpRIndex] == FALSE)
            break;
          }

        /* if all the previous reqs can run then go ahead and set the flag */

        if ((rqindex == tmpRIndex) || (ReqCanRun[tmpRIndex] == TRUE))
          bmset(&J->Flags,mjfBlockedByGRes);
        }

      PreemptNodesRequired = TRUE;
      }    /* END if (MNodeSelectIdleTasks() == FAILURE) */

    MNLCopy(LocalFNL[rqindex],&tmpNL);

    ReqCanRun[rqindex] = TRUE;

    /* adequate nodes located */

    if (bmisset(&J->IFlags,mjifHostList) && 
       (J->ReqHLMode == mhlmSubset))
      {
      mnode_t *tmpN;
      int nindex;
      int rqindex = J->HLIndex;

      for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&tmpN) == SUCCESS;nindex++)
        {
        if (MNLFind(LocalFNL[rqindex],tmpN,NULL) == SUCCESS)
          {
          /* one node from the subset has been found, that's good enough */
          break;
          }
        }

      if (MNLGetNodeAtIndex(&J->ReqHList,nindex,&tmpN) == FAILURE)
        {
        int nindex2;
        mbool_t HostFound = FALSE;

        for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
          {
          if (J->HLIndex != rqindex)
            continue;

          for (nindex2 = 0;MNLGetNodeAtIndex(LocalFNL[rqindex],nindex2,&tmpN) == SUCCESS;nindex2++)
            {
            if (tmpN != N)
              continue;

            HostFound = TRUE;

            break;
            }  /* END for (nindex2) */

          if (HostFound == TRUE)
            break;
          }    /* END for (rqindex) */

        if (HostFound == FALSE)
          {
          PreemptNodesRequired = TRUE;
 
          break;
          } 
        }      /* END for (hindex) */
      }        /* END if (bmisset(&J->IFlags,mjifHostList) && ...) */
    }          /* END for (rqindex) */

  /* NOTE:  multi-req not supported */

  rqindex = 0;

  if ((MSched.BluegeneRM == TRUE) && 
      (PreemptNodesRequired == FALSE)) /* if TRUE, then there weren't any idle resources */
    {
    mnl_t bgNL;

    /* check if job can run on idle nodes, if it can't see if it can
     * run on idle + preemptible nodes 
     *
     * scenario: 2 512 bg partitions and 1 1k partition. if there are jobs on 
     * the two 512 blocks and none on the 1k block and another 512 job is submitted 
     * moab will see that the 1k is available but the job can't run on the 1k block.
     * Moab won't know that it can't run the job on the 1k block till MJobWillRun. */

    MNLInit(&bgNL);

    MNLCopy(&bgNL,tmpMNL[0]);   /* copy tmpMNL in case the job can't run on given nodes,
                                   and so that the willrun for preempt nodes will have
                                   the full list of available nodes to select from. */

    if (MJobWillRun(J,&bgNL,NULL,MSched.Time,NULL,EMsg) == FALSE)
      {
      PreemptNodesRequired = TRUE;
      }
    else
      {
      /* copy the filtered node list back into tmpMNL. This is the list that job will 
       * run on. Currently, another willrun call will be done in MJobAllocMNL. The 
       * previous call had to be made know to know if it was needed to look for
       * preemptible nodes or not. The willrun in MJobAllocMNL is needed for priority 
       * reservations */

      MNLCopy(tmpMNL[0],&bgNL);
      }

    MNLFree(&bgNL);
    }

  if (PreemptNodesRequired == TRUE)
    {
    MaxPreemptionReached = ((MSched.MaxJobPreemptPerIteration > 0) && 
                            (MSched.JobPreempteePerIterationCounter >= MSched.MaxJobPreemptPerIteration));

    if (MaxPreemptionReached == TRUE)
      {
      MDB(2,fSCHED) MLog("INFO:     JOBMAXPREEMPTPERITERATION reached: %d of %d\n",
        MSched.JobPreempteePerIterationCounter,
        MSched.MaxJobPreemptPerIteration);

      rc = FAILURE;

      goto free_and_return;
      }

    PIsConditional = FALSE;

    if (SAllowPreemption == MBNOTSET)
      {
      /* determine if job can be preemptor */

      if (bmisset(&J->Flags,mjfPreemptor) == TRUE)
        {
        AllowPreemption = TRUE;
        }
      else
        {
        rsv_iter RTI;

        mrsv_t *R;

        AllowPreemption = FALSE;

        /* search reservation table */

        MRsvIterInit(&RTI);

        while (MRsvTableIterate(&RTI,&R) == SUCCESS)
          {
          if ((bmisset(&R->Flags,mrfOwnerPreempt) == FALSE) ||
              (bmisset(&R->Flags,mrfIsActive) == FALSE))
            continue;

          if (MCredIsMatch(&J->Credential,R->O,R->OType) == FALSE)
            continue;

          PIsConditional = TRUE;

          AllowPreemption = TRUE;
         
          break;
          }
        }    /* END else (bmisset(&J,mjfPreemptor) == TRUE) */

      if (AllowPreemption == FALSE)
        {
        MDB(8,fSCHED) MLog("INFO:     non-preemptor job cannot find resources\n");

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"non-preemptor job cannot find resources");
          }

        rc = FAILURE;

        goto free_and_return;
        }
      }    /* END if (SAllowPreemption == MBNOTSET) */
    else if (SAllowPreemption == TRUE)
      {
      if (bmisset(&J->Flags,mjfPreemptor))
        AllowPreemption = TRUE;
      }

    if (MSched.PreemptPolicy == mppOvercommit)
      {
      mnode_t *N;

      /* determine 'per queue' overcommit node */

      if ((J->Credential.C == NULL) || 
          (J->Credential.C->OCNodeName == NULL) ||
          (MNodeFind(J->Credential.C->OCNodeName,&N) == FAILURE))
        {
        /* cannot locate OC node */

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot locate 'overcommit' node");
          }

        rc = FAILURE;

        goto free_and_return;
        }

      /* NOTE:  only works for non-spanning jobs */

      TotalAvailITC[rqindex] += RQ->TaskCount;
      TotalAvailINC[rqindex] += 1;
      TotalAvailIPC[rqindex] += RQ->TaskCount * ((RQ->DRes.Procs == -1) ? N->CRes.Procs : RQ->DRes.Procs);

      MNLSetNodeAtIndex(tmpMNL[0],0,N);
      MNLSetTCAtIndex(tmpMNL[0],0,RQ->TaskCount);

      MNLTerminateAtIndex(tmpMNL[0],1);
      MNLTerminateAtIndex(tmpMNL[1],0);
      }  /* END if (MSched.PreemptPolicy == mppOvercommit) */
    else
      {
      int nindex;

      mbool_t ForceCancel = FALSE;

      ExcludedPreempteeList[0] = NULL;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        PNC[rqindex] = 0;
        PTC[rqindex] = 0;

        TotalAvailPNC[rqindex] = 0;
        TotalAvailPTC[rqindex] = 0;   

        PreempteeJobList[rqindex][0] = NULL;
        FeasiblePreempteeJobList[rqindex][0] = NULL;

        MDB(4,fSCHED) MLog("INFO:     attempting to locate preemptible resources\n");

        /* determine jobs/resources available for preemption */

        if ((AllowCancel == TRUE) && 
             bmisset(&J->IFlags,mjifRunNow) && 
             bmisset(&J->Credential.Q->Flags,mqfRunNow))
          {
          ForceCancel = TRUE;
          }

        MNodeGetPreemptList(
          J,
          RQ,
          (FNL == NULL) ?  LocalFNL[rqindex] : FNL,
          &PNL,                               /* O (contains nodes with preemptible resources) */
          ExcludedPreempteeList,              /* I */
          FeasiblePreempteeJobList[rqindex],  /* O (list of jobs to preempt) */
          J->PStartPriority[P->Index],
          P,
          PIsConditional,
          ForceCancel,
          &PNC[rqindex],                      /* O */
          &PTC[rqindex]);                     /* O */

        if ((PTC[rqindex] + ITC[rqindex] > 0) && bmisset(&J->SpecFlags,mjfBestEffort))
          {
          MDB(6,fSCHED) MLog("INFO:     insufficient P+I tasks for best effort job in partition %s: (%d of %d available)\n",
            P->Name,
            ITC[rqindex] + PTC[rqindex],
            J->Request.TC);
          }
        else 
          {
          if ((J->Req[1] == NULL) &&
             ((PTC[rqindex] + ITC[rqindex]) < J->Req[0]->TaskCount))
            {
            MDB(4,fSCHED) MLog("INFO:     insufficient P+I tasks in partition %s: (%d of %d available)\n",
              P->Name,
              ITC[rqindex] + PTC[rqindex],
              J->Request.TC);

            if (EMsg != NULL)
              {
              snprintf(EMsg,MMAX_LINE,"insufficient P+I tasks in partition %s: (%d of %d available)\n",
                P->Name,
                ITC[rqindex] + PTC[rqindex],
                J->Request.TC);
              }
   
            rc = FAILURE;

            goto free_and_return;
            }

          if ((J->Req[1] == NULL) &&
              (J->Request.NC > 0) &&
              ((PNC[rqindex] + INC[rqindex]) < J->Req[0]->NodeCount))
            {
            MDB(4,fSCHED) MLog("INFO:     insufficient P+I nodes in partition %s: (%d of %d available)\n",
              P->Name,
              INC[rqindex] + PNC[rqindex],
              J->Request.NC);

            if (EMsg != NULL)
              {
              snprintf(EMsg,MMAX_LINE,"insufficient P+I nodes in partition %s: (%d of %d available)\n",
                P->Name,
                INC[rqindex] + PNC[rqindex],
                J->Request.NC);
              }

            rc = FAILURE;

            goto free_and_return;
            }

          /* check initial task availability */

          TotalJobPC = J->TotalProcCount;

          if (J->Req[1] == NULL) 
            {
            /* single req job */

            if (TotalJobPC > (P->ARes.Procs + PTC[rqindex] * RQ->DRes.Procs))
              {
              MDB(4,fSCHED) MLog("INFO:     insufficient partition procs: (%d+%d available %d needed)\n",
                P->ARes.Procs,
                PTC[rqindex] * RQ->DRes.Procs,
                TotalJobPC);

              if (EMsg != NULL)
                {
                snprintf(EMsg,MMAX_LINE,"insufficient partition procs: (%d+%d available %d needed)\n",
                  P->ARes.Procs,
                  PTC[rqindex] * RQ->DRes.Procs,
                  TotalJobPC);
                }

              rc = FAILURE;

              goto free_and_return;
              }

            if (TotalJobPC > (MPar[0].ARes.Procs + PTC[rqindex] * RQ->DRes.Procs))
              {
              MDB(4,fSCHED) MLog("INFO:     insufficient GP procs: (%d+%d available %d needed)\n",
                MPar[0].ARes.Procs,
                PTC[rqindex] * RQ->DRes.Procs,
                TotalJobPC);

              if (EMsg != NULL)
                {
                snprintf(EMsg,MMAX_LINE,"insufficient GP procs: (%d+%d available %d needed)\n",
                  MPar[0].ARes.Procs,
                  PTC[rqindex] * RQ->DRes.Procs,
                  TotalJobPC);
                }

              rc = FAILURE;

              goto free_and_return;
              }
            }    /* END if (J->Req[1] == NULL) */
          }      /* END else (IsBestEffort) */

        /* locate available nodes */

        memset(RCount,0,sizeof(RCount));

        /* NOTE:  broken for multi-req */

        TotalAvailPTC[rqindex] = 0;
        TotalAvailPNC[rqindex] = 0;

        /* inadequate feasible idle nodes located */

        /* NOTE:  job may be global or conditional preemptor */

        /* inadequate tasks located */
    
        if (AllowPreemption == FALSE)
          { 
          MDB(6,fSCHED) MLog("INFO:     adequate tasks (P+I=%d+%d) located for job %s but preemption not allowed\n",
            PTC[rqindex],
            ITC[rqindex],
            J->Name);

          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"adequate tasks (P+I=%d+%d) located for job %s but preemption not allowed\n",
              PTC[rqindex],
              ITC[rqindex],
              J->Name);
            }

          rc = FAILURE;

          goto free_and_return;
          }

        MDB(6,fSCHED) MLog("INFO:     adequate tasks (P+I=%d+%d) located for job %s\n",
          PTC[rqindex],
          ITC[rqindex],
          J->Name);

        /* locate preemptible resources */

        TotalAvailPTC[rqindex] = TotalAvailITC[rqindex];
        TotalAvailPNC[rqindex] = TotalAvailINC[rqindex];

        /* preempt tasks required */

        ReqTC[rqindex] = MAX(0,RQ->TaskCount - TotalAvailITC[rqindex]);
        ReqNC[rqindex] = MAX(0,RQ->NodeCount - TotalAvailINC[rqindex]);
        ReqPC[rqindex] = MAX(0,(RQ->TaskCount * RQ->DRes.Procs) - TotalAvailITC[rqindex]);

        if ((MSched.BluegeneRM == TRUE) &&
            (J->DestinationRM != NULL) &&
            (J->DestinationRM->Version >= 20100))
          {
          mnl_t bgNL;

          MNLInit(&bgNL);

          MNLCopy(&bgNL,tmpMNL[0]);

          /* Slurm will say which jobs to preempt. */

          if (MJobSelectBGPJobList(
                J,
                FeasiblePreempteeJobList[rqindex],      /* I */
                &bgNL,                         /* I/O */
                PreempteeJobList[rqindex],     /* O (list of selected preemptible jobs) */
                PreempteeTCList[rqindex],      /* O */
                PreempteeNCList[rqindex],      /* O */
                PreempteeTaskList[rqindex]) == FAILURE) /* O (per job list of preemptible resources) */
            {
            MDB(7,fSCHED) MLog("INFO:     can't determine preemtible node list for bgl job.\n");

            /* return and get a priority reservation */

            MNLFree(&bgNL);

            rc = FAILURE;

            goto free_and_return;
            }

          /* Call MNodeSelectIdleTasks to get correct number of idle tasks since previous counts
           * could have included infeasible nodes for the slurm job. */

          /* Without this flag ignidlejobrsv jobs aren't accounted in TotalAvailITC */

          bmset(&J->IFlags,mjifIgnGhostRsv);

          MNodeSelectIdleTasks(
            J,
            RQ,
            &bgNL,              /* I */
            tmpMNL,             /* O */
            &TotalAvailITC[rqindex],     /* O */
            &TotalAvailINC[rqindex],     /* O */
            NodeMap,            /* O */
            RCount); 

          bmunset(&J->IFlags,mjifIgnGhostRsv);
    
          ReqTC[rqindex] = MAX(0,RQ->TaskCount - TotalAvailITC[rqindex]);
          ReqNC[rqindex] = MAX(0,RQ->NodeCount - TotalAvailINC[rqindex]);
          ReqPC[rqindex] = MAX(0,(RQ->TaskCount * RQ->DRes.Procs) - TotalAvailITC[rqindex]);

          MNLFree(&bgNL);
          }
        else
          {
          mnl_t *NL = &PNL;

          if (MSched.BluegeneRM == TRUE)
            {
            if (MSched.BGPreemption == TRUE)
              {
              mnl_t bgNL;

              MNLInit(&bgNL);

              /* find preemptees that match the preemptor's size exactly or find
               * preemptees starting with the smallest size. */

              if (MJobSelectBGPList(
                    J,
                    tmpMNL[0],    /* I */
                    &bgNL,        /* O */
                    FeasiblePreempteeJobList[rqindex]) == FAILURE) /* I/O (modified) */
                {
                MDB(7,fSCHED) MLog("INFO:     can't determine preemtible node list for bgl job.\n");

                /* return and get a priority reservation */

                MNLFree(&bgNL);

                rc = FAILURE;

                goto free_and_return;
                }

              MNLCopy(&PNL,&bgNL);

              MNLFree(&bgNL);

              NL = &PNL;
              }
            else
              {
              mnl_t *WillRunNL = NULL;
              /* see when job can run the soonest, even though selected nodeset might 
               * not be best topology (ex. preempting larger jobs before smaller ones). */

              /* merge INL and PNL and ask slurm if the job can be ran on the list of nodes.
               * Slurm has an understanding bluegene network topology. 
               * This has to be done, else moab could select nodes that the job can't run on. */
              
              WillRunNL = &PNL;

              MNLMerge(WillRunNL,WillRunNL,tmpMNL[0],NULL,NULL);

              if (MJobWillRun(J,WillRunNL,NULL,MSched.Time + MMAX_TIME,NULL,EMsg) == FAILURE)
                {
                MDB(7,fSCHED) MLog("INFO:     job can't run on idle/preemptible nodes according to willrun.\n");

                /* return and get a priority reservation */

                rc = FAILURE;

                goto free_and_return;
                }
              }

            /* Call MNodeSelectIdleTasks to get correct number of idle tasks since previous counts
             * could have included infeasible nodes for the slurm job. */

            MNodeSelectIdleTasks(
              J,
              RQ,
              NL,                 /* I */
              tmpMNL,             /* O */
              &TotalAvailITC[rqindex],     /* O */
              &TotalAvailINC[rqindex],     /* O */
              NodeMap,            /* O */
              RCount); 
        
            ReqTC[rqindex] = MAX(0,RQ->TaskCount - TotalAvailITC[rqindex]);
            ReqNC[rqindex] = MAX(0,RQ->NodeCount - TotalAvailINC[rqindex]);
            ReqPC[rqindex] = MAX(0,(RQ->TaskCount * RQ->DRes.Procs) - TotalAvailITC[rqindex]);
            }

          if (MJobSelectPreemptees(
               J,
               RQ,
               ReqTC[rqindex],                         /* I - tasks required for job that must be preempted */
               ReqNC[rqindex],
               FeasiblePreempteeJobList[rqindex],      /* I */
               NL,                            /* I - nodes which are feasible for job to run on */
               tmpMNL,                        /* I - idle tasks available to job */
               PreempteeJobList[rqindex],              /* O (list of selected preemptible jobs) */
               PreempteeTCList[rqindex],               /* O */
               PreempteeNCList[rqindex],               /* O */
               PreempteeTaskList[rqindex]) == FAILURE) /* O (per job list of preemptible resources) */
            {
            /* unable to select needed preempt jobs */

            MDB(6,fSCHED) MLog("INFO:     unable to select needed preempt jobs for job %s\n",
              J->Name);

            if (EMsg != NULL)
              snprintf(EMsg,MMAX_LINE,"unable to select needed preempt jobs for job");

            rc = FAILURE;

            goto free_and_return;
            }
          }    /* END else */

        /* add jobs in PreempteeJobList[rqindex] to ExcludePreempteeJobList */

        for (jindex = 0;PreempteeJobList[rqindex][jindex] != NULL;jindex++)
          {
          for (jindex2 = 0;jindex2 < MMAX_JOB;jindex2++)
            {
            if (ExcludedPreempteeList[jindex2] == NULL)
              {
              ExcludedPreempteeList[jindex2] = PreempteeJobList[rqindex][jindex];

              ExcludedPreempteeList[jindex2 + 1] = NULL;

              break;
              }
            }   /* END for (jindex2) */
          }     /* END for (jindex) */

        /* job can preempt needed resources */

#ifdef __MPREEMPTEVAL
        /* determine earliest job could start without preemption */

        if (MJobReserve(&J,NULL,0,jrPriority) == SUCCESS)
          {
          char TString[MMAX_LINE];

          MULToTString(J->Rsv->StartTime - MSched.Time,TString);

          MDB(1,fALL) MLog("NOTE:     job %s would start in %s without preemption (PC: %d)\n",
            J->Name,
            TString,
            J->PreemptCount);

          MJobReleaseRsv(J,TRUE,TRUE);
          }
#endif /* __MPREEMPTEVAL */

        /* Job can preempt, reserve with accounting manager before preempting */
     
        if (MAMHandleStart(&MAM[0],J,mxoJob,NULL,NULL,NULL) == FAILURE)
          {
          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"Unable to register job start with accounting manager");
            }
     
          /* This needs to be augmented to apply StartFailureAction as in 
           * MJobStart to avoid the job looping until it is deferred */

          rc = FAILURE;

          goto free_and_return;
          }

        /* preempt selected jobs and add preempted resources to 
           existing idle resource list */

        nindex = TotalAvailINC[rqindex];  /* HACK */

        NodeList = tmpMNL[rqindex];        

        MDB(1,fALL) MLog("INFO:     preempting jobs to allow job %s to start - required resources  T: %d  N: %d  P: %d\n",
          J->Name,
          ReqTC[rqindex],
          ReqNC[rqindex],
          ReqPC[rqindex]);

        PreemptedJobList[rqindex][0] = NULL;

        if ((MPar[0].RsvRetryTime > 0) &&
            (MSched.NoWaitPreemption != TRUE) &&
            (bmisset(&J->Flags,mjfPreempted)) &&
            (MPar[0].RsvRetryTime + J->PreemptTime > MSched.Time))
          {
          /* If RESERVATIONRETRYTIME is set, use that to block.  First preemptee may just be taking a while to preempt.  Don't want to preempt more jobs if not needed. This can be turned off with NOWAITPREEMPTION */

          MDB(4,fSCHED) MLog("INFO:    Job %s cannot preempt now, has already preempted at %d, can preempt in %d seconds\n",
            J->Name,
            J->PreemptTime,
            (MPar[0].RsvRetryTime + J->PreemptTime) - MSched.Time);
   
          rc = FAILURE;

          goto free_and_return;
          }
        }    /* END for (rqindex) */

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        for (jindex = 0;PreempteeJobList[rqindex][jindex] != NULL;jindex++)
          {
          /* If bluegene, don't stop preempting when enough tasks are found to
           * start the job because slurm tells moab which jobs to preempt to
           * start the preemptor. */

          if ((MSched.BluegeneRM == FALSE) &&
              (ReqTC[rqindex] <= 0) && (ReqNC[rqindex] <= 0) && (ReqPC[rqindex] <= 0))
            {
            /* all required resources located - free dynamically allocated structures */
#if defined(MPREEMPTDEBUG)
            fprintf(stderr,"freeing %lu at PreempteeTaskList[%d][%d] (%d)\n",(unsigned long)PreempteeTaskList[rqindex][jindex],rqindex,jindex,__LINE__);
#endif /* MPREEMPTDEBUG */

            MNLFree(PreempteeTaskList[rqindex][jindex]);

            continue;
            }

          tJ = PreempteeJobList[rqindex][jindex];

#if 0  /* because a single preemptee may contribute less than 1 task but 
          still contribute something we still add it */

          if (PreempteeTCList[rqindex][jindex] <= 0)
            {
            MDB(1,fALL) MLog("ALERT:    preemptee list is corrupt - preemptee %s selected providing 0 tasks to preemptor %s\n",
              tJ->Name,
              J->Name);

            continue;
            }
#endif

          /* NOTE:  partition template resources preempted via
                    provisioning activity of preemptor activity. */

          /* NOTE:  MJobPreempt() releases resources from resource manager
                    and from scheduler reservations */

          MDB(2,fSCHED) MLog("INFO:     job %s preempting job %s (statemtime: %ld) (preempted this iteration: %d)\n",
            J->Name,
            tJ->Name,
            tJ->StateMTime,
            MSched.JobPreempteePerIterationCounter);

          if (MJobPreempt(
                tJ,
                J,
                PreempteeJobList[rqindex],
                PreempteeTaskList[rqindex][jindex],
                (bmisset(&J->IFlags,mjifIsDynReq)) ? J->Description : J->Name,
                mppNONE,
                FALSE,
                NULL,
                tmpEMsg,
                NULL) == FAILURE)
            {
#if defined(MPREEMPTDEBUG)
            fprintf(stderr,"freeing %lu at PreempteeTaskList[%d][%d] (%d)\n",(unsigned long)PreempteeTaskList[rqindex][jindex],rqindex,jindex,__LINE__);
#endif /* MPREEMPTDEBUG */

            MNLFree(PreempteeTaskList[rqindex][jindex]);

            MDB(2,fSCHED) MLog("ALERT:    job %s cannot preempt job %s - will try other eligible preemptees - %s\n",
              J->Name,
              tJ->Name,
              tmpEMsg);

            /* NOTE:  if partial failure detected, adequate resources may still 
                      be available */

            /* evaluate next preemptee job */
 
            continue;
            }  /* END if (MJobPreempt() == FAILURE) */

          bmset(&J->Flags,mjfPreempted);
          bmset(&J->SpecFlags,mjfPreempted);

          PreemptedJobList[rqindex][PJCount++] = tJ;
          PreemptedJobList[rqindex][PJCount]   = NULL;

          /* NOTE:  PreempteeTCList may be 0, this is expected when a single preemptee
                    does not provide a full task to a job, but a partial task */

          ReqTC[rqindex] -= PreempteeTCList[rqindex][jindex];
          ReqPC[rqindex] -= PreempteeTCList[rqindex][jindex];
          ReqNC[rqindex] -= PreempteeNCList[rqindex][jindex];

          MDB(2,fSCHED) MLog("NOTE:     job %s preempted job %s - added idle resources (T: %d  N: %d  P: %d)/remaining (T: %d  N: %d  P: %d)\n",
            J->Name,
            tJ->Name,
            PreempteeTCList[rqindex][jindex],
            PreempteeNCList[rqindex][jindex],
            PreempteeTCList[rqindex][jindex],
            ReqTC[rqindex],
            ReqNC[rqindex],
            ReqPC[rqindex]);

          TotalAvailITC[rqindex] += PreempteeTCList[rqindex][jindex];
          TotalAvailIPC[rqindex] += PreempteeTCList[rqindex][jindex];
          TotalAvailINC[rqindex] += PreempteeNCList[rqindex][jindex];    

          /* determine nodes made available */

          TL = PreempteeTaskList[rqindex][jindex];

          NodeList = tmpMNL[rqindex];        

          for (index = 0;MNLGetNodeAtIndex(TL,index,&N) == SUCCESS;index++)
            {
            MNLAdd(NodeList,N,MNLGetTCAtIndex(TL,index));

            NodeMap[N->Index] = mnmPreemptible;

            nindex++;
            }  /* END for (index) */

#if defined(MPREEMPTDEBUG)
          fprintf(stderr,"freeing %lu at PreempteeTaskList[%d] (%d)\n",(unsigned long)PreempteeTaskList[rqindex][jindex],jindex,__LINE__);
#endif /* MPREEMPTDEBUG */

          MNLFree(PreempteeTaskList[rqindex][jindex]);
          }    /* END for (jindex) */
        }      /* END for (rqindex) */

      /* increment preempt counter if preemption was successful */
      /*  even if inadequate preempt nodes become available */

      MSched.JobPreemptorPerIterationCounter++;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        if (!bmisset(&J->SpecFlags,mjfBestEffort) &&
           ((ReqTC[rqindex] > 0) || (ReqNC[rqindex] > 0) || (ReqPC[rqindex] > 0)))
          {
          /* FIXME: need to resume suspended jobs so that jobs do not run out of order */

          MDB(2,fSCHED) MLog("ALERT:    unable to preempt adequate resources to start job %s\n",
            J->Name);

          if (EMsg != NULL)
            snprintf(EMsg,MMAX_LINE,"unable to preempt adequate resources to start job");

          if (MSched.ResumeOnFailedPreempt == TRUE)
            {
            for (jindex = 0;jindex < MMAX_JOB;jindex++)
              {
              tJ = PreemptedJobList[rqindex][jindex];

              if (tJ == NULL)
                break;

              tJ->SubState = mjsstNONE;

              MRsvJCreate(tJ,NULL,MSched.Time,mjrActiveJob,NULL,FALSE);
              }
            }

          /* Clean up any leftover liens */
          MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,NULL);

          rc = FAILURE;

          goto free_and_return;
          }  /* END if ((ReqTC[rqindex] > 0) || (ReqNC[rqindex] > 0) || (ReqPC[rqindex] > 0)) */
        }    /* END for (rqindex) */

      bmset(&J->IFlags,mjifPreemptCompleted);

      if ((J->DestinationRM != NULL) && (J->DestinationRM->PreemptDelay > 0))
        {
        bmset(&J->IFlags,mjifProlog);
        }

      J->PreemptTime = MSched.Time;

      if (PIsReq != NULL)
        *PIsReq = TRUE;

/*
      NodeList[nindex].N = NULL; 
*/
      }  /* END else (MSched.PreemptPolicy == mppOvercommit) */
    }    /* END if (PreemptNodesRequired == TRUE) */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    MDB(4,fSCHED) MLog("INFO:     %d(%d) tasks/%d(%d) nodes found for job %s in %s\n",
      TotalAvailITC[rqindex],
      TotalAvailPTC[rqindex], 
      TotalAvailINC[rqindex],
      TotalAvailPNC[rqindex], 
      J->Name,
      FName);
    }

  {
  if (MJobWantsSharedMem(J) == TRUE)
    {
    MJobPTransform(J,P,0,TRUE);

    rc = MJobDistributeMNL(J,tmpMNL,AMNL);

    MJobPTransform(J,P,0,FALSE);
    }
  else
    {
    rc = MJobDistributeMNL(  /* NOTE:  distribute tasks across reqs and copy resulting node info into final output buffer, AMNL[] */
           J,                /* I */
           tmpMNL,           /* I */
           AMNL);            /* O */
    }

  if (rc == FAILURE)
    {
    /* insufficient nodes were found */

    MDB(4,fSCHED)
      {
      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        MDB(4,fSCHED) MLog("INFO:     insufficient tasks found for job %s:%d in MJobDistributeMNL (%d required)\n",
          J->Name,
          rqindex,
          RQ->TaskCount);

        for (index = 0;index < marLAST;index++)
          {
          if (RCount[rqindex][index] > 0)
            {
            MLogNH("[%s: %d]",
              MAllocRejType[index],
              RCount[rqindex][index]);
            }
          }    /* END for (index) */

        MLogNH("\n");
        }  /* END for (rqindex) */
      }    /* END MDB(4,fSCHED) */

    if (EMsg != NULL)
      sprintf(EMsg,"cannot distribute tasks");

    /* Clean up any leftover liens */
    MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,NULL);

    rc = FAILURE;

    goto free_and_return;
    }  /* END if (MJobDistributeMNL()) == FAILURE) */
  }  /* END BLOCK */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    MDB(4,fSCHED) MLog("INFO:     resources found for job %s tasks: %d+%d of %d  nodes: %d+%d of %d\n",
      J->Name,
      TotalAvailITC[rqindex],
      TotalAvailPTC[rqindex],   
      J->Request.TC,
      TotalAvailINC[rqindex],
      TotalAvailPNC[rqindex],   
      J->Request.NC);
    }

  if ((AMNL != NULL) && (MNLIsEmpty(AMNL[0])) && (!bmisset(&J->Flags,mjfNoResources)))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"no resources found");

    /* Clean up any leftover liens */
    MAMHandleDelete(&MAM[0],(void *)J,mxoJob,NULL,NULL);

    rc = FAILURE;

    goto free_and_return;
    }

free_and_return:

  MNLMultiFree(IdleNode);
  MNLMultiFree(LocalFNL);
  MNLMultiFree(tmpMNL);
  MNLFree(&tmpNL);
  MNLFree(&PNL);

  return(rc);
  }  /* END MJobSelectMNL() */

