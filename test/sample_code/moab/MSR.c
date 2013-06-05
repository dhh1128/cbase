/* HEADER */

/**
 * @file MSR.c
 *
 * Moab Standing Reservations
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  
 


/**
 * Load standing reservation config parameters.
 *
 * @see MSRProcessConfig() - child
 *
 * @param SRName (I)
 * @param PName (I) parameter name [optional]
 * @param ABuf (I) config buffer [optional]
 */

int MSRLoadConfig( 

  char       *SRName,
  const char *PName,
  char       *ABuf)
 
  {
  char    IndexName[MMAX_NAME];

  /* allow for large hostexpressions */
 
  char    Value[MMAX_BUFFER];

  char   *head;

  char   *ptr;
  char   *tptr;
 
  msrsv_t *SR;
 
  /* FORMAT:  <KEY>=<VAL>[<WS><KEY>=<VAL>]...         */
  /*          <VAL> -> <ATTR>=<VAL>[:<ATTR>=<VAL>]... */
 
  /* load all/specified SR config info */

  head = (ABuf != NULL) ? ABuf : MSched.ConfigBuffer;
 
  if (head == NULL)
    {
    return(FAILURE);
    }

  if ((SRName == NULL) || (SRName[0] == '\0'))
    {
    /* load ALL SR config info */
 
    ptr = head;
 
    IndexName[0] = '\0';
 
    while (MCfgGetSVal(
             head,
             &ptr,
             (PName != NULL) ? PName : MCredCfgParm[mxoSRsv],
             IndexName,
             NULL,
             Value,
             sizeof(Value), 
             0,
             NULL) != FAILURE)
      {
      if ((PName != NULL) && (!strcmp(PName,MPARM_RSVPROFILE)))
        {
        mrsv_t *R;

        if ((MRsvProfileFind(IndexName,&R) == SUCCESS) &&
            (R->MTime == MSched.Time))
          {
          /* profile already loaded */
 
          IndexName[0] = '\0';

          continue;
          }
        }
 
      if ((MSRFind(IndexName,&SR,NULL) == FAILURE) &&
          (MSRAdd(IndexName,&SR,NULL) == FAILURE))
        {
        /* unable to locate/create SR */
 
        IndexName[0] = '\0';
 
        continue;
        }

      if (((mulong)SR->MTime == MSched.Time) && (ABuf == NULL))
        {
        /* config line already loaded */

        IndexName[0] = '\0';

        continue;
        }
 
      /* load all config lines for specified SR */

      tptr = ptr;

      MSRProcessConfig(SR,Value);

      while (MCfgGetSVal(
               head,
               &tptr,
               (PName != NULL) ? PName : MCredCfgParm[mxoSRsv],
               IndexName,
               NULL,
               Value,
               sizeof(Value),
               0,
               NULL) != FAILURE)
        {
        MSRProcessConfig(SR,Value);
        }

      /* validate config */

      if (MSRCheckConfig(SR) == FAILURE)
        {
        /* standing reservation is invalid */

        MSRDestroy(&SR);
        }

      if ((PName != NULL) && (!strcmp(PName,MPARM_RSVPROFILE)))
        {
        mrsv_t *tmpR = (mrsv_t *)MUCalloc(1,sizeof(mrsv_t));

        /* process rsv profile */

        MSRToRsvProfile(SR,tmpR);

        MRsvProfileAdd(tmpR);

        MSRDestroy(&SR);
        }

      /* load next SR */

      IndexName[0] = '\0';
      }  /* END while (MCfgGetSVal() != FAILURE) */
    }    /* END if ((SRName == NULL) || (SRName[0] == '\0')) */
  else
    {
    /* load specified SR config info */

    ptr = head;         

    SR = NULL;

    while (MCfgGetSVal(
             head,
             &ptr, 
             (PName != NULL) ? PName : MCredCfgParm[mxoSRsv],
             SRName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) == SUCCESS)
      {
      if ((SR == NULL) &&
          (MSRFind(SRName,&SR,NULL) == FAILURE) &&
          (MSRAdd(SRName,&SR,NULL) == FAILURE))
        {
        /* unable to add standing reservation */
 
        return(FAILURE);
        }
 
      /* load SR attributes */

      MSRProcessConfig(SR,Value);
      }  /* END while (MCfgGetSVal() == SUCCESS) */

    if (SR == NULL)
      {
      return(FAILURE);
      }

    if ((PName != NULL) && (!strcmp(PName,MPARM_RSVPROFILE)))
      {
      mrsv_t *tmpR = (mrsv_t *)MUCalloc(1,sizeof(mrsv_t));

      /* process rsv profile */

      MSRToRsvProfile(SR,tmpR);

      MRsvProfileAdd(tmpR);

      MSRDestroy(&SR);
      }
    else
      {
      /* validate SR */

      if (MSRCheckConfig(SR) == FAILURE)
        {
        MSRDestroy(&SR);

        /* invalid standing reservation destroyed */

        return(FAILURE);
        }
      }
    }  /* END else ((SRName == NULL) || (SRName[0] == '\0')) */
 
  return(SUCCESS); 
  }  /* END MSRLoadConfig() */




/**
 * Validate standing reservation configuration.
 *
 * @param SR (I)
 */

int MSRCheckConfig(

  msrsv_t *SR)  /* I */

  {
  if (SR == NULL)
    {
    return(FAILURE);
    }

  if (SR->Period == mpWeek)
    {
    SR->WStartTime = SR->StartTime;
    SR->WEndTime   = SR->EndTime;
    }

  if (SR->StartTime > SR->EndTime)
    {
    MMBAdd(&SR->MB,"WARNING: invalid timeframe specified",MSCHED_SNAME,mmbtAnnotation,MMAX_TIME,0,NULL);
    }

  return(SUCCESS);
  }  /* END MSRCheckConfig() */





/**
 * Remove/free standing reservation.
 *
 * @param SRP (I) [modified]
 */

int MSRDestroy(

  msrsv_t **SRP)  /* I (modified) */

  {
  msrsv_t *SR;

  if (SRP == NULL)
    {
    return(SUCCESS);
    }

  if (*SRP == NULL)
    {
    return(SUCCESS);
    }

  SR = *SRP;

  if (bmisset(&SR->Flags,mrfIsVPC))
    {
    mpar_t *VPCP;

    MVPCFind(SR->Name,&VPCP,FALSE);

    MParDestroy(&VPCP);
    }

  if (SR->T != NULL)
    {
    MTListClearObject(SR->T,FALSE);

    MUArrayListFree(SR->T);

    MUFree((char **)&SR->T);
    }  /* END if (R->T != NULL) */

  if (SR->MB != NULL)
    MMBFree(&SR->MB,TRUE);

  MNLFree(&SR->HostList);

  if (SR->SpecRsvGroup != NULL)
    MUFree(&SR->SpecRsvGroup);

  if (SR->RsvAList != NULL)
    {
    int index;

    for (index = 0;index < MMAX_PJOB;index++)
      {
      if (SR->RsvAList[index] == NULL)
        break;

      MUFree(&SR->RsvAList[index]);
      }

    MUFree((char **)&SR->RsvAList);
    }

  bmclear(&SR->Flags);
  bmclear(&SR->Days);

  MCResFree(&SR->DRes);

  MACLFreeList(&SR->ACL);

  MULLFree(&SR->Variables,MUFREE);

  memset(SR,0,sizeof(msrsv_t));

  SR->Name[0] = '\1';

  return(SUCCESS);
  }  /* END MSRDestroy() */





/**
 * Find standing reservation with matching id.
 *
 * @param SRName (I)
 * @param SR     (O) [optional]
 * @param SRsv   (O) [optional]
 */

int MSRFind(
 
  char     *SRName,
  msrsv_t **SR,
  marray_t *SRsv)
 
  {
  /* if found, return success with SR pointing to srsv */
 
  int      srindex;
  int      cindex;

  char     tmpName[MMAX_NAME];

  marray_t *SRP;

  msrsv_t *tmpSR;
 
  if (SR != NULL)
    *SR = NULL;
 
  if ((SRName == NULL) ||
      (SRName[0] == '\0'))
    {
    return(FAILURE);
    }

  if (SRsv != NULL)
    {
    SRP = SRsv;
    }
  else
    {
    SRP = &MSRsv;
    }

  for (cindex = 0;SRName[cindex] != '\0';cindex++)
    {
    if (!isspace(SRName[cindex]) && (SRName[cindex] != '.'))
      tmpName[cindex] = SRName[cindex];
    else
      tmpName[cindex] = '_';
    }  /* END for (cindex) */

  tmpName[cindex] = '\0';
 
  for (srindex = 0;srindex < SRP->NumItems;srindex++)
    {
    tmpSR = (msrsv_t *)MUArrayListGet(SRP,srindex);
 
    if ((tmpSR == NULL) || (tmpSR->Name[0] == '\0'))
      break;

    if (tmpSR->Name[0] == '\1')
      continue;
 
    if (strcmp(tmpSR->Name,tmpName) != 0)
      continue;
 
    /* reservation found */

    if (SR != NULL) 
      *SR = tmpSR;
 
    return(SUCCESS);
    }  /* END for (srindex) */ 
 
  /* entire table searched */
 
  return(FAILURE);
  }  /* END MSRFind() */





/**
 *
 *
 * @param SRName (I)
 * @param SR     (O) [optional]
 * @param SRsv   (I) [optional]
 */

int MSRAdd(

  char     *SRName,
  msrsv_t **SR,
  marray_t *SRsv)

  {
  int     srindex;
  int     cindex;
 
  msrsv_t *tmpSR;

  marray_t *SRP;

  char tmpName[MMAX_NAME];

  const char *FName = "MSRAdd";
 
  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (SRName != NULL) ? SRName : "NULL",
    (SR != NULL) ? "SR" : "NULL");
 
  if ((SRName == NULL) || (SRName[0] == '\0'))
    {
    return(FAILURE);
    }
 
  if (SR != NULL)
    *SR = NULL;

  if (SRsv != NULL)
    {
    SRP = SRsv;
    }
  else 
    {
    SRP = &MSRsv;
    }

  for (cindex = 0;SRName[cindex] != '\0';cindex++)
    {
    if (!isspace(SRName[cindex]) && (SRName[cindex] != '.'))
      tmpName[cindex] = SRName[cindex];
    else
      tmpName[cindex] = '_';
    }  /* END for (cindex) */

  tmpName[cindex] = '\0';

  for (srindex = 0;srindex < SRP->NumItems;srindex++)
    {
    tmpSR = (msrsv_t *)MUArrayListGet(SRP,srindex);

    if ((tmpSR != NULL) && !strcmp(tmpSR->Name,tmpName))
      {
      /* SR already exists */

      if (SR != NULL)
        *SR = tmpSR;
 
      return(SUCCESS);
      }
 
    if ((tmpSR != NULL) &&
        ((tmpSR->Name[0] == '\0') ||
         (tmpSR->Name[0] == '\1')))
      {
      /* found cleared-out standing rsv slot */

      break;
      }
    }  /* END for(srindex) */

  if (srindex >= SRP->NumItems)
    {
    msrsv_t NewSR;

    /* append new standing reservation record */

    if (MSRCreate(tmpName,&NewSR) == FAILURE)
      {
      MDB(1,fALL) MLog("ERROR:    cannot clear memory for SR '%s'\n",
        tmpName);

      return(FAILURE);
      }

    MUArrayListAppend(SRP,&NewSR);

    /* get pointer to newly added srsv */

    srindex = SRP->NumItems-1;
    tmpSR = (msrsv_t *)MUArrayListGet(SRP,srindex);
    }

  /* initialize new standing rsv record */

  MSRInitialize(tmpSR,tmpName);

  if (SR != NULL)
    *SR = tmpSR;

  tmpSR->Index = srindex;

  /* update SR record */

  MCPRestore(mcpSRsv,tmpName,(void *)tmpSR,NULL);

  MDB(5,fSTRUCT) MLog("INFO:     SR %s added\n",
    tmpName);

  return(SUCCESS);
  }  /* END MSRAdd() */




/**
 *
 *
 * @param SRName (I)
 * @param SR (I) [modified]
 */

int MSRCreate(

  char    *SRName,  /* I */
  msrsv_t *SR)      /* I (modified) */

  {
  if (SR == NULL)
    {
    return(FAILURE);
    }

  /* use static memory for now */

  memset(SR,0,sizeof(msrsv_t));

  if ((SRName != NULL) && (SRName[0] != '\0'))
    {
    MUStrCpy(SR->Name,SRName,sizeof(SR->Name));
    }

  return(SUCCESS);
  }  /* END MSRCreate() */






/**
 * Locate resources to allocate to specified rsv w/resource requirements specified via pseudo-job J
 *
 * NOTE: there is a problem with standing reservations, if you request TASKCOUNT=1 and RESOURCES=PROCS:2
 *       and the node supports 2 tasks of size 2 then you will get 2 unless you specify FLAGS=REQFULL
 *       currently "besteffort" means do whatever you want, either more or less than the requested
 *       amount.
 *
 * @param J (I)
 * @param RName (I)
 * @param RPName (I)
 * @param RFlags (I) [BM of enum MRsvFlagsEnum]
 * @param RTC (I)
 * @param DstNL (O) selected nodes
 * @param NC (O) [optional]
 * @param StartTime (I)
 * @param ReqNL (I) required nodelist
 * @param RsvModeBM (I) [bitmap of enum MServiceType]
 * @param EMsg (O) [optional,minsize=MMAX_BUFFER]
 */

int MRsvSelectNL(
 
  mjob_t        *J,          /* I */
  char          *RName,      /* I */
  char          *RPName,     /* I */
  mbitmap_t     *RFlags,     /* I (BM of enum MRsvFlagsEnum) */
  int            RTC,        /* I */
  mnl_t         *DstNL,      /* O selected nodes */
  int           *NC,         /* O number of nodes found (optional) */
  long           StartTime,  /* I */
  mnl_t         *ReqNL,      /* I required nodelist */
  mbitmap_t     *RsvModeBM,  /* I (bitmap of enum MServiceType) */
  char          *EMsg)       /* O (optional,minsize=MMAX_BUFFER) */
 
  {
  int         TC;
  
  int         BestTC;
  mpar_t     *BestP;

  int         MaxTasks;
 
  int         rqindex;
  int         nindex;
  int         nindex2;
 
  int         pindex;
  enum MNodeAllocPolicyEnum NAPolicy;
 
  mnl_t  *FNL; 
 
  mnl_t  *AvailMNL[MMAX_REQ_PER_JOB];
  mnl_t  *tmpMNL[MMAX_REQ_PER_JOB];
  mnl_t  *BestFMNL[MMAX_REQ_PER_JOB];
  mnl_t   tmpNL;
 
  char       *NodeMap = NULL;

  mpar_t     *P;
 
  mreq_t     *RQ;

  mbool_t     IsSizeFlex; 
  mbool_t     IsForced;    /* force rsv onto requested node regardless of state or other constraints */

  char       *BPtr;
  int         BSpace;

  char        LocalEMsg[MMAX_LINE];

  char TString[MMAX_LINE];
  const char *FName = "MRsvSelectNL";

  MULToTString(StartTime - MSched.Time,TString);

  MDB(3,fSCHED) MLog("%s(%s,%s,Flags,DstNL,NC,%s,ReqNL,%ld,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    RName,
    TString,
    RsvModeBM);

  if (EMsg != NULL)
    MUSNInit(&BPtr,&BSpace,EMsg,MMAX_BUFFER);

  if (J == NULL)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"bad parameters");

    return(FAILURE);
    }

  /* NOTE:  only support single req rsv's */
 
  TC     = 0;

  BestTC = 0;
  BestP  = NULL;

  MaxTasks   = J->Request.TC;

  IsSizeFlex = bmisset(RsvModeBM,mstBestEffort);

  if (IsSizeFlex == TRUE)
    {
    J->Request.TC        = 1;
    J->Req[0]->TaskCount = 1;
    }
 
  if (MNodeMapInit(&NodeMap) == FAILURE)
    {
    return(FAILURE);
    }

  MNLInit(&tmpNL);
  MNLMultiInit(BestFMNL);

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];
 
    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\1')
        continue;

      if (P->Name[0] == '\0')
        break;

      if ((P->CRes.Procs <= 0) &&
         ((RQ->DRes.Procs == -1) || (RQ->DRes.Procs > 0)))
        continue;
 
      if (RPName[0] != '\0')
        {
        if (!strcmp(RPName,"ALL"))
          {
          /* reservation may span */

          if (pindex != 0)
            continue;
          }
        else
          {
          if (strcmp(P->Name,RPName))
            continue;
          }
        }
      else
        {
        /* reservation may not span */
  
        if (pindex == 0) 
          continue;
        }  /* END else (RPName[0] != '\0') */
 
      if ((strcmp(RPName,"ALL") != 0) &&
          (P->Index == 0))
        {
        continue;
        }
 
      MDB(6,fSCHED) MLog("INFO:     checking partition %s for resources for reservation %s\n",
        P->Name,
        RName);
 
      MNLClear(&tmpNL);
 
      FNL = &tmpNL;

      /* NOTE: previously ignored node state unless job required advance reservation */
 
      if (MReqGetFNL(
            J,
            RQ,
            P,
            NULL,
            FNL,
            NC,
            &TC,
            bmisset(&J->Flags,mjfIgnState) ? MMAX_TIME : StartTime, 
            RsvModeBM,
            NULL) == FAILURE)
        {
        MDB(6,fSCHED) MLog("INFO:     cannot get feasible tasks for job %s:%d in partition %s\n",
          J->Name,
          rqindex,
          P->Name);
 
        TC = 0;
 
        continue;
        }

      if (TC > BestTC)
        {
        BestTC = TC;
        BestP  = P; 

        /* save best feasible list */

        MNLCopy(BestFMNL[rqindex],&tmpNL);
        }

      /* NOTE:  should be modified to select partition with most tasks */
      }     /* END for (pindex) */
 
    if (BestP == NULL) 
      {
      /* no feasible nodes located */
 
      MDB(6,fSCHED) MLog("INFO:     no feasible tasks found for job %s:%d in partition %s\n",
        J->Name,
        rqindex,
        RPName);
 
      MUFree(&NodeMap);
      MNLMultiFree(BestFMNL);
      MNLClear(DstNL);
      MNLFree(&tmpNL);

      if (EMsg != NULL)
        strcpy(EMsg,"no feasible tasks found");
 
      return(FAILURE);
      }
    }     /* END for (rqindex) */

  MNLFree(&tmpNL);

  /* NOTE:  RTC only 'cap' rsv */

  if (!bmisset(RFlags,mrfSpaceFlex) &&
      (bmisset(RsvModeBM,mstForce) ||
      (RTC == 0) ||
       bmisset(RFlags,mrfIgnRsv)))
    {
    /* NOTE: HostExp specified or TC not specified, */
    /* select from all feasible nodes */

    IsForced = TRUE;
    }
  else
    {
    IsForced = FALSE;
    }

  RQ = J->Req[0];  /* FIXME */

  /* extract nodes which do not satify rsv allocation constraints */

  if (!bmisset(RFlags,mrfExcludeAll))
    {
    if ((RTC != 0) || bmisset(RFlags,mrfExcludeJobs))
      {
      bmset(&J->RsvAccessBM,mjapAllowAllNonJob);
  
      IsForced = FALSE;
      }

    if (bmisset(RFlags,mrfExcludeAllButSB))
      {
      bmset(&J->RsvAccessBM,mjapAllowSandboxOnly);
   
      IsForced = FALSE;
      }
    }    /* END if (!bmisset(RFlags,mrfExcludeAll)) */

  MNLMultiInit(AvailMNL);

  if (IsForced == TRUE)
    {
    /* NOTE:  make all feasible nodes available for allocation selection *
     * regardless of current state */

    /* if HostExp specified or if TC not specified, */
    /* select from all feasible nodes */

    if (!bmisset(RFlags,mrfIgnRsv))
      {
      int nindex;
      int rqindex;

      int ncounter;

      mnode_t *N;
      mrsv_t  *tR = NULL;
      mre_t   *RE;

      long     Overlap;

      TC = 0;

      /* must extract exclusive user rsv nodes */

      /* walk all nodes, remove those with exclusive flag set and time overlap */

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        ncounter = 0;

        for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
          {
          int tmpTC;

          if (MNLGetNodeAtIndex(BestFMNL[rqindex],nindex,NULL) == FAILURE)
            {
            MNLTerminateAtIndex(AvailMNL[rqindex],ncounter);
            MNLSetTCAtIndex(AvailMNL[rqindex],ncounter,0);

            break;
            }          

          MNLGetNodeAtIndex(BestFMNL[rqindex],nindex,&N);
          tmpTC = MNLGetTCAtIndex(BestFMNL[rqindex],nindex);

          if (N->RE == NULL)
            {
            /* node has no reservations - add to avail list */

            MNLSetNodeAtIndex(AvailMNL[rqindex],ncounter,N);
            MNLSetTCAtIndex(AvailMNL[rqindex],ncounter,tmpTC);

            ncounter++;

            TC += tmpTC;

            continue;
            }

          for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
            {
            tR = MREGetRsv(RE);

            if (!bmisset(&tR->Flags,mrfExcludeAll))
              {
              /* reservation is not explicitly exclusive - ignore */

              continue;
              }
 
            if ((Overlap = MUGetOverlap(
                  tR->StartTime,
                  tR->EndTime,
                  StartTime,
                  StartTime + J->WCLimit)) <= 0)
              {
              /* reservations do not overlap - ignore */

              continue;
              }
    
            if (MRsvCheckJAccess(
                  tR,
                  J,
                  Overlap,
                  NULL,
                  FALSE,
                  NULL,
                  NULL,
                  NULL,
                  NULL) == TRUE)
              {
              /* reservation is inclusive - ignore */

              continue;
              }

            /* exclusive rsv located */

            break;
            }    /* END for (rindex) */

          if (RE != NULL)
            {
            /* exclusive overlapping rsv located, do not add node to avail list */

            continue;
            }
  
          /* add node to list */

          MNLSetNodeAtIndex(AvailMNL[rqindex],ncounter,N);
          MNLSetTCAtIndex(AvailMNL[rqindex],ncounter,tmpTC);

          ncounter++;

          TC += tmpTC;
          }  /* END for (nindex) */
        }    /* END for (rqindex) */ 

      memset(NodeMap,mnmNONE,sizeof(NodeMap));
      }      /* END if (!bmisset(RFlags,mrfIgnRsv)) */
    else
      {
      memset(NodeMap,mnmNONE,sizeof(NodeMap));
 
      MNLMultiCopy(AvailMNL,BestFMNL);

      TC = BestTC;
      }
    }    /* END if (IsForced == TRUE) */
  else
    {
    /* determine available nodes from feasible nodes */

    if (IsSizeFlex == TRUE)
      {
      /* MJobGetAMNodeList() takes a "best effort" approach when TC == -1 */

      TC = -1;
      }

    if (MJobGetAMNodeList(
          J,                     /* I */
          BestFMNL,              /* I */
          AvailMNL,              /* O */
          NodeMap,               /* O */
          NC,                    /* O */
          &TC,                   /* O */
          StartTime) == FAILURE) /* I */
      {
      MDB(4,fSCHED) MLog("INFO:     cannot locate available tasks in partition %s for job %s at time %ld\n",
        RPName,
        J->Name,
        StartTime);
 
      MNLClear(DstNL);

      if (EMsg != NULL)
        strcpy(EMsg,"cannot locate available tasks");
 
      MNLMultiFree(AvailMNL);
      MUFree(&NodeMap);

      return(FAILURE);
      }

    TC            = MIN(MaxTasks,TC);
 
    J->Alloc.TC   = TC;
    RQ->TaskCount = TC;
    }    /* END else (IsForced == TRUE) */
 
  for (nindex = 0;MNLGetNodeAtIndex(ReqNL,nindex,NULL) == SUCCESS;nindex++)
    {
    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      for (nindex2 = 0;MNLGetNodeAtIndex(AvailMNL[rqindex],nindex2,NULL) == SUCCESS;nindex2++)
        {
        mnode_t *N1;
        mnode_t *N2;

        MNLGetNodeAtIndex(AvailMNL[rqindex],nindex2,&N1);
        MNLGetNodeAtIndex(ReqNL,nindex,&N2);

        if (N1 == N2)
          {
          NodeMap[N2->Index] = mnmRequired;
 
          break;
          }
        }    /* END for (nindex2) */
      }      /* END for (rqindex) */
    }        /* END for (nindex)  */

  if (IsSizeFlex == TRUE)
    { 
    /* adjust required task count */
 
    if (RTC > 0)
      J->Request.TC = MIN(TC,RTC);
    else
      J->Request.TC = TC;
 
    RQ->TaskCount = J->Request.TC;
    }  /* END if (SizeFlex == TRUE) */

  MNLMultiInit(tmpMNL);

  NAPolicy = (J->Req[0]->NAllocPolicy != mnalNONE) ?
    J->Req[0]->NAllocPolicy :
    MPar[0].NAllocPolicy;

  if (MJobAllocMNL(
        J,
        AvailMNL,
        NodeMap,
        tmpMNL,
        NAPolicy,
        StartTime,
        NULL,
        LocalEMsg) == FAILURE)
    {
    MDB(0,fSCHED) MLog("ERROR:    cannot allocate best nodes for job '%s'\n",
      J->Name);

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_BUFFER,"cannot allocate nodes - %s",
        (LocalEMsg[0] == '\0') ? '\0' : LocalEMsg);
      }
 
    MNLMultiFree(BestFMNL);
    MNLMultiFree(tmpMNL);
    MNLMultiFree(AvailMNL);
    MUFree(&NodeMap);

    return(FAILURE);
    }

  /* FIXME:  call MJobDistributeTasks() */

  if ((J->Request.TC > 0) && (J->Request.NC == 0))
    {
    mnode_t *tmpN;
    int      tmpTC;

    int TC = J->Request.TC;

    for (nindex = 0;MNLGetNodeAtIndex(tmpMNL[0],nindex,&tmpN) == SUCCESS;nindex++)
      {
      tmpTC = MNLGetTCAtIndex(tmpMNL[0],nindex);

      if (tmpTC <= 0)
        break;

      MNLSetNodeAtIndex(DstNL,nindex,tmpN);

      if (tmpTC > TC)
        MNLSetTCAtIndex(DstNL,nindex,TC);
      else
        MNLSetTCAtIndex(DstNL,nindex,tmpTC);

      TC -= MNLGetTCAtIndex(DstNL,nindex);
      }

    MNLTerminateAtIndex(DstNL,nindex);
    }  /* END if ((J->Request.TC > 0) && (J->Request.NC == 0)) */
  else
    {
    mnode_t *tmpN;
    int      tmpTC;

    RQ->TaskCount = 0;  /* FIXME??? */
 
    for (nindex = 0;MNLGetNodeAtIndex(tmpMNL[0],nindex,&tmpN) == SUCCESS;nindex++)
      {
      tmpTC = MNLGetTCAtIndex(tmpMNL[0],nindex);

      if (tmpTC == 0)
        break;
   
      RQ->TaskCount += tmpTC;
   
      MNLSetNodeAtIndex(DstNL,nindex,tmpN);
      MNLSetTCAtIndex(DstNL,nindex,tmpTC);
      }  /* END for (nindex) */

    MNLTerminateAtIndex(DstNL,nindex);
    }    /* END else */

  if (NC != NULL)
    *NC = nindex; 
 
  MUFree(&NodeMap);
  MNLMultiFree(BestFMNL);
  MNLMultiFree(tmpMNL);
  MNLMultiFree(AvailMNL);

  if (nindex == 0)
    {
    MDB(2,fSCHED) MLog("INFO:     no tasks found for job %s\n",
      J->Name);

    if (EMsg != NULL)
      strcpy(EMsg,"no tasks found");
 
    return(FAILURE);
    }
 
  return(SUCCESS);
  }  /* END MRsvSelectNL() */




/**
 *
 *
 * @param SR (I)
 * @param R (I)
 */

int MSRCheckReservation(
 
  msrsv_t *SR,  /* I */
  mrsv_t  *R)   /* I */
 
  {
  mre_t *RE;

  int nindex;
  int TC;
 
  mnode_t *N;

  char tmpLine[MMAX_LINE];
 
  /* return SUCCESS if complete reservation located    */
  /* return if reservation lacks adequate resources or */
  /* has invalid resources allocated                   */
 
  TC = 0;
 
  if ((SR == NULL) || (R == NULL))
    {
    return(FAILURE);
    }
 
  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;
 
    for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
      {
      if (MREGetRsv(RE) != R)
        continue;
 
      /* reservation located on node */
 
      /* Check if the reservation has started or will have started within the time
         that the node is assumed to be down/drained */

      if ((SR->ReqTC > 0) &&
          (((mulong)R->StartTime <= MSched.Time) ||
           ((N->State == mnsDown) && 
            ((MPar[0].NodeDownStateDelayTime < 0) || /* assume node is down forever */
             ((MPar[0].NodeDownStateDelayTime > 0) && /* rsv starttime is within time node is assumed down */
              ((mulong)R->StartTime <= MSched.Time + MPar[0].NodeDownStateDelayTime)))) ||
           (((N->State == mnsDrained) || (N->State == mnsDraining)) &&
            ((MPar[0].NodeDrainStateDelayTime < 0) || /* assume node is drained forever */
             ((MPar[0].NodeDrainStateDelayTime> 0) && /* rsv starttime is within time node is assumed drained */
              ((mulong)R->StartTime <= MSched.Time + MPar[0].NodeDrainStateDelayTime))))) &&
           (N->State != mnsIdle) &&
           (N->State != mnsActive) &&
           (N->State != mnsBusy))
        {
        /* floating reservation contains node in invalid state */

        R->AllocTC -= RE->TC;
 
        return(FAILURE);
        }
 
      TC += RE->TC;
 
      break;
      }  /* END for (nrindex) */
    }    /* END for (nindex)  */

  if (TC < R->ReqTC)
    {
    sprintf(tmpLine,"RESERVATIONCORRUPTION:  reservation corruption detected in reservation '%s'  Req/Detected TC %d/%d\n",
      R->Name,
      R->ReqTC,
      TC);

    MSysRegEvent(tmpLine,mactNONE,1);

    R->AllocTC = TC;

    return(FAILURE);
    }
 
  return(SUCCESS);
  }  /* END MSRCheckReservation() */





/**
 * Called once per iteration to optimize/correct standing rsvs 
 *
 * @see MSchedProcessJobs() - parent
 * @see MSysJobUpdatePeriodic() - parent
 *
 * @param SpecSR (I) [optional] - specified standing reservation
 * @param SRsv (I) [optional] - specified srsv table
 * @param SEMsg (O) [optional,minsize=MMAX_LINE]
 */

int MSRIterationRefresh(
  
  msrsv_t  *SpecSR,  /* I (optional) */
  marray_t *SRsv,    /* I (optional) */
  char     *SEMsg)   /* O (optional,minsize=MMAX_LINE) */
 
  {
  msrsv_t *SR;
 
  marray_t *SRP;

  int     SRIndex;
  int     DIndex;
 
  mbool_t Incomplete[MMAX_SRSV_DEPTH];
  mbool_t SRRefreshRequired;
  mbool_t FailureDetected = FALSE;

  char    tmpEMsg[MMAX_LINE];
  char    tmpLine[MMAX_LINE];

  char   *EMsg;

  const char *FName = "MSRIterationRefresh";

  MDB(7,fCORE) MLog("%s(%s,%s,EMsg)\n",
    FName,
    (SpecSR != NULL) ? SpecSR->Name : "NULL",
    (SRsv != NULL) ? "SRsv" : "NULL");

  if (SEMsg != NULL)
    EMsg = SEMsg;
  else
    EMsg = tmpEMsg;

  EMsg[0] = '\0';

  if (SRsv != NULL)
    {
    SRP = SRsv;
    }
  else
    {
    SRP = &MSRsv;
    }

  for (SRIndex = 0;SRIndex < SRP->NumItems;SRIndex++)
    {
    SR = (msrsv_t *)MUArrayListGet(SRP,SRIndex);

    if (SR == NULL)
      break;

    if (SR->Name[0] == '\0')
      break;

    if (SR->Name[0] == '\1')
      continue;

    if ((SpecSR != NULL) && (SR != SpecSR))
      continue;

    if (SR->Disable == TRUE)
      {
      MDB(4,fCORE) MLog("ALERT:    SR %s is disabled\n",
        SR->Name);

      continue;
      }

    /* merge temporary values into temporary SR */

    if (SR->RollbackOffset > 0)
      {
      /* evaluate potential start time adjustment */
      }
    else if (SR->ReqTC == 0)
      {
      if (SR->HostExp[0] == '\0')
        {
        /* continue if SR not specified */
 
        MDB(7,fCORE) MLog("INFO:     SR[%d] is empty (TC: %d  HL: '%s')\n",
          SRIndex,
          SR->ReqTC,
          SR->HostExp);

        continue;
        }
      else
        {
        /* continue if exact hosts specified */
 
        /* all hosts located and incorporated in reservation at node load time */
 
        MDB(5,fCORE) MLog("INFO:     SR[%d] exactly specifies nodes (TC: %d  HL: '%s')\n",
          SRIndex,
          SR->ReqTC,
          SR->HostExp);
        }

      if (SR->R[0] != NULL) 
        {
        /* continue if reservations already populated */

        continue;
        }
      }    /* END if (SR->ReqTC == 0) */
 
    memset(Incomplete,FALSE,sizeof(Incomplete));

    SRRefreshRequired = FALSE;

    for (DIndex = 0;DIndex < MMAX_SRSV_DEPTH;DIndex++)
      {
      if ((DIndex >= SR->Depth) ||
         ((DIndex > 0) && (SR->Period == mpInfinity)))
        {
        break;
        }

      /* don't update reservation if it has its required taskcount and
       * has the ingstate reservation since it got what it was asking for. */

      if ((SR->R[DIndex] == NULL) ||
          (SR->R[DIndex]->AllocTC < SR->ReqTC) ||
          (!bmisset(&SR->R[DIndex]->Flags,mrfIgnState) &&
           MSRCheckReservation(SR,SR->R[DIndex]) == FAILURE))
        {
        Incomplete[DIndex] = TRUE;
        SRRefreshRequired  = TRUE;
        }
      }  /* END for (DIndex) */

    if ((SRRefreshRequired == FALSE) && 
        (SR->R[0] != NULL) && 
        (SR->RollbackOffset > 0) && 
       ((SR->RollbackIsConditional == FALSE) || 
        (MRsvHasOverlap(
           SR->R[0],
           (mrsv_t *)1,  /* look for job overlap */
           TRUE,
           &SR->R[0]->NL,
           NULL,
           NULL) == FALSE)) &&
        (MSched.Time + SR->RollbackOffset > (mulong)SR->R[0]->StartTime))
      {
      /* only index '0' rsv should be impacted */

      Incomplete[0]     = TRUE;
      SRRefreshRequired = TRUE;
      }

    if ((SRRefreshRequired == TRUE) ||
        (MACLGet(SR->ACL,maDuration,NULL) == FAILURE))
      {
      MSim.QueueChanged = TRUE;
      MSched.EnvChanged = TRUE;
      }
    else
      {
      /* srsv does not need to be refreshed */
 
      continue;
      }
 
    for (DIndex = 0;DIndex < MMAX_SRSV_DEPTH;DIndex++)
      {
      if ((DIndex >= SR->Depth) ||
         ((DIndex > 0) && (SR->Period == mpInfinity)))
        {
        break;
        }
 
      if (Incomplete[DIndex] == FALSE)
        {
        continue;
        }

      /* When using triggers that modify the acl of a standing reservation, it's
       * possible that the acls that were removed will be brought back with SR's
       * acls. So use the current reservation's acl instead. */

      if (SR->R[DIndex] != NULL)
        {
        MACLCopy(&SR->ACL,SR->R[DIndex]->ACL);
        }

      /* NOTE:  if only time adjustment required, can reduce load with rsv modify 
       *        rather than configure (NYI) */ 

      if (MRsvConfigure(SR,NULL,SRIndex,DIndex,tmpLine,NULL,SRP,FALSE) == SUCCESS)
        {
        if (SR->R[DIndex] != NULL)
          {
          while (SR->R[DIndex]->ReqTC < SR->ReqTC)
            {
            if (MRsvPreempt(SR->R[DIndex]) == FAILURE)
              break;

            MRsvConfigure(SR,NULL,SRIndex,DIndex,NULL,NULL,SRP,FALSE);

            /* NOTE:  allocated SR values must be assigned to parent MSRsv[SRIndex] structure */

            /* SR->HostList handled inside MRsvConfigure */
            }  /* END while (SR->R[DIndex]->ReqTC) */
          }
        }    /* END if (MRsvConfigure(SR,NULL,SRIndex,DIndex,...) == SUCCESS) */
      else
        {
        MDB(2,fSCHED) MLog("ALERT:    cannot create standing reservation '%s' - %s\n",
          SR->Name,
          tmpLine);

        if (SEMsg != NULL)
          MUStrCpy(SEMsg,tmpLine,MMAX_LINE);

        FailureDetected = TRUE;
        }
      }      /* END for (DIndex)  */
    }        /* END for (SRIndex) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSRIterationRefresh() */





/**
 * Process standing reservation configuration.
 *
 * For credential ACL specify "mAdd" to MACLFromString() so that
 * additional lines from moab.cfg do not overwrite previous lines
 *
 * @see MSRLoadConfig() - parent
 *
 * @param SR (I) [modified]
 * @param Value (I)
 */

int MSRProcessConfig(

  msrsv_t *SR,     /* I (modified) */
  char    *Value)  /* I */

  {
  int   aindex;
  enum MSRAttrEnum AIndex;

  enum MCompEnum ValCmp;

  char *ptr;
  char *TokPtr;

  char *ptr2;
  char *TokPtr2;

  char  ValBuf[MMAX_BUFFER];

  char  tmpBuf[MMAX_BUFFER];

  int   tmpI;

  int   rc;

  const char *FName = "MSRProcessConfig";

  MDB(2,fSCHED) MLog("%s(SR,%.32s)\n",
    FName,
    (Value != NULL) ? Value : "NULL");

  if ((SR == NULL) || 
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  /* process value line */
 
  ptr = MUStrTokE(Value," \t\n",&TokPtr);
 
  while (ptr != NULL)
    {
    /* parse name-value pairs */
 
    /* FORMAT:  <ATTR><CMP><VALUE>[,<VALUE>] */
 
    if (MUGetPair(
          ptr,      /* I */
          (const char **)MSRsvAttr,
          &aindex,  /* O (attribute) */
          NULL,
          FALSE,
          &tmpI,    /* O (relative comparison) */
          ValBuf,   /* O (value) */
          sizeof(ValBuf)) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      MDB(2,fSCHED) MLog("ALERT:    cannot parse SRCFG[%s] config attribute '%s'\n",
        SR->Name,
        ptr);

      snprintf(tmpLine,sizeof(tmpLine),"ALERT: cannot parse attribute '%s'",
        ptr);

      MMBAdd(&SR->MB,tmpLine,MSCHED_SNAME,mmbtAnnotation,MMAX_TIME,0,NULL);

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);
 
      continue;
      }

    ValCmp = (enum MCompEnum)tmpI;

    snprintf(tmpBuf,sizeof(tmpBuf),"%s%s",
      MComp[ValCmp],
      ValBuf);

    SR->MTime = MSched.Time;

    AIndex = (enum MSRAttrEnum)aindex;

    rc = SUCCESS;

    switch (AIndex)
      {
      case msraAccess:

        if (!strcmp(ValBuf,"SHARED"))
          {
          bmunset(&SR->Flags,mrfExcludeAll);
          }
        else if (!strcmp(ValBuf,"DEDICATED"))
          {
          bmset(&SR->Flags,mrfExcludeAll);
          }

        break;

      case msraAccountList:

        MACLFromString(&SR->ACL,tmpBuf,1,maAcct,mAdd,NULL,FALSE);

        break;

      case msraChargeAccount:

        MAcctAdd(ValBuf,&SR->A);

        break;

      case msraChargeUser:

        MUserAdd(ValBuf,&SR->U);

        break;

      case msraClassList:

        MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mAdd);

        break;

      case msraClusterList:

        MACLFromString(&SR->ACL,tmpBuf,1,maCluster,mAdd,NULL,FALSE);

        break;

      case msraComment:
      case msraDescription:

        MMBAdd(&SR->MB,(char *)ValBuf,MSCHED_SNAME,mmbtAnnotation,MMAX_TIME,0,NULL);

        break;

      case msraDays:

        /* FORMAT:  ALL|MON|TUE|... */

        bmclear(&SR->Days);

        /* NOTE:  Allow case insensitive and substring day specification */

        if (strstr(ValBuf,"ALL") != NULL)
          {
          bmset(&SR->Days,0);
          bmset(&SR->Days,1);
          bmset(&SR->Days,2);
          bmset(&SR->Days,3);
          bmset(&SR->Days,4);
          bmset(&SR->Days,5);
          bmset(&SR->Days,6);
          bmset(&SR->Days,MCONST_ALLDAYS);
          }
        else
          { 
          int dindex;

          for (dindex = 0;MWeekDay[dindex] != NULL;dindex++)
            {
            if (MUStrStr(ValBuf,(char *)MWeekDay[dindex],0,TRUE,FALSE) != NULL)
              {
              bmset(&SR->Days,dindex);
              }
            }   /* END for (dindex) */
          }     /* END else (strstr(ValBuf,"ALL") != NULL) */   
 
        break;

      case msraDepth:

        SR->Depth = (int)strtol(ValBuf,NULL,0);

        break;

      case msraDisable:

        MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mSet);

        if (SR->Disable == TRUE)
          {
          int DIndex;

          for (DIndex = 0;DIndex < MMAX_SRSV_DEPTH;DIndex++)
            {
            if ((DIndex >= SR->Depth) ||
                (SR->R[DIndex] == NULL))
              break;

            MRsvDestroy(&SR->R[DIndex],TRUE,TRUE);
            }
          }

        break;

      case msraDisableTime:
      case msraDuration:
      case msraEnableTime:

        MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case msraEndTime:

        /* FORMAT:  <RELATIVE_TIME>|<ABS_TIME> */

        SR->EndTime = MUTimeFromString(ValBuf);     

        break;

      case msraFlags:

        bmfromstring(ValBuf,MRsvFlags,&SR->Flags);

        if (bmisset(&SR->Flags,mrfAllowPRsv) && (MSched.PRsvSandboxRID == NULL))
          MUStrDup(&MSched.PRsvSandboxRID,SR->Name);

        if (bmisset(&SR->Flags,mrfAllowGrid) && (MSched.GridSandboxRID == NULL))
          MUStrDup(&MSched.GridSandboxRID,SR->Name);

        break;

      case msraGroupList:

        MACLFromString(&SR->ACL,tmpBuf,1,maGroup,mAdd,NULL,FALSE);

        break;

      case msraHostList:

        /* FORMAT:  <HOST>[ :\t,+<HOST>]... || CLASS:<CLASSID> */

        MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case msraIdleTime:

        SR->MaxIdleTime = MUTimeFromString(ValBuf);   

        break;

      case msraJobAttrList:

        MACLFromString(&SR->ACL,tmpBuf,1,maJAttr,mSet,NULL,FALSE);

        break;
  
      case msraJPriority:

        MACLFromString(&SR->ACL,tmpBuf,1,maJPriority,mSet,NULL,FALSE);

        break;
 
      case msraJTemplateList:

        MACLFromString(&SR->ACL,tmpBuf,1,maJTemplate,mSet,NULL,FALSE);

        break;

      case msraLogLevel:

        SR->LogLevel = (int)strtol(ValBuf,NULL,10);

        break;

      case msraMaxJob:

        SR->MaxJob = (int)strtol(ValBuf,NULL,10);

        break;

      case msraTimeLimit:
      case msraMaxTime:

        MACLFromString(&SR->ACL,ValBuf,1,maDuration,mSet,NULL,FALSE);
 
        break;

      case msraMinMem:

        {
        char *ptr;

        macl_t tACL;

        MACLInit(&tACL);

        if (strchr(ValBuf,'*') != NULL)
          {
          bmset(&tACL.Flags,maclRequired);

          ptr = MUStrTok(ValBuf,"*",&ptr);
          }
        else
          {
          ptr = ValBuf;
          }

        tACL.LVal     = strtol(ValBuf,NULL,10);
        tACL.Type     = maMemory;
        tACL.Cmp      = mcmpGE;
        tACL.Affinity = mnmPositiveAffinity;

        MACLSetACL(&SR->ACL,&tACL,FALSE);
        }  /* END BLOCK */

      case msraNodeFeatures:

        if (!strcmp(ValBuf,NONE))
          break;
 
        if (strchr((char *)ValBuf,'|'))
          {
          SR->FeatureMode = mclOR;
          }
        else if (strchr((char *)ValBuf,'&')) 
          {
          SR->FeatureMode = mclOR;   
          }

        ptr2 = MUStrTok(ValBuf,"|&:[] ,\t",&TokPtr2);
 
        while (ptr2 != NULL)
          {
          MUGetNodeFeature(ptr2,mAdd,&SR->FBM);
 
          ptr2 = MUStrTok(NULL,"|&:[] ,\t",&TokPtr2);
          }

        break;

      case msraOS:
      case msraOwner:

        rc = MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case msraPartition:

        MUStrCpy(SR->PName,ValBuf,sizeof(SR->PName));       

        break;

      case msraPeriod:

       rc = MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case msraPriority:

        SR->Priority = strtol(ValBuf,NULL,0);

        break;

      case msraProcLimit:

        {
        char tmpLine[MMAX_LINE];

        /* prepend comparision which was stripped off earlier */

        snprintf(tmpLine,sizeof(tmpLine),"%s%s",
          MComp[ValCmp],
          ValBuf);

        MACLFromString(&SR->ACL,tmpLine,1,maProc,mSet,NULL,FALSE);
        }  /* END BLOCK case msraProcLimit */

        break;

      case msraPSLimit:

        {
        char tmpLine[MMAX_LINE];

        /* prepend comparision which was stripped off earlier */

        snprintf(tmpLine,sizeof(tmpLine),"%s%s",
          MComp[ValCmp],
          ValBuf);

        MACLFromString(&SR->ACL,tmpLine,1,maPS,mSet,NULL,FALSE);
        }  /* END BLOCK case msraPSLimit */

        break;

      case msraQOSList:

        MACLFromString(&SR->ACL,tmpBuf,1,maQOS,mAdd,NULL,FALSE);  

        break;

      case msraResources:

        /* FORMAT: <X>=<Y>[+<X>=<Y>]... */

        MCResClear(&SR->DRes);

        MCResFromString(&SR->DRes,ValBuf); 

        break;

      case msraRollbackOffset:

        {
        char *ptr;

        /* FORMAT:  <TIME>[*] */

        if ((ptr = strchr(ValBuf,'*')) != NULL)
          {
          *ptr = '\0';

          SR->RollbackIsConditional = TRUE;
          }

        SR->RollbackOffset = MUTimeFromString(ValBuf);
        }  /* END BLOCK (case msraRollbackOffset) */

        break;

      case msraRsvGroup:

        MUStrDup(&SR->SpecRsvGroup,ValBuf);

        break;

      case msraRsvAccessList:

        MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case msraStartTime:

        SR->StartTime = MUTimeFromString(ValBuf);     

        break;

      case msraTaskCount:

        SR->ReqTC = strtol(ValBuf,NULL,0);      

        break;

      case msraSubType:

        if (isdigit(((char *)ValBuf)[0]))
          {
          SR->SubType = (enum MRsvSubTypeEnum)strtol(ValBuf,NULL,0);
          }
        else
          {
          SR->SubType = (enum MRsvSubTypeEnum)MUGetIndex(ValBuf,MRsvSubType,FALSE,mrsvstNONE);
  
          if (SR->SubType == mrsvstNONE)
            SR->SubType = (enum MRsvSubTypeEnum)MUGetIndex(ValBuf,MRsvSSubType,FALSE,mrsvstNONE);
          }

        break;

      case msraSysJobTemplate:

        MUStrDup(&SR->SysJobTemplate,ValBuf);

        break;

      case msraTaskLimit:

        MACLFromString(&SR->ACL,ValBuf,1,maTask,mAdd,NULL,FALSE);
 
        break;

      case msraTPN:

        SR->TPN = (int)strtol(ValBuf,NULL,0);          

        break;

      case msraTrigger:

        /* triggers are initialized on the instanteations in MRsvConfigure() */

        MTrigLoadString(&SR->T,ValBuf,TRUE,FALSE,mxoSRsv,SR->Name,NULL,NULL);

        break;

      case msraUserList:

        MACLFromString(&SR->ACL,tmpBuf,1,maUser,mAdd,NULL,FALSE);

        break;

      case msraVariables:

        MSRSetAttr(SR,AIndex,(void **)ValBuf,mdfString,mAdd);

        break;

      case msraWEndTime:

        SR->WEndTime = MUTimeFromString(ValBuf);    

        break;

      case msraWStartTime:

        SR->WStartTime = MAX(0,MUTimeFromString(ValBuf));

        break;
 
      default:
 
        MDB(4,fFS) MLog("WARNING:  SR attribute '%s' not handled\n",
          MSRsvAttr[AIndex]);

        rc = FAILURE;
 
        break;
      }  /* END switch (AIndex) */

    if (rc == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      /* cannot set value for attribute */

      MDB(2,fSCHED) MLog("ALERT:    cannot set SRCFG[%s] attribute '%s' to value: '%s'\n",
        SR->Name,
        MSRsvAttr[AIndex],
        ValBuf);

      snprintf(tmpLine,sizeof(tmpLine),"cannot set SRCFG[%s] attribute '%s' to value: '%s'\n",
        SR->Name,
        MSRsvAttr[AIndex],
        ValBuf);

      MMBAdd(&SR->MB,tmpLine,MSCHED_SNAME,mmbtAnnotation,MMAX_TIME,0,NULL);
      }  /* END if (rc == FAILURE) */
 
    ptr = MUStrTokE(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MSRProcessConfig() */





/**
 * Report standing reservation state.
 *
 * @see MSRConfigShow() - parent
 *
 * @param SR (I)
 * @param String (O)
 * @param VFlag (I)
 * @param PIndex (I) parameter index
 */

int MSRShow(

  msrsv_t           *SR,
  mstring_t         *String,
  int                VFlag,
  enum MCfgParamEnum PIndex)

  {
  mbitmap_t BM;

  int  aindex;

  enum MSRAttrEnum AList[] = {
    msraAccountList, 
    msraClassList,  
    msraClusterList,
    msraDays,
    msraEndTime,
    msraFlags,
    msraGroupList,
    msraHostList,
    msraJobAttrList,
    msraMaxJob,
    msraMinMem,
    msraNodeFeatures,
    msraOwner,
    msraPeriod,
    msraQOSList, 
    msraStartTime,
    msraTaskCount,
    msraTimeLimit,
    msraTrigger,
    msraUserList,    
    msraVariables,
    msraNONE };

  /* initialize output */

  if (String != NULL)
    MStringSet(String,"\0");

  if ((SR == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  /* display SR if enabled */

  if ((SR->Name[0] == '\0') ||(SR->Name[0] == '\1') ||
      (PIndex != mcoNONE))
    {
    return(SUCCESS);
    }

  MStringSetF(String,"%s[%s] ",
    MCredCfgParm[mxoSRsv],
    SR->Name);

  mstring_t tmpString(MMAX_LINE);

  /* show attributes */

  if (VFlag != 0)
    bmset(&BM,mcmVerbose);

  for (aindex = 0;AList[aindex] != msraNONE;aindex++)
    {
    if (MSRAToString(
          SR,
          AList[aindex],
          &tmpString,
          &BM) == FAILURE)
      {
      continue;
      }

    if (tmpString.empty())
      {
      continue;
      }

    MStringAppendF(String,"%s=%s ",
      MSRsvAttr[AList[aindex]],
      tmpString.c_str());
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MSRShow() */





/**
 * Diagnose standing reservation state/config (process 'mdiag -s').
 *
 * @see MUIDiagnose() - parent
 * @see MUIRsvDiagnose() - peer - diagnose reservations
 *
 * NOTE:  if DFormat==mfmXML, SBuffer will be string <Data>...</Data>
 *
 * NOTE:  This function has been deprecated to be replaced by MSRDiagXML in Moab 6.1. We're
 *        keeping this around for reference.
 *
 * @param SRS (I)
 * @param String (O)
 * @param DFormat (I)
 * @param SArray (I)
 * @param FlagBM (I) [bitmap of enum mcm*]
 */

int MSRDiag(

  msrsv_t   *SRS,
  mstring_t *String,
  enum MFormatModeEnum DFormat,
  marray_t  *SArray,
  mbitmap_t *FlagBM)

  {
  int srindex;
  int rindex;
  int pindex;

  char tmpLine[MMAX_LINE];
  char TString[MMAX_LINE];

  char StartTime[MMAX_NAME];
  char EndTime[MMAX_NAME];
  char Duration[MMAX_NAME];

  mxml_t *DE;
  mxml_t *SRE;

  marray_t *SRP;

  msrsv_t *SR;
  mrsv_t  *R;

  mbitmap_t BM;

  enum MSRAttrEnum AList[] = {
    msraAccess,
    msraActionTime,
    msraAccountList,
    msraChargeAccount,
    msraChargeUser,
    msraClassList,
    msraClusterList,
    msraComment,
    msraDays,
    msraDepth,
    msraDisable,
    msraDisableTime,
    msraEndTime,
    msraFlags,
    msraGroupList,
    msraHostList,
    msraIdleTime,
    msraJobAttrList,
    msraMaxTime,
    msraMinMem,
    msraName,
    msraNodeFeatures,
    msraOS,
    msraOwner,
    msraPartition,
    msraPeriod,
    msraPriority,
    msraProcLimit,
    msraPSLimit,
    msraQOSList,
    msraResources,
    msraRollbackOffset,
    msraRsvList,
    msraStartTime,
    msraStIdleTime,
    msraStTotalTime,
    msraTaskCount,
    msraTaskLimit,
    msraTimeLimit,
    msraTPN,
    msraTrigger,
    msraUserList,
    msraWEndTime,
    msraWStartTime,
    msraNONE };

  mbool_t IsPartial;

  if (String == NULL)
    {
    return(FAILURE);
    }

  if (SArray != NULL)
    {
    SRP = SArray;
    }
  else
    {
    SRP = &MSRsv;    
    }

  /* initialize */

  if (DFormat != mfmXML)
    {
    MStringSet(String,"\0");
    }
  else
    {
    DE = NULL;

    MXMLCreateE(&DE,(char*)MSON[msonData]);
    }

  /* display header */

  if (DFormat == mfmXML)
    {
    /* NO-OP */
    }
  else
    {
    MStringAppendF(String,"standing reservation overview\n\n");

    MStringAppendF(String,"%-20s %10s %4.4s %11s %11s  %11s %10s\n",
      "RsvID",
      "Type",
      "Par",
      "StartTime",
      "EndTime",
      "Duration",
      "Period");

    MStringAppendF(String,"%-20s %10s %4.4s %11s %11s  %11s %10s\n",
      "-----",
      "----",
      "---",
      "---------",
      "-------",
      "--------",
      "------");
    }

  bmset(&BM,mfmHuman);
  bmset(&BM,mfmVerbose);

  for (srindex = 0;srindex < SRP->NumItems;srindex++)
    {
    SR = (msrsv_t *)MUArrayListGet(SRP,srindex);

    if (SR == NULL)
      break;

    if ((SR->Name[0] == '\0') || (SR->Name[0] == '\1'))
      continue;

    if ((SRS != NULL) && (SRS != SR))
      continue;

    if (DFormat == mfmXML)
      {
      SRE = NULL;

      MXMLCreateE(&SRE,(char *)MXO[mxoSRsv]);

      MSRToXML(SR,SRE,(enum MSRAttrEnum *)AList);

      MXMLAddE(DE,SRE);

      /* NOTE:  do not add diagnostic messages to XML */
 
      continue;
      }
    else
      {
      /* NOTE:  do not add config information to non-XML */

      /* NO-OP */
      }

    MULToTString((long)SR->StartTime,TString);
    MUStrCpy(StartTime,TString,sizeof(StartTime));

    if (SR->EndTime > 0)
      {
      MULToTString((long)SR->EndTime,TString);

      MUStrCpy(EndTime,TString,sizeof(EndTime));
      }
    else
      {
      MUStrCpy(EndTime,"---",sizeof(EndTime));
      }

    if (SR->Duration > 0)
      {
      MULToTString(SR->Duration,TString);

      MUStrCpy(Duration,TString,sizeof(Duration));
      }
    else
      {
      MULToTString(((long)SR->EndTime - (long)SR->StartTime),TString);

      MUStrCpy(Duration,TString,sizeof(Duration));
      }

    MStringAppendF(String,"\n%-20s %10s %4.4s %11s %11s  %11s %10s\n",
      SR->Name,
      MRsvType[SR->Type],
      (SR->PName[0] != '\0') ? SR->PName : "---",
      StartTime,
      EndTime,
      Duration,
      MCalPeriodType[SR->Period]);

    if (SR->Period == mpDay)
      {
      char tmpLine[MMAX_LINE];

      if (bmisclear(&SR->Days))
        {
        MStringAppendF(String,"  ALERT:  standing reservation not enabled for any day of the week - set attributes DAYS\n");
        }
      else if (bmisset(&SR->Days,MCONST_ALLDAYS))
        {
        MStringAppend(String,"  Days:        ALL\n");
        }
      else
        {
        MStringAppendF(String,"  Days:        %s",
          bmtostring(&SR->Days,MWeekDay,tmpLine));

        if (bmisset(&SR->Days,0))
          {
          /* NOTE:  bmtostring() will not display index 0, ie Sun */

          MStringAppendF(String,",%s\n",
            MWeekDay[0]);
          }

        MStringAppendF(String,"\n");
        }  /* END else */
      }    /* END if (SR->Perion == mpDay) */

    MACLListShow(SR->ACL,maNONE,&BM,tmpLine,sizeof(tmpLine));

    if (tmpLine[0] != '\0')
      {
      MStringAppendF(String,"  ACL:         %s\n",
        tmpLine);
      }

    if (!bmisclear(&SR->Flags))
      {
      mstring_t Line(MMAX_LINE);

      bmtostring(&SR->Flags,MRsvFlags,&Line);

      MStringAppendF(String,"  Flags:       %s\n",
        Line.c_str());
      }

    if (SR->OS != NULL)
      {
      MStringAppendF(String,"  OS:          %s\n",
        SR->OS);
      }

    if (SR->ReqTC > 0)
      {
      MStringAppendF(String,"  TaskCount:   %d\n",
        SR->ReqTC);
      }

    if ((SR->DRes.Procs != -1) || (!MSNLIsEmpty(&SR->DRes.GenericRes)))
      {
      mstring_t tmp(MMAX_LINE);

      MSRAToString(SR,msraResources,&tmp,0);

      MStringAppendF(String,"  Resources:   %s\n",
        tmp.c_str());
      }

    if (SR->Depth > 0)
      {
      MStringAppendF(String,"  Depth:       %d\n",
        SR->Depth);
      }

    if (SR->O != NULL)
      {
      MStringAppendF(String,"  Owner:       %s\n",
        SR->O->Name);
      }

    if (SR->SubType != mrsvstNONE)
      {
      MStringAppendF(String,"  Subtype:     %s\n",
        MRsvSubType[SR->SubType]);
      }

    if (SR->MaxJob > 0)
      {
      MStringAppendF(String,"  MaxJob:      %d\n",
        SR->MaxJob);
      }

    if (SR->LogLevel > 0)
      {
      MStringAppendF(String,"  LogLevel:    %d\n",
        SR->LogLevel);
      }

    {
    mstring_t tmp(MMAX_LINE);

    MSRAToString(SR,msraRsvList,&tmp,0);

    if (!MUStrIsEmpty(tmp.c_str()))
      {
      MStringAppendF(String,"  RsvList:     %s\n",
        tmp.c_str());
      }
    else
      {
      MStringAppendF(String,"  ALERT:  no child reservations detected\n");
      }
    }
 
    for (pindex = 0;pindex < MMAX_SRSV_DEPTH;pindex++)
      {
      if (SR->DisabledTimes[pindex] > MSched.Time)
        {
        MStringAppendF(String,"  NOTE:  standing reservation is disabled for period ending in %ld seconds, use 'mrsvctl -C %s' to restore\n",
          SR->DisabledTimes[pindex] - MSched.Time,
          SR->Name);
        }
      }    /* END for (pindex) */

    if (SR->T != NULL)
      {
      mstring_t tmp(MMAX_LINE);

      /* xml data displayed in MXMLToString() */

      MTListToMString(SR->T,TRUE,&tmp);

      MStringAppendF(String,"  Trigger:     %s\n",tmp.c_str());
      }

    /* evaluate configuration */

    if ((SR->ReqTC <= 0) &&
        (SR->HostExp[0] == '\0'))
      {
      /* NOTE:  SR does not specify tasks */

      MStringAppendF(String,"  ALERT:  SR %s does not specify requested resources (taskcount/hostexpression not set)\n",
        SR->Name);
 
      continue;
      }

    if (!bmisclear(&SR->FBM))
      {
      char Line[MMAX_LINE];

      MUNodeFeaturesToString(',',&SR->FBM,Line);

      MStringAppendF(String,"  FeatureList: %s\n",
        Line);

      if (SR->FeatureMode != 0)
        {
        MStringAppendF(String,"  FeatureMode: %s\n",
          MCLogic[SR->FeatureMode]);
        }
      }  /* END if (SR->FBM != NULL) */

    if (SR->Type == mrtUser)
      {
      if (SR->A != NULL)
        {
        MStringAppendF(String,"  Accounting Creds: %s%s%s%s\n",
          (SR->A != NULL) ? " Account:" : "",
          (SR->A != NULL) ? SR->A->Name : "",
          (SR->U != NULL) ? " User:" : "",
          (SR->U != NULL) ? SR->U->Name : "");
        }  /* END if (SR->A != NULL) ... ) */
      }    /* END if (SR->Type == mrtUser) */

    if (SR->O != NULL)
      {
      MStringAppendF(String,"  Owner:       %s:%s\n",
        MXO[SR->OType],
        SR->O->Name);
      }

    if ((SR->HostExp != NULL) && (SR->HostExp[0] != '\0'))
      {
      MStringAppendF(String,"  HostExp:     '%s'\n",
        SR->HostExp);
      }

    if (SR->RollbackOffset > 0)
      {
      MULToTString((long)SR->RollbackOffset,TString);

      MStringAppendF(String,"  RollBack:    %s\n",
        TString);
      }

    if (SR->Priority > 0)
      {
      MStringAppendF(String,"  Priority:    %ld\n",
        SR->Priority);
      }

    if (SR->MB != NULL)
      {
      char tmpBuf[MMAX_BUFFER];

      MMBPrintMessages(SR->MB,mfmHuman,FALSE,-1,tmpBuf,sizeof(tmpBuf));
 
      MStringAppend(String,tmpBuf);
      }

    /* evaluate active reservations */

    for (rindex = 0;rindex < MMAX_SRSV_DEPTH;rindex++)
      {
      R = SR->R[rindex];

      if (R == NULL)
        {
        /* if reservation should exist, determine why missing */

        /* NYI */

        continue;
        }

      IsPartial = FALSE;

      if (SR->ReqTC > 0)
        {
        if (R->AllocTC < SR->ReqTC)
          IsPartial = TRUE;

        MStringAppendF(String,"  reservation %s (depth %d) is %s (%d of %d tasks reserved)\n",
          R->Name,
          rindex,
          (IsPartial == TRUE) ? "incomplete" : "complete",
          R->AllocTC,
          SR->ReqTC);
        } 
      else
        {
        MStringAppendF(String,"  reservation %s exists (depth %d)\n",
          R->Name,
          rindex);
        }

      if (IsPartial == TRUE)
        {
        /* determine why resources are unavailable */

        /* NYI */
        }
      }    /* END for (rindex) */
    }      /* END for (srindex) */

  switch (DFormat)
    {
    case mfmXML:

      MXMLToMString(DE,String,NULL,TRUE);

      MXMLDestroyE(&DE);

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case mfmHuman:
    default:

      /* NO-OP */

      break;
    }  /* END switch (DFormat) */

  return(SUCCESS);
  }  /* END MSRDiag() */





/**
 * Diagnose standing reservation state/config (process 'mdiag -s'). Returns only XML.
 *
 * @see MUIRsvDiagnoseXML() - peer
 *
 * @param S       (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth    (I) authorization credentials for request
 */

int MSRDiagXML(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  int srindex;
  /*int rindex;*/
  /*int pindex;*/

  char DiagOpt[MMAX_LINE];
  /*char FlagString[MMAX_LINE];*/
  char SRsvID[MMAX_LINE];


  /*mstring_t tmpLine;*/

  mxml_t *DE;
  mxml_t *SRE;

  msrsv_t *SR;
  /*mrsv_t  *R;*/

  marray_t *SRList = &MSRsv;

  enum MSRAttrEnum AList[] = {
    msraAccess,
    msraActionTime,
    msraAccountList,
    msraChargeAccount,
    msraChargeUser,
    msraClassList,
    msraClusterList,
    msraComment,
    msraDays,
    msraDepth,
    msraDisable,
    msraDisableTime,
    msraDisabledTimes,
    msraEndTime,
    msraFlags,
    msraGroupList,
    msraHostList,
    msraIdleTime,
    msraJobAttrList,
    msraMaxJob,
    msraMaxTime,
    msraMinMem,
    msraMMB,
    msraName,
    msraNodeFeatures,
    msraOS,
    msraOwner,
    msraPartition,
    msraPeriod,
    msraPriority,
    msraProcLimit,
    msraPSLimit,
    msraQOSList,
    msraResources,
    msraRollbackOffset,
    msraRsvList,
    msraStartTime,
    msraStIdleTime,
    msraStTotalTime,
    msraTaskCount,
    msraTaskLimit,
    msraTimeLimit,
    msraTPN,
    msraTrigger,
    msraType,
    msraUserList,
    msraWEndTime,
    msraWStartTime,
    msraNONE };

  SRsvID[0] = '\0';

  if (MXMLCreateE((mxml_t **)&S->SDE,(char*)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    MCacheJobLock(FALSE,TRUE);
    return(FAILURE);
    }

  DE = (mxml_t *)S->SDE;

  /* look for a specified resveration id */

  if (MXMLGetAttr((mxml_t *)S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    MUStrCpy(SRsvID,DiagOpt,sizeof(SRsvID));
    }

  for (srindex = 0;srindex < SRList->NumItems;srindex++)
    {
    SR = (msrsv_t *)MUArrayListGet(SRList,srindex);

    if (SR == NULL)
      break;

    if ((SR->Name[0] == '\0') || (SR->Name[0] == '\1'))
      continue;

    if ((!MUStrIsEmpty(SRsvID)) && (strcmp(SR->Name,SRsvID) != 0))
      continue;

    SRE = NULL;

    MXMLCreateE(&SRE,(char *)MXO[mxoSRsv]);

    MSRToXML(SR,SRE,(enum MSRAttrEnum *)AList);

    MXMLAddE(DE,SRE);

    } /* END for (srindex...) */

  return(SUCCESS);
  } /* END MSRDiagXML() */




/**
 * Initialize new standing reservation.
 *
 * @param SR (I) [modified]
 * @param Name (I)
 */

int MSRInitialize(
 
  msrsv_t *SR,   /* I (modified) */
  char    *Name) /* I */
 
  {
  int srindex;
  msrsv_t *tmpSR;

  if ((SR == NULL) || (Name == NULL))
    {
    return(FAILURE);
    }

  for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
    {
    tmpSR = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);

    if (tmpSR == SR)
      {
      SR->Index = srindex;

      break;
      }
    }    /* END for (srindex) */

  if (strcmp(SR->Name,Name))
    { 
    MUStrCpy(SR->Name,Name,sizeof(SR->Name));
    }
 
  SR->Type       = mrtUser;
  SR->Depth      = MDEF_SRDEPTH;
  bmset(&SR->Days,MCONST_ALLDAYS);
 
  SR->StartTime  = MDEF_SRSTARTTIME;
  SR->EndTime    = MDEF_SRENDTIME;
 
  SR->WStartTime = MDEF_SRSTARTTIME;
  SR->WEndTime   = MDEF_SRENDTIME;
 
  MCResInit(&SR->DRes);

  SR->DRes.Procs = MDEF_SRPROCS;

  MNLFree(&SR->HostList);
  MNLInit(&SR->HostList);

  SR->Priority   = MDEF_SRPRIORITY;
  SR->Period     = MDEF_SRPERIOD;

  return(SUCCESS);
  }  /* END MSRInitialize() */




/**
 *
 *
 * @param SRP (I) [optional]
 * @param VFlag (I)
 * @param PIndex (I)
 * @param String (O)
 */

int MSRConfigShow(

  msrsv_t           *SRP,
  int                VFlag,
  enum MCfgParamEnum PIndex,
  mstring_t         *String)

  {
  int srindex;

  msrsv_t *SR;


  const char *FName = "MSRConfigShow";

  MDB(4,fCORE) MLog("%s(SRP,%d,%d,String)\n",
    FName,
    VFlag,
    PIndex);
 
  if (String == NULL)
    {
    return(FAILURE);
    }

  String->clear();

  mstring_t tmpString(MMAX_LINE);

  for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
    {
    SR = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);
 
    if ((SR->ReqTC == 0) &&
        (SR->HostExp[0] == '\0') &&
        !VFlag)
      {
      continue;
      }
 
    if ((MSRShow(SR,&tmpString,VFlag,PIndex) == SUCCESS) &&
        (!tmpString.empty()))
      {
      MStringAppendF(String,"%s\n",
        tmpString.c_str()); 
      }
    }  /* END for (srindex) */
  
  return(SUCCESS);
  }  /* END MSRConfigShow() */





/**
 *
 *
 * @param RPP (I) [optional]
 * @param VFlag (I)
 * @param PIndex (I)
 * @param String (O)
 */

int MRsvProfileConfigShow(

  msrsv_t           *RPP,
  int                VFlag,
  enum MCfgParamEnum PIndex,
  mstring_t         *String)

  {
  int rpindex;

  mrsv_t *R;

  char *tBPtr;
  int   tBSpace;

  char  RPArgs[MMAX_LINE];

  char  tmpLine[MMAX_LINE];

  const char *FName = "MRsvProfileConfigShow";

  MDB(4,fCORE) MLog("%s(RPP,%d,%d,String)\n",
    FName,
    VFlag,
    PIndex);

  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  for (rpindex = 0;rpindex < MMAX_RSVPROFILE;rpindex++)
    {
    R = MRsvProf[rpindex];

    if ((R == NULL) || (R->Name[0] == '\0'))
      break;

    if (R->Name[0] == '\1')
      continue;

    MUSNInit(&tBPtr,&tBSpace,RPArgs,sizeof(RPArgs));

    if (R->T != NULL)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"  %s=%s",
        MRsvAttr[mraTrigger],
        "set");
      }

    if (R->Comment != NULL)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"  %s='%s'",
        MRsvAttr[mraComment],
        R->Comment);
      }

    if (R->ReqTC > 0)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"  %s=%d",
        MRsvAttr[mraReqTaskCount],
        R->ReqTC);
      }

    MUShowSSArray(
      MParam[mcoRsvProfile],
      R->Name,
      RPArgs,
      tmpLine,
      sizeof(tmpLine));

    MStringAppend(String,tmpLine);
    }  /* END for (rpindex) */

  return(SUCCESS);
  }  /* END MRsvProfileConfigShow() */





/**
 * Report specified SR attributes.
 *
 * @param SR (I) standing reservation description
 * @param PeriodIndex (I) period index
 * @param SRStartTime (O) time reservation should start
 * @param SRDuration (O) duration of reservation
 */

int MSRGetAttributes(
 
  msrsv_t *SR,          /* I standing reservation description */
  int      PeriodIndex, /* I period index                     */
  mulong  *SRStartTime, /* O time reservation should start    */
  mulong  *SRDuration)  /* O duration of reservation          */
 
  {
  int          sindex;

  mulong       SRStartOffset;
  mulong       SREndOffset;
 
  mulong       DayTime;
  mulong       WeekTime;

  unsigned int WeekDay;
 
  struct tm *Time;
 
  long       PeriodStart;
  long       tmpL;
 
  time_t     tmpTime;
  char       TString[MMAX_LINE];
  char       DString[MMAX_LINE];

  const char *FName = "MSRGetAttributes";
 
  MDB(3,fSTRUCT) MLog("%s(%s,%d,Start,Duration)\n",
    FName,
    SR->Name,
    PeriodIndex); 

  if ((NULL == SR) || 
      (NULL == SRStartTime) || 
      (NULL == SRDuration))
      return(FAILURE);

  /* determine absolute rsv timeframe */

  MUGetPeriodRange(
    (SR->EnableTime != 0) ? SR->EnableTime : MSched.Time,
    0,
    PeriodIndex,
    SR->Period,
    &PeriodStart,
    NULL,
    NULL);
 
  if (PeriodIndex == 0)
    tmpTime = (time_t)MAX((mulong)PeriodStart,MSched.Time);
  else
    tmpTime = (time_t)PeriodStart;

  if ((PeriodIndex == 0) && (SR->EnableTime != 0))
    tmpTime = (time_t)MAX((mulong)PeriodStart,SR->EnableTime);
 
  Time = localtime(&tmpTime);
 
  DayTime  = (3600 * Time->tm_hour) + (60 * Time->tm_min) + Time->tm_sec;
  WeekTime = DayTime + (Time->tm_wday * MCONST_DAYLEN);
  WeekDay  = Time->tm_wday;
 
  /* set reservation time boundaries */
 
  SRStartOffset = 0;
 
  switch (SR->Period)
    {
    case mpMinute:

      SREndOffset = MCONST_MINUTELEN;

      if (SR->StartTime > 0)
        {
        SRStartOffset = SR->StartTime;
        }

      if (SR->Duration > 0)
        {
        SREndOffset = SRStartOffset + SR->Duration;
        }

      break;

    case mpHour:

      SREndOffset = MCONST_HOURLEN;

      if (SR->StartTime > 0)
        {
        SRStartOffset = SR->StartTime;
        }

      if (SR->Duration > 0)
        {
        SREndOffset = SRStartOffset + SR->Duration;
        }       
 
      break;

    case mpDay:
 
      SREndOffset = MCONST_DAYLEN;
 
      if ((SR->WStartTime > 0) &&
          (WeekDay == (SR->WStartTime / MCONST_DAYLEN)))
        {
        SRStartOffset = SR->WStartTime % MCONST_DAYLEN;
        }
 
      if (SR->StartTime > 0)
        {
        SRStartOffset = SR->StartTime;
        }
 
      if ((SR->WEndTime > 0) &&
          (WeekDay == (SR->WEndTime / MCONST_DAYLEN)))
        {
        SREndOffset = SR->WEndTime % MCONST_DAYLEN; 
        }
 
      if (SR->EndTime > 0)
        {
        SREndOffset = SR->EndTime;
        }
 
      if (PeriodIndex == 0)
        {
        SRStartOffset = MAX(SRStartOffset,DayTime);
        }
 
      break;
 
    case mpWeek:
 
      SREndOffset = MCONST_WEEKLEN;
 
      if (SR->WStartTime > 0)
        {
        SRStartOffset = SR->WStartTime;
        }
 
      if (SR->WEndTime > 0)
        {
        SREndOffset = SR->WEndTime;
        }
 
      if (PeriodIndex == 0)
        {
        SRStartOffset = MAX(SRStartOffset,WeekTime);
        }
 
      break;

    case mpMonth:

      /* NOTE:  month's vary in length (FIXME) */

      SREndOffset = MCONST_DAYLEN * 30;

      if (SR->WStartTime > 0)
        {
        SRStartOffset = SR->WStartTime;
        }

      if (SR->WEndTime > 0)
        {
        SREndOffset = SR->WEndTime;
        }

      if (PeriodIndex == 0)
        {
        SRStartOffset = MAX(SRStartOffset,WeekTime);
        }

      break;
 
    case mpInfinity:
    default:
 
      /* Durations of MMAX_TIME make it so jobs defer because Moab can't
       * find a time, which is never, to make a reservation. */

      SREndOffset = MCONST_EFFINF;
 
      break;
    }  /* END switch (SR->Period) */

  /* determine if reservation is needed for current period */
 
  switch (SR->Period)
    {
    case mpMinute:

      /* ignore if we're coming in after starttime */

      if ((PeriodStart + SRStartOffset) < MSched.Time)
        {
        return(FAILURE);
        }

      break;

    case mpHour:

      /* ignore if we're coming in after starttime */

      if ((PeriodStart + SRStartOffset) < MSched.Time)
        {
        return(FAILURE);
        }

      break;

    case mpDay:
 
      if ((SR->Period == mpWeek) && ((SR->WStartTime / MCONST_DAYLEN) > WeekDay))
        {
        /* ignore, week reservation has not yet started */
 
        MDB(6,fSTRUCT) MLog("INFO:     week reservation %s starts after day %d\n",
          SR->Name,
          WeekDay);
 
        return(FAILURE);
        }
 
      if ((SR->WEndTime > 0) && (SR->WEndTime < WeekTime))
        {
        /* ignore, week reservation has ended */
 
        MULToTString(WeekTime,TString);

        MDB(6,fSTRUCT) MLog("INFO:     week reservation %s ends before %s\n",
          SR->Name,
          TString);
 
        return(FAILURE);
        }
 
      if (DayTime > SREndOffset)
        {
        /* ignore, day reservation has already ended */

        MULToTString(DayTime,TString);

        MDB(6,fSTRUCT) MLog("INFO:     day reservation %s ends before %s\n",
          SR->Name,
          TString);
 
        return(FAILURE);
        }
 
      if (!bmisset(&SR->Days,WeekDay))
        {
        /* day period is not included in SRDays */
 
        MDB(6,fSTRUCT) MLog("INFO:     day reservation %s does not include day %d\n",
          SR->Name,
          WeekDay);
 
        return(FAILURE);
        }
 
      break;
 
    case mpWeek:
 
      if (WeekTime > SREndOffset)
        {
        /* ignore, week reservation has ended */

        MULToTString(WeekTime,TString);

        MDB(6,fSTRUCT) MLog("INFO:     week reservation %s ends before %s\n",
          SR->Name,
          TString);
 
        return(FAILURE);
        }
 
      break;
 
    case mpInfinity:
    default:
 
      if (PeriodIndex != 0)
        {
        /* ignore, only one infinite period reservation */ 
 
        MDB(6,fSTRUCT) MLog("INFO:     ignoring infinite srsv %s for period %d\n",
          SR->Name,
          PeriodIndex);
 
        return(FAILURE);
        }

      /* check if absolute times specified */

      if (SR->StartTime > MCONST_YEARLEN)
        {
        SRStartOffset = SR->StartTime;
        }
      else
        {
        /* no start specified - use epoch */

        /* NO-OP */
        }

      if (SR->EndTime > MCONST_YEARLEN)
        {
        SREndOffset = SR->EndTime;
        }
      else
        { 
        /* reservation not ended - use MMAX_TIME */
   
        /* NO-OP */
        }
 
      break;
    }  /* END switch (SR->Period) */
 
  tmpL = (PeriodStart + SRStartOffset);

  if (SR->DisableTime != 0)
    {
    /* ??? */
    }

  MULToDString((mulong *)&tmpL,DString);

  MDB(5,fSTRUCT) MLog("INFO:     srsv start: %s",
    DString);
 
  MULToTString(DayTime,TString);

  MDB(5,fSTRUCT) MLog("INFO:     srsv attributes: Start: %ld  Duration: %ld (W: %d  D: %s)\n",
    PeriodStart + SRStartOffset,
    SREndOffset - SRStartOffset,
    WeekDay,
    TString);

  if (SRDuration != NULL)
    *SRDuration = SREndOffset - SRStartOffset;

  if (SRStartTime != NULL)
    {
    *SRStartTime = PeriodStart + SRStartOffset;

    MDB(5,fSTRUCT) MLog("INFO:     PeriodStart: %ld StartOffset: %ld Period: %s RollbackOffset: %ld\n",
      PeriodStart,
      SRStartOffset,
      MCalPeriodType[SR->Period],
      SR->RollbackOffset);

    if ((*SRStartTime == 0) && 
        (SR->Period == mpInfinity) &&
        (SR->RollbackOffset > 0))
      {
      /* Set the time for a rollback reservation */

      *SRStartTime = MSched.Time;

      MDB(5,fSTRUCT) MLog("INFO:    Setting StartTime to %ld\n",
        MSched.Time);
      }
    }

  for (sindex = 0;sindex < MMAX_SRSV_DEPTH;sindex++)
    {
    if (SR->DisabledTimes[sindex] == 0)
      {
      /* end of list */

      break;
      }

    if (SR->DisabledTimes[sindex] == 1)
      {
      /* slot no longer used */

      continue;
      }

    if (SR->DisabledTimes[sindex] == (*SRStartTime + *SRDuration))
      {
      /* disabled time matches end time of rsv */

      return(FAILURE);
      }
    }    /* END for (sindex) */

  for (sindex = 0;sindex < MMAX_SRSV_DEPTH;sindex++)
    {
    if (SR->R[sindex] == NULL)
      continue;

    if ((SR->R[sindex]->EndTime == (*SRStartTime + *SRDuration)) &&
        (SR->R[sindex]->AllocTC >= SR->ReqTC))
      {
      return(FAILURE);
      }
    }    /* END for (sindex) */

  return(SUCCESS);
  }  /* END MSRGetAttributes() */


  
     
/**
 * Determine best nodes for admin and standing reservations and create them.
 *
 * NOTE: MRsvConfigure() will add triggers for Standing Reservations into the
 *       global trigger table.  It WILL NOT add triggers for admin reservations
 *       into the global trigger table
 *
 * NOTE: if R exists, allocate resources to R with current allocated list 
 *       required
 *
 * For standing reservations SR->HostList contains all possible hosts that the
 * reservation can run on if a host expression was specified.  For the 
 * instantiations of the standing reservation, R->ReqNL is NOT populated and 
 * therefore NOT checkpointed.  Only the AllocNodeList is checkpointed for 
 * standing reservations.  When SR->HostList is not NULL it is used to rebuild
 * standing reservations in the case of failures.
 *
 * For admin reservations, R->ReqNL contains the required host list.  This is 
 * checkpointed and can be used to determine the best nodes to use after a 
 * restart.  R->ReqNL is a combination of the host expression and the 
 * taskcount.  When rebuilding R after a failure a new R->ReqNL is calculated 
 * based on the host expression and/or task constraints.
 *
 * @see MUIRsvCreate() - parent ('process 'msrvctl -c' and 'setres')
 *
 * @param SR            (I) [optional]
 * @param R             (I) [optional]
 * @param SRIndex       (I) not used if SR == NULL [optional]
 * @param DIndex        (I) not used if SR == NULL
 * @param EMsg          (O) [optional,minsize=MMAX_LINE]
 * @param SROP          (O) [pointer]
 * @param SRsv          (I) [optional]
 * @param DoCheckLimits (I)
 */

int MRsvConfigure(

  msrsv_t  *SR,
  mrsv_t   *R,
  int       SRIndex,
  int       DIndex,
  char     *EMsg,
  mrsv_t  **SROP,
  marray_t *SRsv,
  mbool_t   DoCheckLimits)

  {
  int ReturnCode = SUCCESS;

  mjob_t     *J;

  mulong      BestTime;

  int         index;
  int         aindex;
  int         nindex;

  char        TempString[MMAX_NAME];
  char        MsgBuf[MMAX_BUFFER];

  int         TC;
  int         PC;

  marray_t   *SRsvArray;

  mnl_t   NodeList;
  mnl_t   tmpANodeList;

  mreq_t     *RQ;
 
  mbitmap_t Flags;

  mgcred_t  **RAAP;
  macl_t     *RACL;
  macl_t     *RCL;

  mln_t      *RVariables;

  char*       RComment = NULL;
  char*       RSysJobTemplate = NULL;

  mcres_t    *RDRes;

  mbitmap_t RFlags;

  mbool_t     ModifyRsv     = FALSE;

  mbitmap_t RFBM;

  enum MCLogicEnum RFeatureMode;

  char        RName[MMAX_NAME];
  char       *RSpecName = NULL;
  char       *RHostExp = NULL;

  mnl_t   LocalRHostList;  /* not always used, only used if SR->HostList or R->ReqNL are NULL */

  mbool_t     RHostExpIsSpecified = MBNOTSET;
  mnl_t  *RHostList;

  int         ReqOS;
  int         ReqArch;
  int         ReqMem;

  enum MHostListModeEnum ReqHLMode;

  char        RPName[MMAX_NAME];
  long        RPriority;

  char        SRGName[MMAX_NAME];

  char       *ROS   = NULL;

  char       *GRID = NULL;
  char       *SID  = NULL;

  enum MRsvSubTypeEnum RType;

  enum MVMUsagePolicyEnum RVMUsagePolicy;

  char        ORName[MMAX_NAME];
  mbool_t     ORIsActive = FALSE;

  int         RReqOS;

  void       *ROwner;
  enum MXMLOTypeEnum ROType;

  mgcred_t   *RAU = NULL;
  mgcred_t   *RAG = NULL;
  mqos_t     *RAQ = NULL;

  char        RLabel[MMAX_NAME];

  int         RNC;
  int         RTC;
  int         RTPN;

  mmb_t      *RMB = NULL;

  marray_t   *RTList;
  marray_t   *SRTList = NULL;

  mulong      RStartTime;
  mulong      RDuration;
  mulong      REndTime;
  mulong      RExpireTime;

  int         TotalTC;
  int         TotalNC;
  int         ProcCount;
  int         NodeCount;            

  int         LogLevel;

  mrsv_t    **ROP = NULL;
  mrsv_t     *RO;

  enum MRsvAllocPolicyEnum RAllocPolicy;

  mbool_t     IsSpaceFlex;
  mbool_t     RIsTracked = FALSE;
  mbool_t     RHostListIsLocal;
  mbool_t     RsvIsAdmin;        /* what does this mean? */
  mbool_t     EnforceHostExp;
  mbool_t     RecycleUID = FALSE;  /* TRUE if MRsvRecycleUID() needs to be called on failure */
  mbool_t     TCIsMax = FALSE;

  double      CurrentIdlePS;
  double      CurrentActivePS;
  double      TotalIdlePS;
  double      TotalActivePS;

  char       *RsvExcludeList[MMAX_PJOB]; 
  char        AllJob[MMAX_NAME];

  char        TString[MMAX_LINE];

  mbitmap_t   ModeBM;  /* bitmap of enum MServiceType */

  mpar_t     *P = NULL;

  const char *FName = "MRsvConfigure";

  MDB(3,fSCHED) MLog("%s(%s,%s,%d,EMsg,RP)\n",
    FName,
    (SR != NULL) ? SR->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    DIndex);

  if (SROP != NULL)
    *SROP = NULL;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SRsv != NULL)
    {
    SRsvArray = SRsv;
    }
  else
    {
    SRsvArray = &MSRsv;
    }

  if ((SR == NULL) && (R == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal failure");

    return(FAILURE);
    }

  if ((R != NULL) && (SROP == NULL))
    {
    ModifyRsv = TRUE;
    }

  /* NOTE:  if RP specified, R is template */

  /* build rsv request job */

  MUStrCpy(AllJob,"[ALLJOB]",sizeof(AllJob));

  memset(RsvExcludeList,0,sizeof(RsvExcludeList));

  ORName[0] = '\0';

  MNLInit(&NodeList);
  MNLInit(&tmpANodeList);

  MJobMakeTemp(&J);

  RQ = J->Req[0];

  J->SubmitRM = NULL;
  
  J->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

  J->RsvExcludeList = RsvExcludeList;

  RsvIsAdmin = FALSE;

  EnforceHostExp = FALSE;

  /* determine destination RID */

  if (SROP != NULL)
    {
    ROP = SROP;
    RO  = *ROP;  /* ROP is pointer only */
    }
  else if (SR != NULL)
    {
    ROP = &((msrsv_t *)MUArrayListGet(SRsvArray,SRIndex))->R[DIndex];
    RO  = *ROP;

    if (RO != NULL)
      {
      strcpy(ORName,RO->Name);
      ORIsActive = bmisset(&RO->Flags,mrfIsActive);
      }
    }
  else if (R != NULL)
    {
    ROP = &R;
    RO  = R;
    }
  else
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid request");

    ReturnCode = FAILURE;

    goto free_and_return;
    }

  RLabel[0] = '\0';

  ReqHLMode = mhlmNONE;

  /* load rsv config parameters */

  if (SR != NULL)
    {
    if (SR->HostExp[0] != '\0')
      RHostExpIsSpecified = TRUE;

    RAAP  = &SR->A;

    RACL  = SR->ACL;

    RCL   = NULL;
    
    RTList = SR->T; 

    RDRes = &SR->DRes;

    RHostExp = SR->HostExp;

    bmcopy(&RFBM,&SR->FBM);

    RFeatureMode = SR->FeatureMode;

    ReqArch = 0;
    ReqMem  = 0;
    ReqOS   = 0;

    LogLevel = SR->LogLevel;

    bmcopy(&RFlags,&SR->Flags);

    bmset(&RFlags,mrfStanding);

    MUStrCpy(SRGName,SR->Name,sizeof(SRGName));
   
    MUStrDup(&J->MasterJobName,SRGName);

    if (SR->OS != NULL)
      {
      ROS   = SR->OS;

      ReqOS = MUMAGetIndex(meOpsys,ROS,mVerify);

      if (ReqOS <= 0)
        {
        if (EMsg != NULL)
          snprintf(EMsg,sizeof(EMsg),"Cannot modify OS for reservation %s to %s: "
              "operating system doesn't exist\n",
              R->Name,
              ROS);

        ReturnCode = FAILURE;

        goto free_and_return;
        }
      }

    RMB   = SR->MB;

    RHostList = &SR->HostList;

    RAllocPolicy = mralpNONE;

    ROwner  = SR->O;
    ROType  = SR->OType;

    RReqOS  = SR->ReqOS;

    RVMUsagePolicy = SR->VMUsagePolicy;
   
    RVariables = NULL;

    /* determine rsv name */
    /* use ORName if present */

    if (ORName[0] != '\0')
      {
      strcpy(RName,ORName);
      }
    else
      {
      MRsvGetUID(SR->Name,-1,NULL,RName);

      RecycleUID = TRUE;
      }

    strcpy(J->Name,RName);

    strcpy(RPName,SR->PName);

    if (MParFind(RPName,&P) == FAILURE)
      {
      P = NULL;
      } 

    RPriority = SR->Priority;
    RExpireTime = 0;
 
    RTC  = SR->ReqTC;
    RNC  = SR->ReqNC;

    /* import RsvAccessList info from SR (NYI) */

    if (SR->SubType != mrsvstNONE)
      RType = SR->SubType; 
    else
      RType = mrsvstStandingRsv;

    RTPN = SR->TPN;

    if (MSRGetAttributes(
          SR,
          DIndex,
          &RStartTime,
          &RDuration) == FAILURE)
      {
      MDB(3,fUI) MLog("INFO:     rsv not required for specified period\n");

      if (RecycleUID == TRUE)
        MRsvRecycleUID(RName);

      ReturnCode = SUCCESS;

      goto free_and_return;
      }

    if ((SR->RollbackOffset > 0) && (MSched.Time + SR->RollbackOffset > RStartTime))
      {
      long tOffset;

      tOffset = MSched.Time + SR->RollbackOffset - RStartTime;

      MDB(3,fUI) MLog("INFO:    Time: %ld RollbackOffset: %ld RsvStartTime: %ld RsvDuration %ld\n",
        MSched.Time,
        SR->RollbackOffset,
        RStartTime,
        RDuration);

      if (tOffset > (long)RDuration)
        {
        /* remove rsv */

        MDB(3,fUI) MLog("INFO:     rsv not required for specified period - rollback\n");

        if (RecycleUID == TRUE)
          MRsvRecycleUID(RName);

        ReturnCode = SUCCESS;

        goto free_and_return;
        }

      /* adjust rsv */

      RStartTime += tOffset;
      RDuration -= tOffset;
      }

    /* by default our psuedo job wants physical machines */

    J->VMUsagePolicy = mvmupPMOnly;

    if (SR->RsvAList != NULL)
      {
      for (index = 0;index < MMAX_PJOB;index++)
        {
        if (SR->RsvAList[index] == NULL)
          break;

        MUStrDup(&J->RsvAList[index],SR->RsvAList[index]);
        }
      }
    }    /* END if (SR != NULL) */
  else
    {
    /* load info from specified rsv */

    RAAP   = &R->A;
    RAU    = R->U;
    RAG    = R->G;
    RAQ    = R->Q;

    RACL   = R->ACL;
    RCL    = R->CL;

    RType  = R->SubType;

    RTList = R->T;

    RDRes  = &R->DRes;

    LogLevel = R->LogLevel;

    /* NOTE: these 4 variables must be NULL'ed on the original if reservation
             creation is successful */

    RComment = R->Comment;
    RSysJobTemplate = R->SysJobTemplate;
    GRID = R->SystemRID;
    SID  = R->SystemID;
    RVariables = R->Variables;

    MUStrDup(&RSpecName,R->SpecName);

    RHostExp = R->HostExp;
    RHostExpIsSpecified = R->HostExpIsSpecified;

    ReqHLMode = R->ReqHLMode;

    bmcopy(&RFBM,&R->ReqFBM);

    RFeatureMode = R->ReqFMode;

    ReqArch = R->ReqArch;
    ReqMem  = R->ReqMemory;
    ReqOS   = R->ReqOS;

    RVMUsagePolicy = R->VMUsagePolicy;

    if (ReqOS > 0)
      ROS = MAList[meOpsys][ReqOS];

    bmcopy(&RFlags,&R->Flags);

    SRGName[0] = '\0';

    /*
    EnforceHostExp = R->EnforceHostExp;
    */

    EnforceHostExp = TRUE;

    if ((ModifyRsv == FALSE) || (EnforceHostExp == FALSE))
      RHostList = &R->ReqNL;  /* required host list */
    else
      RHostList = NULL;

    ROwner   = R->O;
    ROType   = R->OType;

    RReqOS   = R->ReqOS;

    RAllocPolicy = R->AllocPolicy;
   
    if (SROP != NULL)
      { 
      MRsvGetUID(R->SpecName,-1,RACL,RName);

      strcpy(J->Name,RName);

      RecycleUID = TRUE;
      }
    else
      {
      /* rsv is not template */

      RName[0] = '\0';

      RsvIsAdmin = TRUE;

      strcpy(J->Name,R->Name);
      }

    if (R->PtIndex != 0)
      {
      /* -1 = span partitions, > 0 specified partition */

      P = &MPar[MAX(0,R->PtIndex)]; 
      strcpy(RPName,P->Name);
      }
    else
      {
      /*  0 = any single partition */

      P = NULL;
      RPName[0] = '\0';
      }

    if (R->Label != NULL)
      MUStrCpy(RLabel,R->Label,sizeof(RLabel));

    RPriority = R->Priority;
    RExpireTime = R->ExpireTime;

    RTC  = R->ReqTC;
    RNC  = R->ReqNC;

    RTPN = R->ReqTPN;

    RStartTime = R->StartTime;

    RDuration  = R->EndTime - R->StartTime;

    RIsTracked = R->IsTracked;

    if (R->RsvAList != NULL)
      {
      for (index = 0;index < MMAX_PJOB;index++)
        {
        if (SR->RsvAList[index] == NULL)
          break;

        MUStrDup(&J->RsvAList[index],SR->RsvAList[index]);
        }
      }

    if (R->RsvExcludeList != NULL)
      memcpy(RsvExcludeList,R->RsvExcludeList,sizeof(RsvExcludeList));

    /* RsvGroup is set in MUIRsvCreate. Set it on the Job as MasterJobName 
     * so that the resveration can be place over jobs that have advres set.
     * Checked in MRsvCheckJAccess */

    if (R->RsvGroup != NULL)
      MUStrDup(&J->MasterJobName,R->RsvGroup);

    /* by default our psuedo job wants physical machines */

    J->VMUsagePolicy = mvmupPMOnly;
    }  /* END else (SR != NULL) */

  RHostListIsLocal = (RHostList == NULL) ? TRUE : FALSE;

  if (RHostListIsLocal == TRUE)
    {
    RHostList = &LocalRHostList;

    MNLInit(RHostList);
    }

  if ((RTC == 0) && (SR != NULL))
    {
    /* ignore state for hostlist-based SR */

    bmset(&RFlags,mrfIgnState);
    }

  if (bmisset(&RFlags,mrfIgnIdleJobs))
    bmset(&J->SpecFlags,mjfIgnIdleJobRsv);

  if (bmisset(&RFlags,mrfIgnState))
    bmset(&J->SpecFlags,mjfIgnState);

  if (bmisset(&RFlags,mrfIgnJobRsv))
    bmset(&J->IFlags,mjifIgnJobRsv);

  if (bmisset(&RFlags,mrfACLOverlap))
    bmset(&J->IFlags,mjifACLOverlap);

  if (bmisset(&RFlags,mrfIgnRsv))
    bmset(&J->IFlags,mjifIgnRsv);

  if ((SR != NULL) && 
      !bmisset(&SR->Flags,mrfACLOverlap) && 
      !bmisset(&SR->Flags,mrfExcludeAll) && 
      !bmisset(&SR->Flags,mrfIgnIdleJobs) && 
      !bmisset(&SR->Flags,mrfIgnJobRsv))
    {
    /* standing reservations should always ignore rsv, unless some other "ignore" type flag was specified */

    bmset(&J->IFlags,mjifIgnRsv);
    }

  bmset(&J->SpecFlags,mjfRsvMap);
  bmset(&J->Flags,mjfRsvMap);

  if (bmisset(&RFlags,mrfEnforceNodeSet))
    {
    J->Req[0]->SetBlock = TRUE;
    }
  else
    {
    J->Req[0]->SetBlock = FALSE;
    }

  if (ModifyRsv == FALSE)
    {
    /* why are these not set for modify? */

    if ((R != NULL) && bmisset(&R->Flags,mrfExcludeAll))
      {
      bmset(&J->IFlags,mjifRsvMapIsExclusive);
      }
    else if ((SR != NULL) && (bmisset(&SR->Flags,mrfExcludeAll)))
      {
      bmset(&J->IFlags,mjifRsvMapIsExclusive);
      }
    }

  if (!bmisset(&RFlags,mrfExcludeAll))
    bmset(&J->IFlags,mjifSharedResource);

  /*
  if (bmisset(&RFlags,mrfByName))
    bmset(&J->SpecFlags,mjfByName);
  */

  if ((RHostExpIsSpecified == TRUE) && (SR == NULL))
    bmset(&J->SpecFlags,mjfCoAlloc);

  if (!strcmp(RPName,"ALL"))
    bmset(&J->SpecFlags,mjfCoAlloc);

  if ((RHostExp != NULL) && (RHostExp[0] != '\0'))
    {
    /* verify hostlist */

    if (MNLIsEmpty(RHostList))
      {
      msrsv_t *tmpSR = NULL;

      if (SRIndex >= 0)
        {
        tmpSR = (msrsv_t *)MUArrayListGet(SRsvArray,SRIndex);

        MDB(3,fSCHED) MLog("INFO:     rsv %s hostlist %s\n",
          RName,
          (MSched.Iteration == 0) ? "initialized" : "lost");

        /* preserve allocated memory */

        if ((SR != NULL) && (tmpSR != NULL) && (SR != tmpSR))
          {
          if ((MNLIsEmpty(&tmpSR->HostList)) && (!MNLIsEmpty(&SR->HostList)))
            {
            MNLCopy(&tmpSR->HostList,&SR->HostList);
            }
          }
        }
 
      /* NOTE: for EXCLUSIVE reservations RTC=0, MRsvCreateNL() will return all possible hosts, 
               and the actual hosts for the reservation will be determined by MJobGetEStartTime().
               for non-exclusive reservations RTC=<RTC> and MRsvCreateNL() will only return <RTC>
               number of hosts to use for the reservation */

      if (MRsvCreateNL(
           RHostList,
           RHostExp,
           (bmisset(&RFlags,mrfExcludeAll)) ? 0 : RTC,
           RTPN,
           RPName,  /* I */
           RDRes,
           EMsg) == SUCCESS)
        {
        /* associate hostlist with job hostlist constraint */

        if ((SR != NULL) && (SRIndex >= 0))
          MNLCopy(&tmpSR->HostList,RHostList);
        }    /* END if (MRsvCreateNL() == SUCCESS) */
      else
        {
        /* will populate EMsg */

        MDB(3,fSCHED) MLog("ALERT:    cannot build hostlist for rsv %s in %s\n",
          RName,
          FName);

        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          strcpy(EMsg,"cannot build hostlist for reservation\n");

        ReturnCode = FAILURE;

        goto free_and_return;
        }  /* END else (MRsvCreateNL() == SUCCESS) */
      }    /* END if (HL[0].N == NULL) */
    }      /* END if ((RHostExp != NULL) && ...) */

  bmclear(&ModeBM);

  /* use best effort for all template rsvs */
 
  if (RTC >= 0)
    {
    if ((RsvIsAdmin == FALSE) && !bmisset(&RFlags,mrfReqFull))
      {
      bmset(&J->SpecFlags,mjfBestEffort);
      bmset(&ModeBM,mstBestEffort);
      }
    }

  /* initialize requirements */

  if (!bmisclear(&RFBM))
    {
    bmcopy(&RQ->ReqFBM,&RFBM);

    if (RFeatureMode != 0)
      {
      RQ->ReqFMode = RFeatureMode;
      }
    }

  /* NOTE:  only enforced with task specification */

  RQ->Arch    = ReqArch;
  RQ->Opsys   = ReqOS;
  RQ->ReqNR[mrMem]  = ReqMem;
  RQ->ReqNRC[mrMem] = MDEF_RESCMP;

  RQ->RMIndex = -1;

  TotalTC = 0;
  TotalNC = 0;

  if (DoCheckLimits == TRUE)
    {
    /* if rsv is grid or personal and sandbox is created */

    if (bmisset(&RFlags,mrfPRsv) && (MSched.PRsvSandboxRID != NULL))
      {
      /* request is personal rsv */

      bmset(&J->SpecFlags,mjfAdvRsv);

      J->ReqRID = MSched.PRsvSandboxRID;
      }
    }  /* END if (DoCheckLimits == TRUE) */
 
  if (!MNLIsEmpty(RHostList))
    {
    mnode_t *N;

    bmset(&J->IFlags,mjifHostList);

    if (RTC == 0)
      bmset(&J->SpecFlags,mjfBestEffort);

    /* If hostlist, taskcount, and -E are specified then set mode to superset. 
     * Scenario: 4 nodes with jobs on nodes 2 and 4 and create a reservation
     * with -h node[1-4] -t 2 -E. When looking at nodes 2 and 4 for start
     * ranges the job gets an affinity to the nodes and MRLAnd is called
     * which makes the ranges so that 1 node is available now and 4 are
     * available later where it should be two availabe now and 4 available later. */

    if ((RTC > 0) && bmisset(&RFlags,mrfExcludeAll))
      J->ReqHLMode = mhlmSuperset;

    if (ReqHLMode != mhlmNONE)
      J->ReqHLMode = ReqHLMode;

    if ((R != NULL) && (R->AllocTC < R->ReqTC))
      {
      /* allow access to allocated resources */

      /* NOTE:  temp job, do not allocate */

      for (index = 0;index < MMAX_PJOB;index++)
        {
        if (J->RsvAList[index] == NULL)
          {
          MUStrDup(&J->RsvAList[index],R->Name);

          break;
          }
        }

      /* allow allocation of additional resources */

      if (EnforceHostExp == FALSE)
        J->ReqHLMode = mhlmSubset;
      else if (ModifyRsv == TRUE)
        J->ReqHLMode = mhlmSuperset;

      if (!strncasecmp(R->HostExp,"class:",strlen("class:")))
        J->ReqHLMode = mhlmSuperset;
      }
    else if ((SR != NULL) && (RTC > 0))
      {
      if (!strncasecmp(RHostExp,"class:",strlen("class:")))
        J->ReqHLMode = mhlmSuperset;
      }

    /* FIXME (Dangerous) - The sizeof() should match the thing being copied */
    
    MNLCopy(&J->ReqHList,RHostList);

    for (nindex = 0;MNLGetNodeAtIndex(RHostList,nindex,&N) == SUCCESS;nindex++)
      {
      if (!bmisset(&RFlags,mrfIgnState) && !MNODEISUP(N))
        continue; 

      TC = MNodeGetTC(N,&N->CRes,&N->CRes,&N->DRes,RDRes,MMAX_TIME,1,NULL);

      TotalTC += TC;
      TotalNC++;
  
      MDB(5,fUI) MLog("INFO:     evaluating node %sx%d of hostlist for rsv %s (%d)\n",
        N->Name,
        TC,
        RName,
        TotalTC);

      if ((RTC != 0) && (TotalTC >= RTC))
        break;
      }  /* END for (nindex) */
    }    /* END if ((RHostList != NULL) && (RHostList[0].N != NULL)) */
  else
    {
    /* NOTE: new else statement added to make sure standing reservations requesting
             taskcounts and REQFULL don't get created if there aren't enough resources
             SRCFG[] TASKCOUNT=3 FLAGS=REQFULL was being created with 2 tasks */

    TotalTC = RTC;
    }

  if (RHostListIsLocal == TRUE)
    MNLFree(RHostList);

  if (RTC > 0)
    {
    if ((RTC != TotalTC) && (RsvIsAdmin == FALSE))
      bmset(&ModeBM,mstBestEffort);

    TotalTC = RTC;
    }
  else
    {
    if (TotalTC < 1)
      {
      MDB(3,fUI) MLog("WARNING:  empty hostlist in %s()\n",
        FName);

      if (RecycleUID == TRUE)
        MRsvRecycleUID(RName);

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"invalid resource request");

      ReturnCode = FAILURE;

      goto free_and_return;
      }
    }    /* END else (RTC > 0) */

  if ((RHostExp != NULL) && (RHostExp[0] != '\0'))
    {
    /* if rsv is not dedicated grant global job access */

    if (!bmisset(&RFlags,mrfExcludeAll))
      {
      /* Add to end of list */

      /* NOTE:  temp job, do not allocate */
      
      bmset(&ModeBM,mstForce); /* force rsv onto requested node regardless of state or other constraints */

      for (index = 0;index < MMAX_PJOB;index++)
        {
        if (J->RsvAList[index] == NULL)
          {
          MUStrDup(&J->RsvAList[index],AllJob);

          break;
          }
        }
      }
    }
  else
    {
    /* if no host expression set */

    /* how can a user specify no host expression and no taskcount? */

    if (!bmisset(&RFlags,mrfExcludeAll) || (RTC == 0))
      {
      bmset(&ModeBM,mstForce);
      }
    }

  /* set credentials */

  /* NOTE:  location sets previous updated job flags */

  if (MJobSetCreds(J,ALL,ALL,ALL,0,NULL) == FAILURE)
    {
    MDB(3,fUI) MLog("INFO:     cannot setup rsv job creds\n");

    if (RecycleUID == TRUE)
      MRsvRecycleUID(RName);

    if (EMsg != NULL)
      strcpy(EMsg,"cannot initialize reservation");

    ReturnCode = FAILURE;

    goto free_and_return;
    }

  /* MJobSetQOS(J,MSched.DefaultQ,0); */

  if (RTPN > 0)
    {
    RQ->TasksPerNode  = RTPN;
    bmset(&J->IFlags,mjifExactTPN);
    }

  J->WCLimit        = MIN(RDuration,MCONST_EFFINF);
  J->SpecWCLimit[0] = J->WCLimit;

  J->Request.TC     = TotalTC;
  RQ->TaskCount     = TotalTC;

  J->Credential.Q = RAQ;

  /* allow override of the default "physical machines" */

  if (RVMUsagePolicy != mvmupNONE)
    J->VMUsagePolicy = RVMUsagePolicy;

  if (J->Request.NC == 0)
    {
    J->Request.NC = TotalNC;
    RQ->NodeCount = TotalNC;
    }

  BestTime = MAX(MSched.Time,RStartTime);

  if (!MACLIsEmpty(RACL))
    {
    MACLCopy(&J->Credential.CL,RACL);

    if (!MACLIsEmpty(RCL))
      {
      MACLMerge(&J->Credential.CL,RCL);
      }
    }    /* END if (!MACLIsEmpty(RACL)) */
  else
    {
    MACLFreeList(&J->Credential.CL);
    }

  if (bmisset(&J->IFlags,mjifHostList) && 
     (!bmisclear(&J->Req[0]->ReqFBM)))
    {
    J->ReqHLMode = mhlmSuperset;
    }

  /* NOTE:  does not support PSlot only rsvs */

  if ((RQ->TaskCount == 0) ||
      ((RDRes->Procs == 0) && (MSNLIsEmpty(&RDRes->GenericRes)) && (RDRes->Mem == 0) && (RDRes->Disk == 0) && (RDRes->Swap == 0)))
    {
    MDB(2,fSCHED) MLog("ALERT:    no resources requested by rsv (%d)\n",
      RQ->TaskCount);

    if ((SR != NULL) && (RO != NULL))
      {
      MRsvDestroy(ROP,TRUE,TRUE);
      }

    if (RecycleUID == TRUE)
      MRsvRecycleUID(RName);

    if (EMsg != NULL)
      strcpy(EMsg,"cannot locate resource match");

    ReturnCode = FAILURE;

    goto free_and_return;
    }

  MULToTString(RDuration,TString);

  strcpy(TempString,TString);
  
  MJobUpdateResourceCache(J,NULL,0,NULL);

  PC = J->TotalProcCount;

  MULToTString(RStartTime - MSched.Time,TString);

  MDB(3,fSCHED) MLog("INFO:     attempting reservation of %d procs in %s for %s\n",
    PC,
    TString,
    TempString);

  /* rsv job creation complete */

  aindex = 0;

  if (RO != NULL)
    {
    mnode_t *N;

    mre_t   *RE;

    CurrentIdlePS   = RO->CIPS;
    CurrentActivePS = RO->CAPS;
    TotalIdlePS     = RO->TIPS;
    TotalActivePS   = RO->TAPS;

    /* get current active reservation list */

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;

      for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
        {
        if (RE->Type != mreStart)
          continue;

        if (MREGetRsv(RE) == RO)
          {
          if ((N->State == mnsBusy) || (N->State == mnsActive))
            {
            /* add node to list */

            MNLSetNodeAtIndex(&tmpANodeList,aindex,N);
            MNLSetTCAtIndex(&tmpANodeList,aindex,1);

            aindex++;
            }
          }
        }    /* END for (MREGetNext) */
      }      /* END for (nindex) */
    }        /* END if (RO != NULL) */
  else
    {
    CurrentIdlePS   = 0.0;
    CurrentActivePS = 0.0;
    TotalIdlePS     = 0.0;
    TotalActivePS   = 0.0;
    }        /* END else (RO != NULL) */

  /* initialize RQ structures for scheduling the reservation */

  MCResCopy(&RQ->DRes,RDRes);

  /* determine rsv nodelist */

  if ((bmisset(&RFlags,mrfExcludeAll) == TRUE) ||
      (ModifyRsv == TRUE))
    {
    mnl_t  *tmpMNL[MMAX_REQ_PER_JOB];
    int         TC;

    long      tmpTime;

    tmpTime = BestTime;

    MNLMultiInit(tmpMNL);

    /* we do this because with ACLOverlap we need to make sure we include nodes
       that are running jobs that match even the HardPolicyEnabled ACL.  To do that
       we turn on the policy here and turn it off immediately after the call.  We
       might need to do this in more places think of another solution but this should 
       fix NETSUITE 10063 */

    InHardPolicySchedulingCycle = TRUE;

    /* NOTE:  tmpMNL[] will contain target nodes but may contain maximum 
              possible per-node taskcount */

    if (MJobGetEStartTime(
          J,                 /* I */
          &P,
          &NodeCount,        /* O */
          &TC,               /* O */
          tmpMNL,            /* O */
          &tmpTime,
          NULL,
          TRUE,
          FALSE,
          EMsg) == FAILURE)  /* I/O */
      {
      InHardPolicySchedulingCycle = TRUE;
 
      MDB(1,fSCHED) MLog("ALERT:    cannot select %d procs in partition '%s' for rsv '%s'\n",
        PC,
        (RPName[0] != '\0') ? RPName : ALL,
        RName);

      if (RecycleUID == TRUE)
        MRsvRecycleUID(RName); 

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"cannot select resources for reservation");

      MNLMultiFree(tmpMNL);

      ReturnCode = FAILURE;

      goto free_and_return;
      }
   
    InHardPolicySchedulingCycle = TRUE;

    TCIsMax = TRUE;
 
    if (tmpTime > (long)BestTime)
      {
      MULToTString(tmpTime - MSched.Time,TString);

      MDB(1,fSCHED) MLog("ALERT:    requested resources (%d procs) not available at time %s in partition '%s' for rsv '%s'\n",
        PC,
        TString,
        (RPName[0] != '\0') ? RPName : ALL,
        RName);

      if (RecycleUID == TRUE)
        MRsvRecycleUID(RName); 

      if (EMsg != NULL)
        strcpy(EMsg,"resources unavailable at requested time");

      MNLMultiFree(tmpMNL);

      ReturnCode = FAILURE;

      goto free_and_return;
      }

    if (bmisset(&RFlags,mrfReqFull) &&
        (((J->Request.TC > 0) && (TC < J->Request.TC)) ||
         ((J->Request.NC > 0) && (NodeCount < J->Request.NC))))
      {
      MULToTString(tmpTime - MSched.Time,TString);

      MDB(1,fSCHED) MLog("ALERT:    requested resources (%d procs) not available at time %s in partition '%s' for rsv '%s'\n",
        PC,
        TString,
        (RPName[0] != '\0') ? RPName : ALL,
        RName);

      if (RecycleUID == TRUE)
        MRsvRecycleUID(RName); 

      if (EMsg != NULL)
        strcpy(EMsg,"resources unavailable at requested time");

      MNLMultiFree(tmpMNL);

      ReturnCode = FAILURE;

      goto free_and_return;
      }
    else if (((J->Request.TC > 0) && (TC < J->Request.TC)) ||
             ((J->Request.NC > 0) && (NodeCount < J->Request.NC)))
      {
      /* TODO: notify user that reservation does not contain all requested resources */
      }

    /* FIXME:  call MJobDistributeTasks() */

    if ((J->Request.TC > 0) && (J->Request.NC == 0))
      {
      mnode_t *tmpN;
      int      tmpTC;

      int TC = J->Request.TC;
   
      for (nindex = 0;MNLGetNodeAtIndex(tmpMNL[0],nindex,&tmpN) == SUCCESS;nindex++)
        {
        tmpTC = MNLGetTCAtIndex(tmpMNL[0],nindex);

        if (tmpTC == 0)
          break;
   
        MNLSetNodeAtIndex(&NodeList,nindex,tmpN);
   
        if (tmpTC > TC)
          MNLSetTCAtIndex(&NodeList,nindex,TC);
        else
          MNLSetTCAtIndex(&NodeList,nindex,tmpTC);
   
        TC -= MNLGetTCAtIndex(&NodeList,nindex);
        }

      MNLTerminateAtIndex(&NodeList,nindex);
      }  /* END if ((J->Request.TC > 0) && (J->Request.NC == 0)) */
    else
      {
      /* copy MNL to NL */

      MNLCopy(&NodeList,tmpMNL[0]);
      }
   
    MNLMultiFree(tmpMNL);

    /*
    if (RHostList != NULL)
      MNLCopy(RHostList,tmpMNL[0]);
    */
    }  /* END if (bmisset(&RFlags,mrfExcludeAll) == TRUE) */
  else if ((R != NULL) && (!MNLIsEmpty(&R->ReqNL)))
    {
    /* reservation specifies explicit required nodelist */

    if (((ReqArch == 0) && (ReqOS == 0) && (ReqMem == 0) && (bmisclear(&RFBM))) ||
        (bmisset(&R->IFlags,mrifTIDRsv)))
      {
      /* NOTE: if this is a TID reservation then all requested features have already been satisfied */

      MNLCopy(&NodeList,&R->ReqNL);
      }
    else
      {
      int nindex;
      int nindex2;

      nindex2 = 0;

      for (nindex = 0;MNLGetNodeAtIndex(&R->ReqNL,nindex,NULL) == SUCCESS;nindex++)
        {
        enum MAllocRejEnum RejectReason;
        int rc;
        mnode_t *N;

        MNLGetNodeAtIndex(&R->ReqNL,nindex,&N);

        rc = MReqCheckResourceMatch(
              NULL,
              J->Req[0],
              N,
              &RejectReason,
              RStartTime,
              FALSE,
              NULL,
              NULL,
              NULL);

        if (rc == FAILURE)
          {
          if ((RejectReason == marOpsys) && (MNodeIsValidOS(N,ReqOS,NULL) == TRUE))
            {
            rc = SUCCESS;
            }
          }

        if (rc == SUCCESS)
          {
          MNLSetNodeAtIndex(&NodeList,nindex2,N);
          MNLSetTCAtIndex(&NodeList,nindex2,MNLGetTCAtIndex(&R->ReqNL,nindex));

          nindex2++;

          MNLTerminateAtIndex(&NodeList,nindex2);
          }
        }    /* END for (nindex) */
      }      /* END else */

    NodeCount = 0;
    }   /* END else if ((R != NULL) && (!MNLIsEmpty(&R->ReqNL))) */
  else if (MRsvSelectNL(
        J,              /* I */
        RName,
        RPName,         /* I */
        &RFlags,         /* I */
        RTC,            /* I */
        &NodeList,      /* O */
        &NodeCount,     /* O */
        BestTime,       /* I */
        &tmpANodeList,  /* I (required node list) */
        &ModeBM,        /* I (bitmap of enum mst*) */
        MsgBuf) == FAILURE)
    {
    MDB(1,fSCHED) MLog("ALERT:    cannot select %d procs in partition '%s' for rsv '%s'\n",
      PC,
      (RPName[0] != '\0') ? RPName : ALL,
      RName);

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot select %d procs in partition '%s' for rsv - %s",
        PC,
        (RPName[0] != '\0') ? RPName : ALL,
        MsgBuf);
      }

    /* why recycle here? - should only recycle Rsv ID after rsv creation below */

    if (RecycleUID == TRUE)
      MRsvRecycleUID(RName); 

    ReturnCode = FAILURE;

    goto free_and_return;
    }

  /* NodeList is populated */

  BestTime = MAX(BestTime,MSched.Time);

  bmcopy(&Flags,&RFlags);

  IsSpaceFlex = bmisset(&RFlags,mrfSpaceFlex);

  if (SR != NULL)
    {
    /* NOTE:  previously *RAAP was set to NULL but this clobbered SR->A or R->A externally */

    /* NOTE:  we must determine why *RAAP=NULL was specified - to prevent double billing somewhere? */

    /* *RAAP = NULL; */

    if ((ORName[0] != '\0') && (RecycleUID == TRUE))
      MRsvRecycleUID(RName);

    strcpy(RName,ORName);

    MRsvDestroy(ROP,TRUE,TRUE);
    }  /* END if (SR != NULL) */

  ProcCount = 0;

  if ((*RAAP != NULL) &&
       strcmp((*RAAP)->Name,NONE))
    {
    mnode_t *tmpN;

    /* if accountable rsv is specified, determine procs to bill */

    for (nindex = 0;MNLGetNodeAtIndex(&NodeList,nindex,&tmpN) == SUCCESS;nindex++)
      {
      if (RQ->DRes.Procs > 0)
        ProcCount += RQ->DRes.Procs * MNLGetTCAtIndex(&NodeList,nindex);
      else
        ProcCount += tmpN->CRes.Procs;
      }
    }

  REndTime = MIN(MMAX_TIME,(unsigned long)BestTime + RDuration);

  if (DoCheckLimits == TRUE)
    {
    mbitmap_t BM;

    mrsv_t tmpR;

    mulong EStartTime = RStartTime;

    long   tmpL;

    char   tmpLine[MMAX_LINE];

    int rc;

    memset(&tmpR,0,sizeof(tmpR));

    tmpR.AllocPC   = PC;
    tmpR.AllocNC   = TotalTC;
    tmpR.StartTime = RStartTime;
    tmpR.EndTime   = REndTime;

    MCResInit(&tmpR.DRes);

    tmpR.U = RAU;
    tmpR.Q = RAQ;
    tmpR.G = RAG;
    tmpR.A = *RAAP;

    tmpR.Name[0] = '\0';

    if (!MACLIsEmpty(RCL))
      MACLCopy(&tmpR.CL,RCL);

    bmset(&BM,mlSystem);

    if (MOCheckLimits(
          (void *)&tmpR,
          mxoRsv,
          mptHard,
          NULL,
          &BM,
          EMsg) == FAILURE)
      {
      MDB(3,fUI) MLog("WARNING:  cannot create requested rsv, policy violation (%s)\n",
        (EMsg != NULL) ? EMsg : "N/A");

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"cannot create reservation - policy violation");

      ReturnCode = FAILURE;

      MACLFreeList(&tmpR.CL);
      MCResFree(&tmpR.DRes);

      goto free_and_return;
      }

    tmpL = (long)EStartTime;

    if (MSched.EnableHighThroughput != TRUE)
      { 
      rc = MPolicyGetEStartTime(
        (void *)&tmpR,
        mxoRsv,
        &MPar[0],
        mptHard,
        &tmpL,     /* I/O */
        tmpLine);
      }
    else
      {
      rc = SUCCESS;
      }

    EStartTime = (mulong)tmpL;

    MACLFreeList(&tmpR.CL);
    MCResFree(&tmpR.DRes);

    if ((rc == FAILURE) || 
       ((EStartTime > MSched.Time) && (EStartTime != tmpR.StartTime)))
      {
      if (EStartTime != tmpR.StartTime)
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"available start time (%ld) > requested start time (%ld)%s%s\n",
            EStartTime,
            tmpR.StartTime,
            (tmpLine[0] == '\0') ? "" : " - ",
            (tmpLine[0] == '\0') ? "" : tmpLine);
          }
        }

      MDB(3,fUI) MLog("WARNING:  cannot create requested rsv at time %lu (%s)\n",
        tmpR.StartTime,
        (EMsg != NULL) ? EMsg : "NULL");

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"cannot select resources at requested time");

      ReturnCode = FAILURE;

      goto free_and_return;
      }
    }   /* END if (DoCheckLimits == TRUE) */

  /* create/modify rsv */

  if (ModifyRsv == TRUE)
    {
    /* modify existing rsv */

    mnode_t *N1;
    mnode_t *N2;

    int      TC1;

    int onindex;
    int nnindex;

    mnl_t *ONL;
    mnl_t *NNL;

    int ONLCount;

    /* R is real - not template, modify R with new hostlist */

    ONL = &R->NL;
    NNL = &NodeList;

    /* NOTE:  routine only called to add nodes to existing nodelist */

    if (ONL == NULL)
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"no nodelist on reservation\n");

      MDB(3,fUI) MLog("WARNING:  rsv '%s' has empty nodelist\n",
        R->Name);

      ReturnCode = FAILURE;

      goto free_and_return;
      }

    for (onindex = 0;MNLGetNodeAtIndex(ONL,onindex,NULL) == SUCCESS;onindex++);

    ONLCount = onindex;

    /* walk nodelist and set changes */

    /* for every node in the new node index, check to see if we need to add the node
       to the reservation and make sure it is in the reservations nodelist */

    for (nnindex = 0;MNLGetNodeAtIndex(NNL,nnindex,&N1) == SUCCESS;nnindex++)
      {
      for (onindex = 0;MNLGetNodeAtIndex(ONL,onindex,&N2) == SUCCESS;onindex++)
        {
        if (N1 == N2)
          {
          if (MNLGetTCAtIndex(NNL,nnindex) != MNLGetTCAtIndex(ONL,onindex))
            {
            TC1 = MNLGetTCAtIndex(NNL,nnindex);

            if ((R->ReqTC < TC1) && (TCIsMax == TRUE))
              {
              /* ignore reported available TC info since it is max possible,
                 not the amount which should be allocated */

              /* NOTE:  handle situation w/ R->DRes=1:procs=2, 1 task allocated
                        on node w/procs=5.  When rsv walltime is modified this 
                        routine should NOT grow allocated resources to 2. */

              /* the 'if' statement above should be much more aggressive but
                 is in place to just handle the specific situation */

              /* NO-OP */
              }
            else
              {
              /* adjust node taskcount */

              MRsvAdjustNode(R,N1,TC1,0,TRUE);

              MNLSetTCAtIndex(ONL,onindex,TC1);
              }
            }

          break;
          }
        }  /* END for (onindex) */

      /* NOTE:  We don't need to add more nodes if the reservation now has
       *        allocated all of the tasks it requested. */

      if ((MNLGetNodeAtIndex(ONL,onindex,NULL) == FAILURE) && (R->AllocTC < R->ReqTC))
        {
        TC1 = MNLGetTCAtIndex(NNL,nnindex);

        /* add node */

        MRsvAdjustNode(R,N1,TC1,1,TRUE);
        /* ONL is limited to R->NLSize. */

        MNLSetNodeAtIndex(ONL,ONLCount,N1);
        MNLSetTCAtIndex(ONL,ONLCount,TC1);
        ONLCount++;
        }
      }    /* END for (nnindex) */

    MNLTerminateAtIndex(ONL,nnindex);
    } 
  /* create new rsv */

  else if (MRsvCreate(
        mrtUser,
        RACL,
        RCL,
        &Flags,
        &NodeList,          /* I */
        BestTime,
        (long)REndTime,
        RNC,               /* I */
        RTC,               /* I */
        J->Name,
        ROP,               /* O */
        RHostExp,
        RDRes,
        EMsg,
        TRUE,
        TRUE) == FAILURE)
    {
    MDB(1,fSCHED) MLog("ALERT:    cannot reserve %d procs in partition '%s' for SR\n",
      RTC * RDRes->Procs,
      (RPName[0] != '\0') ? RPName : ANY);

    /* charge for previous segment */

    if (IsSpaceFlex == TRUE)
      {
      /* This will handle final charge and removal of reservation */
      if ((RO->A != NULL) && (MAMHandleEnd(&MAM[0],(void *)RO,mxoRsv,NULL,NULL) == FAILURE))
        {
        MDB(1,fSCHED) MLog("WARNING:  Unable to register reservation end with accounting manager for %6.2f PS for reservation %s\n",
          RO->CIPS,
          RO->Name);
        }

      /* Update reservation cycle tallies */
      RO->TIPS += RO->CIPS;
      RO->TAPS += RO->CAPS;
      RO->CIPS = 0.0;
      RO->CAPS = 0.0;
      }      /* END if (IsSpaceFlex == TRUE) */ 

    if (RecycleUID == TRUE)
      MRsvRecycleUID(RName);

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      strcpy(EMsg,"rsv creation failed");
      }

    ReturnCode = FAILURE;

    goto free_and_return;
    }  /* END if (MRsvCreate() == FAILURE) */

  if (ROP != NULL)
    RO = *ROP;

  /* add rsv attributes */

  RO->CIPS       = CurrentIdlePS;
  RO->CAPS       = CurrentActivePS;
  RO->TIPS       = TotalIdlePS;
  RO->TAPS       = TotalActivePS;

  RO->Priority   = RPriority;
  RO->ExpireTime = RExpireTime;

  RO->O          = ROwner;
  RO->OType      = ROType;
  RO->ReqTC      = RTC;
  RO->SubType    = RType;
  RO->ReqOS      = RReqOS;
  RO->ReqArch    = ReqArch;
  RO->ReqMemory  = ReqMem;
  RO->LogLevel   = LogLevel;

  RO->SpecName   = RSpecName;

  RO->VMUsagePolicy = RVMUsagePolicy;

  if ((RO->RsvExcludeList == NULL) && (RsvExcludeList != NULL))
    {
    RO->RsvExcludeList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

    for (index = 0;index < MMAX_PJOB;index++)
      {
      if (RsvExcludeList[index] == NULL)
        break;

      MUStrDup(&RO->RsvExcludeList[index],RsvExcludeList[index]);
      }
    }

  if (bmisset(&RO->Flags,mrfProvision))
    bmset(&RO->Flags,mrfSystemJob);

  if (RMB != NULL)
    MMBCopyAll(&RO->MB,RMB);

  if (RLabel != NULL)
    MUStrDup(&RO->Label,RLabel);

  RO->HostExpIsSpecified = RHostExpIsSpecified;

  if (R != NULL)
    {
    R->Comment = NULL;
    R->SystemRID = NULL;
    R->SystemID = NULL;
    R->Variables = NULL;
    }
 
  RO->Variables = RVariables;

  RO->AllocPolicy = RAllocPolicy;

  MMovePtr(&RComment,&RO->Comment);
  MMovePtr(&RSysJobTemplate,&RO->SysJobTemplate);
  MMovePtr(&GRID,&RO->SystemRID);
  MMovePtr(&SID,&RO->SystemID);

  MMovePtr(&J->MasterJobName,&RO->RsvGroup);

  if (SR != NULL)
    {
    if (SR->SpecRsvGroup != NULL)
      MUStrDup(&RO->RsvGroup,SR->SpecRsvGroup);
    else
      MUStrDup(&RO->RsvGroup,SRGName);

    MUStrDup(&RO->RsvParent,SRGName);

    RO->MaxJob = SR->MaxJob;

    if (SR->U != NULL)
      RO->U = SR->U;

    /* Update rsv with how many jobs are running in it. Only do for 
     * reservations that are child's of standing reservations */

    MRsvSetJobCount(RO);
    }

  if (!bmisclear(&RFlags))
    {
    MRsvSetAttr(RO,mraFlags,(void *)&RFlags,mdfOther,mIncr);
    }

  if (!bmisclear(&RFBM))
    MRsvSetAttr(RO,mraReqFeatureList,(void *)&RFBM,mdfOther,mSet);

  if ((SR == NULL) && (ModifyRsv == FALSE))
    {
    /* NOTE:  RAAP handled directly by MRsvCreate() */

    if (RType == mrsvstNONE)
      RO->SubType = mrsvstAdmin_Other;

    RO->Q = RAQ;
    RO->U = RAU;
    RO->G = RAG;

    RO->IsTracked = RIsTracked;

    if (ORName[0] != '\0')
      {
      if (ORIsActive == TRUE)
        bmset(&RO->Flags,mrfIsActive);
      }
    }
  else if (RType == mrsvstNONE)
    {
    RO->SubType = mrsvstStandingRsv;
    }
  /*
  MRsvCheckAdaptiveState(RO,EMsg);
  */

  if ((RTList != NULL) &&
      (ModifyRsv == FALSE) &&
      (SRTList == NULL))
    {
    int tindex;

    if (RTList != NULL)
      MTListCopy(RTList,&RO->T,TRUE);

    if (ROS != NULL)
      {
      marray_t JArray;
      int jindex;
      int nindex;
      int nindex2 = 0;

      mjob_t **JList;
   
      mnode_t *tmpN;

      mnl_t tmpNL;

      MNLInit(&tmpNL);

      for (nindex = 0;MNLGetNodeAtIndex(&NodeList,nindex,&tmpN) == SUCCESS;nindex++)
        {
        if (tmpN->ActiveOS != ReqOS)
          {
          MNLSetNodeAtIndex(&tmpNL,nindex2,tmpN);

          MNLSetTCAtIndex(&tmpNL,nindex2,MNLGetTCAtIndex(&NodeList,nindex));
          }
        }

      if (!MNLIsEmpty(&tmpNL))
        {
        MUArrayListCreate(&JArray,sizeof(mjob_t *),4);

        if (MUGenerateSystemJobs(
              NULL,
              NULL,
              &tmpNL,
              msjtOSProvision,
              ROS,
              ReqOS,
              NULL,
              NULL,
              FALSE,
              (MSched.AggregateNodeActions == TRUE) ? FALSE : TRUE,
              FALSE, /* force execution */
              &JArray) == FAILURE)
          {
          /* NYI: append EMsg to Msg */

          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"ERROR:    Cannot modify OS for reservation %s to %s: system job creation failed\n",
              R->Name,
              ROS);
            }

          MNLFree(&tmpNL);

          MUArrayListFree(&JArray);

          ReturnCode = FAILURE;

          goto free_and_return;
          }

        JList = (mjob_t **)JArray.Array;

        for (jindex = 0;jindex < JArray.NumItems;jindex++)
          {
          MJobSetAttr(JList[jindex],mjaRsvAccess,(void **)RO->Name,mdfString,mAdd);
          }

        MUArrayListFree(&JArray);
        }

      MNLFree(&tmpNL);
      }  /* END if (ROS != NULL) */

    /* link new triggers to global table */

    if ((SR != NULL) && (RO->T != NULL))
      {
      mtrig_t *T;

      for (tindex = 0; tindex < RO->T->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(RO->T,tindex);

        if (MTrigIsValid(T) == FALSE)
          continue;

        T->OType = mxoRsv;

        MUStrDup(&T->OID,RO->Name);

        MTrigInitialize(T);
        } /* END for (tindex) */

      /* report anticipated events for SR */

      MOReportEvent((void *)RO,RO->Name,mxoRsv,mttCreate,RO->CTime,TRUE);
      MOReportEvent((void *)RO,RO->Name,mxoRsv,mttStart,RO->StartTime,TRUE);
      MOReportEvent((void *)RO,RO->Name,mxoRsv,mttEnd,RO->EndTime,TRUE);
      }   /* END if (SR != NULL) */
    }     /* END if ((RTList != NULL) || (ROS != NULL)) */
  else if (((RTList != NULL) || (ROS != NULL)) &&
           (ModifyRsv == FALSE) &&
           (SRTList != NULL))
    {
    mtrig_t *T;

    int tindex;

    RO->T = SRTList; 

    for (tindex = 0; tindex < RO->T->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(RO->T,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      T->OType = mxoRsv;

      MUStrDup(&T->OID,RO->Name);

      T->O = (void *)RO;
      } /* END for (tindex) */
    }

  if ((*RAAP != NULL) && strcmp((*RAAP)->Name,NONE))
    {
    RO->A = *RAAP;
    }

  PC = J->TotalProcCount;

  if (PC == RO->AllocPC)
    {
    char DString[MMAX_LINE];

    MULToTString(BestTime - MSched.Time,TString);
    MULToDString((mulong *)&BestTime,DString);

    MDB(2,fSCHED) MLog("INFO:     full rsv reserved %d procs in partition '%s' to start in %s at (%ld) %s",
      PC,
      (RPName[0] != '\0') ? RPName : "[ALL]",
      TString,
      BestTime,
      DString);
    }
  else
    {
    char DString[MMAX_LINE];

    MULToTString(BestTime - MSched.Time,TString);
    MULToDString((mulong *)&BestTime,DString);

    MDB(2,fSCHED) MLog("WARNING:  partial rsv %s reserved %d of %d procs in partition '%s' to start in %s at (%ld) %s",
      RName,
      RO->AllocPC,
      PC,
      (RPName[0] != '\0') ? RPName : "[ALL]",
      TString,
      BestTime,
      DString);
    }

  MRsvCreateCredLock(RO);

free_and_return:

  bmclear(&Flags);
  bmclear(&RFlags);

  MUFree(&J->MasterJobName);

  MNLFree(&NodeList);
  MNLFree(&tmpANodeList);

  MJobFreeTemp(&J);

  return(ReturnCode);
  }  /* END MRsvConfigure() */


/**
 * Refresh/update existing standing reservations.
 *
 * NOTE: called once per period and when new resources located (not called per iteration).
 *
 * @param SSR  (I) [optional]
 * @param SRsv (I) [optional]
 */

int MSRPeriodRefresh(

  msrsv_t  *SSR,
  marray_t *SRsv)

  {
  int      index;
  int      SRIndex;
  msrsv_t *SR;

  marray_t *SRP;

  int      DIndex;

  mbool_t  WasActive;

  char     EMsg[MMAX_LINE];

  const char *FName = "MSRPeriodRefresh";

  MDB(4,fCORE) MLog("%s()\n",
    FName);

  if (SRsv != NULL)
    {
    SRP = SRsv;
    }
  else
    {
    SRP = &MSRsv;
    }

  for (SRIndex = 0;SRIndex < SRP->NumItems;SRIndex++)
    {
    SR = (msrsv_t *)MUArrayListGet(SRP,SRIndex);

    if (SR->Name[0] == '\0')
      break;

    if (SR->Name[0] == '\1')
      continue;

    if ((SSR != NULL) && (SSR != SR))
      continue;

    for (index = 0;index < MMAX_SRSV_DEPTH;index++)
      {
      if (SR->DisabledTimes[index] == 0)
        break;

      if (SR->DisabledTimes[index] < MSched.Time)
        {
        /* clear SR disabled time */

        SR->DisabledTimes[index] = 1;
        }
      }

    index = 1;

    while (index < MMAX_SRSV_DEPTH)
      {
      /* FIXME:  may have incorrect flags from Checkpoint file
                 overwrite with correct flags */

      if (SR->R[index] == NULL)
        {
        index++;

        continue;
        }

      if (bmisset(&SR->R[index]->Flags,mrfWasActive))
        WasActive = TRUE;
      else
        WasActive = FALSE;

      bmcopy(&SR->R[index]->Flags,&SR->Flags);

      bmset(&SR->R[index]->Flags,mrfStanding);
      
      MRsvSetAttr(SR->R[index],mraPartition,SR->PName,mdfString,mSet);

      if (WasActive == TRUE)
        bmset(&SR->R[index]->Flags,mrfWasActive);
 
      if (SR->ReqTC == 0)
        bmset(&SR->R[index]->Flags,mrfIgnState);

      if (SR->MaxJob > 0) /* put sr maxjob policy on children. */
        SR->R[index]->MaxJob = SR->MaxJob;

      /* Update rsv with how many jobs are running in it. Only do for 
       * reservations that are children of standing reservations */

      MRsvSetJobCount(SR->R[index]);

      MACLMergeMaster(&SR->R[index]->ACL,SR->ACL);

      index++;
      }

    /* erase expired reservation */

    if (SR->R[0] != NULL)
      {
      mulong StartTime;

      /* remove rsv if rsv ends or rollback offset completely overlaps rsv */

      if (SR->RollbackOffset > 0)
        StartTime = MAX(SR->R[0]->StartTime,MSched.Time + SR->RollbackOffset);
      else
        StartTime = SR->R[0]->StartTime;

      if ((SR->R[0]->EndTime > MSched.Time) && (SR->R[0]->EndTime > StartTime))
        {
        for (DIndex = 0;DIndex < (MMAX_SRSV_DEPTH - 1);DIndex++)
          {
          if (DIndex >= SR->Depth)
            break;

          if (SR->R[DIndex] == NULL)
            {
            /* create missing reservations */

            /* NOTE:  may allocate SR->HostList */

            if (MRsvConfigure(SR,NULL,SRIndex,DIndex,EMsg,NULL,SRP,FALSE) == FAILURE)
              {
              MDB(2,fCORE) MLog("ALERT:    cannot create SR[%d] %s at depth %d (%s)\n",
                SRIndex,
                SR->Name,
                DIndex,
                EMsg);
              }
            }
          }    /* END for (DIndex)                 */

        continue;
        }

      MRsvDestroy(&SR->R[0],TRUE,TRUE);

      SR->R[0] = NULL;
      }

    if (SR->Disable == TRUE)
      {
      MDB(4,fCORE) MLog("ALERT:    SR %s is disabled\n",
        SR->Name);

      continue;
      }

    /* re-evaluate rsv */

    if (SR->HostExp[0] != '\0')
      {
      /* NOTE:  MRsvCreateNL will allocate space if SRsv[SRIndex].HostList is NULL */

      if (MRsvCreateNL(
            &SR->HostList,
            SR->HostExp,
            0,
            SR->TPN,
            SR->PName,
            &SR->DRes,
            EMsg) == FAILURE)
        {
        MDB(4,fCORE) MLog("ALERT:    cannot create hostlist for SR %s (%s)",
          SR->Name,
          EMsg);
        } 
      }         /* END if (SR->HostExp[0] == '\0') */
    else
      {
      if (SR->ReqTC == 0)
        {
        continue;
        }
      }

    /* roll standing reservations forward, and creating missing reservations */

    for (DIndex = 0;DIndex < (MMAX_SRSV_DEPTH - 1);DIndex++)
      {
      if (DIndex >= SR->Depth)
        break;

      SR->R[DIndex]        = SR->R[DIndex + 1];

      SR->R[DIndex + 1]    = NULL;

      if (SR->R[DIndex] == NULL)
        {
        /* NOTE:  may allocate SR->HostList */

        if (MRsvConfigure(SR,NULL,SRIndex,DIndex,EMsg,NULL,SRP,FALSE) == FAILURE)
          {
          /* NOTE:  may allocate SR->HostList on SUCCESS/FAILURE */
          /*        HostList only allocated if originally NULL */

          MDB(2,fCORE) MLog("ALERT:    cannot create SR[%d] %s at depth %d (%s)\n",
            SRIndex,
            SR->Name,
            DIndex,
            EMsg);
          }
        }
      }    /* END for (DIndex)                 */
    }  /* END for (SRIndex) */

  return(SUCCESS);
  }  /* END MSRPeriodRefresh() */




/**
 * Creates a reservations node list.
 *
 * NOTE:  may eliminate some nodes from hostexpression if effective node
 *          available taskcount is 0 
 *
 * NOTE:  RHostExp FORMAT:  <HOST>[,<HOST>]... || <HOSTREGEX> || 
 *          R:<HOSTRANGE> || CLASS:<CLASSID>[,<CLASSID>] NOTE:  if RTC ==
 *          0, return all matches, otherwise return first RTC matches 
 *
 * @param NL       (I/O) [clear/alloc]
 * @param RHostExp (I) [host expression]
 * @param RTC      (I) - not set if <= 0 [optional]
 * @param RMaxTPN  (I) - not set if <= 0 [optional]
 * @param RPName   (I) [optional]
 * @param RDRes    (I)
 * @param EMsg     (O) [optional,minsize=MMAX_NAME]
 */

int MRsvCreateNL(

  mnl_t      *NL,
  char       *RHostExp,
  int         RTC,
  int         RMaxTPN,
  char       *RPName,
  mcres_t    *RDRes,
  char       *EMsg)

  {
  int     nindex;
  int     nindex2;
  int     oindex;
  int     TC;

  int     ReqTC;

  char   *Expr = NULL;
  
  char    Buffer[MMAX_BUFFER];
  char   *ptr;
  char   *TokPtr = NULL;

  marray_t NodeList;

  mnl_t     *HL;
  mnode_t *N;
  mpar_t *P;

  /* return node list of all hosts matching expression */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((NL == NULL) || (RHostExp == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid parameters - bad nodelist");

    return(FAILURE);
    }

  nindex = 0;

  HL = NL;

  MUStrDup(&Expr,RHostExp);

  ptr = MUStrTok(Expr," ",&TokPtr);

  if ((RPName == NULL) || (RPName[0] == '\0'))
    {
    P = &MPar[0];
    }
  else
    {
    if (MParFind(RPName,&P) == FAILURE)
      {
      /* requested partition does not exist */

      if (EMsg != NULL)
        strcpy(EMsg,"cannot locate requested partition");

      MUFree(&Expr);

      return(FAILURE);
      }
    }

  MUArrayListCreate(&NodeList,sizeof(mnode_t *),-1);

  while (ptr != NULL)
    {
    Buffer[0] = '\0';

    if (!strncasecmp(ptr,"class:",strlen("class:")))
      {
      MRsvExpandClassExpression(ptr,&NodeList,P);
      }
    else if (MUREToList(
          ptr,
          mxoNode,
          P,          /* routine checks to make sure nodes are in the partition */
          &NodeList,
          FALSE,
          Buffer,
          sizeof(Buffer)) == FAILURE)
      {
      MDB(2,fCONFIG) MLog("ALERT:    cannot expand hostlist '%s'\n",
        ptr);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot process host expression - %s",
          Buffer);
        }
 
      ptr = MUStrTok(NULL," ",&TokPtr);

      continue;
      }

    if (EMsg != NULL)
      {
      /* clear error buffer on success */

      EMsg[0] = '\0';
      }

    ptr = MUStrTok(NULL," ",&TokPtr);

    /* populate hostlist */

    for (oindex = 0;oindex < NodeList.NumItems;oindex++)
      {
      N = (mnode_t *) MUArrayListGetPtr(&NodeList,oindex);

      MNLAddNode(HL,N,1,FALSE,NULL);
      }     /* END for (oindex)        */
    }       /* END while (ptr != NULL) */

  MUArrayListFree(&NodeList);
      
  MUFree(&Expr); /* Done using. */

  if (MNLIsEmpty(HL))
    {
    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      strcpy(EMsg,"hostlist is empty");
      }

    return(FAILURE);
    }

  /* adjust host list to match available resources */

  nindex2 = 0;

  ReqTC = RTC;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    if (MNLGetNodeAtIndex(HL,nindex,&N) == FAILURE)
      break;

    /* Scenario: RSVREALLOCPOLICY FAILURE; reservation with a host-expression covering 3 nodes 
     * with a taskcount of 2. The reservation gets the first 2 nodes. Later the 2nd node goes
     * down. Because this routine thinks that the downed node has available taskcount and returns
     * RTC nodes from the host-expression, reservation will never get a taskcount of 2 because
     * the third node isn't considered and the downed node isn't available in MJobGetEStartTime. 
     * Reservation Nuance: A host-expression reservation is ignores the state of nodes. */

    TC = MNodeGetTC(N,&N->ARes,&N->CRes,&N->DRes,RDRes,MMAX_TIME,1,NULL);

    if (TC <= 0)
      continue;

    if (nindex != nindex2)
      {
      MNLSetNodeAtIndex(HL,nindex2,N);
      }

    if (RMaxTPN > 0)
      {
      MNLSetTCAtIndex(HL,nindex2,MIN(TC,RMaxTPN));
      }
    else if (ReqTC > 0)
      {
      MNLSetTCAtIndex(HL,nindex2,MIN(TC,ReqTC));

      ReqTC -= TC;

      if (ReqTC <= 0)
        {
        nindex2++;

        break;
        }
      }
    else
      {
      MNLSetTCAtIndex(HL,nindex2,TC);
      }

    nindex2++;
    }       /* END for (nindex) */

  MNLTerminateAtIndex(HL,nindex2);

  return(SUCCESS);
  }  /* END MRsvCreateNL() */




 
/**
 * Report standing rsv attribute as string.
 *
 * @see MSRSetAttr() - peer
 *
 * @param SR (I)
 * @param AIndex (I)
 * @param String (O)
 * @param ModeBM (I) [enum mcm* bitmap]
 */

int MSRAToString(
 
  msrsv_t          *SR,
  enum MSRAttrEnum  AIndex,
  mstring_t        *String,
  mbitmap_t        *ModeBM)
 
  {
  if (String != NULL)
    String->clear();
  if ((SR == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case msraAccountList:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maAcct,&BM,String);
      }

      break;

    case msraChargeAccount:

      if (SR->A != NULL)
        {
        MStringAppendF(String,"%s",SR->A->Name);
        }
      else if (bmisset(ModeBM,mcmVerbose))
        {
        MStringAppendF(String,"%s","NONE");
        }

      break;

    case msraChargeUser:

      if (SR->U != NULL)
        {
        MStringAppendF(String,"%s",SR->U->Name);
        }
      else if (bmisset(ModeBM,mcmVerbose))
        {
        MStringAppendF(String,"%s","NONE");
        }

      break;

    case msraClassList:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maClass,&BM,String);
      }

      break;

    case msraClusterList:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maCluster,&BM,String);
      }

      break;

    case msraDays:

      if ((!bmisclear(&SR->Days)) && (!bmisset(&SR->Days,MCONST_ALLDAYS)))
        {
        bmtostring(&SR->Days,MWeekDay,String);

        /* NOTE:  bmtostring() will not display index 0, ie Sun */
 
        if (bmisset(&SR->Days,0))
          {
          if (!MUStrIsEmpty(String->c_str()))
            MStringAppend(String,",");

          MStringAppend(String,MWeekDay[0]);
          }
        }
      else if (bmisset(&SR->Days,MCONST_ALLDAYS))
        {

        }

      break;

    case msraDepth:

      if (SR->Depth > 0)
        {
        MStringAppendF(String,"%d",
          SR->Depth);
        }

      break;

    case msraDisable:

      if (bmisset(ModeBM,mcmVerbose) || (SR->Disable == TRUE))
        {
        MStringAppendF(String,"%s",
          MBool[SR->Disable]);
        }

      break;

    case msraDisabledTimes:
      {
      int sindex;

      for (sindex = 0;sindex < MMAX_SRSV_DEPTH;sindex++)
        {
        if (SR->DisabledTimes[sindex] == 0)
          break;

        if (SR->DisabledTimes[sindex] == 1)
          continue;

        if (MUStrIsEmpty(String->c_str()))
          MStringAppendF(String,"%ld",
            SR->DisabledTimes[sindex]);
        else
          MStringAppendF(String,",%ld",
            SR->DisabledTimes[sindex]);
        } 
      }
      
      break;

    case msraEndTime:

      if (SR->EndTime > 0)
        {
        MStringAppendF(String,"%ld",
          SR->EndTime);
        }

      break;

    case msraFlags:

      if (!bmisclear(&SR->Flags))
        {
        bmtostring(&SR->Flags,MRsvFlags,String);
        }

      break;

    case msraGroupList:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maGroup,&BM,String);
      }

      break;

    case msraHostList:

      if (SR->HostExp[0] != '\0')
        {
        MStringAppend(String,SR->HostExp);
        }

      break;

    case msraIsDeleted:

      if (bmisset(ModeBM,mcmVerbose) || (SR->IsDeleted == TRUE))
        {
        MStringAppendF(String,"%s",
          MBool[SR->IsDeleted]);
        }

      break;

    case msraJobAttrList:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maJAttr,&BM,String);
      }

      break;

    case msraMaxJob:

      MStringAppendF(String,"%d",SR->MaxJob);

      break;

    case msraMinMem:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maMemory,&BM,String);
      }

      break;

    case msraMMB:

      if (SR->MB != NULL)
        {
        char tmpBuf[MMAX_BUFFER];

        MMBPrintMessages(SR->MB,mfmHuman,FALSE,-1,tmpBuf,sizeof(tmpBuf));

        MStringAppend(String,tmpBuf);
        }

      break;

    case msraName:

      if (SR->Name != NULL)
        {
        MStringAppendF(String,"%s",
          SR->Name);
        }

      break;

    case msraNodeFeatures:

      {
      char Line[MMAX_LINE];

      MStringAppend(String,MUNodeFeaturesToString(',',&SR->FBM,Line));
      }

      break;

    case msraOS:

      if (SR->OS != NULL)
        MStringAppend(String,SR->OS);

      break;

    case msraOwner:

      if (SR->OType != mxoNONE)
        {
        MStringAppendF(String,"%s:%s",
          MXOC[SR->OType],
          MOGetName(SR->O,SR->OType,NULL));
        }

      break;

    case msraPartition:

      if (SR->PName != NULL)
        MStringAppend(String,SR->PName);

      break;

    case msraPeriod:

      if (SR->Period != mpNONE)
        {
        MStringAppendF(String,"%s",
          MCalPeriodType[SR->Period]);
        }

      break;

    case msraQOSList:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maQOS,&BM,String);
      }

      break;

    case msraResources:

      if ((SR->DRes.Procs == -1) && (MSNLIsEmpty(&SR->DRes.GenericRes)))
        break;

      MCResToMString(
        &SR->DRes,
        0,
        mfmHuman,
        String);

      break;

    case msraRollbackOffset:

      if (SR->RollbackOffset > 0)
        {
        MStringAppendF(String,"%ld",
          SR->RollbackOffset);
        }

      break;

    case msraRsvList:

      {
      int   rindex;

      for (rindex = 0;rindex < MMAX_SRSV_DEPTH;rindex++)
        {
        if (SR->R[rindex] == NULL)
          continue;

        if (!MUStrIsEmpty(String->c_str()))
          {
          MStringAppendF(String,",%s",
            SR->R[rindex]->Name);
          }
        else
          {
          MStringAppendF(String,"%s",
            SR->R[rindex]->Name);
          }
        }  /* END for (rindex) */
      }

      break;

    case msraStartTime:

      if (SR->StartTime > 0)
        {
        MStringAppendF(String,"%ld",
          SR->StartTime);
        }

      break;

    case msraStIdleTime:

      if (bmisset(ModeBM,mcmVerbose) || (SR->IdleTime > 0.0))
        {
        MStringAppendF(String,"%.2f",
          SR->IdleTime);
        }

      break;

    case msraStTotalTime:

      if (bmisset(ModeBM,mcmVerbose) || (SR->TotalTime > 0.0))
        {
        MStringAppendF(String,"%.2f",
          SR->TotalTime);
        }

      break;

    case msraTaskCount:

      if (bmisset(ModeBM,mcmVerbose) || (SR->ReqTC > 0))
        {
        MStringAppendF(String,"%d",
          SR->ReqTC);
        }

      break;

    case msraTimeLimit:

      if (bmisset(ModeBM,mcmXML))
        {
        macl_t *tACL = NULL;

        MACLGet(SR->ACL,maDuration,&tACL);

        if (tACL != NULL)
          MStringAppendF(String,"%ld",tACL->LVal);
        }
      else
        {
        mbitmap_t BM;

        bmset(&BM,mfmHuman);

        MACLListShowMString(SR->ACL,maDuration,&BM,String);
        }

      break;

    case msraTrigger:

      if (SR->T != NULL)
        MTListToMString(SR->T,TRUE,String);

      break;

    case msraType:

      if (SR->Type != mrtNONE)
        {
        MStringAppendF(String,"%s",
          MRsvType[SR->Type]);
        }

      break;

    case msraUserList:

      {
      mbitmap_t BM;

      bmset(&BM,mfmHuman);

      MACLListShowMString(SR->ACL,maUser,&BM,String);
      }

      break;

    case msraVariables:

      if (SR->Variables != NULL)
        {
        mln_t *tmpL;

        tmpL = SR->Variables;

        while (tmpL != NULL)
          {
          MStringAppend(String,tmpL->Name);

          if (tmpL->Ptr != NULL)
            {
            MStringAppendF(String,"=%s;",(char *)tmpL->Ptr);
            }
          else
            {
            MStringAppend(String,";");
            }

          tmpL = tmpL->Next;
          }  /* END while (tmpL != NULL) */
        }    /* END (case msraVariable) */

      break;

    default:

      /* not supported */
 
      return(FAILURE);
 
      /*NOTREACHED*/
 
      break;
    }  /* END switch (AIndex) */

  if (MUStrIsEmpty(String->c_str()))
    {
    return(FAILURE);
    }
 
  return(SUCCESS);
  }  /* END MSRAToString() */





/**
 * Report standing rsv as string.
 *
 * @see MSRToXML() - child
 *
 * @param SR (I)
 * @param Buf (O)
 */

int MSRToString(

  msrsv_t *SR,  /* I */
  char    *Buf) /* O */

  {
  const enum MSRAttrEnum CPAList[] = {
    msraStIdleTime,
    msraStTotalTime,
    msraDisabledTimes,
    msraChargeAccount,
    msraChargeUser,
    msraNONE };

  mxml_t *E = NULL;

  if ((SR == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MXMLCreateE(&E,(char *)MXO[mxoSRsv]);

  MSRToXML(SR,E,(enum MSRAttrEnum *)CPAList);

  MXMLToString(E,Buf,MMAX_BUFFER,NULL,TRUE);

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MSRToString() */





/**
 * Report standing reservation as XML.
 *
 * @see MSRFromXML() - peer
 *
 * @param SR (I)
 * @param E (O)
 * @param SAList (I) [optional]
 */

int MSRToXML(

  msrsv_t          *SR,      /* I */
  mxml_t           *E,       /* O */
  enum MSRAttrEnum *SAList)  /* I (optional) */

  {
  enum MSRAttrEnum DAList[] = {
    msraName,
    msraStIdleTime,
    msraStTotalTime,
    msraTaskCount,
    msraChargeAccount,
    msraChargeUser,
    msraStartTime,
    msraEndTime,
    msraVariables,
    msraNONE };

  int  aindex;

  enum MSRAttrEnum *AList;

  mbitmap_t BM;

  if ((SR == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = DAList;

  mstring_t tmpString(MMAX_LINE);

  bmset(&BM,mcmXML);

  for (aindex = 0;AList[aindex] != msraNONE;aindex++)
    {
    if ((MSRAToString(SR,AList[aindex],&tmpString,&BM) == FAILURE) ||
        (tmpString.empty()))
      {
      continue;
      }

    MXMLSetAttr(E,(char *)MSRsvAttr[AList[aindex]],tmpString.c_str(),mdfString);
    }  /* END for (aindex) */

  if (!MACLIsEmpty(SR->ACL))
    {
    macl_t *tACL;
    mxml_t *AE = NULL;

    for (tACL = SR->ACL;tACL != NULL;tACL = tACL->Next)
      {
      MACLToXML(tACL,&AE,NULL,FALSE); 

      MXMLAddE(E,AE);
      }
    }    /* END if (!MACLIsEmpty(SR->ACL)) */

  return(FAILURE);
  }  /* END MSRToXML() */





/**
 *
 *
 * @param SR (O) [modified]
 * @param Buf (I)
 */

int MSRFromString(

  msrsv_t *SR,   /* O (modified) */
  char    *Buf)  /* I */

  {
  mxml_t *E = NULL;

  int rc;

  if ((Buf == NULL) || (SR == NULL))
    {
    return(FAILURE);
    }

  rc = MXMLFromString(&E,Buf,NULL,NULL);

  if (rc == SUCCESS)
    {
    rc = MSRFromXML(SR,E);
    }

  MXMLDestroyE(&E);

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSRFromString() */





/**
 * Extract/update standing reservation from XML.
 *
 * @see MSRToXML() - peer
 *
 * @param SR (O) [modified]
 * @param E (I)
 */

int MSRFromXML(

  msrsv_t *SR,  /* O (modified) */
  mxml_t  *E)   /* I */

  {
  int aindex;

  enum MSRAttrEnum saindex;

  if ((SR == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  do not initialize.  may be overlaying data */

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    saindex = (enum MSRAttrEnum)MUGetIndex(E->AName[aindex],MSRsvAttr,FALSE,0);

    if (saindex == msraNONE)
      continue;

    MSRSetAttr(SR,saindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */
 
  return(SUCCESS);
  }  /* END MSRFromXML() */





/**
 * Set standing rsv attribute.
 *
 * @see MSRAToString() - peer
 *
 * @param SR (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I) [bitmap of enum mcm*]
 */

int MSRSetAttr(
 
  msrsv_t         *SR,     /* I (modified) */
  enum MSRAttrEnum AIndex, /* I */
  void           **Value,  /* I */
  enum MDataFormatEnum Format, /* I */
  int              Mode)   /* I (bitmap of enum mcm*) */
 
  {
  long tmpL;
  int  tmpI;

  char *tmpS;

  if (SR == NULL)
    {
    return(FAILURE);
    }

  tmpL = 0;
  tmpI = 0;

  tmpS = NULL;

  if (Value != NULL)
    {
    switch (Format)
      {
      case mdfLong:

        tmpL = *(long *)Value;

        break;

      case mdfInt:

        tmpI = *(int *)Value;

        break;

      default:

        tmpS = (char *)Value;
 
        tmpL = strtol(tmpS,NULL,0);
        tmpI = (int)tmpL;

        break;
      }  /* END switch (Format) */
    }    /* END if (Value != NULL) */

  switch (AIndex)
    {
    case msraChargeAccount:

      MAcctAdd((char *)Value,&SR->A);

      break;

    case msraChargeUser:

      MUserAdd((char *)Value,&SR->U);

      break;

    case msraClassList:

      MACLFromString(&SR->ACL,tmpS,1,maClass,mSet,NULL,FALSE);

      break;

    case msraDepth:

      SR->Depth = tmpI;

      break;

    case msraDisable:

      SR->Disable = MUBoolFromString((char *)Value,FALSE);
 
      break;

    case msraDisableTime:

      if (MUStringToE(tmpS,(long *)&SR->DisableTime,TRUE) != SUCCESS)
        {
        SR->DisableTime = 0;
        }

      break;

    case msraDisabledTimes:

      {
      int     sindex = 0;
      mulong  tmpL;

      char *ptr;
      char *TokPtr = NULL;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        tmpL = (mulong)strtol(ptr,NULL,10);

        SR->DisabledTimes[sindex++] = MAX(1,tmpL);

        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }
      
      break;

    case msraDuration:

      SR->Duration = MUTimeFromString(tmpS);

      break;

    case msraEnableTime:

      if (MUStringToE(tmpS,(long *)&SR->EnableTime,TRUE) != SUCCESS)
        {
        SR->EnableTime = 0;
        }

      break;

    case msraEndTime:

      if (Format == mdfLong)
        SR->EndTime = tmpL;
      else
        SR->EndTime = MUTimeFromString(tmpS);

      break;

    case msraHostList:

      {
      MUStrCpy(SR->HostExp,tmpS,sizeof(SR->HostExp));

      /* NOTE: taken out by Doug 8-21-06 
               causes reservations to not be created correctly from the .moab.ck */
     
#ifdef __MNOT
      /* replace commas with spaces */

      while ((ptr = strchr(SR->HostExp,',')) != NULL)
        {
        *ptr = ' ';
        }
#endif /* __MNOT */
      }    /* END BLOCK */

      break;

    case msraMaxJob:

      if (Format == mdfInt)
        SR->MaxJob = tmpL;
      else
        SR->MaxJob = MUTimeFromString(tmpS);

      break;

    case msraName:

      MUStrCpy(SR->Name,tmpS,sizeof(SR->Name));

      break;

    case msraOS:

      MUStrDup(&SR->OS,tmpS);

      break;

    case msraOwner:

      {
      if (Format == mdfString)
        {
        char tmpLine[MMAX_LINE];

        enum MXMLOTypeEnum oindex;

        char *TokPtr;
        char *ptr;

        void *optr;

        /* FORMAT:  <CREDTYPE>:<CREDID> */

        MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

        ptr = MUStrTok(tmpLine,": \t\n",&TokPtr);

        if ((ptr == NULL) || 
           ((oindex = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXOC,FALSE,mxoNONE)) == mxoNONE))
          {
          /* invalid format */

          MDB(2,fCONFIG) MLog("WARNING:  cannot process owner '%s' for SR[%s] - invalid format\n",
            (char *)Value,
            SR->Name);

          return(FAILURE);
          }

        ptr = MUStrTok(NULL,": \t\n",&TokPtr);

        if (MOGetObject(oindex,ptr,&optr,mAdd) == FAILURE)
          {
          MDB(2,fCONFIG) MLog("WARNING:  cannot process owner '%s' for SR[%s] - invalid cred type\n",
            (char *)Value,
            SR->Name);

          return(FAILURE);
          }

        SR->O     = (mgcred_t *)optr;
        SR->OType = oindex;
        }  /* END if (Format == mdfString) */
      else
        {
        /* NYI */

        return(FAILURE);
        }
      }    /* END BLOCK (case msraOwner) */

      break;

    case msraPartition:

      MUStrCpy(SR->PName,tmpS,sizeof(SR->PName));

      break;

    case msraPeriod:

      {
      enum MCalPeriodEnum period = mpNONE;
      period = (enum MCalPeriodEnum)MUGetIndexCI(tmpS,MCalPeriodType,FALSE,mpNONE);
      if (period == mpNONE)
        {
        return(FAILURE);
        }
      else
        {
        SR->Period = period;
        }
      }
      break;

    case msraRollbackOffset:

      if (Format == mdfLong)
        SR->RollbackOffset = tmpL;
      else
        SR->RollbackOffset = MUTimeFromString(tmpS);

      break;

    case msraRsvAccessList:

      {
      int   index;
      char *TokPtr;
      char *ptr;

      /* clear old values */

      if (SR->RsvAList != NULL)
        {
        for (index = 0;index < MMAX_PJOB;index++)
          {
          if (SR->RsvAList[index] == NULL)
            break;

          MUFree(&SR->RsvAList[index]);
          }  /* END for (index) */
        }

      if ((tmpS == NULL) ||
          (((char *)tmpS)[0] == '\0') ||
          !strcmp((char *)tmpS,NONE))
        {
        /* empty list specified */

        break;
        }

      if (SR->RsvAList == NULL)
        SR->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

      ptr = MUStrTok((char *)tmpS,", \t\n",&TokPtr);

      index = 0;

      while ((ptr != NULL) && (ptr[0] != '\0'))
        {
        MUStrDup(&SR->RsvAList[index],ptr);

        index++;

        if (index >= MMAX_PJOB)
          break;

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case msraRsvAccessList) */

      break;
 
    case msraStartTime:

      if (Format == mdfLong)
        SR->StartTime = tmpL;
      else
        SR->StartTime = MUTimeFromString(tmpS);

      break;

    case msraStIdleTime:
 
      SR->IdleTime = tmpL;

      break;

    case msraStTotalTime:

      SR->TotalTime = tmpL;

      break;

    case msraTaskCount:

      SR->ReqTC = tmpI;

      break;

    case msraType:

      SR->Type = (enum MRsvTypeEnum)MUGetIndexCI(tmpS,MRsvType,FALSE,0);

      break;

    case msraVariables:

      /* FORMAT:  <name>[=<value>][[;<name[=<value]]...] */

      {
      mln_t *tmpL;

      char *ptr;
      char *TokPtr = NULL;

      char *TokPtr2 = NULL;
      char tmpVal[MMAX_LINE];

      /* add specified data */

      MUStrCpy(tmpVal,(char *)Value,sizeof(tmpVal));

      ptr = MUStrTok((char *)tmpVal,";",&TokPtr);

      while (ptr != NULL)
        {
        MUStrTok(ptr,"=",&TokPtr2);

        MULLAdd(&SR->Variables,ptr,NULL,&tmpL,MUFREE);

        if (TokPtr2[0] != '\0')
          {
          MUStrDup((char **)&tmpL->Ptr,TokPtr2);
          }

        ptr = MUStrTok(NULL,";",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END (case msraVariable) */

      break;

    default:

      /* not supported */

      return(FAILURE);

      /*NOTREACHED*/
 
      break;
    }  /* END switch (AIndex) */
 
  return(SUCCESS);
  }  /* END MSRSetAttr() */




/**
 * Converts given standing-reservation into a reservation profile.
 *
 * @param SR (I)
 * @param RProf (O)
 */

int MSRToRsvProfile(

  msrsv_t *SR,    /* I */
  mrsv_t  *RProf) /* O */

  {
  if ((SR == NULL) || (RProf == NULL))
    {
    return(FAILURE);
    }

  /* copy name */

  strcpy(RProf->Name,SR->Name);

  /* copy triggers */

  if (SR->T != NULL)
    {
    int tindex;

    mtrig_t *T;

    MTListCopy(SR->T,&RProf->T,FALSE);

    for (tindex = 0;tindex < RProf->T->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(RProf->T,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      bmset(&T->InternalFlags,mtifIsTemplate);
      }
    }

  /* copy acls's */

  MRsvSetAttr(RProf,mraACL,(void *)&SR->ACL[0],mdfOther,mSet);

  /* copy host expression */

  if ((SR->HostExp != NULL) && (SR->HostExp[0] != '\0'))
    {
    MRsvSetAttr(RProf,mraHostExp,(void *)SR->HostExp,mdfString,mSet);
    }

  /* copy feature requirements */

  if (!bmisclear(&SR->FBM)) 
    {
    MRsvSetAttr(RProf,mraReqFeatureList,(void *)&SR->FBM,mdfOther,mSet);

    RProf->ReqFMode = SR->FeatureMode;
    }

  /* copy flags */

  if (!bmisclear(&SR->Flags))
    {
    MRsvSetAttr(RProf,mraFlags,(void *)&SR->Flags,mdfOther,mAdd);
    }

  /* copy taskcount */

  if (SR->Duration != 0)
    RProf->Duration = SR->Duration;

  RProf->ReqTC = SR->ReqTC;
  
  if (SR->RsvAList != NULL)
    {
    int index = 0;

    /* NOTE: this overrides existing RsvAList */

    if (RProf->RsvAList != NULL)
      {
      for (index = 0;index < MMAX_PJOB;index++)
        {
        if (RProf->RsvAList[index] == NULL)
          break;
   
        MUFree(&RProf->RsvAList[index]);
        }
      }
   
    RProf->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

    while (SR->RsvAList[index] != NULL)
      {
      if (index >= MMAX_PJOB)
        break;

      MUStrDup(&RProf->RsvAList[index],SR->RsvAList[index]);

      index++;
      } 
    }   /* END if (SR->RsvAList != NULL) */

  if (SR->SysJobTemplate != NULL)
    {
    MUStrDup(&RProf->SysJobTemplate,SR->SysJobTemplate);
    }

  bmset(&RProf->IFlags,mrifReservationProfile);

  return(SUCCESS);
  }  /* END MSRToRsvProfile() */




int MSRFreeTable()

  {
  int srindex;

  msrsv_t *SR;

  for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
    {
    SR = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);

    if (SR == NULL)
      {
      break;
      }

    if (SR == (msrsv_t *)1)
      continue;

    if ((SR->Name[0] == '\0') || (SR->Name[0] == '\1'))
      continue;

    MSRDestroy(&SR);
    }  /* END for (srindex) */

  return(SUCCESS);
  }  /* END MSRFreeTable() */



/**
 *
 *
 * @param GridSandboxIsGlobal (O) [optional]
 */

int MSRUpdateGridSandboxes(

  mbool_t *GridSandboxIsGlobal)  /* O (optional) */

  {
  int srindex;
  mbool_t IsGlobal;
  msrsv_t *tmpSR;

  IsGlobal = FALSE;

  for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
    {
    tmpSR = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);

    if (tmpSR->Name[0] == '\0')
      break;

    if (bmisset(&tmpSR->Flags,mrfAllowGrid))
      {
      MACLCheckGridSandbox(tmpSR->ACL,&IsGlobal,NULL,mSet);

      if (IsGlobal == TRUE)
        break;
      }
    }  /* END for (srindex) */

  if (GridSandboxIsGlobal != NULL)
    *GridSandboxIsGlobal = IsGlobal;

  return(SUCCESS);  
  }    /* END MSRUpdateGridSandboxes() */




/**
 * Updates all standing reservation reservation's job count when
 * there is overlap with a standing reservation and the given job.
 *
 * @param J         (I) The Job that possibly has overlap.
 * @param Increment (I) If TRUE, will increment Job Count on reservations (JobStart), 
 *                      otherwise decrement count (Job Removal).
 */

int MSRUpdateJobCount(

  mjob_t  *J,
  mbool_t  Increment)

  {
  int rindex;

  marray_t RList;

  mrsv_t *R;

  const char *FName = "MSRUpdateJobCount";
 
  MDB(8,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MBool[Increment]);
    
  if (J == NULL)
    {
    return(FAILURE);
    }

  if (MSRsv.NumItems <= 0)
    {
    return(SUCCESS);
    }

  MUArrayListCreate(&RList,sizeof(mrsv_t *),MSRsv.NumItems + 1);

  MJobGetOverlappingSRsvs(J,&RList);

  for (rindex = 0;rindex < RList.NumItems;rindex++)
    {
    R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

    if (Increment == TRUE)
      {
      R->JCount++;
      }
    else
      {
      if (R->JCount <= 0)
        {
        MDB(7,fSTRUCT) MLog("WARNING:  reservation %s jobcount is %d, shouldn't decrement less than 0\n",
          R->Name,
          R->JCount);

        continue;
        }

      R->JCount--;
      }

    MDB(7,fSTRUCT) MLog("INFO:     reservation %s jobcount %s to %d\n",
      R->Name,
      (Increment == TRUE) ? "incremented" : "decremented",
      R->JCount);
    }

  MUArrayListFree(&RList);
  
  return(SUCCESS);
  } /* END int MSRUpdateJobCount() */

/* END MSR.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
