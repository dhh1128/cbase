/* HEADER */

      
/**
 * @file MJobTransition.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"
#include "MBSON.h"
 
/**
 * set the hold state and substates, with an appropriate description/block
 * message for the given job transition, based on the given job
 *
 * @param   DJ (I) (Modified)
 * @param   SJ (I)
 */

int __MJobTransitionSetState(

  mtransjob_t *DJ,  /* I (Modified) */
  mjob_t      *SJ)  /* I */

  {
  enum MJobNonEType tmpBlockReason = mjneNONE;

  /* if the job is active or completed, not much to do... */

  if ((MJOBISACTIVE(SJ)) || (MJOBISCOMPLETE(SJ)))
    {
    DJ->State = SJ->State;

    DJ->SubState = SJ->SubState;

    if (SJ->State == mjsRunning)
      MUStrDup(&DJ->BlockReason,"State:non-idle state 'Running'");

    DJ->ExpectedState = SJ->EState;

    return(SUCCESS);
    } /* END if (job is active or completed) */

  /* if the job is idle... */

  if (MJOBISIDLE(SJ))
    {
    char *tmpBlockMsg = NULL;

    tmpBlockReason = MJobGetBlockReason(SJ,NULL);

    if (tmpBlockReason != mjneNONE)
      {
      /* get the block reason */
      MUStrDup(&DJ->BlockReason,MJobBlockReason[tmpBlockReason]);
      
      /* get the block message, if it exists */
      if ((tmpBlockMsg = MJobGetBlockMsg(SJ,NULL)) != NULL)
        {
        MUStrDup(&DJ->BlockMsg,tmpBlockMsg);
        }
      } /* END if (tmpBlockReason != mjneNONE) */

    /* check for holds */

    DJ->Hold = SJ->Hold;
    } /* END if(MJOBISIDLE(SJ)) */

  /* set DJ->State/SubState from SJ */

  DJ->State = SJ->State;
  DJ->SubState = SJ->SubState;
  DJ->ExpectedState = SJ->EState;

  return(SUCCESS);
  } /* END __MJobTransitionSetState() */


/**
 * Store job in transition structure for storage in databse.
 *
 * @see MReqToTransitionStruct (child)
 * 
 * @param   SJ (I) the job to store
 * @param   DJ (O) the transition object to store it in
 * @param   LimitedTransition (I)
 */

int MJobToTransitionStruct(

  mjob_t      *SJ,
  mtransjob_t *DJ,
  mbool_t      LimitedTransition)

  {
  int rqindex;
  int pindex;

  char tmpBuf[MMAX_LINE];

#ifdef MUSEMONGODB
  MJobTransitionToBSON(SJ,&DJ->BSON);
#endif

  MUStrCpy(DJ->Name,SJ->Name,MMAX_LINE);

  __MJobTransitionSetState(DJ,SJ);

  DJ->UserPriority = SJ->UPriority;
  DJ->SystemPriority = SJ->SystemPrio;
  DJ->Priority = SJ->CurrentStartPriority;
  DJ->RunPriority = SJ->RunPriority;
  DJ->PSDedicated = SJ->PSDedicated;
  DJ->PSUtilized = SJ->PSUtilized;
  DJ->EffQueueDuration = SJ->EffQueueDuration;
  DJ->EligibleTime = SJ->EligibleTime;

  if (MSched.PerPartitionScheduling == TRUE)
    {
    char tmpLine[MMAX_LINE];

    tmpBuf[0] = '\0';

    /* store the partition priorities as a string
       <parname>:<priority>:<rsvstarttime>,<parname>:<priority>:<rsvstarttime>,... */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      mulong RsvStartTime = 0;

      if (MPar[pindex].Name[0] == '\0')
        break;

      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (!bmisset(&SJ->PAL,pindex))
        continue;

      if ((SJ->Rsv != NULL) && (SJ->Rsv->PtIndex == pindex))
        RsvStartTime = SJ->Rsv->StartTime;
  
      sprintf(tmpLine,"%s%s:%ld:%lu",
        (tmpBuf[0] == '\0') ? "" : ",",
        MPar[pindex].Name,
        SJ->PStartPriority[pindex],
        RsvStartTime);
  
      MUStrCat(tmpBuf,tmpLine,sizeof(tmpBuf));
      } /* END for ... */

    MUStrDup(&DJ->PerPartitionPriority,tmpBuf);
    } /* END if (MSched.PerPartitionScheduling == TRUE) */

  for (rqindex = 0; rqindex < MMAX_REQ_PER_JOB; rqindex++)
    {
    mreq_t *RQ;

    if (SJ->Req[rqindex] == NULL)
      {
      break;
      }

    RQ = SJ->Req[rqindex];

    DJ->MaxProcessorCount += RQ->TaskCount * RQ->DRes.Procs;

    /* allocate and populate this request in the job transition requirements */

    MReqTransitionAllocate(&DJ->Requirements[rqindex]);

    MReqToTransitionStruct(SJ,RQ,DJ->Requirements[rqindex],LimitedTransition);
    }

  if (DJ->Requirements[0] == NULL)
    return(FAILURE);

  mstring_t tmpString(MMAX_LINE);

  /* copy the block reason and block message for each partition in the format
        "<blockreason>:<blockmessage>"
     or if per partition scheduling
        "<parname>:<blockreason>:<blockmessage> ~rs <parname>:<blockreason>:<blockmessage> ~rs ..."
   
     NOTE: This overwrites the BlockReason created in __MJobTransitionSetState are resets it with
     BlockMsg appended to the end */

  if ((MJobAToMString(SJ,mjaBlockReason,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->BlockReason,tmpString.c_str());

  bmcopy(&DJ->Hold,&SJ->Hold);
  DJ->HoldTime = SJ->HoldTime;

  DJ->CompletionTime = SJ->CompletionTime;

  if (MJOBISCOMPLETE(SJ) == TRUE)
    {
    /* AWallTime comes from RM so it's the actual used walltime, go ahead and use it for completed jobs */
    /* also, this is what checkjob uses so if we don't use this then you get two different numbers. */
    DJ->UsedWalltime = SJ->AWallTime;
    }
  else if (SJ->StartTime > 0)
    {
    DJ->UsedWalltime = MAX(MSched.Time - SJ->StartTime - SJ->SWallTime,0);
    }
  else
    {
    DJ->UsedWalltime = SJ->AWallTime;
    }

  DJ->MSUtilized = SJ->MSUtilized;

  /* figure out whether this job is an array master or sub job and set the
   * appropriate flag if it is */

  if (bmisset(&SJ->Flags,mjfArrayMaster))
    {
    bmset(&DJ->TFlags,mtransfArrayMasterJob);
    DJ->ArrayMasterID = MJobGetArrayMasterTransitionID(SJ);
    }
  else if ((bmisset(&SJ->Flags,mjfArrayJob)) && (SJ->JGroup != NULL))
    {
    bmset(&DJ->TFlags,mtransfArraySubJob);
    DJ->ArrayMasterID = MJobGetArrayMasterTransitionID(SJ);
    }
  else
    {
    DJ->ArrayMasterID = -1;
    }

  /* Set the SystemJob flag on system jobs */

  if (SJ->System != NULL)
    bmset(&DJ->TFlags,mtransfSystemJob);

  if (bmisset(&SJ->IFlags,mjifWasCanceled))
    bmset(&DJ->TFlags,mtransfWasCanceled);

  if (bmisset(&SJ->IFlags,mjifPurge))
    bmset(&DJ->TFlags,mtransfPurge);

  if (SJ->Rsv != NULL)
    {
    DJ->RsvStartTime = SJ->Rsv->StartTime;
    
    MUStrDup(&DJ->ReservationAccess,SJ->Rsv->Name);
    }

  if ((MJobAToMString(SJ,mjaMessages,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Messages,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaFlags,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Flags,tmpString.c_str());

  bmcopy(&DJ->GenericAttributes,&SJ->AttrBM);

  DJ->SuspendTime = SJ->SWallTime;
  DJ->StartTime   = SJ->StartTime; /* Moved above limited transition for non master/slave jobs. */

  DJ->CompletionCode = SJ->CompletionCode;

  if ((MJobAToMString(SJ,mjaParentVCs,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->ParentVCs,tmpString.c_str());

  if ((LimitedTransition == TRUE) && (MSched.MDB.DBType != mdbODBC))
    {
    /* we don't support limited updates of the database yet, so if we're
       talking to ODBC then we always have to transition the entire job */

    bmset(&DJ->TFlags,mtransfLimitedTransition);

    return(SUCCESS);
    }

  /* attributes below are not set with LimitedTransition == TRUE */

  if ((MJobAToMString(SJ,mjaSRMJID,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->SourceRMJobID,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaDRMJID,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->DestinationRMJobID,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaGJID,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->GridJobID,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaSID,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->SystemID,tmpString.c_str());

  DJ->SessionID = SJ->SessionID;

  if ((MJobAToMString(SJ,mjaJobName,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->AName,tmpString.c_str());
  
  if ((MJobAToMString(SJ,mjaUser,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->User,tmpString.c_str());
  
  if ((SJ->Grid != NULL) && (SJ->Grid->User != NULL))
    MUStrDup(&DJ->GridUser,SJ->Grid->User);

  if ((SJ->Credential.U != NULL) && (SJ->Credential.U->HomeDir != NULL))
    MUStrDup(&DJ->UserHomeDir,SJ->Credential.U->HomeDir);

  if ((MJobAToMString(SJ,mjaAccount,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Account,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaClass,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Class,tmpString.c_str());

  if ((SJ->Grid != NULL) && (SJ->Grid->Class != NULL))
    MUStrDup(&DJ->GridClass,SJ->Grid->Class);

  if ((MJobAToMString(SJ,mjaQOS,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->QOS,tmpString.c_str());

  if ((SJ->Grid != NULL) && (SJ->Grid->QOS != NULL))
    MUStrDup(&DJ->GridQOS,SJ->Grid->QOS);

  if ((MJobAToMString(SJ,mjaQOSReq,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->QOSReq,tmpString.c_str());

  if ((SJ->Grid != NULL) && (SJ->Grid->QOSReq != NULL))
    MUStrDup(&DJ->GridQOSReq,SJ->Grid->QOSReq);

  if ((MJobAToMString(SJ,mjaGroup,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Group,tmpString.c_str());

  if ((SJ->Grid != NULL) && (SJ->Grid->Group!= NULL))
    MUStrDup(&DJ->GridGroup,SJ->Grid->Group);

  if ((MJobAToMString(SJ,mjaJobGroup,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->JobGroup,tmpString.c_str());

  /* NOTE:  Jobs in BatchHold have state Idle. need to send them to the DB with
   *        state BatchHold. */

  DJ->SubmitTime         = SJ->SubmitTime;
  DJ->QueueTime          = SJ->SystemQueueTime;
  DJ->CompletionCode     = SJ->CompletionCode;
  DJ->RequestedStartTime = SJ->SpecSMinTime;

  if (SJ->SysSMinTime != NULL)
    DJ->SystemStartTime = SJ->SysSMinTime[0];

  DJ->RequestedMinWalltime = SJ->MinWCLimit;

  if (SJ->WCLimit > 0)
    DJ->RequestedMaxWalltime = SJ->WCLimit;
  else if (SJ->SpecWCLimit[0] > 0)
    DJ->RequestedMaxWalltime = SJ->SpecWCLimit[0];
  else if (SJ->OrigWCLimit > 0)
    DJ->RequestedMaxWalltime = SJ->OrigWCLimit;
  else
    DJ->RequestedMaxWalltime = MDEF_SYSDEFJOBWCLIMIT;

  DJ->CPULimit = SJ->CPULimit;

  DJ->RequestedNodes = (MSched.BluegeneRM == TRUE) ? SJ->Request.TC / MSched.BGNodeCPUCnt : SJ->Request.NC;

  /* set MasterHost from SJ if present.  Override with NodeList if present */
  if (SJ->MasterHostName != NULL)
    MUStrDup(&DJ->MasterHost,SJ->MasterHostName);
  if (!MNLIsEmpty(&SJ->NodeList))
    {
    mnode_t *N = MNLReturnNodeAtIndex(&SJ->NodeList,0);

    DJ->ActivePartition = N->PtIndex;

    if (MJOBISACTIVE(SJ) || MJOBISCOMPLETE(SJ))
      MUStrDup(&DJ->MasterHost,N->Name);
    }

  if (SJ->DestinationRM != NULL)
    MUStrDup(&DJ->DestinationRM,SJ->DestinationRM->Name);

  if (SJ->SubmitRM != NULL)
    {
    MUStrDup(&DJ->SourceRM,SJ->SubmitRM->Name);

    DJ->SourceRMType = SJ->SubmitRM->Type;

    if (bmisset(&SJ->SubmitRM->IFlags,mrmifLocalQueue))
      DJ->SourceRMIsLocalQueue = TRUE;
    }

  DJ->MinPreemptTime = SJ->MinPreemptTime;

  if ((MJobAToMString(SJ,mjaDepend,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Dependencies,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaDependBlock,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->DependBlockMsg,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaReqHostList,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->RequestedHostList,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaExcHList,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->ExcludedHostList,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaSubmitHost,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->SubmitHost,tmpString.c_str());

  DJ->Cost = SJ->Cost;

  if ((MJobAToMString(SJ,mjaDescription,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Description,tmpString.c_str());

  if (SJ->NotifyAddress != NULL)
    MUStrDup(&DJ->NotificationAddress,SJ->NotifyAddress);

  bmcopy(&DJ->NotifyBM,&SJ->NotifyBM);

  DJ->StartCount = SJ->StartCount;
  DJ->BypassCount = SJ->BypassCount;

  if ((MJobAToMString(SJ,mjaCmdFile,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->CommandFile,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaArgs,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->Arguments,tmpString.c_str());

  DJ->RMSubmitType = SJ->RMSubmitType;

  if ((MJobAToMString(SJ,mjaStdIn,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->StdIn,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaStdOut,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->StdOut,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaStdErr,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->StdErr,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaRMOutput,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->RMOutput,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaRMError,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->RMError,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaIWD,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->InitialWorkingDirectory,tmpString.c_str());

  DJ->UMask = SJ->Env.UMask;

  if ((MJobAToMString(SJ,mjaReqReservation,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->RequiredReservation,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaReqReservationPeer,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->PeerRequiredReservation,tmpString.c_str());

  if ((MJobAToMString(SJ,mjaParentVCs,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->ParentVCs,tmpString.c_str());

  DJ->PSDedicated = SJ->PSDedicated;
  DJ->PSUtilized = SJ->PSUtilized;

  bmcopy(&DJ->PAL,&SJ->PAL);
  bmcopy(&DJ->SpecPAL,&SJ->SpecPAL);
  bmcopy(&DJ->SysPAL,&SJ->SysPAL);

  if ((MJobAToMString(SJ,mjaTemplateSetList,&tmpString) == SUCCESS) && !tmpString.empty())
    MUStrDup(&DJ->TemplateSetList,tmpString.c_str());

  if (SJ->MigrateBlockReason != mjneNONE)
    MUStrDup(&DJ->MigrateBlockReason,MJobBlockReason[SJ->MigrateBlockReason]);

  if (SJ->MigrateBlockMsg != NULL)
    MUStrDup(&DJ->MigrateBlockMsg,SJ->MigrateBlockMsg);

  if (!MUHTIsEmpty(&SJ->Variables))
    {
    /* Check if there is a VMID in the job variables */

    char *tmpVMID = NULL;

    if (MUHTGet(&SJ->Variables,"VMID",(void **)&tmpVMID,NULL) == SUCCESS)
      {
      MUStrDup(&DJ->AllocVMList,tmpVMID);
      }
    }

  if (SJ->Rsv)
    {
    MUStrDup(&DJ->ReservationAccess,SJ->Rsv->Name);
    }

  /* Ensure mstring_t TaskMap is allocated */
  if (DJ->TaskMap == NULL)
    DJ->TaskMap = new mstring_t(MMAX_NAME >> 2);

  /* Perform the Job toMString operation */
  MJobAToMString(SJ,mjaTaskMap,DJ->TaskMap);

  /* Ensure mstring_t RMExtensions is allocated */
  if (DJ->RMExtensions == NULL)
    DJ->RMExtensions = new mstring_t(MMAX_NAME >> 2);

  MJobAToMString(SJ,mjaRMXString,DJ->RMExtensions);

  if (!MUHTIsEmpty(&SJ->Variables))
    {
    MUVarsToXML(&DJ->Variables,&SJ->Variables);

    if (MUStrIsEmpty(DJ->Description))
      {
      mstring_t Vars(MMAX_LINE);

      MXMLToMString(DJ->Variables,&Vars,NULL,FALSE);

      MUStrDup(&DJ->Description,Vars.c_str());
      }
    }  /* END if (!MUHTIsEmpty(&SJ->Variables)) */

  if (SJ->Env.BaseEnv != NULL)
    {
    MUStrDup(&DJ->Environment,SJ->Env.BaseEnv);
    }

  MUStrDup(&DJ->RMSubmitString,SJ->RMSubmitString);

  return(SUCCESS);
  }  /* END MJobToTransitionStruct() */




/**
 * Allocate storage for the given job transition object
 *
 * @param   JP (O) [alloc-ed] pointer to the newly allocated job transition object
 */

int MJobTransitionAllocate(

  mtransjob_t **JP)

  {
  int rqindex;

  mtransjob_t *J;

  if (JP == NULL)
    {
    return(FAILURE);
    }

  J = (mtransjob_t *)MUCalloc(1,sizeof(mtransjob_t));

  for (rqindex = 0; rqindex < MMAX_REQ_PER_JOB; rqindex++)
    {
    J->Requirements[rqindex] = NULL;
    }

  *JP = J;

  return(SUCCESS);
  }  /* END MJobTransitionAllocate() */




/**
 * Free the storage for the given job transtion object
 *
 * @param   JP (I) [freed] pointer to the job transition object
 */

int MJobTransitionFree(

  void **JP)

  {
  int rqindex;

  mtransjob_t *J;

  if (JP == NULL)
    {
    return(FAILURE);
    }

  J = (mtransjob_t *)*JP;

  /* free all request objects */

  for (rqindex = 0; rqindex < MMAX_REQ_PER_JOB; rqindex++)
    {
    if (J->Requirements[rqindex] == NULL)
      {
      break;
      }

    MReqTransitionFree((void **)&J->Requirements[rqindex]);
    }

  /* free the TaskMap mstring */
  if (J->TaskMap != NULL)
    {
    delete J->TaskMap;
    J->TaskMap = NULL;
    }

  /* free the RMExtensions mstring */
  if (J->RMExtensions != NULL)
    {
    delete J->RMExtensions;
    J->RMExtensions = NULL;
    }

  MUFree(&J->Environment);
  MUFree(&J->RMSubmitString);
  MUFree(&J->SourceRMJobID);
  MUFree(&J->DestinationRMJobID);
  MUFree(&J->AName);
  MUFree(&J->JobGroup);
  MUFree(&J->User);
  MUFree(&J->Group);
  MUFree(&J->Account);
  MUFree(&J->Class);
  MUFree(&J->QOS);
  MUFree(&J->QOSReq);
  MUFree(&J->GridUser);
  MUFree(&J->GridGroup);
  MUFree(&J->GridAccount);
  MUFree(&J->GridClass);
  MUFree(&J->GridQOS);
  MUFree(&J->GridQOSReq);
  MUFree(&J->UserHomeDir);
  MUFree(&J->DestinationRM);
  MUFree(&J->SourceRM);
  MUFree(&J->Dependencies);
  MUFree(&J->DependBlockMsg);
  MUFree(&J->RequestedHostList);
  MUFree(&J->ExcludedHostList);
  MUFree(&J->MasterHost);
  MUFree(&J->SubmitHost);
  MUFree(&J->ReservationAccess);
  MUFree(&J->RequiredReservation);
  MUFree(&J->PeerRequiredReservation);
  MUFree(&J->Flags);
  MUFree(&J->StdErr);
  MUFree(&J->StdOut);
  MUFree(&J->StdIn);
  MUFree(&J->RMOutput);
  MUFree(&J->RMError);
  MUFree(&J->CommandFile);
  MUFree(&J->Arguments);
  MUFree(&J->InitialWorkingDirectory);
  MUFree(&J->Description);
  MUFree(&J->Messages);
  MUFree(&J->NotificationAddress);
  MUFree(&J->BlockReason);
  MUFree(&J->BlockMsg);
  MUFree(&J->AllocVMList);
  MUFree(&J->MigrateBlockReason);
  MUFree(&J->MigrateBlockMsg);
  MUFree(&J->GridJobID);
  MUFree(&J->SystemID);
  MUFree(&J->TemplateSetList);
  MUFree(&J->PerPartitionPriority);
  MUFree(&J->ParentVCs);

  bmclear(&J->TFlags);
  bmclear(&J->SpecPAL);
  bmclear(&J->SysPAL);
  bmclear(&J->PAL);
  bmclear(&J->GenericAttributes);
  bmclear(&J->Hold);
  bmclear(&J->NotifyBM);

  MXMLDestroyE(&J->Variables);

#ifdef MUSEMONGODB
  if (J->BSON != NULL)
    {
    delete J->BSON;
    J->BSON = NULL;
    }
#endif

  /* free the actual job transition */
  MUFree((char **)JP);

  return(SUCCESS);
  }  /* END MJobTransitionFree() */




/**
 * @param SJ            (I)
 * @param DJ            (O)
 */

int MJobTransitionCopy(

  mtransjob_t *SJ,         /* I */
  mtransjob_t *DJ)         /* O */

  {
  int rindex;
  char *Dependencies;
  char *DependBlockMsg;

  if ((DJ == NULL) || (SJ == NULL))
    return(FAILURE);

  /* free the allocated pointers so the memorybefore they are clobbered
     by the memcpy - prevent memory leaks */

  Dependencies = DJ->Dependencies;
  DependBlockMsg = DJ->DependBlockMsg;

  memcpy(DJ,SJ,sizeof(mtransjob_t));

  DJ->Dependencies = Dependencies;
  DJ->DependBlockMsg = DependBlockMsg;

  MUStrCpy(DJ->Dependencies,SJ->Dependencies,MMAX_LINE);
  MUStrCpy(DJ->DependBlockMsg,SJ->DependBlockMsg,MMAX_LINE);

  for (rindex = 0; SJ->Requirements[rindex] != NULL; rindex++)
    {
    MReqTransitionAllocate(&DJ->Requirements[rindex]);
    if (DJ->Requirements[rindex] != NULL)
      MReqTransitionCopy(DJ->Requirements[rindex],SJ->Requirements[rindex]);
    else
      raise(SIGINT);
    }

  return(SUCCESS);
  } /* END MJobTransitionCopy */ 


/**
 * check to see if a given mtransjob_t matches a list of constraints
 *
 * Use the following rules:
 *
 * @li 1) within each mjobconstraint_t, there can be a list of matches--these
 *    are all OR'd with each other--i.e. a match of any of them results in 
 *    success.
 *
 * @li 2) if a mjobconstraint_t contains multiple items, they are stored as follows:
 *    a) in an AVal, the items are in a comma separated string in Aval[0]
 *    b) in a ALong, the items are in Along[i] where the list is terminated by
 *       msjNONE if there are fewer than MMAX_JOBCVAL items.
 *
 * @li 3) When there are more than one mjobconstraint_t's in the JConstraintList, the job
 *    must match all of them to ultimately return TRUE--i.e. AND'd between 
 *    individual constraints.
 *
 *
 * @param J               (I)
 * @param JConstraintList (I)
 */
mbool_t MJobTransitionMatchesConstraints(

    mtransjob_t *J,                 /* I */
    marray_t    *JConstraintList)   /* I */

  {
  int cindex;
  mjobconstraint_t *JConstraint;
  mbool_t singlematch = FALSE;

  if (J == NULL)
    {
    return(FALSE);
    }

  if ((JConstraintList == NULL) || (JConstraintList->NumItems == 0))
    {
    return(TRUE);
    }

  MDB(7,fSCHED) MLog("INFO:     Checking job %s against constraint list.\n",
    J->Name);

  for (cindex = 0; cindex < JConstraintList->NumItems; cindex++)
    {
    JConstraint = (mjobconstraint_t *)MUArrayListGet(JConstraintList,cindex);
    singlematch = FALSE;

    switch(JConstraint->AType)
      {
      case mjaJobID:

        if ((!strcasecmp(JConstraint->AVal[0],"ALL")) ||
            (MUStrIsInList(JConstraint->AVal[0],J->Name,',')))
          {
          singlematch = TRUE;
          }

        break;

      case mjaState:

        if (JConstraint->ACmp == mcmpEQ2)
          {
          if (!strcmp(JConstraint->AVal[0],"ALL"))
            {
            singlematch = TRUE;
            }
          else if (MUStrIsInList(JConstraint->AVal[0],MJobState[J->State],','))
            {
            singlematch = TRUE;
            }
          else if ((bmisset(&J->TFlags,mtransfSystemJob)) && (!strcmp(JConstraint->AVal[0],"system")))
            {
            singlematch = TRUE;
            }
          else if ((MSTATEISCOMPLETE(J->State)) && (!strcmp(JConstraint->AVal[0],"completed")))
            {
            singlematch = TRUE;
            }
          }
        else if (JConstraint->ACmp == mcmpNE)
          {
          /* return FALSE if the job state matches any of the state values in
           * this constraint list */

          int index = 0;
          singlematch = TRUE;

          while ((JConstraint->ALong[index] != mjsNONE) && (index < MMAX_JOBCVAL))
            {
            if (J->State == JConstraint->ALong[index])
              {
              singlematch = FALSE;
              }

            index++;
            } /* END While */

          } /* END if */

        break;

      case mjaArrayMasterID:

        if ((JConstraint->ACmp == mcmpEQ) && (J->ArrayMasterID == JConstraint->ALong[0]))
          {
          singlematch = TRUE;
          }

        break;

      case mjaUser:

        if ((!MUStrIsEmpty(J->User)) && 
            (MUStrIsInList(JConstraint->AVal[0],J->User,',')))
          {
          singlematch = TRUE;
          }

         break;

      case mjaGroup:

        if ((!MUStrIsEmpty(J->Group)) && 
            (MUStrIsInList(JConstraint->AVal[0],J->Group,',')))
          {
          singlematch = TRUE;
          }

         break;

      case mjaAccount:

        if ((!MUStrIsEmpty(J->Account)) && 
            (MUStrIsInList(JConstraint->AVal[0],J->Account,',')))
          {
          singlematch = TRUE;
          }

         break;

      case mjaClass:

        if ((!MUStrIsEmpty(J->Class)) && 
            (MUStrIsInList(JConstraint->AVal[0],J->Class,',')))
          {
          singlematch = TRUE;
          }

         break;

      case mjaQOS:

        if ((!MUStrIsEmpty(J->QOS)) && 
            (MUStrIsInList(JConstraint->AVal[0],J->QOS,',')))
          {
          singlematch = TRUE;
          }

         break;


      default:

        break;
      } /* end switch(JConstraint->AType) */

    if (FALSE == singlematch)
      {
      /* if it has failed to match any single constraint then it fails entirely 
       * because the constraints are AND'ed together */

      return(FALSE);
      }

    } /* END for (cindex = 0; cindex < JConstraintList->NumItems; cindex++) */

  return(TRUE);
  } /* END MJobTransitionMatchesConstraints */



/**
 * get the string version of a given job transition attribute
 *
 * @param J       (I) the job transition object
 * @param AIndex  (I) the attribute we want
 * @param OBuf    (O) the output buffer
 * @param BufSize (I) the size of the output buffer
 * @param Mode    (I) [not used except by mjaVariable]
 */

int MJobTransitionAToString(

  mtransjob_t          *J,        /* I */
  enum MJobAttrEnum     AIndex,   /* I */
  char                 *OBuf,     /* O */
  int                   BufSize,  /* I */
  enum MFormatModeEnum  Mode)     /* I (not used except by mjaVariable) */

  {
  if ((J == NULL) || (OBuf == NULL))
    {
    return(FAILURE);
    }

  MDB(7,fSCHED) MLog("INFO:     Getting string attribute for %s on job %s\n",
    MJobAttr[AIndex],
    J->Name);

  OBuf[0] = '\0';

  switch(AIndex)
    {
    case mjaAccount:

      if (!MUStrIsEmpty(J->Account))
        MUStrCpy(OBuf,J->Account,BufSize);

      break;

    case mjaAllocNodeList:

      if (J->Requirements[0] == NULL)
        {
        return(FAILURE);
        }

      /* TODO enable multi-req */
      
      /* Check if there is a AllocNodeList mstring_t present prior to dereferencing it */
      if (J->Requirements[0]->AllocNodeList != NULL)
        MUStrCpy(OBuf,J->Requirements[0]->AllocNodeList->c_str(),BufSize);
      else
        MUStrCpy(OBuf,"",BufSize);

      break;

    case mjaAllocVMList:
  
      /* if there is an AllocVMList present, copy to the output buffer */

      if ((J->AllocVMList != NULL) && (!MUStrIsEmpty(J->AllocVMList)))
        MUStrCpy(OBuf,J->AllocVMList,BufSize);

      break;

    case mjaAWDuration:
      
      if (J->UsedWalltime > 0)
        {
        snprintf(OBuf,
          BufSize,
          "%ld",
          J->UsedWalltime);
        }

      break;

    case mjaArgs:

      if (!MUStrIsEmpty(J->Arguments))
        MUStrCpy(OBuf,J->Arguments,BufSize);

      break;

    case mjaBecameEligible:

      snprintf(OBuf,BufSize,"%ld",J->EligibleTime);

      break;

    case mjaBlockReason:

      if (!MUStrIsEmpty(J->BlockReason))
        MUStrCpy(OBuf,J->BlockReason,BufSize);

      break;

    case mjaBypass:

      if (J->BypassCount > 0)
        snprintf(OBuf,BufSize,"%d",J->BypassCount);

      break;

    case mjaClass:

      if (!MUStrIsEmpty(J->Class))
        MUStrCpy(OBuf,J->Class,BufSize);

      break;

    case mjaCmdFile:

      if (!MUStrIsEmpty(J->CommandFile))
        MUStrCpy(OBuf,J->CommandFile,BufSize);

      break;

    case mjaCompletionCode:

      if (MSTATEISCOMPLETE(J->State))
        snprintf(OBuf,BufSize,"%d",J->CompletionCode);

      break;

    case mjaCompletionTime:

      if (J->CompletionTime > 0)
        snprintf(OBuf,BufSize,"%ld",J->CompletionTime);

      break;

    case mjaCost:

      if (J->Cost > 0.0)
        snprintf(OBuf,BufSize,"%.3f",J->Cost);

      break;

    case mjaCPULimit:

      if (J->CPULimit > 0)
        snprintf(OBuf,BufSize,"%ld",J->CPULimit);

      break;

    case mjaDepend:

      MUStrCpy(OBuf,J->Dependencies,BufSize);

      break;

    case mjaDependBlock:

      MUStrCpy(OBuf,J->DependBlockMsg,BufSize);

      break;

    case mjaDescription:

      if (!MUStrIsEmpty(J->Description))
        MUStrCpy(OBuf,J->Description,BufSize);

      break;

    case mjaDRM:

      if (!MUStrIsEmpty(J->DestinationRM))
        MUStrCpy(OBuf,J->DestinationRM,BufSize);

      break;

    case mjaDRMJID:

      if (!MUStrIsEmpty(J->DestinationRMJobID))
        MUStrCpy(OBuf,J->DestinationRMJobID,BufSize);

      break;

    case mjaEEWDuration:

      snprintf(OBuf,BufSize,"%ld",J->EffQueueDuration);

      break;

    case mjaEffPAL:

      if (!bmisclear(&J->PAL))
        MPALToString(&J->PAL,NULL,OBuf);

      break;

    case mjaEFile:

      if (!MUStrIsEmpty(J->StdErr))
        MUStrCpy(OBuf,J->StdErr,BufSize);

      break;

    case mjaEState:

      if (J->ExpectedState != mjsNONE)
        MUStrCpy(OBuf,MJobState[J->ExpectedState],BufSize);

      break;

    case mjaExcHList:

      if (!MUStrIsEmpty(J->ExcludedHostList))
        MUStrCpy(OBuf,J->ExcludedHostList,BufSize);

      break;

    case mjaFlags:

      if (!MUStrIsEmpty(J->Flags))
        MUStrCpy(OBuf,J->Flags,BufSize);

      break;

    case mjaGAttr:

      if (!bmisclear(&J->GenericAttributes))
        {
        MUJobAttributesToString(&J->GenericAttributes,OBuf);
        }

      break;

    case mjaGJID:

      if (!MUStrIsEmpty(J->GridJobID))
        MUStrCpy(OBuf,J->GridJobID,BufSize);

      break;

    case mjaGroup:

      if (!MUStrIsEmpty(J->Group))
        MUStrCpy(OBuf,J->Group,BufSize);

      break;

    case mjaHold:

      bmtostring(&J->Hold,MHoldType,OBuf);

      break;

    case mjaHoldTime:

      snprintf(OBuf,BufSize,"%ld",J->HoldTime);

      break;

    case mjaJobGroup:

      if (!MUStrIsEmpty(J->JobGroup))
        MUStrCpy(OBuf,J->JobGroup,BufSize);

      break;

    case mjaReqHostList:

      if (!MUStrIsEmpty(J->RequestedHostList))
        MUStrCpy(OBuf,J->RequestedHostList,BufSize);

      break;

    case mjaIWD:

      if (!MUStrIsEmpty(J->InitialWorkingDirectory))
        MUStrCpy(OBuf,J->InitialWorkingDirectory,BufSize);

      break;

    case mjaJobID:

      MUStrCpy(OBuf,J->Name,BufSize);

      break;

    case mjaJobName:

      if (!MUStrIsEmpty(J->AName))
        MUStrCpy(OBuf,J->AName,BufSize);

      break;

    case mjaMessages:

      if (!MUStrIsEmpty(J->Messages))
        MUStrCpy(OBuf,J->Messages,BufSize);

      break;

    case mjaMinPreemptTime:

      if (J->MinPreemptTime > 0)
        snprintf(OBuf,BufSize,"%ld",J->MinPreemptTime);

      break;

    case mjaMinWCLimit:

      if (J->RequestedMinWalltime > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(J->RequestedMinWalltime,TString);

        snprintf(OBuf,BufSize,"%s",TString);
        }

      break;

    case mjaNotification:

      if (!bmisclear(&J->NotifyBM))
        {
        bmtostring(&J->NotifyBM,MNotifyType,OBuf); 
        }

      break;

    case mjaNotificationAddress:
      
      if (!MUStrIsEmpty(J->NotificationAddress))
        MUStrCpy(OBuf,J->NotificationAddress,BufSize);

      break;
      
    case mjaOFile:

      if (!MUStrIsEmpty(J->RMOutput))
        MUStrCpy(OBuf,J->RMOutput,BufSize);

      break;

    case mjaPAL:

      if (!bmisclear(&J->SpecPAL))
        MPALToString(&J->SpecPAL,NULL,OBuf);

      break;

    case mjaParentVCs:

      if (!MUStrIsEmpty(J->ParentVCs))
        MUStrCpy(OBuf,J->ParentVCs,BufSize);

      break;

    case mjaRsvAccess:

      if (!MUStrIsEmpty(J->ReservationAccess))
        MUStrCpy(OBuf,J->ReservationAccess,BufSize);

      break;

    case mjaQueueStatus:

      if (MJOBSISACTIVE(J->State))
        {
        MUStrCpy(OBuf,(char *)MJobStateType[mjstActive],BufSize);
        }
      else if (MSTATEISCOMPLETE(J->State))
        {
        /* NO-OP */
        }
      else if ((!MJOBSISBLOCKED(J->State)) &&
               (!MJOBSISBLOCKED(J->ExpectedState)) &&
               (bmisset(&J->Hold,mjneNONE)))
        {
        MUStrCpy(OBuf,(char *)MJobStateType[mjstEligible],BufSize);
        }
      else
        {
        MUStrCpy(OBuf,(char *)MJobStateType[mjstBlocked],BufSize);
        }

      break;

    case mjaQOS:

      if (!MUStrIsEmpty(J->QOS))
        MUStrCpy(OBuf,J->QOS,BufSize);

      break;

    case mjaQOSReq:

      if (!MUStrIsEmpty(J->QOSReq))
        MUStrCpy(OBuf,J->QOSReq,BufSize);

      break;

    case mjaReqAWDuration:

      {
      char tmpTime[20];

      MUStrCpy(OBuf,MULToTStringSeconds(J->RequestedMaxWalltime,tmpTime,20),BufSize);
      }

      break;

    case mjaReqProcs:

      snprintf(OBuf,BufSize,"%d",J->MaxProcessorCount);

      break;

    case mjaReqNodes:

      snprintf(OBuf,BufSize,"%d",J->RequestedNodes);

      break;

    case mjaReqReservation:

      if (!MUStrIsEmpty(J->RequiredReservation))
        MUStrCpy(OBuf,J->RequiredReservation,BufSize);

      break;

    case mjaReqReservationPeer:

      /* If PeerRequiredReservation is not empty then then the job was
       * submitted from a peer which had an advres. If it's empty but
       * RequiredReservation is not empty then the job is local with a
       * advres. The other peer doesn't need to know about the advres. */

      if (!MUStrIsEmpty(J->PeerRequiredReservation))
        MUStrCpy(OBuf,J->PeerRequiredReservation,BufSize);
      else if (!MUStrIsEmpty(J->RequiredReservation))
        MUStrCpy(OBuf,J->RequiredReservation,BufSize);

      break;

    case mjaReqSMinTime:

      if (J->RequestedStartTime > 0)
        snprintf(OBuf,BufSize,"%ld",J->RequestedStartTime);

      break;

    case mjaRM:

      if (!MUStrIsEmpty(J->SourceRM))
        MUStrCpy(OBuf,J->SourceRM,BufSize);

      break;

    case mjaRMError:

      if (!MUStrIsEmpty(J->RMError))
        MUStrCpy(OBuf,J->RMError,BufSize);

      break;

    case mjaRMOutput:

      if (!MUStrIsEmpty(J->RMOutput))
        MUStrCpy(OBuf,J->RMOutput,BufSize);

      break;

    case mjaSessionID:

      if (J->SessionID != 0)
        {
        snprintf(OBuf,BufSize,"%d",J->SessionID);
        }

      break;

    case mjaStdIn:

      if (!MUStrIsEmpty(J->StdIn))
        MUStrCpy(OBuf,J->StdIn,BufSize);

      break;

    case mjaStdOut:

      if (!MUStrIsEmpty(J->StdOut))
        MUStrCpy(OBuf,J->StdOut,BufSize);

      break;

    case mjaStdErr:

      if (!MUStrIsEmpty(J->StdErr))
        MUStrCpy(OBuf,J->StdErr,BufSize);

      break;

    case mjaSID:

      if (!MUStrIsEmpty(J->SystemID))
        MUStrCpy(OBuf,J->SystemID,BufSize);

      break;

    case mjaSRMJID:

      if (!MUStrIsEmpty(J->SourceRMJobID))
        MUStrCpy(OBuf,J->SourceRMJobID,BufSize);

      break;

    case mjaStartCount:

      snprintf(OBuf,BufSize,"%d",J->StartCount);

      break;

    case mjaStartPriority:

      snprintf(OBuf,BufSize,"%ld",J->Priority);

      break;

    case mjaStartTime:

      snprintf(OBuf,BufSize,"%ld",J->StartTime);

      break;

    case mjaState:

      MUStrCpy(OBuf,MJobState[J->State],BufSize);

      break;

    case mjaStatMSUtl:

      snprintf(OBuf,BufSize,"%.3f",J->MSUtilized);

      break;

    case mjaStatPSDed:

      snprintf(OBuf,BufSize,"%.3f",J->PSDedicated);

      break;

    case mjaStatPSUtl:

      snprintf(OBuf,BufSize,"%.3f",J->PSUtilized);

      break;

    case mjaSuspendDuration:

      snprintf(OBuf,BufSize,"%ld",J->SuspendTime);

      break;

    case mjaSubmitTime:

      snprintf(OBuf,BufSize,"%ld",J->SubmitTime);

      break;

    case mjaSubmitHost:

      if (!MUStrIsEmpty(J->SubmitHost))
        MUStrCpy(OBuf,J->SubmitHost,BufSize);

      break;

    case mjaSubmitLanguage:

      if (J->RMSubmitType != mrmtNONE)
        MUStrCpy(OBuf,MRMType[J->RMSubmitType],BufSize);

      break;

    case mjaSysSMinTime:

      if (J->SystemStartTime > 0)
        snprintf(OBuf,BufSize,"%ld",J->SystemStartTime);

      break;

    case mjaSysPrio:

      if (J->SystemPriority > 0)
        {
        mbool_t IsRelative = FALSE;

        long AdjustedSysPrio = J->SystemPriority;

        if (J->SystemPriority > (MMAX_PRIO_VAL << 1))
          {
          IsRelative = TRUE;
          AdjustedSysPrio -= (MMAX_PRIO_VAL << 1);
          }

        snprintf(OBuf,BufSize,"%s%ld",
          (IsRelative == TRUE) ? "+" : "",
          AdjustedSysPrio);
        }

      break;

    case mjaTemplateSetList:

      if (!MUStrIsEmpty(J->TemplateSetList))
        MUStrCpy(OBuf,J->TemplateSetList,BufSize);

      break;

    case mjaUMask:

      snprintf(OBuf,BufSize,"%ld",J->UMask);

      break;

    case mjaUser:

      if (!MUStrIsEmpty(J->User))
        MUStrCpy(OBuf,J->User,BufSize);

      break;

    case mjaUtlMem:

      if (J->UsedWalltime > 0)
        {
        if (J->MSUtilized > 0.01)
          {
          snprintf(OBuf,BufSize,"%.2f", J->MSUtilized / J->UsedWalltime);
          }
        }

      break;

    case mjaUserPrio:

      snprintf(OBuf,BufSize,"%ld",J->UserPriority);

      break;

    default:

      /* no-op */
      
      MDB(7,fSCHED) MLog("WARNING:  Job attribute %s not yet translated to string value.\n",MJobAttr[AIndex]);

      break;

    } /* END switch(AIndex) */

  return(SUCCESS);
  } /* END __MJobTransitionAToString() */




/**
 * transition a job to the queue to be written to the database
 *
 * NOTE: if LimitedTransition == TRUE then we will only transition
 *       a few attributes, like state and nodelist
 *
 * @see MJobToTransitionStruct (child)
 *      MReqToTransitionStruct (child)
 *
 * @param   J (I) the job to be transitioned
 * @param   LimitedTransition (I)
 * @param   DeleteExistingJob (I)
 */

int MJobTransition(

  mjob_t *J,
  mbool_t LimitedTransition,
  mbool_t DeleteExistingJob)

  {
  mtransjob_t *TJ;

  /* check for a name and a Req[0] on the job before we even try */

  const char *FName = "MJobTransition";

  MDB(7,fTRANS) MLog("%s(%s,%s,%s)\n",
    FName,
    (MUStrIsEmpty(J->Name)) ? "NULL" : J->Name,
    MBool[LimitedTransition],
    MBool[DeleteExistingJob]);

  if ((!MJobPtrIsValid(J)) || (J->Req[0] == NULL))
    {
    MDB(7,fTRANS) MLog("INFO:     Cannot transition job with empty name or null req.\n");

    return(SUCCESS);
    }

  if (bmisset(&MSched.DisplayFlags,mdspfUseBlocking))
    return(SUCCESS);

#if defined (__NOMCOMMTHREAD)
  return(SUCCESS);
#endif

  MJobTransitionAllocate(&TJ);
  
  if (MJobToTransitionStruct(J,TJ,LimitedTransition) == FAILURE)
    {
    MDB(3,fTRANS) MLog("ERROR:   failed to transition job '%s'\n",
      J->Name);

    MJobTransitionFree((void **)&TJ);

    return(FAILURE);
    }

  /* if there is a job in the database with the same SRMJID
     then delete that job before writing this new copy of the job */

  if (DeleteExistingJob == TRUE)
    bmset(&TJ->TFlags,mtransfDeleteExisting);

  MDB(7,fTRANS) MLog("INFO:     transitioning job '%s'\n",TJ->Name);

  MOExportTransition(mxoJob,TJ);

  /* if the job is active and has a valid node list, transition the nodes in
   * its node list */

  if (((MJOBISCOMPLETE(J)) || (MJOBISACTIVE(J))) &&
      (!MNLIsEmpty(&J->NodeList)))
    {
    mnode_t *N;

    int nindex;

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS; nindex++)
      {
      MNodeTransition(N);
      }
    }

  return(SUCCESS);
  }  /* END MJobTransition() */



/**
 * Sees if the trans job fits the given constraints list
 *
 * @param J
 * @param ConstraintList
 */

int MJobTransitionFitsConstraintList(

  mtransjob_t *J,              /* I */
  marray_t    *ConstraintList) /* I */

  {
  int CIndex;
  mjobconstraint_t *JConstraint;

  if (J == NULL)
    return(FAILURE);

  if (ConstraintList == NULL)
    {
    /* No constraints, return success */

    return(SUCCESS);
    }

  for (CIndex = 0; CIndex < ConstraintList->NumItems;CIndex++)
    {
    JConstraint = (mjobconstraint_t *)MUArrayListGet(ConstraintList,CIndex);

    switch (JConstraint->AType)
      {
      case mjaPAL:

        {
        if ((J->ActivePartition > 0) &&
            (J->ActivePartition != JConstraint->ALong[0]))
          {
          return(FAILURE);
          }
        else if (!MJOBSISACTIVE(J->State) &&
                 !MSTATEISCOMPLETE(J->State) &&
                 (!bmisset(&J->PAL,JConstraint->ALong[0])))
          {
          return(FAILURE);
          }
        }

        break;

      case mjaUser:

        if (MUStrIsEmpty(J->User) || strcmp(J->User,JConstraint->AVal[0]))
          return(FAILURE);

        break;

      case mjaAccount:

        if (MUStrIsEmpty(J->Account) || strcmp(J->Account,JConstraint->AVal[0]))
          return(FAILURE);

        break;

      case mjaQOS:

        if (MUStrIsEmpty(J->QOS) || strcmp(J->QOS,JConstraint->AVal[0]))
          return(FAILURE);

        break;

      case mjaClass:

        if (MUStrIsEmpty(J->Class) || strcmp(J->Class,JConstraint->AVal[0]))
          return(FAILURE);

        break;

      case mjaGroup:

        if (MUStrIsEmpty(J->Group) || strcmp(J->Group,JConstraint->AVal[0]))
          return(FAILURE);

        break;

      default:

        break;
      } /* END switch (JConstraint->AType) */
    } /* END for (CIndex = 0; CIndex < ConstraintList->NumItems;CIndex++) */

  return(SUCCESS);
  } /* END MJobTransitionFitsConstraintList() */
    

/**
 * Sees if the trans job is blocked
 *
 * Returns TRUE if transjob is blocked because of any of these:
 * 1) J->State is blocked
 * 2) J->State is completed
 * 3) J is blocked on all partitions
 * 4) J is blocked on "Par" partition  (showq -p)
 *
 * @param J (I)
 * @param Par (I)  [optional]
 */

mbool_t MJobTransitionJobIsBlocked(

  mtransjob_t *J,
  mpar_t      *Par)

  {

  if (NULL == J)
    return(FALSE);

  if (MJOBSISBLOCKED(J->State))
    return(TRUE);

  if (MSTATEISCOMPLETE(J->State))
    {
    /* jobs that are complete are not blocked, even if they have BlockReason */

    return(FALSE);
    }

   
  if (MUStrIsEmpty(J->BlockReason) == FALSE)
    {
    
    /* if we are doing PerPartitionScheduling, we need to parse the BlockReason to see if
     * we are blocked on all partitions.  If there are partitions that are not blocked
     * then the whole job is not blocked. */
    if (MSched.PerPartitionScheduling == TRUE) 
      {
      int pindex;
      char tmpLine[MMAX_LINE];

      for (pindex = 0;pindex < MMAX_PAR;pindex++)
        { 
        if (MPar[pindex].Name[0] == '\0')
          break;

        if (MPar[pindex].Name[0] == '\1')
          continue;

        if (!bmisset(&J->PAL,pindex))
          continue;

        if ((Par != NULL) && (Par->Index != pindex))
          continue;

        sprintf(tmpLine,"%s:",MPar[pindex].Name);
        if (strstr(J->BlockReason,tmpLine) == NULL)
          {
          /* found a partition we aren't blocked in. */
          return(FALSE);
          }

        } 
        /* blocked in all partitions */
        return(TRUE);
      }  /* END PerPartitionScheduling */

    /* __MJobTransitionSetState() for some reason sets the BlockReason to State:..... for JIRA ticket 724
     * We should not consider the job blocked if it has been set to this string */ 
    if (strstr(J->BlockReason,"State:") == NULL)
      {
      return(TRUE);
      }

    }  /* END J->BlockReason non-empty */

  return(FALSE);
  } /* END MJobTransitionJobIsBlocked() */



/**
 * Processes a resource manager extension string and populates the mtransjob_t struct.
 *
 * FORMAT:  <ATTR>:<VALUE>[{;?}<ATTR>:<VALUE>]... 
 *
 * @see MJobProcessExtensionString
 *
 * @param J (I)
 * @param RMXString (I)
 */

int MJobTransitionProcessRMXString(

  mtransjob_t *J,
  char        *RMXString)

  {
  const char *DelimList = ";?\n";  /* field delimiter */
  char *currAttr;
  char *TokPtr = 0;
  char *valPtr;
  char *tmpRMXString = NULL;
  enum MXAttrType aindex;

  MASSERT(J != NULL,"bad job pointer when processing a transition's rmxstring.");

  MUStrDup(&tmpRMXString,RMXString);

  currAttr = MUStrTok(tmpRMXString,DelimList,&TokPtr);

  while (currAttr != NULL)
    {
    aindex = (enum MXAttrType)MUGetIndexCI(currAttr,MRMXAttr,MBNOTSET,mxaNONE);

    valPtr = currAttr + strlen(MRMXAttr[aindex]) + 1; /* ATTR{:|=}VALUE */

    switch (aindex)
      {
      case mxaSID:

        MUStrDup(&J->SystemID,valPtr);

        break;

      case mxaSJID:
        
        MUStrDup(&J->GridJobID,valPtr);

        break;

      default:

        /* NOT HANDLED */

        break;
      } /* END switch (aindex) */
  
    currAttr = MUStrTok(NULL,DelimList,&TokPtr);
    } /* while (currAttr != NULL) */

  MUFree(&tmpRMXString);

  return(SUCCESS);
  } /* END MJobTransitionProcessRMXString() */

