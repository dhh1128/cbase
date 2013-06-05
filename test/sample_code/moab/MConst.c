/* HEADER */

/**
 * @file src/moab/MConst.c
 *
 * Moab Constants
 */

#include "moab.h"

/* repository for char arrays */

/* sync w/FALSE, TRUE, MBNOTSET */

const char *MBool[] = {
  "FALSE",
  "TRUE",
  "NOTSET",
  NULL };

/* boolean tracking (use strcasecmp to catch all capitalization permutations) */

const char *MTrueString[] = {
  "1",
  "true",
  "on",
  "yes",
  NULL };

const char *MFalseString[] = {
  "0",
  "false",
  "off",
  "no",
  NULL };


/* used in parameter diagnostics only (use strcasecmp to catch all capitalization permutations) */

const char *MBoolString[] = {
  "1",
  "true",
  "on",
  "yes",
  "0",
  "false",
  "off",
  "no",
  NULL };


/* sync w/enum MCompEnum */
        
const char *MComp[] = {
  "NC",
  "<",
  "<=",
  "==",
  ">=",
  ">",
  "<>",
  "=",
  "!=",
  "%<",
  "%!",
  "%=",
  "+=",
  "-=",
  NULL };

/* peers - MComp[] 
 * sync with MCompEnum 
 * NOTE: incompleteness is OK for now, see MUCmpFromString2() */
const char *MCompAlpha[] = {
  "NC",
  "LT",
  "LE",
  "EQ",
  "GE",
  "GT",
  "NE",
  "EQ",
  "NE",
  NULL };

/*Database Type. Sync with MDBTypeEnum */
const char *MDBType[] =  {
  "NONE",
  "INTERNAL",
  "ODBC",
  NULL}; 

/* sync w/MAMOTable[] in MJob.c */

const char *MAMOCmp[] = {
  NONE,
  "lt",
  "le",
  "eq",
  "ge",
  "gt",
  "ne",
  NULL };

/* sync w/enum MClientKeywordEnum */
 
const char *MCKeyword[] = {
  NONE,
  "SC=",
  "ARG=",
  "AUTH=",
  "CMD=",
  "DT=",
  "CLIENT=",
  "TS=",
  "CK=",
  NULL };

/* sync w/enum MNodeMatchPolicyEnum */

const char *MJobNodeMatchType[] = { 
  NONE, 
  "AUTO",
  "EXACTNODE", 
  "EXACTPROC", 
  "EXACTTASK",
  NULL };


/* sync w/enum MPolicyTypeEnum */

const char *MPolicyMode[] = { 
  "DEFAULT", 
  "OFF", 
  "ON", 
  "SOFT", 
  "HARD", 
  "EXEMPT", 
  "QUEUE",
  NULL };


const char *MJVarName[] = {
  "HOSTLIST",
  "JOBID",
  "MASTERHOST",
  "NODECOUNT",
  "OWNER",
  NULL };


const char *NullP = "NULL";

/* sync w/enum MSimJobSubmissionPolicyEnum */

const char *MSimQPolicy[]  = { 
  NONE,
  "NORMAL", 
  "CONSTANTJOBDEPTH", 
  NULL };


/* sync w/XXX */

const char *MSimNCPolicy[] = { 
  "NORMAL", 
  "UNIFORM", 
  "PARTITIONED", 
  NULL };


/* sync w/enum MBFPolicyEnum */

const char *MBFPolicy[] = { 
  "NONE", 
  "BESTFIT",
  "FIRSTFIT",
  "GREEDY",
  "PREEMPT", 
  NULL };


/* sync w/enum MBlockListEnum */

const char *MBlockList[] = {
  "NONE",
  "DEPEND",
  NULL };


const char *MStatusCode[] = { 
  "FAILURE", 
  "SUCCESS", 
  NULL };


/* sync w/enum MAMTypeEnum */

const char *MAMType[] = { 
  NONE, 
  "FILE",
  "GOLD",
  "native",
  "MAM",
  NULL };


/* sync w/enum MRMStateEnum */

const char *MAMState[] = { 
  NONE, 
  "Active", 
  "Configured",
  "Corrupt", 
  "Disabled",
  "Down",
  NULL };     

const char *MAMProtocol[] = { NONE, "XML", "KVP", NULL };


/* sync w/enum MAMConsumptionPolicyEnum */

const char *MAMChargeMetricPolicy[] = {
  NONE,
  "DEBITALLWC",
  "DEBITALLCPU",
  "DEBITALLPE",
  "DEBITALLBLOCKED",
  "DEBITSUCCESSFULWC",
  "DEBITSUCCESSFULCPU",
  "DEBITSUCCESSFULNODE",
  "DEBITSUCCESSFULPE",
  "DEBITSUCCESSFULBLOCKED",
  "DEBITREQUESTEDWC",
  NULL };


/* sync w/MAMConsumptionRatePolicyEnum */

const char *MAMChargeRatePolicy[] = {
  NONE,
  "QOSDEL",
  "QOSREQ",
  NULL };


/* sync w/enum MRMStateEnum */

const char *MRMState[] = { 
  NONE, 
  "Active", 
  "Configured",
  "Corrupt",
  "Disabled",
  "Down", 
  NULL };


/* sync w/MRMNodeStatePolicyEnum */

const char *MRMNodeStatePolicy[] = {
  NONE,
  "OPTIMISTIC",
  "PESSIMISTIC",
  NULL };


/* sync w/enum MRMTypeEnum */

const char *MRMType[] = { 
  NONE,
  "LL",
  "OTHER",
  "PBS",
  "SSS",
  "WIKI",
  "NATIVE",
  "MOAB",
  "TORQUE",
  NULL };


/* sync w/enum MRMSubTypeEnum */

const char *MRMSubType[] = { 
  NONE, 
  "AGFULL",
  "LSF",
  "CCS",
  "MSM",
  "SGE",
  "SLURM",
  "X1E",
  "XT3",
  "XT4",
  NULL };        


/**
 * @see enum MRMResourceTypeEnum  (sync)
 * @see mrm_t.RTypes
 * @see RMCFG[] RESOURCETYPE=<X>
 */

const char *MRMResourceType[] = {
  NONE,
  "COMPUTE",
  "INFO",
  "LICENSE",
  "NETWORK",
  "PROV",
  "STORAGE",
  "ANY",
  NULL };



/* sync w/enum MSystemJobTypeEnum */

const char *MSysJobType[] = {
  NONE,
  "generic",
  "osprovision",
  "osprovision2",
  "poweroff",
  "poweron",
  "reset",
  "storage",
/*  "vmcreate",
  "vmdestroy",*/
  "vmmap",
  "vmmigrate",
/*  "vmstorage",*/
  "vmtracking",
  NULL };



/* sync w/enum MJobCompletionPolicyEnum */

const char *MJobCompletionPolicy[] = {
  NONE,
  "generic",
  "osprovision",
  "osprovision2",
  "oswait",
  "poweroff",
  "poweron",
  "reset",
  "storage",
  "vmcreate",
  "vmdestroy",
  "vmmap",
  "vmmigrate",
  "vmstorage",
  "vmtracking",
  NULL };


/* sync w/enum MSystemJobCompletionActionEnum  */

const char *MSystemJobCompletionAction[] = {
  NONE,
  "JobReserve",
  "JobStart",
  NULL };

/* sync w/enum MJobSysAttrEnum */

const char *MJobSysAttr[] = {
   NONE,
  "Action",
  "CFlags",
  "CompletionPolicy",
  "Dependents",
  "FAction",
  "ICPVar1",
  "JobToStart",
  "JobType",
  "RetryCount",
  "SAction",
  "SCPVar1",
  "ProxyUser",
  "VM",
  "VMDestinationNode",
  "VMSourceNode",
  "VMToMigrate",
  "WCLimit",
  NULL };


/* sync w/enum MRMResListType */

const char *MRMResList[] = {
  NONE,
  "arch",
  "cput",
  "file",
  "flags",
  "host",    /* NYI */
  "mem",
  "ncpus",
  "nice",     /* NYI */
  "nodes",
  "procs",
  "opsys",
  "os",
  "pcput",    /* NYI */
  "placement",
  "pmem",     /* NYI */
  "pvmem",    /* NYI */
  "software", 
  "trl",
  "vmem",
  "walltime",
  "vmusage",
  NULL };




/* sync w/enum MRMFuncEnum (MAX=30 - more allowed if not controlled by bitmap) */

const char *MRMFuncType[]  = {
  NONE,
  "clusterquery",
  "credctl",
  "infoquery",
  "jobcancel",
  "jobcheckpoint",
  "jobgetproximatetasks",
  "jobmigrate",
  "jobmodify",
  "jobrequeue",
  "jobresume",
  "jobsetattr",
  "jobstart",
  "jobsubmit",
  "jobsuspend",
  "nodemodify",
  "ping",
  "queuecreate",
  "queuemodify",
  "queuequery",
  "reset",
  "resourcemodify",
  "resourcequery",
  "rsvctl",
  "rmeventquery",
  "rminitialize",
  "rmquery",
  "rmstart",
  "rmstop",
  "systemmodify",
  "systemquery",
  "workloadquery",
  "alloc",           /* this line and below not controlled by bitmap */
  "credctl",
  "cyclefinalize",
  "dataisloaded",
  "getdata",
  "jobsignal",
  "jobvalidate",
  "nodemigrate",
  "nodepower",
  "nodevirtualize",
  "parcreate",
  "pardestroy",
  "resourcecreate",
  "rmfailure",
  NULL };


/* sync w/MREEnum */

const char *MREType[] = { NONE, "start", "end", "instant", NULL };


/* sync w/MCLogicEnum */
 
const char *MCLogic[] = { "AND", "OR", "NOT", NULL };


/* sync w/enum MSimFlagEnum */

const char *MSimFlagType[] = { 
  NONE, 
  "IGNHOSTLIST", 
  "IGNCLASS", 
  "IGNQOS", 
  "IGNMODE", 
  "IGNFEATURES", 
  "IGNGRES",
  "IGNRACK",
  "IGNALL", 
  NULL };  


/* sync w/enum MTrigTypeEnum */

const char *MTrigType[] = {
  NONE,
  "cancel",
  "checkpoint",
  "create",
  "discover",
  "end",
  "epoch",
  "fail",
  "hold",
  "logroll",
  "migrate",
  "modify",
  "preempt",
  "standing",
  "start",
  "threshold",
  NULL };


/* sync w/enum MTrigActionTypeEnum */
 
const char *MTrigActionType[] = {
  NONE,
  "cancel",
  "changeparam",
  "exec",
  "internal",
  "jobpreempt",
  "mail",
  "modify",
  "query",
  "reserve",
  "submit",
  NULL };


/* sync w/MTrigTresholdTypeEnum */

const char *MTrigThresholdType[] = {
  NONE,
  "Availability",
  "Backlog",
  "CPULoad",
  "GMetric",
  "QueueTime",
  "Statistic",
  "Usage",
  "XFactor",
  NULL };


/* sync w/enum MJobPrioAccrualPolicyEnum */
 
const char *MJobPrioAccrualPolicyType[] = { 
  NONE, 
  "ACCRUE",
  "ALWAYS", 
  "QUEUEPOLICYUSER",
  "QUEUEPOLICY", 
  "QUEUEPOLICYRESET",
  "FULLPOLICY", 
  "FULLPOLICYRESET",
  "RESET",
  NULL };


/* sync w/enum MJobPrioExceptionEnum */

const char *MJobPrioException[] = {
  NONE,
  "BATCHHOLD",
  "DEFER",
  "DEPENDS",
  "HARDPOLICY",
  "IDLEPOLICY",
  "SOFTPOLICY",
  "SYSTEMHOLD",
  "USERHOLD",
  NULL };


/* sync w/enum MActivityEnum[] */

const char *MActivity[] = {
  NONE,
  "JobStart",
  "JobSuspend",
  "JobResume",
  "JobCancel",
  "JobMigrate",
  "RMResourceLoad",
  "RMResourceProcess",
  "RMResourceModify",
  "RMWorkloadLoad",
  "RMWorkloadProcess",
  "Schedule",
  "Trigger",
  "UIProcess",
  "UIWait",
  NULL };



/* sync w/enum MBFPriorityEnum */ 

const char *MBFPriorityPolicyType[] = { 
  NONE, 
  "RANDOM", 
  "DURATION", 
  "HWDURATION", 
  NULL };


/* sync w/enum MResLimitPolicyEnum */

const char *MResourceLimitPolicyType[] = { 
  NONE, 
  "ALWAYS", 
  "EXTENDEDVIOLATION", 
  "BLOCKEDWORKLOADONLY", 
  "NEVER",
  NULL };


/* sync w/enum MResLimitVActionEnum */

const char *MPolicyAction[] = { 
  NONE, 
  "CANCEL", 
  "CHECKPOINT",
  "NOTIFY",
  "PREEMPT",
  "REQUEUE", 
  "SUSPEND", 
  "USER",
  NULL };


/* sync w/enum MNodeLoadPolicyEnum */

const char *MNodeLoadPolicyType[] = { 
  NONE, 
  "ADJUSTSTATE", 
  "ADJUSTPROCS", 
  NULL };


/* sync w/enum MFSPolicyEnum */

const char *MFSPolicyType[] = { 
  NONE, 
  "DEDICATEDPS", 
  "DEDICATEDPES", 
  "UTILIZEDPS", 
  "SDEDICATEDPES",
  "PDEDICATEDPS",
  NULL };


/* sync w/enum MFSTargetEnum */

#ifdef __cplusplus
extern "C"
#endif
const char MFSTargetTypeMod[] = {
  ' ',
  ' ',
  '+',
  '-',
  '%',
  '^',
  '\0' };



/* sync w/enum MCalPeriodEnum */

const char *MCalPeriodType[] = { 
  NONE, 
  "MINUTE",
  "HOUR",
  "DAY", 
  "WEEK", 
  "MONTH",
  "INFINITY", 
  NULL };


/* sync w/enum MCalPeriodEnum */

#ifdef __cplusplus
extern "C"
#endif
const mulong MCalPeriodDuration[] = {
  0,
  MCONST_MINUTELEN,
  MCONST_HOURLEN,
  MCONST_DAYLEN,
  MCONST_WEEKLEN,
  MCONST_MONTHLEN,
  MMAX_TIME,
  MMAX_TIME };


/* sync w/enum MActivePolicyTypeEnum */
/* sync w/ MIdlePolicyType[] */

const char *MPolicyType[] = { 
  NONE, 
  "MAXJOB", 
  "MAXNODE", 
  "MAXPE", 
  "MAXPROC", 
  "MAXPS", 
  "MAXWC", 
  "MAXMEM", 
  "MINPROC",
  "MAXGRES",
  "MAXARRAYJOB",
  NULL };

/* sync w/enum MActivePolicyTypeEnum */
/* sync w/MPolicyType[] */

const char *MIdlePolicyType[] = { 
  NONE, 
  "MAXIJOB", 
  "MAXINODE", 
  "MAXIPE", 
  "MAXIPROC", 
  "MAXIPS", 
  "MAXIWC", 
  "MAXIMEM", 
  "MINIPROC",
  "MAXIGRES",
  "MAXIARRAYJOB",
  NULL };



/* resource selection policy */

const char *MResSelType[] = { 
  "NONE",
  "NORMAL",
  "BESTEFFORT",
  "FORCE", 
  NULL };


/* sync w/enum MVWConflictPolicyEnum */

const char *MVWConflictPolicy[] = {
  NONE,
  "PREEMPT",
  NULL };


/* sync w/enum MJobCtlCmdEnum */

const char *MJobCtlCmds[] = {
  NONE,
  "cancel",
  "checkpoint",
  "complete",
  "create",
  "delete",
  "diagnose",
  "forcecancel",
  "join",
  "migrate",
  "modify",
  "query",
  "requeue",
  "rerun",
  "resume",
  "show",
  "signal",
  "start",
  "submit",
  "suspend",
  "terminate",
  "adjusthold",
  "adjustprio",
  "check",
  NULL };


/* sync w/enum MJobStateTypeEnum */

const char *MJobStateType[] = {
  NONE,
  "active",
  "blocked",
  "completed",
  "eligible",
  NULL };


/* sync w/enum MJobSubStateEnum */

const char *MJobSubState[] = {
  NONE,
  "Epilogue",
  "Migrated",
  "Preempted",
  "Prologue",
  NULL };
 

/* sync w/enum MRMCtlCmdEnum */

const char *MRMCtlCmds[] = {
  NONE,
  "clear",
  "create",
  "destroy",
  "failure",
  "init",
  "kill",
  "list",
  "modify",
  "peercopy",
  "ping",
  "purge",
  "query",
  "reconfig",
  "resume",
  "stop",
  "start",
  NULL };


/* sync w/enum MDeadlinePolicy */

const char *MDeadlinePolicy[] = {
  NONE,
  "CANCEL",
  "ESCALATE",
  "HOLD",
  "IGNORE",
  "RETRY",
  NULL };


/* sync w/enum MMissingDependencyActionEnum */

const char *MMissingDependencyAction[] = {
  NONE,
  "ASSUMESUCCESS",
  "CANCEL",
  "HOLD",
  "RUN",
  NULL };

 
/* sync w/enum MSchedCtlCmdEnum */

const char *MSchedCtlCmds[] = {
  NONE,
  "copyfile",
  "create",  
  "destroy",
  "event",
  "failure",
  "flush",
  "init",
  "kill",
  "list",
  "log",
  "modify",
  "pause",
  "squery",            /* HACK: prevent mschedctl -q commands from being routed to MUICredCtl() */
  "reconfig",
  "resume",
  "step",
  "stop",
  "vmmigrate",
  NULL };

/*sync w/enum MRsvShowModeEnum */

const char *MRsvShowMode[] = {
  NONE,
  "overlap",
  "free",
  NULL };

/* sync w/enum MRsvCtlCmdEnum */

const char *MRsvCtlCmds[] = {
  NONE,
  "alloc",
  "create",
  "destroy",
  "flush",
  "join",
  "list",
  "migrate",
  "modify",
  "query",
  "other",
  "signal",
  "submit",
  "show",
  NULL };

const char *MBalCmds[] = {
  NONE,
  "execute",
  "query",
  NULL };


/* sync w/enum MGlobalCredEnum */

const char *MGCredType[] = {
  NONE,
  "UNIX",
  "GLOBUS",
  NULL };


/* sync w/enum MPeerEventTypeEnum */

const char *MPeerEventType[] = {
  NONE,
  "CLASSCHANGE",
  NULL };


/* sync w/enum MCredCtlCmdEnum */

const char *MCredCtlCmds[] = {
  NONE,
  "create",
  "destroy",
  "list",
  "modify",
  "query",
  "reset",
  NULL };


/* sync w/enum MShowCmdEnum */

const char *MShowCmds[] = {
  NONE,
  "list",
  "query",
  NULL };


/* sync w/enum MCModeEnum */

const char *MClientMode[] = {
  "NONE",
  "BLOCK",
  "CLEAR",
  "COMPLETE",
  "CP",
  "EXCLUSIVE",
  "FORCE",
  "FORCE2",
  "FUTURE",
  "GREP",
  "JOBNAME",
  "LOCAL",
  "NONBLOCK",
  "OVERLAP",
  "PARSE",
  "PEER",
  "PERSISTENT",
  "POLICY",
  "RELATIVE",
  "SORT",
  "SUMMARY",
  "TIMEFLEX",
  "USER",
  "TID",
  "VERBOSE",
  "VERBOSE2",
  "XML",
  NULL };



const char *MALType[] = { 
  "", 
  "&", 
  "^", 
  NULL };


/* sync w/enum MDisplayFlagEnum */

const char *MCDisplayType[] = { 
  NONE,
  "ACCOUNTCENTRIC",
  "CONDENSED",
  "ENABLEPOSPRIO",
  "HIDEBLOCKED",
  "HIDEOUTPUT",
  "LOCAL", 
  "NODECENTRIC", 
  "REPORTEDNODESONLY", 
  "SERVICEPROVISIONING",
  "SHOWSYSTEMJOBS",
  "SUMMARY",
  "TEST",
  "VERBOSE",
  "USEBLOCKING",
  "HIDEDRAINED",
  NULL };


/* sync w/MRsvFlagEnum */

const char *MRsvFlags[] = { 
  NONE,
  "ADVRESJOBDESTROY",
  "APPLYPROFRESOURCES",
  "STANDINGRSV", 
  "SINGLEUSE", 
  "SYSTEMJOB",
  "BYNAME", 
  "PREEMPTEE",
  "TIMEFLEX",
  "SPACEFLEX",
  "DEDICATEDNODE",
  "DEDICATEDRESOURCE",
  "ENFORCENODESET",
  "EVACVMS",
  "EXCLUDEJOBS",
  "EXCLUDEALLBUTSB",
  "EXCLUDEMYGROUP",
  "ADVRES",
  "IGNIDLEJOBS",
  "IGNJOBRSV",
  "IGNRSV",
  "IGNSTATE",
  "OWNERPREEMPT",
  "OWNERPREEMPTIGNOREMINTIME",
  "PARENTLOCK",
  "PRSV",
  "ALLOWPRSV",
  "ALLOWGRID",
  "ALLOWJOBOVERLAP",
  "ACLOVERLAP",
  "VPC",
  "ISCLOSED",
  "ISGLOBAL",
  "ISACTIVE",
  "WASACTIVE",
  "REQFULL",
  "STATIC",
  "TRIGHASFIRED",
  "ENDTRIGHASFIRED",
  "DEADLINE",
  "NOCHARGE",
  "PROVISION",
  "NOVMMIGRATIONS",
  "PLACEHOLDER",
  NULL };


/* sync w/enum MNotifyEnum */

const char *MNotifyType[] = {
  NONE,
  "JobStart",
  "JobEnd",
  "JobFail",
  "All",
  NULL };

/* sync w/enum MEventLogEntityEnum */

const char *MEventLogEntityType[] = {
  NONE,
  "Event",
  "PrimaryObject",
  "RelatedObject",
  "RelatedObjects",
  "SerializedObject",
  "InitiatedBy",
  "ErrorMessage",
  "Detail",
  "Details",
  NULL };

/* sync w/enum MEventLogAttributeEnum */

const char *MEventLogAttributeType[] = {
  NONE,
  "eventTime",
  "eventType",
  "eventCategory",
  "sourceComponent",
  "status",
  "facility",
  "objectType",
  "objectID",
  "initiatedByUser",
  "proxyUser",
  "originator",
  "errorCode",
  "message",
  "name",
  NULL };


/* sync w/enum MRecordEventTypeEnum */

/*
 * WARNING!!!!  Any changes here have to be synced with the database.  These are used for lookups in the 
 *              database so don't make them alphabetical, just append to the end to not invalidate the database
 */

const char *MRecordEventType[] = {
  NONE,
  "GEVENT",
  "JOBCANCEL",
  "JOBEND",
  "JOBFAILURE",
  "JOBHOLD",
  "JOBMIGRATE",
  "JOBMODIFY",
  "JOBPREEMPT",
  "JOBREJECT",
  "JOBRESUME",
  "JOBSTART",
  "JOBSUBMIT",
  "JOBUPDATE",
  "JOBVARSET",
  "JOBVARUNSET",
  "NODEDOWN",
  "NODEFAILURE",
  "NODEUP",
  "NODEUPDATE",
  "NOTE",
  "QOSVIOLATION",
  "RMDOWN",
  "RMUP",
  "RSVCANCEL",
  "RSVCREATE",
  "RSVEND",
  "RSVMODIFY",
  "RSVSTART",
  "SCHEDCOMMAND",
  "SCHEDFAILURE",
  "SCHEDMODIFY",
  "SCHEDPAUSE",
  "SCHEDRECYCLE",
  "SCHEDRESUME",
  "SCHEDSTART",
  "SCHEDSTOP",
  "TRIGCREATE",
  "TRIGEND",
  "TRIGFAILURE",
  "TRIGSTART",
  "TRIGTHRESHOLD",
  "VMCREATE",
  "VMDESTROY",
  "VMMIGRATE",
  "VMPOWERON",
  "VMPOWEROFF",
  "NODEMODIFY",
  "NODEPOWEROFF",
  "NODEPOWERON",
  "NODEPROVISION",
  "ALLSCHEDCOMMAND",
  "AMDELETE",
  "AMEND",
  "AMQUOTE",
  "AMSTART",
  "RMPOLLEND",
  "RMPOLLSTART",
  "SCHEDCYCLEEND",
  "SCHEDCYCLESTART",
  "JOBCHECKPOINT",
  "NODEDESTROY",
  "AMCREATE",
  "AMUPDATE",
  "AMPAUSE",
  "AMRESUME",
  "AMMODIFY",
  "CLIENTCOMMAND",
  NULL };

/* sync w/enum MEventLogTypeEnum */

const char *MEventLogType[] = {
  NONE,
  "jobcancel",
  "jobend",
  "jobhold",
  "jobmodify",
  "jobreject",
  "jobrelease",
  "jobstart",
  "jobsubmit",
  "rsvcreate",
  "rsvend",
  "rsvstart",
  "schedcommand",
  "schedcycleend",
  "schedcyclestart",
  "schedpause",
  "schedrecycle",
  "schedresume",
  "schedstart",
  "schedstop",
  "trigcreate",
  "trigend",
  "trigstart",
  "vmcancel",
  "vmdestroy",
  "vmend",
  "vmmigrateend",
  "vmmigratestart",
  "vmready",
  "vmsubmit",
  NULL };


/* sync w/enum MEventLogFacilityEnum */

const char *MEventLogFacility[] = {
  NONE,
  "job",
  "reservation",
  "scheduler",
  "trigger",
  "vm",
  NULL };


/* sync w/enum MEventLogCategoryEnum */

const char *MEventLogCategory[] = {
  NONE,
  "cancel",
  "command",
  "create",
  "destroy",
  "end",
  "hold",
  "migrate",
  "modify",
  "pause",
  "ready",
  "recycle",
  "reject",
  "release",
  "resume",
  "start",
  "stop",
  "submit",
  NULL };

/* sync w/enum MVPCPFlagEnum */

const char *MVPCPFlags[] = {
  NONE,
  "autoaccess",
  NULL };



/**
 * Virtual Private Cluster
 *
 * @see enum MVPCPAttrEnum (sync)
 */

const char *MVPCPAttr[] = {
  NONE,
  "ACL",
  "COALLOC",
  "DESCRIPTION",
  "FLAGS",
  "MINDURATION",
  "NAME",
  "NODEHOURCHARGERATE",
  "NODESETUPCOST",
  "OPRSVPROFILE",
  "OPTIONALVARS",
  "PRIORITY",
  "PURGETIME",
  "QUERYDATA",
  "REALLOCPOLICY",
  "REQDEFATTR",
  "REQSETATTR",
  "REQENDPAD",
  "REQSTARTPAD",
  "SETUPCOST",
  NULL };



/* job reservation */

/* sync w/enum MJRsvSubTypeEnum */

const char *MJRType[] = { 
  NONE, 
  "ActiveJob",
  "Deadline",
  "Meta",
  "Priority", 
  "QOSReserved", 
  "Retry",
  "Timelock",
  "User", 
  NULL };


/* sync w/enum MRsvTypeEnum */

const char *MRsvType[] = { 
  "NONE",
  "Job", 
  "User",
  "Meta",
  NULL };

/* sync w/enum MRsvSubTypeEnum and MRsvSSubType[] (also sync w/const char *MNodeCat[]) */

const char *MRsvSubType[] = {
  NONE,
  "Active",
  "BatchFailure",
  "Benchmark",
  "EmergencyMaintenance",
  "GridReservation",
  "HardwareFailure",
  "HardwareMaintenance",
  "Idle",
  "JobReservation",
  "NetworkFailure",
  "Other",
  "OtherFailure",
  "PersonalReservation",
  "Site1",
  "Site2",
  "Site3",
  "Site4",
  "Site5",
  "Site6",
  "Site7",
  "Site8",
  "SoftwareFailure",
  "SoftwareMaintenance",
  "StandingReservation",
  "StorageFailure",
  "UserReservation",
  "VPC",
  "Meta",
  NULL };



/* Rsv 'short' subtype names - sync w/enum MRsvSubTypeEnum and MRsvSubType[] */

const char *MRsvSSubType[] = {
  NONE,
  "active",
  "batchfail",
  "bench",
  "emain",
  "grid",
  "hwfail",
  "hwmain",
  "idle",
  "jobrsv",
  "netfail",
  "other",
  "otherfail",
  "prsv",
  "site1",
  "site2",
  "site3",
  "site4",
  "site5",
  "site6",
  "site7",
  "site8",
  "swfail",
  "swmain",
  "sr",
  "storagefail",
  "user",
  "vpc",
  NULL };


/* sync w/enum MNodeAccessPolicyEnum */

/* NOTE:  These must remain ordered by precedenc, least restrictive to most
          restrictive. */

const char *MNAccessPolicy[] = {
  NONE,
  "SHARED",
  "SHAREDONLY",
  "UNIQUEUSER",
  "SINGLEUSER",
  "SINGLEJOB",
  "SINGLETASK",
  "SINGLEGROUP",
  "SINGLEACCOUNT",
  NULL };


/* NOTE:  old list was: active,down-batch,down-hw,down-net,down-other,down-storage,down-sw,idle,rsv-puser,rsv-sysbench,rsv-sysmaint,rsv-sysumaint,rsv-user,site1,site2,site3,site4,site5,site6,site7,site8 */

/* NOTE:  should sync w/MRsvSubType[] */

/* sync w/enum MNodeCatEnum */

const char *MNodeCat[] = {
  NONE,
  "Active",
  "BatchFailure",
  "Benchmark",
  "EmergencyMaintenance",
  "GridReservation",
  "HardwareFailure",
  "HardwareMaintenance",
  "Idle",
  "JobReservation",
  "NetworkFailure",
  "Other",
  "OtherFailure",
  "PersonalReservation",
  "Site1",
  "Site2",
  "Site3",
  "Site4",
  "Site5",
  "Site6",
  "Site7",
  "Site8",
  "SoftwareFailure",
  "SoftwareMaintenance",
  "StandingReservation",
  "StorageFailure",
  "UserReservation",
  "VPC",
  NULL };


/* scheduler phases (sync w/enum MSchedPhaseEnum) */

const char *MSchedPhase[] = {
  NONE,
  "idle",
#ifdef MIDLE
  "measuredidle", 
#endif /* MIDLE */
  "rmaction",
  "rmload",
  "rmprocess",
  "sched",
  "total",
  "triggers",
  "ui",
  NULL };


/* NOTE: sync w/enum MExpAttrEnum */

const char *MAttrType[] = {
  "Feature",
  "Opsys",
  "Arch",
  "GRes",
  "JFeature",
  "GEvent",
  "GMetric",
  NULL };


/* sync w/enum MNetNameEnum */

const char *MNetName[] = {
  NONE,
  "ethernet",
  "hps_ip",
  "hps_us",
  NULL };


/* sync w/enum MJobStartFlagsEnum */

const char *MJobStartFlags[] = {
  NONE,
  "ignorenodestate",
  "ignorersv",
  "ignorepolicies",
  NULL };


/* sync w/enum MJobSubmitFlagsEnum */

const char *MJobSubmitFlags[] = {
  NONE,
  "dsdependency",
  "dataonly",
  "globalqueue",
  "noeventlogs",
  "startnow",
  "syncjobid",
  "template",
  "usejobname",
  "templatedepend",
  NULL };


/* sync w/enum MResourceSetSelectionEnum */

const char *MResSetSelectionType[] = {
  NONE,
  "ONEOF",
  "ANYOF",
  "FIRSTOF",
  NULL };


/* sync w/enum MResourceSetTypeEnum */

const char *MResSetAttrType[] = {
  NONE,
  "CLASS",
  "FEATURE",
  NULL };


/* sync w/enum MResourceSetPrioEnum */

const char *MResSetPrioType[] = {
  NONE,
  "AFFINITY",
  "FIRSTFIT",
  "BESTFIT",
  "WORSTFIT",
  "BESTRESOURCE",
  "MINLOSS",
  NULL };


/* sync w/enum MNodeSetPlusPolicyEnum */

const char *MNodeSetPlusPolicy[] = {
  NONE,
  "SpanEvenly",
  "Delay",
  NULL };


const char *MBNFCommand[] = {
  NONE,
  "query",
  "status",
  "message",
  "set",
  NULL };



/* sync w/enum MDiagnoseCmdEnum */

const char *MDiagnoseCmds[] = {
  NONE,
  "Account",
  "BlockedJobs",
  "Class",
  "Config",
  "Event",
  "Fairshare",
  "Green",
  "Group",
  "Job",
  "Limit",
  "Node",
  "Priority",
  "Partition",
  "QOS",
  "RM",
  "Rsv",
  "SRsv",
  "Sched",
  "Trig",
  "User",
  NULL };

/* sync w/enum MNodeCtlCmdEnum */

const char *MNodeCtlCmds[] = {
  NONE,
  "check",
  "create",
  "destroy",
  "list",
  "migrate",
  "modify",
  "query",
  "reinitialize",
  NULL };

/* sync w/ enum MVCCtlCmdEnum */

const char *MVCCtlCmds[] = {
  NONE,
  "add",
  "create",
  "destroy",
  "modify",
  "query",
  "remove",
  "execute",
  NULL };

/* sync w/enum MVMCtlCmdEnum */

const char *MVMCtlCmds[] = {
  NONE,
/*  "create", */
  "destroy",
  "migrate",
  "modify",
  "query",
  "forcemigration",
  NULL };


/* sync w/enum MVMMigrateModeEnum */

const char *MVMMigrateMode[] = {
  NONE,
  "Overcommit",
  "Green",
  "Rsv",
  NULL };


/* sync w/enum MNodeQueryCmdEnum */

const char *MNodeQueryCmd[] = {
  NONE,
  "cat",
  "diag",
  "profile",
  "wiki",
  NULL };


/* sync w/enum MNodeActionEnum */

const char *MNodeActions[] = {
  NONE,
  "reset",
  "shutdown",
  "poweron",
  "provision",
  "scrub",
  NULL };


/* sync w/enum MJobQueryCmdEnum */

const char *MJobQueryCmd[] = {
  NONE,
  "estimatedStart",
  "reservedStart",
  "status",
  NULL };


/* sync w/enum MAttrOwnerEnum */

const char *MAttrOwner[] = {
  NONE,
  "APPLICATIONS",
  "BASE",
  "GRES",
  "LICENSES",
  "NETWORK",
  "OS",
  "STATE",
  "USAGE",
  "WORKLOAD",
  NULL };


/* sync w/enum MNodeFlagEnum */

const char *MNodeFlags[] = {
  NONE,
  "globalvars",
  "novmmigrations",
  NULL };


/* sync w/enum MNodeDestroyPolicyEnum */

const char *MNodeDestroyPolicy[] = {
  NONE,
  "ADMIN",
  "DRAIN",
  "HARD",
  "SOFT",
  NULL };


/* sync w/enum MPendingActionAttrEnum */

const char *MPendingActionAttr[] = {
  NONE,
  "description",
  "hostlist",
  "maxduration",
  "migrationsource",
  "migrationdestination",
  "motivation",
  "pactionid",
  "requester",
  "sched",
  "starttime",
  "state",
  "substate",
  "targetos",
  "type",
  "variables",
  "vcvariables",
  "vmid",
  NULL };

/* sync w/enum MPendingActionTypeEnum */

const char *MPendingActionType[] = {
  NONE,
  "vmosprovision",
  "vmpoweroff",
  "vmpoweron",
  NULL };

/**
 * VM object
 *
 * @see enum MVMAttrEnum (sync)
 *  NOTE: don't need to sync with MNodeAttrEnum anymore because mvmctl is 
 *        decoupled from mnodectl
 * @see enum MNodeAttrEnum (sync string names where possible)
 */

const char *MVMAttr[] = {
  NONE,
  "OS",
  "RADISK",
  "ALIAS",
  "RAMEM",
  "RAPROC",
  "RCSWAP",
  "RCDISK",
  "RCMEM",
  "RCPROC",
  "CPULOAD",
  "CONTAINERNODE",
  "RDDISK",
  "RDMEM",
  "RDPROC",
  "DESCRIPTION",
  "EFFECTIVETTL",
  "FLAGS",
  "GEVENT",
  "GMETRIC",
  "VMID",
  "JOBID",
  "LASTMIGRATETIME",
  "LASTSUBSTATE",
  "LASTSUBSTATEMTIME",
  "LASTUPDATETIME",
  "MIGRATECOUNT",
  "NETADDR",
  "NEXTOS",
  "NODETEMPLATE",
  "OSLIST",
  "POWERISENABLED",
  "POWERSTATE",
  "POWERSELECTSTATE",
  "PROVDATA",
  "RACK",
  "SLOT",
  "SOVEREIGN",
  "SPECIFIEDTTL",
  "STARTTIME",
  "STATE",
  "SUBSTATE",
  "STORAGERSVNAMES",
  "TRACKINGJOB",
  "TRIGGER",
  "CLASS",
  "VARIABLE",
  NULL };

/* sync w/ enum MVMFlagEnum */

const char *MVMFlags[] = {
    NONE,
    "canmigrate",
    "creationcompleted",
    "deleted",
    "destroypending",
    "destroyed",
    "initializing",
    "onetimeuse",
    "shared",
    "sovereign",
    "utilizationreported",
    NULL };



/* sync w/enum MImageAttrEnum */

const char *MImageAttr[] = {
  NONE,
  "OSLIST",
  "REQMEM",
  "VMOSLIST",
  NULL };


/* sync w/enum MNodeAttrEnum */

const char *MNodeAttr[] = {
  NONE,
  "ACCESS",
  "ACCOUNTLIST",
  "AGRES",
  "ALLOCRES",
  "ALLOCURL",
  "ARCH",
  "AVLCLASS",
  "AVLMEMWEIGHT",
  "AVLPROCWEIGHT",
  "AVLTIME",
  "CCODEFAILURE",
  "CCODESAMPLE",
  "CFGCLASS",
  "CFGDISKWEIGHT",
  "CFGMEMWEIGHT",
  "CFGPROCWEIGHT",
  "CFGSWAPWEIGHT",
  "GRES",            /* alphabetically order in MNodeAttrEnum */
  "CHARGERATE",
  "CLASSLIST",
  "COMMENT",
  "DEDGRES",
  "DYNAMICPRIORITY", 
  "ENABLEPROFILING",
  "EXTLOAD",
  "FAILURE",
  "FEATURES",
  "FLAGS",
  "GEVENT",
  "GMETRIC",         /* generic performance metric (temperature, throughput, etc) */
  "GROUPLIST",
  "HOPCOUNT",
  "HVTYPE",
  "INDEX",
  "ISDELETED",
  "JOBLIST",
  "JOBPRIORITYACCESS",
  "JTEMPLATELIST",
  "KBDDETECTPOLICY",
  "KBDIDLETIME",
  "LASTUPDATETIME",
  "LOAD",
  "LOADWEIGHT",
  "LOGLEVEL",
  "MAXJOB",
  "MAXJOBPERACCT",
  "MAXJOBPERGROUP",
  "MAXJOBPERUSER",
  "MAXLOAD",
  "MAXPE",
  "MAXPEPERJOB",
  "MAXPROC",
  "MAXPROCPERACCT",
  "MAXPROCPERCLASS",
  "MAXPROCPERGROUP",
  "MAXPROCPERUSER",
  "MAXWATTS",
  "MAXWCLIMITPERJOB",
  "MESSAGES",
  "MINPREEMPTLOAD",
  "MINRESUMEKBDIDLETIME",
  "NETADDR",
  "NODEID",
  "NODEINDEX",
  "NODESTATE",
  "NODESUBSTATE",
  "NODETYPE",
  "OLDMESSAGE",
  "OPERATIONS",
  "OS",
  "OSLIST",
  "OVERCOMMIT",             /* deprecate */
  "ALLOCATIONLIMITS",       /* alias for OVERCOMMIT */  
  "VMOCTHRESHOLD",          /* deprecate */
  "UTILIZATIONTHRESHOLDS",  /* alias for VMOCTHRESHOLD */
  "OWNER",
  "PARTITION",
  "POWER",
  "POWERPOLICY",
  "POWERSELECTSTATE",
  "POWERSTATE",
  "PREEMPTMAXCPULOAD",
  "PREEMPTMINMEMAVAIL",
  "PREEMPTPOLICY",
  "PRIORITYF",
  "PRIORITY",
  "PRIOWEIGHT",
  "PROCSPEED",
  "PROVDATA",
  "PROVRM",
  "QOSLIST",
  "RACK",
  "RADISK",
  "RAMEM",
  "RAPROC",
  "RASWAP",
  "RCDISK",
  "RCMEM",
  "RCPROC",
  "RCSWAP",
  "RDDISK",
  "RDMEM",
  "RDPROC",
  "RDSWAP",
  "RELEASEPRIORITY",
  "RESCOUNT",         /* number of reservations on node */
  "RSVLIST",
  "RMACCESSLIST",
  "RMMESSAGE",
  "SIZE",
  "SLOT",
  "SPEED",
  "SPEEDWEIGHT",
  "STATACTIVETIME",
  "STATMODIFYTIME",
  "STATTOTALTIME",
  "STATUPTIME",
  "TASKCOUNT",
  "TRIGGER",
  "USAGEWEIGHT",
  "USERLIST",
  "VARIABLE",
  "VMOSLIST",
  "VARATTR",
  NULL };


/* mnodectl node attribute args (sync w/enum MNodeCmdAttrEnum) */

const char *MNCMArgs[] = {
  NONE,
  "gevent",
  "os",
  "partition",
  "power",
  "state",
  "variable",
  NULL };


/* sync w/enum MXResourceEnum */

const char *MXResourceAttr[] = {
  NONE,
  "LOGINCOUNT",
  "KBDDELAY",
  NULL };


/* sync w/enum MNPrioEnum */

const char *MNPComp[] = {
  NONE,
  "ADISK",
  "AGRES",
  "AMEM",
  "APROCS",
  "ARCH",
  "ASWAP",
  "CDISK",
  "CGRES",
  "CMEM",
  "COST",
  "CPROCS",
  "CSWAP",
  "FEATURE",
  "FREETIME",
  "GMETRIC",
  "JOBCOUNT",
  "LOAD",
  "LOCAL",
  "MTBF",
  "NODEINDEX",
  "OS",
  "PARAPROCS",
  "PLOAD",
  "POWER",
  "PERCENTPARAPROCS",
  "PREF",
  "PRIORITY",
  "PROXIMITY",
  "RANDOM",
  "REQOS",
  "RSVAFFINITY",
  "SERVERLOAD",
  "SPEED",
  "SUSPENDEDJCOUNT",
  "TASKCOUNT",
  "USAGE",
  "VMCOUNT",
  "WINDOWTIME",
  NULL };


/* sync w/enum MAMJFActionEnum */

const char *MAMJFActionType[] = {
  NONE,
  "CANCEL",
  "DEFER",
  "HOLD",
  "IGNORE",
  "REQUEUE",
  "RETRY",
  NULL };



/* ID Manager types (sync w/enum MIDTypeEnum) */

const char *MIDType[] = {
  NONE,
  "EXEC",
  "FILE",
  "LDAP",
  "OTHER",
  "SQL",
  "XML",
  NULL };



/**
 * Allocation Manager attributes
 *
 * @see enum MAMAttrEnum (sync)
 * @see struct mam_t (sync)
 * @see struct mid_t (sync)
 */

const char *MAMAttr[] = {
  NONE,
  "BLOCKCREDLIST",
  "CHARGEOBJECTS",
  "CHARGEPOLICY",
  "CHARGERATE",
  "CONFIGFILE",
  "CREATECRED",
  "CREATECREDURL",
  "CREATEFAILUREACTION",
  "CREATEURL",
  "CSALGO",
  "CSKEY",
  "DELETEURL",
  "DESTROYCREDURL",
  "ENDURL",
  "FALLBACKACCOUNT",
  "FALLBACKQOS",
  "FLAGS",
  "FLUSHINTERVAL",
  "HOST",
  "JOBFAILUREACTION",
  "MESSAGES",
  "MODIFYURL",
  "NAME",
  "NODECHARGEPOLICY",
  "PAUSEURL",
  "PORT",
  "QUERYURL",
  "QUOTEURL",
  "REFRESHPERIOD",
  "RESETCREDLIST",
  "RESETCREDURL",
  "RESUMEFAILUREACTION",
  "RESUMEURL",
  "SERVER",
  "SOCKETPROTOCOL",
  "STARTFAILUREACTION",
  "STARTURL",
  "STATE",
  "TIMEOUT",
  "TYPE",
  "UPDATEFAILUREACTION",
  "UPDATEREFRESHONFAILURE",
  "UPDATEURL",
  "VERSION",
  "VALIDATEJOBSUBMISSION",
  "WIREPROTOCOL",
  NULL };


/* sync w/enum MAMFlagEnum, struct mam_t.Flags */

const char *MAMFlags[] = {
  NONE,
  "localcost",
  "strictquote",
  "accountfailasfunds",
  NULL };

/* node charge policy (sync w/enum MAMNodeChargePolicyEnum[]) */

const char *MAMNodeChargePolicy[] = {
  "NONE",
  "AVG",
  "MAX",
  "MIN",
  NULL };


/* sync w/enum MPSIEnum */

const char *MPSIAttr[] = {
  NONE,
  "avgResponseTime",
  "maxResponseTime",
  "totalRequests",
  "failCount",
  NULL };

/* sync w/enum MPSFEnum */

const char *MPSFAttr[] = {
  NONE,
  "failMsg",
  "failTime",
  "failType",
  "failCount",
  "totalCount",
  NULL };


/* sync w/enum MAMFuncTypeEnum */

const char *MAMFuncType[] = {
  NONE,
  "joballocreserve",
  "joballocdebit",
  "resallocreserve",
  "resallocdebit",
  "userdefaultquery",
  "amsync",
  NULL };

/* sync w/enum MAMNativeFuncTypeEnum */

const char *MAMNativeFuncType[] = {
  NONE,
  "CREATE",
  "DELETE",
  "END",
  "MODIFY",
  "PAUSE",
  "QUERY",
  "QUOTE",
  "RESUME",
  "START",
  "UPDATE",
  NULL };


/* sync w/enum MFSAttrEnum */

const char *MFSAttr[] = {
  NONE,
  "cap",
  "target",
  "usageList",
  NULL };


/* sync w/enum MFSCAttrEnum */

const char *MFSCAttr[] = {
  NONE, 
  "decay",
  "defTarget",
  "depth",
  "interval",
  "isabsolute",
  "policy",
  "usageList",
  NULL };


/**
 * @see enum MRMFlagEnum (sync)
 * @see mrm_t.Flags
 */

const char *MRMFlags[] = {
  NONE,
  "asyncdelete",
  "asyncstart",
  "autostart",
  "autosync",
  "becomemaster",
  "ckptrequeue",
  "client",
  "clockskewchecking",
  "compresshostlist",
  "dynamicCred",
  "dynamicCompute",
  "dynamicDataRM",
  "dynamicStorage",
  "executionServer",
  "fsisremote",
  "fullcp",
  "ignMaxStarters",
  "ignNP",
  "ignOS",
  "ignQueueState",
  "ignWorkloadState",
/* "imageIsStateful", */
  "jobNoExec",
  "localWorkloadExport",
  "localRsvExport",
  "migrateNow",
  "migrateAllJobAttributes",
  "autoURes",
  "noCreateAll",
  "noCreateResource",
  "nodeGroupOpEnabled",
  "private",
  "pushSlaveJobUpdates",
  "queueIsPrivate",
  "recordgpumetrics",
  "recordmicmetrics",
  "report",
  "rmclassupdate",
  "rootSubmit",
  "rsvExport",
  "rsvImport",
  "shared",
  "slavePeer",
  "SlurmWillRun",
  "static",
  "template",
  "userSpaceIsSeparate",
  "vlan",
  "proxyjobsubmission",
  "preemptionTesting",
  "flushresourceswhendown",
  NULL };



/* sync w/enum MRMIFlagEnum (MAX=31) */

const char *MRMIFlags[] = {
  NONE,
  "deltaresourceinfo",
  "compresshostlist",
  "defaultdedicated",
  "disableincrextjob",
  "disablepreemptextjob",
  "ignoredefaultenv",
  "imageisstateful",
  "istorque",
  "jobstageinenabled",
  "loadbalance",
  "localQueue",
  "maximizeextjob",  
  "multisource",
  "nativebasedgrid",
  "noexpandpath",
  "nomigratecheckpoint",
  "noTaskOrdering",
  "setTaskmapEnv",
  "timeoutspecified",
  NULL };


/* sync w/enum MSecFlagEnum */

const char *MSecFlags[] = {
  NONE,
  "scrub",
  "dynamicstorage",
  "reset",
  "vlan",
  NULL };


/* sync w/enum MSchedFlagEnum */

const char *MSchedFlags[] = {
  NONE,
  "allowmulticompute",
  "allowperjobnodesetisoptional",
  "extendedgroupsupport",
  "disableperjobnodesets",
  "disabletorquejobpurging",
  "donotapplygroups",
  "dynamicrm",
  "optimizedbackfill",
  "fastgrouplookup",
  "hosting",
  "jobsusersvwalltime",
  "nogridcreds",
  "socketdebug",
  "threadtest",
  "validatenodesets",
  "jobresumelog",
  "filelockha",
  "showrequestedprocs",
  "newpreemption",
  "fastrsvstartup",
  "trimtransactions",
  "strictspooldirpermissions",
  "normalizetaskdefinitions",
  "uselocalusergroup",
  "minimaljobsize",
  "allowinfinitejobs",
  "alwaysupdatecache",
  "ignorepidfilelock",
  "unmigrateondefer",
  "forceprocbasedpreemption",
  "enforcereservednodes",
  "boundedrmlimits",
  "aggregatenodefeatures",
  "novmdestroydependencies",
  "rsvtablelogdump",
  "outofbandvmrsv",
  NULL };


/* sync w/enum MLicenseFeaturesEnum */

const char *MLicenseFeatures[] = {
  NONE,
  "grid",
  "green",
  "provision",
  "vm",
  "msm",
  "vpc",
  "nohpc",
  "dynamic",
  NULL };


/* sync w/enum MResAvailQueryMethodEnum */

const char *MResAvailQueryMethod[] = {
  NONE,
  "all",
  "history",
  "local",
  "priority",
  "reservation",
  NULL };


/* sync w/enum MRMAttrEnum */

const char *MRMAttr[] = {
  NONE,
  "ADMINEXEC",
  "APPLYGRESTOALLREQS",
  "AUTHTYPE",
  "AUTHALIST",
  "AUTHCLIST",
  "AUTHGLIST",
  "AUTHQLIST",
  "AUTHULIST",
  "BANDWIDTH",
  "CANCELFAILUREEXTENDTIME",
  "CHECKPOINTSIG",
  "CHECKPOINTTIMEOUT",
  "CLIENT",
  "CLOCKSKEW",
  "CLUSTERQUERYURL",
  "COMPLETEDJOBPURGE",
  "CONFIGFILE",
  "CREDCTLURL",
  "CSALGO",
  "CSKEY",
  "DATARM",
  "DEFAULTCLASS",
  "DEFOS",
  "DEFSTAGEINTIME",
  "DEFAULTHIGHSPEEDADAPTER",
  "DEFAULT.JOB",
  "DESCRIPTION",
  "ENCRYPTION",
  "ENV",
  "EPORT",
  "EXTHOST",
  "FAILTIME",
  "FBSERVER",
  "FEATURELIST",
  "FLAGS",
  "FNLIST",
  "HOST",
  "IGNHNODES",
  "INFOQUERYURL",
  "ISDELETED",
  "JOBCANCELURL",
  "JOBCOUNT",
  "JOBCOUNTER",
  "JOBEXPORTPOLICY",
  "JOBEXTENDDURATION",
  "JOBIDFORMAT",
  "JOBMIGRATEURL",
  "JOBMODIFYURL",
  "JOBREQUEUEURL",
  "JOBRESUMEURL",
  "JOBRSVRECREATE",
  "JOBSIGNALURL",
  "JOBSTARTCOUNT",
  "JOBSTARTURL",
  "JOBSUBMITURL",
  "JOBSUSPENDURL",
  "JOBVALIDATEURL",
  "LANGUAGES",
  "LOCALDISKFS",
  "MASTERRM",
  "MAXITERATIONFAILURECOUNT",
  "MAX.JOB",
  "MAXJOBHOP",
  "MAXJOBPERMINUTE",
  "MAXJOBS",
  "MAXJOBSTARTDELAY",
  "MAXNODEHOP",
  "MAXTRANSFERRATE",
  "MESSAGES",
  "MINETIME",
  "MIN.JOB",
  "MINSTAGETIME",
  "NAME",
  "NC",
  "NMHOST",
  "NMPORT",
  "NMSERVER",
  "NOALLOCMASTER",
  "NODEDESTROYPOLICY",
  "NODEFAILURERSVPROFILE",
  "NODELIST",
  "NODEMIGRATEURL",
  "NODEMODIFYURL",
  "NODEPOWERURL",
  "NODESTATEPOLICY",
  "NODEVIRTUALIZEURL",
  "NONEEDNODES",
  "OMAP",
  "PACKAGE",
  "PARCREATEURL",
  "PARDESTROYURL",
  "PARTITION",
  "POLLINTERVAL",
  "POLLTIMEISRIGID",
  "PORT",
  "PREEMPTDELAY",
  "PROCS",
  "PROFILE",
  "PROVDURATION",
  "PTYSTRING",
  "USEVNODES",
  "QUEUEQUERYURL",
  "REQNC",
  "RESOURCECREATEURL",
  "RESOURCETYPE",
  "RMINITIALIZEURL",
  "RMSTARTURL",
  "RMSTOPURL",
  "RSVCTLURL",
  "SBINDIR",
  "SERVER",
  "SET.JOB",
  "SLURMFLAGS",
  "SOCKETPROTOCOL",
  "SOFTTERMSIG",
  "SQLDATA",
  "STAGETHRESHOLD",
  "STARTCMD",
  "STATE",
  "STATS",
  "SUBMITCMD",
  "SUBMITPOLICY",
  "SUSPENDSIG",
  "SYNCJOBID",
  "SYSTEMMODIFYURL",
  "SYSTEMQUERYURL",
  "TARGETUSAGE",
  "TIMEOUT",
  "TRIGGER",
  "TYPE",
  "UPDATEJOBRMCREDS",
  "UPDATETIME",
  "VARIABLES",
  "VERSION",
  "VMOWNERRM",
  "VPGRES",
  "WIREPROTOCOL",
  "WORKLOADQUERYURL",
  NULL };


/* sync w/enum MRMSubmitPolicyEnum */

const char *MRMSubmitPolicy[] = {
  NONE,
  "NODECENTRIC",
  "PROCCENTRIC",
  NULL };


/* sync w/enum MClassFlagsEnum */

const char *MClassFlags[] = {
  NONE,
  "REMOTE",
  NULL };



/* sync w/enum MCredAttrEnum */

const char *MCredAttr[] = {
  NONE,
  "CDEF",
  "CHARGERATE",
  "COMMENT",
  "EMAILADDRESS",
  "FLAGS",
  "FSCAP",
  "FSTARGET",
  "GLOBALFSTARGET",
  "GLOBALFSUSAGE",
  "HOLD",
  "PRIORITY",
  "MAXGRES",
  "MAXJOB",
  "MAXARRAYJOB",
  "MAXNODE",
  "MAXPE",
  "MAXPROC",
  "MINPROC",
  "MAXPS",
  "MAXWC",
  "MAXMEM",
  "MAXIGRES",
  "MAXIJOB",
  "MAXIARRAYJOB",
  "MAXINODE",
  "MAXIPE",
  "MAXIPROC",
  "MAXIPS", 
  "MAXIWC",
  "MAXIMEM",
  "OMAXJOB",
  "OMAXNODE",
  "OMAXPE",
  "OMAXPROC",
  "OMAXPS",
  "OMAXWC",
  "OMAXMEM",
  "OMAXIJOB",
  "OMAXINODE",
  "OMAXIPE",
  "OMAXIPROC",
  "OMAXIPS",
  "OMAXIWC",
  "OMAXIMEM",
  "OMAXJNODE",
  "OMAXJPE",
  "OMAXJPROC",
  "OMAXJPS",
  "OMAXJWC",
  "OMAXJMEM",
  "REQRID",
  "RMAXDURATION",
  "RMAXPROC",
  "RMAXPS",
  "RMAXCOUNT",
  "RMAXTOTALDURATION",
  "RMAXTOTALPROC",
  "RMAXTOTALPS",
  "PDEF",
  "PLIST",
  "QDEF",
  "QLIST",
  "ROLE",
  "ALIST",
  "ADEF",
  "JOBFLAGS",   
  "MAXJOBPERUSER",
  "MAXJOBPERGROUP",
  "MAXNODEPERUSER",
  "MAXPROCPERUSER",
  "MAXPROCPERNODEPERQUEUE",
  "OVERRUN",
  "ID",
  "INDEX",
  "DEFAULT.TPN",
  "DEFAULT.WCLIMIT",
  "MIN.WCLIMIT",
  "MAX.WCLIMIT",
  "MAXPROCPERNODE",
  "MIN.NODE",
  "MAX.NODE",
  "MAX.PROC",
  "MEMBERULIST",
  "NOSUBMIT",
  "CSPECIFIC",
  "PREF",
  "ENABLEPROFILING",
  "FSWEIGHT",
  "MANAGERS",
  "TRIGGER",
  "MESSAGE",
  "ISDELETED",
  "MAXSUBMITJOBS",
  "VARIABLE",
  "RSVCREATIONCOST",
  NULL };




/**< sync w/enum MGResAttrEnum */

const char *MGResAttr[] = {
  NONE,
  "chargerate",
  "inverttaskcount",
  "private",
  "startdelay",
  "type",
  "featuregres",
  NULL };



/* sync w/enum MCreditTypeEnum */

const char *MCreditType[] = {
  "NONE",
  "INTERNAL",
  "ENERGY",
  NULL };



/* sync w/enum MFSTreeACLPolicyEnum */

const char *MFSTreeACLPolicy[] = {
  "OFF",
  "FULL",
  "PARENT",
  "SELF",
  NULL };



/* sync w/enum MMBTypeEnum */

const char *MMBType[] = {
  NONE,
  "annotation",
  "hold",
  "other",
  "pendingactionerror",
  "db",
  NULL };



/* sync w/MMBAttrEnum[] */

const char *MMBAttr[] = {
  NONE,
  "COUNT",
  "CTIME",
  "DATA",
  "EXPIRETIME",
  "LABEL",
  "OWNER",
  "PRIORITY",
  "SOURCE",
  "TYPE",
  NULL };


/* sync w/enum MParAttrEnum */

const char *MParAttr[] = {
  NONE,
  "ACCOUNT",
  "ACL",
  "ALLOCRESFAILPOLICY",
  "ARCH",
  "BACKLOGDURATION",
  "BACKLOGPS",
  "CALENDAR",
  "CLASSES",
  "CMDLINE",
  "CONFIGFILE",
  "COST",
  "DEDSRESOURCES",
  "DEFAULT.CPULIMIT",
  "DEFAULTNODEACTIVEWATTS",
  "DEFAULTNODEIDLEWATTS",
  "DEFAULTNODESTANDBYWATTS",
  "DURATION",
  "FLAGS",
  "GREENBACKLOGTHRESHOLD",
  "GREENQUEUETIMETHRESHOLD",
  "GREENXFACTORTHRESHOLD",
  "GRES",
  "GROUP",
  "ID",
  "ISREMOTE",
  "JOBACTIONONNODEFAILURE",
  "JOBCOUNT",
  "JOBNODEMATCHPOLICY",
  "MAX.CPULIMIT",
  "MAX.PROC",
  "MAX.PS",
  "MAX.NODE",
  "MAX.WCLIMIT",
  "MIN.PROC",
  "MIN.NODE",
  "MIN.WCLIMIT",
  "MESSAGES",
  "NODEACCESSPOLICY",
  "NODEALLOCATIONPOLICY",
  "NODECHARGERATE",
  "NODEFEATURES",
  "NODEPOWEROFFDURATION",
  "NODEPOWERONDURATION",
  "OS",
  "OWNER",
  "PRIORITY",
  "PROFILE",
  "PROVENDTIME",
  "PROVSTARTTIME",
  "PURGETIME",
  "QOS",
  "REQOS",
  "REQRESOURCES",
  "RESERVATIONDEPTH",
  "RESOURCES",
  "RSVNODEALLOCATIONPOLICY",
  "RSVNODEALLOCATIONPOLICYPRIORITYF",
  "RM",
  "RMTYPE",
  "RSVPROFILE",
  "SIZE",
  "STARTTIME",
  "STATE",
  "SUSPENDRESOURCES",
  "UPDATETIME",
  "USER",
  "USETTC",
  "VARIABLES",
  "VMCREATEDURATION",
  "VMDELETEDURATION",
  "VMMIGRATEDURATION",
  "VPCFLAGS",
  "VPCNODEALLOCATIONPOLICY",
  "WATTCHARGERATE",
  NULL };


/* sync w/enum MParFlagEnum */

const char *MParFlags[] = {
  NONE,
  "UseMachineSpeed",
  "UseSystemQueueTime",
  "UseCPUTime",
  "RejectNegPrioJobs",
  "EnableNegJobPriority",
  "EnablePosUserPriority",
  "DisableMultiReqJobs",
  "UseLocalMachinePriority",
  "UseFloatingNodeLimits",
  "UseTTC",
  "SLURMIsShared",
  "DestroyIfEmpty",
  "IsDynamic",
  "IsRemote",
  "SharedMem",
  NULL };


/* sync w/enum MOServiceAttrEnum */

const char *MOServAttr[] = {
  NONE,
  "AUTH",
  "AUTHTYPE",
  "CSALGO",
  "CSKEY",
  "DROPBADREQUEST",
  "HOST",
  "KEY",
  "PORT",
  "SPROTOCOL",
  "VERSION",
  NULL };


/* NOTE:  must be sync'd with MXO, enum MXMLOTypeEnum */

/* WARNING!!!ALERT!!! Adding or changing anything here needs to be synced with the
                      create.db.all.sql script and should only happen during major
                      revisions. */

const char *MCredCfgParm[] = {
  NULL,
  "ACCOUNTCFG",
  "AMCFG",
  "CLASSCFG",
  "VCPROFILE",
  "GROUPCFG",
  "JOBCFG",
  "NODECFG",
  "PARCFG",
  "QOSCFG",
  "QUEUECFG",
  NULL,
  NULL,
  NULL,
  "RMCFG",
  "SCHEDCFG",
  "SRCFG",
  "SYSCFG",
  NULL,
  NULL,
  "USERCFG",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  "IMAGECFG",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL };

const char *MCredCfgParm2[] = {
  NULL,
  "ACCOUNT",
  "AM",
  "CLASS",
  "VCPROFILE",
  "GROUP",
  "JOB",
  "NODE",
  "PAR",
  "QOS",
  "QUEUE",
  NULL,
  NULL,
  NULL,
  "RM",
  "SCHED",
  "SR",
  "SYS",
  NULL,
  NULL,
  "USER",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  "IMAGE",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL };

/* sync w/MQOSAttrEnum */

const char *MQOSAttr[] = {
  NONE,
  "DYNATTRS",
  "PREEMPTMAXTIME",
  "PREEMPTMINTIME",
  "PREEMPTPOLICY",
  "PRIORITY",
  "MAXJOB",
  "MAXPROC",
  "MAXNODE",
  "XFWEIGHT",
  "QTWEIGHT",
  "XFTARGET",
  "QTTARGET",
  "QFLAGS",
  "FSSCALINGFACTOR",
  "FSTARGET",
  "PREEMPTBLTHRESHOLD",
  "PREEMPTQTTHRESHOLD",
  "PREEMPTXFTHRESHOLD",
  "RSVBLTHRESHOLD",
  "RSVQTTHRESHOLD",
  "RSVXFTHRESHOLD",
  "ACLBLTHRESHOLD",
  "ACLQTTHRESHOLD",
  "ACLXFTHRESHOLD",
  "POWERBLTHRESHOLD",
  "POWERQTTHRESHOLD",
  "POWERXFTHRESHOLD",
  "TRIGGERBLTHRESHOLD",
  "TRIGGERQTTHRESHOLD",
  "TRIGGERXFTHRESHOLD",
  "DEDRESCOST",
  "UTLRESCOST",
  "RSVACCESSLIST",
  "RSVCREATIONCOST",
  "RSVPREEMPTLIST",
  "MINRSVRLSTIME",
  "RSVTIMEDEPTH",
  "BACKLOG",
  "ONDEMAND",
  "SPECATTRS",
  "GREENBACKLOGTHRESHOLD",
  "GREENQUEUETIMETHRESHOLD",
  "GREENXFACTORTHRESHOLD",
  "EXEMPTLIMIT",
  "SYSPRIO",
  "JOBPRIOACCRUALPOLICY",
  "JOBPRIOEXCEPTIONS",
  "PREEMPTEES",
  NULL };


/* sync w/enum MCalAttrEnum */

const char *MCalAttr[] = {
  NONE,
  "ENDTIME",
  "IMASK",
  "NAME",
  "PERIOD",
  "STARTTIME",
  NULL };



/* sync w/enum MTransAttrEnum */

const char *MTransAttr[] = {
  NONE,
  "ACL",
  "NodeList",
  "Duration",
  "StartTime",
  "Flags",
  "OpSys",
  "DRes",
  "RsvProfile",
  "Label",
  "DependentTIDList",
  "MShowCmd",
  "Cost",
  "VCDescription",
  "VCMaster",
  "VPCProfile",
  "Variables",
  "VMUsage",
  "NodeFeatures",
  "Name",
  "RsvID",
  "IsValid",
  NULL };

/* sync w/enum MClassAttrEnum */

const char *MClassAttr[] = {
  NONE,
  "RESFAILPOLICY",
  "COMMENT",
  "OCNODE",
  "CANCELFAILEDJOBS",
  "DEFAULT.ATTR",
  "DEFAULT.EXT",
  "DEFAULT.DISK",
  "DEFAULT.FEATURES",
  "DEFAULT.GRES",
  "DEFAULT.MEM",
  "DEFAULT.NODE",
  "DEFAULT.NODESET",
  "DEFAULT.OS",
  "DEFAULT.PROC",
  "DISABLEAM",
  "EXCL.FEATURES",
  "EXCL.FLAGS",
  "EXCLUDEUSERLIST",
  "FORCENODEACCESSPOLICY",
  "HOSTLIST",
  "IGNNODESETMAXUSAGE",
  "JOBEPILOG",
  "JOBPROLOG",
  "JOBTERMSIG",
  "JOBTRIGGER",
  "LOGLEVEL",
  "MAXPROCPERNODE",
  "MAX.ARRAYSUBJOBS",
  "MAX.CPUTIME",
  "MAX.NODE",
  "MAX.PROC",
  "MAX.PS",
  "MAX.GPU",
  "MIN.NODE",
  "MIN.PROC",
  "MIN.PS",
  "MIN.TPN",
  "MIN.GPU",
  "NAME",
  "NODEACCESSPOLICY",
  "PRIORITYF",
  "OCDPROCFACTOR",
  "OTHER",
  "PARTITION",
  "PRETERMINATIONSIGNAL",
  "REQ.FEATURES",
  "REQ.FLAGS",
  "REQ.IMAGE",
  "REQUIREDQOSLIST",
  "REQUIREDUSERLIST",
  "REQUIREDACCOUNTLIST",
  "RM",
  "RMLIST",
  "SYSPRIO",
  "WCOVERRUN",
  NULL };


/* sync w/enum MClassStateEnum */

const char *MClassState[] = {
  NONE,
  "active",
  "disabled",
  "closed",
  "stopped",
  NULL };


/* sync w/enum MACLAttrEnum */

const char *MACLAttr[] = {
  NONE,
  "aff",
  "cmp",
  "dval",      /* conditional value (double) */
  "flag",
  "name",
  "type",
  "val",
  NULL };


/* sync w/enum MACLEnum */

const char *MACLFlags[] = {
  NONE,
  "Deny",
  "XOR",
  "Required",
  "Conditional",
  "CredLock",
  "HPEnable",
  NULL };


/* sync w/enum MAcctAttrEnum */

const char *MAcctAttr[] = {
  NONE,
  "VDEF",
  NULL };


/* sync w/enum MGroupAttrEnum */

const char *MGroupAttr[] = {
  NONE,
  "CLASSWEIGHT",
  NULL };


/**
 * User-specific config attributes.
 *
 * NOTE: sync w/enum MUserAttrEnum
 *
 * @see struct mgcred_t,enum MCredAttrEnum
 */

const char *MUserAttr[] = {
  NONE,
  "ADMINLEVEL",
  "JOBNODEMATCHPOLICY",
  "MAX.WCLIMIT",
  "NOEMAIL",
  "PROXYLIST",
  "VIEWPOINTPROXY",
  "PRIVILEGES",
  NULL };



/* workload statistics are relative to job records entered */
/* resource statistics are relative to cluster monitored */
/* timefame monitored included */

/* sync w/enum MStatAttrEnum, must_t */

const char *MStatAttr[] = {
  NONE,
  "AQT",  /* average queue time */
  "ABP",  /* average bypass */
  "AXF",  /* average x factor */
  "TANC", /* total active node count (updated while job is active) */
  "TAPC", /* total active processor count (updated while job is active) */
  "TCapacity", /* total available accessible capacity (in procs) */
  "TJC",  /* total job count */
  "TNJC", /* total allocated node count (only updated on job completion) */
  "TQT",  /* total queue wait time */
  "MQT",  /* max queue wait time */
  "TRT",  /* total requested job walltime */
  "TET",  /* total execution job walltime */
  "TMSA", /* total memory-seconds (MB * sec) available */
  "TMSD", /* total memory-seconds (MB * sec) dedicated */
  "TMSU", /* total memory-seconds (MB * sec) utilized */
  "TDSA", /* total disk-seconds (MB * sec) available */
  "TDSD", /* total disk-seconds (MB * sec) dedicated */
  "TDSU", /* total disk-seconds (MB * sec) utilized */
  "IDSU", /* total disk-seconds (MB * sec) utilized instant */
  "TPSR", /* total proc-seconds (proc * sec) requested */
  "TPSE", /* total proc-seconds (proc * sec) executed */
  "TPSD", /* total proc-seconds (proc * sec) dedicated */
  "TPSU", /* total proc-seconds (proc * sec) utilized */
  "TJA",  /* total job accuracy */
  "TNJA", /* total node weighted job accuracy */
  "TXF",  /* total xfactor */
  "TNXF", /* total node-weighted xfactor */
  "MXF",  /* max xfactor */
  "TBP",  /* total bypass count */
  "MBP",  /* max bypass */
  "TQM",  /* total completed jobcount which satisfied QOS */
  "StartTime",  /* time stats collection began */
  "IStatCount",
  "IStatDuration",  /* profile duration length (see PROFILEDURATION in config docs) */
  "GCEJobs", /* current eligible jobs */
  "GCIJobs", /* current idle jobs */
  "GCAJobs", /* current active jobs */
  "GPHAvl",  /* processor hours available */
  "GPHUtl",  /* processor hours utilized */
  "GPHDed",  /* processor hours dedicated */
  "GPHSuc",  /* processor hours successful */
  "MinEff",  /* minimum efficiency */
  "MinEffIteration", /* mininum efficiency iteration */
  "TPreemptPH",  /* total preempted processor hours */
  "TPreemptJC",  /* total preepmted job count */
  "TEvalJC",
  "TSchedDuration",
  "TSchedCount",
  "TNC",  /* total node count on system */
  "TPC",  /* total proc count on system */
  "TQPH", /* total queued processor hours */
  "TQJC", /* total queued job count */
  "TSubmitJC", /* jobs submitted this interval */
  "TSubmitPH", /* proc-hours submitted this interval (avg job ph = TSubmitPH / TSubmitJC) */
  "TStartJC",  /* jobs started this interval */
  "TStartPC",  /* proc-weighted jobs started this interval (avg job pc = TStartPC / TStartJC) */
  "TStartQT",  /* total queuetime of jobs started this interval (avgqt = TStartQT / TStartJC) */
  "TStartXF",  /* total xfactor of jobs started this interval (avgxf = TStartXF / TStartJC) */
  "Duration",
  "IC",
  "Profile",
  "QOSCredits",
  "TNL",
  "TNM",
  "XLoad",
  "GMetric",
  NULL };



const char *MLimitAttr[] = {
  NONE,
  "UJobs",
  "UProcs",
  "UPS",
  "UMem",
  "UNodes",
  "HLAJobs",
  "HLAProcs",
  "HLAPS",
  "HLANodes",
  "HLIJobs",
  "HLINodes",
  "SLAJobs",
  "SLAProcs",
  "SLAPS",
  "SLANodes",
  "SLIJobs",
  "SLINodes",
  NULL };



/* sync w/enum MAdminAttrEnum */

const char *MAdminAttr[] = {
  NONE,
  "CANCELJOBS",
  "ENABLEPROXY",
  "INDEX",
  "NAME",
  "SERVICES",
  "USERS",
  "GROUPS",
  NULL };


/* sync w/enum MSchedAttrEnum */

const char *MSchedAttr[] = {
  NONE,
  "SERVERALIAS",
  "CHANGESET",
  "CHARGEMETRICPOLICY",
  "CHARGERATEPOLICY",
  "CHECKPOINTDIR",
  "CPVERSION",
  "DATABASEHANDLE",
  "DEFAULTNODEACCESSPOLICY",
  "EVENTCOUNTER",
  "FBSERVER",
  "FLAGS",
  "GID",
  "HALOCKCHECKTIME",
  "HALOCKFILE",
  "HALOCKUPDATETIME",
  "HOMEDIR",
  "HTTPSERVERPORT",
  "IGNORENODES",
  "ITERATIONCOUNT",
  "JOBMAXNODECOUNT",
  "LASTCPTIME",
  "LASTTRANSCOMMITTED",
  "LOCALQUEUEISACTIVE",
  "MAXJOBID",
  "MAXRECORDEDCJOBID",
  "MESSAGE",
  "MINJOBID",
  "MODE",
  "NAME",
  "RECOVERYACTION",
  "RESTARTSTATE",
  "REVISION",
  "RSVCOUNTER",
  "RSVGROUPCOUNTER",
  "LOCKFILE",
  "SCHEDULERNAME",
  "SERVER", 
  "SPOOLDIR",
  "STATE",
  "STATEMADMIN",
  "STATEMTIME",
  "TIME",
  "TRIGCOUNTER",
  "TRANSCOUNTER",
  "TRIGGER",
  "UNSUPPORTEDDEPENDENCIES",
  "UID",
  "VARDIR",
  "VARLIST",
  "VERSION",
  "MAXJOB",
  NULL };


/* sync w/MRecoveryActionEnum */

const char *MRecoveryAction[] = {
  NONE,
  "DIE",
  "IGNORE",
  "RESTART",
  "TRAP",
  NULL };


/* sync w/enum MClientAttrEnum */

const char *MClientAttr[] = {
  NONE,
  "AUTHCMD",
  "AUTHCMDOPTIONS",
  "CLOCKSKEW",
  "AUTHTYPE",
  "DEFAULTSUBMITPARTITION",
  "ENCRYPTION",
  "FLAGS",
  "LOCALDATASTAGEHEAD",
  "TIMEOUT",
  NULL };


/* sync w/enum MSysAttrEnum */

const char *MSysAttr[] = {
  NONE,
  "AJC",           /* active job count */
  "APS",           /* active job proc-seconds remaining */
  "APServerHost",  /* access portal server state */
  "APServerTime",
  "ATAPH",         /* average time queued PH ? */
  "ATQPH",         /* average time queued PH ? */
  "CPITime",       /* checkpoint initialization time */
  "EJC",           /* eligible job count */
  "fsInitTime",
  "IJC",           /* idle job count */
  "IMEM",          /* idle mem */
  "INC",           /* idle node count */
  "IPC",           /* idle proc count */
  "IC",            /* Iteration count */
  "mineff",        /* minimum efficiency */
  "minEffIter",    /* minimum efficiency iteration */
  "PJC",           /* preempt job count */
  "time",          /* present time */
  "QPS",           /* queued ps */
  "RMPI",          /* RM poll interval */
  "SCJC",          /* successful completed job count */
  "statInitTime",
  "UPMEM",
  "UPN",           /* up node count */
  "UPP",           /* up proc count */
  "version",
  NULL };


/* sync w/enum MRackAttrEnum */

const char *MRackAttr[] = {
  NONE,
  "Index",
  "Name",
  NULL };


/* sync w/enum MFSTreeAttrEnum */

const char *MFSTreeAttr[] = {
  NONE,
  "memberlist",
  "comment",
  "cshare",
  "fscap",
  "fsfactor",
  "limits",
  "managers",
  "name",
  "partition",
  "qlist",
  "qdef",
  "share",
  "totalshare",
  "type",
  "usage",
  NULL };



/* sync w/enum MCKPtModeEnum */

const char *MCKMode[] = {
  NONE,
  "cjob",
  "general",
  "rsv",
  "sys",
  "sched",
  NULL };


/* sync w/enum MClusterAttrEnum */

const char *MClusterAttr[] = {
  NONE,
  "MaxSlot",
  "Name",
  "PresentTime",
  NULL };


/* sync w/enum MTrigFlagEnum */

const char *MTrigFlag[] = {
  NONE,
  "asynchronous",
  "attacherror",
  "cleanup",
  "interval",
  "leavefiles",
  "multifire",
  "probe",
  "probeall",
  "user",
  "genericsysjob",
  "globalvars",
  "globaltrig",
  "removestdfiles",
  "resetonmodify",
  "checkpoint",
  "objectxmlstdin",
  "softkill",
  NULL };


/* sync w/enum MTrigPolicyFlagEnum */

const char *MTrigPolicyFlag[] = {
  NONE,
  "removeparentjoboncompletion",
  "removetrigonunsetviolation",
  "reprocessonvarset",
  NULL };

/* sync w/enum MTrigVarAttr */

const char *MTrigVarFlag[] = {
  NONE,
  "incr",
  "export",
  "noexport",
  NULL };

/* sync w/enum MTrigECPolicyEnum */

const char *MTrigECPolicy[] = {
  NONE,
  "ignore",
  "failneg",
  "failpos",
  "failnonzero",
  NULL };


/* sync w/enum MTrigAttrEnum */

const char *MTrigAttr[] = {
  NONE,
  "Action",
  "AType",
  "BlockTime",
  "DependsOn",
  "Description",
  "Disabled",
  "EBuf",
  "ECPolicy",
  "Env",
  "ETime",
  "EType",
  "ExpireTime",
  "FailOffset",
  "FailureDetected",
  "Flags",
  "IBuf",
  "IsComplete",
  "Interval",
  "LaunchTime",
  "MaxRetry",
  "Message",
  "MultiFire",
  "Name",
  "OID",
  "OType",
  "OBuf",
  "Offset",
  "Period",
  "PID",
  "RearmTime",
  "Requires",
  "Sets",
  "State",
  "StdIn",
  "Threshold",
  "TID",
  "Timeout",
  "Unsets",
  "RetryCount",
  NULL };



/* sync w/MTrigStateEnum */

const char *MTrigState[] = {
  NONE,
  "Active",
  "Launched",
  "Successful",
  "Failure",
  NULL };


/* sync w/enum MHistoryAttrEnum */

const char *MHistoryAttr[] = {
  NONE,
  "elementcount",
  "time",
  "state",
  NULL };


/* sync w/enum MHostListModeEnum */

const char *MHostListMode[] = {
  NONE,
  "superset",
  "subset",
  "exactset",
  NULL };


/* sync w/enum MRsvAttrEnum */

const char *MRsvAttr[] = {
  NONE,
  "AAccount",
  "ACL",
  "AGroup",
  "AllocNodeCount",
  "AllocNodeList",
  "AllocProcCount",
  "AllocTaskCount",
  "AQOS",
  "AUser",
  "CL",
  "comment",
  "cost",
  "creds",
  "ctime",
  "duration",
  "endtime",
  "ExcludeRsv",
  "expiretime",
  "flags",
  "GlobalID",
  "History",
  "HostExp", 
  "HostExpIsSpecified",
  "IsActive",
  "IsTracked",
  "JProfile",
  "JState",     
  "Label",     
  "LastChargeTime",
  "LogLevel",
  "MaxJob",
  "Messages",
  "Name",
  "NodeSetPolicy",
  "Owner",
  "Partition",
  "Priority",
  "Profile",
  "ReqArch",
  "ReqFeatureList",
  "ReqFeatureMode",
  "ReqMemory",
  "ReqNodeCount",
  "ReqNodeList",
  "ReqOS",
  "ReqVC",
  "Resources",
  "RsvAccessList",
  "RsvGroup",
  "RsvParent",
  "SystemID",
  "SpecName",
  "starttime",
  "StatCAPS",
  "StatCIPS",
  "StatTAPS",
  "StatTIPS",
  "SubType",
  "SysJobID",
  "SysJobTemplate",
  "ReqTaskCount",
  "ReqTPN",
  "Trigger",
  "Type",
  "UIndex",
  "Variable",
  "VMList",
  "VMUsage",
  NULL };

const char *MNRsvAttr[] = {
  NONE,
  "DRes",
  "End",
  "Name",
  "State",
  "Start",
  "TC",
  "Type",
  NULL };



/* sync w/enum MSTAttrEnum */

const char *MSTrigAttr[] = {
  NONE,
  "DOFIREMISSEDTRIGGERS",
  "LASTTRIGGEREVALTIME",
  "NAME",
  "PERIOD",
  "OFFSET",
  "TRIGGER",
  NULL };


/* sync w/enum MSRAttrEnum */

const char *MSRsvAttr[] = {
  NONE,
  "ACCESS",
  "ACCOUNTLIST",
  "ACTIONTIME",
  "CHARGEACCOUNT",
  "CHARGEUSER",
  "CLASSLIST",
  "CLUSTERLIST",
  "COMMENT",
  "DAYS",
  "DEPTH",
  "DESCRIPTION",
  "DISABLE",
  "DISABLETIME",
  "DISABLEDTIMES",
  "DURATION",
  "ENABLETIME",
  "ENDTIME",
  "FLAGS",
  "GROUPLIST",
  "HOSTLIST",
  "IDLETIME",
  "ISDELETED",
  "JOBATTRLIST",
  "JOBPRIORITYACCESS",
  "JTEMPLATELIST",
  "LOGLEVEL",
  "MAXJOB",
  "MAXTIME",
  "MINMEM",
  "MMB",
  "NAME",
  "NODEFEATURES",
  "OS",
  "OWNER",
  "PARTITION",
  "PERIOD",
  "PRIORITY",
  "PROCLIMIT",
  "PSLIMIT",
  "QOSLIST",
  "RESOURCES",
  "ROLLBACKOFFSET",
  "RSVGROUP",
  "RSVACCESSLIST",
  "RSVLIST",
  "STARTTIME",
  "STIDLETIME",
  "STTOTALTIME",
  "SUBTYPE",
  "SYSJOBTEMPLATE",
  "TASKCOUNT",
  "TASKLIMIT",
  "TIMELIMIT",
  "TPN",
  "TRIGGER",
  "TYPE",
  "USERLIST",
  "VARIABLES",
  "WENDTIME",
  "WSTARTTIME",
  NULL };


/* sync w/enum MJobNonEType */

const char *MJobBlockReason[] = {
  NONE,
  "ActivePolicy",
  "BadUser",
  "Dependency",
  "EState",
  "FairShare",
  "Hold",
  "IdlePolicy",
  "LocalPolicy",
  "NoClass",
  "NoData",
  "NoResource",
  "NoTime",
  "PartitionAccess",
  "Priority",
  "RMSubmissionFailure",
  "StartDate",
  "State",
  "SysLimits",
  NULL };


/* sync w/enum MObjectActionEnum */

const char *MObjectAction[] = {
  NONE,
  "creat",
  "list",
  "modify",
  "destroy",
  "diagnose",
  NULL };



/* sync w/enum MXT3ParAttrEnum */

const char *MXT3ParAttr[] = {
  NONE,
  "CREATETIME",
  "JOBID",
  "ORPHANED",
  "PROCS",
  NULL };
   


/* sync w/enum MJobAttrEnum */

const char *MJobAttr[] = {
  NONE,
  "Account",
  "AllocNodeList",
  "AllocVMList",
  "Args",
  "ArrayInfo",
  "ArrayLimit",
  "AWDuration",
  "BecameEligible",
  "BlockReason",
  "Bypass",
  "CancelExitCodes",
  "Class",
  "CmdFile",
  "CompletionCode",
  "CompletionTime",
  "Cost",
  "CPULimit",
  "Depend",
  "DependBlock",
  "Description",
  "DRM",
  "DRMJID",
  "EEDuration",
  "EffPAL",
  "EFile",
  "EligibilityNotes",
  "Env",
  "EnvOverride",
  "EpilogueScript",
  "EState",
  "EstWCTime",
  "ExcHList",
  "Flags",
  "GAttr",
  "Geometry",
  "GJID",
  "Group",
  "Hold",
  "Holdtime",
  "HopCount",
  "AllocHostList",
  "HostList",
  "IFile",
  "ImmediateDepend",
  "InteractiveScript",
  "IsInteractive",
  "IsRestartable",
  "IsSuspendable",
  "IWD",
  "JobID",
  "JobName",
  "JobGroup",
  "JobGroupArrayIndex",
  "LastChargeTime",
  "LogLevel",
  "MasterHost",
  "Messages",
  "MinPreemptTime",
  "MinWCLimit",
  "NodeAllocationPolicy",
  "NodeMod",
  "NodeSelection",
  "Notification",
  "MailUser",       /* notification address */
  "OFile",
  "OMinWCLimit",
  "OWCLimit",
  "Owner",
  "PAL",
  "ParentVCs",
  "PrologueScript",
  "QueueStatus",
  "QOS",
  "QOSReq",
  "RCL",
  "ReqFeatures",
  "ReqAWDuration",
  "ReqCMaxTime",
  "ReqMem",
  "ReqNodes",
  "ReqProcs",
  "ReqReservation",
  "ReqReservationPeer",
  "ReqRM",
  "ReqRMType",
  "ReqSMinTime",
  "ReqVMList",
  "ResFailPolicy",
  "RM",
  "RMCompletionTime",
  "RMStdErr",
  "RMStdOut",
  "RMFlags",
  "RMXString",
  "RsvAccess",
  "RsvStartTime",
  "RunPriority",
  "SessionID",
  "Shell",
  "SID",
  "Size",           /* Cray's interpretation of size (maps to Tasks and DProcs) only used by XT*/
  "Storage",
  "SRM",
  "SRMJID",
  "StageIn",
  "StartCount",
  "StartPriority",
  "StartTime",
  "State",
  "StatMSUtl",
  "StatPSDed",
  "StatPSUtl",
  "StdErr",
  "StdIn",
  "StdOut",
  "StepID",
  "SubmitArgs",
  "SubmitDir",
  "SubmitHost",
  "SubmitLanguage",
  "SubmitString",
  "SubmissionTime",
  "SubState",
  "SuspendDuration",
  "ServiceJob",
  "SysPrio",
  "SysSMinTime",
  "TaskMap",
  "TemplateFailurePolicy",
  "TemplateList",
  "TemplateSetList",
  "TermTime",
  "TemplateFlags",
  "Trigger",
  "TrigNameSpace",
  "TaskCount",            /* Total Task Count */
  "UMask",
  "User",
  "UserPrio",
  "UtlMem",
  "UtlProcs",
  "Variable",
  "VM",
  "VMCR",
  "VMUsagePolicy",
  "VWCTime",
  "Warning",
  "WorkloadRMID",
  "ArrayMasterID",
  NULL };

/* sync w/enum MJobCfgAttrEnum */

const char *MJobCfgAttr[] = {
  NONE,
  "ACCOUNT",
  "ACTION",
  "CLASS",
  "CPULIMIT",
  "DESCRIPTION",
  "DGRES",
  "FAILUREPOLICY",
  "FLAGS",
  "GROUP",
  "JOBSCRIPT",
  "MEM",              /* NOTE: This memory is for MEM=<Value> */
  "NODEACCESSPOLICY",
  "NODESET",
  "PARTITION",
  "PREF",
  "QOS",
  "RESTARTABLE",
  "RM",
  "RMSERVICEJOB",
  "SELECT",
  "STAGEIN",
  "SRM",
  "TFLAGS",
  "USER",
  "VMUSAGE",
  "WALLTIME",
  "SYSTEMJOBTYPE",

  /* The following are Wiki Attributes */

  "ARGS",
  "DMEM", 
  "DDISK",
  "DSWAP",
  "ERROR",
  "EXEC",
  "EXITCODE",
  "GATTR",
  "GEVENT",
  "IWD",
  "JNAME",
  "NAME",
  "PARTITIONMASK",
  "PRIORITYF",
  "RDISK",
  "RSWAP",
  "RAGRES",
  "RCGRES",
  "TASKPERNODE",
  "TRIGGER",
  "VARIABLE",

  /* Wiki attributes documented in Job Templates */

  "DPROCS",
  "EUSER",
  "GENERICSYSJOB",
  "GNAME",
  "GRES",
  "INHERITRES",
  "NODES",
  "PRIORITY",
  "RARCH",
  "RFEATURES",
  "ROPSYS",
  "TASKS",
  "TEMPLATEDEPEND",
  "UNAME",
  "WCLIMIT",
  "DESTROYTEMPLATE",
  "MIGRATETEMPLATE",
  "MODIFYTEMPLATE",
  NULL };

/* sync w/enum MPostDSFileEnum */

const char *MPostDSFile[] = {
  NONE,
  "STDOUT",
  "STDERR",
  "STDALL",
  "EXPLICIT",
  NULL };


/* sync w/enum MJTXEnum */

const char *MJTXAttr[] = {
  NONE,
  "templatedepend",
  "reqfeatures",
  "gres",
  "variables",
  "walltime",
  "resources",
  "samplecount",
  "memory",
  NULL };


/* sync w/enum MJobLLAttrEnum */

const char *MJobLLAttr[] = {
  NONE,
  "arguments",
  "blocking",
  "class",
  "executable",       /* Cmd */
  "environment",
  "requirements",     /* HostList */
  "initialdir",
  "job_name",
  "job_type",
  /* NEED SUPPORT FOR 'network.<X>' - NYI */
  "queue",
  "wall_clock_limit",
  "node",             /* ReqNodes */
  "restart",          /* job is restartable */
  "startdate",        /* release time */
  "error",            /* stderr */
  "output",           /* stdout */
  "task_geometry",
  "total_tasks",
  NULL };


/**
 * sync w/ enum MVMMigrationPolicyEnum
 */
  
const char *MVMMigrationPolicy[] = {
  NONE,
  "CONSOLIDATION",
  "CONSOLIDATIONOVERCOMMIT",
  "OVERCOMMIT",
  "PERFORMANCE",  /* alias for OVERCOMMIT */
  NULL };




/**
 *  sync w/ enum MVMStaleActionEnum
 */

const char *MVMStaleAction[] = {
  NONE,
  "IGNORE",
  "CANCELTRACKINGJOB",
  "DESTROY",
  NULL };



/**
 * Virtual Machine Usage Policy
 *
 * @see mjob_t.VMUsagePolicy
 * @see enum MVMUsagePolicyEnum
 */

const char *MVMUsagePolicy[] = {
  NONE,
  "any",
  "requirepm",
  "prefpm",
  "createvm",
  "requirevm",
  "prefvm",
  "createpersistentvm",
  NULL };



/* sync w/enum MXAttrType */

const char *MRMXAttr[] = {
  NONE,
  "ADVRES",
  "ARRAY",
  "BLOCKING",
  "CGRES",
  "DEADLINE",
  "DEPEND",
  "DDISK",
  "DMEM",
  "ENVREQUESTED",
  "EPILOGUE",
  "EXCLUDENODES",
  "FEATURE",
  "FLAGS",
  "GATTR",
  "GEOMETRY",
  "GMETRIC",
  "GRES",
  "HOSTLIST",
  "IMAGE",
  "JOBFLAGS",
  "JOBREJECTPOLICY",
  "JGROUP",
  "LOCALDATASTAGEHEAD",
  "LOGLEVEL",
  "MAXMEM", 
  "MAXPROC",
  "MINPREEMPTTIME",
  "MINPROCSPEED",
  "MINWCLIMIT",
  "MPPMEM",
  "MPPHOST",
  "MPPARCH",
  "MPPWIDTH",
  "MPPNPPN",
  "MPPDEPTH",
  "MPPNODES",
  "MPPLABELS",
  "MSTAGEIN",
  "MSTAGEOUT",
  "NACCESSPOLICY",
  "NALLOCPOLICY",
  "NMATCHPOLICY",
  "NODEMEM",
  "NODESCALING",
  "NODESET",
  "NODESETISOPTIONAL",
  "NODESETCOUNT",
  "NODESETDELAY",
  "OPSYS",
  "PLACEMENT",
  "PARTITION",
  "POSTSTAGEACTION",
  "POSTSTAGEAFTERTYPE",
  "PREF",
  "PROCS",
  "PROCS_BITMAP",
  "PROLOGUE",
  "QOS",
  "QUEUEJOB",
  "REQATTR",
  "RESFAILPOLICY",
  "RMTYPE",
  "SELECT",
  "SGE",
  "SID",
  "SIGNAL",
  "SIZE",
  "SJID",
  "SRMJID",
  "STAGEIN",
  "STAGEOUT",
  "STDERR",
  "STDOUT",
  "TASKDISTPOLICY",
  "TEMPLATE",
  "TERMSIG",
  "TERMTIME",
  "TID",
  "TPN",
  "TRIG",
  "TRIGNS",
  "TRL",
  "TTC",
  "UMASK",
  "SPRIORITY",
  "VAR",
  "VC",
  "VMUSAGE",
  "VPC",
  "VRES",
  "WCREQUEUE",
  "WCKEY",
  NULL };


/* sync w/enum MTJobFlagEnum (MAX=32) */
const char *MTJobFlags[] = {
  NONE,
  "GLOBALRSVACCESS",
  "HIDDEN",
  "HWJOB",
  "PRIVATE",
  "SYNCJOBID",
  "TEMPLATEISDYNAMIC",
  "SELECT",
  NULL };

const char *MTJobActionTemplate[] = {
  NONE,
  "DESTROY",
  "MIGRATE",
  "MODIFY",
  NULL };


/* sync w/enum MJobFlagEnum */

const char *MJobFlags[] = {
  NONE,
  "BACKFILL",
  "COALLOC",
  "ADVRES",
  "NOQUEUE",
  "ARRAYJOB",
  "ARRAYJOBPARLOCK",
  "ARRAYJOBPARSPAN",
  "ARRAYMASTER",
  "BESTEFFORT",
  "RESTARTABLE",
  "SUSPENDABLE",
  "HASPREEMPTED",
  "PREEMPTEE",
  "PREEMPTOR",
  "RSVMAP",
  "SPVIOLATION",
  "IGNNODEPOLICIES",
  "IGNPOLICIES",
  "IGNNODESTATE",
  "IGNIDLEJOBRSV",
  "INTERACTIVE",
  "FSVIOLATION",
  "GLOBALQUEUE",
  "NORESOURCES",
  "NORMSTART",
  "CLUSTERLOCKED",
  "FRAGMENT",
  "SYSTEMJOB",
  "ADMINSETIGNPOLICIES",
  "EXTENDSTARTWALLTIME",
  "SHAREDMEM",
  "BLOCKEDBYGRES",
  "GRESONLY",
  "TEMPLATESAPPLIED",
  "META",
  "WIDERSVSEARCHALGO",
  "VMTRACKING",
  "DESTROYTEMPLATESUBMITTED",
  "PROCSPECIFIED",
  "CANCELONFIRSTFAILURE",
  "CANCELONFIRSTSUCCESS",
  "CANCELONANYFAILURE",
  "CANCELONANYSUCCESS",
  "CANCELONEXITCODE",
  NULL };


/* dynamic job attributes, sync w/enum MJobDynAttrEnum */

const char *MJobDynAttr[] = {
  NONE,
  "account",
  "nodeaccesspolicy",
  "nodecount",
  "preempt",
  "priority",
  "qos",
  "queue",
  "taskcount",
  "wclimit",
  "var",
  NULL };



/* sync w/enum MReqAttrEnum */

const char *MReqAttr[] = {
  NONE,
  "AllocNodeList",
  "AllocPartition",
  "Depend",
  "ExclNodeFeatures",
  "GPUS",
  "MICS",
  "GRes",
  "CGRes",
  "Index",
  "NodeAccess",
  "NodeSet",
  "Pref",
  "ReqArch",
  "ReqAttr",
  "ReqDiskPerTask",
  "ReqImage",
  "ReqMemPerTask",
  "ReqNodeDisk",
  "ReqNodeFeature",
  "ReqNodeMem",
  "ReqNodeAMem",
  "ReqNodeProc",
  "ReqNodeSwap",
  "ReqNodeASwap",
  "ReqOpsys",
  "ReqPartition",
  "ReqProcPerTask",
  "ReqSoftware",
  "ReqSwapPerTask",
  "RMName",
  "SpecNodeFeature",
  "NCReqMin",
  "TCReqMin",
  "TPN",
  "TaskCount",
  "UtilMem",
  "UtilProc",
  "UtilSwap",
  NULL };


/* sync w/enum MQOSFlagEnum */

const char *MQOSFlags[] = {
  NONE,
  "IGNJOBPERUSER",
  "IGNPROCPERUSER",
  "IGNJOBQUEUEDPERUSER",
  "IGNSYSMAXPE",
  "IGNSYSMAXPROC",
  "IGNSYSMAXTIME",
  "IGNSYSMAXPS",
  "IGNUSER",
  "IGNSYSTEM",
  "IGNALL",
  "PREEMPTOR",
  "PREEMPTEE",
  "DEDICATED",
  "RESERVEALWAYS",
  "USERESERVED",
  "NOBF",
  "NORESERVATION",
  "NTR",
  "RUNNOW",
  "PREEMPTFSV",
  "PREEMPTSPV",
  "IGNHOSTLIST",
  "PROVISION",
  "DEADLINE",
  "ENABLEUSERRSV",
  "COALLOC",
  "TRIGGER",
  "PREEMPTCONFIG",
  "PREEMPTORX",
  "VIRTUAL",
  NULL };


/* sync with enum MLogLevelEnum */

const char *MLogLevel[] = {
  "DEBUG", 
  "INFO",
  "WARN",
  "ERROR",
  "FATAL",
  NULL };



/* sync with enum MLogFacilityEnum */

const char *MLogFacilityType[] = {
  "CORE",
  "SCHED",
  "SOCK",
  "UI",
  "LL",
  "SDR",
  "CONFIG",
  "STAT",
  "SIM",
  "STRUCT",
  "FS",
  "CKPT",
  "AM",
  "RM",
  "PBS",
  "WIKI",
  "TRANS",
  "SSS",
  "NAT",
  "GRID",
  "COMM",
  "MIGRATE",
  "ALL" };

const char *MTLimitType[] = {
  NONE,
  "active",
  "idle",
  "system",
  NULL };


/* sync w/enum MRsvThresholdEnum */

const char *MRsvThresholdType[] = {
  "NONE",
  "BYPASS",
  "QUEUETIME",
  "XFACTOR",
  NULL };


/* sync w/enum MNodeAllocPolicyEnum */

const char *MNAllocPolicy[] = { 
  NONE,
  "CONTIGUOUS",
  "CPULOAD",
  "PROCESSORLOAD",   /* alias for CPULOAD */
  "FASTEST",
  "NODESPEED",       /* alias for FASTEST */
  "FIRSTAVAILABLE",
  "INREPORTEDORDER", /* alias for FIRSTAVAILABLE */
  "FIRSTSET",
  "LASTAVAILABLE",
  "INREVERSEREPORTEDORDER", /* alias for LASTAVAILABLE */
  "LOCAL",
  "PLUGIN",
  "PRIORITY",
  "CUSTOMPRIORITY",  /* alias for PRIORITY */
  "MAXBALANCE",
  "PROCESSORSPEEDBALANCE", /* alias for MAXBALANCE */
  "MINGLOBAL",
  "MINLOCAL", 
  "MINRESOURCE",
  "MINIMUMCONFIGUREDRESOURCES",  /* alias for MINRESOURCE */ 
  NULL };


/**
 * Node Allocation Resource Failure Policy
 *
 * sync w/enum MAllocResFailurePolicyEnum 
 */

const char *MARFPolicy[] = {
  NONE,
  "CANCEL",
  "FAIL",
  "HOLD",
  "IGNORE",
  "NOTIFY",
  "REQUEUE",
  NULL };


/* partition allocation policies (sync w/enum MParAllocPolicyEnum) */

const char *MPAllocPolicy[] = {
  NONE,
  "BESTFIT",
  "BESTPFIT",
  "FIRSTCOMPLETION",
  "FIRSTSTART",
  "ROUNDROBIN",
  "LOADBALANCE",
  "LOADBALANCEP",
  "RANDOM",
  NULL };


/**
 * Reservation allocation policy (sync w/enum MRsvAllocPolicyEnum) 
 */

const char *MRsvAllocPolicy[] = {
  NONE,
  "failure",
  "never",
  "optimal",
  "prestartfailure",
  "prestartmaintenance",
  "prestartoptimal",
  "remap",
  NULL };


/* sync w/enum MNodeKbdDetectPolicyEnum */

const char *MNodeKbdDetectPolicy[] = {
  NONE,
  "DRAIN",
  "PREEMPT",
  NULL };


const char *MAllocLocalityPolicy[] = {
  NONE,
  "BESTEFFORT",
  "FORCED",
  "RMSPECIFIC",
  NULL };


/* sync w/enum MTaskDistEnum */

const char *MTaskDistributionPolicy[] = {
  NONE,
  "DEFAULT",
  "LOCAL",
  "ARBGEO",
  "PACK",
  "RR",
  "DISPERSE",
  NULL };

const char *MNAvailPolicy[] = {
  NONE,
  "COMBINED",
  "DEDICATED",
  "UTILIZED",
  NULL };

const char *MBFWindowTraversal[] = { 
  NONE, 
  "LONGESTFIRST", 
  "WIDESTFIRST", 
  NULL };


/* sync w/enum MRsvSearchAlgoEnum */

const char *MRsvSearchAlgo[] = {
  NONE,
  "WIDE",
  "LONG",
  NULL };


const char *MBFMPolicy[] = { 
  NONE,
  "PROCS",
  "NODES",
  "PROCSECONDS",
  "SECONDS", 
  NULL };


/* job size policy */

const char *MJSPolicy[] = { 
  "MINPROC", 
  "MAXPROC", 
  "NDMAXPROC", 
  NULL };


/* job reservation policy (sync w/enum XXX) */

const char *MRsvPolicy[] = { 
  "DEFAULT", 
  "HIGHEST", 
  "CURRENTHIGHEST", 
  "NEVER",
  NULL };


/* job scheduling policy (sync w/enum MSchedPolicyEnum) */

const char *MSchedPolicy[] = {
  NONE,
  "EARLIESTSTART",
  "EARLIESTCOMPLETION",
  NULL };

 
/* sync w/enum MHoldTypeEnum */

const char *MHoldType[] = { 
  NONE, 
  "User", 
  "System", 
  "Batch", 
  "Defer", 
  "All", 
  NULL };


const char *MComType[] = { 
  "ROUNDROBIN", 
  "BROADCAST", 
  "TREE", 
  "MASTERSLAVE", 
  NULL };


/* sync w/enum MHoldReasonEnum */

const char *MHoldReason[] = { 
  "NONE",
  "Admin",
  "NoResources",
  "SystemLimitsExceeded",
  "BankFailure",
  "CannotDebitAccount",
  "InvalidAccount",
  "RMFailure",
  "RMReject",
  "PolicyViolation",
  "CredAccess",
  "CredHold",
  "PreReq",
  "Data",
  "Security",
  "MissingDependency",
  NULL };


/* sync w/enum MCkptTypeEnum */

const char *MCPType[] = { 
  "Sched",
  "JOB",
  "RESERVATION",
  "SRES",
  "NODE",
  "USER",
  "GROUP",
  "ACCOUNT",
  "TOTAL",
  "RTOTAL",
  "CTOTAL",
  "GTOTAL",
  "SYSSTATS",
  "RM",
  "AM",
  "SYS",
  "TRIGGER",
  "QOS",
  "CLASS",
  "STRIG",
  "PAR",
  "CJOB",
  "LISTCJOB",
  "CVM",
  "TJOB",
  "DTJob",
  "TRANS",
  "VC",
  "VM",
  NULL };


/* sync w/enum MStatObjectEnum */

const char *MStatObject[] = {
  "NONE",
  "credential",
  "node",
  NULL };


/* sync w/enum MMStatTypeEnum */

const char *MStatType[] = {
  "NONE",
  "AVGXFACTOR", 
  "MAXXFACTOR", 
  "AVGQTIME",
  "AVGBYPASS", 
  "MAXBYPASS", 
  "JOBCOUNT", 
  "PHREQUEST",
  "PHRUN",
  "WCACCURACY",
  "BFCOUNT",
  "BFPHRUN",
  "QOSDELIVERED",
  "ESTSTARTTIME",
  NULL };


const char *MLLJobState[] = {
  "Idle",
  "Pending",
  "Starting",
  "Running",
  "CompletePending",
  "RejectPending",
  "RemovePending",
  "VacatePending",
  "Completed",
  "Rejected",
  "Removed",
  "Vacated",
  "Cancelled",
  "NotRun",
  "Terminated",
  "Unexpanded",
  "SubmissionErr",
  "Hold",
  "Deferred",
  "NotQueued",
  "Preempted",
  "PreemptPending",
  "ResumePending",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  "???",
  NULL };

/* FORMAT:  NAME       FUNC    ADM0    ADM1   ADM2   ADM3   ADM4    ADM5 */

/* admin0:   not used
 * admin1:   scheduler admin (full authority) 
 * admin2:   operator (full information + job/rsv managment)
 * admin3:   helpdesk (full information + limited job/rsv management)
 * admin4:   authenticated user (limited information + limited job/rsv management) 
 * admin5:   non-authenticated user (limited information) */

/* sync w/enum MSvcEnum */

/* specify if role has access, no access, or ownership (MBNOTSET) access */

/* NOTE:  MUI[].Func is initialized in MUIInitialize() */

/* @see MCRequestF[] (peer) */

/* @see enum mcs* (sync) */

muis_t MUI[] = {
  /* SName             Func  AdminAccess                                     */
  /*                          NotUsed  Admin1 Admin2 Admin3 Admin4 Admin5   */

  { NONE,              NULL, { FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE },       },
  { "showstate",       NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "setspri",         NULL, { FALSE,   TRUE,  TRUE, FALSE, FALSE, FALSE },       },
  { "setupri",         NULL, { FALSE,   TRUE,  TRUE, FALSE, FALSE, FALSE },       }, /* NOTE: not used */
  { "showq",           NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE,  TRUE },       },
  { "sethold",         NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "releasehold",     NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "showstats",       NULL, { FALSE,   TRUE,  TRUE,  TRUE, MBNOTSET, MBNOTSET }, },
  { "setres",          NULL, { FALSE,   TRUE,  TRUE,  TRUE, FALSE, FALSE },       },
  { "releaseres",      NULL, { FALSE,   TRUE,  TRUE, FALSE, FALSE, FALSE },       },
  { "showres",         NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, MBNOTSET },    },
  { "diagnose",        NULL, { FALSE,   TRUE,  TRUE,  TRUE, MBNOTSET, FALSE },    },
  { "setqos",          NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "showbf",          NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE,  TRUE },       },
  { "showconfig",      NULL, { FALSE,   TRUE,  TRUE,  TRUE, FALSE, FALSE },       },
  { "checkjob",        NULL, { FALSE,   TRUE,  TRUE,  TRUE, MBNOTSET, MBNOTSET},  },
  { "checknode",       NULL, { FALSE,   TRUE,  TRUE,  TRUE, MBNOTSET, MBNOTSET},  },
  { "runjob",          NULL, { FALSE,   TRUE,  TRUE, FALSE, FALSE, FALSE },       },
  { "canceljob",       NULL, { FALSE,   TRUE,  TRUE, MBNOTSET, MBNOTSET, FALSE }, },
  { "changeparam",     NULL, { FALSE,   TRUE, FALSE, FALSE, FALSE, FALSE },       },
  { "showstart",       NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "mjobctl",         NULL, { FALSE,   TRUE,  TRUE, MBNOTSET, MBNOTSET, FALSE }, },
  { "mnodectl",        NULL, { FALSE,   TRUE,  TRUE, FALSE, FALSE, FALSE },       },
  { "mrsvctl",         NULL, { FALSE,   TRUE,  TRUE, MBNOTSET, MBNOTSET, FALSE }, },
  { "mschedctl",       NULL, { FALSE,   TRUE,  TRUE, FALSE, FALSE, FALSE },       }, /* NOTE: disable */
  { "mstat",           NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "mdiag",           NULL, { FALSE,   TRUE,  TRUE,  TRUE, MBNOTSET, FALSE },    }, /* NOTE: disable */
  { "mshow",           NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "mvcctl",          NULL, { FALSE,   TRUE,  TRUE,  TRUE, FALSE, FALSE },       },
  { "mvmctl",           NULL, { FALSE,   TRUE,  TRUE,  FALSE,  FALSE, FALSE },    },
  { "mbal",            NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "mcredctl",        NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "mrmctl",          NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { "msub",            NULL, { FALSE,   TRUE,  TRUE,  TRUE,  TRUE, FALSE },       },
  { NULL,              NULL, { FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE },       } };


/* sync w/enum MClientCmdEnum */

const char *MClientCmd[] = {
  NONE,
  "mbal",
  "mdiag",     /* diagnose */    
  "mjobctl",   /* runjob, canceljob, setqos, sethold, releasehold, setspri, setupri */
  "mnodectl",
  "mvcctl",
  "mvmctl",
  "mrsvctl",   /* setres */
  "mschedctl", /* schedctl, changeparam, resetstats, showconfig */
  "mshow",     /* showq, checknode, checkjob, showres, showstate */     
  "mstat",     /* showstats */
  "msub",
  "mcredctl",
  "mrmctl",
  "meventthreadsafe",
  "mshowthreadsafe",
  "mjobctlthreadsafe",
  "mnodectlthreadsafe",
  "mrsvdiagnosethreadsafe",
  "mtrigdiagnosethreadsafe",
  "mvcctlthreadsafe",
  "mvmctlthreadsafe",
  "mjobsubmitthreadsafe",
  "mpeerresourcesquery",
  "checkjob",
  "checknode",
  "showres",
  "showstate",
  NULL };


/* sync w/enum MAllocRejEnum */

const char *MAllocRejType[] = {
  NONE,
  "Account",
  "ACL",
  "Arch",
  "Class",
  "Corruption",
  "CPU",
  "Data",
  "Dependency",
  "Disk",
  "DynamicConfig",
  "EState",
  "FairShare",
  "Features",
  "GRes",
  "Group",
  "Hold",
  "HostList",
  "Locality",
  "Memory",
  "NodeCount",
  "NodePolicy",
  "NodeSet",
  "NoSubmit",
  "Opsys",
  "Partition",
  "Policy",
  "Pool",
  "Priority",
  "QOS",
  "ReleaseTime",
  "RM",
  "State",
  "Swap",
  "SystemLimits",
  "TaskCount",
  "Reserved",
  "User",           /* 32 */
  "VM",
  NULL };


/* sync w/enum MDependEnum */
/* sync w/SDependtime[] */
/* sync w/TDependtime[] */

const char *MDependType[] = {
  NONE,
  "jobstart",
  "jobcomplete",
  "jobsuccessfulcomplete",
  "jobfailedcomplete",
  "jobsynccount",
  "jobsync",
  "on",
  "before",
  "beforeany",
  "beforeok",
  "beforenotok",
  "DAG",
  "set",
  "notset",
  "eq",
  "ne",
  "ge",
  "le",
  "gt",
  "lt",
  NULL };


/* sync w/enum MDependEnum */
/* sync w/MDependType[] */
/* sync w/TDependType[] */

const char* SDependType[] = {
  NONE,
  "after",
  "afterany",
  "afterok",
  "afternotok",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL };

/* sync w/enum MDependEnum */
/* sync w/MDependType[] */
/* sync w/SDependType[] */

const char* TDependType[] = {
  NONE,
  "after",
  "afterany",
  "afterok",
  "afternotok",
  "synccount",
  "syncwith",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL };


/* sync w/enum MDependEnum */

const char* TriggerDependType[] = {
  NONE,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  "varset",
  "varnotset",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL };

/* sync w/XXX */

const char *MPolicyRejection[] = {
  NONE,
  "MaxJobPerUser",
  "MaxProcPerUser",
  "MaxNodePerUser",
  "MaxProcSecondPerUser",
  "MaxJobQueuedPerUser",
  "MaxPEPerUser",
  "MaxJobPerGroup",
  "MaxNodePerGroup",
  "MaxNodeSecondPerGroup",
  "MaxJobQueuedPerGroup",
  "MaxJobPerAccount",
  "MaxNodePerAccount",
  "MaxNodeSecondPerAccount",
  "MaxJobQueuedPerAccount",
  "MaxSystemJobSize",
  "MaxSystemJobTime", "MaxSystemJobPS",
  "UnknownUser",
  "UnknownGroup",
  "UnknownAccount",
  NULL };
 
const char *MRackName[] = { "0",
   "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9", "10",
  "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
  "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
  "31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
  NULL };

const char *MNodeName[] = {
  "00","01","02","03","04","05","06","07","08","09","10","11","12","13","14","15","16",
  NULL };

/* sync w/enum MSchedStateEnum */

const char *MSchedState[] = {
  NONE,
  "PAUSED",
  "STOPPED",
  "RUNNING",
  "SHUTDOWN",
  NULL };

/* sync w/enum MSchedModeEnum */

const char *MSchedMode[] = {
  NONE,
  "INTERACTIVE",
  "MONITOR",
  "NORMAL",
  "NOSCHEDULE",
  "PROFILE",
  "SIMULATION",
  "SINGLESTEP",
  "SLAVE",
  "TEST",
  NULL };


/* sync w/enum MClusterStateEnum */

const char *MClusterState[] = {
  NONE,
  "pending",
  "initializing",
  "active",
  "cleanup",
  "completed",
  NULL };


/* sync w/enum MJobTemplateFailurePolicyEnum */

const char *MJobTemplateFailurePolicy[] = {
  NONE,
  "cancel",
  "hold",
  "retry",
  "retrystep",
  NULL };

/* sync w/enum MPowerPolicyEnum */

const char *MPowerPolicy[] = {
  "NONE",
  "Green",
  "OnDemand",
  "Static",
  NULL };


/* sync w/enum MPowerStateEnum */

const char *MPowerState[] = {
  "NONE",
  "Off",
  "On",
  "Reset",
  "Hibernate",
  "Standby",
  NULL };



/* sync w/enum MNodeStateEnum */

const char *MNodeState[] = {
  "NONE",
  "None",
  "Down",
  "Idle",
  "Busy",
  "Running",
  "Drained",  /* mnsDrained  */
  "Draining", /* mnsDraining */
  "Flush",
  "Reserved",
  "Unknown",
  "Up",
  NULL };


/* sync w/enum MNodeStatTypeEnum, mnust_t */
/* NOTE:  no string can be substring of another field - see MStatFromXML() */

const char *MNodeStatType[] = {
  NONE,
  "AGRes",
  "AProc",
  "CAT",
  "CGRes",
  "CProc",
  "CPULoad",
  "DGRes",
  "DProc",
  "FailureJC",
  "GMetric",
  "IStatCount",
  "IStatDuration",
  "IC",
  "MaxCPULoad",
  "MinCPULoad",
  "MaxMem",
  "MinMem",
  "NodeState",
  "Profile",
  "Queuetime",
  "RCAT",
  "StartJC",
  "STARTTIME",
  "SuccessiveJobFailures",
  "UMem",
  "XLoad",
  "XFactor",
  NULL };


/* sync w/enum MJEnvVarEnum */

const char *MJEnvVarAttr[] = {
  "NONE",
  "MOAB_ACCOUNT",      /* Account Name */
  "MOAB_BATCH",        /* Set if a batch job (non-interactive). */
  "MOAB_CLASS",        /* Class Name */
  "MOAB_DEPEND",       /* Job dependency string. */
  "MOAB_GROUP",        /* Group Name */
  "MOAB_JOBARRAYINDEX",/* Job index into array.  */
  "MOAB_JOBARRAYRANGE",/* Job array count.  */
  "MOAB_JOBID",        /* Job ID. If submitted from the grid, grid jobid. */
  "MOAB_JOBINDEX",     /* Index in job array. */
  "MOAB_JOBNAME",      /* Job Name */
  "MOAB_MACHINE",      /* Name of the machine (ie. Destination RM) that the job is running on. */
  "MOAB_NODECOUNT",    /* Number of nodes allocated to job */
  "MOAB_NODELIST",     /* Comma-separated list of nodes (listed singly with no ppn info). */
  "MOAB_PARTITION",    /* Partition name the job is running in. If grid job, cluster scheduler's name. */
  "MOAB_PROCCOUNT",    /* Number of processors allocated to job */
  "MOAB_QOS",          /* QOS Name */
  "MOAB_TASKMAP",      /* Node list with procs per node listed. <nodename>.<procs> */
  "MOAB_USER",         /* User Name */
  NULL};


/* sync w/enum MJTMatchAttrEnum */

const char *MJTMatchAttr[] = {
  NONE,
  "FLAGS",
  "JMAX",
  "JMIN",
  "JDEF",
  "JSET",
  "JSTAT",
  "JUNSET",
  NULL };

/* sync w/enum MJTSetActionEnum */

const char *MJTSetAction[] = {
  NONE,
  "HOLD",
  "CANCEL",
  NULL };

const char *MWCVAction[] = {
  NONE,
  "CANCEL",
  "PREEMPT",
  NULL };

const char *MTimePolicy[] = {
  NONE,
  "REAL",
  NULL };


/* preempt policies */

/* sync w/enum MPreemptPolicyEnum */

const char *MPreemptPolicy[] = {
  NONE,
  "CANCEL",
  "CHECKPOINT",
  "MIGRATE",
  "OVERCOMMIT",
  "REQUEUE",
  "SUSPEND",
  NULL };


/* sync w/enum MActionPolicyEnum */

const char *MActionPolicy[] = {
  NONE,
  "BESTEFFORT",
  "FORCE",
  "NEVER",
  NULL };

/* sync w/enum MOExpSearchOrderEnum */

const char *MOExpSearchOrder[] = {
  NONE,
  "exact",
  "range",
  "regex",
  "rangere",
  "rerange",
  NULL };


/* sync w/enum MJobRejectPolicyEnum */

const char *MJobRejectPolicy[] = {
  NONE,
  "CANCEL",
  "HOLD",
  "IGNORE",
  "MAIL",
  "RETRY",
  NULL };

/* sync w/enum MGEWikiAttrEnum */

const char *MGEWikiAttr[] = {
  NONE,
  "MESSAGE",
  "MTIME",
  "PARENTNODE",
  "PARENTVM",
  "SEVERITY",
  "STATUSCODE",
  NULL };

/* sync w/enum MGEActionEnum */

const char *MGEAction[] = {
  NONE,
  "AVOID",
  "CHARGE",
  "DISABLE",
  "ENABLE",
  "EXECUTE",
  "FAIL",
  "NOTIFY",
  "OBJECTXMLSTDIN",
  "OFF",
  "ON",
  "PREEMPT",
  "RECORD",
  "RESERVE",
  "RESET",
  "SCHED.RESET",
  "SIGNAL",
  "SUBMIT",
  NULL };


/* sync w/enum MVCFlagEnum */

const char *MVCFlags[] = {
  NONE,
  "DESTROYOBJECTS",
  "DESTROYWHENEMPTY",
  "DELETING",
  "HASSTARTED",
  "HOLDJOBS",
  "WORKFLOW",
  NULL };

/* sync w/enum MVCExecuteEnum */

const char *MVCExecute[] = {
  NONE,
  "clearmb",
  "schedulevc",
  NULL };

/* sync w/enum MVCAttrEnum */

const char *MVCAttr[] = {
  NONE,
  "ACL",
  "CREATEJOB",
  "CREATETIME",
  "CREATOR",
  "DESCRIPTION",
  "FLAGS",
  "JOBS",
  "MESSAGES",
  "NAME",
  "NODES",
  "OWNER",
  "PARENTVCS",
  "REQNODESET",
  "REQSTARTTIME",
  "RSVS",
  "THROTTLEPOLICIES",
  "VARIABLES",
  "VCS",
  "VMS",
  NULL };


/* sync w/ enum MDependFailPolicyEnum */

const char *MDependFailPolicy[] = {
  NONE,
  "HOLD",
  "CANCEL",
  NULL };


/* sync w/enum MJobMigratePolicyEnum */

const char *MJobMigratePolicy[] = {
  NONE,
  "auto",
  "immediate",
  "justintime",
  "loadbalance",
  NULL };



/* sync w/enum MMonthEnum */

const char *MMonth[] = {
  "january",
  "february",
  "march",
  "april",
  "may",
  "june",
  "july",
  "august",
  "september",
  "october",
  "november",
  "december" };

/** 
 * config data 
 * @see enum MCfgParamEnum
 * @see struct mcfg_t
 */

/*  PARAMETER                   INDEX                         FORMAT      OBJECT    ???   DEPR   NewIndex ValueArray */

#ifdef __cplusplus
extern "C"
#endif
const mcfg_t MCfg[] = {
  { NONE,                       mcoNONE,                      mdfString,  mxoNONE,  NULL, FALSE, mcoNONE, NULL },
  { "ACCOUNTINGINTERFACEURL",   mcoAccountingInterfaceURL,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ACCOUNTCAP",               mcoCACap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "ACCOUNTCFG",               mcoAcctCfg,                   mdfString,  mxoAcct,  NULL, FALSE, mcoNONE, NULL },
  { "ACCOUNTWEIGHT",            mcoCAWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "ALLOWMULTIREQNODEUSE",     mcoAllowMultiReqNodeUse,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADJUSTUTILIZEDRES",        mcoAdjUtlRes,                 mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADMINCFG",                 mcoAdminCfg,                  mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADMIN1",                   mcoAdmin1Users,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADMIN2",                   mcoAdmin2Users,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADMIN3",                   mcoAdmin3Users,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADMIN4",                   mcoAdmin4Users,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADMIN5",                   mcoAdmin5Users,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ADMINS",                   mcoAdminUsers,                mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "AFSWEIGHT",                mcoOLDAFSWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFAWeight, NULL },
  { "AGGREGATENODEACTIONS",     mcoAggregateNodeActions,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "AGGREGATENODEACTIONSTIME", mcoAggregateNodeActionsTime,  mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ALLOCATIONPOLICY",         mcoOLDAllocationPolicy,       mdfString,  mxoSched, NULL, TRUE,  mcoNodeAllocationPolicy, NULL },
  { "ALLOCRESFAILPOLICY",       mcoAllocResFailPolicy,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "ALLOWROOTJOBS",            mcoAllowRootJobs,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ALLOWVMMIGRATION",         mcoAllowVMMigration,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ALWAYSAPPLYQOSOVERRIDE",   mcoAlwaysApplyQOSLimitOverride, mdfString, mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ALWAYSEVALUATEALLJOBS",    mcoAlwaysEvaluateAllJobs,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "AMCFG",                    mcoAMCfg,                     mdfString,  mxoAM,    NULL, FALSE, mcoNONE, NULL },
  { "APIFAILURETHRESHOLD",      mcoAPIFailureThreshold,       mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "APIFAILURETHRESHHOLD",     mcoAPIFailureThreshold,       mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },   /* typo - should be deprecated */
  { "ARRAYJOBPARLOCK",          mcoArrayJobParLock,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ATTRATTRCAP",              mcoAJobAttrCap,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },         
  { "ATTRATTRWEIGHT",           mcoAJobAttrWeight,            mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },     
  { "ASSIGNVLANFEATURES",       mcoAssignVLANFeatures,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ATTRGRESCAP",              mcoAJobGResCap,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "ATTRGRESWEIGHT",           mcoAJobGResWeight,            mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "ATTRSTATECAP",             mcoAJobStateCap,              mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },      
  { "ATTRSTATEWEIGHT",          mcoAJobStateWeight,           mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "ATTRWEIGHT",               mcoAttrWeight,                mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },   
  { "AUTHTIMEOUT",              mcoAuthTimeout,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "BACKFILLDEPTH",            mcoBFDepth,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BACKFILLMAXSCHEDULES",     mcoBFMaxSchedules,            mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BACKFILLMETRIC",           mcoBFMetric,                  mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BACKFILLPOLICY",           mcoBFPolicy,                  mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBFPolicy },
  { "BACKFILLPROCFACTOR",       mcoBFProcFactor,              mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BACKFILLTYPE",             mcoBFType,                    mdfString,  mxoPar,   NULL, TRUE,  mcoBFPolicy, NULL },
  { "BFCHUNKDURATION",          mcoBFChunkDuration,           mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BFCHUNKSIZE",              mcoBFChunkSize,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BFMINVIRTUALWALLTIME",     mcoBFMinVirtualWallTime,      mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BFVIRTUALWALLTIMECONFLICTPOLICY", mcoBFVirtualWallTimeConflictPolicy, mdfString, mxoPar, NULL, FALSE, mcoNONE, NULL },
  { "BFVIRTUALWALLTIMESCALINGFACTOR", mcoBFVirtualWallTimeScalingFactor, mdfString, mxoPar, NULL, FALSE, mcoNONE, NULL },
  { "BFPRIORITYPOLICY",         mcoBFPriorityPolicy,          mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BGPREEMPTION",             mcoBGPreemption,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "BLOCKLIST",                mcoBlockList,                 mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBlockList },
  { "BYPASSCAP",                mcoSBPCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "BYPASSWEIGHT",             mcoSBPWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USEDATABASE",              mcoUseDatabase,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DONTCANCELINTERACTIVEHJOBS", mcoDontCancelInteractiveHJobs, mdfString, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CHECKPOINTALLACCOUNTLISTS", mcoCheckPointAllAccountLists,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CHECKPOINTDIR",            mcoCheckPointDir,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CHECKPOINTEXPIRATIONTIME", mcoCheckPointExpirationTime,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CHECKPOINTFILE",           mcoCheckPointFile,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CHECKPOINTINTERVAL",       mcoCheckPointInterval,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CHECKPOINTWITHDATABASE",   mcoCheckPointWithDatabase,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CHECKSUSPENDEDJOBPRIORITY", mcoCheckSuspendedJobPriority, mdfString, mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "CHILDSTDERRCHECK",         mcoChildStdErrCheck,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CLASSCAP",                 mcoCCCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "CLASSCFG",                 mcoClassCfg,                  mdfString,  mxoClass, NULL, FALSE, mcoNONE, NULL },
  { "CLASSWEIGHT",              mcoCCWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "CLIENTCFG",                mcoClientCfg,                 mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CLIENTMAXPRIMARYRETRY",    mcoClientMaxPrimaryRetry,     mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CLIENTMAXPRIMARYRETRYTIMEOUT", mcoClientMaxPrimaryRetryTimeout, mdfInt, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CLIENTMAXCONNECTIONS",     mcoClientMaxConnections,      mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CLIENTTIMEOUT",            mcoClientTimeout,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CLIENTUIPORT",             mcoClientUIPort,              mdfInt,     mxoSched,  NULL, FALSE, mcoNONE, NULL },
  { "RECORDEVENTLIST",          mcoRecordEventList,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "REPORTPROFSTATSASCHILD",   mcoReportProfileStatsAsChild, mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "CREDCAP",                  mcoCredCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "CREDWEIGHT",               mcoCredWeight,                mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "CREDDISCOVERY",            mcoCredDiscovery,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DATASTAGEHOLDTYPE",        mcoDataStageHoldType,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MHoldType },
  { "DEADLINECAP",              mcoSDeadlineCap,              mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "DEADLINEPOLICY",           mcoDeadlinePolicy,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEADLINECAP",              mcoSDeadlineCap,              mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "DEADLINEWEIGHT",           mcoSDeadlineWeight,           mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTACCOUNT",           mcoDefaultAccountName,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTCLASSLIST",         mcoDefaultClassList,          mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTDOMAIN",            mcoDefaultDomain,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTJOBOS",             mcoDefaultJobOS,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTQMHOST",            mcoDefaultQMHost,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTRACKSIZE",          mcoDefaultRackSize,           mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTSUBMITLANGUAGE",    mcoDefaultSubmitLanguage,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTTASKMIGRATIONTIME", mcoDefaultTaskMigrationTime,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFAULTVMSOVEREIGN",       mcoDefaultVMSovereign,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DEFERCOUNT",               mcoDeferCount,                mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFERSTARTCOUNT",          mcoDeferStartCount,           mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEFERTIME",                mcoDeferTime,                 mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DEPENDFAILUREPOLICY",      mcoDependFailurePolicy,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DELETESTAGEOUTFILES",      mcoDeleteStageOutFiles,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DIRECTSPECWEIGHT",         mcoOLDDirectSpecWeight,       mdfInt,     mxoPar,   NULL, TRUE,  mcoCredWeight, NULL },
  { "DISABLEBATCHHOLDS",        mcoDisableBatchHolds,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLEEXCHLIST",          mcoDisableExcHList,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLEINTERACTIVEJOBS",   mcoDisableInteractiveJobs,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLEREGEXCACHING",      mcoDisableRegExCaching,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLEREQUIREDGRESNONE",  mcoDisableRequiredGResNone,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "IGNORERMDATASTAGING",      mcoIgnoreRMDataStaging,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLESAMECREDPREEMPTION",mcoDisableSameCredPreemption, mdfString,  mxoSched, NULL, FALSE,  mcoNONE, NULL },
  { "DISABLESAMEQOSPREEMPTION", mcoDisableSameQOSPreemption,  mdfString,  mxoSched, NULL, TRUE,  mcoDisableSameCredPreemption, (char **)MBoolString },
  { "DISABLESCHEDULING",        mcoDisableScheduling,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLESLAVEJOBSUBMIT",    mcoDisableSlaveJobSubmit,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLETHRESHOLDTRIGGERS", mcoDisableThresholdTriggers,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLEUICOMPRESSION",     mcoDisableUICompression,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLEVMDECISIONS",       mcoDisableVMDecisions,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISABLEMULTIREQJOBS",      mcoDisableMultiReqJobs,       mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISKCAP",                  mcoRDiskCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "DISKWEIGHT",               mcoRDiskWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "DISPLAYFLAGS",             mcoDisplayFlags,              mdfString, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "DONTENFORCEPEERJOBLIMITS", mcoDontEnforcePeerJobLimits,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "DISPLAYPROXYUSERASUSER",   mcoDisplayProxyUserAsUser,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "EMULATIONMODE",            mcoEmulationMode,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MRMSubType },
  { "ENABLEJOBARRAYS",          mcoEnableJobArrays,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLECLUSTERMONITOR",     mcoEnableClusterMonitor,      mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLECPJOURNAL",          mcoEnableCPJournal,           mdfString, mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEENCRYPTION",         mcoEnableEncryption,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEFAILUREFORPURGEDJOB",  mcoEnableFailureForPurgedJob,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEFASTSPAWN",          mcoEnableFastSpawn,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEFSVIOLATIONPREEMPTION", mcoEnableFSViolationPreemption, mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },         
  { "ENABLEHIGHTHROUGHPUT",     mcoEnableHighThroughput,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEFASTNODERSV",        mcoEnableFastNodeRsv,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEHPAFFINITYSCHEDULING", mcoEnableHPAffinityScheduling, mdfString, mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEIMPLICITSTAGEOUT",   mcoEnableImplicitStageOut,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEJOBEVALCHECKSTART",  mcoEnableJobEvalCheckStart,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLENEGJOBPRIORITY",     mcoEnableNegJobPriority,      mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLENODEADDRLOOKUP",     mcoEnableNSNodeAddrLookup,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEPOSUSERPRIORITY",    mcoEnablePosUserPriority,     mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLESPVIOLATIONPREEMPTION", mcoEnableSPViolationPreemption, mdfString,mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEUNMUNGECREDCHECK",   mcoEnableUnMungeCredCheck,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEVMDESTROY",          mcoEnableVMDestroy,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENABLEVPCPREEMPTION",      mcoEnableVPCPreemption,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENFORCEACCOUNTACCESS",     mcoEnforceAccountAccess,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "ENFORCEGRESACCESS",        mcoEnforceGRESAccess,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "SUBMITENVFILELOCATION",    mcoSubmitEnvFileLocation,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "EVALMETRICS",              mcoEvalMetrics,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char**)MBoolString },
  { "EVENTFILEROLLDEPTH",       mcoEventFileRollDepth,        mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL }, 
  { "EVENTLOGWSPASSWORD",       mcoEventLogWSPassword,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "EVENTLOGWSURL",            mcoEventLogWSURL,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "EVENTLOGWSUSER",           mcoEventLogWSUser,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "EXTENDEDJOBTRACKING",      mcoExtendedJobTracking,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FALLBACKCLASS",            mcoFallBackClass,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FEATURENODETYPEHEADER",    mcoNodeTypeFeatureHeader,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FEATUREPARTITIONHEADER",   mcoPartitionFeatureHeader,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FEATUREPROCSPEEDHEADER",   mcoProcSpeedFeatureHeader,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FEATURERACKHEADER",        mcoRackFeatureHeader,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FEATURESLOTHEADER",        mcoSlotFeatureHeader,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SPOOLDIRKEEPTIME",         mcoSpoolDirKeepTime,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "GEVENTTIMEOUT",            mcoGEventTimeout,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FEEDBACKPROGRAM",          mcoJobFBAction,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FILEREQUESTISJOBCENTRIC",  mcoFileRequestIsJobCentric,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FILTERCMDFILE",            mcoFilterCmdFile,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FORCENODEREPROVISION",     mcoForceNodeReprovision,      mdfString,  mxoSched, NULL, FALSE,  mcoNONE, (char **)MBoolString },
  { "FORCERSVSUBTYPE",          mcoForceRsvSubType,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FORCETRIGJOBSIDLE",        mcoForceTrigJobsIdle,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FORKITERATIONS",           mcoForkIterations,            mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FSACCOUNTCAP",             mcoFACap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSACCOUNTWEIGHT",          mcoFAWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSACCOUNTWEIGHT",          mcoOLDFSAWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFAWeight , NULL },
  { "FSCAP",                    mcoFSCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSCHARGERATE",             mcoFSChargeRate,              mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FSCLASSCAP",               mcoFCCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSCLASSWEIGHT",            mcoFCWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSCLASSWEIGHT",            mcoOLDFSCWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFCWeight, NULL },
  { "FSCONFIGFILE",             mcoFSConfigFile,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSDECAY",                  mcoFSDecay,                   mdfDouble,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSDEPTH",                  mcoFSDepth,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSENABLECAPPRIORITY",      mcoFSEnableCapPriority,       mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSENFORCEMENT",            mcoFSEnforcement,             mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGACCOUNTCAP",            mcoGFACap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGACCOUNTWEIGHT",         mcoGFAWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGGROUPCAP",              mcoGFGCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGGROUPWEIGHT",           mcoGFGWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGROUPCAP",               mcoFGCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGROUPWEIGHT",            mcoFGWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGROUPWEIGHT",            mcoOLDFSGWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFGWeight, NULL },
  { "FSGUSERCAP",               mcoGFUCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSGUSERWEIGHT",            mcoGFUWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSINTERVAL",               mcoFSInterval,                mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSJPUCAP",                 mcoFJPUCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSJPUWEIGHT",              mcoFJPUWeight,                mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSJRPUCAP",                mcoFJRPUCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSJRPUWEIGHT",             mcoFJRPUWeight,               mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSMOSTSPECIFICLIMIT",      mcoFSMostSpecificLimit,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSPPUCAP",                 mcoFPPUCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSPPUWEIGHT",              mcoFPPUWeight,                mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSPSPUCAP",                 mcoFPSPUCap,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSPSPUWEIGHT",             mcoFPSPUWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSPOLICY",                 mcoFSPolicy,                  mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MFSPolicyType },
  { "FSQOSCAP",                 mcoFQCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSQOSWEIGHT",              mcoFQWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSQOSWEIGHT",              mcoOLDFSQWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFQWeight, NULL },
  { "FSRELATIVEUSAGE",          mcoFSRelativeUsage,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSTARGETISABSOLUTE",       mcoFSIsAbsolute,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSTREE",                   mcoFSTree,                    mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FSTREEACLPOLICY",          mcoFSTreeACLPolicy,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MFSTreeACLPolicy },
  { "FSTREECAPTIERS",           mcoFSTreeCapTiers,            mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSTREEISPROPORTIONAL",     mcoFSTreeIsProportional,      mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSTREEISREQUIRED",         mcoFSTreeIsRequired,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSTREESHARENORMALIZE",     mcoFSTreeShareNormalize,      mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSTREEACCTSHALLOWSEARCH",  mcoFSTreeAcctShallowSearch,   mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "FSTREETIERMULTIPLIER",     mcoFSTreeTierMultiplier,      mdfDouble,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSTREEUSERISREQUIRED",     mcoFSTreeUserIsRequired,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "FSUSERCAP",                mcoFUCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSUSERWEIGHT",             mcoFUWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FSUSERWEIGHT",             mcoOLDFSUWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFUWeight, NULL },
  { "FSWEIGHT",                 mcoFSWeight,                  mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FULLJOBTRIGCP",            mcoFullJobTrigCP,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "GEVENTCFG",                mcoGEventCfg,                 mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },  
  { "GFSWEIGHT",                mcoOLDGFSWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFGWeight, NULL }, 
  { "GMETRICCFG",               mcoGMetricCfg,                mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "GREENPOOLEVALUATIONINTERVAL", mcoGreenPoolEvalInterval,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "GRESCFG",                  mcoGResCfg,                   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "GRESTOJOBATTR",            mcoGResToJobAttrMap,          mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "GROUPCAP",                 mcoCGCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "GROUPCFG",                 mcoGroupCfg,                  mdfString,  mxoGroup, NULL, FALSE, mcoNONE, NULL },
  { "GROUPWEIGHT",              mcoCGWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "GUARANTEEDPREEMPTION",     mcoGuaranteedPreemption,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "HAPOLLINTERVAL",           mcoHAPollInterval,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "HIDEVIRTUALNODES",         mcoHideVirtualNodes,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "IDCFG",                    mcoIDCfg,                     mdfString,  mxoAM,    NULL, FALSE, mcoNONE, NULL },
  { "IGNORECLASSES",            mcoIgnoreClasses,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "IGNOREJOBS",               mcoIgnoreJobs,                mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "IGNORENODES",              mcoIgnoreNodes,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "IGNORELLMAXSTARTERS",      mcoIgnoreLLMaxStarters,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "IGNOREPREEMPTEEPRIORITY",  mcoIgnorePreempteePriority,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "IGNOREUSERS",              mcoIgnoreUsers,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "IMAGECFG",                 mcoImageCfg,                  mdfString,  mxoNode,  NULL, FALSE, mcoNONE, NULL },
  { "INSTANTSTAGE",             mcoInstantStage,              mdfString,  mxoSched, NULL, TRUE,  mcoJobMigratePolicy, (char **)MBoolString },
  { "INTERNALTVARPREFIX",       mcoInternalTVarPrefix,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "INVALIDFSTREEMSG",         mcoInvalidFSTreeMsg,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBACTIONONNODEFAILURE",   mcoJobActionOnNodeFailure,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MARFPolicy },
  { "JOBACTIONONNODEFAILUREDURATION", mcoARFDuration,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBAGGREGATIONTIME",       mcoJobAggregationTime,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBCCODEFAILSAMPLESIZE",   mcoJobCCodeFailSampleSize,    mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBCFG",                   mcoJobCfg,                    mdfString,  mxoJob,   NULL, FALSE, mcoNONE, NULL },
  { "JOBCPURGETIME",            mcoJobCPurgeTime,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBCTRUNCATENLCP",         mcoJobCTruncateNLCP,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "JOBEXTENDSTARTWALLTIME",   mcoJobExtendStartWallTime,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "JOBFAILRETRYCOUNT",        mcoJobFailRetryCount,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBIDCAP",                 mcoAJobIDCap,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBIDWEIGHT",              mcoAJobIDWeight,              mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBMATCHCFG",              mcoJobMatchCfg,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBRSVRETRYINTERVAL",      mcoJobRsvRetryInterval,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBMAXHOLDTIME",           mcoMaxJobHoldTime,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBMAXOVERRUN",            mcoJobMaxOverrun,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBMAXNODECOUNT",          mcoJobMaxNodeCount,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBMAXTASKCOUNT",          mcoJobMaxTaskCount,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBMAXSTARTPERITERATION",  mcoMaxJobStartPerIteration,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBMAXPREEMPTPERITERATION",mcoMaxJobPreemptPerIteration, mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBMAXSTARTTIME",          mcoMaxJobStartTime,           mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBMIGRATEPOLICY",         mcoJobMigratePolicy,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MJobMigratePolicy },
  { "JOBNAMECAP",               mcoAJobNameCap,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBNAMEWEIGHT",            mcoAJobNameWeight,            mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBNODEMATCHPOLICY",       mcoJobNodeMatch,              mdfStringArray, mxoPar, NULL, FALSE, mcoNONE, (char **)MJobNodeMatchType },
  { "JOBPREEMPTCOMPLETIONTIME", mcoJobPreemptCompletionTime,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBPREEMPTMAXACTIVETIME",  mcoJobPreemptMaxActiveTime,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBPREEMPTMINACTIVETIME",  mcoJobPreemptMinActiveTime,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBPRIOACCRUALPOLICY",     mcoJobPrioAccrualPolicy,      mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBPRIOEXCEPTIONS",        mcoJobPrioExceptions,         mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBPRIOF",                 mcoJobAttrPrioF,              mdfStringArray, mxoPar, NULL, FALSE, mcoNONE, NULL },
  { "JOBPURGETIME",             mcoJobPurgeTime,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBREJECTPOLICY",          mcoJobRejectPolicy,           mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, (char **)MJobRejectPolicy },
  { "JOBREMOVEENVVARLIST",      mcoJobRemoveEnvVarList,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBRETRYTIME",             mcoJobRetryTime,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "JOBSTARTLOGLEVEL",         mcoJobStartLogLevel,          mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "JOBSYNCTIME",              mcoJobSyncDeadline,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LICENSECHARGERATE",        mcoLicenseChargeRate,         mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LIMITEDJOBCP",             mcoLimitedJobCP,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char**)MBoolString },
  { "LIMITEDNODECP",            mcoLimitedNodeCP,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char**)MBoolString },
  { "LOADALLJOBCP",             mcoLoadAllJobCP,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char**)MBoolString },
  { "LOCKFILE",                 mcoSchedLockFile,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGDIR",                   mcoLogDir,                    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGFACILITY",              mcoLogFacility,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGFILEMAXSIZE",           mcoLogFileMaxSize,            mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGFILE",                  mcoLogFile,                   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGFILEROLLDEPTH",         mcoLogFileRollDepth,          mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGLEVEL",                 mcoLogLevel,                  mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGPERMISSIONS",           mcoLogPermissions,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGLEVELOVERRIDE",         mcoLogLevelOverride,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "LOGROLLACTION",            mcoLogRollAction,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAILPROGRAM",              mcoMailAction,                mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAPFEATURETOPARTITION",    mcoMapFeatureToPartition,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "MAXACL",                   mcoMaxACL,                    mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXDEPENDS",               mcoMaxDepends,                mdfInt,     mxoSched, NULL, FALSE ,mcoNONE, NULL },
  { "MAXGMETRIC",               mcoMaxGMetric,                mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXGREENSTANDBYPOOLSIZE",  mcoMaxGreenStandbyPoolSize,   mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXGRES",                  mcoMaxGRes,                   mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXJOB",                   mcoMaxJob,                    mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXNODE",                  mcoMaxNode,                   mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXRSV",                   mcoMaxRsv,                    mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXRSVPERNODE",            mcoMaxRsvPerNode,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXSLEEPITERATION",        mcoMaxSleepIteration,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MAXSUSPENDTIME",           mcoMaxSuspendTime,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MCSOCKETPROTOCOL",         mcoMCSocketProtocol,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MDIAGXMLJOBNOTES",         mcoMDiagXmlJobNotes,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char**)MBoolString },
  { "MEMCAP",                   mcoRMemCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "MEMREFRESHINTERVAL",       mcoMemRefreshInterval,        mdfString,  mxoSched, NULL, TRUE,  mcoRestartInterval, NULL },
  { "MEMWEIGHT",                mcoRMemWeight,                mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "METAMAXTASKS",             mcoMaxMetaTasks,              mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MINADMINSTIME",            mcoAdminMinSTime,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MINDISPATCHTIME",          mcoMinDispatchTime,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MISSINGDEPENDENCYACTION",  mcoMissingDependencyAction,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MONGOPASSWORD",            mcoMongoPassword,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL }, 
  { "MONGOSERVER",              mcoMongoServer,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL }, 
  { "MONGOUSER",                mcoMongoUser,                 mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL }, 
  { "MSUBQUERYINTERVAL",        mcoMSubQueryInterval,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NAMITRANSVARNAME",         mcoNAMITransVarName,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NESTEDNODESET",            mcoNestedNodeSet,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NETWORKCHARGERATE",        mcoNetworkChargeRate,         mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEACCESSPOLICY",         mcoNAPolicy,                  mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MNAccessPolicy },
  { "NODEACTIVELIST",           mcoNodeActiveList,            mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEALLOCATIONPOLICY",     mcoNodeAllocationPolicy,      mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MNAllocPolicy },
  { "NODEALLOCRESFAILUREPOLICY", mcoNodeAllocResFailurePolicy, mdfString, mxoPar,   NULL, FALSE, mcoNONE, (char **)MARFPolicy },
  { "NODEAVAILABILITYPOLICY",   mcoNodeAvailPolicy,           mdfStringArray, mxoPar, NULL, FALSE, mcoNONE, (char **)MNAvailPolicy },
  { "NODECAP",                  mcoRNodeCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "NODECATCREDLIST",          mcoNodeCatCredList,           mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODECFG",                  mcoNodeCfg,                   mdfString,  mxoNode,  NULL, FALSE, mcoNONE, NULL },
  { "NODEBUSYSTATEDELAYTIME",   mcoNodeBusyStateDelayTime,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },    
  { "NODECPUOVERCOMMITFACTOR",  mcoNodeCPUOverCommitFactor,   mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL }, 
  { "NODEDOWNSTATEDELAYTIME",   mcoNodeDownStateDelayTime,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEDOWNTIME",             mcoNodeDownTime,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEDRAINSTATEDELAYTIME",  mcoNodeDrainStateDelayTime,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEFAILURERESERVETIME",   mcoNodeFailureReserveTime,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEIDFORMAT",             mcoNodeIDFormat,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEIDLEPOWERTHRESHOLD",   mcoNodeIdlePowerThreshold,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },   /* the amount of time to allow a node to remain idle before powering it off (on demand provisioning model) */
  { "NODEMAXLOAD",              mcoNodeMaxLoad,               mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEMEMOVERCOMMITFACTOR",  mcoNodeMemOverCommitFactor,   mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODENAMECASEINSENSITIVE",  mcoNodeNameCaseInsensitive,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "NODEPOLLFREQUENCY",        mcoNodePollFrequency,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEPURGETIME",            mcoNodePurgeTime,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODESETATTRIBUTE",         mcoNodeSetAttribute,          mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MResSetAttrType },
  { "NODESETDELAY",             mcoNodeSetDelay,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "NODESETFORCEMINALLOC",     mcoNodeSetForceMinAlloc,      mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "NODESETISOPTIONAL",        mcoNodeSetIsOptional,         mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "NODESETLIST",              mcoNodeSetList,               mdfStringArray, mxoPar, NULL, FALSE, mcoNONE, NULL },
  { "NODESETMAXUSAGE",          mcoNodeSetMaxUsage,           mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODESETPLUS",              mcoNodeSetPlus,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MNodeSetPlusPolicy },
  { "NODESETPOLICY",            mcoNodeSetPolicy,             mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MResSetSelectionType },
  { "NODESETPRIORITYTYPE",      mcoNodeSetPriorityType,       mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MResSetPrioType },
  { "NODESETTOLERANCE",         mcoNodeSetTolerance,          mdfDouble,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "NODESYNCTIME",             mcoNodeSyncDeadline,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODETOJOBATTRMAP",         mcoNodeToJobFeatureMap,       mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEUNTRACKEDLOADFACTOR",  mcoNodeUntrackedProcFactor,   mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEUNTRACKEDRESDELAYTIME",mcoNodeUntrackedResDelayTime, mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NONBLOCKINGCACHEINTERVAL", mcoNonBlockingCacheInterval,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NODEWEIGHT",               mcoRNodeWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "NOJOBHOLDNORESOURCES",     mcoNoJobHoldNoResources,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char**)MBoolString },
  { "NOLOCALUSERENV",           mcoNoLocalUserEnv,            mdfString,  mxoSched, NULL, TRUE,  mcoRMCfg, (char **)MBoolString },
  { "NOTIFICATIONINTERVAL",     mcoAdminEInterval,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NOTIFICATIONPROGRAM",      mcoAdminEAction,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "NOWAITPREEMPTION",         mcoNoWaitPreemption,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "OBJECTELIST",              mcoObjectEList,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MOExpSearchOrder },
  { "OPTIMIZEDCHECKPOINTING",   mcoOptimizedCheckpointing,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "OPTIMIZEDJOBARRAYS",       mcoOptimizedJobArrays,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "OSCREDLOOKUP",             mcoOSCredLookup,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MActionPolicy },
  { "PARADMIN",                 mcoParAdmin,                  mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PARALLOCATIONPOLICY",      mcoParAllocationPolicy,       mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MPAllocPolicy },
  { "PARCFG",                   mcoParCfg,                    mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PARIGNQUEUELIST",          mcoParIgnQList,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PBSACCOUNTINGDIR",         mcoPBSAccountingDir,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PERPARTITIONSCHEDULING",   mcoPerPartitionScheduling,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "PECAP",                    mcoRPECap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PEWEIGHT",                 mcoRPEWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PREEMPTJOBCOUNT",          mcoPreemptJobCount,           mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PREEMPTPOLICY",            mcoPreemptPolicy,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MPreemptPolicy },
  { "PREEMPTPRIOJOBSELECTWEIGHT", mcoPreemptPrioJobSelectWeight, mdfDouble, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PREEMPTRTIMEWEIGHT",       mcoPreemptRTimeWeight,        mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PREEMPTSEARCHDEPTH",       mcoPreemptSearchDepth,        mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PRIORITYTARGETDURATION",   mcoPriorityTargetDuration,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PRIORITYTARGETPROCCOUNT",  mcoPriorityTargetProcCount,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PROCCAP",                  mcoRProcCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PROCWEIGHT",               mcoRProcWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PROFILECOUNT",             mcoStatProfCount,             mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PROFILEDURATION",          mcoStatProfDuration,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PROFILEGRANULARITY",       mcoStatProfGranularity,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "PSCAP",                    mcoRPSCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PSWEIGHT",                 mcoRPSWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "PURGETIME",                mcoOLDJobPurgeTime,           mdfString,  mxoSched, NULL, TRUE,  mcoJobPurgeTime, NULL },
  { "PUSHCACHETOWEBSERVICE",    mcoPushCacheToWebService,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "PUSHEVENTSTOWEBSERVICE",   mcoPushEventsToWebService,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "QOSCAP",                   mcoCQCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "QOSCFG",                   mcoQOSCfg,                    mdfString,  mxoQOS,   NULL, FALSE, mcoNONE, NULL },
  { "QOSDEFAULTORDER",          mcoQOSDefaultOrder,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "QOSISOPTIONAL",            mcoQOSIsOptional,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "QOSREJECTPOLICY",          mcoQOSRejectPolicy,           mdfStringArray,  mxoSched, NULL, FALSE, mcoNONE, (char **)MJobRejectPolicy },
  { "QOSWEIGHT",                mcoCQWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "QUEUECFG",                 mcoQueueCfg,                  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "QUEUENAMIACTIONS",         mcoQueueNAMIActions,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "QUEUETIMECAP",             mcoSQTCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "QUEUETIMETARGETWEIGHT",    mcoOLDQTWeight,               mdfInt,     mxoPar,   NULL, TRUE,  mcoSQTWeight, NULL },
  { "QUEUETIMEWEIGHT",          mcoSQTWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL }, 
  { "RANGEEVALPOLICY",          mcoRangeEvalPolicy,           mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "REALTIMEDBOBJECTS",        mcoRealTimeDBObjects,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "REGISTRATIONURL",          mcoRegistrationURL,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "REJECTDOSSCRIPTS",         mcoRejectDosScripts,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "REJECTINFEASIBLEJOBS",     mcoRejectInfeasibleJobs,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "REJECTDUPLICATEJOBS",      mcoRejectDuplicateJobs,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "REJECTNEGPRIOJOBS",        mcoRejectNegPrioJobs,         mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "RELATIVETOABSOLUTEPATHS",  mcoRelToAbsPaths,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "REMAPCLASS",               mcoRemapClass,                mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "REMAPCLASSLIST",           mcoRemapClassList,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "REMOTEFAILTRANSIENT",      mcoRemoteFailTransient,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "REMOTESTARTCOMMAND",       mcoRemoteStartCommand,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "REPORTPEERSTARTINFO",      mcoReportPeerStartInfo,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "REQUEUERECOVERYRATIO",     mcoRequeueRecoveryRatio,      mdfDouble,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RESCAP",                   mcoResCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "RESDEPTH",                 mcoOLDMaxRsvPerNode,          mdfInt,     mxoSched, NULL, FALSE, mcoMaxRsvPerNode, NULL },
  { "RESENDCANCEL",             mcoResendCancelCommand,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "RESERVATIONDEPTH",         mcoRsvBucketDepth,            mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RESERVATIONPOLICY",        mcoRsvPolicy,                 mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MRsvPolicy },
  { "RESERVATIONQOSLIST",       mcoRsvBucketQOSList,          mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RESERVATIONRETRYTIME",     mcoRsvRetryTime,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "RESERVATIONTHRESHOLDTYPE", mcoRsvThresholdType,          mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "RESERVATIONTHRESHOLDVALUE", mcoRsvThresholdValue,        mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "RESTARTINTERVAL",          mcoRestartInterval,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RSVREALLOCPOLICY",         mcoRsvReallocPolicy,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MRsvAllocPolicy },
  { "RSVLIMITPOLICY",           mcoRsvLimitPolicy,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },  
  { "REMOVETRIGOUTPUTFILES",    mcoRemoveTrigOutputFiles,     mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "RESOURCELIMITMULTIPLIER",  mcoResourceLimitMultiplier,   mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "RESOURCELIMITPOLICY",      mcoResourceLimitPolicy,       mdfStringArray, mxoPar, NULL, FALSE, mcoNONE, NULL },
  { "RESOURCEQUERYDEPTH",       mcoResourceQueryDepth,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RESWEIGHT",                mcoResWeight,                 mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "RMCFG",                    mcoRMCfg,                     mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RMMSGIGNORE",              mcoRMMsgIgnore,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RMPOLLINTERVAL",           mcoRMPollInterval,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RMRETRYTIMECAP",           mcoRMRetryTimeCap,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RMSERVER",                 mcoOLDRMServer,               mdfString,  mxoRM,    NULL, TRUE,  mcoRMCfg, NULL },
  { "RMTIMEOUT",                mcoOLDRMTimeOut,              mdfString,  mxoSched, NULL, TRUE,  mcoRMCfg, NULL },
  { "RMTYPE",                   mcoOLDRMType,                 mdfString,  mxoSched, NULL, TRUE,  mcoRMCfg, NULL },
  { "RSVNODEALLOCATIONPOLICY",  mcoRsvNodeAllocPolicy,        mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MNAllocPolicy },
  { "RSVNODEALLOCATIONPRIORITYF",  mcoRsvNodeAllocPriorityF,  mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "FREETIMELOOKAHEADDURATION", mcoFreeTimeLookAheadDuration, mdfString, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RSVPROFILE",               mcoRsvProfile,                mdfString,  mxoSRsv,  NULL, FALSE, mcoNONE, NULL },
  { "RSVPURGETIME",             mcoRsvPurgeTime,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "RSVSEARCHALGO",            mcoRsvSearchAlgo,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MRsvSearchAlgo },
  { "RSVTIMEDEPTH",             mcoRsvTimeDepth,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SCHEDCFG",                 mcoSchedCfg,                  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SCHEDPOLICY",              mcoSchedPolicy,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MSchedPolicy },
  { "SERVERCONFIGFILE",         mcoSchedConfigFile,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SERVERHOMEDIR",            mcoMServerHomeDir,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SERVERHOST",               mcoServerHost,                mdfString,  mxoSched, NULL, TRUE,  mcoSchedCfg, NULL },
  { "SERVERMODE",               mcoSchedMode,                 mdfString,  mxoSched, NULL, TRUE,  mcoSchedCfg, NULL },
  { "SERVERNAME",               mcoServerName,                mdfString,  mxoSched, NULL, TRUE,  mcoSchedCfg, NULL },
  { "SERVERPORT",               mcoServerPort,                mdfInt,     mxoSched, NULL, TRUE,  mcoSchedCfg, NULL },
  { "SERVICETARGETWEIGHT",      mcoOLDServWeight,             mdfInt,     mxoPar,   NULL, TRUE,  mcoTargWeight, NULL },
  { "SERVICECAP",               mcoServCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SERVICEWEIGHT",            mcoServWeight,                mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SHOWMIGRATEDJOBSASIDLE",   mcoShowMigratedJobsAsIdle,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SIDEBYSIDE",               mcoSideBySide,                mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SIMAUTOSHUTDOWN",          mcoSimAutoShutdown,           mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "SIMCHECKPOINTINTERVAL",    mcoSimCheckpointInterval,     mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "SIMFLAGS",                 mcoSimFlags,                  mdfStringArray, mxoxSim, NULL, FALSE, mcoNONE, NULL },
  { "SIMINITIALQUEUEDEPTH",     mcoSimInitialQueueDepth,      mdfInt,     mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "SIMJOBSUBMISSIONPOLICY",   mcoSimJobSubmissionPolicy,    mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, (char **)MSimQPolicy },
  { "SIMPURGEBLOCKEDJOBS",      mcoSimPurgeBlockedJobs,       mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "SIMRANDOMIZEJOBSIZE",      mcoSimRandomizeJobSize,       mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "SIMSTARTTIME",             mcoSimStartTime,              mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "SIMTIMEPOLICY",            mcoSimTimePolicy,             mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, (char **)MTimePolicy },
  { "SIMTIMERATIO",             mcoSimTimeRatio,              mdfDouble,  mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "SIMWORKLOADTRACEFILE",     mcoSimWorkloadTraceFile,      mdfString,  mxoxSim,  NULL, FALSE, mcoNONE, NULL },
  { "STOPITERATION",            mcoStopIteration,             mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SOCKETEXPLICITNONBLOCK",   mcoSocketExplicitNonBlock,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "SOCKETLINGERVALUE",        mcoSocketLingerVal,           mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SOCKETWAITTIME",           mcoSocketWaitTime,            mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SPOOLDIR",                 mcoSpoolDir,                  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SPVIOLATIONCAP",           mcoSSPVCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SPVIOLATIONWEIGHT",        mcoSSPVWeight,                mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SRCFG",                    mcoSRCfg,                     mdfString,  mxoSRsv,  NULL, FALSE, mcoNONE, NULL },
  { "SRHOSTLIST",               mcoOLDSRHostList,             mdfString,  mxoSched, NULL, TRUE,  mcoSRCfg, NULL },
  { "STARTCOUNTCAP",            mcoSStartCountCap,            mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "STARTCOUNTWEIGHT",         mcoSStartCountWeight,         mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "STATDIR",                  mcoStatDir,                   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATPROCMAX",              mcoStatProcMax,               mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATPROCMIN",              mcoStatProcMin,               mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATPROCSTEPCOUNT",        mcoStatProcStepCount,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATPROCSTEPSIZE",         mcoStatProcStepSize,          mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATTIMEMAX",              mcoStatTimeMax,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATTIMEMIN",              mcoStatTimeMin,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATTIMESTEPCOUNT",        mcoStatTimeStepCount,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STATTIMESTEPSIZE",         mcoStatTimeStepSize,          mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STRICTPROTOCOLCHECK",      mcoStrictProtocolCheck,       mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "STEPCOUNT",                mcoSchedStepCount,            mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SUBMITHOSTS",              mcoSubmitHosts,               mdfStringArray, mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "STOREJOBSUBMISSION",       mcoStoreSubmission,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SUBMITFILTER",             mcoSubmitFilter,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SERVERSUBMITFILTER",       mcoServerSubmitFilter,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "UNSUPPORTEDDEPENDENCIES",  mcoUnsupportedDependencies,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "SUSPENDRESOURCES",         mcoSuspendRes,                mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SWAPCAP",                  mcoRSwapCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SWAPWEIGHT",               mcoRSwapWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SYSCFG",                   mcoSysCfg,                    mdfString,  mxoSys,   NULL, FALSE, mcoNONE, NULL },
  { "SYSTEMMAXJOBWALLTIME",     mcoSystemMaxJobTime,          mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SYSTEMMAXPROCPERJOB",      mcoSystemMaxJobProc,          mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SYSTEMMAXPROCSECONDPERJOB", mcoSystemMaxJobPS,           mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "SYSTEMMAXSECONDPERJOB",    mcoSystemMaxJobSecond,        mdfString,  mxoPar,   NULL, TRUE,  mcoSystemMaxJobTime, NULL },
  { "TARGETCAP",                mcoTargCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "TARGETQUEUETIMECAP",       mcoTQTCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "TARGETQUEUETIMEWEIGHT",    mcoTQTWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "TARGETWEIGHT",             mcoTargWeight,                mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "TARGETXFACTORCAP",         mcoTXFCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "TARGETXFACTORWEIGHT",      mcoTXFWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "TASKDISTRIBUTIONPOLICY",   mcoTaskDistributionPolicy,    mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MTaskDistributionPolicy },
  { "THREADPOOLSIZE",           mcoThreadPoolSize,            mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "TOOLSDIR",                 mcoSchedToolsDir,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "TRACKSUSPENDEDJOBUSAGE",   mcoTrackSuspendedJobUsage,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "TRACKUSERIO",              mcoTrackUserIO,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "MOUNTPOINTCHECKINTERVAL",  mcoMountPointCheckInterval,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "MOUNTPOINTCHECKTHRESHOLD", mcoMountPointCheckThreshold,  mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "TRIGCHECKTIME",            mcoTrigCheckTime,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "TRIGEVALLIMIT",            mcoTrigEvalLimit,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "TRIGFLAGS",                mcoTrigFlags,                 mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "UFSWEIGHT",                mcoOLDUFSWeight,              mdfInt,     mxoPar,   NULL, TRUE,  mcoFUWeight, NULL },
  { "UMASK",                    mcoUMask,                     mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "USAGECAP",                 mcoUsageCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGECONSUMEDCAP",         mcoUConsCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGECONSUMEDWEIGHT",      mcoUConsWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGEPERCENTCAP",          mcoUPerCCap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGEPERCENTWEIGHT",       mcoUPerCWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGEREMAININGCAP",        mcoURemCap,                   mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGEREMAININGWEIGHT",     mcoURemWeight,                mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGEEXECUTIONTIMECAP",    mcoUExeTimeCap,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGEEXECUTIONTIMEWEIGHT", mcoUExeTimeWeight,            mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USAGEWEIGHT",              mcoUsageWeight,               mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USEANYPARTITIONPRIO",      mcoUseAnyPartitionPrio,       mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USECPRSVNODELIST",         mcoUseCPRsvNodeList,          mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USECPUTIME",               mcoUseCPUTime,                mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USEFLOATINGNODELIMITS",    mcoUseFloatingNodeLimits,     mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USEJOBREGEX",              mcoUseJobRegEx,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USELOCALMACHINEPRIORITY",  mcoUseLocalMachinePriority,   mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USEMACHINESPEED",          mcoUseMachineSpeed,           mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USEMOABJOBID",             mcoUseMoabJobID,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USENODEHASH",              mcoUseNodeHash,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "USERCAP",                  mcoCUCap,                     mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USERCFG",                  mcoUserCfg,                   mdfString,  mxoUser,  NULL, FALSE, mcoNONE, NULL },
  { "USERPRIOCAP",              mcoSUPrioCap,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USERPRIOWEIGHT",           mcoSUPrioWeight,              mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USERWEIGHT",               mcoCUWeight,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USESYSLOG",                mcoUseSyslog,                 mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "USEMOABCTIME",             mcoUseMoabCTime,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "USESYSTEMQUEUETIME",       mcoUseSystemQueueTime,        mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "USEUSERHASH",              mcoUseUserHash,               mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "VADDRESSLIST",             mcoVAddressList,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMCALCULATELOADBYVMSUM",   mcoCalculateLoadByVMSum,      mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "VMCPURGETIME",             mcoVMCPurgeTime,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMCREATETHROTTLE",         mcoVMCreateThrottle,          mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMMIGRATETHROTTLE",        mcoVMMigrateThrottle,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMGRESMAP",                mcoVMGResMap,                 mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MTrueString },
  { "VMMIGRATETOZEROLOADNODES", mcoMigrateToZeroLoadNodes,    mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "VMMIGRATIONPOLICY",        mcoVMMigrationPolicy,         mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "VMMINOPDELAY",             mcoVMMinOpDelay,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "VMOCTHRESHOLD",            mcoVMOvercommitThreshold,     mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },  /* deprecate */
  { "VMPROVISIONSTATUSREADYVALUE",mcoVMProvisionStatusReady,  mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMSARESTATIC",             mcoVMsAreStatic,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "VMCFG",                    mcoVMCfg,                     mdfString,  mxoxVM,   NULL, FALSE, mcoNONE, NULL },
  { "VMSTALEITERATIONS",        mcoVMStaleIterations,         mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMSTALEACTION",            mcoVMStaleAction,             mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MVMStaleAction },
  { "VMSTORAGENODETHRESHOLD",   mcoVMStorageNodeThreshold,    mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMSTORAGEMOUNTDIR",        mcoVMStorageMountDir,         mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VMTRACKING",               mcoVMTracking,                mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "VCPROFILE",                mcoVCProfile,                 mdfString,  mxoCluster, NULL, FALSE, mcoNONE, NULL },
  { "VPCMIGRATIONPOLICY",       mcoVPCMigrationPolicy,        mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "VPCNODEALLOCATIONPOLICY",  mcoVPCNodeAllocationPolicy,   mdfString,  mxoPar,   NULL, FALSE, mcoNONE, (char **)MNAllocPolicy },
  { "WALLTIMECAP",              mcoRWallTimeCap,              mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL }, 
  { "WALLTIMEWEIGHT",           mcoRWallTimeWeight,           mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "WATTCHARGERATE",           mcoWattChargeRate,            mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "WCACCURACYCAP",            mcoFUWCACap,                  mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "WCACCURACYWEIGHT",         mcoFUWCAWeight,               mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "WCVIOLATIONACTION",        mcoWCViolAction,              mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "WCVIOLATIONCCODE",         mcoWCViolCCode,               mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "WEBSERVICESURL",           mcoWebServicesUrl,            mdfString,  mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "WIKIEVENTS",               mcoWikiEvents,                mdfString,  mxoSched, NULL, FALSE, mcoNONE, (char **)MBoolString },
  { "WIKISUBMITTIMEOUT",        mcoMWikiSubmitTimeout,        mdfInt,     mxoSched, NULL, FALSE, mcoNONE, NULL },
  { "XFACTORCAP",               mcoSXFCap,                    mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "XFACTORTARGETWEIGHT",      mcoOLDXFWeight,               mdfInt,     mxoPar,   NULL, TRUE,  mcoTXFWeight, NULL },
  { "XFACTORWEIGHT",            mcoSXFWeight,                 mdfInt,     mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { "XFMINWCLIMIT",             mcoXFMinWCLimit,              mdfString,  mxoPar,   NULL, FALSE, mcoNONE, NULL },
  { NULL,                       mcoNONE,                      mdfString,  mxoNONE,  NULL, FALSE, mcoNONE, NULL } };



/* sync w/enum MRoleEnum */

const char *MRoleType[] = {
  NONE,
  "admin1",
  "admin2",
  "admin3",
  "admin4",
  "admin5",
  "owner",
  "granted",
  "template",
  NULL };


/* rsvctl polices */

const char *MRsvCtlPolicy[] = {
  NONE,
  "ADMINONLY",
  "ANY",
  NULL };



/* resource types */

/* NOTE:  sync with enum MResourceTypeEnum */

const char *MResourceType[] = {
  NONE,
  "NODE",
  "PROC",
  "MEM",
  "SWAP",
  "DISK",
  "AMEM",
  "ASWAP",
  "DPROCS",
  NULL };


/* NOTE:  sync with enum MResLimitTypeEnum */

const char *MResLimitType[] = {
  NONE,
  "JOBMEM",
  "JOBPROC",
  "NODE",
  "PROC",
  "MEM",
  "SWAP",
  "DISK",
  "WALLTIME",
  "CPUTIME",
  "MINJOBPROC",
  "TEMP",
  "GMETRIC",
  NULL };


/* sync w/enum MFormatModeEnum */

const char *MFormatMode[] = {
  NONE, 
  "human",
  "http",
  "xml",
  "avp",
  "verbose",
  NULL };


/* sync w/enum MObjectSetModeEnum, MObjOpString[] */

const char *MObjOpType[] = {
  NONE,
  "verify",
  "set",
  "setOnEmpty",
  "add",
  "clear",
  "unset",
  "decr",
  NULL };

const char *MObjOpString[] = {
  "<!=>",
  "<!=>",
  "==",
  "<!=>",
  "+=",
  "<!=>",
  "<!=>",
  "-=",
  NULL };

/* sync w/enum MBaseProtocolEnum */

const char *MBaseProtocol[] = {
  NONE,
  "exec",
  "file",
  "ganglia",
  "http",
  "noop",
  NULL };


/* sync w/enum MHVTypeEnum */

const char *MHVType[] = {
  NONE,
  "HyperV",
  "ESX",
  "ESX35",
  "ESX35i",
  "ESX40",
  "ESX40i",
  "KVM",
  "LinuxContainer",
  "XEN",
  NULL };


/* sync w/enum MExportPolicyEnum[] */

const char *MExportPolicy[] = {
  NONE,
  "control",
  "execution",
  "info",
  NULL };



/* sync w/enum MDSProtoEnum */
                                                                                
const char *MDSProtocol[] = {
  NONE,
  "ftp",
  "gass",   /* https protocol at specfic port */
  "gpfs",
  "gsiftp", /* gridftp */
  "http",
  "https",
  "nfs",
  "pvfs",
  "scp",
  NULL };


/**
 * attribute object types 
 *
 * @see enum MAttrEnum (sync) 
 * @see MHRObj[] (sync)
 */

const char *MAttrO[] = {
  "NONE",
  "JOB",
  "NODE",
  "RSV",
  "USER",
  "GROUP",
  "ACCT",
  "PAR",
  "SCHED",
  "SYSTEM",
  "CLASS",
  "QOS",
  "JATTR",
  "JPRIORITY",
  "JTEMPLATE",
  "DURATION",
  "TASK",
  "PROC",
  "PS",
  "RACK",
  "QUEUE",
  "CLUSTER",
  "QTIME",
  "XFACTOR",
  "MEMORY",
  "VC",
  NULL };


/* NOTE:  MXO must be sync'd with MXOC, MCredCfgParm[], enum MXMLOTypeEnum */

/* WARNING!!!ALERT!!! Adding or changing anything here needs to be synced with the
                      create.db.all.sql script and should only happen during major
                      revisions. */



const char *MXO[] = {
  NONE,
  "acct",
  "am",
  "class",
  "cluster",
  "group",
  "job",
  "node",
  "par",
  "qos",
  "queue",
  "rack",
  "req",
  "rsv",
  "rm",
  "sched",
  "srsv",
  "sys",
  "tnode",
  "trig",
  "user",
  "ALL",
  "acl",
  "cjob",
  "cp",
  "depend",
  "event",
  "fs",
  "fsc",
  "green",
  "gres",
  "gmetric",
  "hybrid",
  "image",
  "limits",
  "nodefeature",
  "priority",
  "range",
  "sim",
  "stats",
  "task",
  "tasksperjob",
  "tjob",
  "paction",
  "tx",
  "vm",
  "vmcr",
  "vpc",
  "jgroup", /* short to avoid substring matches */
  "trans",
  "vc",
  NULL };


/* WARNING!!!ALERT!!! Adding or changing anything here needs to be synced with the
                      create.db.all.sql script and should only happen during major
                      revisions. */



const char *MXOC[] = {
  NONE,
  "ACCT",
  "AM",
  "CLASS",
  "CLUSTER",
  "GROUP",
  "JOB",
  "NODE",
  "PAR",
  "QOS",
  "QUEUE",
  "RACK",
  "REQ",
  "RSV",
  "RM",
  "SCHED",
  "SRSV",
  "SYS",
  "TNODE",
  "TRIG",
  "USER",
  "ALL",
  "ACL",
  "CJOB",
  "CP",
  "DEPEND",
  "EVENT",
  "FS",
  "FSC",
  "GREEN",
  "GRES",
  "GMETRIC",
  "IMAGE",
  "LIMITS",
  "NODEFEATURE",
  "PRIORITY",
  "RANGE",
  "SIM",
  "STATS",
  "TASK",
  "TASKSPERJOB",
  "TJOB",
  "PACTION",
  "TX",
  "VM",
  "VMCR",
  "VPC",
  "JGROUP", /* short to avoid substring matches */
  "TRANS",
  "VC",
  NULL };


/* human readable object (sync w/enum MAttrEnum) */

const char *MHRObj[] = {
  "NONE",
  "job",
  "node",
  "reservation",
  "user",
  "group",
  "account",
  "partition",
  "scheduler",
  "system",
  "class",
  "QOS",
  "jobfeature",
  "jobpriority",
  "jobtemplate",
  "duration",
  "task",
  "proc",
  "procsecond",
  "rack",
  "queue",
  "cluster",
  "queuetime",
  "xfactor",
  "memory",
  "vc",
  NULL };



/* sync w/enum MPrioSubComponentEnum */

const char *MPrioSCName[] = {
  NONE,
  "queuetime",
  "xfactor",
  "deadline",
  "policyviolation",
  "userprior",
  "startcount",
  "bypass",
  "queuetime",
  "xfactor",
  "user",
  "group",
  "account",
  "qos",
  "class",
  "user",
  "group",
  "account",
  "qos",
  "class",
  "guser",
  "ggroup",
  "gaccount",
  "userwcacc",
  "jobsperuser",
  "jobsrunningperuser",
  "procsperuser",
  "psperuser",
  "attribute",
  "gres",
  "jobid",
  "jobname",
  "state",
  "node",
  "proc",
  "memory",
  "swap",
  "disk",
  "procsecond",
  "procequivalent",
  "walltime",
  "consumed",
  "remaining",
  "percentconsumed",
  "executiontime",
  NULL };


const char *MPrioCName[] = {
  NONE,
  "service",
  "target",
  "credential",
  "attribute",
  "fairshare",
  "resource",
  "usage",
  NULL };


/* sync w/enum MSchedStatTypeEnum */

const char *MSchedStatType[] = {
  NONE,
  "usage",
  "fairshare",
  "all",
  NULL };

#ifdef __cplusplus
extern "C"
#endif
const int MPSCStart[] = { 
  mpsNONE,
  mpsSQT, 
  mpsTQT,
  mpsCU,
  mpsAAttr,
  mpsFU,
  mpsRNode,
  mpsUCons };

#ifdef __cplusplus
extern "C"
#endif
const int MPSCEnd[] = { 
  mpsNONE,
  mpsSBP, 
  mpsTXF,
  mpsCC,
  mpsAState,
  mpsFPSPU,
  mpsRWallTime,
  mpsUExeTime };

const char *MWeekDay[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL};
const char *MWEEKDAY[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", NULL};

const char *MResModeS[] = {
  "InclusiveShared",
  "ExclusiveShared",
  "ExclusivePrivate",
  "Ignored",
  NULL };


/* callback type */

const char *MCBType[] = {
  "RESCANCEL",
  "RESSTART",
  "RESEND",
  "JOBSTART",
  "JOBEND",
  "JOBSTAGE",
  NULL };


/* sync w/enum MProfAttrEnum */

const char *MProfAttr[] = {
  NONE,
  "Count",
  "Duration",
  "MaxCount",
  "StartTime",
  NULL };



/* sync w/enum MRMAuthEnum */
        
const char *MRMAuthType[] = { 
  "NONE", 
  "CHECKSUM", 
  "OTHER",
  "PKI", 
  "SECUREPORT", 
  NULL };


/* sync w/enum MWCmdEnum */

const char *MWMCommandList[] = {
  "NONE",
  "SUBMITJOB",
  "GETJOBS",
  "GETNODES",
  "UPDATEJOB",
  "STARTJOB",
  "CANCELJOB",
  "SUSPENDJOB",
  "RESUMEJOB",
  "REQUEUEJOB",
  "CHECKPOINTJOB",
  "MIGRATEJOB",
  "PURGEJOB",
  "MODIFYJOB",
  "STARTTASK",
  "KILLTASK",
  "SUSPENDTASK",
  "RESUMETASK",
  "GETTASKSTATUS",
  "GETNODESTATUS",
  "GETJOBSTATUS",
  "UPDATETASKSTATUS",
  "UPDATENODESTATUS",
  "UPDATEJOBSTATUS",
  "ALLOCATENODE",
  "STARTNODEMAN",
  "SPAWNTASKMAN",
  "SPAWNJOBMAN",
  "RECONFIG",
  "SHUTDOWN",
  "DIAGNOSE",
  "REPARENT",
  "SIGNALJOB",
  "JOBWILLRUN",
  NULL };


/* sync w/enum MWNodeAttrEnum */

const char *MWikiNodeAttr[] = {
  NONE,
  "ACLASS",
  "ADISK",
  "AFS",
  "AMEMORY",
  "APROC",
  "ARCH",
  "ARES",
  "ASWAP",
  "ATTR",
  "CAT",
  "CCLASS",
  "CDISK",
  "CFS",
  "CHARGERATE",
  "CMEMORY",
  "CONTAINERNODE",
  "CPROC",
  "CPULOAD",
  "CPUSPEED",
  "CRES",
  "CSWAP",
  "CURRENTTASK",
  "FEATURE",
  "GEVENT",
  "GMETRIC",
  "IDLETIME",
  "IOLOAD",
  "MAXTASK",
  "MESSAGE",
  "NAME",
  "NODEINDEX",
  "OSLIST",
  "OS",
  "OTHER",
  "PARTITION",
  "POWER",
  "PRIORITY",
  "RACK",
  "SLOT",
  "SPEED",
  "STATE",
  "TRANSFERRATE",
  "TYPE",
  "UPDATETIME",
  "VARATTR",
  "VARIABLE",
  "VMOSLIST",
  "NETADDR",
  "RM",
  "NODEACCESSPOLICY",
  "XRES",
  NULL };



/* sync w/enum MWJobAttrEnum */

const char *MWikiJobAttr[] = {
  "NONE",
  "GATTR",
  "GMETRIC",
  "GEVENT",
  "UPDATETIME",
  "SIMWCTIME",
  "EXITCODE",
  "STATE",
  "WCLIMIT",
  "TASKS",
  "NODES",
  "MAXNODES",
  "HOLDTYPE",
  "GEOMETRY",
  "QUEUETIME",  /* submit time */
  "STARTDATE",
  "STARTTIME",
  "CHECKPOINTSTARTTIME",
  "COMPLETETIME",
  "UNAME",
  "GNAME",
  "ACCOUNT",
  "EUSER",
  "PREF",
  "RFEATURES",
  "XFEATURES",
  "RCLASS",
  "ROPSYS",
  "RARCH",
  "RMEM",
  "RAMEM",
  "RMEMCMP",
  "RAMEMCMP",
  "DMEM",
  "RDISK",
  "RDISKCMP",
  "DDISK",
  "RSWAP",
  "RASWAP",
  "RSWAPCMP",
  "RASWAPCMP",
  "DSWAP",
  "CPROCS",
  "CPROCSCMP",
  "PARTITIONMASK",
  "EXEC",
  "ARGS",
  "IWD",
  "COMMENT",
  "REJCOUNT",
  "REJMESSAGE",
  "REJCODE",
  "EVENT",
  "TASKLIST",
  "TASKPERNODE",
  "QOS",
  "ENDDATE",
  "DPROCS",
  "HOSTLIST",
  "EXCLUDE_HOSTLIST",
  "SUSPENDTIME",
  "RESACCESS",
  "NAME",
  "JNAME",
  "ENV",
  "INPUT",
  "OUTPUT",
  "ERROR",
  "FLAGS",
  "MASTERHOST",  /* required node for master task */
  "MASTERREQALLOCTASKLIST",
  "PRIORITY",
  "PRIORITYF",
  "REQRSV",
  "SHELL",
  "SID",
  "SLAVESCRIPT",
  "SUBMITDIR",
  "SUBMITHOST",
  "SUBMITSTRING",
  "SUBMITRM",
  "TEMPLATE",
  "TRIGGER",
  "CHECKPOINTFILE",
  "NODESET",
  "NODEALLOCATIONPOLICY",
  "UPRIORITY",
  "UPROCS",
  "VARIABLE",
  "RCGRES",
  "RAGRES",
  "DGRES",
  "RSOFTWARE",
  "WCKEY",
  "SRM",
  "COMMAND",
  "SUBMITTIME",
  "DISPATCHTIME",
  "SYSTEMQUEUETIME",
  "REQUESTEDTC",
  "REQUESTEDNC",
  "REQUESTEDQOS",
  "BYPASSCOUNT",
  "PARTITION",
  "PSUTILIZED",
  "RMXSTRING",
  "TASKMAP",
  "RMSUBMITSTRING",
  "EFFECTIVEQUEUEDURATION",
  "MESSAGE",
  "VMID",
  NULL };


/* sync w/MWSDAttrEnum[] */

const char *MWikiSDAttr[] = {
  "NONE",
  "BYTESTRANSFERRED",
  "ENDTIME",
  "ERROR",
  "FILESIZE",
  "REMOVETIME",
  "SOURCEURL",
  "STARTTIME",
  "STATE",
  "USER",
  NULL };

/* sync w/MWRsvAttrEnum[] */

const char *MWikiRsvAttr[] = {
  "NONE",
  "CREATIONTIME",
  "STARTTIME",
  "ENDTIME",
  "ALLOCTC",
  "ALLOCNC",
  "STATTAPS",
  "STATTIPS",
  "ALLOCNODELIST",
  "OWNER",
  "NAME",
  "ACL",
  "PRIORITY",
  "TYPE",
  "SUBTYPE",
  "LABEL",
  "RSVGROUP",
  "FLAGS",
  "MESSAGE",
  NULL };

/* need maxbacklog */
/* need maxresponsetime */

const char *MWikiException[] = {
  "NONE",
  "NOREPORT",
  "NOPARENT",
  NULL };


const char *MWikiStatType[] = {
  "NONE",
  "JOB",
  "NODE",
  NULL };


/* sync w/enum MJobStageMethodEnum */

const char *MJobStageMethod[] = {
  NONE,
  "direct",    /* use spawn to call rm-specific default job submission client */
  "globus",    /* use globus 'gatekeeper' based job submission */
  "other",
  NULL };


/* sync w/enum MJobStateEnum */

const char *MJobState[] = {
  "NONE",
  "Idle",
  "Starting",
  "Running",
  "Removed",
  "Completed",
  "Hold",
  "Deferred",
  "SubmitErr",
  "Vacated",
  "NotRun",
  "NotQueued",
  "Unknown",
  "BatchHold",
  "UserHold",
  "SystemHold",
  "Staging",
  "Staged",
  "Suspended",
  "Lost",
  "Allocating",
  "Blocked",
  NULL };


/* sync w/enum MPeerServiceEnum */

const char *MS3CName[] = {
  NONE,
  "system-monitor",
  "queue-manager",
  "scheduler",
  "meta-scheduler",
  "process-manager",
  "accounting-manager",
  "event-manager",
  "service-directory",
  "www",
  NULL };


const char *MS3JobState[] = {
  "NONE",
  "idle",
  "starting",
  "running",
  "removed",
  "completed",
  "held",
  "deferred",
  "submiterr",
  "vacated",
  "notrun",
  "notqueued",
  "unknown",
  "batchhold",
  "userhold",
  "systemhold",
  "staging",
  "staged",
  "suspended",
  "lost",
  NULL };
 
const char *MAMOType[] = {
  NONE,
  "account",
  "class",
  "wallclock",
  "jobid",
  "jobtype",
  "machine",
  "nodetype",
  "procs",
  "proccrate",
  "qos",
  "user",
  NULL };


/* sync w/enum MS3AttrNameEnum */

const char *MSAN[] = {
  NONE,
  "action",
  "args",
  "flags",
  "name",
  "object",
  "op",      /* operation */
  "option",
  "tid",
  "value",
  NULL };


/* sync w/enum MS3ObjNameEnum */

const char *MSON[] = {
  NONE,
  "Body",
  "Data",
  "Envelope",
  "Get",
  "Object",
  "Request",
  "Response",
  "Set",
  "Status",
  "Where",
  "Option",
  NULL };


/* sync w/enum MAttrAffinityType */

const char *MAttrAffinityType[] = {
  NONE,
  "must",
  "should",
  "shouldnot",
  "mustnot",
  NULL };


/* sync w/enum MNodeAffinityTypeEnum */

const char *MAffinityType[] = {
  "unavailable",
  "required",
  "positive",
  "neutral",
  "negative",
  "preemptible",
  NONE,
  NULL };


/* sync w/enum MJobSearchEnum */

const char *MJobSearchMode[] = {
  NONE,
  "basic",
  "completed",
  "extended",
  "internal",
  "jobname",
  NULL };



/* sync w/enum MJobTraceEnum */

const char *MJobSource[] = {
  NONE, 
  "local",
  "trace",
  "wiki",
  "xml",
  NULL };


/* sync w/MVPCMigrationPolicyEnum */

const char *MVPCMigrationPolicy[] = {
  NONE,
  "NEVER",
  "ONFAILURE",
  "OPTIMAL",
  NULL };


/* sync w/MDataStageStateEnum */

const char *MDataStageState[] = {
  NONE,
  "PENDING",
  "STAGED",
  "FAILED",
  "REMOVED" };


/* sync w/MDataStageTypeEnum */

const char *MDataStageType[] = {
  NONE,
  "STAGEIN",
  "STAGEOUT" };


/* sync w/MSDataAttrEnum */

const char *MSDataAttr[] = {
  NONE,
  "DstFileName",
  "DstFileSize",
  "DstHostName",
  "DstLocation",
  "ESDuration",
  "IsProcessed",
  "SrcFileName",
  "SrcFileSize",
  "SrcHostName",
  "SrcLocation",
  "State",
  "TransferRate",
  "TStartDate",
  "Type",
  "UpdateTime",
  NULL };

/* sync w/MSLURMFlagsEnum */

const char *MSLURMFlags[] = {
  NONE,
  "NODEDELTAQUERY",
  "JOBDELTAQUERY",
  "COMPRESSOUTPUT",
  NULL };

/* sync w/MLSFEmulationEnvEnum */

const char *MLSFEmulationEnv[] = {
  NONE,
  "LSB_HOSTS",
  "LS_SUBCWD",
  "LSB_JOBID",
  "LSB_JOBNAME",
  "LSB_QUEUE",
  "LSB_JOBINDEX",
  "LSB_UNIXGROUP",
  "LSB_DEFAULTPROJECT",
  "LSB_MCPU_HOSTS",
  NULL
  };

/* sync with MSLATypeEnum */

const char *MSLAType[] = {
  NONE,
  "AppBacklog",
  "AppFailureRate",
  "AppLoad",
  "AppLoad2",
  "AppResponseTime",
  "AppThroughput",
  "ResCPULoad",
  "ResMemLoad",
  "ResNetLoad",
  NULL };

/* sync with MSLATargetTypeEnum */

const char *MSLATargetType[] = {
  NONE,
  "Hard",
  "Soft",
  NULL};


const enum MXMLOTypeEnum MAToO[] = {
    mxoNONE,
    mxoJob,
    mxoNode,
    mxoRsv,
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoPar,
    mxoSys,
    mxoSys,
    mxoClass,
    mxoQOS,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoRack,
    mxoQueue,
    mxoCluster,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoNONE,
    mxoxVC,
    mxoNONE };

/* END MConst.c */            
