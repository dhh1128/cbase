/* HEADER */

/**
 * @file moab-wiki.h
 *
 * Moab Wiki Header
 */
        
#ifndef __MWIKI_H__
#define __MWIKI_H__

/* sync w/MWMCommandList */

int MWikiGetAttr(mrm_t *,enum MXMLOTypeEnum,char *,int *,char *,mstring_t *,char **);
int MWikiUpdateJob(char *,mjob_t *,int);

/* sync w/MWMCommandList[] */

enum MWCmdEnum {
  mwcmdNone = 0,
  mwcmdSubmitJob,
  mwcmdGetJobs,
  mwcmdGetNodes,
  mwcmdUpdateJob,
  mwcmdStartJob,
  mwcmdCancelJob,
  mwcmdSuspendJob,
  mwcmdResumeJob,
  mwcmdRequeueJob,
  mwcmdCheckPointJob,
  mwcmdMigrateJob,
  mwcmdPurgeJob,
  mwcmdModifyJob,
  mwcmdStartTask,
  mwcmdKillTask,
  mwcmdSuspendTask,
  mwcmdResumeTask,
  mwcmdGetTaskStatus,
  mwcmdGetNodeStatus,
  mwcmdGetJobStatus,
  mwcmdUpdateTaskStatus,
  mwcmdUpdateNodeStatus,
  mwcmdUpdateJobStatus,
  mwcmdAllocateNode,
  mwcmdStartNodeman,
  mwcmdSpawnTaskman,
  mwcmdSpawnJobman,
  mwcmdReconfig,
  mwcmdShutdown,
  mwcmdDiagnose,
  mwcmdReparent,
  mwcmdSignalJob,
  mwcmdCheckJob,      /* verify job/task mix is valid (maps to SLURM willrun) */
  mwcmdLAST };


/* sync w/MWikiNodeAttr[] */

enum MWNodeAttrEnum {
  mwnaNONE = 0,
  mwnaAClass,
  mwnaADisk,
  mwnaAFS,
  mwnaAMemory,
  mwnaAProc,
  mwnaArch,
  mwnaARes,      /* available generic resources */
  mwnaASwap,
  mwnaAttr,      /* generic attributes */
  mwnaCat,
  mwnaCClass,
  mwnaCDisk,
  mwnaCFS,
  mwnaChargeRate,
  mwnaCMemory,
  mwnaContainerNode,
  mwnaCProc,
  mwnaCPULoad,
  mwnaCPUSpeed,
  mwnaCRes,      /* configured generic resources */
  mwnaCSwap,
  mwnaCurrentTask,
  mwnaFeature,
  mwnaGEvent,    /* generic event */
  mwnaGMetric,   /* generic metric */
  mwnaIdleTime,  /* duration since last local mouse or keyboard activity */
  mwnaIOLoad,
  mwnaMaxTask,
  mwnaMessage,
  mwnaName,
  mwnaNodeIndex, /* N->Index */
  mwnaOSList,
  mwnaOS,
  mwnaOther,
  mwnaPartition,
  mwnaPowerIsEnabled,
  mwnaPriority,
  mwnaRack,
  mwnaSlot,
  mwnaSpeed,
  mwnaState,
  mwnaTransferRate,
  mwnaType,
  mwnaUpdateTime,
  mwnaVarAttr,
  mwnaVariables,
  mwnaVMOSList,
  mwnaNetAddr,
  mwnaRM,               /* PRINT ONLY (N->RM) */
  mwnaEffNAccessPolicy, /* PRINT ONLY (N->EffNAPolicy) */
  mwnaXRes,
  mwnaLAST };


/* sync w/MWikiJobAttr[] */

enum MWJobAttrEnum {
  mwjaNONE = 0,
  mwjaGAttr,
  mwjaGMetric,
  mwjaGEvent,       /* Used for pending action jobs, currently sent to messages (MB) */
  mwjaUpdateTime,
  mwjaEstWCTime,
  mwjaExitCode,
  mwjaState,
  mwjaWCLimit,
  mwjaTasks,
  mwjaNodes,
  mwjaMaxNodes,
  mwjaHoldType,
  mwjaGeometry,
  mwjaQueueTime,
  mwjaStartDate,
  mwjaStartTime,
  mwjaCheckpointStartTime,
  mwjaCompletionTime,
  mwjaUName,
  mwjaGName,
  mwjaAccount,
  mwjaEUser,      /**< execution user (see mwjaUser) */
  mwjaPref,
  mwjaRFeatures,
  mwjaXFeatures,
  mwjaRClass,
  mwjaROpsys,
  mwjaRArch,
  mwjaRMem,
  mwjaRAMem,
  mwjaRMemCmp,
  mwjaRAMemCmp,
  mwjaDMem,
  mwjaRDisk,
  mwjaRDiskCmp,
  mwjaDDisk,
  mwjaRSwap,
  mwjaRASwap,
  mwjaRSwapCmp,
  mwjaRASwapCmp,
  mwjaDSwap,
  mwjaCProcs,         /* configured processors per node */
  mwjaCProcsCmp,
  mwjaPartitionList,
  mwjaExec,
  mwjaArgs,
  mwjaIWD,
  mwjaComment,
  mwjaRejectionCount,
  mwjaRejectionMessage,  /* reason job is invalid - hold placed on job if specified */
  mwjaRejectionCode,
  mwjaEvent,
  mwjaTaskList,
  mwjaTaskPerNode,
  mwjaQOS,
  mwjaEndDate,
  mwjaDProcs,
  mwjaHostList,
  mwjaExcludeHostList,
  mwjaSuspendTime,
  mwjaRsvAccess,
  mwjaName,         /* jobid */
  mwjaJName,        /* user specified job name */
  mwjaEnv,          /* job environment variables */
  mwjaInput,
  mwjaOutput,
  mwjaError,
  mwjaFlags,        /* interactive status */
  mwjaMasterHost,   /* required node for master task */
  mwjaMasterReqAllocTaskList,
  mwjaPriority,     /* system priority */
  mwjaPriorityF,    /* priority function */
  mwjaReqRsv,       /* required reservation */
  mwjaShell,
  mwjaSID,          /* session id */
  mwjaSlaveScript,  /* job submission script to launch slave batch job for dynamic jobs */
  mwjaSubmitDir,
  mwjaSubmitHost,   /* submit host */
  mwjaSubmitString,
  mwjaSubmitType,
  mwjaTemplate,
  mwjaTrigger,
  mwjaCheckpointFile,     /* name of checkpoint file used for job */
  mwjaNodeSet,
  mwjaNodeAllocationPolicy,
  mwjaUPriority,
  mwjaUProcs,
  mwjaVariables,
  mwjaCGRes,
  mwjaAGRes,
  mwjaDGRes,
  mwjaRSoftware,
  mwjaWCKey,
  mwjaSRM,            /* PRINT ONLY (J->SRM)*/
  mwjaCommand,        /* PRINT ONLY (J->E.Cmd) */
  mwjaSubmitTime,     /* PRINT ONLY (J->SubmitTime) */
  mwjaDispatchTime,   /* PRINT ONLY (J->DispatchTime) */
  mwjaSystemQueueTime,/* PRINT ONLY (J->SystemQueueTime) */
  mwjaRequestedTC,    /* PRINT ONLY (J->Request.TC) */
  mwjaRequestedNC,    /* PRINT ONLY (J->Request.NC) */
  mwjaRequestedQOS,   /* PRINT ONLY (J->QReq) */
  mwjaBypassCount,    /* PRINT ONLY (J->BypassCount) */
  mwjaPartition,      /* PRINT ONLY (J->Req[0]->PtIndex) */
  mwjaPSUtilized,     /* PRINT ONLY (J->PSUtilized) */
  mwjaRMXString,      /* PRINT ONLY (J->RMXString) */
  mwjaTaskMap,        /* PRINT ONLY (J->TaskMap) */
  mwjaRMSubmitStr,    /* PRINT ONLY (J->RMSubmitString) */
  mwjaEffQDuration,   /* PRINT ONLY (J->EffQueueDuration) */
  mwjaMessage,        /* PRINT ONLY (J->MMB) */
  mwjaVMID,           /* PRINT ONLY (J->System->VM) */
  mwjaLAST };


/* sync w/MWikiSDAttr[] */

enum MWSDAttrEnum {
  mwsdaNONE = 0,
  mwsdaBytesTransferred, /* current size of destination file */
  mwsdaEndTime,
  mwsdaError,
  mwsdaFileSize,         /* size of source file */
  mwsdaRemoveTime,       /* amount of time after the end of the transmission that this record will remain persistent */
  mwsdaSourceURL,
  mwsdaStartTime,
  mwsdaState,
  mwsdaUser };


/* sync w/MWikiRsvAttr[] 
 *
 * Currently these are all PRINT ONLY
 */


enum MWRsvAttrEnum {
  mwraNONE = 0,
  mwraCreationTime,
  mwraStartTime,
  mwraEndTime,
  mwraAllocTC,
  mwraAllocNC,
  mwraTAPS,
  mwraTIPS,
  mwraAllocNodeList,
  mwraOwner,
  mwraName,
  mwraACL,
  mwraPriority,
  mwraType,
  mwraSubType,
  mwraLabel,
  mwraRsvGroup,
  mwraFlags,
  mwraMessage,
  mwraLAST };


#endif /* __MWIKI_H__ */

