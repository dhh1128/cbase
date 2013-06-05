/* HEADER */

/**
 * @file MNatI.c
 *
 * Moab Native Interface
 */

/* Contains:                          
 * MNatWorkloadQuery()
 * MNatJobPreUpdate()
 * MNatDoCommand()          
 */

#include "moab.h"
#include "moab-wiki.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/* NOTE:  create serial launcher to set SID, obtain PID (NYI) */

/* NAT prototypes */

int MNatInitialize(mrm_t *,char *,enum MStatusCodeEnum *);
int MNatPing(mrm_t *,enum MStatusCodeEnum *);
int MNatRMStop(mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MNatGetData(mrm_t *,mbool_t,int,mbool_t *,char *,enum MStatusCodeEnum *);
int MNatJobStart(mjob_t *,mrm_t *,char *,enum MStatusCodeEnum *);
int MNatJobSubmit(const char *,mrm_t *,mjob_t **,mbitmap_t *,char *,char *,enum MStatusCodeEnum *);
int MNatJobRequeue(mjob_t *,mrm_t *,mjob_t **,char *,int *);
int MNatJobMigrate(mjob_t *,mrm_t *,char **,mnl_t *,char *,enum MStatusCodeEnum *);
int MNatJobModify(mjob_t *,mrm_t *,const char *,const char *,const char *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
int MNatJobCancel(mjob_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MNatJobSuspend(mjob_t *,mrm_t *,char *,int *);
int MNatJobResume(mjob_t *,mrm_t *,char *,int *);
int MNatJobCheckpoint(mjob_t *,mrm_t *,int,char *,int *);
int MNatClusterQuery(mrm_t *,int *,char *,enum MStatusCodeEnum *);
int MNatQueueQuery(mrm_t *,int *,char *,enum MStatusCodeEnum *);
int MNatWorkloadQuery(mrm_t *,int *,int *,char *,enum MStatusCodeEnum *);
int MNatInfoQuery(mrm_t *,char *,char *,char *,char *,enum MStatusCodeEnum *);
int MNatRsvCtl(mrsv_t *,mrm_t *,enum MRsvCtlCmdEnum,void **,char *,enum MStatusCodeEnum *);
int MNatSystemModify(mrm_t *,const char *,const char *,mbool_t,char *,enum MFormatModeEnum,char *,enum MStatusCodeEnum *);
int MNatSystemQuery(mrm_t *,char *,char *,mbool_t,char *,char *,enum MStatusCodeEnum *);
int MNatJobPreUpdate(mjob_t *,mrm_t *);
int MNatNodeLoad(mnode_t *,void *,enum MNodeStateEnum,mrm_t *);
int MNatNodeLoadFlat(mnode_t *,void *,enum MNodeStateEnum,mrm_t *);
int MNatNodeLoadExt(mnode_t *,void *,enum MNodeStateEnum,mrm_t *);
int MNatNodeModify(mnode_t *,mvm_t *,char *,mrm_t *,enum MNodeAttrEnum,void *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
int MNatResourceCreate(mrm_t *,enum MXMLOTypeEnum,char *,mstring_t *,char *,enum MStatusCodeEnum *);
int __MNatJobGetState(mxml_t *,mrm_t *,char *,int *);
int __MNatNodeGetState(char *,enum MNodeStateEnum *,char *,char *);
int MNatGetQueueInfo(mnode_t *,char C[][MMAX_NAME],char A[][MMAX_NAME]);
int __MNatGetTaskList(char *,int *,int *,int *);
int __MNatJobSetAttr(mjob_t *,char *,char *);
int __MNatNodeSetAttr(mnode_t *,mrm_t *,char *,char *);
char *__MNatGetErrMsg(int);

int __MNatClusterQueryGanglia(mstring_t *,mrm_t *,int *,char *,enum MStatusCodeEnum *);
int __MNatClusterLoadGanglia(mnat_t *,mstring_t *,char *);
int __MNatClusterProcessGanglia(mxml_t *,mrm_t *,int *);
int __MNatNodeProcess(mnode_t *,mrm_t *,mnat_t *,char *,mbool_t,mbool_t,enum MWNodeAttrEnum *,mnode_t *);
int __MNatClusterProcessData(mrm_t *,mnat_t *,mbool_t,mbool_t,mbool_t *,int *);
int __MNatWorkloadProcessData(mrm_t *,mnat_t *,const char *,int *,int *);
/* END NAT prototypes */


/* ganglia data */

#define MDEF_GANGLIAPORT 8649

/* NOTE:  sync with enum MGangliaXMLType */

const char *MGangliaXML[] = {
  "GANGLIA_XML",
  "CLUSTER",
  "GRID",
  "HOST",
  "METRIC",
  "NAME",
  "REPORTED",
  "VAL",
  NULL };

/* NOTE:  sync with MGangliaXML[] */

enum MGangliaXMLType {
  mxgMain = 0,
  mxgCluster,
  mxgGrid,
  mxgHost,
  mxgMetric,
  mxgName,
  mxgUTime,
  mxgVal };

/* Ganglia attributes captured */

/* sync w/enum MRMGangliaAttr */

const char *MRMGangliaAttrList[] = {
  "bytes_in",     /* Number of net bytes in per second (bytes/sec) */
  "bytes_out",    /* Number of net bytes out per second */
  "cpu_num",      /* Number of CPUs */
  "cpu_speed",    /* processor speed (in MHz) */
  "disk_free",    /* Total free disk space (GB) */
  "disk_total",   /* Total available disk space */
  "load_one",     /* One minute load average */
  "machine_type", /* cpu architecture */
  "mem_free",     /* Amount of available memory (KB) */
  "mem_total",    /* Amount of available memory */
  "os_name",
  "os_release",   /* operating system release */
  "pkts_in",      /*NYI */ /* Packets in per second (packets/sec) */
  "pkts_out",     /*NYI */ /* Packets out per second */
  "swap_free",    /* Amount of available swap memory (KB) */
  "swap_total",   /* Total amount of swap memory */
  NULL };

/* sync w/MRMGangliaAttrList[] */

enum MRMGangliaAttr {
  mrmgBytesIn = 0,
  mrmgBytesOut,
  mrmgCpuNum,
  mrmgCpuSpeed,
  mrmgDiskFree,
  mrmgDiskTotal,
  mrmgLoadOne,
  mrmgMachineType,
  mrmgMemFree,
  mrmgMemTotal,
  mrmgOSName,
  mrmgOSRelease,
  mrmgPktSIn,      /* NYI */
  mrmgPktSOut,     /* NYI */
  mrmgSwapFree,
  mrmgSwapTotal,
  mrmgUnknown,
  mrmgLAST };





/**
 * Load function pointers for native RM calls.
 *
 * @see MRMLoadModule() - parent
 *
 * @param F (I) [modified]
 */

int MNatLoadModule(

  mrmfunc_t *F)  /* I (modified) */

  {
  if (F == NULL)
    {
    return(FAILURE);
    }

  F->IsInitialized  = TRUE;

  F->ClusterQuery   = MNatClusterQuery;
  F->GetData        = NULL;
  F->InfoQuery      = MNatInfoQuery;
  F->JobCancel      = MNatJobCancel;
  F->JobCheckpoint  = NULL;
  F->JobMigrate     = MNatJobMigrate;
  F->JobModify      = MNatJobModify;
  F->JobRequeue     = MNatJobRequeue;
  F->JobResume      = MNatJobResume;
  F->JobStart       = MNatJobStart;
  F->JobSubmit      = MNatJobSubmit;
  F->JobSuspend     = MNatJobSuspend;
  F->QueueQuery     = MNatQueueQuery;
  F->Ping           = MNatPing;
  F->ResourceModify = MNatNodeModify;
  F->ResourceQuery  = NULL;
  F->RMInitialize   = MNatInitialize;
  F->RMEventQuery   = NULL;
  F->RMQuery        = NULL;
  F->RMStart        = MNatRMStart;
  F->RMStop         = MNatRMStop;
  F->SystemModify   = MNatSystemModify;
  F->SystemQuery    = MNatSystemQuery;
  F->WorkloadQuery  = MNatWorkloadQuery;
  F->NodeVirtualize = NULL;
  F->NodeMigrate    = NULL;
  F->RsvCtl         = MNatRsvCtl;
  F->ResourceCreate = MNatResourceCreate;
  
  return(SUCCESS);
  }  /* END MNatLoadModule() */





/**
 * Initialize native resource manager interface.
 *
 * @see MRMInitialize() - parent 
 * @see MNatRMInitialize() - child - performs external rm initialization
 *
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatInitialize(

  mrm_t                *R,    /* I */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  mnat_t *ND;

  char *ptr;

  static mbool_t ToolsDirAdded = FALSE;

  char   RMTool[MMAX_NAME];

  const char *FName = "MNatInitialize";

  MDB(1,fNAT) MLog("%s(%s,EMsg,SC)\n",
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

  if (ToolsDirAdded == FALSE)
    {
    MUStrAppend(&MSched.ServerPath,NULL,MSched.ToolsDir,':');

    ToolsDirAdded = TRUE;
    }

  if (R->S == NULL)
    {
    R->S = (mnatrm_t *)MUCalloc(1,sizeof(mnatrm_t));
    }

  RMTool[0] = '\0';  /* RMTool is name to RM tool suite to use, ie dstage */

  ND = &R->ND;
  ND->R = (void *)R;

  /* validate interfaces */

  /* NO-OP */

  /* set defaults */

  if (R->P[0].Timeout < 1000)
    {
    /* convert timeout to us */

    R->P[0].Timeout *= MDEF_USPERSECOND;
    }

  /* use the 'placement new' C++ operator to call the constructor on these */
  new (&R->ND.QueueQueryData) mstring_t(MMAX_LINE);
  new (&R->ND.ResourceQueryData) mstring_t(MMAX_LINE);
  new (&R->ND.WorkloadQueryData) mstring_t(MMAX_LINE);

  R->P[0].R = R;

  R->IsVirtual = TRUE;

  if (R->ND.Protocol[mrmClusterQuery] == mbpGanglia)
    {
    R->P[0].Port = ND->ServerPort;

    if (ND->Host[mrmClusterQuery] != NULL)
      R->P[0].HostName = ND->Host[mrmClusterQuery];
    else
      R->P[0].HostName = ND->ServerHost;

    if (ND->Port[mrmClusterQuery] != 0)
      R->P[0].Port = ND->Port[mrmClusterQuery];
    else
      R->P[0].Port = MDEF_GANGLIAPORT; 

    if (R->P[0].Timeout == 0)
      R->P[0].Timeout = MDEF_USPERSECOND * 10;  /* 10 seconds */

    if ((R->P[0].HostName == NULL) || (R->P[0].HostName[0] == '\0'))
      {
      /* NO-OP */
      }

    /* disable job execution and workload query */

    MRMSetDefaultFnList(R);

    bmunset(&R->FnList,mrmJobStart);
    bmunset(&R->FnList,mrmWorkloadQuery);
    }    /* END if (R->ND.Protocol[mrmClusterQuery] == mbpGanglia) */

  /* NOTE:  do not consider native resource managers to be executable 
            as far as batch jobs are concerned (although they can run 
            specific tasks) */

  if (!bmisset(&R->SpecFlags,mrmfExecutionServer))
    {
    bmunset(&R->Flags,mrmfExecutionServer);
    }

  /* default native subtype is aggregate-extension */

  switch (R->SubType)
    {
    case mrmstNONE:
    case mrmstAgFull:

      if (R->SubType == mrmstNONE)
        R->SubType = mrmstAgFull;

      if (bmisset(&MSched.Flags,mschedfAllowMultiCompute))
        {
        /* NOTE:  TORQUE autoconfig for mschedfAllowMultiCompute accomplished 
                  inside of mschedfAllowMultiCompute */

        /* NO-OP */
        }      /* END if (bmisset(&MSched.Flags,mschedfAllowMultiCompute)) */

      break;

    case mrmstCCS:

      if (R->ND.URL[mrmClusterQuery] == NULL)
        MRMSetAttr(R,mrmaClusterQueryURL,(void **)"exec://$TOOLSDIR/ccs/node.query.ccs.pl",mdfString,mSet);

      if (R->ND.URL[mrmWorkloadQuery] == NULL)
        MRMSetAttr(R,mrmaWorkloadQueryURL,(void **)"exec://$TOOLSDIR/ccs/job.query.ccs.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobSubmit] == NULL)
        MRMSetAttr(R,mrmaJobSubmitURL,(void **)"exec://$TOOLSDIR/ccs/job.submit.ccs.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobStart] == NULL)
        MRMSetAttr(R,mrmaJobStartURL,(void **)"exec://$TOOLSDIR/ccs/job.start.ccs.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobCancel] == NULL)
        MRMSetAttr(R,mrmaJobCancelURL,(void **)"exec://$TOOLSDIR/ccs/job.cancel.ccs.pl",mdfString,mSet);

      R->ND.AdminExec[mrmJobSubmit] = TRUE;

      bmset(&R->IFlags,mrmifNoExpandPath);

      break;

    case mrmstMSM:

      {
      muenv_t EP;

      char SrcEnvBuf[MMAX_LINE];
      char DstEnvBuf[MMAX_LINE];

      if (R->ND.URL[mrmClusterQuery] == NULL)
        MRMSetAttr(R,mrmaClusterQueryURL,(void **)"exec://$TOOLSDIR/msm/contrib/cluster.query.pl",mdfString,mSet);

#ifdef MNOT
          NOTE:  MOAB-1915 was recycling moab after the vmstorage was successful.  The vmcreate job was reported
          via the msm without proper user credentials that caused moab to reject the create job and resubmit the storage and
          create job.  Therefore, we decided not to have a default msm workload query and allow the vmcreate to be restored
          by the checkpoint file.
       
      if (R->ND.URL[mrmWorkloadQuery] == NULL)
        MRMSetAttr(R,mrmaWorkloadQueryURL,(void **)"exec://$TOOLSDIR/msm/contrib/workload.query.pl",mdfString,mSet);
#endif /* MNOT */

      if (R->ND.URL[mrmJobCancel] == NULL)
        MRMSetAttr(R,mrmaJobCancelURL,(void **)"exec://$TOOLSDIR/msm/contrib/job.cancel.pl",mdfString,mSet);

      /* moab currently does VM migrations using the JOBMIGRATEURL, which needs to
       * point to node.migrate.pl for MSM */

      if (R->ND.URL[mrmJobMigrate] == NULL)
        MRMSetAttr(R,mrmaJobMigrateURL,(void **)"exec://$TOOLSDIR/msm/contrib/node.migrate.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobModify] == NULL)
        MRMSetAttr(R,mrmaJobModifyURL,(void **)"exec://$TOOLSDIR/msm/contrib/job.modify.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobStart] == NULL)
        MRMSetAttr(R,mrmaJobStartURL,(void **)"exec://$TOOLSDIR/msm/contrib/job.start.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobSubmit] == NULL)
        MRMSetAttr(R,mrmaJobSubmitURL,(void **)"exec://$TOOLSDIR/msm/contrib/job.submit.pl",mdfString,mSet);

      if (R->ND.URL[mrmNodeModify] == NULL)
        MRMSetAttr(R,mrmaNodeModifyURL,(void **)"exec://$TOOLSDIR/msm/contrib/node.modify.pl",mdfString,mSet);

      if (R->ND.URL[mrmXNodePower] == NULL)
        MRMSetAttr(R,mrmaNodePowerURL,(void **)"exec://$TOOLSDIR/msm/contrib/node.power.pl",mdfString,mSet);

      if (R->ND.URL[mrmResourceCreate] == NULL)
        MRMSetAttr(R,mrmaResourceCreateURL,(void **)"exec://$TOOLSDIR/msm/contrib/node.create.pl",mdfString,mSet);

      if (R->ND.URL[mrmSystemModify] == NULL)
        MRMSetAttr(R,mrmaSystemModifyURL,(void **)"exec://$TOOLSDIR/msm/contrib/node.modify.pl",mdfString,mSet);

      if (R->ND.URL[mrmRMStart] == NULL)
        MRMSetAttr(R,mrmaRMStartURL,(void **)"exec://$TOOLSDIR/msm/bin/msmd",mdfString,mSet);

      if (R->ND.URL[mrmRMStop] == NULL)
        MRMSetAttr(R,mrmaRMStopURL,(void **)"exec://$TOOLSDIR/msm/bin/msmctl?-k",mdfString,mSet);

      bmset(&R->Flags,mrmfAutoSync);

      if ((MSched.DefaultNAccessPolicy == mnacNONE) || (MSched.ParamSpecified[mcoNAPolicy] == FALSE))
        MSched.DefaultNAccessPolicy = mnacSingleJob;

      if (R->Env == NULL)
        {
        sprintf(SrcEnvBuf,"MSMHOMEDIR=$TOOLSDIR/msm" ENVRS_ENCODED_STR "MSMLIBDIR=$TOOLSDIR/msm");

        memset(&EP,0,sizeof(EP));

        MUEnvAddValue(&EP,"TOOLSDIR",MSched.ToolsDir);

        MUEnvAddValue(&EP,"HOME",MSched.CfgDir);

        MUInsertEnvVar(SrcEnvBuf,&EP,DstEnvBuf);

        MUEnvDestroy(&EP);

        /* NOTE:  custom RM Env attributes should already be applied at this 
                  point.  If so, append defaults to allow custom Env attributes
                  to override default values */

        if (R->Env != NULL)
          {
          MUStrAppendStatic(DstEnvBuf,R->Env,'\036',sizeof(DstEnvBuf));
          }

        MRMSetAttr(R,mrmaEnv,(void **)DstEnvBuf,mdfString,mSet);
        }  /* END if (R->Env == NULL) */  
      }

      break;  /* END BLOCK (case mrmstMSM) */

    case mrmstSGE:

      if (R->ND.URL[mrmClusterQuery] == NULL)
        MRMSetAttr(R,mrmaClusterQueryURL,(void **)"exec://$TOOLSDIR/sge/node.query.sge.pl",mdfString,mSet);

      if (R->ND.URL[mrmWorkloadQuery] == NULL)
        MRMSetAttr(R,mrmaWorkloadQueryURL,(void **)"exec://$TOOLSDIR/sge/job.query.sge.pl",mdfString,mSet);

      if (R->ND.URL[mrmQueueQuery] == NULL)
        MRMSetAttr(R,mrmaQueueQueryURL,(void **)"exec://$TOOLSDIR/sge/class.query.sge.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobSubmit] == NULL)
        MRMSetAttr(R,mrmaJobSubmitURL,(void **)"exec://$TOOLSDIR/sge/job.submit.sge.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobStart] == NULL)
        MRMSetAttr(R,mrmaJobStartURL,(void **)"exec://$TOOLSDIR/sge/job.start.sge.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobModify] == NULL)
        MRMSetAttr(R,mrmaJobModifyURL,(void **)"exec://$TOOLSDIR/sge/job.modify.sge.pl",mdfString,mSet);

      /* job suspend/resume/checkpoint/requeue? */

      if (R->ND.URL[mrmJobCancel] == NULL)
        MRMSetAttr(R,mrmaJobCancelURL,(void **)"exec://$TOOLSDIR/sge/job.cancel.sge.pl",mdfString,mSet);

      R->ND.AdminExec[mrmJobStart] = TRUE;

      bmset(&MSched.Flags,mschedfValidateFutureNodesets);

      MPar[0].JobRetryTime = MCONST_MINUTELEN;

      break;  /* END SGE */

    case mrmstXT3:

      {
      char *ptr;

      /* NOTE: 'noallocmaster' enabled by default for XT3 - switch to non-default by Q1 2007 */

      if (getenv("MOABALLOCMASTER") != NULL)
        R->NoAllocMaster = FALSE;
      else
        R->NoAllocMaster = TRUE;

      if (getenv("MOABDELTARESOURCEINFO") != NULL)
        bmset(&R->IFlags,mrmifAllowDeltaResourceInfo);

      bmset(&R->Flags,mrmfCompressHostList);

      ptr = getenv("MOABJOBCTLDIR");

      if (ptr != NULL)
        {
        if (!strcasecmp(ptr,"home"))
          R->ND.JobCtlDir[0] = '\0';
        else
          MUStrCpy(R->ND.JobCtlDir,ptr,sizeof(R->ND.JobCtlDir));
        }
      else
        {
        /* issue job control commands from Moab home dir */
  
        R->ND.JobCtlDir[0] = '\1';
        }

      if (R->ND.URL[mrmClusterQuery] == NULL)
        MRMSetAttr(R,mrmaClusterQueryURL,(void **)"exec://$TOOLSDIR/xt3/node.query.xt3.pl",mdfString,mSet);

      if (R->ND.URL[mrmWorkloadQuery] == NULL)
        MRMSetAttr(R,mrmaWorkloadQueryURL,(void **)"exec://$TOOLSDIR/xt3/job.query.xt3.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobStart] == NULL)
        MRMSetAttr(R,mrmaJobStartURL,(void **)"exec://$TOOLSDIR/xt3/job.start.xt3.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobCancel] == NULL)
        MRMSetAttr(R,mrmaJobCancelURL,(void **)"exec://$TOOLSDIR/xt3/job.cancel.xt3.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobRequeue] == NULL)
        MRMSetAttr(R,mrmaJobRequeueURL,(void **)"exec://$TOOLSDIR/xt3/job.requeue.xt3.pl",mdfString,mSet);

      if (R->ND.URL[mrmSystemQuery] == NULL)
        MRMSetAttr(R,mrmaSystemQueryURL,(void **)"exec://$TOOLSDIR/xt3/partition.query.xt3.pl",mdfString,mSet);

      /* NOTE:  nodeindex must be enabled before nodes are loaded */
      /*        also enabled in MRMProcessConfig() */

      /* enable node index by default */

      if ((ptr = getenv("MOABUSENODEINDEX")) != NULL) 
        {
        MSched.UseNodeIndex = MUBoolFromString(ptr,TRUE);
        }
      else
        {
        MSched.UseNodeIndex = TRUE;
        }

      R->ND.AdminExec[mrmJobStart] = TRUE;

      /* NOTE:  XT3 systems utilize TORQUE/PBSPro for RM services */

      if (MRMFunc[mrmtPBS].RMEventQuery != NULL)
        {
        MRMFunc[mrmtNative].RMEventQuery = MRMFunc[mrmtPBS].RMEventQuery;

        R->PBS.SchedS.sd = -1;
        }

      if (MRMFunc[mrmtPBS].JobModify != NULL)
        {
        MRMFunc[mrmtNative].JobModify = MRMFunc[mrmtPBS].JobModify;

        R->PBS.ServerSD = -1;
        }

      if (MRMFunc[mrmtPBS].QueueQuery != NULL)
        {
        MRMFunc[mrmtNative].QueueQuery = MRMFunc[mrmtPBS].QueueQuery;

        R->PBS.ServerSD = -1;
        }

      if (MRMFunc[mrmtPBS].SystemModify != NULL)
        {
        MRMFunc[mrmtNative].SystemModify = MRMFunc[mrmtPBS].SystemModify;
        }

      MRMFunc[mrmtNative].JobSubmit  = MRMFunc[mrmtPBS].JobSubmit;

      if (MRMFunc[mrmtPBS].RMInitialize != NULL)
        {
        if ((MRMFunc[mrmtPBS].RMInitialize)(R,NULL,NULL) == SUCCESS)
          {
          char tmpLine[MMAX_LINE];

          if (R->PBS.RMJobCPurgeTime <= 5 * MCONST_MINUTELEN)
            {
            strcpy(tmpLine,"300");

            if (MRMSystemModify(
                R,
                "system:modify",
                "keep_completed",
                TRUE,
                tmpLine,  /* I/O */
                mfmNONE,
                EMsg,
                NULL) == FAILURE)
              {
              return(FAILURE);
              }
            }

          strcpy(tmpLine,"true");

          if (MRMSystemModify(
              R,
              "system:modify",
              "scheduling",
              TRUE,
              tmpLine,  /* I/O */
              mfmNONE,
              EMsg,
              NULL) == FAILURE)
            {
            return(FAILURE);
            }
          }
        }

      if (bmisclear(&R->FnList))
        {
        MRMFuncSetAll(R);
        }

      bmset(&R->Languages,mrmtPBS);
      }  /* END BLOCK (case mrmstXT3) */

      break;  

    case mrmstXT4:

      {
      char *ptr;

      /* NOTE: 'noallocmaster' enabled by default for XT4 - switch to non-default by Q3 2007 */

      if (getenv("MOABALLOCMASTER") != NULL)
        {
        R->NoAllocMaster = FALSE;
        }
      else
        {
        mrm_t *InternalRM = NULL;

        MRMGetInternal(&InternalRM);
        
        InternalRM->NoAllocMaster = TRUE;
        R->NoAllocMaster          = TRUE;
        }

      if (getenv("MOABDELTARESOURCEINFO") != NULL)
        bmset(&R->IFlags,mrmifAllowDeltaResourceInfo);

      bmset(&R->Flags,mrmfCompressHostList);

      ptr = getenv("MOABJOBCTLDIR");

      if (ptr != NULL)
        {
        if (!strcasecmp(ptr,"home"))
          R->ND.JobCtlDir[0] = '\0';
        else
          MUStrCpy(R->ND.JobCtlDir,ptr,sizeof(R->ND.JobCtlDir));
        }
      else
        {
        /* issue job control commands from Moab home dir */
  
        R->ND.JobCtlDir[0] = '\1';
        }

      if (R->ND.URL[mrmClusterQuery] == NULL)
        MRMSetAttr(R,mrmaClusterQueryURL,(void **)"exec://$TOOLSDIR/xt4/node.query.xt4.pl",mdfString,mSet);

      if (R->ND.URL[mrmWorkloadQuery] == NULL)
        MRMSetAttr(R,mrmaWorkloadQueryURL,(void **)"exec://$TOOLSDIR/xt4/job.query.xt4.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobStart] == NULL)
        MRMSetAttr(R,mrmaJobStartURL,(void **)"exec://$TOOLSDIR/xt4/job.start.xt4.pl",mdfString,mSet);

      if (R->ND.URL[mrmSystemQuery] == NULL)
        MRMSetAttr(R,mrmaSystemQueryURL,(void **)"exec://$TOOLSDIR/xt4/partition.query.xt4.pl",mdfString,mSet);

      if (R->ND.URL[mrmXParCreate] == NULL)
        MRMSetAttr(R,mrmaParCreateURL,(void **)"exec://$TOOLSDIR/xt4/partition.create.xt4.pl",mdfString,mSet);

      if (R->ND.URL[mrmXParDestroy] == NULL)
        MRMSetAttr(R,mrmaParDestroyURL,(void **)"exec://$TOOLSDIR/xt4/partition.delete.xt4.pl",mdfString,mSet);

      /* NOTE:  nodeindex must be enabled before nodes are loaded */
      /*        also enabled in MRMProcessConfig() */

      /* enable node index by default */

      if ((ptr = getenv("MOABUSENODEINDEX")) != NULL) 
        {
        MSched.UseNodeIndex = MUBoolFromString(ptr,TRUE);
        }
      else
        {
        MSched.UseNodeIndex = TRUE;
        }

      R->ND.AdminExec[mrmJobStart] = TRUE;

      /* We need the mom node to be the same when checkpointing, */
      /* not the first compute node. */
      /* This is now handled in tools/xt4/job.start.xt4.pl */
      /* bmset(&R->IFlags,mrmifNoMigrateCkpt); */

      /* NOTE:  XT4 systems utilize TORQUE/PBSPro for RM services */

      MRMFunc[mrmtNative].JobSubmit  = MRMFunc[mrmtPBS].JobSubmit;

      if (MRMFunc[mrmtPBS].RMEventQuery != NULL)
        {
        MRMFunc[mrmtNative].RMEventQuery = MRMFunc[mrmtPBS].RMEventQuery;

        R->PBS.SchedS.sd = -1;
        }

      if (MRMFunc[mrmtPBS].JobModify != NULL)
        {
        MRMFunc[mrmtNative].JobModify = MRMFunc[mrmtPBS].JobModify;

        R->PBS.ServerSD = -1;
        }

      if (MRMFunc[mrmtPBS].JobCancel != NULL)
        {
        MRMFunc[mrmtNative].JobCancel = MRMFunc[mrmtPBS].JobCancel;

        R->PBS.ServerSD = -1;
        }

      if (MRMFunc[mrmtPBS].JobRequeue != NULL)
        {
        MRMFunc[mrmtNative].JobRequeue = MRMFunc[mrmtPBS].JobRequeue;

        R->PBS.ServerSD = -1;
        }

      if (MRMFunc[mrmtPBS].JobSignal != NULL)
        {
        MRMFunc[mrmtNative].JobSignal = MRMFunc[mrmtPBS].JobSignal;
        }

      if (MRMFunc[mrmtPBS].QueueQuery != NULL)
        {
        MRMFunc[mrmtNative].QueueQuery = MRMFunc[mrmtPBS].QueueQuery;

        R->PBS.ServerSD = -1;
        }

      if (MRMFunc[mrmtPBS].SystemModify != NULL)
        {
        MRMFunc[mrmtNative].SystemModify = MRMFunc[mrmtPBS].SystemModify;
        }

      if (MRMFunc[mrmtPBS].RMInitialize != NULL)
        {
        if ((MRMFunc[mrmtPBS].RMInitialize)(R,NULL,NULL) == SUCCESS)
          {
          char tmpLine[MMAX_LINE];

          if (R->PBS.RMJobCPurgeTime <= 5 * MCONST_MINUTELEN)
            {
            strcpy(tmpLine,"300");

            if (MRMSystemModify(
                R,
                "system:modify",
                "keep_completed",
                TRUE,
                tmpLine,  /* I/O */
                mfmNONE,
                EMsg,
                NULL) == FAILURE)
              {
              return(FAILURE);
              }
            }

          strcpy(tmpLine,"true");

          if (MRMSystemModify(
              R,
              "system:modify",
              "scheduling",
              TRUE,
              tmpLine,  /* I/O */
              mfmNONE,
              EMsg,
              NULL) == FAILURE)
            {
            return(FAILURE);
            }
          }
        }    /* END if (MRMFunc[mrmtPBS].RMInitialize != NULL) */

      if (bmisclear(&R->FnList))
        {
        MRMFuncSetAll(R);
        }

      /* NOTE:  XT4/APBasil does not allow sharing of nodes within
                allocation reservation system (ALPS) */

      if (MSched.DefaultNAccessPolicy == MDEF_NACCESSPOLICY)
        MSched.DefaultNAccessPolicy = mnacSingleJob;

      bmset(&R->Languages,mrmtPBS);
      }  /* END BLOCK (case mrmstXT4) */

      break;  

    case mrmstX1E:

      R->ND.NatType = mrmstX1E;

      if (getenv("MOABNOALLOCMASTER") != NULL)
        R->NoAllocMaster = TRUE;

      if (R->ND.URL[mrmClusterQuery] == NULL)
        MRMSetAttr(R,mrmaClusterQueryURL,(void **)"exec://$TOOLSDIR/x1e/node.query.x1e.pl",mdfString,mSet);

      if (R->ND.URL[mrmWorkloadQuery] == NULL)
        MRMSetAttr(R,mrmaWorkloadQueryURL,(void **)"exec://$TOOLSDIR/x1e/job.query.x1e.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobSubmit] == NULL)
        MRMSetAttr(R,mrmaJobSubmitURL,(void **)"exec://$TOOLSDIR/x1e/job.submit.x1e.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobStart] == NULL)
        MRMSetAttr(R,mrmaJobStartURL,(void **)"exec://$TOOLSDIR/x1e/job.start.x1e.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobCancel] == NULL)
        MRMSetAttr(R,mrmaJobCancelURL,(void **)"exec://$TOOLSDIR/x1e/job.cancel.x1e.pl",mdfString,mSet);

      if (R->ND.URL[mrmJobRequeue] == NULL)
        MRMSetAttr(R,mrmaJobRequeueURL,(void **)"exec://$TOOLSDIR/x1e/job.requeue.x1e.pl",mdfString,mSet);

      /* NOTE:  nodeindex must be enabled before nodes are loaded */
      /*        also enabled in MRMProcessConfig() */

      /* enable node index by default */

      if ((ptr = getenv("MOABUSENODEINDEX")) != NULL)
        {
        MSched.UseNodeIndex = MUBoolFromString(ptr,TRUE);
        }
      else
        {
        MSched.UseNodeIndex = TRUE;
        }

      R->ND.AdminExec[mrmJobStart] = TRUE;

      if (bmisclear(&R->FnList))
        {
        MRMFuncSetAll(R);
        }

      bmset(&R->Languages,mrmtPBS);

      break;  /* END BLOCK (case mrmstX1E) */

    default:

      /* NO-OP */

      break;
    }  /* END switch (R->SubType) */

  /* disable node cache by default */

  if ((ptr = getenv("MOABUSENODECACHE")) != NULL)
    {
    R->UseNodeCache = MUBoolFromString(ptr,FALSE);
    }
  else
    {
    R->UseNodeCache = FALSE;
    }

  if ((MSched.Iteration <= 0) &&
      (R->State == mrmsNONE) &&
       bmisset(&R->Flags,mrmfAutoSync) &&
      (ND->URL[mrmRMStart] != NULL))
    {
    if (MNatRMStart(R,NULL,EMsg,SC) == FAILURE)
      {
      bmset(&R->IFlags,mrmifRMStartRequired);

      MDB(2,fNAT) MLog("WARNING:  unable to start RM '%s' - %s\n",
        R->Name,
        (EMsg != NULL) ? EMsg : "???");
      }
    }

  if (bmisset(&R->RTypes,mrmrtStorage) &&
      (ND->URL[mrmRMInitialize] != NULL))
    {
    /* always need to initialize a storage manager as they can be created
       dynamically in a hosting environment */

    MNatRMInitialize(R,RMTool,EMsg,SC);
    }
  else if ((MSched.Iteration <= 0) &&
      (ND->URL[mrmRMInitialize] != NULL))
    {
    MNatRMInitialize(R,RMTool,EMsg,SC);
    }

  if (bmisset(&R->IFlags,mrmifRMStartRequired))
    {
    /* unable to start RM service */

    MRMSetState(R,mrmsDown);
    }
  else if (MNatPing(R,NULL) == SUCCESS)
    {
    MRMSetState(R,mrmsActive);
    }
  else
    {
    /* NOTE:  Should pass in SC to MNatPing and only mark RM down if ping 
              failed and ping function is supported (NYI) */

    if (bmisset(&R->RTypes,mrmrtCompute))
      MRMSetState(R,mrmsDown);
    }

  return(SUCCESS);
  }  /* END MNatInitialize() */





/**
 * Check RM health via native interface.
 *
 * @see MRMPing() - parent
 *
 * @param R (I)
 * @param SC (O) [optional]
 */

int MNatPing(

  mrm_t                *R,   /* I */
  enum MStatusCodeEnum *SC)  /* O (optional) */

  {
  enum MBaseProtocolEnum Protocol = mbpNONE;
  char *fileName;

  const char *FName = "MNatPing";

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

  if (R->ND.URL[mrmClusterQuery] != NULL)
    {
    Protocol = R->ND.Protocol[mrmClusterQuery];
    }
  else if (R->ND.URL[mrmWorkloadQuery] != NULL)
    {
    Protocol = R->ND.Protocol[mrmWorkloadQuery];
    }

  if (Protocol == mbpGanglia)
    {
    int   rc;
  
    mstring_t String(MMAX_LINE);

    rc = __MNatClusterLoadGanglia(
           &R->ND,
           &String,
           NULL);

    if (rc == FAILURE)
      {
      MDB(2,fCORE) MLog("INFO:     cannot ping ganglia\n");

      if (SC != NULL)
        *SC = mscRemoteFailure;

      return(FAILURE);
      }
    }    /* END if (Protocol == mbpGanglia) */
  else if ((Protocol == mbpExec) || (Protocol == mbpFile))
    {
    struct stat sbuf;

    if (R->ND.Path[mrmClusterQuery] != NULL)
      {
      fileName = R->ND.Path[mrmClusterQuery];
      }
    else if (R->ND.Path[mrmWorkloadQuery] != NULL)
      {
      fileName = R->ND.Path[mrmWorkloadQuery];
      }
    else 
      {
      MDB(2,fCORE) MLog("INFO:     file not specified, cannot ping RM\n");

      if (SC != NULL)
        *SC = mscNoFile;

      return(FAILURE);
      }

    if (stat(fileName,&sbuf) == -1)
      {
      MDB(2,fCORE) MLog("INFO:     cannot stat file '%s', errno: %d (%s)\n",
        fileName,
        errno,
        strerror(errno));

      if (SC != NULL)
        *SC = mscNoFile;

      return(FAILURE);
      }

    if (Protocol == mbpExec)
      {
      /* check if file has executable permissions */

      if ((sbuf.st_mode & S_IXUSR) == FALSE)
        {
        MDB(2,fCORE) MLog("INFO:     file '%s' does not have user execute permission (st_mode = %ld)\n",
          fileName,
          (long)sbuf.st_mode);

        if (SC != NULL)
          *SC = mscNoAuth;

        return(FAILURE);
        }
      }
    }    /* END else if ((Protocol == mbpExec) || (Protocol == mbpFile)) */
  else
    {
    /* Protocol not supported */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatPing() */





/**
 * Process cluster/node/resource info from native interface.
 *
 * @see MRMClusterQuery() - parent
 * @see MNatGetData() - peer - loads raw cluster data from RM
 * @see MNatWorkloadQuery() - peer - process workload info
 * @see __MNatClusterProcessData() - child - process raw cluster data
 *
 * @param R (I)
 * @param RCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatClusterQuery(

  mrm_t                *R,       /* I */
  int                  *RCount,  /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  mbool_t GlobalIsNew;
  mbool_t IsMaster;
  mbool_t NodeAdded;

  const char *FName = "MNatClusterQuery";

  MDB(1,fNAT) MLog("%s(%s,RCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (RCount != NULL)
    *RCount = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, skipping cluster query\n",
      R->Name);

    if (EMsg != NULL)
      strcpy(EMsg,"rm is down");

    return(FAILURE);
    }

  /* NOTE:  MNatGetData() will populate DefND->ResourceQueryData */

  if (MNatGetData(
        R,
        FALSE,
        -1,
        NULL,
        EMsg,
        SC) == FAILURE)
    {
    MDB(1,fNAT) MLog("INFO:     cluster query getdata failed for native interface\n");

    return(FAILURE);
    }

  /* determine default ND structure */

  if (R->ND.URL[mrmClusterQuery] == NULL)
    {
    MDB(1,fNAT) MLog("INFO:     cluster query action not specified for native interface\n");

    if (EMsg != NULL)
      strcpy(EMsg,"cluster query action not specified");

    return(SUCCESS);
    }

  if (R->ND.EffProtocol[mrmClusterQuery] == mbpGanglia)
    {
    int rc;

    rc = __MNatClusterQueryGanglia(
       &R->ND.ResourceQueryData,  /* I */
       R,
       RCount,
       EMsg,    /* O */
       SC);

    return(rc);
    }  /* END if ((Protocol == mbpGanglia) || ...) */

  /* initialize stats */

  /* NYI */

  GlobalIsNew = FALSE;
  NodeAdded   = FALSE;

  NodeAdded = GlobalIsNew;

  switch (R->SubType)
    {
    case mrmstAgFull:
    case mrmstMSM:
    case mrmstSGE:
    case mrmstX1E:
    case mrmstXT3:
    case mrmstXT4:

      IsMaster = TRUE;

      break;

    default:

      if (bmisset(&R->Flags,mrmfNoCreateAll) || bmisset(&R->Flags,mrmfNoCreateResource))
        IsMaster = FALSE;
      else
        IsMaster = TRUE;

      break;
    }  /* END switch (R->SubType) */

  /* initialize per iteration RM stats */

  if (bmisset(&R->RTypes,mrmrtLicense))
    {
    /* preserve info on individual license rm for multi-license rm env */

    R->U.Nat.CurrentUResCount      = 0;
    R->U.Nat.CurrentResAvailCount  = 0;
    R->U.Nat.CurrentResConfigCount = 0;

    /* according to moab.h these are actually running totals
    R->U.Nat.ResAvailCount[0]  = 0;
    R->U.Nat.ResConfigCount[0] = 0;
    */
    }  /* END if (bmisset(&R->RTypes,mrmrtLicense)) */

  /* examine all nodes */

  if (__MNatClusterProcessData(
       R,
       &R->ND,
       GlobalIsNew,
       IsMaster,
       &NodeAdded,
       RCount) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatClusterQuery() */







/**
 * NOTE:  make M*NodeModify support async flag for group operations (NYI)
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

int MNatNodeModify(

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
  mnode_t *N;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (((SN == NULL) && (Ext == NULL)) || (AIndex <= mnaNONE) || (AttrValue == NULL))
    {
    return(FAILURE);
    }

  if (SN == NULL)
    {
    char *tail;

    /* extract nodename from list */

    /* FORMAT: <NODEID>[,<NODEID>]... */

    if ((tail = strchr(Ext,',')) != NULL)
      {
      *tail = '\0';
      }

    if (MNodeFind(Ext,&N) == FAILURE)
      {
      if (tail != NULL)
        *tail = ',';

      return(FAILURE);
      }

    if (tail != NULL)
      *tail = ',';
    }
  else
    {
    N = SN;
    }

  if (R->ND.Protocol[mrmNodeModify] == mbpNONE)
    {
    MDB(1,fNAT) MLog("ALERT:    cannot modify attribute %s on node %s to value %s (%s)\n",
      MNodeAttr[AIndex],
      (Ext != NULL) ? Ext : N->Name,
      (char *)AttrValue,
      "no modify operation defined");

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"no modify operation defined");

    return(FAILURE);
    }

  mstring_t CmdLine(MMAX_LINE);

  /* FORMAT:  <EXEC> <NODEID>[,<NODEID>]... [--force] {--set <ATTR>=<VAL>|--clear <ATTR>} */

  /* NOTE: R->ND.NodeModifyPath may contain '?' delimited args, must strip and perform 
           variable substitution as needed (NYI) */

  /* NOTE:  must enable support for 'force' (NYI) */

  if (Mode == mSet)
    {
    if (SV != NULL)
      {
      MStringAppendF(&CmdLine,"%s %s:%s --set %s=%s",
        (R->ND.Path[mrmNodeModify] != NULL) ? R->ND.Path[mrmNodeModify] : MNAT_NODEMODIFYPATH,
        (Ext != NULL) ? Ext : N->Name,
        SV->VMID,
        MNodeAttr[AIndex],
        (char *)AttrValue);
      }
    else
      {
      MStringAppendF(&CmdLine,"%s %s --set %s=%s",
        (R->ND.Path[mrmNodeModify] != NULL) ? R->ND.Path[mrmNodeModify] : MNAT_NODEMODIFYPATH,
        (Ext != NULL) ? Ext : N->Name,
        MNodeAttr[AIndex],
        (char *)AttrValue);
      }
    }
  else
    {
    MStringAppendF(&CmdLine,"%s %s --clear %s",
      (R->ND.Path[mrmNodeModify] != NULL) ? R->ND.Path[mrmNodeModify] : MNAT_NODEMODIFYPATH,
      (Ext != NULL) ? Ext : N->Name,
      MNodeAttr[AIndex]);
    }

  mstring_t Response(MMAX_LINE);

  if (MNatDoCommand(
       &R->ND,
       (void *)N,
       mrmResourceModify,
       R->ND.Protocol[mrmNodeModify],
       TRUE,
       NULL,
       CmdLine.c_str(),    /* I */
       &Response,
       EMsg,
       NULL) == FAILURE)
    {
    /* cannot change node attribute */

    MDB(1,fNAT) MLog("ALERT:    cannot modify attribute %s on node %s to value %s (%s)\n",
      MNodeAttr[AIndex],
      (Ext != NULL) ? Ext : N->Name,
      (char *)AttrValue,
      Response.c_str());

    return(FAILURE);
    }

  if (R->ND.Path[mrmNodeModify] == NULL)
    {
    if (AIndex == mnaNodeState)
      {
      /* who/what uses this code?  can this be removed? */

      enum MNodeStateEnum NState;

      NState = (enum MNodeStateEnum)MUGetIndexCI((char *)AttrValue,MNodeState,FALSE,0);

      /* set/clear state within Moab because we do not communicate with RM */

      if (Mode == mSet)
        {
        MNodeSetState(N,NState,FALSE);

        N->SpecState = NState;
        }
      else
        {
        MNodeSetState(N,NState,FALSE);

        N->SpecState = mnsNONE;
        }
      }
    }

  return(SUCCESS);
  }  /* END MNatNodeModify() */





/**
 * Perform core native RM command and report results/status.
 *
 * @see MUSpawnChild() - child
 *
 * @param ND         (I)
 * @param O          (I) [optional object - usually mjob_t or mnode_t]
 * @param CmdType    (I)
 * @param Protocol   (I)
 * @param IsBlocking (I)
 * @param HostName   (I) [optional]
 * @param SPath      (I) [optional,may include args to path]
 * @param ResponseP  (O) [optional,minsize=BufSize] STDOUT of request
 * @param EMsg       (O) [optional,minsize=MMAX_LINE]
 * @param SC         (O) [optional]
 *
 * @return FAILURE if the underlying command returns a non-zero exit code or if
 *  MSched.ChildStderrCheck is TRUE and stderr contains they keyword ERROR. Returns
 *  SUCCESS otherwise.
 */

int MNatDoCommand(

  mnat_t                *ND,
  void                  *O,
  enum MRMFuncEnum       CmdType,
  enum MBaseProtocolEnum Protocol,
  mbool_t                IsBlocking,
  const char            *HostName,
  const char            *SPath,
  mstring_t             *ResponseP,
  char                  *EMsg,
  enum MStatusCodeEnum  *SC)

  {
  char *Path;
  char *mutableSPath = NULL;

  int rc;

  mulong Timeout = 0;  /* in milli-seconds */

  mrm_t *R = NULL;

  mjob_t  *J = NULL;

  const char *FName = "MNatDoCommand";

  MDB(1,fNAT) MLog("%s(ND,%s,%s,%s,%s,%s,Response,%s,SC)\n",
    FName,
    MRMFuncType[CmdType],
    MBaseProtocol[Protocol],
    MBool[IsBlocking],
    (HostName != NULL) ? HostName : "NULL",
    (SPath != NULL) ? SPath : "NULL",
    (EMsg != NULL) ? "EMsg" : "NULL");

  if (ResponseP != NULL)
    MStringSet(ResponseP,"\0");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((ResponseP == NULL) || (ND == NULL))
    {
    if (SC != NULL)
      *SC = mscBadParam;

    return(FAILURE);
    }

  if (ND->R != NULL)
    {
    R = (mrm_t *)ND->R;

    /* NOTE: native RMs have Timeouts of microseconds (us) not seconds like normal ones */                   

    if (MSched.EnableFastNativeRM == FALSE)
      Timeout = MAX((R->P[0].Timeout / 1000),3000);
    else
      Timeout = R->P[0].Timeout / 1000;
    }
  else if (MSched.EnableFastNativeRM == FALSE)
    {
    Timeout = 3000;
    }

#ifdef MYAHOO
  Timeout = 0;  /* this can have dangerous side-effects...namely the OBuf and EBuf
                   will no longer produce the actual output, but instead only give
                   back filenames (see inside of MUSpawnChild)!!!! */
#endif /* MYAHOO */

  if (IsBlocking == FALSE)
    {
    /* This may not be a good idea, as it could break expected behavior for
     * some calls (see above comment in MYAHOO block), but this REALLY helps
     * with ODVM model and MSM slow-downs. A more robust method of setting timeout to 0
     * will be needed in the future. */

    Timeout = 0;
    }

  if (SPath != NULL)
    {
    MUStrDup(&mutableSPath,SPath);
    Path = mutableSPath;
    }
  else
    {
    Path = NULL;

    switch (CmdType)
      {
      case mrmResourceModify:

        Path = ND->Path[mrmNodeModify];

        break;

      case mrmJobCancel:

        Path = ND->Path[mrmJobCancel];
        J    = (mjob_t *)O;

        break;

      case mrmJobStart:

        Path = ND->Path[mrmJobStart];
        J    = (mjob_t *)O;

        break;

      case mrmResourceQuery:

        /* If HTTP request, port will be set in the mbpHTTP section below */

        Path = ND->Path[mrmClusterQuery];

        break;

      case mrmWorkloadQuery:

        Path = ND->Path[mrmWorkloadQuery];

        break;

      case mrmJobModify:
      case mrmJobRequeue:
      case mrmJobSuspend:
      case mrmJobResume:
      case mrmJobCheckpoint:
      case mrmXJobValidate:
      default:

        Path = ND->Path[CmdType];
        J    = (mjob_t *)O;

        break;
      }  /* END switch (CmdType) */
    }    /* END if (SPath != NULL) */

  switch (Protocol)
    {
    case mbpNoOp:

      /* don't do anything, just return success */

      MUFree(&mutableSPath);
      return(SUCCESS);

      /*NOTREACHED*/

      break;

    default:
    case mbpExec:

      if ((CmdType == mrmJobStart) ||
          (IsBlocking == FALSE))
        {
        char SubmitExe[MMAX_LINE];

        int  rc;
        int  sc;
 
        char *OBuf = NULL;
        char *EBuf = NULL;

        if ((Path == NULL) || (Path[0] == '\0'))
          {
          MDB(1,fNAT) MLog("ALERT:    invalid parameters in %s (Path is NULL)\n",
            FName);

          if (EMsg != NULL)
            strcpy(EMsg,"invalid path specified");

          if (SC != NULL)
            *SC = mscBadParam;

          MUFree(&mutableSPath);
          return(FAILURE);
          }

        mstring_t ArgBuf(MMAX_LINE);
        mstring_t EnvBuf(MMAX_LINE);

        if ((HostName != NULL) &&
            (HostName[0] != '\0') &&
            (strcmp(MSched.ServerHost,HostName)))
          {
          /* NOTE:  stderr/stdout not merged */

          snprintf(SubmitExe,sizeof(SubmitExe),"%s",
            MNAT_RCMD);

          MStringSetF(&ArgBuf,"%s \"%s\"",
              HostName,
              Path);
          }    /* END if ((HostName != NULL) && ...) */
        else
          {
          /* perform local command exec */

          /* NOTE:  stderr/stdout not merged */
    
          char *ptr;
          char *TokPtr = NULL;

          if ((CmdType == mrmXJobValidate) && (J != NULL))
            {
            MStringSetF(&ArgBuf,"%s",
              J->Name);
            }
            
          ptr = MUStrTok(Path," \t",&TokPtr);

          if (ptr != NULL)
            {
            snprintf(SubmitExe,sizeof(SubmitExe),"%s",
              ptr);
            }

          if ((TokPtr != NULL) &&
              (TokPtr[0] != '\0'))
            {
            MStringSetF(&ArgBuf,"%s",TokPtr);
            }
          }    /* END else ((HostName != NULL) && ...) */

        if (MSched.ServerPath != NULL)
          {
          MStringSetF(&EnvBuf,"PATH=%s%s%s",
            MSched.ServerPath,
            ((R != NULL) && (R->Env != NULL)) ? ENVRS_ENCODED_STR : "",
            ((R != NULL) && (R->Env != NULL)) ? R->Env : "");
          }
        else if ((R != NULL) && (R->Env != NULL))
          {
          MStringSet(&EnvBuf,R->Env);
          }
        else
          {
          MStringSet(&EnvBuf,"");
          }

        if (ND->NatType == mrmstX1E)
          {
          rc = MUSpawnChild(
            SubmitExe,
            NULL,           /* Job Name */
            ArgBuf.c_str(),         /* args */
            NULL,           /*  */
            MSched.UID,     /* UID */
            MSched.GID,     /* GID */
            MSched.CfgDir,  /* cwd */
            EnvBuf.c_str(),         /* env */
            NULL,
            NULL,           /*  */
            NULL,           /* stdin */
            NULL,
            &OBuf,          /* stdout */
            &EBuf,          /* stderr */
            NULL,           /*   */
            &sc,            /* child exit code */
            Timeout,        /* wait time (milliseconds) */
            0,              /*   */
            NULL,           /*   */
            mxoNONE,        /*   */
            FALSE,
            FALSE,
            EMsg);          /* errors */
          }  /* END if ((MSched.EnableJITServiceProvisioning == TRUE) || ...) */
        else
          {
          char SubmitDir[MMAX_LINE];

          mbool_t UseAdminCreds;

          /* NOTE:  allow sites to specify job submit dir */

          switch (CmdType)
            { 
            case mrmJobStart:
            case mrmJobCancel:
            case mrmJobModify:
            case mrmJobRequeue:
            case mrmJobSuspend:
            case mrmJobResume:
            case mrmJobCheckpoint:

              if (ND->JobCtlDir[0] == '\0')
                {
                MJobGetIWD(J,SubmitDir,sizeof(SubmitDir),TRUE);
                }
              else if (ND->JobCtlDir[0] == '\1')
                {
                /* do not set IWD, use Moab home */

                strcpy(SubmitDir,NONE);
                }
              else
                {
                MUStrCpy(SubmitDir,ND->JobCtlDir,sizeof(SubmitDir));
                }

              break;

            default:

              MJobGetIWD(J,SubmitDir,sizeof(SubmitDir),TRUE);

              break;
            }  /* END switch (CmdType) */

          if ((J == NULL) || 
             ((ND != NULL) && (ND->AdminExec[mrmJobStart] == TRUE)))
            {
            UseAdminCreds = TRUE;
            }
          else
            {
            UseAdminCreds = FALSE;
            }

          if (CmdType == mrmJobSubmit)
            {
            /* move job submit exe to "su - <UNAME> -c '[<VAR>=<VAL> ]...<SUBEXE>'" */

            /* NYI */
            }

          rc = MUSpawnChild(
            SubmitExe,
            NULL,           /* job name */
            ArgBuf.c_str(),         /* args */
            NULL,           /*  */
            (UseAdminCreds == FALSE) ? J->Credential.U->OID : MSched.UID, /* UID */
            (UseAdminCreds == FALSE) ? J->Credential.G->OID : MSched.GID, /* GID */
            SubmitDir,      /* cwd */
            EnvBuf.c_str(),         /* env */
            NULL,
            NULL,           /*  */
            NULL,           /* stdin */
            NULL,
            &OBuf,          /* O - stdout */
            &EBuf,          /* O - stderr */
            NULL,           /*   */
            &sc,            /* child exit code */
            Timeout,        /* wait time (ms) */
            0,              /*   */
            NULL,           /*   */
            mxoNONE,        /*   */
            FALSE,
            FALSE,
            EMsg);          /* O - errors */
          }  /* END else ((MSched.EnableJITServiceProvisioning == TRUE) || ...) */

        if (rc == FAILURE)
          {
          MDB(1,fNAT) MLog("ALERT:    could not execute native command (\"%s\")\n",
            EMsg);

          if (SC != NULL)
            *SC = mscRemoteFailure;

          if ((sc == mscTimeout) && (R != NULL))
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"command '%s' timed out after %.2f seconds",
              SubmitExe,
              Timeout / 1000.0);

            MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
            }

          MUFree(&mutableSPath);
          return(FAILURE);
          }

        if (sc != 0)
          {

          /* array jobs may fail if they have a policy limit violation, but we need
             to check to see if they are able to run now */
          mjob_t *JA;
          mjob_t *J = (mjob_t*)O;

          if ((J != NULL) && (bmisset(&J->Flags, mjfArrayJob)))
            {

            /* find master and check limit, active, and idle -- start job if active is less than limit */
            if (MJobFind(J->JGroup->Name,&JA,mjsmExtended) == FAILURE)
            {
            /* no-op */
            }
            else if ((JA->Array != NULL) && 
              (JA->Array->Limit > 0) &&
              (JA->Array->Active < JA->Array->Limit))
              {
              MUFree(&OBuf);
              MUFree(&EBuf);

              MUFree(&mutableSPath);
              return(SUCCESS);
              }
            }
          MDB(0,fRM) MLog("ALERT:    request failed with sc=%d, stderr='%s'  stdout='%s'  EMsg='%s'\n",
            sc,
            (EBuf != NULL) ? EBuf : "",
            (OBuf != NULL) ? OBuf : "",
            (EMsg != NULL) ? EMsg : "");

          if ((EBuf != NULL) && (EBuf[0] != '\0'))
            {
            if (EMsg != NULL)
              MUStrCpy(EMsg,EBuf,MMAX_LINE);
            }
          else if ((OBuf != NULL) && (OBuf[0] != '\0'))
            {
            if (EMsg != NULL)
              MUStrCpy(EMsg,OBuf,MMAX_LINE);
            }
          else
            {
            if (EMsg != NULL)
              {
              snprintf(EMsg,MMAX_LINE,"request failed with rc=%d",
                sc);
              }
            }

          /* NOTE:  see XXX for standard meaning of child exit codes */

          if (sc == 4)
            {
            /* remote service has failed */

            bmset(&R->IFlags,mrmifRMStartRequired);
            }

          if (SC != NULL)
            *SC = mscRemoteFailure;

          MUFree(&EBuf);
          MUFree(&OBuf);

          MUFree(&mutableSPath);
          return(FAILURE);
          }  /* END if (sc != 0) */

        if (OBuf != NULL)
          MStringSet(ResponseP,OBuf);

        if (EBuf != NULL)
          {
          if (strstr(EBuf,"moabpending"))
            {
            if (SC != NULL)
              *SC = mscPending;
            }

          if ((MSched.ChildStdErrCheck == TRUE) &&(strstr(EBuf,"ERROR") != NULL))
            {
              MDB(2,fSTRUCT) MLog("INFO:     failure in %s detected by the presence of "
                  "ERROR in stderr, '%s'\n",
                SubmitExe,
                EBuf);

            if (SC != NULL)
              *SC = mscRemoteFailure;

            MUFree(&mutableSPath);
            return(FAILURE);
            }
          }

        if ((OBuf == NULL) || (OBuf[0] == '\0'))
          {
          if ((EBuf != NULL) && (EBuf[0] != '\0'))
            {
            MDB(5,fRM) MLog("WARNING:  request succeeded with no stdout but stderr='%s'\n",
              (EBuf != NULL) ? EBuf : "");
            }
          }

        MUFree(&OBuf);
        MUFree(&EBuf);

        MUFree(&mutableSPath);
        return(SUCCESS);
        }  /* END if ((CmdType == mrmJobStart) || ...) */
      else
        {
        /* used for all non-job start URL's */

        if (R == NULL)
          {
          MDB(1,fNAT) MLog("ALERT:    invalid parameters in %s (RM is NULL)\n",
            FName);

          if (EMsg != NULL)
            strcpy(EMsg,"invalid RM specified");

          MUFree(&mutableSPath);
          return(FAILURE);
          }

        if ((Path == NULL) || (Path[0] == '\0'))
          {
          MDB(1,fNAT) MLog("ALERT:    invalid parameters in %s (Path is NULL)\n",
            FName);

          if (EMsg != NULL)
            strcpy(EMsg,"invalid path specified");

          MUFree(&mutableSPath);
          return(FAILURE);
          }

        mstring_t tmpBuf(MMAX_LINE);

        if ((HostName != NULL) &&
            (HostName[0] != '\0') &&
            (strcmp(MSched.ServerHost,HostName)))
          {
          /* NOTE:  stderr/stdout not merged */

          MStringSetF(&tmpBuf,"%s %s \"%s\"",
            MNAT_RCMD,
            HostName,
            Path);
          }
        else
          {
          /* NOTE:  stderr/stdout not merged */

          switch (CmdType)
            {
            case mrmJobCancel:

              /* job arg applied externally */

              MStringSet(&tmpBuf,Path);

              break;

            default:

              /* apply jobid as first arg */

              MStringSetF(&tmpBuf,"%s%s%s",  
                Path,
                (J == NULL) ? "" : " ",
                (J == NULL) ? "" : J->Name);

              break;
            }  /* END switch (CmdType) */
          }    /* END else ((HostName != NULL) && ...) */

        /* NOTE:  EnvBuf required (FIXME) */

        mstring_t Response(MMAX_LINE);
        mstring_t EBuf(MMAX_LINE);

        if (MUReadPipe2(
              tmpBuf.c_str(),  /* I */
              NULL,
              &Response,       /* O */
              &EBuf,           /* O */
              &R->P[0],
              &rc,
              EMsg,            /* O */
              SC) == FAILURE)  /* O */
          {
          if (rc == 0)
            {
            MDB(1,fNAT) MLog("ALERT:    cannot read output of command '%s'\n",
              tmpBuf.c_str());

            if ((EMsg != NULL) && (EMsg[0] == '\0'))
              {
              if (!EBuf.empty())
                MUStrCpy(EMsg,EBuf.c_str(),MMAX_LINE);
              else
                strcpy(EMsg,"cannot read output of command");
              }
            }
          else
            {
            MDB(1,fNAT) MLog("ALERT:    command '%s' failed with status code %d\n",
              tmpBuf.c_str(),
              rc);

            if ((EMsg != NULL) && (EMsg[0] == '\0'))
              {
              if (!EBuf.empty())
                MUStrCpy(EMsg,EBuf.c_str(),MMAX_LINE);
              else
                snprintf(EMsg,MMAX_LINE,"command '%s' failed with status code %d\n",
                  tmpBuf.c_str(),
                  rc);
              }
            }

          if ((SC != NULL) && (*SC == mscTimeout) && (R != NULL))
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"command '%s' timed out after %.2f seconds",
              tmpBuf.c_str(),
              (R->P[0].Timeout > 1000) ? R->P[0].Timeout / 1000.0 : R->P[0].Timeout);

            MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
            }

          MUFree(&mutableSPath);
          return(FAILURE);
          } /* END MUReadPipe2(...) == FAILURE */

        *ResponseP = Response;

        if ((SC != NULL) && ((*SC == mscRemoteFailure) || (*SC == mscTimeout)))
          {
          /* command didn't return successfully */

          if ((EMsg != NULL) && (EMsg[0] == '\0'))
            {
            if (!EBuf.empty())
              MUStrCpy(EMsg,EBuf.c_str(),MMAX_LINE);
            else
              snprintf(EMsg,MMAX_LINE,"command '%s' failed",
                tmpBuf.c_str());
            }

          MDB(5,fRM) MLog("ALERT:    request failed with SC=%d and stderr='%s'\n",
            *SC,
            (!EBuf.empty()) ? EBuf.c_str() : "");

          MUFree(&mutableSPath);
          return(FAILURE);
          }

        if (ResponseP->empty())
          {
          if (!EBuf.empty())
            {
            MDB(5,fRM) MLog("WARNING:  request succeeded with no stdout but stderr='%s'\n",
              (!EBuf.empty()) ? EBuf.c_str() : "");
            }
          }

        if (strstr(EBuf.c_str(),"moabpending"))
          {
          if (SC != NULL)
            *SC = mscPending;
          }

        if ((EMsg != NULL) && (!EBuf.empty()))
          MUStrCpy(EMsg,EBuf.c_str(),MMAX_LINE);
        }   /* END else ((CmdType == mrmJobStart) || ...) */

      break;

    case mbpFile:

      {
      int   tSC = 0;

      char *ptr;

      if (Path == NULL)
        {
        MDB(1,fNAT) MLog("ALERT:    invalid parameters in %s (path is null)\n",
          FName);

        MUFree(&mutableSPath);
        return(FAILURE);
        }

      if ((ptr = MFULoadNoCache(Path,1,NULL,NULL,NULL,NULL)) == NULL)
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot load nat file '%s'\n",
          Path);

        if (EMsg != NULL)
          {
          if ((tSC == (int)mscNoEnt) || (tSC == (int)mscNoFile))
            {
            snprintf(EMsg,MMAX_LINE,"file '%s' does not exist",
              Path);
            }
          else if (tSC == (int)mscSysFailure)
            {
            strcpy(EMsg,"cannot load file data - check path/permissions");
            }
          else
            {
            strcpy(EMsg,"cannot load file data - internal failure");
            }
          }

        MUFree(&mutableSPath);
        return(FAILURE);
        }

      if (strstr(ptr,"moabpending"))
        {
        if (SC != NULL)
          *SC = mscPending;
        }

      MStringSet(ResponseP,ptr);

      free(ptr);
      }    /* END BLOCK */

      break;

    case mbpGanglia:

      if (__MNatClusterLoadGanglia(ND,ResponseP,EMsg) == FAILURE)
        {
        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          snprintf(EMsg,MMAX_LINE,"ganglia query failed");
                       
        MUFree(&mutableSPath);
        return(FAILURE);
        }

      break;

    case mbpHTTP:

      {
      int tmpI;

      tmpI = 0;

      if ((ND != NULL) && (ND->Port[CmdType] > 0))
        {
        /* apply function-specific port */

        tmpI = ND->Port[CmdType];
        }
      else 
        {
        /* apply RM-specific default port */

        switch (CmdType)
          {
          case mrmResourceQuery:

            if ((ND != NULL) && (ND->ServerPort > 0))
              tmpI = ND->ServerPort;
            else if ((ND != NULL) && (ND->Port[mrmClusterQuery] > 0))
              tmpI = ND->Port[mrmClusterQuery];

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (CmdType) */
        }

      if (tmpI == 0)
        tmpI = MDEF_MHPORT;

      /* Check if there is a key to send across */

      mstring_t tmpPath(MMAX_LINE); /* may include key=<key> */

      tmpPath = Path;

      if (R->P[0].CSKey != NULL)
        {
        MStringAppendF(&tmpPath,"?key=%s",R->P[0].CSKey);
        }

      if (MSysHTTPGet(
            HostName,
            tmpI,
            tmpPath.c_str(),
            (R != NULL) ? (long)R->P[0].Timeout : -1,
            ResponseP) == FAILURE)
        {
        MUFree(&mutableSPath);
        return(FAILURE);
        }

      /* NOTE: if string is found anywhere in response, the request is not complete */

      if (strstr(ResponseP->c_str(),"moabpending"))
        {
        if (SC != NULL)
          *SC = mscPending;
        }
      }    /* END BLOCK (cae mbpHTTP) */

      break;
    }  /* END switch (Protocol) */

  MUFree(&mutableSPath);
  return(SUCCESS);
  }  /* END MNatDoCommand() */





/**
 * Process workload reported via native interface.
 *
 * Step 1.0  Initialize
 * Step 2.0  Update reported workload
 * Step 3.0  Update non-reported workload
 *  if completed, process completed jobs
 *  if not completed, issue MRMJobPostUpdate()
 *
 * @see MRMWorkloadQuery() - parent
 * @see MNatClusterQuery() - peer
 * @see __MNatWorkloadProcessData() - child
 * @see MNatGetData() - peer - load raw workload data from RM
 *
 * @param R (I)
 * @param WCount (O) [optional]
 * @param NWCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatWorkloadQuery(

  mrm_t                *R,       /* I */
  int                  *WCount,  /* O (optional) */
  int                  *NWCount, /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  mjob_t  *J;

  mbool_t  DoUpdate;

  mln_t *JobsComletedList = NULL;
  mln_t *JobsRemovedList = NULL;
  mln_t *JobListPtr = NULL;

  enum MJobStateEnum OldState;

  const char *FName = "MNatWorkloadQuery";

  MDB(1,fNAT) MLog("%s(%s,WCount,NWCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  /* Step 1.0  Initialize */

  if (WCount != NULL)
    *WCount = 0;

  if (NWCount != NULL)
    *NWCount = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((R == NULL) || (R->S == NULL))
    {
    return(FAILURE);
    }

  /* determine default ND structure */

  if (R->ND.URL[mrmWorkloadQuery] == NULL)
    {
    MDB(1,fNAT) MLog("INFO:     workload query action not specified for native interface %s (disabling function)\n",
      R->Name);

    if (bmisclear(&R->FnList))
      {
      MRMFuncSetAll(R);
      }

    bmunset(&R->FnList,mrmWorkloadQuery);

    return(SUCCESS);
    }

  if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, skipping workload query\n",
      R->Name);

    if (EMsg != NULL)
      {
      sprintf(EMsg,"RM in state %s",
        MRMState[R->State]);
      }

    return(FAILURE);
    }

  if ((R->ND.WorkloadQueryData.empty()) || 
      (R->WorkloadUpdateIteration != MSched.Iteration))
    {
    /* updated workload info not available */

    MDB(1,fNAT) MLog("INFO:     no workload query information available for RM %s on iteration %d\n",
      R->Name,
      MSched.Iteration);

    /* might not be any new information but there could be some stale jobs to clean up so
     * skip to Step 3.0. */
    }
  else
    {
    /* Step 2.0  Process Reported Workload Data */

    /* buffer successfully loaded */

    /* NOTE:  operation on 'mutableString' is destructive */

    char *mutableString = NULL;
    MUStrDup(&mutableString,R->ND.WorkloadQueryData.c_str());

    __MNatWorkloadProcessData(
      R,
      &R->ND,
      mutableString,
      WCount,
      NWCount);

    R->ND.WorkloadQueryData = mutableString;
    MUFree(&mutableString);

    R->WorkloadUpdateIteration = MSched.Iteration;
    }

  /* Step 3.0  Update Stale Workload Objects */

  if (!bmisset(&R->Flags,mrmfNoCreateAll))
    {
    job_iter JTI;

    MULLCreate(&JobsComletedList);
    MULLCreate(&JobsRemovedList);

    MJobIterInit(&JTI);

    /* clean completed jobs */

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if (J->DestinationRM != R)
        continue;

      if (J->UIteration >= MSched.Iteration)
        {
        /* Do not purge jobs if reported this iteration */
        continue;
        }

      switch (R->SubType)
        {
        case mrmstXT3:
        case mrmstXT4:
        case mrmstX1E:

          /* RM is reliable */
          /*NO-OP */

          break;

        default:

          /* RM may be unreliable, do not purge jobs until JOBPURGETIME has
             expired */

          DoUpdate = FALSE;

          if ((J->ATime <= 0) ||
             ((MSched.Time - J->ATime) <= MSched.JobPurgeTime))
            {
            /* purge time not reached */

            DoUpdate = TRUE;
            }

          if (DoUpdate == TRUE)
            {
            MDB(4,fNAT) MLog("INFO:     updating stale native job %s\n",
              J->Name);

            MRMJobPreUpdate(J);
            MRMJobPostUpdate(J,NULL,J->State,R);

            continue;
            }

          break;
        }  /* END switch (R->SubType) */

      if (MJOBISACTIVE(J) == TRUE)
        {
        MDB(1,fNAT) MLog("INFO:     active NATIVE job %s has been removed from the queue.  assuming successful completion\n",
          J->Name);

        MRMJobPreUpdate(J);

        /* remove job from MAQ */

        /* NYI */

        /* assume job completed successfully for now */

        OldState          = J->State;

        J->State          = mjsCompleted;

        J->CompletionTime = J->ATime;

        MRMJobPostUpdate(J,NULL,OldState,J->SubmitRM);

        MJobSetState(J,mjsCompleted);
        if (MSDProcessStageOutReqs(J) == FAILURE)
          {
          MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"data stage-out failed");

          continue;
          }

        /* add job to completed list to be removed after exiting this loop */

        MULLAdd(&JobsComletedList,J->Name,(void *)J,NULL,NULL);
        }  /* END if (MJOBISACTIVE(J) == TRUE) */
      else
        {
        MDB(1,fNAT) MLog("INFO:     non-active NATIVE job %s has been removed from the queue - assuming job was cancelled\n",
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
    }        /* END if (!MISSET(R->Flags,mrmfNoCreateAll)) */

  return(SUCCESS);
  }  /* END MNatWorkloadQuery() */





/**
 * Pre-process a job before updating it.
 *
 * @param J  (I) (may be modified)
 * @param RM (I)
 */

int MNatJobPreUpdate(

  mjob_t *J,
  mrm_t  *RM)

  {
  const char *FName = "MNatJobPreUpdate";

  MDB(2,fNAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name: "NULL");

  if (J == NULL)
    {
    return FAILURE;
    }

  if (RM->SubType == mrmstSGE)
    {
    /* we want to make sure that AdvRsv is not set in the job, so if it has 
     * been removed (via qalter, or modifying the job file) it will stay 
     * removed and not get put back in here */

    MUFree(&J->ReqRID);

    if (bmisset(&J->Flags,mjfAdvRsv))
      {
      bmunset(&J->Flags,mjfAdvRsv);
      bmunset(&J->SpecFlags,mjfAdvRsv);
      }
    } /* END if(RM->SubType == mrmstSGE */

  return SUCCESS;
  } /* END MNatJobPreUpdate */






/**
 * Submit job using native RM interface.
 *
 * @see MRMJobSubmit() - parent
 *
 * NOTE:  Does not call MNatDoCommand() - should this be changed?
 *
 * This function fails if it finds the string "ERROR" in stdout of the called
 * script, which is unique to MNatJobSubmit.
 *
 * @param SubmitString (I) [optional]
 * @param R (I)
 * @param JP (O) [optional]
 * @param FlagBM (I) [bitmap of mjsuf*]
 * @param JobID (O) [optional,minsize=MMAX_NAME]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobSubmit(

  const char           *SubmitString,  /* I (optional) */
  mrm_t                *R,             /* I */
  mjob_t              **JP,            /* O (optional) */
  mbitmap_t            *FlagBM,        /* I (bitmap of mjsuf*) */
  char                 *JobID,         /* O (optional,minsize=MMAX_NAME) */
  char                 *EMsg,          /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)            /* O (optional) */

  {
  int     rc;
  int childSC;

  mjob_t *J;

  char   *BPtr;
  int     BSpace;

  char    SPath[MMAX_BUFFER];
  char    SubmitDir[MMAX_LINE];
  char    tmpLine[MMAX_LINE];

  int     fd;

  int     index;

  mbool_t GRes = FALSE;

  char    CmdFile[MMAX_LINE];
  char    SpoolDir[MMAX_LINE];
  char    CommandBuffer[MMAX_BUFFER];

  const char *FName = "MNatJobSubmit";

  MDB(1,fNAT) MLog("%s(%.64s,%s,JP,%ld,JobID,%s,SC)\n",
    FName,
    (SubmitString != NULL) ? SubmitString : "NULL",
    (R != NULL) ? R->Name : "NULL",
    FlagBM,
    (EMsg != NULL) ? "EMsg" : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (JobID != NULL)
    JobID[0] = '\0';

  if ((JP == NULL) || (R == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error - bad parameters");

    return(FAILURE);
    }

  if (R->ND.URL[mrmJobSubmit] == NULL)
    {
    if (EMsg != NULL)
      sprintf(EMsg,"no submit URL specified for RM %s",
        R->Name);

    return(FAILURE);
    }

  if (R->ND.Protocol[mrmJobSubmit] == mbpNONE)
    {
    if (EMsg != NULL)
      sprintf(EMsg,"specified submit URL is invalid for RM %s",
        R->Name);

    return(FAILURE);
    }

  J  = *JP;

  if (!MJobPtrIsValid(J))
    {
    return(FAILURE);
    }

  CmdFile[0] = '\0';

  if ((SubmitString != NULL) || (J->RMSubmitString != NULL))
    {
    /* create new cmd file */

    if (SubmitString != NULL)
      MUStrCpy(CommandBuffer,SubmitString,sizeof(CommandBuffer));
    else
      MUStrCpy(CommandBuffer,J->RMSubmitString,sizeof(CommandBuffer));

    /* create submission command */

    MUGetSchedSpoolDir(SpoolDir,sizeof(SpoolDir));

    errno = 0;

    snprintf(CmdFile,sizeof(CmdFile),"%s/%s",
      SpoolDir,
      "moab.job.XXXXXX");

    if ((fd = mkstemp(CmdFile)) < 0)
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"cannot create temp file in spool directory '%s' (errno: %d, '%s')",
          SpoolDir,
          errno,
          strerror(errno));
        }

      if (SC != NULL)
        *SC = mscNoFile;

      return(FAILURE);
      }

    close(fd);

    if (MFUCreate(
         CmdFile,
         NULL,
         CommandBuffer,
         strlen(CommandBuffer),
         S_IXUSR|S_IRUSR,
         J->Credential.U->OID,
         J->Credential.G->OID,
         TRUE,
         NULL) == FAILURE)
      {
      MFURemove(CmdFile);

      if (SC != NULL)
        *SC = mscNoFile;

      return(FAILURE);
      }

    MUStrDup(&J->Env.Cmd,CmdFile);
    }  /* END if ((SubmitString != NULL) || (J->RMSubmitString != NULL)) */

  /* translate job into WIKI format job description */

  MUSNInit(&BPtr,&BSpace,SPath,sizeof(SPath));

  /* NOTE:  user, group, and account names may potentially contain whitespace */

  MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
    MWikiJobAttr[mwjaUName],
    J->Credential.U->Name);

  MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
    MWikiJobAttr[mwjaName],
    J->Name);

  if (J->Credential.G != NULL)
    {
    if (strchr(J->Credential.G->Name,' ') != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s=\"%s\" ",
        MWikiJobAttr[mwjaGName],
        J->Credential.G->Name);
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
        MWikiJobAttr[mwjaGName],
        J->Credential.G->Name);
      }
    }

  if (J->SpecWCLimit[0] > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%ld ",
      MWikiJobAttr[mwjaWCLimit],
      J->SpecWCLimit[0]);
    }

  if (J->Request.TC > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%d ",
      MWikiJobAttr[mwjaTasks],
      J->Request.TC);
    }

  if (J->AName != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaJName],
      J->AName);
    }

  if (J->Credential.C != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaRClass],
      J->Credential.C->Name);
    }

  if (J->Credential.A != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaAccount],
      J->Credential.A->Name);
    }

  if ((J->TaskMap != NULL) && (J->TaskMap[0] != -1))
    {
    char NodeList[MMAX_BUFFER];

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaTaskList],
      MUTaskMapToString(J->TaskMap,NULL,'\0',NodeList,sizeof(NodeList)));
    }

  if (J->Env.Shell != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaShell],
      J->Env.Shell);
    }

  if (J->TSets != NULL)
    {
    char TemplateString[MMAX_LINE];

    MULLToString(J->TSets,FALSE,NULL,TemplateString,sizeof(TemplateString));

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaTemplate],
      TemplateString);
    }

  if (J->RMSubmitFlags != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      "RMFLAGS",
      J->RMSubmitFlags);
    }

  if (R->SubType == mrmstSGE)
    {
    if ((J->RMSubmitString != NULL) && (J->RMSubmitString[0] != '\0'))
      {
      char tmpLine[MMAX_LINE];

      if (MJobPBSExtractArg(
            "-l",
            "nodes",
            J->RMSubmitString,
            tmpLine,
            sizeof(tmpLine),
            FALSE) == SUCCESS)
        {
        MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
          "REQUEST",
          tmpLine);
        }
      }
    }

  if ((J->SystemJID != NULL) &&
      (J->SystemID != NULL) &&
      (J->SRMJID != NULL))
    {
    /* should this call MRMXToString() */

    MUSNPrintF(&BPtr,&BSpace,"%s=\"SID=%s?SJID=%s?SRMJID=%s\" " ,
      MWikiJobAttr[mwjaComment],
      J->SystemID,
      J->SystemJID,
      J->SRMJID);
    }

  if (!MNLIsEmpty(&J->ReqHList))
    {
    char tmpLine[MMAX_BUFFER];

    MNLToString(&J->ReqHList,FALSE,",",'\0',tmpLine,sizeof(tmpLine));

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaHostList],
      tmpLine);
    }

  MJobGetIWD(
    J,
    SubmitDir,
    sizeof(SubmitDir),
    !bmisset(&R->IFlags,mrmifNoExpandPath));

  MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
    MWikiJobAttr[mwjaIWD],
    SubmitDir);

  if (J->Env.Input != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaInput],
      J->Env.Input);
    }

  if (J->Env.Output != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaOutput],
      J->Env.Output);
    }

  if (J->Env.Error != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaError],
      J->Env.Error);
    }

  if (!bmisclear(&J->AttrBM))
    {
    mstring_t tmp(MMAX_LINE);;

    MJobAToMString(J,mjaGAttr,&tmp);

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaGAttr],
      tmp.c_str());
    }

  if (!bmisclear(&J->Req[0]->ReqFBM))
    {
    char tmpLine[MMAX_LINE];

    MUNodeFeaturesToString(',',&J->Req[0]->ReqFBM,tmpLine);

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaRFeatures],
      tmpLine);
    }

  /* display generic resources */

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    { 
    if (MGRes.Name[index][0] == '\0')
      break;

    if (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,index) == 0)
      continue;

    if (GRes == FALSE)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s=%s:%d",
        MWikiJobAttr[mwjaDGRes],
        MGRes.Name[index],
        MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,index));

      GRes = TRUE;
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,",%s:%d",
        MGRes.Name[index],
        MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,index));
      }
    }  /* END for (index) */

  if (GRes == TRUE)
    MUSNCat(&BPtr,&BSpace," ");

  if (J->Variables.Table != NULL)
    {
    mstring_t tmp(MMAX_LINE);

    MJobAToMString(J,mjaVariables,&tmp);

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaVariables],
      tmp.c_str());
    }
  else if (R->SubType == mrmstXT3)
    {
    char tmpLine[MMAX_LINE];

    /* FORMAT:  VARIABLE=BATCH_TUPLE:<PROCS>:<MEM>:<FEATURES> */

    MUNodeFeaturesToString(':',&J->Req[0]->ReqFBM,tmpLine);

    MUSNPrintF(&BPtr,&BSpace,"%s=%s=%d:%d:%s ",
      MWikiJobAttr[mwjaVariables],
      "BATCH_TUPLE",
      J->Req[0]->TaskCount * J->Req[0]->DRes.Procs,
      J->Req[0]->DRes.Mem >> 10,  /* report in GB */
      tmpLine);
    }

  /* NOTE:  ARGS=<VALUE>[ <VALUE>]... must be last in list of attrs! */

  if (J->Env.Cmd != NULL)
    {
    char *ptr;
    char *TokPtr;

    mbool_t IsExe;

    /* FORMAT:  <CMD>=<FULLEXECPATH>[ <ARG>][ <ARG>]... */

    MUStrCpy(tmpLine,J->Env.Cmd,sizeof(tmpLine));

    ptr = MUStrTok(tmpLine," ",&TokPtr);

    MUStringChop(ptr);

    if ((MFUGetInfo(ptr,NULL,NULL,&IsExe,NULL) == SUCCESS) ||
        (IsExe == TRUE) ||
        (bmisset(&R->Flags,mrmfFSIsRemote)))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
        MWikiJobAttr[mwjaExec],
        ptr);

      if ((TokPtr != NULL) && (TokPtr[0] != '\0'))
        {
        MUSNPrintF(&BPtr,&BSpace,"%s=%s %s",
          MWikiJobAttr[mwjaArgs],
          TokPtr,
          (J->Env.Args != NULL) ? J->Env.Args : "");
        }
      else if (J->Env.Args != NULL)
        {
        /* Need to quote, args may have spaces, messes up parsing later */

        MUSNPrintF(&BPtr,&BSpace,"%s=\"%s\" ",
          MWikiJobAttr[mwjaArgs],
          J->Env.Args);
        }
      }    /* END if ((MFUGetInfo(ptr,NULL,NULL,&IsExe,NULL) == SUCCESS) || ..) */
    else
      {
      MDB(1,fNAT) MLog("ALERT:    executable '%s' for job '%s' is invalid\n",
        J->Env.Cmd,
        J->Name);

      /* RMSubmitString not specified */

      if (EMsg != NULL)
        {
        if (J->Env.Cmd[0] == '\0')
          {
          sprintf(EMsg,"no executable specified for job");
          }
        else
          {
          snprintf(EMsg,MMAX_LINE,"invalid executable '%s' specified",
            J->Env.Cmd);
          }
        }

      return(FAILURE);
      }
    }    /* END if (J->E.Cmd != NULL) */

  if (R->ND.Protocol[mrmJobSubmit] == mbpExec)
    {
    char *OBuf = NULL;
    char *EBuf = NULL;
    char *IBuf = NULL;

    if (R->ND.Path[mrmJobSubmit] == NULL)
      {
      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"submit path not specified");
        }

      return(FAILURE);
      }

    if (((J->Credential.U->OID <= 0) || 
         (J->Credential.G->OID <= 0)) && 
        (MSched.AllowRootJobs == FALSE))
      {
      /* NOTE:  this check is already enforced in MJobMigrate() - is there any
                reason to maintain this check here? */

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot submit NATIVE jobs as root");
        }

      return(FAILURE);
      }

    /* submit job in user environment */

    mstring_t EnvBuf(MMAX_LINE);

    if (J->Env.BaseEnv != NULL)
      {
      MStringSet(&EnvBuf,J->Env.BaseEnv);
      }

    if (R->Env != NULL)
      {
      if (EnvBuf.empty())
        {
        MStringSet(&EnvBuf,R->Env);
        }
      else
        {
        MStringAppendF(&EnvBuf,ENVRS_ENCODED_STR "%s",
          R->Env);
        }
      }    /* END if (R->Env != NULL) */

    if (J->Env.IncrEnv != NULL)
      {
      /* override env should be inserted into jobsubmit exe */

      /* NYI */
      }

    if (bmisset(&R->Flags,mrmfRootSubmit))
      {
      mstring_t SubmitCmd(MMAX_LINE);

      /* launch w/admin credentials */

       if (bmisset(&R->Flags,mrmfRootSubmit))
         {
         /* should 'su - <UNAME> -c <CMD>' be used? */
         }

      SubmitCmd = R->ND.Path[mrmJobSubmit];
    
      rc = MUSpawnChild(
        SubmitCmd.c_str(),
        J->Name, 
        SPath,          /* args */
        NULL,
        MSched.UID,
        MSched.GID,
        MSched.CfgDir,  /* cwd */
        EnvBuf.c_str(),
        NULL,
        NULL,
        IBuf,           /* I - stdin */
        NULL,           /* O */
        &OBuf,          /* O */
        &EBuf,          /* O */
        NULL,
        &childSC,
        100000,
        0,
        NULL,
        mxoNONE,
        FALSE,
        FALSE,
        EMsg);          /* O */
      }  /* END if (MSched.EnableJITServiceProvisioning == TRUE) */
    else
      {
      mbool_t UseAdminCreds;
      char    CWD[MMAX_LINE];

      if (R->ND.AdminExec[mrmJobSubmit] == TRUE)
        UseAdminCreds = TRUE;
      else
        UseAdminCreds = FALSE;

      if ((J->Env.IWD != NULL) && (!strcmp("$HOME",J->Env.IWD)))
        {
        MUStrCpy(CWD,J->Credential.U->HomeDir,sizeof(CWD));
        }
      else
        {
        MUStrCpy(CWD,J->Env.IWD,sizeof(CWD));
        }

      /* launch with job credentials */

      rc = MUSpawnChild(
        R->ND.Path[mrmJobSubmit],
        J->Name, 
        SPath,     /* args */
        NULL,
        (UseAdminCreds == TRUE) ? MSched.UID : J->Credential.U->OID,
        (UseAdminCreds == TRUE) ? MSched.GID : J->Credential.G->OID,
        (!bmisset(&R->Flags,mrmfFSIsRemote)) ? CWD : NULL,
        EnvBuf.c_str(),
        NULL,
        NULL,
        IBuf,      /* I - stdin */
        NULL,
        &OBuf,     /* O */
        &EBuf,     /* O */
        NULL,
        &childSC,
        100000,
        0,
        NULL,
        mxoNONE,
        FALSE,
        FALSE,
        EMsg);     /* O */
      }  /* END else (MSched.EnableJITServiceProvisioning == TRUE) */

    if (rc == FAILURE)
      {
      MDB(1,fRM) MLog("ALERT:    Execution of RM %s job submit script %s failed (%s)\n",
        R->Name,
        R->ND.Path[mrmJobSubmit],
        (EMsg != NULL) ? EMsg : "NULL");

      return(FAILURE); 
      }

    if ((EBuf != NULL) && (EBuf[0] != '\0'))
      {
      if (EMsg != NULL)
        MUStrCpy(EMsg,EBuf,MMAX_LINE);
  
      MDB(1,fRM) MLog("ALERT:    job start attempt produced stderr message '%s'\n",
        (EBuf != NULL) ? EBuf : "NULL");
                                                                              
      MUFree(&EBuf);
      MUFree(&OBuf);

      if (CmdFile[0] != '\0')
        MFURemove(CmdFile);
                                                                                
      return(FAILURE);
      }
    else if (childSC != 0)
      {
      char tmpErr[MMAX_LINE];

      snprintf(tmpErr,sizeof(tmpErr),"RM %s job submit script %s failed with "
        "exit status %d\n",
        R->Name,
        R->ND.Path[mrmJobSubmit],
        childSC);

      MDB(1,fRM) MLog("%s",tmpErr);

      if (EMsg != NULL)
        MUStrCpy(EMsg,tmpErr,MMAX_LINE);

      MUFree(&EBuf);
      MUFree(&OBuf);

      if (CmdFile[0] != '\0')
        MFURemove(CmdFile);

      return(FAILURE);
      }

    if ((OBuf == NULL) || (OBuf[0] == '\0') || strstr(OBuf,"ERROR"))
      {
      MDB(1,fRM) MLog("ALERT:    no job ID detected in job submit - stdout/stderr=%s/%s\n",
        (OBuf != NULL) ? OBuf : "NULL",
        (EBuf != NULL) ? EBuf : "NULL");
                                                                                
      if ((EBuf != NULL) && (EBuf[0] != '\0'))
        {
        if (!strcmp(EBuf,"cannot set GID"))
          {
          strcpy(EMsg,"cannot launch command - no permission to change UID");
          }
        else
          {
          MUStrCpy(EMsg,EBuf,MMAX_LINE);
          }
        }
      else if (EMsg[0] == '\0')
        {
        strcpy(EMsg,"unknown failure - no job ID detected");
        }
      
      MUStrCpy(JobID,J->Name,MMAX_NAME);
                                                                          
      /* return(FAILURE);*/
      }
    else
      {                                                                                
      MUStrCpy(JobID,OBuf,MMAX_NAME);
      }

    MUStringChop(JobID);

    MUFree(&EBuf);
    MUFree(&OBuf);
    }  /* END if (R->ND.Protocol[mrmJobSubmit] == mbpExec) */
  else
    {
    char CMDPath[MMAX_LINE];

    mstring_t Response(MMAX_LINE);

    snprintf(CMDPath,sizeof(CMDPath),"%s %s",
      R->ND.Path[mrmJobSubmit],
      SPath);

    rc = MNatDoCommand(
       &R->ND,
       NULL,
       mrmJobSubmit,
       R->ND.Protocol[mrmJobSubmit],
       TRUE,
       R->ND.Host[mrmJobSubmit],
       CMDPath,            /* I (job description) */
       &Response,          /* O */
       EMsg,
       NULL);              /* O */

    if (rc == SUCCESS)
      MUStrCpy(JobID,Response.c_str(),MMAX_NAME);

    if (rc == FAILURE)
      {
      if (CmdFile[0] != '\0')
        MFURemove(CmdFile);

      MDB(1,fRM) MLog("ALERT:    Execution of RM %s job submit script %s failed (%s)\n",
        R->Name,
        R->ND.Path[mrmJobSubmit],
        (EMsg != NULL) ? EMsg : "NULL");

      return(FAILURE); 
      }
    }

  if (CmdFile[0] != '\0')
    MFURemove(CmdFile);

  /* NOTE:  job will show up on next job query */

  MDB(1,fNAT) MLog("INFO:     successfully submitted job %s\n",
    JobID);

  return(SUCCESS);
  }  /* END MNatJobSubmit() */





/**
 * Start job via native interface
 *
 * @see MRMJobStart() - parent
 *
 * @param J (I)
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobStart(

  mjob_t               *J,    /* I */
  mrm_t                *R,    /* I */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  const char *FName = "MNatJobStart";

  char ExeHost[MMAX_NAME];

  enum MBaseProtocolEnum Protocol;
 
  MDB(1,fNAT) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) || (R == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error - bad parameters");

    return(FAILURE);
    }

  if ((J->TaskMap == NULL) || (MNLIsEmpty(&J->NodeList)))
    {
    /* invalid host list */

    if (EMsg != NULL)
      strcpy(EMsg,"invalid hostlist");

    return(FAILURE);
    }

  /* NOTE:  pass generic job attributes down to RM */

  /* if RM->IsVirtual == TRUE, don't do job start */

  mstring_t SPath(MMAX_LINE);

  /* FORMAT:  <STARTCMD> <JOBID> <NODELIST> <USER> [<ANAME>] <FLAGS> */

  mstring_t NodeList(MMAX_LINE);

  if (bmisset(&R->Flags,mrmfCompressHostList))
    {
    /* NOTE:  only first node required but compress full hostlist anyway for now */

    MUNLToRangeString(
      &J->NodeList,
      NULL,
      -1,
      TRUE,
      TRUE,
      &NodeList);
    }
  else
    {
    if (MJobIsMultiRM(J) == TRUE)
      {
      MJobRMNLToString(J,R,mrmrtNONE,&NodeList);
      }
    else
      {
      char tmpBuf[MMAX_BUFFER];

      MUTaskMapToString(J->TaskMap,NULL,'\0',tmpBuf,sizeof(tmpBuf));

      MStringSetF(&NodeList,"%s",tmpBuf);
      }
    }    /* END else */

  if (R->ND.Path[mrmJobStart] == NULL)
    {
    /* no start URL specified */

    if (EMsg != NULL)
      strcpy(EMsg,"no start URL specified");

    return(FAILURE);
    }

  if (J->DRMJID != NULL)
    {
    MStringAppendF(&SPath,"%s %s %s %s",
      R->ND.Path[mrmJobStart],
      J->DRMJID,
      NodeList.c_str(),
      J->Credential.U->Name);
    }
  else
    {
    /* FORMAT:  <PATH> <JOBID> <NODE>[,<NODE>]... <USER> */

    MStringAppendF(&SPath,"%s %s %s %s",
      R->ND.Path[mrmJobStart],
      J->Name,
      NodeList.c_str(),
      J->Credential.U->Name);
    }

  if (bmisset(&R->RTypes,mrmrtNetwork))
    {
    mstring_t tmpNodeList(MMAX_LINE);

    /* add network RM meta data */

    MJobRMNLToString(J,NULL,mrmrtCompute,&tmpNodeList);

    MStringAppendF(&SPath," --computenodes=%s",
      tmpNodeList.c_str());
    }

  switch (R->SubType)
    {
    case mrmstXT4:

      MStringAppendF(&SPath," --rm %s",
        R->Name);

      break;

    default:

      /* NO-OP */

      break;
    }

#ifdef MYAHOO
  if (J->AName != NULL)
    {
    MStringAppend(&SPath," ");
    MStringAppend(&SPath,J->AName);
    }
#endif /* MYAHOO */

  if (R->SubType == mrmstXT3)
    {
    char tmpLine[MMAX_LINE];

    int rqindex;

    mreq_t *RQ;

    MStringAppendF(&SPath," %s=",
      MWikiJobAttr[mwjaVariables]);

    for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
      {
      /* FORMAT:  VARIABLE={BATCH_TUPLE<%d>=<PROCS>:<MEM>:<FEATURES>}[;...] */

      RQ = J->Req[rqindex];

      MUNodeFeaturesToString(':',&RQ->ReqFBM,tmpLine);

      MStringAppendF(&SPath,"%s%s%d=%d:%d:%s",
        (RQ->Index > 0) ? ";" : "",
        "BATCH_TUPLE",
        RQ->Index,
        RQ->TaskCount * RQ->DRes.Procs,
        (RQ->DRes.Mem / RQ->DRes.Procs) >> 10,  /* report in GB */
        (MUStrIsEmpty(tmpLine)) ? "" : tmpLine);
      }  /* END for (rqindex) */
    }    /* END if (R->SubType == mrmstXT3) */

  Protocol = R->ND.Protocol[mrmJobStart];

  /* execute command locally */

  ExeHost[0] = '\0';

  if (R->SubType == mrmstXT4)
    MJobPBSInsertEnvVarsIntoRM(J,R);
  
  mstring_t Response(MMAX_LINE);

  if (MNatDoCommand(
       &R->ND,
       (void *)J,
       mrmJobStart,
       Protocol,
       TRUE,
       ExeHost,
       SPath.c_str(),
       &Response,               /* O */
       EMsg,                    /* O */
       SC) == FAILURE)
    {
    char tmpLine[MMAX_LINE];

    /* cannot execute job */

    snprintf(tmpLine,sizeof(tmpLine),"cannot start job - %s",
      ((EMsg != NULL) && (EMsg[0] != '\0')) ? EMsg : "???");

    MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

    MDB(1,fNAT) MLog("INFO:     job '%s' could not be started with native rm %s - %s\n",
      J->Name,
      R->Name,
      (EMsg != NULL) ? EMsg : "???");

    return(FAILURE);
    }

  MDB(1,fNAT) MLog("INFO:     job '%s' successfully started\n",
    J->Name);
 
  return(SUCCESS);
  }  /* END MNatJobStart() */





/**
 * Cancel job via native interface.
 *
 * @see MRMJobCancel() - parent
 *
 * @param J (I)
 * @param R (I)
 * @param Msg (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobCancel(

  mjob_t               *J,    /* I */
  mrm_t                *R,    /* I */
  char                 *Msg,  /* I (optional) */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  char  tmpLine[MMAX_LINE];
  char *tmpJID;

  enum MBaseProtocolEnum Protocol;

  const char *FName = "MNatJobCancel";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,EMsg,SC)\n",
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

  if ((R->ND.Path[mrmJobCancel] == NULL) ||
      (R->ND.Path[mrmJobCancel][0] == '\0'))
    {
    if (bmisset(&R->RTypes,mrmrtStorage) ||
        bmisset(&R->RTypes,mrmrtNetwork) ||
        bmisset(&R->RTypes,mrmrtLicense))
      {
      /* cancel interface is optional for logical resources */

      return(SUCCESS);
      }

    if (EMsg != NULL)
      strcpy(EMsg,"URL not specified");

    /* NOTE:  job message created in MRMJobCancel() */

    return(FAILURE);
    }
    
  /* use destination RM jobID if available */

  if ((J->DRMJID != NULL) &&
      (!bmisset(&R->RTypes,mrmrtStorage))) /* Datarm reqs submitted w/J->Name */
    {
    tmpJID = J->DRMJID;
    }
  else
    {
    tmpJID = J->Name;
    }

  Protocol = (R->ND.Protocol[mrmJobCancel] != mbpNONE) ? 
    R->ND.Protocol[mrmJobCancel] : 
    mbpExec;

  switch (Protocol)
    {
    case mbpHTTP:

      /* FORMAT: <URL>?jobname=<JOBID> */

      {
      char tmpJName[MMAX_LINE];

      if (MUURLEncode(tmpJID,tmpJName,sizeof(tmpJName)) == NULL)
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot URL encode job name for request");
          }

        return(FAILURE);
        }

      snprintf(tmpLine,sizeof(tmpLine),"%s?jobname=%s",
        (R->ND.Path[mrmJobCancel] != NULL) ? R->ND.Path[mrmJobCancel] : "/",
        tmpJName);
      }

      break;

    default:

      /* FORMAT:  <JOBID> */

      snprintf(tmpLine,sizeof(tmpLine),"%s %s",
        R->ND.Path[mrmJobCancel],
        tmpJID);

#ifdef MYAHOO
      {
      /* add the job's node to the argument list */

      char NodeList[MMAX_LINE];  /* should be plenty big for Yahoo's one node jobs */

      MUTaskMapToString(J->TaskMap,NULL,'\0',NodeList,sizeof(NodeList));

      MUStrCat(tmpLine," ",sizeof(tmpLine));
      MUStrCat(tmpLine,NodeList,sizeof(tmpLine));
      }
#endif /* MYAHOO */

      break;
    } /* END switch (R->ND.Protocol[mrmJobCancel]) */

  switch (R->SubType)
    {
    case mrmstXT4:

      MUStrCat(tmpLine," --rm ",sizeof(tmpLine));
      MUStrCat(tmpLine,R->Name,sizeof(tmpLine));

      break;

    default:

      /* NO-OP */

      break;
    }

  mstring_t  Response(MMAX_LINE);

  if (MNatDoCommand(
       &R->ND,
       (void *)J,
       mrmJobCancel,
       R->ND.Protocol[mrmJobCancel],
       TRUE,
       R->ND.Host[mrmJobCancel],
       tmpLine,
       &Response,
       EMsg,
       NULL) == FAILURE)
    {
    /* cannot cancel/terminate job */

    MDB(2,fNAT) MLog("ALERT:    cannot execute cancel command for job %s - '%s'\n",
      J->Name,
      (EMsg != NULL) ? EMsg : "command failed");

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute cancel command");

    return(FAILURE);
    }

  /* setting state to Completed causes job to never call MJobProcessRemoved() nor 
     MJobProcessCompleted().  Removed 5/1/09 DRW. */

/*
  MJobSetState(J,mjsCompleted);
*/

  bmset(&J->IFlags,mjifWasCanceled);

  MJobTransition(J,TRUE,FALSE);
 
  return(SUCCESS);
  }  /* END MNatJobCancel() */






/**
 * Migrate job tasks via native RM interface.
 *
 * NOTE:  Should be called MNatTaskMigrate()?
 *
 * @see MRMJobMigrate() - parent
 * @see MNatNodeMigrate() - peer
 * @see MNatDoCommand() - child
 *
 * @param J (I) [optional,modified]
 * @param R (I)
 * @param SrcVMList (I) [optional]
 * @param DstNL (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobMigrate(

  mjob_t     *J,            /* I (modified) */
  mrm_t      *R,            /* I */
  char      **SrcVMList,    /* I (optional) */
  mnl_t      *DstNL,        /* I */
  char       *EMsg,         /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC) /* O (optional) */

  {
  char *tmpJID;

  enum MBaseProtocolEnum Protocol;

  const char *FName = "MNatJobMigrate";

  MDB(1,fNAT) MLog("%s(%s,%s,SrcVMList,DstNL,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (R == NULL)
    {
    if (EMsg != NULL)
      sprintf(EMsg,"invalid job specified");

    return(FAILURE);
    }

  if ((R->ND.Path[mrmJobMigrate] == NULL) ||
      (R->ND.Path[mrmJobMigrate][0] == '\0'))
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"invalid 'JOBMIGRATEURL' specified for RM '%s'",
        R->Name);
      }

    return(FAILURE);
    }

  mstring_t tmpBuf(MMAX_LINE);

  /* use destination RM jobID if available */

  if (J == NULL)
    {
    tmpJID = NULL;
    }
  else if (J->DRMJID != NULL)
    {
    tmpJID = J->DRMJID;
    }
  else
    {
    tmpJID = J->Name;
    }

  /* FORMAT:  [-j <JOBEXPR>] [--vmigrate <VMID>.pn=<PN>] ... */

  Protocol = (R->ND.Protocol[mrmJobModify] != mbpNONE) ?
    R->ND.Protocol[mrmJobMigrate] :
    mbpExec;

  switch (Protocol)
    {
    case mbpHTTP:

      {
      char tmpLine[MMAX_LINE];

      char tmpJName[MMAX_LINE];

      if ((SrcVMList == NULL) || (SrcVMList[0] == NULL))
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot URL encode attr/value info for request");
          }

        return(FAILURE);
        }

      /* NYI */
 
      MUStrCpy(tmpLine,SrcVMList[0],sizeof(tmpLine));

      if (MUURLEncode(tmpLine,tmpJName,sizeof(tmpJName)) == NULL)
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot URL encode attr/value info for request");
          }

        return(FAILURE);
        }

      MStringSetF(&tmpBuf,"%s?jobname=%s&vmid=%s&pnode=%s",
        (R->ND.Path[mrmJobMigrate] != NULL) ? R->ND.Path[mrmJobMigrate] : "/",
        (tmpJID != NULL) ? tmpJID : "",
        tmpLine,
        tmpLine);
      }  /* END BLOCK (case mbpHTTP) */

      break;

    default:

      {
      mnode_t *tmpN;

      int   tindex;

      if ((SrcVMList == NULL) || (SrcVMList[0] == NULL))
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"VM list not specified");
          }

        return(FAILURE);
        }

      MStringAppend(&tmpBuf,R->ND.Path[mrmJobMigrate]);

      for (tindex = 0;SrcVMList[tindex] != NULL;tindex++)
        {
        if (MNLGetNodeAtIndex(DstNL,tindex,&tmpN) == FAILURE)
          break;

        /* Also pass the system job id so that MSM can report back gevents */

        MStringAppendF(&tmpBuf," %s%s.pn=%s",
          (tindex >= 0) ? "--vmigrate " : "",
          SrcVMList[tindex],
          tmpN->Name);
        }

      if (tmpJID != NULL)
        { 
        if (R->SubType == mrmstMSM)
          {
          MStringAppendF(&tmpBuf," operationid=%s",tmpJID);
          }
        else
          {
          MStringAppendF(&tmpBuf," -j %s",tmpJID);
          }
        } /* END if (tmpJID != NULL) */
      }  /* END BLOCK (case default) */

      break;
    }  /* END switch (Protocol) */

  mstring_t Response(MMAX_LINE);

  if (MNatDoCommand(
       &R->ND,
       (void *)J,
       mrmJobMigrate,
       Protocol,
       FALSE,  /* IsBlocking */
       R->ND.Host[mrmJobMigrate],
       tmpBuf.c_str(),           /* I */
       &Response,                /* O */
       EMsg,                     /* O */
       SC) == FAILURE)
    {
    /* cannot migrate job */

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute migrate command");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatJobMigrate() */






/**
 * Modify job via native RM interface.
 *
 * @param J (I) [modified]
 * @param R (I)
 * @param Attr (I)
 * @param SubAttr (I) [optional]
 * @param Val (I)
 * @param Op (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobModify(

  mjob_t     *J,
  mrm_t      *R,
  const char *Attr,
  const char *SubAttr,
  const char *Val,
  enum MObjectSetModeEnum Op,
  char       *EMsg,
  enum MStatusCodeEnum *SC)

  {
  char *tmpJID;

  char JAttr[MMAX_LINE];

  enum MBaseProtocolEnum Protocol;

  const char *FName = "MNatJobModify";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,%s,%s,%d,EMsg,SC)\n",
    FName,
    (J    != NULL) ? J->Name : "NULL",
    (R    != NULL) ? R->Name : "NULL",
    (Attr != NULL) ? Attr : "NULL",
    (SubAttr != NULL) ? SubAttr : "NULL",
    (Val != NULL) ? Val : "NULL",
    Op);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((J == NULL) || (R == NULL))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"invalid job specified");

    return(FAILURE);
    }

  if ((R->ND.Path[mrmJobModify] == NULL) || 
      (R->ND.Path[mrmJobModify][0] == '\0'))
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"invalid 'JOBMODIFYURL' specified for RM '%s'",
        R->Name);
      }

    return(FAILURE);
    }

  if ((Attr != NULL) && (!strcasecmp(Attr,"comment")))
    {
    /* NOTE:  by default, comments should only be passed to RM's if these RM's 
              have well-defined interfaces and will be independently queried 
              and diagnosed - thus most comments should not be passed to native 
              interfaces */

    return(SUCCESS);
    }

  mstring_t tmpBuf(MMAX_LINE);

  /* use destination RM jobID if available */

  if (J->DRMJID != NULL)
    {
    tmpJID = J->DRMJID;
    }
  else
    {
    tmpJID = J->Name;
    }

  /* FORMAT:  [-j <JOBEXPR>] [--s|--c|--i|--d] <ATTR>[=<VALUE>] [<ATTR>[=<VALUE>]]... */
  /* { s-set, c-create, i-incr, d-decr } */

  if ((Attr != NULL) && (!strcasecmp(Attr,"allocnodelist")))
    {
    strcpy(JAttr,"TASKLIST");
    }
  else
    {
    MUStrCpy(JAttr,Attr,sizeof(JAttr));
    }

  Protocol = (R->ND.Protocol[mrmJobModify] != mbpNONE) ? 
    R->ND.Protocol[mrmJobModify] : 
    mbpExec;

  switch (Protocol)
    {
    case mbpHTTP:

      {
      char tmpJName[MMAX_LINE];
      char tmpJAttr[MMAX_LINE];
      char tmpJVal[MMAX_LINE];

      if ((MUURLEncode(tmpJID,tmpJName,sizeof(tmpJName)) == NULL) ||
          (MUURLEncode(JAttr,tmpJAttr,sizeof(tmpJAttr)) == NULL) ||
          (MUURLEncode(Val,tmpJVal,sizeof(tmpJVal)) == NULL)) 
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot URL encode attr/value info for request");
          }

        return(FAILURE);
        }

      MStringSetF(&tmpBuf,"%s?jobname=%s&attr=%s&value=%s",
        (R->ND.Path[mrmJobModify] != NULL) ? R->ND.Path[mrmJobModify] : "/",
        tmpJName,
        tmpJAttr,
        tmpJVal);
      }  /* END BLOCK (case mbpHTTP) */

      break;

    default:

      {
      int  rindex;
      int  nindex;

      char tmpVal[MMAX_LINE];

      char tmpString[MMAX_BUFFER];

      tmpString[0] = '\0';

      if (Val == NULL)
        break;

      MUStrCpy(tmpVal,Val,sizeof(tmpVal));

      MUStringChop(tmpVal); /* chop off trailing whitespace */

      nindex = 0;

      for (rindex = 0;rindex < (int)strlen(tmpVal);rindex++)
        {
        if (nindex >= (int)(sizeof(tmpString) - 1))
          break;

        /* replace " and \n with \" and ' ' */

        if (tmpVal[rindex] == '\n')
          {
          tmpString[nindex] = ' ';
          }
        else if (tmpVal[rindex] == '\"')
          {
          tmpString[nindex++] = '\\';
          tmpString[nindex]   = '\"';
          }
        else
          {
          tmpString[nindex] = tmpVal[rindex];
          }
 
        nindex++;
        }  /* end for (rindex) */

      tmpString[nindex] = '\0';

      MStringSetF(&tmpBuf,"%s -j %s --set %s=\"%s\"",
        R->ND.Path[mrmJobModify],
        tmpJID,
        JAttr,
        tmpString);
      }

      break;
    }  /* END switch (Protocol) */

  mstring_t Response(MMAX_LINE);

  if (MNatDoCommand(
       &R->ND,
       (void *)J,
       mrmJobModify,
       Protocol,
       TRUE,
       R->ND.Host[mrmJobModify],
       tmpBuf.c_str(),           /* I */
       &Response,                /* O */
       EMsg,                     /* O */
       SC) == FAILURE)
    {
    /* cannot modify job */

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot execute modify command");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatJobModify() */





/**
 * Send requeue job request via native resource manager interface.
 *
 * @see MRMJobRequeue() - parent
 * @see MJobRequeue() - parent
 *
 * @param J (I)
 * @param R (I)
 * @param JPList (I) [not used]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobRequeue(

  mjob_t   *J,      /* I */
  mrm_t    *R,      /* I */
  mjob_t  **JPList, /* I (not used) */
  char     *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  int      *SC)     /* O (optional) */

  {
  char tmpLine[MMAX_LINE];

  const char *FName = "MNatJobRequeue";

  MDB(1,fNAT) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((!MJobPtrIsValid(J)) || (R == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  if (MJOBISACTIVE(J) == TRUE)
    {
    char ExeHost[MMAX_NAME];

    if (MNLIsEmpty(&J->NodeList))
      {
      /* invalid host list */

      if (EMsg != NULL)
        strcpy(EMsg,"cannot locate allocated resources");

      return(FAILURE);
      }

    if (R->ND.Path[mrmJobRequeue] == NULL)
      {
      /* requeue command not available */

      if (EMsg != NULL)
        strcpy(EMsg,"no requeue interface defined");

      /* job message created in MRMJobRequeue() */

      return(FAILURE);
      }

    sprintf(tmpLine,"%s %s",
      R->ND.Path[mrmJobRequeue],
      (J->DRMJID != NULL) ? J->DRMJID : J->Name);

    switch (R->SubType)
      {
      case mrmstXT4:

        MUStrCat(tmpLine," --rm ",sizeof(tmpLine));
        MUStrCat(tmpLine,R->Name,sizeof(tmpLine));

        break;

      default:

        /* NO-OP */

        break;
      }

    mstring_t Response(MMAX_LINE);

    ExeHost[0] = '\0';

    if (MNatDoCommand(
         &R->ND,
         (void *)J,
         mrmJobRequeue,
         mbpExec,
         TRUE,
         ExeHost,
         tmpLine,
         &Response,
         EMsg,
         NULL) == FAILURE)
      {
      /* cannot requeue job */

      if (EMsg != NULL)
        strcpy(EMsg,"cannot execute requeue command");

      return(FAILURE);
      }
    }    /* END if (MJOBISACTIVE(J) == TRUE) */

  MJobSetState(J,mjsIdle);

  return(SUCCESS);
  }  /* END MNatJobRequeue() */







/**
 * Send suspend job request via native resource manager interface.
 *
 * @see MRMJobSuspend() - parent
 *
 * @param J (I)
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobSuspend(

  mjob_t   *J,    /* I */
  mrm_t    *R,    /* I */
  char     *EMsg, /* O (optional,minsize=MMAX_LINE) */
  int      *SC)   /* O (optional) */

  {
  char tmpLine[MMAX_LINE];

  const char *FName = "MNatJobSuspend";

  MDB(1,fNAT) MLog("%s(%s,%s,EMsg,SC)\n",
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

  if (MJOBISACTIVE(J) == TRUE)
    {
    char ExeHost[MMAX_NAME];

    if (MNLIsEmpty(&J->NodeList))
      {
      /* invalid host list */

      if (EMsg != NULL)
        strcpy(EMsg,"cannot locate allocated resources");

      return(FAILURE);
      }

    if (R->ND.Path[mrmJobSuspend] == NULL)
      {
      /* suspend interface not specified */

      if (EMsg != NULL)
        strcpy(EMsg,"no suspend interface defined");

      /* job message created in MRMJobSuspend() */

      return(FAILURE);
      }

    sprintf(tmpLine,"%s %s",
      R->ND.Path[mrmJobSuspend],
      J->Name);

    ExeHost[0] = '\0';

    mstring_t Response(MMAX_LINE);

    if (MNatDoCommand(
         &R->ND,
         (void *)J,
         mrmJobSuspend,
         mbpExec,
         TRUE,
         ExeHost,
         tmpLine,
         &Response,
         EMsg,
         NULL) == FAILURE)
      {
      /* cannot suspend job */

      if (EMsg != NULL)
        strcpy(EMsg,"cannot execute suspend command");

      return(FAILURE);
      }
    }    /* END if (MJOBISACTIVE(J) == TRUE) */

  MJobSetState(J,mjsSuspended);

  return(SUCCESS);
  }  /* END MNatJobSuspend() */





/**
 * Send resume job request via native resource manager interface.
 *
 *
 * @param J (I)
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatJobResume(

  mjob_t  *J,    /* I */
  mrm_t   *R,    /* I */
  char    *EMsg, /* O (optional,minsize=MMAX_LINE) */
  int     *SC)   /* O (optional) */

  {
  char tmpLine[MMAX_LINE];

  const char *FName = "MNatJobResume";

  MDB(1,fNAT) MLog("%s(%s,%s,EMsg,SC)\n",
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

  if (MJOBISACTIVE(J) == FALSE)
    {
    char ExeHost[MMAX_NAME];

    if (MNLIsEmpty(&J->NodeList))
      {
      /* invalid host list */

      if (EMsg != NULL)
        strcpy(EMsg,"cannot locate allocated resources");

      return(FAILURE);
      }

    sprintf(tmpLine,"%s %s",
      R->ND.Path[mrmJobResume],
      J->Name);

    ExeHost[0] = '\0';

    mstring_t Response(MMAX_LINE);

    if (MNatDoCommand(
         &R->ND,
         (void *)J,
         mrmJobResume,
         mbpExec,
         TRUE,
         ExeHost,
         tmpLine,
         &Response,
         EMsg,
         NULL) == FAILURE)
      {
      /* cannot resume job */

      if (EMsg != NULL)
        strcpy(EMsg,"cannot execute resume command");

      return(FAILURE);
      }
    }    /* END if (MJOBISACTIVE(J) == TRUE) */

  MJobSetState(J,mjsRunning);

  return(SUCCESS);
  }  /* END MNatJobResume() */





/**
 * Load native node information.
 *
 * @see MNatClusterQuery() - parent
 * @see MWikiNodeUpdate() - child
 *
 * @param N (I) [modified]
 * @param NP (I)
 * @param NewState (I)
 * @param R (I)
 */

int MNatNodeLoad(

  mnode_t *N,   /* I (modified) */
  void    *NP,  /* I */
  enum MNodeStateEnum NewState, /* I */
  mrm_t   *R)   /* I */

  {
  char *Value;
  char *ptr;

  int   cindex;

  char  ValBuf[MMAX_BUFFER];

  char  EMsg[MMAX_LINE];

  int   Length;

#ifdef MNOT

  char *TokPtr;

  char  AttrArray[MMAX_NAME];

  enum MNodeStateEnum sindex;

  enum MNodeAttrEnum aindex;

  int  tmpI;


#endif /* MNOT */

  int  SQCount = 0;
  int  DQCount = 0;

  const char *FName = "MNatNodeLoad";

  MDB(5,fRM) MLog("%s(%s,NP,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    MNodeState[NewState],
    (R != NULL) ? R->Name : "NULL");

  if ((N == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  Value = (char *)NP;

  if (strchr(Value,'=') == NULL)
    {
    return(MNatNodeLoadFlat(N,NP,NewState,R));
    }

  /*  FORMAT: <NODEID>[ <ATTR>=<VALUE>]... */

  if ((Value == NULL) || (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  ptr = Value;

  /* skip over name */

  while (*ptr != '\0' && !isspace(*(ptr++)));

  MUStrCpy(ValBuf,ptr,sizeof(ValBuf));

  if (R->UseNodeCache == TRUE)
    {
    if ((N->AttrString != NULL) && !strcmp(N->AttrString,ptr))
      {
      /* NOTE:  no changes, no processing required */

      return(SUCCESS);
      }

    /* cache and process data */

    MUStrDup(&N->AttrString,ptr);
    }

  /* convert to WIKI */

  Length = strlen(ValBuf);

  for (cindex = 0;cindex < Length - 1;cindex++)
    {
    if (ValBuf[cindex] == '<')
      {
      /* skip any xml */

      while ((cindex < Length) && (ValBuf[cindex] != '>'))
        {
        cindex++;
        }

      continue;
      }

    if (ValBuf[cindex] == '\'')
      {
      SQCount++;
      }
    else if (ValBuf[cindex] == '\"')
      {
      DQCount++;
      }
    else if ((SQCount % 2 == 0) && (DQCount % 2 == 0))
      {
      if (strchr(" \t\n",ValBuf[cindex]) != NULL)
        ValBuf[cindex] = ';';
      }
    }  /* END for (cindex) */

  strcat(ValBuf,";");

  MWikiNodeUpdate(ValBuf,N,R,EMsg);

  if (EMsg[0] != '\0')
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"info corrupt for node %s - %s",
      N->Name,
      EMsg);

    MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
    }

  /* NOTE:  race condition:  if one rm loads node info and one loads queue info,
            if RM 1 reports class hostlist, it needs nodes already loaded 
            if RM 2 reports nodes, it needs class host expression */

  if (!bmisset(&N->IFlags,mnifRMClassUpdate) && 
     (bmisclear(&N->Classes)) &&
     !bmisset(&R->Flags,mrmfQueueIsPrivate) &&
     !bmisset(&R->Flags,mrmfRMClassUpdate) &&
     (N->CRes.Procs > 0))
    {
    mclass_t *C;

    char tmpLine[MMAX_LINE];

    /* load class info from Moab config */

    /* Using MCLASS_FIRST_INDEX to skip DEFAULT class */
    for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
      {
      if (MClass[cindex].Name[0] == '\0')
        break;

      if (MClass[cindex].Name[0] == '\1')
        continue;

      C = &MClass[cindex];

      if (C->HostExp != NULL)
        {
        strcpy(tmpLine,N->Name);

        /* create a mutable string from the HostExp */

        char *mutableHostExp = NULL;
        MUStrDup(&mutableHostExp,C->HostExp);

        int rc = MUREToList(
             mutableHostExp,
             mxoNode,
             NULL,
             NULL,
             FALSE,
             tmpLine,  /* NodeName - may be overwritten */
             sizeof(tmpLine));

        /* free mutable string */
        MUFree(&mutableHostExp);

       if (rc  == FAILURE)
          {
          /* node not listed in HostExp */

          continue;
          }
        }    /* END if (C->HostExp != NULL) */

      bmset(&N->Classes,C->Index);
      }  /* END for (cindex) */
    }    /* END if (!bmisset(&N->Flags,mnfRMClassUpdate) && ...) */

  /* perform subtype-specific tasks */

  if (bmisset(&R->RTypes,mrmrtStorage))
    {
    /* update storage resource */

    int fsindex;
    int duration;

    double tmpUsage;
    double TotalUsage;

    mgcred_t *U;

    /* NOTE:  fs usage information available in N->FSys */
    /*        N->FSys contains utilized disk, NOT available disk */

    /* NOTE:  should load configured disk space (NYI) */

    if (N->State == mnsNONE)
      MNodeSetState(N,mnsIdle,FALSE);

    if (MNLIsEmpty(&R->NL))
      {
      mnl_t *NL = &R->NL;

      if (R->MaxDSOp > 0)
        {
        int oindex;

        oindex = MUMAGetIndex(meGRes,"dsop",mAdd);

        if (MSNLGetIndexCount(&N->CRes.GenericRes,oindex) == 0)
          {
          MSNLSetCount(&N->CRes.GenericRes,oindex,R->MaxDSOp);
          }
        }

      MNLSetNodeAtIndex(NL,0,N);
      MNLSetTCAtIndex(NL,0,1);
      MNLTerminateAtIndex(NL,1);
      }  /* END if (R->NL == NULL) */

    /* update global and credential statistics */

    if (N->FSys != NULL)
      {
      TotalUsage = 0.0;

      duration = (R->UTime != 0) ? MSched.Time - R->UTime : R->PollInterval;

      for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
        {
        if (N->FSys->Name[fsindex] == NULL)
          break;

        if (MUserFind(N->FSys->Name[fsindex],&U) == FAILURE)
          {
          /* ERROR:  cannot find user */

          continue;
          }

        /* update account usage */

        tmpUsage = (double)N->FSys->ASize[fsindex] * duration;

        TotalUsage += tmpUsage;

        ((must_t *)U->Stat.IStat[U->Stat.IStatCount - 1])->DSUtilized += tmpUsage;
        ((must_t *)U->Stat.IStat[U->Stat.IStatCount - 1])->IDSUtilized = tmpUsage;

        /* tmpUsage is disk usage is MB-seconds */

        /* FSChargeRate converts Proc-seconds to MB seconds (ie FSChargeRate ~ .0001?) */

        if (MAM[0].State == mrmsActive)
          {
          /* charge usage against accounting manager */

          /* NYI */
          }
        }  /* END for (fsindex) */

      /* update global usage */

      MPar[0].S.DSUtilized  += TotalUsage; 
      MPar[0].S.IDSUtilized += TotalUsage;
      }  /* END if (N->FSys != NULL) */
    }    /* END if (bmisset(&R->RTypes,mrmrtStorage)) */

  if (bmisset(&R->RTypes,mrmrtLicense))
    {
    if (N->State == mnsNONE)
      MNodeSetState(N,mnsIdle,FALSE);
    }

  if (bmisset(&R->RTypes,mrmrtNetwork))
    {
    if (N->State == mnsNONE)
      MNodeSetState(N,mnsIdle,FALSE);

    if (MNLIsEmpty(&R->NL))
      {
      mnl_t *NL = &R->NL;

      /* NOTE:  only create NL list for a single node plus terminator */

      MNLSetNodeAtIndex(NL,0,N);
      MNLSetTCAtIndex(NL,0,1);
      MNLTerminateAtIndex(NL,1);
      }  /* END if (R->NL == NULL) */
    }    /* END if (bmisset(&R->RTypes,mrmrtNetwork)) */

  /* update non-compute generic resources */

  if ((N->CRes.Procs == 0) && !bmisset(&R->RTypes,mrmrtCompute) && (!MSNLIsEmpty(&N->CRes.GenericRes)))
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
  }  /* END MNatNodeLoad() */





/**
 * Load simple node attributes from flat file text buffer.
 *
 * @see MNatNodeLoad() - parent
 *
 * @param N (I) [modified]
 * @param NP (I)
 * @param NewState (I)
 * @param R (I)
 */

int MNatNodeLoadFlat(

  mnode_t *N,   /* I (modified) */
  void    *NP,  /* I */
  enum MNodeStateEnum NewState, /* I */
  mrm_t   *R)   /* I */

  {
  int  aindex;
  int  ACount;
  int  tmpInt;

  char *tmpNAttr[mnaLAST];

  char *ptr;
  char *TokPtr;

  enum MNodeAttrEnum NatATable[] = {
    mnaNodeID,
    mnaNodeState,
    mnaRCProc,
    mnaRAProc,
    mnaRCMem,
    mnaRAMem,
    mnaOSList,
    mnaNONE };

  if (N == NULL)
    {
    return(FAILURE);
    }

  ptr = (char *)NP;

  /*  FORMAT: <NODEID> <NODESTATE> <CCPU> <ACPU> <CMEM> <AMEM> <OS> */

  if ((ptr == NULL) || (ptr[0] == '\0'))
    {
    return(FAILURE);
    }

  memset(tmpNAttr,0,sizeof(tmpNAttr));

  aindex = 0;

  tmpNAttr[aindex] = MUStrTok(ptr," ",&TokPtr);

  do
    {
    if (NatATable[aindex] == mnaNONE)
      break;

    tmpNAttr[NatATable[++aindex]] = MUStrTok(NULL," ",&TokPtr);
    }
  while (tmpNAttr[NatATable[aindex]] != NULL);

  ACount = aindex;

  /* NOTE:  Allow entries which include ONLY nodeid */

  if (ACount < 1)
    {
    /* line is corrupt */

    N->RMState[R->Index] = mnsDown;

    return(FAILURE);
    }

  if (tmpNAttr[mnaNodeState] != NULL)
    {
#if 0
    enum MNodeStateEnum OldState = N->State;
#endif

    if (!strcmp(tmpNAttr[mnaNodeState],"Draining"))
      {
      N->State = (enum MNodeStateEnum)MUGetIndexCI("DRAIN",MNodeState,TRUE,mnsNONE);
      }
    else
      {
      N->State = (enum MNodeStateEnum)MUGetIndexCI(tmpNAttr[mnaNodeState],MNodeState,TRUE,mnsNONE);
      }

    if (N->RM != NULL)
      {
      N->RMState[N->RM->Index] = N->State;
      }
    }     /* END if (tmpNAttr[mnaNodeState] != NULL) */

  if ((tmpNAttr[mnaOSList] != NULL) && (tmpNAttr[mnaOSList][0] != '-'))
    {
    MNodeSetAttr(N,mnaOSList,(void **)tmpNAttr[mnaOSList],mdfString,mSet);

    if (N->ActiveOS == 0)
      {
      if ((N->OSList != NULL) && (N->OSList[1].AIndex <= 0))
        MNodeSetAttr(
          N,
          mnaOS,
          (void **)MAList[meOpsys][N->OSList[0].AIndex],
          mdfString,
          mSet);
      else
        MNodeSetAttr(N,mnaOS,(void **)ANY,mdfString,mSet);
      }
    }

  if ((tmpNAttr[mnaRCProc] != NULL) && 
     ((tmpNAttr[mnaRCProc][0] != '-') ||
      (tmpNAttr[mnaRCProc][1] != '\0')))
    {
    N->CRes.Procs = (int)strtol(tmpNAttr[mnaRCProc],NULL,10);
    }

  if ((tmpNAttr[mnaRAProc] != NULL) &&
     ((tmpNAttr[mnaRAProc][0] != '-') ||
      (tmpNAttr[mnaRAProc][1] != '\0')))
    {
    N->ARes.Procs = (int)strtol(tmpNAttr[mnaRAProc],NULL,10);
    }

  if ((tmpNAttr[mnaRCMem] != NULL) &&
     ((tmpNAttr[mnaRCMem][0] != '-') ||
      (tmpNAttr[mnaRCMem][1] != '\0')))
    {
    tmpInt = (int)strtol(tmpNAttr[mnaRCMem],NULL,10);
    MNodeSetAttr(N,mnaRCMem,(void **)&tmpInt,mdfInt,mSet);
    }

  if ((tmpNAttr[mnaRAMem] != NULL) &&
     ((tmpNAttr[mnaRAMem][0] != '-') ||
      (tmpNAttr[mnaRAMem][1] != '\0')))
    {
    tmpInt = (int)strtol(tmpNAttr[mnaRAMem],NULL,10);
    MNodeSetAttr(N,mnaRAMem,(void **)&tmpInt,mdfInt,mSet);
    }

  /* NYI */

  return(SUCCESS);
  }  /* END MNatNodeLoadFlat() */





/**
 * Retrieve information from ganglia.
 *
 * @param ND   (I)
 * @param OBuf (O)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int __MNatClusterLoadGanglia(

  mnat_t    *ND,
  mstring_t *OBuf,
  char      *EMsg)

  {
  mrm_t *R;

  char *ptr  = NULL;

  const char *FName = "__MNatClusterLoadGanglia";

  MDB(3,fRM) MLog("%s(ND,OBuf,EMsg)\n",
    FName);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((ND == NULL) || (OBuf == NULL))
    {
    return(FAILURE);
    }

  R = (mrm_t *)ND->R;

  /* NOTE:  allow cached ganglia */

  if (R->ND.Path[mrmClusterQuery] != NULL)
    {
    int rc;

    rc = MNatDoCommand(
      &R->ND,
      NULL,
      mrmResourceQuery,
      mbpExec,
      TRUE,
      NULL,    /* HostName */
      NULL,    /* SPath */
      OBuf,
      NULL,
      NULL);  

    if (rc == FAILURE)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"recovery of cached ganglia info failed");
      }

    return(rc);
    }  /* END if (R->ND.Path[mrmClusterQuery] != NULL) */

  if ((R->P[0].S == NULL) && (MSUCreate(&R->P[0].S) == FAILURE))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"cannot create socket for ganglia");

    return(FAILURE);
    }

  if (MSUInitialize(
       R->P[0].S,   /* (modified) */
       R->P[0].HostName,
       R->P[0].Port,
       R->P[0].Timeout) == FAILURE)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"cannot initialize socket for ganglia");

    return(FAILURE);
    }

  if (MSUConnect(R->P[0].S,FALSE,EMsg) == FAILURE)
    {
    if (bmisset(&R->Flags,mrmfReport))
      {
      static mulong Offset = 0;
      static mulong ETime  = 0;

      char Msg[MMAX_LINE];

      if (MUCheckExpBackoff(MSched.Time,&ETime,&Offset) == SUCCESS)
        {
        sprintf(Msg,"RMFAILURE  cannot connect to rm '%s'\n",
          R->Name);

        MSysRegEvent(Msg,mactMail,1);
        }
      }

    return(FAILURE);
    }

  if (MSURecvPacket(
        R->P[0].S->sd,
        &ptr,  /* O */
        0,
        "</GANGLIA_XML>",
        R->P[0].Timeout,
        NULL) == FAILURE)
    {
    MSUDisconnect(R->P[0].S);

    if (bmisset(&R->Flags,mrmfReport))
      {
      char Msg[MMAX_LINE];

      sprintf(Msg,"RMFAILURE  cannot read data from rm '%s'\n",
        R->Name);

      MSysRegEvent(Msg,mactMail,1);
      }

    MUFree(&ptr);

    if (EMsg != NULL)
      strcpy(EMsg,"cannot read ganglia data");

    return(FAILURE);
    }

  *OBuf = ptr;

  MUFree(&ptr);

  MSUDisconnect(R->P[0].S);

  if ((strstr(OBuf->c_str(),"<GANGLIA_XML")) == NULL)  
    {
    if (bmisset(&R->Flags,mrmfReport))
      {
      char Msg[MMAX_LINE];

      sprintf(Msg,"RMFAILURE  cannot parse data from rm '%s'\n",
        R->Name);

      MSysRegEvent(Msg,mactMail,1);
      }

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"ganglia info is corrupt - read '%32s...'",
        OBuf->c_str());
      }

    return(FAILURE);  /* cannot locate beginning of data */
    }

  return(SUCCESS);
  }  /* END __MNatClusterLoadGanglia() */





/**
 * Load and process Ganglia cluster info.
 *
 * @see MNatClusterQuery() - parent
 * @see __MNatClusterProcessGanglia() - child()
 *
 * @param Buf (I) - ganglia output string in xml format
 * @param R (I)
 * @param RCount (O)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int __MNatClusterQueryGanglia(

  mstring_t            *Buf,    /* I - ganglia output string in xml format */
  mrm_t                *R,      /* I */
  int                  *RCount, /* O */
  char                 *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)     /* O (optional) */

  {
  mxml_t *DE = NULL;
  mxml_t *GE = NULL;
  mxml_t *CE = NULL;

  int     gtok;
  int     ctok;

  const char *FName = "__MNatClusterQueryGanglia";

  MDB(1,fNAT) MLog("%s(%s,RCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  if (MXMLFromString(&DE,Buf->c_str(),NULL,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  /* NOTE:  GANGLIA object may contain one or more CLUSTER objects or
            may contain one or more GRID objects which contain one or 
            most CLUSTER objects */

  /* process all grids */

  gtok = -1;

  while (MXMLGetChild(DE,(char *)MGangliaXML[mxgGrid],&gtok,&GE) == SUCCESS)
    {
    /* process each grid 'cluster' */

    ctok = -1;

    while (MXMLGetChild(GE,(char *)MGangliaXML[mxgCluster],&ctok,&CE) == SUCCESS)
      {
      /* process 'cluster' */

      __MNatClusterProcessGanglia(CE,R,RCount);
      }
    }

  /* process all direct attached clusters */
 
  ctok = -1;

  while (MXMLGetChild(DE,(char *)MGangliaXML[mxgCluster],&ctok,&CE) == SUCCESS)
    {
    /* process 'cluster' */

    __MNatClusterProcessGanglia(CE,R,RCount);
    }

  MXMLDestroyE(&DE);

  return(SUCCESS);
  }  /* END __MNatClusterQueryGanglia() */





/**
 * Process ganglia output.
 *
 * @param CE (I) - ganglia cluster XML
 * @param R (I)
 * @param RCount (O) [optional]
 */

int __MNatClusterProcessGanglia(

  mxml_t  *CE,     /* I - ganglia cluster XML */
  mrm_t   *R,      /* I */
  int     *RCount) /* O (optional) */

  {
  int tmpInt;

  mnode_t *N = NULL;

  enum MNodeStateEnum NewState;

  mxml_t *NE = NULL;
  mxml_t *AE = NULL;

  int     ntok;
  int     mtok;

  char    tmpNodeName[MMAX_NAME];
  char    tmpName[MMAX_NAME];
  char    tmpVal[MMAX_NAME];

  char    tmpOSName[MMAX_NAME];
  char    tmpOSRelease[MMAX_NAME];

  int     sindex;

  mulong  UTime;  /* ganglia data update time */

  const char *FName = "__MNatClusterProcessGanglia";

  MDB(1,fNAT) MLog("%s(CE,%s,RCount)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if ((CE == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  ntok = -1;

  while (MXMLGetChild(CE,(char *)MGangliaXML[mxgHost],&ntok,&NE) == SUCCESS)
    {
    /* process 'host' */

    if (MXMLGetAttr(NE,(char *)MGangliaXML[mxgName],NULL,tmpNodeName,sizeof(tmpNodeName)) == FAILURE)
      {
      /* XML is corrupt */

      continue;
      }

    if (MXMLGetAttr(NE,(char *)MGangliaXML[mxgUTime],NULL,tmpVal,sizeof(tmpVal)) == FAILURE)
      {
      /* XML is corrupt */
                                                                               
      continue;
      }

    UTime = strtol(tmpVal,NULL,10);

    /* resource list not specified */
    /* parse the xml for the next node */

    if (MNodeFind(tmpNodeName,&N) == FAILURE)
      {
      if (bmisset(&R->Flags,mrmfNoCreateAll) || bmisset(&R->Flags,mrmfNoCreateResource))
        {
        /* ignore node if not discovered by master RM */

        continue;
        }

      if (MNodeAdd(tmpNodeName,&N) == FAILURE)
        {
        /* cannot add node */

        MDB(1,fNAT) MLog("ALERT:    cannot add node '%s'\n",
          tmpNodeName);

        return(FAILURE); 
        }

      if (N->RMState[R->Index] == mnsNONE)
        N->RMState[R->Index] = mnsIdle;

      MRMNodePreLoad(N,N->RMState[R->Index],R);

      if (N->RMList == NULL)
        MNodeSetState(N,N->RMState[R->Index],FALSE);
      }
    else
      {
      MNodeAddRM(N,R);
      }

    if (RCount != NULL)
      (*RCount)++;

    MDB(1,fNAT) MLog("INFO:     evaluating node '%s' in %s\n",
      (N != NULL) ? N->Name : "NULL",
      FName);

    /* load in node specs from xml */

    if ((N == NULL) || (MNodeInitXLoad(N) == FAILURE))
      {
      MDB(1,fNAT) MLog("ERROR:    cannot alloc xload struct for node '%s' in %s\n",
        (N != NULL) ? N->Name : "NULL",
        FName);

      return(FAILURE);
      }

    tmpOSName[0] = '\0';
    tmpOSRelease[0] = '\0';

    mtok = -1;

    while (MXMLGetChild(NE,(char *)MGangliaXML[mxgMetric],&mtok,&AE) == SUCCESS)
      {
      /* process 'metric' */

      if (MXMLGetAttr(AE,(char *)MGangliaXML[mxgName],NULL,tmpName,sizeof(tmpName)) == FAILURE)
        {
        continue;
        }

      if (MXMLGetAttr(AE,(char *)MGangliaXML[mxgVal],NULL,tmpVal,sizeof(tmpVal)) == FAILURE)
        {
        continue;
        }

      sindex = (enum MRMGangliaAttr)MUGetIndex(
        tmpName,
        MRMGangliaAttrList,
        FALSE,
        mrmgUnknown);

      switch (sindex)
        {
        case mrmgCpuNum:    

          N->CRes.Procs = strtol(tmpVal,(char **)NULL,10); 

          break;

        case mrmgCpuSpeed:

          N->ProcSpeed = strtol(tmpVal,(char **)NULL,10);

          break;

        case mrmgDiskFree:  

          N->ARes.Disk     = strtol(tmpVal,(char **)NULL,10) << 10; 

          break;

        case mrmgDiskTotal: 

          N->CRes.Disk     = strtol(tmpVal,(char **)NULL,10) << 10; 

          break;

        case mrmgLoadOne:   

          N->Load          = strtod(tmpVal,(char **)NULL); 

          break;

        case mrmgMachineType:

          N->Arch = MUMAGetIndex(meArch,tmpVal,mAdd);

          break;

        case mrmgMemFree:   

          tmpInt = strtol(tmpVal,(char **)NULL,10) >> 10;
          MNodeSetAttr(N,mnaRAMem,(void **)&tmpInt,mdfInt,mSet); 

          break;

        case mrmgMemTotal:  

          tmpInt = strtol(tmpVal,(char **)NULL,10) >> 10;
          MNodeSetAttr(N,mnaRCMem,(void **)&tmpInt,mdfInt,mSet); 

          break;

        case mrmgOSName:

          /* NOTE:  should merge osname and osrelease into single variable */

          MUStrCpy(tmpOSName,tmpVal,sizeof(tmpOSName));

          break;

        case mrmgOSRelease:

          MUStrCpy(tmpOSRelease,tmpVal,sizeof(tmpOSRelease));

          break;

        case mrmgSwapFree:  

          tmpInt = strtol(tmpVal,(char **)NULL,10) >> 10;
          MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet); 

          break;

        case mrmgSwapTotal: 

          tmpInt = strtol(tmpVal,(char **)NULL,10) >> 10;
          MNodeSetAttr(N,mnaRCSwap,(void **)&tmpInt,mdfInt,mSet); 

          break;

        case mrmgPktSIn:
        case mrmgPktSOut:
        case mrmgUnknown:
        default:

          /* NOT PROCESSED */

          /* NO-OP */

          break;
        };  /* END switch (sindex) */
      }     /* END while (MXMLGetChild(NE) == SUCCESS) */

    /* NOTE:  if OS already available from alternate source - ignore ganglia
              info */

    if (!bmisset(&R->Flags,mrmfIgnOS) && (tmpOSName[0] != '\0'))
      {
      if (tmpOSRelease[0] != '\0')
        {
        char tmpLine[MMAX_NAME];

        sprintf(tmpLine,"%s-%s",
          tmpOSName,
          tmpOSRelease);

        N->ActiveOS = MUMAGetIndex(meOpsys,tmpLine,mAdd);
        }
      else
        {
        N->ActiveOS = MUMAGetIndex(meOpsys,tmpOSName,mAdd);
        }
      }

    /* NOTE:  ganglia adjustments should take place after all RM's are loaded */

    /* ganglia should always 'see' configured processors, so allow eval always */

/*
    if ((N->IsNew == FALSE) ||
        (!bmisset(&R->Flags,mrmfNoCreateAll) && bmisset(&R->Flags,mrmfNoCreateResource)) || 
        (N->RMList == NULL))
*/
      {
      NewState = N->RMState[R->Index];

      /* adjust node state */

      if (N->CRes.Procs == 0)
        {
        NewState = mnsDown;
        }
      else if (MSched.Time - UTime > MCONST_HOURLEN)
        {
        NewState = mnsDown;
        }
      else if (N->Load > 2.0 * N->CRes.Procs)
        {
        NewState = mnsBusy;
        }

      N->RMState[R->Index] = NewState;

      if (N->State == mnsNONE)
        MNodeSetState(N,NewState,FALSE);
      }    /* END if ((N->IsNew == FALSE) || ...) */

    MDB(7,fNAT)
      MNodeShow(N);
    }   /* END while (MXMLGetChild(NE) == SUCCESS) */

  return(SUCCESS);
  }  /* END __MNatClusterProcessGanglia() */





/**
 * Process native node attributes and update node.
 *
 * @see MNatClusterQuery() - parent
 * @see MNatNodeLoadExt() - child
 *
 * Data is queried on a separate thread for each native RM.  When the query is ready for processing
 * This routine (and its parent) is called once for each RM from MRMClusterQuery() and the query data
 * is then processed 
 *
 * @param N          (I) [modified]
 * @param R          (I)
 * @param DefND      (I)
 * @param CLine      (I) [optional]
 * @param RMIsMaster (I)
 * @param IsNew      (I)
 * @param AList      (I) Apply attrs in list to node instead of using attr buf [optional]
 * @param TmpN       (I) Used in conjuction iwth AList. AList specifies which attrs to copy from tmpN to N [optional] @see MWikiNodeApplyAttr()
 */

int __MNatNodeProcess(

  mnode_t *N,
  mrm_t   *R,
  mnat_t  *DefND,
  char    *CLine,
  mbool_t  RMIsMaster,
  mbool_t  IsNew,
  enum MWNodeAttrEnum *AList,
  mnode_t *TmpN)

  {
  char    *ptr = NULL;

  char     tmpLine[MMAX_LINE];

  /* NOTE:  IsNew may be set to FALSE if node discovered on this iteration by another RM */

  const char *FName = "__MNatNodeProcess";

  if ((N == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if (bmisset(&MSched.Flags,mschedfAllowMultiCompute) && 
     (RMIsMaster == TRUE) && 
     (N->RM != NULL) && 
     (N->RM != R))
    {
    char    ActiveOS[MMAX_NAME];
    mbool_t ShouldUpdateNode;

    /* node is linked to alternate compute RM */

    __MNatNodeGetState(CLine,&N->RMState[R->Index],NULL,ActiveOS);
  
    MNodeUpdateMultiComputeStatus(
      N,
      R,
      ActiveOS,            /* O */
      &ShouldUpdateNode);  /* O */
  
    if (ShouldUpdateNode == FALSE)
      {
      return(SUCCESS);
      }

    if (bmisset(&N->IFlags,mnifIsNew))
      IsNew = TRUE;
    }    /* END if (bmisset(&MSched.Flags,mschedfAllowMultiCompute) && ...) */
  else
    {
    __MNatNodeGetState(CLine,&N->RMState[R->Index],NULL,NULL);
    }

  if (CLine != NULL)
    {
    /* data already loaded */

    ptr = CLine;
    }
  else if (R->ND.Path[mrmClusterQuery] == NULL)
    {
    /* active query not required */

    tmpLine[0] = '\0';

    ptr = tmpLine;
    }
  else
    {
    mstring_t String(MMAX_LINE);

    /* contact compute host */

    MNatDoCommand(
       &R->ND,
       NULL,
       mrmResourceQuery,
       R->ND.Protocol[mrmClusterQuery],
       TRUE,
       N->Name,
       NULL,
       &String,
       NULL,
       NULL);

    MUStrCpy(tmpLine,String.c_str(),sizeof(tmpLine));

    ptr = tmpLine;
    }

  MDB(5,fNAT) MLog("INFO:     evaluating node '%s' in %s\n",
    N->Name,
    FName);

  if (RMIsMaster == TRUE)
    {
    if ((IsNew == TRUE) || !bmisset(&N->IFlags,mnifRMDetected))
      {
      MRMNodePreLoad(N,N->RMState[R->Index],R);
      }
    else
      {
      MRMNodePreUpdate(N,N->RMState[R->Index],R);
      }
    }
  else
    {
    if ((IsNew == TRUE) || (bmisset(&N->IFlags,mnifIsNew)))
      MNodeAddRM(N,R);
    }

  if (AList != NULL && TmpN != NULL)
    MWikiNodeApplyAttr(N,TmpN,R,AList);
  else
    MNatNodeLoad(N,(void *)ptr,N->RMState[R->Index],R);

  if (N->CRes.Procs == -1)
    N->CRes.Procs = 0;

  if (N->ARes.Procs == -1)
    N->ARes.Procs = N->CRes.Procs;

  if (IsNew == TRUE)
    MRMNodePostLoad(N,R);
  else
    MRMNodePostUpdate(N,R);

  /* update resource type specific statistics */

  if (bmisset(&R->RTypes,mrmrtLicense))
    {
    int rindex;

    if (R->U.Nat.N == NULL)
      R->U.Nat.N = (void *)N;

    if (!MSNLIsEmpty(&N->CRes.GenericRes))
      {
      for (rindex = 1;rindex < MSched.M[mxoxGRes];rindex++)
        {
        if (MGRes.Name[rindex][0] == '\0')
          break;

        if ((R->U.Nat.ResName) &&
            (R->U.Nat.ResName[rindex][0] == 0))
          {
          MUStrCpy(
            R->U.Nat.ResName[rindex],
            MGRes.Name[rindex],
            strlen(MGRes.Name[rindex]) + 1);
          }
        }  /* END for (rindex) */
      }

    /* by default, disable active power management for non-compute resources */

    if (N->PowerPolicy == mpowpNONE)
      N->PowerPolicy = mpowpStatic;
    }  /* END if (bmisset(&R->RTypes,mrmrtLicense)) */

  if (bmisset(&R->RTypes,mrmrtNetwork) ||
      bmisset(&R->RTypes,mrmrtStorage))
    {
    if (IsNew == TRUE)
      {
      /* initialize non-compute resources */

      /* by default, disable active power management for non-compute resources */

      if (N->PowerPolicy == mpowpNONE)
        N->PowerPolicy = mpowpStatic;
      }
    }

  MDB(7,fNAT)
    MNodeShow(N);

  return(SUCCESS);
  }  /* END __MNatNodeProcess() */





/**
 *
 *
 * @param R (I)
 * @param QType (I) [optional]
 * @param Attribute (I)
 * @param Value (O) [minsize=MMAX_LINE]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatInfoQuery(

  mrm_t *R,         /* I */
  char  *QType,     /* I (optional) */
  char  *Attribute, /* I */
  char  *Value,     /* O (minsize=MMAX_LINE) */
  char  *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC) /* O (optional) */

  {
  int rc;

  char tmpBuffer[MMAX_BUFFER];

  const char *FName = "MNatInfoQuery";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (QType != NULL) ? QType : "NULL",
    (Attribute != NULL) ? Attribute : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Value != NULL)
    Value[0] = '\0';

  if ((R == NULL) || (Attribute == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, rejecting info query request\n",
      R->Name);

    return(FAILURE);
    }

  /* determine default ND structure */

  if (R->ND.URL[mrmInfoQuery] != NULL)
    {
    MDB(1,fNAT) MLog("ALERT:    info query action not specified for native interface\n");

    if (EMsg != NULL)
      strcpy(EMsg,"info query action not specified");

    return(FAILURE);
    }

  snprintf(tmpBuffer,sizeof(tmpBuffer),"%s %s %s",
    R->ND.Path[mrmInfoQuery],
    QType,
    Attribute);

  mstring_t Response(MMAX_LINE);

  rc = MNatDoCommand(
         &R->ND,
         NULL,
         mrmResourceQuery,
         R->ND.Protocol[mrmInfoQuery],
         TRUE,
         R->ND.Host[mrmInfoQuery],
         tmpBuffer,   /* I - should be populated? */
         &Response,   /* O */
         EMsg,
         NULL);

  MUStrCpy(Value,Response.c_str(),MMAX_LINE);

  if (rc == FAILURE)
    {
    /* info query failed */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatInfoQuery() */





/**
 * Issue system modify request via native RM interface.
 *
 * @param R (I)
 * @param AType (I) [optional]
 * @param Attribute (I)
 * @param IsBlocking (I)
 * @param Value (O) [minsize=MMAX_LINE]
 * @param Format (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatSystemModify(

  mrm_t                *R,          /* I */
  const char           *AType,      /* I action type (optional) */
  const char           *Attribute,  /* I */
  mbool_t               IsBlocking, /* I */
  char                 *Value,      /* O (minsize=MMAX_LINE) */
  enum MFormatModeEnum  Format,     /* I */
  char                 *EMsg,       /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)         /* O (optional) */

  {
  int rc;

  const char *FName = "MNatSystemModify";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (AType != NULL) ? AType : "NULL",
    (Attribute != NULL) ? Attribute : "NULL",
    MBool[IsBlocking]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Value != NULL)
    Value[0] = '\0';

  if ((R == NULL) || (Attribute == NULL) || (Value == NULL))
    {
    MDB(1,fNAT) MLog("INFO:     internal failure - invalid parameters passed to %s\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"invalid parameters");

    return(FAILURE);
    }

  if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, rejecting system modify request\n",
      R->Name);

    if (EMsg != NULL)
      strcpy(EMsg,"rm is down");

    return(FAILURE);
    }

  if (R->ND.URL[mrmSystemModify] == NULL)
    {
    MDB(1,fNAT) MLog("ALERT:    system modify action not specified for native interface\n");

    if (EMsg != NULL)
      strcpy(EMsg,"system modify action not specified");

    return(FAILURE);
    }

  mstring_t tmpCmdLine(MMAX_LINE);
  mstring_t Response(MMAX_LINE);

  MStringSetF(&tmpCmdLine,"%s %s %s",
    R->ND.Path[mrmSystemModify],
    (AType != NULL) ? AType : "default",
    Attribute);

  rc = MNatDoCommand(
        &R->ND,
        NULL,
        mrmSystemModify,
        R->ND.Protocol[mrmSystemModify],
        IsBlocking,
        R->ND.Host[mrmSystemModify],
        tmpCmdLine.c_str(),
        &Response,  /* O */
        EMsg,
        NULL);

  MUStrCpy(Value,Response.c_str(),MMAX_LINE);

  if (rc == FAILURE)
    {
    /* system modify failed */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatSystemModify() */





/**
 * Issue system query request via native interface.
 *
 * @param R (I)
 * @param QType (I) [optional]
 * @param Attribute (I)
 * @param IsBlocking (I)
 * @param Value (O) [minsize=MMAX_BUFFER]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatSystemQuery(

  mrm_t                *R,           /* I */
  char                 *QType,       /* I (optional) */
  char                 *Attribute,   /* I */
  mbool_t               IsBlocking,  /* I */
  char                 *Value,       /* O (minsize=MMAX_BUFFER) */
  char                 *EMsg,        /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)          /* O (optional) */

  {
  int rc;

  char    tmpLine[MMAX_LINE];

  char   *BPtr;
  int     BSpace;

  const char *FName = "MNatSystemQuery";

  MDB(1,fNAT) MLog("%s(%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (QType != NULL) ? QType : "NULL",
    (Attribute != NULL) ? Attribute : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Value != NULL)
    Value[0] = '\0';

  if ((R == NULL) || (Attribute == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, rejecting system query request\n",
      R->Name);

    return(FAILURE);
    }

  if (R->ND.URL[mrmSystemQuery] == NULL)
    {
    MDB(1,fNAT) MLog("ALERT:    system query action not specified for native interface\n");

    if (EMsg != NULL)
      strcpy(EMsg,"system query action not specified");

    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

  switch (R->SubType)
    {
    case mrmstXT4:

      MUSNPrintF(&BPtr,&BSpace,"%s --rm %s",
        R->ND.Path[mrmSystemQuery],
        R->Name);

      break;

    default:

      MUSNPrintF(&BPtr,&BSpace,"%s %s %s",
        R->ND.Path[mrmSystemQuery],
        (QType != NULL) ? QType : "default",
        Attribute);
    
      break;
    }    /* END switch(R->SubType) */

  mstring_t Response(MMAX_LINE);

  rc = MNatDoCommand(
         &R->ND,
         NULL,
         mrmResourceQuery,
         R->ND.Protocol[mrmSystemQuery],
         TRUE,  /* IsBlocking */
         R->ND.Host[mrmSystemQuery],
         tmpLine,
         &Response,  /* O */
         EMsg,
         SC);

  MUStrCpy(Value,Response.c_str(),MMAX_BUFFER);

  if (rc == FAILURE)
    {
    /* system query failed */

    return(FAILURE);
    }

  MDB(7,fNAT) MLog("INFO:     system query buffer ----\n%s\n", 
    Value);

  if (!strncasecmp(Value,"error:",strlen("error:")))
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,&Value[strlen("error:")],MMAX_LINE);

    Value[0] = '\0';

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatSystemQuery() */





/**
 * Issue RM start request via native interface.
 *
 * @see MNatRMStop() - peer - stop RM
 * @see MNatRMInitialize() - parent
 *
 * @param R (I)
 * @param Attr (I) [not used]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatRMStart(

  mrm_t                *R,    /* I */
  char                 *Attr, /* I (not used) */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  enum MBaseProtocolEnum Protocol = mbpNONE;

  char  tEMsg[MMAX_LINE];
  char *EMsgP;

  const char *FName = "MNatRMStart";

  MDB(1,fNAT) MLog("%s(%s,Attr,EMsg,SC)\n",
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

  if (R->ND.URL[mrmRMStart] == NULL)
    {
    return(FAILURE);
    }

  EMsgP = (EMsg != NULL) ? EMsg : tEMsg;

  Protocol = R->ND.Protocol[mrmRMStart];

  if (Protocol == mbpExec)
    {
    int rc;

    mstring_t Response(MMAX_LINE);

    rc = MNatDoCommand(
          &R->ND,
          NULL,
          mrmRMStart,
          Protocol,
          TRUE,
          R->ND.Host[mrmRMStart],
          R->ND.Path[mrmRMStart],
          &Response,  /* O */
          EMsgP,
          NULL);

    if (rc == FAILURE)
      {
      /* RM start failed */

      char tmpLine[MMAX_LINE];

      MDB(3,fRM) MLog("ALERT:    cannot start RM (RM '%s' failed in function '%s') - %s\n",
        R->Name,
        MRMFuncType[mrmRMStart],
        EMsgP);
  
      snprintf(tmpLine,sizeof(tmpLine),"cannot start RM - %s",
        EMsgP);

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(FAILURE);
      }
    }    /* END if (Protocol == mbpExec) */

  return(SUCCESS);
  }  /* END MNatRMStart() */






/**
 * Issue Native RM Stop command.
 *
 * @see MNatRMStart() - peer - start RM
 *
 * @param R (I)
 * @param Attr (I) [not used]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatRMStop(

  mrm_t                *R,    /* I */
  char                 *Attr, /* I (not used) */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  enum MBaseProtocolEnum Protocol = mbpNONE;

  const char *FName = "MNatRMStop";

  MDB(1,fNAT) MLog("%s(%s,Attr,EMsg,SC)\n",
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

  if (R->ND.URL[mrmRMStop] == NULL)
    {
    return(FAILURE);
    }

  Protocol = R->ND.Protocol[mrmRMStop];

  if (Protocol == mbpExec)
    {
    int rc;

    mstring_t Response(MMAX_LINE);

    rc = MNatDoCommand(
           &R->ND,
           NULL,
           mrmRMStop,
           Protocol,
           TRUE,
           R->ND.Host[mrmRMStop],
           R->ND.Path[mrmRMStop],
           &Response,  /* O */
           EMsg,
           NULL);

    if (rc == FAILURE)
      {
      /* RM stop failed */

      return(FAILURE);
      }
    }    /* END if (Protocol == mbpExec) */

  return(SUCCESS);
  }  /* END MNatRMStop() */





/**
 * Issue Native RM Initialize command.
 *
 * @see MNatRMStart() - peer - start RM
 * @see MRMRestore() - parent - re-initialize if failure detected
 * @see MNatInitialize() - parent
 *
 * @param R (I)
 * @param Attr (I) [not used]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatRMInitialize(

  mrm_t                *R,    /* I */
  char                 *Attr, /* I */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  enum MBaseProtocolEnum Protocol = mbpNONE;

  char CmdLine[MMAX_BUFFER];

  char AltArgs[MMAX_LINE];

  const char *FName = "MNatRMInitialize";

  MDB(1,fNAT) MLog("%s(%s,Attr,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

#ifndef __MOPT
  if (R == NULL)
    {
    return(FAILURE);
    }
#endif /* !__MOPT */

  if (SC != NULL)
    {
    *SC = mscNoError;
    }

  if (R->ND.URL[mrmRMInitialize] == NULL)
    {
    return(FAILURE);
    }

  Protocol = R->ND.Protocol[mrmRMInitialize];

  if (Protocol == mbpExec)
    {
    int rc;

    if (R->Variables != NULL)
      {
      char VarString[MMAX_BUFFER];

      MULLToString(R->Variables,TRUE,",",VarString,sizeof(VarString));

      AltArgs[0] = '\0';

      if (bmisset(&R->RTypes,mrmrtStorage))
        {
        snprintf(AltArgs,sizeof(AltArgs),"%s",
          (R->P[0].HostName != NULL) ? R->P[0].HostName : "");
        }
 
      snprintf(CmdLine,sizeof(CmdLine),"%s --replace=%s %s",
        R->ND.Path[mrmRMInitialize],
        VarString,
        AltArgs);
      }
    else
      {
      MUStrCpy(CmdLine,R->ND.Path[mrmRMInitialize],sizeof(CmdLine));
      }

    mstring_t Response(MMAX_LINE);

    rc = MNatDoCommand(
           &R->ND,
           NULL,
           mrmRMInitialize,
           Protocol,
           TRUE,
           R->ND.Host[mrmRMInitialize],
           CmdLine,    /* I */
           &Response,  /* O */
           EMsg,
           NULL);

    if (rc == FAILURE)
      {
      /* RM initialize failed */

      return(FAILURE);
      }
    }    /* END if (Protocol == mbpExec) */

  return(SUCCESS);
  }  /* END MNatRMInitialize() */






/**
 * Issue queue query request via native RM interface.
 *
 * @see MRMQueueQuery() - parent
 * @see MCredProcessConfig() - child
 * @see MClassProcessConfig() - child
 *
 * NOTE:  Allows any CLASSCFG[] based attributes 
 *
 * @param R (I)
 * @param QCountP (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNatQueueQuery(

  mrm_t                *R,       /* I */
  int                  *QCountP, /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  char       *ptr;
  char const *TokPtr;

  char    *nptr;
  char    *TokPtr2;

  char     tmpQBuf[MMAX_BUFFER];

  int      len;

  mclass_t *C;

  if (QCountP != NULL)
    *QCountP = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((R != NULL) && bmisset(&R->RTypes,mrmrtStorage))
    {
    return(SUCCESS);
    }

  if ((R == NULL) || (R->S == NULL))
    {
    return(FAILURE);
    }

  /* determine default ND structure */

  if (R->ND.URL[mrmQueueQuery] == NULL)
    {
    MDB(1,fNAT) MLog("INFO:     queue query action not specified for native interface %s (disabling function)\n",
      R->Name);

    if (bmisclear(&R->FnList))
      {
      MRMFuncSetAll(R);
      }

    bmunset(&R->FnList,mrmQueueQuery);

    return(SUCCESS);
    }

  if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, skipping queue query\n",
      R->Name);

    if (EMsg != NULL)
      {
      sprintf(EMsg,"RM in state %s",
        MRMState[R->State]);
      }

    return(FAILURE);
    }

  /* load queues from source */

  if (MNatDoCommand(
       &R->ND,
       NULL,
       mrmQueueQuery,
       R->ND.Protocol[mrmQueueQuery],
       TRUE,
       NULL,
       NULL,
       &R->ND.QueueQueryData,  /* O */
       EMsg,
       NULL) == FAILURE)
    {
    /* cannot query queue */

    return(FAILURE);
    }

  /* walk all queues, processing one class line at a time */

  len = strlen(R->ND.QueueQueryData.c_str());

  if (len + 1 < (int) R->ND.QueueQueryData.capacity())
    {
    /* MUBufGetMatchToken() must be doubly terminated */

    R->ND.QueueQueryData[len + 1] = '\0';
    }

  MUBufGetMatchToken(
    R->ND.QueueQueryData.c_str(),
    NULL,
    "\n",
    &TokPtr,
    tmpQBuf,
    sizeof(tmpQBuf));

  while (tmpQBuf[0] != '\0')
    {
    /* FORMAT:  <CLASSID> <ATTR>=<VAL>[ <ATTR>=<VAL>]... */

    nptr = MUStrTok(tmpQBuf," \t",&TokPtr2);

    if (nptr != NULL)
      {
      ptr = MUStrTok(NULL,"\n",&TokPtr2);

      if (MClassAdd(nptr,&C) == SUCCESS)
        {
        bmunset(&C->Flags,mcfRemote);

        /* NOTE:  use class processing routine to parse queue info */

        /* convert string to 'config file' format (no action required) */

        MCredProcessConfig((void *)C,mxoClass,NULL,ptr,&C->L,&C->F,FALSE,EMsg);

        MClassProcessConfig(C,ptr,FALSE);
        }
      }    /* END if (nptr != NULL) */

    MUBufGetMatchToken(
      NULL,
      NULL,
      "\n",
      &TokPtr,
      tmpQBuf,
      sizeof(tmpQBuf));
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MNatQueueQuery() */




/**
 * Extract node name and node state info from line.
 *
 * @param NData (I)
 * @param NState (O) [optional]
 * @param NName (O) [optional,minsize=MMAX_NAME] 
 * @param NOpsys (O) [optional,minsize=MMAX_NAME]
 */

int __MNatNodeGetState(

  char                *NData,
  enum MNodeStateEnum *NState,  /* O (optional) */
  char                *NName,   /* O (optional,minsize=MMAX_NAME) */
  char                *NOpsys)  /* O (optional,minsize=MMAX_NAME) */
 
  {
  char *ptr;

  if (NState != NULL)
    *NState = mnsNONE;

  if (NName != NULL)
    NName[0] = '\0';

  if (NOpsys != NULL)
    NOpsys[0] = '\0';

  if (NData == NULL)
    {
    return(FAILURE);
    }

  if (NState != NULL)
    {
    /* search for wiki node 'state' attribute */

    ptr = MUStrStr(NData,"state=",0,TRUE,FALSE);

    if (ptr != NULL)
      {
      ptr += strlen("state=");

      *NState = (enum MNodeStateEnum)MUGetIndexCI(ptr,MNodeState,TRUE,0);
      }
    }
 
  if (NOpsys != NULL)
    {
    /* search for wiki node 'active opsys' attribute */

    ptr = MUStrStr(NData,"os=",0,TRUE,FALSE);

    if (ptr != NULL)
      {
      char *tptr;

      ptr += strlen("os=");

      if ((tptr = strchr(ptr,';')) != NULL)
        *tptr = '\0';

      MUStrCpy(NOpsys,ptr,MMAX_NAME);

      if (tptr != NULL)
        *tptr = ';';
      }
    }
 
  if (NName != NULL)
    {
    sscanf(NData,"%64s",
      NName);
    }
 
  return(SUCCESS);
  }  /* END __MNatNodeGetState() */





/*
 *
 *
 */

int MNatRsvCtl(

  mrsv_t               *R,
  mrm_t                *RM,
  enum MRsvCtlCmdEnum   Cmd,
  void                **Response, /* O (minsize=MMAX_BUFFER if doing mrcmQuery) */
  char                 *EMsg,
  enum MStatusCodeEnum *Status)

  {
  int rc;

  char tmpBuffer[MMAX_BUFFER] = {0};

  const char *FName = "MNatRsvCtl";

  MDB(1,fNAT) MLog("%s(%s,%s,Value,EMsg,SC)\n",
    FName,
    (RM != NULL) ? RM->Name : "NULL",
    MRsvCtlCmds[mrcmNONE]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((RM == NULL) || (Cmd == mrcmNONE))
    {
    return(FAILURE);
    }

  if ((RM->State == mrmsDown) || (RM->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, rejecting info query request\n",
      RM->Name);

    return(FAILURE);
    }

  /* determine default ND structure */

  if (RM->ND.URL[mrmRsvCtl] != NULL)
    {
    MDB(1,fNAT) MLog("ALERT:    rsv ctl action not specified for native interface\n");

    if (EMsg != NULL)
      strcpy(EMsg,"rsv ctl action not specified");

    return(FAILURE);
    }

  mstring_t String(MMAX_LINE);

  rc = MNatDoCommand(
         &RM->ND,
         NULL,
         mrmRsvCtl,
         RM->ND.Protocol[mrmRsvCtl],
         TRUE,
         RM->ND.Host[mrmRsvCtl],
         tmpBuffer,   /* I - should be populated? */
         &String,
         EMsg,
         NULL);

  MUStrCpy((char *)Response,String.c_str(),MMAX_BUFFER);

  if (rc == FAILURE)
    {
    /* info query failed */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatRsvCtl() */





/**
 * Load cluster/workload data for specified iteration via Native interface.
 *
 * NOTE:  If NonBlock is set to false, Moab will block and wait for results.
 *
 * @see MNatDoCommand() - child
 * @see __MNatClusterProcessData() - peer - process cluster data
 * @see __MNatWorkloadProcessData() - peer - process workload data
 *
 * @param R (I) [modified]
 * @param NonBlock (I)
 * @param Iteration (I)
 * @param IsPending (O) [optional]
 * @param EMsg (O)
 * @param SC (O) [optional]
 */

int MNatGetData(

  mrm_t                *R,         /* I (modified) */
  mbool_t               NonBlock,  /* I */
  int                   Iteration, /* I */
  mbool_t              *IsPending, /* O (optional) */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  enum MBaseProtocolEnum Protocol = mbpNONE;

  mbool_t UseCachedQuery = FALSE;

  int     rc;

  char    CmdLine[MMAX_LINE];

  char   *BPtr;
  int     BSpace;

  long    Duration;

  enum MStatusCodeEnum tSC;

  mbool_t SkipClusterQuery = FALSE;

  const char *FName = "MNatGetData";

  MDB(1,fNAT) MLog("%s(%s,%s,%d,IsPending,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MBool[NonBlock],
    Iteration);

  /* NOTE:  load, but do not process raw cluster and workload data from Native interface */

  /* Step 1.0 - Initialize */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (IsPending != NULL)
    *IsPending = FALSE;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
    {
    MDB(1,fNAT) MLog("INFO:     RM %s state is down, skipping cluster query\n",
      R->Name);

    if (EMsg != NULL)
      strcpy(EMsg,"rm is down");

    return(FAILURE);
    }

  /* determine default ND structure and protocol */

  if (R->ND.URL[mrmClusterQuery] == NULL)
    {
    MDB(1,fNAT) MLog("INFO:     cluster query action not specified for native interface\n");

    if (EMsg != NULL)
      strcpy(EMsg,"cluster query action not specified");

    SkipClusterQuery = TRUE;
    }
  else
    {
    Protocol = R->ND.Protocol[mrmClusterQuery];
    }

  if (SkipClusterQuery == FALSE)
    {
    /* Step 3.0 - Load Aggegate Cluster Data */
   
    if (R->SubType != mrmstNONE)
      {
      /* performed query if scheduled, else use cached query data */
   
      if (R->PollTimeIsRigid == TRUE)
        {
        mulong tmpL;
   
        if (R->PollInterval > 0)
          tmpL = (MSched.Time + MSched.GreenwichOffset) % R->PollInterval;
        else
          tmpL = (MSched.Time + MSched.GreenwichOffset) % MSched.MaxRMPollInterval;
   
        if (MSched.Time - tmpL <= R->UTime)
          {
          UseCachedQuery = TRUE;
          }
        }   /* END if (R->PollTimeIsRigid == TRUE) */
      else if (MSched.Time - R->PollInterval < R->UTime)
        {
        UseCachedQuery = TRUE;
        }
   
      if (UseCachedQuery == TRUE)
        {
        if (MUStrIsEmpty(R->ND.ResourceQueryData.c_str()))
          {
          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"URL '%s' reports no information",
              R->ND.URL[mrmClusterQuery]);
            }
          }
   
        MDB(6,fNAT) MLog("INFO:     using cached cluster query data for native rm '%s'\n",
          R->Name);
        }  /* END if (UseCachedQuery == TRUE) */
      else
        {
        /* build command */
   
        MUSNInit(&BPtr,&BSpace,CmdLine,sizeof(CmdLine));
   
        if (R->ND.Path[mrmClusterQuery] != NULL)
          {
          if (bmisset(&R->IFlags,mrmifAllowDeltaResourceInfo) &&
             (R->UTime <= 0))
            {
            if (Protocol == mbpHTTP)
              {
              MUSNPrintF(&BPtr,&BSpace,"%s?delta",
                R->ND.Path[mrmClusterQuery]);
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"%s --delta",
                R->ND.Path[mrmClusterQuery]);
              }
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s",
              R->ND.Path[mrmClusterQuery]);
            }
          }
        else
          {
          /* ganglia and other non-exec/file based protocols do not use path */
   
          /* NO-OP */
          }
   
        switch (R->SubType)
          {
          case mrmstXT4:
   
            if (Protocol == mbpExec)
              {
              MUSNPrintF(&BPtr,&BSpace," --rm %s",
                R->Name);
              }
   
            break;
   
          default:
   
            /* NO-OP */
   
            break;
          }
   
        /* load aggregate data for entire cluster */
   
        MRMStartFunc(R,NULL,mrmXGetData);
 
        rc = MNatDoCommand(
           &R->ND,
           NULL,
           mrmResourceQuery,
           Protocol,
           TRUE,
           R->ND.Host[mrmClusterQuery],
           CmdLine,                /* I */
           &R->ND.ResourceQueryData, /* O */
           EMsg,                   /* O */
           &tSC);                  /* O */
 
        if (rc == FAILURE)
          {
          if (SC != NULL)
            *SC = tSC;
       
          if ((EMsg != NULL) && (EMsg[0] == '\0'))
            {
            snprintf(EMsg,MMAX_LINE,"URL '%s' cannot be loaded, SC=%d\n",
              R->ND.URL[mrmWorkloadQuery],
              tSC);
            }
       
          return(FAILURE);
          }

        MRMEndFunc(R,NULL,mrmXGetData,&Duration);
 
        MSched.LoadEndTime[mschpRMLoad]      += Duration;
        MSched.LoadStartTime[mschpRMProcess] += Duration;
 
        /* NOTE:  attempt auto-protocol detection */
   
        if (R->ND.EffProtocol[mrmClusterQuery] == mbpNONE)
          {
          mstring_t tmpBuf(MMAX_LINE);

          /* NOTE: only search first 128 chars for ganglia marker */

          if (!MUStrIsEmpty(R->ND.ResourceQueryData.c_str()))
            MStringSet(&tmpBuf,R->ND.ResourceQueryData.c_str());

          if (strstr(tmpBuf.c_str(),MGangliaXML[mxgMain]) != NULL)
            {
            R->ND.EffProtocol[mrmClusterQuery] = mbpGanglia;
            }
          else
            {
            R->ND.EffProtocol[mrmClusterQuery] = Protocol;
            }
          }    /* END if (R->ND.EffProtocol[mrmClusterQuery] == mpbNONE) */
   
        if (!MUStrIsEmpty(R->ND.ResourceQueryData.c_str()))
          {
          /* process cluster, removing comments, etc,
           * since the buffer is being altered, we need to alloc
           * a temp copied modifiable buffer from the mstring, passed to the
           * AdjustBuffer function, then reassign back to the
           * mstring
          */
          char *modifyBuffer = (char *) MUCalloc(1,R->ND.ResourceQueryData.length() << 1);
   
          /* Make a copy of the mstring */
          strcpy(modifyBuffer,R->ND.ResourceQueryData.c_str());

          /* pass the copy to be modified */
          MCfgAdjustBuffer(&modifyBuffer,FALSE,NULL,FALSE,TRUE,FALSE);

          /* reset the modified copy BACK to the mstring */
          R->ND.ResourceQueryData = modifyBuffer;

          /* Free the temp modify buffer */
          MUFree(&modifyBuffer);
          }
   
        R->UTime = MSched.Time;
        }      /* END else (UseCachedQuery == TRUE) */
      }        /* END if ((R->SubType != mrmstNONE) && ...) */
    }          /* END if (SkipClusterQuery == FALSE) */

  /* load workload info */

  if (R->ND.URL[mrmWorkloadQuery] == NULL)
    {
    MDB(1,fNAT) MLog("INFO:     workload query action not specified for native interface %s (disabling function)\n",
      R->Name);

    if (bmisclear(&R->FnList))
      {
      MRMFuncSetAll(R);
      }

    bmunset(&R->FnList,mrmWorkloadQuery);

    return(SUCCESS);
    }

  /* build command */

  MUSNInit(&BPtr,&BSpace,CmdLine,sizeof(CmdLine));

  if (R->ND.Path[mrmWorkloadQuery] != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s",
      R->ND.Path[mrmWorkloadQuery]);
    }

  switch (R->SubType)
    {
    case mrmstXT4:

      if (Protocol == mbpExec)
        {
        MUSNPrintF(&BPtr,&BSpace," --rm %s",
          R->Name);
        }

      break;

    default:

      /* NO-OP */

      break;
    }

  MRMStartFunc(R,NULL,mrmXGetData);

  rc = MNatDoCommand(
     &R->ND,
     NULL,
     mrmWorkloadQuery,
     R->ND.Protocol[mrmWorkloadQuery],
     TRUE,
     R->ND.Host[mrmWorkloadQuery],
     CmdLine,
     &R->ND.WorkloadQueryData, /* O */
     EMsg,                   /* O */
     &tSC);                  /* O */

  MRMEndFunc(R,NULL,mrmXGetData,&Duration);

  MSched.LoadEndTime[mschpRMLoad]      += Duration;
  MSched.LoadStartTime[mschpRMProcess] += Duration;

  if (rc == FAILURE)
    {
    if (SC != NULL)
      *SC = tSC;

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      snprintf(EMsg,MMAX_LINE,"URL '%s' cannot be loaded, SC=%d\n",
        R->ND.URL[mrmWorkloadQuery],
        tSC);
      }

    return(FAILURE);
    }

  /* buffer successfully loaded */

  MDB(2,fNAT) MLog("INFO:     loaded native workload buffer (%d bytes), processing jobs\n",
    (int)strlen(R->ND.WorkloadQueryData.c_str()));

  MDB(5,fNAT) MLog("INFO:     workload buffer ----\n%s\n",
    R->ND.WorkloadQueryData.c_str());

  R->WorkloadUpdateIteration = MSched.Iteration;

  return(SUCCESS);
  }  /* END MNatGetData() */





/**
 * Process resource data for specified resource manager.
 * 
 * @see MNatClusterQuery() - parent
 * @see __MNatNodeProcess() - child
 *
 * @param R (I)
 * @param ND (I)
 * @param GlobalIsNew (I)
 * @param IsMaster (I)
 * @param NodeAddedP
 * @param RCountP
 */

int __MNatClusterProcessData(

  mrm_t    *R,           /* I */
  mnat_t   *ND,          /* I */
  mbool_t   GlobalIsNew, /* I */
  mbool_t   IsMaster,    /* I */
  mbool_t  *NodeAddedP,  /* O (optional) */
  int      *RCountP)     /* O (optional) */

  {
  char const *Data;

  char const *TokPtr = NULL;

  mnode_t *N;

  mnl_t NodeList;       /* node range list */

  mstring_t tmpBuf(MMAX_LINE);

  char     NIDBuf[MMAX_BUFFER + 1];       /* must be large enough to handle NID ranges */

  int      nindex;

  mbool_t  IsNew;

  const char *FName = "__MNatClusterProcessData";

  MDB(4,fNAT) MLog("%s(%s,ND,%s,%s,NodeAddedP,RCountP)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MBool[GlobalIsNew],
    MBool[IsMaster]);

  if ((R == NULL) || (ND == NULL))
    {
    return(FAILURE);
    }

  Data = ND->ResourceQueryData.c_str();

  /* examine all nodes */

  if (R->SubType != mrmstNONE)
    {
    char NBuf[MMAX_BUFFER];

    int  len;

    if (Data != NULL)
      {
      len = strlen(ND->ResourceQueryData.c_str());

      if (len + 1 < (int) ND->ResourceQueryData.capacity())
        {
        /* MUBufGetMatchToken() must be doubly terminated */

        ND->ResourceQueryData[len + 1] = '\0';
        }
      }

    /* all needed data loaded, process each node referenced in ResourceQueryData */

    NBuf[0] = '\0';

    MUBufGetMatchToken(
      ND->ResourceQueryData.c_str(),
      NULL,
      "\n",
      &TokPtr,
      NBuf,
      sizeof(NBuf));

    MNLInit(&NodeList);

    while (NBuf[0] != '\0')
      {
      tmpBuf = NBuf;

      MUBufGetMatchToken(
        NULL,
        NULL,
        "\n",
        &TokPtr,        /* I/O */
        NBuf,           /* O */
        sizeof(NBuf));

      IsNew = GlobalIsNew;

      /* resource info is aggregated, extract info on all nodes from buffer */

      sscanf(tmpBuf.c_str(),"%65536s",
        NIDBuf);

      if (MUStringIsRange(NIDBuf) == TRUE)
        {
        /* Handle node ranges if node name has a range */

        int nindex;

        mbool_t CreateNodes;

        enum MWNodeAttrEnum AList[mwnaLAST]; /* list of attrs to copy to range of nodes */

        mnode_t  tmpN;                       /* temp node to hold attrs for range */
        mnode_t *nPtr = &tmpN;               

        char *tmpPtr;

        /* convert Name to node range - create nodes if they don't yet exist */

        if (bmisset(&R->Flags,mrmfNoCreateAll) ||
            bmisset(&R->Flags,mrmfNoCreateResource))
          {
          CreateNodes = FALSE;
          }
        else
          {
          CreateNodes = TRUE;

          if (NodeAddedP != NULL)
            *NodeAddedP = TRUE;
          }

        if (MUNLFromRangeString(
            NIDBuf,     /* I - node range expression, (modified as side-affect) */
            NULL,       /* I - delimiters */
            &NodeList,  /* O - nodelist */
            NULL,       /* O - nodeid list */
            0,          /* I */
            0,
            CreateNodes,
            FALSE) == FAILURE)
          {
          MDB(1,fNAT) MLog("ALERT:    cannot expand range-based tasklist '%s' in %s\n",
            NIDBuf,
            FName);

          MNLFree(&NodeList);

          continue;
          }

        /* populate tmp node with given attributes */

        mstring_t WikiBuf(MMAX_LINE);

        MWikiFromAVP(NULL,tmpBuf.c_str(),&WikiBuf);

        MNodeInitialize(nPtr,NULL);

        MCResInit(&nPtr->CRes);
        MCResInit(&nPtr->DRes);
        MCResInit(&nPtr->ARes);

        bmset(&nPtr->Flags,mnifIsTemp);

        MRMNodePreLoad(nPtr,mnsDown,R);

        if ((tmpPtr = MUStrChr(WikiBuf.c_str(),':')) == NULL) /* Wiki format jobid:attr=value;[attr=value;] */
          {
          MDB(1,fNAT) MLog("ALERT:    malformed wiki buffer. Missing ':'. \n");

          continue;
          }

        tmpPtr++; /* increment past ':' */

        /* apply attrs to tempNode and get attr list back to apply to range */

        MWikiNodeLoad(tmpPtr,nPtr,R,AList);

        /* apply nPtr attrs to each node in nodelist */

        for (nindex = 0;MNLGetNodeAtIndex(&NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          if (nindex >= MSched.M[mxoNode])
            break;

          if (N->State == mnsNONE)
            IsNew = TRUE;
          else
            IsNew = FALSE;

          __MNatNodeProcess(
            N,
            R,
            ND,
            tmpBuf.c_str(),  /* I */
            IsMaster,
            IsNew,
            AList,   /* I */
            nPtr);  /* I */
          }  /* END for (nindex) */

        MNLFree(&NodeList);

        if (RCountP != NULL)
          (*RCountP) += nindex;

        MNodeDestroy(&nPtr);
        }    /* END if (MUStringIsRange(NIDBuf)) */
      else
        {
        /* non-range node id specified */

        char *ptr;

        if (MNodeFind(NIDBuf,&N) == FAILURE)
          {
          mvm_t *V;

          char  *NIDPtr;
          mbool_t HasParent = FALSE;

          /* physical node not located */
          /*  possible VM or GEvent */

          V = NULL;

          if (!strncasecmp(NIDBuf,"vm:",strlen("vm:")))
            {
            /* vm reported by RM */

            NIDPtr = NIDBuf + strlen("vm:");

            /* Do MVMFind, not MVMAdd, in case VM is a destroyed VM that is still showing up */

            MVMFind(NIDPtr,&V);

            bmset(&V->Flags,mvmfCreationCompleted);
            }
          else if (!strncasecmp(NIDBuf,"gevent:",strlen("gevent:")))
            {
            /* This line is a gevent, not a vm */

            /* Parse out gevent name */
            NIDPtr = NIDBuf + strlen("gevent:");

            MWikiGEventUpdate(NIDPtr,tmpBuf.c_str(),NULL);

            continue;
            }
          else
            {
            NIDPtr = NIDBuf;
            }

          ptr = MUStrStr(tmpBuf.c_str(),"containernode=",0,TRUE,FALSE);

          if (ptr != NULL)
            {
            HasParent = TRUE;
            }

          /* load state info */

          ptr = MUStrStr(tmpBuf.c_str(),"state=",0,TRUE,FALSE);

          if (ptr != NULL)
            {
            ptr += strlen("state=");

            if (!strncasecmp(ptr,"down:destroyed",strlen("down:destroyed")))
              {
              /* ignore destroyed VM which has already been removed */

              continue;
              }
            }

          if ((V != NULL) || 
              (MVMFind(NIDBuf,&V) == SUCCESS))
            {
            /* VM located - populate VM attributes */

            MWikiVMUpdate(V,tmpBuf.c_str(),R,NULL);

            continue;
            }

          if (MVMFindCompleted(NIDBuf,&V) == SUCCESS)
            {
            /* Completed VM, update the VM but not the RM */

            if (V != NULL)
              MWikiVMUpdate(V,tmpBuf.c_str(),NULL,NULL);

            continue;
            }

          if ((HasParent == TRUE) && (MVMAdd(NIDBuf,&V) == SUCCESS))
            {
            /* VM located - populate VM attributes */

            MWikiVMUpdate(V,tmpBuf.c_str(),R,NULL);

            continue;
            }

          if (bmisset(&R->Flags,mrmfNoCreateAll) ||
              bmisset(&R->Flags,mrmfNoCreateResource) ||
             (MNodeAdd(NIDBuf,&N) == FAILURE))
            {
            /* RM can only update nodes discovered via other RM's - cannot add node */

            continue;
            }

          IsNew = TRUE;
 
          if (NodeAddedP != NULL)
            *NodeAddedP = TRUE;
          }  /* END if (MNodeFind(NIDBuf,&N) == FAILURE) */

        /* switch MSM+Provisioning master RM to compute master RM for power state */

        if (!IsNew && (N->RM != NULL) && (R != N->RM) && (N->RMList != NULL) &&
            (N->RMList[0] == R))
          {
          if (bmisset(&N->RM->RTypes,mrmrtProvisioning))
            {
            /* switch master RM */

#if !defined(NDEBUG)
            {
            int rmindex;

            for (rmindex = 1; N->RMList[rmindex] != NULL; ++rmindex) 
              assert(N->RMList[rmindex] != R);
            }
#endif

            N->RMList[0] = NULL;
            N->RM = NULL;
            
            MNodeAddRM(N,R);
            }
          }

        __MNatNodeProcess(
          N,
          R,
          ND,
          tmpBuf.c_str(),  /* I */
          IsMaster,
          IsNew,
          NULL,
          NULL);

        if (RCountP != NULL)
          (*RCountP)++;
        }  /* END else (MUStringIsRange(NIDBuf)) */
      }    /* END while (NBuf[0] != '\0') */
    }      /* END if ((R->SubType == mrmstAgFull) || (R->SubType == mrmstAgExt)) */
  else
    {
    /* walk all known nodes and load node data individually */

    IsNew = GlobalIsNew;

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        {
        MDB(1,fNAT) MLog("INFO:     all located non-native nodes loaded (%d)\n",
          nindex);

        break;
        }

      if (N->Name[0] == '\1')
        continue;

      __MNatNodeProcess(
        N,
        R,
        ND,
        NULL,
        IsMaster,
        IsNew,
        NULL,
        NULL);

      if (RCountP != NULL)
        (*RCountP)++;
      }   /* END if (MSched.ResourceList[0] == NULL) */
    }     /* END else (MSched.ResourceList[0] != NULL) */

  /* update global RM state */

  if (bmisset(&R->RTypes,mrmrtLicense))
    {
    mnode_t *GN;

    int rindex;

    GN = (mnode_t *)R->U.Nat.N;

    /* record license availability */

    /* base stats handled in MWikiNodeUpdateAttr() */

    for (rindex = 1;rindex < MSched.M[mxoxGRes];rindex++)
      {
      if (MSNLGetIndexCount(&R->U.Nat.ResConfigCount,rindex) == 0)
        continue;

      if (GN != NULL)
        {
        /* if (R->U.Nat.ResAvailCount[rindex] > 0) */

        if (MSNLGetIndexCount(&GN->ARes.GenericRes,rindex) > 0)
          MSNLSetCount(&R->U.Nat.ResAvailICount,rindex,MSNLGetIndexCount(&R->U.Nat.ResAvailICount,rindex) + 1);

        if (MSNLGetIndexCount(&GN->ARes.GenericRes,rindex) == MSNLGetIndexCount(&GN->CRes.GenericRes,rindex))
          MSNLSetCount(&R->U.Nat.ResFreeICount,rindex,MSNLGetIndexCount(&R->U.Nat.ResFreeICount,rindex) + 1);
        }
      }  /* END for (rindex) */

    R->U.Nat.ICount++;
    }    /* END if (bmisset(&R->RTypes,mrmrtLicense)) */

  MNLFree(&NodeList);

  return(SUCCESS);
  }  /* END __MNatClusterProcessData() */





/**
 * Process workload data for specified resource manager.
 *
 * @see MNatWorkloadQuery() - parent
 * @see __MNatXXXProcess() - child
 * @see MRMJobPreUpdate() - child
 * @see MRMJobPostUpdate() - child
 * @see MNatGetData() - populate 'Data'
 *
 * @param R          (I)
 * @param ND         (I)
 * @param Data       (I) [modified as side-affect]
 * @param WCountP    (O) [optional]
 * @param NewWCountP (O) [optional]
 *
 * NOTE:  do NOT initialize WCountP, NewWCountP
 */

int __MNatWorkloadProcessData(

  mrm_t      *R,
  mnat_t     *ND,
  const char *Data,
  int        *WCountP,
  int        *NewWCountP)

  {
  char *ptr;
  char *TokPtr = NULL;

  int   rc;

  mjob_t *J;

  enum MJobStateEnum Status;

  char SJobID[MMAX_NAME];
  char SpecJobID[MMAX_NAME];

  char tmpMessage[MMAX_LINE];

  if ((R == NULL) || (ND == NULL) || (Data == NULL))
    {
    return(FAILURE);
    }
 
  mstring_t AttrBuf(MMAX_LINE);
  mstring_t WikiBuf(MMAX_LINE);

  char *mutableData=NULL;
  MUStrDup(&mutableData,Data);

  ptr = MUStrTok(mutableData,"\n",&TokPtr);

  while (ptr != NULL)
    {
    /* process job description line */

    if ((ptr[0] == '#') || (ptr[0] == '\0'))
      {
      /* ignore comment line */

      ptr = MUStrTok(NULL,"\n",&TokPtr);

      continue;
      }

    if (strchr(ptr,'=') != NULL)
      {
      WikiBuf.clear();  /* reset string */

      rc = MWikiFromAVP(NULL,ptr,&WikiBuf);

      if (rc == FAILURE)
        {
        /* line too long */

        MDB(2,fNAT) MLog("INFO:     line '%.32s' too long - ignored (%d bytes)\n",
          ptr,
          (int)sizeof(WikiBuf));

        MMBAdd(&R->MB,"native job line too long",NULL,mmbtNONE,0,0,NULL);

        ptr = MUStrTok(NULL,"\n",&TokPtr);

        continue;
        }
      }  /* END if (strchr(ptr,'=') != NULL) */
    else
      {
      MStringSetF(&WikiBuf,ptr);
      }

    ptr = WikiBuf.c_str();

    AttrBuf.clear();  /* reset string */

    if ((MWikiGetAttr(
           R,
           mxoJob,
           SpecJobID,       /* O */
           (int *)&Status,  /* O */
           NULL,
           &AttrBuf,         /* O */
           &ptr) == FAILURE) ||
        (Status == mjsNONE))
      {
      /* cannot locate status info for job */

      if (!bmisset(&R->Flags,mrmfNoCreateAll))
        {
        /* NOTE:  'non-master' RM's do not need to report job status */

        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"info corrupt for job %s - missing 'state' attribute",
          SpecJobID);

        MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

        ptr = MUStrTok(NULL,"\n",&TokPtr);

        continue;
        }

      /* slave RM does not require RM-specific status */

      if (MJobFind(SpecJobID,&J,mjsmBasic) == FAILURE)
        break;

      Status = J->State;
      }  /* END if ((MWikiGetAttr() == FAILURE) || ...) */

    MJobGetName(NULL,SpecJobID,R,SJobID,sizeof(SJobID),mjnShortName);

    if ((MSched.SpecJobs != NULL) || (MSched.IgnoreJobs != NULL))
      {
      if (MUCheckList(SJobID,MSched.SpecJobs,MSched.IgnoreJobs) == FALSE)
        {
        ptr = MUStrTok(NULL,"\n",&TokPtr);

        continue;
        }
      }

    if (WCountP != NULL)
      (*WCountP)++;

    MDB(3,fNAT) MLog("INFO:     processing job %s\n",
      SJobID);

    switch (Status)
      {
      case mjsIdle:
      case mjsHold:
      case mjsUnknown:
      case mjsStarting:
      case mjsRunning:
      case mjsSuspended:

        if (MJobFind(SJobID,&J,mjsmExtended) == SUCCESS)
          {
          /* update existing job */

          /* pre-process the job before we do anything else to it */

          MNatJobPreUpdate(J,R);

          if (!bmisset(&R->Flags,mrmfNoCreateAll))
            MRMJobPreUpdate(J);

          if ((R->NoAllocMaster == FALSE) &&
             ((R->SubType == mrmstXT3) ||
             (R->SubType == mrmstXT4) ||
             (R->SubType == mrmstX1E)))
            MWikiJobUpdate(AttrBuf.c_str(),J,J->Req[1],R);
          else
            MWikiJobUpdate(AttrBuf.c_str(),J,NULL,R);

          /* do not re-read RMXString for existing job */
/*
          if (J->RMXString != NULL)
            MJobProcessExtensionString(J,J->RMXString,NULL,NULL);
*/

          J->MTime = MSched.Time;

          /* 12-11-09 BOC - Commenting out as the the call to MRMJobPostUpdate
           * was added after it was commented out in MWikiJobUpdate(). The call,
           * MRMJobPostUpdate, in MWikiJobUpdate was *un*commented to put the 
           * behavior back to the way it was in 5.3, since MRMJobPostUpdate
           * handles the wiki tasklist attr and puts slurm jobs in the MAQ. 

          if (!bmisset(&R->Flags,mrmfNoCreateAll))
            MRMJobPostUpdate(J,NULL,J->State,R);
           */
          }
        else if (bmisset(&R->Flags,mrmfNoCreateAll))
          {
          MDB(1,fWIKI) MLog("INFO:     rm is slave (ignoring job '%s')\n",
            SJobID);
          }
        else if (MJobCreate(SJobID,TRUE,&J,NULL) == SUCCESS)
          {
          int *TaskList = NULL;

          mreq_t *DRQ = NULL;

          mjob_t *SJ;

          mbool_t JobRequestsProcs = TRUE;

          int     rc;

          if (NewWCountP != NULL)
            (*NewWCountP)++;

          /* if new job, load data */

          MRMJobPreLoad(J,SJobID,R);

          /* determine if job template exists for 'service' job */

          /* NOTE:  should call a routine which looks up job templates based
                    on jobid and applies template defaults 'before' overlaying
                    these defaults w/RM-specified values.  THIS IS NOT WHAT
                    MTJOBLOAD() DOES. */

          /* MTJobLoad(J,NULL,R,NULL); */

          TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxNodeCount);

          if ((R->SubType == mrmstXT3) ||
              (R->SubType == mrmstXT4) ||
              (R->SubType == mrmstX1E))
            {
            if (J->DRMJID == NULL)
              MJobSetAttr(J,mjaDRMJID,(void **)SpecJobID,mdfString,mSet);

            if (R->NoAllocMaster == FALSE)
              {
              /* create single-task master req for load balancing */

              /* NOTE:  both TASKS=0 and NODES=0 should exist in zero proc jobs */

              if (strstr(AttrBuf.c_str(),"DPROCS=0"))
                JobRequestsProcs = FALSE;

              if (J->Req[0] == NULL)
                {
                if (JobRequestsProcs == TRUE)
                  {
                  MReqCreate(J,NULL,&J->Req[0],FALSE);

                  /* initialize compute req */

                  MRMReqPreLoad(J->Req[0],R);
                  }
                }

              if (MJobAddRequiredGRes(J,"master",1,mxaVRes,TRUE,TRUE) == FAILURE)
                {
                MJobSetHold(J,mhBatch,0,mhrRMReject,"job is corrupt - cannot add master req");

                ptr = MUStrTok(NULL,"\n",&TokPtr);

                continue;
                }

              DRQ = (J->Req[1] != NULL) ? J->Req[1] : J->Req[0];

              /* allow optional taskcount */

              bmset(&J->IFlags,mjifTaskCountIsDefault);
              }  /* END if (R->NoAllocMaster == FALSE) */
            else
              {
              if (strstr(AttrBuf.c_str(),"DPROCS=0"))
                {
                if (MJobAddRequiredGRes(J,"master",1,mxaVRes,TRUE,TRUE) == FAILURE)
                  {
                  MJobSetHold(J,mhBatch,0,mhrRMReject,"job is corrupt - cannot add master req");

                  ptr = MUStrTok(NULL,"\n",&TokPtr);

                  continue;
                  }

                JobRequestsProcs = FALSE;
                }
              }
            }    /* END if ((R->SubType == mrmstXT3) || (R->SubType == mrmstX1E)) */

          if (J->SRMJID == NULL)
            MJobSetAttr(J,mjaSRMJID,(void **)SJobID,mdfString,mSet);

          rc = MWikiJobLoad(
                SJobID,
                AttrBuf.c_str(),
                J,
                DRQ,
                TaskList,  /* O */
                R,
                tmpMessage);  /* O */

          if (MTJobFind(SJobID,&SJ) == SUCCESS)
            {
            /* matching job template found */

            MJobApplySetTemplate(J,SJ,NULL);
            }

          if (JobRequestsProcs == FALSE)
            {
            if (R->NoAllocMaster == FALSE)
              {
              /* restore defaults for noproc job */

              J->Request.TC = 1;

              J->Req[0]->DRes.Procs = 0;
              J->Req[0]->TaskCount = 1;
              }
            else
              {
              /* handle identically for now??? */

              J->Request.TC = 1;

              J->Req[0]->DRes.Procs = 0;
              J->Req[0]->TaskCount = 1;
              }
            }

          /* TaskCountIsDefault allows MWikiJobLoad() to succeed with no
             job taskcount specified */

          bmunset(&J->IFlags,mjifTaskCountIsDefault);

          if (rc == FAILURE)  /* O */
            {
            char tmpLine[MMAX_LINE];

            MDB(1,fWIKI) MLog("ALERT:    cannot load wiki job '%s'\n",
              SJobID);

            snprintf(tmpLine,sizeof(tmpLine),"info corrupt for job %s - %s",
              SJobID,
              tmpMessage);

            MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

            MJobRemove(&J);

            ptr = MUStrTok(NULL,"\n",&TokPtr);

            continue;
            }

          if ((R->NoAllocMaster == FALSE) &&
             ((R->SubType == mrmstXT3) ||
              (R->SubType == mrmstXT4) ||
              (R->SubType == mrmstX1E)))
            {
            /* initialize single-task master node req */

            if (bmisset(&J->Flags,mjfInteractive))
              {
              /* add login node requirement */

              MDB(3,fWIKI) MLog("INFO:     interactive XT3 job %s set to use %s as master host\n",
                J->Name,
                (J->SubmitHost != NULL) ? J->SubmitHost : "NULL");

              MJobSetAttr(J,mjaReqHostList,(void **)J->SubmitHost,mdfString,mSet);
              }
            else
              {
              /* add yod node requirement */

              MReqSetAttr(
                J,
                J->Req[0],  /* gres req guaranteed to be req 0 from MJobAddRequiredGRes() */
                mrqaReqNodeFeature,
                (void **)"yod",
                mdfString,
                mAdd);
              }
            }   /* END if ((R->NoAllocMaster == FALSE) && ...) */

          MRMJobPostLoad(J,TaskList,R,NULL);

          if (J->RMXString != NULL)
            MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,NULL);

          J->MTime = MSched.Time;

          MDB(2,fWIKI)
            MJobShow(J,0,NULL);
 
          MUFree((char **)&TaskList);
          }
        else
          {
          MDB(1,fWIKI) MLog("ERROR:    job buffer is full  (ignoring job '%s')\n",
            SJobID);
          }

        break;

      case mjsRemoved:
      case mjsCompleted:
      case mjsVacated:

        if (MJobFind(SJobID,&J,mjsmExtended) == SUCCESS)
          {
          /* if job never ran, remove record.  job cancelled externally */

          if ((J->State != mjsRunning) && (J->State != mjsStarting))
            {
            MDB(1,fWIKI) MLog("INFO:     job '%s' was cancelled externally\n",
              J->Name);

            if (MSDProcessStageOutReqs(J) == FAILURE)
              {
              MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"data stage-out failed");

              continue;
              }

            /* remove job from joblist */

            MJobCheckEpilog(J);

            if (J->EpilogTList != NULL)
              {
              if (J->SubState == mjsstEpilog)
                {             
                /* we are still running the epilog ... don't remove the job */

                /* FIXME: This only gives it one second chance to complete.
                   If it doesn't complete in this time it's done for. 
                   We need to fix it so it will allow the epilog to run longer
                   if it needs to or timeout. */

                continue;
                }
              }

            MJobProcessRemoved(&J);

            /* if we call this then jobs will be charged, but completed jobs
               will be charged each time Moab restarts */

/*
            MJobProcessRemoved(&J);
*/

            break;
            }

          if (!bmisset(&R->Flags,mrmfNoCreateAll))
            MRMJobPreUpdate(J);

          if ((R->NoAllocMaster == FALSE) &&
             ((R->SubType == mrmstXT3) ||
             (R->SubType == mrmstXT4) ||
             (R->SubType == mrmstX1E)))
            MWikiJobUpdate(AttrBuf.c_str(),J,J->Req[1],R);
          else
            MWikiJobUpdate(AttrBuf.c_str(),J,NULL,R);

          if (J->State == mjsCompleted)
            {
            if (J->EpilogTList != NULL)
              {
              if ((!bmisset(&J->IFlags,mjifRanEpilog)) &&           
              (J->SubState != mjsstEpilog))
                {
                /* job is not currently in epilog */
   
                MJobLaunchEpilog(J);
   
                J->SubState = mjsstEpilog;
   
                bmset(&J->IFlags,mjifRanEpilog);
   
                J = NULL;
                }
              else
                {
                mtrig_t *T;

                MJobCheckEpilog(J);

                T = (mtrig_t *)MUArrayListGetPtr(J->EpilogTList,0);

                if (!MTRIGISRUNNING(T))
                  {
                  J->SubState = mjsstNONE;
                  }
                }
              }  /* END if (J->EpilogTList != NULL) */
            }

          if (bmisset(&R->Flags,mrmfFullCP))
            {
            char FileName[MMAX_PATH_LEN];

            sprintf(FileName,"%s/%s.cp",
              MSched.SpoolDir,
              J->Name);

            MFURemove(FileName);
            }

          if (MSDProcessStageOutReqs(J) == FAILURE)
            {
            MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"data stage-out failed");

            continue;
            }

          switch (Status)
            {
            case mjsRemoved:
            case mjsVacated:

              if (MSched.Time < (J->StartTime + J->WCLimit))
                {
                MJobProcessRemoved(&J);
                }
              else
                {
                char TString[MMAX_LINE];

                MULToTString(J->WCLimit,TString);

                sprintf(tmpMessage,"JOBWCVIOLATION:  job '%s' exceeded WC limit %s\n",
                  J->Name,
                  TString);

                MSysRegEvent(tmpMessage,mactNONE,1);

                MDB(3,fWIKI) MLog("INFO:     job '%s' exceeded wallclock limit %s\n",
                  J->Name,
                  TString);

                MJobProcessCompleted(&J);
                }

              break;

            case mjsCompleted:

              MJobProcessCompleted(&J);

              break;

            default:

              /* unexpected job state */

              MDB(1,fWIKI) MLog("WARNING:  unexpected job state (%d) detected for job '%s'\n",
                Status,
                J->Name);

              break;
            }   /* END switch (Status)                        */
          }     /* END if (MJobFind(SJobID,&J,NULL,mjsmExtended) == SUCCESS) */
        else if ((MSched.Iteration == 0) && (MJobCFind(SJobID,&J,mjsmBasic) == FAILURE))
          {
          mbool_t IsFound = FALSE;

          /* create temporary job so checkpoint info can be loaded */

          if (MJobCreate(SJobID,TRUE,&J,NULL) == SUCCESS)
            {
            if (MSDProcessStageOutReqs(J) == FAILURE)
              {
              MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"data stage-out failed");

              continue;
              }

            MCPRestore(mcpJob,SJobID,(void *)J,&IsFound);

            /* NOTE:  checkpoint info is lost!! (FIXME) */

            if (IsFound == TRUE)
              {
              MDB(6,fPBS) MLog("INFO:     job '%s' exited while moab not running\n",
                J->Name);

              /* job exited while Moab was down */

              if (!bmisset(&R->Flags,mrmfNoCreateAll))
                {
                /* NOTE:  MJobCreate() should create J->Req[0] */

                MRMJobPreLoad(J,SJobID,R);

                if (J->Req[0] == NULL)
                  MReqCreate(J,NULL,&J->Req[0],FALSE);

                MRMReqPreLoad(J->Req[0],R);
                }

              MJobSetState(J,Status);

              if ((R->NoAllocMaster == FALSE) &&
                 ((R->SubType == mrmstXT3) ||
                  (R->SubType == mrmstXT4) ||
                  (R->SubType == mrmstX1E)))
                MWikiJobUpdate(AttrBuf.c_str(),J,J->Req[1],R);
              else
                MWikiJobUpdate(AttrBuf.c_str(),J,NULL,R);

              if (bmisset(&R->Flags,mrmfFullCP))
                {
                char FileName[MMAX_PATH_LEN];

                sprintf(FileName,"%s/%s.cp",
                  MSched.SpoolDir,
                  J->Name);

                MFURemove(FileName);
                }

              MJobProcessCompleted(&J);
              }  /* END if (IsFound == TRUE) */
            else
              {
              /* completed job already processed */

              /* use MJobRemove() to remove from job table */

              MJobRemove(&J);

              /* ignore job */

              MDB(4,fPBS) MLog("INFO:     ignoring job '%s'  (state: %s) - completed before restart\n",
                SJobID,
                MJobState[Status]);
              }  /* END else (IsFound == TRUE) */
            }    /* END if (MJobCreate(SJobID,TRUE,&J) == SUCCESS) */
          else
            {
             MDB(4,fPBS) MLog("INFO:     ignoring job '%s'  (state: %s) - cannot add job record\n",
              SJobID,
              MJobState[Status]);
            }
          }
        else
          {
          /* ignore job for now */

          MDB(4,fWIKI) MLog("INFO:     ignoring job '%s'  (state: %s)\n",
            SJobID,
            MJobState[Status]);
          }

        break;

      default:

        MDB(1,fWIKI) MLog("WARNING:  job '%s' detected with unexpected state '%d'\n",
          SJobID,
          Status);

        break;
      }  /* END switch (Status) */

    /* get pointer to next job */

    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }    /* END while (ptr != NULL) */

  MUFree(&mutableData);
  return(SUCCESS);
  }  /* END __MNatWorkloadProcessData() */




/**
 * Create a resource on the specified RM.
 *
 * @param RM
 * @param OType
 * @param Data
 * @param Output
 * @param EMsg
 * @param SC
 */

int MNatResourceCreate(
 
  mrm_t                  *RM,
  enum   MXMLOTypeEnum    OType,
  char                   *Data,
  mstring_t              *Output,
  char                   *EMsg,
  enum   MStatusCodeEnum *SC)

  {
  char CmdLine[MMAX_LINE*10];

  const char *FName = "MNatResourceCreate";

  MDB(1,fRM) MLog("%s(%s,%s,Data,Output,EMsg,SC)\n",
    FName,
    (RM != NULL) ? RM->Name : "",
    MXO[OType]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Output != NULL)
    {
    MStringSet(Output,"\0");
    }

  if (SC != NULL)
    *SC = mscNoError;

  if ((RM == NULL) || (OType == mxoNONE) || (Output == NULL))
    {
    return(FAILURE);
    }

  if (RM->ND.Protocol[mrmResourceCreate] == mbpNONE)
    {
    MDB(1,fNAT) MLog("ALERT:    cannot create %s (%s)\n",
      MXO[OType],
      "no resource create operation defined");

    if (EMsg != NULL)
      MUStrCpy(EMsg,"ERROR: no resource create operation defined",MMAX_LINE);

    return(FAILURE);
    }

  switch (OType)
    {
    case mxoxVM:

      {
      char  tmpLine[MMAX_LINE*10];

      char *ptr;
      char *TokPtr;
      char *HypervisorName;
      char *VMID;

      char *BPtr;
      int BSpace;

      mbitmap_t SetAttrs;

      MUSNInit(&BPtr,&BSpace,CmdLine,sizeof(CmdLine));

      MUStrCpy(tmpLine,Data,sizeof(tmpLine));

      /* Data FORMAT: Hypervisor:VMID:wiki-attributes */

      TokPtr = tmpLine;

      if ((HypervisorName = MUStrTokEPlus(NULL,":,",&TokPtr)) == NULL)
        {
        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"No Hypervisor Specified in Data Line");
        return(FAILURE);
        }

      if ((VMID = MUStrTokEPlus(NULL,":,",&TokPtr)) == NULL)
        {
        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"No VM ID Specified in Data Line");
        return(FAILURE);
        }

      MUSNPrintF(&BPtr,&BSpace,"%s vm %s:%s",
        (RM->ND.Path[mrmResourceCreate] != NULL) ? RM->ND.Path[mrmResourceCreate] : MNAT_NODEMODIFYPATH,
        HypervisorName,
        VMID);

      while ((ptr = MUStrTokEPlus(NULL,":,",&TokPtr)) != NULL)
        {
        char *Attribute;
        char *Value;
        char *TokPtr2 = ptr;
        enum MWNodeAttrEnum AttrType = mwnaNONE;

        Attribute = MUStrTokEPlus(NULL,"=",&TokPtr2);
        Value = TokPtr2;

        if ((Attribute == NULL) || (Value == NULL))
          {
          MDB(5,fSTRUCT) MLog("ERROR:     encountered invalid wiki attribute "
            "while reading %s\n",
            Data);

          continue;
          }

        AttrType = (enum MWNodeAttrEnum)MUGetIndexCI(Attribute,MWikiNodeAttr,FALSE,mwnaNONE);

        if (AttrType == mwnaNONE)
          {
          if (!strcmp(Attribute,"operationid"))
            {
            MUSNPrintF(&BPtr,&BSpace," operationid=%s",
              Value);

            continue;
            }
          else if (!strncmp(Attribute,"STORAGE",strlen("STORAGE")))
            {
            MUSNPrintF(&BPtr,&BSpace," %s=\"%s\"",
              Attribute,
              Value);
            }
          else if (!strncmp(Attribute,"ALIAS",strlen("ALIAS")))
            {
            MUSNPrintF(&BPtr,&BSpace," %s=%s",
              Attribute,
              Value);
            }
          else
            {
            MDB(5,fSTRUCT) MLog("ERROR:     Attribute %s is not a valid wiki attribute\n",
                Attribute);
            continue;
            }
          }
        else if (bmisset(&SetAttrs,AttrType))
          {
          MDB(5,fSTRUCT) MLog("ERROR:     Attribute %s is already set\n",
              Attribute);
          continue;
          }

        bmset(&SetAttrs,AttrType);

        switch (AttrType)
          {
          case mwnaCDisk:
          case mwnaCMemory:
          case mwnaCProc:
          case mwnaOS:

            MUSNPrintF(&BPtr,&BSpace," %s=%s",
              MWikiNodeAttr[AttrType],Value);

            break;

          case mwnaVariables:

            MUSNPrintF(&BPtr,&BSpace," %s=\"%s\"",          /* quote VARIABLE= string */
              MWikiNodeAttr[AttrType],Value);

            break;

          default:

            MDB(5,fSTRUCT) MLog("ERROR:     attribute %s is unsupported for VM "
              "creation\n",
              Attribute);

            break;
          }
        }

      if (!bmisset(&SetAttrs,mwnaOS))
        {
        char tmpLine[MMAX_LINE];
        char *Buffer = (EMsg == NULL) ? tmpLine : EMsg;

        snprintf(Buffer,MMAX_LINE,"ERROR:     VM creation requires that "
            "wiki attribute %s be set\n",
            MWikiNodeAttr[mwnaOS]);

        MDB(5,fSTRUCT) MLog("%s",Buffer);

        bmclear(&SetAttrs);

        return(FAILURE);
        }

      bmclear(&SetAttrs);
      }

      break;

    default:

      if (EMsg != NULL)
        MUStrCpy(EMsg,"object creation not supported for type",MMAX_LINE);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (OType) */

  if (MNatDoCommand(
       &RM->ND,
       NULL,
       mrmResourceCreate,
       RM->ND.Protocol[mrmResourceCreate],
       FALSE,  /* IsBlocking */
       NULL,
       CmdLine,    /* I */
       Output,
       EMsg,
       NULL) == FAILURE)
    {
    /* cannot create resource */

    MDB(1,fNAT) MLog("ALERT:    cannot create resource %s (%s)\n",
      MXO[OType],
      Output->c_str());

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNatCreateResource() */

/* END MNatI.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
