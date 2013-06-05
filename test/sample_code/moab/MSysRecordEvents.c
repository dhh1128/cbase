/* HEADER */

      
/**
 * @file MSysRecordEvents.c
 *
 * Contains:Function to persist Events
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


/**
 * Record an event for an executed command
 *
 * @param S      (I) Socket with commmandline (and other extraneous) xml
 * @param SIndex (I) Which command type
 * @param RC     (I) return code of command.
 * @param Auth   (I) Who is running the command.
 *
 * @see MUISProcessRequest() - parent
 */

int MSysRecordCommandEvent(

  msocket_t        *S,        /* I */
  enum MSvcEnum     SIndex,   /* I */
  int               RC,       /* * */
  char             *Auth)     /* I */

  {
  char tmpLine[MMAX_LINE] = {0};
  char tmpMsg[MMAX_LINE];

  tmpLine[0] = '\0';

  if (S->RDE != NULL) 
    {
    if (MXMLGetAttr(S->RDE,"cmdline",NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      MUStringUnpack(tmpLine,tmpLine,sizeof(tmpLine),NULL);
      }
    }

  if (strncmp(Auth,"peer:",strlen("peer:")) == 0)
    {
    /* do not record grid requests */

    return(FAILURE);
    }

  if (tmpLine[0] == '\0')
    {
    strcpy(tmpLine,"command-line arg info not available");
    }

  snprintf(tmpMsg,sizeof(tmpMsg),"command %s by user %s from host %s %s '%s'",
    MUI[SIndex].SName,
    Auth,
    (S->RemoteHost != NULL) ? S->RemoteHost : "???",
    (RC == SUCCESS) ? "succeeded" : "failed",
    tmpLine);

  MSysRecordEvent(
    mxoSched,
    MSched.Name,
    mrelClientCommand,
    Auth,
    tmpMsg,
    NULL);

  if (MSched.PushEventsToWebService == TRUE)
    {
    MEventLog *Log = new MEventLog(meltSchedCommand);
    Log->SetCategory(melcCommand);
    Log->SetFacility(melfScheduler);
    Log->SetPrimaryObject(MSched.Name,mxoSched,NULL);
    Log->AddDetail("msg",tmpMsg);

    MEventLogExportToWebServices(Log);

    delete Log;
    }
        
  return(SUCCESS);
  }  /* END MSysRecordCommandEvent() */


/**
 * Record event to event file.
 *
 * @see MTraceLoadWorkload() - peer - parse event records - event format must be synchronized
 * @see MDBWriteEvent() - peer - Records events to database.
 *
 * @param OType (I)
 * @param OID (I)
 * @param EType (I)
 * @param EID (I)
 * @param Name (I) [optional]
 * @param Msg (I) [optional]
 */

int MSysRecordEventToFile(

  enum MXMLOTypeEnum         OType,  /* I */
  char                      *OID,    /* I */
  enum MRecordEventTypeEnum  EType,  /* I */
  int                        EID,    /* I */
  char                      *Name,   /* I (optional) */
  char                      *Msg)    /* I (optional) */

  {
  struct tm *tmpT;

  time_t now = {0};

  char   tmpTime[MMAX_NAME];

  if (MStat.eventfp == NULL)
    {
    return(FAILURE);
    }

  /* FORMAT:  HH:MM:SS ETIME:EID OTYPE OID ETYPE DETAILS */

  MUGetTime((mulong *)&now,mtmNONE,&MSched);

  tmpT = localtime(&now);

  if (tmpT != NULL)
    {
    sprintf(tmpTime,"%02d:%02d:%02d",
      tmpT->tm_hour,
      tmpT->tm_min,
      tmpT->tm_sec);
    }
  else
    {
    strcpy(tmpTime,"??:??:??");
    }

  mstring_t String(MMAX_LINE);

  /* NOTE:  sync format with MSysQueryEvents() */

  MStringAppendF(&String,"%s %ld:%d %-8s %-12s %-12s %s",
    tmpTime,
    MSched.Time,
    EID,
    MXO[OType],
    (OID != NULL) ? OID : "-",
    (EType != mrelNONE) ? MRecordEventType[EType] : "-",
    (Msg != NULL) ? Msg : "-");

  if (MSched.WikiEvents == TRUE && Name != NULL)
    {
    size_t len = String.length();

    if ((len > 1) && String[len-1] == '\n')
      {
      /* Strip off final newline */

      MStringStripFromIndex(&String, -1);
      }
    MStringAppendF(&String," USER=%s", (Name == NULL ? "NONE" : Name));
    }

  fprintf(MStat.eventfp,"%s\n",String.c_str());

  fflush(MStat.eventfp);

  switch (MSched.AccountingInterfaceProtocol)
    {
    case mbpFile:

      /* write data to specified file */

      MFUCreate(
        MSched.AccountingInterfacePath,
        NULL,
        (void *)String.c_str(),
        strlen(String.c_str()),
        -1,
        -1,
        -1,
        TRUE,
        NULL);

      break;

    case mbpExec:

      /* pass to script as STDIN */

      MUSpawnChild(
        MSched.AccountingInterfacePath,
        NULL,
        NULL,
        MDEF_SHELL,
        -1,
        -1,
        NULL,
        NULL,
        NULL,
        NULL,
        String.c_str(),  /* I stdin */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        100000,
        0,
        NULL,
        mxoNONE,
        FALSE,
        FALSE,
        NULL);

      break;

    default:

      /* processing handled elsewhere */

      /* NO-OP */

      break;
    }  /* END switch (MSched.AccountingInterfaceProtocol) */

  if (MSched.RecordEventToSyslog == TRUE)
    {
    MOSSyslog(LOG_NOTICE,String.c_str());
    }

  /* fsync() should also be used (NYI) */

  /* NOTE: for compat mode */

#define PBSEVENT_ERROR          0x0001          /* internal errors            */
#define PBSEVENT_SYSTEM         0x0002          /* system (server) events     */
#define PBSEVENT_ADMIN          0x0004          /* admin events               */
#define PBSEVENT_JOB            0x0008          /* job related events         */
#define PBSEVENT_JOB_USAGE      0x0010          /* end of Job accounting      */
#define PBSEVENT_SECURITY       0x0020          /* security violation events  */
#define PBSEVENT_SCHED          0x0040          /* scheduler events           */
#define PBSEVENT_DEBUG          0x0080          /* common debug messages      */
#define PBSEVENT_DEBUG2         0x0100          /* less needed debug messages */
#define PBSEVENT_FORCE          0x8000          /* set to force a message     */

  if (MStat.eventcompatfp != NULL)
    {
    int eventtype;

    switch (EType)
      {
      case  mrelJobCancel:
      case  mrelJobCheckpoint:
      case  mrelJobComplete:
      case  mrelJobFailure:
      case  mrelJobMigrate:
      case  mrelJobPreempt:
      case  mrelJobReject:
      case  mrelJobResume:
      case  mrelJobStart:
      case  mrelJobSubmit:

        eventtype = PBSEVENT_JOB;

        break;

      case  mrelNONE:
      case  mrelGEvent:
      case  mrelNodeDown:
      case  mrelNodeFailure:
      case  mrelNodeUp:
      case  mrelQOSViolation:
      case  mrelRMDown:
      case  mrelRMUp:
      case  mrelRsvCancel:
      case  mrelRsvCreate:
      case  mrelRsvEnd:
      case  mrelRsvStart:
      case  mrelSchedCommand:
      case  mrelSchedFailure:
      case  mrelSchedModify:
      case  mrelSchedPause:
      case  mrelSchedRecycle:
      case  mrelSchedResume:
      case  mrelSchedStart:
      case  mrelSchedStop:
      case  mrelTrigEnd:
      case  mrelTrigFailure:
      case  mrelTrigStart:    /* 33 */
      case  mrelLAST:
      default:

        eventtype = 0;

        break;
      }  /* END switch (EType) */

    if ((eventtype != 0) && (tmpT != NULL))
      {
      fprintf(MStat.eventcompatfp,"%02d/%02d/%04d %02d:%02d:%02d;%04x;sched;%s;%s;%s",
        tmpT->tm_mon + 1,
        tmpT->tm_mday,
        tmpT->tm_year + 1900,
        tmpT->tm_hour,
        tmpT->tm_min,
        tmpT->tm_sec,
        eventtype,
        MXO[OType],
        OID,
        Msg);

      fflush(MStat.eventcompatfp);
      }
    }   /* END if (MStat.eventcompatfp != NULL) */

  return(SUCCESS);
  }  /* END MSysRecordEventToFile() */



/**
 * Record event to appropriate locations.
 *
 * @see MOWriteEvent() - parent
 * @see MTraceLoadWorkload() - peer - parse event records - event format must be synchronized
 * @see MSysQueryEvents() - peer - parse event records - event fromat must be synchronized
 *
 * @param OType (I)
 * @param OID (I)
 * @param EType (I)
 * @param Name (I)
 * @param Msg (I) [optional]
 * @param EID (O) [optional]
 */

int MSysRecordEvent(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  enum MRecordEventTypeEnum  EType,
  char                      *Name,
  char                      *Msg,
  int                       *EID)

  {
  int rc = SUCCESS;
  mdb_t *MDBInfo;

  if (!bmisset(&MSched.RecordEventList,EType))
    {
    return(SUCCESS);
    }

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MDBInfo->DBType != mdbNONE)
    {
    mbool_t Retry;

    enum MStatusCodeEnum SC = mscNoError;

    do
      {
      Retry = FALSE;

      MDBWriteEvent(MDBInfo,OType,OID,EType,MSched.EventCounter,Name,Msg,&SC);

      if (MDBShouldRetry(SC))
        {
        Retry = TRUE;

        MDBFreeDPtr(&MDBInfo);

        MSysInitDB(MDBInfo);

        MDBInfo->DBType = MSched.ReqMDBType;

        MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);
        }
      } while (Retry == TRUE);
    }

  rc &= MSysRecordEventToFile(OType,OID,EType,MSched.EventCounter,Name,Msg);

  if (EID != NULL)
    *EID = MSched.EventCounter;

  MSysUpdateEventCounter(MSched.EventCounter);

  return(rc);
  }




/* END MSysRecordEvents.c */
