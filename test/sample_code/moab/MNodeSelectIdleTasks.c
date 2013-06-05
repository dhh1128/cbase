/* HEADER */

      
/**
 * @file MNodeSelectIdleTasks.c
 *
 * Contains: MNodeSelectIdelTasks()
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/* private structures */

typedef struct
  {
  mnode_t *N;
  int      TC;
  long     ATime;
  } _mpnodealloc_t;

/* END private structures */

/**
 *
 *
 * @param A (I)
 * @param B (I)
 */

int __MNLATimeCompLToH(

  _mpnodealloc_t *A,  /* I */
  _mpnodealloc_t *B)  /* I */

  {
  /* order low to high */

  return(A->ATime - B->ATime);
  }  /* END __MNLATimeCompLToH() */



/**
 * Select nodes from specified feasible node list which are available for immediate use.
 *
 * @see MJobSelectMNL() - parent - select tasks available for immediate use
 * @see MReqCheckNRes() - child
 *
 * @param J         (I)
 * @param SRQ       (I) [optional]
 * @param FNL       (I)
 * @param IMNL      (O) idle nodes
 * @param TaskCount (O) [optional]
 * @param NodeCount (O) [optional]
 * @param NodeMap   (O)
 * @param RejCount  (O)
 *
 * @return report SUCCESS only if all tasks required for job are idle
 */

int MNodeSelectIdleTasks(

  const mjob_t  *J,
  const mreq_t  *SRQ,
  const mnl_t   *FNL,
  mnl_t        **IMNL,
  int           *TaskCount,
  int           *NodeCount,
  char          *NodeMap,
  int            RejCount[MMAX_REQ_PER_JOB][marLAST])

  {
  mreq_t  *RQ;
  mnode_t *N;

  int     rqindex;
  int     nindex;
  int     index;

  enum MAllocRejEnum RIndex;

  char    NodeAffinity;

  int     TotalTC;
  int     TotalNC;

  long    ATime;

  mnl_t  *NodeList;

  int     ANC[MMAX_REQ_PER_JOB];
  int     ATC[MMAX_REQ_PER_JOB];

  _mpnodealloc_t *MPN = NULL;

  int     tmpTC;

  mbool_t InadequateTasks = FALSE;

  const char *FName = "MNodeSelectIdleTasks";

  MDB(4,fSCHED) MLog("%s(%s,%d,FNL,IMNL,TC,NC,NodeMap,RejCount)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (SRQ != NULL) ? SRQ->Index : -1);

  if (TaskCount != NULL)
    *TaskCount = 0;
 
  if (NodeCount != NULL)
    *NodeCount = 0;

  if (RejCount != NULL)
    memset(RejCount,0,sizeof(RejCount));

  if ((J == NULL) || 
      (FNL == NULL) ||
      (IMNL == NULL))
    {
    return(FAILURE);
    }

  TotalTC = 0;
  TotalNC = 0;

  MPN = (_mpnodealloc_t *)MUCalloc(1,sizeof(_mpnodealloc_t) * MSched.M[mxoNode]);

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if ((SRQ != NULL) && (RQ != SRQ))
      continue;

    NodeList = IMNL[rqindex];
 
    nindex = 0;
 
    ATC[rqindex] = 0;
    ANC[rqindex] = 0;
 
    for (index = 0;MNLGetNodeAtIndex(FNL,index,&N) == SUCCESS;index++)
      {
      /* NOTE:  MReqCheckNRes does not check job hostlist */
      /*        hostlist constraints must be enforced when creating FNL */

      if (MReqCheckNRes(
            J,
            N,
            RQ,
            MSched.Time,
            &tmpTC,       /* O */
            1.0,
/* NOTE: added in 04/07, removed 10/07 as it causes suspended jobs to require all feasible tasks
         to be available in order to be resumed
            (J->State == mjsSuspended) ? FNL[index].TC : 0,
*/
            0,
            &RIndex,
            &NodeAffinity,
            &ATime,
            FALSE,
            FALSE,
            NULL) == FAILURE)
        {
        /* record why job failed requirements check */

        if (RejCount != NULL)
          RejCount[rqindex][RIndex]++;

        if (NodeMap != NULL) 
          NodeMap[N->Index] = mnmUnavailable;

        if (tmpTC > 0)
          ATC[rqindex] += tmpTC;

        continue;
        }
 
      if ((!MReqIsGlobalOnly(RQ)) &&
          (bmisset(&J->Flags,mjfAdvRsv)) && (!bmisset(&J->IFlags,mjifNotAdvres)))
        {
        if ((NodeAffinity == mnmPositiveAffinity) ||
            (NodeAffinity == mnmNeutralAffinity) ||
            (NodeAffinity == mnmNegativeAffinity) ||
            (NodeAffinity == mnmRequired))
          {
          MDB(7,fSCHED) MLog("INFO:     node '%s' added (reserved)\n",
            N->Name);
          }
        else
          {
          MDB(7,fSCHED) MLog("INFO:     node '%s' rejected (not reserved)\n",
            N->Name);
 
          NodeMap[N->Index] = mnmUnavailable;
 
          continue;
          }
        }

      if ((MPar[0].NAllocPolicy == mnalLastAvailable) ||
          (MPar[0].NAllocPolicy == mnalInReverseReportedOrder))
        { 
        /* NOTE:  sort jobs in min avail time first order */
      
        MPN[nindex].N     = N;
        MPN[nindex].TC    = MIN(tmpTC,MNLGetTCAtIndex(FNL,index));
        MPN[nindex].ATime = ATime;

        MNLSetNodeAtIndex(NodeList,nindex,N);
        MNLSetTCAtIndex(NodeList,nindex,MIN(tmpTC,MNLGetTCAtIndex(FNL,index)));
        }
      else
        {
        MNLSetNodeAtIndex(NodeList,nindex,N);
        MNLSetTCAtIndex(NodeList,nindex,MIN(tmpTC,MNLGetTCAtIndex(FNL,index)));
        }
 
      if (RQ->TasksPerNode > 0)
        {
        int tmpTC = MNLGetTCAtIndex(NodeList,nindex);

        MNLSetTCAtIndex(NodeList,nindex,tmpTC - tmpTC % RQ->TasksPerNode);
        }
 
      ATC[rqindex] += MNLGetTCAtIndex(NodeList,nindex);

      ANC[rqindex]++;
 
      NodeMap[N->Index] = NodeAffinity;
 
      MDB(6,fSCHED) MLog("INFO:     node[%d] %s added to task list (%d tasks : %d tasks total)\n",
        nindex,
        N->Name,
        MNLGetTCAtIndex(NodeList,nindex),
        ATC[rqindex]);
 
      nindex++;
      }    /* END for (index)  */

    if ((MPar[0].NAllocPolicy == mnalLastAvailable) ||
        (MPar[0].NAllocPolicy == mnalInReverseReportedOrder))
      {
      int index;

      /* sort node list */

      qsort(
        (void *)MPN,
        nindex,
        sizeof(_mpnodealloc_t),
        (int(*)(const void *,const void *))__MNLATimeCompLToH);

      /* copy list into NodeList */

      for (index = 0;index < nindex;index++)
        {
        MNLSetNodeAtIndex(NodeList,index,MPN[index].N);
        MNLSetTCAtIndex(NodeList,index,MPN[index].TC);
        }  /* END for (index) */ 
      }    /* END if (MPar[0].NAllocPolicy == mnalLastAvailable) */

    /* terminate list */

    MNLTerminateAtIndex(NodeList,nindex);

    TotalTC += ATC[rqindex];
    TotalNC += ANC[rqindex];

    if (ATC[rqindex] < RQ->TaskCount)
      {
      MDB(3,fSCHED) MLog("INFO:     inadequate feasible tasks found for job %s:%d (%d < %d)\n",
        J->Name,
        rqindex,
        ATC[rqindex],
        RQ->TaskCount);

      InadequateTasks = TRUE;
      }
 
    if ((RQ->NodeCount > 0) &&
        (ANC[rqindex] < RQ->NodeCount))
      {
      MDB(3,fSCHED) MLog("INFO:     inadequate nodes found for job %s:%d (%d < %d)\n",
        J->Name,
        rqindex, 
        ANC[rqindex],
        RQ->NodeCount);

      InadequateTasks = TRUE; 
      }
    }      /* END for (rqindex) */

  if (TaskCount != NULL)
    *TaskCount = TotalTC;
 
  if (NodeCount != NULL)
    *NodeCount = TotalNC;

  MUFree((char **)&MPN);

  if (InadequateTasks == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNodeSelectIdleTasks() */
/* END MNodeSelectIdleTasks.c */
