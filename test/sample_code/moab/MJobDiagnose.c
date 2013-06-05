/* HEADER */

      
/**
 * @file MJobDiagnose.c
 *
 * Contains:
 *     Job Diagnose functions
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report general job health and eligibility and resource access info.
 *
 * @see MUIJobShow() - parent
 * @see MJobDiagnoseEligibility() - child
 *
 * NOTE:  will report resource access details for idle, held, and suspended jobs.
 * NOTE:  reports job/resource eligibility for 'checkjob' request.
 *
 * @param J (I)
 * @param SN (I) [optional]
 * @param PLevel (I)
 * @param Flags (I) [bitmap of enum mcm*]
 * @param IgnBM (I) [optional]
 * @param Buffer (O) (should already be initialized)
 */

int MJobDiagnoseState(

  mjob_t        *J,
  mnode_t       *SN,
  enum MPolicyTypeEnum PLevel,
  mbitmap_t     *Flags,
  mbitmap_t     *IgnBM,
  mstring_t     *Buffer)

  {
  const char *FName = "MJobDiagnoseState";

  MDB(4,fSTRUCT) MLog("%s(%s,%s,%u,IgnBM)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (SN != NULL) ? SN->Name : "NULL",
    PLevel);

  if ((J == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  /* perform general analysis */

  /* display defer info/state discrepancies */

  if (J->EState == mjsDeferred)
    {
    /* job defer reported in job hold display */

    /* NO-OP */
    }
  else if ((J->State != J->EState) &&
          ((J->State != mjsRunning) ||
           (J->EState != mjsStarting)))
    {
    MStringAppendF(Buffer,"EState '%s' does not match current state '%s'\n",
      MJobState[J->EState],
      MJobState[J->State]);
    }

  /* display job holds */

  if (!bmisclear(&J->Hold))
    {
    char tmpLine[MMAX_LINE];
    char DString[MMAX_LINE];

    if (bmisset(&J->Hold,mhDefer))
      {
      char TString[MMAX_LINE];

      MULToTString(MSched.DeferTime - ((long)MSched.Time - J->HoldTime),TString);

      sprintf(DString,"(DeferTime: %s  DeferCount: %d)",
        TString,
        J->DeferCount);        
      }
    else
      {
      DString[0] = '\0';
      }

    if (J->HoldReason != mhrNONE)
      {
      MStringAppendF(Buffer,"Holds:          %s:%s  %s\n",
        bmtostring(&J->Hold,MHoldType,tmpLine),
        MHoldReason[J->HoldReason],
        DString);
      }
    else
      {
      MStringAppendF(Buffer,"Holds:          %s  %s\n",
        bmtostring(&J->Hold,MHoldType,tmpLine),
        DString);
      }
    }    /* END if (J->Hold != 0) */

  switch (J->State)
    {
    case mjsHold:
    case mjsIdle:

      {
      char tmpMsg[MMAX_LINE << 4];

      tmpMsg[0] = '\0';

      MJobDiagnoseEligibility(
        J,
        NULL,
        SN,
        PLevel,
        mfmHuman,
        Flags,
        IgnBM,
        tmpMsg,
        sizeof(tmpMsg),
        TRUE);

      MStringAppend(Buffer,tmpMsg);
      }

      break;

    case mjsSuspended:

      {
      mnl_t     *MNodeList[MMAX_REQ_PER_JOB];

      char *NodeMap = NULL;
      char  EMsg[MMAX_LINE];

      mbool_t AllowPreemption = MBNOTSET;

      EMsg[0] = '\0';

      MNodeMapInit(&NodeMap);
      MNLMultiInit(MNodeList);

      if (MJobSelectMNL(
            J,
            &MPar[J->Req[0]->PtIndex],
            &J->NodeList,
            MNodeList,
            NodeMap,
            AllowPreemption,
            FALSE,
            NULL,
            EMsg,
            NULL) == FAILURE)
        {
        MStringAppendF(Buffer,"job cannot be resumed: %s\n",
          EMsg);
        }
      else
        {
        MStringAppendF(Buffer,"job can be resumed\n");
        }

      MUFree(&NodeMap);
      MNLMultiFree(MNodeList);
      }

      break;

    case mjsStarting:
    case mjsRunning:
    default:

      /* NO-OP */

      break;
    }  /* END switch (J->State) */

  if ((bmisset(Flags,mcmVerbose) == TRUE) &&
      (bmisset(&J->Flags,mjfPreemptor) == TRUE) && 
      (J->State == mjsIdle) &&
      (SN != NULL) && 
      (SN->JList != NULL) &&
      (SN->JList[0] != NULL))
    {
    int     jindex;

    mjob_t *NJ;

    char    tmpLine[MMAX_LINE];

    /* job is preemptor */

    for (jindex = 0;jindex < SN->MaxJCount;jindex++)
      {
      NJ = SN->JList[jindex];

      if (NJ == NULL)
        break;

      if (NJ == (mjob_t *)1) 
        continue;

      if (MJobCanPreempt(J,NJ,J->PStartPriority[SN->PtIndex],&MPar[SN->PtIndex],tmpLine) == FALSE)
        {
        MStringAppendF(Buffer,"NOTE:  job %s cannot be preempted - '%s - partition (%s)'\n",
          NJ->Name,
          tmpLine,
          MPar[SN->PtIndex].Name);
        }
      else
        {
        MStringAppendF(Buffer,"NOTE:  job %s can be preempted - partition (%s)\n",
          NJ->Name,
          MPar[SN->PtIndex].Name);
        }
      }   /* END for (jindex) */
    }     /* END if ((bmisset(Flags,mcmVerbose) == TRUE) && ...) */

  return(SUCCESS);
  }  /* END MJobDiagnoseState() */





/**
 * Report diagnostics on eligibility of job (handle 'checkjob -v' request).
 *
 * NOTE:  report job, partition, and node level eligibility
 *
 * @see __MUIJobToXML() - parent
 * @see MJobDiagnoseState() - parent
 * @see MNodeDiagnoseEligibility() - child
 *
 * @param J (I)
 * @param SJE (I) [optional]
 * @param SN (I) [optional]
 * @param PLevel (I)
 * @param DFormat (I) [mfmHuman|mfmXML]
 * @param Flags (I) [bitmap of enum mcm*]
 * @param IgnBM (I) [optional]
 * @param OBuf (O) [may be XML or char buffer]
 * @param OBufSize (I)
 * @param DiagnoseReqConstraints (I)
 */

#define MTMP_QUEUE_SIZE 2

int MJobDiagnoseEligibility(

  mjob_t              *J,        /* I */
  mxml_t              *SJE,      /* I (optional) */
  mnode_t             *SN,       /* I specified target node (optional) */
  enum MPolicyTypeEnum PLevel,   /* I */
  enum MFormatModeEnum DFormat,  /* I (mfmHuman|mfmXML) */
  mbitmap_t           *Flags,    /* I (bitmap of enum mcm*) */
  mbitmap_t           *IgnBM,    /* I rejection reason ignore list (optional) */
  char                *OBuf,     /* O (may be XML or char buffer) */
  int                  OBufSize, /* I */
  mbool_t              DiagnoseReqConstraints) /* I */

  {
  char *BPtr;
  int   BSpace;

  char *tBPtr;
  int   tBSpace;

  mbool_t BlockCount;

  mbool_t IsHeld;
  mbool_t IsDeferred;
  mbool_t IsMultiReq;

  mjob_t *SrcQ[MTMP_QUEUE_SIZE];
  mjob_t *DstQ[MTMP_QUEUE_SIZE];

  int  pindex;
  int  nindex;

  int  rindex;
  int  rqindex;

  enum MAllocRejEnum RIndex;

  int  pcount;
  int  tcount;
  int  ncount;
  int  rcount;

  int  RCount[MMAX_TAPERNODE];

  enum MPolicyRejectionEnum PReason;

  char  MsgBuf[MMAX_BUFFER << 3];
  char  GlobalMsg[MMAX_LINE];

  char  tmpLine[MMAX_LINE];
  char  tmpPLine[MMAX_LINE];  /* allocation priority */

  int   IProcs;
  int   TC;
  int   JPC;                 /* job request proc count */

  mbitmap_t HList;
  enum MJobStateEnum JState = mjsNONE;  /* initialized to avoid compiler warning */

  mnode_t *N;
  mreq_t  *RQ;
  mpar_t  *P;

  mbitmap_t BM;

  mxml_t  *DE = NULL;  /* set to avoid compiler warnings */
  mxml_t  *CE;
  mxml_t  *JE;
  mxml_t  *NE;

  mbool_t  ProvMayHelp;  /* provisioning may help in eligible job start */

  int      MaxTC = 0;
  int      roffset;

  char *RNL = NULL;

  mbool_t  IgnPolicies  = FALSE;  /* NYI */
  /* mbool_t  IgnNodeState = FALSE; */  /* NYI */
  /* mbool_t  IgnNodeRsv   = FALSE; */  /* NYI */

  const char *FName = "MJobDiagnoseEligibility";

  MDB(4,fSTRUCT) MLog("%s(%s,%s,%s,%u,%u,IgnBM,OBuf,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (SJE != NULL) ? "SJE" : "NULL",
    (SN != NULL) ? SN->Name : "NULL",
    PLevel,
    DFormat,
    OBufSize);

  if ((J == NULL) || (OBuf == NULL))
    {
    return(FAILURE);
    }

  if ((DFormat == mfmHuman) && (OBufSize == 0))
    {
    return(FAILURE);
    }

  if ((MPar[0].ConfigNodes > 256) && !bmisset(Flags,mcmVerbose2))
    {
    bmset(Flags,mcmSummary);
    }

  /* initialize global job response */

  BlockCount = 0;

  IsHeld     = FALSE;
  IsDeferred = FALSE;

  MUSNInit(&BPtr,&BSpace,MsgBuf,sizeof(MsgBuf));

  if (DFormat == mfmXML)
    {
    DE = (mxml_t *)OBuf;

    CE = NULL;

    MXMLCreateE(&CE,(char *)MXO[mxoCluster]);

    MXMLAddE(DE,CE);

    if (SJE != NULL)
      {
      JE = SJE;
      }
    else
      {
      MXMLCreateE(&JE,(char *)MXO[mxoJob]);

      MXMLAddE(DE,JE);
      }
    }
  else
    {
    OBuf[0] = '\0';

    MUSNPrintF(&BPtr,&BSpace,"\nJob Eligibility Analysis -------\n\n");
    }

  MNodeMapInit(&RNL);

  MUSNInit(&tBPtr,&tBSpace,GlobalMsg,sizeof(GlobalMsg));

  JPC = J->TotalProcCount;

  if (!bmisset(Flags,mcmFuture) && (J->State != mjsSuspended))
    {
    enum MDependEnum DType;

    /* evaluate dynamic job state/constraint attributes */

    if (!MJOBISACTIVE(J))
      {
      if ((J->State != mjsIdle) && (J->State != mjsHold))
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%sNOTE:  job cannot run  (non-idle state %s)",
          ((DFormat != mfmXML) && (GlobalMsg[0] != '\0')) ? "\n" : " ",
          MJobState[J->State]);

        BlockCount++;
        }
      else
        {
        /* check job expected state */

        if ((J->EState != mjsIdle) &&
            (J->EState != mjsHold) &&
            (J->EState != mjsDeferred))
          {
          MUSNPrintF(&tBPtr,&tBSpace,"%sNOTE:  job cannot run  (non-idle expected state %s)",
            ((DFormat != mfmXML) && (GlobalMsg[0] != '\0')) ? "\n" : " ",
            MJobState[J->EState]);

          BlockCount++;
          }
        }    /* END else ((J->State != mjsIdle) ...) */
      }      /* END if (!MJOBISACTIVE(J)) */

    if ((J->SMinTime != 0) && (J->SMinTime > MSched.Time))
      {
      char TString[MMAX_LINE];

      MULToTString(J->SMinTime - MSched.Time,TString);

      MUSNPrintF(&tBPtr,&tBSpace,"%sNOTE:  job cannot run  (%s start date not reached for %s)",
        ((DFormat != mfmXML) && (GlobalMsg[0] != '\0')) ? "\n" : " ",
        (J->SpecSMinTime != 0) ? "user specified" : "system specified",
        TString);

      if (J->SpecSMinTime != 0)
        {
        /* user specified release date, no further analysis */

        BlockCount++;
        }
      else
        {
        /* Moab specified release date, analyze nodes */

        /* NO-OP */
        }
      }

    /* check job holds */

    if ((!bmisclear(&J->Hold)) || (J->State == mjsHold))
      {
      if (bmisset(&J->Hold,mhDefer))
        {
        /* NOTE:  do not display hold message - defer message already reported */

        IsDeferred = TRUE;
        }
      else
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%sNOTE:  job cannot run  (job has hold in place)",
          ((DFormat != mfmXML) && (GlobalMsg[0] != '\0')) ? "\n" : " ");
        }

      BlockCount++;
      }

    /* check job dependencies */

    mstring_t DValue(MMAX_LINE);

    if (MJobCheckDependency(J,&DType,FALSE,NULL,&DValue) == FAILURE)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sNOTE:  job cannot run  (dependency %s %s not met)",
       ((DFormat != mfmXML) && (GlobalMsg[0] != '\0')) ? "\n" : " ",
        DValue.c_str(),
        MDependType[DType]);

      BlockCount++;
      }

    /* check avl proc count */

    if (JPC > MPar[0].ARes.Procs)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%sNOTE:  job cannot run  (insufficient available procs:  %d available)",
        ((DFormat != mfmXML) && (GlobalMsg[0] != '\0')) ? "\n" : " ",
        MPar[0].ARes.Procs);

      BlockCount++;
      }

    /* terminate rejection info */

    MUSNPrintF(&tBPtr,&tBSpace,"%s",
      ((DFormat != mfmXML) && (GlobalMsg[0] != '\0')) ? "\n" : "");

    if (BlockCount > 0)
      {
      if ((BlockCount > 1) || (IsDeferred == FALSE))
        {
        if (DFormat != mfmXML) 
          {
          strncat(OBuf,GlobalMsg,MAX(OBufSize - strlen(OBuf),0));
          }

        /* no additional analysis required */

        if (!bmisset(Flags,mcmVerbose))
          {
          MUFree(&RNL);

          return(SUCCESS);
          }
        }
      }
    }      /* END if (!bmisset(Flags,mcmFuture) && ...)) */

  /* check static state information */

  if (J->EState == mjsDeferred)
    {
    /* temporarily change EState to allow full feasibility checking */

    IsDeferred = TRUE;
    J->EState  = J->State;
    }
  else
    {
    IsDeferred = FALSE;
    }

  if (!bmisclear(&J->Hold))
    {
    IsHeld = TRUE;
    bmcopy(&HList,&J->Hold);

    JState  = mjsIdle;
    bmclear(&J->Hold);
    }  /* END if (J->Hold != 0) */
  else
    {
    IsHeld = FALSE;
    }

  /* check global configured proc count */

  if (JPC > MPar[0].CRes.Procs)
    {
    MUSNPrintF(&BPtr,&BSpace,"NOTE:  job cannot run in any partition  (insufficient total procs:  %d available)\n",
      MPar[0].CRes.Procs);
    }

  /* preserve global job messages */

  if (DFormat == mfmXML)
    {
    if (GlobalMsg[0] != '\0')
      {
      MXMLSetAttr(JE,(char *)MJobAttr[mjaWarning],(void *)GlobalMsg,mdfString);
      }
    }
  else
    {
    /* MUSNCat(&BPtr,&BSpace,GlobalMsg); */

    /* strncat(OBuf,MsgBuf,MAX(0,OBufSize - strlen(OBuf))); */
    }

  /* NOTE:  MsgBuf used by both mfmHuman and mfmXML display formats */

  MsgBuf[0] = '\0';

  /* evaluate each partition */

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    mxml_t *PE;

    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if (DFormat == mfmXML)
      {
      MXMLCreateE(&PE,(char *)MXO[mxoPar]);
      MXMLSetAttr(PE,"name",(void *)P->Name,mdfString);
      MXMLSetAttr(PE,"eligible",(void *)MBool[FALSE],mdfString);

      MXMLAddE(DE,PE);
      }

    if (DFormat == mfmHuman)
      {
      if (MsgBuf[0] != '\0')
        {
        strncat(OBuf,MsgBuf,MAX(OBufSize - strlen(OBuf),0));
        }
      }

    MUSNInit(&BPtr,&BSpace,MsgBuf,sizeof(MsgBuf));

    if (bmisset(&J->Flags,mjfCoAlloc))
      {
      if (pindex != 0)
        continue;
      }
    else
      {
      if (pindex == 0)
        continue;
      }

    if (P->ConfigNodes == 0)
      {
      continue;
      }

    if (!strcmp(P->Name,"SHARED"))
      {
      continue;
      }

    /* can job be selected? */

    tmpLine[0] = '\0';

    if (MQueueSelectJob(J,
          mptHard,
          P,
          FALSE,  /* UpdateStats */
          FALSE,  /* TrackIdle */
          TRUE,   /* Diagnostic */
          FALSE,  /* RecordBlock */
          tmpLine) == FAILURE)
      {
      if (!(MJOBWASMIGRATEDTOREMOTEMOAB(J)))
        MUSNPrintF(&BPtr,&BSpace,"NOTE:  job violates constraints for partition %s (%s)\n\n",
          P->Name,
          tmpLine);

      continue;
      }

    MOQueueInitialize(SrcQ);

    SrcQ[0] = J;
    SrcQ[1] = NULL;

    if ((MQueueSelectJobs(
          SrcQ,
          DstQ,
          mptHard,
          MSched.JobMaxNodeCount,
          MSched.JobMaxTaskCount,
          MMAX_TIME,
          P->Index,
          RCount,
          FALSE,
          FALSE,  /* do not update stats */
          FALSE,  /* do not record block */
          FALSE,
          FALSE) == FAILURE) || (DstQ[0] == NULL))
      {
      for (rindex = 0;rindex < marLAST;rindex++)
        {
        if (RCount[rindex] != 0)
          break;
        }

      MUSNPrintF(&BPtr,&BSpace,"NOTE:  job does not meet specific constraints for partition %s (%s)\n\n",
        P->Name,
        (rindex != marLAST) ? MAllocRejType[rindex] : "UNKNOWN");

      continue;
      }  /* END if ((MQueueSelectJobs() == FAILURE) || ...) */

    /* job selected */

    /* check policies */

    /* NOTE:  idle slots already filled previously - do not recheck idle policies */

    /* NOTE:  if job is blocked by idle policy, check all limits/policies, otherwise, 
              check only active and system limits */

    bmclear(&BM);

    if (MJobGetBlockReason(J,P) != mjneIdlePolicy)
      {
      bmset(&BM,mlActive);
      bmset(&BM,mlSystem);
      }

    if ((IgnPolicies == FALSE) &&
        (MJobCheckPolicies(
          J,
          PLevel,
          &BM,   /* limit types to check */
          P,
          &PReason,
          tmpLine,
          MSched.Time) == FAILURE))
      {
      MUSNPrintF(&BPtr,&BSpace,"NOTE:  job cannot run in partition %s (%s)\n",
        P->Name,
        tmpLine);

      continue;
      }

    /* NOTE:  only check class availability for primary req */

    if ((J->Credential.C != NULL) && !bmisset(&P->Classes,J->Credential.C->Index))
      {
      MUSNPrintF(&BPtr,&BSpace,"NOTE:  required class not configured in partition %s\n",
        P->Name);

      continue;
      }

    /* preserve partition level job messages */

    if (DFormat == mfmXML)
      {
      if (MsgBuf[0] != '\0')
        {
        MXMLAppendAttr(JE,(char *)MJobAttr[mjaWarning],MsgBuf,',');
        }
      }

    if (J->Req[1] != NULL)
      IsMultiReq = TRUE;
    else
      IsMultiReq = FALSE;

    /* check per req constraints */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      ProvMayHelp = FALSE;

      if (DiagnoseReqConstraints == FALSE)
        break;

      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      /* check per node availability */

      ncount = 0;
      pcount = 0;
      tcount = 0;

      memset(RCount,0,sizeof(RCount));

      IProcs = 0;

      /* check per node requirements */

      if (bmisset(Flags,mcmVerbose))
        {
        /* display detailed rejection reasons */

        if (DFormat == mfmHuman)
          {
          if (IsMultiReq == FALSE)
            {
            MUSNPrintF(&BPtr,&BSpace,"Node Availability for Partition %s --------\n\n%s",
              P->Name,
              GlobalMsg);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"Node Availability for Req %d in Partition %s --------\n\n%s",
              RQ->Index,
              P->Name,
              GlobalMsg);
            }
          }
        }    /* END if (bmisset(Flags,mcmVerbose)) */

      if (RQ->DRes.Procs == 0)
        {
        mbool_t MatchFound = FALSE;

        mpar_t *SharedP = NULL;

        /* assume use of global/shared node */

        if (MSched.GN != NULL)
          {
          N = MSched.GN;
  
          MatchFound = TRUE;
 
          /* NOTE:  pass in IgnNodeState and IgnRsv (NYI) */
   
          MNodeDiagnoseEligibility(
            N,
            J,
            RQ,
            &RIndex,  /* O */
            tmpLine,  /* O */
            (bmisset(Flags,mcmFuture)) ? MMAX_TIME : MSched.Time,
            &TC,      /* O */
            &IProcs);
   
          if (RIndex != marNONE)
            {
            RCount[RIndex]++;
   
            if (bmisset(Flags,mcmVerbose) || (SN != NULL))
              {
              if (DFormat == mfmXML)
                {
                NE = NULL;
 
                MXMLCreateE(&NE,(char *)MXO[mxoNode]);

                MXMLSetAttr(
                  NE,
                  (char *)MNodeAttr[mnaOldMessages],
                  (void *)MAllocRejType[RIndex],
                  mdfString);

                MXMLAddE(CE,NE);
                }
              else
                {
                if ((IgnBM == NULL) || !bmisset(IgnBM,RIndex))
                  {
                  /* ignore list means do not display verbose details about 
                     associated rejection */

                  if (bmisset(Flags,mcmVerbose2) || (SN != NULL))
                    {
                    double PVal;

                    /* display allocation priority */

                    MNodeGetPriority(
                      N,
                      J,
                      RQ,
                      NULL,
                      0,
                      FALSE,
                      &PVal,        /* O */
                      MSched.Time,
                      NULL);

                    sprintf(tmpPLine,"  allocationpriority=%.2f",
                      PVal);
                    }
                  else
                    {
                    tmpPLine[0] = '\0';
                    }

                  if ((MSched.ProvRM != NULL) &&
                     ((RIndex == marDynCfg) || (RIndex == marPolicy)))
                    {
                    ProvMayHelp = TRUE; 
                    }

                  if (tmpLine[0] != '\0')
                    {
                    MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s (%s)%s\n",
                      N->Name,
                      MAllocRejType[RIndex],
                      tmpLine,
                      tmpPLine);
                    }
                  else
                    {
                    MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s%s\n",
                      N->Name,
                      MAllocRejType[RIndex],
                      tmpPLine);
                    }
                  }    /* END if ((IgnBM == NULL) || !bmisset(IgnBM,RIndex)) */
                }      /* END else (DFormat == mfmXML) */
              }        /* END if (bmisset(Flags,mcmVerbose) || ...) */
            }          /* END if (RIndex != marNONE) */
          else
            {
            if (DFormat != mfmXML)
              {
              /* NOTE:  move to range display (NYI) */

              MUSNPrintF(&BPtr,&BSpace,"%-24s available: %d tasks supported\n",
                N->Name,
                TC);
              }
            }
          }     /* END if (MSched.GN != NULL) */
        else if (MParFind("SHARED",&SharedP) == SUCCESS)
          {
          /* NYI:  loop through nodes in SHARED partition */

          for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
            {
            N = MNode[nindex];
     
            if ((N == NULL) || (N->Name[0] == '\0'))
              break;
     
            if (N->Name[0] == '\1')
              continue;
     
            if (N->PtIndex != SharedP->Index)
              continue;
     
            if ((SN != NULL) && (SN != N))
              {
              continue;
              }

            MatchFound = TRUE;

            MNodeDiagnoseEligibility(
              N,
              J,
              RQ,
              &RIndex,  /* O */
              tmpLine,  /* O */
              (bmisset(Flags,mcmFuture)) ? MMAX_TIME : MSched.Time,
              &TC,
              &IProcs);
     
            if (RIndex != marNONE)
              {
              RCount[RIndex]++;
     
              if (bmisset(Flags,mcmVerbose) || (SN != NULL))
                {
                if (DFormat == mfmXML)
                  {
                  NE = NULL;
 
                  MXMLCreateE(&NE,(char *)MXO[mxoNode]);

                  MXMLSetAttr(
                    NE,
                    (char *)MNodeAttr[mnaOldMessages],
                    (void *)MAllocRejType[RIndex],
                    mdfString);

                  MXMLAddE(CE,NE);
                  }
                else
                  {
                  if ((IgnBM == NULL) || !bmisset(IgnBM,RIndex))
                    {
                    if (tmpLine[0] != '\0')
                      {
                      MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s (%s)\n",
                        N->Name,
                        MAllocRejType[RIndex],
                        tmpLine);
                      }
                    else
                      {
                      MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s\n",
                        N->Name,
                        MAllocRejType[RIndex]);
                      }
                    }
                  }
                }
              }        /* END if (RIndex != marNONE) */
            else
              {
              if (DFormat != mfmXML)
                {
                /* move to range display (NYI) */

                MUSNPrintF(&BPtr,&BSpace,"%-24s available: %d tasks supported\n",
                  N->Name,
                  TC);
                }
              }
            }    /* END for (nindex) */
          }      /* END else if (MParFind("SHARED",&SharedP) == SUCCESS) */
        else
          {
          /* loop through nodes with no procs associated */

          for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
            {
            N = MNode[nindex];

            if ((N == NULL) || (N->Name[0] == '\0'))
              break;

            if (N->Name[0] == '\1')
              continue;

            if (N->CRes.Procs > 0)
              continue;

            if ((SN != NULL) && (SN != N))
              {
              continue;
              }

            MatchFound = TRUE;

            MNodeDiagnoseEligibility(
              N,
              J,
              RQ,
              &RIndex,  /* O */
              tmpLine,  /* O */
              (bmisset(Flags,mcmFuture)) ? MMAX_TIME : MSched.Time,
              &TC,
              &IProcs);

            if (RIndex != marNONE)
              {
              RCount[RIndex]++;

              if (bmisset(Flags,mcmVerbose) || (SN != NULL))
                {
                if (DFormat == mfmXML)
                  {
                  NE = NULL;
 
                  MXMLCreateE(&NE,(char *)MXO[mxoNode]);

                  MXMLSetAttr(
                    NE,
                    (char *)MNodeAttr[mnaOldMessages],
                    (void *)MAllocRejType[RIndex],
                    mdfString);

                  MXMLAddE(CE,NE);
                  }
                else
                  {
                  if ((IgnBM == NULL) || !bmisset(IgnBM,RIndex))
                    {
                    if (bmisset(Flags,mcmSummary))
                      {
                      RNL[N->Index] = RIndex;
                      }
                    else if (tmpLine[0] != '\0')
                      {
                      MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s (%s)\n",
                        N->Name,
                        MAllocRejType[RIndex],
                        tmpLine);
                      }
                    else
                      {
                      MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s\n",
                        N->Name,
                        MAllocRejType[RIndex]);
                      }
                    }
                  }
                }
              }        /* END if (RIndex != marNONE) */
            else
              {
              if (DFormat != mfmXML)
                {
                if (bmisset(Flags,mcmSummary))
                  {
                  roffset = MIN(MMAX_TAPERNODE,marLAST + TC);

                  MaxTC = MAX(MaxTC,TC);

                  RCount[roffset]++;

                  RNL[N->Index] = roffset;
                  }
                else
                  {
                  MUSNPrintF(&BPtr,&BSpace,"%-24s available: %d tasks supported\n",
                    N->Name,
                    TC);
                  }
                }
              }
            }    /* END for (nindex) */

          if (bmisset(Flags,mcmSummary))
            {
            mstring_t RBuffer(MMAX_BUFFER);

            for (rindex = marLAST + 1;rindex <= MIN(MMAX_TAPERNODE,marLAST + MaxTC);rindex++)
              {
              if (RCount[rindex] == 0)
                continue;

              RBuffer.clear();  /* Reset string */
    
              MUNLToRangeString(
                NULL,
                RNL,     /* NOTE:  RNL can only handle up to 200 tasks per node (FIXME) */
                rindex,
                FALSE,
                TRUE,
                &RBuffer);
    
              MUSNPrintF(&BPtr,&BSpace,"available for %d task%s     - %s\n",
                rindex - marLAST,
                (rindex - marLAST != 1) ? "s" : " ",
                RBuffer.c_str());
              }  /* END for (rindex) */

            for (rindex = 0;rindex < marLAST;rindex++)
              {
              if (RCount[rindex] == 0)
                continue;
            
              RBuffer.clear();  /* Reset string */
    
              MUNLToRangeString(
                NULL,
                RNL,
                rindex,
                FALSE,
                TRUE,
                &RBuffer);
    
              MUSNPrintF(&BPtr,&BSpace,"rejected for %-12s - %s\n",
                MAllocRejType[rindex],
                RBuffer.c_str());
              }  /* END for (rindex) */
            }    /* END if (bmisset(Flags,mcmSummary)) */
          }      /* END else (MParFind("SHARED",&SharedP) == SUCCESS) */

        if (MatchFound == FALSE)
          {
          MUSNPrintF(&BPtr,&BSpace,"NOTE:  no matching nodes located\n");
          }

        continue;
        }  /* END if (RQ->DRes.Procs == 0) */

      memset(RNL,0,sizeof(char) * MSched.M[mxoNode]);

      for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
        {
        N = MNode[nindex];

        if ((N == NULL) || (N->Name[0] == '\0'))
          break;

        if (N->Name[0] == '\1')
          continue;

        if (N->PtIndex != pindex)
          continue;
 
        if ((SN != NULL) && (SN != N))
          {
          continue;
          }

        if (bmisset(Flags,mcmVerbose) || (SN != NULL))
          {
          if (DFormat == mfmXML)
            {
            NE = NULL;
 
            MXMLCreateE(&NE,(char *)MXO[mxoNode]);

            MXMLSetAttr(
              NE,
              (char *)MNodeAttr[mnaNodeID],
              (void *)N->Name,
              mdfString);

            MXMLAddE(CE,NE);
            }
          }

        MNodeDiagnoseEligibility(
          N,
          J,
          RQ,
          &RIndex, /* O */
          tmpLine, /* O */
          (bmisset(Flags,mcmFuture)) ? MMAX_TIME : MSched.Time,      
          &TC,
          &IProcs);

        /* ignore list means do not display verbose details about
           associated rejection */

        if ((DFormat != mfmXML) &&
           (bmisset(Flags,mcmVerbose2) || (SN != NULL)))
          {
          double PVal;

          /* display allocation priority */

          MNodeGetPriority(
            N,
            J,
            RQ,
            NULL,
            0,
            FALSE,
            &PVal,        /* O */
            MSched.Time,
            NULL);

          sprintf(tmpPLine,"  allocationpriority=%.2f",
            PVal);
          }
        else
          {
          tmpPLine[0] = '\0';
          }

        if (RIndex != marNONE)
          {
          RCount[RIndex]++;

          if (bmisset(Flags,mcmVerbose) || (SN != NULL))
            {
            if (DFormat == mfmXML)
              {
              MXMLSetAttr(
                NE,
                (char *)MNodeAttr[mnaOldMessages],
                (void *)MAllocRejType[RIndex],
                mdfString);
              }
            else
              {
              if ((IgnBM == NULL) || !bmisset(IgnBM,RIndex))
                {
                if (bmisset(Flags,mcmSummary))
                  {
                  RNL[N->Index] = RIndex;
                  }
                else if (tmpLine[0] != '\0')
                  {
                  MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s (%s)%s\n",
                    N->Name,
                    MAllocRejType[RIndex],
                    tmpLine,
                    tmpPLine);
                  }
                else
                  {
                  MUSNPrintF(&BPtr,&BSpace,"%-24s rejected: %s%s\n",
                    N->Name,
                    MAllocRejType[RIndex],
                    tmpPLine);
                  }
                }
              }
            }
          }    /* END if (RIndex != marNONE) */
        else
          {
          /* node is available */

          if (DFormat != mfmXML)
            {
            if (bmisset(Flags,mcmSummary))
              {
              roffset = MIN(MMAX_TAPERNODE,marLAST + TC);
             
              MaxTC = MAX(MaxTC,TC);

              RCount[roffset]++;

              RNL[N->Index] = roffset;
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"%-24s available: %d tasks supported%s\n",
                N->Name,
                TC,
                tmpPLine);
              }
            }
          }

        if (TC > 0)
          {
          ncount++;
          tcount += TC;
          }
        }      /* END for (nindex) */

      if (bmisset(Flags,mcmSummary))
        {
        mstring_t RBuffer(MMAX_BUFFER);

        for (rindex = marLAST + 1;rindex <= MIN(MMAX_TAPERNODE,marLAST + MaxTC);rindex++)
          {
          if (RCount[rindex] == 0)
            continue;

          RBuffer.clear();  /* reset striing */

          MUNLToRangeString(
            NULL,
            RNL,     /* NOTE:  RNL can only handle up to 200 tasks per node (FIXME) */
            rindex,
            FALSE,
            TRUE,
            &RBuffer);

          MUSNPrintF(&BPtr,&BSpace,"available for %d task%s     - %s\n",
            rindex - marLAST,
            (rindex - marLAST != 1) ? "s" : " ",
            RBuffer.c_str());
          }  /* END for (rindex) */

        for (rindex = 0;rindex < marLAST;rindex++)
          {
          if (RCount[rindex] == 0)
            continue;

          RBuffer.clear();  /* reset striing */

          MUNLToRangeString(
            NULL,
            RNL,
            rindex,
            FALSE,
            TRUE,
            &RBuffer);

          MUSNPrintF(&BPtr,&BSpace,"rejected for %-12s - %s\n",
            MAllocRejType[rindex],
            RBuffer.c_str());
          }  /* END for (rindex) */
        }    /* END if (bmisset(Flags,mcmSummary)) */

      if (RQ->DRes.Procs != -1)
        pcount = tcount * RQ->DRes.Procs;
      else if (MNode[0] != NULL)
        pcount = tcount * MNode[0]->CRes.Procs;
      else 
        pcount = tcount;

      if ((RQ->DRes.Procs != 0) && 
          (!bmisset(Flags,mcmFuture)) && 
          ((long)P->ARes.Procs < JPC))
        {
        if (DFormat != mfmXML)
          {
          MUSNPrintF(&BPtr,&BSpace,"NOTE:  job req cannot run in partition %s (insufficient procs available: %d < %d)\n\n",
            P->Name,
            P->ARes.Procs,
            JPC);
          }
        }
      else if ((RQ->DRes.Procs != 0) && (pcount < RQ->DRes.Procs * RQ->TaskCount))
        {
        if (DFormat == mfmHuman)
          {
          MUSNPrintF(&BPtr,&BSpace,"NOTE:  job req cannot run in partition %s (available procs do not meet requirements : %d of %d procs found)\n",
            P->Name,
            pcount,
            RQ->DRes.Procs * RQ->TaskCount);

          MUSNPrintF(&BPtr,&BSpace,"idle procs: %3d  feasible procs: %3d\n",
            IProcs,
            pcount);

          /* check for incompatibility */
          if ((pcount == 0) && (J->ReqRID != NULL))
            {
            msrsv_t *SR;
            int StrSize;
            char const *Context;
            char const *Delims = ".";

            /* test to see if we have the standing reservation name and correct if we just have the rsv name*/
            MUStrScan(J->ReqRID,Delims,&StrSize,&Context);
            if (Context != NULL && StrSize != 0)
              J->ReqRID[StrSize]= '\0';

            if (MSRFind(J->ReqRID,&SR,NULL) == SUCCESS)
              {
              if ((SR->RollbackOffset > 0) && (bmisset(&J->Flags,mjfAdvRsv)))
                {
                if (DFormat == mfmHuman)
                  {
                  MUSNPrintF(&BPtr,&BSpace,"WARNING:   ADVRES flag on job is incompatible with ROLLBACKOFFSET on standing reservation\n"); 
                  }
                }
              }
            }

          rcount = 0;

          for (rindex = 0;rindex < marLAST;rindex++)
            {
            if (RCount[rindex] == 0)
              continue;

            if (!(rcount % 5))
              {
              if (J->Req[1] != NULL)
                {
                /* job is multi-req */

                MUSNPrintF(&BPtr,&BSpace,"\nReq %d Node Rejection Summary: ",
                  RQ->Index);
                }
              else
                {
                MUSNCat(&BPtr,&BSpace,"\nNode Rejection Summary: ");
                }
              }

            MUSNPrintF(&BPtr,&BSpace,"[%s: %d]",
              MAllocRejType[rindex],
              RCount[rindex]);

          rcount++;
          }  /* END for (rindex) */

          MUSNCat(&BPtr,&BSpace,"\n\n");
          }  /* END if (DFormat == mfmHuman) */
        }    /* END else if (pcount < JPC) */
      else if ((J->Request.NC > 0) && (ncount < J->Request.NC))
        {
        if (DFormat == mfmHuman)
          {
          MUSNPrintF(&BPtr,&BSpace,"NOTE:  job cannot run in partition %s (insufficient idle nodes available: %d < %d)\n\n",
            P->Name,
            ncount,
            J->Request.NC);
          }
        }
      else if (BlockCount == 0)
        {
        if (DFormat == mfmHuman)
          {
          if ((RQ->DRes.Procs != 0) && (pcount <= 0))
            {
            MUSNPrintF(&BPtr,&BSpace,"NOTE:  job req cannot run in partition %s (insufficient procs available: %d < %d)\n\n",
              P->Name,
              pcount,
              RQ->DRes.Procs * RQ->TaskCount);

            if (ProvMayHelp == TRUE)
              {
              if ((J->Credential.Q == NULL) || 
                  !bmisset(&J->Credential.Q->Flags,mqfProvision))
                {
                MUSNPrintF(&BPtr,&BSpace,"NOTE:  provisioning may be possible but job does not possess 'prov' QOS flag\n");
                }
              else if (MSched.TwoStageProvisioningEnabled == FALSE)
                {
                MUSNPrintF(&BPtr,&BSpace,"NOTE:  provisioning may be possible but two-stage provisioning is disabled\n");
                }
              }
            }
          else
            {
            if (IsMultiReq == FALSE)
              {
              MUSNPrintF(&BPtr,&BSpace,"NOTE:  job can run in partition %s (%d procs available  %d procs required)\n",
                P->Name,
                pcount,
                JPC);
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"NOTE:  req %d can run in partition %s (%d tasks available, %d tasks required)\n\n",
                RQ->Index,
                P->Name,
                tcount,
                RQ->TaskCount);
              }
            }

          if ((P->RM != NULL) &&
              (P->RM->State == mrmsDisabled))
            {
            MUSNPrintF(&BPtr,&BSpace,"NOTE:  RM '%s' (partition '%s') is disabled\n",
              P->RM->Name,
              P->Name);
            }
          }    /* END else if (DFormat == mfmHuman) */
        }  /* END else if (BlockCount == 0) */
      }    /* END for (rqindex) */

    if (DFormat == mfmXML)
      {
      MXMLSetAttr(PE,"eligible",(void *)MBool[TRUE],mdfString);
      }
    }      /* END for (pindex) */

  if (DFormat == mfmHuman)
    {
    if (MsgBuf[0] != '\0')
      strncat(OBuf,MsgBuf,MAX(OBufSize - strlen(OBuf),0));
    }

  /* restore job */

  if (IsDeferred == TRUE)
    {
    J->EState = mjsDeferred;
    }

  if (IsHeld == TRUE)
    {
    /* restore job attributes */

    bmcopy(&J->Hold,&HList);

    J->State = JState;
    }

  MUFree(&RNL);

  return(SUCCESS);
  }  /* END MJobDiagnoseEligibility() */

/* END MJobDiagnose.c */
