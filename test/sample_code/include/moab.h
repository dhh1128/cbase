/* HEADER */

/**
 * @file moab.h
 *
 * Moab Header
 */

#if !defined(__MOAB_H)
#define __MOAB_H

/* convienence defines */
#if defined(__AIX52) || defined(__AIX53) || defined(__AIX54)
#define __AIX 1
#endif /* __AIX */

#if defined(__SOLARISX86)
#define MNOREENTRANT
#endif

#if defined(__LINUX) && !defined(__cplusplus) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif /* END __LINUX */

/* system includes */

#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <clocale>
#include <csignal>
#include <ctime>
#include <utime.h>
#include <cstdarg>
#include <cctype>
#include <dirent.h>
#include <map>
#include <list>
#include <vector>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <set>

#if defined(__FREEBSD)
#include <sys/resource.h>
#endif  /* END __FREEBSD */

#include <regex.h>
#include <unistd.h>
#include <strings.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#ifdef MUSEMONGODB
#include <mongo/client/dbclient.h>
#endif

#if defined(__LINUX)
#include <linux/if.h>
#endif /* END __LINUX */

/* asserts 
 * NOTE:  NDEBUG must be defined or undefined, as desired, before assert is included
 * To enable asserts: ./configure --enable-asserts */
#if !defined(MASSERTFORCEON) 
#define NDEBUG
#endif 

#include <assert.h>
/* END asserts */

#if defined(__FREEBSD)
 #include <sys/mount.h>
#elif defined(__AIX)
 #include <sys/statfs.h>
#else
 #include <sys/statvfs.h>
#endif /* END __FREEBSD */

#define EMBED_BREAKPOINT asm volatile ("int3;")
/* END system includes */

#include "moab-build.h"
#include "moab-local.h"
#include "mcom.h"
#include "mapi.h"


/*
#include "memwatch.h"
*/

/*
#include "mmgr.h"
*/

#include "MXML.h"
#include "MUArray.h"
#include "MString.h"


#define MDEF_DATFILE "moab.dat"
#define MDEF_PRIVFILE "moab-private.cfg"

#ifndef MDEF_SHELL
#define MDEF_SHELL   "/bin/sh"
#endif /* MDEF_SHELL */

#define MBMSIZE(val) (((val) / 8) + (((val) % 8) == 0 ? 0 : 1) + 1)  /* add extra byte to account for 1 offset values like MMAX_QOS */
#define MARR_SIZE(arr) ((int)sizeof(arr) / sizeof((arr)[0]))

#define MDEF_DEPLIST_SIZE 4

/* MUCheckAuthFile  #defines 
 *
 * By setting MAUTH_FILE_ENV_OVERRIDE in the enviornment of moab
 * one can change the keyfile name
 */
#define MAUTH_FILE_ENV_OVERRIDE   "MAUTH_FILE"


#ifndef mulong
# define mulong unsigned long
#endif /* mulong */

/* NOTE:  In Moab 5.3 and higher, enable job level generic metrics by default */

/* enumerations */

/* sync w/MBaseProtocol[] */

enum MBaseProtocolEnum {
  mbpNONE = 0,
  mbpExec,
  mbpFile,
  mbpGanglia,
  mbpHTTP,
  mbpNoOp,
  mbpLAST };

/* sync w/MHVType[] */

enum MHVTypeEnum {
  mhvtNONE = 0,
  mhvtHyperV,
  mhvtESX,
  mhvtESX35,
  mhvtESX35i,
  mhvtESX40,
  mhvtESX40i,
  mhvtKVM,
  mhvtLinuxContainer,
  mhvtXEN,
  mhvtLAST };

#define MUHVALLOWSCOMPUTE(H) \
  ((((H) != mhvtESX) && ((H) != mhvtESX35) && ((H) != mhvtESX35i) && ((H) != mhvtESX40) && ((H) != mhvtESX40i)) ? \
    TRUE : FALSE)


/* sync w/MExportPolicy[] */

enum MExportPolicyEnum {
  mexpNONE = 0,
  mexpControl,
  mexpExecution,
  mexpInfo,
  mexpLAST };


/* NOTE: sync w/MDSProtocol[] */

enum MDSProtoEnum {
  mdspNONE = 0,
  mdspFTP,
  mdspGASS,
  mdspGPFS,
  mdspGridFTP,
  mdspHTTP,
  mdspHTTPS,
  mdspNFS,
  mdspPVFS,
  mdspSCP,
  mdspLAST };

#define MPARM_RSVPROFILE       "RSVPROFILE"

#define MDEF_MBYTE             1048576
#define MDEF_KBYTE             1024

#define MDEF_RMMAXHOP          1
#define MMAX_NETHOP            16  /* max network link hops */
#define MMAX_NETHOPGROUP       32  /* maximum number of hop groups at each hop level */

#define MDEF_TARGETXF          0.0
#define MDEF_TARGETQT          0

#define MDEF_QOSPRIORITY       0
#define MDEF_QOSDEDRESCOST    -1.0

#define MDEF_QOSPRIOACCRUALPOLICY mjpapNONE

#define MDEF_NODEWEIGHT        0
#define MDEF_PREEMPTPRIOWEIGHT 256.0
#define MDEF_PREEMPTRTIMEWIGHT 0
#define MDEF_XFMINWCLIMIT      0
#define MDEF_QOSXFWEIGHT       0
#define MDEF_QOSQTWEIGHT       0

#define MDEF_MINPROVDURATION   150
#define MDEF_SJOBDURATION      3600
#define MDEF_PROVDURATION      600

#define MDEF_MINVMOPDELAY      60   /* min wait time between subsequent VM operations on node */
#define MDEF_FUTUREVMLOADFACTOR 0.8

#define MDEF_UINTVAL           4294967295

#define MDEF_USPERSECOND       1000000 /* microseconds per second */
#define MDEF_MSPERSECOND       1000    /* milliseconds per second */

#define MDEF_THREADSTACKSIZE   (MDEF_MBYTE * 10)

#define MDEF_TP_HANDLER_THREADS 2 /* default number of thread pool handler threads */
#define MMAX_TP_HANDLER_THREADS 25 /* max number of thread pool handler threads */

#define MDEF_DYNAMIC_RM_FAILURE_WAIT_TIME 300

#define MMAX_CRED               1024

/*
#ifndef MCONST_CKEY
#define MCONST_CKEY "hello"
#endif
*/

#ifndef MCONST_LKEY
#define MCONST_LKEY "hello"
#endif  /* MCONST_LKEY */

#ifndef MUFREE
#  ifndef MPROFMEM
#define MUFREE (int (*)(void**))MUFree
#  else
#define MUFREE (int (*)(void**))MUProfFreeTrad
#  endif /* !MPROFMEM */
#endif  /* MUFREE */

#ifndef MJOBFREE
#define MJOBFREE (int (*)(void**))MJobDestroy
#endif  /* MJOBFREE */

#define MVMFREE ((int (*)(void **))MVMFree)

#define MVMMIGRATEUSAGEFREE (int (*)(void**))FreeVMMigrationUsage

/* determine endian-ness (change it so we are not basing this off of OS, but rather ARCH) */

#ifdef __BYTE_ORDER
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define MLITTLEENDIAN
# else
#  if __BYTE_ORDER == __BIG_ENDIAN
#   define MBIGENDIAN
#  else
Error: unknown byte order!
#  endif
# endif
#endif /* __BYTE_ORDER */

#ifdef BYTE_ORDER
# if BYTE_ORDER == LITTLE_ENDIAN
#  define MLITTLEENDIAN
# else
#  if BYTE_ORDER == BIG_ENDIAN
#   define MBIGENDIAN
#  else
Error: unknown byte order!
#  endif
# endif
#endif /* BYTE_ORDER */


#if defined(__AIX) || defined(__HPUX) || defined(__SOLARIS) || defined(__UNICOS) || defined(MBIGENDIAN)

/******
 * MBIG_ENDIAN is used for runtime checking of architecture
 * MBIGENDIAN  is used for compile-time checking of architecture
 ******/

/* big endian arch */
# define MBIG_ENDIAN    1
# define MLITTLE_ENDIAN 0

#else

/******
 * MLITTLE_ENDIAN is used for runtime checking of architecture
 * MLITTLEENDIAN  is used for compile-time checking of architecture
 ******/


/* little endian arch */
# define MBIG_ENDIAN    0
# define MLITTLE_ENDIAN 1

#endif /* defined(__AIX) || ... */

#if !defined(MBIGENDIAN) && !defined(MLITTLE_ENDIAN)
Error: unknown byte order!
#endif

/* see mcfg_t MCfg */

/* NOTE:  to add a new global config parameter:
    - select enum X for new policy
    - select object O for corresponding object
    - select default value V for new policy
    - add X to enum MCfgParamEnum (moab.h)
    - specify default value V as #define (moab.h)
    - add X, O to new entry in mcfg_t MCfg (MConst.c)
    - add X to MCfgSetVal() for object O (MConfig.c)
    - add case in M<O>ProcessOConfig()
    - add initialization to M<O>Initialize()
    - add case in M<O>SetAttr()
    - add case in M<O>AToString()
    - add case in M<O>OConfigShow
*/

enum MCfgParamEnum {
  mcoNONE = 0,
  mcoAccountingInterfaceURL,
  mcoAcctCfg,
  mcoAllowMultiReqNodeUse,
  mcoAdjUtlRes,
  mcoAdminCfg,
  mcoAdmin1Users,
  mcoAdmin2Users,
  mcoAdmin3Users,
  mcoAdmin4Users,
  mcoAdmin5Users,
  mcoAdminEAction,
  mcoAdminEInterval,
  mcoAdminEventAggregationTime,      /**< time - refresh moab on admin based node/job modify */
  mcoAdminMinSTime,
  mcoAdminUsers,
  mcoAggregateNodeActions,
  mcoAggregateNodeActionsTime,
  mcoAllocResFailPolicy,
  mcoAllowRootJobs,
  mcoAllowVMMigration,
  mcoAlwaysApplyQOSLimitOverride,
  mcoAlwaysEvaluateAllJobs,
  mcoAMCfg,
  mcoAPIFailureThreshold,
  mcoArrayJobParLock,
  mcoAssignVLANFeatures,
  mcoAuthTimeout,               /* duration Moab will block waiting for munge/unmunge to complete */
  mcoBFChunkDuration,
  mcoBFChunkSize,
  mcoBFDepth,
  mcoBFMaxSchedules,
  mcoBFMetric,
  mcoBFMinVirtualWallTime,
  mcoBFVirtualWallTimeConflictPolicy,
  mcoBFVirtualWallTimeScalingFactor,
  mcoBFPolicy,
  mcoBFPriorityPolicy,
  mcoBFProcFactor,
  mcoBFType,                            /* deprecated */
  mcoBGPreemption,
  mcoBlockList,
  mcoUseDatabase,
  mcoDontCancelInteractiveHJobs,
  mcoCheckPointAllAccountLists,
  mcoCheckPointDir,
  mcoCheckPointExpirationTime,
  mcoCheckPointFile,
  mcoCheckPointInterval,
  mcoCheckPointWithDatabase,
  mcoCheckSuspendedJobPriority,
  mcoChildStdErrCheck,
  mcoClassCfg,
  mcoClientCfg,
  mcoClientMaxConnections,
  mcoClientMaxPrimaryRetry,
  mcoClientMaxPrimaryRetryTimeout,
  mcoClientTimeout,
  mcoClientUIPort,
  mcoCredDiscovery,
  mcoEvalMetrics,
  mcoRecordEventList,
  mcoReportProfileStatsAsChild,
  mcoDataStageHoldType,
  mcoDeadlinePolicy,
  mcoDefaultAccountName,
  mcoDefaultClassList,
  mcoDefaultDomain,
  mcoDefaultJobOS,
  mcoDefaultTaskMigrationTime,
  mcoDefaultQMHost,
  mcoDefaultRackSize,
  mcoDefaultSubmitLanguage,
  mcoDefaultVMSovereign,
  mcoDeferCount,
  mcoDeferStartCount,
  mcoDeferTime,
  mcoDeleteStageOutFiles,
  mcoDependFailurePolicy,
  mcoDisableBatchHolds,
  mcoDisableExcHList,
  mcoDisableInteractiveJobs,
  mcoDisableRegExCaching,
  mcoDisableRequiredGResNone,
  mcoIgnoreRMDataStaging,
  mcoDisableSameCredPreemption,
  mcoDisableSameQOSPreemption,
  mcoDisableScheduling,
  mcoDisableSlaveJobSubmit,         /* Disable job submission from a slave */
  mcoDisableThresholdTriggers,
  mcoDisableUICompression,
  mcoDisableVMDecisions,
  mcoDisableMultiReqJobs,
  mcoDisplayFlags,
  mcoDontEnforcePeerJobLimits,
  mcoDisplayProxyUserAsUser,
  mcoEmulationMode,
  mcoEnableJobArrays,
  mcoEnableClusterMonitor,
  mcoEnableCPJournal,
  mcoEnableEncryption,
  mcoEnableFailureForPurgedJob,
  mcoEnableFastSpawn,
  mcoEnableFSViolationPreemption,
  mcoEnableHighThroughput,
  mcoEnableFastNodeRsv,
  mcoEnableHPAffinityScheduling,
  mcoEnableImplicitStageOut,
  mcoEnableJobEvalCheckStart,       /* config option to check at job submission if job can ever run */
  mcoEnableNegJobPriority,
  mcoEnableNSNodeAddrLookup,
  mcoEnablePosUserPriority,
  mcoEnableSPViolationPreemption,
  mcoEnableUnMungeCredCheck,
  mcoEnableVMDestroy,
  mcoEnableVMSwap,
  mcoEnableVPCPreemption,
  mcoEnforceAccountAccess,
  mcoEnforceGRESAccess,
  mcoSubmitEnvFileLocation,
  mcoEventFileRollDepth,
  mcoEventLogWSPassword,
  mcoEventLogWSURL,
  mcoEventLogWSUser,
  mcoExtendedJobTracking,
  mcoFallBackClass,
  mcoFileRequestIsJobCentric,
  mcoFilterCmdFile,
  mcoForceNodeReprovision,
  mcoForceRsvSubType,
  mcoForceTrigJobsIdle,
  mcoForkIterations,
  mcoFSChargeRate,
  mcoFSConfigFile,
  mcoFSDecay,
  mcoFSDepth,
  mcoFSEnableCapPriority,
  mcoFSEnforcement,
  mcoFSRelativeUsage,
  mcoFSMostSpecificLimit,
  mcoFSTreeCapTiers,
  mcoFSTreeIsProportional,
  mcoFSTreeIsRequired,
  mcoFSTreeShareNormalize,
  mcoFSTreeAcctShallowSearch,
  mcoFSTreeTierMultiplier,
  mcoFSTreeUserIsRequired,
  mcoFSInterval,
  mcoFSIsAbsolute,
  mcoFSPolicy,
  mcoFSTree,
  mcoFSTreeACLPolicy,
  mcoFullJobTrigCP,
  mcoGEventCfg,
  mcoGEventTimeout,
  mcoGMetricCfg,
  mcoGreenPoolEvalInterval,
  mcoGResCfg,
  mcoGResToJobAttrMap,
  mcoGroupCfg,
  mcoGuaranteedPreemption,
  mcoHAPollInterval,
  mcoHideVirtualNodes,
  mcoIDCfg,
  mcoIgnoreClasses,
  mcoIgnoreJobs,
  mcoIgnoreLLMaxStarters,
  mcoIgnoreNodes,
  mcoIgnorePreempteePriority,
  mcoIgnoreUsers,
  mcoImageCfg,
  mcoInstantStage,  /* deprecated - see mcoJobMigratePolicy */
  mcoInternalTVarPrefix,
  mcoInvalidFSTreeMsg,
  mcoJobActionOnNodeFailure,
  mcoARFDuration,
  mcoJobAggregationTime,
  mcoJobAttrPrioF,
  mcoJobCCodeFailSampleSize,
  mcoJobCfg,
  mcoJobCPurgeTime,
  mcoJobCTruncateNLCP,
  mcoJobExtendStartWallTime, /* enable feature to extend the wall time withing a mib max range at job start */
  mcoJobMatchCfg,
  mcoJobRsvRetryInterval,
  mcoJobFailRetryCount,
  mcoJobFBAction,
  mcoJobMaxNodeCount,
  mcoJobMaxTaskCount,
  mcoJobMaxOverrun,
  mcoJobMigratePolicy,
  mcoJobNodeMatch,
  mcoJobPreemptCompletionTime,
  mcoJobPreemptMaxActiveTime,
  mcoJobPreemptMinActiveTime,
  mcoJobPrioAccrualPolicy,
  mcoJobPrioExceptions,
  mcoJobPurgeTime,
  mcoJobRejectPolicy,
  mcoJobRemoveEnvVarList,
  mcoJobRetryTime,
  mcoJobStartLogLevel,
  mcoJobSyncDeadline,
  mcoLicenseChargeRate,
  mcoLimitedJobCP,
  mcoLimitedNodeCP,
  mcoLoadAllJobCP,  /* load all jobs from CP file regardless RM reports */
  mcoLogDir,
  mcoLogFacility,
  mcoLogFile,
  mcoLogPermissions,
  mcoLogFileMaxSize,
  mcoLogFileRollDepth,
  mcoLogLevel,
  mcoLogLevelOverride,          /* if a client sends across a higher log level, use that for the duration of the command */
  mcoLogRollAction,             /* script for trigger to launch when logs are rolled. */
  mcoMailAction,
  mcoMaxACL,                    /* max ACL size for jobs/rsvs */
  mcoMaxDepends,
  mcoMaxGMetric,
  mcoMaxGreenStandbyPoolSize,   /* the max number of idle nodes to leave up in green mode */
  mcoMaxGRes,
  mcoMaxJob,
  mcoMaxNode,
  mcoMaxJobHoldTime,
  mcoMaxJobStartPerIteration,   /* maximum number of jobs allowed to start per iteration */
  mcoMaxJobStartTime,
  mcoMaxJobPreemptPerIteration, /* maximum number of jobs allowed to preempt per iteration */
  mcoMaxMetaTasks,
  mcoMaxRsv,
  mcoMaxRsvPerNode,
  mcoMaxSleepIteration,
  mcoMaxSuspendTime,
  mcoMemRefreshInterval,
  mcoMCSocketProtocol,
  mcoMDiagXmlJobNotes,
  mcoMinDispatchTime,
  mcoMissingDependencyAction,   /* action to take when a dependency job cannot be found */
  mcoMongoPassword,             /* Mongo password for authentication (only used if MongoUser also given) */
  mcoMongoServer,               /* Mongo server (and optionally port with [:<port>]) */
  mcoMongoUser,                 /* Mongo user for authentication (only used if MongoPassword also given) */
  mcoMountPointCheckInterval,
  mcoMountPointCheckThreshold,
  mcoMServerHomeDir,
  mcoMSubQueryInterval,         /* interval for msub -K to sleep between job queries */
  mcoMWikiSubmitTimeout,
  mcoNAPolicy,                  /* node access policy */
  mcoMapFeatureToPartition,
  mcoNAMITransVarName,          /* variable name to use for billing tids */
  mcoNestedNodeSet,
  mcoNetworkChargeRate,
  mcoNodeActiveList,
  mcoNodeAllocationPolicy,      /* node allocation policy */
  mcoNodeAllocResFailurePolicy,
  mcoNodeAvailPolicy,
  mcoNodeBusyStateDelayTime,
  mcoNodeCatCredList,
  mcoNodeCfg,
  mcoNodeCPUOverCommitFactor,
  mcoNodeDownStateDelayTime,
  mcoNodeDownTime,
  mcoNodeDrainStateDelayTime,
  mcoNodeFailureReserveTime,
  mcoNodeIDFormat,
  mcoNodeIdlePowerThreshold,
  mcoNodeIsOptional,
  mcoNoLocalUserEnv,
  mcoNodeMaxLoad,
  mcoNodeMemOverCommitFactor,
  mcoNodeNameCaseInsensitive,  /* true if the node names should not be case sensitive */
  mcoNodePollFrequency,
  mcoNodePurgeTime,
  mcoNodeSetAttribute,
  mcoNodeSetDelay,
  mcoNodeSetForceMinAlloc,
  mcoNodeSetIsOptional,
  mcoNodeSetList,
  mcoNodeSetMaxUsage,
  mcoNodeSetPlus,
  mcoNodeSetPolicy,
  mcoNodeSetPriorityType,
  mcoNodeSetTolerance,
  mcoNodeSyncDeadline,
  mcoNodeToJobFeatureMap,
  mcoNodeTypeFeatureHeader,
  mcoNodeUntrackedProcFactor,
  mcoNodeUntrackedResDelayTime,
  mcoNoJobHoldNoResources,
  mcoNonBlockingCacheInterval,
  mcoNoWaitPreemption,
  mcoObjectEList,
  mcoOptimizedCheckpointing,
  mcoOptimizedJobArrays,
  mcoOSCredLookup,
  mcoParAdmin,
  mcoParAllocationPolicy,
  mcoParCfg,
  mcoParIgnQList,
  mcoPartitionFeatureHeader,
  mcoPBSAccountingDir,
  mcoPerPartitionScheduling,
  mcoPreemptJobCount,
  mcoPreemptPolicy,
  mcoPreemptPrioJobSelectWeight,
  mcoPreemptRTimeWeight,
  mcoPreemptSearchDepth,
  mcoPriorityTargetDuration,
  mcoPriorityTargetProcCount,
  mcoProcSpeedFeatureHeader,
  mcoPushCacheToWebService,
  mcoPushEventsToWebService,
  mcoQOSCfg,
  mcoQOSIsOptional,
  mcoQOSRejectPolicy,
  mcoQueueCfg,
  mcoQueueNAMIActions,
  mcoRackFeatureHeader,
  mcoRangeEvalPolicy,
  mcoRealTimeDBObjects,
  mcoRegistrationURL,
  mcoRejectDosScripts,
  mcoRejectNegPrioJobs,
  mcoRejectInfeasibleJobs,
  mcoRejectDuplicateJobs,
  mcoRelToAbsPaths,
  mcoRemapClass,
  mcoRemapClassList,
  mcoRemoteStartCommand,
  mcoRemoveTrigOutputFiles,
  mcoReportPeerStartInfo,
  mcoRemoteFailTransient,
  mcoRequeueRecoveryRatio,
  mcoResendCancelCommand,
  mcoResourceLimitMultiplier,
  mcoResourceLimitPolicy,
  mcoResourceQueryDepth,
  mcoRestartInterval,
  mcoRMCfg,
  mcoRMEPort,
  mcoRMMsgIgnore,
  mcoRMPollInterval,
  mcoRMRetryTimeCap,
  mcoRsvReallocPolicy,
  mcoRsvBucketDepth,
  mcoRsvBucketQOSList,
  mcoRsvLimitPolicy,
  mcoRsvPolicy,
  mcoRsvProfile,
  mcoRsvPurgeTime,
  mcoRsvTimeDepth,
  mcoRsvRetryTime,
  mcoRsvSearchAlgo,
  mcoRsvThresholdType,
  mcoRsvThresholdValue,
  mcoSchedCfg,
  mcoSchedConfigFile,
  mcoSchedLockFile,
  mcoSchedMode,
  mcoSchedPolicy,
  mcoSchedStepCount,
  mcoSchedToolsDir,
  mcoServerHost,
  mcoServerName,
  mcoServerPort,
  mcoSideBySide,
  mcoSimAutoShutdown,
  mcoSimPurgeBlockedJobs,
  mcoSimCheckpointInterval,
  mcoSimFlags,
  mcoSimInitialQueueDepth,
  mcoSimJobSubmissionPolicy,
  mcoSimRandomizeJobSize,
  mcoSimStartTime,
  mcoSimTimePolicy,
  mcoSimTimeRatio,
  mcoSimWorkloadTraceFile,
  mcoSlotFeatureHeader,
  mcoSpoolDir,
  mcoSpoolDirKeepTime,
  mcoSocketExplicitNonBlock,
  mcoSocketLingerVal,
  mcoSocketWaitTime,
  mcoSRCfg,
  mcoStatDir,
  mcoStatProcMax,
  mcoStatProcMin,
  mcoStatProcStepCount,
  mcoStatProcStepSize,
  mcoStatProfCount,
  mcoStatProfDuration,
  mcoStatProfGranularity,
  mcoStatTimeMax,
  mcoStatTimeMin,
  mcoStatTimeStepCount,
  mcoStatTimeStepSize,
  mcoStrictProtocolCheck,
  mcoStopIteration,
  mcoSubmitHosts,
  mcoStoreSubmission,
  mcoSubmitFilter,
  mcoServerSubmitFilter,
  mcoUnsupportedDependencies,
  mcoSuspendRes,
  mcoSysCfg,
  mcoSystemMaxJobProc,
  mcoSystemMaxJobPS,
  mcoSystemMaxJobTime,
  mcoSystemMaxJobSecond,
  mcoTaskDistributionPolicy,
  mcoTrackSuspendedJobUsage,
  mcoTrackUserIO,
  mcoTrigCheckTime,           /**< milliseconds */
  mcoTrigEvalLimit,           /**< number of times we are allowed to look at ALL triggers in an iteration */
  mcoTrigFlags,
  mcoUMask,
  mcoUseAnyPartitionPrio,
  mcoUseCPUTime,
  mcoUseCPRsvNodeList,
  mcoUseFloatingNodeLimits,
  mcoUseJobRegEx,
  mcoUseLocalMachinePriority,
  mcoUseMoabCTime,
  mcoUseMachineSpeed,
  mcoUseMoabJobID,
  mcoUseNodeHash,
  mcoUserCfg,
  mcoUseSyslog,
  mcoUseSystemQueueTime,
  mcoUseUserHash,             /* Use user hash to lookup user */
  mcoVAddressList,
  mcoVCProfile,
  mcoCalculateLoadByVMSum,
  mcoMigrateToZeroLoadNodes,
  mcoVMCPurgeTime,            /**< time to keep completed VMs around */
  mcoVMCreateThrottle,        /**< limits the number of simultaneous vmcreate jobs */
  mcoVMMigrateThrottle,       /**< limits the number of simultaneous vmmigration jobs */
  mcoVMGResMap,
  mcoVMMigrationPolicy,
  mcoVMMinOpDelay,
  mcoVMOvercommitThreshold,
  mcoVMProvisionStatusReady,
  mcoVMsAreStatic,
  mcoVMStaleAction,           /**< Action to take when a VM has been deemed stale */
  mcoVMStaleIterations,       /**< Number of iterations that VM needs to be unreported before taking an action */
  mcoVMStorageMountDir,       /**< Default location to place VM storage mounts (subdirectories of given directory) */
  mcoVMStorageNodeThreshold,  /* Threshold percentage for max storage node allocation by storage mounts. */
  mcoVMTracking,
  mcoVPCFlags,                /**< virtual private cluster flags */
  mcoVPCMigrationPolicy,      /**< virtual private cluster migration policy */
  mcoVPCNodeAllocationPolicy, /**< VPC node allocation policy */
  mcoRsvNodeAllocPolicy,
  mcoRsvNodeAllocPriorityF,
  mcoFreeTimeLookAheadDuration,
  mcoXFMinWCLimit,
  mcoWattChargeRate,
  mcoWCViolAction,
  mcoWCViolCCode,
  mcoWebServicesUrl,
  mcoWikiEvents,
  mcoShowMigratedJobsAsIdle,      /**< Show migrated jobs as idle (Default:FALSE) */
  mcoServWeight,  /* NOTE: component weight parameters must sync w/enum MPrioComponentEnum */
  mcoTargWeight,
  mcoCredWeight,
  mcoAttrWeight,
  mcoFSWeight,
  mcoResWeight,
  mcoUsageWeight,
  mcoSQTWeight,   /* NOTE: subcomponent weights must sync w/enum MPrioSubComponentEnum */
  mcoSXFWeight,
  mcoSDeadlineWeight,
  mcoSSPVWeight,
  mcoSUPrioWeight,
  mcoSStartCountWeight,
  mcoSBPWeight,
  mcoTQTWeight,
  mcoTXFWeight,
  mcoCUWeight,
  mcoCGWeight,
  mcoCAWeight,
  mcoCQWeight,
  mcoCCWeight,
  mcoFUWeight,
  mcoFGWeight,
  mcoFAWeight,
  mcoFQWeight,
  mcoFCWeight,
  mcoGFUWeight,
  mcoGFGWeight,
  mcoGFAWeight,
  mcoFUWCAWeight,
  mcoFJPUWeight,
  mcoFJRPUWeight,
  mcoFPPUWeight,
  mcoFPSPUWeight,
  mcoAJobAttrWeight,
  mcoAJobGResWeight,
  mcoAJobIDWeight,
  mcoAJobNameWeight,
  mcoAJobStateWeight,
  mcoRNodeWeight,
  mcoRProcWeight,
  mcoRMemWeight,
  mcoRSwapWeight,
  mcoRDiskWeight,
  mcoRPSWeight,
  mcoRPEWeight,
  mcoRWallTimeWeight,
  mcoUConsWeight,
  mcoURemWeight,
  mcoUPerCWeight,
  mcoUExeTimeWeight,
  mcoServCap,  /* NOTE: parameters must sync with MPrioComponentEnum and MPrioSubComponentEnum enums */
  mcoTargCap,
  mcoCredCap,
  mcoAttrCap,
  mcoFSCap,
  mcoResCap,
  mcoUsageCap,
  mcoSQTCap,
  mcoSXFCap,
  mcoSDeadlineCap,
  mcoSSPVCap,
  mcoSUPrioCap,
  mcoSStartCountCap,
  mcoSBPCap,
  mcoTQTCap,
  mcoTXFCap,
  mcoCUCap,
  mcoCGCap,
  mcoCACap,
  mcoCQCap,
  mcoCCCap,
  mcoFUCap,
  mcoFGCap,
  mcoFACap,
  mcoFQCap,
  mcoFCCap,
  mcoGFUCap,
  mcoGFGCap,
  mcoGFACap,
  mcoFUWCACap,
  mcoFJPUCap,
  mcoFJRPUCap,
  mcoFPPUCap,
  mcoFPSPUCap,
  mcoAJobAttrCap,
  mcoAJobGResCap,
  mcoAJobIDCap,
  mcoAJobNameCap,
  mcoAJobStateCap,
  mcoRNodeCap,
  mcoRProcCap,
  mcoRMemCap,
  mcoRSwapCap,
  mcoRDiskCap,
  mcoRPSCap,
  mcoRPECap,
  mcoRWallTimeCap,
  mcoUConsCap,
  mcoURemCap,
  mcoUPerCCap,
  mcoUExeTimeCap,
  mcoOLDFSUWeight,        /* deprecated (params for compatibility) */
  mcoOLDFSGWeight,
  mcoOLDFSAWeight,
  mcoOLDFSQWeight,
  mcoOLDFSCWeight,
  mcoOLDServWeight,
  mcoOLDQTWeight,
  mcoOLDXFWeight,
  mcoOLDUFSWeight,
  mcoOLDGFSWeight,
  mcoOLDAFSWeight,
  mcoOLDDirectSpecWeight,
  mcoOLDBankServer,
  mcoOLDRMServer,
  mcoOLDMaxRsvPerNode,
  mcoOLDRMTimeOut,
  mcoOLDJobPurgeTime,
  mcoOLDAllocationPolicy,
  mcoOLDSRHostList,
  mcoOLDRMType,
  mcoThreadPoolSize,
  mcoVMCfg,
  mcoQOSDefaultOrder,
  mcoLAST };

/* linux GROUP count */
#define MMAX_USERGROUPCOUNT 32


/* sync w/MMonth[] */

enum MMonthEnum {
  mmJan = 0,
  mmFeb,
  mmMar,
  mmApr,
  mmMay,
  mmJun,
  mmJul,
  mmAug,
  mmSep,
  mmOct,
  mmNov,
  mmDec };


/* sync w/MURSpecToL(),MUDToRSpec(),... in MUtil.c */

enum MValModEnum {
  mvmByte = 0,
  mvmWord,
  mvmKilo,
  mvmMega,
  mvmGiga,
  mvmTera,
  mvmPeta,
  mvmExa,
  mvmZetta,
  mvmYotta,
  mvmLAST };

enum MTimeModeEnum {
  mtmNONE = 0,
  mtmInit,
  mtmInternalSchedTime,
  mtmRefresh,
  mtmLAST };


/* client keyword values (sync w/MCKeyword[]) */

enum MClientKeywordEnum {
  mckNONE = 0,
  mckStatusCode,
  mckArgs,
  mckAuth,
  mckCommand,
  mckData,
  mckClient,
  mckTimeStamp,
  mckCheckSum,
  mckLAST };

#define MDEF_TRUSTEDCHARLIST       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,._-$:@#/=+~*() \t\n"
#define MDEF_TRUSTEDCHARLISTX      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,_-:@#/=+~ "

#define MDEF_DIGITCHARLIST         "0123456789"


#define MCONST_WRSPTOK             "RSP="
#define MCONST_WRSPTOK2            "RESPONSE="

#define MCONST_MASTERCONFIGFILE    "/etc/moab.cfg"
#define MCONST_CONFIGFILE          "moab.cfg"

#define MCONST_GLOBALPARNAME       "ALL"
#define MCONST_SHAREDPARNAME       "SHARED"

#define MCONST_DEFAULTPARNAME      "DEFAULT"
#define MDEF_CLASSLIST             "[DEFAULT:1]"

#define MCONST_DEFAULTPARINDEX     1

#define MCONST_NODEPOLLNEVER       -2
#define MCONST_NODEPOLLONCE        -1
#define MCONST_NODEPOLLALWAYS      0

#define MDEF_RESCMP  mcmpGE


/* NOTE: sync with MXO[], MXOC[], MCredCfgParm[], MCredCfgParm2[] */

/* WARNING!!!ALERT!!! Adding or changing anything here needs to be synced with the
                      create.db.all.sql script and should only happen during major
                      revisions. */

/* NOTE: gres and vpc objects are mxo extensions */

enum MXMLOTypeEnum {
  mxoNONE = 0,
  mxoAcct,            /** 1 */
  mxoAM,              /** 2 */
  mxoClass,           /** 3 */
  mxoCluster,         /** 4 */
  mxoGroup,           /** 5 */
  mxoJob,             /** 6 */
  mxoNode,            /** 7 */
  mxoPar,             /** 8 */
  mxoQOS,             /** 9 */
  mxoQueue,           /** 10 */
  mxoRack,            /** 11 */
  mxoReq,             /** 12 */
  mxoRsv,             /** 13 */
  mxoRM,              /** 14 */
  mxoSched,           /** 15 */
  mxoSRsv,            /** 16 */
  mxoSys,             /** 17 */
  mxoTNode,           /** 18 FSTree intermediate node */
  mxoTrig,            /** 19 */
  mxoUser,            /** 20 */
  mxoALL,             /** 21 end of list for objects which can have policies, limits, cred mapping, etc */
  mxoxACL,            /** 22 access control list */
  mxoxCJob,           /** 23 completed job */
  mxoxCP,             /** 24 checkpoint */
  mxoxDepends,        /** 25 dependencies */
  mxoxEvent,          /** 26 event object */
  mxoxFS,             /** 27 per cred faairshare object */
  mxoxFSC,            /** 28 fairshare config */
  mxoxGreen,          /** 29 green config */
  mxoxGRes,           /** 30 */
  mxoxGMetric,        /** 31 generic metrics */
  mxoxHybrid,         /** 32 put Hybrid back in */
  mxoxImage,          /** 33 server/vm image */
  mxoxLimits,         /** 34 per cred limits object */
  mxoxNodeFeature,    /** 35 node feature */
  mxoxPriority,       /** 36 job priority */
  mxoxRange,          /** 37 */
  mxoxSim,            /** 38 */
  mxoxStats,          /** 39 */
  mxoxTask,           /** 40 */
  mxoxTasksPerJob,    /** 41 */
  mxoxTJob,           /** 42 template job */
  mxoxPendingAction,  /** 43 pending actions */
  mxoxTX,             /** 44 TX structure (mjtx_t) */
  mxoxVM,             /** 45 virtual machine */
  mxoxVMCR,           /** 46 VM create request */
  mxoxVPC,            /** 47 */
  mxoxJobGroup,       /** 48 */
  mxoxTrans,          /** 49 */
  mxoxVC,             /** 50 Virtual Container */
  mxoLAST };


/* calendar object (sync w/MCalAttr[], mcal_t) */

enum MCalAttrEnum {
  mcalaNONE = 0,
  mcalaEndTime,
  mcalaIMask,
  mcalaName,
  mcalaPeriod,
  mcalaStartTime,
  mcalaLAST };



enum MJVarNameEnum {
  mjvnHostList,
  mjvnJobID,
  mjvnMasterHost,
  mjvnNodeCount,
  mjvnOwner,
  mjvnLAST };


/* features allowed by license (sync w/MLicenseFeatures[]) */
/* NOTE:  sync with auto license setting in MSysGetLicense() */

/* NOTE: ONLY DOUG WIGHTMAN is allowed to work on license code due to
 * bugs introduced in the past! Preserve the order in this enum
 * if adding new entries--ORDER MATTERS! */

enum MLicenseFeaturesEnum {
  mlfNONE,
  mlfGrid,
  mlfGreen,
  mlfProvision,
  mlfVM,
  mlfMSM,
  mlfVPC,
  mlfNoHPC,       /* UNIQUE: this feature disables capabilities */
  mlfDynamic,
  mlfLAST };


/* class object (sync w/MClassAttr[],mclass_t) */

enum MClassAttrEnum {
  mclaNONE = 0,
  mclaAllocResFailPolicy,
  mclaComment,
  mclaOCNode,
  mclaCancelFailedJobs,
  mclaDefJAttr,
  mclaDefJExt,
  mclaDefReqDisk,
  mclaDefReqFeature,
  mclaDefReqGRes,
  mclaDefReqMem,
  mclaDefNode,
  mclaDefNodeSet,
  mclaDefOS,
  mclaDefProc,
  mclaDisableAM,
  mclaExclFeatures,
  mclaExclFlags,
  mclaExclUserList,
  mclaForceNAPolicy,
  mclaHostList,
  mclaIgnNodeSetMaxUsage,
  mclaJobEpilog,
  mclaJobProlog,
  mclaJobTermSig,
  mclaJobTrigger,
  mclaLogLevel,
  mclaMaxProcPerNode,
  mclaMaxArraySubJobs,    /* max number of array sub jobs in class */
  mclaMaxCPUTime,  /* max cputime per running job in class */
  mclaMaxNode,     /* max nodes per job allowed in class */
  mclaMaxProc,     /* max procs per job allowed in class */
  mclaMaxPS,
  mclaMaxGPU,
  mclaMinNode,
  mclaMinProc,
  mclaMinPS,
  mclaMinTPN,
  mclaMinGPU,
  mclaName,
  mclaNAPolicy,    /* node access policy - overrides job spec */
  mclaNAPrioF,     /* node alloc priority function */
  mclaOCDProcFactor,
  mclaOther,
  mclaPartition,
  mclaPreTerminationSignal,
  mclaReqFeatures,
  mclaReqFlags,
  mclaReqImage,    /* required OS/VM Image */
  mclaReqQOSList,
  mclaReqUserList,
  mclaReqAccountList,
  mclaRM,          /* creator RM */
  mclaRMAList,     /* list of RM's which can see/utilize queue */
  mclaSysPrio,
  mclaWCOverrun,
  mclaLAST };


#define MDEF_CLASSATTRBMSIZE MBMSIZE(mclaLAST)

/* job search mode (sync w/MJobSearchMode[]) */

enum MJobSearchEnum {
  mjsmNONE = 0,
  mjsmBasic,
  mjsmCompleted,
  mjsmExtended,
  mjsmInternal,
  mjsmJobName,
  mjsmLAST };


/* policy enforcement types (sync w/MPolicyMode[]) */

enum MPolicyTypeEnum {
  mptDefault = 0,
  mptOff,
  mptOn,
  mptSoft,
  mptHard,
  mptExempt,
  mptQueue,    /**< what does this mean? */
  mptLAST2 };


/* sync w/MRoleType[], MSched.Admin index */

enum MRoleEnum {
  mcalNONE = 0,
  mcalAdmin1,
  mcalAdmin2,
  mcalAdmin3,
  mcalAdmin4,
  mcalAdmin5,
  mcalOwner,        /* access granted if requestor is resource owner */
  mcalGranted,      /* requestor has full access to service */
  mcalTemplate,     /* cred is template - no privileges */
  mcalLAST };


enum MServiceType {
  mstNONE = 0,
  mstNormal,
  mstBestEffort,
  mstForce };


/* display flag types (sync w/MCDisplayType[]) */

enum MDisplayFlagEnum {
  mdspfNONE = 0,
  mdspfAccountCentric,
  mdspfCondensed,
  mdspfEnablePosPrio,
  mdspfHideBlocked,
  mdspfHideOutput,
  mdspfLocal,
  mdspfNodeCentric,
  mdspfReportedNodesOnly,    /* display "N/A" for NODES in showq until rm reports info */
  mdspfServiceProvisioning,
  mdspShowSystemJobs,
  mdspfSummary,
  mdspfTest,
  mdspfVerbose,
  mdspfUseBlocking,
  mdspfHideDrained,          /* When set, did not display object (Node,VM,...) while in drained state */
  mdspfLAST };


/* access list types */

enum MALTypeEnum {
  malOR = 0,
  malAND,
  malOnly,
  malLAST };

/* BF metric */

enum MBFMetricEnum {
  mbfmNONE = 0,
  mbfmProcs,
  mbfmNodes,
  mbfmPS,
  mbfmWalltime,
  mbfmLAST };


/* deadline policies (sync w/MDeadlinePolicy[]) */

enum MDeadlinePolicyEnum {
  mdlpNONE = 0,
  mdlpCancel,
  mdlpEscalate,
  mdlpHold,
  mdlpIgnore,
  mdlpRetry,
  mdlpLAST };



/* missing dependency actions (sync w/MMissingDependencyAction) */

enum MMissingDependencyActionEnum {
  mmdaNONE = 0,
  mmdaAssumeSuccess,
  mmdaCancel,
  mmdaHold,
  mmdaRun,
  mmdaLAST };



/* distribution types (no sync) - see MTaskDistPolicyEnum */

enum MDistPolicyEnum {
  mdpRR = 0,
  mdpArbGeo,
  mdpBF,
  mdpCustom,
  mdpDisperse,
  mdpPack,
  mdpLAST };


/* BF policies (sync w/MBFPolicy[]) */

enum MBFPolicyEnum {
  mbfNONE = 0,
  mbfBestFit,
  mbfFirstFit,
  mbfGreedy,
  mbfPreempt,
  mbfLAST };


/* sync with MBFPriorityPolicyType[] */

enum MBFPriorityEnum {
  mbfpNONE = 0,
  mbfpRandom,
  mbfpDuration,
  mbfpHWDuration,
  mbfpLAST };


/**
 * Client commands
 *
 * @see MClientCmd[] (sync)
 * @see enum MSvcEnum (peer)
 */

enum MClientCmdEnum {
  mccNONE = 0,
  mccBal,
  mccDiagnose,
  mccJobCtl,
  mccNodeCtl,
  mccVCCtl,
  mccVMCtl,
  mccRsvCtl,
  mccSchedCtl,
  mccShow,
  mccStat,
  mccSubmit,
  mccCredCtl,
  mccRMCtl,
  mccEventThreadSafe,
  mccShowThreadSafe,
  mccJobCtlThreadSafe,
  mccNodeCtlThreadSafe,
  mccRsvDiagnoseThreadSafe,
  mccTrigDiagnoseThreadSafe,
  mccVCCtlThreadSafe,
  mccVMCtlThreadSafe,
  mccJobSubmitThreadSafe,
  mccPeerResourcesQuery,
  mccCheckJob,
  mccCheckNode,
  mccRsvShow,
  mccShowState,
  mccLAST };

/* rsv policies */

enum MRsvPolicyEnum {
  mrpDefault = 0,
  mrpHighest,
  mrpCurrentHighest,
  mrpNever,
  mrpLAST };

/* sync w/MRsvThresholdType */

enum MRsvThresholdEnum {
  mrttNONE = 0,
  mrttBypass,
  mrttQueueTime,
  mrttXFactor,
  mrttLAST };

/* rsv time constraints */

enum MRangeTypeEnum {
  mrtcNONE = 0,
  mrtcStartOnly,
  mrtcStartLatest,
  mrtcStartEarliest,
  mrtcFeasible,
  mrtcLAST };

/* hold reasons (sync w/MHoldReason[]) */

enum MHoldReasonEnum {
  mhrNONE = 0,
  mhrAdmin,
  mhrNoResources,
  mhrSystemLimits,
  mhrAMFailure,
  mhrNoFunds,
  mhrAMReject,
  mhrAPIFailure,
  mhrRMReject,         /* rm rejects job execution */
  mhrPolicyViolation,  /* job violates job size policy */
  mhrCredAccess,       /* job cannot access requested credential */
  mhrCredHold,         /* credential hold in place */
  mhrPreReq,           /* job prereq failed */
  mhrData,             /* data staging cannot be completed */
  mhrSecurity,         /* job security cannot be established */
  mhrMissingDependency,/* dependency job cannot be found */
  mhrLAST };


/* cluster states, sync w/MClusterState[] */

enum MClusterStateEnum {
  mvpcsNONE = 0,
  mvpcsPending,         /* not yet active */
  mvpcsInitializing,
  mvpcsActive,
  mvpcsCleanup,
  mvpcsCompleted,
  mvpcsLAST };


/* node states, sync w/MNodeState[] */
/* NOTE:  if node state is update, this indicates that the RM reports the node 
          as healthy but the RM is unaware of workload on the node or the impact
          of workload on the node.  Moab needs to update the node's 'up' state 
          to one of 'idle', 'active', or 'busy' in MNodeAdjustAvailResources()
 */

enum MNodeStateEnum {
  mnsNONE = 0,   /* not set */
  mnsNone,       /* set to 'none' by RM */
  mnsDown,
  mnsIdle,       /* node is available for workload but is not running anything */
  mnsBusy,       /* node is running workload and cannot accept more */
  mnsActive,     /* node is running workload and can accept more */
  mnsDrained,    /* node has been sent a "drain" request and node has no workload on it */
  mnsDraining,   /* node has workload on it, but a "drain" request has been received */
  mnsFlush,      /* node is being re-provisioned */
  mnsReserved,   /* node is reserved (internal) */
  mnsUnknown,    /* same as mnsUp (deprecated) */
  mnsUp,         /* node is up, usage must be determined */
  mnsLAST };


/**
 * Node Statistics Attribute Enum
 *
 * @see MNodeStatType[] (sync)
 * @see struct mnust_t (sync)
 *
 * @see enum MStatAttrEnum - peer
 */

enum MNodeStatTypeEnum {
  mnstNONE = 0,
  mnstAGRes,
  mnstAProc,
  mnstCat,            /* overall node category */
  mnstCGRes,
  mnstCProc,
  mnstCPULoad,
  mnstDGRes,
  mnstDProc,
  mnstFailureJC,
  mnstGMetric,
  mnstIStatCount,
  mnstIStatDuration,
  mnstIterationCount,
  mnstMaxCPULoad,
  mnstMinCPULoad,
  mnstMaxMem,
  mnstMinMem,
  mnstNodeState,
  mnstProfile,
  mnstQueuetime,
  mnstRCat,           /* rm/rsv-assigned node category */
  mnstStartJC,
  mnstStartTime,
  mnstSuccessiveJobFailures,
  mnstUMem,
  mnstXLoad,          /* see generic metric overview */
  mnstXFactor,
  mnstLAST };


/* Scheduler Statistics Types, sync w/MSchedStatType[] */

enum MSchedStatTypeEnum {
  msstNONE = 0,
  msstUsage,
  msstFairshare,
  msstAll,
  msstLAST };


/* sync w/MNodeCat[], enum MRsvSubTypeEnum */

enum MNodeCatEnum {
  mncNONE = 0,
  mncActive,        /* node is executing jobs */
  mncDown_Batch,    /* batch system is down on node */
  mncRsv_SysBench,
  mncMain_Unsched,  /* node is reserved for unscheduled system maintenance */
  mncRsv_Grid,
  mncDown_HW,       /* node system hardware failure prevents node usage */
  mncMain_HW,
  mncIdle,          /* node is idle, available to execute job */
  mncRsv_Job,       /* node is idle, reserved for a priority job */
  mncDown_Net,      /* node is down due to network failure */
  mncOther,
  mncDown_Other,    /* node is down for unknown/other reason */
  mncRsv_PUser,     /* node is reserved for personal user rsv */
  mncSite1,         /* site specific category 1 */
  mncSite2,         /* site specific category 2 */
  mncSite3,         /* site specific category 3 */
  mncSite4,         /* site specific category 4 */
  mncSite5,         /* site specific category 5 */
  mncSite6,         /* site specific category 6 */
  mncSite7,         /* site specific category 7 */
  mncSite8,         /* site specific category 8 */
  mncDown_SW,       /* node system software failure prevents node usage */
  mncMain_SW,
  mncRsv_Standing,  /* node is idle, reserved by policy standing rsv */
  mncDown_Storage,  /* node is down due to storage failure */
  mncRsv_User,      /* node is idle, reserved for dedicated end-user usage */
  mncVPC,           /* node is part of active virtual private cluster */
  mncLAST };


/**
 * Node Access Policy (sync w/MNAccessPolicy[])
 *
 * @see struct mreq_t.NAccessPolicy
 * @see struct mpar_t.NAccessPolicy
 * @see struct mnode_t.EffNAPolicy
 * @see struct mnode_t.SpecNAPolicy
 * @see struct msched_t.DefaultNAccessPolicy
 * @see enum MReqAttrEnum->mrqaNodeAccess
 * @see MDEF_NACCESSPOLICY macro
 *
 * @see MNLAdjustResources()->MNodeGetEffNAPolicy()
 *
 * NOTE:  These MUST be maintained in order of precedence, least to most
 *        restrictive (ate 9-29-09)
 */

enum MNodeAccessPolicyEnum {
  mnacNONE = 0,      /* DEFAULT:  shared */
  mnacShared,        /* any combination of workload allowed */
  mnacSharedOnly,    /* only 'shared' node access policy jobs will be allowed */
  mnacUniqueUser,    /* any number of tasks from any user not currently running on the node */
  mnacSingleUser,    /* any number of tasks from the same user may utilize node */
  mnacSingleJob,     /* peer tasks from a single job may utilize node */
  mnacSingleTask,    /* only a single task may utilize node */
  mnacSingleGroup,   /* any number of tasks from the same group may utilize node */
  mnacSingleAccount, /* any number of tasks from the same account may utilize node */
  mnacLAST };

#define MDEF_NACCESSPOLICY mnacShared

#define MDEF_PRETERMINATIONSIGNALOFFSET 150
#define MDEF_PRETERMINATIONSIGNAL       10    /* SIGUSR1 */

/**
 * Node allocation policies, sync w/MNAllocPolicy[]
 *
 * @see struct mpar_t.NAllocPolicy
 * @see struct mpar_t.VPCNAllocPolicy
 *
 * @see mclaNAPrioF, mnaPrioF
 * @see enum MNPrioEnum - custom priority components
 * @see struct mnode_t.P (struct mnprio_t)
 * @see MNodeGetPriority() - calculates custom priority
 * @see MJobAllocMNL() - enforces node allocation policy
 */

enum MNodeAllocPolicyEnum {
  mnalNONE = 0,
  mnalContiguous,
  mnalCPULoad,
  mnalProcessorLoad,  /* alias for mnalCPULoad */
  mnalFastest,
  mnalNodeSpeed,      /* alias for mnalFastest */
  mnalFirstAvailable,
  mnalInReportedOrder,  /* alias for mnalFirstAvailable */
  mnalFirstSet,
  mnalLastAvailable,
  mnalInReverseReportedOrder,  /* alias for mnalLastAvailable */
  mnalLocal,
  mnalPlugin,
  mnalMachinePrio,
  mnalCustomPriority,   /* alias for mnalMachinePrio */
  mnalMaxBalance,
  mnalProcessorSpeedBalance,  /* alias for mnalProcessorSpeedBalance */
  mnalMinGlobal,
  mnalMinLocal,
  mnalMinResource,
  mnalMinimumConfiguredResources,  /* alias for mnalMinResource */
  mnalLAST };


/* node allocate resource failure policy, sync w/MARFPolicy[] */
/* NOTE:  ARF Policy currently associated with class, partition, and scheduler */

enum MAllocResFailurePolicyEnum {
  marfNONE = 0,
  marfCancel,
  marfFail,
  marfHold,
  marfIgnore,
  marfNotify,
  marfRequeue,
  marfLAST };


/**
 * VM migration policies (how to choose from/where for migrations)
 *
 * sync w/ MVMMigrationPolicy[]
 */

enum MVMMigrationPolicyEnum {
  mvmmpNONE = 0,
  mvmmpConsolidation,            /**< Consolidation migration - try to consolidate nodes (will also call Consolidation Overcommit) */
  mvmmpConsolidationOvercommit,  /**< Consolidation overcommit migration - migrate away from nodes that are overcommitted (but keep nodes consolidated) */
  mvmmpOvercommit,               /**< Overcommit migration - migrate away from nodes that are overcommitted (distribute overcommitted load) */
  mvmmpPerformance,               /**< Alias for Overcommit migration */
  mvmmpLAST };


/**
 * Action to take when a VM is stale
 *
 * sync w/ MVMStaleAction[]
 */

enum MVMStaleActionEnum {
  mvmsaNONE = 0,
  mvmsaIgnore,          /**< ignore the staleness (leave alone) */
  mvmsaCancelTracking,  /**< cancel the VMTracking job, leave the VM */
  mvmsaDestroy,         /**< destroy the VM */
  mvmsaLAST };


/**
 * Virtual Machine Usage Policy 
 *
 * @see mjob_t.VMUsagePolicy
 * @see const char *MVMUsagePolicy[]
 */

enum MVMUsagePolicyEnum {
  mvmupNONE = 0,
  mvmupAny,       /**< any resource selection acceptable */
  mvmupPMOnly,    /**< only execute on physical machines */
  mvmupPMPref,    /**< prefer execution on physical machines */
  mvmupVMCreate,  /**< create one-time use VM */
  mvmupVMOnly,    /**< only execute on virtual machines */
  mvmupVMPref,    /**< prefer execution on virtual machines */
  mvmupVMCreatePersist, /**< create VM and keep it persisted. */
  mvmupLAST };

 
/* partition allocation policies (sync w/MPAllocPolicy[]) */

enum MParAllocPolicyEnum {
  mpapNONE = 0,
  mpapBestAFit,        /* best absolute fit */
  mpapBestPFit,        /* best percentage fit */
  mpapFirstCompletion, /* earliest job completion */
  mpapFirstStart,      /* earliest job start */
  mpapRR,              /* round robin (allocate least-recently allocated partition) */
  mpapLoadBalance,     /* worst absolute fit */
  mpapLoadBalanceP,    /* worst percentage fit */
  mpapRandom,          /* randomize it */
  mpapLAST };



/**
 * Job access policies (no-sync) 
 * 
 * @see mjob_t.RsvAccessBM
 */

enum MJobAccessPolicyEnum {
  mjapNONE = 0,
  mjapAllowAll,
  mjapAllowJob,
  mjapAllowAllNonJob,
  mjapAllowSandboxOnly,
  mjapLAST };


#define MDEF_NALLOCPOLICY   mnalLastAvailable

/**
 * Keyboard Detection Policy
 *
 * NOTE:  sync w/MNodeKbdDetectPolicy[]
 *
 * @see struct mnode_t.KbdDetectPolicy
 */

enum MNodeKbdDetectPolicyEnum {
  mnkdpNONE = 0,
  mnkdpDrain,
  mnkdpPreempt,
  mnkdpLAST };


/* sync w/MVWConflictPolicy[] */

enum MVWConflictPolicyEnum {
  mvwcpNONE = 0,
  mvwcpPreempt,
  mvwcpLAST };

#define MDEF_VWCONFLICTPOLICY     mvwcpNONE

#define MDEF_NODEKBDDETECTPOLICY  mnkdpNONE
#define MDEF_PREEMPTONKBD          MBNOTSET
#define MDEF_MINRESUMEKBDIDLETIME       600
#define MDEF_MINPREEMPTLOAD            -1.0  /* not specified */

/* sync w/MREType[] */

enum MREEnum {
  mreNONE = 0,
  mreStart,
  mreEnd,
  mreInstant,
  mreLAST };

/* conjunction logic (sync w/MCLogic[]) */

enum MCLogicEnum {
  mclAND = 0,
  mclOR,
  mclNOT,
  mclLAST };

/* sync w/MJobPrioAccrualPolicyType */

enum MJobPrioAccrualPolicyEnum {
  mjpapNONE = 0,
  mjpapAccrue,            /* accrue based on exceptions, but don't reset */
  mjpapAlways,            /* always accrue priority (deprecated) */
  mjpapQueuePolicyUser,   /* accrue priority while not on user hold (deprecated) */
  mjpapQueuePolicy,       /* accrue priority while not violating idle policy (deprecated) */
  mjpapQueuePolicyReset,  /* reset priority if idle policy violated (deprecated) */
  mjpapFullPolicy,        /* accrue priority while not violating idle/run policy (deprecated) */
  mjpapFullPolicyReset,   /* reset priority if idle/run policy violated (deprecated) */
  mjpapReset,
  mjpapLAST };


/* sync w/MJobPrioException */

enum MJobPrioExceptionEnum {
  mjpeNONE,
  mjpeBatchhold,      /* accrue priority even if batchhold */
  mjpeDefer,          /* accrue priority even if defer */
  mjpeDepends,        /* accrue priority even if dependency not satisfied */
  mjpeHardPolicy,     /* accrue priority even if hard policy violation */
  mjpeIdlePolicy,     /* accrue priority even if idle policy violation */
  mjpeSoftPolicy,     /* accrue priority even if soft policy violation */
  mjpeSystemhold,     /* accrue priority even if systemhold */
  mjpeUserhold,       /* accrue priority even if userhold */
  mjpeLAST };


/* task distribution policies (sync w/MTaskDistributionPolicy[]) */

enum MTaskDistEnum {
  mtdNONE = 0,
  mtdDefault,
  mtdLocal,
  mtdArbGeo,
  mtdPack,
  mtdRR,        /* round robin */
  mtdDisperse,  /* distribute across max number of nodes */
  mtdLAST };


/** sync w/MJobNodeMatchType[] */

enum MNodeMatchPolicyEnum {
  mnmpNONE = 0,
  mnmpDefault,
  mnmpExactNodeMatch,
  mnmpExactProcMatch,
  mnmpExactTaskMatch,  /* map nodes requested to tasks with ppn matched to DRes.procs */
  mnmpLAST };


/* FS enums */

/* NOTE:  sync w/MFSAttr[] */

enum MFSAttrEnum {
  mfsaNONE = 0,
  mfsaCap,
  mfsaTarget,
  mfsaUsageList,
  mfsaLAST };


/* NOTE:  sync w/MFSTreeAttr[] */

enum MFSTreeAttrEnum {
  mfstaNONE,
  mfstaChildList,
  mfstaComment,
  mfstaCShare,
  mfstaFSCap,
  mfstaFSFactor,
  mfstaLimits,
  mfstaManagers,
  mfstaName,
  mfstaPartition,
  mfstaQList,
  mfstaQDef,
  mfstaShare,
  mfstaTotalShare,
  mfstaType,
  mfstaUsage,
  mfstaLAST };


/* NOTE:  sync w/MFSCAttr[] */

enum MFSCAttrEnum {
  mfscaNONE = 0,
  mfscaDecay,
  mfscaDefTarget,
  mfscaDepth,
  mfscaInterval,
  mfscaIsAbsolute,
  mfscaPolicy,
  mfscaUsageList,
  mfscaLAST };


enum MFSActionEnum {
  mfsactNONE = 0,
  mfsactCalc,
  mfsactRotate,
  mfsactWrite,
  mfsactLAST };


/* sync w/MFSTargetTypeMod[] */

enum MFSTargetEnum {
  mfstNONE = 0,
  mfstTarget,
  mfstFloor,
  mfstCeiling,
  mfstCapRel,
  mfstCapAbs,
  mfstLAST };


/* sync w/MFSPolicyType[] */

enum MFSPolicyEnum {
  mfspNONE = 0,
  mfspDPS,
  mfspDPES,
  mfspUPS,
  mfspDSPES,  /* dedicated, scaled proc-equivalent seconds */
  mfspDPPS,   /* proc speed */
  mfspLAST };


/* rmctl commands (sync w/MRMCtlCmds[]) */

enum MRMCtlCmdEnum {
  mrmctlNONE = 0,
  mrmctlClear,       /* clear rm messages/stats */
  mrmctlCreate,
  mrmctlDestroy,
  mrmctlFailure,
  mrmctlInit,
  mrmctlKill,
  mrmctlList,
  mrmctlModify,
  mrmctlPeerCopy,    /* copy file to remote peer (Moab RM only) */
  mrmctlPing,
  mrmctlPurge,
  mrmctlQuery,       /* query internal aspects of RM (Moab RM only) */
  mrmctlReconfig,
  mrmctlResume,
  mrmctlStop,
  mrmctlStart,
  mrmctlLAST };



/**
 * Scheduler State
 *
 * @see MSchedState[] (sync)
 * @see msched_t.State
 */

enum MSchedStateEnum {
  mssmNONE = 0,
  mssmPaused,      /* scheduler will not start jobs (will update RM information) */
  mssmStopped,     /* scheduler will not start jobs or update RM information */
  mssmRunning,     /* scheduler will start jobs and update RM information */
  mssmShutdown,    /* scheduler is shutting down */
  mssmLAST };


/**
 * Scheduler Modes
 *
 * @see MSchedMode[] (sync)
 *
 * @see struct msched_t.Mode
 * @see const char *MSchedMode[]
 */

enum MSchedModeEnum {
  msmNONE = 0,
  msmInteractive,  /* each scheduling decision but be manually approved */
  msmMonitor,      /* scheduler should monitor and report status, but not impact workload */
  msmNormal,       /* scheduler performs all scheduling duties */
  msmNoSchedule,   /* do everything but schedule jobs */
  msmProfile,
  msmSim,          /* scheduler operates in simulation environment */
  msmSingleStep,
  msmSlave,        /* scheduler only schedules by external request */
  msmTest,         /* scheduler monitors only (difference from monitor mode?) */
  msmLAST };

/* sched command modes (sync w/MSchedCtlCmds[]) */

enum MSchedCtlCmdEnum {
  msctlNONE = 0,
  msctlCopyFile,       /* copy a file using Moab's socket connection */
  msctlCreate,
  msctlDestroy,
  msctlEvent,          /* used for sending events to other Moab peers */
  msctlFailure,        /* introduce synthetic failure */
  msctlFlush,          /* flush things */
  msctlInit,           /* initialize server state */
  msctlKill,           /* shutdown server */
  msctlList,           /* list config */
  msctlLog,            /* log */
  msctlModify,         /* modify config parameter */
  msctlPause,          /* pause scheduling */
  msctlQuery,          /* query attributes */
  msctlReconfig,       /* refresh configuration */
  msctlResume,         /* resume scheduling (recycle) */
  msctlStep,           /* advance scheduling to specified iteration */
  msctlStop,           /* stop scheduling at specified iteration */
  msctlVMMigrate,      /* migrate VMs around */
  msctlLAST };


#define MDEF_ARFDURATION            300

/* sync w/XXX */

enum MSchedActionEnum {
  mactNONE = 0,
  mactAdminEvent,
  mactJobFB,
  mactMail,
  mactLAST };


/* sync w/MResourceLimitPolicyType[] */

enum MResLimitPolicyEnum {
  mrlpNONE = 0,
  mrlpAlways,
  mrlpExtendedViolation,
  mrlpBlockedWorkloadOnly,
  mrlpNever,
  mrlpLAST };


/* sync w/MPolicyAction[] */

enum MResLimitVActionEnum {
  mrlaNONE = 0,
  mrlaCancel,
  mrlaCheckpoint,
  mrlaNotify,
  mrlaPreempt,
  mrlaRequeue,
  mrlaSuspend,
  mrlaUser,
  mrlaLAST };


/* sync w/MJobStateType[] */

enum MJobStateTypeEnum {
  mjstNONE = 0,
  mjstActive,
  mjstBlocked,
  mjstCompleted,
  mjstEligible,
  mjstLAST };


#define MDEF_JOBSTATEBMSIZE MBMSIZE(mjstLAST)


/* sync w/MJobSubState[] */

enum MJobSubStateEnum {
  mjsstNONE = 0,
  mjsstEpilog,
  mjsstMigrated,
  mjsstPreempted,
  mjsstProlog,
  mjsstLAST };

/* resource set */

/* sync w/MResSetAttrType[] */

enum MResourceSetTypeEnum {
  mrstNONE = 0,
  mrstClass,
  mrstFeature,
  mrstLAST };


/* sync w/MResSetSelectionType[] */

enum MResourceSetSelectionEnum {
  mrssNONE = 0,
  mrssOneOf,
  mrssAnyOf,
  mrssFirstOf,  /* NOTE:  firstof is only used internally to allow allocation optimization */
  mrssLAST };


/* sync w/MResSetPrioType[] */

enum MResourceSetPrioEnum {
  mrspNONE = 0,
  mrspAffinity,
  mrspFirstFit,
  mrspBestFit,
  mrspWorstFit,
  mrspBestResource,
  mrspMinLoss,
  mrspLAST };


/* sync w/MNodeSetPlusPolicy[] */

enum MNodeSetPlusPolicyEnum {
  mnspNONE = 0,
  mnspSpanEvenly,
  mnspDelay,
  mnspLAST };

/**
 * This is the enumeration of possible job states.
 * This enum must be synced with MJobState[] in MConst.c
 */

enum MJobStateEnum {
  mjsNONE = 0,   /**< A sentinal value. Not used */
  mjsIdle,       /**< eligible according to all RM constraints */
  mjsStarting,   /**< job launching, executing prolog */
  mjsRunning,    /**< job is executing */
  mjsRemoved,    /**< job cancelled before executing */
  mjsCompleted,  /**< job successfully completed execution */
  mjsHold,       /**< job is blocked by RM hold */
  mjsDeferred,   /**< job temporarily blocked */
  mjsSubmitErr,
  mjsVacated,    /**< job cancelled after partial execution */
  mjsNotRun,
  mjsNotQueued,  /**< job not eligible for execution */
  mjsUnknown,
  mjsBatchHold,  /**< job has a batch hold in place */
  mjsUserHold,   /**< job has a user hold in place */
  mjsSystemHold, /**< job has a system hold in place */
  mjsStaging,    /**< staging of input/output data is currently underway */
  mjsStaged,     /**< all staging pre-reqs are satisified - waiting for remote RM to start */
  mjsSuspended,
  mjsLost,
  mjsAllocating, /**< resources are selected and are being prepared for job */
  mjsBlocked,    /**< state used only by database for job that is idle but has a block reason */
  mjsLAST };


/**
 * @see MNAvailPolicy[] (sync)
 * @see mnode_t.NAvailPolicy[]
 * @see mpar_t.AvailPolicy[]
 * @see MDEF_RESOURCEAVAILPOLICY
 * @see mcoNodeAvailPolicy (param: NODEAVAILABILITYPOLICY)
 *
 * @see MNodeGetTC()
 * @see MNodeAdjustAvailResources()
 * @see MNodePostLoad()
 * @see MParProcessNAvailPolicy()
 */

enum MResourceAvailabilityPolicyEnum {
  mrapNONE = 0,
  mrapCombined,
  mrapDedicated,
  mrapUtilized,
  mrapLAST };


/* sync w/MPolicyType[] */
/* sync w/MIdlePolicyType[] */

enum MActivePolicyTypeEnum {
  mptNONE = 0,
  mptMaxJob,
  mptMaxNode,
  mptMaxPE,
  mptMaxProc,
  mptMaxPS,
  mptMaxWC,
  mptMaxMem,
  mptMinProc,
  mptMaxGRes,
  mptMaxArrayJob,
  mptLAST };


/* sync w/MPeerEventType[] */

enum MPeerEventTypeEnum {
  mpetNONE = 0,
  mpetClassChange,
  mpetLAST };


/* priority object (sync w/pServWeight, mco*Weight, mco*Cap) */

enum MPrioComponentEnum {
  mpcNONE = 0,
  mpcServ,      /* service */
  mpcTarg,      /* target service */
  mpcCred,      /* credential */
  mpcAttr,      /* job attribute */
  mpcFS,        /* fairshare */
  mpcRes,       /* resource request */
  mpcUsage,     /* resource usage */
  mpcLAST };


/**
 * prio subcomponent (sync w/MPrioSCName[], mco*Weight, mco*Cap)
 *
 * @see MFSProcessOConfig()
 * @see MCfgSetVal()
 * @see struct mcfg_t MCfg[]
 */

enum MPrioSubComponentEnum {
  mpsNONE = 0,
  mpsSQT,
  mpsSXF,
  mpsSDeadline,
  mpsSSPV,
  mpsSUPrio,
  mpsSStartCount,
  mpsSBP,
  mpsTQT,
  mpsTXF,
  mpsCU,
  mpsCG,
  mpsCA,
  mpsCQ,
  mpsCC,
  mpsFU,   /* local fs */
  mpsFG,
  mpsFA,
  mpsFQ,
  mpsFC,
  mpsGFU,  /* global fs */
  mpsGFG,
  mpsGFA,
  mpsFUWCA,
  mpsFJPU,
  mpsFJRPU,
  mpsFPPU,
  mpsFPSPU,
  mpsAAttr,     /* attribute subcomponent */
  mpsAGRes,
  mpsAJobID,
  mpsAJobName,
  mpsAState,
  mpsRNode,
  mpsRProc,
  mpsRMem,
  mpsRSwap,
  mpsRDisk,
  mpsRPS,
  mpsRPE,
  mpsRWallTime,
  mpsUCons,
  mpsURem,
  mpsUPerC,
  mpsUExeTime,
  mpsLAST };


enum MJobNameEnum {
  mjnNONE = 0,
  mjnFullName,
  mjnRMName,
  mjnShortName,
  mjnSpecName,
  mjnLAST };


enum MPrioDisplayEnum {
  mpdJob = 0,
  mpdHeader,
  mpdFooter };

/* reservation types */

/* sync w/MRsvType[] */

enum MRsvTypeEnum {
  mrtNONE = 0,
  mrtJob,
  mrtUser,
  mrtMeta,
  mrtLAST };


/* sync w/MRsvSubType[], enum MNodeCatEnum */

enum MRsvSubTypeEnum {
  mrsvstNONE = 0,
  mrsvstActive,
  mrsvstAdmin_BatchFailure,
  mrsvstBench,
  mrsvstAdmin_EmergencyMaintenance,
  mrsvstGridRsv,
  mrsvstAdmin_HWFailure,
  mrsvstAdmin_HWMaintenance,
  mrsvstIdle,
  mrsvstJob,
  mrsvstAdmin_NetFailure,
  mrsvstAdmin_Other,
  mrsvstAdmin_OtherFailure,
  mrsvstPRsv,
  mrsvstSite1,
  mrsvstSite2,
  mrsvstSite3,
  mrsvstSite4,
  mrsvstSite5,
  mrsvstSite6,
  mrsvstSite7,
  mrsvstSite8,
  mrsvstAdmin_SWFailure,
  mrsvstAdmin_SWMaintenance,
  mrsvstStandingRsv,
  mrsvstAdmin_StorageFailure,
  mrsvstUserRsv,
  mrsvstVPC,
  mrsvstMeta,
  mrsvstLAST };


/* sync w/MJRType[] */

enum MJRsvSubTypeEnum {
  mjrNONE = 0,
  mjrActiveJob,
  mjrDeadline,
  mjrMeta,
  mjrPriority,
  mjrQOSReserved,
  mjrRetry,
  mjrTimeLock,
  mjrUser,
  mjrLAST };


/* period types (sync w/MCalPeriodType[],MCalPeriodDuration[]) */

enum MCalPeriodEnum {
  mpNONE = 0,
  mpMinute,
  mpHour,
  mpDay,
  mpWeek,
  mpMonth,
  mpInfinity,
  mpLAST };

enum MRCListEnum {
  mrclNONE = 0,
  mrclYear,
  mrclDecade,
  mrclCentury,
  mrclMillenium,
  mrclLAST };

/* sync w/MResourceType[], need to add license */

enum MResourceTypeEnum {
  mrNONE = 0,
  mrNode,
  mrProc,
  mrMem,
  mrSwap,
  mrDisk,
  mrAMem,
  mrASwap,
  mrDProcs,
  mrGRes,
  mrLAST };


/* sync w/MResLimitType[] */

enum MResLimitTypeEnum {
  mrlNONE = 0,
  mrlJobMem,
  mrlJobProc,
  mrlNode,
  mrlProc,
  mrlMem,
  mrlSwap,
  mrlDisk,
  mrlWallTime,
  mrlCPUTime,
  mrlMinJobProc,
  mrlTemp,
  mrlGMetric,        /* used as a flag to indicate gmetrics exist */
  mrlLAST };


/** 
 * scheduler services 
 *
 * sync w/MCRequestF[] (in MUI.c)
 * sync w/muis_t MUI 
 */

enum MSvcEnum {
  mcsNONE = 0,
  mcsShowState,
  mcsSetJobSystemPrio, /* compat */
  mcsSetJobUserPrio,   /* compat */
  mcsShowQueue,
  mcsSetJobHold,       /* compat */
  mcsReleaseJobHold,   /* compat */
  mcsStatShow,         /* compat */
  mcsRsvCreate,        /* compat */
  mcsRsvDestroy,       /* compat */
  mcsRsvShow,
  mcsDiagnose,
  mcsSetJobQOS,        /* compat */
  mcsShowResAvail,
  mcsShowConfig,
  mcsCheckJob,
  mcsCheckNode,
  mcsJobStart,
  mcsCancelJob,
  mcsConfigure,
  mcsShowEstimatedStartTime,
  mcsMJobCtl,
  mcsMNodeCtl,
  mcsMRsvCtl,
  mcsMSchedCtl,
  mcsMStat,
  mcsMDiagnose,
  mcsMShow,
  mcsMVCCtl,
  mcsMVMCtl,
  mcsMBal,
  mcsCredCtl,
  mcsRMCtl,
  mcsMSubmit,
  mcsLAST };


/* rm states (sync w/MRMState[],MAMState[]) */

enum MRMStateEnum {
  mrmsNONE = 0,
  mrmsActive,      /* interface is active */
  mrmsConfigured,  /* interface is configured */
  mrmsCorrupt,     /* service is active but interface is corrupt */
  mrmsDisabled,    /* admin disabled RM access */
  mrmsDown,        /* service is unavailable, cannot connect to RM */
  mrmsLAST };


/**
 * An enum of Resource Manager Types.
 *
 * A Resource Manager (RM) controls resources (obviously). For HPC Clusters, the resources are compute nodes, disk, memory, CPUs, etc.
 * Resources can also be things like licenses, network bandwidth, or disk space.
 * By definition, an RM *does not* schedule jobs.
 * This is a separation of control and policy.
 * Scheduling is the process of deciding whether or not a job can run.
 * Resource Management controls the resources that a job uses.
 * This includes starting and stopping the job, and monitoring resources that the job is using.
 * If the job violates constraints set by the scheduler (i.e. runtime limit), the RM is responsible for killing the job and cleaning up after it.
 *
 * Must be synced with MRMType[] in MConst.c
 */
enum MRMTypeEnum {
  mrmtNONE = 0,    /**< A sentinal value*/
  mrmtLL,          /**< LL RM (Load Leveler)*/
  mrmtOther,
  mrmtPBS,         /**< PBS RM (Portable Batch Scheduler). aka TORQUE*/
  mrmtS3,          /**< The internal RM */
  mrmtWiki,        /**< Wiki RM. Used with SLURM.*/
  mrmtNative,      /**< Native RM. Used to gather data from homemade scripts, or text files, etc.*/
  mrmtMoab,        /**< Moab RM*/
  mrmtTORQUE,      /**< TORQUE RM -> re-mapped to mrmtPBS */
  mrmtLAST };



/* sync w/MRMSubType[] */

enum MRMSubTypeEnum {
  mrmstNONE = 0,
  mrmstAgFull,   /**< node data from aggregated source (w/node validation) */
  mrmstLSF,      /**< used for emulation */
  mrmstCCS,      /**< Microsoft CCS */
  mrmstMSM,      /**< moab service manager */
  mrmstSGE,      /**< SGE (Sun GridEngine) */
  mrmstSLURM,    /**< SLURM */
  mrmstX1E,      /**< cray X1E commands */
  mrmstXT3,      /**< cray XT3 (cpa) commands */
  mrmstXT4,      /**< cray XT4 (basil) commands */
  mrmstLAST };


enum MQueueTypeEnum {
  mqtNONE = 0,
  mqtRM,
  mqtLocal,
  mqtLAST };


/* sync w/??? */

enum MQueueFuncTypeEnum {
  mqftNONE = 0,
  mqftExecution,
  mqftRouting,
  mqftLAST };


/* node accessibility (sync w/MAffinityType[]) */

enum MNodeAffinityTypeEnum {
  mnmUnavailable = 0,
  mnmRequired,
  mnmPositiveAffinity,
  mnmNeutralAffinity,
  mnmNegativeAffinity,
  mnmPreemptible,
  mnmNONE,             /* no affinity (ie, no reservation) */
  mnmLAST };


/* See MBitMaps.[ch] */

typedef struct mbitmap_t {
  unsigned char *BM;      /* calloc'd: Bit Map Array - Size bytes long */
  unsigned int Size;      /* Number of bytes in BM array */

  /* default constructor */
  mbitmap_t();

  /* destructor */
  ~mbitmap_t();

  /* assignment operator */
  mbitmap_t& operator= (const mbitmap_t &rhs);

  /* Copy constructor */
  mbitmap_t(const mbitmap_t& rhs);

  /* bmclear method */
  void clear(void);

  /* string ToString() method */
  std::string  ToString(void);

  } mbitmap_t;


/* acl modifier flags (sync w/macl_t) */

enum MACLEnum {
  maclNONE = 0,
  maclDeny,           /* if matched, deny access regardless of other acl settings */
  maclXOR,            /* match all settings but specified value */
  maclRequired,       /* must be matched to enable global match */
  maclConditional,    /* acl must be matched and conditional requirements satisfied */
  maclCredLock,       /* credential is locked to this reservation */
  maclHPEnable,       /* acl is enabled during hard policy scheduling pass only */
  maclLAST };

/* ACL conditional attribute structure */

typedef struct maclcon_t {
  long   QT;
  double XF;
  double BL;    /* backlog */
  } maclcon_t;


/* ended processes */

typedef struct mpid_t {
  int PID;
  int StatLoc;
  void *Object;
  } mpid_t;


/** Structure for storing dynamic data attributes.  This is so that rarely
 *   used attributes don't need to take up space on frequently used structures,
 *   like a mulong on a job that is only used for one RM type, etc.
 *
 *  These operate as a linked list.
 */

typedef struct mdynamic_attr_s {
  int      AttrType;

  union {
    char  *String;
    int    Int;
    double Double;
    long   Long;
    mulong Mulong;
    void  *Void;
    } Data;

  enum MDataFormatEnum Format;

  struct mdynamic_attr_s *Next;
  } mdynamic_attr_t;



/**
 * object access types 
 *
 * sync w/MAttrO[], MHRObj[], MAToO[] (located in MPolicy.c)
 *
 * @see macl_t
 */

enum MAttrEnum {
  maNONE = 0,
  maJob,        /* allow by jobid */
  maNode,
  maRsv,
  maUser,       /* allow user or jobs owned by user */
  maGroup,
  maAcct,
  maPartition,
  maScheduler,
  maSystem,
  maClass,
  maQOS,
  maJAttr,      /* allow jobs which have/request specified attribute */
  maJPriority,  /* allow jobs which have/exceed specified priority */
  maJTemplate,  /* allow jobs which have specified template applied */
  maDuration,
  maTask,
  maProc,
  maPS,
  maRack,
  maQueue,
  maCluster,    /* for ACL's, only jobs originating at this cluster have access to this resource */
  maQueueTime,
  maXFactor,
  maMemory,
  maVC,
  maLAST };


/* admin object */

/* sync w/MAdminAttr[], madmin_t */

enum MAdminAttrEnum {
  madmaNONE = 0,
  madmaCancelJobs,
  madmaEnableProxy,
  madmaIndex,
  madmaName,
  madmaServiceList,
  madmaUserList,
  madmaGroupList,
  madmaLAST };


/**
 * sched object  attributes
 *
 * @see struct msched_t (sync)
 * @see MSchedAttr[] (sync)
 */


enum MSchedAttrEnum {
  msaNONE = 0,
  msaAlias,
  msaChangeset,
  msaChargeMetricPolicy,
  msaChargeRatePolicy,
  msaCheckpointDir,
  msaCPVersion,
  msaDBHandle,
  msaDefNodeAccessPolicy,
  msaEventCounter,
  msaFBServer,
  msaFlags,
  msaGID,
  msaHALockCheckTime,
  msaHALockFile,         /* high availability lock file */
  msaHALockUpdateTime,
  msaHomeDir,
  msaHTTPServerPort,
  msaIgnoreNodes,
  msaIterationCount,     /* scheduler iteration count (dup'd in MSysAttr) */
  msaJobMaxNodeCount,    /* maximum nodes allowed per job */
  msaLastCPTime,
  msaLastTransCommitted,
  msaLocalQueueIsActive, /* local queue is in use */
  msaMaxJobID,
  msaMaxRecordedCJobID,  /* Max Completed JobID recorded in the completed jobs table */
  msaMessage,
  msaMinJobID,
  msaMode,
  msaName,
  msaRecoveryAction,
  msaRestartState,
  msaRevision,
  msaRsvCounter,
  msaRsvGroupCounter,
  msaSchedLockFile,
  msaSchedulerName,
  msaServer,
  msaSpoolDir,
  msaState,
  msaStateMAdmin,
  msaStateMTime,
  msaTime,
  msaTrigCounter,
  msaTransCounter,
  msaTrigger,
  msaUnsupportedDependencies,
  msaUID,
  msaVarDir,
  msaVarList,
  msaVersion,
  msaMaxJob,
  msaLAST };


/**
 * Partition Object 
 *
 * @see MParAttr[] (sync)
 * @see struct mpar_t (sync)
 * @see MParAToString() - report partition attributes (sync)
 * @see MParSetAttr() - sets partition attributes (sync)
 * @see mjob_t.mreq_t.PtIndex,mnode_t.PtIndex,mrsv_t.PtIndex
 */

/* sync w/MParAttr[] */

enum MParAttrEnum {
  mpaNONE = 0,
  mpaAcct,          /**< (for VPC) */
  mpaACL,           /**< creds which can view/utilize partition/VPC */
  mpaAllocResFailPolicy, /* resource failure policy */
  mpaArch,          /* this info is stored in P->ReqN */
  mpaBacklogDuration,
  mpaBacklogPS,
  mpaCal,           /* calendar (for VPC) */
  mpaClasses,
  mpaCmdLine,
  mpaConfigFile,    /* partition configuration file */
  mpaCost,          /* total cost (for VPC) */
  mpaDedSRes,       /* resources dedicated while job is suspended */
  mpaDefCPULimit,
  mpaDefaultNodeActiveWatts,
  mpaDefaultNodeIdleWatts,
  mpaDefaultNodeStandbyWatts,
  mpaDuration,      /* (for VPC) */
  mpaFlags,
  mpaGreenBacklogThreshold,
  mpaGreenQTThreshold,
  mpaGreenXFactorThreshold,
  mpaGRes,          /* generic resources */
  mpaGroup,
  mpaID,
  mpaIsRemote,
  mpaJobActionOnNodeFailure,
  mpaJobCount,
  mpaJobNodeMatchPolicy,
  mpaMaxCPULimit,
  mpaMaxProc,
  mpaMaxPS,
  mpaMaxNode,
  mpaMaxWCLimit,
  mpaMinProc,
  mpaMinNode,
  mpaMinWCLimit,
  mpaMessages,
  mpaNodeAccessPolicy,
  mpaNodeAllocPolicy,  /**< see XXX */
  mpaNodeChargeRate, /**< default node charge rate */
  mpaNodeFeatures,
  mpaNodePowerOffDuration, /**< time to power off nodes in this partition */
  mpaNodePowerOnDuration, /**< time to power on nodes in this partition */
  mpaOS,            /**< stored in P->RepN, OS will be assigned as default to all nodes in partition (see mpaTrackOS) */
  mpaOwner,         /**< (for VPC) */
  mpaPriority,      /**< (for VPC) */
  mpaProfile,       /**< (for VPC) */
  mpaProvEndTime,   /**< (for VPC) */
  mpaProvStartTime, /**< (for VPC) */
  mpaPurgeTime,     /**< (for VPC) */
  mpaQOS,
  mpaReqOS,         /**< required operating system */
  mpaReqResources,  /**< (for VPC) */
  mpaReservationDepth, /**< for grid scheduling */
  mpaRsvGroup,      /**< (for VPC) */
  mpaRsvNodeAllocPolicy, /**< (for rsvs) */
  mpaRsvNodeAllocPriorityF, 
  mpaRM,
  mpaRMType,        /**< this info is stored in P->RM->Type */
  mpaRsvProfile,    /**< used only at VPC creation time */
  mpaSize,          /**< (for VPC) */
  mpaStartTime,     /**< (for VPC) */
  mpaState,         /**< (for VPC) */
  mpaSuspendResources,
  mpaUpdateTime,    /**< this info is stored in P->RM->UTime */
  mpaUser,          /**< (for VPC) */
  mpaUseTTC,        /**< use ttc for scheduling */
  mpaVariables,     /**< (for VPC) */
  mpaVMCreationDuration, /**< time required to create a VM */
  mpaVMDeletionDuration, /**< time required to delete a VM */
  mpaVMMigrationDuration, /**< time required to migrate a VM */
  mpaVPCFlags,      /**< (for VPC) */
  mpaVPCNodeAllocPolicy, /**< (for VPC) */
  mpaWattChargeRate, /**< charge rate for watts in this partition */
  mpaLAST };


/* sync w/mccfg_t,MClientAttr[] */

enum MClientAttrEnum {
  mcltaNONE = 0,
  mcltaAuthCmd,
  mcltaAuthCmdOptions, /* Specify cmd options for the AuthCmd */
  mcltaClockSkew,
  mcltaCSAlgo,
  mcltaDefaultSubmitPartition,
  mcltaEncryption,
  mcltaFlags,
  mcltaLocalDataStageHead,  /* the node where stdout/stderr files are copied to */
  mcltaTimeout,
  mcltaLAST };


enum MWCVioActEnum {
  mwcvaNONE = 0,
  mwcvaCancel,
  mwcvaPreempt,
  mwcvaLAST };


/* sync w/MPreemptPolicy[] */

enum MPreemptPolicyEnum {
  mppNONE = 0,
  mppCancel,
  mppCheckpoint,
  mppMigrate,
  mppOvercommit,
  mppRequeue,
  mppSuspend,
  mppLAST };

#define MDEF_PREEMPTPOLICY    mppRequeue


/* sync w/MJobRejectPolicy[] */

enum MJobRejectPolicyEnum {
  mjrpNONE,
  mjrpCancel,
  mjrpHold,
  mjrpIgnore,
  mjrpMail,
  mjrpRetry,
  mjrpLAST };

#define MDEF_JOBREJECTPOLICY  (1 << mjrpHold)

#define MDEF_QOSREJECTPOLICY  (1 << mjrpHold)


/* sync w/MProfAttr[] */

enum MProfAttrEnum {
  mpraNONE = 0,
  mpraCount,
  mpraDuration,
  mpraMaxCount,
  mpraStartTime,
  mpraLAST };

/* limit types */

enum MLimitTypeEnum {
  mlNONE = 0,
  mlActive,
  mlIdle,
  mlSystem,
  mlLAST };

/* sync w/MJobBlockReason[] */

enum MJobNonEType {
  mjneNONE = 0,
  mjneActivePolicy,
  mjneBadUser,
  mjneDependency,
  mjneEState,
  mjneFairShare,
  mjneHold,      /* should split into temp hold and permanent hold */
  mjneIdlePolicy,
  mjneLocalPolicy,
  mjneNoClass,
  mjneNoData,
  mjneNoResource,
  mjneNoTime,
  mjnePartitionAccess,
  mjnePriority,
  mjneRMSubmitFailure,
  mjneStartDate,
  mjneState,
  mjneSysLimits, /* permanent issue */
  mjneLAST };

enum MEventFlagsEnum {
  mefNONE = 0,
  mefExternal,
  mefLAST };

enum MTimePolicyEnum {
  mtpNONE = 0,
  mtpReal };


/* sync w/MRMAuthType[] */

enum MRMAuthEnum {
  mrmapNONE = 0,
  mrmapCheckSum,
  mrmapOther,
  mrmapPKI,
  mrmapSecurePort,
  mrmapLAST };


/* sync w/J->ReqHLMode (impacts J->ReqHList) */
/* sync w/MHostListMode[] */

/* NOTE:  these are only set when HostList is loaded as an RM Extension
          when mode=NONE it should be interpreted as ExactSet */

enum MHostListModeEnum {
  mhlmNONE = 0,
  mhlmSuperset,    /* hostlist is superset of what jobs needs, tasks assigned should be subset of nodes in hostlist */
  mhlmSubset,      /* hostlist is subset of what job needs, i.e., tasks assigned should be superset of nodes in hostlist */
  mhlmExactSet,    /* tasks assigned should exactly match hostlist */
  mhlmLAST };


/**
 * Node Priority Enum
 *
 * @see MNPComp[] (sync)
 * @see struct mnprio_t 
 */

enum MNPrioEnum {
  mnpcNONE = 0,
  mnpcADisk,           /* avl local disk */
  mnpcAGRes,           /* availabile generic resources */
  mnpcAMem,            /* avl real memory */
  mnpcAProcs,          /* avl procs */
  mnpcArch,            /* arch */
  mnpcASwap,           /* avl swap */
  mnpcCDisk,           /* cfg local disk */
  mnpcCGRes,           /* configured generic resource */
  mnpcCMem,            /* cfg memory */
  mnpcCost,            /* node cost (see N->ChargeRate) */
  mnpcCProcs,          /* cfg procs  */
  mnpcCSwap,           /* cfg swap   */
  mnpcFeature,         /* feature is available on node */
  mnpcFreeTime,        /* free time availableon node (MNodeGetFreeTime) */
  mnpcGMetric,         /* generic metric */
  mnpcJobCount,        /* active jobs on node */
  mnpcLoad,            /* processor load */
  mnpcLocal,           /* locally specified algorithm */
  mnpcMTBF,            /* mean time between failure (in seconds) */
  mnpcNodeIndex,       /* node index */
  mnpcOS,              /* active OS */
  mnpcParAProcs,       /* available procs in partition */
  mnpcPLoad,           /* partition load */
  mnpcPower,           /* power state */
  mnpcPParAProcs,      /* percentage available procs in partition */
  mnpcPref,            /* node is preferred */
  mnpcPriority,        /* node admin priority */
  mnpcProximity,       /* node proximity to server */
  mnpcRandom,          /* random per iteration factor */
  mnpcReqOS,           /* future workload requires specified OS */
  mnpcRsvAffinity,     /* reservation affinity */
  mnpcServerLoad,      /* overall effective server load */
  mnpcSpeed,           /* processor/machine speed (???) */
  mnpcSuspendedJCount, /* suspended job count */
  mnpcTaskCount,       /* active tasks on nodes */
  mnpcUsage,           /* percent utilization over time */
  mnpcVMCount,         /* number of VMs running on or migrating to node */
  mnpcWindowTime,      /* the window size (in seconds) for the job/rsv */
  mnpcLAST };          /* percentage load */


/* sync w/MDependType[], see mjdepend_t.Type */

enum MDependEnum {
  mdNONE = 0,
  mdJobStart,                /* job must have previously started */
  mdJobCompletion,           /* job must have previously completed */
  mdJobSuccessfulCompletion, /* job must have previously completed successfully */
  mdJobFailedCompletion,
  mdJobSyncMaster,           /* slave jobs must start at same time */
  mdJobSyncSlave,            /* must start at same time as master */
  mdJobOnCount,              /* number of dependencies job will have on it (used with before) */
  mdJobBefore,               /* job cannot start until this job completes */
  mdJobBeforeAny,            /* job cannot start until this job starts */
  mdJobBeforeSuccessful,     /* job cannot start until this job completes successfully */
  mdJobBeforeFailed,         /* job cannot start until this job fails */
  mdDAG,
  mdVarSet,                  /* variable is set */
  mdVarNotSet,               /* variable is not set */
  mdVarEQ,
  mdVarNE,
  mdVarGE,
  mdVarLE,
  mdVarGT,
  mdVarLT,
  mdLAST };


/* limit object (sync w/MLimitAttr[]) */

enum MLimitAttrEnum {
  mlaNONE = 0,
  mlaAJobs,
  mlaAProcs,
  mlaAPS,
  mlaAMem,
  mlaANodes,
  mlaHLAJobs,
  mlaHLAProcs,
  mlaHLAPS,
  mlaHLANodes,
  mlaHLIJobs,
  mlaHLINodes,
  mlaSLAJobs,
  mlaSLAProcs,
  mlaSLAPS,
  mlaSLANodes,
  mlaSLIJobs,
  mlaSLINodes,
  mlaLAST };


/**
 * security flags 
 *
 * @see mqaSecurity
 * @see enum MQOSFlagEnum (peer)
 * @see mqos_t.Security
 * @see MSecFlags[] (sync) 
 *
 * NOTE:  see mqfVPN (mqfVPN may be deprecated)
 */

enum MSecFlagEnum {
  msfNONE = 0,
  msfDiskScrub,
  msfDynamicStorage,  /** dynamically allocate/destroy storage for workload */
  msfReset,
  msfVLAN,            /** dynamically create/destroy VLAN around workload */
  msfLAST };


/**
 * Scheduler Flags (sync w/MSchedFlags[])
 *
 * @see MSched.Flags
 *
 * NOTE:  for now, assume multiple master compute RM's each report a different
 *        OS and each application can only run in one of the target OS's.  If
 *        an OS is not specified, select the OS associated with a feasible target
 *        node.
 */

enum MSchedFlagEnum {
  mschedfNONE = 0,
  mschedfAllowMultiCompute,           /**< allow multiple master compute RM's per node */
  mschedfAllowPerJobNodeSetIsOptional,
  mschedfExtendedGroupSupport,
  mschedfDisablePerJobNodesets,       /**< disable per-job nodesets (not documented, used by Boeing, RT5883) */
  mschedfDisableTorqueJobPurging,     /**< disable job purging with torque */
  mschedfDoNotApplySecondaryGroupToChild,
  mschedfDynamicRM,                   /**< dynamic rm exists */
  mschedfOptimizedBackfill,           /**< turn on optimized backfill algorithm (see comment in MReqGetFNL and MBFFirstFit) */
  mschedfFastGroupLookup,             /**< uses getgrouplist instead of getgrent where possible */
  mschedfHosting,
  mschedfJobsUseRsvWalltime,          /**< Jobs w/out walltime req inherit from their reservation */
  mschedfNoGridCreds,                 /**< disable multi-hop grid cred translation path reporting */
  mschedfSocketDebug,                 /**< enable socket debug behavior */
  mschedfThreadTestCancel,            /**< use non-asychronous cancel threads */
  mschedfValidateFutureNodesets,      /**< verify nodesets possible when evaluating future job start times */
  mschedfJobResumeLog,                /**< temporary, used for debugging why a job can't resume, but a new job can start */
  mschedfFileLockHA,                  /**< use filelock HA method */
  mschedfShowRequestedProcs,          /**< show requested procs (regardless of NodeAccessPolicy) in showq */
  mschedfNewPreemption,               /**< use the new preemption routines */
  mschedfFastRsvStartup,              /**< faster startup by updating rsv nodelists after all nodes have been loaded, instead of using MNodeUpdateResExpression() */
  mschedfTrimTransactions,            /**< trim stale transactions in MTransCheckStatus() -- save memory */
  mschedfStrictSpoolDirPermissions,   /**< use strict spooldir permissions */
  mschedfNormalizeTaskDefinitions,    /**< normalize the task definitions for "mshow -a -w mintasks" (see MCResNormalize and MClusterShowARes) */
  mschedfUseLocalUserGroup,           /**< on job submission across moab, assume the local user's group ID for Cred.Group */
  mschedfMinimalJobSize,              /**< keep job sizes small */
  mschedfAllowInfWallTimeJobs,        /**< Allows infinite walltime jobs (msub, job template, etc.) */
  mschedfAlwaysUpdateCache,           /**< go ahead and do limited transitions every iteration for jobs, even with ENABLEHIGHTHROUGHPUT */
  mschedfIgnorePIDFileLock,           /**< don't fail if we can't get a lock on the .moab.pid file */
  mschedfUnmigrateOnDefer,            /**< unmigrate jobs when they go deferred */
  mschedfForceProcBasedPreemption,    /**< force preemption to only consider processors */
  mschedfEnforceReservedNodes,        /**< only try to start reserved jobs on their reserved nodes */
  mschedfBoundedRMLimits,               /**< don't allow RM to set limits beyond moab's limits (currently slurm only)*/
  mschedfAggregateNodeFeatures,       /**< aggregate node features over RMs and moab.cfg */
  mschedfNoVMDestroyDependencies,     /**< vmdestroy jobs will not have dependencies (MOAB-4469) */
  mschedfRsvTableLogDump,             /**< dump the rsv table into the logs while scheduling to track down memory corruption */
  mschedfOutOfBandVMRsv,              /**< Wrap out-of-band VMs with a reservation */
  mschedfLAST };

#define MDEF_SCHEDFLAGSIZE MBMSIZE(mschedfLAST)



/**
 * The enum for Resource Manager (RM) flags.
 *
 * @see enum MRMIFlagEnum (peer)
 * @see MRMFlags[] (sync)
 * @see struct mrm_t (sync)
 * @see mrm_t.Flags[]
 *
 * This is used to check things like whether the job is local or remote,
 * whether the RM should checkpoint jobs, if the job is a Grid job, etc.
 *
 * It must be synced with the mrm_t struct, and MRMFlags[] in MConst.c
 *
 * NOTE: mrmfMigrateAllJobAttributes is currently only implemented in
 * MPBSJobSubmit(), other RMs do not yet support this flag
 *
 */
enum MRMFlagEnum {
  mrmfNONE = 0,
  mrmfASyncDelete,        /**< rm will delete jobs asynchronously */
  mrmfASyncStart,         /**< rm will start jobs asynchronously */
  mrmfAutoStart,          /**< rm will automatically start job once staged */
  mrmfAutoSync,           /**< rm state will start/stop w/moab */
  mrmfBecomeMaster,       /**< adopt all resources and workload which are reported by this RM if the current RM does not also have this flag */
  mrmfCkptRequeue,        /**< rm will signal and requeue job on checkpoint request */
  mrmfClient,             /**< moab is client of rm interface */
  mrmfClockSkewChecking,  /**< moab checks for clock skew and adjusts job times accordingly */
  mrmfCompressHostList,   /**< compress hostlist expressions */
  mrmfDCred,              /**< rm may automatically create creds needed by workload */
  mrmfDynamicCompute,     /**< RM can create/destroy dynamic compute resources */
  mrmfDynamicDataRM,      /**< RM can create dynamic Data RM */
  mrmfDynamicStorage,     /**< RM can dynamically create private workload storage (ie, NAS, SAN, etc) */
  mrmfExecutionServer,
  mrmfFSIsRemote,         /**< filesystem is remote - do not validate files/dir at migrate time */
  mrmfFullCP,             /**< always do a full job checkpoint */
  mrmfIgnMaxStarters,     /**< ignore the MAX_STARTERS in LL */
  mrmfIgnNP,
  mrmfIgnOS,              /**< ignore OS reported by rm */
  mrmfIgnQueueState,      /**< moab should ignore state of queues reported by rm */
  mrmfIgnWorkloadState,   /**< ignore the job state reported by this RM's workload query */
  /* mrmfImageIsStateful, */    /**< provisioning images from this RM are not stateless */
  mrmfJobNoExec,          /**< jobs submitted to RM should run but not execute child */
  mrmfLocalWorkloadExport, /**< dst RM should export entire local workload to src peer */
  mrmfLocalRsvExport,     /**< dst RM should export entire rsv table to src peer */
  mrmfMigrateNow,         /**< submissions will fail if cannot start immediately (no queuing allowed) */
  mrmfMigrateAllJobAttrbutes, /**< migrate "all" job information when submitting to this RM */
  mrmfAutoURes,           /**< automatically fill in RQ->URes based on assumptions (only affects PBS) */
  mrmfNoCreateAll,        /**< RM with this flag will only update resources/jobs discovered by other non-slave RMs (see NOTE below) */
  mrmfNoCreateResource,   /**< RM with this flag will only update resources discovered by other non-slave RMs */
  mrmfNodeGroupOpEnabled, /**< group node operations allowed */
  mrmfPrivate,            /**< use secure resource availability query */
  mrmfPushSlaveJobUpdates,/**< use MSet instead of MSetOnEmpty when loading remote jobs from XML */
  mrmfQueueIsPrivate,
  mrmfRecordGPUMetrics,   /**< turn on gmetric recording of gpu metrics */
  mrmfRecordMICMetrics,   /**< turn on gmetric recording of mic metrics */
  mrmfReport,             /**< report RM failures via email */
  mrmfRMClassUpdate,      /**< RM provides node-queue info, disable auto-class intelligence */
  mrmfRootSubmit,         /**< submit job under root id and environment allowed */
  mrmfRsvExport,          /**< RM should export rsv's to peer */
  mrmfRsvImport,          /**< RM should import rsv's from peer */
  mrmfSharedAccess,       /**< resource is shared amongst other schedulers */
  mrmfSlavePeer,          /**< RM will not independently schedule/manage jobs */
  mrmfSlurmWillRun,       /**< SLURM RM decides which nodes will be used */
  mrmfStatic,             /**< rm info is static (only load on new object discovery) */
  mrmfTemplate,           /**< template RM (not real RM) */
  mrmfUserSpaceIsSeparate,/**< RM userspace inaccessible to Moab server */
  mrmfVLAN,               /**< rm can create/manage VLAN's */
  mrmfProxyJobSubmission, /**< submit jobs as proxy for user. */
  mrmfPreemptionTesting,  /**< test preemption and don't send the suspend signal */
  mrmfFlushResourcesWhenDown, /**< for license RMs, if down then zero out ARes->GRes */
  mrmfLAST };            

#define MDEF_RMFLAGSIZE MBMSIZE(mrmfLAST)

/* NOTE:  non-Moab slaves cannot report resources not seen by non-slave RMs
          Moab slaves can report resources not seen by non-slave RMs but cannot
          take independent scheduling action
*/



/**
 * RM Internal flags
 *
 * @see enum MRMFlagEnum
 *
 * NOTE:  not specified via configuration/admin
 * NOTE:  sync w/MRMIFlags[] (max=30)
 * NOTE:  @see mrm_t.IFlags
 */

enum MRMIFlagEnum {
  mrmifNONE = 0,
  mrmifAllowDeltaResourceInfo,  /**< allow delta based resource info import from RM */
  mrmifDefaultNADedicated,      /**< by default, allocated nodes are dedicated */
  mrmifDisableIncrExtJob,       /**< disable incremental increase when extending jobs */
  mrmifDisablePreemptExtJob,    /**< disable the setting of the preemptee flag for extended jobs */
  mrmifIgnoreDefaultEnv,        /**< ignore the default environment */
  mrmifImageIsStateful,         /**< provisioning RM reports stateful images */
  mrmifIsTORQUE,                /**< RM is remap of TORQUE */
  mrmifJobStageInEnabled,       /**< scheduler can stage jobs to resource manager */
  mrmifLoadBalanceQuery,        /**< TEMP */
  mrmifLocalQueue,              /**< manager for locally submitted jobs */
  mrmifMaximizeExtJob,          /**< immediately extend jobs to maximum size */
  mrmifMultiSource,             /**< interface loads data for multiple independent RM's */
  mrmifNativeBasedGrid,         /**< grid/peer communication should utilize native interface */
  mrmifNoExpandPath,            /**< do not expand $HOME,variables in path */
  mrmifNoMigrateCkpt,           /**< job checkpoints are locked to nodes on which they were created */
  mrmifNoTaskOrdering,          /**< task ordering information is not maintained/reported */
  mrmifRMStartRequired,         /**< RM must be explicitly started */
  mrmifSetTaskmapEnv,           /**< set taskmap env variable */
  mrmifTimeoutSpecified,        /**< rm timeout specified */
  mrmifLAST };


/**
 * RM attributes
 *
 * NOTE:  To add attribute, update MRMProcessConfig(), MRMSetAttr(),
          MRMAToString(),MRMConfigShow(), and MRMToXML()
 *
 * @see MRMAttr[] (sync)
 * @see struct mrm_t (sync)
  */

enum MRMAttrEnum {
  mrmaNONE = 0,
  mrmaAdminExec,
  mrmaApplyGResToAllReqs,
  mrmaAuthType,
  mrmaAuthAList,
  mrmaAuthCList,
  mrmaAuthGList,
  mrmaAuthQList,
  mrmaAuthUList,
  mrmaBW,                /* bandwidth to/from RM */
  mrmaCancelFailureExtendTime,  /* amount of time to extend job's wclimit if cancel failure is detected */
  mrmaCheckpointSig,
  mrmaCheckpointTimeout,
  mrmaClient,            /* name of client 'peer' used to authenticate (P2P) */
  mrmaClockSkew,
  mrmaClusterQueryURL,
  mrmaCompletedJobPurge,
  mrmaConfigFile,
  mrmaCredCtlURL,
  mrmaCSAlgo,
  mrmaCSKey,
  mrmaDataRM,            /* data rm for handling datastaging services */
  mrmaDefClass,          /* default job class */
  mrmaDefOS,             /* default node/job operating system */
  mrmaDefDStageInTime,   /* default data stage-in time */
  mrmaDefHighSpeedAdapter, /* used for MLLJobStart */
  mrmaDefJob,            /* default job template */
  mrmaDescription,       /* human readable resource manager description */
  mrmaEncryption,
  mrmaEnv,               /**< environment to be applied to child tools (see R->Env) */
  mrmaEPort,             /**< event port */
  mrmaExtHost,           /**< host name to use when reporting to remote grids */
  mrmaFailTime,          /**< time to wait until failure trigger fires */
  mrmaFBServer,          /**< host:port location of RM fallback/HA server */
  mrmaFeatureList,       /**< node feature bitmap applied to all created nodes */
  mrmaFlags,
  mrmaFnList,
  mrmaHost,              /* host on which RM server is running */
  mrmaIgnHNodes,
  mrmaInfoQueryURL,
  mrmaIsDeleted,
  mrmaJobCancelURL,
  mrmaJobCount,
  mrmaJobCounter,
  mrmaJobExportPolicy,   /* how job control/info is made available to peer */
  mrmaJobExtendDuration,
  mrmaJobIDFormat,       /* format of Moab-generated job id's */
  mrmaJobMigrateURL,
  mrmaJobModifyURL,
  mrmaJobRequeueURL,
  mrmaJobResumeURL,
  mrmaJobRsvRecreate,
  mrmaJobSignalURL,
  mrmaJobStartCount,
  mrmaJobStartURL,
  mrmaJobSubmitURL,
  mrmaJobSuspendURL,
  mrmaJobValidateURL,
  mrmaLanguages,         /* supported RM script languages */
  mrmaLocalDiskFS,
  mrmaMasterRM,
  mrmaMaxFailCount,
  mrmaMaxJob,
  mrmaMaxJobHop,    /* maximum job hop count */
  mrmaMaxJobPerMinute,
  mrmaMaxJobs,      /* maximum jobs Moab is allowed to load from this RM */
  mrmaMaxJobStartDelay,
  mrmaMaxNodeHop,   /* maximum node hop count */
  mrmaMaxTransferRate,
  mrmaMessages,
  mrmaMinETime,
  mrmaMinJob,
  mrmaMinStageTime, /* minimum time to stage job and data from scheduler to rm */
  mrmaName,
  mrmaNC,
  mrmaNMHost,
  mrmaNMPort,
  mrmaNMServer,
  mrmaNoAllocMaster, /* RM should not allocate independent req for master head node (XT only) */
  mrmaNodeDestroyPolicy,
  mrmaNodeFailureRsvProfile,
  mrmaNodeList,
  mrmaNodeMigrateURL,
  mrmaNodeModifyURL,
  mrmaNodePowerURL,    /* power on, power off, reboot */
  mrmaNodeStatePolicy,
  mrmaNodeVirtualizeURL,
  mrmaNoNeedNodes,
  mrmaOMap,            /* object to object mapping file */
  mrmaPackage,
  mrmaParCreateURL,    /**< create external workload partition */
  mrmaParDestroyURL,   /**< destroy external workload partition */
  mrmaPartition,       /* partition into which reported resources will be assigned */
  mrmaPollInterval,
  mrmaPollTimeIsRigid,
  mrmaPort,
  mrmaPreemptDelay,
  mrmaProcs,
  mrmaProfile,         /* dynamic package used to create this RM */
  mrmaProvDuration,
  mrmaPtyString,
  mrmaUseVNodes,
  mrmaQueueQueryURL,
  mrmaReqNC,
  mrmaResourceCreateURL,
  mrmaResourceType,
  mrmaRMInitializeURL,
  mrmaRMStartURL,
  mrmaRMStopURL,
  mrmaRsvCtlURL,
  mrmaSBinDir,         /* directory containing RM system binaries */
  mrmaServer,          /* host:port location of RM server */
  mrmaSetJob,          /* job template to force job attributes */
  mrmaSLURMFlags,
  mrmaSocketProtocol,
  mrmaSoftTermSig,
  mrmaSQLData,
  mrmaStageThreshold,  /* a measurement of a job's local starttime that must be surpassed before job can be remotely migrated */
  mrmaStartCmd,
  mrmaState,
  mrmaStats,
  mrmaSubmitCmd,
  mrmaSubmitPolicy,
  mrmaSuspendSig,
  mrmaSyncJobID,
  mrmaSystemModifyURL,
  mrmaSystemQueryURL,
  mrmaTargetUsage,
  mrmaTimeout,
  mrmaTrigger,
  mrmaType,              /* resource manager arch type (see enum mrmtXXX) */
  mrmaUpdateJobRMCreds,     /* if set, update account/group/queue within RM (default is TRUE) */
  mrmaUpdateTime,       /* time rm last reported updated info */
  mrmaVariables,         /* generic variables set on RM */
  mrmaVersion,
  mrmaVMOwnerRM,
  mrmaVPGRes,           /* virtual process generic resource (@see struct mrm_t.VPGResIndex) */
  mrmaWireProtocol,
  mrmaWorkloadQueryURL,
  mrmaLAST };


/* sync w/MSchedPolicy[] */

enum MSchedPolicyEnum {
  mscpNONE = 0,
  mscpEarliestStart,
  mscpEarliestCompletion,
  mscpLAST };


/* sync w/MRMSubmitPolicy[] */

enum MRMSubmitPolicyEnum {
  mrmspNONE = 0,
  mrmspNodeCentric,   /* nodes=2:ppn=4 -> RQ->TaskCount=2,RQ->NodeCount=2,RQ->DRes.Procs=4 */
  mrmspProcCentric,   /* nodes=2:ppn=4 -> RQ->TaskCount=8,RQ->NodeCount=2,RQ->DRes.Procs=1 */
  mrmspLAST };

enum MMigrationPolicyEnum {
  mmpNONE = 0,
  mmpGreen,
  mmpRsv,
  mmpOverCommit,
  mmpLAST };

/* definitions */

typedef char name_t[MMAX_NAME];

#ifndef MDB

#define MDEF_EXTENDEDJOBTRACKING FALSE

enum MDBActionEnum { mdbLOGFILE, mdbNOTIFY, mdbSYSLOG };

#define MDB(X,F) if ((mlog.Threshold >= (X)) && (bmisset(&mlog.FacilityBM,fALL) || bmisset(&mlog.FacilityBM,F)))
#define MDBE(X,F) if ((mlog.Threshold == (X)) && (bmisset(&mlog.FacilityBM,fALL) || bmisset(&mlog.FacilityBM,F)))
#define MDBO(X,O,F) if ((bmisset(&mlog.FacilityBM,fALL) || bmisset(&mlog.FacilityBM,F)) && ((mlog.Threshold >= (X)) || (((O) != NULL) && ((O)->LogLevel >= (X)))))

#ifdef __MTEST
#define MLog printf
#define MUSNPrintF(X,Y,...) printf(__VA_ARGS__)
#endif /* __MTEST */

#define MDEBUG(X) if (mlog.Threshold >= X)

#endif /* MDB */

#define MFARG     -1
#define MFMEM     -2
#define MFTIMEOUT -3


#define MHAVE_SPAWN      1

/* NAT RM interface definitions */

#define MNAT_GETLOADCMD    "/usr/bin/vmstat | /usr/bin/tail -1 | /bin/awk '{ print \\$8 \" \" \\$9 \" \" \\$10 \" \" \\$11 \" \" \\$16 }'"
#define MNAT_CANCELCMD     "/bin/kill"
#define MNAT_CANCELSIG     "-15"
#define MNAT_RCMD          "/usr/bin/ssh"

#define MNAT_CLUSTERQUERYPATH   ""
#define MNAT_NODEMODIFYPATH     "echo modify"
#define MNAT_WORKLOADQUERYPATH  ""

/* 64 bit values */

#define M64INTBITS   64
#define M64INTLBITS  6  /* log of number of bits in an int */
#define M64INTSIZE   8
#define M64INTSHIFT  3
#define M64UINT4     unsigned int
#define M64UINT8     unsigned long

/* 32 bit values */

#define M32INTBITS   32
#define M32INTLBITS  5  /* log of number of bits in an int */
#define M32INTSIZE   4
#define M32INTSHIFT  2
#define M32UINT4     unsigned long
#define M32UINT8     unsigned long long

#ifdef __M64
#define MUINT4      unsigned int
#define MUINT8      unsigned long
#define MMAX32INT   0xffffffff
#define MMAX64INT   0xffffffffffffffff
#define MBITSHIFT   M64INTLBITS
#else /* __M64 */
#define MUINT4      unsigned long
#define MUINT8      unsigned long long
#define MMAX32INT   0xffffffff
#define MMAX64INT   0xffffffff
#define MBITSHIFT   M32INTLBITS
#endif /* __M64 */

typedef struct m64_t {
  mbool_t Is64;
  int     INTBC;     /* bits in int */
  int     INTLBC;    /* log of bits in int, ie 2^X = INTBC */
  int     MIntSize;
  int     IntShift;
  } m64_t;

/* general values */

#define MUINT1     unsigned char
#define MUINT2     unsigned short

#ifndef MMAX_INT
# define MMAX_INT               0x7fffffff
#endif /* MMAX_INT */

#define MMAX_WORD           32  /* NOTE:  currently not used */
#define MMAX_LIST           17
#define MMAX_ARG            128
#define MMAX_ENVVAR         64


#define MMAX_PATH_LEN       1024 /* max size of any single file path */
#define MMAX_PATH           64   /* maximum number of directories in $PATH */

#define MMAX_TIME           2140000000

#ifndef mutime
# define mutime unsigned long
#endif /* mutime */

                                      /* These can't start at 0 because of the
                                       * way MOGetNextObject works. */
#define MCLASS_DEFAULT_INDEX     1    /* Index where default class resides*/
#define MCLASS_FIRST_INDEX       2    /* Index of first "real" class. Use this
                                       *  to begin looping after the DEFAULT
                                       *  class. */

#ifndef MMAX_UNSUPPORTEDDEPENDLIST
#define MMAX_UNSUPPORTEDDEPENDLIST     30
#endif /* MMAX_UNSUPPORTEDDEPENDLIST */

#ifndef MMAX_GRES
#define MMAX_GRES          127
#endif /* MMAX_GRES */

#define MDEF_MAXGRES MMAX_GRES
#define MMAX_CONSTGRES       12

#ifndef MMAX_GCOUNTER
#define MMAX_GCOUNTER        16
#endif /* MMAX_GCOUNTER */

#ifndef MMAX_SRSV_DEPTH
#define MMAX_SRSV_DEPTH     64
#endif

/* NOTE:  MMAX_RANGE proportional to number of simultaneous reservations (related to active jobs) */

#ifndef MMAX_RANGE
#define MMAX_RANGE          2048
#endif /* MMAX_RANGE */

#ifndef MDEF_MAXJOB
# define MDEF_MAXJOB        4096
#endif /* !MDEF_MAXJOB */

#ifndef MMAX_JOB
# define MMAX_JOB MDEF_MAXJOB
#endif /* !MMAX_JOB */

#ifndef MDEF_CJOB
# define MDEF_CJOB          1024
#endif /* !MDEF_CJOB */

#ifndef MDEF_CJOB_CEILING
# define MDEF_CJOB_CEILING (MDEF_CJOB * 100)
#endif /* !MDEF_CJOB_CEILING */

#ifndef MMAX_TJOB
# define MMAX_TJOB           256
#endif /* !MMAX_TJOB */

#ifndef MMAX_PJOB
# define MMAX_PJOB 		512
#endif /* !MMAX_PJOB */

#ifndef MMAX_JOB_TRACE
#define MMAX_JOB_TRACE        4096
#endif /* !MMAX_JOB_TRACE */

#ifndef MMAX_JOB_PER_NODE
#define MMAX_JOB_PER_NODE     64
#endif /* !MMAX_JOB_PER_NODE */

/* maximum size in nodes of any given job */
/* @see #define MDEF_JOBDEFNODECOUNT      */

#ifndef MDEF_JOBMAXNODECOUNT
#define MDEF_JOBMAXNODECOUNT   1024
#endif /* !MDEF_JOBMAXNODECOUNT */

/* default size in nodes of any given job */

#ifndef MDEF_JOBDEFNODECOUNT
#define MDEF_JOBDEFNODECOUNT  MDEF_JOBMAXNODECOUNT
#endif /* !MDEF_JOBDEFNODECOUNT */

#ifndef MDEF_JOBTASKMAPINCRSIZE
#define MDEF_JOBTASKMAPINCRSIZE  256
#endif /* !MDEF_JOBTASKMAPINCRSIZE */

#ifndef MMAX_TASK
# define MMAX_TASK          32768
#endif /* !MMAX_TASK */

#define MDEF_TASKS_PER_NODE 999999

#ifndef MMAX_PAR
# define MMAX_PAR           32
#endif /* !MMAX_PAR */

#define MDEF_PALBMSIZE     MBMSIZE(MMAX_PAR)

#ifndef MMAX_CREDS
#define MMAX_CREDS          1024  /* used for things like credential mapping */
#endif /* !MMAX_CREDS */

#ifndef MMAX_CONFIGATTR
# define MMAX_CONFIGATTR    128
#endif /* !MMAX_CONFIGATTR */

#ifndef MMAX_QOS
# define MMAX_QOS           256  /* declared to be one size smaller than is evently divisible by 8, see MUBMFLAGClear() */
#endif /* !MMAX_QOS */

#define MDEF_QOSLISTSIZE     MBMSIZE(MMAX_QOS)

/* NOTE:  size of MMAX_ATTR impacts mnode_t, mreq_t, mpar_t, mrsv_t, msrsv_t, mclass_t, mrm_t, mattrlist_t, and mvpcp_t */

#ifndef MMAX_ATTR
# define MMAX_ATTR          1024
#endif /* !MMAX_ATTR */

#define MDEF_ATTRBMSIZE     MBMSIZE(MMAX_ATTR)

#ifndef MMAX_CLASS
# define MMAX_CLASS         256
#endif /* MMAX_CLASS */

/* maximum triggers allowed per object */

#ifndef MMAX_SPECTRIG
# define MMAX_SPECTRIG          24
#endif /* MMAX_SPECTRIG */

#ifndef MMAX_RSVEVENTS
#define MMAX_RSVEVENTS 8192
#endif

#define MMAX_RSVPROFILE     32

/**
 * Reservation bounds
 *
 * MMAX_RSV_DEPTH - max rsv's allowed per node without special build 
 * MMAX_RSV_GDEPTH - max rsv's allowed per global node without special build
 * MDEF_RSV_DEPTH - default rsv's allowed per node without setting MAXRSVPERNODE parameter 
 * MDEF_RSV_GDEPTH - default rsv's allowed on global node without setting MAXRSVPERNODE parameter
 */

#ifndef MMAX_RSV_DEPTH
#define MMAX_RSV_DEPTH      4096
#endif /* MMAX_RSV_DEPTH */

#ifndef MMAX_RSV_GDEPTH
#define MMAX_RSV_GDEPTH     8192
#endif /* !MMAX_RSV_GDEPTH */

#ifndef MDEF_RSV_DEPTH
#define MDEF_RSV_DEPTH      48
#endif /* !MDEF_RSV_DEPTH */

#ifndef MDEF_RSV_GDEPTH
#define MDEF_RSV_GDEPTH     4096
#endif /* !MDEF_RSV_GDEPTH */

#ifndef MMAX_RM
#define MMAX_RM             32   /* resource managers */
#endif /* MMAX_RM */

#define MMAX_AM             4    /* accounting managers */
#define MMAX_ID             4    /* id managers */

#ifndef MMAX_FSDEPTH
# define MMAX_FSDEPTH       32
#endif /* MMAX_FSDEPTH */

#ifndef MMAX_SHAPE
#define MMAX_SHAPE          8     /* maximum number of malleable job configuration shapes */
#endif /* MMAX_SHAPE */

#ifndef MMAX_PEER
#define MMAX_PEER           20
#endif /* MMAX_PEER */

#define MMAX_NHBUF          512   /* node hash buffer/search depth */
#define MCONST_NHNEVERSET   -1
#define MCONST_NHCLEARED    -2

#define MMAX_TRANS          1024  /* maximum socket transactions (may be limited by MMAX_CLIENT)*/

#define MMAX_GRIDTIMECOUNT  32
#define MMAX_GRIDSIZECOUNT  32
#define MMAX_GRIDDEPTH      2     /* number of time intervals to track */
#define MMAX_SCALE          128

#define NONE                                   "[NONE]"
#define ALL                                    "[ALL]"
#define NOBODY                                 "[NOBODY]"
#define ANY                                    "[ANY]"

#ifndef DEFAULT
# define DEFAULT                               "[DEFAULT]"
#endif /* DEFAULT */

#define MCKPT_VERSION                          "400"
#define MCKPT_SVERSIONLIST                     "400"

#define MDEF_UMASK                             0022    /* gives 0644 whith fopen (0666 & ~022) */

#define MDEF_LOGLEVEL                          0
#define MMAX_LOGLEVEL                          20
#define MDEF_LOGFACILITY                       fALL

#define MDEF_NODETYPE                          "DEFAULT"

#define MDEF_NODEDOWNSTATEDELAYTIME            -1      /* by default don't schedule on down nodes */
#define MDEF_NODEBUSYSTATEDELAYTIME            60      /* 1 minute if node has background load */
#define MDEF_NODEDRAINSTATEDELAYTIME           10800   /* 3 hours if node is offline */

#define MDEF_NODEFAILURERSVTIME                300

#define MDEF_FREETIMELOOKAHEADDURATION        MCONST_MONTHLEN * 2
#define MDEF_NONBLOCKINGCACHEINTERVAL          -1      /* default is to cache every interval */

#define MDEF_NSDELAY                           0
#define MDEF_NSISOPTIONAL                      TRUE
#define MDEF_NSATTRIBUTE                       mrstNONE
#define MDEF_NSSELECTIONTYPE                   mrssONEOF
#define MDEF_NSPRIORITYTYPE                    mrspMinLoss
#define MDEF_NSTOLERANCE                       0.0

#define MDEF_CPINTERVAL                        300
#define MDEF_CPEXPIRATIONTIME                  3000000
#define MDEF_CPJOBEXPIRATIONTIME               30000

#define MDEF_RMPOLLINTERVAL                    30
#define MDEF_RMMSGIGNORE                       FALSE
#define MDEF_MAXSLEEPITERATION                 1
#define MDEF_SCHEDSTEPCOUNT                    60
#define MDEF_TRIGCHECKTIME                     1500  /* milliseconds */
#define MDEF_TRIGINTERVAL                      5
#define MDEF_TRIGEVALLIMIT                     1
#define MDEF_HALOCKCHECKTIME                   9
#define MDEF_HALOCKUPDATETIME                  3

#define MDEF_CONFIGSUFFIX                      ".cfg"
#define MDEF_LOCKSUFFIX                        ".pid"
#define MDEF_CHECKPOINTSUFFIX                  ".ck"
#define MDEF_LOGSUFFIX                         ".log"
#define MDEF_LOGDIR                            "log/"
#define MDEF_TOOLSDIR                          "tools/"
#ifndef MBUILD_INSTDIR
#define MDEF_INSTDIR                           "/opt/moab/"
#else
#define MDEF_INSTDIR                           MBUILD_INSTDIR
#endif
#define MDEF_STATDIR                           "stats/"

#define MDEF_PAR                               0

#define MDEF_LOGFILEMAXSIZE                    10000000
#define MDEF_LOGFILEROLLDEPTH                  3

#define MDEF_PRIMARYADMIN                      "root"
#define MMAX_ADMINTYPE                         5

#define MDEF_JOBFBACTIONPROGRAM                ""
#define MDEF_MAILACTION                        "/usr/sbin/sendmail"
#define MDEF_DEFERTIME                         3600

#ifndef MMAX_RMFAILCOUNT
#define MMAX_RMFAILCOUNT                       80
#endif /* MMAX_RMFAILCOUNT */

#define MDEF_DEFERCOUNT                        24
#define MDEF_DEFERSTARTCOUNT                   1
#define MDEF_JOBPURGETIME                      0
#define MDEF_JOBCPURGETIME                     300
#define MDEF_RSVCPURGETIME                     300
#define MDEF_VMCPURGETIME                      300
#define MDEF_NODEPURGETIME                     MMAX_TIME
#define MDEF_APIFAILURETHRESHOLD              6
#define MDEF_NODESYNCDEADLINE                  600
#define MDEF_JOBSYNCDEADLINE                   600
#define MDEF_JOBMAXOVERRUN                     600    /* job may exceed wclimit by 10min */
#define MDEF_MINSUSPENDTIME                    MCONST_MINUTELEN

#define MDEF_USEMACHINESPEED                   FALSE
#define MMAX_NODESPEED                         100.0
#define MDEF_USESYSTEMQUEUETIME                mptOff
#define MDEF_JOBPRIOACCRUALPOLICY              mjpapAccrue
#define MDEF_RESOURCEAVAILPOLICY               mrapCombined
#define MDEF_NODELOADPOLICY                    mnlpAdjustState
#define MDEF_NODEUNTRACKEDRESOURCEFACTOR       1.2
#define MDEF_NODEUNTRACKEDPROCFACTOR           1.2
#define MDEF_USELOCALMACHINEPRIORITY           mptOff

#define MDEF_MHSERVER                          "clusterresources.com"
#define MDEF_MHPORT                            80
#define MDEF_SLURMSCHEDPORT                    7321  /* default as of SLURM 1.1.8 */
#define MDEF_MHSYNCLOCATION                    "/moab/syncdir/400/update.html"

/* default policy values */

#define MDEF_FSPOLICY                          mfspNONE  /* policy to apply if nothing specified */
#define MDEF_FSTARGETPOLICY                    mfspDPS   /* policy to apply if FS target only specified */

#define MDEF_FSINTERVAL                        43200
#define MDEF_FSDEPTH                           8
#define MDEF_FSDECAY                           1.0

/* grid statistics defaults */

#define MDEF_STIMEMIN                          900
#define MDEF_STIMEMAX                          0
#define MDEF_STIMESTEPCOUNT                    5
#define MDEF_STIMESTEPSIZE                     4

#define MDEF_SPROCMIN                          1
#define MDEF_SPROCMAX                          0
#define MDEF_SPROCSTEPCOUNT                    4
#define MDEF_SPROCSTEPSIZE                     4

#define MDEF_ACCURACYSCALE                     10

#define MDEF_BFPOLICY                          mbfFirstFit
#define MDEF_BFDEPTH                           0
#define MDEF_BFNODEFACTOR                      0
#define MDEF_BFMAXSCHEDULES                    10000
#define MDEF_BFMETRIC                          mbfmProcs

#define MDEF_SRDEPTH                           2
#define MDEF_SRTIMELOGIC                       mclOR
#define MDEF_SRPRIORITY                        0
#define MDEF_SRPERIOD                          mpDay
#define MDEF_SRSTARTTIME                       0
#define MDEF_SRENDTIME                         0
#define MDEF_SRPROCS                           -1
#define MDEF_SRTPN                             0

#define MCONST_HOURSPERDAY                     24
#define MCONST_DAYSPERWEEK                     7
#define MCONST_MONTHSPERYEAR                   12
#define MCONST_MINUTELEN                       60
#define MCONST_MINPERHOUR                      60
#define MCONST_HOURLEN                         3600
#define MCONST_DAYLEN                          86400
#define MCONST_WEEKLEN                         604800
#define MCONST_MONTHLEN                        2592000
#define MCONST_YEARLEN                         31536000
#define MCONST_ALLDAYS                         16 /* just some random number that is more than 7 */

/* NOTE: adjust MMAX_CLIENTSKEW to adjust sensitivity to old client requests - old
         requests will be rejected by the server (-1 disables timestamp check) */

#define MMAX_CLIENTSKEW                        3600

#define MCONST_EFFINF                          200000000  /* ~ 6.0 years */

/* For rsvs, infinity is MMAX_TIME -1.  For jobs, it's MMAX_TIME */
#define MCONST_RSVINF                          MMAX_TIME - 1

#define MDEF_ADMINEACTIONPROGRAM               ""

#define MDEF_BCCLIENTTIMEOUT                   60000000  /* add longer timeout for large clusters */
#define MDEF_CLIENTTIMEOUT                     30000000  /* (in microseconds) */

/* system defaults */

#define MDEF_SYSDEFJOBWCLIMIT                  8639999
#define MDEF_SYSMAXJOBWCLIMIT                  MCONST_EFFINF

#define MDEF_MAXMETATASKS                      0
#define MMAX_JOBSTARTTIME                      MMAX_TIME
#define MDEF_TASKDISTRIBUTIONPOLICY            mtdDefault
#define MDEF_JOBSIZEPOLICY                     mjsMinProc
#define MDEF_JOBRETRYTIME                      60
#define MDEF_NODEIDLEPOWERTHRESHOLD            60

#define MDEF_RSVPOLICY                         mrpCurrentHighest
#define MDEF_RSVRETRYTIME                      60
#define MDEF_PREEMPTRSVTIME                    120
#define MDEF_RSVTHRESHOLDTYPE                  mrttNONE
#define MDEF_RSVTHRESHOLDVALUE                 0
#define MDEF_RSVDEPTH                          1
#define MDEF_RSVQOSLIST                        (mqos_t *)MMAX_QOS
#define MMAX_QOSBUCKET                         (MMAX_QOS >> 1)
#define MDEF_JOBRSVRETRYINTERVAL               1
#define MDEF_AUTHTIMEOUT                       3

#define MDEF_SYSQAL                            0
#define MDEF_SYSPAL                            0
#define MDEF_SYSPDEF                           0
#define MDEF_SYSQDEF                           0

#define MMAX_ACCURACY                          5

/* rm defaults */

#define MDEF_PBSSUBMITEXE                   "/usr/local/bin/qsub"
#define MDEF_PBSQSUBPATHNAME                "qsub"
#define MDEF_SLURMSRUNPATHNAME              "srun"
#define MDEF_SLURMSBATCHPATHNAME            "sbatch"
#define MDEF_SLURMSALLOCPATHNAME            "salloc"

/* virtualization defaults */
#define MDEF_VIRTUALNODEPREFIX              "v_"
#define MDEF_VMCREATETHROTTLE                 -1
#define MDEF_VMMIGRATETHROTTLE                -1

#define MDEF_MAXCLIENT                      128   /* maximum number of client connections that can be open at a time */


/* macros */

#ifndef MIN
#  define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#  define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

/* NOTE:  MMOVEPTR causes issues with some C++ and AIX based compilers */

#define MMOVEPTR(S,D) { D = S;S = (char *)0; }

/* elemental objects */

typedef struct mtree_t {
  struct mtree_t *L;
  struct mtree_t *R;

  char            SBuf[MMAX_NAME];

  void           *Key;
  int             Type;
  } mtree_t;

typedef struct mxtree_t {
  mtree_t     *Root;
  mtree_t     *Buf;

  int          BufSize;
  int          BufIndex;
  } mxtree_t;


/* sparse numlist class */

typedef class msnl_t {
  public:
  int Init();
  int Clear();
  int Free();
  int Size() const;
  mbool_t IsEmpty() const;
  int RefreshTotal();
  int SetCount(int,int);
  int GetIndexCount(int) const;
  int GetSum() const;
  int Copy(const msnl_t *);
  int HowManyFit(const msnl_t *,int *) const;  // previously known as: "GetCount"
  int ToMString(msnl_t *,const char *,mstring_t *,int);

  int FromString(msnl_t *,const char *,const char *,mrm_t *);

  private:
  std::map<short,int> data;   /// a collection of counts stored by their index
  int                 sum;    /// contains the sum of all the counts from all indices
                              /// it should always be kept in sync with data.
  } msnl_t;

  typedef msnl_t MSNL;

#define MMAX_FILECACHE  32
#define MMAX_FBUFFER    16000000

typedef struct muenv_t {
  int    Size;
  int    Count;
  char **List;
  } muenv_t;

typedef struct mcres_t {
  int     Procs;
  int     Mem;    /* (in MB) */
  int     Swap;   /* (in MB) */
  int     Disk;   /* (in MB) */

  mbool_t GResSpecified;         /* For job templates, TRUE if specified a GRes */

  msnl_t  GenericRes;
  } mcres_t;


/* Structure to hold a void pointer and a type.
 * Useful for structures that may need to hold different object types
 *  such as nodes and vms, etc.
 */

typedef struct mvoid_t {
  void  *Ptr;
  char   Name[MMAX_NAME];
  enum   MXMLOTypeEnum PtrType;
  } mvoid_t;


typedef struct mnodea_s {
  int              AIndex;    /* attribute index */
  long             CfgTime;   /* duration to reconfigure */
  mbitmap_t        Flags;     /* bitmap of node attr flags (NYI) */
  struct mnodea_s *Next;
  } mnodea_t;


typedef struct mln_s {
  char         *Name;      /* OID (alloc) */
  int           NameLen;   /* the strlen of the name (calculated at creation time only) */
  void         *Ptr;       /* internal object structure (WARNING:  sometimes alloc) */
  int           ListSize;  /* the number of items in this linked list (only valid for list head) */

  mbitmap_t     BM;    /* generic BM */

  struct mln_s *Next;  /* next structure in list */
  } mln_t;


/** 
 * History object
 * 
 * NOTE: don't use alloc as this routine uses the marray_t structures
 *       and marray routines copy data rather than using originals
 */

typedef struct mhistory_s {
  mulong  Time;

  mcres_t Res;

  int ElementCount;
  } mhistory_t;


typedef struct mfutureres_t {
  mcres_t ConsumedRes;  /* expected future consumed resources */
  double  CPULoad;      /* expected future CPULoad */
  struct mgmetric_t **GMetric; /* expected future gmetrics */
  } mfutureres_t;


/**
 * Power Policy (sync w/MPowerPolicy[])
 */

enum MPowerPolicyEnum {
  mpowpNONE = 0,
  mpowpGreen,
  mpowpOnDemand,
  mpowpStatic,
  mpowpLAST };


/**
 * Power State (sync w/MPowerState[])
 */

enum MPowerStateEnum {
  mpowNONE = 0,
  mpowOff,
  mpowOn,
  mpowReset,
  mpowHibernate,
  mpowStandby,
  mpowLAST };


/**
 * Job template (workflow) failure policy (sync w/MJobTemplateFailurePolicy[])
 */

enum MJobTemplateFailurePolicyEnum {
  mjtfpNONE = 0,
  mjtfpCancel,
  mjtfpHold,
  mjtfpRetry,
  mjtfpRetryStep,
  mjtfpLAST };


/**
 * System job special type
 *
 * @see designdoc SystemJobDoc
 * @see MSysJobType[] (sync)
 * @see struct mjob_t.System.ICPVar1
 */

enum MSystemJobTypeEnum {
  msjtNONE = 0,
  msjtGeneric,      /**< generic sysjob (trigger attached) */
  msjtOSProvision,  /**< reprovision OS */
  msjtOSProvision2, /**< perform 2-phase (base+VM) OS reprovision */
  msjtPowerOff,     /**< power off node */
  msjtPowerOn,      /**< power on node */
  msjtReset,        /**< reboot node */
  msjtStorage,      /**< dynamic storage allocation */
/*  msjtVMCreate,     *< create virtual machine */
/*  msjtVMDestroy,    *< destroy existing virtual machine */
  msjtVMMap,        /**< map to VM to track resource consumption */
  msjtVMMigrate,    /**< migrate virtual machine */
/*  msjtVMStorage,    *< create storage mounts for VMs */
  msjtVMTracking,   /**< job for tracking a VM */
  msjtLAST };


/**
 * System job completion policy types
 *
 * @see MJobCompletionPolicy[] (sync)
 * @see mjob_t.msysjob_t.CompletionPolicy
 * @see enum MSystemJobTypeEnum
 * @see designdoc SystemJobDoc
 */

enum MJobCompletionPolicyEnum {
  mjcpNONE = 0,
  mjcpGeneric,
  mjcpOSProvision,
  mjcpOSProvision2,
  mjcpOSWait, /* This entry is needed for consolidation to work. I am not yet sure why - Sean */
  mjcpPowerOff,
  mjcpPowerOn,
  mjcpReset,
  mjcpStorage,
  mjcpVMCreate,
  mjcpVMDestroy,
  mjcpVMMap,
  mjcpVMMigrate,
  mjcpVMStorage,
  mjcpVMTracking, /* Job for tracking a VM */
  mjcpLAST };


/**
 * System job completion actions
 *
 * sync w/ MSystemJobCompletionAction
 * @see designdoc SystemJobDoc
 */

enum MSystemJobCompletionActionEnum {
  msjcaNONE,
  msjcaReserve,
  msjcaJobStart,
  msjcaLAST };


/**
 * System job error flags 
 *
 * @see msysjob_t 
 */

enum MSystemJobErrorFlagsEnum {
  msjefNONE,
  msjefOnlyProvRM,
  msjefProvFail,
  msjefProvRMFail,
  msjefPowerRMFail,
  msjefLAST };

#define MDEF_SYSJOBEFLAGBMSIZE MBMSIZE(msjefLAST)



/** @see designdoc HashTable */

#define MDEF_HASHTABLESIZE 2 /* size in power of 2 (the actual size of the table is 2^MDEF_HASHTABLESIZE ... or 4 in this case) */
#define MDEF_HASHCHAINMAX  4 /* the largest a hash's chain should get before the table is grown and rehashed */

struct mhash_t {

  int NumCollisions;  /* used for performance tracking--counts num. bucket collisions */
  int LongestChain;   /* used for performance tracking--keeps track of the longest chain in a single bucket */
  int HashSize;       /* current size of hash table */
  int NumItems;       /* number of items in this hash table */

  mln_t **Table;      /* the table of hash buckets (each possibly holding a linked list) */
  };

typedef struct mhashiter_t {

  unsigned int Bucket;
  mln_t *LLNode;

  } mhashiter_t;


/**
 * Generic Event Action
 *
 * @see MSysProcessGEvent() - process triggered event
 * @see MSchedProcessOConfig():mcoGEventCfg - process config
 *
 * NOTE: sync w/MGEAction[]
 */

enum MGEActionEnum {
  mgeaNONE = 0,
  mgeaAvoid,
  mgeaCharge,
  mgeaDisable,
  mgeaEnable,
  mgeaExecute,
  mgeaFail,
  mgeaNotify,
  mgeaObjectXML,       /* passes XML of the GEvent to the script, like objectxmlstdin on triggers */
  mgeaOff,             /* power device off immediately */
  mgeaOn,
  mgeaPreempt,
  mgeaRecord,
  mgeaReserve,
  mgeaReset,
  mgeaScheduledReset,  /* reset when node is idle */
  mgeaSignal,          /* signal jobs or service daemons */
  mgeaSubmit,
  mgeaLAST };


/* GEvent Structures:  Instance information */

typedef struct mgevent_obj_t {
  char  *Name;          /* generic event name (alloc) */
  mulong GEventMTime;   /* generic event modification time */
  char  *GEventMsg;     /* generic event message (alloc) */
  int    Severity;      /* severity reported with this instance (0 - unset) */
  int    StatusCode;     /* error code for this event */
} mgevent_obj_t;



#define MMAX_GEVENTSEVERITY 4

/* GEvent Description information
 *  Stores general information, stats for a gevent
 */

typedef struct mgevent_desc_t {
  char    *Name;                  /* name of the gevent (alloc) */
  mbitmap_t   GEventAction;          /* actions to take when generic
                                       event received (bitmap of
                                       enum MGEActionEnum) */
  char    *GEventActionDetails[mgeaLAST];   /* action details (alloc) */
  int      GEventCount;           /* number of times event 
                                       has occured */
  int      GEventAThreshold;      /* number of times event should
                                       occur before action is taken */
  mulong   GEventReArm;           /* minimum time between
                                       re-processing event */
  mulong   GEventLastFired;       /* last time gevent was fired */

  int      Severity;              /* 0 is not set, otherwise 1 through MMAX_EVENTSEVERITY (inclusive) - overridden by mgevent_obj_t->Severity */
} mgevent_desc_t;


/* GEvent Iterator structure */

typedef struct mgevent_iter_t {
  mhashiter_t  htiter;              /* Encapulate how this iterator is implemented */
} mgevent_iter_t;

/* GEvent List Container */

typedef struct mgevent_list_t {
  mhash_t     Hash;
} mgevent_list_t;



/* sync w/ MPendingActionAttr[] */

enum MPendingActionAttrEnum {
  mpaaNONE = 0,
  mpaaDescription,
  mpaaHostlist,
  mpaaMaxDuration,
  mpaaMigrationSource,
  mpaaMigrationDest,
  mpaaMotivation,
  mpaaPendingActionID,
  mpaaRequester,
  mpaaSched,
  mpaaStartTime,
  mpaaState,
  mpaaSubState,
  mpaaTargetOS,
  mpaaType,
  mpaaVariables,
  mpaaVCVariables,
  mpaaVMID,
  mpaaLAST };

/* sync w/ MPendingActionType[] */

enum MPendingActionTypeEnum {
  mpatNONE = 0,
  mpatVMOSProvision,
  mpatVMPowerOff,
  mpatVMPowerOn,
  mpatLAST };

/* NOTE:  MMAX_GMETRIC arrays are 1-based (because of MAList) */

#define MDEF_GMETRIC_VALUE 999999999

#ifndef MMAX_GMETRIC
#define MMAX_GMETRIC 10
#endif

#define MDEF_MAXGMETRIC MMAX_GMETRIC

/* sync w/ MVMFlags[] */

enum MVMFlagEnum {
    mvmfNone,
    mvmfCanMigrate,          /**< VM may be directly migrated w/o direction from parent workload */
    mvmfCreationCompleted,   /**< specifies that the RM now reports the VM NOTE: does not indicate that VM is fully initialized and ready for use */
    mvmfCompleted,           /**< VM has been moved to the completed table */
    mvmfDeleted,             /**< Moab is in process of freeing mvm_t structure */
    mvmfDestroyPending,      /**< VM destroy requested */
    mvmfDestroyed,           /**< VM reported destroy by RM - remove when RM no longer reports VM */
    mvmfDestroySubmitted,    /**< VM destroy system job created */
    mvmfInitializing,        /**< VM creation/initialization process still underway */
    mvmfOneTimeUse,
    mvmfShared,
    mvmfSovereign,           /**< VM should be treated as resource consumer regardless of Moab-tracked workload */
    mvmfUtilizationReported, /**< VM utilization info reported via RM */
    mvmfLAST };



enum MVMIFlagEnum {
    mvmifNone,
    mvmifReadyForUse,   /**< VM is fully ready for use */
    mvmifUnreported,    /**< VM was loaded from CP, but hasn't be reported by RM */
    mvmifDeletedByTTL,  /**< VM is being removed because of TTL expiration */
    mvmifLAST };




/* Structure for holding VM storage mount info */

typedef struct mvm_storage_t {
  char  Name[MMAX_NAME];     /** storage name (not globally unique, just unique to the vm) */

  char  Location[MMAX_LINE]; /** location of the allocated storage (path given by MSM) */
  char  MountPoint[MMAX_LINE]; /** location to place the storage mount in the VM */
  char  MountOpts[MMAX_LINE*2];  /** mount options */

  char  Type[MMAX_NAME];     /* Type (gold, silver, etc) */
  int   Size;                /* Size (number after the ':') */

  struct mrsv_t *R;                 /* Reservation that was created for this mount -> deleted by parent VM */

  char   Hostname[MMAX_NAME];       /* Hostlist of the reservation (will be only one node) */
              /* Hostname is used instead of R->NL[0].N->Name because of checkpointing issues */

  struct mtrig_t *T;                /* Trigger that will be setting up this mount */
  char  TVar[MMAX_NAME];
  }mvm_storage_t;



/**
 * Virtual Machine Structure (@struct mvm_t)
 *
 * @see VM Management Design Doc
 *
 * @see mnvirtual_t - structure for alternate method of VM management
 * @see MVMDestroy() - perform 'deep' free
 * @see MVMAToString() - sync
 * @see MVMSetAttr() - sync
 * @see MVMToXML() - must add new attributes which should be reported via XML
 * @see enum MVMAttrEnum - sync
 */

#define MMIN_VMPERNODE           8

struct mvm_t {
  char      VMID[MMAX_NAME];  /**< VM ID as reported via ProvRM */

  char      JobID[MMAX_NAME]; /**< if specified, VM is currently running job. */
                                   
  char     *OwnerJob; /**< if specified, VM is completely owned by job and cannot run other jobs. Used for on-demand vm jobs */

  mbitmap_t Flags; 
  mbitmap_t IFlags;

  enum MNodeStateEnum State;     /**< effective VM state as reported by the compute RM running on the VM (i.e. state
                                      as perceived by pbs_mom) */

  char     *SubState;            /**< VM substate (i.e. ACTUAL state of the VM),
                                    as reported by MSM/provisioning manager (alloc,opaque) */

  char     *LastSubState;        /**< Last reported substate (substate is not always reported) */
  mutime    LastSubStateMTime;   /**< Time that LastSubState was modified */

  mcres_t   ARes;  /**< available resources w/in VM */
  mcres_t   CRes;  /**< configured resources w/in VM */
  mcres_t   DRes;  /**< configured resources w/in VM */
  mcres_t   LURes; /**< historical 'long-term' utilization information */

  double    CPULoad;
  double    NetLoad;

  int       Rack;
  int       Slot;   /**< populate in ??? */

  mrm_t    *RM;
  mrm_t   **RMList; /* (alloc) */
  enum MNodeStateEnum RMState[MMAX_RM];  /**< rm specific state */


  mnodea_t *OSList;    /**< potential operating systems (alloc,size=MMAX_NODEOSTYPE) */

  int       ActiveOS;
  int       NextOS;               /**< OS VM is currently provisioning to */
  mulong    LastOSModRequestTime; /**< time last OS change was requested */
  int       LastOSModIteration;   /**< Iteration last OS change was completed */

  mulong    LastMigrationTime;    /**< Last time this VM was migrated */
  int       MigrationCount;       /**< Number of times that this VM has been migrated */

  enum MPowerStateEnum PowerState;       /**< node power state reported by RM */
  enum MPowerStateEnum PowerSelectState; /**< node power state requested by Moab power policy */
  mulong    PowerMTime;                  /**< time VM power state change was requested */

  int       UpdateIteration;             /**< iteration vm was last updated by RM */
  mulong    UpdateTime;                  /**< time vm was last updated by RM */

  mulong    StateMTime;                  /**< time since last state change */

  struct mjob_t  *J;                     /**< pointer to parent system job - NOTE:  not same as V->JobID */
  struct mnode_t *N;                     /**< pointer to physical node currently hosting VM (ie, parent) */

  struct mjob_t  *CreateJob;             /**< vmcreate job for this VM */

  mln_t    *Action;       /**< list of job id's for all active VM system jobs */

  char     *LastActionDescription;       /**< human readable message describing last completed action (alloc) */
  mulong    LastActionStartTime;
  mulong    LastActionEndTime;

  long      SpecifiedTTL;                /**< user specified time that this VM has to run (0 - not set) */
  long      EffectiveTTL;                /**< time that this VM has to run (0 - not set) */
  mulong    StartTime;                   /**< time that this VM was started (ready and usable) */

  enum MSystemJobTypeEnum LastActionType;

  char     *Description;                 /**< vm description/comment (alloc) */

  /* NOTE: application-specific metrics contained in mjtx_t (J->TX) */

  mgevent_list_t   GEventsList;  /* List of mgevent_obj_t items (alloc elements - add with MGEventAddItem) */

  mhash_t   Variables;    /* hash of variables (alloc elements) */

  mln_t    *Storage;  /* linked list of storage mounts for this VM (type mvm_storage_t) */
  mln_t    *StorageRsvs; /* linked list of mrsv_t for storage */

  char     *StorageRsvNames; /* names of StorageRsvs reservations, used for checkpointing (alloc) */

  char     *NetAddr;  /* Network address, for example IP address. (alloc) */

  mln_t    *Aliases;  /* Network hostname aliases (alloc elements - char *) */

  struct mgmetric_t **GMetric;

  marray_t *T;      /* vm triggers (alloc) */

  mln_t *ParentVCs; /* Parent VCs (alloc) - create only in MVCAddObject() */

  mjob_t   *TrackingJ; /* VMTracking job for this VM */
  struct mrsv_t *OutOfBandRsv; /* Rsv to represent this VM's resources (if out-of-band ->
                              should be NULL if a VMTracking job exists) */
  };  /* END mvm_t */



/**
 * VM attributes
 *
 * @see MVMAttr[] (sync)
 * @see mvm_t (sync)
 */

enum MVMAttrEnum {
  mvmaNONE = 0,
  mvmaActiveOS,
  mvmaADisk,
  mvmaAlias,
  mvmaAMem,
  mvmaAProcs,
  mvmaCSwap,
  mvmaCDisk,
  mvmaCMem,
  mvmaCProcs,
  mvmaCPULoad,
  mvmaContainerNode,
  mvmaDDisk,
  mvmaDMem,
  mvmaDProcs,
  mvmaDescription,
  mvmaEffectiveTTL,
  mvmaFlags,
  mvmaGEvent,
  mvmaGMetric,
  mvmaID,
  mvmaJobID,
  mvmaLastMigrateTime,
  mvmaLastSubState,
  mvmaLastSubStateMTime,
  mvmaLastUpdateTime,
  mvmaMigrateCount,
  mvmaNetAddr,
  mvmaNextOS,
  mvmaNodeTemplate,
  mvmaOSList,
  mvmaPowerIsEnabled,
  mvmaPowerState,
  mvmaPowerSelectState,
  mvmaProvData,
  mvmaRackIndex,
  mvmaSlotIndex,
  mvmaSovereign,
  mvmaSpecifiedTTL,
  mvmaStartTime,
  mvmaState,
  mvmaSubState,
  mvmaStorageRsvNames,
  mvmaTrackingJob,
  mvmaTrigger,
  mvmaClass,
  mvmaVariables,
  mvmaLAST };




typedef struct mrack_t {
  char   Name[MMAX_NAME + 1];   /* name */

  int    Index;

  long   MTime;
  int    Type;

  int    Network;    /* total KB/s of network activity of all nodes in rack */

  int    Memory;     /* per node memory in rack (in MB) */
  int    Disk;       /* per node disk space in rack (in MB) */

  int    PtIndex;    /* partition index */
  int    NodeCount;
  int    TempSize;
  char   NodeType[MMAX_NAME];
  } mrack_t;



/* list-range */

typedef struct mlr_t {
  char **List;
  char  *Prefix;
  int    PrefixLen;
  int    MinIndex;
  int    MaxIndex;
  } mlr_t;



/* sync w/MMBType[] */

enum MMBTypeEnum {
  mmbtNONE = 0,
  mmbtAnnotation,  /* permanent object comment - do not checkpoint */
  mmbtHold,        /* hold reason/hold details */
  mmbtOther,
  mmbtPendActionError, /* error from MSM with a system job */
  mmbtDB,
  mmbtLAST };



/* sync w/MMBAttr[] */

enum MMBAttrEnum {
  mmbaNONE = 0,
  mmbaCount,
  mmbaCTime,
  mmbaData,
  mmbaExpireTime,
  mmbaLabel,
  mmbaOwner,
  mmbaPriority,
  mmbaSource,
  mmbaType,
  mmbaLAST };


/* environment:
      handle network adapter failure
      file system full
      service down
      mount is missing

  each event requires status
    active  - problem is currently active
    cleared - problem no longer actively detected
  each event should have optional message
  each event requires incidence count
  each event requires event time
  each event requires impact mask -
    avoid - adjust node allocation to minimize scheduling on node
    disable - mark node as unavailable
    execute - execute specified action
    notify - mail administrators
    record - record event to exernal system
    reserve - create system rsv on node for specified period of time


------
# moab.cfg

EVENTCFG[badadapter] ACTION=avoid,execute,reserve,notify DURATION=24:00:00 MAXEVENT=2
EVENTCFG[lowmem]     ACTION=disable,notify  DURATION=8:00:00 MAXEVENT=4
...
------

------
# node.dat (for native rm cluster query)

node01 GEVENT[badadapter]='myrinet adapter is glitching' GEVENT[lowmem]='swap is below 10MB' APROCS=13
...
------

  each event should allow admin to clear, annotate, or delete it
*/

/* NOTE:  need per node generic event buffer
 *  Attributes:
 *   1 - native rm can set event
 *   2 - event should be reported via 'mdiag -n', checknode, and mcm
*/

typedef struct mmb_t {

  char *Data;        /* message string (alloc) */

  char *Label;       /* message label (alloc) */
  char *Owner;       /* message creator - user (alloc) */
  char *Source;      /* message source - service (alloc) - used in job transitioning to say which Moab set the message */
  long  ExpireTime;  /* message expiration date */
  long  CTime;       /* message creation time */
  enum MMBTypeEnum Type;

  int   Priority;    /* message priority (-1 == no checkpoint) */
  int   Count;       /* message instance count */

  mbool_t IsPrivate; /* message can only be viewed by admins */

  struct mmb_t *Next;        /* link to next message */
  } mmb_t;


/* trace constants */

#define MCONST_TRACE_WORKLOAD_VERSION          "VERSION"
#define MCONST_TRACE_RESOURCE_VERSION          "VERSION"

typedef struct mprofcfg_t {
  int           IStatDuration;  /* profile interval duration */
  long          IStatEnd;       /* end of current profile interval */
  int           MaxIStatCount;  /* max number of steps */

  unsigned long TraceCount;

  unsigned long MinTime;
  unsigned long MaxTime;
  unsigned long TimeStepCount;
  unsigned long TimeStep[MMAX_SCALE];
  unsigned long TimeStepSize;

  unsigned long MinProc;
  unsigned long MaxProc;
  unsigned long ProcStepCount;
  unsigned long ProcStep[MMAX_SCALE];
  unsigned long ProcStepSize;

  unsigned long AccuracyScale;
  unsigned long AccuracyStep[MMAX_SCALE];

  long          StartTime;
  long          EndTime;
  } mprofcfg_t;


/**
 * Object modify operations
 *
 * @see MObjOpType[] (sync)
 * @see MComp[] - peer - object comparisons
 */

enum MObjectSetModeEnum {
  mNONE2 = 0,
  mVerify,
  mSet,
  mSetOnEmpty,
  mAdd,
  mClear,
  mUnset,
  mDecr,
  mLAST2 };

#define mIncr mAdd

#define MDEF_PSTEPDURATION   1800  /* 30 minutes */
#define MDEF_PSTEPCOUNT      50    /* 30 * 50 ~ 1 day (everything else is written to file) */

/* @see EnvRSDoc */
#define ENVRS_SYMBOLIC_STR     "~rs;"
#define ENVRS_ENCODED_STR      "\036"
#define ENVRS_ENCODED_CHAR     '\036'

typedef struct mjobe_t {
  char  *IWD;                /**< initial working directory of job (alloc)     */
  char  *SubmitDir;          /**< directory that msub was called from (alloc)  */
  char  *Cmd;                /**< job executable (alloc)                       */
  char  *Input;              /**< requested input file (alloc)                 */
  char  *Output;             /**< requested/created output file (alloc)        */
  char  *Error;              /**< requested/created error file (alloc)         */
  char  *RMOutput;           /**< stdout file created by RM (alloc)            */
  char  *RMError;            /**< stderr file created by RM (alloc)            */
  char  *Args;               /**< command line args (alloc)                    */
  char  *BaseEnv;            /**< shell environment (alloc,format='<ATTR>=<VAL><ENVRS_ENCODED_CHAR>[<ATTR>=<VAL><ENVRS_ENCODED_CHAR>]...') */
  char  *IncrEnv;            /**< override environment (alloc,format='<ATTR>=<VAL><ENVRS_ENCODED_CHAR>[<ATTR>=<VAL><ENVRS_ENCODED_CHAR>]...') */
  char  *Shell;              /**< execution shell (alloc)                      */

  int    UMask;              /**< default file creation mode mask */
  } mjobe_t;

typedef struct mnallocp_t {
  enum MNodeAllocPolicyEnum NAllocPolicy;
  } mnallocp_t;

/* data location enum (sync w/XXX) */

enum MDataLocationEnum {
  mdlNONE = 0,
  mdlHost,
  mdlResource,
  mdlGlobal,
  mdlLAST };


/* status of data-staging reqs enum (sync w/MDataStageState[]) */

enum MDataStageStateEnum {
  mdssNONE = 0,
  mdssPending,
  mdssStaged,
  mdssFailed,
  mdssRemoved };


/* different types of msdata_t objects (sync w/MDataStageType[]) */

enum MDataStageTypeEnum {
  mdstNONE = 0,
  mdstStageIn,
  mdstStageOut };


/* staging data */

typedef struct msdata_t {
  enum MDataStageTypeEnum Type;

  char *SrcLocation;     /* source URL (alloc) */
  char *DstLocation;     /* destination URL (alloc) */

  enum MDataStageStateEnum State;

  enum MDataLocationEnum Scope;

  /* NOTE:  IsActive boolean evaluated w/SrcFileSize != DstFileSize */

  char *SrcProtocol;     /* (alloc) */
  char *DstProtocol;     /* (alloc) */

  char *SrcFileName;     /* (alloc) */
  char *DstFileName;     /* (alloc) */

  char *SrcHostName;     /* (alloc) */
  char *DstHostName;     /* (alloc) */

  /* NOTE: If the src and dst file sizes are equal, we assume that
   * the staging operation has completed.  If the src size is not
   * specified, then both the src and dst sizes will be set to one
   * when the storage RM reports the staging operation is complete.
   *
   * NYI: These must be changed to support really big file sizes.
   */

  mulong  SrcFileSize;   /* total file size (in bytes) */
  mulong  DstFileSize;   /* current destination file size (in bytes) */

  mulong  UTime;         /* last time the DstFileSize changed - used to detect "stalled" staging */

  mulong  TStartDate;    /* when data transfer was initiated (set to 0 if transfer not started) */

  double  TransferRate;  /* MB/s (should this ever differ from the storage manager's MaxStageBW?) */

  mulong ESDuration;     /* estimated stage duration (set to MMAX_TIME if unknown) */
                         /* NOTE:  ESDuration should be set to 0 if stage operation is complete */

  char  *EMsg;           /* holds any error messages reported by data-staging scripts (alloc) */

  mbool_t IsProcessed;   /* stage is complete and has been post-processed */

  struct msdata_t *Next;
  } msdata_t;


#define MDEF_ENABLEIMPLICITSTAGEOUT TRUE
#define MDEF_HIDEVIRTUALNODES       TRUE

#define MDEF_AGGREGATENODEACTIONSTIME 60

/**
 * Node Allocation Priority structure.
 *
 * @see enum MNPrioEnum (sync)
 * @see enum MNodeAllocPolicyEnum
 * @see mnode_t.P
 *
 * @see MNLSort() - sort node list based on allocation priority
 * @see MNodeGetPriority() - calculate effective allocation priority for node
 *
 * NOTE:  as new priority components are added, support for components must be
 *        explicitly specified in MNodeGetPriority()
 */

struct mnprio_t {
  double  CW[mnpcLAST];      /* component weight */
  mbool_t CWIsSet;

  double  SP;                /* static priority  */
  mbool_t SPIsSet;

  double  DP;                /* dynamic priority */
  mbool_t DPIsSet;
  int     DPIteration;

  int     AIndex;            /* architecture index */
  double  *GMValues;         /* generic metric values (optional) */
  double  *AGRValues;        /* available generic resource values (optional) */
  double  *CGRValues;        /* available generic resource values (optional) */
  double  FValues[MMAX_ATTR];              /* feature values (optional) */
  };


/* msub journal (job submission journal) */

typedef struct msubjournal_t {

  FILE *FP;
  mulong JournalSize;
  int OutstandingEntries;
  mmutex_t Lock;
  long LastClearTime;
  } msubjournal_t;


/* extended load metrics (must checkpoint w/node object) */

/**
 * Generic Metrics Overview
 *
 * @see struct mnust_t.XLoad[]
 * @see struct must_t.XLoad[]
 * @see mnstXLoad/mnstGMetric in enum MNodeStatTypeEnum
 * @see mstaXLoad/mstaGMetric in enum MStatAttrEnum
 *
 * @see MStatsUpdateActiveJobUsage() - sync job GMetrics with cred GMetrics
 * @see MStatToXML() - report GMetric stats as XML
 * @see MStatAToString() - report GMetric as string
 * @see MStatFromXML() - update GMetric values from XML source data
 *
 * NOTE:  support generic performance metrics, X0 through X5
 *        if specified, should be able to
 *          1 - forward information to MCM via 'mdiag -n'
 *          2 - adjust node allocation policies
 *          3 - display information via checknode
 *          4 - generate min/max/avg stats
 *          5 - checkpoint generic metric info
 *          6 - clear/reset generic metric info
 *          7 - fire triggers based on generic metric thresholds
 *            (particularly threshold based admin notification, ie, send email if 'GMetric[3].Avg > X')
 *          8 - display usage distribution over time (ie flow control style window)
 */

/* NOTE:  node gmetrics are moved from N->XLoad to N->Stat.XLoad and
          N->Stat.IStat[0].XLoad in MNodeUpdateProfileStats()
          node gmetrics are reported via MUINodeStatsToXML() using 'showstats -n'
 */

typedef struct mgmetric_t {
  double IntervalLoad;
  double TotalLoad;
  mulong Start;
  mulong Count;
  double Base;
  double Max;
  double Min;
  double Avg;
  double Limit;
  double Total;

  mulong EMTime;
  mulong SampleCount;
  } mgmetric_t;


typedef struct mxload_t {
  mgmetric_t  **GMetric;

  mbitmap_t     RMSetBM;     /* value reported via RM */
  } mxload_t;


typedef struct mjckpt_t {
  long   CPInterval;

  long   StoredCPDuration; /* duration of walltime checkpointed in previous runs */
  long   ActiveCPDuration; /* duration of walltime checkpointed in current run */

  long   InitialStartTime;
  long   LastCPTime;

  mbool_t SystemICPEnabled;
  } mjckpt_t;


#define MMAX_RESPONSEDATA 4

/**
 * @struct msocket_t 
 * structure for client communication
 */

typedef struct msocket_t {
  /* base config */

  long  Version;
  int   sd;                     /* socket descriptor */

  char  Name[MMAX_NAME];        /* logical name of client */
  char  Label[MMAX_NAME];       /* generic label for socket instance */
  char  Host[MMAX_NAME];        /* local host name (is this ever really used?) */
  char  RemoteHost[MMAX_NAME];  /* name of remote host on other side of this socket */
  int   RemotePort;             /* the remote port on the other side of this socket */
  char *URI;                    /* (alloc) */

  mbitmap_t Flags;              /* bitmap of MSockFTypeEnum */

  mulong Timeout;               /* (in microseconds) */

  mulong LocalTID;              /* locally assigned request transaction id */
  mulong RemoteTID;             /* remotely assigned request transaction id */
  mulong JournalID;             /* ID used to identify this socket in the submit journal */

  enum MSvcEnum SIndex;         /* requested service type            */

  char *RID;                    /* requestor ID-actor (alloc)        */
  char *Forward;                /* forwarding destination (alloc)    */

  int   LogThreshold;           /* the log level to be used (user command log level) */
  char *LogFile;                /* the log file that this command will log to (alloc) */


  /* message data */

  long  StatusCode;             /* return code of service request (enum MSFC) */
  char *SMsg;                   /* (alloc) */

  char *ClientName;             /* (alloc) */

  struct mxml_t *RE;                   /* incoming message - full (alloc) */
  struct mxml_t *RDE;                  /* incoming message (alloc) */
  char *RBuffer;                /* raw message data (alloc) */
  long  RBufSize;               /* size of RBuffer */
  char *RPtr;                   /* pointer to data element value */

  mxml_t *SE;                   /* xml to send - full (alloc,w/header) */
  mxml_t *SDE;                  /* xml to send (alloc,optional) */

  char *SData;                  /* text to send (alloc,optional) */
  char *SBuffer;                /* text to send - full (w/header) */

  mbool_t IsLoaded;             /* socket data read from sd and RBuffer is populated */
  mbool_t IsNonBlocking;        /* socket requires immediate response/process later */
  mbool_t SBIsDynamic;          /* send buffer is dynamically allocated */

  long  SBufSize;               /* length of SBuffer */
  char *SPtr;                   /* pointer to beginning of data message in SBuffer */

  mulong State;                 /* socket's state (may be unused by Moab) */

  mulong CreateTime;            /* time socket was created (in milliseconds) */
                                /* for calculating service time) @see S->ProcessTime */

  mulong ProcessTime;           /* time request processing began (in milliseconds) */

  mulong SendTime;              /* time response send began (in milliseconds) */

  mulong RequestTime;           /* time (epoch) when request was submitted */


  void *Data;                   /* misc. data (ex: used for JobID in non-blocking submission) (alloc,optional) */

  /* triggered when a non-blocking response returns from peer */

  int (*ResponseHandler)(struct msocket_t *,void *,void *,void *,void *);
  void *ResponseContextList[MMAX_RESPONSEDATA]; /* data to be manipulated by response handler */

  /* comm config */

  enum MWireProtocolEnum   WireProtocol;
  enum MSocketProtocolEnum SocketProtocol;
  enum MChecksumAlgoEnum   CSAlgo;

  mbool_t                  DoEncrypt;
  mbool_t                  Accepted;
  mbool_t                  Processed;
  mbool_t                  TimeStampIsOptional;

  char  CSKey[MMAX_LINE + 1];

  void *P;                    /* peer (mpsi_t) associated with this socket (if any) */
  } msocket_t;

  typedef struct msocket_request_t {
    msocket_t     *S;
    long           CIndex;  /**< command associated with socket request */
    mbitmap_t      AFlags;
    name_t         Auth;
    } msocket_request_t;

typedef void (* mthread_handler_t)(void *);
typedef int (*msuhandler_f)(msocket_t *);
typedef void (*mtpdestructor_t)(void *);
typedef int (*mfree_t)(void **);

/* NOTE:  sync with enum MRMResListType */

/* NOTE:  sync with MRMResList[] */

enum MRMResListType{
  mrmrNONE = 0,
  mrmrArch,
  mrmrCput,
  mrmrFile,
  mrmrFlags,
  mrmrHosts,
  mrmrMem,
  mrmrNCPUs,
  mrmrNice,     /* NYI */
  mrmrNodes,
  mrmrProcs,
  mrmrOpsys,
  mrmrOS,
  mrmrPcput,    /* NYI */
  mrmrPlacement,
  mrmrPmem,
  mrmrPvmem,    /* NYI */
  mrmrSoftware,
  mrmrTrl,
  mrmrVmem,
  mrmrWalltime,
  mrmrVMUsagePolicy,
  mrmrLAST };


/* sync w/MAMFuncType[] */

enum MAMFuncTypeEnum {
  mamNONE = 0,
  mamJobAllocReserve,
  mamJobAllocDebit,
  mamRsvAllocReserve,
  mamRsvAllocDebit,
  mamUserDefaultQuery,
  mamAMSync,
  mamLAST };


/* sync w/MAMNativeFuncType[] */

enum MAMNativeFuncTypeEnum {
  mamnNONE,
  mamnCreate,
  mamnDelete,
  mamnEnd,
  mamnModify,
  mamnPause,
  mamnQuery,
  mamnQuote,
  mamnResume,
  mamnStart,
  mamnUpdate,
  mamnLAST };

/**
 * Resource Manager Functions
 *
 * @see MRMFuncType[] (sync)
 * @see mrmfunc_t (sync)
 * @see mrm_t.ND->URL[]
 */

enum MRMFuncEnum {
  mrmNONE = 0,
  mrmClusterQuery,
  mrmCredCtl,
  mrmInfoQuery,        /* generic information query of non-resource objects */
  mrmJobCancel,
  mrmJobCheckpoint,
  mrmJobGetProximateTasks,
  mrmJobMigrate,
  mrmJobModify,
  mrmJobRequeue,
  mrmJobResume,
  mrmJobSetAttr,
  mrmJobStart,
  mrmJobSubmit,
  mrmJobSuspend,
  mrmNodeModify,
  mrmPing,
  mrmQueueCreate,
  mrmQueueModify,
  mrmQueueQuery,
  mrmReset,
  mrmResourceModify,
  mrmResourceQuery,
  mrmRsvCtl,
  mrmRMEventQuery,
  mrmRMInitialize,
  mrmRMQuery,
  mrmRMStart,
  mrmRMStop,
  mrmSystemModify,   /* generic resource object modification */
  mrmSystemQuery,    /* generic resource object query */
  mrmWorkloadQuery,
  /* index 32 - enum's after here cannot be specified via bitmap */
  mrmXAlloc,
  mrmXCredCtl,
  mrmXCycleFinalize,
  mrmXDataIsLoaded,
  mrmXGetData,
  mrmXJobSignal,
  mrmXJobValidate,
  mrmXNodeMigrate,
  mrmXNodePower,
  mrmXNodeVirtualize,
  mrmXParCreate,
  mrmXParDestroy,
  mrmResourceCreate,
  mrmXRMFailure,     /* responds to a RM failure (used in MRMSetFailure()) */
  mrmLAST };


#define MDEF_RMFUNCSIZE MBMSIZE(mrmLAST)

#define MCONST_MRMALL       0xffffffff

#define MMAX_RMSUBTYPE      2
#define MMAX_PSFAILURE      8

/* MAX of RM and AM function types (sync w/enum MRMFuncEnum) */
/* NOTE:  rm has significantly more functions, use mrmLAST, not mamLAST */


#define MMAX_PSFUNC        mrmLAST


/* peer service interface, sync w/mpsi_t */

enum MPSIEnum {
  mpsiaNONE = 0,
  mpsiaAvgResponseTime,
  mpsiaMaxResponseTime,
  mpsiaTotalRequests,
  mpsiaRespFailCount,
  mpsiaLAST };


/* peer service function */

enum MPSFEnum {
  mpsfaNONE = 0,
  mpsfaFailMsg,
  mpsfaFailTime,
  mpsfaFailType,
  mpsfaFailCount,
  mpsfaTotalCount,
  mpsfaLAST };


/**
 * Peer Service Interface
 *
 * @see enum MPSIEnum
 * @see struct mrm_t.P[]
 * @see struct msocket_t.P
 */

typedef struct mpsi_t {
  char Name[MMAX_NAME];
  char Label[MMAX_NAME];   /* generic interface label */

  /* interface configuration */

  enum MPeerServiceEnum    Type;

  enum MWireProtocolEnum   WireProtocol;
  enum MSocketProtocolEnum SocketProtocol;

  mbool_t DoEncryption;
  mbool_t DropBadRequest;  /* drop connection if corrupt or non-authorized request received */
  mbool_t TimeStampIsOptional; /**< timestamp not required in response */
  mbool_t IsBogus;         /* peer service has been confirmed to be invalid */
  mbool_t IsNonBlocking;   /* peer request uses delayed (non-blocking) responses */

  char   *Version;         /* (alloc) */

  /* security */

  struct mrm_t   *R;               /* pointer to parent RM (pointer only - do not free) */

  enum MChecksumAlgoEnum   CSAlgo;

  enum MRoleEnum RIndex;

  char   *CSKey;           /* shared secret key (alloc) */

  char   *DstAuthID;       /* used for cred services (Globus) that require dest
                              subject/id is known before handshake (alloc) */

  char   *RID;             /* requestor id (alloc) */

  char   *HostName;        /* remote server hostname (alloc) */
  char   *IPAddr;          /* remote server IP addr (alloc)  */
  int     Port;

  char   *Path;            /* access path (alloc) */

  mulong  Timeout;         /* in sec - FIXME migrate to micro-seconds throughout code! */
                           /* NOTE:  native RM's use this field as a micro-second field */

  /* availability information */

  long   FailTime[MMAX_PSFAILURE];
  int    FailType[MMAX_PSFAILURE]; /* array of enum XXX */

  char  *FailMsg[MMAX_PSFAILURE];  /* (alloc) */
  int    FailIndex;

  /* performance information */

  long   RespTotalTime[MMAX_PSFUNC];
  long   RespMaxTime[MMAX_PSFUNC];
  int    RespTotalCount[MMAX_PSFUNC];
  int    RespFailCount[MMAX_PSFUNC];
  long   RespStartTime[MMAX_PSFUNC];

  const void  *Data;  /* (alloc) */

  msocket_t *S; /* (alloc) */

  struct mpsi_t *FallBackP;  /* pointer to this peer's fallback peer */
  } mpsi_t;


/* policy consumption struct */

typedef struct mpc_t {

  long Job;
  long Node;
  long PE;
  long Proc;
  long PS;
  long WC;
  long Mem;
  long ArrayJob;
  
  msnl_t GRes;
  } mpc_t;

/* generic policy usage struct */

typedef struct mgpu_t {
  mulong Usage[MMAX_PAR];
  long   SLimit[MMAX_PAR];
  long   HLimit[MMAX_PAR];

  void  *Ptr;
  } mgpu_t;


/* defined policy usage struct */

typedef struct mpu_t {
  mulong Usage[mptLAST][MMAX_PAR];
  long SLimit[mptLAST][MMAX_PAR];
  long HLimit[mptLAST][MMAX_PAR];

  mhash_t *GLimit;

  void *Ptr;  /* used when mpu_t is part of a hash */
  } mpu_t;


typedef struct mnode_limits_t {
  mulong Usage[mptLAST];
  long HLimit[mptLAST];
  } mnode_limits_t;

/* sync func w/MAdminProcessConfig() */

#define MBMALL  0xffffffff
#define MBMNONE 0x0

typedef struct muis_t {
  const char *SName;                                   /* (alloc) */
  int (*Func)(const char *,char *,mbitmap_t *,char *,long *);
  mbool_t AdminAccess[MMAX_ADMINTYPE + 1];       /* admin types which can access this command */
  } muis_t;


typedef struct muistat_t {
  int    Count;    /* total count */
  int    FCount;   /* failure count */
  mulong QTime;    /* total time requests were queued */
  mulong PTime;    /* total time requests were processed */
  mulong STime;    /* total time request responsed were sent */
  } muistat_t;


#ifndef MMAX_ACL
#define MMAX_ACL  32
#endif /* MMAX_ACL */

/* NOTE:  cred value may be conditional metric for value */

typedef struct macl_t {
  char  *Name;             /* credential name (if applicable) - alloc */

  enum MAttrEnum Type;     /* cred type (one of enum MAttrEnum) */
                           /*  Type == maNONE indicates end of list */

  char   Cmp;              /* type enum MCompEnum */
  char   Affinity;         /* type enum MNodeAffinityTypeEnum */

  long   LVal;             /* credential value/conditional (if applicable) */
  double DVal;             /* credential value/conditional (if applicable) */

  mbitmap_t Flags;         /* (bitmap of enum MACLEnum) */

  void  *OPtr;             /* object pointer */

  struct macl_t *Next; /* pointer to next member of array */
  } macl_t;


#ifndef MMAX_DEPEND
#define MMAX_DEPEND 32
#endif /* MMAX_DEPEND */

#define MDEF_MAXDEPENDS  MMAX_DEPEND

enum MDependBMEnum {
  mdbmNONE = 0,
  mdbmImport,
  mdbmExtSearch,
  mdbmLAST };

 
/**
 * dependency structure (alloc's memory - requires deep copy/free)
 *
 * @see MJobDependDestroy() - frees alloc'd memory
 * 
 * @see mjob_t.Depend,mtrig_t.Depend
 *
 * Whenever alloc'ing a mdepend_t, make sure to set Satisfied to MBNOTSET. 
 */

typedef struct mdepend_s {
  enum MDependEnum    Type;

  int                 Index;

  mbool_t             Satisfied; /* TRUE->Satisfied,FALSE->Unsatisfiable,default to MBNOTSET */

  char               *Value;   /* processed value - Name (alloc) (i.e., jobid) */
  char               *SValue;  /* specified value - Value (alloc) */

  mstring_t          *DepList; /* jobnames of fulfilled dependencies, for on:<count> (alloc) */

  /* NOTE:  move to enum MJobAttrEnum */

  /* NOTE:  JAttr specifies how to interpret 'Value' */

  int                 JAttr;   /* default is mjaName (support mjaJobName) */

  int                 ICount;  /* number of dependencies which must be satisfied */

  long                Offset;  /* relative offset to depend event */

  mbitmap_t           BM;      /**< bitmap of object specified attributes 
                                    (see enum MTrigVarAttrEnum) */

  struct mdepend_s   *NextAnd; /* ptr */
  struct mdepend_s   *NextOr;  /* ptr */
  } mdepend_t;



typedef struct mrange_t {
  mulong  STime;    /* start time */
  mulong  ETime;    /* end time */
  int     TC;       /* task count */
  int     NC;       /* node count */

  char    Aff;      /* affinity */

  mulong  Cost;
  mulong  Duration; /* range duration - for floating ranges (optional) */

  void   *R;        /* owner resource */
  void   *HostList; /* component resources (alloc) */
  } mrange_t;

typedef struct mfcache_t {
  char    FileName[MMAX_PATH_LEN];
  mulong  ProgMTime;
  mulong  FileMTime;
  char   *Buffer;     /* (alloc) */
  int     BufSize;
  } mfcache_t;

typedef struct mfindex_t {
  char    FileName[MMAX_PATH_LEN];
  int     Index;
  } mfindex_t;

typedef struct mrclass_t {
  enum MResourceSetTypeEnum Attr;  /* configured attribute */

  int           NC;

  unsigned long PS;
  unsigned long IPS;               /* initial proc-seconds */
  } mrclass_t;

/* AM Interface */

typedef struct mamolist_t {
  const char *Attr;
  char        Value[MMAX_LINE];
  char       *Cmp;
  } mamolist_t;


/* net names (sync w/MNetName) */

enum MNetNameEnum {
  mnnNONE = 0,
  mnnEthernet,
  mnnHSShared,
  mnnHSDedicated,
  mnnLAST };


/* net interface types */

enum MNetTypeEnum {
  mnetNONE = 0,
  mnetEN0,
  mnetEN1,
  mnetCSS0 };

#define MMAX_NETTYPE          4

#define MMAX_RACKCOUNT      200  /* maximum rack index */

#ifndef MMAX_RACKSIZE
#define MMAX_RACKSIZE       100  /* maximum slots/nodes per rack */
#endif /* !MMAX_RACKSIZE */

#ifndef MCONST_DEFRACKSIZE
#define MCONST_DEFRACKSIZE   16
#endif /* !MMAX_DEFRACKSIZE */

#ifndef MMAX_RACK
#define MMAX_RACK           400  /* maximum number of racks */
#endif /* !MMAX_RACK */

typedef struct mhost_t {
  long     MTime;

  char     RMName[MMAX_NAME + 1];
  mbool_t  NetNameSet; /* Set to TRUE if a name is ever put in NetName (at least 1 non-NULL entry) */
  char    *NetName[MMAX_NETTYPE];  /* (alloc) */
  mulong   NetAddr[MMAX_NETTYPE];

  void    *N;  /* (pointer) */

  int      State;
  mbitmap_t Attributes;
  int      SlotsUsed;
  mbitmap_t Failures;
  } mhost_t;

typedef mhost_t msys_t[MMAX_RACK + 1][MMAX_RACKSIZE + 1];

/**
 *  general job flags (sync w/MJobFlags[])
 *
 * @see enum MJobIFlagEnum - internal flags
 * @see mjob_t.Flags
 */

enum MJobFlagEnum {
  mjfNONE = 0,
  mjfBackfill,
  mjfCoAlloc,         /**< Job can use resources from multiple RMs and partitions */
  mjfAdvRsv,          /**< job requires reservation */
  mjfNoQueue,         /**< execute job immediately or fail */
  mjfArrayJob,        /**< job will share 'reserved' resources */
  mjfArrayJobParLock, /**< array job will only run one partition */
  mjfArrayJobParSpan, /**< array job will span partitions (default) */
  mjfArrayMaster,     /**< job is master of job array */
  mjfBestEffort,      /**< succeed even if only partial resources available */
  mjfRestartable,
  mjfSuspendable,
  mjfPreempted,       /**< job preempted other jobs to start */
  mjfPreemptee,
  mjfPreemptor,
  mjfRsvMap,          /**< job is reservation pseudo-job */
  mjfSPViolation,     /**< job was started with soft policy violation */
  mjfIgnNodePolicies,
  mjfIgnPolicies,     /**< ignore idle, active, class, partition, and system policies */
  mjfIgnState,
  mjfIgnIdleJobRsv,   /**< job can ignore idle job reservations - job granted access to all idle job rsvs */
  mjfInteractive,
  mjfFSViolation,     /**< job was started with fairshare violation */
  mjfGlobalQueue,     /**< job is directly submitted (unauthenticated) */
  mjfNoResources,     /**< system job - requires no compute/generic/virtual resources */
  mjfNoRMStart,       /**< system job - do not start RM job (24) */
  mjfClusterLock,     /**< job is locked into current cluster (cannot be migrated out) */
  mjfFragment,        /**< job can run fragmented (individual jobs on each node) */
  mjfSystemJob,       /**< system job */
  mjfAdminSetIgnPolicies,  /**< job ignpolicies flag set by Admin (30) */
  mjfExtendStartWallTime,  /**< job walltime extended at job start. */
  mjfSharedMem,       /**< job will share its memory across nodes (NUMA) */
  mjfBlockedByGRes,   /**< job's gres requirement caused the job to start later */
  mjfGResOnly,        /**< job is only requesting GRes, no compute resources */
  mjfTemplatesApplied,/**< job has had all applicable templates applied to it */
  mjfMeta,            /**< META job, just a container around resources */
  mjfWideRsvSearchAlgo, /**< this job prefers the wide search algo in MJobGetSNRange */
  mjfVMTracking,      /**< job is a VMTracking job for an externally created VM (via job template, NOT mvmctl) */
  mjfDestroyTemplateSubmitted, /**< a destroy job has already been created from the template for this job */
  mjfProcsSpecified,  /**< job requested procs on cmd line (ex. -l procs) */
  mjfCancelOnFirstFailure, /**< cancel job array on first array job failure */
  mjfCancelOnFirstSuccess, /**< cancel job array on first array job success */
  mjfCancelOnAnyFailure, /**< cancel job array on any array job failure */
  mjfCancelOnAnySuccess, /**< cancel job array on any array job success */
  mjfCancelOnExitCode,  /**< cancel job array on a specific exit code */
  mjfLAST };


#define MDEF_JOBFLAGSIZE MBMSIZE(mjfLAST)



/* defines a request to create a virtual machine */

typedef struct mvm_req_create_t {
  mcres_t        CRes;
  int            OSIndex;
  char           VMID[MMAX_NAME];
  char           HVName[MMAX_NAME];  /* Name of HV this VM is to be created on */
  struct mjob_t *JT;
  struct mjob_t *OwnerJob;
  mbool_t        IsSovereign;
  mbool_t        IsOneTimeUse;
  char           Vars[MMAX_LINE*2];
  char           Storage[MMAX_LINE];   /* Original storage request string */
  mln_t         *StorageRsvs;          /* linked list of storage rsvs */
  char           Aliases[MMAX_LINE];   /* Network aliases */
  char           Triggers[MMAX_LINE*2];/* Triggers */
  long           Walltime;             /* Requested walltime for the VM */
  mnode_t       *N;                    /* hypervisor */
  char           TrackingJ[MMAX_NAME]; /* VMTracking job ID */
  char           VC[MMAX_NAME];        /* VC to submit the VM to (optional) */
  mbitmap_t      FBM; /*required feature (BM)  */
  struct mgcred_t *U;                  /* user who requested the VM */
  } mvm_req_create_t;

/* default VMTracking time is INFINITY (MMAX_TIME) */
#define MDEF_VMTTTL MMAX_TIME

/* defines a request to power off a node */

/* useful for sorting arrays and such */

typedef struct mnalloc_old_t {
  mnode_t *N;
  int      TC;
  } mnalloc_old_t ;


typedef struct mnl_t {
  mnalloc_old_t *Array;

  int Size;
  } mnl_t;


typedef struct mpoweroff_req_t {
  mnl_t    NodeList;       /* type mnl_t List of nodes to be powered off */
  marray_t Dependencies;   /* array of type mln_t *[]. List of list of job 
                              dependencies that must be completed before the 
                              node can be powered off. */
  mbool_t  ForceSpecified; /* whether or not to force the request by allowing 
                              the poweroff jobs to work on top of other 
                              reservations */
  } mpoweroff_req_t;


 /* Buffered-I/O structure (similar to RIO structure found in
  * "Computer Systems: A Programmer's Prospective" */
  
typedef struct mbuffd_t {
 
  int   fd;              /* file descriptor */
  int UnreadCount;       /* unread bytes in buffer */
  char *BufPtr;          /* next unread byte in buffer */
  char  Buf[MMAX_LINE];  /* internal buffer */
  mulong ReadCount;      /* how many bytes have been read from buffer */
  } mbuffd_t;

/* doubly-linked list */

struct mdllist_node_t;

typedef struct mdllist_node_t {
  void *Data;
  struct mdllist_node_t *Prev;
  struct mdllist_node_t *Next;
  } mdlnode_t;

typedef struct mdllist_t {
  int Size;
  mdlnode_t *Head;
  mdlnode_t *Tail;
  } mdllist_t;


/* timeline */

typedef struct mtl_t {
  mulong Time;  /* event time (list terminated w/MMAX_TIME) */
  int    X1;    /* data value 1 */
  int    X2;    /* data value 2 */
  } mtl_t;



/* mid-level objects */

/* per-cred limits */

#define MMAX_QDEF 4

typedef struct mcredl_t {
  mpu_t   ActivePolicy;               /* active inter-job usage limits */
  mpu_t  *IdlePolicy;                 /* idle inter-job usage limits (alloc) */
  mpu_t  *JobPolicy;                  /* intra-job resource usage limits (alloc) */

  mpu_t  *OverrideActivePolicy;       /* override active usage policies (alloc)  */
  mpu_t  *OverrideIdlePolicy;         /* override idle usage policies (alloc) */
  mpu_t  *OverriceJobPolicy;          /* override intra-job resource usage limits (alloc) */

  mhash_t *UserIdlePolicy;            /* idle policy (per user) (alloc) */
  mhash_t *AcctIdlePolicy;            /* idle policy (per acct) (alloc) */
  mhash_t *QOSIdlePolicy;             /* idle policy (per qos) (alloc) */
  mhash_t *GroupIdlePolicy;           /* idle policy (per qos) (alloc) */
  mhash_t *ClassIdlePolicy;           /* idle policy (per qos) (alloc) */

  mhash_t *ClassActivePolicy;         /* active policy (per class) (alloc) */
  mhash_t *QOSActivePolicy;           /* active policy (per QOS) (alloc) */
  mhash_t *AcctActivePolicy;          /* active policy (per acct) (alloc) */
  mhash_t *GroupActivePolicy;         /* active policy (per group) (alloc) */
  mhash_t *UserActivePolicy;          /* active policy (per user) (alloc) */

  mpu_t  *RsvPolicy;                  /* intra-rsv resource usage limits (alloc) */
  mpu_t  *RsvActivePolicy;            /* rsv active policy (alloc) */

  /* for all job pointers, alloc for class, ptr for RM */

  struct mjob_t   *JobDefaults;               /* job defaults */
  struct mjob_t   *JobMaximums[MMAX_PAR + 1]; /* job maximums */
  struct mjob_t   *JobMinimums[MMAX_PAR + 1]; /* job minimums */
  struct mjob_t   *JobSet;                    /* forced job attributes */

  mbitmap_t  OverrideActiveCredBM;               /* override active policy list (bitmap of mxo*) */

  unsigned int TotalJobs[MMAX_PAR + 1];      /* total number of jobs with this credential */
  unsigned int MaxSubmitJobs[MMAX_PAR + 1];  /* max jobs allowed in the scheduler for this credential */
  } mcredl_t;


enum MFSFlagEnum {
  mffNONE = 0,
  mffIsLocalPriority, /* priority is locally specified in Moab config, not RM */
  mffQDefSpecified,   /* QOS default explicitly specified */
  mffLAST };


#define MMAX_CREDMANAGER 4

/* per cred fairness structure */

typedef struct mfs_t {
  mbitmap_t JobFlags;               /* default job flags associated w/cred    */
                                            /* (bitmap of enum MJobFlagEnum)*/

  long   Priority;                  /* admin specified credential priority    */
  mbool_t IsInitialized;

  mbitmap_t  Flags;                 /* bitmap of enum MFSFlagEnum             */
  mulong  Overrun;                  /* duration job may overrun wclimit (in seconds) */

  struct mgcred_t *GDef;            /* default group                          */
  mln_t           *GAL;             /* group access list                      */

  struct mgcred_t *ADef;            /* default account                        */
  mln_t           *AAL;             /* account access list (ptr is NOT alloc) */

  struct mclass_t  *CDef;           /* default class (ptr)                    */
  mln_t            *CAL;            /* class access list (not yet supported)  */

  struct mqos_t   *QDef[MMAX_QDEF]; /* QOS precedence list                    */
  mbitmap_t        QAL;             /* QOS access list                        */
  enum MALTypeEnum QALType;

  struct mpar_t *PDef;              /* default partition                     */
  mbitmap_t      PAL;
  mbool_t        PDefSet;           /* default partition explicitly set      */
  enum MALTypeEnum PALType;

  double FSTarget;                  /* credential fs usage target             */
  enum MFSTargetEnum FSTargetMode;  /* FS target type                         */

  double FSCap;
  enum MFSTargetEnum FSCapMode;     /* FS cap type                            */

  double GFSTarget;                 /* global FS target                       */
  double GFSUsage;                  /* global FS usage (imported)             */

  double FSFactor;                  /* effective decayed FS factor            */
  double FSUsage[MMAX_FSDEPTH];     /* FS usage history                       */

#ifdef __MFSOLDTREE
  double PFSFactor[MMAX_PAR];       /* partition usage history */
  double PFSUsage[MMAX_FSDEPTH][MMAX_PAR];    /* effective decayed partition FS factor */
#endif

#ifdef __MFSOLDTREE
  double PFSTreeFactor[MMAX_PAR];   /* per-partition effective FS Tree priority */
#endif
  double FSTreeFactor;              /* effective FS Tree priority             */
  void  *FSTree;                    /* (pointer only, points to parent tnode) */

  char  *ReqRsv;                    /* required reservation (alloc)           */

  struct mgcred_t *ManagerU[MMAX_CREDMANAGER]; /* cred manager user pointer */

  /* should we add mulong ExemptList here? */
  } mfs_t;

#define MDSTAT_STEPSIZE  64

typedef struct mdstat_t {
  char *Data;
  int   DSize;
  int   Count;
  int   Size;
  } mdstat_t;


/* sync w/MStatObject[]) */

enum MStatObjectEnum {
  msoNONE = 0,
  msoCred,
  msoNode,
  msoLAST };


/* stat types (sync w/MStatType[]) */

enum MMStatTypeEnum {
  mgstNONE = 0,
  mgstAvgXFactor,
  mgstMaxXFactor,
  mgstAvgQTime,
  mgstAvgBypass,
  mgstMaxBypass,
  mgstJobCount,
  mgstPSRequest,
  mgstPSRun,
  mgstWCAccuracy,
  mgstBFCount,
  mgstBFPSRun,
  mgstQOSDelivered,
  mgstEstStartTime,
  mgstLAST };


/* statistics modes (sync w/NONE) */

enum MProfModeEnum {
  mNONE = 0,
  mUseRunTime,
  mSystemQueue,
  mMatrix,
  mTrueXFactor,
  mUseRemoved,
  mXML,
  mPeer,
  mLAST };


/* sync w/MTrigThresholdType[] */

enum MTrigThresholdTypeEnum {
  mtttNONE = 0,
  mtttAvailability,
  mtttBacklog,
  mtttCPULoad,
  mtttGMetric,
  mtttQueueTime,
  mtttStatistic,
  mtttUsage,
  mtttXFactor,
  mtttLAST };


/* trigger event types */

/**
 * This enum contains the list of possible events that will cause a trigger to activate.
 * Must be synced with MTrigType[] in MConst.c
 *
 */
enum MTrigTypeEnum {
  mttNONE = 0,
  mttCancel,
  mttCheckpoint,
  mttCreate,
  mttDiscover,
  mttEnd,
  mttEpoch,
  mttFailure,
  mttHold,
  mttLogRoll,
  mttMigrate,
  mttModify,
  mttPreempt,   /**< Occurs on a "preempt" event. For example, when a job is preempted*/
  mttStanding,  /**< Occurs on a "start" event. A job starting is an example of when this event occurs. */
  mttStart,
  mttThreshold,
  mttLAST };



/**
 * Trigger state (sync w/MTrigState[])
 *
 * @see struct mtrig_t.State
 */

enum MTrigStateEnum {
  mtsNONE = 0,
  mtsActive,  /* trigger was launched and is now actively running */
  mtsLaunched, /* trigger has been successfully launched, but is not yet considered running */
  mtsSuccessful, /* trigger has successfully completed */
  mtsFailure,    /* trigger failed to properly complete */
  mtsLAST };


#define MTRIGISRUNNING(x) (((x)->State == mtsActive) || ((x)->State == mtsLaunched))



/**
 * Tells Moab what to do when the trigger event has occurred.
 *
 * Must be synced with MTrigActionType[] in MConst.c
 */

enum MTrigActionTypeEnum {
  mtatNONE = 0,
  mtatCancel,
  mtatChangeParam, /**< run changeparam (NOT PERSISTENT) */
  mtatExec,        /**< Execute the trigger action. Typically used to run a script.*/
  mtatInternal,    /**< Modifies an object internally in Moab. This can be used to set a job hold for example.*/
  mtatJobPreempt,  /**< preempt jobs utilizing allocated resources */
  mtatMail,        /**< Sends an e-mail. */
  mtatModify,      /**< Can modify object that trigger is attached to. */
  mtatQuery,
  mtatReserve,
  mtatSubmit,
  mtatLAST };


/** @see designdoc General Credential Stats */

/* NOTE: verify stats w/'mcredctl -q profile --format=xml <OTYPE>[:<OID>] [-o time:<START>,<END>]' */

typedef struct must_t {
  int   Index;

  enum MXMLOTypeEnum OType;         /* object type with which stats are associated */

  /* time attributes of statistics */

  long  StartTime;                  /* time stat profile started */
  long  Duration;
  int   IterationCount;

  /* workload statistics */

  /* jobs */

  int   JC;                         /* total jobs in grid                            */
  int   SubmitJC;                   /* jobs submitted - updated at job load          */
  int   ISubmitJC;                  /* jobs submitted this iteration                 */
  int   SuccessJC;                  /* jobs successfully completed                   */
  int   RejectJC;                   /* jobs submitted that have been rejected        */
  int   QOSSatJC;                   /* jobs that met requested QOS                   */
  int   JAllocNC;                   /* total nodes allocated to jobs                 */
  int   ActiveNC;                   /* total active nodes allocated to job           */
  int   ActivePC;                   /* total active processors allocated to job      */

  mulong TotalQTS;                  /* job queue time                                */
  mulong MaxQTS;

  /* queued workload */

  int    TQueuedJC;   /* updated each iteration */

  double AvgXFactor;  /* total avg xfactor since submission - updated at job complete */
  double AvgQTime;    /* total avg queue time since submission (in unit X?) - updated at job complete */
  int    IICount;     /* idle iteration count */

  /* resource statistics */

  int    TPC;
  int    TNC;

  /* ps */

  double PSRequest;                 /* requested procseconds - updated at job complete */
  double SubmitPHRequest;           /* requested prochours submitted - updated at job load (see SubmitJC) */
  double PSRun;                     /* executed procsecond - updated at job end */
  double PSRunSuccessful;           /* successful procseconds - updated at job complete */

  /* utilization statistics */

  double TQueuedPH;
  double TotalRequestTime;          /* total requested job walltime - updated on job complete, ckpt? */
  double TotalRunTime;              /* total execution job walltime - updated on job complete, ckpt? */

  /* iteration-level statistics */

  double PSDedicated;               /* procseconds dedicated - updated per-iteration for active jobs */
  double PSUtilized;                /* procseconds utilized - updated per-iteration for active jobs */

  double DSAvail;                   /* disk-seconds available                        */
  double DSUtilized;
  /* double DSDedicated; */         /* disk-seconds dedicated                        */
  double IDSUtilized;               /* disk-seconds (instant) utilized               */

  double MSAvail;                   /* memoryseconds available                       */
  double MSUtilized;                /* memoryseconds utilized - updated per-iteration for active jobs (in MB-seconds) */
  double MSDedicated;               /* memoryseconds dedicated - updated per iteration for active jobs (in MB-seconds) */

  double JobAcc;                    /* job accuracy sum - updated on job complete */
  double NJobAcc;                   /* node weighed job accuracy sum - updated on job complete */

  double XFactor;                   /* xfactor sum - updated on job complete         */
  double NXFactor;                  /* node-weighed xfactor sum - updated on job complete */
  double PSXFactor;                 /* ps-weighted xfactor sum - updated on job complete */
  double MaxXFactor;                /* max xfactor detected - updated on job complete */

  int    Bypass;                    /* number of times job was bypassed - updated on job complete */
  int    MaxBypass;                 /* max number of times job was bypassed - updated on job complete */

  int    BFCount;                   /* number of jobs backfilled - updated on job complete */
  double BFPSRun;                   /* procseconds backfilled - updated on job complete */

  double QOSCredits;                /* number of credits used - updated on job complete */

  int    Accuracy[MMAX_ACCURACY];   /* wallclock accuracy distribution */

  /* job start stats */

  int    StartJC;                   /* number of jobs started - updated on job start */
  int    StartPC;                   /* proc-weights job started - updated on job start */
  double StartXF;                   /* XFactor at job start - updated on job start */
  long   StartQT;                   /* queue time at job start - update on job start (in seconds) */

  long   TCapacity;                 /* accessible capacity (in procs) - updated per iteration */

  double TNL;                       /* total node load on all dedicatd nodes (only accurate if DEDICATED) */
  double TNM;                       /* total memory usage on all dedicated nodes (only accurate if DEDICATED) */

  /* profile attributes */

  mbool_t IsProfile;                /* stat instance is profile interval */

  int    IStatCount;                /* for parent, number of active profile stats */
                                    /* for first IStat, maximum number of profile stats */

  mbool_t Verified;                 /* NOT USED */

  double *XLoad;                    /* generic metrics stats */

  struct must_t **IStat;            /* (alloc) */
  } must_t;



/**
 * used in showstats command.
 */

typedef struct {
    char Name[MMAX_NAME];
    int  Index;

    must_t   S;
    mfs_t    F;
    mcredl_t L;
  } mshowstatobject_t;



/* stats for triggers */

typedef struct mtrigstats_t {

  int NumAllTrigsEvaluatedThisIteration;  /* number of times all triggers were evaluated this iteration */
  int NumTrigStartsThisIteration;  /* number of triggers launched last iteration */
  int NumTrigEvalsThisIteration;   /* number of triggers evaluated last iteration */
  int NumTrigChecksThisIteration;  /* number of times we entered trigger processing code last iteration (MSchedCheckTriggers) */
  int NumTrigCheckInterruptsThisIteration;   /* number of times TrigCheckTime was violated in MSchedCheckTriggers */

  long TimeSpentEvaluatingTriggersLastIteration;  /* time spent (ms) evaluating triggers last iteration */
  int NumAllTrigsEvaluatedLastIteration;  /* number of times all triggers were evaluated last iteration */
  int NumTrigStartsLastIteration;  /* number of triggers launched last iteration */
  int NumTrigEvalsLastIteration;   /* number of triggers evaluated last iteration */
  int NumTrigChecksLastIteration;  /* number of times we entered trigger processing code last iteration (MSchedCheckTriggers) */
  int NumTrigCheckInterruptsLastIteration;   /* number of times TrigCheckTime was violated in MSchedCheckTriggers */

  } mtrigstats_t;



#define MMAX_BCAT 16  /* with 15 minute minimum size, provide for up to 3 years of information */

/* node utilization state */

enum MTBFEnum {
  mtbf15m,
  mtbf30m,
  mtbf1h,
  mtbf2h,
  mtbf4h,
  mtbf8h,
  mtbf16h,
  mtbf32h,
  mtbf64h,
  mtbf128h,
  mtbf256h,
  mtbf512h,
  mtbf1024h,
  mtbf2048h,
  mtbf4096h,
  mtbf8192h,   /* ~ 1 year */
  mtbfLAST };


/**
 * mnust_t structure
 *
 * node utilization stat structure
 *
 * @see struct must_t - peer - cred stats structure
 * @see enum MNodeStatTypeEnum (sync)
 *
 * NOTE: for profile support sync new attributes with
 * - MStatToXML() - DAList[] w/in mxoNode block (enables checkpointing)
 */

typedef struct mnust_t {
  int   Index;

  enum MXMLOTypeEnum OType;   /* object type with which stats are associated */

  /* time attributes of statistics */

  mulong  StartTime;
  long  Duration;
  int   IterationCount;

  /* Node's categorization precedence is
      N->SCat   - per node system categorization
      R->RType  - rsv based system categorization
      N->RCat   - node categorization reported by RM
      node failures
      node usage
  */

  enum MNodeCatEnum SCat;     /* system specifed categorization */
  enum MNodeCatEnum RCat;     /* node state category specified by external system */
  enum MNodeCatEnum Cat;      /* node state category */

  int  AProc;
  int  CProc;
  int  DProc;

  enum MNodeStateEnum NState;

  double CPULoad;
  double MaxCPULoad;
  double MinCPULoad;

  int  UMem;
  int  MaxMem;
  int  MinMem;

  int  StartJC;               /* job starts since "StartTime" */
  int  FailureJC;             /* job failures since "StartTime" */

  mulong Queuetime;

  double XFactor;

  msnl_t  CGRes;
  msnl_t  AGRes;
  msnl_t  DGRes;

  double *XLoad;

  /* profile attributes */

  mbool_t IsProfile;          /* stat instance is profile interval */

  int    IStatCount;          /* for parent, number of active profile stats */
                              /* for first IStat, maximum number of profile stats */
  struct mnust_t **IStat;

  mulong FCurrentTime;        /* time represented by FCount[0] */
  int    FCount[MMAX_BCAT];   /* current binary accounting representation of failures */
  int    OFCount[MMAX_BCAT];  /* previous binary accounting representation of failures */

  int    SuccessiveJobFailures; /* number of successive job failures, will reset with a successful job */
  } mnust_t;



/* Default threshold for the storage node for allocated storage by storage mounts.
   See MVMCheckStorage for implementation details.
 */
#define MDEFAULT_STORAGE_NODE_THRESHOLD 80


/**
 * mnapp_t is used by Moab to "learn" which nodes are best for running jobs
 * with specific applications 
 *
 * @see must_t.ApplStat
 */

typedef struct mnapp_t {
  /* performance info */

  mulong Sum;        /* sum-total of all samples taken (job-size/runtime = sample) */

  /* failure info */

  int FailCount;     /* number of samples that did not complete successfully */
  int SampleCount;   /* number of samples taken */

  /* power consumption info */

  double WattUsageSum;         /* sum of all watt usage samples on active jobs */
  int    WattUsageSampleCount; /* number of samples of watt usage taken */
  } mnapp_t;

typedef struct mnappb_t {
  mulong UpperBound;  /* the largest job-size/runtime sample seen thus far */
  mulong LowerBound;  /* the smallest job-size/runtime sample seen thus far */
  int SampleCount;    /* global number of samples taken */
  } mnappb_t;


/**
 * comparison types
 *
 * @see MComp[] (sync)
 * @see enum MObjectSetModeEnum - peer for handling object actions
 */

enum MCompEnum {
  mcmpNONE = 0,
  mcmpLT,
  mcmpLE,
  mcmpEQ,
  mcmpGE,
  mcmpGT,
  mcmpNE,
  mcmpEQ2,
  mcmpNE2,
  mcmpSSUB,
  mcmpSNE,
  mcmpSEQ,
  mcmpINCR,
  mcmpDECR,
  mcmpLAST };


/*Database Type. Sync with MDBType*/
typedef enum MDBTypeEnum {
  mdbNONE = 0,
  mdbSQLite3,
  mdbODBC,
  mdbLast
} MDBTypeEnum;

/* depend sets control the number of set and dependency variables per trigger */

#ifndef MMAX_TRIGVARIABLES
#define MMAX_TRIGVARIABLES 256
#endif /* MMAX_TRIGVARIABLES */


/**
 *  Trigger flags. (sync w/MTrigFlag[])
 */

enum MTrigFlagEnum {
  mtfNONE = 0,        /**< A sentinal value*/
  mtfAsynchronous,    /**< asynchronous trigger */
  mtfAttachError,     /**< If the trigger outputs anything to stderr moab will attach this as a message to the trigger object*/
  mtfCleanup,         /**< If the trigger is still running when the parent object completes or is canceled, the trigger will be killed */
  mtfInterval,        /**< Trigger is periodic. Does it imply MultFire???*/
  mtfLeaveFiles,      /**< Do not remove stderr and stdout files in MTrigFree() */
  mtfMultiFire,       /**< Trigger can be fired multiple times.*/
  mtfProbe,           /**< The trigger's stdout will be monitored.  */
  mtfProbeAll,        /**< The trigger's stdout will be monitored.  */
  mtfUser,            /**< the trigger will execute under the User ID of the object's owner. If the parent
                           object is sched, the user to run under may be explicitly specified using the
                           format user+<username>, for example flags=user+john */
  mtfGenericSysJob,   /**< trigger belongs to a generic system job (for checkpointing) */
  mtfGlobalVars,      /**< The trigger will look in the namespace of all nodes with the globalvars flag in
                           addition to its own namespace. A specific node to search can be specified using
                           the following format:  globalvars+node_id */
  mtfGlobalTrig,      /**< The trigger will be/was inserted into the global trigger list. */
  mtfRemoveStdFiles,  /**< The trigger will delete stdout/stderr files after it has been reset. */
  mtfResetOnModify,   /**< Reset trigger if the object is modified (even if multifire=FALSE) */
  mtfCheckpoint,      /**< checkpoint this trigger no matter what */
  mtfObjectXMLStdin,  /**< pass in object XML for the owner object to the stdin of the trigger (currently only rsv type owner) */
  mtfSoftKill,        /**< if TRUE and the trigger times out, it will be sent a kill -15 signal instead of kill -9 */
  mtfLAST };


#define MDEF_TRIGFLAGSIZE MBMSIZE(mtfLAST) 


/* sync w/MTrigVarFlag */

enum MTrigVarAttr {
  mtvaNONE = 0,
  mtvaIncr,
  mtvaExport,
  mtvaNoExport,
  mtvaLAST };

/* sync w/MTrigECPolicy[] */

enum MTrigECPolicyEnum {
  mtecpNONE = 0,
  mtecpIgnore,      /* ignore exitcode value */
  mtecpFailNeg,     /* consider trigger failed if exitcode is negative */
  mtecpFailPos,     /* consider trigger failed if exitcode is positive */
  mtecpFailNonZero, /* consider trigger failed if non-zero exit code returned */
  mtecpLAST };


/* Internal trigger attributes to save on size */
enum MTrigInternalFlagEnum {
  mtifNONE = 0,
  mtifFailureDetected,
  mtifIsComplete,          /* trigger status */
  mtifOIsRemoved,          /* parent object has been removed */
  mtifDisabled,            /* trigger is no longer being evaluated */
  mtifOutstandingTrigDeps, /* trigger has outstanding trigger deps. */
  mtifIsHarvested,         /* trigger's PID has been harvested */
  mtifTimeoutIsRelative,   /* timeout duration is relative to parent object */
  mtifIsTemplate,          /* template trigger -- do not launch or print */
  mtifIsAlloc,             /* trigger was alloced */
  mtifIsInterval,          /* trigger fires on intervals */
  mtifMultiFire,           /* should trigger re-arm? */
  mtifThresholdIsPercent,  /* threshold should be interpreted as a percentage */
  mtifRecorded,            /* trigger was already recorded (currently used for VM Storage creation) */
  mtifLAST };

#define MDEF_TRIGINTERNALFLAGSIZE MBMSIZE(mtifLAST) 

/* NOTE:  all new policy/config attributes should be handled in MTListCopy() */

typedef struct mtrig_t {
  char   *TName;                   /* uniq trigger id (alloc,format=[<NAME>.]<TID>) */
  char   *Name;                    /* non-unique (user specified) trigger name (alloc) */

  char   *OID;                     /* trigger parent oid (alloc) */
  enum MXMLOTypeEnum OType;        /* parent object type */
  void   *O;                       /* object pointer */

  char   *UserName;                /* pointer to username under which to fire the trigger (alloc) (not supported on all objects) */
                                   /* NOTE: this is a "char *" because scheduler triggers are processed before any users are processed */
                                   /*       ie. race condition */

  char   *GetVars;                 /* node name from which to get variables  */

  enum MTrigTypeEnum          EType; /* trigger type */
  enum MTrigActionTypeEnum    AType; /* action type */

  enum MTrigThresholdTypeEnum ThresholdType; /* threshold type */
  enum MCompEnum              ThresholdCmp;  /* threshold comparison type */

  double  Threshold;            /* threshold - current threshold value (what the boundary is) */
  double  ThresholdUsage;       /* value of what the measured metric is at */
  char *ThresholdGMetricType;   /* GMetric type for threshold (alloc) */

  enum MCalPeriodEnum Period;

  /* state */

  enum MTrigStateEnum State;       /* state of trigger */
  enum MTrigStateEnum LastExecStatus; /* state of trigger at completion of last trigger launch */

  mulong CTime;                    /* time trigger was created */
  mulong LaunchTime;               /* time trigger was launched (spawned a child) */
  mulong ETime;                    /* time event occurred or will occur */
  mulong LastETime;                /* event time of previous period */
  mulong FailTime;                 /* time trigger failure occured */
  mulong EndTime;                  /* time trigger completed */

  int    MaxRetryCount;
  int    RetryCount;

  double Status;                   /* percentage complete */

  int    PID;                      /* pid of the process spawned */
  int    StatLoc;                  /* status returned by WEXITSTATUS macro (see man waitpid) */

  int    ExitCode;

  enum   MTrigECPolicyEnum ECPolicy; /* how to process exit codes into success/failure */

  char  *IBuf;                     /* path and filename of trigger input buffer */
  char  *OBuf;                     /* path and filename of trigger output buffer */
  char  *EBuf;                     /* path and filename of trigger error buffer */

  mbool_t IsExtant;                /* this trigger is effectively "dead" and is now just a placeholder (is dead if FALSE) */

  /* config */

  mulong BlockTime;                /* time trigger will block */
  mulong RearmTime;                /* time between multifire launches */
  mulong Timeout;                  /* max duration of trigger execution */
  mulong ExpireTime;               /* epoch time by which trig must launch or be removed */

  long   Offset;                   /* trigger offset */

  mulong FailOffset;               /* fail offset - time threshold must be set before trigger is allowed to fire */

  char   *Action;                  /* action data (alloc) */
  char   *CompleteAction;          /* action data with all variables replaced (alloc) */
  char   *StdIn;                   /* stdin (alloc) */

  char   *PostAction;              /**< action that is to run AFTER all other trigs. are processed in MSchedCheckTriggers() (alloc) */


  /* add dependency checks */

  marray_t Depend;                 /* list variable dependencies (type mdepend_t *) */

  /* the below three variables can be used in conjunction with Depend (above) to increase overall
   * trigger performance when a large amount of dependencies and triggers are present */

  mstring_t  *TrigDepString;       /* trigger dependencies by name in delimited list (alloc) */
  mln_t *TrigDeps;                 /* trigger dependencies in linked list (alloc) */
  mln_t *Dependents;               /* triggers that rely on this trigger to be enabled (alloc) */

  char  *Env;                      /* shell environment (FORMAT:  <ATTR>=<VAL>;[<ATTR>=<VAL>;]...) */

  /* FORMAT of SSets,FSets:  <ATTR>=<VAL> */

  marray_t *SSets;       /* list of vars to set when trigger completes successfully (type char*) (alloc)*/
  marray_t *SSetsFlags;  /* list of BM (enum MTrigVarAttr) (alloc) */
  marray_t *FSets;       /* list of vars to set when trigger fails (type char*) (alloc) */
  marray_t *FSetsFlags;  /* list of BMs (enum MTrigVarAttr) (alloc) */
  marray_t *Unsets;      /* list of vars to unset when the trigger completes successfully (type char*) (alloc) */
  marray_t *FUnsets;     /* list of vars to unset when the trigger fails (type char*) (alloc) */

  mbitmap_t SFlags;          /* specified flags (see MTrigFlagEnum)*/
  mbitmap_t InternalFlags;   /* Internal attribute flags (see MTrigInternalFlagEnum) */

  char   *Msg;                     /* (alloc) */

  char   *Description;             /* description of trigger (alloc) */
  } mtrig_t;




/**< sync w/MGResAttr[] */

enum MGResAttrEnum {
  mgresaNONE = 0,
  mgresaChargeRate,
  mgresaInvertTaskCount,
  mgresaPrivate,
  mgresaStartDelay,
  mgresaType,
  mgresaFeatureGRes,
  mgresaLAST };


/**
 * Cred attributes (sync w/MCredAttr[], mgcred_t)
 *
 * NOTE:  For configurable options, add appropriate entries to
 *  MCredProcessConfig()
 *  M<OBJECT>Show()
 *  MCredSetAttr()
 *  MCredAToString()
 *  MCredToString() - add to CPAList[] to checkpoint attribute
 */

enum MCredAttrEnum {
  mcaNONE,
  mcaCDef,
  mcaChargeRate,
  mcaComment,      /* dup of mcaMessages */
  mcaEMailAddress,
  mcaFlags,
  mcaFSCap,
  mcaFSTarget,
  mcaGFSTarget,
  mcaGFSUsage,
  mcaHold,
  mcaPriority,     /* NOTE:  must be located before first usage limit - see MCredShowAttrs() */
  mcaMaxGRes,
  mcaMaxJob,
  mcaMaxArrayJob,
  mcaMaxNode,
  mcaMaxPE,
  mcaMaxProc,
  mcaMinProc,
  mcaMaxPS,
  mcaMaxWC,        /* max total wclimit */
  mcaMaxMem,
  mcaMaxIGRes,
  mcaMaxIJob,      /* idle/queued total limits */
  mcaMaxIArrayJob,
  mcaMaxINode,
  mcaMaxIPE,
  mcaMaxIProc,
  mcaMaxIPS,
  mcaMaxIWC,
  mcaMaxIMem,
  mcaOMaxJob,      /* override active limits */
  mcaOMaxNode,
  mcaOMaxPE,
  mcaOMaxProc,
  mcaOMaxPS,
  mcaOMaxWC,
  mcaOMaxMem,
  mcaOMaxIJob,      /* override idle limits */
  mcaOMaxINode,
  mcaOMaxIPE,
  mcaOMaxIProc,
  mcaOMaxIPS,
  mcaOMaxIWC,
  mcaOMaxIMem,
  mcaOMaxJNode,     /* override job limits */
  mcaOMaxJPE,
  mcaOMaxJProc,
  mcaOMaxJPS,
  mcaOMaxJWC,
  mcaOMaxJMem,
  mcaReqRID,        /* credential to rsv binding  */
  mcaRMaxDuration,  /* per rsv reservation limits */
  mcaRMaxProc,
  mcaRMaxPS,
  mcaRMaxCount,     /* aggregate reservation limits */
  mcaRMaxTotalDuration,
  mcaRMaxTotalProc,
  mcaRMaxTotalPS,
  mcaPDef,
  mcaPList,
  mcaQDef,
  mcaQList,
  mcaRole,
  mcaAList,
  mcaADef,
  mcaJobFlags,
  mcaMaxJobPerUser,
  mcaMaxJobPerGroup,
  mcaMaxNodePerUser,
  mcaMaxProcPerUser,
  mcaMaxProcPerNodePerQueue,
  mcaOverrun,         /* amount of time job may exceed its specified wclimit */
  mcaID,              /* credential id/name */
  mcaIndex,
  mcaDefTPN,
  mcaDefWCLimit,
  mcaMinWCLimitPerJob,
  mcaMaxWCLimitPerJob,      /* max wclimit per job */
  mcaMaxProcPerNode,
  mcaMinNodePerJob,
  mcaMaxNodePerJob,
  mcaMaxProcPerJob,
  mcaMemberUList,     /* list of users which are members of this cred */
  mcaNoSubmit,        /* don't allow job submission to this credential */
  mcaCSpecific,
  mcaPref,
  mcaEnableProfiling,
  mcaFSCWeight,
  mcaManagers,        /* list of users allowed to control account objects */
  mcaTrigger,         /* trigger list */
  mcaMessages,        /* credential message */
  mcaIsDeleted,
  mcaMaxSubmitJobs,   /* max queued jobs for the credential - checked at job submission */
  mcaVariables,
  mcaRsvCreationCost, /* Reservation creation cost per node */
  mcaMaxGMetric,
  mcaLAST };

#define MDEF_CREDATTRBMSIZE MBMSIZE(mcaLAST)

/* sync w/MObjectAction[] */

enum MObjectActionEnum {
 moaNONE = 0,
 moaCreate,
 moaList,
 moaModify,
 moaDestroy,
 moaDiagnose,
 moaLAST };

#define MDEF_OBJECTACTIONBMSIZE MBMSIZE(moaLAST)

typedef struct mprivilege_t {

  enum MXMLOTypeEnum OType;

  mbitmap_t Perms;

  /* assignment operator */
  mprivilege_t& operator=(const mprivilege_t& rhs)
    {
    OType = rhs.OType;
    Perms = rhs.Perms;
    return *this;
    }

  /* copy constructor */
  mprivilege_t(const mprivilege_t& rhs) : OType(rhs.OType), Perms(rhs.Perms) { };
  mprivilege_t(){};

  }  mprivilege_t;



/**
 * @struct mgcred_t
 *
 * @see enum MCredAttrEnum, struct mclass_t, struct mqos_t
 *
 * general cred (user, group, account)
 */

typedef struct mgcred_t {
  char        Name[MMAX_NAME];  /* cred name                     */

  long        MTime;            /* modify time                   */

  char      **ResourceList;     /* list of accessible resources  */

  mulong      AMUpdateTime;     /* time AM state data was updated */

  /* tdesc_t    *CLimit; */

  /* NOTE:  CList contains local credentials including the following */
  /*        user, group, account                                     */

  void **CList;  /* conditional value type sucval_t in grid evaluation */

  char  *Pref;                        /* preference string (alloc)     */
  char  *EMailAddress;                /* email address (alloc/user only) */

  enum MRoleEnum Role;                /* admin role (user only)        */

  mulong Key;                         /* array hash key                */

  long   OID;                         /* was "unsigned long" - set to -1 for unknown */

  /* priority adjustments */

  long   ClassSWeight;                /* class subcomponent weight     */
  long   FSCWeight;

  mfs_t  F;                           /* fairness policy config        */

  mcredl_t L;                         /* current resource usage        */

  must_t Stat;                        /* historical usage statistics   */

  /* TODO:  roll DoHold,ProfilingIsEnabled,Messages,ReqRID into mcredcom_t */

  mbool_t DoHold;                     /* credential jobs should be blocked */
  mbool_t ProfilingIsEnabled;         /* profiling is enabled */
  mbool_t IsDeleted;
  mbool_t CPLoaded;                   /* CP data has already been loaded */
  mbool_t NoSubmit;                   /* credential cannot submit jobs */
  mbool_t NoEmail;                    /* user cannot send e-mail */

  /* NOTE:  cred flags located in mfs_t structure */

  mulong  ETime;                      /* job is dynamic, remove after ETime */

#ifdef MNOT
  char   *ReqRID;                     /* cred to rsv binding (alloc)   */
#endif

  mmb_t  *MB;                         /* (alloc) */

  /* don't put a double in-between an mbool_t and a char* */
  /* causes problems in some cases */

  enum MCalPeriodEnum CreditRefreshPeriod;

  mbitmap_t  JobNodeMatchBM;             /**< bitmap of enum MNodeMatchPolicyEnum */

  double  ChargeRate;                 /* charge rate (in SU/second?) */
  double  RsvCreationCost;            /* cost per node to create a reservation (qos will be checked first) */

  char   *HomeDir;                    /* (alloc,specified for user only) */
  char   *ProxyList;                  /* comma-delimited list of allowed proxies (alloc,specified for user only) */
  mbool_t ValidateProxy;              /* validate any proxy not explicitly listed in ProxyList */

  mbool_t ViewpointProxy;             /* this credential is a viewpoint proxy */

  mln_t  *Variables;                  /* linked-list of generic variables (alloc) */

  char   *VDef;                       /* default VPC profile (alloc) */

  marray_t *TList;                    /* triggers (alloc) */

  mbitmap_t UserSetAttrs;             /*Bitmap representing attributes that have been set by a user/admin at runtime */

  mln_t *Privileges;
  } mgcred_t;



/* class specific flags, sync w/MClassFlags[] */

enum MClassFlagsEnum {
  mcfNONE = 0,
  mcfRemote,     /* queue is reported by local RM */
  mcfLAST };


#define MDEF_CLASSFLAGSIZE MBMSIZE(mdfLAST)

/**
 * @struct mclass_t
 *
 * class/queue structure
 *
 * @see struct mgcred_t - general cred structure
 * @see struct mfs_t - child
 * @see struct must_t - child
 * @see struct mcredl_t - child
 */

typedef struct mclass_t {
  char       Name[MMAX_NAME];
  int        Index;
  mulong     MTime;            /* time record was last updated */
  mulong     RMMTime;          /* time resource manager config was last updated */

  int        LogLevel;         /* NOTE:  loglevel enforces log value on class AND jobs */

  enum MQueueTypeEnum     Type;
  enum MQueueFuncTypeEnum FType;
  mfs_t      F;
  mcredl_t   L;
  mcredl_t   RMLimit;          /* Limits to 'L' that RMs must keep changes bounded within */
  must_t     Stat;

  int        GResIndex;

  mbitmap_t Flags;        /**< class flags */

  mbool_t    IsDisabled;       /* queue cannot execute jobs */
  mbool_t    IsPrivate;        /* queue available only to specific nodes */
  mbool_t    IsDeleted;

  mbool_t    RMACLEnforce;     /* resource manager is responsible for enforcing class ACL's, scheduler should ignore */
  mbool_t    DisableAM;        /* if set, do not contact accounting manager for associated jobs */
  mbool_t    NAIsSharedOnSerialJob; /* if set, serial jobs will inherit a node allocation policy of shared */
  mbool_t    NAIsDefaultDedicated;  /**< by default, node access is dedicated unless explicitly
                                         specified otherwise by job, @see mrmifDefaultNADedicated */

  void      *RM;               /* RM with which class is associated (pointer,NULL for all) */
  mbitmap_t  RMAList;          /* bitmap of RMs which can view/utilize RM */

  enum MRoleEnum Role;         /* admin role (template spec only) */

  /* NOTE:  most policies contained w/in L.JSet->Req[0] */

  enum MAllocResFailurePolicyEnum ARFPolicy; /* action to take when allocated node fails for active job */
  mbool_t    ARFOnMasterTaskOnly;

  enum MTaskDistEnum  DistPolicy;

  char      *HostExp;          /* (alloc) */

  struct mnl_t NodeList;         /* list of nodes which can access class */

  char      *OCNodeName;       /* overcommit node */
  double     OCDProcFactor;    /* dedicated proc factor */

  mbitmap_t DefFBM;            /* default node features (BM) */

  int        MaxProcPerNode;   /* limit? */

  int        MaxArraySubJobs;   /* max number of array sub jobs per class */

  mbool_t    DoHold;
  mbool_t    ProfilingIsEnabled;  /* profiling is enabled */

#ifdef MNOT
  char      *ReqRID;           /* cred to rsv binding (alloc)   */
#endif

  char      *JobProlog;        /* (alloc) */
  char      *JobEpilog;        /* (alloc) */

  char      *TermSig;          /* class job termination signal (alloc) */

  mulong     PreTerminationSignalOffset;  /* time before termination that signal should be sent to job */
  int        PreTerminationSignal; /* signal to send prior to termination */

  char      *Partition;        /* partition that jobs from this class must run inside */

  mnprio_t  *P;                /* node allocation priority parameters (optional/alloc) */

  mmb_t     *MB;               /* general message buffer (alloc) */

  mln_t     *Variables;        /* linked-list of generic variables (alloc) */

  marray_t  *TList;            /* triggers (alloc) */

  mxml_t    *FSTreeNode[MMAX_PAR]; /* pointer into FSTree */

  mbool_t    NoSubmit;  /* no jobs can be submitted to this class
                           NOTE: C->State reports RM queue state, this reports
                                 scheduler level policy, both must be enforced */
  
  mbool_t    IgnNodeSetMaxUsage; /* Ignore all nodesetmaxusage policies. (Default: FALSE) */

  mbool_t    CancelFailedJobs;   /* for scinet: cancel jobs that fail to start (instead of adding to excluded nodelist) */
  
  int        ConfiguredProcs;    /* Number of processors configured in Class */
  int        ConfiguredNodes;    /* Number of Nodes configured in Class */
  } mclass_t;



/**
 * dynamic/specified job attributes
 *
 * @see MJobDynAttr[] (sync)
 * @see struct mqos_t.DynAttrs
 * @see struct mqos_t.SpecAttrs
 */

enum MJobDynAttrEnum {
  mjdaNONE = 0,
  mjdaAccount,
  mjdaNAPolicy,    /* node access policy */
  mjdaNodeCount,
  mjdaPreempt,
  mjdaPriority,
  mjdaQOS,
  mjdaQueue,
  mjdaTaskCount,
  mjdaWCLimit,
  mjdaVar,
  mjdaLAST };


/** 
 * QOS flags 
 *
 * @see MQOSFlags[] (sync)
 * @see MSecFlagsEnum (peer)
 * @see struct mqos_t.Flags[]
 */

enum MQOSFlagEnum {
  mqfNONE = 0,
  mqfignJobPerUser,
  mqfignProcPerUser,
  mqfignJobQueuedPerUser,
  mqfignMaxPE,
  mqfignMaxProc,
  mqfignMaxTime,
  mqfignMaxPS,
  mqfignUser,
  mqfignSystem,
  mqfignAll,
  mqfpreemptor,
  mqfpreemptee,          /**< job is always preemptee */
  mqfDedicated,          /**< dedicate full node resources to job for all allocated nodes */
  mqfReserved,           /**< job should always get reservation */
  mqfUseReservation,     /**< job can only run w/in existing rsv */
  mqfNoBF,               /**< job may not utilize backfill */
  mqfNoReservation,      /**< job may not create future priority rsv */
  mqfNTR,                /**< next to run */
  mqfRunNow,             /**< take all actions necessary to run job immediately */
  mqfPreemptFSV,         /**< mark job preemptible if fairshare violation is detected */
  mqfPreemptSPV,         /**< mark job preemptible if soft policy violation is detected */
  mqfIgnHostList,
  mqfProvision,          /**< modify resources to meet job requirements */
  mqfDeadline,           /**< specify deadline to meet QOS performance targets (26) */
  mqfEnableUserRsv,      /**< allow end user to create/manage own reservations */
  mqfCoAlloc,            /**< allow multi-partition co-allocation */
  mqfTrigger,            /**< job is allowed to explicitly request job trigger */
  mqfPreemptCfg,         /**< allow per-job preempt config specification */
  mqfPreemptorX,         /**< only allow preemption conditionally (ie, if worth lost cycles) */
  mqfVirtual,            /**< qos allows jobs to run in virtual environments */
  mqfLAST };

/* sync w/ MQosFlags[] */
#define MQFUSER       "ignjobperuser,ignjobqueuedperuser,ignprocperuser"
#define MQFSYSTEM     "ignsysmaxproc,ignsysmaxtime,ignsysmaxps"
#define MQFALL        "ignjobperuser,ignjobqueuedperuser,ignprocperuser,ignsysmaxproc,ignsysmaxtime,ignsysmaxps"

#define MDEF_QOSFLAGSIZE MBMSIZE(mqfLAST)



#define MMAX_QOSRSV   16

/**
 * QoS (quality of service) structure
 *
 * @see struct mjob_t.Cred
 *
 * @see struct mgcred_t (general cred structure)
 * @see struct mclass_t (class/queue structure)
 */

typedef struct mqos_t {
  char          Name[MMAX_NAME];
  int           Index;
  long          MTime;            /* time record was last updated             */

  /* priority adjustments */

  long          QTSWeight;
  long          XFSWeight;
  long          FSCWeight;

  /* service targets */

  long          QTTarget;         /* queue time target                        */
  double        XFTarget;         /* xfactor target                           */

  /* preemption thresholds */

  double        BLPreemptThreshold; /* allow preemption if backlog threshold exceeded */
  long          QTPreemptThreshold; /* allow preemption if qtime threshold exceeded */
  double        XFPreemptThreshold; /* allow preemption if xfactor threshold exceeded */

  /* reservation thresholds */

  double        BLRsvThreshold;   /* maintain rsv if threshold exceeded */
  long          QTRsvThreshold;   /* maintain rsv if threshold exceeded */
  double        XFRsvThreshold;   /* maintain rsv if threshold exceeded */

  /* green thresholds */

  double        GreenBacklogThreshold;
  long          GreenQTThreshold;
  double        GreenXFactorThreshold;

  /* trigger thresholds */

  double        BLTriggerThreshold;
  long          QTTriggerThreshold;
  double        XFTriggerThreshold;

  /* power thresholds - if all thresholds are satisfied for a specific job, allow nodes to be powered on */

  double        BLPowerThreshold;
  long          QTPowerThreshold;
  double        XFPowerThreshold;

  /* reservation access thresholds */

  /* if set, only grant QOS based ACL access if thresholds are exceeded */

  double        BLACLThreshold;   /* enable rsv access if threshold exceeded (in proc-seconds?) */
  long          QTACLThreshold;   /* enable rsv access if threshold exceeded (in seconds?) */
  double        XFACLThreshold;   /* enable rsv access if threshold exceeded (unitless) */

  /* performance metrics - updated in MStatAdjustEJob() */

  double        Backlog;          /* total backlog associated w/QOS - see Q->L.IP->Usage[mptMaxPS][0] */
  double        MaxXF;            /* max XF for jobs in QoS for current iteration */
  long          MaxQT;            /* max QT for jobs in QoS for current iteration */

  /* privileged service access */

  enum MRoleEnum Role;            /* admin role (template spec only) */

  /* NOTE:  jobflags located in Q->F */

  mbitmap_t     Flags;            /**< qos flags (bitmap of enum MQOSFlagEnum) */
  mbitmap_t     DynAttrs;         /**< job attributes which can be dynamically modified by user (bitmap of enum MJobDynAttrEnum) */
  mbitmap_t     SpecAttrs;        /**< job attributes which can be specified by user (bitmap of enum MJobDynAttrEnum) */
  char         *ReqRsvID[MMAX_QOSRSV];       /* list of rsv's in which jobs must run */
  char         *RsvAccessList[MMAX_QOSRSV];  /* list of rsv's which requestor can access */
  char         *RsvPreemptList[MMAX_QOSRSV]; /* list of rsv''s which requestor can preempt */

  long           SystemJobPriority; /* If non-zero, set sysprio on the job (like setspri) */

  enum MPreemptPolicyEnum PreemptPolicy;
  mulong        PreemptMinTime;   /* policy - job should be allowed to run for this duration
                                              before being considered for preemption */
  mulong        PreemptMaxTime;   /* policy - job is not considered for preemption once it has reached this time */

  mulong        RsvTimeDepth;

  /* Job Accrual Policy */

  enum MJobPrioAccrualPolicyEnum JobPrioAccrualPolicy;
  mbitmap_t                      JobPrioExceptions;                    /* (bitmap of MJobPrioExceptionEnum) */

  /* general attributes */

  mfs_t         F;
  mcredl_t      L;
  must_t        Stat;

  mbool_t       OnDemand;
  mbool_t       DoHold;              /* QoS-credential hold enabled */
  mbool_t       ProfilingIsEnabled;  /* profiling is enabled */
  mbool_t       IsDeleted;
  mbool_t       NoSubmit;            /* jobs cannot be submitted to this QOS */
  mbool_t       SpecAttrsSpecified;  /**< NOTE:  replace w/mqaXXX bitmap to cover all qos attributes (NYI) */

  mmb_t        *MB;                  /* general message buffer (alloc) */

  mbitmap_t     ExemptFromLimits;    /* Limits that this QOS is free from (MActivePolicyTypeEnum)*/

  /* costing information */

  double        DedResCost;
  double        UtlResCost;

  double        FSScalingFactor;      /* fairshare usage scaling factor */

  double        RsvCreationCost;
  mulong        RsvRlsMinTime;

  mln_t        *Variables;            /* linked-list of generic variables (alloc) */

  marray_t     *TList;                /* triggers (alloc) */

  mxml_t       *FSTreeNode[MMAX_PAR]; /* pointer into FS tree nodes */
  struct mqos_t  *Preemptees[MMAX_QOS+1];  /* QOS preemptee list (NULL terminated)                    */
  } mqos_t;


typedef struct mcred_t {
  mgcred_t *U;
  mgcred_t *G;
  mgcred_t *A;
  mclass_t *C;
  mqos_t   *Q;

  int       CredType;

  macl_t   *ACL;
  macl_t   *CL; 

  char    **Templates; /* (alloc) - list of 'job match' structures @see mjtmatch_t, @see J->TSets */

  long      MTime;
  } mcred_t;



typedef struct mrep_t {
  long    Time;
  struct  mrsv_t *R;
  } mrep_t;


/* NOTE:  DRes is actively blocked resources during specified res event */

typedef struct mre_t {
  long       Time;   /* time of event */
  short      Type;   /* event type (should be MREEnum, but use short to save memory) */
#if 0
  short      OIndex; /* index of source range which created event */
  int        Index;  /* index of associated rsv ptr in N->R[] */
#endif
  int        TC;
  mcres_t    BRes;   /* total resources dedicated to rsv across all tasks on node */

  struct mre_t *Next;

  struct mrsv_t *R;
  } mre_t;


typedef struct mreold_t {
  long       Time;   /* time of event */
  short      Type;   /* event type (should be MREEnum, but use short to save memory) */
  int        Index;  /* index used for various purposes in MRsvAdjustDRes() */
  int        TC;
  mcres_t    BRes;   /* total resources dedicated to rsv across all tasks on node */

  struct mre_t *Next;

  struct mrsv_t *R;
  } mreold_t;



/* sql object */

typedef struct msql_t {
  char HostName[MMAX_NAME];
  char UserName[MMAX_NAME];
  char Passwd[MMAX_NAME];
  char NodeDBName[MMAX_NAME];
  char JobDBName[MMAX_NAME];
  char NodeTableName[MMAX_NAME];
  char JobTableName[MMAX_NAME];
  int  Port;

  char *Data;  /* used to store failed data... */
  } msql_t;


typedef struct mllmdata_t {
  int   VersionNumber;
  int   ProcVersionNumber;

  void *JobQO;
  void *JobList;

  void *NodeQO;
  void *NodeList;
  } mllmdata_t;

#ifndef MGANGLIABUFSIZE
#define MGANGLIABUFSIZE 1 << 22
#endif  /* MGANGLIABUFSIZE */


typedef struct mnatmdata_t {
  int   ICount;                    /* iteration count */

  char  **ResName;

  msnl_t   ResAvailICount; /* iterations with resources available (no or partial use) */
  msnl_t   ResFreeICount;  /* iterations with resources idle (no use) */

  msnl_t   ResAvailCount;  /* total quantity of resources available (*IC) */
  msnl_t   ResConfigCount; /* total quantity of resources configured (*IC) */

  int   CurrentUResCount;          /* unique resource types */
  int   CurrentResAvailCount;      /* total avail resource instances */
  int   CurrentResConfigCount;     /* total config resource instances */

  void *N;                         /* virtual node pointer */
  } mnatmdata_t;


typedef struct mmoabmdata_t {
  char VPCID[MMAX_NAME];

  char *ReqArch;
  char *ReqOS;
  char *ReqPkg;
  int   ReqMem;

  /* cluster */

  long  CMTime;
  int   CIteration;  /* iteration when CData was last updated */
  int   CCount;
  void *CData;
  char *CEMsg;
  mbool_t CDataIsProcessed;  /* signifies whether or not CData has been processed */

  /* workload */

  long  WMTime;
  int   WIteration;  /* iteration when WData was last updated */
  int   WCount;
  void *WData;
  mbool_t WDataIsProcessed;  /* signifies whether or not WData has been processed */

  /* queue */

  long  QMTime;
  int   QIteration;
  int   QCount;
  void *QData;

  /* server */

  long  SMTime;
  int   SIteration;
  int   SCount;
  void *SData;
  } mmoabmdata_t;


/**
 * Native RM data structure
 *
 * @see mrm_t.ND 
 */

typedef struct mnat_t {
  char *ServerHost;         /* general server host for all interfaces (alloc) */
  int   ServerPort;

  mbool_t IsNodeCentric;    /* cluster report is node centric */

  msql_t *S;                /* (alloc) */

  enum MRMSubTypeEnum NatType;

  char JobCtlDir[MMAX_PATH];

  /* specified URL */

  char *URL[mrmaLAST];         /* (alloc) */
  enum MBaseProtocolEnum Protocol[mrmaLAST];
  enum MBaseProtocolEnum EffProtocol[mrmaLAST];  /* used if data format is different
                                                   than data source (ie, ganglia via file) */

  char *Path[mrmaLAST];        /* (alloc) */
  mbool_t AdminExec[mrmaLAST]; /* execute action as admin */
  char *Host[mrmaLAST];        /* server host associated with interface (alloc) */
  int   Port[mrmaLAST];        /* server port associated with interface */

  /* AM specific URLs.  These are needed native AMCFG configurations. */

  char   *AMURL[mamnLAST];         /* AMCFG URLs (alloc) */
  char   *AMHost[mamnLAST];          /* AMCFG Hosts (alloc) */
  char   *AMPath[mamnLAST];        /* AMCFG Paths (alloc) */
  int     AMPort[mamnLAST];
  enum MBaseProtocolEnum AMProtocol[mamnLAST];

  /* processed values */

  mstring_t QueueQueryData;    /* (alloc) */
  mstring_t ResourceQueryData; /* (alloc) */
  mstring_t WorkloadQueryData; /* (alloc) */

  void *R;                   /* parent RM pointer */

  /* network/storage rm attributes */

  double   TransferRate;      /* for storage node, specified/reported speed
                                 of data transfers   */
                              /* for compute node, speed of network adapter  */
                              /* measured in kb/s                            */
  } mnat_t;


typedef struct pbsnodedata_t {
  char    Name[MMAX_NAME];

  long    MTime;

  int     CProcs;
  int     CMem;
  int     CSwap;

  int     ADisk;

  int     ArchIndex;

  int     MState;

  double  Load;

  void   *xd;
  } pbsnodedata_t;


/* SLURM flags (sync w/MSLURMFlags[]) */

enum MSLURMFlagsEnum {
  mslfNONE = 0,
  mslfNodeDeltaQuery,  /* allow delta based node cluster query */
  mslfJobDeltaQuery,   /* allow delta based job cluster query */
  mslfCompressOutput,  /* compress job start task list (e.g. odev[01-10]) */
  mslfLAST };

enum MLSFEmulationEnvEnum {
  mlsfeNONE = 0,
  mlsfeHosts,
  mlsfeSubmitDir,
  mlsfeJobID,
  mlsfeJobName,
  mlsfeQueue,
  mlsfeJobIndex,
  mlsfeUnixGroup,
  mlsfeDefaultProject,
  mlsfeMCPUHosts,
  mlsfeLAST };

typedef struct mwikimdata_t {
  msocket_t EventS;
  mbitmap_t Flags;
  char      SBinDir[MMAX_PATH_LEN];
  } mwikimdata_t;


typedef struct msssmdata_t {
  int    VersionNumber;
  int    ServerSD;
  long   ServerSDTimeStamp;
  int    SchedSD;
  long   SchedSDTimeStamp;
  char   LocalDiskFS[MMAX_PATH_LEN];
  char   SubmitExe[MMAX_PATH_LEN];

  long   NodeMTime;
  long   JobMTime;
  long   QueueMTime;

  void  *NodeList;
  void  *QueueList;

  mhash_t JobListIndex;  /* stores job names that this RM manages */
  } msssmdata_t;


typedef struct mpbsmdata_t {
  int    VersionNumber;
  int    ServerSD;
  long   ServerSDTimeStamp;
  msocket_t SchedS;
  long   SchedSDTimeStamp;
  char   LocalDiskFS[MMAX_PATH_LEN];
  char   SubmitExe[MMAX_PATH_LEN];
  char   SBinDir[MMAX_PATH_LEN];

  int    UpdateIteration;
  long   MTime;

  mbool_t PBS5IsEnabled;
  mbool_t S3IsEnabled;
  mbool_t IsInitialized;
  mbool_t NoNN;             /* neednodes not required to maintain alloc hostlist */
  mbool_t DisableStartAPI;  /* start API is broken, directly call job run command */
  mbool_t EnableCompletedJobPurge;  /* enables the feature which Moab tells TORQUE when to purge
+                                       completed job (only available in new versions of TORQUE) */


  /* cluster */

  long  CMTime;
  int   CIteration;
  int   CCount;
  void *CData; /* cluster data (nodes), PBSPro 9: Hosts */
  void *VData; /* PBSPro 9: virtual nodes */
  char *CEMsg;

  /* workload */

  long  WMTime;
  int   WIteration;
  int   WCount;
  void *WData;

  /* queue */

  long  QMTime;
  int   QIteration;
  int   QCount;
  void *QData;

  /* server */

  long  SMTime;
  int   SIteration;
  int   SCount;
  void *SData;

  mulong RMJobCPurgeTime;

  /* The completion time of the most recent successfully
     checkpointed completed job. This is used to tell
     TORQUE that it can purge completed jobs that completed at
     or before this time.  */

  mulong LastCPCompletedJobTime;
  } mpbsmdata_t;


/* sync w/MRMNodeStatePolicy[] */

enum MRMNodeStatePolicyEnum {
  mrnspNONE = 0,
  mrnspOptimistic,
  mrnspPessimistic,
  mrnspLAST };


/* sync w/MRMResourceType[] (see R->RTypes in mrm_t) */

enum MRMResourceTypeEnum {
  mrmrtNONE = 0,
  mrmrtCompute,
  mrmrtInfo,
  mrmrtLicense,
  mrmrtNetwork,
  mrmrtProvisioning,
  mrmrtStorage,
  mrmrtAny,           /* used in MNodeGetRMState() */
  mrmrtLAST };


/**
 * Rsv allocation policy (sync w/MRsvAllocPolicy[])
 *
 * @see MRsvRefresh() - enforces Rsv ReAlloc policy
 * @see struct mrsv_t.AllocPolicy
 */

enum MRsvAllocPolicyEnum {
  mralpNONE = 0,
  mralpFailure,             /* re-alloc if failure detected at any time */
  mralpNever,               /* never re-alloc resources */
  mralpOptimal,             /* re-alloc for failure and to improve resource selection */
  mralpPreStartFailure,     /* re-alloc for failures which occur before rsv starts */
  mralpPreStartMaintenance, /* re-alloc at pre-start if failure detected or maintenance rsv overlaps */
  mralpPreStartOptimal,     /* re-alloc at pre-start for failures and to improve resource selection */
  mralpRemap,
  mralpLAST };

/**
 * Data staging object
 */

typedef struct mds_t {

  msdata_t   *SIData;         /**< stage in data (alloc) */
  msdata_t   *SOData;         /**< stage out data (alloc) */

  char  *PostStagePath;       /** Post stage action perl script from the tools dir on server (alloc) */
  char  *LocalDataStageHead;  /**< node where stdout/stderr files will be staged back to (alloc) */

  mbitmap_t PostStageAfterType; /** The post stage action runs after this type of file(s) has been staged */
                                 /** BM of enum MPostDSFileEnum */
 } mds_t;


/* object cred and file translation map */

typedef struct momap_t {
  /* currently only support file and exec */

  enum MBaseProtocolEnum  Protocol;
  char                   *Path;                 /* (alloc) */

  mulong                  UpdateTime;

  mbool_t                 IsMapped[mxoALL];

  mbool_t                 AreGlobalCreds; /* SpecNames refer to global credential names (i.e. Globus creds) */

  enum MXMLOTypeEnum      OType[MMAX_CREDS];     /* currently support:  mxoUser, mxoGroup, mxoClass, mxoAcct, mxoQOS */
                                                /*                     mxoCluster (for files)                       */

  mbool_t                 Local[MMAX_CREDS];     /* mbpGridMap protocol */

  char                   *SpecName[MMAX_CREDS];  /* specified object name (alloc) */
  char                   *TransName[MMAX_CREDS]; /* translated name (alloc) */
  } momap_t;


/* sync w/MResAvailQueryMethod[] */

enum MResAvailQueryMethodEnum {
  mraqmNONE = 0,
  mraqmAll,        /* includes historical, priority, and reservation only */
  mraqmHistorical,
  mraqmLocal,
  mraqmPriority,
  mraqmReservation,
  mraqmLAST };


/* node destroy policy (sync w/MNodeDestroyPolicy[]) */

enum MNodeDestroyPolicyEnum {
  mndpNONE = 0,
  mndpAdmin,
  mndpDrain,
  mndpHard,
  mndpSoft,
  mndpLAST };

/**
 * @struct mrm_t
 *
 * resource manager object
 *
 * @see enum MRMAttrEnum
 */

#define MDEF_RMSTAGETHRESHOLD   30   /* see R->StageThreshold */

struct mrm_t {
  char   Name[MMAX_NAME + 1];
  int    Index;

  char  *Description;  /* human readable description (alloc) */

  char  *Env;          /* environment to be applied to child tools (alloc) */

  char   ConfigFile[MMAX_PATH_LEN];

  /* interface */

  enum MRMTypeEnum    Type;
  enum MRMSubTypeEnum SubType;

  mbitmap_t RTypes;                /**< Resource Types (bitmap of enum MRMResourceTypeEnum) */

  mbitmap_t Languages;                        /* BM of MRMTypeEnum */
  mbitmap_t SubLanguages;                     /* BM of MRMSubTypeEnum */

  enum MRMNodeStatePolicyEnum NodeStatePolicy;

  char   APIVersion[MMAX_NAME];
  int    Version;               /* integer representation of rm protocol version */

  enum MRMStateEnum State;

  int    VPGResIndex;           /**< generic resource to map node virtual processors to */
  double VPGResMultiplier;      /**< virtual processor generic resource multiplier */
  mulong CTime;                 /**< time dynamic RM was created */
  mulong StateMTime;
  mulong LastSubmissionTime;
  mulong PollInterval;          /* if specified, indicates duration between subsequent polls */
  mulong MaxASyncJobStartTime;  /* time in which an async job start must finalize  */
  mulong WorkloadUpdateTime;    /* time last successful workload update was initiated */
  mulong MaxJobStartDelay;      /* time newly started job can report idle before action is taken */
  mulong TaskMigrationDuration; /** estimated time to complete migration of tasks w/in active job */

  long   TrigFailDuration;      /* length of time after failure at which trigger will fire */
  int    MaxFailCount;          /* number of failures allowed per iteration before giving up */

  int    RestoreFailureCount;   /* number of subsequent attempts to restore interface which have failed */
                                /* checkpointing is optional */

  mbool_t PollTimeIsRigid;
  mbool_t JobIDFormatIsInteger; /* NOTE: expand to jobidformat enum (NYI) */

  mbitmap_t   LocalJobExportPolicy; /* bitmap of enum MExportPolicyEnum */

  /* state boolean */

  mbool_t TypeIsExplicit;       /* RM type is explicitly specified */
  mbool_t NoAllocMaster;        /* RM should not allocate independent req for master head node (XT only) */

  mulong UTime;                 /* last update time */
  mulong NextUpdateTime;        /* next update time - do not update RM before this time, if set */
  double MinExecutionSpeed;     /* minimum relative clock execution speed (% factor) */

  long ClockSkewOffset;         /* difference in time between scheduler and resource manager */
                                /* MSched.Time - RM->Time */

  long BackLog;

  /* NOTE:  use P for primary/fallback and for distributed RM services */

  int    ActingMaster;          /* index into 'P' for current master RM server */

  mpsi_t P[MMAX_RMSUBTYPE];     /* peer interface */

  enum MRMAuthEnum AuthType;

  int    NMPort;                /* node monitor port */

  int    EPort;                 /* event port */
  long   EMinTime;              /* min time between processing events */
  mulong EventTime;             /* time last event was received */

  int    SpecPort[MMAX_RMSUBTYPE];

  mbool_t IgnNCPUs;             /* ncpus job attr is derived, not specified, ignore */

  mbool_t IsVirtual;            /* rm does not execute core job */
  mbool_t IsDeleted;
  mbool_t FirstContact;         /* first contact received */

  char    *Profile;             /* profile used to create dynamic RM */
  char    *ExtHost;             /**< external hostname to use when reporting to grid (alloc) */
  mbool_t IsInitialized;        /* client rm has been initialized */
  mbool_t GridSandboxExists;    /* requests associated with this interface are constrained by grid sandbox */

  mbool_t JobRsvRecreate;
  mbool_t ApplyGResToAllReqs;
  mbool_t SyncJobID;            /* push jobid in job submit */

  mbool_t UseVNodes;            /* Query vnodes in MPBSClusterQuery instead of hostnodes */
  mbool_t IgnHNodes;            /* Ignore HostNodes (natural nodes) when using virtual nodes */

  mulong  StandbyBacklogPS;     /* backlog for standby node specific jobs */

  char     *AuthAServer;        /* requestor list or list location - URL (alloc) */
  char    **AuthA;              /* list of requestors allowed thru this RM (alloc,maxsize=MMAX_CREDS) */

  char     *AuthCServer;        /* requestor list or list location - URL (alloc) */
  char    **AuthC;              /* list of requestors allowed thru this RM (alloc,maxsize=MMAX_CREDS) */

  char     *AuthGServer;        /* requestor list or list location - URL (alloc) */
  char    **AuthG;              /* list of requestors allowed thru this RM (alloc,maxsize=MMAX_CREDS) */

  char     *AuthQServer;        /* requestor list or list location - URL (alloc) */
  char    **AuthQ;              /* list of requestors allowed thru this RM (alloc,maxsize=MMAX_CREDS) */

  char     *AuthUServer;        /* requestor list or list location - URL (alloc) */
  char    **AuthU;              /* list of requestors allowed thru this RM (alloc,maxsize=MMAX_CREDS) */

  char     *OMapServer;         /* object map location - URL (alloc) */
  momap_t  *OMap;               /* object map (alloc) */
  char     *OMapMsg;            /* object map error message (alloc) */
  char     *CMapEMsg;           /* most recent credential mapping error (alloc) */
  char     *FMapEMsg;           /* most recent file mapping error (alloc) */

  mclass_t *DefaultC;           /* default class (pointer) */
  mulong    DefWCLimit;         /* default wallclock limit */
  int       DefOS;              /* default node operating system */

  mjob_t   *JSet;
  mjob_t   *JMax;
  mjob_t   *JDef;
  mjob_t   *JMin;

  struct mnl_t NL;              /* list of nodes reported by rm */

  mln_t *Variables;             /* arbitrary rm variables (alloc) */

  char   DefaultQMDomain[MMAX_NAME];
  char   DefaultQMHost[MMAX_NAME];

  char   SoftTermSig[MMAX_NAME];
  char   SuspendSig[MMAX_NAME];
  char   CheckpointSig[MMAX_NAME];
  long   CheckpointTimeout;        /* time between checkpoint request and termination signal */

  int    InitIteration;            /* will be set to the iteration at which Moab successfully establishes connection with RM (success in MRMInitialize()) */
  int    FailIteration;            /* last iteration general resource manager failure has been detected */
  int    WorkloadUpdateIteration;  /* last iteration workload data was successfully updated */
  int    FailCount;                /* number of failures detected this iteration (cleared in MRMRestore()) */

  int    JobCount;                 /* jobs currently reported via RM */
  int    JobStartCount;            /* jobs started via RM since last restart */
  int    JobNewCount;              /* new jobs reported via RM since last iteration */

  int    MaxJobsAllowed;           /* the max num of jobs this RM interface will load into
                                      Moab each iteration */

  int    NC;                       /* number of nodes currently available */
  int    TC;                       /* number of tasks/processors currently available */
  int    ReqNC;                    /* requested number of nodes (for dynamic resource managers) */

  int    JobCounter;               /* keeps track of job's naming number */

  int    RMNI;                     /* network interface utilized by RM */

  char  *SubmitCmd;                /* (alloc) */
  char  *StartCmd;                 /* (alloc) */

  int    PtIndex;                  /* partition index into which detected resources will be assigned */

  /* policies */

  int    MaxDSOp;           /* maximum simultaneous data stage operations */
  int    MaxJobHop;         /* maximum number of staging hops per job */
  int    MaxNodeHop;        /* maximum number of staging hops per node */
  int    MaxJobPerMinute;   /* maximum number of jobs allowed to start per one minute interval */
  double MaxTransferRate;   /* maximum transfer rate for staging jobs and data to this resource manager (MB/s) */
  mulong IntervalStart;     /* epoch time current interval started */
  mulong IntervalJobCount;  /* number of jobs launched during current interval */
  double TargetUsage;       /* target percentage usage for primary resource (varies by RType) */

  mulong AvgQTime;          /* avg queue time for workload associated with this RM (in seconds) */
  double AvgXFactor;        /* avg xfactor for workload associated with this RM */

  mulong CancelFailureExtendTime;  /* length of time to extend job wclimit if cancel fails to complete */
                               /* this prevents other jobs from attempting to use resources which will be unavailable */

  char  *NodeFailureRsvProfile;

  enum MRMSubmitPolicyEnum SubmitPolicy;

  mpbsmdata_t  PBS;
  mwikimdata_t Wiki;

  struct {
    msssmdata_t  S3;
    mllmdata_t   LL;
    mnatmdata_t  Nat;  /* native mode statistics */
    mmoabmdata_t Moab;
    } U;

  char  *DefHighSpeedAdapter;  /* used for LL (alloc) */

  void  *S;   /* resource manager specific data */

  mmb_t *MB;  /* general message buffer (alloc) */

  mnat_t ND;  /* native data */

  mbitmap_t FBM; /* default node features (MAttr[] BM) */

  mbitmap_t Flags;      /* general resource management attribute flags (MRMFlagEnum BM) */
  mbitmap_t SpecFlags;  /* admin specified flags (MRMFlagEnum BM) */
  mbitmap_t IFlags;     /* general resource management attribute flags (MRMIFlagEnum BM) */
  mbitmap_t FnList;     /* list of enabled functions (enum MRMFuncEnum BM) */

  mbool_t FnListSpecified; /* explicit FnList specified as attribute */
  mbool_t NoUpdateJobRMCreds;  /* if set, do not sync RM-level creds with internal attributes */

  char *IQBuffer;         /* infoquery attribute registration buffer (alloc) */

  mbool_t UseNodeCache;

  struct mrm_t *MRM;      /* (pointer,optional) the "master" RM--this RM appears to have
                             two purposes:

			   1) to point back to discovery RM (e.g. a Ganglia RM has its MRM
                              pointing to TORQUE, which both report info about the same
                              node.  (NOTE:  This purpose should be eliminated and
                              replaced with reference to R->Flags, mrmfNoCreateResource)
			   2) to point back to the utility hosting RM that created this RM
                              (e.g. a newly created "virtual cluster" RM is generated to
                              dynamically increase the size of the cluster--this new RM
                              needs to point back to the RM that helped create it--this is
                              the MRM */
                           /* NOTE:  @see R->PRM */

  struct mrm_t *PRM;       /* (pointer,optional) the "parent" RM--this is the RM through
                              which query and control operations should be routed */

  struct mrm_t *VMOwnerRM; /* for RM's that provision VM's but don't own them,
                            * this points to the RM (such as torque) that does 
                            * own them */

  mulong StageThreshold;   /* (seconds) if job's local starttime is less than this value,
                              then don't consider job for remote migration - also, do not
                              migrate job if local resources are available within <StageThreshold>
                              seconds of remote resource availability */

  mbool_t StageThresholdConfigured; /* This boolean is set to TRUE if the StageThreshold is set from the moab.cfg file.
                                      If this is set then we do not set it to the default value in MMInitialize. */

  struct mrm_t **DataRM;   /* data resource managers (array alloc) */

  mulong DefDStageInTime;  /* default DATA stage-in duration (in seconds) */

  mulong MinStageTime;    /* minimum time to stage data/job to remote rm (in seconds) */
                          /* for prov servers, min time to provision OS */

  mulong MinJobExtendDuration; /* min duration job will be extended */
  mulong MaxJobExtendDuration; /* max duration job will be extended */

  double MaxStageBW;      /* maximum possible DATA bandwidth between local moab and storage RM (in MB/sec) */

  char   ClientName[MMAX_NAME];   /* name of peer client */

  char   StageAction[MMAX_LINE];  /* data stage action */

  mulong PreemptDelay;          /**< amount of time between preempt request and preempt completion */

  /* peer information */

  enum MNodeDestroyPolicyEnum NodeDestroyPolicy;

  marray_t *T;   /* triggers (alloc) */

  void  *MaxN;   /* pseudo-grid 'max capacity' node (alloc) */

  struct mjob_t *OrderedJobs; /* order that jobs were read in from RM (alloc) */

  struct mrrsv_t *RemoteR;    /* pointer to remote reservations */

  mbool_t IsBluegene;     /* true SelectType = select/bluegene in slurm.conf */

  mulong DeltaGetNodesTime; /* save last successful node config update time for delta updates */
  mulong DeltaGetJobsTime;  /* save last successful job config update time for delta updates */
  mulong LastGetDataTime;   /* save last successful MWiki get data time for delta updates */
  mulong LastInitFailureTime; /* last time failed to connect to RM via MRMInitialize */

  char *PTYString; /* config specified pty string for interactive jobs (alloc) */
  };



/* TX object for resource managers.  Used for learning that is RM-specific. */
/* This struct is attached to each RM and alloc'd */

typedef struct mrmtx_t {
  mln_t *OSProvTime;   /* Time for each OS to provision */
  mln_t *OSProvSample; /* Number of samples for each OS provisioning time */
  } mrmtx_t;




/** @see designdoc Generic Resource Structure */

typedef struct mgres_t {
  char    Name[MMAX_NAME];     /**< should sync w/MAList[meGRes] */
  mrm_t  *R;                   /**< rm associated w/gres (optional) */

  char    Type[MMAX_NAME];     /**< generic GRes type/group */

  double  ChargeRate;          /**< per instance-second chargerate */

  mbool_t NotCompute;          /**< generic resource found on compute node */
  mbool_t IsPrivate;           /**< workload requesting this gres should place requirement on private req */
  mbool_t InvertTaskCount;     /**< Inverts the the gres's taskcount to be 1 task with x number of gres per task. */
  mbool_t JobAttrMap;          /**< whether this gres should also be reported as a job attribute */
  mbool_t FeatureGRes;         /**< treat this gres as a scheduler feature, do not place as GRes on job, place in J->ReqFeatureGResBM */

  mrm_t  *RM;                  /**< RM that reported the gres */

  mulong  StartDelay;          /**< time other jobs requesting gres should be blocked when job using gres is started */
  mulong  StartDelayDate;      /**< date of most recent delay */
  } mgres_t;


typedef struct mgrestable_t {
  char **Name;

  mgres_t **GRes; 
  } mgrestable_t;

typedef struct mgmetrictable_t {
  char     **Name;
  mulong    *GMetricReArm;         /* minimum time between re-processing metric threshold */
  mulong    *GMetricLastFired;     /* last time gmetric event was fired */
  mbitmap_t *GMetricAction;        /* actions to take when generic metric threshold exceeded (bitmap of MGEActionEnum) */
  double    *GMetricThreshold;     /* generic metric threshold */
  enum MCompEnum *GMetricCmp;      /* generic metric comparison */
  char    ***GMetricActionDetails; /* action details (alloc) (bitmap of enum MGEActionEnum) */
  int       *GMetricCount;         /* number of times event has occurred */
  int       *GMetricAThreshold;    /* number of times event should occur before action is taken */
  double    *GMetricChargeRate;    /* generic metric chargerate */
  mbool_t   *GMetricIsCummulative; /* gmetric values should be summed across job's allocated nodes when determining total credential gmetric stats */
  } mgmetrictable_t;

/* grid structure */

typedef struct mgrid_t {
  mulong MTime;

  /* interface activity monitors */

  int   FailureCount;

  int   GCount;  /* general call count */
  int   QCount;  /* query count */
  int   JCount;  /* job count */
  int   RCount;  /* reservation count */
  int   CCount;  /* command count */

  /* interface status booleans */

  mbool_t SIsInitialized;  /* server */
  mbool_t IIsInitialized;  /* interface */

  /* default grid callback/event server interface */

  char *CBHost;            /* (alloc) */
  int   CBPort;

  /* grid messages */

  char *Messages;

  /* grid default credentials */

  mclass_t *FallBackClass;   /* class to be used by job if no class is specified */
  mqos_t   *FallBackQOS;     /* qos to be used by job if no qos is specified */
  } mgrid_t;


#define MMAX_JSHOP 1   /* maximum allowed number of job stage hops */

typedef struct mjgrid_t {
  char *SID[MMAX_JSHOP];    /**< job system id history (alloc) */
  char *SJID[MMAX_JSHOP];   /**< job system job id history (alloc) */

  char *User;              /**< (alloc) peer mapping cred */
  char *Group;             /**< (alloc) peer mapping cred */
  char *Class;             /**< (alloc) peer mapping cred */
  char *QOS;               /**< (alloc) peer mapping cred */
  char *QOSReq;            /**< (alloc) peer mapping cred */
  char *Account;           /**< (alloc) peer mapping cred */

  int HopCount;   /**< number of job migrations already completed */
  } mjgrid_t;


/* stat matrix */

typedef struct mstatm_t {
  must_t Grid[MMAX_GRIDTIMECOUNT+1][MMAX_GRIDSIZECOUNT]; /* stat matrix       */
  must_t RTotal[MMAX_GRIDSIZECOUNT];  /* row totals                                         */
  must_t CTotal[MMAX_GRIDSIZECOUNT];  /* column totals                                      */
  } mstatm_t;


/* global stats */

typedef struct mstat_t {
  long   InitTime;
  long   SchedRunTime;            /* elapsed time scheduler has been scheduling             */

  /* current job statistics */

  int    IdleJobs;
  int    EligibleJobs;            /* number of jobs eligible for scheduling                 */
  int    ActiveJobs;              /* number of jobs active                                  */

  /* resource statistics */

  double TotalPHAvailable;        /* total proc hours available to schedule                 */
  double TotalPHBusy;             /* total proc hours consumed by scheduled jobs            */
  double TotalPHDed;              /* total proc hours consumed by scheduled jobs            */

  double SuccessfulPH;            /* proc hours completed successfully                      */
  int    SuccessfulJobsCompleted; /* number of jobs completed successfully                  */

  int    IJobsStarted;            /* number of idle jobs started in current interation      */

  int    PC;                      /* currently available processors */
  int    NC;
  int    QueuedJC;
  mulong QueuedPH;                /* currently queued proc-hours */
  mulong LastQueuedPH;            /* last iteration's queued proc-hours */

  int    TPC;
  int    TNC;                     /* total 'up' node count                                  */
  int    TQueuedJC;               /* total queued job count (since startup)                 */
  mulong TQueuedPH;               /* total queued proc hours (since startup)                */
  mulong TActivePH;               /* total remaining proc hours for active jobs (since startup) */

  int    EvalJC;                  /* total jobs evaluated for execution                     */

  mstatm_t SMatrix[MMAX_GRIDDEPTH + 1];

  char   StatDir[MMAX_LINE + 1];  /* directory for stat files                               */
  int    EventFileRollDepth;      /* maximum number of event files                          */
  FILE  *eventfp;                 /* pointer to daily event file                            */
  FILE  *eventcompatfp;           /* pointer to daily event file in pbs compat format       */

  double MinEff;                  /* minimum scheduling efficiency                          */
  int    MinEffIteration;         /* iteration on which the minimum efficiency occurred     */

  double TotalComDelay;           /* total communication based losses                       */
  int    TotalPreemptJobs;
  double TotalPHPreempted;

  mprofcfg_t P;
  } mstat_t;


/* fairshare */

typedef struct mfsc_t {
  long   PCW[mpcLAST];  /* priority component weight */
  long   PCP[mpcLAST];
  long   PSW[mpsLAST];  /* priority subcomponent weight */
  long   PSP[mpsLAST];
  long   PCC[mpcLAST];  /* priority component cap */
  long   PSC[mpsLAST];  /* priority subcomponent cap */

  mbool_t PSCIsActive[mpsLAST]; /* priority subcomponent is active, this should be triggered by setting a cap, a priority, or a target onto an object */

  long   XFMinWCLimit;

  long   FSInitTime;         /* time to begin tracking FS usage              */

  enum MFSPolicyEnum FSPolicy; /* FS consumption metric                      */
  long   FSInterval;         /* time interval covered by each FS data file   */
  int    FSDepth;            /* number of FS time intervals considered by FS */
  double FSDecay;            /* weighting factor to decay older FS intervals */
  mbool_t FSPolicySpecified; /* TRUE if FSPolicy explicitly specified via config */

  mbool_t UseExpTarget;
  mbool_t EnableCapPriority; /* fs cap targets should also provide priority  */
  mbool_t TargetIsAbsolute;  /* fs target is relative to theoretical usage   */
  /* NOTE:  should also support TargetIsBasedonAvailable (NYI) */

  mbool_t FSTreeCapTiers;    /* prevent lower-level tier contribution from exceeding
                                upper level tier contribution */
  mbool_t FSTreeIsProportional;

  mbool_t FSTreeShareNormalize; /* normalize the share counts within the tree and store in nshares */

  mbool_t FSTreeAcctShallowSearch; /* looks for user directly underneath specified account in fairshare tree
                                      overrides the normal deep subaccount search in fairshare tree. 
                                      Note that this is only for llnl and should be added to our customer docs. */

  double  FSTreeTierMultiplier; /* contribution multiplier for lower level tiers */

  double  FSTreeConstant;

  mxml_t *ShareTree;         /* fairshare tree root (alloc) */
  } mfsc_t;


/* sync w/MFSTreeACLPolicy[] */

enum MFSTreeACLPolicyEnum {
  mfsapOff = 0,
  mfsapFull,
  mfsapParent,
  mfsapSelf,
  mfsapLAST };


/* fairshare tree object */

typedef struct mfst_s {
  double Shares;   /* number of shares allocated to this node */
  enum MFSTargetEnum ShareTargetType;

  double CShares;  /* total number of shares allocated to all child nodes */

  double NShares;  /* total number of normalized shares */
  double UsageNShares; /* normalized number of shares used out of the parent's NShares */

  double UsageShares;    /* effective total fairshare usage for this node */
                   /* NOTE:  this is the sum of all child leaf nodes */

  double Offset;   /* difference of usage from target in shares (under target type policy) */

  enum MXMLOTypeEnum  OType;    /* object type */

  int    PtIndex;  /* partition index */

  void   *O;        /* pointer to cred object */
  mfs_t  *F;        /* pointer to cred fs object, locally allocated for non-cred tnodes */

  must_t    *S;        /* stat object, (alloc) */
  mcredl_t  *L;        /* limit object (alloc) */

  char *Comment;   /* comment on node - prints in mdiag -f -v -v */

  mxml_t *Parent;  /* pointer to parent xml object */
  } mfst_t;


/* mxml_t structure */


typedef struct mxml_t {
  char *Name;        /* node name (alloc) */
  char *Val;         /* node value (alloc) */

  int   ACount;      /* current number of attributes */
  int   ASize;       /* attribute array size */

  int   CCount;      /* current number of children */
  int   CSize;       /* child array size */

  char **AName;      /* array of attribute names (alloc) */
  char **AVal;       /* array of attribute values (alloc) */

  struct mxml_t **C; /* child node list */

  int    CDataCount; /* current number of CData elements */
  int    CDataSize;  /* size of cdata array */
  char **CData;      /* CData elements */

  int     Type;        /* generic XML object type */
  mfst_t *TData;       /* optional extension data (alloc) */
  } mxml_t;

/* XML Node Value encoding of a '<' */
#define XML_VALUE_ANGLE_BRACKET_INT        14
#define XML_VALUE_ANGLE_BRACKET_CHAR      '\016'
#define XML_VALUE_ANGLE_BRACKET_STR       "\016"



/* sync w/MSAN[] */

enum MS3AttrNameEnum {
  msanNONE = 0,
  msanAction,
  msanArgs,
  msanFlags,
  msanName,
  msanObject,
  msanOp,      /**< operation */
  msanOption,
  msanTID,
  msanValue,
  msanLAST };


/* sync w/MSON[] */

enum MS3ObjNameEnum {
  msonNONE = 0,
  msonBody,
  msonData,
  msonEnvelope,
  msonGet,
  msonObject,
  msonRequest,
  msonResponse,
  msonSet,
  msonStatus,
  msonWhere,
  msonOption,
  msonLAST };

enum MJobPrioAttrTypeEnum {
  mjpatNONE = 0,
  mjpatGAttr,
  mjpatGRes,
  mjpatState,
  mjpatLAST };

typedef struct mjpriof_s {
  enum MJobPrioAttrTypeEnum Type;

  int                AIndex;  /* generic attribute */

  long               Priority;

  struct mjpriof_s  *Next;
  } mjpriof_t;


/* sync with MTransAttr[] */

enum MTransAttrEnum {
  mtransaNONE,
  mtransaACL,
  mtransaNodeList,
  mtransaDuration,
  mtransaStartTime,
  mtransaFlags,
  mtransaOpSys,
  mtransaDRes,
  mtransaRsvProfile,
  mtransaLabel,
  mtransaDependentTIDList,
  mtransaMShowCmd,
  mtransaCost,
  mtransaVCDescription,
  mtransaVCMaster,
  mtransaVPCProfile,
  mtransaVariables,
  mtransaVMUsage,
  mtransaNodeFeatures,
  mtransaName,
  mtransaRsvID,
  mtransaIsValid,
  mtransaLAST };

/* NOTE:  for resource/rsv queries, use the following mapping:

    T1=HostList, T2=Duration, T3=StartTime, T4=Flags, T5=OS, T6=Offset

*/



/**
 * Reservation state/config flags
 *
 * @see struct mrsv_t.Flags
 * NOTE: no limit on size
 *
 * sync with MRsvFlags[]
 */

enum MRsvFlagEnum {
  mrfNONE = 0,
  mrfAdvRsvJobDestroy,  /**< cancel advrsv jobs when released */
  mrfApplyProfResources, /**< only apply resource allocation info from profile */
  mrfStanding,          /**< rsv is spawned by a standing reservation object */
  mrfSingleUse,
  mrfSystemJob,         /**< map system job over rsv for accounting */
  mrfByName,            /**< rsv must be requested by name using -l advres */
  mrfPreemptible,
  mrfTimeFlex,          /**< rsv is allowed to adjust the reserved time frame in an attempt to optimize resource utilization */
  mrfSpaceFlex,         /**< rsv is allowed to adjust resources allocated over time in an attempt to optimize resource utilization */
  mrfDedicatedNode,     /**< only one active reservation allowed on node */
  mrfExcludeAll,        /**< reservation does not share reserved resources (see NOTE below) */
  mrfEnforceNodeSet,    /**< enforce node sets when creating reservation */
  mrfEvacVMs,           /**< evacuate VMs on the node when the reservation starts */
  mrfExcludeJobs,
  mrfExcludeAllButSB,   /**< reservation only shares resources with sandboxes */
  mrfExcludeMyGroup,    /**< exclude reservations within the same group */
  mrfAdvRsv,            /**< may only utilize reserved resources */
  mrfIgnIdleJobs,       /**< ignore idle job reservations */
  mrfIgnJobRsv,         /**< ignore job rsv's, but not user or other rsv's */
  mrfIgnRsv,            /**< force rsv onto nodes regardless of other rsv */
  mrfIgnState,          /**< ignore node state when creating reservation */
  mrfOwnerPreempt,
  mrfOwnerPreemptIgnoreMinTime,  /**< owner ignores preemptmintime for this rsv */
  mrfParentLock,        /**< reservation cannot be directly destroyed, only via destruction of parent object */
  mrfPRsv,              /**< non-admin, non-sr, user reservation */
  mrfAllowPRsv,         /**< allow personal rsv to use reserved resources */
  mrfAllowGrid,         /**< allow grid rsv to use reserved resources */
  mrfAllowJobOverlap,   /* allow jobs to overlap rsv, but NOT start during rsv (unless they have ACL access) */
  mrfACLOverlap,        /**< reservation can overlap job rsv when ACLs match */
  mrfIsVPC,             /**< rsv is attached to a virtual private cluster */
  mrfIsClosed,          /**< rsv has empty acl */
  mrfIsGlobal,          /**< rsv applies to all resources */
  mrfIsActive,          /**< rsv has become active/live */
  mrfWasActive,         /**< rsv was previously active/recorded */
  mrfReqFull,           /**< fail if all resources cannot be allocated */
  mrfStatic,            /**< admin modification/cancellation not allowed */
  mrfTrigHasFired,      /**< one or more rsv triggers have been launched */
  mrfEndTrigHasFired,   /**< an end-time non-multifire trigger has fired */
  mrfDeadline,          /**< rsv should be scheduled against a deadline */
  mrfProvision,         /**< rsv should be capable of provisioning */
  mrfNoCharge,          /**< do not create an AM charge for idle cycles in a reservation */
  mrfNoVMMigrations,    /**< override vmigrationpolicy and don't migrate vms that overlap reservation. */
  mrfPlaceholder,       /**< rsv used TID at creation but did is placeholder for VPC (PlaceholderRsvDoc) */
  mrfLAST };

#define MDEF_RSVFLAGSIZE MBMSIZE(mrfLAST)


/**
 * Internal reservation state/config flags
 *
 * @see struct mrsv_t.IFlags
 *
 * no sync 
 */

enum MRsvIFlagEnum {
  mrifNONE = 0,
  mrifReservationProfile, /**< reservation is a reservation profile */
  mrifSystemJobSRsv,
  mrifTIDRsv, /**< reservation was made from TID */
  mrifUsedByVMStorage, /**< reservation has been claimed by a mvm_storage_t structure */
  mrifLAST };           


#define MDEF_RSVIFLAGSIZE MBMSIZE(mrifLAST)

/**
 * Notes on mrfExcludeAll:
 *
 * mrfExcludeAll is set when a user runs "mrsvctl -c -E".  This is a creation
 * only flag and does not stay on the reservation throughout its lifetime.
 * In almost all cases reservations only need to be exclusive at creation
 * time but are then filled with jobs that match the acl. There is a case
 * where on a 2 proc node with 1 active job from user A a reservation is
 * created with -E asking for 1 proc with user A in the ACL.  In this case
 * Moab will make sure there is an exclusive processor at creation time but
 * after that the reservation will be "optimized" and the reservation will
 * effectively be on top of the running job.  This is expected behavior.  As
 * a result a second reservation could be created asking for 1 proc and it
 * would be created.
 */

/**
 * Transaction ID structure
 *
 * @see enum MTransAttrEnum
 *
 * @see MTransAdd()
 * @see MTransFind()
 * @see MTransSet()
 */

typedef struct mtrans_t {
  int   ID;                 /* transaction ID */

  char *Requestor;          /* owner/requestor of transaction (alloc) */

  char *Val[mtransaLAST];   /* (alloc) */

  struct mnl_t NL;

  char *RsvID;              /* (alloc) reservation (rsvgroup) created from this TID */

  mbool_t IsInvalid;
  } mtrans_t;


/* NOTE: Removing MMAX_RALSIZE for preemption purposes, since preemptible jobs are
         stored in J->RsvAList.  Everywhere that MMAX_RALSIZE is used will now be MMAX_PJOB */

/*#define MMAX_RALSIZE          16 */ /* number of rsv access reservations or rsvgroups */

typedef struct mrrsv_t {
  char     Name[MMAX_NAME];  /* remote name of reservation */

  mrm_t   *RM;               /* rm that reported the remote rsv */

  mulong   LastSync;         /* when rsv was last synced */
  struct mrsv_t  *LocalRsv;  /* pointer to local rsv */

  struct mrrsv_t *Next;      /* pointer to next remote rsv */
  } mrrsv_t;



/* composite objects */

/**
 * @struct mrsv_t
 *
 * Holds information about reservations.
 *
 * @see struct msrsv_t - standing reservation
 *
 * NOTE:  when adding new attributes, update the following:
 *  MRsvLoadCP() - if attribute is to be checkpointed add explicit attribute support in routine
 *  MRsvToXML() - update DAList[] to store to checkpoint
 *  MRsvAToString() - all attributes which can be ckpt'd or viewed
 *  MRsvSetAttr() - all attributes which can be ckpt'd or explicitly/implicitly set
 *  MRsvDestroy() - if attribute is dynamically allocated
 *  MRsvToJob() - if rsv attribute should be mapped to job attribute
 *  MRsvInitialize() - initializes new rsv structure and applies rsv profile
 *
 * IMPORTANT NOTE:  All dynamic rsv attributes must be explicitly handled in
 *                  rsv profile application code inside of MRsvInitialize()
*/

typedef struct mrsv_t {
  char    Name[MMAX_NAME + 1];   /**< reservation id                             */

  char   *RsvGroup;              /**< reservation group (alloc)                  */
  char   *RsvParent;             /**< parent reservation, ie SR->Name (alloc)    */

  char   *Profile;               /**< profile was used to create rsv (alloc)     */
  char   *Label;                 /**< reservation label (alloc)                  */

  char   *VPC;                   /**< name of container VPC                      */

  int     LogLevel;              /**< rsv logging/messaging level */

  mulong  CTime;                 /**< creation time */
  mulong  MTime;                 /**< attribute modify time */

  int     CIteration;            /**< creation iteration */

  /* policies/attributes */

  enum MRsvTypeEnum    Type;     /**< rsv type                                   */
  enum MRsvSubTypeEnum SubType;  /**< rsv subtype                                */

  enum MRsvAllocPolicyEnum AllocPolicy; /**< resource allocation policy          */

  int     Mode;                  /**< persistent, slide-forward, slide-back,
                                    slide-any (bitmap of ???) - NYI              */

  long    Priority;              /**< priority of reservation and pseudo-jobs it creates */

  mbitmap_t Flags;               /**< rsv flags (bitmap of enum MRsvFlagEnum) */

  char   *SpecName;              /**< (alloc) */
  char   *Comment;               /**< (alloc) */
  char   *NodeSetPolicy;         /**< (alloc) */
  char   *SysJobID;              /**< id of system job map - should sync w/R->J (alloc,ckpt) */
  char   *SysJobTemplate;        /**< (alloc) template to apply to system job    */

  /* requested/allocated resources */

  char   *HostExp;               /**< host regular expression (alloc)            */
  mbool_t HostExpIsSpecified;    /**< host regular expression was requested      */

  enum MHostListModeEnum ReqHLMode; /**< req hostlist mode                       */

  int     ReqOS;                 /**< MAList[] index for requested OS            */

  mbitmap_t ReqFBM;              /**< required node features          */
  enum MCLogicEnum ReqFMode;

  mcres_t DRes;                  /**< resources to be dedicated per task         */

  mcres_t *RDRes;                /**< requested dedicated resources (alloc)      */

  int     PtIndex;               /**< partition where resources must be located
                                  -1 = span, 0 = any single partition        */

  int     ReqNC;                 /**< requested nodes */
  int     ReqTC;                 /**< requested tasks - 0 if not specified */
  int     MinReqTC;              /**< minimum requested tasks - 0 if not specified */

  int     ReqTPN;                /**< 0 = allocate as many tasks as node will support
                                  1 = allocate single task per node */

  int     ReqArch;               /**< MAList[] index for requested architecture */
  int     ReqMemory;             /**< required minimum node memory (in MB) */

  /* NOTE:  @see MRsvCreate()->MRsvAllocate() - sets R->Alloc*C
            @see MNodePostLoad()->MNodeUpdateResExpression() - restores R->Alloc*C from checkpoint
            @see MRsvReplaceNode(), MRsvModifyNodeList(), MRsvModifyTaskCount()
  */

  int     AllocNC;               /**< nodes currently allocated to rsv */
  int     AllocTC;
  int     AllocPC;               /**< procs currently allocated to rsv - only updated at rsv creation/modification */

  int     DownNC;                /**< allocated nodes which are down */

  struct mnl_t  NL;              /**< allocated node list */
  struct mnl_t  ReqNL;           /**< required node list */
                                 /**< rsv wants all the resources listed in ReqNL */
                                 /**< ReqNL reflects constraints of R->HostExp, R->ReqTC, R->ReqNC (verify this?) */

  /* time constraints */

  mulong    StartTime;           /**< rsv starttime                              */
  mulong    EndTime;             /**< rsv endtime                                */

  int       Duration;            /**< sorry dave...this is needed to make rsvprofiles not kludgy */

  mulong    ExpireTime;          /**< complete rsv: time rsv should be purged    */
                                 /**< courtesy rsv: time by which rsv must be confirmed */

  /* access credentials */

  /* NOTE:  default ACL and CL attrs require 3328 bytes each */

  macl_t *ACL;                   /**< who can access this reservation's resources (alloc) */
  macl_t *CL;                    /**< credentials to use when determining what
                                      resources this rsv has access to (alloc) */

  char  **RsvExcludeList;        /**< (alloc) array of rsv names to be excluded from reservation */

  /* list of rsv/rsvgroups which can be accessed by reservation */

  /* NOTE:  default AccessList attr requires 1024 bytes */

  char  **RsvAList;              /**< (alloc) array of MMAX_PJOB */

  struct mjob_t  *SpecJ;         /**< (alloc) in the case of "-l period" type job,
                                      this contains information about the repeating
                                      job */
  struct mjob_t  *J;             /**< job associated w/rsv (ptr) */
  char   *JName;                 /**< job name (alloc) */

  /* accounting credentials */

  mgcred_t *U;                   /**< accountable user, group, account, qos (ptr) */
  mgcred_t *G;                   /**< NOTE:  accountable cred same as creation cred (ptr) */
  mgcred_t *A;                   /* (ptr) */
  mqos_t   *Q;                   /* (ptr) */
  mclass_t *C;

  void               *O;         /**< rsv owner (ptr) */
  enum MXMLOTypeEnum  OType;     /**< rsv owner type */

  /* utilization statistics */

  double  CIPS;   /**< current idle proc-seconds */
  double  CAPS;   /**< current active proc-seconds */
  double  TIPS;   /**< total idle proc-seconds */
  double  TAPS;   /**< total active proc-seconds */

  mbool_t AllocResPending;  /* rsv is VPC or has 'accountable' credential
                               associated and should have accounting manager
                               charges. */

  double  LienCost;   /**< AM lien/charge */

  /* extension properties */

  char   *SystemID;       /**< user or daemon which created rsv (alloc) */
  char   *SystemRID;      /**< global rsv id */

  mrm_t  *PRM;            /**< peer rm */

  /* triggers to be launched on server host */

  marray_t *T;           /**< trigger list (optional/alloc) */


  mbitmap_t IFlags;        /**< rsv internal flags */

  /* TODO: migrate booleans to Internal Flags */

  mbool_t IsTracked;       /**< reservation resource consumption is tracked */
  mbool_t CancelIsPending; /**< reservation is in the process of being canceled */
  mbool_t EnforceHostExp;  /**< enforce HostExp when modifying rsv */
  mbool_t DisableCheckpoint; /**< do not checkpoint this rsv */

  mmb_t  *MB;         /**< message buffer (alloc) */

  struct mrsv_t *P;   /**< profile (optional,ptr) */

  mln_t *Variables;   /**< list of set variables (alloc) */

  mrrsv_t *RemoteRsv; /**< list of remote reservations (alloc) */

  void   *xd;

  int    MaxJob;      /**< Max number of jobs allowed in reservation */
  int    JCount;      /**< number of jobs in this reservation */

  struct msrsv_t *SR; /**< pointer to standing reservation parent - ONLY populated for non-global SRsvs (periodic jobs) */

  char *CmdLineArgs;  /**< Command line args used to create reservation. (alloc) */

  enum MVMUsagePolicyEnum VMUsagePolicy;

  mvm_t **VMTaskMap; /**< analogous to J->VMTaskMap */

  marray_t *History; /**< linked-list of history */

  long LastChargeTime; /**< time reservation charge was last flushed */

  mln_t *ParentVCs; /* Parent VCs (alloc) - create only in MVCAddObject() */
  } mrsv_t;



/**
 * standing reservation structure
 *
 * @see struct mrsv_t
 */

typedef struct msrsv_t {
  char          Name[MMAX_NAME + 1];

  char         *SysJobTemplate;

  int           Index;

  long          MTime;  /* modification time */

  /* timeframe constraints */

  enum MRsvTypeEnum    Type;
  enum MRsvSubTypeEnum SubType;

  /* time constraints */

  enum MCalPeriodEnum  Period;

  int           Depth;

  mbitmap_t     Days;               /* (BM) */

  int           LogLevel;           /* SR log level */

  mulong        StartTime;          /* day starttime offset */
  mulong        EndTime;            /* day endtime offset */
  mulong        WStartTime;         /* week starttime offset */
  mulong        WEndTime;           /* week endtime offset */

  mulong        Duration;           /* duration (in seconds) */

  mulong        EnableTime;         /* epoch time after which SR is valid */
  mulong        DisableTime;        /* epoch time after which SR is invalid */

  mulong        RollbackOffset;     /* minimum time in the future rsv can start */
  mbool_t       RollbackIsConditional; /* if true, rollback only if rsv is empty */

  /* general attributes */

  long          Priority;           /* reservation priority */

  char         *OS;                 /* (alloc) os to provision inside rsv */

  mbitmap_t     Flags;              /* bitmap of enum MRsvFlagEnum */

  /* credentials */

  mgcred_t     *A;                  /* accountable account (ptr) */
  mgcred_t     *U;                  /* accountable user (ptr) - With GOLD, SR->A must also have SR->U */

  mgcred_t     *O;                  /* owner (ptr) */
  enum MXMLOTypeEnum OType;         /* owner type */

  macl_t       *ACL;

  /* allocated resources */

  mrsv_t       *R[MMAX_SRSV_DEPTH]; /* child reservations (remote rsv) */
  mulong        DisabledTimes[MMAX_SRSV_DEPTH]; /* slots that should not be created */

  struct mnl_t HostList;            /* allocated hosts */
                                    /* UPDATE: (9/25/07 drw) HostList contains all the hosts that this reservation can run on (expanded hostexp) */

  char         *SpecRsvGroup;       /* config-specified RsvGroup - will override SRName as R->RsvGroup
                                       in child reservations if specified */

  /* statistics */

  double        TotalTime;
  double        IdleTime;

  /* resource allocation constraints */

  int           ReqTC;              /* tasks requested */
  int           ReqNC;              /* nodes requested */

  int           TPN;

  mcres_t       DRes;               /* per task dedicated resources requested */

  char          HostExp[MMAX_LINE << 4];  /* requested host expression */

  char          PName[MMAX_NAME];   /* partition name */

  enum MCLogicEnum FeatureMode;

  char        **RsvAList;

  mbitmap_t     FBM;                 /* node feature bitmap for resource allocation */

  /* annotation */

  mmb_t        *MB;                 /* message buffer (alloc) */

  mln_t        *Variables;

  /* state booleans */

  mbool_t       Disable;            /* standing rsv should not spawn child rsvs */
  mbool_t       IsDeleted;

  /* behavior and actions */

  marray_t     *T;                  /* (alloc) */

  mjob_t       *SpecJ;              /* pointer to job that should be used for scheduling rsv */

  int           MaxIdleTime;        /* idle WC time allowed before reservation is released */
  int           MaxJob;             /* Max number of jobs allowed in reservation */
  double        MinLoad;            /* min load threshold for auto-cancellation */

  enum MVMUsagePolicyEnum VMUsagePolicy;  /**< virtual machine usage (only used for period jobs) */

  int           ReqOS;
  } msrsv_t;



/* dynamic attribute (ie, software, os, etc) */

typedef struct mda_s {
  mbool_t        IsActive;  /* is currently in use */
  int            Size;      /* quantity of resources consumed (-1 -> dedicated) */
  long           EATime;    /* estimated availability time */
  char          *AName;     /* attribute name (alloc) */
  void          *Consumer;  /* object requiring attribute */
  } mda_t;




#define MMAX_FSYS 32

typedef struct mfsys_t {
  int            BestFS;

  char          *Name[MMAX_FSYS];     /* (alloc) */

  int            RJCount[MMAX_FSYS];  /* jobs reading fs */
  int            WJCount[MMAX_FSYS];  /* jobs writing fs */
  int            OJCount[MMAX_FSYS];  /* jobs utilizing fs (other) */

  int            CSize[MMAX_FSYS];  /* configured size in MB */
  int            ASize[MMAX_FSYS];  /* available size in MB */
  double         CMaxIO[MMAX_FSYS]; /* configured max I/O */
  double         AIO[MMAX_FSYS];    /* available I/O */
  } mfsys_t;


/* node object (sync w/MNodeAttr[]) */

enum MNodeAttrEnum {
  mnaNONE = 0,
  mnaAccess,           /* access policy */
  mnaAccountList,
  mnaAvlGRes,          /* available generic resource */
  mnaAllocRes,         /* resources node has allocated */
  mnaAllocURL,
  mnaArch,
  mnaAvlClass,
  mnaAvlMemW,          /* priority weight */
  mnaAvlProcW,
  mnaAvlTime,
  mnaCCodeFailure,     /* CCode failure rate, see N->JobCCodeFailureCount */
  mnaCCodeSample,      /* CCode failure samples, N->JobCCodeSampleCount */
  mnaCfgClass,
  mnaCfgDiskW,
  mnaCfgMemW,
  mnaCfgProcW,
  mnaCfgSwapW,
  mnaCfgGRes,          /* configured generic resource */
  mnaChargeRate,
  mnaClassList,
  mnaComment,
  mnaDedGRes,
  mnaDynamicPriority,
  mnaEnableProfiling,
  mnaExtLoad,
  mnaFailure,           /* imposed failure condition */
  mnaFeatures,
  mnaFlags,
  mnaGEvent,            /* generic node event (ie failure, reboot, etc) */
  mnaGMetric,           /* generic performance metric (temperature, throughput, etc) */
  mnaGroupList,
  mnaHopCount,
  mnaHVType,            /**< hypervisor type */
  mnaIndex,             /* node table index */
  mnaIsDeleted,
  mnaJobList,
  mnaJPriorityAccess,
  mnaJTemplateList,
  mnaKbdDetectPolicy,
  mnaKbdIdleTime,       /* time since last detected keyboard or mouse activity */
  mnaLastUpdateTime,    /* time since last update via a resource manager */
  mnaLoad,
  mnaLoadW,
  mnaLogLevel,
  mnaMaxJob,
  mnaMaxJobPerAcct,
  mnaMaxJobPerGroup,
  mnaMaxJobPerUser,
  mnaMaxLoad,
  mnaMaxPE,
  mnaMaxPEPerJob,
  mnaMaxProc,
  mnaMaxProcPerAcct,
  mnaMaxProcPerClass,
  mnaMaxProcPerGroup,
  mnaMaxProcPerUser,
  mnaMaxWatts,
  mnaMaxWCLimitPerJob,
  mnaMessages,        /* new-style messages - mmb_t */
  mnaMinPreemptLoad,
  mnaMinResumeKbdIdleTime,
  mnaNetAddr,
  mnaNodeID,
  mnaNodeIndex,
  mnaNodeState,
  mnaNodeSubState,
  mnaNodeType,
  mnaOldMessages,     /* old-style messages - char* */
  mnaOperations,      /* adaptive operations supported by node */
  mnaOS,              /* current active operating system */
  mnaOSList,
  mnaResOvercommitFactor,
  mnaAllocationLimits,       /* alias for mnaResOvercommitFactor */
  mnaResOvercommitThreshold, /* includes/implies GMetricOvercommitThreshold */
  mnaUtilizationThresholds,  /* alias for mnaResOvercommitThreshold */
  mnaOwner,
  mnaPartition,
  mnaPowerIsEnabled,
  mnaPowerPolicy,
  mnaPowerSelectState,   /**< Moab specified power state */
  mnaPowerState,         /**< RM specified power state (no checkpoint) */
  mnaPreemptMaxCPULoad,
  mnaPreemptMinMemAvail,
  mnaPreemptPolicy,
  mnaPrioF,
  mnaPriority,
  mnaPrioW,
  mnaProcSpeed,
  mnaProvData,       /**< provisioning data (NextOS,OSMTime,EndTime) */
  mnaProvRM,         /**< provisioning RM */
  mnaQOSList,
  mnaRack,
  mnaRADisk,
  mnaRAMem,
  mnaRAProc,
  mnaRASwap,
  mnaRCDisk,
  mnaRCMem,
  mnaRCProc,
  mnaRCSwap,
  mnaRDDisk,
  mnaRDMem,
  mnaRDProc,
  mnaRDSwap,
  mnaReleasePriority,
  mnaRsvCount,
  mnaRsvList,    /* list of reservations */
  mnaRMList,
  mnaRMMessage,
  mnaSize,
  mnaSlot,
  mnaSpeed,
  mnaSpeedW,
  mnaStatATime,
  mnaStatMTime,
  mnaStatTTime,
  mnaStatUTime,
  mnaTaskCount,
  mnaTrigger,
  mnaUsageW,
  mnaUserList,
  mnaVariables,
  mnaVMOSList,
  mnaVarAttr,
  mnaLAST };

#define MDEF_NODEATTRSIZE MBMSIZE(mnaLAST)


/*
 * node flags
 *
 * @see MNodeFlags[] (sync)
 * @see struct mnode_t
 * @see enum MNodeIFlagEnum (peer) 
 *
 * NOTE:  use bmset(), bmisset() to manage
 */

enum MNodeFlagEnum {
  mnfNONE = 0,
  mnfGlobalVars,      /**< node has variables available for triggers */
  mnfNoVMMigrations,  /**< node is excluded from all VM migration decisions */
  mnfLAST };


/** 
 * node internal flags 
 *
 * @see enum MNodeFlagsEnum (peer)
 * @see struct mnode_t.IFlags
 *
 * NOTE:  no sync
 * NOTE:  mnifCan* set in MNodeUpdateOperations() based on policy and config
 */

enum MNodeIFlagEnum {
  mnifNONE = 0,
  mnifLoadedFromCP,      /**< set if the node was found in the checkpoint file */
  mnifCanCreateVM,       /**< VM's can be dynamically created/destroyed on this node  */
  mnifCanMigrateVM,
  mnifRMReportsWatts,    /**< set if any external RM reports the power state */
  mnifRMReportsPower,    /**< set if any external RM reports watt usage */
  mnifPurge,             /**< purge node in MNodeCheckStatus() */
  mnifMemOverride,       /**< node memory explicitly specified via config */
  mnifSwapOverride,      /**< node swap explicitly specified via config. NODECFG[] RCSWAP=#^ */
  mnifCanModifyOS,
  mnifCanModifyPower,
  mnifIsVM,            /**< node is pseudo-node representing VM */
  mnifKbdDrainActive,  /**< true if node is marked drained due to kdb activity */
  mnifIsPref,          /**< TEMP                                   */
  mnifIsNew,           /**< node discovered on this iteration, set in MRMNodePreLoad(), cleared in MNodePostLoad() */
  mnifIsDeleted,
  mnifSrcVMMigrate,      /**< node was source for a VM migrate, don't use as a destination for this iteration */
  mnifMultiComputeRM,    /**< node has multiple master compute RM's (internal) */
  mnifRMClassUpdate ,    /**< rm has updated class info */
  mnifRMDetected,        /**< node has been reported by RM */
  mnifRTableFull,        /**< R, RE or other internal table is full (do not checkpoint) */
  mnifIsTemp,            /**< node is temporary place-holder node - not real, not in MNode[] table */
  mnifLAST };



/* image object (sync w/MImageAttr[]) */

enum MImageAttrEnum {
  miaNONE = 0,
  miaOSList,
  miaReqMem,
  miaVMOSList,           /* VM OS list */
  miaLAST };

#define MDEF_NODEIFLAGSIZE MBMSIZE(mnifLAST)

/* node usage and preemption policies */

typedef struct mnpolicy_t {
  int      MaxJobPerUser;     /* */
  int      MaxProcPerUser;    /* */
  int      MaxJobPerGroup;    /* */
  int      MaxProcPerGroup;   /* */
  int      MaxProcPerClass;   /* */
  double   MaxPEPerJob;       /* */

  enum MPreemptPolicyEnum PreemptPolicy;  /* preemption method */

  int      MaxProcPerJob;

  double   PreemptMaxCPULoad;  /* preempt jobs if cpu load exceeded */
  double   PreemptMinMemAvail; /* preempt jobs if available memory drops below threshold */
  } mnpolicy_t;

/* Hardware Accelerators */

#define MCONST_GPUS               "GPUS" /* string for the gres type to be used with gpus */
#define MCONST_MICS               "MICS" /* String for the Intel MIC's */

/* Various object sizes */

#define MMAX_NODEOSTYPE           256 /* max number of potential target OS's in node's OS list */
#define MMAX_CCODEARRAY           5  /* Size of the JobCCodeFailureCount and JobCCodeSampleCount arrays */
#define MMAX_CCODESAMPLE          20 /* Number of samples allowed at each index of JobCCodeSampleCount */
#define MMAX_NODECHARGERATE       999999999 /* maximum chargerate allowed for a node */

/* sync w/MAttrAffinityType[] */

enum MAttrAffinityTypeEnum {
  maatNONE = 0,
  maatMust,
  maatShould,
  maatShouldNot,
  maatMustNot,
  maatLAST };

typedef std::multimap<std::string,std::string> AttrTable;
typedef std::multimap<std::string,std::string>::const_iterator attr_iter;

/**
 * @struct mnode_t
 *
 * node structure
 *
 * @see MDEF_MAXNODE, MDEF_JOBMAXNODECOUNT
 *
 * @see enum MNodeFlagEnum
 */

struct mnode_t {
  char     Name[MMAX_NAME + 1]; /**< name of node                               */
  int      Index;               /**< index in node array                        */

  char    *FullName;            /**< fully qualified host name (alloc)          */

  mulong   CTime;               /**< time node record was created               */
  mulong   ATime;               /**< time of most recent RM node update (last time RM reported node) */
  mulong   MTime;               /**< time node structure was modified (basically the same as ATime as 
                                     we assume the node is updated every iteration (see MRMNodePostUpdate) */
  int      MIteration;          /**< iteration node structure was loaded/updated from rm */

  int      LogLevel;            /**< node specific logging verbosity            */

  long     RsvMTime;
  mulong   StateMTime;          /**< time node changed state                    */

  int      HopCount;            /**< how many "hops" (between peers) this node has traveled */

  enum MNodeStateEnum RMState[MMAX_RM];  /**< rm specific state */

  enum MNodeStateEnum State;     /**< effective node state                      */
  enum MNodeStateEnum EState;    /**< expected node state resulting from sched action */
  enum MNodeStateEnum SpecState; /**< moab specified state */
  enum MNodeStateEnum OldState;
  enum MNodeStateEnum NewState;

  enum MPowerStateEnum PowerState;   /**< node power state reported by RM */
  enum MPowerStateEnum PowerSelectState; /**< node power state requested by Moab power policy */

  enum MPowerPolicyEnum PowerPolicy; /**< node power policy */

  char    *SubState;          /**< (alloc) */

  char    *LastSubState;      /**< last substate reported (alloc) */
  mulong   LastSubStateMTime; /**< time LastSubState was modified */

  mulong   SyncDeadLine;      /**< time by which state and estate must agree   */

  mcres_t  DRes;              /**< dedicated resources (dedicated to consumer) */
  mcres_t  CRes;              /**< configured resources (includes any overcommit factor) */
  mcres_t *RealRes;           /**< configured resources (before applying overcommit factor - alloc) */
  mcres_t  ARes;              /**< available resources - SHOULD NOT BE AFFECTED BY OVERCOMMIT VALUE */
  mcres_t  URes;              /**< utilized resources */
  mcres_t *XRes;              /**< externally used resources */

  double  *ResOvercommitFactor;  /**< resource type overcommit factor - see MResLimitType (alloc) */
                              /**< does not modify configured resources - see Node{Mem,CPU}OverCommitFactor */
  double  *ResOvercommitThreshold;  /**< resource type overcommit threshold - see MResLimitType (alloc) */

  double  *GMetricOvercommitThreshold; /**< thresholds for each gmetric - (alloc sizeof MSched.M[mxoxGMetric]) */

  int      PtIndex;           /**< partition assigned to node                  */

  mbitmap_t Flags;            /**< node meta flags (bitmap of enum MNodeFlagEnum) */
  mbitmap_t IFlags;           /**< internal flags (BM of enum MNodeIFlagEnum) */

  /* move booleans below to IFlags (NYI) */

  mbool_t  ProfilingEnabled;  /* node specific profiling enabled        */
  mbool_t  PowerIsEnabled;    /* node is powered up (deprecated? see N->PowerState) */

  mbitmap_t FBM; /**< available node features (BM)              */

  mbitmap_t Classes; /**< configured classes */

  int      Arch;              /**< node hardware architecture (index into MAList[meArch]) */

  mnodea_t *OSList;           /**< potential operating systems (alloc,size=MMAX_NODEOSTYPE) */
  int       ActiveOS;         /**< active node operating system (index into MAList[meOpsys]) */
  int       NextOS;           /**< set when provisioning system job is started (index into MAList[meOpsys]) */
  mulong    LastOSModRequestTime; /**< time last OS change was requested */
  int       LastOSModIteration;   /**< Iteration last OS change was completed */
  mnodea_t *VMOSList;         /**< potential VM operating systems (alloc,size=MMAX_NODEOSTYPE) */

  enum MHVTypeEnum HVType;    /**< hypervisor technology */

  mre_t   *RE;                /**< reservation event table (alloc,size=RESize) */

  /* cpu attributes */

  double   Speed;             /**< relative processing speed of node           */
  int      ProcSpeed;         /**< proc speed in MHz                           */

  double   Load;              /**< total node 1 minute CPU load average        */

  double   MaxLoad;           /**< max total load allowed                      */

  /* NOTE:  all storage node attributes in RMData */

  /* extended load indices (including gmetrics) */

  mxload_t *XLoad;            /* network, swap, GMetric, and I/O load (alloc with MNodeInitXLoad())  */
  mxload_t *XLimit;           /* (alloc) */

  mgevent_list_t  GEventsList;    /* List of mgevent_obj_t items (alloc elements, add with GEventAddItem ) */

  /* usage statistics */

  mulong   STTime;            /* time node was monitored (in 1/100's)        */
  mulong   SUTime;            /* time node was available (in 1/100's)        */
  mulong   SATime;            /* time node was active    (in 1/100's)        */

  int      TaskCount;         /**< number of tasks currently allocating node */
  int      EProcCount;

  int      NodeIndex;         /* node index - 0 to MAXNODE                   */
  int      RackIndex;         /* rack index                                  */
  int      SlotIndex;         /* slot index                                  */
  int      SlotsUsed;

  mrm_t   *RM;                /* master resource manager                     */
  mrm_t  **RMList;            /* (alloc,terminated w/NULL pointer,@see RMDoc)*/
  mrm_t   *ProvRM;            /* provisioning RM (pointer)                   */

  mbitmap_t RMAOBM;           /* attribute owner - MNodeAttrEnum */

  macl_t  *ACL;               /* node ACL                                   */

  /* NOTE:  make NodeType dynamically allocated (NYI) */

  char    *NodeType;  /* used for allocation management purposes (alloc) */

  int      IterationCount;    /* iterations since last RM update             */
  int      Priority;          /* node allocation priority (admin specified)  */
  int      PrioritySet;       /* TRUE if node has it's own priority, FALSE to use DefaultN's priority */

  int      MaxJCount;         /* maximum number of jobs supported on node    */
                              /* NOTE:  MaxJCount defines size of JList      */
  struct mjob_t   **JList;    /**< list of active and suspended jobs (alloc) */
                              /**< NOTE:  JList does not include reserved jobs (@see MNodeGetSysJobs()) */
  int     *JTC;               /* list of job taskcounts (alloc)              */

#if 0
  void    *RMData;            /* mnat_t * contains URL's
                                 (alloc or ptr - see RMDAta->IsNodeCentric) */
#endif

  enum MResourceAvailabilityPolicyEnum NAvailPolicy[mrLAST];  /* node availability policy (36 bytes) */

  long     KbdIdleTime;       /* time duration since keyboard/mouse activity detected (in seconds) */

  enum MNodeKbdDetectPolicyEnum KbdDetectPolicy; /* what to do in event of kbd activity */
  long     MinResumeKbdIdleTime; /* (duration in seconds) */
  double   MinPreemptLoad;
  char    *KbdSuspendedJobList; /* comma delimited list of jobs suspended due to kbd activity (alloc) */

  mnode_limits_t  AP;         /* active policy tracking (176 bytes) */

  mnpolicy_t *NP;             /* (alloc) */

  mfsys_t *FSys;              /* TEMP (alloc) filesystem list           */

  double   ChargeRate;        /* charge rate for this node              */
  int      ReleasePriority;   /* priority of this node to be released to UC ether (highest=release first) */

  mnprio_t *P;                /* node priority parameters (optional/alloc) */

  /* migrate all Message references to MB and remove Message in Moab 5.5 */

  char    *Message;           /* TEMP event message (alloc)             */

  mmb_t   *MB;                /* general message buffer (alloc)         */

  char    *RMMsg[MMAX_RM];    /* messages sent by rm (alloc)            */
  mbool_t  RMMsgNew[MMAX_RM];

  marray_t *T;                /* trigger list (optional/alloc) */

  mhash_t  Variables;         /* arbitrary node variables */

  AttrTable AttrList;

#if 0
  mln_t   *AttrList;          /* specified node attributes (not consumable,no checkpoint,alloc) */
                              /* NOTE: mln_t->BM != 0, attr reported by RM */
#endif

  mln_t   *AllocRes;          /* resources this node has allocated (checkpoint,alloc) */
                              /*   ie, VLAN node w/AllocRes=nodes in VLAN */

  int      SuspendedJCount;   /* number of suspended jobs currently on node */

  mnust_t  Stat;              /* historical usage statistics (1872 bytes)*/

  mcred_t  Cred;              /* make dynamic? (80 bytes) */

  mln_t   *VMList;            /**< list of child VM's w/Ptr-mvm_t (alloc) */
  int      VMOpTime;          /**< time of last VM operation */

  enum MNodeAccessPolicyEnum SpecNAPolicy; /* specified node access policy */
  enum MNodeAccessPolicyEnum EffNAPolicy;  /* effective node access policy */

  void    *xd;                /* extension data */

  char    *AttrString;        /* list of attributes specified by job (alloc) */
                              /* NOTE:  used for native RM only (ie, node attribute caching, not used in scheduling) */

  int   *JobCCodeFailureCount; /* count of how many consecutive jobs have had non-zero completion codes (alloc,size=MMAX_CCODEARRAY) */
  int   *JobCCodeSampleCount;  /* count of total jobs for each array bracket, including successful jobs (alloc,size=MMAX_CCODEARRAY) */

  mln_t  *Action;  /**< list of job id's for all active Node system jobs */
  char   *NetAddr;  /* Network address, for example IP address. (alloc) */

  mln_t *ParentVCs;         /* Parent VCs (alloc) - create only in MVCAddObject() */
  };


#ifndef MMAX_REQ_PER_JOB
#define MMAX_REQ_PER_JOB      32
#endif /* !MMAX_REQ_PER_JOB */

#define MMAX_TAPERNODE 1024  /* max tasks per node */


/**
 * multi-node list structure
 *
 * @see MUNLCopy()
 * @see MUMNLCopy()
 * @see MJobCopyNL()
 */

typedef struct mnpri_t {
  mnode_t *N;
  int      TC;
  double   Prio;
  } mnpri_t;

/**
 * Optional Requirement Extension
 *
 * @see mreq_t.OptReq
 */

typedef struct mgmreq_s {
  int    GMIndex;
  char   GMVal[MMAX_NAME];
  enum MCompEnum GMComp;
  } mgmreq_t;

typedef struct moptreq_t {
  char  *ReqService;         /* required service which must be available (alloc) */
  char  *ReqImage;           /* required environment/virtual image for execution (alloc) */

  /* required generic metrics */

  mln_t *GMReq;              /** linked list of generic metric requirements (alloc) */

  char  *ReqJID;             /* if set, indicates req is associated with co-alloc job (alloc) */
  } moptreq_t;



/**
 * Resource Preferences
 *
 * @see mreq_t
 */

typedef struct mpref_t {
  mbitmap_t FeatureBM; /**< preferred feature (BM)           */
  mln_t   *Variables;                     /**< preferred node variables (alloc) */
  mcres_t  CRes;                          /**< preferred configured resources   */
  mcres_t  ARes;                          /**< preferred available resources    */
  } mpref_t;


typedef struct RequestedAttribute_t {
  enum MAttrAffinityTypeEnum Affinity;
  std::string Name;
  std::string Value;
  enum MCompEnum Comparison;
  } RequestedAttribute_t;
  

typedef std::vector<RequestedAttribute_t> ReqAttrArray;

/**
 * @struct mreq_t 
 *
 * job req/taskgroup
 *
 * Defines requirements for objects in Moab - Jobs, reservations, etc.
 *
 * @see struct mjob_t - parent
 */

typedef struct mreq_t {
  /* NOTE:  req name required to allow rm's to specify task allocation
            to named req's
  */

  /* char Name[MMAX_NAME]; */

  int    Index;

  char  *ReqJID;             /**< if set, indicates req is associated with co-alloc job (alloc) */
  int    ReqNR[mrLAST];      /**< required resource attributes (ie, cfg mem >= X) */
  char   ReqNRC[mrLAST];     /**< required resource attribute comparison */

  mbitmap_t ReqFBM; /**< required feature (BM)                  */
  mbitmap_t SpecFBM; /**< specified feature (BM)                 */
  mbitmap_t ExclFBM; /**< excluded feature (BM)                  */

  mpref_t *Pref;             /**< resource preferences (alloc) */

  enum MCLogicEnum ReqFMode; /**< required feature logic (and or or)          */
  enum MCLogicEnum ExclFMode; /**< required feature logic (and or or)         */

  enum MResourceSetSelectionEnum SetSelection; /**< one of NONE, ONEOF, ANYOF */
  enum MResourceSetTypeEnum SetType; /**< one of NONE, Feature, Memory, ProcSpeed */
  mbool_t SetBlock;          /**< block on nodeset */
  long SetDelay;             /**< how long to block on nodeset */

  int  SetIndex;             /**< resource set selected w/in MJobAllocMNL() */

  char **SetList;            /**< list of set delineating job attributes (alloc) */

  mln_t  *NestedNodeSet;     /**< used to recursively search nodesets */

  int    Opsys;              /**< required OS (MAList[meOpsys])         */
  int    Arch;               /**< HW arch (MAList[meArch])              */

  moptreq_t *OptReq;         /**< optional extension requirements (alloc) */

  int    PtIndex;            /**< assigned partition index                    */

  mcres_t  DRes;             /**< dedicated resources per task (active)       */
  mcres_t *SDRes;            /**< dedicated resources per task (suspended)    */
  mcres_t *CRes;             /**< configured resource requirements            */

  /* NOTE:  URes.Procs and LURes.Procs are cpuload * 100 */

  mcres_t *URes;             /**< utilized resources per task (instant)       */
  mcres_t *LURes;            /**< req lifetime utilized resources per task    */
  mcres_t *MURes;            /**< maximum utilized resources per task         */

  mxload_t *XURes;           /**< utilized extension resources (alloc)        */

  mulong RMWTime;            /**< req walltime as reported by RM              */

  /* index '0' is active, indices 1 through MMAX_SHAPE contain requests */

  int    TaskRequestList[MMAX_SHAPE + 1];

  int    NodeCount;          /**< nodes allocated */
  int    TaskCount;          /**< tasks allocated */

  int    TasksPerNode;       /**< (0 indicates TPN not specified) */
  int    BlockingFactor;

  int    NodeSetCount;       /**< tell how to span with node sets (0 is unset) */

  int    MinProcSpeed;       /**< minimum proc speed for this req */

  enum MNodeAccessPolicyEnum NAccessPolicy; /**< node access policy */
  enum MNodeAllocPolicyEnum  NAllocPolicy;

  int    RMIndex;       /**< index of DRM if set, otherwise index of SRM (-1 if not set) */

#if 0
  char *Label;          /**< req label (alloc) */
#endif

  /* used with co-alloc jobs */

  enum MJobStateEnum State;   /**< state of job-part associated with this req */
  enum MJobStateEnum EState;  /**< expected state of job-part associated with this req */

  mrsv_t    *R;         /**< req specific reservation (only used w/multi-rm jobs) */
  mrrsv_t   *RemoteR;   /**< req specific remote reservation (only used w/multi-rm jobs) */

  mnl_t      NodeList;

  ReqAttrArray ReqAttr;

#if 0
  mln_t     *ReqAttr;   /**< required node attributes (alloc) */
                        /**< old variable "NamedGRes" may have been moved to ReqAttr as well,
                             name=gres:label */
#endif

  /* NOTE:  may replace NamedGRes with ReqAttr (???) */

  int ReqMem;           /**< total required memory for job, not per task */

  int OldReqTC;         /**< holder for storing previous TaskCount while being modified */
  msnl_t CGRes;         /* configured generic resources */
  msnl_t AGRes;         /* available generic resources */

  /* For VC scheduling */
  int ParentReqIndex;   /* Req index of req this req is a copy of */
  char *ParentJobName;  /* Job whose req this req is a copy of */
  } mreq_t;


typedef struct mrsrc_t {
  int TC;
  int NC;
  } mrsrc_t;

/* NOTE: Removing MMAX_JOBRA for preemption.  MMAX_PJOB will now be used instead see MMAX_RALSIZE */

/*#define MMAX_JOBRA    8*/


/* sync w/MJTXAttr[] and struct mjtx_t */

enum MJTXEnum {
  mjtxaNONE = 0,
  mjtxaTemplateDepend,  /**< TX object, dependencies */
  mjtxaReqFeatures,     /**< Job, Reqs */
  mjtxaGRes,            /**< Job, GRes */
  mjtxaVariables,       /**< Job, variables */
  mjtxaWallTime,        /**< TX object, walltime array */
  mjtxaResources,       /**< TX object, task definition */
  mjtxaSampleCount,     /**< TX object, sample count array */
  mjtxaMemory,          /**< TX object, memory usage array */
  mjtxaLAST };

#define MDEF_JOBTXBMSIZE MBMSIZE(mjtxaLAST)


/* job template set actions */

/* sync w/MJTSetAction[] */

enum MJTSetActionEnum {
  mjtsaNONE = 0,
  mjtsaHold,
  mjtsaCancel,
  mjtsaLAST };


/* allow specification of job load, ie, avgload=X*IO + Y*MEMUSAGE + Z*SWAP */
/* very similar to node priority */


/**
 * Job flags for job templates
 *
 * sync w/ MTJobFlags[]
 *
 * @see mjtx_t.TemplateFlags
 */

enum MTJobFlagEnum {
  mtjfNONE = 0,
  mtjfGlobalRsvAccess,
  mtjfIsHidden,
  mtjfIsHWJob,
  mtjfPrivate,
  mtjfSyncJobID,
  mtjfTemplateIsDynamic,         /**< should always be set for dynamic templates */
  mtjfTemplateIsSelectable,      /**< if a job can select this template */
  mtjfLAST };


#define MMAX_PREEMPTJLIST  32
#define MMAX_LDEPTH         5
#define MDEF_LSAMPLES      32
#define MDEF_JOBTFLAGSIZE MBMSIZE(mtjfLAST)
/* @see MMIN_APPLSAMPLES */

/**
 * sync w/ MTJobActionTemplate[]
 *
 * Actions that can cause another job to be created from a template.
 */

enum MTJobActionTemplateEnum {
  mtjatNONE = 0,
  mtjatDestroy,
  mtjatMigrate,
  mtjatModify,
  mtjatLAST };

/**
 * Job Template Extension Structure
 *
 * @see enum MJTXEnum (sync)
 */

typedef struct mjtx_t {
  /* job/application performance (see mstaAppBacklog, etc for stats) */

  enum MJobTemplateFailurePolicyEnum FailurePolicy;

  /* resource allocation limits */

  char  *SpecTrig[MMAX_SPECTRIG]; /* (alloc) */
  char  *SpecTemplates;           /* (alloc) */

  char  *SpecArray;           /* (alloc) */
  char  *SpecMinPreemptTime;  /* (alloc) */

  /* NOTE:  should support min and max alloc sizes - min to make efficient, max to prevent overwhelming
            provisioning infrastructure (NYI) */

  enum MJTSetActionEnum SetAction; /* Job set template action */

  mrsv_t *R;           /* reservation for future resources (pointer) */

  char   *WorkloadRMID; /* job management RM id - destination RM for job migration (alloc) */
  mrm_t  *WorkloadRM;   /* pointer to workloadrm */

  mln_t     *Depends;      /* (alloc?) */

  mln_t     *Action;  /**< list of job id's for all active pseudo-job system jobs */

  mbitmap_t SetAttrs;
  mbitmap_t TemplateFlags;    /**< job template flags (bm of enum MTJobFlagEnum) */

  /* The follwoing six elements are for RM extension requests, taken from mrmx_t (deprecated) */
  marray_t   Signals;

  mstring_t  ReqSetTemplates;
  mbool_t    BlockingTaskDistPolicySpecified;

  /* Template actions */

  char      *DestroyTemplate;  /* Job to call, attach to workflow when this job is cancelled/destroyed (alloc) */
  char      *ModifyTemplate;   /* Job to call, put in workflow VC when job is modified (alloc) (NYI - placeholder) */
  char      *MigrateTemplate;  /* Job to call, put in workflow VC when job is migrated (alloc) (NYI - placeholder) */

  char      *JobReceivingAction; /* The job that this job is performing an action for */
  enum MTJobActionTemplateEnum TJobAction; /* The action that this job is performing for JobReceivingAction */
  } mjtx_t;

#define MDEF_VARVALBUFCOUNT 8


#define MDEF_MAXUSERPRIO 1023
#define MDEF_MINUSERPRIO -1024


/**
 * Client modes
 *
 * NOTE:  max count is 32
 *
 * sync w/MClientMode[]
 */
enum MCModeEnum {
  mcmNONE = 0,
  mcmBlock,
  mcmClear,
  mcmComplete,
  mcmCP,
  mcmExclusive,
  mcmForce,
  mcmForce2,
  mcmFuture,
  mcmGrep,
  mcmJobName,
  mcmLocal,
  mcmNonBlock,
  mcmOverlap,
  mcmParse,
  mcmPeer,
  mcmPersistent,
  mcmPolicy,
  mcmRelative,
  mcmSort,
  mcmSummary,
  mcmTimeFlex,
  mcmUser,
  mcmUseTID,
  mcmVerbose,
  mcmVerbose2, /* used for extra-super verbosity */
  mcmXML,
  mcmLAST };

#define MDEF_CLIENTBMSIZE MBMSIZE(mcmLAST)


/**
 * system job attributes
 *
 * sync w/MJobSysAttr[]
 * @see designdoc SystemJobDoc
 * @see mjob_t.System (sync)
 */

enum MJobSysAttrEnum {
  mjsaNone = 0,
  mjsaAction,
  mjsaCFlags,
  mjsaCompletionPolicy,
  mjsaDependents,
  mjsaFAction,
  mjsaICPVar1,
  mjsaJobToStart,
  mjsaJobType,
  mjsaRetryCount,
  mjsaSAction,
  mjsaSCPVar1,
  mjsaProxyUser,
  mjsaVM,
  mjsaVMDestinationNode,
  mjsaVMSourceNode,
  mjsaVMToMigrate,
  mjsaWCLimit,
  mjsaLAST };


/**
 * @struct msysjob_t
 *
 * system job specific data structure dynamically allocated and associated with system job
 * specific tasks.
 *
 * @see mjob_t.System
 * @see MJobFreeSystemStruct() - deep free msysjob_t structure (sync)
 * @see MJobShowSystemAttrs() - report msysjob_t system attributes (sync)
 * @see MJobSystemToXML() - sync
 * @see MJobSystemFromXML() - sync
 * @see enum MSystemJobTypeEnum
 */

#define MDEF_SYSJOBFLAGSIZE MBMSIZE(msjcaLAST)
#define MDEF_SYSJOBERRORFLAGSIZE MBMSIZE(msjefLAST)

typedef struct msysjob_t {
  enum MSystemJobTypeEnum JobType;
  mbool_t IsAdmin; /* specifies whether an administrator requested the system job or not */

  char *Action;   /* system job action */
  char *SAction;  /* system job success action */
  char *FAction;  /* system job failure action */

  int RetryCount; /* system job action retry count */

  enum MJobCompletionPolicyEnum CompletionPolicy;

  mbitmap_t CFlags; /**< BM of completion action flags MSystemJobCompletionActionEnum */
  mbitmap_t MCMFlags;  /** Moab Client Mode flags. See MCModeEnum */

  int   ICPVar1;           /* integer variable can be used to store data specific to CompletionPolicy */

  void *VCPVar1;           /* void pointer variable (no alloc) */

  char *SCPVar1;           /* string variable for data specific to CompletionPolicy (alloc) */

  char *ProxyUser;         /* record proxy user, if available (alloc) */

  char *VM;                /* VM to migrate (alloc) */
  char *VMSourceNode;      /* VM source node id (alloc)*/
  char *VMDestinationNode; /* VM destination node id (alloc)*/
  char *RecordEventInfo;   /* string to use when recording event (alloc) */

  int   WCLimit;           /* time systemjob should take to complete */

  mln_t *Dependents;       /* jobs that are depending on this job to complete, array of job ids (strings) */

  mln_t *Variables;

  mbitmap_t EFlags;   /* error flags */
  mvm_t **VMTaskMap; /* used in msjtVMCreate jobs. Filled in when job completes */
  char *VMJobToStart; /* used in msjtVMCreateJobs. Starts jobs when it's VMTaskMap is full */
#if 0
  mtrig_t *Trig; /* used to let system jobs be a thin trigger wrapper */
#endif
  mbool_t ModifyRMJob; /* used by storage jobs */

  marray_t   *TList;    /**< Generic System Job Trigger (really only supports one) */
  } msysjob_t;


typedef struct mjobgroup_t {
  char     Name[MMAX_NAME]; /**< job group name */
  mjob_t  *RealJob;         /**< real job (if job group is associated with real job) */

  int      ArrayIndex;      /**< index of job in job array */

  mln_t  *SubJobs;
  } mjobgroup_t;


typedef struct mhist_t {
  mulong  Time;

  char   *CmdLine;

  enum MObjectSetModeEnum Mode;
  mcres_t Delta;

  struct mhist_t *Next;
  } mhist_t;


/**
 * Cache object, thread library dependent.
 */

typedef struct mcache_t {
  mrwlock_t  JobLock;      /* reader/writer lock for jobs. */
  mrwlock_t  NodeLock;     /* reader/writer lock for nodes. */
  mrwlock_t  RsvLock;      /* reader/writer lock for reservations. */
  mrwlock_t  TriggerLock;     /* reader/writer lock for triggers. */
  mrwlock_t  VCLock;       /* reader/writer lock for VCs. */
  mrwlock_t  VMLock;      /* reader/writer lock for VMs. */

  mhash_t Jobs;           /* Hash table for all job entries. */
  mhash_t Nodes;          /* Hash table for all nodes entries. */
  mhash_t Rsvs;           /* Hash table for all reservation entries. */
  mhash_t Triggers;       /* Hash table for all trigger entries. */
  mhash_t VCs;            /* Hash table for all VC entries. */
  mhash_t VMs;            /* Hash table for all VM entries. */
  } mcache_t;



/**
 * Internal job flags (no string array to sync with).
 *
 * These flags can only be accessed or set by Moab, unlike MJobFlagEnum.
 *
 * IFlags are not checkpointed. The policy is that an iflag should be something
 * that will be set again after a restart.
 *
 * @see enum MJobFlagEnum
 * @see mjob_t.IFlags
 */

enum MJobIFlagEnum {
  mjifNONE = 0,
  mjifBFJob,                    /**< internal BFWindow pseudo job */
  mjifIsRMCheckpointable,       /**< job can be checkpointed by RM */
  mjifDataDependency,           /**< job has outstanding data dependency that must be fulfilled */
  mjifDataOnly,                 /**< saves submit string of this job to a spool file and doesn't submit it */
  mjifFlexDuration,             /**< job has flexible duration */
  mjifHasCreatedEnvironment,    /**< adaptive job has created its environment */
  mjifHasInternalDependencies,  /**< job has internal dependencies and must checkpoint them */
  mjifHostList,                 /**< job has hostlist specified */
  mjifIgnRsv,                   /**< job is rsv map and should ignore other reservations */
  mjifIgnGhostRsv,
  mjifIsDynReq,                 /**< job is dynamic job request */
  mjifIsEligible,               /**< job is eligible for scheduling on this iteration */
  mjifIsExiting,                /**< job has begun exit process (cannot be cancelled) */
  mjifIsMultiRMJob,             /**< job utilizes resources from multiple rm's */
  mjifIsNew,                    /**< job discovered on this iteration */
  mjifIsTemplate,               /**< job is template job only (ie, C->JMin) */
  mjifNodesSpecified,
  mjifPBSCPPSpecified,          /**< 'cpp=' specified */
  mjifPBSNodesSpecified,        /**< requested 'nodes' attribute specified */
  mjifPBSPPNSpecified,          /**< 'ppn=' specified */
  mjifPBSGPUSSpecified,         /**< 'gpus=' specified */
  mjifPBSNCPUSSpecified,        /**< 'ncpus=' specified */
  mjifPBSMICSSpecified,         /**< 'mics=' specified */
  mjifPendingDynRequest,        /**< new dynamic request must be processed */
  mjifPreemptCompleted,         /**< job has preempted other resources and is attempting to utilize the freed resources */
  mjifPreemptFailed,            /**< job preemption attempt on this job failed this iteration */
  mjifRanEpilog,                /**< job's epilog was run */
  mjifRanProlog,                /**< job's prolog was run */
  mjifReqTimeLock,              /**< job has rigid time association with dependency */
  mjifReqSpaceLock,             /**< job has rigid space association with dependency */
  mjifRsvMap,                   /**< job is reservation based */
  mjifRsvMapIsExclusive,        /**< job represents an exclusive reservation */
  mjifRULHPActionTaken,         /**< resource utl limit hard policy action taken */
  mjifRULSPActionTaken,         /**< resource utl limit soft policy action taken */
  mjifStartPending,             /**< job has reserved policy slots to start this iteration */
  mjifTaskCountIsDefault,       /**< task is not explicity specified and is default */
  mjifTasksSpecified,           /**< task count explicitly specified by job (no inherited defaults) */
  mjifWCNotSpecified,           /**< job's wallclock not explicitly specified (default loaded) */
  mjifAccountLocked,            /**< acct cannot be changed except by explicit admin action */
  mjifAllocPartitionCreated,    /**< alloc partition has been created for this job and should be managed */
  mjifClassLocked,              /**< class cannot be changed except by explicit admin action */
  mjifCopyTaskList,             /**< job requires TaskMap to be copied over RM-reported TaskList */
  mjifDestroyDynamicStorage,    /**< job should destroy dynamic storage when complete */
  mjifDestroyDynamicVM,         /**< job should destroy dynamic VM when complete */
  mjifEnvSpecified,             /**< job specifies full environment */
  mjifExactTPN,                 /**< job should allocate exactly TPN tasks per node */
  mjifExecTemplateUsed,         /**< true if a template exec= statement has been applied */
  mjifExitOnTrigCompletion,     /**< job should complete once all triggers have fired */
  mjifFailedNodeAllocated,      /**< active job has one or more allocated resources fail */
  mjifFlexNodeAlloc,            /**< node allocation is flexible (based on # of procs avail) */
  mjifFSTreeProcessed,          /**< job's FSTREE access has been computed */
  mjifIsResuming,               /**< job is currently being scheduled via MQueueScheduleSJobs() */
  mjifIsRsvEval,                /**< job is currently being evaluated for future rsv */
  mjifIsServiceJob,             /**< job is a TORQUE service job (handled as a service by compute RM) */
  mjifIsValidated,              /**< job has been successfully validated */
  mjifMasterSyncJob,            /**< job is merged 'super-job' (synccount/syncwith) */
  mjifMaxNodes,                 /**< slurm has reported maxnodes */
  mjifMaxTPN,                   /**< job should allocate no more that TPN tasks per node */
  mjifMergeJobOutput,           /**< merge job stdout and stderr */
  mjifNewBatchHold,             /**< job has a new batch hold */
  mjifNoAllocVM,                /**< job does not allocate virtual machines */
  mjifNodeScaling,
  mjifNoGRes,
  mjifNoImplicitCancel,         /**< do not allow the job to be canceled implicitly (via canceljob ALL or via RM purge, etc.) */
  mjifNoViolation,              /**< no soft, active violation detected this iteration */
  mjifQOSLocked,                /**< QOS cannot be changed except by explicit admin action */
  mjifRemote,                   /**< grid job */
  mjifRequiresVM,               /**< job requires a virtual machine to run */
  mjifReserveAlways,            /**< job should always reserve needed resources - cannot be externally specified */
  mjifRMManagedDepend,          /**< job dependency managed via RM - RM will handle dependency constraints */
  mjifRunAlways,                /**< run job even in overcommit or policy violation situations (ie, break all rules and start job ignoring policies and limits - see mjifRunNow) cannot be externally specified */
  mjifRunNow,                   /**< run the job now (ie, adjust priority, preempt, etc - do what must be done to allow job to properly run - see mjifRunAlways) */
  mjifStalePartition,           /**< completed job has stale allocation partition */
  mjifSubmitEval,               /**< job is being evaluated at submit time */
  mjifUseDRMJobID,              /**< rename job to match dest rm jobid */
  mjifWasRequeued,              /**< job was requeued by Moab since last start */
  mjifWasCanceled,              /**< job was canceled (by user, admin, preemption, usage policy, walltime, etc) */
  mjifWCRequeue,                /**< on WClimit violation requeue job */
  mjifAllowProxy,               /**< allow proxy requested by job */
  mjifCancel,                   /**< cancel this job in MRMJobPostUpdate or MRMJobPostLoad */
  mjifClassSpecified,           /**< class explicitly specified via RM */
  mjifDProcsSpecified,          /**< job explicitly specifies dedicated procs. Set in MJobSetAttr with mjaSize when value == 0 and in MWikiJobUpdateAttr with mwjaDProcs. (ex. -l size=0) */
  mjifFBAcctInUse,              /**< job is using a fallback account */
  mjifFBQOSInUse,               /**< job is using a fallback qos */
  mjifGlobalRsvAccess,          /**< specified dynamic job can access all rsv's including system rsv's */
  mjifIgnoreNodeAccessPolicy,   /**< ignore node access policy (force to shared) */
  mjifIsArrayJob,               /**< job is in a job array */
  mjifIsHidden,                 /**< job is hidden to non-admins */
  mjifIsHWJob,                  /**< job is hardware-based pseudo-job */
  mjifIsPowerTemplate,          /**< job is power management template */
  mjifIsPreempting,             /**< job is currently trying to preempt other jobs */
  mjifJobEvalCheckStart,        /**< job was checked at submit time to see if it can ever be started */
  mjifNoTemplates,              /**< apply no job templates (except possibly specified template) */
  mjifPurge,                    /**< job has been removed by RM and should be purged at end of iteration */
  mjifReqIsAllocated,           /**< job req is dynamically allocated */
  mjifRMSpecifiedHold,          /**< job has a hold that was specified by the RM */
  mjifSharedResource,           /**< used when scheduling reservations */
  mjifSyncJobID,                /**< sync jobid w/jobname */
  mjifTemplateIsSelectable,     /**< job template can be directly requested by job at submission */
  mjifUIQ,                      /**< job is in the MUIQ */
  mjifUseLocalGroup,            /**< job should use primary group of destination cluster */
  mjifWaitForTrigLaunch,        /**< job should not be eligible for starting until at least one trigger attached to the job has launched */
  mjifWasLoadedFromCP,          /**< job was wholely/partially loaded from checkpoint file */
  mjifRequestForceCancel,       /**< user force cancel request */
  mjifNoResourceRequest,        /**< job requested no resources, use RM defaults */
  mjifMaster,                   /**< job can perform on-demand provisioning */
  mjifIsRsvJob,
  mjifVPCMap,                   /**< job is pseudo-job representing VPC */
  mjifAllocatedNodeSets,        /**< job's allocation has already been determined in MJobGetRangeValue(), skip MJobDistributeMNL() and MJobAllocMNL() */
  mjifNodeSetDelay,             /**< toggle to pass MJobAllocMNL on priority reservation to find best nodeset with delay. */
  mjifNotAdvres,                /**< if true, use any reservation but this one (ReqRID/PeerReqRID). */
  mjifSpecNodeSetPolicy,        /**< job specifies nodeset policy */
  mjifProlog,                   /**< job has a Moab specific Prolog */
  mjifIgnJobRsv,                /**< job is rsv map, ignore job reservations only */
  mjifStageInExec,              /**< job's exec needs to be staged in. Created for staging in job template dependency execs. */
  mjifAMReserved,               /**< TRUE if a reservation has already been made in the accounting manager for this job */
  mjifFairShareFail,            /**< job has had a cred combination that was not valid in the fairshare tree */
  mjifACLOverlap,               /**< TRUE if the reservation's ACLOverlap flag is set */
  mjifIgnoreNodeSets,           /**< TRUE if "spanevenly" is in use and job should ignore nodesets (see MJobGetRangeValue()) */
  mjifEnvRequested,             /**< -E was specified to pass MOAB_* env vars to job (see MJobGetEnvVars()). */
  mjifTemplateIsDynamic,        /**< TRUE if template was created through UI */
  mjifTemplateIsDeleted,        /**< this template has been deleted */
  mjifRequeueOnFailure,         /**< requeue this job if it fails */
  mjifSystemJobFailure,         /**< somehow this system job failed, clean up next iteration in MJobUpdateSystemJob() */
  mjifServiceJob,               /**< job is service job, submit to torque with -s y */
  mjifStorageAllocating,        /**< job's dynamic storage is currently allocating */
  mjifVMDestroyCreated,         /**< VMCreate job cancelled, a VMDestroy job has already been created */
  mjifClassSetByDefault,        /**< job's class was set to the default class for the scheduling partition */
  mjifQOSSetByDefault,          /**< job's qos was set to the default qos speecified in the fairshare tree  */
  mjifQOSSetByTemplate,          /**< job's qos was set by the job template */
  mjifGenericSysJob,            /**< this job will become a generic system job (set by JOBCFG[] GENSYSJOB=TRUE) */
  mjifInheritResources,         /**< if job was created via templatedepend, it will start of as a copy of the parent */
  mjifRMOwnsHold,               /**< the RM owns the hold on the job */
  mjifTemplateStat,             /**< job holds template stats */
  mjifTemporaryJob,             /**< not a real job, backfill, showstart, etc. */
  mjifShouldCheckpoint,         /**< when job checkpointing is optimized, this job should be checkpointed */
  mjifCompletedJob,             /**< job is a completed job and needs to be freed */
  mjifBillingReserved,          /**< the NAMI "reserve" URL was already called on this job */
  mjifBillingReserveFailure,    /**< the NAMI "reserve" call failed */
  mjifDestroyByDestroyTemplate, /**< job is being cancelled by a destroytemplate returning SUCCESS */
  mjifTRLRequest,               /**< job requested TRL */
  mjifCreatedByTemplateDepend,  /**< job was created by a job template dependency */
  mjifCheckCurrentOS,           /**< set if the job only wants to know about the current OS, nothing else */
  mjifNoEventLogs,              /**< do not write out events for this job */
  mjifLAST };


/**
 * RM extension attributes 
 *
 * @see const char *MRMXAttr[] (sync)
 * @see designdoc RMExtensionsDoc
 * @see mjob_t.RMX
 * @see mjob_t.RMXSetAttrs
 * @see mjob_t.RMXString
 */

enum MXAttrType {
  mxaNONE,
  mxaAdvRsv,         /* required reservation */
  mxaArray,          /* job array specification */
  mxaBlocking,       /* blocking factor */
  mxaCGRes,          
  mxaDeadline,       /* job completion deadline */
  mxaDependency,
  mxaDDisk,          /* per task dedicated disk (in MB) */
  mxaDMem,           /* per task dedicated mem (in MB) */
  mxaEnvRequested,   /* user requested (msub -E) that MOAB_* env variables be placed in job's env. */
  mxaEpilogueScript, /* per-job epilogue script */
  mxaExcHList,       /* nodes to be excluded from host list */
  mxaFeature,        /* required per job features */
  mxaFlags,          /* same as jobflags */
  mxaGAttr,          /* arbitrary job attribute */
  mxaGeometry,
  mxaGMetric,        /* generic metric constraint for allocated nodes */
  mxaGRes,           /* generic resources (floating or per task) */
  mxaHostList,
  mxaImage,          /* image for virtualization */
  mxaJobFlags,       /* same as flags */
  mxaJobRejectPolicy, /* Job level override for global JobRejectPolicy */
  mxaJobGroup,
  mxaLocalDataStageHead,  /* node where this job's stdout/stderr files should be copied back to */
  mxaLogLevel,       /* job tracking/reporting loglevel */
  mxaMaxJobMem,
  mxaMaxJobProc,
  mxaMinPreemptTime, /* see J->MinPreemptTime */
  mxaMinProcSpeed,
  mxaMinWCLimit,     /* see J->MinWCLimit */
  mxaMPPmem,
  mxaMPPhost,
  mxaMPParch,
  mxaMPPwidth,
  mxaMPPnppn,
  mxaMPPdepth,
  mxaMPPnodes,
  mxaMPPlabels,
  mxaMoabStageIn,    /* Moab-specific stage-in info (to avoid conflicts w/ PBS) */
  mxaMoabStageOut,   /* Moab-specific stage-out info (to avoid conflicts w/ PBS) */
  mxaNAccessPolicy,
  mxaNAllocPolicy,
  mxaNMatchPolicy,
  mxaNodeMem,        /* required config mem for all eligible node */
  mxaNodeScaling,
  mxaNodeSet,
  mxaNodeSetIsOptional,
  mxaNodeSetCount,
  mxaNodeSetDelay,
  mxaOS,             /* required operating system */
  mxaPlacement,
  mxaPMask,          /* partition mask */
  mxaPostStageAction,/* Perl script to be run remotely on the data storage after job and data staging*/
  mxaPostStageAfterType, /* Post stage action is run after this file has been staged out */
  mxaPref,
  mxaProcs,
  mxaProcsBitmap,    /* bitmap geometry request */
  mxaPrologueScript, /* per-job prologue script */
  mxaQOS,
  mxaQueueJob,
  mxaReqAttr,        /* required node attributes */
  mxaResFailPolicy,  /* resource failure policy */
  mxaRMType,         /* required resource manager type */
  mxaSelect,         /* mimic pbspro 'select' */
  mxaSGE,            /* request IBM's SGE hardware (obsolete!) */
  mxaSID,            /* system ID */
  mxaSignal,         /* pre-termination signal */
  mxaSize,           /* Cray's interpretation of size */
  mxaSJID,           /* system's job ID */
  mxaSRMJID,         /* source resource mananger's job ID */
  mxaStageIn,        /* job requires data stage-in services */
  mxaStageOut,       /* job requires data stage-out services */
  mxaStdErr,         /* where RM is storing stderr */
  mxaStdOut,         /* where RM is storing stdout */
  mxaTaskDistPolicy, /* job task distribution policy */
  mxaTemplate,       /* job 'set' template */
  mxaTermSig,        /* job termination signal */
  mxaTermTime,       /* job termination time */
  mxaTID,            /* use transaction to determine job attributes */
  mxaTPN,            /**< tasks per node */
  mxaTrig,           /**< triggers */
  mxaTrigNamespace,  /**< trigger namespaces */
  mxaTRL,            /**< task request list */
  mxaTTC,            /**< total task count */
  mxaUMask,          /**< Umask of job */
  mxaSPrio,          /**< system priority (sets J->SystemPrio) */
  mxaVariables,      /**< job variables */
  mxaVC,             /**< requested VC */
  mxaVMUsage,        /**< VM usage policy */
  mxaVPC,            /**< job's requested VPC */
  mxaVRes,           /**< virtual resource located on virtual node */
  mxaWCRequeue,      /**< how to handle WCLimit violation */
  mxaWCKey,          /**< special key for slurm resource manager */
  mxaLAST };


#define MDEF_RMEXTENSIONBMSIZE MBMSIZE(mxaLAST)


#define MDEF_JOBIFLAGSIZE MBMSIZE(mjifLAST)

/*
 * Requested GRES resources as a resoure manager extension
 */

typedef struct mreqgres_t {
  int Count;
  long TimeFrame;
} mreqgres_t;

/*
 * Requested signal triggers as a resource manager extension
 */

typedef struct mreqsignal_t {
  int Signo;
  mulong TimeOffset;
} mreqsignal_t;

typedef struct mreqvcores_t {
  int Count;
  int PCPVC; /* I have no idea what this stands for */
} mreqvcores_t;



/**
 * The enum of possible job attributes.
 *
 * @see MJobAttr[] in MConst.c (sync)
 * @see struct mjob_t (sync)
 * @see enum MReqAttrEnum (peer)
 *
 * @see MJobMoveAttr(),MJobDuplicate() - update when adding new attributes
 * @see MMJobSubmit() - update when adding new attributes that are sent
 *                      between Moab peers
 *
 * NOTE: mjaLAST must be < MMAX_S3ATTR
 */

enum MJobAttrEnum {
  mjaNONE = 0,
  mjaAccount,        /**< The account that the job should charge */
  mjaAllocNodeList,  /**< nodes allocated to job */
  mjaAllocVMList,    /**< VMs allocated to job */
  mjaArgs,           /**< job executable arguments */
  mjaArrayInfo,      /**< job array info (only used for master job) */
  mjaArrayLimit,     /**< limit jobs running in an array */
  mjaAWDuration,     /**< active wall time consumed */
  mjaBecameEligible, /**< time that the job switched from blocked to eligible */
  mjaBlockReason,    /**< block index of reason job is not eligible */
  mjaBypass,
  mjaCancelExitCodes,/**< job array cancelation exit codes */
  mjaClass,          /**< NOTE:  class credential, not req class initiator count */
  mjaCmdFile,        /**< command file path */
  mjaCompletionCode, /**< return code of job extract from RM */
  mjaCompletionTime,
  mjaCost,           /**< cost a executing job */
  mjaCPULimit,       /**< job CPU limit */
  mjaDepend,         /**< dependencies on status of other jobs */
  mjaDependBlock,    /**< block dependencies */
  mjaDescription,    /**< job description (for templates, service jobs, etc) */
  mjaDRM,            /**< master destination job rm */
  mjaDRMJID,         /**< destination rm job id */
  mjaEEWDuration,    /**< eff eligible duration:  duration job has been eligible for scheduling */
  mjaEffPAL,         /**< Effective partition access list */
  mjaEFile,          /**< stderr file */
  mjaEligibilityNotes,/**< list of blocked job eligibility notes rebuilt each iteration */
  mjaEnv,            /**< job execution environment variables */
  mjaEnvOverride,    /**< job execution environment override variables */
  mjaEpilogueScript, /**< epilogue scripts */
  mjaEState,         /**< expected state */
  mjaEstWCTime,      /**< estimated walltime job will execute */
  mjaExcHList,       /**< exclusion host list */
  mjaFlags,
  mjaGAttr,          /**< requested generic attributes */
  mjaGeometry,       /**< job requested arbitrary geometry */
  mjaGJID,           /**< global job id */
  mjaGroup,          /**< group cred */
  mjaHold,           /**< hold list */
  mjaHoldTime,       /**< time currently held job was put on hold */
  mjaHopCount,       /**< hop count of job between peers */
  mjaHostList,       /**< allocated hosts */
  mjaReqHostList,    /**< requested hosts */
  mjaIFile,          /**< stdin file */
  mjaImmediateDepend,/**< jobs to run immediately after job finishes, with same allocation */
  mjaInteractiveScript,/**< script to run while in interactive mode */
  mjaIsInteractive,  /**< job is interactive */
  mjaIsRestartable,  /**< job is restartable (NOTE: map into specflags) */
  mjaIsSuspendable,  /**< job is suspendable (NOTE: map into specflags) */
  mjaIWD,            /**< execution directory */
  mjaJobID,          /**< job batch id */
  mjaJobName,        /**< user specified job name */
  mjaJobGroup,       /**< job group id */
  mjaJobGroupArrayIndex, /**< job group array index */
  mjaLastChargeTime, /**< last time a job was charged to NAMI */
  mjaLogLevel,       /**< job's individual log level */
  mjaMasterHost,     /**< specified host to run primary task on */
  mjaMessages,       /**< new style message - mmb_t */
  mjaMinPreemptTime, /**< minimum time job must run before being eligible for preemption */
  mjaMinWCLimit,     /**< minimum wallclock limit for this job */
  mjaNodeAllocationPolicy, /** maps to J->Req[0]->NAllocPolicy->NAllocPolicy **/
  mjaNodeMod,        /**< used for template attr tracking only */
  mjaNodeSelection,  /**< node ordering criteria (i.e. priority function) */
  mjaNotification,   /**< notify user of specified events */
  mjaNotificationAddress, /**< the email address at which to notify users of events */
  mjaOFile,          /**< stdout file */
  mjaOMinWCLimit,    /**< original min. wallclock limit (for job extend duration) */
  mjaOWCLimit,       /**< original wallclock limit (for virtual walltime scaling) */
  mjaOwner,          /**< requestor who submitted job, may be different than User */
  mjaPAL,            /**< partition access list */
  mjaParentVCs,      /**< list of parent VCs */
  mjaPrologueScript, /**< prologue scripts */
  mjaQueueStatus,    /**< queue status this iteration */
  mjaQOS,
  mjaQOSReq,         /**< requested qos */
  mjaRCL,            /**< required CL */
  mjaReqFeatures,    /**< requested features, only used for client parsing */
  mjaReqAWDuration,  /**< req active walltime duration */
  mjaReqCMaxTime,    /**< req latest allowed completion time */
  mjaReqMem,         /**< total job memory requested/dedicated */
  mjaReqNodes,       /**< number of nodes requested (obsolete) */
  mjaReqProcs,       /**< obsolete */
  mjaReqReservation, /**< required reservation */
  mjaReqReservationPeer, /**< required peer reservation */
  mjaReqRM,          /**< required destination RM */
  mjaReqRMType,      /**< required rm type */
  mjaReqSMinTime,    /**< req earliest start time */
  mjaReqVMList,      /**< required VM's */
  mjaResFailPolicy,  /**< policy to handle alloc resource failures (see J->ResFailPolicy) */
  mjaRM,             /**< master source job RM */
  mjaRMCompletionTime, /**< completion time as reported by RM's clock */
  mjaRMError,        /**< stderr file assigned by RM */
  mjaRMOutput,       /**< stdout file assigned by RM */
  mjaRMSubmitFlags,  /**< opaque flags to pass to RM at job submit */
  mjaRMXString,      /**< RM extension string */
  mjaRsvAccess,      /**< list of reservations accessible by job */
  mjaRsvStartTime,   /**< reservation start time */
  mjaRunPriority,    /**< effective job priority */
  mjaSessionID,      /**< Session ID/PID */
  mjaShell,          /**< execution shell */
  mjaSID,            /**< job's system id (parent cluster) */
  mjaSize,           /**< Cray interpretation of size */
  mjaStorage,        /**< storage information (used for dynamic storage) */
  mjaSRM,            /**< source rm */
  mjaSRMJID,         /**< source rm job id */
  mjaStageIn,        /**< used for job template attr tracking only */
  mjaStartCount,
  mjaStartPriority,  /**< effective job priority */
  mjaStartTime,      /**< most recent time job started execution */
  mjaState,
  mjaStatMSUtl,      /**< memory seconds utilized */
  mjaStatPSDed,
  mjaStatPSUtl,
  mjaStdErr,         /**< path of stderr output */
  mjaStdIn,          /**< path of stdin */
  mjaStdOut,         /**< path of stdout output */
  mjaStepID,
  mjaSubmitArgs,     /**< J->AttrString */
  mjaSubmitDir,      /**< dir where msub was called from (since IWD may be changed via msub -d) */
  mjaSubmitHost,     /**< host from which job was submitted */
  mjaSubmitLanguage, /**< resource manager language of submission request */
  mjaSubmitString,   /**< string containing full submission request */
  mjaSubmitTime,
  mjaSubState,
  mjaSuspendDuration, /**< duration job has been suspended */
  mjaServiceJob,     /**< job is system job (can be migrated to a flag when flags are expanded */
  mjaSysPrio,        /**< admin specified system priority */
  mjaSysSMinTime,    /**< system specified min start time (J->SMinTime) */
  mjaTaskMap,        /**< job allocation taskmap */
  mjaTemplateFailurePolicy, /**< job template failure policy */
  mjaTemplateList,   /**< templates (job matches) assigned to job */
  mjaTemplateSetList,/**< templates (job sets and job defs) assigned to job */
  mjaTermTime,       /**< job termination time (terminate job whether active or idle at this time) */
  mjaTemplateFlags,  /**< job template flags */
  mjaTrigger,
  mjaTrigNamespace,  /**< namespaces accepted by this job's triggers (dynamic attr) */
  mjaTTC,            /**< total task count */
  mjaUMask,          /**< umask of the user running this job */
  mjaUser,
  mjaUserPrio,       /**< user specified job priority */
  mjaUtlMem,
  mjaUtlProcs,
  mjaVariables,
  mjaVM,
  mjaVMCR,
  mjaVMUsagePolicy,  /**< policy regarding usage of virtual machine resources */
  mjaVWCLimit,       /**< virtual wc limit */
  mjaWarning,        /**< temp XML only attribute - not persistent across iterations, not checkpointed, no associated job attribute */
  mjaWorkloadRMID,   /**< J->TX->WorkloadRMID */
  mjaArrayMasterID, /**< index of array master job (only used by job arrays) */
  mjaLAST };

/**
 * The enum of possible job configuration attributes.
 *
 * @see MJobCfgAttr[] in MConst.c (sync)
 * @see MTJobLoad in MJob.c
 *
 * NOTE: The purpose of this list is to match the JOBCFG 
 * parameters against the parameters checked by mdiag -C 
 *
 */

enum MJobCfgAttrEnum {
  mjcaNONE = 0,
  mjcaAccount,
  mjcaAction,
  mjcaClass,
  mjcaCPULimit,
  mjcaDescription,
  mjcaDGres,
  mjcaFailurePolicy,
  mjcaFlags,
  mjcaGroup,
  mjcaJobScript,
  mjcaMem,              /* NOTE: This memory is for MEM=<Value> */
  mjcaNodeAccessPolicy,
  mjcaNodeSet,
  mjcaPartition,
  mjcaPref,
  mjcaQOS,
  mjcaRestartable,
  mjcaRM,
  mjcaRMServiceJob,
  mjcaSelect,
  mjcaStageIn,
  mjcaSRM,
  mjcaTFlags,
  mjcaUser,
  mjcaVMUsage,
  mjcaWalltime,
  mjcaSystemJobType,

  /* Wiki Attributes */

  mjcaArgs,
  mjcaDMem, 
  mjcaDDisk,
  mjcaDSwap,
  mjcaError,
  mjcaExec,
  mjcaExitCode,
  mjcaGAttr,
  mjcaGEvent,
  mjcaIWD,
  mjcaJName,
  mjcaName,
  mjcaPartitionMask,
  mjcaPriorityF,
  mjcaRDisk,
  mjcaRSwap,
  mjcaRAGRes,
  mjcaRCGRes,
  mjcaTaskPerNode,
  mjcaTrigger,
  mjcaVariables,

  /* Wiki attributes documented in Job Templates */

  mjcaDProcs,
  mjcaEUser,
  mjcaGenSysJob,
  mjcaGname,
  mjcaGres,
  mjcaInheritResources, /* If part of a templatedepend, job will be clone of parent job, then apply template */
  mjcaNodes,
  mjcaPriority,
  mjcaRArch,
  mjcaRFeatures,
  mjcaROpsys,
  mjcaTasks,
  mjcaTemplateDepend,
  mjcaUName,
  mjcaWClimit,
  mjcaDestroyTemplate,
  mjcaMigrateTemplate,
  mjcaModifyTemplate,
  mjcaLAST };


#define MDEF_JOBATTRSIZE MBMSIZE(mjaLAST)



/**
 * req object attributes
 *
 * @see MReqAttr[] - sync
 * @see struct mreq_t - sync
 * @see enum MJobAttrEnum - peer
 */

enum MReqAttrEnum {
  mrqaNONE = 0,
  mrqaAllocNodeList,    /* allocated hosts */
  mrqaAllocPartition,
  mrqaDepend,           /* dependent job */
  mrqaExclNodeFeature,
  mrqaGPUS,             /* gpus (gres internally, here for syntax) */
  mrqaMICS,             /* mics (gres internally, here for syntax) */
  mrqaGRes,             /* generic resources */
  mrqaCGRes,
  mrqaIndex,
  mrqaNodeAccess,
  mrqaNodeSet,
  mrqaPref,
  mrqaReqArch,          /* architecture */
  mrqaReqAttr,
  mrqaReqDiskPerTask,
  mrqaReqImage,
  mrqaReqMemPerTask,
  mrqaReqNodeDisk,
  mrqaReqNodeFeature,
  mrqaReqNodeMem,       /* configured memory */
  mrqaReqNodeAMem,      /* available memory */
  mrqaReqNodeProc,
  mrqaReqNodeSwap,      /* configured swap */
  mrqaReqNodeASwap,     /* available swap */
  mrqaReqOpsys,
  mrqaReqPartition,
  mrqaReqProcPerTask,
  mrqaReqSoftware,
  mrqaReqSwapPerTask,
  mrqaRMName,
  mrqaSpecNodeFeature,
  mrqaNCReqMin, /* NodeRequestList is not implemented so it is just Request.NC. */
  mrqaTCReqMin, /* was used to define the min count in Req->TaskRequestList but is really representing Req->TaskCount */
  mrqaTPN,
  mrqaTaskCount,
  mrqaUtilMem,
  mrqaUtilProc,
  mrqaUtilSwap,
  mrqaLAST };


#define MDEF_REQATTRSIZE MBMSIZE(mrqaLAST)

 
/* S3 */

/* sync w/MS3JobAttr[] */

enum MS3JobAttrEnum {

  ms3jaNONE,
  ms3jaAccount,         
  ms3jaAllocNodeList,   
  ms3jaArgs,            
  ms3jaAWDuration,      
  ms3jaClass,
  ms3jaCmdFile,         
  ms3jaCompletionTime,  
  ms3jaCPULimit,        
  ms3jaDepend,          
  ms3jaEnv,             
  ms3jaFlags,           
  ms3jaGAttr,           
  ms3jaGroup,           
  ms3jaHold,            
  ms3jaHopCount,
  ms3jaInteractiveScript,     
  ms3jaIsInteractive,   
  ms3jaIsRestartable,   
  ms3jaIsSuspendable,   
  ms3jaIWD,             
  ms3jaSubmitDir,       
  ms3jaJobName,         
  ms3jaJobID,           
  ms3jaNotification,    
  ms3jaOwner,           
  ms3jaNodeSelection,   
  ms3jaPAL,             
  ms3jaPrologueScript,  
  ms3jaEpilogueScript,  
  ms3jaQOSReq,          
  ms3jaReqAWDuration,   
  ms3jaTTC,             
  ms3jaReqReservation,  
  ms3jaReqReservationPeer,
  ms3jaReqRMType,       
  ms3jaReqSMinTime,     
  ms3jaResFailPolicy,   
  ms3jaRMSubmitFlags,   
  ms3jaRMXString,       
  ms3jaRsvAccess,       
  ms3jaShell,           
  ms3jaStartTime,       
  ms3jaState,           
  ms3jaStdErr,          
  ms3jaStdIn,           
  ms3jaStdOut,          
  ms3jaServiceJob,
  ms3jaStepID,          
  ms3jaSubmitLanguage,  
  ms3jaSubmitString,    
  ms3jaSubmitTime,      
  ms3jaUser,            
  ms3jaUserPrio,        
  ms3jaVMUsagePolicy,
  ms3jaSubmitHost,
  ms3jaLAST };


/* sync w/MS3ReqAttr[] */

enum MS3ReqAttrEnum {

  ms3rqaNONE,
  ms3rqaGPUS,
  ms3rqaMICS,
  ms3rqaGRes,           
  ms3rqaNCReqMin,       
  ms3rqaReqArch,        
  ms3rqaReqDiskPerTask, 
  ms3rqaReqMemPerTask,  
  ms3rqaReqNodeDisk,    
  ms3rqaReqNodeFeature, 
  ms3rqaReqNodeMem,     
  ms3rqaReqNodeProc,    
  ms3rqaReqNodeSwap,    
  ms3rqaReqOpsys,       
  ms3rqaReqSoftware,    
  ms3rqaReqSwapPerTask, 
  ms3rqaTCReqMin,       
  ms3rqaTPN,
  ms3rqaReqProcPerTask,
  ms3rqaLAST };



#define MMAX_WCHILD 16
#define MMAX_WORKFLOWJOBCOUNT 256

typedef struct mjworkflow_t {
  mjob_t *J;
  mulong  StartTime; /* workflow start time */
  int     PCount;    /* parent count */
  int     DepthIndex; /* relative depth from node to "root node */
  int     ChildIndex[MMAX_WCHILD + 1]; /* index of child in worfklow graph */
  enum MDependEnum ChildType[MMAX_WCHILD + 1];  /* child dependency type */  
  mbool_t StartTimeIsComputed; /*specifies whether the node currently stores a correct start time. This implies that its parents have been processed and have a correct start time as well */
  } mjworkflow_t;


/**
 * @struct mjblockinfo_t
 *
 * job structure holding relevant per partition block information about a job.
 * stored in a linked list in the job structure.
 *
 * @see struct mjob_t - parent
 */

typedef struct mjblockinfo_t {
  int                PIndex;      /**< partition index */
  enum MJobNonEType  BlockReason; /**< reason not considered for scheduling */
  char              *BlockMsg;    /**< (alloc)                          */
  } mjblockinfo_t;

#define MDEF_BADCOMPLETIONCODE -1 /* completion code for if a job fails internally (system job, etc.) */ 


/**
 * @struct mjob_t
 *
 * master job structure holding all relevant information about a job.
 *
 * @see struct mreq_t - child
 * @see struct msdata_t - for data staging
 * @see designdoc Job Structure
 *
 * NOTE: when adding an attribute to mjob_t, update the following:
 *  - enum MJobAttrEnum
 *  - const char *MJobAttr[]
 *  - MJobAToString()
 *  - MJobSetAttr()
 *  - MS3JobToXML() - modify DJAList[] array and switch (DJAList[]) - for
 *      grid support
 *  - MJobToExportXML() - modify attribute handling for grid translation
 */



struct mjob_t {
  char   Name[MMAX_NAME + 1]; /**< job ID                                  */
  char  *AName;               /**< alternate name - user specified (alloc) */
  char  *SRMJID;              /**< source resource manager job ID (alloc)  */
  char  *DRMJID;              /**< destination resource manager job ID (alloc) */
  char  *Owner;               /**< job owner (alloc)                       */
  char  *EUser;               /**< execution user (alloc,specified for user only) */

  long   CTime;               /**< creation time (first RM report)         */
  long   MTime;               /**< modification time (time attribute change from any source) */
  long   ATime;               /**< access time (most recent RM report)     */
  int    UIteration;          /**< update iteration (most recent RM report) - updated in MRMJobPre{Load,Update}() */

  long   StateMTime;          /**< time job state changed (includes Canceling) */

  long   EligibleTime;        /**< time job became eligible */

  long   SWallTime;           /**< duration job was suspended              */
  long   AWallTime;           /**< duration job was executing              */

  mjckpt_t *Ckpt;             /**< checkpoint structure (alloc)            */

  struct mjobgroup_t *JGroup; /**< job group information (alloc) */

  int TotalProcCount;         /**< job resource allocation cache */

  mqos_t     *QOSRequested;           /**< quality of service requested (pointer only) */

  macl_t     *RequiredCredList;            /**< req CL - must be met */
  char       *ReqVPC;         /*< requested VPC (alloc)                    */
                              /*< WARNING: also used during "mshow -a" so
                                           lower job-scheduling routines
                                           know how to prioritize the request */

  mbitmap_t ReqFeatureGResBM; /**< requested feature GRes BM          */

  mqos_t     *QOSDelivered;           /**< quality of service delivered (pointer only) */

  char      **RsvExcludeList; /**< rsv exclude list (alloc,maxcount=MMAX_PJOB,terminated w/NULL) */

  int         SessionID;      /**< session id of primary task */

  mnode_t    *RejectN;        /**< last node this job was rejected on      */

  marray_t   *PrologTList;    /**< Job Prolog Triggers. Executed immediately before the job begins.(alloc) */
  marray_t   *EpilogTList;    /**< Job Epilog Triggers. Executed immediately after the job exits.(alloc) */


  char                *Description; /**< job description/comment for templates and service jobs (alloc) */

  enum MVMUsagePolicyEnum VMUsagePolicy;  /**< virtual machine usage      */
  char   *CompletedVMList;  /**< (alloc) list of vms used by job */


  mvm_t    **ReqVMList;     /**< required vms (alloc,size-J->NLSize)      */

  mulong CMaxDate;          /**< user specified latest completion date    */

  int         FloatingNodeCount;            /**< floating node count (+ TPN for def job) sum of non-compute nodes */

  int         TotalTaskCount;            /**< Total Task Count for SLURM i.e. nodes=4,ttc=11 */

  mcred_t     Credential;


  int         LogLevel;       /**< verbosity of job tracking/logging       */
  double      Cost;           /**< job execution cost                      */

  int Size;                   /**< Cray's interpretation of size (Tasks and DProcs). */

  mrsv_t     *Rsv;              /**< active job rsv (pointer only)           */

  /* if ReqRID is populated, job MUST run in ReqRID only */
  char       *ReqRID;         /**< required rsv name/group (alloc)         */
  char       *PeerReqRID;     /**< Used in Grid mode to save the
                                Master reservation information
                                on the Slave in case the moab on the master
                                is restarted and queries the job information
                                from the slave. */

  mbitmap_t   RsvAccessBM;    /**< bitmap of enum MJobAccessPolicyEnum     */


  /* job can utilize resources of RsvAList even if not in rsv's ACL */
  char      **RsvAList;       /**< rsv access list (alloc,maxcount=MMAX_PJOB,terminated w/NULL) */

  mrm_t      *SubmitRM;            /**< rm to which job is submitted */
  mrm_t      *DestinationRM;            /**< rm on which job will execute */

  mbitmap_t   NotifyBM;       /**< events for which job should notify user (enum MNotifyEnum BM) */
  char       *SpecNotification; /**< rm-specific notification specified by user (alloc) */
  char       *NotifyAddress;  /**< destination to send notifications */
  mbitmap_t   NotifySentBM;   /**< email notifications sent (enum MJobNotifySentEnum BM) */

  marray_t   *Triggers;              /**< job triggers (alloc) */

  mreq_t     *Req[MMAX_REQ_PER_JOB + 1]; /* (alloc) */

  mjobe_t     Env;              /**< job execution environment               */

  mrsrc_t     Alloc;
  mrsrc_t     Request;        /**< summary of all req[x] entries for easy access */

  enum MTaskDistEnum SpecDistPolicy;
  enum MTaskDistEnum DistPolicy;

  char       *Geometry;       /**< arbitrary distribution specification (alloc) */

  mulong   SystemQueueTime;   /**< time job was initially eligible to start (only used if EffQueueDuration not specified) */

  /* performance criteria */

  long   EffQueueDuration;    /**< duration of time job was eligible to run (if -1, use SystemQueueTime) */
  double EffXFactor;

  /* history/time constraints */

  mulong  SMinTime;         /**< effective earliest start time (0 if not set) */
  mulong  SpecSMinTime;     /**< user specified earliest start time (0 if not set) */
  mulong *SysSMinTime;      /**< system mandated earliest start time (may be recalculated each iteration) */
                            /**< per partition SysSMinTime (alloc) */

  /* NOTE:  overused to report time job cancellation specified */

  mulong CancelReqTime;     /**< time last cancel request was sent to rm  */
  mulong PreemptTime;       /**< time this preemptor job preempted its last preemptee batch */
  mulong TermTime;          /**< time by which job must be terminated     */
  mulong SubmitTime;        /**< time job was submitted to RM             */
  mulong StartTime;         /**< time job began most recent execution     */
  mulong DispatchTime;      /**< time job was dispatched by RM            */
  mulong RsvRetryTime;      /**< time job was first started with MJobStartRsv() */
  mulong CompletionTime;    /**< time job execution completed (according to Moab's clock) */
  mulong RMCompletionTime;  /**< time job execution completed (as reported by TORQUE's clock) */
  mulong CheckpointStartTime; /**< time checkpoint was started            */
  mulong HoldTime;          /**< time currently held job was put on hold  */
  mulong MinPreemptTime;    /**< minimum time job must run before being eligible to be preempted (relative to start) */

  int    CompletionCode;    /**< execution completion code                */

  int    StartCount;        /**< number of times job was started          */
  int    DeferCount;        /**< number of times job was deferred         */
  int    CancelCount;       /**< number of times job has been cancelled   */
  int    MigrateCount;      /**< number of times job has been migrated    */

  long PreemptIteration;    /**< iteration job preempted existing workload
                                    on targetted resources */

  int    PreemptCount;      /**< number of times job was preempted        */
  int    BypassCount;       /**< number of times lower prio job was backfilled */

  enum MHoldReasonEnum HoldReason;  /**< reason deferred/held             */

  mln_t    *BlockInfo;      /**< linked list of job block info mjblockinfo_t (alloc) */

  enum MJobNonEType    MigrateBlockReason; /**< reason not able to migrate */
  char                *MigrateBlockMsg;    /**< (alloc) */

  enum MDependEnum     DependBlockReason; /**< dependency reason at
                                               last iteration */
  char                *DependBlockMsg;    /**< (alloc) */

  mmb_t *MessageBuffer;                /**< general message buffer (alloc)           */

  mutime OrigWCLimit;       /**< orig walltime limit (for virtual scaling) */
  mutime VWCLimit;          /**< virtual walltime limit (for virtual scaling) */
  mutime WCLimit;           /**< effective walltime limit                 */
  mutime CPULimit;          /**< specified CPU limit per job (in proc-seconds) */
  mutime SpecWCLimit[MMAX_SHAPE + 2]; /**< requested walltime limit NOTE:  unsigned long, does not support -1 */
  mutime RemainingTime;     /**< execution time job has remaining         */
  mutime EstWCTime;         /**< estimated walltime job will consume      */
  mutime MinWCLimit;
  mutime OrigMinWCLimit;

  int    MaxJobMem;         /**< job level limits (should replace w/ap_t?) */
  int    MaxJobProc;        /**< job level limits */


  mbitmap_t SpecPAL; /**< user specified partition access list - only user can change (bitmap) */
  mbitmap_t SysPAL;  /**< feasible partition access list built by scheduler (bitmap) */
  mbitmap_t PAL;     /**< effective job partition access list (bitmap) */
                                         /**< created from SpecPAL & SysPAL when PAL == 0 (or new SpecPAL is given) */
  mbitmap_t CurrentPAL;    /**< partition access list for current iteration (see MQueueSelectJobs() for clear/set (why is this 0 during UI phase?) */
  mbitmap_t EligiblePAL;    /**< eligible partition access list for current iteration */

  enum MJobStateEnum State;  /**< RM job state                            */
  enum MJobStateEnum EState; /**< expected job state due to scheduler action */
  enum MJobStateEnum IState; /**< internal job state                      */

  enum MJobSubStateEnum SubState; /**< moab internal job state */

  /* should these be moved to job flags? */

  mbool_t CMaxDateIsActive;

  char *InteractiveScript;  /**< script to be run in interactive mode     */

  mbool_t IsInteractive;    /**< job is interactive                       */
  mbool_t SoftOverrunNotified;

  mbool_t InRetry;          /**< job is in job start retry cycle */

  mbitmap_t     Hold;             /**< bitmap of MHoldTypeEnum { ie, USER, SYS, BATCH } */

  long    SystemPrio;       /**< system admin assigned priority (NOTE: >> MAXPRIO indicates relative) */
  long    UPriority;        /**< job priority given by user               */
  long    CurrentStartPriority;    /**< most recently calculated partition priority of job to start */
  long    PStartPriority[MMAX_PAR + 1];  /**  per partition priority of job to start */
  long    RunPriority;      /**< priority of job to continue running      */

  int    *TaskMap;          /**< terminated w/-1 (alloc)                  */
  int     TaskMapSize;      /**< current size of TaskMap not including termination marker (max=MSched.JobMaxNodeCount) */

  mds_t *DataStage;                /**< used for data staging (alloc) */

  mnl_t NodeList;           /**< list of allocated hosts */
  mnl_t ReqHList;           /**< required hosts - {sub,super}set (NOTE: TC portion is not used)*/
  mnl_t ExcHList;           /**< excluded hosts */

  int     HLIndex;          /**< index of hostlist req                    */

  /* NOTE:  this is set when HostList is loaded as an RM Extension
            when mode=NONE it should be interpreted as ExactSet */

  enum MHostListModeEnum ReqHLMode; /**< req hostlist mode                */

  enum MAllocResFailurePolicyEnum ResFailPolicy; /**< allocated resource
                                                      failure policy */

  enum MJRsvSubTypeEnum RType;  /**< reservation subtype (deadline, priority, etc) */

  mbitmap_t  JobRejectPolicyBM;  /* bitmap of enum JobRejectPolicyBM */

#if 0

  int    Cluster;           /**< step cluster                             */
  int    Proc;              /**< step proc                                */
#endif

  char  *SubmitHost;        /**< job submission host (alloc)              */

  mbitmap_t Flags;     /**< misc job flags (bitmap of enum MJobFlagEnum) */
  mbitmap_t SpecFlags; /**< user specified flags                     */
  mbitmap_t SysFlags;  /**< system specified flags                   */

  mbitmap_t IFlags;   /**< internal flags (bitmap of enum MJobIFlagEnum) */

  mbitmap_t RMXSetAttrs;

  mbitmap_t NodeMatchBM;    /**< job node match policy (bitmap of enum MNodeMatchPolicy) */
                            /**< not user-settable */

  mbitmap_t AttrBM;         /**< job attributes (BM) */

  char  *MasterHostName;    /**< name of requested/required master host (alloc) */
  char  *MasterJobName;

  double PSUtilized;        /**< procseconds utilized by job              */
  double PSDedicated;       /**< procseconds dedicated to job             */
  double MSUtilized;
  double MSDedicated;

  char  *RMXString;         /**< RM extension string (alloc/opaque)       */
  char  *RMSubmitString;    /**< raw command file (alloc)                 */
  char  *RMSubmitFlags;     /**< opaque flags to pass to RM at job submit */

  enum MRMTypeEnum RMSubmitType;  /**< RM language that the submitted script was originally written in */
  enum MRMTypeEnum ReqRMType;  /**< required RM submit language (can match type or language) */

  char  *TermSig;           /**< signal to send job at terminal time (alloc) */

  /* NOTE:  SystemJID in format <JID>[/<JID>]... */
  /*        hop count contained in 'G' */

  char  *SystemID;          /**< identifier of the system owner (alloc)   */
  char  *SystemJID;         /**< external system identifier (alloc)       */
  mrm_t *SystemRM;          /**< pointer to system owner peer RM */

  mdepend_t  *Depend;       /**< job dependency list (alloc)              */
  mln_t *ImmediateDepend;   /**< jobs that will run immediately after this job, using this job's resource allocation (alloc) */

  mulong      RULVTime;     /**< resource utilization violation time.
                                 duration job has violated resource limit.
                                 NOTE:  single dimensional, how to account
                                 for multiple metrics going in and out
                                 of violation */

  mxml_t    **FairShareTree;       /**< pointer to array of size MMAX_PAR (alloc) */

  /* mulong    ExemptList; */     /**< policies against which a job is
                                       exempt (bitmap of enum XXX) */
  mxload_t *ExtLoad;        /**< extended load tracking (alloc) */

  void     *ExtensionData;             /**< extension data (alloc, for dynamic jobs, struct mdjob_t) */

  struct mjarray_t *Array;  /**< array data (alloc) */
  struct msysjob_t *System; /**< system job data (alloc) */

  mjtx_t   *TemplateExtensions;             /**< job template/profile extensions (alloc) */
  mjgrid_t *Grid;              /**< grid data (alloc) */

  char     *AttrString;     /**< list of attributes specified by job, ie submit args (alloc) */

  mln_t    *TStats;         /**< template stats associated with job (alloc) */
  mln_t    *TSets;          /**< template sets/defs associated with job (alloc) */

  mhash_t   Variables;      /**< list of variables (alloc) */

  int       CPUTime;        /**< stores the CPU usage time as exactly reported by the RM
                              This usually is a metric that indicates how much time was spent
                              running on a CPU for the ENTIRE job. */

  mnprio_t *NodePriority;    /**< priority parameters (optional,alloc) */

  char*     PrologueScript; /**< per job prologue or epilogue script */
  char*     EpilogueScript; /**< ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ */

  char    *EligibilityNotes; /** < (alloc) eligibility notes generated for blocked messages in MQueueSelectJobs */
  mjob_t *ActualJ;           /** Non-NULL if job is a temporary job represeting another job */

  mulong WillrunStartTime;   /* Start time reported by slurm's willrun. In the case where slurm says that the job can't run for
                                so many seconds in MQueueScheduleIJobs, pass the start time reported by willrun to 
                                MJobPReserve->MJobReserve->MJobGetEStartTime which will only find times for the job to start,
                                starting at willrun starttime. */

  mvm_req_create_t *VMCreateRequest;    /**< information for a VM (if this job is a msjtVMTracking job) - alloc */

  mln_t *ParentVCs;       /* Parent VCs (alloc) - create only in MVCAddObject() */

  long LastChargeTime;    /* Last time job was charged/modified (NAMI) */

  mdynamic_attr_t *DynAttrs; /* Dynamic attributes */
#if 0
  /* These are attributes in DynAttrs, "declared" here so we know what types they should be.
      sync w/ MJobSetDynamicAttr(), MJobGetDynamicAttr() */

  char *TrigNameSpace;    /* Namespaces that triggers on this job will accept (comma-delimeted) */
#endif
  };


#include <string>

typedef std::map<std::string,mjob_t *> JobTable;
typedef std::map<std::string,mjob_t *>::iterator job_iter;

typedef std::map<std::string,mrsv_t *> RsvTable;
typedef std::map<std::string,mrsv_t *>::iterator rsv_iter;

typedef std::map<std::string,int> mmap;
typedef std::map<std::string,int>::iterator miter;


/* job array structures */

/**
 * Job Array Architecture overview
 *
 * A job is first submitted with an array specification (outlined in
 * MJobProcessExtensionString():mxaArray).  To avoid race conditions,
 * however, the job is not duplicated internally until the end of
 * MRMJobPostLoad().  At the end of MRMJobPostLoad() a call is made to
 * MJobLoadArray() which will create all the array jobs internally (they
 * will not be submitted to the RM).  The jobs will be given an ID of
 * <original_job>.<orginal_id>.<array_index>.  Because any job array *MUST*
 * originally submitted using msub it will retain its moab job id.
 *
 * MJobLoadArray() will duplicate these jobs internally and add them to the
 * internal resource manager.  Array jobs will not be scheduled individually.
 * For idle jobs the original job will be scheduled and before it reaches
 * MJobAllocMNL() the job will be broken up and as many resources as are
 * available for the job will be used (up to moab limits and up to the job's
 * array limits).  Each of these jobs will call MJobAllocMNL() and will be
 * placed into the nodes already determined to be available.
 */

#ifndef MDEF_MAXJOBARRAYSIZE
#define MDEF_MAXJOBARRAYSIZE 100000
#endif /* MDEF_MAXJOBARRAYSIZE */

#define MMAX_JOBARRAYSIZE MDEF_MAXJOBARRAYSIZE

/**
 * job array data
 *
 * @see MJobArrayAdd() - create new job array
 * @see MJobArrayUpdate() - update job array
 * @see MQueueCollapse()
 * @see MMAX_JOBARRAYSIZE
 */

typedef struct mjarray_t {

  /* info for master job */
  char Name[MMAX_NAME];

  int Count;   /* number of job steps currently in array */
  int Size;    /* number of job steps supported in array */

  int Active;
  int Migrated;
  int Idle;
  int Complete;

  int  Limit;   /* slot limit as specified at submission */
  long PLimit;  /* policy limit for current iteration */

  int                 *Members;  /* index of members that are required (alloc) */

  mjob_t             **JPtrs;    /* pointers for all subjobs (alloc) */
  char               **JName;    /* names of all subjobs (alloc) */
  enum MJobStateEnum  *JState;   /* states of all subjobs (alloc) */

  /* info for individual jobs */

/*
  int     ArrayIndex;

  int     JobIndex;
*/
  mbitmap_t  Flags;                 /* BM of MJArrayFlagEnum */

  mnl_t **FNL;
  } mjarray_t;


/* no sync */

enum MJArrayFlagEnum {
  mjafNONE,
  mjafDynamicArray,
  mjafIgnoreArray,
  mjafPartitionLocked,
  mjafLAST };


/* sync w/MJEnvVarAttr */

enum MJEnvVarEnum {
  mjenvNONE = 0,
  mjenvAccount,    
  mjenvBatch,      
  mjenvClass,      
  mjenvDepend,     
  mjenvGroup,      
  mjenvJobArrayIndex,      
  mjenvJobArrayRange,      
  mjenvJobID,      
  mjenvJobIndex,   
  mjenvJobName,    
  mjenvMachine,    
  mjenvNodeCount,  
  mjenvNodeList,   
  mjenvPartition,  
  mjenvProcCount,  
  mjenvQos,        
  mjenvTaskMap,    
  mjenvUser,       
  mjenvLAST };

/* job will run structure */

typedef struct mjwrs_t {
  mjob_t     *J;
  mnl_t       NL;         /* nodelist for will-run call */
  mjob_t    **Preemptees; /* list of preemptees the job could preempt */
  mulong      StartDate;  /* epoch time when NL is available */
  int         TC;         /* Contains the size of the midplane block size */
  } mjwrs_t;


typedef struct mtjobstat_t {
  /* NOTE: currently do not track stats for jobs which were canceled by user or admin */

  /* host performance */
  /* host failure rate */
  /* host power consumption */
  /* number of samples */
  /* io/mem/walltime/input/output */

  mfs_t    F;
  must_t   S;
  mcredl_t L;  /* for current usage */

  double PerfUpperBound;  /* the largest job-size/runtime sample seen thus far */
  double PerfLowerBound;  /* the smallest job-size/runtime sample seen thus far */
  double PerfGlobalSampleCount;    /* global number of samples taken */
  } mtjobstat_t;


enum MJTMatchFlagEnum {
  mjtmfNONE = 0,
  mjtmfExclusive,
  mjtmfLAST };


/* job template matching structure */

/* sync w/MJTMatchAttr[] */

enum MJTMatchAttrEnum {
  mjtmaNONE = 0,
  mjtmaFlags,
  mjtmaJMax,
  mjtmaJMin,
  mjtmaJDef,
  mjtmaJSet,
  mjtmaJStat,
  mjtmaJUnset,
  mjtmaLAST };


#define MMAX_JTSTAT 4

typedef struct mjtmatch_t {
  char           Name[MMAX_NAME];

  struct mjob_t *JMax;   /* max job constraints - cannot be exceeded or cannot be set */
  struct mjob_t *JMin;   /* min job constraints - must be reached or exceeded or must be set */
  struct mjob_t *JDef;   /* attribute values to apply if not explicitly set */
  struct mjob_t *JSet;   /* attributes to set even if explicitly set */
  struct mjob_t *JUnset; /* attributes to unset if explicitly set */

  struct mjob_t *JStat[MMAX_JTSTAT]; /* job template to update stats for - can match self */

  mbitmap_t      FlagBM; /* (bitmap of enum MJTMatchFlagEnum) */
  } mjtmatch_t;



typedef struct mjob_submit_t {
  mxml_t *JE;

  int     ID;

  char    User[MMAX_NAME];

  long    SubmitTime;

  mulong  JournalID; /* ftell status of entry in msub submit journal */
  } mjob_submit_t;

/**
 * reservation 'bucket' 
 *
 * @see mpar_t.QOSBucket[]
 */

typedef struct mrsv_bucket_t {
  char    Name[MMAX_NAME];  /* '\1' for cleared bucket */
  mqos_t *List[MMAX_QOS];   /* NULL terminated */
  int     RsvDepth;
  } mrsv_bucket_t;

typedef struct mnatq_t {
  mjob_t *MJob[MDEF_MAXJOB];
  } mnatq_t;

typedef struct mnatrm_t {
  mnatq_t NQ;  /* nat queue */
  } mnatrm_t;


typedef struct mobjdb_t {
  enum MXMLOTypeEnum OType;
  void *O;
  } mobjdb_t;

/**
 * @struct mtransnode_t
 *
 * structure used for transitioning nodes to/from the database
 *
 * NOTE: when adding an attribute to mtransnode_t update the following:
 *  - ensure there is a corresponding field in the Nodes table in the database
 *  - MNodeTransitionAllocate (if needed)
 *  - MNodeTransitionFree (if allocated)
 *  - MNodeTransition
 *  - MNodeToTransitionStruct
 *  - MNodeTransitionToXML
 */

#define MMAX_NODE_DB_WRITE MDEF_MAXNODE

typedef struct mtransnode_t {

  char      Name[MMAX_NAME];                /* from N->Name */
  int       Index;                          /* from N->Index */

  enum MNodeStateEnum State;               /* from N->State */

  char     *OperatingSystem;               /* from N->ActiveOS */
  
  int       ConfiguredProcessors;           /* from N->CRes.Procs */
  int       AvailableProcessors;            /* from N->ARes.Procs */

  int       AvailableDisk;                  /* from N->ARes.Disk */
  int       ConfiguredDisk;                 /* from N->CRes.Disk */

  int       AvailableSwap;                  /* from N->ARes.Swap */
  int       ConfiguredSwap;                 /* from N->CRes.Swap */
  
  int       ConfiguredMemory;               /* from N->CRes.Mem */
  int       AvailableMemory;                /* from N->ARes.Mem */

  char     *Architecture;                  /* from N->Arch */
  char     *AvailGRes;                     /* from N->AGres */
  char     *ConfigGRes;                    /* from N->CGRes */
  char     *DedGRes;                       /* from N->CGRes */
  char     *AvailClasses;                  /* from N->ARes.PSlot */
  char     *ConfigClasses;                 /* from N->CRes.PSlot */

  double    ChargeRate;                     /* from N->ChargeRate */
  double    DynamicPriority;                /* from N->P */

  mbool_t   EnableProfiling;                /* from N->ProfilingEnabled */

  char     *Features;                      /* from N->FBM */
  char     *Flags;                         /* from N->Flags */

  int       HopCount;                       /* from N->HopCount */

  int       NodeIndex;                      /* from N->NodeIndex */

  enum MHVTypeEnum HypervisorType;         /* from N->HVType */

  mbool_t   IsDeleted;                      /* from N->IsDeleted */
  mbool_t   IsDynamic;                      /* from N->RM->IsDynamic */

  char     *JobList;                       /* from N->JList */

  long      LastUpdateTime;                 /* from N->ATime */

  double    LoadAvg;                        /* from N->Load */
  double    MaxLoad;                        /* from N->MaxLoad */

  int       MaxJob;                         /* from N->AP.HLimit[mptMaxJob][] */
  int       MaxJobPerUser;                  /* from N->NP->MaxJobPerUser */
  int       MaxProc;                        /* from N->AP.HLimit[mptMaxProc][] */
  int       MaxProcPerUser;                 /* from N->NP->MaxProcPerUser */

  char     *Messages;                       /* from N->MB */
  char     *OldMessages;                    /* from N->Message */
  char     *NetworkAddress;                 /* from N->NetAddr */
  char     *NodeSubState;                   /* from N->SubState */
  char     *Operations;                     /* see MNodeAToStr (line 12222) */
  char     *OSList;                         /* from N->OSList */
  char     *Owner;                          /* see MNodeAToStr (line 12338) */
  char     *ResOvercommitFactor;            /* from N->ResOverCommitFactor */
  char     *ResOvercommitThreshold;         /* from N->ResOverCommitThreshold */
  char     *Partition;                      /* from N->PtIndex */

  mbool_t   PowerIsEnabled;                 /* from N->PowerIsEnabled */

  enum MPowerPolicyEnum PowerPolicy;        /* from N->PowerPolicy */
  enum MPowerStateEnum PowerSelectState;    /* from N->PowerSelectState */
  enum MPowerStateEnum PowerState;          /* from N->PowerState */

  int       Priority;                       /* from N->Priority */

  char     *PriorityFunction;               /* from see MNodeAToStr (line 12411) */

  int       ProcessorSpeed;                 /* from N->ProcSpeed */

  char     *ProvisioningData;               /* see MNodeAToStr (line 12437) */
  int       ReservationCount;               /* see MNodeAToStr (line 12667) */

  char      *ResourceManagerList;           /* from N->RMList */

  int       Size;                           /* from N->SlotsUsed */
  double    Speed;                          /* from N->Speed */
  double    SpeedWeight;                    /* from N->P->CW[mnpcSpeed] */

  long      TotalNodeActiveTime;            /* from N->SATime */
  long      LastModifyTime;                 /* from N->StateMTime */
  long      TotalTimeTracked;               /* from N->STTime */
  long      TotalNodeUpTime;                /* from N->SUTime */
  int       TaskCount;                      /* from N->TaskCount */

  int       Rack;                           /* from N->RackIndex */
  int       Slot;                           /* from N->SlotIndex */

  mxml_t   *Variables;                      /* from N->Variables */

  mstring_t AttrList;                       /* from N->AttrList */
  mstring_t ReservationList;                /* from N->R[] */
  mstring_t GMetric;                        /* from N->XLoad->GMetric */
  mstring_t GEvents;                        /* from N->GEvent */

  char      *VMOSList;                    /* from N->VMOSList */

#ifdef MUSEMONGODB
  mongo::BSONObjBuilder *BSON;                     /* BSON to send to Mongo */
#endif
  } mtransnode_t; 


enum MTransFlagsEnum {
  mtransfNONE = 0,
  mtransfArrayMasterJob,
  mtransfArraySubJob,
  mtransfDeleteExisting,
  mtransfWasCanceled,
  mtransfPurge,
  mtransfLimitedTransition,
  mtransfSystemJob };

/**
 * @struct mtransjob_t
 * 
 * structure used for transitioning jobs to/from the database
 *
 * NOTE: when adding an attribute to mtransjob_t update the following:
 *  - ensure there is a corresponding field in the Jobs table in the database
 *  - MJobTransitionAllocate (if needed)
 *  - MJobTransitionFree (if allocated)
 *  - MJobTransition
 *  - MJobToTransitionStruct
 *  - MJobTransitionToXML
 */

#define MMAX_JOB_DB_WRITE MMAX_NODE_DB_WRITE

typedef struct mtransjob_t {

  mbitmap_t  TFlags;  /* BM of MTransFlagsEnum */

  /* transition data fields - stored in database */

  char Name[MMAX_NAME];             /* JobID/Name */
  char *SourceRMJobID;              /* ID of the job from the source RM */
  char *DestinationRMJobID;         /* ID of the job from the destination RM */
  char *GridJobID;                  /* from J->SystemJID */
  char *SystemID;                   /* from J->SystemID */
  int  SessionID;                   /* from J->SessionID */

  char *AName;                      /* Alternate name (user specified) */
  char *JobGroup;                   /* name of Job Group the job belongs to */
   
  char *User;                       /* name of the user who owns the job */
  char *Group;                      /* name of the OS group which owns the job */
  char *Account;                    /* account the job is associated with */
  char *Class;                      /* class the job is associated with */
  char *QOS;                        /* qos the job is associated with */
  char *QOSReq;                     /* J->QReq->Name */

  char *GridUser;                   /* J->G->User */
  char *GridGroup;                  /* J->G->Group */
  char *GridAccount;                /* J->G->Account*/
  char *GridClass;                  /* J->G->Class */
  char *GridQOS;                    /* J->G->QOS */
  char *GridQOSReq;                 /* J->G->QOSReq */

  char *UserHomeDir;                /* J->Cred.U->HomeDir */
  
  enum MJobStateEnum State;         /* current state the job is in */
  enum MJobStateEnum ExpectedState; /* state we expect the job to be in */
  enum MJobSubStateEnum SubState;                 /* job substate from J->SubState */

  char *DestinationRM;              /* name of the RM where the job will run */
  char *SourceRM;                   /* name of the RM the job originated */
  enum MRMTypeEnum SourceRMType;    /* type of the RM the job originated */
  mbool_t SourceRMIsLocalQueue;     /* SRM has mrmifLocalQueue flag set. */

  long SubmitTime;                  /* time the job was submitted */
  long StartTime;                   /* time the job started */
  long CompletionTime;              /* time the job completed */
  long QueueTime;                   /* time job was initially eligible to start */
  long EffQueueDuration;            /* duration of time job was eligible to run */
  long SuspendTime;                 /* from J->SWallTime */
  long RequestedCompletionTime;     /*FIXME not in the spreadsheet??? */
  long RequestedStartTime;          /* J->SpecSMinTime */
  long SystemStartTime;             /* J->SysSMinTime */
  long EligibleTime;                /* J->EligibleTime */
  
  long RequestedMinWalltime;        /* from J->MinWCLimit */
  long RequestedMaxWalltime;        /* from J->SpecWCLimit[0] or J->WCLimit */
  long UsedWalltime;                /* from J->AWallTime */
  long MinPreemptTime;              /* from J->MinPreemptTime */
  long HoldTime;                    /* from J->HoldTime */

  char *Dependencies;               /* from MJobCheckDependency */
  char *DependBlockMsg;             /* from J->DependBlockMsg */

  char *RequestedHostList;          /* from J->ReqHList */
  char *ExcludedHostList;           /* from J->ExcHList */
  char *MasterHost;                 /* from J->MasterHostName */
  char *SubmitHost;                 /* from J->SubmitHost */

  mbitmap_t SpecPAL;     /* from J->SpecPAL */
  mbitmap_t SysPAL;      /* from J->SysPAL */
  mbitmap_t PAL;         /* from J->PAL */

  int  ActivePartition;           /* from J->NodeList[0]->PtIndex */

  long RsvStartTime;              /* from J->R->RsvStartTime */
  char *ReservationAccess;        /*FIXME not in the spreadsheet??? */
  char *RequiredReservation;      /*FIXME not in the spreadsheet??? */
  char *PeerRequiredReservation;  /* J->PeerReqRID */

  mbitmap_t GenericAttributes; /* from J->AttrBM */
  char *Flags;                                      /* from J->Flags */
  mbitmap_t  Hold;                                     /* == J->Hold */

  char *StdErr;                   /* from J->E.Error */
  char *StdOut;                   /* from J->E.Output */
  char *StdIn;                    /* from J->E.Input */
  char *RMOutput;                 /* from J->E.RMOutput */
  char *RMError;                  /* from J->E.RMError */

  char *CommandFile;              /* from J->E.Cmd */
  char *Arguments;                /* from J->E.Args */
  char *InitialWorkingDirectory;  /* from J->E.IWD */

  long Size;                      /* FIXME not in the spreadsheet??? */
  double Cost;                    /* from J->Cost */
  long CPULimit;                  /* from J->CPULimit */
  long UMask;                     /* from J->E.UMask */

  int CompletionCode;             /* from J->CompletionCode */

  int StartCount;                 /* from J->StartCount */
  int BypassCount;                /* from J->BypassCount */
 
  long UserPriority;              /* from J->UPriority */
  long SystemPriority;            /* from J->SystemPrio */
  long Priority;                  /* from J->CurrentStartPriority */
  long RunPriority;               /* from J->RunPriority */
  char *PerPartitionPriority;     /* from J->PStartPriority[] */

  char *Description;              /* from J->Description */
  char *Messages;                 /* from J->MB */
  char *NotificationAddress;      /* from J->NotifyAddress */
  mbitmap_t   NotifyBM;           /* from J->NotifyBM */
 
  enum MRMTypeEnum RMSubmitType;  /* from J->RMSubmitType */

  int MaxProcessorCount;          /* total proc count of the job (total of all reqs) */

  int RequestedNodes;             /* total requested node count */

  char *BlockReason;              /* from J->BlockReason */
  char *BlockMsg;                 /* from J->BlockMsg */

  char *AllocVMList;              /* from J->VMTaskMap */

  char *MigrateBlockReason;       /* from J->MigrateBlockReason */
  char *MigrateBlockMsg;          /* from J->MigrateBlockMsg */

  double MSUtilized;              /* from J->MSUtilized */
  double PSDedicated;             /* from J->PSDedicated */
  double PSUtilized;              /* from J->PSUtilized */

  char *ParentVCs;                /* from J->ParentVCs */
  char *TemplateSetList;          /* from J->TSets */
  char *RMSubmitString;           /* from J->RMSubmitString */
  char *Environment;              /* from J->E.BaseEnv */

  mstring_t *TaskMap;             /* from J->TaskMap -  allocated by 'new' */
  mstring_t *RMExtensions;        /* from J->E.BaseEnv -  allocated by 'new' */
  
  mxml_t   *Variables;            /* from J->Variables */

  long       ArrayMasterID;       /* Index of array master job (not use for non-array jobs) */

  struct mtransreq_t *Requirements[MMAX_REQ_PER_JOB]; /* job requests */

#ifdef MUSEMONGODB
  mongo::BSONObjBuilder *BSON;                     /* BSON to send to Mongo */
#endif
  } mtransjob_t;

/**
 * @struct mtransreq_t
 *
 * structure used for transitioning job requests to/from the database
 *
 * NOTE: when adding an attribute bo mtransreq_t update the following:
 *  - ensure there is a corresponding field in the Requests table in the database
 *  - MReqTransitionAllocate (if needed)
 *  - MReqTransitionFree (if allocated)
 *  - MReqToTransitionStruct
 *  - MReqTransitionToXML
 */

typedef struct mtransreq_t {
  char JobID[MMAX_NAME];            /* name of the job owning this req (alloc) */
  int   Index;                      /* index of the req in the job */

  mstring_t   *AllocNodeList;       /* from R->NodeList allocated by 'new' operator */
  char *Partition;                  /* from R->PtIndex */

  enum MNodeAccessPolicyEnum NodeAccessPolicy; /* from R->NAccessPolicy */

  char *PreferredFeatures;          /* from R->Pref->FeatureBM */
  char *RequestedApp;               /* from R->OptReq->Appl */
  char *RequestedArch;              /* from R->Arch */
  char *ReqOS;                      /* from R->Opsys */
  char *ReqNodeSet;                 /* from R->SetList */
  
  /*int MaxNodeCount;*/             /* NYI */
  int   MinNodeCount;               /* from R->NodeCount (mrqaNCReqMin) */
  /*int MaxTaskCount;*/             /* NYI */
  int   MinTaskCount;               /* from R->TaskRequestList[0] (mrqaTCReqMin) */

  int   TaskCount;                  /* from R->TaskCount */
  int   TaskCountPerNode;           /* from R->TasksPerNode */

  int   DiskPerTask;                /* from R->DRes->Disk */
  int   MemPerTask;                 /* from R->DRes->Mem */
  int   ProcsPerTask;               /* from R->DRes->Procs */
  int   SwapPerTask;                /* from R->DRes->Swap */

  int   UtilSwap;                   /* from R->URes->Swap */

  char  *NodeDisk;                  /* from R->ReqNR[mrDisk] */
  char  *NodeFeatures;              /* from R->ReqFBM */
  char  *NodeMemory;                /* from R->ReqNR[mrMem] */
  char  *NodeSwap;                  /* from R->ReqNR[mrSwqp] */
  char  *NodeProcs;                 /* from R->ReqNR[mrProc] */

  mbool_t TasksSpecified;           /* from J->IFlags(mjifTasksSpecified) */
  mbool_t DProcsSpecified;          /* from J->IFlags(mjifDProcsSpecified) */
  
  char *GenericResources;           /* from R->GenericResources */
  char *ConfiguredGenericResources; /* from R->ConfiguredGenericResources */

  mxml_t *RequestedAttrs;           /* from R->ReqAttr */
  } mtransreq_t;

/**
 * @struct mtransrsv_t
 * 
 * structure used for transitioning reservations to/from the 
 * database 
 *
 * NOTE: when adding an attribute to mtransrsv_t update the 
 * following: 
 *  - ensure there is a corresponding field in the Reservations
 *    table in the database
 *  - MRsvTransitionAllocate (if needed)
 *  - MRsvTransitionFree (if allocated)
 *  - MRsvTransition
 *  - MRsvToTransitionStruct
 *  - MRsvTransitionToXML
 */

#define MMAX_RSV_DB_WRITE MMAX_NODE_DB_WRITE

typedef struct mtransrsv_t {

  /* transition data fields - stored in database */

  char Name[MMAX_NAME];

  mbitmap_t  TFlags;  /* BM of MTransFlagsEnum */

  int  AllocNodeCount;
  int  AllocProcCount;
  int  AllocTaskCount;
  int  MaxJob;

  long CTime;
  long Duration;
  long EndTime;
  long ExpireTime;
  long LastChargeTime;
  long StartTime;
  long Priority;

  double StatCAPS;
  double StatCIPS;
  double StatTAPS;
  double StatTIPS;
  double Cost;

  char *CL;
  char *Comment;
  char *ExcludeRsv;
  char *Flags;
  char *GlobalID;
  char *HostExp;
  char *History;
  char *Label;
  char *ACL;
  char *AAccount;
  char *AGroup;
  char *AUser;
  char *AQOS;
  char *LogLevel;
  char *Messages;
  char *Owner;
  char *Partition;
  char *Profile;
  char *ReqArch;
  char *ReqFeatureList;
  char *ReqMemory;
  char *ReqNodeCount;
  char *ReqNodeList;
  char *ReqOS;
  char *ReqTaskCount;
  char *ReqTPN;
  char *Resources;
  char *RsvAccessList;
  char *RsvGroup;
  char *RsvParent;
  char *SID;
  char *SubType;
  char *Trigger;
  char *Type;
  char *VMList;

  mstring_t *AllocNodeList;   /* allocated by 'new' */
  mstring_t *Variables;       /* allocated by 'new' */
  } mtransrsv_t;

/**
 * @struct mtranstrig_t
 * 
 * structure used for transitioning triggers to/from the 
 * database 
 *
 * NOTE: when adding an attribute to mtranstrig_t update the 
 * following: 
 *  - ensure there is a corresponding field in the Trigger
 *    table in the database
 *  - MTrigTransitionAllocate (if needed)
 *  - MTrigTransitionFree (if allocated)
 *  - MTrigTransition
 *  - MTrigToTransitionStruct
 *  - MTrigTransitionToXML
 */

#define MMAX_TRIG_DB_WRITE MMAX_NODE_DB_WRITE

typedef struct mtranstrig_t {

  char TrigID[MMAX_NAME];

  char *Name;
  char *ActionData;
  char *ActionType;
  char *Description;
  char *EBuf;
  char *EventType;
  char *IBuf;
  char *FailOffset;
  char *Flags;
  char *Message;
  char *MultiFire;
  char *ObjectID;
  char *ObjectType;
  char *OBuf;
  char *Offset;
  char *Period;
  char *Requires;
  char *Sets;
  char *State;
  char *Threshold;
  char *Unsets;

  int PID;
  long BlockTime;
  long EventTime;
  long LaunchTime;
  long RearmTime;
  long Timeout;
  mbool_t Disabled;
  mbool_t FailureDetected;
  mbool_t IsComplete;
  mbool_t IsInterval;
  } mtranstrig_t;

#define MMAX_VC_DB_WRITE MMAX_NODE_DB_WRITE

/**
 * @struct mtransvc_t
 *
 * structure used for transitioning VCs to/from the
 * database
 *
 * NOTE: when adding an attribute to mtranstrig_t update the
 * following:
 *  - ensure there is a corresponding filed in the VC table
 *    in the database
 *  - MVCTransitionAllocate (if needed)
 *  - MVCTransitionFree (if needed)
 *  - MVCTransition
 *  - MVCToTransitionStruct
 *  - MVCTransitionToXML
 */

typedef struct mtransvc_t {

  char Name[MMAX_NAME];

  char *Description;
  char Owner[MMAX_NAME + 10]; /* FORMAT <TYPE>:<NAME> */

  char Creator[MMAX_NAME];
  char CreateTime[20];

  mxml_t *ACL;
  mxml_t *Variables;
  mxml_t *MB;

  char *Jobs;
  char *Nodes;
  char *VMs;
  char *Rsvs;
  char *VCs;
  char *ParentVCs; /* Parent VCs, used to see if VC is top-level or not and checking ACLs */

  char Flags[MMAX_LINE];

  char *ReqStartTime;

  mbool_t IsDeleteRequest; /* if TRUE, this transvc is a delete request - removes transition from cache */
  } mtransvc_t;

#define MMAX_VM_DB_WRITE MMAX_NODE_DB_WRITE

  /**
 * @struct mtransvm_t
 * 
 * structure used for transitioning vm's to/from the 
 * cache 
 *
 * NOTE: when adding an attribute to mtransvm_t update the 
 * following: 
 *    table in the database
 *  - MVMTransitionAllocate (if needed)
 *  - MVMTransitionFree (if allocated)
 *  - MVMTransition
 *  - MVMToTransitionStruct
 *  - MVMTransitionToXML
 */

typedef struct mtransvm_t {

  char     VMID[MMAX_NAME];

  char     *JobID;
  char     *SubState;
  char     *LastSubState;
  char     *ContainerNodeName;
  char     *Description;
  char     *StorageRsvNames;
  char     *NetAddr;
  char     *Aliases;
  char     *GMetric;
  char     *TrackingJob;

  mbitmap_t    Flags; 
  mbitmap_t    IFlags;

  mulong    LastOSModRequestTime;
  mulong    LastMigrationTime;
  mulong    StartTime;
  mulong    LastSubStateMTime;

  enum MNodeStateEnum State;
  enum MPowerStateEnum PowerState;

  int       RADisk;
  int       RAMem;
  int       RAProc;
  int       RCDisk;
  int       RCMem;
  int       RCProc;
  int       RDDisk;
  int       RDMem;
  int       RDProc;
  int       Rack;
  int       Slot;
  int       ActiveOS;
  int       NextOS;
  int       LastOSModIteration;
  int       MigrationCount;

  double    CPULoad;
  double    NetLoad;

  mxml_t   *Variables;

  mbool_t   IsSovereign;

  long      LastUpdateTime;
  long      SpecifiedTTL;
  long      EffectiveTTL;

#ifdef MUSEMONGODB
  mongo::BSONObjBuilder *BSON;                     /* BSON to send to Mongo */
#endif
  } mtransvm_t;




#define MDB_MAX_LINE 128

#define MRESUTLSOFT 0
#define MRESUTLHARD 1

#ifndef MDEF_WATTCHARGERATE
#define MDEF_WATTCHARGERATE .000  /* price per KWH */
#endif


/**
 * Partition state/config flags
 *
 * @see struct mpar_t.Flags
 * NOTE: no limit on size
 *
 * sync with MParFlags[]
 */

enum MParFlagEnum {
  mpfNONE = 0,
  mpfUseMachineSpeed,
  mpfUseSystemQueueTime,
  mpfUseCPUTime,
  mpfRejectNegPrioJobs,
  mpfEnableNegJobPriority,
  mpfEnablePosUserPriority,
  mpfDisableMultiReqJobs,
  mpfUseLocalMachinePriority,
  mpfUseFloatingNodeLimits,
  mpfUseTTC,
  mpfSLURMIsShared,          /* select_type=cons_res */
  mpfDestroyIfEmpty,         /* free/destroy partition if no associated nodes exist */
  mpfIsDynamic,
  mpfIsRemote,               /* this partition contains remote peer resources */
  mpfSharedMem,
  mpfLAST };

#define MDEF_PARFLAGSIZE MBMSIZE(mpfLAST)



/**
 * @struct mpar_t
 *
 * This structure that holds all partition state, history, config, and performance information.
 *
 * NOTE: structure used for both VPC's and partitions.
 *
 * @see MPar[] - global partition table
 * @see MVPCPar[] - global VPC table
 *
 * @see MParUpdate() - updates partition ARes and CRes structures.
 * @see MParAdd() - create/add new partition
 * @see MParInitialize()
 * @see MParSetDefault()
 * @see MParProcessConfig()
 * @see MVPCUpdate()
 *
 * @see MVPCPAttr[] - vpc attributes
 *
 * NOTE: sync w/enum MParAttrEnum
 */

struct mpar_t {
  char   Name[MMAX_NAME + 1];

  int    Index;

  int    ConfigNodes;               /**< nodes configured */
  int    IdleNodes;                 /* any proc is idle                        */
  int    ActiveNodes;               /* any proc is busy                        */
  int    UpNodes;

  int    MaxPPN;                    /**< maximum procs per node detected in partition */

  mclass_t **CList;                 /* class list (alloc) */

  mcres_t    CRes;                  /* configured resources (updated in MParUpdate) */
  mcres_t    UpRes;                 /* up resources                            */
  mcres_t    ARes;                  /* available resources                     */
  mcres_t    DRes;                  /* dedicated resources                     */
  must_t     S;                     /* partition stats                         */

  char      *ConfigFile;            /* partition configuration file */

  mfsc_t     FSC;

  mcredl_t   L;                     /* partition per-cred limits */
  mfs_t      F;                     /* delivered fairshare */

  mrm_t     *RM;                    /* NOTE:  only one master RM per partition (ptr) */

  mnode_t   *RepN;                  /* representative node (pointer) */

  mulong     MigrateTime;           /* time of last job migration */

  int        AllocUpdateIteration;  /* iteration on which most recent provisioning step completed */

  int        ReqOS;                 /* required OS for provisioning */

  enum MParAllocPolicyEnum PAllocPolicy;  /* partition allocation algo */

  mbitmap_t Flags;  /**< rsv flags (bitmap of enum MParFlagEnum) */

  /* job priority factors */

  double  QTWeight;                 /* queue time weight */
  double  NPWeight;                 /* node prio weight */
  double  ATWeight;                 /* allocation time weight */
  double  SJWeight;                 /* # submitted jobs/iteration weight */

  long    Priority;                 /**< VPC Priority */

  mgcred_t *U;                   /**< VPC accountable user, group, account, qos (ptr) */
  mgcred_t *G;                   /**< NOTE:  accountable cred same as creation cred (ptr) */
  mgcred_t *A;                   /* (ptr) */
  mqos_t   *Q;                   /* (ptr) */

  enum MNodeSetPlusPolicyEnum NodeSetPlus;   /* extended node-set discovery and scheduling */

  struct mvpcp_t *VPCProfile;    /* VPC */

  mulong  PriorityTargetDuration;
  int     PriorityTargetProcCount;

  enum MFSTargetEnum PTDurationTargetMode;
  enum MFSTargetEnum PTProcCountTargetMode;

  enum MBFPolicyEnum BFPolicy;      /**< backfill policy                         */
  int  BFDepth;                     /**< depth queue will be searched            */
  enum MBFMetricEnum  BFMetric;     /**< backfill utilization metric             */
  int  BFProcFactor;                /**< factor to encourage use of large jobs   */
  int  BFMaxSchedules;              /**< maximum schedules to consider           */
  enum MBFPriorityEnum BFPriorityPolicy;
  int  BFChunkSize;
  long BFChunkDuration;
  long BFChunkBlockTime;          /* time at which jobs smaller than BFChunkSize may be considered */

  long   MinVirtualWallTime;
  double VirtualWallTimeScalingFactor;
  enum MVWConflictPolicyEnum VWConflictPolicy;

  long    NodePowerOnDuration;      /**< approximate time required for a powered on node to be usable */
  long    NodePowerOffDuration;      /**< approximate time required for a powered off node to be usable */
  long    VMMigrationDuration;      /**< approximate time required to migrate a VM */
  long    VMCreationDuration;      /**< approximate time required to create a VM */
  long    VMDeletionDuration;      /**< approximate time required to delete a VM */

  double WattChargeRate;        /**< charge rate for watts */

  double  BacklogPS;    /* number of remaining queued and active job PS */
  long    BacklogDuration; /* estimated time to execute backlog (incorporates
                              wcaccuracy, sched efficiency, resource state) */

  double  IterationBacklogPS; /* current iteration's backlog, used for dynamic partition scheduling */
  int     IterationIdleJC;    /* current iteration's idle job count, used for dynamic partition scheduling */
  int     IterationIdleNC;    /* current iteration's idle node count, used for dynamic partition scheduling */

  int     IterationIdleTC;    /* current iteration's idle task/proc count, used for dynamic partition scheduling */
  int     IterationMinJobPC;  /* procs required by partition's smallest eligible job */

  mbitmap_t FBM; /* node feature (BM)                   */
  mbitmap_t Classes; /**< configured classes in this partition */

  /* system policies */

  int MaxMetaTasks;

  int MaxJobStartTime;            /* time allowed for nodes to become busy     */
  enum MNodeAccessPolicyEnum      NAccessPolicy;
  enum MNodeAllocPolicyEnum       NAllocPolicy; /* algo for allocating nodes to jobs */
  enum MNodeAllocPolicyEnum       VPCNAllocPolicy; /* algo for allocating nodes to vpcs */
  enum MNodeAllocPolicyEnum       RsvNAllocPolicy; /* algo for allocating nodes to rsvs */

  mnprio_t *RsvP; /* policies for node priority (alloc) */

  enum MAllocResFailurePolicyEnum ARFPolicy;    /* policy for handling node failures on alloc nodes */

  enum MTaskDistEnum DistPolicy;
  mbitmap_t JobNodeMatchBM;          /* bitmap of enum MNodeMatchPolicyEnum       */

  enum MRsvPolicyEnum RsvPolicy;  /* policy for creating/managing rsv          */

  mbool_t RestrictedRsvRetryTime; /* restricted reservations retry (see MJobStartRsv()) */
  mulong RsvRetryTime;            /* time rsvs are retried if blocked by system problems */
  mulong JobRetryTime;            /* time jobs are retried if blocked by transient failures */
                                  /* (including start failure and reject) */

  enum MRsvThresholdEnum RsvTType; /* reservation threshold type               */
  int RsvTValue;                   /* reservation threshold value              */

  mrsv_bucket_t *QOSBucket;        /* terminated w/QOSBucket[X].Name[0] == '\0' */
  int           ParBucketRsvDepth;

  double        DefNodeActiveWatts;
  double        DefNodeIdleWatts;
  double        DefNodeStandbyWatts;

  double        TotalWattsInUse;  /* watts consumed by all resources in partition */

  enum MJobPrioAccrualPolicyEnum JobPrioAccrualPolicy;
  mbitmap_t                      JobPrioExceptions;                    /* (bitmap of MJobPrioExceptionEnum) */

  enum MResourceAvailabilityPolicyEnum NAvailPolicy[mrLAST];

  /* resource limit handling (support hard and soft limits)  */
  /* index 0 = soft, index 1 = hard */

  double ResourceLimitMultiplier[mrlLAST];

  enum MResLimitPolicyEnum ResourceLimitPolicy[2][mrlLAST];
  mbitmap_t                ResourceLimitViolationActionBM[2][mrlLAST]; /* (bitmap of enum MResLimitVActionEnum) */
  long                     ResourceLimitMaxViolationTime[2][mrlLAST];

  long   AdminMinSTime;    /* min time before an admin-suspended job becomes eligible to resume */
  long   JobMinSTime;      /* min time before a job-suspended job becomes eligible to resume */

  double UntrackedProcFactor;

  double DefNodeChargeRate;      /* default node charge rate (may be overridden on node-by-node basis) */

  /* node set attributes */

  enum MResourceSetSelectionEnum NodeSetPolicy;
  enum MResourceSetTypeEnum      NodeSetAttribute;
  enum MResourceSetPrioEnum      NodeSetPriorityType;

  mbool_t NodeSetForceMinAlloc;   /* force job to allocate nodes within
                                     minimum number of nodesets */

  int    NodeSetMaxSize;          /* size of largest feasible nodeset */

  char  *NodeSetList[MMAX_ATTR];  /* list of set delineating job attributes     */
  double  NodeSetListMaxUsage[MMAX_ATTR];     /* like NodeSetMaxUsage, but on a per-nodeset basis */
  mbool_t NodeSetMaxUsageSet;     /* Used to tell if a NodeSetListMaxUsage was set */
  long   NodeSetDelay;
  mbool_t NodeSetIsOptional;
  mbool_t SetBlock;

  double NodeSetTolerance;
  double NodeSetMaxUsage;         /* primarily used for smps, but puts a limit of how much the node's resources the job can use */

  long   NodeBusyStateDelayTime;  /* minimum time a busy node should be considered unavailable */
  long   NodeDownStateDelayTime;  /* minimum time a down node should be considered unavailable */
  long   NodeDrainStateDelayTime; /* minimum time a drain node should be considered unavailable */
  long   UntrackedResDelayTime;   /* minimum time that untracked resources should be considered unavailable */

  char  *Message;                 /* event messages (alloc) */

  mbool_t SuspendRes[mrLAST];     /* dedicated suspend resources                */

  /* VPC attributes */

  mrsv_t *VPCMasterRsv;           /* first VPC rsv found, not always rsv group leader (-1 for not set) */

  char   *MShowCmd;               /* mshow command used to generate this vpc */

  mpsi_t *P;                      /* peer service associated w/requestor (ptr) */

  /* NOTE:  partition IsShared indicated by MSched.SharedPtIndex */

  void  *O;                       /* owner */
  char   OID[MMAX_NAME];          /* owner ID */
  enum MXMLOTypeEnum OType;       /* owner type */

  mulong ReqStartTime;            /* requested start time */
  mulong ReqDuration;             /* requested duration */

  mulong ProvStartTime;           /* time first resources become allocated */
  mulong ProvEndTime;             /* time last resources become released */

  char   *ReqResources;           /* requested resources (alloc) */

  char   *RsvGroup;               /* rsv group id (alloc) */

  enum MClusterStateEnum State;

  mbool_t IsVPC;

  mulong VPCFlags;                /**< VPC flags BM of MCModeEnum */

  mulong PurgeTime;               /**< earliest time VPC can be removed */
  mulong PurgeDuration;           /**< duration VPC should persist after completion */

  mmb_t  *MB;                     /**< general message buffer (alloc) */

  double  Cost;                   /**< vpc cost - calculated in MUIVPCCreate() and
                                       __MUISchedCtlModify() */

  mln_t  *Variables;              /**< WARNING: for internal use ONLY.  Any variables created externally
                                       will go on the reservation group and NOT on the mpar_t object.
                                       These variables are being used for DISA2 to keep track of some
                                       DISA2 specific variables (see MLocalRsvGetDISACost()) */

  class PluginNodeAllocate *NodeAllocationPlugin;
  };




#define MIN_FUTURERSVTIME 5
                            /* earliest time (in seconds) into the future to attempt
                               to reserve resources for job which cannot immediately
                               start */



/**
 * Virtual Cluster Profile
 *
 * NOTE:  sync w/enum MVPCPAttrEnum
 *
 * NOTE:  packages are used in MClusterShowARes() to process co-alloc dependencies
 * NOTE:  package info is stored in TID (see mtransaVPCProfile)
 * NOTE:  packages are used in MUIVPCCreate() to build committed VPC's
 * NOTE:  package info is stored in VPC (see struct mpar_t.ProfileID)
 *
 * @see MClusterProcessConfig()
 * @see MVPCProfileToString()
 * @see MVPCProfileFind() - locate profile
 *
 * NOTE:  VPC's maintained via struct mpar_t structures
 */

typedef struct mvpcp_t {
  char          Name[MMAX_NAME];

  int           Index;         /* index in global MVCP table */

  char         *Description;   /* public cluster description (alloc) */

  macl_t       *ACL;           /* users, groups, accounts, etc which can view and utilize VPC profile */

  mln_t        *QueryData;     /* linked list of mxml_t data (alloc) used to create co-allocation
                                  'where' reqs beyond what was explicitly specified */

  char         *ReqSetAttrs;   /* list of forced attributes to modify explicitly specified reqs */
  char         *ReqDefAttrs;   /* list of default attributes to modify explicitly specified reqs */

  char         *OpRsvProfile;  /* (alloc) */

  long          Priority;

  /* opaque attributes */

  char         *OptionalVars;

  /* policies */

  mulong        MinDuration;
  mbitmap_t     Flags;         /**< flags affecting VPC creation/behavior (bitmap of enum MVPCPFlagEnum) */

  /* time constraints/adjustments */

  mulong        ReqStartPad;   /**< start pad for all explicitly specified reqs */
  mulong        ReqEndPad;     /**< end pad for all explicitly specified reqs */

  mulong        PurgeDuration; /**< duration to keep VPC record after completion */

  /* charge/costing info */

  double        NodeHourAllocChargeRate;  /* charge rate adjustor */
  double        NodeSetupCost;            /* per node overhead cost to setup/destroy VPC */
  double        SetUpCost;                /* per VPC cost to setup/destroy */

  /* resource allocation */

  enum MRsvAllocPolicyEnum ReAllocPolicy; /**< re-allocation policy to handle failures, conflicts, etc */
  /* NOTE:  ReAllocPolicy copied into R->AllocPolicy of VPC child rsv's */
  } mvpcp_t;



#define MDEF_RESOURCELIMITMULTIPLIER        1.1
#define MDEF_RESOURCELIMITPOLICY            mrlpNONE
#define MDEF_RESOURCELIMITRESOURCES         mrlrProcs|mrlrMem
#define MDEF_RESOURCELIMITVIOLATIONACTION   mrlaRequeue

#define MDEF_SPVJOBISPREEMPTIBLE            TRUE
#define MDEF_ADJUSTUTLRESOURCES             FALSE

/* node access policy */

#define MDEF_GNNAME  "GLOBAL"


#ifndef __M_H
#define __M_H
#endif /* !__M_H */

#ifdef __M_H
#include "msched.h"
#endif /* __M_H */

#if defined (__MMEMTEST)
#ifndef MEMWATCH
# define MEMWATCH
# include "memwatch.h"
#endif /* MEMWATCH */

#ifndef MEMWATCH_STDIO
# define MEMWATCH_STDIO
#endif /* MEMWATCH_STDIO */
#endif /* __MMEMTEST */

#ifndef __M_H
#define __M_H
#endif /* !__M_H */

#ifndef mulong
# define mulong unsigned long
#endif /* mulong */

#ifndef mutime
# define mutime unsigned long
#endif /* mutime */

#if !defined(MDEF_SERVERHOST)
# define MDEF_SERVERHOST              ""
#endif /* !MDEF_SERVERHOST */

#if !defined(MDEF_SERVERPORT)
# define MDEF_SERVERPORT                   42559
#endif /* !MDEF_SERVERPORT */

#define MDEF_SERVERMODE                    msmNormal

#ifndef MYAHOO
#define MMAX_PRIO_VAL                      1000000000
#define MCONST_PRIO_NOTSET                -1000000001  /* out of range value which cannot be specified */
#else
#ifdef __M64
#define MMAX_PRIO_VAL                      0x0fffffffffffffff /* warning - this value is very large and causes problems with double values cast to a long (e.g. setspri) */
#define MCONST_PRIO_NOTSET                 (0x0fffffffffffffff * -1L) /* out of range value which cannot be specified */
#else
#define MMAX_PRIO_VAL                      1000000000
#define MCONST_PRIO_NOTSET                -1000000001  /* out of range value which cannot be specified */
#endif /* __M64 */
#endif /* MYAHOO */

#define MDEF_REJECTNEGPRIOJOBS             FALSE

#define MMAX_SPSLOT                        16

#define MCONST_CPTERMINATOR "MOABCHECKPOINTEND"
#define MCONST_CPTERMINATOR2 "MOABCHECKPOINTSUCCESS"

/* server checkpointing object */

typedef struct mckpt_t {
  char  CPFileName[MMAX_LINE];

  /* simulation/test behavior */

  mbool_t EnableTestRead;
  mbool_t EnableTestWrite;
  mbool_t UseDatabase;

  char *Buffer;               /* (alloc) */

  mulong CPInterval;          /* seconds between subsequent checkpoints        */
  mulong CPExpirationTime;    /* time stale checkpoint data will be kept (configurable) */
  mulong CPJobExpirationTime; /* time stale *job* checkpoint data will be kept (not configurable) */
  mulong LastCPTime;          /* time of most recent checkpoint                */
  mulong CPInitTime;          /* time CP was initialized                       */

  char  DVersion[MMAX_NAME];      /* detected data version */
  char  SVersionList[MMAX_LINE];  /* supported versions    */
  char  WVersion[MMAX_NAME];      /* write version         */

  FILE *fp;                       /* file pointer to the main checkpoint file */
  FILE *JournalFP;                /* file pointer to the checkpoint journal */

  char *OBuffer;              /* (alloc) */

  mstring_t OldNodeBuffer;
  mstring_t OldJobBuffer;

  mbool_t OldNodeBufferIsInitialized;
  mbool_t OldJobBufferIsInitialized;

  mbool_t UseCPJournal;   /* whether to use CP journaling for "quick saves" (config) */
  mbool_t CheckPointAllAccountLists; /* specifies whether to specify Account lists besides those added at runtime */
  } mckpt_t;


/* CP types (sync w/MCPType[]) */

enum MCkptTypeEnum {
  mcpSched = 0,
  mcpJob,
  mcpRsv,
  mcpSRsv,
  mcpNode,
  mcpUser,
  mcpGroup,
  mcpAcct,
  mcpTotal,
  mcpRTotal,
  mcpCTotal,
  mcpGTotal,
  mcpSysStats,
  mcpRM,
  mcpAM,
  mcpSys,
  mcpTrigger,
  mcpQOS,
  mcpClass,
  mcpSTrig,
  mcpPar,
  mcpCJob,
  mcpCJobList,
  mcpCVM,
  mcpTJob,
  mcpDTJob,
  mcpTrans,
  mcpVC,
  mcpVM,  /* VM */
  mcpLAST };



/* general objects */

/* reasons for requirements rejection */

/* sync w/MAllocRejType[] */

enum MAllocRejEnum {
  marNONE = 0,
  marAccount,
  marACL,
  marArch,
  marClass,
  marCorruption,
  marCPU,
  marData,
  marDepend,
  marDisk,
  marDynCfg,
  marEState,
  marFairShare,
  marFeatures,
  marGRes,  /* license/gres */
  marGroup,
  marHold,
  marHostList,
  marLocality,
  marMemory,
  marNodeCount,
  marNodePolicy,
  marNodeSet,
  marNoSubmit,
  marOpsys,
  marPartition,
  marPolicy,
  marPool,
  marPriority,
  marQOS,
  marRelease,
  marRM,
  marState,
  marSwap,
  marSystemLimits,
  marTaskCount,  /* cannot satisfy total taskcount or TPN constraint */
  marTime,
  marUser,
  marVM,
  marLAST };

#define MDEF_ALLOCREJBMSIZE MBMSIZE(marLAST)


enum MAccessModeEnum {
  macmNONE = 0,
  macmRead,
  macmWrite };

enum MConstraintTypeEnum {
  mcsNONE2 = 0,
  mcsLimits,
  mcsFS,
  mcsGeneral,
  mcsUsage,
  mcsLAST2 };


/* experienced values */

/**
 * NOTE:  sync w/MAttrType[] 
 * 
 * WARNING: meLAST must be less than MMAX_LIST 
 */

enum MExpAttrEnum {
  meNFeature = 0,  /* node features */
  meOpsys,
  meArch,
  meGRes,          /* generic consumable resource */
  meJFeature,      /* job features */
  meGEvent,        /* generic event */
  meGMetrics,
  meLAST };

typedef char mattrlist_t[meLAST][MMAX_ATTR][MMAX_NAME];


/* sync w/MJobStartFlags[] */

enum MJobStartFlagsEnum {
  mjsfNONE = 0,
  mjsfIgnNState,
  mjsfIgnRsv,
  mjsfIgnPolicies,
  mjsfLAST };


/* sync w/MJobSubmitFlags[] */

enum MJobSubmitFlagsEnum {
  mjsufNONE = 0,
  mjsufDSDependency,   /**< job has data staging dependency - should not be started until it is fulfilled */
  mjsufDataOnly,       /**< job's RMSubmitString should be written to a temporary file */
  mjsufGlobalQueue,
  mjsufNoEventLogs,    /**< set the NoEventLogs internal flag on the job being created */
  mjsufStartNow,       /**< job must start immediately or stage request should be rejected */
  mjsufSyncJobID,      /**< job ID is propogated to destination RM */
  mjsufTemplateJob,    /**< job is system job */
  mjsufUseJobName,     /**< ??? */
  mjsufTemplateDepend, /**< job is being created by a template dependency */
  mjsufLAST };


/* no sync object */

enum MStatusCodeEnum {
  mscNoError = 0,
  mscFuncFailure,   /* ??? */
  mscBadParam,      /* invalid parameters */
  mscBadState,      /* operation not possible due to state of object */
  mscNoAuth,        /* request not authorized */
  mscNoEnt,         /* entity does not exist */
  mscNoFile,        /* file does not exist or file cannot be created */
  mscNoMemory,      /* inadequate memory (ex. bufsize too small) */
  mscRemoteFailure, /* remote service failed */
  mscRemoteFailureTransient, /* remote service failed (due to temp issue) */
  mscSysFailure,    /* system call failed */
  mscTimeout,       /* request timed out */
  mscBadRequest,    /* request is corrupt */
  mscBadResponse,   /* response is corrupt */
  mscNoData,        /* data not yet available */
  mscPartial,       /* request partially failed */
  mscPending,       /* operation could not proceed because of "pending" response (not really an error) */
  mscLAST };


/* node failures */

enum MNodeFailure {
  mnfLocalDisk = 0,
  mnfMemory,
  mnfEthernet,
  mnfHPS_IP,
  mnfHPS_User,
  mnfSwap };

/* specific network types */

#define  MLLHPNETWORK  "hps_user"



/* sync w/mpsi_t,MOServAttr[] */

enum MOServiceAttrEnum {
  mosaNONE = 0,
  mosaAuth,
  mosaAuthType,
  mosaCSAlgo,    /* deprecated, use AuthType */
  mosaCSKey,     /* deprecated, use Key */
  mosaDropBadRequest,
  mosaHost,
  mosaKey,
  mosaPort,
  mosaProtocol,
  mosaVersion };


/* acl object (sync w/MACLAttr[]) */

enum MACLAttrEnum {
  maclaNONE = 0,
  maclaAffinity,
  maclaComparison,
  maclaDValue,      /* conditional value (double) */
  maclaFlags,
  maclaName,
  maclaType,
  maclaValue,
  maclaLAST };



/**
 * Account-specific config attributes (sync w/MAcctAttr[])
 *
 * @see struct mgcred_t
 * @see enum MCredAttrEnum
 */

enum MAcctAttrType {
  maaNONE = 0,
  maaVDef,     /* default VPC profile */
  maaLAST };

#define MDEF_ACCTATTRBMSIZE MBMSIZE(maaLAST)


/**
 * Group-specific config attributes (sync w/MGroupAttr[])
 *
 * @see struct mgcred_t
 * @see enum MCredAttrEnum
 */

enum MGroupAttrType {
  mgaNONE = 0,
  mgaClassWeight,
  mgaLAST };

#define MDEF_GROUPATTRBMSIZE MBMSIZE(mgaLAST)

/**
 * User-specific config attributes (sync w/MUserAttr[])
 *
 * @see struct mgcred_t
 * @see enum MCredAttrEnum
 */

enum MUserAttrEnum {
  muaNONE = 0,
  muaAdminLevel,
  muaJobNodeMatchPolicy,
  muaMaxWCLimit,
  muaNoEmail,       /* User cannot send e-mail */
  muaProxyList,
  muaViewpointProxy,
  muaPrivileges,
  muaLAST };

#define MDEF_USERATTRBMSIZE MBMSIZE(muaLAST)

#define MDEF_QOSFLOWPERIOD    mpDay
#define MDEF_QOSFLOWCOUNT     7
#define MDEF_QOSFLOWISSLIDING TRUE

/**
 * QoS object 
 *
 * NOTE:  If adding new attribute, update the following:
 *   MQOSShow()
 *   MQOSAToString()
 *   MQOSSetAttr()
 *   MQOSProcessConfig()
 *
 * @see MQOSAttr[] (sync)
 * @see struct mqos_t
 */

enum MQOSAttrEnum {
  mqaNONE = 0,
  mqaDynAttrs,       /* job attributes which can be dynamically modified by user */
  mqaPreemptMaxTime, /* time at which job is no longer eligible for preemption */
  mqaPreemptMinTime, /* minimun time job must run before being considered for preemption */
  mqaPreemptPolicy,
  mqaPriority,
  mqaMaxJob,
  mqaMaxProc,
  mqaMaxNode,
  mqaXFWeight,
  mqaQTWeight,
  mqaXFTarget,
  mqaQTTarget,
  mqaFlags,
  mqaFSScalingFactor,
  mqaFSTarget,
  mqaBLPreemptThreshold,
  mqaQTPreemptThreshold,
  mqaXFPreemptThreshold,
  mqaBLRsvThreshold,
  mqaQTRsvThreshold,
  mqaXFRsvThreshold,
  mqaBLACLThreshold,  /* backlog based ACL threshold */
  mqaQTACLThreshold,  /* queue time based ACL threshold */
  mqaXFACLThreshold,
  mqaBLPowerThreshold,
  mqaQTPowerThreshold,
  mqaXFPowerThreshold,
  mqaBLTriggerThreshold,
  mqaQTTriggerThreshold,
  mqaXFTriggerThreshold,
  mqaDedResCost,
  mqaUtlResCost,
  mqaRsvAccessList,   /* list of rsv's and rsvgroup's requestor can access */
  mqaRsvCreationCost, /* charge associated w/creating a reservation */
  mqaRsvPreemptList,  /* list of rsv's and rsvgroup's requestor can preempt */
  mqaRsvRlsMinTime,   /* minimum time prior to start a rsv can be released w/o charge */
  mqaRsvTimeDepth,    /* max time into the future a job rsv is allowed to be scheduled */
  mqaBacklog,
  mqaOnDemand,
  mqaSpecAttrs,       /**< job attributes which can be specified by user */
  mqaGreenBacklogThreshold,
  mqaGreenQTThreshold,
  mqaGreenXFactorThreshold,
  mqaExemptLimit,     /* Exempt from a limit (MActivePolicyTypeEnum) */
  mqaSystemPrio,      /* Sets the sysprio on jobs, like setspri */
  mqaJobPrioAccrualPolicy,
  mqaJobPrioExceptions,
  mqaPreemptees,
  mqaLAST };


#define MDEF_QOSATTRBMSIZE MBMSIZE(mqaLAST)


/* trig policy flags (sync w/MTrigPolicyFlag[]) */

enum MTrigPolicyFlagEnum {
  mtpfNONE = 0,
  mtpfRemoveParentJobOnCompletion,
  mtpfRemoveTrigOnNotSetViolation,
  mtpfReprocessOnVarSet,
  mtpfLAST };

/* trigger object (sync w/MTrigAttr[], mtrig_t) */

enum MTrigAttrEnum {
  mtaNONE = 0,
  mtaActionData,
  mtaActionType,
  mtaBlockTime,
  mtaDependsOn,
  mtaDescription,
  mtaDisabled,
  mtaEBuf,
  mtaECPolicy,
  mtaEnv,
  mtaEventTime,
  mtaEventType,
  mtaExpireTime,
  mtaFailOffset,
  mtaFailureDetected,
  mtaFlags,            /* enable interval, user, multifire, probe, probeall */
  mtaIBuf,
  mtaIsComplete,
  mtaIsInterval,
  mtaLaunchTime,
  mtaMaxRetry,
  mtaMessage,
  mtaMultiFire,
  mtaName,
  mtaObjectID,
  mtaObjectType,
  mtaOBuf,
  mtaOffset,
  mtaPeriod,
  mtaPID,
  mtaRearmTime,
  mtaRequires,
  mtaSets,
  mtaState,
  mtaStdIn,
  mtaThreshold,
  mtaTrigID,
  mtaTimeout,
  mtaUnsets,
  mtaRetryCount,
  mtaLAST };


/**
 * History attribute enum
 *
 * @see MHistoryAttr[] - sync
 * @see struct mhistory_t - sync
 */

enum MHistoryAttrEnum {
  mhaNONE = 0,
  mhaElementCount,
  mhaTime,
  mhaState,
  mhaLAST };


/**
 * Reservation attribute enum
 *
 * @see MRsvAttr[] - sync
 * @see struct mrsv_t - sync
 */

enum MRsvAttrEnum {
  mraNONE = 0,
  mraAAccount,  /**< accountable account */
  mraACL,       /**< @see also mraCL */
  mraAGroup,    /**< accountable group */
  mraAllocNodeCount,
  mraAllocNodeList,
  mraAllocProcCount,
  mraAllocTaskCount,
  mraAQOS,      /**< accountable QoS */
  mraAUser,     /**< accountable user */
  mraCL,        /**< credential list */
  mraComment,   /**< opaque reservation comment string */
  mraCost,      /**< rsv AM lien/charge */
  mraCreds,     /**< access credentials */
  mraCTime,     /**< creation time */
  mraDuration,
  mraEndTime,
  mraExcludeRsv,
  mraExpireTime, /* when non-committed reservation expires and vacates */
  mraFlags,
  mraGlobalID,  /* global rsv id */
  mraHistory,
  mraHostExp,   /* host expression (describing hostnames or resource requirements) */
  mraHostExpIsSpecified, /* host expression was requested (not generated) */
  mraIsActive,  /* reservation is active at current time */
  mraIsTracked, /* reservation resource usage is tracked */
  mraJProfile,  /* job profile */
  mraJState,    /* job state (for job reservations only) */
  mraLabel,     /* reservation label */
  mraLastChargeTime, /* time rsv charge was last flushed */
  mraLogLevel,  /* reservation loglevel */
  mraMaxJob,
  mraMessages,
  mraName,      /* RID */
  mraNodeSetPolicy,
  mraOwner,
  mraPartition,
  mraPriority,
  mraProfile,
  mraReqArch,
  mraReqFeatureList,
  mraReqFeatureMode,
  mraReqMemory,
  mraReqNodeCount,
  mraReqNodeList,
  mraReqOS,
  mraReqVC,      /* requested VC */
  mraResources,  /* task description */
  mraRsvAccessList, /* list of rsv's and rsv groups which can be accessed */
  mraRsvGroup,   /* reservation group to which reservation belongs */
  mraRsvParent,  /* reservation parent (used by standing reservations) */
  mraSID,        /* rsv's system id (parent cluster) */
  mraSpecName,   /* user specified rsv name */
  mraStartTime,
  mraStatCAPS,   /* current active ps */
  mraStatCIPS,   /* current idle ps */
  mraStatTAPS,
  mraStatTIPS,
  mraSubType,    /* reservation subtype (refereneced only via R->J job object) */
  mraSysJobID,   /**< id of system job map */
  mraSysJobTemplate,
  mraReqTaskCount,
  mraReqTPN,
  mraTrigger,
  mraType,
  mraUIndex,     /* globally unique reservation index */
  mraVariables,  /* variables */
  mraVMList,
  mraVMUsagePolicy,
  mraLAST };


/* nres object */

enum MNResAttrType {
  mnraNONE = 0,
  mnraDRes,
  mnraEnd,
  mnraName,
  mnraState,
  mnraStart,
  mnraTC,
  mnraType };


/* sync w/MNotifyType[] */

enum MNotifyEnum {
  mntNONE = 0,
  mntJobStart,
  mntJobEnd,
  mntJobFail,
  mntAll,
  mntLAST };

/**
 *  Event logging
 */

/* Event Log Entity
 *  These are the types of the main sections of XML/JSON that are passed
 *  to event logs using the web service.  These are not attributes.
 *
 * sync w/MEventLogEntityType[]
 */

enum MEventLogEntityEnum {
  meleNONE = 0,
  meleEvent,
  melePrimaryObject,
  meleRelatedObject,
  meleRelatedObjects,
  meleSerializedObject,
  meleInitiatedBy,
  meleErrorMessage,
  meleDetail,
  meleDetails,
  meleLAST };

/* Event Log Attribute
 *  These are attributes in the XML/JSON for event logs.
 *
 * sync w/MEventLogAttributeType[]
 */

enum MEventLogAttributeEnum {
  melaNONE = 0,
  melaEventTime,
  melaEventType,
  melaEventCategory,
  melaSourceComponent,
  melaStatus,
  melaFacility,
  melaObjectType,
  melaObjectID,
  melaInitiatedByUser,
  melaProxyUser,
  melaOriginator,
  melaErrorCode,
  melaMessage,
  melaName,
  melaLAST };


/**
 * event types
 *
 * @see MRecordEventType[] - sync
 * @see MDEF_RELISTBITMAP - default events
 *
 * NOTE:  no maximum number of entries 
 *
 * WARNING!!!!  Any changes here have to be synced with the database.  These are used for lookups in the 
 *              database so don't make them alphabetical, just append to the end to not invalidate the database
 */

enum MRecordEventTypeEnum {
  mrelNONE = 0,
  mrelGEvent,
  mrelJobCancel,
  mrelJobComplete,  /* turned on with JOBEND */
  mrelJobFailure,   /* currently never triggered in live code */
  mrelJobHold,
  mrelJobMigrate,
  mrelJobModify,
  mrelJobPreempt,
  mrelJobReject,
  mrelJobResume,
  mrelJobStart,
  mrelJobSubmit,
  mrelJobUpdate,
  mrelJobVarSet,   /* handled in MJobWriteVarStats() */
  mrelJobVarUnset, /* handled in MJobWriteVarStats() */
  mrelNodeDown,
  mrelNodeFailure,
  mrelNodeUp,
  mrelNodeUpdate,
  mrelNote,
  mrelQOSViolation,
  mrelRMDown,
  mrelRMUp,
  mrelRsvCancel,
  mrelRsvCreate,
  mrelRsvEnd,
  mrelRsvModify,
  mrelRsvStart,
  mrelSchedCommand,
  mrelSchedFailure,
  mrelSchedModify,
  mrelSchedPause,
  mrelSchedRecycle,
  mrelSchedResume,
  mrelSchedStart,
  mrelSchedStop,
  mrelTrigCreate,
  mrelTrigEnd,
  mrelTrigFailure,
  mrelTrigStart,  /* 34 */
  mrelTrigThreshold,
  mrelVMCreate,
  mrelVMDestroy,
  mrelVMMigrate,
  mrelVMPowerOn,
  mrelVMPowerOff,
  mrelNodeModify,
  mrelNodePowerOff,
  mrelNodePowerOn,
  mrelNodeProvision,
  mrelAllSchedCommand, /* Show all sched commands, even info queries */
  mrelAMDelete,
  mrelAMEnd,
  mrelAMQuote,
  mrelAMStart,
  mrelRMPollEnd,
  mrelRMPollStart,
  mrelSchedCycleEnd,
  mrelSchedCycleStart,
  mrelJobCheckpoint,
  mrelNodeDestroy,
  mrelAMCreate,
  mrelAMUpdate,
  mrelAMPause,
  mrelAMResume,
  mrelAMModify,
  mrelClientCommand,
  mrelLAST };

#define MDEF_RELSIZE MBMSIZE(mrelLAST)

#define MDEF_RELISTBITMAP "jobcancel,jobend,jobstart,schedpause,schedstart,schedstop,trigend,trigfailure,trigstart,nodedestroy"

/* sync w/ MEventLogType[], GetCategoryAndFacility() */

enum MEventLogTypeEnum {
  meltNONE = 0,
  meltJobCancel,
  meltJobEnd,
  meltJobHold,
  meltJobModify,
  meltJobReject,
  meltJobRelease,
  meltJobStart,
  meltJobSubmit,
  meltRsvCreate,
  meltRsvEnd,
  meltRsvStart,
  meltSchedCommand,
  meltSchedCycleEnd,
  meltSchedCycleStart,
  meltSchedPause,
  meltSchedRecycle,
  meltSchedResume,
  meltSchedStart,
  meltSchedStop,
  meltTrigCreate,
  meltTrigEnd,
  meltTrigStart,
  meltVMCancel,
  meltVMDestroy,
  meltVMEnd,
  meltVMMigrateEnd,
  meltVMMigrateStart,
  meltVMReady,
  meltVMSubmit,
  meltLAST };


/* sync w/ MEventLogFacility[] */

enum MEventLogFacilityEnum {
  melfNONE = 0,
  melfJob,
  melfRsv,
  melfScheduler,
  melfTrigger,
  melfVM,
  melfLAST };

/* sync w/ MEventLogCategory[] */

enum MEventLogCategoryEnum {
  melcNONE = 0,
  melcCancel,
  melcCommand,
  melcCreate,
  melcDestroy,
  melcEnd,
  melcHold,
  melcMigrate,
  melcModify,
  melcPause,
  melcReady,
  melcRecycle,
  melcReject,
  melcRelease,
  melcResume,
  melcStart,
  melcStop,
  melcSubmit,
  melcLAST };


/*
                          (1 << mrelJobCancel)|(1 << mrelJobComplete)|(1 << mrelJobStart)| \
                          (1 << mrelSchedPause)|(1 << mrelSchedStart)|(1 << mrelSchedStop)| \
                          (1 << mrelTrigEnd)|(1 << mrelTrigFailure)|(1 << mrelTrigStart)
*/

enum MJobNotifySentEnum {
  mjnsNONE = 0,
  mjnsCancelDelay,   /* job cancel is failing */
  mjnsLAST };



/* sync w/MVPCPFlags[] */

enum MVPCPFlagEnum {
  mvpcfNONE = 0,
  mvpcfAutoAccess,
  mvpcfLAST };



/**
 * VPC Attributes
 *
 * NOTE: sync w/MVPCPAttr[],mvpcp_t
 *
 * @see struct mvpcp_t
 * @see MClusterLoadConfig() - processes VPC config parameters
 */

enum MVPCPAttrEnum {
  mvpcpaNONE = 0,
  mvpcpaACL,
  mvpcpaCoAlloc,       /**< NOTE: synonym for QueryData - co-allocation requirement */
  mvpcpaDescription,
  mvpcpaFlags,         /**< flags affecting creation of resulting VPC */
  mvpcpaMinDuration,
  mvpcpaName,
  mvpcpaNodeHourChargeRate,
  mvpcpaNodeSetupCost,
  mvpcpaOpRsvProfile,  /**< optional reservation profile */
  mvpcpaOptionalVars,
  mvpcpaPriority,
  mvpcpaPurgeDuration,
  mvpcpaQueryData,     /**< NOTE: deprecated - see CoAlloc */
  mvpcpaReallocPolicy, /**< @see struct mvpcp_t.ReAllocPolicy */
  mvpcpaReqDefAttr,    /**< default attributes to apply to ALL explicitly specified requirements */
  mvpcpaReqSetAttr,    /**< attributes to apply to ALL explicitly specified requirements */
  mvpcpaReqEndPad,
  mvpcpaReqStartPad,
  mvpcpaSetupCost,
  mvpcpaLAST };



/**
 * Stat Object
 *
 * @see MStatAttr[] (sync)
 * @see struct must_t (sync)
 * @see enum MStatAttrTypeEnum (sync) - stat metric type (int, long, double, or string)
 * @see enum MNodeStatTypeEnum - peer
 *
 * @see MStatToXML() - reports stats in XML format
 * @see MCredProfileToXML() - process 'mcredctl -q profile'
 *
 * NOTE:  stat object present in creds, job templates, etc
 *
 * NOTE:  number of samples in record: IC (mstaIterationCount)
 */

enum MStatAttrEnum {
  mstaNONE = 0,
  mstaAvgQueueTime,   /* Job weighted average qtime, TotalQTS / JC */
  mstaAvgBypass,      /* Job weighted average bypass count, Bypass / JC */
  mstaAvgXFactor,     /* PS weighted average xfactor, PSXFactor / PSRun */
  mstaTANodeCount,    /* total active node count */
  mstaTAProcCount,    /* total active processor count */
  mstaTCapacity,      /* nodes available of parent object (currently only set on class) */
  mstaTJobCount,      /* total jobs finished during period, |J| -> JC */
  mstaTNJobCount,     /* node weighted finished job count, SUM(NC[J]) */
  mstaTQueueTime,     /* total queuetime of finished jobs, SUM(QT[J]) */
  mstaMQueueTime,     /* max queuetime of finished jobs,   MAX(QT[J]) */
  mstaTReqWTime,      /* total requested walltime for finished jobs, SUM(WCLIMIT[j]) */
  mstaTExeWTime,      /* total run walltime for finished jobs, SUM(CTIME[J] - STIME[J]) */
  mstaTMSAvl,         /*  */
  mstaTMSDed,         /*  */
  mstaTMSUtl,         /*  */
  mstaTDSAvl,         /* total disk seconds available */
  mstaTDSDed,         /* total disk seconds dedicated */
  mstaTDSUtl,         /* total disk seconds utilized */
  mstaIDSUtl,         /* instant disk seconds utilized */
  mstaTPSReq,         /* proc-seconds requested associated w/completed jobs (WallReqTime*PC) */
  mstaTPSExe,         /* proc-seconds executed associated w/completed jobs (WallRunTime*PC) */
  mstaTPSDed,         /* per iteration */
  mstaTPSUtl,         /* per iteration */
  mstaTJobAcc,
  mstaTNJobAcc,
  mstaTXF,
  mstaTNXF,
  mstaMXF,
  mstaTBypass,
  mstaMBypass,        /* maximum bypass count */
  mstaTQOSAchieved,   /* total QOS achieved */
  mstaStartTime,      /* time profile started */
  mstaIStatCount,
  mstaIStatDuration,
  mstaGCEJobs,        /* current eligible jobs */
  mstaGCIJobs,        /* current idle jobs */
  mstaGCAJobs,        /* current active jobs */
  mstaGPHAvl,         /* total processor hours */
  mstaGPHUtl,         /* total processor hours used */
  mstaGPHDed,         /* total processor hours dedicated */
  mstaGPHSuc,         /* proc hours dedicated to jobs which have successfully completed */
  mstaGMinEff,
  mstaGMinEffIteration,
  mstaTPHPreempt,
  mstaTJPreempt,
  mstaTJEval,
  mstaSchedDuration,
  mstaSchedCount,
  mstaTNC,
  mstaTPC,
  mstaTQueuedPH,
  mstaTQueuedJC,
  mstaTSubmitJC,      /* S->SubmitJC */
  mstaTSubmitPH,      /* S->SubmitPH */
  mstaTStartJC,       /* S->StartJC (field 50) */
  mstaTStartPC,       /* S->StartPC */
  mstaTStartQT,       /* S->StartQT */
  mstaTStartXF,       /* S->StartXF */
  mstaDuration,
  mstaIterationCount, /* number of iterations contributing to record */
  mstaProfile,
  mstaQOSCredits,
  mstaTNL,
  mstaTNM,
  mstaXLoad,          /* see GMetric Overview */
  mstaGMetric,        /* see GMetric Overview */
  mstaLAST };

#define MDEF_STATATTRBMSIZE MBMSIZE(mstaLAST)


typedef struct mstatdata_t {
  enum MStatAttrEnum   AType;
  enum MDataFormatEnum DType;
  } mstatdata_t;


#define MMAX_JOBATTRTYPE 4

typedef struct mjobattr_t {
  int                 AIndex;   /* generic attribute index */
  enum MXMLOTypeEnum  OType;    /* object type */
  char                *AName; /* XML attribute name */
  } mobjattr_t;



#define MCONST_EXIT_SERVERISDOWN   4
#define MCONST_EXIT_SERVERISGOOFY  5
#define MCONST_EXIT_SERVERISMICKEY 6

/** hold types, sync w/MHoldType[] */

enum MHoldTypeEnum {
  mhNONE = 0,
  mhUser,
  mhSystem,
  mhBatch,
  mhDefer,
  mhAll,
  mhLAST };


/* sync w/MFormatMode[] */

enum MFormatModeEnum {
  mfmNONE = 0,
  mfmHuman,
  mfmHTTP,
  mfmXML,
  mfmAVP,
  mfmVerbose,
  mfmLAST };


#define MDEF_RMPORT                        0
#define MDEF_RMSERVER                      ""
#define MDEF_RMTYPE                        mrmtPBS
#define MDEF_RMUSTIMEOUT                   30 * MDEF_USPERSECOND /* increased form 12 to 30 to handle pbs api timeouts on big systems */
#define MDEF_RMAUTHTYPE                    mrmapCheckSum


/* ID Manager types (sync w/MIDType[]) */

enum MIDTypeEnum {
  midtNONE = 0,
  midtExec,
  midtFile,
  midtLDAP,
  midtOther,
  midtSQL,
  midtXML,
  midtLAST };


/* sync w/MRsvSearchAlgo[] */

enum MRsvSearchAlgoEnum {
  mrsaNONE = 0,
  mrsaWide,
  mrsaLong,
  mrsaLAST };


/* am object */

/* AM types (sync w/MAMType[]) */

enum MAMTypeEnum {
  mamtNONE = 0,
  mamtFILE,
  mamtGOLD,
  mamtNative,
  mamtMAM,
  mamtLAST };


/**
 * AM consumption policies
 *
 * @see MAMChargeMetricPolicy[] (sync)
 * @see struct mam_t.ChargePolicy
 */

enum MAMConsumptionPolicyEnum {
  mamcpNONE = 0,
  mamcpDebitAllWC,
  mamcpDebitAllCPU,
  mamcpDebitAllPE,
  mamcpDebitAllBlocked,
  mamcpDebitSuccessfulWC,
  mamcpDebitSuccessfulCPU,
  mamcpDebitSuccessfulNode,
  mamcpDebitSuccessfulPE,
  mamcpDebitSuccessfulBlocked,
  mamcpDebitRequestedWC,
  mamcpLAST };


/* sync w/MAMChargeRatePolicy */

enum MAMConsumptionRatePolicyEnum {
  mamcrpNONE = 0,
  mamcrpQOSDel,
  mamcrpQOSReq,
  mamcrpLAST };


/* sync w/MAMJFActionType[] */

enum MAMJFActionEnum {
  mamjfaNONE = 0,
  mamjfaCancel,
  mamjfaDefer,
  mamjfaHold,
  mamjfaIgnore,
  mamjfaRequeue,
  mamjfaRetry,
  mamjfaLAST };



/* node charge policy (sync w/MAMNodeChargePolicy[]) */

enum MAMNodeChargePolicyEnum {
  mamncpNONE = 0,
  mamncpAvg,
  mamncpMax,
  mamncpMin,
  mamncpLAST };


/**
 * Accounting Manager Structure
 *
 * @see enum MAMAttrEnum (sync)
 * @see struct midm_t (peer)
 *
 * @see MAMSetAttr() - set AM attributes
 * @see MAMAToString() - report AM attributes
 * @see MAMProcessConfig() - process AMCFG parameter
 * @see MAMShow() - process 'mdiag -R -v' request
 */

typedef struct mam_t {
  char  Name[MMAX_NAME];
  int   Index;

  enum MRMStateEnum State;

  char  ClientName[MMAX_NAME];

  /* interface specification */

  enum MWireProtocolEnum   WireProtocol;
  enum MSocketProtocolEnum SocketProtocol;

  enum MChecksumAlgoEnum   CSAlgo;

  char  CSKey[MMAX_LINE + 1];

  char  Host[MMAX_LINE];          /* active AM server host/destination               */
  int   Port;                     /* active AM service port                          */

  char  SpecHost[MMAX_LINE];      /* specified AM server host/destination            */
  int   SpecPort;                 /* specified AM service port                       */

  mbool_t UseDirectoryService;

  mbitmap_t Flags;                   /* accounting manager specific flags (bitmap of enum MAMFlagEnum) */
  enum MAMTypeEnum Type;          /* type of AM server                               */

  int   Timeout;                  /* in ms */

  /* policies */

  enum MAMConsumptionPolicyEnum ChargePolicy; /* allocation charge policy            */

  enum MCalPeriodEnum ChargePeriod; /**< unit of charge */
  double              ChargeRate;   /**< quantity of charge per <CHARGEPERIOD>*<PROC>  */

  mbool_t             ChargePolicyIsLocal;  /**< charge policy handled via MLocal plug-in */
  mbool_t             UseDisaChargePolicy;
  mbool_t             ValidateJobSubmission;

  mulong              FlushInterval; /**< AM flush interval */
  mulong              FlushTime;
  enum MCalPeriodEnum FlushPeriod; /* period for allocation flushing */

  enum MAMNodeChargePolicyEnum NodeChargePolicy;  /* how parallel jobs should be charged for allocated resources */

  mbool_t CreateCred;               /**< create cred from AM database */
  mbool_t IsChargeExempt[mxoLAST];  /**< if TRUE, do not bill for corresponding
                                         object */

  char  FallbackAccount[MMAX_NAME]; /* account to use if primary account is unavailable */
  char  FallbackQOS[MMAX_NAME];     /* qos to use if primary account is unabailable */
  mpsi_t P;

  int    FailCount;
  int    FailIteration;

  FILE  *FP;

  mmb_t *MB;

  mnat_t ND;

  /* Optimization queues for NAMI (used if MSched.QueueNAMIActions is TRUE) */

  mxml_t *NAMIChargeQueueE; /* periodic charges (alloc) */
  mxml_t *NAMIDeleteQueueE; /* deletes (alloc) */

  enum MAMJFActionEnum CreateFailureAction;  /* action to take if NAMI create call fails  (retry not supported) */
  enum MAMJFActionEnum CreateFailureNFAction; /* action to take if NAMI reports no funds (retry not supported) */
  enum MAMJFActionEnum StartFailureAction; /* action to take if NAMI start call fails */
  enum MAMJFActionEnum StartFailureNFAction; /* action to take if NAMI reports no funds */
  enum MAMJFActionEnum UpdateFailureAction; /* action to take if NAMI update call fails */
  enum MAMJFActionEnum UpdateFailureNFAction; /* action to take if NAMI update reports no funds */
  enum MAMJFActionEnum ResumeFailureAction; /* action to take if NAMI resume call fails */
  enum MAMJFActionEnum ResumeFailureNFAction; /* action to take if NAMI resume reports no funds */
  } mam_t;


typedef struct mnlbucket {
  int OS;
 
  mnl_t NL;

  mulong    LastFlushTime;
  } mnlbucket_t;

/* identity manager structure (sync w/enum MAMAttrEnum) */
/* NOTE:  midm_t shares mam_t attr list */

typedef struct midm_t {
  char  Name[MMAX_NAME];
  int   Index;
  int   Version;

  enum MChecksumAlgoEnum CSAlgo;

  char  CSKey[MMAX_LINE + 1];

  char  Host[MMAX_NAME];          /* active ID server host                           */
  int   Port;                     /* active ID service port                          */

  char  SpecHost[MMAX_NAME];      /* specified ID server host                        */
  int   SpecPort;                 /* specified ID service port                       */

  mulong MTime;

  enum MCalPeriodEnum RefreshPeriod; /* support hour, day, or infinite */

  mbool_t UpdateRefreshOnFailure;    /* update timestamp of refresh in the case id load fails */

  mulong LastRefresh;

  mbitmap_t CredResetList;              /* bitmap of mxo* */
  mbitmap_t CredBlockList;              /* bitmap of mxo* */

  mbool_t UseDirectoryService;
  mbool_t CreateCred;                /* create creds which have not been discovered locally */

  enum MIDTypeEnum  Type;            /* type of ID server */

  enum MRMStateEnum State;

  char *CredCreateURL;   /* (alloc) */
  char *CredDestroyURL;  /* (alloc) */
  char *CredResetURL;    /* (alloc) */

  enum MBaseProtocolEnum CredCreateProtocol;
  enum MBaseProtocolEnum CredDestroyProtocol;
  enum MBaseProtocolEnum CredResetProtocol;

  char *CredCreatePath;    /* (alloc) */
  char *CredDestroyPath;   /* (alloc) */
  char *CredResetPath;     /* (alloc) */

  int   OCount[mxoLAST + 1]; /* number of objects loaded on last refresh */

  mpsi_t P;

  mmb_t *MB;               /* (alloc) */
  } midm_t;



/**
 * AM Attributes
 *
 * @see MAMAttr[] (sync)
 * @see struct mam_t (sync)
 */

enum MAMAttrEnum {
  mamaNONE = 0,
  mamaBlockCredList,
  mamaChargeObjects,
  mamaChargePolicy,
  mamaChargeRate,
  mamaConfigFile,
  mamaCreateCred,       /* boolean - create internal cred if TRUE */
  mamaCreateCredURL,
  mamaCreateFailureAction,
  mamaCreateURL,
  mamaCSAlgo,
  mamaCSKey,
  mamaDeleteURL,
  mamaDestroyCredURL,
  mamaEndURL,
  mamaFallbackAccount,
  mamaFallbackQOS,
  mamaFlags,
  mamaFlushInterval,
  mamaHost,
  mamaJFAction,
  mamaMessages,
  mamaModifyURL,
  mamaName,
  mamaNodeChargePolicy,
  mamaPauseURL,
  mamaPort,
  mamaQueryURL,
  mamaQuoteURL,
  mamaRefreshPeriod,
  mamaResetCredList,
  mamaResetCredURL,     /* reset cred if URL is specified */
  mamaResumeFailureAction,
  mamaResumeURL,
  mamaServer,
  mamaSocketProtocol,
  mamaStartFailureAction,
  mamaStartURL,
  mamaState,
  mamaTimeout,
  mamaType,
  mamaUpdateFailureAction,
  mamaUpdateRefreshOnFailure,
  mamaUpdateURL,
  mamaVersion,
  mamaValidateJobSubmission,
  mamaWireProtocol,
  mamaLAST };

/* AM Flags (sync w/MAMFlags[]) */

enum MAMFlagEnum {
  mamfNONE = 0,
  mamfLocalCost,
  mamfStrictQuote,
  mamfAccountFailAsFunds,
  mamfLAST };

#define MDEF_AMTYPE                    mamtNONE
#define MDEF_AMVERSION                 0
#define MDEF_AMCHARGEPOLICY            mamcpDebitSuccessfulWC

#define MDEF_AMHOST                    ""
#define MDEF_AMPORT                    40560
#define MMAX_AMFLUSHINTERVAL           86400
#define MDEF_AMFLUSHINTERVAL           3600
#define MDEF_AMAUTHTYPE                mrmapCheckSum
#define MDEF_AMWIREPROTOCOL            mwpAVP
#define MDEF_AMSOCKETPROTOCOL          mspSingleUseTCP

#define MDEF_AMCREATEFAILUREACTION     mamjfaNONE
#define MDEF_AMRESUMEFAILUREACTION     mamjfaNONE
#define MDEF_AMSTARTFAILUREACTION      mamjfaHold
#define MDEF_AMUPDATEFAILUREACTION     mamjfaNONE

#define MDEF_AMAPPENDMACHINENAME       FALSE
#define MDEF_AMTIMEOUT                 15

#define MDEF_AMQBANKPORT               7111
#define MDEF_AMGOLDPORT                7112

#define MDEF_ENABLEINTERNALCHARGING    MBNOTSET

/* no sync */

enum MSockFTypeEnum {
  msftNONE = 0,
  msftDropResponse,
  msftIncomingClient,  /* this socket is from an incoming client and was put on the socket stack */
  msftViewpointProxy,  /* this socket is coming from viewpoint and represents a proxy request */
  msftReadOnlyCommand, /* this socket is a query and doesn't change/modify moab */
  msftLAST };



/* sim object */

/* sync w/MJobSource[] */

enum MJobTraceEnum {
  mjtNONE = 0,
  mjtLocal,
  mjtTrace,
  mjtWiki,
  mjtXML,
  mjtLAST };


/* sync w/MSimQPolicy[] */

enum MSimJobSubmissionPolicyEnum {
  msjsNONE = 0,
  msjsTraceSubmit,
  msjsConstantJob,
  msjsLAST };


typedef struct msim_t {
  char  WorkloadTraceFile[MMAX_LINE + 1]; /* workload trace file */

  /* general config */

  int   InitialQueueDepth;        /* depth of queue at start time             */
  enum MSimJobSubmissionPolicyEnum JobSubmissionPolicy; /* job submission policy */
  char  StatFileName[MMAX_LINE];  /* simulation statistics file name          */

  mulong StartTime;               /* epoch time to start simulation           */
  mulong SpecStartTime;           /* specified StartTime                      */

  char *CorruptJTMsg;             /* corrupt job trace message (alloc) */

  /* config booleans */

  mbool_t AutoShutdown;           /* shutdown when queues are empty           */
  mbool_t AutoStop;               /* pause scheduling when final job is processed */
  mbool_t PurgeBlockedJobs;       /* removed jobs which are permanently blocked */
  mbool_t RandomizeJobSize;       /* randomize job size distribution          */

  int     ProcessJC;              /* total job records processed */
  int     RejectJC;               /* number of jobs in trace rejected */
  int     RejReason[marLAST];

  int     JTIndex;                /* job trace cache index */
  int     JTCount;                /* job trace cache size currently loaded */
  int     TotalJTCount;           /* total job trace lines loaded since startup */
  int     JTLines;                /* total workload file lines processed since startup */

  /* general status */

  mulong  RMFailureTime;
  int     TraceOffset;              /* offset to next trace in tracefile        */

  /* status booleans */

  mbool_t   QueueChanged;
  } msim_t;

/* default sim values */

#define MDEF_SIMWCSCALINGPERCENT        100
#define MDEF_SIMINITIALQUEUEDEPTH       1000
#define MDEF_SIMWCACCURACY              0.0  /* 0 (use trace execution time) */
#define MDEF_SIMWCACCURACYCHANGE        0.0
#define MDEF_SIMSTATFILENAME            "simstat.out"
#define MDEF_SIMJSPOLICY                msjsConstantJob
#define MDEF_SIMNCPOLICY                msncNormal
#define MDEF_SIMNODECOUNT               0                 /* 0 (utilize trace count) */
#define MDEF_SCHEDALLOCLOCALITYPOLICY   malpNONE
#define MDEF_RESOURCEQUERYDEPTH         3   /* number of timeframes to report for flexible resource availability response */

#define MDEF_SIMCOMMUNICATIONTYPE       mcmRoundRobin
#define MDEF_SIMINTRARACKCOST           0.3
#define MDEF_SIMINTERRACKCOST           0.3
#define MDEF_SIMCOMRATE                 0.1

#define MDEF_SIMRANDOMIZEJOBSIZE        FALSE
#define MDEF_SIMAUTOSHUTDOWNMODE        TRUE
#define MDEF_SIMPURGEBLOCKEDJOBS        TRUE

#define MDEF_SIMWORKLOADTRACEFILE       "workload"
#define MDEF_SIMRESOURCETRACEFILE       "resource"

#define MSCHED_KEYFILE                  ".moab.key"
#define MCONST_LICFILE                  "moab.lic"


/* Statistic defines */

#define MMAX_STATTARGET 32          /* max number of stat objects to update */


/* SR object (sync w/MSRsvAttr[]) */

enum MSRAttrEnum {
  msraNONE = 0,
  msraAccess,
  msraAccountList,
  msraActionTime,
  msraChargeAccount,
  msraChargeUser,
  msraClassList,
  msraClusterList,
  msraComment,
  msraDays,
  msraDepth,
  msraDescription,
  msraDisable,
  msraDisableTime,
  msraDisabledTimes,
  msraDuration,
  msraEnableTime,
  msraEndTime,
  msraFlags,
  msraGroupList,
  msraHostList,
  msraIdleTime,
  msraIsDeleted,
  msraJobAttrList,
  msraJPriority,
  msraJTemplateList,
  msraLogLevel,
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
  msraRsvGroup,
  msraRsvAccessList,
  msraRsvList,
  msraStartTime,
  msraStIdleTime,
  msraStTotalTime,
  msraSubType,
  msraSysJobTemplate,
  msraTaskCount,
  msraTaskLimit,
  msraTimeLimit,
  msraTPN,
  msraTrigger,
  msraType,
  msraUserList,
  msraVariables,
  msraWEndTime,
  msraWStartTime,
  msraLAST };


/* sync w/MNCMArgs[] */

enum MNodeCmdAttrEnum {
  mncmaNONE,
  mncmaGEvent,
  mncmaOS,
  mncmaPartition,
  mncmaPower,
  mncmaState,
  mncmaVariables,
  mncmaLAST };


/* sync w/MXResourceAttr[] */

enum MXResourceEnum {
  mxrNONE = 0,
  mxrLoginCount,
  mxrKbdDelay,
  mxrLast };


/* sync w/MDiagnoseCmds[] */

enum MDiagnoseCmdEnum {
  mdcmNONE = 0,
  mdcmAccount,
  mdcmBlockedJobs,
  mdcmClass,
  mdcmConfig,
  mdcmEvent,
  mdcmFairshare,
  mdcmGreen,
  mdcmGroup,
  mdcmJob,
  mdcmLimit,
  mdcmNode,
  mdcmPriority,
  mdcmPartition,
  mdcmQOS,
  mdcmRM,
  mdcmRsv,
  mdcmSRsv,
  mdcmSched,
  mdcmTrig,
  mdcmUser,
  mdcmLast };


/* sync w/MNodeCtlCmds[] */

enum MNodeCtlCmdEnum {
  mncmNONE = 0,
  mncmCheck,
  mncmCreate,
  mncmDestroy,
  mncmList,
  mncmMigrate,
  mncmModify,
  mncmQuery,
  mncmReinitialize,
  mncmLAST };

/* sync w/MVCCtlCmds[] */

enum MVCCtlCmdEnum {
  mvccmNONE = 0,
  mvccmAdd,       /* Add an object to a VC */
  mvccmCreate,    /* Create a VC */
  mvccmDestroy,   /* Destroy a VC */
  mvccmModify,    /* Modify a VC */
  mvccmQuery,     /* Query VCs */
  mvccmRemove,    /* Remove an object from a VC */
  mvccmExecute,   /* Execute an action on the VC (-x, not set by client, used for readability) */
  mvccmLAST };

/* sync w/MVMCtlCmds[] */

enum MVMCtlCmdEnum {
  mvmcmNONE = 0,
/*  mvmcmCreate, */
  mvmcmDestroy,
  mvmcmMigrate,
  mvmcmModify,
  mvmcmQuery,
  mvmcmForceMigration,
  mvmcmLAST };


/* sync w/MVMMigrateMode[] */

enum MVMMigrateModeEnum {
  mvmmmNONE = 0,
  mvmmmOvercommit,
  mvmmmGreen,
  mvmmmRsv,
  mvmmmLAST };


/* sync w/MNodeQueryCmd[] */

enum MNodeQueryCmdEnum {
  mnqcNONE = 0,
  mnqcCat,
  mnqcDiag,
  mnqcProfile,
  mnqcWiki,
  mnqcLAST };


/* sync w/MNodeActions[] */

enum MNodeActionEnum {
  mnactNONE = 0,
  mnactReset,
  mnactShutdown,
  mnactPowerOn,
  mnactProvision,
  mnactScrub,
  mnactLAST };


/* xt3 partition attributes (sync w/MXT3ParAttr[]) */

enum MXT3ParAttrEnum {
  mxt3paNONE = 0,
  mxt3paCreateTime,
  mxt3paJobID,
  mxt3paOrphaned,
  mxt3paProcs,
  mxt3paLAST };




/* sync w/MPostDSFile[] */
/*
 *  Possible values for which file to use post data-staging actions after
 *   In other words, after the specified file has been staged, the post
 *   data staging action will take place.
 */
enum MPostDSFileEnum {
  mpdsfNONE,
  mpdsfStdOut,    /** After stdout */
  mpdsfStdErr,    /** After stderr */
  mpdsfStdAll,    /** After both stdout and stderr (called twice) */
  mpdsfExplicit,  /** After any explicit data stage that the user specified */
  mpdsfLAST };

/* sync w/MJobLLAttr[] */

enum MJobLLAttrEnum {
  mjllaNONE,
  mjllaArgs,
  mjllaBlocking,
  mjllaClass,
  mjllaCmd,
  mjllaEnvironment,
  mjllaHostList,
  mjllaIWD,
  mjllaJobName,
  mjllaJobType,
  mjllaQueue,
  mjllaReqAWDuration,
  mjllaReqNodes,
  mjllaRestart,
  mjllaStartDate,
  mjllaStdErr,
  mjllaStdOut,
  mjllaTaskGeometry,
  mjllaTotalTasks,
  mjllaLAST };


/* generic sub command types, used in MUICheckAuthorization() */

enum MSubCmdEnum {
  mgscNONE = 0,
  mgscCreate,
  mgscDestroy,
  mgscList,
  mgscModify,
  mgscQuery };


/* sync w/MJobCtlCmds[] */

enum MJobCtlCmdEnum {
  mjcmNONE = 0,
  mjcmCancel,
  mjcmCheckpoint,
  mjcmComplete,
  mjcmCreate,    /**< for templates */
  mjcmDelete,    /**< for templates */
  mjcmDiagnose,
  mjcmForceCancel,
  mjcmJoin,
  mjcmMigrate,
  mjcmModify,    /* general attr, hold, prio modify */
  mjcmQuery,     /* view job details, environment, health, stats, etc */
  mjcmRequeue,
  mjcmRerun,     /* rerun a completed job */
  mjcmResume,
  mjcmShow,
  mjcmSignal,
  mjcmStart,
  mjcmSubmit,
  mjcmSuspend,
  mjcmTerminate,
  mjcmAdjustHold,
  mjcmAdjustPrio,
  mjcmCheck,  /**< for checkjob */
  mjcmLAST };

/* sync w/MRsvShowMode[] */

enum MRsvShowModeEnum {
  mrsmNONE = 0,
  mrsmOverlap,
  mrsmFree,
  mrsmLAST };

/* sync w/MRsvCtlCmds[] */

enum MRsvCtlCmdEnum {
  mrcmNONE = 0,
  mrcmAlloc,
  mrcmCreate,
  mrcmDestroy,
  mrcmFlush,
  mrcmJoin,
  mrcmList,
  mrcmMigrate,
  mrcmModify,
  mrcmQuery,
  mrcmOther,
  mrcmSignal,
  mrcmSubmit,
  mrcmShow,
  mrcmLAST };


/* sync w/MBalCmds[] */

enum MBalEnum {
  mbcmNONE = 0,
  mbcmExecute,
  mbcmQuery,
  mbcmLAST };


/* sync w/MCredCtlCmds[] */

enum MCredCtlCmdEnum {
  mccmNONE = 0,
  mccmCreate,
  mccmDestroy,
  mccmList,
  mccmModify,
  mccmQuery,
  mccmReset,
  mccmLAST };


/* sync w/MShowCmds[] */

enum MShowCmdEnum {
  mscmdNONE = 0,
  mscmdList,
  mscmdQuery,
  mscmdLAST };


/* msdata object (sync w/MSDataAttr) */

enum MSDataAttrEnum {
  msdaNONE = 0,
  msdaDstFileName,
  msdaDstFileSize,
  msdaDstHostName,
  msdaDstLocation,
  msdaESDuration,
  msdaIsProcessed,
  msdaSrcFileName,
  msdaSrcFileSize,
  msdaSrcHostName,
  msdaSrcLocation,
  msdaState,
  msdaTransferRate,
  msdaTStartDate,
  msdaType,
  msdaUpdateTime,
  msdaLAST };

/* sys object (sync w/MSysAttr[]) */

enum MSysAttrEnum {
  msysaNONE = 0,
  msysaActiveJobCount,
  msysaActivePS,
  msysaAPServerHost,
  msysaAPServerTime,
  msysaAvgTActivePH,
  msysaAvgTQueuedPH,
  msysaCPInitTime,
  msysaEligibleJobCount,
  msysaFSInitTime,
  msysaIdleJobCount,
  msysaIdleMem,
  msysaIdleNodeCount,
  msysaIdleProcCount,
  msysaIterationCount,
  msysaMinEfficiency,
  msysaMinEfficiencyIteration,
  msysaPreemptJobCount,
  msysaPresentTime,
  msysaQueuePS,
  msysaRMPollInterval,
  msysaSCompleteJobCount,
  msysaStatInitTime,           /* date stats were initialized */
  msysaUpMem,
  msysaUpNodes,
  msysaUpProcs,
  msysaVersion,
  msysaLAST };


/* priority mode */

enum MJobPrioEnum {
  mjpNONE = 0,
  mjpRelative,
  mjpAbsolute,
  mjpLAST };


/* checkpoint modes (sync w/MCKMode[]) */

enum MCKPtModeEnum {
  mckptNONE = 0,
  mckptCJob,
  mckptGeneral,
  mckptRsv,
  mckptSys,
  mckptSched,
  mckptLAST };


/* cluster object (sync w/MClusterAttr[]) */

enum MClusterAttrEnum {
  mcluaNONE = 0,
  mcluaMaxSlot,
  mcluaName,
  mcluaPresentTime,
  mcluaLAST };

/* rack object */

enum MRackAttrEnum {
  mrkaNONE = 0,
  mrkaIndex,
  mrkaName,
  mrkaLAST };


/* log level (sync w/MLogLevel[]) */

enum MLogLevelEnum {
  lDEBUG = 0,
  lINFO,
  lWARN,
  lERROR,
  lFATAL };


/* log facilities (sync w/MLogFacilityType[]) */

enum MLogFacilityEnum {
  fCORE = 0,
  fSCHED,
  fSOCK,
  fUI,
  fLL,
  fSDR,
  fCONFIG,
  fSTAT,
  fSIM,
  fSTRUCT,
  fFS,
  fCKPT,
  fAM,
  fRM,
  fPBS,
  fWIKI,
  fTRANS,
  fS3,
  fNAT,
  fGRID,
  fCOMM,
  fMIGRATE,
  fALL };

#define MLOG_FBMSIZE MBMSIZE(fALL)


/* communication method types */

enum MComMethodEnum {
  mcmRoundRobin = 0,
  mcmBroadcast,
  mcmTree,
  mcmMasterSlave,
  mcmLAST2 };


#define MDEF_VPCPURGETIME 86400  /* keep completed VPC's for 1 day after completion */


/* sync w/MVPCMigrationPolicy[] */

enum MVPCMigrationPolicyEnum {
  mvpcmpNONE = 0,
  mvpcmpNever,          /* never change node allocation for vpc */
  mvpcmpAvailable,      /* only change node allocation for vpc when nodes go down */
  mvpcmpOptimal,        /* freely change node allocation for vpc for optimal utitlization */
  mvpcmpLAST };


/**
 * Configurable policy of what to do to a job when its dependency fails.
 *
 * sync w/ MDependFailPolicy[]
 */

enum MDependFailPolicyEnum {
  mdfpNONE = 0,
  mdfpHold,
  mdfpCancel,
  mdfpLAST };


/**
 * Job Migration Policy
 *
 * @see MJobMigratePolicy[] (sync)
 *
 * @see MJobMigratePolicy[]
 * @see MSched.JobMigratePolicy
 */

/* NOTE:  should add 'MigrateOnDataStage' policy for meta only applications 
 *        (NYI) */

enum MJobMigratePolicyEnum {
  mjmpNONE = 0,
  mjmpAuto,        /**< migrate just in time or immediate based on number of 
                        potential destinations */
  mjmpImmediate,   /**< migrate job as soon as it is submitted */
  mjmpJustInTime,  /**< migrate only when job is able to start on remote 
                        resources */
  mjmpLoadBalance, /**< migrate prior to scheduling after job templates
                        have been applied using basic load balancing */
  mjmpLAST };



enum MLogStateEnum {
  mlsClosed = 0,
  mlsOpen,
  mlsLAST };

/* NOTE: sync w/suem_t */

#define MMAX_MEMTABLESIZE 2000000
#define MMAX_MEMTABLEBUF  100000

/* Control structure to tracker memory allocations if enabled */
typedef struct mmemtab_t  {
  void *Ptr;
  int   Size;
  } mmemtab_t;

typedef struct memtracker_t {
  int TotalMemCount;
  int TotalMemAlloc;

  mmemtab_t *MemTable;  /* (alloc) */
  } memtracker_t;


/* MLog control structure for writing to log files
 * and a place to store the memory tracker control structure
 */
typedef struct mlog_t {
  enum MLogStateEnum State;      /* External */

  int            Threshold;      /* External */
  mbitmap_t      FacilityBM;

  FILE          *logfp;

  unsigned long  LogFileMaxSize; /* External */
  int            LogFileRollDepth;

  char          *LogDir;         /* External (alloc) */

  char           Buffer[MMAX_BUFFER];

  /* memory tracker control structure used for memory usage analysis */
  memtracker_t  *MemoryTracker;

  mlog_t()
    {
    State = mlsClosed;
    Threshold = 0;
    logfp = NULL;
    LogFileMaxSize = 0;
    LogFileRollDepth = 0;
    LogDir = NULL;
    }
  } mlog_t;


#define MMAX_CFG  1024

typedef struct mcfg_t {
  const char          *Name;
  enum MCfgParamEnum   PIndex;
  enum MDataFormatEnum Format;
  enum MXMLOTypeEnum   OType;
  void                *OPtr;
  mbool_t              IsDeprecated;
  enum MCfgParamEnum   NewPIndex;     /* replacement parameter (optional) */
  char               **AttrList;      /* valid parameter values (optional) */
  } mcfg_t;


/* RM objects */

#define MMIN_LLCFGDISK       1   /* (in MB) */



/* client object (@see enum MClientAttrEnum) */

#define MMAX_ADMINUSERS      64
#define MMAX_SUBMITHOSTS     64

typedef struct mccfg_t {
  char  ServerHost[MMAX_NAME];
  int   ServerPort;

  char  Label[MMAX_NAME];

  long  Timeout;       /* client connection timeout (in us) */

  enum MSvcEnum        CIndex;  /**< requested service */

  enum MFormatModeEnum Format;

  enum MSocketProtocolEnum SocketProtocol;

  char  ServerCSKey[MMAX_LINE + 1];

  mbool_t DoEncryption;
  mbool_t IsNonBlocking;

  enum MChecksumAlgoEnum ServerCSAlgo;

  char  ConfigFile[MMAX_LINE];
  char  CfgDir[MMAX_LINE];
  char  VarDir[MMAX_LINE];

  char  SchedulerMode[MMAX_NAME];

  char  BuildDir[MMAX_LINE];
  char  BuildHost[MMAX_LINE];
  char  BuildDate[MMAX_LINE];
  char  BuildArgs[MMAX_LINE];

  char  Msg[MMAX_LINE];  /* client event message */

  mbitmap_t  DisplayFlags;  /* BM of enum MDisplayFlagEnum */
  
  enum MJobStateTypeEnum QType;

  enum MRMTypeEnum EmulationMode;  /* client emulation mode */
  
  int ActiveSortOrder;

  mbool_t GridCentric;
  mbool_t ShowJobName;
  mbool_t EnableEncryption;
  mbool_t IsInteractive;
  mbool_t IsInitialized;
  mbool_t NoProcessCmdFile;
  mbool_t AllowUnknownResource;
  mbool_t MapFeatureToPartition;
  mbool_t RejectDosScripts;

  mbool_t MBalForwardX;
  mbool_t MBalPseudoTTY;
  mbool_t MBalQuiet;

  mbool_t PerPartitionDisplay; /* showq displays jobs on a per partition basis */

  char *CmdFlags;  /* request flags to be passed to server (alloc) */
  char *PCmdFlags; /* private client flags (alloc) */
  char *PeerFlags; /* opaque flags to be passed to peer, ie SLURM (alloc) */
  char *Forward;   /* forwarding destination (alloc) */
  char *ORange;    /* object range (alloc) */

  char *SpecUser;  /* (alloc) */
  char *SpecGroup; /* (alloc) */
  char *SpecJobID; /* (alloc) */

  mbitmap_t SubmitConstraints;   /* (bitmap of enum MJobAttrEnum) */

  char *DefaultSubmitPartition;  /* (alloc) - NOTE: only applied if host, feature, and partition not specified by user */

  char *LocalDataStageHead;  /* (alloc) - specifies the node where stdout/stderr files are copied back to */
  mxml_t *WE;

  mpsi_t *Peer;    /* associated peer (ptr,if any) */

  /* command specific attributes */

  char  PName[MMAX_NAME];  /* partition name */

  char  RID[MMAX_NAME];    /* specified requestor id */
  char  Proxy[MMAX_NAME];

  char     **EnvP;             /* pointer to environment variables */
  marray_t   SpecEnv;          /* Used to pass specified variables to exec for slurm interactive jobs */
  mbool_t    FullEnvRequested;

  mbool_t KeepAlive;       /* keep client alive */

  /* client functions */

  int (*ShowUsage)(enum MSvcEnum,const char *);

  /* cached data */

  mulong JobCompletionTime;
  mulong UTime;              /* time data was last updated */

  /* SMP/SMT job placement */

  int PPN;
  int Sockets;
  int Numa;
  } mccfg_t;


/**
 * Scheduler phases 
 *
 * @see MSchedPhase[] (sync)
 * @see MSched.Activity
 * @see MSched.{LoadStartTime[],LoadEndTime}
 *
 * @see enum MActivityEnum
 */

enum MSchedPhaseEnum {
  mschpNONE = 0,
  mschpIdle,
#ifdef MIDLE
  mschpMeasuredIdle,  /* count how long we are ACTUALLY sleeping in the UI phase */
#endif /* MIDLE */
  mschpRMAction,
  mschpRMLoad,
  mschpRMProcess,
  mschpSched,
  mschpTotal,
  mschpTriggers,
  mschpUI,
  mschpLAST };


typedef struct madmin_t {
  char Name[MMAX_NAME];

  int  Index;

  mbool_t AllowCancelJobs;          /* allows this admin level to cancel jobs (temporary ) */

  mbool_t EnableProxy;

  char UName[MMAX_ADMINUSERS + 1][MMAX_NAME]; /* admin usernames */
  } madmin_t;

/**
 * @struct mvaddress_t
 *
 * Holds a single VM hostname and IP address.
 *
 */
typedef struct mvaddress_t {
  char HName[MMAX_NAME];      /**< virtual hostname */
  char Address[MMAX_NAME];    /**< IP or MAC address */

  mbool_t IsAvail;            /**< host/address name pair are available for consumption */
  } mvaddress_t;

#define MMAX_APSERVER           16  /* access portal servers */
#define MMAX_HOSTALIAS          8
#define MCONST_FBSYNCFREQUENCY  8
#define MCONST_BFJOBNAME        "BFWindow"

#define MDEF_SMATRIXPERIOD      mpDay;

/* sync w/MOExpSearchOrder[] */

enum MOExpSearchOrderEnum {
  moesoNONE = 0,
  moesoExact,
  moesoRange,
  moesoRE,
  moesoRangeRE,
  moesoRERange,
  moesoLAST };

#define MDEF_OESEARCHORDER      moesoRE

/* sync w/MActionPolicy[] */

enum MActionPolicyEnum {
  mapNONE = 0,
  mapBestEffort,
  mapForce,
  mapNever,
  mapLAST };



enum MErrorMsgEnum {
  memNONE = 0,
  memInvalidFSTree,
  memLAST };


/* sync w/MRecoveryAction[] */

enum MRecoveryActionEnum {
  mrecaNONE = 0,
  mrecaDie,
  mrecaIgnore,
  mrecaRestart,
  mrecaTrap,
  mrecaLAST };


/**
 * Scheduler activity.
 *
 * @see enum MSchedPhaseEnum
 *
 * sync w/MActivity[]
 */

enum MActivityEnum {
  macNONE = 0,
  macJobStart,
  macJobSuspend,
  macJobResume,
  macJobCancel,
  macJobMigrate,
  macRMResourceLoad,
  macRMResourceProcess,
  macRMResourceModify,
  macRMWorkloadLoad,
  macRMWorkloadProcess,
  macSchedule,
  macTrigger,
  macUIProcess,  /* UI phase--processing user input */
  macUIWait,     /* UI phase--in sleep or initiating GetData() from RMs */
  macLAST };

#define MDEF_OSCREDLOOKUP       mapBestEffort
#define MMAX_IGNJOBS            256

#define MDEF_CLIENTRETRYCOUNT   0
#define MDEF_CLIENTRETRYTIME    2000000 /* 2 seconds (in microseconds) */

/* for tracking client/server and peer-to-peer requests (see
 * TransactionIDCounter in msched_t and TransactionID in msocket_t) */

#define MMIN_TRANSACTIONID      1000000000

#define MCONST_INTERNALRMNAME   "internal"

/* NOTE:  SpoolDir permissions should be '1777'. If anyone wants something
          else they will have to change it manually. */

#define MCONST_SPOOLDIRPERM     (S_ISVTX|S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)
#define MCONST_STRICTSPOOLDIRPERM     (S_ISVTX|S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)


#define MCONST_ARCHIVEDIRPERM   0755

typedef struct mdb_t {

  union {
    struct modbc_t *ODBC;
    struct msqlite3_t *SQLite3;
    } DBUnion;

  MDBTypeEnum DBType;

  mbool_t WriteNodesInitialized;
  mbool_t WriteJobsInitialized;
  mbool_t WriteRsvInitialized;
  mbool_t WriteTriggerInitialized;
  mbool_t WriteVCInitialized;
  mbool_t WriteRequestsInitialized;
  mbool_t InsertCPInitialized;
} mdb_t;

/*Moab logged events struct */

typedef struct mevent_t {
  int ID;
  char Name[MMAX_NAME];
  enum MXMLOTypeEnum         OType;
  enum MRecordEventTypeEnum  EType;
  int unsigned Time;
  char                      Description[MMAX_LINE];
} mevent_t;


/* job trace iterator */

#define MITR_DONE (SUCCESS << 1)

enum MEventItrTypeEnum {
  meiNONE,
  meiFile,
  meiDB
};

typedef struct mevent_itr_t {
  enum MEventItrTypeEnum Type;
  union {

    struct {
      struct mstmt_t *MStmt;
    } DBItr;

    struct {
      char                      *FileBuf;
      mulong                     StartTime;
      mulong                     EndTime;
      int                       *EID;
      enum MRecordEventTypeEnum *EType;
      enum MXMLOTypeEnum        *OType;
      char                     (*OID)[MMAX_LINE];

      int EIDSize;
      int ETypeSize;
      int OTypeSize;
      int OIDSize;

      mulong                     CurTime;
      char                      *CurFilePos;
      char                      *CurFileTokContext;
    } FileItr;

  } TypeUnion;
} mevent_itr_t;

/**
 * Wiki attributes for gevents (when on their own line, not part of a node
 *
 * Sync w/ MGEWikiAttr, mgevent_obj_t
 *
 * Note that these attributes will have everything that can be set in an
 *  mgevent_obj_t, but will also include other options (such as ParentNode)
 */

enum MGEWikiAttrEnum {
  mgewaNONE = 0,
  mgewaMessage,    /* GEvent message */
  mgewaMTime,      /* Modified time (time event was fired) */
  mgewaParentNode, /* Node to attach this gevent to */
  mgewaParentVM,   /* VM to attach this gevent to */
  mgewaSeverity,   /* Severity for the given instance */
  mgewaStatusCode, /* Status code */
  mgewaLAST };







/* sync w/ MVCFlags[] */

enum MVCFlagEnum {
  mvcfNONE = 0,
  mvcfDestroyObjects,   /* Destroy rsvs, jobs, and VMs in VC when the VC is destroyed */
  mvcfDestroyWhenEmpty, /* Destroy VC when it contains no objects */
  mvcfDeleting,         /* VC has started removal process (may be waiting on workflows, etc. to finish) */
  mvcfHasStarted,       /* VC has jobs that have started (workflows only) */
  mvcfHoldJobs,         /* VC will place a hold on jobs that are submitted to it while this flag is set */
  mvcfWorkflow,         /* VC for a workflow (1 workflow VC per workflow max) */
  mvcfLAST };

#define MDEF_VCFLAGSIZE MBMSIZE(mvcfLAST)


/* VC internal flags - these are not checkpointed! */

enum MVCIFlagEnum{
  mvcifNONE = 0,
  mvcifHarvest,         /* VC is empty and should be removed, only set by DestroyWhenEmpty evaluation */
  mvcifRemoved,         /* Used to not get a double-free when a VC is empty in MVCRemove */
  mvcifDontTransition,  /* Don't transition the VC (unless deleting the transition) */
  mvcifDeleteTransition,/* When transitioning, make the transition a destroy request */
  mvcifLAST };

#define MDEF_VCIFLAGSIZE MBMSIZE(mvcifLAST)

/* sync w/ MVCExecute[]
 *
 * Actions that VCs can execute
 */

enum MVCExecuteEnum {
  mvceNONE = 0,
  mvceClearMB,         /* clear VC message buffer */
  mvceScheduleVCJobs,  /* VC should schedule all jobs together (all have reservations at same time) */
  mvceLAST };

/* sync w/ MVCAttr[] */

enum MVCAttrEnum {
  mvcaNONE = 0,
  mvcaACL,         /* creds that have full access to VC (can't change owner) */
  mvcaCreateJob,   /* job that created this VC */
  mvcaCreateTime,  /* creation time of the VC */
  mvcaCreator,     /* user who created the VC */
  mvcaDescription, /* description */
  mvcaFlags,       /* flags (from MVCFlagEnum) */
  mvcaJobs,        /* names of jobs in VC */
  mvcaMessages,    /* VC messages */
  mvcaName,        /* VC name */
  mvcaNodes,       /* names of nodes in VC */
  mvcaOwner,       /* owner of the VC */
  mvcaParentVCs,   /* names of parent VCs */
  mvcaReqNodeSet,  /* feature to be used as node set for jobs in this VC */
  mvcaReqStartTime,/* requested start time for guaranteed start times */
  mvcaRsvs,        /* names of rsvs in VC */
  mvcaThrottlePolicies, /* throttling policies (like MAXJOB) */
  mvcaVariables,   /* VC vars */
  mvcaVCs,         /* sub-VCs */
  mvcaVMs,         /* names of VMs in VC */
  mvcaLAST };


/* Virtual Container (VC)
 *
 * Contains policies and shared objects for a group of jobs/nodes/rsvs/vms/sub-vcs.
 */

typedef struct mvc_t {
  char      Name[MMAX_NAME];   /* VC name - must be unique */

  char     *Description;       /* Description of VC (alloc) (required - set to same value as Name if not specified) */

  /* Owner - Unless changed, the owner is the user who created the VC.
      If the VC is created internally, only administrators can access it. */

  enum MXMLOTypeEnum OwnerType;/* The type of the owner object */
  char     *Owner;             /* The name of the owner (alloc) */ 

  macl_t   *ACL;               /* creds that have full access to VC (except change owner) */

  /* Creation information */
  mgcred_t *Creator;               /* The user who created this VC */
  mulong CreateTime;               /* Time that this VC was created */

  char *CreateJob;                /* Name of the job that generated this VC, if applicable (alloc) */

  /* Resources/Jobs */

  mln_t *Jobs;      /* Jobs under this VC (alloc) */
  mln_t *Nodes;     /* Nodes (alloc) */
  mln_t *VMs;       /* VMs (alloc) */
  mln_t *Rsvs;      /* Reservations (alloc) */
  mln_t *VCs;       /* Sub-VCs (alloc) */

  /* Parent VCs */

  mln_t *ParentVCs; /* Parent VCs (alloc) - create only in MVCAddObject() */

  /* Variables/Shared */

  mhash_t  Variables;  /* Variables, viewable by any jobs in this VC (alloc) */

  /* Flags */

  mbitmap_t Flags;  /* misc VC flags (bitmap of enum MVCFlagEnum) */
  mbitmap_t IFlags;/* internal VC flags (enum VMCIFlagEnum - not checkpointed!) */

  char *CheckpointDesc; /* checkpoint file description of this VC (alloc, free after use) */

  /* Trigger control list for VC */

  marray_t        *T;        /* VC triggers (alloc) */

  mmb_t *MB;                 /* general messages (includes error messages from triggers) */

  mdynamic_attr_t *DynAttrs; /* Dynamic attributes */
#if 0
  /* These are attributes in DynAttrs, "declared" here so we know what types they should be.
      sync w/ MVCSetDynamicAttr(), MVCGetDynamicAttr() */

  char *ReqNodeSet;    /* The feature to be used as a nodeset for jobs in this VC (or sub-vcs) */
  mulong RequestedStartTime; /* Requested time for guaranteed start time for jobs in this VC */
#endif
} mvc_t;



/**
 * Internal Charging
 *
 * @see mrsv_t.LienCost (checkpointed)
 * @see mgcred_t.LienCredits (not checkpointed)
 * @see mgcred_t.BaseCredits (not checkpointed)
 * @see mgcred_t.UsedCredits (checkpointed)
 * @see msched_t.EnableInternalCharging
 *
 * @see MAMAllocResCancel() - clear existing lien on reservation
 * @see MAMAllocRReserve() - create lien on reservation, check available account
 *                           credits
 * @see MAMAllocRDebit() - create allocation charge AND adjust reservation lien
 * @see MRsvLoadCP() - load mrsv_t.LienCredits and update mgcred_t.LienCredits
 * @see MAcctShow() - report account credit status
 * @see MUICredCtl() - enable admin-initiated credit transfer
 * @see MAMAcctUpdateBalance() - sync credit balance w/AM
 *
 * NOTE:  credential credits specified via parameters CREDITS and USEDCREDITS
 * NOTE:  lien handling only enabled with GOLD AM
 */




/**
 * Global scheduler object
 *
 * @struct msched_t
 *
 * @see enum MSchedAttrEnum (sync)
 *
 * @see MSchedProcessConfig() - process configurable scheduler attributes (see SCHEDCFG)
 * @see MSchedToString() - convert scheduler attributes to string
 *        NOTE:  contains list of attributes to checkpoint
 * @see MSchedAToString() - must have entries for all attributes which can be viewed/checkpointed
 * @see MSchedSetAttr() - must have entries for all attributes which can be set/checkpointed
 * @see MSchedSetDefaults() - set default values for all scheduler attributes
 */

typedef struct msched_t {
  char  Name[MMAX_NAME];
  long  StartTime;                  /* time current scheduler instance started */

  mulong Time;                      /* current epoch time (updated only once
                                       at the start of each iteration) */
  mulong IntervalStart;             /* epoch time when current iteration started */

  mbitmap_t Flags; /* bitmap of MSchedFlagEnum */

  mbitmap_t FeatureGRes; /* set scheduler featuregres          */

  char  MFullRevision[MMAX_LINE];   /* Full revision with name, version, changeset */
  char  Version[MMAX_NAME];         /* scheduler version */
  char  Revision[MMAX_NAME];        /* scheduler revision */

  char  Changeset[MMAX_NAME];       /* scheduler changeset id (corresponds to
                                       repo version) */

  struct timeval SchedTime;         /* accurate system time at start
                                       of scheduling */
  long  GreenwichOffset;            /* local offset associated with timezone */
  int   Interval;                   /* time transpired since last poll
                                       interval (hundredths of a second) */

  int   Day;                        /* current local day of the year */
  int   DayTime;                    /* current local hour of the day */
  int   WeekDay;                    /* day of week (0 - 6) */
  int   Month;                      /* month of year (0 - 11) */
  int   MonthDay;                   /* day of month (0 - 30) NOT consistent
                                       w/localtime() */

  long  ClientMaxPrimaryRetry;        /* number of attempts to connect to
                                         primary server on failure */
  int   ClientMaxPrimaryRetryTimeout; /* time in microseconds in between
                                         primary retries */
  int   ClientMaxConnections;         /* Maximum number of client connections.*/

  int   QOSPreempteeCount[MMAX_QOS];  /* potential preemptees w/in QoS for
                                         current iteration */

  int   SerialJobPreemptSearchDepth;  /* number of potential preemptees to
                                         find for a serial job preemptor */

  int   ClientUIPort;   /* specified port for the client UI moab to use, default is ServerPort+1 */
  int   ForkIterations; /* number of iterations between creating a new child process to interact with client commands.  ForkIterations <= 0 means do not ever fork  */

  enum MCalPeriodEnum SMatrixPeriod; /* stats matrix roll period */

  long  MaxRMPollInterval;          /* max interval between scheduling
                                       attempts (seconds) */
  long  MinRMPollInterval;          /* min interval between scheduling
                                       attempts (seconds) */
  long  NextUIPhaseDuration;        /* max user interface duration for
                                       current iteration (seconds) */

  long  MaxTrigCheckSingleTime;     /* max amount of time (ms) that can be
                                       spent on checking triggers before UI
                                       events are allowed to be processed */
  long  MaxTrigCheckTotalTime;      /* max amount of time (ms) that can be
                                       spent on checking triggers before UI
                                       phase is allowed to end */

  long  MaxTrigCheckInterval;       /* max amount of time spent between 
                                       checking triggers in MUIAcceptRequests */

  int   TrigEvalLimit;              /* Moab must check ALL triggers exactly
                                       this many times per iteration--Moab
                                       will stay in the UI phase until ALL
                                       triggers have been evaluated this
                                       many times--no more and no less */
  long  LastTrigIndex;              /* the index (in global MTList) of the
                                       last trigger evaluated */

  long  RMJobAggregationTime;       /* right now the same
                                       as RMEventAggregationTime */
  long  RMEventAggregationTime;     /**< see mcoRMEvent??? */
  long  AdminEventAggregationTime;  /**< see mcoAdminEventAggregationTime */
  long  FirstEventReceivedTime;     /* marks when the first event was
                                       processed during an event aggregation
                                       time span */

  mulong SpoolDirKeepTime;          /* the amount of time in seconds to keep
                                       files in the spool dir */
  mulong GEventTimeout;             /* Default timeout for gevents. */
  mulong JobSubmitEventReceivedTime;
  mulong JobTermEventReceivedTime;
  mulong NextSysEventTime;          /**< time of next system event which is
                                      not represented by trigger, rsv, etc */

  int   ActiveLocalRMCount;         /**< number of local active compute RM's */
  enum MSchedModeEnum ForceMode;    /* forced scheduler operation mode */
  enum MSchedModeEnum Mode;         /* mode of scheduler operation */
  enum MSchedModeEnum SpecMode;     /* configured scheduler operation mode */

  enum MRMSubTypeEnum EmulationMode;  /* cluster RM emulation mode */

  int   MaxJobCounter;              /* maximum job id allowed for internal queue */
  int   MinJobCounter;              /* minimum job id allowed for internal queue */
  int   StepCount;                  /* number of scheduling steps before shutdown */

  int   MaxRecordedCJobID;          /* maximum completed job id allowed in
                                       the completed job table*/

  mulong TransactionIDCounter;      /* transaction id counter */

  int   TransactionCount;           /* the number of socket transactions in
                                       the socket queue */
  int   HistMaxTransactionCount;    /* the largest number of waiting
                                       transactions ever seen since Moab
                                       last started */
  int   HistMaxClientCount;         /* the largest number of waiting
                                       client connections ever seen since Moab
                                       last started */

  /* NOTE:  must add 'per include file' buffers to allow persistent updates */

  int   ConfigSerialNumber;         /* time configuration was last modified */
  char  ConfigFile[MMAX_LINE];      /* sched configuration file name        */

  char  WSFailOverFile[MMAX_LINE];  /* fail over file for web services */

  char *ConfigBuffer;               /* in memory configfile (alloc)         */
  char *PvtConfigBuffer;            /* (alloc) */
  char *PvtConfigFile;              /* (alloc) */

  mbool_t StrictConfigCheck;        /* server should not start if any failures
                                       are found in the config file (config) */

  long  ClockSkew;                  /* max allowed client clockskew */
  char *AuthCmd;                    /* client auth command (alloc) */

  char *DatBuffer;                  /* ??? */
  char *DatStart;                   /* ??? */

  char *NodeIDFormat;               /* (alloc) */
  enum MOExpSearchOrderEnum OESearchOrder;  /* object expression search order */
  enum MActionPolicyEnum OSCredLookup;  /* policy to use for looking
                                           OS-specified up user credentials */
  char  CfgDir[MMAX_LINE];          /* scheduler config directory */
  char  VarDir[MMAX_LINE];          /* Set by ENV Variable MOABHOMEDIR,       */
                                    /* directory for logging, statistics, etc */
  char  InstDir[MMAX_LINE];         /* Install directory */

  mln_t *VarList;                   /* scheduler variables (alloc) */

  char *ServerPath;                 /* the PATH (env value) of the Moab
                                       server daemon (alloc) */
  char *PathList[MMAX_PATH];        /* (alloc) */
  char  LogDir[MMAX_LINE];          /* directory for logging */
  char  LogFile[MMAX_LINE];         /* log file */
  char  ToolsDir[MMAX_LINE];        /* directory for sched tools */
  char  SpoolDir[MMAX_LINE];        /* spool directory */
  char  CheckpointDir[MMAX_LINE];   /* directory which holds non-migrated
                                       jobs' checkpoint files (if not set, we
                                       use SpoolDir) */
  char  LockFile[MMAX_LINE];        /* scheduler lock file name */
  int   LockFD;                     /* scheduler lock file descriptor */
  char  KeyFile[MMAX_LINE];

  char  Action[mactLAST][MMAX_LINE]; /* response for specified events    */

  int   LogFileMaxSize;             /* maximum size of log file */
  int   LogFileRollDepth;
  int   LogPermissions;             /* (OCTAL) Log File Permissions */

  char  ServerHost[MMAX_NAME << 1]; /* name of scheduler server host */

  char  ServerAlias[MMAX_HOSTALIAS][MMAX_NAME << 1];
  mulong ServerAliasAddr[MMAX_HOSTALIAS];

  char *TestModeBuffer;             /* (alloc) */
  int   TestModeBufferSize;
  char *TBPtr;                      /* test buffer state variable */
  int   TBSpace;                    /* test buffer state variable */

  int     ServerPort;               /* socket used to communicate with
                                       server client */
  int     HTTPServerPort;           /* socket used to provide access to
                                       mini-http server */

  char    AltServerHost[MMAX_NAME << 1]; /* when using file-lock HA, name
                                            of alternate (FB) host */

  char    MinJobAName[MMAX_NAME];   /* lowest valued user specified job name
                                       in queue */
                                    /* used by jobname priority factor */

  char   LocalHost[MMAX_NAME << 1]; /* host name of local machine */
  mulong LocalAddr;

  char  *RegistrationURL;           /* URL signifying where Moab should
                                       register its node/proc count (alloc) */

  mulong HAPollInterval;            /* interval between peer server queries */
  char   HALockFile[MMAX_PATH];     /* path to the lock file used for Moab's
                                       HA (when in file lock mode) */
  int    HALockFD;                  /* file descriptor to the HA lock file */

  mulong HALockUpdateTime;          /* how often to update the HA lock file
                                       to prevent others from taking
                                       it (heartbeat) */
  mulong HALockCheckTime;           /* how often to check HA lock file to see
                                       if we can acquire it */

  int    JobDefNodeCount;           /**< default nodes allocated per job */
  int    JobMaxNodeCount;           /**< maximum nodes allowed per job */
  int    JobMaxTaskCount;           /**< maximum tasks allowed per job */

  mulong EvalMetrics;               /* bitmap of evaluation metrics */

  /* green attributes */

  int           TempGMetricIndex;   /* gmetric index into MAList[]
                                       for temperature */
  int           WattsGMetricIndex;  /* gmetric index into MAList[] for
                                       actual consumed watts */
  int           PWattsGMetricIndex; /* gmetric index into MAList[] for
                                       passive consumed watts i.e., watts
                                       which would be consumed if active
                                       green standby provisioning was not
                                       enabled */

  int           CPUGMetricIndex;
  int           MEMGMetricIndex;
  int           IOGMetricIndex;

  enum MRecoveryActionEnum RecoveryAction;  /* action to take if critical
                                               signal received */

  int    PNodeGResIndex;            /* physical node gres index (for collapsed
                                       view nodes) */

  /* state booleans */

  mbool_t IsPreemptTest;            /* FIXME - remove in Moab 5.3+ */

  mbool_t LocalQueueIsActive;       /* local queue has been used, may
                                       contain jobs */
  mbool_t PeerConfigured;           /* moab peer rm interface configured */
  mbool_t NoJobMigrationMethod;     /* no job migration method exists */
  mbool_t AllocPartitionCleanup;    /* moab should cleanup discovered
                                       allocation partitions (XT only) */
  mbool_t IsUIPhase;                /* is the scheduling thread currently
                                       waiting for incoming UI requests and
                                       can service them immediately */
  mbool_t ResOvercommitSpecified[mrlLAST]; /**< resource overcommit factor specified */

  enum MActivityEnum Activity;       /* current activity */
  mulong  ActivityTime;              /* time currect activity started */
  int     ActivityCount[macLAST];    /* instances of activity */
  mulong  ActivityDuration[macLAST]; /* time currect activity started */
  char   *ActivityMsg;               /* human readable activity message (alloc) */

  int     DefaultJobOS;              /* OS to assign to jobs if not specified */
 
  mbool_t PeerTruncatedUICycle;     /* most recent UI cycle was truncated
                                       to process pending RM data (deprecated
                                       and replaced by R->NextUpdateTime) */

  enum MNodeSetPlusPolicyEnum NodeSetPlus;   /* extended node-set discovery
                                                and scheduling */

  mbool_t UIDeadLineReached;        /* ui phase exited with no events or
                                       refresh commands */
  mbool_t DesktopHarvestingEnabled; /* keyboard detect policy activated */
  mbool_t Reload;                   /* reload config file at next interval */
  mbool_t Shutdown;                 /* request to shutdown has been received */
  int     ShutdownSigno;            /* number of the signal received
                                       prompting shutdown */
  mbool_t ReadyForThreadedCommands; /* don't answer threaded client commands until this is set */
  mbool_t Recycle;                  /* request to recycle has been received */
  mbool_t EnvChanged;               /* scheduling env has changed */
  mbool_t IsServer;                 /* process is server daemon (not client) */
  mbool_t SRsvIsActive;             /* system rsv is active */
  mbool_t GridSandboxExists;        /* grid sandbox is active */
  mbool_t DefaultDropBadRequest;    /* drop client connections if not
                                       authorized without reporting back
                                       rejection reason */
  mbool_t MultipleComputePartitionsActive;
  mbool_t NodeFeaturesSpecifiedInConfig; /**< state boolean - feature specification detected within moab.cfg */

  mbool_t NoEvalLicense;            /* eval license is not allowed */
  mbool_t DynamicNodeGResSpecified; /* dynamic node gres was added using
                                       mnodectl -m */
  mbool_t TrigCompletedJob;         /* a trigger completed a system job...skip
                                       UI phase */
  mbool_t CredDiscovery;            /* enable cred discovery from old XML stats */
  mbool_t ChildStdErrCheck;         /* enable checking of stderr in child
                                       processes for the keyword ERROR */
  mbool_t ExtendedJobTracking;      /* track job usage extensively */
  mbool_t QOSIsOptional;            /* QOS is required for every job */
  mbool_t SideBySide;               /* running in side-by-side mode */
  mbool_t UseKeyFile;               /* server is using .moab.key for
                                       client authentication */
  mbool_t ProfilingIsEnabled;       /* profiling on ANY object has been turned
                                       on */
  mbool_t StrictProtocolCheck;      /* if a newer Moab communicates with
                                       added/unexpected XML, a strict check will
                                       cause the communication to be rejected */
  mbool_t TrackIdleJobPriority;     /* JobReservation specified separately
                                       in NODECATCREDLIST, do not group idle
                                       nodes with job reservations into idle
                                       node category */
  mbool_t ResumeOnFailedPreempt;    /* resume preempted jobs if the
                                       preemptor fails */
  mbool_t RejectInfeasibleJobs;     /* reject jobs that cannot run on any
                                       available nodes */
  mbool_t RejectDuplicateJobs;      /* reject jobs that duplicate an existing JobID */
  mbool_t EnableVPCPreemption;      /* vpc preemption is enabled */
  mbool_t BFNSDelayJobs;            /* backfill jobs with a
                                       specified nodesetdelay */

  /* NOTE: if set to TRUE, only defer jobs during the hard policy
           scheduling phase */
  mbool_t HPEnabledRsvExists;       /* rsv with HPEnabled ACL modifier exists */

  char   *PBSAccountingDir;         /* when present, put pbs compat events
                                       file here */

  int     FSTreeDepth;              /* the largest FSTree configured */

  mbool_t UseAnyPartitionPrio;      /* use the first feasible FSTree partition
                                       info for priority */
  mbool_t FSTreeIsRequired;         /* job must map to FSTree node to be eligible */
  mbool_t FSTreeUserIsRequired;     /* user must be explicitly set in the
                                       FSTree for node to be eligible */
  mbool_t FSRelativeUsage;          /* compute relative fairshare usage
                                       or absolute */
  mbool_t CoAllocationAllowed;      /* one or more QoS objects allow coallocation */
  mbool_t AllowRootJobs;            /**< allow jobs from root to be scheduled */
  mbool_t AllowVMMigration;         /**< allow VM's to migration without
                                      direct workload involvement */
  mbool_t EnableVMSwap;             /**< manage VM migration via VM
                                      swap operations */
  mbool_t VMGResMap;                /**< associate matching gres structure with each VM on parent node */
  mbool_t DefaultVMSovereign;       /**< by default, VM's are sovereign
                                      and directly consume resources */

  mbool_t ValidateVMGRes;           /**< verify VM locally possesses required generic resources */

  double  FutureVMLoadFactor;       /**< predicted load factor for future scheduled VMs */

  mbool_t DebugMode;                /* (state) */
  mbool_t InRecovery;               /* (state) */
  mbool_t MaxRsvPerNodeReached;     /* (state) */

  mbool_t UseCPRestartState;        /* Use the restart state stored in
                                       the checkpoint file when starting moab
                                       with "moab -s" command */

  msocket_t ServerS;                /* main communications socket */
  msocket_t ServerSH;               /* web interface */

  char *RecycleArgs;                /* (alloc) */
  char *SchedEnv;                   /* scheduler environment (alloc) */
  enum  MSocketProtocolEnum DefaultMCSocketProtocol;
  long  ClientTimeout;              /* (in seconds) */
  long  AuthTimeout;                /* (in seconds) */

  mbool_t SocketExplicitNonBlock;   /* use the explicit O_NONBLOCK flag
                                       when creating non-blocking sockets
                                       (instead of the O_NDELAY) */
  int   SocketLingerVal;            /* the value (in seconds) that should be
                                       used for sockets' SOLINGER */
  long  SocketWaitTime;             /* how long moab will wait for a socket
                                       to become ready for writing/reading */
  
  char                     DefaultCSKey[MMAX_LINE + 1];
  enum MChecksumAlgoEnum   DefaultCSAlgo;
  char *AuthCmdOptions;             /* Auth Cmd Options defined in CLIENTCFG  */

  /* SSL interface */

  msocket_t SServerS;

  /* directory server */

  char  DefaultDomain[MMAX_NAME];   /* domain appended to unqualified hostnames */
  char  DefaultQMHost[MMAX_NAME];

  char *DefaultMailDomain;          /* mail domain to use if not
                                       explicitly specified (alloc) */

  mrm_t *DefaultComputeNodeRM;

  madmin_t Admin[MMAX_ADMINTYPE + 1]; /* admin role information */

  char  SubmitHosts[MMAX_SUBMITHOSTS + 1][MMAX_NAME]; /* hosts allowed to
                                                        submit jobs */

  char  ClientSubmitFilter[MMAX_LINE];    /* client side submit filter program */
  char  ServerSubmitFilter[MMAX_LINE];    /* server side submit filter program */

  mulong MaxRsvDelay;               /* max time job rsv has been delayed by
                                       system issues */

  enum MDependFailPolicyEnum DependFailPolicy;   /* what to do when a job's dependency fails */

  long  DeferTime;                  /* time job will stay deferred */
  int   DeferCount;                 /* times job will get deferred before
                                       being held   */
  int   DeferStartCount;            /* times job will fail starting before
                                       getting deferred */
  int   JobFailRetryCount;          /**< times job will be retried/requeued if
                                      the job exits with a non-zero exit code */
  mulong JobPurgeTime;              /**< time job must be missing before
                                      getting purged (see MDEF_JOBPURGETIME) */
  mulong JobPreemptMaxActiveTime;   /* maximum time after which a job is no longer
                                       eligible for preemption */
  mulong JobPreemptMinActiveTime;   /* minimum time job must be active before
                                       being eligible for preemption */
  mulong JobPreemptCompletionTime;  /* mimimum time job has left before
                                       hitting wallclock limit to be eligible
                                       for preemption */
  int   JobRsvRetryInterval;        /* time to wait on job failures
                                       (cancel, preempt, start) */
  long  JobCPurgeTime;              /* time to maintain completed job info */
  long  VMCPurgeTime;               /* time to keep completed VM info */

  mbool_t ParamSpecified[mcoLAST];  /**< parameter explicitly specified */

  mbool_t JobCPurgeTimeSpecified;   /**< JobCPurgeTime explicitly specified */
  long  RsvCPurgeTime;              /* time to maintain completed rsv info */
  long  NodePurgeTime;
  long  NodeDownTime;               /* max time node can remain unreported
                                       before it will be marked down */
  int   NodePurgeIteration;         /* number of iterations to maintain node
                                       info when RM is no longer reporting node */
  long  NodeFailureReserveTime;     /* duration of reservation when node
                                       failure detected */
  int   APIFailureThreshold;        /* times API can fail before notifying admins */
  long  NodeSyncDeadline;           /* time in which node must reach
                                       expected state */
  long  JobSyncDeadline;            /* time in which job must reach
                                       expected state */
  long  JobHMaxOverrun;             /* time job is allowed to exceed wallclock limit */
  long  JobSMaxOverrun;             /* time job is allowed to exceed wallclock limit */
  double JobPercentOverrun;         /* percent of job's original wallclock it is allowed to exceed */
  int   Iteration;                  /* number of scheduling cycles completed */
                                    /* iteration -1 is before RMs are
                                     * polled, iteration 0 is when RMs are polled */
  int   RsvCounter;                 /* counter for unique rsv id's */
  int   RsvGroupCounter;            /* counter for unique rsv groups */
  int   TrigCounter;                /* counter for triggers */
  int   EventCounter;               /* counter for events */
  unsigned int   TransCounter;      /* counter for unique transaction id's */
  long            LastTransCommitted; /* last committed transaction */

  char  NodeIDBase[MMAX_NAME];
  int   NodeIDCounter;
  int   VMIDCounter;
  int   VCIDCounter;                /* counter for generating VC names */
  
  mulong RsvTimeDepth;              /* time window during which reservations
                                       will be created */

  int   CommTID;                    /* id of comm thread */

  long  IgnoreToTime;
  int   IgnoreToIteration;

  int   MaxSleepIteration;          /* max iterations scheduler can go
                                       without scheduling */

  /* policies */

  mbool_t **AResInfoAvail;         /* (alloc,size=MMAX_NODE*(MSched.M[mxoxGRes]+1)) N->ARes.GRes info is RM bound */

  enum MSchedPolicyEnum             SchedPolicy;

  enum MNodeAccessPolicyEnum        DefaultNAccessPolicy;

  enum MRsvAllocPolicyEnum          DefaultRsvAllocPolicy;

  enum MWCVioActEnum                WCViolAction;
  int                               WCViolCCode;

  enum MAMConsumptionRatePolicyEnum ChargeRatePolicy;
  enum MAMConsumptionPolicyEnum     ChargeMetric;

  double RequeueRecoveryRatio;      /* used with cost-based requeue preemption */

  enum MPolicyTypeEnum     RsvLimitPolicy;
  enum MPreemptPolicyEnum  PreemptPolicy;
  enum MAllocResFailurePolicyEnum ARFPolicy;   /* action to take when
                                                  allocated node fails for
                                                  active job */

  mbool_t    ArrayJobParLock;

  mbool_t ARFOnMasterTaskOnly;
  int ARFDuration;                             /**< duration of failure before
                                                 triggering policy */

  mbool_t AlwaysApplyQOSLimitOverride;         /* if set, always apply QoS
                                                  usage limit override even
                                                  if corresponding object
                                                  limit is not set */

  int  SingleJobMaxPreemptee;  /**< max number of preemptees allowed
                                 per preemptor */
  mbool_t AlwaysEvaluateAllJobs;               /* if set, always loop through all jobs in MQueueScheduleIJobs()
                                                  see RT 5670 (HPEnable, Partitions, ADVRes) */

  enum MDeadlinePolicyEnum DeadlinePolicy;     /* policy for handling
                                                  deadline conflicts */
  enum MMissingDependencyActionEnum MissingDependencyAction; /* action taken when dependency job cannot be found. */

  mbool_t ResendCancelCommand;  /* For migrated jobs, resend the job cancel
                                   command until the job is
                                   successfully cancelled. */

  mbitmap_t                      JobRejectPolicyBM;  /* bitmap of enum MJobRejectPolicy */
  mbitmap_t                      QOSRejectPolicyBM;  /* QOS reject policy */

  int  AddDynamicNC;       /* number of nodes currently waiting to be added */

                           /* don't allow modification of dynamic resources
                            * until they are added */

  enum MTimePolicyEnum     TimePolicy;

  int  OLogLevel;          /* original loglevel */
  int  OLogLevelIteration; /* iteration to restore loglevel */

  char *OLogFile;          /* original log file (alloc) */

  mulong FStatInitTime;

  /* scheduler load monitoring statistics */

  long   LoadStartTime[mschpLAST];
  long   LoadEndTime[mschpLAST];

  mulong Load5MDI[5][mschpLAST];
  mulong Load24HDI[24][mschpLAST];

  mulong Load5MD[mschpLAST];
  mulong Load24HD[mschpLAST];

  mulong LoadLastIteration[mschpLAST];

  mulong Load5MStart;
  mulong Load24HStart;

  long   FreeTimeLookAheadDuration; /* how far to look ahead when determing a node's freetime -MNodeGetFreeTime()
                                       default is 2 months */

  long   NonBlockingCacheInterval; /* interval for caching non-critical
                                      data (default: 60) */

  float  MountPointUsageThreshold; /* percentage of usage (how full) a mount
                                      point can get before an action must
                                      be taken */
  int    MountPointCheckInterval;  /* how often to check mount points for
                                      threshold violations */

  long   TimeOffset;               /* offset between actual and trace time */
  double TimeRatio;

  enum MSchedStateEnum State;
  char  *StateMAdmin;              /* admin which changed scheduler state (alloc) */
  long   StateMTime;               /* time scheduler state was changed */
  enum MSchedStateEnum RestartState;

  long   EstCurrentInterval;
  long   EstDuration;
  int    EstDepth;

  mtrigstats_t TrigStats;          /* trigger stats */

  double StatMaxThreshold;         /**< min and max values considered the
                                     'same' value for stats profiling */
  double StatMinThreshold;

  mulong SLTime;
  mulong ResumeTime;               /* epoch time to resume scheduling (see
                                      MSched.DisableScheduling) */
  mulong RestartTime;              /* time scheduler should restart/recycle */
  mulong RestartJobCount;          /* number of job loads before recycling */
  mulong TotalJobQueryCount;       /* number of individual RM job queries (for
                                      LL mem leak) */

  mbool_t HadCleanStart;           /* when journaling is turned on,
                                      indicates whether start was done
                                      cleanly (from a non-crashed state) */

  /* policy booleans */

  mbool_t UseJobRegEx;             /* (config) */
  mbool_t SPVJobIsPreemptible;     /* (config) soft policy violation jobs
                                      are preemptible */
  mbool_t SeekLong;                /* (config) */
  enum MRsvSearchAlgoEnum RsvSearchAlgo;
  mbool_t NoLocalUserEnv;          /* (config) user ID, group ID, homedir,
                                      etc, not present on Moab server host */
  mbool_t AdjustUtlRes;            /* (config) */
  mbool_t FileRequestIsJobCentric; /* (config) */
  mbool_t EnableInternalCharging;  /* (config) */
  mbool_t BluegeneRM;              /* (autodetect) */
  mbool_t TrackUserIO;             /* (config) track User IO (vs track Application IO) */
  mbool_t ForceNodeReprovision;    /* (config) force nodes to be reprovisioned */
  mbool_t ManageTransparentVMs;    /* (config) virtual children should be hidden within parent node */
  mbool_t UseMoabCTime;            /* (config) use moab creation time in determining job's EffQueueDuration */

  mbool_t EnableFailureForPurgedJob;  /* (config) if set, a job that disappears from an RM will not be assumed to be completed/canceled--it will be marked as failed (only for TORQUE right now) */
  mbool_t EnableHighThroughput;    /* (config) if set, set various high throughput optimizations (ie, NoEnforceFuturePolicy) */
  mbool_t EnableFastNodeRsv;       /* (config) if set, use fast node reservations in MnodeBuildRE */

  mbool_t DisableMRE;              /* (config) if set, do not use global rsv event table */
  mbool_t NoEnforceFuturePolicy;   /* (config) if set, do not enforce checking of future object usage limits */
  mbool_t EnableFastNativeRM;      /* (config) if set, enable high throughput native interface */
  mbool_t EnableFastSpawn;         /* (config) if set, enable high throughput MUSpawnChild() */
  mbool_t UseDRMJID;               /**< J->DRMJID specified and differs from J->Name */

  mbool_t FilterCmdFile;           /* (config) if set to FALSE, job scripts are not filtered (leave comments, blank lines, etc.) default is TRUE*/

  mbool_t StoreSubmission;         /* (config) If set, jobs submission strings will be included in event files */
  mbool_t LimitedJobCP;            /* (config) */
  mbool_t LimitedNodeCP;           /* (config) */
  mbool_t LoadAllJobCP;            /* (config) - load all jobs from checkpoint file at startup, regardless of RM reports */
  mbool_t FullJobTrigCP;           /* (config) */
  mbool_t JobCTruncateNLCP;        /* (config) truncate the nodelist stored for completed jobs in the checkpoint file */
  mbool_t EnableJobArrays;         /* (config) */
  mbool_t OptimizedJobArrays;      /* (config) */
  mbool_t EnableEncryption;        /* (config) */
  mbool_t MapFeatureToPartition;   /* (config) */
  mbool_t RejectDosScripts;        /* (config) */
  mbool_t ForceRsvSubType;         /* (config) */
  mbool_t ForceTrigJobsIdle;       /* (config) -- forces "trigger jobs" to be idle until one of its triggers is eligible to run */
  mbool_t IgnorePreempteePriority; /* (config) */
  mbool_t IgnoreLLMaxStarters;     /* (config) */
  mbool_t DisableSlaveJobSubmit;   /* (config) - do not allow jobs to be submitted from a slave */
  mbool_t EnableJobEvalCheckStart; /* (config) - enable the check at job submission to see if the job will start */
  mbool_t EnableNodeAddrLookup;    /* enable nameserver address lookups on nodes reported by rm) */
  mbool_t CheckSuspendedJobPriority; /* (config) */

  /* (config,BM of enum MRecordEventListEnum) */
  mbitmap_t RecordEventList;

  mbool_t RecordEventToSyslog;
  mbool_t NoWaitPreemption;        /* (config) Disable the reservation retry time waiting when preempting jobs */

  mbitmap_t RealTimeDBObjects;

#ifdef __MNOT
  mbool_t PercentBasedFS;          /* use percentage based fairshare
                                      target priority calculations) */
#endif /* __MNOT */

  mbool_t ReportPeerStartInfo;     /* report start estimate info to peers */
  mbool_t UseMoabJobID;            /* always use the moab job id */
  mbool_t UseNodeIndex;            /* use node index lookup table */
  mbool_t UseCPRsvNodeList;        /* use the checkpointed rsv ReqNodeList */
  mbool_t RemoteFailTransient;     /* (config) */
  mbool_t EnableUnMungeCredCheck;  /* (config) check the munge creds (name,
                                      uid, group name, gid) against local creds */
  mbool_t ReportProfileStatsAsChild; /* (config) report profile stat values as
                                        xml children */
  mbool_t JobExtendStartWallTime;  /* (config) if TRUE then we support walltime extension at job start if a min and max walltime supplied on the msub */

  /* NOTE:  UseNodeIndex must be set before ANY node is loaded, currently this
            is done in MRMProcessConfig() and MNatInitialize() */

  /* NOTE:  if Ignore* is specified, Spec* is ignored */

  char   **IgnoreClasses;          /* (optional,alloc) - jobs in classes
                                      specified are ignored */
  mlr_t    IgnoreNodes;            /* (optional,alloc) - nodes specified
                                      are ignored */
  char   **IgnoreUsers;            /* (optional,alloc) - users specified
                                      are ignored */
  char   **IgnoreJobs;             /* (optional,alloc) - jobs specified
                                      are ignored */

  char   **SpecClasses;            /* (optional,alloc) - jobs in classes
                                      not specified are ignored  */
  mlr_t    SpecNodes;              /* (optional,alloc) - nodes not specified
                                      are ignored  */
  char   **SpecUsers;              /* (optional,alloc) - users not specified
                                      are ignored  */
  char   **SpecJobs;               /* (optional,alloc) - jobs not specified
                                      are ignored */

  char   *LaunchDir;               /* directory from which Moab was
                                      launched (alloc) */

  char   *ArgV[MMAX_ARG];          /* command line arguments (alloc) */
  int     ArgC;                    /* command line argument count */

  char   *SpecEMsg[memLAST];       /* customizable error messages (alloc) */

  int   ResourceQueryDepth;           /* depth of resource availability
                                         query responses */
  int   MaxRsvPerNode;                /* maximum allowed number of rsvs per node */
  int   MaxRsvPerGNode;               /* maximum allowed number of rsvs per
                                         global/shared node */
  char  DefaultClassList[MMAX_LINE];  /* list of classes if none are specified
                                         by RM */

  long  CurrentFSInterval;
  mbitmap_t  DisplayFlags;      /* (BM) */
  mbitmap_t  TraceFlags;        /* (BM) */
  int   NodePollFrequency;
  int   NodePollOffset;
  char  ProcSpeedFeatureHeader[MMAX_NAME];
  char  NodeTypeFeatureHeader[MMAX_NAME];
  char  PartitionFeatureHeader[MMAX_NAME];
  char  RackFeatureHeader[MMAX_NAME];
  char  SlotFeatureHeader[MMAX_NAME];

  int     DefaultRackSize;   /* default node slots per rack */

  mbool_t InitialLoad;

  mgcred_t *DefaultU;    /* pointer only */
  mgcred_t *DefaultG;    /* pointer only */
  mgcred_t *DefaultA;    /* pointer only */
  mqos_t   *DefaultQ;    /* pointer only */
  mclass_t *DefaultC;    /* contains attributes to be used when local
                            class attributes are not set */
  mnode_t   DefaultN;
  mjob_t   *DefaultJ;    /* pointer to default job template */

  mjob_t   *NTRJob;      /* points to the VIP job that should run next (specified by QFLAGS=NTR) */

  mnode_t  *DefaultVMTemplate;  /**< node template containing default VM
                                  attributes (alloc) */
  mulong    MinVMOpDelay;       /**< amount of time to wait between subsequent rounds of VM 
                                     operations on compute nodes */
  mbool_t NodeCatCredEnabled;

  mgcred_t *NodeCatU[mncLAST];    /* pointer only */
  mgcred_t *NodeCatG[mncLAST];    /* pointer only */
  mgcred_t *NodeCatA[mncLAST];    /* pointer only */
  mqos_t   *NodeCatQ[mncLAST];    /* pointer only */
  mclass_t *NodeCatC[mncLAST];    /* pointer only */

  char     *NodeCatGroup[mncLAST];       /* node cat cred group name */
  int       NodeCatGroupIndex[mncLAST];  /* map node cat to node cat cred group */

  mnode_t  *GN;          /* global node (optional,pointer only) */

  mrm_t    *InfoRM;      /* global information service (optional,pointer only) */
  mrm_t    *DefStorageRM;   /* storage manager (optional,pointer only) */
  mrm_t    *NetworkRM;   /* network manager (optional,pointer only) */
  mrm_t    *ProvRM;      /* provisioning manager (optional,pointer only) */

  mulong    DefProvDuration;  /* default provisioning duration if not
                                 specified on a per image basis */
  mulong    DefaultTaskMigrationDuration; /**< default delay for migrating
                                            tasks w/in active jobs */

  enum MJobMigratePolicyEnum JobMigratePolicy;  /* grid job migration policy */
  mbool_t EnableImplicitStageOut;       /* stage-out stdout/stderr
                                           files automatically */

  mrm_t    *InternalRM;  /* internal rm/global queue */

  mclass_t *RemapC;      /* specified class to be remapped class (pointer only) */
  mclass_t *RemapCList[MMAX_CLASS]; /* destination classes to map to (pointer only) */

  char     *UnsupportedDependencies[MMAX_UNSUPPORTEDDEPENDLIST+1]; /* unsupported dependency types (NULL terminated)*/

  int       SharedPtIndex;

  char     *GridSandboxRID;  /* (alloc) */
  char     *PRsvSandboxRID;  /* (alloc) */

  mbool_t   EnforceAccountAccess;
  mbool_t   DontEnforcePeerJobLimits;
  mbool_t   DisplayProxyUserAsUser;
  mbool_t   VMCalculateLoadByVMSum;
  mbool_t   MigrateToZeroLoadNodes;

  char   DefaultAccountName[MMAX_NAME];  /* account to be used by job if no
                                            account is specified */
  mclass_t *FallBackClass;               /* class to be used by job if no
                                            class is specified */

  int    ReferenceProcSpeed;
  long   PreemptPrioMargin;
  double PreemptPrioWeight;
  double PreemptRTimeWeight;

  long    MaxJobStartPerIteration;    /**< maximum number of jobs allowed to
                                        start per iteration */
  long    MaxPrioJobStartPerIteration;/**< maximum number of jobs allowed to
                                        start via priority scheduling per iteration */
  int    MaxJobPreemptPerIteration;  /**< maximum number of jobs allowed to
                                        preempt per iteration */
  long   MaxJobHoldTime;

  int    JobStartPerIterationCounter;
  int    JobPreempteePerIterationCounter;
  int    JobPreemptorPerIterationCounter;

  int    JobCCodeFailSampleSize;     /* number of "samples" (finished jobs) to
                                        take per stat bucket */

  int    TotalJobStartCounter;       /* sum of job start requests since launch */
  int    TotalJobPreempteeCounter;   /* sum of successful job preemptee
                                        actions since launch */
  int    TotalJobPreemptorCounter;   /* sum of successful job preemptor
                                        actions since launch */

  long   MinDispatchTime;
  double MaxOSCPULoad;

  mbool_t NodeNameCaseInsensitive;   /* (config) true if node names should be
                                        case insensitive */
  mbool_t UseSyslog;      /* config boolean */
  mbool_t SyslogActive;   /* state boolean */
  mbool_t SyslogFacility;
  mbool_t ForceBG;

  mbool_t DisableTriggers;         /* do not launch triggers */
  mbool_t EnforceInternalCharging; /**< use account costing */
  mbool_t DisableSameQOSPreemption; /* do not allow jobs from the same QOS to preempt each other regardless of priority */
  mbool_t EnableFSViolationPreemption; /* allow a job in the same class to preempt if preemptee is FS violator */
  mbool_t EnableSPViolationPreemption; /* allow a job in the same class to preempt if preemptee is SPV */
  mbool_t DisableJobFeasibilityCheck; /* do not verify jobs can run in the future */
  mbool_t DisableBatchHolds;          /* disable BatchHolds in Moab (MJobSetHold ignores holds of type mhBatch) */
  mbool_t DisableExcHList;         /* do not add node to a job's Exclude Host List if the RM rejects the job on that node */
  mbool_t DisableInteractiveJobs;  /* do not allow interactive jobs (msub -I) */
  mbool_t DisableUICompression;    /* do not allow incoming or outgoing user interface compression */
  mbool_t DisableRequiredGResNone; /* do not allow gres of "none" when ENFORCEGRESACCESS is enabled */

  mbool_t RemoveTrigOutputFiles;    /* set all triggers to automatically
                                       delete their stdout/stderr files */

  mbitmap_t  DisableSameCredPreemptionBM;  /* BM of MAttrEnum */

  enum MVPCMigrationPolicyEnum VPCMigrationPolicy;      /* virtual private
                                                           cluster
                                                           migration policy */

  enum MFSTreeACLPolicyEnum FSTreeACLPolicy;

  mbool_t ResourcePrefIsActive;    /* state boolean */

  double  FSChargeRate;            /* FS to compute conversion rate */
  double  NetworkChargeRate;       /* network to compute conversion rate */
  double  LicenseChargeRate;       /* license to compute conversion rate */

#ifdef __SANSI__
  int     PID;
#else /* __SANSI__ */
  pid_t   PID;
#endif /* __SANSI__ */

  int     UID;             /* server user id */
  int     GID;             /* server group id */

  time_t  ActionDate;      /* Future license action Date */
  int     MaxCPU;          /* max cpu per scheduler (from license) */
  int     TotalCPU;        /* total configured processors */
  int     MaxVM;           /* max vm per scheduler (from license) */

  int     UMask;           /* default file creation mask */

  mpar_t    *GP;

  char     **IgnQList;

  void      *T[mxoLAST];   /* object table   */
  int        Size[mxoLAST];   /* size of object */
  int        AS[mxoLAST];  /* allocated size of object (including dynamic memory) */
  int        M[mxoLAST];   /* max number of objects in table */
  int        A[mxoLAST];   /* active number of objects in table */
  int        Max[mxoLAST]; /* maximum number of active objects in table */
  void      *E[mxoLAST];   /* last object in table */

  char      *FailedOSLookupUser;  /* user name which cannot be located in
                                     password file (alloc) */
  char      *FailedOSLookupUserMsg; /* error info associated with user
                                       lookup failure (alloc) */

  char      *OFOID[mxoLAST]; /* overflow object (alloc) */
  char      *JobMaxNodeCountOFOID;  /* job max node count overflow job id
                                       (alloc) */
  char      *UIBufferMsg;   /* job buffer overflow (alloc) */
  char      *UIBadRequestMsg;  /* last bad remote request (alloc) */
  char      *UIBadRequestor;   /* last bad remote request source (alloc) */

  char      *AccountingInterfaceURL;  /* (alloc) */
  enum MBaseProtocolEnum AccountingInterfaceProtocol;
  char      *AccountingInterfacePath; /* (alloc) */

  char      *Copy;         /* general extension copyright notice */

  mmb_t     *MB;           /* general system messages (alloc) */

#if 0
  mgrid_t    G;
#endif

  mjpriof_t *JPrioF;       /* (alloc?) */

  mhash_t    Variables;                /**< global scheduler variables (alloc)
                                         - see MSched.VarList */
  mhash_t    VMTable;                  /**< global VM mapping table */
  mhash_t    VMCompletedTable;         /**< table of completed/destroyed VMs */

  int        VMStaleIterations;        /**< number of iterations that a VM must be unreported to be stale */
  enum MVMStaleActionEnum VMStaleAction;/**< action to take on stale VMs */

  mhash_t    OSList;
  mhash_t    VMOSList[MMAX_ATTR + 1];
  int        VMOSSize[MMAX_ATTR];      /**< minimum memory required to service OS */

  marray_t  *TList;                    /* trigger list (alloc) */
  mtrig_t   *LogRollTrigger;           /* trigger to fire when rolling logs */

  mbitmap_t  TrigPolicyFlags;          /* (bitmap of enum MTrigPolicyFlagEnum) */

  char    MsgErrHeader[MMAX_NAME];
  char    MsgInfoHeader[MMAX_NAME];

  char       NodeToJobAttrMap[MMAX_ATTR][MMAX_NAME];

  int        GResCount;                       /* number of generic resources currently loaded */
  mbool_t    EnforceGRESAccess;               /* if true, don't dynamically add gres from submission */

  mgevent_list_t GEventsList;                 /* GEvents that have no parent obj, add with mgevent_obj_t (alloc elements) */

  mbool_t    LocalEventReceived;

  mbool_t    ParBufOverflow;            /* partition buffer overflow detected (deprecated by OFOID?) */
  mbool_t    NodeREOverflowDetected;    /* node R/RE overflow detected (no OFOID support) */

  mbool_t    RMMsgIgnore;  /* do not take action when RM generated node error messages are detected */

  char   MAPServerHost[MMAX_APSERVER][MMAX_NAME];
  mulong MAPServerTime[MMAX_APSERVER];

  int    CfgOS[MMAX_ATTR];
  int    CfgArch[MMAX_ATTR];
  int    CfgMem[MMAX_ATTR+1];  /* need extra entry for MSchedUpdateCfgRes() which terminates list */

  int    MaxCfgMem;

  char   TrustedCharList[MMAX_LINE];
  char   TrustedCharListX[MMAX_LINE];

  int    TotalSystemProcs;

  mulong LicensedFeatures;  /* license features, ie grid, etc - read from
                               license file (bitmap of enum
                               MLicenseFeaturesEnum) */
  char  *LicMsg;            /* (alloc) */

  /* function pointers */

  int (*RMILoadModule)(void);
  int (*HTTPProcessF)(msocket_t *,char *);
  int (*SQLQuery)(msql_t *,char *,char *,int);
  int (*SQLInsert)(msql_t *,char *,char *,int);

  int DefaultJobOREnd;    /* ??? */

  mbool_t  FSTreeConfigDetected;  /* config file contains FSTree parameters */
  mbool_t *ParHasFSTree;  /* partitions has local share tree (alloc)*/
  mbool_t  FSMostSpecificLimit;  /* Fairshare Limit checking for a given
                                    policy stops once most specific
                                    limit succeeds */

  mbool_t  TrackSuspendedJobUsage;

  int      JobStartThreshold; /* threshold for the MJobStartRsv method (off by default) */
  mbool_t  LogLevelOverride;  /* set to TRUE to use a loglevel specified by a client command
                                 for the duration of that command */

  mulong   MaxSuspendTime; /* The maximum time a job is allowed to be
                              suspended before being requeued */

  struct mdb_t  MDB;
  enum MDBTypeEnum ReqMDBType;   /* database type requested by client configuration */
  int  DBIterationRetries;       /* number of retries this iteration */

  char        *WebServicesURL;        /* URL to Web Services */
  char        *EventLogWSURL;         /* URL to push event logs to */

  mvaddress_t *AddrList;
  int          AddrListSize;

  char        *ExitMsg;        /* (alloc) */

  mbool_t      DefaultPseudoJobRMIndexSet;
  int          DefaultPseudoJobRMIndex;  /* RM to be associated with
                                            temporary/internal pseudo-jobs */

  mbool_t UseUserHash;          /* (config) Lookup user using the user hash
                                   value instead of iterating through all users */
  mbool_t DeleteStageOutFiles;  /* (config) Delete explicit stage out files
                                   after successful staging */

  int MaxGreenStandbyPoolSize;  /**< Maximum number of idle nodes to leave up
                                  in green node */

  mbool_t AggregateNodeActions;   /**< aggregate green jobs */
  int     AggregateNodeActionsTime; /**< time to aggregate node actions */

  enum MCalPeriodEnum GreenPoolEvalInterval; /**< how often to evaluate and modify the GeenPool size */

  mbool_t             RunVMConsolidationMigration; /* Continuously do consolidation migration */
  mbool_t             RunVMOvercommitMigration;    /* Continuously do overcommit migration */


  int         TPSize;           /* (config) size of thread pool. TPSize <= 0
                                   means do not allocate a thread pool */

  mbool_t DontCancelInteractiveHJobs; /* Determines if interactive jobs will
                                         be canceled or not if can't run -
                                         default = true */
  mulong DynamicRMFailureWaitTime; /* Length of time in seconds to wait
                                      before severing ties with a dynamic
                                      resource manager */

  mbool_t EnableHPAffinityScheduling; /* temporary fix for jobs to go to
                      correct reservation on correct partition ex. (ParA,
                      Rsvs(A+ ~B-), AccountA) and (ParB, Rsvs(~A- B+),
                      AccountB).  AccountB job should go to RsvB on ParB
                      but because ParA is listed first, the job gets a
                      reservation on ParA and breaks out of priority
                      scheduling, not looking at ParB.  This bool allows ParB
                      to be looked at even though a reservation was created
                      in MQueueScheduleIJobs. The proposed correct solution is
                      to pre-prioritize the partitions in MQueueSheduleIJobs
                      and look at the generated partititon list instead of
                      walking over the partititons in order. */

  mbool_t NativeAM;            /* (automatically set) native AM is in use on the system */

  mbool_t MDiagXmlJobNotes;    /* (config) mdiag -j -v --xml command
                                  includes checkjob -v style NOTES */
  mbool_t PerPartitionScheduling; /* set to TRUE to prioritize and schedule jobs on a per partition basis */
  mbool_t OnDemandProvisioningEnabled; /* set to TRUE if there is at least one
                                          QOS with the mqfProvision flag bit set */
  mbool_t OnDemandGreenEnabled;        /* set to true if there is at least one node with mpowpOnDemand */
  mbool_t TwoStageProvisioningEnabled; /* set to TRUE if there is at least one
                                          prov RM which can perfrom two-stage provisioning */

  int     NodeIdlePowerThreshold;      /* the number of seconds to let a node
                                          stay idle before powering it down
                                          (on demand provisioning model) */

  mbool_t NoJobHoldNoResources; /* (config) don't hold jobs that have no resources available (e.g. class exists but has no nodes) */


  mbool_t RelToAbsPaths; /* RT 3894 (PNNL) slurm 1.2.30 relative paths don't
                            work so make absolute. */
  mbool_t VMsAreStatic; /* specifies that moab will not schedule vm creations
                           or migrations ever */

  mbool_t IsClient;     /* To specify whether we are in a client command or not (to avoid loading certain scheduler parameters like triggers) */

  mbool_t AllowMultiReqNodeUse; /* Specifies whether to allow multiple reqs on
                                   a single job to share the same node  */

  pid_t   UIChildPid;          /* process id of moab user interface child process */

  mbool_t ShowMigratedJobsAsIdle; /* (config) Show migrated jobs as idle not blocked. (Default:FALSE) */

  mbool_t BGPreemption; /* (config) BG:TRUE=preempt preemptees of exact size or using smallest preemptee jobs first. 
                                       FALSE=preempt preemptees where the preemptor could start the soonest 
                                                    if no preemption were to happen. ie. preemptees don't have
                                                    much time left to finish. */

  int BGNodeCPUCnt;           /* (slurm/bluegene) how many procs there are per c-node. */
  int BGBasePartitionNodeCnt; /* (slurm/bluegene) how many nodes there are per slurm node/mid-plane. */

  mbool_t GuaranteedPreemption; /* (config) Don't start a preemptor until all the preemptees have finsihed being preempted (ex. job is completed). */

  int VMCreateThrottle;         /* limits the number of simultaneous vm create jobs */
  int VMMigrateThrottle;        /* limits the number of simultaneous vm migrate jobs */
  int VMStorageNodeThreshold;   /* Threshold percentage for the maximum amount of storage that can be allocated by storage mounts*/

  mbool_t DisableRegExCaching;  /* Disable caching in MUREToList */

  mbool_t IgnoreRMDataStaging; /* (config) Ignore the resource manager's data staging. ex. qsub -W stagein=... */

  enum MHoldTypeEnum DataStageHoldType;       /* type of hold to do when a data-staging operation fails */

  long RMRetryTimeCap;    /* seconds - cap for the rm retry interval time (grows exponentially) */ 

  int MWikiSubmitTimeout; /* seconds - timeout for MWikiJobSubmit */

  mbool_t DisableVMDecisions;   /* Disables decisions related to VM services -
                                   automatic VM migration, node provisioning, node power changes,
                                   and threshold triggers.  Places these events in the event logs */

  mbool_t DisableThresholdTrigs;/* Disable threshold trigger actions, but record in event logs */

  char   *TVarPrefix;           /* Prefix to place on all internally created trigger variables */
  char   *VMStorageMountDir;    /* Directory in the VM to place mount points */
  char   *VMProvisionStatusReady;      /* Variable to check to see if a VM is up and ready */
 
  mln_t  *VMTrackingSubmitted;  /* VM Tracking jobs that have been submitted but not started (alloc) */
  mbool_t AssignVLANFeatures;   /* Forces any VMTracking job to request a VLAN feature (assigned if not requested) */
  mbool_t VMTracking;           /* config option to use the new VMTracking system job VM model */

  mln_t  *NestedNodeSet;        /* used to recursively search nodesets */
  int    *StartUpRacks;         /* used to speed up startup, tracks which racks had a netname set in any slot (alloc) - used by MNodeGetLocation, released after startup */

  mbool_t OptimizedCheckpointing; /* Will only checkpoint certain objects on creation, modification, and shutdown (instead of every checkpoint interval) */
  mbool_t WikiEvents;           /* Event logs will use the wiki format to describe objects (nodes, jobs, rsvs) instead of the trace format */

  char *JobRemoveEnvVarList;    /* List of environment variables to remove from E.BaseEnv before migrating job. */

  mbool_t QueueNAMIActions;     /* If TRUE, Moab will queue up create, charge, and destroy actions to avoid multiple execs */

  char *NAMITransVarName;       /* The name to give to a variable that is used for a NAMI TID (alloc) */

  int QOSRsvCount[MMAX_QOS]; /* Counter for keeping track of per iteration qos priority reservation counts. */
  int ParRsvCount[MMAX_PAR]; /* Counter for keeping track of per iteration partition priority reservation counts. */

  int    StopIteration;
  mqos_t    *QOSDefaultOrder[MMAX_QOS];

  mbool_t EnableVMDestroy;      /* If TRUE, Moab is allowed to perform automatic VM destruction (stale VMs, expired VMs, etc.) */

  char *SubmitEnvFileLocation; /**< options are FILE or PIPE. If FILE, the job the environment will written a file and passed to SLURM. 
                                    If PIPE, the environment will be piped to sbatch. */

  mbool_t WebServicesConfigured;  /* TRUE if Moab was configured (compile-time) to use web services */

  mbool_t PushCacheToWebService;  /* If TRUE, will push cache information to configured web services */
  mbool_t PushEventsToWebService; /* If TRUE, will push events info to configured web services */

  char *EventLogWSUser;           /* Optional user name to be used when sending events to web services */
  char *EventLogWSPassword;       /* Optional password to be used when sending events to web services */

  mbool_t EventLogWSRunning;      /* If TRUE, event log web services will continue running */

  char *MongoServer;              /* Mongo server address --> hostname[:portnumber] */
  char *MongoPassword;            /* Mongo password for authentication (only used if MongoUser also given) */
  char *MongoUser;                /* Mongo user for authentication (only used if MongoPassword also given) */
  mbool_t MongoServerIsUp;        /* If TRUE, Mongo connection is working correctly */
  } msched_t;

extern mhash_t MUserHT;
extern mhash_t MAcctHT;
extern mhash_t MGroupHT;

extern mdb_t MDBFakeHandle; /* used as a stub handle to avoid NULL checks */
extern mhash_t MDBHandles;
extern mmutex_t MDBHandleHashLock;

extern mhash_t MActiveJobs;
extern mlog_t mlog;

extern mhash_t MActiveJobs;

#define MDEF_JOBOREND 12000

#define MMAX_JOBCVAL 32  /* maximum number of contstraint values per type */

#define MDEF_NODE_FAILURE_CCODE 99 /* the completion code given to jobs that
                                      Moab internally marks as failed due to
                                      failed nodes */
#define MDEF_PURGED_JOB_CCODE   98 /* the completion code given to jobs that
                                      are purged when EnableFailureOnPurgedJob
                                      is TRUE */


typedef struct mjobconstraint_t {
  enum MJobAttrEnum AType;

  /* use of one or the other is based on AType */

  char AVal[MMAX_JOBCVAL][MMAX_NAME];
  long ALong[MMAX_JOBCVAL];

  enum MCompEnum ACmp;
  } mjobconstraint_t;

typedef struct mnodeconstraint_t {
  enum MNodeAttrEnum AType;

  /* use of one or the other is based on AType */

  char AVal[MMAX_JOBCVAL][MMAX_NAME];
  long ALong[MMAX_JOBCVAL];

  enum MCompEnum ACmp;
  } mnodeconstraint_t;

typedef struct mrsvconstraint_t {
  enum MRsvAttrEnum AType;

  /* use of one or the other is based on AType */

  char AVal[MMAX_JOBCVAL][MMAX_NAME];
  long ALong[MMAX_JOBCVAL];

  enum MCompEnum ACmp;
  } mrsvconstraint_t;

typedef struct mtrigconstraint_t {
  enum MTrigAttrEnum AType;

  /* use of one or the other is based on AType */

  char AVal[MMAX_JOBCVAL][MMAX_NAME];
  long ALong[MMAX_JOBCVAL];

  enum MCompEnum ACmp;
  } mtrigconstraint_t;

typedef struct mvcconstraint_t {
  enum MVCAttrEnum AType;

  /* use of one or the other is based on AType */

  char AVal[MMAX_JOBCVAL][MMAX_NAME];
  long ALong[MMAX_JOBCVAL];

  enum MCompEnum ACmp;
  } mvcconstraint_t;

typedef struct mvmconstraint_t {
  enum MVMAttrEnum AType;

  /* use of one or the other is based on AType */

  char AVal[MMAX_JOBCVAL][MMAX_NAME];
  long ALong[MMAX_JOBCVAL];

  enum MCompEnum ACmp;
  } mvmconstraint_t;

/* extension support */

typedef struct mtrma_t {
  mulong MTime;
  mulong ETime;
  long WallTime;

  long WCLimit;

  int  JobSwapLimit;
  int  JobMemLimit;
  int  JobDiskLimit;

  int  NCPUs;
  int  NodesRequested;
  int  ProcsRequested;

  long ProcCPULimit;
  long JobCPULimit;

  long UtlJobCPUTime;

  char MasterHost[MMAX_NAME];
  } mtrma_t;

/* Web Service information - URL, password, etc. */

typedef struct mwsinfo_t {
  char *URL;
  char *Username;
  char *Password;
  char *HTTPHeader;
  } mwsinfo_t;


/* sync w/enum MRMFuncEnum */

typedef struct mrmfunc_t {
  int (*Alloc)(mrm_t *,char *,char *,char *,char *,enum MStatusCodeEnum *);
  int (*ClusterQuery)(mrm_t *,int *,char *,enum MStatusCodeEnum *);
  int (*CredCtl)(mrm_t *,const char *,char *,char *,enum MStatusCodeEnum *);
  int (*CycleFinalize)(mrm_t *,int *);
  mbool_t (*DataIsLoaded)(mrm_t *,enum MStatusCodeEnum *);
  int (*GetData)(mrm_t *,mbool_t,int,mbool_t *,char *,enum MStatusCodeEnum *);
  int (*InfoQuery)(mrm_t *,char *,char *,char *,char *,enum MStatusCodeEnum *);
  int (*Initialize)();
  int (*JobCancel)(mjob_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
  int (*JobCheckpoint)(mjob_t *, mrm_t *,mbool_t,char *,int *);
  int (*JobGetProximateTasks)(mjob_t *,mrm_t *,mnl_t **,mnl_t **,long,mbool_t,char *,int *);
  int (*JobMigrate)(mjob_t *,mrm_t *,char **,mnl_t *,char *,enum MStatusCodeEnum *);
  int (*JobModify)(mjob_t *,mrm_t *,const char *,const char *,const char *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
  int (*JobRequeue)(mjob_t *,mrm_t *,mjob_t **,char *,int *);
  int (*JobResume)(mjob_t *,mrm_t *,char *,int *);
  int (*JobSetAttr)(mjob_t *,mrm_t *,void *,char *,mtrma_t *,int *,mbool_t,int *);
  int (*JobSignal)(mjob_t *,mrm_t *,int,char *,char *,int *);
  int (*JobStart)(mjob_t *, mrm_t *, char *, enum MStatusCodeEnum *);
  int (*JobSubmit)(const char *,mrm_t *,mjob_t **,mbitmap_t *,char *,char *,enum MStatusCodeEnum *);
  int (*JobSuspend)(mjob_t *,mrm_t *,char *,int *);
  int (*JobValidate)(mjob_t *,mrm_t *,int,char *,char *,int *);
  int (*NodeMigrate)(mnode_t *,mrm_t *,mnode_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
  int (*NodeModify)(mrm_t *,char *,char *,char *,char *,enum MStatusCodeEnum *);
  int (*NodePower)(mrm_t *,char *,char *,char *,char *,enum MStatusCodeEnum *);
  int (*NodeVirtualize)(mnode_t *,char *,char *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
  int (*QueueCreate)(mclass_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
  int (*QueueModify)(mclass_t *,mrm_t *,enum MClassAttrEnum,char *,void *,char *,enum MStatusCodeEnum *);
  int (*QueueQuery)(mrm_t *,int *,char *,enum MStatusCodeEnum *);
  int (*Ping)(mrm_t *,enum MStatusCodeEnum *);
  int (*Reset)(mrm_t *);
  int (*ResourceModify)(mnode_t *,mvm_t *,char *,mrm_t *,enum MNodeAttrEnum,void *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
  int (*ResourceQuery)(mnode_t *,mrm_t *,char *,int *);
  int (*RsvCtl)(mrsv_t *,mrm_t *,enum MRsvCtlCmdEnum,void **,char *,enum MStatusCodeEnum *);
  int (*RMEventQuery)(mrm_t *,int *);
  int (*RMFailure)(mrm_t *,enum MStatusCodeEnum);
  int (*RMStart)(mrm_t *,char *,char *,enum MStatusCodeEnum *);
  int (*RMStop)(mrm_t *,char *,char *,enum MStatusCodeEnum *);
  int (*RMInitialize)(mrm_t *,char *,enum MStatusCodeEnum *);
  int (*RMQuery)(void);
  int (*SystemModify)(mrm_t *,const char *,const char *,mbool_t,char *,enum MFormatModeEnum, char *,enum MStatusCodeEnum *);
  int (*SystemQuery)(mrm_t *,char *,char *,mbool_t,char *,char *,enum MStatusCodeEnum *);
  int (*WorkloadQuery)(mrm_t *,int *,int *,char *,enum MStatusCodeEnum *);
  int (*Free)(mrm_t *);
  int (*ResourceCreate)(mrm_t *,enum MXMLOTypeEnum,char *,mstring_t *,char *,enum MStatusCodeEnum *);

  mbool_t IsInitialized;
  } mrmfunc_t;

/* macros */

#define MOINITLOOP(OP,OI,OE,ITER) \
  *(OP)=MSched.T[OI];      \
  *(OE)=MSched.E[OI];      \
  MUHTIterInit(ITER);

/* Node state management */

#define MNODEISUP(N) MNODESTATEISUP((N)->State)

/* see MNSISUP() - sync -  should NOT be two macros for same thing (FIXME) */

#define MNODESTATEISUP(S) \
  ((((S) == mnsIdle) || ((S) == mnsActive) || ((S) == mnsBusy) || \
    ((S) == mnsUp) || ((S) == mnsUnknown) || ((S) == mnsDraining) || \
    ((S) == mnsFlush) || ((S) == mnsReserved)) ? TRUE : FALSE)

#define MNODEISGREEN(N) \
  ((((N)->PowerSelectState == mpowOff) || ((N)->PowerSelectState == mpowHibernate) || \
    ((N)->PowerSelectState == mpowStandby)) ? TRUE : FALSE)

#define MNODEISACTIVE(N) \
  ((((N)->State == mnsBusy) || ((N)->State == mnsActive) || \
    ((N)->State == mnsDraining)) ? TRUE : FALSE)

#define MNODEISAVAIL(S) \
  ((((S) == mnsIdle) || ((S) == mnsActive) || ((S) == mnsUp)) ? TRUE : FALSE)

/* see MNODESTATEISUP() - sync - should NOT be two macros for same thing (FIXME) */

#define MNSISUP(NS) \
  (((NS == mnsIdle) || (NS == mnsActive) || (NS == mnsBusy) || \
    (NS == mnsUp) || (NS == mnsUnknown) || (NS == mnsDraining) || \
    (NS == mnsFlush) || (NS == mnsReserved)) ? TRUE : FALSE)

#define MNSISACTIVE(NS) \
  (((NS == mnsBusy) || (NS == mnsActive) || \
    (NS == mnsDraining)) ? TRUE : FALSE)

#define MNODEISDRAINING(N) \
  ((((N)->State == mnsDraining) || ((N)->State == mnsDrained) || \
    ((N)->State == mnsFlush)) ? TRUE : FALSE)

#define MPOWPISGREEN(P) \
  (((P == mpowpGreen) || (P == mpowpOnDemand)) ? TRUE : FALSE)

/* job state management */

#define MJOBISACTIVE(J) \
  ((((J)->State == mjsStarting) || ((J)->State == mjsRunning)) ? TRUE : FALSE)

#define MJOBSISACTIVE(State) \
  (((State == mjsStarting) || (State == mjsRunning)) ? TRUE: FALSE)

#define MJOBSISBLOCKED(State) \
  ((((State == mjsHold) || \
    (State == mjsBatchHold) || \
    (State == mjsSystemHold) || \
    (State == mjsUserHold) || \
    (State == mjsBlocked) || \
    (State == mjsDeferred))) ? TRUE : FALSE)

#define MJOBISALLOC(J) \
  ((((J)->State == mjsStarting) || ((J)->State == mjsRunning) || \
    ((J)->State == mjsSuspended) || ((J)->State == mjsStaging)) ? TRUE : FALSE)

#define MJOBWASCANCELED(J) \
  (((bmisset(&J->IFlags,mjifWasCanceled)) || ((J)->CancelReqTime > 0)) ? TRUE : FALSE)

#define MJOBISSUSPEND(J) \
   (((J)->State == mjsSuspended) ? TRUE : FALSE)

#define MJOBISCOMPLETE(J) \
  ((((J)->State == mjsCompleted) || ((J)->State == mjsRemoved) || \
    ((J)->State == mjsVacated)) ? TRUE : FALSE)

#define MSTATEISCOMPLETE(State) \
  ((((State) == mjsCompleted) || ((State) == mjsRemoved) || \
    ((State) == mjsVacated)) ? TRUE : FALSE)

#define MJOBISSCOMPLETE(J) \
  ((((J)->State == mjsCompleted)) ? TRUE : FALSE)

#define MJOBISFCOMPLETE(J) \
  ((((J)->State == mjsRemoved) || ((J)->State == mjsVacated)) ? TRUE : FALSE)

#define MJOBISIDLE(J) \
  ((((J)->State == mjsIdle) || ((J)->State == mjsHold) || ((J)->State == mjsNotRun) || \
    ((J)->State == mjsNONE) || ((J)->State == mjsNotQueued)) ? TRUE : FALSE)

#define MJOBSISIDLE(State) \
  (((State == mjsIdle) || (State == mjsHold) || (State == mjsNotRun) || \
    (State == mjsNONE) || (State == mjsNotQueued)) ? TRUE : FALSE)

/* NOTE: this will return false for on-demand vm create jobs or for VMUsagePolicy=any */
#define MJOBREQUIRESVMS(J) \
    (((MSched.ManageTransparentVMs == TRUE) && \
           ((J)->VMUsagePolicy == mvmupNONE) && \
           ((J)->VMUsagePolicy != mvmupAny) && \
           ((J)->System == NULL)) || \
           ((J)->VMUsagePolicy == mvmupVMOnly)) 

#define MJOBISVMCREATEJOB(J) \
    (/*(MJobGetSystemType(J) == msjtVMCreate) || */\
    ((J)->VMUsagePolicy == mvmupVMCreate))

#define MJOBISINRESERVEALWAYSQOS(J) \
    (((J)->Credential.Q != NULL) && bmisset(&(J)->Credential.Q->Flags,mqfReserved))


/** 
 * MJOBWASMIGRATEDTOREMOTEMOAB is used to prevent the moab that migrated the
 * job to a remote moab from scheduling the job and counting the migrated job
 * against the moab's (the one that migrated the job) active policies.
 *
 * The following are the two scenarios of jobs and their migration paths.
 *
 * 1: Job was submit to c1 and migrated to c2.
 *    c1 shows job as blocked/migrated and an SRM of internal and DRM of c2
 *    c2 shows job as idle and no DRM or SRM until job is running.
 *    Both sides show systemid and jid
 *    Job can't come back to c1 because job "violates hop count" partition c1 
 *      and is not added into the jobs' CPAL which is checked in MQueueScheduleIJobs.
 *
 * 2: Job was submited c1 and stays on c1.
 *    c1 shows job as idle
 *    c1 shows systemid and systemjid
 *    c2 doesn't show systemid and systemjid
 *    c2 shows job as blocked with SRM and DRM of c1 with CLUSTERLOCK and NORMSTART flags.
 *    
 * non-slave peers have the autostart flag.
 */

#define MJOBWASMIGRATEDTOREMOTEMOAB(J) \
    ((((J) != NULL) && \
      ((J)->SubmitRM != NULL) && \
      ((J)->SubmitRM->Type != mrmtMoab) && \
      ((J)->DestinationRM != NULL) && \
      ((J)->DestinationRM->Type == mrmtMoab) && \
      (bmisset(&(J)->DestinationRM->Flags,mrmfAutoStart))) ? TRUE : FALSE) 

/**
 * Determin if rsv is a ghost reservation or not. 
 */

#define MRSVISGHOSTRSV(Rsv) \
  ((((Rsv)->J != NULL) && (bmisset(&(Rsv)->J->Flags,mjfIgnIdleJobRsv))) ? TRUE : FALSE)

/* bitmap processing */

#define MOLDISSET(B,F) \
  (((B) & (1 << (F))) ? TRUE : FALSE)

#define MOLDSET(B,F) \
  (B) |= (1 << (F))

#define MOLDUNSET(B,F) \
  (B) &= ~(1 << (F))

#define MPRINTNULL(STR) \
  ((STR) != NULL) ? (STR) : "NULL"

/* if DST is 0, then we want to assign its value to be SRC */
#define MASSIGNIFZERO(DST,SRC) \
  if ((DST) == 0) (DST) = (SRC)

/* if DST is 0, then we want to assign its value to be SRC */
#define MASSIGNIFZERO(DST,SRC) \
  if ((DST) == 0) (DST) = (SRC)

/* hashtable processing */

#define MHASHSIZE(n) ((uint32_t)1<<(n))
#define MHASHMASK(n) (MHASHSIZE(n)-1)

#include "moab-wiki.h"

/* Include structures for VM automatic migration */
#include "MVMMigration.h"

#ifdef __MX
#include "mg2.h"
#endif /* !__MX */

#ifdef __M_H

#define MAM_CLIENTTYPE  "moab"

#endif /* __M_H */

#define COND_FAIL(cond) if(cond) return FAILURE;
  /* Propogate a failure */
#define PROP_FAIL(retval) COND_FAIL((retval) == FAILURE);

#define jlog(Level,J,Format,args...) MLogJob(Level,__FILE__,__LINE__,J,Format,## args)


#endif /* __MOAB_H */

/* END moab.h */
