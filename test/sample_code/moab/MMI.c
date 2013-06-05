/* HEADER */

/**
 * @file MMI.c
 *
 * Moab Interface
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"




/* NOTE:  create serial launcher to set SID, obtain PID (NYI) */

/* local prototypes */


int MMPing(mrm_t *,enum MStatusCodeEnum *);
int MMGetData(mrm_t *,mbool_t,int,mbool_t *,char *,enum MStatusCodeEnum *);
mbool_t MMDataIsLoaded(mrm_t *,enum MStatusCodeEnum *);
int MMJobStart(mjob_t *,mrm_t *,char *,enum MStatusCodeEnum *);
int MMJobSubmit(const char *,mrm_t *,mjob_t **,mbitmap_t *,char *,char *,enum MStatusCodeEnum *);
int MMJobRequeue(mjob_t *,mrm_t *,mjob_t **,char *,int *);
int MMJobModify(mjob_t *,mrm_t *,const char *,const char *,const char *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
int MMJobCancel(mjob_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MMJobSignal(mjob_t *,mrm_t *,int,char *,char *,int *);
int MMJobSuspend(mjob_t *,mrm_t *,char *,int *);
int MMJobResume(mjob_t *,mrm_t *,char *,int *);
int MMClusterQuery(mrm_t *,int *,char *,enum MStatusCodeEnum *);
int MMWorkloadQuery(mrm_t *,int *,int *,char *,enum MStatusCodeEnum *);
int MMJobLoad(char *,mxml_t *,mjob_t *,int *,mrm_t *);
int MMJobUpdate(mxml_t *,mjob_t *,int *,mrm_t *);
int MMNodeLoad(mnode_t *,mxml_t *,enum MNodeStateEnum,mrm_t *);
int MMNodeUpdate(mnode_t *,mxml_t *,enum MNodeStateEnum,mrm_t *);
int MMNodeModify(mnode_t *,mvm_t *,char *,mrm_t *,enum MNodeAttrEnum,void *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
int MMRsvCtl(mrsv_t *,mrm_t *,enum MRsvCtlCmdEnum,void **,char *,enum MStatusCodeEnum *);
int MMQueueModify(mclass_t *,mrm_t *,enum MClassAttrEnum,char *,void *,char *,enum MStatusCodeEnum *);
int MMJobOMap(mxml_t *,mrm_t *);
int MMNodeOMap(mxml_t *,mrm_t *);
int MMFailure(mrm_t *,enum MStatusCodeEnum SC);
int MMSystemModify(mrm_t *,const char *,const char *,mbool_t,char *,enum MFormatModeEnum,char *,enum MStatusCodeEnum *);
int __MMJobGetState(mxml_t *,mrm_t *,char *,enum MJobStateEnum *);
int __MMNodeGetState(mxml_t *,mrm_t *,char *,enum MNodeStateEnum *);
int MMGetQueueInfo(mnode_t *,char C[][MMAX_NAME],char A[][MMAX_NAME]);
int __MMGetTaskList(char *,int *,int *,int *);
int __MMJobSetAttr(mjob_t *,char *,char *);
int __MMNodeSetAttr(mnode_t *,mrm_t *,char *,char *);
char *__MMGetErrMsg(int);
int __MMDoCommand(mrm_t *,mpsi_t *,enum MSvcEnum,const char *,char *,int,char *,enum MStatusCodeEnum *);
int __MMNodeProcess(mnode_t *,mrm_t *,mnat_t *,char *,mbool_t,mbool_t,int *,int *);
int __MMAppendAuthListToACL(char **,enum MAttrEnum,macl_t **);
int __MMClusterQueryPostLoad(mrm_t *,int *,char *EMsg,enum MStatusCodeEnum *);
int __MMWorkloadQueryPostLoad(mrm_t *,int *,int *,char *,enum MStatusCodeEnum *);
int __MMInitializePostLoad(msocket_t *,void *,void *,void *,void *);
int __MMJobSubmitPostLoad(msocket_t *,void *,void *,void *,void *);
int __MMJobSubmitPostLoadByJobName(msocket_t *,void *,void *,void *,void *);
int __MMConvertNLToPeers(char *,char *,int);
int __MMWorkloadQueryGetData(msocket_t *,void *,void *,void *,void *);
int __MMClusterQueryGetData(msocket_t *,void *,void *,void *,void *);
int __MMCreateDynamicResources(mrm_t *R,char *,mrm_t **,enum MFormatModeEnum,char *,char *,enum MStatusCodeEnum *);
int __MMIncreaseDynamicResources(mrm_t *,char *,char *,enum MFormatModeEnum,char *,char *,enum MStatusCodeEnum *);
int __MMFindDynamicResourceMatch(char *,char *,char *,int,mrm_t *,mrm_t *,mrm_t **,mrm_t **);
int __MMJobSubmitGlobus(mjob_t *,mrm_t *,char *);



/* END local prototypes */



/**
 * Build list of Moab Peer Interface function calls.
 *
 * @param F (I) [modified]
 */

int MMLoadModule(

  mrmfunc_t *F)  /* I (modified) */

  {
  if (F == NULL)
    {
    return(FAILURE);
    }

  F->IsInitialized = TRUE;

  F->ClusterQuery    = MMClusterQuery;
  F->DataIsLoaded    = MMDataIsLoaded;
  F->GetData         = MMGetData;
  F->InfoQuery       = MMInfoQuery;
  F->JobCancel       = MMJobCancel;
  F->JobCheckpoint   = NULL;
  F->JobModify       = MMJobModify;
  F->JobRequeue      = MMJobRequeue;
  F->JobResume       = NULL;
  F->JobSignal       = MMJobSignal;
  F->JobStart        = MMJobStart;
  F->JobSubmit       = MMJobSubmit;
  F->JobSuspend      = NULL;
  F->QueueModify     = MMQueueModify;
  F->QueueQuery      = NULL;
  F->Ping            = MMPing;
  F->ResourceModify  = MMNodeModify;
  F->ResourceQuery   = NULL;
  F->RsvCtl          = MMRsvCtl;
  F->RMFailure       = MMFailure;
  F->RMInitialize    = MMInitialize;
  F->RMQuery         = NULL;
  F->WorkloadQuery   = MMWorkloadQuery;
  F->SystemModify    = MMSystemModify;

  return(SUCCESS);
  }  /* END MMLoadModule() */





/**
 * Initialize interface with peer Moab.
 *
 * @see MRMInitialize() - parent
 * @see MUIPeerSchedQuery() 
 *
 * @param R (I) [state modified]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMInitialize(

  mrm_t                *R,    /* I (state modified) */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  char tmpName[MMAX_NAME];
  char tmpLine[MMAX_LINE];

  char CmdBuf[MMAX_BUFFER];

  char tEMsg[MMAX_LINE];
  char *EMsgP;

  enum MStatusCodeEnum tmpSC;

  mxml_t *DE;
  mxml_t *RE = NULL;

  mbitmap_t Flags;

  const char *FName = "MMInitialize";

  MDB(1,fNAT) MLog("%s(%s,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';

    EMsgP = EMsg;
    }
  else
    {
    EMsgP = tEMsg;
    }

  if (SC != NULL)
    *SC = mscNoError;

  if (!MOLDISSET(MSched.LicensedFeatures,mlfGrid))
    {
    snprintf(EMsgP,MMAX_LINE,"current license file does not support peer connections");

    MMBAdd(
      &MSched.MB,
      "ERROR:  license does not allow for grid communication, please contact Adaptive Computing\n",
      NULL,
      mmbtOther,
      0,
      0,
      NULL);

    if (SC != NULL)
      *SC = mscSysFailure;
       
    return(FAILURE);
    }

#ifndef __MOPT
  if (R == NULL)
    {
    return(FAILURE);
    }
#endif /* !__MOPT */

  if (R->State == mrmsDisabled)
    {
    snprintf(EMsgP,MMAX_LINE,"RM interface disabled, must re-enable before initializing");

    if (SC != NULL)
      *SC = mscSysFailure;

    return(FAILURE);
    }
    
  if (bmisclear(&R->FnList))
    {
    /* mrmAlloc,mrmRMStart cannot be supported - too many fn types for int bitmap */

    if (R->ND.URL[mrmClusterQuery] != NULL)
      bmset(&R->FnList,mrmClusterQuery);

    if (R->ND.URL[mrmInfoQuery] != NULL)
      bmset(&R->FnList,mrmInfoQuery);

    if (R->ND.URL[mrmJobCancel] != NULL)
      bmset(&R->FnList,mrmJobCancel);

    if (R->ND.URL[mrmJobResume] != NULL)
      bmset(&R->FnList,mrmJobResume);

    if (R->ND.URL[mrmJobStart] != NULL)
      bmset(&R->FnList,mrmJobStart);

    if (R->ND.URL[mrmJobSubmit] != NULL)
      bmset(&R->FnList,mrmJobSubmit);

    if (R->ND.URL[mrmJobSuspend] != NULL)
      bmset(&R->FnList,mrmJobSuspend);

    if (R->ND.URL[mrmNodeModify] != NULL)
      bmset(&R->FnList,mrmNodeModify);

    if (R->ND.URL[mrmRMInitialize] != NULL)
      bmset(&R->FnList,mrmRMInitialize);

    if (R->ND.URL[mrmRMStart] != NULL)
      bmset(&R->FnList,mrmRMStart);

    if (R->ND.URL[mrmRMStop] != NULL)
      bmset(&R->FnList,mrmRMStop);

    if (R->ND.URL[mrmSystemModify] != NULL)
      bmset(&R->FnList,mrmSystemModify);

    if (R->ND.URL[mrmSystemQuery] != NULL)
      bmset(&R->FnList,mrmSystemQuery);

    if (R->ND.URL[mrmWorkloadQuery] != NULL)
      bmset(&R->FnList,mrmWorkloadQuery);
    }  /* END if (R->FnList == 0) */

  /* set defaults */
    
  if (bmisclear(&R->RTypes))
    {
    bmset(&R->RTypes,mrmrtCompute);
    }

  if (R->P[R->ActingMaster].RID == NULL)
    { 
    snprintf(tmpName,sizeof(tmpName),"PEER:%s",
      MSched.Name);
    
    MUStrDup(&R->P[R->ActingMaster].RID,tmpName);
    }
    
  /* Set to the default if not already configured in the moab.cfg file */

  if (R->StageThresholdConfigured != TRUE)
    {
    R->StageThreshold = MDEF_RMSTAGETHRESHOLD;
    }
                                                                                  
  /* check CS algo/key */
                                                                                         
  /* NYI */

  /* validate interfaces */

  /* create initialize request */

  /* initialize request and receive buffer */

  /* NOTE:  effectively issue 'mschedctl -q sched' */

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);

  MS3SetObject(RE,(char *)MXO[mxoSched],NULL);

  bmset(&Flags,mcmXML);

  bmtostring(&Flags,MClientMode,tmpLine);

  MXMLSetAttr(RE,"flags",(void *)&tmpLine,mdfString);

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MSchedCtlCmds[msctlQuery],
    mdfString);

  MXMLSetAttr(
    RE,
    MSAN[msanArgs],
    (void *)MXO[mxoSched],
    mdfString);

  if (bmisset(&R->Flags,mrmfRsvImport))
    {
    MXMLAppendAttr(
      RE,
      "rmflags",
      (char *)MRMFlags[mrmfRsvImport],
      ',');
    }

  MXMLToString(RE,CmdBuf,sizeof(CmdBuf),NULL,TRUE);

  MXMLDestroyE(&RE);

  R->P[R->ActingMaster].IsNonBlocking = TRUE;

  /* mark state as active so MMDoCommand will attempt init */

  MRMSetState(R,mrmsActive);

  if (__MMDoCommand(
        R,
        &R->P[R->ActingMaster],            /* I/O */
        mcsMSchedCtl,
        CmdBuf,
        NULL,
        0,
        EMsgP,    /* O */
        &tmpSC) == FAILURE)
    {
    if (tmpSC == mscNoEnt)
      {
      MRMSetState(R,mrmsDown);
      }
    else
      {
      MRMSetState(R,mrmsCorrupt);
      }

    if (EMsgP[0] != '\0')
      MMBAdd(&R->MB,EMsgP,NULL,mmbtNONE,0,0,NULL);

    if (SC != NULL)
      *SC = (enum MStatusCodeEnum)tmpSC;

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }  /* END if (__MMDoCommand() == FAILURE) */

  if ((R->P[R->ActingMaster].S == NULL) || (R->P[R->ActingMaster].S->RDE == NULL))
    {
    MRMSetState(R,mrmsCorrupt);

    MSUFree(R->P[R->ActingMaster].S);

    sprintf(EMsgP,"socket is empty");

    return(FAILURE);
    }
  
  DE = (mxml_t *)R->P[R->ActingMaster].S->RDE;
  
  if ((DE->Val != NULL) &&
      !strcmp(DE->Val,"PENDING"))
    {
    msocket_t *tmpS;
    
    /* non-blocking response - will get result later */

    MSUDup(&tmpS,R->P[R->ActingMaster].S);

    MUITransactionAdd(
      &MUITransactions,
      tmpS,
      R,
      NULL,
      __MMInitializePostLoad);

    /* prevent disconnect */
    
    R->P[R->ActingMaster].S->sd = -1;

    MSUFree(R->P[R->ActingMaster].S);

    return(SUCCESS);
    }

  MRMSetState(R,mrmsActive);

  /* process results */

  __MMInitializePostLoad(R->P[R->ActingMaster].S,R,NULL,NULL,NULL);

  MSUFree(R->P[R->ActingMaster].S);

  return(SUCCESS);
  }  /* END MMInitialize() */





/**
 * Send 'ping' request to verify peer Moab interface is responsive.
 *
 * @param R (I)
 * @param SC (O) [optional]
 */

int MMPing(

  mrm_t                *R,   /* I */
  enum MStatusCodeEnum *SC)  /* O (optional) */

  {
  const char *FName = "MMPing";

  MDB(1,fNAT) MLog("%s(%s,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (SC != NULL)
    {
    *SC = mscNoError;
    }

  /* NYI */

  return(SUCCESS);
  }  /* END MMPing() */





/**
 * Load cluster/workload data for specified iteration via Moab (peer grid) interface.
 *
 * NOTE:  If NonBlock is set to false, Moab will block and wait for results.
 *
 * @see MMClusterQuery() - parent
 * @see MMWorkloadQuery() - parent
 *
 * @param R (I) [modified]
 * @param NonBlock (I) [not yet used]
 * @param Iteration (I)
 * @param IsPending (O) [optional]
 * @param EMsg (O)
 * @param SC (O) [optional]
 */

int MMGetData(

  mrm_t                *R,         /* I (modified) */
  mbool_t               NonBlock,  /* I (not yet used) */
  int                   Iteration, /* I */
  mbool_t              *IsPending, /* O (optional) */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  static char CmdLine[MMAX_LINE];  /* NOT THREAD SAFE */

  static mbool_t IsInitialized = FALSE;

  enum MStatusCodeEnum tmpSC;

  mxml_t *DE;

  const char *FName = "MMGetData";

  MDB(1,fNAT) MLog("%s(%s,%s,%d,IsPending,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MBool[NonBlock],
    Iteration);

  /* load, but do not process raw cluster and workload data from Moab interface (peer grid) */

  if (SC != NULL)
    *SC = mscNoError;

  if (IsPending != NULL)
    *IsPending = FALSE;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (R->State != mrmsActive)
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is %s, skipping get data query\n",
      R->Name,
      MRMState[R->State]);

    if (SC != NULL)
      *SC = mscBadState;

    if (EMsg != NULL)
      strcpy(EMsg,"rm is down");

    return(FAILURE);
    }

  if ((MUITransactionFindType(__MMClusterQueryGetData,R,NULL) == SUCCESS) ||
      (MMDataIsLoaded(R,NULL) == TRUE))
    {
    /* don't queue another request until the previous one has returned */

    MDB(1,fNAT) MLog("INFO:     RM %s was already has a pending query - skipping get data query\n",
      R->Name);

    if (IsPending != NULL)
      *IsPending = TRUE;

    return(SUCCESS);
    }

  if (IsInitialized == FALSE)
    {
    /* initialize request and receive buffer */

    mxml_t *RE = NULL;
    mbitmap_t Flags;

    char tmpLine[MMAX_LINE];

    MXMLCreateE(&RE,(char*)MSON[msonRequest]);

    MS3SetObject(RE,(char *)MXO[mxoSched],NULL);

    bmset(&Flags,mcmXML);

    bmtostring(&Flags,MClientMode,tmpLine);

    MXMLSetAttr(RE,"flags",(void *)&tmpLine,mdfString);

    MXMLSetAttr(
      RE,
      MSAN[msanAction],
      (void *)MSchedCtlCmds[msctlQuery],
      mdfString);

    MXMLSetAttr(
      RE,
      MSAN[msanOp],
      (void *)"noauto",
      mdfString);

#ifdef __MMSPLITREQUESTS
    MXMLSetAttr(
      RE,
      MSAN[msanArgs],
      (void *)MXO[mxoCluster],
      mdfString);
#else /* __MMSPLITREQUESTS */
    MXMLSetAttr(
      RE,
      MSAN[msanArgs],
      (void *)MXO[mxoALL],
      mdfString);
#endif /* __MMSPLITREQUESTS */

    MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE);

    MXMLDestroyE(&RE);

    IsInitialized = TRUE;
    }  /* END if (IsInitialized == FALSE) */

  /* check to see if we just processed a pending request */

  if (R->NextUpdateTime <= MSched.Time)
    {
    R->P[R->ActingMaster].IsNonBlocking = TRUE;

    if (__MMDoCommand(
          R,
          &R->P[R->ActingMaster],      /* I/O */
          mcsMSchedCtl,
          CmdLine,
          NULL,
          0,
          EMsg,
          &tmpSC) == FAILURE)
      {
      if (SC != NULL)
        *SC = tmpSC;

      if (R->P[R->ActingMaster].S->StatusCode == msfEGServerBus)
        {
        /* re-initialize RM interface */

        /* MRMSetFailure is also called by parent function - is doing this here reduntant? */

        MRMSetFailure(R,mrmClusterQuery,mscBadResponse,NULL);  /* will set R->State = mrmsCorrupt */  /* SPAWNS ANOTHER THREAD! */

        /* FIXME: should we set this to 0 or to 5 minutes ago? */

        R->StateMTime = 0;
        }

      MSUFree(R->P[R->ActingMaster].S);

      return(FAILURE);
      }  /* END if (__MMDoCommand() == FAILURE) */

    if ((R->P[R->ActingMaster].S == NULL) || (R->P[R->ActingMaster].S->RDE == NULL))
      {
      if (EMsg != NULL)
        strcpy(EMsg,"no data");

      MSUFree(R->P[R->ActingMaster].S);

      return(FAILURE);
      }

    /* check for non-blocking response */

    DE = (mxml_t *)R->P[R->ActingMaster].S->RDE;

    R->P[R->ActingMaster].S->RDE = NULL;

    if ((DE->Val != NULL) &&
        !strcmp(DE->Val,"PENDING"))
      {
      msocket_t *tmpS;

      /* non-blocking response - register __MMClusterQueryGetData() to process results later */

      MSUDup(&tmpS,R->P[R->ActingMaster].S);

      MUITransactionAdd(               /* NOT THREAD SAFE */
        &MUITransactions,
        tmpS,
        R,
        NULL,
        __MMClusterQueryGetData);

      /* prevent disconnect */

      R->P[R->ActingMaster].S->sd = -1;

      if (IsPending != NULL)
        *IsPending = TRUE;
      }
    else
      {
      /* results available for immediate processing */

      /* NOTE:  CData should be destroyed within MRMUpdate() once processed */

      if (R->U.Moab.CData != NULL)
        {
        MXMLDestroyE((mxml_t **)&R->U.Moab.CData);
        }

      R->U.Moab.CData = (void *)DE;
      R->U.Moab.CMTime = MSched.Time;
      R->U.Moab.CIteration = Iteration;
      R->U.Moab.CDataIsProcessed = FALSE;

#ifndef __MMSPLITREQUEST
      R->U.Moab.WData = (void *)DE;
      R->U.Moab.WMTime = MSched.Time;
      R->U.Moab.WIteration = Iteration;
      R->U.Moab.WDataIsProcessed = FALSE;

      MDB(4,fNAT) MLog("INFO:     RM %s UM WIteration set to %d \n",
        R->Name,
        Iteration);

#endif  /* __MMSPLITREQUEST */

      R->P[R->ActingMaster].S->IsNonBlocking = FALSE;
      }  /* END else ((DE->Val != NULL) && ...) */
    }    /* END if (R->NextUpdateTime <= MSched.Time) */

  MSUFree(R->P[R->ActingMaster].S);

  return(SUCCESS);
  }  /* END MMGetData() */





/**
 * Report if cluster/workload data is loaded for specified RM.
 * 
 * NOTE: This function must remain thread safe!
 * @param R (I)
 * @param SC (O] [optional]
 */

mbool_t MMDataIsLoaded(

  mrm_t                *R,   /* I */
  enum MStatusCodeEnum *SC)  /* O (optional) */

  {
  if (R == NULL)
    {
    return(FALSE);
    }

  if ((R->U.Moab.CData == NULL) ||
      (R->U.Moab.CDataIsProcessed == TRUE) ||
      (R->U.Moab.WData == NULL) ||
      (R->U.Moab.WDataIsProcessed == TRUE))
    {
    return(FALSE);
    }

  return(TRUE);
  }  /* END MMDataIsLoaded() */





/**
 * Query resource, node, and partition information via Moab resource manager.
 *
 * @see __MMClusterQueryPostLoad() - routine to process the reported data
 * @see __MMDoCommand() - routine which sends request to peer
 *
 * NOTE:  If peer responds immediately, directly call __MMClusterQueryPostLoad()
 *        to process.
 * NOTE:  If peer blocks, register __MMClusterQueryGetData() to process results
 *        when available.
 *
 * NOTE:  This command will request all peer information (cluster and workload) 
 *        - use '__MMSPLITREQUESTS' to change.
 *
 * @param R (I)
 * @param RCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMClusterQuery(

  mrm_t                *R,       /* I */
  int                  *RCount,  /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  int rc; 

  const char *FName = "MMClusterQuery";

  MDB(1,fNAT) MLog("%s(%s,RCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (RCount != NULL)
    *RCount = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (R->State != mrmsActive)  
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is %s, skipping cluster query\n",
      R->Name,
      MRMState[R->State]);

    if (SC != NULL)
      *SC = mscBadState;

    if (EMsg != NULL)
      strcpy(EMsg,"rm is down");

    return(FAILURE);
    }

  /* see if we should get data */

  if (MMDataIsLoaded(R,NULL) == FALSE)
    {
    if (MMGetData(R,FALSE,MSched.Iteration,NULL,EMsg,SC) == FAILURE)
      {
      return(FAILURE);
      }
    }

  /* try to process results now (if any) */

  rc = __MMClusterQueryPostLoad(R,NULL,EMsg,SC);

  if (RCount != NULL)
    *RCount = R->NC;

#ifdef __MMSPLITREQUEST

  MSUFree(R->P[R->ActingMaster].S);

#else /* __MMSPLITREQUEST */

  /* socket will be freed at completion of MMWorkloadQuery() */

  /* NO-OP */

#endif /* __MMSPLITREQUEST */

  return(rc);
  }  /* END MMClusterQuery() */





/**
 * Issue reservation control request to peer Moab.
 *
 * @param R (I)
 * @param RM (I)
 * @param Action (I)
 * @param OBuf (I/O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMRsvCtl(

  mrsv_t               *R,      /* I */
  mrm_t                *RM,     /* I */
  enum MRsvCtlCmdEnum   Action, /* I */
  void                **OBuf,   /* I/O (optional) */
  char                 *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)     /* O (optional) */

  {
  const char *FName = "MMRsvCtl";

  mxml_t *DE;
  mxml_t *RE;

  char   *BPtr;
  int     BSpace;

  int     TC;

  enum MStatusCodeEnum tmpSC;

  char CmdLine[MMAX_LINE << 1];
  char Response[MMAX_LINE];
  char tmpLine[MMAX_LINE + 1];

  char tmpName[MMAX_LINE + 1];
  char tmpBuffer[MMAX_BUFFER];

  int index;

  if ((R == NULL) || (RM == NULL))
    {
    return(FAILURE);
    }

  tmpLine[0] = '\0';
  Response[0] = '\0';

  /* build command */

  RE = NULL;

  if (MXMLCreateE(&RE,(char*)MSON[msonRequest]) == FAILURE)
    {
    MDB(1,fNAT) MLog("ALERT:    cannot create request XML in %s\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error - no memory");

    if (SC != NULL)
      *SC = mscNoMemory;

    return(FAILURE);
    }

  MS3SetObject(RE,(char *)MXO[mxoRsv],NULL);

  switch (Action)
    {
    case mrcmCreate:

      /* NOTE: do not support trigger, variables, profile, group mode */

      if (R->Name[0] == '\0')
        {
        if (EMsg != NULL)
          strcpy(EMsg,"cannot export rsv - no name specified for SystemRID");

        return(FAILURE);
        }

      MXMLSetAttr(RE,MSAN[msanAction],(void *)MRsvCtlCmds[mrcmCreate],mdfString);

      TC = 0;

      if (!MNLIsEmpty(&R->NL))
        {
        mnode_t *N;

        mnl_t *NL = &R->NL;

        MUSNInit(&BPtr,&BSpace,tmpBuffer,sizeof(tmpBuffer));

        for (index = 0;MNLGetNodeAtIndex(NL,index,&N) == SUCCESS;index++)
          {
          if ((N->Name == NULL) || (N->Name[0] == '\0'))
            break;

          if ((N->RM == NULL) || (N->RM != RM))
            continue;

          MOMap(RM->OMap,mxoNode,N->Name,FALSE,FALSE,tmpName);

          MUSNPrintF(&BPtr,&BSpace,"%s,",tmpName);

          TC += MNLGetTCAtIndex(NL,index);
          }
        
        MS3AddSet(RE,(char *)MRsvAttr[mraHostExp],tmpBuffer,NULL);
        }

      if (!MACLIsEmpty(R->ACL))
        {
        char ACLLine[MMAX_LINE];

        mbitmap_t BM;

        macl_t *tACL;

        /* extract ACL */

        MUSNInit(&BPtr,&BSpace,tmpBuffer,sizeof(tmpBuffer));

        bmset(&BM,mfmVerbose);
        bmset(&BM,mfmAVP);

        for (tACL = R->ACL;tACL != NULL;tACL = tACL->Next)
          {
          if (tmpBuffer[0] != '\0')
            { 
            MUSNPrintF(&BPtr,&BSpace,";%s",MACLShow(tACL,&BM,TRUE,RM->OMap,FALSE,ACLLine));
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s",MACLShow(tACL,&BM,TRUE,RM->OMap,FALSE,ACLLine));
            }
          }  /* END for (aindex) */
  
        if (tmpBuffer[0] != '\0')
          {
          MS3AddSet(RE,(char *)MRsvAttr[mraACL],tmpBuffer,NULL);
          }
        } 

      /* extract time constraints */

      if (R->StartTime != 0)
        {
        /* must adjust for time offsets */

        sprintf(tmpLine,"%ld",
          MAX(R->StartTime - RM->ClockSkewOffset,0));

        MS3AddSet(RE,(char *)MRsvAttr[mraStartTime],tmpLine,NULL);
        }

      if (R->EndTime != 0)
        {
        /* must adjust for time offsets */

        sprintf(tmpLine,"%ld",
          MAX(R->EndTime - RM->ClockSkewOffset,0));

        MS3AddSet(RE,(char *)MRsvAttr[mraEndTime],tmpLine,NULL);
        }

      MS3AddSet(RE,(char *)MRsvAttr[mraSpecName],R->Name,NULL);

      if (R->Comment != NULL)
        {
        MS3AddSet(RE,(char *)MRsvAttr[mraComment],R->Comment,NULL);
        }

      if (!bmisclear(&R->ReqFBM))
        {
        char tmpLine[MMAX_LINE];

        MUNodeFeaturesToString(',',&R->ReqFBM,tmpLine); 

        MS3AddSet(RE,(char *)MRsvAttr[mraReqFeatureList],tmpLine,NULL);
        }

      if (!bmisclear(&R->Flags))
        {
        mstring_t tmp(MMAX_LINE);

        bmtostring(&R->Flags,MRsvFlags,&tmp);

        MS3AddSet(RE,(char *)MRsvAttr[mraFlags],tmp.c_str(),NULL);
        }

      if (R->OType != mxoNONE)
        {
        mstring_t tmp(MMAX_LINE);

        MRsvAToMString(R,mraOwner,&tmp,0);

        MS3AddSet(RE,(char *)MRsvAttr[mraOwner],tmp.c_str(),NULL);
        }

      MS3AddSet(RE,(char *)MRsvAttr[mraSID],MSched.Name,NULL);
      MS3AddSet(RE,(char *)MRsvAttr[mraGlobalID],R->Name,NULL);

      /*
      if (R->Partition != 0)
        {
        MS3AddSet(RE,(char *)MRsvAttr[mraPartition],OptArg,NULL);
        }
      */
  
      MCResToString(&R->DRes,0,mfmAVP,tmpLine);      
  
      if (strcmp(tmpLine,NONE))
        {
        MS3AddSet(RE,(char *)MRsvAttr[mraResources],tmpLine,NULL);
        }

      if (TC != 0)
        {
        sprintf(tmpLine,"%d",
          TC);

        MS3AddSet(RE,(char *)MRsvAttr[mraReqTaskCount],tmpLine,NULL);
        }

      MS3AddSet(RE,(char *)MRsvAttr[mraReqTPN],"1",NULL);

      break;

    case mrcmDestroy:

      {
      mrrsv_t *RR;

      /* <Request action="destroy" actor="wightman"><Object>rsv</Object><Where name="Name">system.1.1</Where></Request> */

      MXMLSetAttr(RE,MSAN[msanAction],(void *)MRsvCtlCmds[mrcmDestroy],mdfString);

      RR = R->RemoteRsv;

      while (RR != NULL)
        {
        if (RR->RM != RM)
          {
          RR = RR->Next;

          continue;
          }

        MS3AddWhere(RE,(char *)MRsvAttr[mraName],RR->Name,NULL);

        break;
        }      
      }

      break;

    default:

      /* NO-OP */

      break;
    }

  if (MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE) == FAILURE)
    {
    MDB(1,fNAT) MLog("ALERT:    job cmdfile too large in %s\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error - job cmdfile too large");

    if (SC != NULL)
      *SC = mscNoMemory;

    MXMLDestroyE(&RE);

    return(FAILURE);
    }

  MXMLDestroyE(&RE);

  RM->P[RM->ActingMaster].IsNonBlocking = FALSE;

  if (__MMDoCommand(
      RM,
      &RM->P[RM->ActingMaster],      /* I/O */
      mcsMRsvCtl,
      CmdLine,
      Response,
      sizeof(Response),
      NULL,
      &tmpSC) == FAILURE)
    {
    if (SC != NULL)
      *SC = tmpSC;

    MSUFree(RM->P[RM->ActingMaster].S);

    return(FAILURE);
    }

  if ((RM->P[RM->ActingMaster].S == NULL) || (RM->P[RM->ActingMaster].S->RDE == NULL))
    {
    MSUFree(RM->P[RM->ActingMaster].S);

    return(FAILURE);
    }

  /* check for non-blocking response */

  DE = (mxml_t *)RM->P[RM->ActingMaster].S->RDE;

  RM->P[RM->ActingMaster].S->RDE = NULL;

  if ((DE->Val != NULL) &&
      !strcmp(DE->Val,"PENDING"))
    {
    /* non-blocking response - will get result later */
    }

  if (strstr(Response,"ERROR"))
    {
    /* error occured */

    MDB(1,fNAT) MLog("ERROR:    cannot reserve resources: '%s'\n",
      Response);

    MSUFree(RM->P[RM->ActingMaster].S);

    return(FAILURE);
    }

  switch (Action)
    {
    case mrcmCreate:

      /* grab remote rsv id */

      /* NOTE:  tmpVal -> 'NOTE:  reservation system.1 created' */

      tmpName[0] = '\0';

      sscanf(Response,"%1024s %1024s %64s %1024s",
        tmpLine,
        tmpLine,
        tmpName,
        tmpLine);

      if ((tmpName[0] != '\0') && (OBuf != NULL))
        {
        MUStrCpy((char *)OBuf,tmpName,MMAX_NAME);
        }

      break;

    default:

      /* NO-OP */

      break;
    }

  MSUFree(RM->P[RM->ActingMaster].S);

  return(SUCCESS);
  }  /* END MMRsvCtl() */





/* NOTE:  make M*NodeModify support async flag for group operations */

/**
 * Modify node/resource within grid
 *
 * @see MRMNodeModify() - parent
 *
 * @param SN (I) [optional,modified]
 * @param SV (I) [optional,modified]
 * @param Ext (I) [optional,nodelist string]
 * @param R (I)
 * @param AIndex (I)
 * @param AttrValue (I)
 * @param Mode (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMNodeModify(

  mnode_t              *SN,        /* I (optional,modified) */
  mvm_t                *SV,        /* I (optional,modified) */
  char                 *Ext,       /* I (optional,nodelist string) */
  mrm_t                *R,         /* I */
  enum MNodeAttrEnum    AIndex,    /* I */
  void                 *AttrValue, /* I */
  enum MObjectSetModeEnum Mode,    /* I */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  char Response[MMAX_LINE];

  int  rc;

  if (((SN == NULL) && (Ext == NULL)) || (AIndex <= mnaNONE))
    {
    return(FAILURE);
    }

  /* NYI */

  rc = FAILURE;

  if (rc == FAILURE)
    {
    /* cannot change node attribute */

    MDB(1,fNAT) MLog("ALERT:    cannot modify attribute %s on node %s to value %s (%s)\n",
      MNodeAttr[AIndex],
      (Ext != NULL) ? Ext : SN->Name,
      (char *)AttrValue,
      Response);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MMNodeModify() */





/**
 * Perform general command via Moab interface.
 *
 * @see MCDoCommand() - child
 *
 * NOTE: This function must remain thread safe!
 * @param R (I) [optional]
 * @param P (I)
 * @param SvcIndex (I)
 * @param Args (I) [optional]
 * @param Response (O) [optional,minsize=BufSize]
 * @param BufSize (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int __MMDoCommand(

  mrm_t                *R,         /* I (optional) */
  mpsi_t               *P,         /* I */
  enum MSvcEnum         SvcIndex,  /* I */
  const char           *Args,      /* I (optional) */
  char                 *Response,  /* O (optional,minsize=BufSize) */
  int                   BufSize,   /* I */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  int        rc;

  mccfg_t    tC;

  char      *CmdResponse;  /* pointer to request response */

  enum MSFC  StatusCode;

  const char *FName = "__MMDoCommand";

  MDB(1,fNAT) MLog("%s(%s,P,%s,%s,Response,%d,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : NULL,
    MUI[SvcIndex].SName,
    (Args != NULL) ? Args : "NULL",
    BufSize);

  if (Response != NULL)
    Response[0] = '\0';

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((P == NULL) || (SvcIndex == mcsNONE))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error - invalid parameters in command request");

    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  /* call Moab peer */

  /* This should be the catch-all for disabled RM's (just in case something gets by) */

  if (R != NULL &&
      R->State == mrmsDisabled)
    {
    if (EMsg != NULL)
      sprintf(EMsg,"RM '%s' disabled",
        R->Name);

    return(FAILURE);
    }
  
  memset(&tC,0,sizeof(tC));

  MUStrCpy(tC.ServerHost,P->HostName,sizeof(tC.ServerHost));
  MUStrCpy(tC.ServerCSKey,P->CSKey,sizeof(tC.ServerCSKey));
  tC.ServerCSAlgo  = P->CSAlgo;
  tC.ServerPort    = P->Port;

  tC.Timeout = P->Timeout;

  if (R != NULL)
    sprintf(tC.Label,"RM:%s",
      R->Name);

  /* if Timeout is less than 1000 we assume it is in seconds and we need to convert to us */
  /* FIXME: in future P->Timeout should already be in us! */

  if (tC.Timeout < 1000)
    tC.Timeout = MAX(tC.Timeout * MDEF_USPERSECOND,3 * MDEF_USPERSECOND);

  tC.IsNonBlocking = P->IsNonBlocking;
  tC.Peer = P;

  /* reset so other peer services are not affected! */

  P->IsNonBlocking = FALSE;
  
  if (P->S == NULL)
    {
    P->S = (msocket_t *)MUCalloc(1,sizeof(msocket_t));
    }

  if (P->RID != NULL)
    {
    MUStrCpy(tC.RID,P->RID,sizeof(tC.RID));
    }
              
  MDB(2,fCONFIG) MLog("INFO:     sending peer server command to %s:%d (Cmd: %s, Requestor: %s, Key: %c...)\n",
    tC.ServerHost,
    tC.ServerPort,
    MUI[SvcIndex].SName,
    tC.RID,
    tC.ServerCSKey[0]);

  /* NOTE:  Doug, Dave, and Josh spoke about the native-based grid code and
   * decided that we can use SSH tunneling w/ helper setup scripts to accomplish
   * a cleaner, cheaper, and overall better solution than trying to reinvent
   * the wheel using native-based grids. Essentially, native-based grids were supposed
   * to allow new users to easily setup a grid over one secure SSH port regardless
   * of the number of clusters involved--it was to essentially multiplex multiple secure
   * Moab cluster connections. We are abandoning this route and should go with SSH tunneling.
   * This code should be removed soon. (Written 2/28/08). */

  /* NOTE:  to support native communication, add changes here */

  if ((R != NULL) && bmisset(&R->IFlags,mrmifNativeBasedGrid))
    {
    char CmdLine[MMAX_LINE];

    enum MRMFuncEnum CmdIndex;

    /* create mschedctl command */

    rc = FAILURE;

    StatusCode = msfENone;

    CmdResponse = NULL;

    snprintf(CmdLine,sizeof(CmdLine),"mschedctl %s",
      "data");

    /* convert SvcIndex to RMCmdIndex */

    switch (SvcIndex)
      {
      case mcsMSchedCtl:
      case mcsMJobCtl:
      case mcsMRsvCtl:
      default:

        CmdIndex = mrmWorkloadQuery;

        break;
      }  /* END switch (SvcIndex) */

    mstring_t RespBuf(MMAX_LINE);

    rc = MNatDoCommand(
       &R->ND,
       NULL,
       CmdIndex,
       R->ND.Protocol[CmdIndex],
       TRUE,
       NULL,
       CmdLine,         /* I */
       &RespBuf,        /* O */
       EMsg,
       NULL);
    }  /* END if ((R != NULL) && bmisset(&R->IFlags,mrmifNativeBasedGrid)) */
  else
    {               
    rc = MCDoCommand(
      SvcIndex,
      &tC,
      Args,
      NULL,  
      EMsg,      /* O */
      P->S);     /* O */

    StatusCode = (enum MSFC)P->S->StatusCode;

    CmdResponse = P->S->RPtr;
    }
    
  if (rc == FAILURE)
    {
    MDB(3,fNAT) MLog("INFO:     RM[%s] command '%s' failed - %s\n",
      (R != NULL) ? R->Name : "???",
      MUI[SvcIndex].SName,
      (EMsg != NULL) ? EMsg : "???");

    if (SC != NULL)
      {
      if (StatusCode == msfConnRejected)
        *SC = mscNoEnt;
      else if (StatusCode == msfEBadRequestor)
        *SC = mscNoAuth;
      else
        *SC = mscRemoteFailure;
      }

    if (StatusCode == msfConnRejected)
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"cannot connect to peer at %s:%d - check configuration/firewalls",
          P->S->RemoteHost,
          P->S->RemotePort);

      MRMSetState(R,mrmsDown);
      }    /* END if (StatusCode == msfConnRejected) */

    if ((CmdResponse != NULL) && 
        (CmdResponse[0] != '\0') &&
        (EMsg != NULL))
      {
      /* if server side info available, overwrite any possible local messages */

      snprintf(EMsg,MMAX_LINE,"message from %s: %s",
        (R != NULL) ? R->Name : tC.ServerHost,
        CmdResponse);
      }

    return(FAILURE);
    }
         
  if (Response != NULL)
    {
    if (CmdResponse != NULL)
      MUStrCpy(Response,CmdResponse,BufSize); 
    }

  return(SUCCESS);
  }  /* END __MMDoCommand() */





/**
 * Load workload information from peer Moab.
 *
 * NOTE:  This routine now only calls  __MMWorkloadQueryPostLoad() to handle
 *        all processing of reported peer workload info. MMGetData() now
 *        creates the request.
 *
 * @see __MMWorkloadQueryPostLoad() - child
 * @see MUIPeerJobQuery() - peer - generate data w/in peer server
 *
 * @param R (I)
 * @param WCount (O) [optional]
 * @param NWCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMWorkloadQuery(

  mrm_t                *R,       /* I */
  int                  *WCount,  /* O (optional) */
  int                  *NWCount, /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  int rc;

  const char *FName = "MMWorkloadQuery";

  MDB(1,fNAT) MLog("%s(%s,WCount,NWCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (WCount != NULL)
    *WCount = 0;

  if (NWCount != NULL)
    *NWCount = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (R->State != mrmsActive)
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, skipping workload query\n",
      R->Name);

    if (EMsg != NULL)
      {
      sprintf(EMsg,"bad state - %s",
        MRMState[R->State]);
      }

    return(FAILURE);
    }

#ifdef __MMSPLITREQUEST

  /* see if we should get data */

  if (MMDataIsLoaded(R,NULL) == FALSE)
    {
    if (MMGetData(R,FALSE,MSched.Iteration,NULL,EMsg,SC) == FAILURE)
      {
      return(FAILURE);
      }
    }
#endif /* __MMSPLITREQUEST */

  /* process results now (if any) */

  rc = __MMWorkloadQueryPostLoad(R,WCount,NWCount,EMsg,SC);

  MSUFree(R->P[R->ActingMaster].S);

  return(rc);
  }  /* END MMWorkloadQuery() */





/**
 * Load attributes for new job reported via Moab (grid peer) interface.
 *
 * @see MMJobUpdate() - peer
 * @see __MMWorkloadQueryPostLoad() - parent
 *
 * @param JobName (I)
 * @param JE (I)
 * @param J (I) [modified]
 * @param TaskList (O) [minsize=MSched.JobMaxTaskCount]
 * @param R (I)
 */

int MMJobLoad(

  char   *JobName,  /* I */
  mxml_t *JE,       /* I */
  mjob_t *J,        /* I (modified) */
  int    *TaskList, /* O (minsize=MSched.JobMaxTaskCount) */
  mrm_t  *R)        /* I */

  {
  mqos_t *QDef;

  char tmpName[MMAX_NAME];

  const char *FName = "MMJobLoad";

  char *AccountName = NULL;

  int rc = FAILURE;

  MDB(2,fRM) MLog("%s(%s,JE,J,TaskList,%s)\n",
    FName,
    JobName,
    (R != NULL) ? R->Name : "NULL");

  if (TaskList != NULL)
    TaskList[0] = -1;

  if ((J == NULL) || (JE == NULL))
    {
    return(FAILURE);
    }

  /* map job attributes */

  if ((R != NULL) && (R->OMap != NULL))
    {
    if (MMJobOMap(JE,R) == FAILURE)
      {
      return(FAILURE);
      }
    }

  /* determine job's source */

  if ((MXMLGetAttr(JE,(char *)MJobAttr[mjaRM],NULL,tmpName,sizeof(tmpName)) == SUCCESS) &&
      !strcmp(MSched.Name,tmpName))
    {
    /* job originated from this peer and is reporting back */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void *)"internal",mdfString);
    }
  else
    {
    /* job originates from the reporting peer */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void *)R->Name,mdfString);
    }

  /* Mark that the rm owns the hold just during the call to MJobFromXML
   * even if there is no hold. If the rm owns the hold then moab won't
   * send the hold back down to the cluster in MJobSetAttr(mjahold. */

  bmset(&J->IFlags,mjifRMOwnsHold);

  /* load job from xml */

  rc = MJobFromXML(J,JE,FALSE,mSet);
    
  bmunset(&J->IFlags,mjifRMOwnsHold);
  
  if (rc == FAILURE)
    {
    MDB(1,fNAT) MLog("INFO:     cannot load job in %s",
      FName);

    return(FAILURE);
    }

  MJobAdjustETimes(J,R);

#ifdef MNOT
  /* is the following code still valid? */

  if ((J->DestinationRM == NULL) &&
      (J->SubmitRM == NULL))
    {
    /* 'lost' job discovered (was initially submitted locally) */

    MRMAdd("internal",&J->SubmitRM);
    J->DestinationRM = R;
    }
  else
    {
    MJobSetAttr(J,mjaRM,(void **)R->Name,mdfString,mSet);
    }

  if ((J->DestinationRM == NULL) && (J->DRMJID != NULL))
    {
    /* assume job was submitted locally and staged to reporting peer */

    MJobSetAttr(J,mjaDRM,(void **)R,mdfOther,mSet);
    }
#endif /* MNOT */

  if (bmisset(&MRM[J->Req[0]->RMIndex].IFlags,mrmifLocalQueue) &&
      (J->DestinationRM != NULL))
    {
    /* update the J->Req's to appropriately reflect where the job was staged (to DRM) */
 
    int rqindex;

    mreq_t *RQ;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      if (bmisset(&MRM[RQ->RMIndex].IFlags,mrmifLocalQueue))
        RQ->RMIndex = J->DestinationRM->Index;
      }  /* END for (rqindex) */
    }

  /* NOTE:  populate TaskList from J->TaskMap (NYI) */

  if (TaskList != NULL)
    {
    if (J->TaskMap != NULL)
      {
      memcpy(
        TaskList,
        J->TaskMap,
        sizeof(int) * (MIN(J->TaskMapSize,MSched.JobMaxTaskCount)));
      }
    else
      {
      TaskList[0] = -1;
      }
    }

  if ((R != NULL) && 
      (bmisset(&R->Flags,mrmfClient)) && 
      (R->OMap != NULL))
    {
    char tmpUName[MMAX_LINE];
    char tmpGName[MMAX_LINE];
    char tmpDName[MMAX_PATH];

    MOMap(R->OMap,mxoUser,J->Credential.U->Name,TRUE,FALSE,tmpUName);
    MOMap(R->OMap,mxoGroup,J->Credential.G->Name,TRUE,FALSE,tmpGName);
    MOMap(R->OMap,mxoCluster,J->Env.IWD,TRUE,FALSE,tmpDName);

    MJobSetAttr(J,mjaUser,(void **)tmpUName,mdfString,mSet);
    MJobSetAttr(J,mjaGroup,(void **)tmpGName,mdfString,mSet);
    MJobSetAttr(J,mjaIWD,(void **)tmpDName,mdfString,mSet);
    }

  if ((MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&QDef) == FAILURE) ||
      (J->QOSRequested == NULL))
    {
    MJobSetQOS(J,QDef,0);
    }
  else
    {
    MJobSetQOS(J,J->QOSRequested,0);
    }

  if (J->Request.TC == 0)
    {
    MDB(2,fNAT) MLog("ALERT:    no job task info located for job '%s' (assigning taskcount to 1)\n",
      J->Name);

    J->Request.TC        = 1;
    J->Req[0]->TaskCount = 1;
    }

  if (R != NULL)
    bmset(&J->SpecPAL,R->PtIndex);

  /* setup the account credential */

  if (J->Credential.A != NULL)
    {
    AccountName = J->Credential.A->Name;
    }

  if (MJobSetCreds(J,J->Credential.U->Name,J->Credential.G->Name,AccountName,0,NULL) == FAILURE)
    {
    MDB(1,fNAT) MLog("INFO:     cannot authenticate job '%s' (U: %s  G: %s A: %s)\n",
      J->Name,
      J->Credential.U->Name,
      J->Credential.G->Name,
      (AccountName == NULL) ? "" : AccountName);

    return(FAILURE);
    }    /* END if (MJobSetCreds() == FAILURE) */

  return(SUCCESS);
  }  /* END MMJobLoad() */





/**
 * Load attributes for existing job reported via Moab (grid peer) interface.
 *
 * @see MMJobLoad() - peer
 * @see __MMWorkloadQueryPostLoad() - parent
 *
 * @param JE (I)
 * @param J (I) [modified]
 * @param TaskList (O) [optional,minsize=MSched.JobMaxTaskCount]
 * @param R (I)
 */

int MMJobUpdate(

  mxml_t *JE,       /* I */
  mjob_t *J,        /* I (modified) */
  int    *TaskList, /* O (optional,minsize=MSched.JobMaxTaskCount) */
  mrm_t  *R)        /* I */

  {
  mds_t *DS = NULL;

  enum MObjectSetModeEnum SetMode;

  char tmpName[MMAX_NAME];
  mbitmap_t tmpSpecFlags;

  long idleQueueTime = -1; /* Save time job was in the idle queue */

  int savedTC;
  int savedNC;

  int rc = FAILURE;

  const char *FName = "MMJobUpdate";

  MDB(2,fNAT) MLog("%s(JE,%s,TaskList,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (TaskList != NULL)
    TaskList[0] = -1;

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  must save values that could be overwritten in MJobFromXML() */

  /* Save flags that we need to preserve after we load the flags from XML. 
   * Currently the only flag that we know we need to preserve is mjfIgnPolicies.
   * We may discover additional flags. */

  if (bmisset(&J->SpecFlags,mjfIgnPolicies))
    {
    bmset(&tmpSpecFlags,mjfIgnPolicies);
    }

  if (bmisset(&J->SpecFlags,mjfAdminSetIgnPolicies))
    {
    bmset(&tmpSpecFlags,mjfAdminSetIgnPolicies);
    }

  /* Save TC and NC to compare later (MJobFromXML might change these values)
   * Inside MJobFromXML, the J->Request.NC is zeroed out and a new value is 
   * re-calculated based on the true values obtained from the Req, but during 
   * the time the value is zero the job may be transitioned because of other 
   * updates leaving the cache with the wrong value.   If the newly calculated 
   * value for J->Request.NC (or TC) is different, then it needs to get 
   * transitioned to update the cache correctly. MOAB-3149 */

  savedTC = J->Request.TC;
  savedNC = J->Request.NC;

  /* Copy off staging ptrs so that if job was migrated to peer with staging
   * request the peer can be the authority on the state of the stagin. */

  if (J->DataStage != NULL)
    {
    MMovePtr((char **)&J->DataStage,(char **)&DS);
    }

  idleQueueTime = J->EffQueueDuration;

  /* map job attributes */

  if (R->OMap != NULL)
    {
    if (MMJobOMap(JE,R) == FAILURE)
      {
      return(FAILURE);
      }
    }

  /* determine job's source */

  if ((MXMLGetAttr(JE,(char *)MJobAttr[mjaRM],NULL,tmpName,sizeof(tmpName)) == SUCCESS) &&
      !strcmp(MSched.Name,tmpName))
    {
    /* job originated from this peer and is reporting back */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void *)"internal",mdfString);
    }
  else
    {
    /* job originates from the reporting peer */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void *)R->Name,mdfString);
    }

  /* Mark that the rm owns the hold just during the call to MJobFromXML
   * even if there is no hold. If the rm owns the hold then moab won't
   * send the hold back down to the cluster in MJobSetAttr(mjahold. */

  bmset(&J->IFlags,mjifRMOwnsHold);

  /* Release the hold on the master if the master has a hold and the
   * cluster doesn't, ie. hold released on the cluster. Assuming the cluster 
   * is the authority on the hold since if a hold is set on the grid head 
   * the hold will be sent to the cluster. */

  if ((!bmisclear(&J->Hold)) &&
      (MXMLGetAttr(JE,(char *)MJobAttr[mjaHold],NULL,NULL,-1) == FAILURE))
    {
    MJobSetAttr(J,mjaHold,(void **)&J->Hold,mdfOther,mUnset);

    MMBRemoveMessage(&J->MessageBuffer,NULL,mmbtHold); 
    }

  /* load job from XML */

  /* NOTE: because of mSetOnEmpty mode only some job attrs will be updated from XML */

  /* NOTE:  we want the job to be updated in case it was modified on the remote
            peer we need to know about it... */

  if (bmisset(&R->Flags,mrmfPushSlaveJobUpdates))
    {
    SetMode = mSet;
    }
  else
    {
    SetMode = mSetOnEmpty;
    }

  rc = MJobFromXML(J,JE,FALSE,SetMode);
    
  bmunset(&J->IFlags,mjifRMOwnsHold);
    
  if (rc == FAILURE)
    {
    MDB(1,fNAT) MLog("INFO:     cannot update job");
                                                                                
    return(FAILURE);
    }

  MJobAdjustETimes(J,R);

  /* copy back settings */

  /* Ignore the idle queue time from a slave Moab */
  /* This is because the slave can't tell between an idle and a blocked job. */

  MJobSetAttr(J,mjaEEWDuration,(void **)&idleQueueTime,mdfLong,mSet);

  if ((bmisclear(&J->Hold)) && !MJOBISACTIVE(J))
    {
    MJobAddEffQueueDuration(J,NULL);
    }

  /* If the job has new staging ptrs that means that the job was migrated to a
   * peer that would handle the staging, otherwise the staging is being handled
   * by this peer */

  if (J->DataStage == NULL)
    MMovePtr((char **)&DS,(char **)&J->DataStage);
  else
    MDSFree(&DS);

#ifdef MNOT
  /* this could be dangerous ... DRMJID may be set to a local RM
   * if this job is exported due to "LocalWorkloadExport" */
  if (J->DRMJID != NULL)
    {
    /* job already has destination on reporting peer */

    MJobSetAttr(J,mjaDRM,(void **)R,mdfOther,mSet);
    }
#endif /* MNOT */

  /* set migrate sub-state state if Idle */

  if ((J->State == mjsIdle) &&
      bmisset(&R->Flags,mrmfAutoStart) &&
      !bmisset(&J->Flags,mjfClusterLock))
    {
    J->SubState = mjsstMigrated;
    }
  else
    {
    J->SubState = mjsstNONE;
    }

  /* NOTE:  populate TaskList from J->TaskMap (NYI) */

  if (TaskList != NULL)
    {
    if (J->TaskMap != NULL)
      {
      memcpy(TaskList,
             J->TaskMap,
             sizeof(int) * (MIN(J->TaskMapSize,MSched.JobMaxTaskCount)));
      }
    else
      {
      TaskList[0] = -1;
      }
    }

  if ((bmisset(&R->Flags,mrmfClient) && (R->OMap != NULL)))
    {
    char tmpUName[MMAX_LINE];
    char tmpGName[MMAX_LINE];
    char tmpDName[MMAX_PATH];

    MOMap(R->OMap,mxoUser,J->Credential.U->Name,TRUE,FALSE,tmpUName);
    MOMap(R->OMap,mxoGroup,J->Credential.G->Name,TRUE,FALSE,tmpGName);
    MOMap(R->OMap,mxoCluster,J->Env.IWD,TRUE,FALSE,tmpDName);

    MJobSetAttr(J,mjaUser,(void **)tmpUName,mdfString,mSet);
    MJobSetAttr(J,mjaGroup,(void **)tmpGName,mdfString,mSet);
    MJobSetAttr(J,mjaIWD,(void **)tmpDName,mdfString,mSet);
    }

  if (!bmisclear(&tmpSpecFlags))
    {
    /* OR in the flags that we saved before we called MJobFromXML() */

    MJobSetAttr(J,mjaFlags,(void **)&tmpSpecFlags,mdfLong,mIncr);
    }

  if((savedTC != J->Request.TC) ||
     (savedNC != J->Request.NC))
    {
    /* iff these values changed then update the cache */

    MJobTransition(J,FALSE,FALSE);
    }

  return(SUCCESS);
  } /* END MMJobUpdate */




/**
 * Handle the migration or submission of a job to a remote Moab peer.
 *
 * When adding a new mjaXXX attribute, complete the foillowing coding steps
 * to allow the attribute to be included in the job migration.
 *
 * Moab.h - add mjaXXX to MJobAttrEnum
 * MConst.c - add test description for new attribute in MJobAttr[]
 * MS3I.c - In MS3JobToXML() add mjaXXX to DJAList[] and also add a case entry.
 *          If the new attribute falls within the SSS spec (see Scott J) then
 *          add it to the MS3JobTable[]
 *          In __MS3JobSetAttr() add it as a case statement with other attributes 
 *          in the MS3JobTable[] or if it is not in the table then add it
 *          as an exception later in the routine (see mjaUMask as an example).
 * MJob.c  - Optionally add to MJobDuplicate()
 *           MJobSetAttr() add case
 *           MJobAToMString() add case
 *           Add to MJobStoreCP() and MCJobStoreCP() if it should be checkpointed.
 * (Note that the procedure is somewhat different for a new mrqaXXX attribute)
 *
 * @see MRMJobSubmit() - parent
 * @see MS3JobToXML() - child (creates job XML to pass to peer)
 *
 * @param JobSpec (I)
 * @param R (I)
 * @param JP (I/O) [optional]
 * @param FlagBM (I) [bitmap of enum MJobSubmitFlags]
 * @param JobID (O) [optional,minsize=MMAX_NAME]
 * @param EMsg (O) [optional,minsize=MMAX_LINE,minsize==MMAX_BUFFER if mjfTest]
 * @param SC (O) [optional]
 */

int MMJobSubmit(

  const char           *JobSpec,  /* I */
  mrm_t                *R,        /* I */
  mjob_t              **JP,       /* I/O (optional) */
  mbitmap_t            *FlagBM,   /* I (bitmap of enum MJobSubmitFlags) */
  char                 *JobID,    /* O (optional,minsize=MMAX_NAME) */
  char                 *EMsg,     /* O (optional,minsize=MMAX_LINE,minsize==MMAX_BUFFER if mjfTest) */
  enum MStatusCodeEnum *SC)       /* O (optional) */

  {
  mxml_t *RE;
  mxml_t *JE;

  char *ptr;
  char *TokPtr;

  char Response[MMAX_LINE];

  char *tmpRMSubmitString;
  char *OrigRMSubmitString = NULL;  /* set to avoid compiler warnings */

  long OrigSMinTime = 0;

  int rc = 0;

  mjob_t *J;

  const char *FName = "MMJobSubmit";

  MDB(1,fNAT) MLog("%s(%.64s,%s,J,%ld,JobID,EMsg,SC)\n",
    FName,
    (JobSpec != NULL) ? JobSpec : "NULL",
    (R != NULL) ? R->Name : "NULL",
    FlagBM);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((R == NULL) || (FlagBM == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error - invalid parameters");

    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  if ((JP == NULL) || (*JP == NULL))
    {
    /* should this always be true? */

    if (EMsg != NULL)
      strcpy(EMsg,"internal error - invalid job");

    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  J = *JP; 

  /* check for conditions that bypass normal P2P migration */

  if (bmisset(&J->Flags,mjfInteractive))
    {
    int rc;

    /* NOTE:  how do we determine the correct local interface to use? */

    /* will populate J->RMSubmitString */

    rc = MWikiJobSubmit(
        JobSpec,
        R,
        JP,
        FlagBM,
        JobID,
        EMsg,
        SC);

    return(rc);
    }

  if ((J->RMSubmitString == NULL) && (J->Env.Cmd == NULL))
    {      
    if (EMsg != NULL)
      strcpy(EMsg,"internal error - cannot locate a valid job script");

    if (SC != NULL)
      *SC = mscBadParam;
    
    return(FAILURE);
    }

  Response[0] = '\0';

  /* build command */

  RE = NULL;

  if (MXMLCreateE(&RE,(char*)MSON[msonRequest]) == FAILURE)
    {
    MDB(1,fNAT) MLog("ALERT:    cannot create request XML in %s\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error - no memory");

    if (SC != NULL)
      *SC = mscNoMemory;

    return(FAILURE);
    }

  MS3SetObject(RE,(char *)MXO[mxoJob],NULL);
  MXMLSetAttr(RE,MSAN[msanAction],(void *)MJobCtlCmds[mjcmSubmit],mdfString);

  if (bmisset(&J->IFlags,mjifDataDependency))
    {
    /* data-staging dependencies exist */

    bmset(FlagBM,mjsufDSDependency);
    }

  if (R->SyncJobID == TRUE)
    {
    bmset(FlagBM,mjsufSyncJobID);
    }

  JE = NULL;

  tmpRMSubmitString = NULL;

  if (bmisset(&J->Flags,mjfCoAlloc) && 
     (J->RMSubmitString != NULL) &&
     (bmisset(&R->Languages,J->RMSubmitType)))
    {
    int rqindex;
    mreq_t *RQ;

    char    tmpVal[MMAX_LINE];

    int     size;

    size = strlen(J->RMSubmitString) + strlen("#PBS -l nodes=XXXXXX") + 1;

    if ((tmpRMSubmitString = (char *)MUMalloc(size)) == NULL)
      {
      MXMLDestroyE(&RE);
  
      return(FAILURE);
      }

    MUStrCpy(tmpRMSubmitString,J->RMSubmitString,size);

    /* locate req associated with rm */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      if (RQ->RMIndex != R->Index)
        continue;

      /* may need to modify submit string */

      switch (J->RMSubmitType)
        {
        case mrmtPBS:

          /* insert nodecount */

          MJobPBSExtractArg(
            "-l",
            "nodes",
            tmpRMSubmitString,
            tmpVal,
            sizeof(tmpVal),
            TRUE);

          sprintf(tmpVal,"nodes=%d",
            RQ->TaskCount);

          MJobPBSInsertArg(
            "-l",
            tmpVal,
            tmpRMSubmitString,
            tmpRMSubmitString,
            size);

          break;

        default:

          /* NO-OP */

          break;
        }

      /* NOTE: assume only one req per rm is embedded in J->RMSubmitString */

      break;
      }    /* END for (rqindex) */
    }      /* END if (bmisset(&J->IFlags,mjifCoAlloc) && ...) */


  /* NOTE:  MS3JobToXML() should only pass info associated with RM req for coalloc jobs */

  if (tmpRMSubmitString != NULL)
    {
    OrigRMSubmitString = J->RMSubmitString;
    J->RMSubmitString = tmpRMSubmitString;
    }

  /* rt7548 - llnl moab slave code has problems if we send EligibleTime in the job submit.
     SMinTime should not apply to the slave if we are doing "just in time" scheduling
     so there should be no harm if we do not send it to the slave. */

  if ((MSched.JobMigratePolicy == mjmpJustInTime) && (J->SMinTime > MSched.Time))
    {
    OrigSMinTime = J->SMinTime;
    J->SMinTime = 0;
    }

  MS3JobToXML(J,&JE,R,TRUE,FlagBM,NULL,NULL,NULL,EMsg);

  if (tmpRMSubmitString != NULL)
    {
    J->RMSubmitString = OrigRMSubmitString;

    MUFree(&tmpRMSubmitString);
    }

  if (OrigSMinTime != 0)
    J->SMinTime = OrigSMinTime;

  MXMLAddE(RE,JE);

  mstring_t CmdLine(MMAX_SCRIPT);
    
  rc = MXMLToMString(RE,&CmdLine,NULL,TRUE);
    
  MXMLDestroyE(&RE);

  if (rc == FAILURE)
    {
    MDB(1,fNAT) MLog("ALERT:    job cmdfile too large in %s\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error - job cmdfile too large");

    if (SC != NULL)
      *SC = mscNoMemory;

    return(FAILURE);
    }

  R->P[R->ActingMaster].IsNonBlocking = TRUE;

  rc = __MMDoCommand(
      R,
      &R->P[R->ActingMaster],
      mcsMJobCtl,
      CmdLine.c_str(),
      Response,                /* O */
      sizeof(Response),
      EMsg,
      SC);

  if (rc == FAILURE)
    {
    /* cannot migrate job */

    MDB(1,fNAT) MLog("ALERT:    remote job migration failed in %s\n",
      FName);

    /* handle errors in post load */

    __MMJobSubmitPostLoad(R->P[R->ActingMaster].S,R,J,NULL,NULL);

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  if (strstr(Response,"ERROR"))
    {
    /* error occurred */
    
    MDB(1,fNAT) MLog("ERROR:    cannot migrate job: '%s'\n",
      Response);

    if (EMsg != NULL)
      {
      char *ptr;

      /* remove PENDING from end of error message */      

      ptr = strstr(Response,"PENDING");
      if (ptr != NULL)
        *ptr = '\0';
      
      MUStrCpy(EMsg,Response,MMAX_LINE);
      }

    /* handle errors */

    __MMJobSubmitPostLoad(R->P[R->ActingMaster].S,R,J,NULL,NULL);

    MSUFree(R->P[R->ActingMaster].S);

    if ((SC != NULL) && (*SC == mscNoError))
      *SC = mscRemoteFailure;

    return(FAILURE);
    }  /* END if (strstr(Response,"ERROR")) */

  /* check for non-blocking response */

  if (strstr(Response,"PENDING"))
    {
    msocket_t *tmpS;

    /* non-blocking response - will get confirmation later */

    MSUDup(&tmpS,R->P[R->ActingMaster].S);

    MUITransactionAdd(
      &MUITransactions,
      tmpS,
      R,
      strdup(J->Name),
      __MMJobSubmitPostLoadByJobName);

    /* prevent disconnect */

    R->P[R->ActingMaster].S->sd = -1;
    }

  /* FORMAT: <JobName>:<HumanDesc>[|<CmdFile>:<HumanDesc>] [(PENDING)] */

  /* create copy to preserve orig response */

  ptr = MUStrTok(Response,":|\n",&TokPtr);  /* <JobName> */

  /* obtain job id */

  if (JobID != NULL)
    MUStrCpy(JobID,ptr,MMAX_NAME);

  /* NOTE:  job will show up on next job query */

  ptr = MUStrTok(NULL,":|\n",&TokPtr);  /* <HumanDesc> */
  ptr = MUStrTok(NULL,":|\n",&TokPtr);  /* possibly <CmdFile> */

  MDB(1,fNAT) MLog("INFO:     job '%s' successfully migrated to remote peer\n",
    J->Name);

  if (bmisset(FlagBM,mjsufDataOnly) &&
      (ptr != NULL) &&
      strcmp(ptr,"PENDING"))
    {
    /* process staging file info */

    MUStrDup(&J->Env.Cmd,ptr);
    }    /* END if (MISET(FlagBM,mjsufDataOnly)) */

  MSUFree(R->P[R->ActingMaster].S);

  return(SUCCESS);
  }  /* END MMJobSubmit() */




/**
 * Issue remote job start request via Moab interface.
 *
 * @param J (I)
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMJobStart(

  mjob_t               *J,    /* I */
  mrm_t                *R,    /* I */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  char  Response[MMAX_LINE];
  char  NodeName[MMAX_LINE];

  static char *CmdLine = NULL;    /* FIXME: make static */
  static int   CmdLineSize = MMAX_BUFFER;
  static int   MaxCmdLineSize = MMAX_BUFFER << 2;


  mxml_t *RE = NULL;

  int     nindex;
  int     tindex;
   
  mnode_t *N;

  const char *FName = "MMJobStart";

  MDB(1,fNAT) MLog("%s(%s,%s,Msg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if (MNLIsEmpty(&J->NodeList))
    {
    /* invalid host list */

    return(FAILURE);
    }

  if (!bmisset(&J->Flags,mjfCoAlloc) && (J->DestinationRM != R))
    {
    /* job must be staged to remote cluster */

    return(FAILURE);  
    }

  if (bmisset(&R->Flags,mrmfAutoStart) &&
      ((J->DataStage == NULL) || (J->DataStage->SIData == NULL)))
    {
    /* remote server will start job automatically */

    return(SUCCESS);
    }
    
  /* build command */

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);
                                                                                
  MS3SetObject(RE,(char *)MXO[mxoJob],NULL);

  /* populate hostlist, FORMAT=<HOST>[,<HOST>]... */

  /* Only multi-req jobs with one req for procs and another with the
   * GLOBAL node are supported. The GLOBAL node will be handled in 
   * MUIJobCtl on the slave. The GLOBAL node will be ignored on the 
   * slave unless the slave is configured with a GLOBAL node. */

  /* when we populate hostlist taking into account task counts, change
   * mjcmStart processing in MUIJobCtl() see ticket 2595 */ 
 
  mstring_t HostList(MMAX_LINE);

  for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    for (tindex = 0;tindex < MNLGetTCAtIndex(&J->NodeList,nindex);tindex++)
      {
      MOMap(R->OMap,mxoNode,N->Name,FALSE,FALSE,NodeName);
      
      if (HostList.empty())
        {
        MStringAppend(&HostList,NodeName);
        }
      else
        {
        MStringAppendF(&HostList,",%s",
          NodeName);
        }
      }  /* END for (tindex) */
    }    /* END for (nindex) */

  MS3AddWhere(RE,(char *)MJobAttr[mjaReqHostList],HostList.c_str(),NULL);

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MJobCtlCmds[mjcmStart],
    mdfString);

  if (J->DRMJID != NULL)
    {
    MXMLSetAttr(
      RE,
      (char *)MXO[mxoJob],
      (void *)J->DRMJID,
      mdfString);
    }
  else
    {
    MXMLSetAttr(
      RE,
      (char *)MXO[mxoJob],
      (void *)J->Name,
      mdfString);
    }

  /* used for jobs with possible temp. pending IDs */

  MXMLSetAttr(
    RE,
    (char *)MJobAttr[mjaGJID],  /* SystemJID */
    (void *)J->Name,
    mdfString);

  if (CmdLine == NULL)
    CmdLine = (char *)MUCalloc(1,CmdLineSize * sizeof(char)); 
  else
    CmdLine[0] = '\0';

  if (MXMLToXString(RE,&CmdLine,&CmdLineSize,MaxCmdLineSize,NULL,TRUE) == FAILURE)
    {
    MDB(1,fNAT) MLog("ERROR:    cannot start job: '%s', failure generating XML\n",
      J->Name);

    MMBAdd(&J->MessageBuffer,"Could not start job: failure generating XML",NULL,mmbtNONE,0,0,NULL); 

    return(FAILURE);
    }
                                                                                
  MXMLDestroyE(&RE);

  R->P[R->ActingMaster].IsNonBlocking = TRUE;  /* FIXME: finish code that handles pending requests! */

  if (__MMDoCommand(
         R,
         &R->P[R->ActingMaster],
         mcsMJobCtl,
         CmdLine,
         Response,         /* O */
         sizeof(Response),
         EMsg,
         SC) == FAILURE)
    {
    /* cannot execute job */

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  if (strstr(Response,"ERROR"))
    {
    mbool_t BlockPartition = FALSE;
    char    BMsg[MMAX_LINE];

    /* error occurred */
    
    MDB(1,fNAT) MLog("ERROR:    cannot start job: '%s'\n",
      Response);

    if (EMsg != NULL)
      {
      char *ptr;

      /* remove PENDING from end of error message */      

      ptr = strstr(Response,"PENDING");
      if (ptr != NULL)
        *ptr = '\0';
      
      MUStrCpy(EMsg,Response,MMAX_LINE);
      }

    switch (R->P[R->ActingMaster].S->StatusCode)
      {
      case msfEGSInternal:

        BlockPartition = TRUE;

        sprintf(BMsg,"general failure starting job on RM '%s'",
          R->Name);

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (R->P[R->ActingMaster].S->StatusCode) */

    if (BlockPartition == TRUE)
      {
      /* eliminate partition from scheduling */

      MJobDisableParAccess(J,&MPar[R->PtIndex]);

      MMBAdd(&J->MessageBuffer,BMsg,NULL,mmbtNONE,0,0,NULL); 
      }  /* END if (BlockPartition == TRUE) */

    MSUFree(R->P[R->ActingMaster].S);

    if ((SC != NULL) && (*SC == mscNoError))
      *SC = mscRemoteFailure;

    return(FAILURE);
    }  /* END if (strstr(Response,"ERROR")) */

  if (strstr(Response,"PENDING") != NULL)
    {
    /* Moab doesn't know where the job has started or not. Moab relies on 
     * the next workload query to know if the job was successful or not.
     * In the case of guaranteed preemption, if moab assumes that this job
     * successfully started, then J->InRetry won't be set and the job will
     * won't hold on to it's reservation and will preempt more than it 
     * should. Returning failure will cause the job to get a reservation
     * and hang onto it for JobRetryTime. */

    if ((bmisset(&J->Flags,mjfPreempted)) &&
        (MSched.GuaranteedPreemption == TRUE))
      return(FAILURE);
    }

  MSUFree(R->P[R->ActingMaster].S);

  MDB(1,fNAT) MLog("INFO:     job '%s' successfully started\n",
    J->Name);
 
  return(SUCCESS);
  }  /* END MMJobStart() */




/**
 * Cancel job via Moab RM interface.
 *
 * @param J (I)
 * @param R (I)
 * @param Msg (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMJobCancel(

  mjob_t               *J,    /* I */
  mrm_t                *R,    /* I */
  char                 *Msg,  /* I (optional) */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  mxml_t *RE;
  mxml_t *DE;
  
  char CmdLine[MMAX_LINE];
  char Response[MMAX_LINE];

  const char *FName = "MMJobCancel";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,ErrMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    (Msg != NULL) ? Msg : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((J == NULL) || (R == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  RE = NULL;

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);
                                                                                
  MS3SetObject(RE,(char *)MXO[mxoJob],NULL);

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MJobCtlCmds[mjcmCancel],
    mdfString);

  if (J->DRMJID != NULL)
    {
    MXMLSetAttr(
      RE,
      (char *)MXO[mxoJob],
      (void *)J->DRMJID,
      mdfString);
    }
  else
    {
    MXMLSetAttr(
      RE,
      (char *)MXO[mxoJob],
      (void *)J->Name,
      mdfString);
    }

  /* used for jobs with possible temp. pending IDs */

  MXMLSetAttr(
    RE,
    (char *)MJobAttr[mjaGJID],  /* SystemJID */
    (void *)J->Name,
    mdfString);
    
  MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE);

  MXMLDestroyE(&RE);

  R->P[R->ActingMaster].IsNonBlocking = TRUE;

  if (__MMDoCommand(
       R,
       &R->P[R->ActingMaster],
       mcsMJobCtl,
       CmdLine,
       Response,
       sizeof(Response),
       EMsg,
       SC) == FAILURE)
    {
    /* cannot cancel/terminate job */

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute cancel command");

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  DE = (mxml_t *)R->P[R->ActingMaster].S->RDE;

  if ((DE->Val != NULL) &&
      !strcmp(DE->Val,"PENDING"))
    {
    /* non-blocking response */

    MDB(1,fNAT) MLog("INFO:     request to cancel job '%s' sent, but could not confirm cancelation (pending response)\n",
      J->Name);

    if (EMsg != NULL)
      {
      snprintf(
        EMsg,
        MMAX_LINE,
        "request to cancel sent, but could not confirm cancelation (pending response)");
      }

    if (SC != NULL)
      *SC = mscPending;

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  if (strstr(Response,"ERROR"))
    {
    /* error occured */
    
    MDB(1,fNAT) MLog("ERROR:    cannot cancel job: '%s'\n",
      Response);

    if (EMsg != NULL)
      {
      MUStrCpy(EMsg,Response,MMAX_LINE);
      }

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  MSUFree(R->P[R->ActingMaster].S);

  /* don't update job information, wait for info from RM */

  return(SUCCESS);
  }  /* END MMJobCancel() */




/**
 *
 *
 * @param J (I) [modified]
 * @param R (I)
 * @param Sig (I) [optional, -1=notset]
 * @param Action (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMJobSignal(

  mjob_t *J,      /* I (modified) */
  mrm_t  *R,      /* I */
  int     Sig,    /* I (optional, -1=notset) */
  char   *Action, /* I non-signal based action or <ACTION>=<VAL> (optional) */
  char   *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  int    *SC)     /* O (optional) */

  {
  char  Val[MMAX_LINE];
  char  CmdLine[MMAX_LINE];
  char  Response[MMAX_LINE];
  char  tmpSig[MMAX_LINE];
  enum MStatusCodeEnum tmpSC;

  mxml_t *RE;
  mxml_t *SE;
  mxml_t *DE;

  const char *FName = "MMJobSignal";

  MDB(3,fNAT) MLog("%s(%s,%s,%d,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    Sig,
    (Action != NULL) ? Action : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != 0)
    *SC = 0;

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if (MUBuildSignalString(Sig,Action,tmpSig) == FAILURE)
    {
    return(FAILURE);
    }

  RE = NULL;
  SE = NULL;

  /* FORMAT: <Request action="signal" actor="{USER}" job="{DRMJID}">
   * <Object>job</Object><Set name="signal">{Val}</Set></Request> */

  /* create request XML */  

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MJobCtlCmds[mjcmSignal],
    mdfString);

  MXMLSetAttr(RE,(char *)MXO[mxoJob],(void *)J->DRMJID,mdfString);

  MS3SetObject(RE,(char *)MXO[mxoJob],NULL);

  /* create set for specifying the signal*/

  MXMLCreateE(&SE,(char*)MSON[msonSet]);

  MXMLSetAttr(SE,MSAN[msanName],(char *)"signal",mdfString);

  if (Sig != -1)
    {
    snprintf(Val,sizeof(Val),"%d",
      Sig);
    }
  else
    {
    MUStrCpy(Val,Action,sizeof(Val));
    }

  MXMLSetVal(SE,Val,mdfString);

  MXMLAddE(RE,SE);

  MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE);

  MXMLDestroyE(&RE);

  tmpSC = mscNoError;

  R->P[R->ActingMaster].IsNonBlocking = TRUE;  /* FIXME: support non-blocking signal calls */

  if (__MMDoCommand(
       R,
       &R->P[R->ActingMaster],
       mcsMJobCtl,
       CmdLine,
       Response,
       sizeof(Response),
       EMsg,
       &tmpSC) == FAILURE)
    {
    /* cannot signal job */

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute signal command");

    if (SC != NULL)
      *SC = tmpSC;

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  DE = (mxml_t *)R->P[R->ActingMaster].S->RDE;

  if (SC != NULL)
    *SC = tmpSC;

  if ((DE->Val != NULL) &&
      !strcmp(DE->Val,"PENDING"))
    {
    /* non-blocking response */

    /* NYI */
    }

  if (strstr(Response,"ERROR"))
    {
    /* error occured */
    
    MDB(1,fNAT) MLog("ERROR:    cannot signal job: '%s'\n",
      Response);

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  MSUFree(R->P[R->ActingMaster].S);

  return(SUCCESS);
  }  /* END MMJobSignal() */






/**
 * Load node info from Moab peer interface (moab grid).
 *
 * @see MMNodeUpdate() - peer
 * @see __MMClusterQueryPostLoad() - parent
 *
 * @param N (I) [modified]
 * @param NE (I)
 * @param NewState (I)
 * @param R (I)
 */

int MMNodeLoad(

  mnode_t            *N,        /* I (modified) */
  mxml_t             *NE,       /* I */
  enum MNodeStateEnum NewState, /* I */
  mrm_t              *R)        /* I */

  {
  const char *FName = "MMNodeLoad";

  MDB(3,fRM) MLog("%s(%s,NE,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (N != NULL) ? MNodeState[N->State] : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* map node attributes */

  if ((R != NULL) && (R->OMap != NULL))
    {
    if (MMNodeOMap(NE,R) == FAILURE)
      {
      return(FAILURE);
      }
    }

  /* translate node from XML */

  MNodeFromXML(N,NE,TRUE);

  /* remove cluster-specific information */

  N->SlotIndex = -1;
  N->RackIndex = -1;
  
  /* update non-compute generic resources */

  if ((R != NULL) && 
      (!bmisset(&R->RTypes,mrmrtCompute)) && 
      (!MSNLIsEmpty(&N->CRes.GenericRes)))
    {
    int gindex;

    for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
      {
      if (MGRes.Name[gindex][0] == '\0')
        break;

      if (MSNLGetIndexCount(&N->CRes.GenericRes,gindex) > 0)
        {
        MGRes.GRes[gindex]->NotCompute = TRUE;
        }
      }
    }

  return(SUCCESS);
  }  /* END MMNodeLoad() */





/**
 * Update node from XML reported via Moab interface (grid peer).
 *
 * @see MMNodeLoad() - peer
 * @see __MMClusterQueryPostLoad() - parent
 *
 * @param N (I) [modified]
 * @param NE (I)
 * @param NewState (I)
 * @param R (I)
 */

int MMNodeUpdate(
                                                                                
  mnode_t *N,   /* I (modified) */
  mxml_t  *NE,  /* I */
  enum MNodeStateEnum NewState, /* I */
  mrm_t   *R)   /* I */
                                                                                
  {
  const char *FName = "MMNodeUpdate";
    
  MDB(7,fRM) MLog("%s(%s,NE,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (N != NULL) ? MNodeState[N->State] : "NULL",
    (R != NULL) ? R->Name : "NULL");
                                                                                
  if (N == NULL)
    {
    return(FAILURE);
    }

  /* map node attributes */

  if ((R != NULL) && (R->OMap != NULL))
    {
    if (MMNodeOMap(NE,R) == FAILURE)
      {
      return(FAILURE);
      }
    }

  /* convert node from XML */

  MNodeFromXML(N,NE,TRUE);

  /* remove cluster-specific information */

  N->SlotIndex = -1;
  N->RackIndex = -1;

  /* update non-compute generic resources */

  if ((R != NULL) && 
      (!bmisset(&R->RTypes,mrmrtCompute)) && 
      (!MSNLIsEmpty(&N->CRes.GenericRes)))
    {
    int gindex;

    for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
      {
      if (MGRes.Name[gindex][0] == '\0')
        break;

      if (MSNLGetIndexCount(&N->CRes.GenericRes,gindex) > 0)
        {
        MGRes.GRes[gindex]->NotCompute = TRUE;
        }
      }
    }

  N->ATime = MSched.Time;
                                                                                
  return(SUCCESS);
  }  /* END MMNodeUpdate() */




/**
 * Send system modify request to peer Moab.
 *
 * @param R (I) [modified]
 * @param AType
 * @param Attribute
 * @param IsBlocking
 * @param Value
 * @param Format
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMSystemModify(

  mrm_t                *R,      /* I (modified) */
  const char           *AType,
  const char           *Attribute,
  mbool_t               IsBlocking,
  char                 *Value,  /* (not currently used in function) */
  enum MFormatModeEnum  Format,
  char                 *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)     /* O (optional) */

  {
  char CmdLine[MMAX_LINE];
  char tmpAType[MMAX_LINE];

  enum MStatusCodeEnum tmpSC;

  mxml_t *RE;

  char *ptr;
  char *TokPtr = NULL;
  char *Action;

  const char *FName = "MMSystemModify";

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (R == NULL)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid parameters - no RM");

    return(FAILURE);
    }

  RE = NULL;

  if (MXMLCreateE(&RE,(char*)MSON[msonRequest]) == FAILURE)
    {
    MDB(1,fNAT) MLog("ALERT:    cannot create request XML in %s\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"internal error - no memory");

    return(FAILURE);
    }

  /* AType Format:  <objtype>:<action>:<attr> */
  /*           IE:  node:destroy */
  /*           IE:  node:create  */
  /*           IE:  file.name:modify:<SRC>,<USER>@<DST> */

  MUStrCpy(tmpAType,AType,sizeof(tmpAType));
  
  ptr = MUStrTok(tmpAType,":",&TokPtr);

  if (strstr(ptr,"node") != NULL)
    {
    enum MNodeCtlCmdEnum Mode;

    MS3SetObject(RE,(char *)MXO[mxoNode],NULL);

    Action = MUStrTok(NULL,":",&TokPtr);

    Mode = (enum MNodeCtlCmdEnum) MUGetIndexCI(Action,MNodeCtlCmds,FALSE,mncmNONE);

    MXMLSetAttr(RE,MSAN[msanAction],(void *)MNodeCtlCmds[Mode],mdfString);

    MS3AddWhere(RE,MSAN[msanName],Attribute,NULL);
    }
  else if (strstr(ptr,"file") != NULL) 
    {
    MXMLSetAttr(RE,MSAN[msanAction],(void *)"modify",mdfString);

    MS3AddWhere(RE,MSAN[msanName],Attribute,NULL);
    }

  MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE);

  MXMLDestroyE(&RE);

  /* send request */

  if (__MMDoCommand(
        R,
        &R->P[R->ActingMaster],      /* I/O */
        mcsMSchedCtl,
        CmdLine,
        NULL,
        0,
        EMsg,
        &tmpSC) == FAILURE)
    {
    if (SC != NULL)
      *SC = tmpSC;

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  if ((R->P[R->ActingMaster].S == NULL) || (R->P[R->ActingMaster].S->RDE == NULL))
    {
    /* NOTE:  removing vpc returns no data */

    /* NO-OP */
    }

  /* release rm static reservations */

  MSUFree(R->P[R->ActingMaster].S);
 
  return(SUCCESS);
  }  /* END MMSystemModify() */







/**
 * send info query request to peer Moab
 *
 * @param R (I)
 * @param QType (I) [optional]
 * @param Attribute (I)
 * @param Value (O) [minsize=MMAX_LINE]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMInfoQuery(

  mrm_t                *R,         /* I */
  char                 *QType,     /* I (optional) */
  char                 *Attribute, /* I */
  char                 *Value,     /* O (minsize=MMAX_LINE) */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  char           MsgBuffer[MMAX_BUFFER];
  char          *RPtr;

  mccfg_t        Peer;

  enum MSvcEnum  CIndex;

  int            rc;

  mbool_t        FullQuery = TRUE;

  const char *FName = "MMInfoQuery";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (QType != NULL) ? QType : "NULL",
    (Attribute != NULL) ? Attribute : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Value != NULL)
    Value[0] = '\0';

  if (QType != NULL) 
    {
    return(FAILURE);
    }  /* END if (QType != NULL) */

  if ((R == NULL) || (Attribute == NULL) || (Value == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error - invalid parameters");

    return(FAILURE);
    }

  if (R->State != mrmsActive) 
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is %s, info query request rejected\n",
      R->Name,
      MRMState[R->State]);

    if (EMsg != NULL)
      strcpy(EMsg,"resource manager unavailable");

    return(FAILURE);
    }

  if (MSched.Time - R->StateMTime < R->PollInterval)
    {
    return(SUCCESS);
    }

  /* call Moab peer */

  memset(&Peer,0,sizeof(Peer));

  MUStrCpy(Peer.ServerHost,R->P[R->ActingMaster].HostName,sizeof(Peer.ServerHost));
  MUStrCpy(Peer.ServerCSKey,R->P[R->ActingMaster].CSKey,sizeof(Peer.ServerCSKey));
  Peer.ServerCSAlgo = R->P[R->ActingMaster].CSAlgo;
  Peer.ServerPort   = R->P[R->ActingMaster].Port;

  if (FullQuery == TRUE)
    {
    /* NOTE:  should simultaneously send all queries (threaded/cached) */

    /* query on jobs/nodes */

    /* NYI */

    CIndex = mcsMSchedCtl;

    MUStrCpy(MsgBuffer,"",sizeof(MsgBuffer));
    }
  else
    {
    CIndex = mcsStatShow;

    MUStrCpy(MsgBuffer,"20 ESTSTARTTIME ALL 65536",sizeof(MsgBuffer));
    }

  MDB(2,fCONFIG) MLog("INFO:     sending peer server command: '%s'\n",
    MsgBuffer);

  rc = MCDoCommand(
    CIndex,
    &Peer,
    MsgBuffer,
    &RPtr,     /* O (alloc) */
    NULL,
    NULL);     /* O */

  /* do we need to free socket (NYI) */

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  /* NOTE:  is RPtr mem leak? */

  if (Value == NULL)
    MUStrDup((char **)&R->S,RPtr);
  else
    MUStrCpy(Value,RPtr,MMAX_LINE);

  return(SUCCESS);
  }  /* END MMInfoQuery() */




/**
 *
 *
 * @param NE (I)
 * @param R (I)
 * @param NodeID (O) [optional,minsize=MMAX_NAME]
 * @param Status (O) [optional]
 */

int __MMNodeGetState(
                                                                                
  mxml_t *NE,     /* I */
  mrm_t  *R,      /* I */
  char   *NodeID, /* O (optional,minsize=MMAX_NAME) */
  enum MNodeStateEnum *Status) /* O (optional) */
                                                                                
  {
  const char *FName = "__MMNodeGetState";
                                                                                
  MDB(5,fNAT) MLog("%s(%s,%s,%s,%s)\n",
    FName,    
    (NE != NULL) ? "NE" : "NULL",
    (R != NULL) ? R->Name : "NULL",
    (NodeID != NULL) ? "NodeID" : "NULL",
    (Status != NULL) ? "Status" : "NULL");
                                                                                
  if ((NE == NULL) || (R == NULL))
    {
    return(FAILURE);
    }
 
  if ((NodeID != NULL) && (NodeID[0] == '\0'))
    {
    /* get node name */

    if (MXMLGetAttr(NE,(char *)MNodeAttr[mnaNodeID],NULL,NodeID,MMAX_NAME) == FAILURE)
      {
      return(FAILURE);
      }
    }

  if ((NodeID != NULL) &&
      (!strcmp(NodeID,"GLOBAL")))
    {
    char tmpName[MMAX_NAME];

    sprintf(tmpName,"%s-GLOBAL",R->Name);

    MUStrCpy(NodeID,tmpName,MMAX_NAME); 
    }

  if (Status != NULL)
    {
    char tmpVal[MMAX_NAME];

    if (MXMLGetAttr(NE,(char *)MNodeAttr[mnaNodeState],NULL,tmpVal,sizeof(tmpVal)) == FAILURE)
      {
      return(FAILURE);
      }

    *Status = (enum MNodeStateEnum) MUGetIndex(tmpVal,MNodeState,FALSE,mnsNONE);

    if (*Status == mnsNONE)
      {
      return(FAILURE);
      }
    }    /* END if (Status != NULL) */

  return(SUCCESS);
  }  /* END __MMNodeGetState() */




/**
 *
 *
 * @param JE (I)
 * @param R (I)
 * @param JobID (I/O) [optional]
 * @param Status (O)
 */

int __MMJobGetState(

  mxml_t             *JE,      /* I */
  mrm_t              *R,       /* I */
  char               *JobID,   /* I/O (optional) */
  enum MJobStateEnum *Status)  /* O */

  {
  const char *FName = "__MMJobGetState";
                                                                                
  MDB(5,fNAT) MLog("%s(%s,%s,%s,%s)\n",
    FName,
    (JE != NULL) ? "JE" : "NULL",
    (R != NULL) ? R->Name : "NULL",
    (JobID != NULL) ? "JobID" : "NULL",
    (Status != NULL) ? "Status" : "NULL");
                                                                                
  if ((JE == NULL) || (R == NULL))
    {
    return(FAILURE);
    }
                                                                                
  if ((JobID != NULL) && (JobID[0] == '\0'))
    {
    /* get job name */
                                                                                
    if (MXMLGetAttr(JE,(char *)MJobAttr[mjaJobID],NULL,JobID,MMAX_NAME) == FAILURE)
      {
      return(FAILURE);
      }
    }
                                                                                
  if (Status != NULL)
    {
    char tmpVal[MMAX_NAME];
                                                                                
    if (MXMLGetAttr(JE,(char *)MJobAttr[mjaState],NULL,tmpVal,sizeof(tmpVal)) == FAILURE)
      {
      return(FAILURE);
      }
                                                                                
    *Status = (enum MJobStateEnum) MUGetIndex(tmpVal,MJobState,FALSE,mjsNONE);
                                                                                
    if (*Status == mjsNONE)
      {
      return(FAILURE);
      }
    }    /* END if (Status != NULL) */

  return(SUCCESS);
  }  /* END __MMJobGetState() */




/**
 *
 *
 * @param R     (I)
 * @param J     (I)
 * @param IData (I) [optional]
 * @param OE    (O) [optional]
 * @param EMsg  (O) [optional,minsize=MMAX_LINE]
 * @param SC    (O) [optional]
 */

int MMJobQuery(

  mrm_t                 *R,
  mjob_t                *J,
  char                  *IData,
  mxml_t               **OE,
  char                  *EMsg,
  enum MStatusCodeEnum  *SC)

  {
  mxml_t *RE = NULL;
  mxml_t *DE = NULL;
  mxml_t *JE = NULL;      
  
  enum MResAvailQueryMethodEnum RAQMethod;
  char CmdLine[MMAX_BUFFER];
  enum MStatusCodeEnum tmpSC;

  const char *FName = "MMJobQuery";

  MDB(2,fRM) MLog("%s(%s,%s,IData,OData,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (J != NULL) ? J->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((R == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  RAQ method specified via IData */

  if (IData != NULL)
    {
    RAQMethod = (enum MResAvailQueryMethodEnum)MUGetIndexCI(IData,MResAvailQueryMethod,FALSE,mraqmLocal);
    }
  else
    {
    RAQMethod = mraqmNONE;
    }

  if (MMJobQueryCreateRequest(J,R,RAQMethod,&RE,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE);

  MXMLDestroyE(&RE);

  /* NOTE: query is of type BLOCKING */

  if (__MMDoCommand(
        R,
        &R->P[R->ActingMaster],      /* I/O */
        mcsMSchedCtl,
        CmdLine,
        NULL,
        0,
        EMsg,
        &tmpSC) == FAILURE)
    {
    if (SC != NULL)
      *SC = tmpSC;

    if (R->P[R->ActingMaster].S->StatusCode == msfEGServerBus)
      {
      /* re-initialize RM interface */

      MRMSetState(R,mrmsCorrupt);

      /* FIXME: should we set this to 0 or 5 minutes ago? */

      R->StateMTime = 0;
      }

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  /* process response */

  DE = (mxml_t *)R->P[R->ActingMaster].S->RDE;

  if (IData != NULL)
    {
    RAQMethod = (enum MResAvailQueryMethodEnum)MUGetIndexCI((char *)IData,MResAvailQueryMethod,FALSE,mraqmNONE);
    }
  else
    {
    RAQMethod = mraqmNONE;
    }

  if (MXMLGetChildCI(DE,(char *)MXO[mxoJob],NULL,&JE) == FAILURE)
    {
    /* invalid response - no job child found */

    return(FAILURE);
    }

  MXMLDupE(JE,OE);
        
  MSUFree(R->P[R->ActingMaster].S);

  return(SUCCESS);
  }  /* END MMJobQuery() */





/**
 * Append char array auth list to access control list (macl_t *)
 *
 * @param AuthList (I)
 * @param AuthType (I)
 * @param ACL (I) [modified]
 */

int __MMAppendAuthListToACL(

  char           **AuthList, /* I */
  enum MAttrEnum   AuthType, /* I */
  macl_t         **ACL)      /* I (modified) */   

  {
  int authIndex;

  if ((AuthList == NULL) || (ACL == NULL))
    {
    return(FAILURE);
    }
    
  for (authIndex = 0;authIndex < MMAX_CREDS;authIndex++)
    {
    if (AuthList[authIndex] == NULL)
      break;

    if (MACLSet(
        ACL,
        AuthType,
        (void *)AuthList[authIndex],
        mcmpNONE,
        mnmNeutralAffinity, /* encourage local RMs over peer RMs */
        0,
        TRUE) == FAILURE)
      {
      return(FAILURE);
      }
    }    /* END for (authIndex) */

  return(SUCCESS);
  }  /* END __MMAppendAuthListToACL() */





/**
 * Process pre-loaded 'initialize' response from peer.
 *
 * @param S (I)
 * @param DPtr (I)
 * @param Arg2 (I)
 * @param Arg3 (I)
 * @param Arg4 (I)
 */

int __MMInitializePostLoad(

  msocket_t *S,    /* I */
  void      *DPtr, /* I */
  void      *Arg2, /* I */
  void      *Arg3, /* I */
  void      *Arg4) /* I */

  {
  mxml_t *DE;
  mxml_t *SE;
  mrm_t  *R;
 
  char tmpLine[MMAX_LINE];
 
  if ((S == NULL) || (DPtr == NULL))
    {
    return(FAILURE);
    }

  R = (mrm_t *)DPtr;

  /* process results */

  DE = (mxml_t *)S->RDE;

  if (MXMLGetChild(DE,(char *)MXO[mxoSched],NULL,&SE) == SUCCESS)
    {
    char FlagString[MMAX_LINE];
    char tmpLine[MMAX_LINE];
    char *ptr;
    char *TokPtr;
    int langCount;

    enum MRMTypeEnum tmpRMType;

    int   CTok;

    mxml_t *CE;

    mclass_t *C;

    mbitmap_t RMFlags;

    if (MXMLGetAttr(SE,"flags",NULL,FlagString,sizeof(FlagString)) == SUCCESS)
      {
      /* load rm flags */

      bmfromstring(FlagString,MRMFlags,&RMFlags," ,:\t\n");

      R->TypeIsExplicit = TRUE;

      if (bmisset(&RMFlags,mrmfSlavePeer))
        {
        bmset(&R->Flags,mrmfSlavePeer);
        bmunset(&R->Flags,mrmfAutoStart);
        }

      if (bmisset(&RMFlags,mrmfAutoStart))
        {
        bmset(&R->Flags,mrmfAutoStart);
        }
      }    /* END if (MXMLGetAttr("flags") == SUCCESS */

    if (MXMLGetAttr(SE,(char *)MSchedAttr[msaVarList],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      MULLUpdateFromString(&R->Variables,tmpLine,",");
      }  /* END if (MXMLGetAttr() == SUCCESS) */

    /* load in clock skew */

    if (MXMLGetAttr(SE,"time",NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      mulong RMTime;

      RMTime = strtol(tmpLine,NULL,10);

      if ((RMTime > 0) &&
          bmisset(&R->Flags,mrmfClockSkewChecking))
        {
        /* note - clock skew checking can cause problems since it is only set when we initialize
                  communication with the RM. If the clock is adjusted later we still use this
                  computed clock skew unti the next moab reboot.
         
                  Also, the relation between MSched.Time on this system
                  and the time sent in the response message can be wildly out of sync
                  due to processing delays, timeouts, etc. even if the clocks on the two
                  machines are identical.
         
                  Most systems these days should be using an NTP server to keep the clocks in sync. */

        R->ClockSkewOffset = MSched.Time - RMTime;
        }
      else
        {
        R->ClockSkewOffset = 0;
        }
      }

    /* extract rm language info */
    /* FORMAT: TYPE[:SUBTYPE][,...] */

    MXMLGetAttr(SE,"rmlanguages",NULL,tmpLine,sizeof(tmpLine));

    /* TODO: put the following code into a nice function */

    TokPtr = 0;
    ptr = MUStrTok(tmpLine,",",&TokPtr);

    for (langCount = 0;langCount < MMAX_RM;langCount++)
      {
      char *ptr2;
      char *TokPtr2;
      enum MRMSubTypeEnum SubRMType;

      if (ptr == NULL)
        break;

      ptr2 = MUStrTok(ptr,":",&TokPtr2);
      
      tmpRMType = (enum MRMTypeEnum)MUGetIndexCI(ptr2,MRMType,FALSE,mrmtNONE);

      if (tmpRMType == mrmtNONE)
        {
        MDB(3,fRM) MLog("WARNING:  peer '%s' reported unknown RM language: '%s'\n",
          R->Name,
          ptr2);

        ptr = MUStrTok(NULL,",",&TokPtr);

        continue;
        }

      bmset(&R->Languages,tmpRMType);

      ptr2 = MUStrTok(NULL,":",&TokPtr2);

      if (ptr2 != NULL)
        {
        SubRMType = (enum MRMSubTypeEnum)MUGetIndexCI(ptr2,MRMSubType,FALSE,mrmstNONE);

        if (SubRMType == mrmstNONE)
          {
          MDB(3,fRM) MLog("WARNING:  peer '%s' reported unknown RM sub-language: '%s'\n",
            R->Name,
            ptr2);

          ptr = MUStrTok(NULL,",",&TokPtr);

          continue;
          }

        bmset(&R->SubLanguages,SubRMType);
        }

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END for (langCount) */

    CTok = -1;

    while (MXMLGetChild(SE,(char *)MXO[mxoClass],&CTok,&CE) == SUCCESS)
      {
      int pindex = (MSched.PerPartitionScheduling == TRUE) ? R->PtIndex : 0;
      unsigned long BoundedLimit = 0;

      if (MXMLGetAttr(CE,(char *)MCredAttr[mcaID],NULL,tmpLine,sizeof(tmpLine)) == FAILURE)
        {
        /* XML is corrupt */

        continue;
        }

      if (!strcmp(tmpLine,"DEFAULT"))
        {
        /* ignore remote default queue info */

        continue;
        }

      if (MClassFind(tmpLine,&C) == SUCCESS)
        {
        if (!bmisset(&C->Flags,mcfRemote))
          {
          /* NOTE:  class configured both locally and remotely - ignore remote config */

          continue;
          }
        }

      if (MClassAdd(tmpLine,&C) == FAILURE)
        {
        /* NOTE:  cannot add class */

        continue;
        }

      /* If per-partition scheduling then put the limits on the specific 
       * partition else put it on the global partition. */

      MOFromXML(
          (void *)C,
          mxoClass,
          CE,
          (MSched.PerPartitionScheduling == TRUE) ? &MPar[R->PtIndex] : NULL);

      /* check to see that special limits weren't exceeded by this change -- 
         if so, truncate them (MOAB-3560) */

                 /* WCLimit ceiling */
      if ((C->L.JobMaximums[pindex]) &&
          (MClassGetEffectiveRMLimit(C,pindex,mcaMaxWCLimitPerJob,&BoundedLimit) == SUCCESS) && 
          (BoundedLimit < C->L.JobMaximums[pindex]->WCLimit))
        {
        MDB(3,fRM) MLog("WARNING:  truncating max WCLimit for class:%s, (rm reports:%d  Moab enforces:%d)\n",
                        C->Name,
                        C->L.JobMaximums[pindex]->WCLimit,
                        BoundedLimit);
        C->L.JobMaximums[pindex]->WCLimit        = BoundedLimit;
        C->L.JobMaximums[pindex]->SpecWCLimit[0] = BoundedLimit;
        }

                 /* MAX.NODE ceiling */
      if ((C->L.JobMaximums[pindex]) &&
          (MClassGetEffectiveRMLimit(C,pindex,mcaMaxNodePerJob,&BoundedLimit) == SUCCESS) && 
          ((int)BoundedLimit < C->L.JobMaximums[pindex]->Request.NC))
        {
        MDB(3,fRM) MLog("WARNING:  truncating MAX.NODE for class:%s, (rm reports:%d  Moab enforces:%d)\n",
                        C->Name,
                        C->L.JobMaximums[pindex]->Request.NC,
                        BoundedLimit);
        C->L.JobMaximums[pindex]->Request.NC     = BoundedLimit;
        }

                 /* MIN.NODE floor */
      if ((C->L.JobMinimums[pindex]) &&
          (MClassGetEffectiveRMLimit(C,pindex,mcaMinNodePerJob,&BoundedLimit) == SUCCESS) && 
          ((int)BoundedLimit > C->L.JobMinimums[pindex]->Request.NC))
        {
        MDB(3,fRM) MLog("WARNING:  truncating MIN.NODE for class:%s, (rm reports:%d  Moab enforces:%d)\n",
                        C->Name,
                        C->L.JobMinimums[pindex]->Request.NC,
                        BoundedLimit);
        C->L.JobMinimums[pindex]->Request.NC     = BoundedLimit;
        }
      }    /* END  while (MXMLGetChild... */

    bmclear(&RMFlags);
    }     /* END if (MXMLGetChild() == SUCCESS) */
    
  /* parse high-availability information (fbserver) */
  
  if (MXMLGetAttr(SE,(char *)MSchedAttr[msaFBServer],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
    {
    MRMSetAttr(R,mrmaFBServer,(void **)tmpLine,mdfString,mSet);
    }        

  if (R->State == mrmsNONE)
    { 
    if (MMPing(R,NULL) == SUCCESS)
      {
      MRMSetState(R,mrmsActive);
      }
    else
      {
      MRMSetState(R,mrmsDown);
      }
    }    /* END if (R->State == mrmsNONE) */

  return(SUCCESS);
  }  /* END __MMInitializePostLoad() */






#define NODE_VALID_TIME 300  /* 5 minutes */

 

/**
 * Process loaded cluster query response (for ClusterQueryURL).
 *
 * NOTE:  Cluster data contained in R->U.Moab.CData.
 *
 * NOTE:  Cluster query response generated by peer in MUISchedCtl()
 *        see case msctlQuery->case mxoCluster
 *
 * @see MMClusterQuery() - parent
 * @see MMRsvQueryPostLoad() - child
 *
 * @param R (I)
 * @param RCountP (O) [ignored]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int __MMClusterQueryPostLoad(

  mrm_t                *R,       /* I */
  int                  *RCountP, /* O (ignored) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  mxml_t *DE;
  mxml_t *NE;
 
  mnode_t *N;
  enum MNodeStateEnum NSIndex;
  mbool_t NewNode = FALSE;  
  char NodeName[MMAX_NAME]; 
  char tmpName[MMAX_LINE];

  int CTok;
  int RCount;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if ((R->U.Moab.CData == NULL) ||
      (R->U.Moab.CDataIsProcessed == TRUE)) /* data has already been processed in previous iteration */
    {
    /* no fresh data to process */

    if (R->U.Moab.CData != NULL)
      {
      MDB(3,fRM) MLog("INFO:     skipping stale node data (current iteration: %d, data iteration: %d)\n",
        MSched.Iteration,
        R->U.Moab.CIteration);
      }

    return(SUCCESS);
    }

  DE = (mxml_t *)R->U.Moab.CData;

  /* R->U.Moab.CData = NULL; - so we can free it in __MMWorkloadQueryGetData() */

  /* process results */

  RCount = 0;

  CTok = -1;

  while (MXMLGetChild(DE,(char *)MXO[mxoNode],&CTok,&NE) == SUCCESS)
    {
    NodeName[0] = '\0';
    N = NULL;

    if (__MMNodeGetState(NE,R,NodeName,&NSIndex) == FAILURE)
      {
      /* node object is corrupt */
                                                                                
      continue;
      }

    RCount++;

    /* translate names according to given mapping */
    /* TODO: check for a RM->Boolean to indicate whether we should translate or not */    

    if (MOMap(R->OMap,mxoNode,NodeName,TRUE,FALSE,tmpName) == SUCCESS)
      {
      MUStrCpy(NodeName,tmpName,sizeof(NodeName));
      }

    /* The "max" node is net yet utilized! */

    /* NYI */

    if ((MNodeFind(NodeName,&N) == SUCCESS) && (bmisset(&N->IFlags,mnifRMDetected)))
      {
      MRMNodePreUpdate(N,NSIndex,R);

      MMNodeUpdate(N,NE,NSIndex,R);

      /* increment node hop count */
      
      N->HopCount++;

      MRMNodePostUpdate(N,NULL);
      }
    else if (MNodeAdd(NodeName,&N) == SUCCESS)
      {
      NewNode = TRUE;
      MRMNodePreLoad(N,NSIndex,R);

      MMNodeLoad(N,NE,NSIndex,R);

      /* increment node hop count */

      N->HopCount++;

      MNLAddNode(&R->NL,N,1,TRUE,NULL);

      MRMNodePostLoad(N,NULL);

      MDB(2,fNAT)
        MNodeShow(N);
      }  /* END else if (MNodeAdd(NodeName,&N) == SUCCESS) */
    else
      {
      MDB(1,fNAT) MLog("ERROR:    node buffer is full  (ignoring node '%s')\n",
        NodeName);
      }
    }    /* END while (MXMLGetChild(DE,MXO[mxoNode],&CTok,&NE) == SUCCESS) */

  R->NC = RCount;

  MMRsvQueryPostLoad(DE,R,FALSE);

  MSUFree(R->P[R->ActingMaster].S);

  if (!MNLIsEmpty(&R->NL))
    {
    int nindex;
    mnl_t     *NL;
    mnode_t   *N;

    /* detect if node info has become stale (hasn't been reported by remote peer) */

    NL = &R->NL;

    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS; nindex++)
      {
      if ((MSched.Time - N->ATime) > NODE_VALID_TIME)
        {
        /* node is stale - mark as down */

        char tmpLine[MMAX_LINE];
        char DString[MMAX_LINE];

        MNodeSetState(N,mnsDown,FALSE);

        MULToDString(&N->ATime,DString);

        sprintf(tmpLine,"node has not been reported by peer since '%s'",
          DString);
        
        MMBAdd(
          &N->MB,
          tmpLine,
          "N/A",
          mmbtOther,
          MCONST_DAYLEN,
          1,
          NULL);
        }
      }  /* END for (nindex) */
    }    /* END if (R->NL != NULL) */

  if (NewNode == TRUE)
    {
    /* load queue info */

    /* NYI */
    }

  /* mark the node data as being processed so we don't go through it again */

  R->U.Moab.CDataIsProcessed = TRUE;

  return(SUCCESS);
  }  /* END __MMClusterQueryPostLoad() */





/**
 * Process workload query results.
 *
 * @param R (I)
 * @param WCount (O) [optional]
 * @param NWCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int __MMWorkloadQueryPostLoad(

  mrm_t                *R,       /* I */
  int                  *WCount,  /* O (optional) */
  int                  *NWCount, /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  job_iter JTI;

  int CTok;

  int tmpWCount;
  int tmpNWCount;
  mulong PurgeTime;

  mxml_t *JE;
  mxml_t *DE;

  mjob_t *J;

  char JobID[MMAX_NAME];
  char Message[MMAX_LINE];
  char tmpBuf[MMAX_BUFFER];

  mln_t *JobsComletedList = NULL;
  mln_t *JobsRemovedList = NULL;
  mln_t *JobListPtr = NULL;

  char JClass[MMAX_NAME];

  enum MJobStateEnum JSIndex;

  int *TaskList = NULL;

  const char *FName = "__MMWorkloadQueryPostLoad";
    
  MDB(6,fNAT) MLog("%s(%s,WCount,NWCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (R == NULL)
    {
    return(FAILURE);
    }

  tmpWCount = 0;
  tmpNWCount = 0;

  if (WCount == NULL)
    {
    WCount = &tmpWCount;
    }

  if (NWCount == NULL)
    {
    NWCount = &tmpNWCount; 
    }

  if ((R->U.Moab.WData == NULL) ||
      (R->U.Moab.WDataIsProcessed == TRUE)) /* data has already been processed in previous iteration */
    {
    /* no fresh data to process */

    if (R->U.Moab.WData != NULL)
      {
      MDB(3,fRM) MLog("INFO:     skipping stale workload data (current iteration: %d, data iteration: %d)\n",
        MSched.Iteration,
        R->U.Moab.WIteration);
      }

    return(SUCCESS);
    }
  
  DE = (mxml_t *)R->U.Moab.WData;

  /* R->U.Moab.WData = NULL; - so we can free it in __MMWorkloadQueryGetData */

  CTok = -1;

  while (MXMLGetChild(DE,(char *)MXO[mxoJob],&CTok,&JE) == SUCCESS)
    {
    JobID[0] = '\0';

    J = NULL;

    if (__MMJobGetState(JE,R,JobID,&JSIndex) == FAILURE)
      {
      /* job object is corrupt */

      continue;
      }

    /* see if this job is from an accepted/allowed class */

    if ((MSched.SpecClasses != NULL) || (MSched.IgnoreClasses != NULL))
      {
      if (MXMLGetAttr(JE,(char *)MJobAttr[mjaClass],NULL,JClass,sizeof(JClass)) == SUCCESS)
        {
        if (MUCheckList(JClass,MSched.SpecClasses,MSched.IgnoreClasses) == FALSE)
          continue;
        }
      }

    /* add up jobs in workload if we are under the allowed job capacity */

    if ((R->MaxJobsAllowed > 0) &&
        (*WCount >= R->MaxJobsAllowed) &&
        (MSTATEISCOMPLETE(JSIndex) == FALSE))
       
      {
      /* we aren't allowed to load in any more active jobs from this RM */

      continue;
      }

    (*WCount)++;

    /* NOTE: must localize job (reduce layer of GJID's, GSID's, and adjust JID) */

    /* should sync with SSS, when SSS supports GJID */
    /* NOTE: SSS JobId != JobID - FIXME! */

    if (MXMLGetAttr(JE,(char *)MJobAttr[mjaGJID],NULL,tmpBuf,sizeof(tmpBuf)) == SUCCESS)
      {
      /* override reported JobID with the "system" jobID */

      MUStrCpy(JobID,tmpBuf,sizeof(JobID));

      if (JobID[0] == '\0')
        {
        char tmpXMLLine[MMAX_LINE * 2];

        MXMLToString(JE,tmpXMLLine,sizeof(tmpXMLLine),NULL,TRUE);

        MDB(1,fRM) MLog("ALERT:    job ID is empty after MXMLGetAttr (%s)!\n",
          tmpXMLLine);
        }

      MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],(void **)JobID,mdfString);

      /* When heiarchical P2P is supported SJID and SID should be reduced, not destroyed */

      MXMLSetAttr(JE,(char *)MJobAttr[mjaGJID],(void **)NULL,mdfString);
      MXMLSetAttr(JE,(char *)MJobAttr[mjaSID],(void **)NULL,mdfString);
      }

    TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

    switch (JSIndex)
      {
      case mjsIdle:
      case mjsStarting:
      case mjsRunning:
      case mjsSuspended:
      case mjsHold:

        if (MJobFind(JobID,&J,mjsmExtended) == SUCCESS)
          {
          if ((J->DestinationRM != NULL) && (J->DestinationRM != R))
            {
            /* we can't believe what this RM says--it doesn't "own" the job */

            MDB(3,fRM) MLog("WARNING:  job '%s' is being reported by RM '%s' but is owned by RM '%s'!\n",
              J->Name,
              R->Name,
              (J->DestinationRM != NULL) ? J->DestinationRM->Name : "NULL");

            continue;
            }

          MRMJobPreUpdate(J);

          MMJobUpdate(JE,J,TaskList,R);
          
          /* if J->EState == mjsActive, then adjust job's per RM->StartFailureCount
           * and possibly set a job hold - NYI JOSH */

          MRMJobPostUpdate(J,TaskList,JSIndex,R);

#ifdef __MDEBUG
          bmunset(&J->SpecFlags,mjfNoRMStart);
          bmunset(&J->Flags,mjfNoRMStart);
#endif /* __MDEBUG */
          }
        else
          {
          if (MJobCreate(JobID,TRUE,&J,NULL) == SUCCESS)
            {
            /* add up new jobs in workload */

            (*NWCount)++;

            MRMJobPreLoad(J,JobID,R);

            MMJobLoad(JobID,JE,J,TaskList,R);
      
            if (J->DRMJID != NULL)
              {
              /* job is being reported first by this peer--make it the DRM */
 
              MJobSetAttr(J,mjaDRM,(void **)R,mdfOther,mSet);
              }

            if (MRMJobPostLoad(J,TaskList,R,NULL) == FAILURE)
              {
              MJobRemove(&J);

              continue;
              }

            /* If we received a PeerReqRID from a workload query and our
             * current ReqRID is "ALL" (It can be ALL in the case that the RM
             * on the slave reports ADVRES as "ALL" in the extensions since
             * it does not know about the reservation on the master) then we
             * have lost our original ReqRID (due to a moab restart) so
             * restore the original ReqRID which was saved and reported by
             * the peer.*/

            if ((J->PeerReqRID != NULL) && (J->ReqRID != NULL) && (strcmp(J->ReqRID,"ALL") == 0))
              {
              MUStrDup(&J->ReqRID,J->PeerReqRID);

              MDB(7,fRM) MLog("INFO:     job '%s' ReqRID restored from peer as '%s'\n",
                J->Name,
                J->ReqRID);
              }

            MDB(2,fNAT)
              MJobShow(J,0,NULL);

            if (JSIndex == mjsRunning)
              {
              /* Note - this was added in 2005 with the note "p2p fixes". 
               * However, apparently there is a problem with calling both
               * MRMJobPostLoad (see above) and MRMJobPostUpdate for the same
               * job in the same iteration. One of the problems is that they
               * both call MQueueAddAJob() which complains if you attempt to
               * add a job that has already been added, which will be true
               * for this call since it was just added by MRMJobPostLoad() above. */ 

              MRMJobPostUpdate(J,TaskList,JSIndex,R);
              }

#ifdef __MDEBUG
          bmunset(&J->SpecFlags,mjfNoRMStart);
          bmunset(&J->Flags,mjfNoRMStart);
#endif /* __MDEBUG */
            }
          else
            {
            MDB(1,fNAT) MLog("ERROR:    could not create job (ignoring job '%s')\n",
              JobID);
            }
          }  /* END else (MJobFind(JobID,&J,NULL,mjsmExtended) == SUCCESS) */

        if (J != NULL)
          {
          /* MParUpdate(&MPar[0],FALSE,FALSE); */ /* don't update stats for now */
          }

        break;

      case mjsRemoved:
      case mjsCompleted:
      case mjsVacated:

        if (MJobCFind(JobID,NULL,mjsmExtended) == SUCCESS)
          {
          /* already completed, don't do anything */

          break;
          }         
        else if (MJobFind(JobID,&J,mjsmExtended) == SUCCESS)
          {
          /* load in completed job information */

          if (J->DestinationRM != R)
           {
           /* we can't believe what this RM says--it doesn't "own" the job */

            MDB(3,fRM) MLog("WARNING:  job '%s' is being reported by RM '%s' but is owned by RM '%s'!\n",
              J->Name,
              R->Name,
              (J->DestinationRM != NULL) ? J->DestinationRM->Name : "NULL");

           continue;
           }

          MRMJobPreUpdate(J);
 
          if (MMJobUpdate(JE,J,TaskList,R) == FAILURE)
            {
            MDB(1,fRM) MLog("ALERT:    cannot update job '%s'\n",
              JobID);

            continue;
            }

          MRMJobPostUpdate(J,TaskList,JSIndex,R);
          }
        else if (MJobCreate(JobID,TRUE,&J,NULL) == SUCCESS)
          {
          /* temporarily add job, then mark it as completed */

          MRMJobPreLoad(J,JobID,R);

          MMJobLoad(JobID,JE,J,TaskList,R);

          if (J->DRMJID != NULL)
            {
            /* job is being reported first by this peer--make it the DRM */

            MJobSetAttr(J,mjaDRM,(void **)R,mdfOther,mSet);
            }

          MRMJobPostLoad(J,TaskList,R,NULL);

          /* Restore the ReqRID since it may have been lost due to a moab reboot */

          if ((J->PeerReqRID != NULL) && (J->ReqRID != NULL) && (strcmp(J->ReqRID,"ALL") == 0))
            {
            MUStrDup(&J->ReqRID,J->PeerReqRID);

            MDB(7,fRM) MLog("INFO:     completed job '%s' ReqRID restored from peer as '%s'\n",
              J->Name,
              J->ReqRID);
            }
          }
        else
          {
          /* could not load job */

          MDB(6,fRM) MLog("ERROR:   could not load job '%s' (state: %s)!\n",
            JobID,
            MJobState[JSIndex]);

          break;
          }

        if ((J->State != mjsRunning) && (J->State != mjsStarting))
          {
          if (JSIndex == mjsCompleted)
            {
            /* job ran and finished within one iteration */

            MDB(1,fRM) MLog("INFO:     job '%s' appears to have been started and completed within one iteration\n",
              J->Name);
            }
          else
            {
            /* assume job was canceled externally - remove record */

            MDB(1,fRM) MLog("INFO:     job '%s' appears to have been canceled externally\n",
              J->Name);
            }
          }

        /* remove checkpoint files */

        if (J->SubmitRM != NULL)
          {
          if ((bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM == NULL)) ||
               ((bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM != NULL) &&
                 (J->DestinationRM->Type == mrmtMoab))) ||
               ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->IFlags,mrmifLocalQueue))) ||
               ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->Flags,mrmfFullCP))))
            {
            MJobRemoveCP(J);
            }
          }

        /* stage-data from destination peer (if configured to do so) */

        if (MSDProcessStageOutReqs(J) == FAILURE)
          {
          MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"data stage-out failed");

          break;
          }

        switch (JSIndex)
          {
          case mjsRemoved:
          case mjsVacated:

            MJobSetState(J,JSIndex);

            if (MSched.Time < (J->StartTime + J->WCLimit))
              {
              MJobProcessRemoved(&J);
              }
            else
              {
              char TString[MMAX_LINE];

              MULToTString(J->WCLimit,TString);

              sprintf(Message,"JOBWCVIOLATION:  job '%s' exceeded WC limit %s\n",
                J->Name,
                TString);

              MSysRegEvent(Message,mactNONE,1);

              MDB(3,fRM) MLog("INFO:     job '%s' exceeded wallclock limit %s\n",
                J->Name,
                TString);

              MJobProcessRemoved(&J);
              }

            break;

          case mjsCompleted:

            MJobSetState(J,JSIndex);

            MJobProcessCompleted(&J);

            break;

          default:

            /* unexpected job state */

            MDB(1,fRM) MLog("WARNING:  unexpected job state (%d) detected for job '%s'\n",
              JSIndex,
              J->Name);

            break;
          }    /* END switch (JSIndex) */

          break;

        default:
  
          MDB(1,fRM) MLog("WARNING:  job '%s' detected with unexpected state '%d'\n",
            JobID,
            JSIndex);
  
          break;
      }  /* END switch (JSIndex) */

    MUFree((char **)&TaskList);
  
    if (J != NULL)
      {
      /* check for errors on this job */

      if (MXMLGetAttr(JE,(char *)MJobAttr[mjaBlockReason],NULL,tmpBuf,sizeof(tmpBuf) == SUCCESS))
        {
        int eindex;
        char tmpLine[MMAX_LINE];

        eindex = MUGetIndex(tmpBuf,MJobBlockReason,TRUE,0);
        
        switch (eindex)
          {
          case mjneBadUser:
          case mjneNoResource:

          default:

            {
            mbool_t DoCancel = TRUE;
            
            if (strstr(tmpBuf,"timed out") != NULL)  /* sync text with that found in MUSpawnChild() */
              {
              /* FORMAT: child process (%s) timed out and was killed */

              /* do NOT cancel job on a time out */

              DoCancel = FALSE;
              }

            /* NOTE: it is difficult to specify a finer level of granularity because MRMJobSubmit
             * returns only EMsg - not machine readable codes */

            snprintf(tmpLine,sizeof(tmpLine),"job cannot be started on RM '%s' - %s",
              R->Name,
              MJobBlockReason[eindex]);

            MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

            snprintf(tmpLine,sizeof(tmpLine),"job cannot be started locally - %s",
              MJobBlockReason[eindex]);

            if (!bmisset(&J->Flags,mjfClusterLock) &&
                (DoCancel == TRUE))
              {
              mrm_t *InternalRM;

              /* NOTE: in the future we may want to just use MJobUnMigrate for the below code */

              /* remove from remote peer */

              MMJobCancel(J,J->DestinationRM,tmpLine,NULL,NULL);

              /* send back to idle state */
              /* NOTE: MJobToIdle clears J->DRM and resets partition lists */

              MJobToIdle(J,TRUE,TRUE);

              MRMGetInternal(&InternalRM);

              /* job must be added to local queue on internal RM or it will be marked as stale
               * and removed */

              if (MS3AddLocalJob(InternalRM,J->Name) == FAILURE)
                {
                return(FAILURE);
                }
                 
              MDB(7,fRM) MLog("INFO:     job '%s' has a block reason of %s\n",
                J->Name,
                MJobBlockReason[eindex]);

              if ((eindex == mjneRMSubmitFailure) && (MSched.RemoteFailTransient == FALSE))
                {
                /* note that an RM Submit failure on the slave may not be transient but we will assume that it
                   is so that we do not end up with jobs in batch hold on the master with an empty partition access list */

                /* eliminate partition from scheduling */
  
                MJobDisableParAccess(J,&MPar[R->PtIndex]);
                }
              }

            break;
            }  /* END BLOCK */
          }  /* END switch (eindex) */
        }    /* END if (MXMLGetAttr == SUCCESS) */
      }      /* END if (J != NULL) */
    }        /* END while (MXMLGetChild(DE,MXO[mxoNode],&CTok,&NE) == SUCCESS) */

  R->WorkloadUpdateIteration = MSched.Iteration;

  MDB(7,fRM) MLog("INFO:     %s WorkloadUpdateIteration set to Current Iteration %d, UM WIteration: %d)\n",
    R->Name,
    R->WorkloadUpdateIteration, 
    R->U.Moab.WIteration);

  /* update RM struct to reflect workload amounts */

  R->JobCount = *WCount;
  R->JobNewCount = *NWCount;

  MULLCreate(&JobsComletedList);
  MULLCreate(&JobsRemovedList);

  /* if job has disappeared, complete job */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (((J->SubmitRM != NULL) && (J->SubmitRM->Index != R->Index)) &&
       ((J->DestinationRM == NULL) || (J->DestinationRM->Index != R->Index)))
      continue;

    if ((MSched.JobPurgeTime == 0) && 
        ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtMoab) && (J->DestinationRM->NC == 0) && (J->DestinationRM->TC == 0)))
      {
      /* If a job purge time is not specified in the config file then do not purge jobs that are on a slave moab that
         is reporting no resources (e.g. SLURM on the slave is down). We do not purge jobs if the slave moab
         is down so we want the same behavior if the slave moab is up but the RM on the slave is down. */
      continue;
      }

    if ((MSched.Time - J->SubmitTime) < MCONST_MINUTELEN)
      {
      /* don't purge job until after at least 60 seconds from submit time */

      PurgeTime = MAX(MCONST_MINUTELEN,MSched.JobPurgeTime);
      }
    else
      {
      PurgeTime = MAX(MAX(15,(mulong)MSched.MaxRMPollInterval),MSched.JobPurgeTime);
      }

    /* can cause job to be removed but then rediscovered if remote peer is slow
     * to delete job...
      (!bmisset(&J->IFlags,mjifIsExiting) &&
        !bmisset(&J->IFlags,mjifWasCanceled) && */

    if ((J->ATime <= 0) ||
        ((MSched.Time - J->ATime) <= PurgeTime))
      {
      continue;
      }

    if (MJOBISACTIVE(J) == TRUE)
      {
      MDB(1,fRM) MLog("INFO:     active peer job %s has been removed from the queue.  assuming successful completion\n",
        J->Name);

      MRMJobPreUpdate(J);

      /* remove job from MAQ */

      /* NYI */

      /* assume job completed successfully for now */

      JSIndex           = J->State;
      J->State          = mjsCompleted;
      J->CompletionTime = J->ATime;

      MRMJobPostUpdate(J,NULL,JSIndex,J->SubmitRM);
      MJobSetState(J,mjsCompleted);

      /* stage-data from destination peer (if configured to do so) */

      if (MSDProcessStageOutReqs(J) == FAILURE)
        {
        MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"data stage-out failed");

        continue;
        }

      /* add job to completed list to be removed after exiting this loop */

      MULLAdd(&JobsComletedList,J->Name,(void *)J,NULL,NULL);
      }
    else
      {
      MDB(1,fRM) MLog("INFO:     non-active peer job %s has been removed from the queue.  assuming job was canceled\n",
        J->Name);

      /* just remove job */

      if (MSDProcessStageOutReqs(J) == FAILURE)
        {
        MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"data stage-out failed");

        continue;
        }

      /* add job to removed list to be removed after exiting this loop */

      MULLAdd(&JobsRemovedList,J->Name,(void *)J,NULL,NULL);
      }    /* END if (MJOBISACTIVE(J) == TRUE) */
    }      /* END for (J) */

  /* delete completed jobs */

  while (MULLIterate(JobsComletedList,&JobListPtr) == SUCCESS)
    MJobProcessCompleted((mjob_t **)&JobListPtr->Ptr);

  /* delete removed jobs */

  while (MULLIterate(JobsRemovedList,&JobListPtr) == SUCCESS)
    MJobProcessRemoved((mjob_t **)&JobListPtr->Ptr);

  if (JobsComletedList != NULL)
    MULLFree(&JobsComletedList,NULL);

  if (JobsRemovedList != NULL)
    MULLFree(&JobsRemovedList,NULL);

  /* mark the workload data as being processed so we don't go through it again */

  R->U.Moab.WDataIsProcessed = TRUE;

  return(SUCCESS);
  }  /* END __MMWorkloadQueryPostLoad() */



/**
 *
 *
 * @param origList (I) [modified]
 * @param newList (O)
 * @param newListSize (O)
 */

int __MMConvertNLToPeers(
  
  char *origList,    /* I (modified) */
  char *newList,     /* O */
  int   newListSize) /* O */

  {
  char *TokPtr1 = NULL;
  char *TokPtr2 = NULL;
  char *Pair;
  char *ptr;
  char  tmpName[MMAX_NAME];
  char  tmpTC[MMAX_NAME];
  int   rmindex;
  mrm_t *R;
  
  if ((origList == NULL) ||
      (newList == NULL))
    {
    return(FAILURE);
    }

  newList[0] = '\0';
  
  /* NodeList FORMAT: <ClusterName>[:<TaskCount>][,...] */

  Pair = MUStrTok(origList,",",&TokPtr1);

  while (Pair != NULL)
    {
    ptr = MUStrTok(Pair,":",&TokPtr2); /* extract <ClusterName> */

    MUStrCpy(tmpName,ptr,sizeof(tmpName));

    ptr = MUStrTok(NULL,":",&TokPtr2); /* extract <TaskCount> */

    if (ptr != NULL)
      {
      MUStrCpy(tmpTC,ptr,sizeof(tmpTC));
      }
    else
      {
      tmpTC[0] = '\0';
      }    

    /* search for corresponding peer */
    
    for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
      {
      R = &MRM[rmindex];

      if (R->Name[0] == '\0')
        break;
      
      if (MRMIsReal(R) == FALSE)
        continue;

      if (R->Type != mrmtMoab)
        continue;

      if (!strcmp(tmpName,R->ClientName) && (tmpTC[0] != '\0'))
        {
        /* add the "masked" grid name for this node */
        /* NOTE: needs changing to work with hop count groups */

        if (newList[0] != '\0')
          {
          MUStrCat(newList,",",newListSize);
          }
        
        MUStrCat(newList,R->Name,newListSize);
        MUStrCat(newList,":",newListSize);
        MUStrCat(newList,tmpTC,newListSize);

        break;
        }
      }  /* END for (rmindex) */

    Pair = MUStrTok(NULL,",",&TokPtr1);
    }    /* END while (Pair != NULL) */

  return(SUCCESS);
  }  /* END __MMConvertNLToPeers() */




/**
 * Wrapper to __MMJobSubmitPostLoad as the job could have been deleted before
 * the peer moab reports back.
 *
 * @param S (I)
 * @param R (I) [mrm_t *]
 * @param JobName (I) (optional/modified) [char *] strdup'ed' before being push on to transactions. Must FREE.
 * @param Arg3 () N/A
 * @param Arg4 () N/A
 */

int __MMJobSubmitPostLoadByJobName(

  msocket_t *S,        /* I */
  void      *R,        /* I (mrm_t *)  */
  void      *JobName,  /* I (char *) (alloc) */
  void      *Arg3,     /* N/A */
  void      *Arg4)     /* N/A */

  {
  mjob_t *tmpJ = NULL;

  if ((S == NULL) ||
      (R == NULL) ||
      (JobName == NULL))
    {
    if (JobName != NULL)
      MUFree((char **)&JobName);

    return(FAILURE);
    }

  if (MJobFind((char *)JobName,&tmpJ,mjsmExtended) == SUCCESS)
    __MMJobSubmitPostLoad(S,(mrm_t *)R,tmpJ,NULL,NULL);

  MUFree((char **)&JobName);

  return(SUCCESS);
  } /* END int __MMJobSubmitPostLoadByJobName() */




/**
 * Finishes off the submission of a job to a remote Moab peer. Handles
 * any error messages that may have returned, attaches those messages to
 * socket, etc.
 *
 * @param S (I)
 * @param DPtr (I) [mrm_t *]
 * @param Arg2 (I) (optional/modified) [mjob_t *]
 * @param Arg3 () N/A
 * @param Arg4 () N/A
 */

int __MMJobSubmitPostLoad(

  msocket_t *S,        /* I */
  void      *DPtr,     /* I (mrm_t *)  */
  void      *Arg2,     /* I (mjob_t *) (optional/modified) */
  void      *Arg3,     /* N/A */
  void      *Arg4)     /* N/A */

  {
  mjob_t *J;
  mrm_t  *R;
  mbool_t BlockPartition = FALSE;
  char BMsg[MMAX_LINE];

  if ((S == NULL) ||
      (DPtr == NULL) ||
      (Arg2 == NULL))
    {
    return(FAILURE);
    }
  
  BMsg[0] = '\0';

  R = (mrm_t *)DPtr;
  J = (mjob_t *)Arg2;

  /* check for errors that have happened */

  if (S->StatusCode <= msfGWarning)
    {
    /* success */

    /* NO-OP */
    }
  else
    {
    switch (S->StatusCode)
      {
      /* job probably does not exist on remote system in these cases */

      /* permanent failure - do not retry */

      case msfEBadRequestor:

        BlockPartition = TRUE;
        
        snprintf(BMsg,sizeof(BMsg),"scheduler not authorized to stage job to RM %s",
          R->Name);

        break;

      case msfEGServerBus:

        BlockPartition = TRUE;

        snprintf(BMsg,sizeof(BMsg),"cannot translate job for use on RM %s",
          R->Name);

        break;

      case msfEGSMapping:

        BlockPartition = TRUE;

        snprintf(BMsg,sizeof(BMsg),"invalid user or file mapping for RM %s",
          R->Name);

        break;

      case msfEGSInternal:

        BlockPartition = TRUE;

        snprintf(BMsg,sizeof(BMsg),"general failure migrating job to RM %s",
          R->Name);

        break;

      /* transient failures - retry */

      default:

        {
        mxml_t *EE;

        EE = (mxml_t *)S->RDE;

        if ((EE != NULL) &&
            !strcmp(EE->Name,MSON[msonData]))
          {
          snprintf(BMsg,sizeof(BMsg),"message from %s: %s",
            R->Name,
            EE->Val);
          }
        }  /* END BLOCK */
        
        break;
      }  /* END switch (S->StatusCode) */
    }    /* END else (S->StatusCode <= msfGWarning) */

  /* attach any error messages */

  if (BMsg[0] != '\0')
    {
    MMBAdd(&J->MessageBuffer,BMsg,NULL,mmbtNONE,0,0,NULL);
    }

  if (BlockPartition == TRUE)
    {
    /* send back to idle state */
    /* NOTE: MJobToIdle clears DRM and resets partition lists */

    MJobToIdle(J,TRUE,TRUE);

    MJobDisableParAccess(J,&MPar[R->PtIndex]);
    }

  return(SUCCESS);
  }  /* END __MMJobSubmitPostLoad() */





/**
 * Extract cluster data from fully loaded socket.
 *
 * @param S (I)
 * @param DPtr (I) [mrm_t *]
 * @param Arg2 () N/A
 * @param Arg3 () N/A
 * @param Arg4 () N/A
 */

int __MMClusterQueryGetData(

  msocket_t *S,        /* I */
  void      *DPtr,     /* I (mrm_t *) */
  void      *Arg2,     /* N/A */
  void      *Arg3,     /* N/A */
  void      *Arg4)     /* N/A */

  {
  mrm_t *R;

  if ((S == NULL) || (DPtr == NULL))
    {
    return(FAILURE);
    }

  R = (mrm_t *)DPtr;

  /* free current CData */
  
  if (R->U.Moab.CData != NULL)
    {
    MXMLDestroyE((mxml_t **)&R->U.Moab.CData);

#ifndef __MMSPLITREQUEST
    R->U.Moab.WData = NULL;
#endif /* __MMSPLITREQUEST */
    }

  if (S->StatusCode != 0)
    {
    int tmpI;

    /* failure occured */

    MDB(3,fRM) MLog("ALERT:    cannot load cluster resources on RM (RM '%s' failed in non-blocking response function)\n",
      R->Name);

    MRMSetFailure(
      R,
      mrmClusterQuery,
      mscSysFailure,
      "cannot get node info from non-blocking response"); 

    MUGetRC(NULL,S->StatusCode,&tmpI,mrclCentury);

    switch (tmpI)
      {
      case 100:
 
        /* general warning - ignore */

        /* NO-OP */

        break;

      default:

        /* probable cause if client was restarted and needs to be re-initialized */

        MDB(3,fRM) MLog("INFO:     remote peer %s reports that interface must be re-initialized\n",
          R->Name);

        MRMSetState(R,mrmsCorrupt);

        break;
      }  /* END switch (S->StatusCode) */

    return(FAILURE);
    }

  R->UTime = MSched.Time;

  R->U.Moab.CData = (void *)S->RDE;
  R->U.Moab.CMTime = MSched.Time;
  R->U.Moab.CIteration = MSched.Iteration;
  R->U.Moab.CDataIsProcessed = FALSE;

#ifndef __MMSPLITREQUESTS
  /* combined query, share response */

  R->U.Moab.WData = (void *)S->RDE;
  R->U.Moab.WMTime = MSched.Time;
  R->U.Moab.WIteration = MSched.Iteration;
  R->U.Moab.WDataIsProcessed = FALSE;

  MDB(4,fNAT) MLog("INFO:     RM %s UM WIteration set to %d \n",
    R->Name,
    MSched.Iteration);

#endif /* __MMSPLITREQUESTS */

  S->RDE = NULL;

  return(SUCCESS);  
  }  /* END __MMClusterQueryGetData() */





/**
 *
 *
 * @param S (I)
 * @param DPtr (I) [mrm_t *]
 * @param Arg2 () N/A
 * @param Arg3 () N/A
 * @param Arg4 () N/A
 */

int __MMWorkloadQueryGetData(

  msocket_t *S,        /* I */
  void      *DPtr,     /* I (mrm_t *) */
  void      *Arg2,     /* N/A */
  void      *Arg3,     /* N/A */
  void      *Arg4)     /* N/A */

  {
  mrm_t *R;

  if ((S == NULL) || (DPtr == NULL))
    {
    return(FAILURE);
    }

#ifndef __MMSPLITREQUESTS
  /* all actions handled by __MMClusterQueryGetData() */

  /* NO-OP */

  return(SUCCESS);
#endif /* __MMSPLITREQUESTS */

  R = (mrm_t *)DPtr;

  /* free current WData */
  
  if (R->U.Moab.WData != NULL)
    {
    MXMLDestroyE((mxml_t **)&R->U.Moab.WData);

    R->U.Moab.WData = NULL;
    }

  if (S->StatusCode != 0)
    {
    /* failure occured */

    MDB(3,fRM) MLog("ALERT:    cannot load cluster workload on RM (RM '%s' failed in non-blocking response function)\n",
      R->Name);

    MRMSetFailure(R,mrmWorkloadQuery,mscSysFailure,"cannot get job info from non-blocking response"); 

    return(FAILURE);
    }

  R->UTime = MSched.Time;

  R->U.Moab.WData = (void *)S->RDE;
  R->U.Moab.WMTime = MSched.Time;
  R->U.Moab.WIteration = MSched.Iteration;
  R->U.Moab.WDataIsProcessed = FALSE;

  MDB(4,fNAT) MLog("INFO:     RM %s UM WIteration set to %d \n",
    R->Name,
    MSched.Iteration);
 
  S->RDE = NULL;

  return(SUCCESS);  
  }  /* END __MMWorkloadQueryGetData() */





/**
 * Report TRUE if cluster or workload queries are outstanding.
 *
 * @param R (I) [optional]
 */

mbool_t MMPeerRequestsArePending(

  mrm_t *R) /* I (optional) */

  {
  /* PURPOSE:  we want to only send one pending cluster or workload query per
               peer */

  if ((MUITransactionFindType(__MMClusterQueryGetData,R,NULL) == SUCCESS) ||
      (MUITransactionFindType(__MMWorkloadQueryGetData,R,NULL) == SUCCESS))
    {
    return(TRUE);
    }

  return(FALSE);
  }  /* END MMPeerRequestsArePending() */





/**
 *
 *
 * @param C (I)
 * @param R (I)
 * @param AIndex (I)
 * @param SubAttr (I) [optional]
 * @param AVal (I)
 * @param EMsg (O) [optional]
 * @param SC (O) [optional]
 */

int MMQueueModify(

  mclass_t *C,                /* I */
  mrm_t    *R,                /* I */
  enum MClassAttrEnum AIndex, /* I */
  char     *SubAttr,          /* I (optional) */
  void     *AVal,             /* I */
  char     *EMsg,             /* O (optional) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  /* NYI */

  return(SUCCESS);
  }  /* END MMQueueModify() */




/**
 *
 *
 * @param J         (I)
 * @param R         (I)
 * @param RAQMethod (I)
 * @param REP       (O) [alloc]
 * @param EMsg      (O) [optional]
 */

int MMJobQueryCreateRequest(

  mjob_t  *J,
  mrm_t   *R,
  enum MResAvailQueryMethodEnum RAQMethod,
  mxml_t **REP,
  char    *EMsg)

  {
  mxml_t *RE;
  mxml_t *JE;
  mxml_t *WE;

  mbitmap_t Flags;

  char tmpLine[MMAX_LINE];
  
  if ((J == NULL) || (R == NULL) || (REP == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  *REP = NULL;

  RE = NULL;   

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);

  MS3SetObject(RE,(char *)MXO[mxoSched],NULL);

  bmset(&Flags,mcmXML);

  bmtostring(&Flags,MClientMode,tmpLine);

  MXMLSetAttr(RE,MSAN[msanFlags],(void *)&tmpLine,mdfString);

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MSchedCtlCmds[msctlQuery],
    mdfString);

  MXMLSetAttr(
    RE,
    MSAN[msanArgs],
    (void *)MXO[mxoJob],
    mdfString);

  MXMLSetAttr(
    RE,
    MSAN[msanOp],
    (void *)"starttime",
    mdfString);            

  if (MS3JobToXML(
        J,
        &JE,
        R,
        TRUE,
        0,    /* FlagBM */
        NULL,
        NULL,
        NULL,
        EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MS3AddWhere(RE,(char *)MXO[mxoJob],"",&WE) == FAILURE)
    {
    return(FAILURE);
    }

  MXMLSetAttr(WE,(char *)MSAN[msanFlags],(char *)MResAvailQueryMethod[RAQMethod],mdfString);

  MXMLAddE(WE,JE);
    
  *REP = RE;

  return(SUCCESS);
  }  /* END MMJobQueryCreateRequest() */




/** 
 * Provides forward/reverse job attribute mapping.
 *
 * @param JE (I) [modified]
 * @param R (I)
 */

int MMJobOMap(

  mxml_t *JE, /* I (modified) */
  mrm_t  *R)  /* I */

  {
  if ((JE == NULL) ||
      (R == NULL))
    {
    return(FAILURE);
    }

  /* map allocated node list */

  mstring_t New(MMAX_BUFFER);
  mstring_t Old(MMAX_BUFFER);

  if (MXMLGetAttrMString(JE,(char *)MJobAttr[mjaAllocNodeList],NULL,&Old) == SUCCESS)
    {
    /* enable reverse look-up based on R->Flags==mrmfClient? */

    char *mutableString = NULL;
    MUStrDup(&mutableString,Old.c_str());  /* must make a mutable string from the mstring */

    if (MOMapList(R->OMap,mxoNode,mutableString,TRUE,FALSE,&New) == FAILURE)
      {
      MDB(3,fNAT) MLog("INFO:     cannot map job's allocated node list");

      MUFree(&mutableString);
      return(FAILURE);
      }


    MXMLSetAttr(JE,(char *)MJobAttr[mjaAllocNodeList],(void *)New.c_str(),mdfString);

    Old = mutableString;

    MUFree(&mutableString);
    }

#ifdef __MNOT
  /* NOTE: this code is no longer used--reverse mapping is now done on a smaller scale
   * and is located in the MMJobLoad and MMJobUpdate functions */

//NOTE:  New won't work in the following code any more 

  /* map user name (reverse look-up) */

  if (MXMLGetAttr(JE,(char *)MJobAttr[mjaUser],NULL,Old,sizeof(Old)) == SUCCESS)
    {
    if (MOMap(R->OMap,mxoUser,Old,TRUE,FALSE,New) == FAILURE)
      {
      MDB(3,fNAT) MLog("INFO:     cannot map job's user information");
      
      return(FAILURE);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void *)New,mdfString);
    }

  /* map class (reverse look-up) */

  if (MXMLGetAttr(JE,(char *)MJobAttr[mjaClass],NULL,Old,sizeof(Old)) == SUCCESS)
    {
    if (MOMap(R->OMap,mxoClass,Old,TRUE,FALSE,New) == FAILURE)
      {
      MDB(3,fNAT) MLog("INFO:     cannot map job's class information");
      
      return(FAILURE);
      }

    if (New[0] != '\0')
      MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void *)New,mdfString);

    /* also, look for req 'ReqClass' attribute */

    CTok = -1;

    while (MXMLGetChild(JE,(char *)MXO[mxoReq],&CTok,&RE) == SUCCESS)
      {
      if (MXMLGetAttr(RE,(char *)MReqAttr[mrqaReqClass],NULL,NULL,0) == SUCCESS)
        {
        MXMLSetAttr(RE,(char *)MReqAttr[mrqaReqClass],(void *)New,mdfString);
        }
      }
    }    /* END if (MXMLGetAttr(JE,(char *)MJobAttr[mjaClass],NULL,Old,sizeof(Old)) == SUCCESS) */
#endif /* __MNOT */

  return(SUCCESS);
  }  /* END MMJobOMap() */




/**
 * Map node attributes based on the given resource manager's configuration.
 *
 * @param NE
 * @param R
 */

int MMNodeOMap(

  mxml_t *NE,
  mrm_t  *R)

  {
  char Old[MMAX_LINE];
  char New[MMAX_LINE];

  if ((NE == NULL) ||
      (R == NULL))
    {
    return(FAILURE);
    }

  if (MXMLGetAttr(NE,(char *)MNodeAttr[mnaAvlClass],NULL,Old,sizeof(Old)) == SUCCESS)
    {
    if (MOMapClassList(R->OMap,Old,TRUE,FALSE,New,sizeof(New)) == FAILURE)
      {
      MDB(3,fNAT) MLog("INFO:     cannot map node's available class list");
      
      return(FAILURE);
      }

    MXMLSetAttr(NE,(char *)MNodeAttr[mnaAvlClass],(void *)New,mdfString);
    }

  if (MXMLGetAttr(NE,(char *)MNodeAttr[mnaCfgClass],NULL,Old,sizeof(Old)) == SUCCESS)
    {
    if (MOMapClassList(R->OMap,Old,TRUE,FALSE,New,sizeof(New)) == FAILURE)
      {
      MDB(3,fNAT) MLog("INFO:     cannot map node's configured class list");
      
      return(FAILURE);
      }

    MXMLSetAttr(NE,(char *)MNodeAttr[mnaCfgClass],(void *)New,mdfString);
    }

  return(SUCCESS);
  }  /* END MMNodeOMap() */



/**
 *
 *
 * @param R (I)
 * @param SC (I)
 */

int MMFailure(

  mrm_t *R,  /* I */
  enum MStatusCodeEnum SC)  /* I */

  {
  mnl_t     *NL;
  mnode_t   *N;

  int nindex;
  char tmpLine[MMAX_LINE];

  if (R == NULL)
    {
    return(FAILURE);
    }

  /* down all nodes associated with RM if failure has occured */

  NL = &R->NL;

  if (!MNLIsEmpty(NL))
    {
    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS; nindex++)
      {
      /* mark node as down */

      MNodeSetState(N,mnsDown,FALSE);

      sprintf(tmpLine,"node's resource manager is in state '%s'",
        MRMState[R->State]);

      MMBAdd(
        &N->MB,
        tmpLine,
        "N/A",
        mmbtOther,
        MCONST_DAYLEN,
        1,
        NULL);
      }  /* END for (nindex) */
    }    /* END if (NL != NULL) */

  return(SUCCESS);
  }  /* END MMQueueModify() */





/**
 * Sends a job modification request to a remote Moab peer.
 *
 * @see MRMJobModify() - parent
 *
 * @param J (I) [modified]
 * @param R (I)
 * @param Name (I)
 * @param Resource (I) [optional]
 * @param Val (I)
 * @param Op (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMJobModify(

  mjob_t               *J,
  mrm_t                *R,
  const char           *Name,
  const char           *Resource,
  const char           *Val,
  enum MObjectSetModeEnum Op,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  mxml_t *RE;
  mxml_t *SE;
  mxml_t *DE;
  
  char CmdLine[MMAX_LINE];
  char Response[MMAX_LINE];
  const char *NamePtr = NULL;

  const char *FName = "MMJobModify";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,%s,%s,%d,EMsg,SC)\n",
    FName,
    (J    != NULL) ? J->Name : "NULL",
    (R    != NULL) ? R->Name : "NULL",
    (Name != NULL) ? Name : "NULL",
    (Resource != NULL) ? Resource : "NULL",
    (Val != NULL) ? Val : "NULL",
    Op);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((J == NULL) || (R == NULL) || (Name == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  if (!strcasecmp(Name,"comment"))
    {
    /* NOTE:  do not pass comments down to peers in grid mode */

    /* NOTE:  This is done to minimize messages, peer blocking, etc */

    return(SUCCESS);
    }

  RE = NULL;
  SE = NULL;

  /* FORMAT: <Request action="modify" actor="{USER}" job="{DRMJID}">
   * <Object>job</Object><Set name="{Name}" op="set">{Val}</Set></Request> */

  /* create request XML */  

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);
                                                                                
  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MJobCtlCmds[mjcmModify],
    mdfString);

  MXMLSetAttr(RE,(char *)MXO[mxoJob],(void *)J->DRMJID,mdfString);

  MS3SetObject(RE,(char *)MXO[mxoJob],NULL);

  /* create set for attribute modification */

  MXMLCreateE(&SE,(char*)MSON[msonSet]);

  if (Op == mNONE2)
    {
    Op = mSet;
    }
  
  MXMLSetAttr(SE,MSAN[msanOp],(void *)MObjOpType[Op],mdfString);

  /* customize according to inputs */

  if (Name == NULL)
    {
    NamePtr = Resource;
    }
  else if (!strcasecmp(Name,"Resource_List") && (Resource != NULL))
    {
    NamePtr = Resource;
    }
  else
    {
    NamePtr = Name;
    }

  if (NamePtr == NULL)
    {
    return(FAILURE);
    }

  /* translate to MUI-compatible values */

  if (!strcasecmp(NamePtr,"queue"))
    {
    MXMLSetAttr(SE,MSAN[msanName],(char *)"class",mdfString);
    }
  else if (!strcasecmp(NamePtr,"walltime"))
    {
    MXMLSetAttr(SE,MSAN[msanName],(char *)"wclimit",mdfString);
    }
  else
    {
    MXMLSetAttr(SE,MSAN[msanName],NamePtr,mdfString);
    }

  MXMLSetVal(SE,Val,mdfString);

  MXMLAddE(RE,SE);

  MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE);

  MXMLDestroyE(&RE);

  R->P[R->ActingMaster].IsNonBlocking = TRUE;  /* FIXME: support non-blocking modify calls */

  if (__MMDoCommand(
       R,
       &R->P[R->ActingMaster],
       mcsMJobCtl,
       CmdLine,
       Response,
       sizeof(Response),
       EMsg,
       SC) == FAILURE)
    {
    /* cannot modify job */

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute modify command");

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  DE = (mxml_t *)R->P[R->ActingMaster].S->RDE;

  if ((DE->Val != NULL) &&
      !strcmp(DE->Val,"PENDING"))
    {
    /* non-blocking response */

    /* NYI */
    }

  if (strstr(Response,"ERROR"))
    {
    /* error occured */
    
    MDB(1,fNAT) MLog("ERROR:    cannot modify job: '%s'\n",
      Response);

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  MSUFree(R->P[R->ActingMaster].S);

  /* don't update job information, wait for info from RM */

  return(SUCCESS);
  }  /* END MMJobModify() */


/**
 *
 *
 * @param J (I)
 * @param R (I)
 * @param JPList (I) [not used]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MMJobRequeue(

  mjob_t   *J,      /* I */
  mrm_t    *R,      /* I */
  mjob_t  **JPList, /* I (not used) */
  char     *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  int      *SC)     /* O (optional) */

  {
  char CmdLine[MMAX_LINE];
  char Response[MMAX_LINE];
  enum MStatusCodeEnum tmpSC;

  mxml_t *RE;  
  mxml_t *DE;

  const char *FName = "MMJobRequeue";

  MDB(1,fNAT) MLog("%s(%s,%s,JPList,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((J == NULL) || (R == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  /* <Request action="requeue" actor="josh" job="poli.8"><Object>job</Object></Request> */

  /* create request XML */

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MJobCtlCmds[mjcmRequeue],
    mdfString);

  MXMLSetAttr(RE,(char *)MXO[mxoJob],(void *)J->DRMJID,mdfString);

  MS3SetObject(RE,(char *)MXO[mxoJob],NULL);

  MXMLToString(RE,CmdLine,sizeof(CmdLine),NULL,TRUE);

  MXMLDestroyE(&RE);

  tmpSC = mscNoError;

  R->P[R->ActingMaster].IsNonBlocking = FALSE;  /* FIXME: support non-blocking signal calls */
    
  if (__MMDoCommand(
       R,
       &R->P[R->ActingMaster],
       mcsMJobCtl,
       CmdLine,
       Response,
       sizeof(Response),
       EMsg,
       &tmpSC) == FAILURE)
    {
    /* cannot signal job */
  
    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute requeue command");
  
    if (SC != NULL)
      *SC = tmpSC;

    MSUFree(R->P[R->ActingMaster].S);
    
    return(FAILURE);
    }
    
  DE = (mxml_t *)R->P[R->ActingMaster].S->RDE;
  
  if (SC != NULL)
    *SC = tmpSC;

  if ((DE->Val != NULL) &&
      !strcmp(DE->Val,"PENDING"))
    {
    /* non-blocking response */
  
    /* NYI */
    }
    
  if (strstr(Response,"ERROR"))
    {
    /* error occured */

    MDB(1,fNAT) MLog("ERROR:    cannot requeue job: '%s'\n",
      Response);

    MSUFree(R->P[R->ActingMaster].S);

    return(FAILURE);
    }

  MSUFree(R->P[R->ActingMaster].S);

  return(SUCCESS);
  }  /* END MMJobRequeue() */
  
  
  
  
 /**
  * Used to copy files between two Moab peers. For now, files can
  * only be copied to remote directories within Moab's home directory.
  * Note that this function isn't really designed to copy large files.
  * Another solution should be used for such instances.
  *
  * @param File (I)
  * @param DstR (I) [optional]
  * @param PPtr (I) [optional if DstR specific]
  * @param EMsg (O) [optional,minsize=MMAX_LINE]
  * @param SC   (O) [optional]
  */

int MMPeerCopy(

  char   *File,
  mrm_t  *DstR,
  mpsi_t *PPtr,
  char   *EMsg,
  int    *SC)
  
  {  
  mxml_t *RE;
  mxml_t *FE;
  mxml_t *DE;
  
  char   *CmdBuf = NULL;
  int     CmdBufSize = MMAX_BUFFER;
  int     MaxCmdBufSize = MMAX_BUFFER << 2;
  
  char    Response[MMAX_LINE];
  enum MStatusCodeEnum tmpSC;
  
  char   *tmpBuffer = NULL;
  int     tmpBufSize = 0;

  mpsi_t *P;

  const char *FName = "MMPeerCopy";

  MDB(8,fNAT) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (File != NULL) ? File : "NULL",
    (DstR != NULL) ? DstR->Name : "NULL");    
  
  if ((File == NULL) ||
      ((DstR == NULL) && (PPtr == NULL)))
    {
    return(FAILURE);
    }
    
  if (EMsg != NULL)
    {
    EMsg[0] = '\0';
    }

  /* load file into buffer */

  if (MUPackFileNoCache(File,&tmpBuffer,&tmpBufSize,EMsg) == FAILURE)
    {
    return(FAILURE);
    }
    
  /* <Request action="filecopy" actor="josh">
   *   <Object>sched</Object>
   *   <FileData file="spool/job1.cp">...</FileData>
   * </Request> */

  /* create request XML */ 

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);
  
  MS3SetObject(RE,(char *)MXO[mxoSched],NULL);  

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MSchedCtlCmds[msctlCopyFile],
    mdfString);

  MXMLCreateE(&FE,"FileData");
  
  MXMLSetAttr(FE,(char *)"file",(void *)File,mdfString);

  MXMLSetVal(FE,(void *)tmpBuffer,mdfString);

  MUFree(&tmpBuffer);
  
  MXMLAddE(RE,FE);
  
  CmdBuf = (char *)MUCalloc(1,CmdBufSize * sizeof(char));

  MXMLToXString(RE,&CmdBuf,&CmdBufSize,MaxCmdBufSize,NULL,TRUE);

  MXMLDestroyE(&RE);
  
  tmpSC = mscNoError;

  if (DstR != NULL)
    {
    P = &DstR->P[DstR->ActingMaster];
    }
  else
    {
    /* load peer info and secret key */

    P = PPtr;
    }

  P->IsNonBlocking = FALSE;  /* FIXME: support non-blocking signal calls */
    
  if (__MMDoCommand(
       DstR,
       P,
       mcsMSchedCtl,
       CmdBuf,
       Response,
       sizeof(Response),
       EMsg,
       &tmpSC) == FAILURE)
    {
    /* cannot copy file */
  
    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute file copy command");
  
    if (SC != NULL)
      *SC = tmpSC;

    MUFree(&CmdBuf);
    MSUFree(P->S);
    
    return(FAILURE);
    }
    
  DE = (mxml_t *)P->S->RDE;
  
  if (SC != NULL)
    *SC = tmpSC;

  if ((DE->Val != NULL) &&
      !strcmp(DE->Val,"PENDING"))
    {
    /* non-blocking response */
  
    /* NYI */
    }
    
  if (strstr(Response,"ERROR"))
    {
    /* error occured */

    MDB(1,fNAT) MLog("ERROR:    cannot copy file: '%s'\n",
      Response);

    MUFree(&CmdBuf);
    MSUFree(P->S);

    return(FAILURE);
    }

  MUFree(&CmdBuf);
  MSUFree(P->S);    
    
  return(SUCCESS);
  }  /* END MMPeerCopy() */




 /**
  * Use to register events in preparation for sending them to Moab peer who are "listening." Note that
  * for now, only SLAVE Moabs can have listening peers, and only peers that have the CLIENT flag set 
  * are considered listening. In the future this definition may change. For now events carry no payload
  * and are sent immediately. We plan on allowing events to be queued up so they can be sent more efficiently
  * in batches. A threaded model, or a specific RM list, could be used to prevent events from going to all
  * listeners.
  *
  * @param EventType (I) The event to send the listening Moab peers.
  */

int MMRegisterEvent(

  enum MPeerEventTypeEnum EventType)  /* I */

  {
  mxml_t *RE;
  mxml_t *DE;
  
  char CmdBuf[MMAX_LINE];
  char tEMsg[MMAX_LINE];
  
  char Response[MMAX_LINE];
  enum MStatusCodeEnum tmpSC;
  
  int     rmindex;

  mpsi_t *P;
  mrm_t  *R;

  const char *FName = "MMRegisterEvent";

  MDB(1,fNAT) MLog("%s(%s)\n",
    FName,
    MPeerEventType[EventType]);

  /* for now, only send events if we are in SLAVE mode (we are just keeping it simple
   * for the first pass at the event system implementation) */

  if (MSched.Mode != msmSlave)
    {
    return(SUCCESS);
    }
  
  /* <Request action="event" actor="josh" eventtype="reconfig">
   *   <Object>sched</Object>
   * </Request> */

  /* create request XML */ 

  MXMLCreateE(&RE,(char*)MSON[msonRequest]);
  
  MS3SetObject(RE,(char *)MXO[mxoSched],NULL);  

  MXMLSetAttr(
    RE,
    MSAN[msanAction],
    (void *)MSchedCtlCmds[msctlEvent],
    mdfString);

  MXMLSetAttr(
    RE,
    "eventtype",
    (void *)MPeerEventType[EventType],
    mdfString);

  MXMLToString(RE,CmdBuf,sizeof(CmdBuf),NULL,TRUE);

  MXMLDestroyE(&RE);
  
  tmpSC = mscNoError;

  /* cycle through all "master" peers and send the event to them (this can be improved upon, of course) */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (!bmisset(&R->Flags,mrmfClient))
      continue;

    P = &R->P[R->ActingMaster];

    P->IsNonBlocking = FALSE;  /* FIXME: support non-blocking signal calls */
      
    if (__MMDoCommand(
         R,
         P,
         mcsMSchedCtl,
         CmdBuf,
         Response,
         sizeof(Response),
         tEMsg,
         &tmpSC) == FAILURE)
      {
      /* cannot send event */
    
      MDB(1,fNAT) MLog("ERROR:    cannot send event '%s' to RM '%s'\n",
        MPeerEventType[EventType],
        R->Name);

      MSUFree(P->S);

      continue;
      }
      
    DE = (mxml_t *)P->S->RDE;
    
    if ((DE->Val != NULL) &&
        !strcmp(DE->Val,"PENDING"))
      {
      /* non-blocking response */
    
      /* NYI */
      }
      
    if (strstr(Response,"ERROR"))
      {
      /* error occured */

      MDB(1,fNAT) MLog("ERROR:    cannot send event '%s' to RM '%s': '%s'\n",
        MPeerEventType[EventType],
        R->Name,
        Response);

      MSUFree(P->S);

      continue;      
      }

    MSUFree(P->S);    
    }  /* END for(rmindex) */

  return(SUCCESS);
  }  /* END MMRegisterEvent() */





/**
 * Process Rsv Import Query
 *
 * @param DE (I)
 * @param RM (I)
 * @param MakeStatic (I)
 *
 * @see __MMClusterQueryPostLoad() - parent
 * @see MRsvCreateFromXML() - child
 */

#define MREMOTERSV_PURGETIME 60

int MMRsvQueryPostLoad(

  mxml_t *DE,          /* I */
  mrm_t  *RM,          /* I */
  mbool_t MakeStatic)  /* I */

  {
  int CTok;

  mxml_t *RE;

  mrsv_t  *RP;
  mrrsv_t *RR;
  mrrsv_t *RPrev;
  mrrsv_t *RNext;

  char     EMsg[MMAX_LINE];

  char tmpName[MMAX_LINE];

  const char *FName = "MMRsvQueryPostLoad";

  MDB(1,fNAT) MLog("%s(DE,%s,%s)\n",
    FName,
    (RM != NULL) ? RM->Name : "NULL",
    MBool[MakeStatic]);

  if ((DE == NULL) || (RM == NULL))
    {
    return(FAILURE);
    }

  CTok = -1;

  while (MXMLGetChild(DE,(char *)MXO[mxoRsv],&CTok,&RE) == SUCCESS)
    {
    if (MXMLGetAttr(RE,(char *)MRsvAttr[mraGlobalID],NULL,tmpName,sizeof(tmpName)) == FAILURE)
      {
      /* rsv is corrupt */

      MDB(2,fNAT) MLog("WARNING:  corrupt rsv received from peer %s (ignoring rsv)\n",
        RM->Name);

      continue; 
      }

    if (MRsvFind(tmpName,&RP,mraGlobalID) == FAILURE)
      {
      if (MRsvCreateFromXML(NULL,tmpName,RE,&RP,RM,EMsg) == FAILURE)
        {
        char tmpLine[MMAX_LINE];

        /* rsv is corrupt */

        MDB(2,fNAT) MLog("WARNING:  corrupt rsv received from peer %s (ignoring rsv)\n",
          RM->Name);

        snprintf(tmpLine,sizeof(tmpLine),"cannot import rsv '%s' - %s",
          tmpName,
          EMsg);

        MMBAdd(&RM->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

        continue;
        }

      MOAddRemoteRsv((void *)RP,mxoRsv,RM,tmpName);

      if (MakeStatic == TRUE)
        bmset(&RP->Flags,mrfStatic);
      }
    else
      {
      /* update sync time */
  
      RR = RP->RemoteRsv;
   
      while (RR != NULL)
        {
        RR->LastSync = MSched.Time;

        RR = RR->Next;
        }

      RR = RM->RemoteR;

      while (RR != NULL)
        {
        if (RP != RR->LocalRsv)
          {
          RR = RR->Next;

          continue;
          }

        RR->LastSync = MSched.Time;

        RR = RR->Next;
        }

      /* NYI */
      }
    }  /* END while (MXMLGetChild(DE,(char *)MXO[mxoRsv],&CTok,&RE) == SUCCESS) */

  /* process all reservations from RM to see if any should be deleted */

  RR = RM->RemoteR;
  RPrev = NULL;

  while (RR != NULL)
    {
    if ((RR->LocalRsv != NULL) &&
        (RR->LastSync > MSched.Time - MREMOTERSV_PURGETIME))
      {
      RPrev = RR;

      RR = RR->Next;

      continue;
      }

    /* remove local reservation */

    RNext = RR->Next;

    if (RPrev == NULL)
      RM->RemoteR = RNext;
    else
      RPrev->Next = RNext;

    MRsvDestroy(&RR->LocalRsv,TRUE,FALSE);

    MUFree((char **)&RR);

    RR = RNext;
    }

  return(SUCCESS);
  }  /* END MMRsvQueryPostLoad() */





/* NOTES/Pseudo-code for hosting center */

/* add per 'seat' active VPC session licensing for hosting center */
/* add per 'node' Moab client licensing which constrains number of local resources consumable at one time */
/* allow node license count to be exceeded if communicating with Moab hosting center */
/* allow moab to show hosting center resources which could be available 'if committed' */
/* allow support for per rm static reservations which cannot be removed by local admins to enforce resource availability timeframes (ie provisioning setup time, resource release time) */

/* END MMI.c */
