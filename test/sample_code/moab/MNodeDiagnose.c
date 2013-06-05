/* HEADER */

      
/**
 * @file MNodeDiagnose.c
 *
 * Contains: Node Diagnose functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param N (I)
 * @param Buffer (O) (already initialized) 
 */

int MNodeDiagnoseState(

  mnode_t   *N,
  mstring_t *Buffer)

  {
  char *tBPtr;
  int   tBSpace;

  int     rindex;
  int     jindex;

  mrm_t  *R;

  mjob_t *J;

  mbool_t NodeAllocated;

  mulong  MinStartTime;

  char    tmpLine[MMAX_LINE];

  const char *FName = "MNodeDiagnoseState";

  MDB(5,fSTRUCT) MLog("%s(%s,Buffer)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if ((N == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  /* verify node information is not stale */

  if (((long)MSched.Time - N->ATime) > 600)
    {
    char TString[MMAX_LINE];

    MULToTString((long)MSched.Time - N->ATime,TString);

    MStringAppendF(Buffer,"ALERT:  node has not been updated in %s\n",
      TString);
    }

  /* display job allocation information */

  /* NOTE:  switch to use N->JList array (NYI) */

  NodeAllocated = FALSE;
  MinStartTime  = MMAX_TIME;

  MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

  if (N->JList != NULL)
    {
    for (jindex = 0;jindex < N->MaxJCount;jindex++)
      {
      J = N->JList[jindex];

      if (J == NULL)
        break;

      if (J == (mjob_t *)1)
        continue;

      if (tmpLine[0] != '\0')
        MUSNCat(&tBPtr,&tBSpace,",");

      MUSNCat(&tBPtr,&tBSpace,J->Name);

      if (J->State == mjsSuspended)
        MUSNCat(&tBPtr,&tBSpace,"(S)");

      NodeAllocated = TRUE;

      MinStartTime = MIN(MinStartTime,J->StartTime);
      }  /* END for (jindex) */
    }    /* END if (N->JList != NULL) */

  if (NodeAllocated == TRUE)
    {
    MStringAppendF(Buffer,"Jobs:        %s\n",
      tmpLine);
    }

  /* evaluate node state */

  if ((MNODEISACTIVE(N) == TRUE) && (NodeAllocated == FALSE))
    {
    char TString[MMAX_LINE];

    MULToTString((long)MSched.Time - MinStartTime,TString);

    MStringAppendF(Buffer,"ALERT:  no jobs active on node for %s but state is %s\n",
      (MinStartTime != MMAX_TIME) ? TString : "INFINITY",
      MNodeState[N->State]);
    }
  else if ((MNODEISACTIVE(N) == FALSE) && (NodeAllocated == TRUE)) 
    {
    MStringAppendF(Buffer,"ALERT:  jobs active on node but state is %s\n",
      MNodeState[N->State]);
    }

  /* evaluate utilized resources */

  if (MNODEISACTIVE(N) == TRUE)
    {
    /* check low thresholds */

    if (N->State == mnsBusy)
      {
      if ((N->Load) / MAX(1,N->CRes.Procs) < 0.2)
        {
        MStringAppendF(Buffer,"ALERT:  node is in state %s but load is low (%0.3f)\n",
          MNodeState[N->State],
          N->Load);
        }
      }
    else if (N->DRes.Procs > 0)
      {
      if ((N->Load) / N->DRes.Procs < 0.2)
        {
        MStringAppendF(Buffer,"ALERT:  node has %d procs dedicated but load is low (%0.3f)\n",
          N->DRes.Procs,
          N->Load);
        }
      }

    /* check high thresholds */

    /* NYI */
    }  /* END if (MNODEISACTIVE(N) == TRUE) */
  else
    {
    /* check high thresholds */

    if ((N->Load / MAX(1,N->CRes.Procs)) > 0.5)
      {
      MStringAppendF(Buffer,"ALERT:  node is in state %s but load is high (%0.3f)\n",
        MNodeState[N->State],
        N->Load);
      }
    }      /* END else (MNODEISACTIVE(N) == TRUE) */

  if (MSched.ProvRM != NULL)
    {
    enum MNodeStateEnum State = mnsNONE;

    /* run MSM specific diagnostics */

    MNodeGetRMState(N,mrmrtCompute,&State);
    
    if ((State == mnsDown) && (N->PowerState == mpowOn))
      {
      MStringAppendF(Buffer,"ALERT:  node is in state Down but power is ON, provisioning or power failure?\n");
      }
    }

  /* evaluate dedicated resources */

  if ((N->DRes.Mem > N->CRes.Mem) || (N->ARes.Mem < 0))
    {
    MStringAppendF(Buffer,"ALERT:  node has memory overcommitted\n");
    }

  if ((N->DRes.Swap > N->CRes.Swap) || (N->ARes.Swap < 0))
    {
    MStringAppendF(Buffer,"ALERT:  node has swap overcommitted\n");
    }

  if ((N->DRes.Disk > N->CRes.Disk) || (N->ARes.Disk < 0))
    {
    MStringAppendF(Buffer,"ALERT:  node has disk overcommitted\n");
    }

  if ((N->RackIndex != -1) &&
      (MRack[N->RackIndex].Memory > 0) &&
      (N->CRes.Mem != MRack[N->RackIndex].Memory))
    {
    MStringAppendF(Buffer,"ALERT:  node memory does not match expected memory (%d != %d)\n",
      N->CRes.Mem,
      MRack[N->RackIndex].Memory);
    }

  if (N->Message != NULL)
    {
    MStringAppendF(Buffer,"NOTE:    node message '%s'\n",
      N->Message);
    }  /* END if (N->Message != NULL) */

  for (rindex = 0;rindex < MSched.M[mxoRM];rindex++)
    {
    R = &MRM[rindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (N->RMMsg[rindex] == NULL)
      continue;

    MStringAppendF(Buffer,"RM[%s] '%s'\n",
      R->Name,
      N->RMMsg[rindex]);
    }  /* END for (rindex) */

  return(SUCCESS);
  }  /* END MNodeDiagnoseState() */




/**
 * Diagnose rsv health/config on specified node.
 *
 * @see MUINodeShow() - parent
 *
 * @param N (I)
 * @param Flags (I) [bitmap of enum MCModeEnum]
 * @param Buffer (O) [appended, not initialized]
 */

int MNodeDiagnoseRsv(

  mnode_t   *N,      /* I */
  mbitmap_t *Flags,  /* I (bitmap of enum MCModeEnum) */
  mstring_t *Buffer) /* I (already initialized) */

  {
  mre_t *RE;

  long    ETime;

  mcres_t NDRes;    /* non-dedicated resources */

  int     TC;

  mrsv_t *R;
  mjob_t *J;

  const char *FName = "MNodeDiagnoseRsv";

  MDB(5,fSTRUCT) MLog("%s(%s,Flags,Buffer)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if ((N == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  /* evaluate reserved resources */

  ETime = 0;

  MCResInit(&NDRes);

  MCResCopy(&NDRes,&N->CRes);

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    TC = RE->TC;

    if (RE->Time > ETime)
      {
      if ((MCResIsNeg(&NDRes,NULL,NULL) == TRUE) && (RE->Type == mreStart))
        {
        char TString[MMAX_LINE];

        MULToTString(ETime - MSched.Time,TString);

        MStringAppendF(Buffer,"  WARNING:  node '%s' is overcommitted at time %s (P: %d)\n",
          N->Name,
          TString,
          NDRes.Procs);
        }

      ETime = RE->Time;
      }

    if (RE->Type == mreStart)
      {
      if (R->Type == mrtJob)
        {
        J = R->J;

        /* NOTE:  must select req associated with node */
 
        if (J == NULL)
          {
          /* NO-OP */
          }
        else if (J->Req[1] == NULL)
          {
          MCResRemove(&NDRes,&N->CRes,&J->Req[0]->DRes,TC,FALSE);
          }
        else
          {
          int rqindex;

          for (rqindex = 0;rqindex < MSched.M[mxoReq];rqindex++)
            {
            if (MNodeGetTC(
                  N,
                  &N->ARes,
                  &N->CRes,
                  &N->DRes,
                  &J->Req[rqindex]->DRes,
                  MMAX_TIME,
                  1,
                  NULL) > 0)
              {
              MCResRemove(&NDRes,&N->CRes,&J->Req[rqindex]->DRes,TC,FALSE);

              break;
              }
            }
          }
        }
      else
        {
        MCResRemove(&NDRes,&N->CRes,&RE->BRes,1,FALSE);
        }
      }
    else if (RE->Type == mreEnd)
      {
      if (R->Type == mrtJob)
        {
        J = R->J;

        if (J == NULL)
          {
          /* NO-OP */
          }
        else if (J->Req[1] == NULL)
          {
          MCResAdd(&NDRes,&N->CRes,&J->Req[0]->DRes,TC,FALSE);
          }
        else
          {
          int rqindex;

          for (rqindex = 0;rqindex < MSched.M[mxoReq];rqindex++)
            {
            if (MNodeGetTC(
                  N,
                  &N->ARes,
                  &N->CRes,
                  &N->DRes,
                  &J->Req[rqindex]->DRes,
                  MMAX_TIME,
                  1,
                  NULL) > 0)
              {
              MCResAdd(&NDRes,&N->CRes,&J->Req[rqindex]->DRes,TC,FALSE);

              break;
              }
            }
          }
        }
      else
        {
        MCResAdd(&NDRes,&N->CRes,&RE->BRes,1,FALSE);
        }
      }
    }    /* END for (reindex) */

  MCResFree(&NDRes);

  return(SUCCESS);
  }  /* END MNodeDiagnoseRsv() */





/**
 * Report node eligibility for a given req
 *
 * @see MJobDiagnoseEligibility() - parent
 * @see MReqCheckNRes() - child
 *
 * @param N (I)
 * @param J (I)
 * @param RQ (I)
 * @param RIndexP (O) [optional]
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 * @param StartTime (I)
 * @param TCP (O) [optional]
 * @param IPC (O) idle proc count [optional]
 */

int MNodeDiagnoseEligibility(

  mnode_t              *N,         /* I */
  mjob_t               *J,         /* I */
  mreq_t               *RQ,        /* I */
  enum MAllocRejEnum   *RIndexP,   /* O (optional) */
  char                 *Msg,       /* O (optional,minsize=MMAX_LINE) */
  mulong                StartTime, /* I */
  int                  *TCP,       /* O (optional) */
  int                  *IPC)       /* O (optional) idle proc count */

  {
  int tmpI;

  char Affinity;

  static double MinSpeed = 1.0;

  if (RIndexP != NULL)
    *RIndexP = marNONE;

  if (TCP != NULL)
    *TCP = 0;

  if (Msg != NULL)
    Msg[0] = '\0';

#ifndef MOPT

  if ((N == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

#endif /* MOPT */

  if ((StartTime <= MSched.Time + MCONST_HOURLEN) &&
    (((N->State != mnsIdle) && (N->State != mnsActive)) ||
     ((N->EState != mnsIdle) && (N->EState != mnsActive))))
    {
    /* only show state issues if asking for near-term resource availability */

    if (RIndexP != NULL)
      *RIndexP = marState;

    if (Msg != NULL)
      MUStrCpy(Msg,(char *)MNodeState[N->State],MMAX_NAME);
    
    return(FAILURE);
    }

  if (bmisset(&J->IFlags,mjifHostList) &&
     (!MNLIsEmpty(&J->ReqHList)) &&
     (J->HLIndex == RQ->Index) &&
     (J->ReqHLMode != mhlmSubset))
    {
    mnode_t *tmpN;

    int hlindex;

    /* check hostlist constraints */

    for (hlindex = 0;MNLGetNodeAtIndex(&J->ReqHList,hlindex,&tmpN) == SUCCESS;hlindex++)
      {
      if (tmpN == N)
        break;
      }  /* END for (hlindex) */

    if (MNLGetNodeAtIndex(&J->ReqHList,hlindex,NULL) == FAILURE)
      { 
      if (RIndexP != NULL)
        *RIndexP = marHostList;

      return(FAILURE);
      }
    }

  if (N->ACL != NULL)
    {
    mbool_t IsInclusive = FALSE;

    MACLCheckAccess(N->ACL,J->Credential.CL,NULL,NULL,&IsInclusive);

    if (IsInclusive == FALSE)
      {
      if (RIndexP != NULL)
        *RIndexP = marACL;

      return(FAILURE);
      }
    }  /* end if (N->ACL != NULL) */

  /* examine other access constraints */

  if (IPC != NULL)
    *IPC += N->ARes.Procs;

  if (MReqCheckNRes(
       J,
       N,
       RQ,
       StartTime,
       TCP,        /* O */
       MinSpeed,
       0,
       RIndexP,    /* O */
       &Affinity,  /* O */
       NULL,
       TRUE,  /* do feasibility check */
       FALSE,
       Msg) == FAILURE)  /* O */
    {
    if (TCP == NULL)
      {
      return(FAILURE);
      }
    }

  if ((TCP != NULL) && (*TCP == 0))
    {
    return(FAILURE);
    }

  tmpI = (N->CRes.Procs != 0) ? N->CRes.Procs : 9999;

  if (MNodeCheckPolicies(J,N,MSched.Time,&tmpI,Msg) == FAILURE)
    {
    if (RIndexP != NULL)
      *RIndexP = marNodePolicy;

    if ((Msg != NULL) && (Msg[0] == '\0'))
      sprintf(Msg,"violates node policy");

    return(FAILURE);
    }

  if ((RQ->TasksPerNode > 1) && (tmpI < RQ->TasksPerNode))
    {
    if (RIndexP != NULL)
      *RIndexP = marTaskCount;

    if ((Msg != NULL) && (Msg[0] == '\0'))
      sprintf(Msg,"cannot satisfy TPN");

    return(FAILURE);
    }
 
  if (TCP != NULL)
    *TCP = MIN(*TCP,tmpI);

  return(SUCCESS);
  }  /* END MNodeDiagnoseEligibility() */
/* END MNodeDiagnose.c */
