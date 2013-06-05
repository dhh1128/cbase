
/* HEADER */

      
/**
 * @file: MJobAllocMNL.c
 *
 * Contains:
 *     int MJobAllocMNL
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "PluginNodeAllocate.h"  

/**
 * Apply node allocation policy to allocate subset of selected nodes to job 
 * to meet job node/task requirements.
 *
 * @see MJobAllocatePriority() - child - enforce 'priority' node allocation policy
 * @see MJobAllocMNLWrapper() - parent - handle job arrays
 *
 * @note MJobAllocMNL() will return an exact set of nodes and possibly a 
 *       superset of tasks.
 *
 * @note This routine will only allow a given node to be allocated to a single req
 *
 * @param J         (I) job requesting resources
 * @param SFMNL     (I) specified feasible nodes
 * @param NodeMap   (I)
 * @param AMNL      (O) allocated nodes (optional, if NULL, modify J->Req[X]->NodeList)
 * @param NAPolicy  (I) node allocation policy
 * @param StartTime (I) time job must start
 * @param SC        (O) (optional)
 * @param EMsg      (O) (optional,minsize=MMAX_LINE)
 *
 * @li Step 1.0  Initialize
 * @li Step 2.0  Allocate Required HostList Nodes (optional)
 * @li Step 3.0  Perform initial per-req analysis 
 * @li   Step 3.1  Consider Latency Aspects (optional) 
 * @li   Step 3.2  Determine Effective Node Allocation Policy 
 * @li Step 4.0  Enforce NodeSets 
 * @li   Step 4.1  Evaluate Availability w/in Each Affinity 
 * @li Step 5.0  Validate Allocated Resources 
 * 
 * 
 * 
 */

int MJobAllocMNL(

  mjob_t      *J,
  mnl_t      **SFMNL,
  char        *NodeMap,
  mnl_t      **AMNL,
  enum MNodeAllocPolicyEnum NAPolicy,
  mulong       StartTime,
  int         *SC,
  char        *EMsg)

  {
  int                 index;
  int                 rqindex;
  int                 rnindex;

  mreq_t             *RQ;

  int                 nindex;

  /* NOTE:  ConsumedTC[] only supported w/in mnalLastAvailable */

  static int         *ConsumedTC = NULL;  /* tasks consumed so far by req allocations */
  static mnl_t       *FMNL[MMAX_REQ_PER_JOB];
  static mbool_t      Init = FALSE;

  int                 NodeIndex[MMAX_REQ_PER_JOB];

  int                 FNC[MMAX_REQ_PER_JOB];
  int                 FTC[MMAX_REQ_PER_JOB];

  int                 AllocTC[MMAX_REQ_PER_JOB];
  int                 AllocNC[MMAX_REQ_PER_JOB];

  int                 TotalAllocTC;
  int                 TotalAllocNC;

  int                 TotalFTC;
  int                 TotalFNC;

  mnl_t              *NL;
  mnl_t              *BestList[MMAX_REQ_PER_JOB];
 
  mbool_t             NodeSetIsOptional;

  enum MNodeAllocPolicyEnum effectiveNAPolicy;
  PluginNodeAllocate       *Plugin = NULL;  /* plugin used when [effective]NAPolicy == mnalPlugin */
  
  int                 TC;
  int                 NC;

  int                 Delta;

  int                 MaxTPN[MMAX_REQ_PER_JOB];
  int                 MinTPN[MMAX_REQ_PER_JOB];

  mnode_t            *N;
  mpar_t             *P = NULL;

  int                 ResourceIteration;
  char                AffinityLevel;

  int                 AffinityAvailNC[10];

  mbool_t             AllocComplete;  
  mbool_t             IsMultiReq;
  mbool_t             NLChanged = FALSE;

  enum MResourceSetSelectionEnum tmpRSS;
 
  int                 nsindex;
  int                 SelNSIndex = 0;

  /* NOTE:  once best nodes are selected, nodes should be sorted by tasks */
  /*        available */

  const char *FName = "MJobAllocMNL";

  MDB(4,fSCHED) MLog("%s(%s,SFMNL,NodeMap,%s,%s,%ld,SC,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (AMNL != NULL) ? "AMNL" : "NULL",
    MNAllocPolicy[NAPolicy],  
    StartTime);

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';
    }

  if (SC != NULL)
    {
    *SC = 0;
    }

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (Init == FALSE)
    {
    ConsumedTC = (int *)MUMalloc(sizeof(int) * MSched.M[mxoNode]);

    MNLMultiInit(FMNL);

    Init = TRUE;
    }

  /* Step 1.0  Initialize */

  /* Bluegene systems check with slurm to see if, when, and where jobs elligble. */

  /* Note that Non Bluegene RMs can use Will Run as well (on a cluster but not the grid) */

  if ((MSched.BluegeneRM) || 
      ((J->DestinationRM != NULL) && 
       bmisset(&J->DestinationRM->Flags,mrmfSlurmWillRun) && 
       !bmisset(&J->DestinationRM->Flags,mrmfSlavePeer) &&
       (J->DestinationRM->Type != mrmtMoab)))
    {
    mjob_t *Preemptees[MMAX_JOB];
    mbool_t DoWillRun = TRUE;
    int adjustedStartTime = StartTime;

    Preemptees[0] = NULL;

    MNodeListGetIgnIdleJobRsvJobs(SFMNL[0],Preemptees);

    /* Job already preempted and the nodes passed into MJobAllocNL are the ones
     * that willrun said the job would run on if the preemption occurred. Let
     * the job get a nodelist with the given nodes and if slurm says it can't
     * run right now then the guaranteed preemption will kick in and the job 
     * will hold on to the nodes while moab keeps trying to start the job. */

    if ((MSched.GuaranteedPreemption == TRUE) &&
        (bmisset(&J->Flags,mjfPreempted)))
      DoWillRun = FALSE;

    /* If the job isn't able to start because another job is taking too long 
     * to clean up, give a buffer to the willrun of the remaining reservation 
     * retry time so that the willrun won't fail because slurm returns a time 
     * few seconds futher into the future than we are expecting to handle 
     * clean up. */

    if ((J->RsvRetryTime != 0) &&
        ((MSched.Time - J->RsvRetryTime) < MPar[0].RsvRetryTime))
      adjustedStartTime += MAX(MPar[0].RsvRetryTime - (MSched.Time - J->RsvRetryTime),0);

    if ((DoWillRun == TRUE) && 
        (MJobWillRun(J,SFMNL[0],Preemptees,adjustedStartTime,NULL,EMsg) == FALSE))
      {
      return(FAILURE);
      }
    }

  if (bmisset(&J->Flags,mjfNoResources))
    {
    return(SUCCESS);
    }

#if 0  /* NOT YET WORKING */
  if ((MSched.OptimizedJobArrays == TRUE) && 
      (J->Array != NULL) && 
      !bmisset(&J->Array->Flags,mjafIgnoreArray))
    {
    MJobArrayAllocMNL(
      J,
      SFMNL,
      NodeMap, 
      AMNL,
      NAPolicy,
      StartTime,
      SC,
      EMsg);

    return(SUCCESS);
    }
#endif

  if (bmisset(&J->IFlags,mjifAllocatedNodeSets) && (J->Rsv != NULL))
    {
    /* job's allocation already determined in MJobGetRangeValue() */
    /* populate AMNL[0] or J->Req[0]->NodeList */

    if (AMNL != NULL)
      {
      MNLCopy(AMNL[0],&J->Rsv->NL);

      MNLTerminateAtIndex(AMNL[1],0);
      }
    else
      {
      MNLCopy(&J->Req[0]->NodeList,&J->Rsv->NL);
      }

    return(SUCCESS);
    }   /* END if (bmisset(&J->IFlags,mjifAllocatedNodeSets)) */

  if ((MSched.NodeSetPlus == mnspDelay) &&
      ((MPar[0].NodeSetDelay > 0) || ((J->Req[0] != NULL) && (J->Req[0]->SetDelay > 0))) &&
      (!bmisset(&J->IFlags,mjifNodeSetDelay)))
    {
    if (SC != NULL)
      *SC = (int)marNodeSet;

    return(FAILURE);
    }

  MNLMultiCopy(FMNL,SFMNL);
  memset(NodeIndex,0,sizeof(NodeIndex));
  memset(AllocTC,0,sizeof(AllocTC));
  memset(AllocNC,0,sizeof(AllocNC));
  memset(FTC,0,sizeof(FTC));
  memset(FNC,0,sizeof(FNC));

  IsMultiReq = (J->Req[1] != NULL) ? TRUE : FALSE;

  nindex = 0;

  /* Initialize AMNL */

  if (AMNL != NULL)
    {
    /* NOTE:  why is memset required - why not just clear initial entry */

    MNLMultiClear(AMNL);
    }

  /* Step 2.0  Allocate Required HostList Nodes */

  if (bmisset(&J->IFlags,mjifHostList))
    {
    mnode_t *N1;
    mnode_t *N2;

    if (MNLIsEmpty(&J->ReqHList))
      {
      MDB(1,fSCHED) MLog("ERROR:    hostlist specified but hostlist is NULL/EMPTY for job %s\n",
        J->Name);

      return(FAILURE);
      }

    /* NOTE:  change the following (NYI/FIXME) */

    rqindex = J->HLIndex;

    RQ = J->Req[rqindex]; /* NOTE:  hostlist jobs only have one req */

    MDB(5,fSCHED) MLog("INFO:     using specified hostlist for job %s\n",
      J->Name);

    if (J->ReqHLMode == mhlmSubset)
      {
      int nindex;

      for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N1) == SUCCESS;nindex++)
        {
        if (MNLFind(FMNL[rqindex],N1,NULL) == SUCCESS)
          {
          /* one node from the subset has been found, that's good enough */

          break;
          }
        }

      if (MNLGetNodeAtIndex(&J->ReqHList,nindex,&N1) == FAILURE)
        {
        /* we didn't find any node, return FAILURE */

        return(FAILURE);
        }
      }
    else if (J->ReqHLMode == mhlmExactSet)
      {
      /* If the requested hostlist mode is exactmatch then all of the
       * requested nodes must be found in the feasible node list. */

      mbool_t MissingNodes;

      MDB(5,fSCHED) MLog("INFO:     job %s check to make sure all specified nodes are in the feasible hostlist\n",
        J->Name);

      /* MNLDiff will return FAILURE if one of the nodelists is NULL.
       * It will set MissingNodes to TRUE if a node in J->ReqHList is not found in FMNL[rqindex] */

      if ((MNLDiff(&J->ReqHList,FMNL[rqindex],NULL,&MissingNodes,NULL) != SUCCESS) ||
          (MissingNodes == TRUE))
        {
        MDB(2,fSCHED) MLog("WARNING:  specified node(s) not found in feasible hostlist for job %s:%d\n",
          J->Name,
          RQ->Index);

        return(FAILURE);
        }
      }

    /* check if HostList taskcount exceeds job taskcount */

    TC = 0;
    NC = 0;

    /* loop through each feasible node to determine whether it matches the 
       job's requested hostlist and hostlist-mode */

    for (index = 0;MNLGetNodeAtIndex(FMNL[rqindex],index,&N1) == SUCCESS;index++)
      {
      for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N2) == SUCCESS;nindex++)
        {
        /* If we find a matching entry in the feasible list then 
         * bump our counts and advance to the next requested node. */

        if (N2 == N1)
          {
          /* found a node in both feasible and requested */

          if (J->ReqHLMode == mhlmSubset)
            {
            /* NOTE:  if subset list specified, hostlist does not *
             *        provide task constraint info */

            NodeMap[N1->Index] = mnmRequired;

            TC += MNLGetTCAtIndex(FMNL[rqindex],index);
            NC++;
            }
          else
            {
            if (J->ReqHLMode != mhlmSuperset)
              {
              NodeMap[N1->Index] = mnmRequired;

              MNLSetTCAtIndex(FMNL[rqindex],index,
                MIN(MNLGetTCAtIndex(FMNL[rqindex],index),MNLGetTCAtIndex(&J->ReqHList,nindex)));
              }

            TC += MIN(MNLGetTCAtIndex(FMNL[rqindex],index),MNLGetTCAtIndex(&J->ReqHList,nindex));
            NC++;
            }

          /* We found the requested node in the feasible node list so 
           * break out of the feasible node loop and proceed to the next 
           * requested node */

          break;
          } /* END if (J->ReqHList[nindex].N == FMNL[rqindex][index].N) */

        /* The feasible node is not in the requested host list, so check to see if we have already met
         * our task and node requirements and if so, don't bother to keep iterating
         * through the feasible node list searching for requested nodes */

        if ((RQ->TaskCount > 0) && (TC >= RQ->TaskCount))
          {
          if ((RQ->NodeCount == 0) || (NC >= RQ->NodeCount))
            {
            /* adequate resources found */

            break;
            }
          }
        }  /* END for (nindex) */

      /* If we have met our node and task requirements then we are done and no
       * longer need to keep iterating on the requested node list */

      if ((RQ->TaskCount > 0) && (TC >= RQ->TaskCount))
        {
        if ((RQ->NodeCount == 0) || (NC >= RQ->NodeCount))
          {
          /* adequate resources found */

          break;
          }
        }
      }    /* END for (index) */

    if (J->ReqHLMode == mhlmSubset)
      {
      /* Add in nodes that are not specified in the hostlist if 
          taskcount has not been satisfied */

      mbool_t NodeInHL;

      for (index = 0;MNLGetNodeAtIndex(FMNL[rqindex],index,&N1) == SUCCESS;index++)
        {
        NodeInHL = FALSE;

        if ((RQ->TaskCount > 0) && (TC >= RQ->TaskCount))
          {
          if ((RQ->NodeCount == 0) || (NC >= RQ->NodeCount))
            {
            /* adequate resources found */

            break;
            }
          }

        for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N2) == SUCCESS;nindex++)
          {
          if (N2 == N1)
            {
            NodeInHL = TRUE;

            break;
            }
          }

        if (NodeInHL == FALSE)
          {
          TC += MNLGetTCAtIndex(FMNL[rqindex],index);
          NC++;
          }
        }  /* END for (index = 0;... */
      } /* END if (J->ReqHLMode == mhlmSubmset) */

    MDB(5,fSCHED) MLog("INFO:     job %s Requested TC %d, Hostlist TC %d, Hostlist Mode %d, Best Effort Flag %s\n",
      J->Name,
      RQ->TaskCount,
      TC,
      J->ReqHLMode,
      bmisset(&J->Flags,mjfBestEffort) != 0 ? "TRUE" : "FALSE");

    if (((TC < RQ->TaskCount) && 
         !bmisset(&J->Flags,mjfBestEffort) &&
         (J->ReqHLMode != mhlmSuperset)) || 
         (TC == 0))
      {
      /* inadequate tasks found  */

      MDB(2,fSCHED) MLog("WARNING:  inadequate tasks specified in hostlist for job %s:%d (%d < %d)\n",
        J->Name,
        RQ->Index,
        TC,
        RQ->TaskCount);

      return(FAILURE);
      }

    if (J->ReqHLMode != mhlmSubset)
      {
      /* distribute tasks TPN at a time to hosts firstfit */

      if (!bmisset(&J->Flags,mjfBestEffort))
        {
        TC = RQ->TaskCount;
        }
      else if (bmisset(&J->Flags,mjfRsvMap) && (RQ->TaskCount > 0))
        {
        /* 2-2-10 BOC RT6787 - A reservation with a hostlist and a taskcount
         * of specific resources was getting the full node instead of the
         * requested tasks. TC must be set to RQ->TaskCount or the nodelist's
         * taskcount will include the whole node. Ex: Each node has 16 procs.
         * SRCFG[] HOSTLIST=[1-3] RESOURCES=PROCS:1;MEM:512 TASKCOUNT=4 
         * The mem is fullfilled in MReqGetFNL and the Procs here. */

        TC = RQ->TaskCount;
        }

      rnindex = 0;

      /* add feasible nodes found in specified hostlist */

      for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N1) == SUCCESS;nindex++)
        {
        for (index = 0;MNLGetNodeAtIndex(FMNL[rqindex],index,&N2) == SUCCESS;index++)
          {
          if (N1 == N2)
            {
            break;
            }
          }  /* END for (index) */

        if (N2 == NULL)
          {
          /* node is not feasible */

          continue;
          }

        /* must satisfy both NC and TC (if specified) */

        if ((RQ->NodeCount > 0) && (TC <= 0))
          {
          /* when NC is specified and we've already satisfied TC grab everything on node */

          Delta = MNLGetTCAtIndex(&J->ReqHList,nindex);
          }
        else
          {
          /* Get the taskcount from the feasible node list. (not J->ReqHList) 
             because it is the most reliable source at this time */

          Delta = MIN(MNLGetTCAtIndex(FMNL[rqindex],index),TC);
          }

        if ((RQ->TasksPerNode > 0) && 
            (Delta > RQ->TasksPerNode)) /* if Delta is less than TasksPerNode then this is the tail node - no need to adjust delta */
          {
          Delta -= (Delta % RQ->TasksPerNode);
          }

        if (Delta > 0)
          {
          MNLGetNodeAtIndex(&J->ReqHList,nindex,&N);

          MNLSetNodeAtIndex(&RQ->NodeList,rnindex,N);
          MNLSetTCAtIndex(&RQ->NodeList,rnindex,Delta);
   
          if (AMNL != NULL)
            {
            MNLSetNodeAtIndex(AMNL[rqindex],rnindex,N);
            MNLSetTCAtIndex(AMNL[rqindex],rnindex,Delta);
            }

          MDB(7,fSCHED) MLog("INFO:     hostlist node %sx%d added to job %s\n",
            N->Name,
            Delta,
            J->Name);

          rnindex++;

          TC -= Delta;
          NC--;
          }  /* END if (Delta > 0) */

        if ((TC <= 0) &&
            ((RQ->NodeCount <= 0) || 
             (NC <= 0)))
          break;
        }  /* END for (nindex) */

      if (rnindex == 0)
        {
        MDB(3,fSCHED) MLog("WARNING:  empty hostlist for job %s in %s()\n",
          J->Name,
          FName);

        return(FAILURE);
        }

      MNLTerminateAtIndex(&RQ->NodeList,rnindex);

      if (AMNL != NULL)
        MNLTerminateAtIndex(AMNL[rqindex],rnindex);

      if (TC > 0)
        {
        MDB(3,fSCHED) MLog("WARNING:  cannot allocate tasks specified in hostlist for job %s (%d remain)\n",
          J->Name,
          TC);

        return(FAILURE);
        }

      MDB(3,fSCHED) MLog("INFO:     %d requested hostlist tasks allocated for job %s (%d remain)\n",
        J->Request.TC,
        J->Name,
        TC);

      if (IsMultiReq == FALSE)
        {
        return(SUCCESS);
        }
      }  /* END if (J->ReqHLMode != mhlmSubset) */
    }    /* END if (bmisset(&J->IFlags,mjifHostList)) */

  /* Step 3.0  Perform initial per-req analysis */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (bmisset(&J->IFlags,mjifHostList) && 
       (J->ReqHLMode != mhlmSubset) &&
       (rqindex == J->HLIndex))
      {
      /* ignore hostlist req */

      continue;
      }

    NL = FMNL[rqindex];

    MDB(8,fSCHED)
      {
      char tmpBuf[MMAX_BUFFER];

      MNLToString(NL,TRUE,",",'\0',tmpBuf,sizeof(tmpBuf));

      MLog("INFO:     FMNL[%d](NL): %s\n",
        rqindex,
        tmpBuf);
      }

    /* test first node in list and get the partition from that */

    if (MNLGetNodeAtIndex(NL,0,&N) == SUCCESS)
      {
      P = &MPar[N->PtIndex];
      }
    else
      {
      P = &MPar[0];
      }

    /* update node preferences */

    if (MSched.ResourcePrefIsActive == TRUE)
      {
      mbool_t tmpPref;

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        tmpPref = bmisset(&N->IFlags,mnifIsPref);
   
        MReqGetPref(RQ,N,&tmpPref);
   
        if (tmpPref == TRUE)
          bmset(&N->IFlags,mnifIsPref);
        else
          bmunset(&N->IFlags,mnifIsPref);
        }
      }  

    /* only enforce nodesets for non-peer allocations */
    /* per job NodeSet added 10/25/07 by BC and DRW */
    /* determine if NodeSets are optional */

    NodeSetIsOptional = TRUE;

    if (RQ->SetBlock != MBNOTSET)
      {
      /* job is overriding the system setting */

      NodeSetIsOptional = !RQ->SetBlock;
      }
    else if (MPar[0].NodeSetIsOptional == FALSE)
      {
      /* nothing on the job, use the system setting */

      NodeSetIsOptional = FALSE;
      }

    if ((NodeSetIsOptional == FALSE) &&
        (MPar[0].NodeSetPolicy == mrssNONE) &&
        (RQ->SetSelection == mrssNONE))
      {
      /* if the system does not have a nodeset and the job does not have a nodeset
         then there is no nodeset */

      NodeSetIsOptional = TRUE;
      }

    if (NodeSetIsOptional == FALSE)
      {
      /* mandatory node set */

      tmpRSS = (RQ->SetSelection != mrssNONE) ? 
        RQ->SetSelection : 
        MPar[0].NodeSetPolicy;

      /* only enforce node sets for compute host based reqs */

      if ((RQ->NestedNodeSet != NULL) ||
          (MSched.NestedNodeSet != NULL))
        {
        int    rc = FAILURE;
        int    index;
     
        char   tmpName[MMAX_NAME];
     
        mln_t *NodeSets = (RQ->NestedNodeSet != NULL) ? RQ->NestedNodeSet : MSched.NestedNodeSet;
        mln_t *tmpL;
     
        mnl_t  tmpNodeList;
     
        char  **List;
     
        snprintf(tmpName,sizeof(tmpName),"%d",0);
     
        if (MULLCheck(RQ->NestedNodeSet,tmpName,&tmpL) == FAILURE)
          {
          /* invalid NestedNodeSet specified */
     
          MMBAdd(&J->MessageBuffer,"invalid NestedNodeSet specified",NULL,mmbtNONE,0,0,NULL);

          return(FAILURE);
          }
     
        List = (char **)tmpL->Ptr;
     
        MNLInit(&tmpNodeList);

        for (index = 0;List[index] != NULL;index++)
          {
          /* need to copy NodeList, SelSetIndex */
     
          MNLCopy(&tmpNodeList,NL);
     
          if (MJobSelectResourceSet(
                J,
                RQ,
                StartTime,
                (RQ->SetType != mrstNONE) ? RQ->SetType : MPar[0].NodeSetAttribute,
                (tmpRSS != mrssOneOf) ? tmpRSS : mrssFirstOf,
                (char **)tmpL->Ptr,
                NL,
                NodeMap,
                0,
                List[index],
                &SelNSIndex,
                NodeSets) == FAILURE)
            {
            /* copy back */
     
            MNLCopy(NL,&tmpNodeList);

            rc = FAILURE;

            continue;
            }
          else
            {
            rc = SUCCESS;

            break;
            }
          }   /* END for (index) */

        if (rc == FAILURE)
          {
          MDB(4,fSCHED) MLog("INFO:     cannot alloc resource set for job %s:%d\n",
            J->Name,
            RQ->Index);
   
          MMBAdd(&J->MessageBuffer,"job cannot alloc nodeset",NULL,mmbtNONE,0,0,NULL);
   
          if (EMsg != NULL)
            strcpy(EMsg,"cannot alloc nodeset");
   
          if (SC != NULL)
            *SC = (int)marNodeSet;
   
          MNLFree(&tmpNodeList);
          return(FAILURE);
          }  /* END if (rc == FAILURE) */

        MNLFree(&tmpNodeList);
        }
      else if (MJobSelectResourceSet(
           J,
           RQ,
           StartTime,
           (RQ->SetType != mrstNONE) ? RQ->SetType : MPar[0].NodeSetAttribute,
           (tmpRSS != mrssOneOf) ? tmpRSS : mrssFirstOf,
           ((RQ->SetList != NULL) && (RQ->SetList[0] != NULL)) ? RQ->SetList : MPar[0].NodeSetList,
           NL,                     /* I */
           NodeMap,
           -1,
           NULL,
           &SelNSIndex,
           NULL) == FAILURE)
        {
        /* cannot locate needed set based resources */

        if (MSched.NodeSetPlus != mnspSpanEvenly)
          {
          MDB(4,fSCHED) MLog("INFO:     cannot alloc resource set for job %s:%d\n",
            J->Name,
            RQ->Index);

          MMBAdd(&J->MessageBuffer,"job cannot alloc nodeset",NULL,mmbtNONE,0,0,NULL);

          if (EMsg != NULL)
            strcpy(EMsg,"cannot alloc nodeset");

          if (SC != NULL)
            *SC = (int)marNodeSet;

          return(FAILURE);
          }               
        }    /* END else if (MJobSelectResourceSet()) */

      /* preserve the nodesetpolicy and nodesetattribute for diagnostics */

      RQ->SetType = (RQ->SetType != mrstNONE) ? RQ->SetType : MPar[0].NodeSetAttribute;

      RQ->SetSelection = (RQ->SetSelection != mrssNONE) ? 
        RQ->SetSelection : 
        MPar[0].NodeSetPolicy;

      MDB(8,fSCHED)
        {
        char tmpBuf[MMAX_BUFFER];

        MNLToString(NL,TRUE,",",'\0',tmpBuf,sizeof(tmpBuf));

        MLog("INFO:     NL after set constraints applied: %s\n",
          tmpBuf);
        }
      }    /* END if (NodeSetIsOptional == FALSE) */

    MinTPN[rqindex] = 1;
    MaxTPN[rqindex] = RQ->TaskCount;

    if (RQ->TasksPerNode > 1)
      {
      MinTPN[rqindex] = RQ->TasksPerNode;
      }

    if ((RQ->TasksPerNode >= 1) && (bmisset(&J->IFlags,mjifExactTPN)))
      {
      MaxTPN[rqindex] = RQ->TasksPerNode;
      }

    if (RQ->NodeCount > 0)
      {
      if (MRM[RQ->RMIndex].Type == mrmtLL)
        {
        MinTPN[rqindex] = MAX(MinTPN[rqindex],RQ->TaskCount / RQ->NodeCount);
        MaxTPN[rqindex] = MinTPN[rqindex];

        if (RQ->TaskCount % RQ->NodeCount)
          MaxTPN[rqindex]++;
        }
      }

    if (AMNL == NULL)
      BestList[rqindex] = &RQ->NodeList;
    else
      BestList[rqindex] = AMNL[rqindex];

    NC = 0;

    /* direct access for speed */

    for (index = 0;NL->Array[index].N != NULL;index++)
      {
      N = NL->Array[index].N;

      FTC[rqindex] += NL->Array[index].TC;

      NC++;
      }  /* END for (index) */

    FNC[rqindex] = (long)NC;

    if (FTC[rqindex] < RQ->TaskCount)
      {
      MDB(0,fSCHED) MLog("ERROR:    invalid nodelist for job %s:%d (inadequate taskcount, %d < %d)\n",
        J->Name,
        rqindex,
        FTC[rqindex],
        RQ->TaskCount);

      return(FAILURE);
      }

    if ((RQ->NodeCount > 0) &&
        (FNC[rqindex] < RQ->NodeCount))
      {
      MDB(0,fSCHED) MLog("ERROR:    invalid nodelist for job %s:%d (inadequate nodecount, %d < %d)\n",
        J->Name,
        rqindex,
        FNC[rqindex],
        RQ->NodeCount);

      return(FAILURE);
      }
    }  /* END for (rqindex) */

  RQ = J->Req[0];

#if 0 /* deprecated */
  /* Step 3.1  Consider Latency Aspects */

  if ((RQ->XRRes != NULL) && (RQ->XRRes->Latency > 0.0))
    {
    /* proximity scheduling requested */

    int hindex;
 
    int tmpHopCount[MMAX_NETHOPGROUP];

    for (hindex = 1;hindex < MMAX_NETHOP;hindex++)
      {
      if (P->MaxTaskAtHop[hindex] == -1)
        {
        /* end of list reached */

        break;
        }

      if (J->Request.TC > P->MaxTaskAtHop[hindex])
        {
        /* no group at current hop level is adequately large */

        continue;
        }

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
          {
          N = FMNL[rqindex][nindex].N;

          if (N == NULL)
            break;

          }  /* END for (nindex) */

        if (nindex == 0)
          break;
        }  /* END for (rqindex) */

      /* collect available resources into groups at current hop level */

      /* NYI */

      /* policy should select bestfit, worstfit, or random selection of hop group
         with adequate resources */

      /* NOTE:  for now, use first fit */

      /* constrain resources to selected hop group */
      }
    }    /* END if ((J->XLimit != NULL) && (J->XLimit->Latency > 0.0)) */
#endif

  /* Step 3.2  Determine Effective Node Allocation Policy */

  effectiveNAPolicy = MJobAdjustNAPolicy(NAPolicy,J,P,StartTime,&Plugin);

  MDB(6,fSCHED)
    {
    memset(AffinityAvailNC,0,sizeof(AffinityAvailNC));

    for (index = 0;index < MSched.M[mxoNode];index++)
      {
      if (MNode[index] == NULL)
        break;

      AffinityAvailNC[(int)NodeMap[index]]++;
      }    /* END for (index) */

    for (index = 0;index <= 6;index++)
      {
      MLog("INFO:     affinity level %d nodes: %d\n",
        index,
        AffinityAvailNC[index]);
      }  /* END for (index) */

    for (index = 0;MNLGetNodeAtIndex(FMNL[0],index,&N) == SUCCESS;index++)
      {
      MLog("INFO:     nodelist[%d] %s  %d  %d\n",
        index,
        N->Name,
        MNLGetTCAtIndex(FMNL[0],index),
        NodeMap[N->Index]);
      }
    }    /* END MDB(6,fSCHED) */

  /* Step 4.0  Enforce NodeSets */

  /* NOTE:  NodeMap[] initialized at this.  NodeMap[]/ConsumedTC[] updated as 
            jobs are allocated */

  memset(ConsumedTC,0,sizeof(int) * MSched.M[mxoNode]);

  for (nsindex = 0;nsindex < 2;nsindex++)
    {
    /* PASS 0:  attempt node set allocation */
    /* PASS 1:  attempt any set allocation  */

    memset(NodeIndex,0,sizeof(NodeIndex));
    memset(AllocTC,0,sizeof(AllocTC));
    memset(AllocNC,0,sizeof(AllocNC));

    /* check various nodesets */

    if (nsindex >= 1)
      MNLMultiCopy(FMNL,SFMNL);

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (bmisset(&J->IFlags,mjifHostList) &&
         (rqindex == J->HLIndex) &&
         (J->ReqHLMode != mhlmSubset))
        {
        /* ignore hostlist req */

        continue;
        }

      if (AMNL == NULL)
        MNLClear(&RQ->NodeList);
 
      tmpRSS = (RQ->SetSelection != mrssNONE) ? 
        RQ->SetSelection : 
        MPar[0].NodeSetPolicy;

      if (((tmpRSS == mrssNONE) && (nsindex == 0)) || 
          (MPar[0].NodeSetIsOptional == FALSE) ||
          (RQ->SetBlock == FALSE))
        {
        /* if node set delay > 0 (no best effort) nodelist modification *
         * handled in pre-test                                          */

        MDB(7,fSCHED) MLog("INFO:     ignoring pass 1 for job %s:%d (node set forced in feasible list)\n",
          J->Name,
          rqindex);

        continue;
        }

      if ((tmpRSS != mrssNONE) && (nsindex == 1)) 
        {
        /* ns iteration 1 only for use if iteration 0 failed and *
         * NodeSetIsOptional == TRUE (ie, best effort allowed)   */

        MDB(7,fSCHED) MLog("INFO:     ignoring pass 2 for job %s (node sets enabled)\n",
          J->Name);
 
        continue;
        }
 
      NL = FMNL[rqindex];

      nindex = MNLCount(FMNL[rqindex]);

      if ((nsindex == 0) && (MJobSelectResourceSet(
           J,
           RQ,
           StartTime,
           (RQ->SetType != mrstNONE) ? RQ->SetType : MPar[0].NodeSetAttribute,
           (tmpRSS != mrssOneOf) ? tmpRSS : mrssFirstOf,
           ((RQ->SetList != NULL) && (RQ->SetList[0] != NULL)) ? RQ->SetList : MPar[0].NodeSetList,     
           NL,
           NodeMap,
           -1,
           NULL,
           &SelNSIndex,
           NULL) == FAILURE))
        {
        /* cannot locate needed set based resources */

        MDB(4,fSCHED) MLog("INFO:     cannot locate adequate resource set for job %s:%d\n",
          J->Name,
          RQ->Index);
 
        continue;
        }

      NLChanged = TRUE;   
      }    /* END for (rqindex) */

    /* Step 4.1  Evaluate Availability w/in Each Affinity */

    for (ResourceIteration = 0;ResourceIteration <= 5;ResourceIteration++)
      {
      /* NOTE:  apply job preferences 'within' affinity values */

      /* NOTE:

         iteration 0:  Required
         iteration 1:  PositiveAffinity
         iteration 2:  NeutralAffinity 
         iteration 3:  None
         iteration 4:  NegativeAffinity
         iteration 5:  Preemptible
      */

      switch (ResourceIteration)
        {
        case 0:

          AffinityLevel = (char)mnmRequired;

          break;

        case 1:

          AffinityLevel = (char)mnmPositiveAffinity;

          break;

        case 2:

          AffinityLevel = (char)mnmNeutralAffinity;

          break;

        case 3:
        default:

          AffinityLevel = (char)mnmNONE;

          break;

        case 4:

          AffinityLevel = (char)mnmNegativeAffinity;

          break;

        case 5:

          AffinityLevel = (char)mnmPreemptible;

          break;
        }  /* END switch (ResourceIteration) */

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        if (bmisset(&J->IFlags,mjifHostList) &&
           (rqindex == J->HLIndex) &&
           (J->ReqHLMode != mhlmSubset))
          {
          /* ignore hostlist req */

          continue;
          }

        MDB(5,fSCHED) MLog("INFO:     evaluating nodes on alloc iteration %d for job %s:%d with NAPolicy '%s'\n",
          ResourceIteration,
          J->Name,
          rqindex,
          MNAllocPolicy[effectiveNAPolicy]);

        RQ = J->Req[rqindex];

        NL = FMNL[rqindex];

        if (NLChanged == TRUE)
          {
          /* NL may have changed from nodesets, update counts */

          FTC[rqindex] = 0;

          FNC[rqindex] = 0;

          for (index = 0;NL->Array[index].N != NULL;index++)
            {
            FNC[rqindex]++;
            FTC[rqindex] += NL->Array[index].TC;
            }
          } 

        if (AllocTC[rqindex] >= RQ->TaskCount)
          {
          /* all required procs found */

          if ((RQ->NodeCount == 0) ||
              (AllocNC[rqindex] >= RQ->NodeCount))
            {
            /* req is satisfied */

            continue;
            }
          }

        switch (effectiveNAPolicy)
          {
          case mnalLocal:

            MLocalJobAllocateResources(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC,
              StartTime);

            break;

          case mnalPlugin:

            /* only call on first req--all reqs are handled with one call. */
            if (rqindex > 0)
              {
              ResourceIteration = 6; /* don't do affinity loop */
              break;
              }

            Plugin->Run(
              J,
              FMNL,  //* all nodes (for all reqs)
              MinTPN,
              MaxTPN,
              NodeMap,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC);

            break;

          case mnalContiguous:
 
            MJobAllocateContiguous(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC);
 
            break;

          case mnalMaxBalance:
          case mnalProcessorSpeedBalance:

            MJobAllocateBalanced(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC);

            break;

          case mnalFastest:
          case mnalNodeSpeed:

            MJobAllocateFastest(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC);
 
            break;

          case mnalMachinePrio:
          case mnalCustomPriority:

            MJobAllocatePriority(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC,
              StartTime);
 
            break;

          case mnalCPULoad:
          case mnalProcessorLoad:

            MJobAllocateCPULoad(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC);

            break;

          case mnalLastAvailable:
          case mnalInReverseReportedOrder:

            MJobAllocateLastAvailable(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC,
              ConsumedTC);

            break;

          case mnalFirstAvailable:
          case mnalInReportedOrder:

            MJobAllocateFirstAvailable(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC);

            break;

          case mnalMinLocal:
  
            /* select 'RQ->TaskCount' nodes with minimal near-term idle queue requests */

            /* NYI */

            break;

          case mnalMinGlobal:
  
            /* select 'RQ->TaskCount' nodes with minimal long-term idle queue requests */

            /* NYI */

            break;

          case mnalMinResource:
          case mnalMinimumConfiguredResources:

            MJobAllocateMinResource(
              J,
              RQ,
              NL,
              rqindex,
              MinTPN,
              MaxTPN,
              NodeMap,
              AffinityLevel,
              NodeIndex,
              BestList,
              AllocTC,
              AllocNC);

            break;

          default:

            MDB(1,fSCHED) MLog("ERROR:    invalid allocation policy (%d) in %s()\n",
              effectiveNAPolicy,
              FName);

            return(FAILURE);

            /*NOTREACHED*/

            break; 
          }  /* END switch (effectiveNAPolicy)     */

        /* If the resource manager is concerned about the order in which the list
         * of nodes is presented then MNLSort could be called here. MNLSort is
         * being called in MJobCreateAllocPartition to do just this thing for
         * cray jobs. */

        }    /* END for (rqindex)            */
      }      /* END for (ResourceIteration)  */

    AllocComplete = FALSE;

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];
 
      if (AllocTC[rqindex] >= RQ->TaskCount)
        {
        /* all required procs found */
 
        if ((RQ->NodeCount == 0) ||
            (AllocNC[rqindex] >= RQ->NodeCount))
          {
          AllocComplete = TRUE;

          break;
          }
        }
      else
        {
        /* unable to locate adequate tasks */
/*
        fprintf(stderr,"NOTE:  cannot locate all required tasks for job %s in nodeset iteration %d (TC: %d)\n",
          J->Name,
          nsindex,
          FTC[rqindex]);
*/
        }
      }      /* END for (rqindex) */

    if (AllocComplete == TRUE)
      break;
    }      /* END for (nsindex) */

  /* Step 5.0  Validate Allocated Resources */

  TotalAllocNC = 0;
  TotalFNC     = 0;

  TotalAllocTC = 0;
  TotalFTC     = 0;
 
  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {  
    RQ = J->Req[rqindex];

    if (bmisset(&J->IFlags,mjifHostList) && 
       (rqindex == J->HLIndex) &&
       (J->ReqHLMode != mhlmSubset))
      {
      /* ignore hostlist req */

      continue;
      }

    if (AllocTC[rqindex] < RQ->TaskCount)
      {
      MDB(2,fSCHED) MLog("ALERT:    inadequate tasks to allocate to job %s:%d (%d < %d)\n",
        J->Name,
        rqindex,
        AllocTC[rqindex],
        RQ->TaskCount);

      return(FAILURE);
      }

    if ((RQ->NodeCount > 0) && 
        (AllocNC[rqindex] < RQ->NodeCount))
      {
      MDB(2,fSCHED) MLog("ALERT:    inadequate nodes found for job %s:%d (%d < %d)\n",
        J->Name,
        rqindex,
        AllocNC[rqindex],
        RQ->NodeCount);

      return(FAILURE);
      }

#ifdef MNOT
    /* BOC 9/3/10 RT7585 - This check was added to help ensure that the job
     * getting the correct allocations but it led to false positives. The
     * false positives occur when the nodeboards on the smp have non-uniform
     * memory. For example if the job requested 1065600 memory and if the node
     * being used in MJobGetNBCount has 11832 configured memory then
     * MJobGetNBCount will return 91. But if the all of the other nodes in the
     * smp have 11856 configured memory then the job could get fewer nodes than
     * 91 which it did in the ticket. MJobGetNBCount with a node of 11856
     * returns a count of 90. MJobGetNBCount is only used to help determine
     * which smp moab should schedule the job on even though allocations may
     * be different due to non-uniform memory. */

    if (MSched.SharedMem == TRUE)
      {
      /* check if enough nodes were allocated to meet job wide reqs */

      int JobWideProcs = 0;

      JobWideProcs = MJobGetNBCount(J,BestList[rqindex][0].N);

      if ((JobWideProcs == 0) || (AllocNC[rqindex] < JobWideProcs))
        {
        MDB(2,fSCHED) MLog("ALERT:    inadequate node boards located for job %s:%d  (%d < %d)\n",
          J->Name,
          rqindex,
          AllocNC[rqindex],
          JobWideProcs);

        return(FAILURE);
        }
      } /* END if (MSched.SharedMem == TRUE) */
#endif

    TotalAllocNC += AllocNC[rqindex];
    TotalFNC     += FNC[rqindex];

    TotalAllocTC += AllocTC[rqindex];
    TotalFTC     += FTC[rqindex];
    }    /* END for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++) */

  MDB(2,fSCHED) MLog("INFO:     tasks located for job %s:  %d of %d required (%d feasible)\n",
    J->Name,
    TotalAllocTC,
    J->Request.TC,
    TotalFTC);

  /* set allocated tasks on job */
  J->Alloc.TC = TotalAllocTC;

  MDB(2,fSCHED) MLog("INFO:     nodes located for job %s:  %d of %d required (%d feasible)\n",
    J->Name,
    TotalAllocNC,
    J->Request.NC,
    TotalFNC);

  /* NOTE:  if input and output buffers are the same, copy AllocatedNode */
  /*        list to output buffer */

  if (AMNL != NULL)
    {
    /* this is wrong.  Must determine proper copying */

    /* memcpy(AMNL,MBestList,sizeof(AMNL)); */
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (AMNL == NULL)
      BestList[rqindex] = &RQ->NodeList;
    else
      BestList[rqindex] = AMNL[rqindex];

    if (MSched.EnableHighThroughput == TRUE)
      {
      for (index = 0;MNLGetNodeAtIndex(BestList[rqindex],index,&N) == SUCCESS;index++)
        {
        MDB(4,fSCHED) MLog("INFO:     allocated node %sx%d to %s:%d\n",
          N->Name,
          MNLGetTCAtIndex(BestList[rqindex],index),
          J->Name,
          rqindex);

        MNLRemoveNode(SFMNL[rqindex],N,MNLGetTCAtIndex(BestList[rqindex],index));
        }
      }  /* END for (index)   */
    }    /* END for (rqindex) */

  if ((AMNL != NULL) && (MNLIsEmpty(AMNL[0])))
    {
    /* nothing allocated */

    return(FAILURE);
    }

  if (SelNSIndex >= 0)
    {
    J->Req[0]->SetIndex = SelNSIndex;

    /* NOTE:  for class-based nodesets, job RM class modified w/in MJobStart() */
    }

  return(SUCCESS);
  }  /* END MJobAllocMNL() */




/**
 * Adjust the NodeAllocationPolicy (if needed) to conform to special cases.
 * Also fills in the plugin pointer when the NAPolicy ends up being mnalPlugin
 *
 * @see MJobAllocMNL() - parent - handle job arrays
 *
 * @param SpecNAPolicy  (I) Specified (pre-adjustment) NodeAllocationPolicy
 * @param Job           (I) job requesting resources
 * @param Par           (I) partition we are scheduling the job on.
 * @param StartTime     (I) time job must start
 * @param Plugin        (O) The plugin associated with the the policy (if mnalPlugin)
 * 
 */

enum MNodeAllocPolicyEnum MJobAdjustNAPolicy(

  enum MNodeAllocPolicyEnum SpecNAPolicy,
  mjob_t              *Job,
  mpar_t              *Par,
  mulong               StartTime,
  PluginNodeAllocate **Plugin)

  {
  enum MNodeAllocPolicyEnum effectiveNAPolicy;

  if (Job->NodePriority != NULL)
    {
    /* if a priority function has been set on the job, use that first */

    effectiveNAPolicy = mnalMachinePrio;
    }
  else if (bmisset(&Job->Flags,mjfRsvMap))
    {
    /* reservations use MinResource if RSVNODEALLOCATIONPOLICY is not set */

    if (Par->RsvNAllocPolicy != mnalNONE)
      effectiveNAPolicy = Par->RsvNAllocPolicy;
    else if (MPar[0].RsvNAllocPolicy != mnalNONE)
      effectiveNAPolicy = MPar[0].RsvNAllocPolicy;
    else
      effectiveNAPolicy = mnalMinResource;
    }
  else if ((StartTime > MSched.Time) && ((SpecNAPolicy == mnalCPULoad) || (SpecNAPolicy == mnalProcessorLoad)))
    {
    effectiveNAPolicy = mnalMinResource;
    }
  else if (SpecNAPolicy != mnalNONE)
    {
    effectiveNAPolicy = SpecNAPolicy;
    }
  else if (MPar[0].NAllocPolicy != mnalNONE)
    {
    effectiveNAPolicy = MPar[0].NAllocPolicy;
    }
  else
    {
    effectiveNAPolicy = MDEF_NALLOCPOLICY;
    }

  /* set up Plugin if needed */
  if (effectiveNAPolicy == mnalPlugin)
    {
    if (Plugin == NULL)
      {
      /* caller hasn't provided a way for us to get the information out--revert to default */
      effectiveNAPolicy = MDEF_NALLOCPOLICY;
      }
    else if (Par->NodeAllocationPlugin != NULL)
      {
      /* use the plugin from this partition */
      *Plugin = Par->NodeAllocationPlugin;
      }
    else if (MPar[0].NodeAllocationPlugin != NULL)
      {
      /* use the plugin from the default partition */
      *Plugin = MPar[0].NodeAllocationPlugin;
      }
    else
      {
      /* no plugin available even though the type is mnalPlugin--revert to default */
      effectiveNAPolicy = MDEF_NALLOCPOLICY;
      *Plugin = NULL;
      }
    }
  if (SpecNAPolicy != effectiveNAPolicy)
    {
    MDB(4,fSCHED) MLog("Node allocation policy has been adjusted from %s to %s\n",
      MNAllocPolicy[SpecNAPolicy],  
      MNAllocPolicy[effectiveNAPolicy]);
    }

  return (effectiveNAPolicy);
  }  /* END MJobAdjustNAPolicy() */


/**
 * Allocate nodes using the CPULoad algorithm (mnalCPULoad)
 *   allocate job to nodes with maximum idle processors, ie 
 *     (N->CRes.Procs - N->Load)
 *  
 *
 * @param J (I) job allocating nodes
 * @param RQ (I) req allocating nodes
 * @param NL (I) eligible nodes
 * @param RQIndex (I) index of job req to evaluate
 * @param MinTPN (I) min tasks per node allowed
 * @param MaxTPN (I) max tasks per node allowed
 * @param NodeMap (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex (I/O) index of next node to find in BestList
 * @param BestList (O) list of selected nodes (Best fit according to this algorithm)
 * @param TaskCount (I/O) [optional]
 * @param NodeCount (I/O) [optional]
 */

int MJobAllocateCPULoad(

  mjob_t *J,
  mreq_t *RQ,
  mnl_t  *NL,
  int     RQIndex,
  int    *MinTPN,
  int    *MaxTPN,
  char   *NodeMap,
  int     AffinityLevel,
  int    *NodeIndex,
  mnl_t **BestList,
  int    *TaskCount,
  int    *NodeCount)

  {
  int                 nindex;
  int                 FNC;  /* count of nodes in the NodeList */
  mnode_t            *N;
  int                 NTasks;
  double              MaxCPUAvail;

  FNC = 0;
  for (nindex = 0;NL->Array[nindex].N != NULL;nindex++)
    {
    FNC++;
    }

  while (1)
    {
    MaxCPUAvail = 0.0;

    /* determine best fit */
  
    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,NULL) == SUCCESS;nindex++)
      {
      if (MNLGetNodeAtIndex(NL,FNC - nindex - 1,&N) == FAILURE)
        break;    

      if (N == NULL)
        continue;

      if (NodeMap[N->Index] != AffinityLevel)
        {
        /* node unavailable */
 
        continue;
        }

      if ((N->CRes.Procs == 0) && (MSched.SharedPtIndex == N->PtIndex))
        {
        /* no way to evaluate global node based on CPULoad so any shared node will work */

        MaxCPUAvail = 1;

        continue;
        }
      else if ((N->CRes.Procs - MAX(N->DRes.Procs,N->Load)) <= MaxCPUAvail)
        {
        /* better CPU availability already located */

        continue;
        }

      MaxCPUAvail = (double)N->CRes.Procs - MAX((double)N->DRes.Procs,N->Load);
      }  /* END for (nindex) */

    if (MaxCPUAvail <= 0.0)
      {
      /* no additional nodes found */

      break;
      }

    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,NULL) == SUCCESS;nindex++)
      {
      MNLGetNodeAtIndex(NL,FNC - nindex - 1,&N);

      if (N == NULL)
        continue;

      if (NodeMap[N->Index] != AffinityLevel)
        {
        /* node unavailable */
 
        continue;
        }

      if ((N->CRes.Procs == 0) && (MSched.SharedPtIndex == N->PtIndex))
        {
        /* no way to evaluate global node based on CPULoad so any shared node will work */

        /* NO-OP */
        }
      else if (((double)N->CRes.Procs - MAX((double)N->DRes.Procs,N->Load)) < MaxCPUAvail - 0.1)
        {
        /* better CPU availability already located */

        continue;
        }
   
      NTasks = MNLGetTCAtIndex(NL,FNC - nindex - 1);

      if (NTasks < MinTPN[RQIndex])
        continue;

      NTasks = MIN(NTasks,MaxTPN[RQIndex]);

      MNLSetNodeAtIndex(BestList[RQIndex],NodeIndex[RQIndex],N);
      MNLSetTCAtIndex(BestList[RQIndex],NodeIndex[RQIndex],NTasks);

      NodeIndex[RQIndex] ++;
      TaskCount[RQIndex] += NTasks;
      NodeCount[RQIndex] ++;

      /* mark node as used */

      if (RQ->DRes.Procs != 0)
        {
        /* only mark node as unavailable for compute node based reqs */

        NodeMap[N->Index] = mnmUnavailable;
        }

      /* NOTE:  must incorporate TaskRequestList information */
  
      if (TaskCount[RQIndex] >= RQ->TaskCount)
        {
        /* all required procs found */

        /* NOTE:  HANDLED BY DIST */

        if ((RQ->NodeCount == 0) ||
            (NodeCount[RQIndex] >= RQ->NodeCount))
          {
          /* terminate BestList */

          MNLTerminateAtIndex(BestList[RQIndex],NodeIndex[RQIndex]);

          break;
          }
        }
      }     /* END for (nindex) */

    if (MNLGetNodeAtIndex(NL,nindex,NULL) == FAILURE)
      {
      /* no node could be allocated */
 
      break;
      }

    if (TaskCount[RQIndex] >= RQ->TaskCount)
      {
      /* all required procs found */

      if ((RQ->NodeCount == 0) ||
          (NodeCount[RQIndex] >= RQ->NodeCount))
        {
        break;
        }
      }
    }       /* END while (1) */

  return(SUCCESS);

  }  /* END MJobAllocateCPULoad() */



/**
 * Allocate nodes using the Last Available algorithm (mnalLastAvailable)
 * nodes will be in First-to-Last available order because of
 * MJobGetEStartTime()
 *
 * @param J (I) job allocating nodes
 * @param RQ (I) req allocating nodes
 * @param NL (I) eligible nodes
 * @param RQIndex (I) index of job req to evaluate
 * @param MinTPN (I) min tasks per node allowed
 * @param MaxTPN (I) max tasks per node allowed
 * @param NodeMap (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex (I/O) index of next node to find in BestList
 * @param BestList (O) list of selected nodes (Best fit according to this algorithm)
 * @param TaskCount (I/O) [optional]
 * @param NodeCount (I/O) [optional]
 * @param ConsumedTC (I/O) 
 */

int MJobAllocateLastAvailable(

  mjob_t *J,
  mreq_t *RQ,
  mnl_t  *NL,
  int     RQIndex,
  int    *MinTPN,
  int    *MaxTPN,
  char   *NodeMap,
  int     AffinityLevel,
  int    *NodeIndex,
  mnl_t **BestList,
  int    *TaskCount,
  int    *NodeCount,
  int    *ConsumedTC)

  {
  int                 nindex;
  int                 FNC;  /* count of nodes in the NodeList */
  mnode_t            *N;
  int                 NTasks;

  FNC = 0;
  for (nindex = 0;NL->Array[nindex].N != NULL;nindex++)
    {
    FNC++;
    }

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,NULL) == SUCCESS;nindex++)
    {
    int TC;

    if (NodeIndex[RQIndex] >= MSched.JobMaxNodeCount)
      {
      MDB(5,fSCHED) MLog("ALERT:    job '%s' requests too many nodes (> %d)  check %s\n",
        J->Name,
        MSched.JobMaxNodeCount,
        "JOBMAXNODECOUNT");

      MMBAdd(
        &MSched.MB,
        "ALERT:    too many nodes requested (increase JOBMAXNODECOUNT)",
        NULL,
        mmbtNONE,
        0,
        0,
        NULL);

      break;
      }

    MNLGetNodeAtIndex(NL,FNC - nindex - 1,&N);

    if (NodeMap[N->Index] != AffinityLevel)
      {
      /* node unavailable */

      continue;
      }

    NTasks = MNLGetTCAtIndex(NL,FNC - nindex - 1);

    TC = (RQ->TaskCount > 0) ? MIN(NTasks,RQ->TaskCount) : NTasks;

    /* NOTE:  assume task definition is similar across reqs
              (scary assumption - FIXME) */

    TC -= ConsumedTC[N->Index];

    if (TC < MinTPN[RQIndex])
      continue;

    TC = MIN(TC,MaxTPN[RQIndex]);

    MNLSetNodeAtIndex(BestList[RQIndex],NodeIndex[RQIndex],N);
    MNLSetTCAtIndex(BestList[RQIndex],NodeIndex[RQIndex],TC);

    NodeIndex[RQIndex] ++;
    TaskCount[RQIndex] += TC;

    NodeCount[RQIndex] ++;

    /* mark node as used */

    MNLAddTCAtIndex(NL,FNC - nindex - 1,-1 * TC);

    if (MNLGetTCAtIndex(NL,FNC - nindex - 1) <= 0)
      { 
      if (RQ->DRes.Procs != 0)
        {
        /* only mark node as unavailable for compute node based reqs */

        if (TC >= NTasks)
          NodeMap[N->Index] = mnmUnavailable;
        else
          ConsumedTC[N->Index] += TC;
        }
      }

    /* NOTE:  must incorporate TaskRequestList information */

    if (TaskCount[RQIndex] >= RQ->TaskCount)
      {
      /* all required procs found */

      /* NOTE:  HANDLED BY DIST */

      if ((RQ->NodeCount == 0) || 
          (NodeCount[RQIndex] >= RQ->NodeCount))
        {
        /* terminate BestList */
  
        MNLTerminateAtIndex(BestList[RQIndex],NodeIndex[RQIndex]);

        break;
        }
      }
    }     /* END for (nindex) */

  return(SUCCESS);

  }  /* END MJobAllocateLastAvailable() */

/**
 * Allocate nodes using the First Available algorithm (mnalFirstAvailable)
 *
 * @param J (I) job allocating nodes
 * @param RQ (I) req allocating nodes
 * @param NL (I) eligible nodes
 * @param RQIndex (I) index of job req to evaluate
 * @param MinTPN (I) min tasks per node allowed
 * @param MaxTPN (I) max tasks per node allowed
 * @param NodeMap (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex (I/O) index of next node to find in BestList
 * @param BestList (O) list of selected nodes (Best fit according to this algorithm)
 * @param TaskCount (I/O) [optional]
 * @param NodeCount (I/O) [optional]
 */

int MJobAllocateFirstAvailable(

  mjob_t *J,
  mreq_t *RQ,
  mnl_t  *NL,
  int     RQIndex,
  int    *MinTPN,
  int    *MaxTPN,
  char   *NodeMap,
  int     AffinityLevel,
  int    *NodeIndex,
  mnl_t **BestList,
  int    *TaskCount,
  int    *NodeCount)

  {
  int                 nindex;
  mnode_t            *N;
  int                 TC;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (NodeMap[N->Index] != AffinityLevel)
      {
      /* node unavailable */

      continue;
      }

    TC = MNLGetTCAtIndex(NL,nindex);

    if (TC < MinTPN[RQIndex])
      continue;

    TC = MIN(TC,MaxTPN[RQIndex]);

    MNLSetNodeAtIndex(BestList[RQIndex],NodeIndex[RQIndex],N);
    MNLSetTCAtIndex(BestList[RQIndex],NodeIndex[RQIndex],TC);

    NodeIndex[RQIndex] ++;
    TaskCount[RQIndex] += TC;
    NodeCount[RQIndex] ++;

    /* mark node as used */

    if (RQ->DRes.Procs != 0)
      {
      /* only mark node as unavailable for compute node based reqs */

      NodeMap[N->Index] = mnmUnavailable;
      }

    if (TaskCount[RQIndex] >= RQ->TaskCount)
      {
      /* all required tasks found */

      /* NOTE:  HANDLED BY DIST */

      if ((RQ->NodeCount == 0) || 
          (NodeCount[RQIndex] >= RQ->NodeCount))
        {
        /* terminate BestList */

        MNLTerminateAtIndex(BestList[RQIndex],NodeIndex[RQIndex]);

        break;
        }
      }
    }     /* END for (nindex) */

  return(SUCCESS);
  }  /* END MJobAllocateFirstAvailable() */

/**
 * Allocate nodes using the Min Resource algorithm (mnalMinResource)
 *
 * @param J (I) job allocating nodes
 * @param RQ (I) req allocating nodes
 * @param NL (I) eligible nodes
 * @param RQIndex (I) index of job req to evaluate
 * @param MinTPN (I) min tasks per node allowed
 * @param MaxTPN (I) max tasks per node allowed
 * @param NodeMap (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex (I/O) index of next node to find in BestList
 * @param BestList (O) list of selected nodes (Best fit according to this algorithm)
 * @param TaskCount (I/O) [optional]
 * @param NodeCount (I/O) [optional]
 */

int MJobAllocateMinResource(

  mjob_t *J,
  mreq_t *RQ,
  mnl_t  *NL,
  int     RQIndex,
  int    *MinTPN,
  int    *MaxTPN,
  char   *NodeMap,
  int     AffinityLevel,
  int    *NodeIndex,
  mnl_t **BestList,
  int    *TaskCount,
  int    *NodeCount)

  {
  int                 nindex;
  int                 FNC;  /* count of nodes in the NodeList */
  mnode_t            *N;
  int                 NTasks;
  int                 mindex;
  enum MTaskDistEnum  DistPolicy;

  FNC = 0;
  for (nindex = 0;NL->Array[nindex].N != NULL;nindex++)
    {
    FNC++;
    }

  /* NOTE:  NodeMap is used to mark when nodes should no longer be considered
            because they are already allocated.  However, availability is
            determined on a per req basis.  If req 0 requests resources of type 
            A and req 1 requests resources of type B, a node may be marked as
            unavailable even though it may still have resources available for
            a subsequent req (FIXME) */

  /* select 'RQ->TaskCount' tasks with minimal configured resources
     which meet job requirements (last available) */

  if (J->DistPolicy != mtdNONE)
    DistPolicy = J->DistPolicy;
  else if ((J->Credential.C != NULL) && (J->Credential.C->DistPolicy != mtdNONE))
    DistPolicy = J->Credential.C->DistPolicy;
  else
    DistPolicy = MPar[0].DistPolicy;

  /* loop through all memory sizes */

  for (mindex = 0;MSched.CfgMem[mindex] >= 0;mindex++)
    {
    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,NULL) == SUCCESS;nindex++)
      {
      MNLGetNodeAtIndex(NL,FNC - nindex - 1,&N);

      if (N == NULL)
        {
        continue;
        }
 
      if (NodeMap[N->Index] != AffinityLevel)
        {
        /* node unavailable */

        continue;
        }

      MDB(9,fSCHED) MLog("INFO:  comparing node '%s' mem %d with mem bucket %d\n",
        N->Name,
        MNode[N->Index]->CRes.Mem,
        MSched.CfgMem[mindex]);
 
      if ((MNode[N->Index]->CRes.Mem > MSched.CfgMem[mindex]) && 
          (MSched.CfgMem[mindex + 1] >= 0))
        {
        /* node resources too great */
  
        continue;
        } 

      NTasks = MNLGetTCAtIndex(NL,FNC - nindex - 1);

      if (NTasks < MinTPN[RQIndex])
        continue;

      NTasks = MIN(NTasks,MaxTPN[RQIndex]);

      if (RQ->TasksPerNode > 0)
        {
        NTasks -= (NTasks % RQ->TasksPerNode);

        if (NTasks == 0)
          continue;
        }

      MNLSetNodeAtIndex(BestList[RQIndex],NodeIndex[RQIndex],N);
      MNLSetTCAtIndex(BestList[RQIndex],NodeIndex[RQIndex],NTasks);

      NodeIndex[RQIndex]++;

      if (NodeIndex[RQIndex] > MSched.JobMaxNodeCount)
        {
        /* current node mapping requires too many processors */

        /* other feasible node mappings may be available */
 
        /* NOTE:  should compare allocation against optimal possible allocation 
                  and only set hold if job can never run (NYI) */

        MJobSetHold(J,mhBatch,0,mhrNoResources,"job requests too many nodes");

        return(FAILURE);
        }

      TaskCount[RQIndex] += NTasks;
      NodeCount[RQIndex] ++;

      /* mark node as used */

      if (RQ->DRes.Procs != 0)
        {
        /* only mark node as unavailable for compute node based reqs */

        NodeMap[N->Index] = mnmUnavailable;
        }

      if ((DistPolicy != mtdDisperse) || (NodeCount[RQIndex] >= RQ->TaskCount))
        {
        if (TaskCount[RQIndex] >= RQ->TaskCount)
          {
          /* NOTE: HANDLED BY DIST */

          /* all required tasks found */

          if ((RQ->NodeCount == 0) || 
              (NodeCount[RQIndex] >= RQ->NodeCount))
            {
            /* terminate list */

            MNLTerminateAtIndex(BestList[RQIndex],NodeIndex[RQIndex]);

            break;
            }
          }
        }     /* END if ((DistPolicy != mtdDisperse) || (AllocNC[RQIndex] >= RQ->TaskCount)) */
      }       /* END for (nindex) */

    if (TaskCount[RQIndex] >= RQ->TaskCount)
      {
      /* all required procs found */

      if ((RQ->NodeCount == 0) || 
          (NodeCount[RQIndex] >= RQ->NodeCount))
        break;
      }
    }    /* END for (mindex) */

  return(SUCCESS);
  }  /* END MJobAllocateMinResource() */



