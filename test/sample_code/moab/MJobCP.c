/* HEADER */

      
/**
 * @file MJobCP.c
 *
 * Contains:
 *    MJob Check Point functions
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Takes a job structure and expresses its data in an XML format in preparation for
 * saving it in a persistent checkpoint file.
 *
 * NOTE:  used for checkpointing non-completed jobs.
 *
 * @see MCPStoreCJobs()->MCJobStoreCP() - store completed jobs
 * @see MJobToXML() - encapsulate job structure
 * @see MJobLoadCP() - peer
 *
 * @param J (I) The job to convert to XML for checkpointing.
 * @param String (O) The buffer where the XML will be saved.
 * @param StoreComplete (I) Whether or not a complete job XML string should be created or a smaller, more
 * brief version should be output.
 */

int MJobStoreCP(

  mjob_t    *J,             /* I */
  mstring_t *String,        /* I/O */
  mbool_t    StoreComplete) /* I */

  {
  /* below enums define the fields to be included in the brief job
   * checkpoint XML */

  enum MJobAttrEnum DeltaJAList[] = {
    mjaAccount,
    mjaAllocVMList,
    mjaAWDuration,
    mjaClass,
    mjaCmdFile,
    mjaDepend,
    mjaEEWDuration,
    mjaFlags,
    mjaGeometry,
    mjaGJID,
    mjaHold,
    mjaHoldTime,
    mjaIWD,
    mjaJobID,
    mjaJobGroup,
    mjaJobGroupArrayIndex,
    mjaJobName,
    mjaLastChargeTime,
    mjaMessages,
    mjaMinWCLimit,
    mjaOMinWCLimit,
    mjaOWCLimit,
    mjaPAL,
    mjaQOS,
    mjaQOSReq,
    mjaReqHostList,    /* added to preserve hostlist of system jobs */
    mjaDRM,
    mjaDRMJID,
    mjaRM,
    mjaServiceJob,
    mjaSID,
    mjaSRMJID,
    mjaStorage,
    mjaSubmitHost,
    mjaReqAWDuration,
    mjaReqReservation,
    mjaReqReservationPeer,
    mjaReqSMinTime,
    mjaRMXString,
    mjaStartCount,
    mjaState,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaSubmitLanguage,
    mjaSubmitString,
    mjaSubmitTime,
    mjaSuspendDuration,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaTemplateFlags,
    mjaTemplateSetList,
#ifdef MYAHOO
    mjaTrigger,
    mjaTrigVariable,
#endif /* MYAHOO */
    mjaTrigNamespace,
    mjaTTC,
    mjaUMask,
    mjaUser,
    mjaUserPrio,
    mjaVariables,
    mjaVM,
    mjaVMCR,
    mjaVMUsagePolicy,
    mjaVWCLimit,
    mjaNONE,
    mjaNONE,
    mjaNONE };

  int DeltaJAListInsertIndex = (sizeof(DeltaJAList) / sizeof(DeltaJAList[0])) - 3;

  enum MReqAttrEnum DeltaRQAList[] = {
    mrqaAllocNodeList,  /* is this here for multi-req jobs? */
    mrqaNodeAccess,
    mrqaIndex,
    mrqaTCReqMin,
    mrqaReqArch,
    mrqaReqOpsys, /* added to preserve OS of system jobs */
    mrqaReqNodeFeature,
    mrqaGRes,
    mrqaNONE };

  static enum MJobAttrEnum FullJAList[mjaLAST + 1] = { mjaNONE };

  static enum MReqAttrEnum FullRQAList[mrqaLAST + 1] = { mrqaNONE };

  /* NOTE:  need to be able to checkpoint attributes according to job specific 
            attribute ownership.  IE, only checkpoint scheduler owned 
            attributes where attribute ownership varies according to resource 
            manager capabilities */

  /* NOTE:  possible implementation:  
              for each RM, determine list of attributes report 
              for each job 
                for each rm
                  mark job attributes owned 
                checkpoint unmarked attributes
  */

  mxml_t *JE = NULL;


  int     aindex;

  const char *FName = "MJobStoreCP";

  MDB(4,fCKPT) MLog("%s(%s,string,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (StoreComplete == TRUE) ? "TRUE" : "FALSE");

  if ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->Flags,mrmfFullCP)))
    StoreComplete = TRUE;

#if 0
  if ((J->VMUsagePolicy == mvmupVMCreate) || 
     ((J->System != NULL) && (J->System->JobType == msjtVMCreate)))
#endif
  if (J->VMUsagePolicy == mvmupVMCreate)
    {
    DeltaJAList[DeltaJAListInsertIndex++] = mjaAllocNodeList;
    DeltaJAList[DeltaJAListInsertIndex++] = mjaTaskMap;
    }

  if (FullJAList[0] == mjaNONE)
    {
    for (aindex = 0;aindex != mjaLAST;aindex++)
      {
      FullJAList[aindex] = (enum MJobAttrEnum)(aindex + 1);
      }
 
    FullJAList[aindex] = mjaNONE;
    }

  if (FullRQAList[0] == mrqaNONE)
    {
    for (aindex = 0;aindex != mrqaLAST;aindex++)
      {
      FullRQAList[aindex] = (enum MReqAttrEnum)(aindex + 1);
      }

    FullRQAList[aindex] = mrqaNONE;
    }

  if ((J == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  String->clear();

  MMBPurge(&J->MessageBuffer,NULL,-1,MSched.Time);

  if (MJobToXML(
    J,
    &JE,
    (StoreComplete == TRUE) ? 
      (enum MJobAttrEnum *)FullJAList : 
      (enum MJobAttrEnum *)DeltaJAList,
    (StoreComplete == TRUE) ?
      (enum MReqAttrEnum *)FullRQAList :
      (enum MReqAttrEnum *)DeltaRQAList,
    NULL,
    TRUE,
    FALSE) == FAILURE)
    {
    MDB(1,fCKPT) MLog("ALERT:    failed to convert job '%s' to XML\n",
      J->Name);
    }

  if (MXMLToMString(JE,String,NULL,TRUE) == FAILURE)
    {
    String->clear();
  
    MXMLDestroyE(&JE);

    MDB(1,fCKPT) MLog("ALERT:    failed to convert job '%s' from XML to string\n",
      J->Name);

    return(FAILURE);
    }

  if ((MSched.EnableHighThroughput == FALSE) && (MSched.LimitedJobCP == FALSE))
    {
    /* NOTE:  routine below may be CPU-intensive - remove when possible */

    if (MSysVerifyTextBuf(String->c_str(),(int) String->size(),"MJobStoreCP",J->Name) == FAILURE)
      {
      String->clear();

      MXMLDestroyE(&JE);

      return(FAILURE);
      }
    }

  MXMLDestroyE(&JE);

  return(SUCCESS);
  }  /* END MJobStoreCP() */




/**
 * Writes job checkpoint to checkpoint file in spool directory.
 *
 * @param J (I) Job to write a checkpoint file for.
 */

int MJobWriteCPToFile(

  mjob_t *J) /* I */

  {
  if (J == NULL)
    return(FAILURE);

  mstring_t JobCPString(MMAX_LINE);

  if ((MJobStoreCP(J,&JobCPString,TRUE) == FAILURE) ||
      (MCPPopulateJobCPFile(J,JobCPString.c_str(),FALSE) == FAILURE))
    {
    MDB(1,fRM) MLog("ERROR:    failed to create checkpoint file for job '%s'\n",
        J->Name);

    return(FAILURE);
    }

  return(SUCCESS);
  } /* END int MJobWriteCPToFile() */





/**
 * This new routine will directly populate the REAL job with the XML from the checkpoint.  
 *
 * NOTE: this routine overwrites whatever is on the REAL job and basically assumes the job
 *       is "virgin".  This routine is now called BEFORE M{PBS|WIKI|MM}JobLoad and loads
 *       the job from checkpoint before we load it from the RM.  The RM will then overwrite
 *       whatever it wants, just like it always has.
 *
 * @see MCPRestore() - parent
 * @see MRMLoadJobCP() - parent
 * @see MJobStoreCP() - peer
 *
 * @param DestinationJ (I) If NULL then MJobFind will try to find the REAL job or fail.
 * @param SBuf (I) Line from the checkpoint file.
 */

int MJobLoadCPNew(

  mjob_t *DestinationJ,
  const char   *SBuf)

  {
  char     tmpName[MMAX_NAME + 1];
  char     JobID[MMAX_NAME + 1];
  char     EMsg[MMAX_LINE];

  long     CkTime;

  const char    *ptr;

  mxml_t  *E = NULL;  

  int      rc;

  const char *FName = "MJobLoadCP";

  MDB(4,fCKPT) MLog("%s(%s,%s)\n",
    FName,
    (DestinationJ != NULL) ? "DestinationJob" : "NULL",
    (SBuf != NULL) ? SBuf : "NULL");

  if (SBuf == NULL)
    {
    return(FAILURE);
    }

  /* FORMAT:  <JOBHEADER> <JOBID> <CKTIME> <JOBSTRING> */

  /* determine job ID */
 
  rc = sscanf(SBuf,"%64s %64s %ld",
    tmpName,
    JobID,
    &CkTime);
 
  if (rc != 3)
    {
    return(FAILURE); /* we must at least have the header, jobid, and cktime */
    }

  /* if this job is older than the checkpoint expiration time then we are done */

  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }
 
  if (DestinationJ == NULL)
    {
    if (MJobFind(JobID,&DestinationJ,mjsmBasic) != SUCCESS)
      {
      MDB(5,fCKPT) MLog("INFO:     job '%s' no longer detected\n",
        JobID);

      return(SUCCESS);
      }
    }

  /* Make sure that the checkpoint line has an XML style JOBSTRING */

  ptr = strchr(SBuf,(int) '<');
  if (ptr == NULL)
    {
    return(FAILURE);
    }

  /* Parse the XML style JOBSTRING and load the parsed attributes into the local job object */

  /* NOTE: MJobFromXML() allocs DestinationJ->Req[0] */

  if ((MXMLFromString(&E,ptr,NULL,EMsg) == FAILURE) ||
      (MJobFromXML(DestinationJ,E,FALSE,mNONE2) == FAILURE))   
    {
    MXMLDestroyE(&E);

    return(FAILURE);
    }

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MJobLoadCPNew() */







/**
 * NOTE: This routine is no longer called but is kept around for legacy purposes.
 *       We may need to examine this routine from time to time as we shake out bugs
 *       related to the new checkpoint-restore model.
 *
 * This function parses a job entry line from a checkpoint file (Buf) and
 * optionally populates a job object using the job information in the supplied
 * checkpoint line. 
 *
 * Note that if a NULL job object pointer is supplied, then
 * this routine looks for the job specified in the checkpoint
 * line and if it exists it parses the XML job information from 
 * the supplied checkpoint line. If the job no longer exists, 
 * SUCCESS is returned, or if the job is found and the
 * XML information can be parsed then SUCCESS is returned.
 * If the supplied job object pointer is not NULL then a local 
 * job object is initialized and filled in with the XML
 * information from the supplied checkpoint file line. If the 
 * job name in the supplied job object is empty then the local job 
 * object is memcpy'd to the supplied job object (possibly clobbering 
 * allocated job attributes), otherwise, specific CP fields are 
 * copied from the local job object to the supplied job object.
 *
 * @see MCPRestore() - parent
 * @see MRMLoadJobCP() - parent
 * @see MJobStoreCP() - peer
 *
 * @param DestinationJ (I) The job object to be populated. NULL if we are to just lookup the job in the checkpoint file. [optional,job to populate]
 * @param SBuf (I) Line from the checkpoint file.
 */

int MJobLoadCPOld(

  mjob_t *DestinationJ,
  char   *SBuf)

  {
  int      ReturnCode = SUCCESS;
  int      rqindex;

  char     tmpName[MMAX_NAME + 1];
  char     JobID[MMAX_NAME + 1];
  char     EMsg[MMAX_LINE];
  char     ParLine[MMAX_LINE];

  long     CkTime;

  char    *ptr;

  mxml_t  *E = NULL;  

  mjob_t  *tmpXMLJ;

  int      rc;

  mbool_t  LocalJob;

  const char *FName = "MJobLoadCP";

  MDB(4,fCKPT) MLog("%s(%s,%s)\n",
    FName,
    (DestinationJ != NULL) ? "DestinationJob" : "NULL",
    (SBuf != NULL) ? SBuf : "NULL");

  if (SBuf == NULL)
    {
    return(FAILURE);
    }

  /* FORMAT:  <JOBHEADER> <JOBID> <CKTIME> <JOBSTRING> */

  /* determine job ID */
 
  rc = sscanf(SBuf,"%64s %64s %ld",
    tmpName,
    JobID,
    &CkTime);
 
  if (rc != 3)
    {
    return(FAILURE); /* we must at least have the header, jobid, and cktime */
    }

  /* if this job is older than the checkpoint expiration time then we are done */

  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }
 
  if (DestinationJ == NULL)
    {
    if (MJobFind(JobID,&tmpXMLJ,mjsmBasic) != SUCCESS)
      {
      MDB(5,fCKPT) MLog("INFO:     job '%s' no longer detected\n",
        JobID);

      return(SUCCESS);
      }

    LocalJob = FALSE;
    }
  else
    {
    MJobCreate(JobID,FALSE,&tmpXMLJ,NULL);

    bmset(&tmpXMLJ->IFlags,mjifTemporaryJob);
    MJobInitialize(tmpXMLJ);
    LocalJob = TRUE;
    }

  /* Make sure that the checkpoint line has an XML style JOBSTRING */

  if ((ptr = strchr(SBuf,'<')) == NULL)
    {
    return(FAILURE);
    }

  /* Parse the XML style JOBSTRING and load the parsed attributes into the local job object */

  /* NOTE: MJobFromXML() allocs tmpXMLJ->Req[0] */

  if ((MXMLFromString(&E,ptr,NULL,EMsg) == FAILURE) ||
      (MJobFromXML(tmpXMLJ,E,FALSE,mNONE2) == FAILURE))   
    {
    MXMLDestroyE(&E);
    ReturnCode = FAILURE;
    goto free_and_return;
    }

  MXMLDestroyE(&E);

  /* If we were given a job structure to fill in then proceed, otherwise we are done */

  if (DestinationJ == NULL)
    {
    goto free_and_return;
    }

  /* Fill in the SourceJ object */

  if (MUStrIsEmpty(DestinationJ->Name))
    {
    MJobDuplicate(DestinationJ,tmpXMLJ,TRUE);
    }
  else
    {
    int tindex;
    int CRQCount;  /* number of compute reqs */

    /* update CP fields only */
    /* NOTE: there may be many other CP fields that need to be copied from tmpXMLJ to DestinationJ !!! */

    if (bmcompare(&tmpXMLJ->Hold,&DestinationJ->Hold) == 0)
      bmunset(&DestinationJ->IFlags,mjifNewBatchHold);

    if (DestinationJ->DestinationRM == NULL)
      DestinationJ->DestinationRM = tmpXMLJ->DestinationRM;

    if (DestinationJ->SubmitRM == NULL)
      DestinationJ->SubmitRM = tmpXMLJ->SubmitRM;

    if (tmpXMLJ->State != mjsNONE)
      {
      if (DestinationJ->DestinationRM != NULL)
        {
        MDB(7,fSCHED) MLog("INFO:     job %s RM state %s, RM type %s, NC %d, TC %d in MJobLoadCP\n", 
          DestinationJ->Name,
          MRMState[DestinationJ->DestinationRM->State],
          MRMType[DestinationJ->DestinationRM->Type],
          DestinationJ->DestinationRM->NC,
          DestinationJ->DestinationRM->TC);
        }

      if  (((DestinationJ->DestinationRM != NULL) && (DestinationJ->DestinationRM->State != mrmsActive)) ||
           ((DestinationJ->DestinationRM != NULL) && (DestinationJ->DestinationRM->Type == mrmtMoab) && (DestinationJ->DestinationRM->NC == 0) && (DestinationJ->DestinationRM->TC == 0)))
        {
        /* only restore checkpointed state if destination RM is not reporting or
           slave moab is up but the native resource manager is down */

        DestinationJ->State = tmpXMLJ->State;

        MDB(7,fSCHED) MLog("INFO:     job %s setting state to %s in MJobLoadCP\n", 
          DestinationJ->Name,
          MJobState[DestinationJ->State]);
        }
      else 
        {
        MDB(7,fSCHED) MLog("INFO:     job %s state defaults to %s in MJobLoadCP\n", 
          DestinationJ->Name,
          MJobState[DestinationJ->State]);
        }
      }    /* END if (tmpXMLJ->State != mjsNONE) */

    MJobSetAttr(DestinationJ,mjaReqSMinTime,(void **)&tmpXMLJ->SpecSMinTime,mdfLong,mSet);

    DestinationJ->Hold             = tmpXMLJ->Hold;
    DestinationJ->HoldTime         = tmpXMLJ->HoldTime;

    DestinationJ->LastChargeTime   = tmpXMLJ->LastChargeTime;

    /* the below variables may have already been set by RM, which
     * takes precendence over the checkpoint file for this particular data */

    MASSIGNIFZERO(DestinationJ->CompletionCode,tmpXMLJ->CompletionCode);
    MASSIGNIFZERO(DestinationJ->CompletionTime,tmpXMLJ->CompletionTime);

    MASSIGNIFZERO(DestinationJ->StartCount,tmpXMLJ->StartCount);

    DestinationJ->QOSRequested             = tmpXMLJ->QOSRequested;
    DestinationJ->SystemPrio       = tmpXMLJ->SystemPrio;
    DestinationJ->UPriority        = tmpXMLJ->UPriority;

    if (tmpXMLJ->System != NULL)
      {
      if ((DestinationJ->System != NULL) && (DestinationJ->System != tmpXMLJ->System))
        MJobFreeSystemStruct(DestinationJ);

      DestinationJ->System  = tmpXMLJ->System;
      tmpXMLJ->System = NULL;
      }

    if (tmpXMLJ->VMCreateRequest != NULL)
      {
      if ((DestinationJ->VMCreateRequest != NULL) && (DestinationJ->VMCreateRequest != tmpXMLJ->VMCreateRequest))
        MUFree((char **)&DestinationJ->VMCreateRequest);

      DestinationJ->VMCreateRequest = tmpXMLJ->VMCreateRequest;
      tmpXMLJ->VMCreateRequest = NULL;
      }

    if (tmpXMLJ->TemplateExtensions != NULL)
      {
      if ((DestinationJ->TemplateExtensions != NULL) && (DestinationJ->TemplateExtensions != tmpXMLJ->TemplateExtensions))
        MJobDestroyTX(DestinationJ);

      DestinationJ->TemplateExtensions = tmpXMLJ->TemplateExtensions;
      tmpXMLJ->TemplateExtensions = NULL;
      }

    DestinationJ->VMUsagePolicy = tmpXMLJ->VMUsagePolicy;

    DestinationJ->MinWCLimit       = tmpXMLJ->MinWCLimit;
    DestinationJ->OrigMinWCLimit   = tmpXMLJ->OrigMinWCLimit;
    if ((tmpXMLJ->MinWCLimit > 0) && (MSched.JobExtendStartWallTime == TRUE))
      {
      DestinationJ->OrigWCLimit = tmpXMLJ->OrigWCLimit;
      }

    DestinationJ->TotalTaskCount              = tmpXMLJ->TotalTaskCount;

    /* set hold time to a sane value (may not be needed anymore) */

    if (bmisset(&DestinationJ->Hold,mhDefer) && (DestinationJ->HoldTime == 0))
      DestinationJ->HoldTime = MSched.Time;

    DestinationJ->PSDedicated      = tmpXMLJ->PSDedicated;
    DestinationJ->PSUtilized       = tmpXMLJ->PSUtilized;
    DestinationJ->MSUtilized       = tmpXMLJ->MSUtilized;
    DestinationJ->AWallTime        = tmpXMLJ->AWallTime;      

    if (tmpXMLJ->EffQueueDuration > 0)
      {
      DestinationJ->EffQueueDuration = tmpXMLJ->EffQueueDuration;
      DestinationJ->SystemQueueTime  = MSched.Time - tmpXMLJ->EffQueueDuration;
      }

    if ((DestinationJ->Request.TC == 1) && (tmpXMLJ->Request.TC > 1))
      DestinationJ->Request.TC = tmpXMLJ->Request.TC;

    bmunset(&DestinationJ->IFlags,mjifTaskCountIsDefault);

    if ((DestinationJ->Request.NC == 0) && (tmpXMLJ->Request.NC > 0))
      DestinationJ->Request.NC = tmpXMLJ->Request.NC;

    if (tmpXMLJ->MessageBuffer != NULL)
      {
      MMBCopyAll(&DestinationJ->MessageBuffer,tmpXMLJ->MessageBuffer);
      }

    if (!MNLIsEmpty(&tmpXMLJ->ReqHList))
      {
      MNLCopy(&DestinationJ->ReqHList,&tmpXMLJ->ReqHList);

      bmset(&DestinationJ->IFlags,mjifHostList);

      MNLClear(&tmpXMLJ->ReqHList);
      }

    if ((DestinationJ->SpecWCLimit[0] == 0) && (tmpXMLJ->SpecWCLimit[0] != 0))
      {
      /* load from checkpoint only if job data not initialized */

      DestinationJ->SpecWCLimit[0] = tmpXMLJ->SpecWCLimit[0];
      DestinationJ->SpecWCLimit[1] = tmpXMLJ->SpecWCLimit[1];
      }

    if ((DestinationJ->System != NULL) && (DestinationJ->System->WCLimit != 0))
      {
      /* Set the walltime based on the system walltime using the system job. */

      DestinationJ->SpecWCLimit[0] = DestinationJ->System->WCLimit;
      DestinationJ->SpecWCLimit[1] = DestinationJ->System->WCLimit;
      }

    DestinationJ->Env.UMask = tmpXMLJ->Env.UMask;

    if (bmisset(&tmpXMLJ->SpecFlags,mjfMeta))
      {
      /* MOAB-1249: vmtracking jobs are not being restored correctly.
       * Doug adds code to just copy the XML job's structures onto the
       * resulting job.  Why don't we do this for all jobs? */

      /* NOTE that this effectively disables all the rest of the Req
       * code as tmpXMLJ->Req[0] will be NULL after all this. */

      for (rqindex = 0;tmpXMLJ->Req[rqindex] != NULL;rqindex++)
        {
        MMovePtr((char **)&tmpXMLJ->Req[rqindex],(char **)&DestinationJ->Req[rqindex]); 
        }
      }

    /* update empty fields in existing job */

    if (DestinationJ->Req[0] == NULL)
      {
      for (rqindex = 0; tmpXMLJ->Req[rqindex] != NULL; rqindex++)
        {
        MMovePtr((char **)&tmpXMLJ->Req[rqindex],(char **)&DestinationJ->Req[rqindex]); 
        }
      }

    if ((tmpXMLJ->Req[0] != NULL) && (tmpXMLJ->Req[0]->XURes != NULL))
      {
      MMovePtr((char **)&tmpXMLJ->Req[0]->XURes,(char **)&DestinationJ->Req[0]->XURes);
      }

    CRQCount = 0;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (tmpXMLJ->Req[rqindex] == NULL)
        break;

      if (tmpXMLJ->Req[rqindex]->DRes.Procs > 0)
        CRQCount++;
      }  /* END for (rqindex) */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (tmpXMLJ->Req[rqindex] == NULL)
        break;

      if ((DestinationJ->Req[rqindex] == NULL) &&
          (MReqCreate(DestinationJ,NULL,&DestinationJ->Req[rqindex],FALSE) == FAILURE))
        {
        /* cannot allocate memory or MMAX_REQ reached */
        ReturnCode = FAILURE;
        goto free_and_return;
        }

      /* if the checkpoint file contained a nodeaccesspolicy for this
         request, add it to the job */

      if (tmpXMLJ->Req[rqindex]->NAccessPolicy != mnacNONE)
        {
        DestinationJ->Req[rqindex]->NAccessPolicy = tmpXMLJ->Req[rqindex]->NAccessPolicy;
        }

      /* NOTE:  load req node list to 'remember' node-to-req assignment */
      /*        This is only required for multi-compute-req jobs */

      /* NOTE:  for multi-compute req jobs, Moab should verify that the job's */
      /*        node allocation has not changed.  If it has changed, the */
      /*        checkpointed allocation should probably be ignored (NYI) */
    
      /* NOTE:  We want to remember any GLOBAL node assignments in the node list */

      if ((MNLIsEmpty(&DestinationJ->Req[rqindex]->NodeList)) && 
          (!MNLIsEmpty(&tmpXMLJ->Req[rqindex]->NodeList)) &&
          (MNLReturnNodeAtIndex(&tmpXMLJ->Req[rqindex]->NodeList,0) == MSched.GN))
        {
        /* don't load nodelist for single req jobs */

        MNLClear(&DestinationJ->Req[rqindex]->NodeList);

        MNLCopy(&DestinationJ->Req[rqindex]->NodeList,&tmpXMLJ->Req[rqindex]->NodeList);
        }

      /* copy over any GRes attributes read in from the checkpoint file */

      if (!MSNLIsEmpty(&tmpXMLJ->Req[rqindex]->DRes.GenericRes))
        {
        MSNLCopy(&DestinationJ->Req[rqindex]->DRes.GenericRes,&tmpXMLJ->Req[rqindex]->DRes.GenericRes);
        }
      
      DestinationJ->Req[rqindex]->Opsys = tmpXMLJ->Req[rqindex]->Opsys;

      DestinationJ->Req[rqindex]->Arch = tmpXMLJ->Req[rqindex]->Arch;

      /* remember the requested job features */
      bmcopy(&DestinationJ->Req[0]->ReqFBM,&tmpXMLJ->Req[0]->ReqFBM);
      }   /* END for (rqindex) */

    /* copy scheduler features */

    bmcopy(&DestinationJ->ReqFeatureGResBM,&tmpXMLJ->ReqFeatureGResBM);

    /* Copy the tmpXMLJ->NodeList */

    if ((MNLIsEmpty(&DestinationJ->NodeList)) && (!MNLIsEmpty(&tmpXMLJ->NodeList)))
      {
      MNLClear(&DestinationJ->NodeList);

      MNLCopy(&DestinationJ->NodeList,&tmpXMLJ->NodeList);
      }

    if (DestinationJ->Credential.U == NULL)
      {
      DestinationJ->Credential.U = tmpXMLJ->Credential.U;
      DestinationJ->Credential.G = tmpXMLJ->Credential.G;
      }

    if (!MNLIsEmpty(&DestinationJ->NodeList) &&
        !MNLIsEmpty(&tmpXMLJ->NodeList))
      {
      MNLClear(&DestinationJ->NodeList);

      MNLCopy(&DestinationJ->NodeList,&tmpXMLJ->NodeList);
      }

    if ((DestinationJ->TaskMapSize > 0) &&
        (DestinationJ->TaskMap[0] == -1) &&
        (tmpXMLJ->TaskMapSize > 0) &&
        (tmpXMLJ->TaskMap[0] != -1))
      {
      MUFree((char **)&DestinationJ->TaskMap);

      MMovePtr((char **)&tmpXMLJ->TaskMap,(char **)&DestinationJ->TaskMap);

      DestinationJ->TaskMapSize = tmpXMLJ->TaskMapSize;
      }

    /* if set in ckpt file, override any defaults that may be in force for 
       credentials */

    /* should we also check if class is set before overriding (???) */

    DestinationJ->Credential.C = tmpXMLJ->Credential.C;

    if (tmpXMLJ->Credential.A != NULL)
      DestinationJ->Credential.A = tmpXMLJ->Credential.A;

    if (tmpXMLJ->Credential.Q != NULL)
      {
      /* Checkpoint file is authoritative, lock the QOS */

      MJobSetQOS(DestinationJ,tmpXMLJ->Credential.Q,0);

      bmset(&DestinationJ->IFlags,mjifQOSLocked);

      if (bmisset(&DestinationJ->Credential.Q->Flags,mqfProvision) && (DestinationJ->Req[0]->Opsys != 0))
        {
        bmset(&DestinationJ->IFlags,mjifMaster);

#if 0
        DestinationJ->Req[0]->NAccessPolicy = mnacSingleJob;
#endif
        }
      }

    if ((bmisclear(&DestinationJ->SpecFlags)) || (!bmisclear(&tmpXMLJ->SpecFlags)))
      {
      /* this overwrites flags retrieved from the RM */

      mbitmap_t OrigSpecFlags;

      bmcopy(&OrigSpecFlags,&DestinationJ->SpecFlags);

      bmcopy(&DestinationJ->SpecFlags,&tmpXMLJ->SpecFlags);
      bmcopy(&DestinationJ->Flags,&tmpXMLJ->SpecFlags);

      if (bmisset(&OrigSpecFlags,mjfRestartable))
        {
        bmset(&DestinationJ->SpecFlags,mjfRestartable);
        bmset(&DestinationJ->Flags,mjfRestartable);
        }

      /* NOTE:  removed when mjfHostList moved to mjifHostList */

      /*
      if (bmisset(&OrigSpecFlags,mjfHostList))
        {
        bmset(&DestinationJ->SpecFlags,mjfHostList);
        bmset(&DestinationJ->Flags,mjfHostList);
        }
      */
      }

    bmcopy(&DestinationJ->SpecPAL,&tmpXMLJ->SpecPAL);

    if (MUStrStr(MPALToString(&DestinationJ->SpecPAL,NULL,ParLine),"internal",0,TRUE,FALSE) != NULL)
      {
      MDB(3,fCONFIG) MLog("INFO:     job %s SpecPAL (pmask) set to %s from checkpoint\n",
        DestinationJ->Name,
        MPALToString(&DestinationJ->SpecPAL,NULL,ParLine));
      }
                          
    /* If features can be mapped to partitions then check to see if 
       any requested features map to a partition and change the
       specified partition list to only include features which
       map to partitions. If no features map to a partition or
       no features are specified then do not change SpecPAL */

    if (MSched.MapFeatureToPartition == TRUE)
      {
      int NumFound = 0;
      mbitmap_t tmpPAL;

      MJobFeaturesInPAL(DestinationJ,&DestinationJ->SpecPAL,&tmpPAL,&NumFound);

      /* Only change the SpecPAL if feature(s) mapped to partition(s) */

      if (NumFound > 0)
        {
        bmcopy(&DestinationJ->SpecPAL,&tmpPAL);

        if (MUStrStr(MPALToString(&DestinationJ->SpecPAL,NULL,ParLine),"internal",0,TRUE,FALSE) != NULL)
          {
          MDB(3,fCONFIG) MLog("INFO:     job %s SpecPAL (pmask) set to %s (feature) from checkpoint\n",
            DestinationJ->Name,
            MPALToString(&DestinationJ->SpecPAL,NULL,ParLine));
          }
        }
      }

    MUStrDup(&DestinationJ->Env.IWD,tmpXMLJ->Env.IWD);
    MUStrDup(&DestinationJ->Env.Cmd,tmpXMLJ->Env.Cmd);
    MUStrDup(&DestinationJ->SubmitHost,tmpXMLJ->SubmitHost);

    if ((tmpXMLJ->SRMJID != NULL) && (DestinationJ->SRMJID == NULL))
      {
      MMovePtr(&tmpXMLJ->SRMJID,&DestinationJ->SRMJID);
      }

    if (tmpXMLJ->DRMJID != NULL)
      {
      if (DestinationJ->DRMJID != NULL)
        MUFree(&DestinationJ->DRMJID);

      MMovePtr(&tmpXMLJ->DRMJID,&DestinationJ->DRMJID);

      MUHTAdd(&MJobDRMJIDHT,DestinationJ->DRMJID,DestinationJ,NULL,NULL);  /* update HT with new index */
      }

    if (bmisset(&tmpXMLJ->IFlags,mjifTasksSpecified))
      bmset(&DestinationJ->IFlags,mjifTasksSpecified);

    if (bmisset(&tmpXMLJ->IFlags,mjifRunAlways))
      bmset(&DestinationJ->IFlags,mjifRunAlways);

    if ((tmpXMLJ->ReqRID != NULL) && (DestinationJ->ReqRID == NULL))
      {
      /* NOTE: if ReqRID has changed in moab.cfg (ie. QOSCFG[] USERESERVED) */
      /*       don't overwrite with .moab.ck */
      /*       moab.cfg loaded onto job BEFORE .moab.ck */

      MMovePtr(&tmpXMLJ->ReqRID,&DestinationJ->ReqRID);

      bmset(&tmpXMLJ->SpecFlags,mjfAdvRsv);
      bmset(&tmpXMLJ->Flags,mjfAdvRsv);
      }

    if (tmpXMLJ->Env.BaseEnv != NULL)
      MUStrDup(&DestinationJ->Env.BaseEnv,tmpXMLJ->Env.BaseEnv);

    if (tmpXMLJ->Env.IncrEnv != NULL)
      MUStrDup(&DestinationJ->Env.IncrEnv,tmpXMLJ->Env.IncrEnv);

    if (tmpXMLJ->Env.Input != NULL)
      MUStrDup(&DestinationJ->Env.Input,tmpXMLJ->Env.Input);

    if (tmpXMLJ->Env.Args != NULL)
      MUStrDup(&DestinationJ->Env.Args,tmpXMLJ->Env.Args);

    if (tmpXMLJ->Env.Shell != NULL)
      MUStrDup(&DestinationJ->Env.Shell,tmpXMLJ->Env.Shell);

    if (tmpXMLJ->Env.Output != NULL)
      MUStrDup(&DestinationJ->Env.Output,tmpXMLJ->Env.Output);

    if (tmpXMLJ->Env.Error != NULL)
      MUStrDup(&DestinationJ->Env.Error,tmpXMLJ->Env.Error);
  
    if (tmpXMLJ->Array != NULL)
      {
      if (DestinationJ->Array != NULL)
        MJobArrayFree(DestinationJ);

      MMovePtr((char **)&tmpXMLJ->Array,(char **)&DestinationJ->Array);
      }

    if (tmpXMLJ->Depend != NULL)
      {
      if (DestinationJ->Depend != NULL)
        MJobDependDestroy(DestinationJ);
  
      MMovePtr((char **)&tmpXMLJ->Depend,(char **)&DestinationJ->Depend);
      }

    if (tmpXMLJ->DependBlockReason != mdNONE)
      {
      DestinationJ->DependBlockReason = tmpXMLJ->DependBlockReason;
      }

    if (tmpXMLJ->DependBlockMsg != NULL)
      {
      MMovePtr(&tmpXMLJ->DependBlockMsg,&DestinationJ->DependBlockMsg);
      }

    if (tmpXMLJ->ImmediateDepend != NULL)
      {
      DestinationJ->ImmediateDepend = tmpXMLJ->ImmediateDepend;
      tmpXMLJ->ImmediateDepend = NULL;
      }

    if (tmpXMLJ->ExtLoad != NULL)
      MMovePtr((char **)&tmpXMLJ->ExtLoad,(char **)&DestinationJ->ExtLoad);

    if (tmpXMLJ->SystemJID != NULL)
      MUStrDup(&DestinationJ->SystemJID,tmpXMLJ->SystemJID);

    if (tmpXMLJ->RMXString != NULL)
      MUStrDup(&DestinationJ->RMXString,tmpXMLJ->RMXString);

    if (tmpXMLJ->Geometry != NULL)
      MUStrDup(&DestinationJ->Geometry,tmpXMLJ->Geometry);

    if (tmpXMLJ->RMSubmitString != NULL)
      MUStrDup(&DestinationJ->RMSubmitString,tmpXMLJ->RMSubmitString);

    if (tmpXMLJ->RMSubmitFlags != NULL)
      MUStrDup(&DestinationJ->RMSubmitFlags,tmpXMLJ->RMSubmitFlags);

    if (tmpXMLJ->TSets != NULL)
      {
      if (DestinationJ->TSets != NULL)
        {
        MULLFree(&DestinationJ->TSets,NULL);
        }

      MMovePtr((char **)&tmpXMLJ->TSets,(char **)&DestinationJ->TSets);
      }

    if (tmpXMLJ->Variables.Table != NULL)
      {
      MUHTFree(&DestinationJ->Variables,TRUE,MUFREE);
      memcpy(&DestinationJ->Variables,&tmpXMLJ->Variables,sizeof(mhash_t));
      memset(&tmpXMLJ->Variables,0x0,sizeof(mhash_t));
      }

    if (tmpXMLJ->SystemID != NULL)
      {
      MUStrDup(&DestinationJ->SystemID,tmpXMLJ->SystemID);

     /* NOTE: we eventually need to change this so the RM matches the SID */

      MRMGetInternal(&tmpXMLJ->SubmitRM);  /* why are we setting this in the temp job? */
      }

    if (tmpXMLJ->JGroup != NULL)
      {
      if (DestinationJ->JGroup != NULL)
        MJGroupFree(DestinationJ);

      MMovePtr((char **)&tmpXMLJ->JGroup,(char **)&DestinationJ->JGroup);
      }

    if (tmpXMLJ->AName != NULL)
      {
      if (DestinationJ->AName != NULL)
        MUFree(&DestinationJ->AName);

      MMovePtr((char **)&tmpXMLJ->AName,(char **)&DestinationJ->AName);

#if 0 /* disabled in 6.1 to use for DRMJID hash table */
      MUHTAdd(&MJobANameHT,DestinationJ->AName,MUIntDup(DestinationJ->Index),NULL,MUFREE);  /* update HT with new index */
#endif
      }

    DestinationJ->RMSubmitType = tmpXMLJ->RMSubmitType;
    DestinationJ->SubmitTime = tmpXMLJ->SubmitTime;

    DestinationJ->EffQueueDuration = tmpXMLJ->EffQueueDuration;

    MJobSetAttr(DestinationJ,mjaJobID,(void **)tmpXMLJ->Name,mdfString,mSet);

    /* copy data-staging information */

    if (tmpXMLJ->DataStage != NULL)
      {
      MDSCopy(&DestinationJ->DataStage,tmpXMLJ->DataStage);
      }  /* END if (tmpXMLJ->DS != NULL) */

    if (bmisset(&DestinationJ->SpecFlags,mjfNoRMStart))
      {
      DestinationJ->StartTime = tmpXMLJ->StartTime;

      DestinationJ->State = tmpXMLJ->State;
      /*
      if (DestinationJ->StartTime + tmpXMLJ->SpecWCLimit[0] <= MSched.Time)
        return(FAILURE);
      */
      }

    if (bmisset(&DestinationJ->SpecFlags,mjfGlobalQueue))
      {
      mrm_t *R;

      if (MRMAddInternal(&R) == SUCCESS)
        {
        DestinationJ->SubmitRM = R;
        }
      }

    if (bmisset(&tmpXMLJ->IFlags,mjifDestroyDynamicStorage))
      bmset(&DestinationJ->IFlags,mjifDestroyDynamicStorage);

    if (bmisset(&tmpXMLJ->IFlags,mjifDestroyDynamicVM))
      bmset(&DestinationJ->IFlags,mjifDestroyDynamicVM);

    if (!bmisclear(&tmpXMLJ->NotifyBM))
      bmcopy(&DestinationJ->NotifyBM,&tmpXMLJ->NotifyBM);

    if (tmpXMLJ->SpecNotification != NULL)
      MUStrDup(&DestinationJ->SpecNotification,tmpXMLJ->SpecNotification);

    /* PAL is initialized with all bits set. It is assumed that it is reset later.
       This was causing a problem in MJobDetermineTTC() which is called shortly
       after this routine, but before the PAL gets set for jobs that map feature
       to partition. Jobs that have a partition in the RMXString are OK since
       the PAL gets set in MJobProcessExtensionString() before MJobDetermineTTC()
       is called. rt8309 */

    bmcopy(&DestinationJ->PAL,&DestinationJ->SpecPAL);

    if (tmpXMLJ->Triggers != NULL)
      {
      if (DestinationJ->Triggers != NULL)
        {
        MTrigFreeList(DestinationJ->Triggers);

        MUArrayListFree(DestinationJ->Triggers);

        MUFree((char **)&DestinationJ->Triggers);

        DestinationJ->Triggers = NULL;
        }

      MMovePtr((char **)&tmpXMLJ->Triggers,(char **)&DestinationJ->Triggers);

      /* Don't blow away trigger stdout/stderr files here.  If trig was left
          during shutdown, we want to leave it here (generic sys jobs,
          for example) */

      /* These flags won't touch the triggers that have been copied already */

      for (tindex = 0;tindex < DestinationJ->Triggers->NumItems;tindex++)
        {
        mtrig_t *T;

        T = (mtrig_t *)MUArrayListGetPtr(DestinationJ->Triggers,tindex);

        T->O = (void *)DestinationJ;
        MUStrDup(&T->OID,DestinationJ->Name);
        MOAddTrigPtr(&DestinationJ->Triggers,T);
        }
      }  /* END if tmpXMLJ->T != NULL */

    if (tmpXMLJ->DynAttrs != NULL)
      {
      MUDynamicAttrCopy(tmpXMLJ->DynAttrs,&DestinationJ->DynAttrs);
      }

    for (rqindex = 0;DestinationJ->Req[rqindex] != NULL;rqindex++)
      {
      /* FIXME: add DestinationJ->Req[rqindex]->PtIndex = tmpXMLJ->Req[rqindex]->PtIndex; */

      if (DestinationJ->DestinationRM != NULL)
        {
        DestinationJ->Req[rqindex]->RMIndex = DestinationJ->DestinationRM->Index;
        }
      else if (DestinationJ->SubmitRM != NULL)
        {
        DestinationJ->Req[rqindex]->RMIndex = DestinationJ->SubmitRM->Index;
        }
      }
    }  /* END else (DestinationJ->Name[0] == '\0') */

  /* Removed check to see if job had access to QOS. 12/3/2008
     An admin can change the QOS with mjobctl -m qos=<qos>, but 
     this change would be undone if Moab were restarted because
     the job didn't have access.  The checkpoint file is authoritative */

free_and_return:

  /* tmpXMLJ->Req is populated and needs to be free.  MJobDestroy() will not destroy the reqs
     because the job is an index of 0.  Must free manually here */

  if (LocalJob == TRUE)
    {
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (tmpXMLJ->Req[rqindex] == NULL)
        break;
  
      MReqDestroy(&tmpXMLJ->Req[rqindex]);
  
      tmpXMLJ->Req[rqindex] = NULL;
      }
  
    MJobDestroy(&tmpXMLJ);
    }

  return(ReturnCode);
  }  /* END MJobLoadCPOld() */



/**
 * Get the name we should use to CP this job.
 *
 * @param J
 * @param Name (MMAX_NAME)
 */

int MJobGetCPName(

  mjob_t  *J,
  char    *Name)

  {
  if ((J == NULL) || (Name == NULL))
    {
    return(FAILURE);
    }

  /* initialize to normal name */

  MUStrCpy(Name,J->Name,MMAX_NAME);

  if ((J->DRMJID == NULL) || (J->DestinationRM == NULL) || (!strcmp(J->DRMJID,J->Name)))
    {
    /* this situation doesn't matter, we don't have DRM or it is the same */

    return(SUCCESS);
    }

  if (J->DestinationRM->Type == mrmtPBS)
    {
    /* if we migrated this job to torque then checkpoint using DRMJID */

    MJobGetName(NULL,J->DRMJID,J->DestinationRM,Name,MMAX_NAME,mjnShortName);

    return(SUCCESS);
    }

  return(SUCCESS);
  }  /* END MJobGetCPName() */




/**
 * Remove job checkpoint data.
 *
 * @param J (I)
 */

int MJobRemoveCP(

  mjob_t *J)  /* I */

  {
  char FileName[MMAX_PATH_LEN];

  const char *FName = "MJobRemoveCP";

  MDB(4,fSCHED) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (!MJobPtrIsValid(J))
    {
    return(SUCCESS);
    }

  MCPGetCPFileName(J,FileName,sizeof(FileName));
 
  MFURemove(FileName);

  MORemoveCheckpoint(mxoJob,J->Name,0,NULL);
  
  return(SUCCESS);
  }  /* END MJobRemoveCP() */




/**
 * Create 'completed' job checkpoint record.
 *
 * @see MCPStoreCJobs() - parent
 * @see MJobToXML() - child
 * @see MJobStoreCP() - peer - store non-complete jobs
 *
 * @param J (I)
 * @param String (I/O)
 * @param StoreComplete (I)
 */

int MCJobStoreCP(

  mjob_t    *J,
  mstring_t *String,
  mbool_t    StoreComplete)

  {
  const enum MJobAttrEnum DeltaJAList[] = {
    mjaAWDuration,
    mjaCmdFile,
    mjaDescription,
    mjaEEWDuration,
    mjaGJID,
    mjaJobID,
    mjaHold,
    mjaOWCLimit,
    mjaPAL,
    mjaQOSReq,
    mjaDRMJID,
    mjaMasterHost,
    mjaSID,
    mjaSRMJID,
    mjaReqReservation,
    mjaReqSMinTime,
    mjaStartCount,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaSubmitLanguage,
    mjaSubmitString,
    mjaSuspendDuration,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaTTC,
    mjaUMask,
    mjaUser,
    mjaUserPrio,
    mjaVariables,  /* variables required for completed jobs with allocation partitions */
    mjaVWCLimit,
    mjaWarning,
    mjaNONE };

  const enum MReqAttrEnum DeltaRQAList[] = {
    mrqaAllocNodeList,
    mrqaIndex,
    mrqaNONE };

  static enum MJobAttrEnum FullJAList[mjaLAST + 1] = { mjaNONE };

  static enum MReqAttrEnum FullRQAList[mrqaLAST + 1] = { mrqaNONE };

  mxml_t *JE = NULL;

  int     aindex;

  const char *FName = "MCJobStoreCP";

  MDB(4,fCKPT) MLog("%s(%s,String,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MBool[StoreComplete]);

  if ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->Flags,mrmfFullCP)))
    StoreComplete = TRUE;

  if (FullJAList[0] == mjaNONE)
    {
    for (aindex = 1;aindex != mjaLAST;aindex++)
      {
      FullJAList[aindex - 1] = (enum MJobAttrEnum)aindex;
      }
 
    FullJAList[aindex - 1] = mjaNONE;
    }

  if (FullRQAList[0] == mrqaNONE)
    {
    for (aindex = 1;aindex != mrqaLAST;aindex++)
      {
      FullRQAList[aindex - 1] = (enum MReqAttrEnum)aindex;
      }

    FullRQAList[aindex] = mrqaNONE;
    }

  if ((J == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  if (MJobToXML(
    J,
    &JE,
    (StoreComplete == TRUE) ? 
      (enum MJobAttrEnum *)FullJAList : 
      (enum MJobAttrEnum *)DeltaJAList,
    (StoreComplete == TRUE) ?
      (enum MReqAttrEnum *)FullRQAList :
      (enum MReqAttrEnum *)DeltaRQAList,
    NULL,
    TRUE,
    FALSE) == FAILURE)
    {
    MDB(1,fCKPT) MLog("ALERT:    failed to convert completed job '%s' to XML\n",
      J->Name);
    }

  if (MXMLToMString(JE,String,NULL,TRUE) == FAILURE)
    {
    MStringSet(String,"\0");
  
    MXMLDestroyE(&JE);
                                                                                
    MDB(1,fCKPT) MLog("ALERT:    failed to convert completed job '%s' from XML to string\n",
      J->Name);

    return(FAILURE);
    }

  if ((MSched.EnableHighThroughput == FALSE) && (MSched.LimitedJobCP == FALSE))
    {
    int BufLen;

    BufLen = MIN(strlen(String->c_str()),String->capacity() - 1);

    /* NOTE:  routine below may be CPU-intensive - remove when possible */

    if (MSysVerifyTextBuf(String->c_str(),BufLen,"MCJobStoreCP",J->Name) == FAILURE)
      {
      MStringSet(String,"\0");

      MXMLDestroyE(&JE);

      return(FAILURE);
      }
    }

  MXMLDestroyE(&JE);

  return(SUCCESS);
  }  /* END MCJobStoreCP() */



/* END MJobCP.c */
/**
 * Determine if policies require job to checkpoint.
 *
 * NOTE: if limited checkpointing is enabled, only checkpoint jobs with explicitly
 * modified job attributes.  Do not record minor performance and utilization
 * statistics
 *
 * @see MCPStoreQueue() - parent
 *
 * @param J (I)
 * @param R (I) [optional]
 */

mbool_t MJobShouldCheckpoint(

  mjob_t *J,   /* I */
  mrm_t  *R)   /* I (optional,notused) */

  {
  if (J == NULL)
    {
    return(FALSE);
    }

  if ((MSched.LimitedJobCP == TRUE) && (!bmisset(&J->IFlags,mjifShouldCheckpoint)))
    {
    return(FALSE);
    }  

  if (bmisset(&J->SpecFlags,mjfNoRMStart) && bmisset(&J->SpecFlags,mjfClusterLock))
    {
    /* job is remotely monitored job - no local state or settings */

    return(FALSE);
    }

/*
  if ((bmisset(&J->Flags,mjfNoRMStart) &&
      (J->StartTime + J->SpecWCLimit[0] <= MSched.Time)))
    {
    return(FALSE);
    }
*/

  if (bmisset(&J->IFlags,mjifMaster))
    {
    /* FIXME: this needs to be smarter, but we need to checkpoint jobs that may be provisioning */

    return(TRUE);
    }

  if (bmisset(&J->SpecFlags,mjfGlobalQueue))
    {
    /* this stores all jobs ever submitted via msub */

    return(TRUE);
    }

  if (bmisset(&J->IFlags,mjifVPCMap))
    {
    /* do not checkpoint system jobs which map to VPC's */

    return(FALSE);
    }

  if ((J->System != NULL) && !bmisset(&J->System->MCMFlags,mcmPersistent))
    {
    switch (J->System->JobType)
      {
      case msjtVMMap:

        /* NOTE:  currently do not checkpoint VMMap jobs - later, once VMMap jobs
                  are properly re-associated with VM's, checkpointing will be
                  useful to maintain settings, track usage states, billing, etc.
        */

        return(FALSE);

        /*NOTREACHED*/

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (J->System->JobType) */
    }    /* END if (J->System != NULL) */

  /* The 30 is a 30 second buffer from the actual submission to Moab detection */

  if ((J->EffQueueDuration > 0) && 
      ((MSched.Time - J->SubmitTime) - J->EffQueueDuration) > 30)
    {
    return(TRUE);
    }

  if ((MSched.SideBySide == TRUE) && (bmisset(&J->SpecFlags,mjfNoResources)))
    {
    return(TRUE);
    }

  if ((MSched.LimitedJobCP == FALSE) && (MJOBISIDLE(J) == FALSE))
    {
    return(TRUE);
    }

  if (!bmisclear(&J->Hold))
    {
    return(TRUE);
    }

  if (J->SystemPrio != 0)
    {
    return(TRUE);
    }

  if ((MSched.LimitedJobCP == FALSE) && (J->MessageBuffer != NULL))
    {
    return(TRUE);
    }

  if (J->UPriority != 0)
    {
    return(TRUE);
    }

  if (bmisset(&J->Flags,mjfArrayJob))
    {
    return(TRUE);
    }

  if (J->Depend != NULL) 
    {
    /* job may have dependencies which are not supported by RM */

    /* NOTE:  make more specific (NYI) */

    return(TRUE);
    }
 
  /* losing SystemQueueTime information */ 

  /* PAL and flag info can be set and information can be lost */
  /* is this a good thing, or should this be fixed!? */

  if ((J->Credential.Q != NULL) && 
      (J->Credential.U->F.QDef[0] != NULL) && 
      (J->Credential.Q != J->Credential.U->F.QDef[0]))
    {
    return(TRUE);
    }

  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    return(TRUE);
    }

  if ((J->DataStage != NULL) && 
      ((J->DataStage->SIData != NULL) || (J->DataStage->SOData != NULL)))
    {
    /* jobs with data reqs need checkpointing */

    return(TRUE);
    }

  /* jobs only in internal queue need checkpointing */

  if ((J->DestinationRM == NULL) &&      
      (J->SubmitRM != NULL) &&
      (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue)))
    {
    return(TRUE);
    }

  if (bmisset(&J->IFlags,mjifShouldCheckpoint))
    {
    return(TRUE);
    }

  return(FALSE); 
  }  /* END MJobShouldCheckpoint() */



/**
 *
 *
 * @param J (I)
 * @param CP (O)
 */

int MJobCPDestroy(

  mjob_t    *J,   /* I */
  mjckpt_t **CP)  /* O */

  {
  if ((J == NULL) || (CP == NULL))
    {
    return(FAILURE);
    }

  MUFree((char **)CP);

  return(SUCCESS);
  }  /* END MJobCPDestroy() */


