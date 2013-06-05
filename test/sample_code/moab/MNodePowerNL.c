/* HEADER */

      
/**
 * @file MNodePowerNL.c
 *
 * Contains: MNLSetPower and support functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Given a Node, perform a skip filter on whether the request
 * to power on/off should be done. 
 *
 * Return a skip boolean status for this node and 
 *    a skip reason that will be returned to caller
 *
 * @param     N           (I)  pointer to N to be checked
 * @param     PState      (I)  Desired power state of Node
 * @param     CFlags
 * @param     ShouldSkip  (I)  return pointer to return skip flag
 * @param     SkipReason  (I)  String reason for the skip
 */

int __MNLCheckNodeSkipFilter(
    
  mnode_t                *N,
  enum MPowerStateEnum    PState,
  mbitmap_t              *CFlags,
  mbool_t                *ShouldSkip,
  const char            **SkipReason)

  {
  enum MSystemJobTypeEnum JobType;
  mjob_t                 *J;

  /* map power request to a JobType state */
  JobType = (PState == mpowOn) ? msjtPowerOn : msjtPowerOff;

  /* Initialize returned values to default state */
  *ShouldSkip = FALSE;
  *SkipReason = "unknown";

  /* check for current Node 'state to see if it matches request */
  if (N->PowerState == PState)
    {
    *ShouldSkip = TRUE;
    *SkipReason = ((PState == mpowOn) ? "already ON" : "already OFF");
    }
  else
    {
    /* Check for other constraints to skip this node */
    mln_t *tmpL = NULL;

    /* Iterate over all active Node System jobs and see:
     *    that they can be found AND they are a poweroff/poweron
     *    system job, that is the same as the current request. 
     *    That means they are already 'inflight' and doing
     *    what we already want, therefore no need to re-issue at this time
     */
    for (tmpL = N->Action;tmpL != NULL;tmpL = tmpL->Next)
      {
      if ((MJobFind(tmpL->Name,&J,mjsmBasic) == SUCCESS) &&
          (MJobGetSystemType(J) == JobType))
        {
        *ShouldSkip = TRUE;
        *SkipReason = ((PState == mpowOn) ? "power ON in process" : "power OFF in process");
        break;
        }
      }

    /* If a power OFF request && NOT in FORCED mode && not already skipped: 
     *    need to check for Active VMs and Jobs on the node and if present skip them.
     * If in FORCED mode, then include busy nodes in the power down operation
     */
    if ((mpowOff == PState) &&
        ((CFlags != NULL) && bmisset(CFlags,mcmForce)) &&
        (*ShouldSkip == FALSE))
      {
      /* Check for VMs on the node, if any, filter this node out */
      if ((N->VMList != NULL) && (MULLGetCount(N->VMList) > 0))
        {
        *ShouldSkip = TRUE;
        *SkipReason = "has active VMs running";
        }
      else
        {
        /* Now check for active non-VM jobs on the node */
        if (SUCCESS == MNodeCheckAllocation(N))
          {
          *ShouldSkip = TRUE;
          *SkipReason = "has active jobs running";
          }
        } 
      } /* END if ((*ShouldSkip == FALSE) && (mpowOff == PState)) */

    } /* END if(N->PowerState == PState) */

  return(SUCCESS);
  } /* END __MNLCheckNodeSkipFilter() */

    
/**
 * Build a list of PowerRequest entries, one per node
 *
 * @param PowerOffReq     (I)
 * @param NodeList        (I) [modified]
 * @param PState          (I) requested power state  (ON or OFF)
 * @param CFlags          (I)
 * @param sindexP         (I)
 * @param Response        (I)
 */

int MNLPowerBuildActionReqList(

  mpoweroff_req_t      *PowerOffReq,
  marray_t             *NodeList,
  enum MPowerStateEnum  PState,
  mbitmap_t            *CFlags,
  int                  *sindexP,
  mstring_t            *Response)

  {
  int                     nindex;
  int                     countIndex = 0;
  const char             *SkipReason;
  mbool_t                 ShouldSkip;
  mnode_t                *N;

  if ((NULL == Response) || (NULL == sindexP))
    return(FAILURE);

  /* Init the list for powerReqs */
  MPowerOffReqInit(PowerOffReq);

  *sindexP = 0;

  /* Build the action node list:
   *    Iterate the argument node list, and do skip filtering on it
   */
  for (nindex = 0;nindex < NodeList->NumItems;nindex++)
    {
    /* Get current iteration item */
    N = (mnode_t *)MUArrayListGetPtr(NodeList,nindex);
            
    /* Do a skip filter on a given node */
    __MNLCheckNodeSkipFilter(N,PState,CFlags,&ShouldSkip,&SkipReason);

    /* If a node should be skipped, report it 'Ignored" with a reason */

    if (ShouldSkip == TRUE)
      {
      MStringAppendF(Response,"Ignoring node %s: %s\n", N->Name, SkipReason);
      continue;
      }

    /* Position the remaining Nodes into the returned Node container */

    MNLSetNodeAtIndex(&PowerOffReq->NodeList,countIndex,N);
    MNLSetTCAtIndex(&PowerOffReq->NodeList,countIndex,1);

    countIndex++;
    }  /* END for (nindex) */
 
  /* If we have something in the list, then terminate the list */
  MNLTerminateAtIndex(&PowerOffReq->NodeList,countIndex);

  /* return number of nodes in the request list */
  *sindexP = countIndex;

  return(SUCCESS);
  } /* END MNLPowerBuildActionReqList() */



/**
 * Given a Node List, preempt jobs on that node
 *
 * @param NodeList           (I)
 */

int MNLIssueJobPreempt(
    
  mnl_t *NodeList)

  {
  /* aggregate info for per-job rather than per-node preemption */

  int        jindex;

  mjob_t    *JList[MMAX_JOB] = {0};
  mnl_t      tmpJNL;

  /* Gather a list of jobs on the node list */
  MNLGetJobList(
    NodeList,
    NULL,
    JList,
    NULL,
    MMAX_JOB);

  MNLInit(&tmpJNL);

  /* Iterate over the Job List, preempting each entry */
  for (jindex = 0;JList[jindex] != NULL;jindex++)
    {
    MNLAND(&JList[jindex]->NodeList,NodeList,&tmpJNL);

    /* Preempt the job one at a time */
    MJobPreempt(
      JList[jindex],
      NULL,
      NULL,
      &tmpJNL,
      "node powered off",
      mppNONE,
      TRUE,
      NULL,
      NULL,
      NULL);
    }  /* END for (jindex) */

  MNLFree(&tmpJNL);

  return(SUCCESS);
  } /* END MNLIssueJobPreempt() */


/**
 * Generate migration Jobs for VMs on this node
 *
 * @param  PowerOffReq  (I)
 * @param  NumItems     (I) 
 * @param  Response
 */

int MNLPowerOffPerformMigrationOfJobs(

  mpoweroff_req_t      *PowerOffReq,
  int                   NumItems,
  mstring_t            *Response)

  {
  /* submit requisite migration jobs */

  int       nindex;
  int       ListEnd;
  int       rc = SUCCESS;
  mnode_t **FailedNodes;

  FailedNodes = (mnode_t **)MUMalloc(NumItems * sizeof(FailedNodes[0]));

  /* Set to NumItems + 1 to allow for NULL termination */

  MUArrayListResize(&PowerOffReq->Dependencies,NumItems + 1);

  MPowerOffSubmitMigrationJobs(
    &PowerOffReq->NodeList,
    (mln_t **)PowerOffReq->Dependencies.Array,
    FailedNodes,
    &ListEnd);

  PowerOffReq->Dependencies.NumItems = ListEnd;

  for (nindex = 0;FailedNodes[nindex] != NULL;nindex++)
    {
    MStringAppendF(Response,"evacuation of jobs/VMs failed for node %s: unable to migrate VMs away from node\n",
      FailedNodes[nindex]->Name);
    }

  if (FailedNodes[0] != NULL)
    rc = FAILURE;

  return(rc);
  } /* END MPowerOffSubmitMigrationJobs() */


/**
 * Generate system jobs to perform the Power operation
 *
 */
int MNLGeneratePowerActionSystemJobs(

  mpoweroff_req_t      *PowerOffReq,
  enum MPowerStateEnum  PState,
  int                   sindex,
  char                 *Auth,
  mbitmap_t            *MCMFlags,
  mstring_t            *Response,
  msysjob_t           **SysJobArray,      /* (O) */
  unsigned int          SysJobArraySize)
  
  {
  int             rc = SUCCESS;
  marray_t        Jobs;

  MUArrayListCreate(&Jobs,sizeof(mjob_t *),sindex);

  if ((MUGenerateSystemJobs(
       Auth,
       NULL,
       &PowerOffReq->NodeList,
       (PState == mpowOn) ? msjtPowerOn : msjtPowerOff,
       (PState == mpowOn) ? "poweron" : "poweroff",
       -1,
       NULL,
       PowerOffReq,
       FALSE,
       TRUE,
       MCMFlags,
       &Jobs) == FAILURE))
    {
    MStringAppendF(Response,"ERROR:    power %s request failed\n",
      (PState == mpowOn) ? "on" : "off");

    rc = FAILURE;
    }
  else 
    {
    mjob_t **JList = (mjob_t **)Jobs.Array;

    int jindex = 0;

    MAssert((Jobs.NumItems == sindex) ||  /* power-on jobs */
           (Jobs.NumItems == sindex - 1), /* for power-off jobs */
           "Number of Power on/off jobs should be set",
           __FUNCTION__, __LINE__); 

    /* Generate a return list of jobs, which are the system jobs just generated */
    for (jindex = 0;jindex < Jobs.NumItems;jindex++)
      {
      mjob_t *J = JList[jindex];
      mnode_t *N;
 
      MNLGetNodeAtIndex(&J->ReqHList,0,&N);

      MStringAppendF(Response,"submitted job %s for node %s \n",
        J->Name,
        N->Name);

      if ((SysJobArray != NULL) && ((unsigned int)jindex < SysJobArraySize))
        {
        SysJobArray[jindex] = J->System;
        }
      }

    if ((SysJobArray != NULL) && ((unsigned int)jindex < SysJobArraySize))
      {
      SysJobArray[jindex] = NULL;
      }
    }

  MUArrayListFree(&Jobs);
  return(rc);
  }


/**
 * modify node's power state
 *
 * @param Auth      (I)
 * @param NodeList  (I) [modified]
 * @param PState    (I) requested power state  (ON or OFF)
 * @param CFlags    (I) [bitmap of XXX]
 * @param Response
 * @param SysJobArray     (O)  [optional]
 * @param SysJobArraySize (I)
 */

int MNLSetPower(

  char                 *Auth,
  marray_t             *NodeList,
  enum MPowerStateEnum  PState,
  mbitmap_t            *CFlags,
  mstring_t            *Response,
  msysjob_t           **SysJobArray,
  unsigned int          SysJobArraySize)

  {
  mpoweroff_req_t PowerOffReq;

  mbitmap_t MCMFlags;

  int             sindex;
  int             rc;

  /* Setup return string buffer and error message buffer */

  /* Ensure that we have a valid Nodelist with at least one Node */
  if ((Response == NULL) || (NodeList == NULL) || (NodeList->NumItems <= 0))
    {
    MStringAppendF(Response,"No nodes specified\n");
    return(FAILURE);
    }

  /* If a SysJobArray was requested, initialize it */
  if (SysJobArray != NULL) 
    {
    if (SysJobArraySize < 1)
      {
      return(FAILURE);
      }
    SysJobArray[0] = NULL;
    }

  /* Generate a PowerOffRequest list from the NodeList */
  rc = MNLPowerBuildActionReqList(&PowerOffReq,NodeList,PState,CFlags,&sindex,Response);
  if (FAILURE == rc)
    {
    MStringAppendF(Response,"Could not build action list\n");

    return(FAILURE);
    }

  /* Check if there are any nodes after the filter, if not, then we are done */
  if (sindex == 0)
    {
    MStringAppendF(Response,"No nodes to power %s\n",
      (PState == mpowOn) ? "on" : "off");

    MPowerOffReqFree(&PowerOffReq);

    return(SUCCESS);
    }

  /* We have nodes that need to change power state: we peform that below */

  /* If --flags=force were set, then we must evacuate the node before power OFF */
  if ((CFlags != NULL) && bmisset(CFlags,mcmForce))
    {
    /* convert from mulong flags to longer char based flags, for use below */
    bmset(&MCMFlags,mcmForce);

    /* On a FLAGS=FORCE and a power-OFF request, we need to evacuate the node */
    if (PState == mpowOff)
      {
      MNLIssueJobPreempt(&PowerOffReq.NodeList);
      MNLPowerOffPerformMigrationOfJobs(&PowerOffReq,sindex,Response);
      } /* END if (PState == mpowOff) */
    } /* END if (bmisset(CFlags,mcmForce)) */


  /* Here we actually issue the POWER OFF jobs to the nodes */

  rc = MNLGeneratePowerActionSystemJobs(
                        &PowerOffReq,
                        PState,
                        sindex,
                        Auth,
                        &MCMFlags,
                        Response,
                        SysJobArray,
                        SysJobArraySize);

  bmclear(&MCMFlags);

  /* free temporary structures */
  MPowerOffReqFree(&PowerOffReq);    

  return(rc);
  }  /* END MNLSetPower() */

/* END MNodePowerNL.c */
