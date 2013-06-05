/* HEADER */

      
/**
 * @file MNodeNL.c
 *
 * Contains: Node List functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Set specified node attribute on nodelist.
 *
 * @see MNodeAToString() - report node attribute as string
 * @see MNodeProcessConfig() - process NODECFG config line
 * @see MNodeSetAttr() - set an attribute on an NL in one operation
 *
 * @param NL (I) [modified]
 * @param AIndex (I)
 * @param Value (I) [modified as side-effect]
 * @param Format (I)
 * @param Mode (I)
 */


int MNLSetAttr(

  mnl_t                  *NL,
  enum MNodeAttrEnum      AIndex,
  void                  **Value,
  enum MDataFormatEnum    Format,
  enum MObjectSetModeEnum Mode)

  {
  const char *FName = "MNLSetAttr";

  mnode_t *N;

  MDB(6,fSTRUCT) MLog("%s(%s,%s,Value,%s,%s)\n",
    FName,
    (NL != NULL) ? "NL" : "NULL",
    MNodeAttr[AIndex],
    MFormatMode[Format],
    MObjOpType[Mode]);

  if (NL == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mnaPowerIsEnabled:

      {
      int nindex;

      /* synchronize with ModeSetAttr(mnaPowerIsEnabled) */
      /* FORMAT:  <BOOLEAN> */

      mbool_t PowerIsEnabled;

      enum MPowerStateEnum PowerState = mpowNONE;

      PowerIsEnabled = MUBoolFromString((char *)Value,TRUE);

      if (MNLIsEmpty(NL))
        return(SUCCESS);

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        if (Mode != mVerify)
          {
          /* no external action specified, make local changes only */

          N->PowerIsEnabled = PowerIsEnabled;
   
          if (PowerIsEnabled == TRUE)
            {
            if ((N->PowerState == mpowNONE) || (N->PowerState == mpowOff))
              N->PowerState = mpowOn;
            }
          else if (PowerIsEnabled == FALSE)
            {
            if ((N->PowerState == mpowNONE) || (N->PowerState == mpowOn))
              N->PowerState = mpowOff;
            }
   
          return(SUCCESS);
          }
   
        switch (PowerIsEnabled)
          {
          case TRUE:
   
            PowerState = mpowOn;
   
            break;
   
          case FALSE:
   
            PowerState = mpowOff;
   
            break;
   
          default:
   
            PowerState = mpowOn;
   
            break;
          }
        }   /* END for (nindex) */

      if (MRMNodeSetPowerState(NULL,NL,NULL,PowerState,NULL) == FAILURE)
        {
        /* indicates RM communication failure */
      
        return (FAILURE);
        } /* END if (MRMNodeSetPowerState(...) == FAILURE) */
      } /* END BLOCK (case mnaPowerIsEnabled) */

      break;

    default:

      /* NO-OP */

      break;
    }   /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MNLSetAttr() */



  

/**
 * Get active job list running on specified nodes.
 *
 * NOTE:  JList will be NULL terminated
 *
 * @param NL (I)
 * @param NLPHP (O) [optional] total prochours remaining associated with jobs on nodes
 * @param JList (O)
 * @param JCountP (O) [optional]
 * @param JListSize (I)
 */

int MNLGetJobList(

  mnl_t      *NL,         /* I */
  long       *NLPHP,      /* O (optional) total prochours remaining associated with jobs on nodes */
  mjob_t    **JList,      /* O */
  int        *JCountP,    /* O (optional) */
  int         JListSize)  /* I */

  {
  int jlindex;
  int jindex;
  int nindex;

  mnode_t *N;
  mjob_t  *J;

  long     NLPH;

  if (NLPHP != NULL)
    *NLPHP = 0;

  if (JCountP != NULL)
    *JCountP = 0;

  if ((NL == NULL) || (JList == NULL))
    {
    return(FAILURE);
    }

  JList[0] = NULL;

  NLPH = 0;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    for (jindex = 0;jindex < N->MaxJCount;jindex++)
      {
      if (N->JList[jindex] == NULL)
        break;

      if (N->JList[jindex] == (mjob_t *)1)
        continue;

      J = N->JList[jindex];

      /* NOTE:  assume dedicated node for calculating NLPH (FIXME) */

      NLPH += N->CRes.Procs * J->RemainingTime;
 
      for (jlindex = 0;jlindex < JListSize;jlindex++)
        {
        if (JList[jlindex] == NULL)
          {
          /* end of list located */

          JList[jlindex] = J;

          if (JCountP != NULL)
            *JCountP = jlindex + 1;

          if (jlindex >= JListSize - 1)
            {
            /* buffer is full */

            return(FAILURE);
            }

          /* terminate buffer */

          JList[jlindex + 1] = NULL;

          break; 
          }

        if (JList[jlindex] == J)
          {
          /* list already contains job */

          break;            
          }
        }  /* END for (jlindex) */
      }    /* END for (jindex) */
    }      /* END for (nindex) */

  if (NLPHP != NULL)
    *NLPHP = NLPH;

  return(SUCCESS);
  }  /* END MNLGetJobList() */


/**
 * Sorting routing to sort by NodeIndex from lowest to highest.
 */

int __MNLSortByNodeIndex(

  mnalloc_old_t *A,
  mnalloc_old_t *B)

  {
  /* order low to high */
  return(A->N->NodeIndex - B->N->NodeIndex);
  }


/**
 * Returns a subset of NodeList that is a continguous in noderange which contains 
 * the fewest amount of resources to fullfill the taskcount (BestFit).
 *
 * Requires N->NodeIndex be set and unique for each node.
 *
 * @param PIndex (I)
 * @param MinTC (I) [minimum tasks required per range]
 * @param NodeList (I) [terminated w/N==NULL]
 * @param AdjNodeList (O) [terminated w/N==NULL]
 */

int MNLSelectContiguousBestFit(

  int         PIndex,                    /* I */
  int         MinTC,                     /* I (minimum tasks required per range) */
  mnl_t      *NodeList,                  /* I (terminated w/N==NULL) */
  mnl_t      *AdjNodeList)               /* O */

  {
  int rc = SUCCESS;
  int nindex;
  int addedCount = 0;

  int firstRIndex = 0; /* first node of range index */
  int startIndex = 0;
  int endIndex = 0;
  int prevNIndex = 0;
  int rangeTC = 0;

  int BestRTC;

  mnl_t    tmpNodeList = {0};

  mnode_t *N;

  MASSERT(NodeList    != NULL,"NodeList is NULL when selecting adjacent nodes.");
  MASSERT(AdjNodeList != NULL,"Adjacent NodeList is NULL when selecting adjacent nodes.");

  MNLInit(&tmpNodeList);

  /* sort available nodes into ranges */
  for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    if (N->PtIndex != PIndex)
      continue;

    MNLCopyIndex(&tmpNodeList,addedCount++,NodeList,nindex);
    }  /* END for (nindex) */

  if (MNLIsEmpty(&tmpNodeList) == TRUE)
    {
    MNLFree(&tmpNodeList);
    return(FAILURE);
    }

  /* qsort by NodeIndex low to high */
  if (addedCount > 1)
    {
    qsort(
      (void *)tmpNodeList.Array,
      addedCount,
      sizeof(mnalloc_old_t),
      (int(*)(const void *,const void *))__MNLSortByNodeIndex);
    }

  BestRTC = -1;

  /* locate contiguous ranges */
  for (nindex = 0;nindex <= MNLSize(&tmpNodeList);nindex++)
    {
    MNLGetNodeAtIndex(&tmpNodeList,nindex,&N);

    /* keep adding to range if node is adjacenet */
    if ((nindex != 0) && /* ignore first node */
        (N != NULL) &&
        (N->NodeIndex == prevNIndex + 1))
      {
      prevNIndex++;
      rangeTC += MNLGetTCAtIndex(&tmpNodeList,nindex);
      continue;
      }
    else if (nindex != 0) /* ignore first node */
      {
      /* save off range if it has enough tasks */
      if ((MinTC <= rangeTC) &&
          ((BestRTC == -1) || (rangeTC < BestRTC))) /* Find first bestfit range */
        {
        startIndex = firstRIndex;
        endIndex   = nindex;
        BestRTC    = rangeTC;
        } /* END if (MinTC <= rangeTC) */
      } /* END else if (nindex != 0) */

    if (N == NULL)
      break;

    /* start new range */
    firstRIndex = nindex;
    prevNIndex  = N->NodeIndex;
    rangeTC     = MNLGetTCAtIndex(&tmpNodeList,nindex);
    }  /* END for (nindex) */

  if (BestRTC == -1)
    {
    rc = FAILURE; /* no range found */
    }
  else
    {
    /* copy range into return list */
    int copyFrom;
    int copyTo = 0;

    for (copyFrom = startIndex;copyFrom < endIndex;copyFrom++)
      MNLCopyIndex(AdjNodeList,copyTo++,&tmpNodeList,copyFrom);

    MNLTerminateAtIndex(AdjNodeList,copyTo);
    }

  MNLFree(&tmpNodeList);

  return(rc);
  }  /* END MNLSelectAdjacentNodes() */





/**
 * Reserve node for specific purpose (typically failure or trigger based rsv).
 *
 * @see MNodeReserve()
 *
 * @param NL (I)
 * @param Duration (I) [if <= 0, take no action]
 * @param Name (I) [optional]
 * @param RsvProfile (I) [optional]
 * @param Msg (I) [optional]
 */

int MNLReserve(

  mnl_t       *NL,
  mulong       Duration,
  const char  *Name,
  char        *RsvProfile,
  const char  *Msg)

  {
  char    RName[MMAX_LINE];

  mrsv_t *R;
  mrsv_t *RProf = NULL;

  mnl_t     tmpNL;

  mulong    EndTime;

  mbitmap_t tmpFlags;

  char EMsg[MMAX_LINE];

  if (Duration <= 0)
    {
    return(SUCCESS);
    }

  if ((Name != NULL) && (Name[0] != '\0'))
    {
    snprintf(RName,sizeof(RName),"%s.%d",
      Name,
      MSched.RsvCounter++);
    }
  else
    {
    snprintf(RName,sizeof(RName),"maintenance.%d",
      MSched.RsvCounter++);
    }

  MNLInit(&tmpNL);

  MNLCopy(&tmpNL,NL);

  if (MRsvFind(RName,&R,mraNONE) == SUCCESS)
    {
    MRsvDestroy(&R,TRUE,TRUE);
    }

  if (RsvProfile != NULL)
    MRsvProfileFind(RsvProfile,&RProf);

  EndTime = MSched.Time + ((RProf != NULL) ? (mulong)RProf->Duration : Duration);

  if (MRsvCreate(
        mrtUser,                                /*  Type         */
        (RProf != NULL) ? RProf->ACL : NULL,    /*  ACL          */
        NULL,                                   /*  CL           */
        (RProf != NULL) ? &RProf->Flags : &tmpFlags, /* Flags      */
        NL,                                     /*  NodeList     */
        MSched.Time,                            /*  StartTime    */
        EndTime,                                /*  EndTime      */
        1,                                      /*  NC           */
        -1,                                     /*  PC           */
        RName,                                  /*  RBaseName    */
        &R,                                     /*  RsvP         */
        NULL,                                   /*  HostExp      */
        NULL,                                   /*  DRes         */
        EMsg,                                   /*  EMsg         */
        TRUE,                                   /*  DoRemote     */
        TRUE) == FAILURE)                       /*  DoAlloc      */
    {
    MNLFree(&tmpNL);

    return(FAILURE);
    }

  if (Msg != NULL)
    {
    MRsvSetAttr(R,mraComment,(void *)Msg,mdfString,mSet);
    }

  if ((RProf != NULL) && (RProf->T != NULL))
    {
    /* alloc triggers */

    R->T = NULL;

    MTListCopy(RProf->T,&R->T,TRUE);
    }

  if (R->T != NULL)
    {
    mtrig_t *T;

    int tindex;

    for (tindex = 0; tindex < R->T->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(R->T,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      /* update trigger OID with new RID */

      T->O = (void *)R;
      T->OType = mxoRsv;

      MUStrDup(&T->OID,R->Name);

      MTrigInitialize(T);
      }    /* END for (tindex) */

    /* report events for admin reservations */

    MOReportEvent((void *)R,R->Name,mxoRsv,mttCreate,R->CTime,TRUE);
    MOReportEvent((void *)R,R->Name,mxoRsv,mttStart,R->StartTime,TRUE);
    MOReportEvent((void *)R,R->Name,mxoRsv,mttEnd,R->EndTime,TRUE);
    }      /* END if (R->T != NULL) */

  MNLFree(&tmpNL);

  return(SUCCESS);
  }  /* END MNLReserve() */





/**
 *
 *
 * @param NL (I)
 * @param Message (I) preemption reason [optional]
 * @param SPreemptPolicy (I)
 * @param DoBestEffort (I) [continue preemption on partial failure]
 */

int MNLPreemptJobs(

  mnl_t                   *NL,
  const char              *Message,
  enum MPreemptPolicyEnum  SPreemptPolicy,
  mbool_t                  DoBestEffort)

  {
  int jindex;
  int jindex2;
  int nindex;

  int JC;

  mnode_t *N;

  char EMsg[MMAX_LINE];

  mbool_t FailureDetected;

  static mjob_t **JList = NULL;

  enum MPreemptPolicyEnum PreemptPolicy;

  const char *FName = "MNLPreemptJobs";

  if ((NL == NULL) || (MNLIsEmpty(NL)))
    {
    return(SUCCESS);
    }

  PreemptPolicy = (SPreemptPolicy != mppNONE) ? SPreemptPolicy : MSched.PreemptPolicy;

  JC = 0;

  if ((JList == NULL) &&
      (MJobListAlloc(&JList) == FAILURE))
    {
    return(FAILURE);
    }

  JList[0] = NULL;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (N->JList == NULL)
      continue;

    for (jindex = 0;jindex < N->MaxJCount;jindex++)
      {
      if (N->JList[jindex] == NULL)
        break;

      if (N->JList[jindex] == (mjob_t *)1)
        continue;

      for (jindex2 = 0;JList[jindex] != NULL;jindex2++)
        {
        if (JList[jindex] == N->JList[jindex])
          {
          /* job previously added to list */

          break;
          }
        }

      if (JList[jindex2] == NULL)
        {
        /* job not previously found - add to list */

        JList[JC] = N->JList[jindex];

        JC++;

        break;
        }
      }    /* END for (jindex) */ 
    }      /* END for (nindex) */

  FailureDetected = FALSE;

  for (jindex = 0;jindex < JC;jindex++)
    {
    if (MJobPreempt(
          JList[jindex],
          NULL,
          NULL,
          NULL,
          Message,
          PreemptPolicy,
          FALSE,          /* FIXME:  must determine if request is admin or non-nonadmin */
          NULL,
          EMsg,
          NULL) == FAILURE)
      {
      MDB(2,fCORE) MLog("ALERT:    cannot preempt job '%s' in %s (EMsg='%s')\n",
        JList[jindex]->Name,
        FName,
        EMsg);

      if (DoBestEffort == FALSE)
        {
        return(FAILURE);
        }

      FailureDetected = TRUE;
      }
    }  /* END for (jindex) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNLPreemptJobs() */





/**
 * Adjusts the Available/Dedicated resources of a NodeList. 
 *
 * @see MNodeGetEffNAPolicy() - child
 * 
 * @param NL (I) The list of nodes to adjust resources for.
 * @param NAccessPolicy (I) Don't know.
 * @param DRes (I) The dedicated resources to adjust
 * @param TCP (O, Optional) Task Count?
 * @param NCP (O, Optional) Node Count?
 * @param TotalNodeLoadP (O, Optional) Don't know. 
 */

int MNLAdjustResources(

  mnl_t                      *NL,              /* I */
  enum MNodeAccessPolicyEnum  NAccessPolicy,   /* I */
  mcres_t                     DRes,            /* I */
  int                        *TCP,             /* O (optional) */
  int                        *NCP,             /* O (optional) */
  double                     *TotalNodeLoadP)  /* O (optional) */

  {
  int    ReqNC = 0;
  int    ReqTC = 0;
  double TotalNodeLoad = 0.0;

  double NodeLoad;
  double MinProcSpeed = 1.0;

  int TC;

  int nindex;

  mnode_t *N;

  const char *FName = "MNLAdjustResources";

  MDB(1,fCORE) MLog("%s(NL,%s,DRes,TCP,NCP,TotalNodeLoadP)\n",
    FName,
    MNAccessPolicy[NAccessPolicy]);

  if (TCP != NULL)
    *TCP = 0;

  if (NCP != NULL)
    *NCP = 0;

  if (TotalNodeLoadP != NULL)
    *TotalNodeLoadP = 0.0;

  if (NL == NULL)
    {
    return(FAILURE);
    }

  MUNLGetMinAVal(NL,mnaProcSpeed,NULL,(void **)&MinProcSpeed);

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    mbitmap_t tmpL;

    ReqNC++;
    ReqTC += MNLGetTCAtIndex(NL,nindex);

    N->TaskCount += MNLGetTCAtIndex(NL,nindex);

    /* adjust available resources, values overwritten on next iteration */

    bmset(&tmpL,mrProc);
    bmset(&tmpL,mrMem);
    bmset(&tmpL,mrDisk);
    bmset(&tmpL,mrSwap);
    /*bmset(&tmpL,mrGRes); We do not want to remove GRES here. It is done in MJobAddtoNL() */
 
    if ((NAccessPolicy == mnacSingleJob) ||
        (NAccessPolicy == mnacSingleTask))
      {
      MCResAdd(&N->DRes,&N->CRes,&N->CRes,1,FALSE);

      if (N != MSched.GN)
        {
        /* NOTE:  global node resource availability handled in MJobAddtoNL() */

        MCResRemoveSpec(&N->ARes,&N->CRes,&N->CRes,1,TRUE,&tmpL);
        }

      NodeLoad = (double)N->CRes.Procs;
      }
    else
      {
      TC = MNLGetTCAtIndex(NL,nindex);

      /* node sharing - only remove request resources */

      MCResAdd(&N->DRes,&N->CRes,&DRes,TC,FALSE);

      if (N != MSched.GN)
        {
        /* NOTE:  global node resource availability handled in MJobAddtoNL() */

        MCResRemoveSpec(&N->ARes,&N->CRes,&DRes,TC,TRUE,&tmpL);
        }

      if (DRes.Procs >= 0)
        NodeLoad = (double)TC * DRes.Procs;
      else
        NodeLoad = (double)N->CRes.Procs;
      }

    /* temporarily set node state to eliminate diagnostic warnings
       (possible side-effects?) */

    if (N->State == mnsIdle)
      {
      if ((N->DRes.Procs > 0) &&
          (N->DRes.Procs == N->CRes.Procs) &&
          (MPar[0].NAvailPolicy[mrProc] != mrapUtilized))
        {
        MNodeSetState(N,mnsBusy,FALSE);
        }
      else
        {
        MNodeSetState(N,mnsActive,FALSE);
        }
      }

    if (MinProcSpeed > 2)
      {
      /* scale load to account for parallel job synchronization delays */

      if (MinProcSpeed != N->ProcSpeed)
        {
        /* job is heterogeneous */

        /* NO-OP */
        }

      NodeLoad *= (double)MinProcSpeed / MAX(MinProcSpeed,N->ProcSpeed);
      }  /* END if (MinProcSpeed > 0) */

    N->Load += MAX(0,NodeLoad);

    TotalNodeLoad += MAX(0,NodeLoad);

    /* set expected state */

    MNodeAdjustState(N,&N->EState);

    N->SyncDeadLine = MSched.Time + MSched.NodeSyncDeadline;

    N->StateMTime   = MSched.Time;

    MNodeGetEffNAPolicy(
      N,
      (N->EffNAPolicy != mnacNONE) ? N->EffNAPolicy : N->SpecNAPolicy,
      MPar[N->PtIndex].NAccessPolicy,
      NAccessPolicy,
      &N->EffNAPolicy);

    MDB(7,fRM)  MLog("INFO:     node '%s' effective node access policy %s\n",
      N->Name,
      MNAccessPolicy[N->EffNAPolicy]);

    MDB(7,fRM)  MLog("INFO:     partition '%s' node access policy %s\n",
      MPar[N->PtIndex].Name,
      MNAccessPolicy[MPar[N->PtIndex].NAccessPolicy]);

/*
    if (N == MSched.GN)
      J->FNC++;
*/
    }   /* END for (nindex)  */

  if (TCP != NULL)
    *TCP = ReqTC;

  if (NCP != NULL)
    *NCP = ReqNC;

  if (TotalNodeLoadP != NULL)
    *TotalNodeLoadP = TotalNodeLoad;

  return(SUCCESS);
  }  /* END MNLAdjustResources() */





/**
 * Gets nodes ready, starting provisioning jobs to OSName or poweron jobs
 * as necessary.
 *
 * @param NL     (I)
 * @param OSName (I) [optional]
 * @param EMsg   (I) [optional,minsize=MMAX_LINE]
 */

int MNLGetemReady(

  mnl_t      *NL,
  char       *OSName,
  char       *EMsg)

  {
  mnl_t  NeedsProv = {0};
  mnl_t  NeedsPower = {0};

  int OS;
  int nindex;
  int provindex = 0;
  int powerindex = 0;
  mnode_t *N;
  char tmpEMsg[MMAX_LINE];

  if (EMsg == NULL)
    EMsg = tmpEMsg;

  if ((OSName == NULL) || !strcmp(OSName,MAList[meOpsys][0]))
    {
    OS = 0;
    }
  else
    {
    OS = MUMAGetIndex(meOpsys,OSName,mVerify);

    if (OS == 0)
      return(FAILURE);
    }

  MNLInit(&NeedsProv);
  MNLInit(&NeedsPower);

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (N == MSched.GN)
      continue;

    if (((OS == 0) || (N->ActiveOS == OS)) &&
        (N->PowerSelectState == mpowOff))
      {
      MNLSetNodeAtIndex(&NeedsPower,powerindex,N);
      MNLSetTCAtIndex(&NeedsPower,powerindex++,1);
      }
    else if ((OS != 0) && 
             (MNodeGetSysJob(N,msjtOSProvision,TRUE,NULL) == FAILURE))
      {
      /* we don't need to check if the provisioning job is to the right OS here,
         that is handled elsewhere -- MJobGetSNRange() and MNodeGetAdaptiveAccessPolicy() */

      MNLSetNodeAtIndex(&NeedsProv,provindex,N);
      MNLSetTCAtIndex(&NeedsProv,provindex++,1);
      }
    }

  if (powerindex > 0)
    {
    if (MNLSetAttr(NL,mnaPowerIsEnabled,(void **)"true",mdfString,mVerify) == FAILURE)
      {
      snprintf(EMsg,MMAX_LINE,"ERROR:   could not power on nodes");

      MNLFree(&NeedsProv);
      MNLFree(&NeedsPower);

      return(FAILURE);
      }
    }

  if (provindex > 0)
    {
    mstring_t NeedsProvIDs(MMAX_LINE);

    MNLToMString(&NeedsProv,FALSE,",",'\0',0,&NeedsProvIDs);

    if (MNodeProvision(NULL,NULL,NeedsProvIDs.c_str(),OSName,TRUE,EMsg,NULL) == FAILURE)
      {
      MNLFree(&NeedsProv);
      MNLFree(&NeedsPower);

      return(FAILURE);
      }
    }

  MNLFree(&NeedsProv);
  MNLFree(&NeedsPower);

  return(SUCCESS);
  }  /* END MNodeGetemReady() */



/**
 *
 *
 */

int MNLProvisionAddToBucket(

  mnl_t     *INL,
  char      *NodeString,
  int        OS)

  {
  int oindex;
  int nindex;

  mnl_t   *NL;
  mnl_t    LNL = {0};

  mnode_t *N;

  if ((INL == NULL) && (NodeString == NULL))
    {
    return(FAILURE);
    }

  if (INL != NULL)
    {
    NL = INL;
    }
  else
    {
    MNLInit(&LNL);

    MNLFromString(NodeString,&LNL,1,FALSE);

    NL = &LNL;
    }

  for (oindex = 0;oindex < MMAX_ATTR;oindex++)
    {
    if (MNLProvBucket[oindex] == NULL)
      break;

    if (MNLProvBucket[oindex]->OS != OS)
      continue;
   
    if (MNLIsEmpty(&MNLProvBucket[oindex]->NL))
      {
      MNLProvBucket[oindex]->LastFlushTime = MSched.Time;
      }

    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      MNLAdd(&MNLProvBucket[oindex]->NL,N,1);
      }

    return(SUCCESS);
    } /* END for (oindex) */

  if (oindex >= MMAX_ATTR)
    {
    return(FAILURE);
    }
  else if (MNLProvBucket[oindex] == NULL)
    {
    MNLProvBucket[oindex] = (mnlbucket_t *)MUCalloc(1,sizeof(mnlbucket_t));

    MNLProvBucket[oindex]->OS = OS;

    MNLClear(&MNLProvBucket[oindex]->NL);

    MNLProvBucket[oindex]->LastFlushTime = MSched.Time;

    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      MNLAdd(&MNLProvBucket[oindex]->NL,N,1);
      }

    return(SUCCESS);
    }

  return(FAILURE); 
  }  /* END MNLProvisionAddToBucket() */





int MNLProvisionFlushBuckets()

  {
  int oindex;

  mstring_t NodeList(MMAX_LINE);

  mnode_t *N;

  char EMsg[MMAX_LINE];

  for (oindex = 0;oindex < MMAX_ATTR;oindex++)
    {
    if (MNLProvBucket[oindex] == NULL)
      break;

    if (MNLIsEmpty(&MNLProvBucket[oindex]->NL))
      continue;

    if ((MNLProvBucket[oindex]->LastFlushTime > 0) &&
        ((MSched.Time - MNLProvBucket[oindex]->LastFlushTime) < (mulong)MSched.AggregateNodeActionsTime))
      continue;

    MNLGetNodeAtIndex(&MNLProvBucket[oindex]->NL,0,&N);

    NodeList.clear();

    MNLToMString(&MNLProvBucket[oindex]->NL,FALSE,",",'\0',1,&NodeList);

    if (MRMNodeModify(
          N,
          NULL,
          NodeList.c_str(),
          N->RM,
          mnaOS,
          NULL,
          MAList[meOpsys][MNLProvBucket[oindex]->OS],
          FALSE,
          mSet,
          EMsg,
          NULL) == SUCCESS)
      {
      MNLClear(&MNLProvBucket[oindex]->NL);

      MNLProvBucket[oindex]->LastFlushTime = MSched.Time;
      }
    }  /* END for (oindex) */

  return(SUCCESS);
  }  /* END MNLProvisionFlushBuckets() */





int MNLProvisionFreeBuckets()

  {
  int oindex;

  for (oindex = 0;oindex < MMAX_ATTR;oindex++)
    {
    if (MNLProvBucket[oindex] == NULL)
      break;

    MUFree((char **)&MNLProvBucket[oindex]);
    }  /* END for (oindex) */

  return(SUCCESS);
  }  /* END MNLProvisionFreeBuckets() */


/**
 * Generates a node list that is constrained by nodeset (for vlans).
 *
 * @param nsNL (O) Used to return the list of nodes in the set.
 * @param nsAttr (I) The key that identifies the vlan (feature) constraint.
 * @param nsListSize (I) Size of nsNL array.
 */

int MNLNodeSetByVLAN(

  mnl_t    *nsNL,
  char     *nsAttr,    /* nodeset attribute for key */
  int       nsListSize)

  {
  mjob_t FakeJ;
  msysjob_t FakeSysJ;
  char *AttrList[MMAX_ATTR];

  MNLFromArray(MNode,nsNL,1);

  /* format attribute list for resource set selection */
  memset(AttrList,0x0,sizeof(char *) * MMAX_ATTR);

  AttrList[0] = nsAttr;

  memset(&FakeJ,0,sizeof(mjob_t));    
  memset(&FakeSysJ,0,sizeof(msysjob_t));

  strcpy(FakeJ.Name,"policy.premigrate.test");
  FakeJ.System = &FakeSysJ;
  FakeJ.Request.NC = 2;

  /* use nodeset routine to generate node list based on nsAttr */

  if (MJobSelectResourceSet(
       &FakeJ,
       (mreq_t *)NULL,
       MSched.Time,
       mrstFeature,
       mrssFirstOf,
       AttrList,
       nsNL,            /* O */
       NULL,            /* NodeMap */
       -1,
       NULL,
       (int *)NULL,
       NULL) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNLNodeSetByVLAN() */
/* END MNodeNL.c */
