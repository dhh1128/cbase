/* HEADER */

/**
 * @file MCluster.c
 *
 * Moab Cluster
 */

/* Contains:                                 *
 *  int MClusterLoadConfig(CName,Buf)        *
 *  int MClusterProcessConfig(C,Value)       *
 *  int MClusterUpdateNodeState()            *
 *  int MClusterShowARes()                   *
 *                                           */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/* MMAX_REQS must be less than or equal to MMAX_REQ_PER_JOB */

#define MMAX_REQS MMAX_REQ_PER_JOB


/* internal prototypes */

int __MClusterNToXML(mnode_t *,mxml_t **,mxml_t *);
int __MClusterRangeToXML(mxml_t *,char *,mvpcp_t *,long,long,int,int,int,mbitmap_t *,char *,char *,char *,char *,char *,char *,char *,mcres_t *,int,char *,mbitmap_t *,mbool_t,int *,mxml_t **);
int __MClusterGetCoAllocConstraints(mpar_t *,int C[][MMAX_REQS + 1],long *,mrange_t *,mjob_t **,mreq_t **,mrange_t D[][MMAX_RANGE],mnl_t **,mbool_t,char *);

/* END internal prototypes */




/**
 * Load VPC profile-specific config parameters.
 *
 * @see MClusterProcessConfig() - child
 *
 * NOTE:  Load 'VCPROFILE[]' parameter.
 *
 * @param CName (I) [optional]
 * @param Buf (I) [optional]
 */

int MClusterLoadConfig(

  char *CName,  /* I name (optional) */
  char *Buf)    /* I config buffer (optional) */

  {
  char  IndexName[MMAX_NAME];
  char  Value[MMAX_LINE];

  char *ptr;

  mvpcp_t *C;

  if (MSched.ConfigBuffer == NULL)
    {
    return(FAILURE);
    }

  ptr = (Buf != NULL) ? Buf : MSched.ConfigBuffer;

  if ((CName == NULL) || (CName[0] == '\0'))
    {
    /* load ALL cluster info */

    IndexName[0] = '\0';

    while (MCfgGetSVal(
             MSched.ConfigBuffer,
             &ptr,
             MCredCfgParm[mxoCluster],
             IndexName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) != FAILURE)
      {
      if (MVPCProfileFind(IndexName,&C) == FAILURE)
        {
        if (MVPCProfileAdd(IndexName,&C) == FAILURE)
          {
          /* unable to locate/create vc */

          IndexName[0] = '\0';

          continue;
          }
        }

      MClusterProcessConfig(C,Value);

      IndexName[0] = '\0';
      }  /* END while (MCfgGetSVal() != FAILURE) */
    }    /* END if ((CName == NULL) || (CName[0] == '\0')) */

  return(SUCCESS);
  }  /* END MClusterLoadConfig() */





/**
 * Process cluster-specific (VPC profile) config parameters.
 *
 * NOTE:  Process 'VCPROFILE[]' parameter.
 *
 * @see MClusterLoadConfig() - parent
 * @see MVPCProfileToString() - peer - show VPC profile attributes
 * @see MVPCPAttr[]
 *
 * @param C (I)
 * @param Value (I)
 */

int MClusterProcessConfig(

  mvpcp_t *C,      /* I */
  char    *Value)  /* I */

  {
  int  aindex;

  enum MVPCPAttrEnum AIndex;

  char *ptr;
  char *TokPtr;

  char  ValLine[MMAX_LINE];

  const char *FName = "MClusterProcessConfig";

  MDB(3,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (C != NULL) ? C->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((C == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  /* process vlaue line */

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    if (MUGetPair(
          ptr,
          (const char **)MVPCPAttr,
          &aindex,
          NULL,
          TRUE,
          NULL,
          ValLine,
          sizeof(ValLine)) == FAILURE)
      {
      /* cannot parse value pair */

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    AIndex = (enum MVPCPAttrEnum)aindex;

    switch (AIndex)
      {
      case mvpcpaACL:

        /* list of who can view/utilize this profile */

        MACLLoadConfigLine(&C->ACL,ValLine,mSet,NULL,FALSE);

        break;

      case mvpcpaDescription:

        MUStrDup(&C->Description,ValLine);

        break;

      case mvpcpaFlags:

        /* flags affecting configuration of VPC */

        bmfromstring(ValLine,MVPCPFlags,&C->Flags);

        break;

      case mvpcpaName:

        MUStrCpy(C->Name,ValLine,sizeof(C->Name));

        break;

      case mvpcpaNodeHourChargeRate:

        C->NodeHourAllocChargeRate = (double)strtod(ValLine,NULL);

        break;

      case mvpcpaNodeSetupCost:

        C->NodeSetupCost = (double)strtod(ValLine,NULL);

        break;

      case mvpcpaPurgeDuration:

        C->PurgeDuration = MUTimeFromString(ValLine);

        break;

      case mvpcpaReallocPolicy:

        C->ReAllocPolicy = (enum MRsvAllocPolicyEnum)MUGetIndexCI(
          ValLine,
          MRsvAllocPolicy,
          FALSE,
          0);

        break;

      case mvpcpaOpRsvProfile:

        {
        int cindex;

        MUStrDup(&C->OpRsvProfile,ValLine);

        for (cindex = 0;C->OpRsvProfile[cindex] != '\0';cindex++)
          {
          if (!isspace(C->OpRsvProfile[cindex]) && (C->OpRsvProfile[cindex] != '.'))
            C->OpRsvProfile[cindex] = C->OpRsvProfile[cindex];
          else
            C->OpRsvProfile[cindex] = '_';
          }  /* END for (cindex) */
        }

        break;

      case mvpcpaPriority:

        C->Priority= strtol((char *)ValLine,NULL,10);

        break;

      case mvpcpaCoAlloc:
      case mvpcpaQueryData:

        {
        char  tmpName[MMAX_NAME];
        int   index;

        char *ptr;
        char *vptr;

        char *TokPtr;

        mln_t *tmpL;

        mxml_t *E;

        tmpL = C->QueryData;

        /* determine number of query data items previously processed */

        index = 0;

        while (tmpL != NULL)
          {
          tmpL = tmpL->Next;

          index++;
          }

        tmpL = NULL;
        E    = NULL;

        tmpName[0] = '\0';

        MXMLCreateE(&E,(char *)MSON[msonSet]);

        /* FORMAT:  <ATTR>=<VAL>[,<ATTR>=<VAL>]... */

        ptr = MUStrTok(ValLine,"=,",&TokPtr);

        while (ptr != NULL)
          {
          vptr = MUStrTok(NULL,"=,",&TokPtr);

          MS3AddWhere(E,ptr,vptr,NULL);

          if (!strcasecmp(ptr,"label"))
            {
            MUStrCpy(tmpName,vptr,sizeof(tmpName));
            }

          ptr = MUStrTok(NULL,"=,",&TokPtr);
          }  /* END while (ptr != NULL) */

        if (tmpName[0] == '\0')
          {
          snprintf(tmpName,sizeof(tmpName),"%s.%d",
            C->Name,
            index);
          }

        MULLAdd(&C->QueryData,tmpName,NULL,&tmpL,MUFREE);

        tmpL->Ptr = (void *)E;
        }    /* END BLOCK mvcapQueryString */

        break;

      case mvpcpaReqEndPad:

        C->ReqEndPad = MUTimeFromString(ValLine);

        break;

      case mvpcpaReqStartPad:

        C->ReqStartPad = MUTimeFromString(ValLine);

        break;

      case mvpcpaReqDefAttr:

        MUStrDup(&C->ReqDefAttrs,ValLine);

        break;

      case mvpcpaReqSetAttr:

        MUStrDup(&C->ReqSetAttrs,ValLine);
 
        break;

      default:

        MDB(4,fSTRUCT) MLog("WARNING:  cluster attribute '%s' not handled\n",
          MVPCPAttr[AIndex]);

        break;
      }  /* END switch (AIndex) */

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MClusterProcessConfig() */





/**
 * Update node-state after all RM info is loaded.
 *
 * called after all RM-specific and RM-general node information is loaded
 * called after all RM-specific and RM-general job information is loaded
 *
 * NOTE: this routine is used to make node state adjustments which require 
 *   complete synchronized node+job info
 *
 * set node dedicated resources and joblist 
 *
 * @see MNodeAdjustAvailResources() - child - to adjust available resources 
 * @see MNodeUpdateResExpression() - child - to update rsv and class status 
 * @see MNodeCheckStatus() - peer - called after MSchedProcessJobs() 
 * @see MJobAddToNL() - child
 */

int MClusterUpdateNodeState(void)

  {
  job_iter JTI;

  int nindex;
  int rqindex;
  int cindex;
 
  mreq_t *RQ;
  mjob_t *J;
  mclass_t *C;
 
  int  *TotalTaskDed = NULL;
  int  *TotalProcDed = NULL;
 
  double *TotalProcUtl = NULL;

  mbool_t *JobCPUTrackingIsDisabled = NULL;
 
  double tmpAvgProcUsage;

  mnode_t *N;

  int      TC;
  int      OAPC;

  mbool_t  SystemJobAdjustState = FALSE;

  const char *FName = "MClusterUpdateNodeState";
 
  MDB(3,fSTAT) MLog("%s()\n",
    FName);
 
  TotalTaskDed = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);
  TotalProcDed = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);
  TotalProcUtl = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoNode]);
  JobCPUTrackingIsDisabled = (mbool_t *)MUCalloc(1,sizeof(mbool_t) * MSched.M[mxoNode]);
 
  MJobIterInit(&JTI);

  /* add procs of all active jobs (newly updated or not) */
 
  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (MJOBISACTIVE(J) && !bmisset(&J->Flags,mjfMeta))
      {
      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        /* RQ->URes.Procs = % of processor usage * RQ->TC */

        TC = MAX(RQ->TaskRequestList[0],RQ->TaskCount);
        TC = MAX(1,TC);

        tmpAvgProcUsage = (RQ->URes != NULL) ? (RQ->URes->Procs / TC) / 100.0 : 0;
 
        for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          TotalTaskDed[N->Index] += MNLGetTCAtIndex(&RQ->NodeList,nindex);
          TotalProcDed[N->Index] += MNLGetTCAtIndex(&RQ->NodeList,nindex) * ((RQ->DRes.Procs >= 0) ?
                                      RQ->DRes.Procs : N->CRes.Procs);

          if (MPar[0].NAvailPolicy[mrProc] == mrapUtilized)
            {
            /* don't set TotalProcUtl based on job reqs, use CPULoad for node */

            continue;
            }

          if (tmpAvgProcUsage == 0)
            {
            /* jobs with 0 cput (launched via mpirun?) or cpu usage not being tracked */
            /* sync URes with DRes to prevent loss of available resources in */
            /* MNodeAdjustAvailResources() */

            TotalProcUtl[N->Index] = MAX(
              TotalProcUtl[N->Index],
              TotalProcDed[N->Index]);          
            }
          else
            {
            TotalProcUtl[N->Index] += MNLGetTCAtIndex(&RQ->NodeList,nindex) * tmpAvgProcUsage;
            }
          } /* END for (nindex)   */
        }   /* END for (rqindex)  */

      if (bmisset(&J->Flags,mjfSystemJob))
        SystemJobAdjustState = TRUE;
      }     /* END if (MJOBISACTIVE(J))  */
    }       /* END for (J = MJob[0]->Next;(J != NULL) && (J != MJob[0]);J = J->Next) */

  /* clear class capacity */

  for (cindex = MCLASS_FIRST_INDEX;cindex < MSched.M[mxoClass];cindex++)
    {
    C = &MClass[cindex];

    if ((C == NULL) || (C->Name[0] == '\0'))
      break;

    if (C->Name[0] == '\1')
      continue;

    C->ConfiguredProcs = 0;
    C->ConfiguredNodes = 0;
    }  /* END for (cindex) */

  /* Clear SMP information */

  MSMPNodeResetAllStats();

  /* update node attributes */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    mln_t *Link;

    N = MNode[nindex];
 
    if ((N == NULL) || (N->Name[0] == '\0'))
      break;
 
    if (N->Name[0] == '\1')
      continue;

    MNodeUpdateAttrList(N);

    /* MNodeProcessRMMsg() moved from MClusterUpdateNodeState() to 
       incorporate information from RMMsg in node state decisions */

    OAPC = N->ARes.Procs;
 
    /* reset job info */
 
    N->JList[0] = NULL; 

    for (Link = N->VMList;Link != NULL;Link = Link->Next)
      {
      mvm_t *VM = (mvm_t *)Link->Ptr;

      if ((VM->JobID[0] != 0) && (MJobFind(VM->JobID,NULL,mjsmBasic) == FAILURE))
        memset(VM->JobID,0,sizeof(VM->JobID));
      }

    if (MPar[0].NAvailPolicy[0] == mrapUtilized)
      {
      /* set TotalProcUtl based on CPULoad for each node */

      TotalProcUtl[N->Index] = N->Load;
      }

    MNodeAdjustAvailResources(
      N,
      TotalProcUtl[N->Index],
      TotalProcDed[N->Index],
      TotalTaskDed[N->Index]);

    /* transition node after adjusting */

    MNodeTransition(N);

    /* connect to existing classes and reservations */

    /* each RM may provide partial resource info, so must wait         */
    /* until last node RM is loaded before adding node to rsv or class */
    /* NOTE:  adding to class requires N->ARes.Procs to be populated   */

    if (N->ARes.Procs != OAPC) 
      {
      /* connect to existing classes and reservations */

      /* each RM may provide partial resource info, so must wait         */
      /* until last node RM is loaded before adding node to rsv or class */
      /* NOTE:  adding to class requires N->ARes.Procs to be populated   */

      MNodeUpdateResExpression(N,FALSE,TRUE);  /* do update class */
      }

      for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
        {
        C = &MClass[cindex];

        if (bmisset(&N->Classes,C->Index))
          {
          C->ConfiguredProcs += N->CRes.Procs;
          C->ConfiguredNodes++;
          }
        }
    
    if ((N->NextOS != 0) && 
        ((N->OSList != NULL) || (MSched.DefaultN.OSList != NULL)))
      {
      mnodea_t* OSList = ( (N->OSList != NULL) ? N->OSList : MSched.DefaultN.OSList );
      
      if (N->NextOS == N->ActiveOS)
        {
        int OIndex;
        char TString[MMAX_LINE];

        for (OIndex = 0;OIndex < MMAX_NODEOSTYPE;OIndex++)
          {
          if (OSList[OIndex].AIndex == 0)
            {
            /* end of list */
 
            break;
            }

          if (OSList[OIndex].AIndex == N->NextOS)
            {
            /* matching OS found - update provduration estimate */

            /* NOTE: if the OS we're looking at is on the DEFAULT node then this will
             * change the config time and affect other nodes - is this a problem? */

            if (OSList[OIndex].CfgTime == 0)
              OSList[OIndex].CfgTime = MAX(1,MSched.Time - N->LastOSModRequestTime);

            break;
            }
          }    /* END for (OIndex) */

        MULToTString(MSched.Time - N->LastOSModRequestTime,TString);

        MDB(4,fPBS) MLog("INFO:     provisioning to OS '%s' complete for node %s - elapsed time: %s\n",
          MAList[meOpsys][N->NextOS],
          N->Name,
          TString);

        N->NextOS = 0;
        }
      }    /* END if ((N->NextOS != 0) && (OSList != NULL)) */

    if ((bmisset(&MPar[N->PtIndex].Flags,mpfSharedMem)) &&
        (MPar[0].NodeSetPriorityType == mrspMinLoss))
      MSMPNodeUpdate(N);
 
    MDB(6,fPBS) MLog("INFO:     node %s C/A/D procs:  %d/%d/%d\n",
      N->Name,
      N->CRes.Procs,
      N->ARes.Procs,
      N->DRes.Procs);
    }    /* END for (nindex) */

  MDB(7,fSCHED) MSMPPrintAll();

  /* associate job to node - may have dependency on node loop above ??? */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (MJOBISACTIVE(J) || MJOBISSUSPEND(J))
      {
      MJobAddToNL(J,NULL,MBNOTSET);
      }  /* END if (MJOBISACTIVE(J) || MJOBISSUSPEND(J)) */
    }  /* END for (J) */
 
  /* NOTE:  move back to MPBSNodeUpdate() */

  if (MRM[0].Type == mrmtPBS)
    {
    int jindex;
    int SJCount;

    /* NOTE:  address job suspension bug in TORQUE 2.1 and earlier */

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;

      if ((MSched.TrackSuspendedJobUsage == TRUE) && 
          (N->JList != NULL) &&
          (N != MSched.GN))   /* FIXME:  should this check include all SHARED resources */
        {
        int jindex;

        int DSwap;
        int DDisk;
        int DMem;

        mjob_t *J;

        /* adjust dedicated resources based on swap requiered by both active and suspended jobs */

        DSwap = 0;
        DDisk = 0;
        DMem  = 0;

        for (jindex = 0;jindex < N->MaxJCount;jindex++)
          {
          J = (mjob_t *)N->JList[jindex];

          if (J == NULL)
            break;

          if (J == (mjob_t *)1)
            continue;

          DSwap += J->Req[0]->DRes.Swap;
          DDisk += J->Req[0]->DRes.Disk;

#ifdef MTRACKSUSPENDEDJOBDMEM
          DMem += J->Req[0]->DRes.Mem; 
#endif /* MTRACKSUSPENDEDJOBDMEM */
          }    /* END for (jindex) */

        N->DRes.Swap = MAX(N->DRes.Swap,DSwap);
        N->DRes.Disk = MAX(N->DRes.Disk,DDisk);
        N->DRes.Mem  = MAX(N->DRes.Mem,DMem);
        }  /* END if ((MSched.TrackSuspendedJobUsage == TRUE) && (N->JList != NULL)) */

      if ((N->State != mnsBusy) || 
         ((int)N->Load >= N->CRes.Procs) ||
          (N->DRes.Procs >= N->CRes.Procs))
        continue;

      if ((N->JList != NULL) && (N->MaxJCount > 0))
        {
        SJCount = 0;

        for (jindex = 0;jindex < N->MaxJCount;jindex++)
          {
          if (N->JList[jindex] == NULL)
            break;

          if (N->JList[jindex] == (mjob_t *)1)
            continue;

          if (((mjob_t *)N->JList[jindex])->State == mjsSuspended)
            {
            SJCount++;

            break;
            }
          }    /* END for (jindex) */

        if (SJCount > 0)
          {
          /* NOTE:  bug in TORQUE 2.1.0-, OpenPBS, etc, node may be marked 
                    job-exclusive if suspend job count == node's np value */

          if (N->DRes.Procs == 0)
            MNodeSetState(N,mnsIdle,FALSE);
          else
            MNodeSetState(N,mnsActive,FALSE);
          }
        }
      }    /* END for (nindex) */
    }      /* END if (MRM[0].Type == mrmtPBS) */

  if (MSched.DesktopHarvestingEnabled == TRUE)
    {
    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;

      MNodeCheckKbdActivity(N);
      }
    }

  /* NOTE:  address change of state associated with system jobs */

  if (SystemJobAdjustState == TRUE)
    {
    MJobIterInit(&JTI);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if (MJOBISACTIVE(J))
        {
        for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
          {
          RQ = J->Req[rqindex];

          for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
            {
            /* NOTE:  should establish rich system job based node state management policy 
                      via J->AllocNodeState? (NYI) */

            if (N->State == mnsIdle)
              N->State = mnsActive;
            }  /* END for (nindex) */
          }    /* END for (rqindex) */
        }      /* END if (MJOBISACTIVE(J)) */
      }        /* END for (J) */
    }          /* END if (SystemJobAdjustState == TRUE) */

  MUFree((char **)&TotalTaskDed);
  MUFree((char **)&TotalProcDed);
  MUFree((char **)&TotalProcUtl);
  MUFree((char **)&JobCPUTrackingIsDisabled);
 
  return(SUCCESS);
  }  /* END MClusterUpdateNodeState() */





/**
 * Clear dedicated resource usage for all nodes in cluster.
 *  
 * @see MRMUpdate() - parent
 *
 * NOTE: also clear partition stats?
 */

int MClusterClearUsage(void)

  {
  int      nindex;
  mln_t   *VMLink;
  mnode_t *N;

  const char *FName = "MClusterClearUsage";
 
  MDB(3,fSTRUCT) MLog("%s()\n",
    FName);

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];
 
    if ((N == NULL) || (N->Name[0] == '\0'))
      break;
 
    if (N->Name[0] == '\1')
      continue;
 
    MCResClear(&N->DRes);

    for (VMLink = N->VMList;VMLink != NULL;VMLink = VMLink->Next)
      {
      mvm_t *VM = (mvm_t *)VMLink->Ptr;

      MCResClear(&VM->DRes);
      }
    }  /* END for (nindex) */

  /* adjust global node */

  if (MSched.GN != NULL)
    {
    N = MSched.GN;

    /* reset global node */

    N->MTime = MSched.Time;
    N->ATime = MSched.Time;

    MCResCopy(&N->ARes,&N->CRes);

    MNodeSetState(N,mnsIdle,FALSE);
    }  /* END if (MNodeFind(MDEF_GNNAME,&N) == SUCCESS) */
 
  return(SUCCESS);
  }  /* END MClusterClearUsage() */





/**
 * Report current cluster usage.
 *
 * @param Format (I)
 * @param DFlagBM (I) [bitmap of mcmComplete]
 * @param Req (I)
 * @param Rsp (O)
 * @param BufSize (I)
 *
 * NOTE:  called by UI thread only
 */

int MClusterShowUsage(

  enum MWireProtocolEnum Format,
  mbitmap_t             *DFlagBM,
  void                  *Req,
  void                 **Rsp,
  int                    BufSize)

  {
  char      *BPtr;
  int        BSpace;

  mxml_t    *E = NULL;  /* set to avoid compiler warning */
  mxml_t    *CE;
  mxml_t    *QE;
  mxml_t    *RE[MMAX_RACK + 1];
  mxml_t    *NE;

  int        rindex;
  int        nindex;
  int        sindex;
 
  int        MaxSlotPerRack;

  int        MaxSpecRack;
  int        UANC;

  mhost_t   *S;

  mnode_t   *N;

  mrack_t   *R;

  mbool_t    AutoAssign;

  int        ARIndex;
  int        ASIndex;

  int        DSize[MMAX_RACKSIZE];

  const char *FName = "MClusterShowUsage";

  MDB(2,fUI) MLog("%s(%s,Flags,Req,Rsp,%d)\n",
    FName,
    MWireProtocol[Format],
    BufSize);

  if (Rsp == NULL)
    {
    return(FAILURE);
    }

  /* initialize response */

  memset(RE,0,sizeof(RE));

  if (Format == mwpXML)
    {
    if ((Rsp == NULL) || (*Rsp == NULL))
      {
      return(FAILURE);
      }

    E = *(mxml_t **)Rsp;
    }
  else
    {
    MUSNInit(&BPtr,&BSpace,(char *)Rsp,BufSize);
    }

  AutoAssign = (bmisset(DFlagBM,mcmComplete));

  UANC        = 0;
  MaxSpecRack = 0;

  /* provide current usage on node grid */

  /* determine max slot index */

  MaxSlotPerRack = 1;

  for (rindex = 1;rindex < MMAX_RACK;rindex++)
    {
    for (sindex = 1;sindex <= MMAX_RACKSIZE;sindex++)
      {
      S = &MSys[rindex][sindex];

      if (S->RMName[0] != '\0')
        {
        MaxSpecRack = rindex;

        MaxSlotPerRack = MAX(
          MaxSlotPerRack,
          sindex + MAX(0,S->SlotsUsed - 1));
        }
      }    /* END for (sindex) */
    }      /* END for (rindex) */

  /* locate MaxSlot info via Node table */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if ((N->SlotIndex <= 0) || (N->RackIndex <= 0))
      {
      UANC ++;
      }
    }  /* END for (nindex) */

  if ((MaxSlotPerRack <= 1) || (UANC > 0))
    {
    if (AutoAssign == TRUE)
      MaxSlotPerRack = MCONST_DEFRACKSIZE;
    }


  if (Format == mwpXML)
    {
    /* create cluster */

    CE = NULL;

    MXMLCreateE(&CE,(char *)MXO[mxoCluster]);

    MXMLSetAttr(CE,(char *)MClusterAttr[mcluaPresentTime],(void *)&MSched.Time,mdfLong);
    MXMLSetAttr(CE,(char *)MClusterAttr[mcluaMaxSlot],(void *)&MaxSlotPerRack,mdfInt);

    MXMLAddE(E,CE);

    /* create queue */

    QE = NULL;

    MXMLCreateE(&QE,(char *)MXO[mxoQueue]);

    MXMLAddE(E,QE);
    }
  else
    {
    /* initialize structures */

    return(FAILURE);
    }  /* END else (Format == mwpXML) */

  /* display summary rack information */

  for (rindex = 1;rindex < MMAX_RACK;rindex++)
    {
    char tmpName[MMAX_NAME];

    R = &MRack[rindex];

    MDB(5,fSTRUCT) MLog("INFO:     displaying status for rack[%d] '%s'\n",
      rindex,
      R->Name);

    if (R->NodeCount > 0)
      {
      strcpy(tmpName,R->Name);
      }
    else if ((AutoAssign == TRUE) && 
             (UANC - (rindex - MaxSpecRack - 1) * MaxSlotPerRack > 0))
      {
      sprintf(tmpName,"%02d",
        rindex);
      }
    else
      {
      continue;
      }

    if (Format == mwpXML)
      {
      MXMLCreateE(&RE[rindex],(char *)MXO[mxoRack]);

      MXMLSetAttr(RE[rindex],(char *)MRackAttr[mrkaName],(void *)tmpName,mdfString);
      MXMLSetAttr(RE[rindex],(char *)MRackAttr[mrkaIndex],(void *)&R->Index,mdfInt);
 
      MXMLAddE(CE,RE[rindex]);
      }
    else
      {
      memset(DSize,0,sizeof(DSize));
      }
    }      /* END for (rindex) */ 

  if (Format == mwpXML)
    {
    int       tmpI;

    ARIndex = MaxSpecRack + 1;
    ASIndex = 1;

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;
 
      if (N == MSched.GN)
        continue;

      rindex = N->RackIndex;
      sindex = N->SlotIndex;

      /* Validate Rack & Slot Index. */

      if (((rindex <= 0) || (sindex < 1)) && (AutoAssign == TRUE))
        {
        rindex = ARIndex;
        sindex = ASIndex;

        ASIndex++;

        if (ASIndex > MCONST_DEFRACKSIZE)
          {
          ASIndex = 1;
          ARIndex++;

          ARIndex = MIN(ARIndex,MMAX_RACK);
          }
        }
      else
        {
        rindex = 1;
        }

      tmpI = N->SlotIndex;

      N->SlotIndex = sindex;

      __MClusterNToXML(N,&NE,QE);

      N->SlotIndex = tmpI;

      if (RE[rindex] == NULL)
        {
        MXMLCreateE(&RE[rindex],(char *)MXO[mxoRack]);
   
        MXMLSetAttr(RE[rindex],(char *)MRackAttr[mrkaName],(void *)&rindex,mdfInt);
        MXMLSetAttr(RE[rindex],(char *)MRackAttr[mrkaIndex],(void *)&R->Index,mdfInt);
   
        MXMLAddE(CE,RE[rindex]);
        }

      MXMLAddE(RE[rindex],NE);
      }    /* END for (nindex) */
    }      /* END if (DFormat == mwpXML) */

  return(SUCCESS);
  }  /* END MClusterShowUsage() */





/**
 * Report limited node info for specified cluster node via XML.
 *
 * @param N (I)
 * @param SNE (O)
 * @param QE (O)
 */

int __MClusterNToXML(

  mnode_t  *N,        /* I */
  mxml_t  **SNE,      /* O */
  mxml_t   *QE)       /* O */

  {
  int JTok;
 
  mjob_t *J;

  int     tmpI;

  mxml_t *NE;

  const char *FName = "__MClusterNToXML";

  MDB(2,fUI) MLog("%s(%s,SNE,JobFound,QE)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if (SNE != NULL)
    *SNE = NULL;

  if (N == NULL)
    {
    return(FAILURE);
    }

  NE = NULL;

  if (MXMLCreateE(&NE,(char *)MXO[mxoNode]) == FAILURE)
    {
    return(FAILURE);
    }
  
  if (SNE != NULL)
    *SNE = NE;

  MXMLSetAttr(NE,(char *)MNodeAttr[mnaNodeID],(void *)N->Name,mdfString);
  MXMLSetAttr(NE,(char *)MNodeAttr[mnaNodeState],(void *)MNodeState[N->State],mdfString);

  tmpI = (int)N->SlotIndex;
  MXMLSetAttr(NE,(char *)MNodeAttr[mnaSlot],(void *)&tmpI,mdfInt);

  tmpI = (int)N->SlotsUsed;
  MXMLSetAttr(NE,(char *)MNodeAttr[mnaSize],(void *)&tmpI,mdfInt);

  JTok = -1;

  mstring_t tmpString(MMAX_LINE);

  while (MNodeGetJob(N,&J,&JTok) == SUCCESS)
    {
    if (MNodeAToString(N,mnaJobList,&tmpString,0) == SUCCESS)
      {
      mxml_t *JE = NULL;

      /* NOTE:  only add first instance of job to queue */

      if (MJobBaseToXML(J,&JE,mjstNONE,TRUE,FALSE) == SUCCESS)
        {
        MXMLAddE(QE,JE);
        }

      MXMLSetAttr(NE,(char *)MNodeAttr[mnaJobList],(void *)tmpString.c_str(),mdfString);
      }
    }    /* END while (MNodeGetJob() == SUCCESS) */

  /* add node diagnostics */

  if ((N->CRes.Disk > 0) && (N->ARes.Disk <= 0))
    MXMLSetAttr(NE,(char *)MNodeAttr[mnaOldMessages],(void *)"available disk is low",mdfString);

  if ((N->CRes.Mem > 0) && (N->ARes.Mem <= 0))
    MXMLSetAttr(NE,(char *)MNodeAttr[mnaOldMessages],(void *)"available memory is low",mdfString);

  if ((N->CRes.Swap > 0) && (N->ARes.Swap < MDEF_MINSWAP))
    MXMLSetAttr(NE,(char *)MNodeAttr[mnaOldMessages],(void *)"available swap is low",mdfString);

  return(SUCCESS);
  }  /* END MClusterNToXML() */





/**
 * @see MClusterShowARes() - child
 *
 * NOTE:  why is this here?
 */

int MClusterShowAResWrapper(

  char                  *Auth,         /* I - requestor */
  enum MWireProtocolEnum DFormat,      /* I */
  mbitmap_t             *SFlagBM,      /* I (bitmap of enum MCModeEnum) */
  mrange_t              *RRange,       /* O (modified) */
  enum MWireProtocolEnum SFormat,      /* I (source format) */
  void                  *SReq,         /* I (request constraints) */
  void                 **DRsp,         /* O (string or XML) */
  int                    BufSize,      /* I */
  int                    NSIndex,      /* I (node set index) */
  mbool_t               *NSIsComplete, /* O (optional) */
  char                  *EMsg)         /* O (optional,minsize=MMAX_LINE) */

  {
  int rc;

  if (bmisset(SFlagBM,mcmComplete))
    {
    rc = MClusterShowARes(
           Auth,
           DFormat,
           SFlagBM,
           RRange,
           SFormat,
           SReq,
           DRsp,
           BufSize,
           NSIndex,
           NSIsComplete,
           EMsg);
    }
  else
    {
    rc = MClusterShowARes(
           Auth,
           DFormat,
           SFlagBM,
           RRange,
           SFormat,
           SReq,
           DRsp,
           BufSize,
           NSIndex,
           NSIsComplete,
           EMsg);
    }

  return(rc);
  }  /* END MClusterShowAResWrapper() */





/* use heap variables - NOTE: not thread-safe */
/* addresses gdb/gcc stack size failures */

mrange_t  CARange[MMAX_REQS][MMAX_RANGE];

mrange_t ARange[MMAX_REQS][MMAX_RANGE];

const char *MRAAttr[] = {
  NONE,
  "account",
  "acl",
  "arch",
  "class",
  "coalloc",
  "count",
  "nestednodeset",
  "displaymode",
  "duration",
  "excludehostlist",
  "gmetric",
  "gres",
  "group",
  "hostlist",
  "job",
  "jobfeature",
  "jobflags",
  "label",
  "minnodes",
  "minprocs",
  "mintasks",
  "naccesspolicy",
  "nodedisk",
  "nodefeature",
  "nodemem",
  "nodeset",
  "offset",
  "os",
  "other",
  "partition",
  "policylevel",
  "qos",
  "rsvaccesslist",
  "rsvprofile",
  "starttime",
  "taskmem",
  "tpn",
  "user",
  "variables",
  "vcdesc",
  "vmusage",
  "costvariable",
  NULL };

  enum MTaskReqAttrEnum {
    mtraNONE = 0,
    mtraAccount,
    mtraACL,
    mtraArch,
    mtraClass,
    mtraCoAlloc,
    mtraCount,       /* instances of base package - mintasks will be multiplied by this value */
    mtraNestedNodeSet, /* allow for deep nodesets (recursive), processed in MJobAllocMNL() */
    mtraDisplayMode,
    mtraDuration,
    mtraExcludeHostList,
    mtraGMetric,
    mtraGRes,
    mtraGroup,
    mtraHostList,
    mtraJob,
    mtraJobFeature,
    mtraJobFlags,
    mtraLabel,
    mtraMinNodes,
    mtraMinProcs,
    mtraMinTasks,
    mtraNAccessPolicy,
    mtraNodeDisk,
    mtraNodeFeature,
    mtraNodeMem,
    mtraNodeSet,
    mtraOffset,
    mtraOS,
    mtraOther,
    mtraPartition,
    mtraPolicyLevel,
    mtraQOS,
    mtraRsvAccessList,
    mtraRsvProfile,
    mtraStartTime,
    mtraTaskMem,
    mtraTPN,
    mtraUser,
    mtraVariables,
    mtraVCDescription,    /* a VC to create to hold this in */
    mtraVMUsage,
    mtraCostVariables,    /* query variables - for quoting only */
    mtraLAST };

/**
 * Report current and future resource availability (process 'mshow -a').
 *
 * @see MVPCCheckStatus() - parent
 * @see MClusterShowAResWrapper() - parent
 * @see MUIOClusterShowARes() - parent - process 'showbf'
 * @see MUIShow() - parent - process 'mshow -a'
 * @see MJobGetRange() - child
 * @see __MClusterRangeToXML() - child - report available range
 * @see __MClusterGetCoAllocConstraints() - child
 * @see MRsvGetCost() - child - determine associated cost 
 * @see MJobAllocMNL() - child - enforce node allocation policy on available resources
 * @see MCClusterShowARes() - peer - report results w/in client
 * @see MUIVMCreate() - parent - creates a VM, uses this function for reserving storage
 *
 * @see const char *MRAAttr[]
 *
 * @param Auth (I) - requestor
 * @param DFormat (I)
 * @param SFlagBM (I) [bitmap of enum MCModeEnum]
 * @param RRange (O) [optional,modified]
 * @param SFormat (I) [source format]
 * @param SReq (I) [request constraints]
 * @param DRsp (O) [string or XML]
 * @param BufSize (I)
 * @param NSIndex (I) [node set index]
 * @param NSIsComplete (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 *
 * NOTE:  Complete multi-req solution must be located in single partition
 *        (shared/global partition allowed
 *
 * NOTE:  Cost is reported if flags==policy is specified
 *
 * NOTE:  MSched.ResourceQueryDepth controls number of responses to provide
 *
 * NOTE:  This routine performs actions in the following order:
 *
 * NOTE:  Don't return, do "goto free_and_return" to do cleanup
 *
 *  Step 1:  Initialize
 *    Step 1.1:  Perform Initial Request Validation
 *    Step 1.2:  Initialize Response
 *    Step 1.3:  Initialize Request Structure
 *    Step 1.4:  Set Global Request Options 
 *    Step 1.5:  Apply Request Profile 
 *    Step 1.6:  Set Default Request Constraints 
 *    Step 1.7:  Parse Request and Populate Pseudo-Jobs 
 *    Step 1.8:  Perform Final Request Initialization 
 *    Step 1.9:  Generate Co-allocation Groups 
 *    Step 1.10: Generate Response Header 
 *    Step 1.11: Adjust Available Time Range based on Policy Constraints 

 *  Step 2:  Process Request - Do For Each Partition
 *    Step 2.1:  Evaluate Partition 
 *    Step 2.2:  Calculate Co-Allocation Time/Host Constraints 
 *    Step 2.3:  Loop Through All Requests
 *      Step 2.3.1:  Evaluate Request
 *      Step 2.3.2:  Apply Co-alloc Host Constraints 
 *      Step 2.3.3:  Calculate/Apply Time Constraints for Request w/in Partition 
 *        - Call MJobGetRange()
 *      Step 2.3.4:  Logically AND Time Constraints for each Request 
 *      Step 2.3.5:  If 'Longest' Range Fails, Eval 'Widest' Range 
 *    Step 2.4:  Apply Range Intersection Constraints 
 *    Step 2.5:  Report Available Ranges 
 *
 *  Step 3:  Finalize
 *    Step 3.1:  Report Summary Info
 *    Step 3.2:  Free Dynamic Memory 
 */

int MClusterShowARes(

  char                  *Auth,         /* I - requestor */
  enum MWireProtocolEnum DFormat,      /* I */
  mbitmap_t             *SFlagBM,      /* I (bitmap of enum MCModeEnum) */
  mrange_t              *RRange,       /* O (optional,modified) */
  enum MWireProtocolEnum SFormat,      /* I (source format) */
  void                  *SReq,         /* I (request constraints) */
  void                 **DRsp,         /* O (string or XML) */
  int                    BufSize,      /* I */
  int                    NSIndex,      /* I (node set index) */
  mbool_t               *NSIsComplete, /* O (optional) */
  char                  *EMsg)         /* O (optional,minsize=MMAX_LINE) */

  {
  mjob_t    *J[MMAX_REQS];
  mreq_t    *RQ[MMAX_REQS];


  mjob_t    *SJ = NULL;
  mpar_t    *P  = NULL;
  mpar_t    *SP = NULL;

  mvpcp_t   *C = NULL;    /* VPC profile */

  long       ADuration;
  long       tmpStartTime;

  char       tmpPName[MMAX_NAME];  /* partition name */
  char       tmpEMsg[MMAX_LINE];   /* temp buf for failure messages from child routines */

  /* per range host list (NOTE:  must be cleared per partition) */

  char      *ReqHList[MMAX_RANGE]; /* hostlist assigned to req */

  mrange_t   GRRange[MMAX_RANGE];  /* global time constraint range */

/*
  char      *ReqPtr[MMAX_RANGE];
  int        ReqSpace[MMAX_RANGE];
*/

  char    *ptr;
  char    *TokPtr;

  mbool_t  IsComplete[MMAX_REQS];
  long     Offset[MMAX_REQS];

  mbool_t  OffsetIsUsed = FALSE;

  mulong   MaxDuration;
  mulong   tmpDuration;

  char     tmpHList[MMAX_BUFFER];

  long     MinTime;

  mbitmap_t Flags;

  int      NC;
  int      TC;
  int      PC;

  int      FNC = -1;
  int      FTC = -1;

  int      rindex;
  int      aindex;
  int      caindex;
  int      endindex;

  int      PCount;
  int      pindex;
  int      ReqCount = 0;

  int      STok = -1;

  mbool_t  IsPeer = FALSE;

  char     tmpAName[MMAX_NAME];  /* default accont name */

  char     OptionLine[MMAX_LINE];
  char     ProfileLine[MMAX_LINE];         /* VPC Profile (aka 'Package') */
  char     GlobalRsvAccessList[MMAX_LINE];
  char     CmdLine[MMAX_LINE << 2];

  char     RsvProfile[MMAX_REQS][MMAX_NAME];
  char    *OpRsvProfile = NULL;

  char     LabelList[MMAX_REQS][MMAX_NAME];
  char     VCDescList[MMAX_REQS][MMAX_NAME]; /* List of VC descriptions */
  char     VCMaster[MMAX_NAME]; /* Name of VC (or label to be used) to wrap VCs/rsvs created by mshow -a */
  char     CostVariables[MMAX_REQS][MMAX_LINE];

  char    *NodeMap[MMAX_REQS] = {NULL};

  mstring_t *Variables[MMAX_REQS];    /* Array of mstring_t ptrs, one for each REQ, allocated by 'new' */
  mstring_t *ACLLine[MMAX_REQS];      /* Array of mstring_t ptrs, ACL for each TID, allocated by 'new' */

  int      TotalRangesPrinted;

  mxml_t  *SE;
  mxml_t  *E = NULL;                  /* set to avoid compiler warning */
  mxml_t  *PE;
  mxml_t  *RE;
  mxml_t  *JE = NULL;
 
  int      rc;
  int      FinalRC;
  int      EReqCount = 0;             /* explicit req count */

  int      PackageCount = 1;          /* package instances (see mtraCount) */

  mbool_t  DoLoop;

  mbool_t  SubmitTID        = FALSE;
  mbool_t  MergeTID         = FALSE;
  mbool_t  DoFlexible       = FALSE;  /* allow multiple timeframe options? */
  mbool_t  DoIntersection   = FALSE;  /* report intersection in time */
  mbool_t  DoAggregate      = TRUE;
  mbool_t  RequestPartition = FALSE;
  mbool_t  SeekLong         = MSched.SeekLong;
  mbool_t  InitDefaultReq   = FALSE;

  mbool_t  ProcsNeeded; /* ProcRequest is also set if tasks are requested.  4 gres could be 1 task (no procs) */
  mbool_t  ProcRequest[MMAX_REQS];
  char     CoAlloc[MMAX_REQS][MMAX_NAME];
  int      CoAllocIndex[MMAX_REQS][MMAX_REQS + 1]; /* lists of co-allocated reqs */
  mulong   CoAllocDuration[MMAX_REQS];             /* max duration of co-alloc group */
  int      SingleAllocIndex[MMAX_REQS + 1];        /* reqs w/no co-alloc requirements */

  mnl_t *CAHostList[MMAX_REQS];
  mnl_t *RequestedHostList[MMAX_REQS];  /* making small to conserve memory */
  mnl_t *DstMNL[MMAX_REQS];  /* making small to conserve memory */
  mnl_t *tmpMNL[MMAX_REQS];  /* making small to conserve memory */
  mnl_t *coMNL[MMAX_REQS];
  mnl_t *DList[MMAX_REQS];
  mnl_t *AllocNodeList[MMAX_REQS];
  mnl_t *tmp[MMAX_REQS];

  mbool_t  PCReqIsExplicit; /* 'minprocs' or 'min tasks with proc count' explicitly specified in request */

  enum MPolicyTypeEnum PLevel[MMAX_REQS];

  enum MNodeAllocPolicyEnum NAllocPolicy;

  char MTRAVal[mtraLAST][MMAX_LINE];

  const char *FName = "MClusterShowARes";

  MDB(2,fUI) MLog("%s(%.32s,%s,%s,%s,SReq,DRsp,%d,%d,NSIsComplete,EMsg)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL",
    MWireProtocol[DFormat],
    (RRange != NULL) ? "RRange" : "NULL",
    MWireProtocol[SFormat],
    BufSize,
    NSIndex);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (NSIsComplete != NULL)
    *NSIsComplete = TRUE;

  ProcsNeeded = FALSE;

  /* Step 1:  Initialize */

  VCMaster[0] = '\0';

  /* Step 1.1:  Perform Initial Reqest Validation */

  if ((DRsp == NULL) || (RRange == NULL))
    {
    MDB(1,fUI) MLog("INFO:     invalid parameters in %s\n",
      FName);

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"ERROR:    invalid parameters\n");
      }

    return(FAILURE);
    }

  /* Step 1.2:  Initialize Response */

  /* NOTE:  only initialize output data on first nodeset pass */

  if (DFormat == mwpXML)
    {
    if (*DRsp == NULL)
      {
      MDB(1,fUI) MLog("INFO:     invalid parameters in %s\n",
        FName);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:    invalid parameters\n");
        }

      return(FAILURE);
      }

    E = *(mxml_t **)DRsp;
    }
  else
    {
    return(FAILURE);
    }

  /* Step 1.3:  Initialize Request Structures */

  FinalRC = SUCCESS;

  memset(NodeMap,0,sizeof(NodeMap));
  memset(J,0,sizeof(J));
  memset(RQ,0,sizeof(RQ));
  memset(ReqHList,0,sizeof(ReqHList));
  memset(Offset,0,sizeof(Offset));

  MNLMultiInit(RequestedHostList);
  MNLMultiInit(CAHostList);
  MNLMultiInit(DstMNL);
  MNLMultiInit(tmpMNL);
  MNLMultiInit(coMNL);
  MNLMultiInit(DList);
  MNLMultiInit(AllocNodeList);
  MNLMultiInit(tmp);

  memset(GRRange,0,sizeof(GRRange));

  /* Set all the mstring_t pointers to NULL, as a default state of non-newed */
  memset(ACLLine,0,sizeof(ACLLine));
  memset(Variables,0,sizeof(Variables));


  TotalRangesPrinted = 0;

  ARange[0][0].ETime = 0;

  aindex = 0;

  /* Create array of temporary jobs with one job+req per request 
     where each request is specified as a 'where' cluase.  (see 'mshow -w')
  */

  MJobMakeTemp(&J[aindex]);

  snprintf(J[aindex]->Name,MMAX_NAME,"clusterquery%d",
    aindex);

  RQ[aindex] = J[aindex]->Req[0];

  ProfileLine[0] = '\0';
  CmdLine[0] = '\0';
  GlobalRsvAccessList[0] = '\0';

  NAllocPolicy = (MPar[0].VPCNAllocPolicy != mnalNONE) ?
    MPar[0].VPCNAllocPolicy :
    MPar[0].NAllocPolicy;

  /* Step 1.4:  Set Global Request Options */

  if (SFormat == mwpXML)
    {
    /* load global request information (options) */

    MXMLGetAttr((mxml_t *)SReq,MSAN[msanOption],NULL,OptionLine,sizeof(OptionLine));

    MXMLToString((mxml_t *)SReq,CmdLine,sizeof(CmdLine),NULL,FALSE);

    ptr = MUStrTok(OptionLine,",",&TokPtr);

    while (ptr != NULL)
      {
      /* We need the VC name to not be uppercased */

      if (MUStrStr(ptr,"VC",0,TRUE,FALSE) != NULL)
        {
        if ((ptr[2] == '=') && (ptr[3] != '\0'))
          {
          MUStrCpy(VCMaster,&ptr[3],sizeof(VCMaster));
          }

        ptr = MUStrTok(NULL,",",&TokPtr);

        continue;
        }

      if (strcasestr(ptr,"INTERSECTION") != NULL)
        {
        DoIntersection = TRUE;
        }
      else if (strcasestr(ptr,"NOAGGREGATE") != NULL) 
        {
        DoAggregate = FALSE;
        }
      else if (strcasestr(ptr,"PROFILES") != NULL)
        {
        MUStrCpy(ProfileLine,&ptr[strlen("profiles") + 1],sizeof(ProfileLine));
        }
      else if (strcasestr(ptr,"TIMELOCK") != NULL)
        {
        OffsetIsUsed = TRUE;
        }
      else if (strcasestr(ptr,"RSVACCESSLIST") != NULL)
        {
        MUStrCpy(
          GlobalRsvAccessList,
          &ptr[strlen("rsvaccesslist") + 1],
          sizeof(GlobalRsvAccessList));
        }

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (ptr != NULL) */

    /* determine number of explicit resource requests */

    STok = -1;

    while (MXMLGetChild(
        (mxml_t *)SReq,
        (char *)MSON[msonSet],
        &STok,
        &SE) == SUCCESS)
      {
      /* don't do anything with children, just count them */

      EReqCount++;
      }

    bmcopy(&Flags,SFlagBM);
    }  /* END if (SFormat == mwpXML) */
  else if (SFormat == mwpNONE)
    {
    int rqindex;

    /* internal query */

    /* set internal option defaults and validate request */

    DoIntersection   = TRUE;
    DoAggregate      = TRUE;
    SubmitTID        = TRUE;
    DoFlexible       = TRUE;
    RequestPartition = TRUE;

    bmset(&Flags,mcmVerbose);
    bmset(&Flags,mcmFuture);

    /* NOTE:  assume req 1 depends on req 0, extract offset info 
              from req 0 wclimit */

    SJ = (mjob_t *)SReq;

    if (SJ == NULL)
      {
      MDB(1,fUI) MLog("INFO:     invalid pseudo-job specified in %s\n",
        FName);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:    invalid pseudo-job specified\n");
        }

      FinalRC = FAILURE;
      goto free_and_return;
      }

    for (rqindex = 0;rqindex < MMAX_REQS;rqindex++)
      {
      if (SJ->Req[rqindex] == NULL)
        break;
 
      EReqCount++;
      }  /* END for (rqindex) */
    }    /* END else if (SFormat == mwpNONE) */
  else
    {
    bmcopy(&Flags,SFlagBM);
    }

  if ((Auth != NULL) &&
      (Auth[0] != '\0') &&
      !strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    IsPeer = TRUE;

    MUStrCpy(tmpAName,&Auth[strlen("peer:")],sizeof(tmpAName));
    }
  else
    {
    MUStrCpy(tmpAName,"ANY",sizeof(tmpAName));
    }

  /* Step 1.5:  Apply Request Profile */

  if ((ProfileLine[0] == '\0') && (IsPeer == TRUE))
    {
    mvpcp_t *VPCProfile;

    /* grid peers REQUIRE a package for full functionality, ADMIN requests do 
       not require a package */

    mgcred_t *A;

    VPCProfile = (mvpcp_t *)MUArrayListGet(&MVPCP,0);

    if ((MAcctFind(MTRAVal[mtraAccount],&A) == SUCCESS) && (A->VDef != NULL))
      {
      MUStrCpy(ProfileLine,A->VDef,sizeof(ProfileLine));
      }
    else if ((MSched.DefaultA != NULL) && (MSched.DefaultA->VDef != NULL))
      {
      MUStrCpy(ProfileLine,MSched.DefaultA->VDef,sizeof(ProfileLine));
      }
    else if (VPCProfile != NULL)
      {
      MUStrCpy(ProfileLine,VPCProfile->Name,sizeof(ProfileLine));
      }
    else if (bmisset(&MSched.Flags,mschedfHosting))
      {
      /* profile is required */

      MDB(1,fUI) MLog("INFO:     required package/profile not specified in %s\n",
        FName);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:    required package/profile not specified\n");
        }

      FinalRC = FAILURE;
      goto free_and_return;
      }
    }    /* END if ((ProfileLine[0] == '\0') && (IsPeer == TRUE)) */

  if (ProfileLine[0] != '\0')
    {
    /* locate and apply default VPC profile to request */

    mln_t *tmpL;

    mxml_t *DE;

    if (MVPCProfileFind(ProfileLine,&C) == FAILURE)
      {
      MDB(1,fUI) MLog("INFO:     cannot locate requested VPC profile in %s\n",
        FName);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:    cannot locate requested VPC %s\n",
          ProfileLine);
        }

      FinalRC = FAILURE;
      goto free_and_return;
      }

    if (((C->ACL != NULL) && (MCredCheckAccess(NULL,Auth,maUser,C->ACL) == FAILURE)) &&
        ((C->ACL != NULL) && (MCredCheckAccess(NULL,tmpAName,maAcct,C->ACL) == FAILURE)))
      {
      MDB(1,fUI) MLog("INFO:     cannot access requested VPC profile in %s\n",
        FName);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:    cannot access requested VPC %s (unauthorized)\n",
          ProfileLine);
        }

      FinalRC = FAILURE;
      goto free_and_return;
      }

    tmpL = C->QueryData;

    OpRsvProfile = C->OpRsvProfile;

    /* add package co-allocation queries */

    if (SFormat == mwpXML)
      {
      char tmpName[MMAX_NAME];
      char ValLine[MMAX_NAME];

      mxml_t *E;
      mxml_t *SE;

      int WTok;

      E = (mxml_t *)SReq;

      MXMLGetChild(E,MSON[msonSet],NULL,&SE);

      while (tmpL != NULL)
        {
        /* determine if req has been explicitly specified */

        if (SE != NULL)
          {
          /* locate 'label' Where child */

          WTok = -1;

          while (MS3GetWhere(
              SE,
              NULL,
              &WTok,
              tmpName,            /* O */
              sizeof(tmpName),
              ValLine,            /* O */
              sizeof(ValLine)) == SUCCESS)
            {
            if (!strcmp(tmpName,"label"))
              break;
            }

          if (!strcmp(ValLine,tmpL->Name))
            {
            /* vpc req explicitly specified */

            tmpL = tmpL->Next;

            /* NOTE:  currently ignore default querydata if explicitly set */

            /* IMPORTANT NOTE:  If any aspect of a given req is specified explicitly,
                                all profile attributes are ignored */

            /* NOTE:  may want to set each attribute not explicitly set by request */

            continue;
            }
          }    /* END if (SE != NULL) */

        /* add package co-allocation request to explicitly specified request */

        DE = NULL;
  
        MXMLDupE((mxml_t *)tmpL->Ptr,&DE);

        MXMLSetAttr(DE,"ispkgcoalloc",(void *)"TRUE",mdfString);

        MXMLAddE(E,DE);

        tmpL = tmpL->Next;
        }  /* END while (tmpL != NULL) */
      }    /* END if (SFormat == mwpXML) */
    }    /* END if (ProfileLine[0] != '\0') */

  /* Step 1.6:  Set Default Request Constraints */

  RQ[aindex]->Index = aindex;

  RQ[aindex]->DRes.Procs = 1;

  J[aindex]->WCLimit        = 1;
  J[aindex]->SpecWCLimit[0] = 1;
  J[aindex]->SpecWCLimit[1] = 1;

  MaxDuration = 1;

  memset(Variables,0,sizeof(Variables));

  RsvProfile[aindex][0] = '\0';
  LabelList[aindex][0]  = '\0';
  VCDescList[aindex][0] = '\0';

  PLevel[aindex] = mptOff;

  /* NOTE:  by default, set one request */

  SE = NULL;

  STok = -1;

  /* Step 1.7:  Parse Request and Populate Pseudo-Jobs */

  while (((aindex < MMAX_REQS) &&
        (((SFormat == mwpNONE) && (aindex < EReqCount)) ||
         ((SFormat != mwpXML) && (aindex <= 0)) ||
         ((SFormat == mwpXML) && 
          (MXMLGetChild((mxml_t *)SReq,(char *)MSON[msonSet],&STok,&SE) == SUCCESS)))) ||
          (InitDefaultReq == FALSE))
    { 
    /* step through each specified requirement */

    InitDefaultReq = TRUE;

    tmpDuration = 1;

    ReqCount++;

    /* initialize request */

    /* get a 'new' mstring_t for this 'aindex' to use */

    Variables[aindex] = new  mstring_t(MMAX_LINE);

    MUStrCpy(MTRAVal[mtraUser],"ANY",MMAX_LINE);
    MUStrCpy(MTRAVal[mtraGroup],"ANY",MMAX_LINE);
    MUStrCpy(MTRAVal[mtraQOS],"ANY",MMAX_LINE);
    MUStrCpy(MTRAVal[mtraClass],"ANY",MMAX_LINE);
    MUStrCpy(MTRAVal[mtraAccount],tmpAName,MMAX_LINE);

    /* NOTE:  load in RM job template and extract default credentials */

    /* NYI */

    PCReqIsExplicit = FALSE;

    /* set default request constraints */

    ProcRequest[aindex] = FALSE;

    CoAlloc[aindex][0] = '\0';
    CostVariables[aindex][0] = '\0';

    if (RRange != NULL)
      {
      RRange[aindex].STime = 0;
      RRange[aindex].ETime = MMAX_TIME;
      }

    if ((SFormat == mwpNONE) && (SJ != NULL) && (aindex == 0))
      {
      /* if internal job structure specified, copy over */

      MJobDuplicate(J[aindex],SJ,FALSE);
      }

    if (J[aindex] == NULL)
      {
      /* NOTE:  request single proc per req by default */

      MJobMakeTemp(&J[aindex]);

      RQ[aindex] = J[aindex]->Req[0];
      RQ[aindex]->Index = aindex;

      if ((SFormat == mwpNONE) && (SJ != NULL))
        {
        if (SJ->Req[aindex] == NULL)
          break;

        MJobDuplicate(J[aindex],SJ,FALSE);

        tmpDuration = SJ->Req[aindex]->RMWTime;

        Offset[aindex] = SJ->Req[aindex]->RMWTime;
        }

      J[aindex]->WCLimit        = tmpDuration;
      J[aindex]->SpecWCLimit[0] = tmpDuration;
      J[aindex]->SpecWCLimit[1] = tmpDuration;

      PLevel[aindex] = mptOff;

      sprintf(J[aindex]->Name,"mshow-req:%d",
        aindex);
      }  /* END if (J[aindex] == NULL) */

    if (C != NULL)
      {
      bmset(&J[aindex]->IFlags,mjifVPCMap);

      MUStrDup(&J[aindex]->ReqVPC,C->Name);
      }

    RsvProfile[aindex][0] = '\0';
    LabelList[aindex][0]  = '\0';
    VCDescList[aindex][0] = '\0';

    if ((SFormat == mwpXML) && (SE == NULL))
      {
      /* no request explicitly specified */

      aindex++;

      break;
      }

    /* parse resource description */

    if ((SReq != NULL) || (SFormat == mwpXML))
      {
      char *ptr = NULL;  /* set to avoid compiler warning */
      char *tptr;

      char  ValLine[MMAX_BUFFER];
      char  tmpName[MMAX_NAME];

      int   attrindex;
      enum MCompEnum   CIndex;    /* attribute comparison index */
 
      int   WTok;

      switch (SFormat)
        {
        case mwpXML:

          if ((C != NULL) && 
              (C->ReqSetAttrs != NULL) &&
              (MXMLGetAttr(SE,"ispkgcoalloc",NULL,NULL,0) == FAILURE))
            {
            /* NOTE:  ReqSetAttrs only apply to explicitly specified reqs */

            char tmpLine[MMAX_LINE];

            char *ptr;
            char *TokPtr;

            char *cptr;

            MUStrCpy(tmpLine,C->ReqSetAttrs,sizeof(tmpLine));

            /* FORMAT:  <ATTR>=<VAL>[,<ATTR>=<VAL>] */

            ptr = MUStrTok(tmpLine,",",&TokPtr);

            while (ptr != NULL)
              {
              cptr = strchr(ptr,'=');

              if (cptr != NULL)
                {
                *cptr = '\0';

                MS3AddWhere(SE,ptr,cptr + 1,NULL);
                }

              ptr = MUStrTok(NULL,",",&TokPtr);
              }
            }    /* END if ((C != NULL) && (C->SetReqAttrs != NULL)) */

          break;

        case mwpHTML:
        default:

          ptr = MUStrTok((char *)SReq,",&? \t\n",&tptr);

          break;
        }  /* END switch (SFormat) */

      DoLoop = TRUE;

      WTok = -1;

      tmpName[0] = '\0';

      while (DoLoop == TRUE)
        {
        switch (SFormat)
          {
          case mwpXML:

            {
            mxml_t *WE;

            CIndex = mcmpGE;

            if (MS3GetWhere(
                 SE,
                 &WE,
                 &WTok,
                 tmpName,
                 sizeof(tmpName),
                 ValLine,
                 sizeof(ValLine)) == FAILURE)
              {
              DoLoop = FALSE;
              }

            if (WE != NULL)
              {
              char tmpOp[MMAX_NAME];

              MXMLGetAttr(WE,MSAN[msanOp],NULL,tmpOp,sizeof(tmpOp));

              if (tmpOp[0] != '\0')
                {
                CIndex = (enum MCompEnum)MUGetIndexCI(tmpOp,MAMOCmp,FALSE,0);
                }
              }
            }

            break;

          case mwpHTML:
          default:

            /* extract user, group, account, class, partition, duration, */
            /*         minnodes, minprocs, nodemem, taskmem, nodefeature, */
            /*         qos, displaymode */
     
            /* FORMAT:  <ATTR>=<VAL>[,<ATTR>=<VAL>]... */
   
            if (MUParseComp(ptr,tmpName,&CIndex,ValLine,sizeof(ValLine),TRUE) == FAILURE) 
              {
              MDB(3,fUI) MLog("INFO:     cannot parse request in %s\n",
                FName);

              if (EMsg != NULL)
                {
                snprintf(EMsg,MMAX_LINE,"ERROR:    cannot parse parameter '%.64s'\n",
                  ptr);
                }

              FinalRC = FAILURE;
              goto free_and_return;
              }

            /* NOTE:  MUParseComp() strips square brackets */

            if (!strcmp(ValLine,"NONE"))
              strcpy(ValLine,NONE);
        
            break;
          }  /* END switch (SFormat) */

        if (tmpName[0] != '\0')
          {
          if ((attrindex = MUGetIndexCI(tmpName,MRAAttr,FALSE,0)) == 0)
            {
            MDB(3,fUI) MLog("INFO:     cannot parse parameter '%s' in %s\n",
              tmpName,
              FName);

            if (EMsg != NULL)
              {
              snprintf(EMsg,MMAX_LINE,"ERROR:    cannot parse parameter '%s'\n",
                tmpName);
              }

            FinalRC = FAILURE;
            goto free_and_return;
            }

          MUStrCpy(MTRAVal[attrindex],ValLine,MMAX_LINE);
          }
        else
          {
          attrindex = mtraNONE;
          }

        switch (attrindex)
          {
          case mtraAccount:

            if (strcmp(ValLine,ALL) && strcmp(ValLine,"ANY"))
              {
              MUStrCpy(MTRAVal[mtraAccount],ValLine,MMAX_LINE);

              MAcctFind(ValLine,&J[aindex]->Credential.A);
              }

            break;

          case mtraACL:

            if (ACLLine[aindex] == NULL)
              {
              /* construct a new ACL string entry with the 'ValLine' */
              ACLLine[aindex] = new mstring_t(ValLine);
              }
            else
              {
              *ACLLine[aindex] += ',';
              *ACLLine[aindex] += ValLine;
              }

            break;

          case mtraArch:

            if ((RQ[aindex]->Arch = MUMAGetIndex(
                 meArch,
                 ValLine,
                 mAdd)) == FAILURE)
              {
              MDB(1,fUI) MLog("INFO:     invalid architecture in %s\n",
                FName);

              if (EMsg != NULL)
                {
                snprintf(EMsg,MMAX_LINE,"ERROR:    invalid architecture requested\n");
                }

              FinalRC = FAILURE;
              goto free_and_return;
              }
 
            break;

          case mtraClass:

            if (!strcmp(ValLine,NONE))
              {
              J[aindex]->Credential.C = NULL;
              }
            else if (strcmp(ValLine,ALL) &&
                strcmp(ValLine,"ANY"))
              {
              if (MClassFind(ValLine,&J[aindex]->Credential.C) == FAILURE)
                {
                if (EMsg != NULL)
                  {
                  snprintf(EMsg,MMAX_LINE,"ERROR:    requested class does not exist\n");
                  }

                FinalRC = SUCCESS;
                goto free_and_return;
                }
              }

            break;

          case mtraCoAlloc:

            /* NOTE:  taskcounts for all co-alloc members must match */

            {
            /* enforce co-allocation in space of all matching reqs */

            MUStrCpy(CoAlloc[aindex],ValLine,sizeof(CoAlloc));
            }

            break;

          case mtraCount:

            PackageCount = (int)strtol(ValLine,NULL,10);

            break;

          case mtraNestedNodeSet:

            {
            /* FORMAT: <feature>[|<feature>...][:<feature>[|<feature>]]... */
       
            mln_t *tmpL;
       
            char Buf[MMAX_NAME];
       
            int  index = 0;
            int  index2;
       
            char *ptr;
            char *ptr2;
            char *TokPtr;
            char *TokPtr2;
            char *String = NULL;
       
            char **List;
       
            MUStrDup(&String,ValLine);
            
            ptr = MUStrTokE(String,":",&TokPtr);
       
            while (ptr != NULL)
              {
              /* Add a new nodeset, name is just the index */
       
              ptr2 = MUStrTokE(ptr,"|",&TokPtr2);
       
              if (ptr2 == NULL)
                break;
       
              snprintf(Buf,sizeof(Buf),"%d",index++);
       
              MULLAdd(&RQ[aindex]->NestedNodeSet,Buf,NULL,&tmpL,NULL);
       
              tmpL->Ptr = (char **)MUCalloc(1,sizeof(char *) * MMAX_ATTR);
       
              List = (char **)tmpL->Ptr;
       
              index2 = 0;
       
              while (ptr2 != NULL)
                {     
                MUStrDup(&List[index2++],ptr2);
       
                ptr2 = MUStrTok(NULL,"|",&TokPtr2);
                }         
       
              ptr = MUStrTokE(NULL,":",&TokPtr);
              }  /* END while (ptr != NULL) */
       
            RQ[aindex]->SetType = mrstFeature;
            RQ[aindex]->SetSelection = mrssOneOf;
            RQ[aindex]->SetBlock = TRUE;

            MUFree(&String);
            }  /* END case mtraNestedNodeSet */

            break;

          case mtraDuration:

            {
            mulong tmpL;

            /* FORMAT:  [+][[[DD:]HH:]MM:]SS */

            if (!strcasecmp(ValLine,"infinity"))
              {
              tmpL = MCONST_EFFINF;
              }
            else
              {
              tmpL = MUTimeFromString(ValLine);
              }

            if ((aindex > 0) && 
               ((ValLine[0] == '+') || (ValLine[0] == '-')))
              {
              tmpL += J[0]->WCLimit;
              }

            tmpL = MAX(1,tmpL);

            J[aindex]->WCLimit        = tmpL;
            J[aindex]->SpecWCLimit[0] = tmpL;
            J[aindex]->SpecWCLimit[1] = tmpL;

            if (tmpL > MaxDuration)
              MaxDuration = tmpL;
            }  /* END BLOCK */
 
            break;

          case mtraJob:

            {
            mjob_t *DJP;

            /* locate job, set defaults to job values */

            if (MJobFind(ValLine,&DJP,mjsmBasic) == FAILURE)
              {
              MDB(7,fUI) MLog("INFO:     cannot locate requested job in %s\n",
                FName);

              if (EMsg != NULL)
                {
                snprintf(EMsg,MMAX_LINE,"ERROR:    cannot locate requested job\n");
                }

              FinalRC = FAILURE;
              goto free_and_return;
              }

            memcpy(J[aindex],DJP,sizeof(mjob_t));
   
            J[aindex]->WCLimit = DJP->WCLimit;
            J[aindex]->SpecWCLimit[0] = J[aindex]->WCLimit;
            J[aindex]->SpecWCLimit[1] = J[aindex]->WCLimit;
  
            if (DJP->Req[0] != NULL)
              {
              MReqDuplicate(RQ[aindex],DJP->Req[0]);
              }
            }  /* END BLOCK */

            break;

          case mtraJobFeature:

            MJobSetAttr(J[aindex],mjaGAttr,(void **)ValLine,mdfString,mSet);

            break;

          case mtraJobFlags:

            if (!strcmp(ValLine,MJobFlags[mjfRsvMap]))
              {
              if (MSched.PRsvSandboxRID != NULL)
                {
                MUStrCpy(ValLine,(char *)MJobFlags[mjfAdvRsv],sizeof(ValLine));

                MUStrDup(&J[aindex]->ReqRID,MSched.PRsvSandboxRID);
                }
              else
                {
                /* no global prsvsandbox, only check policies */

                /* NO-OP */
                }
              }

            MJobFlagsFromString(J[aindex],NULL,NULL,NULL,NULL,ValLine,FALSE);

            bmor(&J[aindex]->Flags,&J[aindex]->SpecFlags);

            break;

          case mtraGMetric:

            {
            char *ptr;
            char *TokPtr;

            /* FORMAT:  <GMETRIC><COMP><VAL>[{+;}<GMETRIC><COMP><VAL>]... */

            ptr = MUStrTok(ValLine,"+;",&TokPtr);

            while (ptr != NULL)
              {
              if (MJobAddGMReq(J[aindex],RQ[aindex],ptr) == FAILURE)
                {
                MDB(7,fUI) MLog("INFO:  gmetric requirement requested in %s\n",
                  FName);

                if (EMsg != NULL)
                  {
                  snprintf(EMsg,MMAX_LINE,"ERROR:    invalid gmetric requirement requested\n");
                  }
                }

              ptr = MUStrTok(NULL,"+;",&TokPtr);
              }  /* END while (ptr != NULL) */
            }    /* END BLOCK (case mtraGMetric) */

            break;

          case mtraGRes:

            {
            char *ptr;
            char *TokPtr;

            char *ptr2;
            char *TokPtr2;

            int   gindex;
            int   tmpS;

            /* FORMAT:  <GRES>[:<COUNT>][;<GRES>[:<COUNT>]]... */

            ptr = MUStrTok(ValLine,"+;",&TokPtr);

            while (ptr != NULL)
              {
              if (ptr[0] == ':')
                {
                gindex = 0;

                ptr2 = MUStrTok(ptr,":",&TokPtr2);
                }
              else
                {
                ptr2 = MUStrTok(ptr,":",&TokPtr2);
                
                gindex = MUMAGetIndex(
                  meGRes,
                  ptr2,
                  (MSched.EnforceGRESAccess == TRUE) ? mVerify : mAdd);

                ptr2 = MUStrTok(NULL,":",&TokPtr2);
                }

              if (ptr2 != NULL)
                tmpS = (unsigned int)strtol(ptr2,NULL,10);
              else
                tmpS = 1;
   
              if (gindex > 0)
                { 
                /* verify resource not already added */
    
                if (MSNLGetIndexCount(&RQ[aindex]->DRes.GenericRes,gindex) > 0)
                  {
                  FinalRC = SUCCESS;
                  goto free_and_return;
                  }
                }

              if (gindex > 0)
                MSNLSetCount(&RQ[aindex]->DRes.GenericRes,gindex,tmpS);

              MSNLRefreshTotal(&RQ[aindex]->DRes.GenericRes);

              if (PCReqIsExplicit == FALSE)
                RQ[aindex]->DRes.Procs = 0;  /* assume global GRes request is standalone */

              if ((MSched.GN != NULL) && (MSNLGetIndexCount(&MSched.GN->DRes.GenericRes,gindex) > 0))
                {
                /* resource exists on the global node */

                /* minprocs, minnodes, and global node gres are mutually exclusive */

                RQ[aindex]->TaskCount          = 1;
                RQ[aindex]->TaskRequestList[0] = 1;
                RQ[aindex]->TaskRequestList[1] = 1;
 
                /* increment floating node count */
 
                J[aindex]->FloatingNodeCount++;
                }
              else
                {
                /* resources exist on compute node */

                if (RQ[aindex]->TaskCount == 0)
                  {
                  ProcRequest[aindex] = TRUE;

                  RQ[aindex]->TaskCount          = 1;
                  RQ[aindex]->TaskRequestList[0] = 1;
                  RQ[aindex]->TaskRequestList[1] = 1;

                  /* if (J[aindex]->Request.TC == 0)
                    J[aindex]->Request.TC = 1; */
                  }
                }
 
              ptr = MUStrTok(NULL,"+;",&TokPtr);
              }  /* END while (ptr != NULL) */
            }    /* END (case mtraGRes) */
  
            break;

          case mtraGroup:
 
            if (strcmp(ValLine,ALL) && strcmp(ValLine,"ANY"))
              MGroupAdd(ValLine,&J[aindex]->Credential.G);

            break;

          case mtraHostList:

            /* FORMAT: <HOST>[:<HOST>]... */

            /* don't set anything on the job here, this info is only needed for MJobGetRange() and will 
               mess all the other routines up (like MJobDistributeMNL() and MJobAllocMNL() */

            MNLFromString(ValLine,RequestedHostList[aindex],1,FALSE);

#if 0
            /* cannot call MJobSetAttr() as it will allocate memory and freeing it in this routine
               will be a nightmare */

            MJobSetAttr(J[aindex],mjaReqHostList,(void **)ValLine,mdfString,mSet);
#endif

            break;

          case mtraExcludeHostList:

            MNLFromString(ValLine,&J[aindex]->ExcHList,1,FALSE);

            break;

          case mtraOffset:

            Offset[aindex] = MUTimeFromString(ValLine);

            OffsetIsUsed = TRUE;

            break;

          case mtraLabel:

            MUStrCpy(LabelList[aindex],ValLine,sizeof(LabelList[aindex]));

            break;

          case mtraOS:

            if ((RQ[aindex]->Opsys = MUMAGetIndex(
                 meOpsys,
                 ValLine,
                 mAdd)) == FAILURE)
              {
              MDB(7,fUI) MLog("INFO:     invalid OS requested in %s\n",
                FName);

              if (EMsg != NULL)
                {
                snprintf(EMsg,MMAX_LINE,"ERROR:    invalid os requested\n");
                }

              FinalRC = FAILURE;
              goto free_and_return;
              }

            break;

          case mtraOther:

            /* NOTE: this is used to handle the case in DISA where "mshow -a" is used to see if
                     VPC growth is possible and at what cost.  In their use case, an RsvMap systemjob
                     is already running on a SINGLEJOB node, but we need to test if there is space 
                     for this job to grow.  This flag allows us to bypass the SINGLEJOB parameter
                     and test for available space on the node */

            if (!strcasecmp(ValLine,"ignorenodeaccesspolicy"))
              {
              bmset(&J[aindex]->IFlags,mjifIgnoreNodeAccessPolicy);
              }

            break;

          case mtraPartition:

            if (MParFind(ValLine,&SP) == SUCCESS)
              {
              RQ[aindex]->PtIndex = SP->Index;

              if (SP->Index != 0)
                RequestPartition = TRUE;
              }

            break;

          case mtraRsvProfile:

            MUStrCpy(RsvProfile[aindex],ValLine,sizeof(RsvProfile[aindex]));

            break;

          case mtraMinNodes:

            /* cannot specify both minprocs and minnodes */

            RQ[aindex]->NodeCount = (int)strtol(ValLine,NULL,0);
            J[aindex]->Request.NC = RQ[aindex]->NodeCount;

            RQ[aindex]->DRes.Procs = -1;
            ProcsNeeded = TRUE;

            break;

          case mtraMinProcs:

            {
            int tmpI = (int)strtol(ValLine,NULL,10);

            if (tmpI > 0)
              {
              ProcsNeeded = TRUE;
              ProcRequest[aindex] = TRUE;

              /* cannot specify both minprocs and minnodes */
              
              RQ[aindex]->TaskCount = tmpI;
              J[aindex]->Request.TC = RQ[aindex]->TaskCount;

              RQ[aindex]->DRes.Procs = 1;

              PCReqIsExplicit = TRUE;
              }
            }    /* END BLOCK */

            break;

          case mtraMinTasks:

            {
            char *ptr;

            /* FORMAT:  <TASKCOUNT>[@<RTYPE>:<COUNT>[+<RTYPE>:<COUNT>]...] */

            /* ie, 4@gres=vm:2 */

            int tmpI = (int)strtol(ValLine,&ptr,10);

            if (tmpI <= 0)
              break;

            if ((ptr != NULL) && (*ptr == '@'))
              {
              ptr++;

              MCResFromString(&RQ[aindex]->DRes,ptr);

              if (bmisset(&MSched.Flags,mschedfNormalizeTaskDefinitions))
                {
                MCResNormalize(&RQ[aindex]->DRes,&tmpI);

                RQ[aindex]->TasksPerNode = tmpI;
                }

              if (RQ[aindex]->DRes.Procs > 0)
                {
                ProcsNeeded = TRUE;
                ProcRequest[aindex] = TRUE;

                PCReqIsExplicit = TRUE;
                }

              RQ[aindex]->TaskCount = tmpI;
              J[aindex]->Request.TC = RQ[aindex]->TaskCount;
              }
            else
              {
              /* by default, map mintasks to minprocs */

              ProcRequest[aindex] = TRUE;
              ProcsNeeded = TRUE;

              /* cannot specify both minprocs and minnodes */

              RQ[aindex]->TaskCount = tmpI;
              J[aindex]->Request.TC = RQ[aindex]->TaskCount;

              RQ[aindex]->DRes.Procs = 1;

              PCReqIsExplicit = TRUE;
              }
            }    /* END BLOCK (case mtraMinTasks) */

            break;

          case mtraNAccessPolicy:
 
            RQ[aindex]->NAccessPolicy = (enum MNodeAccessPolicyEnum) MUGetIndexCI(
              ValLine,
              MNAccessPolicy,
              FALSE,
              mnacShared);

            break;

          case mtraNodeDisk:

            RQ[aindex]->ReqNR[mrDisk] = (int)strtol(ValLine,NULL,10);

            RQ[aindex]->ReqNRC[mrDisk] = CIndex;

            break;

          case mtraNodeMem:
  
            RQ[aindex]->ReqNR[mrMem] = (int)strtol(ValLine,NULL,10);
 
            RQ[aindex]->ReqNRC[mrMem] = CIndex;

            break;

          case mtraNodeFeature:

            if (!strcasecmp(ValLine,"$common"))
              {
              int taindex;

              /* locate explicitly set node features from previous req */

              for (taindex = 0;taindex < aindex;taindex++)
                {
                if (!bmisclear(&RQ[taindex]->ReqFBM))
                  {
                  /* req node feature is set */

                  bmcopy(&RQ[aindex]->ReqFBM,&RQ[taindex]->ReqFBM);

                  break;
                  }
                }  /* END for (taindex) */
              }
            else if (strcmp(ValLine,NONE))
              {
              MReqSetFBM(J[aindex],RQ[aindex],mAdd,ValLine);
              }
 
            break;

          case mtraNodeSet:

            {
            char *ptr;
            char *TokPtr;

            int   sindex;

            /* node set info */

            /* FORMAT:  <SETTYPE>:<SETATTR>[:<SETLIST>] */

            if ((ptr = MUStrTok(ValLine,":",&TokPtr)) != NULL)
              {
              RQ[aindex]->SetSelection = (enum MResourceSetSelectionEnum)MUGetIndex(
                ptr,
                MResSetSelectionType,
                FALSE,
                mrssNONE);

              if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
                {
                RQ[aindex]->SetType = (enum MResourceSetTypeEnum)MUGetIndex(
                  ptr,
                  MResSetAttrType,
                  FALSE,
                  mrstNONE);

                sindex = 0;

                while ((ptr = MUStrTok(NULL,",",&TokPtr)) != NULL)
                  {
                  MUStrDup(&RQ[aindex]->SetList[sindex],ptr);

                  sindex++;
                  }
                }

              ptr = MUStrTok(NULL,":",&TokPtr);

              if (ptr != NULL)
                {
                RQ[aindex]->SetBlock = MUBoolFromString(ptr,TRUE);
                }
              else
                {
                RQ[aindex]->SetBlock = TRUE;
                }
              }
            }   /* END of case mtraNodeset */

            break;

          case mtraPolicyLevel:

            PLevel[aindex] = (enum MPolicyTypeEnum)MUGetIndexCI(
              ValLine,
              MPolicyMode,
              FALSE,
              PLevel[aindex]);

            break;

          case mtraQOS:

            if (MQOSFind(ValLine,&J[aindex]->QOSRequested) == FAILURE)
              {
              /* cannot locate requested QOS */

              MQOSFind(DEFAULT,&J[aindex]->QOSRequested);
              }

            MJobSetQOS(J[aindex],J[aindex]->QOSRequested,0);

            break;

          case mtraRsvAccessList:

            {
            int index;
            char *TokPtr;
            char *ptr;
          
            /* clear old values */
       
            if (J[aindex]->RsvAList == NULL)
              J[aindex]->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);
       
            ptr = MUStrTok((char *)ValLine,": \t\n",&TokPtr);
       
            index = 0;
       
            while (ptr != NULL)
              {
              MUStrDup(&J[aindex]->RsvAList[index],ptr);
       
              index++;
       
              if (index >= MMAX_PJOB)
                break;
       
              ptr = MUStrTok(NULL,": \t\n",&TokPtr);
              }  /* END while (ptr != NULL) */
            }    /* END BLOCK (case mraRsvAccessList) */

            break;

          case mtraTaskMem:

            RQ[aindex]->DRes.Mem = (int)strtol(ValLine,NULL,0);

            break;

          case mtraTPN:

            RQ[aindex]->TasksPerNode = (int)strtol(ValLine,NULL,0);

            break;
 
          case mtraDisplayMode:
 
            if (strstr(ValLine,"seekwide"))
              {
              SeekLong = FALSE;
              }

            if (strstr(ValLine,"future"))
              {
              DoFlexible = TRUE;

              bmset(&Flags,mcmFuture);
              }

            break;

          case mtraStartTime:

            {
            /* FORMAT:  <STIME>[-<ENDTIME>][;<STIME>[-<ENDTIME>]]... */

            bmset(&Flags,mcmFuture);
 
            MRLFromString(ValLine,GRRange);

            if ((C != NULL) && (C->ReqStartPad > 0))
              {
              /* must move starttime back to compensate for start pad */

              int index;

              for (index = 0;GRRange[index].ETime != 0;index++)
                {
                if (GRRange[index].STime != 0)
                  GRRange[index].STime -= C->ReqStartPad;

                GRRange[index].ETime -= C->ReqStartPad;
                }
              }

            if (GRRange[0].STime < MSched.Time)
              {
              if ((C != NULL) && (C->ReqStartPad > 0) && (EMsg != NULL))
                {
                snprintf(EMsg,MMAX_LINE,"ERROR:    requested starttime does not leave room for required startpad\n");
                }
              else if (EMsg != NULL)
                {
                snprintf(EMsg,MMAX_LINE,"ERROR:    requested starttime is in the past\n");
                }
              }

            RRange[aindex].STime = GRRange[0].STime;
            RRange[aindex].ETime = GRRange[0].ETime;

            if (RRange[aindex].STime >= MMAX_TIME)
              {
              RRange[aindex].STime = 0;
              }

            if (RRange[0].STime <= MSched.Time)
              {
              bmset(&J[aindex]->IFlags,mjifCheckCurrentOS);
              }
            }  /* END BLOCK */
 
            break;

          case mtraUser:

            if (strcmp(ValLine,ALL) && strcmp(ValLine,"ANY"))
              MUserAdd(ValLine,&J[aindex]->Credential.U);

            break;

          case mtraVariables:

            /* it's possible to have multiple variables (coming from VCPROFILE[] REQSETATTR and -w) */

            if (!Variables[aindex]->empty())
              {
              *Variables[aindex] += ';';
              }

            *Variables[aindex] += ValLine;

            break;

          case mtraVCDescription:

            MUStrCpy(VCDescList[aindex],ValLine,sizeof(VCDescList[aindex]));

            break;

          case mtraVMUsage:

            J[aindex]->VMUsagePolicy = (enum MVMUsagePolicyEnum)MUGetIndexCI(
              ValLine,
              MVMUsagePolicy,
              FALSE,
              0);

            break;

          case mtraCostVariables:

            /* variable string - for quoting only */

            MUStrCpy(CostVariables[aindex],ValLine,sizeof(CostVariables));

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (attrindex) */
     
        switch (SFormat)
          {
          case mwpXML:

            /* NO-OP */

            break;

          case mwpHTML:
          default:
  
            ptr = MUStrTok(NULL,",&? \t\n",&tptr);

            if (ptr == NULL)
              DoLoop = FALSE;
          }  /* END switch (SFormat) */
        }    /* END while (DoLoop == TRUE) */

      /* Check if default creds required */
      if (strcmp(MTRAVal[mtraAccount],"ANY") &&
         (J[aindex]->Credential.A == NULL))
        {
        MAcctAdd(MTRAVal[mtraAccount],&J[aindex]->Credential.A);
        }

      MJobBuildCL(J[aindex]);
      }   /* END if (SReq != NULL) */

    if ((GlobalRsvAccessList[0] != '\0') && (J[aindex]->RsvAList == NULL))
      {
      char tmpAList[MMAX_LINE];

      int index;
      char *TokPtr;
      char *ptr;

      MUStrCpy(tmpAList,GlobalRsvAccessList,sizeof(tmpAList));
    
      /* clear old values */
 
      J[aindex]->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);
 
      ptr = MUStrTok((char *)tmpAList,": \t\n",&TokPtr);
 
      index = 0;
 
      while (ptr != NULL)
        {
        MUStrDup(&J[aindex]->RsvAList[index],ptr);
 
        index++;
 
        if (index >= MMAX_PJOB)
          break;
 
        ptr = MUStrTok(NULL,": \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }

    aindex++;

    if (aindex >= MMAX_REQ_PER_JOB)
      {
      /* too many co-allocation requests */

      MDB(1,fUI) MLog("ALERT:    too many co-allocation requests in %s\n",
        FName);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"ERROR:  too many co-allocation requests (%d > %d)\n",
          aindex + 1,
          MMAX_REQ_PER_JOB);
        }

      FinalRC = FAILURE;
      goto free_and_return;
      }
    }     /* END while ((aindex < MMAX_REQS) && ...) */

  /* Step 1.8:  Perform Final Request Initialization */
 
  /********************************************
   *   Done reading in requirements, do the   *
   *   the final preparations before we start *
   *   to process everything                  *
   *                                          *
   *  A) Adjust nodesets                      *
   *  B) Apply profile walltime adjustments   *
   *  C) Determine active partition count     *
   *  D) Adjust query options                 *
   ********************************************/

  if (C != NULL)
    {
    int index;

    /* VPC profile specified */

    /* B)  Apply profile walltime adjustments */

    if ((!bmisset(&Flags,mcmOverlap)) && ((C->ReqStartPad > 0) || (C->ReqEndPad > 0)))
      {
      int  index;
      long tmpL;

      for (index = 0;index < EReqCount;index++)
        {
        tmpL = J[index]->WCLimit;
        tmpL += C->ReqStartPad + C->ReqEndPad;

        J[index]->WCLimit = tmpL;
        J[index]->SpecWCLimit[0] = tmpL;
        J[index]->SpecWCLimit[1] = tmpL;
        }
      }    /* END if () */

    for (index = 1;index < aindex;index++)
      {
      if (J[index]->WCLimit == 1)
        {
        J[index]->WCLimit = J[0]->WCLimit + abs(Offset[index]);
        J[index]->SpecWCLimit[0] = J[index]->WCLimit;
        J[index]->SpecWCLimit[1] = J[index]->WCLimit;
        }
      }
    }    /* END if (C != NULL) */

  if (PackageCount > 1)
    {
    int index;

    for (index = 0;index < aindex;index++)
      {
      RQ[index]->TaskCount *= PackageCount;
      J[index]->Request.TC *= PackageCount;
      }
    }

  /* C) Determine active partition count */

  PCount = 0;

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    if (MPar[pindex].Name[0] == '\1')
      continue;

    if (MPar[pindex].Name[0] == '\0')
      break;

    if (MPar[pindex].ConfigNodes == 0)
      continue;

    if (pindex == MSched.SharedPtIndex)
      continue;

    PCount++;
    }  /* END for (pindex) */

  /* D) Adjust query options */

  if (bmisset(&Flags,mcmUseTID))
    {
    SubmitTID = TRUE;
    }  /* END if (bmisset(&Flags,mcmUseTID)) */

  if (bmisset(&Flags,mcmSummary))
    {
    /* if Summary, create multi-trans TIDs */
    /* no other side-affects */

    MergeTID = TRUE;
    }  /* END if (bmisset(&Flags,mcmSummary)) */

  if (bmisset(&Flags,mcmFuture))
    {
    DoFlexible = TRUE;
    }

  if (ReqCount == 1)
    DoAggregate = FALSE;

  /* Step 1.9:  Generate Co-allocation Groups */

  rindex = 0;
  caindex = 0;

  for (aindex = 0;aindex < ReqCount;aindex++)
    {
    int aindex2;
    int acount;

    if (CoAlloc[aindex][0] == '\1')
      {
      /* co-alloc constraint already processed */

      /* NO-OP */

      continue;
      }

    if (CoAlloc[aindex][0] == '\0')
      {
      /* no co-allocation requested */

      SingleAllocIndex[rindex] = aindex;

      rindex++;

      continue;
      }

    CoAllocIndex[caindex][0] = aindex;
    CoAllocDuration[caindex] = J[aindex]->WCLimit;

    acount = 1;

    for (aindex2 = aindex + 1;aindex2 < ReqCount;aindex2++)
      {
      if (!strcmp(CoAlloc[aindex],CoAlloc[aindex2]))
        { 
#ifdef MNOTDAVE
        /* req's are members of the same co-alloc group */

        if (RQ[aindex]->TaskCount != RQ[aindex2]->TaskCount)
          {
          /* why must intra-coalloc-group req taskcounts match? */

          MDB(1,fUI) MLog("INFO:     co-allocation request taskcounts do not match in %s\n",
            FName);

          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"ERROR:    request taskcounts within coalloc group '%s' do not match (%d != %d)",
              CoAlloc[aindex],
              RQ[aindex]->TaskCount,
              RQ[aindex2]->TaskCount);
            }

          FinalRC = FAILURE;
          goto free_and_return;
          }
#endif /* MNOTDAVE */

        CoAllocIndex[caindex][acount] = aindex2;
        CoAllocDuration[caindex] = MAX(CoAllocDuration[caindex],J[aindex]->WCLimit);

        acount++;

        /* mark CoAlloc as processed */

        CoAlloc[aindex2][0] = '\1';
        }
      }    /* END for (aindex2) */

    if (acount > 1)
      {
      CoAllocIndex[caindex][acount] = -1;

      caindex++;
      }
    else
      {
      /* only single reference to req, no co-allocation required */

      SingleAllocIndex[rindex] = aindex;

      rindex++;
      }
    }   /* END for (aindex) */

  /* terminate co-allocation lists */

  CoAllocIndex[caindex][0] = -1;
  SingleAllocIndex[rindex] = -1;

/*
  if ((C != NULL) && (C->ReqStartPad > 0))
    {
    int index;

    for (index = 0;index < aindex;index++)
      {
      if (J[index]->WCLimit < (ARange[index][0].ETime - ARange[index][0].STime))
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"ERROR:     requested duration does not fit time constraints\n");
          }

        return(FAILURE);
        }
      }
    }
*/

  /* Step 1.10:  Generate Response Header */
  
  MXMLCreateE(&JE,(char *)MXO[mxoJob]);

  if (J[0]->Credential.U != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void *)J[0]->Credential.U->Name,mdfString);

  if (J[0]->Credential.G != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void *)J[0]->Credential.G->Name,mdfString);

  if (RQ[0]->PtIndex != 0)
    {
    MXMLSetAttr(
      JE,
      (char *)MReqAttr[mrqaReqPartition],
      (void *)MPar[RQ[0]->PtIndex].Name,
      mdfString);
    }

  MXMLSetAttr(JE,"time",(void *)&MSched.Time,mdfLong);

  MXMLAddE(E,JE);

  PE = NULL;

  MXMLCreateE(&PE,(char *)MXO[mxoPar]);

  MXMLSetAttr(PE,"Name",(void **)"template",mdfString);
  MXMLAddE(E,PE);

  MXMLCreateE(&RE,"range");
  MXMLSetAttr(RE,"proccount",(void **)"Procs",mdfString);
  MXMLSetAttr(RE,"nodecount",(void **)"Nodes",mdfString);

  if (bmisset(&Flags,mcmPolicy))
    MXMLSetAttr(RE,"cost",(void **)"Cost",mdfString);

  MXMLSetAttr(RE,"duration",(void **)"Duration",mdfString);
  MXMLSetAttr(RE,"starttime",(void **)"StartTime",mdfString);

  MXMLAddE(PE,RE);

  PE = NULL;

  /* Step 1.11: Adjust Available Time Range based on Policy Constraints */

  if (PLevel[0] == mptHard)
    {
    long tmpL;

    for (aindex = 0;aindex < ReqCount;aindex++)
      {
      tmpL = RRange[aindex].STime;

      if (MPolicyGetEStartTime(
            J[aindex],
            mxoJob,
            &MPar[0],
            mptHard,
            &tmpL,
            NULL) == SUCCESS)
        {
        if (RRange != NULL)
          RRange[aindex].STime = tmpL;

        GRRange[0].STime = tmpL;
        }
      }
    }    /* END if (PLevel[0] == mptHard) */

  /* Step 1.12:  Count compute reqs (for DISA accounting) */

  if ((MAM[0].Type != mamtNONE) && (MAM[0].ChargePolicyIsLocal == TRUE))
    {
    char CBuffer[MMAX_LINE];

    int SANIndex = -1;
    int CCount = 0;

    for (aindex = 0;aindex < ReqCount;aindex++)
      {
      if (RQ[aindex]->DRes.Procs != 0)
        {
        CCount++;
        }
      else if (!MSNLIsEmpty(&RQ[aindex]->DRes.GenericRes))
        {
        SANIndex = aindex;
        }
      }  /* END for (aindex) */

    if (SANIndex >= 0)
      {
      snprintf(CBuffer,sizeof(CBuffer),";VM_COUNT=%d",CCount);

      *Variables[SANIndex] += CBuffer;
      }
    }    /* END if ((MAM[0].Type != mamtNONE) && (MAM[0].ChargePolicyIsLocal == TRUE)) */

  /* Step 2:  Process Request */

  /***************************************
   *       THE ACTION STARTS HERE        *
   *  at this point everything has been  *
   *  setup and we will begin processing *
   ***************************************/

  /* for each partition, evaluate each request */
  /* for each request, evaluate each range */
  /* DoAggregate = merge all request values within each range */

  /* NOTE:  if (N->CRes - N->ARes > N->DRes) we have a temporary spike that may block 
            immediate reservations. If failure detected (No Resources) retry with 
            earliest starttime adjusted (extend horizon way out there) NYI 
            Only do loop when range found within MJobGetRange()  */

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    /* Step 2.1:  Evaluate Each Partition */

    P = &MPar[pindex];

    if (P->Name[0] == '\1')
      continue;

    if (P->Name[0] == '\0')
      break;

    if ((PCount == 1) && (RequestPartition == FALSE) && (P->Index != 0))
      break;

    /* If no possible partitions, only check the "ALL" partition */

    if ((PCount == 0) && (P->Index != 0))
      break;

    if (P->ConfigNodes == 0)
      continue;

    /* If the job requires procs, check for procs */

    if ((P->UpRes.Procs == 0) && (ProcsNeeded))
      continue;

    /* Step 2.2:  Calculate Co-Allocation Time/Host Constraints */

    if (__MClusterGetCoAllocConstraints(
          P,            /* I */
          CoAllocIndex, /* I */
          Offset,       /* I */
          RRange,       /* I */
          J,            /* I */
          RQ,           /* I */
          CARange,      /* O (terminated w/ETime == 0) */
          CAHostList,   /* O (terminated w/N == NULL) */
          SeekLong,
          tmpEMsg) == FAILURE)
      {
      /* partition does not support required co-allocation */

      MDB(5,fUI) MLog("INFO:     partition %s does not support co-allocation - %s\n",
        P->Name,
        tmpEMsg);

      continue;
      }

    /* NOTE:  since all co-alloc reqs must exactly match wrt task count, CARange can be 
              used in place of ARange for coalloc reqs.  Right? */
 
    /* memset(ReqPtr,0,sizeof(ReqPtr)); */

    for (rindex = 0;rindex < MMAX_RANGE;rindex++)
      {
      if (ReqHList[rindex] != NULL)
        MUFree(&ReqHList[rindex]);
      }

    /* For each partition, evaluate each request */

    /* Step 2.3:  Loop Through All Requests */
 
    for (aindex = 0;aindex < ReqCount;aindex++)
      {
      /* Step 2.3.1:  Evaluate Request */

      if ((RQ[aindex]->PtIndex > 0) && (RQ[aindex]->PtIndex != P->Index))
        {
        /* different partition explicitly requested */

        break;
        }

      if (RQ[aindex]->TaskCount == 0)
        {
        RQ[aindex]->TaskCount = RQ[aindex]->NodeCount;
        }

      MinTime = 0;
  
      MDB(7,fUI) MLog("INFO:     checking resource availability in partition %s\n",
        P->Name);

      strcpy(tmpPName,MCONST_GLOBALPARNAME);

      if ((PE == NULL) || (PE->AVal[0] == NULL) || strcmp(PE->AVal[0],P->Name))
        {
        /* add global partition */

        PE = NULL;

        MXMLCreateE(&PE,(char *)MXO[mxoPar]);
 
        MXMLSetAttr(PE,"Name",P->Name,mdfString);

        MXMLAddE(E,PE);
        }

      /* load availability information */

      if (bmisset(&Flags,mcmFuture))
        {
        /* Process 'mshow -a' request */

        int rc;

        mrange_t tmpARange[MMAX_RANGE];

        tmpARange[0].ETime = 0;

        /* only process first range time constraint */

        if ((RRange != NULL) && (RRange[aindex].STime > 0))
          tmpStartTime = RRange[aindex].STime;
        else
          tmpStartTime = MSched.Time;

        /* Step 2.3.2:  Apply Co-alloc Host Constraints */

        if (!MNLIsEmpty(CAHostList[aindex]))
          {
          MNLCopy(&J[aindex]->ReqHList,CAHostList[aindex]);

          bmset(&J[aindex]->IFlags,mjifHostList);

          J[aindex]->ReqHLMode = mhlmSuperset;
          }

        /* Step 2.3.3:  Calculate/Apply Time Constraints for Request w/in Partition */

        if (CARange[aindex][0].ETime == 0)
          {
          /* no co-alloc time constraints specified - determine available 
             times for req within partition */

          if (!MNLIsEmpty(RequestedHostList[aindex]))
            {
            bmset(&J[aindex]->IFlags,mjifHostList);

            MNLCopy(&J[aindex]->ReqHList,RequestedHostList[aindex]);

            RQ[aindex]->Index = 0;
            }

          rc = MJobGetRange(
                J[aindex],
                RQ[aindex],
                NULL,
                P,
                tmpStartTime,                /* I */
                FALSE,
                (mrange_t *)ARange[aindex],  /* O */
                NULL,
                NULL,
                NULL,
                0,  /* do not support firstonly/notask */
                SeekLong,
                NULL,
                NULL);

          if (!MNLIsEmpty(RequestedHostList[aindex]))
            {
            bmunset(&J[aindex]->IFlags,mjifHostList);

            MNLClear(&J[aindex]->ReqHList);

            RQ[aindex]->Index = aindex;
            }
          }
        else
          {
          /* use pre-calculated co-allocation time range */

          memcpy(ARange[aindex],CARange[aindex],sizeof(ARange[0]));

          rc = SUCCESS;
          }

        /* NOTE:  may want to enable optimization flag if calculation becomes 
                  too expensive */

        /* Step 2.3.4:  Logically AND Time Constraints for each Request */

        if (rc == SUCCESS)
          {
          if ((GRRange[0].ETime > 0) && 
              (GRRange[0].ETime < MMAX_TIME))
            {
            /* find the range intersection in time */

            rc = MRLAND(ARange[aindex],ARange[aindex],GRRange,FALSE);
            }
          }    /* END if (rc == SUCCESS) */

        /* Step 2.3.5:  If 'Longest' Range Fails, Eval 'Widest' Range */

        if (((rc == FAILURE) || (ARange[aindex][0].STime > (mulong)tmpStartTime)) && 
             (SeekLong == TRUE))
          {
          if (CARange[aindex][0].ETime == 0)
            {
            rc = MJobGetRange(
                  J[aindex],
                  RQ[aindex],
                  NULL,
                  P,
                  tmpStartTime, /* I */
                  FALSE,
                  tmpARange,    /* O */
                  NULL,
                  NULL,
                  NULL,
                  0,        /* do not support firstonly/notask */
                  FALSE,
                  NULL,
                  NULL);
            }
          else
            {
            /* use pre-calculated co-allocation time range */

            memcpy(ARange[aindex],CARange[aindex],sizeof(ARange[0]));

            rc = SUCCESS;
            }

          if ((rc == SUCCESS) &&
             ((ARange[aindex][0].ETime == 0) ||
              ((tmpARange[0].ETime != 0) && (tmpARange[0].STime < ARange[aindex][0].STime))))
            {
            /* earlier range located - apply 'wide' range results */

            MDB(7,fUI) MLog("INFO:     improved availability range located for request %d in partition %s with 'wide' range search - applying\n",
              aindex,
              P->Name);

            memcpy(ARange[aindex],tmpARange,sizeof(tmpARange));
            }

          if (rc == SUCCESS)
            {
            if ((GRRange[0].ETime > 0) &&
                (GRRange[0].ETime < MMAX_TIME))
              {
              rc = MRLAND(ARange[aindex],ARange[aindex],GRRange,FALSE);
              }
            }
          }    /* END if (((rc == FAILURE) || ...) && ...) */

        if (rc == FAILURE)
          {
          MDB(3,fUI) MLog("INFO:     cannot get resource information for request %d in partition %s\n",
            aindex,
            P->Name);

          break;
          }

        if (FNC == -1)
          {
          /* extract summary feasible resource info */

          int rindex;

          for (rindex = 0;rindex < MMAX_RANGE;rindex++)
            {
            if (ARange[aindex][rindex].ETime != 0)
              continue;

            FNC = ARange[aindex][rindex].NC;
            FTC = ARange[aindex][rindex].TC;

            if (PE != NULL)
              {
              MXMLSetAttr(
                PE,
                "feasibleNodeCount",
                (void *)&FNC,
                mdfInt);

              MXMLSetAttr(
                PE,
                "feasibleTaskCount",
                (void *)&FTC,
                mdfInt);
              }
 
            break;
            }
          }    /* END if (FNC == -1) */

        /* NOTE:  results masked by RRange timeframe constraints above */

        if (SubmitTID == TRUE)
          {
          /* filter out ranges which do not satisfy TaskCount requirement */
/*
          MRLReduce(
            ARange[aindex],
            TRUE,
            MAX(RQ[aindex]->TaskCount,RQ[aindex]->NodeCount));
*/
          }
        }    /* END if (bmisset(&Flags,mcmFuture) == TRUE) */
      else
        {
        /* Process 'showbf' Request */

        mnl_t ANL;

        int ReqPIndex;

        MNLInit(&ANL);

        for (rindex = 0;rindex < MMAX_RANGE;rindex++)
          {
          if ((PLevel[0] == mptHard) && (GRRange[0].STime != MSched.Time))
            {
            break;
            }

          ReqPIndex = J[aindex]->Req[0]->PtIndex;

          J[aindex]->Req[0]->PtIndex = pindex;

          /* set template to prevent loading of non-compute resources by default */

          bmset(&J[aindex]->IFlags,mjifIsTemplate);

          rc = MBFGetWindow(
                 J[aindex],
                 MinTime,
                 &NC,
                 &TC,
                 &ANL,
                 &ADuration,
                 SeekLong,
                 bmisset(&Flags,mcmVerbose) ?
                   tmpHList :
                   NULL);

          J[aindex]->Req[0]->PtIndex = ReqPIndex;

          if (rc == FAILURE)
            {
            break;
            }

          MinTime = ADuration;
  
          PC = TC;
 
          ARange[aindex][rindex].TC    = TC;
          ARange[aindex][rindex].NC    = NC;
          ARange[aindex][rindex].STime = MSched.Time;
          ARange[aindex][rindex].ETime = MSched.Time + ADuration;

          MDB(4,fUI) MLog("INFO:     located resource window w/%d procs, %ld seconds\n",
            PC,
            ADuration);
          }    /* END for (rindex) */

        /* terminate list */

        MNLFree(&ANL);

        ARange[aindex][rindex].ETime = 0;
        }      /* END else (bmisset(&Flags,mcmFuture) == TRUE) */
      }        /* END for (aindex) */

    if ((RQ[aindex] != NULL) && 
        (RQ[aindex]->PtIndex > 0) && 
        (RQ[aindex]->PtIndex != P->Index))
      {
      /* different partition explicitly requested */

      continue;
      }

    /* Step 2.4:  Apply Range Intersection Constraints */

    if (DoIntersection == TRUE)
      {
      mrange_t IRange[MMAX_RANGE];

      if (Offset[0] != 0)
        MRLOffset(ARange[0],-Offset[0]);

      memcpy(IRange,ARange[0],sizeof(IRange));

      for (aindex = 1;aindex < ReqCount;aindex++)
        {
        if (Offset[aindex] != 0)
          MRLOffset(ARange[aindex],-Offset[aindex]);

        MRLGetIntersection(IRange,IRange,ARange[aindex]);
        }  /* END for (aindex) */

      /* IRange = result of intersection in time of all ranges */

      if (Offset[0] != 0)
        MRLOffset(ARange[0],Offset[0]);

      /* NOTE:  aindex start changed from 1 to 0 (DBJ) */

      /* NOTE:  strip off all ranges that do not meet coallocation constraints */

      for (aindex = 0;aindex < ReqCount;aindex++)
        {
        MRLAND(ARange[aindex],ARange[aindex],IRange,FALSE);

        if (Offset[aindex] != 0)
          MRLOffset(ARange[aindex],Offset[aindex]);
        }  /* END for (aindex) */
      }    /* END if (DoIntersection == TRUE) */

    /* Step 2.5:  Report Available Ranges */

    if ((SubmitTID != TRUE) && !bmisset(&Flags,mcmVerbose))
      {
      for (aindex = 0;aindex < ReqCount;aindex++)
        {
        if (aindex >= MMAX_REQS)
          {
          /* NOTE:  only allow up to MMAX_REQS ranges currently */

          break;
          }

        for (rindex = 0;rindex < MMAX_RANGE;rindex++)
          {
          if (ARange[aindex][rindex].ETime == 0)
            break;

          /* NOTE: Should we check the range after we are done processing both constraints */

          ADuration = ARange[aindex][rindex].ETime - 
                      ARange[aindex][rindex].STime;

          if (((mulong)ADuration < J[aindex]->WCLimit) ||
              (ARange[aindex][rindex].NC < RQ[aindex]->NodeCount) ||
              (ARange[aindex][rindex].TC < RQ[aindex]->TaskCount))
            {
            /* range does not meet specified constraints, ignore */

            continue;
            }

          __MClusterRangeToXML(
            PE,
            Auth,
            NULL,
            ARange[aindex][rindex].STime,
            ADuration,
            aindex,
            ARange[aindex][rindex].NC,
            ARange[aindex][rindex].TC,
            &Flags,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            ProfileLine,
            NULL,
            0,
            NULL,
            &RQ[aindex]->ReqFBM,
            FALSE,
            NULL,
            NULL);

          TotalRangesPrinted++;
          }      /* END for (rindex) */
        }        /* END for (aindex) */
      }          /* END if ((SubmitTID != TRUE) || bmisset(Flags,mcmVerbose)) */
    else 
      {
      int RangeCount[MMAX_REQS];
      int OldIndex = 0;
      int RangesPrinted[MMAX_REQS];

      mbool_t ErrorOccurred = FALSE;

      long ETime[MMAX_RANGE];
      int  RIndex[MMAX_REQS];

      int eindex;

      /* TID or verbose response requested (constrain resources reported) */

      memset(IsComplete,FALSE,sizeof(IsComplete));
      memset(RangeCount,0,sizeof(RangeCount));
      memset(RangesPrinted,0,sizeof(RangesPrinted));

      for (aindex = 0;aindex < ReqCount;aindex++)
        {
        MNodeMapInit(&NodeMap[aindex]);

        J[aindex]->Req[0] = RQ[aindex];
        J[aindex]->Req[1] = NULL;
        }

      /* extract array of event times from ARange */

      MRLGetEList(ARange,ReqCount,ETime);

      memset(RIndex,0,sizeof(RIndex));
      /* NOTE:  if DoAggregate == TRUE, offsets and per req starttimes not supported */

      for (eindex = 0;ETime[eindex] != -1;eindex++)
        {
        int         index;
        int         tindex;

        mrange_t    tRange[MMAX_RANGE];

        MRLGetIndexArray(ARange,ReqCount,ETime[eindex],RIndex);

        if (eindex > 0)
          {
          /* check if everything has already been printed */

          for (aindex = 0;aindex < ReqCount;aindex++)
            {
            if (RangesPrinted[aindex] != MSched.ResourceQueryDepth)
              break;
            }
    
          if (aindex >= ReqCount)
            break;
          }

        /* initialize DstMNL, tmpMNL */
 
        MNLMultiClear(DstMNL);
        MNLMultiClear(tmpMNL);

        for (index = (OldIndex > 0) ? OldIndex : 0;index < MSched.ResourceQueryDepth;index++)
          {
          long STime = 0;

          int  NC;
          int  TC;

          /* tmpTIDList[] used for MergeTID feature */

          char tmpTIDList[MMAX_LINE];
          int  FirstTID;
          
          if (RangeCount[index] >= MSched.ResourceQueryDepth)
            break;

          if ((index > 0) &&
             ((DoFlexible == FALSE) || (SubmitTID == FALSE)))
            {
            /* return only single timeframe unless 'flexible' results are specified */

            break;
            }

          for (aindex = 0;aindex < ReqCount;aindex++)
            {
            /* evaluate each request for specific range */

            rindex = RIndex[aindex];

            if (IsComplete[aindex] == TRUE)
              {
              continue;
              }

            if (ARange[aindex][rindex].ETime == 0)
              {
              IsComplete[aindex] = TRUE;

              continue;
              }

            if (RangesPrinted[aindex] >= MSched.ResourceQueryDepth)
              continue;

            if ((DoIntersection == TRUE) && ((OffsetIsUsed == TRUE) || (CoAllocIndex[0][0] != -1)))
              {
              STime = ETime[eindex] + index * MaxDuration;
              /*STime = ARange[aindex][rindex].STime + index * MaxDuration;*/
              }
            else
              {
              STime = ETime[eindex] + index * J[aindex]->WCLimit;
              /*STime = ARange[aindex][rindex].STime + index * J[aindex]->WCLimit;*/
              }
  
            if ((index > 0) &&
                (STime + J[aindex]->WCLimit > ARange[aindex][rindex].ETime))
              {
              /* end of range located */

              /* break out of request loop (aindex) */
  
              break;
              }

            /* NOTE:  ARange is start range, job can start at any time during range */

            ADuration = ARange[aindex][rindex].ETime - 
                        ARange[aindex][rindex].STime;

            if ((rindex > 0) &&
                (RRange[aindex].STime > 0) &&
                (DoFlexible == FALSE))
              {
              /* break after first range if fuzzy report not requested */

              break;
              }

            /* extract nodelist which satisfies request */

            /* NOTE:  must be re-run with STime adjusted for MSched.ResourceQueryDepth 
                      responses */

            if (!MNLIsEmpty(RequestedHostList[aindex]))
              {
              /* Because MReqCheckResourceMatch() checks RQ->Index and J->HLIndex we 
                 have to munge those here and then set them back afterwards.  This is 
                 because we can't have multiple hostlist requirements on each req. This
                 messes up MJobAllocMNL().  (See MOAB-49) */

              bmset(&J[aindex]->IFlags,mjifHostList);
   
              MNLCopy(&J[aindex]->ReqHList,RequestedHostList[aindex]);
   
              J[aindex]->HLIndex = RQ[aindex]->Index;
              }

            if (MJobGetRange(
                  J[aindex],
                  RQ[aindex],
                  NULL,
                  P,
                  STime,     /* I */
                  FALSE,
                  tRange,    /* O (not used) */
                  tmpMNL,    /* O (each req placed in different index) */
                  NULL,
                  NodeMap[aindex],
                  0,         /* I do not support firstonly/notask */
                  SeekLong,
                  NULL,
                  EMsg) == FAILURE)
              {
              MDB(3,fUI) MLog("INFO:     cannot get resource information - '%s'\n",
                EMsg);

              ErrorOccurred = TRUE;

              break;
              }

            if (!MNLIsEmpty(RequestedHostList[aindex]))
              {
              bmunset(&J[aindex]->IFlags,mjifHostList);
   
              J[aindex]->HLIndex = 0;

              MNLClear(&J[aindex]->ReqHList);
   
              RQ[aindex]->Index = aindex;
              }
            }    /* END for (aindex) */

          if ((index > 0) &&
              (aindex < ReqCount) &&
              (STime > (long)ARange[aindex][RIndex[aindex]].ETime))  /* ARange is StartRange */
            {
            OldIndex = index;

            /* end of range located */

            /* break out of query depth loop (index) */

            break;
            }

          if (bmisset(&Flags,mcmExclusive))
            {
            /* NOTE:  all responses must be generated before distribution 
                      is merged */

            /* verify resources contained in range are exclusive of other ranges */

            if (CoAllocIndex[0][0] == -1)
              {
              /* no co-allocation specified */

              for (aindex = 0;aindex < ReqCount;aindex++)
                {
                if (aindex >= MMAX_REQ_PER_JOB)
                  {
                  MDB(1,fUI) MLog("ERROR:    too many co-allocation requests in %s\n",
                    FName);

                  if (EMsg != NULL)
                    {
                    snprintf(EMsg,MMAX_LINE,"ERROR:  too many co-allocation requests (%d > %d)\n",
                      aindex + 1,
                      MMAX_REQ_PER_JOB);
                    }

                  FinalRC = FAILURE;
                  goto free_and_return;
                  }

                J[0]->Req[aindex] = RQ[aindex];

                MNLCopy(coMNL[aindex],tmpMNL[aindex]);
                }  /* END for (aindex) */

              if (aindex < MMAX_REQ_PER_JOB)
                J[0]->Req[aindex] = NULL;
              }    /* END if (CoAlloc[0][0] == -1) */
            else
              {
              int aindex2;

              /* NOTE:  req exclusivity not applied to members of current
                        req's coallocation set (NYI) */
                                                                                              
              /* NOTE:  total resources should only be distributed across
                        one representative member of each co-allocation set */

              aindex = 0;

              for (aindex2 = 0;aindex2 < MMAX_REQS;aindex2++)
                {
                if (CoAllocIndex[aindex2][0] == -1)
                  break;

                J[0]->Req[aindex] = RQ[CoAllocIndex[aindex2][0]];

                MNLCopy(coMNL[aindex],tmpMNL[CoAllocIndex[aindex2][0]]);

                aindex++;
                }

              for (aindex2 = 0;aindex2 < MMAX_REQS;aindex2++)
                {
                if (SingleAllocIndex[aindex2] == -1)
                  break;

                MNLCopy(coMNL[aindex],tmpMNL[SingleAllocIndex[aindex2]]);

                J[0]->Req[aindex] = RQ[SingleAllocIndex[aindex2]];

                aindex++;
                }

              if (aindex > MMAX_REQ_PER_JOB)
                {
                MDB(1,fUI) MLog("ERROR:    too many co-allocation requests in %s\n",
                  FName);
                                                                                              
                if (EMsg != NULL)
                  {
                  snprintf(EMsg,MMAX_LINE,"ERROR:  too many co-allocation requests (%d > %d)\n",
                    aindex + 1,
                    MMAX_REQ_PER_JOB);
                  }
                }

              if (aindex < MMAX_REQ_PER_JOB)
                J[0]->Req[aindex] = NULL;
              }  /* END else (CoAlloc[0][0] == -1) */

            if (J[0]->Req[1] == NULL)
              {
              /* only single effective req to evaluate, no distribution required */

              MNLMultiCopy(DList,tmpMNL);
              }
            else if (MJobDistributeMNL(
                  J[0],               /* I */
                  coMNL,              /* I */
                  DList) == FAILURE)  /* O */
              {
              /* insufficient nodes were found */

              /* FIXME: should we move onto next range rather than returning? */

              ErrorOccurred = TRUE;

              MDB(3,fUI) MLog("INFO:     could not find exclusive resources more multi-req request with range '%d'\n",
                RIndex[aindex]);
#ifdef __MNOT

              MDB(3,fUI) MLog("INFO:     exclusive resources not available\n");

              MXMLSetAttr(
                PE,
                "message",
                (void *)"resources not available (no resource) 2",
                mdfString);

              for (rindex = 0;rindex < MMAX_RANGE;rindex++)
                {
                if (ReqHList[rindex] != NULL)
                  MUFree(&ReqHList[rindex]);
                }  /* END for (rindex) */

              FinalRC = SUCCESS;
              goto free_and_return;
#endif
              }  /* END if (MJobDistributeMNL() == FAILURE) */

            if (CoAllocIndex[0][0] == -1)
              {
              /* no co-allocation required */

              MNLMultiCopy(tmpMNL,DList);
              }
            else if (ErrorOccurred == FALSE)
              {
              int aindex2;

              /* must make multiple copies of NodeList, one for each 
                 co-alloc group member */

              for (aindex = 0;aindex < MMAX_REQS;aindex++)
                {
                if (CoAllocIndex[aindex][0] == -1)
                  break;

                for (aindex2 = 0;aindex2 < MMAX_REQS;aindex2++)
                  {
                  if (CoAllocIndex[aindex][aindex2] == -1)
                    break;

                  MNLCopy(tmpMNL[CoAllocIndex[aindex][aindex2]],DList[aindex]);
                  }
                }    /* END for (aindex) */

              for (aindex2 = 0;aindex2 < MMAX_REQS;aindex2++)
                {
                if (SingleAllocIndex[aindex2] == -1)
                  break;

                MNLCopy(tmpMNL[SingleAllocIndex[aindex2]],DList[aindex]);

                aindex++;
                }  /* END for (aindex2) */
              }
            }      /* END if (bmisset(&FlagBM,mcmExclusive)) */

          /* NOTE: this loop is to check if we have any resources this iteration when
                   taking into account nodesets */

          for (aindex = 0;aindex < ReqCount;aindex++)
            {
            if (RQ[aindex]->NestedNodeSet != NULL)
              {
              /* set returned nodelist to requested resources */

              J[aindex]->Req[1] = NULL;

              MNLCopy(tmp[0],tmpMNL[aindex]);
              MNLTerminateAtIndex(tmp[1],0);

              if (MJobAllocMNL(
                    J[aindex],           /* I */
                    tmp,                 /* I */
                    NodeMap[aindex],
                    AllocNodeList,       /* O */
                    NAllocPolicy,
                    STime,
                    NULL,
                    EMsg) == SUCCESS)
                {
                /* NO-OP */
                }
              else
                {
                MDB(4,fSTRUCT) MLog("WARNING:  could not allocate req '%d' used NAllocPolicy '%s' (%s)\n",
                  aindex,
                  MNAllocPolicy[MPar[0].NAllocPolicy],
                  EMsg);

                ErrorOccurred = TRUE;

                break;
                }
              }  /* END if ((RQ[aindex]->TaskCount > 0) || (RQ[aindex]->NodeCount > 0)) */
            }  /* END for (aindex) */

          if (ErrorOccurred == TRUE)
            {
            /* resources not available this range? */

            /* break out of query depth loop (index) */
            /* evaluate next range (rindex) */

            ErrorOccurred = FALSE;

            break;
            }

          tmpTIDList[0] = '\0';
          FirstTID = 0;
 
          for (aindex = 0;aindex < ReqCount;aindex++)
            {
            int TCReq;
            
            /* process each request */

            rindex = RIndex[aindex];

            if (IsComplete[aindex] == TRUE)
              {
              continue;
              }

            if (ARange[aindex][rindex].ETime == 0)
              {
              IsComplete[aindex] = TRUE;

              continue;
              }

            if (RangesPrinted[aindex] >= MSched.ResourceQueryDepth)
              continue;

            if ((DoIntersection == TRUE) && ((OffsetIsUsed == TRUE) || (CoAllocIndex[0][0] != -1)))
              {
              STime = ETime[eindex] + index * MaxDuration;
              /*STime = ARange[aindex][rindex].STime + index * MaxDuration;*/
              }
            else
              {
              STime = ETime[eindex] + index * J[aindex]->WCLimit;
              /*STime = ARange[aindex][rindex].STime + index * J[aindex]->WCLimit;*/
              }
 
            /* NOTE:  ARange is start range, job can start at any time during range */

            ADuration = ARange[aindex][rindex].ETime - ARange[aindex][rindex].STime;

            if ((rindex > 0) &&
                (RRange[aindex].STime > 0) &&
                (DoFlexible == FALSE))
              {
              /* break after first range if fuzzy report not requested */

              break;
              }

            /* NOTE:  merging may cause ARange[aindex][rindex]/tRange discrepancy */

            ARange[aindex][rindex].TC = 0;
            ARange[aindex][rindex].NC = 0;

            for (tindex = 0;MNLGetNodeAtIndex(tmpMNL[aindex],tindex,NULL) == SUCCESS;tindex++)
              {
              ARange[aindex][rindex].TC += MNLGetTCAtIndex(tmpMNL[aindex],tindex);
              ARange[aindex][rindex].NC++;
              }  /* END for (tindex) */

            if (SubmitTID == TRUE)
              {
              /* bound located resources by request size */

              /* NOTE:  only required once per range */

              /* NOTE: assumes dedicated node model
                       may want to change to allow partial node allocation */

              if (RQ[aindex]->NodeCount > 0)
                ARange[aindex][rindex].NC = RQ[aindex]->NodeCount;

              if ((RQ[aindex]->TaskCount > 0) || (RQ[aindex]->NodeCount > 0))
                {
                int tindex;
                int TC;

                int TCReq;  /* tasks still required */

                mbool_t DoJobAllocMNL = TRUE;

                TC = 0;

                /* set returned nodelist to requested resources */

                if (DoJobAllocMNL == TRUE) 
                  {
                  J[aindex]->Req[1] = NULL;

                  MNLCopy(tmp[0],tmpMNL[aindex]);
                  MNLTerminateAtIndex(tmp[1],0);

                  if (MJobAllocMNL(
                        J[aindex],           /* I */
                        tmp,                 /* I */
                        NodeMap[aindex],
                        AllocNodeList,       /* O */
                        NAllocPolicy,
                        STime,
                        NULL,
                        EMsg) == SUCCESS)
                    {
                    MNLCopy(tmpMNL[aindex],AllocNodeList[0]);
                    }
                  else
                    {
                    /* use firstfit node allocation */

                    MDB(4,fSTRUCT) MLog("WARNING:  could not allocate req '%d' used NAllocPolicy '%s' (%s)\n",
                      aindex,
                      MNAllocPolicy[MPar[0].NAllocPolicy],
                      EMsg);

                    /* NO-OP unless we are using NestedNodeSet */

                    if (RQ[aindex]->NestedNodeSet != NULL)
                      MNLTerminateAtIndex(tmpMNL[aindex],0);
                    }
                  }

                if ((MNLIsEmpty(tmpMNL[aindex])) &&
                    (RQ[aindex]->NestedNodeSet != NULL))
                  {
                  continue;
                  }

                for (tindex = 0;MNLGetNodeAtIndex(tmpMNL[aindex],tindex,NULL) == SUCCESS;tindex++)
                  {
                  if ((ProcRequest[aindex] == TRUE) && (RQ[aindex]->TaskCount > 0))
                    TCReq = RQ[aindex]->TaskCount - TC;
                  else
                    TCReq = MSched.JobMaxTaskCount;

                  if (TCReq <= 0)
                    break;

                  if ((RQ[aindex]->NodeCount > 0) && (tindex >= RQ[aindex]->NodeCount))
                    break;

                  /* NOTE:  change to support tid exact proc match (3/31/05) - side effects? */

                  MNLSetTCAtIndex(tmpMNL[aindex],tindex,MIN(TCReq,MNLGetTCAtIndex(tmpMNL[aindex],tindex)));

                  TC += MNLGetTCAtIndex(tmpMNL[aindex],tindex);
                  }  /* END for (tindex) */

                MNLTerminateAtIndex(tmpMNL[aindex],tindex);

                /* set taskcount and nodecount to requested resources */

                ARange[aindex][rindex].TC = TC;

                if (RQ[aindex]->NodeCount > 0)
                  {
                  ARange[aindex][rindex].NC = MIN(
                    ARange[aindex][rindex].NC,
                    RQ[aindex]->NodeCount);
                  }

                ARange[aindex][rindex].NC = MIN(
                  ARange[aindex][rindex].NC,
                  tindex);
                }
              }    /* END if (SubmitTID == TRUE) */

            if (ReqHList[rindex] == NULL)
              {
              if ((ReqHList[rindex] = (char *)MUMalloc(MMAX_BUFFER)) == FAILURE)
                {
                if (EMsg != NULL)
                  {
                  snprintf(EMsg,MMAX_LINE,"ERROR:    out of memory error\n");
                  }

                FinalRC = FAILURE;
                goto free_and_return;
                }
              }

            if (DoAggregate == FALSE)
              MNLMultiClear(DstMNL);

            if (DoAggregate == TRUE)
              {
              mnode_t *N1;
              mnode_t *N2;

              int nindex1;
              int nindex2;

              /* DstMNL contains aggregate node list */

              for (nindex1 = 0;MNLGetNodeAtIndex(tmpMNL[aindex],nindex1,&N1) == SUCCESS;nindex1++)
                {
                for (nindex2 = 0;MNLGetNodeAtIndex(DstMNL[0],nindex2,&N2) == SUCCESS;nindex2++)
                  {
                  if (N1 == N2)
                    {
                    /* skipping previously added node */
  
                    break;
                    }
                  }

                if (N2 == NULL)
                  {
                  MNLSetNodeAtIndex(DstMNL[0],nindex2,N1);
                  MNLTerminateAtIndex(DstMNL[0],nindex2 + 1);

                  if (N1->CRes.Procs != 0)
                    {
                    MNLSetTCAtIndex(DstMNL[0],nindex2,N1->CRes.Procs);
                    }
                  else
                    {
                    /* assume all reqs satisfied on single global node */
  
                    MNLSetTCAtIndex(DstMNL[0],nindex2,RQ[aindex]->TaskCount);
                    }
                  }  /* END for (nindex2) */
                }    /* END for (nindex1) */

              if (aindex < ReqCount - 1)
                {
                /* why continue? - NOTE this seems to break multi-req VPC's */

                if (getenv("MOABHOSTING") == NULL)
                  continue;
                }
              }   /* END if (DoAggregate == TRUE) */
            else
              {
              /* just copy over the nodelist from tmpMNL */

              MNLCopy(DstMNL[0],tmpMNL[aindex]);
              }      /* END else (DoAggregate == TRUE) */

            /* chop timeframe into up to pieces of length 'ReqTime' */
            /* report range for each piece */

            /* return up to MSched.ResourceQueryDepth available time slots */

            /* NOTE:  must determine node list for each timeframe as nodes capable of 
                      satisfying range may change */

            MNLToString(
              DstMNL[0],
              TRUE,
              ",",
              '\0',
              ReqHList[rindex],  /* O */
              MMAX_BUFFER);

            RangeCount[index]++;

            TC = 0;

            if ((ProcRequest[aindex] == TRUE) && (RQ[aindex]->TaskCount > 0))
              TCReq = RQ[aindex]->TaskCount;
            else
              TCReq = MSched.JobMaxTaskCount;

            for (NC = 0;MNLGetNodeAtIndex(DstMNL[0],NC,NULL) == SUCCESS;NC++)
              {
              /* NOTE:  nodes may be partitioned, cannot assume dedicated access */

              TC = MIN(TCReq,TC + MNLGetTCAtIndex(DstMNL[0],NC));
              }  /* END for (NC) */

            {
            char ReqACL[MMAX_LINE];
            char ReqOS[MMAX_NAME];

            int  TID;

            mxml_t *RE;

            ReqACL[0] = '\0';
            ReqOS[0]  = '\0';

            if (!bmisclear(&J[aindex]->AttrBM))
              {
              mstring_t tmp(MMAX_LINE);

              MJobAToMString(J[aindex],mjaGAttr,&tmp);

              snprintf(ReqACL,sizeof(ReqACL),"%s==%s",
                MAttrO[maJAttr],
                tmp.c_str());
              }

            if (RQ[aindex]->Opsys != 0)
              {
              strcpy(ReqOS,MAList[meOpsys][RQ[aindex]->Opsys]);
              }

            __MClusterRangeToXML(
                PE,
                Auth,
                C,
                STime,
                J[aindex]->WCLimit,
                aindex,
                NC,
                TC,
                &Flags,
                ReqACL,
                ReqOS,
                LabelList[aindex],
                VCDescList[aindex],
                VCMaster,
                ((RsvProfile[aindex][0] == '\0') && (aindex < EReqCount)) ? 
                  OpRsvProfile : 
                  RsvProfile[aindex],
                ProfileLine,
                &RQ[aindex]->DRes,
                index,
                ReqHList[rindex],  /* I */
                &RQ[aindex]->ReqFBM,
                (aindex >= EReqCount) ? FALSE : TRUE,
                &TID,  /* O */
                &RE);  /* O */

            RangesPrinted[aindex]++;
            TotalRangesPrinted++;

            if (MergeTID == TRUE)
              {
              if (aindex == 0)
                {
                FirstTID = TID;
                }
              else
                {
                if (tmpTIDList[0] == '\0')
                  {
                  sprintf(tmpTIDList,"%d",
                    TID);
                  }
                else
                  { 
                  char tmpVal[MMAX_NAME];

                  sprintf(tmpVal,",%d",
                    TID);

                  strcat(tmpTIDList,tmpVal);
                  }
                } 
              }    /* END if (MergeTID == TRUE) */

            if (CmdLine[0] != '\0')
              {
              MTransSet(TID,mtransaMShowCmd,CmdLine);
              }

            /* if there is a ACL string, then set in the MTrans */
            if (ACLLine[aindex] != NULL)
              {
              MTransSet(TID,mtransaACL,ACLLine[aindex]->c_str());
              }

            if (!Variables[aindex]->empty())
              {
              MTransSet(TID,mtransaVariables,Variables[aindex]->c_str());
              }

            if (J[aindex]->VMUsagePolicy != mvmupNONE)
              {
              MTransSet(TID,mtransaVMUsage,(char *)MVMUsagePolicy[J[aindex]->VMUsagePolicy]);
              }

            if (bmisset(&Flags,mcmPolicy))
              {
              double Cost = 0.0;

              mnode_t *N;

              char *TokPtr;
              char *ptr;

              char  tmpName[MMAX_NAME];

              /* extract node for hostlist string */

              /* FORMAT:  <HOST>:<TC>[,<HOST>:<TC>]... */

              MUStrCpy(tmpName,ReqHList[rindex],sizeof(tmpName));

              ptr = MUStrTok(tmpName,",:",&TokPtr);

              if ((ptr == NULL) || (MNodeFind(ptr,&N) == FAILURE))
                {
                Cost = 0.0;
                }
              else
                {
                int PC;

                /* determine cost and associate it with range XML */

                PC = J[aindex]->TotalProcCount;

                if ((J[aindex]->Req[0]->DRes.Procs == 0) && (!MSNLIsEmpty(&J[aindex]->Req[0]->DRes.GenericRes)))
                  {
                  /* for gres only jobs J->TotalProcCount is still 1 */

                  PC = 0;
                  }

                MRsvGetCost(
                  NULL,
                  C,
                  STime,
                  J[aindex]->WCLimit,
                  TC,
                  PC,
                  NC,
                  &RQ[aindex]->DRes,
                  N,
                  FALSE,   /* query is for full rsv, not partial */
                  Variables[aindex]->c_str(),
                  &Cost);  /* O */
                }

              /* apply environmental adjustments */

              Cost += MLocalApplyCosts(J[aindex]->WCLimit,TC,CostVariables[aindex]);

              MXMLSetAttr(RE,"cost",&Cost,mdfDouble);

              sprintf(tmpName,"%.2f",
                Cost);

              if (Cost != 0.0)
                MXMLSetAttr(RE,"cost",tmpName,mdfString);

              MTransSet(TID,mtransaCost,tmpName);
              }  /* END if (bmisset(&Flags,mcmPolicy)) */
            }  /* END block */

            if (J[aindex]->WCLimit <= 0)
              {
              /* request time not specified */

              break;
              }
            }    /* END for (aindex) */

          if (MergeTID == TRUE)
            {
            /* allow user to request specified TID and has all associated
               dependent TID's automatically commited */

            if ((FirstTID != 0) && (tmpTIDList[0] != '\0'))
              {
              MTransSet(FirstTID,mtransaDependentTIDList,tmpTIDList);
              }
            }
          }      /* END for (index) */
        }        /* END for (rindex) */
      }          /* END else ((SubmitTID != TRUE) || bmisset(&FlagBM,mcmVerbose)) */
    }            /* END for (pindex) */

  /* Step 3:  Finalize */

  /* free host lists */

  for (rindex = 0;rindex < MMAX_RANGE;rindex++)
    {
    if (ReqHList[rindex] != NULL)
      MUFree(&ReqHList[rindex]);
    }

  /* free dynamically allocated job and req structures */
  /* free ReqRID, ReqVPC, J->RsvAList */

  for (aindex = 0;aindex < ReqCount;aindex++)
    {
    if (RQ[aindex]->OptReq != NULL)
      {
      MReqOptReqFree(&RQ[aindex]->OptReq); 
      }

    if (RQ[aindex]->NestedNodeSet != NULL)
      {
      MReqFreeNestedNodeSet(&RQ[aindex]->NestedNodeSet);
      }
    }    /* END for (aindex) */

  MDB(2,fUI) MLog("END %s(%.32s,%s,%s,%s,SReq,DRsp,%d,%d,NSIsComplete,EMsg)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL",
    MWireProtocol[DFormat],
    (RRange != NULL) ? "RRange" : "NULL",
    MWireProtocol[SFormat],
    BufSize,
    NSIndex);

  if (TotalRangesPrinted == 0)
    {
    snprintf(EMsg,MMAX_LINE,"resources not available");

    FinalRC = FAILURE;
    }  /* END if (TotalRangesPrinted == 0) */

free_and_return:

  /* iterate over the various arrays for all instances and free resources */
  for (endindex = 0;endindex < aindex;endindex++)
    {
    MUFree(&NodeMap[endindex]);

    if (J[endindex] != NULL)
      {
      /* parts of this routine will set J[0]->Req[1] to J[1]->Req[0] (combining jobs), set it back to NULL
         here to avoid double-free */

      J[endindex]->Req[1] = NULL;

      MJobFreeTemp(&J[endindex]);
      }

    if (Variables[endindex] != NULL)
      {
      delete Variables[endindex];
      Variables[endindex] = NULL;
      }

    if (ACLLine[aindex] != NULL)
      {
      delete ACLLine[endindex];
      ACLLine[endindex] = NULL;
      }
    }
   
  if (aindex == 0) /* handle aindex is 0 case */
    MJobFreeTemp(&J[aindex]);

  MNLMultiFree(RequestedHostList);
  MNLMultiFree(CAHostList);
  MNLMultiFree(tmpMNL);
  MNLMultiFree(DstMNL);
  MNLMultiFree(coMNL);
  MNLMultiFree(DList);
  MNLMultiFree(AllocNodeList);
  MNLMultiFree(tmp);

  return(FinalRC);
  }  /* END MClusterShowARes() */






/**
 * Report XML containing 'mshow -a' response.
 *
 * @param DE (O) [populated]
 * @param Auth (I) [optional]
 * @param C (I) [optional]
 * @param STime (I)
 * @param Duration (I)
 * @param ReqIndex (I)
 * @param NC (I)
 * @param TC (I)
 * @param FlagBM (I)
 * @param FString (I) [optional]
 * @param OString (I) [optional]
 * @param Label (I) [optional]
 * @param VCDescription (I) [optional] - The description to put on a created VC
 * @param VCMaster (I) [optional] - Wraps elements of mshow -a
 * @param RProfile (I) [optional]
 * @param VProfile (I) [optional]
 * @param DRes (I) [optional]
 * @param Index (I) [optional]
 * @param HostList (I) when specified, update trans id [optional]
 * @param AttrBM   (I) [optional]
 * @param DoOffset (I)
 * @param TIDP (O) [optional]
 * @param REP (O) [optional]
 */

int __MClusterRangeToXML(

  mxml_t   *DE,        /* O (populated) */
  char     *Auth,      /* I (optional) */
  mvpcp_t  *C,         /* I (optional) */
  long      STime,     /* I */
  long      Duration,  /* I */
  int       ReqIndex,  /* I */
  int       NC,        /* I */
  int       TC,        /* I */
  mbitmap_t *FlagBM,   /* I */
  char     *FString,   /* I node feature (optional) */
  char     *OString,   /* I operating system (optional) */
  char     *Label,     /* I (optional) */
  char     *VCDescription, /* I (optional) */
  char     *VCMaster,  /* I (optional) */
  char     *RProfile,  /* I (optional) */
  char     *VProfile,  /* I (optional) */
  mcres_t  *DRes,      /* I (optional) */
  int       Index,     /* I (optional) */
  char     *HostList,  /* I (optional) when specified, update trans id */
  mbitmap_t *AttrBM,   /* I (optional) */
  mbool_t   DoOffset,  /* I */
  int      *TIDP,      /* O (optional) */
  mxml_t  **REP)       /* O (optional) */
 
  {
  char TString[MMAX_LINE];
  char FeatureString[MMAX_LINE];

  mxml_t *RE = NULL;

  if (TIDP != NULL)
    *TIDP = 0;

  if (DE == NULL)
    {
    return(FAILURE);
    }

  MXMLCreateE(&RE,"range");

  if ((C != NULL) && bmisset(FlagBM,mcmUser) && (DoOffset == TRUE))
    {
    long Start = STime;
    long Dur   = Duration;

    if (C->ReqStartPad > 0)
      {
      Start += C->ReqStartPad;

      Dur -= C->ReqStartPad;
      }

    if (C->ReqEndPad > 0)
      {
      Dur -= C->ReqEndPad;
      }

    MXMLSetAttr(RE,"starttime",(void *)&Start,mdfLong);
    MXMLSetAttr(RE,"duration",(void *)&Dur,mdfLong);
    }
  else
    {
    MXMLSetAttr(RE,"starttime",(void *)&STime,mdfLong);
    MXMLSetAttr(RE,"duration",(void *)&Duration,mdfLong);
    }

  MXMLSetAttr(RE,"reqid",(void *)&ReqIndex,mdfInt);
  MXMLSetAttr(RE,"nodecount",(void *)&NC,mdfInt);
  MXMLSetAttr(RE,"proccount",(void *)&TC,mdfInt);
  MXMLSetAttr(RE,"index",(void *)&Index,mdfInt);

  if ((OString != NULL) && (OString[0] != '\0'))
    MXMLSetAttr(RE,"os",(void *)OString,mdfString);

  if ((Label != NULL) && (Label[0] != '\0'))
    MXMLSetAttr(RE,"label",(void *)Label,mdfString);

  if ((VCDescription != NULL) && (VCDescription[0] != '\0'))
    MXMLSetAttr(RE,"vcdesc",(void *)VCDescription,mdfString);

  if (RProfile != NULL)
    MXMLSetAttr(RE,"rsvprofile",(void *)RProfile,mdfString);

  if ((FString != NULL) && (FString[0] != '\0'))
    {
    /* node feature is not used (no corresponding transaction attr) */

    MXMLSetAttr(RE,"nodefeature",(void *)FString,mdfString);
    }

  FeatureString[0] = '\0';

  if ((AttrBM != NULL) && (!bmisclear(AttrBM)))
    {
    MUNodeFeaturesToString(':',AttrBM,FeatureString);
    }

  if (DRes != NULL)
    {
    /* NOTE:  multi-task request needs hack for multiple tasks on same node--adjust DRes if needed */
    /* FIXME: instead of hostlist use tasklist in TID to store task per node info */

    int tprocs = DRes->Procs;

    MCResToString(DRes,0,mfmAVP,TString);

    DRes->Procs = tprocs;
    }
  else
    {
    TString[0] = '\0';
    }

  if (bmisset(FlagBM,mcmVerbose))
    {
    if (HostList != NULL)
      MXMLSetAttr(RE,"hostlist",(void *)HostList,mdfString);
    }  /* END if (bmisset(&FlagBM,mcmVerbose)) */

  if (bmisset(FlagBM,mcmUseTID))
    {
    char FlagLine[MMAX_LINE];
    int  TID;
    long tmpL;

    char tmpDurationString[MMAX_NAME];
    char tmpSTimeString[MMAX_NAME];

    /* must record hostlist and duration */

    sprintf(tmpDurationString,"%ld",
      MAX(1,Duration));

    sprintf(tmpSTimeString,"%ld",
      STime);

    MTransAdd(
      NULL,
      Auth,
      HostList,
      tmpDurationString,
      tmpSTimeString, 
      bmtostring(FlagBM,MClientMode,FlagLine),
      OString,
      TString,
      RProfile,
      Label,
      VCDescription,
      VCMaster,
      NULL,     /* NOTE:  TIDList populated externally */
      NULL,
      VProfile,
      NULL,
      FeatureString,
      &TID);

    if (TIDP != NULL)
      *TIDP = TID;

    tmpL = (long)TID;

    MXMLSetAttr(RE,"tid",(void *)&tmpL,mdfLong);
    }  /* END if (bmisset(&FlagBM,mcmUseTID)) */

  MXMLAddE(DE,RE);

  if (REP != NULL)
    *REP = RE;

  return(SUCCESS);
  }  /* END __MClusterRangeToXML() */





/**
 * Find VPC profile with specified name 
 *
 * NOTE:  search is case-insensitive
 *
 * @see MVPCProfileAdd() - peer
 *
 * @param Name (I)
 * @param CP (I) [optional]
 */

int MVPCProfileFind(

  char      *Name,    /* I */
  mvpcp_t  **CP)      /* I (optional) */

  {
  /* if found, return success with CP pointing to mvcp */

  int      cindex;
  mvpcp_t *C;

  if (CP != NULL)
    *CP = NULL;

  if ((Name == NULL) ||
      (Name[0] == '\0'))
    {
    return(FAILURE);
    }

  for (cindex = 0;cindex < MVPCP.NumItems;cindex++)
    {
    C = (mvpcp_t *)MUArrayListGet(&MVPCP,cindex);;

    if ((C == NULL) || (C->Name[0] == '\0'))
      {
      break;
      }

    if (strcasecmp(C->Name,Name) != 0)
      continue;

    /* found */

    if (CP != NULL)
      *CP = C;

    return(SUCCESS);
    }  /* END for (cindex) */

  return(FAILURE);
  }  /* END MVCProfileFind() */



/**
 * Create VPC Profile with specified name.
 *
 * @see MVPCProfileAdd() - parent
 *
 * @param Name (I)
 * @param C (O) [optional]
 */

int MVPCProfileCreate(

  char    *Name,
  mvpcp_t *C)

  {
  if (C == NULL)
    {
    return(FAILURE);
    }

  memset(C,0,sizeof(mvpcp_t));

  if ((Name != NULL) && (Name[0] != '\0'))
    {
    MUStrCpy(C->Name,Name,sizeof(C->Name));
    }

  return(SUCCESS);
  }  /* END MVPCProfileCreate() */



/**
 * Add VPC Profile with specified name.
 *
 * @see MVPCProfileFind() - peer
 *
 * @param Name (I)
 * @param CP (O) [optional]
 */

int MVPCProfileAdd(

  char     *Name,    /* I */
  mvpcp_t **CP)      /* O (optional) */

  {
  int  cindex;

  mvpcp_t *C;

  const char *FName = "MVPCProfileAdd";

  MDB(5,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (Name != NULL) ? "Name" : "NULL",
    (CP != NULL) ? "CP" : "NULL");

  if ((Name == NULL) || (Name[0] == '\0'))
    {
    return(FAILURE);
    }

  if (CP != NULL)
    *CP = NULL;

  for (cindex = 0;cindex < MVPCP.NumItems;cindex++)
    {
    C = (mvpcp_t *)MUArrayListGet(&MVPCP,cindex);

    if ((C != NULL) && !strcasecmp(C->Name,Name))
      {
      /* C already exists */

      if (CP != NULL)
        *CP = C;

      return(SUCCESS);
      }

    if ((C != NULL) &&
        ((C->Name[0] == '\0') ||
         (C->Name[0] == '\1')))
      {
      /* found cleared-out vpcprofile slot */

      break;
      }
    }  /* END for (cindex) */

  if (cindex >= MVPCP.NumItems)
    {
    mvpcp_t NewVPCP;

    /* append new standing reservation record */

    if (MVPCProfileCreate(Name,&NewVPCP) == FAILURE)
      {
      MDB(1,fALL) MLog("ERROR:    cannot clear memory for VPCP '%s'\n",
        Name);

      return(FAILURE);
      }

    MUArrayListAppend(&MVPCP,&NewVPCP);

    /* get pointer to newly added srsv */

    cindex = MVPCP.NumItems-1;
    C = (mvpcp_t *)MUArrayListGet(&MVPCP,cindex);
    }

  /* create/initialize new record */

  memset(C,0,sizeof(mvpcp_t));

  MUStrCpy(C->Name,Name,sizeof(C->Name));

  if (CP != NULL)
    *CP = C;

  C->Index = cindex;

  /* update from checkpoint? (NYI) */

  return(SUCCESS);
  }  /* END MVPCProfileAdd() */





/**
 * Convert VPC profile to human-readable or XML string.
 *
 * NOTE:  process 'mschedctl -l vpcprofile'
 *
 * @see MVPCProfileToXML() - peer
 * @see MUISchedCtl() - parent
 * @see MVPCShow() - peer - show VPC
 *
 * @param SC (I) [optional]
 * @param Auth (I)
 * @param String (O)
 * @param IsAdmin (I)
 */

int MVPCProfileToString(

  mvpcp_t   *SC,
  char      *Auth,
  mstring_t *String,
  mbool_t    IsAdmin)

  {
  mvpcp_t *C;

  int cindex;

  mln_t  *tmpL;

  mgcred_t *U = NULL; /* NOTE:  map Auth to user 'peer:X' */
  mgcred_t *A = NULL; /* NOTE:  map Auth to acct 'X' */

  char tmpLine[MMAX_LINE];

  mbool_t IsPeer = FALSE;

  /* if Auth specified, only show accessible profiles */

  if (String == NULL)
    {
    return(FAILURE);
    }

  if (Auth != NULL)
    {
    /* Format: [PEER:]<CRED> */

    if (!strncasecmp(Auth,"peer:",strlen("peer:")))
      {
      IsPeer = TRUE;

      MAcctFind(Auth + strlen("peer:"),&A);
      }

#ifdef FUTURE
    MVPCProfileCheckCredAccess(SC,Auth);
#endif

    /* determine accessible Q/A creds */

    MUserFind(Auth,&U);
    }  /* END if (Auth != NULL) */

  for (cindex = 0;cindex < MVPCP.NumItems;cindex++)
    {
    C = (mvpcp_t *)MUArrayListGet(&MVPCP,cindex);

    if ((C == NULL) || (C->Name[0] == '\0'))
      {
      break;
      }

    if ((SC != NULL) && (SC != C))
      {
      /* profile does not match specified request */

      continue;
      }

    if (((IsAdmin == FALSE) || (IsPeer == TRUE)) && 
         (Auth != NULL) && 
         (!MACLIsEmpty(C->ACL)))
      {
      /* NOTE:  even if requestor is admin, enforce ACL if requestor is peer */

      /* verify requestor can access profile */

      if (((A != NULL) && (MCredCheckAccess(A,NULL,maAcct,C->ACL) == SUCCESS)) ||
          ((U != NULL) && (MCredCheckAccess(U,NULL,maUser,C->ACL) == SUCCESS)))
        {
        /* access allowed */
   
        /* NO-OP */
        }
      else
        {
        /* requestor cannot view this VPC */
         
        continue;
        }
      }

    MStringAppendF(String,"VPCPROFILE[%s] -------\n",
      C->Name);

    if (C->Description != NULL)
      {
      MStringAppendF(String,"Description:  '%s'\n",
        C->Description);
      }

    if (C->Priority != 0)
      {
      MStringAppendF(String,"Priority:     %ld\n",
        C->Priority);
      }

    if ((C->OpRsvProfile != NULL) && (C->OpRsvProfile[0] != '\0'))
      {
      MStringAppendF(String,"RsvProfile:   %s\n",
        C->OpRsvProfile);
      }

    if (C->ReqDefAttrs != NULL)
      {
      MStringAppendF(String,"ReqDefAttrs:  %s\n",
        C->ReqDefAttrs);
      }

    if (C->ReqSetAttrs != NULL)
      {
      MStringAppendF(String,"ReqSetAttrs:  %s\n",
        C->ReqSetAttrs);
      }

    if (C->MinDuration > 0)
      {
      char TString[MMAX_LINE];

      MULToTString(C->MinDuration,TString);

      MStringAppendF(String,"MinDuration:  %s\n",
        TString);
      }

    if (C->ReqStartPad != 0)
      {
      char TString[MMAX_LINE];

      MULToTString(C->ReqStartPad,TString);

      MStringAppendF(String,"StartPad:     %s\n",
        TString);
      }

    if (C->ReqEndPad != 0)
      {
      char TString[MMAX_LINE];

      MULToTString(C->ReqEndPad,TString);

      MStringAppendF(String,"EndPad:       %s\n",
        TString);
      }

    if (C->PurgeDuration > 0)
      {
      char TString[MMAX_LINE];

      MULToTString(C->PurgeDuration,TString);

      MStringAppendF(String,"PurgeTime:    %s\n",
        TString);
      }

    if (C->ReAllocPolicy != mralpNONE)
      {
      MStringAppendF(String,"ReAllocPolicy: %s\n",
        MRsvAllocPolicy[C->ReAllocPolicy]);
      }

    if (!bmisclear(&C->Flags))
      {
      char Line[MMAX_LINE];

      MStringAppendF(String,"Flags:        %s\n",
        bmtostring(&C->Flags,MVPCPFlags,Line));
      }

    if (!MACLIsEmpty(C->ACL))
      {
      mbitmap_t Flags;

      char tmpLine[MMAX_LINE];

      bmset(&Flags,mfmHuman);
      bmset(&Flags,mfmVerbose);

      MACLListShow(C->ACL,maNONE,&Flags,tmpLine,sizeof(tmpLine));

      MStringAppendF(String,"ACL:          %s\n",
        tmpLine);
      }

    tmpL = C->QueryData;

    while (tmpL != NULL)
      { 
      MXMLToString((mxml_t *)tmpL->Ptr,tmpLine,sizeof(tmpLine),NULL,TRUE);

      MStringAppendF(String,"CoAlloc[%s]    %s\n",
        tmpL->Name,
        tmpLine);

      tmpL = tmpL->Next;
      }

    if (C->NodeHourAllocChargeRate != 0.0)
      {
      /* NOTE:  nodehour charge rate can be very small, ie < .00001 */

      MStringAppendF(String,"NodeHour ChargeRate: %.5f\n",
        C->NodeHourAllocChargeRate);
      }

    if (C->NodeSetupCost != 0.0)
      {
      MStringAppendF(String,"Node Setup Cost:     %.2f\n",
        C->NodeSetupCost);
      }

    if (C->SetUpCost != 0.0)
      {
      MStringAppendF(String,"VPC Setup Cost:      %.2f\n",
        C->SetUpCost);
      }

    if (C->OptionalVars != NULL)
      {
      MStringAppendF(String,"OptionalVars=%s\n",
        C->OptionalVars);
      }

    MStringAppendF(String,"\n");
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MVPCProfileToString() */





/**
 * Convert VPC profile to XML (handle 'mschedctl -l vpcprofile' request).
 *
 * @see MVPCProfileToString() - peer - report as human readable
 * @see MUISchedCtl() - parent
 *
 * @param SC (I) [optional]
 * @param Auth (I) [optional]
 * @param E (O)
 * @param IsAdmin (I)
 * @param Flags (I) [bitmap of ???,not used]
 */

int MVPCProfileToXML(

  mvpcp_t *SC,      /* I (optional) */
  char    *Auth,    /* I (optional) */
  mxml_t  *E,       /* O */
  mbool_t  IsAdmin, /* I */
  int      Flags)   /* I (bitmap of ???,not used) */

  {
  mln_t    *tmpL;

  mxml_t   *VE;
  mxml_t   *CE;

  mxml_t   *tE;

  mgcred_t *U = NULL;

  int       cindex;

  mvpcp_t  *C;

  if (Auth != NULL)
    {
    /* determine accessible Q/A creds */

    MUserFind(Auth,&U);
    }  /* END if (Auth != NULL) */

  for (cindex = 0;cindex < MVPCP.NumItems;cindex++)
    {
    C = (mvpcp_t *)MUArrayListGet(&MVPCP,cindex);

    if ((SC != NULL) && (C != SC))
      continue;

    if ((C == NULL) || (C->Name[0] == '\0'))
      {
      break;
      }

    if ((IsAdmin == FALSE) && (Auth != NULL) && (!MACLIsEmpty(C->ACL)))
      {
      /* verify requestor can access profile */
      
      if (MCredCheckAccess(U,NULL,maUser,C->ACL) == FAILURE)
        {
        /* user cannot view VPC profile */

        continue;
        }
      }

    VE = NULL;

    MXMLCreateE(&VE,"VCPROFILE");

    if (C->Description != NULL)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaDescription],(void *)C->Description,mdfString);

    MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaName],(void *)C->Name,mdfString);

    if (C->OpRsvProfile != NULL)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaOpRsvProfile],(void *)C->OpRsvProfile,mdfString);

    if (C->OptionalVars != NULL)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaOptionalVars],(void *)C->OptionalVars,mdfString);

    if (C->ReqStartPad != 0)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaReqStartPad],(void *)&C->ReqStartPad,mdfLong);

    if (C->ReqEndPad != 0)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaReqEndPad],(void *)&C->ReqEndPad,mdfLong);

    if (C->MinDuration != 0)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaMinDuration],(void *)&C->MinDuration,mdfLong);

    if (C->NodeHourAllocChargeRate != 0.0)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaNodeHourChargeRate],(void *)&C->NodeHourAllocChargeRate,mdfDouble);

    if (C->NodeSetupCost != 0.0)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaNodeSetupCost],(void *)&C->NodeSetupCost,mdfDouble); 

    if (C->ReqSetAttrs != NULL)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaReqSetAttr],(void *)C->ReqSetAttrs,mdfString);

    if (C->Priority != 0)
      MXMLSetAttr(VE,(char *)MVPCPAttr[mvpcpaPriority],(void *)&C->Priority,mdfLong);

    if (!MACLIsEmpty(C->ACL))
      {
      macl_t *tACL;

      mxml_t *AE;
     
      for (tACL = C->ACL;tACL != NULL;tACL = tACL->Next)
        {
        MACLToXML(tACL,&AE,NULL,FALSE);
     
        MXMLAddE(VE,AE);
        }
      } 

    tmpL = C->QueryData;

    while (tmpL != NULL)
      {
      CE = NULL;

      MXMLCreateE(&CE,tmpL->Name);

      /* MUST dup Ptr XML */

      tE = NULL;

      MXMLDupE((mxml_t *)tmpL->Ptr,&tE);

      MXMLAddE(CE,tE);

      MXMLAddE(VE,CE);

      tmpL = tmpL->Next;
      }   /* END while (tmpL != NULL) */

    MXMLAddE(E,VE);
    }   /* END for (cindex) */

  return(SUCCESS);
  }  /* END MVPCProfileToXML() */





/**
 * Process request co-allocation requirements to determine feasible time range 
 * and hostlist info for specified partition.
 *
 * NOTE:  If no co-allocation requirements specified within request, then no 
 *        additional hostlist or timeframe constraints need to be applied and
 *        routine should return SUCCESS.
 *
 * @see MClusterShowARes() - parent
 * @see MJobGetMRRange() - child
 *
 * @param P (I)
 * @param CoAlloc (I)
 * @param Offset (I) 
 * @param RRange (I)
 * @param JList (I)
 * @param RQList (I)
 * @param CARange (O)
 * @param CAHostList (O)
 * @param SeekLong (I)
 * @param EMsg (O) [optional]
 */

int __MClusterGetCoAllocConstraints(

  mpar_t     *P,                        /* I */
  int         CoAlloc[][MMAX_REQS + 1], /* I - list of co-allocation groupings requested */
  long        Offset[],                 /* I */
  mrange_t   *RRange,                   /* I */
  mjob_t    **JList,                    /* I */
  mreq_t    **RQList,                   /* I */
  mrange_t    CARange[][MMAX_RANGE],    /* O */
  mnl_t     **CAHostList,               /* O */
  mbool_t     SeekLong,                 /* I */
  char       *EMsg)                     /* O (optional) */

  {
  mjob_t tmpJ;

  int    caindex;

  int    rqindex;
  int    aindex;

  int    rindex;
  int    nindex;

  mulong StartTime;

  mulong Offsets[MMAX_REQ_PER_JOB];

  int    OTC[MMAX_REQ_PER_JOB] = {0};

  mnl_t *MNL[MMAX_REQ_PER_JOB];

  int rc;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (CARange != NULL)
    {
    for (rindex = 0;rindex < MMAX_REQS;rindex++)
      CARange[rindex][0].ETime = 0;
    }

  if (CAHostList != NULL)
    {
    MNLMultiClear(CAHostList);
    }

  if ((CoAlloc == NULL) || (CoAlloc[0][0] == -1))
    {
    /* no co-allocation requests located */

    return(SUCCESS);
    }

  /* NOTE:  if co-alloc specified, must build multi-req job
            and use MJobGetMRRange() */

  /* each req should require only one task and a resulting
     constraining hostlist and GRange should be produced */

  MNLMultiInit(MNL);

  for (aindex = 0;aindex < MMAX_REQS;aindex++)
    {
    if (CoAlloc[aindex][0] == -1)
      break;

    memcpy(&tmpJ,JList[aindex],sizeof(tmpJ));

    for (rqindex = 0;rqindex < MMAX_REQS;rqindex++)
      {
      if (CoAlloc[aindex][rqindex] == -1)
        break;

      tmpJ.Req[rqindex] = RQList[CoAlloc[aindex][rqindex]];

      /* pass in req duration */

      tmpJ.Req[rqindex]->RMWTime = JList[CoAlloc[aindex][rqindex]]->WCLimit; 

      /* pass in req taskcount */

      OTC[rqindex] = tmpJ.Req[rqindex]->TaskCount; 
      tmpJ.Req[rqindex]->TaskCount = JList[CoAlloc[aindex][rqindex]]->Request.TC; 

      /* roll in req offset */

      Offsets[rqindex] = Offset[CoAlloc[aindex][rqindex]];
      }

    if ((RRange != NULL) && (RRange[aindex].STime > 0))
      StartTime = RRange[aindex].STime;
    else
      StartTime = MSched.Time;

    /* This routine will do the heavy lifting, for each req in this multi-req psuedo job
       it will find a hostlist and a range, and then it will AND the hostlist and ranges
       to return a final single hostlist for all reqs within the co-alloc */

    rc = MJobGetMRRange(
      &tmpJ,            /* I  job description                          */
      Offsets,          /* I  job req time offset list                 */
      P,                /* I  partition in which resources must reside */
      StartTime,        /* I  earliest possible starttime              */
      CARange[aindex],  /* O  array of time ranges located (optional)  */
      MNL,              /* O  multi-req list of nodes found            */
      NULL,             /* O  number of available nodes located        */
      NULL,             /* O  nodemap (optional)                       */
      0,                /* I  RTBM (bitmap)                            */
      SeekLong,         /* I                                           */
      NULL,             /* O  TRange                                   */
      EMsg);            /* O */

    /* restore original req settings */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (tmpJ.Req[rqindex] == NULL)
        break;

      tmpJ.Req[rqindex]->TaskCount = OTC[rqindex]; 
      }  /* END for (rqindex) */

    if (rc == FAILURE)
      {
      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"cannot locate available range");

      MNLMultiFree(MNL);

      return(FAILURE);
      }

    /* migrate MNL to CAHostList[aindex] (NYI) */

    for (caindex = 0;CoAlloc[aindex][caindex] != -1;caindex++)
      {
      for (nindex = 0;MNLGetNodeAtIndex(MNL[0],nindex,NULL) == SUCCESS;nindex++)
        {
        if (CAHostList != NULL)
          {
          MNLSetNodeAtIndex(CAHostList[CoAlloc[aindex][caindex]],nindex,MNLReturnNodeAtIndex(MNL[0],nindex));
          MNLSetTCAtIndex(CAHostList[CoAlloc[aindex][caindex]],nindex,1);
          }
        }
      }
    }  /* END for (aindex) */

  MNLMultiFree(MNL);

  return(SUCCESS);
  }  /* END __MClusterGetCoAllocConstraints() */

/* END MCluster.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
