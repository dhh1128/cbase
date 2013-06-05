/* HEADER */

      
/**
 * @file MRsvCRUD.c
 *
 * Contains: Rsv  Create/Read/Update/Delete functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

/**
 *
 * NOTE:  does not guarantee that 'RBaseName' is unique
 *
 * @param Type      (I)
 * @param ACL       (I)
 * @param CL        (I) [optional]
 * @param Flags     (I) [bitmap of enum MRsvFlagsEnum]
 * @param NodeList  (I) [optional,minsize=X]
 * @param StartTime (I)
 * @param EndTime   (I)
 * @param NC        (I) nodes requested, 0 means no allocation [optional]
 * @param PC        (I) procs requested [optiona]
 * @param RBaseName (I)
 * @param RsvP      (O)
 * @param HostExp   (I)
 * @param DRes      (I) [optional]
 * @param EMsg      (O) [optional,minsize=MMAX_LINE]
 * @param DoRemote  (I)
 * @param DoAlloc   (I)
 */

int MRsvCreate(

  enum MRsvTypeEnum  Type,
  macl_t            *ACL,
  macl_t            *CL,
  mbitmap_t         *Flags,
  mnl_t             *NodeList,
  long               StartTime,
  long               EndTime,
  int                NC,
  int                PC,
  char              *RBaseName,
  mrsv_t           **RsvP,
  char              *HostExp,
  mcres_t           *DRes,
  char              *EMsg,
  mbool_t            DoRemote,
  mbool_t            DoAlloc)

  {
  mrsv_t *R = NULL;

  mcres_t *tmpDRes;

  char    Message[MMAX_LINE];

  mrm_t  *RM = NULL;

  const char *FName = "MRsvCreate";

  MDB(3,fSTRUCT) MLog("%s(%s,ACL,CL,Flags,NodeList,%ld,%ld,%d,%d,%s,RsvP,'%s',DRes,EMsg)\n",
    FName,
    MRsvType[Type],
    StartTime,
    EndTime,
    NC,
    PC,
    RBaseName,
    (HostExp != NULL) ? HostExp : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (RsvP != NULL)
    *RsvP = NULL;

  /* perform sanity check */

  if ((StartTime >= EndTime) || ((long)MSched.Time >= EndTime) || (Flags == NULL))
    {
    /* invalid timeframe */

    if (EMsg != NULL)
      strcpy(EMsg,"invalid timeframe requested");

    return(FAILURE);
    }

  if (MSched.Time > MCONST_EFFINF)
    {
    StartTime = MAX(StartTime,(long)MSched.Time - MCONST_EFFINF);
    }

  MRsvInitialize(&R);

  if (DRes == NULL)
    tmpDRes = &R->DRes;
  else
    tmpDRes = DRes;

  MACLSet(&R->CL,maRsv,RBaseName,mcmpSEQ,mnmNeutralAffinity,0,0);

  if (MACLGet(ACL,maRsv,NULL) != SUCCESS)
    MACLSet(&R->ACL,maRsv,RBaseName,mcmpSEQ,mnmNeutralAffinity,0,0);

  MUStrCpy(R->Name,RBaseName,MMAX_NAME);

  MRsvTableInsert(R->Name,R);

  MDB(3,fSTRUCT) MLog("INFO:     creating new rsv - MRsv '%s'\n",
    R->Name);

  if ((!MACLIsEmpty(ACL)) && (MRsvSetAttr(R,mraACL,(void *)ACL,mdfOther,mAdd)) == FAILURE)
    {
    char tmpLine[MMAX_LINE];

    /* reservation will be created with bad ACL */

    snprintf(tmpLine,sizeof(tmpLine),"WARNING: could not create complete ACL list\n");

    MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
    }

  /* Setup the reservation Partition Index based upon the node partition index */

  if ((NodeList != NULL ) && (!MNLIsEmpty(NodeList)))
    {
    mnode_t *tmpN;

    MNLGetNodeAtIndex(NodeList,0,&tmpN);

    R->PtIndex = tmpN->PtIndex;

    RM = tmpN->RM;
    }
  else
    {
    R->PtIndex = -1;
    }

  if (CL != NULL)
    {
    macl_t *tmpACL = R->CL;

    MRsvSetAttr(R,mraCL,(void *)CL,mdfOther,mAdd);

    while (tmpACL != NULL)
      {
      switch (tmpACL->Type)
        {
        case maAcct:

          /* check allocation */

          MAcctAdd(tmpACL->Name,&R->A);

          break;

        case maQOS:

          MQOSFind(tmpACL->Name,&R->Q);

          break;

        case maUser:

          MUserFind(tmpACL->Name,&R->U);

          break;

        case maGroup:

          MGroupAdd(tmpACL->Name,&R->G);

          break;

        case maClass:
        default:

          /* NO-OP */

          break;
        }  /* END switch (tmpACL->Type) */

      tmpACL = tmpACL->Next;
      }    /* END while (tmpACL != NULL) */
    }      /* END if (CL != NULL) */

  MRsvSetAttr(R,mraType,(void **)&Type,mdfInt,mSet);

  R->StartTime = StartTime;
  R->EndTime   = MIN(EndTime,MMAX_TIME - 1);

  MDB(3,fSTRUCT) MLog("INFO:     event table updated for rsv MRsv '%s'\n",
    R->Name);

  /* NOTE:  all rsv's have 'self' in ACL */

  if (MACLCount(R->ACL) == 1)
    bmset(Flags,mrfIsClosed);

  if ((HostExp != NULL) && !strcmp(HostExp,"ALL"))
    bmset(Flags,mrfIsGlobal);

  MRsvSetAttr(R,mraFlags,Flags,mdfOther,mSet);

  if (HostExp != NULL)
    {
    MRsvSetAttr(R,mraHostExp,(void **)HostExp,mdfString,mSet);
    }
  else
    {
    MUFree(&R->HostExp);
    }


  if (DRes != NULL)
    {
    MCResCopy(&R->DRes,tmpDRes);
    }
  else
    {
    /* initialize DRes */

    MCResClear(&R->DRes);

    R->DRes.Procs = -1;
    R->DRes.Mem   = -1;
    R->DRes.Disk  = -1;
    R->DRes.Swap  = -1;
    }

  if ((DoAlloc == TRUE) && (NodeList != NULL))
    {
    if (MRsvAllocate(R,NodeList) == FAILURE)
      {
      MRsvDestroy(&R,TRUE,TRUE);

      if (EMsg != NULL)
        strcpy(EMsg,"cannot allocate nodes to reservation");

      return(FAILURE);
      }

    /* record nodelist */

    /* NOTE:  only supports single req requests */

    MNLCopy(&R->NL,NodeList);
    }    /* END if ((DoAlloc == TRUE) && (NodeList != NULL)) */

  if (NC > 0)
    R->ReqNC = MAX(R->AllocNC,NC);

  if ((Type != mrtJob) && !bmisset(&R->Flags,mrfStanding))
    {
    sprintf(Message,"RESERVATIONCREATED:  %s %s %s %ld %ld %ld %d\n",
      R->Name,
      MRsvType[Type],
      RBaseName,
      MSched.Time,
      StartTime,
      EndTime,
      R->ReqNC);

    MSysRegEvent(Message,mactNONE,1);
    }

  if (RsvP != NULL)
    *RsvP = R;

  R->CTime = MSched.Time;
  R->MTime = MSched.Time;

  R->CIteration = MSched.Iteration;

  if (R->A != NULL)
    {
    if (MSched.Iteration >= 0)
      {
      if (MAMHandleCreate(
           &MAM[0],
           R,
           mxoRsv,
           NULL,
           EMsg,
           NULL) == FAILURE)
        {
        MDB(1,fSCHED) MLog("ALERT:  Unable to register reservation creation with accounting manager for %d procs and reservation %s\n",
          PC,
          R->Name);

        MRsvDestroy(&R,TRUE,TRUE);

        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          strcpy(EMsg,"Unable to register reservation creation with accounting manager");

        return(FAILURE);
        }
      }

    /* do not create AM reservation for previously created rsv's being
       restored from checkpoint at iteration -1 */
    else /* if (MSched.Iteration > 0) */
      {
      R->AllocResPending = TRUE;
      }
    } /* END if ((R->A != NULL) || (R->U != NULL)) */

  /* loop through and export the reservation where applicable */

  if ((DoRemote == TRUE) &&
      (NodeList != NULL) &&
      (!MNLIsEmpty(NodeList)))
    {
    char EMsg[MMAX_LINE];

    enum MStatusCodeEnum SC;
    mnode_t *N;

    int  index;
    mbitmap_t BitMap;

    char tmpRsvName[MMAX_NAME];

    for (index = 0;MNLGetNodeAtIndex(NodeList,index,&N) == SUCCESS;index++)
      {
      if (N->RM == NULL)
        continue;

      if (bmisset(&BitMap,N->RM->Index) || !bmisset(&N->RM->Flags,mrmfRsvExport))
        continue;

      bmset(&BitMap,RM->Index);

      if (MRMRsvCtl(R,N->RM,mrcmCreate,(void **)tmpRsvName,EMsg,&SC) == SUCCESS)
        {
        MOAddRemoteRsv((void *)R,mxoRsv,N->RM,tmpRsvName);
        }
      }
    }    /* END if ((DoRemote == TRUE) && ...) */

  /* don't transition here, everything will be transitioned in 
     MRsvUpdateStats, MRsvJCreate, MUIRsvCreate (a higher level) */

  return(SUCCESS);
  }  /* END MRsvCreate() */




/**
 * @see MRsvCreate() - parent
 * @see MRsvAdjust() - parent
 * @see MRsvAdjustNode() - child
 *
 * @param R (I) [modified]
 * @param NodeList (I)
 */

int MRsvAllocate(

  mrsv_t    *R,        /* I (modified) */
  mnl_t     *NodeList) /* I */

  {
  int nindex;

  int TC;

  mnode_t *N;

  const char *FName = "MRsvAllocate";

  MDB(7,fSTRUCT) MLog("%s(%s,NodeList)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if ((R == NULL) || (NodeList == NULL) || (MNLIsEmpty(NodeList)))
    {
    MDB(2,fSTRUCT) MLog("ALERT:    allocation request for rsv %s has empty nodelist\n",
      (R != NULL) ? R->Name : "NULL");

    return(FAILURE);
    }

  R->AllocNC = 0;
  R->AllocTC = 0;
  R->AllocPC = 0;

  /* link nodes to reservation */

  for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    TC = MNLGetTCAtIndex(NodeList,nindex);

    R->AllocNC ++;
    R->AllocTC += MAX(1,TC);
    R->AllocPC += (R->DRes.Procs == -1) ?
      N->CRes.Procs :
      (TC * R->DRes.Procs);

    /* TC is always a positive number */

    if (MRsvAdjustNode(R,N,TC,0,FALSE) == FAILURE)
      {
      MRsvDeallocateResources(R,NodeList);

      return(FAILURE);
      }

    if (N->PtIndex != R->PtIndex)
      R->PtIndex = 0;
    }    /* END for (nindex) */

  if (nindex == MSched.M[mxoNode])
    {
    MDB(1,fSTRUCT) MLog("ERROR:    node overflow in %s\n",
      FName);
    }

  return(SUCCESS);
  }  /* END MRsvAllocate() */





/**
 * Free/Destroy reservation.
 *
 * NOTE: free all alloc'ed structures, mark container as empty
 *
 * @see MJobReleaseRsv() - parent
 *
 * if (IsReal == TRUE)    clear SR->R (for standing reservations)
 *                        deallocate resources
 *                        charge for allocation
 *                        clear J->R (for job rsv)
 *
 * @param RP (I) [modified]
 * @param IsReal (I) [real, not template]
 * @param DoRemote (I) - free corresponding remote rsv
 */

int MRsvDestroy(

  mrsv_t  **RP,       /* I (modified) */
  mbool_t   IsReal,   /* I - reservation is currently in MRsv[] (real, not template) */
  mbool_t   DoRemote) /* I - free corresponding remote rsv */

  {
  int sindex;
  int index;

  mjob_t  *J;

  char     RName[MMAX_NAME];
  char     Message[MMAX_LINE];
  char     WCString[MMAX_NAME];

  mrsv_t  *R;

  const char *FName = "MRsvDestroy";

  MDB(3,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    ((RP != NULL) && (*RP != NULL)) ? (*RP)->Name : "NULL",
    MBool[IsReal],
    MBool[DoRemote]);

  /* invalid reservation specified */

  if ((RP == NULL) ||
     (*RP == NULL) ||
     (*RP == (mrsv_t *)1))
    {
    /* don't use MRsvIsValid() because some temporary reservations don't have names */

    MDB(8,fSTRUCT) MLog("INFO:     no rsv to release\n");

    return(SUCCESS);
    }

  R = *RP;

  MUStrCpy(RName,R->Name,sizeof(RName));

  if (IsReal == TRUE)
    {
    MRsvDestroyCredLock(R);
    }

  /* Remove from VCs */

  if (R->ParentVCs != NULL)
    {
    MVCRemoveObjectParentVCs((void *)R,mxoRsv);
    }

  if (bmisset(&R->Flags,mrfSystemJob))
    {
    if ((R->JName != NULL) && (MJobFind(R->JName,&J,mjsmBasic) == SUCCESS))
      {
      if ((R->J != NULL) && (R->J == J) && (R->CancelIsPending == FALSE))
        {
        int nindex;
        int TC;

        enum MJobStateEnum OldState;

        mnode_t *N;

        mreq_t *RQ = J->Req[0];

        J->CompletionTime = MSched.Time;

        /* assume system job completed successfully */

        MDB(1,fPBS) MLog("INFO:     active S3 job %s has been removed from the queue.  assuming successful completion\n",
          J->Name);

        for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          if ((RQ->NAccessPolicy == mnacSingleJob) ||
              (RQ->NAccessPolicy == mnacSingleTask))
            {
            MCResRemove(&N->DRes,&N->CRes,&N->CRes,1,TRUE);

            MCResAdd(&N->ARes,&N->CRes,&N->CRes,1,FALSE);
            }
          else
            {
            TC = MNLGetTCAtIndex(&J->NodeList,nindex);

            MCResRemove(&N->DRes,&N->CRes,&RQ->DRes,TC,TRUE);

            MCResAdd(&N->ARes,&N->CRes,&RQ->DRes,TC,FALSE);
            }
          }

        MRMJobPreUpdate(J);

        OldState          = J->State;

        J->State          = mjsCompleted;

        J->CompletionTime = J->ATime;

        if (J->StartTime == 0)
          J->StartTime = J->CompletionTime;

        MRMJobPostUpdate(J,NULL,OldState,J->SubmitRM);

        MJobProcessCompleted(&J);
        }
      else if ((R->J != NULL) && (R->J == J) && (R->CancelIsPending == TRUE))
        {
        int nindex;
        int TC;

        enum MJobStateEnum OldState;

        mnode_t *N;

        mreq_t *RQ = J->Req[0];

        /* why is J->CompletionTime set twice? */

        J->CompletionTime = MSched.Time;

        /* assume system job completed successfully */

        MDB(1,fPBS) MLog("INFO:     active S3 job %s has been removed from the queue.  assuming successful completion\n",
          J->Name);

        for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          if ((RQ->NAccessPolicy == mnacSingleJob) ||
              (RQ->NAccessPolicy == mnacSingleTask))
            {
            MCResRemove(&N->DRes,&N->CRes,&N->CRes,1,TRUE);

            MCResAdd(&N->ARes,&N->CRes,&N->CRes,1,FALSE);
            }
          else
            {
            TC = MNLGetTCAtIndex(&J->NodeList,nindex);

            MCResRemove(&N->DRes,&N->CRes,&RQ->DRes,TC,TRUE);

            MCResAdd(&N->ARes,&N->CRes,&RQ->DRes,TC,FALSE);
            }
          }

        MRMJobPreUpdate(J);

        OldState          = J->State;

        J->State          = mjsRemoved;

        J->CompletionTime = J->ATime;

        if (J->StartTime == 0)
          J->StartTime = J->CompletionTime;

        MRMJobPostUpdate(J,NULL,OldState,J->SubmitRM);

        MJobProcessRemoved(&J);
        }
      }    /* END if ((R->JName != NULL) && (MJobFind(R->JName,&J,NULL,mjsmBasic) == SUCCESS)) */

    R->J = NULL;
    }  /* END if (bmisset(&R->Flags,mrfSystemJob)) */

  if ((IsReal == TRUE) && (MSched.Shutdown == FALSE) && (MSched.Recycle == FALSE))
    {
    /* remove the reservation from the cache */

    MRsvTransition(R,TRUE);

    MORemoveCheckpoint(mxoRsv,R->Name,0,NULL);

    MDB(8,fSTRUCT)
      {
      char TString[MMAX_LINE];

      MULToTString(R->StartTime - MSched.Time,TString);

      strcpy(WCString,TString);

      MULToTString(R->EndTime - MSched.Time,TString);

      MLog("INFO:     releasing reservation %s (%s -> %s : %d)\n",
        R->Name,
        WCString,
        TString,
        R->AllocTC);
      }

    if (bmisset(&R->Flags,mrfStanding) &&
        bmisset(&R->IFlags,mrifSystemJobSRsv) &&
        (R->SR != NULL))
      {
      msrsv_t *tmpSR = R->SR;

      for (index = 0;index < MMAX_SRSV_DEPTH;index++)
        {
        if (tmpSR->R[index] == R)
          {
          tmpSR->R[index] = NULL;

          break;
          }
        }
      }

    if (bmisset(&R->Flags,mrfStanding) &&
        !bmisset(&R->IFlags,mrifSystemJobSRsv))
      {
      msrsv_t *tmpSR = NULL;

      /* clear SR pointer */

      for (sindex = 0;sindex < MSRsv.NumItems;sindex++)
        {
        for (index = 0;index < MMAX_SRSV_DEPTH;index++)
          {
          tmpSR = (msrsv_t *)MUArrayListGet(&MSRsv,sindex);

          if (tmpSR == NULL)
            break;

          if (tmpSR->R[index] == R)
            {
            tmpSR->R[index] = NULL;

            break;
            }
          }    /* END for (index)   */
        }      /* END for (sindex)  */
      }        /* END if (bmisset(&R->Flags,mrfStanding)) */

    /* clear node reservation pointers */

    if (R->Type != mrtJob)
      {
      /* do not charge if rsv was immediately destroyed after creation */
      /* NOTE: charge if setup triggers have fired or if trigger is/was active */
      if (bmisset(&R->Flags,mrfTrigHasFired) ||
          bmisset(&R->Flags,mrfIsActive) ||
          bmisset(&R->Flags,mrfWasActive))
        {
        /* This will handle final charge and removal of reservation */
        if ((R->A != NULL) && (MAMHandleEnd(&MAM[0],(void *)R,mxoRsv,NULL,NULL) == FAILURE))
          {
          MDB(1,fSCHED) MLog("WARNING:  Unable to register reservation end with accounting manager for %6.2f PS for reservation %s\n",
            R->CIPS,
            R->Name);
          }

        /* Update reservation cycle tallies */
        R->TIPS += R->CIPS;
        R->TAPS += R->CAPS;
        R->CIPS = 0.0;
        R->CAPS = 0.0;
        }
      }    /* END if (R->Type != mrfJOB) */

    if (R->Type != mrtJob)
      {
      /* NOTE:  must write stats before resources are deallocated */

      if (bmisset(&R->Flags,mrfIsActive) == TRUE)
        {
        MOWriteEvent((void *)R,mxoRsv,mrelRsvEnd,NULL,MStat.eventfp,NULL);

        CreateAndSendEventLog(meltRsvEnd,R->Name,mxoRsv,(void *)R);
        }
      }

    /* NOTE:  only called for active reservations (why?) */

    MRsvDeallocateResources(R,&R->NL);

    J = NULL;

    if ((R->Type == mrtJob) || (R->Type == mrtMeta))
      {
      /* should be clearing R->J->Req[X]->R as well if we are about to destroy a referenced reservation */

      if (R->J != NULL)
        {
        int rqindex;

        /* find this specific R in job's reqs */

        for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
          {
          if (R->J->Req[rqindex] == NULL)
            break;

          if (R->J->Req[rqindex]->R == R)
            R->J->Req[rqindex]->R = NULL;
          }

        /* clear job rsv pointer */

        if (R->J->Rsv == R)
          R->J->Rsv = NULL;

        MDB(5,fSTRUCT) MLog("INFO:     job '%s' reservation released (tasks requested: %d)\n",
          R->Name,
          R->J->Request.TC);
        }
      else
        {
        MDB(3,fSTRUCT) MLog("INFO:     cannot locate job associated with reservation '%s' (possible multi-req job with rsv previously released)\n",
          R->Name);
        }
      }
    else
      {
      /* send notification for non-job rsv only */

      sprintf(Message,"RESERVATIONDESTROYED:  %s %s %ld %ld %ld %d\n",
        R->Name,
        MRsvType[R->Type],
        MSched.Time,
        R->StartTime,
        R->EndTime,
        R->ReqNC);

      MSysRegEvent(Message,mactNONE,1);
      }      /* END if (R->Type == mrtJob) */

    if (R->J != NULL)
      {
      if ((R->J->TemplateExtensions != NULL) && (R->J->TemplateExtensions->R == R))
        {
        /* clear dynamic job pointer */

        R->J->TemplateExtensions->R = NULL;
        }
      }
    }  /* END if (IsReal == TRUE) */

  if ((DoRemote == TRUE) && (R->RemoteRsv != NULL))
    {
    mrrsv_t *RR = R->RemoteRsv;
    mrrsv_t *tmpR;

    enum MStatusCodeEnum SC;

    while (RR != NULL)
      {
      MRMRsvCtl(R,RR->RM,mrcmDestroy,NULL,NULL,&SC);

      tmpR = RR->Next;

      RR = tmpR;
      }
    }

  if (R->RemoteRsv != NULL)
    {
    mrrsv_t *RR = R->RemoteRsv;
    mrrsv_t *tmpR;

    while (RR != NULL)
      {
      tmpR = RR->Next;

      MUFree((char **)&RR);

      RR = tmpR;
      }
    }

  R->StartTime = 0;

  if (R->T != NULL)
    {
    MTListClearObject(R->T,R->CancelIsPending);

    MUArrayListFree(R->T);

    MUFree((char **)&R->T);
    }  /* END if (R->T != NULL) */

  MNLFree(&R->NL);

  MNLFree(&R->ReqNL);

  if (R->C != NULL)
    MUFree((char **)&R->C);

  MACLFreeList(&R->CL);
  MACLFreeList(&R->ACL);

  MUFree(&R->NodeSetPolicy);

  MUFree(&R->Comment);
  MUFree(&R->CmdLineArgs);
  MUFree(&R->SystemID);
  MUFree(&R->HostExp);

#if 0 /* there is corruption somewhere that is causing the address
         of this pointer to be the name of another reservation.  For now
         don't free this pointer as it may cause a crash */
#endif

  /* update for gatling, re-enabling to see if it is fixed */

  if (R->RsvGroup != NULL)
    MUFree(&R->RsvGroup);

  if (R->RsvParent != NULL)
    MUFree(&R->RsvParent);

  MUFree(&R->Label);
  MUFree(&R->SpecName);
  MUFree(&R->SystemRID);
  MUFree(&R->SysJobID);
  MUFree(&R->Profile);

  bmclear(&R->Flags);
  bmclear(&R->ReqFBM);
  bmclear(&R->IFlags);

  if (R->History != NULL)
    {
    mhistory_t *tmpH;

    int hindex;

    for (hindex = 0;hindex < R->History->NumItems;hindex++)
      {
      tmpH = (mhistory_t *)MUArrayListGet(R->History,hindex);

      MCResFree(&tmpH->Res);
      }

    MUArrayListFree(R->History);

    MUFree((char **)&R->History);
    }

  if (R->RsvAList != NULL)
    {
    for (index = 0;index < MMAX_PJOB;index++)
      {
      if (R->RsvAList[index] == NULL)
        break;

      MUFree(&R->RsvAList[index]);
      }

    MUFree((char **)&R->RsvAList);
    }

  if (R->RsvExcludeList != NULL)
    {
    for (index = 0;index < MMAX_PJOB;index++)
      {
      if (R->RsvExcludeList[index] == NULL)
        break;

      MUFree(&R->RsvExcludeList[index]);
      }

    MUFree((char **)&R->RsvExcludeList);
    }

  if (R->JName != NULL)
    MUFree(&R->JName);

  MULLFree(&R->Variables,MUFREE);

  MMBFree(&R->MB,TRUE);

  MCResFree(&R->DRes);

  if ((IsReal == TRUE) && (MSched.Shutdown == FALSE) && (MSched.Recycle == FALSE))
    {
    MSched.A[mxoRsv]--;

    if (R->J != NULL)
      R->J->RType = mjrNONE;
    }  /* END if (IsReal == TRUE) */

  *RP = NULL;

  MDB(7,fSTRUCT) MLog("INFO:     rsv '%s' released\n",
    R->Name);

  R->Name[0] = '\1';

  if (R->VMTaskMap != NULL)
    {
#if 0
    MVMDestroyInTaskMap((R->VMTaskMap));
#endif
    MUFree((char **)&R->VMTaskMap);
    }

  R->J = NULL;

  if (IsReal == TRUE)
    {
    MRsvHT.erase(RName);

    MUFree((char **)&R);
    }

  MRsvTableDumpToLog(__FUNCTION__,__LINE__);

  return(SUCCESS);
  }  /* END MRsvDestroy() */





/**
 * Initialize reservation, apply rsv profile if specified
 *
 * NOTE:  All dynamic rsv attributes must be explicitly handled if rsv template
 *        is specified.
 *
 * @param RP (I) [modified]
 */

int MRsvInitialize(

  mrsv_t **RP)

  {
  mrsv_t *R;

  if (RP == NULL)
    {
    return(FAILURE);
    }

  if (*RP == NULL)
    {
    *RP = (mrsv_t *)MUCalloc(1,sizeof(mrsv_t));

    MSched.A[mxoRsv]++;
    }
  else
    {
    memset(*RP,0,sizeof(mrsv_t));
    }

  R = *RP;

  R->HostExpIsSpecified = MBNOTSET;

  R->Name[0] = '\1';

  MCResInit(&R->DRes);

  R->SubType = mrsvstNONE;

  return(SUCCESS);
  }  /* END MRsvInitialize() */

/* END MRsvCRUD.c */
