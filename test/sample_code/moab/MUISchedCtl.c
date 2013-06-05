/* HEADER */

/**
 * @file MUISchedCtl.c
 *
 * Contains MUI Scheduler Control
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


/**
 * Process 'mschedctl' client request.
 *
 * NOTE:  This routine is used to respond to Moab/grid initialization, query, 
 *        and job management requests.
 *
 * @see MUISProcessRequest() - parent
 * @see MCSchedCtlCreateRequest() - peer - generate client-side request
 * @see MCSchedCtl() - peer - report results w/in client
 * @see MUIExportRsvQuery()
 * @see MUISchedCtlProcessEvent() - child - process incoming event records
 * @see MUISchedCtlCreate() - child - create new objects
 * @see MUISchedCtlModify() - child - process 'mschedctl -m' - modify existing objects (events, vpcs, triggers, etc)
 * @see MUISchedCtlQueryEvents() - child - process 'mschedctl -q event'
 * @see MVPCProfileToString() - child - handle 'mschedctl -l vpcprofile'
 * @see MVPCProfileToXML() - child
 * @see MUISchedCtlModify() - child - process 'mschedctl -m'
 * @see MVPCShow() - child - handle 'mschedctl -l vpc'
 *
 * @param S (I)
 * @param CFlags (I) [bitmap of enum XXX]
 * @param Auth (I)
 */

int MUISchedCtl(

  msocket_t *S,      /* I */
  mbitmap_t *CFlags, /* I (bitmap of enum XXX) */
  char      *Auth)   /* I */

  {
  long  SchedIteration;

  enum MSchedCtlCmdEnum CIndex;

  int     SSNumber;

  mbool_t IsVerbose;

  char  tmpLine[MMAX_LINE];
  char  tmpName[MMAX_NAME];
  char  ArgString[MMAX_BUFFER + 1];
  char  FlagString[MMAX_LINE];
  char  OString[MMAX_LINE];
  char  TString[MMAX_LINE];

  enum MXMLOTypeEnum OType = mxoNONE;
  char OID[MMAX_NAME];
  char EventMsg[MMAX_LINE];

  enum MObjectSetModeEnum Op = mSet;

  mgcred_t *U = NULL;
  mpsi_t   *P = NULL;

  int  CTok;

  int  sindex;

  long  tmpTime = 0;
  long  pauseTime = 0;

  mxml_t *RE;  /* pointer (not locally allocated) */
  mxml_t *DE;
  mxml_t *OE;

  enum MFormatModeEnum DFormat;

  mbool_t  IsAdmin = FALSE;

  const char *FName = "MUISchedCtl";

  MDB(2,fUI) MLog("%s(S,%ld,%s)\n",
    FName,
    CFlags,
    (Auth != NULL) ? Auth : "NULL");

  if (S->RDE == NULL)
    {
    MDB(3,fUI) MLog("WARNING:  corrupt command received '%32.32s'\n",
      (S->RBuffer != NULL) ? S->RBuffer : "NULL");

    MUISAddData(S,"ERROR:    corrupt command received\n");

    return(FAILURE);
    }

  if (Auth == NULL)
    {
    return(FAILURE);
    }

  RE = S->RDE;

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }
  
  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    if (MPeerFind(Auth,FALSE,&P,FALSE) == FAILURE)
      {
      /* NYI */
      }      

    U = NULL;
    }
  else
    {
    MUserAdd(Auth,&U);

    P = NULL;
    }
        
  DE = S->SDE;

  if ((MXMLGetAttr(RE,MSAN[msanAction],NULL,tmpLine,0) == FAILURE) ||
     ((CIndex = (enum MSchedCtlCmdEnum)MUGetIndexCI(tmpLine,MSchedCtlCmds,FALSE,msctlNONE)) == msctlNONE)) 
    {
    /* ping received 'mschedctl --flags=status' (allow pings without authorizing request) */

    if (MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString)) != FAILURE)
      {
      int aindex;

      /* record sender */

      for (aindex = 0;aindex < MMAX_APSERVER;aindex++)
        {
        if (MSched.MAPServerHost[aindex][0] == '\0')
          {
          MUStrCpy(
            MSched.MAPServerHost[aindex],
            ArgString,
            sizeof(MSched.MAPServerHost[aindex]));
        
          break;
          }

        if (!strcmp(ArgString,MSched.MAPServerHost[aindex]))
          break;
        }  /* END for (aindex) */

      if (aindex < MMAX_APSERVER)
        MSched.MAPServerTime[aindex] = MSched.Time;      
      }  /* END if ((MXMLGetAttr(RE,MSAN[msanAction],NULL,tmpLine,0) == FAILURE) || ...) */

    if (MS3GetWhere(
          RE,
          NULL,
          NULL,
          tmpName,
          sizeof(tmpName),
          tmpLine,
          sizeof(tmpLine)) == SUCCESS)
      {
      /* determine max clock skew */

      /* NYI */
      }

    /* send response */

    MUIGenerateSchedStatusXML(&OE,P,(U == NULL) ? NULL : U->Name);
 
    MXMLAddE(DE,OE);

    return(SUCCESS);
    }  /* END if ((MXMLGetAttr(RE,MSAN[msanAction],NULL,tmpLine,0) == FAILURE) || ... */

  /* set defaults */

  IsVerbose    = FALSE;
  DFormat      = mfmHuman;

  SSNumber     = -1;

  OID[0] = '\0';
  EventMsg[0] = '\0';

  /* load flags */

  if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0) == SUCCESS)
    {
    if (strcasestr(FlagString,"verbose"))
      IsVerbose = TRUE;

    if (strcasestr(FlagString,"xml"))
      DFormat = mfmXML;
    }  /* END if (MXMLGetAttr(RE,MSAN[msanFlags],...) == SUCCESS) */

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
    if (!strcmp(tmpName,"CONFIGSERIALNUMBER"))
      {
      /* used by fallback HA server to sync config */

      SSNumber = (int)strtol(tmpLine,NULL,10);

      continue;
      }
    }    /* END while (MXMLGetChild() == SUCCESS) */

  CTok = -1;

  while (MS3GetSet(
      RE,
      NULL,
      &CTok,
      tmpName,
      sizeof(tmpName),
      tmpLine,
      sizeof(tmpLine)) == SUCCESS)
    {
    if (!strcmp(tmpName,"otype"))
      {
      OType = (enum MXMLOTypeEnum)MUGetIndexCI(tmpLine,MXO,FALSE,mxoNONE);

      continue;
      }

    if (!strcmp(tmpName,"oid"))
      {
      MUStrCpy(OID,tmpLine,sizeof(OID));

      continue;
      }

    if (!strcmp(tmpName,"msg"))
      {
      MUStrCpy(EventMsg,tmpLine,sizeof(EventMsg));

      continue;
      }

    sindex = MUGetIndexCI(tmpName,MSchedAttr,FALSE,msaNONE);

    switch (sindex)
      {
      case msaName:
  
        {
        char *ptr;
        char *TokPtr;

        /* FORMAT:  <OTYPE>[:<OID>] */

        ptr = MUStrTok(tmpLine,":",&TokPtr);

        OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

        if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
          {
          MUStrCpy(OID,ptr,sizeof(OID));
          }
        }    /* END BLOCK */

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (sindex) */
    }    /* END while (MXMLGetChild() == SUCCESS) */

  /* load command specific options */

  MXMLGetAttr(RE,MSAN[msanOption],NULL,OString,sizeof(OString));

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));

  if (MXMLGetAttr(RE,"op",NULL,tmpLine,0) == SUCCESS)
    {
    Op = (enum MObjectSetModeEnum)MUGetIndexCI(tmpLine,MObjOpType,FALSE,Op);
    }

  SchedIteration = 0;

  if ((CIndex == msctlPause) || 
      (CIndex == msctlResume) ||
      (CIndex == msctlStep) || 
      (CIndex == msctlStop))
    {
    if (MUICheckAuthorization(
          U,
          P,
          NULL,
          mxoSched,
          mcsMSchedCtl,
          CIndex,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }

    /* parse stop/step arguments */

    if (strchr(ArgString,':') != NULL)
      {
      long tmpL;

      tmpL = MUTimeFromString(ArgString);

      SchedIteration = tmpL / MSched.MaxRMPollInterval;
      }
    else
      {
      SchedIteration = strtol(ArgString,NULL,10);
      }

    if ((CIndex == msctlStop) || (CIndex == msctlStep))
      {
      if (strchr(ArgString,'S') != NULL)
        {
        if (CIndex == msctlStop)
          {
          MSched.IgnoreToTime = SchedIteration;
          }
        else
          {
          MUGetTime((mulong *)&tmpTime,mtmNONE,&MSched);

          MSched.IgnoreToTime = (long)tmpTime + SchedIteration;
          }

        MULToTString(SchedIteration,TString);

        sprintf(tmpLine,"interface will block for %s\n",
          TString);
 
        MUISAddData(S,tmpLine);

        return(SUCCESS);
        }
      }
    }    /* END if ((CIndex == msctlPause) || ...) */

  switch (CIndex)
    {
    case msctlDestroy:

      {
      char tmpName[MMAX_NAME];

      if (MXMLGetAttr(RE,MSAN[msanOption],NULL,tmpName,sizeof(tmpName)) == FAILURE)
        {
        return(FAILURE);
        }

      if (!strcmp(tmpName,"learning"))
        {
        if (MUICheckAuthorization(
              U,
              P,
              NULL,
              mxoSched,
              mcsMSchedCtl,
              msctlDestroy,
              &IsAdmin,
              NULL,
              0) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        MUISAddData(S,"application stats successfully cleared\n");

        return(SUCCESS);
        }
      else if (!strcmp(tmpName,"message"))
        {
        int Index;

        if (MUICheckAuthorization(
              U,
              P,
              NULL,
              mxoSched,
              mcsMSchedCtl,
              msctlDestroy,
              &IsAdmin,
              NULL,
              0) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (MXMLGetAttr(RE,MSAN[msanName],NULL,tmpName,sizeof(tmpName)) == FAILURE)
          {
          MUISAddData(S,"unspecified message index\n");

          return(FAILURE);
          }

        Index = (int)strtol(tmpName,NULL,10);

        if (MMBRemove(Index,&MSched.MB) == FAILURE)
          {
          MUISAddData(S,"could not delete message\n");

          return(FAILURE);
          }

        return(SUCCESS);
        }
      else if (!strcmp(tmpName,"trigger"))
        {
        mtrig_t *T;

        if (MUICheckAuthorization(
              U,
              P,
              NULL,
              mxoSched,
              mcsMSchedCtl,
              msctlDestroy,
              &IsAdmin,
              NULL,
              0) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (MUICheckAuthorization(
              U,
              P,
              NULL,
              mxoTrig,
              mcsMSchedCtl,
              msctlDestroy,
              &IsAdmin,
              NULL,
              0) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (MXMLGetAttr(RE,MSAN[msanName],NULL,tmpName,sizeof(tmpName)) == FAILURE)
          {
          MUISAddData(S,"unspecified trigger\n");

          return(FAILURE);
          }

        if (MTrigFind(tmpName,&T) == FAILURE)
          {
          MUISAddData(S,"unidentified trigger id\n");

          return(FAILURE);
          }

        if (MTrigRemove(NULL,tmpName,TRUE) == FAILURE)
          {
          MUISAddData(S,"An error occurred, please check the log file\n");

          return(FAILURE);
          }

        MUISAddData(S,"success");

        return(SUCCESS);
        } /* END if (!strcmp(tmpName,"trigger")) */
      else if (!strcmp(tmpName,"config"))
        {
        char tmpLine[MMAX_LINE];

        if (MXMLGetAttr(RE,MSAN[msanName],NULL,tmpName,sizeof(tmpName)) == FAILURE)
          {
          MUISAddData(S,"unspecified parameter\n");

          return(FAILURE);
          }

        /* purge config references */

        if (MUIConfigPurge(tmpName,tmpLine,sizeof(tmpLine)) == FAILURE)
          {
          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        MUISAddData(S,"successfully purged config files\n");
        }  /* END else if (!strcmp(tmpName,"config")) */
      else if (!strcmp(tmpName,"vpc"))
        {
        int pindex;

        mbool_t VPCFound = FALSE;

        /* destroy VPC */

        mpar_t  *C;

        if (MXMLGetAttr(RE,MSAN[msanName],NULL,tmpName,sizeof(tmpName)) == FAILURE)
          {
          MUISAddData(S,"unspecified vpc\n");

          return(FAILURE);
          }

        for (pindex = 0;pindex < MVPC.NumItems;pindex++)
          {
          C = (mpar_t *)MUArrayListGet(&MVPC,pindex);

          if ((C == NULL) || (C->Name[0] == '\0'))
            {
            /* end of list found */

            break;
            }

          if (C->Name[0] == '\1')
            {
            /* 'free' partition slot found */

            continue;
            }

          if (strcmp(tmpName,"ALL") && strcasecmp(tmpName,C->Name))
            {
            /* vpc does not match */

            continue;
            }

          if (MUICheckAuthorization(
                U,
                P,
                C,
                mxoPar,
                mcsMSchedCtl,
                msctlDestroy,
                &IsAdmin,
                NULL,
                0) == FAILURE)
            {
            sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
              Auth);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          VPCFound = TRUE;

          MVPCDestroy(&C,ArgString);
 
          MUISAddData(S,"vpc destroyed\n");
          }

        if (VPCFound == FALSE)
          {
          MUISAddData(S,"cannot locate vpc\n");

          return(FAILURE);
          }
        }    /* END else if (!strcmp(tmpName,"vpc")) */
      else
        {
        /* object type not handled */

        /* NO-OP */
        }
      }   /* END BLOCK (case msctlDestroy) */

      break;

    case msctlFlush:
  
      {
      enum MSchedStatTypeEnum SType;

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlFlush,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      SType = (enum MSchedStatTypeEnum)MUGetIndexCI(
        ArgString,
        MSchedStatType,
        FALSE,
        msstNONE);
     
      switch (SType)
        {
        case msstAll:
        case msstUsage:

          MStatClearUsage(
            mxoNONE,
            TRUE,
            TRUE,
            TRUE,
            FALSE,
            FALSE);
       
          MStat.InitTime = MSched.Time;
       
          if (MSched.Mode == msmNormal)
            {
            MOSSyslog(LOG_INFO,"usage statistics initialized");
            }
       
          MDB(3,fUI) MLog("INFO:     usage statistics initialized\n");

          if (SType != msstAll)
            break;

        case msstFairshare:

          MPar[0].FSC.FSInitTime = MSched.Time;
       
          MFSInitialize(&MPar[0].FSC);
       
          if (MSched.Mode == msmNormal)
            {
            MOSSyslog(LOG_INFO,"fairshare statistics initialized");
            }
       
          MDB(3,fUI) MLog("INFO:     fairshare statistics initialized\n");

          break;

        default:

          SType = msstNONE;

          sprintf(tmpLine,"ERROR:  unknown statistics type '%s'\n",ArgString);
 
          break; 
        } 

      if (SType != msstNONE)
        sprintf(tmpLine,"'%s' statistics reset successfully\n",
          MSchedStatType[SType]);

      MUISAddData(S,tmpLine);
      }  /* END BLOCK (case msctlFlush) */

      break;

    case msctlFailure:

       /* introduce artificial failure */

      {
      long tmpL;

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlFailure,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      tmpL = strtol(ArgString,NULL,0);

      if (tmpL <= 0)
        {
        MSim.RMFailureTime = 0;

        MUISAddData(S,"RM failure mode disabled");
        }
      else
        {
        MSim.RMFailureTime = MSched.Time + tmpL;

        MULToTString(tmpL,TString);

        sprintf(tmpLine,"RM failure mode enabled for %s\n",
          TString);

        MUISAddData(S,tmpLine);
        }
      }    /* END BLOCK (case msctlFailure) */

      break;

    case msctlPause:

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlPause,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if (MSched.State != mssmPaused)
        {
        sprintf(tmpLine,"scheduling paused by %s",
          Auth);

        MSysRecordEvent(
          mxoSched,
          MSched.Name,
          mrelSchedPause,
          Auth,
          (EventMsg[0] != '\0') ? EventMsg : tmpLine,
          NULL);

        CreateAndSendEventLogWithMsg(meltSchedPause,MSched.Name,mxoSched,(void *)&MSched,tmpLine);

        MSchedSetState(mssmPaused,Auth);
        }
      else
        {
        sprintf(tmpLine,"scheduling was already paused\n");

        MUISAddData(S,tmpLine);
        }

      MDB(2,fUI) MLog("INFO:     scheduling will be disabled, cluster information will continue to be updated\n");

      sprintf(tmpLine,"scheduling will be disabled, cluster information will continue to be updated\n");

      MUISAddData(S,tmpLine);

      MSchedSetState(mssmPaused,Auth);

      MOSSyslog(LOG_INFO,"scheduler pause command received");

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case msctlStop:

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlStop,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if ((int)SchedIteration > MSched.Iteration)
        {
        MULToTString(MSched.MaxRMPollInterval * (SchedIteration - MSched.Iteration),TString);

        MDB(2,fUI) MLog("INFO:     scheduling will stop in %s at iteration %d\n",
          TString,
          (int)SchedIteration);

        sprintf(tmpLine,"scheduling will stop in %s at iteration %d\n",
          TString,
          (int)SchedIteration);

        MUISAddData(S,tmpLine);

        MSchedSetState(mssmRunning,Auth);
        }
      else
        {
        if (MSched.State != mssmStopped)
          {
          snprintf(tmpLine,sizeof(tmpLine),"scheduler stopped by %s",
            Auth);

          MSysRecordEvent(
            mxoSched,
            MSched.Name,
            mrelSchedStop,
            Auth,
            (EventMsg[0] != '\0') ? EventMsg : tmpLine,
            NULL);

          if (MSched.PushEventsToWebService == TRUE)
            {
            MEventLog *Log = new MEventLog(meltSchedStop);
            Log->SetCategory(melcStop);
            Log->SetFacility(melfScheduler);
            Log->SetPrimaryObject(MSched.Name,mxoSched,(void *)&MSched);
            Log->AddDetail("msg",(EventMsg[0] != '\0') ? EventMsg : tmpLine);
    
            MEventLogExportToWebServices(Log);
    
            delete Log;
            }

          MSchedSetState(mssmStopped,Auth);

          sprintf(tmpLine,"scheduling will stop immediately at iteration %d\n",
            MSched.Iteration);
  
          MUISAddData(S,tmpLine);
          }
        else
          {
          MDB(2,fUI) MLog("INFO:     scheduling will stop immediately at iteration %d\n",
            MSched.Iteration);

          sprintf(tmpLine,"scheduling was already stopped\nscheduling will stop immediately at iteration %d\n",
            MSched.Iteration);
  
          MUISAddData(S,tmpLine);
          }

        MSched.IgnoreToIteration = 0;
        }

      MOSSyslog(LOG_INFO,"scheduler stop command received");

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case msctlStep:

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlStep,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if (SchedIteration > 0)
        {
        MSched.StopIteration = MSched.Iteration + SchedIteration;

        MULToTString(SchedIteration * MSched.MaxRMPollInterval,TString);

        MDB(2,fUI) MLog("INFO:     scheduling will stop in %s at iteration %d\n",
          TString,
          MSched.StopIteration);

        sprintf(tmpLine,"scheduling will stop in %s at iteration %d\n",
          TString,
          MSched.StopIteration);

        MUISAddData(S,tmpLine);

        MSchedSetState(mssmRunning,Auth);

        if (strchr(ArgString,'I') != NULL)
          MSched.IgnoreToIteration = MSched.StopIteration;
        }
      else
        {
        MULToTString(MSched.MaxRMPollInterval,TString);

        MDB(2,fUI) MLog("INFO:     scheduling will stop in %s at iteration %d\n",
          TString,
          MSched.Iteration + 1);

        sprintf(tmpLine,"scheduling will stop in %s at iteration %d\n",
          TString,
          MSched.Iteration + 1);

        MUISAddData(S,tmpLine);

        MSchedSetState(mssmRunning,Auth);

        MSched.StopIteration = MSched.Iteration + 1;

        if (strchr(ArgString,'I') != NULL)
          MSched.IgnoreToIteration = MSched.StopIteration;
        }

      MOSSyslog(LOG_INFO,"scheduler step command received");

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case msctlResume:

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlResume,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      MSched.StopIteration = -1;

      if (MSched.State != mssmRunning)
        {
        snprintf(tmpLine,sizeof(tmpLine),"scheduler resumed by %s",
          Auth);

        MSysRecordEvent(
          mxoSched,
          MSched.Name,
          mrelSchedResume,
          Auth,
          (EventMsg[0] != '\0') ? EventMsg : tmpLine,
          NULL);

        CreateAndSendEventLogWithMsg(meltSchedResume,MSched.Name,mxoSched,(void *)&MSched,(EventMsg[0] != '\0') ? EventMsg : tmpLine);
        }

      if (MSched.Mode == msmSlave)
        MSchedSetState(mssmPaused,Auth);
      else
        MSchedSetState(mssmRunning,Auth);

      MUGetTime((mulong *)&tmpTime,mtmNONE,&MSched);

      MUIDeadLine = tmpTime + SchedIteration;

      /* how long will the scheduler wait to be resumed */
      pauseTime = MUIDeadLine - tmpTime;

      MDB(2,fUI) MLog("INFO:     scheduling will resume in %ld seconds\n",pauseTime);

      if (MSched.Mode == msmNormal)
        {
        MOSSyslog(LOG_INFO,"scheduler resume command received");
        }

      /* clear RM->UTime for each RM - NYI */

      if (pauseTime <=0)
        {
        sprintf(tmpLine,"scheduling will resume immediately\n");
        }
      else
        {
        sprintf(tmpLine,"scheduling will resume in %ld seconds\n", pauseTime);
        }

      MUISAddData(S,tmpLine);

      if (strchr(ArgString,'I') != NULL)
        MSched.IgnoreToIteration = 0;

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case msctlReconfig:

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlReconfig,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      MDB(2,fUI) MLog("INFO:     %s will be recycled immediately\n",
        MSCHED_SNAME);

      MOSSyslog(LOG_NOTICE,"scheduler recycle command received");

      sprintf(tmpLine,"%s will be recycled immediately\n",
        MSCHED_SNAME);

      MUISAddData(S,tmpLine);

      MSched.Recycle = TRUE;

      MUStrDup(&MSched.RecycleArgs,ArgString);

      /* Need to prevent client request handler threads from trying to
       * service requests in the middle of a shutdown */

      MCacheLockAll();

      MSched.Shutdown = TRUE;  /* NOTE:  must be set to preserve job ckpt records */

      snprintf(tmpLine,sizeof(tmpLine),"scheduler recycled by %s",
        Auth);

      MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedRecycle,
        Auth,
        (EventMsg[0] != '\0') ? EventMsg : tmpLine,
        NULL);

      CreateAndSendEventLogWithMsg(meltSchedRecycle,MSched.Name,mxoSched,(void *)&MSched,tmpLine);

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case msctlKill:

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlKill,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      MDB(2,fUI) MLog("INFO:     %s will shutdown immediately\n",
        MSCHED_SNAME);

      MOSSyslog(LOG_NOTICE,"scheduler kill command received");

      sprintf(tmpLine,"%s will be shutdown immediately\n",
        MSCHED_SNAME);

      MUISAddData(S,tmpLine);

      MCacheLockAll();

      MSched.Shutdown = TRUE;

      if (tolower(ArgString[0]) == 'b')
        bmset(&S->Flags,msftDropResponse);

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case msctlLog:

      {
      char tmpLogName[MMAX_LINE];
      char tmpSnapString[MMAX_LINE];
      int  tmpLogLevel;

      char *ptr;

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlLog,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if (MSched.OLogFile != NULL)
        {
        sprintf(tmpLine,"ERROR:    already creating temporary log\n");

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      MDB(2,fUI) MLog("INFO:     %s will create temporary log\n",
        MSCHED_SNAME);

      if (OString[0] != '\0')
        tmpLogLevel = (int)strtol(OString,NULL,10);
      else
        tmpLogLevel = 7;  /* should be handled via #define */

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

      snprintf(tmpLine,sizeof(tmpLine),"creating temporary log '%s' at log level %d\n",
        tmpLogName,
        tmpLogLevel);

      MUISAddData(S,tmpLine);

      MSched.OLogLevel = mlog.Threshold;

      MSched.OLogLevelIteration = MSched.Iteration + 1;

      MLogGetName(&MSched.OLogFile);

      MLogShutdown();

      MLogInitialize(tmpLogName,MSched.LogFileMaxSize,MSched.Iteration);

      MUGetTime((mulong *)&MUIDeadLine,mtmNONE,&MSched);

      mlog.Threshold = tmpLogLevel;

      MLog("INFO:  starting temporary logging at loglevel %d\n",
        tmpLogLevel);

      MLog("INFO:  configfile:  >>>\n%s\n<<<\n",
        (MSched.ConfigBuffer != NULL) ? MSched.ConfigBuffer : "NULL");
      }  /* END BLOCK (case msctlLog) */

      break;

    case msctlQuery:

      {
      char tmpLine[MMAX_LINE];
      char DiagOpt[MMAX_LINE];

      enum MXMLOTypeEnum OType;

      mbitmap_t AuthBM;
      mbitmap_t IFlags;

      mbool_t ShowOnlyGridJobs = FALSE;

      char   *PName = NULL;
      mrm_t  *PR    = NULL;

      bmset(&S->Flags,msftReadOnlyCommand);

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlQuery,
            &IsAdmin,
            tmpLine,        /* O */
            sizeof(tmpLine)) == FAILURE)
        {
        if (!strncasecmp(Auth,"peer:",strlen("peer:")))
          {
          PName = Auth + strlen("peer:");

          sprintf(tmpLine,"ERROR:    cannot locate peer '%s' or peer is not authorized to run command - %s (check moab-private.cfg on %s)\n",
            PName,
            MSchedCtlCmds[CIndex],
            MSched.ServerHost);
          }
        else
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);
          }

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      OType = (enum MXMLOTypeEnum)MUGetIndexCI(ArgString,MXO,TRUE,mxoNONE);

      if ((OType == mxoNONE) && (strncasecmp("pactions",ArgString,strlen("pactions"))))
        {
        /* Originally the mxoxPendingAction enum was pactions, but was changed to paction */

        /* This is here for backwards-compatibility, we will want to remove this if statement */

        OType = mxoxPendingAction;
        }

      if ((OType != mxoxEvent) && (OType != mxoxPendingAction)) 
        {
        /* if a peer is initiating this query */

        if (MUIPeerSetup(S,Auth,&ShowOnlyGridJobs,&PR) == FAILURE)
          {
          MUISAddData(S,"only supported for peer requests");

          return(FAILURE);
          }

        if ((PR->IsInitialized == FALSE) && (OType != mxoSched))
          {
          /* must report failure reason to peer - failure processed in 
             __MMClusterQueryGetData() */

          S->StatusCode = msfEGServerBus;

          MUStrDup(&S->SMsg,"WARNING: client not yet initialized");

          return(FAILURE);
          }
        }    /* END if ((OType != mxoxEvent) && ...) */

      if (OType != mxoNONE)
        {
        /* initialize */

        MXMLGetAttr(RE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt));

        MXMLGetAttr(RE,"flags",NULL,tmpLine,sizeof(tmpLine));

        if (isalpha(tmpLine[0]))
          {
          if (strstr(FlagString,"XML"))
            bmset(&IFlags,mcmXML);
          }

        /* only display raw, local information */

        bmset(&IFlags,mcmExclusive);

        MSysGetAuth(Auth,mcsMSchedCtl,CIndex,&AuthBM);
        }  /* END if (OType != mxoNONE) */

      switch (OType)
        {
        case mxoCluster:
        case mxoALL:

          /* for now, grid requests will always come back in XML */
           
          bmset(&IFlags,mcmXML);

          /* creates 'MXO[mxoNode]' children */

          MUINodeDiagnose(
            Auth,
            NULL,
            mxoNONE,
            NULL,
            &DE,            /* O */
            NULL,
            NULL,           /* partition */
            NULL,           /* access string (not used) */
            DiagOpt,
            &IFlags);

          if (OType == mxoCluster)
            break;

          /* NOTE: for mxoALL requests, fall through to jobs */

        case mxoJob:

          if ((DiagOpt != NULL) &&
              (!strcasecmp(DiagOpt,"starttime")))
            {            
            /* resource availabililty query */

            if (MUIPeerQueryLocalResAvail(
                  S,
                  RE,
                  Auth,
                  DE) == FAILURE)
              {
              return(FAILURE);
              }
            }
          else
            {
            /* report grid workload - @see MMWorkloadQuery() */

            if (MUIPeerJobQuery(
                  PR,
                  Auth,
                  DE,
                  ShowOnlyGridJobs,
                  DiagOpt) == FAILURE)
              {
              return(FAILURE);
              }
            }

          if (OType == mxoJob)
            break;

          /* NOTE: for mxoALL requests, fall through to rsv's */

        case mxoRsv:

          /* report grid reservations */

          if (MUIExportRsvQuery(
                NULL,
                PR,
                Auth,
                DE,
                0) == FAILURE)
            {
            return(FAILURE);
            }

          break;

        case mxoxEvent:

          MUISchedCtlQueryEvents(RE,DE);

          break;

        case mxoxPendingAction:

          MUISchedCtlQueryPendingActions(RE,DE);
 
          break;

        case mxoSched:

          /* report initialization attributes - see MMInitialize() */

          if (MUIPeerSchedQuery(
                PR,
                RE,
                DE,
                Auth,
                &IFlags) == FAILURE)
            {
            return(FAILURE);
            }

          break;

        default:

          {
          mstring_t String(MMAX_LINE);

          /* provide general scheduler attributes */

          MSchedToString(&MSched,mfmXML,FALSE,&String);

          MUISAddData(S,String.c_str());

          return(SUCCESS);
          }  /* END (case default) */

          break;
        }  /* END switch (OType) */

      if ((MSched.DisableUICompression != TRUE) && (strlen(S->SBuffer) > (MMAX_BUFFER >> 1)))
        {
        MSecCompress(
          (unsigned char *)S->SBuffer,
          strlen(S->SBuffer),
          NULL,
          (MSched.EnableEncryption == TRUE) ? (char *)MCONST_CKEY : NULL);
        }

      if (S->SBufSize != 0)
        S->SBufSize = strlen(S->SBuffer);

      return(SUCCESS);
      }  /* END (case msctlQuery) */

      break;

    case msctlCopyFile:

      {
      int rc;
      mxml_t *FE;

      char tmpLine[MMAX_LINE];
      char FileType[MMAX_LINE];

      char EMsg[MMAX_LINE] = {0};

      MXMLGetAttr(RE,"filetype",NULL,FileType,sizeof(FileType));

      if (MXMLGetAttr(RE,"filenum",NULL,tmpLine,sizeof(tmpLine)) == FAILURE)
        {
        /* we are being sent a file */

        if (MXMLGetChild(RE,"FileData",NULL,&FE) == FAILURE)
          {
          /* malformed sending of file */

          return(FAILURE);
          }

        rc = MUISchedCtlRecvFile(Auth,FE,EMsg);
        }
      else
        {
        int FileNum;

        /* remote peer has requested that we send it a file */

        FileNum = strtol(tmpLine,NULL,10);

        rc = MUISchedCtlSendFile(Auth,FileNum,FileType,DE,EMsg);
        }

      return(rc);
      }  /* END BLOCK (case msctlCopyFile) */

      break;

    case msctlCreate:

      {
      int  rc;

      char EMsg[MMAX_BUFFER] = {0};

      rc = MUISchedCtlCreate(Auth,RE,OID,OType,EMsg);

      if (DFormat == mfmXML)
        {
        mxml_t *VE = NULL;

        MXMLFromString(&VE,EMsg,NULL,NULL);

        MXMLAddE(DE,VE);
        }
      else
        {
        MUISAddData(S,EMsg);
        }

      return(rc);
      }  /* END BLOCK (case msctlCreate) */

      break;

    case msctlEvent:

      {
      int rc;

      char EventName[MMAX_LINE];
      enum MPeerEventTypeEnum EventType;
      mrm_t *PR;

      MXMLGetAttr(RE,"eventtype",NULL,EventName,sizeof(EventName));

      EventType = (enum MPeerEventTypeEnum)MUGetIndexCI(EventName,MPeerEventType,FALSE,mpetNONE);

      if ((P == NULL) ||
          (MRMFind(P->Name,&PR) == FAILURE))
        {
        rc = FAILURE;
        }
      else
        {
        rc = MUISchedCtlProcessEvent(PR,Auth,EventType,NULL);
        }

      return(rc);
      }  /* END BLOCK (case msctlEvent) */

      break;

    case msctlModify:

      {
      int rc;

      char  OString[MMAX_LINE];

      if (OType == mxoNONE)
        {
        OType = mxoSched;
        }

      MXMLGetAttr(RE,MSAN[msanOption],NULL,OString,sizeof(OString));

      if ((strstr(OString,"message") != NULL) ||
          (strstr(OString,MXO[mxoTrig]) != NULL) ||
          (strstr(OString,"vpc") != NULL) ||
          (strstr(OString,"sched") != NULL) ||
          (strstr(OString,"gevent") != NULL))
        {
        char EMsg[MMAX_BUFFER] = {0};

        rc = MUISchedCtlModify(
               Auth,
               RE,
               OID,
               OType,
               EMsg);  /* O */

        MUISAddData(S,EMsg);

        return(rc);
        }
      else
        {
        mstring_t Response(MMAX_BUFFER << 2);  /* BIG buffer */

        rc = MUISchedConfigModify(
               Auth,
               RE,
               OID,
               OType,
               &Response);

        MUISAddData(S,Response.c_str());

        return(rc);
        }
      }

      break;

    case msctlList:

      {
      int cindex;

      enum MCfgParamEnum PIndex;

      bmset(&S->Flags,msftReadOnlyCommand);

      if (strstr(OString,"message") != NULL)
        {
        char tmpBuf[MMAX_BUFFER];

        if (MUICheckAuthorization(
              U,
              P,
              NULL,
              mxoSched,
              mcsMSchedCtl,
              msctlList,
              &IsAdmin,
              NULL,
              0) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        switch (DFormat)
          {
          case mfmXML:

            {
            /* DE allocated earlier */

            MMBPrintMessages(
              MSched.MB,
              mfmXML,
              TRUE,
              -1,
              (char *)DE,
              0);
            }  /* END BLOCK */

            break;

          case mfmHuman:
          default:

            {
            char *BPtr;
            int   BSpace;

            /* generate header */

            MMBPrintMessages(
              NULL,
              mfmHuman,
              TRUE,
              -1,
              tmpBuf,
              sizeof(tmpBuf));

            if (MSched.MB != NULL)
              {
              BSpace = sizeof(tmpBuf) - strlen(tmpBuf);
              BPtr   = tmpBuf + strlen(tmpBuf);

              /* display data */

              MMBPrintMessages(
                MSched.MB,
                mfmHuman,
                TRUE,
                -1,
                BPtr,
                BSpace);
              }

            MUISAddData(S,tmpBuf);
            }  /* END BLOCK */

            break;
          }  /* END switch (DFormat) */
        }    /* END if (strstr(OString,"message") != NULL) */
      else if (strstr(OString,"trans") != NULL)
        {
        /* process 'mschedctl -l trans' */

        int  rc;

        rc = MUISchedCtlListTID(S,U,ArgString,DFormat,DE,NULL);

        return(rc);
        }      /* END else if (strstr(OString,"trans") != NULL) */
      else if (strstr(OString,"vpcprofile") != NULL)
        {
        /* list virtual cluster profiles */

        mvpcp_t *C = NULL;

        /* NOTE:  ArgString will optionally contain requested VPC profile name */

        if (ArgString != NULL)
          {
          MVPCProfileFind(ArgString,&C);
          }

        if (MUICheckAuthorization(
              U,
              P,
              C,
              mxoPar,
              mcsMSchedCtl,
              msctlList,
              &IsAdmin,
              NULL,
              0) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        switch (DFormat)
          {
          case mfmHuman:
          default:

            {
            mstring_t tmpBuf(MMAX_LINE);

            MVPCProfileToString(C,Auth,&tmpBuf,IsAdmin);

            MUISAddData(S,tmpBuf.c_str());
            }

            break;

          case mfmXML:

            MVPCProfileToXML(C,Auth,DE,IsAdmin,0);

            break;
          }  /* END switch (DFormat) */

        return(SUCCESS);
        }  /* END else if (strstr(OString,"vpcprofile") != NULL) */
      else if (strstr(OString,"vpc") != NULL)
        {
        /** @see MVPCToXML() */

        /* list virtual cluster profiles */

        mstring_t Buffer(MMAX_LINE);

        char  tmpName[MMAX_LINE];
        char  tmpLine[MMAX_LINE];

        int   CTok;

        mpar_t *SC = NULL;
        mpar_t *C;

        if ((ArgString != NULL) && (ArgString[0] != '\0'))
          {
          MVPCFind(ArgString,&SC,FALSE);
          }

        CTok = -1;

        while (MS3GetWhere(
            RE,
            NULL,
            &CTok,
            tmpName,  /* O */
            sizeof(tmpName),
            tmpLine,  /* O */
            sizeof(tmpLine)) == SUCCESS)
          {
          if ((!strcasecmp(tmpName,MSchedAttr[msaName])) &&
              (MVPCFind(tmpLine,&SC,FALSE) == FAILURE))
            {
            MUISAddData(S,"ERROR: unknown VPC specified\n");

            return(FAILURE);
            }
          }

        memset(&Buffer,0,sizeof(Buffer));

        switch (DFormat)
          {
          case mfmXML:

            /* NO-OP */

            break;

          case mfmHuman:
          default:

            break;
          }  /* END switch (DFormat) */
  
        for (cindex = 0;cindex < MVPC.NumItems;cindex++)
          {
          C = (mpar_t *)MUArrayListGet(&MVPC,cindex);

          if ((C == NULL) || (C->Name[0] == '\0') || (C->Name[0] == '\1'))
            continue;

          if (!bmisset(&C->Flags,mpfIsDynamic))
            continue;

          if ((SC != NULL) && (C != SC))
            {
            continue;
            }

          if (MUICheckAuthorization(
                U,
                P,
                C,
                mxoPar,
                mcsMSchedCtl,
                msctlList,
                &IsAdmin,
                NULL,
                0) == FAILURE)
            {
            continue;
            }

          switch (DFormat)
            {
            case mfmXML:

              {
              mxml_t *CE;

              const enum MParAttrEnum AList[] = {
                mpaACL,
                mpaCmdLine,
                mpaCost,
                mpaDuration,
                mpaID,
                mpaMessages,
                mpaOwner,
                mpaPriority,
                mpaProfile,
                mpaPurgeTime,    
                mpaReqResources,
                mpaRsvGroup, 
                mpaSize,  
                mpaStartTime,   
                mpaState,       
                mpaVariables,
                mpaNONE };

              CE = NULL;

              if ((MXMLCreateE(&CE,(char *)MXO[mxoPar]) == SUCCESS) &&
                  (MVPCToXML(C,CE,(enum MParAttrEnum *)AList) == SUCCESS))
                {
                MXMLAddE(DE,CE);
                }
              }  /* END (case mfmXML) */

              break;

            case mfmHuman:
            default:

              MVPCShow(C,&Buffer);

              break;
            }  /* END switch (DFormat) */
          }    /* END for (cindex) */

        switch (DFormat)
          {
          case mfmXML:

            /* NO-OP */

            break;

          case mfmHuman:
          default:

            MUISAddData(S,Buffer.c_str());

            break;
          }  /* END switch (DFormat) */
          
        return(SUCCESS);
        }  /* END else if (strstr(OString,"vpc") != NULL) */
      else if (MUGetIndexCI(OString,MAttrType,FALSE,meLAST) != meLAST)/*(strstr(OString,"gres") != NULL)*/
        {
        char tmpLine[MMAX_LINE * 4];
        int iter;
        char *BPtr = NULL;
        int BSpace;
        int OIndex = 0; 

        OIndex = MUGetIndexCI(OString,MAttrType,FALSE,meLAST);

        switch (OIndex)
          {
          case meOpsys:

            if (IsVerbose == TRUE)
              {
              switch (DFormat)
                {
                case mfmXML:

                  {
                  mxml_t *CE;

                  mhashiter_t Iter;
                  int         iter;

                  /* Display OSList and VMOSList */

                  /* NOTE:  report msched_t structures mhash_t OSList, mhash_t VMOSList[MMAX_ATTR + 1] */

                  MUHTIterInit(&Iter);

                  if (MXMLCreateE(&CE,(char *)"oslist") == SUCCESS)
                    {
                    MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

                    for (iter = 1;iter < MMAX_ATTR;iter++)
                      {
                      if (MAList[OIndex][iter][0] == '\0')
                        break;

                      if (tmpLine[0] != '\0')
                        MUSNCat(&BPtr,&BSpace,",");

                      MUSNCat(&BPtr,&BSpace,MAList[OIndex][iter]);
                      }

                    MXMLSetVal(CE,(void *)tmpLine,mdfString);

                    MXMLAddE(DE,CE);
                    }

                  MUHTIterInit(&Iter);

                  for (iter = 1;iter < MMAX_ATTR;iter++)
                    {
                    char *OSName;

                    if (MAList[OIndex][iter][0] == '\0')
                     break;

                    CE = NULL;

                    if (MXMLCreateE(&CE,(char *)"vmoslist") == SUCCESS)
                      {
                      MXMLSetAttr(CE,"os",(void *)MAList[OIndex][iter],mdfString);

                      MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

                      MUHTIterInit(&Iter);

                      while (MUHTIterate(&MSched.VMOSList[iter],&OSName,NULL,NULL,&Iter) == SUCCESS)
                        {
                        if (tmpLine[0] != '\0')
                          MUSNCat(&BPtr,&BSpace,",");

                        MUSNCat(&BPtr,&BSpace,OSName);
                        }

                      MXMLSetVal(CE,(void *)tmpLine,mdfString);

                      MXMLAddE(DE,CE);
                      }
                    }    /* END for (iter) */
                  }      /* END BLOCK */

                  break;

                case mfmHuman:
                default:

                  {
                  char *OSName;

                  mhashiter_t Iter;
                  int         iter;

                  /* Display OSList and VMOSList */

                  /* NOTE:  report msched_t structures mhash_t OSList, mhash_t VMOSList[MMAX_ATTR + 1] */

                  MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

                  MUHTIterInit(&Iter);

                  MUSNCat(&BPtr,&BSpace,"oslist=");
                  for (iter = 1;iter < MMAX_ATTR;iter++)
                    {
                    if (MAList[OIndex][iter][0] == '\0')
                      break;

                    if (iter>1)
                      MUSNCat(&BPtr,&BSpace,",");
                    MUSNCat(&BPtr,&BSpace,MAList[OIndex][iter]);
                    }

                  /* add a new line */
                  MUSNCat(&BPtr,&BSpace,"\n");

                  MUHTIterInit(&Iter);

                  MUSNCat(&BPtr,&BSpace,"vmoslist=");

                  /* reset our iter variable */
                  iter=0;

                  /* NOTE:  is only VMOSList[0] populated? */

                  while (MUHTIterate(&MSched.VMOSList[0],&OSName,NULL,NULL,&Iter) == SUCCESS)
                    {
                    if (iter++>0)
                      MUSNCat(&BPtr,&BSpace,",");

                    MUSNCat(&BPtr,&BSpace,OSName);
                    }
                  }
                  break;
                }  /* END switch (DFormat) */

              break;
              }    /* END if (IsVerbose == TRUE) */

            /* NOTE:  Fall Through */

          case meGMetrics:

            {
            int gmindex;

            MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

            for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
              {
              if (MGMetric.Name[gmindex][0] == '\0')      /* if no name, we are done */
                break;

              if (tmpLine[0] != '\0')
                MUSNCat(&BPtr,&BSpace,",");

              MUSNCat(&BPtr,&BSpace,MGMetric.Name[gmindex]);
              }

            if (tmpLine[0] == '\0')
              {
              MUSNCat(&BPtr,&BSpace,"no configured or reported gmetrics");
              }
            } /* END BLOCK */

            break;

          case meGRes:

            {
            int gindex = 0;

            MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

            for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
              {
              if (MGRes.Name[gindex][0] == '\0')
                break;

              if (tmpLine[0] != '\0')
                MUSNCat(&BPtr,&BSpace,",");

              MUSNCat(&BPtr,&BSpace,MGRes.Name[gindex]);
              }

            if (tmpLine[0] == '\0')
              {
              MUSNCat(&BPtr,&BSpace,"no configured or reported gres");
              }
            } /* END BLOCK */

            break;

          case meNFeature:
          case meArch:
          case meJFeature:

            {
            MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

            for (iter = 1;iter < MMAX_ATTR;iter++)
              {
              if (MAList[OIndex][iter][0] == '\0')
                break;

              if (tmpLine[0] != '\0')
                MUSNCat(&BPtr,&BSpace,",");

              MUSNCat(&BPtr,&BSpace,MAList[OIndex][iter]);
              }

            /*MUArrayToString((const char **)&MGRes.Name[1],",",tmpLine,sizeof(tmpLine));*/

            if (tmpLine[0] == '\0')
              {
              MUSNCat(&BPtr,&BSpace,"no configured or reported ");
              MUSNCat(&BPtr,&BSpace,MAttrType[OIndex]);
              }
            } /* END BLOCK */

            break;

          case meGEvent:

            {
            char                *GEName = NULL;
            mgevent_desc_t      *GEventDesc = NULL;
            mgevent_iter_t       GEIter;

            MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));


            /* Walk the MGEvent Desc table for each item */
            MGEventIterInit(&GEIter);

            while (MGEventDescIterate(&GEName,&GEventDesc,&GEIter) == SUCCESS)
              {
              if (tmpLine[0] != '\0')
                MUSNCat(&BPtr,&BSpace,",");

              MUSNCat(&BPtr,&BSpace,GEName);
              }

            if (tmpLine[0] == '\0')
              {
              MUSNCat(&BPtr,&BSpace,"no configured or reported ");
              MUSNCat(&BPtr,&BSpace,MAttrType[OIndex]);
              }
            } /* END BLOCK meGEvent */

            break;

          default:

            MUStrCpy(tmpLine,"cannot list given object type",sizeof(tmpLine));

            MUISAddData(S,tmpLine);

            return(FAILURE);

            break;
          } /* END switch(OIndex) */

        MUISAddData(S,tmpLine);

        return(SUCCESS);
        }  /* END else if (strstr(OString,"gres") != NULL) */
      else
        {
        /* handle showconfig request */

        char tmpBuf[MMAX_BUFFER << 3];   /* NOTE:  should make buffer dynamic (NYI) */

        /* list config by default */

        if (MUICheckAuthorization(
              U,
              P,
              NULL,
              mxoSched,
              mcsMSchedCtl,
              msctlList,
              &IsAdmin,
              NULL,
              0) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
            Auth);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if ((SSNumber >= 0) && (SSNumber >= MSched.ConfigSerialNumber))
          {
          /* no config updates available */

          return(SUCCESS);
          }

        /* remove trailing whitespace */

        for (cindex = 0;cindex < (int)sizeof(ArgString);cindex++)
          {
          if ((ArgString[cindex] != '\0') && !isspace(ArgString[cindex]))
            continue;

          ArgString[cindex] = '\0';

          break;
          }  /* END for (cindex) */

        if (strstr(ArgString,"CONFIGSERIALNUMBER"))
          {
          sprintf(tmpBuf,"CONFIGSERIALNUMBER %d\n",
            MSched.ConfigSerialNumber);
          }
        else if (strstr(ArgString,"MOABVERSION"))
          {
          sprintf(tmpBuf,"MOABVERSION %s\n",
            MOAB_VERSION);
          }
        else
          {
          PIndex = (enum MCfgParamEnum)MUGetIndexCI(
            ArgString,
            (const char **)MParam,
            FALSE,
            mcoNONE);

          MUIConfigShow(Auth,PIndex,IsVerbose,tmpBuf,sizeof(tmpBuf));

          if (SSNumber >= 0)
            {
            char tmpLine[MMAX_LINE];

            /* append serial number if serial number specified */

            sprintf(tmpLine,"CONFIGSERIALNUMBER %d\n",
              MSched.ConfigSerialNumber);

            strncat(tmpBuf,tmpLine,sizeof(tmpBuf) - strlen(tmpBuf));
            }
          }    /* END else (strstr(ArgString,"CONFIGSERIALNUMBER")) */

        MUISAddData(S,tmpBuf);

        return(SUCCESS);
        }  /* END else */

      return(SUCCESS);
      }  /* END BLOCK */

      /*NOTREACHED*/

      break;

    case msctlInit:

      {
      /* initialize resource manager - see MMInitialize() */

      if (MUICheckAuthorization(
            U,
            P,
            NULL,
            mxoSched,
            mcsMSchedCtl,
            msctlInit,
            &IsAdmin,
            NULL,
            0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      sprintf(tmpLine,"initialize command not supported for RM\n");

      MUISAddData(S,tmpLine);

      return(SUCCESS);
      }  /* END BLOCK */

      /*NOTREACHED*/

      break;

    default:

      MDB(2,fUI) MLog("WARNING:  received unexpected sched command '%s'\n",
        MSchedCtlCmds[CIndex]);

      sprintf(tmpLine,"ERROR:    unexpected sched command: '%s'\n",
        MSchedCtlCmds[CIndex]);

      MUISAddData(S,tmpLine);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (CIndex) */

  return(SUCCESS);
  }  /* END MUISchedCtl() */
/* END MUISchedCtl.c */
