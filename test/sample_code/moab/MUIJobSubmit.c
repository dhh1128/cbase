/* HEADER */

/**
 * @file MUIJobSubmit.c
 *
 * Contains MUI Job Submit
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"



int __MUIJobSubmitRemove(mjob_t **JP);
int __MUIJobCtlSubmitLogRejectEvent(mjob_t *,char *);


/** 
 * Checks the QOS for a submit request 
 *  
 * @param J (I) Job pointer
 * @param tmpLine (O) QOS failure reason string.
 * @param tmpLineSize (I) size of reason string buffer 
 */

int __MUIJobCtlSubmitCheckQOS(
  mjob_t *J,
  char   *tmpLine,
  long    tmpLineSize)
  {
  /* if a qos was requested and FSTREEISREQUIRED,
   * validate through the fairshare tree */

  if ((MSched.FSTreeIsRequired == TRUE) &&
      (J->Credential.A != NULL) &&
      ((J->QOSRequested != NULL) || (J->Credential.Q != NULL)) &&
      (MQOSValidateFairshare(J,((J->QOSRequested != NULL) ? J->QOSRequested : J->Credential.Q)) == FAILURE))
    {
    snprintf(tmpLine,tmpLineSize,"ERROR:  cannot submit job - invalid QOS/User/Account combination in fairshare\n");

    return(FAILURE);
    }

  /* BOC 5-6-10 - The !mjifFBQOSInUse check was removed in 5.3, but we're 
   * not sure why. I'm putting it back in 5.4, if it needs to be removed 
   * please comment why. */

  if ((J->QOSRequested != NULL) && 
      (J->QOSRequested != J->Credential.Q) &&
      !bmisset(&J->IFlags,mjifFBQOSInUse) &&
      !bmisset(&J->IFlags,mjifFBAcctInUse))
    {
    mqos_t *QDef;

    if (MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&QDef) == FAILURE)
      {
      if (bmisset(&MSched.QOSRejectPolicyBM,mjrpIgnore))
        MJobSetQOS(J,QDef,0);

      if (bmisset(&MSched.QOSRejectPolicyBM,mjrpCancel))
        {
        snprintf(tmpLine,tmpLineSize,"ERROR:  cannot submit job - invalid QOS requested\n");

        return(FAILURE);
        }
      }
    else
      {
      MJobSetQOS(J,J->QOSRequested,0);
      }
    }
  else if ((bmisset(&MSched.QOSRejectPolicyBM,mjrpCancel)) &&
           (bmisset(&J->IFlags,mjifCancel)))
    {
    snprintf(tmpLine,tmpLineSize,"ERROR:  cannot submit job - invalid QOS requested\n");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END  __MUIJobCtlSubmitCheckQOS() */


int __MUIJobCtlSubmitLogRejectEvent(
  mjob_t *J,
  char *Msg)

  {
  if (J == NULL)
    return(FAILURE);

  if (MSched.PushEventsToWebService == TRUE)
    { 
    MEventLog *Log = new MEventLog(meltJobReject);
    Log->SetCategory(melcReject);
    Log->SetFacility(melfJob);
    Log->SetPrimaryObject(J->Name,mxoJob,(void *)J);
    if (Msg != NULL)
      {
      Log->AddDetail("msg",Msg);
      }

    MEventLogExportToWebServices(Log);

    delete Log;
    }

  return(SUCCESS);
  }  /* END __MUIJobCtlSubmitLogRejectEvent */




/**
 * Filter job XML before processing.
 *
 * @see MFilterXML - peer for client side processing
 *
 * @param JEP (I/O) pointer to XML structure that may optionally be destroyed and replaced
 * @param EMsg     (O)
 */

int MUIJobSubmitFilterXML(

  mxml_t **JEP,
  char    *EMsg)

  {
  mxml_t    *JE = NULL;
  mxml_t    *EE = NULL;
  int        ReturnCode;
  char       tmpEMsg[MMAX_LINE];

  mpsi_t     tmpP;

  int rc = SUCCESS;

  if ((JEP == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  /* send XML to a filter command and use the output of the command as the final XML
   * to send to the server
   */

  if (MUStrIsEmpty(MSched.ServerSubmitFilter))
    {
    return(SUCCESS);
    }
  
  JE = *JEP;

  if ((JE == NULL) || (MUStrIsEmpty(JE->Name)) || strcmp(JE->Name,MXO[mxoJob]))
    {
    return(SUCCESS);
    }

  mstring_t  MBuffer(MMAX_BUFFER);

  if (MXMLToMString(JE,&MBuffer,NULL,TRUE) == FAILURE)
    {
    snprintf(EMsg,MMAX_LINE,"error converting request to XML");

    return(FAILURE);
    }

  /* make a buffer at least twice as big as needed to hold the current XML
   * structure, and at least as big as MMAX_BUFFER
   */

  memset(&tmpP,0,sizeof(tmpP));

  if (C.Timeout > 0)
    tmpP.Timeout = C.Timeout; 

  mstring_t  Output(MMAX_LINE);
  mstring_t  ErrBuf(MMAX_LINE);

  MUReadPipe2(
    MSched.ServerSubmitFilter,
    MBuffer.c_str(),
    &Output,
    &ErrBuf,
    &tmpP,            /* I - contains timeout */
    &ReturnCode,
    tmpEMsg,
    NULL);

  snprintf(EMsg,MMAX_LINE,"%s",ErrBuf.c_str());

  switch(ReturnCode)
    {
    case 0:

      if (MXMLFromString(&EE,Output.c_str(),NULL,tmpEMsg) == FAILURE)
        {
        snprintf(EMsg,MMAX_LINE,"failed to parse the XML output of Filter Command %s: (%s)\n",
          MSched.ServerSubmitFilter,
          tmpEMsg);

        rc = FAILURE;
        }

      rc = SUCCESS;

      *JEP = EE;

      break;

    case mscTimeout:

      rc = FAILURE;

      snprintf(EMsg,MMAX_LINE,"%s",tmpEMsg);

      break;

    default:

      rc = FAILURE;

      if (ErrBuf.empty())
        {
        snprintf(EMsg,MMAX_LINE,"\nFilter Command '%s' failed with status code %d\n",
            MSched.ServerSubmitFilter,
            ReturnCode);
        }

      /*NOTREACHED*/

      break;
    }  /* END switch (ReturnCode) */

  return(rc);
  }  /* END MUIJobSubmitFilterXML() */








/**
 * Handles the submission of jobs from the msub command or
 * from other Moab peers.
 *
 * NOTE:  Upon successful job submission, register event to end UI
 *        phase and schedule newly submitted job.
 *
 * @see MUIJobCtl() - parent
 * @see MCSubmitCreateRequest() - peer - create submit request w/in client
 * @see MRMJobSubmit() - child
 * @see MJobEval() - child
 *
 * @param Socket (I) optional - The socket containing a job submission request.
 * @param Auth   (I) The username/peer name submitting the job.
 * @param ID     (I)
 * @param RE     (I) An XML representation of the requested job submission. 
 * @param JP     (O) A Job pointer to return. [optional]
 * @param Msg    (O)
 */

int MUIJobSubmit(

  msocket_t  *Socket,
  char       *Auth,
  int         ID,
  mxml_t     *RE,
  mjob_t    **JP,
  char       *Msg)

  {
  char    tmpRMType[MMAX_NAME];

  int     RMCount;

  mrm_t  *R = NULL;
  mrm_t  *StageR = NULL;
  mrm_t  *PR;

  mxml_t *JE;
  mxml_t *SE;

  mxml_t *RetE; /* Return XML if format==XML */

  int     CTok;

  int     rmindex;
  int     pindex;
  int     SpecParCount = 0;

  mbitmap_t FlagBM;
  mbitmap_t RMFlags;

  char    EMsg[MMAX_LINE] = {0};

  mjob_t *J = NULL;
  char    JobName[MMAX_NAME];

  char    tmpLine[MMAX_LINE];
  int     rc;
  int     tmpI;

  enum    MAllocRejEnum JobEvalFailReason;
  enum    MAMJFActionEnum MAMFailureAction;
  mbool_t CanRunCurIteration = TRUE;
  char    RejBuf[MMAX_LINE];
  char   *BPtr;
  int     BSpace;

  int     TotalRMCount;

  mbool_t MatchFound;

  char  FlagString[MMAX_LINE];
  enum MFormatModeEnum Format;

  const char *FName = "MUIJobSubmit";
        
  MDB(2,fUI) MLog("%s(S,%s,RE)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (Auth == NULL)
    return(FAILURE);

  if (Msg != NULL)
    Msg[0] = '\0';

  JobName[0] = '\0';  
  RetE = NULL;

  /* NOTE:  job may be submitted by admin (i.e., system job) or by peer service */

  if (MXMLGetAttr(RE,MSAN[msanOption],NULL,tmpRMType,sizeof(tmpRMType)) == SUCCESS)
    {
    int rmindex;

    enum MRMTypeEnum RMType;

    if ((RMType = (enum MRMTypeEnum)MUGetIndexCI(
          tmpRMType,
          MRMType,
          FALSE,
          0)) == 0)
      {
      char    tmpLine[MMAX_LINE];

      if (Msg != NULL)
        snprintf(Msg,MMAX_LINE,"unknown msub submit option in XML\n");

      MOWriteEvent(MSched.DefaultJ,mxoJob,mrelJobReject,"unknown msub submit option in XML",NULL,NULL);

      strcpy(tmpLine,"unknown msub submit option in XML");
      __MUIJobCtlSubmitLogRejectEvent(MSched.DefaultJ,tmpLine);

      return(FAILURE);
      }

    /* locate resource manager with matching RM type */

    for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
      {
      R = &MRM[rmindex];

      if (R->Name[0] == '\0')
        break;

      if (MRMIsReal(R) == FALSE)
        continue;

      if (!bmisset(&R->IFlags,mrmifLocalQueue))
        {
        continue;
        }

      if (R->Type == RMType)
        {
        /* verify method exists to migrate job to RM */

        if ((MSched.UID == 0) ||
            (R->Type == mrmtMoab))
          {
          /* job can migrate to RM */

          break;
          }
        }
      }    /* END for (rmindex) */

    if ((R != NULL) && (R->Type != RMType))
      {
      R = NULL;
      }
    }    /* END if (MXMLGetAttr() == SUCCESS) */
  else
    {
    /* NOTE:  by default, submit job to first feasible resource manager which 
              allows migration */

    for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
      {
      R = &MRM[rmindex];

      if (R->Name[0] == '\0')
        {
        /* end of list reached */

        R = NULL;

        break;
        }

      if (MRMIsReal(R) == FALSE)
        continue;

      if ((MSched.UID == 0) ||
          ((R->Type == mrmtPBS) && (bmisset(&R->Flags,mrmfProxyJobSubmission))) ||
          ((R->Type == mrmtWiki) && (R->SubType == mrmstSLURM)) ||
         /* ((J->Cred.U->OID > 1) && (MSched.UID == J->Cred.U->OID)) || */
          (R->Type == mrmtMoab) ||
          (getenv("MFORCESUBMIT") != NULL))
        {
        /* job can migrate to RM */

        break;
        }
      }    /* END for (rmindex) */

    if (R == NULL)
      {
      char    tmpLine[MMAX_LINE];

      MOWriteEvent(
        (void *)MSched.DefaultJ,
        mxoJob,
        mrelJobReject,
        "cannot submit job - migration not enabled",NULL,NULL);

      strcpy(tmpLine,"cannot submit job - migration not enabled");
      __MUIJobCtlSubmitLogRejectEvent(MSched.DefaultJ,tmpLine);

      if (Socket != NULL)
        MUISAddData(Socket,"cannot submit job - migration not enabled, see admin for further assistance");

      MSched.NoJobMigrationMethod = TRUE;

      if (Msg != NULL)
        snprintf(Msg,MMAX_LINE,"migration not enabled\n");

      return(FAILURE);
      }    /* END if ((R == NULL) || ...) */
    }      /* END else if (MXMLGetAttr() == SUCCESS) */

  if ((R == NULL) || !bmisset(&R->IFlags,mrmifLocalQueue))
    {
    if (MRMGetInternal(&R) == FAILURE)
      {
      if (Socket != NULL)
        MUISAddData(Socket,"cannot locate local queue");

      if (Msg != NULL)
        snprintf(Msg,MMAX_LINE,"internal error: cannot locate local queue\n");

      return(FAILURE);
      }
    }    /* END if ((R == NULL) || ...) */

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    MRMFindByPeer(Auth + strlen("peer:"),&PR,TRUE);
    }
  else
    {
    PR = NULL;
    }

  CTok = -1;

  Format = mfmHuman;

  if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    {
    char tmpFlags[MMAX_LINE];
    mbitmap_t CFlags;

    MUStrCpy(tmpFlags,FlagString,MMAX_LINE);

    bmfromstring(tmpFlags,MClientMode,&CFlags);

    if (bmisset(&CFlags,mcmXML) == TRUE)
      Format = mfmXML;
    }

  if (Format == mfmXML)
    {
    MXMLCreateE(&RetE,(char *)MSON[msonData]);
    }

  /* NOTE: if more than one job is passed the MJobEval() (and job submission) 
     will not properly send job rejection info back to sender.  Must enable 
     partial job rejection info in XML response. NYI */

  while (MXMLGetChildCI(RE,(char *)MXO[mxoJob],&CTok,&JE) == SUCCESS)
    {
    /* NOTE:  job description in S3 format */

    /* run the filter */

    if (MUIJobSubmitFilterXML(&JE,EMsg) == FAILURE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR: submit filter failed - %s\n",EMsg);

      MOWriteEvent((void *)MSched.DefaultJ,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

      __MUIJobCtlSubmitLogRejectEvent(MSched.DefaultJ,tmpLine);

      if (Socket != NULL)
        {
        MCPSubmitJournalRemoveEntry(Socket->JournalID);
        MUISAddData(Socket,tmpLine);
        }

      if (Msg != NULL)
        MUStrCpy(Msg,tmpLine,MMAX_LINE);

      return(FAILURE);
      }

    /* make sure this job is not a duplicate and also manage credential mapping */

    if (MS3JobPreSubmit(JE,Auth,PR,EMsg) == FAILURE)
      {
      MOWriteEvent((void *)MSched.DefaultJ,mxoJob,mrelJobReject,EMsg,NULL,NULL);

      __MUIJobCtlSubmitLogRejectEvent(MSched.DefaultJ,EMsg);

      if (Socket != NULL)
        {
        MCPSubmitJournalRemoveEntry(Socket->JournalID);
        MUISAddData(Socket,EMsg);
        }

      if (Msg != NULL)
        MUStrCpy(Msg,EMsg,MMAX_LINE);

      return(FAILURE);
      }

    /* NOTE:  if system job is specified, should we break out at this point 
              and process separately? */

    /* must support the following attributes:

         sprio, duration, starttime == or >=, exec, failaction={notify|record|reserve} <- enabled via trigger?, 
         hostlist, isserial, maxactive, issystem */

    /* exec is launched via trigger.  The job should have noexec set by default */

    /* check for data-only */

    /* NOTE: should be MCS3JobAttr[MCS3VIndex][mjaSubmitString] */

    if (MXMLGetChildCI(JE,(char *)"SubmitString",NULL,&SE) == SUCCESS)  
      {
      char tmpVal[MMAX_NAME];
      
      if (MXMLGetAttr(SE,(char *)MJobSubmitFlags[mjsufDataOnly],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
        {
        if (!strcasecmp(tmpVal,"true"))
          {
          bmset(&FlagBM,mjsufDataOnly);
          }
        }

      if (MXMLGetAttr(SE,(char *)MJobSubmitFlags[mjsufDSDependency],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
        {
        if (!strcasecmp(tmpVal,"true"))
          {
          bmset(&FlagBM,mjsufDSDependency);
          }
        }

      if (MXMLGetAttr(SE,(char *)MJobSubmitFlags[mjsufSyncJobID],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
        {
        MUStrCpy(JobName,tmpVal,sizeof(JobName));
        }
      }  /* END if (MXMLGetChildCI(JE) == SUCCESS) */

    if (PR != NULL)
      {
      /* adjust hop count of job submitted by remote peer */

      /* NOTE: must adjust hop count BEFORE MRMJobSubmit() so checkpoint is 
               accurate */

      if (MXMLGetChildCI(JE,(char *)MJobAttr[mjaHopCount],NULL,&SE) == SUCCESS)
        {
        if (SE->Val != NULL)
          {
          tmpI = (int)strtol(SE->Val,NULL,10);
          }
        else
          {
          tmpI = 0;
          }

        tmpI++;
        }
      else
        {
        tmpI = 1;

        MXMLCreateE(&SE,(char *)MJobAttr[mjaHopCount]);

        MXMLAddE(JE,SE);
        }

      MXMLSetVal(SE,(void *)&tmpI,mdfInt);
      }  /* END if (PR != NULL) */

    /* force owner if not specified */

    if (Socket != NULL)
      {
      if (MXMLGetChildCI(JE,(char *)MJobAttr[mjaOwner],NULL,&SE) == FAILURE)
        {
        MXMLCreateE(&SE,(char *)MJobAttr[mjaOwner]);

        MXMLAddE(JE,SE);

        MXMLSetVal(SE,(void *)Socket->RID,mdfString);
        }
      else if (SE->Val == NULL)
        {
        MXMLSetVal(SE,(void *)Socket->RID,mdfString);
        }
      }

    J = NULL;

    /* job credentials set via MRMJobSubmit() */

    bmset(&FlagBM,mjsufGlobalQueue);

    /* if job name specified, do not assign new jobid */

    if (ID > 0)
      {
      snprintf(JobName,sizeof(JobName),"%d",ID);
      }
    else if ((Socket != NULL) && (Socket->Data != NULL))
      {
      MUStrCpy(JobName,(char *)Socket->Data,sizeof(JobName));
      }
    else if ((MXMLGetChildCI(JE,(char *)"Interactive",NULL,&SE) == SUCCESS) && 
             (MUBoolFromString(SE->Val,FALSE) == TRUE))
      {
      if (MSched.DisableInteractiveJobs == TRUE)
        {
        sprintf(tmpLine,"ERROR:    interactive jobs are disabled\n");

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"interactive jobs are disabled\n");

        return(FAILURE);
        }

      /* 3-4-10 BOC RT7119 - msub'ed' interactive jobs should get a moab jobid
       * so there aren't collisions when the job is shown on another peer. 

      strcpy(JobName,"interactive");
       */
      }

    /* convert to string */

    mstring_t tmpBuf(MMAX_BUFFER);

    if (MXMLToMString(JE,&tmpBuf,NULL,TRUE) == FAILURE)
      {
      sprintf(tmpLine,"ERROR:    cannot parse job, invalid XML received\n");

      if (Socket != NULL)
        {
        MCPSubmitJournalRemoveEntry(Socket->JournalID);
        MUISAddData(Socket,tmpLine);
        }
    
      continue;
      }

    /* submit to internal resource manager */

    rc = MRMJobSubmit(
          tmpBuf.c_str(),  /* I (job description) */
          R,
          &J,      /* O */
          &FlagBM,  /* data only */
          JobName, /* I (special) */
          EMsg,    /* O */
          NULL);

    bmclear(&FlagBM);

    if (rc == FAILURE)
      {
      if (EMsg[0] != '\0')
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot submit job - %s\n",
          EMsg);
        }
      else
        {
        sprintf(tmpLine,"ERROR:    cannot submit job\n");
        }

      if ((EMsg != NULL) && (strstr(tmpLine,"directory")))
        {
        MUStrDup(&R->FMapEMsg,EMsg);
        }

      if ((EMsg != NULL) && (strstr(tmpLine,"group")))
        {
        MUStrDup(&R->CMapEMsg,EMsg);
        }

      MOWriteEvent(
        (J != NULL) ? (void *)J : (void *)MSched.DefaultJ,
        mxoJob,
        mrelJobReject,
        tmpLine,
        NULL,
        NULL);

      __MUIJobCtlSubmitLogRejectEvent((J != NULL) ? J : MSched.DefaultJ,tmpLine);

      if (J != NULL)
        {
        __MUIJobSubmitRemove(&J); /* remove job from cache and database */
        MJobRemove(&J);
        }

      if (Socket != NULL)
        {
        MCPSubmitJournalRemoveEntry(Socket->JournalID);
        MUISAddData(Socket,tmpLine);
        }

      if (Msg != NULL)
        MUStrCpy(Msg,EMsg,MMAX_LINE);

      return(FAILURE);
      }  /* END if (rc == FAILURE) */

    if (JP != NULL)
      {
       /* save job to return */

      *JP = J;
      }

    /* store job submission script to job archive */

    if (MSched.StoreSubmission == TRUE)
      {
      if (MFUWriteFile(
           J,
           J->RMSubmitString,
           MStat.StatDir,
           "jobarchive",
           NULL,
           FALSE,
           FALSE,
           NULL) == FAILURE)
        {
        MMBAdd(
          &MSched.MB,
          "storing job submission script to jobarchive directory failed (create stats/jobarchive or restart Moab).",
          NULL,
          mmbtOther,
          MSched.Time + MCONST_DAYLEN,
          10,
          NULL);
        }
      }    /* END if (MSched.StoreSubmission == TRUE) */

    if ((J->System != NULL) &&
        (J->System->JobType == msjtVMTracking) &&
        (MSched.PushEventsToWebService == TRUE))
      {
      char *VMID = NULL;

      if (MUHTGet(&J->Variables,"VMID",(void **)&VMID,NULL) == SUCCESS)
        {
        /* VM hasn't been created yet, can't get serialization (but could pull from job) */
        CreateAndSendEventLog(meltVMSubmit,VMID,mxoxVM,NULL);
        }
      }

    /* set the job's submit host */

    if (Socket != NULL)
      {
      /* If SubmitHost is NOT already set, then set with socket client's host */
      if(NULL == J->SubmitHost)
        {
        MJobSetAttr(J,mjaSubmitHost,(void **)Socket->RemoteHost,mdfString,mSet);
        }

      /* we can remove a job entry because the job was checkpointed at submit
       * time to the internal RM */

      MCPSubmitJournalRemoveEntry(Socket->JournalID);
      }

#ifdef MYAHOO
    /* need to checkpoint RM so that this job is loaded on restart */

    {
    mdb_t *MDBInfo;

    MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

    if (MDBInfo->DBType != mdbNONE)
      MCPStoreCluster(&MCP,NULL);
    }
#endif /* MYAHOO */

    if (J->Owner != NULL)
      {
      /* verification also happens at the end of MS3JobSubmit()
         Do we need both? */

      if ((Socket != NULL) && 
          ((Socket->RID == NULL) || strcmp(Socket->RID,J->Owner)))
        {
        mgcred_t *U = NULL;

        mbitmap_t AuthBM;

        MUserFind(Socket->RID,&U);

        if ((U != NULL) && (U->ViewpointProxy == FALSE))
          {
          if (Socket->RID != NULL)
            {
            MSysGetAuth(Socket->RID,mcsMSchedCtl,0,&AuthBM);
            }

          if (!bmisset(&AuthBM,mcalAdmin1))
            {
            char    tmpLine[MMAX_LINE];

            /* do not allow differences unless Socket->RID is admin1 */

            MOWriteEvent((void *)J,mxoJob,mrelJobReject,"invalid credentials specified for submitted job",NULL,NULL);

            strcpy(tmpLine,"invalid credentials specified for submitted job");
            __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

            MUISAddData(Socket,"invalid credentials specified for submitted job");
  
            __MUIJobSubmitRemove(&J);
  
            if (Msg != NULL)
              snprintf(Msg,MMAX_LINE,"invalid credentials\n");

            return(FAILURE);
            }
          }
        }
      }    /* END if (J->Owner != NULL) */

    if (PR != NULL)
      {
      /* job being submitted by remote peer */

      /* set appropriate sandbox settings */

      if (PR->GridSandboxExists == TRUE)
        {
        bmset(&J->SpecFlags,mjfAdvRsv);
        bmset(&J->Flags,mjfAdvRsv);
        bmset(&J->SpecFlags,mjfRsvMap);
        bmset(&J->Flags,mjfRsvMap);
        }
 
      bmset(&J->IFlags,mjifRemote);

      /* do not allow hierarchical co-allocation */

      bmunset(&J->SpecFlags,mjfCoAlloc);
      bmunset(&J->Flags,mjfCoAlloc);

      /* NOTE: mjfAdvRsv is optional on peer to peer */

      bmset(&J->RsvAccessBM,mjapAllowSandboxOnly);
      }  /* END if (PR != NULL) */

    /* NOTE: J may be destroyed in MRMJobSubmit() */

    /* validate job */

    if (J->RMXString != NULL)
      {
      char SEMsg[MMAX_LINE];

      /* When the job is submitted as with msub -l naccesspolicy the 
         naccesspolicy, will override a class's default and forced 
         naccesspolicy because they are set before this in MJobSetDefaults()
         (default) and MJobApplyDefaults (forced) within MRMJobPostload().
       */

      mulong DETime;

      /* 3-19-09 BOC RT7025 - Set SubmitEval flag so that the dependency 
       * check can return failure. */

      if ((MSched.MissingDependencyAction != mmdaRun) &&
          (MSched.MissingDependencyAction != mmdaAssumeSuccess))
        {
        bmset(&J->IFlags,mjifSubmitEval);
        }

      if (MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,SEMsg) == FAILURE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot submit job - %s\n",
         (MUStrIsEmpty(SEMsg)) ? "error in argument string (e.g. -l argument)" : SEMsg);

        if (Socket != NULL)
          {
          MUISAddData(Socket,tmpLine);
          }

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"error in extension string\n");

        return(FAILURE);
        }

      bmunset(&J->IFlags,mjifSubmitEval);
 
      if (__MUIJobCtlSubmitCheckQOS(J,tmpLine,sizeof(tmpLine)) != SUCCESS)
        {  
        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"error with job's QOS\n");

        return(FAILURE);
        }

      /* When the job is submitted with msub -l naccesspolicy the naccesspolicy,
       * calling MJobProcessExtensionString here will override a class's default 
       * and forced naccesspolicy, which are set before this in MJobSetDefaults (default) 
       * and MJobApplyDefaults (forced) within MRMJobPostload. */

      if ((J->Req[0] != NULL) &&
          (J->Credential.C != NULL) && 
          (J->Credential.C->L.JobSet != NULL) &&
          (J->Credential.C->L.JobSet->Req[0] != NULL) &&
          (J->Credential.C->L.JobSet->Req[0]->NAccessPolicy != mnacNONE))
        {
        J->Req[0]->NAccessPolicy = J->Credential.C->L.JobSet->Req[0]->NAccessPolicy;
        }
      
      if ((J->VMUsagePolicy == mvmupVMCreate) && (J->Req[0]->Opsys == 0))
        {
        snprintf(tmpLine,sizeof(tmpLine),"VM usage policy of VM-create requires "
            "that an operating system be specified");

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"error with job's VMUsage policy\n");

        return(FAILURE);
        }
      else if ((J->VMUsagePolicy == mvmupVMOnly) && (J->Req[0]->Opsys == 0))
        {
        /*  requirevm jobs don't need to specify an OS */
       /* NO-OP */
        }
      else if (((J->VMUsagePolicy == mvmupVMOnly) || (J->VMUsagePolicy == mvmupVMCreate)) &&
          ((MUHTGet(&MSched.VMOSList[0],MAList[meOpsys][J->Req[0]->Opsys],NULL,NULL) == FAILURE)))
        {
        /* VM OS must be specified for VM workload */

        snprintf(tmpLine,sizeof(tmpLine),"Invalid OS specified for VM workload");

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"invalid OS specified for VM workload\n");

        return(FAILURE);
        }

      if (MSDUpdateStageInStatus(J,&DETime) == SUCCESS)
        {
        /* TODO: only set SysSMinTime if we know we are only overriding previous DS SysSMinTime changes */

        /*
        if (bmisset(&J->IFlags,mjifDataDependency) && (DETime != MMAX_TIME))
          {
          MJobSetAttr(J,mjaSysSMinTime,(void **)&DETime,mdfLong,mSet);
          }
        */
        }

      if (MJobProcessRestrictedAttrs(J,EMsg) == FAILURE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot submit job - error creating triggers (%s)\n",
          EMsg);

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          MUStrCpy(Msg,EMsg,MMAX_LINE);

        return(FAILURE);
        }

      if ((J->Array != NULL) &&
          (MJobLoadArray(J,EMsg) == FAILURE))
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot submit job - error creating job array (%s)\n",
          EMsg);

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          MUStrCpy(Msg,EMsg,MMAX_LINE);

        return(FAILURE);
        }
      }  /* END if (J->RMXString != NULL) */
    else
      {
      if (__MUIJobCtlSubmitCheckQOS(J,tmpLine,sizeof(tmpLine)) != SUCCESS)
        {  
        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"error with job's QOS\n");

        return(FAILURE);
        }
      } /* END else (J->RMXString != NULL) */

    bmset(&J->IFlags,mjifSubmitEval);

    if (MJobEval(J,PR,NULL,EMsg,&JobEvalFailReason) == FAILURE)
      {
      if (((JobEvalFailReason != marPartition) &&
          (JobEvalFailReason != marClass)) ||
          (bmisset(&J->IFlags,mjifFairShareFail)))
        {
        if (EMsg[0] != '\0')
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot submit job - %s\n",
            EMsg);
          }
        else
          {
          sprintf(tmpLine,"ERROR:    invalid job specified\n");
          }
        
        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);
          
        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);
        
        __MUIJobSubmitRemove(&J);
        
        if (Msg != NULL)
          MUStrCpy(Msg,EMsg,MMAX_LINE);

        return(FAILURE);
        }
      
      /* Job could not find partition, could be because 
       partition has not reported back to Moab yet.  Set 
       to the MAX in case J->SMinTime is already set. */
         
      J->SMinTime = MAX(MSched.Time + 2,J->SMinTime);
      
      CanRunCurIteration = FALSE;
      }  /* END if (MJobEval() == FAILURE) */

    bmunset(&J->IFlags,mjifSubmitEval);

    /* LLNL requested a configuration parameter to disallow msubs from a 
       slave. */

    if ((MSched.Mode == msmSlave) && (MSched.DisableSlaveJobSubmit == TRUE))
      {
      if (J->SystemID == NULL) /* job was submitted from the slave and not migrated from the master */
        {
        if (EMsg[0] != '\0')
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot submit job from slave - %s\n",
            EMsg);
          }
        else
          {
          sprintf(tmpLine,"ERROR:    cannot submit job from slave\n");
          }

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"cannot submit job from slave\n");

        return(FAILURE);
        }
      } 

    /* Check total cred limits */

    if ((PR == NULL) && 
        (MCredCheckTotals(J,EMsg) == FAILURE))
      {
      /* exceeded a cred limit */

      if (EMsg[0] != '\0')
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot submit job - cred limit reached - %s\n",
          EMsg);
        }
      else
        {
        sprintf(tmpLine,"ERROR:    cannot submit job - cred limit reached\n");
        }

      MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

      __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

      if (Socket != NULL)
        MUISAddData(Socket,tmpLine);

      __MUIJobSubmitRemove(&J);

      if (Msg != NULL)
        snprintf(Msg,MMAX_LINE,"cred limit reached\n");

      return(FAILURE);
      } /* END if (PR == NULL) */

    /* verify credential mapping is valid (only remote job submission) */

    if ((PR != NULL) &&
        (J->Credential.U != NULL) &&
        (J->Credential.G != NULL))
      {
      char FailureReason[MMAX_LINE];
      mbool_t IsFailure = FALSE;

      /* Is U a member of G? */

      if (J->Credential.U->OID > 0)
        {
        if (MCredCheckGroupAccess(J->Credential.U,J->Credential.G) == FAILURE)
          {
          snprintf(FailureReason,sizeof(FailureReason),"user '%s' does not belong to group '%s'",
            J->Credential.U->Name,
            J->Credential.G->Name);

          IsFailure = TRUE;
          }
        else if ((J->Credential.A != NULL) &&
            (MCredCheckAcctAccess(J->Credential.U,J->Credential.G,J->Credential.A) == FAILURE))
          {
          snprintf(FailureReason,sizeof(FailureReason),"user '%s' does not belong to account '%s'",
            J->Credential.U->Name,
            J->Credential.A->Name);

          IsFailure = TRUE;
          }
        }

      if (IsFailure == TRUE)
        {
        sprintf(tmpLine,"ERROR:    job requests invalid credential access (%s)\n",
          FailureReason);

        MDB(3,fSCHED) MLog("WARNING:  job %s requests invalid credential access (%s)\n",
          J->Name,
          FailureReason);

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        __MUIJobSubmitRemove(&J);

        if (Msg != NULL)
          MUStrCpy(Msg,tmpLine,MMAX_LINE);

        return(FAILURE);
        }
      }    /* END if ((PR != NULL) && ...) */

    if (bmisset(&J->SpecFlags,mjfNoQueue))
      {
      mnl_t      *MNodeList[MMAX_REQ_PER_JOB];
      char       *NodeMap = NULL;

      /* job must start immediately or submit should be rejected */

      MNodeMapInit(&NodeMap);
      MNLMultiInit(MNodeList);

      if (MJobSelectMNL(
            J,
            &MPar[0],
            NULL,
            MNodeList,  /* O */
            NodeMap,    /* O */
            TRUE,
            FALSE,
            NULL,
            NULL,
            NULL) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    job cannot start immediately\n");

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);
 
        __MUIJobSubmitRemove(&J);

        MUFree(&NodeMap);
        MNLMultiFree(MNodeList);

        if (Msg != NULL)
          snprintf(Msg,MMAX_LINE,"job cannot start immediately\n");

        return(FAILURE);
        }

      MJobStart(J,NULL,NULL,"no-queue job started");

      MUFree(&NodeMap);
      MNLMultiFree(MNodeList);
      }  /* END if (bmisset(&J->SpecFlags,mjfNoQueue)) */

    RMCount = 0;

    /* locate destination RM */

    for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
      {
      R = &MRM[rmindex];

      if (R->Name[0] == '\0')
        break;

      if (MRMIsReal(R) == FALSE)
        continue;

      if (bmisset(&R->IFlags,mrmifLocalQueue))
        {
        bmcopy(&RMFlags,&R->Flags);

        continue;
        }

      /* NOTE:  verify at least one partition in J->PAL is associated with
                this partition */

      /* NOTE:  job extension string may not be processed at this point? */

      MatchFound = FALSE;

      MUSNInit(&BPtr,&BSpace,RejBuf,sizeof(RejBuf));

      for (pindex = 0;pindex < MMAX_PAR;pindex++)
        {
        if (MPar[pindex].Name[0] == '\0')
          break;

        if (MPar[pindex].Name[0] == '\1')
          continue;

        if (bmisset(&J->PAL,pindex) == FAILURE)
          continue;

        if (MPar[pindex].RM != R)
          continue;

        if (MJobCheckParFeasibility(J,&MPar[pindex],(MSched.NoJobHoldNoResources == FALSE),EMsg) == FAILURE)
          {
          MUSNPrintF(&BPtr,&BSpace,"cannot use partition %s in RM %s - %s\n",
            MPar[pindex].Name,
            R->Name,
            EMsg);

          continue;
          }

        /* SUCCESS - partition matches */

        MatchFound = TRUE;

        break;
        }    /* END for (pindex) */
 
      if (MatchFound == FALSE)
        {
        /* RM does not match partition constraints */

        continue;
        }

      /* NOTE:  native RM's can be full compute RM systems */

      if (R->Type == mrmtNative)
        {
        /* ignore if there is no way to migrate job to RM */

        if (!bmisclear(&R->FnList))
          {
          if (!bmisset(&R->FnList,mrmJobSubmit))
            continue;
          }
        else if (R->ND.URL[mrmJobSubmit] == NULL)
          {
          continue;
          }
        }

      if (R->Type == mrmtMoab)
        {
        /* NOTE: allow users to msub to an RM that is down */

        if ((R->State != mrmsActive) && (R->State != mrmsDown))
          {
          continue;
          }

        if (!bmisset(&R->Flags,mrmfExecutionServer) ||
             bmisset(&R->Flags,mrmfClient))
          {
          continue;
          }

        if (bmisset(&J->IFlags,mjifDataOnly))
          {
          /* data-only jobs must be executed locally */

          continue;
          }
        }    /* END if (R->Type == mrmtMoab) */

      if (StageR == NULL)
        StageR = R;

      RMCount++;
      }    /* END for (rmindex) */

    if ((StageR == NULL) &&
        !bmisset(&MSched.Flags,mschedfDynamicRM) &&
        (CanRunCurIteration == TRUE))
      {
      if (RejBuf[0] == '\0')
        {
        sprintf(tmpLine,"ERROR:    cannot locate valid destination resource manager\n");
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot locate valid destination resource manager -\n%s",
          RejBuf);
        }

      MDB(3,fSCHED) MLog("WARNING:  no valid destinations located for job '%s'\n",
        J->Name);

      MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

      __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

      if (Socket != NULL)
        MUISAddData(Socket,tmpLine);

      __MUIJobSubmitRemove(&J);

      if (Msg != NULL)
        snprintf(Msg,MMAX_LINE,"cannot locate valid destination resource manager\n");

      return(FAILURE);
      }  /* END if (StageR == NULL) */

    if (MSched.Mode == msmSim)
      {
      bmset(&J->Flags,mjfNoRMStart);
      bmset(&J->SpecFlags,mjfNoRMStart);
      }

    if (bmisset(&RMFlags,mrmfJobNoExec))
      {
      bmset(&J->SpecFlags,mjfNoRMStart);
      bmset(&J->Flags,mjfNoRMStart);
      }

    if (bmisset(&J->Flags,mjfInteractive))
      {
      /* for interactive job submit, mark job as data only, extract job 
         script, remove script, remove job, and return job script string */

      bmset(&J->IFlags,mjifDataOnly);
      }

    MRMGetComputeRMCount(&TotalRMCount);

    /* Get the number of partitions that this job requested. If there 
     * is only one then migrate the job to it since the user doesn't
     * want it to run any where else. If the user changes his/her mind
     * then the user will have to cancel the job and submit to a different
     * partition since unmigrate isn't fully implemented. */

    SpecParCount = MJobGetSpecPALCount(J);

    /* Note: interactive jobs can not be migrated "just in time", 
              so allow to be submitted here */

    if ((!bmisset(&J->Flags,mjfNoRMStart) && 
        (CanRunCurIteration == TRUE) &&
        ((MSched.JobMigratePolicy != mjmpJustInTime) ||
          (bmisset(&J->Flags,mjfInteractive))) &&
        (RMCount == 1) &&
        ((TotalRMCount == 1) || 
         (SpecParCount == 1) ||
         (MJobCheckParRes(J,&MPar[StageR->PtIndex]) == SUCCESS))) || 
         bmisset(&J->IFlags,mjifDataOnly))
      {
      enum MStatusCodeEnum SC;
      char EMsg[MMAX_LINE] = {0};

      /* if only one non-local queue rm exists, immediately migrate job 
         to non-local queue rm (also if job is data-only) */

      bmset(&J->IFlags,mjifSubmitEval);

      if (MJobEval(J,NULL,StageR,EMsg,&JobEvalFailReason) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    cannot migrate job to %s - %s\n",
          (StageR != NULL) ? MRMType[StageR->Type] : "None",
          (EMsg[0] != '\0') ? EMsg : "");

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        if (J != NULL)
          {
          __MUIJobSubmitRemove(&J); /* remove job from cache and database */
          MJobRemove(&J); /* remove job */
          }

        if (Msg != NULL)
          MUStrCpy(Msg,EMsg,MMAX_LINE);

        return(FAILURE); 
        }

      bmunset(&J->IFlags,mjifSubmitEval);

      if ((MSched.JobMigratePolicy == mjmpAuto) ||
          (MSched.JobMigratePolicy == mjmpImmediate && (!bmisclear(&J->Hold))))        
        {
        /* make a decision based on how many resource managers and what types */
 
        if ((StageR->Type != mrmtMoab) &&
            (StageR->SyncJobID == FALSE) &&
            (MSched.UseMoabJobID == FALSE) &&
            (RMCount == 1))
          {
          bmset(&J->IFlags,mjifUseDRMJobID);
          }

        if (MSched.UseMoabJobID == TRUE)
          {
          MJobSetAttr(J,mjaSID,(void **)MSched.Name,mdfString,mSet);
          MJobSetAttr(J,mjaGJID,(void **)J->Name,mdfString,mSet);
          }

        if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRMID != NULL))
          {
          /* NOTE:  job was evaluated against StageR resources but must be
                    migrated to specified 'application' RM to utilize resources
          */

          if ((J->TemplateExtensions->WorkloadRM != NULL) ||
              (MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM) == SUCCESS))
            {
            StageR = J->TemplateExtensions->WorkloadRM;
            }
          }  /* END if ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */

        if ((StageR->State == mrmsActive) &&  /* if RM isn't active, just let alone */
            (rc = MJobMigrate(&J,StageR,FALSE,NULL,EMsg,&SC)) == FAILURE)
          {
          /*
           * removed at request of LLNL--jobs that fail to migrate should be
           * kept in internal queue, in the case that they could be
           * scheduled else where!

          if (J != NULL)
            MJobRemove(&J);
          */

          sprintf(tmpLine,"ERROR:    cannot migrate job '%s' to %s - %s\n",
            J->Name,
            MRMType[StageR->Type],
            EMsg);

          MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

          __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

          if (Socket != NULL)
            {
            MUISAddData(Socket,tmpLine);
            MSCToMSC(SC,(enum MSFC *)&Socket->StatusCode);
            }

          if (Msg != NULL)
            MUStrCpy(Msg,EMsg,MMAX_LINE);

          return(FAILURE); 
          }  /* END if ((StageR->State == mrmsActive) && ...) */

        if (bmisset(&J->Flags,mjfInteractive))
          {
          if (J->RMSubmitString != NULL)
            {
            MUStrCpy(tmpLine,J->RMSubmitString,sizeof(tmpLine));
            }
          else
            {
            sprintf(tmpLine,"ERROR:  cannot submit interactive job\n");
            }

          MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

          __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

          /* Update the cred total usage for creds used by this job */

          MStatIncrementCredUsage(J);

          __MUIJobSubmitRemove(&J);

          if (Socket != NULL)
            MUISAddData(Socket,tmpLine);

          MSysRaiseEvent();

          if (Msg != NULL)
            snprintf(Msg,MMAX_LINE,"cannot submit interactive job\n");

          return(SUCCESS);
          }  /* END if (bmisset(&J->Flags,mjfInteractive)) */

        if ((bmisset(&J->IFlags,mjifUseDRMJobID)) && (J->DRMJID != NULL))
          {
          if ((StageR->Type == mrmtPBS) || 
              ((StageR->Type == mrmtNative) && (StageR->SubType == mrmstXT4)))
            {
            MJobGetName(NULL,J->DRMJID,StageR,JobName,sizeof(JobName),mjnShortName);
            }
          else
            {
            MUStrCpy(JobName,J->DRMJID,sizeof(JobName));
            }

          /* NOTE:  If JobName[] is populated, MRMJobSubmit will preserve this 
                    name as the job id
          */

          /* rename the job unless it is the master in a job array */
          if (strcmp(J->Name,JobName) && (J->Array == NULL))
            {
            MJobRename(J,JobName);
            }
          }    /* END if ((bmisset(&J->IFlags,mjifUseDRMJobID)) && ...) */

        if (!bmisset(&J->IFlags,mjifDataOnly) ||
           (J->Env.Cmd == NULL))
          {
          if (bmisset(&J->IFlags,mjifRemote))
            {
            sprintf(tmpLine,"%s:job %s successfully submitted\n",
              J->Name,
              J->Name);
            }
          else
            {
            /* return only job id */

            sprintf(tmpLine,"%s",
              J->Name);
            }
          }
        else
          {
          if (bmisset(&J->IFlags,mjifRemote))
            {          
            sprintf(tmpLine,"%s:job %s staged to %s successfully|%s\n",
              J->Name,
              J->Name,
              J->Env.Cmd,
              J->Env.Cmd);
            }
          else
            {
            sprintf(tmpLine,"INFO:    job %s staged to %s successfully\n",
              J->Name,
              J->Env.Cmd);
            }
          }
        }    /* END if (MSched.JobMigratePolicy == mjmpAuto) */
      else if ((MSched.JobMigratePolicy == mjmpImmediate) || 
               (bmisset(&J->Flags,mjfInteractive)))
        { 
        char NewJobName[MMAX_NAME];

        mbitmap_t IFlags;
        mbitmap_t JobHolds;

        mbool_t IsRemoteJob = FALSE;
   
        char *CmdFile = NULL;
        char *Name    = NULL;

        MUStrDup(&CmdFile,J->Env.Cmd);
        MUStrDup(&Name,J->Name);

        bmor(&IFlags,&J->IFlags);

        IsRemoteJob = bmisset(&J->IFlags,mjifRemote);
        bmcopy(&JobHolds,&J->Hold);

        /* WARNING: J is destroyed, if not interactive, upon MJobMigrate success! */

        if ((StageR->Type != mrmtMoab) &&
            (StageR->SyncJobID == FALSE))
          bmset(&J->IFlags,mjifUseDRMJobID);

        if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRMID != NULL))
          {
          /* NOTE:  job was evaluated against StageR resources but must be
                    migrated to specified 'application' RM to utilize resources
          */

          if ((J->TemplateExtensions->WorkloadRM != NULL) ||
              (MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM) == SUCCESS))
            {
            StageR = J->TemplateExtensions->WorkloadRM;
            }
          }  /* END if ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */

        NewJobName[0] = '\0';

        if ((rc = MJobMigrate(
              &J,
              StageR,
              (bmisset(&J->Flags,mjfInteractive)) ? FALSE : TRUE, /* remove if not interactive */
              NewJobName,
              EMsg,
              &SC)) == FAILURE)
          {
          if (J != NULL)
            {
            __MUIJobSubmitRemove(&J); /* remove job from cache and database */
            MJobRemove(&J);
            }

          sprintf(tmpLine,"ERROR:    cannot migrate job to %s - %s\n",
            MRMType[StageR->Type],
            EMsg);

          MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

          __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

          if (Socket != NULL)
            {
            MUISAddData(Socket,tmpLine);
            MSCToMSC(SC,(enum MSFC *)&Socket->StatusCode);
            }

          MUFree(&CmdFile);
          MUFree(&Name);

          if (Msg != NULL)
            MUStrCpy(Msg,tmpLine,MMAX_LINE);

          return(FAILURE);
          }
        else
          {
          /* Job shouldn't be destroyed if interactive even though migrate
           * policy is immediate because jobs needs submit string to
           * properly call interactive job. */

          if ((J != NULL) && (bmisset(&J->Flags,mjfInteractive)))
            {
            if (J->RMSubmitString != NULL)
              {
              MUStrCpy(tmpLine,J->RMSubmitString,sizeof(tmpLine));
              }
            else
              {
              sprintf(tmpLine,"ERROR:  cannot submit interactive job\n");
              }

            MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

            __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

            /* Update the cred total usage for creds used by this job */

            MStatIncrementCredUsage(J);

            __MUIJobSubmitRemove(&J);

            if (Socket != NULL)
              MUISAddData(Socket,tmpLine);

            if (Msg != NULL)
              MUStrCpy(Msg,tmpLine,MMAX_LINE);

            return(SUCCESS);
            }  /* END if (bmisset(&J->Flags,mjfInteractive)) */

          /* if peer job has data-staging batch hold, preserve it across migration */

          MSysRaiseEvent();

          if (bmisset(&JobHolds,mhUser) &&
              (IsRemoteJob == TRUE))
            {
            MSDAddJobHold(NewJobName,mhUser);
            }

          if (!bmisset(&IFlags,mjifDataOnly) ||
              (CmdFile == NULL))
            {
            if (bmisset(&IFlags,mjifRemote))
              {
              sprintf(tmpLine,"%s:job %s successfully submitted\n",
                NewJobName,
                NewJobName);
              }
            else
              {
              sprintf(tmpLine,"%s",
                NewJobName);
              }
            }
          else
            {
            if (bmisset(&IFlags,mjifRemote))
              {
              sprintf(tmpLine,"%s:job %s staged to %s successfully|%s\n",
                Name,
                Name,
                CmdFile,
                CmdFile);
              }
            else
              {
              sprintf(tmpLine,"INFO:    job %s staged to %s successfully\n",
                Name, 
                CmdFile);
              }
            }

          MUFree(&CmdFile);
          MUFree(&Name);
          }   /* END else ((rc == MJobMigrate()) == FAILURE) */
        }     /* END else if (MSched.JobMigratePolicy == mjmpImmediate) */
      }       /* END if (!bmisset(&J->Flags,mjfNoRMStart) && (RMCount == 1)) */
    else if (!bmisset(&J->Flags,mjfNoRMStart) &&
            (CanRunCurIteration == TRUE) &&
           ((MSched.JobMigratePolicy == mjmpImmediate) || (MSched.JobMigratePolicy == mjmpLoadBalance)))
      {
      mpar_t *BestP = NULL;

      long    BestStartTime = 0;
      
      char    NewJobName[MMAX_NAME];

      enum MStatusCodeEnum SC;

      mbool_t IsRemoteJob;
      mbitmap_t JobHolds;
     
      char    tEMsg[MMAX_LINE] = {0};

      int     tmpTC;
 
      /* must migrate job immediately */

      /* determine best partition */

      if (MSched.JobMigratePolicy == mjmpLoadBalance)
        {
        /* NOTE:  customize per ORNL's needs (NYI) */

        if ((J->Request.TC == 1) && (J->Size > 0.0))
          {
          /* hack for XT4! - 'size' attribute not mapped to procs until after job 
             is submitted to and reported by RM */

          tmpTC = (int)J->Size;

          J->Request.TC = tmpTC;

          if (J->Req[0] != NULL)
            J->Req[0]->TaskCount = tmpTC;
          }
        else
          {
          tmpTC = J->Request.TC;
          }

        MJobGetEStartTime(
          J,               /* I job                                */
          &BestP,          /* I/O (optional) constraining/best partition */
          NULL,            /* O Nodecount   (optional) number of nodes located */
          NULL,            /* O TaskCount   (optional) tasks located           */
          NULL,            /* O mnodelist_t (optional) list of best nodes to execute job */
          &BestStartTime,  /* I/O earliest start time possible     */
        NULL,
          TRUE,
          FALSE,
          tEMsg);          /* O char *      (optional,minsize=MMAX_LINE)       */

        if (J->Request.TC != tmpTC)
          {
          /* hack for XT4! */

          J->Request.TC = tmpTC;

          if (J->Req[0] != NULL)
            J->Req[0]->TaskCount = tmpTC;
          }
        }
      else
        {
        MJobGetEStartTime(
          J,               /* I job                                */
          &BestP,          /* I/O (optional) constraining/best partition */
          NULL,            /* O Nodecount   (optional) number of nodes located */
          NULL,            /* O TaskCount   (optional) tasks located           */
          NULL,            /* O mnodelist_t (optional) list of best nodes to execute job */
          &BestStartTime,  /* I/O earliest start time possible     */
          NULL,
          TRUE,
          FALSE,
          tEMsg);          /* O char *      (optional,minsize=MMAX_LINE)       */
        }

      if (BestP == NULL)
        {
        sprintf(tmpLine,"INFO:     job cannot run in any partition - %s\n",
          tEMsg);

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          MUISAddData(Socket,tmpLine);

        if (Msg != NULL)
          MUStrCpy(Msg,tmpLine,MMAX_LINE);

        return(FAILURE);
        }

      MDB(2,fUI) MLog("INFO:     best partition for job '%s' %s\n",
        J->Name,
        BestP->Name);

      sprintf(tmpLine,"INFO:     job %s best partition is %s\n",
        J->Name,
        BestP->Name);

      IsRemoteJob = bmisset(&J->IFlags,mjifRemote);

      bmcopy(&JobHolds,&J->Hold);

      /* WARNING!!!! when successful, MJobMigrate() destroys J */
  
      if ((BestP->RM->Type != mrmtMoab) &&
          (BestP->RM->SyncJobID == FALSE))
        {
        bmset(&J->IFlags,mjifUseDRMJobID);
        }

      StageR = BestP->RM;

      if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRMID != NULL))
        {
        /* NOTE:  job was evaluated against StageR resources but must be
                  migrated to specified 'application' RM to utilize resources
        */

        if ((J->TemplateExtensions->WorkloadRM != NULL) ||
            (MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM) == SUCCESS))
          {
          StageR = J->TemplateExtensions->WorkloadRM;
          }
        }  /* END if ((J->TX != NULL) && (J->TX->WorkloadRMID != NULL)) */

      /* NOTE:  at this point, the following has occurred:
                - job has been submitted to internal queue 
                - job has been processed through MRMJobPostLoad()
                - job defaults and job templates have been applied
       */

      if ((rc = MJobMigrate(&J,StageR,TRUE,NewJobName,EMsg,&SC)) == FAILURE)
        {
        if (J != NULL)
          {
          __MUIJobSubmitRemove(&J); /* remove job from cache and database */
          MJobRemove(&J);
          }

        /* NOTE:  should report workload RM if specified */

        sprintf(tmpLine,"ERROR:    cannot migrate job to %s - %s\n",
          MRMType[StageR->Type],
          EMsg);

        MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

        __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

        if (Socket != NULL)
          {
          MUISAddData(Socket,tmpLine);
          MSCToMSC(SC,(enum MSFC *)&Socket->StatusCode);
          }

        if (Msg != NULL)
          MUStrCpy(Msg,tmpLine,MMAX_LINE);

        return(FAILURE);
        }
      else
        {
        /* if peer job has data-staging batch hold, preserve it across migration */

        if (bmisset(&JobHolds,mhBatch) &&
            (IsRemoteJob == TRUE))
          {
          MSDAddJobHold(NewJobName,mhBatch);
          }

        if (bmisset(&JobHolds,mhUser) &&
            (IsRemoteJob == TRUE))
          {
          MSDAddJobHold(NewJobName,mhUser);
          }

        if (IsRemoteJob == TRUE)
          {
          sprintf(tmpLine,"%s:job %s successfully submitted\n",
            NewJobName,
            NewJobName);
          }
        else
          {
          sprintf(tmpLine,"%s",
            NewJobName);
          }
        }
      }    /* END else if (!bmisset(&J->Flags,mjfNoRMStart) && ..) */
    else
      {
      /* system job received, cannot immediately migrate */

      if ((bmisset(&J->Flags,mjfSystemJob) && (bmisset(&J->Flags,mjfFragment))))
        {
        /* split job up */

        MJobFragment(J,NULL,TRUE);
        }

      MSysRaiseEvent();

      MJobSetAttr(J,mjaSID,(void **)MSched.Name,mdfString,mSet);
      MJobSetAttr(J,mjaGJID,(void **)J->Name,mdfString,mSet);

      /* still send back response for non-blocking communication */

      if (bmisset(&J->IFlags,mjifRemote))
        {
        sprintf(tmpLine,"%s:job %s successfully submitted\n",
          J->Name,
          J->Name);
        }
      else
        {      
        sprintf(tmpLine,"%s",
          J->Name); 
        }
      }    /* END else ((!bmisset(&J->Flags,mjfNoRMStart) && ...) */
  
    if (J != NULL)
      {
      MJobCalcStartPriority(
        J,
        NULL,
        NULL,
        mpdJob,
        NULL,
        0,
        mfmHuman,
        FALSE);
      }

    /* NAMI billing - Create */

    if ((J != NULL) && /* Job may be deleted because of immediate jobmigration */
        (MAM[0].ValidateJobSubmission == TRUE) &&
        (MAMHandleCreate(&MAM[0],
          (void *)J,
          mxoJob,
          &MAMFailureAction,
          NULL,
          NULL) == FAILURE))
      {
      MDB(3,fSTRUCT) MLog("WARNING:  Unable to register job creation with accounting manager for job '%s', taking action '%s'\n",
        J->Name,
        MAMJFActionType[MAMFailureAction]);

      if ((!bmisset(&J->IFlags,mjifRunAlways)) &&
          (!bmisset(&J->IFlags,mjifRunNow)))
        {
        /* Failure action */

        switch (MAMFailureAction)
          {
          case mamjfaCancel:

            snprintf(tmpLine,sizeof(tmpLine),"Unable to register job creation with accounting manager\n");

            MOWriteEvent((void *)J,mxoJob,mrelJobReject,tmpLine,NULL,NULL);

            __MUIJobCtlSubmitLogRejectEvent(J,tmpLine);

            if (J->ParentVCs != NULL)
              {
              /* Need to remove parent VCs, and if part of a workflow, remove the workflow */

              mln_t *tmpL;
              mvc_t *tmpVC;
              char VCName[MMAX_LINE];
              mbool_t WorkflowFound = TRUE;

              while (WorkflowFound == TRUE)
                {
                /* WorkflowFound is because MVCRemoveObject will modify the list
                    as we're iterating over it.  We'll just break and start over. */

                WorkflowFound = FALSE;

                for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
                  {
                  tmpVC = (mvc_t *)tmpL->Ptr;

                  if (tmpVC == NULL)
                    break;

                  if (bmisset(&tmpVC->Flags,mvcfWorkflow))
                    {
                    MUStrCpy(VCName,tmpVC->Name,sizeof(tmpVC->Name));

                    MVCRemoveObject(tmpVC,(void *)J,mxoJob);

                    /* tmpVC may have been removed by MVCRemoveObject */
                    /* Workflow hasn't started, don't need to wait for sub-objects to destroy VC */

                    if (MVCFind(VCName,&tmpVC) == SUCCESS)
                      MVCRemove(&tmpVC,TRUE,FALSE,NULL);

                    WorkflowFound = TRUE;

                    break;
                    }
                  }
                }  /* END while (WorkflowFound == TRUE) */

              MVCRemoveObjectParentVCs((void *)J,mxoJob);
              }  /* END if (J->ParentVCs != NULL) */

            __MUIJobSubmitRemove(&J);

            if (Socket != NULL)
              {
              MUISAddData(Socket,tmpLine);
              }

            if (Msg != NULL)
              MUStrCpy(Msg,tmpLine,MMAX_LINE);

            return(FAILURE);

            /* NOTREACHED */

            break;

          case mamjfaHold:

            /* Just set the hold, don't exit */

            MJobSetHold(J,mhSystem,MSched.DeferTime,mhrAMFailure,NULL);

            break;

          default:

            /* NO-OP, ignore */

            break;
          }
        }  /* END if ((!bmisset(&J->IFlags,mjifRunAlways)) ...) */
      }  /* END if (MAMHandleCreate() == FAILURE) */

    if (Format != mfmXML)
      {
      if (Socket != NULL)
        MUISAddData(Socket,tmpLine);
      }
    else
      {
      /* Add the job to the XML (name only) */

      mxml_t *tmpXML = NULL;
      mln_t  *tmpVCL = NULL;
      mvc_t  *VC = NULL;

      MXMLCreateE(&tmpXML,"job");
      MXMLSetAttr(tmpXML,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);

      MXMLAddE(RetE,tmpXML);

      /* See if there was a VC created */

      for (tmpVCL = J->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
        {
        VC = (mvc_t *)tmpVCL->Ptr;

        if (VC == NULL)
          continue;

        if ((VC->CreateJob != NULL) && (!strcmp(VC->CreateJob,J->Name)))
          {
          /* Found a VC created by this job */

          mxml_t *CJ = NULL;

          MXMLCreateE(&CJ,"CreatedVC");

          MXMLSetVal(CJ,(void *)VC->Name,mdfString);

          MXMLAddE(RetE,CJ);

          break;
          }
        }
      }  /* END else if (Format == mfmXML) */
    }  /* END while (MXMLGetChild() == SUCCESS) */

  if ((Format == mfmXML) && (Socket != NULL))
    {
    /* Output the XML to the socket */

    mstring_t XMLStr(MMAX_LINE);

    MXMLToMString(RetE,&XMLStr,NULL,TRUE);

    MUISAddData(Socket,XMLStr.c_str());
    }  /* END if ((Format == mfmXML) && (Socket != NULL)) */

  if (Format == mfmXML)
    MXMLDestroyE(&RetE);

  MSched.EnvChanged = TRUE;

  MSysRaiseEvent();

#ifdef MYAHOO
  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    /* only checkpoint "system" jobs as the RM will hold other pertinent information */

    MOCheckpoint(mxoJob,(char *)J,FALSE,NULL);
    }
#endif /* MYAHOO */

  /* Update the cred total usage for creds used by this job */

  if (J != NULL)
    {
    MStatIncrementCredUsage(J);

    MJobTransition(J,FALSE,FALSE);
    }

  return(SUCCESS);
  }  /* END MUIJobSubmit() */
/* END MUIJobSubmit.c */
