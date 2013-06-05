/* HEADER */

/**
 * @file MUIPeer.c
 *
 * Contains MUI Peer Functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report job status to peer.
 *
 * @see MMWorkloadQuery() - peer - generate peer/grid request and process results
 * @see MJobToExportXML() - child 
 * @see MJobToXML() - child
 *
 * @param PR (I)
 * @param Auth (I)
 * @param DE (I) [modified]
 * @param ShowOnlyGridJobs (I)
 * @param DiagOpt (I) [optional]
 */

int MUIPeerJobQuery(

  mrm_t   *PR,                /* I */
  char    *Auth,              /* I */
  mxml_t  *DE,                /* I (modified) */
  mbool_t  ShowOnlyGridJobs,  /* I */
  char    *DiagOpt)           /* I (optional) */

  {
  mjob_t *J;
  
  mxml_t *JE;

  mbool_t WasClusterLocked;
  mbool_t WasNoRMStart;

  job_iter JTI;

  char *PName = NULL;

  /* list of job attributes which are not affected by grid-mapping or information
     control policies */

  enum MJobAttrEnum JAList[] = {
    /* mjaAccount,        must check cred-mapping */
    /* mjaAllocNodeList,  special grid treatment like mrqaAllocNodeList */
    mjaArgs,           /* job executable arguments */
    mjaAWDuration,     /* active wall time consumed */
    mjaBypass,
    /* mjaClass,          must check cred-mapping */
    mjaCmdFile,        /* command file path */
    mjaCompletionCode, 
    mjaCompletionTime,
    mjaCost,           /* cost a executing job */
    mjaCPULimit,       /* job CPU limit */
    mjaDepend,         /* dependencies on status of other jobs */
    mjaDescription,    /* old-style message - char* */
    mjaDRM,            /* master destination job rm */
    mjaDRMJID,         /* destination rm job id */
    mjaEEWDuration,    /* eff eligible duration:  duration job has been eligible for scheduling */
    /* mjaEFile,          must check grid mapping - RM error file path */
    mjaEnv,            /* job execution environment variables */
    mjaEState,         /* expected state */
    mjaExcHList,       /* exclusion host list */
    mjaFlags,
    mjaGAttr,          /* requested generic attributes */
    mjaGJID,           /* global job id */
    /* mjaGroup,          must check cred-mapping */
    mjaHold,           /* hold list */
    mjaReqHostList,    /* requested hosts */
    mjaHoldTime,       /* time since job had hold placed on it */
    mjaIsInteractive,  /* job is interactive */
    mjaIsRestartable,  /* job is restartable (NOTE: map into specflags) */
    mjaIsSuspendable,  /* job is suspendable (NOTE: map into specflags) */
    mjaIWD,            /* execution directory */
    mjaJobID,          /* job batch id */
    mjaJobName,        /* user specified job name */
    mjaJobGroup,       /* job group id */
    mjaMasterHost,     /* host to run primary task on */
    mjaMinPreemptTime, /* earliest duration after job start at which job can be preempted */
    mjaMessages,       /* new style message - mmb_t */
    mjaNotification,   /* notify user of specified events */
    mjaNotificationAddress, /* the address to send user notifications to */
    /* mjaOFile,          must check grid mapping - RM output file path */
    /* mjaPAL,            partition access list */
    /* mjaQOS,            must check cred-mapping */
    /* mjaQOSReq,         must check cred-mapping */
    mjaReqAWDuration,  /* req active walltime duration */
    mjaReqNodes,       /* number of nodes requested (obsolete) */
    mjaReqProcs,       /* obsolete */
    mjaReqReservation, /* required reservation */
    mjaReqReservationPeer, /* required reservation originally received from peer */
    mjaReqSMinTime,    /* req earliest start time */
    /* mjaRM,             master source job rm */
    mjaRMXString,      /* RM extension string */
    mjaRsvAccess,      /* list of reservations accessible by job */
    mjaRsvStartTime,   /* reservation start time */
    mjaRunPriority,    /* effective job priority */
    /* mjaSID, */
    mjaSize,           /* job's computational size */
    mjaSRMJID,         /* source rm job id */
    mjaStartCount,
    mjaStartPriority,  /* effective job priority */
    mjaStartTime,      /* most recent time job started execution */
    mjaState,
    mjaStatMSUtl,      /* memory seconds utilized */
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaStdErr,         /* path of stderr output */
    mjaStdIn,          /* path of stdin */
    mjaStdOut,         /* path of stdout output */
    mjaStepID,
    mjaSubmitLanguage, /* resource manager language of submission request */
    mjaSubmitString,   /* string containing full submission request */
    mjaSubmitTime,
    mjaSuspendDuration, /* duration job has been suspended */
    mjaSysPrio,        /* admin specified system priority */
    mjaSysSMinTime,    /* system specified min start time */
    mjaTaskMap,        /* nodes allocated to job */
    /* mjaUser,        DO NOT PRINT -- Must check cred-mapping */
    mjaUMask,          /* umask of user submitting the job */
    mjaUserPrio,       /* user specified job priority */
    mjaUtlMem,
    mjaUtlProcs,
    mjaVariables,
    mjaVWCLimit,       /* virtual wc limit */
    mjaNONE };

  enum MReqAttrEnum JRQList[] = {
    /* mrqaAllocNodeList,  special case for grid handled below */
    /* mrqaAllocPartition, grid case below */
    mrqaIndex,
    mrqaNodeAccess,
    mrqaNodeSet,
    mrqaPref,
    mrqaReqArch,          /* architecture */
    mrqaReqDiskPerTask,
    mrqaReqMemPerTask,
    mrqaReqNodeDisk,
    mrqaReqNodeFeature,
    mrqaReqNodeMem,
    mrqaReqNodeProc,
    mrqaReqNodeSwap,
    mrqaReqOpsys,
    mrqaReqPartition,
    mrqaReqProcPerTask,
    mrqaReqSwapPerTask,
    mrqaNCReqMin,
    mrqaTCReqMin,
    mrqaTPN,
    mrqaGRes,
    mrqaCGRes,
    mrqaNONE };

  /* list of attributes which could be affected by grid mapping files or information 
     control policies */

  enum MJobAttrEnum JAList2[] = {
    mjaAccount,
    mjaBlockReason,    /* block index of reason job is not eligible */
    mjaClass,
    mjaEFile,
    mjaGroup,
    mjaOFile,
    mjaPAL,
    mjaQOS,
    mjaQOSReq,
    mjaRM,
    mjaSID,
    mjaUser,
    mjaNONE };

  enum MReqAttrEnum JRQList2[] = {
    mrqaAllocNodeList,
    mrqaAllocPartition,
    mrqaNONE };

  const char *FName = "MUIPeerJobQuery";

  MDB(7,fUI) MLog("%s(%s,%s,%s,%s,%s)\n",
    FName,
    (PR != NULL) ? PR->Name : "NULL",
    (Auth != NULL) ? Auth : "NULL",
    (DE != NULL) ? "DE" : "NULL",
    MBool[ShowOnlyGridJobs],
    (DiagOpt != NULL) ? DiagOpt : "NULL");

  if ((DE == NULL) || (PR == NULL) || (Auth == NULL))
    {
    return(FAILURE);
    }

  PName = Auth + strlen("peer:");

  /* report full cluster + workload */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    JE = NULL;

    WasClusterLocked = bmisset(&J->Flags,mjfClusterLock);
    WasNoRMStart = bmisset(&J->Flags,mjfNoRMStart);

    if (ShowOnlyGridJobs == TRUE)
      {
      /* only report jobs owned by peer */

      if ((J->SystemID == NULL) || strcmp(J->SystemID,PName))
        {
        MDB(7,fUI) MLog("INFO:     job '%s' not being reported to peer '%s' (belongs to '%s')\n",
          J->Name,
          PName,
          (J->SystemID != NULL) ? J->SystemID : "NULL");

        continue;
        }
      }
    else if ((J->SystemID == NULL) || (strcmp(J->SystemID,PName)))
      {
      /* don't send job info if peer already knows about it through migration */

      if ((J->DestinationRM != NULL) && (strcmp(PName,J->DestinationRM->Name) == 0))
        continue;

      /* job is not owned by current querying peer */

      if ((MSched.Mode == msmNormal) || (MSched.Mode == msmMonitor))
        {
        /* signify that this job should NOT be scheduled by remote peer */

        bmset(&J->Flags,mjfNoRMStart);
        }

      /* NOTE: eventually we will NOT want to lock jobs that are in the internal queue of the SLAVE
         so that we can possibly migrate it to another cluster */

      if ((J->SubmitRM != NULL) &&
          ((J->SubmitRM->Type != mrmtMoab) ||
          bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue)))
        {
        /* reporting local job - signify that this job should not leave cluster */

        bmset(&J->Flags,mjfClusterLock);
        }
      }

    /* creates 'MXO[mxoJob]' children */

    MDB(7,fUI) MLog("INFO:     reporting job '%s' to peer '%s'\n",
      J->Name,
      PName);

    MJobToXML(J,&JE,JAList,JRQList,NULL,FALSE,FALSE);
    MJobToExportXML(J,&JE,JAList2,JRQList2,NULL,FALSE,PR);

    /* restore local jobs to their original status */

    if (WasClusterLocked == FALSE)
      {
      bmunset(&J->Flags,mjfClusterLock);
      }

    if (WasNoRMStart == FALSE)
      {
      bmunset(&J->Flags,mjfNoRMStart);
      }
      
    MXMLAddE(DE,JE);
    }  /* END for (J) */

  /* show completed jobs */

  MCJobIterInit(&JTI);

  while (MCJobTableIterate(&JTI,&J) == SUCCESS)
    {
    WasClusterLocked = bmisset(&J->Flags,mjfClusterLock);
    WasNoRMStart = bmisset(&J->Flags,mjfNoRMStart);

    if (ShowOnlyGridJobs == TRUE)
      {
      if ((J->SystemID == NULL) || strcmp(J->SystemID,PName))
        {
        MDB(7,fUI) MLog("INFO:     completed job '%s' not being reported to peer '%s' (belongs to '%s')\n",
          J->Name,
          PName,
          (J->SystemID != NULL) ? J->SystemID : "NULL");
   
        continue;
        }
      }
    else 
      {
      if ((J->SubmitRM != NULL) && (strcmp(PName,J->SubmitRM->Name) == 0))
        {
        MDB(7,fUI) MLog("INFO:     completed job '%s' not being reported to peer '%s' (belongs to '%s')\n",
          J->Name,
          PName,
          (J->SubmitRM->Name != NULL) ? J->SubmitRM->Name : "NULL");

        continue;
        }

      if ((J->SystemID == NULL) || (strcmp(J->SystemID,PName)))
        {
        /* exporting local job - signify that this job was scheduled locally */

        bmset(&J->Flags,mjfClusterLock);
        }
      }

    JE = NULL;

    /* creates 'MXO[mxoJob]' children */

    MDB(7,fUI) MLog("INFO:     reporting completed job '%s' to peer '%s'\n",
      J->Name,
      PName);

    MJobToXML(J,&JE,JAList,JRQList,NULL,FALSE,FALSE);
    MJobToExportXML(J,&JE,JAList2,JRQList2,NULL,FALSE,PR);

    /* restore local jobs to their original status */

    if (WasClusterLocked == FALSE)
      {
      bmunset(&J->Flags,mjfClusterLock);
      }

    if (WasNoRMStart == FALSE)
      {
      bmunset(&J->Flags,mjfNoRMStart);
      }

    MXMLAddE(DE,JE);
    }  /* END for (jindex) */

  return(SUCCESS);
  }    /* END MUIPeerJobQuery() */




/**
 * Return true if should only report grid jobs, false otherwise.
 *
 * @param PeerRM Peer Resource Manager
 */

mbool_t MUIPeerShowOnlyGridJobs(

  mrm_t *PeerRM) /* I */

  {
  int     rmindex;
  mbool_t ShowOnlyGridJobs = FALSE;

  if (MSched.Mode == msmSlave)
    {
    int ccount = 0;

    for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
      {
      if (MRM[rmindex].Name[0] == '\0')
        break;

      if (MRMIsReal(&MRM[rmindex]) == FALSE)
        continue;

      if (bmisset(&MRM[rmindex].Flags,mrmfClient))
        ccount++;
      }  /* END for (rmindex) */

    if (ccount > 1)
      {
      /* slave with multiple masters */
      /* do not report local jobs to any master */

      ShowOnlyGridJobs = TRUE; 
      }
    }
  else
    {
    if ((PeerRM != NULL) && (bmisset(&PeerRM->Flags,mrmfLocalWorkloadExport)))
      {
      /* peer explicitly requests report of local jobs (non-slave mode)*/

      ShowOnlyGridJobs = FALSE;
      }
    else
      {
      /* don't report local jobs (non-slave mode) */

      ShowOnlyGridJobs = TRUE;
      }
    }

  return(ShowOnlyGridJobs);
  } /* mbool_t MUIPeerShowOnlyGridJobs() */




/**
 * Process the "initialization" request sent by remote Moab peers.
 *
 * @see MMInitialize()
 *
 * @param PR (I) [modified]
 * @param RE (I)
 * @param DE (I) [modified]
 * @param Auth
 * @param IFlags (I)
 */

int MUIPeerSchedQuery(

  mrm_t      *PR,
  mxml_t     *RE,
  mxml_t     *DE,
  char       *Auth,
  mbitmap_t  *IFlags)

  {
  char tmpLine[MMAX_LINE];
  mbitmap_t DFlags;

  enum MFormatModeEnum DFormat;

  int  cindex;

  mclass_t *C;

  if ((PR == NULL) || (RE == NULL) || (DE == NULL))
    {
    return(FAILURE);
    }

  if (bmisset(IFlags,mcmXML))
    {
    DFormat = mfmXML;
    }
  else
    {
    DFormat = mfmNONE;
    }

  /* NOTE:  query may also make local changes */

  if (MXMLGetAttr(RE,"reqclasslist",NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    char *ptr;
    char *TokPtr;

    mclass_t *C;

    mrm_t    *R;

    R = &MRM[0];

    ptr = MUStrTok(tmpLine,",",&TokPtr);

    while (ptr != NULL)
      {
      if (MClassAdd(ptr,&C) == SUCCESS)
        {
        int rc;
        enum MStatusCodeEnum SC;

        char tmpMsg[MMAX_LINE];

        rc = MRMQueueCreate(
            C,
            R,
            C->Name,
            TRUE,
            tmpMsg,
            &SC);

        if (rc == FAILURE)
          {
          MDB(3,fUI) MLog("WARNING:  rm queue create command failed - '%s'\n",
            tmpMsg);
          }
        }    /* END if (MClassAdd(ptr,&C) == SUCCESS) */
  
      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (ptr) */
    }    /* END if (MXMLGetAttr() != FAILURE) */

  /* load info reported by peer */

  MXMLGetAttr(RE,"rmflags",NULL,tmpLine,sizeof(tmpLine));

  /* adjust local interface to peer based on info reported in the
     initialization request */

  if (strstr(tmpLine,MRMFlags[mrmfRsvImport]))
    {
    bmset(&PR->Flags,mrmfRsvExport);
    }

  /* report sched config, time info, language info, class info, and 
     vpc/virtual node info */

  switch (DFormat)
    {
    case mfmXML:

      {
      mxml_t *SE;
      
      mbitmap_t tFlags;

      char tmpLine[MMAX_LINE];
      char tmpBuf[MMAX_BUFFER];

      int  Duration;
      int  NC;

      char *ptr;
      char *TokPtr;

      mln_t *tmpL;

      mpar_t *P;
      mrsv_t *R;

      SE = NULL;

      MXMLCreateE(&SE,(char *)MXO[mxoSched]);

      /* is this the scheduler a slave and is this the only master? */

      /* RM is shared if MSched.Mode != msmSlave, or more than one 'client' rm's
         are detected */

      if (MSched.Mode == msmNormal)
        bmset(&tFlags,mrmfAutoStart);

      if (MSched.Mode == msmSlave)
        {
        bmset(&tFlags,mrmfSlavePeer);
        }

      mstring_t String(MMAX_LINE);

      bmtostring(&tFlags,MRMFlags,&String);

      MXMLSetAttr(
        SE,
        "flags",
        String.c_str(),
        mdfString);

      MXMLSetAttr(SE,"time",(void *)&MSched.Time,mdfLong);

      /* add languages that this virtual node supports */

      MRMAddLangToXML(SE);

      bmset(&DFlags,mcmVerbose);

      /* Using MCLASS_FIRST_INDEX to skip DEFAULT class */
      for (cindex = MCLASS_FIRST_INDEX;cindex < MSched.M[mxoClass];cindex++)
        {
        C = &MClass[cindex];

        if ((!bmisclear(&C->RMAList)) && !bmisset(&C->RMAList,PR->Index))
          {
          continue;
          }

        MClassShow(
          NULL,
          C->Name,
          SE,
          NULL,
          &DFlags,
          TRUE,
          mfmXML);
        }  /* END for (cindex) */

      /* send high-availability information */
      
      if (MSched.AltServerHost[0] != '\0')
        {
        mstring_t tmp(MMAX_LINE);

        MSchedAToString(
           &MSched,
           msaFBServer,
           &tmp,
           mfmVerbose);             
                
        MXMLSetAttr(SE,(char *)MSchedAttr[msaFBServer],tmp.c_str(),mdfString);
        }

      if (MSched.VarList != NULL)
        {
        if (MULLToString(MSched.VarList,TRUE,NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
          {
          MXMLSetAttr(SE,(char *)MSchedAttr[msaVarList],tmpLine,mdfString);
          }
        }
 
      MXMLAddE(DE,SE);
           
      SE = NULL;

      /* send across any vpc info if available */

      for (cindex = 0;cindex < MVPC.NumItems;cindex++)
        {
        P = (mpar_t *)MUArrayListGet(&MVPC,cindex);
     
        if ((P == NULL) || (P->Name[0] == '\0'))
          break;
     
        if (P->Name[0] == '\1')
          continue;
     
        /* vpc found */
     
        if (strcmp(P->OID,Auth))
          continue;
     
        if (MRsvFind(P->RsvGroup,&R,mraNONE) == FAILURE)
          continue;

        if (MULLCheck(R->Variables,"master",&tmpL) == FAILURE)
          continue;

        MXMLCreateE(&SE,(char *)MXO[mxoxVPC]);

        MXMLSetVal(SE,(void *)P->Name,mdfString);

        MXMLSetAttr(SE,"master",tmpL->Ptr,mdfString);

        if (MULLCheck(R->Variables,"port",&tmpL) != FAILURE)
          {
          MXMLSetAttr(SE,"port",tmpL->Ptr,mdfString);
          }

        if (MULLCheck(R->Variables,"port",&tmpL) != FAILURE)
          {
          MXMLSetAttr(SE,"port",tmpL->Ptr,mdfString);
          }

        Duration = R->EndTime - R->StartTime;

        MXMLSetAttr(SE,"starttime",&R->StartTime,mdfLong);
        MXMLSetAttr(SE,"duration",&Duration,mdfInt);

        MNLToString(&R->NL,FALSE,",",'\0',tmpBuf,sizeof(tmpBuf));

        ptr = MUStrTok(tmpBuf,", \t\n;:",&TokPtr);
   
        NC = 0;

        while (ptr != NULL)
          {
          MUINodeDiagnose(
            Auth,
            NULL,
            mxoNONE,
            NULL,
            &SE,    /* O */
            NULL,
            NULL,
            NULL,
            ptr,
            IFlags);
   
          ptr = MUStrTok(NULL,", \t\n;:",&TokPtr);

          NC++;
          }

        MXMLSetAttr(SE,"NC",&NC,mdfInt);

        MXMLAddE(DE,SE);

        SE = NULL;
        }   /* END for (cindex) */

/*
      SE = NULL;

      MXMLCreateE(&SE,"vcprofile");

      MVPCProfileToXML(NULL,Auth,SE,TRUE,0);

      MXMLAddE(DE,SE);
*/
      }  /* END (case mfmXML) */

      break;

    default:

      /* NOT SUPPORTED */

      break;
    }

  PR->IsInitialized = TRUE;

  return(SUCCESS);
  }  /* END MUIPeerSchedQuery() */





/**
 * Handle the lookup of a RM when a peer request comes in. Each authorized
 * peer should have a valid RM that corresponds with it. Note that this
 * function will create an '<PEERNAME>.INBOUND' RM if no RM already exists. It
 * will create the RM with all the proper default config, etc.
 *
 * @param S (I)
 * @param Auth (I)
 * @param ShowOnlyGridJobs (O)
 * @param PP (O)
 */

int MUIPeerSetup(

  msocket_t *S,                /* I */
  char      *Auth,             /* I */
  mbool_t   *ShowOnlyGridJobs, /* O */
  mrm_t     **PP)              /* O */

  {
  int rmindex;
  char *PName = NULL;

  char tmpName[MMAX_NAME];

  mrm_t *PR;

  if ((S == NULL) ||
      (Auth == NULL) ||
      (ShowOnlyGridJobs == NULL) ||
      (PP == NULL))
    {
    return(FAILURE);
    }

  PR = *PP;

  /* find/create an appropriate peer */
  
  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    PName = Auth + strlen("peer:");

    PR = NULL;

    if (MRMFindByPeer(PName,&PR,TRUE) == FAILURE)
      {
      /* search based on host communicating with Moab */

      /* NYI */
      }

    if ((PR != NULL) && (PR->State == mrmsNONE))
      {
      /* initialize peer RM on initial connection */

      MRMSetState(PR,mrmsActive);
      }

    *ShowOnlyGridJobs = MUIPeerShowOnlyGridJobs(PR);
    }  /* END if (!strncasecmp(Auth,"peer:",... */

  /* assuming only peer-to-peer client makes these queries */

  if (PR != NULL)
    {
    PR->Type = mrmtMoab;
    }

  /* verify client rm exists */

  if (PR == NULL)
    {
    /* create mandatory client rm */

    if (strncasecmp(Auth,"peer:",strlen("peer:")))
      {
      return(FAILURE);
      }
        
    PName = Auth + strlen("peer:");

    snprintf(tmpName,sizeof(tmpName),"%s.INBOUND",
      PName);

    if (MRMAdd(tmpName,&PR) == FAILURE)
      {
      return(FAILURE);
      }

    MRMSetDefaults(PR);

    PR->Type = mrmtMoab;
    
    MRMSetState(PR,mrmsActive);

    PR->UTime = MSched.Time;

    PR->TypeIsExplicit = TRUE;

    bmset(&PR->Flags,mrmfClient);

    MUStrCpy(PR->ClientName,PName,sizeof(PR->ClientName));

    /* re-sync grid-sandboxes to new RM */

    MSRUpdateGridSandboxes(NULL);
    MRsvUpdateGridSandboxes(NULL);
    }

  /* configure peer interface */

  if (S->CSKey[0] != '\0')
    MUStrDup(&PR->P[0].CSKey,S->CSKey);

  MUStrDup(&PR->P[0].HostName,S->RemoteHost);

  snprintf(tmpName,sizeof(tmpName),"PEER:%s",
    MSched.Name);

  MUStrDup(&PR->P[0].RID,tmpName);

  /* update dest/src peer resource managers as active as well */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    if (MRM[rmindex].Name[0] == '\0')
      break;

    if (MRMIsReal(&MRM[rmindex]) == FALSE)
      continue;

    if (!bmisset(&MRM[rmindex].Flags,mrmfClient))
      continue;

    if (!strcmp(MRM[rmindex].ClientName,PR->ClientName))
      {
      MRM[rmindex].UTime = MSched.Time;
      MRMSetState(&MRM[rmindex],mrmsActive);

      /* give early warning about sandboxing */
      
      if (PR->GridSandboxExists == FALSE)
        {
        PR->GridSandboxExists = MRM[rmindex].GridSandboxExists;
        }          
      }
    }

  *PP = PR;

  return(SUCCESS);
  }  /* END MUIPeerSetup() */





/**
 *
 *
 * @param S (I)
 * @param RE (I)
 * @param Auth (I)
 * @param DE (I) [modified]
 */

int MUIPeerQueryLocalResAvail(

  msocket_t *S,    /* I */
  mxml_t    *RE,   /* I */
  char      *Auth, /* I */
  mxml_t    *DE)   /* I (modified) */

  {
  /* resource availability query */

  mxml_t *JE;
  mxml_t *WE;

  char tmpName[MMAX_NAME];
  char tmpLine[MMAX_NAME];
  char JobSpec[MMAX_BUFFER];

  if ((S == NULL) ||
      (RE == NULL) ||
      (Auth == NULL) ||
      (DE == NULL))
    {
    return(FAILURE);
    }

  /* NOTE: assuming only one where in XML */

  if (MS3GetWhere(
        RE,
        &WE,
        NULL,
        tmpName,
        sizeof(tmpName),
        tmpLine,
        sizeof(tmpLine)) == FAILURE)
    {
    /* no "where" included in query */

    return(FAILURE);
    }

  if (strcasecmp(tmpName,MXO[mxoJob]))
    {
    /* where must be of type job */

    return(FAILURE);
    }  

  /* NOTE: assuming only one child in "where" */

  if (MXMLGetChildCI(WE,(char *)MXO[mxoJob],NULL,&JE) == FAILURE)
    {
    return(FAILURE);
    }

  /* find out what type of query to make */
  
  if (MXMLGetAttr(WE,(char *)MSAN[msanFlags],NULL,tmpLine,sizeof(tmpLine)) == FAILURE)
    {
    return(FAILURE);
    }


  if (MXMLToString(JE,JobSpec,sizeof(JobSpec),NULL,TRUE) == FAILURE)
    {
    return(FAILURE);
    }

  JE = NULL;
      
  if (__MUIJobGetEStartTime(JobSpec,Auth,S,tmpLine,NULL,NULL,&JE) == FAILURE)
    {
    char EString[MMAX_LINE];

    sprintf(EString,"\nINFO:  cannot determine start time for job\n");

    MUISAddData(S,EString);

    return(FAILURE);
    }

  /* job start time information available */

  MXMLAddE(DE,JE);

  return(SUCCESS);
  }  /* END MUIPeerQueryLocalResAvail() */
/* END MUIPeer.c */
