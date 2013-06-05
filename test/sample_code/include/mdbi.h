/* 
 * File:   mdbi.h
 * Author: sreque
 *
 * Created on August 13, 2008, 11:59 AM
 */

#ifndef _MDBI_H
#define	_MDBI_H

#include "moab.h"

#define SQLITE3_SUCCEEDED(x) (((x) == SQLITE_OK) ? SUCCESS : FAILURE)
#define MDB_FETCH_NO_DATA (SUCCESS << 1)

/**
 *NOTES
 *As a rule of thumb, most, if not all structures defined in this file
 *need to be zero-initialized to work properly. Functions that initialize and 
 *free the resources contained in them generally assume the struct was zero-initialized.
 */

  /** forward declarations **/

typedef struct modbc_t modbc_t;
typedef struct mstmt_odbc_t mstmt_odbc_t;
typedef struct mpstmt_odbc_t mpstmt_odbc_t;
typedef struct mdstmt_odbc_t mdstmt_odbc_t;
typedef struct msqlite3_value_t msqlite3_value_t;
typedef struct mstmt_sqlite3_t mstmt_sqlite3_t;
typedef struct msqlite3_t msqlite3_t;

/**sqlite3 declarations **/

typedef enum MSQLite3StmtState {
    msqlite3Invalid,
    msqlite3FetchReady,
    msqlite3FetchDone,
    msqlite3Error
} MSQLite3StmtState;

/**odbc declarations */

#ifndef MDISABLEODBC
#include "sqltypes.h"
#else
#define SQLCHAR unsigned char
#define SQLHENV int
#define SQLHDBC int
#define SQLHSTMT void *
#define SQLINTEGER int
#define SQLLEN int
#define SQLUSMALLINT unsigned int
#define SQLSMALLINT int
#define SQLUINTEGER unsigned int
#define SQLPOINTER void *
#define SQLHANDLE int
#define SQL_HANDLE_STMT 0
#define SQLFreeHandle(I,J)
#endif /* MODBC */


typedef struct mstmt_odbc_vtbl 
  {
  int (*ExecFunc)(struct mstmt_odbc_t* Stmt);
  } mstmt_odbc_vtbl;

struct mstmt_odbc_t 
  {
  modbc_t *DB;
  const mstmt_odbc_vtbl *vtbl;
  SQLHSTMT Handle;
  mbool_t IsInitialized;
  unsigned char *StmtText;
  };

struct mpstmt_odbc_t 
  {
  mstmt_odbc_t super;
  };

struct mdstmt_odbc_t 
  {
  mstmt_odbc_t super;
  };

/* odbc object */

struct modbc_t {
  SQLHENV  SQLEnv;              /* ODBC environment handle (is NULL if we aren't connected) */
  long	   SQLErrorNo;          /* result of functions */
  SQLHDBC  SQLHandle;           /* connection handle */

  SQLCHAR  SQLStatus[10];       /* status */

  mbool_t  ConnectedOnce;       /* used to keep track of whether at least one successful connection has been made */

  mbool_t  ConnectedReadOnly;   /* indicates that the connection to the db for this thread is read only */
  mbool_t  ReadOnlyRequest;     /* set by a thread to request that the connection be set to read only */ 

  mpstmt_odbc_t InsertCPStmt; 
  mpstmt_odbc_t PurgeCPStmt; 
  mpstmt_odbc_t QueryCPStmt; 
  mpstmt_odbc_t DeleteCPStmt;
  mpstmt_odbc_t IJobQueryStmt;
  mpstmt_odbc_t InsertGeneralStatsStmt;
  mpstmt_odbc_t InsertNodeStatsStmt;
  mpstmt_odbc_t InsertEventStmt;
  mpstmt_odbc_t SelectEventsStmt;
  mpstmt_odbc_t SelectGeneralStatsStmt;
  mpstmt_odbc_t SelectGenericMetricsStmt;
  mpstmt_odbc_t SelectGeneralStatsRangeStmt;
  mpstmt_odbc_t SelectGenericMetricsRangeStmt; 
  mpstmt_odbc_t SelectGenericResourcesRangeStmt; 
  mpstmt_odbc_t SelectNodeStatsStmt;
  mpstmt_odbc_t SelectNodeStatsRangeStmt;
  mpstmt_odbc_t SelectNodeStatsGenericResourcesStmt;
  mpstmt_odbc_t SelectNodeStatsGenericResourcesRangeStmt;
  mpstmt_odbc_t SelectJobsStmt;
  mpstmt_odbc_t SelectRsvsStmt;
  mpstmt_odbc_t SelectTriggersStmt;
  mpstmt_odbc_t SelectVCStmt;
  mpstmt_odbc_t SelectRequestsStmt;
  mpstmt_odbc_t SelectNodesStmt;
  mpstmt_odbc_t DeleteJobsStmt;
  mpstmt_odbc_t DeleteRsvsStmt;
  mpstmt_odbc_t DeleteTriggersStmt;
  mpstmt_odbc_t DeleteVCStmt;
  mpstmt_odbc_t DeleteRequestsStmt;
  mpstmt_odbc_t DeleteNodesStmt;
  mpstmt_odbc_t InsertJobStmt;
  mpstmt_odbc_t InsertRsvStmt;
  mpstmt_odbc_t InsertTriggerStmt;
  mpstmt_odbc_t InsertVCStmt;
  mpstmt_odbc_t InsertRequestStmt;
  mpstmt_odbc_t InsertNodeStmt;
  };

#define SQL_FAIL(retval) COND_FAIL(!SQL_SUCCEEDED((retval)))

/**common declarations**/
extern const char *MDBGeneralStatBaseSelect;
extern const char *MDBGenericMetricsBaseSelect;
extern const char *MDBNodeStatBaseSelect;
extern const char *MDBNodeStatsGenericResourcesBaseSelect;
extern const char *MDBQueryCPStmtText;
extern const char *MDBInsertCPStmtText;
extern const char *MDBPurgeCPStmtText;
extern const char *MDBIJobQueryStmtText;
extern const char *MDBDeleteCPStmtText;
extern const char *MDBInsertNodeStatsStmtText;
extern const char *MDBInsertEventStmtText;
extern const char *MDBInsertJobStmtText;
extern const char *MDBInsertRsvStmtText;
extern const char *MDBInsertTriggerStmtText;
extern const char *MDBInsertVCStmtText;
extern const char *MDBInsertRequestStmtText;
extern const char *MDBInsertNodeStmtText;
extern const char *MDBSelectEventsStmtText;
extern const char *MDBInsertGeneralStatsStmtText;
extern char  MDBSelectGeneralStatsStmtText[];
extern char  MDBSelectNodeStatsStmtText[];
extern char  MDBSelectNodeStatsRangeStmtText[];
extern char  MDBSelectNodeStatsGenericResourcesStmtText[];
extern char  MDBSelectNodeStatsGenericResourcesRangeStmtText[];
extern char  MDBSelectGeneralStatsRangeStmtText[];
extern char  MDBSelectGenericMetricsStmtText[];
extern char  MDBSelectGenericMetricsRangeStmtText[];
extern char  MDBSelectJobsStmtText[];
extern char  MDBSelectRsvsStmtText[];
extern char  MDBSelectTriggersStmtText[];
extern char  MDBSelectVCsStmtText[];
extern char  MDBSelectRequestsStmtText[];
extern char  MDBSelectNodesStmtText[];
extern char  MDBDeleteJobsStmtText[];
extern char  MDBDeleteRsvsStmtText[];
extern char  MDBDeleteTriggersStmtText[];
extern char  MDBDeleteVCsStmtText[];
extern char  MDBDeleteRequestsStmtText[];
extern char  MDBDeleteNodesStmtText[];

typedef enum MStmtType {
  mstmtPrepared,
  mstmtDirect
} MStmtType;

typedef union mstmt_union {
  mstmt_sqlite3_t *SQLite3;
  struct mstmt_odbc_t    *ODBC;
  struct mpstmt_odbc_t   *PODBC;
  struct mdstmt_odbc_t   *DODBC;
} mstmt_union;

typedef struct mstmt_t {
  mstmt_union StmtUnion;
  MDBTypeEnum DBType;
  MStmtType StmtType;
} mstmt_t;

int MODBCPurgeCPStmt(modbc_t *,mstmt_t *);
int MODBCStmtError(mstmt_odbc_t *,char const *,char *,enum MStatusCodeEnum *);
int MODBCConnect(modbc_t *,char *);
void MODBCUpdateMessage(int,char *);
int MODBCBindParamDouble(mstmt_odbc_t *,int,double *,int);
int MODBCSelectGeneralStatsStmt(modbc_t *,mstmt_t *,char *);
int MODBCSelectEventsMsgsStmt(modbc_t *,mstmt_t *,char *);
int MODBCSelectGenericMetricsStmt(modbc_t *,mstmt_t *,char *);
int MODBCNumResultCols(mstmt_odbc_t *,int *);
int MODBCIsIgnorableError(SQLCHAR *,SQLINTEGER,SQLCHAR *);
int MODBCBindColText(mstmt_odbc_t *,int,void *,int);
int MODBCResetStmt(mstmt_odbc_t *);
int MODBCBindParameter(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLUINTEGER,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLLEN *,unsigned char *,char *);
int MODBCQueryCPStmt(modbc_t *,mstmt_t *,char *);
int MODBCSelectGenericMetricsRangeStmt(modbc_t *,mstmt_t *,char *);
int MODBCFree(modbc_t *);
int MODBCInitAllocatedDirectStmt(modbc_t *,mdstmt_odbc_t *,char const *,char *);
int MODBCBuildConnectionString(modbc_t *,mstring_t *);
int MODBCBindCol(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLLEN *,SQLCHAR *,char *);
int MODBCFreeStmt(mstmt_odbc_t *);
int MODBCInsertGeneralStatsStmt(modbc_t *,mstmt_t *,char *);
int MODBCFetch(mstmt_odbc_t *);
int MODBCFetchScroll(mstmt_odbc_t *);
int MODBCError(SQLHANDLE,SQLSMALLINT,char const *,char *,enum MStatusCodeEnum *);
int MODBCBindParamInteger(mstmt_odbc_t *,int,int *,int);
int MODBCSelectNodeStatsStmt(modbc_t *,mstmt_t *,char *);
int MODBCExecStmt(mstmt_odbc_t *);
int MODBCSelectNodeStatsRangeStmt(modbc_t *,mstmt_t *,char *);
int MODBCSelectNodeStatsGenericResourcesRangeStmt(modbc_t *,mstmt_t *,char *);
int MODBCIJobQueryStmt(modbc_t *,mstmt_t *,char *);
int MODBCDeleteCPStmt(modbc_t *,mstmt_t *,char *);
int MODBCBindParamText(mstmt_odbc_t *,int,char const *,int);
int MODBCBindColInteger(mstmt_odbc_t *,int,int *);
int MODBCInitPrepStmt(modbc_t *,mpstmt_odbc_t *,const char *,char *);
int MODBCSelectGeneralStatsRangeStmt(modbc_t *,mstmt_t *,char *);
int MODBCInitialize(mdb_t *);
int MODBCAllocHandle(SQLSMALLINT,SQLHANDLE,SQLHANDLE *,char *);
int MODBCSetPreparedStmt(modbc_t *,mstmt_t *,mpstmt_odbc_t *,const char *,char *);
int MODBCAllocDirectStmt(modbc_t *,mstmt_t *,char const *,char *);
int MODBCAllocPrepStmt(modbc_t *,mstmt_t *,const char *,char *);
int MODBCSelectNodeStatsGenericResourcesStmt(modbc_t *,mstmt_t *,char *);
int MODBCSelectJobsStmt(modbc_t *,mstmt_t *,char *,char *);
int MODBCSelectRsvsStmt(modbc_t *,mstmt_t *,char *,char *);
int MODBCSelectTriggersStmt(modbc_t *,mstmt_t *,char *,char *);
int MODBCSelectVCsStmt(modbc_t *,mstmt_t *,char *,char *);
int MODBCSelectRequestsStmt(modbc_t *, mstmt_t *,char *,char *);
int MODBCSelectNodesStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertNodeStatsStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertEventStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertJobStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertRsvStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertTriggerStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertVCStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertRequestStmt(modbc_t *,mstmt_t *,char *);
int MODBCInsertNodeStmt(modbc_t *,mstmt_t *,char *);
int MODBCExecPrepStmt(mpstmt_odbc_t *,enum MStatusCodeEnum *);
int MODBCExecDirectStmt(mdstmt_odbc_t *,enum MStatusCodeEnum *);
int MODBCBindColDouble(mstmt_odbc_t *,int,double *);
int MODBCInsertCPStmt(modbc_t *,mstmt_t *,char *);
int MODBCNumChangedRows(mstmt_odbc_t *,int *);
int MODBCBindParamBlob(mstmt_odbc_t *,int,void *,int);
int MODBCBeginTransaction(modbc_t *,char *);
int MODBCEndTransaction(modbc_t *,char *,enum MStatusCodeEnum *);
int MODBCDeleteJobsStmt(modbc_t *,mstmt_t *,char *);
int MODBCDeleteRsvsStmt(modbc_t *,mstmt_t *,char *);
int MODBCDeleteTriggersStmt(modbc_t *,mstmt_t *,char *);
int MODBCDeleteVCsStmt(modbc_t *,mstmt_t *,char *);
int MODBCDeleteRequestsStmt(modbc_t *,mstmt_t *,char *);
int MODBCDeleteNodesStmt(modbc_t *,mstmt_t *,char *);
mbool_t MODBCInTransaction(modbc_t *);

int MSQLite3SelectNodeStatsGenericResourcesStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectJobsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectRsvsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectTriggersStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectVCsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectRequestsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectNodesStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertNodeStatsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertEventStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertJobStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertRsvStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertTriggerStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertVCStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertRequestStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3InsertNodeStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3ResetStmt(mstmt_sqlite3_t *);
int MSQLite3Fetch(mstmt_sqlite3_t *);
int MSQLite3NumParams(mstmt_sqlite3_t *,int *);
int MSQLite3BindParamInteger(mstmt_sqlite3_t *,int,int *,int);
int MSQLite3InsertCPStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3BindParamBlob(mstmt_sqlite3_t *,int,char *);
int MSQLite3IJobQueryStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3FreeStmt(mstmt_sqlite3_t *);
int MSQLite3SelectGeneralStatsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectEventsMsgsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectGenericMetricsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3SelectGenericMetricsRangeStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3QueryCPStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3Initialize(mdb_t *,char const *);
int MSQLite3BindColDouble(mstmt_sqlite3_t *,int,double *);
int MSQLite3BindColText(mstmt_sqlite3_t *,int,void *,int);
int MSQLite3SelectNodeStatsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3AllocDirectStmt(msqlite3_t *,mstmt_t *,char const *,char *);
int MSQLite3SelectNodeStatsGenericResourcesRangeStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3NumChangedRows(mstmt_sqlite3_t *,int *);
int MSQLite3Connect(msqlite3_t *,char *);
int MSQLite3SelectGeneralStatsRangeStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3NumResultCols(mstmt_sqlite3_t *,int *);
int MSQLite3Free(msqlite3_t *);
int MSQLite3BindColBlob(mstmt_sqlite3_t *,int,void *,int);
int MSQLite3BindParamDouble(mstmt_sqlite3_t *,int,double *,int);
int MSQLite3BindParamText(mstmt_sqlite3_t *,int,char const *,int);
int MSQLite3InsertGeneralStatsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3AllocStmt(msqlite3_t *,mstmt_t *,char const *,char *);
int MSQLite3SelectNodeStatsRangeStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3DeleteCPStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3ExecStmt(mstmt_sqlite3_t *);
int MSQLite3InitAllocatedStmt(msqlite3_t *,mstmt_sqlite3_t *,const char *,char *);
int MSQLite3SetStmt(msqlite3_t *,mstmt_t *,mstmt_sqlite3_t *,const char *,char *);
int MSQLite3BindColInteger(mstmt_sqlite3_t *,int,int *);
int MSQLite3PurgeCPStmt(msqlite3_t *,mstmt_t *);
int MSQLite3StmtError(mstmt_sqlite3_t *,char const *,char *);
int MSQLite3BeginTransaction(msqlite3_t *,char *);
int MSQLite3EndTransaction(msqlite3_t *,char *);
mbool_t MSQLite3InTransaction(msqlite3_t *);
int MSQLite3DeleteJobsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3DeleteRsvsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3DeleteTriggersStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3DeleteVCsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3DeleteRequestsStmt(msqlite3_t *,mstmt_t *,char *);
int MSQLite3DeleteNodesStmt(msqlite3_t *,mstmt_t *,char *);

int MDBBindParamText(mstmt_t *,int,char const *,int);
void MSQLite3BindValue(msqlite3_value_t *,int,int,void const *,int);
int MDBSelectGenericMetricsRangeStmt(mdb_t *,mstmt_t *,char *);
int MDBDeleteCPStmt(mdb_t *,mstmt_t *,char *);
int MDBNumResultCols(mstmt_t *,int *);
int MDBNumChangedRows(mstmt_t *,int *);
int MDBReadNodeStatsRange(mdb_t *,mnust_t *,char *,long unsigned int,long unsigned int,long unsigned int,char *);
int MDBAllocDirectStmt(mdb_t *,mstmt_t *,char const *,char *);
int MDBAllocPrepStmt(mdb_t *,mstmt_t *,const char *,char *);
int MDBSelectNodeStatsStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertNodeStatsStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertEventStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertJobStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertRsvStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertTriggerStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertVCStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertRequestStmt(mdb_t *, mstmt_t *,char *);
int MDBInsertNodeStmt(mdb_t *,mstmt_t *,char *);
int MDBBindNodeStat(mstmt_t *,mnust_t *,char *,char *);
int MDBReadGeneralStats(mdb_t *,must_t *,enum MXMLOTypeEnum,char *,long unsigned int,char *);
int MDBReadGenericMetrics(mdb_t *,double *,enum MXMLOTypeEnum,char *,long unsigned int,char *);
int MDBBindGeneralStat(mstmt_t *,must_t *,char *,char *);
int MDBStmtError(mstmt_t *,char const *,char *);
int MDBInsertGeneralStatsStmt(mdb_t *,mstmt_t *,char *);
int MDBInsertCPStmt(mdb_t *,mstmt_t *,char *);
int MDBPurgeCPStmt(mdb_t *,mstmt_t *);
int MDBReadNodeStatsGenericResources(mdb_t *,char *,long unsigned int,msnl_t *,msnl_t *,msnl_t *,char *);
int MDBBindParamDouble(mstmt_t *,int,double *,int);
int MDBSelectNodeStatsRangeStmt(mdb_t *,mstmt_t *,char *);
int MDBExecStmt(mstmt_t *,enum MStatusCodeEnum *);
int MDBBindGMetricsRead(mstmt_t *,char *,int,double *,long unsigned int *,char *);
int MDBModuleInitialize();
int MDBFree(mdb_t *);
int MDBSelectNodeStatsGenericResourcesStmt(mdb_t *,mstmt_t *,char *);
int MDBSelectNodeStatsGenericResourcesRangeStmt(mdb_t *,mstmt_t *,char *);
int MDBSelectGenericMetricsStmt(mdb_t *,mstmt_t *,char *);
int MDBResetStmt(mstmt_t *);
int MDBPurgeCP(mdb_t *,long unsigned int,char *);
int MDBReadNodeStats(mdb_t *,mnust_t *,char *,long unsigned int,char *);
int MDBBindColInteger(struct mstmt_t *,int,int *);
int MDBSelectGeneralStatsStmt(mdb_t *,mstmt_t *,char *);
int MDBSelectEventsMsgsStmt(mdb_t *,mstmt_t *,char *);
int MDBIJobQueryStmt(mdb_t *,mstmt_t *,char *);
int MDBQueryInternalJobsCP(mdb_t *,int,marray_t *,char *);
int MDBBindParamInteger(mstmt_t *,int,int *,int);
int MDBSelectGeneralStatsRangeStmt(mdb_t *,mstmt_t *,char *);
int MDBQueryCPStmt(mdb_t *,mstmt_t *,char *);
int MDBBindNodeStatGenericResource(mstmt_t *,int *,int *,int *,char *,int,long unsigned int *,char *,char *);
int MDBFetch(mstmt_t *);
int MDBFetchScroll(mstmt_t *);
int MDBReadNodeStatsGenericResourcesRange(mdb_t *,mnust_t **,char *,long unsigned int,long unsigned int,long unsigned int,char *);
int MDBConnect(mdb_t *,char *);
int MDBReadGeneralStatsRange(mdb_t *,must_t *,enum MXMLOTypeEnum,char *,int,int,int,char *);
int MDBFreeStmt(mstmt_t *);
int MDBBindColDouble(struct mstmt_t *,int,double *);
int MDBBindColText(struct mstmt_t *,int,void *,int);
int MDBReadGenericMetricsRange(mdb_t *,void **,enum MXMLOTypeEnum,char *,long unsigned int,long unsigned int,long unsigned int,char *);
int MDBQueryCP(mdb_t *,const char *,const char *,const char *,int,mstring_t *,char *);
int MDBInsertCP(mdb_t *,const char *,const char *,long unsigned int,const char *,char *,enum MStatusCodeEnum *);
int MDBGetEventMessages(mdb_t *,enum MXMLOTypeEnum OType,char *OID,enum MRecordEventTypeEnum EType,int unsigned,char **Array,int const ArrSize);
int MDBBeginTransaction(mdb_t *,char *);
int MDBEndTransaction(mdb_t *,char *,enum MStatusCodeEnum *);
int MDBDeleteJobs(mdb_t *,marray_t *,char *);
int MDBDeleteRequests(mdb_t *, marray_t *,char *);
int MDBDeleteNodes(mdb_t *,marray_t *,char *);
int MDBDeleteJobsStmt(mdb_t *,mstmt_t *,char *);
int MDBDeleteRsvsStmt(mdb_t *,mstmt_t *,char *);
int MDBDeleteTriggersStmt(mdb_t *,mstmt_t *,char *);
int MDBDeleteVCsStmt(mdb_t *,mstmt_t *,char *);
int MDBDeleteRequestsStmt(mdb_t *,mstmt_t *, char *);
int MDBDeleteNodesStmt(mdb_t *,mstmt_t *,char *);
int MDBBindRequest(mstmt_t *,mtransreq_t *,char *,char *);
int MDBBindJob(mstmt_t *,mtransjob_t *,char *,char *);
mbool_t MDBInTransaction(mdb_t *);
int MDBDeleteJobsStmtAddWhereClause(marray_t *);
int MDBDeleteRsvsStmtAddWhereClause(marray_t *);
int MDBDeleteTriggersStmtAddWhereClause(marray_t *);
int MDBDeleteVCsStmtAddWhereClause(marray_t *);
int MDBDeleteRequestsStmtAddWhereClause(marray_t *);
int MDBDeleteNodesStmtAddWhereClause(marray_t *);

int MDBWriteNodeStatsGenericResourcesArray(mdb_t *,char const *,char const *,long unsigned int,msnl_t *,msnl_t *,msnl_t *,int,char *,enum MStatusCodeEnum *);
int MDBWriteGenericMetricsArray(mdb_t *,double *,enum MXMLOTypeEnum,char const *,long unsigned int,char *,enum MStatusCodeEnum *);
int MDBWriteNodeStats(mdb_t *,mnust_t *,char const *,char *,enum MStatusCodeEnum *);
int MDBWriteGeneralStats(mdb_t *,must_t *,char const *,char *,enum MStatusCodeEnum *);
int MDBWriteEvent(struct mdb_t *,enum MXMLOTypeEnum,char *,enum MRecordEventTypeEnum,int, char *,const char *,enum MStatusCodeEnum *);
int MDBWriteObjectsWithRetry(struct mdb_t *,void **,enum MXMLOTypeEnum,char *);

int MDBWriteJobs(struct mdb_t *,mtransjob_t **,char *,enum MStatusCodeEnum *);
int MDBWriteRsv(struct mdb_t *,mtransrsv_t **,char *,enum MStatusCodeEnum *);
int MDBWriteTrigger(struct mdb_t *,mtranstrig_t **,char *,enum MStatusCodeEnum *);
int MDBWriteVC(struct mdb_t *,mtransvc_t **,char *,enum MStatusCodeEnum *);
int MDBWriteRequests(struct mdb_t *,mtransreq_t **,char *,enum MStatusCodeEnum *);
int MDBWriteNodes(struct mdb_t *,mtransnode_t **,char *,enum MStatusCodeEnum *);

mbool_t MDBShouldRetry(enum MStatusCodeEnum);

/* global declarations */

extern msched_t MSched;
#endif	/* _MDBI_H */

