/* HEADER */

/**
 * @file MRsv.c
 *
 * Moab Reservations
 */

/* Contains:                          */
/*  int MRsvCreate(Type,ACL,CAName,Flags,NL,STime,ETime,NC,PC,RBName,RP,HExp,DRes,NULL) */
/*                                    */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "mcom.h"
#include "MEventLog.h"


/**
 *
 *
 * @param O
 * @param OType
 * @param RM
 * @param RsvName
 */

int MOAddRemoteRsv(

  void               *O,
  enum MXMLOTypeEnum  OType,
  mrm_t              *RM,
  char               *RsvName)

  {
  mrrsv_t *RR;

  mrsv_t  *R;

  mreq_t  *RQ;

  if ((O == NULL) || (RM == NULL) || (RsvName == NULL) || (RsvName[0] == '\0'))
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoRsv:

      R = (mrsv_t *)O;

      if (R->RemoteRsv == NULL)
        {
        R->RemoteRsv = (mrrsv_t *)MUCalloc(1,sizeof(mrrsv_t));

        if (R->RemoteRsv == NULL)
          {
          return(FAILURE);
          }

        RR = R->RemoteRsv;
        }
      else
        {
        RR = R->RemoteRsv;

        while (RR->Next != NULL)
          {
          RR = RR->Next;
          }

        RR->Next = (mrrsv_t *)MUCalloc(1,sizeof(mrrsv_t));

        RR = RR->Next;

        RR->LocalRsv = R;
        }

      break;

    case mxoReq:

      RQ = (mreq_t *)O;

      if (RQ->RemoteR == NULL)
        {
        RQ->RemoteR = (mrrsv_t *)MUCalloc(1,sizeof(mrrsv_t));

        if (RQ->RemoteR == NULL)
          {
          return(FAILURE);
          }

        RR = RQ->RemoteR;
        }
      else
        {
        RR = RQ->RemoteR;
        }

      break;

    default:

      return(FAILURE);

      /* NOTREACHED */

      break;
    }

  MUStrCpy(RR->Name,RsvName,MMAX_NAME);

  RR->RM = RM;

  RR->LastSync = MSched.Time;

  RR->Next = NULL;

  /* add reservation to rm list of remote rsvs */

  if (RM->RemoteR == NULL)
    {
    RM->RemoteR = (mrrsv_t *)MUCalloc(1,sizeof(mrrsv_t));

    if (RM->RemoteR == NULL)
      {
      return(FAILURE);
      }

    RR = RM->RemoteR;
    }
  else
    {
    RR = RM->RemoteR;

#ifdef MNOT
    if (!strcmp(RR->Name,RsvName))
      {
      RR->LastSync = MSched.Time;

      return(SUCCESS);
      }
#endif

    while (RR->Next != NULL)
      {
#ifdef MNOT
      if (!strcmp(RR->Name,RsvName))
        {
        RR->LastSync = MSched.Time;

        return(SUCCESS);
        }
#endif

      RR = RR->Next;
      }

    RR->Next = (mrrsv_t *)MUCalloc(1,sizeof(mrrsv_t));

    RR = RR->Next;
    }

  if (OType == mxoRsv)
    RR->LocalRsv = (mrsv_t *)O;

  RR->RM = RM;

  RR->Next = NULL;

  MUStrCpy(RR->Name,RsvName,MMAX_NAME);

  RR->LastSync = MSched.Time;

  return(SUCCESS);
  }  /* END MRsvAddRemoteRsv() */




/**
 *
 *
 * @param R
 * @param OIndex
 * @param NodeList
 * @param DoModifyHostExp
 * @param EMsg
 */

int MRsvModifyNodeList(

  mrsv_t                  *R,               /* I (modified) */
  enum MObjectSetModeEnum  OIndex,          /* I */
  char                    *NodeList,        /* I */
  mbool_t                  DoModifyHostExp, /* I */
  char                    *EMsg)            /* O (optional) */

  {
  char *ptr;
  char *TokPtr = NULL;

  char *MPtr;
  int   MSpace;

  int HostCount;
  int TC;
  int NC;
  int nindex;

  int Mode;

  char  Buffer[MMAX_BUFFER];
  char  tmpName[MMAX_NAME];
  
  marray_t NList;

  mnode_t *N;

  if ((R == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  switch (OIndex)
    {
    case mAdd:

      Mode = 1;

      break;

    case mDecr:

      Mode = -1;

      break;

    default:

      snprintf(EMsg,MMAX_LINE,"ERROR:  operation on rsv %s not supported\n",
        R->Name);

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (OIndex) */

  MUSNInit(&MPtr,&MSpace,EMsg,MMAX_LINE);

  HostCount = 0;

  /* NOTE:  convert tmpVal into node expression (NYI) */

  ptr = MUStrTok(NodeList,"= \t\n",&TokPtr);

  while (ptr != NULL)
    {
    Buffer[0] = '\0';

    MUArrayListCreate(&NList,sizeof(mnode_t *),-1);

    if ((MUREToList(
            ptr,
            mxoNode,
            NULL,    /* partition constraint */
            &NList,  /* O */
            FALSE,
            Buffer,
            sizeof(Buffer)) == FAILURE) || (NList.NumItems == 0))
      {
      MDB(2,fCONFIG) MLog("ALERT:    cannot expand hostlist '%s'\n",
        ptr);

      MUSNPrintF(&MPtr,&MSpace,"ERROR:  expression '%s' does not match any nodes\n",
        ptr);

      ptr = MUStrTok(NULL,"= \t\n",&TokPtr);

      MUArrayListFree(&NList);

      continue;
      }

    for (nindex = 0;nindex < NList.NumItems;nindex++)
      {
      N = (mnode_t *)MUArrayListGetPtr(&NList,nindex);

      if (MRsvAdjustNode(R,N,-1,Mode,TRUE) == FAILURE)
        {
        MUSNPrintF(&MPtr,&MSpace,"ERROR:  cannot locate node %s in rsv %s\n",
          N->Name,
          R->Name);

        MDB(3,fUI) MLog("INFO:     cannot remove node '%s' from rsv %s\n",
          ptr,
          R->Name);

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);

        continue;
        }

      if (Mode == -1)
        {
        sprintf(tmpName,"^%s$",
          N->Name);

        /* remove node from R->NL,R->ReqNL,R->HostExp */

        MNLRemove(&R->NL,N);
        MNLRemove(&R->ReqNL,N);

        if ((DoModifyHostExp == TRUE) &&
            (MUStrRemoveFromList(R->HostExp,tmpName,'|',FALSE) == FAILURE))
          {
          sprintf(tmpName,"%s",
            N->Name);

          MUStrRemoveFromList(R->HostExp,tmpName,',',FALSE);
          }
        }
      else if (Mode == 1)
        {
        if (DoModifyHostExp == TRUE)
          {
          if (strchr(R->HostExp,'|') != NULL)
            {
            sprintf(tmpName,"^%s$",
              N->Name);

            MUStrAppend(&R->HostExp,NULL,tmpName,'|');
            }
          else
            {
            sprintf(tmpName,"%s",
              N->Name);

            MUStrAppend(&R->HostExp,NULL,tmpName,',');
            }
          }

        /* add node to R->NL,R->ReqNL,R->HostExp */

        MNLAdd(&R->NL,N,0);
        MNLAdd(&R->ReqNL,N,0);
        }
      else
        {
        /* set R->HostExp */

        /* NYI */
        }

      HostCount++;
      }  /* END for (nindex) */

    MUArrayListFree(&NList);

    ptr = MUStrTok(NULL,", \t\n",&TokPtr);
    }    /* END while (ptr != NULL) */

  TC = 0;
  NC = 0;

  for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,NULL) == SUCCESS;nindex++)
    {
    TC += MNLGetTCAtIndex(&R->NL,nindex);
    NC++;
    }

  if (R->ReqTC != 0)
    R->ReqTC   = TC;

  if (R->ReqNC != 0)
    R->ReqNC   = NC;

  R->AllocTC = TC;
  R->AllocNC = NC;

  MUSNPrintF(&MPtr,&MSpace,"NOTE:  %d host%s %s rsv %s\n",
    HostCount,
    (HostCount != 1) ? "s" : "",
    (Mode == -1) ? "removed from" : "added to",
    R->Name);

  return(SUCCESS);
  }  /* END MRsvModifyNodeList() */





/**
 * @see MRsvModifyNodeList() - peer
 *
 * @param R
 * @param OIndex
 * @param TC
 * @param OldTC
 * @param EMsg
 */

int MRsvModifyTaskCount(

  mrsv_t                  *R,      /* I (modified) */
  enum MObjectSetModeEnum  OIndex, /* I */
  int                      TC,     /* I */
  int                      OldTC,  /* I */
  char                    *EMsg)   /* O (required) */

  {
  int NewTC;
  int OrigTC;

  mnl_t *NL;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((R == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  if (TC == 0)
    {
    return(FAILURE);
    }

  OrigTC = R->ReqTC;

  switch (OIndex)
    {
    case mAdd:

      NewTC = OrigTC + TC;

      break;

    case mDecr:

      NewTC = OrigTC - TC;

      break;

    case mSet:
    default:

      NewTC = TC;

      break;
    }  /* END switch (OIndex) */

  if (NewTC == OrigTC)
    {
    /* no change */

    return(SUCCESS);
    }

  if (NewTC <= 0)
    {
    /* illegal request */

    snprintf(EMsg,MMAX_LINE,"NOTE:  illegal task count specified for rsv %s\n",
      R->Name);

    return(FAILURE);
    }

  if ((MNLIsEmpty(&R->NL)) &&
      ((R->AllocNC > 0) ||
      ((R->HostExp != NULL) && (strstr(R->HostExp,"GLOBAL") != NULL))))
    {
    MRsvCreateNL(
      &R->NL,
      R->HostExp,
      0,
      0,
      NULL,
      &R->DRes,
      NULL);

    if (MNLIsEmpty(&R->NL))
      {
      snprintf(EMsg,MMAX_LINE,"NOTE:  cannot create nodelist for %s\n",
        R->Name);

      return(FAILURE);
      }
    }

  NL = &R->NL;

  if (NewTC < OrigTC)
    {
    mnode_t *tmpN;

    int nindex;
    int NC;
    int tmpI;
    int tmpTC;

    /* release resources */

    TC = OrigTC - NewTC;

    NC = MNLCount(NL);

    /* assume nodes allocated in highest priority first order */
    /* release nodes in last node first order */

    for (nindex = NC - 1;nindex >= 0;nindex--)
      {
      /* walk list */

      MNLGetNodeAtIndex(NL,nindex,&tmpN);
      tmpTC = MNLGetTCAtIndex(NL,nindex);

      tmpI = MIN(tmpTC,TC);

      /* adjust node and rsv alloc info */

      MRsvAdjustNode(R,tmpN,tmpI,-1,TRUE);

      /* adjust rsv nodelist */

      if (tmpI == tmpTC)
        MNLTerminateAtIndex(NL,nindex);
      else
        MNLSetTCAtIndex(NL,nindex,tmpTC - tmpI);

      TC -= tmpI;

      if (TC <= 0)
        break;
      }  /* END for (nindex) */

    MRsvSetAttr(R,mraReqTaskCount,(void **)&NewTC,mdfInt,mSet);

    snprintf(EMsg,MMAX_LINE,"reservation %s taskcount adjusted from %d to %d\n",
      R->Name,
      OrigTC,
      NewTC);

    /* does rsv hostexpression need to be adjusted? (NYI) */
    }
  else
    {
    char tmpBuf[MMAX_BUFFER];

    /* allocate resources */

    MRsvSetAttr(R,mraReqTaskCount,(void **)&NewTC,mdfInt,mSet);

    if (MRsvConfigure(NULL,R,-1,-1,tmpBuf,NULL,NULL,TRUE) == FAILURE)
      {
      snprintf(EMsg,MMAX_LINE,"NOTE:  cannot allocate requested resources for rsv %s - %s\n",
        R->Name,
        tmpBuf);

      MRsvSetAttr(R,mraReqTaskCount,(void **)&OldTC,mdfInt,mSet);

      return(FAILURE);
      }
    else
      {
      int nindex;

      int TC;
      int NC;

      /* rebuild ReqNL */

      MNLToString(&R->NL,TRUE,",",'\0',tmpBuf,sizeof(tmpBuf));

      MRsvSetAttr(R,mraReqNodeList,tmpBuf,mdfString,mSet);

      TC = 0;
      NC = 0;

      for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,NULL) == SUCCESS;nindex++)
        {
        TC += MNLGetTCAtIndex(&R->NL,nindex);
        NC++;
        }

      R->AllocTC = TC;

      if (R->ReqTC != 0)
        R->ReqTC   = TC;

      R->AllocNC = NC;

      if (R->ReqNC != 0)
        R->ReqNC   = NC;

      snprintf(EMsg,MMAX_LINE,"reservation %s taskcount adjusted from %d to %d\n",
        R->Name,
        OrigTC,
        NewTC);
      }  /* END else */
    }    /* END else */

  return(SUCCESS);
  }  /* END MRsvModifyTaskCount() */





/**
 * Removes given nodes from the reseravtion and reconfigures the reservation.
 *
 * Scenario: RSVREALLOCPOLICY FAILURE; reservation with a host-expression covering 3 nodes 
 * with a taskcount of 2. The reservation gets the first 2 nodes. Later the 2nd node goes
 * down. Because this routine thinks that the downed node has available taskcount and returns
 * RTC nodes from the host-expression, reservation will never get a taskcount of 2 because
 * the third node isn't considered and the downed node isn't available in MJobGetEStartTime. 
 *
 * Reservation Nuance: A host-expression reservation ignores the state of nodes. 
 *
 * @see MRsvReplaceNode() - relationship???
 *
 * @see MRsvRefresh() - parent
 * @see MRsvModifyNodeList() - child
 * @see MRsvModifyTaskCount() - child
 *
 * @param R (I) [modified]
 * @param NodeList (I)
 * @param EMsg (O) [optional]
 */

int MRsvReplaceNodes(

  mrsv_t    *R,         /* I (modified) */
  char      *NodeList,  /* I */
  char      *EMsg)      /* O (optional) */

  {
  int OldTC;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  OldTC = MNLSumTC(&R->NL);

  /* remove NodeList nodes from reservation R */

  if (MRsvModifyNodeList(R,mDecr,NodeList,FALSE,EMsg) == FAILURE)
    return(FAILURE);

  /* Reconfigure reservation through MRsvConfigure */
  
  if (MRsvModifyTaskCount(R,mSet,OldTC,OldTC,EMsg) == FAILURE)
    return(FAILURE);

  /* if modify failed, then previous reservation is ruined. */

  return(SUCCESS);
  }  /* END MRsvReplaceNodes() */






/**
 * Locate reservation with matching id or reservation group id.
 *
 * @param RsvID (I)
 * @param RP (O) [optional]
 * @param RAttr (I) [mraNONE,mraRsvGroup,mraUIndex,mraGlobalID]
 */

int MRsvFind(

  const char  *RsvID,
  mrsv_t     **RP,
  enum MRsvAttrEnum RAttr)

  {
  rsv_iter RTI;

  mrsv_t *R;

  char   *ptr;

  const char *FName = "MRsvFind";

  MDB(8,fCORE) MLog("%s(%s,RP,RsvTok,%s)\n",
    FName,
    (RsvID != NULL) ? RsvID : "NULL",
    MRsvAttr[RAttr]);

  if (RP != NULL)
    *RP = NULL;

  if ((RsvID == NULL) || (RsvID[0] <= '\1'))
    {
    return(FAILURE);
    }

  /* locate reservation */

  R = NULL;

  if (RAttr == mraNONE)
    {
    return (MRsvTableFind(RsvID,RP));
    }

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (RAttr == mraUIndex)
      {
      /* NOTE:  UID only apply to non-job, non-standing reservations */

      if (R->Type == mrtJob)
        continue;

      if (bmisset(&R->Flags,mrfStanding) == TRUE)
        continue;

      /* FORMAT:  <RSVNAME>.<UID> */

      if ((ptr = strchr(R->Name,'.')) == NULL)
        continue;

      if (!strcmp(RsvID,ptr + 1))
        {
        /* match located */

        break;
        }
      }
    else if (RAttr == mraRsvGroup)
      {
      if ((R->RsvGroup != NULL) && !strcmp(RsvID,R->RsvGroup))
        {
        /* match located */

        break;
        }
      }
    else if ((RAttr == mraGlobalID) &&
             (R->SystemRID != NULL) &&
             !strcmp(RsvID,R->SystemRID))
      {
      /* match located */

      break;
      }
    else if (!strcmp(RsvID,R->Name))
      {
      /* match located */

      break;
      }
    else if ((R->Label != NULL) && !strcmp(RsvID,R->Label))
      {
      /* match located */

      break;
      }
    }  /* END while (MRsvTableIterate()) */

  if (RP != NULL)
    *RP = R;

  if (R == NULL)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRsvFind() */




/**
 * Called every iteration after scheduling is complete.
 *
 * @see MSysMainLoop()->MSchedUpdateStats() - parent
 * @see MRsvRefresh() - peer - called after MRMUpdate() before start of 
 *        scheduling cycle
 * @see MSRUpdateGridSandboxes() - child
 *
 * responsible for updating stats, SR application launch,
 * adjusting rsv active state, setting global rsv bools, etc
 */

int MRsvUpdateStats()

  {
  rsv_iter RTI;

  mrsv_t  *R;
  mnode_t *N;
  mre_t   *RE;

  int     nindex;
  int     rmindex;

  int     RC;
  int     PC;

  int     BPC;

  mcres_t *BR;

  mbool_t GridSandboxIsGlobal;
  mbool_t tmpGridSandboxIsGlobal = FALSE;

  double  IPS;

  double  interval;
  double  fsusage;

  const char *FName = "MRsvUpdateStats";

  MDB(3,fSTRUCT) MLog("%s()\n",
    FName);

  interval = (double)MSched.Interval / 100.0;

  MSched.SRsvIsActive = FALSE;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    if (MRM[rmindex].Name[0] == '\0')
      break;

    if (MRMIsReal(&MRM[rmindex]) == FALSE)
      continue;

    MRM[rmindex].GridSandboxExists = FALSE;
    }

  /* NOTE:  SR check should be moved to SR update */

  GridSandboxIsGlobal = FALSE;

  MSRUpdateGridSandboxes(&GridSandboxIsGlobal); /* check standing reservations */

  MRsvUpdateGridSandboxes(&tmpGridSandboxIsGlobal);
  GridSandboxIsGlobal |= tmpGridSandboxIsGlobal; /* "or" the result for regular reservations */

  /* no support for admin (grid sandbox) reservations yet */

  /* don't transition any rsvs in this loop, transition them all after */

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((GridSandboxIsGlobal == FALSE) && bmisset(&R->Flags,mrfAllowGrid))
      {
      MSched.GridSandboxExists = TRUE;
      }

    if (MSched.Time > R->EndTime)
      {
      /* rsv is complete */

      if (bmisset(&R->Flags,mrfIsActive) == TRUE)
        {
        bmunset(&R->Flags,mrfIsActive);

        if ((R->Type != mrtJob) && !bmisset(&R->Flags,mrfStanding))
          {
          /* record admin rsv end event */

          MOWriteEvent((void *)R,mxoRsv,mrelRsvEnd,NULL,MStat.eventfp,NULL);

          CreateAndSendEventLog(meltRsvEnd,R->Name,mxoRsv,(void *)R);
          }
        }

      continue;
      } /* END if (MSched.Time > R->EndTime) */

    if (bmisset(&R->Flags,mrfSystemJob))
      {
      MRsvSyncSystemJob(R);
      } /* END if (bmisset(&R->Flags,mrfSystemJob) && (R->J != NULL)) */

    if (MSched.Time < R->StartTime)
      continue;

    /* rsv is active */

    if (bmisset(&R->Flags,mrfIsGlobal) && bmisset(&R->Flags,mrfIsClosed) && !bmisset(&R->Flags,mrfIsActive))
      MSched.SRsvIsActive = TRUE;

    if (!bmisset(&R->Flags,mrfIsActive))
      {
      char EMsg[MMAX_LINE];

      /* rsv is now active */

      bmset(&R->Flags,mrfIsActive);

      if (R->Type != mrtJob)
        {
        /* record admin rsv start event */

        if (!bmisset(&R->Flags,mrfWasActive))
          {
          if (bmisset(&R->Flags,mrfEvacVMs))
            MNodeEvacVMsMultiple(&R->NL);

          MOWriteEvent((void *)R,mxoRsv,mrelRsvStart,NULL,MStat.eventfp,NULL);

          CreateAndSendEventLog(meltRsvStart,R->Name,mxoRsv,(void *)R);
          }
        else
          bmunset(&R->Flags,mrfWasActive);
        }

      MOReportEvent((void *)R,R->Name,mxoRsv,mttStart,MSched.Time,FALSE);

      MRsvCheckAdaptiveState(R,EMsg);

      if (bmisset(&R->Flags,mrfSystemJob))
        {
        MRsvSyncSystemJob(R);
        } /* END if (bmisset(&R->Flags,mrfSystemJob)) */
      }   /* END if (!bmisset(&R->Flags,mrfIsActive)) */

    /* NOTE:  when a reservation is created from the checkpoint file it builds a
              reservation event table on the nodes with BRes.Procs == 0.  This happens
              because we build the RE table before we have node information from the
              resource managers.  At this point in the code the nodes have been loaded
              from the resource managers so we need to rebuild them all.  FIXME */

    if (MSched.Iteration == 0)
      {
      if ((R->AllocPC == 0) && (R->DRes.Procs != 0))
        {
        MNLGetAttr(&R->NL,mrProc,&R->AllocPC,mdfInt);
        }

#if 0
      for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
        {
        N = MNode[nindex];

        if ((N == NULL) || (N->Name[0] == '\0'))
          break;

        if (N->Name[0] == '\1')
          continue;

        N->RE = MRESort(N->RE);
        }
#endif /* 0 */
      }
    }      /* END while (MRsvTableIterate()) */
  
  /* transition all reservations */

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    MRsvTransition(R,FALSE);
    } /* END while (MRsvTableIterate()) */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    MNodeBuildRE(N);
    N->RE = MRESort(N->RE);

    for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
      {
      if (RE->Time > (long)MSched.Time)
        break;

      if (RE->Type != mreStart)
        continue;

      R = MREGetRsv(RE);

      if (R == NULL)
        continue;

      if (R->Type == mrtJob)
        continue;

      if (MSched.Time > R->EndTime)
        continue;

      RC = RE->TC;

      PC = RC * ((R->DRes.Procs != -1) ? R->DRes.Procs : N->CRes.Procs);

      BR = &RE->BRes;  /* total resources dedicated */

      BPC = (BR->Procs == -1) ? N->CRes.Procs : BR->Procs;

      MDB(9,fSTRUCT) MLog("INFO:     checking utilization for MNode[%03d] '%s'\n",
        nindex,
        N->Name);

      MDB(5,fSTRUCT) MLog("INFO:     updating usage stats for reservation %s on node '%s'\n",
        R->Name,
        N->Name);

      IPS = (interval * BPC);

      R->CAPS += (interval * (PC - BPC));
      R->CIPS += IPS;

      switch (MPar[0].FSC.FSPolicy)
        {
        case mfspDPES:

          {
          double PEC;

          MResGetPE(BR,&MPar[0],&PEC);

          fsusage = interval * PEC;
          }  /* END BLOCK */

          break;

        case mfspUPS:

          fsusage = 0.0;

          break;

        case mfspDPS:
        default:

          fsusage = IPS;

          break;
        }  /* END switch (FS[0].FSPolicy) */

      if (R->A != NULL)
        R->A->F.FSUsage[0] += fsusage;

      if (R->G != NULL)
        R->G->F.FSUsage[0] += fsusage;

      if (R->U != NULL)
        R->U->F.FSUsage[0] += fsusage;
      }    /* END for reindex */
    }      /* END for nindex  */

  return(SUCCESS);
  }  /* END MRsvUpdateStats() */





/**
 * Update allocation charges associated w/reservation.
 *
 * NOTE:  Update allocation charges on all reservations or specified rsv if
 *        RS is specified.
 *
 * NOTE:  Will only return FAILURE if RS is specified.
 *
 * @see MRsvDestroy() - parent - charge rsv upon rsv completion
 * @see MSchedUpdateStats() - perform periodic charge 'flush'
 * @see MAMHandleUpdate() - child
 *
 * @param RS                  (I) [optional]
 * @param DestroyRsvOnFailure (I)
 * @param EMsg                (O) [optional,minsize=MMAX_LINE]
 */

int MRsvChargeAllocation(

  mrsv_t  *RS,
  mbool_t  DestroyRsvOnFailure,
  char    *EMsg)

  {
  rsv_iter RTI;

  enum MS3CodeDecadeEnum S3C;

  mrsv_t *R;

  int     rc;

  const char *FName = "MRsvChargeAllocation";

  MDB(3,fSTAT) MLog("%s(%s,%s,EMsg)\n",
    FName,
    (RS != NULL) ? RS->Name : "NULL",
    MBool[DestroyRsvOnFailure]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (((MSched.Shutdown == TRUE) || (MSched.Recycle == TRUE)) &&
       (MAM[0].FlushPeriod != mpNONE))
    {
    /* do not flush allocation charges if AM flush period is set and
       Moab is shutting down */

    return(SUCCESS);
    }

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((RS != NULL) && (R != RS))
      continue;

    if (R->Type == mrtJob)
      continue;

    if ((R->A == NULL) || !strcmp(R->A->Name,NONE))
      continue;

    if (DestroyRsvOnFailure == MBNOTSET)
      DestroyRsvOnFailure = TRUE;

    if ((R->AllocResPending == TRUE) && (R->A != NULL))
      {
      /* The only path here is from an existing reservation restored from
         checkpoint and this is essentially retroactively determining whether
         this reservation should have been revivified */
      if (MAMHandleCreate(
               &MAM[0],
               (void *)R,
               mxoRsv,
               NULL,
               EMsg,
               &S3C) == FAILURE)
        {
        MDB(1,fSCHED) MLog("WARNING:  Unable to register reservation creation with accounting manager for %d procs for reservation %s\n",
          R->AllocPC,
          R->Name);

        if (S3C == ms3cNoFunds)
          {
          if (DestroyRsvOnFailure == TRUE)
            MRsvDestroy(&R,TRUE,TRUE);
          }

        if (RS != NULL)
          {
          if ((EMsg != NULL) && (EMsg[0] == '\0'))
            sprintf(EMsg,"Unable to register reservation creation with accounting manager");

          return(FAILURE);
          }

        continue;
        }

      R->AllocResPending = FALSE;

      continue;
      }  /* END if (R->AllocResPending == TRUE) */

    if (R->StartTime > MSched.Time)
      {
      /* reservation in future */

      continue;
      }

    /* MRsvChargeAllocation() is called when scheduler is shutting down
       but we don't want to charge in this case */
    if ((MSched.State == mssmShutdown) &&
        (MAM[0].Type == mamtNative))
      {
      continue;
      }

    /* Periodic charge and lien */

    rc = MAMHandleUpdate(&MAM[0],(void *)R,mxoRsv,NULL,EMsg,&S3C);

    /* Update reservation cycle tallies */
    R->TIPS += R->CIPS;
    R->TAPS += R->CAPS;
    R->CIPS = 0.0;
    R->CAPS = 0.0;

    if (rc == FAILURE)
      {
      MDB(2,fSCHED) MLog("WARNING:  Unable to register reservation update with accounting manager for %6.2f PS for reservation %s\n",
        R->CIPS,
        R->Name);

      if (S3C == ms3cNoFunds)
        {
        if (DestroyRsvOnFailure == TRUE)
          MRsvDestroy(&R,TRUE,TRUE);
        }

      if (RS != NULL)
        {
        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          sprintf(EMsg,"Unable to register reservation update with accounting manager");

        return(FAILURE);
        }
      }

    continue;
    }    /* END while (MRsvTableIterate()) */

  return(SUCCESS);
  }  /* END MRsvChargeAllocation() */




/**
 * Update rsv allocation and class availability.
 *
 * @param N (I) [modified]
 * @param UpdateRsv (I)
 * @param UpdateClass (I)
 */

int MNodeUpdateResExpression(

  mnode_t *N,            /* I (modified) */
  mbool_t  UpdateRsv,    /* I */
  mbool_t  UpdateClass)  /* I */

  {
  rsv_iter RTI;

  int    srindex;

  char  *tmpHostExp = NULL;

  int    TC;

  char   Buffer[MMAX_LINE];

  /* NOTE:  When restoring from checkpoint, Moab stores the previously allocated
            nodelist in R->ReqNL.  This is then converted to a hostexp in 
            MRsvLoadCP and dropped.  When HostExp is populated with CLASS:<class>
            the R->ReqNL is not dropped.  R->ReqNL has the previously allocated
            nodelist but the TC is wrong.  Use the ClassExpression to ignore
            the TC.  We should add a check to make sure N is part of the
            nodelist (NYI) */

  mbool_t ClassExpression;

  mrsv_t *R;

  const char *FName = "MNodeUpdateResExpression";

  MDB(4,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    MBool[UpdateRsv],
    MBool[UpdateClass]);

  /* NOTE:  empty standing reservations should be supported */

  if (N == NULL)
    {
    return(FAILURE);
    }

  if (UpdateClass == TRUE)
    {
    int        cindex;

    mclass_t  *C;

    /* if configured classes not specified, set configured classes
       based on class hostlist info */

    /* examine classes */

    for (cindex = 1;cindex < MSched.M[mxoClass];cindex++)
      {
      C = &MClass[cindex];

      if (C->Name[0] == '\0')
        break;

      strcpy(Buffer,N->Name);

      if (C->HostExp != NULL)
        {
        MUStrDup(&tmpHostExp,C->HostExp);

        if (MUREToList(
              tmpHostExp,
              mxoNode,
              NULL,
              NULL,
              FALSE,
              Buffer,
              sizeof(Buffer)) == SUCCESS)
          {
          bmset(&N->Classes,C->Index);
          MUFree(&tmpHostExp);

          continue;
          }
   
        MUFree(&tmpHostExp);
        }

      /* C->NodeList may not be fully populated (with this node) as that doesn't happen until the
         end of MRMClusterQuery() on the iteration this node is first discovered */

      if (!MNLIsEmpty(&C->NodeList))
        {
        if (MNLFind(&C->NodeList,N,NULL) == SUCCESS)
          {
          /* add class to node */

          bmset(&N->Classes,C->Index);
          }    /* END for (cindex) */
        }
      }      /* END if (N->CRes.PSlot[0].count == 0) */
    }        /* END if (UpdateClass == TRUE) */

  if (UpdateRsv == TRUE)
    {
    MRsvIterInit(&RTI);

    while (MRsvTableIterate(&RTI,&R) == SUCCESS)
      {
      char TString[MMAX_LINE];

      if (R->StartTime == 0)
        continue;

      if ((R->Type == mrtJob) ||
          (R->HostExp == NULL) ||
          (R->HostExpIsSpecified == FALSE))
        {
        /* NOTE:  added check if host expression is specified Aug 28, 2006,
                  this may break something */

        /* R->HostExpIsSpecified == MBNOTSET means use HostExp 1st iteration and then
           get rid of it on 2nd iteration */

        continue;
        }

      MULToTString(R->EndTime - MSched.Time,TString);

      MDB(5,fSTRUCT) MLog("INFO:     checking Rsv '%s'  end: %s\n",
        R->Name,
        TString);

      if ((R->ReqTC > 0) && (R->AllocTC >= R->ReqTC))
        {
        continue;
        }

      /* verify node is not already connected to reservation */

      if (MREFindRsv(N->RE,R,NULL) == SUCCESS)
        {
        /* reservation already exists on node */

        continue;
        }

      /* verify reservation resource constraints */

      if (!bmisclear(&R->ReqFBM))
        {
        if (MAttrSubset(&N->FBM,&R->ReqFBM,R->ReqFMode) != SUCCESS)
          {
          /* node does not possess required features */

          continue;
          }
        }  /* END if (R->ReqFBM != NULL) */

      strcpy(Buffer,N->Name);

      /* NOTE:  should refactor to look at all nodes added during current iteration
                and check all new nodes at the same time.  Current eval per node
                per rsv is not scalable (NYI) */

      MUStrDup(&tmpHostExp,R->HostExp);

      ClassExpression = FALSE;

      if (!strncasecmp(tmpHostExp,"class:",strlen("class:")))
        {
        if ((ClassExpression = MNodeHasClassInExpression(tmpHostExp,N)) == FALSE)
          {
          MUFree(&tmpHostExp);
          continue;
          }
        }
      else if (MUREToList(
           tmpHostExp,
           mxoNode,
           (R->PtIndex > 0) ? &MPar[R->PtIndex] : NULL,
           NULL,
           FALSE,
           Buffer,
           sizeof(Buffer)) == FAILURE)
        {
        MDB(7,fSTRUCT) MLog("INFO:     cannot expand regex '%s' to check node '%s'\n",
          R->HostExp,
          N->Name);

        MUFree(&tmpHostExp);
        continue;
        }

      MUFree(&tmpHostExp);

      if ((R->PtIndex > 0) && (R->PtIndex != N->PtIndex))
        {
        /* 1-7-09 BOC for NOAA (RT3626) */

        /* Scenario: class has two partions A and B. A has fewer nodes than B. If 
         * the reservation requests more nodes than A has the reservation should go 
         * to B (with REQFULL set). On restart and in this routine there is no check for how many nodes A 
         * and B have, and thus nodes are just assigned to nodes starting with A and 
         * overflowing onto B. The check above prevents nodes from going to other partitions other 
         * than their originally assigned partition on restart. */

        continue;
        }

      /* node belongs to reservation */

      TC = MNodeGetTC(
          N,
          &N->CRes,
          &N->CRes,
          &N->DRes,
          &R->DRes,
          MMAX_TIME,
          1,
          NULL);

      TC = MAX(TC,1);

      if (R->ReqTC > 0)
        TC = MIN(TC,R->ReqTC - R->AllocTC);

      /* used to reserve subsets of an SMP node */

      /* NOTE:  Old behavior = RESOURCES=PROCS=4 on smp node resulted in one task of four processors */
      /*        New behavior = RESOURCES=PROCS=4 on smp node results in as many tasks on node as supported */
      /*        To fine grain the amount allocated use TASKCOUNT */

      /* search R->ReqNL for TC info (if available) */

      if ((MSched.UseCPRsvNodeList == TRUE) && (!MNLIsEmpty(&R->ReqNL)))
        {
        /* this parameter is used by VLP */

        mnode_t *tmpN;

        int rnindex;

        for (rnindex = 0;MNLGetNodeAtIndex(&R->ReqNL,rnindex,&tmpN) == SUCCESS;rnindex++)
          {
          if (tmpN == N)
            {
            if (ClassExpression == FALSE)
              TC = MNLGetTCAtIndex(&R->ReqNL,rnindex);

            break;
            }
          }

        if (tmpN != N)
          continue;
        }
      else if (!MNLIsEmpty(&R->ReqNL))
        {
        mnode_t *tmpN;

        if (ClassExpression == FALSE)
          {
          int rnindex;

          for (rnindex = 0;MNLGetNodeAtIndex(&R->ReqNL,rnindex,&tmpN) == SUCCESS;rnindex++)
            {
            if (tmpN == N)
              {
              /* when coming up from checkpoint R->ReqNL will be populated 
                 with the right nodes, but not the right node taskcounts */

              /* doug is not sure when R->ReqNL contains the correct 
                 taskcount and is going to wait for a case where it does 
                 and decide what to do then */

              TC = TC;
              }
            }
          }
        }
      else if (R->ReqTC > 0)
        {
        TC = MIN(TC,R->ReqTC);
        }

      /*
      if ((R->DRes.Procs > 1) && (R->ReqTC == 0))
        {
        TC = MIN(TC,R->DRes.Procs);
        }
      */

      /* find first available reservation slot on node */

      /* TC is a positive number */

      /* warning, this routine can overcommit a node, see RT5663 */

      if (MRsvAdjustNode(R,N,TC,0,FALSE) == FAILURE)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot add node %s to reservation %s\n",
          N->Name,
          R->Name);

        continue;
        }

      R->AllocNC ++;

      R->AllocTC += TC;

      R->AllocPC += (R->DRes.Procs == -1) ?
        N->CRes.Procs :
        (TC * R->DRes.Procs);

      MNLAddNode(&R->NL,N,TC,TRUE,NULL);

      /* These lines overwrite what was coming from the checkpoint file.
         They essentially kill reservation partitions. */

      /* removed 8/13/08 by JG */

      /*
      if (R->PtIndex == -1)
        R->PtIndex = N->PtIndex;
      else if (R->PtIndex != N->PtIndex)
        R->PtIndex = 0;
      */
      }      /* END while (MRsvTableIterate()) */

    /* NOTE:  examine all empty standing reservations */

    for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
      {
      msrsv_t   *SR;
      int        nindex;
      mnl_t     *HL;
      mnode_t   *tmpN;

      SR = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);

      if (SR == NULL)
        break;

      if (SR->Name[0] == '\0')
        break;

      if (SR->Name[0] == '\1')
        continue;

      if (SR->R[0] != NULL)
        continue;

      if (MNLIsEmpty(&SR->HostList))
        {
        continue;
        }

      HL = &SR->HostList;

      /* rebuild SR if N in SR hostlist */

      for (nindex = 0;MNLGetNodeAtIndex(HL,nindex,&tmpN);nindex++)
        {
        if (tmpN == N)
          {
          /* node included in SR hostlist */

          MSRPeriodRefresh(SR,NULL);

          break;
          }
        }  /* END for (nindex) */
      }    /* END for (srindex) */
    }      /* END if (UpdateRsv == TRUE) */

  return(SUCCESS);
  }  /* END MNodeUpdateResExpression() */



/**
 * Checks if reservation is the requsted reservation.
 *
 * @param R (I)
 * @param ReqR (I)
 */

mbool_t MRsvIsReqRID(

  const mrsv_t *R,
  char         *ReqR)

  {
  if ((R->RsvGroup != NULL) &&
      !strncmp(R->RsvGroup,ReqR,strlen(R->RsvGroup)) &&
      (strlen(ReqR) <= strlen(R->RsvGroup)))
    {
    MDBO(8,R,fSTRUCT) MLog("INFO:     inclusive ('ReqRID' rsv group)\n");
    }
  else if (!strcmp(R->Name,ReqR))
    {
    MDBO(8,R,fSTRUCT) MLog("INFO:     inclusive ('ReqRID' rsv id)\n");
    }
  else
    {
    MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (not 'ReqRID' reservation)\n");

    return(FALSE);
    }

  return(TRUE);
  }  /* END MRsvIsReqRID() */



/**
 *
 *
 * @param R (I) [modified]
 */

int MRsvPreempt(

  mrsv_t *R)  /* I (modified) */

  {
  long      MinPriority;

  rsv_iter RTI;

  mrsv_t   *PR;
  mrsv_t   *MinPR;

  const char *FName = "MRsvPreempt";

  MDB(3,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (R == NULL)
    {
    return(FAILURE);
    }

  MinPriority = 0xffffffff;

  MinPR = NULL;

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&PR) == SUCCESS)
    {
    if (!bmisset(&PR->Flags,mrfPreemptible))
      continue;

    if (PR->Priority >= MIN(R->Priority,MinPriority))
      continue;

    MinPR = PR;

    MinPriority = PR->Priority;
    }  /* END while (MRsvTableIterate()) */

  if ((long)MinPriority >= R->Priority)
    {
    return(FAILURE);
    }

  if (MinPR == NULL)
    return(FAILURE);
  else
    MRsvDestroy(&MinPR,TRUE,TRUE);

  return(SUCCESS);
  }  /* END MRsvPreempt() */


/**
 * Initialize a rsv table iterator.
 *
 * @param RTI
 */

int MRsvIterInit(

  rsv_iter *RTI)

  {
  *RTI = MRsvHT.begin();

  return(SUCCESS);
  }  /* END MRsvIterInit() */



/**
 * Iterate over the rsv table.
 * 
 * NOTE: this routine runs the normal checks to make sure the rsv is valid.
 *
 * @param RTIP (I) - must already be initialized
 * @param RP   (O) - valid rsv or NULL
 */

int MRsvTableIterate(

  rsv_iter   *RTIP,
  mrsv_t    **RP)

  {
  rsv_iter RTI;

  if (RP != NULL)
    *RP = NULL;

  if ((RP == NULL) || (RTIP == NULL))
    {
    return(FAILURE);
    }

  if (MRsvHT.size() == 0)
    return(FAILURE);

  RTI = *RTIP;

  if (RTI == MRsvHT.end())
    return(FAILURE);

  *RP = (*RTI).second;

  while (++RTI != MRsvHT.end())
    {
    if (MRsvIsValid((*RTI).second) == FALSE)
      {
      continue;
      }

    /* as soon as we find a valid rsv then break out and return it */

    break;
    }

  *RTIP = RTI;

  return(SUCCESS);
  }  /* END MRsvTableIterate() */


int MRsvTableFind(

  const char *Name,
  mrsv_t    **RP)

  {
  rsv_iter RTI;

  if (RP != NULL)
    *RP = NULL;

  if (MUStrIsEmpty(Name))
    {
    return(FAILURE);
    }

  RTI = MRsvHT.find(Name);

  if (RTI == MRsvHT.end())
    {
    return(FAILURE);
    }

  if (RP != NULL)
    *RP = (*RTI).second;

  return(SUCCESS);
  }  /* END MRsvTableFind() */


int MRsvTableInsert(

  const char *Name,
  mrsv_t     *R)

  {
  std::string RName;

  if ((MUStrIsEmpty(Name) || (R == NULL)))
    {
    return(FAILURE);
    }

  RName = Name;

  MRsvHT.insert(std::pair<std::string,mrsv_t *>(RName,R));

  return(SUCCESS);
  }  /* END MRsvTableInsert() */




int MRsvFreeTable(void)

  {
  rsv_iter RTI;

  while (MRsvHT.size() != 0)
    {
    RTI = MRsvHT.begin();

    MRsvDestroy(&(*RTI).second,TRUE,TRUE);
    }

  MRsvHT.clear();

  return(SUCCESS);
  }  /* END MRsvFreeTable() */






#if 0
/**
 * Iterate over the global reservation table.
 *
 * @param HTI (already initialized)
 * @param RP (populated on SUCCESS, NULL on FAILURE)
 */

int MRsvTableIterate(

  rsv_iter  *RTI,
  mrsv_t   **RP)

  {
  mrsv_t *R = NULL;

  if ((RTI == NULL) || (RP == NULL))
    return(FAILURE);
  
  *RP = NULL;

  if (MUHTIterate(&MRsvHT,NULL,(void **)&R,NULL,HTI) == FAILURE)
    return(FAILURE);

  if (MRsvIsValid(R) == FALSE)
    return(FAILURE);

  *RP = R;

  return(SUCCESS);
  }  /* END MRsvTableIterate() */


/**
 * Free MRsv (reservation) table.
 */

int MRsvFreeTable()

  {
  mhashiter_t HTI;

  mrsv_t *R;

  MUHTIterInit(&HTI);

  while (MRsvTableIterate(&HTI,&R) == SUCCESS)
    {
    MRsvDestroy(&R,TRUE,TRUE);

    MUFree((char **)&R);
    }  /* END while (MRsvTableIterate()) */

  return(SUCCESS);
  }  /* END MRsvFreeTable() */



#endif



/**
 * Return TRUE if the given reservation pointer is valid.
 *
 * @param R
 */

mbool_t MRsvIsValid(

  mrsv_t *R)

  {
  if (R == NULL)
    return(FALSE);

  if (MUStrIsEmpty(R->Name))
    return(FALSE);

  if (R == (mrsv_t *)1)
    return(FALSE);
 
  if (R->Name[0] == '\1')
    return(FALSE);

#if 0 /* we could enable this is if we wanted to be extra sure */
  if (MRsvFind(R->Name,NULL,mraNONE) == FAILURE)
    return(FALSE);
#endif

  return(TRUE);
  }  /* END MRsvIsValid() */





/**
 * Release allocated resources from reservation.
 *
 * NOTE:  clear associated entries from N->R[], and N->RE[] tables
 *
 * @param R (I) [modified]
 * @param NL The node list from which to remove the reservation. (currently ignored)
 * @param RebuildNodeRE (I) if TRUE, patch rsv holes
 */

int MRsvDeallocateResources(

  mrsv_t *R,             
  mnl_t  *NL)

  {
  int nindex;

  mnode_t *N;

  const char *FName = "MRsvDeallocateResources";

  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (NL != NULL) ? "NL" : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if ((R == NULL) || (NL == NULL))
    {
    return(FAILURE);
    }

  /* There is a bug somewhere in the code that causes job reservations to leak
     onto other nodes.  This mostly happens with reservations on the GLOBAL node
     or shared partitions.  To address this bug we need to walk all the nodes in
     the cluster when we tear down an RSV.  This is inefficient and wasteful but
     the only way to guarantee that the reservations have been properly cleaned 
     up. */

  /* walk all nodes, remove node R, and RE entries */

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
#if 0 /* old, unoptimized behavior */
  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;
#endif

    MDB(5,fSTRUCT) MLog("INFO:     removing reservation %s from node %s\n",
      R->Name,
      N->Name);

    MRERelease(&N->RE,R);

    MNodeBuildRE(N);
    }      /* END for (nindex)           */

  R->AllocNC = 0;
  R->AllocTC = 0;
  R->AllocPC = 0;

  MRsvTableDumpToLog(__FUNCTION__,__LINE__);

  return(SUCCESS);
  }  /* END MRsvDeallocateResources() */





/**
 * Initialize reservation, apply rsv profile if specified
 *
 * NOTE:  All dynamic rsv attributes must be explicitly handled if rsv template
 *        is specified.
 *
 * @param RP                     (I) [modified]
 * @param RProf                  (I) [optional]
 * @param ApplyProfResourcesOnly (I)
 */

int MUIRsvInitialize(

  mrsv_t **RP,
  mrsv_t  *RProf,
  mbool_t  ApplyProfResourcesOnly)

  {
  mrsv_t *R;

  if (RP == NULL)
    {
    return(FAILURE);
    }

  MRsvInitialize(RP);

  R = *RP;

  if (RProf != NULL)
    {
    MRsvFromProfile(R,RProf);

    if (ApplyProfResourcesOnly == TRUE)
      {
      /* If only applying resources, remove triggers */

      if (R->T != NULL)
        {
        MTListClearObject(R->T,R->CancelIsPending);

        MUArrayListFree(R->T);

        MUFree((char **)&R->T);
        }  /* END if (R->T != NULL) */
      }    /* END if (ApplyProfResourcesOnly == FALSE) */
    }  /* END if (RProf != NULL) */

  return(SUCCESS);
  }  /* END MRsvInitialize() */






/**
 * Populate Rsv 'R' with profile attributes.
 *
 * NOTE: will not overwrite a value if it already exists.
 *
 * @see MRsvInitialize() - peer
 *
 * @param R (I) [modified]
 * @param RProf (I) [optional]
 */

int MRsvFromProfile(

  mrsv_t  *R,    /* I (modified) */
  mrsv_t  *RProf) /* I (optional) */

  {
  mbitmap_t tmpRFlags;

  if ((R == NULL) || (RProf == NULL))
    {
    return(FAILURE);
    }

  MUStrDup(&R->Profile,RProf->Name);

  if (RProf->Label != NULL)
    MUStrDup(&R->Label,RProf->Label);

  if (RProf->RsvGroup != NULL)
    MUStrDup(&R->RsvGroup,RProf->RsvGroup);

  if (RProf->SpecName != NULL)
    MUStrDup(&R->SpecName,RProf->SpecName);

  if (RProf->Comment != NULL)
    MUStrDup(&R->Comment,RProf->Comment);

  if (RProf->NodeSetPolicy != NULL)
    MUStrDup(&R->NodeSetPolicy,RProf->NodeSetPolicy);

  if (RProf->HostExp != NULL)
    {
    MRsvSetAttr(R,mraHostExp,(void *)RProf->HostExp,mdfString,mSet);

    R->HostExpIsSpecified = TRUE;
    }

  bmcopy(&tmpRFlags,&R->Flags);

  if (!bmisclear(&RProf->Flags))
    {
    /* NOTE:  why is R->Flags cleared? - does profile contain
              'set' attributes or 'default' attributes. */

    bmclear(&R->Flags);

    MRsvSetAttr(R,mraFlags,&RProf->Flags,mdfOther,mAdd);

    if (bmisset(&tmpRFlags,mrfApplyProfResources) &&
        bmisset(&RProf->Flags,mrfSystemJob))
      {
      /* do not apply system job flag from profile */

      bmunset(&R->Flags,mrfSystemJob);
      }
    }    /* END if (!bmisclear(&RProf->Flags)) */

  if (!bmisclear(&RProf->ReqFBM))
    {
    /* alloc feature requirements */

    bmclear(&R->ReqFBM);

    MRsvSetAttr(R,mraReqFeatureList,(void *)&RProf->ReqFBM,mdfOther,mSet);
    }

  if ((RProf->T != NULL) && (!bmisset(&tmpRFlags,mrfApplyProfResources)))
    {
    MTListCopy(RProf->T,&R->T,TRUE);
    }

  if (RProf->RsvAList != NULL)
    {
    int index = 0;

    /* NOTE: this overrides existing RsvAList */

    if (R->RsvAList != NULL)
      {
      for (index = 0;index < MMAX_PJOB;index++)
        {
        if (R->RsvAList[index] == NULL)
          break;
   
        MUFree(&R->RsvAList[index]);
        }
      }
   
    R->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

    while (RProf->RsvAList[index] != NULL)
      {
      if (index >= MMAX_PJOB)
        break;

      MUStrDup(&R->RsvAList[index],RProf->RsvAList[index]);

      index++;
      } 
    }   /* END if (RProf->RsvAList != NULL) */

  if (RProf->SysJobTemplate != NULL)
    MUStrDup(&R->SysJobTemplate,RProf->SysJobTemplate);

  if (!MACLIsEmpty(RProf->ACL))
    MACLCopy(&R->ACL,RProf->ACL);

  if (!MACLIsEmpty(RProf->CL))
    MACLCopy(&R->CL,RProf->CL);

  if (RProf->ReqOS > 0)
    R->ReqOS = RProf->ReqOS;
  if (RProf->ReqNC > 0)
    R->ReqNC = RProf->ReqNC;
  if (RProf->ReqTC > 0)
    R->ReqTC = RProf->ReqTC;

  if (RProf->MinReqTC > 0)
    R->MinReqTC = RProf->MinReqTC;
  if (RProf->ReqTPN > 0)
    R->ReqTPN   = RProf->ReqTPN;
  if (RProf->ReqArch > 0)
    R->ReqArch  = RProf->ReqArch;
  if (RProf->ReqMemory > 0)
    R->ReqMemory = RProf->ReqMemory;

  if (RProf->StartTime > 0)
    R->StartTime = RProf->StartTime;
  if (RProf->EndTime > 0)
    R->EndTime   = RProf->EndTime;

  if (RProf->Duration > 0)
    R->Duration  = RProf->Duration;
  if (RProf->ExpireTime > 0)
    R->ExpireTime = RProf->ExpireTime;

  if (RProf->U != NULL)
    R->U = RProf->U;
  if (RProf->G != NULL)
    R->G = RProf->G;
  if (RProf->A != NULL)
    R->A = RProf->A;
  if (RProf->Q != NULL)
    R->Q = RProf->Q;


  R->AllocResPending = RProf->AllocResPending;
  R->LienCost = RProf->LienCost;

  if (RProf->SystemID != NULL)
    MUStrDup(&R->SystemID,RProf->SystemID);

  if (RProf->SystemRID != NULL)
    MUStrDup(&R->SystemRID,RProf->SystemRID);

  R->EnforceHostExp = RProf->EnforceHostExp;
  R->IsTracked = RProf->IsTracked;

  R->MaxJob = RProf->MaxJob;

  R->VMUsagePolicy = RProf->VMUsagePolicy;

  R->P = RProf;

  return(SUCCESS);
  }  /* END MRsvFromProfile() */





/**
 * Set reservation start/end timeframe.
 *
 * WARNING: this routine will not work in arbitrary order
 *
 * @param R      (I) [modified]
 * @param AIndex (I)
 * @param Mode   (I)
 * @param Value  (I)
 * @param EMsg   (O)
 */

int MRsvSetTimeframe(

  mrsv_t                  *R,
  enum MRsvAttrEnum        AIndex,
  enum MObjectSetModeEnum  Mode,
  long                     Value,
  char                    *EMsg)

  {
  mulong     tmpStartTime = 0;
  long       tmpDuration  = 0;

  /* adjust rsv timeframe */

  if (AIndex == mraEndTime)
    {
    long OldTime = R->EndTime;

    if (Mode == mAdd)
      R->EndTime += Value;
    else if (Mode == mDecr)
      R->EndTime -= Value;
    else
      R->EndTime = Value;

    /* Check that end is not negative or before start*/

    if ((R->EndTime < R->StartTime) || (Value < 0))
      {
      R->EndTime = OldTime;

      MUStrCpy(EMsg,"ERROR:  invalid endtime specified",MMAX_LINE);

      return(FAILURE);
      }

    tmpStartTime = R->StartTime;
    tmpDuration  = R->EndTime - tmpStartTime;
    }
  else if (AIndex == mraStartTime)
    {
    /* NOTE: If the starttime is greater than the endtime, then
             the endtime is moved back, keeping the same duration */

    if (Mode == mAdd)
      tmpStartTime = R->StartTime + Value;
    else if (Mode == mDecr)
      tmpStartTime = R->StartTime - Value;
    else
      tmpStartTime = Value;

    if ((R->StartTime > 0) && (R->EndTime > 0))
      tmpDuration = R->EndTime - R->StartTime;
    else if ((R->StartTime == 0) && (R->EndTime > 0) && (tmpStartTime > 0))
      tmpDuration = R->EndTime - tmpStartTime;
    }
  else  /* set duration */
    {
    tmpStartTime = R->StartTime;

    tmpDuration = (long)(R->EndTime - R->StartTime);

    if (Mode == mAdd)
      tmpDuration += Value;
    else if (Mode == mDecr)
      tmpDuration -= Value;
    else
      tmpDuration = Value;

    if ((long)tmpDuration < 0)
      {
      MUStrCpy(EMsg,"ERROR:  duration cannot be negative",MMAX_LINE);

      return(FAILURE);
      }
    }

  /* Check to see if the change will place endtime in the past*/

  if ((unsigned long)(tmpStartTime + tmpDuration) < MSched.Time)
    {
    MUStrCpy(EMsg,"ERROR:  modification puts reservation endtime in the past (remove reservation?)", 
      MMAX_LINE);

    return(FAILURE);
    }

  R->StartTime = tmpStartTime;

  if (tmpDuration > 0)
    R->EndTime = (mulong)(tmpStartTime + tmpDuration);

  return(SUCCESS);
  }  /* END MRsvSetTimeframe() */





/**
 *
 *
 * @param SBuffer (O)
 * @param SBufSize (I)
 * @param Mode (I) [not used]
 */

int MRsvDiagGrid(

  char *SBuffer,  /* O */
  int   SBufSize, /* I */
  int   Mode)     /* I (not used) */

  {
  int rindex;

  int   len;

  char *BPtr;
  int   BSpace;

  char  TimeLine[MMAX_LINE];

  const char *FName = "MRsvDiagGrid";

  MDB(5,fSTRUCT) MLog("%s(SBuffer,%d,%d)\n",
    FName,
    SBufSize,
    Mode);

  if (SBuffer == NULL)
    {
    return(FAILURE);
    }

  len = strlen(SBuffer);

  BPtr   = SBuffer + len;
  BSpace = SBufSize - len;

  if (MPar[0].MaxMetaTasks > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"\nMeta Tasks\n\n");

    MUSNPrintF(&BPtr,&BSpace,"%5s  %10s -> %10s\n",
      "Tasks",
      "StartTime",
      "EndTime");

    MUSNPrintF(&BPtr,&BSpace,"%5s  %10s -- %10s\n",
      "-----",
      "----------",
      "----------");

    if (MRange[0].ETime == 0)
      {
      char TString[MMAX_LINE];

      MULToTString(0,TString);

      strcpy(TimeLine,TString);

      MULToTString(MMAX_TIME,TString);

      MUSNPrintF(&BPtr,&BSpace,"%5d  %10s -> %10s\n",
        0,
        TimeLine,
        TString);
      }
    else
      {
      char TString[MMAX_LINE];

      for (rindex = 0;rindex < MMAX_RANGE;rindex++)
        {
        if (MRange[rindex].ETime == 0)
          break;

        MULToTString(MRange[rindex].STime - MSched.Time,TString);

        strcpy(TimeLine,TString);

        MULToTString(MRange[rindex].ETime - MSched.Time,TString);

        MUSNPrintF(&BPtr,&BSpace,"%5d  %10s -> %10s\n",
          MRange[rindex].TC,
          TimeLine,
          TString);
        }  /* END for (rindex)                  */
      }    /* END else (MRange[0].ETime == 0) */
    }      /* END if (MPar[0].MaxMetaTasks > 0) */

  return(SUCCESS);
  }  /* END MRsvDiagGrid() */



/**
 *
 *
 * @param SpecName (I)
 * @param Index (I) [optional]
 * @param ACL (I) [optional]
 * @param URID (O)
 */

int MRsvGetUID(

  char   *SpecName,  /* I */
  int     Index,     /* I (optional) */
  macl_t *ACL,       /* I (optional) */
  char   *URID)      /* O */

  {
  char *BPtr;
  int   BSpace;

  /* determine/set unique base rsv id */

  if (URID == NULL)
    {
    return(FAILURE);
    }

  BPtr = URID;
  BSpace = MMAX_NAME;

  BPtr[0] = '\0';

  if ((SpecName != NULL) && (SpecName[0] != '\0'))
    {
    /* rsv id specified */

    MUSNPrintF(&BPtr,&BSpace,"%s",
      SpecName);
    }
  else if (!MACLIsEmpty(ACL))
    {
    macl_t *tmpACL = ACL;

    /* use rsv acl id */

    while (tmpACL != NULL)
      {
      if ((tmpACL->Type == maUser) ||
          (tmpACL->Type == maGroup) ||
          (tmpACL->Type == maAcct) ||
          (tmpACL->Type == maClass) ||
          (tmpACL->Type == maQOS))
        {
        break;
        }

      tmpACL = tmpACL->Next;
      }  /* END for (aindex) */

    if (tmpACL != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        tmpACL->Name);
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        "system");
      }
    }
  else
    {
    MUSNPrintF(&BPtr,&BSpace,"%s",
      "system");
    }

  /* append index if specified */

  if (Index >= 0)
    {
    MUSNPrintF(&BPtr,&BSpace,".%d",
      Index);
    }

  /* append rsv counter */

  MUSNPrintF(&BPtr,&BSpace,".%d",
    MSched.RsvCounter);

  MSched.RsvCounter++;

  return(SUCCESS);
  }  /* END MRsvGetUID() */




/**
 *
 *
 * @param RName (I)
 */

int MRsvRecycleUID(

  char *RName)  /* I */

  {
  if ((RName == NULL) || (RName[0] == '\0'))
    {
    return(FAILURE);
    }

  MSched.RsvCounter--;

  return(SUCCESS);
  }  /* END MRsvRecycleUID() */






/**
 *
 *
 * @param RProf (I)
 */

int MRsvProfileAdd(

  mrsv_t *RProf)  /* I */

  {
  int rindex;

  if (RProf == NULL)
    {
    return(FAILURE);
    }

  for (rindex = 0;rindex < MMAX_RSVPROFILE;rindex++)
    {
    if (MRsvProf[rindex] != NULL)
      continue;

    MRsvProf[rindex] = RProf;

    MRsvProf[rindex]->MTime = MSched.Time;

    return(SUCCESS);
    }  /* END for (rindex) */

  return(FAILURE);
  }  /* END MRsvProfileAdd() */




/**
 *
 *
 * @param Name (I)
 * @param RProf (O)
 */

int MRsvProfileFind(

  char    *Name,   /* I */
  mrsv_t **RProf)  /* O */

  {
  int rindex;

  if (RProf != NULL)
    *RProf = NULL;

  if ((Name == NULL) || (RProf == NULL))
    {
    return(FAILURE);
    }

  for (rindex = 0;rindex < MMAX_RSVPROFILE;rindex++)
    {
    if (MRsvProf[rindex] == NULL)
      continue;

    if (strcmp(MRsvProf[rindex]->Name,Name))
      continue;

    *RProf = MRsvProf[rindex];

    return(SUCCESS);
    }  /* END for (rindex) */

  return(FAILURE);
  }  /* END MRsvProfileFind() */




/**
 *
 *
 * @param O (I)
 * @param OIndex (I)
 * @param PLevel (I)
 * @param P (I)
 * @param MLimitBM (I)
 * @param Message (O)
 */

int MOCheckLimits(

  void                *O,        /* I */
  enum MXMLOTypeEnum   OIndex,   /* I */
  enum MPolicyTypeEnum PLevel,   /* I */
  mpar_t              *P,        /* I */
  mbitmap_t           *MLimitBM, /* I */
  char                *Message)  /* O (minsize=MMAX_LINE) */

  {
  mpc_t OUsage;

  mrsv_t *R = NULL;
  mjob_t *J = NULL;

  const char *MType[] = {
    "active",
    "idle",
    "system",
    NULL };

  enum MXMLOTypeEnum OList1[] = {
    mxoNONE,
    mxoUser,
    mxoGroup,
    mxoClass,
    mxoALL };

  enum MXMLOTypeEnum OList2[] = {
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoPar,
    mxoALL };

  enum MActivePolicyTypeEnum PList[]  = {
    mptMaxJob,
    mptMaxProc,
    mptMaxNode,
    mptMaxPS,
    mptMaxPE,
    mptMaxWC,
    mptNONE };

  enum MLimitTypeEnum MList[] = {
    mlActive,
    mlIdle,
    mlSystem,
    mlNONE };

  int oindex1;
  int oindex2;

  int pindex;
  int mindex;

  long tmpL;

  mpu_t *OP;
  mpu_t *DP;
  mpu_t *QP;

  mqos_t   *Q;
  mgcred_t *U;
  mgcred_t *G;
  mgcred_t *A;
  mclass_t *C;

  mcredl_t *tL;
  mcredl_t *tDL;

  char *OID;
  char *ONameP;

  /* MLimitBM:  active, system, queue */

  const char *FName = "MOCheckLimits";

  MDB(7,fSTRUCT) MLog("%s(%s,%s,%s,P,%d,Message)\n",
    FName,
    (O != NULL) ? "O" : "NULL",
    MXO[OIndex],
    MPolicyMode[PLevel],
    MLimitBM);

  if (O == NULL)
    {
    return(FAILURE);
    }

  if (Message != NULL)
    Message[0] = '\0';

  if (PLevel == mptOff)
    {
    /* ignore policies */

    return(SUCCESS);
    }

  /* determine resource consumption */

  memset(&OUsage,0,sizeof(OUsage));

  MSNLInit(&OUsage.GRes);

  if (OIndex == mxoRsv)
    {
    R = (mrsv_t *)O;

    ONameP = R->Name;

    MPCFromRsv(R,&OUsage);

    Q = R->Q;
    U = R->U;
    A = R->A;
    G = R->G;
    C = NULL;
    }
  else
    {
    J = (mjob_t *)O;

    ONameP = J->Name;

    MPCFromJob(J,&OUsage);

    Q = J->Credential.Q;
    U = J->Credential.U;
    A = J->Credential.A;
    G = J->Credential.G;
    C = J->Credential.C;
    }

  for (mindex = 0;MList[mindex] != mlNONE;mindex++)
    {
    if (!bmisset(MLimitBM,MList[mindex]))
      continue;

    /* limits: user, group, account, class, override w/QOS */

    /* locate override limits */

    if ((Q != NULL) && (OIndex == mxoJob))
      {
      /* override policies only support for job objects */

      if (MList[mindex] == mlActive)
        QP = Q->L.OverrideActivePolicy;
      else if (MList[mindex] == mlIdle)
        QP = Q->L.OverrideIdlePolicy;
      else if (MList[mindex] == mlSystem)
        QP = Q->L.OverriceJobPolicy;
      else
        QP = NULL;
      }
    else
      {
      QP = NULL;
      }

    for (oindex1 = 0;OList1[oindex1] != mxoALL;oindex1++)
      {
      for (oindex2 = 0;OList2[oindex2] != mxoALL;oindex2++)
        {
        if (((OList2[oindex2] == mxoPar) && (MList[mindex] != mlSystem)) ||
            ((OList2[oindex2] != mxoPar) && (MList[mindex] == mlSystem)))
          {
          if (OIndex != mxoRsv)
            continue;
          }

        OP = NULL;
        DP = NULL;

        OID = NULL;

        tL = NULL;
        tDL = NULL;

        switch (OList2[oindex2])
          {
          case mxoUser:

            if (U != NULL)
              {
              OID = U->Name;

              tL = &U->L;
              }

            if (MSched.DefaultU != NULL)
              tDL = &MSched.DefaultU->L;

            break;

          case mxoGroup:

            if (G != NULL)
              {
              OID = G->Name;

              tL = &G->L;
              }

            if (MSched.DefaultG != NULL)
              tDL = &MSched.DefaultG->L;

            break;

          case mxoAcct:

            if (A != NULL)
              {
              OID = A->Name;

              tL = &A->L;
              }

            if (MSched.DefaultA != NULL)
              tDL = &MSched.DefaultA->L;

            break;

          case mxoQOS:

            if (Q != NULL)
              {
              OID = Q->Name;

              tL = &Q->L;
              }

            if (MSched.DefaultQ != NULL)
              tDL = &MSched.DefaultQ->L;

            break;

          case mxoClass:

            if (C != NULL)
              {
              OID = C->Name;

              tL = &C->L;
              }

            if (MSched.DefaultC != NULL)
              tDL = &MSched.DefaultC->L;

            break;

          case mxoPar:

            if (P != NULL)
              {
              OID = P->Name;

              tL = &P->L;
              }

            tDL = &MPar[0].L;

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (OList2[oindex2]) */

        if (tL != NULL)
          {
          if ((OIndex == mxoRsv) && (OList1[oindex1] == mxoNONE))
            {
            if (MList[mindex] == mlActive)
              OP = tL->RsvActivePolicy;
            else if (MList[mindex] == mlSystem)
              OP = tL->RsvPolicy;
            }
          else
            {
            switch (OList1[oindex1])
              {
              case mxoNONE:

                if (MList[mindex] == mlActive)
                  OP = &tL->ActivePolicy;
                else if (MList[mindex] == mlIdle)
                  OP = tL->IdlePolicy;
                else if (MList[mindex] == mlSystem)
                  OP = tL->JobPolicy;

                break;

              case mxoAcct:

                if ((MList[mindex] == mlActive) &&
                    (tL->AcctActivePolicy != NULL) &&
                    (A != NULL))
                  {
                  MUHTGet(tL->AcctActivePolicy,A->Name,(void **)&OP,NULL);
                  }

                break;


              case mxoGroup:

                if ((MList[mindex] == mlActive) &&
                    (tL->GroupActivePolicy != NULL) &&
                    (G != NULL))
                  {
                  MUHTGet(tL->GroupActivePolicy,G->Name,(void **)&OP,NULL);
                  }

                break;

              case mxoClass:

                if ((MList[mindex] == mlActive) &&
                    (tL->ClassActivePolicy != NULL) &&
                    (C != NULL))
                  {
                  MUHTGet(tL->ClassActivePolicy,C->Name,(void **)&OP,NULL);
                  }

                break;

              case mxoQOS:

                if ((MList[mindex] == mlActive) &&
                    (tL->QOSActivePolicy != NULL) &&
                    (Q != NULL))
                  {
                  MUHTGet(tL->QOSActivePolicy,Q->Name,(void **)&OP,NULL);
                  }

                break;

              case mxoUser:

                if ((MList[mindex] == mlActive) &&
                    (tL->UserActivePolicy != NULL) &&
                    (U != NULL))
                  {
                  MUHTGet(tL->UserActivePolicy,U->Name,(void **)&OP,NULL);
                  }

                break;

              default:

                /* NO-OP */

                break;
              }  /* END switch (OList1[oindex1]) */
            }    /* END else ((OIndex == mxoRsv) && (OList1[oindex1] == mxoNONE)) */
          }      /* END if (tL != NULL) */

        if (tDL != NULL)
          {
          if (OIndex == mxoRsv)
            {
            if (MList[mindex] == mlActive)
              DP = tDL->RsvActivePolicy;
            }
          else
            {
            if (MList[mindex] == mlActive)
              DP = &tDL->ActivePolicy;
            else if (MList[mindex] == mlIdle)
              DP = tDL->IdlePolicy;
            }
          }    /* END if (tDL != NULL) */

        if (OP == NULL)
          continue;

        for (pindex = 0;PList[pindex] != mptNONE;pindex++)
          {
          if (MPolicyCheckLimit(
                &OUsage,
                FALSE,
                PList[pindex],
                PLevel,
                0, /* NOTE:  change to P->Index for per partition limits */
                OP,
                DP,
                QP,
                &tmpL,
                NULL,
                NULL) == FAILURE)
            {
            int AdjustVal;

            switch (PList[pindex])
              {
              case mptMaxJob:  AdjustVal = OUsage.Job;  break;
              case mptMaxNode: AdjustVal = OUsage.Node; break;
              case mptMaxPE:   AdjustVal = OUsage.PE;   break;
              case mptMaxProc: AdjustVal = OUsage.Proc; break;
              case mptMaxPS:   AdjustVal = OUsage.PS;   break;
              case mptMaxWC:   AdjustVal = OUsage.WC;   break;
              case mptMaxMem:  AdjustVal = OUsage.Mem;  break;
              case mptMinProc: AdjustVal = OUsage.Proc; break;
              case mptMaxGRes: AdjustVal = MSNLGetIndexCount(&OUsage.GRes,0); break;
              default: AdjustVal = 0; break;
              }

            if (Message != NULL)
              {
              snprintf(Message,MMAX_LINE,"%s %s violates %s %s %s limit of %ld for %s %s %s (Req: %d  InUse: %lu)",
                MXO[OIndex],
                ONameP,
                MType[mindex],
                MPolicyMode[PLevel],
                MPolicyType[PList[pindex]],
                tmpL,
                MXO[OList2[oindex2]],
                (OID != NULL) ? OID : NONE,
                (oindex1 != mxoNONE) ? MXO[OList1[oindex1]] : "",
                AdjustVal,
                OP->Usage[PList[pindex]][0]);
              }  /* END if (Message != NULL) */

            MDB(3,fSCHED) MLog("%s %s violates %s %s %s limit of %d for %s %s %s (Req: %d, InUse: %ld)\n",
              MXO[OIndex],
              ONameP,
              MType[mindex],
              MPolicyMode[PLevel],
              MPolicyType[PList[pindex]],
              tmpL,
              MXO[OList2[oindex2]],
              (OID != NULL) ? OID : NONE,
              (oindex1 != mxoNONE) ? MXO[OList1[oindex1]] : "",
              AdjustVal,
              OP->Usage[PList[pindex]][0]);

            MSNLFree(&OUsage.GRes);

            return(FAILURE);
            }  /* END if (MPolicyCheckLmit() == FAILURE) */
          }    /* END for (pindex) */
        }      /* END for (oindex2) */
      }        /* END for (oindex1) */
    }          /* END for (mindex) */

  MSNLFree(&OUsage.GRes);

  return(SUCCESS);
  }  /* END MOCheckLimits() */




/**
 * Get time of next job, reservation, or trigger event.
 *
 * @param ETimeP (O)
 */

int MSysGetNextEvent(

  mulong *ETimeP)  /* O */

  {
  rsv_iter RTI;

  int index;

  mrsv_t *R;

  mtrig_t *T;

  mulong tmpETime;

  if (ETimeP == NULL)
    {
    return(FAILURE);
    }

  *ETimeP = MMAX_TIME;

  tmpETime = MMAX_TIME;

  if (MSched.NextSysEventTime > 0)
    tmpETime = MIN(tmpETime,MSched.NextSysEventTime);

  /* locate next trigger event */

  for (index = 0;index < MTList.NumItems;index++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(&MTList,index);

    if (MTrigIsReal(T) == FALSE)
      continue;

    /* if the trigger has a state then it's either running, failed, or completed 
       so we don't care about it */

    if ((T->EType == mttStanding) && (T->State == mtsNONE))
      {
      /* look for re-arm event */

      if (T->ETime > 0)
        {
        /* rsv is active - set event for next instance */

        tmpETime = MIN(tmpETime,T->ETime + T->RearmTime);
        }
      else if (T->LastETime > 0)
        {
        /* rsv is complete - set event for next instance */

        tmpETime = MIN(tmpETime,T->LastETime + T->RearmTime);
        }
      }

    if (T->ETime > MSched.Time)
      {
      if ((T->ETime < tmpETime))
        {
        tmpETime = T->ETime;
        }
      }
    }    /* END for (tindex) */

  if ((MStat.P.IStatEnd > 0) && (((mulong)MStat.P.IStatEnd) < tmpETime))
    tmpETime = MStat.P.IStatEnd;

  /* MRE[] table not populated - determine earliest event from MRsv[] */

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    /* NOTE:  ignore self-optimizing SR rsvs, starttime will keep migrating */

    if (bmisset(&R->Flags,mrfStanding))
      continue;

    if (((mulong)R->StartTime > MSched.Time) && (R->StartTime < tmpETime))
      tmpETime = R->StartTime;
    else if (((mulong)R->EndTime > MSched.Time) && (R->EndTime < tmpETime))
      tmpETime = R->EndTime;
    }  /* END while (MRsvTableIterate()) */

  if (tmpETime == MMAX_TIME)
    {
    /* no event detected */

    return(FAILURE);
    }

  *ETimeP = tmpETime;

  return(SUCCESS);
  }  /* END MSysGetNextEvent() */





/**
 * Destroy all reservations in reservation group.
 *
 * @see MVPCDestroy() - parent
 *
 * @param RsvGroupID (I)
 * @param IsCancel (I) [job/reservation is being cancelled]
 */

int MRsvDestroyGroup(

  char    *RsvGroupID, /* I */
  mbool_t  IsCancel)   /* I (job/reservation is being cancelled) */

  {
  rsv_iter RTI;

  mrsv_t *R;

  if ((RsvGroupID == NULL) || (RsvGroupID[0] == '\0'))
    {
    return(FAILURE);
    }

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((R->RsvGroup == NULL) || strcmp(R->RsvGroup,RsvGroupID))
      continue;

    MDBO(6,R,fUI) MLog("INFO:     removing reservation '%s' in group '%s'\n",
      R->Name,
      RsvGroupID);

    if (IsCancel == TRUE)
      {
      R->CancelIsPending = TRUE;
      }

    MRsvDestroy(&R,TRUE,TRUE);
    }  /* END while (MRsvTableIterate()) */

  return(SUCCESS);
  }  /* END MRsvDestroyGroup() */





/**
 * Set specified attribute on all reservations within reservation group.
 *
 * @param RsvGroupID (I)
 * @param AIndex (I)
 * @param AValue (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MRsvGroupSetAttr(

  char                    *RsvGroupID, /* I */
  enum MRsvAttrEnum        AIndex,     /* I */
  void                    *AValue,     /* I */
  enum MDataFormatEnum     Format,     /* I */
  enum MObjectSetModeEnum  Mode)       /* I */

  {
  rsv_iter RTI;

  mrsv_t *R;

  if ((RsvGroupID == NULL) || (RsvGroupID[0] == '\0'))
    {
    return(FAILURE);
    }

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (R->RsvGroup == NULL)
      continue;

    if (strcmp(R->RsvGroup,RsvGroupID))
      continue;

    MDBO(6,R,fUI) MLog("INFO:     modifying rsv '%s' in group '%s'\n",
      R->Name,
      RsvGroupID);

    MRsvSetAttr(R,AIndex,AValue,Format,Mode);
    }  /* END while (MRsvTableIterate()) */

  return(SUCCESS);
  }  /* END MRsvGroupSetAttr() */





/**
 * Send signal to all reservations within reservation group.
 *
 * @param RsvGroupID (I)
 * @param Signal (I)
 */

int MRsvGroupSendSignal(

  char               *RsvGroupID,  /* I */
  enum MTrigTypeEnum  Signal)      /* I */

  {
  rsv_iter RTI;

  mrsv_t *R;

  if ((RsvGroupID == NULL) || (RsvGroupID[0] == '\0'))
    {
    return(FAILURE);
    }

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (R->RsvGroup == NULL)
      continue;

    if (strcmp(R->RsvGroup,RsvGroupID))
      continue;

    MDBO(6,R,fUI) MLog("INFO:     sending rsv '%s' in group '%s' signal '%s'\n",
      R->Name,
      RsvGroupID,
      MTrigType[Signal]);

    MOReportEvent((void *)R,NULL,mxoRsv,Signal,MSched.Time,TRUE);
    }  /* END while (MRsvTableIterate()) */

  return(SUCCESS);
  }  /* END MRsvGroupSendSignal() */





/**
 * Report list of ids of all reservations in reservation group.
 *
 * NOTE: return SUCCESS only if one or more reservations found.
 * NOTE: do not return failure if RString buffer is too small to hold all reservations.
 *
 * @param RsvGroupID (I)
 * @param RList (O) [optional]
 * @param RString (O) [optional]
 * @param RStringSize (I)
 */

int MRsvGroupGetList(

  char     *RsvGroupID,  /* I */
  marray_t *RList,       /* O (optional) */
  char     *RString,     /* O (optional) */
  int       RStringSize) /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  rsv_iter RTI;

  int RIndex;

  mrsv_t *R;

  if ((RsvGroupID == NULL) || (RsvGroupID[0] == '\0'))
    {
    return(FAILURE);
    }

  RIndex = 0;

  if (RString != NULL)
    MUSNInit(&BPtr,&BSpace,RString,RStringSize);
  
  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (R->RsvGroup == NULL)
      continue;

    if (strcmp(R->RsvGroup,RsvGroupID))
      continue;

    if (RList != NULL)
      MUArrayListAppendPtr(RList,R);

    if (RString != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        (RIndex != 0) ? ":" : "",
        R->Name);
      }

    RIndex++;
    }  /* END while (MRsvTableIterate()) */

  if (RIndex == 0)
    {
    /* no reservations located */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRsvGroupGetList() */





/**
 * Converts range list to string
 *
 * NOTE:  not threadsafe - limited to MMAX_BUFFER
 *
 * @param RL (I)
 * @param ShowAbs (I) - show time as absolute vs relative
 * @param LogOutput (I)
 */

char *MRLShow(

  mrange_t *RL,        /* I */
  mbool_t   ShowAbs,   /* I */
  mbool_t   LogOutput) /* I */

  {
  char *BPtr;
  int   BSpace;

  static char tmpLine[MMAX_BUFFER];

  char tmpSTime[MMAX_NAME];
  char tmpETime[MMAX_NAME];

  int rindex;

  mbool_t SIsConnected;
  mbool_t EIsConnected;

  if (RL == NULL)
    {
    return(NULL);
    }

  MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

  for (rindex = 0;rindex < MMAX_RANGE;rindex++)
    {
    if (RL[rindex].ETime == 0)
      break;

    if ((rindex > 0) && (RL[rindex].STime == RL[rindex - 1].ETime))
      SIsConnected = TRUE;
    else
      SIsConnected = FALSE;

    if ((RL[rindex].ETime > 0) && (RL[rindex].ETime == RL[rindex + 1].STime))
      EIsConnected = TRUE;
    else
      EIsConnected = FALSE;

    /* FORMAT:  <-- STime -> ETime  TC / NC --> */

    if (ShowAbs == TRUE)
      {
      char DString[MMAX_LINE];

      MULToDString(&RL[rindex].STime,DString);
      strcpy(tmpSTime,DString);
      MULToDString(&RL[rindex].ETime,DString);
      strcpy(tmpETime,DString);
      }
    else
      {
      char TString[MMAX_LINE];

      MULToTString(RL[rindex].STime - MSched.Time,TString);
      strcpy(tmpSTime,TString);
      MULToTString(RL[rindex].ETime - MSched.Time,TString);
      strcpy(tmpETime,TString);
      }

    MUSNPrintF(&BPtr,&BSpace,"[%02d]  %3s %14s -> %14s  %3d / %-3d %3s\n",
      rindex,
      (SIsConnected) ? "<--" : "   ",
      tmpSTime,
      tmpETime,
      RL[rindex].TC,
      RL[rindex].NC,
      (EIsConnected) ? "-->" : "   ");
    }  /* END for (rindex) */

  if (LogOutput == TRUE)
    {
    MLog("%s",
      tmpLine);

    return(NULL);
    }

  return(tmpLine);
  }  /* END MRLShow() */





/**
 * Determine effective cost of reservation.
 *
 * @see MClusterShowARes() - parent
 * @see MNodeGetChargeRate() - child
 * @see MRsvGetAllocCost() - child
 * @see MLocalApplyCosts() - peer
 *
 * @param R         (I) [optional]
 * @param V         (I) [optional]
 * @param STime     (I)
 * @param Duration  (I)
 * @param TC        (I)
 * @param PC        (I)
 * @param NC        (I)
 * @param Res       (I)
 * @param N         (I) template node [optional]
 * @param IsPartial (I)
 * @param Variables (I)
 * @param CostP     (O)
 */

int MRsvGetCost(

  mrsv_t  *R,
  mvpcp_t *V,
  mulong   STime,
  mulong   Duration,
  int      TC,
  int      PC,
  int      NC,
  mcres_t *Res,
  mnode_t *N,
  mbool_t  IsPartial,
  const char    *Variables,
  double  *CostP)

  {
  if (CostP == NULL)
    {
    return(FAILURE);
    }

  if (R == NULL)
    {
    /* return cost quote */

    if ((MAM[0].Type != mamtNONE) && (MAM[0].ChargePolicyIsLocal == TRUE))
      {
      mrsv_t tmpR;

      memset(&tmpR,0,sizeof(tmpR));

      strcpy(tmpR.Name,"cost");

      tmpR.AllocPC   = PC;
      tmpR.AllocTC   = TC;
      tmpR.StartTime = STime;
      tmpR.EndTime   = STime + Duration;
      tmpR.RsvGroup  = NULL;
      tmpR.Variables = NULL;

      if ((Variables != NULL) && (Variables[0] != '\0'))
        MRsvSetAttr(&tmpR,mraVariables,(void **)Variables,mdfString,mAdd);

      if (MAM[0].UseDisaChargePolicy == TRUE)
        {
        mhistory_t *tmpH;

        if (Res == NULL)
          return(FAILURE);

        MCResInit(&tmpR.DRes);

        MCResCopy(&tmpR.DRes,Res);

        tmpR.History = NULL;

        MHistoryAddEvent(&tmpR,mxoRsv);

        tmpH = (mhistory_t *)MUArrayListGet(tmpR.History,0);

        tmpH->Time = STime;

        MLocalRsvGetDISAAllocCost(&tmpR,V,CostP,IsPartial,TRUE);

        MUArrayListFree(tmpR.History);

        MUFree((char **)&tmpR.History);
        MCResFree(&tmpR.DRes);

        MULLFree(&tmpR.Variables,MUFREE);
        }
      else
        MLocalRsvGetAllocCost(&tmpR,V,CostP,IsPartial,TRUE);
      }
    else if (MAM[0].Type == mamtNative)
      {
      int    rc;

      mrsv_t tmpR;

      if (Res == NULL)
        return(FAILURE);

      memset(&tmpR,0,sizeof(tmpR));

      strcpy(tmpR.Name,"cost");

      MCResCopy(&tmpR.DRes,Res);

      tmpR.AllocTC    = TC;
      tmpR.StartTime  = STime;
      tmpR.EndTime    = STime + Duration;
      tmpR.RsvGroup   = NULL;
      tmpR.Variables  = NULL;

      if ((Variables != NULL) && (Variables[0] != '\0'))
        MRsvSetAttr(&tmpR,mraVariables,(void **)Variables,mdfString,mAdd);

      rc = MAMHandleQuote(&MAM[0],&tmpR,mxoRsv,CostP,NULL,NULL);

      MULLFree(&tmpR.Variables,MUFREE);

      MCResFree(&tmpR.DRes);

      return(rc);
      }
    else
      {
      /* NOTE:  only apply node charge rate if explicitly set */

      if ((MSched.DefaultN.ChargeRate > 0.0) || ((N != NULL) && (N->ChargeRate > 0.0)))
        {
        /* NOTE:  VPC charge rates can be added to explicit node charge rates */

        *CostP = Duration * TC * MNodeGetChargeRate(N);
        }
      }
    }    /* END if (R == NULL) */
  else
    {
    /* request quote */

    MRsvGetAllocCost(R,V,CostP,MBNOTSET,TRUE);
    }

  if (V != NULL)
    {
    /* Rsv is part of VPC */

    *CostP += V->SetUpCost;

    *CostP += V->NodeSetupCost * TC;

    *CostP += V->NodeHourAllocChargeRate * Duration * TC / MCONST_HOURLEN;
    }

  MRsvTransition(R,FALSE);

  return(SUCCESS);
  }  /* END MRsvGetCost() */





/**
 * Enforce ReqTC/ReqNC and RSVALLOCPOLICY for all reservations.
 *
 * @see MSchedProcessJobs() - parent
 * @see MRsvConfigure() - child
 *
 * NOTE:  called immediately after RM query, before scheduling jobs
 * FIXME: the same intelligence should be duplicated in MRsvConfigure()
 *        in order for MSRPeriodRefresh() and MSRIterationRefresh() to
 *        function correctly.
 */

int MRsvRefresh()

  {
  rsv_iter RTI;

  int nindex;

  int NIndex;

  mrsv_t  *R;
  mnode_t *N;
  mnl_t    NL;     /* list of nodes which should be removed from rsv */

  enum MRsvAllocPolicyEnum APolicy;

  /* NOTE:  based on R->AllocPolicy, adjust allocated resources */

  MNLInit(&NL);

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((R->Type == mrtJob) && (R->StartTime <= MSched.Time))
      {
      /* do not migrate active jobs here */

      continue;
      }

    if (!bmisset(&R->Flags,mrfStanding))
      {
      if ((R->AllocTC < R->ReqTC) ||
          (R->AllocNC < R->ReqNC))
        {
        /* NOTE: reservation "needs" more resources */
        /*       srsv's handled in MSRIterationRefresh() */

        MRsvConfigure(NULL,R,0,0,NULL,NULL,NULL,FALSE);
        }
      }

    APolicy = (R->AllocPolicy != mralpNONE) ?
      R->AllocPolicy :
      MSched.DefaultRsvAllocPolicy;

    switch (APolicy)
      {
      default:
      case mralpNONE:
      case mralpNever:

        /* NO-OP */

        break;

#if 0   /* not supported */
      case mralpPreStartOptimal:

        if (MSched.Time >= R->StartTime)
          {
          /* rsv has already started - do not re-alloc */

          continue;
          }

        if (bmisset(&R->Flags,mrfIsVPC) && bmisset(&R->Flags,mrfTrigHasFired))
          {
          /* VPC rsv has launched triggers - do not re-alloc */

          continue;
          }

        /* fall-through */

      case mralpOptimal:
      case mralpRemap:

        {
        mnode_t *tmpN;

        mbool_t   Force[MSched.M[mxoNode] + 1];  /* should reservation migration be forced? */

        int reindex;

        mrsv_t *NR;

        int     NCount;

        int     PassIndex;

        mulong  Overlap;

        mbool_t IsFeasible;
        mbool_t JobFound;
        mbool_t ACLViolation;

        mbool_t ActionTaken = FALSE;

        int     MaxRsv;

        if (MNLIsEmpty(&R->NL))
          continue;

        if (R->Type != mrtUser)
          {
          /* only optimize placement of user reservations */

          continue;
          }

        /* first pass: find nodes in reservation which do not overlap with
           job */

        NCount = 0;

        MDB(3,fSTRUCT) MLog("INFO:     refreshing rsv '%s'\n",
          R->Name);

        ACLViolation = FALSE;

        for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,&N) == SUCCESS;nindex++)
          {
          IsFeasible = TRUE;
          ACLViolation = FALSE;

          if (N->PtIndex == MSched.SharedPtIndex)
            MaxRsv = MSched.MaxRsvPerGNode;
          else
            MaxRsv = MSched.MaxRsvPerNode;

          for (reindex = 0;reindex < MaxRsv << 1;reindex++)
            {
            if (N->RE[reindex].Type == 0)
              break;

            if (N->RE[reindex].Time > (long)R->EndTime)
              break;

            if (N->RE[reindex].Type != (int)mreStart)
              {
              /* only evaluate start events */

              continue;
              }

            if (N->R[N->RE[rindex].Index] == (mrsv_t *)1)
              continue;

            NR = N->R[N->RE[reindex].Index];

            if (NR == R)
              continue;

            /* NOTE:  migration is not feasible if rsv is 'pinned' by matching job */

            if (((mulong)NR->Type != mrtJob) && !bmisset(&R->Flags,mrfIsVPC))
              {
              continue;
              }

            Overlap = (long)MIN(R->EndTime,NR->EndTime) -
                      (long)MAX(R->StartTime,NR->StartTime);

            if (Overlap <= 0)
              continue;

            /* for VPC overlap need to check priorities so we don't migrate the wrong vpc */

            if (MRsvCheckJAccess(
                  R,
                  (mjob_t *)NR->J,
                  Overlap,
                  NULL,
                  FALSE,
                  NULL,
                  NULL,
                  NULL,
                  NULL) == FAILURE)
              {
              MDB(3,fSTRUCT) MLog("INFO:     found ACL mismatch on node '%s' with rsv '%s'\n",
                N->Name,
                NR->Name);

              /* NOTE:  for RM's with floating allocations, rsv may overlap
                        with ACL mismatch job */

              /* active ACL-violating job has migrated into rsv */

              /* NOTE:  this node should always migrate */

              ACLViolation = TRUE;

              break;
              }  /* END if (MRsvCheckJAccess() == FAILURE) */

            IsFeasible = FALSE;

            break;
            }  /* END for (reindex) */

          if (IsFeasible == TRUE)
            {
            /* add node to list */

            MNLSetNodeAtIndex(&NL,NCount,N);
            MNLSetTCAtIndex(&NL,NCount,MNLGetTCAtIndex(&R->NL,nindex));

            if (ACLViolation == TRUE)
              Force[NCount] = TRUE;
            else
              Force[NCount] = FALSE;

            NCount++;

            if (NCount >= MSched.M[mxoNode])
              break;
            }
          }    /* END for (nindex) */

        /* NL is list of nodes which should be migrated out of rsv */

        MNLTerminateAtIndex(&NL,NCount);

        if (NCount > 0)
          {
          /* rsv can be migrated off of one or more nodes */

          /* locate destination nodes which have matching rsv's */
          /* NOTE:  this includes nodes with no overlapping sys rsv's and with
                    overlapping job rsv's during rsv timeframe */

          for (PassIndex = 0;PassIndex < 2;PassIndex++)
            {
            /* pass one - migrate ACL violators */
            /* pass two - migrate non-ACL violators */

            for (NIndex = 0;NIndex < NCount;NIndex++)
              {
              if ((PassIndex == 0) && (Force[NIndex] == TRUE))
                break;
              else if ((PassIndex == 1) && (Force[NIndex] != TRUE))
                break;
              }

            if (NIndex >= NCount)
              {
              /* no migratable nodes for this pass */

              MDB(3,fSTRUCT) MLog("INFO:     cannot locate feasible nodes to allow migration of rsv '%s'\n",
                R->Name);

              continue;
              }

            for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
              {
              N = MNode[nindex];

              if (N == NULL)
                break;

              if (MNODEISUP(N) == FALSE)
                {
                /* ignore node if it is down */

                continue;
                }

              IsFeasible = TRUE;
              JobFound   = FALSE;

              if (N->PtIndex == MSched.SharedPtIndex)
                MaxRsv = MSched.MaxRsvPerGNode;
              else
                MaxRsv = MSched.MaxRsvPerNode;

              for (reindex = 0;reindex < MaxRsv << 1;reindex++)
                {
                if (N->RE[reindex].Type == (int)mreNONE)
                  break;

                if (N->RE[reindex].Time > (long)R->EndTime)
                  {
                  /* all remaining events are in the future and can be ignored */

                  break;
                  }

                if (N->RE[reindex].Type != (int)mreStart)
                  {
                  /* only evaluate start events */

                  continue;
                  }

                if (N->R[N->RE[rindex].Index] == (mrsv_t *)1)
                  continue;

                NR = N->R[N->RE[reindex].Index];

                if (NR == R)
                  {
                  /* node already allocated to rsv */

                  IsFeasible = FALSE;

                  break;
                  }

                Overlap = (long)MIN(R->EndTime,NR->EndTime) -
                          (long)MAX(R->StartTime,NR->StartTime);

                if (NR->Type == mrtJob)
                  {
                  if (Overlap > 0)
                    {
                    if (MRsvCheckJAccess(R,(mjob_t *)NR->J,Overlap,NULL,FALSE,NULL,NULL,NULL,NULL) == SUCCESS)
                      {
                      /* matching job located */

                      JobFound = TRUE;
                      }
                    else if ((NR->J != NULL) &&
                             MJOBISACTIVE((mjob_t *)NR->J))
                      {
                      /* if job is active, node is not feasible */

                      /* NOTE:  allow nodes with ACL violating idle job rsvs to be considered.
                                if selected, Moab will remap this rsv and then when Moab sees
                                the ACL violation with the idle rsv, will remap the idle job
                                rsv
                      */

                      IsFeasible = FALSE;
                      }
                    }    /* END if (Overlap > 0) */
                  }      /* END if (NR->Type == mrtJob) */
                else
                  {
                  if (Overlap > 0)
                    {
                    IsFeasible = FALSE;

                    break;
                    }
                  }
                }      /* END for (reindex) */

              if (IsFeasible != TRUE)
                {
                MDBO(7,R,fSTRUCT) MLog("INFO:     node %s is not feasible for rsv %s - ignoring node\n",
                  N->Name,
                  R->Name);

                continue;
                }

              if ((JobFound != TRUE) && (Force[NIndex] == FALSE))
                {
                MDBO(7,R,fSTRUCT) MLog("INFO:     job found on node %s for rsv %s - ignoring node\n",
                  N->Name,
                  R->Name);

                continue;
                }

              /* NOTE:  verify node meets rsv resource constraints */

              if (MRsvCheckNodeAccess(R,N) == FAILURE)
                {
                MDBO(7,R,fSTRUCT) MLog("INFO:     reservation %s cannot access node %s\n",
                  R->Name,
                  N->Name);

                continue;
                }

              MNLGetNodeAtIndex(&NL,NIndex,&tmpN);

              if (MRsvReplaceNode(R,tmpN,MNLGetTCAtIndex(&NL,NIndex),N) == SUCCESS)
                {
                NIndex++;

                ActionTaken = TRUE;

                for (;NIndex < NCount;NIndex++)
                  {
                  if ((PassIndex == 0) && (Force[NIndex] == TRUE))
                    break;
                  else if ((PassIndex == 1) && (Force[NIndex] != TRUE))
                    break;
                  }

                if (NIndex >= NCount)
                  {
                  /* no remaining migratable nodes for this pass */

                  break;
                  }

                MDBO(3,R,fSTRUCT) MLog("INFO:     successfully replaced reserved node %s with %s in rsv %s\n",
                  tmpN->Name,
                  N->Name,
                  R->Name);
                }    /* END if (MRsvReplaceNode()) */
              else
                {
                MDBO(3,R,fSTRUCT) MLog("WARNING:  failed to replace reserved node %s with %s in rsv %s\n",
                  tmpN->Name,
                  N->Name,
                  R->Name);
                }
              }      /* END for (nindex) */
            }        /* END for (PassIndex) */
          }          /* END if (NCount > 0) */

        if ((ACLViolation == TRUE) && (ActionTaken == FALSE))
          {
          MDBO(3,R,fSTRUCT) MLog("INFO:     cannot realloc resources for rsv '%s' (no room to migrate)\n",
            R->Name);
          }
        }          /* END BLOCK (case mralpOptimal) */

        break;

      case mralpPreStartFailure:
      case mralpPreStartMaintenance:

        if (MSched.Time >= R->StartTime)
          {
          /* rsv has already started - do not re-alloc */

          continue;
          }

        if (bmisset(&R->Flags,mrfIsVPC) && bmisset(&R->Flags,mrfTrigHasFired))
          {
          /* VPC rsv has launched triggers - do not re-alloc */

          continue;
          }

        /* fall through */

#endif /* not-supported */

      case mralpFailure:

        {
        char tmpBuf[MMAX_BUFFER];
        char tmpLine[MMAX_LINE];

        int  DownNC;

        if (MNLIsEmpty(&R->NL))
          {
          /* no node list exists on rsv */

          continue;
          }

        if ((APolicy == mralpFailure) &&
            ((bmisset(&R->Flags,mrfStanding)) ||
             (R->Type == mrtJob)))
          {
          /* Don't reallocate nodes for standing or job reservations. */

          continue;
          }

        /* detect failure */

        NIndex = 0;

        DownNC = 0;

        for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,&N) == SUCCESS;nindex++)
          {
          if (MNODEISUP(N) == TRUE)
            {
            if (APolicy != mralpPreStartMaintenance)
              {
              continue;
              }

            /* search for rsv overlap w/maintenance rsv */

            if (MRsvHasOverlap(R,(mrsv_t *)2,TRUE,NULL,N,NULL) == FALSE)
              {
              continue;
              }
            }  /* END if (MNODEISUP(N) == TRUE) */
          else
            {
            DownNC++;
            }

          /* node is down or overlaps maintenance rsv, add node to list */

          MNLSetNodeAtIndex(&NL,NIndex,N);
          MNLSetTCAtIndex(&NL,NIndex,MNLGetTCAtIndex(&R->NL,nindex));

          NIndex++;
          }  /* END for (nindex) */

        R->DownNC = DownNC;

        if (NIndex == 0)
          {
          /* all nodes available - take no action */

          continue;
          }    /* END if (NIndex == 0) */

        /* terminate list */

        MNLTerminateAtIndex(&NL,NIndex);

        /* one or more nodes are unavailable, take action */

        MNLToString(&NL,FALSE,",",'\0',tmpBuf,sizeof(tmpBuf));

        /* NOTE:  rsv start occurs if triggers fire or the rsv is active */

        if ((APolicy == mralpFailure) || (bmisset(&R->Flags,mrfIsActive) != TRUE))
          {
          R->EnforceHostExp = TRUE;

          if (MRsvReplaceNodes(R,tmpBuf,tmpLine) == FAILURE)
            {
            /* NOTE: Reservation is possibly ruined. In MRsvReplaceNodes, nodes are removed from the 
             * reservation and then the reservation is reconfigured. If the nodes were removed and the 
             * reconfiguring failed, then the reservation is ruined. */

            MDBO(6,R,fSTRUCT) MLog("INFO:     cannot realloc resources for rsv '%s' (missing %d)\n",
              R->Name,
              NIndex);

            if (R->DownNC != NIndex)
              {
              sprintf(tmpBuf,"cannot realloc resources for rsv '%s' (missing %d procs) (%s)",
                R->Name,
                NIndex,
                tmpLine);

              MSysSendMail(
                MSysGetAdminMailList(1),
                NULL,
                "moab detected hardware failure",
                NULL,
                tmpBuf);
              }
            }
          else
            {
            char tmpLine[MMAX_LINE];

            /* successfully re-alloc'd new nodes */

            MDB(6,fSTRUCT) MLog("INFO:     realloc'd resources for rsv '%s')\n",
              R->Name);

            if (R->DownNC != NIndex)
              {
              sprintf(tmpLine,"successfully realloc'd resources for rsv '%s' (located %d procs)",
                R->Name,
                NIndex);

              MSysSendMail(
                MSysGetAdminMailList(1),
                NULL,
                "moab corrected hardware failure",
                NULL,
                tmpLine);
              }
            }

          R->EnforceHostExp = FALSE;
          }  /* END if ((APolicy == mralpFailure) || ... */

        R->DownNC = NIndex;
        }  /* END BLOCK (case mralpFailure) */

        break;
      }  /* END switch (APolicy) */
    }    /* END while (MRsvTableIterate()) */

  MNLFree(&NL);

  return(SUCCESS);
  }  /* END MRsvRefresh() */





/**
 *
 *
 * @param NL (I)
 * @param RType (I)
 * @param Var (O)
 * @param DFormat (I)
 */

int MNLGetAttr(

  mnl_t                   *NL,      /* I */
  enum MResourceTypeEnum   RType,   /* I */
  void                    *Var,     /* O */
  enum MDataFormatEnum     DFormat) /* I */

  {
  int      index;

  mnode_t *N;

  if ((NL == NULL) || (DFormat == mdfNONE) || (RType == mrNONE))
    {
    return(FAILURE);
    }

  for (index = 0;MNLGetNodeAtIndex(NL,index,&N) == SUCCESS;index++)
    {
    switch (RType)
      {
      case mrProc:

        switch (DFormat)
          {
          case mdfInt:

            *((int *)Var) += MNLGetTCAtIndex(NL,index);

            break;

          default:

            /* NO-OP */

            break;
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (RType) */
    }    /* END for (index) */

  return(SUCCESS);
  }  /* END MNLGetAttr() */



/**
 * Report TRUE if R2 overlaps R1.
 *
 * @param R1 (I)
 * @param R2 (I) [set to '\1' to check for overlap with any job, set to '\2' to
 *                check for overlap with any maintenance rsv]
 * @param AllowPartial (I) report TRUE if partial overlap detected
 * @param SNL (I) [optional]
 * @param SN (I) [optional]
 * @param IsPartialP (O) [optional] TRUE if only partial overlap detected
 *                                  only set if AllowPartial is TRUE
 */

mbool_t MRsvHasOverlap(

  mrsv_t    *R1,
  mrsv_t    *R2,
  mbool_t    AllowPartial,
  mnl_t     *SNL,
  mnode_t   *SN,
  mbool_t   *IsPartialP)
                            

  {
  mre_t   *RE;
  mrsv_t  *R;

  mnode_t *N;

  mbool_t  InternalFound = FALSE;
  mbool_t  ExternalFound = FALSE;

  mbool_t  CheckJob      = FALSE;
  mbool_t  CheckMaintenance = FALSE;

  int      nindex = 0;

  if (IsPartialP != NULL)
    *IsPartialP = FALSE;

  if ((R1 == NULL) || (R2 == NULL))
    {
    return(FALSE);
    }

  /* verify overlap in time */

  if (R2 == (mrsv_t *)1)
    {
    CheckJob = TRUE;
    }
  else if (R2 == (mrsv_t *)2)
    {
    CheckMaintenance = TRUE;
    }
  else
    {
    if (MIN(R1->EndTime,R2->EndTime) <= MAX(R1->StartTime,R2->StartTime))
      {
      /* no overlap */

      return(FALSE);
      }
    }

  if ((SN == NULL) && ((SNL == NULL) || (MNLIsEmpty(SNL))))
    {
    /* no nodes specified */

    return(FALSE);
    }

  if (SN != NULL)
    {
    N = SN;
    }
  else
    {
    MNLGetNodeAtIndex(SNL,0,&N);
    }

  while (N != NULL)
    {
    R = NULL;

    for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
      {
      R = MREGetRsv(RE);

      if (R == R1)
        {
        /* rsv entry is self */

        continue;
        }

      if (CheckJob == TRUE)
        {
        long TimeDiff;

        if (R->Type != mrtJob)
          continue;

        TimeDiff = MIN(R1->EndTime,R->EndTime) - MAX(R1->StartTime,R->StartTime);

        if (TimeDiff < 0)
          {
          /* no overlap */

          continue;
          }

        R2 = R;
        }
      else if (CheckMaintenance == TRUE)
        {
        long TimeDiff;

        if ((R->SubType != mrsvstAdmin_HWMaintenance) &&
            (R->SubType != mrsvstAdmin_SWMaintenance) &&
            (R->SubType != mrsvstAdmin_EmergencyMaintenance) &&
            (!MACLIsEmpty(R->ACL)))
          {
          /* NOTE:  maintenance rsv's have 'self' in ACL */

          if ((R->ACL->Type != maRsv) && (R->ACL->Type != maNONE))
            {
            /* not maintenance rsv */

            continue;
            }
          }

        TimeDiff = MIN(R1->EndTime,R->EndTime) - MAX(R1->StartTime,R->StartTime);

        if (TimeDiff < 0)
          {
          /* no overlap */

          continue;
          }

        R2 = R;
        }  /* END else if (CheckMaintenance == TRUE) */

      if (R == R2)
        {
        /* R2 overlaps R1 on N */

        InternalFound = TRUE;

        if (AllowPartial == TRUE)
          {
          if (IsPartialP == NULL)
            {
            return(TRUE);
            }
          else if (ExternalFound == TRUE)
            {
            *IsPartialP = TRUE;

            return(TRUE);
            }
          else
            {
            /* must check remaining nodes if AllowPartial is FALSE */
            }
          }

        break;
        }  /* END if (R == R2) */
      }    /* END for (rindex) */

    if (R != R2)
      {
      ExternalFound = TRUE;

      if (AllowPartial == FALSE)
        {
        return(FALSE);
        }
      else if (InternalFound == TRUE)
        {
        if (IsPartialP != NULL)
          *IsPartialP = TRUE;

        return(TRUE);
        }
      }  /* END if (R != R2) */

    if (SN != NULL)
      {
      /* NOTE:  IsPartial cannot be TRUE for single node */

      if (InternalFound == TRUE)
        {
        return(TRUE);
        }
      else
        {
        return(FALSE);
        }
      }

    nindex++;

    MNLGetNodeAtIndex(SNL,nindex,&N);
    }    /* END while (N != NULL) */

  if (InternalFound == FALSE)
    {
    return(FALSE);
    }

  if (ExternalFound == FALSE)
    {
    return(TRUE);
    }

  /* allocation is partial */

  if (AllowPartial == FALSE)
    {
    return(FALSE);
    }

  if (IsPartialP != NULL)
    *IsPartialP = TRUE;

  return(TRUE);
  }  /* END MRsvHasOverlap() */



/**
 *
 *
 * @param RS
 */

int MRsvDumpNL(

  mrsv_t *RS)

  {
  rsv_iter RTI;

  mrsv_t *R;

  if (mlog.Threshold < 8)
    {
    return(SUCCESS);
    }

  MDB(8,fSTRUCT) MLog("INFO:     dumping rsv nodelist\n");

  MRsvIterInit(&RTI);

  mstring_t tmpString(MMAX_LINE);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((RS != NULL) && (R != RS))
      continue;

    MRsvAToMString(R,mraAllocNodeList,&tmpString,mcmCP);

    MDB(8,fSTRUCT) MLog("rsv '%s' NL-> '%s'\n",
      R->Name,
      tmpString.c_str());
    }  /* END while (MRsvTableIterate()) */

  return(SUCCESS);
  }  /* END MRsvDumpNL() */





/**
 * Determine total reservation cost.
 *
 * ALGO:  Cost = NodeCount * (Node.ChargeRate) * (Account.Cost) * (QOS.DedResCost) * Duration
 *
 * NOTE:  If R is part of VPC, use VPC charge rate
 * NOTE:  If IsQuote is TRUE, charge to R->EndTime, otherwise, only charge up to current time
 *
 * @see MRsvGetCost() - parent
 * @see MJobGetCost() - peer
 * @see MVPCFind() - child - locate associated VPC
 * @see MUNLGetTotalChargeRate() - child
 * @see MLocalRsvGetCost() - child
 *
 * @param R (I)
 * @param V (I) [optional]
 * @param RCostP (O)
 * @param IsPartial (I)
 * @param IsQuote (I)
 */

int MRsvGetAllocCost(

  mrsv_t  *R,         /* I */
  mvpcp_t *V,         /* I (optional) */
  double  *RCostP,    /* O */
  mbool_t  IsPartial, /* I */
  mbool_t  IsQuote)   /* I */

  {
  double Cost;

  mqos_t   *Q;
  mgcred_t *A;

  mpar_t   *C;
  mvpcp_t  *tmpV;

  mulong    Duration;
  mulong    StartTime;
  mulong    EndTime;

  if (RCostP != NULL)
    *RCostP = 0.0;

  if ((R == NULL) || (RCostP == NULL))
    {
    return(FAILURE);
    }

  if ((MAM[0].ChargePolicyIsLocal == TRUE) && 
      (MLocalRsvGetAllocCost(R,V,RCostP,IsPartial,IsQuote) == SUCCESS))
    {
    return(SUCCESS);
    }

  if ((MAM[0].Type == mamtNative) &&
      (IsQuote == TRUE))
    {
    MAMHandleQuote(&MAM[0],R,mxoRsv,RCostP,NULL,NULL);

    return(SUCCESS);
    }

  /* NOTE:  support min/max and avg costing (NYI) */

  if (!MNLIsEmpty(&R->NL))
    {
    /* must handle indirect node resource charging (ie, GRes) */

    MUNLGetTotalChargeRate(&R->NL,&Cost);
    }
  else
    {
    Cost = (double)R->AllocNC;
    }

  if (R->SubType == mrsvstVPC)
    {
    if (MVPCFind(R->RsvGroup,&C,TRUE) == SUCCESS)
      {
      if (C->Cost > 0)
        {
        /* total cost already calculated */

        *RCostP = C->Cost;

        return(SUCCESS);
        }

      /* extract Pkg from VPC from Rsv */

      tmpV = V;

      if ((tmpV != NULL) ||
          (C->VPCProfile != NULL))
        {
        /* NOTE:  If VPC cost not explicitly set, assume SetUpCosts charged
                  elsewhere at VPC creation/modify time */

        /* NOTE:  adjust chargerate to match node-second based usage */

        if (tmpV == NULL)
          tmpV = C->VPCProfile;

        Cost *= (tmpV->NodeHourAllocChargeRate / MCONST_HOURLEN);
        }
      }
    }    /* END if (R->SubType == mrsvstVPC) */

  A = R->A;

  if (A != NULL)
    {
    if (A->ChargeRate != 0)
      Cost *= A->ChargeRate;
    }

  MACLGetCred(R->CL,maQOS,(void **)&Q);

  if (Q != NULL)
    {
    if (Q->DedResCost >= 0.0)
      Cost *= Q->DedResCost;
    }

  /* EndTime is the MIN of now and R->EndTime */

  if (IsQuote == TRUE)
    EndTime = R->EndTime;
  else
    EndTime = MIN(R->EndTime,MSched.Time);
  /* if rsv has been charged before, use lastchargetime instead of starttime*/
  if (R->LastChargeTime > 0)
    {
     StartTime = R->LastChargeTime;
    }	
  else
    {
     StartTime = R->StartTime;
    }

  if ((long)EndTime > R->LastChargeTime)
    Duration = EndTime - StartTime;
  else
    Duration = 1;

  Cost = Duration;

  *RCostP = Cost;

  return(SUCCESS);
  }  /* END MRsvGetAllocCost() */





/**
 * Remove OrigN from R, replace with NewN.
 *
 * @param R (I) [modified]
 * @param OrigN (I)
 * @param OrigTC (I)
 * @param NewN (I)
 */

int MRsvReplaceNode(

  mrsv_t  *R,      /* I (modified) */
  mnode_t *OrigN,  /* I */
  int      OrigTC, /* I */
  mnode_t *NewN)   /* I */

  {
  int TC;
  int NC;
  int nindex;

  const char *FName = "MRsvReplaceNode";

  if ((R == NULL) || (OrigN == NULL) || (NewN == NULL))
    {
    return(FAILURE);
    }

  MDB(3,fSTRUCT) MLog("%s(%s,%s,%d,%s)\n",
    FName,
    R->Name,
    OrigN->Name,
    OrigTC,
    NewN->Name);

  if (R->HostExpIsSpecified == MBNOTSET)
    {
    /* see MRsvCheckStatus() */

    R->HostExpIsSpecified = FALSE;

    MUFree(&R->HostExp);
    }

  /* remove OrigN from R */

  if (MRsvAdjustNode(R,OrigN,-1,-1,TRUE) == FAILURE)
    {
    return(FAILURE);
    }

  /* add NewN to R */

  if (MRsvAdjustNode(R,NewN,-1,1,TRUE) == FAILURE)
    {
    return(FAILURE);
    }

  if (!MNLIsEmpty(&R->NL))
    {
    mnode_t *N;

    /* replace OrigN with NewN in R->NL */

    for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,&N) == SUCCESS;nindex++)
      {
      if (N == OrigN)
        {
        MNLSetNodeAtIndex(&R->NL,nindex,NewN);
        MNLSetTCAtIndex(&R->NL,nindex,OrigTC);

        break;
        }
      }    /* END for (nindex) */
    }      /* END if (R->NL != NULL) */

  TC = 0;
  NC = 0;

  TC = MNLSumTC(&R->NL);
  NC = MNLCount(&R->NL);

  R->AllocTC = TC;

  if (R->ReqTC != 0)
    R->ReqTC = TC;

  R->AllocNC = NC;

  if (R->ReqNC != 0)
    R->ReqNC   = NC;

  return(SUCCESS);
  }  /* END MRsvReplaceNode() */





/**
 *
 *
 * @param J (I) job reserving nodes
 * @param MNodeList (I) [optional]
 * @param StartTime (I) time reservation starts
 * @param RsvSType (I) reservation subtype
 * @param RP (O) [optional]
 * @param NoRsvTransition (I)
 */

int MRsvSyncJCreate(

  mjob_t                 *J,            /* I  job reserving nodes     */
  mnl_t                 **MNodeList,    /* I  nodes to be reserved (optional) */
  long                    StartTime,    /* I  time reservation starts */
  enum MJRsvSubTypeEnum   RsvSType,     /* I  reservation subtype  */
  mrsv_t                **RP,           /* O  reservation pointer (optional) */
  mbool_t                 NoRsvTransition) /* I  don't transition */

  {
  int rqindex;

  mdepend_t *D = NULL;

  mjob_t *tJ;

  if ((J == NULL) || (J->Depend == NULL))
    {
    return(FAILURE);
    }

  if (MJobFind(J->Name,&tJ,mjsmBasic) == FAILURE)
    {
    return(FAILURE);
    }

  MRsvJCreate(tJ,MNodeList,StartTime,RsvSType,RP,NoRsvTransition);

  rqindex = 0;

  while (tJ->Req[rqindex] != NULL)
    {
    rqindex++;
    }

  /* NOTE:  only search 'AND' dependencies */

  /* NOTE: ignoring D->Type constraints - FIXME */

  D = J->Depend->NextAnd;

  while (D != NULL)
    {
    if (MJobFind(D->Value,&tJ,mjsmBasic) == SUCCESS)
      {
      MRsvJCreate(tJ,&MNodeList[rqindex],StartTime,RsvSType,RP,NoRsvTransition);

      while (tJ->Req[rqindex] != NULL)
        {
        rqindex++;
        }
      }

    D = D->NextAnd;
    }  /* END while (D != NULL) */

  return(SUCCESS);
  }  /* END MRsvSyncJCreate() */





/**
 *
 *
 * @param R (I)
 */

int MRsvCreateCredLock(

  mrsv_t *R)  /* I */

  {
  job_iter JTI;

  mgcred_t *U[MMAX_ACL];
  mgcred_t *A[MMAX_ACL];
  mgcred_t *G[MMAX_ACL];
  mqos_t   *Q[MMAX_ACL];
  mclass_t *C[MMAX_ACL];

  macl_t *tACL;

  mjob_t *J;

  char   *Name;

  int uindex;
  int aindex;
  int gindex;
  int qindex;
  int cindex;

  U[0] = NULL;
  A[0] = NULL;
  G[0] = NULL;
  Q[0] = NULL;
  C[0] = NULL;

  uindex = 0;
  aindex = 0;
  gindex = 0;
  qindex = 0;
  cindex = 0;

  if (R->RsvGroup != NULL)
    {
    Name = R->RsvGroup;
    }
  else
    {
    Name = R->Name;
    }

  tACL = R->ACL;

  while (tACL != NULL)
    {
    if (!bmisset(&tACL->Flags,maclCredLock))
      {
      tACL = tACL->Next;

      continue;
      }

    switch (tACL->Type)
      {
      case maUser:

        U[uindex] = (mgcred_t *)tACL->OPtr;

        if (U[uindex] != NULL)
          MUStrDup(&U[uindex]->F.ReqRsv,Name);

        uindex++;
        U[uindex] = NULL;

        break;

      case maGroup:

        G[gindex] = (mgcred_t *)tACL->OPtr;

        if (G[gindex] != NULL)
          MUStrDup(&G[gindex]->F.ReqRsv,Name);

        gindex++;
        G[gindex] = NULL;

        break;

      case maAcct:

        A[aindex] = (mgcred_t *)tACL->OPtr;

        if (A[aindex] != NULL)
          MUStrDup(&A[aindex]->F.ReqRsv,Name);

        aindex++;
        A[aindex] = NULL;

        break;

      case maQOS:

        Q[qindex] = (mqos_t *)tACL->OPtr;

        if (Q[qindex] != NULL)
          MUStrDup(&Q[qindex]->F.ReqRsv,Name);

        qindex++;
        Q[qindex] = NULL;

        break;

      case maClass:

        C[cindex] = (mclass_t *)tACL->OPtr;

        if (C[cindex] != NULL)
          MUStrDup(&C[cindex]->F.ReqRsv,Name);

        cindex++;
        C[cindex] = NULL;

        break;

      default:

        break;
      } /* END switch (tACL->Type) */
 
    tACL = tACL->Next;
    }   /* END while() */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    for (uindex = 0;U[uindex] != NULL;uindex++)
      {
      if (J->Credential.U == U[uindex])
        {
        bmset(&J->Flags,mjfAdvRsv);
        bmset(&J->SpecFlags,mjfAdvRsv);

        MUStrDup(&J->ReqRID,Name);
        }
      }

    for (gindex = 0;G[gindex] != NULL;gindex++)
      {
      if (J->Credential.G == G[gindex])
        {
        bmset(&J->Flags,mjfAdvRsv);
        bmset(&J->SpecFlags,mjfAdvRsv);

        MUStrDup(&J->ReqRID,Name);
        }
      }

    for (aindex = 0;A[aindex] != NULL;aindex++)
      {
      if (J->Credential.A == A[aindex])
        {
        bmset(&J->Flags,mjfAdvRsv);
        bmset(&J->SpecFlags,mjfAdvRsv);

        MUStrDup(&J->ReqRID,Name);
        }
      }

    for (qindex = 0;Q[qindex] != NULL;qindex++)
      {
      if (J->Credential.Q == Q[qindex])
        {
        bmset(&J->Flags,mjfAdvRsv);
        bmset(&J->SpecFlags,mjfAdvRsv);

        MUStrDup(&J->ReqRID,Name);
        }
      }

    for (cindex = 0;C[cindex] != NULL;cindex++)
      {
       if (J->Credential.C == C[cindex])
        {
        bmset(&J->Flags,mjfAdvRsv);
        bmset(&J->SpecFlags,mjfAdvRsv);

        MUStrDup(&J->ReqRID,Name);
        }
      }
    }

  return(SUCCESS);
  }   /* END MRsvCreateCredLock() */





/**
 * Destroy/free cred lock pointers associated with specified reservation.
 *
 * @param R (I)
 */

int MRsvDestroyCredLock(

  mrsv_t *R)  /* I */

  {
  job_iter JTI;

  mgcred_t *U[MMAX_ACL];
  mgcred_t *A[MMAX_ACL];
  mgcred_t *G[MMAX_ACL];
  mqos_t   *Q[MMAX_ACL];
  mclass_t *C[MMAX_ACL];

  macl_t *tACL;

  mjob_t *J;

  int uindex;
  int aindex;
  int gindex;
  int qindex;
  int cindex;

  const char *FName = "MRsvDestroyCredLock";

  MDB(2,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

   if ((R == NULL) || (MJobHT.size() == 0))
    {
    return(FAILURE);
    }

  if (bmisset(&R->Flags,mrfAdvRsvJobDestroy))
    {
    MJobIterInit(&JTI);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if (J->ReqRID != NULL)
        {
        /* check standing reservation name */
   
        if ((R->RsvGroup != NULL) && !strncmp(R->RsvGroup,J->ReqRID,strlen(R->RsvGroup)))
          {
          MDB(8,fSTRUCT) MLog("INFO:     inclusive ('ReqRID' rsv group)\n");
          }     /* END if (bmisset(&R->RsvGroup != NULL) && ...) */
        else if (!strcmp(R->Name,J->ReqRID))
          {
          MDB(8,fSTRUCT) MLog("INFO:     inclusive ('ReqRID' rsv id)\n");
          } /* END else if (strcmp(R->Name,J->ReqRID)) */
        else
          {
          continue;
          }
   
        MJobCancel(J,"Required reservation was destroyed",FALSE,NULL,NULL);
   
        continue;
        } /* END if (J->ReqRID != NULL) */
      }   /* END for (J) */
    }     /* END if (bmisset(&R->Flags,mrfAdvRsvJobDestroy)) */

  if (bmisset(&R->Flags,mrfStanding))
    {
    return(SUCCESS);
    }

  U[0] = NULL;
  A[0] = NULL;
  G[0] = NULL;
  Q[0] = NULL;
  C[0] = NULL;

  uindex = 0;
  aindex = 0;
  gindex = 0;
  qindex = 0;
  cindex = 0;

  tACL = R->ACL;

  while (tACL != NULL)
    {
    if (!bmisset(&tACL->Flags,maclCredLock))
      {
      tACL = tACL->Next;

      continue;
      }

    switch (tACL->Type)
      {
      case maUser:

        U[uindex] = (mgcred_t *)tACL->OPtr;

        if (U[uindex] != NULL)
          MUFree(&U[uindex]->F.ReqRsv);

        uindex++;
        U[uindex] = NULL;

        break;

      case maGroup:

        G[gindex] = (mgcred_t *)tACL->OPtr;

        if (G[gindex] != NULL)
          MUFree(&G[gindex]->F.ReqRsv);

        gindex++;
        G[gindex] = NULL;

        break;

      case maAcct:

        A[aindex] = (mgcred_t *)tACL->OPtr;

        if (A[aindex] != NULL)
          MUFree(&A[aindex]->F.ReqRsv);

        aindex++;
        A[aindex] = NULL;

        break;

      case maQOS:

        Q[qindex] = (mqos_t *)tACL->OPtr;

        if (Q[qindex] != NULL)
          MUFree(&Q[qindex]->F.ReqRsv);

        qindex++;
        Q[aindex] = NULL;

        break;

      case maClass:

        C[cindex] = (mclass_t *)tACL->OPtr;

        if (C[cindex] != NULL)
          MUFree(&C[cindex]->F.ReqRsv);

        cindex++;
        C[cindex] = NULL;

        break;

      default:

        /* NO-OP */

        break;
      } /* END switch (tACL->Type) */

    tACL = tACL->Next;
    }   /* END while() */

  if ((uindex == 0) && (gindex == 0) && (cindex == 0) &&
      (aindex == 0) && (qindex == 0))
    {
    return(SUCCESS);
    }

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    for (uindex = 0;U[uindex] != NULL;uindex++)
      {
      if (J->Credential.U == U[uindex])
        {
        bmunset(&J->Flags,mjfAdvRsv);
        bmunset(&J->SpecFlags,mjfAdvRsv);

        MUFree(&J->ReqRID);
        }
      }

    for (gindex = 0;G[gindex] != NULL;gindex++)
      {
      if (J->Credential.G == G[gindex])
        {
        bmunset(&J->Flags,mjfAdvRsv);
        bmunset(&J->SpecFlags,mjfAdvRsv);

        MUFree(&J->ReqRID);
        }
      }

    for (aindex = 0;A[aindex] != NULL;aindex++)
      {
      if (J->Credential.A == A[aindex])
        {
        bmunset(&J->Flags,mjfAdvRsv);
        bmunset(&J->SpecFlags,mjfAdvRsv);

        MUFree(&J->ReqRID);
        }
      }

    for (qindex = 0;Q[qindex] != NULL;qindex++)
      {
      if (J->Credential.Q == Q[qindex])
        {
        bmunset(&J->Flags,mjfAdvRsv);
        bmunset(&J->SpecFlags,mjfAdvRsv);

        MUFree(&J->ReqRID);
        }
      }

    for (cindex = 0;C[cindex] != NULL;cindex++)
      {
       if (J->Credential.C == C[cindex])
        {
        bmunset(&J->Flags,mjfAdvRsv);
        bmunset(&J->SpecFlags,mjfAdvRsv);

        MUFree(&J->ReqRID);
        }
      }
    }

  return(SUCCESS);
  }  /* END MRsvDestroyCredLock() */




/**
 * Synchronize a reservation and its systemjob
 *
 * NOTE:  This job will create a mapping system job if one does not exist.
 * NOTE:  R->J will point to new system job
 *
 * @see mrfSystemJob
 * @see __MUIRsvCtlModify() - parent
 * @see MJobCreate() - child
 * @see MRsvToJob() - child
 * @see MRsvUpdateStats() - parent
 * @see MUGenerateSystemJobs() - peer - creates operational system jobs to 
 *        complete system tasks (ie, migrate, provision, power-on, etc)
 *
 * @param R (I)
 */

int MRsvSyncSystemJob(

  mrsv_t *R) /* I */

  {
  mjob_t *J = NULL;

  mjob_t *JT;  /* job stat template */

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (!bmisset(&R->Flags,mrfSystemJob))
    {
    return(SUCCESS);
    }

  if (R->J == NULL)
    {
    /* create a parallel system job for this reservation */

    char tmpBaseName[MMAX_NAME];
    char tmpJName[MMAX_NAME];

    mreq_t *Req;
    mrm_t  *RM;

    if (MRMGetInternal(&RM) == FAILURE)
      {
      return(FAILURE);
      }

    if ((MJobCreate(R->Name,TRUE,&J,NULL) == FAILURE) ||
        (MReqCreate(J,NULL,&Req,TRUE) == FAILURE))
      {
      MJobDestroy(&J);

      return(FAILURE);
      }

    MRMJobPreLoad(J,R->Name,RM);

    /* this can get lost in MRMJobPostUpdate()->MJobReleaseRsv() */

    MRsvToJob(R,J);

    J->Rsv = R;

    if (!MNLIsEmpty(&R->NL))
      {
      /* FIXME: does not work with multi-req jobs yet */

      MNLCopy(&J->Req[0]->NodeList,&J->ReqHList);
      MNLCopy(&J->NodeList,&J->ReqHList);
      }

    MUStrCpy(tmpBaseName,R->Name,sizeof(tmpBaseName));

/*
    if ((ptr = strchr(tmpBaseName,'.')) != NULL)
      *ptr = '\0';
*/

    snprintf(tmpJName,sizeof(tmpJName),"%s.%d",
      tmpBaseName,
      MRMJobCounterIncrement(RM));

    MJobSetAttr(J,mjaJobID,(void **)tmpJName,mdfString,mSet);
    MJobSetAttr(J,mjaJobName,(void **)tmpBaseName,mdfString,mSet);

    if (MTJobFind(tmpBaseName,&JT) == SUCCESS)
      {
      /* associate job with matching template */

      MULLAdd(
        &J->TStats,
        tmpBaseName,
        (void *)JT,
        NULL,
        NULL);
      }

    MUStrDup(&J->ReqRID,R->Name);

    MRMJobPostLoad(J,NULL,RM,NULL);

    /* return failure if the user credential is not set */

    if (J->Credential.U == NULL)
      {
      return FAILURE;
      }

    bmset(&J->Flags,mjfNoRMStart);
    bmset(&J->Flags,mjfAdvRsv);
    bmset(&J->Flags,mjfRsvMap);
    bmset(&J->Flags,mjfSystemJob);

    bmset(&J->SpecFlags,mjfNoRMStart);
    bmset(&J->SpecFlags,mjfAdvRsv);
    bmset(&J->SpecFlags,mjfRsvMap);
    bmset(&J->SpecFlags,mjfSystemJob);

    J->DestinationRM = RM;

    MS3AddLocalJob(RM,J->Name);

    R->J = J;
    J->Rsv = R;

    MUStrDup(&R->JName,J->Name);

    /* allow system job to access rsv */

    J->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

    MUStrDup(&J->RsvAList[0],R->Name);

    if (bmisset(&R->Flags,mrfIsVPC))
      {
      /* VPC's are directly charged - do not charge via system job */

      bmset(&J->IFlags,mjifVPCMap);
      }
    }  /* END if (R->J == NULL) */
  else
    {
    J = R->J;

    /* FIXME: need to see if reservation is different than job */

    /* MRsvToJob copies R->NL to J->ReqHList */

    /* this can get lost in MRMJobPostUpdate()->MJobReleaseRsv() */

    MRsvToJob(R,J);

    J->Rsv = R;

    if (!MNLIsEmpty(&R->NL))
      {
      /* FIXME: does not work with multi-req jobs yet */

      MNLCopy(&J->Req[0]->NodeList,&J->ReqHList);
      MNLCopy(&J->NodeList,&J->ReqHList);
      }

    if (MJOBISACTIVE(R->J))
      {
      J->StartTime = R->StartTime;
      }

    J->SpecWCLimit[0] = R->EndTime - R->StartTime;
    J->SpecWCLimit[1] = J->SpecWCLimit[0];
    J->WCLimit        = J->SpecWCLimit[0];
    }

  if ((J != NULL) && (R->VMUsagePolicy != mvmupNONE) && (R->ReqOS > 0) && (bmisset(&R->Flags,mrfIsActive)))
    {
    char EMsg[MMAX_LINE];

    J->VMUsagePolicy = R->VMUsagePolicy;
    J->Req[0]->Opsys = R->ReqOS;

    MRsvCheckAdaptiveState(R,EMsg);
    }

  if (bmisset(&R->Flags,mrfIsActive) && !MJOBISACTIVE(R->J))
    {
    long tmpL;

    J = R->J;

    /* adjust job to start in rsv then restore original attributes */

    if ((J->JGroup == NULL) ||
        (J->JGroup->Name != R->RsvGroup))
      {
      MRsvToJob(R,J);

      MUStrDup(&R->JName,J->Name);
      }

    tmpL = R->EndTime - MSched.Time;

    J->WCLimit = tmpL;
    J->SpecWCLimit[0] = tmpL;
    J->SpecWCLimit[1] = tmpL;

    if (J->Req[0] != NULL)
      {
      /* start the job */

      if (!MNLIsEmpty(&R->NL))
        {
        MNLCopy(&J->Req[0]->NodeList,&J->ReqHList);
        }

      MJobStart(J,NULL,NULL,"rsv system job started");

      MJobSetState(J,mjsRunning);

      J->EState = mjsRunning;

      J->StartTime = R->StartTime;
      J->WCLimit   = R->EndTime - R->StartTime;
      J->SpecWCLimit[0] = J->WCLimit;
      J->SpecWCLimit[1] = J->WCLimit;
      }

    MUIQRemoveJob(R->J);
    }  /* END if (bmisset(&R->Flags,mrfIsActive) && !MJOBISACTIVE(R->J)) */

  if (R->J != NULL)
    bmset(&R->J->IFlags,mjifIsRsvJob);

  return(SUCCESS);
  }  /* END MRsvSyncSystemJob() */



/**
 * Calculates which jobs overlap the reservation and sets its jcount accordingly.
 *
 * @param Rsv The reservation to update.
 */

int MRsvSetJobCount(

  mrsv_t *Rsv) /*I*/

  {
  job_iter JTI;

  int jcount = 0;

  mjob_t *J;

  if (Rsv == NULL)
    return(FAILURE);

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (MJOBISACTIVE(J) == FALSE)
      continue;
    
    if (MRsvHasOverlap(J->Rsv,Rsv,TRUE,&J->NodeList,NULL,NULL) == SUCCESS)
      {
      MDB(7,fSTRUCT) MLog("INFO:     Job %s overlaps reservation %s\n",
        J->Name,
        Rsv->Name);

      jcount++;
      }
    }

  MDB(7,fSTRUCT) MLog("INFO:     setting reservation %s's jobcount to %d\n",
    Rsv->Name,
    jcount);

  Rsv->JCount = jcount;

  return(SUCCESS);
  }  /* END MRsvSetJobCount() */






/**
 * Takes the given reservation, duration, and deadline
 * and finds the soonest the reservation could start given
 * these paramters.
 *
 * @param R (I) [modified]
 * @param Duration (I)
 * @param Deadline (I)
 * @param EMsg [minsize=MMAX_LINE,optional]
 */

int MRsvCreateDeadlineRsv(

  mrsv_t *R,
  long    Duration,
  long    Deadline,
  char   *EMsg)

  {
  long FoundStartTime = -1;
  mjob_t tmpJ;
  mreq_t tmpRQ;
  mnl_t *MNodeList[MMAX_REQ_PER_JOB];
  char tmpTime[MMAX_NAME];
  long tmpStartTime = 0;
  long tmpEndTime = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';
  
  /* create pseudo-job */
  MJobInitialize(&tmpJ);
  memset(&tmpRQ,0,sizeof(tmpRQ));
  tmpJ.Req[0] = &tmpRQ;

  MRsvToJob(R,&tmpJ);
  MJobSetAttr(&tmpJ,mjaPAL,NULL,mdfString,mSet);

  MNLMultiInit(MNodeList);

  if (MJobGetEStartTime(
      &tmpJ,
      NULL,
      NULL,
      NULL,
      MNodeList,
      &FoundStartTime,
      NULL,
      FALSE,
      FALSE,
      EMsg) == FAILURE)
    {
    MNLMultiFree(MNodeList);

    return(FAILURE);
    }

  MNLMultiFree(MNodeList);

  if ((FoundStartTime + Duration) > Deadline)
    {
    /* deadline cannot be met--fail creation */

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"rsv cannot be created before deadline '%s' due to time constraints",
        MULToATString(Deadline,tmpTime));
      }

    return(FAILURE);
    }

  tmpStartTime = FoundStartTime;
  tmpEndTime = tmpStartTime + Duration;

  MRsvSetAttr(R,mraStartTime,(void *)&tmpStartTime,mdfLong,mSet);
  MRsvSetAttr(R,mraEndTime,(void *)&tmpEndTime,mdfLong,mSet);

  return(SUCCESS);
  }  /* END MRsvCreateDeadlineRsv() */



/**
 * Determines if there is an ownerpreempt reservation or not.
 */

mbool_t MRsvOwnerPreemptExists()

  {
  rsv_iter RTI;

  mrsv_t *R;

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (bmisset(&R->Flags,mrfOwnerPreempt) == FALSE)
      continue;

    return(TRUE);
    }  /* END while (MRsvTableIterate()) */

  return(FALSE);
  } /* END MRsvOwnerPreemptExists() */




/**
 * Migrate R's resources to CR's and delete CR.
 *
 * NOTE: this will force things through (not scheduled)
 *       The new rsv will have the same timeframe of R.
 */

int MRsvMigrate(

  mrsv_t *R,
  mrsv_t *CR,
  char   *Msg)

  {
  int nindex;

  int TC;
  int NC;

  char     *CRHostExp = NULL;

  mnl_t  NewNL;

  mnode_t *N;

  MNLInit(&NewNL);

  MNLCopy(&NewNL,&CR->NL);

  if (CR->HostExp != NULL)
    MUStrDup(&CRHostExp,CR->HostExp);

  CR->CancelIsPending = TRUE;

  CR->EndTime = MSched.Time - 1;
  CR->ExpireTime = 0;

  /* remove reservation */

  MCResCopy(&R->DRes,&CR->DRes);

  MRsvCheckStatus(NULL);

  MRsvClearNL(R);

  /* there's a bug in MRsvAdjustNode where it will leave R->AllocPC negative
     don't have time to figure it out now.  Here's how to reproduce it:

     mrsvctl -c -R procs:1 -t1
     mrsvctl -c -R procs:1 -t1
     mrsvctl -c -R procs:1 -t1
     mrsvctl -j system.2 system.1
     mrsvctl -j system.3 system.1 <-- happens during this call */

  for (nindex = 0;MNLGetNodeAtIndex(&NewNL,nindex,&N) == SUCCESS;nindex++)
    {
    /* By default only add one task when using a rsv modify */

    if (MRsvAdjustNode(R,N,MNLGetTCAtIndex(&NewNL,nindex),1,TRUE) == FAILURE)
      {
      continue;
      }

    /* add node to R->NL,R->ReqNL,R->HostExp */

    MNLAdd(&R->NL,N,MNLGetTCAtIndex(&NewNL,nindex));
    MNLAdd(&R->ReqNL,N,MNLGetTCAtIndex(&NewNL,nindex));
    }  /* END for (nindex) */

  MNLFree(&NewNL);

  /* change the reqtc,alloctc,reqnc,allocnc */

  TC = 0;
  NC = 0;

  TC = MNLSumTC(&R->NL);
  NC = MNLCount(&R->NL);

  R->AllocTC = TC;
  R->ReqTC   = TC;
  R->AllocNC = NC;
  R->ReqNC   = NC;

  if ((R->HostExp != NULL) || (CRHostExp != NULL))
    {
    mstring_t NodeString(MMAX_LINE);

    /* if either reservation had an expression we must create a new 
       expression to restore correctly from checkpoint */

    MNLToRegExString(&R->NL,&NodeString);

    MRsvSetAttr(R,mraHostExp,NodeString.c_str(),mdfString,mSet);
    }

  MUFree(&CRHostExp);

  return(SUCCESS);
  }  /* END MRsvMigrate() */






/**
 * Join CR's resources to R's and delete CR.
 *
 * NOTE: this will force things through (not scheduled)
 *       the task structure of both reservations will be
 *       reduced to the lowest common denominator
 *       (1proc/Xmem/Ydisk/Zswap)
 *       The new rsv will have the same timeframe of R.
 *       A new hostexp will be created.
 */

int MRsvJoin(

  mrsv_t *R,
  mrsv_t *CR,
  char   *Msg)

  {
  int nindex;

  int TC;
  int NC;
  int PC;
  int Mem;
  int Disk;

  char     *CRHostExp = NULL;

  mnl_t  NewNL;

  msnl_t GRes;   /* generic resources */

  mnode_t *N;

  if ((R == NULL) || (CR == NULL) || (Msg == NULL) || (R == CR))
    {
    snprintf(Msg,MMAX_LINE,"cannot join reservations (invalid parameters)\n");

    return(FAILURE);
    }

  Mem = 0;

  Mem += R->DRes.Mem * MAX(R->ReqTC,R->AllocTC);
  Mem += CR->DRes.Mem * MAX(CR->ReqTC,CR->AllocTC);

  MSNLInit(&GRes);

  Disk = 0;

  Disk += R->DRes.Disk * MAX(R->ReqTC,R->AllocTC);
  Disk += CR->DRes.Disk * MAX(CR->ReqTC,CR->AllocTC);

  MCResSumGRes(
    &R->DRes.GenericRes,
    MAX(R->ReqTC,R->AllocTC),
    &CR->DRes.GenericRes,
    MAX(CR->ReqTC,CR->AllocTC),
    &GRes);

  PC = 0;

  /* routine assumes reservations are exclusive */
  /* must convert all task structures to 1 proc */

  for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (R->DRes.Procs == -1)
      {
      PC += N->CRes.Procs;

/*
      MNLSetTCAtIndex(&R->NL,nindex,N->CRes.Procs);
*/
      }
    else
      {
      PC += MNLGetTCAtIndex(&R->NL,nindex) * R->DRes.Procs;

/*
      MNLSetTCAtIndex(&R->NL,nindex,MNLGetTCAtIndex(&R->NL,nindex) * R->DRes.Procs);
*/
      }
    }

  for (nindex = 0;MNLGetNodeAtIndex(&CR->NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (CR->DRes.Procs == -1)
      {
      PC += N->CRes.Procs;

/*
      MNLSetTCAtIndex(&CR->NL,nindex,N->CRes.Procs);
*/
      }
    else
      {
      PC += MNLGetTCAtIndex(&CR->NL,nindex) * CR->DRes.Procs;

/*
      MNLSetTCAtIndex(&CR->NL,nindex,MNLGetTCAtIndex(&CR->NL,nindex) * CR->DRes.Procs);
*/
      }
    }

  if ((PC == 0) && (MSNLIsEmpty(&GRes)))
    {
    return(FAILURE);
    }

  MNLInit(&NewNL);

  MNLMerge(&NewNL,&R->NL,&CR->NL,NULL,&NC);

  /* new reservation has normalized task definition */

  if (PC > 0)
    {
    int gindex;

    R->DRes.Procs = 1;
    R->DRes.Mem   = Mem/PC;
    R->DRes.Disk  = Disk/PC;

    for (gindex = 1;MGRes.Name[gindex][0] != '\0';gindex++)
      {
      MSNLSetCount(&R->DRes.GenericRes,gindex,MSNLGetIndexCount(&GRes,gindex) / PC);
      }
    }
  else
    {
    /* only allow the reservations with no procs defined to be joined if they're on the same node */
    if (NC == 1)
      {
      R->DRes.Mem  = Mem;
      R->DRes.Disk = Disk;

      /* this is assuming GRES reservations are only using the global node */
      MSNLCopy(&R->DRes.GenericRes,&GRes);
      }
    else
      {
      snprintf(Msg,MMAX_LINE,"cannot join reservations with generic resources on different nodes\n");

      return(FAILURE);
      }
    }

  if (CR->HostExp != NULL)
    MUStrDup(&CRHostExp,CR->HostExp);

  CR->CancelIsPending = TRUE;

  CR->EndTime = MSched.Time - 1;
  CR->ExpireTime = 0;

  /* remove reservation */

  MRsvCheckStatus(NULL);

  MRsvClearNL(R);

  /* there's a bug in MRsvAdjustNode where it will leave R->AllocPC negative
     don't have time to figure it out now.  Here's how to reproduce it:

     mrsvctl -c -R procs:1 -t1
     mrsvctl -c -R procs:1 -t1
     mrsvctl -c -R procs:1 -t1
     mrsvctl -j system.2 system.1
     mrsvctl -j system.3 system.1 <-- happens during this call */

  if (PC > 0)
    {
    R->ReqTC = PC;
    R->ReqNC = NC;
    }
  else
    {
    /* this is assuming GRES reservations are only using a single node (verified above) */
    R->ReqTC = 1;
    R->ReqNC = 1;

    MNLSetTCAtIndex(&NewNL,0,1);
    }

  for (nindex = 0;nindex < NC;nindex++)
    {
    MNLGetNodeAtIndex(&NewNL,nindex,&N);

    /* By default only add one task when using a rsv modify */

    if (MRsvAdjustNode(R,N,MNLGetTCAtIndex(&NewNL,nindex),1,TRUE) == FAILURE)
      {
      continue;
      }

    /* add node to R->NL,R->ReqNL,R->HostExp */

    MNLAdd(&R->NL,N,MNLGetTCAtIndex(&NewNL,nindex));
    MNLAdd(&R->ReqNL,N,MNLGetTCAtIndex(&NewNL,nindex));
    }  /* END for (nindex) */

  MNLFree(&NewNL);

  /* change the reqtc,alloctc,reqnc,allocnc */

  TC = 0;
  NC = 0;

  TC = MNLSumTC(&R->NL);
  NC = MNLCount(&R->NL);

  R->AllocTC = TC;
  R->ReqTC   = TC;
  R->AllocNC = NC;
  R->ReqNC   = NC;
  R->AllocPC = PC;

  if ((R->HostExp != NULL) || (CRHostExp != NULL))
    {
    mstring_t NodeString(MMAX_LINE);

    /* if either reservation had an expression we must create a new 
       expression to restore correctly from checkpoint */

    MNLToRegExString(&R->NL,&NodeString);

    MRsvSetAttr(R,mraHostExp,NodeString.c_str(),mdfString,mSet);
    }

  MUFree(&CRHostExp);

  return(SUCCESS);
  }  /* END MRsvJoin() */




/**
 * Remove all nodes from reservation.
 */

int MRsvClearNL(

  mrsv_t *R)

  {
  mnode_t *N;
  int TC;

  while (MNLGetNodeAtIndex(&R->NL,0,&N) == SUCCESS)
    {
    TC = MNLGetTCAtIndex(&R->NL,0);

    /* MRsvAdjustNode() has bugs when dealing with non-proc 
       related reservations, to avoid this bug send in the TC
       so MRsvAdjustNode() doesn't try to figure out the TC */

    if (MRsvAdjustNode(R,N,TC,-1,TRUE) == FAILURE)
      {
      MDB(3,fUI) MLog("INFO:     cannot remove node '%s' from rsv %s\n",
        N->Name,
        R->Name);
 
      continue;
      }

    /* remove node from R->NL,R->ReqNL,R->HostExp */

    MNLRemove(&R->NL,N);
    MNLRemove(&R->ReqNL,N);
    }

  R->AllocPC = 0;
  R->AllocTC = 0;

  return(SUCCESS);
  }  /* END MRsvClearNL() */



/**
 * Sum reservation resources and place in mcres_t structure.
 *
 */

int MRsvSumResources(

  mrsv_t  *R,
  mcres_t *Res)

  {
  int gindex;

  if ((R == NULL) || (Res == NULL))
    {
    return(FAILURE);
    }

  Res->Procs = R->AllocPC;
  Res->Mem   = R->DRes.Mem * MAX(R->AllocTC,R->ReqTC);
  Res->Disk  = R->DRes.Disk * MAX(R->AllocTC,R->ReqTC);

  for (gindex = 0;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    MSNLSetCount(&Res->GenericRes,gindex,MSNLGetIndexCount(&R->DRes.GenericRes,gindex) * MAX(R->AllocTC,R->ReqTC));
    }

  return(FAILURE);
  }  /* END MRsvSumResources() */




/**
 * Allocate a specific GRes based on Type.
 *
 * Handles "mrsvctl -A gres.type=dvd vpc.3
 *
 */

int MRsvAllocateGResType(

  mrsv_t *R,
  char   *GResType,
  char   *EMsg)

  {
  int gindex;

  mjob_t tmpJ;
  mreq_t tmpRQ;

  mjob_t *JP;

  mnode_t *N;
  mnode_t *tmpN;

  mcres_t ODRes;

  int      AllocCount;
  int      ReqCount;

  mrange_t RRange[2];
  mrange_t ARange[MMAX_RANGE];

  if ((EMsg == NULL) ||
      (MNLGetNodeAtIndex(&R->NL,0,&N) == FAILURE) ||
      (MNLGetNodeAtIndex(&R->NL,1,NULL) == SUCCESS))
    {
    sprintf(EMsg,"ERROR: invalid reservation specified\n");

    return(FAILURE);
    }

  if (GResType[0] == '\0')
    {
    sprintf(EMsg,"ERROR:  no allocation resource specified\n");

    return(FAILURE);
    }

  /* create pseudo job */

  memset(&tmpJ,0,sizeof(tmpJ));
  memset(&tmpRQ,0,sizeof(tmpRQ));

  tmpJ.Req[0] = &tmpRQ;

  MRsvToJob(R,&tmpJ);

  JP = &tmpJ;

  if (MSched.Time > R->StartTime)
    {
    JP->WCLimit        = R->EndTime - MSched.Time;
    JP->SpecWCLimit[0] = JP->WCLimit;
    JP->SpecWCLimit[1] = JP->WCLimit;
    }

  MCResCopy(&ODRes,&tmpRQ.DRes);

  /* need to set everything but gres to 0: NYI */

  ODRes.Procs = 0;

  ReqCount = 0;

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    if (MSNLGetIndexCount(&ODRes.GenericRes,gindex) == 0)
      continue;

    if (strcmp(MGRes.Name[gindex],GResType))
      continue;

    ReqCount += MSNLGetIndexCount(&ODRes.GenericRes,gindex);

    /* do not reallocate generic resource type master */

    MSNLSetCount(&ODRes.GenericRes,gindex,0);
    }  /* END for (gindex) */

  /* verify node has at least 1 configured matching generic resource */

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    if (strcmp(MGRes.GRes[gindex]->Type,GResType))
      continue;

    if (MSNLGetIndexCount(&N->CRes.GenericRes,gindex) > 0)
      break;
    }

  if ((gindex >= MSched.M[mxoxGRes]) || (MSNLGetIndexCount(&N->CRes.GenericRes,gindex) <= 0))
    {
    sprintf(EMsg,"ERROR:  cannot locate available generic resource of type %s\n",
      GResType);

    MJobDestroy(&JP);

    return(FAILURE);
    }

  /* assume solution requires node match */

  RRange[0].STime = MAX(MSched.Time,R->StartTime);
  RRange[0].ETime = MAX(MSched.Time,R->EndTime);
  RRange[0].TC    = 1;

  RRange[1].ETime = 0;

  /* verify node can support task */

  if (MNodeGetTC(N,&N->ARes,&N->CRes,&N->DRes,&tmpRQ.DRes,MMAX_TIME,1,NULL) == 0)
    {
    sprintf(EMsg,"ERROR:  cannot locate available generic resource of type %s\n",
      GResType);

    MJobDestroy(&JP);

    return(FAILURE);
    }

  AllocCount = 0;

  /* search through all matching gres instances, find gres which fits in appropriate time frame */

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    if (strcmp(MGRes.GRes[gindex]->Type,GResType))
      continue;

    MCResCopy(&tmpRQ.DRes,&ODRes);

    MSNLSetCount(&tmpRQ.DRes.GenericRes,gindex,1);

    MDB(7,fSCHED) MLog("INFO:      attempting to use '%s' on node '%s' for rsv '%s'\n",
      MGRes.Name[gindex],
      N->Name,
      R->Name);

    if (MJobGetSNRange(
         JP,
         &tmpRQ,
         N,
         RRange,
         1,
         NULL,  /* O (affinity) */
         NULL,  /* O (type) */
         ARange,
         NULL,
         TRUE,
         NULL) == FAILURE)
      {
      /* requested gres not available during rsv lifetime */

      continue;
      }

    /* make sure returned range is valid */

    if ((ARange[0].STime > RRange[0].STime) ||
        (ARange[0].ETime < RRange[0].ETime))
      {
      continue;
      }

    /* match located */

    MSNLSetCount(&R->DRes.GenericRes,gindex,1);

    /* adjust N->RE as needed */

    if (MNLGetNodeAtIndex(&R->NL,0,&tmpN) == SUCCESS)
      tmpN->RE = MRESort(tmpN->RE);

    sprintf(EMsg,"%s",
      MGRes.Name[gindex]);

    AllocCount++;

    if (AllocCount >= ReqCount)
      {
      MJobDestroy(&JP);

      return(SUCCESS);
      }
    }  /* END for (gindex) */
  
  MJobDestroy(&JP);

  sprintf(EMsg,"ERROR:  cannot locate available generic resource of type %s\n",
    GResType);

  return(FAILURE);
  }  /* END MRsvAllocateGRes() */




int MRsvTableDumpToLog(

  const char *Function,
  int         Line)

  {
  mrsv_t *R;

  rsv_iter RTI;

  if (!bmisset(&MSched.Flags,mschedfRsvTableLogDump))
    {
    return(SUCCESS);
    }

  MRsvIterInit(&RTI);

  MLog("INFO:     dumping reservations on iteration %d from %s:%d\n",
    MSched.Iteration,
    Function,
    Line);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    MLog("INFO:     Name='%s' RsvGroup='%x'\n",
      R->Name,
      R->RsvGroup);
    }
  
  return(SUCCESS);
  }



/**
 * Populate a reservation with the nodes from the checkpointed host expression.
 * 
 * Does not work with job reservations.
 *
 * @param R (I) NL modified
 */

int MRsvPopulateNL(
    
  mrsv_t *R)

  {
  char       tmpMsg[MMAX_LINE];
  int        nindex;
  int        TC;
  mnode_t   *N;

  if ((R == NULL) ||
      (R->Type == mrtJob) ||
      (R->HostExp == NULL))
    {
    return(FAILURE);
    }
      
  if (MNLIsEmpty(&R->ReqNL))
    {
    marray_t NodeList;

    /* non-hostlist reservation, hostexp set for 1st iteration. */

    mstring_t tmpHostExp(R->HostExp);

    tmpMsg[0] = '\0';

    MUArrayListCreate(&NodeList,sizeof(mnode_t *),-1);

    if (MUREToList(
          tmpHostExp.c_str(),
          mxoNode,
          NULL,
          &NodeList,
          FALSE,
          tmpMsg,
          sizeof(tmpMsg)) == FAILURE)
      {
      MUArrayListFree(&NodeList);

      MDB(2,fSTRUCT) MLog("ERROR:    host expression '%s' - '%s'\n",
        R->HostExp,
        tmpMsg);

      return(FAILURE);
      }

    for (nindex = 0;nindex < NodeList.NumItems;nindex++)
      {
      N = (mnode_t *)MUArrayListGetPtr(&NodeList,nindex);

      TC = MNodeGetTC(
          N,
          &N->CRes,
          &N->CRes,
          &N->DRes,
          &R->DRes,
          MMAX_TIME,
          1,
          NULL);
   
      TC = MAX(TC,1);
   
      if (R->ReqTC > 0)
        TC = MIN(TC,R->ReqTC - R->AllocTC);
   
      if (MRsvAdjustNode(R,N,TC,0,FALSE) == FAILURE)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot add node %s to reservation %s\n",
          N->Name,
          R->Name);
   
        continue;
        }
   
      R->AllocNC ++;
   
      R->AllocTC += TC;
   
      R->AllocPC += (R->DRes.Procs == -1) ?
        N->CRes.Procs :
        (TC * R->DRes.Procs);
   
      MNLSetNodeAtIndex(&R->NL,nindex,N);
      MNLSetTCAtIndex(&R->NL,nindex,TC);

      MNLTerminateAtIndex(&R->NL,nindex +1);
      }

    MUArrayListFree(&NodeList);
    }
  else
    {
    /* hostlist reservation, ReqNL contains nodes from checkpoint */
    
    for (nindex = 0;MNLGetNodeAtIndex(&R->ReqNL,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(&R->ReqNL,nindex);
   
      if (R->ReqTC > 0)
        TC = MIN(TC,R->ReqTC - R->AllocTC);
   
      if (MRsvAdjustNode(R,N,TC,0,FALSE) == FAILURE)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot add node %s to reservation %s\n",
          N->Name,
          R->Name);
   
        continue;
        }
   
      R->AllocNC ++;
   
      R->AllocTC += TC;
   
      R->AllocPC += (R->DRes.Procs == -1) ?
        N->CRes.Procs :
        (TC * R->DRes.Procs);
   
      MNLSetNodeAtIndex(&R->NL,nindex,N);
      MNLSetTCAtIndex(&R->NL,nindex,TC);

      MNLTerminateAtIndex(&R->NL,nindex + 1);
      }
    }  /* END else */

  return(SUCCESS);
  } /* END int MRsvPopulateNL() */


/**
 * see MSRUpdateGridSandboxes()
 *
 * @param GridSandboxIsGlobal (O) [optional]
 */

int MRsvUpdateGridSandboxes(

  mbool_t *GridSandboxIsGlobal)  /* O (optional) */

  {
  rsv_iter RTI;

  mbool_t IsGlobal = FALSE;
  mrsv_t *R;

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (bmisset(&R->Flags,mrfAllowGrid))
      {
      MACLCheckGridSandbox(R->ACL,&IsGlobal,NULL,mSet);

      if (IsGlobal == TRUE)
        break;
      }
    }  /* END while (MRsvTableIterate()) */

  if (GridSandboxIsGlobal != NULL)
    *GridSandboxIsGlobal = IsGlobal;

  return(SUCCESS);  
  }    /* END MRsvUpdateGridSandboxes() */

/* END MRsv.c */
