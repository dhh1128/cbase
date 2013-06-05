/* HEADER */

/**
 * @file MUISchedCtlSupport.c
 *
 * Contains MUI Scheduler Control support routines
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * process 'mschedctl -l event'
 *
 * @see MUISchedCtl() - parent
 * @see MSysQueryEventsToXML() - child
 */

int MUISchedCtlQueryEvents(

  mxml_t *RE,  /* I */
  mxml_t *DE)  /* O */

  {
  #define MMAX_EVENTQUERY   32

  mulong StartTime;
  mulong EndTime;

  char   tmpLine[MMAX_LINE];
  char   tmpName[MMAX_LINE];

  int    CTok;

  enum MRecordEventTypeEnum EType[MMAX_EVENTQUERY + 1];
  enum MXMLOTypeEnum        OType[MMAX_EVENTQUERY + 1];

  char   OID[MMAX_EVENTQUERY + 1][MMAX_LINE];
  int    EID[MMAX_EVENTQUERY + 1];  /* specified event id's */

  if ((RE == NULL) || (DE == NULL))
    {
    return(FAILURE);
    }

  StartTime = 0;
  EndTime = MMAX_TIME;

  /* select all object id's */

  OID[0][0] = '\0';

  /* select all event types */

  EType[0] = mrelLAST;

  /* select all event instances */

  EID[0] = -1;

  /* select all object types */

  OType[0] = mxoLAST;

  CTok = -1;

  while (MS3GetWhere(
      RE,
      NULL,
      &CTok,
      tmpName,                     /* O */
      sizeof(tmpName),             
      tmpLine,                     /* O */
      sizeof(tmpLine)) == SUCCESS) 
    {
    if (!strcasecmp(tmpName,"starttime"))
      {
      StartTime = MUTimeFromString(tmpLine);

      continue;
      }

    if (!strcasecmp(tmpName,"endtime"))
      {
      EndTime = MUTimeFromString(tmpLine);

      continue;
      }

    if (!strcasecmp(tmpName,"eventtypes"))
      {
      int   eindex;

      char *ptr;
      char *TokPtr;

      int   tmpI;

      eindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        tmpI = MUGetIndexCI(ptr,MRecordEventType,FALSE,0);

        if (tmpI != 0)
          EType[eindex++] = (enum MRecordEventTypeEnum)tmpI;

        if (eindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      EType[eindex] = mrelLAST;

      continue;
      }  /* END if (!strcasecmp(tmpName,"eventtypes")) */

    if (!strcasecmp(tmpName,"oidlist"))
      {
      int   oindex;

      char *ptr;
      char *TokPtr;

      oindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        MUStrCpy(OID[oindex++],ptr,sizeof(OID[0]));

        if (oindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      OID[oindex][0] = '\0';

      continue;
      }  /* END if (!strcasecmp(tmpName,"oidlist")) */

    if (!strcasecmp(tmpName,"eidlist"))
      {
      int   oindex;

      char *ptr;
      char *TokPtr;

      oindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        EID[oindex++] = (int)strtol(ptr,NULL,10);

        if (oindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      EID[oindex] = -1;

      continue;
      }  /* END if (!strcasecmp(tmpName,"eidlist")) */

    if (!strcasecmp(tmpName,"objectlist"))
      {
      int   oindex;

      char *ptr;
      char *TokPtr;

      int   tmpI;

      oindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        tmpI = MUGetIndexCI(ptr,MXO,FALSE,0);

        if (tmpI != 0)
          OType[oindex++] = (enum MXMLOTypeEnum)tmpI;

        if (oindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      OType[oindex] = mxoLAST;

      continue;
      }  /* END if (!strcasecmp(tmpName,"objectlist")) */
    }    /* END while (MS3GetWhere() == SUCCESS) */

  MSysQueryEventsToXML(
    StartTime,
    EndTime,
    EID,
    EType,
    OType,
    OID,
    DE);

  return(SUCCESS);
  }  /* END MUISchedCtlQueryEvents() */






/**
 * process 'mschedctl -q pactions'
 *
 * @see MUISchedCtl() - parent
 * @see MSysQueryPendingActionToXML() - child
 */

int MUISchedCtlQueryPendingActions(

  mxml_t *RE,  /* I */
  mxml_t *DE)  /* O */

  {
  mulong StartTime;
  mulong EndTime;

  char   tmpLine[MMAX_LINE];
  char   tmpName[MMAX_LINE];

  int    CTok;

  if ((RE == NULL) || (DE == NULL))
    {
    return(FAILURE);
    }

  StartTime = 0;
  EndTime = MMAX_TIME;

  CTok = -1;

  while (MS3GetWhere(
      RE,
      NULL,
      &CTok,
      tmpName,                     /* O */
      sizeof(tmpName),             
      tmpLine,                     /* O */
      sizeof(tmpLine)) == SUCCESS) 
    {
    /* NOTE:  currently ignore non-time based pending actions search constraints */

    if (!strcasecmp(tmpName,"starttime"))
      {
      StartTime = MUTimeFromString(tmpLine);

      continue;
      }

    if (!strcasecmp(tmpName,"endtime"))
      {
      EndTime = MUTimeFromString(tmpLine);

      continue;
      }
    }    /* END while (MS3GetWhere() == SUCCESS) */

  MSysQueryPendingActionToXML(
    StartTime,
    EndTime,
    DE);

  return(SUCCESS);
  }  /* END MUISchedCtlQueryPendingActions() */





/**
 *
 *
 * @param Auth             (I)
 * @param RequestedFileNum (I)
 * @param Type             (I) [optional]
 * @param DE               (O) response
 * @param EMsg             (O) [optional,minsize=MMAX_LINE]
 */

int MUISchedCtlSendFile(

  char     *Auth,
  int       RequestedFileNum,
  char     *Type,
  mxml_t   *DE,
  char     *EMsg)

  {
  mxml_t *FE;

  char *DirPath;
  DIR  *DirHandle;

  struct dirent *FileHandle;
  const char *FilePattern;
  char *File;
  char  FilePath[MMAX_PATH];
  int   FileCount;

  char   *tmpBuffer = NULL;
  int     tmpBufSize = 0;

  mbool_t IsAdmin   = FALSE;

  mpsi_t   *P = NULL;
  mgcred_t *U = NULL;

  const char *FName = "MUISchedCtlSendFile";

  MDB(3,fUI) MLog("%s(%s,%d,%s,DE,EMsg)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL",
    RequestedFileNum,
    (Type != NULL) ? Type : "NULL");

  if ((DE == NULL) || (Auth == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    if (MPeerFind(Auth,FALSE,&P,FALSE) == FAILURE)
      {
      /* NYI */
      }      
    }
  else
    {
    MUserAdd(Auth,&U);
    }

  if (MUICheckAuthorization(
        U,
        P,
        NULL,
        mxoSched,
        mcsMSchedCtl,
        msctlCopyFile,
        &IsAdmin,
        NULL,
        0) == FAILURE)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"ERROR:    user '%s' is not authorized to run command",
        Auth);
      }

    return(FAILURE);
    }

  if ((Type != NULL) && (Type[0] != '\0'))
    {
    if (!strcmp(Type,"fairshare"))
      {
      DirPath = MStat.StatDir;
      FilePattern = "FS-";
      }
    else if (!strcmp(Type,"checkpoint"))
      {
      DirPath = MSched.SpoolDir;
      FilePattern = ".cp";
      }
    else
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:   invalid file transfer type '%s' specified",
          Type);
        }

      return(FAILURE);
      }
    }
  else
    {
    /* default is getting the checkpoint files */

    DirPath = MSched.SpoolDir;
    FilePattern = ".cp";
    }

  DirHandle = opendir(DirPath);
 
  if (DirHandle == NULL)
    {
    return(FAILURE);
    }

  FileCount = 0;
   
  FileHandle = readdir(DirHandle);
 
  while (FileHandle != NULL)
    {
    /* find the requested spool file */

    if (strstr(FileHandle->d_name,FilePattern) == NULL)
      {
      FileHandle = readdir(DirHandle);

      continue;
      }

    if (FileCount == RequestedFileNum)
      break;

    FileHandle = readdir(DirHandle);

    FileCount++;
    }

  closedir(DirHandle);

  if (FileHandle == NULL)
    {
    /* could not locate the requested file */

    return(FAILURE);
    }

  File = FileHandle->d_name;

  snprintf(FilePath,sizeof(FilePath),"%s/%s",
    DirPath,
    File);

  /* load file into buffer */

  if (MUPackFileNoCache(FilePath,&tmpBuffer,&tmpBufSize,EMsg) == FAILURE)
    {
    return(FAILURE);
    }
 
  /* <Data
   *   <FileData file="spool/job1.cp">...</FileData>
   * </Data> */

  MXMLCreateE(&FE,"FileData");

  MXMLSetAttr(FE,(char *)"file",(void *)FilePath,mdfString);

  MXMLSetVal(FE,(void *)tmpBuffer,mdfString);

  MUFree(&tmpBuffer);

  MXMLAddE(DE,FE);

  return(SUCCESS);
  }  /* END MUISchedCtlSendFile() */




/**
 * Process mschedctl 'create' client request.
 *
 * @see MUISchedCtl() - parent 
 * @see MUIVPCCreate() - child
 *
 * @param Auth (I)
 * @param RE (I)
 * @param OID (I)
 * @param OType (I)
 * @param EMsg (O) [minsize=MMAX_BUFFER]
 */

int MUISchedCtlCreate(

  char                 *Auth,  /* I */
  mxml_t               *RE,    /* I */
  char                 *OID,   /* I */
  enum MXMLOTypeEnum    OType, /* I */
  char                 *EMsg)  /* O (minsize=MMAX_BUFFER) */

  {
  int   rc;
  char  tmpBuf[MMAX_BUFFER];
  char  FlagString[MMAX_LINE];
  char  OString[MMAX_LINE];
  char  ArgString[MMAX_LINE];

  char *BPtr;
  int   BSpace;

  mbitmap_t  Flags;

  mbool_t IsAdmin = FALSE;

  void *O = NULL;

  mpsi_t   *P = NULL;
  mgcred_t *U = NULL;

  if ((EMsg == NULL) || 
      (RE == NULL))
    {
    return(FAILURE);
    }

  EMsg[0] = '\0';

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    if (MPeerFind(Auth,FALSE,&P,FALSE) == FAILURE)
      {
      /* NYI */
      }      
    }
  else
    {
    MUserAdd(Auth,&U);
    }

  if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0) == SUCCESS)
    {
    if (strcasestr(FlagString,"verb"))
      bmset(&Flags,mcmVerbose);

    if (strcasestr(FlagString,"xml"))
      bmset(&Flags,mcmXML);
    }  /* END if (MXMLGetAttr(RE,MSAN[msanFlags],...) == SUCCESS) */

  MXMLGetAttr(RE,MSAN[msanOption],NULL,OString,sizeof(OString));

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));

  MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

  /* NOTE:  flags available in <flag>[,<flag>] format */

  if (strstr(OString,"message") != NULL)
    {
    if (OType == mxoNONE)
      {
      OType = mxoSched;
      }

    if (MOGetObject(OType,OID,(void **)&O,mVerify) == FAILURE)
      {
      sprintf(EMsg,"Cannot locate object");

      return(FAILURE);
      }
    }
  else if (strstr(OString,MXO[mxoTrig]) != NULL)
    {
    if (MOGetObject(OType,OID,(void **)&O,mVerify) == FAILURE)
      {
      sprintf(EMsg,"Cannot locate object");

      return(FAILURE);
      }
    } 
 
  if (strstr(OString,"message") != NULL)
    {
    char tmpName[MMAX_NAME];
    char tmpVal[MMAX_LINE];
    char tmpOwner[MMAX_NAME];

    int  aindex;

    int  CTok;

    mmb_t Message;

    if (MUICheckAuthorization(
          U,
          NULL,
          NULL,
          mxoSched,
          mcsMSchedCtl,
          msctlCreate,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      snprintf(EMsg,MMAX_BUFFER,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }

    memset(&Message,0,sizeof(Message));

    /* set defaults */

    Message.ExpireTime = MSched.Time + MCONST_HOURLEN; 
    Message.Type       = mmbtOther;
    Message.Count      = 1;
    Message.Priority   = 1;
    Message.Owner      = Auth;

    CTok = -1;

    while (MS3GetWhere(
        RE,
        NULL,
        &CTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
      {
      aindex = MUGetIndexCI(tmpName,MMBAttr,FALSE,mmbaNONE);

      switch (aindex)
        {
        case mmbaCount:

          Message.Count = (int)strtol(tmpVal,NULL,0);

          break;

        case mmbaExpireTime:

          Message.ExpireTime = MUTimeFromString(tmpVal);

          break;

        case mmbaOwner:

          MUStrCpy(tmpOwner,tmpVal,sizeof(tmpOwner));

          Message.Owner = tmpOwner;

          break;

        case mmbaPriority:

          Message.Priority = (int)strtol(tmpVal,NULL,0);

          break;

        case mmbaType:

          Message.Type = (enum MMBTypeEnum)MUGetIndexCI(tmpVal,MMBType,FALSE,mmbtOther);

          break;

        default:

          /* not handled */

          /* NO-OP */

          break;
        }   /* END switch (aindex) */
      }     /* END while (MS3GetWhere(RE,NULL,&CTok) == SUCCESS) */

    if (ArgString[0] == '\0')
      {
      sprintf(EMsg,"Message is empty");

      return(FAILURE);
      }

    Message.Data = ArgString;

    rc = MOSetMessage(O,OType,(void **)&Message,mdfOther,mSet);

    if (rc == FAILURE)
      {
      sprintf(EMsg,"Cannot set message");

      return(FAILURE);
      }

    sprintf(EMsg,"INFO:  message added\n");
    }
  else if (strstr(OString,MXO[mxoTrig]) != NULL)
    {
    marray_t **OTListP;
    marray_t   OTList;
    mtrig_t   *T;

    int       tindex;
    char      tmpLine[MMAX_LINE];

    if (OType == mxoNONE)
      {
      sprintf(EMsg,"ERROR:    no object specified\n");

      return(FAILURE);
      }

    if (MUICheckAuthorization(
          U,
          NULL,
          NULL,
          mxoTrig,
          mcsMSchedCtl,
          msctlCreate,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      snprintf(EMsg,MMAX_BUFFER,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }

    /* NOTE: otype/oid loaded in 'Set' processing */

    /* FORMAT:  <TSPEC>[,<TSPEC>]... */

    /* locate object trigger list */

    if (MOGetComponent(O,OType,(void **)&OTListP,mxoTrig) == FAILURE)
      {
      sprintf(EMsg,"cannot process parent object");

      return(FAILURE);
      }

    MUArrayListCreate(&OTList,sizeof(mtrig_t *),10);

    if (MTrigLoadString(
          OTListP,
          ArgString,
          TRUE,
          FALSE,
          OType,
          OID,
          &OTList,
          tmpLine) == FAILURE)
      {
      snprintf(EMsg,MMAX_BUFFER,"cannot parse trigger specification - %s",
        tmpLine);

      MUArrayListFree(&OTList);
      return(FAILURE);
      }

    if (*OTListP == NULL)
      {
      sprintf(EMsg,"cannot locate object trigger list");

      MUArrayListFree(&OTList);
      return(FAILURE);
      }

    /* OTList contains list of newly created triggers */

    for (tindex = 0;tindex < OTList.NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(&OTList,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      /* populate T with object info */

      T->OType = OType;

      if (OID[0] != '\0')
        MUStrDup(&T->OID,OID);

      if (MTrigInitialize(T) == FAILURE)
        {
        sprintf(EMsg,"cannot find object");

        MUArrayListFree(&OTList);
        return(FAILURE);
        }

      switch (T->OType)
        {
        case mxoSched:

          MOReportEvent((void *)&MSched,MSched.Name,mxoSched,mttCreate,0,FALSE);
          MOReportEvent((void *)&MSched,MSched.Name,mxoSched,mttStart,0,FALSE);
          MOReportEvent((void *)&MSched,MSched.Name,mxoSched,mttEnd,MMAX_TIME,FALSE);

          break;

        case mxoRsv:

          {
          mrsv_t *R;

          if (O == NULL)
            break;

          R = (mrsv_t *)O;

          MOReportEvent(O,R->Name,mxoRsv,mttCreate,R->CTime,TRUE);
          MOReportEvent(O,R->Name,mxoRsv,mttStart,R->StartTime,TRUE);
          MOReportEvent(O,R->Name,mxoRsv,mttEnd,R->EndTime,TRUE);
          }  /* END BLOCK */

          break;

        case mxoJob:
 
          /* report events */

          /* NYI */

          break;

        case mxoNode:

          {
          mnode_t *N;

          if (O == NULL)
            break;

          N = (mnode_t *)O;

          if (N != &MSched.DefaultN)
            MOReportEvent(O,N->Name,mxoNode,mttCreate,N->CTime,TRUE);
          }  /* END BLOCK */

          break;

        case mxoRM:

          {
          mrm_t *RM;

          if (O == NULL)
            break;

          RM = (mrm_t *)O;

          /* report start events */

          MOReportEvent(O,RM->Name,mxoRM,mttStart,RM->CTime,TRUE);
          }  /* END case mxoRM */

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (T->OType) */

      sprintf(tmpLine,"trigger %s successfully created\n",
        T->TName);

      MUSNCat(&BPtr,&BSpace,tmpLine);
      }  /* END for (tindex) */

    snprintf(EMsg,MMAX_BUFFER,"%s",
      tmpBuf);

    MUArrayListFree(&OTList);

    return(SUCCESS);
    }  /* END else if (strstr(OString,MXO[mxoTrig]) != NULL) */
  else if (strstr(OString,"vpc") != NULL)
    {
    /* create new vpc */

    if (!MOLDISSET(MSched.LicensedFeatures,mlfVPC))
      {
      MMBAdd(
          &MSched.MB,
          "ERROR:   license does not allow VPC creation, please contact Adaptive Computing\n",
          NULL,
          mmbtOther,
          MSched.Time + MCONST_DAYLEN,
          10,
          NULL);

      sprintf(EMsg,"ERROR:   license does not allow VPC creation, please contact Adaptive Computing\n");

      return(FAILURE);
      }

    if (!bmisset(&MSched.Flags,mschedfHosting) &&
        (MUICheckAuthorization(
           U,
           P,
           NULL,
           mxoPar,
           mcsMSchedCtl,
           msctlCreate,
           &IsAdmin,
           NULL,
           0) == FAILURE))
      {
      snprintf(EMsg,MMAX_BUFFER,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }
 
    return(MUIVPCCreate(RE,Auth,&Flags,EMsg));
    }  /* END else if (strstr(OString,"vpc") != NULL) */
  else if (strstr(OString,"gevent") != NULL)
    {
    /* Create a gevent */

    return(MUIGEventCreate(U,IsAdmin,RE,EMsg));
    }
  else
    {
    sprintf(EMsg,"ERROR:  cannot create requested object type");

    return(FAILURE);
    }

  return(SUCCESS);
  }   /* END MUISchedCtlCreate() */





/**
 * Handle incoming events from Moab peers.
 *
 * @param PR (I)
 * @param Auth (I) [optional,not used]
 * @param EventType (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE)
 */

int MUISchedCtlProcessEvent(

  mrm_t  *PR,    /* I (required) */
  char   *Auth,  /* I (optional,not used) */
  enum MPeerEventTypeEnum EventType,  /* I (optional) */
  char   *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  const char *FName = "MUISchedCtlProcessEvent";

  MDB(3,fUI) MLog("%s(%s,%s,%s,EMsg)\n",
    FName,
    (PR != NULL) ? PR->Name : "NULL",
    (Auth != NULL) ? Auth : "NULL",
    MPeerEventType[EventType]);

  if (PR == NULL)
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  switch (EventType)
    {
    case mpetClassChange:

      /* have RM re-initialize itself */

      MRMSetState(PR,mrmsConfigured);

      break;

    default:

      /* unsupported event type */

      MDB(3,fSCHED) MLog("WARNING:  unsupported event '%s' from RM '%s'\n",
        MPeerEventType[EventType],
        PR->Name);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"unsupported event '%s' from RM '%s'",
          MPeerEventType[EventType],
          PR->Name);
        }

      return(FAILURE);

      break;
    }

  return(SUCCESS);
  }  /* END MUISchedCtlProcessEvent() */





/**
 * Internal routine to modify scheduler object.
 *
 * @see MUISchedCtl() - parent
 * @see MVPCModify() - child - modify VPC attribute
 *
 * @param Auth (I)
 * @param RE (I)
 * @param OID (I)
 * @param OType (I)
 * @param Msg (O) [minsize=MMAX_LINE]
 */

int MUISchedCtlModify(

  char                 *Auth,  /* I */
  mxml_t               *RE,    /* I */
  char                 *OID,   /* I */
  enum MXMLOTypeEnum    OType, /* I */
  char                 *Msg)   /* O (minsize=MMAX_LINE) */

  {
  char  OString[MMAX_LINE];

  char   tmpLine[MMAX_LINE];

  mbool_t IsAdmin      = FALSE;

  enum MObjectSetModeEnum Op = mSet;

  mpsi_t   *P;
  mgcred_t *U;

  const char *FName = "MUISchedCtlModify";

  MDB(3,fUI) MLog("%s(%s,RE,OID,OType,Msg)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if ((Msg == NULL) || (Auth == NULL))
    {
    return(FAILURE);
    }

  Msg[0] = '\0';

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

  MXMLGetAttr(RE,MSAN[msanOption],NULL,OString,sizeof(OString));

  if (MXMLGetAttr(RE,"op",NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
    {
    Op = (enum MObjectSetModeEnum)MUGetIndexCI(tmpLine,MObjOpType,FALSE,Op);
    }

  if (strstr(OString,"message") != NULL)
    {
    char  ArgString[MMAX_BUFFER];

    if (MUICheckAuthorization(
          U,
          P,
          NULL,
          mxoSched,
          mcsMSchedCtl,
          msctlModify,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      snprintf(Msg,MMAX_LINE,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }

    MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));

    switch (Op)
      {
      case mClear:

        MMBPurge(
          &MSched.MB,
          NULL,
          (int)strtol(ArgString,NULL,0),
          0);

        snprintf(Msg,MMAX_LINE,"INFO:  message buffer cleared to priority level %d\n",
          (int)strtol(ArgString,NULL,0));

        break;

      case mSet:

        MMBAdd(
          &MSched.MB,
          ArgString,
          (Auth != NULL) ? Auth : (char *)"N/A",
          mmbtOther,
          MSched.Time + MCONST_DAYLEN,
          10,
          NULL);

        sprintf(Msg,"INFO:  message added\n");

        break;

      case mUnset:

        MMBPurge(
          &MSched.MB,
          ArgString,
          -1,
          0);

        sprintf(Msg,"INFO:  message cleared\n");

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (Op) */

    return(SUCCESS);
    }  /* END if (strstr(OString,"message") != NULL) */
  else if (strstr(OString,MXO[mxoTrig]) != NULL)
    {
    mxml_t *WE;

    mtrig_t *T;

    mbool_t AdjustETime = FALSE;

    enum MTrigAttrEnum AIndex;

    int CTok;

    char tmpName[MMAX_LINE];
    char tmpVal[MMAX_LINE];

    if (MUICheckAuthorization(
          U,
          P,
          NULL,
          mxoTrig,
          mcsMSchedCtl,
          msctlModify,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      snprintf(Msg,MMAX_LINE,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }

    /* FORMAT <where name=<trigID></where><set name=<TrigAttr> value=<value></set>[<set>] */

    CTok = -1;

    if (MS3GetWhere(
          RE,
          &WE,
          &CTok,
          tmpName,         /* NOTE:  ignore name, use only value */
          sizeof(tmpName),
          tmpName,
          sizeof(tmpName)) == FAILURE)
      {
      return(FAILURE);
      }

    /* NOTE: do not check 'Where' name, assume it is set to 'Name" */

    if (MTrigFind(tmpName,&T) == FAILURE)
      {
      return(FAILURE);
      }

    CTok = -1;

    while (MS3GetSet(
        RE,
        NULL,
        &CTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
      {
      AIndex = (enum MTrigAttrEnum)MUGetIndexCI(tmpName,MTrigAttr,FALSE,mtaNONE);

      /* only certain attributes can be modified */

      switch (AIndex)
        {
        case mtaState:

          if (!bmisset(&T->SFlags,mtfAsynchronous))
            {
            return(FAILURE);
            }
          else
            {
            MTrigSetAttr(T,AIndex,(void *)tmpVal,mdfString,mSet);

            if ((T->State == mtsSuccessful) || (T->State == mtsFailure))
              bmset(&T->InternalFlags,mtifIsComplete);
            }
     
          break;

        default:

          if (MTrigSetAttr(T,AIndex,(void *)tmpVal,mdfString,mSet) == FAILURE)
            {
            return(FAILURE);
            }

          break;
        }

      if ((AIndex == mtaOffset) || (AIndex == mtaEventType))
        {
        AdjustETime = TRUE;
        }
      }

    MTrigInitialize(T);

    if ((AdjustETime == TRUE) && (T->O != NULL))
      {
      /* adjusted trigger event or trigger offset, re-evaluate etime */

      switch (T->OType)
        {
        case mxoRsv:

          {
          mrsv_t *R;

          R = (mrsv_t *)T->O;

          MOReportEvent(T->O,T->OID,T->OType,mttCreate,R->CTime,TRUE);
          MOReportEvent(T->O,T->OID,T->OType,mttStart,R->StartTime,TRUE);
          MOReportEvent(T->O,T->OID,T->OType,mttEnd,R->EndTime,TRUE);
          }  /* END (case mxoRsv) */

          break;

        default:

          /* NOT SUPPORTED */

          break;
        }  /* END switch (T->OType) */
      }    /* END if ((AdjustETime == TRUE) && (T->O != NULL)) */

    sprintf(Msg,"Success");

    return(SUCCESS);
    }  /* END if (strstr(OString,MXO[mxoTrig]) != NULL) */
  else if (strstr(OString,"vpc") != NULL)
    {
    int rc;

    mstring_t String(MMAX_LINE);

    /* process 'mschedctl -m vpc:<VPCID>' */

    rc = MUIVPCModify(Auth,RE,OID,OType,&String);

    MUStrCpy(Msg,String.c_str(),MMAX_LINE);

    return(rc);
    }  /* END else (strstr(OString,"vpc") != NULL) */
  else if (strstr(OString,"gevent") != NULL)
    {
    if (MUICheckAuthorization(
          U,
          P,
          NULL,
          mxoSched,
          mcsMSchedCtl,
          msctlModify,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      snprintf(Msg,MMAX_LINE,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }

    if (Op == mDecr)
      {
      mgevent_obj_t  *GEvent;

      if (MGEventGetItem(OID,&MSched.GEventsList,&GEvent) == SUCCESS)
        {
        MGEventRemoveItem(&MSched.GEventsList,OID);
        MUFree((char **)&GEvent->GEventMsg);
        MUFree((char **)&GEvent->Name);
        MUFree((char **)&GEvent);
        }

      return(SUCCESS);
      }
    } /* END else if (strstr(OString,"gevent") != NULL) */
  else if (strstr(OString,"sched") != NULL)
    {
    /* NOTE:  for now, assume all 'sched' modifies are modifying a 
              vmoslist - FIXME when a second attr can be modified this way! */

    char ArgString[MMAX_BUFFER];

    if (MUICheckAuthorization(
          U,
          P,
          NULL,
          mxoSched,
          mcsMSchedCtl,
          msctlModify,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      snprintf(Msg,MMAX_LINE,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);

      return(FAILURE);
      }

    MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));

    /* FORMAT:  sched vmoslist:<OSID>[+=|=]<vmos1>[,<vmos2>][ <ARG>]... */

    {
    char *vptr;
    char *VTokPtr;

    char *ptr = NULL;
    char *TokPtr;

    char *osptr;
    char *Attr;

    int   OSIndex;

    /* ignore 'sched' term and load first arg */

    if ((MUStrTok(ArgString," \t\n",&VTokPtr) == NULL) ||
        ((Attr = MUStrTok(NULL,":",&VTokPtr)) == NULL))
      {
      snprintf(Msg,MMAX_LINE,"ERROR:    bad format\n");

      return(FAILURE);
      }

    if (!strcasecmp(Attr,"vmoslist"))
      {
      vptr = VTokPtr;

      while (vptr != NULL)
        {
        osptr = vptr + 1;

        /* locate operator after vmoslist */

        if (MUStrTok(ptr,"+=",&TokPtr) == NULL)
          {
          snprintf(Msg,MMAX_LINE,"ERROR:    bad format\n");

          return(FAILURE);
          }

        if (TokPtr[0] == '+')
          {
          Op = mIncr;
          }
        else if (TokPtr[0] == '-')
          {
          Op = mDecr;
          }
        else {
          Op = mSet;
          }

        OSIndex = MUMAGetIndex(meOpsys,osptr,mAdd);

        if (OSIndex <= 0)
          {
          snprintf(Msg,MMAX_LINE,"ERROR:    cannot add OS '%s'\n",
            osptr);

          return(FAILURE);
          }

        if (Op == mSet)
          MUHTClear(&MSched.VMOSList[OSIndex],FALSE,NULL);

        ptr = MUStrTok(NULL,",",&TokPtr);

        /* NOTE:  is this modify authoritative, ie, clobber all info from other sources? */

        while (ptr != NULL)
          {
          MUHTAdd(&MSched.VMOSList[OSIndex],strdup(ptr),NULL,NULL,MUFREE);

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */

        vptr = MUStrTok(NULL," \t\n",&VTokPtr);
        }    /* END while (vptr != NULL) */

      sprintf(Msg,"INFO:  vmoslist modified\n");
      }
    else if (!strcasecmp(Attr,"featuregres"))
      {
      int sindex;

      ptr = MUStrTok(NULL,"=",&VTokPtr);

      if (MUStrIsEmpty(ptr))
        {
        sprintf(Msg,"ERROR:  invalid syntax (please specify \"featuregres:NAME=[on|off]\")\n");
        }

      sindex = MUGetNodeFeature(ptr,mVerify,NULL);

      if (sindex <= 0)
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  invalid gres specified (%s)\n",
          ptr);

        return(FAILURE);
        }

      if (MUStrIsEmpty(VTokPtr))
        {
        sprintf(Msg,"ERROR:  invalid syntax (please specify \"featuregres:NAME=[on|off]\")\n");

        return(FAILURE);
        }

      if (!strcasecmp(VTokPtr,"on"))
        {
        bmset(&MSched.FeatureGRes,sindex);

        snprintf(Msg,MMAX_LINE,"INFO: FeatureGRes \'%s\' turned on\n",
          ptr);
        }
      else if (!strcasecmp(VTokPtr,"off"))
        {
        bmunset(&MSched.FeatureGRes,sindex);

        snprintf(Msg,MMAX_LINE,"INFO: FeatureGRes \'%s\' turned off\n",
          ptr);
        }
      else
        {
        sprintf(Msg,"ERROR:  invalid syntax (please specify \"featuregres:NAME=[on|off]\")\n");

        return(FAILURE);
        }
      }
    }      /* END BLOCK */

    return(SUCCESS);
    }  /* END else if (strstr(OString,"sched") != NULL) */

  /* check for modify trigger */

  if (MSched.TList != NULL)
    {
    MOReportEvent((void *)&MSched,MSched.Name,mxoSched,mttModify,MSched.Time,TRUE);
    }

  return(SUCCESS);
  }   /* END int MUISchedCtlModify() */





/**
 * Modify scheduler settings using moab.cfg syntax.
 *
 * @see MUISchedCtl() - parent
 *
 * @param Auth     (I)
 * @param RE       (I)
 * @param OID      (I)
 * @param OType    (I)
 * @param Response (O) [MUST be minsize=MMAX_BUFFER << 2]
 */

int MUISchedConfigModify(

  char                 *Auth,
  mxml_t               *RE,
  char                 *OID,
  enum MXMLOTypeEnum    OType,
  mstring_t            *Response)

  {
  char  FlagString[MMAX_LINE];
  int   STok;

  char  tmpName[MMAX_NAME];
  char  tmpVal[MMAX_LINE];

  char  Annotation[MMAX_LINE];

  char *ptr;
  char *TokPtr;

  char *ArgString = NULL;
  char *ConfigBuf = NULL;

  char  tmpLine[MMAX_LINE];

  mbool_t IsAdmin      = FALSE;
  mbool_t EvalOnly     = FALSE;
  mbool_t IsPersistent = FALSE;

  enum MObjectSetModeEnum Op = mSet;

  mpsi_t   *P;
  mgcred_t *U;

  int       LineNum;

  const char *FName = "MUISchedConfigModify";

  MDB(3,fUI) MLog("%s(%s,RE,OID,OType,Response)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (Response == NULL)
    {
    return(FAILURE);
    }

  if ((Auth != NULL) &&
      (!strncasecmp(Auth,"peer:",strlen("peer:"))))
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

  if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0) == SUCCESS)
    {
    if (strcasestr(FlagString,"pers"))
      IsPersistent = TRUE;

    if (strcasestr(FlagString,"eval"))
      EvalOnly = TRUE;
    }  /* END if (MXMLGetAttr(RE,MSAN[msanFlags],...) == SUCCESS) */

  if (MXMLGetAttr(RE,"op",NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
    {
    Op = (enum MObjectSetModeEnum)MUGetIndexCI(tmpLine,MObjOpType,FALSE,Op);
    }

  Annotation[0] = '\0';

  STok = -1;

  while (MS3GetSet(
      RE,
      NULL,
      &STok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,"annotation"))
      MUStrCpy(Annotation,tmpVal,sizeof(Annotation));
    }

  /* config is default */

  if (MUICheckAuthorization(
        U,
        P,
        NULL,
        mxoSched,
        mcsMSchedCtl,
        msctlModify,
        &IsAdmin,
        NULL,
        0) == FAILURE)
    {
    MStringAppendF(Response,"ERROR:    user '%s' is not authorized to run command\n",
      Auth);
 
    return(FAILURE);
    }
 
  /* change scheduler parameter */
 
  MXMLDupAttr(RE,MSAN[msanArgs],NULL,&ArgString);

  if (MUStrIsEmpty(ArgString))
    {
    return(FAILURE);
    }

  ConfigBuf = (char *)MUMalloc(strlen(ArgString) + 1);
 
  MUStringUnpack(ArgString,ConfigBuf,strlen(ArgString) + 1,NULL);
 
  ptr = ConfigBuf;
 
  MCfgAdjustBuffer(&ptr,FALSE,NULL,FALSE,TRUE,FALSE);
 
  ptr = MUStrTok(ptr,"\n",&TokPtr);
 
  LineNum = 1;
 
  while (ptr != NULL)
    {
    if (MCfgLineIsEmpty(ptr) == TRUE)
      {
      /* ignore line */
 
      /* NO-OP */
      }
    else if (MSysReconfigure(
               ptr,              /* I */
               IsPersistent,     /* I */
               EvalOnly,         /* I */
               TRUE,             /* I - modify master */
               LineNum,          /* I */
               tmpLine) == SUCCESS)  /* O */
      {
      MDB(2,fUI) MLog("INFO:     config line '%.100s' successfully processed\n",
        ptr);
 
      if (Annotation[0] != '\0')
        {
        char Msg[MMAX_LINE];
 
        MUStringChop(ptr);
 
        snprintf(Msg,sizeof(Msg),"config user=%s  change='%.256s'  message='%.256s'",
          Auth,
          ptr,
          Annotation);
 
        MSysRecordEvent(
          mxoSched,
          MSched.Name,
          mrelSchedModify,
          Auth,
          Msg,
          NULL);
        }  /* END if (Annotation[0] != '\0') */
 
      if (tmpLine[0] != '\0')
        {
        MStringAppendF(Response,"INFO:  %s\n",
          tmpLine);
        }
      else
        {
        /* ignore empty lines */
 
        /* NYI */
 
        MStringAppendF(Response,"INFO:  config line #%d '%s' successfully processed\n",
          LineNum,
          ptr);
        }
      }
    else
      {
      MUStringChop(tmpLine);
 
      MStringAppendF(Response,"ALERT: cannot process parameter line #%d '%s' (%s)\n",
        LineNum,
        ptr,
        tmpLine);
      }
 
    LineNum++;
 
    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (ptr != NULL) */
 
  if (EvalOnly == FALSE)
    {
    MSched.EnvChanged = TRUE;
    }
 
  MDB(2,fUI) MLog("INFO:     config line '%s' successfully processed\n",
    ArgString);
 
  MUFree(&ArgString);
  MUFree(&ConfigBuf);

  /* check for modify trigger */

  if (MSched.TList != NULL)
    {
    MOReportEvent((void *)&MSched,MSched.Name,mxoSched,mttModify,MSched.Time,TRUE);
    }

  return(SUCCESS);
  }  /* END MUISchedConfigModify() */




int MUISchedCtlListTID(

  msocket_t            *S,         /* I */
  mgcred_t             *U,         /* I */
  char                 *ArgString, /* I */
  enum MFormatModeEnum  DFormat,   /* I */
  mxml_t               *DE,        /* O */
  char                 *EMsg)      /* O (minsize=MMAX_LINE) */

  {
  /* process 'mschedctl -l trans' */
 
  /* NOTE: everyone can view every transaction */
  /* TODO: enable transaction ownership and transaction object */
  /*       for more finegrained security features */

  /*
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
  */

  switch (DFormat)
    {
    case mfmXML:
    case mfmHuman:
    default:

      {
      char *BPtr;
      int   BSpace;

      char tmpBuf[MMAX_BUFFER];
      int  tindex;

      mtrans_t *Trans;

      int   STID;
      int   ETID;

      MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

      if (ArgString[0] != '\0')
        {
        char *ptr;
        char *TokPtr;
        
        /* FORMAT:  <STID>[,<ETID>] */

        if ((ptr = MUStrTok(ArgString," -,:\t\n",&TokPtr)) != NULL)
          {
          STID = (int)strtol(ptr,NULL,10);

          STID = MAX(1,STID);
          }
        else
          {
          /* malformed arg */

          if (DFormat != mfmXML)
            MUISAddData(S,"invalid TID specified");

          return(FAILURE);
          }

        if ((ptr = MUStrTok(NULL," -,:\t\n",&TokPtr)) != NULL)
          {
          ETID = (int)strtol(ptr,NULL,10) + 1;
          }
        else
          {
          ETID = STID + 1;
          }
        }
      else
        {
        STID = 1;
        ETID = MMAX_TRANS;
        }

      for (tindex = STID;tindex < ETID;tindex++)
        {
        Trans = NULL;

        if (MTransFind(
            tindex,
            U->Name,
            &Trans) == FAILURE)
          {
          continue;
          }

        if (DFormat == mfmXML)
          {
          mxml_t *TE = NULL;

          MTransToXML(Trans,&TE);

          MXMLAddE(DE,TE);
          }  /* END if (DFormat == mfmXML) */
        else
          {
          int aindex;

          MUSNPrintF(&BPtr,&BSpace,"TID[%s]",
            Trans->Val[mtransaName]);

          for (aindex = 0;aindex < mtransaLAST;aindex++)
            {
            if (Trans->Val[aindex] == NULL)
              continue;

            MUSNPrintF(&BPtr,&BSpace,"  A%d='%s'",
              aindex + 1,
              Trans->Val[aindex]);
            }

          MUSNPrintF(&BPtr,&BSpace,"\n");
          }
        }    /* END for (tindex) */

      if (DFormat != mfmXML)
        MUISAddData(S,tmpBuf);
      }  /* END BLOCK (case mfmXML) */

      break;
    }    /* END switch (DFormat) */

  return(SUCCESS);
  }  /* END MUISchedCtlListTID() */
/* END MUISchedCtlSupport.c */
