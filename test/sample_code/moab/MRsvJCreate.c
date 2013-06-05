/* HEADER */

      
/**
 * @file MRsvJCreate.c
 *
 * Contains: MRsvJCreate
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Create a job reservation at a specified time on the specified node list.
 *
 * NOTE:  Rsv incorporates attributes RQ->DRes and J->SpecWCLimit[0] (scaled)
 *
 * NOTE:  NODEACCESSPOLICY enforced by setting R->DRes.Procs == -1 for
 *        singlejob/singletask policies.  Additional enforcement 
 *        enabled w/in MJobGetSNRange()
 *
 * NOTE:  dedicated VM usage enforced where???
 *
 * @see MJobReserve() - parent - create idle job rsv
 * @see MRMJobPostLoad()/MRMJobPostUpdate() - parent - create active job rsv
 * @see MRsvCreate() - create non-job reservation
 *
 * @see MJobReleaseRsv() - child
 * @see MRsvInitialize() - child
 * @see MREInsert() - child - insert RE event in global table
 * @see MRMRsvCtl() - child - export rsv to peer service
 * @see MRsvAdjustNode() - child - update node rsv info
 * @see MRSvAdjustDRes() - child - called only on initial load
 *
 * @param J (I) job reserving nodes
 * @param MNodeList (I) [optional]
 * @param StartTime (I) time reservation starts
 * @param RsvSType (I) reservation subtype
 * @param RP (O) [optional]
 * @param NoRsvTransition (I)
 */

#define MNODESPEEDNOTSET 9999997.0

int MRsvJCreate(

  mjob_t                 *J,            /* I  job reserving nodes */
  mnl_t                 **MNodeList,    /* I  nodes to be reserved (optional) */
  long                    StartTime,    /* I  time reservation starts */
  enum MJRsvSubTypeEnum   RsvSType,     /* I  reservation subtype  */
  mrsv_t                **RP,           /* O  reservation pointer (optional) */
  mbool_t                 NoRsvTransition) /* I  don't transition */

  {
  int   rqindex;
  int   rqindex2;

  int   nindex;

  mrsv_t  *R;
  mnode_t *N;
  mreq_t  *RQ;

  int     RC;

  double  NPFactor;

  int     TaskCount;

  const char *FName = "MRsvJCreate";

  char TString[MMAX_LINE];

  MULToTString(StartTime - MSched.Time,TString);

  MDB(3,fSTRUCT) MLog("%s(%s,MNodeList,%s,%s,RP)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    TString,
    MJRType[RsvSType]);

  if ((J == NULL) || (J->Req[0] == NULL))
    {
    MDB(1,fSTRUCT) MLog("ERROR:    invalid job passed to %s()\n",
      FName);

    return(FAILURE);
    }

  if (bmisset(&J->Flags,mjfRsvMap))
    {
    return(SUCCESS);
    }

  if (J->Rsv != NULL)
    {
    MDB(0,fSTRUCT) MLog("ERROR:    reservation created for reserved job '%s' (existing reservation '%s' deleted)\n",
      J->Name,
      J->Rsv->Name);

    MJobReleaseRsv(J,TRUE,FALSE);
    }

  rqindex2 = 0; /* So valgrind won't complain */

  /* NOTE:  must create per req rsv if task resource definitions differ */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    R = NULL;

    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    /* verify nodelist */

    if ((MNodeList == NULL) &&
        ((RQ == NULL) || (MNLIsEmpty(&RQ->NodeList))))
      {
      MDB(1,fSTRUCT) MLog("ERROR:    invalid nodelist passed to %s()\n",
        FName);

      return(FAILURE);
      }

    MRsvInitialize(&R);

    R->HostExpIsSpecified = TRUE;

    R->CTime = MSched.Time;

    R->CIteration = MSched.Iteration;

    if (rqindex == 0)
      {
      /* NOTE:  returned rsv pointer and J->R only point to req 0 rsv */

      if (RP != NULL)
        *RP = R;

      /* link job to reservation */

      J->Rsv = R;
      }  /* END if (rqindex == 0) */

    RQ->R = R;

    R->J    = J;  /* all req reservations point to J */
    R->Mode = 0;

    /* build reservation */

    if (rqindex > 0)
      {
      snprintf(R->Name,MMAX_NAME,"%s.%d",
        J->Name,
        rqindex);
      }
    else
      {
      MUStrCpy(R->Name,J->Name,MMAX_NAME);
      }

    MRsvTableInsert(R->Name,R);

    if (bmisset(&J->Flags,mjfMeta) &&
        bmisset(&J->Flags,mjfSystemJob) &&
        bmisset(&J->Flags,mjfNoRMStart))
      {
      R->Type = mrtMeta;
      R->SubType = mrsvstMeta;
      }
    else
      {
      R->Type    = mrtJob;
      R->SubType = mrsvstJob;
      }

    /* get partition number from first node */

    R->StartTime = StartTime;

    NPFactor = MNODESPEEDNOTSET;

    /* NOTE:  all rsv's of multi-req job must be scaled by same factor resulting in same duration */

    if (MNodeList != NULL)
      {
      double tmpD;

      /* get partition number from first node */

      if (MNLGetNodeAtIndex(MNodeList[rqindex],0,&N) == SUCCESS)
        {
        R->PtIndex = N->PtIndex;

        MDB(7,fSTRUCT) MLog("INFO:     job '%s' reservation partition set to %s from first node in node list\n",
          J->Name,
          MPar[R->PtIndex].Name);
        }
      else
        {
        R->PtIndex = 0;

        MDB(7,fSTRUCT) MLog("INFO:     job '%s' reservation partition set to %s since node in nodelist is NULL\n",
          J->Name,
          MPar[R->PtIndex].Name);
        }

      for (rqindex2 = 0;MNLGetNodeAtIndex(MNodeList[rqindex2],0,&N) == SUCCESS;rqindex2++)
        {
        if (MUNLGetMinAVal(
             &MNodeList[rqindex2][0],
             mnaSpeed,
             NULL,
             (void **)&tmpD) == SUCCESS)
          {
          NPFactor = MIN(NPFactor,tmpD);
          }
        }  /* END for (rqindex2) */

      if (NPFactor == MNODESPEEDNOTSET)
        {
        /* speed not set */

        NPFactor = 1.0;
        }
      }    /* END if (MNodeList != NULL) */
    else
      {
      double  tmpD;
      mreq_t *tRQ;

      /* get partition number from first node */

      if (MNLGetNodeAtIndex(&RQ->NodeList,0,&N) == SUCCESS)
        {
        R->PtIndex = N->PtIndex;

        MDB(7,fSTRUCT) MLog("INFO:     job '%s' reservation partition set to %s from first node in requested node list\n",
          J->Name,
          MPar[R->PtIndex].Name);
        }
      else
        {
        R->PtIndex = MCONST_DEFAULTPARINDEX;

        MDB(7,fSTRUCT) MLog("INFO:     job '%s' reservation partition set to %s since first node in requested node list is NULL\n",
          J->Name,
          MPar[R->PtIndex].Name);
        }

      for (rqindex2 = 0;J->Req[rqindex2] != NULL;rqindex2++)
        {
        tRQ = J->Req[rqindex2];

        if (!MNLIsEmpty(&tRQ->NodeList))
          {
          if (MUNLGetMinAVal(
              &tRQ->NodeList,
              mnaSpeed,
              NULL,
              (void **)&tmpD) == FAILURE)
            {
            continue;
            }

          NPFactor = MIN(NPFactor,tmpD);
          }
        }  /* END for (rqindex2) */
      }    /* END else (MNodeList != NULL) */

    /* record rsv nodelist */

    if (!MNLIsEmpty(&RQ->NodeList))
      {
      MNLCopy(&R->NL,&RQ->NodeList);
      }
    else if (MNodeList != NULL)
      {
      /* NOTE: idle jobs with rsv will not have RQ->NodeList populated */

      /* NOTE:  R->NL may be to small if J allocation is dynamic */

      MNLCopy(&R->NL,MNodeList[RQ->Index]);
      } 

    if ((NPFactor <= 0.0001) || (NPFactor >= MNODESPEEDNOTSET - 1))
      {
      NPFactor = 1.0;
      }

    R->EndTime = MAX(
                   StartTime + (long)((double)J->SpecWCLimit[0] / NPFactor),
                   (long)MSched.Time + 1);

    R->EndTime = MIN(R->EndTime,MMAX_TIME - 1);

    if (J->SubState == mjsstPreempted)
      {
      /* NOTE:  Job was just preempted -- preempted resources will not be 
                available to preemptor this iteration.  Maintain reservation 
                for MSched.JobRsvRetryInterval to prevent preemptor from using 
                resources immediately. This will enable him to create a 
                reservation in the future. */

      R->EndTime = MIN(R->EndTime,MSched.Time + MSched.JobRsvRetryInterval);
      }

    if (StartTime == 0)
      {
      MDB(0,fSTRUCT) MLog("ERROR:    invalid StartTime specified for reservation on job rsv '%s'\n",
        R->Name);

      StartTime = 1;
      }

    /* link nodes to reservation */

    MDB(6,fSTRUCT) MLog("INFO:     linking nodes to reservation '%s'\n",
      R->Name);

    R->AllocNC = 0;
    R->AllocTC = 0;
    R->AllocPC = MReqGetPC(J,RQ);

    /* copy ACL/CL info */

    if (!MACLIsEmpty(J->Credential.ACL))
      {
      MACLCopy(&R->ACL,J->Credential.ACL);
      }
    else
      {
      MACLFreeList(&R->ACL);
      }

    if (!MACLIsEmpty(J->Credential.CL))
      {
      MACLCopy(&R->CL,J->Credential.CL);
      }
    else
      {
      MACLFreeList(&R->CL);
      }

    MACLSet(&R->ACL,maJob,J->Name,mcmpSEQ,mnmNeutralAffinity,0,0);

    /* copy DRes info */

    MCResCopy(&R->DRes,&RQ->DRes);

    /* make each job a member of an rsv group with the job name */

    MUStrDup(&R->RsvGroup,J->Name);

    /* if this is a remote req must export reservation */

    if ((RQ->RemoteR == NULL) &&
        (MNLGetNodeAtIndex(&R->NL,0,&N) == SUCCESS) &&
        (N->RM != NULL) &&
        (N->RM->Type == mrmtMoab) &&
        bmisset(&N->RM->Flags,mrmfRsvExport))
      {
      char tmpRsvName[MMAX_NAME];

      enum MStatusCodeEnum SC;

      if (MRMRsvCtl(
            R,
            N->RM,
            mrcmCreate,
            (void **)tmpRsvName,
            NULL,
            &SC) == SUCCESS)
        {
        MOAddRemoteRsv((void *)R,mxoRsv,N->RM,tmpRsvName);
        MOAddRemoteRsv((void *)RQ,mxoReq,N->RM,tmpRsvName);
        }
      }
    else if ((RQ->RemoteR != NULL) &&
             (MNLGetNodeAtIndex(&R->NL,0,&N) == SUCCESS) && 
             (N->RM != NULL) &&
             (N->RM->Type == mrmtMoab) &&
             bmisset(&N->RM->Flags,mrmfRsvExport))
      {
      MOAddRemoteRsv((void *)R,mxoRsv,N->RM,RQ->RemoteR->Name);
      }
    }  /* END for (rqindex) */

  nindex  = 0;
  rqindex = 0;

  while (1)
    {
    RQ = J->Req[rqindex];

    /* NYI : must handle class ACL management for multi-req jobs */

    if (MNodeList == NULL)
      {
      if ((RQ == NULL) ||
          (MNLIsEmpty(&RQ->NodeList)) ||
          (rqindex >= MMAX_REQ_PER_JOB))
        {
        break;
        }

      if ((MNLGetNodeAtIndex(&RQ->NodeList,nindex,NULL) == FAILURE) ||
          (MNLGetTCAtIndex(&RQ->NodeList,nindex) == 0))
        {
        rqindex++;

        nindex = 0;

        continue;
        }

      MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N);
      TaskCount = MNLGetTCAtIndex(&RQ->NodeList,nindex);

      nindex++;
      }
    else
      {
      if ((RQ == NULL) ||
          (rqindex >= MMAX_REQ_PER_JOB))
        {
        break;
        }

      if ((MNLGetNodeAtIndex(MNodeList[rqindex],0,NULL) == FAILURE) ||
          (MNLGetTCAtIndex(MNodeList[rqindex],0) == 0))
        {
        break;
        }

      if ((MNLGetNodeAtIndex(MNodeList[rqindex],nindex,NULL) == FAILURE) ||
          (MNLGetTCAtIndex(MNodeList[rqindex],nindex) == 0))
        {
        rqindex++;

        nindex = 0;

        continue;
        }

      MNLGetNodeAtIndex(MNodeList[rqindex],nindex,&N);
      TaskCount = MNLGetTCAtIndex(MNodeList[rqindex],nindex);

      nindex++;
      }  /* END else (MNodeList == NULL) */

    if ((N->Name[0] == '\0') || (N->Name[0] == '\1'))
      {
      MDB(0,fSTRUCT) MLog("ERROR:    job '%s' rsv nodelist contains invalid node at index %d\n",
        J->Name,
        nindex - 1);

      MJobReleaseRsv(J,FALSE,FALSE);

      return(FAILURE);
      }      /* END if ((N->Name[0] == '\0') || (N->Name[0] == '\1')) */

    if ((N->PtIndex != RQ->R->PtIndex) &&
        (N->PtIndex != 0) &&
        !bmisset(&J->Flags,mjfCoAlloc))
      {
      MDB(0,fSTRUCT) MLog("ERROR:    job '%s' reservation spans partitions (node %s: %d)\n",
        J->Name,
        N->Name,
        N->PtIndex);

      MJobReleaseRsv(J,FALSE,FALSE);

      return(FAILURE);
      }

    if ((RQ->NAccessPolicy == mnacSingleJob) ||
        (RQ->NAccessPolicy == mnacSingleTask))
      {
      /* node is dedicated - allocate all node procs */

      MCResClear(&RQ->R->DRes);

      RQ->R->DRes.Procs = -1;

      RC = 1;
      }
    else if (MJOBREQUIRESVMS(J))
      {
      mvm_t *V;

      /* locate associated VM */

      if (MJobFindNodeVM(J,N,&V) == SUCCESS)
        {
        MCResCopy(&RQ->R->DRes,&V->CRes);
        }
      else
        {
        /* cannot locate associated VM - use job's DRes */

        MCResCopy(&RQ->R->DRes,&RQ->DRes);
        }

      RC = 1;
      }
    else
      {
      /* normal job - use job's DRes */

      MCResCopy(&RQ->R->DRes,&RQ->DRes);

      RC = TaskCount;
      }

    RQ->R->AllocNC ++;
    RQ->R->AllocTC += RC;

    /* RC is always a positive number */

    if (MRsvAdjustNode(RQ->R,N,RC,0,FALSE) == FAILURE)
      {
      MJobReleaseRsv(J,FALSE,FALSE);

      return(FAILURE);
      }

    if (RQ->R->StartTime > MSched.Time)
      {
      MUStrDup(&RQ->R->RsvGroup,"idle");
      }
    }     /* END while (1) */

  J->RType = RsvSType;

  if (!NoRsvTransition)
    MRsvTransition(R,FALSE);

#if 0
  MSysJobUpdateRsvFlags(J);
#endif

  MRsvTableDumpToLog(__FUNCTION__,__LINE__);

  return(SUCCESS);
  }  /* END MRsvJCreate() */

/* END MRsvJCreate.c */
