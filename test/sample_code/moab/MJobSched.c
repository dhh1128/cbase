
/* HEADER */

      
/**
 * @file MJobSched.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Locate resource availability range for specified job and req.
 *
 * NOTE:  Aggregate available resources 
 * NOTE:  optional calendar inside J only enforced if MAvlNodeList not set 
 * 
 * @see MJobGetEStartTime() - parent
 *
 * @param J               (I) job description
 * @param RQ              (I) job req
 * @param FNL             (I) [optional]
 * @param P               (I) partition in which resources must reside
 * @param MinStartTime    (I) earliest possible starttime
 * @param FindMaxEndRange (I) if FNL provided, find max end range 
 * @param GRL             (O) [optional,minsize=MMAX_RANGE]
 * @param MAvlNodeList    (O) multi-req list of nodes found
 * @param NodeCount       (O) [minsize=MMAX_NODE]
 * @param NodeMap         (O)
 * @param RTBM            (I) [bitmap]
 * @param SeekLong        (I)
 * @param TRange          (O) [optional]
 * @param SEMsg           (O) [optional,minsize=MMAX_LINE]
 *
 * Step 1.0 - Initialize, Determine Fesible Nodes
 * Step 2.0 - Loop Through Nodes
 *   Step 2.1 - Determine Node Availability Ranges 
 *   Step 2.2 - Extract Start Ranges 
 *   Step 2.3 - Update MNodeList 
 *   Step 2.4 - If NodeList not Specified, Merge Node Ranges into Global Range 
 * Step 3.0 - Perform Post-Merge Operations
 */

int MJobGetRange(

  mjob_t      *J,
  mreq_t      *RQ,
  mnl_t       *FNL,
  mpar_t      *P,
  long         MinStartTime,
  mbool_t      FindMaxEndRange,
  mrange_t    *GRL,
  mnl_t      **MAvlNodeList,
  int         *NodeCount,
  char        *NodeMap,
  mbitmap_t   *RTBM,
  mbool_t      SeekLong,
  mrange_t    *TRange,
  char        *SEMsg)

  {
  int      TC;
  int      FNC;

  mrange_t ARange[MMAX_RANGE];
  mrange_t SRange[MMAX_RANGE];

  mnl_t     NodeList;

  mnl_t     *EffNodeList;

  mrange_t RRange[2];           /* required range */

  long     FoundStartTime;
  int      RCount;

  int      ATC;
  int      ANC;

  int      nindex;
  int      nlindex;

  char     Affinity;

  mnode_t *N;

  char    *EMsg;

  char     tEMsg[MMAX_LINE];
  char     TString[MMAX_LINE];

  const char *FName = "MJobGetRange";

  MULToTString(MinStartTime - MSched.Time,TString);
 
  MDB(5,fSCHED) MLog("%s(%s,%d,%s,%s,GRL,%s,NodeCount,NodeMap,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    (P != NULL) ? P->Name : "NULL",
    TString,
    (MAvlNodeList == NULL) ? "NULL" : "MAvlNodeList",
    (SeekLong == TRUE) ? "TRUE" : "FALSE",
    (TRange != NULL) ? "TRange" : "NULL");

  /* NOTE:  GRL or MAvlNodeList must be specified */

  if (SEMsg != NULL)
    EMsg = SEMsg;
  else
    EMsg = tEMsg;

  EMsg[0] = '\0';

#ifndef __MOPT
  if ((J == NULL) || (RQ == NULL))
    {
    return(FAILURE);
    }
#endif /* __MOPT */

  /* Step 1.0 - Initialize, Determine Feasible Nodes */

  memset(ARange,0,sizeof(mrange_t) * 4);
  memset(SRange,0,sizeof(mrange_t) * 4);

  /* ??? Does this enforce hostlist */

  MNLInit(&NodeList);

  if (FNL == NULL)
    {
    if (MReqGetFNL(
          J,
          RQ,
          P,
          NULL,
          &NodeList,     /* O */
          &FNC,          /* O */
          &TC,           /* O */
          MMAX_TIME,     /* MMAX_TIME = feasible at any point in time (ignore state) */
          0,
          EMsg) == FAILURE)
      {
      MNLFree(&NodeList);

      MDB(6,fSCHED) MLog("INFO:     cannot locate configured resources in %s() - %s\n",
        FName,
        EMsg);

      return(FAILURE);
      }

    FNL = &NodeList;
    }

  if (NodeMap != NULL)
    memset(NodeMap,mnmUnavailable,sizeof(char) * MSched.M[mxoNode]);

  if (GRL != NULL)
    GRL[0].ETime   = 0;

  /* NOTE:  RRange is execution time range */

  RRange[0].STime = MinStartTime;

  if ((J->CMaxDate > 0) && (J->CMaxDateIsActive != FALSE))
    {
    RRange[0].ETime = J->CMaxDate;
    }
  else
    {
    RRange[0].ETime = MMAX_TIME;
    }

/*
  if (MSched.RsvTimeDepth > 0)
    RRange[0].ETime = MIN(RRange[0].ETime,MSched.Time + MSched.MaxRMPollInterval + MSched.RsvTimeDepth);
*/

  RRange[0].TC    = MAX(1,RQ->TasksPerNode);

  RRange[1].ETime = 0;

  nlindex = 0;

  ATC = 0;
  ANC = 0;

  N = NULL;

  /* Step 2.0 - Loop Through Nodes */

  EffNodeList = FNL;

  for (nindex = 0;MNLGetNodeAtIndex(EffNodeList,nindex,&N) == SUCCESS;nindex++)
    {
    int RsvCount = 0;
    int RangeCount = MMAX_RANGE;
    char BRsvID[MMAX_NAME];

    /* Step 2.1 - Determine Node Availability Ranges */

    if (FindMaxEndRange == TRUE)
      {
      if (!MNODEISAVAIL(N->State))
        continue;

      RangeCount = 1;
      }

    if (MJobGetSNRange(
          J,
          RQ,
          N,
          RRange,
          RangeCount,
          &Affinity,
          NULL,
          ARange,  /* O */
          NULL,
          SeekLong,
          BRsvID) == FAILURE)
      {
      MULToTString(MinStartTime - MSched.Time,TString);

      MDB(7,fSCHED) MLog("INFO:     no reservation time found for job %s on node %s at %s\n",
        J->Name,
        N->Name,
        TString);

      if (NodeMap != NULL)
        NodeMap[N->Index] = mnmUnavailable;

      continue;
      }

    if (NodeMap != NULL)
      NodeMap[N->Index] = Affinity;

    if (MRangeApplyLocalDistributionConstraints(ARange,J,N) == FAILURE)
      {
      /* no valid ranges available */

      continue;
      }

    /* Step 2.2 - Extract Start Ranges */

    if (FindMaxEndRange == TRUE)
      {
      /* if we have not filled in a range yet, or the range for this node
         is shorter than the current range then use the new range. */

      if ((SRange[0].ETime == 0) || 
          (ARange[0].ETime < SRange[0].ETime))
        {
        memcpy(SRange,ARange,sizeof(SRange));
        }
      else
        {
        continue;
        }
      }
    else
      {
      if (MRLSFromA(J->SpecWCLimit[0],ARange,SRange) == FAILURE)
        {
        /* no valid ranges available */

        continue;
        }
      }

    if ((SRange[0].ETime != 0) &&
        (SRange[0].STime <= RRange[0].STime))
      {
      /* if exact match on starttime */

      ANC++;

      ATC += ARange[0].TC;

      if (MAvlNodeList != NULL)
        {
        /* Step 2.3 - Update MNodeList */

        RsvCount = MNodeGetRsvCount(N,FALSE,FALSE);

        if (RsvCount < (N->PtIndex == MSched.SharedPtIndex) ? MSched.MaxRsvPerGNode : MSched.MaxRsvPerNode)
          {
          /* only add the node if it can have more reservations */

          MNLSetNodeAtIndex(MAvlNodeList[RQ->Index],nlindex,N);
          MNLSetTCAtIndex(MAvlNodeList[RQ->Index],nlindex,ARange[0].TC);

          nlindex++;

          MDB(7,fSCHED) MLog("INFO:     node %d '%sx%d' added to nodelist\n",
            ANC,
            N->Name,
            ARange[0].TC);
          }
        else
          {
          MDB(7,fSCHED) MLog("INFO:     node %d '%sx%d' not added to nodelist (MAXRSVPERNODE)\n",
            ANC,
            N->Name,
            ARange[0].TC);
          }
        }  /* END if (MAvlNodeList != NULL) */
      }    /* END if ((SRange[0].ETime != 0) && ...) */
    else
      {
      /* check if merging ranges together helps */

      MULToTString(MinStartTime - MSched.Time,TString);

      MDB(7,fSCHED) MLog("INFO:     availability does not fit into primary requested time range on node %s at %s (A[0]: %ld:%ld  R: %ld:%ld)\n",
        N->Name,
        TString,
        ARange[0].STime,
        ARange[0].ETime,
        RRange[0].STime,
        RRange[0].STime + J->WCLimit);

      MDB(7,fSCHED) MLog("INFO:     availability does not fit into primary requested time range on node %s at %ld (A[1]: %ld:%ld  A[2]: %ld:%ld)\n",
        N->Name,
        RRange[0].STime - MSched.Time,
        ARange[1].STime,
        ARange[1].ETime,
        ARange[2].STime,
        ARange[2].ETime);
      }

    /* Step 2.4 - If NodeList not Specified, Merge Node Ranges into Global Range */

    if ((MAvlNodeList == NULL) && (GRL != NULL))
      {
      MRLMerge(GRL,SRange,J->Request.TC,SeekLong,&FoundStartTime);

      if (Affinity == mnmRequired)
        {
        /* constrain feasible time frames */

        MRLAND(GRL,GRL,SRange,FALSE);
        }

      if (bmisset(RTBM,mrtcStartEarliest))
        RRange[0].ETime = FoundStartTime;
      }  /* END else (MAvlNodeList != NULL) */
    }    /* END for (nindex) */

  /* Step 3.0 - Perform Post-Merge Operations */

  if (FindMaxEndRange == TRUE)
    {
    MNLFree(&NodeList);

    /* If we are getting the max end time range for a specified node list then we are done - no post processing required */

    return(SUCCESS);
    }

  if (GRL != NULL)
    {
    MRangeApplyGlobalDistributionConstraints(GRL,J,&ATC);
    }

  if (NodeCount != NULL)
    *NodeCount = ANC;

  if (MAvlNodeList != NULL)
    {
    MNLTerminateAtIndex(MAvlNodeList[RQ->Index],nlindex);

    MNLTerminateAtIndex(MAvlNodeList[RQ->Index + 1],0);

    if (RQ->TaskCount > ATC)
      {
      if (!bmisset(&J->Flags,mjfBestEffort) || (ATC == 0))
        {
        MULToTString(MinStartTime - MSched.Time,TString);

        MDB(6,fSCHED) MLog("ALERT:    inadequate tasks located for job %s at %s (%d < %d)\n",
          J->Name,
          TString,
          ATC,
          RQ->TaskCount);

        snprintf(EMsg,MMAX_LINE,"inadequate tasks located for job %s at %s (%d < %d)",
          J->Name,
          TString,
          ATC,
          RQ->TaskCount);
 
        MNLFree(&NodeList);

        return(FAILURE);
        }
      }

    if ((RQ->NodeCount > ANC) || (ANC == 0))
      {
      if (!bmisset(&J->Flags,mjfBestEffort) || (ANC == 0))
        {
        MULToTString(MinStartTime - MSched.Time,TString);

        MDB(6,fSCHED) MLog("ALERT:    inadequate nodes located for job %s at %s (%d < %d)\n",
          J->Name,
          TString,
          ANC,
          RQ->NodeCount);

        snprintf(EMsg,MMAX_LINE,"inadequate nodes located for job %s at %s (%d < %d)",
          J->Name,
          TString,
          ANC,
          RQ->NodeCount);

        MNLFree(&NodeList);

        return(FAILURE);
        }
      }

    /* NOTE:  GRL not populated if MNodeList specified */

    /* calendar constraints must be enforced w/GRL specification */

    MNLFree(&NodeList);

    return(SUCCESS);
    }  /* END if (MAvlNodeList != NULL) */

  /* get only feasible ranges */

  if (MPar[0].MaxMetaTasks > 0)
    {
    /* impose meta task limit */

    MRLLimitTC(
      GRL,
      MRange,
      NULL,
      MPar[0].MaxMetaTasks);
    }

  if ((GRL != NULL) && (TRange != NULL))
    memcpy(TRange,GRL,sizeof(mrange_t) * MMAX_RANGE);

  if (!bmisset(&J->Flags,mjfBestEffort))
    MJobSelectFRL(J,RQ,GRL,RTBM,&RCount);

  if ((GRL != NULL) && (GRL[0].ETime == 0))
    {
    MDB(6,fSCHED) MLog("INFO:     no ranges found\n");

    sprintf(EMsg,"no ranges found");

    MNLFree(&NodeList);

    return(FAILURE);
    }

  /* return feasible resource info with termination range */

  for (nindex = 0;nindex < MMAX_RANGE;nindex++)
    {
    if ((GRL == NULL) || (GRL[nindex].ETime != 0))
      continue;

    GRL[nindex].NC = FNC;
    GRL[nindex].TC = TC;

    break;
    }

  MDB(6,fSCHED) MLog("INFO:     %d ranges located for job %s in %s\n",
    nindex,
    J->Name,
    FName);
      
  MNLFree(&NodeList);

  return(SUCCESS);
  }  /* END MJobGetRange() */




/**
 * Distribute tasks from a MNL to a multi-req request w/guaranteed per req 
 * exclusive allocation of resources, unless EnableMultiReqNodeUse is TRUE,
 * in which case resources across reqs are proc node exclusive but not proc exclusive.
 *
 *
 * NOTE: this routine should keep allocating until all SrcMNL have been allocated.  
 *       Then MJobAllocMNL() will work on getting the reqs on the right nodes. (TODO, NYI)
 *
 * @see MJobSelectMNL() - parent
 * @see MJobGetEStartTime() - parent
 * @see MJobGetAMNodeList() - parent
 *
 * NOTE: should distribute all tasks available on node to hungriest req.
 *
 * NOTE:  If MSched.AllowMultiReqNodeUse is false, nodes cannot be
 *        assigned to multiple reqs
 *
 * TODO: This function generates an MNL with only the minimal number of tasks
 *       needed for each req. For MSched.AllowMultiReqNodeUse to actually work,
 *       extra available task space on nodes needs to be put into DstMNL.
 *       (See Dave Jackson for details)
 *
 * @param J (I)
 * @param SrcMNL (I)
 * @param DstMNL (O)
 */

int MJobDistributeMNL(

  mjob_t      *J,      /* I */
  mnl_t      **SrcMNL, /* I */
  mnl_t      **DstMNL) /* O */

  {
  int nindex;

  int rqindex;

  int index;
  int tindex;

  int *NFreq = NULL;                    /* number of reqs which can utilize node */
  int *ONFreq = NULL;                   /* original NFreq */
  int  FreqCount[MMAX_REQ_PER_JOB + 1];     /* number of nodes which match req frequency count */

  int *NReqTC[MMAX_REQ_PER_JOB]; /* tasks provided by node to each req */

  int TTAvail[MMAX_REQ_PER_JOB];          /* total tasks available to req */
  double TAvail[MMAX_REQ_PER_JOB]; 

  /* NOTE:  TAvail is SrcMNL[rqindex][nindex].TC / NFreq[nindex] */

  double TReq[MMAX_REQ_PER_JOB];    /* total tasks required by req */

  int   NIndex[MMAX_REQ_PER_JOB];   /* req node index */

  mnl_t *NodeList;

  double mval;
  double val;

  int   nfindex;  /* node frequency index - number of reqs which can use node */
  int   mindex;
  int   TC;

  int   URC;  /* unsatisfied req count */
  int   UNC;  /* unfilled node count */

  int   MaxFreq = 0; /* maximum node req frequency DOUG - FIXME */

  mreq_t *RQ;

  mbool_t  FailureDetected = FALSE;

  mnode_t *N1;
  mnode_t *N2;

  int ArraySize = sizeof(int) * MSched.M[mxoNode];

  const char *FName = "MJobDistributeMNL";

  MDB(4,fSCHED) MLog("%s(%s,SrcMNL,DstMNL)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  /* Step 0  Validate Parameters */

  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((J->Req[1] == NULL) || (DstMNL == NULL))
    {
    /* only one req exists, modification not required */

    if (DstMNL != NULL)
      {
      MNLMultiCopy(DstMNL,SrcMNL);
      }

    return(SUCCESS);
    }

  /* Step 1.0  Initialize */

  /* Step 1.1  Clear Variables */

  NFreq = (int *)MUCalloc(1,ArraySize);
  ONFreq = (int *)MUCalloc(1,ArraySize);

  memset(NReqTC,0,sizeof(NReqTC));

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    NReqTC[rqindex] = (int *)MUCalloc(1,ArraySize);
    }

  memset(FreqCount,0,sizeof(FreqCount));
  memset(TAvail,0,sizeof(TAvail));
  memset(TTAvail,0,sizeof(TTAvail));

  memset(TReq,0,sizeof(TReq));

  /* NOTE:  must balance by node request as well */

  memset(NIndex,0,sizeof(NIndex));

  /* Step 1.2  Determine node frequency sets */

  UNC = 0;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    for (nindex = 0;MNLGetNodeAtIndex(SrcMNL[rqindex],nindex,&N1) == SUCCESS;nindex++)
      {
      if (NFreq[N1->Index] == 0)
        UNC++;

      NFreq[N1->Index]++;

      MaxFreq = MAX(MaxFreq,NFreq[N1->Index]); 

      NReqTC[rqindex][N1->Index] = MNLGetTCAtIndex(SrcMNL[rqindex],nindex);

      MDB(8,fSCHED) MLog("INFO:     job %s:%d source nodelist[%d]: %sx%d\n",
        J->Name,
        rqindex,
        nindex,
        N1->Name,
        MNLGetTCAtIndex(SrcMNL[rqindex],nindex));
      }  /* END for (nindex) */

    MDB(6,fSCHED) MLog("INFO:     job %s:%d source nodes: %d\n",
      J->Name,
      rqindex,
      nindex);
    }  /* END for (rqindex) */

  /* Step 1.3  Calculate available tasks */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    TReq[rqindex] = (double)MAX(
      J->Req[rqindex]->TaskCount,
      J->Req[rqindex]->NodeCount);

    for (nindex = 0;MNLGetNodeAtIndex(SrcMNL[rqindex],nindex,&N1) == SUCCESS;nindex++)
      {
      TAvail[rqindex] += (double)MNLGetTCAtIndex(SrcMNL[rqindex],nindex) /
        NFreq[N1->Index];

      FreqCount[NFreq[N1->Index]]++;

      TTAvail[rqindex] += MNLGetTCAtIndex(SrcMNL[rqindex],nindex);

      MDB(7,fSCHED) MLog("INFO:     job %s:%d has node %s:%d available\n",
        J->Name,
        rqindex,
        N1->Name,
        MNLGetTCAtIndex(SrcMNL[rqindex],nindex));
      }  /* END for (nindex) */

    MDB(6,fSCHED) MLog("INFO:     job %s:%d available nodes: %d (%f,%f,%d)\n",
      J->Name,
      rqindex,
      nindex,
      TReq[rqindex],
      TAvail[rqindex],
      TTAvail[rqindex]);
    }  /* END for (rqindex) */

  URC = rqindex;

  /* Step 2.0  Assign Nodes */

  /* Step 2.1  Assign HostList Nodes (optional) */

  if (bmisset(&J->IFlags,mjifHostList) && 
     (J->ReqHLMode != mhlmSubset) &&
     (J->ReqHLMode != mhlmSuperset))  /* superset constraints already imposed in incoming NodeList */
    {
    int ATC;  /* allocated taskcount */

    if (MNLIsEmpty(&J->ReqHList))
      {
      MDB(4,fSCHED) MLog("ALERT:    job %s requests hostlist but has no hostlist\n",
        J->Name);

      MUFree((char **)&NFreq);
      MUFree((char **)&ONFreq);

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        MUFree((char **)&NReqTC[rqindex]);
        }

      return(FAILURE);
      }

    /* allocate hostlist nodes to reqs first */

    /* NOTE:  assume only hostlist nodes in SrcMNL[J->HLIndex][] */

    rqindex = J->HLIndex;

    RQ = J->Req[rqindex];

    mindex = rqindex;

    NodeList = SrcMNL[rqindex];

    /* NOTE:  only assign number of tasks requested per host to req */

    for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N1) == SUCCESS;nindex++)
      {
      if (NFreq[N1->Index] == 0)
        {
        /* node no longer requested */

        continue;
        }

      /* locate N in J->ReqHList */

      for (index = 0;MNLGetNodeAtIndex(&J->ReqHList,index,&N2) == SUCCESS;index++)
        {
        if (N2 == N1)
          break;
        }  /* END for (index) */

      if (N2 == NULL)
        {
        MDB(6,fSCHED) MLog("ALERT:    cannot locate hostlist node for job %s\n",
          J->Name);

        MUFree((char **)&NFreq);
        MUFree((char **)&ONFreq);

        for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
          {
          MUFree((char **)&NReqTC[rqindex]);
          }

        return(FAILURE);
        }

      if (MNLGetTCAtIndex(NodeList,nindex) < MNLGetTCAtIndex(&J->ReqHList,index))
        {
        MDB(6,fSCHED) MLog("ALERT:    inadequate tasks for hostlist node '%s' for job %s\n",
          N1->Name,
          J->Name);
        
        MUFree((char **)&NFreq);
        MUFree((char **)&ONFreq);

        for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
          {
          MUFree((char **)&NReqTC[rqindex]);
          }

        return(FAILURE);
        }

      ATC = MNLGetTCAtIndex(&J->ReqHList,index);

      MNLSetNodeAtIndex(DstMNL[mindex],NIndex[mindex],N1);
      MNLSetTCAtIndex(DstMNL[mindex],NIndex[mindex],ATC);

      NIndex[mindex]++;

      if (MSched.AllowMultiReqNodeUse == FALSE)
        UNC--;

      TC = MIN(MAX(RQ->TaskCount,RQ->NodeCount),ATC);

      if ((TReq[mindex] > 0.0) && (TReq[mindex] <= (double)TC))
        URC--;
 
      TReq[mindex] -= (double)TC;

      for (index = 0;J->Req[index] != NULL;index++)
        {
        if (NReqTC[index][N1->Index] > 0)
          {
          TAvail[index] -= (double)TC / NFreq[N1->Index];
          TTAvail[index] -= TC;

          if (MSched.AllowMultiReqNodeUse == FALSE)
            NReqTC[index][N1->Index] = 0;
          }
        }    /* END for (index) */

      /* decrement node frequency */

      NFreq[N1->Index]--;

      if (TReq[mindex] <= 0.0)
        break;
      }  /* END for (nindex)  */
    }    /* END if (bmisset(&J->IFlags,mjifHostList) && (J->ReqHLMode != mhlmSubset)) */

  /* Step 2.2  Allocate non-hostlist reqs */

  memcpy(ONFreq,NFreq,ArraySize);

  /* allocate most constrained resources first - ie, locate nodes which 
     can only be used by one req and assign them to that req, then locate
     nodes which can be used by only a few reqs, etc */

  for (nfindex = 1;nfindex <= MaxFreq;nfindex++)
    {
    if (FreqCount[nfindex] == 0)
      {
      /* no nodes have matching frequency count */

      continue;
      }

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      /* examine all nodes available to each req */

      if (bmisset(&J->IFlags,mjifHostList) && 
         (J->ReqHLMode != mhlmSubset) &&
         (J->ReqHLMode != mhlmSuperset))  /* superset constraints already imposed in incoming NodeList */
        {
        if (rqindex == J->HLIndex)
          {
          /* ignore hostlist req which was processed earlier */

          continue;
          }
        }

      NodeList = SrcMNL[rqindex];

      /* search all nodes available to this req which have matching frequency */

      for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N1) == SUCCESS;nindex++)
        {
        if (ONFreq[N1->Index] != nfindex)
          {
          /* node no longer available/requested or does not meet current 
             eval loop constraints */

          continue;
          }

        mval   = -999999.0;
        mindex = -1;

        MDB(7,fSCHED) MLog("INFO:     assigning node %s:%d from %s:%d\n",
          N1->Name,
          MNLGetTCAtIndex(NodeList,nindex),
          J->Name,
          rqindex);

        /* determine 'hungriest' req */

        for (index = 0;J->Req[index] != NULL;index++)
          {
          if (index >= MMAX_REQ_PER_JOB)
            {
            /* req overflow */

            MUFree((char **)&NFreq);
            MUFree((char **)&ONFreq);

            for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
              {
              MUFree((char **)&NReqTC[rqindex]);
              }

            return(FAILURE);
            }

          if (NReqTC[index][N1->Index] <= 0)
            {
            /* node provides no tasks for req */

            continue;
            }

          if (nfindex == 1)
            {
            /* only one req can use node */

            mindex = index;
        
            break;
            }

          /* should algo include tasks provided to each req by node 
             (enable greedy?) */

          /* NOTE:  if choice makes complete solution impossible, discard choice */

          for (tindex = 0;tindex < MMAX_REQ_PER_JOB;tindex++)
            {
            if (J->Req[tindex] == NULL)
              break;

            if (tindex == index)
              {
              /* we are evaluating the feasibility of assigning node resources
                 to J->Req[index] by determining if other reqs can still be 
                 populated, do not evaluate self */

              continue;
              }

            if (((J->Req[tindex]->DRes.Procs != 0) && (J->Req[index]->DRes.Procs != 0)) ||
                ((J->Req[tindex]->DRes.Mem != 0) && (J->Req[index]->DRes.Mem != 0)) ||
                ((J->Req[tindex]->DRes.Disk != 0) && (J->Req[index]->DRes.Disk != 0)))
              {
              int tmpTC;

              tmpTC = NReqTC[tindex][N1->Index];

              if (MSched.AllowMultiReqNodeUse == TRUE)
                {
                tmpTC = MIN(
                  MAX(J->Req[tindex]->TaskCount,J->Req[tindex]->NodeCount),
                  tmpTC);
                }

              /* only enforce check on reqs which mutually consume processors, memory, or disk */

              if ((TReq[tindex] > 0.0) &&
                 ((int)TReq[tindex] > TTAvail[tindex] - tmpTC))
                {
                /* J->Req[tindex] requires TReq[tindex] additional tasks, but 
                   only TTAvail[tindex] tasks are available to this req and 
                   this node provides NReqTC[tindex][] tasks */

                /* remaining tasks required by J->Req[tindex] cannot be satisfied if node
                   resources are made unavailable via allocation to J->Req[index] */

                /* current selection makes complete solution impossible */

                break;
                }
              }
            }  /* END for (tindex) */

          if ((tindex < MMAX_REQ_PER_JOB) && (J->Req[tindex] != NULL))
            {
            /* current selection makes complete solution impossible */

            continue;
            }

          if (URC >= UNC)
            {
            if (NReqTC[index][N1->Index] < (int)TReq[index])
              {
              /* NOTE:  if task does not fully satisfy req and N unsatisfied reqs remain
                        and N available nodes remain, do not assign node to current eligible
                        req in case it can fully satisfy a different req */

              /* current selection makes complete solution impossible */

              break;
              }
            }

          if (TReq[index] > 0)
            {
            /* req still needs resources - calculate hunger as proportion of 
               available tasks required by this req (should guarantee hungry 
               reqs always get fed first) */

            val = TReq[index] / MAX(1,TAvail[index]);
            }
          else
            {
            /* req is satisfied - round-robin allocation */

            val = TReq[index];
            }

          if (val > mval)
            {
            /* new req is highest priority */

            mval   = val;
            mindex = index;
            }
          }    /* END for (index) */

        if (mindex == -1) 
          {
          int rqindex2;

          /* no req located */

          MDB(7,fSCHED) MLog("INFO:     suitable req not located for node %s:%d\n",
            N1->Name,
            MNLGetTCAtIndex(NodeList,nindex));

          for (rqindex2 = 0;rqindex2 < MMAX_REQ_PER_JOB;rqindex2++)
            {
            if (J->Req[rqindex2] == NULL)
              {
              /* end of list reached */

              break;
              }

            if (NReqTC[rqindex2][N1->Index] > 0)
              {
              /* node can provide resources to req - assign FCFS */

              break;
              }
            }  /* END for (rqindex2) */

          if (J->Req[rqindex2] == NULL)
            { 
            /* node is not required by any req (previously allocated?) */

            MDB(7,fSCHED) MLog("INFO:     all reqs satisfied - node %s:%d not required\n",
              N1->Name,
              MNLGetTCAtIndex(NodeList,nindex));
 
            continue;
            }

          /* no possible node assignment selection available */

          MDB(7,fSCHED) MLog("INFO:     no req wants to allocate node %s:%d from %s:%d\n",
            N1->Name,
            MNLGetTCAtIndex(NodeList,nindex),
            J->Name,
            rqindex);

          /* should not occur - but continue to next node to determine if 
             satisfactory allocation still possible */

          continue;
          }  /* END if (mindex == -1) */

        /* hungriest req located */

        /* find TC for alloc'd req node list */

        TC = NReqTC[mindex][N1->Index];

        if (MSched.AllowMultiReqNodeUse == TRUE)
          {
          TC = MIN(
            MAX(J->Req[mindex]->TaskCount,J->Req[mindex]->NodeCount),
            TC);
          }

        MDB(6,fSCHED) MLog("INFO:     node %sx%d assigned to %s:%d (hunger: %f, req: %f, avail: %f)\n",
          N1->Name,
          TC,
          J->Name,
          mindex,
          mval,
          TReq[mindex],
          TAvail[mindex]);

        MNLSetNodeAtIndex(DstMNL[mindex],NIndex[mindex],N1);

        MNLSetTCAtIndex(DstMNL[mindex],NIndex[mindex],TC);

        NIndex[mindex]++;

        if (MSched.AllowMultiReqNodeUse == FALSE)
          UNC--;

        if ((TReq[mindex] > 0.0) && (TReq[mindex] <= (double)TC))
          URC--;

        TReq[mindex] -= (double)TC;

        /* adjust resource consumption w/in other reqs */

        for (index = 0;J->Req[index] != NULL;index++)
          {
          if (NReqTC[index][N1->Index] > 0)
            {
            TAvail[index] -= (double)TC / NFreq[N1->Index];

            TTAvail[index] -= TC;

            if (MSched.AllowMultiReqNodeUse == FALSE)
              {
              /* report node does not/cannot provide resources to other reqs */

              NReqTC[index][N1->Index] = 0;
              }
            else
              {
              /* FIXME: only really works with consistent task definitions across reqs */

              NReqTC[index][N1->Index]--;
              }
            }
          }    /* END for (index) */

        /* decrement node frequency */

        NFreq[N1->Index]--;
        } /* END for (nindex)  */
      }   /* END for (rqindex) */
    }     /* END for (nfindex) */

  /* Step 3.0  Finalize */

  /* terminate lists */

  MUFree((char **)&NFreq);
  MUFree((char **)&ONFreq);

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    MUFree((char **)&NReqTC[rqindex]);
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    if (NIndex[rqindex] == 0)
      {
      FailureDetected = TRUE;
      }

    MNLTerminateAtIndex(DstMNL[rqindex],NIndex[rqindex]);

    MDB(7,fSCHED)
      {
      mnode_t *N;

      for (nindex = 0;MNLGetNodeAtIndex(DstMNL[rqindex],nindex,&N) == SUCCESS;nindex++)
        {
        MLog("INFO:     job %s:%d nodelist[%d]: %sx%d\n",
          J->Name,
          rqindex,
          nindex,
          N->Name,
          MNLGetTCAtIndex(DstMNL[rqindex],nindex));
        }  /* END for (nindex) */
      }

    if (TReq[rqindex] > 0.0)
      {
      MDB(2,fSCHED) MLog("INFO:     cannot locate nodes for job '%s' req[%d] (%d additional needed)\n",
        J->Name,
        rqindex,
        (int)TReq[rqindex]);

      return(FAILURE);
      }

    MDB(4,fSCHED) MLog("INFO:     dist nodelist for job %s:%d (%d of %d)\n",
      J->Name,
      rqindex,
      NIndex[rqindex],
      J->Req[rqindex]->TaskCount);
    }   /* END for (rqindex) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobDistributeMNL() */
