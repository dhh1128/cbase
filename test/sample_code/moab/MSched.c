/* HEADER */

#include "moab-proto.h"


/**
 * @file MSched.c
 *
 * Contains high-level scheduling related functions. 
 * Includes setting scheduling policies, checking jobs and queues, etc.
 */  
        
#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "mdbi.h"  



/**
 *
 *
 * @param A (I)
 * @param B (I)
 */

int __MSchedQSortHLComp(

  mnalloc_old_t *A,  /* I */
  mnalloc_old_t *B)  /* I */

  {
  /* order hi to low */

  return(B->TC - A->TC);
  }  /* END __MSchedQSortHLComp() */


/**
 *
 *
 * @param A (I)
 * @param B (I)
 */

int __MPrioQSortHLPComp(

  mnpri_t *A,  /* I */
  mnpri_t *B)  /* I */

  {
  /* order hi to low */

  if (A->Prio > B->Prio)
    {
    return(-1);
    }
  else if (A->Prio < B->Prio)
    {
    return(1);
    }

  return(0);
  }  /* END __MPrioQSortHLPComp() */


/**
 *
 *
 * @param J             (I) job allocating nodes
 * @param RQ            (I) req allocating nodes
 * @param NodeList      (I) eligible nodes
 * @param RQIndex       (I) index of job req to evaluate
 * @param MinTPN        (I) min tasks per node allowed
 * @param MaxTPN        (I) max tasks per node allowed
 * @param NodeMap       (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex     (IO) index of next node to find in BestList
 * @param BestList      (I/O) list of selected nodes
 * @param TaskCount     (IO) total tasks allocated to req
 * @param NodeCount     (IO) total nodes allocated to req
 */

int MJobAllocateBalanced(
 
  mjob_t     *J,
  mreq_t     *RQ,
  mnl_t      *NodeList,
  int         RQIndex,
  int        *MinTPN,
  int        *MaxTPN,
  char       *NodeMap,
  int         AffinityLevel,
  int        *NodeIndex,
  mnl_t     **BestList,
  int        *TaskCount,
  int        *NodeCount)      /* IO total nodes allocated to req                  */
 
  {
  int nindex;
  int TC;
 
  mnode_t *N;
 
  int         MinSpeed;
  int         LastSpeed = -1;

  /* select slowest nodes first */
 
  while (TaskCount[RQIndex] < RQ->TaskCount)
    { 
    MinSpeed = MMAX_INT;

    /* determine slowest node */

    for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(NodeList,nindex);
 
      if (NodeMap[N->Index] != AffinityLevel)
        continue;

      if ((N->ProcSpeed > LastSpeed) && (N->ProcSpeed < MinSpeed))
        {
        MinSpeed = N->ProcSpeed;
        }
      }    /* END for (nindex) */

    if (MinSpeed == MMAX_INT)
      {
      return(FAILURE);
      }      
   
    /* allocate slowest nodes */

    for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(NodeList,nindex);
 
      if (NodeMap[N->Index] != AffinityLevel)
        { 
        /* node unavailable */
 
        continue;
        }
 
      if (TC < MinTPN[RQIndex])
        continue;
 
      TC = MIN(TC,MaxTPN[RQIndex]);
 
      if (N->ProcSpeed != MinSpeed)
        continue;
 
      /* populate BestList with selected node */
 
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

    LastSpeed = MinSpeed;
    }  /* END while (TaskCount[RQIndex] < RQ->TaskCount) */
 
  return(SUCCESS);
  }  /* END MJobAllocateBalanced() */





/**
 * Enforce 'priority' node allocation policy.
 *
 * @see MJobAllocMNL() - parent
 *
 * @param J             (I) job allocating nodes
 * @param RQ            (I) req allocating nodes
 * @param NL            (I) 
 * @param RQIndex       (I) index of job req to evaluate
 * @param MinTPN        (I) min tasks per node allowed
 * @param MaxTPN        (I) max tasks per node allowed
 * @param NodeMap       (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex     (I/O) index of next node to find in BestList
 * @param BestList      (I/O) list of selected nodes
 * @param TaskCount     (I/O) total tasks allocated to req
 * @param NodeCount     (I/O) total nodes allocated to req
 * @param StartTime     (I)
 */

int MJobAllocatePriority(
 
  const mjob_t *J,
  const mreq_t *RQ,
  mnl_t        *NL,
  int           RQIndex,
  int          *MinTPN,
  int          *MaxTPN,
  char         *NodeMap,
  int           AffinityLevel,
  int          *NodeIndex,
  mnl_t       **BestList,
  int          *TaskCount,
  int          *NodeCount,
  long          StartTime)
 
  {
  int NIndex;
  int nindex;
 
  mpar_t *P = NULL;

  unsigned int TC;
 
  mnode_t *N;

  static mnpri_t *NodePrio = NULL;

  int             ReqMem;  /* memory found for req RQ */

  const char *FName = "MJobAllocatePriority";

  MDB(8,fUI) MLog("%s(%s,RQ,...)\n",
    FName,
    J->Name);

 if ((NL == NULL) || (MNLIsEmpty(NL)))
    {
    return(FAILURE);
    }

  if ((NodePrio == NULL) &&
     ((NodePrio = (mnpri_t *)MUCalloc(1,sizeof(mnpri_t) * (MSched.M[mxoNode] + 1))) == NULL))
    {
    MDB(3,fSCHED) MLog("ALERT:    cannot allocate memory for nodelist in %s\n",
      FName);

    return(FAILURE);
    }

  MNLGetNodeAtIndex(NL,0,&N);

  P = &MPar[N->PtIndex];

  MDB(8,fSCHED)
    {
    char tmpBuf[MMAX_BUFFER];

    MNLToString(NL,TRUE,",",'\0',tmpBuf,sizeof(tmpBuf));

    MLog("INFO:     %s NL: %s\n",
      FName,
      tmpBuf);
    }

  /* select highest priority nodes first */
 
  /* sort list by node priority */
 
  NIndex = 0;

  ReqMem = 0; 

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    /* N is pointer in node list */
    /* taskcount is node's priority    */

    if (NodeMap[N->Index] != AffinityLevel)
      {
      /* node unavailable */

      MDB(8,fSCHED) MLog("INFO:     node %s unavailable: NodeMap[%d]:%d != AffinityLevel:%d\n",
        N->Name,
        N->Index,
        NodeMap[N->Index],
        AffinityLevel);
 
      continue;
      }

    NodePrio[NIndex].N = N;           
    NodePrio[NIndex].TC = MNLGetTCAtIndex(NL,nindex);

    MNodeGetPriority(
      N,
      J,
      RQ,
      NULL,
      NodeMap[N->Index],
      bmisset(&N->IFlags,mnifIsPref),
      &NodePrio[NIndex].Prio,
      StartTime,
      NULL);

    MDB(7,fSCHED) MLog("INFO:     node '%s' priority '%.2f'%s%s\n",
      N->Name,
      NodePrio[NIndex].Prio,
      (J->NodePriority != NULL ? ", Job: " : ""),
      (J->NodePriority != NULL ? J->Name : ""));

    NIndex++;

    if (NIndex >= MSched.M[mxoNode])
      break;
    }  /* END for (nindex) */

  NodePrio[NIndex].N = NULL;
 
  if (NIndex == 0)
    {
    MDB(7,fSCHED) MLog("ALERT:    no nodes with with affinity level %d\n",
      AffinityLevel);

    return(FAILURE);
    }
 
  /* HACK:  machine prio and fastest first both use same qsort algo */

  qsort(
    (void *)&NodePrio[0],
    NIndex,
    sizeof(mnpri_t),
    (int(*)(const void *,const void *))__MPrioQSortHLPComp);

  /* select first 'RQ->TaskCount' procs */
 
  for (nindex = 0;NodePrio[nindex].N != NULL;nindex++)
    {
    TC = NodePrio[nindex].TC;

    if ((TC == 0) || (TC < (unsigned int)MinTPN[RQIndex]))
      {
      /* node unavailable */
 
      continue;
      }
 
    TC = MIN(TC,(unsigned int)MaxTPN[RQIndex]);
 
    N = NodePrio[nindex].N;

    MNLSetNodeAtIndex(BestList[RQIndex],NodeIndex[RQIndex],N);
 
    MNLSetTCAtIndex(BestList[RQIndex],NodeIndex[RQIndex],TC);
 
    NodeIndex[RQIndex] ++;
    TaskCount[RQIndex] += TC;
    NodeCount[RQIndex] ++;

    ReqMem += N->CRes.Mem;
            
    MDB(7,fSCHED) MLog("INFO:     TaskCount: %d of %d  NodeCount: %d of %d  Mem: %d of %d\n",
        TaskCount[RQIndex],
        RQ->TaskCount,
        NodeCount[RQIndex],
        RQ->NodeCount,
        ReqMem,
        RQ->ReqMem);
 
    /* mark node as used */

    if (TC >= (unsigned int)N->CRes.Procs) 
      {
      /* NOTE:  partial hack, need available taskcount info */

      if (RQ->DRes.Procs != 0)
        {
        /* only mark node as unavailable for compute node based reqs */

        NodeMap[N->Index] = mnmUnavailable;
        }
      }
 
    if (TaskCount[RQIndex] >= RQ->TaskCount)
      {
      /* all required tasks found */
 
      /* NOTE:  HANDLED BY DIST */
 
      if ((RQ->NodeCount == 0) ||
          (NodeCount[RQIndex] >= RQ->NodeCount))
        {
        /* all required nodes found */

        MDB(7,fSCHED) MLog("INFO:     all required nodes found\n");

        if (bmisset(&P->Flags,mpfSharedMem))
          {
          /* NOTE:  check enforce RQ->ReqMem */

          if (ReqMem >= RQ->ReqMem)
            {
            /* all required nodes found for job wide requirments */

            MDB(7,fSCHED) MLog("INFO:     Job wide requirements fulfilled (%d/%d tasks) (%d/%d nodes) (%d/%d mem)\n",
                TaskCount[RQIndex],
                RQ->TaskCount,
                NodeCount[RQIndex],
                RQ->NodeCount,
                ReqMem,
                RQ->ReqMem);

            MNLTerminateAtIndex(BestList[RQIndex],NodeIndex[RQIndex]);

            break;
            }
          }
        else
          {
          /* terminate BestList */
  
          MNLTerminateAtIndex(BestList[RQIndex],NodeIndex[RQIndex]);
  
          break;
          }
        } /* END if ((RQ->NodeCount == 0) || */
      }
    }     /* END for (nindex) */
 
  return(SUCCESS);
  }  /* END MJobAllocatePriority() */





/**
 *
 *
 * @param J             (I) job allocating nodes
 * @param RQ            (I) req allocating nodes
 * @param NodeList      (I) eligible nodes
 * @param RQIndex       (I) index of job req to evaluate
 * @param MinTPN        (I) min tasks per node allowed
 * @param MaxTPN        (I) max tasks per node allowed
 * @param NodeMap       (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex     (I/O) index of next node to find in BestList
 * @param BestList      (I/O) list of selected nodes
 * @param TaskCount     (I/O) total tasks allocated to req
 * @param NodeCount     (I/O) total nodes allocated to req
 */

int MJobAllocateFastest(
 
  mjob_t *J,
  mreq_t *RQ,
  mnl_t  *NodeList,
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
  int NIndex;
  int nindex;
 
  int TC;
 
  mnode_t *N;
  mnode_t *N1;
 
  mbool_t UseNodeSpeed;
 
  static mnalloc_old_t *NodeSpeed = NULL;

  if ((NodeList == NULL) || (MNLIsEmpty(NodeList)))
    {
    return(FAILURE);
    }

  if ((NodeSpeed == NULL) &&
     ((NodeSpeed = (mnalloc_old_t *)MUCalloc(1,sizeof(mnalloc_old_t) * MSched.JobMaxNodeCount)) == NULL))
    {
    return(FAILURE);
    }
 
  /* select fastest nodes first */
 
  /* sort list by node speed/procspeed */
 
  memset(NodeSpeed,0,sizeof(NodeSpeed));

  /* verify node speed not identical for all nodes */

  UseNodeSpeed = FALSE;

  MNLGetNodeAtIndex(NodeList,0,&N1);

  for (nindex = 1;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    
    if (N->Speed != N1->Speed)
      {
      UseNodeSpeed = TRUE;
      
      break;
      }
    }    /* END for (nindex) */

  NIndex = 0;
 
  for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    /* N is pointer in node list */
    /* taskcount is node's speed */

    if (NodeMap[N->Index] != AffinityLevel)
      {
      /* node unavailable */
 
      continue;
      }
 
    NodeSpeed[NIndex].N = N;        

    if (UseNodeSpeed == TRUE)
      {
      NodeSpeed[NIndex].TC = (int)(N->Speed * 100.0);
      }
    else
      {
      NodeSpeed[NIndex].TC = N->ProcSpeed;
      }
  
    NIndex++;
    }  /* END for (nindex) */

  if (NIndex == 0)
    {
    return(FAILURE);
    }

  /* NOTE:  machine prio and fastest first both use same qsort algo */
 
  qsort(
    (void *)&NodeSpeed[0],
    NIndex,
    sizeof(mnalloc_old_t),
    (int(*)(const void *,const void *))__MSchedQSortHLComp);
 
  /* select first 'RQ->TaskCount' procs */
 
  for (nindex = 0;NodeSpeed[nindex].N != NULL;nindex++)
    {
    TC = NodeSpeed[nindex].TC;

    if ((TC == 0) || (TC < MinTPN[RQIndex]))
      {
      /* node unavailable */

      continue;
      }

    TC = MIN(TC,MaxTPN[RQIndex]);

    N = NodeSpeed[nindex].N;

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
  }  /* END MJobAllocateFastest() */





/**
 * NOTE:  This routine has no side-affects on AQ or general scheduler environment 
 *
 * @see MSchedProcessJobs() - parent
 * @see MQueueGetRequeueValue() - child
 *
 * @param AQ
 * @param Time
 */

int MQueueGetBestRQTime(

  mjob_t **AQ,    /* I - active queue */
  long    *Time)  /* O */

  {
  long ETime;
  long CTime;

  long OETime;

  int  jindex;

  double tmpValue;

  long BestTime;
  double BestValue;

  mjob_t *J;

  if (Time != NULL)
    *Time = -1;

  if (AQ == NULL)
    {
    return(FAILURE);
    }

  OETime = -1;

  /* AQ is list of active jobs pre-sorted by completion time */

  /* determine final completion time */
 
  CTime = -1;
 
  for (jindex = 0;AQ[jindex] != NULL;jindex++)
    {
    if (AQ[jindex + 1] == NULL)
      CTime = AQ[jindex]->StartTime + AQ[jindex]->WCLimit;
    }  /* END for (jindex) */

  if (CTime == -1)
    {
    /* no active jobs found */
 
    if (Time != NULL)
      *Time = MSched.Time;
 
    return(SUCCESS);
    }

  if (MQueueGetRequeueValue(AQ,MSched.Time,CTime,&tmpValue) != FAILURE)
    {
    char TString[MMAX_LINE];

    BestTime  = MSched.Time;
    BestValue = tmpValue;

    MULToTString(0,TString);

    MDB(6,fSCHED) MLog("INFO:     requeue value %.2f found for immediate action (T: %s)\n",
      BestValue,
      TString);
    }
  else
    {
    BestTime  = -1;
    BestValue = -99999.0;
    }

  /* determine cost at all job completion event times */

  for (jindex = 0;AQ[jindex] > 0;jindex++)
    {
    J = AQ[jindex];

    ETime = J->StartTime + J->WCLimit;

    if (ETime <= OETime)
      continue;

    OETime = ETime;

    if (MQueueGetRequeueValue(AQ,ETime,CTime,&tmpValue) == FAILURE)
      continue;

    if ((BestTime == -1) || (tmpValue > BestValue))
      {
      char TString[MMAX_LINE];
      BestTime  = ETime;
      BestValue = tmpValue;      

      MULToTString(ETime - MSched.Time,TString);

      MDB(3,fSCHED) MLog("INFO:     requeue value %.2f found at completion of job %s (T: %s)\n",
        BestValue,
        J->Name,
        TString);
      }
    }  /* END for (jindex) */

  if (BestTime == -1)
    {
    return(FAILURE);
    }

  if (Time != NULL)
    *Time = BestTime;

  return(SUCCESS);
  }  /* END MQueueGetBestRQTime() */





/**
 * Determine if job is good potential requeue target.
 * 
 * @see MQueueGetBestRQTime() - parent
 *
 * @param AQ
 * @param ETime
 * @param CTime
 * @param Value
 */

int MQueueGetRequeueValue(

  mjob_t **AQ,       /* I */
  long     ETime,    /* I */
  long     CTime,    /* I */
  double  *Value)    /* O */

  {
  int jindex;

  double IPS = 0.0;
  double CPS = 0.0;

  int  PC;

  mjob_t *J;

  /* value = IPS[E -> Ec] - CPS */

  if (AQ == NULL)
    {
    return(FAILURE);
    }

  for (jindex = 0;AQ[jindex] != NULL;jindex++)
    {
    J = AQ[jindex];

    PC = J->TotalProcCount;

    if ((J->StartTime + J->WCLimit) > (mulong)ETime)
      {
      CPS += (double)(ETime - J->StartTime) * PC;     
      }

    IPS += (double)(CTime - ETime) * PC;
    }  /* END for (jindex) */

  if (Value != NULL)
    *Value = IPS - CPS;

  return(SUCCESS);
  }  /* END MQueueGetRequeueValue() */





/**
 * Report config for SCHEDCFG object
 *
 * @see MSchedProcessConfig() - peer - parse SCHEDCFG attributes
 * @see MSchedOConfigShow() - peer - report old-style sched paramters
 *
 * @param S (I)
 * @param ShowVerbose (I)
 * @param String (O)
 */

int MSchedConfigShow(

  msched_t  *S,
  mbool_t    ShowVerbose,
  mstring_t *String)

  {
  int aindex;

  const enum MSchedAttrEnum AList[] = {
    msaFBServer,
    msaChargeMetricPolicy,
    msaChargeRatePolicy,
    msaMaxJobID,
    msaMinJobID,
    msaMaxRecordedCJobID,
    /* msaFBState, */
    msaMode,
    /* msaName, */
    msaServer,
    /* msaState, */
    msaNONE };

  if ((S == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  /* don't clear buffer */

  mstring_t tmpString(MMAX_LINE);

  MStringAppendF(String,"%s[%s]  ",
    MCredCfgParm[mxoSched],
    S->Name);

  for (aindex = 0;AList[aindex] != msaNONE;aindex++)
    {
    if (MSchedAToString(
           S,
           AList[aindex],
           &tmpString,
           mfmVerbose) == FAILURE)
      {
      continue;
      }

    if (MUStrIsEmpty(tmpString.c_str()))
      {
      continue;
      }

    MStringAppendF(String,"%s=%s ",
      MSchedAttr[AList[aindex]],
      tmpString.c_str());
    }    /* END for (aindex) */

  return(SUCCESS);
  }  /* END MSchedConfigShow() */




/**********************************************************************
 *  MSchedDiagDirFileAccess()
 * 
 * This function checks to make sure that the specified file/directory
 * exists and that the permissions allow access to the file/directory.
 *
 * FAILURE is returned if we cannot get the file/directory attributes
 * (most likely the file or directory does not exist in this case) or
 * if the file/directory permissions do not allow Moab write access.
 *  
 **********************************************************************/

int MSchedDiagDirFileAccess(

  char *Filename)  /* I */

  {
  int fuid;
  int fgid;
  int perm;

  if (MFUGetAttributes(Filename,&perm,NULL,NULL,&fuid,&fgid,NULL,NULL) != SUCCESS)
    {
    /* The file or directory must not exist */

    return(FAILURE);
    }

  if (getuid() == 0) 
    {
    /* The user is root so assume that it will have write access */
    }
  else if (perm & S_IWOTH) 
    {
    /* Anyone may write to the file so we are OK */
    }
  else if (((int)getuid() == fuid) && (perm & S_IWGRP)) 
    {
    /* The group for this user has write permissions for the file so we are OK */
    }
  else if (((int)getgid() == fgid) && (perm & S_IWUSR)) 
    {
    /* User owns the file and has write permissions for the file so we are OK */
    }
  else
    {
    /* We have a problem with permissions */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSchedDiagCheckFileAccess() */



/**
 *
 *
 * @param Name (I)
 * @param RIndex (I)
 */

int MSchedAddAdmin(

  char           *Name,   /* I */
  enum MRoleEnum  RIndex) /* I */

  {
  int AIndex;  /* admin authorization level */

  int aindex;

  if ((Name == NULL) || (Name[0] == '\0'))
    {
    return(FAILURE);
    }

  if (!strcasecmp("PEER:",Name))
    {
    return(FAILURE);
    }

  switch (RIndex)
    {
    case mcalAdmin1:

      AIndex = 1;

      break;

    case mcalAdmin2:

      AIndex = 2;

      break;

    case mcalAdmin3:

      AIndex = 3;

      break;

    case mcalAdmin4:

      AIndex = 4;

      break;

    case mcalAdmin5:

      AIndex = 5;

      break;

    default:

      /* not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }    /* END switch (RIndex) */

  for (aindex = 0;aindex < MMAX_ADMINUSERS;aindex++)
    {
    if (!strcmp(Name,MSched.Admin[AIndex].UName[aindex]))
      {
      /* admin already added */

      return(SUCCESS);
      }

    if (MSched.Admin[AIndex].UName[aindex][0] != '\0')
      continue;

    /* add new user/service */

    if (strstr(Name,"PEER:"))
      {
      mgcred_t *U;

      /* update user record for non-peer admin specification */

      if (MUserAdd(Name,&U) == SUCCESS)
        U->Role = RIndex;
      }

    MUStrCpy(
      MSched.Admin[AIndex].UName[aindex],
      Name,
      MMAX_NAME);

    MDB(3,fCONFIG) MLog("INFO:     %s '%s' added\n",
      MRoleType[AIndex],
      Name);

    return(SUCCESS);
    }  /* END for (aindex) */

  /* admin buffer is full */

  MDB(0,fCONFIG) MLog("ALERT:    max admin users (%d) reached\n",
    MMAX_ADMINUSERS);

  return(FAILURE);
  }  /* END MSchedAddAdmin() */



/**
 * Add test/monitor mode operation to iteration's operation list.
 *
 * NOTE:  only called if MSched.Mode==msmTest/msmMonitor
 * NOTE:  use 'mdiag -S -v' to view results
 *
 * @param Action (I)
 * @param OType (I)
 * @param O (I) 
 * @param Msg (I) [optional]
 */

int MSchedAddTestOp(

  enum MRMFuncEnum     Action,
  enum MXMLOTypeEnum   OType,
  void                *O,
  const char          *Msg)

  {
  char tmpLine[MMAX_LINE];

  if (O == NULL)
    {
    return(FAILURE);
    }

  if (MSched.TestModeBuffer == NULL)
    {
    MSched.TestModeBufferSize = MMAX_BUFFER;

    MSched.TestModeBuffer = (char *)MUCalloc(1,MSched.TestModeBufferSize);

    if (MSched.TestModeBuffer == NULL)
      {
      return(FAILURE);
      }

    MUSNInit(
      &MSched.TBPtr,
      &MSched.TBSpace,
      MSched.TestModeBuffer,
      MSched.TestModeBufferSize);
    }

  tmpLine[0] = '\0';

  switch (Action)
    {
    case mrmJobStart:

      {
      int nindex;
      int tindex;

      mnode_t *N;
      mjob_t  *J;

      char tmpBuf[MMAX_BUFFER];

      tmpBuf[0] = '\0';

      J = (mjob_t *)O;

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        for (tindex = 0;tindex < MNLGetTCAtIndex(&J->NodeList,nindex);tindex++)
          {
          if (tmpBuf[0] != '\0')
            MUStrCat(tmpBuf,",",MMAX_BUFFER);

          MUStrCat(tmpBuf,N->Name,MMAX_BUFFER);
          }  /* END for (tindex) */
        }    /* END for (nindex) */

      /* create message */

      if (strlen(tmpBuf) > 512)
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  start job %s on node list %.512s...\n",
          J->Name,
          tmpBuf);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  start job %s on node list %s\n",
          J->Name,
          tmpBuf);
        }
      }  /* END BLOCK */ 

      break;

    case mrmJobCancel:

      {
      mjob_t *J;

      J = (mjob_t *)O;

      if ((Msg != NULL) && (Msg[0] != '\0'))
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  terminate job %s (%s)\n",
          J->Name,
          Msg);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  terminate job %s\n",
          J->Name);
        }
      }    /* END BLOCK */

      break;

    case mrmJobRequeue:

      {
      mjob_t *J;

      J = (mjob_t *)O;

      if ((Msg != NULL) && (Msg[0] != '\0'))
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  requeue job %s (%s)\n",
          J->Name,
          Msg);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  requeue job %s\n",
          J->Name);
        }
      }    /* END BLOCK */

      break;

    case mrmXJobSignal:
    case mrmJobSuspend:
    case mrmJobModify:
    case mrmJobResume:

      {
      mjob_t *J;

      J = (mjob_t *)O;

      if ((Msg != NULL) && (Msg[0] != '\0'))
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  %s on job %s (%s)\n",
          MRMFuncType[Action],
          J->Name,
          Msg);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  %s on job %s - operation details not available\n",
          MRMFuncType[Action],
          J->Name);
        }
      }    /* END BLOCK */

      break;
 
    case mrmNodeModify:

      {
      mnode_t *N;

      N = (mnode_t *)O;

      if ((Msg != NULL) && (Msg[0] != '\0'))
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  modify node %s (%s)\n",
          N->Name,
          Msg);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"Command:  modify node %s - operation details not available\n",
          N->Name);
        }
      }    /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (Action) */

  if (tmpLine[0] != '\0')
    MUSNCat(&MSched.TBPtr,&MSched.TBSpace,tmpLine);

  return(SUCCESS);
  }  /* END MSchedAddTestOp() */




/* update global information about node resource distribution */

/**
 *
 *
 * @param N (I)
 */

int MSchedUpdateCfgRes(

  mnode_t *N)  /* I */

  {
  int aindex;

  int InsertIndex;

  mvpcp_t *VPCProfile;

  /* update memory distribution */

  for (aindex = 0;aindex < MMAX_ATTR;aindex++)
    {
    if (MSched.CfgMem[aindex] < 0)
      {
      /* end of list reached */

      MSched.CfgMem[aindex] = N->CRes.Mem;

      /* terminate list */

      MSched.CfgMem[aindex + 1] = -1;

      break;
      }

    if (MSched.CfgMem[aindex] == N->CRes.Mem)
      {
      /* matching entry located */

      break;
      }

    if (MSched.CfgMem[aindex] > N->CRes.Mem)
      {
      /* insert memory into slot */

      InsertIndex = aindex;

      /* locate end of list */

      for (aindex = InsertIndex;aindex < MMAX_ATTR - 1;aindex++)
        {
        if (MSched.CfgMem[aindex] == 0)
          break;
        }

      /* move end of list out one */

      for (;aindex > InsertIndex;aindex--)
        {
        MSched.CfgMem[aindex] = MSched.CfgMem[aindex - 1];
        }
 
      MSched.CfgMem[InsertIndex] = N->CRes.Mem;

      break;
      }
    }    /* END for (aindex) */

  if (N->CRes.Mem > MSched.MaxCfgMem)
    {
    MSched.MaxCfgMem = N->CRes.Mem;
    }

  VPCProfile = (mvpcp_t *)MUArrayListGet(&MVPCP,0);

  if (VPCProfile == NULL)
    {
    /* only keep track of these if we are doing something with utility computing */

    return(SUCCESS);
    }

  for (aindex = 0;aindex < MMAX_ATTR;aindex++)
    {
    if (MSched.CfgOS[aindex] == 0)
      {
      MSched.CfgOS[aindex] = N->ActiveOS;

      break;
      }

    if (MSched.CfgOS[aindex] == N->ActiveOS)
      {
      break;
      }
    }

  for (aindex = 0;aindex < MMAX_ATTR;aindex++)
    {
    if (MSched.CfgArch[aindex] == 0)
      {
      MSched.CfgArch[aindex] = N->Arch;

      break;
      }

    if (MSched.CfgArch[aindex] == N->Arch)
      {
      break;
      }
    }

  return(SUCCESS);
  }  /* END MSchedUpdateCfgRes() */




#define MDEF_ADDRESSLIST_INCREMENT 25

/**
 *
 *
 * @param HName (I)
 * @param Address (I)
 * @param IList (I/O) [alloc,modified]
 * @param IListSize (I/) [modified]
 */

int MSchedAddToAddressList(

  char         *HName,      /* I */
  char         *Address,    /* I */
  mvaddress_t **IList,      /* I/O (alloc,modified) */
  int          *IListSize)  /* I/0 (modified) */

  {
  int aindex;
  mvaddress_t *List;
  int ListSize;
  
  if ((HName == NULL) ||
      (Address == NULL) ||
      (IList == NULL) ||
      (IListSize == NULL))
    {
    return(FAILURE);
    }

  List = *IList;
  ListSize = *IListSize;

  if (List == NULL)
    {
    List = (mvaddress_t *)MUCalloc(1,sizeof(mvaddress_t) * MDEF_ADDRESSLIST_INCREMENT);
    ListSize = MDEF_ADDRESSLIST_INCREMENT;
    }

  for (aindex = 0;aindex < ListSize;aindex++)
    {
    if (List[aindex].HName[0] == '\0')
      {
      MUStrCpy(List[aindex].HName,HName,MMAX_NAME);
      MUStrCpy(List[aindex].Address,Address,MMAX_NAME);

      List[aindex].IsAvail = TRUE;

      break;
      }
    }

  if (aindex == ListSize)
    {
    mvaddress_t *tmpList;
    
    /* grow list */

    tmpList = (mvaddress_t *)MUCalloc(1,sizeof(mvaddress_t) * (ListSize + MDEF_ADDRESSLIST_INCREMENT));

    memcpy(tmpList,List,sizeof(mvaddress_t) * ListSize);

    MUFree((char **)&List);

    List = tmpList;
    ListSize += MDEF_ADDRESSLIST_INCREMENT;

    MUStrCpy(List[aindex+1].HName,HName,MMAX_NAME);
    MUStrCpy(List[aindex+1].Address,Address,MMAX_NAME);

    List[aindex+1].IsAvail = TRUE;
    }
  
  *IList = List;
  *IListSize = ListSize;
  
  return(SUCCESS);
  }  /* END MSchedAddToAddressList() */



/**
 * Sets the state of the scheduler to a new given State.
 * Can also set the name of the administrator who changed
 * the state. (Use "[internal]" for state changes done by
 * the scheduler itself.)
 *
 * @param State (I)
 * @param Admin (I) [optional]
 */

int MSchedSetState(

  enum MSchedStateEnum  State,
  const char           *Admin)

  {
  const char *FName = "MSchedSetState";

  MDB(5,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    MSchedState[MSched.State],
    MPRINTNULL(Admin));

  MSched.State = State;

  if (Admin != NULL)
    MUStrDup(&MSched.StateMAdmin,Admin);
  else
    MUFree(&MSched.StateMAdmin);

  MSched.StateMTime = MSched.Time;

  return(SUCCESS);
  }  /* END MSchedSetState() */




/**
 *
 *
 * @param EMode (I)
 */

int MSchedSetEmulationMode(

  enum MRMSubTypeEnum EMode) /* I */

  {
  MSched.EmulationMode = EMode;

  switch (EMode)
    {
    case mrmstSLURM:

      {
      mqos_t *Q;

      MSched.QOSIsOptional = TRUE;

      if (MQOSAdd("standby",&Q) == SUCCESS)
        {
        if (Q->F.Priority == 0)
          Q->F.Priority = -1000;

        if (bmisclear(&Q->F.JobFlags))
          {
          bmset(&Q->F.JobFlags,mjfPreemptee);
          }

        if (MSched.DefaultU != NULL)
          MCredSetAttr(
            (void *)MSched.DefaultG,
            mxoGroup,
            mcaQList,
            NULL,
            (void **)"standby",
            NULL,
            mdfString,
            mIncr);

        MDB(3,fWIKI) MLog("INFO:     auto created QOS 'standby' for slurm\n");
        }

      if (MQOSAdd("expedite",&Q) == SUCCESS)
        {
        if (Q->F.Priority == 0)
          Q->F.Priority = 1000;

        if (bmisclear(&Q->F.JobFlags))
          {
          bmset(&Q->F.JobFlags,mjfIgnPolicies);
          bmset(&Q->F.JobFlags,mjfPreemptor);
          }
        }
    
      /* ignore invalid QoS requests w/in SLURM */

      if (bmisclear(&MSched.QOSRejectPolicyBM))
        {
        bmset(&MSched.QOSRejectPolicyBM,mjrpIgnore);

        MDB(7,fWIKI) MLog("INFO:     set qos reject policy to ignore for slurm\n");
        }
      }  /* END BLOCK (case mrmstSLURM) */

      break;

    default:

      /* NO-OP */
   
      break;
    }  /* END switch (EMode) */

  return(SUCCESS);
  }  /* END MSchedSetEmulationMode() */




/**
 * Sets what the scheduler is doing.
 *
 * @param Activity (I)
 * @param AMsg (I) activity message [optional]/
 */

int MSchedSetActivity(

  enum MActivityEnum  Activity, /* I */
  const char         *AMsg)     /* I activity message (optional) */

  {
  mulong now = {0};

  if (MSched.Activity != Activity)
    {
    MUGetTime(&now,mtmNONE,&MSched);

    MSched.ActivityDuration[MSched.Activity] += now - MSched.ActivityTime;

    MSched.Activity = Activity;

    MSched.ActivityTime = (mulong)now;
    }

  MSched.ActivityCount[Activity]++;

  MUStrDup(&MSched.ActivityMsg,AMsg);

  return(SUCCESS);
  }  /* END MSchedSetActivity() */





/**
 * Initialize data structures and parameters before scheduling but after 
 * loading config file.
 *
 * @see main() - parent
 */

int MSchedPreSchedInitialize()

  {
  if (MSched.WattsGMetricIndex == 0)
    MSched.WattsGMetricIndex = MUMAGetIndex(meGMetrics,"watts",mAdd);

  if (MSched.PWattsGMetricIndex == 0)
    MSched.PWattsGMetricIndex = MUMAGetIndex(meGMetrics,"pwatts",mAdd);

  if (MSched.TempGMetricIndex == 0)
    MSched.TempGMetricIndex = MUMAGetIndex(meGMetrics,"temp",mAdd);

  if (MSched.CPUGMetricIndex == 0)
    MSched.CPUGMetricIndex = MUMAGetIndex(meGMetrics,"cpu",mAdd);

  if (MSched.MEMGMetricIndex == 0)
    MSched.MEMGMetricIndex = MUMAGetIndex(meGMetrics,"mem",mAdd);

  if (MSched.IOGMetricIndex == 0)
    MSched.IOGMetricIndex = MUMAGetIndex(meGMetrics,"io",mAdd);

  return(SUCCESS);
  }  /* END MSchedPreSchedInitialize() */


/**
 * Parse a JobRejectPolicy value string and set the appropriate 
 * bit(s) in the supplied bitmap.
 *
 * @param JobRejectPolicyBM (I) 
 * @param SArray            (I)  [modified]
 */

int MSchedProcessJobRejectPolicy(

  mbitmap_t *JobRejectPolicyBM,   
  char	   **SArray)

  {
  int index;
  int pindex;

  char *ptr;
  char *TokPtr;

  bmclear(JobRejectPolicyBM);

  if (SArray == NULL)
    return(SUCCESS);

  for (index = 0;SArray[index] != NULL;index++)
    {
    ptr = MUStrTok(SArray[index],",:;\t\n ",&TokPtr);

    while (ptr != NULL)
      {
      pindex = MUGetIndexCI(ptr,MJobRejectPolicy,FALSE,0);

      ptr = MUStrTok(NULL,",:;\t\n ",&TokPtr);

      if (pindex == 0)
        continue;

      bmset(JobRejectPolicyBM,pindex);
      }  /* END while (ptr != NULL) */
    }    /* END for (index) */

  return(SUCCESS);
  }  /* END MSchedProcesJobRejectPolicy() */



/**
 * Locate requested generic resource on non-compute resource
 *
 */

int MSchedLocateVNode(

  int       GResIndex,
  mnode_t **NP)

  {
  mnode_t *N;

  if (NP != NULL)
    *NP = NULL;

  if (MSched.GN != NULL)
    {
    N = MSched.GN;

    if (MSNLGetIndexCount(&N->CRes.GenericRes,GResIndex) > 0)
      {
      if (NP != NULL)
        *NP = N;

      return(SUCCESS);
      }
    }

  if ((MSched.NetworkRM != NULL) &&
      (MNLGetNodeAtIndex(&MSched.NetworkRM->NL,0,&N) == SUCCESS))
    {
    if (MSNLGetIndexCount(&N->CRes.GenericRes,GResIndex) > 0)
      {
      if (NP != NULL)
        *NP = N;

      return(SUCCESS);
      }
    }

  if ((MSched.DefStorageRM != NULL) &&
      (MNLGetNodeAtIndex(&MSched.DefStorageRM->NL,0,&N) == SUCCESS))
    {
    if (MSNLGetIndexCount(&N->CRes.GenericRes,GResIndex) > 0)
      {
      if (NP != NULL)
        *NP = N;

      return(SUCCESS);
      }
    }

  if ((MSched.InfoRM != NULL) &&
      (MNLGetNodeAtIndex(&MSched.InfoRM->NL,0,&N) == SUCCESS))
    {
    if (MSNLGetIndexCount(&N->CRes.GenericRes,GResIndex) > 0)
      {
      if (NP != NULL)
        *NP = N;

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MSchedLocateVNode() */




/**
 * Get the time when moab was shutdown.
 *  
 * NOTE: only used on startup in DISA environments for now
 */

int MSchedGetLastCPTime()

  {
  char tmpLine[MMAX_LINE + 1];
  char FirstLine[MMAX_LINE << 2];

  long   CkTime = -1;

  FILE  *file = NULL;

  time_t Time;

  struct tm *tp;

  if ((MCP.CPFileName[0] == '\0') || (MCP.CPFileName[0] != '/'))
    {
    return(FAILURE);
    }

  if ((file = fopen(MCP.CPFileName,"r")) == NULL)
    {
    return(FAILURE);
    }

  if (fread(FirstLine,1,sizeof(FirstLine),file) <= 0)
    {
    fclose(file);

    return(FAILURE);
    }

  FirstLine[0] = '\0';
  tmpLine[0]   = '\0';

  sscanf(FirstLine,"%64s %64s %ld %1024s",
    tmpLine,
    tmpLine,
    &CkTime,
    tmpLine);

  fclose(file);

  Time = (time_t)CkTime;

  if ((Time < 0) ||
      (MSched.Time - Time) > MCONST_MONTHLEN)
    {
    return(FAILURE);
    }
 
  tp = localtime(&Time);

  MSched.Day      = tp->tm_yday;
  MSched.DayTime  = tp->tm_hour;
  MSched.WeekDay  = tp->tm_wday;
  MSched.Month    = tp->tm_mon;
  MSched.MonthDay = tp->tm_mday - 1;
  
  return(SUCCESS);
  }



/**
 * Resets counters for priority reservations.
 *
 * Called in MSchedProcessJobs to clear counts where MQueueScheduleIJobs and 
 * MJobPReserve will increment the counts. 
 *
 * @see MSchedProcessJobs
 */

void MSchedResetPriorityRsvCounts()
  {
  memset(MSched.QOSRsvCount,0,sizeof(MSched.QOSRsvCount));
  memset(MSched.ParRsvCount,0,sizeof(MSched.ParRsvCount));
  }

/**
 * Increments the qos bucket counter for priority reservations.
 *
 * @param Bucket Index of QOS to increment qos's reservation count.
 */

void MSchedIncrementQOSRsvCount(int Bucket)
  {
  assert((Bucket >= 0) && (Bucket < MMAX_QOS));

  MSched.QOSRsvCount[Bucket]++;
  }

/**
 * Gets the priority reservation count for the given bucket.
 *
 * @param Bucket Index QOS to get current qos reservation count.
 */

int MSchedGetQOSRsvCount(int Bucket)
  {
  assert((Bucket >= 0) && (Bucket < MMAX_QOS));

  return(MSched.QOSRsvCount[Bucket]);
  }

/**
 * Increments the partition bucket counter for priority reservations for each partition.
 *
 * @param Bucket Index of partition to increment partition's reservation count.
 */

void MSchedIncrementParRsvCount(int Bucket)
  {
  assert((Bucket >= 0) && (Bucket < MMAX_PAR));

  MSched.ParRsvCount[Bucket]++;
  }

/**
 * Gets the partition's priority reservation count for the given bucket.
 *
 * @param Bucket Index of partition to get current reservation count for partition.
 */

int MSchedGetParRsvCount(int Bucket)
  {
  assert((Bucket >= 0) && (Bucket < MMAX_PAR));

  return(MSched.ParRsvCount[Bucket]);
  }

/* END MSched.c */
