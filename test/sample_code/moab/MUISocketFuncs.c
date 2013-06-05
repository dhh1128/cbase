/* HEADER */

      
/**
 * @file MUISocketFuncs.c
 *
 * Contains: Misc MUI Socket processing functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Returns the MacAddress of the first network interface card on the system
 *
 * NOTE: only works on linux systems
 * NOTE: this code was contributed by Joshua Bernstein <jbernstein@penguincomputing.com> Thanks Josh!!
 *
 * @param MacAddress (O) minsize=MMAX_LINE
 * 
 */

int MGetMacAddress(

  char *MacAddress)

  {
#if defined(__LINUX)
  struct ifreq  ifr;
  struct ifreq *IFR;

  struct ifconf ifc;

  char buf[1024];

  int Socket;
  int index;

  int Status = FAILURE;

  u_char Address[6];

  if (MacAddress == NULL)
    {
    return(FAILURE);
    }

  MacAddress[0] = '\0';

  Socket = socket(AF_INET, SOCK_DGRAM, 0);

  if (Socket == -1)
    {
    return(FAILURE);
    }

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;

  ioctl(Socket,SIOCGIFCONF,&ifc);

  IFR = ifc.ifc_req;

  for (index = ifc.ifc_len / sizeof(struct ifreq); --index >= 0; IFR++)
    {
    strcpy(ifr.ifr_name, IFR->ifr_name);

    if (ioctl(Socket,SIOCGIFFLAGS,&ifr) == 0)
      {
      if (!(ifr.ifr_flags & IFF_LOOPBACK))
        {
        if (ioctl(Socket,SIOCGIFHWADDR,&ifr) == 0)
          {
          Status = SUCCESS;

          break;
          }
        }
      }
    }   /* END for (index) */

  close(Socket);

  if (Status == FAILURE)
    {
    return(FAILURE);
    }

  bcopy(ifr.ifr_hwaddr.sa_data,Address,6);

  snprintf(MacAddress,MMAX_LINE,"%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",Address[0],Address[1],Address[2],Address[3],Address[4],Address[5]);

  MUStrToUpper(MacAddress,MacAddress,strlen(MacAddress) + 1);

  return(SUCCESS);
#else
  if (MacAddress != NULL)
    MacAddress[0] = '\0';
  return(FAILURE);
#endif
  }  /* END MGetMacAddress() */

/**
 * Set service success/failure status w/in socket
 *
 * @param S (I) [modified]
 * @param CIndex (I) 
 * @param SC (I)
 * @param SMsg (I) [optional]
 *
 * @see MS3SetStatus() - peer
 * @see MUISProcessRequest() - parent
 */

int MUISSetStatus(

  msocket_t *S,      /* I (modified) */
  int        CIndex, /* I */
  int        SC,     /* I */
  char      *SMsg)   /* I (optional) */

  {
  char *ptr;

  switch (S->WireProtocol)
    {
    case mwpXML:

      if (S->SE == NULL)
        /* Passing a 'const char *', so cast to just a 'char *' */
        MXMLCreateE(&S->SE,(char *) MSON[msonData]);

      /* NOTE:  should use S3 format, ie MS3SetStatus(S->SE,NULL,NULL,SC,SMsg); - NYI */

      {
      char tmpLine[MMAX_LINE];

      /* FORMAT:  <Data code="INT" status="STRING">...</Data> */

      sprintf(tmpLine,"%d",
        SC);

      MXMLSetAttr(S->SE,"code",tmpLine,mdfString);

      if ((SMsg != NULL) && (SMsg[0] != '\0'))
        {
        MXMLSetAttr(S->SE,"status",SMsg,mdfString);
        }
      }  /* END BLOCK */

      break;

    case mwpS32:

      {
      if (SMsg != NULL)
        MUStrDup(&S->SMsg,SMsg);

      S->StatusCode = SC;
      }  /* END BLOCK */

      break;

    default:

      ptr = S->SBuffer + strlen(MCKeyword[mckStatusCode]);

      *ptr = SC + '0';

      break;
    }  /* END switch (S->WireProtocol) */

  return(SUCCESS);
  }  /* END MUISSetStatus() */





/**
 *
 *
 * @param S (I) [modified]
 */

int MUISClear(

  msocket_t *S)  /* I (modified) */

  {
  int Align;

  if (S == NULL)
    {
    return(FAILURE);
    }

  /* clear data and status */

  switch (S->WireProtocol)
    {
    case mwpXML:

      if (S->SE != NULL)
        MXMLDestroyE(&S->SE);

      break;

    case mwpS32:

      if (S->SE != NULL)
        MXMLDestroyE(&S->SE);

      MUFree(&S->SMsg);

      S->StatusCode = 0;

      break;

    default:

      if (S->SBuffer != NULL)
        {
        S->SBuffer[0] = '\0';

        sprintf(S->SBuffer,"%s%d ",
          MCKeyword[mckStatusCode],
          scFAILURE);

        Align = (int)strlen(S->SBuffer) + (int)strlen(MCKeyword[mckArgs]);

        sprintf(S->SBuffer,"%s%*s%s",
          S->SBuffer,
          16 - (Align % 16),
          " ",
          MCKeyword[mckArgs]);

        S->SPtr = &S->SBuffer[strlen(S->SBuffer)];
        }  /* END if (S->SBuffer != NULL) */

      break;
    }  /* END switch (S->WireProtocol) */

  return(SUCCESS);
  }  /* END MUISClear() */





/**
 * Sets the given message to the socket's sending buffer.
 *
 * @param S (I) [modified]
 * @param Msg (I)
 */

int MUISAddData(

  msocket_t  *S,
  const char *Msg)

  {
  mxml_t *E;

  /* NOTE:  existing message is erased, not appended */

  if (S == NULL)
    {
    return(FAILURE);
    }

  switch (S->WireProtocol)
    {
    case mwpXML:

      {
      mbool_t SetVal = TRUE;

      if ((Msg != NULL) && (Msg[0] != '\0'))
        {
        if (S->SE == NULL)
          {
          if (!strncmp(Msg,"<Data>",strlen("<Data>")))
            {
            /* Msg is XML encapsulated in Data object */

            MXMLFromString(&S->SE,Msg,NULL,NULL);
 
            SetVal = FALSE;
            }
          else
            { 
            /* Passing a 'const char *', so cast to just a 'char *' */
            MXMLCreateE(&S->SE,(char *)MSON[msonData]);
            }
          }
        else
          {
          if (!strncmp(Msg,"<Data>",strlen("<Data>")))
            {
            /* Need to remove S->SDE, otherwise we get double Data tags! */

            MXMLDestroyE(&S->SDE);

            MXMLFromString(&S->SDE,Msg,NULL,NULL);

            SetVal = FALSE;
            }
          }

        if (SetVal == TRUE)
          {
          E = S->SE;

          if (E->Val != NULL)
            {
            /* append new string */

            char *ptr = (char *)realloc(E->Val,strlen(E->Val) + strlen(Msg) + 1);
  
            /* '14 hack' (see MXMLSetVal()) */

            if (ptr != NULL)
              {
              strcat(ptr,Msg);

              E->Val = ptr;

              if (E->Val != NULL)
                {
                for (ptr = strchr(E->Val,'<');ptr != NULL;ptr = strchr(ptr,'<'))
                  *ptr = (char)14;
                }
              }
            }
          else
            {
            /* set new string */

            MXMLSetVal(E,(void *)Msg,mdfString);
            }
          }
        }    /* END if ((Msg != NULL) && (Msg[0] != '\0')) */
      }      /* END BLOCK */

      break;

    case mwpS32:

      {
      mbool_t SetVal = TRUE;

      if ((Msg != NULL) && (Msg[0] != '\0'))
        {
        if (S->SDE == NULL)
          {
          if (!strncmp(Msg,"<Data>",strlen("<Data>")))
            {
            /* Msg is XML encapsulated in Data object */

            MXMLFromString(&S->SDE,Msg,NULL,NULL);

            SetVal = FALSE;
            }
          else
            {
            /* Passing a 'const char *', so cast to just a 'char *' */
            MXMLCreateE(&S->SDE,(char *)MSON[msonData]);
            }
          }
        else
          {
          if (!strncmp(Msg,"<Data>",strlen("<Data>")))
            {
            /* Need to remove S->SDE, otherwise we get double Data tags! */

            MXMLDestroyE(&S->SDE);

            MXMLFromString(&S->SDE,Msg,NULL,NULL);

            SetVal = FALSE;
            }
          }

        E = S->SDE;

        if (SetVal == TRUE)
          {
          if (E->Val != NULL)
            {
            /* append new string */

            char *ptr = (char *)realloc(E->Val,strlen(E->Val) + strlen(Msg) + 1);

            if (ptr != NULL)
              {
              strcat(ptr,Msg);

              E->Val = ptr;

              /* '14 hack' (see MXMLSetVal()) */

              if (E->Val != NULL)
                {
                for (ptr = strchr(E->Val,'<');ptr != NULL;ptr = strchr(ptr,'<'))
                  *ptr = (char)14;
                }
              }
            }
          else
            {
            /* set new string */

            MXMLSetVal(E,(void *)Msg,mdfString);
            }
          }    /* END if (SetVal == TRUE) */
        }      /* END if ((Msg != NULL) && (Msg[0] != '\0')) */
      }        /* END BLOCK */

      /* NYI */

      break;

    default:

      if (S->SBuffer == NULL)
        {
        MUStrDup(&S->SBuffer,Msg);
        }
      else
        {
        MUStrCpy(S->SPtr,Msg,S->SBufSize - (S->SPtr - S->SBuffer));
        }

      break;
    }  /* END switch (S->WireProtocol) */

  return(SUCCESS);
  }  /* END MUISAddData() */



/**
 * Translate high-level object/action attributes and issue appropriate routine 
 * to process request.
 *
 * NOTE:  Handle new format XML requests
 *
 * @see MCDoCommand() - child - handle 'forward' requests
 * @see MSysProcessRequest() - parent - also handles old format client requests
 *
 * @param S    (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SCP  (O) [optional]
 */

int MUISProcessRequest(

  msocket_t *S,
  char      *EMsg,
  int       *SCP)

  {
  char tmpAuth[MMAX_LINE];
  char tmpLine[MMAX_LINE];

  enum MClientCmdEnum CIndex;
  enum MSvcEnum       SIndex;

  int                 SC;

  mbitmap_t AFlags;  /* enum MRoleEnum bitmap */

  char *ptr = NULL;

  const char *FName = "MUISProcessRequest";

  MDB(3,fUI) MLog("%s(S,EMsg,SCP)\n",
    FName);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SCP != NULL)
    *SCP = 0;

  if (S == NULL)
    {
    return(FAILURE);
    }

  /* determine if forwarding is required */

  if ((S->RDE != NULL) &&
      (MXMLGetAttr(S->RDE,"forward",NULL,tmpLine,sizeof(tmpLine)) == SUCCESS) &&
      (tmpLine[0] != '\0'))
    {
    char  *RPtr;

    int    rc;

    mpsi_t *P;
    mrm_t  *PR;

    char tmpReq[MMAX_LINE << 3];
    
    enum MSvcEnum CIndex;

    msocket_t tmpS;

    mccfg_t   Peer;

    memset(&Peer,0,sizeof(Peer));

    /* verify forward destination is valid and is authorized */

    if (MRMFind(tmpLine,&PR) == FAILURE)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"cannot locate forwarding destination");

      MUISAddData(S,"cannot locate forwarding destination");

      if (SCP != NULL)
        *SCP = mscNoEnt;

      return(FAILURE);
      }

    P = &PR->P[0];

    if (P == NULL)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"destination has incomplete interface information");

      MUISAddData(S,"destination has incomplete interface information");

      if (SCP != NULL)
        *SCP = mscNoEnt;

      return(FAILURE);
      }

    if (PR->Type != mrmtMoab)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"destination is not peer");

      MUISAddData(S,"destination is not peer");

      if (SCP != NULL)
        *SCP = mscNoEnt;

      return(FAILURE);
      }

    if (PR->State != mrmsActive)
      {
      char tmpLine[MMAX_LINE];

      /* allow room for newline to be added later */

      snprintf(tmpLine,sizeof(tmpLine) - 1,"destination %s in not available - state=%s",
        PR->Name,
        MRMState[PR->State]);

      if (EMsg != NULL)
        strcpy(EMsg,tmpLine);

      strcat(tmpLine,"\n");

      MUISAddData(S,tmpLine);

      if (SCP != NULL)
        *SCP = mscNoEnt;

      return(FAILURE);
      }

    CIndex = mcsMSchedCtl;  /* NOTE:  hardcode for now */

    strcpy(Peer.ServerHost,P->HostName);
    Peer.ServerPort = P->Port;
   
    snprintf(Peer.RID,sizeof(Peer.RID),"PEER:%s",
      MSched.Name);

    Peer.ServerCSAlgo = P->CSAlgo;
    MUStrCpy(Peer.ServerCSKey,P->CSKey,sizeof(Peer.ServerCSKey));
   
    /* remove 'forward' XML attribute */

    MXMLRemoveAttr(S->RDE,"forward");
 
    /* forward message to target */

    MXMLToString(S->RDE,tmpReq,sizeof(tmpReq),NULL,TRUE);

    rc = MCDoCommand(
      CIndex,
      &Peer,
      tmpReq,    /* I */
      &RPtr,     /* O */
      EMsg,
      &tmpS);    /* O */

    if (rc == FAILURE)
      {
      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"forwarded request failed");

      MUISAddData(S,"forwarded request failed");

      return(FAILURE);
      }

    /* report results directly to client */

    S->SBuffer[0] = '\0';

    if (MUStringIsXML(RPtr) == TRUE)
      {
      mxml_t *SE = NULL;

      if (MXMLFromString(&SE,RPtr,NULL,NULL) == SUCCESS)
        {
        mxml_t *DE;

        MXMLCreateE(&DE,"Data");

        MXMLAddE(DE,SE);

        S->SDE = DE;
        }
      }
    else if (RPtr != NULL)
      {
      MUISAddData(S,RPtr);
      }
    else
      {
      S->SDE = tmpS.RDE;
      }

    MUFree(&RPtr);

    MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);

    return(SUCCESS);
    }  /* END if ((S->RDE != NULL) && ...) */

#ifdef MENABLETID
  if ((S->RemoteTID == 0) &&
      (S->RDE != NULL) &&
      (MXMLGetAttr(S->RDE,MSAN[msanTID],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS) &&
      (tmpLine[0] != '\0'))
    {
    S->RemoteTID = strtol(tmpLine,NULL,10);
    }
#endif /* MENABLETID */

  /* get command */

  switch (S->WireProtocol)
    {
    case mwpS32:

      {
      char object[MMAX_NAME];
      char action[MMAX_NAME];

      enum MXMLOTypeEnum OIndex;

      char *ptr;
      char *TokPtr;

      if ((S->RDE == NULL) || 
          (MS3GetObject(S->RDE,object) == FAILURE) ||
          (MXMLGetAttr(S->RDE,MSAN[msanAction],NULL,action,sizeof(action)) == FAILURE))
        {
        MDB(3,fSOCK) MLog("ALERT:    cannot locate action\n");

        if (EMsg != NULL)
          strcpy(EMsg,"cannot locate action");

        MUISAddData(S,"request is corrupt - invalid action specified\n");

        return(FAILURE);
        }

      /*  process only initial object */

      ptr = MUStrTok(object,",",&TokPtr);

      OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

      switch (OIndex)
        {
        /* set per object default action */

        case mxoSched:

          CIndex = mccSchedCtl;

          break;

        case mxoJob:

          CIndex = mccJobCtl;

          break;

        case mxoNode:

          CIndex = mccNodeCtl;

          break;

        case mxoxVC:

          CIndex = mccVCCtl;

          break;

        case mxoxVM:

          CIndex = mccVMCtl;

          break;

        case mxoUser:
        case mxoGroup:
        case mxoAcct:
        case mxoClass:
        case mxoQOS:

          CIndex = mccCredCtl;

          break;

        case mxoRM:

          CIndex = mccRMCtl;

          break;

        default:

          CIndex = mccNONE;

          break;
        }  /* END switch (OIndex) */

      if (!strcmp(action,"check"))
        {
        switch (OIndex)
          {
          case mxoJob:

            CIndex = mccCheckJob;

            break;

          case mxoNode:

            CIndex = mccCheckNode;

            break;

          default:

            /* NO-OP */

            break;
          }
        }
      else if (!strcmp(action,"diagnose"))
        {
        CIndex = mccDiagnose;
        }
      else if (!strcmp(action,"execute"))
        {
        CIndex = mccBal;
        }
      else if (!strcmp(action,"query"))
        {
        switch (OIndex)
          {
          case mxoQueue:
          case mxoxLimits:
          case mxoxCP:
          case mxoCluster:
          case mxoPar:

            CIndex = mccShow;

            break;

          case mxoxStats:

            CIndex = mccStat;

            break;

          case mxoNode:

            CIndex = mccNodeCtl;

            break;

          case mxoUser:
          case mxoGroup:
          case mxoAcct:
          case mxoClass:
          case mxoQOS:
          case mxoSched:  /* NOTE: move */

            CIndex = mccCredCtl;

            break;

          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          default:

            /* NO-OP */
            
            break;
          }  /* END switch (OIndex) */
        }
      else if (!strcmp(action,"list"))
        {
        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:
          case mxoClass:
          case mxoQOS:
          case mxoPar:

            CIndex = mccCredCtl;

            break;

          case mxoNode:

            CIndex = mccNodeCtl;

            break;

          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }
      else if (!strcmp(action,"modify") || !strcmp(action,"alloc")) 
        {
        switch (OIndex)
          {
          case mxoUser:
          case mxoGroup:
          case mxoAcct:
          case mxoClass:
          case mxoQOS:

            CIndex = mccCredCtl;

            break;

          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          case mxoRM:

            CIndex = mccRMCtl;

            break;

          case mxoNode:

            CIndex = mccNodeCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }
      else if (!strcmp(action,"create"))
        {
        switch (OIndex)
          {
          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          case mxoRM:

            CIndex = mccRMCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }
      else if (!strcmp(action,"destroy"))
        {
        switch (OIndex)
          {
          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          case mxoUser:
          case mxoGroup:
          case mxoAcct:
          case mxoClass:
          case mxoQOS:

            CIndex = mccCredCtl;

            break;

          case mxoNode:

            CIndex = mccNodeCtl;

            break;

          case mxoRM:

            CIndex = mccRMCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }    /* END else if (!strcmp(action,"destroy")) */
      else if (!strcmp(action,"show"))
        {
        switch (OIndex)
          {
          case mxoCluster:

            CIndex = mccShowState;

            break;

          case mxoRsv:

            CIndex = mccRsvShow;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }    /* END else if (!strcmp(action,"show")) */
      else if (!strcmp(action,"submit"))
        {
        switch (OIndex)
          {
          case mxoJob:

            CIndex = mccSubmit;

            if (MUFindName(MSched.SubmitHosts,S->RemoteHost,MMAX_SUBMITHOSTS) == FAILURE)
              {
              char Message[MMAX_LINE];

              snprintf(Message,sizeof(Message),"ERROR:    job submission disabled from host '%s'\n",
                 S->RemoteHost);

              if (SCP != NULL)
                *SCP = mscNoEnt;

              MUISAddData(S,Message);

              return(FAILURE);
              }

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }    /* END else if (!strcmp(action,"submit")) */
      else if (!strcmp(action,"migrate"))
        {
        switch (OIndex)
          {
          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }    /* END else if (!strcmp(action,"migrate")) */
      else if (!strcmp(action,"signal"))
        {
        switch (OIndex)
          {
          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }    /* END else if (!strcmp(action,"signal")) */
      else if (!strcmp(action,"flush"))
        {
        switch (OIndex)
          {
          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }
      else if (!strcmp(action,"join"))
        {
        switch (OIndex)
          {
          case mxoRsv:

            CIndex = mccRsvCtl;

            break;

          default:

            /* not handled */

            break;
          }  /* END switch (OIndex) */
        }    /* END else if (!strcmp(action,"join")) */
      }      /* END BLOCK (case mwp32) */

      break;

    case mwpNONE:
    default:

      if ((ptr = strstr(S->RBuffer,MCKeyword[mckCommand])) == NULL)
        {
        MDB(3,fSOCK) MLog("ALERT:    cannot locate command\n");

        if (EMsg != NULL)
          strcpy(EMsg,"cannot locate command");

        return(FAILURE);
        }

      ptr += strlen(MCKeyword[mckCommand]);

      CIndex = (enum MClientCmdEnum)MUGetIndexCI(ptr,MClientCmd,TRUE,mccNONE);

      break;
    }  /* END switch (S->WireProtocol) */

  /*
   * Check the function pointer table for an invalid entry, 
   * if invalid, log and error return
   */
  if (MCheckMCRequestFunction(CIndex) != SUCCESS)
    {
    MDB(3,fUI) MLog("INFO:     command '%.16s...' not handled in %s (will be evaluated elsewhere)\n",
      (ptr != NULL) ? ptr : "N/A",
      FName);

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"cannot support command '%10.10s'",
        (ptr != NULL) ? ptr : "N/A");

    return(FAILURE);
    }

  /* locate arguments */

  switch (S->WireProtocol)
    {
    case mwpS32:

      /* NO-OP */

      break;

    default:

      if ((ptr = strstr(S->RBuffer,MCKeyword[mckArgs])) == NULL)
        {
        MDB(3,fSOCK) MLog("ALERT:    cannot locate command args\n");

        if (EMsg != NULL)
          strcpy(EMsg,"cannot locate command data");

        return(FAILURE);
        }

      ptr += strlen(MCKeyword[mckArgs]);

      S->RPtr = ptr;

      break;
    }  /* END switch (S->WireProtocol) */

  /* check authentication */

  switch (S->WireProtocol)
    {
    case mwpS32:

      if (S->RID != NULL)
        {
        MUStrCpy(tmpAuth,S->RID,sizeof(tmpAuth));
        }
      else
        {
        tmpAuth[0] = '\0';
        }

      break;

    default:

      /* get authentication */

      if ((ptr = strstr(S->RBuffer,MCKeyword[mckAuth])) == NULL)
        {
        MDB(3,fSOCK) MLog("ALERT:    cannot locate authentication\n");

        if (EMsg != NULL)
          strcpy(EMsg,"cannot locate authentication");

        return(FAILURE);
        }

      ptr += strlen(MCKeyword[mckAuth]);

      MUSScanF(ptr,"%x%s",
        sizeof(tmpAuth),
        tmpAuth);

      break;
    }  /*  END switch (S->WireProtocol) */

  MCCToMCS(CIndex,&SIndex);

  MSysGetAuth(tmpAuth,SIndex,0,&AFlags);

  if ((bmisset(&AFlags,mcalAdmin1) && (MSched.Admin[1].EnableProxy)) ||
      (bmisset(&AFlags,mcalAdmin2) && (MSched.Admin[2].EnableProxy)) ||  
      (bmisset(&AFlags,mcalAdmin3) && (MSched.Admin[3].EnableProxy)) ||
      (bmisset(&AFlags,mcalAdmin4) && (MSched.Admin[4].EnableProxy)))
    {
    char proxy[MMAX_NAME];

    MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy));

    if (proxy[0] != '\0')
      {
      MUStrCpy(tmpAuth,proxy,sizeof(tmpAuth));

      MSysGetAuth(proxy,SIndex,0,&AFlags);
      }
    }

  S->SBuffer[0] = '\0';

  if (S->SIndex == mcsNONE)
    S->SIndex = SIndex;

  SC = FAILURE;

  switch (CIndex)
    {
    default:

      {
      MUISClear(S);

      /*
       * Call the selected MUI function to process this request
       */
      SC = MDispatchMCRequestFunction(CIndex,S,&AFlags,tmpAuth);

      if ((SC == FAILURE) && (S->StatusCode == 0))
        {
        MUISSetStatus(S,CIndex,999,NULL);
        }

      if (S->LogFile != NULL)
        {
        char tmpInfoLine[MMAX_LINE];

        snprintf(tmpInfoLine,sizeof(tmpInfoLine),"used log file '%s' at log level %d for this command\n",
          S->LogFile,
          S->LogThreshold);

        MUISAddData(S,tmpInfoLine);
        }

      if (S->SBuffer != NULL)
        S->SBufSize = (long)strlen(S->SBuffer);

      if (!bmisset(&S->Flags,msftDropResponse))
        {
        MSUSendData(S,MSched.SocketWaitTime,TRUE,TRUE,NULL,NULL);
        }
      else
        {
        S->sd = -1;
        }
      }  /* END BLOCK */

      break;
    }  /* END switch (CIndex) */


  /* mrelAllSchedCommand --- Record all commands.
   * mrelSchedCommand --- Record only commands that perform an action. */

  if (bmisset(&MSched.RecordEventList,mrelAllSchedCommand))
    {
    MSysRecordCommandEvent(S,SIndex,SC,tmpAuth);
    }
  else if (bmisset(&MSched.RecordEventList,mrelSchedCommand))
    {
    switch (SIndex)
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
        MSysRecordCommandEvent(S, SIndex, SC, tmpAuth);

        break;
      }  /* END switch (SIndex) */
    }    /* END if (bmisset(&MSched.RecordEventList,mrelSchedCommand)) */

  return(SUCCESS);
  }  /* END MUISProcessRequest() */
/* END MUISocketFuncs.c */
