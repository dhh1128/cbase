/* HEADER */

      
/**
 * @file MJobTransitionXML.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


extern int __MReqTransitionAToMString(mtransreq_t *,enum MReqAttrEnum,mstring_t *);


/**
 * Create xml describing the partition priority from a mtransjob_t
 *  
 * @see __MJobParPriorityToXML() 
 *  
 * @param J (I)  The transition job providing the information 
 * @param JE (O) The XML structure
 */

int __MJobTransitionParPriorityToXML(

  mtransjob_t *J,
  mxml_t      *JE)

  {

  if (MSched.PerPartitionScheduling == TRUE)
    {
    int   numParTokens;
    char *ParTokenArray[MMAX_PAR+1];
    int   ParTokenIndex;
  
    numParTokens = MUStrSplit(J->PerPartitionPriority,",",-1,ParTokenArray,MMAX_PAR+1);
  
    for (ParTokenIndex = 0; ParTokenIndex < numParTokens; ParTokenIndex++)
      {
      mxml_t *ParE;

      const int     numTokens = 3;             /* always in the format parname:priority:rsvstarttime */
      char   *TokenArray[numTokens+1] = {0}; /* +1 so that there is room for a NULL terminator */
      mulong  Priority;
      long    RsvStartTime;
      int     tIndex = 0;

      MUStrSplit(ParTokenArray[ParTokenIndex],":",-1,TokenArray,numTokens+1);

      MXMLCreateE(&ParE,(char *)MXO[mxoPar]);

      MXMLSetAttr(ParE,(char *)MParAttr[mpaID],TokenArray[tIndex++],mdfString);

      Priority = atol(TokenArray[tIndex++]);
      MXMLSetAttr(ParE,(char *)MJobAttr[mjaStartPriority],(void *)&Priority,mdfLong);

      /* only send a rsvstarttime if it is non-zero */

      if ((RsvStartTime = atol(TokenArray[tIndex])) != 0)
        MXMLSetAttr(ParE,(char *)MJobAttr[mjaRsvStartTime],(void *)&RsvStartTime,mdfLong);

      MXMLAddE(JE,ParE);

      for (tIndex = 0;tIndex < numTokens;tIndex++)
        MUFree(&TokenArray[tIndex]);

      MUFree(&ParTokenArray[ParTokenIndex]);
      } /*END for ... */
    } /* END if (MSched.PerPartitionScheduling == TRUE) */
    return(SUCCESS);
  } /* END __MJobTransitionParPriorityToXML() */


/**
 * Store a job transition object in the given XML object
 *
 * @param   J        (I) the job transition object
 * @param   JEP      (O) the XML object to store it in
 * @param   SJAList  (I) [optional] A list of Job attributes to convert to xml
 * @param   SRQAList (I) [optional] A list of Req attributes to convert to xml
 * @param   CFlags   (I) 
 */

int MJobTransitionToXML(

  mtransjob_t        *J,
  mxml_t            **JEP,
  enum MJobAttrEnum  *SJAList,
  enum MReqAttrEnum  *SRQAList,
  long                CFlags)

  {
  mxml_t *JE;     /* element for the job */
  mxml_t *RQE;    /* element to be used to add each request to the job */
  mxml_t *ME;     /* element for the job's messages */

  mbool_t AddMessages = FALSE;

  int rqindex;
  int aindex;

  enum MJobAttrEnum *JAList;

  char tmpBuf[MMAX_BUFFER];

  const enum MJobAttrEnum DJAList[] = {
    mjaAccount,
    mjaAllocNodeList,
    mjaAWDuration,
    /*mjaBlockReason,*/
    mjaClass,
    mjaCmdFile,
    mjaDRM,
    mjaDRMJID,
    mjaEEWDuration,   /* eligible job time */
    mjaEState,
    mjaFlags,
    mjaGAttr,
    mjaGJID,
    mjaHold,
    mjaIWD,
    mjaJobID,
    mjaJobName,
    mjaParentVCs,
    mjaQOSReq,
    mjaRM,
    mjaRMXString,
    mjaReqAWDuration,
    mjaReqNodes,
    mjaReqProcs,
    mjaReqReservation,
    mjaRsvStartTime,
    mjaSRMJID,
    mjaStartCount,
    mjaStartPriority,
    mjaStartTime,
    mjaState,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaSubmitTime,
    mjaSubmitLanguage,
    mjaSubmitString,
    mjaSuspendDuration,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaTaskMap,
    mjaUMask,
    mjaUser,
    mjaUserPrio,
    mjaVariables,
    mjaNONE };

  /*enum MJobStateEnum JobState = (enum MJobStateEnum)MUGetIndexCI(J->State,MJobState,FALSE,mjsIdle);*/

  *JEP = NULL;

  if (MXMLCreateE(JEP,(char *)MXO[mxoJob]) == FAILURE)
    {
    return(FAILURE);
    }

  JE = *JEP;

  if (SJAList != NULL)
    JAList = SJAList;
  else
    JAList = (enum MJobAttrEnum *)DJAList;

  for (aindex = 0; JAList[aindex] != mjaNONE; aindex++)
    {
    tmpBuf[0] = '\0';

    switch(JAList[aindex])
      {
      case mjaMessages:

        AddMessages = TRUE;

        break;

      case mjaStartCount:

        if (J->StartCount > 0)
          {
          MJobTransitionAToString(J,JAList[aindex],tmpBuf,sizeof(tmpBuf),mfmNONE);

          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)tmpBuf,mdfString);
          }

        break;

      case mjaBecameEligible:

        if (J->EligibleTime > 0)
          {
          MJobTransitionAToString(J,JAList[aindex],tmpBuf,sizeof(tmpBuf),mfmNONE);

          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)tmpBuf,mdfString);
          }

        break;

      case mjaStartTime:

        if (J->StartTime > 0)
          {
          MJobTransitionAToString(J,JAList[aindex],tmpBuf,sizeof(tmpBuf),mfmNONE);

          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)tmpBuf,mdfString);
          }

        break;

      case mjaTaskMap:

        /* tmpBuf is too small for TaskMap. TaskMap has already been allocated
         * been allocated the destination string so just use it. */

        if (!J->TaskMap->empty())
          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)J->TaskMap->c_str(),mdfString);

        break;

      case mjaEnv:

        if (!MUStrIsEmpty(J->Environment))
          {
          mstring_t Translated(MMAX_LINE);
          mstring_t Packed(MMAX_LINE);

          /* Prepare env string to be printed. BaseEnv can contain ENVRS_ENCODED_CHAR
           * (record separator to handle nested ;'s) and '\n's which
           * are not printable characters and will cause the job to fail
           * to checkpoint in MSysVerifyTextBuf. */

          MStringReplaceStr(J->Environment,ENVRS_ENCODED_STR,ENVRS_SYMBOLIC_STR,&Translated);
          MUMStringPack(Translated.c_str(),&Packed);

          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)Packed.c_str(),mdfString);
          }

        break;

      case mjaRMXString:

        if (!J->RMExtensions->empty())
          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)J->RMExtensions->c_str(),mdfString);

        break;

      case mjaSubmitString:

        if (!MUStrIsEmpty(J->RMSubmitString))
          {
          mstring_t Packed(MMAX_LINE);

          MUMStringPack(J->RMSubmitString,&Packed);

          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)Packed.c_str(),mdfString);
          }

        break;

      case mjaVariables:

        if (J->Variables != NULL)
          {
          mxml_t *VE = NULL;
   
          MXMLDupE(J->Variables,&VE);

          MXMLAddE(JE,VE);
          }

        break;

      default:

        MJobTransitionAToString(J,JAList[aindex],tmpBuf,sizeof(tmpBuf),mfmNONE);

        if (!MUStrIsEmpty(tmpBuf))
          {
          MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],(void *)tmpBuf,mdfString);
          }

        break;
      } /* END switch(AIndex) */
    } /* END for (AIndex...) */

  if (MSched.PerPartitionScheduling == TRUE)
    {
    __MJobTransitionParPriorityToXML(J,JE);
    } /* END if (MSched.PerPartitionScheduling == TRUE) */

  /* add the job's requests/requirements to the xml */

  for (rqindex = 0; rqindex < MMAX_REQ_PER_JOB; rqindex++)
    {
    /* add request element to xml */

    if (J->Requirements[rqindex] == NULL)
      break;

    if (MReqTransitionToXML(J->Requirements[rqindex],&RQE,SRQAList) == FAILURE)
      {
      MDB(0,fSCHED) MLog("ERROR:   Unable to create xml element from request transition object at index %d on job %s\n",
        J->Requirements[rqindex]->Index,
        J->Name);

      return(FAILURE);
      }

    MXMLAddE(JE,RQE);
    }

  /* add messages */

  if ((AddMessages == TRUE) && (!MUStrIsEmpty(J->Messages)))
    {
    ME = NULL;

    if (MXMLFromString(&ME,J->Messages,NULL,NULL) == SUCCESS)
      {
      if (MXMLAddE(JE,ME) != SUCCESS)
        {
        MDB(3,fSCHED) MLog("WARNING:  Unable to add messages to job %s transition XML\n",J->Name);
        }
      }
    } /* END block (case mjaMessage) */

  return(SUCCESS);
  }  /* END MJobTransitionToXML() */





/**
 * Store the base data of a job transition object in the given XML object
 *
 * @param   J           (I) the job transition object
 * @param   JEP         (O) the XML object to store it in
 * @param   P           (I) [optional] Used to select partition-specific data.
 * @param   FlagString  (I) flags for the request
 */

int MJobTransitionBaseToXML(

  mtransjob_t    *J,
  mxml_t        **JEP,
  mpar_t         *P,
  char           *FlagString)

  {
  mxml_t *JE;

  enum MJobStateEnum tmpState;
  char tmpLine[MMAX_LINE];

  mbitmap_t FlagBM;
  mbool_t DoVerbose = FALSE;

  enum MJobStateEnum JobState;

  if ((J == NULL) || (MXMLCreateE(JEP,(char *)MXO[mxoJob]) == FAILURE))
    {
    return(FAILURE);
    }

  JobState = J->State;

  JE = *JEP;

  bmfromstring(FlagString,MClientMode,&FlagBM);

  if (bmisset(&FlagBM,mcmVerbose))
    DoVerbose = TRUE;

  MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);

  if (!MUStrIsEmpty(J->AName))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaJobName],(void *)J->AName,mdfString);

  if (!MUStrIsEmpty(J->User))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void *)J->User,mdfString);

  if (!MUStrIsEmpty(J->DestinationRMJobID))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaDRMJID],(void *)J->DestinationRMJobID,mdfString);

  if (!MUStrIsEmpty(J->Group))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void *)J->Group,mdfString);

  if (!MUStrIsEmpty(J->Account))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void *)J->Account,mdfString);

  if (!MUStrIsEmpty(J->QOS))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void *)J->QOS,mdfString);

  if (!MUStrIsEmpty(J->Class))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void *)J->Class,mdfString);

  MXMLSetAttr(JE,(char *)MJobAttr[mjaReqAWDuration],(void *)MULToTStringSeconds(J->RequestedMaxWalltime,tmpLine,MMAX_LINE),mdfString);

  if (J->UsedWalltime > 0)
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaAWDuration],(void *)&J->UsedWalltime,mdfLong);
    }
  
  if (J->EffQueueDuration > 0)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaEEWDuration],(void *)&J->EffQueueDuration,mdfLong);

  if (!MUStrIsEmpty(J->GridJobID))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGJID],(void *)J->GridJobID,mdfString);

  if (((MSTATEISCOMPLETE(JobState)) || (MJOBSISACTIVE(JobState))) &&
      (!MUStrIsEmpty(J->MasterHost)))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaMasterHost],(void *)J->MasterHost,mdfString);

  if ((J->Requirements[0] != NULL) && !MUStrIsEmpty(J->Requirements[0]->Partition))
    {
    if ((MJOBSISACTIVE(JobState)) || (MSTATEISCOMPLETE(JobState)))
      {
      MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)J->Requirements[0]->Partition,mdfString);
      }
    }
  else if (DoVerbose == TRUE)
    {
    if (!bmissetall(&J->PAL,MMAX_PAR))
      MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)MPALToString(&J->PAL,",",tmpLine),mdfString);
    else
      MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)MPar[0].Name,mdfString);
    }

  MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&J->MaxProcessorCount,mdfInt);

  if (J->RequestedNodes > 0)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaReqNodes],(void *)&J->RequestedNodes,mdfInt);

  if ((J->RsvStartTime > 0) && (!MSTATEISCOMPLETE(JobState)))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaRsvStartTime],(void *)&J->RsvStartTime,mdfLong);

  if (MJOBSISACTIVE(JobState))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaRunPriority],(void *)&J->RunPriority,mdfLong);

  if (!MSTATEISCOMPLETE(JobState))
    {
    long Priority = J->Priority;

    if (TRUE == MSched.PerPartitionScheduling)
      {
      __MJobTransitionParPriorityToXML(J,JE);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaStartPriority],(void *)&Priority,mdfLong);
    }

  MXMLSetAttr(JE,(char *)MJobAttr[mjaStartTime],(void *)&J->StartTime,mdfLong);

  if ((MSTATEISCOMPLETE(JobState)) || (MJOBSISACTIVE(JobState)))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaStatPSDed],(void *)&J->PSDedicated,mdfDouble);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaStatPSUtl],(void *)&J->PSUtilized,mdfDouble);
    }

  if (bmisset(&J->TFlags,mtransfWasCanceled))
    {
    if (J->CompletionCode != 0)
      {
      snprintf(tmpLine,sizeof(tmpLine),"CNCLD(%d)",
        J->CompletionCode);
      }
    else
      {
      snprintf(tmpLine,sizeof(tmpLine),"CNCLD");
      }
    }
  else if (MSTATEISCOMPLETE(JobState))
    {
    snprintf(tmpLine,sizeof(tmpLine),"%d",J->CompletionCode);
    }

  if (MSTATEISCOMPLETE(JobState))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaCompletionCode],(void *)tmpLine,mdfString);

  if (!bmisclear(&J->Hold))
    {
    tmpState = mjsHold;

    if (bmisset(&J->Hold,mhBatch))
      tmpState = mjsBatchHold;
    else if (bmisset(&J->Hold,mhSystem))
      tmpState = mjsSystemHold;
    else if (bmisset(&J->Hold,mhUser))
      tmpState = mjsUserHold;
    else if (bmisset(&J->Hold,mhDefer))
      tmpState = mjsDeferred;

    MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)MJobState[tmpState],mdfString);

    } /* END if (J->Hold) */

  else
    {
    char tmpStateName[MMAX_NAME];

    enum MJobSubStateEnum JobSubState = J->SubState;

    MUStrCpy(tmpStateName,(char *)MJobState[JobState],sizeof(tmpStateName));

    if (!MSTATEISCOMPLETE(JobState))
      {
      if ((!MUStrIsEmpty(J->DestinationRM) || (bmisset(&J->TFlags,mtransfPurge))) && 
          (bmisset(&J->TFlags,mtransfWasCanceled)))
        {
        MUStrCpy(tmpStateName,"Canceling",sizeof(tmpStateName));
        }
      else if (JobSubState == mjsstMigrated)
        {
        MUStrCpy(tmpStateName,"Migrated",sizeof(tmpStateName));
        }
      else if (JobSubState == mjsstProlog)
        {
        MUStrCpy(tmpStateName,"Starting",sizeof(tmpStateName));
        }
      else if (JobSubState == mjsstEpilog)
        {
        MUStrCpy(tmpStateName,"Ending",sizeof(tmpStateName));
        }
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)tmpStateName,mdfString);
    }

  MXMLSetAttr(JE,(char *)MJobAttr[mjaSubmitTime],(void *)&J->SubmitTime,mdfLong);

  MXMLSetAttr(JE,(char *)MJobAttr[mjaSuspendDuration],(void *)&J->SuspendTime,mdfLong);

  if (J->CompletionTime > 0)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaCompletionTime],(void *)&J->CompletionTime,mdfLong);

  return(SUCCESS);
  } /* END MJobTransitionBaseToXML() */



/**
 * Convert a job into XML that is used when a destination peer is reporting back
 * to a source peer.  This function enforces cred mapping, host mapping, etc.
 *
 * @see MJobTransitionToXML() - peer 
 * @see MJobToExportXML()
 *
 * Handles:
 *  mjaAllocNodeList
 *  mjaGroup
 *  mjaUser
 *  mjaQOS
 *  mjaQOSReq
 *  mjaAccount
 *  mjaClass
 *  mjaMessages - sets source attr
 *  mjaRMOutput
 *  mjaRMError
 *  mjaRM
 *  mjaSRMJID
 *  mjaDRMJID
 *  mjaBlockReason
 *
 * @param J (I)
 * @param JEP (O) [(possibly allocated) - NULL or existing mxml_t object] 
 * @param R (I)
 */

int MJobTransitionToExportXML(

  mtransjob_t  *J,        /* I */
  mxml_t      **JEP,      /* O (possibly allocated) - NULL or existing mxml_t object */
  mrm_t        *R)        /* I */

  {
  mxml_t *JE = *JEP;
  mxml_t *MBE;

  char tmpLine[MMAX_LINE];

  if (R == NULL)
    {
    return(FAILURE);
    }

  mstring_t tmpBuf(MMAX_LINE);

  /* AllocNodeList FORMAT: <Node Name>[:<Task Count>][,...] */
        
  __MReqTransitionAToMString(J->Requirements[0],mrqaAllocNodeList,&tmpBuf);

  MXMLSetAttr(JE,(char *)MJobAttr[mjaAllocNodeList],(void **)tmpBuf.c_str(),mdfString);

  /* check if job was cred-mapped */

  if (!MUStrIsEmpty(J->GridUser))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void **)J->GridUser,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (!MUStrIsEmpty(J->SourceRM) && strcmp(J->SourceRM,R->Name))
      {
      MOMap(R->OMap,mxoUser,J->User,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoUser,J->User,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void **)tmpLine,mdfString);
    }
  else if (!MUStrIsEmpty(J->User))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void **)J->User,mdfString);
    }
  
  /* group */
 
  if (!MUStrIsEmpty(J->GridGroup))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void **)J->GridGroup,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (!MUStrIsEmpty(J->SourceRM) && strcmp(J->SourceRM,R->Name))
      { 
      MOMap(R->OMap,mxoGroup,J->Group,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoGroup,J->Group,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void **)tmpLine,mdfString);
    }
  else if (!MUStrIsEmpty(J->Group))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void **)J->Group,mdfString);
    }

  /* class */

  if (!MUStrIsEmpty(J->GridClass))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void **)J->GridClass,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (!MUStrIsEmpty(J->SourceRM) && strcmp(J->SourceRM,R->Name))
      { 
      MOMap(R->OMap,mxoClass,J->Class,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoClass,J->Class,FALSE,FALSE,tmpLine);
      }

    if (tmpLine[0] != '\0')
      MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void **)tmpLine,mdfString);
    }
  else if (!MUStrIsEmpty(J->Class))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void **)J->Class,mdfString);
    }

  /* account */

  if (!MUStrIsEmpty(J->GridAccount))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void **)J->GridAccount,mdfString);
    }
  else if (R->OMap != NULL)
    {  
    if (!MUStrIsEmpty(J->SourceRM) && strcmp(J->SourceRM,R->Name))
      { 
      MOMap(R->OMap,mxoAcct,J->Account,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoAcct,J->Account,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void **)tmpLine,mdfString);
    }
  else if (!MUStrIsEmpty(J->Account))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void **)J->Account,mdfString);
    }

  /* qos */

  if (!MUStrIsEmpty(J->GridQOS))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void **)J->GridQOS,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (!MUStrIsEmpty(J->SourceRM) && strcmp(J->SourceRM,R->Name))
      { 
      MOMap(R->OMap,mxoQOS,J->QOS,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoQOS,J->QOS,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void **)tmpLine,mdfString);
    }
  else if (!MUStrIsEmpty(J->QOS))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void **)J->QOS,mdfString);
    }

  /* qos requested */

  if (!MUStrIsEmpty(J->GridQOSReq))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOSReq],(void **)J->GridQOSReq,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (!MUStrIsEmpty(J->SourceRM) && strcmp(J->SourceRM,R->Name))
      { 
      MOMap(R->OMap,mxoQOS,J->QOSReq,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoQOS,J->QOSReq,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOSReq],(void **)tmpLine,mdfString);
    }
  else if (!MUStrIsEmpty(J->QOSReq))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOSReq],(void **)J->QOSReq,mdfString);
    }

  if ((MJOBSISACTIVE(J->State) == FALSE) && (!MUStrIsEmpty(J->MigrateBlockReason)))
    {
    if (!MUStrIsEmpty(J->MigrateBlockMsg))
      {
      MStringSetF(&tmpBuf,"%s: %s",
        J->MigrateBlockReason,
        J->MigrateBlockMsg);
      }
    else
      {
      MStringSetF(&tmpBuf,J->MigrateBlockReason);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaBlockReason],(void **)tmpBuf.c_str(),mdfString);
    }

  /* output and error files */

  if (!MUStrIsEmpty(J->RMOutput))
    {
    char *ofile;

    char  tmpLine[MMAX_LINE];
    char *ptr;

    if ((R->ExtHost != NULL) && ((ptr = strchr(J->RMOutput,':')) != NULL))
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s%s",
        R->ExtHost,
        ptr);

      ofile = tmpLine;
      }
    else
      {
      ofile = J->RMOutput;
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaOFile],(void **)ofile,mdfString);
    }    /* END if (J->E.RMOutput != NULL) */

  if (!MUStrIsEmpty(J->RMError))
    {
    char *efile;

    char  tmpLine[MMAX_LINE];
    char *ptr;

    if ((R->ExtHost != NULL) && ((ptr = strchr(J->RMError,':')) != NULL))
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s%s",
        R->ExtHost,
        ptr);

      efile = tmpLine;
      }
    else
      {
      efile = J->RMError;
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaEFile],(void **)efile,mdfString);
    }    /* END if (J->E.RMError != NULL) */

  /* determine SRM and make sure XML accurately reports it */

  if (!MUStrIsEmpty(J->SystemID))
    {
    /* job was staged from another peer */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void **)J->SystemID,mdfString);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaSRMJID],(void **)J->GridJobID,mdfString);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaDRMJID],(void **)J->Name,mdfString);
    }
  else
    {
    /* job was locally submitted */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void **)MSched.Name,mdfString);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaSRMJID],(void **)J->Name,mdfString);    
    }
        
  if ((!MUStrIsEmpty(J->Messages)) && 
      (MXMLGetChild(JE,(char *)MJobAttr[mjaMessages],NULL,&MBE) == SUCCESS))
    {
    mxml_t *ME;
    int     MTok;

    MTok = -1;

    while (MXMLGetChild(MBE,NULL,&MTok,&ME) == SUCCESS)
      {
      MXMLSetAttr(ME,(char *)MMBAttr[mmbaSource],(void **)MSched.Name,mdfString);
      }
    }

  /* Variables */

  if (!MUStrIsEmpty(J->UserHomeDir))
    {
    /* Grid Data Staging: In the case that the home directories are different between
     * the compute node and the submission node, RHOME can be used to distinguish
     * the remote home directory. */

    char tmpVal[MMAX_LINE];

    sprintf(tmpVal,"RHOME=%s",J->UserHomeDir);

    MXMLAppendAttr(JE,(char *)MJobAttr[mjaVariables],tmpVal,';');
    }

  return(SUCCESS);
  }  /* END MJobTransitionToExportXML() */


/**
 * Sets an individual attribute on an mtransjob_t object.
 *
 * @param J (I) modified
 * @param Name (I)
 * @param Val (I)
 * @param E (I)
 */

int __MS3JobTransitionSetAttr(

  mtransjob_t *J,
  char        *Name,
  char        *Val,
  mxml_t      *E)

  {
  enum MJobAttrEnum jaindex;
  enum MReqAttrEnum rqaindex;
  enum MS3JobAttrEnum sjaindex;
  enum MS3ReqAttrEnum srqaindex;

  mtransreq_t *RQ;

  RQ = J->Requirements[0];
  
  if (Val == NULL)
    {
    return(FAILURE);
    }

  if ((sjaindex = (enum MS3JobAttrEnum)MUGetIndex(Name,(const char **)MS3JobAttr,FALSE,0)) != 0)
    {
    jaindex = MS3JAttrToMoabJAttr(sjaindex);

    if (jaindex == mjaNONE)
      {
      return(FAILURE);
      }

    switch (jaindex)
      {
      case mjaAccount:

        MUStrDup(&J->Account,Val);

        break;
        
      case mjaAllocNodeList:

        MUStrDup(&J->RequestedHostList,Val);

        break;

      case mjaArgs:

        MUStrDup(&J->Arguments,Val);

        break;

      case mjaAWDuration:

         J->UsedWalltime = strtol(Val,NULL,10);

         break;

      case mjaCmdFile:

        MUStrDup(&J->CommandFile,Val);

        break;

      case mjaCPULimit:

        J->CPULimit = strtol(Val,NULL,10);

        break;

      case mjaDepend:

        MUStrDup(&J->Dependencies,Val);

        break;

      case mjaEnv:

        MUStrDup(&J->Environment,Val);

        break;

      case mjaFlags:

        MUStrDup(&J->Flags,Val);

        break;

      case mjaGroup:

        MUStrDup(&J->Group,Val);

        break;

      case mjaHold:

        bmfromstring(Val,MHoldType,&J->Hold);

        break;

      case mjaIWD:

        MUStrDup(&J->InitialWorkingDirectory,Val);

        break;

      case mjaJobName:

        MUStrDup(&J->AName,Val);

        break;

      case mjaNotification:

        bmfromstring(Val,MNotifyType,&J->NotifyBM);

        break;

      case mjaQOSReq:

        MUStrDup(&J->QOS,Val);

        break;

      case mjaReqAWDuration:

         J->RequestedMaxWalltime = strtol(Val,NULL,10);

         break;

      case mjaReqReservation:
 
        MUStrDup(&J->RequiredReservation,Val);

        break;

      case mjaRMXString:

        MJobTransitionProcessRMXString(J,Val);

        break;

      case mjaStdErr:

        MUStrDup(&J->StdErr,Val);

        break;

      case mjaStdIn:

        MUStrDup(&J->StdIn,Val);

        break;

      case mjaStdOut:

        MUStrDup(&J->StdOut,Val);

        break;

      case mjaSubmitString:
        
        MUStrDup(&J->RMSubmitString,Val);

        break;

      case mjaSubmitTime:

        J->SubmitTime = MSched.Time;

        break;

      case mjaUser:

        MUStrDup(&J->User,Val);

        break;

      case mjaUserPrio:

        J->UserPriority = strtol(Val,NULL,10);

        break;

      default:
      case mjaPAL:
      case mjaVMUsagePolicy:

        /* NYI */

        break;
      }  /* END switch (jaindex) */
    }    /* END if (sjaindex....) */
  else if ((srqaindex = (enum MS3ReqAttrEnum)MUGetIndex(Name,(const char **)MS3ReqAttr,FALSE,0)) != 0)
    {
    rqaindex = MS3ReqAttrToMoabReqAttr(srqaindex);

    if (rqaindex == mrqaNONE)
      {
      return(FAILURE);
      }

    switch (rqaindex)
      {
      case mrqaGRes:

        MUStrDup(&RQ->GenericResources,Val);

        break;

      case mrqaNCReqMin:

        RQ->MinNodeCount = strtol(Val,NULL,10);

        break;

      case mrqaReqArch:

        MUStrDup(&RQ->RequestedArch,Val);

        break;

      case mrqaReqDiskPerTask:

        RQ->DiskPerTask = strtol(Val,NULL,10);

        break;

      case mrqaReqMemPerTask:

        RQ->MemPerTask = strtol(Val,NULL,10);

        break;

      case mrqaReqNodeDisk:

        MUStrDup(&RQ->NodeDisk,Val);

        break;

      case mrqaReqNodeFeature:

        MUStrDup(&RQ->NodeFeatures,Val);

        break;

      case mrqaReqNodeMem:

        MUStrDup(&RQ->NodeMemory,Val);

        break;

      case mrqaReqNodeProc:

        MUStrDup(&RQ->NodeProcs,Val);

        break;

      case mrqaReqNodeSwap:

        MUStrDup(&RQ->NodeSwap,Val);

        break;

      case mrqaReqOpsys:

        MUStrDup(&RQ->ReqOS,Val);

        break;

      case mrqaReqSoftware:

        MUStrDup(&RQ->GenericResources,Val);

        break;

      case mrqaReqSwapPerTask:

        RQ->SwapPerTask = strtol(Val,NULL,10);

        break;

      case mrqaTCReqMin:

        RQ->MinTaskCount = strtol(Val,NULL,10);

        break;

      case mrqaTPN:

        RQ->TaskCountPerNode = strtol(Val,NULL,10);

        break;

      default:
      case mrqaGPUS:
      case mrqaMICS:
      case mrqaNONE:

        /* NYI */

        break;
      }  /* END (switch (rqaindex) */
    }    /* END else if (srqaindex) */
  else if (!strcmp(Name,"Requested"))
    {
    int CTok = -1;

    mxml_t *CE;

    while (MXMLGetChildCI(E,NULL,&CTok,&CE) == SUCCESS)
      {
      if (__MS3JobTransitionSetAttr(
            J,
            CE->Name,
            CE->Val,
            CE) == FAILURE)
        {
        return(FAILURE);
        }
      }
    }   /* END else if (!strcmp(AName,"Requested")) */


  return(SUCCESS);
  }  /* END __MS3JobTransitionSetAttr() */


/**
 * Convert SSS XML to job transition struct.
 *
 * @param RE 
 * @param Name (I job name)
 * @param DJ (O job transition struct)
 */

int MJobXMLToTransitionStruct(

  mxml_t      *RE,
  char        *Name,
  mtransjob_t *DJ)

  {
  int cindex;

  mxml_t *CE;
  mxml_t *JE;

  MUStrCpy(DJ->Name,Name,sizeof(DJ->Name));

  DJ->State = mjsIdle;

  MReqTransitionAllocate(&DJ->Requirements[0]);

  if (MXMLGetChildCI(RE,(char *)MXO[mxoJob],NULL,&JE) == FAILURE)
    {
    /* code expects "<request><job></job></request>" format */

    return(FAILURE);
    }

  for (cindex = 0;cindex < JE->CCount;cindex++)
    {
    CE = JE->C[cindex];

    __MS3JobTransitionSetAttr(
      DJ,
      CE->Name,
      CE->Val,
      CE);
    }  /* END for (cindex) */
 
  DJ->MaxProcessorCount = DJ->Requirements[0]->MinTaskCount;

  if (DJ->MaxProcessorCount == 0)
    DJ->MaxProcessorCount = 1;

  if (DJ->RequestedMaxWalltime == 0)
    DJ->RequestedMaxWalltime = MDEF_SYSDEFJOBWCLIMIT;

  return(SUCCESS);
  }  /* END MJobXMLToTransitionStruct() */



/**
 * Add an SSS XML job to the cache.
 *
 * @param JE (I) SSS type job XML definitation
 * @param Name (I)
 */

int MJobXMLTransition(

  mxml_t *JE,
  char   *Name)

  {
  mtransjob_t *TJ;

  if (JE == NULL)
    {
    return(FAILURE);
    }

  MJobTransitionAllocate(&TJ);

  MJobXMLToTransitionStruct(JE,Name,TJ);

  TJ->SubmitTime = MSched.Time;
  TJ->QueueTime  = MSched.Time;

  MOExportTransition(mxoJob,TJ);

  return(SUCCESS);
  }  /* END MJobXMLTransition() */


