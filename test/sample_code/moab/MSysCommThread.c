/* HEADER */

      
/**
 * @file MSysCommThread.c
 *
 * Contains: Communications thread functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

extern mmutex_t SaveNonBlockMDiagTMutex;
extern mmutex_t SaveNonBlockMDiagSMutex;
extern mmutex_t SaveNonBlockShowqMutex;


/* static Global for this file */
static int CommTID;


/**
 * Initialize an object of type mrequest_t
 *
 * @param R
 * @param S
 * @param CIndex
 * @param AFlags
 * @param Auth
 */

int MSysInitSocketRequest(

  msocket_request_t *R,
  msocket_t         *S,
  long               CIndex,
  mbitmap_t         *AFlags,
  char              *Auth)

  {
  R->S = S;
  R->CIndex = CIndex;
  bmcopy(&R->AFlags,AFlags);

  MUStrCpy(R->Auth,Auth,sizeof(R->Auth));

  return(SUCCESS);
  } /* END MSysInitSocketRequest() */


/**
 * Destruct an object of type mrequest_t
 * @param R
 */

int MSysFreeSocketRequest(

  msocket_request_t *R)

  {
  MSUFree(R->S);
  MUFree((char **)&R->S);

  return(SUCCESS);
  } /*END MSysFreeSocketRequest() */


/**
 * Process cred control request using a request handler thread  
 *
 * @param Data
 *
 * @see MUICredCtl()
 */

void MTProcessRequest(

  void *Data)

  {
  name_t tmpAuth;
  mbitmap_t tmpAFlags;
  int    SC;
  enum MSvcEnum      tmpSIndex;
  msocket_request_t *R = (msocket_request_t *)Data;
  char tmpMsg[MMAX_LINE];
  char tmpLine[MMAX_LINE];

  MUStrCpy(tmpAuth,R->Auth,sizeof(tmpAuth));
  bmcopy(&tmpAFlags,&R->AFlags);
 
  MCCToMCS((enum MClientCmdEnum)R->CIndex,&tmpSIndex);
  MSysGetAuth(tmpAuth,tmpSIndex,0,&tmpAFlags);

  if ((bmisset(&tmpAFlags,mcalAdmin1) && (MSched.Admin[1].EnableProxy)) ||
      (bmisset(&tmpAFlags,mcalAdmin2) && (MSched.Admin[2].EnableProxy)) ||  
      (bmisset(&tmpAFlags,mcalAdmin3) && (MSched.Admin[3].EnableProxy)) ||
      (bmisset(&tmpAFlags,mcalAdmin4) && (MSched.Admin[4].EnableProxy)))
    {
    char proxy[MMAX_NAME];

    MXMLGetAttr(R->S->RDE,"proxy",NULL,proxy,sizeof(proxy));

    if (proxy[0] != '\0')
      {
      MUStrCpy(tmpAuth,proxy,sizeof(tmpAuth));

      MSysGetAuth(proxy,tmpSIndex,0,&tmpAFlags);
      }
    } 

  /*
   * Call the function "handler" for the current request
   *
   * This is a 'dispatch' call to the selected function
   */
  SC = MDispatchMCRequestFunction(R->CIndex,R->S,&tmpAFlags,tmpAuth);

  if (bmisset(&MSched.RecordEventList,mrelAllSchedCommand))
    {
    MSysRecordCommandEvent(R->S,tmpSIndex,SC,tmpAuth);
    }

  if ((SC == FAILURE) && (R->S->StatusCode == 0))
    {
    MUISSetStatus(R->S,R->CIndex,999,NULL);
    }

  if (R->S->SBuffer != NULL)
    R->S->SBufSize = (long)strlen(R->S->SBuffer);

  MSUSendData(R->S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

  /* Prepare and record system event */

  if ((R->S->RDE != NULL) && (MXMLGetAttr((mxml_t *)R->S->RDE,"cmdline",NULL,tmpLine,sizeof(tmpLine)) == SUCCESS))
    {
    MUStringUnpack(tmpLine,tmpLine,sizeof(tmpLine),NULL);
    }
  else
    {
    strcpy(tmpLine,"command not available");
    }

  snprintf(tmpMsg,sizeof(tmpMsg),"command '%s' by user %s from host %s %s ",
            tmpLine,
            tmpAuth,
            (R->S->RemoteHost != NULL) ? R->S->RemoteHost : "???",
            (SC == SUCCESS) ? "succeeded" : "failed");

  MSysRecordEvent(mxoSched,MSched.Name,mrelClientCommand,tmpAuth,tmpMsg,NULL);

  /* Free socket memory */

  MSysFreeSocketRequest(R);
  MUFree((char **)&R);

  return;
  }  /* END MTProcessReqeust */


/**
 * Delegate a request to another thread, if allowed.
 *
 * @param S the socket representing the request (cannot be NULL)
 * @return SUCCESS if the request was delegated, FAILURE otherwise.
 */

int MSysDelegateRequest(

  msocket_t * S)

  {
  mxml_t *RE = S->RDE;
  char Object[MMAX_NAME] = {};
  char Action[MMAX_NAME] = {};
  char Auth[MMAX_NAME];
  char ArgString[MMAX_LINE] = {};
  char *ptr;
  char *TokPtr;
  enum MXMLOTypeEnum OIndex;
  enum MClientCmdEnum CIndex = mccNONE;
  enum MSvcEnum       SIndex;

  mbitmap_t AFlags;
  int rc = FAILURE;

#ifdef __NOMCOMMTHREAD
  /* no delegation if there are no threads! */
  return (FAILURE);
#endif  /* __NOMCOMMTHREAD */

  /*We check MSched.TPSize instead of S->TPSize because our thread pool code
   *is all global and not tied to any particular scheduler */

  if((S->WireProtocol != mwpS32) ||
      (RE == NULL) ||
      (MSched.TPSize <= 0))
    {
    return(rc);
    }
  
  if ((bmisset(&MSched.DisplayFlags,mdspfUseBlocking) == TRUE) ||
      (S->IsNonBlocking == FALSE))
    {
    /* Threaded routines not being used. */

    return(rc);
    }

  MS3GetObject(RE,Object);
  MXMLGetAttr(RE,MSAN[msanAction],NULL,Action,sizeof(Action)) ;
  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));

  MUStrCpy(Auth,S->RID,sizeof(Auth));

  ptr = MUStrTok(Object,",",&TokPtr);
  OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

  if (MSched.Shutdown == TRUE)
    {
    /* Moab is shutting down so we shouldn't respond to any threadsafe 
       commands, return FAILURE.  Possible cause for MOAB-1317. */

    /* NYI:  The main thread should not exit until all threads are accounted for
             and closed down */

    return(FAILURE);
    }

  switch(OIndex)
    {
    case mxoxEvent:

      if (!strcmp(Action,"diagnose"))
        {
        CIndex = mccEventThreadSafe;
        }

      break;

    case mxoQueue:

      if (!strcmp(Action,"query"))
        {
        CIndex = mccShowThreadSafe;
        }

      break;

    case mxoNode:

      if (!strcmp(Action,"diagnose"))
        {
        CIndex = mccNodeCtlThreadSafe;
        }

      break;

    case mxoJob:

      if (!strcmp(Action,"diagnose"))
        {
        CIndex = mccJobCtlThreadSafe;
        }
      else if ((!strcmp(Action,"submit")) &&
               (MSched.EnableHighThroughput == TRUE) && /* enables threaded msub. */
               (strncasecmp(Auth,"peer:",strlen("peer:"))))
        {
        /* Grid submissions shouldn't use the threaded msub as there are no
         * guarantees of the order that the scheduler will process the submit
         * and start requests. The start can happen before the submit. */

        /* Interactive jobs can't use the threaded msub because msub is
         * expecting to get back from the server a qsub line that it will
         * exec. The threaded msub will instead return a job id back that
         * msub will try to exec and will fail. Interactive jobs shouldn't get
         * into this code because the client won't send a nonblocking
         * interactive job submission. */

        CIndex = mccJobSubmitThreadSafe;
        }

      break;

    case mxoRsv:

      if (!strcmp(Action,"diagnose") && (S->IsNonBlocking == TRUE))
        {
        CIndex = mccRsvDiagnoseThreadSafe;
        }

      break;

    case mxoTrig:

      if (!strcmp(Action,"diagnose") && (S->IsNonBlocking == TRUE) && (bmisset(&S->Flags,mcmXML)))
        {
        CIndex = mccTrigDiagnoseThreadSafe;
        }

      break;

    case mxoSched:

      if (!strcmp(Action,"squery"))
        {
        mrm_t *PeerRM = NULL;
        enum MXMLOTypeEnum OType;
        OType = (enum MXMLOTypeEnum)MUGetIndexCI(ArgString,MXO,TRUE,mxoNONE);

        switch (OType)
          {
          case mxoALL:
          case mxoCluster:
          case mxoJob:

            /* Collapsed view and localrsvexprort only supported through old 
             * non-blocking way.  */
        
            MRMFindByPeer(Auth + strlen("peer:"),&PeerRM,TRUE);

            if ((PeerRM != NULL) && (bmisset(&PeerRM->Flags,mrmfRsvExport)))
              break;

            /* showstarts from the grid shouldn't come here because they
             * are a blocking call. */

            CIndex = mccPeerResourcesQuery;

            break;

          default:

            /* NO-OP */

            break;
          } /* switch (OType) */
        } /* END if (!strcmp(Action,"squery") && (S->IsNonBlocking == TRUE)) */

      break;

    case mxoxVC:

      if (!strcmp(Action,"query") && (S->IsNonBlocking == TRUE))
        {
        CIndex = mccVCCtlThreadSafe;
        }

      break;

    case mxoxVM:

      if (!strcmp(Action,"query") && (S->IsNonBlocking == TRUE))
        {
        CIndex = mccVMCtlThreadSafe;
        }

      break;

    default:

      CIndex = mccNONE;
      /* do nothing */
      break;
    }

  if (MSched.ReadyForThreadedCommands == FALSE)
    {
    S->IsNonBlocking = FALSE;

    return(FAILURE);
    }

  if (CIndex != mccNONE)
    {
    msocket_request_t *R = NULL;

    MCCToMCS(CIndex,&SIndex);
    MSysGetAuth(Auth,SIndex,0,&AFlags);

    if ((bmisset(&AFlags,mcalAdmin1) && (MSched.Admin[1].EnableProxy)) ||
        (bmisset(&AFlags,mcalAdmin2) && (MSched.Admin[2].EnableProxy)) ||  
        (bmisset(&AFlags,mcalAdmin3) && (MSched.Admin[3].EnableProxy)) ||
        (bmisset(&AFlags,mcalAdmin4) && (MSched.Admin[4].EnableProxy)))
      {
      char proxy[MMAX_NAME];

      MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy));

      if (proxy[0] != '\0')
        {
        MUStrCpy(Auth,proxy,sizeof(Auth));

        MSysGetAuth(proxy,SIndex,0,&AFlags);
        }
      }

    R = (msocket_request_t *)MUCalloc(1,sizeof(msocket_request_t));
    MSysInitSocketRequest(R,S,CIndex,&AFlags,S->RID);

    MTPAddRequest(0,MTProcessRequest,(void *)R);

    rc = SUCCESS;
    }

  return(rc);
  } /*END MSysDelegateRequest */

/**
 * Place a new socket into Moab's socket queue. This function uses the main
 * server socket (MSched.ServerS) directly to accept incoming connections and
 * push new sockets. This function is run in the communication thread ONLY!
 * The socket will then be "popped" off in MUIAcceptRequests() and processed.
 *
 * NOTE:  This routine will be called for EVERY new incoming request socket 
 *        regardless of whether or not multi-threading is enabled.
 *
 * @see MSysDequeueSocket() - peer
 *
 * @see ENABLEHIGHTHROUGHPUTMODE
 *
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MSysEnqueueSocket(
    
  char *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  msocket_t *S;
  char       HostName[MMAX_NAME];
  char       tmpEMsg[MMAX_LINE];

  enum MStatusCodeEnum SC;

  const char *FName = "MSysEnqueueSocket";

  MDB(11,fALL) MLog("%s(EMsg)\n",
    FName);

  /* in non-threaded build this is called every iteration by scheduler,
   * but in a threaded build this is called in a loop by the communication thread */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((S = (msocket_t *)MUCalloc(1,sizeof(msocket_t))) == NULL)
    {
    /* no memory */

    MUSleep(10000,FALSE);

    MDB(1,fUI) MLog("ALERT:    no memory to create socket structure - some client requests may be dropped\n");

    if (EMsg != NULL)
      strcpy(EMsg,"no memory");

    return(FAILURE);
    }

  MUGetMS(NULL,(long *)&S->CreateTime);

  /* routine only sets S->sd */
  
  if (MSUAcceptClient(
        &MSched.ServerS,
        S,               /* O - modified */
        HostName) == FAILURE)
    {
    /* nothing to load */

    MSUFree(S);

    MUFree((char **)&S);

    return(FAILURE);
    }

  if (S->SocketProtocol == 0)
    {
    S->SocketProtocol = MSched.DefaultMCSocketProtocol;
    }

  if ((S->RemoteHost[0] == '\0') && (HostName[0] != '\0'))
    {
    MUStrCpy(S->RemoteHost,HostName,sizeof(S->RemoteHost));
    }

  if (MSURecvData(S,MSched.SocketWaitTime,TRUE,&SC,tmpEMsg) == FAILURE)
    {
    MDB(3,fSOCK) MLog("ALERT:    cannot read client packet\n");

    MUStrDup(&MSched.UIBadRequestMsg,tmpEMsg);

    if (S->RemoteHost != NULL)
      MUStrDup(&MSched.UIBadRequestor,S->RemoteHost);
    else
      MUStrDup(&MSched.UIBadRequestor,"???");

    if (MSched.DefaultDropBadRequest == FALSE)
      {
      MUStrDup(&S->SMsg,tmpEMsg);

     if (SC == mscNoAuth)
        S->StatusCode = (long)msfESecServerAuth;
      else
        S->StatusCode = (long)msfEGServer;

      MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);
      }

    MSUFree(S);

    MUFree((char **)&S);

    if (EMsg != NULL)
      MUStrCpy(EMsg,tmpEMsg,MMAX_LINE);

    return(FAILURE);
    }  /* END if (MSURecvData(S,MSched.SocketWaitTime,TRUE,&SC,tmpEMsg) == FAILURE) */

  if (MSysDelegateRequest(S) == SUCCESS)
    {
    return(SUCCESS);
    }

  MDB(8,fSOCK) MLog("INFO:     locking client count mutex in %s\n",
      FName);

  MUMutexLock(&MSUClientCountMutex);

  MDB(8,fSOCK) MLog("INFO:     incrementing client count (%d) in %s\n",
      MSUClientCount,
      FName);
 
  MSUClientCount++;
    
  MDB(8,fSOCK) MLog("INFO:     incremented client count to (%d) in %s\n",
      MSUClientCount,
      FName);

  MUMutexUnlock(&MSUClientCountMutex);

  MDB(8,fSOCK) MLog("INFO:     client count mutex unlocked in %s\n",
      FName);

  bmset(&S->Flags,msftIncomingClient);

  /* check client count */
  if (MSUClientCount >= MSched.ClientMaxConnections)    
    {
    MDB(0,fUI) MLog("ALERT:    cannot accept connection num %d (transaction num %d) from '%s' (limit reached)\n",
      MSUClientCount,
      MSched.TransactionCount+1,
      HostName);

    snprintf(tmpEMsg,sizeof(tmpEMsg),"ERROR:    Moab reached its maximum number of concurrent client connections %d. (CLIENTMAXCONNECTIONS)\n",
      MSched.ClientMaxConnections);

    MUISAddData(S,tmpEMsg);

    S->StatusCode = msfConnRejected;

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    MSUFree(S);

    MUFree((char **)&S);

    MMBAdd(
      &MSched.MB,
      tmpEMsg,
      (char *)"N/A",
      mmbtOther,
      0,
      1,
      NULL);

    if (EMsg != NULL)
      strcpy(EMsg,"maxclient reached");
 
    return(FAILURE);
    }  /* END if (MSUClientCount >= MSched.ClientMaxConnections) */

  S->LocalTID = MSched.TransactionIDCounter++;

  MDB(3,fUI) MLog("INFO:     client socket from '%s' accepted\n",
    HostName);

  MDB(5,fUI) MLog("INFO:     data received from client '%s' - %s\n",
    HostName,
    S->RBuffer);

  MDB(5,fUI) MLog("INFO:     response to client '%s' attributes: IsNonBlocking=%s, Phase=%s, PENDING=%s\n",
    HostName,
    MBool[S->IsNonBlocking],
    MActivity[MSched.Activity],
    MBool[((S->IsNonBlocking == TRUE) && (MSched.Activity != macUIProcess))]);

  if (MSUClientCount > MSched.HistMaxClientCount)
    {
    MSched.HistMaxClientCount = MSUClientCount;
    }

  MSysAddSocketToQueue(S);  /* handles locking for thread safety */

  return(SUCCESS);
  }  /* END MSysEnqueueSocket() */



/**
 * This function runs in a Separate THREAD of execution
 *
 * Enqueue socket connections onto the "socket queue". If thread are enabled,
 * this function continues to loop, pushing sockets onto the queue. If the
 * communication thread is disabled, this function will push only waiting
 * sockets onto the queue and will then return immediately.
 *
 * @see MSysEnqueueSocket() - child
 *
 * @param Arg (I) [not used]
 */

void *MSysCommThread(
    
  void *Arg)  /* I (not used) */

  {
#ifdef __MCOMMTHREAD
  MUINT4 ThreadID;

  pthread_detach(pthread_self());

  ThreadID = MUGetThreadID();

  MDB(6,fALL) MLog("INFO:     running in thread '%lu' \n",
    (unsigned long)ThreadID);

  /* do we need to lock this? */

  CommTID = ThreadID;

  while (MSched.Shutdown == FALSE)
    {
    MSysEnqueueSocket(NULL);
   
    if (MSched.EnableHighThroughput == TRUE)
      MUSleep(2000,FALSE);
    else 
      MUSleep(10000,FALSE);
    }
#else

  while (MSysEnqueueSocket(NULL) != FAILURE);

#endif /* __MCOMMTHREAD */

  /*NOTREACHED*/

  return(NULL);
  }  /* END MSysCommThread() */


/**
 *
 * Start the Communications thread, which listens on the socket
 *
 */

int MSysStartCommThread()    

  {

#ifdef __MCOMMTHREAD
  pthread_t PeerThread;
  pthread_t CacheTransitionThread;
  pthread_t WSTransitionThread;

  pthread_attr_t PeerThreadAttr;
  pthread_attr_t CacheTransitionThreadAttr;
  pthread_attr_t WSTransitionThreadAttr;

  int rc;

#endif /* __MCOMMTHREAD */

  const char *FName = "MSysStartCommThread";

  MDB(6,fALL) MLog("%s()\n",
    FName);

#ifdef __MCOMMTHREAD
  pthread_attr_init(&PeerThreadAttr);
  pthread_attr_init(&CacheTransitionThreadAttr);
  pthread_attr_init(&WSTransitionThreadAttr);

  pthread_atfork(NULL,NULL,MSysPostFork);

  /* Intialize the various mutexes in the system */
  MSysSocketQueueMutexInit();
  MSysJobSubmitQueueMutexInit();
  MObjectQueueMutexInit();
  MWSQueueMutexInit();
  MEventLogWSMutexInit();
  MEventLogWSQueueInit();
  


  pthread_mutex_init(&SaveNonBlockMDiagTMutex,NULL);
  pthread_mutex_init(&SaveNonBlockMDiagSMutex,NULL);
  pthread_mutex_init(&SaveNonBlockShowqMutex,NULL);

  pthread_mutex_init(&MSUClientCountMutex,NULL);
  pthread_mutex_init(&EUIDMutex,NULL);

#if defined(__AIX)
  MDB(4,fALL) MLog("INFO:    setting communication thread stack size to %d bytes\n",
    MDEF_THREADSTACKSIZE);

  pthread_attr_setstacksize(&PeerThreadAttr,MDEF_THREADSTACKSIZE);
  pthread_attr_setstacksize(&CacheTransitionThreadAttr,MDEF_THREADSTACKSIZE);
  pthread_attr_setstacksize(&WSTransitionThreadAttr,MDEF_THREADSTACKSIZE);
#endif /* __AIX */

  /* int pthread_create(pthread_t * thread, pthread_attr_t * attr, void * (*start_routine)(void *), void * arg); */

  rc = pthread_create(&PeerThread,&PeerThreadAttr,MSysCommThread,NULL);

  if (rc != 0)
    {
    /* error creating thread */

    MDB(0,fALL) MLog("ERROR:    cannot create peer thread (error: %d '%s')\n",
      rc,
      strerror(rc));
    
    fprintf(stderr,"ERROR:    cannot create peer thread (error: %d ' %s')\n",
      rc,
      strerror(rc));

    return(FAILURE);
    }

  MDB(4,fALL) MLog("INFO:     new peer thread %lu created\n",
    (mulong)PeerThread);
  
  rc = pthread_create(&CacheTransitionThread,&CacheTransitionThreadAttr,MOCacheThread,NULL);

  if (rc != 0)
    {
    /* error creating thread */

    MDB(0,fALL) MLog("ERROR:    cannot create cache thread (error: %d '%s')\n",
      rc,
      strerror(rc));
    
    fprintf(stderr,"ERROR:    cannot create cache thread (error: %d ' %s')\n",
      rc,
      strerror(rc));

    return(FAILURE);
    }

  MDB(4,fALL) MLog("INFO:     new cache thread %lu created\n",
    (mulong)CacheTransitionThread);

  if (((MSched.WebServicesURL != NULL) && (MSched.PushCacheToWebService == TRUE)) ||
      ((MSched.EventLogWSURL != NULL) && (MSched.PushEventsToWebService == TRUE)))
    {
    
    rc = pthread_create(&WSTransitionThread,&WSTransitionThreadAttr,MOWebServicesThread,NULL);
  
    if (rc != 0)
      {
      
      MDB(0,fALL) MLog("ERROR:    cannot create web services thread (error: %d '%s')\n",
        rc,
        strerror(rc));
      
      fprintf(stderr,"ERROR:    cannot create web services thread (error: %d ' %s')\n",
        rc,
        strerror(rc));
  
      return(FAILURE);
      }
  
    MDB(4,fALL) MLog("INFO:     new cache thread %lu created\n",
      (mulong)WSTransitionThread);
    }

#else /* __MCOMMTHREAD */

  /* threaded peer interface not enabled */
  
  MDB(3,fALL) MLog("WARNING:  cannot create peer thread - pthreads not enabled\n");
  
#endif /* __MCOMMTHREAD */

  return(SUCCESS);
  }  /* END MSysStartCommThread() */




/* END MSysCommThread.c */
