/* HEADER */

      
/**
 * @file MSysProcessRequest.c
 *
 * Contains: MSysProcessRequest function
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Process socket client request.
 *
 * @param S (I)
 * @param RequestIsPreLoaded (I)
 *
 * NOTE: Not thread-safe!
 *
 * @see MSysEnqueueSocket() - parent - caller for non-blocking requests only
 * @see MUIAcceptRequests() - parent - caller for blocking requests only
 * @see MUISProcessRequest() - child
 */

int MSysProcessRequest(

  msocket_t *S,                   /* I */
  mbool_t    RequestIsPreLoaded)  /* I */

  {
  char      Message[MMAX_LINE];
  int       HeadSize;

  int       sindex;  /* enum MSvcEnum */

  mbitmap_t AuthBM;

  char      SName[MMAX_LINE];
  char      Auth[MMAX_NAME];
  char      Passwd[MMAX_NAME];

  char      tmpLine[MMAX_LINE];

  int       SC;
  int       Align;

  char     *ptr;
  char     *ptr2;

  char     *args;
  char     *TokPtr;

  int       rc;

  const char *FName = "MSysProcessRequest";

  MDB(3,fUI) MLog("%s(S,%s)\n",
    FName,
    MBool[RequestIsPreLoaded]);

  if (S == NULL)
    {
    return(FAILURE);
    }

  if ((MGlobalReqBuf == NULL) && 
      (MSUAllocSBuf(
         &MGlobalReqBuf,
         MSched.M[mxoJob],
         MPar[0].ConfigNodes,
         &MGlobalReqBufSize,
         FALSE) == FAILURE))
    {
    MDB(1,fUI) MLog("ERROR:    cannot allocate request buffer\n");

    return(FAILURE);
    }
  else if (MIncrGlobalReqBuf == TRUE) 
    {
    if (MSUAllocSBuf(
         &MGlobalReqBuf,
         0,
         0,
         &MGlobalReqBufSize,
         TRUE) == FAILURE)
      {
      MDB(1,fUI) MLog("ERROR:    cannot allocate increase request buffer\n");

      return(FAILURE);
      }

    MIncrGlobalReqBuf = FALSE;
    }

  if (RequestIsPreLoaded == FALSE)
    {
    char EMsg[MMAX_LINE];

    enum MStatusCodeEnum SC;

    if (MSURecvData(S,MSched.SocketWaitTime,TRUE,&SC,EMsg) == FAILURE)
      {
      MDB(3,fSOCK) MLog("ALERT:    cannot read client packet\n");

      MUStrDup(&MSched.UIBadRequestMsg,EMsg);

      if (S->RemoteHost != NULL)
        MUStrDup(&MSched.UIBadRequestor,S->RemoteHost);
      else
        MUStrDup(&MSched.UIBadRequestor,"???");

      if (MSched.DefaultDropBadRequest == FALSE)
        {
        MUStrDup(&S->SMsg,EMsg);

        if (SC == mscNoAuth)
          S->StatusCode = (long)msfESecServerAuth;
        else
          S->StatusCode = (long)msfEGServer;

        MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);
        }

      MSUDisconnect(S);

      return(FAILURE);
      }  /* END if (MSURecvData(S,MMAX_SOCKETWAIT,TRUE,&SC,EMsg) == FAILURE) */
    }    /* END if (RequestIsPreLoaded == FALSE) */

  S->SBuffer  = MGlobalReqBuf;
  S->SBufSize = MGlobalReqBufSize;

  if (S->ProcessTime == 0)
    MUGetMS(NULL,(long *)&S->ProcessTime);

  switch (S->WireProtocol)
    {
    case mwpXML:
    case mwpS32:

      {
      int SC;

      /* new style request - process and return */
 
      rc = MUISProcessRequest(S,Message,&SC);

      if (rc == FAILURE)
        {
        if (SC != mscNoEnt)
          MUISAddData(S,"ERROR:  command not supported");

        MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);
        }
      
      return(rc);
      }  /* END BLOCK */

      /*NOTREACHED*/

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (S->WireProtocol) */

  /* old style request - process below */

  MGlobalReqBuf[0] = '\0';

  S->SBuffer  = MGlobalReqBuf;
  S->SBufSize = MGlobalReqBufSize;

  MUStrCpy(CurrentHostName,S->Host,MMAX_NAME);  /* NOTE:  not threadsafe */

  /* locate/process args */

  if ((args = strstr(S->RBuffer,MCKeyword[mckArgs])) == NULL)
    {
    MDB(3,fSOCK) MLog("ALERT:    cannot locate command arg\n");

    sprintf(S->SBuffer,"%s%d %s%s\n",
      MCKeyword[mckStatusCode],
      scFAILURE,
      MCKeyword[mckArgs],
      "ERROR:    cannot locate command args");

    S->SBufSize = strlen(S->SBuffer);

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    return(FAILURE);
    }

  *args = '\0';

  args += strlen(MCKeyword[mckArgs]);

  /* get service */

  SName[0] = '\0';

  if ((ptr = strstr(S->RBuffer,MCKeyword[mckCommand])) == NULL)
    {
    MDB(3,fSOCK) MLog("ALERT:    cannot locate command\n");

    sprintf(S->SBuffer,"%s%d %s%s\n",
      MCKeyword[mckStatusCode],
      scFAILURE,
      MCKeyword[mckArgs],
      "ERROR:    cannot locate command");

    S->SBufSize = strlen(S->SBuffer);

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    return(FAILURE);
    }

  ptr += strlen(MCKeyword[mckCommand]);

  MUStrCpy(SName,ptr,sizeof(SName));

  for (ptr2 = &SName[0];*ptr2 != '\0';ptr2++)
    {
    if (isspace(*ptr2))
      {
      *ptr2 = '\0';

      break;
      }
    }  /* END for (ptr2) */

  /* get authentication */

  if ((ptr = strstr(S->RBuffer,MCKeyword[mckAuth])) == NULL)
    {
    MDB(3,fSOCK) MLog("ALERT:    cannot locate authentication\n");

    sprintf(S->SBuffer,"%s%d %s%s\n",
      MCKeyword[mckStatusCode],
      scFAILURE,
      MCKeyword[mckArgs],
      "ERROR:    cannot locate authentication");

    S->SBufSize = (long)strlen(S->SBuffer);

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    return(FAILURE);
    }

  ptr += strlen(MCKeyword[mckAuth]);

  MUStrCpy(Auth,ptr,sizeof(Auth));

  /* FORMAT:  <USERNAME>[:<PASSWORD>] */

  for (ptr2 = &Auth[0];*ptr2 != '\0';ptr2++)
    {
    if (isspace(*ptr2))
      {
      *ptr2 = '\0';

      break;
      }
    }    /* END for (ptr2) */

  if ((ptr2 = MUStrTok(Auth,":",&TokPtr)) != NULL)
    {
    MUStrCpy(Passwd,ptr2,sizeof(Passwd));
    }
  else
    {
    Passwd[0] = '\0';
    }

  /* determine service */

  for (sindex = 1;MUI[sindex].SName != NULL;sindex++)
    {
    if (!strcmp(SName,MUI[sindex].SName))
      break;
    }  /* END for (sindex) */

  if ((MUI[sindex].SName == NULL) || (MUI[sindex].Func == NULL))
    {
    MDB(3,fUI) MLog("ALERT:    cannot support service '%s'\n",
      SName);

    sprintf(Message,"ERROR:    cannot support service '%s' (%s)",
      SName,
      (MUI[sindex].SName == NULL) ? "service" : "func");

    sprintf(S->SBuffer,"%s%d %s%s\n",
      MCKeyword[mckStatusCode],
      scFAILURE,
      MCKeyword[mckArgs],
      Message);

    S->SBufSize = (long)strlen(S->SBuffer);

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    return(FAILURE);
    }  /* END if (MUI[sindex].SName == NULL) */

  if (S->SIndex == mcsNONE)
    S->SIndex = (enum MSvcEnum)sindex;

  MDB(3,fUI) MLog("INFO:     client '%s' read (%ld bytes) initiating service call for '%s' (Auth: %s)\n",
    S->Name,
    S->SBufSize,
    MUI[sindex].SName,
    Auth);

  /* fail if name is not recognized */

  if (Auth[0] == '\0')
    {
    MDB(2,fUI) MLog("WARNING:  client id '%s' is unknown\n",
      Auth);

    sprintf(S->SBuffer,"%s%d %s%s\n",
      MCKeyword[mckStatusCode],
      scFAILURE,
      MCKeyword[mckArgs],
      "ERROR:    cannot authenticate client");

    S->SBufSize = (long)strlen(S->SBuffer);

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    return(FAILURE);
    }

  MSysGetAuth(Auth,(enum MSvcEnum)sindex,0,&AuthBM);

  if (!bmisset(&AuthBM,mcalOwner) && !bmisset(&AuthBM,mcalGranted))
    {
    sprintf(Message,"ERROR:    user '%s' is not authorized to run command '%s'\n",
      Auth,
      MUI[sindex].SName);

    sprintf(S->SBuffer,"%s%d %s%s\n",
      MCKeyword[mckStatusCode],
      scFAILURE,
      MCKeyword[mckArgs],
      Message);

    S->SBufSize = (long)strlen(S->SBuffer);

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    return(FAILURE);
    }

  S->SBufSize = (long)MGlobalReqBufSize;

  sprintf(tmpLine,"%s%d ",
    MCKeyword[mckStatusCode],
    scFAILURE);

  Align = (int)strlen(tmpLine) + (int)strlen(MCKeyword[mckArgs]);

  sprintf(S->SBuffer,"%s%*s%s",
    tmpLine,
    16 - (Align % 16),
    " ",
    MCKeyword[mckArgs]);

  HeadSize = (int)strlen(S->SBuffer);

  S->SBufSize -= HeadSize;

  SC = (*MUI[sindex].Func)(
    args,
    S->SBuffer + HeadSize,
    &AuthBM,
    Auth,
    &S->SBufSize);

  ptr = S->SBuffer + strlen(MCKeyword[mckStatusCode]);

  *ptr = SC + '0';

  if (S->SBufSize != MGlobalReqBufSize)
    S->SBufSize += (long)HeadSize;
  else
    S->SBufSize = (long)strlen(S->SBuffer);

  MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

  fflush(mlog.logfp);

  /* mrelAllSchedCommand --- Record all commands.
   * mrelSchedCommand --- Record only commands that perform an action. */

  if (bmisset(&MSched.RecordEventList,mrelAllSchedCommand))
    {
    MSysRecordCommandEvent(S,(enum MSvcEnum)sindex,SC,Auth);
    }
  else if (bmisset(&MSched.RecordEventList,mrelSchedCommand))
    {
    switch (sindex)
      {
          /* info only commands, do not record event under only mrelSchedCommand */
      case mcsShowQueue:
      case mcsShowState:
      case mcsStatShow:
      case mcsCheckJob:
      case mcsRsvShow:
      case mcsCheckNode:
      case mcsShowResAvail:
      case mcsShowEstimatedStartTime:
      case mcsShowConfig:
      case mcsMShow:
      case mcsMDiagnose:
        break;

          /* info/action commands, do not ALWAYS record event here.  
           * Instrument these functions intenally to determine whether to record
           * command events */
      case mcsMJobCtl:
      case mcsMRsvCtl:
      case mcsMSchedCtl:
      case mcsMVCCtl:
      case mcsMVMCtl:

        if (bmisset(&S->Flags,msftReadOnlyCommand))
          {
          break;
          }
        /* else fall though... */

      default:
        MSysRecordCommandEvent(S,(enum MSvcEnum)sindex,SC,Auth);

        break;
      }  /* END switch (sindex) */
    }    /* END if (bmisset(&MSched.RecordEventList,mrelSchedCommand)) */

  return(SUCCESS);
  }  /* END MSysProcessRequest() */
/* END MSysProcessRequest.c */
