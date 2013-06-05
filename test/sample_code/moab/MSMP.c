/**
 * @file MSMP.c
 *
 * Contains job related functions that deal with msmp_t structs.
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"  
#include "moab-const.h"  


/**
 * Gets index into MPar[0].NodeSetList from SN->Feature. 
 *
 * NOTE: MAList[meNFeature][SN->Feature]
 *
 * @param SN
 */

int __MSMPGetNodeSetListIndex(

  msmpnode_t *SN)

  {
  int sindex;

  for (sindex = 0;MPar[0].NodeSetList[sindex] != NULL;sindex++)
    {
    if (strcmp(MAList[meNFeature][SN->Feature],MPar[0].NodeSetList[sindex]) == 0)
      return(sindex);
    }

  return(-1);
  } /* END __MSMPGetNodeSetListIndex() */



/**
 * Find and return an smpnode by a given node.
 *
 * Checks if the node has a feature in SetList and then looks for an smpnode with that feature. 
 *
 * @see MSMPNodeFindByFeature()
 *
 * @param N (I)
 * @param CreateNode (I) If true, create new smpnode if smp node doesn't exist.
 */

msmpnode_t *MSMPNodeFindByNode(
  
  const mnode_t *N,
  mbool_t        CreateNode)

  {
  int         sindex;   /* smp nodes */
  int         findex;   /* feature index */
  char      **SetList;
  msmpnode_t *SN;
 
  if (N == NULL)
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt args when trying to find an smp with a node\n");

    return(FAILURE);
    }

  SN = NULL;

  SetList = MPar[0].NodeSetList;

  /* check if node has feature from NodeSetList */

  for (sindex = 0;SetList[sindex] != NULL;sindex++)
    {
    /* Get index of feature and see if in MSMPNodes, -1 return indicates: NOT FOUND  */

    if ((findex = MUGetNodeFeature(SetList[sindex],mVerify,NULL)) != -1)
      {
      /* Check node's FBM against index */

      if (bmisset(&N->FBM,findex))
        { 
        if ((SN = MSMPNodeFindByFeature(findex)) == FAILURE)
          {
          /* create new smp node if there is no SMP node with the given nodes feature */

          if (CreateNode == TRUE)
            SN = MSMPNodeCreate(MAList[meNFeature][findex],findex,N);
          }  /* END if ((SN = MSMPNodeFind(findex)) == FAILURE) */

        break;
        } /* END if (bmisset(&N->FBM,findex)) */
      } 
    } /* END for (sindex = 0;SetList[sindex] != NULL;sindex++) */

  return(SN);
  }  /* END MSMPNodeFindByNode() */



/**
 * Return an msmpnode_t if it finds an smpnode with the given partition.
 *
 * @param PIndex
 * @return Returns an msmpnode_t if one is found, NULL otherwise.
 */

msmpnode_t *MSMPNodeFindByPartition(

  int PIndex) /* I */

  {
  int sindex = 0;
  msmpnode_t **tmpSN;
  msmpnode_t  *SN;

  if (PIndex < -1)
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt args when trying to find an smp node by partition\n");

    return(FAILURE);
    }

  for (sindex = 0;sindex < MSMPNodes.NumItems;sindex++)
    {
    tmpSN = (msmpnode_t **)MUArrayListGet(&MSMPNodes,sindex);

    if ((tmpSN == NULL) || (*tmpSN == NULL))
      break;

    SN = *tmpSN;

    /* check if smp node has the give feature */
   
    if (SN->PtIndex == PIndex)
      {
      return(SN);
      }
    }

  MDB(5,fSCHED) MLog("WARNING:  Couldn't find an smpnode in partition %s\n",
    MPar[PIndex].Name);

  return(FAILURE); 
  }  /* END MSMPNodeFindByPartition() */





/**
 * Return an msmpnode_t if it finds an smpnode with the given feature.
 *
 * @param Feature The feature to find a node with.
 * @return Returns an msmpnode_t if one is found, NULL otherwise.
 */

msmpnode_t *MSMPNodeFindByFeature(

  int Feature) /* I */

  {
  int sindex = 0;
  msmpnode_t **tmpSN;
  msmpnode_t  *SN;

  if (Feature < -1)
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt args when trying to find an smp node with a feature index\n");

    return(FAILURE);
    }

  for (sindex = 0;sindex < MSMPNodes.NumItems;sindex++)
    {
    tmpSN = (msmpnode_t **)MUArrayListGet(&MSMPNodes,sindex);

    if ((tmpSN == NULL) || (*tmpSN == NULL))
      break;

    SN = *tmpSN;

    /* check if smp node has the give feature */
   
    if (SN->Feature == Feature)
      {
      return(SN);
      }
    }

  MDB(5,fSCHED) MLog("WARNING:  Couldn't find an smpnode with Feature %s (%d)\n",
    (MAList[meNFeature][Feature] != NULL) ? MAList[meNFeature][Feature] : "NULL",
    Feature);

  return(FAILURE); 
  }  /* END MSMPNodeFindByFeature() */





/**
 * Create and add a new msmpnode_t to the SMPNode array.
 * 
 * @param Name (I) The name of the smp
 * @param Feature (I) The feature MAttr index to associate with the smp
 * @param N (I) The node that represents the smp
 */

msmpnode_t *MSMPNodeCreate(
 
  char          *Name,
  int            Feature,
  const mnode_t *N)

  {
  msmpnode_t *SN;

  if (Name == NULL || Feature < -1)
    {
    MDB(1,fSCHED) MLog("ERROR:    %s when trying to create new smp node\n",
      (Name == NULL) ? "Name was NULL" : "Feature was < 0"); 

    return(FAILURE);
    }

  if (N == NULL)
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt node in trying to create new smp node\n");

    return(FAILURE);
    }

  MDB(7,fSCHED) MLog("INFO:     creating new smpnode %s\n",
    Name);

  if ((SN = (msmpnode_t *)MUMalloc(sizeof(msmpnode_t))) == FAILURE)
    {
    MDB(1,fSCHED) MLog("ERROR:    Failed to create new SMP node: %s\n",
      Name);

    return(FAILURE);
    }

  MSMPNodeInitialize(SN);

  SN->Name = Name;

  SN->Feature = Feature;

  if (MUArrayListAppendPtr(&MSMPNodes,(void *)SN) == FAILURE)
    {
    MDB(1,fSCHED) MLog("ERROR:    Failed to append smpnode %s to MSMPNodes\n",
      SN->Name);

    return(FAILURE);
    }
  
  return(SN);
  }  /* END MSMPNodeCreate() */





/**
 * Initializes the given smpnode.
 *
 * @param SN SMP node to initialize
 */

int MSMPNodeInitialize(

  msmpnode_t *SN) /*I*/

  {
  if (SN == NULL)
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt args to initialize smp node\n");

    return(FAILURE);
    }

  SN->Name = NULL;
  SN->Feature = -1;
  SN->N = NULL;

  MCResInit(&SN->CRes);
  MCResInit(&SN->DRes);
  MCResInit(&SN->URes);
  MCResInit(&SN->ARes);
 
  MSMPNodeResetStats(SN);

  return(SUCCESS);
  }  /* END MSMPNodeInitialize() */





/**
 * Resets the stats on the given smpnode.
 *
 * @param SN SMP node to reset 
 */

int MSMPNodeResetStats(

  msmpnode_t *SN) /*I*/ 

  {
  if (SN == NULL)
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt args to reset smp stats\n");

    return(FAILURE);
    }

  MCResClear(&SN->CRes);
  MCResClear(&SN->DRes);
  MCResClear(&SN->URes);
  MCResClear(&SN->ARes);

  return(SUCCESS);
  }  /* END MSMPNodeResetStats() */





/**
 * Resets the stats of all the SMP nodes.
 */

int MSMPNodeResetAllStats()

  {
  int sindex = 0;
  msmpnode_t **tmpSN;
  msmpnode_t  *SN;

  for (sindex = 0;sindex < MSMPNodes.NumItems;sindex++)
    {
    tmpSN = (msmpnode_t **)MUArrayListGet(&MSMPNodes,sindex);

    if ((tmpSN == NULL) || (*tmpSN == NULL))
      break;

    SN = *tmpSN;

    if (MSMPNodeResetStats(SN) == FAILURE)
      {
      MDB(1,fSCHED) MLog("ERROR:    failed to reset smp stats\n");

      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MSMPNodeResetAllStats() */





/**
 * Destory all SMPNodes in MSMPNodes array.
 */

int MSMPNodeDestroyAll()

  {
  int index;
  msmpnode_t *M;

  /* We need to free all the Res of the SMP node before destorying it */

  for (index = 0; index < MSMPNodes.NumItems; index++)
    {
    M = (msmpnode_t *)MUArrayListGetPtr(&MSMPNodes,index);

    if (M != NULL)
      {
      MCResFree(&M->CRes);
      MCResFree(&M->DRes);
      MCResFree(&M->URes);
      MCResFree(&M->ARes);
      }
    }

  if ((MUArrayListFreePtrElements(&MSMPNodes) == FAILURE) || 
      (MUArrayListFree(&MSMPNodes) == FAILURE))
    {
    MDB(1,fSCHED) MLog("ERROR:    Error in freeing MSMPNodes array list\n");
   
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSMPNodeDestroyAll() */




/**
 * Update SMPNode stats from the given node.
 *
 * Assumes MSMPNodeResetStats has been called prior this calling. 
 * This function is different from MSMPNodeUpdateWithNL in that MSMPNodeUpdate
 * updates without first having the stats reset.
 *
 * @see MSMPNodeUpdateWithNL (peer)
 * @see MSMPNodeResetAllStats
 * @see MClusterUpdateNodeState
 *
 * @param N (I) Node to obtain stats from
 */

int MSMPNodeUpdate(

  mnode_t *N) /*I*/

  {
  msmpnode_t  *SN;
 
  mcres_t URes;

  if (N == NULL)
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt smp args\n");

    return(FAILURE);
    }

  if (MNODEISUP(N) == FALSE)
    {
    MDB(7,fSCHED) MLog("INFO:     not updating smp with node %s (state: %s)\n",
        N->Name,
        MNodeState[N->State]);

    return(FAILURE);
    }

  /* update smp information */

  if ((SN = MSMPNodeFindByNode(N,TRUE)) == NULL)
    {
    MDB(1,fSCHED) MLog("ERROR:    failed to find smp with node %s\n",
        N->Name);

    return(FAILURE);
    }

  MCResInit(&URes);

  /* Configured */
  SN->CRes.Procs +=  N->CRes.Procs;
  SN->CRes.Mem   +=  N->CRes.Mem;

  /* Dedicated */
  SN->DRes.Procs += (N->DRes.Procs > 0) ? N->DRes.Procs : 0;
  SN->DRes.Mem   += (N->DRes.Mem > 0)   ? N->DRes.Mem   : 0;

  /* Available */
  SN->ARes.Procs += (N->DRes.Procs <= 0) ? N->CRes.Procs : N->CRes.Procs - N->DRes.Procs;
  SN->ARes.Mem   += (N->DRes.Mem <= 0)   ? N->CRes.Mem : N->CRes.Mem - N->DRes.Mem; 

  /* Utitlized */
  /* completely correct because we are manually setting ARes and DRes. */
  MCResCopy(&URes,&N->CRes);
  MCResRemove(&URes,&N->CRes,&N->ARes,1,FALSE);

  SN->URes.Procs += URes.Procs;
  SN->URes.Mem   += URes.Mem;

  SN->PtIndex = N->PtIndex;

  MCResFree(&URes);

  /* N is used to represent the smp in order to determine best nodeset/smp.
   * See MJobGetRangeWrapper and MJobPTransform and MJobGetNBCount.
   * It's possible that the very first node is marked as drained and
   * that the mem and procs are configured differently than the rest
   * of the nodeboards to show that the nodeboard has problems which
   * will cause problems when determining how many nodeboards are 
   * needed for the job. */

  if ((SN->N == NULL) ||
      ((N->CRes.Procs > SN->N->CRes.Procs) ||
       (N->CRes.Mem   > SN->N->CRes.Mem)))
    SN->N = N;

  return(SUCCESS);
  }  /* END MSMPNodeUpdate() */





/**
 * Update an smp's ARes, CRes, and URes with a job's nodelist.
 *
 * It will find the the corresponding SMPNode with the given NodeList.
 *
 * This function is different from MSMPNodeUpdate in that it is decrementing and
 * incrementing smpnode stats according the Job's nodelist just after being started,
 * in MJobStart, and the stats haven't been cleared before calling this.
 *
 * This is used to update the smpstats for one smpnode right after a job has been 
 * submitted so that the stats are up to date. smpnode stats will be updated 
 * completely in MClusterUpdateNodeState.
 *
 * @see MSMPNodeUpdate (peer)
 * @see MJobStart
 * @see MClusterUpdateNodeState
 *
 * @param J  (I)
 * @param NL (I)
 */

int MSMPNodeUpdateWithNL(

  mjob_t    *J,
  mnl_t     *NL)

  {
  mnode_t *N;

  msmpnode_t *SN;

  if ((NL == NULL) || (MNLIsEmpty(NL)))
    {
    MDB(1,fSCHED) MLog("ERROR:    corrupt args when updating an smp with a job's nodelist\n");

    return(FAILURE);
    }

  /* Find smpnode */

  MNLGetNodeAtIndex(NL,0,&N);

  if ((SN = MSMPNodeFindByNode(N,FALSE)) == NULL)
    {
    MDB(1,fSCHED) MLog("ERROR:    Couldn't find SMPNode with node %s\n",
      N->Name);

    return(FAILURE);
    }

#ifdef MNOT
  /* update the smp stats from the node list */

  for (nindex = 0;NL[nindex].N != NULL;nindex++)
    {
    N = NL[nindex].N;

    /* Node resources should have been updated previosly. */

    SN->ARes.Procs -= N->DRes.Procs;  
    SN->ARes.Mem   -= N->DRes.Mem;

    SN->DRes.Procs += N->DRes.Procs;
    SN->DRes.Mem   += N->DRes.Mem;

    /* Utitlized */
    MCResCopy(&URes,&N->CRes);
    MCResRemove(&URes,&N->CRes,&N->ARes,1,FALSE);

    SN->URes.Procs += URes.Procs;
    SN->URes.Mem   += URes.Mem;
    }
#endif

  SN->ARes.Procs -= J->TotalProcCount;
  SN->ARes.Mem   -= J->Req[0]->ReqMem;

  SN->DRes.Procs += J->TotalProcCount;
  SN->DRes.Mem   += J->Req[0]->ReqMem;
    
  return(SUCCESS);
  }  /* END MSMPNodeUpdateWithNL() */





/**
 * Checks if the given job is feasible on the smpnode with the given feature index.
 *
 * @param J (I)
 * @param Feature (I)
 * @param Time (I)
 */

int MSMPJobIsFeasible(

  const mjob_t *J,
  int           Feature,
  mulong        Time)

  {
  msmpnode_t *SN;
  int JobProcs = 0; /* Job Wide */
  int JobMem   = 0; /* Job Wide */
  int SMPProcs = 0; 
  int SMPMem   = 0; 

  if ((J == NULL) || (Feature < 0))
    {
    MDB(1,fSCHED) MLog("ERROR:    %s when determining if the job is feasible on an smp\n",
      (J == NULL) ? "J was NULL" : "Feature was < 0"); 

    return(FAILURE);
    }

  if ((SN = MSMPNodeFindByFeature(Feature)) == FAILURE)
    {
    MDB(1,fSCHED) MLog("ERROR:    Couldn't find SMPNode with feature %s\n",
      MAList[meNFeature][Feature]);
    
    return(FAILURE);
    }

  JobProcs = J->TotalProcCount;
  
  if (J->Req[0] != NULL)
    JobMem = J->Req[0]->ReqMem;

  /* Is it possible to have the time in the past? */

  if (Time == MSched.Time)
    {
    SMPProcs = SN->ARes.Procs;
    SMPMem   = SN->ARes.Mem;
    }
  else
    {
    SMPProcs = SN->CRes.Procs;
    SMPMem   = SN->CRes.Mem;
    }

  if ((JobProcs <= SMPProcs) && 
      (JobMem   <= SMPMem))
    {
    MDB(7,fSCHED) MLog("INFO:     Job (%s) is feasible on SMPNode %s\n",
      J->Name,
      SN->Name);

    return(SUCCESS);
    }
  
  MDB(7,fSCHED) MLog("INFO:     Job (%s) is not feasible on SMPNode %s\n",
    J->Name,
    SN->Name);

  return(FAILURE);
  } /* END int MSMPJobIsFeasible() */



/**
 * Checks if the job is within the smp's resource limits.
 *
 * @see NODESETMAXUSAGE
 *
 * @param J (I) Job to run on smp.
 * @param SN (I) smp to check against.
 */

mbool_t MSMPJobIsWithinRLimits(

  const mjob_t *J,
  msmpnode_t   *SN)

  {
  int JobProcs = 0; /* Job Wide */
  int JobMem   = 0; /* Job Wide */

  int MaxProcUsage = 0;
  int MaxMemUsage  = 0;

  int usageIndex;
  int OldLogLevel = mlog.Threshold;

  if ((J == NULL) || (SN == NULL))
    {
    return(FAILURE);
    }

  /* check if job can run on smp with node usage policy */

  if ((usageIndex = __MSMPGetNodeSetListIndex(SN)) == -1)
    {
    /* should never happen. */

    MDB(1,fSCHED) MLog("ERROR:    couldn't find index into NodeSetList for node feature %s\n",
      MAList[meNFeature][SN->Feature]);
      
    mlog.Threshold = OldLogLevel;

    return(FALSE);
    }

  if ((J->Credential.C != NULL) && (J->Credential.C->IgnNodeSetMaxUsage == TRUE))
    {
    MDB(7,fSCHED) MLog("INFO:     Job (%s) is in class %s which ignores NODESETMAXUSAGE\n",
        J->Name,
        J->Credential.C->Name);
    
    mlog.Threshold = OldLogLevel;

    return(TRUE);
    }

  if (((MPar[0].NodeSetListMaxUsage[usageIndex] > 0.0) || 
       (MPar[0].NodeSetMaxUsage > 0.0)) &&
      (bmisset(&J->Flags,mjfRsvMap) == FALSE)) /* only look at real jobs */
    {
    JobProcs = J->TotalProcCount;
    
    if (J->Req[0] != NULL)
      JobMem = J->Req[0]->ReqMem;

    if (MPar[0].NodeSetListMaxUsage[usageIndex] > 0.0)
      {
      /* NodeSetListMaxUsage is 0 based, SN->Feature is 1 based (MAList) */

      MaxProcUsage = (int) (SN->CRes.Procs * MPar[0].NodeSetListMaxUsage[usageIndex]);
      MaxMemUsage  = (int) (SN->CRes.Mem   * MPar[0].NodeSetListMaxUsage[usageIndex]);
      }
    else
      {
      MaxProcUsage = (int) (SN->CRes.Procs * MPar[0].NodeSetMaxUsage);
      MaxMemUsage  = (int) (SN->CRes.Mem   * MPar[0].NodeSetMaxUsage);
      }

    if ((JobProcs >= MaxProcUsage) || 
        (JobMem   >= MaxMemUsage))
      {
      char tmpLine[MMAX_LINE];

      snprintf(tmpLine,sizeof(tmpLine),"node %s unavailable due to NODESETMAXUSAGE Policy",
        SN->Name);

      MDB(7,fSCHED) MLog("INFO:     Job (%s) not feasible on SMPNode %s due to NODESETMAXUSAGE:%.2f\n",
        J->Name,
        SN->Name,
        MPar[0].NodeSetMaxUsage);

      return(FALSE);
      }
    } /* END if (MSched.NodeUsagePolicy > 0.0) */

  mlog.Threshold = OldLogLevel;

  return(TRUE);
  } /* END mbool_t MSMPJobIsWithinRLimits() */





/**
 * Returns the estimated use (percentage) used by this job on the given node/smp.
 *
 * @param N          Node that represents the smp node.
 * @param J 
 * @param StartTime 
 *
 * @return Returns -1 if there was an error.
 */

double MSMPNodeGetJobUse(

  const mnode_t *N,
  const mjob_t  *J,
  mulong         StartTime)

  {
  msmpnode_t *SN;
  double Use;
  int JProcs;
  int JMem;
  int SProcs;
  int SMem;
  int NBCount;

  if (N == NULL || J == NULL)
    {
    return(-1);
    }

  if ((SN = MSMPNodeFindByNode(N,FALSE)) == FAILURE)
    {
    return(-1);
    }

  /* Because the whole node will be allocated, use configured memory. */

  NBCount = MJobGetNBCount(J,N);

  JProcs = NBCount * N->CRes.Procs;
  JMem   = NBCount * N->CRes.Mem;
  
  if (StartTime == MSched.Time)
    {
    SProcs = SN->ARes.Procs;
    SMem   = SN->ARes.Mem;
    }
  else
    {
    SProcs = SN->CRes.Procs;
    SMem   = SN->CRes.Mem;
    }
    
  Use = MAX(((double)JProcs / MAX(1,SProcs)), ((double)JMem / MAX(1,SMem)));

  MDB(7,fPBS) MLog("INFO:     job %s's use on %s: %.4f (JProcs: %d SProcs: %d JMem: %d SMem: %d)\n",
      J->Name,
      SN->Name,
      Use,
      JProcs,
      SProcs,
      JMem,
      SMem);

  return(Use);
  } /* END int MSMPNodeGetJobUse() */




/**
 * Determines the nodeset/smp where the job will use the lease amount of resources.
 *
 * @see MJobSelectResourceSet - child
 *
 * @param J         Job to check against smps.
 * @param SetList   NodeSetList, -1 terminated.
 * @param SetIndex  Indexes of nodeset features to MAList[meNFeature][0].
 * @param NodeList  list of nodes which represent feasible nodesets/smps.
 * @param StartTime 
 *
 * @return Returns the index of the best smp in NSets, -1 if an error occurs.
 */

int MSMPGetMinLossIndex(

  mjob_t     *J,         /*I*/
  char      **SetList,   /*I*/
  int        *SetIndex,  /*I*/
  mnl_t      *NodeList,  /*I*/
  mulong      StartTime) /*I*/

  {
  int sindex;
  int tmpBestSet = -1;

  const char *FName = "MSMPGetMinLossIndex";

  MDB(7,fSTRUCT) MLog("%s(%s,SetList,SetIndex,NodeList,ListSize,%ld)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    StartTime);

  if ((J == NULL) || (SetList == NULL))
    return(-1);

  MJobSelectResourceSet(
    J,            /* I */
    J->Req[0],    /* I (optional) */
    StartTime,    /* I */
    mrstFeature,  /* I */
    mrssFirstOf,  /* I */
    SetList,      /* I (optional) */
    NodeList,     /* I/O constraining nodes/selected nodes */
    NULL,
    -1,
    NULL,
    &tmpBestSet,
    NULL); /* O (optional) pointer to selected node set */

  /* tmpBestSet is the index of the actual nodeset in MAList[meNFeature][] no in SetList */

  for (sindex = 0;SetIndex[sindex] != -1;sindex++)
    {
    /* find/return index of SetIndex to best nodeset */

    if (SetIndex[sindex] == tmpBestSet)
      return(sindex);
    }

  return(-1);
  } /* END int MSMPGetMinLossIndex() */




/**
 * Prints out a diagnostic of the smps.
 *
 * Printed from mdiag -n.
 *
 * @see MUINodeDiagnose() - parent
 *
 * @param Buffer The buffer to return the diagnostic string.
 * @param BufSize Size of the buffer.
 */

int MSMPDiagnose(

  char *Buffer,  /*O*/
  int   BufSize) /*O*/

  {
  char *BPtr = NULL;
  int   BSpace = 0; 

  int sindex = 0;
  msmpnode_t **tmpSN;
  msmpnode_t  *SN;
  
  if (Buffer == NULL)
    {
    return(FAILURE);
    }
  
  MUSNInit(&BPtr,&BSpace,Buffer,BufSize);

  MUSNPrintF(&BPtr,&BSpace,"smp summary\n");

  for (sindex = 0;sindex < MSMPNodes.NumItems;sindex++)
    {
    tmpSN = (msmpnode_t **)MUArrayListGet(&MSMPNodes,sindex);

    if ((tmpSN == NULL) || (*tmpSN == NULL))
      break;

    SN = *tmpSN;

    MUSNPrintF(&BPtr,&BSpace,"%-29.29s %4d:%-4d %6d:%-6d\n",
      SN->Name,
      MIN(SN->ARes.Procs,SN->CRes.Procs - SN->DRes.Procs),
      SN->CRes.Procs,
      MIN(SN->ARes.Mem,SN->CRes.Mem - SN->DRes.Mem),
      SN->CRes.Mem);
    }
  
  MUSNPrintF(&BPtr,&BSpace,"\n");

  return(SUCCESS);
  } /* int MSMPDiagnose() */



/**
 * Prints info about all of the smp machines to the log file.
 */

void MSMPPrintAll()
  {
  int sindex = 0;
  msmpnode_t **tmpSN;
  msmpnode_t  *SN;

  for (sindex = 0;sindex < MSMPNodes.NumItems;sindex++)
    {
    tmpSN = (msmpnode_t **)MUArrayListGet(&MSMPNodes,sindex);

    if ((tmpSN == NULL) || (*tmpSN == NULL))
      break;

    SN = *tmpSN;

    /* check if smp node has the give feature */
   
    MDB(7,fSCHED)
      {
      MLog("INFO:     SMPNode: Name=%s Feature=%s CRes(P:%d M:%d) DRes(P:%d M:%d) ARes(P:%d M:%d) URes(P:%d M:%d)\n",
        SN->Name,
        MAList[meNFeature][SN->Feature],
        SN->CRes.Procs,
        SN->CRes.Mem,
        SN->DRes.Procs,
        SN->DRes.Mem,
        SN->ARes.Procs,
        SN->ARes.Mem,
        SN->URes.Procs,
        SN->URes.Mem);
      }
    } /* END for (sindex = 0;sindex < MSMPNodes.NumItems;sindex++) */
  }

/* END MSMP.c */
