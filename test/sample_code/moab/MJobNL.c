/* HEADER */

      
/**
 * @file MJobNL.c
 *
 * Contains: MJob Node List functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report list of idle nodes acessible by specified job.
 *
 * @see MJobSelectMNL() - parent
 *
 * NOTE:  Should check any attribute which could differentiate a feasible node 
 *        from an available node.
 *
 * NOTE:  checks node state, estate, CRes.Procs/DRes.Procs, ActiveOS, ReqRID, 
 *        and node policies.
 *
 * NOTE:  this routine may evaluate both physical and logical resources (ie, 
 *        GLOBAL node containing software licenses or generic resources).  Some
 *        checks are only valid for processor-centric resources depending on
 *        workload requirements.
 *
 * NOTE:  this routine tracks physical vs VM resources
 *
 * @param J (I)
 * @param RQ (I)
 * @param FNL (I)
 * @param INL (O)
 * @param PIndex (I)
 * @param NodeCount (O) [optional]
 * @param TaskCount (O) [optional]
 */

int MJobGetINL(

  const mjob_t  *J,
  const mreq_t  *RQ,
  const mnl_t   *FNL,
  mnl_t         *INL,
  int            PIndex,
  int           *NodeCount,
  int           *TaskCount)

  {
  static mulong     IdleListMTime[MMAX_PAR];      
  static int        ActiveJobCount[MMAX_PAR];     
  static int        IdleNodeCount[MMAX_PAR];

  static mnl_t     *IdleNodeCache[MMAX_PAR];         
  static mbool_t    IdleNodeCacheInit = FALSE;

  static int       InitializeTimeStamp = TRUE;

  int index;
  int nindex;
  int rqindex;

  int tmpNC;
  int tmpTC;

  int tmpI;

  mnode_t *N;

  mpar_t *P = &MPar[PIndex];

  mbool_t AllowComputeOnly = TRUE;  /* if FALSE, workload possesses at least one non-compute req */

  mreq_t *tmpRQ;

  const char *FName = "MJobGetINL";

  MDB(4,fSCHED) MLog("%s(%s,%s,%s,%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (FNL != NULL) ? "FNL" : "NULL",
    (INL != NULL) ? "INL" : "NULL",
    P->Name,
    (NodeCount != NULL) ? "NodeCount" : "NULL",
    (TaskCount != NULL) ? "TaskCount" : "NULL");

  if ((J == NULL) || (INL == NULL))
    {
    return(FAILURE);
    }

  if (IdleNodeCacheInit == FALSE)
    {
    MNLMultiInit(IdleNodeCache);

    IdleNodeCacheInit = TRUE;
    }

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    tmpRQ = J->Req[rqindex];

    if (tmpRQ == NULL)
      break;

    if (tmpRQ->DRes.Procs == 0)
      {
      /* non-compute job req located */

      AllowComputeOnly = FALSE;

      break;
      }
    }    /* END for (rqindex) */

  /* check resources, state, and policies */

  if (InitializeTimeStamp == TRUE)
    {
    memset(IdleListMTime,0,sizeof(IdleListMTime));
    memset(ActiveJobCount,0,sizeof(ActiveJobCount));

    InitializeTimeStamp = FALSE;
    }

  /* process feasible list if provided */

  if (FNL == NULL)
    {
    /* update IdleList on new iteration or if new job has been started */
 
    if ((MSched.Time > IdleListMTime[P->Index]) ||
        (ActiveJobCount[P->Index] != MStat.ActiveJobs))
      {
      MDB(5,fSCHED) MLog("INFO:     refreshing IdleList for partition %s\n",
        P->Name);
 
      nindex = 0;

      tmpTC = 0;
   
      for (index = 0;index < MSched.M[mxoNode];index++)
        {
        N = MNode[index];

        if ((N == NULL) || (N->Name[0] == '\0'))
          break;

        if (N->Name[0] == '\1')
          continue;
 
        if ((N->PtIndex != P->Index) &&
            (PIndex != 0))
          {
          continue;
          }

        if (((N->State != mnsIdle) && (N->State != mnsActive)) ||
            ((N->EState != mnsIdle) && (N->EState != mnsActive)) ||
             (N->DRes.Procs >= N->CRes.Procs))
          {
          MDBO(6,N,fSCHED) MLog("INFO:     node '%s' is not idle.  (state: %s  exp: %s)\n",
            N->Name,
            MNodeState[N->State],
            MNodeState[N->EState]);

          continue;
          }

        /* acceptable node found */

        MNLSetNodeAtIndex(IdleNodeCache[P->Index],nindex,N);
        MNLSetTCAtIndex(IdleNodeCache[P->Index],nindex,N->CRes.Procs - N->DRes.Procs);
 
        tmpTC += N->CRes.Procs - N->DRes.Procs;

        nindex++;
        }    /* END for (index) */ 

      MNLTerminateAtIndex(IdleNodeCache[P->Index],nindex);
 
      MDB(5,fSCHED) MLog("INFO:     nodes placed in INL[%s] list: %3d (procs: %d)\n",
        P->Name,
        nindex,
        tmpTC);
 
      IdleListMTime[P->Index] = MSched.Time;
 
      ActiveJobCount[P->Index] = MStat.ActiveJobs;
 
      IdleNodeCount[P->Index] = nindex;
      }  /* END if ((MSched.Time > IdleListMTime[P->Index])) || ...) */

    /* determine available nodes from data */

    tmpNC = 0;
    tmpTC = 0;

    nindex = 0;

    for (index = 0;index < IdleNodeCount[P->Index];index++)
      {
      MNLGetNodeAtIndex(IdleNodeCache[P->Index],index,&N);

      tmpI = MNLGetTCAtIndex(IdleNodeCache[P->Index],index);

      if (MNodeCheckPolicies(J,N,MSched.Time,&tmpI,NULL) == FAILURE)
        {
        continue;
        }

      MNLSetNodeAtIndex(INL,nindex,N);
      MNLSetTCAtIndex(INL,nindex,tmpI);

      tmpTC += tmpI;

      tmpNC++;

      nindex++;
      }  /* END for (index) */

    MNLTerminateAtIndex(INL,nindex);
    }    /* END if (FNL == NULL) */
  else
    {
    /* process feasible node list */
 
    tmpTC  = 0;
    tmpNC  = 0;

    nindex = 0;
 
    for (index = 0;MNLGetNodeAtIndex(FNL,index,&N) == SUCCESS;index++)
      {
      /* check state and proc count */

      if (((N->State != mnsIdle) && (N->State != mnsActive)) ||
          ((N->EState != mnsIdle) && (N->EState != mnsActive)) ||
          ((N->CRes.Procs > 0) && (N->DRes.Procs >= N->CRes.Procs)))
        {
        MDBO(5,N,fSCHED) MLog("INFO:     node %s is not available for idle list (state: %s/%s  dprocs: %d)\n",
          N->Name,
          MNodeState[N->State],
          MNodeState[N->EState],
          N->DRes.Procs);

        continue;
        }

      /* NOTE: this routine only reports immediately available resources.
               Only return information for resources that have VMs currently
               in place. MJOBREQUIRESVMS() returns false for on-demand VM creation
               jobs */

      if ((MJOBREQUIRESVMS(J)) && (RQ->DRes.Procs > 0) &&
          (!bmisset(&MSched.Flags,mschedfOutOfBandVMRsv)))
        {
        mcres_t VMCRes;
        mcres_t VMDRes;

        MCResInit(&VMCRes);
        MCResInit(&VMDRes);

        MNodeGetVMRes(N,J,TRUE,&VMCRes,&VMDRes,NULL);

        tmpI = VMCRes.Procs - VMDRes.Procs;

        MCResFree(&VMCRes);
        MCResFree(&VMDRes);
        }
      else if ((MJOBISVMCREATEJOB(J)) && (RQ->DRes.Procs > 0) &&
               (!bmisset(&MSched.Flags,mschedfOutOfBandVMRsv)))
        {
        /* The RQ->DRes.Procs > 0 is so that this doesn't affect the storage reqs */

        mcres_t VMCRes;
        mcres_t VMDRes;

        MCResInit(&VMCRes);
        MCResInit(&VMDRes);

        MNodeGetVMRes(N,J,FALSE,&VMCRes,&VMDRes,NULL);

        /* subtract available VM resources from available node resources */

        tmpI = (N->CRes.Procs - N->DRes.Procs) - (VMCRes.Procs -  VMDRes.Procs);

        MCResFree(&VMCRes);
        MCResFree(&VMDRes);
        }
      else if (N->CRes.Procs != 0)
        {
        tmpI = N->CRes.Procs - N->DRes.Procs;
        }
      else 
        {
        /* node is non-compute resource */

        tmpI = 9999;
        }

      if ((AllowComputeOnly == TRUE) || (N->CRes.Procs > 0))
        {
        /* perform checks which are associated with compute resources */

        /* check active OS */

        if ((RQ->Opsys != 0) && 
            (N->ActiveOS != RQ->Opsys))
          {
          mbool_t OSCanMatch = FALSE;
          mln_t *P;
          mvm_t *V;

          /* OS of physical node does not match job requirements */

          /* check if QOS based provisioning is allowed */

          if ((J->Credential.Q != NULL) &&
             bmisset(&J->Credential.Q->Flags,mqfProvision))
            {
            /* NOTE:  can only provision server immediately if no VM's
                      are currently active */

            if (MNodeHasVMs(N) == FALSE)
              OSCanMatch = TRUE;
            }
          else if ((N->VMList != NULL) && 
                   (J->VMUsagePolicy != mvmupPMOnly) &&
                   (J->VMUsagePolicy != mvmupVMCreate))
            {
            /* search all VM's to locate matching idle VM */

            for (P = N->VMList;P != NULL;P = P->Next)
              {
              V = (mvm_t *)P->Ptr;
 
              if ((V->JobID[0] == '\0') && 
                  (V->State == mnsIdle) && 
                  (V->ActiveOS == RQ->Opsys))
                {
                /* NOTE:  node hosts non-dedicated, idle VM with matching OS */
 
                OSCanMatch = TRUE;

                break;
                } 
              }  /* END for (V) */
            }    /* END if ((J->RQ->Opsys != 0) && ...) */
          else if ((J->VMUsagePolicy == mvmupVMCreate))
            {
            /* TODO: qualify with a non-yet created attribute mnode_t#SupportedVMTypes */
            OSCanMatch = TRUE;
            }

          if (OSCanMatch == FALSE)
            {
            MDB(5,fSCHED) MLog("INFO:     job %s requires os %s but idle node %s has activeos %s\n",
              J->Name,
              MAList[meOpsys][RQ->Opsys],
              N->Name,
              MAList[meOpsys][N->ActiveOS]);

            continue;
            }
          }
        }    /* END if ((AllowComputeOnly == TRUE) || ...) */

      if ((!MReqIsGlobalOnly(RQ)) &&
          (J->ReqRID != NULL) &&
          (!bmisset(&J->IFlags,mjifNotAdvres))) 
        {
        if (MNodeRsvFind(N,J->ReqRID,TRUE,NULL,NULL) == FAILURE)
          {
          MDB(5,fSCHED) MLog("INFO:     job %s requires rsv %s missing from idle node %s\n",
            J->Name,
            J->ReqRID,
            N->Name);

          continue;
          }
        } /* END if (J->ReqRID != NULL)  */

      if (MNodeCheckPolicies(J,N,MSched.Time,&tmpI,NULL) == FAILURE)
        {
        MDB(5,fSCHED) MLog("INFO:     job %s violates policies on idle node %s\n",
          J->Name,
          N->Name);

        continue;
        }

      if (MSched.ResOvercommitSpecified[0] == TRUE)
        {
        int tmpTC;

        MNodeGetAvailTC(N,J,RQ,TRUE,&tmpTC);

        tmpI = MIN(tmpI,tmpTC);
        }

      MNLSetNodeAtIndex(INL,nindex,N);
      MNLSetTCAtIndex(INL,nindex,tmpI);

      MDB(5,fSCHED) MLog("INFO:     idle node %s x%d located (D: %d)\n",
        N->Name,
        tmpI,
        N->DRes.Procs);
 
      tmpTC += tmpI;

      tmpNC++;

      nindex++;
      }  /* END for (nindex) */

    /* terminate list */

    MNLTerminateAtIndex(INL,nindex);
    }    /* END else (FNL == NULL) */

  if (TaskCount != NULL)
    *TaskCount = tmpTC;

  if (NodeCount != NULL)
    *NodeCount = tmpNC;

  MDB(4,fSCHED) MLog("INFO:     idle resources (%d tasks/%d nodes) found with %sfeasible list specified\n",
    tmpTC,
    tmpNC,
    (FNL == NULL) ? "no " : "");

  return(SUCCESS);
  }  /* END MJobGetINL() */



/**
 * NOTE:  updates N->JList, should only be used with active/suspended jobs
 *        updates available classes and job/proc usage for usage limits
 *
 *
 * @param J (I)
 * @param NL (I) [optional] 
 * @param AdjustGRes (I).  Adjust GRes Values, usually on job start.
 *
 * TRUE = adjust all gres under all circumstances (for when a job starts)
 * FALSE = never adjust gres under any circumstances
 * MBNOTSET = decide on a per gres/node basis whether or not to update gres
 *
 * @see MJobRemoveFromNL() - peer
 * @see MJobStart() - parent
 * @see MClusterUpdateNodeState() - parent
 */

int MJobAddToNL(

  mjob_t     *J,  /* I */
  mnl_t      *NL, /* I (optional) */
  mbool_t     AdjustGRes) /* I */

  {
  int      nindex;
  mnode_t *N;

  int      rqindex;
  mreq_t  *RQ;
  int      FIndex;

  int      jindex;

  int      JCount;
  int      PCount;

  int      tindex;

  mjob_t  *tJ;

  const char *FName = "MJobAddToNL";

  MDB(4,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (NL != NULL) ? "NL" : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (MJOBISALLOC(J) == FALSE)
    {
    /* no affect on resources */

    /* NOTE:  what is affect of suspended jobs? */

    return(SUCCESS);
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];
 
    for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      if (MNLGetTCAtIndex(&RQ->NodeList,nindex) <= 0)
        break;

      FIndex = -1;

      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        { 
        if ((N->JList == NULL) || (N->JList[jindex] == NULL))
          break;

        if ((FIndex == -1) && (N->JList[jindex] == (mjob_t *)1))
          FIndex = jindex;
         
        if (N->JList[jindex] == J) 
          {
          /* job already attached to node */

          if (rqindex == 0)
            N->JTC[jindex] = 0;

          break;
          }
        }    /* END for (jindex) */

      if ((jindex >= N->MaxJCount) && (FIndex == -1))
        {
        /* node joblist is full - dynamically expand node's job array */

        MDB(4,fSCHED) MLog("INFO:     job table extended to size %d on node %s for job %s)\n",
          N->MaxJCount + MMAX_JOB_PER_NODE,
          N->Name,
          J->Name);

        /* extend list */

        N->JList = (mjob_t **)realloc(
          N->JList,
          sizeof(mjob_t *) * (N->MaxJCount + MMAX_JOB_PER_NODE + 1));

        N->JTC   = (int *)realloc(
          N->JTC,
          sizeof(int) * (N->MaxJCount + MMAX_JOB_PER_NODE + 1));

        if ((N->JList == NULL) || (N->JTC == NULL))
          {
          MDB(4,fSCHED) MLog("ERROR:    cannot extend node job table\n");

          N->MaxJCount = 0;

          return(FAILURE);
          }

        /* terminate list */

        N->JList[jindex] = NULL;

        N->MaxJCount += MMAX_JOB_PER_NODE;
        }  /* END if (jindex >= N->MaxJCount) */

      if ((N->JList != NULL) && (N->JList[jindex] == NULL))
        {
        if (FIndex != -1)
          jindex = FIndex;

        N->JList[jindex] = J;
        N->JTC[jindex]   = 0;
 
        N->JList[jindex + 1] = NULL;
        }

      N->JTC[jindex] += MNLGetTCAtIndex(&RQ->NodeList,nindex);

      /* NOTE:  sites may want suspended job to apply against idle policies so as
                to guarantee that a resume is possible */

#ifdef MNOT
      if (J->State == mjsSuspended)
        {
        /* suspended jobs do not count against job or proc policies */

        continue;
        }
#endif /* MNOT */

      /* update number of jobs, proc, and class slots currently dedicated on node */

      JCount = 0;
      PCount = 0;

      if ((N->AP.HLimit[mptMaxJob] > 0) ||
          (N->AP.HLimit[mptMaxProc] > 0))
        {
        /* only update if we actually have a limit */

        for (jindex = 0;jindex < N->MaxJCount;jindex++)
          {
          if ((N->JList == NULL) || (N->JList[jindex] == NULL))
            break;
   
          tJ = N->JList[jindex];
   
          if ((tJ == (mjob_t *)1) || (tJ->TaskMap == NULL))
            continue;
   
          JCount++;
   
          for (tindex = 0;tJ->TaskMap[tindex] != -1;tindex++)      
            {
            if (tindex >= MSched.JobMaxTaskCount)
              {
              MDB(1,fSCHED) MLog("ALERT:    task overflow for job %s (node %s/job %s)\n",
                J->Name,
                N->Name,
                tJ->Name);
   
              break;
              }
   
            if (tJ->TaskMap[tindex] != N->Index)
              continue;
   
            /* FIXME:  must adjust for multi-req jobs */
   
            PCount += tJ->Req[0]->DRes.Procs;          
            }  /* END for (tindex) */
          }    /* END for (jindex) */
   
        N->AP.Usage[mptMaxJob]  = JCount;
        N->AP.Usage[mptMaxProc] = PCount;
        }
 
      if (J->State != mjsSuspended)
        {
        /* adjust GRes */

        if (N == MSched.GN)
          {
          /* do this only if current node is the global node */
          MNodeSetState(N,mnsActive,FALSE);
          }

        if (AdjustGRes == TRUE)
          {
          mreq_t rq;

          memset(&rq,0,sizeof(rq));
          MCResInit(&rq.DRes);
          MSNLCopy(&rq.DRes.GenericRes,&RQ->DRes.GenericRes);

          /* Don't enforce configured resoursec if node is a hypervisor.
              If a hypervisor reports negative available resources (which
              is possible with VMs), zeroing out the available resources
              will make the VM migration code not migrate as much as it
              needs to in order to get the hypervisor under threshold.
              MOAB-3600 */

          MCResRemove(
            &N->ARes,
            &N->CRes,
            &rq.DRes,
            MNLGetTCAtIndex(&RQ->NodeList,nindex),
            (N->HVType == mhvtNONE) ? TRUE : FALSE); 

          MCResFree(&rq.DRes);
          }
        else if (AdjustGRes == MBNOTSET)
          {
          int count;
          int gindex;

          /* we need to check each gres that this job is requesting. for the gres
             that are being managed by flexlm or similar DO NOT update N->ARes,
             for the gres that are not managed by flexlm or similar DO update N->ARes */

          if (MSNLGetIndexCount(&RQ->DRes.GenericRes,0) == 0)
            continue;
       
          if (MNLGetNodeAtIndex(&RQ->NodeList,0,&N) == FAILURE)
            continue;
       
          for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
            {
            if (MUStrIsEmpty(MGRes.Name[gindex]))
              break;

            count = MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex);

            if (count <= 0)
              continue;

            if ((MSched.AResInfoAvail == NULL) ||
                (MSched.AResInfoAvail[N->Index] == NULL) ||
                (MSched.AResInfoAvail[N->Index][gindex] == FALSE))
              {
              /* set available to available - whatever this req is consuming */
              MSNLSetCount(
                &N->ARes.GenericRes,
                gindex,
                MSNLGetIndexCount(&N->ARes.GenericRes,gindex) - count);
              }
            }  /* END for (gindex) */
          }      /* END else */
        }    /* END if (J->State != mjsSuspended) */
      }      /* END for (nindex) */
    }        /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MJobAddToNL() */
   




/**
 * Remove job from J->NList on allocated nodes.
 *
 * @see MJobAddToNL() - peer
 *
 * @param J (I)
 * @param NL (I)
 */

int MJobRemoveFromNL(
 
  mjob_t     *J,  /* I */
  mnl_t      *NL) /* I */
 
  {
  int      nindex;
  int      rqindex;
  int      jindex;

  mnode_t *N;
 
  mreq_t  *RQ;
 
  if (J == NULL)
    {
    return(FAILURE);
    }
 
  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (MNLIsEmpty(&RQ->NodeList))
      continue;
 
    for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      for (jindex = 0;jindex < N->MaxJCount;jindex++)
        {
        if (N->JList[jindex] == NULL)
          break;

        if (N->JList[jindex] != J)
          continue;

        /* set 'empty' marker */

        N->JList[jindex] = (mjob_t *)1;

        N->AP.Usage[mptMaxJob]--;

        N->JTC[jindex] = 0;
        }  /* END for (jindex) */
      }    /* END for (nindex) */
    }      /* END for (rqindex) */
 
  return(SUCCESS);
  }  /* END MJobRemoveFromNL() */
/* END MJobNL.c */
