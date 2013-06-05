#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"
#include "mdbi.h"

#define MDB_NODE_STATS_INT_INDECES {1,2,3,4,5,6,7,8,10,11,12,16,17,18,19,20}
#define MDB_NODE_STATS_DOUBLE_INDECES {9,13,14,15}

#define MDB_NODE_STATS_INT_VALUES(IStat) { \
    &(IStat).AProc, \
    &(IStat).MaxMem, \
    (int *)&(IStat).StartTime, \
    (int *)&(IStat).Queuetime, \
    &(IStat).IStatCount, \
    &(IStat).CProc, \
    &(IStat).UMem, \
    (int *)&(IStat).Duration, \
    &(IStat).DProc, \
    &(IStat).IterationCount, \
    &(IStat).MinMem, \
    (int *)&(IStat).Cat, \
    (int *)&(IStat).RCat, \
    (int *)&(IStat).NState, \
    &(IStat).StartJC, \
    &(IStat).FailureJC \
}

#define MDB_NODE_STATS_DOUBLE_VALUES(IStat) { \
  &(IStat).XFactor, \
  &(IStat).MinCPULoad, \
  &(IStat).MaxCPULoad, \
  &(IStat).CPULoad \
}

const char *MDBGeneralStatBaseSelect               = "SELECT ObjectType,ObjectName,Time,StatDuration,IterationCount,IntervalStatCount,IntervalStatDuration,TotalJobCount,JobsSubmitted,JobsActive,JobsEligible,JobsIdle,JobsCompletedByQOS,JobsQueued,JobsPreempted,TotalJobAccuracy,TotalNodeLoad,TotalNodeMemory,TotalNodeCount,TotalAllocatedNodeCount,TotalActiveNodeCount,TotalActiveProcCount,TotalConfiguredProcCount,TotalAvailableCapacity,ProcSecondsDedicated,ProcSecondsExecuted,ProcSecondsRequested,ProcSecondsUtilized,ProcHoursPreempted,ProcHoursSubmitted,ProcHoursQueued,ProcHoursAvailable,ProcHoursDedicated,ProcHoursSuccessful,ProcHoursUtilized,DiskSecondsAvailable,DiskSecondsDedicated,DiskSecondsUtilized,DiskSecondsUtilizedInstant,MemSecondsAvailable,MemSecondsDedicated,MemSecondsUtilized,AvgBypass,MaxBypass,TotalBypass,AvgXFactor,MaxXFactor,AvgQueueTime,MaxQueueTime,TotalQueueTime,TotalRequestedWalltime,TotalExecutionWalltime,MinEfficiency,MinEfficiencyDuration,StartedJobCount,StartedJobsQueueTime,StartedJobsProcCount,StartedJobsXFactor,TotalNodeWeightedXFactor,TotalNodeWeightedJobAccuracy,QOSCredits,AppBacklog,AppLoad,AppResponseTime,AppThroughput,TotalXFactor,Other FROM GeneralStats";
const char *MDBGenericMetricsBaseSelect            = "SELECT GMetricName,GMetricValue,Time FROM GenericMetrics";
const char *MDBNodeStatBaseSelect                  = "SELECT ProcsAvailable,MaxMemoryUsed,Time,TotalQueueTime,IntervalStatCount,ProcsConfigured,TotalMemoryUsed,IntervalStatDuration,TotalXFactor,ProcsDedicated,IterationCount,MinMemoryUsed,MinCPULoad,MaxCPULoad,AvgCPULoad,Category,RCategory,NodeState,StartJC,FailureJC FROM NodeStats";
const char *MDBNodeStatsGenericResourcesBaseSelect = "SELECT GenericResourcesConfigured,GenericResourcesDedicated,GenericResourcesAvailable,GResName,Time FROM NodeStatsGenericResources";
const char *MDBJobsBaseSelect                      = "SELECT ID,SourceRMJobID,DestinationRMJobID,GridJobID,AName,UserName,Account,Class,QOS,OwnerGroup,JobGroup,State,EState,SubState,UserPriority,SystemPriority,CurrentStartPriority,RunPriority,PerPartitionPriority,SubmitTime,QueueTime,StartTime,CompletionTime,CompletionCode,UsedWalltime,RequestedMinWalltime,RequestedMaxWalltime,CPULimit,SuspendTime,HoldTime,ProcessorCount,RequestedNodes,ActivePartition,SpecPAL,DestinationRM,SourceRM,Flags,MinPreemptTime,Dependencies,RequestedHostList,ExcludedHostList,MasterHostName,GenericAttributes,Holds,Cost,Description,Messages,NotificationAddress,StartCount,BypassCount,CommandFile,Arguments,RMSubmitLanguage,StdIn,StdOut,StdErr,RMOutput,RMError,InitialWorkingDirectory,UMask,RsvStartTime,BlockReason,BlockMsg,PSDedicated,PSUtilized FROM Jobs";
const char *MDBRsvsBaseSelect                      = "SELECT Name,ACL,AAccount,AGroup,AUser,AQOS,AllocNodeCount,AllocNodeList,AllocProcCount,AllocTaskCount,CL,Comment,Cost,CTime,Duration,EndTime,ExcludeRsv,ExpireTime,Flags,GlobalID,HostExp,History,Label,LastChargeTime,LogLevel,Messages,Owner,Partition,Profile,ReqArch,ReqFeatureList,ReqMemory,ReqNodeCount,ReqNodeList,ReqOS,ReqTaskCount,ReqTPN,Resources,RsvAccessList,RsvGroup,RsvParent,SID,StartTime,StatCAPS,StatCIPS,StatTAPS,StatTIPS,SubType,Trigger,Type,Variables,VMList FROM Reservations";
const char *MDBTriggersBaseSelect                  = "SELECT TrigID,Name,ActionData,ActionType,BlockTime,Description,Disabled,EBuf,EventTime,EventType,FailOffset,FailureDetected,Flags,IsComplete,IsInterval,LaunchTime,Message,MultiFire,ObjectID,ObjectType,OBuf,TriggerOffset,Period,PID,RearmTime,Requires,Sets,State,Threshold,Timeout,Unsets FROM Triggers";
const char *MDBVCsBaseSelect                       = "SELECT Name,Description,Jobs,Nodes,VMs,Rsvs,VCs,Variables,Flags FROM VCs";
const char *MDBRequestsBaseSelect                  = "SELECT JobID,RIndex,AllocNodeList,AllocPartition,PartitionIndex,NodeAccessPolicy,PreferredFeatures,RequestedApp,RequestedArch,ReqOS,ReqNodeSet,ReqPartition,MinNodeCount,MinTaskCount,TaskCount,TasksPerNode,DiskPerTask,MemPerTask,ProcsPerTask,SwapPerTask,NodeDisk,NodeFeatures,NodeMemory,NodeSwap,NodeProcs,GenericResources,ConfiguredGenericResources FROM Requests";
const char *MDBNodesBaseSelect                     = "SELECT ID,State,OperatingSystem,ConfiguredProcessors,AvailableProcessors,ConfiguredMemory,AvailableMemory,Architecture,AvailGRes,ConfigGRes,AvailClasses,ConfigClasses,ChargeRate,DynamicPriority,EnableProfiling,Features,GMetric,HopCount,HypervisorType,IsDeleted,IsDynamic,JobList,LastUpdateTime,LoadAvg,MaxLoad,MaxJob,MaxJobPerUser,MaxProc,MaxProcPerUser,OldMessages,NetworkAddress,NodeSubState,Operations,OSList,Owner,ResOvercommitFactor,Partition,PowerIsEnabled,PowerPolicy,PowerSelectState,PowerState,Priority,PriorityFunction,ProcessorSpeed,ProvisioningData,AvailableDisk,AvailableSwap,ConfiguredDisk,ConfiguredSwap,ReservationCount,ReservationList,ResourceManagerList,Size,Speed,SpeedWeight,TotalNodeActiveTime,LastModifyTime,TotalTimeTracked,TotalNodeUpTime,TaskCount,VMOSList FROM Nodes";
const char *MDBQueryCPStmtText                     = "SELECT * FROM mcheckpoint where object_type = ? AND object_name = ? AND data NOT LIKE ?";
const char *MDBInsertCPStmtText                    = "INSERT INTO mcheckpoint VALUES (?,?,?,?);";
const char *MDBPurgeCPStmtText                     = "DELETE FROM mcheckpoint where timestamp < ?";
const char *MDBIJobQueryStmtText                   = "SELECT object_name FROM mcheckpoint WHERE object_type='JOB' AND data LIKE '%%RM=\"%%';";
const char *MDBDeleteCPStmtText                    = "DELETE FROM mcheckpoint where object_type= ? AND timestamp < ?";
const char *MDBInsertGeneralStatsStmtText          = "INSERT INTO GeneralStats (ObjectType,ObjectName,Time,StatDuration,IterationCount,IntervalStatCount,IntervalStatDuration,TotalJobCount,JobsSubmitted,JobsActive,JobsEligible,JobsIdle,JobsCompletedByQOS,JobsQueued,JobsPreempted,TotalJobAccuracy,TotalNodeLoad,TotalNodeMemory,TotalNodeCount,TotalAllocatedNodeCount,TotalActiveNodeCount,TotalActiveProcCount,TotalConfiguredProcCount,TotalAvailableCapacity,ProcSecondsDedicated,ProcSecondsExecuted,ProcSecondsRequested,ProcSecondsUtilized,ProcHoursPreempted,ProcHoursSubmitted,ProcHoursQueued,ProcHoursAvailable,ProcHoursDedicated,ProcHoursSuccessful,ProcHoursUtilized,DiskSecondsAvailable,DiskSecondsDedicated,DiskSecondsUtilized,DiskSecondsUtilizedInstant,MemSecondsAvailable,MemSecondsDedicated,MemSecondsUtilized,AvgBypass,MaxBypass,TotalBypass,AvgXFactor,MaxXFactor,AvgQueueTime,MaxQueueTime,TotalQueueTime,TotalRequestedWalltime,TotalExecutionWalltime,MinEfficiency,MinEfficiencyDuration,StartedJobCount,StartedJobsQueueTime,StartedJobsProcCount,StartedJobsXFactor,TotalNodeWeightedXFactor,TotalNodeWeightedJobAccuracy,QOSCredits,AppBacklog,AppLoad,AppResponseTime,AppThroughput,TotalXFactor,Other) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char *MDBInsertEventStmtText                 = "INSERT INTO Events (ID,ObjectType,EventType,EventTime,ObjectName,Name,Description) VALUES (?,?,?,?,?,?,?)";
const char *MDBInsertNodeStatsStmtText             = "INSERT INTO NodeStats (ProcsAvailable,MaxMemoryUsed,Time,TotalQueueTime,IntervalStatCount,ProcsConfigured,TotalMemoryUsed,IntervalStatDuration,TotalXFactor,ProcsDedicated,IterationCount,MinMemoryUsed,MinCPULoad,MaxCPULoad,AvgCPULoad,Category,RCategory,NodeState,StartJC,FailureJC,NodeID) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char *MDBSelectEventsStmtText                = "SELECT Description FROM Events WHERE ObjectName = ? AND ObjectType = ? AND EventType = ? AND EventTime = ?";
const char *MDBInsertJobStmtText                   = "INSERT INTO Jobs (ID,SourceRMJobID,DestinationRMJobID,GridJobID,AName,UserName,Account,Class,QOS,OwnerGroup,JobGroup,State,EState,SubState,UserPriority,SystemPriority,CurrentStartPriority,RunPriority,PerPartitionPriority,SubmitTime,QueueTime,StartTime,CompletionTime,CompletionCode,UsedWalltime,RequestedMinWalltime,RequestedMaxWalltime,CPULimit,SuspendTime,HoldTime,ProcessorCount,RequestedNodes,ActivePartition,SpecPAL,DestinationRM,SourceRM,Flags,MinPreemptTime,Dependencies,RequestedHostList,ExcludedHostList,MasterHostName,GenericAttributes,Holds,Cost,Description,Messages,NotificationAddress,StartCount,BypassCount,CommandFile,Arguments,RMSubmitLanguage,StdIn,StdOut,StdErr,RMOutput,RMError,InitialWorkingDirectory,UMask,RsvStartTime,BlockReason,BlockMsg,PSDedicated,PSUtilized) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char *MDBInsertRsvStmtText                   = "INSERT INTO Reservations (Name,ACL,AAccount,AGroup,AUser,AQOS,AllocNodeCount,AllocNodeList,AllocProcCount,AllocTaskCount,CL,Comment,Cost,CTime,Duration,EndTime,ExcludeRsv,ExpireTime,Flags,GlobalID,HostExp,History,Label,LastChargeTime,LogLevel,Messages,Owner,Partition,Profile,ReqArch,ReqFeatureList,ReqMemory,ReqNodeCount,ReqNodeList,ReqOS,ReqTaskCount,ReqTPN,Resources,RsvAccessList,RsvGroup,RsvParent,SID,StartTime,StatCAPS,StatCIPS,StatTAPS,StatTIPS,SubType,TTrigger,TType,Variables,VMList) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char *MDBInsertTriggerStmtText               = "INSERT INTO Triggers (TrigID,Name,ActionData,ActionType,BlockTime,Description,Disabled,EBuf,EventTime,EventType,FailOffset,FailureDetected,Flags,IsComplete,IsInterval,LaunchTime,Message,MultiFire,ObjectID,ObjectType,OBuf,TriggerOffset,Period,PID,RearmTime,Requires,Sets,State,Threshold,Timeout,Unsets) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char *MDBInsertVCStmtText                    = "INSERT INTO VCs (Name,Description,Jobs,Nodes,VMs,Rsvs,VCs,Variables,Flags) VALUES (?,?,?,?,?,?,?,?,?);";
const char *MDBInsertRequestStmtText               = "INSERT INTO Requests (JobID,RIndex,AllocNodeList,AllocPartition,PartitionIndex,NodeAccessPolicy,PreferredFeatures,RequestedApp,RequestedArch,ReqOS,ReqNodeSet,ReqPartition,MinNodeCount,MinTaskCount,TaskCount,TasksPerNode,DiskPerTask,MemPerTask,ProcsPerTask,SwapPerTask,NodeDisk,NodeFeatures,NodeMemory,NodeSwap,NodeProcs,GenericResources,ConfiguredGenericResources) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char *MDBInsertNodeStmtText                  = "INSERT INTO Nodes (ID,State,OperatingSystem,ConfiguredProcessors,AvailableProcessors,ConfiguredMemory,AvailableMemory,Architecture,AvailGRes,ConfigGRes,AvailClasses,ConfigClasses,ChargeRate,DynamicPriority,EnableProfiling,Features,GMetric,HopCount,HypervisorType,IsDeleted,IsDynamic,JobList,LastUpdateTime,LoadAvg,MaxLoad,MaxJob,MaxJobPerUser,MaxProc,MaxProcPerUser,OldMessages,NetworkAddress,NodeSubState,Operations,OSList,Owner,ResOvercommitFactor,Partition,PowerIsEnabled,PowerPolicy,PowerSelectState,PowerState,Priority,PriorityFunction,ProcessorSpeed,ProvisioningData,AvailableDisk,AvailableSwap,ConfiguredDisk,ConfiguredSwap,ReservationCount,ReservationList,ResourceManagerList,Size,Speed,SpeedWeight,TotalNodeActiveTime,LastModifyTime,TotalTimeTracked,TotalNodeUpTime,TaskCount,VMOSList) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char *MDBJobsBaseDelete                      = "DELETE FROM Jobs;";
const char *MDBRsvsBaseDelete                      = "DELETE FROM Reservations;";
const char *MDBTriggersBaseDelete                  = "DELETE FROM Triggers;";
const char *MDBVCsBaseDelete                       = "DELETE FROM VCs;";
const char *MDBRequestsBaseDelete                  = "DELETE FROM Requests;";
const char *MDBNodesBaseDelete                     = "DELETE FROM Nodes;";
char  MDBSelectGeneralStatsStmtText[MMAX_BUFFER];
char  MDBSelectGeneralStatsRangeStmtText[MMAX_BUFFER];
char  MDBSelectNodeStatsStmtText[MMAX_BUFFER];
char  MDBSelectNodeStatsRangeStmtText[MMAX_BUFFER];
char  MDBSelectNodeStatsGenericResourcesStmtText[MMAX_BUFFER];
char  MDBSelectJobsStmtText[MMAX_BUFFER];
char  MDBSelectRsvsStmtText[MMAX_BUFFER];
char  MDBSelectTriggersStmtText[MMAX_BUFFER];
char  MDBSelectVCsStmtText[MMAX_BUFFER];
char  MDBSelectRequestsStmtText[MMAX_BUFFER];
char  MDBSelectNodesStmtText[MMAX_BUFFER];
char  MDBSelectNodeStatsGenericResourcesRangeStmtText[MMAX_BUFFER];
char  MDBSelectGenericMetricsStmtText[MMAX_BUFFER];
char  MDBSelectGenericMetricsRangeStmtText[MMAX_BUFFER];
char  MDBDeleteJobsStmtText[MMAX_BUFFER];
char  MDBDeleteRsvsStmtText[MMAX_BUFFER];
char  MDBDeleteTriggersStmtText[MMAX_BUFFER];
char  MDBDeleteVCsStmtText[MMAX_BUFFER];
char  MDBDeleteRequestsStmtText[MMAX_BUFFER];
char  MDBDeleteNodesStmtText[MMAX_BUFFER];

int MDBSelectNodesStmtAddWhereClause(marray_t *);
int MDBQueryEvents(mdb_t *,mulong,mulong,int *,enum MRecordEventTypeEnum *,enum MXMLOTypeEnum *,char [][MMAX_LINE],mevent_itr_t *,char *,enum MStatusCodeEnum *);



/**
 *Static initialization of moab's database module. Needs to be called
 *before any other database-related calls are made.
 */

int MDBModuleInitialize()

  {
  const char *FName = "MDBModuleInitialize";

  MDB(5,fSTRUCT) MLog("%s()\n",
    FName);

  snprintf(MDBSelectGeneralStatsStmtText,sizeof(MDBSelectGeneralStatsStmtText),"%s WHERE ObjectType = ? AND ObjectName = ? AND Time = ?;",
    MDBGeneralStatBaseSelect);

  snprintf(MDBSelectGeneralStatsRangeStmtText,sizeof(MDBSelectGeneralStatsRangeStmtText),"%s WHERE ObjectType = ? AND ObjectName = ? AND Time >=  ? AND Time <=  ? ORDER BY Time ASC;",
    MDBGeneralStatBaseSelect);

  snprintf(MDBSelectGenericMetricsStmtText,sizeof(MDBSelectGenericMetricsStmtText),"%s WHERE ObjectType = ? AND ObjectName = ? AND Time = ?;",
    MDBGenericMetricsBaseSelect);

  snprintf(MDBSelectGenericMetricsRangeStmtText,sizeof(MDBSelectGenericMetricsRangeStmtText),"%s WHERE ObjectType = ? AND ObjectName = ? AND Time >= ? AND Time <= ?;",
    MDBGenericMetricsBaseSelect);

  snprintf(MDBSelectNodeStatsStmtText,sizeof(MDBSelectNodeStatsStmtText),"%s WHERE NodeID = ? AND Time = ?;",
    MDBNodeStatBaseSelect);

  snprintf(MDBSelectNodeStatsRangeStmtText,sizeof(MDBSelectNodeStatsRangeStmtText),"%s WHERE NodeID = ? AND Time >= ? AND Time <= ?;",
    MDBNodeStatBaseSelect);

  snprintf(MDBSelectNodeStatsGenericResourcesStmtText,sizeof(MDBSelectNodeStatsGenericResourcesStmtText),"%s WHERE NodeID = ? AND Time = ?;",
    MDBNodeStatsGenericResourcesBaseSelect);

  snprintf(MDBSelectJobsStmtText,sizeof(MDBSelectJobsStmtText),"%s;",
    MDBJobsBaseSelect);

  snprintf(MDBSelectRsvsStmtText,sizeof(MDBSelectRsvsStmtText),"%s;",
    MDBRsvsBaseSelect);

  snprintf(MDBSelectTriggersStmtText,sizeof(MDBSelectTriggersStmtText),"%s;",
    MDBTriggersBaseSelect);

  snprintf(MDBSelectVCsStmtText,sizeof(MDBSelectVCsStmtText),"%s;",
    MDBVCsBaseSelect);

  snprintf(MDBSelectRequestsStmtText,sizeof(MDBSelectRequestsStmtText),"%s WHERE JobID = ?;",
    MDBRequestsBaseSelect);

  snprintf(MDBSelectNodesStmtText,sizeof(MDBSelectNodesStmtText),"%s;",
    MDBNodesBaseSelect);

  snprintf(MDBSelectNodeStatsGenericResourcesRangeStmtText,sizeof(MDBSelectNodeStatsGenericResourcesRangeStmtText),"%s WHERE NodeID = ? AND Time >= ? AND Time <= ?;",
    MDBNodeStatsGenericResourcesBaseSelect);

  snprintf(MDBDeleteJobsStmtText,sizeof(MDBDeleteJobsStmtText),"%s;",
    MDBJobsBaseDelete);

  snprintf(MDBDeleteRsvsStmtText,sizeof(MDBDeleteRsvsStmtText),"%s;",
    MDBRsvsBaseDelete);

  snprintf(MDBDeleteTriggersStmtText,sizeof(MDBDeleteTriggersStmtText),"%s;",
    MDBTriggersBaseDelete);

  snprintf(MDBDeleteVCsStmtText,sizeof(MDBDeleteVCsStmtText),"%s;",
    MDBVCsBaseDelete);

  snprintf(MDBDeleteRequestsStmtText,sizeof(MDBDeleteRequestsStmtText),"%s;",
    MDBRequestsBaseDelete);

  return(SUCCESS);
  }  /* END MDBModuleInitialize() */ 




/**
 * initialize the db info struct
 *
 * @param   MDBInfo (I) [modified]
 * @param   Type    (I) the type of db we'll initialize
 */

int MDBInitialize(

  mdb_t       *MDBInfo, /* I */
  MDBTypeEnum  Type)    /* I */

  {
  int rc;

  const char *FName = "MDBInitialize";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,Type)\n",
    FName);

  if (MDBInfo->DBUnion.ODBC != NULL)
    {
    MDB(0,fSCHED) MLog("WARNING:  thread %lu attempting to re-initialize database info struct\n",
        MUGetThreadID());

    return(FAILURE);
    }

  MDBInfo->DBType = Type;
  MDBInfo->DBUnion.ODBC = NULL;

  switch (Type)
    {
    case mdbNONE:

      rc = SUCCESS;

      break;

    case mdbSQLite3:

      rc = MSQLite3Initialize(MDBInfo,NULL);

      break;

    case mdbODBC:

      rc = MODBCInitialize(MDBInfo);

      break;

    default:

      /* NOT HANDLED */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBInitialize() */




/**
 * Bind a double variable as input into the statement represented by MStmt
 *
 * @param MStmt   (I)
 * @param Param   (I)
 * @param Val     (I)
 * @param IsArray (I)
 */

int MDBBindParamDouble(

  mstmt_t *MStmt,   /* I */
  int      Param,   /* I */
  double  *Val,     /* I */
  int      IsArray) /* I */

  {
  int rc;

  const char *FName = "MDBBindParamDouble";

  MDB(5,fSTRUCT) MLog("%s(MStmt,%d,Val,%d)\n",
    FName,
    Param,
    IsArray);

  switch(MStmt->DBType) 
    {
    case mdbSQLite3:

      rc = MSQLite3BindParamDouble(
             MStmt->StmtUnion.SQLite3,
             Param,
             Val,
             IsArray);

      break;

    case mdbODBC:

      rc = MODBCBindParamDouble(
             MStmt->StmtUnion.ODBC,
             Param,
             Val,
             IsArray);

      break;

    default:

      /* NOT HANDLED */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBBindParamDouble() */ 




/**
 * Bind an integer variable as input into the statement represented by MStmt
 *
 * @param MStmt   (I)
 * @param Param   (I)
 * @param Val     (I)
 * @param IsArray (I)
 */

int MDBBindParamInteger(

  mstmt_t *MStmt,   /* I */
  int      Param,   /* I */
  int     *Val,     /* I */
  int      IsArray) /* I */

  {
  int rc;

  const char *FName = "MDBBindParamInteger";

  MDB(5,fSTRUCT) MLog("%s(MStmt,%d,Val,%d)\n",
    FName,
    Param,
    IsArray);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3BindParamInteger(
             MStmt->StmtUnion.SQLite3,
             Param,
             Val,
             IsArray);

      break;

    case mdbODBC:

      rc = MODBCBindParamInteger(
             MStmt->StmtUnion.ODBC,
             Param,
             Val,
             IsArray);

      break;

    default:

      /* NOT HANDLED */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBBindParamInteger() */ 




/**
 * Bind a char variable as input into the statement represented by MStmt
 *
 * @param MStmt (I)
 * @param Param (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MDBBindParamText(

  mstmt_t    *MStmt, /* I */
  int  const  Param, /* I */
  char const *Val,   /* I */
  int  const  Size)  /* I */

  {
  int rc;

  const char *FName = "MDBBindParamText";

  MDB(5,fSTRUCT) MLog("%s(MStmt,Param,Val,Size)\n",
    FName);

  switch (MStmt->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3BindParamText(
             MStmt->StmtUnion.SQLite3,
             Param,
             Val,
             Size);

      break;

    case mdbODBC:

      rc = MODBCBindParamText(
             MStmt->StmtUnion.ODBC,
             Param,
             Val,
             Size);

      break;

    default:

      /* NO-OP */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBBindParamText() */ 






/**
 *Bind the address of a double as an output sink for the statement MStmt
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 */

int MDBBindColDouble(

  struct mstmt_t *MStmt, /* I */
  int             Col,   /* I */
  double         *Val)   /* I */

  {
  const char *FName = "MDBBindColDouble";

  MDB(5,fSTRUCT) MLog("%s(MStmt,%d,Val)\n",
    FName,
    Col);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3BindColDouble(MStmt->StmtUnion.SQLite3,Col,Val));

    case mdbODBC:
      return(MODBCBindColDouble(MStmt->StmtUnion.ODBC,Col,Val));
      break;

    default:
      return(FAILURE);
    }
  } /* END MDBBindColDouble */ 




/**
 *Bind the address of an integer as an output sink for the statement MStmt
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 */

int MDBBindColInteger(


  struct mstmt_t *MStmt, /* I */
  int             Col,   /* I */
  int            *Val)   /* I */

  {
  const char *FName = "MDBBindColInteger";

  MDB(5,fSTRUCT) MLog("%s(MStmt,%d,Val)\n",
    FName,
    Col);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3BindColInteger(MStmt->StmtUnion.SQLite3,Col,Val));

    case mdbODBC:
      return(MODBCBindColInteger(MStmt->StmtUnion.ODBC,Col,Val));
      break;

    default:
      return(FAILURE);
    }
  } /* END MDBBindColInteger */ 




/**
 *Bind the address of a char array as an output sink for the statement MStmt
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MDBBindColText(

  struct mstmt_t *MStmt, /* I */
  int             Col,   /* I */
  void           *Val,   /* I */
  int             Size)  /* I */

  {
  const char *FName = "MDBBindColText";

  MDB(5,fSTRUCT) MLog("%s(MStmt,%d,Val,%d)\n",
    FName,
    Col,
    Size);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3BindColText(MStmt->StmtUnion.SQLite3,Col,Val,Size));

    case mdbODBC:
      return(MODBCBindColText(MStmt->StmtUnion.ODBC,Col,Val,Size));
      break;

    default:
      return(FAILURE);
    }
  } /* END MDBBindColText */ 




/**
 * Clear all Jobs/Nodes/Requests from their tables
 *
 * @param   MDBInfo (I) the db info struct
 * @param   EMsg    (O) any output error message
 */

int MDBClearTables(

  mdb_t *MDBInfo,
  char  *EMsg)

  {
  MDBDeleteJobs(MDBInfo,NULL,EMsg);

  MDBDeleteRsvs(MDBInfo,NULL,EMsg);

  MDBDeleteTriggers(MDBInfo,NULL,EMsg);

  MDBDeleteVCs(MDBInfo,NULL,EMsg);

  MDBDeleteRequests(MDBInfo,NULL,EMsg);

  MDBDeleteNodes(MDBInfo,NULL,EMsg);

  return(SUCCESS);
  }  /* END MDBClearTables() */




/**
 * delete job(s) from the Jobs table
 *
 * @param   MDBInfo (I) the db info struct 
 * @param   JConstraintList (I) constraints to be added in where clause 
 * @param   EMsg    (O) an output error message
 */

int MDBDeleteJobs(
 
  mdb_t    *MDBInfo,
  marray_t *JConstraintList,
  char     *EMsg)

  {
  enum MStatusCodeEnum SC;
  mstmt_t MStmt;

  if (MDBInfo->DBType == mdbNONE)
    {
    return(SUCCESS);
    }

  /* reset the job delete statement since it may have been changed for a
   * previous query */

  snprintf(MDBDeleteJobsStmtText,MMAX_LINE,"%s;",
    MDBJobsBaseDelete);

  /* if we have constraints then add a where clause to the delete statement
   * to enforce the constraints */

  if (JConstraintList != NULL)
    MDBDeleteJobsStmtAddWhereClause(JConstraintList);

  if (MDBDeleteJobsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBResetStmt(&MStmt) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBDeleteJobs() */




/**
 * Retrieve the statement to delete all jobs from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (O) the statement we'll execute later
 * @param   EMsg    (O) any output error message
 */

int MDBDeleteJobsStmt(
 
  mdb_t   *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3DeleteJobsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCDeleteJobsStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    case mdbNONE:
      return(SUCCESS);

    default:
      return(FAILURE);
    }
  }  /* END MDBDeleteJobsStmt() */




/**
 * delete request(s) from the Requests table
 *
 * @param   MDBInfo (I) the db info struct
 * @param   ReqConstraintList (I) constraints to be added in where clause 
 * @param   EMsg    (O) an output error message
 */

int MDBDeleteRequests(

  mdb_t    *MDBInfo,
  marray_t *ReqConstraintList,
  char     *EMsg)

  {
  enum MStatusCodeEnum SC;
  mstmt_t MStmt;

  if (MDBInfo->DBType == mdbNONE)
    {
    return(SUCCESS);
    }

  snprintf(MDBDeleteRequestsStmtText,MMAX_LINE,"%s;",
    MDBRequestsBaseDelete);

  /* if we have constraints then add a where clause to the delete statement
   * to enforce the constraints */

  if (ReqConstraintList != NULL)
    MDBDeleteRequestsStmtAddWhereClause(ReqConstraintList);

  if (MDBDeleteRequestsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBResetStmt(&MStmt) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MDBDeleteRequests() */




/**
 * Retrieve the statement to delete all requests from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (O) the statement we'll execute later
 * @param   EMsg    (O) any output error message
 */

int MDBDeleteRequestsStmt(

  mdb_t    *MDBInfo,
  mstmt_t  *MStmt,
  char     *EMsg)

  {
  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3DeleteRequestsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCDeleteRequestsStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    case mdbNONE:
      return(SUCCESS);

    default:
      return(FAILURE);
    }
  } /* END MDBDeleteRequestsStmt() */




/**
 * delete all nodes from the Nodes table
 *
 * @param   MDBInfo (I) the db info struct
 * @param   NodeConstraintList 
 * @param   EMsg    (O) an output error message
 */

int MDBDeleteNodes(

  mdb_t    *MDBInfo,
  marray_t *NodeConstraintList,
  char     *EMsg)

  {
  enum MStatusCodeEnum SC;

  mstmt_t MStmt;
 
  if (MDBInfo->DBType == mdbNONE)
    {
    return(SUCCESS);
    }

  snprintf(MDBDeleteNodesStmtText,MMAX_LINE,"%s;",
    MDBNodesBaseDelete);

  if (NodeConstraintList != NULL)
    MDBDeleteNodesStmtAddWhereClause(NodeConstraintList);

  if (MDBDeleteNodesStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBResetStmt(&MStmt) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBDeleteNodesStmt() */




/**
 * Retrieve the statement to delete all nodes from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (O) the statement we'll execute later
 * @param    EMsg    (O) any output error message
 */

int MDBDeleteNodesStmt(

  mdb_t   *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3DeleteNodesStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCDeleteNodesStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    case mdbNONE:
      return(SUCCESS);

    default:
      return(FAILURE);
    }
  }  /* END MDBDeleteNodesStmt() */



/**
 * delete reservations(s) from the Reservations table
 *
 * @param   MDBInfo (I) the db info struct 
 * @param   RConstraintList (I) constraints to be added in where 
 *                          clause
 * @param   EMsg    (O) an output error message
 */

int MDBDeleteRsvs(
 
  mdb_t    *MDBInfo,
  marray_t *RConstraintList,
  char     *EMsg)

  {
  enum MStatusCodeEnum SC;

  mstmt_t MStmt;

  if (MDBInfo->DBType == mdbNONE)
    {
    return(SUCCESS);
    }

  /* reset the reservation delete statement since it may have been changed for a
   * previous query */

  snprintf(MDBDeleteRsvsStmtText,MMAX_LINE,"%s;",
    MDBRsvsBaseDelete);

  /* if we have constraints then add a where clause to the delete statement
   * to enforce the constraints */

  if (RConstraintList != NULL)
    MDBDeleteRsvsStmtAddWhereClause(RConstraintList);

  if (MDBDeleteRsvsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBResetStmt(&MStmt) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBDeleteRsvs() */



/**
 * Retrieve the statement to delete all reservations from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (O) the statement we'll execute later
 * @param   EMsg    (O) any output error message
 */

int MDBDeleteRsvsStmt(

  mdb_t   *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3DeleteRsvsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCDeleteRsvsStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    case mdbNONE:
      return(SUCCESS);

    default:
      return(FAILURE);
    }
  }  /* END MDBDeleteRsvsStmt() */




/**
 * Delete trigger(s) from the Triggers table.
 *
 * @param   MDBInfo (I) the db info struct 
 * @param   TConstraintList (I) constraints to be added in where
 *                          clause
 * @param   EMsg    (O) an output error message
 */

int MDBDeleteTriggers(
 
  mdb_t    *MDBInfo,         /* I */
  marray_t *TConstraintList, /* I */
  char     *EMsg)            /* O */

  {
  enum MStatusCodeEnum SC;

  mstmt_t MStmt;

  if (MDBInfo->DBType == mdbNONE)
    {
    return(SUCCESS);
    }

  /* reset the trigger delete statement since it may have been changed for a
   * previous query */

  snprintf(MDBDeleteTriggersStmtText,MMAX_LINE,"%s;",
    MDBTriggersBaseDelete);

  /* if we have constraints then add a where clause to the delete statement
   * to enforce the constraints */

  if (TConstraintList != NULL)
    MDBDeleteTriggersStmtAddWhereClause(TConstraintList);

  if (MDBDeleteTriggersStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBResetStmt(&MStmt) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBDeleteTriggers() */



/**
 * Delete VC(s) from the VCs table.
 *
 * @param   MDBInfo (I) the db info struct 
 * @param   VCConstraintList (I) constraints to be added in where
 *                          clause
 * @param   EMsg    (O) an output error message
 */

int MDBDeleteVCs(
 
  mdb_t    *MDBInfo,         /* I */
  marray_t *VCConstraintList,/* I */
  char     *EMsg)            /* O */

  {
  enum MStatusCodeEnum SC;

  mstmt_t MStmt;

  if (MDBInfo->DBType == mdbNONE)
    {
    return(SUCCESS);
    }

  /* reset the VC delete statement since it may have been changed for a
   * previous query */

  snprintf(MDBDeleteVCsStmtText,MMAX_LINE,"%s;",
    MDBVCsBaseDelete);

  /* if we have constraints then add a where clause to the delete statement
   * to enforce the constraints */

  if (VCConstraintList != NULL)
    MDBDeleteVCsStmtAddWhereClause(VCConstraintList);

  if (MDBDeleteVCsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBResetStmt(&MStmt) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBDeleteVCs() */



/**
 * Retrieve the statement to delete all triggers from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (O) the statement we'll execute later
 * @param   EMsg    (O) any output error message
 */

int MDBDeleteTriggersStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3DeleteTriggersStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCDeleteTriggersStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    case mdbNONE:
      return(SUCCESS);

    default:
      return(FAILURE);
    }
  }  /* END MDBDeleteTriggersStmt() */


/**
 * Retrieve the statement to delete all VCs from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (O) the statement we'll execute later
 * @param   EMsg    (O) any output error message
 */

int MDBDeleteVCsStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3DeleteVCsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCDeleteVCsStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    case mdbNONE:
      return(SUCCESS);

    default:
      return(FAILURE);
    }
  }  /* END MDBDeleteVCsStmt() */




/**
 * Retrieve statement to select a single events entry
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectEventsMsgsStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  const char *FName = "MDBSelectEventsMsgsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectEventsMsgsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectEventsMsgsStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBSelectEventsStmt */




/**
 *Retrieve statement to select a single statistics entry
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectGeneralStatsStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  const char *FName = "MDBSelectGeneralStatsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectGeneralStatsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectGeneralStatsStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBSelectGeneralStatsStmt */ 




/**
 *Retrieve statement to select generic resources for a single node statistic.
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectNodeStatsGenericResourcesStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBSelectNodeStatsGenericResourcesStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectNodeStatsGenericResourcesStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectNodeStatsGenericResourcesStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBSelectNodeStatsGenericResourcesStmt */ 




/**
 *retrieve statement to select a time-based range of statistics
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectGeneralStatsRangeStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  const char *FName = "MDBSelectGeneralStatsRangeStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectGeneralStatsRangeStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectGeneralStatsRangeStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBSelectGeneralStatsRangeStmt */ 




/**
 * Retrive statement to select generic resources for a time-based range of 
 * node statistics.
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectNodeStatsGenericResourcesRangeStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  int rc;

  const char *FName = "MDBSelectNodeStatsGenericResourcesRangeStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3SelectNodeStatsGenericResourcesRangeStmt(
             MDBInfo->DBUnion.SQLite3,
             MStmt,
             EMsg);

      break;

    case mdbODBC:

      rc = MODBCSelectNodeStatsGenericResourcesRangeStmt(
             MDBInfo->DBUnion.ODBC,
             MStmt,
             EMsg);

      break;

    default:

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBSelectNodeStatsGenericResourcesRangeStmt() */ 




/**
 * Retrive statement to select a single node statistic.
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectNodeStatsStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  int rc;

  const char *FName = "MDBSelectNodeStatsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3SelectNodeStatsStmt(
             MDBInfo->DBUnion.SQLite3,
             MStmt,
             EMsg);

      break;

    case mdbODBC:

      rc = MODBCSelectNodeStatsStmt(
             MDBInfo->DBUnion.ODBC,
             MStmt,
             EMsg);

      break;

    default:

      rc = FAILURE;

      break;
    }
  
  return(rc);
  }  /* END MDBSelectNodeStatsStmt() */ 




/**
 * Retrieve statement to select a time-based rnage of node statistics.
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectNodeStatsRangeStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  int rc;

  const char *FName = "MDBSelectNodeStatsRangeStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3SelectNodeStatsRangeStmt(
             MDBInfo->DBUnion.SQLite3,
             MStmt,
             EMsg);

      break;

    case mdbODBC:

      rc = MODBCSelectNodeStatsRangeStmt(
             MDBInfo->DBUnion.ODBC,
             MStmt,
             EMsg);

      break;

    default:

      /* NOT HANDLED */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBSelectNodeStatsRangeStmt() */ 




/**
 * Retrieve statement to select generic metrics for a single statistic.
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectGenericMetricsStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  int rc;

  const char *FName = "MDBSelectGenericMetricsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3SelectGenericMetricsStmt(
             MDBInfo->DBUnion.SQLite3,
             MStmt,
             EMsg);

      break;

    case mdbODBC:

      rc = MODBCSelectGenericMetricsStmt(
             MDBInfo->DBUnion.ODBC,
             MStmt,
             EMsg);

      break;

    default:

      /* NOT HANDLED */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBSelectGenericMetricsStmt() */ 




/**
 * Retrieve statement to select generic metrics for a time-based range of 
 * statistics.
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBSelectGenericMetricsRangeStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  int rc;

  const char *FName = "MDBSelectGenericMetricsRangeStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3SelectGenericMetricsRangeStmt(
             MDBInfo->DBUnion.SQLite3,
             MStmt,
             EMsg);

      break;

    case mdbODBC:

      rc = MODBCSelectGenericMetricsRangeStmt(
             MDBInfo->DBUnion.ODBC,
             MStmt,
             EMsg);

      break;

    default:

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBSelectGenericMetricsRangeStmt() */ 




/**
 * Retrieve statement to instert a single statistic
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertGeneralStatsStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  int rc;
 
  const char *FName = "MDBInsertGeneralStatsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3InsertGeneralStatsStmt(
             MDBInfo->DBUnion.SQLite3,
             MStmt,
             EMsg);

      break;

    case mdbODBC:

      rc = MODBCInsertGeneralStatsStmt(
             MDBInfo->DBUnion.ODBC,
             MStmt,
             EMsg);

      break;

    default:

      /* NOT HANDLED */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBInsertGeneralStatsStmt() */ 




/**
 * Retrieve statement to insert a single node statistic.
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertNodeStatsStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBInsertNodeStatsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertNodeStatsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertNodeStatsStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertNodeStatsStmt */ 




/**
 * Retrieve statement to insert a single node
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertNodeStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBInsertNodeStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertNodeStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertNodeStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertNodeStmt */




/**
 * Retrieve statement to insert a single job
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertJobStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBInsertJobStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertJobStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertJobStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertJobStmt */




/**
 * Retrieve statement to insert a single reservation
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertRsvStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBInsertRsvStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertRsvStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertRsvStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertRsvStmt */




/**
 * Retrieve statement to insert a single trigger.
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertTriggerStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBInsertTriggerStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertTriggerStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertTriggerStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertTriggerStmt */




/**
 * Retrieve statement to insert a single VC.
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertVCStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBInsertVCStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertVCStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertVCStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertVCStmt */





/**
 * Retrieve statement to insert a single request
 *
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertRequestStmt(

    mdb_t    *MDBInfo,  /* I */
    mstmt_t  *MStmt,    /* O */
    char     *EMsg)     /* O */

  {
  const char *FName = "MDBInsertReqStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertRequestStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertRequestStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertReqStmt */




/**
 * Retrieve statement to insert a single event
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertEventStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *FName = "MDBInsertEventStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertEventStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertEventStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInsertNodeStatsStmt */




/**
 * Retrieve statement to query the database for checkpoint information.
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBQueryCPStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  const char *FName = "MDBQueryCPStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3QueryCPStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCQueryCPStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBQueryCPStmt */ 




/**
 * Retrieve statement to insert a checkpoint entry 
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBInsertCPStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  const char *FName = "MDBInsertCPStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3InsertCPStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCInsertCPStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBInsertCPStmt */ 




/**
 * Retrieve statement to purge checkpoint data earlier than a specified time
 * @param MDBInfo (I)
 * @param MStmt   (O)
 */

int MDBPurgeCPStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt)   /* O */

  {
  const char *FName = "MDBPurgeCPStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3PurgeCPStmt(MDBInfo->DBUnion.SQLite3,MStmt));

    case mdbODBC:
      return(MODBCPurgeCPStmt(MDBInfo->DBUnion.ODBC,MStmt));

    default:
      return(FAILURE);
    }
  } /* END MDBPurgeCPStmt */ 




/**
 * Retrieve statement to query the names of jobs that contain a specified piece of data.
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBIJobQueryStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  const char *FName = "MDBIJobQueryStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3IJobQueryStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCIJobQueryStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBIJobQueryStmt */ 




/**
 * Retrieve statement to delete checkpoint entries of a specified type earlier 
 * than a specified date.
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MDBDeleteCPStmt(

  mdb_t   *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)   /* O */

  {
  const char *FName = "MDBDeleteCPStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3DeleteCPStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCDeleteCPStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }
  } /* END MDBDeleteCPStmt */ 




/**
 * Execute the statement represented by MStmt.
 *
 * @param MStmt   (I)
 * @param SC      (O)
 */

int MDBExecStmt(

  mstmt_t              *MStmt,
  enum MStatusCodeEnum *SC)

  {
  const char *FName = "MDBExecStmt";

  MDB(5,fSTRUCT) MLog("%s(MStmt)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      {
      return(MSQLite3ExecStmt(MStmt->StmtUnion.SQLite3));
      }

    case mdbODBC:
      {
      switch(MStmt->StmtType)
        {
        case mstmtPrepared:
          return(MODBCExecPrepStmt(MStmt->StmtUnion.PODBC,SC));

        case mstmtDirect:
          return(MODBCExecDirectStmt(MStmt->StmtUnion.DODBC,SC));

        default:
          return(FAILURE);

        }
      }
    default:
      return(FAILURE);
    }
  } /* END MDBExecStmt */ 




/**
 *Fetch the results of an executed statement. the actual data will be stored
 *in addresses specified by calls to MDBBindCols. For each column that was 
 *not bound to an address, the data for that column is lost.
 *Returns FAILURE if the fetch failed, MDB_FETCH_NO_DATA if no data remains to
 *be fetched, and SUCCESS if a row of data was successfully fetched.
 * @param MStmt (I)
 */

int MDBFetch(

  mstmt_t *MStmt) /* I */

  {
  const char *FName = "MDBFetch";

  MDB(5,fSTRUCT) MLog("%s(MStmt)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:

      return(MSQLite3Fetch(MStmt->StmtUnion.SQLite3));

      break;

    case mdbODBC:

      return(MODBCFetch(MStmt->StmtUnion.ODBC));

      break;

    default:

      break;
    }

  return(FAILURE);
  }  /* END MDBFetch() */ 


/**
 *Reset an executed statement to a state where it can be re-executed.
 * @param MStmt (O)
 */

int MDBResetStmt(

  mstmt_t *MStmt) /* O */

  {
  const char *FName = "MDBResetStmt";

  MDB(5,fSTRUCT) MLog("%s(MStmt)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3ResetStmt(MStmt->StmtUnion.SQLite3));

    case mdbODBC:
      {

      switch(MStmt->StmtType)
        {
        case mstmtPrepared:
          return(MODBCResetStmt(MStmt->StmtUnion.ODBC));

        case mstmtDirect:
          return(SUCCESS);

        default:
          return(FAILURE);

        }

      default:
        return(FAILURE);
      }
    }

  } /* END MDBResetStmt */ 




/**
 *Allocate a direct statement using StmtText and store it in MStmt
 * @param MDBInfo  (I)
 * @param MStmt    (O)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MDBAllocDirectStmt(

  mdb_t      *MDBInfo,  /* I */
  mstmt_t    *MStmt,    /* O */
  char const *StmtText, /* I */
  char       *EMsg)     /* I */

  {
  const char *FName = "MDBAllocDirectStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,StmtText,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      {
      return(MSQLite3AllocDirectStmt(MDBInfo->DBUnion.SQLite3,MStmt,StmtText,EMsg));
      }

    case mdbODBC:
      return(MODBCAllocDirectStmt(MDBInfo->DBUnion.ODBC,MStmt,StmtText,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInitDirectStmt */ 




/**
 *Allocate a prepared statement using StmtText and store it in MStmt
 * @param MDBInfo  (I)
 * @param MStmt    (O)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MDBAllocPrepStmt(

  mdb_t   *MDBInfo,  /* I */
  mstmt_t *MStmt,    /* O */
  const char    *StmtText, /* I */
  char    *EMsg)     /* I */

  {
  const char *FName = "MDBAllocPrepStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,%s,EMsg)\n",
    FName,
    StmtText);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      {
      return(MSQLite3AllocStmt(MDBInfo->DBUnion.SQLite3,MStmt,StmtText,EMsg));
      }

    case mdbODBC:
      return(MODBCAllocPrepStmt(MDBInfo->DBUnion.ODBC,MStmt,StmtText,EMsg));

    default:
      return(FAILURE);
    }

  } /* END MDBInitPrepStmt */




/**
 *Free the resources associated with the statement stored in MStmt,
 *and free the statement itself.
 * @param MStmt   (O)
 */

int MDBFreeStmt(

  mstmt_t *MStmt)   /* O */

  {
  int rc;

  const char *FName = "MDBFreeStmt";

  MDB(5,fSTRUCT) MLog("%s(MStmt)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      rc = MSQLite3FreeStmt(MStmt->StmtUnion.SQLite3);
      MUFree((char **)&MStmt->StmtUnion.SQLite3);
      break;

    case mdbODBC:
      if (MStmt->StmtUnion.ODBC != NULL)
        {
        rc = MODBCFreeStmt(MStmt->StmtUnion.ODBC);
        MUFree((char **)&MStmt->StmtUnion.ODBC);
        }
      else
        {
          rc = FAILURE;
        }
      break;

    default:
      rc = FAILURE;
    }
  return(rc);
  } /* END MDBFreeStmt */ 




/**
 *Returns the number of columns in the rows that this statement returns,
 *or 0 if this statement does not return any rows.
 * @param MStmt (I)
 * @param Out   (O)
 */

int MDBNumResultCols(

  mstmt_t *MStmt, /* I */
  int     *Out)   /* O */

  {
  const char *FName = "MDBNumResultCols";

  MDB(5,fSTRUCT) MLog("%s(MStmt,Out)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3NumResultCols(MStmt->StmtUnion.SQLite3,Out));

    case mdbODBC:
      return(MODBCNumResultCols(MStmt->StmtUnion.ODBC,Out));

    default:
      return(FAILURE);
    }
  } /* END MDBNumResultCols */ 




/**
 *Return the number of rows changed by the recently executed statement.
 *The result is undefined if the statement was not recently executed.
 *In general, this function only works for INSERT, UPDATE, and DELETE statements
 * @param MStmt (I)
 * @param Out   (O)
 */

int MDBNumChangedRows(

  mstmt_t *MStmt, /* I */
  int     *Out)   /* O */

  {
  const char *FName = "MDBNumChangedRows";

  MDB(5,fSTRUCT) MLog("%s(MStmt,Out)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3NumChangedRows(MStmt->StmtUnion.SQLite3,Out));

    case mdbODBC:
      return(MODBCNumChangedRows(MStmt->StmtUnion.ODBC,Out));

    default:
      return(FAILURE);
    }
  } /* END MDBNumChangedRows */ 




/**
 * returns SUCCESS and an error message in EMsg if there is an error 
 * associated with MStmt, and FAILURE otherwise.
 * @param MStmt    (I)
 * @param ExtraMsg (I)
 * @param EMsg     (O)
 */

int MDBStmtError(

  mstmt_t    *MStmt,    /* I */
  char const *ExtraMsg, /* I */
  char       *EMsg)     /* O */

  {
  enum MStatusCodeEnum SC;

  const char *FName = "MDBStmtError";

  MDB(5,fSTRUCT) MLog("%s(MStmt,ExtraMsg,EMsg)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3StmtError(MStmt->StmtUnion.SQLite3, ExtraMsg,EMsg));

    case mdbODBC:
      return(MODBCStmtError(MStmt->StmtUnion.ODBC, ExtraMsg,EMsg,&SC));

    default:
      return(FAILURE);
    }

  } /* END MDBStmtError */ 




/**
 *Connect to the database represented by mdb_t. For now, it should be safe to 
 *call this function even if a connection was already made previously.
 * @param MDBInfo (I)
 * @param EMsg    (O)
 */

int MDBConnect(

  mdb_t *MDBInfo, /* I */
  char  *EMsg)    /* O */

  {
  const char *FName = "MDBConnect";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3Connect(MDBInfo->DBUnion.SQLite3,EMsg));

    case mdbODBC:
      return(MODBCConnect(MDBInfo->DBUnion.ODBC,EMsg));

    case mdbNONE:
      return(SUCCESS);

    default:
      return(FAILURE);
    }

  } /* END MDBConnect */ 




/**
 *Free the resources associated with the database handle stored in MDBInfo.
 *
 * @param MDBInfo (I)
 */

int MDBFreeDPtr(

  mdb_t **MDBInfo) /* I */

  {
  const char *FName = "MDBFreeDPtr";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo)\n",
    FName);

  if (MDBInfo== NULL)
    {
    return(SUCCESS);
    }

  return (MDBFree(*MDBInfo));

  } /* END MDBFreeDPtr */








/**
 *Free the resources associated with the database handle stored in MDBInfo.
 * @param MDBInfo (I)
 */

int MDBFree(

  mdb_t *MDBInfo) /* I */

  {
  int rc;

  const char *FName = "MDBFree";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo)\n",
    FName);

  if ((MDBInfo == NULL) || (MDBInfo->DBType == mdbNONE))
    {
    return(SUCCESS);
    }

  if (MDBInTransaction(MDBInfo))
    {
    enum MStatusCodeEnum SC;

    MDBEndTransaction(MDBInfo,NULL,&SC);
    }

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3Free(MDBInfo->DBUnion.SQLite3);
      MUFree((char **)&MDBInfo->DBUnion.SQLite3);

      break;

    case mdbODBC:

      rc = MODBCFree(MDBInfo->DBUnion.ODBC);
      MUFree((char **)&MDBInfo->DBUnion.ODBC);

      break;

    default:
      rc = FALSE;
    }

  MDBInfo->WriteNodesInitialized = FALSE;
  MDBInfo->WriteJobsInitialized = FALSE;
  MDBInfo->WriteRsvInitialized = FALSE;
  MDBInfo->WriteTriggerInitialized = FALSE;
  MDBInfo->WriteVCInitialized = FALSE;
  MDBInfo->WriteRequestsInitialized = FALSE;
  MDBInfo->InsertCPInitialized = FALSE;

  MDBInfo->DBType = mdbNONE;
  return(rc);
  } /* END MDBFree */ 




/**
 * append a comma-delimited string of question marks useful for SQL statement
 * construction
 *
 * @param MString (I/O) string to append
 * @param NumQs   (I) Number of question marks to append
 */

int MDBAppendQMarks(

  mstring_t *MString,  /* I/O string to append */
  int const  NumQs)     /* I Number of question marks to append */

  {
  int count;

  const char *FName = "MDBAppendQMarks";

  MDB(5,fSTRUCT) MLog("%s(MString,NumQs)\n",
    FName);

  if (NumQs <= 0)
    return(SUCCESS);

  MStringAppend(MString,"?");

  for (count = 1; count < NumQs; count++)
    {
    MStringAppend(MString,",?");
    }

  return(SUCCESS);
  }



/**
 * initialize an event iterator for use with a database
 *
 * @param Itr (I)
 */

int MDBEItrInit(

 mevent_itr_t *Itr) /* I */

  {
  const char *FName = "MDBEItrInit";

  MDB(5,fSTRUCT) MLog("%s(Itr)\n",
    FName);

  Itr->Type = meiDB;
  Itr->TypeUnion.DBItr.MStmt = (mstmt_t *)MUCalloc(1,sizeof(Itr->TypeUnion.DBItr.MStmt[0]));

  return(SUCCESS);
  } /* END MDBEItrInit */




/**
 * Get the latest ID in a table.
 *
 * @param MDBInfo (I) database handle
 * @param Table   (I) Table name
 * @param Field   (I) Field Name
 * @param Out     (O) [required] receives the resulting ID on success, O on failure
 * @param EMsg    (O) [optional]
 */

int MDBGetLastID(

  mdb_t      *MDBInfo,  /* I database handle */
  char const *Table,    /* I Table name */
  char const *Field,    /* I Field Name */
  int        *Out,      /* O [required] receives the resulting ID on success, O on failure */
  char       *EMsg)     /* O [optional] */

  {
  enum MStatusCodeEnum SC = mscNoError;

  char StmtText[MMAX_LINE];
  mstmt_t MStmt;
  int Result = 0;
  int rc;

  const char *FName = "MDBGetLastID";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,Table,Field,Out,EMsg)\n",
    FName);

  snprintf(StmtText,sizeof(StmtText),"SELECT %s from %s ORDER BY %s DESC LIMIT 1",
      Field,Table,Field);

  if (MDBAllocDirectStmt(MDBInfo,&MStmt,StmtText,EMsg) == FAILURE)
    return(FAILURE);

  rc = MDBExecStmt(&MStmt,&SC);

  if (rc == FAILURE)
    {
    *Out = 0;
    }
  else
    {
    int fetchRC;
    MDBBindColInteger(&MStmt,1,&Result);
    fetchRC = MDBFetch(&MStmt);

    if (fetchRC == FAILURE)
      rc = FAILURE;
    else if (fetchRC == MDB_FETCH_NO_DATA)
      *Out = 0;
    else
      *Out = Result;
    }

  MDBFreeStmt(&MStmt);

  return(rc);
  }





/**
 * Clear resources associated with an iterator of type meiDB
 * @param Itr (I)
 */

int MDBEItrClear(

  mevent_itr_t *Itr) /* I */

  {
  const char *FName = "MDBEItrClear";

  MDB(5,fSTRUCT) MLog("%s(Itr)\n",
    FName);

  Itr->Type = meiNONE;

  MDBFreeStmt(Itr->TypeUnion.DBItr.MStmt);
  MUFree((char **)&Itr->TypeUnion.DBItr.MStmt);

  return(SUCCESS);
  }


int MDBQueryEventsWithRetry(

  mdb_t                     *MDBInfo,
  mulong                     StartTime,
  mulong                     EndTime,
  int                       *EID,
  enum MRecordEventTypeEnum *EType,
  enum MXMLOTypeEnum        *OType,
  char                       OID[][MMAX_LINE],
  mevent_itr_t              *Out,     /* O an unitialized iterator. This function will initialize it to a working iterator of type db */
  char                      *EMsg)

  {
  enum MStatusCodeEnum SC;

  mbool_t Retry;

  if (MDBInfo == NULL)
    {
    return(FAILURE);
    }

  do 
    {
    Retry = FALSE;

    SC = mscNoError;

    MDBQueryEvents(MDBInfo,StartTime,EndTime,EID,EType,OType,OID,Out,EMsg,&SC);

    if (MDBShouldRetry(SC) == TRUE)
      {
      MDB(7,fSOCK) MLog("INFO:    could not write object to database - Retrying\n");

      Retry = TRUE;

      MDBFree(MDBInfo);

      MDBInfo->DBType = MSched.ReqMDBType;

      MSysInitDB(MDBInfo);
      }
    } while (Retry == TRUE);

  return(SUCCESS);
  }  /* END MDBQueryEventsWithRetry() */





/**
 * Query events and retrieve the resulting executed statement. @see MSysQueryEvents.
 * @param MDBInfo  (I) 
 * @param StartTime (I) 
 * @param EndTime  (I)
 * @param EID      (I) [optional,terminated w/-1]
 * @param EType    (I) [optional,terminated w/mrelLAST]
 * @param OType    (I) [optional,terminated w/mxoLAST]
 * @param OID      (I) [optional]
 * @param Out      (I/O) Will contain the executed statement on success
 * @param EMsg     (O) [optional,minsize=MMAX_LINE]
 * @param SC       (O) (NOT OPTIONAL)
 */

int MDBQueryEvents(

  mdb_t                     *MDBInfo,
  mulong                     StartTime,
  mulong                     EndTime,
  int                       *EID,
  enum MRecordEventTypeEnum *EType,
  enum MXMLOTypeEnum        *OType,
  char                       OID[][MMAX_LINE],
  mevent_itr_t              *Out,     /* O an unitialized iterator. This function will initialize it to a working iterator of type db */
  char                      *EMsg,
  enum MStatusCodeEnum      *SC)

  {
  mstmt_t *MStmt;
  static const char * BaseSelectStmt =
    "SELECT ID,ObjectType,EventType,EventTime,ObjectName,Description FROM Events "
    "WHERE EventTime >= ? AND EventTime <= ?";

  int unsigned STime = (int unsigned) StartTime;
  int unsigned ETime = (int unsigned) EndTime;
  mbool_t useEID;
  mbool_t useEType;
  mbool_t useOType;
  mbool_t useOID;

  int NumEIDs = 0;
  int NumOIDs = 0;
  int NumETypes = 0;
  int NumOTypes = 0;

  int rc;
  int index;
  int ParamIndex = 0;

  const char *FName = "MDBQueryEvents";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%lu,%lu,EID,EType,OType,OID,Out,EMsg)\n",
    FName,
    StartTime,
    EndTime);

  if (SC == NULL)
    return(FAILURE);

  *SC = mscNoError;

  mstring_t QueryText(MMAX_LINE);;
  QueryText += BaseSelectStmt;

  MDBEItrInit(Out);
  MStmt = Out->TypeUnion.DBItr.MStmt;

  useEID = ((EID != NULL) && (EID[0] != -1));
  useEType = ((EType != NULL) && (EType[0] != mrelLAST));
  useOType = ((OType != NULL) && (OType[0] != mxoLAST));
  useOID = ((OID != NULL) && (OID[0][0] != 0));

  if (useEID)
    {
    int *End;

    for (End = EID; *End != -1; End++);

    NumEIDs = End - (int *)EID;
    MStringAppend(&QueryText," AND ID IN (");
    MDBAppendQMarks(&QueryText,NumEIDs);
    MStringAppend(&QueryText,")");
    }

  if (useEType)
    {
    int *End;

    for (End = (int *)EType; *End != mrelLAST; End++);

    NumETypes = End - (int *)EType;
    MStringAppend(&QueryText," AND EventType IN (");
    MDBAppendQMarks(&QueryText,NumETypes);
    MStringAppend(&QueryText,")");
    }

  if (useOType)
    {
    int *End;

    for (End = (int *)OType; *End != mxoLAST; End++);

    NumOTypes = End - (int *)OType;
    MStringAppend(&QueryText," AND ObjectType IN (");
    MDBAppendQMarks(&QueryText,NumOTypes);
    MStringAppend(&QueryText,")");
    }

  if (useOID)
    {
    char (*End)[MMAX_LINE];

    for (End = OID; (*End)[0] != 0; End++);

    NumOIDs = End - OID;
    MStringAppend(&QueryText," AND ObjectName IN (");
    MDBAppendQMarks(&QueryText,NumOIDs);
    MStringAppend(&QueryText,")");
    }

  QueryText += " ORDER BY EventTime ASC";

  MDBAllocPrepStmt(MDBInfo,MStmt,QueryText.c_str(),EMsg);

  MDBBindParamInteger(MStmt,++ParamIndex,(int *)&STime,FALSE);
  MDBBindParamInteger(MStmt,++ParamIndex,(int *)&ETime,FALSE);

  if (useEID)
    {
    for (index = 0; index < NumEIDs; index++)
      MDBBindParamInteger(MStmt,++ParamIndex,EID + index,FALSE);
    }

  if (useEType)
    {
    for (index = 0; index < NumETypes; index++)
      MDBBindParamInteger(MStmt,++ParamIndex,(int *)EType + index,FALSE);
    }

  if (useOType)
    {
    for (index = 0; index < NumOTypes; index++)
      MDBBindParamInteger(MStmt,++ParamIndex,(int *)OType + index,FALSE);
    }

  if (useOID)
    {
    for (index = 0;index < NumOIDs;index++)
      MDBBindParamText(MStmt,++ParamIndex,OID[index],0);
    }

  rc = MDBExecStmt(MStmt,SC);

  if (rc == FAILURE)
    MDBEItrClear(Out);

  return(rc);
  }  /* END MDBQueryEvents() */



/**
 * Bind an mevent_t struct to the columns of an MDBQueryEventsResultr
 *
 * @param MStmt (I/O) executed query statement with pending results
 * @param Event (I)   struct to be bound to the results of the next fetch
 */

int MDBBindEventColumns(
  mstmt_t *MStmt,
  mevent_t *Event)

  {
  int rc;

  const char *FName = "MDBBindEventColumns";

  MDB(5,fSTRUCT) MLog("%s(MStmt,Event)\n",
    FName);

  rc = MDBBindColInteger(MStmt,1,&Event->ID);
  rc &= MDBBindColInteger(MStmt,2,(int *)&Event->OType);
  rc &= MDBBindColInteger(MStmt,3,(int *)&Event->EType);
  rc &= MDBBindColInteger(MStmt,4,(int *)&Event->Time);
  rc &= MDBBindColText(MStmt,5,Event->Name,sizeof(Event->Name));
  rc &= MDBBindColText(MStmt,6,Event->Description,sizeof(Event->Description));

  return(rc);
  }



/**
 * Get the next event in Itr. Returns MITR_NONE if no more events are available
 *
 * @param Itr (I)
 * @param Out (O)
 */

int MDBEItrNext(

  mevent_itr_t *Itr,  /* I */
  mevent_t *Out)      /* O */

  {
  int fetchRC;

  const char *FName = "MDBEItrNext";

  MDB(5,fSTRUCT) MLog("%s(Itr,Out)\n",
    FName);

  MDBBindEventColumns(Itr->TypeUnion.DBItr.MStmt,Out);
  fetchRC = MDBFetch(Itr->TypeUnion.DBItr.MStmt);

  if (fetchRC == MDB_FETCH_NO_DATA)
    {
    return(MITR_DONE);
    }

  return(fetchRC);
  }





/**
 * Queries for the specified object.
 *
 * NOTE: if multiple entries exist only the first will be returned.
 * NOTE: make more dynamic and scalable!
 * @param MDBInfo    (I)
 * @param ObjectType (I)
 * @param ObjectName (I)
 * @param DataFilter (I)
 * @param NumResults (I)
 * @param Result     (O)
 * @param EMsg       (O)
 */

int MDBQueryCP(

  mdb_t         *MDBInfo,
  const char    *ObjectType,
  const char    *ObjectName,
  const char    *DataFilter,
  int            NumResults,
  mstring_t *    Result,
  char          *EMsg)

  {
  enum MStatusCodeEnum SC = mscNoError;

  long rc;

  int      Rows;
  int Columns;

  int  ColTimestamp = 0;

  mbool_t UsedDirectStmt = FALSE;
  mstmt_t MStmt;

  char    ColObjectType[MMAX_LINE];
  char    ColObjectName[MMAX_LINE];
  char    ColData[MMAX_BUFFER * 4];
  char    Statement[MMAX_LINE];

  int     index;

  const char *FName = "MDBQueryCP";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%s,%s,%s,%d,Result,EMsg)\n",
    FName,
    ObjectType,
    ObjectName,
    DataFilter,
    NumResults);

  if (MDBQueryCPStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  COND_FAIL((MDBInfo == NULL) || (Result == NULL));

  if (EMsg != NULL)
    EMsg[0] = '\0';


  MDB(3,fCKPT) MLog("INFO:      initializing DB connection\n");

  if (ObjectType == NULL || ObjectName == NULL || DataFilter == NULL)
    {
    /* build direct statement */

    snprintf(Statement,sizeof(Statement),"SELECT * FROM mcheckpoint %s %s%s%s%s %s %s%s%s%s %s %s%s%s;",
      ((ObjectType != NULL) || (ObjectName != NULL) || (DataFilter != NULL)) ? "where" : "",
      (ObjectType != NULL) ? "object_type=" : "",
      (ObjectType != NULL) ? "\'" : "",
      (ObjectType != NULL) ? ObjectType : "",
      (ObjectType != NULL) ? "\'" : "",
      ((ObjectType != NULL) && (ObjectName != NULL)) ? "and" : "",
      (ObjectName != NULL) ? "object_name=" : "",
      (ObjectName != NULL) ? "\'" : "",
      (ObjectName != NULL) ? ObjectName : "",
      (ObjectName != NULL) ? "\'" : "",
      (((ObjectType != NULL) || (ObjectName != NULL)) && (DataFilter != NULL)) ? "and" : "",
      (DataFilter != NULL) ? "data NOT LIKE \'" : "",
      (DataFilter != NULL) ? DataFilter : "",
      (DataFilter != NULL) ? "\'" : "");

    if (MDBAllocDirectStmt(MDBInfo,&MStmt,Statement,EMsg) == FAILURE)
      {
      return(FAILURE);
      }

    UsedDirectStmt = TRUE;
    }
  else 
    {
    MDBBindParamText(&MStmt,1,ObjectType,0);
    MDBBindParamText(&MStmt,2,ObjectName,0);
    MDBBindParamText(&MStmt,3,DataFilter,0);
    }

  /* bind colums to variables */

  MDBBindColText(&MStmt,1,ColObjectType,sizeof(ColObjectType));
  MDBBindColText(&MStmt,2,ColObjectName,sizeof(ColObjectName));
  MDBBindColInteger(&MStmt,3,&ColTimestamp);
  MDBBindColText(&MStmt,4,ColData,sizeof(ColData));
    
  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    if (UsedDirectStmt)
      MDBFreeStmt(&MStmt);
    return(FAILURE);
    }

  /* check column integrity */
  if (MDBNumResultCols(&MStmt,&Columns) == FAILURE)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"Error: could not determine number of columns\n");

    if (UsedDirectStmt)
      MDBFreeStmt(&MStmt);

    return(FAILURE);
    }

  if (Columns != 4)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"Error: columns do not match required (%d != 4)\n",Columns);

    /* invalid table format */

    if (UsedDirectStmt)
      MDBFreeStmt(&MStmt);

    return(FAILURE);
    }

  rc = MDBFetch(&MStmt);  

  index = 0;

  Rows = 0;
  while (rc != MDB_FETCH_NO_DATA && rc != FAILURE)
    {
    Rows++;
    if ((NumResults > 0) && (index >= NumResults))
      break;

    MStringAppendF(Result,"%s %s %d %s\n",
      ColObjectType,
      ColObjectName,
      ColTimestamp,
      ColData);

    rc = MDBFetch(&MStmt);  

    index++;
    }  

  if (UsedDirectStmt)
      MDBFreeStmt(&MStmt);

  MDB(3,fCKPT) MLog("INFO:      DB checkpoint data imported (%d rows/%d results)\n",
    Rows,
    NumResults);

  /* retrieve only first result */

    return(Rows > 0);
  } /* END MDBQueryCP */ 



/**
 * Deletes the specified object(s) from the mcheckpoint table.
 *
 * NOTE: either ObjectType or ObjectName or Timestamp may be NULL/0 but not all
 * @param MDBInfo    (I)
 * @param ObjectType (I)
 * @param ObjectName (I)
 * @param Timestamp  (I)
 * @param EMsg       (O)
 */

int MDBDeleteCP(

  mdb_t             *MDBInfo,
  const char        *ObjectType,
  const char        *ObjectName,
  long unsigned int  Timestamp,
  char              *EMsg)

  {
  enum MStatusCodeEnum SC = mscNoError;

  char TimeString[MMAX_NAME];
  char Statement[MMAX_LINE];

  mstmt_t MStmt;

  const char *FName = "MDBDeleteCP";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%s,%s,%lu,EMsg)\n",
    FName,
    ObjectType,
    ObjectName,
    Timestamp);

  if (MDBDeleteCPStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (ObjectType == NULL || ObjectName == NULL || Timestamp == 0)
    {
    TimeString[0] = '\0';

    snprintf(Statement,sizeof(Statement),"DELETE FROM mcheckpoint where %s%s%s%s %s %s%s%s%s %s %s%s;",
      (ObjectType != NULL) ? "object_type=" : "",
      (ObjectType != NULL) ? "\'" : "",
      (ObjectType != NULL) ? ObjectType : "",
      (ObjectType != NULL) ? "\'" : "",
      ((ObjectType != NULL) && (ObjectName != NULL)) ? "and" : "",
      (ObjectName != NULL) ? "object_name=" : "",
      (ObjectName != NULL) ? "\'" : "",
      (ObjectName != NULL) ? ObjectName : "",
      (ObjectName != NULL) ? "\'" : "",
      (((ObjectType != NULL) || (ObjectName != NULL)) && (Timestamp > 0)) ? "and" : "",
      (Timestamp > 0) ? "timestamp<" : "",
      (Timestamp > 0) ? TimeString : "");

    if (MDBAllocDirectStmt(MDBInfo,&MStmt,Statement,EMsg) == FAILURE)
      {
      return(FAILURE);
      }
    }
  else
    {
    MDBBindParamText(&MStmt,1,ObjectType,0);
    MDBBindParamText(&MStmt,2,ObjectName,0);
    MDBBindParamInteger(&MStmt,3,(int *)&Timestamp,0);
    }
  /* execute statement */

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    if (MStmt.StmtType == mstmtDirect)
      MDBFreeStmt(&MStmt);
    return(FAILURE);
    }

  if (MStmt.StmtType == mstmtDirect)
    MDBFreeStmt(&MStmt);

  return(SUCCESS);
  } /* END MDBDeleteCP */ 




/**
 * Inserts or updates the specified object.
 *
 * @param MDBInfo    (I)
 * @param ObjectType (I)
 * @param ObjectName (I)
 * @param TimeStamp  (I)
 * @param Data       (I)
 * @param EMsg       (O)
 * @param SC         (O)
 */

int MDBInsertCP(

  mdb_t                *MDBInfo,
  const char           *ObjectType,
  const char           *ObjectName,
  long unsigned int     TimeStamp,
  const char           *Data,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  static mstmt_t MStmt;

  static char  OTypeStatic[MMAX_LINE];
  static char  ONameStatic[MMAX_LINE];
  static long unsigned int TStampStatic;

  /* Need to change to a char[] instead of a mstring_t */

  static mstring_t DataStatic(MMAX_LINE);   /* Construct at load time */
                                            /* This is NEVER destructed */

  const char *DataStaticPtr;
  const char *NewDataStaticPtr;

  const char *FName = "MDBInsertCP";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%s,%s,%lu,%s,EMsg)\n",
    FName,
    ObjectType,
    ObjectName,
    TimeStamp,
    Data);

  if (MDBInfo == NULL)
    {
    return(FAILURE);
    }

  if (MDBInsertCPStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (MDBInfo->InsertCPInitialized == FALSE)
    {
    MDBBindParamText(&MStmt,1,OTypeStatic,0);
    MDBBindParamText(&MStmt,2,ONameStatic,0);
    MDBBindParamInteger(&MStmt,3,(int *)&TStampStatic,0);

    MDBBindParamText(&MStmt,4,DataStatic.c_str(),0);

    MDBInfo->InsertCPInitialized = TRUE;
    } /* END if (MDBInfo->InsertCPInitialized == FALSE) */

  /* DataStatic is an MString which could get a new address if realloc'ed
     so we need to check the address and rebind if it has changed.  */

  DataStaticPtr = DataStatic.c_str();

  MStringSet(&DataStatic,Data);

  NewDataStaticPtr = DataStatic.c_str();

  if (DataStaticPtr != NewDataStaticPtr)
    {
    /* Rebind the DataStatic because it's addressed changed */
    MDBBindParamText(&MStmt,4,DataStatic.c_str(),0);
    }

  MUStrCpy(OTypeStatic,ObjectType,MMAX_LINE);
  MUStrCpy(ONameStatic,ObjectName,MMAX_LINE);
  TStampStatic = TimeStamp;

  MDBDeleteCP(MDBInfo,ObjectType,ObjectName,0,NULL);

  MDBExecStmt(&MStmt,SC);
  MDBResetStmt(&MStmt);

  return(SUCCESS);
  } /* END MDBInsertCP */ 




/**
 * Remove entries whose last update time is older than Timestamp.
 * @param MDBInfo   (I)
 * @param Timestamp (I)
 * @param EMsg      (O)
 */

int MDBPurgeCP(

  mdb_t             *MDBInfo,   /* I */
  long unsigned int  Timestamp, /* I */
  char              *EMsg)      /* O */

  {
  enum MStatusCodeEnum SC = mscNoError;

  mstmt_t MStmt;

  const char *FName = "MDBPurgeCP";

  memset(&MStmt,0,sizeof(MStmt));

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%lu,EMsg)\n",
    FName,
    Timestamp);

  MDBPurgeCPStmt(MDBInfo,&MStmt);

  if ((MDBInfo == NULL) ||
      (Timestamp <= 0))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  MDBBindParamInteger(&MStmt,1,(int *)&Timestamp,FALSE);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  MDBResetStmt(&MStmt);

  return(SUCCESS);
  } /* END MDBPurgeCP */ 





/**
 * Queries the checkpoint DB for all records of internal jobs -- or those which
 * have not been migrated and are part of the internal RM's queue.
 *
 * @param MDBInfo    (I)
 * @param MaxResults (I)
 * @param Result     (I)
 * @param EMsg       (O)
 */

int MDBQueryInternalJobsCP(

  mdb_t    *MDBInfo,    /* I */
  int       MaxResults, /* I */
  marray_t *Result,     /* I */
  char     *EMsg)       /* O */

  {
  enum MStatusCodeEnum SC = mscNoError;

  int        rc;

  int  Rows;

  int Columns;

  char        ColObjectName[MMAX_LINE] = {0};

  int         index;
  mstmt_t MStmt;

  const char *FName = "MDBQueryInternalJobsCP";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%d,Result,EMsg)\n",
    FName,
    MaxResults);

  if ((MDBInfo == NULL) ||
      (Result == NULL))
    {
    return(FAILURE);
    }

  if (MDBIJobQueryStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindColText(&MStmt,1,ColObjectName,sizeof(ColObjectName));

  if (EMsg != NULL)
    EMsg[0] = '\0';

	
  /* build and execute statement */

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  /* check column integrity */

  if (MDBNumResultCols(&MStmt,&Columns) == FAILURE)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"Error: could not determine number of columns\n");

    return(FAILURE);
    }

  if (Columns != 1)
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"Error: columns do not match required (%d != 1)\n",Columns);

    /* invalid table format */

    return(FAILURE);
    }

  rc = MDBFetch(&MStmt);  

  index = 0;
  Rows = 0;
  while (rc != FAILURE && rc != MDB_FETCH_NO_DATA)
    {
    Rows++;
    if ((MaxResults > 0) && (index >= MaxResults))
      break;

    MUArrayListAppendPtr(Result,strdup(ColObjectName));

    rc = MDBFetch(&MStmt);  
    index++;
    }

  MDBResetStmt(&MStmt);

  return(Rows > 0);
  } /* END MDBQueryInternalJobsCP */ 




/**
 * Write generic metrics for an entire array.
 * @param MDBInfo   (I)
 * @param XLoadArray (I)
 * @param OType     (I)
 * @param Name      (I)
 * @param StartTime (I)
 * @param EMsg      (O)
 * @param SC        (O) error code (if any)
 */

int MDBWriteGenericMetricsArray(

  mdb_t                *MDBInfo,
  double               *XLoadArray,
  enum MXMLOTypeEnum    OType,
  char const           *Name,
  long unsigned int     StartTime,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  static const char *StmtText = "INSERT INTO GenericMetrics (ObjectType,ObjectName,GMetricName,Time,GMetricValue) VALUES (?,?,?,?,?);";

  int index;
  int NumMetrics;

  mstmt_t MStmt;

  char   GMetricName[MMAX_NAME];

  double XLoad;

  const char *FName = "MDBWriteGenericMetricsArray";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,XLoad,OType,Name,%lu,EMsg)\n",
    FName,
    StartTime);

  if (NULL == SC)
    {
    return(FAILURE);
    }

  for (NumMetrics = 0; NumMetrics < MSched.M[mxoxGMetric] - 1; NumMetrics++)
    {
    if (MGMetric.Name[NumMetrics + 1][0] == '\0')
      break;
    }

  if (NumMetrics <= 0)
    {
    /* No generic metrics to write */

    return(SUCCESS);
    }

  for (index = 1;index < NumMetrics;index++)
    {
    if (XLoadArray[index] == 0.0)
      continue;

    if (MDBAllocDirectStmt(MDBInfo,&MStmt,StmtText,EMsg) == FAILURE)
      {
      return(FAILURE);
      }

    MDBBindParamInteger(&MStmt,1,(int *)&OType,FALSE);
    MDBBindParamText(&MStmt,2,Name,0);
    MDBBindParamText(&MStmt,3,GMetricName,0);
    MDBBindParamInteger(&MStmt,4,(int *)&StartTime,FALSE);
    MDBBindParamDouble(&MStmt,5,&XLoad,FALSE);

    MUStrCpy(GMetricName,MGMetric.Name[index],sizeof(GMetricName));
    XLoad = XLoadArray[index];

    MDBFreeStmt(&MStmt);
    }

  return(SUCCESS);
  } /* END MDBWriteGenericMetricsArray */ 




/**
 * Bind NodeStatsGenericResources field for fetching 
 * @param MStmt           (I)
 * @param GMetricName     (I)
 * @param GMetricNameSize (I)
 * @param GMetricValue    (I)
 * @param GMetricTime     (I)
 * @param EMsg            (O)
 */

int MDBBindGMetricsRead(

  mstmt_t           *MStmt,
  char              *GMetricName,
  int                GMetricNameSize,
  double            *GMetricValue,
  long unsigned int *GMetricTime,
  char              *EMsg)

  {
  const char *FName = "MDBBindGMetricsRead";

  MDB(5,fSTRUCT) MLog("%s(MStmt,GMetricName,GMetricNameSize,GMetricValue,GMetricTime,EMsg)\n",
    FName);

  if (GMetricName != NULL)
    MDBBindColText(MStmt,1,GMetricName,GMetricNameSize);

  if (GMetricValue != NULL)
    MDBBindColDouble(MStmt,2,GMetricValue);

  if (GMetricTime != NULL)
    MDBBindColInteger(MStmt,3,(int *)GMetricTime);

  return(SUCCESS);
  } /* END MDBBindGMetricsRead */ 





/**
 * Read all generic metrics for a stastistic from table GenericMetrics
 *
 * @param MDBInfo  (I)
 * @param XLoad    (I)
 * @param OType    (I)
 * @param Name     (I)
 * @param StatTime (I)
 * @param EMsg     (O)
 */

int MDBReadGenericMetrics(

  mdb_t              *MDBInfo,  /* I */
  double             *XLoad,    /* I */
  enum MXMLOTypeEnum  OType,    /* I */
  char               *Name,     /* I */
  long unsigned int   StatTime, /* I */
  char               *EMsg)     /* O */

  {
  enum MStatusCodeEnum SC;

  double GMetricValue = 0.0;
  char   GMetricName[MMAX_NAME];

  mstmt_t MStmt;

  int     rc;

  const char *FName = "MDBReadGenericMetrics";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,XLoad,OType,%s,%lu,EMsg)\n",
    FName,
    Name,
    StatTime);

  if (MDBSelectGenericMetricsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamInteger(&MStmt,1,(int *)&OType,FALSE);
  MDBBindParamText(&MStmt,2,Name,0);
  MDBBindParamInteger(&MStmt,3,(int *)&StatTime,FALSE);
  MDBBindGMetricsRead(&MStmt,GMetricName,sizeof(GMetricName),&GMetricValue,NULL,EMsg);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  rc = MDBFetch(&MStmt);

  while (rc != MDB_FETCH_NO_DATA && rc != FAILURE)
    {
    MStatInsertGenericMetric(XLoad,GMetricName,GMetricValue);

    rc = MDBFetch(&MStmt);
    }

  MDBResetStmt(&MStmt);

  return(SUCCESS);
  }  /* END MDBReadGenericMetrics() */ 





/**
 * Read in a range of generic metrics.
 *
 * @param MDBInfo   (I)
 * @param IStat     (O)
 * @param OType     (I)
 * @param OID       (I)
 * @param Duration  (I)
 * @param StartTime (I)
 * @param EndTime   (I)
 * @param EMsg      (O)
 */

int MDBReadGenericMetricsRange(

  mdb_t              *MDBInfo,   /* I */
  void *             *IStat,     /* O */
  enum MXMLOTypeEnum  OType,     /* I */
  char               *OID,       /* I */
  long unsigned int   Duration,  /* I */
  long unsigned int   StartTime, /* I */
  long unsigned int   EndTime,   /* I */
  char               *EMsg)      /* O */

  {
  enum MStatusCodeEnum SC = mscNoError;

  double GMetricValue = 0.0;
  char GMetricName[MMAX_NAME];
  mulong GMetricTime = 0;
  ldiv_t StatIndex;

  mstmt_t MStmt;

  int     rc;

  const char *FName = "MDBReadGenericMetricsRange";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,IStat,OType,%s,%lu,%lu,%lu,EMsg)\n",
    FName,
    OID,
    Duration,
    StartTime,
    EndTime);

  if (MDBSelectGenericMetricsRangeStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamInteger(&MStmt,1,(int *)&OType,FALSE);
  MDBBindParamText(&MStmt,2,OID,0);
  MDBBindParamInteger(&MStmt,3,(int *)&StartTime,FALSE);
  MDBBindParamInteger(&MStmt,4,(int *)&EndTime,FALSE);
  MDBBindGMetricsRead(&MStmt,GMetricName,sizeof(GMetricName),&GMetricValue,&GMetricTime,EMsg);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  rc = MDBFetch(&MStmt);

  while ((rc != FAILURE) && (rc != MDB_FETCH_NO_DATA))
    {
    StatIndex = ldiv((long) (GMetricTime - StartTime),(long)Duration);

    if (StatIndex.rem == 0)
      {
      /* Only accept stats that are exactly on the profile interval */

      MStatInsertGenericMetric(MStatExtractXLoad(IStat[StatIndex.quot],OType),GMetricName,GMetricValue);
      }

    rc = MDBFetch(&MStmt);
    }

  if (rc != MDB_FETCH_NO_DATA)
    {
    MDBStmtError(&MStmt,NULL,EMsg);
    }

  MDBResetStmt(&MStmt);

  return(SUCCESS);
  }  /* END MDBReadGenericMetricsRange() */ 





/**
 * Write record for table GeneralStats.
 *
 * @param MDBInfo (I)
 * @param IStat   (I)
 * @param Name    (I)
 * @param EMsg    (O)
 * @param SC      (O) error code (if any)
 */

int MDBWriteGeneralStats(

  mdb_t                *MDBInfo,
  must_t               *IStat,
  char const           *Name,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  must_t *GS;

  mstmt_t MStmt;

  int ParamIndex;

  /* NOTE: the following is the schema, in order, of the database */

  int ObjectType;
  char ObjectName[MMAX_LINE];
  int Time;
  int StatDuration;
  int IterationCount;
  int IntervalStatCount;
  int IntervalStatDuration;
  int TotalJobCount;
  int JobsSubmitted;
  int JobsActive;
  int JobsEligible;
  int JobsIdle;
  int JobsCompletedByQOS;
  int JobsQueued;
  int JobsPreempted;
  double TotalJobAccuracy;
  double TotalNodeLoad;
  double TotalNodeMemory;
  int TotalNodeCount;
  int TotalAllocatedNodeCount;
  int TotalActiveNodeCount;
  int TotalActiveProcCount;
  int TotalConfiguredProcCount;
  int TotalAvailableCapacity;
  double ProcSecondsDedicated;
  double ProcSecondsExecuted;
  double ProcSecondsRequested;
  double ProcSecondsUtilized;
  double ProcHoursPreempted;
  double ProcHoursSubmitted;
  double ProcHoursQueued;
  double ProcHoursAvailable;
  double ProcHoursDedicated;
  double ProcHoursSuccessful;
  double ProcHoursUtilized;
  double DiskSecondsAvailable;
  double DiskSecondsDedicated;
  double DiskSecondsUtilized;
  double DiskSecondsUtilizedInstant;
  double MemSecondsAvailable;
  double MemSecondsDedicated;
  double MemSecondsUtilized;
  int AvgBypass;
  int MaxBypass;
  int TotalBypass;
  double AvgXFactor;
  double MaxXFactor;
  double AvgQueueTime;
  int MaxQueueTime;
  int TotalQueueTime;
  double TotalRequestedWalltime;
  double TotalExecutionWalltime;
  double MinEfficiency;
  double MinEfficiencyDuration;
  int StartedJobCount;
  int StartedJobsQueueTime;
  int StartedJobsProcCount;
  double StartedJobsXFactor;
  double TotalNodeWeightedXFactor;
  double TotalNodeWeightedJobAccuracy;
  double QOSCredits;
  double AppBacklog;
  double AppLoad;
  double AppResponseTime;
  double AppThroughput;
  double TotalXFactor;
  char   Other[MMAX_LINE];

  int TPC;
  int TNC;

  const char *FName = "MDBWriteGeneralStats";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,IStat,Name,EMsg)\n",
    FName);

  if (NULL == SC)
    {
    return(FAILURE);
    }

  if ((IStat->Index != -1) && 
      (IStat->Index < MStat.P.MaxIStatCount) && 
      (MPar[0].S.IStat != NULL))
    GS = MPar[0].S.IStat[IStat->Index];
  else
    GS = &MPar[0].S;

  TPC = 0;

  if (IStat->TPC > 0)
    {
    TPC = IStat->TPC;
    }
  else if ((GS != NULL) && (GS->TPC > 0) && (GS->StartTime == IStat->StartTime))
    {
    TPC = GS->TPC;
    }
  else
    {
    TPC = (int)(MPar[0].UpRes.Procs * (MStat.P.IStatDuration)/ MAX(1,MSched.MaxRMPollInterval));
    }

  TNC = 0;

  if (IStat->TNC > 0)
    {
    TNC = IStat->TNC;
    }
  else if ((GS != NULL) && (GS->TNC > 0) && (GS->StartTime == IStat->StartTime))
    {
    TNC = GS->TNC;
    }

  MDB(7,fSOCK) MLog("INFO:     prepping general stats statement\n");

  if (MDBInsertGeneralStatsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    MDB(7,fSOCK) MLog("INFO:     error prepping general stats statement\n");
    return(FAILURE);
    }

  ParamIndex = 1;

  MDBBindParamInteger(&MStmt,ParamIndex++,&ObjectType,FALSE);
  MDBBindParamText(&MStmt,ParamIndex++,ObjectName,0);
  MDBBindParamInteger(&MStmt,ParamIndex++,&Time,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&StatDuration,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&IterationCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&IntervalStatCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&IntervalStatDuration,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalJobCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&JobsSubmitted,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&JobsActive,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&JobsEligible,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&JobsIdle,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&JobsCompletedByQOS,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&JobsQueued,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&JobsPreempted,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalJobAccuracy,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalNodeLoad,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalNodeMemory,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalNodeCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalAllocatedNodeCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalActiveNodeCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalActiveProcCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalConfiguredProcCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalAvailableCapacity,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcSecondsDedicated,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcSecondsExecuted,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcSecondsRequested,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcSecondsUtilized,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcHoursPreempted,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcHoursSubmitted,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcHoursQueued,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcHoursAvailable,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcHoursDedicated,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcHoursSuccessful,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&ProcHoursUtilized,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&DiskSecondsAvailable,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&DiskSecondsDedicated,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&DiskSecondsUtilized,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&DiskSecondsUtilizedInstant,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&MemSecondsAvailable,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&MemSecondsDedicated,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&MemSecondsUtilized,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&AvgBypass,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&MaxBypass,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalBypass,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&AvgXFactor,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&MaxXFactor,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&AvgQueueTime,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&MaxQueueTime,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&TotalQueueTime,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalRequestedWalltime,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalExecutionWalltime,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&MinEfficiency,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&MinEfficiencyDuration,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&StartedJobCount,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&StartedJobsQueueTime,FALSE);
  MDBBindParamInteger(&MStmt,ParamIndex++,&StartedJobsProcCount,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&StartedJobsXFactor,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalNodeWeightedXFactor,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalNodeWeightedJobAccuracy,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&QOSCredits,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&AppBacklog,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&AppLoad,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&AppResponseTime,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&AppThroughput,FALSE);
  MDBBindParamDouble(&MStmt,ParamIndex++,&TotalXFactor,FALSE);
  MDBBindParamText(&MStmt,ParamIndex++,Other,0);

  ObjectType = IStat->OType;
  MUStrCpy(ObjectName,Name,sizeof(ObjectName));
  Time = IStat->StartTime;
  StatDuration = IStat->Duration;
  IterationCount = IStat->IterationCount;
  IntervalStatCount = IStat->IStatCount;
  IntervalStatDuration = 0;
  TotalJobCount = IStat->JC;
  JobsSubmitted = IStat->SubmitJC;
  JobsActive = 0;
  JobsEligible = 0;
  JobsIdle = IStat->ISubmitJC;
  JobsCompletedByQOS = IStat->QOSSatJC;
  JobsQueued = IStat->TQueuedJC;
  JobsPreempted = 0;
  TotalJobAccuracy = IStat->JobAcc;
  TotalNodeLoad = IStat->TNL;
  TotalNodeMemory = IStat->TNM;
  TotalNodeCount = TNC;
  TotalAllocatedNodeCount = IStat->JAllocNC;
  TotalActiveNodeCount = IStat->ActiveNC;
  TotalActiveProcCount = IStat->ActivePC;
  TotalConfiguredProcCount = TPC;
  TotalAvailableCapacity = 0;
  ProcSecondsDedicated = IStat->PSDedicated;
  ProcSecondsExecuted = IStat->PSRun;
  ProcSecondsRequested = IStat->PSRequest;
  ProcSecondsUtilized = 0.0;
  ProcHoursPreempted = 0.0;
  ProcHoursSubmitted = IStat->SubmitPHRequest;
  ProcHoursQueued = IStat->TQueuedPH;
  ProcHoursAvailable = 0.0;
  ProcHoursDedicated = 0.0;
  ProcHoursSuccessful = 0.0;
  ProcHoursUtilized = IStat->PSUtilized;
  DiskSecondsAvailable = IStat->DSAvail;
  DiskSecondsDedicated = 0.0;
  DiskSecondsUtilized = IStat->DSUtilized;
  DiskSecondsUtilizedInstant = IStat->IDSUtilized;
  MemSecondsAvailable = IStat->MSAvail;
  MemSecondsDedicated = IStat->MSDedicated;
  MemSecondsUtilized = IStat->MSUtilized;
  AvgBypass = 0;
  MaxBypass = IStat->MaxBypass;
  TotalBypass = IStat->Bypass;
  AvgXFactor = IStat->AvgXFactor;
  MaxXFactor = IStat->MaxXFactor;
  AvgQueueTime = IStat->AvgQTime;
  MaxQueueTime = IStat->MaxQTS;
  TotalQueueTime = IStat->TotalQTS;
  TotalRequestedWalltime = IStat->TotalRequestTime;
  TotalExecutionWalltime = IStat->TotalRunTime;
  MinEfficiency  = 0.0;
  MinEfficiencyDuration = 0.0;
  StartedJobCount = IStat->StartJC;
  StartedJobsQueueTime = IStat->StartQT;
  StartedJobsProcCount = IStat->StartPC;
  StartedJobsXFactor = IStat->StartXF;
  TotalNodeWeightedXFactor = IStat->NXFactor;
  TotalNodeWeightedJobAccuracy = IStat->NJobAcc;
  QOSCredits = IStat->QOSCredits;
  AppBacklog = 0.0;
  AppLoad = 0.0;
  AppResponseTime = 0.0;
  AppThroughput = 0.0;
  TotalXFactor = IStat->XFactor;
  Other[0] = '\0';

  MDB(7,fTRANS) MLog("INFO:     writing general stats object '%s' to database\n",
    ObjectName);

  if (MDBExecStmt(&MStmt,SC) == FAILURE)
    {
    MDB(7,fSOCK) MLog("INFO:     failure executing general stats statement\n");
    return(FAILURE);
    }

  MDBResetStmt(&MStmt);
    
  if (MDBWriteGenericMetricsArray(MDBInfo,IStat->XLoad,IStat->OType,Name,IStat->StartTime,EMsg,SC) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MDBWriteGeneralStats */ 




/**
 * Read record from table GeneralStats
 * @param MStmt    (I)
 * @param IStat    (I)
 * @param StmtText (I)
 * @param EMsg     (O)
 */

int MDBBindGeneralStat(

  mstmt_t *MStmt,
  must_t  *IStat,
  char    *StmtText,
  char    *EMsg)

  {
  int ParamIndex = 1;

  const char *FName = "MDBBindGeneralStat";

  MDB(5,fSTRUCT) MLog("%s(MStmt,IStat,%s,EMsg)\n",
    FName,
    StmtText);

  MDBBindColInteger(MStmt,ParamIndex++,(int *)&IStat->OType);
  ParamIndex++; /* Name */
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&IStat->StartTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&IStat->Duration);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->IterationCount);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->IStatCount);
  ParamIndex++; /* IntervalStatDuration */
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->JC);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->SubmitJC);
  ParamIndex++; /* JobsActive */
  ParamIndex++; /* JobsEligible */
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->ISubmitJC);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->QOSSatJC);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->TQueuedJC);
  ParamIndex++; /* JobsPreempted */
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->JobAcc);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->TNL);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->TNM);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->TNC);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->JAllocNC);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->ActiveNC);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->ActivePC);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->TPC);
  ParamIndex++; /* TotalAvailableCapacity */
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->PSDedicated);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->PSRun);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->PSRequest);
  ParamIndex++; /* ProcSecondsUtilized */
  ParamIndex++; /* ProcHoursPreempted */
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->SubmitPHRequest);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->TQueuedPH);
  ParamIndex++; /* ProcHoursAvailable */
  ParamIndex++; /* ProcHoursDedicated */
  ParamIndex++; /* ProcHoursSuccessful */
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->PSUtilized);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->DSAvail);
  ParamIndex++; /* DiskSecondsDedicated */
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->DSUtilized);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->IDSUtilized);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->MSAvail);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->MSDedicated);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->MSUtilized);
  ParamIndex++; /* AvgBypass */
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->MaxBypass);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->Bypass);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->AvgXFactor);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->MaxXFactor);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->AvgQTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&IStat->MaxQTS);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&IStat->TotalQTS);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->TotalRequestTime);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->TotalRunTime);
  ParamIndex++; /* MinEfficiency */
  ParamIndex++; /* MinEfficiencyDuration */
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->StartJC);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&IStat->StartQT);
  MDBBindColInteger(MStmt,ParamIndex++,&IStat->StartPC);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->StartXF);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->NXFactor);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->NJobAcc);
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->QOSCredits);
  ParamIndex++; /* AppBacklog */
  ParamIndex++; /* AppLoad */
  ParamIndex++; /* AppResponseTime */
  ParamIndex++; /* AppThroughput */
  MDBBindColDouble(MStmt,ParamIndex++,&IStat->XFactor);
  ParamIndex++; /* Other */

  return(SUCCESS);
  } /* END MDBBindGeneralStat() */ 





/**
 * Read record from table GeneralStats.
 *
 * @param MDBInfo  (I)
 * @param IStat    (I)
 * @param OType    (I)
 * @param Name     (I)
 * @param StatTime (I)
 * @param EMsg     (O)
 */

int MDBReadGeneralStats(

  mdb_t              *MDBInfo,
  must_t             *IStat,
  enum MXMLOTypeEnum  OType,
  char               *Name,
  long unsigned int   StatTime,
  char               *EMsg)

  {
  enum MStatusCodeEnum SC;
  mstmt_t MStmt;

  const char *FName = "MDBReadGeneralStats";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,IStat,OType,%s,%lu,EMsg)\n",
    FName,
    Name,
    StatTime);

  if (MDBSelectGeneralStatsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamInteger(&MStmt,1,(int *)&OType,FALSE);
  MDBBindParamText(&MStmt,2,Name,FALSE);
  MDBBindParamInteger(&MStmt,3,(int *)&StatTime,FALSE);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }
  
  MDBBindGeneralStat(&MStmt,IStat,NULL,EMsg);

  if (MDBFetch(&MStmt) == MDB_FETCH_NO_DATA)
    {
    return(FAILURE);
    }

  MDBResetStmt(&MStmt);

  if (MDBReadGenericMetrics(MDBInfo,IStat->XLoad,OType,Name,StatTime,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBReadGeneralStats() */ 




/**
 * Allocates and fills Stat->IStat and sets Stat->IStatCount = 
 * (EndTime - StartTime) / Duration.
 *
 * Duration is used as the time interval to search in the database with, such 
 * that * Stat->IStat[i]->StartTime = (StartTime + i * Duration).
 *
 * For each time interval where no statistic is found, the corresponding array 
 * member value is NULL.
 *
 * @param MDBInfo   (I)
 * @param Stat      (I)
 * @param OType     (I)
 * @param OID       (I)
 * @param Duration  (I)
 * @param StartTime (I)
 * @param EndTime   (I)
 * @param EMsg      (O)
 */

int MDBReadGeneralStatsRange(

  mdb_t              *MDBInfo,
  must_t             *Stat,
  enum MXMLOTypeEnum  OType,
  char               *OID,
  int                 Duration,
  int                 StartTime,
  int                 EndTime,
  char               *EMsg)

  {
  enum MStatusCodeEnum SC;

  must_t *IStat = NULL;

  int index;
  int ArrSize;
  int rc = SUCCESS;

  ldiv_t Index;
  mstmt_t MStmt;

  const char *FName = "MDBReadGeneralStatsRange";

  MDB(3,fSCHED) MLog("%s(MDBInfo,Stat,%s,%s,%d,%d,%d,EMsg)\n",
    FName,
    MXO[OType],
    (OID != NULL) ? OID : "",
    Duration,
    StartTime,
    EndTime);

  if (MDBSelectGeneralStatsRangeStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamInteger(&MStmt,1,(int *)&OType,FALSE);
  MDBBindParamText(&MStmt,2,OID,0);
  MDBBindParamInteger(&MStmt,3,(int *)&StartTime,FALSE);
  MDBBindParamInteger(&MStmt,4,(int *)&EndTime,FALSE);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  ArrSize = (EndTime - StartTime) / Duration + 1; /* Need to add 1 because of inclusiveness of EndTime */

  Stat->IStat = (must_t **)MUCalloc(ArrSize,sizeof(must_t *));

  Stat->IStatCount = ArrSize;

  MStatCreate((void **)&IStat,msoCred);

  while (TRUE) 
    {
    MDBBindGeneralStat(&MStmt,IStat,NULL,EMsg);

    if (MDBFetch(&MStmt) == MDB_FETCH_NO_DATA)
      break;
    
    Index = ldiv((long)(IStat->StartTime - (long) StartTime),(long)Duration);

    if (Index.rem == 0)
      {
      /* Only accept stats that are exactly on the profile interval */

      IStat->Index = Index.quot;

      Stat->IStat[Index.quot] = IStat;

      MStatCreate((void **)&IStat,msoCred);
      }
    }   /* END while (TRUE) */

  MStatFree((void **)&IStat,msoCred);

  MDBResetStmt(&MStmt);;

  for (index = 0;index < ArrSize;index++)
    {
    if (Stat->IStat[index] == NULL)
      {
      MStatCreate((void **)&Stat->IStat[index],msoCred);

      MStatPInit( 
        Stat->IStat[index], 
        msoCred, 
        index, 
        StartTime + (Duration * index));

      Stat->IStat[index]->OType = OType;
      }
    }    /* END for (index) */

  if (MDBReadGenericMetricsRange(MDBInfo,(void **)Stat->IStat,OType,OID,Duration,StartTime,EndTime,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MStatAddLastFromMemory(OType,OID,Stat,EndTime);

  return(rc);
  } /* END MDBReadGeneralStatsRange */ 




/**
 * Write record for table NodeStatsGenericResources.
 *
 * @param MDBInfo    (I)
 * @param NodeID     (I)
 * @param GResName   (I)
 * @param Time       (I)
 * @param AGRes      (I)
 * @param CGRes      (I)
 * @param DGRes      (I)
 * @param IterationCount (I)
 * @param EMsg       (O)
 * @param SC         (O) error code (if any)
 */

int MDBWriteNodeStatsGenericResourcesArray(

  mdb_t                *MDBInfo,
  char const           *NodeID,
  char const           *GResName,
  long unsigned int     Time,
  msnl_t               *AGRes,
  msnl_t               *CGRes,
  msnl_t               *DGRes,
  int                   IterationCount,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  static const char *StmtText = "INSERT INTO NodeStatsGenericResources (NodeID,GResName,Time,GenericResourcesAvailable,GenericResourcesConfigured,GenericResourcesDedicated) VALUES (?,?,?,?,?,?);";

  int NumResources;
  int rc = SUCCESS;

  char Name[MMAX_NAME];

  int ARes;
  int CRes;
  int DRes;
  int index;

  mstmt_t MStmt;

  const char *FName = "MDBWriteNodeStatsGenericResourcesArray";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,NodeID,GResName,%lu,GenericResourcesAvailable,GenericResourcesConfigured,GenericResourcesDedicated,EMsg)\n",
    FName,
    Time);

  if (NULL == SC)
    {
    return(FAILURE);
    }

  for (NumResources = 1;NumResources < MSched.M[mxoxGRes];NumResources++)
    {
    if (MGRes.Name[NumResources][0] == '\0')
      break;
    }

  if (NumResources <= 0)
    {
    /* No generic resources to write */
    return(SUCCESS);
    }

  MDB(7,fSOCK) MLog("INFO:     prepping node generic resources statement\n");

  if (MDBAllocDirectStmt(MDBInfo,&MStmt,StmtText,EMsg) == FAILURE)
    {
    MDB(7,fSOCK) MLog("INFO:     error prepping node generic resources statement\n");
    return(FAILURE);
    }

  MDBBindParamText(&MStmt,1,NodeID,0);
  MDBBindParamText(&MStmt,2,Name,0);
  MDBBindParamInteger(&MStmt,3,(int *)&Time,FALSE);
  MDBBindParamInteger(&MStmt,4,(int *)&ARes,FALSE);
  MDBBindParamInteger(&MStmt,5,(int *)&CRes,FALSE);
  MDBBindParamInteger(&MStmt,6,(int *)&DRes,FALSE);

  for (index = 1;index < NumResources;index++)
    {
    if ((CRes = MSNLGetIndexCount(CGRes,index)) > 0)
      {
      MUStrCpy(Name,MGRes.Name[index],sizeof(Name));
  
      ARes = (MSNLGetIndexCount(AGRes,index)) / IterationCount;
      DRes = (MSNLGetIndexCount(DGRes,index)) / IterationCount;
  
      MDB(7,fTRANS) MLog("INFO:     writing generic resource '%s' for node '%s' to database\n",
        Name,
        NodeID);

      if ((rc = MDBExecStmt(&MStmt,SC)) == FAILURE)
        {
        MDB(7,fSOCK) MLog("INFO:     failure executing node generic resources statement\n");
        }
      }
    }

  MDBFreeStmt(&MStmt);

  return(rc);
  } /* END MDBWriteNodeStatsGenericResourcesArray */ 




/**
 * Get the description from a particular a type of Event. Currently only used
 * for testing purposes.
 * 
 * @param MDBInfo  (I)
 * @param OType    (I)
 * @param OID      (I)
 * @param EType    (I)
 * @param Time     (I)
 * @param Array    (O)
 * @param ArrSize  (I)
 *
 */

int MDBGetEventMessages(

  mdb_t                     *MDBInfo,
  enum MXMLOTypeEnum         OType,
  char                      *OID,
  enum MRecordEventTypeEnum  EType,
  int unsigned               Time,
  char                     **Array,
  int const                  ArrSize)

  {
  enum MStatusCodeEnum SC;

  mstmt_t MStmt;
  int rc = SUCCESS;
  int index;

  const char *FName = "MDBGetEventMessages";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,OType,%s,EType,%u,Array,ArrSize)\n",
    FName,
    OID,
    Time);

  if (MDBSelectEventsMsgsStmt(MDBInfo,&MStmt,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamText(&MStmt,1,OID,0);
  MDBBindParamInteger(&MStmt,2,(int *)&OType,FALSE);
  MDBBindParamInteger(&MStmt,3,(int *)&EType,FALSE);
  MDBBindParamInteger(&MStmt,4,(int *)&Time,FALSE);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  for (index = 0; index < ArrSize - 1; index++)
    {
    char DescLine[MMAX_LINE];
    int fetchRC;

    MDBBindColText(&MStmt,1,DescLine,sizeof(DescLine));

    fetchRC = MDBFetch(&MStmt);

    if (fetchRC == MDB_FETCH_NO_DATA)
      {
      break;
      }
    else if (fetchRC == FAILURE)
      {
      rc = FAILURE;
      break;
      }

    Array[index] = strdup(DescLine);
    }

  Array[index] = NULL;

  MDBResetStmt(&MStmt);

  return (rc);
  }  /* END MDBGetEventMessages() */


/**
 * Wrapper for writing database objects (operational, not events or statistics), includes retry logic.
 *
 * NOTE: this routine is not threadsafe, it should only be called
 *       by the MOTransitionFromQueue() thread.
 *
 * @param   MDBP    (I) the db info struct
 * @param   O       (I) 
 * @param   OType   (I) 
 * @param   Msg     (O) any output error message
 */

int MDBWriteObjectsWithRetry(

  mdb_t                     *MDBP,
  void                     **O,
  enum MXMLOTypeEnum         OType,
  char                      *Msg)

  {
  enum MStatusCodeEnum SC;

  mbool_t Retry;

  if ((MDBP == NULL) || (O == NULL))
    {
    return(FAILURE);
    }

  if (!bmisset(&MSched.RealTimeDBObjects,OType))
    {
    /* this object type should not be written */

    return(SUCCESS);
    }

  do 
    {
    Retry = FALSE;

    SC = mscNoError;

    switch (OType)
      {
      case mxoNode: MDBWriteNodes(MDBP,(mtransnode_t **)O,NULL,&SC); break;
      case mxoJob:  MDBWriteJobs(MDBP,(mtransjob_t **)O,NULL,&SC); break;
      case mxoTrig: MDBWriteTrigger(MDBP,(mtranstrig_t **)O,NULL,&SC); break;
      case mxoxVC:  MDBWriteVC(MDBP,(mtransvc_t **)O,NULL,&SC); break;
      case mxoRsv:  MDBWriteRsv(MDBP,(mtransrsv_t **)O,NULL,&SC); break;
      default: /* NO-OP */ break;
      }

    if (MDBShouldRetry(SC) == TRUE)
      {
      MDB(7,fSOCK) MLog("INFO:    could not write object to database - Retrying\n");

      Retry = TRUE;

      MDBFree(MDBP);

      MDBP->DBType = MSched.ReqMDBType;

      MSysInitDB(MDBP);
      }
    } while (Retry == TRUE);

  return(SUCCESS);
  }  /* END MDBWriteNodesWithRetry() */


/**
 * Determines from SC whether the DB write should be retried.
 * 
 * @see MODBCError() which is currently the only routine that sets SC
 *
 * @param SC
 */

#define MOAB_MAXDBRETRIES 3

mbool_t MDBShouldRetry(

  enum MStatusCodeEnum SC)

  {
  if (MSched.Shutdown == TRUE)
    {
    /* don't reconnect if we're shutting down */

    return(FALSE);
    }

  if (MSched.DBIterationRetries >= MOAB_MAXDBRETRIES)
    {
    /* we already tried reconnecting this iteration, don't try again */

    return(FALSE);
    }

  if (SC != mscRemoteFailure)
    {
    /* currently we only retry on mscRemoteFailure */

    return(FALSE);
    }

  MSched.DBIterationRetries++;
 
  return(TRUE);
  }  /* END MDBShouldRetry() */



/**
 * Write the given node transition object to the database.
 *
 * NOTE: this routine is not threadsafe, it should only be called
 *       by the MOTransitionFromQueue() thread.
 *
 * @param   MDBInfo (I) the db info struct
 * @param   N       (I) the node transition object to write
 * @param   Msg     (O) any output error message
 * @param   SC      (O) error code (if any)
 */

int MDBWriteNodes(

  mdb_t                     *MDBInfo,
  mtransnode_t             **N,
  char                      *Msg,
  enum MStatusCodeEnum      *SC)

  {
  int nindex;

  static mstmt_t MStmt;
  int     ParamIndex = 1;

  mbool_t DeleteFirst = TRUE;

  static char  Name[MMAX_LINE];
  static char  State[MMAX_LINE];
  static char  OperatingSystem[MMAX_LINE];
  static char  Architecture[MMAX_LINE];
  static char  AvailGRes[MMAX_LINE];
  static char  ConfigGRes[MMAX_LINE];
  static char  AvailClasses[MMAX_LINE];
  static char  ConfigClasses[MMAX_LINE];
  static char  Features[MMAX_LINE];
  static char  GMetric[MMAX_LINE];
  static char  HypervisorType[MMAX_LINE];
  static char  JobList[MMAX_LINE];
  static char  OldMessages[MMAX_LINE];
  static char  NetworkAddress[MMAX_LINE];
  static char  SubState[MMAX_LINE];
  static char  Operations[MMAX_LINE];
  static char  OSList[MMAX_LINE];
  static char  Owner[MMAX_LINE];
  static char  ResOvercommitFactor[MMAX_LINE];
  static char  Partition[MMAX_LINE];
  static char  PowerPolicy[MMAX_LINE];
  static char  PowerState[MMAX_LINE];
  static char  PowerSelectState[MMAX_LINE];
  static char  PriorityFunction[MMAX_LINE];
  static char  ProvisioningData[MMAX_LINE];
  static char  ReservationList[MMAX_LINE];
  static char  ResourceManagerList[MMAX_LINE];
  static char  VMOSList[MMAX_LINE];

  static int   ConfiguredProcessors;
  static int   AvailableProcessors;
  static int   ConfiguredMemory;
  static int   AvailableMemory;
  static int   EnableProfiling;
  static int   HopCount;
  static int   IsDeleted;
  static int   IsDynamic;
  static int   LastUpdateTime;
  static int   MaxJob;
  static int   MaxJobPerUser;
  static int   MaxProc;
  static int   MaxProcPerUser;
  static int   PowerIsEnabled;
  static int   Priority;
  static int   ProcessorSpeed;
  static int   AvailableDisk;
  static int   AvailableSwap;
  static int   ConfiguredDisk;
  static int   ConfiguredSwap;
  static int   ReservationCount;
  static int   Size;
  static int   TotalNodeActiveTime;
  static int   LastModifyTime;
  static int   TotalTimeTracked;
  static int   TotalNodeUpTime;
  static int   TaskCount;

  static double ChargeRate;
  static double DynamicPriority;
  static double LoadAvg;
  static double MaxLoad;
  static double Speed;
  static double SpeedWeight;

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     writing nodes to database\n");

  if (MDBInfo->WriteNodesInitialized == FALSE)
    {
    MDB(7,fSOCK) MLog("INFO:     prepping node statement\n");
   
    if (MDBInsertNodeStmt(MDBInfo,&MStmt,NULL) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     error prepping node statement\n");
      return(FAILURE);
      }
    
    MDBBindParamText(&MStmt,ParamIndex++,Name,0);
    MDBBindParamText(&MStmt,ParamIndex++,State,0);
    MDBBindParamText(&MStmt,ParamIndex++,OperatingSystem,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&ConfiguredProcessors,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&AvailableProcessors,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&ConfiguredMemory,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&AvailableMemory,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Architecture,0);
    MDBBindParamText(&MStmt,ParamIndex++,AvailGRes,0);
    MDBBindParamText(&MStmt,ParamIndex++,ConfigGRes,0);
    MDBBindParamText(&MStmt,ParamIndex++,AvailClasses,0);
    MDBBindParamText(&MStmt,ParamIndex++,ConfigClasses,0);
    MDBBindParamDouble(&MStmt,ParamIndex++,&ChargeRate,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&DynamicPriority,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&EnableProfiling,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Features,0);
    MDBBindParamText(&MStmt,ParamIndex++,GMetric,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&HopCount,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,HypervisorType,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&IsDeleted,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&IsDynamic,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,JobList,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&LastUpdateTime,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&LoadAvg,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&MaxLoad,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&MaxJob,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&MaxJobPerUser,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&MaxProc,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&MaxProcPerUser,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,OldMessages,0);
    MDBBindParamText(&MStmt,ParamIndex++,NetworkAddress,0);
    MDBBindParamText(&MStmt,ParamIndex++,SubState,0);
    MDBBindParamText(&MStmt,ParamIndex++,Operations,0);
    MDBBindParamText(&MStmt,ParamIndex++,OSList,0);
    MDBBindParamText(&MStmt,ParamIndex++,Owner,0);
    MDBBindParamText(&MStmt,ParamIndex++,ResOvercommitFactor,0);
    MDBBindParamText(&MStmt,ParamIndex++,Partition,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&PowerIsEnabled,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,PowerPolicy,0);
    MDBBindParamText(&MStmt,ParamIndex++,PowerSelectState,0);
    MDBBindParamText(&MStmt,ParamIndex++,PowerState,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&Priority,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,PriorityFunction,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&ProcessorSpeed,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,ProvisioningData,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&AvailableDisk,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&AvailableSwap,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&ConfiguredDisk,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&ConfiguredSwap,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&ReservationCount,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,ReservationList,0);
    MDBBindParamText(&MStmt,ParamIndex++,ResourceManagerList,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,&Size,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&Speed,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&SpeedWeight,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&TotalNodeActiveTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&LastModifyTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&TotalTimeTracked,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&TotalNodeUpTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,&TaskCount,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,VMOSList,0);

    MDBInfo->WriteNodesInitialized = TRUE;
    }

  for (nindex = 0;nindex < MMAX_NODE_DB_WRITE;nindex++)
    {
    if (N[nindex] == NULL)
      break;

    if (MSched.Shutdown == TRUE)
      return(SUCCESS);

    if (DeleteFirst == TRUE)
      {
      marray_t NConstraintList;
      mnodeconstraint_t NConstraint;
   
      /* delete rsv from the database before we try to insert */
   
      MUArrayListCreate(&NConstraintList,sizeof(mnodeconstraint_t),1);
    
      NConstraint.AType = mnaNodeID;
      MUStrCpy(NConstraint.AVal[0],N[nindex]->Name,sizeof(NConstraint.AVal[0]));
      NConstraint.ACmp = mcmpEQ;
    
      MUArrayListAppend(&NConstraintList,(void *)&NConstraint);
    
      MDBDeleteNodes(MDBInfo,&NConstraintList,NULL);
    
      MUArrayListFree(&NConstraintList);
      }

    MUStrCpy(Name,(MUStrIsEmpty(N[nindex]->Name)) ? "\0" : N[nindex]->Name,sizeof(Name));
    MUStrCpy(State,MNodeState[N[nindex]->State],sizeof(State));
    MUStrCpy(OperatingSystem,(MUStrIsEmpty(N[nindex]->OperatingSystem)) ? "\0" : N[nindex]->OperatingSystem,sizeof(OperatingSystem));
    MUStrCpy(Architecture,(MUStrIsEmpty(N[nindex]->Architecture)) ? "\0" : N[nindex]->Architecture,sizeof(Architecture));
    MUStrCpy(AvailGRes,(MUStrIsEmpty(N[nindex]->AvailGRes)) ? "\0" : N[nindex]->AvailGRes,sizeof(AvailGRes));
    MUStrCpy(ConfigGRes,(MUStrIsEmpty(N[nindex]->ConfigGRes)) ? "\0" : N[nindex]->ConfigGRes,sizeof(ConfigGRes));
    MUStrCpy(AvailClasses,(MUStrIsEmpty(N[nindex]->AvailClasses)) ? "\0" : N[nindex]->AvailClasses,sizeof(AvailClasses));
    MUStrCpy(ConfigClasses,(MUStrIsEmpty(N[nindex]->ConfigClasses)) ? "\0" : N[nindex]->ConfigClasses,sizeof(ConfigClasses));
    MUStrCpy(Features,(MUStrIsEmpty(N[nindex]->Features)) ? "\0" : N[nindex]->Features,sizeof(Features));
    MUStrCpy(GMetric,(MUStrIsEmpty(N[nindex]->GMetric.c_str())) ? "\0" : N[nindex]->GMetric.c_str(),sizeof(GMetric));
    MUStrCpy(HypervisorType,MHVType[N[nindex]->HypervisorType],sizeof(HypervisorType));
    MUStrCpy(JobList,(MUStrIsEmpty(N[nindex]->JobList)) ? "\0" : N[nindex]->JobList,sizeof(JobList));
    MUStrCpy(OldMessages,(MUStrIsEmpty(N[nindex]->OldMessages)) ? "\0" : N[nindex]->OldMessages,sizeof(OldMessages));
    MUStrCpy(NetworkAddress,(MUStrIsEmpty(N[nindex]->NetworkAddress)) ? "\0" : N[nindex]->NetworkAddress,sizeof(NetworkAddress));
    MUStrCpy(SubState,(MUStrIsEmpty(N[nindex]->NodeSubState)) ? "\0" : N[nindex]->NodeSubState,sizeof(SubState));
    MUStrCpy(Operations,(MUStrIsEmpty(N[nindex]->Operations)) ? "\0" : N[nindex]->Operations,sizeof(Operations));
    MUStrCpy(OSList,(MUStrIsEmpty(N[nindex]->OSList)) ? "\0" : N[nindex]->OSList,sizeof(OSList));
    MUStrCpy(Owner,(MUStrIsEmpty(N[nindex]->Owner)) ? "\0" : N[nindex]->Owner,sizeof(Owner));
    MUStrCpy(ResOvercommitFactor,(MUStrIsEmpty(N[nindex]->ResOvercommitFactor)) ? "\0" : N[nindex]->ResOvercommitFactor,sizeof(ResOvercommitFactor));
    MUStrCpy(Partition,(MUStrIsEmpty(N[nindex]->Partition)) ? "\0" : N[nindex]->Partition,sizeof(Partition));
    MUStrCpy(PowerPolicy,MPowerPolicy[N[nindex]->PowerPolicy],sizeof(PowerPolicy));
    MUStrCpy(PowerState,MPowerState[N[nindex]->PowerState],sizeof(PowerState));
    MUStrCpy(PowerSelectState,MPowerState[N[nindex]->PowerSelectState],sizeof(PowerSelectState));
    MUStrCpy(PriorityFunction,(MUStrIsEmpty(N[nindex]->PriorityFunction)) ? "\0" : N[nindex]->PriorityFunction,sizeof(PriorityFunction));
    MUStrCpy(ProvisioningData,(MUStrIsEmpty(N[nindex]->ProvisioningData)) ? "\0" : N[nindex]->ProvisioningData,sizeof(ProvisioningData));
    MUStrCpy(ReservationList,N[nindex]->ReservationList.c_str(),sizeof(ReservationList));
    MUStrCpy(ResourceManagerList,(MUStrIsEmpty(N[nindex]->ResourceManagerList)) ? "\0" : N[nindex]->ResourceManagerList,sizeof(ResourceManagerList));
    MUStrCpy(VMOSList,(MUStrIsEmpty(N[nindex]->VMOSList)) ? "\0" : N[nindex]->VMOSList,sizeof(VMOSList));

    ConfiguredProcessors   = N[nindex]->ConfiguredProcessors;
    AvailableProcessors    = N[nindex]->AvailableProcessors;
    ConfiguredMemory       = N[nindex]->ConfiguredMemory;
    AvailableMemory        = N[nindex]->AvailableMemory;
    EnableProfiling        = N[nindex]->EnableProfiling;
    HopCount               = N[nindex]->HopCount;
    IsDeleted              = N[nindex]->IsDeleted;
    IsDynamic              = N[nindex]->IsDynamic;
    LastUpdateTime         = N[nindex]->LastUpdateTime;
    MaxJob                 = N[nindex]->MaxJob;
    MaxJobPerUser          = N[nindex]->MaxJobPerUser;
    MaxProc                = N[nindex]->MaxProc;
    MaxProcPerUser         = N[nindex]->MaxProcPerUser;
    PowerIsEnabled         = N[nindex]->PowerIsEnabled;
    Priority               = N[nindex]->Priority;
    ProcessorSpeed         = N[nindex]->ProcessorSpeed;
    AvailableDisk          = N[nindex]->AvailableDisk;
    AvailableSwap          = N[nindex]->AvailableSwap;
    ConfiguredDisk         = N[nindex]->ConfiguredDisk;
    ConfiguredSwap         = N[nindex]->ConfiguredSwap;
    ReservationCount       = N[nindex]->ReservationCount;
    Size                   = N[nindex]->Size;
    TotalNodeActiveTime    = N[nindex]->TotalNodeActiveTime;
    LastModifyTime         = N[nindex]->LastModifyTime;
    TotalTimeTracked       = N[nindex]->TotalTimeTracked;
    TotalNodeUpTime        = N[nindex]->TotalNodeUpTime;
    TaskCount              = N[nindex]->TaskCount;

    ChargeRate             = N[nindex]->ChargeRate;
    DynamicPriority        = N[nindex]->DynamicPriority;
    LoadAvg                = N[nindex]->LoadAvg;
    MaxLoad                = N[nindex]->MaxLoad;
    Speed                  = N[nindex]->Speed;
    SpeedWeight            = N[nindex]->SpeedWeight;

    MDB(7,fTRANS) MLog("INFO:     writing node '%s' to database\n",
      Name);

    if (MDBExecStmt(&MStmt,SC) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     failure executing node statement\n");
      return(FAILURE);
      }
    }  /* END for (nindex) */

  return(SUCCESS);
  }  /* END MDBWriteNode() */

/**
 * Put a WHERE clause in the delete statement to apply the requested constraint
 *
 * @param   JConstraintList (I) constraint list for the job delete statement
 */

int MDBDeleteJobsStmtAddWhereClause(

  marray_t *JConstraintList)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBDeleteJobsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(JConstraintList)\n",
    FName);

  Clause[0] = '\0';

  if (JConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the delete statement which is where we will add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBDeleteJobsStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < JConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mjobconstraint_t *JConstraint;

    JConstraint = (mjobconstraint_t *)MUArrayListGet(JConstraintList,index);

    switch (JConstraint->AType)
      {
      case mjaJobID:
        {
        char *JobID = &JConstraint->AVal[0][0];

        if (JConstraint->ACmp != mcmpEQ)
          return(FAILURE);

        sprintf(tmpStr," %s ID = '%s'",
          (Clause[0] != '\0') ? "AND " : "",
          JobID);

        MUStrCat(Clause,tmpStr,sizeof(Clause));
        }

        break;

      case mjaSRMJID:
        {
        char *SRMJobID = &JConstraint->AVal[0][0];

        if (JConstraint->ACmp != mcmpEQ)
          return(FAILURE);

        sprintf(tmpStr," %s SourceRMJobID = '%s'",
          (Clause[0] != '\0') ? "AND " : "",
          SRMJobID);

        MUStrCat(Clause,tmpStr,sizeof(Clause));
        }

        break;

      default:

        return(FAILURE);

        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {
    sprintf(WherePtr," WHERE %s;",
      Clause);
    }

  return(SUCCESS);
  }  /* END MDBDeleteJobsStmtAddWhereClause() */
/**
 * Put a WHERE clause in the select statement to apply the requested constraint.
 *
 * @param   ConstraintList (I) constraint struct for the VC
 *                         select statement
 */

int MDBSelectVCsStmtAddWhereClause(

  marray_t *ConstraintList) /* I */

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBSelectVCsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(Constraint)\n",
    FName);

  Clause[0] = '\0';

  if (ConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the select statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBSelectVCsStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < ConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mvcconstraint_t *Constraint;

    Constraint = (mvcconstraint_t *)MUArrayListGet(ConstraintList,index);

    switch (Constraint->AType)
      {
      case mvcaName:
        {
         if(Clause[0] != '\0')
          MUStrCat(tmpStr," OR ",sizeof(tmpStr));

        sprintf(tmpStr,
          " (TrigID %s '%s')",
          MComp[Constraint->ACmp],
          Constraint->AVal[0]);

        MUStrCat(Clause,tmpStr,sizeof(Clause));
        }
        break;

      default:

        return(FAILURE);

        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {
    sprintf(WherePtr," WHERE %s;",
      Clause);
    }

  return(SUCCESS);
  }  /* END MDBSelectVCsStmtAddWhereClause() */






/**
 * Put a WHERE clause in the delete statement to apply the requested constraint
 *
 * @param   RConstraintList (I) constraint list for the 
 *                          reservation delete statement
 */

int MDBDeleteRsvsStmtAddWhereClause(

  marray_t *RConstraintList)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBDeleteRsvsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(RConstraintList)\n",
    FName);

  Clause[0] = '\0';

  if (RConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the delete statement which is where we will add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBDeleteRsvsStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < RConstraintList->NumItems; index++)
    {
    mrsvconstraint_t *RConstraint;
    char *RsvID = NULL;
    char tmpStr[MMAX_LINE];

    RConstraint = (mrsvconstraint_t *)MUArrayListGet(RConstraintList,index);

    RsvID = &RConstraint->AVal[0][0];

    if (RConstraint->ACmp != mcmpEQ)
      return(FAILURE);

    sprintf(tmpStr," %s Name = '%s'",
      (Clause[0] != '\0') ? "AND " : "",
      RsvID);
                   
    MUStrCat(Clause,tmpStr,sizeof(Clause));          
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBDeleteRsvsStmtAddWhereClause() */


/**
 * Put a WHERE clause in the delete statement to apply the requested constraint
 *
 * @param   TConstraintList (I) constraint list for the trigger 
 *                          delete statement (only supports mtaTrigID) 
 */

int MDBDeleteTriggersStmtAddWhereClause(

  marray_t *TConstraintList)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBDeleteTriggersStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(TConstraintList)\n",
    FName);

  Clause[0] = '\0';

  if (TConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the delete statement which is where we will add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBDeleteTriggersStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < TConstraintList->NumItems; index++)
    {
    mtrigconstraint_t *TConstraint;
    char tmpStr[MMAX_LINE];
    char *TrigID = NULL;

    TConstraint = (mtrigconstraint_t *)MUArrayListGet(TConstraintList,index);
    TrigID = &TConstraint->AVal[0][0];

    if (TConstraint->ACmp != mcmpEQ)
      return(FAILURE);

    sprintf(tmpStr," %s TrigID = '%s'",
      (Clause[0] != '\0') ? "AND " : "",
      TrigID);
                     
    MUStrCat(Clause,tmpStr,sizeof(Clause));          
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBDeleteTriggersStmtAddWhereClause() */


/**
 * Put a WHERE clause in the delete statement to apply the requested constraint
 *
 * @param   VCConstraintList (I) constraint list for the VC
 *                          delete statement
 */

int MDBDeleteVCsStmtAddWhereClause(

  marray_t *VCConstraintList)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBDeleteVCsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(VCConstraintList)\n",
    FName);

  Clause[0] = '\0';

  if (VCConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the delete statement which is where we will add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBDeleteVCsStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < VCConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    char *VCID;
    mvcconstraint_t *VCConstraint;

    VCConstraint = (mvcconstraint_t *)MUArrayListGet(VCConstraintList,index);

    VCID = &VCConstraint->AVal[0][0];

    if (VCConstraint->ACmp != mcmpEQ)
      return(FAILURE);

    sprintf(tmpStr," %s Name = '%s'",
      (Clause[0] != '\0') ? "AND " : "",
      VCID);
                     
    MUStrCat(Clause,tmpStr,sizeof(Clause));          
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBDeleteVCsStmtAddWhereClause() */





/**
 * Put a WHERE clause in the delete statement to apply the node constraint
 *
 * @param   NodeConstraintList (I) constraint list for the node delete statement
 */

int MDBDeleteNodesStmtAddWhereClause(

  marray_t *NodeConstraintList)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBDeleteNodesStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(NodeConstraintList)\n",
    FName);

  Clause[0] = '\0';

  if (NodeConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the delete statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBDeleteNodesStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < NodeConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mnodeconstraint_t *NodeConstraint;

    NodeConstraint = (mnodeconstraint_t *)MUArrayListGet(NodeConstraintList,index);

    switch (NodeConstraint->AType)
      {
      case mnaNodeID:
        {
        char *NodeName = &NodeConstraint->AVal[0][0];

        if (NodeConstraint->ACmp != mcmpEQ)
          return(FAILURE);

        /* note that we are mapping the SRMJID to JobID in the requests table */

        sprintf(tmpStr," %s ID = '%s'",
          (Clause[0] != '\0') ? "AND " : "",
          NodeName);

        MUStrCat(Clause,tmpStr,sizeof(Clause));          
        }

        break;

      default:
  
        return(FAILURE);
  
        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBDeleteNodesStmtAddWhereClause() */







/**
 * Put a WHERE clause in the delete statement to apply the requested constraint
 *
 * @param   ReqConstraintList (I) constraint list for the request delete statement
 */

int MDBDeleteRequestsStmtAddWhereClause(

  marray_t *ReqConstraintList)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBDeleteRequestsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(ReqConstraintList)\n",
    FName);

  Clause[0] = '\0';

  if (ReqConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the delete statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBDeleteRequestsStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < ReqConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mjobconstraint_t *ReqConstraint;

    ReqConstraint = (mjobconstraint_t *)MUArrayListGet(ReqConstraintList,index);

    switch (ReqConstraint->AType)
      {
      case mjaSRMJID:
        {
        char *SRMJobID = &ReqConstraint->AVal[0][0];

        if (ReqConstraint->ACmp != mcmpEQ)
          return(FAILURE);

        /* note that we are mapping the SRMJID to JobID in the requests table */

        sprintf(tmpStr," %s JobID = '%s'",
          (Clause[0] != '\0') ? "AND " : "",
          SRMJobID);

        MUStrCat(Clause,tmpStr,sizeof(Clause));          
        }

        break;

      default:
  
        return(FAILURE);
  
        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBDeleteRequestsStmtAddWhereClause() */

/**
 * Write the job transition object and all of its request transition objects
 * to the db
 *
 * See MDBWriteRequest (child)
 *
 * @param   MDBInfo (I) the db info struct
 * @param   J       (I) the job transition object
 * @param   Msg     (O) any output error message
 * @param   SC      (O) error code (if any)
 */

int MDBWriteJobs(

  mdb_t                     *MDBInfo,
  mtransjob_t              **J,
  char                      *Msg,
  enum MStatusCodeEnum      *SC)

  {
  mbool_t DeleteFirst = TRUE;

  static mstmt_t MStmt;

  static char Name[MMAX_LINE];
  static char SourceRMJobID[MMAX_LINE];
  static char DestinationRMJobID[MMAX_LINE];
  static char GridJobID[MMAX_LINE];
  static char AName[MMAX_LINE];
  static char JobGroup[MMAX_LINE];
  static char User[MMAX_LINE];
  static char Group[MMAX_LINE];
  static char Account[MMAX_LINE];
  static char Class[MMAX_LINE];
  static char QOS[MMAX_LINE];
  static char State[MMAX_LINE];
  static char ExpectedState[MMAX_LINE];
  static char SubState[MMAX_LINE];
  static char DestinationRM[MMAX_LINE];
  static char SourceRM[MMAX_LINE];
  static char Dependencies[MMAX_BUFFER];
  static char RequestedHostList[MMAX_LINE];
  static char ExcludedHostList[MMAX_LINE];
  static char MasterHost[MMAX_LINE];

  static char ActivePartition[MMAX_LINE];
  static char SpecPAL[MMAX_LINE];

  static char RequiredReservation[MMAX_LINE];
  static char ReservationAccess[MMAX_LINE];
  static char GenericAttributes[MMAX_LINE];
  static char Flags[MMAX_LINE];
  static char Holds[MMAX_LINE];
  static char StdErr[MMAX_LINE];
  static char StdOut[MMAX_LINE];
  static char StdIn[MMAX_LINE];
  static char RMOutput[MMAX_LINE];
  static char RMError[MMAX_LINE];
  static char CommandFile[MMAX_LINE];
  static char Arguments[MMAX_LINE];
  static char InitialWorkingDirectory[MMAX_LINE];
  static char PerPartitionPriority[MMAX_LINE];
  static char Messages[MMAX_LINE];
  static char NotificationAddress[MMAX_LINE];
  static char RMSubmitLanguage[MMAX_LINE];
  static char BlockReason[MMAX_LINE];
  static char BlockMsg[MMAX_LINE];

  /* larger to potentially hold variables */

  static char Description[MMAX_LINE << 2];

  static long SubmitTime;
  static long StartTime;
  static long CompletionTime;
  static long QueueTime;
  static long SuspendTime;
  static long RequestedMinWalltime;
  static long RequestedMaxWalltime;
  static long UsedWalltime;
  static long MinPreemptTime;
  static long HoldTime;
  static long CPULimit;
  static long UMask;
  static long UserPriority;
  static long SystemPriority;
  static long Priority;
  static long RunPriority;
  static long RsvStartTime;

  static int CompletionCode;
  static int StartCount;
  static int BypassCount;
  static int MaxProcessorCount;
  static int RequestedNodes;

  static double PSDedicated;
  static double PSUtilized;
  static double Cost;

  int jindex;

  int ParamIndex = 1;

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     writing jobs to database\n");

  if (MDBInfo->WriteJobsInitialized == FALSE)
    {
    MDB(7,fSOCK) MLog("INFO:     prepping job statement\n");

    if (MDBInsertJobStmt(MDBInfo,&MStmt,NULL) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     error prepping job statement\n");
      return(FAILURE);
      }
 
    MDBBindParamText(&MStmt,ParamIndex++,Name,0);
    MDBBindParamText(&MStmt,ParamIndex++,SourceRMJobID,0);
    MDBBindParamText(&MStmt,ParamIndex++,DestinationRMJobID,0);
    MDBBindParamText(&MStmt,ParamIndex++,GridJobID,0);
    MDBBindParamText(&MStmt,ParamIndex++,AName,0);
    MDBBindParamText(&MStmt,ParamIndex++,User,0);
    MDBBindParamText(&MStmt,ParamIndex++,Account,0);
    MDBBindParamText(&MStmt,ParamIndex++,Class,0);
    MDBBindParamText(&MStmt,ParamIndex++,QOS,0);
    MDBBindParamText(&MStmt,ParamIndex++,Group,0);
    MDBBindParamText(&MStmt,ParamIndex++,JobGroup,0);
    MDBBindParamText(&MStmt,ParamIndex++,State,0);
    MDBBindParamText(&MStmt,ParamIndex++,ExpectedState,0);
    MDBBindParamText(&MStmt,ParamIndex++,SubState,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&UserPriority,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&SystemPriority,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&Priority,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&RunPriority,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,PerPartitionPriority,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&SubmitTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&QueueTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&StartTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&CompletionTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&CompletionCode,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&UsedWalltime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&RequestedMinWalltime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&RequestedMaxWalltime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&CPULimit,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&SuspendTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&HoldTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&MaxProcessorCount,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&RequestedNodes,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,ActivePartition,0);
    MDBBindParamText(&MStmt,ParamIndex++,SpecPAL,0);
    MDBBindParamText(&MStmt,ParamIndex++,DestinationRM,0);
    MDBBindParamText(&MStmt,ParamIndex++,SourceRM,0);
    MDBBindParamText(&MStmt,ParamIndex++,Flags,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&MinPreemptTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Dependencies,0);
    MDBBindParamText(&MStmt,ParamIndex++,RequestedHostList,0);
    MDBBindParamText(&MStmt,ParamIndex++,ExcludedHostList,0);
    MDBBindParamText(&MStmt,ParamIndex++,MasterHost,0);
    MDBBindParamText(&MStmt,ParamIndex++,GenericAttributes,0);
    MDBBindParamText(&MStmt,ParamIndex++,Holds,0);
    MDBBindParamDouble(&MStmt,ParamIndex++,&Cost,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Description,0);
    MDBBindParamText(&MStmt,ParamIndex++,Messages,0);
    MDBBindParamText(&MStmt,ParamIndex++,NotificationAddress,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&StartCount,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&BypassCount,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,CommandFile,0);
    MDBBindParamText(&MStmt,ParamIndex++,Arguments,0);
    MDBBindParamText(&MStmt,ParamIndex++,RMSubmitLanguage,0);
    MDBBindParamText(&MStmt,ParamIndex++,StdIn,0);
    MDBBindParamText(&MStmt,ParamIndex++,StdOut,0);
    MDBBindParamText(&MStmt,ParamIndex++,StdErr,0);
    MDBBindParamText(&MStmt,ParamIndex++,RMOutput,0);
    MDBBindParamText(&MStmt,ParamIndex++,RMError,0);
    MDBBindParamText(&MStmt,ParamIndex++,InitialWorkingDirectory,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&UMask,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&RsvStartTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,BlockReason,0);
    MDBBindParamText(&MStmt,ParamIndex++,BlockMsg,0);
    MDBBindParamDouble(&MStmt,ParamIndex++,&PSDedicated,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&PSUtilized,FALSE);

    MDBInfo->WriteJobsInitialized = TRUE;
    } /* END if (MDBInfo->WriteJobsInitialized == FALSE) */

  for (jindex = 0;jindex < MMAX_JOB_DB_WRITE;jindex++)
    {
    if (J[jindex] == NULL)
      break;

    if (MSched.Shutdown == TRUE)
      return(SUCCESS);

    /* if the delete existing job flag is set in the transition object (e.g. MJobRename()) 
       then we must delete the current job entry in the database (the one with the same
       SRMJID) and it's associated entries in the requests table before writing the new 
       copy of the job to the database */
   
    if ((bmisset(&J[jindex]->TFlags,mtransfDeleteExisting)) && (!MUStrIsEmpty(J[jindex]->SourceRMJobID)))
      {
      marray_t JConstraintList;
      mjobconstraint_t JConstraint;
   
      /* delete job and reqs from db based on SRMJID */
   
      MUArrayListCreate(&JConstraintList,sizeof(mjobconstraint_t),1);
    
      JConstraint.AType = mjaSRMJID;
      MUStrCpy(JConstraint.AVal[0],J[jindex]->SourceRMJobID,sizeof(JConstraint.AVal[0]));
      JConstraint.ACmp = mcmpEQ;
    
      MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
    
      MDBDeleteJobs(MDBInfo,&JConstraintList,NULL);
      MDBDeleteRequests(MDBInfo,&JConstraintList,NULL);
    
      MUArrayListFree(&JConstraintList);
      }

    if (DeleteFirst == TRUE)
      {
      marray_t JConstraintList;
      mjobconstraint_t JConstraint;
   
      MUArrayListCreate(&JConstraintList,sizeof(mjobconstraint_t),1);
    
      JConstraint.AType = mjaJobID;
      MUStrCpy(JConstraint.AVal[0],J[jindex]->Name,sizeof(JConstraint.AVal[0]));
      JConstraint.ACmp = mcmpEQ;
    
      MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
    
      MDBDeleteJobs(MDBInfo,&JConstraintList,NULL);
      MDBDeleteRequests(MDBInfo,&JConstraintList,NULL);
    
      MUArrayListFree(&JConstraintList);
      }

    MUStrCpy(Name,J[jindex]->Name,MMAX_LINE);
    MUStrCpy(SourceRMJobID,(!MUStrIsEmpty(J[jindex]->SourceRMJobID)) ? J[jindex]->SourceRMJobID : "\0",MMAX_LINE);
    MUStrCpy(DestinationRMJobID,(!MUStrIsEmpty(J[jindex]->DestinationRMJobID)) ? J[jindex]->DestinationRMJobID : "\0",MMAX_LINE);
    MUStrCpy(GridJobID,(!MUStrIsEmpty(J[jindex]->GridJobID)) ? J[jindex]->GridJobID : "\0",MMAX_LINE);
    MUStrCpy(AName,(!MUStrIsEmpty(J[jindex]->AName)) ? J[jindex]->AName : "\0",MMAX_LINE);
    MUStrCpy(JobGroup,(!MUStrIsEmpty(J[jindex]->JobGroup)) ? J[jindex]->JobGroup : "\0",MMAX_LINE);
    MUStrCpy(User,(!MUStrIsEmpty(J[jindex]->User)) ? J[jindex]->User : "\0",MMAX_LINE);
    MUStrCpy(Group,(!MUStrIsEmpty(J[jindex]->Group)) ? J[jindex]->Group : "\0",MMAX_LINE);
    MUStrCpy(Account,(!MUStrIsEmpty(J[jindex]->Account)) ? J[jindex]->Account : "\0",MMAX_LINE);
    MUStrCpy(Class,(!MUStrIsEmpty(J[jindex]->Class)) ? J[jindex]->Class : "\0",MMAX_LINE);
    MUStrCpy(QOS,(!MUStrIsEmpty(J[jindex]->QOS)) ? J[jindex]->QOS : "\0",MMAX_LINE);
    MUStrCpy(State,MJobState[J[jindex]->State],MMAX_LINE);
    MUStrCpy(ExpectedState,MJobState[J[jindex]->ExpectedState],MMAX_LINE);
    MUStrCpy(SubState,MJobSubState[J[jindex]->SubState],MMAX_LINE);
    MUStrCpy(DestinationRM,(!MUStrIsEmpty(J[jindex]->DestinationRM)) ? J[jindex]->DestinationRM : "\0",MMAX_LINE);
    MUStrCpy(SourceRM,(!MUStrIsEmpty(J[jindex]->SourceRM)) ? J[jindex]->SourceRM : "\0",MMAX_LINE);
    MUStrCpy(Dependencies,(!MUStrIsEmpty(J[jindex]->Dependencies)) ? J[jindex]->Dependencies : "\0",MMAX_BUFFER);
    MUStrCpy(RequestedHostList,(!MUStrIsEmpty(J[jindex]->RequestedHostList)) ? J[jindex]->RequestedHostList : "\0",MMAX_LINE);
    MUStrCpy(ExcludedHostList,(!MUStrIsEmpty(J[jindex]->ExcludedHostList)) ? J[jindex]->ExcludedHostList : "\0",MMAX_LINE);
    MUStrCpy(MasterHost,(!MUStrIsEmpty(J[jindex]->MasterHost)) ? J[jindex]->MasterHost : "\0",MMAX_LINE);
    MUStrCpy(ActivePartition,(J[jindex]->ActivePartition > 0) ? MPar[J[jindex]->ActivePartition].Name : "\0",MMAX_LINE);
    MUStrCpy(RequiredReservation,(!MUStrIsEmpty(J[jindex]->RequiredReservation)) ? J[jindex]->RequiredReservation : "\0",MMAX_LINE);
    MUStrCpy(ReservationAccess,(!MUStrIsEmpty(J[jindex]->ReservationAccess)) ? J[jindex]->ReservationAccess : "\0",MMAX_LINE);
    MUStrCpy(Flags,(!MUStrIsEmpty(J[jindex]->Flags)) ? J[jindex]->Flags : "\0",MMAX_LINE);
    MUStrCpy(StdErr,(!MUStrIsEmpty(J[jindex]->StdErr)) ? J[jindex]->StdErr : "\0",MMAX_LINE);
    MUStrCpy(StdOut,(!MUStrIsEmpty(J[jindex]->StdOut)) ? J[jindex]->StdOut : "\0",MMAX_LINE);
    MUStrCpy(StdIn,(!MUStrIsEmpty(J[jindex]->StdIn)) ? J[jindex]->StdIn : "\0",MMAX_LINE);
    MUStrCpy(RMOutput,(!MUStrIsEmpty(J[jindex]->RMOutput)) ? J[jindex]->RMOutput : "\0",MMAX_LINE);
    MUStrCpy(RMError,(!MUStrIsEmpty(J[jindex]->RMError)) ? J[jindex]->RMError : "\0",MMAX_LINE);
    MUStrCpy(CommandFile,(!MUStrIsEmpty(J[jindex]->CommandFile)) ? J[jindex]->CommandFile : "\0",MMAX_LINE);
    MUStrCpy(Arguments,(!MUStrIsEmpty(J[jindex]->Arguments)) ? J[jindex]->Arguments : "\0",MMAX_LINE);
    MUStrCpy(InitialWorkingDirectory,(!MUStrIsEmpty(J[jindex]->InitialWorkingDirectory)) ? J[jindex]->InitialWorkingDirectory : "\0",MMAX_LINE);
    MUStrCpy(PerPartitionPriority,(!MUStrIsEmpty(J[jindex]->PerPartitionPriority)) ? J[jindex]->PerPartitionPriority : "\0",MMAX_LINE);
    MUStrCpy(Description,(!MUStrIsEmpty(J[jindex]->Description)) ? J[jindex]->Description : "\0",sizeof(Description));
    MUStrCpy(Messages,(!MUStrIsEmpty(J[jindex]->Messages)) ? J[jindex]->Messages : "\0",MMAX_LINE);
    MUStrCpy(NotificationAddress,(!MUStrIsEmpty(J[jindex]->NotificationAddress)) ? J[jindex]->NotificationAddress : "\0",MMAX_LINE);
    MUStrCpy(RMSubmitLanguage,(J[jindex]->RMSubmitType != mrmtNONE) ? MRMType[J[jindex]->RMSubmitType] : "\0",MMAX_LINE);
    MUStrCpy(BlockReason,(!MUStrIsEmpty(J[jindex]->BlockReason)) ? J[jindex]->BlockReason : "\0",MMAX_LINE);
    MUStrCpy(BlockMsg,(!MUStrIsEmpty(J[jindex]->BlockMsg)) ? J[jindex]->BlockMsg : "\0",MMAX_LINE);

/*
    MUJobFeaturesToString
    bmtostring(&J[jindex]->GenericAttributes,(const char **)MAList[meJFeature],GenericAttributes);
*/
    bmtostring(&J[jindex]->Hold,MHoldType,Holds);

    SubmitTime                = J[jindex]->SubmitTime;
    StartTime                 = J[jindex]->StartTime;
    CompletionTime            = J[jindex]->CompletionTime;
    QueueTime                 = J[jindex]->QueueTime;
    SuspendTime               = J[jindex]->SuspendTime;
    RequestedMinWalltime      = J[jindex]->RequestedMinWalltime;
    RequestedMaxWalltime      = J[jindex]->RequestedMaxWalltime;
    UsedWalltime              = J[jindex]->UsedWalltime;
    MinPreemptTime            = J[jindex]->MinPreemptTime;
    HoldTime                  = J[jindex]->HoldTime;
    CPULimit                  = J[jindex]->CPULimit;
    UMask                     = J[jindex]->UMask;
    UserPriority              = J[jindex]->UserPriority;
    SystemPriority            = J[jindex]->SystemPriority;
    Priority                  = J[jindex]->Priority;                 
    RunPriority               = J[jindex]->RunPriority; 
    RsvStartTime              = J[jindex]->RsvStartTime;

    CompletionCode    = J[jindex]->CompletionCode;
    StartCount        = J[jindex]->StartCount;
    BypassCount       = J[jindex]->BypassCount;
    MaxProcessorCount = J[jindex]->MaxProcessorCount;
    RequestedNodes    = J[jindex]->RequestedNodes;

    PSDedicated = J[jindex]->PSDedicated;
    PSUtilized  = J[jindex]->PSUtilized;
    Cost        = J[jindex]->Cost;

    MDB(7,fTRANS) MLog("INFO:     writing job '%s' to database\n",
      Name);

    if (MDBExecStmt(&MStmt,SC) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     failure executing job statement\n");
      return(FAILURE);
      }

    MDBWriteRequests(MDBInfo,J[jindex]->Requirements,Msg,SC);
    }  /* END for (jindex) */  /* write all the reqs via MDBWriteRequest */

  return(SUCCESS);
  }  /* END MDBWriteJob() */




/**
 * Write the reservation transition object and all of its 
 * request transition objects to the db 
 *
 * See MDBWriteRequest (child)
 *
 * @param   MDBInfo (I) the db info struct
 * @param   R       (I) the reservation transition object
 * @param   Msg     (O) any output error message
 * @param   SC      (O) error code (if any)
 */

int MDBWriteRsv(

  mdb_t                     *MDBInfo,
  mtransrsv_t              **R,
  char                      *Msg,
  enum MStatusCodeEnum      *SC)

  {
  mbool_t DeleteFirst = TRUE;

  static mstmt_t MStmt;

  static char Name[MMAX_LINE];
  static char ACL[MMAX_LINE];
  static char AAccount[MMAX_LINE];
  static char AGroup[MMAX_LINE];
  static char AUser[MMAX_LINE];
  static char AQOS[MMAX_LINE];
  static int  AllocNodeCount;
  static char AllocNodeList[MMAX_LINE];
  static int  AllocProcCount;
  static int  AllocTaskCount;
  static char CL[MMAX_LINE];
  static char Comment[MMAX_LINE];
  static double Cost;
  static long CTime;
  static long Duration;
  static long EndTime;
  static char ExcludeRsv[MMAX_LINE];
  static long ExpireTime;
  static char Flags[MMAX_LINE];
  static char GlobalID[MMAX_LINE];
  static char HostExp[MMAX_LINE];
  static char History[MMAX_LINE];
  static char Label[MMAX_LINE];
  static long LastChargeTime;
  static char LogLevel[MMAX_LINE];
  static char Messages[MMAX_LINE];
  static char Owner[MMAX_LINE];
  static char Partition[MMAX_LINE];
  static char Profile[MMAX_LINE];
  static char ReqArch[MMAX_LINE];
  static char ReqFeatureList[MMAX_LINE];
  static char ReqMemory[MMAX_LINE];
  static char ReqNodeCount[MMAX_LINE];
  static char ReqNodeList[MMAX_LINE];
  static char ReqOS[MMAX_LINE];
  static char ReqTaskCount[MMAX_LINE];
  static char ReqTPN[MMAX_LINE];
  static char Resources[MMAX_LINE];
  static char RsvAccessList[MMAX_LINE];
  static char RsvGroup[MMAX_LINE];
  static char RsvParent[MMAX_LINE];
  static char SID[MMAX_LINE];
  static long StartTime;
  static double StatCAPS;
  static double StatCIPS;
  static double StatTAPS;
  static double StatTIPS;
  static char SubType[MMAX_LINE];
  static char Trigger[MMAX_LINE];
  static char Type[MMAX_LINE];
  static char Variables[MMAX_LINE];
  static char VMList[MMAX_LINE];

  int rindex;

  int ParamIndex = 1;

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     writing reservations to database\n");

  if (MDBInfo->WriteRsvInitialized == FALSE)
    {
    MDB(7,fSOCK) MLog("INFO:     prepping reservation statement\n");

    if (MDBInsertRsvStmt(MDBInfo,&MStmt,NULL) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     error prepping reservation statement\n");
      return(FAILURE);
      }
 
    MDBBindParamText(&MStmt,ParamIndex++,Name,0);
    MDBBindParamText(&MStmt,ParamIndex++,ACL,0);
    MDBBindParamText(&MStmt,ParamIndex++,AAccount,0);
    MDBBindParamText(&MStmt,ParamIndex++,AGroup,0);
    MDBBindParamText(&MStmt,ParamIndex++,AUser,0);
    MDBBindParamText(&MStmt,ParamIndex++,AQOS,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&AllocNodeCount,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,AllocNodeList,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&AllocProcCount,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&AllocTaskCount,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,CL,0);
    MDBBindParamText(&MStmt,ParamIndex++,Comment,0);
    MDBBindParamDouble(&MStmt,ParamIndex++,&Cost,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&CTime,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&Duration,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&EndTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,ExcludeRsv,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&ExpireTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Flags,0);
    MDBBindParamText(&MStmt,ParamIndex++,GlobalID,0);
    MDBBindParamText(&MStmt,ParamIndex++,HostExp,0);
    MDBBindParamText(&MStmt,ParamIndex++,History,0);
    MDBBindParamText(&MStmt,ParamIndex++,Label,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&LastChargeTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,LogLevel,0);
    MDBBindParamText(&MStmt,ParamIndex++,Messages,0);
    MDBBindParamText(&MStmt,ParamIndex++,Owner,0);
    MDBBindParamText(&MStmt,ParamIndex++,Partition,0);
    MDBBindParamText(&MStmt,ParamIndex++,Profile,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqArch,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqFeatureList,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqMemory,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqNodeCount,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqNodeList,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqOS,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqTaskCount,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqTPN,0);
    MDBBindParamText(&MStmt,ParamIndex++,Resources,0);
    MDBBindParamText(&MStmt,ParamIndex++,RsvAccessList,0);
    MDBBindParamText(&MStmt,ParamIndex++,RsvGroup,0);
    MDBBindParamText(&MStmt,ParamIndex++,RsvParent,0);
    MDBBindParamText(&MStmt,ParamIndex++,SID,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&StartTime,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&StatCAPS,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&StatCIPS,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&StatTAPS,FALSE);
    MDBBindParamDouble(&MStmt,ParamIndex++,&StatTIPS,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,SubType,0);
    MDBBindParamText(&MStmt,ParamIndex++,Trigger,0);
    MDBBindParamText(&MStmt,ParamIndex++,Type,0);
    MDBBindParamText(&MStmt,ParamIndex++,Variables,0);
    MDBBindParamText(&MStmt,ParamIndex++,VMList,0);

    MDBInfo->WriteRsvInitialized = TRUE;
    } /* END if (MDBInfo->WriteRsvInitialized == FALSE) */

  for (rindex = 0;rindex < MMAX_RSV_DB_WRITE;rindex++)
    {
    if (R[rindex] == NULL)
      break;

    if (MSched.Shutdown == TRUE)
      return(SUCCESS);

    if (DeleteFirst == TRUE)
      {
      marray_t RConstraintList;
      mrsvconstraint_t RConstraint;
   
      MUArrayListCreate(&RConstraintList,sizeof(mrsvconstraint_t),1);
    
      RConstraint.AType = mraName;
      MUStrCpy(RConstraint.AVal[0],R[rindex]->Name,sizeof(RConstraint.AVal[0]));
      RConstraint.ACmp = mcmpEQ;
    
      MUArrayListAppend(&RConstraintList,(void *)&RConstraint);
    
      MDBDeleteRsvs(MDBInfo,&RConstraintList,NULL);
    
      MUArrayListFree(&RConstraintList);
      }

    MUStrCpy(Name,R[rindex]->Name,sizeof(Name));
    MUStrCpy(ACL,R[rindex]->ACL,sizeof(ACL));
    MUStrCpy(AAccount,R[rindex]->AAccount,sizeof(AAccount));
    MUStrCpy(AGroup,R[rindex]->AGroup,sizeof(AGroup));
    MUStrCpy(AUser,R[rindex]->AUser,sizeof(AUser));
    MUStrCpy(AQOS,R[rindex]->AQOS,sizeof(AQOS));
    MUStrCpy(LogLevel,R[rindex]->LogLevel,sizeof(LogLevel));
    MUStrCpy(Messages,R[rindex]->Messages,sizeof(Messages));
    MUStrCpy(Owner,R[rindex]->Owner,sizeof(Owner));
    MUStrCpy(Partition,R[rindex]->Partition,sizeof(Partition));
    MUStrCpy(Profile,R[rindex]->Profile,sizeof(Profile));
    MUStrCpy(ReqArch,R[rindex]->ReqArch,sizeof(ReqArch));
    MUStrCpy(ReqFeatureList,R[rindex]->ReqFeatureList,sizeof(ReqFeatureList));
    MUStrCpy(ReqMemory,R[rindex]->ReqMemory,sizeof(ReqMemory));
    MUStrCpy(ReqNodeCount,R[rindex]->ReqNodeCount,sizeof(ReqNodeCount));
    MUStrCpy(ReqNodeList,R[rindex]->ReqNodeList,sizeof(ReqNodeList));
    MUStrCpy(ReqOS,R[rindex]->ReqOS,sizeof(ReqOS));
    MUStrCpy(ReqTaskCount,R[rindex]->ReqTaskCount,sizeof(ReqTaskCount));
    MUStrCpy(ReqTPN,R[rindex]->ReqTPN,sizeof(ReqTPN));
    MUStrCpy(Resources,R[rindex]->Resources,sizeof(Resources));
    MUStrCpy(RsvAccessList,R[rindex]->RsvAccessList,sizeof(RsvAccessList));
    MUStrCpy(RsvGroup,R[rindex]->RsvGroup,sizeof(RsvGroup));
    MUStrCpy(RsvParent,R[rindex]->RsvParent,sizeof(RsvParent));
    MUStrCpy(SID,R[rindex]->SID,sizeof(SID));
    MUStrCpy(CL,R[rindex]->CL,sizeof(CL));
    MUStrCpy(Comment,R[rindex]->Comment,sizeof(Comment));
    MUStrCpy(ExcludeRsv,R[rindex]->ExcludeRsv,sizeof(ExcludeRsv));
    MUStrCpy(Flags,R[rindex]->Flags,sizeof(Flags));
    MUStrCpy(GlobalID,R[rindex]->GlobalID,sizeof(GlobalID));
    MUStrCpy(HostExp,R[rindex]->HostExp,sizeof(HostExp));
    MUStrCpy(History,R[rindex]->History,sizeof(History));
    MUStrCpy(Label,R[rindex]->Label,sizeof(Label));
    MUStrCpy(SubType,R[rindex]->SubType,sizeof(SubType));
    MUStrCpy(Trigger,R[rindex]->Trigger,sizeof(Trigger));
    MUStrCpy(Type,R[rindex]->Type,sizeof(Type));
    MUStrCpy(VMList,R[rindex]->VMList,sizeof(VMList));

    MUStrCpy(AllocNodeList,R[rindex]->AllocNodeList->c_str(),sizeof(AllocNodeList));
    MUStrCpy(Variables,R[rindex]->Variables->c_str(),sizeof(Variables));

    AllocNodeCount        = R[rindex]->AllocNodeCount;
    AllocProcCount        = R[rindex]->AllocProcCount;
    AllocTaskCount        = R[rindex]->AllocTaskCount;
    Cost                  = R[rindex]->Cost;
    CTime                 = R[rindex]->CTime;
    Duration              = R[rindex]->Duration;
    EndTime               = R[rindex]->EndTime;
    ExpireTime            = R[rindex]->ExpireTime;
    LastChargeTime        = R[rindex]->LastChargeTime;
    StartTime             = R[rindex]->StartTime;
    StatCAPS              = R[rindex]->StatCAPS;
    StatCIPS              = R[rindex]->StatCIPS;
    StatTAPS              = R[rindex]->StatTAPS;
    StatTIPS              = R[rindex]->StatTIPS;

    MDB(7,fTRANS) MLog("INFO:     writing reservation '%s' to database\n",
      Name);

    if (MDBExecStmt(&MStmt,SC) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     failure executing reservation statement\n");
      return(FAILURE);
      }
    }  /* END for (rindex) */  /* write all the reqs via MDBWriteRequest */

  return(SUCCESS);
  }  /* END MDBWriteRsv() */




/**
 * Write the trigger transition object and all of its 
 * request transition objects to the db.
 *
 * See MDBWriteRequest (child)
 *
 * @param   MDBInfo (I) the db info struct
 * @param   T       (I) the trigger transition object
 * @param   Msg     (O) any output error message
 * @param   SC      (O) error code (if any)
 */

int MDBWriteTrigger(

  mdb_t                     *MDBInfo,
  mtranstrig_t              **T,
  char                      *Msg,
  enum MStatusCodeEnum      *SC)

  {
  mbool_t DeleteFirst = TRUE;

  static mstmt_t MStmt;

  static char TrigID[MMAX_LINE];
  static char Name[MMAX_LINE];
  static char ActionData[MMAX_LINE];
  static char ActionType[MMAX_LINE];
  static int  BlockTime;
  static char Description[MMAX_LINE];
  static int  Disabled;
  static char EBuf[MMAX_LINE];
  static int  EventTime;
  static char EventType[MMAX_LINE];
  static char FailOffset[MMAX_LINE];
  static int  FailureDetected;
  static char Flags[MMAX_LINE];
  static int  IsComplete;
  static int  IsInterval;
  static int  LaunchTime;
  static char Message[MMAX_LINE];
  static char MultiFire[MMAX_LINE];
  static char ObjectID[MMAX_LINE];
  static char ObjectType[MMAX_LINE];
  static char OBuf[MMAX_LINE];
  static char Offset[MMAX_LINE];
  static char Period[MMAX_LINE];
  static int  PID;
  static int  RearmTime;
  static char Requires[MMAX_LINE];
  static char Sets[MMAX_LINE];
  static char State[MMAX_LINE];
  static char Threshold[MMAX_LINE];
  static int  Timeout;
  static char Unsets[MMAX_LINE];

  int tindex;

  int ParamIndex = 1;

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     writing triggers to database\n");

  if (MDBInfo->WriteTriggerInitialized == FALSE)
    {
    MDB(7,fSOCK) MLog("INFO:     prepping trigger statement\n");

    if (MDBInsertTriggerStmt(MDBInfo,&MStmt,NULL) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     error prepping trigger statement\n");
      return(FAILURE);
      }

    MDBBindParamText(&MStmt,ParamIndex++,TrigID,0);
    MDBBindParamText(&MStmt,ParamIndex++,Name,0);
    MDBBindParamText(&MStmt,ParamIndex++,ActionData,0);
    MDBBindParamText(&MStmt,ParamIndex++,ActionType,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&BlockTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Description,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&Disabled,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,EBuf,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&EventTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,EventType,0);
    MDBBindParamText(&MStmt,ParamIndex++,FailOffset,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&FailureDetected,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Flags,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&IsComplete,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&IsInterval,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&LaunchTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Message,0);
    MDBBindParamText(&MStmt,ParamIndex++,MultiFire,0);
    MDBBindParamText(&MStmt,ParamIndex++,ObjectID,0);
    MDBBindParamText(&MStmt,ParamIndex++,ObjectType,0);
    MDBBindParamText(&MStmt,ParamIndex++,OBuf,0);
    MDBBindParamText(&MStmt,ParamIndex++,Offset,0);
    MDBBindParamText(&MStmt,ParamIndex++,Period,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&PID,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&RearmTime,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Requires,0);
    MDBBindParamText(&MStmt,ParamIndex++,Sets,0);
    MDBBindParamText(&MStmt,ParamIndex++,State,0);
    MDBBindParamText(&MStmt,ParamIndex++,Threshold,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&Timeout,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,Unsets,0);

    MDBInfo->WriteTriggerInitialized = TRUE;
    } /* END if (MDBInfo->WriteTriggerInitialized == FALSE) */

  for (tindex = 0;tindex < MMAX_TRIG_DB_WRITE;tindex++)
    {
    if (T[tindex] == NULL)
      break;

    if (MSched.Shutdown == TRUE)
      return(SUCCESS);

    if (DeleteFirst == TRUE)
      {
      marray_t TConstraintList;
      mtrigconstraint_t TConstraint;
   
      MUArrayListCreate(&TConstraintList,sizeof(mtrigconstraint_t),1);
    
      TConstraint.AType = mtaTrigID;
      MUStrCpy(TConstraint.AVal[0],T[tindex]->Name,sizeof(TConstraint.AVal[0]));
      TConstraint.ACmp = mcmpEQ;
    
      MUArrayListAppend(&TConstraintList,(void *)&TConstraint);
    
      MDBDeleteTriggers(MDBInfo,&TConstraintList,NULL);
    
      MUArrayListFree(&TConstraintList);
      }

    MUStrCpy(TrigID,T[tindex]->TrigID,sizeof(TrigID));
    MUStrCpy(Name,T[tindex]->Name,sizeof(Name));
    MUStrCpy(ActionData,T[tindex]->ActionData,sizeof(ActionData));
    MUStrCpy(ActionType,T[tindex]->ActionType,sizeof(ActionType));
    MUStrCpy(Description,T[tindex]->Description,sizeof(Description));
    MUStrCpy(EBuf,T[tindex]->EBuf,sizeof(EBuf));
    MUStrCpy(EventType,T[tindex]->EventType,sizeof(EventType));
    MUStrCpy(FailOffset,T[tindex]->FailOffset,sizeof(FailOffset));
    MUStrCpy(Flags,T[tindex]->Flags,sizeof(Flags));
    MUStrCpy(Message,T[tindex]->Message,sizeof(Message));
    MUStrCpy(MultiFire,T[tindex]->MultiFire,sizeof(MultiFire));
    MUStrCpy(ObjectID,T[tindex]->ObjectID,sizeof(ObjectID));
    MUStrCpy(ObjectType,T[tindex]->ObjectType,sizeof(ObjectType));
    MUStrCpy(OBuf,T[tindex]->OBuf,sizeof(OBuf));
    MUStrCpy(Offset,T[tindex]->Offset,sizeof(Offset));
    MUStrCpy(Period,T[tindex]->Period,sizeof(Period));
    MUStrCpy(Requires,T[tindex]->Requires,sizeof(Requires));
    MUStrCpy(Sets,T[tindex]->Sets,sizeof(Sets));
    MUStrCpy(State,T[tindex]->State,sizeof(State));
    MUStrCpy(Threshold,T[tindex]->Threshold,sizeof(Threshold));
    MUStrCpy(Unsets,T[tindex]->Unsets,sizeof(Unsets));

    BlockTime               = T[tindex]->BlockTime;
    Disabled                = T[tindex]->Disabled;
    EventTime               = T[tindex]->EventTime;
    FailureDetected         = T[tindex]->FailureDetected;
    IsComplete              = T[tindex]->IsComplete;
    IsInterval              = T[tindex]->IsInterval;
    LaunchTime              = T[tindex]->LaunchTime;
    PID                     = T[tindex]->PID;
    RearmTime               = T[tindex]->RearmTime;
    Timeout                 = T[tindex]->Timeout;

    MDB(7,fTRANS) MLog("INFO:     writing trigger '%s' to database\n",
      Name);

    if (MDBExecStmt(&MStmt,SC) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     failure executing trigger statement\n");
      return(FAILURE);
      }
    }  /* END for (tindex) */  /* write all the reqs via MDBWriteRequest */

  return(SUCCESS);
  }  /* END MDBWriteTrigger() */




/**
 * Write the VC transition object and all of its 
 * request transition objects to the db.
 *
 * See MDBWriteRequest (child)
 *
 * @param   MDBInfo (I) the db info struct
 * @param   VC      (I) the VC transition object
 * @param   Msg     (O) any output error message
 * @param   SC      (O) error code (if any)
 */

int MDBWriteVC(

  mdb_t                     *MDBInfo,
  mtransvc_t               **VC,
  char                      *Msg,
  enum MStatusCodeEnum      *SC)

  {
  mbool_t DeleteFirst = TRUE;

  static mstmt_t MStmt;

  static char Name[MMAX_LINE];
  static char Description[MMAX_LINE];
  static char Jobs[MMAX_LINE];
  static char Nodes[MMAX_LINE];
  static char VMs[MMAX_LINE];
  static char Rsvs[MMAX_LINE];
  static char VCs[MMAX_LINE];
  static char Variables[MMAX_LINE];
  static char Flags[MMAX_LINE];

  int VCIndex;
  int ParamIndex = 1;

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     writing VC's to database\n");

  if (MDBInfo->WriteVCInitialized == FALSE)
    {
    MDB(7,fSOCK) MLog("INFO:     prepping VC statement\n");

    if (MDBInsertVCStmt(MDBInfo,&MStmt,NULL) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     error prepping VC statement\n");
      return(FAILURE);
      }

    MDBBindParamText(&MStmt,ParamIndex++,Name,0);
    MDBBindParamText(&MStmt,ParamIndex++,Description,0);
    MDBBindParamText(&MStmt,ParamIndex++,Jobs,0);
    MDBBindParamText(&MStmt,ParamIndex++,Nodes,0);
    MDBBindParamText(&MStmt,ParamIndex++,VMs,0);
    MDBBindParamText(&MStmt,ParamIndex++,Rsvs,0);
    MDBBindParamText(&MStmt,ParamIndex++,VCs,0);
    MDBBindParamText(&MStmt,ParamIndex++,Variables,0);
    MDBBindParamText(&MStmt,ParamIndex++,Flags,0);

    MDBInfo->WriteVCInitialized = TRUE;
    }  /* END if (MDBInfo->WriteVCInitialized == FALSE) */

  for (VCIndex = 0;VCIndex < MMAX_VC_DB_WRITE;VCIndex++)
    {
    if (VC[VCIndex] == NULL)
      break;

    if (MSched.Shutdown == TRUE)
      return(SUCCESS);

    if (DeleteFirst == TRUE)
      {
      marray_t VConstraintList;
      mvcconstraint_t VConstraint;
   
      MUArrayListCreate(&VConstraintList,sizeof(mvcconstraint_t),1);
    
      VConstraint.AType = mvcaName;
      MUStrCpy(VConstraint.AVal[0],VC[VCIndex]->Name,sizeof(VConstraint.AVal[0]));
      VConstraint.ACmp = mcmpEQ;
    
      MUArrayListAppend(&VConstraintList,(void *)&VConstraint);
    
      MDBDeleteVCs(MDBInfo,&VConstraintList,NULL);
    
      MUArrayListFree(&VConstraintList);
      }

    MUStrCpy(Name,VC[VCIndex]->Name,sizeof(Name));
    MUStrCpy(Description,VC[VCIndex]->Description,sizeof(Description));
    MUStrCpy(Jobs,VC[VCIndex]->Jobs,sizeof(Jobs));
    MUStrCpy(Nodes,VC[VCIndex]->Nodes,sizeof(Nodes));
    MUStrCpy(VMs,VC[VCIndex]->VMs,sizeof(VMs));
    MUStrCpy(Rsvs,VC[VCIndex]->Rsvs,sizeof(Rsvs));

    if (MXMLToString(VC[VCIndex]->Variables,Variables,sizeof(Variables),NULL,TRUE) == FAILURE)
      {
      Variables[0] = '\0';
      }

    MUStrCpy(Flags,VC[VCIndex]->Flags,MMAX_LINE);

    MDB(7,fTRANS) MLog("INFO:     writing VC '%s' to database\n",
      Name);

    if (MDBExecStmt(&MStmt,SC) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     failure executing VC statement\n");
      return(FAILURE);
      }
    }  /* END for (VCIndex = 0;VCIndex < MMAX_VC_DB_WRITE;VCIndex++) */

  return(SUCCESS);
  }  /* END int MDBWriteVC() */

/**
 * Write a Request object to the database
 *
 * @see MDBWriteJob (parent)
 *
 * @param MDBInfo (I)
 * @param R       (I)
 * @param Msg     (O) [optional]
 * @param SC      (O) error code (if any)
 */

int MDBWriteRequests(

  mdb_t                     *MDBInfo,
  mtransreq_t              **R,
  char                      *Msg,
  enum MStatusCodeEnum      *SC)

  {
  static char JobID[MMAX_LINE];
  static int  Index;

  static char AllocNodeList[MMAX_LINE];
  static char AllocPartition[MMAX_LINE];

  static char NodeAccessPolicy[MMAX_LINE];
  static char PreferredFeatures[MMAX_LINE];
  static char RequestedApp[MMAX_LINE];
  static char RequestedArch[MMAX_LINE];
  static char ReqOS[MMAX_LINE];
  static char ReqNodeSet[MMAX_LINE];
  static char ReqPartition[MMAX_LINE];
  static char NodeDisk[MMAX_LINE];
  static char NodeFeatures[MMAX_LINE];
  static char NodeMemory[MMAX_LINE];
  static char NodeProcs[MMAX_LINE];
  static char NodeSwap[MMAX_LINE];
  static char GenericResources[MMAX_LINE];
  static char ConfiguredGenericResources[MMAX_LINE];

  static int   MinNodeCount;                        
  static int   MinTaskCount;                        
  static int   TaskCount;                           
  static int   TaskCountPerNode;                    
  static int   DiskPerTask;                         
  static int   MemPerTask;                          
  static int   ProcsPerTask;                        
  static int   SwapPerTask;                         
  static int   PartitionIndex;                      
  
  static mstmt_t MStmt;

  int rindex;

  int ParamIndex = 1;

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     writing requests to database\n");

  if (MDBInfo->WriteRequestsInitialized == FALSE)
    {
    MDB(7,fSOCK) MLog("INFO:     prepping request statement\n");

    if (MDBInsertRequestStmt(MDBInfo,&MStmt,NULL) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     error prepping request statement\n");
      return(FAILURE);
      }
   
    MDBBindParamText(&MStmt,ParamIndex++,JobID,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&Index,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,AllocNodeList,0);
    MDBBindParamText(&MStmt,ParamIndex++,AllocPartition,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&PartitionIndex,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,NodeAccessPolicy,0);
    MDBBindParamText(&MStmt,ParamIndex++,PreferredFeatures,0);
    MDBBindParamText(&MStmt,ParamIndex++,RequestedApp,0);
    MDBBindParamText(&MStmt,ParamIndex++,RequestedArch,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqOS,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqNodeSet,0);
    MDBBindParamText(&MStmt,ParamIndex++,ReqPartition,0);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&MinNodeCount,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&MinTaskCount,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&TaskCount,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&TaskCountPerNode,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&DiskPerTask,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&MemPerTask,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&ProcsPerTask,FALSE);
    MDBBindParamInteger(&MStmt,ParamIndex++,(int *)&SwapPerTask,FALSE);
    MDBBindParamText(&MStmt,ParamIndex++,NodeDisk,0);
    MDBBindParamText(&MStmt,ParamIndex++,NodeFeatures,0);
    MDBBindParamText(&MStmt,ParamIndex++,NodeMemory,0);
    MDBBindParamText(&MStmt,ParamIndex++,NodeSwap,0);
    MDBBindParamText(&MStmt,ParamIndex++,NodeProcs,0);
    MDBBindParamText(&MStmt,ParamIndex++,GenericResources,0);
    MDBBindParamText(&MStmt,ParamIndex++,ConfiguredGenericResources,0);

    MDBInfo->WriteRequestsInitialized = TRUE;
    }  /* END if (MDBInfo->WriteRequestsInitialized == FALSE) */

  for (rindex = 0;rindex < MMAX_REQ_PER_JOB;rindex++)
    {
    if (R[rindex] == NULL)
      break;

    MUStrCpy(JobID,(R[rindex]->JobID != NULL) ? R[rindex]->JobID : "\0",sizeof(JobID));

    /* IF there is a mstring_t present for the AllocNodeList, then use its string contents, otherwise, null string the copy */
    if (R[rindex]->AllocNodeList != NULL) 
      {
      MUStrCpy(AllocNodeList,R[rindex]->AllocNodeList->c_str(),sizeof(AllocNodeList));
      }
    else
      {
      MUStrCpy(AllocNodeList,"\0",sizeof(AllocNodeList));
      }

    MUStrCpy(AllocPartition,(!MUStrIsEmpty(R[rindex]->Partition)) ? R[rindex]->Partition : "\0",sizeof(AllocPartition));
    MUStrCpy(NodeAccessPolicy,(R[rindex]->NodeAccessPolicy != mnacNONE) ? MNAccessPolicy[R[rindex]->NodeAccessPolicy] : "\0",sizeof(NodeAccessPolicy));
    MUStrCpy(PreferredFeatures,(!MUStrIsEmpty(R[rindex]->PreferredFeatures)) ? R[rindex]->PreferredFeatures : "\0",sizeof(PreferredFeatures));
    MUStrCpy(RequestedApp,(!MUStrIsEmpty(R[rindex]->RequestedApp)) ? R[rindex]->RequestedApp : "\0",sizeof(RequestedApp));
    MUStrCpy(RequestedArch,(!MUStrIsEmpty(R[rindex]->RequestedArch)) ? R[rindex]->RequestedArch : "\0",sizeof(ReqOS));
    MUStrCpy(ReqOS,(!MUStrIsEmpty(R[rindex]->ReqOS)) ? R[rindex]->ReqOS : "\0",sizeof(ReqOS));
    MUStrCpy(ReqNodeSet,(!MUStrIsEmpty(R[rindex]->ReqNodeSet)) ? R[rindex]->ReqNodeSet : "\0",sizeof(ReqNodeSet));
    MUStrCpy(ReqPartition,(!MUStrIsEmpty(R[rindex]->Partition)) ? R[rindex]->Partition : "\0",sizeof(ReqPartition));
    MUStrCpy(NodeFeatures,(!MUStrIsEmpty(R[rindex]->NodeFeatures)) ? R[rindex]->NodeFeatures : "\0",sizeof(NodeFeatures));
    MUStrCpy(GenericResources,(!MUStrIsEmpty(R[rindex]->GenericResources)) ? R[rindex]->GenericResources : "\0",sizeof(GenericResources));
    MUStrCpy(ConfiguredGenericResources,(!MUStrIsEmpty(R[rindex]->ConfiguredGenericResources)) ? R[rindex]->ConfiguredGenericResources : "\0",sizeof(ConfiguredGenericResources));
    MUStrCpy(NodeDisk,(!MUStrIsEmpty(R[rindex]->NodeDisk)) ? R[rindex]->NodeDisk : "\0",sizeof(NodeDisk));
    MUStrCpy(NodeMemory,(!MUStrIsEmpty(R[rindex]->NodeMemory)) ? R[rindex]->NodeMemory : "\0",sizeof(NodeMemory));
    MUStrCpy(NodeProcs,(!MUStrIsEmpty(R[rindex]->NodeProcs)) ? R[rindex]->NodeProcs : "\0",sizeof(NodeProcs));
    MUStrCpy(NodeSwap,(!MUStrIsEmpty(R[rindex]->NodeSwap)) ? R[rindex]->NodeSwap : "\0",sizeof(NodeSwap));

    Index            = R[rindex]->Index;
    MinNodeCount     = R[rindex]->MinNodeCount;
    MinTaskCount     = R[rindex]->MinTaskCount;
    TaskCount        = R[rindex]->TaskCount;
    TaskCountPerNode = R[rindex]->TaskCountPerNode;
    DiskPerTask      = R[rindex]->DiskPerTask;
    MemPerTask       = R[rindex]->MemPerTask;
    ProcsPerTask     = R[rindex]->ProcsPerTask;
    SwapPerTask      = R[rindex]->SwapPerTask;
 
    MDB(7,fTRANS) MLog("INFO:     writing request '%s' to database\n",
      JobID);

    if (MDBExecStmt(&MStmt,SC) == FAILURE)
      {
      MDB(7,fSOCK) MLog("INFO:     failure executing request statement\n");
      return(FAILURE);
      }
    }  /* END for (rindex) */

  return(SUCCESS);
  } /* END MDBWriteRequest() */




/**
 * Write event to database
 *
 * @param MDBInfo  (I)
 * @param OType    (I)
 * @param OID      (I)
 * @param EType    (I)
 * @param EID      (I)
 * @param Name     (I) [optional]
 * @param Msg      (O) [optional]
 * @param SC       (O) error code (if any)
 */

int MDBWriteEvent(

  mdb_t                     *MDBInfo,
  enum MXMLOTypeEnum         OType,
  char                      *OID,
  enum MRecordEventTypeEnum  EType,
  int                        EID,
  char                      *Name,
  const char                *Msg,
  enum MStatusCodeEnum      *SC)

  {
  mstmt_t MStmt;
  int  unsigned Time = MSched.Time;
  char const * RealMsg = (Msg == NULL) ? "" : Msg;

  const char *FName = "MDBWriteEvent";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%d,OType,EType,%u,OID,Msg)\n",
    FName,
    EID,
    Time);

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     prepping event statement\n");

  if (MDBInsertEventStmt(MDBInfo,&MStmt,NULL) == FAILURE)
    {
    MDB(7,fSOCK) MLog("INFO:     error prepping event statement\n");
    return(FAILURE);
    }

  MDBBindParamInteger(&MStmt,1,(int *)&EID,FALSE);
  MDBBindParamInteger(&MStmt,2,(int *)&OType,FALSE);
  MDBBindParamInteger(&MStmt,3,(int *)&EType,FALSE);
  MDBBindParamInteger(&MStmt,4,(int *)&Time,FALSE);
  MDBBindParamText(&MStmt,5,OID,0);
  MDBBindParamText(&MStmt,6,(Name == NULL) ? "NONE" : Name,0);
  MDBBindParamText(&MStmt,7,RealMsg,0);

  MDB(7,fTRANS) MLog("INFO:     writing event '%s' to database\n",
      Name);

  if (MDBExecStmt(&MStmt,SC) == FAILURE)
    {
    MDB(7,fSOCK) MLog("INFO:     failure executing event statement\n");
    return(FAILURE);
    }

  MDBResetStmt(&MStmt);

  return(SUCCESS);
  }  /* END MDBWriteEvent() */




/**
 * Write record for table NodeStats
 * @param MDBInfo (I)
 * @param IStat   (I)
 * @param Name    (I)
 * @param EMsg    (O)
 * @param SC      (O) error code (if any)
 */

int MDBWriteNodeStats(

  mdb_t                *MDBInfo,
  mnust_t              *IStat,
  char const           *Name,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  int unsigned index;

  mstmt_t MStmt;

  int IntIndeces[] = MDB_NODE_STATS_INT_INDECES;
  int DoubleIndeces[] = MDB_NODE_STATS_DOUBLE_INDECES;
  int *IntValues[] = MDB_NODE_STATS_INT_VALUES(*IStat);
  double *DoubleValues[] = MDB_NODE_STATS_DOUBLE_VALUES(*IStat);

  const char *FName = "MDBWriteNodeStats";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,IStat,Name,EMsg)\n",
    FName);

  if (NULL == SC)
    {
    return(FAILURE);
    }

  MDB(7,fSOCK) MLog("INFO:     prepping node stats statement\n");

  if (MDBInsertNodeStatsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    MDB(7,fSOCK) MLog("INFO:     error prepping node stats statement\n");
    return(FAILURE);
    }

  for (index = 0; index < sizeof(IntIndeces) / sizeof(IntIndeces[0]); index++) 
    {
    MDBBindParamInteger(&MStmt,IntIndeces[index],IntValues[index],FALSE);
    }

  for (index = 0; index < sizeof(DoubleIndeces) / sizeof(DoubleIndeces[0]); index++) 
    {
    MDBBindParamDouble(&MStmt,DoubleIndeces[index],DoubleValues[index],FALSE);
    }

  MDBBindParamText(&MStmt,21,Name,0);

  MDB(7,fTRANS) MLog("INFO:     writing node stats for node '%s' to database\n",
      Name);

  if (MDBExecStmt(&MStmt,SC) == FAILURE)
    {
    MDB(7,fSOCK) MLog("INFO:     failure executing node stats statement\n");
    return(FAILURE);
    }

  MDBResetStmt(&MStmt);

  if (MDBWriteGenericMetricsArray(MDBInfo,IStat->XLoad,IStat->OType,Name,IStat->StartTime,EMsg,SC) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBWriteNodeStatsGenericResourcesArray(MDBInfo,Name,MGRes.Name[index],IStat->StartTime,&IStat->AGRes,&IStat->CGRes,&IStat->DGRes,IStat->IterationCount,EMsg,SC) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MDBWriteNodeStats() */ 




/**
 * bind record from table NodeStatsGenericResources for fetching
 * 
 * @param MStmt        (I)
 * @param CGResVal     (I)
 * @param DGResVal     (I)
 * @param AGResVal     (I)
 * @param GResName     (I)
 * @param GResNameSize (I)
 * @param Time         (I)
 * @param StmtText     (I)
 * @param EMsg         (O)
 */

int MDBBindNodeStatGenericResource(

  mstmt_t           *MStmt,        /* I */
  int               *CGResVal,     /* I */
  int               *DGResVal,     /* I */
  int               *AGResVal,     /* I */
  char              *GResName,     /* I */
  int                GResNameSize, /* I */
  long unsigned int *Time,         /* I */
  char              *StmtText,     /* I */
  char              *EMsg)         /* O */

  {
  const char *FName = "MDBBindNodeStatGenericResource";

  MDB(5,fSTRUCT) MLog("%s(MStmt,CGResVal,DGResVal,AGResVal,%s,%d,Time,%s,EMsg)\n",
    FName,
    GResName,
    GResNameSize,
    StmtText);

  if (CGResVal != NULL)
    MDBBindColInteger(MStmt,1,CGResVal);

  if (DGResVal != NULL)
    MDBBindColInteger(MStmt,2,DGResVal);

  if (DGResVal != NULL)
    MDBBindColInteger(MStmt,3,AGResVal);

  if (GResName != NULL)
    MDBBindColText(MStmt,4,GResName,GResNameSize);

  if (Time != NULL)
    MDBBindColInteger(MStmt,5,(int *)Time);

  return(SUCCESS);
  } /* END MDBBindNodeStatGenericResource */ 




/**
* Read record from table NodeStatsGenericResources
 * @param MDBInfo (I)
 * @param NodeID  (I)
 * @param Time    (I)
 * @param CGRes   (I)
 * @param DGRes   (I)
 * @param AGRes   (I)
 * @param EMsg    (O)
 */

int MDBReadNodeStatsGenericResources(

  mdb_t             *MDBInfo, /* I */
  char              *NodeID,  /* I */
  long unsigned int  Time,    /* I */
  msnl_t            *CGRes,   /* I */
  msnl_t            *DGRes,   /* I */
  msnl_t            *AGRes,   /* I */
  char              *EMsg)    /* O */

  {
  enum MStatusCodeEnum SC;

  int tmpCGRes = 0;
  int tmpDGRes = 0;
  int tmpAGRes = 0;
  int rc = SUCCESS;
  char tmpGResName[MMAX_NAME];

  mstmt_t MStmt;

  const char *FName = "MDBReadNodeStatsGenericResources";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,%s,%lu,CGRes,DGRes,AGRes,EMsg)\n",
    FName,
    NodeID,
    Time);

  if (MDBSelectNodeStatsGenericResourcesStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamText(&MStmt,1,NodeID,0);
  MDBBindParamInteger(&MStmt,2,(int *)&Time,FALSE);
  MDBBindNodeStatGenericResource(&MStmt,&tmpCGRes,&tmpDGRes,&tmpAGRes,tmpGResName,sizeof(tmpGResName),NULL,NULL,EMsg);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  MSNLClear(CGRes);
  MSNLClear(DGRes);
  MSNLClear(AGRes);

  rc = MDBFetch(&MStmt);

  while ((rc != FAILURE) && (rc != MDB_FETCH_NO_DATA))
    {
    MUInsertGenericResource(tmpGResName,CGRes,DGRes,AGRes,tmpCGRes,tmpDGRes,tmpAGRes);
    rc = MDBFetch(&MStmt);
    }

  MDBResetStmt(&MStmt);

  return((rc == FAILURE) ? FAILURE : SUCCESS);
  } /* END MDBReadNodeStatsGenericResources */ 




/**
 * Read in a range of generic metrics.
 * @param MDBInfo   (I)
 * @param IStat     (O)
 * @param NodeID    (I)
 * @param Duration  (I)
 * @param StartTime (I)
 * @param EndTime   (I)
 * @param EMsg      (O)
 */

int MDBReadNodeStatsGenericResourcesRange(

  mdb_t             *MDBInfo,   /* I */
  mnust_t *         *IStat,     /* O */
  char              *NodeID,    /* I */
  long unsigned int  Duration,  /* I */
  long unsigned int  StartTime, /* I */
  long unsigned int  EndTime,   /* I */
  char              *EMsg)      /* O */

  {
  enum MStatusCodeEnum SC;

  int tmpCGRes = 0;
  int tmpDGRes = 0;
  int tmpAGRes = 0;
  char tmpGResName[MMAX_NAME];
  mulong tmpTime = 0;
  int rc = SUCCESS;
  ldiv_t StatIndex;
  mstmt_t MStmt;

  const char *FName = "MDBReadNodeStatsGenericResourcesRange";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,IStat,%s,%lu,%lu,%lu,EMsg)\n",
    FName,
    NodeID,
    Duration,
    StartTime,
    EndTime);

  if (MDBSelectNodeStatsGenericResourcesRangeStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamText(&MStmt,1,NodeID,0);
  MDBBindParamInteger(&MStmt,2,(int *)&StartTime,FALSE);
  MDBBindParamInteger(&MStmt,3,(int *)&EndTime,FALSE);
  MDBBindNodeStatGenericResource(&MStmt,&tmpCGRes,&tmpDGRes,&tmpAGRes,tmpGResName,sizeof(tmpGResName),&tmpTime,NULL,EMsg);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  rc = MDBFetch(&MStmt);

  while (rc != FAILURE && rc != MDB_FETCH_NO_DATA)
    {
    StatIndex = ldiv((long)((long)tmpTime - (long)StartTime),(long)Duration);

    if (StatIndex.rem == 0)
      {
      msnl_t *CGRes = &IStat[StatIndex.quot]->CGRes;
      msnl_t *DGRes = &IStat[StatIndex.quot]->DGRes;
      msnl_t *AGRes = &IStat[StatIndex.quot]->AGRes;
      /* Only accept stats that are exactly on the profile interval */
       MUInsertGenericResource(tmpGResName,CGRes,DGRes,AGRes,tmpCGRes,tmpDGRes,tmpAGRes);
      /* MUInsertGenericMetric(MUExtractXLoad(IStat[StatIndex.quot],OType),GMetricName,GMetricValue); */
      }
    rc = MDBFetch(&MStmt);
    }

  if (rc != MDB_FETCH_NO_DATA)
    {
    MDBStmtError(&MStmt,NULL,EMsg);

    rc = FAILURE;
    }

  MDBResetStmt(&MStmt);

  return(rc);
  } /* END MDBReadNodeStatsGenericResourcesRange */ 




/**
 * Bind record from table NodeStats to an mnust_t object for fetching
 *
 * @param MStmt    (I)
 * @param IStat    (I)
 * @param StmtText (I)
 * @param EMsg     (O)
 */

int MDBBindNodeStat(

  mstmt_t *MStmt,    /* I */
  mnust_t *IStat,    /* I */
  char    *StmtText, /* I */
  char    *EMsg)     /* O */

  {
  int unsigned index;
  int IntIndeces[] = MDB_NODE_STATS_INT_INDECES;
  int DoubleIndeces[] = MDB_NODE_STATS_DOUBLE_INDECES;
  int *IntValues[] = MDB_NODE_STATS_INT_VALUES(*IStat);
  double *DoubleValues[] = MDB_NODE_STATS_DOUBLE_VALUES(*IStat);

  const char *FName = "MDBBindNodeStat";

  MDB(5,fSTRUCT) MLog("%s(MStmt,IStat,%s,EMsg)\n",
    FName,
    StmtText);

  for (index = 0; index < sizeof(IntIndeces) / sizeof(IntIndeces[0]); index++) 
    {
    MDBBindColInteger(MStmt,IntIndeces[index],IntValues[index]);
    }

  for (index = 0; index < sizeof(DoubleIndeces) / sizeof(DoubleIndeces[0]); index++) 
    {
    MDBBindColDouble(MStmt,DoubleIndeces[index],DoubleValues[index]);
    }

  return(SUCCESS);
  } /* END MDBBindNodeStat() */ 




/**
 * Read record from table NodeStats
 *
 * @param MDBInfo  (I)
 * @param IStat    (I)
 * @param NodeID   (I)
 * @param StatTime (I)
 * @param EMsg     (O)
 */

int MDBReadNodeStats(

  mdb_t             *MDBInfo,  /* I */
  mnust_t           *IStat,    /* I */
  char              *NodeID,   /* I */
  long unsigned int  StatTime, /* I */
  char              *EMsg)     /* O */

  {
  enum MStatusCodeEnum SC;

  mstmt_t MStmt;
  int rc;

  const char *FName = "MDBReadNodeStats";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,IStat,%s,%lu,EMsg)\n",
    FName,
    NodeID,
    StatTime);

  if (MDBSelectNodeStatsStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamText(&MStmt,1,NodeID,0);
  MDBBindParamInteger(&MStmt,2,(int *)&StatTime,FALSE);
  MDBBindNodeStat(&MStmt,IStat,NULL,EMsg);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  rc = MDBFetch(&MStmt);

  if ((rc == FAILURE) || (rc == MDB_FETCH_NO_DATA))
    return(FAILURE);

  if (MDBReadNodeStatsGenericResources(MDBInfo,NodeID,StatTime,&IStat->CGRes,&IStat->DGRes,&IStat->AGRes,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBReadGenericMetrics(MDBInfo,IStat->XLoad,mxoNode,NodeID,StatTime,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MDBReadNodeStats() */ 




/**
 * Read in node stats range from DB.
 *
 * @param MDBInfo   (I)
 * @param Stat      (I)
 * @param NodeID    (I)
 * @param Duration  (I)
 * @param StartTime (I)
 * @param EndTime   (I)
 * @param EMsg      (O)
 */

int MDBReadNodeStatsRange(

  mdb_t             *MDBInfo,   /* I */
  mnust_t           *Stat,      /* I */
  char              *NodeID,    /* I */
  long unsigned int  Duration,  /* I */
  long unsigned int  StartTime, /* I */
  long unsigned int  EndTime,   /* I */
  char              *EMsg)      /* O */

  {
  enum MStatusCodeEnum SC;

  mnust_t *IStat = NULL;
  int index;
  int ArrSize;
  int rc = SUCCESS;

  ldiv_t Index;

  mstmt_t MStmt;
  const char *FName = "MDBReadNodeStatsRange";

  MDB(6,fSTRUCT) MLog("%s(MDBInfo,Stat,%s,%lu,%lu,%lu,EMsg)\n",
    FName,
    NodeID,
    Duration,
    StartTime,
    EndTime);

  if (MDBSelectNodeStatsRangeStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamText(&MStmt,1,NodeID,0);
  MDBBindParamInteger(&MStmt,2,(int *)&StartTime,FALSE);
  MDBBindParamInteger(&MStmt,3,(int *)&EndTime,FALSE);

  if (MDBExecStmt(&MStmt,&SC) == FAILURE)
    {
    return(FAILURE);
    }

  ArrSize = (EndTime - StartTime) / Duration + 1; /* Need to add 1 because of inclusiveness of EndTime */

  MUFree((char **)&Stat->IStat);
  Stat->IStat =  (mnust_t **)MUCalloc(ArrSize,sizeof(must_t *));

  Stat->IStatCount = ArrSize;

  MStatCreate((void **)&IStat,msoNode);

  while (TRUE) 
    {
    if (MDBBindNodeStat(
          &MStmt,
          IStat,
          NULL,
          EMsg) == FAILURE)
      {
      rc = FAILURE;

      break;
      }

    if (MDBFetch(&MStmt) == MDB_FETCH_NO_DATA)
      break;

    Index = ldiv((long)(IStat->StartTime - (long)StartTime),(long)Duration);

    if (Index.rem == 0)
      {
      /* Only accept stats that are exactly on the profile interval */

      IStat->Index = Index.quot;

      Stat->IStat[Index.quot] = IStat;

      MStatCreate((void **)&IStat,msoNode);
      }
    }   /* END while (TRUE) */

  MStatFree((void **)&IStat,msoNode);

  MDBResetStmt(&MStmt);

  for (index = 0; index < ArrSize; index++)
    {
    if (Stat->IStat[index] == NULL)
      {
      MStatCreate((void **)&Stat->IStat[index],msoNode);
      MStatPInit(Stat->IStat[index],msoNode,index,StartTime + (Duration * index));
      Stat->IStat[index]->OType = mxoNode;
      }
    }
  
  if (MDBReadGenericMetricsRange(MDBInfo,(void **)Stat->IStat,mxoNode,NodeID,Duration,StartTime,EndTime,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MDBReadNodeStatsGenericResourcesRange(MDBInfo,Stat->IStat,NodeID,Duration,StartTime,EndTime,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MStatAddLastFromMemory(mxoNode,NodeID,Stat,EndTime);

  return(rc);
  } /* END MDBReadNodeStatsRange() */ 




/**
 * put MDBInfo into a manual-commit transacation state. If MDBInfo is already in
 * such a state, then the result is undefined.
 * @param MDBInfo (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 *
 * @return SUCCESS if MDBInfo was not in a manual-commit transaction state and
 * is now in such a state. FAILURE if the state transition failed. Undefined if
 * MDBInfo is already in a manual-commit transaction state.
 */

int MDBBeginTransaction(

  mdb_t *MDBInfo,
  char *EMsg)

  {
  const char *FName = "MDBBeginTransaction";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,EMsg)\n",
    FName);

  assert(!MDBInTransaction(MDBInfo));

  switch (MDBInfo->DBType)
    {
    case mdbSQLite3:

      return MSQLite3BeginTransaction(MDBInfo->DBUnion.SQLite3,EMsg);

      break;

    case mdbODBC:

      return MODBCBeginTransaction(MDBInfo->DBUnion.ODBC,EMsg);

      break;

    default:

      assert(FALSE);
      return(FAILURE);
    }
  return (SUCCESS);
  } /* END MDBBeginTransaction() */




/**
 * Put MDBInfo into an auto-commit transacation state and end any
 * existing transaction.
 * If MDBInfo is already in such a state, then the result is undefined.
 *
 * @param MDBInfo (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 * @param SC      (O)
 *
 * @return SUCCESS if MDBInfo was not in an auto-commit transaction state and
 * is now in such a state, with any existing transaction committed.
 *
 * FAILURE if the state transition failed.
 *
 * Undefined if MDBInfo is already in an auto-commit transaction state.
 */

int MDBEndTransaction(

  mdb_t                *MDBInfo,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  int rc;
  const char *FName = "MDBEndTransaction";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,EMsg)\n",
    FName);

#if 0
  assert(MDBInTransaction(MDBInfo));
#else
  if (!MDBInTransaction(MDBInfo))
    return(FAILURE);
#endif

  switch (MDBInfo->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3EndTransaction(MDBInfo->DBUnion.SQLite3,EMsg);

      break;

    case mdbODBC:

      rc = MODBCEndTransaction(MDBInfo->DBUnion.ODBC,EMsg,SC);

      break;

    default:

      rc = FAILURE;
      break;
    }

  return(rc);
  } /* END MDBEndTransaction() */




/**
 *
 * @param MDBInfo (I)
 *
 * @return TRUE if MDBInfo is currently within a manual-commit transaction state,
 * or FALSE otherwise.
 */

mbool_t MDBInTransaction(

  mdb_t *MDBInfo)

  {
  const char *FName = "MDBInTransaction";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo)\n",
    FName);

  switch (MDBInfo->DBType)
    {
    case mdbSQLite3:

      return MSQLite3InTransaction(MDBInfo->DBUnion.SQLite3);

      break;

    case mdbODBC:

      return MODBCInTransaction(MDBInfo->DBUnion.ODBC);

      break;

    default:

      return(FALSE);
  
      /* NO-OP */

      break;
    }
  } /* END MDBInTransaction() */



#if 0  /* NOT SUPPORTED USING unixODBC */
/**
 * The following routines (array reads and inserts) don't work with unixODBC 3.51.27.
 * They are not used and therefore are commented out
 */


/**
 * Specify that the MStmt should bind parameters as pointers to contiguous
 * arrays of data instead of as pointers to scalar values.
 *
 * NOTE: unixODBC does not support array inserts.
 *
 * @param MStmt   (I)
 * @param ArrSize (I)
 */

int MDBBindParamArrays(

  mstmt_t *MStmt,   /* I */
  int      ArrSize) /* I */

  {
  int rc;

  const char *FName = "MDBBindParamArrays";

  MDB(5,fSTRUCT) MLog("%s(MStmt,%d)\n",
    FName,
    ArrSize);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:

      rc = MSQLite3BindParamArrays(
             MStmt->StmtUnion.SQLite3,
             ArrSize);

      break;

    case mdbODBC:

      rc = FAILURE;

      break;

    default:

      /* NOT HANDLED */

      rc = FAILURE;

      break;
    }

  return(rc);
  }  /* END MDBBindParamArrays() */ 


int MDBSetScrollStmt(

  struct mstmt_t *MStmt, /* I */
  int             ScrollSize,
  void           *RowsFetched)

  {
  switch(MStmt->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SetScrollStmt(MStmt->StmtUnion.SQLite3,ScrollSize,RowsFetched));
      break;

    case mdbODBC:
      return(MODBCSetScrollStmt(MStmt->StmtUnion.ODBC,ScrollSize,RowsFetched));
      break;

    default:
      return(FAILURE);
    }
  }  /* END MDBSetScrollStmt() */



#define MMAX_ROWS_RETURNED 5

/**
 * read node transition objects from the database into an array
 *
 * @param   MDBInfo (I) the db info struct
 * @param   NArray  (O) the array to put the node transition objects into
 * @param   NName   ** not using **
 * @param   EMsg    (O) any output error message
 */
int MDBReadNodesScroll(

  mdb_t        *MDBInfo,
  marray_t     *NArray,
  char         *NName,
  marray_t     *ConstraintList,
  char         *EMsg)

  {
  int ParamIndex = 1;

  mstmt_t MStmt;

  int ArraySize = MMAX_ROWS_RETURNED;
  int RowsFetched;

  int rc = SUCCESS;

  char  Name[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  State[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  OperatingSystem[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  Architecture[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  AvailGRes[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  ConfigGRes[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  AvailClasses[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  ConfigClasses[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  Features[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  GMetric[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  HypervisorType[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  JobList[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  OldMessages[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  NetworkAddress[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  NodeSubState[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  Operations[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  OSList[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  Owner[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  ResOvercommitFactor[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  Partition[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  PowerPolicy[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  PowerState[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  PowerSelectState[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  PriorityFunction[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  ProvisioningData[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  ReservationList[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  ResourceManagerList[MMAX_ROWS_RETURNED][MMAX_LINE];
  char  VMOSList[MMAX_ROWS_RETURNED][MMAX_LINE];

  int   ConfiguredProcessors[MMAX_ROWS_RETURNED];
  int   AvailableProcessors[MMAX_ROWS_RETURNED];
  int   ConfiguredMemory[MMAX_ROWS_RETURNED];
  int   AvailableMemory[MMAX_ROWS_RETURNED];
  int   EnableProfiling[MMAX_ROWS_RETURNED];
  int   HopCount[MMAX_ROWS_RETURNED];
  int   IsDeleted[MMAX_ROWS_RETURNED];
  int   IsDynamic[MMAX_ROWS_RETURNED];
  int   LastUpdateTime[MMAX_ROWS_RETURNED];
  int   MaxJob[MMAX_ROWS_RETURNED];
  int   MaxJobPerUser[MMAX_ROWS_RETURNED];
  int   MaxProc[MMAX_ROWS_RETURNED];
  int   MaxProcPerUser[MMAX_ROWS_RETURNED];
  int   PowerIsEnabled[MMAX_ROWS_RETURNED];
  int   Priority[MMAX_ROWS_RETURNED];
  int   ProcessorSpeed[MMAX_ROWS_RETURNED];
  int   AvailableDisk[MMAX_ROWS_RETURNED];
  int   AvailableSwap[MMAX_ROWS_RETURNED];
  int   ConfiguredDisk[MMAX_ROWS_RETURNED];
  int   ConfiguredSwap[MMAX_ROWS_RETURNED];
  int   ReservationCount[MMAX_ROWS_RETURNED];
  int   Size[MMAX_ROWS_RETURNED];
  int   TotalNodeActiveTime[MMAX_ROWS_RETURNED];
  int   LastModifyTime[MMAX_ROWS_RETURNED];
  int   TotalTimeTracked[MMAX_ROWS_RETURNED];
  int   TotalNodeUpTime[MMAX_ROWS_RETURNED];
  int   TaskCount[MMAX_ROWS_RETURNED];

  double ChargeRate[MMAX_ROWS_RETURNED];
  double DynamicPriority[MMAX_ROWS_RETURNED];
  double LoadAvg[MMAX_ROWS_RETURNED];
  double MaxLoad[MMAX_ROWS_RETURNED];
  double Speed[MMAX_ROWS_RETURNED];
  double SpeedWeight[MMAX_ROWS_RETURNED];

  /* reset the node select statement since it may have been changed for a
   * previous query */

  snprintf(MDBSelectNodesStmtText,MMAX_LINE,"%s;",
    MDBNodesBaseSelect);

  /* if we have constraints then add a where clause to the select statement
   * to enforce the constraints */

  if (ConstraintList != NULL)
    MDBSelectNodesStmtAddWhereClause(ConstraintList);

  if (MDBSelectNodesStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBSetScrollStmt(&MStmt,ArraySize,&RowsFetched);

  MDBBindColText(&MStmt,ParamIndex++,Name,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,State,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,OperatingSystem,MMAX_LINE);
  MDBBindColInteger(&MStmt,ParamIndex++,ConfiguredProcessors);
  MDBBindColInteger(&MStmt,ParamIndex++,AvailableProcessors);
  MDBBindColInteger(&MStmt,ParamIndex++,ConfiguredMemory);
  MDBBindColInteger(&MStmt,ParamIndex++,AvailableMemory);
  MDBBindColText(&MStmt,ParamIndex++,Architecture,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,AvailGRes,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,ConfigGRes,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,AvailClasses,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,ConfigClasses,MMAX_LINE); 
  MDBBindColDouble(&MStmt,ParamIndex++,ChargeRate);
  MDBBindColDouble(&MStmt,ParamIndex++,DynamicPriority);
  MDBBindColInteger(&MStmt,ParamIndex++,EnableProfiling); 
  MDBBindColText(&MStmt,ParamIndex++,Features,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,GMetric,MMAX_LINE); 
  MDBBindColInteger(&MStmt,ParamIndex++,HopCount); 
  MDBBindColText(&MStmt,ParamIndex++,HypervisorType,MMAX_LINE); 
  MDBBindColInteger(&MStmt,ParamIndex++,IsDeleted); 
  MDBBindColInteger(&MStmt,ParamIndex++,IsDynamic); 
  MDBBindColText(&MStmt,ParamIndex++,JobList,MMAX_LINE);
  MDBBindColInteger(&MStmt,ParamIndex++,LastUpdateTime);
  MDBBindColDouble(&MStmt,ParamIndex++,LoadAvg); 
  MDBBindColDouble(&MStmt,ParamIndex++,MaxLoad);
  MDBBindColInteger(&MStmt,ParamIndex++,MaxJob); 
  MDBBindColInteger(&MStmt,ParamIndex++,MaxJobPerUser);
  MDBBindColInteger(&MStmt,ParamIndex++,MaxProc); 
  MDBBindColInteger(&MStmt,ParamIndex++,MaxProcPerUser); 
  MDBBindColText(&MStmt,ParamIndex++,OldMessages,MMAX_LINE); 
  MDBBindColText(&MStmt,ParamIndex++,NetworkAddress,MMAX_LINE); 
  MDBBindColText(&MStmt,ParamIndex++,NodeSubState,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,Operations,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,OSList,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,Owner,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,ResOvercommitFactor,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,Partition,MMAX_LINE);
  MDBBindColInteger(&MStmt,ParamIndex++,PowerIsEnabled);
  MDBBindColText(&MStmt,ParamIndex++,PowerPolicy,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,PowerSelectState,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,PowerState,MMAX_LINE);
  MDBBindColInteger(&MStmt,ParamIndex++,Priority);
  MDBBindColText(&MStmt,ParamIndex++,PriorityFunction,MMAX_LINE);
  MDBBindColInteger(&MStmt,ParamIndex++,ProcessorSpeed);
  MDBBindColText(&MStmt,ParamIndex++,ProvisioningData,MMAX_LINE);
  MDBBindColInteger(&MStmt,ParamIndex++,AvailableDisk);
  MDBBindColInteger(&MStmt,ParamIndex++,AvailableSwap);
  MDBBindColInteger(&MStmt,ParamIndex++,ConfiguredDisk);
  MDBBindColInteger(&MStmt,ParamIndex++,ConfiguredSwap);
  MDBBindColInteger(&MStmt,ParamIndex++,ReservationCount);
  MDBBindColText(&MStmt,ParamIndex++,ReservationList,MMAX_LINE);
  MDBBindColText(&MStmt,ParamIndex++,ResourceManagerList,MMAX_LINE);
  MDBBindColInteger(&MStmt,ParamIndex++,Size);
  MDBBindColDouble(&MStmt,ParamIndex++,Speed);
  MDBBindColDouble(&MStmt,ParamIndex++,SpeedWeight);
  MDBBindColInteger(&MStmt,ParamIndex++,TotalNodeActiveTime);
  MDBBindColInteger(&MStmt,ParamIndex++,LastModifyTime);
  MDBBindColInteger(&MStmt,ParamIndex++,TotalTimeTracked);
  MDBBindColInteger(&MStmt,ParamIndex++,TotalNodeUpTime);
  MDBBindColInteger(&MStmt,ParamIndex++,TaskCount);
  MDBBindColText(&MStmt,ParamIndex++,VMOSList,MMAX_LINE);

  MDBExecStmt(&MStmt);

  while (TRUE)
    {
    int index;
    int FetchRC;
    mtransnode_t *N;

    if ((FetchRC = MDBFetchScroll(&MStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&MStmt,NULL,EMsg);

      break;
      }

    for (index = 0;index < RowsFetched;index++)
      {
      MNodeTransitionAllocate(&N);

      MUStrCpy(N->Name,Name[index],MMAX_LINE);
      MUStrCpy(N->State,State[index],MMAX_LINE);
      MUStrCpy(N->OperatingSystem,OperatingSystem[index],MMAX_LINE);
      MUStrCpy(N->Architecture,Architecture[index],MMAX_LINE);
      MUStrCpy(N->AvailGRes,AvailGRes[index],MMAX_LINE);
      MUStrCpy(N->ConfigGRes,ConfigGRes[index],MMAX_LINE);
      MUStrCpy(N->AvailClasses,AvailClasses[index],MMAX_LINE);
      MUStrCpy(N->ConfigClasses,ConfigClasses[index],MMAX_LINE);
      MUStrCpy(N->Features,Features[index],MMAX_LINE);
      MUStrCpy(N->GMetric,GMetric[index],MMAX_LINE);
      MUStrCpy(N->HypervisorType,HypervisorType[index],MMAX_LINE);
      MUStrCpy(N->JobList,JobList[index],MMAX_LINE);
      MUStrCpy(N->OldMessages,OldMessages[index],MMAX_LINE);
      MUStrCpy(N->NetworkAddress,NetworkAddress[index],MMAX_LINE);
      MUStrCpy(N->NodeSubState,NodeSubState[index],MMAX_LINE);
      MUStrCpy(N->Operations,Operations[index],MMAX_LINE);
      MUStrCpy(N->OSList,OSList[index],MMAX_LINE);
      MUStrCpy(N->Owner,Owner[index],MMAX_LINE);
      MUStrCpy(N->ResOvercommitFactor,ResOvercommitFactor[index],MMAX_LINE);
      MUStrCpy(N->Partition,Partition[index],MMAX_LINE);
      MUStrCpy(N->PowerPolicy,PowerPolicy[index],MMAX_LINE);
      MUStrCpy(N->PowerState,PowerState[index],MMAX_LINE);
      MUStrCpy(N->PowerSelectState,PowerSelectState[index],MMAX_LINE);
      MUStrCpy(N->PriorityFunction,PriorityFunction[index],MMAX_LINE);
      MUStrCpy(N->ProvisioningData,ProvisioningData[index],MMAX_LINE);
      MUStrCpy(N->ReservationList,ReservationList[index],MMAX_LINE);
      MUStrCpy(N->ResourceManagerList,ResourceManagerList[index],MMAX_LINE);
      MUStrCpy(N->VMOSList,VMOSList[index],MMAX_LINE);
   
      N->ConfiguredProcessors   = ConfiguredProcessors[index];
      N->AvailableProcessors    = AvailableProcessors[index];
      N->ConfiguredMemory       = ConfiguredMemory[index];
      N->AvailableMemory        = AvailableMemory[index];
      N->EnableProfiling        = EnableProfiling[index];
      N->HopCount               = HopCount[index];
      N->IsDeleted              = IsDeleted[index];
      N->IsDynamic              = IsDynamic[index];
      N->LastUpdateTime         = LastUpdateTime[index];
      N->MaxJob                 = MaxJob[index];
      N->MaxJobPerUser          = MaxJobPerUser[index];
      N->MaxProc                = MaxProc[index];
      N->MaxProcPerUser         = MaxProcPerUser[index];
      N->PowerIsEnabled         = PowerIsEnabled[index];
      N->Priority               = Priority[index];
      N->ProcessorSpeed         = ProcessorSpeed[index];
      N->AvailableDisk          = AvailableDisk[index];
      N->AvailableSwap          = AvailableSwap[index];
      N->ConfiguredDisk         = ConfiguredDisk[index];
      N->ConfiguredSwap         = ConfiguredSwap[index];
      N->ReservationCount       = ReservationCount[index];
      N->Size                   = Size[index];
      N->TotalNodeActiveTime    = TotalNodeActiveTime[index];
      N->LastModifyTime         = LastModifyTime[index];
      N->TotalTimeTracked       = TotalTimeTracked[index];
      N->TotalNodeUpTime        = TotalNodeUpTime[index];
      N->TaskCount              = TaskCount[index];
   
      N->ChargeRate             = ChargeRate[index];
      N->DynamicPriority        = DynamicPriority[index];
      N->LoadAvg                = LoadAvg[index];
      N->MaxLoad                = MaxLoad[index];
      N->Speed                  = Speed[index];
      N->SpeedWeight            = SpeedWeight[index];

      /* copy the data from the bind location to the node transition record */

      MUArrayListAppendPtr(NArray,N);
      }  /* END for (index) */
    } /* END while (TRUE) */

  MDBResetStmt(&MStmt);

  return(rc);
  }  /* END MDBReadNodesScroll() */



int MDBReadJobsScroll(

  mdb_t                  *MDBInfo,
  marray_t               *JArray,
  char                   *JName,
  marray_t               *JConstraintList,
  char                   *EMsg)

  {
  int ArraySize = MMAX_ROWS_RETURNED;
  int RowsFetched;

  int ParamIndex;

  int FetchRC;

  char         JobStmtText[MMAX_LINE];
  char        *JobStmtTextPtr = JobStmtText;
  char         ReqStmtText[MMAX_LINE];
  char        *ReqStmtTextPtr = ReqStmtText;

  mstmt_t      JobStmt;
  mstmt_t      ReqStmt;

  mtransjob_t *J;
  mtransreq_t *R;

  mtransreq_t *BindR = NULL;

  int          rc = SUCCESS;
  int          rindex;

  char JobName[MMAX_LINE];

  char Name[MMAX_ROWS_RETURNED][MMAX_LINE];
  char SourceRMJobID[MMAX_ROWS_RETURNED][MMAX_LINE];
  char DestinationRMJobID[MMAX_ROWS_RETURNED][MMAX_LINE];
  char GridJobID[MMAX_ROWS_RETURNED][MMAX_LINE];
  char AName[MMAX_ROWS_RETURNED][MMAX_LINE];
  char JobGroup[MMAX_ROWS_RETURNED][MMAX_LINE];
  char User[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Group[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Account[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Class[MMAX_ROWS_RETURNED][MMAX_LINE];
  char QOS[MMAX_ROWS_RETURNED][MMAX_LINE];
  char State[MMAX_ROWS_RETURNED][MMAX_LINE];
  char ExpectedState[MMAX_ROWS_RETURNED][MMAX_LINE];
  char SubState[MMAX_ROWS_RETURNED][MMAX_LINE];
  char DestinationRM[MMAX_ROWS_RETURNED][MMAX_LINE];
  char SourceRM[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Dependencies[MMAX_ROWS_RETURNED][MMAX_BUFFER];
  char RequestedHostList[MMAX_ROWS_RETURNED][MMAX_LINE];
  char ExcludedHostList[MMAX_ROWS_RETURNED][MMAX_LINE];
  char MasterHost[MMAX_ROWS_RETURNED][MMAX_LINE];
  char ActivePartition[MMAX_ROWS_RETURNED][MMAX_LINE];
  char SpecPAL[MMAX_ROWS_RETURNED][MMAX_LINE];
  char RequiredReservation[MMAX_ROWS_RETURNED][MMAX_LINE];
  char ReservationAccess[MMAX_ROWS_RETURNED][MMAX_LINE];
  char GenericAttributes[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Flags[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Holds[MMAX_ROWS_RETURNED][MMAX_LINE];
  char StdErr[MMAX_ROWS_RETURNED][MMAX_LINE];
  char StdOut[MMAX_ROWS_RETURNED][MMAX_LINE];
  char StdIn[MMAX_ROWS_RETURNED][MMAX_LINE];
  char RMOutput[MMAX_ROWS_RETURNED][MMAX_LINE];
  char RMError[MMAX_ROWS_RETURNED][MMAX_LINE];
  char CommandFile[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Arguments[MMAX_ROWS_RETURNED][MMAX_LINE];
  char InitialWorkingDirectory[MMAX_ROWS_RETURNED][MMAX_LINE];
  char PerPartitionPriority[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Description[MMAX_ROWS_RETURNED][MMAX_LINE];
  char Messages[MMAX_ROWS_RETURNED][MMAX_LINE];
  char NotificationAddress[MMAX_ROWS_RETURNED][MMAX_LINE];
  char RMSubmitLanguage[MMAX_ROWS_RETURNED][MMAX_LINE];
  char BlockReason[MMAX_ROWS_RETURNED][MMAX_LINE];
  char BlockMsg[MMAX_ROWS_RETURNED][MMAX_LINE];

  int SubmitTime[MMAX_ROWS_RETURNED];
  int StartTime[MMAX_ROWS_RETURNED];
  int CompletionTime[MMAX_ROWS_RETURNED];
  int QueueTime[MMAX_ROWS_RETURNED];
  int SuspendTime[MMAX_ROWS_RETURNED];
  int RequestedCompletionTime[MMAX_ROWS_RETURNED];
  int RequestedStartTime[MMAX_ROWS_RETURNED];
  int SystemStartTime[MMAX_ROWS_RETURNED];
  int RequestedMinWalltime[MMAX_ROWS_RETURNED];
  int RequestedMaxWalltime[MMAX_ROWS_RETURNED];
  int UsedWalltime[MMAX_ROWS_RETURNED];
  int MinPreemptTime[MMAX_ROWS_RETURNED];
  int HoldTime[MMAX_ROWS_RETURNED];
  int Size[MMAX_ROWS_RETURNED];
  int CPULimit[MMAX_ROWS_RETURNED];
  int UMask[MMAX_ROWS_RETURNED];
  int UserPriority[MMAX_ROWS_RETURNED];
  int SystemPriority[MMAX_ROWS_RETURNED];
  int Priority[MMAX_ROWS_RETURNED];
  int RunPriority[MMAX_ROWS_RETURNED];
  int RsvStartTime[MMAX_ROWS_RETURNED];

  int CompletionCode[MMAX_ROWS_RETURNED];
  int StartCount[MMAX_ROWS_RETURNED];
  int BypassCount[MMAX_ROWS_RETURNED];
  int MaxProcessorCount[MMAX_ROWS_RETURNED];
  int RequestedNodes[MMAX_ROWS_RETURNED];

  double PSDedicated[MMAX_ROWS_RETURNED];
  double PSUtilized[MMAX_ROWS_RETURNED];
  double Cost[MMAX_ROWS_RETURNED];

  /* reset the job select statement since it may have been changed for a
   * previous query */

  snprintf(JobStmtText,MMAX_LINE,"%s;",
    MDBJobsBaseSelect);

  /* if we have constraints then add a where clause to the select statement
   * to enforce the constraints */

  if (JConstraintList != NULL)
    MDBSelectJobsStmtAddWhereClause(JConstraintList,JobStmtTextPtr);

  if (MDBSelectJobsStmt(MDBInfo,&JobStmt,JobStmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBSetScrollStmt(&JobStmt,ArraySize,&RowsFetched);

  ParamIndex = 1;

  MDBBindColText(&JobStmt,ParamIndex++,Name,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,SourceRMJobID,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,DestinationRMJobID,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,GridJobID,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,AName,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,User,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,Account,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,Class,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,QOS,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,Group,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,JobGroup,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,State,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,ExpectedState,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,SubState,MMAX_LINE);
  MDBBindColInteger(&JobStmt,ParamIndex++,UserPriority);
  MDBBindColInteger(&JobStmt,ParamIndex++,SystemPriority);
  MDBBindColInteger(&JobStmt,ParamIndex++,Priority);
  MDBBindColInteger(&JobStmt,ParamIndex++,RunPriority);
  MDBBindColText(&JobStmt,ParamIndex++,PerPartitionPriority,MMAX_LINE);
  MDBBindColInteger(&JobStmt,ParamIndex++,SubmitTime);
  MDBBindColInteger(&JobStmt,ParamIndex++,QueueTime);
  MDBBindColInteger(&JobStmt,ParamIndex++,StartTime);
  MDBBindColInteger(&JobStmt,ParamIndex++,CompletionTime);
  MDBBindColInteger(&JobStmt,ParamIndex++,CompletionCode);
  MDBBindColInteger(&JobStmt,ParamIndex++,UsedWalltime);
  MDBBindColInteger(&JobStmt,ParamIndex++,RequestedMinWalltime);
  MDBBindColInteger(&JobStmt,ParamIndex++,RequestedMaxWalltime);
  MDBBindColInteger(&JobStmt,ParamIndex++,CPULimit);
  MDBBindColInteger(&JobStmt,ParamIndex++,SuspendTime);
  MDBBindColInteger(&JobStmt,ParamIndex++,HoldTime);
  MDBBindColInteger(&JobStmt,ParamIndex++,MaxProcessorCount);
  MDBBindColInteger(&JobStmt,ParamIndex++,RequestedNodes);
  MDBBindColText(&JobStmt,ParamIndex++,SpecPAL,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,SystemPAL,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,EffectivePAL,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,DestinationRM,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,SourceRM,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,Flags,MMAX_LINE);
  MDBBindColInteger(&JobStmt,ParamIndex++,MinPreemptTime);
  MDBBindColText(&JobStmt,ParamIndex++,Dependencies,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,RequestedHostList,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,ExcludedHostList,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,MasterHost,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,GenericAttributes,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,Holds,MMAX_LINE);
  MDBBindColDouble(&JobStmt,ParamIndex++,Cost);
  MDBBindColText(&JobStmt,ParamIndex++,Description,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,Messages,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,NotificationAddress,MMAX_LINE);
  MDBBindColInteger(&JobStmt,ParamIndex++,StartCount);
  MDBBindColInteger(&JobStmt,ParamIndex++,BypassCount);
  MDBBindColText(&JobStmt,ParamIndex++,CommandFile,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,Arguments,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,RMSubmitLanguage,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,StdIn,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,StdOut,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,StdErr,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,RMOutput,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,RMError,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,InitialWorkingDirectory,MMAX_LINE);
  MDBBindColInteger(&JobStmt,ParamIndex++,UMask);
  MDBBindColInteger(&JobStmt,ParamIndex++,RsvStartTime);
  MDBBindColText(&JobStmt,ParamIndex++,BlockReason,MMAX_LINE);
  MDBBindColText(&JobStmt,ParamIndex++,BlockMsg,MMAX_LINE);
  MDBBindColDouble(&JobStmt,ParamIndex++,PSDedicated);
  MDBBindColDouble(&JobStmt,ParamIndex++,PSUtilized);

  MDBExecStmt(&JobStmt);

  MReqTransitionAllocate(&BindR);

  snprintf(ReqStmtText,MMAX_LINE,"%s;",
    MDBSelectRequestsStmtText);

  if (MDBSelectRequestsStmt(MDBInfo,&ReqStmt,ReqStmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamText(&ReqStmt,1,JobName,0);

  if (MDBBindRequest(
        &ReqStmt,
        BindR,
        NULL,
        EMsg) == FAILURE)
    {
    MReqTransitionFree(&BindR);

    return(FAILURE);
    }

  while (TRUE)
    {
    int index;

    J = NULL;

    if ((FetchRC = MDBFetchScroll(&JobStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&JobStmt,NULL,EMsg);

      MReqTransitionFree(&BindR);

      break;
      }

    for (index = 0;index < RowsFetched;index++)
      {
      MJobTransitionAllocate(&J);

      MUStrCpy(J->Name,Name[index],MMAX_LINE);
      MUStrCpy(J->SourceRMJobID,SourceRMJobID[index],MMAX_LINE);
      MUStrCpy(J->DestinationRMJobID,DestinationRMJobID[index],MMAX_LINE);
      MUStrCpy(J->GridJobID,GridJobID[index],MMAX_LINE);
      MUStrCpy(J->AName,AName[index],MMAX_LINE);
      MUStrCpy(J->JobGroup,JobGroup[index],MMAX_LINE);
      MUStrCpy(J->User,User[index],MMAX_LINE);
      MUStrCpy(J->Group,Group[index],MMAX_LINE);
      MUStrCpy(J->Account,Account[index],MMAX_LINE);
      MUStrCpy(J->Class,Class[index],MMAX_LINE);
      MUStrCpy(J->QOS,QOS[index],MMAX_LINE);
      MUStrCpy(J->State,State[index],MMAX_LINE);
      MUStrCpy(J->ExpectedState,ExpectedState[index],MMAX_LINE);
      MUStrCpy(J->SubState,SubState[index],MMAX_LINE);
      MUStrCpy(J->DestinationRM,DestinationRM[index],MMAX_LINE);
      MUStrCpy(J->SourceRM,SourceRM[index],MMAX_LINE);
      MUStrCpy(J->Dependencies,Dependencies[index],MMAX_BUFFER);
      MUStrCpy(J->RequestedHostList,RequestedHostList[index],MMAX_LINE);
      MUStrCpy(J->ExcludedHostList,ExcludedHostList[index],MMAX_LINE);
      MUStrCpy(J->MasterHost,MasterHost[index],MMAX_LINE);
      MUStrCpy(J->SpecPAL,SpecPAL[index],MMAX_LINE);
      MUStrCpy(J->SystemPAL,SystemPAL[index],MMAX_LINE);
      MUStrCpy(J->EffectivePAL,EffectivePAL[index],MMAX_LINE);
      MUStrCpy(J->RequiredReservation,RequiredReservation[index],MMAX_LINE);
      MUStrCpy(J->ReservationAccess,ReservationAccess[index],MMAX_LINE);
      MUStrCpy(J->GenericAttributes,GenericAttributes[index],MMAX_LINE);
      MUStrCpy(J->Flags,Flags[index],MMAX_LINE);
      MUStrCpy(J->Holds,Holds[index],MMAX_LINE);
      MUStrCpy(J->StdErr,StdErr[index],MMAX_LINE);
      MUStrCpy(J->StdOut,StdOut[index],MMAX_LINE);
      MUStrCpy(J->StdIn,StdIn[index],MMAX_LINE);
      MUStrCpy(J->RMOutput,RMOutput[index],MMAX_LINE);
      MUStrCpy(J->RMError,RMError[index],MMAX_LINE);
      MUStrCpy(J->CommandFile,CommandFile[index],MMAX_LINE);
      MUStrCpy(J->Arguments,Arguments[index],MMAX_LINE);
      MUStrCpy(J->InitialWorkingDirectory,InitialWorkingDirectory[index],MMAX_LINE);
      MUStrCpy(J->PerPartitionPriority,PerPartitionPriority[index],MMAX_LINE);
      MUStrCpy(J->Description,Description[index],MMAX_LINE);
      MUStrCpy(J->Messages,Messages[index],MMAX_LINE);
      MUStrCpy(J->NotificationAddress,NotificationAddress[index],MMAX_LINE);
      MUStrCpy(J->RMSubmitLanguage,RMSubmitLanguage[index],MMAX_LINE);
      MUStrCpy(J->BlockReason,BlockReason[index],MMAX_LINE);
      MUStrCpy(J->BlockMsg,BlockMsg[index],MMAX_LINE);
   
      J->SubmitTime                = SubmitTime[index];
      J->StartTime                 = StartTime[index];
      J->CompletionTime            = CompletionTime[index];
      J->QueueTime                 = QueueTime[index];
      J->SuspendTime               = SuspendTime[index];
      J->RequestedCompletionTime   = RequestedCompletionTime[index];
      J->RequestedStartTime        = RequestedStartTime[index];
      J->SystemStartTime           = SystemStartTime[index];
      J->RequestedMinWalltime      = RequestedMinWalltime[index];
      J->RequestedMaxWalltime      = RequestedMaxWalltime[index];
      J->UsedWalltime              = UsedWalltime[index];
      J->MinPreemptTime            = MinPreemptTime[index];
      J->HoldTime                  = HoldTime[index];
      J->Size                      = Size[index];
      J->CPULimit                  = CPULimit[index];
      J->UMask                     = UMask[index];
      J->UserPriority              = UserPriority[index];
      J->SystemPriority            = SystemPriority[index];
      J->Priority                  = Priority[index];                 
      J->RunPriority               = RunPriority[index]; 
      J->RsvStartTime              = RsvStartTime[index];
   
      J->CompletionCode    = CompletionCode[index];
      J->StartCount        = StartCount[index];
      J->BypassCount       = BypassCount[index];
      J->MaxProcessorCount = MaxProcessorCount[index];
      J->RequestedNodes    = RequestedNodes[index];
   
      J->PSDedicated = PSDedicated[index];
      J->PSUtilized  = PSUtilized[index];
      J->Cost        = Cost[index];
   
      MUStrCpy(JobName,J->Name,MMAX_LINE);

      /* read in req information */
   
      if (MDBExecStmt(&ReqStmt) == FAILURE)
        {
        return(FAILURE);
        }
   
      rindex = 0;
   
      while (TRUE)
        {
        R = NULL;
   
        if ((FetchRC = MDBFetch(&ReqStmt)) != SUCCESS)
          {
          if (FetchRC != MDB_FETCH_NO_DATA)
            MDBStmtError(&ReqStmt,NULL,EMsg);
   
          break;
          }
   
        MReqTransitionAllocate(&R);
        MReqTransitionCopy(BindR,R);
   
        J->Requirements[rindex++] = R;
        }
   
      MDBResetStmt(&ReqStmt);
   
      /* modify based on contextState */
   
      MUArrayListAppendPtr(JArray,J);
      }
    }   /* END while (TRUE) */

  MDBResetStmt(&JobStmt);

  return(rc);
  }  /* END MDBReadJobsScroll() */



/**
 * Read jobs from the db into an array
 *
 * @param   MDBInfo (I) the db info struct
 * @param   JArray  (I) the array for the jobs (modified)
 * @param   JName   ** not using **
 * @param   EMsg    (O) any output error message
 */

int MDBReadJobs(

  mdb_t                  *MDBInfo,
  marray_t               *JArray,
  char                   *JName,
  marray_t               *JConstraintList,
  char                   *EMsg)

  {
  int FetchRC;

  char         JobStmtText[MMAX_LINE];
  char        *JobStmtTextPtr = JobStmtText;
  char         ReqStmtText[MMAX_LINE];
  char        *ReqStmtTextPtr = ReqStmtText;

  mstmt_t      JobStmt;
  mstmt_t      ReqStmt;

  mtransjob_t *BindJ = NULL;
  mtransreq_t *BindR = NULL;

  mtransjob_t *J;
  mtransreq_t *R;

  int          rc = SUCCESS;
  int          rindex;

  char         JobName[MMAX_LINE];

  /* reset the job select statement since it may have been changed for a
   * previous query */

  snprintf(JobStmtText,MMAX_LINE,"%s;",
    MDBJobsBaseSelect);

  /* if we have constraints then add a where clause to the select statement
   * to enforce the constraints */

  if (JConstraintList != NULL)
    MDBSelectJobsStmtAddWhereClause(JConstraintList,JobStmtTextPtr);

  if (MDBSelectJobsStmt(MDBInfo,&JobStmt,JobStmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBExecStmt(&JobStmt);

  MJobTransitionAllocate(&BindJ);

  if (MDBBindJob(
        &JobStmt,
        BindJ,
        NULL,
        EMsg) == FAILURE)
    {
    MJobTransitionFree((void **)&BindJ);

    return(FAILURE);
    }

  MReqTransitionAllocate(&BindR);

  snprintf(ReqStmtText,MMAX_LINE,"%s;",
    MDBSelectRequestsStmtText);

  if (MDBSelectRequestsStmt(MDBInfo,&ReqStmt,ReqStmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBBindParamText(&ReqStmt,1,JobName,0);

  if (MDBBindRequest(
        &ReqStmt,
        BindR,
        NULL,
        EMsg) == FAILURE)
    {
    MReqTransitionFree((void **)&BindR);

    return(FAILURE);
    }

  while (TRUE)
    {
    J = NULL;

    if ((FetchRC = MDBFetch(&JobStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&JobStmt,NULL,EMsg);

      MJobTransitionFree((void **)&BindJ);

      MReqTransitionFree((void **)&BindR);

      break;
      }

    MJobTransitionAllocate(&J);
    MJobTransitionCopy(BindJ,J); /* Copy data from BindJ into J */

    MUStrCpy(JobName,J->Name,MMAX_LINE);
    /* read in req information */

    if (MDBExecStmt(&ReqStmt) == FAILURE)
      {
      return(FAILURE);
      }

    rindex = 0;

    while (TRUE)
      {
      R = NULL;

      if ((FetchRC = MDBFetch(&ReqStmt)) != SUCCESS)
        {
        if (FetchRC != MDB_FETCH_NO_DATA)
          MDBStmtError(&ReqStmt,NULL,EMsg);

        break;
        }

      MReqTransitionAllocate(&R);
      MReqTransitionCopy(BindR,R);

      J->Requirements[rindex++] = R;
      }

    MDBResetStmt(&ReqStmt);

/*
    MDBReadJobRequests(MDBInfo,&ReqConstraintList,J,EMsg);
*/

    /* modify based on contextState */

    MUArrayListAppendPtr(JArray,J);
    }

  MDBResetStmt(&JobStmt);

  return(rc);
  }  /* END MDBReadJobs() */




/**
 * Read reservations from the db into an array
 *
 * @param   MDBInfo (I) the db info struct
 * @param   RArray  (I) the array for the reservations 
 *                  (modified)
 * @param   RName   ** not using **
 * @param   EMsg    (O) any output error message
 */

int MDBReadRsvs(

  mdb_t                  *MDBInfo,
  marray_t               *RArray,
  char                   *RName,
  marray_t               *RConstraintList,
  char                   *EMsg)

  {
  int FetchRC;

  char         RsvStmtText[MMAX_LINE];
  char        *RsvStmtTextPtr = RsvStmtText;

  mstmt_t      RsvStmt;

  mtransrsv_t *BindR = NULL;

  mtransrsv_t *R;

  int          rc = SUCCESS;
  //int          rindex;

  char         RsvName[MMAX_LINE];

  /* reset the reservation select statement since it may have been changed for a
   * previous query */

  snprintf(RsvStmtText,MMAX_LINE,"%s;",
    MDBRsvsBaseSelect);

  /* if we have constraints then add a where clause to the select statement
   * to enforce the constraints */

  if (RConstraintList != NULL)
    MDBSelectRsvsStmtAddWhereClause(RConstraintList);

  if (MDBSelectRsvsStmt(MDBInfo,&RsvStmt,RsvStmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBExecStmt(&RsvStmt);

  MRsvTransitionAllocate(&BindR);

  if (MDBBindRsv(
        &RsvStmt,
        BindR,
        NULL,
        EMsg) == FAILURE)
    {
    MRsvTransitionFree((void **)&BindR);

    return(FAILURE);
    }

  while (TRUE)
    {
    R = NULL;

    if ((FetchRC = MDBFetch(&RsvStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&RsvStmt,NULL,EMsg);

      MRsvTransitionFree((void **)&BindR);

      break;
      }

    MRsvTransitionAllocate(&R);
    MRsvTransitionCopy(BindR,R); /* Copy data from BindJ into J */

    MUStrCpy(RsvName,R->Name,MMAX_LINE);

    /* modify based on contextState */

    MUArrayListAppendPtr(RArray,R);
    }

  MDBResetStmt(&RsvStmt);

  return(rc);
  }  /* END MDBReadRsvs() */




/**
 * Read triggers from the db into an array.
 *
 * @param   MDBInfo (I) the db info struct
 * @param   TArray  (I) the array for the triggers 
 *                  (modified)
 * @param   TName   ** not using **
 * @param   TConstraintList (I) constraint list
 * @param   EMsg    (O) any output error message
 */

int MDBReadTriggers(

  mdb_t                  *MDBInfo,         /* I */
  marray_t               *TArray,          /* I */
  char                   *TName,           /* I */
  marray_t               *TConstraintList, /* I */
  char                   *EMsg)            /* O */

  {
  int FetchRC;

  char         TriggerStmtText[MMAX_LINE];
  char        *TriggerStmtTextPtr = TriggerStmtText;

  mstmt_t      TrigStmt;

  mtranstrig_t *BindT = NULL;

  mtranstrig_t *T;

  int          rc = SUCCESS;

  /* reset the reservation select statement since it may have been changed for a
   * previous query */

  snprintf(TriggerStmtText,MMAX_LINE,"%s;",
    MDBTriggersBaseSelect);

  /* if we have constraints then add a where clause to the select statement
   * to enforce the constraints */

  if (TConstraintList != NULL)
    MDBSelectTriggersStmtAddWhereClause(TConstraintList);

  if (MDBSelectTriggersStmt(MDBInfo,&TrigStmt,TriggerStmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBExecStmt(&TrigStmt);

  MTrigTransitionAllocate(&BindT);

  if (MDBBindTrigger(
        &TrigStmt,
        BindT,
        NULL,
        EMsg) == FAILURE)
    {
    MTrigTransitionFree((void **)&BindT);

    return(FAILURE);
    }

  while (TRUE)
    {
    T = NULL;

    if ((FetchRC = MDBFetch(&TrigStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&TrigStmt,NULL,EMsg);

      MTrigTransitionFree((void **)&BindT);

      break;
      }

    MTrigTransitionAllocate(&T);
    MTrigTransitionCopy(BindT,T); /* Copy data from BindJ into J */

    /* modify based on contextState */

    MUArrayListAppendPtr(TArray,T);
    }

  MDBResetStmt(&TrigStmt);

  return(rc);
  }  /* END MDBReadTriggers() */




/**
 * Read VCs from the db into an array.
 *
 * @param   MDBInfo (I) the db info struct
 * @param   VCArray (I) the array for the VCs
 *                  (modified)
 * @param   VCName  ** not using **
 * @param   VCConstraintList (I) constraint list
 * @param   EMsg    (O) any output error message
 */

int MDBReadVCs(

  mdb_t                  *MDBInfo,         /* I */
  marray_t               *VCArray,         /* I */
  char                   *VCName,          /* I */
  marray_t               *VCConstraintList,/* I */
  char                   *EMsg)            /* O */

  {
  int FetchRC;

  char         VCStmtText[MMAX_LINE];
  char        *VCStmtTextPtr = VCStmtText;

  mstmt_t      VCStmt;

  mtransvc_t *BindVC = NULL;

  mtransvc_t *VC;

  int          rc = SUCCESS;

  const char *FName = "MDBReadVCs";

  MDB(9,fSTRUCT) MLog("%s(args)\n",
    FName);

  /* reset the reservation select statement since it may have been changed for a
   * previous query */

  snprintf(VCStmtText,MMAX_LINE,"%s;",
    MDBVCsBaseSelect);

  /* if we have constraints then add a where clause to the select statement
   * to enforce the constraints */

  if (VCConstraintList != NULL)
    MDBSelectVCsStmtAddWhereClause(VCConstraintList);

  if (MDBSelectVCsStmt(MDBInfo,&VCStmt,VCStmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBExecStmt(&VCStmt);

  MVCTransitionAllocate(&BindVC);

  if (MDBBindVC(
        &VCStmt,
        BindVC,
        NULL,
        EMsg) == FAILURE)
    {
    MVCTransitionFree((void **)&BindVC);

    return(FAILURE);
    }

  while (TRUE)
    {
    VC = NULL;

    if ((FetchRC = MDBFetch(&VCStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&VCStmt,NULL,EMsg);

      MVCTransitionFree((void **)&BindVC);

      break;
      }

    MVCTransitionAllocate(&VC);
    MVCTransitionCopy(BindVC,VC); /* Copy data from BindVC into VC */

    /* modify based on contextState */

    MUArrayListAppendPtr(VCArray,VC);
    }

  MDBResetStmt(&VCStmt);

  return(rc);
  }  /* END MDBReadVCs() */





/**
 * Read jobs requests from the db into the job stransition struct
 *
 * @param   MDBInfo (I) the db info struct
 * @param   ReqConstraintList  (I) the array of req constraints
 * @param   J (I/O) job transition structure
 * @param   EMsg    (O) any output error message
 */

int MDBReadJobRequests(

  mdb_t                  *MDBInfo,
  marray_t               *ReqConstraintList,
  mtransjob_t            *J,
  char                   *EMsg)

  {
  mstmt_t       MStmt;

  char          Stmt[MMAX_LINE];
  char         *StmtTextPtr = Stmt;

  mtransreq_t  *BindR = NULL;

  int rindex = 0;
  int rc = SUCCESS;

  /* reset the request select statement since it may have been changed for a
   * previous query */

  snprintf(Stmt,MMAX_LINE,"%s;",
    MDBRequestsBaseSelect);

  if (ReqConstraintList != NULL)
    MDBSelectRequestsStmtAddWhereClause(ReqConstraintList,StmtTextPtr);

  if (MDBSelectRequestsStmt(MDBInfo,&MStmt,StmtTextPtr,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBExecStmt(&MStmt);

  MReqTransitionAllocate(&BindR);

  if (MDBBindRequest(
        &MStmt,
        BindR,
        NULL,
        EMsg) == FAILURE)
    {
    MReqTransitionFree((void **)&BindR);

    return(FAILURE);
    }

  while (TRUE)
    {
    int           FetchRC;
    mtransreq_t  *R;

    R = NULL;

    if ((FetchRC = MDBFetch(&MStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&MStmt,NULL,EMsg);

      MReqTransitionFree((void **)&BindR);

      break;
      }

    MReqTransitionAllocate(&R);
    MReqTransitionCopy(BindR,R);

    J->Requirements[rindex++] = R;
    }

  MDBResetStmt(&MStmt);

  return(rc);
  }





int MDBFetchScroll(

  mstmt_t *MStmt) /* I */

  {
  const char *FName = "MDBFetchScroll";

  MDB(5,fSTRUCT) MLog("%s(MStmt)\n",
    FName);

  switch(MStmt->DBType)
    {
    case mdbSQLite3:

      return(FAILURE);

      break;

    case mdbODBC:

      return(MODBCFetchScroll(MStmt->StmtUnion.ODBC));

      break;

    default:

      break;
    }

  return(FAILURE);
  }  /* END MDBFetch() */ 




/**
 * Retrieve the statement to select all nodes from the database
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (O) the statement we'll execute later
 * @param   EMsg    (O) any output error message
 */

int MDBSelectNodesStmt(

  mdb_t   *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  const char *FName = "MDBSelectNodesStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectNodesStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectNodesStmt(MDBInfo->DBUnion.ODBC,MStmt,EMsg));

    default:
      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBSelectNodesStmt() */




/**
 * bind record from table Nodes for fetching
 * 
 * @param MStmt        (I)
 * @param N            (I) 
 * @param StmtText     (I)
 * @param EMsg         (O)
 */

int MDBBindNode(

  mstmt_t           *MStmt,        /* I */
  mtransnode_t      *N,
  char              *StmtText,     /* I */
  char              *EMsg)         /* O */

  {
  int ParamIndex = 1;

  const char *FName = "MDBBindNode";

  MDB(5,fSTRUCT) MLog("%s(MStmt,N,%s,EMsg)\n",
    FName,
    StmtText);

  MDBBindColText(MStmt,ParamIndex++,N->Name,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->State,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->OperatingSystem,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->ConfiguredProcessors);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->AvailableProcessors);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->ConfiguredMemory);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->AvailableMemory);
  MDBBindColText(MStmt,ParamIndex++,N->Architecture,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->AvailGRes,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->ConfigGRes,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->AvailClasses,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->ConfigClasses,MMAX_LINE);
  MDBBindColDouble(MStmt,ParamIndex++,&N->ChargeRate);
  MDBBindColDouble(MStmt,ParamIndex++,&N->DynamicPriority);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->EnableProfiling);
  MDBBindColText(MStmt,ParamIndex++,N->Features,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->GMetric,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->HopCount);
  MDBBindColText(MStmt,ParamIndex++,N->HypervisorType,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->IsDeleted);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->IsDynamic);
  MDBBindColText(MStmt,ParamIndex++,N->JobList,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->LastUpdateTime);
  MDBBindColDouble(MStmt,ParamIndex++,&N->LoadAvg);
  MDBBindColDouble(MStmt,ParamIndex++,&N->MaxLoad);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->MaxJob);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->MaxJobPerUser);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->MaxProc);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->MaxProcPerUser);
  MDBBindColText(MStmt,ParamIndex++,N->OldMessages,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->NetworkAddress,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->NodeSubState,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->Operations,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->OSList,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->Owner,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->ResOvercommitFactor,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->Partition,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->PowerIsEnabled);
  MDBBindColText(MStmt,ParamIndex++,N->PowerPolicy,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->PowerSelectState,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->PowerState,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->Priority);
  MDBBindColText(MStmt,ParamIndex++,N->PriorityFunction,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->ProcessorSpeed);
  MDBBindColText(MStmt,ParamIndex++,N->ProvisioningData,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->AvailableDisk);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->AvailableSwap);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->ConfiguredDisk);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->ConfiguredSwap);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->ReservationCount);
  MDBBindColText(MStmt,ParamIndex++,N->ReservationList,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,N->ResourceManagerList,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->Size);
  MDBBindColDouble(MStmt,ParamIndex++,&N->Speed);
  MDBBindColDouble(MStmt,ParamIndex++,&N->SpeedWeight);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->TotalNodeActiveTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->LastModifyTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->TotalTimeTracked);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->TotalNodeUpTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&N->TaskCount);
  MDBBindColText(MStmt,ParamIndex++,N->VMOSList,MMAX_LINE);

  return(SUCCESS);
  } /* END MDBBindNode */ 





/**
 * read node transition objects from the database into an array
 *
 * @param   MDBInfo (I) the db info struct
 * @param   NArray  (O) the array to put the node transition objects into
 * @param   NName   ** not using **
 * @param   EMsg    (O) any output error message
 */
int MDBReadNodes(

  mdb_t        *MDBInfo,
  marray_t     *NArray,
  char         *NName,
  marray_t     *ConstraintList,
  char         *EMsg)

  {
  int rc = SUCCESS;

  mstmt_t MStmt;

  mtransnode_t  *BindN = NULL;

  /* reset the node select statement since it may have been changed for a
   * previous query */

  snprintf(MDBSelectNodesStmtText,MMAX_LINE,"%s;",
    MDBNodesBaseSelect);

  /* if we have constraints then add a where clause to the select statement
   * to enforce the constraints */

  if (ConstraintList != NULL)
    MDBSelectNodesStmtAddWhereClause(ConstraintList);

  if (MDBSelectNodesStmt(MDBInfo,&MStmt,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MDBExecStmt(&MStmt);

  MNodeTransitionAllocate(&BindN);

  MDBBindNode(&MStmt,BindN,NULL,EMsg);

  while (TRUE)
    {
    int FetchRC;
    mtransnode_t *N;

    if ((FetchRC = MDBFetch(&MStmt)) != SUCCESS)
      {
      if (FetchRC != MDB_FETCH_NO_DATA)
        MDBStmtError(&MStmt,NULL,EMsg);

      MNodeTransitionFree((void **)&BindN);

      break;
      }

    MNodeTransitionAllocate(&N);

    /* copy the data from the bind location to the node transition record */

    MNodeTransitionCopy(BindN,N);

    MUArrayListAppendPtr(NArray,N);
    } /* END while (TRUE) */

  MDBResetStmt(&MStmt);

  return(rc);
  }  /* END MDBReadNode() */




/**
 * Bind a Request object for the database
 *
 * @see MDBBindJob
 *
 * @param MStmt   (I)
 * @param R       (I)
 * @param StmtText(I)
 * @param Msg     (O) [optional]
 */
int MDBBindRequest(

  mstmt_t           *MStmt,         /* I */
  mtransreq_t       *R,             /* I */
  char              *StmtText,      /* I */
  char              *Msg)           /* O */

  {
  int ParamIndex = 1;

  const char *FName = "MDBBindRequest";

  MDB(5,fSTRUCT) MLog("%s(MStmt,J,%s,EMsg)\n",
    FName,
    StmtText);

  MDBBindColText(MStmt,ParamIndex++,R->JobID,sizeof(R->JobID));
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->Index);
  MDBBindColText(MStmt,ParamIndex++,R->AllocNodeList->c_str(),sizeof(R->AllocNodeList.c_str()));
  MDBBindColText(MStmt,ParamIndex++,R->AllocPartition,sizeof(R->AllocPartition));
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->PartitionIndex);
  MDBBindColText(MStmt,ParamIndex++,R->NodeAccessPolicy,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->PreferredFeatures,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->RequestedApp,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->RequestedArch,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqOS,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqNodeSet,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqPartition,MMAX_LINE);
  /* NYI 
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->MaxNodeCount);
  */
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->MinNodeCount);
  /* NYI
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->MaxTaskCount);
  */
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->MinTaskCount);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->TaskCount);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->TaskCountPerNode);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->DiskPerTask);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->MemPerTask);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->ProcsPerTask);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->SwapPerTask);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->NodeDisk);
  MDBBindColText(MStmt,ParamIndex++,R->NodeFeatures,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->NodeMemory);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->NodeSwap);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->NodeProcs);
  MDBBindColText(MStmt,ParamIndex++,R->GenericResources,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ConfiguredGenericResources,MMAX_LINE);

  return(SUCCESS);
  }  /* END MDBBindRequest() */




/**
 * bind record from table Jobs for fetching
 * 
 * @param MStmt        (I)
 * @param J            (I)
 * @param StmtText     (I)
 * @param EMsg         (O)
 */

int MDBBindJob(

  mstmt_t           *MStmt,        /* I */
  mtransjob_t       *J,
  char              *StmtText,     /* I */
  char              *EMsg)         /* O */

  {
  int ParamIndex = 1;

  const char *FName = "MDBBindJob";

  MDB(5,fSTRUCT) MLog("%s(MStmt,J,%s,EMsg)\n",
    FName,
    StmtText);

  MDBBindColText(MStmt,ParamIndex++,J->Name,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->SourceRMJobID,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->DestinationRMJobID,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->GridJobID,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->AName,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->User,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->Account,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->Class,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->QOS,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->Group,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->JobGroup,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->State,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->ExpectedState,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->SubState,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->UserPriority);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->SystemPriority);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->Priority);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->RunPriority);
  MDBBindColText(MStmt,ParamIndex++,J->PerPartitionPriority,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->SubmitTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->QueueTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->StartTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->CompletionTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->CompletionCode);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->UsedWalltime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->RequestedMinWalltime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->RequestedMaxWalltime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->CPULimit);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->SuspendTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->HoldTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->MaxProcessorCount);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->RequestedNodes);
  MDBBindColText(MStmt,ParamIndex++,J->ActivePartition,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->SpecPAL,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->DestinationRM,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->SourceRM,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->Flags,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->MinPreemptTime);
  MDBBindColText(MStmt,ParamIndex++,J->Dependencies,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->RequestedHostList,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->ExcludedHostList,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->MasterHost,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->GenericAttributes,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->Holds,MMAX_LINE);
  MDBBindColDouble(MStmt,ParamIndex++,&J->Cost);
  MDBBindColText(MStmt,ParamIndex++,J->Description,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->Messages,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->NotificationAddress,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->StartCount);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->BypassCount);
  MDBBindColText(MStmt,ParamIndex++,J->CommandFile,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->Arguments,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->RMSubmitLanguage,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->StdIn,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->StdOut,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->StdErr,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->RMOutput,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->RMError,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->InitialWorkingDirectory,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->UMask);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&J->RsvStartTime);
  MDBBindColText(MStmt,ParamIndex++,J->BlockReason,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,J->BlockMsg,MMAX_LINE);
  MDBBindColDouble(MStmt,ParamIndex++,&J->PSDedicated);
  MDBBindColDouble(MStmt,ParamIndex++,&J->PSUtilized);
  return(SUCCESS);
  } /* END MDBBindJob */ 




/**
 * bind record from table Reservation for fetching
 * 
 * @param MStmt        (I)
 * @param R            (I)
 * @param StmtText     (I)
 * @param EMsg         (O)
 */

int MDBBindRsv(

  mstmt_t           *MStmt,        /* I */
  mtransrsv_t       *R,
  char              *StmtText,     /* I */
  char              *EMsg)         /* O */

  {
  const char *FName = "MDBBindRsv";

  MDB(5,fSTRUCT) MLog("%s(MStmt,R,%s,EMsg)\n",
    FName,
    StmtText);

  int ParamIndex = 1;

  MDBBindColText(MStmt,ParamIndex++,R->Name,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ACL,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->AAccount,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->AGroup,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->AUser,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->AQOS,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->AllocNodeCount);
  MDBBindColText(MStmt,ParamIndex++,R->AllocNodeList->c_str(),MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->AllocProcCount);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->AllocTaskCount);
  MDBBindColText(MStmt,ParamIndex++,R->CL,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Comment,MMAX_LINE);
  MDBBindColDouble(MStmt,ParamIndex++,&R->Cost);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->CTime);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->Duration);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->EndTime);
  MDBBindColText(MStmt,ParamIndex++,R->ExcludeRsv,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->ExpireTime);
  MDBBindColText(MStmt,ParamIndex++,R->Flags,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->GlobalID,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->HostExp,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->History,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Label,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->LastChargeTime);
  MDBBindColText(MStmt,ParamIndex++,R->LogLevel,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Messages,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Owner,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Partition,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Profile,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqArch,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqFeatureList,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqMemory,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqNodeCount,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqNodeList,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqOS,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqTaskCount,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->ReqTPN,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Resources,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->RsvAccessList,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->RsvGroup,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->RsvParent,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->SID,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&R->StartTime);
  MDBBindColDouble(MStmt,ParamIndex++,&R->StatCAPS);
  MDBBindColDouble(MStmt,ParamIndex++,&R->StatCIPS);
  MDBBindColDouble(MStmt,ParamIndex++,&R->StatTAPS);
  MDBBindColDouble(MStmt,ParamIndex++,&R->StatTIPS);
  MDBBindColText(MStmt,ParamIndex++,R->SubType,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Trigger,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Type,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->Variables.c_str(),MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,R->VMList,MMAX_LINE);
  return(SUCCESS);
  } /* END MDBBindRsv */ 




/**
 * bind record from table Trigger for fetching
 * 
 * @param MStmt        (I)
 * @param T            (I)
 * @param StmtText     (I)
 * @param EMsg         (O)
 */

int MDBBindTrigger(

  mstmt_t           *MStmt,        /* I */
  mtranstrig_t      *T,            /* I */
  char              *StmtText,     /* I */
  char              *EMsg)         /* O */

  {
  const char *FName = "MDBBindTrigger";

  MDB(5,fSTRUCT) MLog("%s(MStmt,T,%s,EMsg)\n",
    FName,
    StmtText);

  int ParamIndex = 1;

  MDBBindColText(MStmt,ParamIndex++,T->TrigID,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->Name,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->ActionData,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->ActionType,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->BlockTime);
  MDBBindColText(MStmt,ParamIndex++,T->Description,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->Disabled);
  MDBBindColText(MStmt,ParamIndex++,T->EBuf,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->EventTime);
  MDBBindColText(MStmt,ParamIndex++,T->EventType,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->FailOffset,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->FailureDetected);
  MDBBindColText(MStmt,ParamIndex++,T->Flags,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->IsComplete);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->IsInterval);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->LaunchTime);
  MDBBindColText(MStmt,ParamIndex++,T->Message,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->MultiFire,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->ObjectID,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->ObjectType,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->OBuf,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->Offset,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->Period,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->PID);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->RearmTime);
  MDBBindColText(MStmt,ParamIndex++,T->Requires,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->Sets,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->State,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,T->Threshold,MMAX_LINE);
  MDBBindColInteger(MStmt,ParamIndex++,(int *)&T->Timeout);
  MDBBindColText(MStmt,ParamIndex++,T->Unsets,MMAX_LINE);

  return(SUCCESS);
  } /* END MDBBindTrigger */ 



/**
 * bind record from table VC for fetching
 *
 * @param MStmt        (I)
 * @param VC           (I)
 * @param StmtText     (I)
 * @param EMsg         (O)
 */

int MDBBindVC(

  mstmt_t           *MStmt,        /* I */
  mtransvc_t        *VC,           /* I */
  char              *StmtText,     /* I */
  char              *EMsg)         /* O */

  {
  int ParamIndex;

  const char *FName = "MDBBindVC";

  MDB(5,fSTRUCT) MLog("%s(MStmt,VC,%s,EMsg)\n",
    FName,
    StmtText);

  ParamIndex = 1;

  MDBBindColText(MStmt,ParamIndex++,VC->Name,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->Description,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->Jobs,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->Nodes,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->VMs,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->Rsvs,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->VCs,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->Variables,MMAX_LINE);
  MDBBindColText(MStmt,ParamIndex++,VC->Flags,MMAX_LINE);

  return(SUCCESS);
  }  /* END MDBBindVC */




/**
 * Retrieve the statement to select requests from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (I) the statement we'll execute later
 * @param   EMsg    (I) any output error message
 */

int MDBSelectRequestsStmt(

  mdb_t   *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  const char *FName = "MDBSelectRequestsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectRequestsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectRequestsStmt(MDBInfo->DBUnion.ODBC,MStmt,StmtTextPtr,EMsg));

    default:
      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBSelectRequestsStmt() */



/**
 * Put a WHERE clause in the select statement to apply the requested constraint.
 *
 * @param   ReqConstraintList (I) constraint stuct for the job select statement
 * @param   StmtTextPtr (I) statement text pointer
 */

int MDBSelectRequestsStmtAddWhereClause(

  marray_t *ReqConstraintList, /* I */
  char     *StmtTextPtr)       /* I */

  {
  const char *FName = "MDBSelectRequestsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(JConstraint)\n",
    FName);

  int index;

  char *WherePtr;
  char  Clause[MMAX_LINE];

  Clause[0] = '\0';

  if (ReqConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the select statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(StmtTextPtr,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < ReqConstraintList->NumItems; index++)
    {
    mjobconstraint_t *ReqConstraint;

    ReqConstraint = (mjobconstraint_t *)MUArrayListGet(ReqConstraintList,index);

    switch (ReqConstraint->AType)
      {
      default:
  
        return(FAILURE);
  
        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBSelectRequestStmtAddWhereClause() */




/**
 * Retrieve the statement to select all jobs from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (I) the statement we'll execute later
 * @param   EMsg    (I) any output error message
 */

int MDBSelectJobsStmt(

  mdb_t   *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  const char *FName = "MDBSelectJobsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectJobsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectJobsStmt(MDBInfo->DBUnion.ODBC,MStmt,StmtTextPtr,EMsg));

    default:
      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBSelectJobsStmt() */




/**
 * Retrieve the statement to select all reservations from the db
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (I) the statement we'll execute later 
 * @param   StmtTextPtr (I) the text pointer 
 * @param   EMsg    (I) any output error message
 */

int MDBSelectRsvsStmt(

  mdb_t   *MDBInfo,     /* I */
  mstmt_t *MStmt,       /* I */
  char    *StmtTextPtr, /* I */
  char    *EMsg)        /* I */

  {
  const char *FName = "MDBSelectRsvsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectRsvsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectRsvsStmt(MDBInfo->DBUnion.ODBC,MStmt,StmtTextPtr,EMsg));

    default:
      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBSelectRsvsStmt() */




/**
 * Retrieve the statement to select all triggers from the db.
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (I) the statement we'll execute later 
 * @param   StmtTextPtr (I) the text pointer
 * @param   EMsg    (I) any output error message
 */

int MDBSelectTriggersStmt(

  mdb_t   *MDBInfo,     /* I */
  mstmt_t *MStmt,       /* I */
  char    *StmtTextPtr, /* I */
  char    *EMsg)        /* I */

  {
  const char *FName = "MDBSelectTriggersStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectTriggersStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectTriggersStmt(MDBInfo->DBUnion.ODBC,MStmt,StmtTextPtr,EMsg));

    default:
      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBSelectTriggersStmt() */



/**
 * Retrieve the statement to select all VCs from the db.
 *
 * @param   MDBInfo (I) the db info struct
 * @param   MStmt   (I) the statement we'll execute later 
 * @param   StmtTextPtr (I) the text pointer
 * @param   EMsg    (I) any output error message
 */

int MDBSelectVCsStmt(

  mdb_t   *MDBInfo,     /* I */
  mstmt_t *MStmt,       /* I */
  char    *StmtTextPtr, /* I */
  char    *EMsg)        /* I */

  {
  const char *FName = "MDBSelectVCsStmt";

  MDB(5,fSTRUCT) MLog("%s(MDBInfo,MStmt,EMsg)\n",
    FName);

  switch(MDBInfo->DBType)
    {
    case mdbSQLite3:
      return(MSQLite3SelectVCsStmt(MDBInfo->DBUnion.SQLite3,MStmt,EMsg));

    case mdbODBC:
      return(MODBCSelectVCsStmt(MDBInfo->DBUnion.ODBC,MStmt,StmtTextPtr,EMsg));

    default:
      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MDBSelectVCsStmt() */




/**
 * Put a WHERE clause in the select statement to apply the requested constraint
 *
 * @param   JConstraint (I) constraint stuct for the job select statement
 */

int MDBSelectJobsStmtAddWhereClause(

  marray_t *JConstraintList,
  char     *StmtTextPtr)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];
  mbool_t OpenORClauseParen = FALSE;
  mbool_t ORClauseParenRequired = FALSE;

  const char *FName = "MDBSelectJobsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(JConstraint)\n",
    FName);

  Clause[0] = '\0';

  if (JConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the select statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(StmtTextPtr,";")) == NULL)
    return(FAILURE);

  /* see if we have both OR and AND clauses. If so we assume that the OR
   * clauses are grouped together and come first in the list of constraints.
   * If we do have both then set a boolean so that we know to add parens
   * around all of the OR statements. */

  if (JConstraintList->NumItems > 1)
    {
    mbool_t ORClause = FALSE;
    mbool_t ANDClause = FALSE;

    for (index = 0; index < JConstraintList->NumItems; index++)
      {
      mjobconstraint_t *JConstraint;
  
      JConstraint = (mjobconstraint_t *)MUArrayListGet(JConstraintList,index);

      if (JConstraint->AType == mjaState)
        ORClause = TRUE;
      else if (JConstraint->AType == mjaPAL)
        ANDClause = TRUE;

      if ((ORClause == TRUE) && (ANDClause == TRUE))
        {
        ORClauseParenRequired = TRUE;
        break;
        }
      } /* END for loop */
    } /* END JConstraintList->NumItems > 1 */

  for (index = 0; index < JConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mjobconstraint_t *JConstraint;

    JConstraint = (mjobconstraint_t *)MUArrayListGet(JConstraintList,index);

    switch (JConstraint->AType)
      {
      case mjaState:
        {
        enum MJobStateEnum State = (enum MJobStateEnum)JConstraint->ALong[0];
  
        if ((State <= mjsNONE) || (State >= mjsLAST))
          return(FAILURE);

        switch (State)
          {
          case mjsHold:

            if (JConstraint->ACmp != mcmpEQ)
              return(FAILURE);

            sprintf(tmpStr," %s State = 'Hold'",
              (Clause[0] != '\0') ? "OR " : "");

            break;

          case mjsCompleted:
          case mjsIdle:
          case mjsRunning:
          default:

            if ((JConstraint->ACmp != mcmpEQ) && (JConstraint->ACmp != mcmpNE))
              return(FAILURE);
      
            sprintf(tmpStr," %s State %s '%s'",
              (Clause[0] != '\0') ? "OR " : "",
              (JConstraint->ACmp == mcmpEQ) ? "=" : "!=",
              MJobState[State]);               

            break;
          } /* END switch (State) */

        if ((ORClauseParenRequired == TRUE) && (OpenORClauseParen == FALSE))
          {
          MUStrCat(Clause,"(",sizeof(Clause));
          OpenORClauseParen = TRUE;
          }

        MUStrCat(Clause,tmpStr,sizeof(Clause));          
        }
  
        break;
  
      case mjaPAL:
        {
        char *PName = &JConstraint->AVal[0][0];

        if (JConstraint->ACmp != mcmpSSUB)
          return(FAILURE);

        sprintf(tmpStr," %s SpecPAL LIKE '%%[%s]%%'",
          (Clause[0] != '\0') ? "AND " : "",
          PName);
                       
        if ((ORClauseParenRequired == TRUE) && (OpenORClauseParen == TRUE))
          {
          MUStrCat(Clause,")",sizeof(Clause));
          OpenORClauseParen = FALSE;
          }

        MUStrCat(Clause,tmpStr,sizeof(Clause));          
        }

        break;

      case mjaCompletionTime:

        sprintf(tmpStr," %s CompletionTime > %ld",
          (Clause[0] != '\0') ? "AND " : "",
          JConstraint->ALong[0]);

        MUStrCat(Clause,tmpStr,sizeof(Clause));          

        break;

      case mjaJobID:
        {
        char *formatStr = (Clause[0] == '\0') ? (char *)" (ID %s '%s')" : (char *)" OR (ID %s '%s')";

        sprintf(tmpStr,
          formatStr,
          MComp[JConstraint->ACmp],
          JConstraint->AVal[0]);

        MUStrCat(Clause,tmpStr,sizeof(Clause));   
        }
        break;
      default:
  
        return(FAILURE);
  
        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBSelectJobsStmtAddWhereClause() */

/**
 * Put a WHERE clause in the select statement to apply the requested constraint
 *
 * @param   Constraint (I) constraint stuct for the node select
 *                     statement
 */

int MDBSelectNodesStmtAddWhereClause(

  marray_t *ConstraintList)

  {
  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  const char *FName = "MDBSelectNodesStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(Constraint)\n",
    FName);

  Clause[0] = '\0';

  if (ConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the select statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBSelectNodesStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < ConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mnodeconstraint_t *Constraint;

    Constraint = (mnodeconstraint_t *)MUArrayListGet(ConstraintList,index);

    switch (Constraint->AType)
      {
      case mnaNodeID:
        {
         if(Clause[0] != '\0')
          MUStrCat(tmpStr," OR ",sizeof(tmpStr));

        sprintf(tmpStr,
          " (ID %s '%s')",
          MComp[Constraint->ACmp],
          Constraint->AVal[0]);

        MUStrCat(Clause,tmpStr,sizeof(Clause));   
        }
        break;

      case mnaPartition:
        {
        if(Clause[0] != '\0')
          MUStrCat(tmpStr," OR ",sizeof(tmpStr));

        sprintf(tmpStr,
          "Partition %s '%s'",
          MComp[Constraint->ACmp],
          Constraint->AVal[0]);

        MUStrCat(Clause,tmpStr,sizeof(Clause));   
        }
        break;

      default:
  
        return(FAILURE);
  
        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {  
    sprintf(WherePtr," WHERE %s;",
      Clause);               
    }

  return(SUCCESS);
  }  /* END MDBSelectNodesStmtAddWhereClause() */



/**
 * Put a WHERE clause in the select statement to apply the requested constraint
 *
 * @param   Constraint (I) constraint stuct for the reservation 
 *                     select statement
 */

int MDBSelectRsvsStmtAddWhereClause(

  marray_t *ConstraintList)

  {
  const char *FName = "MDBSelectRsvsStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(Constraint)\n",
    FName);

  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  Clause[0] = '\0';

  if (ConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the select statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBSelectRsvsStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < ConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mrsvconstraint_t *Constraint;

    Constraint = (mrsvconstraint_t *)MUArrayListGet(ConstraintList,index);

    switch (Constraint->AType)
      {
      case mraName:
        {
         if(Clause[0] != '\0')
          MUStrCat(tmpStr," OR ",sizeof(tmpStr));

        sprintf(tmpStr,
          " (Name %s '%s')",
          MComp[Constraint->ACmp],
          Constraint->AVal[0]);

        MUStrCat(Clause,tmpStr,sizeof(Clause));
        }
        break;

      default:

        return(FAILURE);

        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {
    sprintf(WherePtr," WHERE %s;",
      Clause);
    }

  return(SUCCESS);
  }  /* END MDBSelectRsvsStmtAddWhereClause() */




/**
 * Put a WHERE clause in the select statement to apply the requested constraint.
 *
 * @param   Constraint (I) constraint stuct for the trigger 
 *                     select statement
 */

int MDBSelectTriggersStmtAddWhereClause(

  marray_t *ConstraintList) /* I */

  {
  const char *FName = "MDBSelectTriggersStmtAddWhereClause";

  MDB(5,fSTRUCT) MLog("%s(Constraint)\n",
    FName);

  int index;

  char *WherePtr;
  char Clause[MMAX_LINE];

  Clause[0] = '\0';

  if (ConstraintList == NULL)
    return(FAILURE);

  /* get a pointer to the end of the select statement which is where we will
   * add a WHERE clause if necessary */

  if ((WherePtr = strpbrk(MDBSelectTriggersStmtText,";")) == NULL)
    return(FAILURE);

  for (index = 0; index < ConstraintList->NumItems; index++)
    {
    char tmpStr[MMAX_LINE];
    mtrigconstraint_t *Constraint;

    Constraint = (mtrigconstraint_t *)MUArrayListGet(ConstraintList,index);

    switch (Constraint->AType)
      {
      case mraName:
        {
         if(Clause[0] != '\0')
          MUStrCat(tmpStr," OR ",sizeof(tmpStr));

        sprintf(tmpStr,
          " (TrigID %s '%s')",
          MComp[Constraint->ACmp],
          Constraint->AVal[0]);

        MUStrCat(Clause,tmpStr,sizeof(Clause));
        }
        break;

      default:

        return(FAILURE);

        break;
      } /* END switch */
    } /* for index .... */

  if (Clause[0] != '\0')
    {
    sprintf(WherePtr," WHERE %s;",
      Clause);
    }

  return(SUCCESS);
  }  /* END MDBSelectTriggersStmtAddWhereClause() */






#endif /* O - NOT SUPPORTED */

/* END MDBI.c */
