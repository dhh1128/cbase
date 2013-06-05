#include "mdbi.h"
#include "moab-proto.h"
#include "cmoab.h"

#ifndef MDISABLEODBC
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sqlext.h>


const mstmt_odbc_vtbl MPStmt_Vtbl = { *(int (*)(mstmt_odbc_t*))MODBCExecPrepStmt };
const mstmt_odbc_vtbl MDStmt_Vtbl = { *(int (*)(mstmt_odbc_t*))MODBCExecDirectStmt };

#define SQL_VARCHAR_MAX 8000
#define MSQL_STATE_LEN 7


#if 0 /* not supported */
int MODBCSetScrollStmt(

  mstmt_odbc_t *MStmt,
  int           ScrollSize,
  void         *RowsFetched)

  {
  SQLHSTMT Handle = MStmt->Handle;

  SQLSetStmtAttr(Handle,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)ScrollSize,0);

  /* set the address of the buffer to contain the number of rows fetched */
  SQLSetStmtAttr(Handle,SQL_ATTR_ROWS_FETCHED_PTR,RowsFetched,0);

  return(SUCCESS);
  }  /* END MODBCScrollStmtSet() */


/**
 * @see MDBBindParamArrays()
 * @param MStmt   (I)
 * @param ArrSize (I)
 */

int MODBCBindParamArrays(

  mstmt_odbc_t *MStmt,   /* I */
  int           ArrSize) /* I */

  {
  SQLHSTMT Handle = MStmt->Handle;
  SQLSetStmtAttr(Handle, SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, 0);

  /* note - a compiler warning is expected for ArrSize here on 64 bit machines.
            Sean R says that this is a known issue and can be ignored.         */
  SQLSetStmtAttr(Handle, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ArrSize, 0);

  return(SUCCESS);
  } /* END MODBCBindParamArrays */ 
#endif



/**
 * @see MDBPurgeCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 */

int MODBCPurgeCPStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt)   /* I */

  {
  const char *StmtTextPtr = MDBPurgeCPStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->PurgeCPStmt,StmtTextPtr,NULL));
  } /* END MODBCPurgeCPStmt */ 




/**
 * @see MDBStmtError()
 *
 * @param MStmt    (I)
 * @param ExtraMsg (I)
 * @param EMsg     (O)
 * @param SC       (O)
 */

int MODBCStmtError(

  mstmt_odbc_t         *MStmt,    /* I */
  char const           *ExtraMsg, /* I */
  char                 *EMsg,     /* O */
  enum MStatusCodeEnum *SC)       /* O */

  {
  return(MODBCError(MStmt->Handle,SQL_HANDLE_STMT,ExtraMsg,EMsg,SC));
  } /* END MODBCStmtError */ 




/**
 * do some basic verification tests of the ODBC database connection
 * to provide useful feedback to the user if the database was, for instance,
 * not properly installed.
 */

static int MODBCVerifyTableIntegrity(

  modbc_t *MDBInfo,
  char   *EMsg)

  {
  enum MStatusCodeEnum SC;

  static char TablesToCheck[][MMAX_NAME << 1] = {
    "GeneralStats",
    "GenericMetrics",
    "Moab",
    "NodeStats",
    "NodeStatsGenericResources",
    "ObjectType",
    "mcheckpoint",
    "Events",
  };

  mdstmt_odbc_t MStmt;
  char Tables[32][MMAX_NAME << 1];
  int NumTables = 0;
  int rc;
  int index;
  char *MissingTable = NULL;

  memset(&MStmt,0,sizeof(MStmt));
  if (MODBCInitAllocatedDirectStmt(MDBInfo,&MStmt,"",EMsg) == FAILURE)
    return(FAILURE);

  if (!SQL_SUCCEEDED(SQLTables(MStmt.super.Handle,NULL,0,NULL,0,NULL,0,NULL,0)))
    {
    MODBCStmtError(&MStmt.super,"Query for table information failed: ",EMsg,&SC);
    }

  MODBCBindColText(&MStmt.super,3,Tables[NumTables],sizeof(Tables[0]));
  rc = MODBCFetch(&MStmt.super);

  while (rc != MDB_FETCH_NO_DATA && rc != FAILURE)
    {
    NumTables++;
    MODBCBindColText(&MStmt.super,3,Tables[NumTables],sizeof(Tables[0]));
    rc = MODBCFetch(&MStmt.super);
    }

  if (rc != MDB_FETCH_NO_DATA)
    {
    MODBCFreeStmt(&MStmt.super);
    return(FAILURE);
    }

  for (index = 0; index < (int)(sizeof(TablesToCheck) / sizeof(TablesToCheck[0])); index++)
    {
    int innerIndex;
    mbool_t foundTable = FALSE;

    for (innerIndex = 0; innerIndex < NumTables; innerIndex++)
      {

      if (!strcasecmp(TablesToCheck[index],Tables[innerIndex]))
        {
        foundTable = TRUE;
        break;
        }
      }

    if (!foundTable)
      {
      MissingTable = TablesToCheck[index];
      break;
      }
    }

  MODBCFreeStmt(&MStmt.super);

  if (MissingTable != NULL)
    {
    if (EMsg != NULL)
      {
      char subMsg[MMAX_LINE];

      if (NumTables == 0)
        snprintf(subMsg,sizeof(subMsg),"any tables");
      else
        snprintf(subMsg,sizeof(subMsg),"table %s",MissingTable);

      snprintf(EMsg,MMAX_LINE,"Could not find %s in ODBC data source. "
        "Check that the database is installed properly and that Moab has appropriate "
        "read/write access to it.",
        subMsg);
      }
    }

  return(MissingTable == NULL);

  } /* END MODBCVerifyTableIntegrity */




/**
 * @see MDBConnect()
 * @param MDB  (I)
 * @param EMsg (O)
 */

int MODBCConnect(

  modbc_t *MDB,  /* I */
  char    *EMsg) /* O */

  {
  enum MStatusCodeEnum SC;

  long SQLErrorNo;

  SQLINTEGER  Error;
  SQLSMALLINT Length;


  if (EMsg != NULL)
    EMsg[0] = '\0';

#if 1
  /* we should only have to connect once per thread even though this routine
     is called many times. Not sure why we were not checking to see if we
     are already connected. 
     
     Note - remove this check if multiple connects are needed for some reason - DCH. */
 
  if (MDB->ConnectedOnce)
    {
    MODBCUpdateMessage(SUCCESS,EMsg);
    return(SUCCESS);
    }
#endif

  /* allocate Environment handle and register version  */

  if (MDB->SQLEnv == NULL)
    {
    PROP_FAIL(MODBCAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&MDB->SQLEnv,EMsg));
   
    SQLErrorNo = SQLSetEnvAttr(MDB->SQLEnv,SQL_ATTR_ODBC_VERSION,(void*)SQL_OV_ODBC3,0); 

    if ((SQLErrorNo != SQL_SUCCESS) && (SQLErrorNo != SQL_SUCCESS_WITH_INFO))
      {
      if (EMsg != NULL)
        strncpy(EMsg,"Error setting sql environment\n",MMAX_LINE - 1);

      MODBCUpdateMessage(FAILURE,EMsg);
      return(FAILURE);
      }
    }

  /* allocate connection handle, set timeout */

  if (MDB->SQLHandle == NULL)
    {
    PROP_FAIL(MODBCAllocHandle(SQL_HANDLE_DBC,MDB->SQLEnv,&MDB->SQLHandle,EMsg)); 

    SQLSetConnectAttr(MDB->SQLHandle,SQL_LOGIN_TIMEOUT,(SQLPOINTER)5,0);
    }

  /* connect to the datasource */

  mstring_t   MStr(MMAX_LINE);   /* Generate a mstring */

  MODBCBuildConnectionString(MDB,&MStr);
 
  SQLErrorNo = SQLDriverConnect(
    MDB->SQLHandle,
    NULL,
    (SQLCHAR *)MStr.c_str(),
    SQL_NTS,
    NULL,
    0,
    NULL,
    SQL_DRIVER_NOPROMPT);


  if (!SQL_SUCCEEDED(SQLErrorNo))
    {
    SQLCHAR     tmpLine[MMAX_LINE];

    SQLGetDiagRec(SQL_HANDLE_DBC,MDB->SQLHandle,1,MDB->SQLStatus,&Error,tmpLine,sizeof(tmpLine),&Length);

    /* Ensure that the error wasn't caused by an already open connection with 08002 */

    /* SQLite erroneously returns an error code HYC00 when connecting to 
     * an already open connection. This is a problem, because HYC00 could point
     * to legitimate problems in other cases*/

    if (strcmp((char*)MDB->SQLStatus,"08002") &&
        strcmp((char*)MDB->SQLStatus,"HYC00"))
      {
      if (EMsg != NULL)
        MODBCError(MDB->SQLHandle,SQL_HANDLE_DBC,"Error connecting to database",EMsg,&SC);

      MODBCUpdateMessage(FAILURE,EMsg);
      return(FAILURE);
      }
    }

  if (!MDB->ConnectedOnce)
    {
    MDB->ConnectedOnce = TRUE;
    if (MODBCVerifyTableIntegrity(MDB,EMsg) == FAILURE)
      {
      MODBCUpdateMessage(FAILURE,EMsg);
      return(FAILURE);
      }
    }

  MODBCUpdateMessage(SUCCESS,EMsg);

  return(SUCCESS);
  } /* END MODBCConnect */ 




/**
 * @see MODBCUpdateMessage()
 * @param Connected  (I) 
 * @param EMsg (O) 
 */

void MODBCUpdateMessage(

  int     Connected,  /* I */
  char    *EMsg)      /* O */ 

  {
  char fullEMsg[MMAX_LINE << 1];

  if (Connected == SUCCESS)
    {
    /* remove any previous error messages about connection problems
      because we have reconnected */

    MMBRemoveMessage(&MSched.MB,NULL,mmbtDB);
    }
  else
    {
    /* can't connect to DB--fallback to normal CP method */

    snprintf(fullEMsg,
      sizeof(fullEMsg),
      "cannot connect to DB--falling back to file and memory-based storage! (%s)",
      EMsg);
  
    MDB(1,fCONFIG) MLog("WARNING:  cannot connect to DB--falling back to file and memory-based storage! (%s)\n",
      fullEMsg);
  
    if (MMBGetIndex(MSched.MB,NULL,NULL,mmbtDB,NULL,NULL) == FAILURE)
      {
      MMBAdd(
        &MSched.MB,
        fullEMsg,
        NULL,
        mmbtDB,
        0,
        0,
        NULL);
      }
    }
  } /* END MODBCUpdateMessage */ 



/**
 * @see MDBBindParamDouble()
 * @param MStmt   (I)
 * @param Param   (I)
 * @param Val     (I)
 * @param IsArray (I)
 */

int MODBCBindParamDouble(

  mstmt_odbc_t *MStmt,   /* I */
  int           Param,   /* I */
  double       *Val,     /* I */
  int           IsArray) /* I */

  {
  int len = (IsArray) ? sizeof(double) : 0;
  return(SQL_SUCCEEDED(SQLBindParameter(
    MStmt->Handle,
    Param,
    SQL_PARAM_INPUT,
    SQL_C_DOUBLE,
    SQL_FLOAT,
    len,
    0,
    Val,
    len,
    NULL)));
  } /* END MODBCBindParamDouble */ 




/**
 * @see MDBSelectGeneralStatsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectGeneralStatsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGeneralStatsStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectGeneralStatsStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectGeneralStatsStmt */ 




/**
 * @see MDBDeleteJobsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCDeleteJobsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteJobsStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteJobsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCDeleteJobsStmt() */




/**
 * @see MDBDeleteRsvsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCDeleteRsvsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteRsvsStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteRsvsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCDeleteRsvsStmt() */




/**
 * @see MDBDeleteTriggersStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCDeleteTriggersStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteTriggersStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteTriggersStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCDeleteTriggersStmt() */




/**
 * @see MDBDeleteVCsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCDeleteVCsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteVCsStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteVCStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCDeleteVCsStmt() */




/**
 * @see   MDBDeleteRequestsStmt()
 * @param MDBInfo (I) the db info struct
 * @param MStmt   (O) the statement we'll execute
 * @param EMsg    (O) any output error message
 */

int MODBCDeleteRequestsStmt(
  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteRequestsStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteRequestsStmt,
    StmtTextPtr,
    EMsg));
  }




/**
 * @see MDBDeleteNodesStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCDeleteNodesStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteNodesStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteNodesStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCDeleteNodesStmt() */




/**
 * Get statement for Selecting event descriptions
 * @see MDBSelectEventsMsgsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectEventsMsgsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBSelectEventsStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectEventsStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectEventsMsgsStmt */




/**
 * @see MDBSelectGenericMetricsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectGenericMetricsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGenericMetricsStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectGenericMetricsStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectGenericMetricsStmt */ 




/**
 * @see MDBNumResultCols()
 * @param MStmt (I)
 * @param Out   (O)
 */

int MODBCNumResultCols(

  mstmt_odbc_t *MStmt, /* I */
  int          *Out)   /* O */

  {
  SQLSMALLINT Result = 0;
  SQLHSTMT Handle = MStmt->Handle;
  if (!SQL_SUCCEEDED(SQLNumResultCols(Handle,&Result)))
    {
    *Out = -1;
    return(FAILURE);
    }
  *Out = Result;
  return(SUCCESS);
  } /* END MODBCNumResultCols */ 




/**
 * returns a boolean value indicating whether the passed in parameters indicate an 
 * ignorable error. The values of these parameters are retrieved through an 
 * ODBC diagnostic function such as SQLGetDiagRec
 * @param State       (I)
 * @param NativeCode  (I)
 * @param MessageText (I)
 */

int MODBCIsIgnorableError(

  SQLCHAR    *State,       /* I */
  SQLINTEGER  NativeCode,  /* I */
  SQLCHAR    *MessageText) /* I */

  {
  /* TODO Expand this method as necessary */
  /* Checks that the native MySQL error code was "Query was empty" */

  return NativeCode == 1065;
  } /* END MODBCIsIgnorableError */ 




/**
 * @see MDBBindColText()
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MODBCBindColText(

  mstmt_odbc_t *MStmt, /* I */
  int           Col,   /* I */
  void         *Val,   /* I */
  int           Size)  /* I */

  {
  return(SQL_SUCCEEDED(SQLBindCol(
    MStmt->Handle,
    Col,
    SQL_C_CHAR,
    Val,
    Size,
    NULL)));
  } /* END MODBCBindColText */ 




/**
 * @see MDBResetStmt()
 * @param MStmt (I)
 */

int MODBCResetStmt(

  mstmt_odbc_t *MStmt) /* I */

  {
  return(SQL_SUCCEEDED(SQLFreeStmt(MStmt->Handle,SQL_CLOSE)));
  } /* END MODBCResetStmt */ 




/**
 * wrapper function for SQLBindParameter that attempts to report an error message
 * in EMsg on failure
 * @param StatementHandle   (I)
 * @param ParameterNumber   (I)
 * @param InputOutputType   (O)
 * @param ValueType         (I)
 * @param ParameterType     (I)
 * @param ColumnSize        (I)
 * @param DecimalDigits     (I)
 * @param ParameterValuePtr (I)
 * @param BufferLength      (I)
 * @param StrLen_or_IndPtr  (I)
 * @param Statement         (I)
 * @param EMsg              (O)
 */

int MODBCBindParameter(

  SQLHANDLE           StatementHandle,   /* I */
  SQLUSMALLINT        ParameterNumber,   /* I */
  SQLSMALLINT         InputOutputType,   /* O */
  SQLSMALLINT         ValueType,         /* I */
  SQLSMALLINT         ParameterType,     /* I */
  SQLUINTEGER         ColumnSize,        /* I */
  SQLSMALLINT         DecimalDigits,     /* I */
  void *              ParameterValuePtr, /* I */
  SQLINTEGER          BufferLength,      /* I */
  SQLLEN             *StrLen_or_IndPtr,  /* I */
  SQLCHAR            *Statement,         /* I */
  char               *EMsg)              /* O */

  {
  enum MStatusCodeEnum SC;

  if (!SQL_SUCCEEDED(SQLBindParameter(StatementHandle,ParameterNumber,
      InputOutputType,ValueType,ParameterType,ColumnSize,DecimalDigits,
      ParameterValuePtr,BufferLength,StrLen_or_IndPtr)))
    {
    char tmpLine[MMAX_LINE];
    Statement = Statement == NULL ? (unsigned char*)"" : Statement;
    snprintf(tmpLine,sizeof(tmpLine),"Failed to Bind Parameter %d to Statement %s",ParameterNumber,Statement);
    MODBCError(StatementHandle,SQL_HANDLE_STMT,tmpLine,EMsg,&SC);

    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MODBCBindParameter */ 




/**
 * @see MDBQueryCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCQueryCPStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBQueryCPStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->QueryCPStmt,StmtTextPtr,EMsg));
  } /* END MODBCQueryCPStmt */ 




/**
 * @see MDBSelectGenericMetricsRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectGenericMetricsRangeStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGenericMetricsRangeStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectGenericMetricsRangeStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectGenericMetricsRangeStmt */ 




/**
 * @see MDBFree()
 * @param MDBInfo (I)
 */

int MODBCFree(

  modbc_t *MDBInfo) /* I */

  {
  if (MDBInfo == NULL)
    {
    return(FAILURE);
    }

  MODBCFreeStmt(&MDBInfo->InsertCPStmt.super); 
  MDBInfo->InsertCPStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->PurgeCPStmt.super); 
  MDBInfo->PurgeCPStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->QueryCPStmt.super); 
  MDBInfo->QueryCPStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->DeleteCPStmt.super);
  MDBInfo->DeleteCPStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->IJobQueryStmt.super);
  MDBInfo->IJobQueryStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertGeneralStatsStmt.super);
  MDBInfo->InsertGeneralStatsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertNodeStatsStmt.super);
  MDBInfo->InsertNodeStatsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertEventStmt.super);
  MDBInfo->InsertEventStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectEventsStmt.super);
  MDBInfo->SelectEventsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectGeneralStatsStmt.super);
  MDBInfo->SelectGeneralStatsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectGenericMetricsStmt.super);
  MDBInfo->SelectGenericMetricsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectGeneralStatsRangeStmt.super);
  MDBInfo->SelectGeneralStatsRangeStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectGenericMetricsRangeStmt.super); 
  MDBInfo->SelectGenericMetricsRangeStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectGenericResourcesRangeStmt.super); 
  MDBInfo->SelectGenericResourcesRangeStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectNodeStatsStmt.super);
  MDBInfo->SelectNodeStatsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectNodeStatsRangeStmt.super);
  MDBInfo->SelectNodeStatsRangeStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectNodeStatsGenericResourcesStmt.super);
  MDBInfo->SelectNodeStatsGenericResourcesStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectNodeStatsGenericResourcesRangeStmt.super);
  MDBInfo->SelectNodeStatsGenericResourcesRangeStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectJobsStmt.super);
  MDBInfo->SelectJobsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectRsvsStmt.super);
  MDBInfo->SelectRsvsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectTriggersStmt.super);
  MDBInfo->SelectTriggersStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectVCStmt.super);
  MDBInfo->SelectVCStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectRequestsStmt.super);
  MDBInfo->SelectRequestsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->SelectNodesStmt.super);
  MDBInfo->SelectNodesStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->DeleteJobsStmt.super);
  MDBInfo->DeleteJobsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->DeleteRsvsStmt.super);
  MDBInfo->DeleteRsvsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->DeleteTriggersStmt.super);
  MDBInfo->DeleteTriggersStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->DeleteVCStmt.super);
  MDBInfo->DeleteVCStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->DeleteRequestsStmt.super);
  MDBInfo->DeleteRequestsStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->DeleteNodesStmt.super);
  MDBInfo->DeleteNodesStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertJobStmt.super);
  MDBInfo->InsertJobStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertRsvStmt.super);
  MDBInfo->InsertRsvStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertTriggerStmt.super);
  MDBInfo->InsertTriggerStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertVCStmt.super);
  MDBInfo->InsertVCStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertRequestStmt.super);
  MDBInfo->InsertRequestStmt.super.IsInitialized = FALSE;
  MODBCFreeStmt(&MDBInfo->InsertNodeStmt.super);
  MDBInfo->InsertNodeStmt.super.IsInitialized = FALSE;

  SQLDisconnect(MDBInfo->SQLHandle);
  SQLFreeHandle(SQL_HANDLE_DBC,MDBInfo->SQLHandle);
  SQLFreeHandle(SQL_HANDLE_ENV,MDBInfo->SQLEnv);

  return(SUCCESS);
  } /* END MODBCFree */ 




/**
 * internal function that generate the connection string for a call to 
 * SQLDriverConnect. It assumes the existence of a dsn file 
 * $MOABHOMEDIR/dsninfo.dsn that contains all the necessary connection information.
 *
 * @see MODBCConnect()
 * @param MDBInfo (I)
 * @param Out     (O)
 */

int MODBCBuildConnectionString(

  modbc_t   *MDBInfo, /* I */
  mstring_t *Out)     /* O */

  {
  MStringAppendF(Out,"FILEDSN=%s/%s;",MUGetHomeDir(),"dsninfo");

  return(SUCCESS);
  } /* END MODBCBuildConnectionString */ 




/**
 * Wrapper function for SQLBindCol that attempts to report any error messages into
 * EMsg on failure.
 * @param StatementHandle  (I)
 * @param ColumnNumber     (I)
 * @param TargetType       (I)
 * @param TargetValuePtr   (I)
 * @param BufferLength     (I)
 * @param StrLen_or_IndPtr (I)
 * @param Statement        (I)
 * @param EMsg             (O)
 */

int MODBCBindCol(

  SQLHANDLE           StatementHandle,  /* I */
  short unsigned int  ColumnNumber,     /* I */
  short int           TargetType,       /* I */
  void *              TargetValuePtr,   /* I */
  SQLINTEGER          BufferLength,     /* I */
  SQLLEN             *StrLen_or_IndPtr, /* I */
  SQLCHAR            *Statement,        /* I */
  char               *EMsg)             /* O */

  {
  enum MStatusCodeEnum SC;

  if (!SQL_SUCCEEDED(SQLBindCol(
      StatementHandle,
      ColumnNumber,
      TargetType,
      TargetValuePtr,
      BufferLength,
      StrLen_or_IndPtr)))
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"Failed to Bind Column %d to Statement %s",
      ColumnNumber,
      (Statement == NULL) ? "" : (char *)Statement);

    MODBCError(StatementHandle,SQL_HANDLE_STMT,tmpLine,EMsg,&SC);

    return(FAILURE);
    }
  return(SUCCESS);
  } /* END MODBCBindCol */ 




/**
 * @see MDBAllocDirectStmt()
 * @param MDBInfo  (I)
 * @param MStmt    (O)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCAllocDirectStmt(

  modbc_t    *MDBInfo,  /* I */
  mstmt_t    *MStmt,    /* O */
  char const *StmtText, /* I */
  char        *EMsg) /* O */

  {
  int rc;
  MStmt->DBType = mdbODBC;
  MStmt->StmtType = mstmtDirect;

  MStmt->StmtUnion.DODBC = (mdstmt_odbc_t *)MUCalloc(1,sizeof(MStmt->StmtUnion.DODBC[0]));

  if (MStmt->StmtUnion.DODBC == NULL)
    {
    if (EMsg != NULL)
      snprintf(EMsg,sizeof(EMsg),"Memory error allocating ODBC statement");

    return(FAILURE);
    }

  rc = MODBCInitAllocatedDirectStmt(MDBInfo,MStmt->StmtUnion.DODBC,StmtText,EMsg);
  if (rc == FAILURE)
    MUFree((char **)&MStmt->StmtUnion.DODBC);
  return(rc);
  } /* END MODBCInitDirectStmt */ 




/**
 * Constructor to initialize a struct of mdstmt_odbc_t. Before calling this function
 * this first time on an instance of mdstmt_odbc_t, ensure that the instance has
 * been zero'd out. If the instance has already been constructed once,
 * this function does nothing and returns SUCCESS.
 * @param MDBInfo  (I)
 * @param Stmt     (I)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCInitAllocatedDirectStmt(

  modbc_t       *MDBInfo,  /* I */
  mdstmt_odbc_t *Stmt,     /* I */
  char const    *StmtText, /* I */
  char          *EMsg)     /* O */

  {
  if (!Stmt->super.IsInitialized) 
    {
    PROP_FAIL(MODBCConnect(MDBInfo,EMsg) );
    PROP_FAIL(MODBCAllocHandle(SQL_HANDLE_STMT,MDBInfo->SQLHandle,&Stmt->super.Handle,EMsg));
    Stmt->super.DB = MDBInfo;
    Stmt->super.IsInitialized = TRUE;
    Stmt->super.StmtText = (SQLCHAR *)StmtText;
    Stmt->super.vtbl = &MDStmt_Vtbl;
    }
  return(SUCCESS);
  } /* END MODBCInitAllocatedDirectStmt */ 




/**
 * @see MDBFreeStmt()
 * @param MStmt   (I)
 */

int MODBCFreeStmt(

  mstmt_odbc_t *MStmt)   /* I */

  {
  int rc = SQL_SUCCEEDED(SQLFreeHandle(SQL_HANDLE_STMT,MStmt->Handle));
  return(rc);
  } /* END MODBCFreeStmt */ 




/**
 * @see MDBInsertGeneralStatsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertGeneralStatsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertGeneralStatsStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->InsertGeneralStatsStmt,StmtTextPtr,EMsg));
  } /* END MODBCInsertGeneralStatsStmt */ 



/**
 * @see MDBFetchScroll()
 * @param MStmt (I)
 */

int MODBCFetchScroll(

  mstmt_odbc_t *MStmt) /* I */

  {
  int rc = SQLFetchScroll(MStmt->Handle,SQL_FETCH_NEXT,0);

  return( (rc == SQL_NO_DATA) ? MDB_FETCH_NO_DATA : SQL_SUCCEEDED(rc) );
  } /* END MODBCFetch */ 





/**
 * @see MDBFetch()
 * @param MStmt (I)
 */

int MODBCFetch(

  mstmt_odbc_t *MStmt) /* I */

  {
  int rc = SQLFetch(MStmt->Handle);
  return( (rc == SQL_NO_DATA) ? MDB_FETCH_NO_DATA : SQL_SUCCEEDED(rc) );
  } /* END MODBCFetch */ 




/**
 *Attempts to retrieve all error and information messages for the passed in
 *handle. These messages are logged and potentially copied into EMsg;
 *
 * @see MOBCStmtError()
 *
 * @param Handle   (I)
 * @param Type     (I)
 * @param ExtraMsg (I)
 * @param EMsg     (O)
 * @param SC       (O)
 */

int MODBCError(

  void                 *Handle,
  short int             Type,
  char const           *ExtraMsg,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  SQLINTEGER	i = 0;
  SQLINTEGER	native = 0;
  SQLCHAR	state[MSQL_STATE_LEN];
  SQLCHAR      *statePtr = state;
  SQLCHAR	text[MMAX_LINE];
  SQLCHAR      *textPtr = text;
  SQLSMALLINT	len;
  SQLRETURN	ReturnValue;

  mstring_t     MStr(MMAX_LINE);

  mbool_t ErrorFound = FALSE;
  
  ExtraMsg = (ExtraMsg == NULL) ? "" : ExtraMsg;

  MStringAppendF(&MStr,"ERROR:    %s\n",ExtraMsg);

  if (SC != NULL)
    *SC = mscNoError;

  do
    {
    ReturnValue = SQLGetDiagRec(Type,Handle,++i,statePtr,&native,textPtr, sizeof(text),&len);

    if ((native == 2006) || (native == 26))
      {
      if (SC != NULL)
        *SC = mscRemoteFailure;
      }

    if (SQL_SUCCEEDED(ReturnValue))
      {
      if (!MODBCIsIgnorableError(state,native,text))
        {
        ErrorFound = TRUE;
        MStringAppendF(&MStr,"\t%s:%ld:%ld:%s\n",state,i,native,text);
        }
      }
    }
  while (ReturnValue == SQL_SUCCESS);

  if (ErrorFound == TRUE)
    {
    MDB(3,fALL) MLog("Error:    %s\n",MStr.c_str());

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"%s",MStr.c_str());
    }

  return (ErrorFound == TRUE) ? SUCCESS : FAILURE;
  } /* END MODBCError */ 




/**
 * @see MDBBindParamInteger()
 * @param MStmt   (I)
 * @param Param   (I)
 * @param Val     (I)
 * @param IsArray (I)
 */

int MODBCBindParamInteger(

  mstmt_odbc_t *MStmt,   /* I */
  int           Param,   /* I */
  int          *Val,     /* I */
  int           IsArray) /* I */

  {
  int len = (IsArray) ? sizeof(int) : 0;
  return(SQL_SUCCEEDED(SQLBindParameter(
    MStmt->Handle,
    Param,
    SQL_PARAM_INPUT,
    SQL_C_SLONG,
    SQL_INTEGER,
    len,
    0,
    Val,
    len,
    NULL)));
  } /* END MODBCBindParamInteger */ 




/**
 * @see MDBSelectNodeStatsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectNodeStatsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectNodeStatsStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectNodeStatsStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectNodeStatsStmt */ 




/**
 * @see MDBExecStmt()
 * @param Stmt    (I)
 */

int MODBCExecStmt(

  mstmt_odbc_t *Stmt)    /* I */

  {
    return(Stmt->vtbl->ExecFunc(Stmt));
  } /* END MODBCExecStmt */ 




/**
 * @see MDBSelectNodeStatsRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectNodeStatsRangeStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectNodeStatsRangeStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectNodeStatsRangeStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectNodeStatsRangeStmt */ 




/**
 * @see MDBSelectNodeStatsGenericResourcesRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectNodeStatsGenericResourcesRangeStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectNodeStatsGenericResourcesRangeStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectNodeStatsGenericResourcesRangeStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectNodeStatsGenericResourcesRangeStmt */ 




/**
 * @see MDBIJobQueryStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCIJobQueryStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBIJobQueryStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->IJobQueryStmt,StmtTextPtr,EMsg));
  } /* END MODBCIJobQueryStmt */ 




/**
 * @see MDBDeleteCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCDeleteCPStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBDeleteCPStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->DeleteCPStmt,StmtTextPtr,EMsg));
  } /* END MODBCDeleteCPStmt */ 




/**
 * @see MDBBindParamText()
 * @param MStmt (I)
 * @param Param (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MODBCBindParamText(

  mstmt_odbc_t *MStmt, /* I */
  int           Param, /* I */
  char const   *Val,   /* I */
  int           Size)  /* I */

  {
  int ODBCType = (Size > SQL_VARCHAR_MAX) ? SQL_LONGVARCHAR : SQL_VARCHAR;
  return(SQL_SUCCEEDED(SQLBindParameter(
    MStmt->Handle,
    Param,
    SQL_PARAM_INPUT,
    SQL_C_CHAR,
    ODBCType,
    Size-1,
    0,
    (void *)Val,
    Size, 
    NULL)));
  } /* END MODBCBindParamText */ 




/**
 * @see MDBBindColInteger()
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 */

int MODBCBindColInteger(

  mstmt_odbc_t *MStmt, /* I */
  int           Col,   /* I */
  int          *Val)   /* I */

  {
  return(SQL_SUCCEEDED(SQLBindCol(
    MStmt->Handle,
    Col,
    SQL_C_SLONG,
    Val,
    sizeof(Val[0]),
    NULL)));
  } /* END MODBCBindColInteger */ 



/**
 * @see MDBAllocPrepStmt
 *
 */

int MODBCAllocPrepStmt(

  modbc_t       *MDBInfo,  /* I */
  mstmt_t       *MStmt,     /* I */
  const char    *StmtText, /* I */
  char          *EMsg)     /* O */

  {
  int rc;
  MStmt->DBType = mdbODBC;
  MStmt->StmtType = mstmtPrepared;

  MStmt->StmtUnion.PODBC = (mpstmt_odbc_t *)MUCalloc(1,sizeof(MStmt->StmtUnion.DODBC[0]));

  if (MStmt->StmtUnion.PODBC == NULL)
    {
    if (EMsg != NULL)
      snprintf(EMsg,sizeof(EMsg),"Memory error allocating ODBC statement");

    return(FAILURE);
    }

  rc = MODBCInitPrepStmt(MDBInfo,MStmt->StmtUnion.PODBC,StmtText,EMsg);

  if (rc == FAILURE)
    MUFree((char **)&MStmt->StmtUnion.PODBC);

  return(rc);
  }




/**
 * Constructor for mpstmt_odbc_t. Always initialize this struct with this function.
 * Also, ensure the struct is zero'd out before calling it for the first time,
 * If the struct has already been initialized, this function does nothing.
 * @param MDBInfo  (I)
 * @param Stmt     (I)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCInitPrepStmt(

  modbc_t       *MDBInfo,  /* I */
  mpstmt_odbc_t *Stmt,     /* I */
  const char    *StmtText, /* I */
  char          *EMsg)     /* O */

  {
  enum MStatusCodeEnum SC;

  COND_FAIL(MDBInfo == NULL);
  if (!Stmt->super.IsInitialized) 
    {
    PROP_FAIL(MODBCConnect(MDBInfo,EMsg) );
    PROP_FAIL(MODBCAllocHandle(SQL_HANDLE_STMT,MDBInfo->SQLHandle,&Stmt->super.Handle,EMsg));
    if (!SQL_SUCCEEDED(SQLPrepare(Stmt->super.Handle,(SQLCHAR *)StmtText,SQL_NTS)))
      {
      MODBCError(Stmt->super.Handle,SQL_HANDLE_STMT,"Error preparing statement",EMsg,&SC);
      return(FAILURE);
      }
    Stmt->super.DB = MDBInfo;
    Stmt->super.IsInitialized = TRUE;
    Stmt->super.vtbl = &MPStmt_Vtbl;
    Stmt->super.StmtText =(SQLCHAR *)StmtText;
    } /* END if(!Stmt->super.IsInitialized)... */
  
  /* statement is already initialized but we might have changed the constraint list 
     and need to re-prepare it */

  else
    {
    if (!SQL_SUCCEEDED(SQLPrepare(Stmt->super.Handle,(SQLCHAR *)StmtText,SQL_NTS)))
      {
      MODBCError(Stmt->super.Handle,SQL_HANDLE_STMT,"Error preparing statement",EMsg,&SC);
      return(FAILURE);
      }
    Stmt->super.StmtText =(SQLCHAR *)StmtText;
    }

  /* if this is a read only statement and the connection is not already in
     read only mode, then set the read only connection attribute */

  if ((MDBInfo->ReadOnlyRequest == TRUE) &&
      (MDBInfo->ConnectedReadOnly != TRUE))
    {
    int rc;

    MDBInfo->ReadOnlyRequest = FALSE; /* reset */

    rc = SQL_SUCCEEDED(SQLSetConnectAttr(MDBInfo->SQLHandle,
        SQL_ATTR_ACCESS_MODE,
        (SQLPOINTER)SQL_MODE_READ_ONLY,
        SQL_IS_UINTEGER));

    if (rc == FAILURE)
      {
      MODBCError(MDBInfo->SQLHandle,SQL_HANDLE_DBC,NULL,EMsg,&SC);
      }
    else
      {
      MDBInfo->ConnectedReadOnly = TRUE;
      }
    }

  return(SUCCESS);
  } /* END MODBCInitPrepStmt */ 




/**
 * @see MDBSelectGeneralStatsRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectGeneralStatsRangeStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGeneralStatsRangeStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectGeneralStatsRangeStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectGeneralStatsRangeStmt */ 




/**
 * @param MDB (I)
 */

int MODBCInitialize(

  mdb_t *MDB) /* I */

  {
  modbc_t *ODBC;

  if (MDB == NULL)
    {
    return(FAILURE);
    }

  ODBC = (modbc_t *)MUCalloc(1,sizeof(modbc_t));

  if (ODBC == NULL)
    return(FAILURE);

  MDB->DBType = mdbODBC;
  MDB->DBUnion.ODBC = ODBC;

  return(SUCCESS);
  } /* END MODBCInitialize */ 




/**
 * Wrapper function call for SQLAllocHandle. logs an error on failure
 *
 * @return SUCCESS if succeeded, FAILURE if failed
 * @param HandleType      (I)
 * @param InputHandle     (I)
 * @param OutputHandlePtr (O)
 * @param EMsg            (O)
 */

int MODBCAllocHandle(

  short int  HandleType,      /* I */
  void *     InputHandle,     /* I */
  SQLHANDLE *OutputHandlePtr, /* O */
  char      *EMsg)            /* O */

  {
  enum MStatusCodeEnum SC;

  if (!SQL_SUCCEEDED(SQLAllocHandle(HandleType,InputHandle,OutputHandlePtr)))
    {
    MODBCError(InputHandle,HandleType,"Handle Allocation Failed",EMsg,&SC);
    return(FAILURE);
    }
  else return(SUCCESS);
  } /* END MODBCAllocHandle */ 




/**
 * Internal function to assign a prepared statement to MStmt, initializing
 * it first if necessary
 * @param MDBInfo  (I)
 * @param MStmt    (I)
 * @param MOPStmt  (I)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCSetPreparedStmt(

  modbc_t       *MDBInfo,  /* I */
  mstmt_t       *MStmt,    /* I */
  mpstmt_odbc_t *MOPStmt,  /* I */
  const char    *StmtText, /* I */
  char          *EMsg)     /* O */

  {
  if (MODBCInitPrepStmt(MDBInfo,MOPStmt,StmtText,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MStmt->DBType = mdbODBC;
  MStmt->StmtType = mstmtPrepared;
  MStmt->StmtUnion.PODBC = MOPStmt;

  return(SUCCESS);
  } /* END MODBCSetPreparedStmt */ 




/**
 * @see MDBSelectJobsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param StmtTextPtr (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectJobsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  MDBInfo->ReadOnlyRequest = TRUE;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectJobsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCSelectJobsStmt */ 




/**
 * @see MDBSelectRsvsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param StmtTextPtr (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectRsvsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  MDBInfo->ReadOnlyRequest = TRUE;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectRsvsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCSelectRsvsStmt */ 




/**
 * @see MDBSelectTriggersStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param StmtTextPtr (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectTriggersStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  MDBInfo->ReadOnlyRequest = TRUE;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectTriggersStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCSelectTriggersStmt */ 



/**
 * @see MDBSelectVCsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param StmtTextPtr (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectVCsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  MDBInfo->ReadOnlyRequest = TRUE;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectVCStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCSelectVCsStmt */ 




/**
 * @see MDBSelectRequestsStmt()
 * @param MDBInfo (I) the db info struct
 * @param MStmt   (O) the statement we'll execute later
 * @param StmtTextPtr (I)
 * @param EMsg    (O) any output error message
 */

int MODBCSelectRequestsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  MDBInfo->ReadOnlyRequest = TRUE;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectRequestsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCSelectRequestsStmt() */




/**
 * @see MDBSelectNodesStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O) the statement we'll execute later
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectNodesStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectNodesStmtText;

  MDBInfo->ReadOnlyRequest = TRUE;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectNodesStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCSelectNodesStmt */ 




/**
 * @see MDBSelectNodeStatsGenericResourcesStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCSelectNodeStatsGenericResourcesStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectNodeStatsGenericResourcesStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->SelectNodeStatsGenericResourcesStmt,StmtTextPtr,EMsg));
  } /* END MODBCSelectNodeStatsGenericResourcesStmt */ 




/**
 * @see MDBInsertNodeStatsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertNodeStatsStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* I */

  {
  const char *StmtTextPtr = MDBInsertNodeStatsStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->InsertNodeStatsStmt,StmtTextPtr,EMsg));
  } /* END MODBCInsertNodeStatsStmt */ 




/**
 * @see MDBInsertJobStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertJobStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* I */

  {
  const char *StmtTextPtr = MDBInsertJobStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertJobStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCInsertJobStmt */




/**
 * @see MDBInsertRsvStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertRsvStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* I */

  {
  const char *StmtTextPtr = MDBInsertRsvStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertRsvStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCInsertRsvStmt */




/**
 * @see MDBInsertTriggerStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertTriggerStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* I */

  {
  const char *StmtTextPtr = MDBInsertTriggerStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertTriggerStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCInsertTriggerStmt */



/**
 * @see MDBInsertVCStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertVCStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* I */

  {
  const char *StmtTextPtr = MDBInsertVCStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertVCStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCInsertVCStmt */





/**
 * @see MDBInsertReqStmt()
 * @param MDBInfo (I) 
 * @param MStmt   (O) the statement we'll execute later
 * @param EMsg    (O) any output error message
 */
int MODBCInsertRequestStmt(

  modbc_t    *MDBInfo,  /* I */
  mstmt_t    *MStmt,    /* I */
  char       *EMsg)     /* I */

  {
  const char *StmtTextPtr = MDBInsertRequestStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertRequestStmt,
    StmtTextPtr,
    EMsg));
  }




/**
 * @see MDBInsertNodeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertNodeStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertNodeStmtText;

  return(MODBCSetPreparedStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertNodeStmt,
    StmtTextPtr,
    EMsg));
  } /* END MODBCInsertNodeStmt */




/**
 * @see MDBInsertEventStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertEventStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)    /* I */

  {
  const char *StmtTextPtr = MDBInsertEventStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->InsertEventStmt,StmtTextPtr,EMsg));
  } /* END MODBCInsertEventStmt */




/**
 * Executes a prepared statement, acting as a wrapper for SQLExecute
 *
 * @see MDBExecStmt()
 *
 * @param Stmt (I)
 * @param SC   (O)
 */

int MODBCExecPrepStmt(

  mpstmt_odbc_t        *Stmt,
  enum MStatusCodeEnum *SC)

  {
  int rc;

  if (!Stmt->super.IsInitialized) 
    {
    PROP_FAIL(MODBCConnect(Stmt->super.DB,NULL));
    }

  rc = SQL_SUCCEEDED(SQLExecute(Stmt->super.Handle));

  if (!rc)
    {
    MDB(0,fSCHED) MLog("Error:    SQL Statement Execution Error: %d\n",rc);
    return(MODBCError(Stmt->super.Handle,SQL_HANDLE_STMT,"SQL Statement Execution Error",NULL,SC) == FAILURE);
    }

  return(SUCCESS);
  } /* END MODBCExecPrepStmt */ 




/**
 * Wrapper function for SQLExecDirect, where both parameters are retrieved from
 * fields in Stmt.
 * @param Stmt    (I)
 * @param SC      (O)
 */

int MODBCExecDirectStmt(

  mdstmt_odbc_t        *Stmt,
  enum MStatusCodeEnum *SC)

  {
  PROP_FAIL(MODBCConnect(Stmt->super.DB,NULL));
  if (!SQL_SUCCEEDED(SQLExecDirect(Stmt->super.Handle,Stmt->super.StmtText,SQL_NTS)))
    {
    MODBCError(Stmt->super.Handle,SQL_HANDLE_STMT,"SQL Statement Execution Error",NULL,SC);
    return(FAILURE);
    }
  return(SUCCESS);
  } /* END MODBCExecDirectStmt */ 




/**
 * @see MDBBindColDouble()
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 */

int MODBCBindColDouble(

  mstmt_odbc_t *MStmt, /* I */
  int           Col,   /* I */
  double       *Val)   /* I */

  {
  return(SQL_SUCCEEDED(SQLBindCol(
    MStmt->Handle,
    Col,
    SQL_C_DOUBLE,
    Val,
    sizeof(Val[0]),
    NULL)));
  } /* END MODBCBindColDouble */ 




/**
 * @see MDBInsertCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MODBCInsertCPStmt(

  modbc_t *MDBInfo, /* I */
  mstmt_t *MStmt,   /* I */
  char    *EMsg)   /* O */

  {
  const char *StmtTextPtr = MDBInsertCPStmtText;

  return(MODBCSetPreparedStmt(MDBInfo,MStmt,&MDBInfo->InsertCPStmt,StmtTextPtr,EMsg));
  } /* END MODBCInsertCPStmt */ 




/**
 * @see MDBNumChangedRows()
 * @param MStmt (I)
 * @param Out   (O)
 */

int MODBCNumChangedRows(

  mstmt_odbc_t *MStmt, /* I */
  int          *Out)   /* O */

  {
  SQLLEN Result = 0;
  SQLHSTMT Handle = MStmt->Handle;
  if (!SQL_SUCCEEDED(SQLRowCount(Handle,&Result)))
    {
    *Out = -1;
    return(FAILURE);
    }
  *Out = Result;
  return(SUCCESS);
  } /* END MODBCNumChangedRows */ 




/**
 * Not yet implemented or used.
 * @see MDBBindParamBlob()
 * @param MStmt (I)
 * @param Param (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MODBCBindParamBlob(

  mstmt_odbc_t *MStmt, /* I */
  int           Param, /* I */
  void         *Val,   /* I */
  int           Size)  /* I */

  {
  /* NYI */
  return(FAILURE);
  } /* END MODBCBindParamBlob */ 




/**
 * @see MDBInTransaction()
 *
 * @param MDBInfo (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MODBCBeginTransaction(

  modbc_t *MDBInfo,
  char *EMsg)

  {
  enum MStatusCodeEnum SC;

  int rc = SQL_SUCCEEDED(SQLSetConnectAttr(MDBInfo->SQLHandle,
      SQL_ATTR_AUTOCOMMIT,
      SQL_AUTOCOMMIT_OFF,
      SQL_IS_UINTEGER));

  if (rc == FAILURE)
    MODBCError(MDBInfo->SQLHandle,SQL_HANDLE_DBC,NULL,EMsg,&SC);

  return (rc);
  } /* END MODBCBeginTransaction() */




/**
 * @see MDBEndTransaction()
 *
 * @param MDBInfo (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 * @param SC      (O)
 */

int MODBCEndTransaction(

  modbc_t              *MDBInfo,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  int rc;

  /* This call to SQLEndTran shouldn't be necessary, but there appears to be a
   * bug in unixODBC such that the current transaction is not committed as
   * described in the Microsoft ODBC docs at
   * http://msdn.microsoft.com/en-us/library/ms131281.aspx
   * when you switch back to auto-commit
   */
  rc = SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC,MDBInfo->SQLHandle,SQL_COMMIT));

  if (rc == FAILURE)
    {
    MODBCError(MDBInfo->SQLHandle,SQL_HANDLE_DBC,NULL,EMsg,SC);
    return(FAILURE);
    }

  rc = SQL_SUCCEEDED(SQLSetConnectAttr(MDBInfo->SQLHandle,
      SQL_ATTR_AUTOCOMMIT,
      (SQLPOINTER)SQL_AUTOCOMMIT_ON,
      SQL_IS_UINTEGER));

  if (rc == FAILURE)
    {
    MODBCError(MDBInfo->SQLHandle,SQL_HANDLE_DBC,NULL,EMsg,SC);
    return(FAILURE);
    }

  return (SUCCESS);
  } /* END MODBCEndTransaction() */




/**
 * @see MDBInTransaction()
 *
 * @param MDBInfo (I)
 */

mbool_t MODBCInTransaction(

    modbc_t *MDBInfo)

  {
  SQLUINTEGER Value = 0;

  SQLGetConnectAttr(MDBInfo->SQLHandle,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)&Value,SQL_IS_UINTEGER,NULL);

  return (Value == SQL_AUTOCOMMIT_OFF);
  } /* END MODBCInTransaction() */

#else


#if 0 /* not supported */
int MODBCSetScrollStmt(

  mstmt_odbc_t *MStmt,
  int           ScrollSize,
  void         *RowsFetched)

  {
  return(FAILURE);
  }



int MODBCBindParamArrays(

  mstmt_odbc_t *MStmt,
  int           ArrSize)

  {
  return(FAILURE);
  }
#endif




int MODBCPurgeCPStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt)

  {
  return(FAILURE);
  }




int MODBCStmtError(

  mstmt_odbc_t *MStmt,
  const char         *ExtraMsg,
  char         *EMsg,
  enum MStatusCodeEnum *SC)

  {
  return(FAILURE);
  }




int MODBCConnect(

  modbc_t *MDB,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCBindParamDouble(

  mstmt_odbc_t *MStmt,
  int           Param,
  double       *Val,
  int           IsArray)

  {
  return(FAILURE);
  }




int MODBCSelectGeneralStatsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCDeleteJobsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCDeleteRsvsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCDeleteTriggersStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }



int MODBCDeleteVCsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }



int MODBCDeleteRequestsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCDeleteNodesStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCSelectEventsMsgsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCSelectGenericMetricsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCNumResultCols(

  mstmt_odbc_t *MStmt,
  int          *Out)

  {
  return(FAILURE);
  }




int MODBCIsIgnorableError(

  SQLCHAR    *State,
  SQLINTEGER  NativeCode,
  SQLCHAR    *MessageText)

  {
  return(FAILURE);
  }




int MODBCBindColText(

  mstmt_odbc_t *MStmt,
  int           Col,
  void         *Val,
  int           Size)

  {
  return(FAILURE);
  }




int MODBCResetStmt(

  mstmt_odbc_t *MStmt)

  {
  return(FAILURE);
  }




int MODBCBindParameter(

  SQLHSTMT       StatementHandle,
  SQLUSMALLINT   ParameterNumber,
  SQLSMALLINT    InputOutputType,
  SQLSMALLINT    ValueType,
  SQLSMALLINT    ParameterType,
  SQLUINTEGER    ColumnSize,
  SQLSMALLINT    DecimalDigits,
  SQLPOINTER     ParameterValuePtr,
  SQLINTEGER     BufferLength,
  SQLLEN        *StrLen_or_IndPtr,
  unsigned char *Statement,
  char          *EMsg)

  {
  return(FAILURE);
  }




int MODBCQueryCPStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCSelectGenericMetricsRangeStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCFree(

  modbc_t *MDBInfo)

  {
  return(FAILURE);
  }




int MODBCInitAllocatedDirectStmt(

  modbc_t       *MDBInfo,
  mdstmt_odbc_t *Stmt,
  char const    *StmtText,
  char          *EMsg)

  {
  return(FAILURE);
  }




int MODBCBuildConnectionString(

  modbc_t   *MDBInfo,
  mstring_t *Out)

  {
  return(FAILURE);
  }




int MODBCBindCol(

  SQLHSTMT      StatementHandle,
  SQLUSMALLINT  ColumnNumber,
  SQLSMALLINT   TargetType,
  SQLPOINTER    TargetValuePtr,
  SQLINTEGER    BufferLength,
  SQLLEN       *StrLen_or_IndPtr,
  SQLCHAR      *Statement,
  char         *EMsg)

  {
  return(FAILURE);
  }




int MODBCFreeStmt(

  mstmt_odbc_t *MStmt)

  {
  return(FAILURE);
  }




int MODBCInsertGeneralStatsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }



int MODBCFetchScroll(

  mstmt_odbc_t *MStmt)

  {
  return(FAILURE);
  }





int MODBCFetch(

  mstmt_odbc_t *MStmt)

  {
  return(FAILURE);
  }




int MODBCError(

  SQLHANDLE    Handle,
  SQLSMALLINT  Type,
  const char        *ExtraMsg,
  char        *EMsg,
  enum MStatusCodeEnum *SC)

  {
  return(FAILURE);
  }




int MODBCBindParamInteger(

  mstmt_odbc_t *MStmt,
  int           Param,
  int          *Val,
  int           IsArray)

  {
  return(FAILURE);
  }




int MODBCSelectNodeStatsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCExecStmt(

  mstmt_odbc_t *Stmt)

  {
  return(FAILURE);
  }




int MODBCSelectNodeStatsRangeStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCSelectNodeStatsGenericResourcesRangeStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCIJobQueryStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCDeleteCPStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCBindParamText(

  mstmt_odbc_t *MStmt,
  int           Param,
  char const   *Val,
  int           Size)

  {
  return(FAILURE);
  }




int MODBCBindColInteger(

  mstmt_odbc_t *MStmt,
  int           Col,
  int          *Val)

  {
  return(FAILURE);
  }


int MODBCAllocPrepStmt(

  modbc_t       *MDBInfo,
  mstmt_t       *MStmt,
  const char    *StmtText,
  char          *EMsg)

  {
  return(FAILURE);
  }


int MODBCInitPrepStmt(

  modbc_t       *MDBInfo,
  mpstmt_odbc_t *Stmt,
  const char    *StmtText,
  char          *EMsg)

  {
  return(FAILURE);
  }




int MODBCSelectGeneralStatsRangeStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* O */
  char    *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MODBCInitialize(

  mdb_t *MDB)

  {
  const char *Msg = "ERROR:    Cannot handle request to use an ODBC data source. Moab was not build with ODBC support.\n";

  fprintf(stderr,"%s",
    Msg);

  MLog("%s",
    Msg);

  MMBAdd(
    &MSched.MB,
    Msg,
    NULL,
    mmbtOther,
    0,
    0,
    NULL);

  return(FAILURE);
  }




int MODBCAllocHandle(

  SQLSMALLINT  HandleType,
  SQLHANDLE    InputHandle,
  SQLHANDLE   *OutputHandlePtr,
  char        *EMsg)

  {
  return(FAILURE);
  }




int MODBCSetPreparedStmt(

  modbc_t       *MDBInfo,
  mstmt_t       *MStmt,
  mpstmt_odbc_t *MOPStmt,
  const char    *StmtText,
  char          *EMsg)

  {
  return(FAILURE);
  }




int MODBCAllocDirectStmt(

  modbc_t    *MDBInfo,
  mstmt_t    *MStmt,
  char const *StmtText,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MODBCSelectNodeStatsGenericResourcesStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCSelectJobsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCSelectRsvsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCSelectTriggersStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  return(FAILURE);
  }



int MODBCSelectVCsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  return(FAILURE);
  }



int MODBCSelectRequestsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *StmtTextPtr,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCSelectNodesStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCInsertNodeStatsStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCInsertNodeStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCInsertJobStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCInsertRsvStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCInsertTriggerStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }



int MODBCInsertVCStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCInsertRequestStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCInsertEventStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,
  char    *EMsg)

  {
  return(FAILURE);
  }




int MODBCExecPrepStmt(

  mpstmt_odbc_t *Stmt,
  enum MStatusCodeEnum *SC)

  {
  return(FAILURE);
  }




int MODBCExecDirectStmt(

  mdstmt_odbc_t *Stmt,
  enum MStatusCodeEnum *SC)

  {
  return(FAILURE);
  }




int MODBCBindColDouble(

  mstmt_odbc_t *MStmt,
  int           Col,
  double       *Val)

  {
  return(FAILURE);
  }




int MODBCInsertCPStmt(

  modbc_t *MDBInfo,
  mstmt_t *MStmt,   /* I */
  char    *EMsg)   /* O */

  {
  return(FAILURE);
  }




int MODBCNumChangedRows(

  mstmt_odbc_t *MStmt,
  int          *Out)

  {
  return(FAILURE);
  }




int MODBCBindParamBlob(

  mstmt_odbc_t *MStmt,
  int           Param,
  void         *Val,
  int           Size)

  {
  return(FAILURE);
  }




int MODBCBeginTransaction(

  modbc_t *MDBInfo,
  char *EMsg)

  {
  return(FAILURE);
  } /* END MODBCBeginTransaction() */




int MODBCEndTransaction(

  modbc_t *MDBInfo,
  char *EMsg,
  enum MStatusCodeEnum *SC)

  {
  return(FAILURE);
  } /* END MODBCEndTransaction() */




mbool_t MODBCInTransaction(

    modbc_t *MDBInfo)

  {
  return (FALSE);
  } /* END MODBCInTransaction() */


#endif /* MDISABLEODBC */

/* end MODBC.c */
