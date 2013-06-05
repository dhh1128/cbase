/* HEADER */

/**
 * @file MUIAcceptRequest.c
 *
 * Contains MUI incoming Socket acceptance code
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Function looks at various system properties to determine if the UI phase should continue.
 * This function may override the user specified min or max RMPOLLINTERVAL!
 *
 * @see MUIAcceptRequests()
 */

mbool_t MUIIsUIPhaseDone(

  mbool_t RMDataIsLoaded)
  
  {
  /* if scheduling has been stopped, there is no need
   * to exit the UI phase */

  if (MSched.State == mssmStopped)
    {
    return(FALSE);
    }

  /* do not exit the UI phase until the RM data is loaded! */

  if (RMDataIsLoaded == FALSE)
    {
    return(FALSE);
    }

  /* we need to look at all triggers before moving out of UI phase! */
  
  if (MSched.TrigStats.NumAllTrigsEvaluatedThisIteration < MSched.TrigEvalLimit)
    {
    return(FALSE);
    }

  /* it is not OK to exit the UI phase */
  return(TRUE);
  }  /* END MUIIsUIPhaseDone() */




/**
 * Look for new socket connection request.
 *
 * @see MSysPopSStack() - child
 *
 * @param SS (I)
 * @param TimeLimit (I)
 */

int MUIAcceptRequests(

  msocket_t *SS,        /* I */
  long       TimeLimit) /* I */

  {
  msocket_t *S;

  long    now = 0;

  mulong  ETime;

  long    tmpStartTime;
  long    tmpEndTime;
  mbool_t LogLevelOverridden;
  int     OldLogLevel = -1;

  mulong  OrigDeadLine;
  mulong  MUIMinDeadLine;  /* minimum UI cycle duration (can't be any faster than this) */

  mbool_t RMDataLoadInitiated;  /* request made to begin loading (but not processing) workload/cluster info from RM */
  mbool_t RMDataLoaded;         /* RM workload/cluster info loaded from RM */

  long    RMDataLoadTime;       /* time request to load workload/cluster info from RM was initiated */

  const char *FName = "MUIAcceptRequests";

  MDB(8,fUI) MLog("%s(%d,%ld)\n",
    FName,
    (SS != NULL) ? SS->sd : -1,
    TimeLimit);

  if (SS == NULL)
    {
    return(FAILURE);
    }

  MUGetTime((mulong *)&now,mtmNONE,&MSched);

  /* init load calculation counters */

  MUGetMS(NULL,&MSched.LoadStartTime[mschpUI]);
  MSched.LoadEndTime[mschpUI] = MSched.LoadStartTime[mschpUI];

#ifdef MIDLE
  MSched.LoadStartTime[mschpMeasuredIdle] = MSched.LoadStartTime[mschpUI];
  MSched.LoadEndTime[mschpMeasuredIdle] = MSched.LoadStartTime[mschpMeasuredIdle];
#endif /* MIDLE */

  MUIDeadLine = now + TimeLimit;
  MUITrigDeadLine = now + MSched.MaxTrigCheckInterval;

  if (MSched.PeerConfigured == TRUE)
    {
    /* prevent rapid successive querying of remote peers or when 
       dynamic provisioning is used */

    MUIMinDeadLine = now + MAX(5,MSched.MinRMPollInterval);  /* minimum UI cycle duration is 5 seconds */
    }
  else
    {
    MUIMinDeadLine = now + MSched.MinRMPollInterval;
    }

  MSched.UIDeadLineReached = FALSE;

  if (MSysGetNextEvent(&ETime) == SUCCESS)
    {
    MUIDeadLine = MIN(MUIDeadLine,(long)ETime);
    MUIDeadLine = MAX(MUIDeadLine,(long)MUIMinDeadLine);
    }

  if (MSched.NextUIPhaseDuration > 0)
    {
    MUIDeadLine = MIN(MUIDeadLine,now + MSched.NextUIPhaseDuration);

    MSched.NextUIPhaseDuration = 0;
    }

  MSchedSetActivity(macUIWait,NULL);
  OrigDeadLine = MUIDeadLine;

  MSched.PeerTruncatedUICycle = FALSE;

  RMDataLoadInitiated = FALSE;
  RMDataLoaded        = FALSE;
  RMDataLoadTime      = -1;

  while (((now <= MUIDeadLine) || (MSched.State == mssmStopped)) &&
          (MUIIsUIPhaseDone(RMDataLoaded) == FALSE) &&
         (MSched.TrigCompletedJob == FALSE))
    {
    if (MSysCheckEvents() == SUCCESS) 
      {
      /* scheduling event occurred */

      MUIDeadLine = MIN(MUIDeadLine,(long)MUIMinDeadLine);
      }

    if (RMDataLoadInitiated == FALSE)
      {
      if ((MUIDeadLine - now) <= 5)
        {
        /* initiate data load 5 seconds early */

        if (MRMGetData(NULL,TRUE,NULL,NULL) == SUCCESS)
          {
          RMDataLoadInitiated = TRUE;
          RMDataLoadTime = now;
          }
        }
      }    /* END if (RMDataLoadInitiated == FALSE) */
    else if (RMDataLoaded == FALSE)
      {
      /* if RM data not loaded  */

      RMDataLoaded = MRMDataIsLoaded(NULL,NULL);

      if (RMDataLoaded == FALSE)
        {
        if ((now - RMDataLoadTime) > MCONST_MINUTELEN * 2)
          {
          /* RM data loading is taking too long */

          MDB(2,fUI) MLog("ALERT:    RM data loading is taking too long (%ld > %d secs.), retrying\n",
            (now - RMDataLoadTime),
            MCONST_MINUTELEN * 2);

          if (MRMGetData(NULL,TRUE,NULL,NULL) == SUCCESS)
            {
            RMDataLoadInitiated = TRUE;
            RMDataLoadTime = now;
            }
          }
        }    /* END if (RMDataLoaded == FALSE) */
      }      /* END else if (RMDataLoaded == FALSE) */

    MSchedSetActivity(macUIProcess,NULL);  /* The position of setting Activity to macUIProcess is critical! This must be placed before we process incoming UI
                                              requests and CANNOT be placed before we attempt any communication with other peers or
                                              there will be deadlocks!!! Activity should be unset from macUIProcess whenever we could
                                              potentially contact other peers! */

    /* NOTE:  if simulation time is proportional to real time, 
              user interface processing loop should still sleep 
              to appropriate time even if ignoretotime is set 
              but ignore user requests (NYI) */

    if (MSched.Mode == msmSim)
      {
      if (MSched.IgnoreToTime != 0)
        {
        if (now < MSched.IgnoreToTime)
          {
          return(SUCCESS);
          }
        else
          {
          MSched.IgnoreToTime = 0;
          }
        }

      if (MSched.IgnoreToIteration != 0)
        {
        if (MSched.Iteration < MSched.IgnoreToIteration)
          {
          return(SUCCESS);
          }
        else
          {
          MSched.IgnoreToIteration = 0;
          }
        }
      }    /* END if (MSched.Mode == msmSim) */

    if (MSched.State == mssmStopped)
      {
      MUSleep(50000,FALSE);
      }
    
    /* check for non-blocking responses */

    MDB(11,fUI) MLog("INFO:     checking non-blocking responses (%ld seconds left)\n",
     (MUIDeadLine - now));

    MUGetMS(NULL,&tmpStartTime);

    MUIProcessTransactions(FALSE,NULL);  /* maybe only process one at a time to prevent UI phase from overrunning its limit? */

    MUGetMS(NULL,&tmpEndTime);

    MSched.LoadEndTime[mschpUI] += tmpEndTime - tmpStartTime;

    MDB(5,fUI)
      {
      if (!((MUIDeadLine - now) % MCONST_MINUTELEN))
        {
        MDB(5,fUI) MLog("INFO:     selecting next clients (%ld seconds left)\n",
          (MUIDeadLine - now));
        }
      }
    else if ((mlog.Threshold >= 9) && (MSched.InRecovery != TRUE))
      {
      MDB(9,fUI) MLog("INFO:     selecting next clients (%ld seconds left)\n",
        (MUIDeadLine - now));
      }

    /* accept client connections */

#ifndef __MCOMMTHREAD
    /* accept socket connections and place on stack */

    MSysCommThread(NULL);
    
    if (MSched.InRecovery != TRUE)
      MDB(9,fUI) MLog("INFO:     all clients connected.  servicing requests\n");
#endif /* !__MCOMMTHREAD */

    /* service clients */
 
    while (MSysDequeueSocket(&S) != FAILURE)
      {
      LogLevelOverridden = FALSE;

      if ((MSched.LogLevelOverride == TRUE) && 
          (S->LogThreshold > 0) &&
          (S->LogThreshold > mlog.Threshold))
        {
        char tmpLogName[MMAX_LINE];
        char tmpInfoLine[MMAX_LINE];
        char tmpSnapString[MMAX_LINE];
        char *ptr;

        if (MSched.OLogFile != NULL)
          {
          sprintf(tmpInfoLine,"ERROR:    already creating a temporary log\n");

          MUISAddData(S,tmpInfoLine);

          break;
          }

        MDB(1,fUI) MLog("INFO:     using client requested loglevel %d\n",
          S->LogThreshold);

        ptr = NULL;

        MLogGetName(&ptr);

        MULToSnapATString(MSched.Time,tmpSnapString);

        if (ptr != NULL)
          {
          snprintf(tmpLogName,sizeof(tmpLogName),"%s.%s",
            ptr,
            tmpSnapString);

          MUFree(&ptr);
          }
        else
          {
          snprintf(tmpLogName,sizeof(tmpLogName),"%s.%s",
            "templog",
            tmpSnapString);
          }

        MUStrDup(&S->LogFile,tmpLogName);

        MLogGetName(&MSched.OLogFile);

        MDB(1,fUI) MLog("INFO:     using log file '%s'\n",
          tmpLogName);

        MLogShutdown();

        MLogInitialize(tmpLogName,MSched.LogFileMaxSize,MSched.Iteration);

        MUGetTime((mulong *)&MUIDeadLine,mtmNONE,&MSched);

        if (S->LogThreshold > MMAX_LOGLEVEL)
          S->LogThreshold = MMAX_LOGLEVEL;

        OldLogLevel = mlog.Threshold;
        LogLevelOverridden = TRUE;
        mlog.Threshold = S->LogThreshold;
        }

      if ((now >= MUIDeadLine) && (MSched.Activity == macUIProcess))
        {
        MDB(4,fUI) MLog("INFO:     no longer accepting user requests\n");

        MSched.Activity = macNONE;

        memset(MSched.ActivityDuration,0,sizeof(MSched.ActivityDuration));
        memset(MSched.ActivityCount,0,sizeof(MSched.ActivityCount));
        }
      
      MDB(11,fUI) MLog("INFO:     checking read status of client (%d, TID: %ld)\n",
        S->sd,
        S->LocalTID);

      if ((S->IsLoaded == TRUE) || (S->sd >= 0))
        {
        /* read data in socket if it is open or if data has already been loaded */

        MUGetMS(NULL,&tmpStartTime);

        if (MSysProcessRequest(S,S->IsLoaded) == FAILURE)
          {
          MDB(6,fUI) MLog("INFO:     could not service client request (TID: %ld)\n",
            S->LocalTID);
          }

        MUGetMS(NULL,&tmpEndTime);

        MSched.LoadEndTime[mschpUI] += tmpEndTime - tmpStartTime;

        if ((MSched.Shutdown == TRUE) || (MSched.Recycle == TRUE))
          {
          if (MSched.EnableHighThroughput == FALSE) 
            {
            MDB(1,fUI) MLog("INFO:     shutting scheduler down (user request)\n");

            /* NOTE:  shutdown request may have arrived during scheduling phase and
                      jobs submits may have occurred after this shutdown request was
                      made - however, if submit request reports as pending, enduser
                      cannot tell - 
                      must close socket interface to disable further requests and then
                      process all pending job submits before shutting down - NYI */

            MUSleep(MDEF_USPERSECOND,TRUE);

            /* free socket to help valgrind output be cleaner */

            MSUFree(S);
            MUFree((char **)&S);

            /* MSysShutdown() does not return */

            if (MSched.Recycle == TRUE)
              MSysShutdown(0,-1);
            else
              MSysShutdown(0,0);

            /*NOTREACHED*/
            }  /* END if (MSched.EnableHighThroughput == FALSE) */
          }    /* END if ((MSched.Shutdown == TRUE) || ...) */
        }      /* END if (S->sd >= 0) */

      if (LogLevelOverridden == TRUE)
        {
        mlog.Threshold = OldLogLevel;

        if (MSched.OLogFile != NULL)
          {
          MLogShutdown();

          MLogInitialize(MSched.OLogFile,-1,MSched.Iteration);

          MUFree(&MSched.OLogFile);
          }

        MDB(1,fUI) MLog("INFO:     returning from client loglevel, using loglevel %d\n",
          mlog.Threshold);
        }

      MSUFree(S);
      MUFree((char **)&S);
      }    /* END while (MSysDequeueSocket()) */

    if (MSched.EnableHighThroughput == TRUE)
      {
      if ((MSched.Shutdown == TRUE) || (MSched.Recycle == TRUE))
        {
        /* all pending requests have been handled */
        /* are the sockets closed at this point? */

        MS3ProcessSubmitQueue();

        MDB(1,fUI) MLog("INFO:     shutting scheduler down (user request)\n");

        MUSleep(MDEF_USPERSECOND,TRUE);

        /* MSysShutdown() does not return */

        if (MSched.Recycle == TRUE)
          MSysShutdown(0,-1);
        else
          MSysShutdown(0,0);

        /*NOTREACHED*/
        }  /* END if ((MSched.Shutdown == TRUE) || ...) */
      }    /* END if (MSched.EnableHighThroughput == TRUE) */

    if ((MSched.Mode == msmSim) &&
        (MSched.TimePolicy != mtpReal) &&
        (MSched.State == mssmRunning) &&
        (MSched.InRecovery != TRUE))
      {
      /* resume simulation immediately */

      break;
      }
    else
      {
      /* sleep 100 ms */

      if ((MSched.Mode == msmSim) && (MSched.State == mssmStopped))
        {
        /* perform real-time sleep but do not advance clock */

        MUSleep(100000,FALSE);
        }
      else
        {

#ifdef MIDLE
        MUGetMS(NULL,&tmpStartTime);
#endif /* MIDLE */

        if (MSched.LocalEventReceived == FALSE)
          {
          if (MSched.EnableHighThroughput == TRUE)
            MUSleep(10000,TRUE);
          else
            MUSleep(100000,TRUE);
          }

#ifdef MIDLE
        MUGetMS(NULL,&tmpEndTime);
        MSched.LoadEndTime[mschpMeasuredIdle] += tmpEndTime - tmpStartTime;
#endif /* MIDLE */

        /* TODO:  should be non-blocking */

        /* check triggers according to MUITrigDeadLine */

        if (now > MUITrigDeadLine)
          {
          MSchedCheckTriggers(NULL,MSched.MaxTrigCheckSingleTime,&MSched.LastTrigIndex);
          MUITrigDeadLine = now + MSched.MaxTrigCheckInterval;
          }
        }   /* END else ((MSched.Mode == msmSim) && (MSched.State == mssmStopped)) */

      MUGetTime((mulong *)&now,mtmNONE,&MSched);
      }     /* END else ((MSched.Mode == msmSim) && ...) */
    }       /* END while ((now < MUIDeadLine) && ...) */

  MSched.TrigCompletedJob = FALSE;

  MDB(4,fUI) MLog("INFO:     user request processing completed\n");

  MSched.Activity = macNONE;

  memset(MSched.ActivityDuration,0,sizeof(MSched.ActivityDuration));
  memset(MSched.ActivityCount,0,sizeof(MSched.ActivityCount));

  if (MSched.Time >= OrigDeadLine)
    MSched.UIDeadLineReached = TRUE;

  return(SUCCESS);
  }  /* END MUIAcceptRequests() */
/* END MUIAcceptRequest.c */


