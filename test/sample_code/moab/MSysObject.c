/* HEADER */

      
/**
 * @file MSysObject.c
 *
 * Contains: Transformation functions upon objects to different sink pipes
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

/* Mutex to gate access to web services queue */
mmutex_t    WSTransitionMutex;

/* Moab Web Services Queue: Protected by WSTransitionMutex */
mdllist_t  *MWSQueue;

/* Event log web service mutex */
mmutex_t    EventLogWSMutex;

/* Event log web service queue: protected by EventLogWSMutex */
mdllist_t  *EventLogWSQueue;

/* Mutex to gate access to cache transition queue (ie MObjectQueue) */
mmutex_t    CacheTransitionMutex;

/* Moab object Queue container: protected by 'CacheTransitionMutex' */
mdllist_t  *MObjectQueue;


/**
 * Create the Object queue
 */

int MObjectQueueInit()
  {
  MObjectQueue = MUDLListCreate();
  return(SUCCESS);
  }

/**
 * Intialize the Mutexes referenced in this file
 *
 */

int MObjectQueueMutexInit()
  {
  /* Initialize the mutexes */
  pthread_mutex_init(&CacheTransitionMutex,NULL);
  return(SUCCESS);
  }


/**
 * Create the WS queue
 */

int MWSQueueInit()
  {
  MWSQueue = MUDLListCreate();
  return(SUCCESS);
  }


/**
 * Intialize the Mutexes referenced in this file
 *
 */

int MWSQueueMutexInit()
  {
  /* Initialize the mutexes */
  pthread_mutex_init(&WSTransitionMutex,NULL);
  return(SUCCESS);
  }

/**
 * Init Event Log Web Services queue
 */

int MEventLogWSQueueInit()
  {
  EventLogWSQueue = MUDLListCreate();
  return(SUCCESS);
  }

int MEventLogWSQueueSize()
  {
  if (EventLogWSQueue == NULL)
    return(0);

  return(MUDLListSize(EventLogWSQueue));
  }  /* END MEventLogWSQueueSize() */

/**
 * Init Event Log Web Services mutex
 */

int MEventLogWSMutexInit()
  {
  pthread_mutex_init(&EventLogWSMutex,NULL);
  return(SUCCESS);
  }


/**
 * Dequeue the first item in the transition queue to be written 
 *
 * @see MOCacheThread() - parent 
 * @see MOWebServicesThread() - parent 
 *
 * @param Queue
 * @param Mutex
 * @param OType (O) the type of object we're dequeueing
 * @param O     (O) the object itself
 */

int MOTransitionFromQueue(

  mdllist_t           *Queue,
  mmutex_t            *Mutex,
  enum MXMLOTypeEnum  *OType,
  void               **O)

  {
  mobjdb_t *P;

  if ((OType == NULL) || (O == NULL))
    {
    return(FAILURE);
    }

  if (MUDLListSize(Queue) == 0)
    {
    /* queue is empty */
    
    return(FAILURE);
    }

  MUMutexLock(Mutex);

  /* NOTE:  could be a problem on shutdown if the queue is gone and we're still
   *        trying to grab from it */

  P = (mobjdb_t *)MUDLListRemoveFirst(Queue);

  MUMutexUnlock(Mutex);

  *OType = P->OType;
  *O     = P->O;

  MUFree((char **)&P);

  return(SUCCESS);
  }  /* END MOTransitionFromQueue() */



/**
 * enqueue the given transition object to be written to the database
 *
 * @param   Queue (I)
 * @param   Mutex (I)
 * @param   OType (I) the type of object we're transitioning
 * @param   O     (I) the object itself
 */

int MOTransitionToQueue(

  mdllist_t           *Queue,
  mmutex_t            *Mutex,
  enum MXMLOTypeEnum  OType,
  void                *O)

  {
  mobjdb_t *P = NULL;

  if (Queue == NULL)
    {
    return(FAILURE);
    }

  P = (mobjdb_t *)MUMalloc(sizeof(mobjdb_t));

  P->O     = O;
  P->OType = OType;

  MUMutexLock(Mutex);

  /* place Object into queue */

  MUDLListAppend(Queue,P);

  MUMutexUnlock(Mutex);

  return(SUCCESS);
  }  /* END MOTransitionToQueue() */


/**
 * convert object to an XML object
  *
 * @param OType    (I) the type of object we're transitioning
 * @param O        (I) the transition object itself 
 * @param StatType
 * @param E        (O) the xml object
 */

int MOExportToXML(

  enum MXMLOTypeEnum     OType,
  void                  *O,
  enum MStatObjectEnum   StatType,
  mxml_t               **E)

  {

  if(O == NULL)
    return(FAILURE);

  switch (OType)
    {

    case mxoNode:

      {
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
        mnaVMOSList,
        mnaNONE };

      MNodeTransitionToXML((mtransnode_t *)O,NAList,E);
      }

      break;

    case mxoJob:

      {
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
        mjaReqNodes,       /* number of nodes requested (obsolete) */
        mjaReqProcs,       /* obsolete */
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

      MJobTransitionToXML((mtransjob_t *)O,E,JAList,JRQList,mcmNONE);
      }
  
      break;

    case mxoRsv:

      {
      const enum MRsvAttrEnum RAList[] = {
        mraName,
        mraACL,       /**< @see also mraCL */
        mraAAccount,
        mraAGroup,
        mraAUser,
        mraAQOS,
        mraAllocNodeCount,
        mraAllocNodeList,
        mraAllocProcCount,
        mraAllocTaskCount,
        mraCL,        /**< credential list */
        mraComment,
        mraCost,      /**< rsv AM lien/charge */
        mraCTime,     /**< creation time */
        mraDuration,
        mraEndTime,
        mraExcludeRsv,
        mraExpireTime,
        mraFlags,
        mraGlobalID,
        mraHostExp,
        mraHistory,
        mraLabel,
        mraLastChargeTime, /* time rsv charge was last flushed */
        mraLogLevel,
        mraMaxJob,
        mraMessages,
        mraOwner,
        mraPartition,
        mraPriority,
        mraProfile,
        mraReqArch,
        mraReqFeatureList,
        mraReqMemory,
        mraReqNodeCount,
        mraReqNodeList,
        mraReqOS,
        mraReqTaskCount,
        mraReqTPN,
        mraResources,
        mraRsvAccessList, /* list of rsv's and rsv groups which can be accessed */
        mraRsvGroup,
        mraRsvParent,
        mraSID,
        mraStartTime,
        mraStatCAPS,
        mraStatCIPS,
        mraStatTAPS,
        mraStatTIPS,
        mraSubType,
        mraTrigger,
        mraType,
        mraVariables,
        mraVMList,
        mraNONE };
  
      MRsvTransitionToXML((mtransrsv_t *)O,E,(enum MRsvAttrEnum *)RAList,NULL,mfmNONE);
      }

      break;

    case mxoTrig:

      {
      const enum MTrigAttrEnum TAList[] = {
        mtaActionData,
        mtaActionType,
        mtaBlockTime,
        mtaDescription,
        mtaDisabled,
        mtaEBuf,
        mtaEventTime,
        mtaEventType,
        mtaFailOffset,
        mtaFailureDetected,
        mtaFlags,
        mtaIsComplete,
        mtaIsInterval,
        mtaLaunchTime,
        mtaMessage,
        mtaMultiFire,
        mtaName,
        mtaObjectID,
        mtaObjectType,
        mtaOBuf,
        mtaOffset,
        mtaPeriod,
        mtaPID,
        mtaRearmTime,
        mtaRequires,
        mtaSets,
        mtaState,
        mtaThreshold,
        mtaTimeout,
        mtaTrigID,
        mtaUnsets,
        mtaNONE };

      MTrigTransitionToXML((mtranstrig_t *)O,E,(enum MTrigAttrEnum *)TAList,NULL,mfmNONE);
      }

      break;

    case mxoxVC:

      {
      const enum MVCAttrEnum VCAList[] = {
        mvcaACL,         /* ACL */
        mvcaCreateTime,  /* creation time */
        mvcaCreator,     /* creator */
        mvcaDescription, /* description */
        mvcaFlags,       /* flags (from MVCFlagEnum) */
        mvcaJobs,        /* names of jobs in VC */
        mvcaName,        /* VC name */
        mvcaNodes,       /* names of nodes in VC */
        mvcaOwner,       /* VC owner */
        mvcaRsvs,        /* names of rsvs in VC */
        mvcaVariables,   /* VC vars */
        mvcaVCs,         /* sub-VCs */
        mvcaVMs,         /* names of VMs in VC */
        mvcaNONE };

      MVCTransitionToXML((mtransvc_t *)O,E,(enum MVCAttrEnum *)VCAList,mfmNONE,TRUE);
      }

      break;

    case mxoxVM:

      {
      const enum MVMAttrEnum VMAList[] = {
        mvmaActiveOS,
        mvmaADisk,
        mvmaAlias,
        mvmaAMem,
        mvmaAProcs,
        mvmaCDisk,
        mvmaCMem,
        mvmaCProcs,
        mvmaCPULoad,
        mvmaContainerNode,
        mvmaDDisk,
        mvmaDMem,
        mvmaDProcs,
        mvmaDescription,
        mvmaEffectiveTTL,
        mvmaFlags,
        mvmaGMetric,
        mvmaID,
        mvmaJobID,
        mvmaLastMigrateTime,
        mvmaLastSubState,
        mvmaLastSubStateMTime,
        mvmaLastUpdateTime,
        mvmaMigrateCount,
        mvmaNetAddr,
        mvmaNextOS,
        mvmaOSList,
        mvmaPowerIsEnabled,
        mvmaPowerState,
        mvmaProvData,
        mvmaRackIndex,
        mvmaSlotIndex,
        mvmaSovereign,
        mvmaSpecifiedTTL,
        mvmaStartTime,
        mvmaState,
        mvmaSubState,
        mvmaStorageRsvNames,
        mvmaTrackingJob,
        mvmaVariables,
        mvmaLAST };

      MVMTransitionToXML((mtransvm_t *)O,E,(enum MVMAttrEnum *)VMAList);
      }

      break;

    case mxoxStats:

      {
      MStatToXML(O,StatType,E,FALSE,NULL);
      }

      break;

    default:

      break;
    }  /* END switch (OType) */

    return(SUCCESS);
  } /* END MOExportToXML() */



/**
 * Export the given object to web services
 *
 * @param   OType     (I) the type of object we're transitioning
 * @param   O         (I) the object itself 
 * @param   StatType  (I) stat type if applicable
 */

int MOExportToWebServices(

  enum MXMLOTypeEnum    OType,
  void                 *O,
  enum MStatObjectEnum  StatType)

  {
  mstring_t *Buffer = new mstring_t(MMAX_BUFFER);
  mxml_t *E = NULL;

  /* Make XML object from object */
  MOExportToXML(OType,O,StatType,&E);

  /* Make XML String */

  MXMLToMString(E,Buffer,NULL,TRUE);
  MXMLDestroyE(&E);

  /* Add XML string to Web Services Queue */
  if (*Buffer->c_str() != '\0')
    {
    MOTransitionToQueue(MWSQueue,&WSTransitionMutex,OType,Buffer);
    }
  else
    {
    delete Buffer;
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MOExportToWebServices() */



/**
 * Export the XML for the given event log to web services
 *
 * @param Log [I] - The event log object to send
 */

int MEventLogExportToWebServices(

  MEventLog *Log)

  {
  mxml_t *LogXML = NULL;
  mbool_t LogXMLFail = FALSE;

  const char *FName = "MEventLogExportToWebServices";

  MDB(8,fSCHED) MLog("%s(%s)\n",
    FName,
    (Log != NULL) ? MEventLogType[Log->GetType()]: "NULL");

  if (Log == NULL)
    return(FAILURE);

  if ((MSched.PushEventsToWebService == FALSE) ||
      (MSched.WebServicesConfigured == FALSE))
    return(SUCCESS);

  if (EventLogWSQueue == NULL)
    {
    /* Don't write events if EventLogWSQueue is NULL - there can be events
        generated by reading in the checkpoint/config files, but
        the queue hasn't be initialized yet */

    return(SUCCESS);
    }

  LogXML = Log->GetXML();

  if (LogXML != NULL)
    {
    mstring_t *LogXMLString = new mstring_t(MMAX_LINE);

    MXMLToMString(LogXML,LogXMLString,NULL,TRUE);

    if ((*LogXMLString->c_str() == '\0') ||
        (MOTransitionToQueue(EventLogWSQueue,&EventLogWSMutex,mxoxEvent,LogXMLString) == FAILURE))
      {
      delete LogXMLString;

      MXMLDestroyE(&LogXML);
      return(FAILURE);
      }

    MXMLDestroyE(&LogXML);
    }
  else
    {
    LogXMLFail = TRUE;
    }

  if (MSched.Recycle == TRUE)
    {
    if (Log->GetType() == meltSchedRecycle)
      MSched.EventLogWSRunning = FALSE;
    }
  else if (Log->GetType() == meltSchedStop)
    MSched.EventLogWSRunning = FALSE;

  if (LogXMLFail == TRUE)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MEventLogExportToWebServices */


/**
 * Accept an Event (in a mstring_t Buffer) and Record it to Web Services
 *
 * @param Buffer  (I)
 */

int MORecordEventToWebServices(
    
    mstring_t      *Buffer)

  {
  return(MOTransitionToQueue(MWSQueue,&WSTransitionMutex,mxoxEvent,Buffer));
  }


/**
 * Dequeue a transition object from the queue and write it to the database.
 * MOTransitionFromQueue() handles the mutex on the queue.
 *
 * @param Arg
 */

void *MOWebServicesThread(
    
  void *Arg)  /* I (not used) */

  {
#ifdef __MCOMMTHREAD
  
  void *Object;
  enum MXMLOTypeEnum OType;

  mstring_t  *WSArray[MMAX_NODE_DB_WRITE];
  mstring_t  *EventLogXML;

  mwsinfo_t  *EventLogInfo = NULL;
  int Count;

  pthread_detach(pthread_self());

  MWSInfoCreate(&EventLogInfo);

  if (MSched.EventLogWSURL != NULL)
    {
    MUStrDup(&EventLogInfo->URL,MSched.EventLogWSURL);
    MUStrDup(&EventLogInfo->HTTPHeader,"Content-Type: application/xml");
    }

  MSched.EventLogWSRunning = TRUE;

  if (MSched.EventLogWSUser != NULL)
    MUStrDup(&EventLogInfo->Username,MSched.EventLogWSUser);

  if (MSched.EventLogWSPassword != NULL)
    MUStrDup(&EventLogInfo->Password,MSched.EventLogWSPassword);

  while (MSched.EventLogWSRunning == TRUE)
    {
    /* Construct a new mstring_t */
    EventLogXML = new mstring_t(MMAX_LINE);

    OType = mxoNONE;
    Object = NULL;

    Count = 0;

    /* Check for enough room in the tables before fetching a new entry */

    while ((MSched.Shutdown == FALSE) &&
           (Count < MMAX_NODE_DB_WRITE - 1) &&
           (MOTransitionFromQueue(MWSQueue,&WSTransitionMutex,&OType,&Object) != FAILURE))
      {
      MDB(7,fTRANS) MLog("INFO:      dequeueing object of type %s for web services\n",
        MXO[OType]);

      WSArray[Count] = (mstring_t *)Object;

      Count++;

      WSArray[Count] = NULL; 
      }    /* END if (MOTransitionFromQueue(&MWSQueue,&WSTransitionMutex,&OType,&O) != FAILURE) */

    /* Write out items to Web Services */
    if (Count > 0)
      {
      MDB(7,fTRANS) MLog("INFO:      writing out objects to web services\n");

      MWSWriteObjects(WSArray,Count,NULL);

      }    /* END if (Count > 0) */

    Count = 0;
    OType = mxoNONE;
    Object = NULL;

    while ((MSched.EventLogWSRunning) &&
           (MOTransitionFromQueue(EventLogWSQueue,&EventLogWSMutex,&OType,&Object) != FAILURE))
      {
      if (EventLogXML->empty())
        {
        MStringSet(EventLogXML,"<Events>");
        }

      MStringAppend(EventLogXML,((mstring_t *)Object)->c_str());

      /* destruct the mstring_t */
      delete (mstring_t *) Object;

      Count++;
      }  /* END while (... MOTransitionFromQueue(EventLogWSQueue,...) != FAILURE) */

    if ((Count > 0) && (MSched.EventLogWSURL != NULL))
      {
      MDB(7,fTRANS) MLog("INFO:     writing out event logs to web services\n");

      MStringAppend(EventLogXML,"</Events>");

      WSArray[0] = EventLogXML;
      WSArray[1] = NULL;

      MWSWriteObjects(WSArray,1,EventLogInfo);
      }
    else
      {
      /* destruct the mstring_t */
      delete (mstring_t*) EventLogXML;

      MUSleep(500,FALSE);
      }
    }  /* END while (MSched.EventLogWSRunning == TRUE) */

  /* shutdown is true - flush the queue */

  while (MOTransitionFromQueue(MWSQueue,&WSTransitionMutex,&OType,&Object) != FAILURE)
    {
    MWSWriteObjects((mstring_t **)&Object,1,NULL);
    }    /* END if (MOTransitionFromQueue(&MWSQueue,&WSTransitionMutex,&OType,&O) != FAILURE) */

  Count = 0;

  EventLogXML = new mstring_t(MMAX_LINE);

  while (MOTransitionFromQueue(EventLogWSQueue,&EventLogWSMutex,&OType,&Object) != FAILURE)
    {
    if (EventLogXML->empty())
      {
      MStringSet(EventLogXML,"<Events>");
      }

    MStringAppend(EventLogXML,((mstring_t *)Object)->c_str());

    /* destruct the mstring_t */
    delete (mstring_t *) Object;

    Count++;
    }  /* END while (MOTransitionFromQueue(EventLogWSQueue,...) != FAILURE) */

  if ((Count > 0) && (MSched.EventLogWSURL != NULL))
    {
    MDB(7,fTRANS) MLog("INFO:     writing out event logs to web services\n");

    MStringAppend(EventLogXML,"</Events>");

    WSArray[0] = EventLogXML;
    WSArray[1] = NULL;

    MWSWriteObjects(WSArray,1,EventLogInfo);
    }
  else
    {
    delete EventLogXML;
    }

  MWSInfoFree(&EventLogInfo);
#endif /* __MCOMMTHREAD */

  /*NOTREACHED*/

  return(NULL);
  }  /* END MOWebServicesThread() */



/**
 * Export the given transition object to web services, the cache
 * and database 
 *
 * @param   OType (I) the type of object we're transitioning
 * @param   O     (I) the object itself
 */

int MOExportTransition(

  enum MXMLOTypeEnum  OType,
  void               *O)

  {
  /* Add transition object to Web Services */
  if ((MSched.WebServicesURL) && (MSched.PushCacheToWebService == TRUE))
    {
    /* Don't export if it won't be sent to web services because then
        it won't be freed */

    MOExportToWebServices(OType,O,msoNONE);
    }

  /* Add transition object Cache Queue */
  MOTransitionToQueue(MObjectQueue,&CacheTransitionMutex,OType,O);

  return(SUCCESS);
  }  /* END MOExportTransition() */





/**
 * Dequeue a transition object from the queue and write it to the database.
 * MOTransitionFromQueue() handles the mutex on the queue.
 *
 * @param Arg 
 *  
 * NOTE: Due to the way we're handling threads it is possible 
 * that this thread does not exit gracefully but get killed by 
 * the main thead if it takes too long to exit. This is 
 * especially prone to happen when connected to a database. 
 * Watch out for memory leaks when cleaning up. 
 */

void *MOCacheThread(
    
  void *Arg)  /* I (not used) */

  {
#ifdef __MCOMMTHREAD
  
  mdb_t MyDB;

  void *O;
  enum MXMLOTypeEnum OType;

  mtransnode_t *Nodes[MMAX_NODE_DB_WRITE];
  mtransjob_t  *Jobs[MMAX_JOB_DB_WRITE];
  mtransrsv_t  *Rsvs[MMAX_RSV_DB_WRITE];
  mtranstrig_t *Triggers[MMAX_TRIG_DB_WRITE];
  mtransvc_t   *VCs[MMAX_VC_DB_WRITE];
  mtransvm_t   *VMs[MMAX_VC_DB_WRITE];

  int jindex;
  int nindex;
  int rindex;
  int tindex;
  int vcindex;
  int vmindex;

  pthread_detach(pthread_self());

  memset(&MyDB,0,sizeof(MyDB));

  if (MSysInitDB(&MyDB) == FAILURE)
    {
    /* for now this is ignored, we'll still update the cache */
    }

  MCacheSysInitialize();

  while (MSched.Shutdown == FALSE)
    {
    MyDB.DBType = MSched.ReqMDBType;

    OType = mxoNONE;
    O = NULL;

    nindex = 0;
    jindex = 0;
    rindex = 0;
    tindex = 0;
    vcindex = 0;
    vmindex = 0;

    /* Check for enough room in the tables before fetching a new entry */

    while ((MSched.Shutdown == FALSE) &&
           (nindex < MMAX_NODE_DB_WRITE - 1) &&
           (jindex < MMAX_JOB_DB_WRITE - 1)  &&
           (rindex < MMAX_RSV_DB_WRITE - 1)  &&
           (tindex < MMAX_TRIG_DB_WRITE - 1) &&
           (vcindex < MMAX_VC_DB_WRITE - 1) &&
           (vmindex < MMAX_VM_DB_WRITE - 1) &&
           (MOTransitionFromQueue(MObjectQueue,&CacheTransitionMutex,&OType,&O) != FAILURE))
      {
      MDB(7,fTRANS) MLog("INFO:      queueing object of type %s for the cache\n",
        MXO[OType]);

      switch (OType)
        {
        case mxoNode:

          Nodes[nindex] = (mtransnode_t *)O;

          nindex++;

          Nodes[nindex] = NULL;

          break;

       case mxoJob:

          Jobs[jindex] = (mtransjob_t *)O;

          jindex++;

          Jobs[jindex] = NULL;
      
          break;

       case mxoRsv:

          Rsvs[rindex] = (mtransrsv_t *)O;

          rindex++;

          Rsvs[rindex] = NULL;

          break;

       case mxoTrig:

          Triggers[tindex] = (mtranstrig_t *)O;

          tindex++;

          Triggers[tindex] = NULL;

          break;

        case mxoxVC:

          VCs[vcindex] = (mtransvc_t *)O;

          vcindex++;

          VCs[vcindex] = NULL;

          break;

        case mxoxVM:

          VMs[vmindex] = (mtransvm_t *)O;

          vmindex++;

          VMs[vmindex] = NULL;

          break;

        default:

          break;
        }  /* END switch (OType) */
      }    /* END if (MOTransitionFromQueue(&OType,&O) != FAILURE) */

    if ((nindex > 0) || (jindex > 0) || (rindex > 0) || (tindex > 0) || (vcindex > 0) || (vmindex > 0))
      {
      MDB(7,fTRANS) MLog("INFO:      writing out objects to cache (jobs=%d) (nodes=%d) (rsvs=%d) (triggers=%d) (vcs=%d) (vms=%d)\n",
        jindex,
        nindex,
        rindex,
        tindex,
        vcindex,
        vmindex);

      if (nindex > 0)
        {
        if (MyDB.DBType == mdbODBC)
          MDBWriteObjectsWithRetry(&MyDB,(void **)Nodes,mxoNode,NULL);

        for (nindex = 0;Nodes[nindex] != NULL;nindex++)
          {
          /* NOTE:  MCacheWriteNode consumes mtransnode_t memory and frees it upon shutdown. */

          MCacheWriteNode(Nodes[nindex]);
          }

        nindex = 0;
        }  /* END for (nindex) */

      if (jindex > 0)
        {
        if (MyDB.DBType == mdbODBC)
          MDBWriteObjectsWithRetry(&MyDB,(void **)Jobs,mxoJob,NULL);

        for (jindex = 0;Jobs[jindex] != NULL;jindex++)
          {
          /* NOTE:  MCacheWriteJob consumes mtranjob_t memory and frees it upon shutdown. */

          MCacheWriteJob(Jobs[jindex]);
          }

        jindex = 0;
        }  /* END for (jindex) */

      if (rindex > 0)
        {
        if (MyDB.DBType == mdbODBC)
          MDBWriteObjectsWithRetry(&MyDB,(void **)Rsvs,mxoRsv,NULL);

        for (rindex = 0;Rsvs[rindex] != NULL;rindex++)
          {
          /* NOTE:  MCacheWriteNode consumes mtransrsv_t memory and frees it upon shutdown. */

          MCacheWriteRsv(Rsvs[rindex]);
          }

        rindex = 0;
        }  /* END for (rindex) */

      if (tindex > 0)
        {
        if (MyDB.DBType == mdbODBC)
          MDBWriteObjectsWithRetry(&MyDB,(void **)Triggers,mxoTrig,NULL);

        for (tindex = 0;Triggers[tindex] != NULL;tindex++)
          {
          /* NOTE:  MCacheWriteTrigger consumes mtranstrig_t memory and frees it upon shutdown. */

          MCacheWriteTrigger(Triggers[tindex]);
          }

        tindex = 0;
        }  /* END for (tindex) */

      if (vcindex > 0)
        {
        if (MyDB.DBType == mdbODBC)
          MDBWriteObjectsWithRetry(&MyDB,(void **)VCs,mxoxVC,NULL);

        for (vcindex = 0;VCs[vcindex] != NULL;vcindex++)
          {
          /* NOTE:  MCacheWriteVC consumes mtransvc_t memory and frees it upon shutdown. */

          MCacheWriteVC(VCs[vcindex]);
          }

        vcindex = 0;
        }  /* END if (vcindex > 0) */

      if (vmindex > 0)
        {

        for (vmindex = 0;VMs[vmindex] != NULL;vmindex++)
          {
          /* NOTE:  MCacheWriteVM consumes mtransvm_t memory and frees it upon shutdown. */

          MCacheWriteVM(VMs[vmindex]);
          }

        vmindex = 0;
        }  /* END if (vcindex > 0) */

      }    /* END if ((nindex > 0) || (jindex > 0)) */
    else
      {
      MUSleep(2000,FALSE);
      }
    }  /* END while (MSched.Shutdown != FALSE) */

  /* shutdown is true - cleanup the transition queue */

  while (MOTransitionFromQueue(MObjectQueue,&CacheTransitionMutex,&OType,&O) != FAILURE)
    {
    switch (OType)
      {
      case mxoJob:

        MJobTransitionFree(&O);

        break;

      case mxoNode:

        MNodeTransitionFree(&O);

        break;

      case mxoRsv:

        MRsvTransitionFree(&O);

        break;

      case mxoTrig:

        MTrigTransitionFree(&O);

        break;

      case mxoxVC:

        MVCTransitionFree(&O);

        break;

      case mxoxVM:

        MVMTransitionFree(&O);

        break;

      default:

        break;
      }  /* END switch (OType) */
    }    /* END if (MOTransitionFromQueue(&OType,&O) != FAILURE) */

  /* Clean up the memory cache */

  MCacheSysShutdown();

  /* Free the MyDB pointer from the DB Hash Table so we don't have a bad dangling pointer */

  MUHTRemoveInt(&MDBHandles,MUGetThreadID(),(mfree_t)MDBFreeDPtr);

#endif /* __MCOMMTHREAD */

  return(NULL);
  }  /* END MOCacheThread() */







/* END MSysObject.c */
