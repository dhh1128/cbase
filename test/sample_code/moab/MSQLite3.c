#include "mdbi.h"
#include "cmoab.h"
#include "moab-proto.h"
#include <stdio.h>
#include <assert.h>

static int BaseResetStmt(mstmt_sqlite3_t *MStmt);

#ifndef MDISABLESQLITE
#include "sqlite3.h"

/**
 *stores a pointer to an instance of one of the value types
 * recognized by sqlite3: blob(binary data), text, integer, or double.
 *Support for int64 values has not been added yet.
 */

struct msqlite3_value_t {
  void *Value;
  int   Type;
  int   Size; /*optional, not always relevant */
  int   ArrLen;
};

struct mstmt_sqlite3_t {
  sqlite3_stmt             *Stmt;
  sqlite3                  *DB;
  struct msqlite3_value_t  *Parameters;
  struct msqlite3_value_t  *Columns;
  int                       NumCols;
  int                       NumParams;
  int                       ParamArraySize;
  MSQLite3StmtState         State;
  mbool_t                   ParametersBound;
  int                       ParamCurIndex;

};

struct msqlite3_t {
  sqlite3 *DB;
  const char *DBFile;
  mstmt_sqlite3_t InsertCPStmt;
  mstmt_sqlite3_t PurgeCPStmt;
  mstmt_sqlite3_t QueryCPStmt;
  mstmt_sqlite3_t DeleteCPStmt;
  mstmt_sqlite3_t IJobQueryStmt;
  mstmt_sqlite3_t InsertGeneralStatsStmt;
  mstmt_sqlite3_t InsertNodeStatsStmt;
  mstmt_sqlite3_t InsertEventStmt;
  mstmt_sqlite3_t InsertJobStmt;
  mstmt_sqlite3_t InsertRsvStmt;
  mstmt_sqlite3_t InsertTriggerStmt;
  mstmt_sqlite3_t InsertVCStmt;
  mstmt_sqlite3_t InsertRequestStmt;
  mstmt_sqlite3_t InsertNodeStmt;
  mstmt_sqlite3_t SelectEventsStmt;
  mstmt_sqlite3_t SelectGeneralStatsStmt;
  mstmt_sqlite3_t SelectGenericMetricsStmt;
  mstmt_sqlite3_t SelectGeneralStatsRangeStmt;
  mstmt_sqlite3_t SelectGenericMetricsRangeStmt;
  mstmt_sqlite3_t SelectGenericResourcesRangeStmt;
  mstmt_sqlite3_t SelectNodeStatsStmt;
  mstmt_sqlite3_t SelectNodeStatsRangeStmt;
  mstmt_sqlite3_t SelectNodeStatsGenericResourcesStmt;
  mstmt_sqlite3_t SelectNodeStatsGenericResourcesRangeStmt;
  mstmt_sqlite3_t SelectJobsStmt;
  mstmt_sqlite3_t SelectRsvsStmt;
  mstmt_sqlite3_t SelectTriggersStmt;
  mstmt_sqlite3_t SelectVCsStmt;
  mstmt_sqlite3_t SelectRequestsStmt;
  mstmt_sqlite3_t SelectNodesStmt;
  mstmt_sqlite3_t BeginTransactionStmt;
  mstmt_sqlite3_t EndTransactionStmt;
  mstmt_sqlite3_t DeleteJobsStmt;
  mstmt_sqlite3_t DeleteRsvsStmt;
  mstmt_sqlite3_t DeleteTriggersStmt;
  mstmt_sqlite3_t DeleteVCsStmt;
  mstmt_sqlite3_t DeleteNodesStmt;
  mstmt_sqlite3_t DeleteRequestsStmt;
};



/**
 * Internal method to bind a set of parameters for execution. This function reads
 * from and modifies the internal state of MStmt to know which parameters to bind.
 * @param MStmt (I)
 */

static int BindParamsForExecution(

  mstmt_sqlite3_t *MStmt) /* I */

  {
  int index;
  int ArrIndex = MStmt->ParamCurIndex;
  if (ArrIndex >= MStmt->ParamArraySize)
    return(ArrIndex == 0); /*we should succeed if there are no parameters to bind */

  for (index = 1; index <= MStmt->NumParams; index++)
    {
    int rc = SQLITE_OK;
    msqlite3_value_t* Param = &MStmt->Parameters[index-1];
    switch (Param->Type)
      {
      case SQLITE_INTEGER:
        {
        int *val =(int *)Param->Value;
        if (Param->Size > 0)
          val += ArrIndex;

        rc = sqlite3_bind_int(MStmt->Stmt,index,*val);
        break;
        }

      case SQLITE_TEXT:
        {
        char *val = (char *)Param->Value;
        if (Param->Size > 0)
          val += (ArrIndex * Param->Size);

        rc = sqlite3_bind_text(MStmt->Stmt,index,val,-1,SQLITE_TRANSIENT);
        break;
        }

      case SQLITE_FLOAT:
        {
        double *val =(double *)Param->Value;
        if (Param->Size > 0)
          val += ArrIndex;

        rc = sqlite3_bind_double(MStmt->Stmt,index,((double *)Param->Value)[ArrIndex]);
        break;
        }

      case SQLITE_NULL:
        break;
        
      case SQLITE_BLOB:
        {
        char *val = (char *)Param->Value;
        if (Param->Size > 0)
          val += (ArrIndex * Param->Size);

        rc = sqlite3_bind_blob(MStmt->Stmt,index,val,Param->Size,SQLITE_TRANSIENT);
        break;
        }

      default:
        /* undefined behavior. Do we need a mechanism to report a human
         *readable error when this occurs? */
        return(FALSE);

      }
    if (rc != SQLITE_OK)
      {
      MStmt->State = msqlite3Error;
      return(FAILURE);
      }

    }
  MStmt->ParamCurIndex++;
  MStmt->ParametersBound = TRUE;
  return(SUCCESS);
  } /* END BindParamsForExecution */ 




/**
 * internal method to invoke sqlite3_reset and update the state stored in MStmt
 * accordingly
 * @param MStmt (I)
 */

static int BaseResetStmt(

  mstmt_sqlite3_t *MStmt) /* I */

  {
  sqlite3_reset(MStmt->Stmt);
  MStmt->ParametersBound = (MStmt->ParamArraySize == 0);
  MStmt->State = msqlite3Invalid;
  return(SUCCESS);
  } /* END BaseResetStmt */ 




/**
 * Internal method to invoke sqlite3_step and update the state of MStmt 
 * accordingly.
 * @param MStmt (I)
 */

static int MSQLite3Step(

  mstmt_sqlite3_t *MStmt) /* I */

  {
  int rc = sqlite3_step(MStmt->Stmt);

  switch(rc)
    {
    case SQLITE_ROW:
      MStmt->State = msqlite3FetchReady;
      break;

    case SQLITE_DONE:
      MStmt->State = msqlite3FetchDone;
      break;
      
    default:
      MStmt->State = msqlite3Error;
    }
  return rc;
  } /* END MSQLite3Step */ 




/**
 * @see MDBSelectJobsStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectJobsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectJobsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectJobsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3SelectJobsStmt */ 




/**
 * @see MDBSelectJobsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O)
 */

int MSQLite3SelectRequestsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectRequestsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectRequestsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3SelectJobsStmt */ 





/**
 * @see MDBSelectJobsStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectNodesStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectNodesStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectNodesStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3SelectNodesStmt */ 




/**
 * @see MDBSelectRsvsStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectRsvsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectRsvsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectRsvsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3SelectRsvsStmt */ 




/**
 * @see MDBSelectTriggersStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectTriggersStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectTriggersStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectTriggersStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3SelectTriggersStmt */



/**
 * @see MDBSelectVCsStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectVCsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectVCsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectVCsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3SelectVCsStmt */




/**
 * @see MDBSelectNodeStatsGenericResourcesStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectNodeStatsGenericResourcesStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* I */

  {
  char *StmtTextPtr = MDBSelectNodeStatsGenericResourcesStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->SelectNodeStatsGenericResourcesStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3SelectNodeStatsGenericResourcesStmt */ 




/**
 * @see MDBInsertNodeStatsStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertNodeStatsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertNodeStatsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertNodeStatsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3InsertNodeStatsStmt */ 



/**
 * @see MDBInsertJobStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertJobStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertJobStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertJobStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3InsertJobStmt */




/**
 * @see MDBInsertRsvStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertRsvStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertRsvStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertRsvStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3InsertRsvStmt */




/**
 * @see MDBInsertTriggerStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertTriggerStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertTriggerStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertTriggerStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3InsertTriggerStmt */



/**
 * @see MDBInsertVCStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertVCStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertVCStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertVCStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3InsertVCStmt */




/**
 * @see MDBInsertRequestStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertRequestStmt(

    msqlite3_t   *MDBInfo,  /* I */
    mstmt_t      *MStmt,    /* I */
    char         *EMsg)     /* O */
  {
  const char *StmtTextPtr = MDBInsertRequestStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertRequestStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3InsertRequestStmt */




/**
 * @see MDBInsertNodeStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertNodeStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertNodeStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->InsertNodeStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3InsertNodeStmt */




/**
 * @see MDBInsertEventStmt
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertEventStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* I */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertEventStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->InsertEventStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3InsertEventStmt */




/**
 * @see MDBResetStmt()
 * @param MStmt (O)
 */

int MSQLite3ResetStmt(

  mstmt_sqlite3_t *MStmt) /* O */

  {
  BaseResetStmt(MStmt);
  MStmt->ParamCurIndex = 0;

  return(SUCCESS);
  } /* END MSQLite3ResetStmt */ 




/**
 * @see MDBFetch()
 * With sqlite3, data is available after the statement is executed and before
 * the sqlite3_step is called. Therefore, MDBFetch will check if data is available
 * from a previous operation, and if so, bind that data and then called sqlite3_step
 * to make data available for the next call, or to change the state if no more
 * data is available or an error occurs.
 * @param MStmt (I)
 */

int MSQLite3Fetch(

  mstmt_sqlite3_t *MStmt) /* I */

  {
  int index;

  switch(MStmt->State)
    {
    case msqlite3FetchReady:

      /* more to fetch */

      break;

    case msqlite3FetchDone:

      return(MDB_FETCH_NO_DATA);
      break;

    case msqlite3Error:
    case msqlite3Invalid:
    default:
      return(FAILURE);
      break;
    }

  for (index = 0;index < MStmt->NumCols;index++)
    {
    msqlite3_value_t *Col = &MStmt->Columns[index];

    if (Col->Value == NULL)
      continue;

    switch(Col->Type)
      {
      case SQLITE_FLOAT:
        *(double *)Col->Value = sqlite3_column_double(MStmt->Stmt,index);
        break;

      case SQLITE_INTEGER:
        *(int *)Col->Value = sqlite3_column_int(MStmt->Stmt,index);
        break;

      case SQLITE_TEXT:
        {
        char *Text = (char *)sqlite3_column_text(MStmt->Stmt,index);
        MUStrCpy((char *)Col->Value,Text,Col->Size);
        break;
        }

      case SQLITE_BLOB:
        {
        void *Dest = (void *)Col->Value;
        const void *Blob = sqlite3_column_blob(MStmt->Stmt,index);
        int Size = sqlite3_column_bytes(MStmt->Stmt,index);
        memcpy(Dest,Blob,Size);
        break;
        }

        default:
          return(FAILURE);
      }
    }
  MSQLite3Step(MStmt);
  return(SUCCESS);
  } /* END MSQLite3Fetch */ 




/**
 * @see MDBNumParams()
 * @param MStmt (I)
 * @param Out   (O)
 */

int MSQLite3NumParams(

  mstmt_sqlite3_t *MStmt, /* I */
  int             *Out)   /* O */

  {
  *Out = MStmt->NumParams;
  return(SUCCESS);
  } /* END MSQLite3NumParams */ 




/**
 * @see MDBBindParamInteger()
 * @param MStmt   (I)
 * @param Param   (I)
 * @param Val     (I)
 * @param IsArray (I)
 */

int MSQLite3BindParamInteger(

  mstmt_sqlite3_t *MStmt,   /* I */
  int              Param,   /* I */
  int             *Val,     /* I */
  int              IsArray) /* I */

  {
  int size = (IsArray) ? sizeof(int) : 0;
  MSQLite3BindValue(MStmt->Parameters,Param,SQLITE_INTEGER,Val,size);
  return(SUCCESS);
  } /* END MSQLite3BindParamInteger */ 




/**
 * @see MDBInsertCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg   (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertCPStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)   /* O */

  {
  const char *StmtTextPtr = MDBInsertCPStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->InsertCPStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3InsertCPStmt */ 




/**
 * @see MDBBindParamBlob()
 *Currently, this function is unused and untested
 * @param MStmt (I)
 * @param Param (I)
 * @param Val   (I)
 */

int MSQLite3BindParamBlob(

  mstmt_sqlite3_t *MStmt, /* I */
  int              Param, /* I */
  char            *Val)   /* I */

  {
  int rc;
  rc = sqlite3_bind_blob(MStmt->Stmt,Param,Val,-1,SQLITE_TRANSIENT);
  return(SQLITE3_SUCCEEDED(rc));
  } /* END MSQLite3BindParamBlob */ 




/**
 * @see MDBIJobQueryStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3IJobQueryStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)   /* O */

  {
  const char *StmtTextPtr = MDBIJobQueryStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->IJobQueryStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3IJobQueryStmt */ 




/**
 * @see MDBFreeStmt()
 *Frees all memory and database resources stored in MStmt. If MStmt->Stmt == NULL,
 *then this function assumes that MStmt was never initialized and therefore 
 *simply returns FAILURE.
 * @param MStmt   (I)
 */

int MSQLite3FreeStmt(

  mstmt_sqlite3_t *MStmt)   /* I */

  {
  if (MStmt->Stmt == NULL)
    return(FAILURE);
  MUFree((char **)&MStmt->Columns);
  MUFree((char **)&MStmt->Parameters);
  sqlite3_finalize(MStmt->Stmt);
  return(SUCCESS);
  } /* END MSQLite3FreeStmt */ 




/**
 * @see MDBSelectGeneralStatsStmt()
 * Retrieve statement to select a time-based range of statistics
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectGeneralStatsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGeneralStatsStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectGeneralStatsStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectGeneralStatsStmt */ 




/**
 * @see MDBSelectEventsMsgsStmt()
 * Retrieve statement to select a time-based range of statistics
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectEventsMsgsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBSelectEventsStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectEventsStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectEventsMsgsStmt */




/**
 * @see MDBSelectGenericMetricsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectGenericMetricsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGenericMetricsStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectGenericMetricsStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectGenericMetricsStmt */ 




/**
 * @see MDBSelectGenericMetricsRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectGenericMetricsRangeStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGenericMetricsRangeStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectGenericMetricsRangeStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectGenericMetricsRangeStmt */ 




/**
 * @see MDBQueryCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3QueryCPStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBQueryCPStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->QueryCPStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3QueryCPStmt */ 




/**
 * @see MDBInitialize()
 *initialize a heap-allocated instance of msqlite3_t and assign it to MDB
 * @param MDB    (O)
 * @param DBFile (I)
 */

int MSQLite3Initialize(

  mdb_t      *MDB,    /* O */
  char const *DBFile) /* I */

  {
  msqlite3_t *SQLite3;

  if (MDB == NULL)
    {
    return(FAILURE);
    }

  SQLite3 = (msqlite3_t *)MUCalloc(1,sizeof(msqlite3_t));

  if (SQLite3 == NULL)
    return(FAILURE);

  MDB->DBType = mdbSQLite3;
  MDB->DBUnion.SQLite3 = SQLite3;
  SQLite3->DB = NULL;

  if (DBFile == NULL)
    {
    char sqlite3File[MMAX_LINE];
    snprintf(sqlite3File,MMAX_LINE,"%s/%s",MUGetHomeDir(),"moab.db");
    MUStrDup((char **)&SQLite3->DBFile,sqlite3File);
    }
  else
    {
    MUStrDup((char **)&SQLite3->DBFile,(char *)DBFile);
    }



  return(SUCCESS);
  } /* END MSQLite3Initialize */ 




/**
 * @see MDBBindColDouble()
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 */

int MSQLite3BindColDouble(

  mstmt_sqlite3_t *MStmt, /* I */
  int              Col,   /* I */
  double          *Val)   /* I */

  {
    MSQLite3BindValue(MStmt->Columns,Col,SQLITE_FLOAT,Val,sizeof(Val[0]));
    return(SUCCESS);
  } /* END MSQLite3BindColDouble */ 




/**
 * @see MDBBindColText()
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MSQLite3BindColText(

  mstmt_sqlite3_t *MStmt, /* I */
  int              Col,   /* I */
  void            *Val,   /* I */
  int              Size)  /* I */

  {
    MSQLite3BindValue(MStmt->Columns,Col,SQLITE_TEXT,Val,Size);
    return(SUCCESS);
  } /* END MSQLite3BindColText */ 




/**
 * @see MDBSelectNodeStatsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectNodeStatsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectNodeStatsStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectNodeStatsStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectNodeStatsStmt */ 




#if 0 /* not supported */
/**
 * @see MDBBindParamArrays()
 * @param MStmt   (I)
 * @param ArrSize (I)
 */

int MSQLite3BindParamArrays(

  mstmt_sqlite3_t *MStmt,   /* I */
  int              ArrSize) /* I */

  {
  MStmt->ParamArraySize = ArrSize;
  return(SUCCESS);
  } /* END MSQLite3BindParamArrays */ 
#endif



/**
 * @see MDBInitDirectStmt()
 * for sqlite3, direct statements and prepared statements are currently the 
 * same thing.
 * @param MDBInfo  (I)
 * @param MStmt    (I)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3AllocDirectStmt(

  msqlite3_t *MDBInfo,  /* I */
  mstmt_t    *MStmt,    /* I */
  char const *StmtText, /* I */
  char       *EMsg)     /* O */

  {
  return(MSQLite3AllocStmt(MDBInfo,MStmt,StmtText,EMsg));
  } /* END MSQLite3InitDirectStmt */ 




/**
 * @see MDBSelectNodeStatsGenericResourcesRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectNodeStatsGenericResourcesRangeStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectNodeStatsGenericResourcesRangeStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectNodeStatsGenericResourcesRangeStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectNodeStatsGenericResourcesRangeStmt */ 




/**
 * @see MDBNumChangedRows()
 * @param MStmt (I)
 * @param Out   (O)
 */

int MSQLite3NumChangedRows(

  mstmt_sqlite3_t *MStmt, /* I */
  int             *Out)   /* O */

  {
  *Out = sqlite3_changes(MStmt->DB);
  return(SUCCESS);
  } /* END MSQLite3NumChangedRows */ 




/**
 * logs an error and optionally copies it into EMsg
 *
 * @param MDBInfo (I) database handle where error occured
 * @param ErrLevel (I) log message error level
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

void MSQLite3Error(

  msqlite3_t *MDBInfo,
  int ErrLevel,
  char *EMsg)

  {
  char const *Msg = sqlite3_errmsg(MDBInfo->DB);
  MDB(ErrLevel,fALL) MLog("Error:    %s\n",Msg);

  if (EMsg)
    strncpy(EMsg,Msg,MMAX_LINE);

  } /* END MSQlite3Error */




/**
 * do some basic verification tests of the sqlite3 database connection
 * to provide useful feedback to the user if a database file is empty or 
 * blatantly corrupt
 *
 * @param MDBInfo (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

static int MSQLite3VerifyTableIntegrity(

  msqlite3_t *MDBInfo,
  char *EMsg)

  {
  static char const * const TableQuery = "SELECT name FROM sqlite_master WHERE type='table' UNION ALL SELECT name FROM sqlite_temp_master WHERE type='table' ORDER BY name ASC;";
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

  mstmt_sqlite3_t MStmt;
  char Tables[32][MMAX_NAME << 1];
  int NumTables = 0;
  int rc;
  int index;
  char *MissingTable = NULL;

  memset(&MStmt,0,sizeof(MStmt));
  if (MSQLite3InitAllocatedStmt(MDBInfo,&MStmt,(char *)TableQuery,EMsg) == FAILURE)
    return(FAILURE);

  while ((rc = MSQLite3Step(&MStmt)) == SQLITE_ROW)
    {
    strncpy(Tables[NumTables],
      (char const *)sqlite3_column_text(MStmt.Stmt,0),
      sizeof(Tables[0]));
    NumTables++;
    }

  if (rc != SQLITE_DONE)
    {
    MSQLite3FreeStmt(&MStmt);
    return(FAILURE);
    }

  for (index = 0; index < (int)(sizeof(TablesToCheck) / sizeof(TablesToCheck[0])); index++)
    {
    int innerIndex;
    mbool_t foundTable = FALSE;

    for (innerIndex = 0; innerIndex < NumTables; innerIndex++)
      {

      if (!strcmp(TablesToCheck[index],Tables[innerIndex]))
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

  MSQLite3FreeStmt(&MStmt);

  if (MissingTable != NULL)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"SQLite3 Database file %s is missing table %s. "
        "The database may need to be reinstalled.",
        MDBInfo->DBFile,
        MissingTable);
      }

    }

  return(MissingTable == NULL);
  } /*END MSQLite3VerifyTableIntegrity */



/**
 * @see MDBConnect()
 *This function allocates a sqlite3 database handle. It assumes that if
 *MDBInfo->DB != NULL, then such a handle has already been allocated, and it
 *then merely returns SUCCESS. Otherwise, it returns the success of the
 *call to connect to the database.
 * @param MDBInfo (I)
 * @param EMsg    (O)
 */

int MSQLite3Connect(

  msqlite3_t *MDBInfo, /* I */
  char       *EMsg)    /* O */

  {
  char fullEMsg[MMAX_LINE << 1];

  if (MDBInfo->DB != NULL)
    {
    return(SUCCESS);
    }
  else
    {
    if (access(MDBInfo->DBFile,F_OK) != 0)
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"SQLite3 database file %s does not exist",MDBInfo->DBFile);
      return(FAILURE);
      }
    else if (access(MDBInfo->DBFile,W_OK | R_OK) != 0)
      {
      if (EMsg != NULL)
        snprintf(EMsg,MMAX_LINE,"Read/Write access to file %s required",MDBInfo->DBFile);
      return(FAILURE);
      }
    else if (sqlite3_open_v2(
      MDBInfo->DBFile,
      &MDBInfo->DB,
      SQLITE_OPEN_READWRITE,
      NULL) != SQLITE_OK)
      {
      MSQLite3Error(MDBInfo,1,EMsg);
      sqlite3_close(MDBInfo->DB);
      MDBInfo->DBFile = NULL;
      return(FAILURE);
      }
    else if (MSQLite3VerifyTableIntegrity(MDBInfo,EMsg) == FAILURE)
      {
      snprintf(fullEMsg,
        sizeof(fullEMsg),
        "cannot connect to DB--falling back to file and memory-based storage! (%s)",
        EMsg);
    
      MDB(1,fCONFIG) MLog("WARNING:  cannot connect to DB--falling back to file and memory-based storage! (%s)\n",
        fullEMsg);
    
      MMBAdd(
        &MSched.MB,
        fullEMsg,
        NULL,
        mmbtDB,
        0,
        0,
        NULL);

      return(FAILURE);
      }
    }
  return(SUCCESS);
  } /* END MSQLite3Connect */ 




/**
 * @see MDBSelectGeneralStatsRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectGeneralStatsRangeStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectGeneralStatsRangeStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectGeneralStatsRangeStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectGeneralStatsRangeStmt */ 




/**
 * @see MDBNumResultCols()
 * @param MStmt (I)
 * @param Out   (O)
 */

int MSQLite3NumResultCols(

  mstmt_sqlite3_t *MStmt, /* I */
  int             *Out)   /* O */

  {
  *Out = MStmt->NumCols;
  return(SUCCESS);
  } /* END MSQLite3NumResultCols */ 




/**
 * @see MDBFree()
 *This function will fail with SQLITE_BUSY if at least one statement has not
 *been freed through  a call to sqlite3_finalize.
 *It first attempts to free all the statement handles that were allocated and 
 *stored in MDBInfo, and then attempts to free MDBInfo->DB itself.
 * @param MDBInfo (I)
 */

int MSQLite3Free(

  msqlite3_t *MDBInfo) /* I */

  {
  int rc;

  MUFree((char **)&MDBInfo->DBFile);

  MSQLite3FreeStmt(&MDBInfo->InsertCPStmt); 
  MSQLite3FreeStmt(&MDBInfo->PurgeCPStmt); 
  MSQLite3FreeStmt(&MDBInfo->QueryCPStmt); 
  MSQLite3FreeStmt(&MDBInfo->DeleteCPStmt);
  MSQLite3FreeStmt(&MDBInfo->IJobQueryStmt);
  MSQLite3FreeStmt(&MDBInfo->InsertGeneralStatsStmt);
  MSQLite3FreeStmt(&MDBInfo->InsertNodeStatsStmt);
  MSQLite3FreeStmt(&MDBInfo->InsertEventStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectEventsStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectGeneralStatsStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectGenericMetricsStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectGeneralStatsRangeStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectGenericMetricsRangeStmt); 
  MSQLite3FreeStmt(&MDBInfo->SelectGenericResourcesRangeStmt); 
  MSQLite3FreeStmt(&MDBInfo->SelectNodeStatsStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectNodeStatsRangeStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectNodeStatsGenericResourcesStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectNodeStatsGenericResourcesRangeStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectJobsStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectRequestsStmt);
  MSQLite3FreeStmt(&MDBInfo->SelectNodesStmt);
  MSQLite3FreeStmt(&MDBInfo->BeginTransactionStmt);
  MSQLite3FreeStmt(&MDBInfo->EndTransactionStmt);

  rc = sqlite3_close(MDBInfo->DB);

  if (rc == SQLITE_OK)
    {
    return(SUCCESS);
    }
  else
    {
    MDB(3,fALL) MLog("Error:    Failed to free SQLite3 Handle (rc = %d)\n",rc);
    return(FAILURE);
    }
  } /* END MSQLite3Free */ 




/**
 *Currently, this function is unused and untested
 * @see MDBBindColBlob()
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MSQLite3BindColBlob(

  mstmt_sqlite3_t *MStmt, /* I */
  int              Col,   /* I */
  void            *Val,   /* I */
  int              Size)  /* I */

  {
    MSQLite3BindValue(MStmt->Columns,Col,SQLITE_BLOB,Val,Size);
    MStmt->Columns[Col-1].Size = Size;
    return(SUCCESS);
  } /* END MSQLite3BindColBlob */ 



#if 0 /* not supported */
int MSQLite3SetScrollStmt(

  mstmt_sqlite3_t *MStmt,
  int              ScrollSize,
  void            *RowsFetched)

  {
  int rc;

  SQLHSTMT Handle = MStmt->Handle;

  rc = SQLSetStmtAttr(handle,SQL_ATTR_ROW_ARRAY_SIZE,(SQLPOINTER)ScrollSize,0);

  /* set the address of the buffer to contain the number of rows fetched */
  rc = SQLSetStmtAttr(MStmt,SQL_ATTR_ROWS_FETCHED_PTR,RowsFetched,0);

  return(FAILURE);
  }  /* END MODBCScrollStmtSet() */
#endif






/**
 * @see MDBBindParamDouble()
 * @param MStmt   (I)
 * @param Param   (I)
 * @param Val     (I)
 * @param IsArray (I)
 */

int MSQLite3BindParamDouble(

  mstmt_sqlite3_t *MStmt,   /* I */
  int              Param,   /* I */
  double          *Val,     /* I */
  int              IsArray) /* I */

  {
  int size = (IsArray) ? sizeof(double) : 0;

  MSQLite3BindValue(MStmt->Parameters,Param,SQLITE_FLOAT,Val,size);
  return(SUCCESS);
  } /* END MSQLite3BindParamDouble */ 




/**
 * @see MDBBindParamText()
 * @param MStmt (I)
 * @param Param (I)
 * @param Val   (I)
 * @param Size  (I)
 */

int MSQLite3BindParamText(

  mstmt_sqlite3_t *MStmt, /* I */
  int              Param, /* I */
  char const      *Val,   /* I */
  int              Size)  /* I */

  {
  MSQLite3BindValue(MStmt->Parameters,Param,SQLITE_TEXT,Val,Size);
  return(SUCCESS);
  } /* END MSQLite3BindParamText */ 




/**
 * @see MDBInsertGeneralStatsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (O)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InsertGeneralStatsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBInsertGeneralStatsStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->InsertGeneralStatsStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3InsertGeneralStatsStmt */ 




/**
 * heap-allocates a statement and calls MSQLite3InitAllocatedStmt on the 
 * @see MSQLite3InitAllocatedStmt()
 * @param MDBInfo  (I)
 * @param MStmt    (I)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3AllocStmt(

  msqlite3_t *MDBInfo,  /* I */
  mstmt_t    *MStmt,    /* I */
  char const *StmtText, /* I */
  char       *EMsg) /* O */

  {
  int rc;
  mstmt_sqlite3_t *Result = (mstmt_sqlite3_t *)MUCalloc(1,sizeof(mstmt_sqlite3_t));
  if (Result == NULL)
    {
    if (EMsg != NULL)
      snprintf(EMsg,sizeof(EMsg),"Memory error allocating SQLite3 statement");

    return(FAILURE);
    }

  COND_FAIL(Result == NULL);
  rc = MSQLite3InitAllocatedStmt(MDBInfo,Result,StmtText,EMsg);

  MStmt->DBType = mdbSQLite3;
  MStmt->StmtUnion.SQLite3 = Result;
  return(rc);

  } /* END MSQLite3InitStmt */ 




/**
 * @see MDBSelectNodeStatsRangeStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3SelectNodeStatsRangeStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBSelectNodeStatsRangeStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->SelectNodeStatsRangeStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3SelectNodeStatsRangeStmt */ 




/**
 * @see MDBDeleteJobsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3DeleteJobsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteJobsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteJobsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3DeleteJobsStmt() */ 




/**
 * @see MDBDeleteRsvsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3DeleteRsvsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteRsvsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteRsvsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3DeleteRsvsStmt() */ 




/**
 * @see MDBDeleteTriggersStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3DeleteTriggersStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteTriggersStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteTriggersStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3DeleteTriggersStmt() */ 




/**
 * @see MDBDeleteVCsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3DeleteVCsStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteVCsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteVCsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3DeleteVCsStmt() */ 





/**
 * @see MDBDeleteRequestsStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3DeleteRequestsStmt(

  msqlite3_t *MDBInfo,  /* I */
  mstmt_t    *MStmt,    /* O */
  char       *EMsg)

  {
  char *StmtTextPtr = MDBDeleteRequestsStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteRequestsStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3DeleteRequestsStmt() */




/**
 * @see MDBDeleteNodesStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3DeleteNodesStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  char *StmtTextPtr = MDBDeleteNodesStmtText;

  return(MSQLite3SetStmt(
    MDBInfo,
    MStmt,
    &MDBInfo->DeleteNodesStmt,
    StmtTextPtr,
    EMsg));
  } /* END MSQLite3DeleteNodesStmt() */ 





/**
 * @see MDBDeleteCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3DeleteCPStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  const char *StmtTextPtr = MDBDeleteCPStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->DeleteCPStmt,StmtTextPtr,EMsg));
  } /* END MSQLite3DeleteCPStmt */ 




/**
 * @see MDBExecStmt()
 * @param MStmt (I)
 */

int MSQLite3ExecStmt(

  mstmt_sqlite3_t *MStmt) /* I */

  {
  int rc;

  if (MStmt->ParametersBound == FALSE)
    {
    if (BindParamsForExecution(MStmt) == FAILURE)
      {
      return(FAILURE);
      }
    }

  rc = MSQLite3Step(MStmt);

  while ((rc == SQLITE_DONE) && (MStmt->ParamCurIndex < MStmt->ParamArraySize))
    {
    BaseResetStmt(MStmt);

    if (BindParamsForExecution(MStmt) == FAILURE)
      {
      return(FAILURE);
      }

    rc = MSQLite3Step(MStmt);
    }

  return((rc == SQLITE_DONE) || (rc == SQLITE_ROW));
  } /* END MSQLite3ExecStmt() */ 




/**
 * internal implementation of the MSQLite3Bind* functions
 * @see MSQLite3BindColText(), MSQLite3BindColDouble(), MSQLite3BindColInteger()
 * @see MSQLite3BindParamText(), MSQLite3BindParamDouble(), MSQLite3BindParamInteger()
 * @param Arr   (I)
 * @param Index (I)
 * @param Type  (I)
 * @param Val   (I)
 * @param Size  (I)
 */

void MSQLite3BindValue(

  msqlite3_value_t *Arr,   /* I */
  int             Index, /* I */
  int             Type,  /* I */
  void const     *Val,   /* I */
  int             Size)  /* I */

  {
  Arr[Index-1].Type =  Type;
  Arr[Index-1].Value =  (void *)Val;
  Arr[Index-1].Size = Size;
  } /* END MSQLite3BindValue */ 




/**
 * prepares the underlying sqlite3 statement stored in MStmt and allocates 
 * data to store parameters and column addresses on the statement
 * Before calling this function on a mstmt_sqlite3_t struct, ensure the struct 
 * has been zero-initialized, or the results of this function will be undefined!
 * @param MDBInfo  (I)
 * @param MStmt    (I)
 * @param StmtText (I)
 * @param EMsg     (O) optional,minsize=MMAX_LINE
 */

int MSQLite3InitAllocatedStmt(

  msqlite3_t      *MDBInfo,  /* I */
  mstmt_sqlite3_t *MStmt,    /* I */
  char const      *StmtText, /* I */
  char            *EMsg)     /* O */

  {
  const char *Tail;

  if (MStmt->Stmt != NULL)
    return(SUCCESS);

  MStmt->DB = MDBInfo->DB;
  if (sqlite3_prepare_v2(MStmt->DB,StmtText,-1,&MStmt->Stmt,&Tail) != SQLITE_OK)
    {
    MSQLite3Error(MDBInfo,1,EMsg);
    sqlite3_finalize(MStmt->Stmt);
    MStmt->Stmt = NULL;
    return(FAILURE);
    }

  MStmt->NumParams = sqlite3_bind_parameter_count(MStmt->Stmt);
  MStmt->NumCols = sqlite3_column_count(MStmt->Stmt);

  if (MStmt->NumParams > 0)
    MStmt->Parameters = (msqlite3_value_t *)MUCalloc(MStmt->NumParams,sizeof(msqlite3_value_t));

  if (MStmt->NumCols > 0)
    MStmt->Columns = (msqlite3_value_t *)MUCalloc(MStmt->NumCols,sizeof(msqlite3_value_t));

  if (((MStmt->NumCols > 0) && (MStmt->Columns == NULL)) || 
      ((MStmt->NumParams > 0) && (MStmt->Parameters == NULL)))
    {
    MSQLite3FreeStmt(MStmt);
    MUFree((char **)&MStmt);

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"Memory Allocation error constructing statement %s",StmtText);

    return(FAILURE);
    }
  MStmt->ParamArraySize = 1;

  return(SUCCESS);
  } /* END MSQLite3InitAllocatedStmt */ 



/**
 *@see MDBBeginTransaction()
 *
 * @param MDBInfo (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3BeginTransaction(

  msqlite3_t *MDBInfo,
    char *EMsg)

  {
  int rc;

  PROP_FAIL(MSQLite3InitAllocatedStmt(MDBInfo,
      &MDBInfo->BeginTransactionStmt,
      "BEGIN TRANSACTION",
      EMsg));

  rc = MSQLite3ExecStmt(&MDBInfo->BeginTransactionStmt);

  if (rc == FAILURE)
    MSQLite3Error(MDBInfo,1,EMsg);

  MSQLite3ResetStmt(&MDBInfo->BeginTransactionStmt);

  assert (!sqlite3_get_autocommit(MDBInfo->DB));

  return (rc);
  } /* END MSQLite3BeginTransaction() */




/**
 *
 * @see MDBEndTransaction()
 *
 * @param MDBInfo (I)
 * @param EMsg    (O) optional,minsize=MMAX_LINE
 */

int MSQLite3EndTransaction(

  msqlite3_t *MDBInfo,
  char *EMsg)

  {
  int rc;

  PROP_FAIL(MSQLite3InitAllocatedStmt(MDBInfo,
      &MDBInfo->EndTransactionStmt,
      "END TRANSACTION",
      EMsg));

  rc = MSQLite3ExecStmt(&MDBInfo->EndTransactionStmt);

  if (rc == FAILURE)
    MSQLite3Error(MDBInfo,1,EMsg);
  else
    assert (sqlite3_get_autocommit(MDBInfo->DB));

  MSQLite3ResetStmt(&MDBInfo->EndTransactionStmt);

  return (rc);
  } /* END MSQLite3EndTransaction() */



/**
 * @see MDBInTransaction()
 *
 * @param MDBInfo (I)
 */

mbool_t MSQLite3InTransaction(

  msqlite3_t *MDBInfo)

  {
  if ((MDBInfo == NULL) || (MDBInfo->DB == NULL)) return (FALSE);

  return(sqlite3_get_autocommit(MDBInfo->DB) == 0);
  } /* END MSQLite3InTransaction() */




/**
 *This functions is for internal use only. It is used to store
 *MSStmt into MStmt, initializing MSStmt first if necessary.
 * @param MDBInfo  (I)
 * @param MStmt    (I)
 * @param MSStmt   (I)
 * @param StmtText (I)
 * @param EMsg (I)
 */

int MSQLite3SetStmt(

  msqlite3_t      *MDBInfo,  /* I */
  mstmt_t         *MStmt,    /* I */
  mstmt_sqlite3_t *MSStmt,   /* I */
  const char      *StmtText, /* I */
  char            *EMsg) /* I */

  {
  PROP_FAIL(MSQLite3InitAllocatedStmt(MDBInfo,MSStmt,StmtText,EMsg));
  MStmt->DBType = mdbSQLite3;
  MStmt->StmtUnion.SQLite3 = MSStmt;
  return(SUCCESS);

  } /* END MSQLite3SetStmt */ 




/**
 * @see MDBBindColInteger()
 * @param MStmt (I)
 * @param Col   (I)
 * @param Val   (I)
 */

int MSQLite3BindColInteger(

  mstmt_sqlite3_t *MStmt, /* I */
  int              Col,   /* I */
  int             *Val)   /* I */

  {
    MSQLite3BindValue(MStmt->Columns,Col,SQLITE_INTEGER,Val,sizeof(Val[0]));
    return(SUCCESS);
  } /* END MSQLite3BindColInteger */ 




/**
 * @see MDBPurgeCPStmt()
 * @param MDBInfo (I)
 * @param MStmt   (I)
 */

int MSQLite3PurgeCPStmt(

  msqlite3_t *MDBInfo, /* I */
  mstmt_t    *MStmt)   /* I */

  {
  const char *StmtTextPtr = MDBPurgeCPStmtText;

  return(MSQLite3SetStmt(MDBInfo,MStmt,&MDBInfo->PurgeCPStmt,StmtTextPtr,NULL));
  } /* END MSQLite3PurgeCPStmt */ 




/**
 * @see MDBStmtError()
 * @param MStmt    (I)
 * @param ExtraMsg (I)
 * @param EMsg     (O)
 */

int MSQLite3StmtError(

  mstmt_sqlite3_t *MStmt,    /* I */
  char const      *ExtraMsg, /* I */
  char            *EMsg)     /* O */

  {
  const char *SQLite3Msg;

  if ((MStmt->State != msqlite3Error) && (MStmt->State != msqlite3Invalid))
    return(FAILURE);

  SQLite3Msg = sqlite3_errmsg(MStmt->DB);

  MDB(3,fALL) MLog("Error:    %s\n",SQLite3Msg);

  if (EMsg != NULL)
    snprintf(EMsg,MMAX_LINE,"%s%s",ExtraMsg,SQLite3Msg);

  return(SUCCESS);
  } /* END MSQLite3StmtError */ 

#else




static int BindParamsForExecution(

  mstmt_sqlite3_t *MStmt)

  {
  return(FAILURE);
  }




static int BaseResetStmt(

  mstmt_sqlite3_t *MStmt)

  {
  return(FAILURE);
  }




static int MSQLite3Step(

  mstmt_sqlite3_t *MStmt)

  {
  return(FAILURE);
  }




int MSQLite3SelectJobsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3SelectRequestsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3SelectNodesStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3SelectRsvsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3SelectTriggersStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }



int MSQLite3SelectVCsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }


int MSQLite3SelectNodeStatsGenericResourcesStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3InsertNodeStatsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }



int MSQLite3InsertNodeStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }






int MSQLite3InsertJobStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3InsertRsvStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3InsertTriggerStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }



int MSQLite3InsertVCStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }



int MSQLite3InsertRequestStmt(

    msqlite3_t   *MDBInfo,
    mstmt_t      *MStmt,
    char         *EMsg)

  {
  return(FAILURE);
  }





int MSQLite3InsertEventStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3ResetStmt(

  mstmt_sqlite3_t *MStmt)

  {
  return(FAILURE);
  }




int MSQLite3Fetch(

  mstmt_sqlite3_t *MStmt)

  {
  return(FAILURE);
  }




int MSQLite3NumParams(

  mstmt_sqlite3_t *MStmt,
  int             *Out)  

  {
  return(FAILURE);
  }




int MSQLite3BindParamInteger(

  mstmt_sqlite3_t *MStmt,  
  int              Param,  
  int             *Val,    
  int              IsArray)

  {
  return(FAILURE);
  }




int MSQLite3InsertCPStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)   /* O */

  {
  return(FAILURE);
  }




int MSQLite3BindParamBlob(

  mstmt_sqlite3_t *MStmt,
  int              Param,
  char            *Val)  

  {
  return(FAILURE);
  }




int MSQLite3IJobQueryStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)   /* O */

  {
  return(FAILURE);
  }




int MSQLite3FreeStmt(

  mstmt_sqlite3_t *MStmt)

  {
  return(FAILURE);
  }




int MSQLite3SelectGeneralStatsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3SelectEventsMsgsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3SelectGenericMetricsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3SelectGenericMetricsRangeStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3QueryCPStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3Initialize(

  mdb_t      *MDB,   
  char const *DBFile)

  {
  const char *Msg = "ERROR:    cannot handle request to use Moab's internal database. Moab was not built with SQLite3.\n";

  fprintf(stderr,"%s",Msg);

  MLog((char *)Msg);

  MMBAdd(
    &MSched.MB,
    (char *)Msg,
    NULL,
    mmbtOther,
    0,
    0,
    NULL);

  return(FAILURE);
  }  /* END MSQLite3Initialize() */





int MSQLite3BindColDouble(

  mstmt_sqlite3_t *MStmt,
  int              Col,  
  double          *Val)  

  {
  return(FAILURE);
  }




int MSQLite3BindColText(

  mstmt_sqlite3_t *MStmt,
  int              Col,  
  void            *Val,  
  int              Size) 

  {
  return(FAILURE);
  }




int MSQLite3SelectNodeStatsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




#if 0 /* not supported */
int MSQLite3BindParamArrays(

  mstmt_sqlite3_t *MStmt,  
  int              ArrSize)

  {
  return(FAILURE);
  }

int MSQLite3SetScrollStmt(

  mstmt_sqlite3_t *MStmt,
  int              ScrollSize,
  void            *RowsFetched)

  {
  return(FAILURE);
  }
#endif



int MSQLite3AllocDirectStmt(

  msqlite3_t *MDBInfo, 
  mstmt_t    *MStmt,   
  char const  *StmtText, /* I */
  char       *EMsg)     /* O */

  {
  return(FAILURE);
  }




int MSQLite3SelectNodeStatsGenericResourcesRangeStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3NumChangedRows(

  mstmt_sqlite3_t *MStmt,
  int             *Out)  

  {
  return(FAILURE);
  }




int MSQLite3Connect(

  msqlite3_t *MDBInfo,
  char       *EMsg)   

  {
  return(FAILURE);
  }




int MSQLite3SelectGeneralStatsRangeStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3NumResultCols(

  mstmt_sqlite3_t *MStmt,
  int             *Out)  

  {
  return(FAILURE);
  }




int MSQLite3Free(

  msqlite3_t *MDBInfo)

  {
  return(FAILURE);
  }



int MSQLite3BindColBlob(

  mstmt_sqlite3_t *MStmt,
  int              Col,  
  void            *Val,  
  int              Size) 

  {
  return(FAILURE);
  }




int MSQLite3BindParamDouble(

  mstmt_sqlite3_t *MStmt,  
  int              Param,  
  double          *Val,    
  int              IsArray)

  {
  return(FAILURE);
  }




int MSQLite3BindParamText(

  mstmt_sqlite3_t *MStmt,
  int              Param,
  char const      *Val,
  int              Size) 

  {
  return(FAILURE);
  }




int MSQLite3InsertGeneralStatsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3AllocStmt(

  msqlite3_t *MDBInfo, 
  mstmt_t    *MStmt,   
  char const *StmtText,
  char       *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3DeleteJobsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3DeleteRsvsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3DeleteTriggersStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }



int MSQLite3DeleteVCsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }



int MSQLite3DeleteRequestsStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3DeleteNodesStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3SelectNodeStatsRangeStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3DeleteCPStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt,   /* O */
  char       *EMsg)    /* O */

  {
  return(FAILURE);
  }




int MSQLite3ExecStmt(

  mstmt_sqlite3_t *MStmt)

  {
  return(FAILURE);
  }




void MSQLite3BindValue(

  msqlite3_value_t *Arr,  
  int               Index,
  int               Type, 
  void const       *Val,
  int               Size) 

  {
  return;
  }




int MSQLite3InitAllocatedStmt(

  msqlite3_t      *MDBInfo, 
  mstmt_sqlite3_t *MStmt,   
  char const      *StmtText,
  char            *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3SetStmt(

  msqlite3_t      *MDBInfo, 
  mstmt_t         *MStmt,   
  mstmt_sqlite3_t *MSStmt,  
  const char      *StmtText,
  char            *EMsg)

  {
  return(FAILURE);
  }




int MSQLite3BindColInteger(

  mstmt_sqlite3_t *MStmt,
  int              Col,  
  int             *Val)  

  {
  return(FAILURE);
  }




int MSQLite3PurgeCPStmt(

  msqlite3_t *MDBInfo,
  mstmt_t    *MStmt)  

  {
  return(FAILURE);
  }




int MSQLite3StmtError(

  mstmt_sqlite3_t *MStmt,   
  char const      *ExtraMsg,
  char            *EMsg)    

  {
  return(FAILURE);
  }




mbool_t MSQLite3InTransaction(

  msqlite3_t *MDBInfo)

  {
  return(FALSE);
  }




int MSQLite3BeginTransaction(

  msqlite3_t *MDBInfo,
    char *EMsg)

  {
  return(FAILURE);
  } /* END MSQLite3BeginTransaction() */




int MSQLite3EndTransaction(

  msqlite3_t *MDBInfo,
    char *EMsg)

  {
  return(FAILURE);
  } /* END MSQLite3EndTransaction() */




void stub(){if(0){BindParamsForExecution(NULL);
BaseResetStmt(NULL);MSQLite3Step(NULL);}}
#endif
/* END MSQLite3.c */
