/* HEADER */

/**
 * @file MUIPeerQueryThreadSafe.c
 *
 * Contains MUI Peer Resources Query function, threadsafe
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Process a peer query request for jobs and nodes in a threadsafe way.
 *
 * @param S       (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth    (I)
 */

int MUIPeerResourcesQueryThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  mrm_t  *PeerRM = NULL;
  mxml_t *RE = NULL;
  mxml_t *DE = NULL;

  enum MXMLOTypeEnum OType;
  char ArgString[MMAX_LINE];

  MDB(7,fSCHED) MLog("INFO:     gathering cluster data for peer request.");

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");
    MCacheJobLock(FALSE,TRUE);
    return(FAILURE);
    }

  RE = S->RDE;
  DE = S->SDE;

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
  OType = (enum MXMLOTypeEnum)MUGetIndexCI(ArgString,MXO,TRUE,mxoNONE);

  /* Finding the peer rm is NOT THREADSAFE but we should be ok because rm's
    * aren't created and destroyed on the fly. */

  MRMFindByPeer(Auth + strlen("peer:"),&PeerRM,TRUE);

  if (PeerRM == NULL)
    return(FAILURE);

  switch (OType)
    {
    case mxoALL:
    case mxoNode:

      MUIPeerNodeQueryThreadSafe(PeerRM,DE);
    
      if (OType == mxoNode) /* Let mxoALL do nodes and jobs */
        break;

    case mxoJob:

      MUIPeerJobQueryThreadSafe(PeerRM,Auth + strlen("peer:"),DE);

      break;

    default:

      /* NO-OP */
      break;
    }

  return(SUCCESS);
  } /* END int MUIPeerResourcesQueryThreadSafe() */


/**
 * Reports nodes to peer.
 *
 * @param PeerRM (I)
 * @param DE (O)
 */

int MUIPeerNodeQueryThreadSafe(

  mrm_t  *PeerRM, /* I */
  mxml_t *DE)     /* O */

  {
  marray_t NList;

  int nindex;
  mtransnode_t *N;
  mxml_t       *NE = NULL;

  enum MNodeAttrEnum NAList[] = {
    mnaNodeID,
    mnaArch,
    mnaAvlGRes,
    mnaCfgGRes,
    /* mnaAvlClass, depends by C->RMAList, handled in ExportXML */
    /* mnaAvlETime, not handled by MNodeSetAttr so probably not used */
    /* mnaAvlSTime, not handled by MNodeSetAttr so probably not used */
    /* mnaAvlTime,  not handled by MNodeSetAttr so probably not used */
    /* mnaCfgClass, depends on C->RMAList handled in ExportXML*/
    mnaChargeRate,
    mnaDynamicPriority, 
    mnaEnableProfiling,
    mnaFeatures,
    mnaGMetric,
    mnaHopCount,
    mnaHVType,
    mnaIsDeleted,
    mnaJobList,
    mnaLastUpdateTime,
    mnaLoad,
    mnaMaxJob,
    mnaMaxJobPerUser,
    mnaMaxProcPerUser,
    mnaMaxLoad,
    mnaMaxProc,
    mnaOldMessages,
    mnaNetAddr,
    mnaNodeState,
    /* mnaNodeSubState, not handled by MNodeSetAttr so probably not used */
    mnaOperations,
    mnaOS,
    mnaOSList,
    mnaOwner,
    mnaResOvercommitFactor,
    mnaResOvercommitThreshold,
    mnaPartition,
    mnaPowerIsEnabled,
    mnaPowerPolicy,
    mnaPowerSelectState,
    mnaPowerState,
    mnaPriority,
    mnaPrioF, 
    mnaProcSpeed,
    mnaProvData,
    mnaRADisk,
    mnaRAMem,
    mnaRAProc,
    mnaRASwap,
    mnaRCDisk,
    mnaRCMem,
    mnaRCProc,
    mnaRCSwap,
    mnaRsvCount,
    mnaRsvList,
    mnaRMList,
    mnaSize,
    mnaSpeed,
    mnaSpeedW,
    mnaStatATime,
    mnaStatMTime,
    mnaStatTTime,
    mnaStatUTime,
    mnaTaskCount,
    mnaVariables,
    mnaVMOSList,
    mnaNONE };

  if (PeerRM == NULL)
    return(FAILURE);

  /* read all nodes from the cache */
  MUArrayListCreate(&NList,sizeof(mtransnode_t *),10);

  MCacheNodeLock(TRUE,TRUE);
  MCacheReadNodes(&NList,NULL);

  for (nindex = 0; nindex < NList.NumItems; nindex++)
    {
    N = (mtransnode_t *)MUArrayListGetPtr(&NList,nindex);

    /* Don't return global node for slaves */
    if ((MSched.Mode == msmSlave) &&
        (strcmp(N->Name,"GLOBAL") == 0))
      continue;

    /* Don't report peer nodes back to peer. */
    if (N->HopCount >= PeerRM->MaxNodeHop)
      continue;

    MNodeTransitionToXML(N,NAList,&NE);
    MNodeTransitionToExportXML(N,NULL,&NE,PeerRM);

    MXMLAddE(DE,NE);
    }

  MCacheNodeLock(FALSE,TRUE);
  MUArrayListFree(&NList);

  return(SUCCESS);
  } /* END int MUIPeerNodeQueryThreadSafe() */



/**
 * Report jobs to peer.
 *
 * @param PR    (I) Peer resource manager
 * @param PName (I) Peer name (doens't have .INBOUND like PeerRM->Name has)
 * @param DE    (O) xml to pass back
 */

int MUIPeerJobQueryThreadSafe(

  mrm_t   *PR,
  char    *PName,
  mxml_t  *DE)

  {
  mxml_t      *JE = NULL;
  char         OrigFlags[MMAX_LINE];
  mbool_t      JobFlagsModified = FALSE;
  mbool_t      ShowOnlyGridJobs;
  mtransjob_t *J;
  marray_t     JList;
  int          jindex;
  mbitmap_t    tmpFlags;

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
    mjaIWD,            /* execution directory */
    mjaJobID,          /* job batch id */
    mjaJobName,        /* user specified job name */
    mjaJobGroup,       /* job group id */
    mjaMinPreemptTime, /* earliest duration after job start at which job can be preempted */
    mjaMessages,       /* new style message - mmb_t */
    mjaNotification,   /* notify user of specified events */
    mjaNotificationAddress, /* the address to send user notifications to */
    /* mjaOFile,          must check grid mapping - RM output file path */
    /* mjaPAL,            partition access list */
    /* mjaQOS,            must check cred-mapping */
    /* mjaQOSReq,         must check cred-mapping */
    mjaReqAWDuration,  /* req active walltime duration */
        /* do not send mjaReqNodes or mjaReqProcs to peer because the values sent are */
        /* not necessarily correct and not authoritative.  In any case, the real      */
        /* numbers will be calculated by the master. MOAB-3149 */
    /*mjaReqNodes,        number of nodes requested (obsolete) */
    /*mjaReqProcs,        obsolete */
    mjaReqReservationPeer, /* required reservation originally received from peer */
    mjaReqSMinTime,    /* req earliest start time */
    /* mjaRM,             master source job rm */
    mjaRMXString,      /* RM extension string */
    /* mjaSID, */
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
    mjaVariables,
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
    mrqaNONE };

  ShowOnlyGridJobs = MUIPeerShowOnlyGridJobs(PR);

  /* read all jobs from the cache */
  MUArrayListCreate(&JList,sizeof(mtransjob_t *),10);

  MCacheJobLock(TRUE,TRUE);
  MCacheReadJobs(&JList,NULL);

  for (jindex = 0; jindex < JList.NumItems; jindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(&JList,jindex);

    JobFlagsModified = FALSE;

    JE = NULL;

    /* J is a mtransjob_t and it's flags are a string. Convert to BM in case
     * NORMStart or ClucsterLock needs to be applied. tmpFlags will be 
     * initialized even if J->Flags in empty. */

    bmclear(&tmpFlags);

    MJobFlagsFromString(NULL,NULL,&tmpFlags,NULL,NULL,J->Flags,TRUE);

    if (bmisset(&tmpFlags,mjfAdvRsv))
      {
      bmunset(&tmpFlags,mjfAdvRsv);
      JobFlagsModified = TRUE;
      }

    if (ShowOnlyGridJobs == TRUE)
      {
      /* only report jobs owned by peer */

      if ((MUStrIsEmpty(J->SystemID)) || strcmp(J->SystemID,PName))
        {
        MDB(7,fUI) MLog("INFO:     job '%s' not being reported to peer '%s' (belongs to '%s')\n",
          J->Name,
          PName,
          (!MUStrIsEmpty(J->SystemID)) ? J->SystemID : "NULL");

        continue;
        }
      }
    else if (J->State == mjsCompleted)
      {
      if ((!MUStrIsEmpty(J->SourceRM)) && (strcmp(PName,J->SourceRM) == 0))
        {
        MDB(7,fUI) MLog("INFO:     completed job '%s' not being reported to peer '%s' (belongs to '%s')\n",
          J->Name,
          PName,
          (!MUStrIsEmpty(J->SourceRM)) ? J->SourceRM : "NULL");

        continue;
        }

      if ((MUStrIsEmpty(J->SystemID)) || (strcmp(J->SystemID,PName)))
        {
        /* exporting local job - signify that this job was scheduled locally */

        bmset(&tmpFlags,mjfClusterLock);
        JobFlagsModified = TRUE;
        }
      }
    else if ((MUStrIsEmpty(J->SystemID)) || (strcmp(J->SystemID,PName)))
      {
      /* don't send job info if peer already knows about it through migration */

      if ((!MUStrIsEmpty(J->DestinationRM)) && (strcmp(PName,J->DestinationRM) == 0))
        continue;

      /* job is not owned by current querying peer */

      if ((MSched.Mode == msmNormal) || (MSched.Mode == msmMonitor))
        {
        /* signify that this job should NOT be scheduled by remote peer */

        bmset(&tmpFlags,mjfNoRMStart);
        JobFlagsModified = TRUE;
        }

      /* NOTE: eventually we will NOT want to lock jobs that are in the internal queue of the SLAVE
         so that we can possibly migrate it to another cluster */

      if ((!MUStrIsEmpty(J->SourceRM)) &&
          ((J->SourceRMType != mrmtMoab) ||
           (J->SourceRMIsLocalQueue == TRUE)))
        {
        /* reporting local job - signify that this job should not leave cluster */

        bmset(&tmpFlags,mjfClusterLock);
        JobFlagsModified = TRUE;
        }
      }

    /* creates 'MXO[mxoJob]' children */

    MDB(7,fUI) MLog("INFO:     reporting job '%s' to peer '%s'\n",
      J->Name,
      PName);

    if (JobFlagsModified == TRUE)
      {
      mstring_t Flags(MMAX_LINE);

      MUStrCpy(OrigFlags,J->Flags,sizeof(OrigFlags));
      MJobFlagsToMString(NULL,&tmpFlags,&Flags);

      /* MUStrDup will free */

      MUStrDup(&J->Flags,Flags.c_str());
      }

    MJobTransitionToXML(J,&JE,JAList,JRQList,mcmNONE);
    MJobTransitionToExportXML(J,&JE,PR);
 
    /* restore local jobs to their original status */

    if (JobFlagsModified == TRUE)
      {
      MUStrDup(&J->Flags,OrigFlags);
      }
      
    MXMLAddE(DE,JE);
    }  /* END for (J) */

  bmclear(&tmpFlags);

  MCacheJobLock(FALSE,TRUE);
  MUArrayListFree(&JList);

  return(SUCCESS);
  } /* END MUIPeerJobQueryThreadSafe() */

/* END MUIPeerQueryThreadSafe.c */
