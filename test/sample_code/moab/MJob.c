/* HEADER */

      
/**
 * @file MJob.c
 *
 * Contains job related functions. Includes mapping a job onto nodes, 
 * diagnosing jobs, starting and destroying jobs, etc.
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"



/**
 * Free the job table.
 *
 * @see MSysAllocJobTable() - peer
 * @see MCJobFreeTable() - peer
 */

int MJobFreeTable(void)

  {
  mjob_t *J = NULL;

  job_iter JTI;

  while (MJobHT.size() != 0)
    {
    JTI = MJobHT.begin();

    J = (*JTI).second;

    MJobRemove(&J);
    }

  MJobHT.clear();

  return(SUCCESS);
  }  /* END MJobFreeTable() */



/**
 * Initialize a job table iterator.
 *
 * @param JTI
 */

int MJobIterInit(

  job_iter *JTI)

  {
  *JTI = MJobHT.begin();

  return(SUCCESS);
  }  /* END MJobIterInit() */



/**
 * Iterate over the job table.
 * 
 * NOTE: this routine runs the normal checks to make sure the job is valid.
 *
 * @param JTIP (I) - must already be initialized
 * @param JP   (O) - valid job or NULL
 */

int MJobTableIterate(

  job_iter   *JTIP,
  mjob_t    **JP)

  {
  job_iter JTI;

  if (JP != NULL)
    *JP = NULL;

  if ((JP == NULL) || (JTIP == NULL))
    {
    return(FAILURE);
    }

  if (MJobHT.size() == 0)
    return(FAILURE);

  JTI = *JTIP;

  if (JTI == MJobHT.end())
    return(FAILURE);

  *JP = (*JTI).second;

  while (++JTI != MJobHT.end())
    {
    if (MJobPtrIsValid((*JTI).second) == FALSE)
      {
      continue;
      }

    /* as soon as we find a valid job then break out and return it */

    break;
    }

  *JTIP = JTI;

  return(SUCCESS);
  }  /* END MJobTableIterate() */



/**
 * Find the job in the table.
 *
 * @param Name
 * @param JP (optional)
 */

int MJobTableFind(

  const char *Name,
  mjob_t    **JP)

  {
  job_iter JTI;

  if (JP != NULL)
    *JP = NULL;

  if (MUStrIsEmpty(Name))
    {
    return(FAILURE);
    }

  JTI = MJobHT.find(Name);

  if (JTI == MJobHT.end())
    {
    return(FAILURE);
    }

  if (JP != NULL)
    *JP = (*JTI).second;

  return(SUCCESS);
  }  /* END MJobTableFind() */



/**
 * Insert the job into the table.
 *
 * @param Name
 * @param J
 */

int MJobTableInsert(

  const char *Name,
  mjob_t     *J)

  {
  std::string JName;

  if ((MUStrIsEmpty(Name) || (J == NULL)))
    {
    return(FAILURE);
    }

  JName = Name;

  MJobHT.insert(std::pair<std::string,mjob_t *>(JName,J));

  return(SUCCESS);
  }  /* END MJobTableInsert() */



/**
 * Initialize a completed job table iterator.
 *
 * @param JTI
 */

int MCJobIterInit(

  job_iter *JTI)

  {
  *JTI = MCJobHT.begin();

  return(SUCCESS);
  }  /* END MJobIterInit() */






/**
 * Free the job table.
 *
 * @see MCJobFreeTable() - peer
 */

int MCJobFreeTable(void)

  {
  job_iter JTI;

  for (JTI = MCJobHT.begin();JTI!=MCJobHT.end();JTI++)
    {
    MJobRemove(&(*JTI).second);
    }

  MCJobHT.clear();

  return(SUCCESS);
  }  /* END MCJobFreeTable() */



/**
 * Iterate over the job table.
 * 
 * NOTE: this routine runs the normal checks to make sure the job is valid.
 *
 * @param JTIP (I) - must already be initialized
 * @param JP (O) - valid job or NULL
 */

int MCJobTableIterate(

  job_iter   *JTIP,
  mjob_t    **JP)

  {
  job_iter JTI;

  if (JP != NULL)
    *JP = NULL;

  if ((JP == NULL) || (JTIP == NULL))
    {
    return(FAILURE);
    }

  if (MCJobHT.size() == 0)
    return(FAILURE);

  JTI = *JTIP;

  if (JTI == MCJobHT.end())
    return(FAILURE);

  *JP = (*JTI).second;

  while (++JTI != MCJobHT.end())
    {
    if (MJobPtrIsValid((*JTI).second) == FALSE)
      {
      continue;
      }

    /* as soon as we find a valid job then break out and return it */

    break;
    }

  *JTIP = JTI;

  return(SUCCESS);
  }  /* END MCJobTableIterate() */



/**
 * Find the job in the table.
 *
 * @param Name
 * @param JP (optional)
 */

int MCJobTableFind(

  const char *Name,
  mjob_t    **JP)

  {
  job_iter JTI;

  if (JP != NULL)
    *JP = NULL;

  if (MUStrIsEmpty(Name))
    {
    return(FAILURE);
    }

  JTI = MCJobHT.find(Name);

  if (JTI == MCJobHT.end())
    {
    return(FAILURE);
    }

  if (JP != NULL)
    *JP = (*JTI).second;

  return(SUCCESS);
  }  /* END MCJobTableFind() */



/**
 * Insert the job into the table.
 *
 * @param Name
 * @param J
 */

int MCJobTableInsert(

  const char *Name,
  mjob_t     *J)

  {
  std::string JName;

  if ((MUStrIsEmpty(Name) || (J == NULL)))
    {
    return(FAILURE);
    }

  JName = Name;

  MCJobHT.insert(std::pair<std::string,mjob_t *>(JName,J));

  return(SUCCESS);
  }  /* END MCJobTableInsert() */





/**
 * Determine whether a given job pointer is valid.
 *
 * @param J
 */

mbool_t MJobPtrIsValid(

  const mjob_t *J)

  {
  if ((J == NULL) || (J == (mjob_t *)1))
    {
    return(FALSE);
    }

  if ((J->Name[0] == '\0') || (!isprint(J->Name[0])))
    {
    return(FALSE);
    }

  return(TRUE);
  }  /* END MJobPtrIsValid() */



/**
 * Resume job.
 *
 * @param J (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MJobResume(

  mjob_t *J,    /* I */
  char   *EMsg, /* O (optional,minsize=MMAX_LINE) */
  int    *SC)   /* O (optional) */

  {
  const char *FName = "MJobResume";

  MDB(3,fSCHED) MLog("%s(%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  J->SubState = mjsstNONE;

  if ((J == NULL) || (J->State != mjsSuspended))
    {
    /* verify job state */

    if (EMsg != NULL)
      sprintf(EMsg,"cannot resume non-suspended job");

    return(FAILURE);
    }

  if (MRMJobResume(J,EMsg,SC) == FAILURE)
    {
    /* cannot resume job */

    return(FAILURE);
    }

  MJobSetState(J,mjsRunning);

  MQueueAddAJob(J);

  return(SUCCESS);
  }  /* END MJobResume() */




/**
 * Send a rejection email because a job failed to submit.
 *
 * @see MJobReject() - peer
 * 
 * @param ID
 * @param User
 * @param Msg
 */

int MJobSubmitFailureSendMail(

  char *ID,
  char *User,
  char *Msg)

  {
  char tmpSubj[MMAX_LINE];
  char tmpLine[MMAX_LINE];

  char *APtr;
 
  mgcred_t *U = NULL;

  if (MUStrIsEmpty(User))
    {
    return(FAILURE);
    }

  if (MUserFind(User,&U) == FAILURE)
    {
    return(FAILURE);
    }

  if (U->NoEmail == TRUE)
    {
    return(SUCCESS);
    }

  /* create job reject message */

  snprintf(tmpLine,sizeof(tmpLine),"job %s failed to submit%s%s\n",
    ID,
    (Msg != NULL) ? " -- " : "",
    (Msg != NULL) ? Msg : "");

  sprintf(tmpSubj,"job %s could not be submitted",
    ID);

  if (U->EMailAddress != NULL)
    APtr = U->EMailAddress;
  else
    APtr = U->Name;

  MSysSendMail(
    APtr,
    NULL,
    tmpSubj,
    NULL,
    tmpLine);

  return(SUCCESS);
  }  /* END MJobSubmitFailureSendMail() */




int MUJobAttributesToString(

  mbitmap_t  *BM,
  char       *String)

  {
  char *BPtr;
  int   BSpace;

  int   sindex;

  if (String != NULL)
    String[0] = '\0';

  if ((BM == NULL) || (String == NULL))
    {
    return(FAILURE);
    }
 
  MUSNInit(&BPtr,&BSpace,String,MMAX_LINE);

  for (sindex = 1;MAList[meJFeature][sindex][0] != '\0';sindex++)
    {
    if (bmisset(BM,sindex))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        (String[0] != '\0') ? "," : "",
        MAList[meJFeature][sindex]);
      }
    }      /* END for (sindex) */

  return(SUCCESS);
  }  /* END MUJobAttributesToString() */


/**
 * Join CR's resources to R's and delete CR.
 *
 * NOTE: this will force things through (not scheduled)
 *       the task structure of both reservations will be
 *       reduced to the lowest common denominator
 *       (1proc/Xmem/Ydisk/Zswap)
 *       The new rsv will have the same timeframe of R.
 *       A new hostexp will be created.
 */

int MJobJoin(

  mjob_t *J,
  mjob_t *CJ,
  char   *Msg)

  {

  mreq_t *RQ;
  mreq_t *CRQ;

  if (Msg == NULL)
    {
    return(FAILURE);
    }

  /* don't join jobs that have multiple reqs, aren't active, have required hostlists, or don't have reservations */

  if ((J->Req[0] == NULL) || (J->Req[1] != NULL) || 
      (CJ->Req[0] == NULL) || (CJ->Req[1] != NULL) ||
      !MNLIsEmpty(&J->ReqHList) || !MNLIsEmpty(&CJ->ReqHList) ||
      (J->Req[0]->R == NULL) || (CJ->Req[0]->R == NULL) ||
      !MJOBISACTIVE(J) || !MJOBISACTIVE(CJ))
    {
    snprintf(Msg,MMAX_LINE,"one or both jobs are in an invalid state for joining\n");

    return(FAILURE);
    }

  RQ  = J->Req[0];
  CRQ = CJ->Req[0];

  /* this routine joins the reservations and calculates the resulting DRes */

  if (MRsvJoin(RQ->R,CRQ->R,Msg) == FAILURE)
    {
    return(FAILURE);
    }

  /* FIXME: this should be Complete, not cancel */

  MJobCancel(CJ,"joined to another job",FALSE,NULL,NULL);

  MNLCopy(&RQ->NodeList,&RQ->R->NL);

  MCResCopy(&RQ->DRes,&RQ->R->DRes);
  
  MJobRefreshFromNodeLists(J);

  MRsvTransition(RQ->R,FALSE);
  MJobTransition(J,TRUE,FALSE);

  MOWriteEvent((void *)J,mxoJob,mrelJobModify,NULL,MStat.eventfp,NULL);

  CreateAndSendEventLog(meltJobModify,J->Name,mxoJob,(void *)J);

  return(SUCCESS);
  }  /* END MJobJoin() */







/**
 * Apply job reject policy to job which was rejected for policy, resource, or other violation.
 *
 * NOTE:  actions include cancel, hold, send mail, etc.
 *
 * @see MJobSubmitFailureSendMail()
 *
 * @param JP (I) [modified, may be cancelled]
 * @param RejTypeBM (I) [BM]
 * @param RejDuration (I)
 * @param RejReason (I)
 * @param RejMsg (I)
 */

int MJobReject(

  mjob_t               **JP,
  enum MHoldTypeEnum     RejType,
  long                   RejDuration,
  enum MHoldReasonEnum   RejReason,
  const char            *RejMsg)

  {
  char tmpLine[MMAX_LINE];

  mjob_t *J;

  char TString[MMAX_LINE];
  const char *FName = "MJobReject";

  MULToTString(RejDuration,TString);

  MDB(3,fSCHED) MLog("%s(%s,%s,%s,%s)\n",
    FName,
    ((JP != NULL) && (*JP != NULL)) ? (*JP)->Name : "NULL",
    TString,
    MHoldReason[RejReason],
    (RejMsg != NULL) ? RejMsg : "NULL");

  if ((JP == NULL) || (*JP == NULL) || (MJOBISACTIVE(*JP) == TRUE))
    {
    /* only reject idle jobs */

    return(FAILURE);
    }

  J = *JP;

  MDB(7,fSCHED) MLog("INFO:     job '%s' is being rejected (reject policy config: %s)\n",
    J->Name,
    bmtostring(&MSched.JobRejectPolicyBM,MJobRejectPolicy,tmpLine));

  if ((bmisset(MJobGetRejectPolicy(J),mjrpMail) == TRUE) &&
      (J->Credential.U->NoEmail != TRUE))
    {
    char tmpSubj[MMAX_LINE];

    char *APtr;
 
    /* create job reject message */

    snprintf(tmpLine,sizeof(tmpLine),"job %s has been %s for the following reason:\n\n  %s\n",
      J->Name,
      (bmisset(MJobGetRejectPolicy(J),mjrpCancel) == TRUE) ? "cancelled" : "placed on hold",
      RejMsg);

    sprintf(tmpSubj,"job %s rejected (%s)",
      J->Name,
      (bmisset(MJobGetRejectPolicy(J),mjrpCancel) == TRUE) ? "job cancelled" : "job placed on hold");

    if (J->Credential.U->EMailAddress != NULL)
      APtr = J->Credential.U->EMailAddress;
    else
      APtr = J->Credential.U->Name;

    MSysSendMail(
      APtr,
      NULL,
      tmpSubj,
      NULL,
      tmpLine);
    }  /* END if (bmisset(MSched.JobRejectPolicy,mjrpMail) == TRUE) && (J->Cred.U->NoEmail != TRUE) */

  if (bmisset(MJobGetRejectPolicy(J),mjrpHold) == TRUE)
    {
    MJobSetHold(J,RejType,RejDuration,RejReason,RejMsg);
    }

  if (bmisset(MJobGetRejectPolicy(J),mjrpCancel) == TRUE)
    {
    mjblockinfo_t *BlockInfoPtr;

    if (RejMsg != NULL)
      {
      sprintf(tmpLine,"MOAB_INFO:  job was rejected - %s\n",
        RejMsg);
      }
    else
      {
      sprintf(tmpLine,"MOAB_INFO:  job was rejected\n");
      }

    /* alloc and add a block record to this job */

    BlockInfoPtr = (mjblockinfo_t *)MUCalloc(1,sizeof(mjblockinfo_t));
  
    BlockInfoPtr->PIndex = 0;

    /* note - previous code did not overwrite existing reason so we will preserve it here.
              maybe we need a new reason code for a job reject? */

    BlockInfoPtr->BlockReason = MJobGetBlockReason(J,NULL);
    MUStrDup(&BlockInfoPtr->BlockMsg,tmpLine);

    if (MJobAddBlockInfo(J,NULL,BlockInfoPtr) != SUCCESS)
      MJobBlockInfoFree(&BlockInfoPtr);

    bmset(&J->IFlags,mjifCancel);
    }

  if (bmisset(MJobGetRejectPolicy(J),mjrpIgnore) == TRUE)
    {
    /* should we ever reach this point with IGNORE turned on? we shouldn't iff IGNORE is implemented correctly! */
    /* NO-OP */
    }

  if (bmisset(MJobGetRejectPolicy(J),mjrpRetry) == TRUE)
    {
    MMBAdd(
      &J->MessageBuffer,
      RejMsg,
      NULL,
      mmbtHold,
      MSched.Time + MCONST_HOURLEN,
      0,
      NULL);

    /* J->SMinTime = MSched.Time + 1; -- LLNL wants job to start and err (JMB) */
  
    /* 2-3-10 BOC RT6946 - Release rsv so that there won't be a reservation 
     * while the job violates policies (ex. MAX.WCLIMIT). */

    MJobReleaseRsv(J,TRUE,TRUE);
    }

  return(SUCCESS);
  }  /* END MJobReject() */



/*
 * Perform internal blackbox test on RM extension.
 *
 * @see MJobProcessExtensionString() - child
 *
 * @param XString (I)
 */

int MJobTestRMExtension(

  char *XString)  /* I */

  {
  mjob_t *tmpJ = NULL;

  MJobMakeTemp(&tmpJ);

  if (MJobProcessExtensionString(tmpJ,XString,mxaNONE,NULL,NULL) == SUCCESS)
    {
    /* test succeeded */

    exit(0);
    }

  exit(1);

  /*NOTREACHED*/

  return(SUCCESS);
  }  /* END MJobTestRMExtension() */





/**
 * determine whether a job is blocked or not
 *
 * @param J (I) -- the job we're checking
 */

mbool_t MJobIsBlocked(

  mjob_t * J)   /* I */

  {
  mjblockinfo_t *BlockInfo = NULL;

  if (J == NULL)
    {
    return(FALSE);
    }

  /* Check for job hold */

  if (!bmisclear(&J->Hold))
    return(TRUE);

  MJobGetBlockInfo(J,NULL,&BlockInfo);

  switch (J->State)
    {
    case mjsHold:
    case mjsDeferred:
    case mjsBatchHold:
    case mjsUserHold:
    case mjsSystemHold:
    case mjsBlocked:

      return(TRUE);

      /* NOTREACHED */

      break;

    default:
      
      break;
    } /* END switch (J->State) */

  if (!MJOBISACTIVE(J))
    {
    if (BlockInfo == NULL)
      return(FALSE);

    if ((BlockInfo != NULL) && ((BlockInfo->BlockReason != mjneNONE) || (!MUStrIsEmpty(BlockInfo->BlockMsg))))
      {
      return(TRUE);
      }

    if ((J->DependBlockReason != mdNONE) || (!MUStrIsEmpty(J->DependBlockMsg)))
      {
      return(TRUE);
      }
    } /* END if (!MJOBISACTIVE(J)) */

  return(FALSE);
  } /* END MJobIsBlocked() */






/**
 *
 *
 * @param J (I) [modified,alloc]
 * @param RsvID (I)
 * @param AllowAlloc (I)
 */

int MJobAddAccess(
  
  mjob_t  *J,          /* I (modified,alloc) */
  char    *RsvID,      /* I */
  mbool_t  AllowAlloc) /* I */

  {
  int rindex;

  if ((J == NULL) || (RsvID == NULL))
    {
    return(FAILURE);
    }

  if (J->RsvAList == NULL)
    {
    if (AllowAlloc == FALSE)
      {
      return(FAILURE);
      }
 
    J->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);
    }

  for (rindex = 0;rindex < MMAX_PJOB;rindex++)
    {
    if (J->RsvAList[rindex] == NULL)
      {
      if (AllowAlloc == TRUE)
        MUStrDup(&J->RsvAList[rindex],RsvID);  
      else
        J->RsvAList[rindex] = RsvID;

      return(SUCCESS);
      }

    if (!strcmp(J->RsvAList[rindex],RsvID))
      {
      return(SUCCESS);
      }
    }    /* END for (rindex) */

  return(FAILURE);
  }  /* END MJobAddAccess() */


/**
 * Allocate a job submission object to put on the job submit queue.
 *
 * @param JS (I/O)
 *
 * @see MUIJobSubmitThreadSafe - parent
 * @see MSysAddJobSubmitToQueue - peer
 * @see MSysDequeueJobSubmit - peer
 */

int MJobSubmitAllocate(

  mjob_submit_t **JS)

  {
  if (JS == NULL)
    {
    return(FAILURE);
    }

  *JS = (mjob_submit_t *)MUCalloc(1,sizeof(mjob_submit_t));

  if (*JS == NULL)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobSubmitAllocate() */



/**
 * Allocate a job submission object to put on the job submit queue.
 *
 * @param JS (I/O)
 *
 * @see MUIJobSubmitThreadSafe - parent
 * @see MSysAddJobSubmitToQueue - peer
 * @see MSysDequeueJobSubmit - peer
 */

int MJobSubmitDestroy(

  mjob_submit_t **JS)

  {
  mjob_submit_t *J = NULL;

  if (JS == NULL)
    {
    return(FAILURE);
    }

  J = *JS;

  MXMLDestroyE(&J->JE);

  MUFree((char **)JS);

  return(SUCCESS);
  }  /* END MJobSubmitAllocate() */




/**
 *  Called when a job cannot complete a deadline,
 *  applies the deadline failure policy.
 *
 *  @param J (I)
 */
 
int MJobApplyDeadlineFailure(
 
  mjob_t *J) /* I */

  {
  switch (MSched.DeadlinePolicy)
    {
    case mdlpEscalate:

      /* NYI */

      break;

    case mdlpRetry:

      J->CMaxDateIsActive = MBNOTSET;

      break;

    case mdlpIgnore:

      J->CMaxDateIsActive = FALSE;

      break;

    case mdlpCancel:

      MJobCancel(J,"deadline cannot be satisfied",FALSE,NULL,NULL);

      if (J != NULL)
        J->CMaxDateIsActive = FALSE;

      break;

    case mdlpHold: 
    default:  

      MJobSetHold(J,mhBatch,MMAX_TIME,mhrCredAccess,"cannot enable deadline");

      J->CMaxDateIsActive = MBNOTSET;

      break;

    } /* END switch (MSched.DeadlinePolicy) */

  MJobTransition(J,FALSE,FALSE);

  return(SUCCESS);
  }  /* END MJobApplyDeadlineFailure() */





/**
 * Report base job info to log.
 *
 * @param J (I)
 * @param Mode (I) [not used]
 * @param Buf (O) [ignored]
 */

int MJobShow(

  mjob_t *J,    /* I */
  int     Mode, /* I (not used) */
  char   *Buf)  /* O (ignored) */

  {
  char    Feature[MMAX_LINE];
  char    tmpName[MMAX_NAME];
  char    TString[MMAX_NAME];
  mreq_t *RQ;

  const char *FName = "MJobShow";

  MDB(9,fSTRUCT) MLog("%s(%s,%d,Buf)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    Mode);

  if ((J == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }
 
  RQ = J->Req[0]; /* only show first req for now */
 
  if (RQ == NULL)
    {
    MDB(2,fSTRUCT) MLog("ALERT:    job %s contains no req info\n",
      J->Name);
 
    return(FAILURE);
    }
 
  if (J->CompletionTime == 0)
    {
    strcpy(tmpName,"????????");
    }
  else
    {
    sprintf(tmpName,"%08ld",
      J->EstWCTime);
    }

  MULToTString(J->WCLimit,TString);

  MLog("%16.16s %3d:%3d:%3d(%d) %3.3s %10s(%8s) %8s %8s %10s %s %10.10s %10ld %6s %6s %2s %6d %2s %6d ",
    J->Name,
    J->Request.TC,
    J->TotalProcCount,
    J->Request.NC,
    RQ->TasksPerNode,
    MPar[RQ->PtIndex].Name,
    TString,
    tmpName,
    (J->Credential.U != NULL) ? J->Credential.U->Name : "NULL",
    (J->Credential.G != NULL) ? J->Credential.G->Name : "NULL",
    MJobState[J->State],
    (J->Credential.Q != NULL) ? J->Credential.Q->Name : "NULL",
    (J->Credential.C != NULL) ? J->Credential.C->Name : "NULL",
    J->SubmitTime,
    MAList[meOpsys][RQ->Opsys],
    MAList[meArch][RQ->Arch],
    MComp[(int)RQ->ReqNRC[mrMem]],
    RQ->ReqNR[mrMem],
    MComp[(int)RQ->ReqNRC[mrDisk]],
    RQ->ReqNR[mrDisk]);
 
  MLogNH("%s",
    MUNodeFeaturesToString(',',&RQ->ReqFBM,Feature));
 
  MLogNH("\n");

  return(SUCCESS);
  }  /* END MJobShow() */



/**
 * Order low to high.
 *
 * @param A (I)
 * @param B (I)
 */

int MJobCTimeComp(
 
  mjob_t *A,  /* I */
  mjob_t *B)  /* I */
 
  {
  long tmp;
 
  tmp = (A->StartTime + A->WCLimit) - 
        (B->StartTime + B->WCLimit);
 
  return((int)tmp);
  }  /* END MJobCTimeComp() */





/**
 * Initializes a job object by clearing the memory it occupries and setting
 * all values to 0 and also sets the effective queue duration to -1.
 *
 * @param J (I) [modified]
 */

int MJobInitialize(

  mjob_t *J)  /* I (modified) */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  memset(J,0,sizeof(mjob_t));

  J->EffQueueDuration = -1;

  return(SUCCESS);
  }  /* END MJobInitialize() */


/**
 *  
 * Check the feature bitmap for a job to see if features can be 
 * mapped to partitions. 
 *  
 *
 * @param J (I) The job to check ReqFBM with partition bitmap.
 * @param srcPAL (I) source partition list.
 * @param tmpPAL (O) destination partition list.
 * @param NumFoundPtr (O) Number of partitions in feature bitmap
 */

int MJobFeaturesInPAL(

  mjob_t        *J,
  mbitmap_t     *srcPAL,
  mbitmap_t     *tmpPAL,
  int           *NumFoundPtr)

  {
  int pindex;
  char tmpLine[MMAX_LINE];
  char *TokPtr;
  char *ptr;

  if ((J == NULL) || (tmpPAL == NULL) || (srcPAL == NULL) || (NumFoundPtr == NULL))
    {
    return(FAILURE);
    }

  *NumFoundPtr = 0;

  if (J->Req[0] == NULL)
    {
    return(SUCCESS);
    }

  if (bmisclear(&J->Req[0]->ReqFBM))
    {
    return(SUCCESS);
    }

  MUNodeFeaturesToString(',',&J->Req[0]->ReqFBM,tmpLine);

  ptr = MUStrTok(tmpLine,",",&TokPtr);

  while (ptr != NULL)
    {
    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\0')
        break;

      if (bmisset(srcPAL,pindex) == FAILURE)
        continue;

      if (!strcmp(MPar[pindex].Name,ptr))
        {
        bmset(tmpPAL,MPar[pindex].Index);

        *NumFoundPtr = *NumFoundPtr + 1;
        }
      }  /* END for (pindex) */

    ptr = MUStrTok(NULL,",",&TokPtr);
    }    /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MJobFeaturesInPAL() */
 



/**
 * expand paths in J that contain the $HOME variable
 * @param J (I) modified
 */

int MJobExpandPaths(

  mjob_t *J)

  {
  unsigned int index;

  char **Paths[] = {
    &J->Env.IWD,
    &J->Env.SubmitDir,
    &J->Env.Cmd,
    &J->Env.Output,
    &J->Env.Error,
    NULL };

  for (index = 0;Paths[index] != NULL;index++)
    {
    char tmpLine[MMAX_LINE];
    char ** const PathPtr = Paths[index];

    if (*PathPtr == NULL)
      continue;

    MUStrCpy(tmpLine,*PathPtr,sizeof(tmpLine));

    MJobInsertHome(J,tmpLine,sizeof(tmpLine));

    MUStrDup(PathPtr,tmpLine);
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MJobExpandPaths() */




int MJobGetGResString(

  mjob_t    *J,
  mstring_t *String)

  {
  int index;
  int rqindex;

  if ((J == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,0) == 0)
      continue;

    for (index = 1;index < MSched.M[mxoxGRes];index++)
      { 
      if (MGRes.Name[index][0] == '\0')
        break;
   
      if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,index) == 0)
        continue;
   
      MStringAppendF(String,"%s%s:%d",
        (MUStrIsEmpty(String->c_str())) ? "" : ",",
        MGRes.Name[index],
        MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,index));
      }  /* END for (index) */
    }  /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MJobGetGResString() */




/**
 *
 *
 * @param J (I)
 */

int MJobSendFB(

  mjob_t *J)  /* I */

  {
  char   Args[32][MMAX_LINE << 2];
  char  *ArgV[32];

  double xfactor;
 
  int    aindex;
  int    ACount;
 
  mreq_t *RQ;

  char   *ptr;

  const char *FName = "MJobSendFB";

  MDB(2,fCORE) MLog("%s(%s)\n",
    FName,
    (MJobPtrIsValid(J)) ? J->Name : NULL);

  if (!MJobPtrIsValid(J))
    {
    return(FAILURE);
    }

  xfactor = (double)(J->CompletionTime - J->SystemQueueTime) /
                    MAX(1,(J->CompletionTime - J->StartTime));
 
  /* FORMAT:    JN UN EM JS QR QTM STM CTM  XA WCL PRQ MRQ APROC MPROC AME MME */
 
  RQ = J->Req[0];
 
  ACount = 0;
 
  MUStrCpy(Args[ACount++],J->Name,sizeof(Args[0]));

  if (J->Credential.U != NULL)
    {
    MUStrCpy(Args[ACount++],J->Credential.U->Name,sizeof(Args[0]));

    MUStrCpy(Args[ACount++],(J->Credential.U->EMailAddress != NULL) ?
      J->Credential.U->EMailAddress : J->Credential.U->Name,sizeof(Args[0]));
    }

  strcpy(Args[ACount++],MJobState[J->State]);

  MUStrCpy(Args[ACount++],
    (J->Credential.Q != NULL) ? J->Credential.Q->Name : (J->QOSRequested != NULL) ? J->QOSRequested->Name : (char *)NONE,
    sizeof(Args[0]));

  sprintf(Args[ACount++],"%ld",J->SubmitTime);
  sprintf(Args[ACount++],"%ld",J->StartTime);
  sprintf(Args[ACount++],"%ld",J->CompletionTime);
  sprintf(Args[ACount++],"%f",xfactor);
  sprintf(Args[ACount++],"%ld",J->WCLimit);
  sprintf(Args[ACount++],"%d",J->TotalProcCount);
  sprintf(Args[ACount++],"%d",RQ->DRes.Mem * RQ->TaskCount);

  sprintf(Args[ACount++],"%f",
    (double)((J->AWallTime > 0) ? J->PSUtilized / J->AWallTime : 0.0));

  if (RQ->MURes != NULL)
    {
    sprintf(Args[ACount++],"%f",(double)RQ->MURes->Procs / 100.0);
    }
  else
    {
    sprintf(Args[ACount++],"%f",0.0);
    }

  if (RQ->LURes != NULL)
    {
    sprintf(Args[ACount++],"%ld",
      (long)((J->AWallTime > 0) ? RQ->LURes->Mem / J->AWallTime : 0));
    }
  else
    {
    sprintf(Args[ACount++],"%d",0);
    }

  if (RQ->MURes != NULL)
    {
    sprintf(Args[ACount++],"%d",RQ->MURes->Mem);
    }
  else
    {
    sprintf(Args[ACount++],"%d",0);
    }

  if (J->MessageBuffer != NULL) 
    {
    ptr = MMBPrintMessages(J->MessageBuffer,mfmHuman,FALSE,0,NULL,0);

    if (ptr != NULL)
      {
      snprintf(Args[ACount++],sizeof(Args[ACount]),"\"%s\"",
        ptr);
      }
    else
      {
      snprintf(Args[ACount++],sizeof(Args[ACount]),"\"%s\"",
        NONE);
      }
    }
  else
    {
    snprintf(Args[ACount++],sizeof(Args[ACount]),"\"%s\"",
      NONE);
    }

  ACount++;

  ArgV[0] = NULL;  /* populated in MSysLaunchAction() */
 
  for (aindex = 0;aindex < ACount;aindex++)
    {
    ArgV[aindex + 1] = Args[aindex];
    }

  /* add hostlist */

  mstring_t HLBuf(MMAX_LINE);

  MJobAToMString(J,mjaHostList,&HLBuf);

  char *mutable1 = NULL;
  MUStrDup(&mutable1,HLBuf.c_str());

  ArgV[aindex++] = mutable1;

  /* add gres string */

  mstring_t GResBuf(MMAX_LINE);

  MJobGetGResString(J,&GResBuf);


  if (GResBuf.empty())
    {
    GResBuf += NONE;
    }

  char *mutable2 = NULL;
  MUStrDup(&mutable2,GResBuf.c_str());

  ArgV[aindex++] = mutable2;

  /* terminate list */
 
  ArgV[aindex] = NULL;

  if (MSysLaunchAction(ArgV,mactJobFB) == SUCCESS)
    {
    MDB(2,fCORE) MLog("INFO:     job usage sent for job '%s'\n",
      J->Name);
    }

  MUFree(&mutable1);
  MUFree(&mutable2);

  return(SUCCESS);
  }  /* END MJobSendFB() */





/**
 * Writes specific information about the setting of variables
 * to the event file.
 *
 * @param J (I)
 * @param Mode (I) Only mSet or mUnset is valid!!!
 * @param VarName (I)
 * @param VarVal (I)
 * @param CCode (I) [optional]
 */

int MJobWriteVarStats(

  mjob_t        *J,
  enum MObjectSetModeEnum Mode,
  const char    *VarName,
  const char    *VarVal,
  int           *CCode)

  {
  char tmpLine[MMAX_LINE * 2];

  if ((J == NULL) ||
      (VarName == NULL))
    {
    return(FAILURE);
    }

  if ((Mode != mSet) &&
      (Mode != mUnset))
    {
    return(FAILURE);
    }

  if (Mode == mSet)
    {
    snprintf(tmpLine,sizeof(tmpLine),"%s=%s",
      VarName,
      MPRINTNULL(VarVal));
    }
  else
    {
    snprintf(tmpLine,sizeof(tmpLine),"%s",
      VarName);
    }

  /* add event to checkpoint journal */

  MCPJournalAdd(
    mxoJob,
    J->Name,
    (char *)MJobAttr[mjaVariables],
    tmpLine,
    Mode);

  if ((Mode == mSet) && !bmisset(&MSched.RecordEventList,mrelJobVarSet))
    {
    return(FAILURE);
    }

  if ((Mode == mUnset) && !bmisset(&MSched.RecordEventList,mrelJobVarUnset))
    {
    return(FAILURE);
    }

  if (Mode == mSet)
    {
    if (CCode != NULL)
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s %s=%s (Setter's exit code=%d)", 
        (J->AName != NULL) ? J->AName : "NULL",
        VarName,
        MPRINTNULL(VarVal),
        *CCode);
      }
    else
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s %s=%s", 
        (J->AName != NULL) ? J->AName : "NULL",
        VarName,
        MPRINTNULL(VarVal));
      }
    }
  else
    {
    if (CCode != NULL)
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s %s (Unsetter's exit code=%d)",
        (J->AName != NULL) ? J->AName : "NULL",
        VarName,
        *CCode);
      }
    else
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s %s",
        (J->AName != NULL) ? J->AName : "NULL",
        VarName);
      }
    }

  if (MSched.PushEventsToWebService == TRUE)
    {
    MEventLog *Log = new MEventLog(meltJobModify);
    Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
    Log->AddDetail("operation",(Mode == mSet) ? "VarSet" : "VarUnset");
    Log->AddDetail("msg",tmpLine);

    MEventLogExportToWebServices(Log);

    delete Log;
    }

  if (Mode == mSet)
    {
    MSysRecordEvent(
      mxoJob,
      J->Name, 
      mrelJobVarSet,
      NULL,
      tmpLine,
      NULL);
    }
  else
    {
    MSysRecordEvent(
      mxoJob,
      J->Name, 
      mrelJobVarUnset,
      NULL,
      tmpLine,
      NULL);
    }

  return(SUCCESS);
  }  /* END MJobWriteVarStats() */





/**
 * check if exec user proxy is allowed in specified job
 *
 * @param J      (I) [modified]
 * @param EUName (I)
 */

mbool_t MJobAllowProxy(

  mjob_t *J,
  char   *EUName)

  {
  mbool_t AllowValidation = FALSE;

  if ((J == NULL) || (J->Credential.U == NULL) || (EUName == NULL))
    {
    return(FALSE);
    }

  if (J->Credential.U->Role == mcalAdmin1)
    {
    /* allow admin1 users to proxy */

    if ((J->Credential.U->ValidateProxy == TRUE) ||
       ((MSched.DefaultU != NULL) &&
        (MSched.DefaultU->ValidateProxy == TRUE)))
      {
      /* neither specific user nor default user proxy allowed */

      bmset(&J->IFlags,mjifIsValidated);
      }

    return(TRUE);
    }

  if ((J->Credential.U->ProxyList != NULL) &&
     (MUCheckStringList(J->Credential.U->ProxyList,EUName,',') == SUCCESS))
    {
    /* proxy explictly authorized */

    if ((J->Credential.U->ValidateProxy == TRUE) ||
       ((MSched.DefaultU != NULL) &&
        (MSched.DefaultU->ValidateProxy == TRUE)))
      {
      /* neither specific user nor default user proxy allowed */

      bmset(&J->IFlags,mjifIsValidated);
      }

    return(TRUE);
    }

  if ((J->Credential.U->ValidateProxy == TRUE) ||
     ((MSched.DefaultU != NULL) && 
      (MSched.DefaultU->ValidateProxy == TRUE)))
    {
    /* neither specific user nor default user proxy allowed */

    AllowValidation = TRUE;
    }

  if (AllowValidation == TRUE)
    {
    mrm_t *R = J->SubmitRM;

    if ((R != NULL) && (R->ND.URL[mrmXJobValidate] != NULL))
      {
      /* job validation will determine if proxy is allowed */

      return(MBNOTSET);
      }

    /* NOTE:  actual job validation will occur outside of this routine */
    }

  return(FALSE);
  }  /* END MJobAllowProxy() */





/**
 * Returns TRUE if R can affect resource allocation for J
 *
 * @see VMSchedulingDoc
 * @see MJobGetSNRange() - parent
 * @see MJobGetFeasibleResources() - peer
 * 
 * @param J (I)
 * @param N (I)
 * @param R (I)
 * @param NAccessPolicy (I)
 */

mbool_t MJobRsvApplies(

  const mjob_t  *J,
  const mnode_t *N,
  const mrsv_t  *R,
  enum MNodeAccessPolicyEnum NAccessPolicy)
  
  {
  mbool_t Result = TRUE;

 if ((J->ReqRID != NULL) && (MRsvIsReqRID(R,J->ReqRID) == FALSE))
    {
    /* if the job is requesting a reservation and we don't have access to THIS reservation
       then this reservation is applicable (needs to be enforced) */

    if (MRsvCheckJAccess(R,J,J->SpecWCLimit[0],NULL,TRUE,NULL,NULL,NULL,NULL) == TRUE)
      {
      /* AdvRsv job has access to this reservation but it's not the advrsv
         reservation so the rsv does not apply */

      Result = FALSE;
      }
    }
  else if (R->J != NULL)
    {
    if ((NAccessPolicy == mnacSingleTask) || (NAccessPolicy == mnacSingleJob))
      {
      Result = TRUE;
      }
    else if (MJOBREQUIRESVMS(J))
      {
      /* only static VM reservations can block static VM jobs */
      Result = MJOBREQUIRESVMS(R->J);
      }
#if 0
    else
      {
      /* this includes createvm jobs, vmcreate jobs, vmdestroy jobs,
          We only screen out jobs on currently existing permanent VMs
          (static, sovereign). VMDestroy jobs consume no resources so
          they can be treated either way.  */
      Result = (MJOBREQUIRESVMS(R->J) == FALSE) && 
               (MJobGetSystemType(R->J) != msjtVMMap);
      }
#endif
    }

  return(Result);
  }  /* END MJobRsvApplies() */



/**
 * Helper function for use with MUHTClear.
 */

int __MJobRemoveHT(

  void **J)

  {
  return (MJobRemove((mjob_t **)J));
  }

/* NOTE:  algorithm does the following:

      initialize global range

      for each node,
        for each req
          get avail range
          convert to start range
          offset range
          AND range
        merge resulting range with global range

      return total range map for nodes which can satisfy co-allocation 
      of at least one task of all reqs 

*/




/**
 *
 *
 * @param J (I)
 * @param RQ (I) [optional]
 * @param G (I/O)
 * @param RTBM (I) [bitmap of enum mrtc*]
 * @param RCount (O) [optional]
 */

int MJobSelectFRL(

  mjob_t    *J,
  mreq_t    *RQ,
  mrange_t  *G,
  mbitmap_t *RTBM,
  int       *RCount)

  {
  int  rindex;
  int  findex;

  int  TC;

  int  RTC;
  int  RNC;

  mulong OldETime;

  const char *FName = "MJobSelectFRL";

  MDB(5,fSCHED) MLog("%s(%s,%d,G,RTBM,RCount)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1);

  if (J == NULL)
    {
    return(FAILURE);
    }

  findex   = 0;

  TC       = 0;
  OldETime = 0;

  if (RQ != NULL)
    {
    RTC = RQ->TaskCount;
    RNC = RQ->NodeCount;
    }
  else
    {
    RTC = J->Request.TC;
    RNC = J->Request.NC;
    }
    
  if (!bmisset(RTBM,mrtcFeasible))
    {
    /* remove ranges without adequate tasks available */

    for (rindex = 0;G[rindex].ETime != 0;rindex++)
      {
      if ((G[rindex].TC >= RTC) &&
          (G[rindex].NC >= RNC))
        {
        /* adequate tasks/nodes available */

        if (findex != rindex)
          {
          /* change to avoid purify errors */

          /*
          memcpy(&G[findex],&G[rindex],sizeof(mrange_t));
          */

          G[findex].STime = G[rindex].STime;
          G[findex].ETime = G[rindex].ETime;
          G[findex].TC    = G[rindex].TC;
          G[findex].NC    = G[rindex].NC;
          G[findex].Aff   = G[rindex].Aff;
          G[findex].Cost  = G[rindex].Cost;
          G[findex].Duration = G[rindex].Duration;
          G[findex].R     = G[rindex].R;
          G[findex].HostList = G[rindex].HostList;
          }

        findex++;
        }
      else
        {
        MDB(5,fSCHED) MLog("INFO:     G[%d] has inadequate resources (T: %d < %d) || (N: %d < %d)\n",
          rindex,
          G[rindex].TC,
          RTC,
          G[rindex].NC,
          RNC);
        }
      }    /* END for (rindex) */

    G[findex].ETime = 0;

    if (RCount != NULL)
      *RCount = findex;

    MDB(7,fSCHED)
      {
      for (findex = 0;G[findex].ETime != 0;findex++)
        {
        MLog("INFO:     G[%02d]  S: %ld  E: %ld  T: %3d  N: %d\n",
          findex,
          G[findex].STime,
          G[findex].ETime,
          G[findex].TC,
          G[findex].NC);
        }
      }

    return(SUCCESS);
    }  /* END if (!bmisset(RTBM,mrtcFeasible)) */

  if (!bmisset(&J->Flags,mjfBestEffort))
    {
    /* remove ranges without adequate tasks/nodes and combine remaining ranges */

    for (rindex = 0;G[rindex].ETime != 0;rindex++)
      {
      if ((TC != 0) &&
         ((G[rindex].TC < RTC) ||
          (G[rindex].NC < RNC) ||
          (G[rindex].STime > OldETime)))
        {
        /* terminate existing range */

        G[findex].ETime = OldETime;
        findex++;

        TC = 0;
        }

      if ((G[rindex].TC >= RTC) &&
          (G[rindex].NC >= RNC))
        {
        OldETime = G[rindex].ETime;

        if (TC == 0)
          {
          /* start new range */

          G[findex].STime = G[rindex].STime;
          G[findex].TC = RTC;

          if (RNC > 0)
            G[findex].NC = RNC;
          else
            G[findex].NC = G[rindex].NC;

          OldETime = G[rindex].ETime;
          TC       = RTC;
          }
        }
      }      /* END for (rindex) */
    }        /* END if (!bmisset(&J->Flags,mjfBestEffort)) */

  if (TC > 0)
    {
    G[findex].ETime = OldETime;
    findex++;
    }

  G[findex].ETime = 0;

  if (RCount != NULL)
    *RCount = findex;

  MDB(7,fSCHED)
    {
    for (findex = 0;G[findex].ETime != 0;findex++)
      {
      MLog("INFO:     G[%02d]  S: %ld  E: %ld  T: %3d  N: %d\n",
        findex,
        G[findex].STime,
        G[findex].ETime,
        G[findex].TC,
        G[findex].NC);
      }
    }

  return(SUCCESS);
  }  /* MJobSelectFRL() */



/**
 *
 *
 * @param String (I)
 */

int MJobTestName(

  char *String)  /* I */

  {
  enum MRMTypeEnum rmtindex;

  char *ptr;
  char *TokPtr;

  char  SJobName[MMAX_NAME];
  char  LJobName[MMAX_NAME];

  mrm_t tmpR;

  if ((String == NULL) || ((ptr = MUStrTok(String,":",&TokPtr)) == NULL))
    {
    exit(1);
    }

  /* FORMAT:  <RM>:<JOBID> */

  rmtindex = (enum MRMTypeEnum)MUGetIndex(ptr,MRMType,FALSE,0);

  if ((ptr = MUStrTok(NULL,":",&TokPtr)) == NULL)
    {
    exit(1);
    }

  memset(&tmpR,0,sizeof(tmpR));

  tmpR.Type = rmtindex;

  /* make short name */

  MJobGetName(NULL,ptr,&tmpR,SJobName,sizeof(SJobName),mjnShortName);

  /* make long name */

  MJobGetName(NULL,SJobName,&tmpR,LJobName,sizeof(LJobName),mjnRMName);

  fprintf(stderr,"RM: '%s'  Orig: '%s'  S: '%s'  L: '%s'  QMHost: '%s'  DD: '%s'\n",
    MRMType[rmtindex],
    ptr,
    SJobName,
    LJobName,
    tmpR.DefaultQMHost,
    MSched.DefaultDomain);

  if (strcmp(ptr,LJobName))
    {
    /* job names do not match */

    exit(1);
    }
  
  exit(0);

  /*NOTREACHED*/

  return(SUCCESS);
  }  /* END MJobTestName() */




/**
 * Allocate continguous best-fit nodes.
 *
 * @param J (I) job allocating nodes
 * @param RQ (I) req allocating nodes
 * @param NodeList (I) eligible nodes
 * @param RQIndex (I) index of job req to evaluate
 * @param MinTPN (I) min tasks per node allowed
 * @param MaxTPN (I) max tasks per node allowed
 * @param NodeMap (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex (I/O) index of next node to find in BestList
 * @param BestList 
 * @param TaskCount (I/O) [optional]
 * @param NodeCount (I/O) [optional]
 */

int MJobAllocateContiguous(

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
  int nindex;
  int TC;

  mnode_t *N;

  mnl_t       MyNodeList = {0};
  int         MyNIndex;

  mnl_t       adjacentList = {0};

  /* select first 'RQ->TaskCount' adjacent procs */

  MyNIndex = 0;

  MNLInit(&MyNodeList);
  MNLInit(&adjacentList);

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

    /* add node to private list */

    MNLSetTCAtIndex(&MyNodeList,MyNIndex,TC);
    MNLSetNodeAtIndex(&MyNodeList,MyNIndex,N);

    MyNIndex++;
    }  /* END for (nindex) */

  if (MyNIndex == 0)
    {
    MNLFree(&MyNodeList);
    MNLFree(&adjacentList);

    /* no nodes located */

    return(FAILURE);
    }

  MNLTerminateAtIndex(&MyNodeList,MyNIndex);

  /* select adjacent nodes */

  MNLGetNodeAtIndex(NodeList,0,&N);

  if (MNLSelectContiguousBestFit(
        N->PtIndex,
        J->Request.TC,
        &MyNodeList,     /* I */
        &adjacentList) == FAILURE)
    {
    /* cannot locate adequate adjacent nodes */
    MNLFree(&MyNodeList);
    MNLFree(&adjacentList);

    return(FAILURE);
    }

  /* populate BestList with selected nodes */

  for (nindex = 0;MNLGetNodeAtIndex(&adjacentList,nindex,&N) == SUCCESS;nindex++)
    {
    TC = MNLGetTCAtIndex(&adjacentList,nindex);

    MNLSetNodeAtIndex(BestList[RQIndex],NodeIndex[RQIndex],N);
    MNLSetTCAtIndex(BestList[RQIndex],NodeIndex[RQIndex],TC);

    NodeIndex[RQIndex] ++;
    TaskCount[RQIndex] += TC;
    NodeCount[RQIndex] ++;

    /* mark node as used */

    NodeMap[N->Index] = mnmUnavailable;

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

  MNLFree(&MyNodeList);
  MNLFree(&adjacentList);

  return(SUCCESS);
  }  /* END MJobAllocateContiguous() */





/**
 * Allocate memory for core template/pseudo-job.
 *
 * @param JP (O) [alloc]
 * @param RQP (O) [optional]
 */

int MJobAllocBase(

  mjob_t **JP,  /* O (alloc) */
  mreq_t **RQP) /* O (optional) */

  {
  mjob_t *J;

  if (RQP != NULL)
    *RQP = NULL;

  if (JP == NULL)
    {
    return(FAILURE);
    }
 
  J = *JP;

  if ((J == NULL) &&
     ((J = (mjob_t *)MUCalloc(1,sizeof(mjob_t))) == NULL))
    {
    return(FAILURE);
    }

  *JP = J;

  /* Why not MReqCreate? */
  /* Yea, why not?  It's cramping my style. */

  if ((J->Req[0] == NULL) &&
     ((J->Req[0] = (mreq_t *)MUCalloc(1,sizeof(mreq_t))) == NULL))
    {
    MUFree((char **)JP);

    return(FAILURE);
    }

  MSNLInit(&J->Req[0]->DRes.GenericRes);

  /* NOTE: distinguish jobs from VPC's regarding nodeset management */

  J->Req[0]->SetBlock = MBNOTSET;

  if (RQP != NULL)
    *RQP = J->Req[0];

  return(SUCCESS);
  }  /* END MJobAllocBase() */







/**
 *
 *
 * @param J (I)
 * @param IsSet (I)
 * @param BM (I)
 */

int MJobApproveFlags(

  mjob_t     *J,     /* I */
  mbool_t     IsSet, /* I */
  mbitmap_t  *BM)    /* I */

  {
  int index;

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (BM == 0)
    {
    return(SUCCESS);
    }

  /* check who is setting the flag */
  /* check if user is authorized to set the flag */

  /* only enabled */

  for (index = 0;index < mjfLAST;index++)
    {
    if (!bmisset(BM,index))
      continue;

    switch (index)
      {
      case mjfPreemptee:
      case mjfRestartable:
      case mjfAdvRsv:

        /* NOTE: not all unsets should be authorized, ie must handle mjfAdvRsv (NYI) */

        /* NO-OP */

        break;

      default:

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }
    }    /* END for (index) */

  return(SUCCESS);
  } /* END MJobApproveFlags() */




/**
 * Translate a full tasklist into an NCPU (DRes.Procs > 1) style tasklist.
 *
 * NOTE: This assumes the list is ordered.
 *       Works in conjunction with MPBSJobGetGPUExecHList() to ensure
 *       GPU jobs don't get messed up on recycle or update.
 *
 * Example: 1 1 1 1 1 1 5 5 5 6 6 6 (DRes.Procs = 3)
 *            =
 *          1 1 5 6
 *
 */

int MJobTranslateNCPUTaskList(

  mjob_t *J,
  int    *TaskList)

  {
  int TaskProc;
  int tindex;

  int ITIndex; /* insertion index */
  int TC;

  int nindex;

  if ((J == NULL) || (TaskList == NULL) || (J->Req[0] == NULL))
    {
    return(FAILURE);
    }

  TaskProc = J->Req[0]->DRes.Procs;

  if (TaskProc == 1)
    return(SUCCESS);

  /* at this point, RQ->DRes.Procs should be populated with appropriate value */

  nindex = MNode[TaskList[0]]->Index;

  ITIndex = 0;
  TC = 0;

  for (tindex = 0;TaskList[tindex] != -1;tindex++)
    {
    if (TaskList[tindex] == nindex)
      {
      TC++;

      if (TC == TaskProc)
        {
        TC = 0;

        TaskList[ITIndex] = nindex;

        ITIndex++;
        }
      }
    else
      {
      /* we shouldn't hit this situation because the list is in order
         and we must assume moab started the job on the right nodes. */

      TC = 1;

      nindex = TaskList[tindex];
      }
    }  /* END for (tindex) */

  TaskList[ITIndex] = -1;

  return(SUCCESS);
  }  /* END MJobTranslateNCPUTaskList() */



/**
 * Report job template statistics to XML.
 *
 * @param TJName (I) [optional]
 * @param DEP (O)
 */

int MUITJobStatToXML(

  char    *TJName,  /* I (optional) */
  mxml_t **DEP)     /* O */

  {
  mxml_t *DE = NULL;
  mxml_t *JE;
  mxml_t *CE;  /* child element */

  mjob_t *TJ;

  mbool_t JobSpecified;

  mbool_t StatLocated = FALSE;

  int tjindex;

  if (DEP == NULL)
    {
    return(FAILURE);
    }

  *DEP = NULL;

  if ((TJName == NULL) || (TJName[0] == '\0') || !strcmp(TJName,NONE))
    JobSpecified = FALSE;
  else
    JobSpecified = TRUE;

  MXMLCreateE(&DE,(char *)MSON[msonData]);

  for (tjindex = 0;MUArrayListGet(&MTJob,tjindex) != NULL;tjindex++)
    {
    TJ = *(mjob_t **)(MUArrayListGet(&MTJob,tjindex));

    if (TJ == NULL)
      break;

    if (JobSpecified == TRUE)
      {
      if (strcmp(TJ->Name,TJName))
        continue;
      }

    StatLocated = TRUE;
 
    if (!bmisset(&TJ->IFlags,mjifDataOnly))
      continue;

    JE = NULL;

    MXMLCreateE(&JE,(char *)MXO[mxoJob]);

    MXMLAddE(DE,JE);

    MXMLSetAttr(
      JE,
      (char *)MJobAttr[mjaJobID],
      TJ->Name,
      mdfString);

    /* send wallclock accuracy, efficiency, walltime used, fs usage, job information */

    if (TJ->ExtensionData != NULL)
      {
      mtjobstat_t *TJS = (mtjobstat_t *)TJ->ExtensionData;

      if (MFSToXML(&TJS->F,&CE,NULL) == SUCCESS)
        {
        MXMLAddE(JE,CE);
        }

      if (MStatToXML(
           (void *)&TJS->S,
           msoCred,
           &CE,
           TRUE,
           NULL) != FAILURE)
        {
        MXMLAddE(JE,CE);
        }

      /* this segment gives active job/proc/prochours information */
      if (MLimitToXML(&TJS->L,&CE,NULL) == SUCCESS)
        {
        MXMLAddE(JE,CE);
        }
      }    /* END if (TJ->xd != NULL) */
    }      /* END for (jtindex) */

  *DEP = DE;

  if (StatLocated == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MTJobStatToXML() */


/**
 *
 *
 * @param J (I) [modified]
 */

int MJobFreeSID(

  mjob_t *J)  /* I (modified) */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  MUFree(&J->SystemID); 
  MUFree(&J->SystemJID);

  if (J->Grid != NULL)
    {
    int index;

    for (index = 0;index < MMAX_JSHOP;index++)
      {
      if (J->Grid->SID[index] != NULL)
        MUFree(&J->Grid->SID[index]);

      if (J->Grid->SJID[index] != NULL)
        MUFree(&J->Grid->SJID[index]);
      }  /* END for (index) */

    MUFree((char **)&J->Grid->User);
    MUFree((char **)&J->Grid->Group);
    MUFree((char **)&J->Grid->Account);
    MUFree((char **)&J->Grid->Class);
    MUFree((char **)&J->Grid->QOS);

    MUFree((char **)&J->Grid);
    }   /* END if (J->G != NULL) */

  return(SUCCESS);
  }  /* END MJobFreeSID() */






/**
 * Load completed job info from checkpoint store.
 *
 * @param JS (I)  Job to populate--must be on the heap. [optional]
 * @param Buf (I) [required]
 */

int MCJobLoadCP(

  char   *Buf) /* I (required) */

  {
  int rc;

  mjob_t   *J = NULL;

  char     tmpName[MMAX_NAME + 1];
  char     JobID[MMAX_NAME + 1];

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  rc = sscanf(Buf,"%64s %64s",
    tmpName,
    JobID);
 
  if (rc == 2)
    {
    char EMsg[MMAX_LINE];

    if (MJobCreate(JobID,FALSE,&J,EMsg) == FAILURE)
      {
      MDB(5,fSCHED) MLog("WARNING:  could not create job record for completed job %s - %s\n",
        JobID,
        EMsg);

      return(FAILURE);
      }

    /* Note that MJobLoadCPNew will fill in J */
    }  /* END else (JS != NULL) */
  else
    {
    MDB(5,fSCHED) MLog("WARNING:  could not process completed job from checkpoint file\n");

    return(FAILURE);
    }

  /* Note - MJobLoadCPNew will malloc a Req structure when it calls MJobFromXML
   *        and it will copy that to J->Req[0]. Note that this memory must be freed
   *        before we return from this routine.                                      */

  if (MJobLoadCPNew(J,Buf) == FAILURE)
    {
    MDB(5,fSCHED) MLog("ALERT:    cannot load checkpoint completed job\n");

    MJobDestroy(&J);

    return(FAILURE);
    } /* END if (MJobLoadCPNew(J,Buf) == FAILURE) */

  if (MQueueAddCJob(J) == FAILURE)
    {
    MDB(5,fSCHED) MLog("ALERT:    cannot add completed job %s to checkpoint queue\n",
      J->Name);

    MJobDestroy(&J);

    return(FAILURE);
    } /* END if (MQueueAddCJob(J) == FAILURE) */

  /* MQueueAddCJob (as called above) calls MJobTransition with a limited transition.
   * We need a full transition when loading from the checkpoint. */

  MJobTransition(J,FALSE,FALSE);

  /* don't destroy the job as it now resides in the MCJobHT */

  return(SUCCESS);
  }  /* END MCJobLoadCP() */





/**
 * Takes a given job and removes all indications that the job was ever in a non-idle state.
 * (Removes any reservations, references to RMs, destroys node list, etc.)
 * The job's state is returned to "Idle" and should now be reconsidered as an eligible job.
 * Note that this function does NOT remove the job from remote RMs where it was migrated to--
 * it only removes any references to remote RMs.
 *
 * @param J (I) The job that should be reset to the Idle state.
 * @param DoClearHolds (I) Whether or not the job's holds should be cleared.
 * @param DoClearDRM (I) If TRUE then DRM and DRMJID will be cleared.
 * @see MJobCancel(), MJobDisableParAccess()
 */

int MJobToIdle(

  mjob_t  *J,             /* I */
  mbool_t  DoClearHolds,  /* I */
  mbool_t  DoClearDRM)    /* I */

  {
  mbitmap_t tmpBM;
  int rqindex;

  mreq_t *RQ;

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* send job back to idle state */

  MNLClear(&J->NodeList);

  /*
  if ((J->Req[0] != NULL) &&
      (J->Req[0]->NodeList != NULL))
    {
    memset(J->Req[0]->NodeList,0,sizeof(MMAX_NODE + 1));
    }
  */

  if (DoClearDRM == TRUE)
    {
    MJobSetAttr(J,mjaDRM,(void **)NULL,mdfOther,mSet);
    MJobSetAttr(J,mjaDRMJID,(void **)NULL,mdfOther,mSet);
    }

  MQueueRemoveAJob(J,mstNONE);  

  if (DoClearHolds == TRUE)
    MJobSetAttr(J,mjaHold,(void **)&tmpBM,mdfOther,mSet);  /* clear holds */

  MJobReleaseRsv(J,TRUE,TRUE);

  /* remove references to other RMs */

  if (DoClearDRM == TRUE)
    {
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        {
        /* end of list located */
          
        break;
        }

      RQ->RMIndex = -1;

      MUFree(&RQ->ReqJID);
      }
    }
    
  MJobSetState(J,mjsIdle);

  MJobLaunchEpilog(J);

  bmset(&J->IFlags,mjifRanEpilog);

  bmunset(&J->IFlags,mjifWasCanceled);

  return(SUCCESS);
  }  /* END MJobToIdle() */




/**
 * Remove partition from list of partitions available to job.
 *
 * @param J (I) [modified]
 * @param P (I)
 */

int MJobDisableParAccess(

  mjob_t *J,  /* I (modified) */
  mpar_t *P)  /* I */

  {
  int pindex;
  mbool_t FoundEligiblePartition = FALSE;

  mpar_t *tP;
  
  const char *FName = "MJobDisableParAccess";

  MDB(3,fCKPT) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL");

  if ((J == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  /* eliminate partition from scheduling */

  bmunset(&J->SysPAL,P->Index);

  /* ensure that there are valid partitions still available */

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    tP = &MPar[pindex];

    if (tP->Name[0] == '\0')
      break;

    if (tP->Name[0] == '\1')
      continue;

    if (tP == P)
      continue;

    if (bmisset(&J->SysPAL,tP->Index))
      {
      if ((tP->RM == NULL) ||
          (tP->RM->State == mrmsActive))
        {
        /* there is still a partition we can use that's not attached to a dead RM */

        FoundEligiblePartition = TRUE;

        break;
        }
      }
    }   /* END for (pindex) */

  /* NOTE: when last partition is disabled, only MPar[0] available */

  if ((bmisclear(&J->SysPAL)) ||
      (FoundEligiblePartition == FALSE))
    {
    /* if no partitions are left, place message on job and block it */

    MMBAdd(&J->MessageBuffer,"all partitions eliminated/ineligible",NULL,mmbtNONE,0,0,NULL);

    MJobSetHold(J,mhBatch,0,mhrRMReject,"all partitions eliminated/ineligible");    
    }

  /* update PAL to reflect changes */

  bmand(&J->PAL,&J->SysPAL);

  return(SUCCESS);
  }  /* END MJobDisableParAccess() */





/**
 * Dynamically increase size of job taskmap.
 *
 * NOTE: modifies mjob_t.TaskMap,mjob_t.TaskMapSize,mjob_t.VMTaskMap
 *
 * @param J (I) [modified]
 * @param STCount (I) [incremental growth]
 */

int MJobGrowTaskMap(

  mjob_t *J,       /* I (modified) */
  int     STCount) /* I (incremental growth) */

  {
  int TCount;  /* number of tasks to grow tasklist size by */
  int tindex;

  const char *FName = "MJobGrowTaskMap";

  MDB(7,fSTRUCT) MLog("%s(%s,%d)\n",
    FName,
    (J != NULL) ? J->Name : "",
    STCount);

  if (J == NULL)
    {
    return(FAILURE);
    }

  TCount = (STCount > 0) ? STCount : MDEF_JOBTASKMAPINCRSIZE;

  if (TCount > MSched.JobMaxTaskCount)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    job %s taskmap cannot be extended (request exceeds %d)\n",
      J->Name,
      MSched.JobMaxTaskCount);

    /* NOTE: preserve/report TPJ overflow */

    if (MSched.OFOID[mxoxTasksPerJob] == NULL)
      MUStrDup(&MSched.OFOID[mxoxTasksPerJob],J->Name);

    return(FAILURE);
    }

  TCount = MIN(TCount,MSched.JobMaxTaskCount - J->TaskMapSize);

  if (TCount <= 0)
    {
    /* cannot increase job taskcount to requested size */

    MDB(1,fSTRUCT) MLog("ALERT:    job %s taskmap cannot be extended (request exceeds %d)\n",
      J->Name,
      MSched.JobMaxTaskCount);

    /* NOTE: preserve/report TPJ overflow */

    if (MSched.OFOID[mxoxTasksPerJob] == NULL)
      MUStrDup(&MSched.OFOID[mxoxTasksPerJob],J->Name);

    return(FAILURE);
    }

  if ((J->TaskMap == NULL) || (J->TaskMapSize <= 0))
    {
    J->TaskMap = (int *)MUCalloc(1,sizeof(int) * (TCount + 1));

    if (J->TaskMap == NULL)
      {
      J->TaskMapSize = 0;

      MDB(1,fSTRUCT) MLog("ALERT:    job %s taskmap cannot be extended (calloc failed, errno=%d)\n",
        J->Name,
        errno);

      return(FAILURE);
      }

    J->TaskMapSize = TCount;

    /* There is a problem somewhere with losing the -1 terminator. For now initialize
     * all the entries to -1 to prevent mystery core dumps. (FIXME) */

    for (tindex = 0; tindex <= TCount; tindex++)
      J->TaskMap[tindex] = -1;

    return(SUCCESS);
    }  /* END if ((J->TaskMap == NULL) || (J->TaskMapSize <= 0)) */

  /* extend existing TaskMap attributes */

  J->TaskMap = (int *)realloc(
    (void *)J->TaskMap,
    (J->TaskMapSize + TCount + 1) * sizeof(int));

  if (J->TaskMap == NULL)
    {
    J->TaskMapSize = 0;

    MDB(1,fSTRUCT) MLog("ALERT:    job %s taskmap cannot be extended (realloc failed, errno=%d)\n",
      J->Name,
      errno);

    return(FAILURE);
    }

  /* There is a problem somewhere with losing the -1 terminator. For now 
   * initialize the entries from the last valid entry to the end of the taskmap
   * to -1 to prevent mystery core dumps. (FIXME) */

  for (tindex = 0; tindex <= TCount; tindex++)
    J->TaskMap[J->TaskMapSize + tindex] = -1;

  J->TaskMapSize += TCount;

  MDB(4,fSTRUCT) MLog("INFO:     job %s taskmap extended to size %d\n",
    J->Name,
    J->TaskMapSize);

  return(SUCCESS);
  }  /* END MJobGrowTaskMap() */






/**
 * Insert Loadleveler directive into command file.
 *
 * @param AName (I)
 * @param AVal (I) [FORMAT:  <ATTR>[=<VAL>]]
 * @param SrcBuf (I)
 * @param DstBuf (O)
 * @param DstBufSize (I)
 */

int MJobLLInsertArg(

  const char *AName,
  char       *AVal,
  char       *SrcBuf,
  char       *DstBuf,
  int         DstBufSize)

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  char tmpLine[MMAX_LINE];

  char tmpBuffer[MMAX_SCRIPT];

  const char *FName = "MJobLLInsertArg";

  MDB(3,fRM) MLog("%s(%s,%s,SrcBuf,DstBuf,%d)\n",
    FName,
    AName,
    (AVal == NULL) ? "" : AVal,
    DstBufSize);

  if ((AName == NULL) || (SrcBuf == NULL) || (DstBuf == NULL))
    {
    return(FAILURE);
    }

  /* create new attribute line */

  snprintf(tmpLine,sizeof(tmpLine),"%s#@%s%s%s\n",
    (DstBuf[0] == '#') ? "\n" : "",  /* we are trying to preserve script magic */
    AName,
    (AVal != NULL) ? "=" : "",       /* handle empty value */
    (AVal != NULL) ? AVal : "");     /* handle empty value */

  /* need to cropy SrcBuf BEFORE we possible overwrite it with MUSNInit */

  MUStrCpy(tmpBuffer,SrcBuf,sizeof(tmpBuffer));

  MUSNInit(&BPtr,&BSpace,DstBuf,DstBufSize);

  /* NOTE: all attrs must happen before #@queue */
  /* should LL attr comments occur before any non-comment lines? */

  if (DstBuf == SrcBuf)
    {
    char *ptr;
    char *TokPtr;

    ptr = MUStrTok(tmpBuffer,"\n",&TokPtr);

    while ((ptr != NULL) && (ptr[0] == '#'))
      {
      /* NOTE:  '# @ queue' is also legal, should use MUIsRMDirective() - NYI */

      if (strstr(ptr,"@queue") != NULL)
        {
        /* must insert attrs before queue */

        MUSNPrintF(&BPtr,&BSpace,"%s\n%s\n%s",
          tmpLine,
          (ptr == NULL) ? " " : ptr,
          (TokPtr == NULL) ? " " : TokPtr);

        return(SUCCESS);
        }

      MUSNPrintF(&BPtr,&BSpace,"%s\n",ptr);

      ptr = MUStrTok(NULL,"\n",&TokPtr);
      }  /* END while ((ptr != NULL) && ...) */

    MUSNPrintF(&BPtr,&BSpace,"%s\n%s\n%s",
      tmpLine,
      (ptr == NULL) ? " " : ptr,
      (TokPtr == NULL) ? " " : TokPtr);

    return(SUCCESS);
    }  /* END if (DstBuf == SrcBuf) */

  /* insert new comment at beginning of command file */

  /* NOTE:  what about script magic? (NYI) */

  MUSNPrintF(&BPtr,&BSpace,"%s%s",
    tmpLine,
    SrcBuf);

  return(SUCCESS);
  }  /* END MJobLLInsertArg() */



/**
 * Insert environment variable into base job environment - propogate to RM
 * if supported.
 *
 * @see MJobAddEnvVar() - peer - does not push new env var to RM.
 *
 * @param J (I) [modified]
 * @param Var (I)
 * @param Val (I) [optional]
 */

int MJobInsertEnv(

  mjob_t *J,   /* I (modified) */
  char   *Var, /* I */
  char   *Val) /* I (optional) */

  {
  /*char tmpBuf[MMAX_BUFFER];*/

  /*char **EPtrP;*/

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (Var == NULL)
    {
    return(SUCCESS);
    }

  MJobAddEnvVar(
      J,
      Var,
      Val,
      ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtPBS)) ? ',' : ENVRS_ENCODED_CHAR);

  if (J->DestinationRM != NULL)
    {
    switch (J->DestinationRM->Type)
      {
      case mrmtPBS:

        if (J->DestinationRM->Version < 253)
          {
          MRMJobModify(J,"Variable_List",NULL,J->Env.BaseEnv,mSet,NULL,NULL,NULL);
          }
        else
          {
          /* Torque 2.5.3 supports incremental additions. */

          char tmpVar[MMAX_LINE];

          snprintf(tmpVar,sizeof(tmpVar),"%s=%s",
              Var,
              (Val != NULL) ? Val : "");

          MRMJobModify(J,"Variable_List",NULL,tmpVar,mIncr,NULL,NULL,NULL);
          }

        break;

      case mrmtMoab:

        break;

      case mrmtS3:

        /* no operation required */

        break;

      default:

        /* NOT-SUPPORTED */

        break;
      }
    }    /* END if (J->DRM != NULL) */
 
  return(SUCCESS);
  }  /* END MJobInsertEnv() */





/**
 * Create a psuedo-job from JSpec, or, if JSpec represents the ID of a real job,
 * duplicate the job with ID JSpec. *JP must be destroyed with MJobDestroy
 * to avoid memory leaks.
 *
 * @param JSpec               (I)
 * @param JP                  (I/O) [modified]
 * @param *RE                 (I) [optional] 
 * @param *Auth               (I) [optional] 
 * @param *OrigJP             (I) [optional] 
 * @param EMsg                (O) [optional]
 * @param EMsgSize            (I) 
 * @param CreatedDuplicateJob (O) [optional] 
 */

int MJobFromJSpec(

  char      *JSpec,
  mjob_t   **JP,
  mxml_t    *RE,
  char      *Auth,
  mjob_t   **OrigJP,
  char      *EMsg,
  int        EMsgSize,
  mbool_t   *CreatedDuplicateJob)

  {
  mjob_t *J;

  int    ReqTaskCount;
  long   ReqDuration;

  int    pindex;

  if (OrigJP != NULL)
    {
    *OrigJP = NULL;
    }

  if ((JSpec == NULL) ||      
      (JP == NULL) ||
      (*JP == NULL))
    {
    return(FAILURE);
    }

  if (CreatedDuplicateJob != NULL)
    {
    *CreatedDuplicateJob = FALSE;
    }

  /* NOTE: JP should originally point to a temp job as it is passed into this function,
   * but during the course of execution this function may use JP to pass out an already
   * allocated, and valid, job */

  if ((strchr(JSpec,'@') == NULL) && (strchr(JSpec,'<') == NULL))
    {
    /* FORMAT:  <JOBID> */

    MDB(5,fUI) MLog("INFO:     attempting to locate job '%s'\n",
      JSpec);

    if (MJobFind(JSpec,&J,mjsmExtended) == FAILURE)
      {
      MDB(2,fUI) MLog("WARNING:  cannot locate job '%s'\n",
        JSpec);

      if (EMsg != NULL)
        {
        if (!strcmp(JSpec,"ALL"))
          {
          snprintf(EMsg,EMsgSize,"Currently there are no active jobs\n");
          }
        else
          {
          snprintf(EMsg,EMsgSize,"ERROR:    cannot locate job '%s'\n",
            JSpec);
          }
        }

      return(FAILURE);
      }

    /* since some items are not copied into the duplicate job, return the job index so that the calling routine
       can look at the job in MJob[] to get the information that is not copied */

    if (OrigJP != NULL)
      *OrigJP = J;

    MJobDuplicate(*JP,J,FALSE);

    if (CreatedDuplicateJob != NULL)
      *CreatedDuplicateJob = TRUE;

    J = *JP;
    }  /* END if (strchr(JSpec,'@') == NULL) */
  else if ((strchr(JSpec,'@') != NULL) && (strchr(JSpec,'<') == NULL))
    {
    int CTok;

    char tmpLine[MMAX_LINE];
    char tmpName[MMAX_NAME];

    mgcred_t *U = NULL;

    char *ptr;
    char *TokPtr;

    /* FORMAT:  <PROCCOUNT>@<WALLTIME> */

    J = *JP;

    strcpy(J->Name,JSpec);

    ReqTaskCount = 1;
    ReqDuration  = 1;

    if ((ptr = MUStrTok(JSpec,"@",&TokPtr)) != NULL)
      {
      ReqTaskCount = (int)strtol(ptr,NULL,0);

      if ((ptr = MUStrTok(NULL,"@",&TokPtr)) != NULL)
        {
        ReqDuration = MUTimeFromString(ptr);
        }
      else
        {
        ReqDuration = 1;
        }
      }

    strcpy(JSpec,J->Name);

    /* allocate auxillary structures */

    J->Req[0]->DRes.Procs = 1;

    J->Req[0]->SetBlock   = MBNOTSET;

    bmset(&J->IFlags,mjifReqIsAllocated);

    strcpy(J->Name,JSpec);

    bmset(&J->IFlags,mjifIsTemplate);

    /* allow no credential access */

    J->SystemQueueTime = MSched.Time;

    J->Request.TC     = ReqTaskCount;
    J->SpecWCLimit[0] = ReqDuration;

    J->Req[0]->TaskCount = ReqTaskCount;

    for (pindex = 0; pindex < MMAX_PAR; pindex++)
      MJobSetStartPriority(J,pindex,1);

    MUserFind(Auth,&U);

    if (U != NULL)
      {
      J->Credential.U = U;
      J->Credential.G = U->F.GDef;
      }

    CTok = -1;

    while (MS3GetWhere(
          RE,
          NULL,
          &CTok,
          tmpName,
          sizeof(tmpName),
          tmpLine,
          sizeof(tmpLine)) == SUCCESS)
      {
      if (!strcmp(tmpName,"class"))
        {
        MClassFind(tmpLine,&J->Credential.C);
        }
      }
   
    MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
    }  /* END else if ((strchr(JSpec,'@') != NULL) && (strchr(JSpec,'<') == NULL)) */
  else
    {
    J = *JP;

    /* job is in SSS xml format */

    char tmpLine[MMAX_LINE];

    mxml_t *JE = NULL;

    if (MXMLFromString(&JE,JSpec,NULL,tmpLine) == FAILURE)
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,EMsgSize,"ERROR:    invalid XML specified '%s' '%s'\n",
          JSpec,
          tmpLine);
        }

      MJobFreeTemp(&J);

      return(FAILURE);
      }

    if (MS3JobFromXML(JE,&J,NULL) == FAILURE)
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,EMsgSize,"ERROR:    invalid SSS job XML specified '%s' - '%s'\n",
          JSpec,
          tmpLine);
        }

      MJobFreeTemp(&J);
      MXMLDestroyE(&JE);

      return(FAILURE);
      }

    MXMLDestroyE(&JE);

    J->SystemQueueTime = MSched.Time;
    
    /* Job needs to have access to all partitions as the grid head doesn't know
     * this peers partitions. */
    
    MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
    }  /* END else (strchr(JSpec,'@') != NULL) */

  if (RE != NULL)
    {
    char tmpLine[MMAX_LINE];

    MXMLGetAttr(RE,(char *)MJobAttr[mjaAccount],NULL,tmpLine,sizeof(tmpLine));

    if (tmpLine[0] != '\0')
      {
      mgcred_t *A = NULL;

      MAcctFind(tmpLine,&A);

      /* check account access */

      if (MCredCheckAcctAccess(J->Credential.U,J->Credential.G,A) == SUCCESS)
        {
        J->Credential.A = A;
        }
      else
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:  cannot submit job - invalid Account requested\n");

        MJobFreeTemp(&J);

        return(FAILURE);
        }
      }  /* END if (tmpLine[0] != '\0') */
    else
      {
      MJobGetAccount(J,&J->Credential.A);
      }

    MXMLGetAttr(RE,"Extension",NULL,tmpLine,sizeof(tmpLine));

    if (tmpLine[0] != '\0')
      {
      MJobProcessExtensionString(J,tmpLine,mxaNONE,NULL,NULL);

      if ((J->QOSRequested != NULL) &&
          (J->QOSRequested != J->Credential.Q) &&
          !bmisset(&J->IFlags,mjifFBQOSInUse) &&
          !bmisset(&J->IFlags,mjifFBAcctInUse))
        {
        if (MQOSGetAccess(J,J->QOSRequested,NULL,NULL,NULL) == FAILURE)
          {
          snprintf(EMsg,MMAX_LINE,"ERROR:  cannot submit job - invalid QOS requested\n");
          MJobFreeTemp(&J);

          return(FAILURE);
          }

        MJobSetQOS(J,J->QOSRequested,0);
        }
      }  /* END if (tmpLine[0] != 0) */
    }    /* END if (RE != NULL)  */

  return(SUCCESS);
  }  /* END MJobFromJSpec() */


/**
 * Assign nodes to a given job updating job nodelist and taskmap.
 *
 * Note: J->Req[index]->NodeList should already be populated at this point.
 * Routine does not consider node state or eligibility.
 * Populates {STaskMap|J->TaskMap} and J->Req[x]->NodeList attributes.

 * <p>NOTE:  routine does not consider node state or eligibility 
 * populates taskmap and J->Req[x]->NodeList attributes 
 * 
 * @param J (I) A mjob_t structure that holds the job whose task map should be populated [J->Req[0]->NodeList modified]
 * @param R (I) The destination resource manager for the job
 * @param GrowTaskMap (I) (Boolean) Allows the task map to grow as needed
 * @param NodeList (O)
 * @param STaskMap (O)
 * @param STaskMapSize (I, Optional) 
 */

int MJobDistributeTasks(

  mjob_t     *J,            /* I (for LL RM's, RQ->NodeLists are modified, if STaskMap not set, populate J->TaskMap) */
  mrm_t      *R,            /* I (destination RM) */
  mbool_t     GrowTaskMap,  /* I (allow dynamic growth of taskmap) */
  mnl_t      *NodeList,     /* O multi-req nodelist w/taskcount info */
  int        *STaskMap,     /* O task map (optional,terminated w/-1,minsize=J->TaskMapSize) */
  int         STaskMapSize) /* I (optional) */

  {
  int index;          /* index into generated aggregate multi-req nodelist */
  int sindex;

  int nindex;
  int pindex;
  int tindex;

  int rqindex;

  int taskcount;
  int tmpNodeCount;
 
  int ReqMem;    /* accumulated mem from nodes */

  enum MDistPolicyEnum Distribution;

  mreq_t *RQ;

  int TPN;
  int Overflow;

  mbool_t  AttemptBalancedDistribution;

  int MaxTPN;

  int rc;

  enum MTaskDistEnum DPolicy = mtdNONE;

  int *TaskMap;
  int  TaskMapSize;

  mnode_t *N;

  static mnl_t tmpNodeList;
  static mbool_t   Init = FALSE;

  /* note: handle pre-loaded arbitrary geometry */

  const char *FName = "MJobDistributeTasks";

  MDB(3,fSCHED) MLog("%s(%s,%s,%s,NodeList,STaskMap,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    MBool[GrowTaskMap],
    STaskMapSize);

  if (NodeList != NULL)
    {
    MNLClear(NodeList);
    }

  if ((J == NULL) || (NodeList == NULL) || (R == NULL))
    {
    MDB(3,fSCHED) MLog("ALERT:    invalid parameters.\n");

    return(FAILURE);
    }

  if (STaskMap == NULL)
    {
    if (J->TaskMap == NULL)
      {
      MDB(3,fSCHED) MLog("ALERT:    invalid job taskmap in %s\n",
        FName);

      return(FAILURE);
      }

    TaskMap     = J->TaskMap;
    TaskMapSize = J->TaskMapSize;
    }
  else
    {
    TaskMap     = STaskMap;
    TaskMapSize = STaskMapSize;
    }

  if (Init == FALSE)
    {
    MNLInit(&tmpNodeList);

    Init = TRUE;
    }

  if (J->DistPolicy != mtdNONE)
    DPolicy = J->DistPolicy;
  else if ((J->Credential.C != NULL) && (J->Credential.C->DistPolicy != mtdNONE))
    DPolicy = J->Credential.C->DistPolicy;
  else
    DPolicy = MPar[0].DistPolicy;

#ifdef __DAVID
  MDB(8,fSCHED) MLog("INFO:     Distribution Policy %s\n",
    MTaskDistributionPolicy[DPolicy]);
#endif

  /* NOTE:  RM specific distribution policies handled in Alloc */

  Distribution = mdpRR;

  switch (DPolicy)
    {
    case mtdPack:

      Distribution = mdpPack;

      break;

    case mtdArbGeo:

      Distribution = mdpArbGeo;

      break;

    case mtdDisperse:

      Distribution = mdpDisperse;

      break;

    case mtdDefault:
    case mtdRR:
    case mtdLocal:

    default:

      Distribution = mdpRR;

      break;
    }  /* END switch (DPolicy) */

  /* build initial tasklist */

  MNLClear(&tmpNodeList);

  tmpNodeCount = 0;

  index        = 0;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    sindex = index;

    RQ = J->Req[rqindex];

    if (RQ->RMIndex != R->Index)
      continue;

    if ((R->Type == mrmtLL) && (Distribution != mdpArbGeo))
      {
#if 0
      mnode_t *MaxTPNN;

      int RRTPN;
      int tpnindex;
      int TasksAvail;
      int TasksRequired;

      if (RQ->NodeCount > 0)
        {
        /* NOTE: LL 1.x, 2.1 require single step, monotonically decreasing algorithm */
        /*       ie, 4,4,3,3 not 4,4,4,2 (pseudo round robin)                        */

        MaxTPN = 1;

        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex) == SUCCESS;nindex++)
          {
          MaxTPN = MAX(MaxTPN,MNLGetTCAtIndex(&RQ->NodeList,nindex));
          }

        RRTPN = RQ->TaskCount / RQ->NodeCount;

        Overflow = RQ->TaskCount % RQ->NodeCount;

        /* assign nodes with RRTPN + 1 tasks */

        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          if (Overflow == 0)
            break;

          if (MNLGetTCAtIndex(&RQ->NodeList,nindex) > RRTPN)
            {
            MNLSetNodeAtIndex(&tmpNodeList,index,N);
            MNLSetTCAtIndex(&tmpNodeList,index,RRTPN + 1);

            MNLSetTCAtIndex(&RQ->NodeList,nindex,0);

            MDB(6,fSCHED) MLog("INFO:     %d tasks assigned to overflow node[%d]\n",
              RRTPN + 1,
              index);

            index++;

            Overflow--;
            }
          }  /* END for (nindex) */

        /* assign remaining nodes */

        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          if (index == RQ->NodeCount)
            {
            MDB(6,fSCHED) MLog("INFO:     nodecount %d reached\n",
              RQ->NodeCount);

            break;
            }

          if (MNLGetTCAtIndex(&RQ->NodeList,nindex) >= RRTPN)
            {
            tmpNodeList[index].N = N;

            tmpNodeList[index].TC = RRTPN;

            MNLSetTCAtIndex(&RQ->NodeList,nindex,0);

            MDB(6,fSCHED) MLog("INFO:     %d tasks assigned to node[%d] %s\n",
              RRTPN,
              index,
              tmpNodeList[index].N->Name);

            index++;
            }
          else
            {
            MDB(6,fSCHED) MLog("INFO:     node %d ignored (%d < %d)\n",
              index,
              MNLGetTCAtIndex(&RQ->NodeList,nindex),
              RRTPN);
            }
          }    /* END for (nindex) */

        
        tmpNodeList[index].N = NULL;

        MNLCopy(&RQ->NodeList,tmpNodeList[sindex]);

        RQ->NodeList[index - sindex].N = NULL;

        MNLCopy(NodeList,tmpNodeList);

        tmpNodeCount = index;
        }  /* END if (RQ->NodeCount > 0) */
      else
        {
        /* RQ->NodeCount not specified */

        /* NOTE: use single step, monotonically decreasing algorithm for now */
        /*       ie, 4,4,3,3 not 4,4,4,2                                     */

        TasksAvail = 0;
        TasksRequired = J->Request.TC;

        AllocIndex = 0;

        while (TasksAvail < TasksRequired)
          {
          /* select MaxTPN node */

          MaxTPN      = -1;
          MaxTPNN     = NULL;
          tpnindex    = 0;

          for (nindex = sindex;RQ->NodeList[nindex].N != NULL;nindex++)
            {
            if ((int)RQ->NodeList[nindex].TC > MaxTPN)
              {
              MaxTPN   = RQ->NodeList[nindex].TC;
              MaxTPNN  = RQ->NodeList[nindex].N;
              tpnindex = nindex;
              }
            }

          if (MaxTPN <= 0)
            break;

          tmpNodeList[AllocIndex].N  = MaxTPNN;
          tmpNodeList[AllocIndex].TC = MaxTPN;

          RQ->NodeList[tpnindex].TC = 0;

          AllocIndex++;

          TasksAvail = 0;

          /* constrain TC delta to X (currently 1) */

          for (nindex = 0;nindex < AllocIndex;nindex++)
            {
            if (RQ->BlockingFactor != 1)
              {
              /* blocking factor specification removes single step constraint */

              tmpNodeList[nindex].TC = MIN(tmpNodeList[nindex].TC,MaxTPN + 1);
              }

            TasksAvail += tmpNodeList[nindex].TC;
            }
          }    /* END while (TasksAvail < TasksRequired) */

        if (TasksAvail < TasksRequired)
          {
          MDB(2,fSCHED) MLog("ALERT:    inadequate tasks found on %d nodes in %s() (%d < %d)\n",
            AllocIndex,
            FName,
            TasksAvail,
            TasksRequired);

          return(FAILURE);
          }

        if (TasksAvail > TasksRequired)
          {
          /* remove excess tasks */

          if (RQ->BlockingFactor == 1)
            {
            tmpNodeList[AllocIndex - 1].TC -= (TasksAvail - TasksRequired);

            TasksAvail = TasksRequired;
            }
          else if (tmpNodeList[0].TC != tmpNodeList[AllocIndex - 1].TC)
            {
            /* locate n+1 -> n TC transition */

            for (nindex = AllocIndex - 1;nindex > 0;nindex--)
              {
              if (tmpNodeList[nindex].TC != tmpNodeList[nindex - 1].TC)
                {
                tmpNodeList[nindex - 1].TC--;

                TasksAvail--;

                if (TasksAvail == TasksRequired)
                  break;
                }
              }    /* END for (nindex) */
            }

          if (TasksAvail > TasksRequired)
            {
            /* reduce last task count */

            for (nindex = AllocIndex - 1;nindex >= 0;nindex--)
              {
              tmpNodeList[nindex].TC--;

              TasksAvail--;

              if (TasksAvail == TasksRequired)
                break;
              }
            }
          }    /* END if (TasksAvail > TasksRequired) */

        tmpNodeList[AllocIndex].N = NULL;

        MJobCopyNL(J,&RQ->NodeList,tmpNodeList);

        tmpNodeCount = AllocIndex;
        }    /* END else (RQ->NodeCount > 0) */
#endif
      }      /* END if (R->Type == mrmtLL) */
    else if (Distribution == mdpDisperse)
      {
      int tmpTC;
      int ReqTC;
      int TPN;

      ReqTC = RQ->TaskCount;

      for (TPN = 1;TPN < MDEF_TASKS_PER_NODE;TPN++)
        {
        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          tmpTC = MNLGetTCAtIndex(&RQ->NodeList,nindex);

          /* If the TPN counter exceeds the requested task count for this node
           * then try the next node in the list. */

          MDB(8,fSCHED) MLog("INFO:     Node %d task count %d (Disperse)\n",
            nindex,
            tmpTC);

          if (TPN > tmpTC)
            continue;

          /* This node has room for another task so put it in the tmpNodeList (overwrite it
           * if it is already there) and set/increment the task count. */

          MNLSetNodeAtIndex(&tmpNodeList,nindex,N);
          MNLSetTCAtIndex(&tmpNodeList,nindex,TPN);

          ReqTC--;

#ifdef __DAVID
          MDB(8,fSCHED) MLog("INFO:     Node %d has room for another task (Disperse), remaining tasks %d\n",
                   nindex,
                   ReqTC);
#endif

          if (ReqTC <= 0)
            {
            nindex++;
            break;
            }
          } /* END for (RQ->NodeList[nindex].N) */
 
        if (TPN == 1)
          tmpNodeCount = nindex; /* We have made one pass of the node list giving us the total available node count */

        if (ReqTC <= 0)
          break;
        }    /* END for (TPN) */

#ifdef __DAVID
      MDB(8,fSCHED) MLog("INFO:     (Disperse) total node count %d\n",
               tmpNodeCount);
#endif

      MNLTerminateAtIndex(&tmpNodeList,tmpNodeCount);

      MNLCopy(&tmpNodeList,&RQ->NodeList);
      }
    else
      {
      int tmpOverflow;
      int tmpTaskCount;

      int tmpTC;

      /* handle allocation for all non-LL resource managers */

      MDB(8,fSCHED) MLog("INFO:     RQ->NodeCount %d\n",
        RQ->NodeCount);

      AttemptBalancedDistribution = FALSE;

      MaxTPN = 0;
      tmpTaskCount = 0;
      tmpNodeCount = 0;
      tmpOverflow  = 0;

      if (RQ->NodeCount > 0)
        {
        TPN      = RQ->TaskCount / RQ->NodeCount;
        Overflow = RQ->TaskCount % RQ->NodeCount;

        /* NOTE:  if nodecount is specified, attempt balanced task distribution */

        tmpTaskCount = RQ->TaskCount;
        tmpNodeCount = RQ->NodeCount;
        tmpOverflow  = Overflow;

        MDB(8,fSCHED) MLog("INFO:     tmpTaskCount %d, tmpNodeCount %d, tmpOverflow %d\n",
          tmpTaskCount, 
          tmpNodeCount, 
          tmpOverflow);

        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          tmpTC = MNLGetTCAtIndex(&RQ->NodeList,nindex);

          MDB(8,fSCHED) MLog("INFO:     nindex %d, RQ TaskCount %d, TPN %d\n",
            nindex,
            tmpTC,
            TPN);

          if (tmpTC >= TPN)
            {
            tmpNodeCount --;
            tmpTaskCount -= TPN;
            tmpOverflow  -= (tmpTC - TPN);
            }
          }    /* END for (nindex) */

        if ((tmpOverflow <= 0) &&
            (tmpNodeCount <= 0) &&
            (tmpTaskCount <= 0))
          {
          AttemptBalancedDistribution = TRUE;
          }
        }
      else
        {
        TPN      = 1;
        Overflow = 0;
        }

      taskcount = 0;
      ReqMem    = 0;

      MDB(8,fSCHED) MLog("INFO:     post processing tmpTaskCount %d, tmpNodeCount %d, tmpOverflow %d\n",
        tmpTaskCount, 
        tmpNodeCount, 
        tmpOverflow);

      MDB(8,fSCHED) MLog("INFO:     Attempt Balanced Distribution %d\n",
        AttemptBalancedDistribution);

      /* NOTE:  if nodecount is specified, attempt balanced task distribution */

      /* obtain nodelist from job RQ nodelist */

      for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        tmpTC = MNLGetTCAtIndex(&RQ->NodeList,nindex);

        MNLSetNodeAtIndex(&tmpNodeList,index,N);

        if (RQ->NodeCount > 0)
          {
          if (AttemptBalancedDistribution == TRUE)
            {
            MaxTPN = TPN;

            if ((tmpTC > TPN) && (Overflow > 0))
              {
              tmpTaskCount = MIN(tmpTC - TPN,Overflow);

              MaxTPN += tmpTaskCount;

              Overflow  -= tmpTaskCount;
              }
            }
          else
            {
            /* MaxTPN = 1 + RemainingTasks - RemainingNodes */

            MaxTPN = 1 + (RQ->TaskCount - taskcount) - (RQ->NodeCount - index);
            }

          MNLSetTCAtIndex(&RQ->NodeList,nindex,MIN(MaxTPN,tmpTC));
          }

        MDB(8,fSCHED) MLog("INFO:     MaxTPN %d node index (%d) TC %d\n",
          MaxTPN,
          nindex,
          tmpTC);

        if ((RQ->TasksPerNode > 0) && !bmisset(&J->IFlags,mjifMaxTPN))
          {
          if (tmpTC >= RQ->TasksPerNode)
            {
            MNLAddTCAtIndex(&RQ->NodeList,nindex,-1 * tmpTC % RQ->TasksPerNode);

            MDB(8,fSCHED) MLog("INFO:     node index (%d) decremented TC %d\n",
              nindex,
              MNLGetTCAtIndex(&RQ->NodeList,nindex));
            }
          else
            {
            /* 
             * why zero out the task count? this is most likely the tail node, so
             * it can have the remainder task count when the task count does not
             * fill up the last node.
             *
             * RQ->NodeList[nindex].TC = 0;

            MDB(8,fSCHED) MLog("INFO:     node index (%d) zeroed TC %d\n",
              nindex,
              tmpTC);
             */
            }
          }

        tmpTC = MNLGetTCAtIndex(&RQ->NodeList,nindex); /* refresh as it may have changed */

        MDB(8,fSCHED) MLog("INFO:     RQ->TaskCount %d, taskcount %d, nindex %d, nindex TC %d\n",
          RQ->TaskCount, 
          taskcount, 
          nindex,
          tmpTC);

        /* When scheduling nodeboards we don't want to decrement task count
           because we want to fulfill job wide mem request as well */

        if (!bmisset(&MPar[N->PtIndex].Flags,mpfSharedMem))
          {
          MNLSetTCAtIndex(&RQ->NodeList,nindex,MIN(RQ->TaskCount - taskcount,MNLGetTCAtIndex(&RQ->NodeList,nindex)));
          }

        tmpTC = MNLGetTCAtIndex(&RQ->NodeList,nindex); /* refresh as it may have changed */

        MDB(8,fSCHED) MLog("INFO:     Adjusted TC %d\n",
          tmpTC);

        if (tmpTC > 0)
          {
          mnode_t *tmpN;

          if (bmisset(&MPar[N->PtIndex].Flags,mpfSharedMem))
            {
            /* check if nodes have fulfilled jobwide requirements */
   
            if ((taskcount >= RQ->TaskCount) && (ReqMem >= RQ->ReqMem))
              {
              /* reqmem fulfilled */

              MDB(7,fSCHED) MLog("INFO:     job distributed across job wide reqs (%d/%d tasks) (%d/%d mem)\n",
                  taskcount,
                  RQ->TaskCount,
                  ReqMem,
                  RQ->ReqMem);
          
              MNLTerminateAtIndex(&tmpNodeList,index);
              
              break;
              }
            } /* END if (MSched.SharedMem == TRUE) */

          MNLSetTCAtIndex(&tmpNodeList,index,tmpTC);

          taskcount += tmpTC;

          MNLGetNodeAtIndex(&tmpNodeList,index,&tmpN);

          ReqMem    += N->CRes.Mem;

          MDB(8,fSCHED) MLog("INFO:     taskcount %d\n",
            taskcount);

          index++;
          } /* END if (RQ->NodeList[nindex].TC > 0) */
        else
          {
          MNLTerminateAtIndex(&tmpNodeList,index);

          MDB(8,fSCHED) MLog("INFO:     Done looking for nodes (stopped at index %d)\n",
            index);

          break;
          }
        }    /* END for (nindex) */

      MDB(4,fSCHED) MLog("INFO:     %d node(s)/%d task(s) added to %s:%d\n",
        index - sindex,
        taskcount,
        J->Name,
        rqindex);

      if (((index - sindex) <= 0) && (!bmisset(&J->Flags,mjfNoResources)))
        {
        MDB(2,fSCHED) MLog("ALERT:    inadequate tasks allocated to job %s:%d\n",
          J->Name,
          rqindex);

        return(FAILURE);
        }

      tmpNodeCount = index;
      }  /* END else (R->Type == rmLL) */
    }    /* END for (rqindex) */

  MNLTerminateAtIndex(&tmpNodeList,tmpNodeCount);

  if (NodeList != NULL)
    {
    MNLCopy(NodeList,&tmpNodeList);
    }

  if (DPolicy == mtdLocal)
    {
    /* NOTE:  does not support 'grow' option */

    rc = MLocalJobDistributeTasks(J,R,&tmpNodeList,TaskMap,TaskMapSize);

    return(rc);
    }

  /* per node taskcounts have been assigned w/in tmpNodeList, 
     now distribute tasks to node by populating TaskMap */

  switch (Distribution)
    {
    case mdpArbGeo:

      {
      int  rc;

      char EMsg[MMAX_LINE];

      if (J->Request.TC > J->TaskMapSize) 
        {
        if ((GrowTaskMap == TRUE) && (TaskMap == J->TaskMap))
          {
          if (MJobGrowTaskMap(J,0) == SUCCESS)
            {
            TaskMap     = J->TaskMap;
            TaskMapSize = J->TaskMapSize;
            }
          }
     
        if (J->Request.TC > J->TaskMapSize)
          {
          MDB(3,fSCHED) MLog("ALERT:    taskmap too small for specified geometry in %s\n",
            FName);

          return(FAILURE);
          }
        }

      if (strchr(J->Geometry,'(') != NULL)
        {
        rc = MJobDistributeLLTaskGeometry(
              J,
              &tmpNodeList,  /* I */
              TaskMap,      /* O */
              TaskMapSize,  /* I */
              EMsg);
        }
      else
        {
        rc = MJobDistributeTaskGeometry(
               J,
               J->Req[0],    /* I */
               &tmpNodeList,  /* I */
               TaskMap,      /* O */
               TaskMapSize,  /* I */
               EMsg);
        }

      if (rc == FAILURE)
        {
        MDB(3,fSCHED) MLog("ALERT:    cannot distribute task geometry in %s %s\n",
          FName,
          (EMsg != NULL) ? EMsg : "");

        MJobSetHold(J,mhBatch,0,mhrPolicyViolation,EMsg);

        return(FAILURE);
        }
      }    /* END BLOCK (case mdpArbGeo) */

      /* TaskMap already populated in MJobDistributeTaskGeometry() */

      break;

    case mdpRR:

      /* NYI */

      /* TEMP:  allow pass through */

      /* break; */

    case mdpPack:
    case mdpDisperse:

      tindex = 0;

      for (nindex = 0;MNLGetNodeAtIndex(&tmpNodeList,nindex,&N) == SUCCESS;nindex++)
        {
        MDB(4,fSCHED)
          {
          MLog("INFO:     MNode[%03d] '%s'(x%d) added to job '%s'\n",
            nindex,
            N->Name,
            MNLGetTCAtIndex(&tmpNodeList,nindex),
            J->Name);

          MNodeShow(N);
          }

        MDB(6,fSCHED) MLog("INFO:     adding %d task(s) from node %s\n",
          MNLGetTCAtIndex(&tmpNodeList,nindex),
          N->Name);

        for (pindex = 0;pindex < MNLGetTCAtIndex(NodeList,nindex);pindex++)
          {
          TaskMap[tindex] = N->Index;

          MDB(8,fSCHED) MLog("INFO:     task[%03d] '%s' added\n",
            tindex,
            N->Name);

          tindex++;

          if (tindex > TaskMapSize)
            {
            if ((GrowTaskMap == TRUE) && (TaskMap == J->TaskMap))
              {
              if (MJobGrowTaskMap(J,0) == SUCCESS)
                {
                TaskMap     = J->TaskMap;
                TaskMapSize = J->TaskMapSize;
                }
              }
      
            if (tindex > TaskMapSize)
              {
              MDB(1,fSCHED) MLog("ERROR:    tasklist for job '%s' too large (size=%d,grow=%s)\n",
                J->Name,
                TaskMapSize,
                MBool[GrowTaskMap]);

              TaskMap[TaskMapSize] = -1;
              }

            return(FAILURE);
            }  /* END if (tindex > TaskMapSize) */

          if (tindex >= J->Request.TC)
            {
            MDB(7,fSCHED) MLog("INFO:     all tasks located\n");

            break;
            }
          }    /* END for (pindex) */
        }      /* END for (nindex) */

      TaskMap[tindex] = -1;

      MDB(4,fSCHED) MLog("INFO:     tasks distributed: %d (Round-Robin)\n",
        tindex);

      if ((tindex <= 0) && (!bmisset(&J->Flags,mjfNoResources)))
        {
        MDB(1,fSCHED) MLog("ERROR:    no tasks allocated to job '%s'\n",
          J->Name);

        return(FAILURE);
        }

      break;

    default:

      TaskMap[0] = -1;

      MDB(3,fSCHED) MLog("ERROR:    unexpected task distribution (%d) in %s()\n",
        Distribution,
        FName);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (Distribution) */

  if ((J->Req[1] != NULL) && (NodeList != NULL) && (Distribution != mdpArbGeo))
    {
    mnode_t *N1;
    mnode_t *N2;
    mnode_t *N3;

    int TC;

    int nindex1;
    int nindex2;
    int nindex3;

    /* coalesce multi-req NodeList */

    nindex3 = 0;

    for (nindex1 = 0;MNLGetNodeAtIndex(NodeList,nindex1,&N1) == SUCCESS;nindex1++)
      {
      TC = MNLGetTCAtIndex(NodeList,nindex1);

      if (TC == 0)
        continue;

      if (nindex3 != nindex1)
        {
        MNLSetNodeAtIndex(NodeList,nindex3,N1);
        MNLSetTCAtIndex(NodeList,nindex3,TC);
        }

      MNLGetNodeAtIndex(NodeList,nindex3,&N3);

      for (nindex2 = nindex1 + 1;MNLGetNodeAtIndex(NodeList,nindex2,&N2) == SUCCESS;nindex2++)
        {
        if (N2 != N3)
          continue;

        MNLAddTCAtIndex(NodeList,nindex3,MNLGetTCAtIndex(NodeList,nindex2));

        MNLSetTCAtIndex(NodeList,nindex2,0);
        }  /* END for (nindex2) */

      nindex3++;
      }  /* END for (nindex1) */

    MNLTerminateAtIndex(NodeList,nindex3);
    MNLSetTCAtIndex(NodeList,nindex3,0);
    }    /* END if ((J->Req[1] != NULL) || ...) */

  return(SUCCESS);
  }  /* END MJobDistributeTasks() */







/**
 *
 *
 * @param J (I)
 * @param Path (I/O) [modified]
 * @param PathSize (I)
 */

int MJobInsertHome(

  mjob_t *J,         /* I */
  char   *Path,      /* I/O (modified) */
  int     PathSize)  /* I */

  {
  char *BPtr;
  int   BSpace;
  char  tmpLine[MMAX_LINE];

  if ((J == NULL) ||
      (Path == NULL))
    {
    return(FAILURE);
    }
  
  if ((J->Credential.U->HomeDir == NULL) ||
      (J->Credential.U->HomeDir[0] == '\0'))
    {
    return(FAILURE);
    }

  if (strncmp(Path,J->Credential.U->HomeDir,strlen(J->Credential.U->HomeDir)))
    {
    /* home not found in path */

    return(SUCCESS);
    }

  MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

  MUSNPrintF(&BPtr,&BSpace,"$HOME%s",
    &Path[strlen(J->Credential.U->HomeDir)]);

  MUStrCpy(Path,tmpLine,PathSize);

  return(SUCCESS);
  }  /* END MJobInsertHome() */






/**
 * Resolves $HOME into the actual home directory.
 *
 * @param J (I)
 * @param IPath (I)
 * @param OPath (O)
 * @param OPathSize (I)
 */

int MJobResolveHome(

  mjob_t *J,         /* I */
  char   *IPath,     /* I */
  char   *OPath,     /* O */
  int     OPathSize) /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  const char *FName = "MJobResolveHome";

  MDB(6,fSCHED) MLog("%s(%s,%s,OPath,OPathSize)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (IPath != NULL) ? IPath : "NULL");

  if (OPath == NULL)
    {
    return(FAILURE);
    }

  OPath[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (IPath != NULL)
    {
    MUSNInit(&BPtr,&BSpace,OPath,OPathSize);

    if (!strncmp(IPath,"$HOME",strlen("$HOME")) && (J->Credential.U != NULL))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        J->Credential.U->HomeDir,
        &IPath[strlen("$HOME")]);

      return(SUCCESS);
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        IPath);

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MJobResolveHome() */




/**
 *
 *
 * @param J (I)
 * @param JList (O) [optional]
 * @param DoFragment (I)
 */

int MJobFragment(

  mjob_t  *J,          /* I */
  mjob_t **JList,      /* O (optional) */
  mbool_t  DoFragment) /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  char HostList[MMAX_LINE << 2];

  char SJobID[MMAX_NAME];

  mrm_t *R;

  mjob_t *tmpJ = NULL;

  int index;

  mreq_t *SReq;
  mreq_t *DReq;

  mnl_t tmpNL;

  mnode_t *N;
  mnode_t *FirstFNode;

  /* job must have a required hostlist */

  if ((J == NULL) || (MNLIsEmpty(&J->ReqHList)) || (J->Req[0] == NULL))
    {
    return(FAILURE);
    }

  SReq = J->Req[0];
  
  MRMFind("internal",&R);

  MUSNInit(&BPtr,&BSpace,HostList,sizeof(HostList));

  /* Copy J->ReqHList to tmpNL while we rebuild the job ReqHList that meets policy criterias. */

  memset(&tmpNL,0,sizeof(tmpNL));
  MNLCopy(&tmpNL,&J->ReqHList);
  MNLClear(&J->ReqHList);

  index = 0;

  while ((MNLGetNodeAtIndex(&tmpNL,index,&N) == SUCCESS) && 
         ((bmisset(&J->SpecPAL,N->PtIndex) == FALSE) ||
          (MReqCheckResourceMatch(
            J,
            J->Req[0],
            N,
            NULL,
            MMAX_TIME,
            FALSE,
            NULL,
            NULL,
            NULL) == FAILURE)))
    {
    index++;
    }

  if  (N == NULL)
    {
    /* no feasible nodes found */

    MNLFree(&tmpNL);
    return(FAILURE);
    }

  FirstFNode = N;

  index++;

  /* setup additional jobs for remaining feasible nodes */

  for (;MNLGetNodeAtIndex(&tmpNL,index,&N) == SUCCESS;index++)
    {
    if (bmisset(&J->SpecPAL,N->PtIndex) == FALSE)
      {
      continue;
      }

    if (MReqCheckResourceMatch(
          J,
          J->Req[0],
          N,
          NULL,
          MMAX_TIME,
          FALSE,
          NULL,
          NULL,
          NULL) == FAILURE)
      {
      /* only fragment job into the number of feasible nodes */

      continue;
      }

    /* validate job */

    if (MSched.Name[0] != '\0')
      {
      snprintf(SJobID,sizeof(SJobID),"%s.%d",
        MSched.Name,
        MRMJobCounterIncrement(R));
      }
    else
      {
      snprintf(SJobID,sizeof(SJobID),"Moab.%d",
        MRMJobCounterIncrement(R));
      }

    if (MJobCreate(SJobID,TRUE,&tmpJ,NULL) == FAILURE)
      {
      MDB(1,fS3) MLog("ERROR:    job buffer is full  (ignoring job '%s')\n",
        SJobID);

      return(FAILURE);
      }

    MRMJobPreLoad(tmpJ,SJobID,J->SubmitRM);

    MUStrDup(&tmpJ->SRMJID,tmpJ->Name);

    if (tmpJ->Req[0] == NULL)
      {
      MReqCreate(tmpJ,SReq,&DReq,TRUE);
      }
    else
      {
      DReq = tmpJ->Req[0];
      }

    tmpJ->RMXString = NULL;

    if (J->RMXString != NULL) 
      MUStrDup(&tmpJ->RMXString,J->RMXString);

    MS3AddLocalJob(J->SubmitRM,tmpJ->Name); 

    bmcopy(&tmpJ->Flags,&J->Flags);
    bmcopy(&tmpJ->SpecFlags,&J->SpecFlags);

    bmcopy(&tmpJ->IFlags,&J->IFlags);

    MJobSetState(tmpJ,mjsIdle);

    MJobBuildCL(tmpJ); 

    tmpJ->SpecWCLimit[0] = J->SpecWCLimit[0];
    tmpJ->SpecWCLimit[1] = J->SpecWCLimit[1];
    tmpJ->SpecWCLimit[2] = J->SpecWCLimit[2];
    tmpJ->WCLimit        = J->WCLimit;

    tmpJ->Credential.U = J->Credential.U;
    tmpJ->Credential.G = J->Credential.G;
    tmpJ->Credential.C = J->Credential.C;
    tmpJ->Credential.Q = J->Credential.Q;
    tmpJ->Credential.A = J->Credential.A;

    bmcopy(&tmpJ->SpecPAL,&J->SpecPAL);
    bmcopy(&tmpJ->SysPAL,&J->SysPAL);
    bmcopy(&tmpJ->PAL,&J->PAL);
    bmcopy(&tmpJ->CurrentPAL,&J->CurrentPAL);

    MUStrCpy(tmpJ->Name,SJobID,sizeof(tmpJ->Name));

    MNLSetNodeAtIndex(&tmpJ->ReqHList,0,N);
    MNLSetTCAtIndex(&tmpJ->ReqHList,0,N->CRes.Procs);
    MNLTerminateAtIndex(&tmpJ->ReqHList,1);

    tmpJ->Request.TC = N->CRes.Procs;
    tmpJ->Request.NC = 1;

    DReq->TaskRequestList[0] = N->CRes.Procs;
    DReq->TaskRequestList[1] = N->CRes.Procs;
    DReq->NodeCount = 1;
    DReq->TaskCount = N->CRes.Procs;
    DReq->TasksPerNode = N->CRes.Procs;

    bmclear(&DReq->ReqFBM);
    bmor(&DReq->ReqFBM,&SReq->ReqFBM);

    tmpJ->CurrentStartPriority = J->CurrentStartPriority;
    memcpy(&tmpJ->PStartPriority,&J->PStartPriority,sizeof(J->PStartPriority));

    MNLTerminateAtIndex(&tmpNL,index);

    /* copy the triggers from J to tmpJ */

    MJobCopyTrigs(J,tmpJ,TRUE,TRUE,TRUE);
    }  /* END for (index) */     

  MNLSetNodeAtIndex(&J->ReqHList,0,FirstFNode);
  MNLSetTCAtIndex(&J->ReqHList,0,FirstFNode->CRes.Procs);
  MNLTerminateAtIndex(&J->ReqHList,1);

  J->Request.TC = FirstFNode->CRes.Procs;
  J->Request.NC = 1;

  J->Req[0]->TaskRequestList[0] = FirstFNode->CRes.Procs;
  J->Req[0]->TaskRequestList[1] = FirstFNode->CRes.Procs;
  J->Req[0]->NodeCount = 1;
  J->Req[0]->TaskCount = FirstFNode->CRes.Procs;
  J->Req[0]->TasksPerNode = FirstFNode->CRes.Procs;

  MNLFree(&tmpNL);

  return(SUCCESS);
  }  /* END MJobFragment() */



/**
 * Returns True if the job can access a reservation that has a job with mjfIgnIdleJobRsv flag.
 *
 * @param J (I)
 * @param R (I)
 */

mbool_t MJobCanAccessIgnIdleJobRsv(

  const mjob_t *J,
  const mrsv_t *R)

  {
  mjob_t *RJ = NULL;

  MASSERT(J != NULL,"null job when checking job access to idle job rsv");
  MASSERT(R != NULL,"null rsv when checking job access to idle job rsv");
  MASSERT(R->J != NULL,"null rsv job when checking job access to idle job rsv");

  RJ = R->J;

  if ((MJOBISACTIVE(RJ) == TRUE) &&
      (!bmisset(&J->Flags,mjfIgnIdleJobRsv)) &&
      (bmisset(&RJ->Flags,mjfIgnIdleJobRsv)) &&
      ((MSched.Time - R->StartTime) > MJobGetMinPreemptTime(RJ)))
    return(TRUE);

  return(FALSE);
  } /* END MJobCanAccessIgnIdleJobRsv() */




/**
 * MJobStartPrioComp
 *
 * is a qsort "comparator" function used to perform priority sorting of jobs.
 *
 * Perform Job Priority (and subjob index) comparison on two elements of
 * an job index integer array. The Arguments are pointers to indicies of
 * jobs to be scheduled. 
 *
 * Keys of the jobs that are to be compared are:
 *
 * 1) priority order of the two jobs
 *
 * 2) For jobs that are subjobs of the same master we need to:
 *    compare the subjob's MasterJ->Array->Member index.
 *    Lower indexes are to be scheduled prior to higher indexes
 *
 * NOTE: order high to low (for int JList[] - array of job indices)
 *
 * @param A (I)    Pointer to the jindex of job A
 * @param B (I)    Pointer to the jindex of job B
 */

int MJobStartPrioComp(
 
  mjob_t **A,  /* I */
  mjob_t **B)  /* I */
 
  {
  int tmp;

  /* Compare the 1st key:  Starting Priority */
  if ((*A)->CurrentStartPriority > (*B)->CurrentStartPriority)
    {
    tmp = -1; /* First argument gets lower sort index */
    }
  else if ((*A)->CurrentStartPriority == (*B)->CurrentStartPriority)
    {
    /* Assume an exact match first when priorities match */
    tmp = 0;

    /* Comprare the 2nd key: Subjob Array Member Index 
     *
     * Validate that each entry is in fact a Subjob, the Group pointers are != NULL
     * and the Group Indexes are the same, then compare the SubjobMemberIndex of the two
     */
    if (bmisset(&(*A)->IFlags,mjifIsArrayJob) && bmisset(&(*B)->IFlags,mjifIsArrayJob))
      {
      /* Ensure we don't have NULL JGroup pointers */
      if (((*A)->JGroup != NULL) && ((*B)->JGroup != NULL))
        {
        /* The two items are in a group: are the groups the same master job? */
        if ((*A)->JGroup->RealJob == (*B)->JGroup->RealJob)
          {
          /* Same group, now compare the MemberIndex of the subjobs */
          if ((*A)->JGroup->ArrayIndex < (*B)->JGroup->ArrayIndex)
            {
            /* First argument gets lower sort index than second arg */
            tmp = -1;
            }
          else if ((*A)->JGroup->ArrayIndex > (*B)->JGroup->ArrayIndex)
            {
            /* First argument gets higher sort index than second arg */
            tmp = 1;
            }
          else
            {
            /* JGroup->ArrayIndex of the two job arguments should NEVER be the same
            * If they are, this is a broken invariant. As a default then, 
            * just assume they are equal and return the current value of 'tmp'
            */
            }
          } /* END if ((*A)->JGroup->RealJob == (*B)->JGroup->RealJob) */
        else
          {
          tmp = strverscmp((*A)->Name,(*B)->Name);
          }
        } /* END  GA and GB are != NULL */
      }  /* END if mjifIsArrayJob on A and B */
    else
      {
      tmp = strverscmp((*A)->Name,(*B)->Name);
      }
    }
  else
    {
    tmp = 1; /* First argument gets higher sort index */
    }
 
  return(tmp);
  }  /* END MJobStartPrioComp() */





/**
 * Removes the list of environment variables from the job's BaseEnv.
 *
 * @see MUStrRemoveFromList - child
 *
 * @param J (I) [modified]
 * @param VariableList (I) Comma delimited.
 */

int MJobRemoveEnvVarList(

  mjob_t *J,            /* I (modified) */
  char   *VariableList) /* I */

  {
  char *tmpVarList = NULL;
  char *envPtr     = NULL;
  char *TokPtr     = NULL;


  if ((J == NULL) || (J->Env.BaseEnv == NULL) || (VariableList == NULL))
    return(FAILURE);

  MUStrDup(&tmpVarList,VariableList);

  envPtr = MUStrTok(tmpVarList,",",&TokPtr);

  while (envPtr != NULL)
    {
    MUStrRemoveFromList(J->Env.BaseEnv,envPtr,ENVRS_ENCODED_CHAR,TRUE);

    envPtr = MUStrTok(NULL,",",&TokPtr);
    }

  MUFree(&tmpVarList);

  return(SUCCESS);
  } /* END int MJobRemoveEnvVarList() */



#define MMAX_LLTASKPERNODE 512

/**
 * Distribute available tasks according to specific LL geometry constraints.
 *
 *
 * From LL api: task_geometry
 *
 * There is a 1:1 correspondence between entries in the nodelist and task
 *      ids specified in the task geometry statement. Entries in the node list 
 *      that correspond to task IDs in the same set must specify the same 
 *      machine. Entries in the node list that correspond to task IDs in 
 *      different sets must specify different machines.
 *
 * @param J (I)
 * @param NL (O) [minsize=MMAX_NODE+1]
 * @param TaskMap (O) [minsize=TaskMapSize]
 * @param TaskMapSize (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobDistributeLLTaskGeometry(
  
  mjob_t    *J,           /* I */
  mnl_t     *NL,          /* I (minsize=MMAX_NODE+1) modified */
  int       *TaskMap,     /* O (minsize=TaskMapSize) */
  int        TaskMapSize, /* I */
  char      *EMsg)        /* O (optional,minsize=MMAX_LINE) */
 
  {
  int   PerNodeTaskMap[MMAX_LLTASKPERNODE] = {0};
  char *ptr;
  char *TokPtr = NULL;

  char *ReqPtr;
  char *ReqTokPtr;

  char *String = NULL;

  int   TCount;
  int   nindex;
  int   tindex;

  mnode_t *N;

  MUStrDup(&String,J->Geometry);

  /* parse off {} */

  ptr = MUStrTok(String,"{}",&TokPtr);

  if (ptr == NULL)
    {
    MUFree(&String);

    return(FAILURE);
    }

  ptr = MUStrTok(ptr,"() ",&TokPtr);

  while (ptr != NULL)
    {
    ReqPtr = MUStrTok(ptr,", ",&ReqTokPtr);

    TCount = 0;

    while (ReqPtr != NULL)
      {
      PerNodeTaskMap[TCount] = (int)strtol(ReqPtr,NULL,10);

      TCount++;

      PerNodeTaskMap[TCount] = -1;

      ReqPtr = MUStrTok(NULL,", ",&ReqTokPtr);
      }  /* END while (ReqPtr != NULL) */

    /* find Node in NL where TC = TCount */

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      MNLGetNodeAtIndex(NL,nindex,&N);

      if (N == NULL)
        {
        MUFree(&String);

        return(FAILURE);
        }

      if (MNLGetTCAtIndex(NL,nindex) != TCount)
        continue;

      /* found node, use and remove */
     
      for (tindex = 0;PerNodeTaskMap[tindex] != -1;tindex++)
        {
        TaskMap[PerNodeTaskMap[tindex]] = N->Index;
        }

      MNLRemove(NL,N);

      break; 
      }

    ptr = MUStrTok(NULL,"() ",&TokPtr);
    }    /* END while (ptr != NULL) */

  MUFree(&String);

  return(SUCCESS);
  }  /* END MJobDistributeLLTaskGeometry() */






/**
 * Distribute available tasks according to specific geometry constraints.
 *
 * @param J (I)
 * @param RQ (I)
 * @param NL (O) [minsize=MMAX_NODE+1]
 * @param TaskMap (O) [minsize=TaskMapSize]
 * @param TaskMapSize (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobDistributeTaskGeometry(
  
  mjob_t    *J,           /* I */
  mreq_t    *RQ,          /* I */
  mnl_t     *NL,          /* O (minsize=MMAX_NODE+1) */
  int       *TaskMap,     /* O (minsize=TaskMapSize) */
  int        TaskMapSize, /* I */
  char      *EMsg)        /* O (optional,minsize=MMAX_LINE) */
 
  {
  mbool_t *IsSet;

  int tindex;
  int nindex;

  char tmpBuffer[MMAX_BUFFER << 2];

  char *ptr;
  char *NTokPtr;
  char *TTokPtr;

  const char *FName = "MJobDistributeTaskGeometry";

  MDB(4,fSCHED) MLog("%s(%s,RQ,NL,TaskMap,%d,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    TaskMapSize);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) || (RQ == NULL))
    {
    return(FAILURE);
    }

  if ((J->Geometry == NULL) || (J->Geometry[0] == '\0'))
    {
    return(FAILURE);
    }

  MUStrCpy(tmpBuffer,J->Geometry,sizeof(tmpBuffer));

  IsSet = (mbool_t *)MUMalloc(sizeof(mbool_t) * MSched.JobMaxTaskCount);

  memset(IsSet,FALSE,sizeof(mbool_t) * MSched.JobMaxTaskCount);

  /* FORMAT:  { X[,Y]...}[,{X,[,Y]...}]... */

  ptr = MUStrTok(tmpBuffer,"{}",&NTokPtr);

  nindex = 0;

  while (ptr != NULL)
    {
    ptr = MUStrTok(ptr," \t\n,",&TTokPtr);

    while (ptr != NULL)
      {
      tindex = (int)strtol(ptr,NULL,10);

      if ((tindex < 0) || (tindex > MSched.JobMaxTaskCount))
        {
        if (EMsg != NULL)
          sprintf(EMsg,"task value %d is out of range",
            tindex);

        MUFree((char **)&IsSet);

        return(FAILURE);
        }

      if (tindex >= TaskMapSize)
        {
        if (EMsg != NULL)
          sprintf(EMsg,"task value %d exceeds job taskmap size",
            tindex);

        MUFree((char **)&IsSet);

        return(FAILURE);
        }

      if ((tindex == 0) && (ptr[0] != '0'))
        {
        if (EMsg != NULL)
          sprintf(EMsg,"task value '%.8s' is invalid",
            ptr);

        MUFree((char **)&IsSet);

        return(FAILURE);
        }

      if (IsSet[tindex] == TRUE)
        {
        if (EMsg != NULL)
          sprintf(EMsg,"task %d assigned to nodes %d and %d",
            tindex,
            nindex,
            TaskMap[tindex]);

        MUFree((char **)&IsSet);

        return(FAILURE);
        }

      TaskMap[tindex] = nindex;
      IsSet[tindex] = TRUE;

      ptr = MUStrTok(NULL," \t\n,",&TTokPtr);
      }  /* END while (ptr != NULL) */

    nindex++;

    ptr = MUStrTok(NULL,"{}",&NTokPtr);

    if ((ptr != NULL) && (ptr[0] == ','))
      ptr = MUStrTok(NULL,"{}",&NTokPtr);
    }  /* END while (ptr != NULL) */

  /* verify no holes exist */

  TaskMap[RQ->TaskCount] = -1;

  for (tindex = 0;tindex < RQ->TaskCount;tindex++)
    {
    if (IsSet[tindex] == FALSE)
      {
      if (EMsg != NULL)
        sprintf(EMsg,"task %d is not assigned",
          tindex);

      MUFree((char **)&IsSet);

      return(FAILURE);
      }
    }  /* END for (tindex) */

  if (IsSet[tindex] == TRUE)
    {
    if (EMsg != NULL)
      sprintf(EMsg,"more tasks specified in geometry than requested by job");
 
    MUFree((char **)&IsSet);

    return(FAILURE);
    }

  MUFree((char **)&IsSet);
 
  /* NYI */

  return(SUCCESS);
  }  /* END MJobDistributeTaskGeometry() */




  

/**
 * This routine will fresh the various counts based on the reqs' nodelist.
 *
 * NOTE: only updates Request.NC and RQ->NodeCount if they were > 0
 * 
 * @see MJobJoin() - parent
 *
 * @param J
 */

int MJobRefreshFromNodeLists(

  mjob_t *J)

  {
  int rqindex;

  int NC;
  int TC;

  mreq_t *RQ;

  NC = 0;
  TC = 0;

  MNLClear(&J->NodeList);

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    TC = MNLSumTC(&RQ->NodeList);
    NC = MNLCount(&RQ->NodeList);
    
    MNLMerge2(&J->NodeList,&RQ->NodeList,TRUE);

    RQ->TaskCount = TC;

    if (RQ->NodeCount > 0)
      RQ->NodeCount = NC;
    }

  J->Request.TC = MNLSumTC(&J->NodeList);
  J->Alloc.TC = J->Request.TC;

  J->Alloc.NC = MNLCount(&J->NodeList);

  if (J->Request.NC > 0)
    {
    J->Request.NC = J->Alloc.NC;
    }

  return(SUCCESS);
  }  /* END MJobRefreshFromNodeLists() */




/**
 * Force job to start regardless of policies, reservations, etc.
 *
 * NOTE: only support single req jobs with hostlists
 *
 * @param J (I) [modified]
 */

int MJobForceStart(

  mjob_t *J)  /* I (modified) */

  {
  const char *FName = "MJobForceStart";

  MDB(4,fSCHED) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((MNLIsEmpty(&J->ReqHList)) || 
      (J->Req[0] == NULL) || 
      (J->Req[1] != NULL))
    {
    return(FAILURE);
    }

  MNLCopy(&J->Req[0]->NodeList,&J->ReqHList);

  if (MJobStart(J,NULL,NULL,"force run job started") == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobForceStart() */




/**
 * Run job immediately, use escalation as needed.
 *
 * @see MQueueScheduleIJobs() - parent
 *
 * @param J (I)
 */

int MJobRunNow(

  mjob_t *J)  /* I */

  {
  mpar_t *P;
  mreq_t  *RQ;

  mnl_t  tmpNL;
  mnl_t *MNodeList[MMAX_REQ_PER_JOB];
  mnl_t *tmpMNodeList[MMAX_REQ_PER_JOB];  /* used for node sets */

  int      tmpSC;

  char    *NodeMap = NULL;

  int      DefTC[MMAX_REQ_PER_JOB];

  enum MNodeAllocPolicyEnum NAPolicy;

  mbool_t  PReq;

  int      pindex;
  int      rqindex;
  int      sindex;

  int      BestSIndex;
  int      BestSValue;

  mulong   DefaultSpecWCLimit;

  /* NOTE:  job previously granted highest system priority */

  if (MNodeMapInit(&NodeMap) == FAILURE)
    {
    return(SUCCESS);
    }

  MNLInit(&tmpNL);
  MNLMultiInit(MNodeList);
  MNLMultiInit(tmpMNodeList);

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\1')
      continue;

    if (P->Name[0] == '\0')
      break;

    if (!bmisset(&J->Flags,mjfCoAlloc))
      {
      if (pindex == 0)
        {
        /* only coalloc jobs was utilize the global partition */

        continue;
        }

      if ((MSched.SharedPtIndex == P->Index) && (J->Req[0]->DRes.Procs > 0))
        {
        /* skip shared partition */

        continue;
        }
      }
    else
      {
      if (pindex != 0)
        {
        /* coalloc jobs may only utilize the global partition */

        break; 
        }
      }

    if (P->ConfigNodes == 0)
      {
      /* partition is empty */

      continue;
      }

    if (P->Index > 0)
      { 
      if (!bmisset(&J->CurrentPAL,P->Index))
        {
        /* job cannot access partition */

        continue;
        }

      if ((P->RM != NULL) &&
          (P->RM->Type == mrmtMoab) &&
          !bmisset(&P->RM->Flags,mrmfSlavePeer))
        {
        /* remote peers (non-slave) have already been analyzed (above) */

        continue;
        }
      }

    /* NYI:  analyze data staging requirements */

    MStat.EvalJC++;

    if (MReqGetFNL(
         J,
         J->Req[0],
         P,
         NULL,
         &tmpNL, /* O */
         NULL,
         NULL,
         MMAX_TIME,
         NULL,
         NULL) == FAILURE)
      {
      /* insufficient feasible nodes found */

      MDB(4,fSCHED) MLog("INFO:     cannot locate adequate feasible tasks for job %s:%d in partition %s\n",
        J->Name,
        0,
        P->Name);

      continue;
      }

    BestSValue = 0;
    BestSIndex = -1; 
 
    NAPolicy = (J->Req[0]->NAllocPolicy != mnalNONE) ?
      J->Req[0]->NAllocPolicy:
      P->NAllocPolicy;
 
    DefaultSpecWCLimit = J->SpecWCLimit[0];
  
    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      DefTC[rqindex] = J->Req[rqindex]->TaskRequestList[0];
      }  /* END for (rqindex) */

    /* multi-req jobs only have as many shapes as are found in the primary req */

    if (bmisset(&J->Flags,mjfNoResources))
      {
      /* only one shape allowed */

      BestSIndex = 1;
      }

    /* NYI: no malleable job support */

    for (sindex = 1;sindex < MMAX_SHAPE;sindex++)
      {
      if (((J->Req[0]->TaskRequestList[sindex] <= 0) && 
           !bmisset(&J->Flags,mjfNoResources)) ||
          ((sindex > 1) && bmisset(&J->Flags,mjfNoResources)))
        {
        /* end of list located */

        break;
        }

      J->SpecWCLimit[0] = J->SpecWCLimit[sindex];

      J->Request.TC = 0;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        RQ->TaskRequestList[0]  = RQ->TaskRequestList[sindex];
        RQ->TaskCount           = RQ->TaskRequestList[sindex];
        J->Request.TC          += RQ->TaskRequestList[sindex];
        }  /* END for (rqindex) */
 
      /* is the best strategy widest fit?  ie, local greedy? */
 
      MJobUpdateResourceCache(J,NULL,sindex,NULL);
 
      MDB(4,fSCHED) MLog("INFO:     checking job %s(%d)  state: %s (ex: %s)\n",
        J->Name,
        sindex,
        MJobState[J->State],
        MJobState[J->EState]);
 
      /* select resources for job */ 
 
      /* check available resources on 'per shape' basis */

      /* 1 - attempt normal job start (w job as preemptor) */
 
      if ((MJobSelectMNL(
            J,
            P,
            NULL,      /* I - provide no feasible node list */
            MNodeList, /* O */
            NodeMap,   /* O */
            TRUE,
            FALSE,
            &PReq,
            NULL,
            NULL) == FAILURE) &&
         (MJobSelectMNL(     /* optimal cancel job list */
            J,
            P,
            NULL,      /* I - provide no feasible node list */
            MNodeList, /* O */
            NodeMap,   /* O */
            TRUE,
            TRUE,
            &PReq,
            NULL,
            NULL) == FAILURE))
        {
        /* NYI: determine extended rsv access (NOTE:  should there be rsv's exempt from RunNow) */

        MDB(6,fSCHED) MLog("INFO:     cannot locate %d tasks for job '%s' in partition %s\n",
          J->Request.TC,
          J->Name,
          P->Name);
 
        continue;
        }  /* END:  if (MJobSelectMNL() == FAILURE) */

      BestSValue = 1;
      BestSIndex = sindex;
      }    /* END for (sindex) */

    J->SpecWCLimit[0] = DefaultSpecWCLimit;
    J->Request.TC = 0;

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];

      RQ->TaskRequestList[0]  = DefTC[rqindex]; 
      RQ->TaskCount           = DefTC[rqindex];
      J->Request.TC          += DefTC[rqindex];
      }  /* END for (rqindex) */
 
    MJobUpdateResourceCache(J,NULL,0,NULL);
 
    if (BestSValue <= 0)
      {
      /* no available slot located */

      continue;
      }    /* END if (BestSValue <= 0) */
 
    /* adequate nodes found */
 
    J->SpecWCLimit[0] = J->SpecWCLimit[BestSIndex];
    J->Request.TC     = 0;

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];

      RQ->TaskRequestList[0]  = RQ->TaskRequestList[BestSIndex];
      RQ->TaskCount           = RQ->TaskRequestList[BestSIndex];
      J->Request.TC          += RQ->TaskRequestList[BestSIndex];

      /* RQ->TaskRequestList[BestSIndex] = DefTC[rqindex]; */
      }  /* END for (rqindex) */

    /* NOTE:  do not store default info in TaskRequestList (DBJ-Mar02/04) */
 
    /* J->SpecWCLimit[BestSIndex] = DefaultSpecWCLimit; */
 
    if (BestSIndex > 1)
      MJobUpdateResourceCache(J,NULL,BestSIndex,NULL);

    if (MPar[0].NodeSetIsOptional == FALSE)
      MNLMultiCopy(tmpMNodeList,MNodeList);
 
    if (MJobAllocMNL( 
          J,
          MNodeList,  /* I */
          NodeMap,    /* I */
          NULL,
          NAPolicy,
          MSched.Time,
          &tmpSC,     /* O */
          NULL) == FAILURE)
      {
      continue;
      }

    if (MJobStart(J,NULL,NULL,"run-now job started") == FAILURE)  /* FIXME: encapsulate in a policy to control immediate staging/starting */
      {
      continue;
      }

    MUFree(&NodeMap);
    MNLFree(&tmpNL);
    MNLMultiFree(MNodeList);
    MNLMultiFree(tmpMNodeList);

    return(SUCCESS);
    }      /* END for (pindex) */

  MUFree(&NodeMap);
  MNLFree(&tmpNL);
  MNLMultiFree(MNodeList);
  MNLMultiFree(tmpMNodeList);

  /* NOTE:  do not cancel/preempt other RunNow jobs */

  /* question:  do we optimize node allocation or minimize job disruption? */

  /* report message to admins regarding actions taken to start job */
 
  return(FAILURE);
  }  /* END MJobRunNow() */



/**
 * Combine children of 'syncmaster' workflow into single co-alloc job for 
 * guaranteed co-allocation scheduling.
 *
 * NOTE:  handles 'SyncMaster' jobs
 * NOTE:  real job is passed in, tmp pseudo job is passed out
 *
 * @see MQueueScheduleIJobs() - parent
 *
 * @param OrigJ (I)
 * @param ComboJP (O) super job is built and then set
 */

int MJobCombine(

  mjob_t  *OrigJ,
  mjob_t **ComboJP) /* I/O super job is built and then set */

  {
  int       rqindex;
  int       rqindex2;

  mjob_t   *tmpJ = NULL;

  mjob_t *JP;
  mjob_t *tJ;

  mdepend_t *D = NULL;

  const char *FName = "MJobCombine";

  MDB(3,fSCHED) MLog("%s(%s,ComboJP)\n",
    FName,
    (OrigJ != NULL) ? OrigJ->Name : "NULL");

  if ((OrigJ == NULL) || (OrigJ->Depend == NULL) || (ComboJP == NULL))
    {
    return(FAILURE);
    }

  JP = OrigJ;

  MJobMakeTemp(&tmpJ);

  MJobDuplicate(tmpJ,JP,FALSE);

  bmset(&tmpJ->IFlags,mjifMasterSyncJob);

  /* FIXME:  NULL out pointers that should not be freed off the temporary job */

  rqindex = 0;

  while (tmpJ->Req[rqindex] != NULL)
    {
    rqindex++;
    }

  /* FIXME: this assumes that the synccount dependency is always first
            and all the rest of the dependencies in the linked list are
            syncwith dependencies */

  /* only process 'AND' dependencies */

  D = JP->Depend->NextAnd;

  rqindex2 = 0;

  while (D != NULL)
    {
      {
      D = D->NextAnd;

      continue;
      }
 
    rqindex2++;

    D = D->NextAnd;
    }  /* END (while D != NULL)) */

  if (rqindex2 >= MMAX_REQ_PER_JOB)
    {
    MJobSetHold(OrigJ,mhBatch,0,mhrAdmin,"job exceeds max synccount");

    while (D != NULL)
      {
      if (MJobFind(D->Value,&tJ,mjsmBasic) == FAILURE)
        {
        D = D->NextAnd;

        continue;
        }
 
      MJobSetHold(tJ,mhBatch,0,mhrAdmin,"job exceeds max synccount");

      D = D->NextAnd;
      }  /* END (while D != NULL)) */

    MJobFreeTemp(&tmpJ);

    return(FAILURE);
    }

  D = JP->Depend->NextAnd;

  while (D != NULL)
    {
    if (MJobFind(D->Value,&tJ,mjsmBasic) == FAILURE)
      {
      D = D->NextAnd;

      continue;
      }

    rqindex2 = 0;

    while (tJ->Req[rqindex2] != NULL)
      {
      if (tmpJ->Req[rqindex] == NULL)
        {
        tmpJ->Req[rqindex] = (mreq_t *)MUMalloc(sizeof(mreq_t));
        }

      MDB(3,fSCHED) MLog("INFO:     combinining job '%s' and '%s'\n",
        tmpJ->Name,
        tJ->Name);

      memcpy(tmpJ->Req[rqindex],tJ->Req[rqindex2],sizeof(mreq_t));

      tmpJ->Req[rqindex]->Index = rqindex;

      rqindex++;
      rqindex2++;

      /* when this super-job is scheduled, the reservations can divided onto the
         corresponding jobs in the same order */
      }

    /* add tJ->R to tmpJ->R ACL */

    if (tJ->Rsv != NULL)
      {
      MJobAddAccess(tmpJ,tJ->Rsv->Name,TRUE);
      }

    /* FIXME:  NULL out pointers that cannot be freed off the temporary req */

    D = D->NextAnd;
    }

  *ComboJP = tmpJ;

  return(SUCCESS);
  }  /* END MJobCombine() */





/**
 * Handle syncwith' job dependencies by traversing J->Depend and starting all 
 * syncslave dependent jobs simultaneously
 *
 * @see MJobStart() - parent/child
 * @see MJobSyncStartRsv() - peer
 *
 * @param J (I)
 */

int MJobSyncStart(

  mjob_t *J)  /* I */

  {
  mdepend_t *D = NULL;

  mjob_t *tJ;

  const char *FName = "MJobSyncStart";

  MDB(3,fSCHED) MLog("%s(J)\n",
    FName);

  if ((J == NULL) || (J->Depend == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  is J->Index pointing to something other than J? */

  MJobStart(J,NULL,NULL,"sync job started");

  /* NOTE:  only process 'AND' dependencies */

  for (D = J->Depend->NextAnd;D != NULL;D = D->NextAnd)
    {
    if (MJobFind(D->Value,&tJ,mjsmBasic) == FAILURE)
      continue;

    if (MDependFind(tJ->Depend,mdJobSyncSlave,J->Name,NULL) == FAILURE)
      /* skip dependencies that are not slave jobs to J */
      continue;

    assert(MJOBISIDLE(tJ));

    MJobStart(tJ,NULL,NULL,"sync job started");
    }  /* END for (D) */

  return(SUCCESS);
  }  /* END MJobSyncStart() */




/**
 * Modify job environment variable list  
 *
 * NOTE:  do not push new env value to RM
 * NOTE:  old value is replaced, not appended to
 *
 * @see MJobInsertEnv - peer -  pushes new env var to RM.
 *
 * @param J (I) [modified]
 * @param Var (I)
 * @param Value (I) [optional]
 * @param Delim (I)
 */

int MJobAddEnvVar(

  mjob_t     *J,
  const char *Var,
  const char *Value,
  char        Delim)

  {
  mbool_t VarExists;

  char *ptr;
  char *tail;

  int   rlen;
  int   BufSize;

  if ((J == NULL) || (Var == NULL))
    {
    return(FAILURE);
    }

  mstring_t tmpBuf(MMAX_LINE);

  tmpBuf = Var;
  tmpBuf += '=';
  tmpBuf += (Value != NULL) ? Value : "";

  if (J->Env.BaseEnv == NULL)
    {
    MUStrDup(&J->Env.BaseEnv,tmpBuf.c_str());

    return(SUCCESS);
    }

  /* determine if variable is already set */
 
  VarExists = FALSE;

  ptr = J->Env.BaseEnv;

  while ((ptr = strstr(ptr,Var)) != NULL)
    {
    /* FORMAT: {<START>|<DELIM>}<VAR>{=} */

    if (ptr[strlen(Var)] != '=')
      {
      ptr++;

      continue;
      }

    if ((ptr > J->Env.BaseEnv) && (ptr[-1] != Delim))
      {
      ptr++;

      continue;
      }

    VarExists = TRUE;

    break;
    }  /* END while (ptr) */

  if (VarExists == FALSE)
    {
    /* variable not set, append value */

    MUStrAppend(&J->Env.BaseEnv,NULL,tmpBuf.c_str(),Delim);

    return(SUCCESS);
    }

  /* variable set, replace value */

  ptr += strlen(Var) + 1;

  /* determine length of old variable value */

  tail = strchr(ptr,Delim);

  /* skip delimiters that are escaped by '\' */

  while ((tail != NULL) && (tail[-1] == '\\'))
    {
    tail = strchr(tail+1,Delim);
    }

  if (tail != NULL)
    rlen = tail - ptr;
  else
    rlen = strlen(ptr);

  BufSize = strlen(J->Env.BaseEnv) + 1;

  if (MUBufReplaceString(
        &J->Env.BaseEnv,              /* I/O */
        &BufSize,                   /* I/O */
        ptr - J->Env.BaseEnv,
        rlen,
        Value,
        TRUE) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobAddEnvVar() */





/**
 * Determine if job J can access partition P.
 *
 * NOTE:  only check FS tree based access constraints.
 *
 * @see MJobCheckParAccess() - parent
 *
 * @param J (I)
 * @param P (I)
 * @param RIndexP (O) optional
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobEvaluateParAccess(

  mjob_t             *J,
  const mpar_t       *P,
  enum MAllocRejEnum *RIndexP,
  char               *EMsg)

  {
  mbool_t FSTreeRead = FALSE;

  const char *FName = "MJobEvaluateParAccess";

  MDB(5,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : NULL,
    (P != NULL) ? P->Name : NULL);

  if ((J == NULL) || (P == NULL))
    return(FAILURE);

  if (bmisset(&J->SpecFlags,mjfRsvMap))
    {
    /* psuedo-rsv jobs require no fstree checks */

    return(SUCCESS);
    }

  if (!bmisset(&J->IFlags,mjifFSTreeProcessed))
    {
    MJobGetFSTree(J,P,FALSE);

    FSTreeRead = TRUE;
    }

  if (MSched.FSTreeIsRequired == TRUE)
    {
    if ((J->FairShareTree == NULL) && (FSTreeRead == FALSE))
      {
      MJobGetFSTree(J,P,FALSE);

      FSTreeRead = TRUE;
      }

    if (J->FairShareTree == NULL)
      {
      if (EMsg != NULL)
        {
        if (MSched.SpecEMsg[memInvalidFSTree] != NULL)
          MUStrCpy(EMsg,MSched.SpecEMsg[memInvalidFSTree],MMAX_LINE);
        else
          MUStrCpy(EMsg,"no valid fstree node found",MMAX_LINE);
        }

      bmset(&J->IFlags,mjifFairShareFail);

      MDB(7,fSTRUCT) MLog("INFO:     job %s no fstree loaded\n",
        J->Name);

      return(FAILURE);
      }

    if ((J->FairShareTree[P->Index] == NULL) && (FSTreeRead == FALSE))
      {
      MJobGetFSTree(J,P,FALSE);

      FSTreeRead = TRUE;
      }

    if ((J->FairShareTree[P->Index] == NULL) && (J->FairShareTree[0] == NULL))
      {
      if (EMsg != NULL)
        {
        if (MSched.SpecEMsg[memInvalidFSTree] != NULL)
          MUStrCpy(EMsg,MSched.SpecEMsg[memInvalidFSTree],MMAX_LINE);
        else
          MUStrCpy(EMsg,"no valid fstree node found",MMAX_LINE);
        }

      bmset(&J->IFlags,mjifFairShareFail);

      MDB(7,fSTRUCT) MLog("INFO:     job %s no fstree loaded for partition %s\n",
        J->Name,
        P->Name);

      return(FAILURE);
      }
    }

  /* this check should not be needed as tree limits will be evaluated 
     along with all other limits */

/*
  if (MJobCheckLimits(J,mptSoft,P,(1 << mlActive),EMsg) == FAILURE)
    {
    return(FAILURE);
    }
*/

  return(SUCCESS);
  }  /* END MJobEvaluateParAccess() */




/**
 * Add to a job's queue duration.
 *
 * If we don't add to the duration and the policy is reset then set it to 0.
 *
 * @param J (I)
 * @param P (I)
 */

int MJobAddEffQueueDuration(

  mjob_t *J,
  mpar_t *P)

  {
  enum MJobPrioAccrualPolicyEnum JobPrioAccrualPolicy;
  mbitmap_t *JobPrioExceptions = NULL;

  MDB(9,fSCHED) MLog("MJobAddEffQueueDuration(%s)\n",J->Name); /* BRIAN - debug (2400) */
  
  if ((J->Credential.Q != NULL) &&
      (J->Credential.Q->JobPrioAccrualPolicy != mjpapNONE))
    {
    JobPrioAccrualPolicy = J->Credential.Q->JobPrioAccrualPolicy;
    JobPrioExceptions = &J->Credential.Q->JobPrioExceptions;
    }
  else
    {
    JobPrioAccrualPolicy = MPar[0].JobPrioAccrualPolicy;
    JobPrioExceptions = &MPar[0].JobPrioExceptions;
    }

  switch (JobPrioAccrualPolicy)
    {
    case mjpapReset:
      
      if (bmisset(&J->IFlags,mjifUIQ) ||
          ((!bmisclear(JobPrioExceptions)) &&
            bmisset(&J->IFlags,mjifNoViolation)))
        {
        /* job was in the idle queue the last iteration OR
           exceptions are allowed and this job is OK */

        J->EffQueueDuration += (MSched.Interval + 50) / 100;

        if (J->EligibleTime == 0)
          J->EligibleTime = MSched.Time;
        }
      else
        {
        J->EffQueueDuration = 0;
        J->SystemQueueTime = MSched.Time;

        if (!MJOBISACTIVE(J) && !MJOBISCOMPLETE(J))
          J->EligibleTime = 0;
        }

      break;

    case mjpapAccrue:
    default:

      /* Don't allow an active job to accrue EffQueueDuration */
      if (MJOBISACTIVE(J))
        break;

      MDB(9,fSCHED) /* BRIAN - debug (2400) */
        {
        MLog("INFO:    acrruing priority\n");
      
        if (bmisset(&J->IFlags,mjifUIQ))
          MLog("INFO:    mjifUIQ set\n");
        if (!bmisclear(JobPrioExceptions))
          MLog("INFO:    JobPrioExecption != 0\n");
        if (bmisset(&J->IFlags,mjifNoViolation))
          MLog("INFO:    mjifNoViolation set\n");
        }

      if (bmisset(&J->IFlags,mjifUIQ) ||
          ((!bmisclear(JobPrioExceptions)) &&
            bmisset(&J->IFlags,mjifNoViolation)))
        {
        /* job was in the idle queue the last iteration OR
           exceptions are allowed and this job is OK */

        J->EffQueueDuration += (MSched.Interval + 50) / 100;

        if (J->EligibleTime == 0)
          J->EligibleTime = MSched.Time;

        MDB(9,fSCHED) MLog("INFO:    (MJobAddEffQueu..) job %s new EffqueueDuration %ld\n", /* BRIAN - debug (2400) */
            J->Name,
            J->EffQueueDuration);
        }
    
      break;
    }  /* END switch (JobPrioAccrualPolicy) */
  return(SUCCESS);
  }  /* END MJobAddEffQueueDuration() */


/**
 * Cancels (and removes if DoRemoteOnly is FALSE) the given job.
 * 
 * NOTE:  DANGER - J may be freed
 *
 * @param J (I) The job to cancel.
 * @param Message (I) [optional]
 * @param DoRemoteOnly (I) If TRUE the job will only be canceled from remote RMs and
 * will not be deleted from Moab's internal queue. If FALSE, the job will be canceled
 * and removed locally as well.
 * @param SEMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional] 
 */

int MJobCancel(

  mjob_t      *J,       /* I */
  const char  *Message, /* I (optional) */  
  mbool_t      DoRemoteOnly, /* I */
  char        *SEMsg,   /* O (optional,minsize=MMAX_LINE) */
  int         *SC)      /* O (optional) */

  {
  return MJobCancelWithAuth(NULL,J,Message,DoRemoteOnly,SEMsg,SC);
  }



/**
 * Cancels (and removes if DoRemoteOnly is FALSE) the given job.
 * 
 * NOTE:  DANGER - J may be freed
 *  
 * @param Auth (I) Authentication User Name 
 * @param J (I) The job to cancel.
 * @param Message (I) [optional]
 * @param DoRemoteOnly (I) If TRUE the job will only be canceled from remote RMs and
 * will not be deleted from Moab's internal queue. If FALSE, the job will be canceled
 * and removed locally as well.
 * @param SEMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional] 
 */

int MJobCancelWithAuth(

  const char  *Auth,
  mjob_t      *J,
  const char  *Message,
  mbool_t      DoRemoteOnly,
  char        *SEMsg,
  int         *SC)

  {
  int rc;
  char *EMsg;
  char DEMsg[MMAX_LINE];

  const char *FName = "MJobCancel";

  MDB(3,fSCHED) MLog("%s(%s,%s,%s,EMsg,SC)\n",
    FName,
    J != NULL ? J->Name : "",
    Message != NULL ? "Message" : "",
    DoRemoteOnly ? "TRUE" : "FALSE");

  if (SC != NULL)
    {
    *SC = mscNoError; 
    }
  
  if (J == NULL)
    {
    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);  
    }

  if (SEMsg != NULL)
    {
    SEMsg[0] = '\0';
    EMsg = SEMsg;
    }
  else
    EMsg = DEMsg;

  if (J->System != NULL)
    {
    if (bmisset(&J->IFlags,mjifRequestForceCancel))
      {
      /* -F will work for all system jobs, but there is no validation checking */

      return(MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC));
      }

    if (J->State == mjsIdle)
      {
      /* cancelling idle system job - undo any pending state actions */

      switch (J->System->JobType)
        {
        case msjtPowerOn:

          /* when a poweron job is submitted, job is removed from greenpool */

          /* @see MSysSchedulePower() */

          /* NYI */

          break;

        default:

          /* NO-OP */

          break;
        }
      }
    }    /* END if (J->System != NULL) */

  if (J->Triggers != NULL)
    {
    MOReportEvent((void *)J,J->Name,mxoJob,mttCancel,MSched.Time,TRUE);

    MSchedCheckTriggers(J->Triggers,-1,NULL);
    }

  /* Check for system jobs, some cannot be cancelled */

  if (J->System != NULL)
    {
    /* Check for VMStorage job. */

#if 0
    if (J->System->JobType == msjtVMStorage)
      {
      if (!bmisset(&J->IFlags,mjifRequestForceCancel))
        {
        /* Can't cancel VMStorage job with mjobctl -c.  To force a kill, use mjobctl -F <storage_job>.
        ** We prefer the user cancel the vm create job. */
    
        sprintf(EMsg,"VM Storage jobs should be cancelled by the VM Create job.");
    
        return(FAILURE);
        }
      else
        {
        /* Force the cancelation of a storage job.  
        ** Note:  MJobCancel already checks for running system jobs. */
  
        return (MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC));
        }
      }
    else if (J->System->JobType == msjtVMTracking)
#endif
    if (J->System->JobType == msjtVMTracking)
      {
      /* Create a VM Destroy job */

      mvm_t   *V;

      if (MVMFind(J->System->VM,&V) == FAILURE)
        {
        if ((MJobWorkflowHasStarted(J) == FALSE) || (bmisset(&J->IFlags,mjifDestroyByDestroyTemplate)))
          {
          /* VM hasn't been created yet or setup failed, just destroy */

          rc = MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC);
          }
        else
          {
          /* VM setup may be in progress */

          if (bmisset(&J->Flags,mjfDestroyTemplateSubmitted))
            {
            sprintf(EMsg,"a destroy job has already been submitted");

            rc = FAILURE;
            }
          else
            {
            rc = MTJobCreateByAction(J,mtjatDestroy,NULL,NULL,Auth);
            }
          }

        return(rc);
        }

      if (bmisset(&J->IFlags,mjifDestroyByDestroyTemplate))
        {
        /* Job was destroyed by a destroy template, move VM to completed queue */

        if (MVMComplete(V) == FAILURE)
          return(FAILURE);

        rc = MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC);

        return(rc);
        }

      if (bmisset(&J->IFlags,mjifVMDestroyCreated))
        {
        /* A destroy job has already been created, don't create a second one */

        sprintf(EMsg,"a vmdestroy job has already been submitted");

        return(FAILURE);
        }

      if (bmisset(&V->Flags,mvmfDestroySubmitted))
        {
        sprintf(EMsg,"a vmdestroy job has already been submitted");

        return(FAILURE);
        }

      if (!bmisset(&V->IFlags,mvmifReadyForUse) && (V->CreateJob != NULL))
        {
        /* VM has not been instantiated yet */

        if (MJobCancel(V->CreateJob,"",FALSE,NULL,NULL) == SUCCESS)
          {
          rc = MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC);

          return(rc);
          }
        else
          {
          sprintf(EMsg,"failed to create vmdestroy job");

          return(FAILURE);
          }
        }

      /* If this job has a destroy template, use it */

      if (bmisset(&J->Flags,mjfDestroyTemplateSubmitted))
        {
        /* Template was already submitted, see if it was successful */

        if (!bmisset(&J->IFlags,mjifDestroyByDestroyTemplate))
          {
          sprintf(EMsg,"a destroy job has already been submitted");

          return(FAILURE);
          }
        }
      else
        {
        /* See if this job has a destroy template */

        if (MTJobCreateByAction(J,mtjatDestroy,NULL,NULL,Auth) == SUCCESS)
          return(SUCCESS);
        }
#if 0
      return MVMSubmitDestroyJob(Auth,V,NULL);    
#endif
      return(FAILURE);
      } /* END else if (J->System->JobType == msjtVMTracking) */
    } /* END if (J->System != NULL) */

#if 0
  /* Check for VMCreate job */

  if ((J->System != NULL) && 
      (J->System->JobType == msjtVMCreate))
    {
    /* This must be done whether the job is running or idle.  Running because
       we don't have fine control over vmcreation yet, and Idle to get rid of
       any created storage */

    mvm_t *V = NULL;
    marray_t JArray;

    if (bmisset(&J->IFlags,mjifVMDestroyCreated) &&
        (!bmisset(&J->IFlags,mjifRequestForceCancel)))
      {
      /* A destroy job has already been created, don't create a second one */

      return(SUCCESS);
      }

    if (MVMFind(J->System->VM,&V) == FAILURE)
      {
      MDB(3,fSTRUCT) MLog("ERROR:    failed to find VM '%s' to destroy in vmcreate cancellation request",
        (J->System->VM != NULL) ? J->System->VM : "NULL");

      if (bmisset(&J->IFlags,mjifRequestForceCancel))
        {
        /* NOTE: admin should try to cancel WITHOUT ForceCancel first */
        /*  This is meant to clean up jobs from Moab's records */

        rc = MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC);

        return(rc);
        }

      return(FAILURE);
      }

    if (bmisset(&J->IFlags,mjifRequestForceCancel))
      {
      /* NOTE: admin should try to cancel WITHOUT ForceCancel first */
      /*  This is meant to clean up jobs from Moab's records */

      MVMComplete(V);
      rc = MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC);

      return(rc);
      }

    if (J->State == mjsIdle)
      {
      /* Job hasn't started yet - see if any storage jobs have started.
          If not, just cancel the job and remove the vm */

      mjob_t *DJ = NULL;
      mdepend_t *D = J->Depend;
      mbool_t NonIdleFound = FALSE;

      while ((D != NULL) && (D->Type != mdNONE) && (D->Value != NULL))
        {
        if (D->Satisfied == TRUE)
          {
          /* Dependency (storage job) has already run */
          NonIdleFound = TRUE;

          break;
          }

        if (MJobFind(D->Value,&DJ,mjsmJobName) == SUCCESS)
          {
          if (DJ->State != mjsIdle)
            {
            NonIdleFound = TRUE;

            break;
            }
          }

        D = D->NextAnd;
        } /* END while ((D != NULL) ... ) */

      if (NonIdleFound == FALSE)
        {
        /* Job and dependencies haven't started, delete them all */

        D = J->Depend;
        while ((D != NULL) && (D->Type != mdNONE) && (D->Value != NULL))
          {
          if (MJobFind(D->Value,&DJ,mjsmJobName) == SUCCESS)
            {
            MRMJobCancel(DJ,Message,DoRemoteOnly,EMsg,SC);
            }

          D = D->NextAnd;
          }

        MVMComplete(V);
        rc = MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC);

        return(rc);
        }
      } /* END if (J->State == Idle) */

    if (V != NULL)
      {
      mjob_t *DestroyJ;
      char DepStr[MMAX_LINE];
      mnode_t *N;
      mnl_t tmpNL;

      /* VM node list may not have been populated at this point,
          use vmcreate job nodelist */

      MNLInit(&tmpNL);
      MNLGetNodeAtIndex(&J->NodeList,0,&N);
      MNLSetNodeAtIndex(&tmpNL,0,N);
      MNLSetTCAtIndex(&tmpNL,0,MNLGetTCAtIndex(&J->NodeList,0));

      MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

      if (MUGenerateSystemJobs(
          Auth,
          NULL,
          &tmpNL,
          msjtVMDestroy,
          "vmdestroy",
          0,
          V->VMID,
          V,
          FALSE,
          FALSE,
          NULL,
          &JArray) == FAILURE)
        {
        MDB(3,fSTRUCT) MLog("ERROR:    failed to create vmdestroy job for VM '%s' in vmcreate cancellation request",
          V->VMID);

        MNLFree(&tmpNL);
 
        MUArrayListFree(&JArray);

        return(FAILURE);
        }

      MNLFree(&tmpNL);

      bmset(&J->IFlags,mjifVMDestroyCreated);

      /* make destroy job dependent on create job completion */

      DestroyJ = *(mjob_t **)MUArrayListGet(&JArray,0);

      snprintf(DepStr,sizeof(DepStr),"afterany:%s",
        J->Name);

      MJobAddDependency(DestroyJ,DepStr);

      MUArrayListFree(&JArray);
      } /* END if (V != NULL) */

    return(SUCCESS); 
    } /* END if ((J->System != NULL) && (J->System->JobType == msjtVMCreate)) */
#endif

  /* If this job has a destroy template, use it */

  if (bmisset(&J->Flags,mjfDestroyTemplateSubmitted))
    {
    /* Template was already submitted, see if it was successful */

    if (!bmisset(&J->IFlags,mjifDestroyByDestroyTemplate))
      {
      if (!bmisset(&J->IFlags,mjifRequestForceCancel))
        {
        sprintf(EMsg,"a destroy job has already been submitted");

        return(FAILURE);
        }
      else
        {
        return (MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC));
        }
      }
    }
  else
    {
    /* See if this job has a destroy template */

    if (MJobWorkflowHasStarted(J) == TRUE)
      {
      /* Don't need a template destroy job if workflow hasn't started */

      if (MTJobCreateByAction(J,mtjatDestroy,NULL,NULL,Auth) == SUCCESS)
        return(SUCCESS);
      }
    }

  rc = MRMJobCancel(J,Message,DoRemoteOnly,EMsg,SC);
  
  return(rc);
  }  /* END MJobCancelWithAuth() */






/**
 * Cycle through list of jobs calling MJobDestroy() on each.
 *
 * @param JList (I) Array of jobs to be destroyed. It is terminated by either a 
 *   NULL pointer or an empty JobID field.
 * @param JobCount (I) The number of jobs in the given list.
 */

int MJobDestroyList(

  mjob_t *JList,
  int     JobCount)

  {
  int jindex;
  mjob_t *tmpJ;

  const char *FName = "MJobDestroyList";

  MDB(3,fSCHED) MLog("%s(%s,%d)\n",
    FName,
    (JList != NULL) ? "JList" : "",
    JobCount);

  if (JList == NULL)
    {
    return(FAILURE);
    }

  for (jindex = 0;jindex < JobCount;jindex++)
    {
    if (JList[jindex].Name[0] == '\0')
      {
      break;
      }

    tmpJ = &JList[jindex];

    MJobDestroy(&tmpJ);
    }  /* END for (jindex) */

  return(SUCCESS);
  }  /* END MJobDestroyList() */







/**
 * This function will update each of the job's allocated nodes' job failure rate
 * statistics.
 *
 * @param J (I)
 */

int MJobUpdateFailRate(

  mjob_t *J)

  {
  mbool_t JobFailed = FALSE;

  int tmpNIndex;
  mnode_t *CCodeN;

  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((MSched.JobCCodeFailSampleSize <= 0) || 
      (MNLIsEmpty(&J->NodeList)))
    {
    return(SUCCESS);
    }

  /* check completion code, update node failed completion code count */

  if (J->CompletionCode != 0)
    {
    JobFailed = TRUE;
    }

  /* iterate through each node, increment JobCCodeFailureCount */

  for (tmpNIndex = 0;MNLGetNodeAtIndex(&J->NodeList,tmpNIndex,&CCodeN) == SUCCESS;tmpNIndex++)
    {
    int tmpCCIndex;

    if ((CCodeN->JobCCodeSampleCount == NULL) ||
        (CCodeN->JobCCodeFailureCount == NULL))
      {
      /* nodes aren't setup to count this stat */

      continue;
      }

    if (CCodeN->JobCCodeSampleCount[0] >= MSched.JobCCodeFailSampleSize) /* check to roll ccode stats */
      {
      for (tmpCCIndex = MMAX_CCODEARRAY - 1;tmpCCIndex > 0;tmpCCIndex--)
        {
        CCodeN->JobCCodeFailureCount[tmpCCIndex] = CCodeN->JobCCodeFailureCount[tmpCCIndex - 1];
        CCodeN->JobCCodeSampleCount[tmpCCIndex] = CCodeN->JobCCodeSampleCount[tmpCCIndex - 1];
        }

      CCodeN->JobCCodeFailureCount[0] = 0;
      CCodeN->JobCCodeSampleCount[0] = 0;
      }

    CCodeN->JobCCodeSampleCount[0]++;

    if (JobFailed == TRUE)
      CCodeN->JobCCodeFailureCount[0]++;
    }

  return(SUCCESS);
  }  /* END MJobUpdateFailRate() */









/**
 * Create a new job from attributes of both template TJob and job J on which 
 * job J will depend.
 *
 * @see MJobApplySetTemplate() - parent
 *
 * @param J     (I) [modified]
 * @param TJob  (I)
 * @param JName (I) New job's name.
 */

int MJobCreateTemplateDependency(

  mjob_t *J,     /* I (modified) */
  mjob_t *TJob,  /* I */
  char   *JName) /* I */

  {
  mjob_t *NewJ;
  
  char FeatureLine[MMAX_LINE];
  char IWDLine[MMAX_LINE];
  char ExtString[MMAX_LINE];
  char ExecString[MMAX_LINE];

  mbitmap_t FlagBM;

  mrm_t *R;

  const char *FName = "MJobCreateTemplateDependency";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (TJob != NULL) ? TJob->Name : "NULL",
    (JName != NULL) ? JName : "NULL");

  if ((J == NULL) || (TJob == NULL) || (JName == NULL))
    return(FAILURE);

  /* NOTE:  XML template for creating job */

  /*
  <job>
    <JobName>MyName</JobName>
    <Owner>MyUser</Owner>
    <UserId>MyUser</UserId>
    <GroupId>MyGroup</GroupId>
    <InitialWorkingDirectory>/home/MyUser</InitialWorkingDirectory>
    <Executable>sleep 600</Executable>
    <SubmitLanguage>PBS</SubmitLanguage>
    <Requested>
      <Nodes>
        <Node>MyNode</Node>
      </Nodes>
      <Processors>1</Processors>
      <WallclockDuration>600</WallclockDuration>
    </Requested>
    <Extension>x=flags:dynamic:systemjob:normstart</Extension>
  </job>
  */

  MRMAddInternal(&R);

  if (!bmisclear(&TJob->Req[0]->ReqFBM))
    {
    char Line[MMAX_LINE];

    snprintf(FeatureLine,sizeof(FeatureLine),"<Feature>%s</Feature>",
      MUNodeFeaturesToString(':',&TJob->Req[0]->ReqFBM,Line));
    }
  else
    {
    FeatureLine[0] = '\0';
    }

  if (J->Env.IWD == NULL)
    {
    IWDLine[0] = '\0';
    }
  else
    {
    snprintf(IWDLine,sizeof(IWDLine),"<InitialWorkingDirectory>%s</InitialWorkingDirectory>",
      J->Env.IWD);
    }

  if ((TJob->Env.Cmd == NULL) || (J->RMSubmitString != NULL))
    {
    ExecString[0] = '\0';
    }
  else
    {
    snprintf(ExecString,sizeof(ExecString),"<Executable>%s</Executable>",
      TJob->Env.Cmd);
    }

  /* we use TEMPLATE:^ to specify that only requested template should be applied
   * to the job, even though we aren't requesting any yet */

  snprintf(ExtString,sizeof(ExtString),"x=JGROUP:%s;TEMPLATE:^",
    J->JGroup->Name);

  mstring_t BufString(MMAX_LINE*10);  /* set string to fairly large size to avoid instant realloc's */
  char  convertBuffer[64];

  /* NOTE:  if not explicitly specified w/in job template description, new job
            should inherit credentials from parent job */

  /* Generate a XML STRING */
  BufString = "<job><Extension>";
  BufString += ExtString;
  BufString += "</Extension><JobName>";
  BufString += JName;
  BufString += "</JobName>";
  BufString += IWDLine;
  BufString += "<UserId>";
  BufString += J->Credential.U->Name;
  BufString += "</UserId><GroupId>";
  BufString += ((mgcred_t *)J->Credential.U->F.GDef)->Name;
  BufString += "</GroupId><ProjectId>";
  BufString += (J->Credential.A != NULL) ? J->Credential.A->Name : "";
  BufString += "</ProjectId><QualityOfService>";
  BufString += (J->Credential.Q != NULL) ? J->Credential.Q->Name : "";
  BufString += "</QualityOfService>";
  BufString += ExecString;
  BufString += "<SubmitLanguage>PBS</SubmitLanguage>";
  BufString += FeatureLine;

  BufString += "<Requested><Processors>";
  snprintf(convertBuffer,sizeof(convertBuffer),"%d",
    (TJob->Request.TC != 0) ? TJob->Request.TC : J->Request.TC);
  BufString += convertBuffer;

  BufString += "</Processors><WallclockDuration>";
  snprintf(convertBuffer,sizeof(convertBuffer),"%ld",
    (TJob->SpecWCLimit[0] != 0) ? TJob->SpecWCLimit[0] : J->SpecWCLimit[0]);
  BufString += convertBuffer;

  BufString += "</WallclockDuration></Requested></job>";

  NewJ = NULL;

  bmset(&FlagBM,mjsufTemplateDepend);

  MS3JobSubmit(BufString.c_str(),R,&NewJ,&FlagBM,JName,NULL,NULL);

  bmclear(&FlagBM);

  /* change by DRW 7/12/12
     MJobDuplicate should only be called for InheritRes templates,
     and that is handled in __MJobTemplateApplyWorkflow(), this
     routine just creates a generic job

  MJobDuplicate(NewJ,J,TRUE,FALSE);
  */

  MUStrCpy(NewJ->Name,JName,sizeof(NewJ->Name));
  MUStrDup(&NewJ->AName,JName);

  /* Check now for a workflow parent VC */

  if (TJob->ParentVCs != NULL)
    {
    mln_t *tmpVCL;
    mvc_t *tmpVC;

    for (tmpVCL = TJob->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
      {
      tmpVC = (mvc_t *)tmpVCL->Ptr;

      if (bmisset(&tmpVC->Flags,mvcfWorkflow))
        {
        MVCAddObject(tmpVC,(void *)NewJ,mxoJob);

        break;
        }
      }
    }  /* END if (TJob->ParentVCs != NULL) */

  bmset(&J->IFlags,mjifCreatedByTemplateDepend);

  /* Apply template now to avoid the check for whether or not the template
   * is selectable by a user or not */

  MJobApplySetTemplate(NewJ,TJob,NULL);

  MJobSetAttr(NewJ,mjaSID,(void **)MSched.Name,mdfString,mSet);
  MJobSetAttr(NewJ,mjaGJID,(void **)NewJ->Name,mdfString,mSet);

  if (bmisset(&TJob->IFlags,mjifStageInExec)) /* Stagein executable */
    bmset(&NewJ->IFlags,mjifStageInExec);

  MJobTransition(NewJ,FALSE,FALSE);

  return(SUCCESS);
  }  /* END MJobCreateTemplateDependency() */



/**
 * Duplicates a job.
 * 
 * @see MReqDuplicate() - child
 *
 * NOTE: this routine only correctly duplicates new jobs, (ie. not running, no reservation, no utilization)
 *
 * @param Dst (O) [alloc]
 * @param Src (I)
 * @param DoTriggers (I)
 */

int MJobDuplicate(

  mjob_t        *Dst,
  const mjob_t  *Src,
  mbool_t        DoTriggers)

  {
  int index; /* used for copying for loops */
  mln_t  *SLPtr = NULL;

  if (!bmisset(&Dst->Flags,mjfArrayJob)) /* array jobs already have a name */
    {
    MUStrCpy(Dst->Name,Src->Name,sizeof(Dst->Name));  /* job name */
    }

  MUStrDup(&Dst->AName,Src->AName);    /**< alternate name - user specified (alloc) */

  MUStrDup(&Dst->Owner,Src->Owner);    /**< job owner (alloc)                       */
  MUStrDup(&Dst->EUser,Src->EUser);    /**< execution user (alloc,specified for user only) */

  Dst->CTime      = Src->CTime;       /**< creation time (first RM report)         */
  Dst->SWallTime  = Src->SWallTime;   /**< duration job was suspended              */
  Dst->AWallTime  = Src->AWallTime;   /**< duration job was executing              */

  Dst->TotalProcCount = Src->TotalProcCount;

  if (bmisset(&Dst->Flags,mjfArrayJob)) /* cred already allocated for array jobs */
    {
    /* duplicate access control lists */
    MJobCopyCL(Dst,Src);
    }
  else
    {
    if (Dst->Credential.ACL != NULL)
      MACLFreeList(&Dst->Credential.ACL);

    if (Dst->Credential.CL != NULL)
      MACLFreeList(&Dst->Credential.CL);

    memcpy(&Dst->Credential,&Src->Credential,sizeof(mcred_t));
    Dst->Credential.ACL = NULL;
    Dst->Credential.CL  = NULL;
    Dst->Credential.Templates  = NULL;
    }

  /*MJobBuildCL*/

  Dst->QOSRequested = Src->QOSRequested;           /**< quality of service requested (pointer only) */

  Dst->LogLevel = Src->LogLevel;       /**< verbosity of job tracking/logging       */

  MUStrDup(&Dst->ReqRID,Src->ReqRID);         /**< required rsv name/group (alloc)         */


  if (Src->RsvAList != NULL)
    {
    for (index = 0;index < MMAX_PJOB;index++)
      {
      if (Src->RsvAList[index][0] == '\0')
        break;

      MUStrDup(&Dst->RsvAList[index],Src->RsvAList[index]);
      }
    }

  Dst->SubmitRM = Src->SubmitRM;            /**< rm to which job is submitted */
  if (!bmisset(&Dst->Flags,mjfArrayJob)) /** remote job won't migrate if DRM is set */
    Dst->DestinationRM = Src->DestinationRM;            /**< rm on which job will execute */

  Dst->SessionID = Src->SessionID;

/*  mnode_t    *RejectN;        < last node this job was rejected on      */

  if (!bmisset(&Dst->Flags,mjfArrayJob))
    {
    MUStrDup(&Dst->NotifyAddress,Src->NotifyAddress);  /**< destination to send notifications */
    }

  bmcopy(&Dst->RsvAccessBM,&Src->RsvAccessBM);    /**< (bitmap of enum MJobAccessPolicyEnum)   */
  bmcopy(&Dst->NotifyBM,&Src->NotifyBM);
  MUStrDup(&Dst->SpecNotification,Src->SpecNotification); /**< rm-specific notification specified by user (alloc) */
  bmcopy(&Dst->NotifySentBM,&Src->NotifySentBM);   /**< email notifications sent (enum MJobNotifySentEnum BM) */
  bmcopy(&Dst->NodeMatchBM,&Src->NodeMatchBM);   /**< copy node match bitmap */
  bmcopy(&Dst->Hold,&Src->Hold);                    /**< bitmap of MHoldTypeEnum { ie, USER, SYSTEM, BATCH } */
  bmcopy(&Dst->ReqFeatureGResBM,&Src->ReqFeatureGResBM);

  if (!bmisclear(&Src->AttrBM))
    bmcopy(&Dst->AttrBM,&Src->AttrBM);

/*  msdata_t   *SIData;         < stage in data  */
/*  msdata_t   *SOData;         < stage out data */

  if (DoTriggers == TRUE)
    {
    MJobCopyTrigs(Src,Dst,TRUE,TRUE,TRUE);
    }

  /* Clear any existing reqs first */

  for (index = 0;index < MMAX_REQ_PER_JOB + 1; index++)
    {
    if (Dst->Req[index] == NULL)
      break;

    MReqDestroy(&Dst->Req[index]);
    }

  for (index = 0;index < MMAX_REQ_PER_JOB + 1; index++)
    {
    if (Src->Req[index] == NULL)
      break;

    /* MReqCreate will call MReqDuplicate */
    MReqCreate(Dst,Src->Req[index],NULL,TRUE);

    /* disable alternate task request processing for array jobs (-l trl) */
    if (bmisset(&Dst->Flags,mjfArrayJob))
      {
      Dst->Req[index]->TaskRequestList[2] = 0;
      }
    }

/*  mreq_t     *GReq;            ??? */

/*  mreq_t     *MReq;           < primary req (no alloc)                  */

  MUStrDup(&Dst->Env.IWD,Src->Env.IWD);                 /* initial working directory of job (alloc)     */
  MUStrDup(&Dst->Env.Cmd,Src->Env.Cmd);                 /* job executable (alloc)                       */
  MUStrDup(&Dst->Env.Input,Src->Env.Input);             /* requested input file (alloc)                 */

  if (!bmisset(&Dst->Flags,mjfArrayJob)) /* Output and Error strings already allocated for array subjobs */
    {
    MUStrDup(&Dst->Env.Output,Src->Env.Output);           /* requested/created output file (alloc)        */
    MUStrDup(&Dst->Env.Error,Src->Env.Error);             /* requested/created error file (alloc)         */    
    }

  MUStrDup(&Dst->Env.RMOutput,Src->Env.RMOutput);       /* stdout file created by RM (alloc)            */
  MUStrDup(&Dst->Env.RMError,Src->Env.RMError);         /* stderr file created by RM (alloc)            */
  MUStrDup(&Dst->Env.Args,Src->Env.Args);               /* command line args (alloc)                    */
  
  MUStrDup(&Dst->Env.BaseEnv,Src->Env.BaseEnv);       /* shell environment (alloc,format='<ATTR>=<VAL><ENVRS_ENCODED_CHAR>[<ATTR>=<VAL><ENVRS_ENCODED_CHAR>]...') */
  MUStrDup(&Dst->Env.IncrEnv,Src->Env.IncrEnv);         /* override environment (alloc,format='<ATTR>=<VAL><ENVRS_ENCODED_CHAR>[<ATTR>=<VAL><ENVRS_ENCODED_CHAR>]...') */
  MUStrDup(&Dst->Env.Shell,Src->Env.Shell);             /* execution shell (alloc)                      */

  Dst->Env.UMask = Src->Env.UMask;                      /* default file creation mode mask */

  Dst->Alloc.TC = Src->Alloc.TC;
  Dst->Alloc.NC = Src->Alloc.NC;
  Dst->Request.TC = Src->Request.TC;
  Dst->Request.NC = Src->Request.NC;

  Dst->FloatingNodeCount = Src->FloatingNodeCount;            /**< floating node count (+ TPN for def job) 
                               sum of non-compute nodes */

  Dst->TotalTaskCount = Src->TotalTaskCount;            /**< Total Task Count for SLURM i.e. nodes=4,ttc=11 */

  Dst->SpecDistPolicy = Src->SpecDistPolicy;
  Dst->DistPolicy = Src->DistPolicy;

  MUStrDup(&Dst->Geometry,Src->Geometry);       /**< arbitrary distribution specification (alloc) */

  Dst->SystemQueueTime = Src->SystemQueueTime;   /**< time job was initially eligible to start (only used if EffQueueDuration not specified) */

  Dst->EffQueueDuration = Src->EffQueueDuration;    /**< duration of time job was eligible to run (if -1, use SystemQueueTime) */
  Dst->EffXFactor = Src->EffXFactor;
  Dst->SMinTime = Src->SMinTime;          /**< effective earliest start time            */
  Dst->SpecSMinTime = Src->SpecSMinTime;      /**< user specified earliest start time       */

/*  mulong *SysSMinTime;      < system mandated earliest start time (may be recalculated each iteration) */
                            /**< per partition SysSMinTime (alloc) */
 
  Dst->CMaxDate = Src->CMaxDate;          /**< user specified latest completion date    */
  Dst->CancelReqTime = Src->CancelReqTime;     /**< time last cancel request was sent to rm  */
  Dst->TermTime = Src->TermTime;          /**< time by which job must be terminated     */
  Dst->SubmitTime = Src->SubmitTime;        /**< time job was submitted to RM             */
  Dst->StartTime = Src->StartTime;         /**< time job began most recent execution     */
  Dst->DispatchTime = Src->DispatchTime;      /**< time job was dispatched by RM            */
  Dst->RsvRetryTime = Src->RsvRetryTime;      /**< time job was first started  with MJobStartRsv() */
  Dst->CompletionTime = Src->CompletionTime;    /**< time job execution completed             */
  Dst->CheckpointStartTime = Src->CheckpointStartTime; /**< time checkpoint was started            */
  Dst->HoldTime = Src->HoldTime;          /**< time currently held job was put on hold  */
  Dst->MinPreemptTime = Src->MinPreemptTime;    /**< minimum time job must run before being eligible to be preempted (relative to start) */
  Dst->CompletionCode = Src->CompletionCode;    /**< execution completion code                */
  Dst->StartCount = Src->StartCount;        /**< number of times job was started          */
  Dst->DeferCount = Src->DeferCount;        /**< number of times job was deferred         */
  Dst->CancelCount = Src->CancelCount;       /**< number of times job has been cancelled   */
  Dst->MigrateCount = Src->MigrateCount;      /**< number of times job has been migrated    */
  Dst->PreemptCount = Src->PreemptCount;      /**< number of times job was preempted        */
  Dst->BypassCount = Src->BypassCount;       /**< number of times lower prio job was backfilled */
  Dst->HoldReason = Src->HoldReason;  /**< reason deferred/held             */

  while (MULLIterate(Src->BlockInfo,&SLPtr) == SUCCESS)
    {
    mjblockinfo_t *DstBlockInfoPtr = NULL;
    mjblockinfo_t *SrcBlockInfoPtr = (mjblockinfo_t *)SLPtr->Ptr;

    /* alloc and populate a copy of the current record in the linked list */

    MJobDupBlockInfo(&DstBlockInfoPtr,SrcBlockInfoPtr);

    /* add the new record to the destination job block message linked list */

    if (MJobAddBlockInfo(Dst,&MPar[DstBlockInfoPtr->PIndex],DstBlockInfoPtr) != SUCCESS)
      MJobBlockInfoFree(&DstBlockInfoPtr);
    }

  Dst->MigrateBlockReason = Src->MigrateBlockReason; /**< reason not able to migrate */
  MUStrDup(&Dst->MigrateBlockMsg,Src->MigrateBlockMsg);    /**< (alloc) */
  Dst->DependBlockReason = Src->DependBlockReason; /**< dependency reason */
  MUStrDup(&Dst->DependBlockMsg,Src->DependBlockMsg);    /**< (alloc) */
  MUStrDup(&Dst->Description,Src->Description); /**< job description/comment for templates and service jobs (alloc) */
  MJobDependCopy(Src->Depend,&Dst->Depend);

/*  mmb_t *MB;                < general message buffer (alloc)           */

  Dst->OrigWCLimit = Src->OrigWCLimit;       /**< orig walltime limit (for virtual scaling) */
  Dst->VWCLimit = Src->VWCLimit;          /**< virtual walltime limit (for virtual scaling) */
  Dst->WCLimit = Src->WCLimit;           /**< effective walltime limit                 */
  Dst->CPULimit = Src->CPULimit;          /**< specified CPU limit per job (in proc-seconds) */
  memcpy(&Dst->SpecWCLimit,&Src->SpecWCLimit,sizeof(Src->SpecWCLimit)); /**< requested walltime limit       */
                            /**< NOTE:  unsigned long, does not support -1 */
  Dst->RemainingTime = Src->RemainingTime;     /**< execution time job has remaining         */
  Dst->EstWCTime = Src->EstWCTime;         /**< estimated walltime job will consume      */
  Dst->MinWCLimit = Src->MinWCLimit;
  Dst->OrigMinWCLimit = Src->OrigMinWCLimit;

  Dst->MaxJobMem = Src->MaxJobMem;         /**< job level limits (should replace w/ap_t?) */
  Dst->MaxJobProc = Src->MaxJobProc;        /**< job level limits */

/*  mjest_t *Est;             < estimated start time, execution, and resource utilization (alloc) */

  bmcopy(&Dst->SpecPAL,&Src->SpecPAL);  /**< user specified partition access list - only user can change (bitmap) */
  bmcopy(&Dst->SysPAL,&Src->SysPAL);   /**< feasible partition access list built by scheduler (bitmap) */
  bmcopy(&Dst->PAL,&Src->PAL);      /**< effective job partition access list (bitmap) */
                                 /**< created from SpecPAL & SysPAL when PAL == 0 (or new SpecPAL is given) */
  bmcopy(&Dst->CurrentPAL,&Src->CurrentPAL);     /**< partition access list for current iteration (why is this 0 during UI phase? ) */

  Dst->State = Src->State;  /**< RM job state                            */
  Dst->EState = Src->EState; /**< expected job state due to scheduler action */
  Dst->IState = Src->IState; /**< internal job state                      */

  Dst->SubState = Src->SubState; /**< moab internal job state */

  Dst->CMaxDateIsActive = Src->CMaxDateIsActive;
  Dst->IsInteractive = Src->IsInteractive;    /**< job is interactive                       */
  Dst->SoftOverrunNotified = Src->SoftOverrunNotified;
  Dst->InRetry = Src->InRetry;              /**< job is in job start retry cycle */
  Dst->SystemPrio = Src->SystemPrio;        /**< system admin assigned priority (NOTE: >> MAXPRIO indicates relative) */
  Dst->UPriority = Src->UPriority;        /**< job priority given by user               */
  Dst->CurrentStartPriority = Src->CurrentStartPriority;    /**< priority of job to start                 */
  Dst->RunPriority = Src->RunPriority;      /**< priority of job to continue running      */

  memcpy(&Dst->PStartPriority,&Src->PStartPriority,sizeof(Src->PStartPriority));

  /* copy scheduler features */


  if (Dst->TaskMapSize > 0)
    {
    MUFree((char **)&Dst->TaskMap);
    Dst->TaskMap = (int *)MUMalloc(Src->TaskMapSize);
    memcpy(Dst->TaskMap,Src->TaskMap,Src->TaskMapSize);
    }

  Dst->TaskMapSize = Src->TaskMapSize;      /**< current size of TaskMap not including termination marker (max=MSched.JobMaxTaskCount) */

/*  int  *SID;              < session id array corresponding to taskmap (alloc) */

/*  mnalloc_t *NodeList;      < list of allocated hosts (alloc,size=???) */

  if (!MNLIsEmpty(&Src->ReqHList))
    {
    MNLCopy(&Dst->ReqHList,&Src->ReqHList);
    }

/*  mnalloc_t *ExcHList;      < excluded hosts (alloc)                   */

  Dst->HLIndex = Src->HLIndex;          /**< index of hostlist req                    */

  Dst->ReqHLMode = Src->ReqHLMode; /**< req hostlist mode                */

  Dst->ResFailPolicy = Src->ResFailPolicy; /**< allocated resource failure policy */

  Dst->RType = Src->RType;  /**< reservation subtype (deadline, priority, etc) */
  MUStrDup(&Dst->SubmitHost,Src->SubmitHost);        /**< job submission host (alloc)              */

  MDSCopy(&Dst->DataStage,Src->DataStage);

  bmcopy(&Dst->Flags,&Src->Flags);
  bmcopy(&Dst->SpecFlags,&Src->SpecFlags);
  bmcopy(&Dst->SysFlags,&Src->SysFlags);

  if (bmisset(&Dst->IFlags,mjifTemporaryJob))
    {
    bmcopy(&Dst->IFlags,&Src->IFlags);

    bmset(&Dst->IFlags,mjifTemporaryJob);
    }
  else
    {
    bmcopy(&Dst->IFlags,&Src->IFlags);
    }

  /* the arraymaster flag may have been set when copying flags, so unset it and set normal array flags for array subjobs*/
  if (bmisset(&Dst->Flags,mjfArrayMaster))
    {
    /* set array subjob specific flags */
    bmset(&Dst->IFlags,mjifEnvSpecified);
    bmset(&Dst->IFlags,mjifIsArrayJob);
    bmset(&Dst->Flags,mjfArrayJob);
    bmset(&Dst->SpecFlags,mjfArrayJob);
    bmunset(&Dst->Flags, mjfArrayMaster);
    bmunset(&Dst->SpecFlags, mjfArrayMaster);
    }

  bmcopy(&Dst->AttrBM,&Src->AttrBM);
  MUStrDup(&Dst->MasterHostName,Src->MasterHostName);    /**< name of requested/required master host (alloc) */

  Dst->PSUtilized = Src->PSUtilized;        /**< procseconds utilized by job              */
  Dst->PSDedicated = Src->PSDedicated;       /**< procseconds dedicated to job             */
  Dst->MSUtilized = Src->MSUtilized;
  Dst->MSDedicated = Src->MSDedicated;

  if (!bmisset(&Dst->Flags,mjfArrayJob))         /* don't copy the extension string from the master array job to the subjob */
    {
    MUStrDup(&Dst->RMXString,Src->RMXString);         /**< RM extension string (alloc/opaque)       */
    MUStrDup(&Dst->RMSubmitString,Src->RMSubmitString);    /**< raw command file (alloc)                 */
    }

  Dst->RMSubmitType = Src->RMSubmitType;  /**< RM language that the submitted script was originally written in */
  Dst->ReqRMType = Src->ReqRMType;  /**< required RM submit language (can match type or language) */

  MUStrDup(&Dst->TermSig,Src->TermSig);            /**< signal to send job at terminal time (alloc) */

  if (!bmisset(&Dst->Flags,mjfArrayJob))
    {
    MUStrDup(&Dst->SystemID,Src->SystemID);          /**< identifier of the system owner (alloc)   */
    MUStrDup(&Dst->SystemJID,Src->SystemJID);         /**< external system identifier (alloc)       */
    Dst->SystemRM = Src->SystemRM;          /**< pointer to system owner peer RM */
    }

/*  mdepend_t  *Depend;       < job dependency list (alloc)              */

  Dst->RULVTime = Src->RULVTime;     /**< resource utilization violation time. 
                                 duration job has violated resource limit. 
                                 NOTE:  single dimensional, how to account for multiple metrics
                                      going in and out of violation */ 
                                 
/*  mcal_t     *Cal;          < job calendar (alloc) */

/*  mxml_t    **FSTree;       < pointer to array of size MMAX_PAR (alloc) */

  /* mulong    ExemptList; */     /**< policies against which a job is exempt (bitmap of enum XXX) */
/*  mxload_t *ExtLoad;        < extended load tracking (alloc) */

/*  void     *ASData;         < application simulator data (alloc) */

/*  void     *xd;             < extension data (alloc, for dynamic jobs, struct mdjob_t) */

/*  struct mjarray_t *Array;  < array data (alloc) */

	/* Copy the system job information */

  if (Src->System != NULL)
    {
    Dst->System = (msysjob_t *)MUCalloc(1,sizeof(msysjob_t));
    if (Dst->System != NULL)
      {
      MJobSystemCopy(Dst->System,Src->System);
      }
    }

/*  mjtx_t   *TX;             < job template/profile extensions (alloc) */
/*  mjgrid_t *G;              < grid data (alloc) */

  MUStrDup(&Dst->AttrString,Src->AttrString);     /**< list of attributes specified by job, ie submit args (alloc) */

  MUHTCopy(&Dst->Variables,&Src->Variables);
/*  mln_t    *TStats;         < template stats associated with job (alloc) */
/*  mln_t    *TSets;          < template sets/defs associated with job (alloc) */

  /* update submit flags */
  if (Src->RMSubmitFlags != NULL)
    MUStrDup(&Dst->RMSubmitFlags,Src->RMSubmitFlags);

  MUStrDup(&Dst->PrologueScript,Src->PrologueScript);

  MUStrDup(&Dst->EpilogueScript,Src->EpilogueScript);

  if (Src->NodePriority != NULL)
    {
    MNPrioAlloc(&Dst->NodePriority);
    memcpy(Dst->NodePriority,Src->NodePriority,sizeof(Dst->NodePriority[0]));
    }

  Dst->VMUsagePolicy = Src->VMUsagePolicy;

  MUDynamicAttrCopy(Src->DynAttrs,&Dst->DynAttrs);

  /* NOTE: don't copy Src->R */

  MNLCopy(&Dst->NodeList,&Src->NodeList);

  MNLCopy(&Dst->Req[0]->NodeList,&Src->Req[0]->NodeList);

  return(SUCCESS);
  } /* END int MJobDuplicate() */



/**
 * This function will mark a job as failed (set the completion code to a non-zero value)
 * and will cancel the job. This capability is useful when Moab must assume a job has failed
 * and must internally mark the job as such.
 *
 * NOTE: DANGER - J may be freed!
 *
 * @param J (I) [modified] The job to mark as failed.
 * @param CompletionCode (I)
 */

int MJobFail(

  mjob_t *J,              /* I (modified) */ 
  int     CompletionCode) /* I */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  MJobSetState(J,mjsCompleted);

  J->CompletionCode = CompletionCode;

  MJobCancel(J,NULL,TRUE,NULL,NULL);

  MJobProcessCompleted(&J);

  return(SUCCESS);
  }  /* END MJobFail() */





/**
 * Determine whether job should use TTC and structure job accordingly.
 *
 * NOTE: returns FAILURE when TTC is not specified or TTC should not be applied
 *       returns SUCCESS when TTC is specified and applied
 *
 * @param J (I)
 */

int MJobDetermineTTC(

  mjob_t *J)
  
  {
  mpar_t *P;
  char ParLine[MMAX_LINE];

  if (J->TotalTaskCount <= 0)
    {
    return(FAILURE);
    }

  MDB(7,fSCHED) MLog("INFO:     job %s requested TTC (%d) with partition access list (%s)\n", 
    J->Name,
    J->TotalTaskCount,
    MPALToString(&J->PAL,NULL,ParLine));

  if (J->Req[0]->PtIndex == 0)
    {
    int pindex;

    /* determine first valid partition which can use TTC */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];
  
      if (P->Name[0] == '\0')
        {
        P = NULL;

        break;
        }
  
      if (P->Name[0] == '\1')
        continue;
  
      if (P->ConfigNodes == 0)
        continue;
  
      /* check partition access */
  
      if (!bmisset(&J->PAL,P->Index))
        {
        MDB(7,fSCHED) MLog("INFO:     job %s not allowed to run in partition %s (allowed %s)\n", 
          J->Name,
          P->Name,
          MPALToString(&J->PAL,NULL,ParLine));
  
        continue;
        }
  
      if (bmisset(&P->Flags,mpfUseTTC))
        break;
      }

    if ((P == NULL) || (pindex == MMAX_PAR))
      {
      MDB(7,fSCHED) MLog("INFO:     job %s does not have a partition which supports TTC processing, ignoring TTC\n", 
        J->Name);

      J->TotalTaskCount = 0;

      return(FAILURE);
      }
 
    }  /* END if (J->Req[0]->PtIndex == 0) */
  else
    {
    P = &MPar[J->Req[0]->PtIndex];
    }

  if (!bmisset(&P->Flags,mpfUseTTC))
    {
    MDB(7,fSCHED) MLog("INFO:     job %s does not process TTC for requested partition %s\n", 
      J->Name,
      P->Name);

    return(FAILURE);
    }
 
  /* restructure job according to TTC */

  J->Request.TC = 1;
  J->Req[0]->TaskCount = 1;

  J->Req[0]->DRes.Procs = J->TotalTaskCount;

  MDB(7,fSCHED) MLog("INFO:     job %s set dedicated resource procs to %d (based on partition %s)\n", 
      J->Name,
      J->TotalTaskCount,
      P->Name);

  if (!MNLIsEmpty(&J->Req[0]->NodeList))
    {
    MNLTerminateAtIndex(&J->Req[0]->NodeList,1);
    }

  if (!MNLIsEmpty(&J->NodeList))
    {
    MNLTerminateAtIndex(&J->NodeList,1);
    }

  J->Req[0]->TaskRequestList[0] = 1;
  J->Req[0]->TaskRequestList[1] = 1;

  if (J->TaskMap != NULL)
    J->TaskMap[1] = -1;

  return(SUCCESS);
  }  /* END MJobDetermineTTC() */





/**
 * Update job and node usage for newly loaded/updated jobs.
 *
 * @see design doc DedResDoc
 * @see MRMJobPostUpdate() - parent
 *
 * NOTE:  Update J->Req->XURes
 *               J-.ExtLoad
 *
 * NOTE:  Update N->GMetrics
 *               N->TaskCount
 *               N->EProcCount
 *               N->DRes
 *
 * NOTE:  Updates job network usage.
 *
 * NOTE:  Updates job I/O usage - see MSched.EnableIOLearning
 * NOTE:  Tracks job cpu usage over time - see MSched.ExtendedJobTracking
 *
 * @see MRMJobPostUpdate() - parent
 *
 * @param J (I) [modified]
 */

int MJobUpdateResourceUsage(

  mjob_t *J)  /* I (modified) */

  {
  double TotalNodeCPULoad = 0.0;  /* used for physical node load */
  double TotalNodeIOLoad  = 0.0;  /* used for physical node load */

  int    IOIndex;

  int    nindex;
  int    rqindex;

  mnode_t *N = NULL;
  mreq_t  *RQ;

  mln_t  *GMetrics   = NULL;

  int    gmindex;

  int    NCount;

  int    TC;

  if (J == NULL)
    {
    return(FAILURE);
    }

  IOIndex = MUMAGetIndex(meGRes,"IOLOAD",mVerify);

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(&RQ->NodeList,nindex);

      N->TaskCount  += TC;
      N->EProcCount += TC * RQ->DRes.Procs;

      TotalNodeCPULoad += N->Load;

      if ((IOIndex != 0) && (MSNLGetIndexCount(&N->CRes.GenericRes,IOIndex) != 0))
        {
        TotalNodeIOLoad += (MSNLGetIndexCount(&N->CRes.GenericRes,IOIndex) - MSNLGetIndexCount(&N->ARes.GenericRes,IOIndex));
        }

      if (MSched.AdjustUtlRes == TRUE)
        {
        if ((RQ->URes != NULL) && (N->RM != NULL) && (N->RM->Type == mrmtPBS))
          {
          int tmpInt;

          /* modify per node utilized resources */

          /* IMPORTANT NOTE:  only works if node is dedicated! */

          tmpInt = MAX(0,N->ARes.Swap - RQ->URes->Swap);
          MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);
          }
        }

      /* NOTE:  even if node accesspolicy is dedicated blocking resources
                from use by other jobs, adjust node dedicated resources only
                by actual job dedicated usage */

      /* NOTE: sync w/MCResAdd() in MRMJobPostLoad() */

      if (bmisset(&J->Flags,mjfMeta))
        {
        /* META jobs don't use resources on the node, so don't update
           anything on the node.  NOTE: this may be the wrong behavior
           for VMs, don't have enough info yet */

        /* NO-OP */
        }
      else if ((J->System != NULL) &&
                (J->System->JobType == msjtGeneric) &&
                (MVMJobIsVMMigrate(J) == TRUE))
        {
        mvm_t *V;

        if (MVMFind(J->System->VM,&V) == SUCCESS)
          MCResAdd(&N->DRes,&N->CRes,&V->CRes,1,FALSE);
        }
      else if (MJobWantsSharedMem(J) == TRUE)
        {
        enum MNodeAccessPolicyEnum NAPolicy;

        /* Shared jobs have to fit on one node, so remove dedicated memory,
         * singlejob job's span nodes and use all resources. */
      
        MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);

        if (NAPolicy == mnacSingleJob)
          {
          /* Show that the whole node is allocated. */

          MCResAdd(&N->DRes,&N->CRes,&N->CRes,1,FALSE);
          }
        else
          {
          mcres_t tmpDRes;

          MCResInit(&tmpDRes);

          MCResCopy(&tmpDRes,&RQ->DRes);

          /* protect against divide by 0 */

          if (RQ->TaskCount != 0)
            {
            tmpDRes.Mem = RQ->ReqMem / RQ->TaskCount;
  
            MCResAdd(&N->DRes,&N->CRes,&tmpDRes,TC,FALSE);
            }
          else
            {
            MDB(1,fSCHED) MLog("ALERT:    unexpected req taskcount of 0.\n");
            }

          MCResFree(&tmpDRes);
          }
        }
      else if (((N->RM == NULL) ||
                (N->RM->Type != mrmtMoab)) &&
               ((RQ->NAccessPolicy == mnacSingleJob) ||
                (RQ->NAccessPolicy == mnacSingleTask)))
        {
        MCResAdd(&N->DRes,&N->CRes,&RQ->DRes,TC,FALSE);
        }
      else
        {
        MCResAdd(&N->DRes,&N->CRes,&RQ->DRes,TC,FALSE);
        }

      /* update req network usage */

      if (N->XLoad != NULL)
        {
        if (RQ->XURes == NULL)
          {
          MReqInitXLoad(RQ);
          }

        if (RQ->XURes != NULL)
          {
          for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
            {
            if ((N->XLoad->GMetric[gmindex] == NULL) ||
                (N->XLoad->GMetric[gmindex]->IntervalLoad == MDEF_GMETRIC_VALUE))
              {
              /* GMetric not specified for allocated node */

              continue;
              }

            if (RQ->XURes->GMetric[gmindex]->IntervalLoad == MDEF_GMETRIC_VALUE)
              RQ->XURes->GMetric[gmindex]->IntervalLoad = 0;

            RQ->XURes->GMetric[gmindex]->IntervalLoad += N->XLoad->GMetric[gmindex]->IntervalLoad;
            }  /* END for (gmindex) */
          }    /* END if (RQ->XURes != NULL) */
        }      /* END if (N->XLoad != NULL) */
      }        /* END for (nindex) */

    NCount = nindex;

    if (RQ->XURes != NULL)
      {
      /* calculate iteration network usage */

      double NodeLoad;
      double JobLoad;

      for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
        {
        JobLoad = RQ->XURes->GMetric[gmindex]->IntervalLoad;

        if (JobLoad == MDEF_GMETRIC_VALUE)
          continue;

        NodeLoad = JobLoad / NCount;

        if (NodeLoad != RQ->XURes->GMetric[gmindex]->IntervalLoad)
          {
          RQ->XURes->GMetric[gmindex]->IntervalLoad = NodeLoad;
          RQ->XURes->GMetric[gmindex]->SampleCount++;
          }

        /* if NodeLoad is set and value is less than RQMin or RQMin is
           uninitialized, set RQMin */

        if ((NodeLoad > 0.0) &&
            ((RQ->XURes->GMetric[gmindex]->Min <= 0.0) || (NodeLoad < RQ->XURes->GMetric[gmindex]->Min)))
          {
          RQ->XURes->GMetric[gmindex]->Min = NodeLoad;
          }

        /* Set the max */

        if ((NodeLoad > 0.0) && (NodeLoad > RQ->XURes->GMetric[gmindex]->Max))
          {
          RQ->XURes->GMetric[gmindex]->Max = NodeLoad;
          }

        RQ->XURes->GMetric[gmindex]->Avg +=
          (RQ->XURes->GMetric[gmindex]->IntervalLoad * MSched.Interval) / NCount;

        if (RQ->XURes->GMetric[gmindex] != NULL)
          {
          RQ->XURes->GMetric[gmindex]->TotalLoad    += (NodeLoad * (MSched.Interval/100));
          RQ->XURes->GMetric[gmindex]->IntervalLoad  = NodeLoad;
          }
        }  /* END for (gmindex) */
      }  /* END if (RQ->XURes != NULL) */

    MULLFree(&GMetrics,MUFREE);
    }    /* END for (rqindex) */

  if (MSched.ExtendedJobTracking == TRUE)
    {
    /* use physical load */
    MJobIncrExtLoad(J,MSched.CPUGMetricIndex,TotalNodeCPULoad);
    }  /* END if (MSched.ExtendedJobTracking == TRUE) */

  return(SUCCESS);
  }  /* END MJobUpdateResourceUsage */



/**
 * Asks slurm through willrun if job is runnable and what nodes the job can run on.
 *
 * @param J          (I)
 * @param FNL        (I/O)
 * @param Preemptees (I/O)
 * @param StartTime  (I)
 * @param ReturnTime (I) [optional]  Time that slurm reported.
 * @param EMsg       (O)
 */

mbool_t MJobWillRun(

  mjob_t      *J,
  mnl_t       *FNL,
  mjob_t     **Preemptees,
  mulong       StartTime,
  mulong      *ReturnTime,
  char        *EMsg)

  {
  char *Response = NULL;

  mjwrs_t JW;

  mrm_t *R;

  mnode_t *N;

  const char *FName = "MJobWillRun";

  MDB(5,fCONFIG) MLog("%s(%s,%s,%ld,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    "FNL",
    StartTime,
    "EMsg");

  if ((J == NULL) || (FNL == NULL))
    {
    return(FALSE);
    }

  J->WillrunStartTime = 0;

  if (MNLIsEmpty(FNL))
    {
    MDB(6,fSTRUCT) MLog("ERROR:     empty feasible node list\n");

    return(FALSE);
    }

  MNLGetNodeAtIndex(FNL,0,&N);

  R = N->RM;

  if (R->IsBluegene == FALSE)
    return(TRUE);

  MNLInit(&JW.NL);

  MNLCopy(&JW.NL,FNL);

  JW.J  = J;
  JW.StartDate = StartTime;
  JW.Preemptees = NULL;

  if (Preemptees != NULL)
    JW.Preemptees = Preemptees;

#ifndef MUNITTEST
  if (MRMSystemQuery(
      R,                    /* (I) RM */
      "willrun",            /* (I) QueryType */
      (char *)&JW,          /* (I) Attribute */
      TRUE,                 /* (I) IsBlocking */
      (char *)&Response,    /* (O) Value */
      NULL,                 /* (O) EMsg [output] */
      NULL) == FAILURE)
    {
    MDB(1,fSCHED) MLog("MRMSystemQuery (willrun) FAILED\n");

    return(FAILURE);
    }
#else

  Response = FakeWillRun(&JW);

#endif


  MDB(8,fWIKI)
    {
    mstring_t HostList(MMAX_BUFFER);

    MUNLToRangeString(FNL,NULL,0,FALSE,TRUE,&HostList);
  
    MLog("INFO:     WILL RUN Response: Job '%s' on hosts '%s' at time '%ld'  Response -> %s\n",
      J->Name,
      HostList.c_str(),
      StartTime,
      Response);
    }

  if (MSLURMParseWillRunResponse(
        &JW,
        R,
        Response,
        StartTime,
        EMsg) == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot parse job string for job %s through RM\n",
      J->Name);

    MUFree(&Response);

    return(FAILURE);
    }

  MUFree(&Response);
  
  J->WillrunStartTime = JW.StartDate;

  if (JW.StartDate > StartTime)
    {
    MDB(7,fWIKI) MLog("WARNING:  unexpected startdate returned by willrun (%ld > %ld)\n",JW.StartDate,StartTime);

    return(FAILURE);
    }

  /* Use subset of tasks that are approved by willrun */ 
  
  MNLCopy(FNL,&JW.NL);

  MNLFree(&JW.NL);

  if (ReturnTime != NULL)
    *ReturnTime = JW.StartDate;

  return(TRUE);
  }  /* END MJobWillRun() */




/**
 * Transfer a job's resource allocation to another job.
 *
 * @see MJobStart() - parent
 * NOTE: used to start a systemjob in the place of a real job
 *
 * @param DstJ (O)
 * @param SrcJ (I)
 */

int MJobTransferAllocation(

  mjob_t *DstJ,
  mjob_t *SrcJ)

  {
  int rqindex;

  /* move the job's allocated node list */

  MNLCopy(&DstJ->NodeList,&SrcJ->NodeList);

/*
  SrcJ->NodeList[0].N = NULL;
*/

  for (rqindex = 0;SrcJ->Req[rqindex] != NULL;rqindex++)
    {
    if (DstJ->Req[rqindex] == NULL)
      {
      if (MReqCreate(DstJ,NULL,NULL,FALSE) == FAILURE)
        {
        return(FAILURE);
        }
      }

    MNLCopy(&DstJ->Req[rqindex]->NodeList,&SrcJ->Req[rqindex]->NodeList);

    DstJ->Req[rqindex]->TaskRequestList[0] = SrcJ->Req[rqindex]->TaskRequestList[0];
    DstJ->Req[rqindex]->TaskRequestList[1] = SrcJ->Req[rqindex]->TaskRequestList[1];
 
    DstJ->Req[rqindex]->TaskCount = SrcJ->Req[rqindex]->TaskCount;
    DstJ->Req[rqindex]->NodeCount = SrcJ->Req[rqindex]->NodeCount;
    }  /* END for (rqindex) */

  DstJ->Request.TC = SrcJ->Request.TC;
  DstJ->Request.NC = SrcJ->Request.NC;

  return(SUCCESS);
  }  /* END MJobTransferAllocation() */





/**
 * Transform J based on P.
 *
 * NOTE: Set   attributes if "TransformNew == TRUE"
 *       Unset attributes if "TransformNew == FALSE" (where possible) 
 *
 * Assumes MJobSTransform will called with TransformNew set to FALSE shortly
 * after it is called with MJobSTransform set to TRUE.
 *
 * @param J (I/O) [modified]
 * @param P (I) 
 * @param SIndex (I) SMPNode feature index
 * @param TransformNew (I) If FALSE undo the change
 */

int MJobPTransform(

  mjob_t       *J,
  const mpar_t *P,
  int           SIndex,
  mbool_t       TransformNew)

  {
  int         rqindex;

  mreq_t     *RQ;
  msmpnode_t *SN;

  int         JTC = 0;

  /* if P->RM is altix then transform memory requirement */
  /* if TransformNew == FALSE then undo the change */

  if (J == NULL)
    {
    MDB(1,fSTRUCT) MLog("ERROR:  J was passed as NULL\n");

    return(FAILURE);
    }

  /* Right now, the job is only transformed for PBS + Altix scheduling */

  if (bmisset(&P->Flags,mpfSharedMem) && 
      (MPar[0].NodeSetPriorityType != mrspMinLoss))
    return(SUCCESS);

  if (bmisset(&P->Flags,mpfSharedMem) && 
      (MJobWantsSharedMem(J) == FALSE))
    return(SUCCESS);

  /* find representative node */

  if (SIndex > 0)
    {
    if ((SN = MSMPNodeFindByFeature(SIndex)) == NULL)
      {
      MDB(3,fSTRUCT) MLog("WARNING:  Unable to find an SMP with feature %s\n",
        (MAList[meNFeature][SIndex] != NULL) ? MAList[meNFeature][SIndex] : "NULL");
 
      return(FAILURE);
      }
    }
  else
    {
    if ((SN = MSMPNodeFindByPartition(P->Index)) == NULL)
      {
      MDB(3,fSTRUCT) MLog("WARNING:  Unable to find an SMP in partition %s\n",
        P->Name);
 
      return(FAILURE);
      }
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (TransformNew == TRUE)
      {
      /* Transform the job's taskcount into the number of procs needed by the
       * job to fullfill the proc and memory requirements. Because MJobGetRange
       * doesn't look at ReqMem to make sure enough nodes are available. */
   
      enum MNodeAccessPolicyEnum NAPolicy;
   
      RQ->OldReqTC = RQ->TaskCount;
   
      MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);
   
      if (NAPolicy == mnacSingleJob)
        {
/*
        RQ->TaskCount = MReqGetNBCount(J,RQ,SN->N) * SN->N->CRes.Procs;
*/
        RQ->TaskCount = MReqGetNBCount(J,RQ,SN->N);
        JTC += RQ->TaskCount;
        }
      }
    else
      {
      RQ->TaskCount = RQ->OldReqTC;
      JTC += RQ->OldReqTC;
      }
    }  /* END for (rqindex) */

  J->Request.TC = JTC;

  return(SUCCESS);
  }  /* END MJobPTransform() */




/**
 * Check whether J1 and J2 match according to specified criteria.
 *
 * @param J1 (I)
 * @param J2 (I)
 */
 
mbool_t MJobIsMatch(

  mjob_t *J1,  /* I */
  mjob_t *J2)  /* I */
  
  {
  int jindex;
  int rindex;

  const enum MJobAttrEnum JAList[] = {
    mjaUser,
    mjaGroup,
    mjaClass,
    mjaAccount,
    mjaQOS,
    mjaAWDuration,
    mjaNONE };

  const enum MReqAttrEnum RQList[] = {
    mrqaTCReqMin,
    mrqaNONE };

  for (jindex = 0;JAList[jindex] != mjaNONE;jindex++)
    {
    switch (JAList[jindex])
      {
      case mjaUser:

        if (J1->Credential.U != J2->Credential.U)
          return(FALSE);

        break;

      case mjaGroup:

        if (J1->Credential.G != J2->Credential.G)
          return(FALSE);

        break;

      case mjaClass:

        if (J1->Credential.C != J2->Credential.C)
          return(FALSE);

        break;

      case mjaAccount:

        if (J1->Credential.A != J2->Credential.A)
          return(FALSE);

        break;

      case mjaQOS:

        if (J1->Credential.Q != J2->Credential.Q)
          return(FALSE);

        break;
 
      case mjaAWDuration:

        if (J1->SpecWCLimit[0] != J2->SpecWCLimit[0])
          return(FALSE);

        break;

      default:

        break;
      } /* END switch (JAList[jindex]) */
    }   /* END for (jindex) */

  for (rindex = 0;RQList[rindex] != mrqaNONE;rindex++)
    {
    switch (RQList[rindex])
      {
      case mrqaTCReqMin:

        if (J1->Req[0]->TaskCount != J2->Req[0]->TaskCount)
          return(FALSE);

        if (J1->Req[0]->NodeCount != J2->Req[0]->NodeCount)
          return(FALSE);

        if (J1->Req[0]->DRes.Procs != J2->Req[0]->DRes.Procs)
          return(FALSE);

        break;

      default:

        break; 
      } /* END switch (RQList[rindex]) */
    }   /* END for (rindex) */

  return(TRUE);
  }  /* END MJobIsMatch() */


/**
 *  If the job is not already running its epilog launch it now.
 *     Set appropriate flags to indicate that it is running.
 *     (J->SubState = mjsstEpilog;)
 *  If it is already running check for done-ness.
 *     Set appropriate flags when done.
 *     (J->SubState = mjsstNONE;)
 *
 *  As a side-effect, J->Status is set to mjsRunning when the 
 *  job is running the epilog
 *  When the epilog finishes running, J->Status is left alone. 
 *
 * @param J (I)
 * @param Status (O) [optional] Tracks the value we set J->Status.
 */

int MJobDoEpilog(

  mjob_t             *J,
  enum MJobStateEnum *Status)

  {
  if ((!bmisset(&J->IFlags,mjifRanEpilog)) &&
      (J->SubState != mjsstEpilog))
    {
    /* job is not currently in epilog -- put it there */
  
    MJobLaunchEpilog(J);
  
    J->SubState = mjsstEpilog;
  
    bmset(&J->IFlags,mjifRanEpilog);
    }
  else
    {
    mtrig_t *T;

    MJobCheckEpilog(J);
  
    T = (mtrig_t *)MUArrayListGetPtr(J->EpilogTList,0);

    if (!MTRIGISRUNNING(T))
      {
      J->SubState = mjsstNONE;
      }
    else
      {
      J->SubState = mjsstEpilog;
      }
    }

    if (J->SubState == mjsstEpilog)
      {
      /* force the job to stay out of "completed" while epilog is running */
      J->State = mjsRunning;
      if (Status != NULL)
        *Status = mjsRunning;
      }
  

  return(SUCCESS);
  }   /* END MJobDoEpilog() */


int MJobCheckEpilog(

  mjob_t *J)

  {
  if ((J->EpilogTList == NULL) ||
      (J->EpilogTList->NumItems <= 0))
    {
    return(SUCCESS);
    }

  MDB(3,fSTRUCT) MLog("INFO:     checking epilog status for job %s\n",
    J->Name);

  MSchedCheckTriggers(J->EpilogTList,-1,NULL);

  return(SUCCESS);
  }   /* END MJobCheckEpilog() */




/**
 * Launch the job epilog, no matter what.
 *
 */

int MJobLaunchEpilog(

  mjob_t *J)

  {
  mtrig_t *T;

  if ((J->EpilogTList == NULL) ||
      (J->EpilogTList->NumItems <= 0))
    {
    return(SUCCESS);
    }

  MDB(5,fSTRUCT) MLog("INFO:     launching epilog for job %s\n",
    J->Name);

  /* check if epilog is already running */

  T = (mtrig_t *)MUArrayListGetPtr(J->EpilogTList,0);

  MTrigReset(T);

  T->ETime = MSched.Time;

  MSchedCheckTriggers(J->EpilogTList,-1,NULL);

  return(SUCCESS);
  }  /* END MJobLaunchEpilog() */






/**
 * Check whether job has finished with its prolog(s).
 *
 * @see MJobStart() - parent
 * @see MPBSJobUpdate() - parent
 *
 * @param J (I)
 * @param State (O) [optional] used to mask real state of job
 */

int MJobCheckProlog(

  mjob_t             *J,
  enum MJobStateEnum *State)

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  J->SubState = mjsstProlog; 

  if (J->PreemptTime + ((J->DestinationRM != NULL) ? J->DestinationRM->PreemptDelay : 0) > MSched.Time)
    {
    if (State != NULL)
      *State = mjsRunning;

    return(SUCCESS);
    }
 
  if (J->PrologTList == NULL)
    {
    /* start job here and now */

    MRMJobStart(J,NULL,NULL);

    J->State = mjsRunning;
    J->SubState = mjsstNONE;

    return(SUCCESS);
    }

  bmset(&J->IFlags,mjifRanProlog);
  bmunset(&J->IFlags,mjifRanEpilog);

  MSchedCheckTriggers(J->PrologTList,-1,NULL);

  if (J->SubState == mjsstProlog)
    MSchedCheckTriggers(J->PrologTList,-1,NULL);

  if ((J->SubState == mjsstProlog) && (State != NULL))
    {
    *State = mjsRunning;
    }  
  else if ((J->State == mjsRunning) && (State != NULL))
    {
    *State = mjsRunning;
    }

  return(SUCCESS);
  }  /* END MJobCheckProlog() */






/**
 * Flatten a workflow graph rooted at J, adding each item to Jobs. This function
 * handles problems with duplicates and cycles that can exist in graphs.
 *
 * @param Jobs (I/O) Initialized hash table to put jobs into. Jobs will be placed using their Name 
 *                   attribute as the key
 * @param J    (I)   Root job
 */

int MJobWorkflowToHash(

  mhash_t *Jobs, /* (I/O) Initialized hash table to put jobs into. Jobs will be placed using their Name attribute as the key */
  mjob_t  *J)    /* (I)   Root job */

  {
  mjob_t *JChild = NULL;

  job_iter JTI;
  job_iter CJTI;

  if (MUHTGet(Jobs,J->Name,NULL,NULL) == SUCCESS)
    {
    /* duplicate detected. Terminate here */

    return(SUCCESS);
    }

  if (MUHTAdd(Jobs,J->Name,J,NULL,NULL) == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ERROR:    unexpected HT error canceling workflow.\n");

    return(FAILURE);
    }

  /* add all children */

  MJobIterInit(&JTI);
  MCJobIterInit(&CJTI);

  while (MJobFindDependent(J,&JChild,&JTI,&CJTI) == SUCCESS)
    {
    MJobWorkflowToHash(Jobs,JChild);
    }

  return(SUCCESS);
  }  /* END MJobWorkflowToHash() */





/**
 * Return SUCCESS if jobs are same.
 *
 * @param J1
 * @param J2
 */

int MJobCmp(

  mjob_t *J1,
  mjob_t *J2)

  {
  int rqindex;

  if ((J1->JGroup != NULL) && (J2->JGroup != NULL) &&
      (!strcmp(J1->JGroup->Name,J2->JGroup->Name)))
    {
    /* jobs are in the same job group, must be the same */

    return(SUCCESS);
    }

  if (MSched.EnableHighThroughput == FALSE)
    return(FAILURE);

  /* the rest of this is only enabled for highthroughput */

  if (J1->Request.TC != J2->Request.TC)
    return(FAILURE);

  if (J1->Request.NC != J2->Request.NC)
    return(FAILURE);

  if (J1->SpecWCLimit[0] != J2->SpecWCLimit[0])
    return(FAILURE);

  for (rqindex = 0;J1->Req[rqindex] != NULL;rqindex++)
    {
    if (J2->Req[rqindex] == NULL)
      return(FAILURE);

    if (MReqCmp(J1->Req[rqindex],J2->Req[rqindex]) == FAILURE)
      return(FAILURE);
    }

  if (J2->Req[rqindex] != NULL)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MJobCmp() */



/**
 * Parse LL style geometry into Moab's multi-req structures.
 *
 * FORMAT:  {(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63)(64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93)}
 *           ^---this becomes a 2 req job, 1x64,1x30
 *          {(0,1) (3) (5,4) (2)}
 *           ^---this becomes a 2 req job, 2x2,2x1
 *
 * @param J
 * @param Geometry 
 */

int MJobParseGeometry(

  mjob_t     *J,
  const char *Geometry)

  {
  int  ReqTaskCount[MMAX_REQ_PER_JOB + 1]; /* space for -1 terminator */
  int  ReqNodeCount[MMAX_REQ_PER_JOB];

  char *ptr;
  char *TokPtr = NULL;

  char *ReqPtr;
  char *ReqTokPtr = NULL;

  char *String = NULL;

  int   TCount;
  int   tindex;

  if (strchr(Geometry,'{') == NULL)
    {
    /* only parse LL style geometry */

    return(FAILURE);
    }

  MUStrDup(&String,Geometry);

  /* parse off {} */

  ptr = MUStrTok(String,"{}",&TokPtr);

  if (ptr == NULL)
    {
    MUFree(&String);

    return(FAILURE);
    }

  ReqTaskCount[0] = -1;

  ptr = MUStrTok(ptr,"() ",&TokPtr);

  while (ptr != NULL)
    {
    ReqPtr = MUStrTok(ptr,", ",&ReqTokPtr);

    TCount = 0;

    while (ReqPtr != NULL)
      {
      TCount++;

      ReqPtr = MUStrTok(NULL,", ",&ReqTokPtr);
      }  /* END while (ReqPtr != NULL) */

    for (tindex = 0;ReqTaskCount[tindex] != -1;tindex++)
      {
      if (TCount != ReqTaskCount[tindex])
        continue;

      ReqNodeCount[tindex]++;

      break;
      }  /* END for (tindex) */

    if (tindex >= MMAX_REQ_PER_JOB)
      {
      MUFree(&String);

      /* add message to job and hold */

      MDB(1,fSCHED) MLog("ALERT:    job '%s' requests too many geometries\n",
        J->Name);
      
      MJobSetHold(J,mhBatch,MMAX_TIME,mhrSystemLimits,"unsupported geometry request");

      return(FAILURE);
      }
    else if (ReqTaskCount[tindex] == -1)
      {
      ReqNodeCount[tindex] = 1;
      ReqTaskCount[tindex] = TCount;
      ReqTaskCount[tindex + 1] = -1;
      }

    ptr = MUStrTok(NULL,"() ",&TokPtr);
    }    /* END while (ptr != NULL) */

  if (J != NULL)
    {
    J->Request.TC = 0;
    J->Request.NC = 0;

    for (tindex = 0;ReqTaskCount[tindex] != -1;tindex++)
      {
      if (J->Req[tindex] == NULL)
        {
        MReqCreate(J,NULL,NULL,FALSE);
        }

      if (tindex > 0)
        {
        J->Req[tindex]->RMIndex = J->Req[0]->RMIndex;
        J->Req[tindex]->DRes.Procs = J->Req[0]->DRes.Procs;
        }

      J->Req[tindex]->TaskCount    = ReqTaskCount[tindex] * ReqNodeCount[tindex];
      J->Req[tindex]->TaskRequestList[0] = J->Req[tindex]->TaskCount;
      J->Req[tindex]->TaskRequestList[1] = J->Req[tindex]->TaskCount;
      J->Req[tindex]->NodeCount    = ReqNodeCount[tindex];
      J->Req[tindex]->TasksPerNode = ReqTaskCount[tindex];

      J->Request.TC += J->Req[tindex]->TaskCount;
      J->Request.NC += ReqNodeCount[tindex];

      bmset(&J->IFlags,mjifExactTPN);
      }
    }   /* END if (J != NULL) */

  MUFree(&String);

  return(SUCCESS);
  }  /* END MJobParseGeometry() */




int MJobParseBlocking(

  mjob_t *J)

  {
  if ((J->Request.TC <= 0) ||
      (J->Req[0]->BlockingFactor <= 0) ||
      ((J->Request.TC % J->Req[0]->BlockingFactor) == 0))
    {
    return(SUCCESS);
    }

  if (J->Req[1] == NULL)
    {
    MReqCreate(J,NULL,NULL,FALSE);
    }

  J->Req[0]->NodeCount = J->Request.TC / J->Req[0]->BlockingFactor;
  J->Req[0]->TaskCount = (J->Req[0]->NodeCount * J->Req[0]->BlockingFactor);
  J->Req[0]->TaskRequestList[0] = J->Req[0]->TaskCount;
  J->Req[0]->TaskRequestList[1] = J->Req[0]->TaskCount;
  J->Req[0]->TasksPerNode = J->Req[0]->BlockingFactor;

  J->Req[1]->RMIndex = J->Req[0]->RMIndex;
  J->Req[1]->DRes.Procs = J->Req[0]->DRes.Procs;

  J->Req[1]->TaskCount = J->Request.TC - J->Req[0]->TaskCount;
  J->Req[1]->NodeCount = 1;
  J->Req[1]->TaskRequestList[0] = J->Req[1]->TaskCount;
  J->Req[1]->TaskRequestList[1] = J->Req[1]->TaskCount;
  J->Req[1]->TasksPerNode = J->Req[1]->TaskCount;

  J->Request.TC = J->Req[0]->TaskCount + J->Req[1]->TaskCount;
  J->Request.NC = J->Req[0]->NodeCount + J->Req[1]->NodeCount;

  bmset(&J->IFlags,mjifExactTPN);

  return(SUCCESS);
  }  /* END MJobParseBlocking() */




/** 
 * Returns TRUE if job has multiple compute reqs.
 *
 * @param J
 */

mbool_t MJobIsMultiReq(

  mjob_t *J)

  {
  if (J->Req[1] == NULL)
    return(FALSE);

  if (J->Req[1]->DRes.Procs == 0)
    return(FALSE);

  return(TRUE);
  }




/**
 * Populate req nodelists based on global job nodelist (BAD).
 *
 * update per req Task and Node counts
 *
 * J->NodeList[] should be ordered as <req0nodelist>[<req1nodelist>]...
 *
 * NOTE:  must handle overlapping reqs, ie, req 1 and req 2 both
 *        allocate node X (NYI)
 *        will require checkpointing of allocated req level nodelists
 *
 * sanity check req nodelist, taskcount, and nodecount
 * @see MRMJobPostLoad - parent
 * @see MRMJobPostUpdate - parent
 *
 * @param J
 * @param TCHasChanged
 */

int MJobPopulateReqNodeLists(

  mjob_t  *J,
  mbool_t *TCHasChanged)

  {
  mreq_t *RQ;
  mreq_t *NextRQ;

  int rqindex;
  int nindex;
  int SNIndex = 0;

  int tmpNC;
  int tmpTC;

  mnode_t *N1;
  mnode_t *N2;

  *TCHasChanged = FALSE;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ     = J->Req[rqindex];
    NextRQ = J->Req[rqindex + 1];

    tmpNC = 0;
    tmpTC = 0;

    if (MNLGetNodeAtIndex(&RQ->NodeList,0,&N1) == SUCCESS)
      {
      /* req information is up to date */

      /* verify J->NodeList does not include RQ->NodeList nodes */

      if (MNLReturnNodeAtIndex(&J->NodeList,SNIndex) != N1) 
        continue;
      }

    for (nindex = SNIndex;MNLGetNodeAtIndex(&J->NodeList,nindex,&N1) == SUCCESS;nindex++)
      {
      if (MNLGetTCAtIndex(&J->NodeList,nindex) <= 0)
        {
        /* end of job nodelist reached */

        break;
        }

      if (NextRQ != NULL)
        {
        /* NOTE:  newly started multi-req jobs will not have 
                  RQ->NodeList[0].N populated */

        if (MNLGetNodeAtIndex(&NextRQ->NodeList,0,&N2) == SUCCESS)
          {
          /* NOTE:  on X1E, nodes may be re-ordered at start up so
                    J->NodeList may be out of order from RQ->NodeList */

          if (N1 == N2)
            {
            /* start of next req reached, terminate current req list */

            break;
            }

          if ((RQ->DRes.Procs == 0) && 
              (N1->CRes.Procs != 0))
            {
            /* transition from local virtual resource req to compute req */

            break;
            }
          }
        else if (tmpTC >= RQ->TaskCount)
          {
          /* start of next req reached, terminate current req list */

          break;
          }
        }    /* END if (NextRQ != NULL) */

      tmpNC++;

      if (N1 == MSched.GN)
        {
        /* increase req node count for non-global nodes */

        /* NOTE:  must handle multi-allocation of global node (NYI) */

        J->FloatingNodeCount = 1;
        }

      /* copy nodelist entry to req nodelist */

      tmpTC += MNLGetTCAtIndex(&J->NodeList,nindex);

      MNLSetNodeAtIndex(&RQ->NodeList,nindex - SNIndex,N1);
      MNLSetTCAtIndex(&RQ->NodeList,nindex - SNIndex,MNLGetTCAtIndex(&J->NodeList,nindex));
      }  /* END for (nindex) */

    MNLTerminateAtIndex(&RQ->NodeList,nindex - SNIndex);

    if ((tmpTC != RQ->TaskCount) && (!bmisset(&J->IFlags,mjifPBSNCPUSSpecified)))
      {
      /* if it doesn't match TaskCount or DRes.Procs then just stick it in TaskCount */
      *TCHasChanged = TRUE;

      RQ->TaskCount = tmpTC;
      RQ->TaskRequestList[0] = tmpTC;
      }

    if (tmpNC != RQ->NodeCount)
      {
      *TCHasChanged = TRUE;

      RQ->NodeCount = tmpNC;
      }

    SNIndex = nindex;
    }  /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MJobPopulateReqNodeLists() */





mbool_t MJobLangIsSupported(

  mbitmap_t       *SupportedLangBM,  /* I (bitmap of MRMTypeEnum) */
  enum MRMTypeEnum RequiredLang)     /* I */
    
  {
  /* check if language is explicitly requested */

  if (bmisset(SupportedLangBM,RequiredLang))
    {
    return(TRUE);
    }

  /* check if satisfactory dialects exist */

  if (RequiredLang == mrmtPBS)
    {
    if (bmisset(SupportedLangBM,mrmtNative))
      {
      /* NOTE:  RM subtype info not routed in (FIXME) */

      /* assume native:xt4 */

      return(TRUE);
      }
    }

  /* process other dialects */

  /* NYI */

  return(FALSE);
  }  /* MJobLangIsSupported() */








/**
 * Parse job generic metric requirements.
 *
 * FORMAT:  <NAME><CMP><VAL> or
 *          <NAME>:<CMP>:<VAL> 
 */

int MJobAddGMReq(

  mjob_t *J,
  mreq_t *RQ,
  char   *GMReqString)

  {
  char tmpGMName[MMAX_NAME];
  char tmpGMVal[MMAX_NAME];

  enum MCompEnum tmpI;

  int  GMIndex;

  mln_t *gmptr;

  if (strchr(GMReqString,':') != NULL)
    {
    char *ptr;
    char *TokPtr = NULL;

    const enum MCompEnum MAMOTable[] = {
      mcmpNONE,
      mcmpLT,
      mcmpLE,
      mcmpEQ,
      mcmpGE,
      mcmpGT,
      mcmpNE,
      mcmpLAST };

    ptr = MUStrTok(GMReqString,".:",&TokPtr);

    if (ptr != NULL)
      MUStrCpy(tmpGMName,ptr,sizeof(tmpGMName));
    else
      tmpGMName[0] = '\0';

    ptr = MUStrTok(NULL,".:",&TokPtr);

    if (ptr != NULL)
      tmpI = MAMOTable[MUGetIndexCI(ptr,MAMOCmp,FALSE,0)];
    else
      tmpI = mcmpNONE;

    ptr = MUStrTok(NULL,":",&TokPtr);

    if (ptr != NULL)
      MUStrCpy(tmpGMVal,ptr,sizeof(tmpGMVal));
    else
      tmpGMVal[0] = '\0';
    }
  else if (MUParseComp(
            GMReqString,
            tmpGMName,
            &tmpI,
            tmpGMVal,
            sizeof(tmpGMVal),
            TRUE) == FAILURE)
    {
    /* FAILURE - cannot parse expression */

    return(FAILURE);
    }

  if ((GMIndex = MUMAGetIndex(meGMetrics,tmpGMName,mAdd)) <= 0)
    {
    /* FAILURE - cannot locate requested generic metric */

    return(FAILURE);
    }

  if ((RQ->OptReq == NULL) &&
     ((RQ->OptReq = (moptreq_t *)MUCalloc(1,sizeof(moptreq_t))) == NULL))
    {
    /* FAILURE - cannot alloc parent optreq structure */

    return(FAILURE);
    }

  if (MULLAdd(&RQ->OptReq->GMReq,MGMetric.Name[GMIndex],NULL,&gmptr,MUFREE) == SUCCESS)
    {
    mgmreq_t *G;

    G = (mgmreq_t *)gmptr->Ptr;

    if (G == NULL)
      {
      if ((G = (mgmreq_t *)MUCalloc(1,sizeof(mgmreq_t))) == NULL)
        {
        /* cannot alloc needed memory */

        return(FAILURE);
        }

      gmptr->Ptr = (void *)G;
      }

    /* NOTE:  only support one comparison per generic metric */

    G->GMIndex = GMIndex;
    G->GMComp = (enum MCompEnum)tmpI;

    MUStrCpy(G->GMVal,tmpGMVal,sizeof(G->GMVal));
    }

  return(SUCCESS);
  }  /* END MJobAddGMReq() */




mbool_t MJobIsMultiRM(

  mjob_t *J)  /* I */

  {
  int rqindex;

  for (rqindex = 1;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    if (J->Req[rqindex]->RMIndex != J->Req[0]->RMIndex)
      {
      return(TRUE);
      }
    }  /* END for (rqindex) */

  return(FALSE);
  }  /* END MJobIsMultiRM() */





/* NOTE:  one of 'R' or 'RType' must be specified */

int MJobRMNLToString(

  mjob_t                   *J,          /* I */
  mrm_t                    *R,          /* I (optional) */
  enum MRMResourceTypeEnum  RType,      /* I (optional) */
  mstring_t                *Buf)        /* O */

  {
  int   rqindex;

  if (R != NULL)
    {
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      if (J->Req[rqindex]->RMIndex != R->Index)
        continue;

      if (MNLIsEmpty(&J->Req[rqindex]->NodeList))
        continue;

      MNLToMString(&J->Req[rqindex]->NodeList,TRUE,",",'\0',-1,Buf);
      }    /* END for (rqindex) */
    }      /* END if (R != NULL) */
  else
    {
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      if ((!bmisclear(&MRM[J->Req[rqindex]->RMIndex].RTypes)) &&
          (!bmisset(&MRM[J->Req[rqindex]->RMIndex].RTypes,RType)))
        continue;

      if (MNLIsEmpty(&J->Req[rqindex]->NodeList))
        continue;

      MNLToMString(&J->Req[rqindex]->NodeList,TRUE,",",'\0',-1,Buf);
      }    /* END for (rqindex) */
    }

  return(SUCCESS);
  }  /* END MJobRMNLToString() */



/**
 * Submit a job that is like JSpec.
 *
 * @param JSpec (I)
 * @param NewJ  (O) optional
 * @param EMsg
 */

int MJobSubmitFromJSpec(

  mjob_t  *JSpec,
  mjob_t **NewJ,
  char    *EMsg)

  {
  mbitmap_t BM;
  mrm_t *RM;
  mxml_t *JE = NULL;
  mjob_t *tmpNewJ;

  char JobName[MMAX_NAME];

  int rc;

  const char *FName = "MJobSubmitFromJSpec";

  MDB(4,fSCHED) MLog("%s(%s,%s,EMsg)\n",
    FName,
    (JSpec != NULL) ? "JSpec" : "NULL",
    (NewJ != NULL) ? "NewJ" : "NULL");

  if (JSpec == NULL)
    {
    MUStrCpy(EMsg,"internal error\n",MMAX_LINE);

    return(FAILURE);
    }

  if (NewJ == NULL)
    NewJ = &tmpNewJ;

  /* submit job based on R->SpecJ */

  if (MS3JobToXML(JSpec,&JE,NULL,TRUE,0,NULL,NULL,NULL,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  mstring_t JobString(MMAX_LINE);
  
  if (MXMLToMString(JE,&JobString,NULL,TRUE) == FAILURE)
    {
    MUStrCpy(EMsg,"could not convert to XML\n",MMAX_LINE);

    return(FAILURE);
    }

  MRMAddInternal(&RM);

  JobName[0] = '\0';

  rc = MS3JobSubmit(
         JobString.c_str(),
         RM,
         NewJ,
         &BM,
         JobName,
         EMsg,                            /* O */
         NULL);

  MXMLDestroyE(&JE);

  if (rc == SUCCESS)
    {
    MJobDuplicate(*NewJ,JSpec,TRUE);
    }

  bmclear(&BM);

  return(rc);
  }  /* END MJobSubmitFromJSpec() */



/**
 * Returns TRUE if RQ only consumes gres resources on the SHARED partition.
 * 
 * @param RQ
 */

mbool_t MReqIsGlobalOnly(

  const mreq_t *RQ)

  {
  int gindex;

  if ((RQ->DRes.Procs != 0) ||
      (RQ->DRes.Mem != 0) ||
      (RQ->DRes.Swap != 0) ||
      (RQ->DRes.Disk != 0))
    {
    /* requesting resource that does not exist on GLOBAL node */

    return(FALSE);
    }

  if (MSNLIsEmpty(&RQ->DRes.GenericRes))
    {
    return(FALSE);
    }

  /* Check GRes on Shared */

  if (MSched.SharedPtIndex >= 0)
    {
    for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
      {
      if (MGRes.Name[gindex][0] == '\0')
        break;

      if ((MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) > 0) &&
          (MSNLGetIndexCount(&MPar[MSched.SharedPtIndex].CRes.GenericRes,gindex) > 0))
        {
        /* because you can't combine floating and node-locked gres on the same req
           we know that this req is global only */

        return(TRUE);
        }
      }
    }

  /* Check GRes on Global */

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    if ((MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) > 0) &&
        ((MSched.GN != NULL) &&
         (MSNLGetIndexCount(&MSched.GN->CRes.GenericRes,gindex) > 0)))
      {
      /* requesting resource that exists on GLOBAL node */

      return(TRUE);
      }
    }

  return(FALSE);
  }  /* END MReqIsGlobalOnly() */



/** 
 * Return FALSE if job wants anything besides the GRes specified.
 *
 * @param J (I)
 * @param gindex (I)
  */
    
mbool_t MJobOnlyWantsThisGRes(
      
  mjob_t *J,
  int     gindex)
      
  {   
  mreq_t *RQ;
    
  if (J->Req[1] != NULL)
    {
    /* job is requesting more than one req, how can it possibly only want this gres? */
    
    return(FALSE);
    } 

  RQ = J->Req[0];
      
  if (RQ->DRes.Procs != 0)
    return(FALSE);
    
  if (RQ->DRes.Mem != 0)
    return(FALSE);
    
  if (RQ->DRes.Disk != 0)
    return(FALSE);
    
  if (RQ->DRes.Swap != 0)
    return(FALSE);
    
  if (MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) != MSNLGetIndexCount(&RQ->DRes.GenericRes,0))
    return(FALSE);
  
  return(TRUE);
  }  /* END MJobOnlyWantsThisGRes() */







/**
 * Increments a job's extended load information. Simply
 * give this function which metric you wish to modify
 * and it does the rest!
 *
 * @param J (I)
 * @param Index (I)
 * @param IncrAmount (I)
 *
 */

int MJobIncrExtLoad(

  mjob_t *J,
  int     Index,
  double  IncrAmount)

  {
  mgmetric_t *XM;

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->ExtLoad == NULL)
    {
    J->ExtLoad = (mxload_t *)MUCalloc(1,sizeof(mxload_t));
     
    MGMetricCreate(&J->ExtLoad->GMetric);
    }

  XM = J->ExtLoad->GMetric[Index];

  XM->Total += IncrAmount;

  if ((XM->Max == 0) || (XM->Max < IncrAmount))
    XM->Max = IncrAmount;

  if ((XM->Min == 0) || (XM->Min > IncrAmount))
    XM->Min = IncrAmount;

  XM->SampleCount++;

  return(SUCCESS);
  }  /* END MJobIncrExtLoad() */




/**
 * Initialize a list of job pointers.
 *
 * @param JList
 */

int MJobListAlloc(

  mjob_t ***JList)

  {
  if (JList == NULL)
    {
    return(FAILURE);
    }

  /* allocate new array */

  *JList = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * (MSched.M[mxoJob] + 1));

  if (*JList == NULL)
    return(FAILURE);
 
  return(SUCCESS);
  }  /* END MJobArrayAlloc() */




/**
 * Returns TRUE if J is in List.
 *
 */

mbool_t MJobIsInList(

  mjob_t **List,
  mjob_t  *J)

  {
  int jindex;

  for (jindex = 0;List[jindex] != NULL;jindex++)
    {
    if (List[jindex] == J)
      return(TRUE);
    }

  return(FALSE);
  }  /* END MJobIsInList() */




/**
 * Determine whether job should be NUMA scheduled.
 *
 * @param J
 */

mbool_t MJobWantsSharedMem(

  mjob_t *J)

  {
  if ((J->SubmitRM != NULL) && 
      (bmisset(&MPar[J->SubmitRM->PtIndex].Flags,mpfSharedMem)))
    {
    /* job came from a NB scheduled partition */

    return(TRUE);
    }

  if ((J->DestinationRM != NULL) && 
      (bmisset(&MPar[J->DestinationRM->PtIndex].Flags,mpfSharedMem)))
    {
    /* job is going to a NB scheduled parition */
    return(TRUE);
    }

  if (bmisset(&MPar[0].Flags,mpfSharedMem))
    {
    /* all jobs should be scheduled with shared memory */

    return(TRUE);
    }
 
  if (bmisset(&J->Flags,mjfSharedMem))
    {
    /* job explicitly requested shared memory */

    return(TRUE);
    }

  return(FALSE);
  }  /* END MJobWantsSharedMem() */
    
    

/**
 *  Does the periodic NAMI charges for running jobs
 */

int MJobDoPeriodicCharges()

  {
  job_iter JTI;

  mjob_t *J;

  const char *FName = "MJobDoPeriodicCharges";

  MDB(3,fSTAT) MLog("%s()\n",
    FName);

  MJobIterInit(&JTI);

  if (MAM[0].Name[0] == '\0')
    
  /* Don't do periodic billing if ChargePolicy is not DebitAll */
  if ((MAM[0].ChargePolicy == mamcpDebitSuccessfulWC) ||
      (MAM[0].ChargePolicy == mamcpDebitSuccessfulCPU) ||
      (MAM[0].ChargePolicy == mamcpDebitSuccessfulNode) ||
      (MAM[0].ChargePolicy == mamcpDebitSuccessfulPE) ||
      (MAM[0].ChargePolicy == mamcpDebitSuccessfulBlocked) ||
      (MAM[0].ChargePolicy == mamcpDebitRequestedWC))
    {
    MDB(3,fSTAT) MLog("ERROR:    Periodic charging disabled due to incompatible charge policy (%s)\n",
      MAMChargeMetricPolicy[MAM[0].ChargePolicy]);

    return(SUCCESS);
    }

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (!MJOBISACTIVE(J))
      continue;

    /* NAMI billing - Update Charges */

    if (MAMHandleUpdate(&MAM[0],
          (void *)J,
          mxoJob,
          NULL,
          NULL,
          NULL) == FAILURE)
      {
      MDB(3,fSTRUCT) MLog("WARNING:  Unable to register job update with accounting manager for job '%s'",
        J->Name);

      /* Update failure action not yet implemented */
      }
    }  /* END for (J = MJob[0]->Next;J != NULL;J = J->Next) */

  return(SUCCESS);
  }  /* END MJobDoPeriodicCharges() */




/**
 * determines if a job's request is Proc-centric or not
 *
 * @param J           (I)
 * @param TaskString  (I) (optional) the task request string for the job
 */

mbool_t MJobReqIsProcCentric(

  mjob_t *J,
  char   *TaskString)

  {
  mbitmap_t *JNMatchBM;
  mbool_t Result = TRUE;

  const char *FName = "MJobReqIsProcCentric";

  MDB(8,fPBS) MLog("%s(%s,'%s')\n",
    FName,
    (!MUStrIsEmpty(J->Name)) ? J->Name : "J",
    (!MUStrIsEmpty(TaskString)) ? TaskString : "TaskString");

  JNMatchBM = ((J->Credential.U != NULL) && (!bmisclear(&J->Credential.U->JobNodeMatchBM))) ?
    &J->Credential.U->JobNodeMatchBM :
    &MPar[0].JobNodeMatchBM;

  /* if the job's SRM has a NodeCentric submit policy or we're doing
   * ExactTask, we don't want proc-centric */

  if (((J->SubmitRM != NULL) &&
       (J->SubmitRM->SubmitPolicy == mrmspNodeCentric)) ||
       bmisset(JNMatchBM,mnmpExactTaskMatch))
    {
    Result = FALSE;
    }

  /* if the job is requesting gpus we don't want proc-centric */

  else if ((bmisset(&J->IFlags,mjifPBSGPUSSpecified)) ||
      ((!MUStrIsEmpty(TaskString)) && (strstr(TaskString,"gpus") != NULL)))
    {
    Result = FALSE;
    }

  return(Result);
  }
/* END MJob.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
