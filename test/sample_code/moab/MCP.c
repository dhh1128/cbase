/* HEADER */

#include "moab-proto.h"


/**
 * @file MCP.c
 *
 * Moab Checkpointing
 */

/* Contains:                                    *
 *                                              *
 * int MCPCreate(CheckPointFile)                *
 * int MCPWriteSystemStats(ckfp)                *
 * int MCPWriteGridStats(ckfp)                  *
 * int MCPWriteSystemStats(ckfp)                *
 * int MCPLoad(CheckPointFile,Mode)             *
 * int MCPStoreRsvList(CP,RL)                   *
 * int MCPRestore(CkIndex,OName,O,Found)        *
 * int MCPLoadSysStats(Line)                    *
 *                                              */
 
 
 
#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"

 
static const enum MStatAttrEnum DefCPStatAList[] = {
  mstaTJobCount,
  mstaTNJobCount,
  mstaTQueueTime,
  mstaMQueueTime,
  mstaTReqWTime,
  mstaTExeWTime,
  mstaTPSReq,
  mstaTPSExe,
  mstaTPSDed,
  mstaTPSUtl,
  mstaTNJobAcc,
  mstaTXF,
  mstaTNXF,
  mstaMXF,
  mstaTBypass,
  mstaMBypass,
  mstaTQOSAchieved,
  mstaQOSCredits,
  mstaNONE };

/* GLOBALS */
msubjournal_t MSubJournal;

mhash_t MCPHashTable;

/**
 * Create checkpoint records for all jobs in specified job list.
 *
 * WARNING: Not thread-safe due to global var usage!
 *
 * @see MCPStoreCJobs() - peer (store completed jobs)
 * @see MCPCreate() - parent
 * @see MJobShouldCheckpoint() - child (determine if job is eligible to be checkpointed)
 * @see MJobStoreCP() - child
 * @see MCPStoreCJobs()
 *
 * @param CP        (I)
 * @param StoreFull (I)
 */

int MCPStoreQueue(
 
  mckpt_t  *CP,
  mbool_t   StoreFull)

  {
  mjob_t *J;

  mstring_t String(MMAX_LINE);

  job_iter JTI;

  char   *ptr;
  char   *tail;

  char    Name[MMAX_NAME + 1]; /* 64bytes + null byte for sscanf below */

  long    CheckPointTime = 0;
 
  mbool_t FullCP;

  mdb_t *MDBInfo;

  const char *FName = "MCPStoreQueue";
 
  MDB(3,fCKPT) MLog("%s(CP,JL,%s)\n",
    FName,
    MBool[StoreFull]);

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (MJobShouldCheckpoint(J,NULL) == FALSE)
      continue;
 
    MDB(4,fCKPT) MLog("INFO:     checkpointing job '%s'\n",
      J->Name);
 
    /* create job XML */

    if (StoreFull == TRUE)
      {
      FullCP = TRUE;
      }
    else if ((bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM == NULL)) ||
             ((J->DestinationRM != NULL) && bmisset(&J->DestinationRM->IFlags,mrmifLocalQueue)))
      {
      /* job has not been staged - all data must be checkpointed */

      FullCP = TRUE;
      }
    else if ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtMoab))
      {
      /* do full checkpoints for jobs migrated to other Moab peers */

      FullCP = TRUE;
      }
    else if (bmisset(&J->IFlags,mjifMaster))
      {
      FullCP = TRUE;
      }
    else
      {
      /* store partial data - the rest comes from RM */

      FullCP = FALSE;
      }

    MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

    if ((MCP.UseDatabase == TRUE) && (MDBInfo->DBType != mdbNONE))
      {
      MOCheckpoint(mxoJob,(void *)J,FullCP,NULL);

      bmunset(&J->IFlags,mjifShouldCheckpoint);

      continue;
      }

    String.clear();

    /* do a Job to CP string conversion */
    if (MJobStoreCP(J,&String,FullCP) == FAILURE)
      {
      continue;
      }

    bmunset(&J->IFlags,mjifShouldCheckpoint);

    if ((bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM == NULL)) ||
        ((bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM != NULL) &&
         (J->DestinationRM->Type == mrmtMoab))) ||
        ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->IFlags,mrmifLocalQueue))) ||
        ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->Flags,mrmfFullCP))))

      {
      /* store file in spool/$jobid.cp */

      MCPPopulateJobCPFile(J,String.c_str(),MCP.UseCPJournal);

      continue;
      }   /* END if ((bmisset(&J->SRM))... */
 
    MJobGetCPName(J,Name);

    fprintf(CP->fp,"%-9s %20s %9ld %s\n",
      MCPType[mcpJob],
      Name,
      MSched.Time,
      String.c_str());
    }    /* END for (J) */

  /* NOTE:  should only perform this action at start up,
            i.e., load stale info into internal buffer and dump it out
            en-masse at ckpt create time */

  if ((CP->OBuffer != NULL) && (CP->OldJobBufferIsInitialized == FALSE))
    {
    int   OverflowCnt = 0;
 
    /* locate old job checkpoint objects */

    MDB(3,fCKPT) MLog("INFO:     checkpointing old non-completed job data (%d byte buffer)\n",
      (int)((CP->OBuffer != NULL) ? strlen(CP->OBuffer) : 0));

    CP->OldJobBuffer = "";;

    ptr = CP->OBuffer;

    while ((ptr = strstr(ptr,MCPType[mcpJob])) != NULL)
      {
      char *Entry = NULL;
      char  tmpLine[MMAX_LINE + 1]; /* 1024 bytes + null byte for sscanf below */

      sscanf(ptr,"%1024s %64s %ld",
        tmpLine,  /* throwaway - should just be the checkpoint type, ie: JOB */
        Name,
        &CheckPointTime);

      if ((ptr > CP->OBuffer) && (*(ptr - 1) != '\0') && (*(ptr - 1) != '\n'))
        {
        /* ignore non-job entries */

        ptr += strlen(MCPType[mcpJob]);

        continue;
        }

      if (((long)MSched.Time - CheckPointTime) > (long)MAX(CP->CPJobExpirationTime,MCP.CPExpirationTime))
        {
        char TString[MMAX_LINE];

        MULToTString(MSched.Time - CheckPointTime,TString);

        MDB(2,fCKPT) MLog("INFO:     expiring checkpoint data for job %s.  not updated in %s\n",
          Name,
          TString);

        ptr += strlen(MCPType[mcpJob]);

        continue;
        }

      if ((MJobFind(Name,&J,mjsmBasic) == SUCCESS) ||
          (MJobCFind(Name,&J,mjsmBasic) == SUCCESS))
        {
        /* current job info already stored */

        ptr += strlen(MCPType[mcpJob]);

        continue;
        }

      if ((tail = strchr(ptr,'\n')) == NULL)
        {
        MDB(1,fCKPT) MLog("ALERT:    checkpoint file corruption at offset %d\n",
          (int)(ptr - CP->OBuffer));

        ptr += strlen(MCPType[mcpJob]);

        continue;
        }

      /* copy old data - ptr is beginning and tail is end */

      Entry = (char *)MUMalloc(sizeof(char) * (tail - ptr) + 1);

      MUStrCpy(Entry,ptr,tail - ptr + 1);

      /* Assign the char array to the string object */
      CP->OldJobBuffer = Entry;
      CP->OldJobBuffer += "\n" ;

      ptr = tail;

      MUFree(&Entry);
      }  /* END while (ptr = strstr()) */

    if (OverflowCnt > 0) 
      {
      MDB(1,fCKPT) MLog("ALERT:    checkpoint old job buffer overflow, lost %d entries (%d KB buffer)\n",
         OverflowCnt,
         (int)sizeof(CP->OldJobBuffer) >> 10);  
      }
 
    CP->OldJobBufferIsInitialized = TRUE;
    }    /* END if ((CP->OBuffer != NULL) && (CP->OldJobBuffer[0] == '\0')) */

  if (!CP->OldJobBuffer.empty())
    {
    fprintf(CP->fp,"%s\n",
      CP->OldJobBuffer.c_str());
    }

  return(SUCCESS);
  }  /* END MCPStoreQueue() */






int MCPStoreJobTemplates(

  mckpt_t   *CP,        /* I */
  marray_t  *JL,        /* I */
  mbool_t    StoreFull)

  {
  int jindex;

  mjob_t *TJ = NULL;
  
  mdb_t *MDBInfo;

  mstring_t String(MMAX_LINE);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  for (jindex = 0;MUArrayListGet(JL,jindex);jindex++)
    {
    TJ = *(mjob_t **)(MUArrayListGet(JL,jindex));

    if (TJ == NULL)
      break;

    if (!bmisset(&TJ->IFlags,mjifTemplateIsDynamic))
      continue;

    if (bmisset(&TJ->IFlags,mjifTemplateIsDeleted))
      continue;

    if ((MCP.UseDatabase == TRUE) && (MDBInfo->DBType != mdbNONE))
      {
      MOCheckpoint(mxoxTJob,(void *)TJ,TRUE,NULL);

      continue;
      }

    String.clear();   /* Reset the string on every itration */

    if (MJobStoreCP(TJ,&String,TRUE) == FAILURE)
      {
      continue;
      }

    fprintf(CP->fp,"%-9s %20s %9ld %s\n",
      MCPType[mcpDTJob],
      TJ->Name,
      MSched.Time,
      String.c_str());
    }   /* END for (jindex) */

  return(SUCCESS);
  }  /* END MCPStoreJobTemplates() */





/**
 * Store checkpoint information about all jobs associated with specified RM.
 *
 * @see MCPStoreQueue()
 *
 * @param CP (I)
 * @param RM (I) [optional]
 * @param StoreFull (I)
 */

int MCPStoreRMJobList(
 
  mckpt_t  *CP,        /* I */
  mrm_t    *RM,        /* I (optional) */
  mbool_t   StoreFull) /* I */

  {
  mjob_t *J;

  mstring_t String(MMAX_LINE);

  mbool_t FullCP;

  mrm_t  *tmpRM;

  char  tmpName[MMAX_NAME];
  char *JobName = NULL;

  int     rindex;

  mhashiter_t JobListIter;

  const char *FName = "MCPStoreRMJobList";
 
  MDB(3,fCKPT) MLog("%s(CP,JL,%s)\n",
    FName,
    MBool[StoreFull]);

  for (rindex = 0;rindex < MSched.M[mxoRM];rindex++)
    {
    tmpRM = &MRM[rindex];

    if (tmpRM->Name[0] == '\0')
      break;

    if (MRMIsReal(tmpRM) == FALSE)
      continue;

    if ((RM != NULL) && (tmpRM != RM))
      continue;

    if (!bmisset(&tmpRM->IFlags,mrmifLocalQueue))
      continue;

    if (MUHTIsInitialized(&tmpRM->U.S3.JobListIndex) == FAILURE)
      continue;

    MUHTIterInit(&JobListIter);

    while (MUHTIterate(&tmpRM->U.S3.JobListIndex,&JobName,NULL,NULL,&JobListIter) == SUCCESS)
      {
      if (MJobFind(JobName,&J,mjsmBasic) == FAILURE)
        continue;

      if (MJobShouldCheckpoint(J,tmpRM) == FALSE)
        continue;

      MDB(4,fCKPT) MLog("INFO:     checkpointing job '%s'\n",
        J->Name);
 
      /* create job XML */

      if ((StoreFull == TRUE) ||
         (bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM == NULL)) ||
         ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->IFlags,mrmifLocalQueue))))
        {
        /* job has not been staged - all data must be checkpointed */
  
        FullCP = TRUE;
        }
      else
        {
        /* store partial data - the rest comes from RM */

        FullCP = FALSE;
        }

      String.clear();

      if (MJobStoreCP(J,&String,FullCP) == FAILURE)
        {
        continue;
        }

      if ((bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM == NULL)) ||
          ((bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && (J->DestinationRM != NULL) &&
           (J->DestinationRM->Type == mrmtMoab))) ||
          ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->IFlags,mrmifLocalQueue))) ||
          ((J->DestinationRM != NULL) && (bmisset(&J->DestinationRM->Flags,mrmfFullCP))))
        {
        /* store file in spool/$jobid.cp */

        MCPPopulateJobCPFile(J,String.c_str(),MCP.UseCPJournal);

        continue;
        }   /* END if ((bmisset(&J->SRM))... */
 
      MJobGetCPName(J,tmpName);

      fprintf(CP->fp,"%-9s %20s %9ld %s\n",
        MCPType[mcpJob],
        tmpName,
        MSched.Time,
        String.c_str());
      }    /* END while (MUHTIterate()) */

    /* do not check old data */

    }      /* END for (rindex) */
 
  return(SUCCESS);
  }  /* END MCPStoreRMJobList() */





/**
 * Create/update checkpoint file.
 *
 * @see MSysShutdown() - parent - checkpoint at shutdown
 * @see MSysProcessEvents() - perform periodic CP update
 * @see MCPStoreCredList() - child
 * @see MCPLoad() - peer - load checkpoint info
 * @see MCPStoreObj() - child
 *
 * @param CPFile (I)
 */

int MCPCreate(
 
  char *CPFile)  /* I */
 
  {
  int   count;

  mckpt_t *CP;
 
  char  tmpCPFileName[MMAX_LINE];

  mdb_t *MDBInfo;

  const char *FName = "MCPCreate";
 
  MDB(3,fCKPT) MLog("%s(%s)\n",
    FName,
    (CPFile != NULL) ? CPFile : "NULL");
 
  if ((CPFile == NULL) || (CPFile[0] == '\0'))
    {
    return(FAILURE);
    }

  CP = &MCP;

  /* load old checkpoint file */

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
    {
    if (MCPIsSupported(CP,CP->DVersion) == TRUE)
      { 
      if ((CP->OBuffer = MFULoad(CPFile,1,macmRead,&count,NULL,NULL)) == NULL)
        {
        if ((MSched.Mode == msmNormal) || (MSched.Mode == msmMonitor))
          {
          MDB(1,fCKPT) MLog("WARNING:  cannot load checkpoint file '%s'\n",
            CPFile);
          }
        }
      }
    else
      {
      CP->OBuffer = NULL;
      }
  
    MUStrCpy(tmpCPFileName,CPFile,sizeof(tmpCPFileName));
    MUStrCat(tmpCPFileName,".tmp",sizeof(tmpCPFileName));
   
    /* open checkpoint file - use MFUCreate to set file permissions */
   
    MFUCreate(tmpCPFileName,NULL,NULL,0,0600,-1,-1,TRUE,NULL);
   
    if ((CP->fp = fopen(tmpCPFileName,"w+")) == NULL)
      { 
      if ((MSched.Mode == msmNormal) || (MSched.Mode == msmMonitor))
        {
        MDB(1,fCKPT) MLog("WARNING:  cannot open checkpoint file '%s'.  errno: %d (%s)\n",
          CPFile,
          errno,
          strerror(errno));
        }
   
      return(FAILURE);
      }
    }    /* END if (MDBInfo->DBType == mdbNONE) */
 
  /* checkpoint scheduler, job, reservation, and node state information */
 
  mstring_t OString(MMAX_LINE);

  MSchedToString(&MSched,mfmAVP,TRUE,&OString);
  MCPStoreObj(CP->fp,mcpSched,(char *)MXO[mxoSched],OString.c_str());
 
  OString = "";

  MSysToString(&MSched,&OString,TRUE);
  MCPStoreObj(CP->fp,mcpSys,(char *)MXO[mxoSys],OString.c_str());

  /* must store the RM data before the job data */

  MCPStoreCluster(CP,MNode);

  MCPStoreRMList(CP);

  MCPStoreRMJobList(CP,NULL,FALSE);

  MCPStoreQueue(CP,FALSE);

  /* store completed jobs */

  MCPStoreCJobs(CP);

  /* store completed VMs */

  MCPStoreVMs(CP,&MSched.VMCompletedTable,TRUE);

  /* store job templates if they have stats */

  MCPStoreTJobs(CP);

  /* stores dynamic job templates */

  MCPStoreJobTemplates(CP,&MTJob,TRUE);

  /* store active and completed reservations */

  MCPStoreRsvList(CP);
 
  MCPStoreSRList(CP); 

  if (MSched.OptimizedCheckpointing == FALSE)
    {
    MCPStoreVPCList(CP,&MVPC);
    }
 
  MCPStoreCredList(CP,NULL,mxoUser);
  MCPStoreCredList(CP,NULL,mxoGroup);
  MCPStoreCredList(CP,NULL,mxoAcct);
  MCPStoreCredList(CP,NULL,mxoQOS);
  MCPStoreCredList(CP,NULL,mxoClass);

  MCPStoreVCs(CP);

  MCPStoreTransactions(CP);

  /* ENABLE FROM HERE */
  MCPWriteGridStats(CP);
   
  MCPWriteSystemStats(CP);
   
  MCPWriteTList(CP);
  /* TO HERE */
   
  if (CP->fp != NULL)
    {
    fprintf(CP->fp,"%s\n",
      MCONST_CPTERMINATOR);
   
    fclose(CP->fp);
   
    CP->fp = NULL;
    }
 
  if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
    {
    MUStrCpy(tmpCPFileName,CPFile,sizeof(tmpCPFileName));
    MUStrCat(tmpCPFileName,".1",sizeof(tmpCPFileName));
   
    if (rename(CPFile,tmpCPFileName) == -1)
      {
      MDB(0,fCORE) MLog("ERROR:    cannot rename checkpoint file '%s' to '%s' errno: %d (%s)\n",
        CPFile,
        tmpCPFileName, 
        errno,
        strerror(errno));
      }
   
    MUStrCpy(tmpCPFileName,CPFile,sizeof(tmpCPFileName));
    MUStrCat(tmpCPFileName,".tmp",sizeof(tmpCPFileName));
   
    if (rename(tmpCPFileName,CPFile) == -1)
      {
      MDB(0,fCORE) MLog("ERROR:    cannot rename checkpoint file '%s' to '%s' errno: %d (%s)\n",
        tmpCPFileName,
        CPFile,
        errno,
        strerror(errno));
      }
    }  /* END if(MDBInfo->DBType == mdbNONE) */
  else
    {
    MDBPurgeCP(MDBInfo,MSched.Time - MCP.CPExpirationTime,NULL);
    }

  /* clear out journal since we just did a full update */

  MCPJournalClear();

  return(SUCCESS);
  }  /* END MCPCreate() */





/**
 *  Build a hash table for each line in the checkpoint file for processing on 
 *  moab boot for increased speed.
 *
 *  NOTE: Only used before iteration 1 since checkpoint file can be modified 
 *        after iteration 0. 
 *
 * @param tmpPtr (I)  Pointer to a copy of the MCP.Buffer
 */

int MCPBuildHashTable(

  char *tmpPtr)

  {
  char  HashString[MMAX_LINE];
  char  EndType[MMAX_LINE];
  char  EndName[MMAX_LINE];
  char  EndTime[MMAX_LINE];

  static mbool_t MCPHashTableCreated = FALSE;
 
  if (tmpPtr == NULL)
    {
    return(FAILURE);
    }

  if (MSched.Iteration > 0)
    {
    return(FAILURE);
    }

  if (MCPHashTableCreated == TRUE)
    {
    return(FAILURE);
    }
 
  MUHTCreate(&MCPHashTable,-1);

  MCPHashTableCreated = TRUE;

  while (tmpPtr != NULL)
    {
    EndType[0] = '\0';
    EndName[0] = '\0';
    EndTime[0] = '\0';

    if ((sscanf(tmpPtr,"%s%s%s",EndType,EndName,EndTime) == 3) && (strtol(EndTime,NULL,10) > 0))
      {
      char *cpyLoc;

      cpyLoc = strstr(tmpPtr,EndTime);

      if (cpyLoc != NULL)
        {
        MUStrCpy(HashString,tmpPtr,(cpyLoc - tmpPtr) + 1);

        if (!MUStrIsEmpty(HashString))
          {
          MUHTAdd(
            &MCPHashTable,
            HashString,   /* I hash key */
            tmpPtr,       /* I object pointer */
            NULL,
            NULL);
          }
        }
      }

    /* Get pointer to the next record */

    tmpPtr = strchr(tmpPtr,'\n');

    if (tmpPtr != NULL)
      {
      tmpPtr++; /* advance beyond the line feed at the end of the last record */
      }
    }    /* END while (tmpPtr != NULL) */ 

  return(SUCCESS);
  }  /* END MCPBuildHashTable() */





/**
 * Restores checkpoint information to a given object
 *
 * @param CKIndex (I) The type of object to look for. Can be one of. 
 *   - mcpJob      @see MJobLoadCP()
 *   - mcpNode
 *   - mcpAcct
 *   - mcpUser
 *   - mcpGroup
 *   - mcpPar
 *   - mcpRM
 *   - mcpSched
 *   - mcpSys
 *   - mcpSRsv
 * @param OName (I) A string denoting the name of the object to look for
 * @param O (I) [modified] The object that will be populated if found
 * @param Found (O) [optional/modified] Set to FALSE if the object is not found in the checkpoint buffer.
 */

int MCPRestore(
 
  enum MCkptTypeEnum  CKIndex, /* I */
  const char         *OName,   /* I */
  void               *O,       /* I (modified) */
  mbool_t            *Found)   /* O (optional/modified) */
 
  {
  int   count;
  char *ptr;
  char *tmp;
 
  mstring_t MLine(MMAX_BUFFER);;
  char CPLineKey[MMAX_LINE];
  int LineLength;

  static char *LastJobPtr = NULL;
  static char *tmpMCPBuffer = NULL;

  const char *FName = "MCPRestore";

  mdb_t *MDBInfo;

  MDB(4,fCKPT) MLog("%s(%s,%s,%s,Found)\n",
    FName,
    OName,
    MCPType[CKIndex],
    (O == NULL) ? "NULL" : "OPtr");

  if (MCP.CPFileName == NULL)
    {
    MDB(6,fCKPT) MLog("INFO:     no checkpoint file name\n"); 

    return(SUCCESS);
    }

  if ((O == NULL) || (OName == NULL))
    {
    return(FAILURE);
    }

  if (Found != NULL)
    *Found = TRUE;
 
  /* load checkpoint file */

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
    {
    if ((MSched.Iteration > 0) && (tmpMCPBuffer != NULL))
      {
      /* After iterations -1 and 0, the checkpoint file may have been modified 
         so the original hash table is stale.  We only use this hash table 
         during iterations -1 and 0 to speed up boot time processing - after 
         that free it. */

      /* free the copy of the MCP buffer that the hash table is pointing at 
         before we free the hash table */

      MUFree(&tmpMCPBuffer);

      MUHTFree(
        &MCPHashTable,
        FALSE,
        NULL);

      MDB(7,fCKPT) MLog("INFO:     checkpoint hash table freed in iteration %d\n",
        MSched.Iteration);
      }

    if (MCP.Buffer == NULL)
      { 
      if ((MCP.Buffer = MFULoadNoCache(MCP.CPFileName,1,&count,NULL,NULL,NULL)) == NULL)
        {
        MDB(1,fCKPT) MLog("WARNING:  cannot load checkpoint file '%s'\n",
          MCP.CPFileName);
   
        if (Found != NULL)
          *Found = FALSE;
   
        return(FAILURE);
        }
   
      MCP.LastCPTime = MSched.Time;

      /* must reset variables pointing to old checkpoint buffer */

      LastJobPtr = NULL;

      if ((MSched.Iteration < 1) && (tmpMCPBuffer == NULL))
        {
        /* Build a hash table for boot time processing (iterations -1 and 0) of the checkpoint file */
        /* Note that this copy is freed just before we free the hash table */

        MUStrDup(&tmpMCPBuffer,MCP.Buffer);

        MCPBuildHashTable(tmpMCPBuffer);

        MDB(7,fCKPT) MLog("INFO:     checkpoint hash table created in iteration %d\n",
          MSched.Iteration);
        }
      }
   
    /* NOTE:  CP version should be verified */
   
    if (LastJobPtr == NULL)
      LastJobPtr = MCP.Buffer;
   
    /* create a key to search for a checkpoint file line entry for the requested object */

    sprintf(CPLineKey,"%-9s %20s ",
      MCPType[CKIndex],
      OName);

    if (tmpMCPBuffer != NULL)
      {
      /* At boot time use the fast hash table lookup instead of the much slower strstr() */

      if (MUHTGet(&MCPHashTable,CPLineKey,(void **)&ptr,NULL) != SUCCESS)
        {
        /* no checkpoint entry for object */
    
        MDB(4,fCKPT) MLog("INFO:     no checkpoint entry for object '%s'\n",
          CPLineKey); 
   
        if (Found != NULL)
          *Found = FALSE;
   
        return(SUCCESS);
        }
      }
    else
      {
      MDB(8,fCKPT) MLog("INFO:     checkpoint non hash table lookup '%s' in iteration %d\n",
        CPLineKey,
        MSched.Iteration);

      if ((CKIndex == mcpJob) && 
         ((ptr = strstr(LastJobPtr,CPLineKey)) != NULL))
        {
        /* NOTE: optimization depends on job checkpoint file being written
                 in order of RM reporting */
   
        /* job found */
   
        if (MSched.M[mxoJob] > 10000)
          LastJobPtr = ptr;
        }
      else if ((ptr = strstr(MCP.Buffer,CPLineKey)) == NULL)
        {
        /* no checkpoint entry for object */
   
        MDB(4,fCKPT) MLog("INFO:     no checkpoint entry for object '%s'\n",
          CPLineKey);
   
        if (Found != NULL)
          *Found = FALSE;
   
        return(SUCCESS);
        }
      }
     
    /* We now have a pointer to a newline delimited line in either the MCP.Buffer or the tmpMCPBuffer */
    /* get the length of this line using the newline delimiter */
   
    if ((tmp = strchr(ptr,'\n')) == NULL)
      {
      MDB(1,fCKPT) MLog("WARNING:  incorrectly formed checkpoint line corresponding to key '%s'\n",
        CPLineKey);

      return(FAILURE);
      }

    LineLength = tmp - ptr;


    /* Need a tmp string buffer to format the data */
    char *tmpBuf = new char[LineLength << 1];

    snprintf(tmpBuf,LineLength << 1,"%.*s",LineLength,ptr);

    MLine = tmpBuf;      /* initialize the string object */

    delete[] tmpBuf;

    }    /* END if (MDBInfo->DBType == mdbNONE) */
  else
    {
    /* load checkpoint info from db */

    if (MDBQueryCP(MDBInfo,(char *)MCPType[CKIndex],OName,NULL,1,&MLine,NULL) == FAILURE)
      {
      /* no entry found in DB for given object */

      return(SUCCESS);
      }
    }
 
  switch (CKIndex)
    {
    case mcpAcct:

      MAcctLoadCP((mgcred_t *)O,MLine.c_str());

      break;

    case mcpJob:
 
      MJobLoadCPNew((mjob_t *)O,MLine.c_str());
 
      break;

    case mcpVM:
    case mcpCVM:

      MVMLoadCP((mvm_t *)O,MLine.c_str());

      break;

    case mcpTJob:

      MTJobLoadCP((mjob_t *)O,MLine.c_str(),FALSE);

      break;
 
    case mcpNode:
 
      MNodeLoadCP((mnode_t *)O,MLine.c_str());
 
      break;
 
    case mcpUser:

      MOLoadCP((mgcred_t *)O,mxoUser,MLine.c_str());
 
      break; 
 
    case mcpGroup:

      MGroupLoadCP((mgcred_t *)O,MLine.c_str());       
 
      break;

    case mcpPar:

      /* NO-OP */

      break;
 
    case mcpRM:

      MRMLoadCP((mrm_t *)O,MLine.c_str());       
 
      break;

    case mcpSched:

      MCPLoadSched(&MCP,MLine.c_str(),(msched_t *)O); 

      break;

    case mcpSys:

      MCPLoadSys(&MCP,MLine.c_str(),(msched_t *)O);

      break;

    case mcpTrans:

      MTransLoadCP((mtrans_t *)O,MLine.c_str());

      break;

    case mcpSRsv:

      /* NYI */

      break;
 
    default:
 
      MDB(1,fCKPT) MLog("ERROR:    unexpected checkpoint type, %d, detected in %s()\n",
        CKIndex,
        FName);
 
      break;
    }  /* END switch (CKIndex) */

  return(SUCCESS);
  }  /* END MCPRestore() */





/**
 * Loads the contents of the given checkpoint file.
 *
 * @see MSysStartServer() - parent
 *
 * @param CPFile (I) The path of the file that should be loaded. [utilized]
 * @param Mode Indicatest what should be loaded from the checkpoint file. (I)
 */

int MCPLoad(
 
  char               *CPFile, /* I (utilized) */
  enum MCKPtModeEnum  Mode)   /* I */
 
  {
  char *ptr;
 
  char *head;
 
  int   ckindex;
  int   count;
 
  int   buflen;
 
  char *tokptr;
 
  static int FailureIteration = -999;

  mdb_t *MDBInfo;

  const char *FName = "MCPLoad";
 
  MDB(3,fCKPT) MLog("%s(%s,%s)\n",
    FName,
    CPFile,
    MCKMode[Mode]);
 
  if (FailureIteration == MSched.Iteration) 
    {
    return(FAILURE);
    }

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);
 
  if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
    {
    /* load checkpoint file */
   
    if ((MCP.OBuffer = MFULoad(CPFile,1,macmWrite,&count,NULL,NULL)) == NULL)
      {
      MDB(1,fCKPT) MLog("WARNING:  cannot load checkpoint file '%s'\n",
        CPFile);
   
      FailureIteration = MSched.Iteration;
  
      /* create SCHEDFAIL event (NYI) */
 
      return(FAILURE);
      }
   
    if ((strstr(MCP.OBuffer,MCONST_CPTERMINATOR) == NULL) &&
        (strstr(MCP.OBuffer,MCONST_CPTERMINATOR2) == NULL))
      {
      return(FAILURE);
      }
   
    head   = MCP.OBuffer;
    buflen = strlen(head);
   
    /* determine checkpoint version */
   
    if (MCP.DVersion[0] == '\0')
      {
      if (MCPRestore(mcpSched,"sched",(void *)&MSched,NULL) == FAILURE)
        {
        MDB(1,fCKPT) MLog("ALERT:    cannot load sched data.  aborting checkpoint load\n");
        
        MUFree(&MCP.OBuffer);
       
        return(FAILURE);
        }
   
      if (MCPIsSupported(&MCP,MCP.DVersion) == FALSE)
        {
        /* cannot load checkpoint file */
   
        return(FAILURE);
        }
      }
   
#ifdef __MNOT
    MDB(7,fCKPT) MLog("INFO:   checkpoint file contents: %s \n",
        head);
#endif  /* END __MNOT */
   
    ptr = MUStrTok(head,"\n",&tokptr);
    }   /* END if (MDBInfo->DBType == mdbNONE) */
  else
    {
    char EMsg[MMAX_LINE];
    const char *ObjectType = NULL;
    const char *DataFilter = NULL;

    if (MCP.DVersion[0] == '\0')
      {
      if (MCPRestore(mcpSched,"sched",(void *)&MSched,NULL) == FAILURE)
        {
        MDB(1,fCKPT) MLog("ALERT:    cannot load sched data.  aborting checkpoint load\n");
        
        MUFree(&MCP.OBuffer);
        
        return(FAILURE);
        }
      }

    mstring_t MString(MMAX_BUFFER);

    if (Mode == mckptCJob)
      {
      ObjectType = MCPType[mcpCJob];
      }
    else if (Mode == mckptSched)
      {
      ObjectType = MCPType[mcpSched];
      }
    else
      {
      /* skip any "internal" non-completed job records that are not usually processed at this 
         time (they aren't part of the moab.ck, but instead would get their own *.cp file in
         the spool dir) */

      DataFilter = "%RM=\"internal\"%";
      }

    if (MDBQueryCP(MDBInfo,(char *)ObjectType,NULL,DataFilter,-1,&MString,EMsg) == FAILURE)
      {
      return(FAILURE);
      }

    /* Get a writable string from the MString */

    MCP.OBuffer = strdup(MString.c_str());

    head   = MCP.OBuffer;
    buflen = strlen(head);
 
    ptr = MUStrTok(head,"\n",&tokptr);
    }
 
  while (ptr != NULL)
    {
    /* traverse ckpt buffer, one line at a time */

    if ((!strcmp(ptr,MCONST_CPTERMINATOR)) ||
        (!strcmp(ptr,MCONST_CPTERMINATOR2)))
      {
      /* end of file located */

      break;
      }

    head = ptr + strlen(ptr) + 1;
 
    if ((head - MCP.OBuffer) > buflen)
      {
      head = MCP.OBuffer + buflen;
      }
 
    for (ckindex = 0;MCPType[ckindex] != NULL;ckindex++)
      {
      if (strncmp(ptr,MCPType[ckindex],strlen(MCPType[ckindex])))
        continue;

      if ((Mode == mckptRsv) &&
          (ckindex != mcpRsv) &&
          (ckindex != mcpSRsv) && 
          (ckindex != mcpPar))
        {
        /* Rsv mode only processes Rsv, SRsv, Par objects */

        break;
        }

      if ((Mode == mckptGeneral) &&
         ((ckindex == mcpRsv)  ||
          (ckindex == mcpSRsv) ||
          (ckindex == mcpPar)  ||
          (ckindex == mcpCJob)))
        { 
        /* general mode does not process this object */

        break;
        }

      if ((Mode == mckptSched) &&
          (ckindex != mcpSched))
        {
        break;
        }

      if ((Mode == mckptSys) && 
          (ckindex != mcpSys))
        {
        break;
        }

      if ((Mode == mckptCJob) &&
          (ckindex != mcpCJob))
        {
        break;
        }
 
      MDB(5,fCKPT) MLog("INFO:     loading %s checkpoint data '%s'\n",
        MCPType[ckindex],
        ptr);

      switch (ckindex)
        {
        case mcpAcct:

          MOLoadCP(NULL,mxoAcct,ptr);

          break;

        case mcpClass:

          MOLoadCP(NULL,mxoClass,ptr);

          break;

        case mcpCJob:

          MCJobLoadCP(ptr);

          break;

        case mcpVM:
        case mcpCVM:

          MVMLoadCP(NULL,ptr);

          break;

        case mcpGroup:

          MOLoadCP(NULL,mxoGroup,ptr);

          break;

        case mcpJob:

           /* if LOADALLJOBCP == FALSE then checkpointed jobs are
             handled on a per job basis in MRMJobPostLoad() by MCPRestore()
             and old data is stored by MCPStoreQueue() */

          /* only handles jobs that have already been stored in .moab.ck file--NOT the individual spool *.cp files */

          if (MSched.LoadAllJobCP == TRUE)
            MJobLoadCPNew(NULL,ptr);

          break;
 
        case mcpNode:

          /* NO-OP */

          /* handled on a per node basis by MNodePostLoad()->MCPRestore(mxoNode)
             and old data is stored by MCPStoreCluster() */

          /* MNodeLoadCP((mnode_t *)O,Line); */

          break;

        case mcpPar:

          MCPLoadPar(ptr);

          break;

        case mcpQOS:

          MOLoadCP(NULL,mxoQOS,ptr);

          break;

        case mcpRM:

          /* for static rm's, handled on a per RM basis by MRMLoadConfig() */
          /* internal queue handled in MRMInitializeLocalQueue */

          MRMLoadCP(NULL,ptr); 

          break;

        case mcpTotal:
        case mcpRTotal:
        case mcpCTotal:
        case mcpGTotal:

          MCPLoadStats(ptr);

          break;

        case mcpRsv:

          MRsvLoadCP(NULL,ptr);

          break;

        case mcpSRsv:

          MCPLoadSR(ptr);

          break;

        case mcpSched:

          /* handled above in MCPRestore(mcpSched,"sched",(void *)&MSched,NULL) */

          break;

        case mcpSys:

          MCPLoadSys(&MCP,ptr,&MSched);

          break;

        case mcpSysStats:

          MCPLoadSysStats(ptr);

          break;

        case mcpTrigger:

          MOLoadCP(NULL,mxoTrig,ptr);

          break;

        case mcpUser:

          MOLoadCP(NULL,mxoUser,ptr);

          break;

        case mcpDTJob:

          MTJobLoadCP(NULL,ptr,TRUE);

          break;

        case mcpTJob:

          /* handled in MCPRestore */

          break;

        case mcpTrans:

          MTransLoadCP(NULL,ptr);

          break;

        case mcpVC:

          MCPLoadVC(ptr);

          break;

        default:

          MDB(1,fCKPT) MLog("ERROR:    line '%s' not handled in checkPoint file '%s'\n",
            ptr,
            CPFile);

          break;
        }  /* END switch (ckindex) */

      break; 
      }      /* END for (ckindex) */

    if (MCPType[ckindex] == NULL)
      {
      MDB(3,fCKPT) MLog("WARNING:  unexpected line '%s' in checkpoint file '%s'\n",
        ptr,
        CPFile);
      }
 
    ptr = MUStrTok(NULL,"\n",&tokptr);
    }        /* END while (ptr != NULL) */

  MUFree(&MCP.OBuffer);  /* free and clear the pointer */

  if (MCP.CPInitTime == 0)
    MCP.CPInitTime = MSched.Time;

  return(SUCCESS);
  }  /* END MCPLoad() */





/**
 * Store reservation information in checkpoint repository.
 *
 * @see MRsvLoadCP()
 * @see MRsvToXML() - child (called w/Mode=mcmCP)
 * @see MCPCreate() - parent
 *
 * @param CP (I)
 */

int MCPStoreRsvList(
 
  mckpt_t  *CP)
 
  {
  rsv_iter RTI;

  int     BufSize;

  mrsv_t *R;
 
  /* NOTE: this needs to be able to handle 3 hostlists of up to MMAX_NODE side */

  char   *BufP = NULL;

  mxml_t *E = NULL;

  const char *FName = "MCPStoreRsvList";
 
  MDB(3,fCKPT) MLog("%s(CP)\n",
    FName);
 
  if (CP == NULL)
    {
    return(FAILURE);
    }
 
  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    MMBPurge(&R->MB,NULL,-1,MSched.Time);

    /* ignore job-based reservations */
 
    if ((R->Type == mrtJob) || (R->Type == mrtMeta))
      {
      MDB(6,fCKPT) MLog("INFO:     ignoring rsv '%s'\n",
        R->Name);
 
      continue;
      }

    /* ignore static reservations */

#if 0  /* this disables checkpointing vpcs */
    if (bmisset(&R->Flags,mrfStatic))
      {
      continue;
      }
#endif 

    /* ignore dynamic job reservations */

    if (R->DisableCheckpoint == TRUE)
      {
      continue;
      }

    /* NOTE:  allow checkpointing of SR rsv's to preserve alloc resource list */
 
    MDB(4,fCKPT) MLog("INFO:     checkpointing rsv '%s'\n",
      R->Name);

    E = NULL;

    /* NOTE: checkpoint default rsv attributes */
 
    if (MRsvToXML(R,&E,NULL,NULL,FALSE,mcmCP) == FAILURE)
      {
      continue;
      }
 
    if (MXMLToXString(
          E,
          &BufP,     /* O (alloc) */
          &BufSize,  /* I/O */
          MMAX_BUFFER << 8,
          NULL,
          TRUE) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      MXMLDestroyE(&E);

      snprintf(tmpLine,sizeof(tmpLine),"ALERT:    could not checkpoint rsv '%s' buffer too small\n",
        R->Name);

      MDB(1,fCKPT) MLog("ALERT:    could not checkpoint rsv '%s' buffer too small\n",
        R->Name);

      /* this failure is critical, add message to scheduler */

      MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,1,NULL);

      continue;
      }
 
    MXMLDestroyE(&E);

    MCPStoreObj(CP->fp,mcpRsv,R->Name,BufP);
    }    /* END for (rindex) */

  MUFree(&BufP);
 
  return(SUCCESS);
  }  /* END MCPStoreRsvList() */





/**
 * Stores the given object and its data in the checkpoint file/DB.
 *
 * @param CFP (I) [utilized]
 * @param CPType (I)
 * @param OName (I)
 * @param Buf (I)
 */

int MCPStoreObj(

  FILE *CFP,      /* I (utilized) */
  enum MCkptTypeEnum CPType, /* I */
  char *OName,    /* I */
  const char *Buf)      /* I */

  {
  enum MStatusCodeEnum SC;

  mdb_t *MDBInfo;

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((MCP.UseDatabase == TRUE) && (MDBInfo->DBType != mdbNONE))
    {
    mstring_t String = Buf;

    MUStringToSQLString(&String,TRUE);

    MDBInsertCP(MDBInfo,(char *)MCPType[CPType],OName,MSched.Time,String.c_str(),NULL,&SC);

    return(SUCCESS);
    }

  if (CFP == NULL)
    {
    return(FAILURE);
    }

  fprintf(CFP,"%-9s %20s %9ld %s\n",
    MCPType[CPType],
    OName,
    MSched.Time,
    Buf);

  return(SUCCESS);
  }  /* END MCPStoreObj() */





/**
 * Checkpoints the system-wide standing reservations.
 *
 * @param CP (I) [utilized]
 */

int MCPStoreSRList(

  mckpt_t *CP)  /* I (utilized) */

  {
  char     tmpBuf[MMAX_BUFFER];

  int      srindex;
  msrsv_t *SR;

  const char *FName = "MCPStoreSRList";

  MDB(3,fCKPT) MLog("%s(CP)\n",
    FName);

  if (CP == NULL)
    {
    return(FAILURE);
    }

  srindex = 0;

  for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
    {
    SR = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);

    if (SR == NULL)
      break;
    else if (SR == (void *)0x1)
      continue;

    MMBPurge(&SR->MB,NULL,-1,MSched.Time);

    if ((SR->ReqTC == 0) &&
        (SR->HostExp[0] == '\0'))
      {
      MDB(9,fCKPT) MLog("INFO:     skipping empty SR[%02d]\n",
        srindex);

      continue;
      }

    if (SR->IsDeleted == TRUE)
      continue;

    MDB(4,fCKPT) MLog("INFO:     checkpointing SR '%s'\n",
      SR->Name);

    MSRToString(SR,tmpBuf);

    MCPStoreObj(CP->fp,mcpSRsv,SR->Name,tmpBuf);
    }  /* END for (srindex) */

  fflush(CP->fp);

  return(FAILURE);
  }  /* END MCPStoreSRList() */





/**
 *
 *
 * @param CP (I)
 * @param PList (I)
 */

int MCPStoreVPCList(

  mckpt_t  *CP,    /* I */
  marray_t *PList) /* I */

  {
  int pindex;

  mpar_t *P;

  mxml_t *PE;

  mstring_t tmpString;

  for (pindex = 0;pindex < PList->NumItems;pindex++)
    {
    P = (mpar_t *)MUArrayListGet(PList,pindex);
   
    if ((P == NULL) || (P->Name[0] == '\0'))
      break;

    if (P->Name[0] == '\1')
      continue;

    /* only store vpc's and dynamic partitions */

    if (!bmisset(&P->Flags,mpfIsDynamic))
      continue;

    PE = NULL;

    MXMLCreateE(&PE,(char *)MXO[mxoPar]);

    MParToXML(P,PE,NULL);

    tmpString = "";

    if (MXMLToMString(PE,&tmpString,NULL,TRUE) == SUCCESS)
      {
      MCPStoreObj(CP->fp,mcpPar,P->Name,tmpString.c_str());
      }

    MXMLDestroyE(&PE);
    }  /* END for (pindex) */

  return(SUCCESS);
  }  /* END MCPStoreVPCList() */






/**
 * Stores active node information and RM information to the checkpoint file/DB.
 *
 * @param CP (I)
 */

int MCPStoreRMList(
 
  mckpt_t  *CP)
 
  {
  int     rmindex;
 
  mrm_t   *R;
 
  char    tmpBuffer[MMAX_BUFFER*4];  /* make dyanmic */

  const char *FName = "MCPStoreRMList";
 
  MDB(3,fCKPT) MLog("%s(CP)\n",
    FName);

  /* store dynamic rm info */

  MDB(2,fCKPT) MLog("INFO:     storing RM state\n");

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R == NULL)
      break;

    if (R->Name[0] == '\0')
      break;

    MMBPurge(&R->MB,NULL,-1,MSched.Time);

    if (MRMIsReal(R) == FALSE)
      continue;

    if (R->IsDeleted == TRUE)
      continue;

    MDB(4,fCKPT) MLog("INFO:     storing state for RM %s\n",
      R->Name);

    if (MRMStoreCP(R,tmpBuffer,sizeof(tmpBuffer)) == SUCCESS)
      {
      MCPStoreObj(CP->fp,mcpRM,R->Name,tmpBuffer);
      }
    else
      {
      MDB(2,fCKPT) MLog("ALERT:    cannot store checkpoint state for RM %s\n",
        R->Name);
      }
    }  /* END for (rmindex) */
 
  if (CP->fp != NULL)
    fflush(CP->fp);
 
  return(SUCCESS);
  }  /* END MCPStoreRMList() */





/**
 * Stores active node information and RM information to the checkpoint file/DB.
 *
 * @param CP (I)
 * @param NL (I) [optional]
 */

int MCPStoreCluster(
 
  mckpt_t  *CP,  /* I */
  mnode_t **NL)  /* I (optional) */
 
  {
  int     nindex;
 
  mnode_t *N;
 
  char   *ptr;
  char   *tail;


  char    tmpBuffer[MMAX_BUFFER*4];  /* make dyanmic */
  char    Name[MMAX_NAME + 1];
  char    tmpTok[MMAX_NAME];
 
  long    CheckPointTime;

  int     rc;

  const char *FName = "MCPStoreCluster";
 
  MDB(3,fCKPT) MLog("%s(CP,%s)\n",
    FName,
    (NL != NULL) ? "NL" : "NULL");

  if (MSched.LimitedNodeCP == TRUE)
    {
    /* for now, do not checkpoint any node info */

    return(SUCCESS);
    }
 
  /* store active node info */
 
  if (NL != NULL)
    {
    mstring_t NodeLine(MMAX_LINE);

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = NL[nindex];
   
      if ((N == NULL) || (N->Name[0] == '\0'))
        break;
   
      if (N->Name[0] == '\1')
        continue;

      MMBPurge(&N->MB,NULL,-1,MSched.Time);

      if (bmisset(&N->IFlags,mnifIsDeleted))
        continue;

      /* When the checkpoint file is read in, nodes are added for jobs that reference
         a node which does not exist. We should not write these "pseudo" nodes out as
         active nodes. These "old" nodes will be written to the checkpoint file below 
         (in this routine) with the old node records from the checkpoint file. */

      if (!bmisset(&N->IFlags,mnifRMDetected)) /* skip "pseudo" node */
        continue;

      MDB(4,fCKPT) MLog("INFO:     checkpointing node '%s'\n",
        N->Name);
   
      NodeLine.clear();   /* clear the current string from NodeLine */

      if (MNodeToString(N,&NodeLine) == FAILURE)
        {
        MDB(4,fCKPT) MLog("INFO:     cannot checkpoint node '%s'\n",
          N->Name);
   
        continue;
        }
   
      MCPStoreObj(CP->fp,mcpNode,N->Name,NodeLine.c_str());
      }  /* END for (nindex) */
    }    /* END if (NL != NULL)  */
 
  /* maintain old node CP info */

  /* NOTE:  should only perform this action at start up, 
            i.e., load stale info into internal buffer and dump it out
            en-masse at ckpt create time */

  if ((CP->OBuffer != NULL) && (CP->OldNodeBufferIsInitialized == FALSE))
    {
    MDB(3,fCKPT) MLog("INFO:     checkpointing old node data (%d byte buffer)\n",
      (int)((CP->OBuffer != NULL) ? strlen(CP->OBuffer) : 0));

    CP->OldNodeBuffer = "";

    /* locate old node checkpoint objects */

    sprintf(tmpTok,"\n%s ",
      MCPType[mcpNode]);

    ptr = CP->OBuffer;
 
    while ((ptr = strstr(ptr,tmpTok)) != NULL)
      {
      /* move beyond newline */

      ptr++;

      rc = sscanf(ptr,"%2048s %64s %ld",
        tmpBuffer,
        Name, 
        &CheckPointTime);

      if (rc != 3)
        {
        MDB(2,fCKPT) MLog("INFO:     corrupt node line detected (%.128s)\n",
          (ptr != NULL) ? ptr : "NULL");

        ptr += strlen(tmpTok);

        continue;
        }
 
      if (((long)MSched.Time - CheckPointTime) > (long)CP->CPExpirationTime)
        {
        char TString[MMAX_LINE];

        MULToTString(MSched.Time - CheckPointTime,TString);

        MDB(2,fCKPT) MLog("INFO:     expiring checkpoint data for node %s.  not updated in %s\n",
          (Name != NULL) ? Name : "NULL",
          TString);
 
        ptr += strlen(tmpTok);
 
        continue;
        }

      if ((MNodeFind(Name,&N) == SUCCESS) && (bmisset(&N->IFlags,mnifRMDetected)))
        {
        /* current node info already stored */

        ptr += strlen(tmpTok);
 
        continue;
        }
 
      if ((tail = strchr(ptr,'\n')) == NULL)
        {
        MDB(1,fCKPT) MLog("ALERT:    checkpoint file corruption at offset %d\n",
          (int)(ptr - CP->OBuffer));
 
        ptr += strlen(tmpTok);
 
        continue;
        }
 
      /* copy old data */
 
      MUStrCpy(tmpBuffer,ptr,MIN((int)sizeof(tmpBuffer),tail - ptr + 1));
 
      /* Added this instance to the 'string' */
      CP->OldNodeBuffer = tmpBuffer;
      CP->OldNodeBuffer += "\n";

      ptr = tail;
      }  /* END while (ptr = strstr()) */

    CP->OldNodeBufferIsInitialized = TRUE;
    }    /* END if ((CP->OBuffer != NULL) && (CP->OldNodeBufferIsInitialized == FALSE)) */

  if ((CP->OldNodeBuffer.c_str() != NULL) && 
      (!CP->OldNodeBuffer.empty()) && 
      (CP->fp != NULL))
    {
    fprintf(CP->fp,"%s\n",
      CP->OldNodeBuffer.c_str());
    }

  if (CP->fp != NULL)
    fflush(CP->fp);
 
  return(SUCCESS);
  }  /* END MCPStoreCluster() */





/**
 * Report if Moab can support the specified checkpoint protocol version.
 *
 * @param CP
 * @param Version
 */

mbool_t MCPIsSupported(

  mckpt_t *CP,
  char    *Version)

  {
  if ((CP == NULL) || (Version == NULL))
    {
    return(FALSE);
    }

  if (strstr(CP->SVersionList,Version))
    {
    return(TRUE);
    }

  return(FALSE);
  }  /* END MCPIsSupported() */





/**
 * Load checkpointed scheduler info.
 *
 * @see MSchedFromString() - child
 *
 * @param CP   (I)
 * @param Line (I)
 * @param S    (I) [modified]
 */

int MCPLoadSched(
 
  mckpt_t  *CP,
  const char     *Line,
  msched_t *S)
 
  {
  char   TempName[MMAX_NAME + 1];
  char   tmpLine[MMAX_LINE + 1];
  long   CkTime;

  const char  *ptr;

  const char *FName = "MCPLoadSched";
 
  MDB(4,fCKPT) MLog("%s(CP,Line,S)\n",
    FName);
 
  /* FORMAT:   TY TM Name DATA */ 
 
  sscanf(Line,"%64s %64s %ld %1024s",
    TempName,
    TempName,
    &CkTime,
    tmpLine);

  if (MCP.LastCPTime <= 0)
    MCP.LastCPTime = CkTime;

  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    MDB(2,fCKPT) MLog("WARNING:  checkpoint file has expired\n");
    }

  if ((ptr = strchr(Line,'<')) == NULL)
    {
    return(FAILURE);
    }
 
  MSchedFromString(S,ptr);

  /* If we are starting up and moab was started with the "moab -s" command
   * then use the restart state stored in the checkpoint file. */

  if (S->UseCPRestartState)
    {
    switch(S->RestartState)
      {
      case mssmNONE:
      case mssmShutdown:

        S->UseCPRestartState = FALSE;

        MDB(2,fCKPT) MLog("INFO:     CP Restart State %s ignored\n",
          MSchedState[S->RestartState]);

        break;

      default:

        /* All other states - Paused, Stopped, Running, etc */

        S->State = S->RestartState;

        MDB(2,fCKPT) MLog("INFO:     Starting scheduler using CP Restart State %s\n",
          MSchedState[S->RestartState]);

        break;
      } /* END switch(S->RestartState) */
    } /* END if (S->UseCPRestartState) */

  if (S->LocalQueueIsActive == TRUE)
    {
    MRMInitializeLocalQueue();
    }
 
  return(SUCCESS);
  }  /* END MCPLoadSched() */





/**
 *
 *
 * @param CP (I)
 * @param Line (I)
 * @param S (I) [modified]
 */

int MCPLoadSys(

  mckpt_t  *CP,    /* I */
  const char     *Line,  /* I */
  msched_t *S)     /* I (modified) */

  {
  char   TempName[MMAX_NAME + 1];
  char   tmpLine[MMAX_LINE + 1];
  long   CkTime;

  const char  *ptr;

  const char *FName = "MCPLoadSys";

  MDB(4,fCKPT) MLog("%s(CP,Line,S)\n",
    FName);

  /* FORMAT:   TY TM Name DATA */

  sscanf(Line,"%64s %ld %1024s",
    TempName,
    &CkTime,
    tmpLine);

  if ((ptr = strchr(Line,'<')) == NULL)
    {
    return(FAILURE);
    }

  MOFromString((void *)S,mxoSched,ptr);

  return(SUCCESS);
  }  /* END MCPLoadSys() */





/**
 * Store checkpoint records for specified credential table.
 *
 * @see MCPCreate() - parent
 * @see MCredToString() - child
 *
 * @param CP (I)
 * @param OID (I) [optional]
 * @param OIndex (I)
 */

int MCPStoreCredList(

  mckpt_t            *CP,      /* I */
  char               *OID,     /* I (optional) */
  enum MXMLOTypeEnum  OIndex)  /* I */
 
  {
  void    *O;
  void    *OP;

  void    *OE;

  enum MCkptTypeEnum CPType;

  char    *NameP;
 
  mhashiter_t HTI;

  const char *FName = "MCPStoreCredList";
 
  MDB(3,fCKPT) MLog("%s(CP,%s,%s)\n",
    FName,
    (OID != NULL) ? OID : "NULL",
    MXO[OIndex]);

  switch (OIndex)
    {
    case mxoUser:  CPType = mcpUser;  break;
    case mxoGroup: CPType = mcpGroup; break;
    case mxoAcct:  CPType = mcpAcct;  break;
    case mxoClass: CPType = mcpClass; break;
    case mxoQOS:   CPType = mcpQOS;   break;

    default:  

      /* object not supported */

      return(FAILURE);

      break;
    }  /* END switch (OIndex) */
 
  /* store object info */

  MOINITLOOP(&OP,OIndex,&OE,&HTI);

  while ((O = MOGetNextObject(&OP,OIndex,OE,&HTI,&NameP)) != NULL)
    {
    if ((OID != NULL) && strcmp(OID,NONE))
      {
      if (strcmp(OID,NameP))
        continue;
      }

    switch (OIndex)
      {
      case mxoGroup:
      case mxoUser:
      case mxoAcct:

        MMBPurge(&((mgcred_t *)O)->MB,NULL,-1,MSched.Time);

        if (((mgcred_t *)O)->IsDeleted == TRUE)
          continue;

        /* NOT REACHED */

        break;

      case mxoQOS:

        MMBPurge(&((mqos_t *)O)->MB,NULL,-1,MSched.Time);

        if (((mqos_t *)O)->IsDeleted == TRUE)
          continue;

        break;

      case mxoClass:

        MMBPurge(&((mclass_t *)O)->MB,NULL,-1,MSched.Time);

        if (((mclass_t *)O)->IsDeleted == TRUE)
          continue;

        break;

      default:

        /* not supported */

        return(FAILURE);

        break;
      }  /* END switch (OIndex) */

    MDB(4,fCKPT) MLog("INFO:     checkpointing %s:%s\n",
      MXO[OIndex],
      NameP);

    /* MCredToString expects a MMAX_BUFFER sized array as last param */
 
    mstring_t String(MMAX_LINE);

    MCredToString(O,OIndex,TRUE,&String);
 
    MCPStoreObj(CP->fp,CPType,NameP,String.c_str());
    }  /* END for (uindex) */

  return(SUCCESS);
  }  /* END MCPStoreCredList() */






/**
 * Report grid (ie, Matrix) stats to checkpoint store.
 *
 * @see MCPCreate() - parent
 *
 * @param CP (I) [utilized]
 */

int MCPWriteGridStats(

  mckpt_t            *CP)

  {
  int mindex;
  int rindex;
  int cindex;

  char tmpName[MMAX_NAME];
  char tmpBuf[MMAX_BUFFER + 1];

  FILE *ckfp = CP->fp;

  const char *FName = "MCPWriteGridStats";

  MDB(3,fCKPT) MLog("%s(ckfp)\n",
    FName);

  /* global total stats */

  MStatToString(
    &MPar[0].S,
    tmpBuf,      /* O */
    (enum MStatAttrEnum *)DefCPStatAList);

  snprintf(tmpName,sizeof(tmpName),"%d:%d:%d",
    -1,
    -1,
    0);

  MCPStoreObj(ckfp,mcpTotal,tmpName,tmpBuf);

  /* Par: mindex  Time: cindex  Proc: rindex */

  /* FORMAT:  <RINDEX>:<CINDEX>[:<MINDEX>] */

  for (mindex = 0;mindex < MMAX_GRIDDEPTH + 1;mindex++)
    {
    for (rindex = 0;rindex <= (int)MStat.P.ProcStepCount;rindex++)
      {
      /* step through all rows */

      for (cindex = 0;cindex <= (int)MStat.P.TimeStepCount;cindex++)
        {
        if (rindex == 0)
          {
          /* column total */

          MStatToString(
            &MStat.SMatrix[mindex].CTotal[cindex],
            tmpBuf,  /* O */
            (enum MStatAttrEnum *)DefCPStatAList);

          snprintf(tmpName,sizeof(tmpName),"%d:%d:%d",
            -1,
            cindex, 
            mindex);

          MCPStoreObj(ckfp,mcpCTotal,tmpName,tmpBuf);
          }

        /* step through all columns */

        MDB(7,fCKPT) MLog("INFO:     checkpointing Grid[%d][%02d][%02d]\n",
          mindex,
          cindex,
          rindex);

        MStatToString(
          &MStat.SMatrix[mindex].Grid[cindex][rindex],
          tmpBuf,  /* O */
          (enum MStatAttrEnum *)DefCPStatAList);

        snprintf(tmpName,sizeof(tmpName),"%d:%d:%d",
          rindex,
          cindex,
          mindex);

        MCPStoreObj(ckfp,mcpGTotal,tmpName,tmpBuf);
        }    /* END for (cindex) */

      /* row total */

      MStatToString(
        &MStat.SMatrix[mindex].RTotal[rindex],
        tmpBuf,  /* O */
        (enum MStatAttrEnum *)DefCPStatAList);

      snprintf(tmpName,sizeof(tmpName),"%d:%d:%d",
        rindex,
        -1,
        mindex);

      MCPStoreObj(ckfp,mcpRTotal,tmpName,tmpBuf);
      }  /* END for (rindex) */
    }    /* END for (mindex) */

  return(SUCCESS);
  }  /* END MCPWriteGridStats() */





/**
 * Load matrix stats from checkpoint data store.
 *
 * @param Buf (I)
 */

int MCPLoadStats(

  char *Buf)  /* I */

  {
  char tmpName[MMAX_NAME + 1];
  char tmpMID[MMAX_NAME + 1];

  int  MIndex = 0;
  int  RIndex = -1;
  int  CIndex = -1;
  
  long CkTime;

  int  rc;

  char *ptr;
  char *TokPtr = NULL;

  must_t *S;

  const char *FName = "MCPLoadStats";

  MDB(4,fCKPT) MLog("%s(%.32s)\n",
    FName,
    (Buf != NULL) ? Buf : "NULL");

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  if ((rc = sscanf(Buf,"%32s %32s %ld ",
      tmpName,
      tmpMID,
      &CkTime)) != 3)
    {
    return(FAILURE);
    }

  /* FORMAT:  <RINDEX>:<CINDEX>[:<MINDEX>] */

  ptr = MUStrTok(tmpMID,":",&TokPtr);

  if (ptr != NULL)
    RIndex = (int)strtol(ptr,NULL,10);

  ptr = MUStrTok(NULL,":",&TokPtr);

  if (ptr != NULL)
    CIndex = (int)strtol(ptr,NULL,10);

  ptr = MUStrTok(NULL,":",&TokPtr);

  if (ptr != NULL)
    MIndex = (int)strtol(ptr,NULL,10);

  if ((MSched.Time - CkTime) > MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  if (RIndex >= MMAX_SCALE)
    {
    return(FAILURE);
    }

  if (CIndex >= MMAX_SCALE)
    {
    return(FAILURE);
    }

  if ((ptr = strchr(Buf,'<')) == NULL)
    {
    return(FAILURE);
    }

  if (RIndex == -1)
    {
    if (CIndex == -1)
      {
      /* global total */
 
      S = &MPar[0].S;
      }
    else
      {
      /* column total */

      S = &MStat.SMatrix[MIndex].CTotal[CIndex];
      }
    }
  else if (CIndex == -1)
    {
    /* row total */

    S = &MStat.SMatrix[MIndex].RTotal[RIndex];
    }
  else
    {
    /* grid */

    S = &MStat.SMatrix[MIndex].Grid[CIndex][RIndex];
    }
       
  if (MStatFromString(ptr,(void *)S,msoCred) == FAILURE)
    {
    MDB(2,fCKPT) MLog("INFO:    cannot read stat index for location %d:%d:%d\n",
      MIndex,
      CIndex,
      RIndex);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCPLoadStats() */




           
/**
 *
 *
 * @param CP (I) [utilized]
 */

int MCPWriteSystemStats(

  mckpt_t            *CP)

  {
  char Buffer[MMAX_LINE << 2];

  FILE *ckfp = CP->fp;

  must_t *T;

  const char *FName = "MCPWriteSystemStats";
 
  MDB(3,fCKPT) MLog("%s(ckfp)\n",
    FName);

  /* must restore all statistics required to maintain stats */

  T = &MPar[0].S;

  /*            LABL TIME PHAVAIL PHBUSY  ITM CT SJ MSA MSD NJA JAC MXF MB MQT TXF TB TQT PSR PSD PSU JE SPH WEF WIT */

  snprintf(Buffer,sizeof(Buffer),"%10.3f %10.3f %ld %d %d %f %f %f %f %f %d %ld %f %d %ld %f %f %f %d %f %f %d %10.3f\n",
    MStat.TotalPHAvailable,
    MStat.TotalPHBusy,
    MStat.InitTime,
    T->JC,
    MStat.SuccessfulJobsCompleted,
    T->MSAvail,
    T->MSDedicated,
    T->NJobAcc,
    T->JobAcc,
    T->MaxXFactor,
    T->MaxBypass,
    T->MaxQTS,
    T->PSXFactor,
    T->Bypass,
    T->TotalQTS,
    T->PSRun,
    T->PSDedicated,
    T->PSUtilized,
    MStat.EvalJC,
    MStat.SuccessfulPH,
    MStat.MinEff,
    MStat.MinEffIteration,
    MStat.TotalPHDed);

  MCPStoreObj(ckfp,mcpSysStats,(char *)MCPType[mcpSysStats],Buffer);

  return(SUCCESS);
  }  /* END MCPWriteSystemStats() */





/**
 * Load checkpointed partition attributes.
 *
 * @param Line (I)
 */

int MCPLoadPar(

  char *Line)  /* I */

  {
  mpar_t *C;

  mpar_t  tmpP = {{0}};

  char    tmp[MMAX_LINE + 1];

  long    CkTime;
  long    tmpL;

  char   *ptr;

  mxml_t *PE;

  const char *FName = "MCPLoadPar";

  MDB(4,fCKPT) MLog("%s(%s)\n",
    FName,
    Line);

  memset(&tmpP,0,sizeof(mpar_t));

  sscanf(Line,"%1024s %64s %ld",
    tmp,
    tmpP.Name,
    &CkTime);

  tmpL = (long)MSched.Time - CkTime;

  if (tmpL > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  if ((ptr = strchr(Line,'<')) == NULL)
    {
    return(FAILURE);
    }

  PE = NULL;

  if (MXMLFromString(&PE,ptr,NULL,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  if (MXMLGetAttr(PE,(char *)MParAttr[mpaPurgeTime],NULL,NULL,0) == TRUE)
    {
    /* create VPC */

    if (MVPCAdd(tmpP.Name,&C) == FAILURE)
      {
      MXMLDestroyE(&PE);

      return(FAILURE);
      }

    bmset(&C->Flags,mpfIsDynamic);
    }
  else if (MParAdd(tmpP.Name,&C) == FAILURE)
    {
    MXMLDestroyE(&PE);

    return(FAILURE);
    }

  MParFromXML(C,PE);

  MXMLDestroyE(&PE);

  return(SUCCESS);
  }  /* END MCPLoadPar() */





/**
 * Load standing reservation attributes from checkpoint.
 *
 * @param Line (I)
 */

int MCPLoadSR(

  char *Line)  /* I */

  {
  int      srindex;

  msrsv_t *SR;
  msrsv_t  tmpSR;

  char    *ptr;
  char     tmp[MMAX_LINE + 1];

  long     CkTime;
  mbool_t  FoundSR = FALSE;

  const char *FName = "MCPLoadSR";

  MDB(4,fCKPT) MLog("%s(%s)\n",
    FName,
    Line);

  SR = &tmpSR;

  memset(SR,0,sizeof(msrsv_t));

  sscanf(Line,"%64s %64s %ld",
    tmp,
    SR->Name,
    &CkTime);

  if ((MSched.Time - CkTime) > MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  if ((ptr = strchr(Line,'<')) == NULL)
    {
    return(FAILURE);
    }

  MSRFromString(SR,ptr);

  /* must locate existing standing reservation */

  for (srindex = 0;srindex < MSRsv.NumItems;srindex++)
    {
    SR = (msrsv_t *)MUArrayListGet(&MSRsv,srindex);

    if (SR == NULL)
      break;
    else if (SR == (void *)0x1)
      continue;

    if ((SR->ReqTC == 0) &&
        (SR->HostExp[0] == '\0'))
      {
      MDB(7,fCKPT) MLog("INFO:     skipping empty SR[%02d]\n",
        srindex);

      continue;
      }

    if (!strcmp(SR->Name,tmpSR.Name))
      {
      FoundSR = TRUE;

      break;
      }
    }  /* END while (SR) */

  if (FoundSR == FALSE)
    {
    MDB(7,fCKPT) MLog("INFO:     cannot locate checkpointed sres '%s', ignoring\n",
      tmpSR.Name);

    return(SUCCESS);
    }

  memcpy(&SR->DisabledTimes,tmpSR.DisabledTimes,sizeof(mulong) * MMAX_SRSV_DEPTH);

  SR->IdleTime  = tmpSR.IdleTime;
  SR->TotalTime = tmpSR.TotalTime;

  return(SUCCESS);
  }  /* END MCPLoadSR() */





/**
 *
 *
 * @param Line (I)
 */

int MCPLoadSysStats(

  char *Line)  /* I */

  {
  char   tmpName[MMAX_NAME + 1];

  must_t  tmpG;
  mstat_t tmpS;

  long    InitTime;

  long   CkTime;

  must_t *T;
  must_t *S;

  time_t tmpTime;

  const char *FName = "MCPLoadSysStats";

  MDB(4,fCKPT) MLog("%s(%s)\n",
    FName,
    Line);

  memset(&tmpG,0,sizeof(tmpG));
  memset(&tmpS,0,sizeof(tmpS));

  T = &tmpG;

  time(&tmpTime);

  /* must restore all statistics required to maintain ShowStats */

  /*         LABL TME PHA PHB ITM CT SJ MSA MSD NJA JAC MXF MB MQT TXF TB TQT PSR PSD PSU JE SPH WEF WIT */

  /* NOTE:  older CP files will not report TotalPHDed */

  tmpS.TotalPHDed = 0.0;

  sscanf(Line,"%64s %64s %ld %lf %lf %ld %d %d %lf %lf %lf %lf %lf %d %lu %lf %d %lu %lf %lf %lf %d %lf %lf %d %lf\n",
    tmpName,
    tmpName,
    &CkTime,
    &tmpS.TotalPHAvailable,
    &tmpS.TotalPHBusy,
    &InitTime,
    &T->JC,
    &tmpS.SuccessfulJobsCompleted,
    &T->MSAvail,
    &T->MSDedicated,
    &T->NJobAcc,
    &T->JobAcc,
    &T->MaxXFactor,
    &T->MaxBypass,
    &T->MaxQTS,
    &T->PSXFactor,
    &T->Bypass,
    &T->TotalQTS,
    &T->PSRun,
    &T->PSDedicated,
    &T->PSUtilized,
    &tmpS.EvalJC,
    &tmpS.SuccessfulPH,
    &tmpS.MinEff,
    &tmpS.MinEffIteration,
    &tmpS.TotalPHDed);

  if ((MSched.Time - CkTime) > MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  S = &MPar[0].S;

  S->JC           = T->JC;
  S->MSAvail      = T->MSAvail;
  S->MSDedicated  = T->MSDedicated;
  S->NJobAcc      = T->NJobAcc;
  S->JobAcc       = T->JobAcc;
  S->MaxXFactor   = T->MaxXFactor;
  S->MaxBypass    = T->MaxBypass;
  S->MaxQTS       = T->MaxQTS;
  S->PSRun        = T->PSRun;
  S->PSDedicated  = T->PSDedicated;
  S->PSUtilized   = T->PSUtilized;

  MStat.InitTime         = InitTime;
  MStat.TotalPHAvailable = tmpS.TotalPHAvailable;
  MStat.TotalPHBusy      = tmpS.TotalPHBusy;
  MStat.TotalPHDed       = tmpS.TotalPHDed;
  MStat.EvalJC           = tmpS.EvalJC;
  MStat.SuccessfulPH     = tmpS.SuccessfulPH;
  MStat.MinEff           = tmpS.MinEff;
  MStat.MinEffIteration  = tmpS.MinEffIteration;

  MStat.SuccessfulJobsCompleted = tmpS.SuccessfulJobsCompleted;

  return(SUCCESS);
  }  /* END MCPLoadSysStats() */




/**
 *
 *
 * @param CP (I) [utilized]
 */

int MCPWriteTList(

  mckpt_t            *CP)

  {
  FILE *CFP = CP->fp;

  mtrig_t *T;

  mxml_t  *TE;

  char     tmpLine[MMAX_LINE];

  int      tindex;

  /* MTList only contains event based triggers (non-threshold) */

  for (tindex = 0;tindex < MTList.NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

    if (MTrigIsReal(T) == FALSE)
      continue;

    if (T->EType == mttStanding)
      continue;
   
    if ((T->OType == mxoSched) && !bmisset(&T->SFlags,mtfCheckpoint))
      {
      /* do not checkpoint scheduler triggers until we can compare checkpointed triggers against configured triggers */

      continue;
      }

    if ((T->OType == mxoJob) || (T->OType == mxoRM) || (T->OType == mxoNode) || (T->OType == mxoClass))
      {
      if (!bmisset(&T->SFlags,mtfCheckpoint))
        {
        /* NOTE: checkpointing node triggers causes duplicates from the moab.cfg file */

        continue;
        }
      }

    if ((T->OType == mxoJob) &&
        (MTrigIsGenericSysJob(T) == FALSE) &&
        (MSched.FullJobTrigCP == FALSE) &&
        !bmisset(&T->SFlags,mtfCheckpoint))
      {
      /* Don't generally checkpoint job triggers (but always checkpoint generic
          system job triggers */

      continue;
      }

    TE = NULL;

    if (MXMLCreateE(&TE,(char *)MXO[mxoTrig]) == FAILURE)
      {
      return(FAILURE);
      }

    if ((MTrigToXML(T,TE,NULL) == SUCCESS) &&
        (MXMLToString(TE,tmpLine,sizeof(tmpLine),NULL,TRUE) == SUCCESS))
      {
      MCPStoreObj(CFP,mcpTrigger,T->TName,tmpLine);
      }

    MXMLDestroyE(&TE);
    }  /* END for (tindex) */

  return(SUCCESS);
  }  /* END MCPWriteTList() */





/**
 * Store checkpoint information for completed jobs.
 *
 * @see MCJobStoreCP() - child - create completed job ckpt record
 * @see MCPStoreQueue() - peer - checkpoint non-completed jobs
 * @see MCPCreate() - parent
 *
 * @param CP (I)
 */

int MCPStoreCJobs(
 
  mckpt_t  *CP)

  {
  job_iter JTI;

  mjob_t *J;

  mstring_t String(MMAX_LINE);

  char tmpID[MMAX_NAME];
  char tmpName[MMAX_NAME];

  mbool_t FullCP;

  const char *FName = "MCPStoreCJobs";
 
  MDB(3,fCKPT) MLog("%s(CP)\n",
    FName);

  if (CP == NULL)
    {
    return(FAILURE);
    }

  /* for now always store full CP */

  FullCP = TRUE;

  MCJobIterInit(&JTI);

  while (MCJobTableIterate(&JTI,&J) == SUCCESS)
    {
    MMBPurge(&J->MessageBuffer,NULL,-1,MSched.Time);

    MDB(4,fCKPT) MLog("INFO:     checkpointing completed job '%s'\n",
      J->Name);

    String.clear();  /* Reset the string on every iteration */

    /* create individual job XML */

    if (MCJobStoreCP(J,&String,FullCP) == FAILURE)
      {
      MDB(4,fCKPT) MLog("INFO:     Unable to create checkpoint record for completed job '%s'\n",
        J->Name);

      continue;
      }

    /* workload checkpointing */

    /* extract RM name */

    if (J->DRMJID != NULL)
      MUStrCpy(tmpID,J->DRMJID,sizeof(tmpID));
    else
      MUStrCpy(tmpID,J->Name,sizeof(tmpID));

    MJobGetName(NULL,tmpID,J->DestinationRM,tmpName,sizeof(tmpName),mjnShortName);

    if (MCPStoreObj(CP->fp,mcpCJob,tmpName,String.c_str()) == FAILURE)
      {
      MDB(4,fCKPT) MLog("INFO:     completed job '%s' to checkpoint file write failed\n",
        J->Name);
      }
    else
      {
      MDB(4,fCKPT) MLog("INFO:     completed job '%s' written to checkpoint file\n",
        J->Name);

      if ((J->DestinationRM != NULL) &&
          (J->DestinationRM->Type == mrmtPBS))
        {
        /* record that this completed job was successfully checkpointed so we can tell TORQUE
         * to remove the job from its memory */

        mrm_t *R = J->DestinationRM;

        R->PBS.LastCPCompletedJobTime = MAX(J->RMCompletionTime,R->PBS.LastCPCompletedJobTime);
        }
      }
    }  /* END for (jindex) */

  return(SUCCESS);
  }  /* END MCPStoreCJobs() */




/**
 *  Checkpoints VMs from a hash table.
 *
 * @param CP            [I] (modified) - The checkpoint file to checkpoint to
 * @param VML           [I] - The hash table with VMs.
 * @param CompletedVMs  [I] - TRUE if VMs are completed VMs
 */

int MCPStoreVMs(

  mckpt_t  *CP,           /* I */
  mhash_t  *VML,          /* I */
  mbool_t   CompletedVMs) /* I */

  {
  mhashiter_t HTI;
  char tmpLine[MMAX_LINE << 2];
  mvm_t *VM;
  enum MCkptTypeEnum CPType;

  const char *FName = "MCPStoreCVMs";

  MDB(3,fCKPT) MLog("%s(CP,VML)\n",
    FName);

  if (CP == NULL)
    return(FAILURE);

  if (VML == NULL)
    return(SUCCESS);

  MUHTIterInit(&HTI);

  CPType = mcpVM;

  if (CompletedVMs)
    CPType = mcpCVM;

  while (MUHTIterate(VML,NULL,(void **)&VM,NULL,&HTI) == SUCCESS)
    {
    mxml_t *VE = NULL;

    if (VM == NULL)
      continue;

    if (VM->VMID[0] == '\0')
      continue;

    MDB(4,fCKPT) MLog("INFO:     checkpointing completed vm '%s'\n",
      VM->VMID);

    MXMLCreateE(&VE,(char *)MXO[mxoxVM]);

    MVMToXML(VM,VE,NULL);

    MXMLToString(VE,tmpLine,sizeof(tmpLine),NULL,TRUE);

    MCPStoreObj(CP->fp,CPType,VM->VMID,tmpLine);

    MXMLDestroyE(&VE);
    }

  return(SUCCESS);
  }  /* END MCPStoreVMs() */



mbool_t MCPIsCorrupt()

  {
  mbool_t IsExtant;
  mbool_t IsCorrupt;

  /* routine disabled */

  return(FALSE);

  MCPValidate(MCP.CPFileName,&IsCorrupt,&IsExtant);

  if ((IsExtant == FALSE) || (IsCorrupt == TRUE))
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"%s.1",
      MCP.CPFileName);

    MCPValidate(tmpLine,&IsCorrupt,&IsExtant);

    if (IsCorrupt == TRUE)
      {
      return(TRUE);
      }
    }
 
  return(FALSE);
  }  /* END MCPIsCorrupt() */





/**
 *
 *
 * @param FileName
 * @param IsCorrupt
 * @param IsExtant
 */

int MCPValidate(
  
  char    *FileName,
  mbool_t *IsCorrupt,
  mbool_t *IsExtant)  

  {
  FILE        *dfp = NULL;
  char        *ptr;

  int ReadCount;
  int BlockSize = 1;
  int tmpBufSize;
  int count;

  struct stat sbuf;

  *IsCorrupt = FALSE;
  *IsExtant  = TRUE;

  if ((FileName[0] == '\0') ||
      (stat(FileName,&sbuf) == -1))
    {
    *IsExtant = FALSE;

    return(SUCCESS);
    }

  if ((dfp = fopen(FileName,"r")) == NULL)
    {
    *IsExtant = FALSE;

    return(SUCCESS);
    }

  ReadCount = sbuf.st_size / BlockSize;

  tmpBufSize = ReadCount * BlockSize;

  if ((ptr = (char *)MUMalloc(tmpBufSize + 1)) == NULL)
    {
    *IsExtant = FALSE;

    fclose(dfp);
    return(SUCCESS);
    }

  if ((count = (int)fread(ptr,ReadCount,BlockSize,dfp)) < 0)
    {
    fclose(dfp);
    return(FALSE);
    }

  ptr[tmpBufSize] = '\0';

  if ((strstr(ptr,MCONST_CPTERMINATOR) == NULL) &&
      (strstr(ptr,MCONST_CPTERMINATOR2) == NULL))
    {
    *IsCorrupt = TRUE;
    }

  fclose(dfp);

  dfp = NULL;

  MUFree(&ptr);

  return(SUCCESS);
  }  /* END MCPValidate() */





/**
 * Store template jobs to the checkpoint file.
 *
 * @param CP (I) 
 *
 */

int MCPStoreTJobs(

  mckpt_t  *CP)        /* I */

  {
  int jindex;

  mjob_t *TJ;

  char    tmpBuf[MMAX_BUFFER << 2];  /* large for full checkpointing */

  const char *FName = "MCPStoreTJobs";
 
  MDB(3,fCKPT) MLog("%s(CP)\n",
    FName);

  /* TODO: add support for ODBC */

  if (CP->fp == NULL)
    {
    return(SUCCESS);
    }

  for (jindex = 0;MUArrayListGet(&MTJob,jindex) != NULL;jindex++)
    {
    TJ = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));

    if (TJ == NULL)
      break;

    if ((TJ->ExtensionData == NULL) &&
        (TJ->TemplateExtensions == NULL))
      continue;

    if (bmisset(&TJ->IFlags,mjifTemplateIsDynamic))
      continue;

    if (bmisset(&TJ->IFlags,mjifTemplateIsDeleted))
      continue;

    if (MTJobStoreCP(TJ,tmpBuf,sizeof(tmpBuf),TRUE) == FAILURE)
      {
      continue;
      }

    fprintf(CP->fp,"%-9s %20s %9ld %s\n",
      MCPType[mcpTJob],
      TJ->Name,
      MSched.Time,
      tmpBuf);
    }

  return(SUCCESS);
  }  /* END MCPStoreTJobs() */





/**
 * Loads all the checkpoint files found in Moab's spool directory, creating a
 * job/node for each checkpoint file. If a job already exists in memory, and a
 * corresponding checkpoint file also exists, the checkpoint file will NOT be
 * loaded! This function only creates jobs and loads checkpoint data if the
 * jobs do not already exist in Moab's memory.
 */

int MCPLoadAllSpoolCP()

  {
  DIR *DirHandle;
  struct dirent *File;

  char tmpPath[MMAX_PATH];
  char tmpEMsg[MMAX_LINE];
  char JobName[MMAX_NAME];

  mjob_t *J;

  mrm_t *InternalRM;

  mdb_t *MDBInfo;

  const char *FName = "MCPLoadAllSpoolCP";

  MDB(3,fCKPT) MLog("%s()\n",
    FName);

  /* ensure MSched.Internal is initialized */

  MRMGetInternal(&InternalRM);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
    {
    int *TaskList = NULL;

    /* open directory for reading */

    DirHandle = opendir(MSched.CheckpointDir);

    if (DirHandle == NULL)
      {
      return(FAILURE);
      }

    File = readdir(DirHandle);

    TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

    while (File != NULL)
      {
      /* find checkpoint files to load */

      if (MUStrCmpReverse(".cp",File->d_name))
        {
        File = readdir(DirHandle);

        continue;
        }

      snprintf(tmpPath,sizeof(tmpPath),"%s/%s",
        MSched.CheckpointDir,
        File->d_name);

      /* remove .cp from file name to extract the name of the job */

      MUStrCpy(JobName,File->d_name,sizeof(JobName));

      MUStrRemoveFromEnd(JobName,".cp");

      /* see if job already exists */

      TaskList[0] = -1;

      if (MJobFind(JobName,&J,(MSched.LimitedJobCP == TRUE) ? mjsmBasic: mjsmExtended) == FAILURE)
        {
        if (MJobCreate(JobName,TRUE,&J,NULL) == FAILURE)
          {
          /* cannot create any more jobs!? */

          File = readdir(DirHandle);

          continue;
          }

        MRMJobPreLoad(J,JobName,InternalRM);

        /* load job from checkpoint file */

        if (MRMJobPostLoad(J,TaskList,InternalRM,tmpEMsg) == FAILURE)
          {
          /* invalid job--insufficient data recorded in checkpoint file to instantiate job */

          MDB(2,fCKPT) MLog("ALERT:    invalid job %s in internal queue (removing job: %s)\n",
            J->Name,
            tmpEMsg);

          /* NOTE:  complete job rather than remove it (NYI) */

          MJobRemove(&J);

          File = readdir(DirHandle);

          continue;
          }

        bmunset(&J->IFlags,mjifShouldCheckpoint);

        /* if the destination RM is another moab and the connection to that moab is down then
           transition the job so that it is in the cache. */

        if ((J->DestinationRM != NULL) && 
            (J->DestinationRM->Type == mrmtMoab) &&
            (J->DestinationRM->State == mrmsDown))
          {
          MJobTransition(J,FALSE,FALSE);
          }

        MS3AddLocalJob(InternalRM,J->Name);
        }

      File = readdir(DirHandle);
      }

    MUFree((char **)&TaskList);

    closedir(DirHandle);
    }  /* END if (MDBInfo->DBType == mdbNONE) */
  else
    {
    int *TaskList = NULL;

    marray_t JobNames;
    char **tmpList;
    int jindex;

    MUArrayListCreate(&JobNames,sizeof(char *),-1);

    if (MDBQueryInternalJobsCP(MDBInfo,-1,&JobNames,tmpEMsg) == FAILURE)
      {
      MUArrayListFree(&JobNames);

      return(FAILURE);
      }

    TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

    tmpList = (char **)JobNames.Array;

    for (jindex = 0;jindex < JobNames.NumItems;jindex++)
      {
      /* see if job already exists */

      TaskList[0] = -1;

      if (MJobFind(tmpList[jindex],&J,mjsmExtended) == FAILURE)
        {
        if (MJobCreate(tmpList[jindex],TRUE,&J,NULL) == FAILURE)
          {
          /* cannot create any more jobs!? */

          continue;
          }

        MRMJobPreLoad(J,tmpList[jindex],NULL);

        /* load job from checkpoint file */

        if (MRMJobPostLoad(J,TaskList,InternalRM,tmpEMsg) == FAILURE)
          {
          /* invalid job--insufficient data recorded in checkpoint file to instantiate job */

          MDB(2,fCKPT) MLog("ALERT:    invalid job %s in internal queue (removing job: %s)\n",
            J->Name,
            tmpEMsg);

          /* NOTE:  complete job rather than remove it (NYI) */

          MJobRemove(&J);

          continue;
          }

        /* if the destination RM is another moab and the connection to that moab is down then
           transition the job so that it is in the cache. */

        if ((J->DestinationRM != NULL) && 
            (J->DestinationRM->Type == mrmtMoab) &&
            (J->DestinationRM->State == mrmsDown))
          {
          MJobTransition(J,FALSE,FALSE);
          }

        MS3AddLocalJob(InternalRM,J->Name);
        bmunset(&J->IFlags,mjifShouldCheckpoint);
        }
      }

    MUFree((char **)&TaskList);

    MUArrayListFreePtrElements(&JobNames);
    MUArrayListFree(&JobNames);
    }  /* END else (MDBInfo->DBType != mdbNONE) */

  if (MSched.EnableHighThroughput == TRUE)
    {
    /* fixme
    mjob_t *OldHead = MJob[0];
    mjob_t *NewHead;

    NewHead = MQueueSort(OldHead);
    */
    }

  MRMUpdateInternalJobCounter(InternalRM);

  return(SUCCESS);
  }  /* END MCPLoadAllSpoolCP() */



/**
 * Returns the absolute path location of the checkpoint file
 * for the given job.
 *
 * @param J (I)
 * @param OBuf (I/O) [modified]
 * @param OBufSize (I)
 */

int MCPGetCPFileName(

  const mjob_t *J,
  char         *OBuf,
  int           OBufSize)

  {
  if ((J == NULL) || (OBuf == NULL))
    {
    return(FAILURE);
    }

  snprintf(OBuf,OBufSize,"%s/%s.cp",
    MSched.CheckpointDir,
    J->Name);

  return(SUCCESS);
  }  /* END MCPGetCPFileName() */




/**
 * If the journal file is already open (see MCP.JournalFP), it successfully
 * clears the journal file (truncates to size 0) and resets the file pointer to
 * the beginning of the file.
 */

int MCPJournalClear()

  {
  if (MCP.UseCPJournal == FALSE)
    {
    return(SUCCESS);
    }

  if (MCP.JournalFP == NULL)
    {
    return(FAILURE);  /* no journal file is open */
    }

  /* truncates file to size 0 */

  if (ftruncate(fileno(MCP.JournalFP),0) != 0)
    {
    /* error occurred */

    MDB(0,fCKPT) MLog("ALERT:  cannot clear checkpoint journal.  errno: %d (%s)\n",
      errno,
      strerror(errno));

    return(FAILURE);
    }

  rewind(MCP.JournalFP);  /* set FD pointer to beginning of file */

  return(SUCCESS);
  }  /* END MCPJournalClear() */



/**
 * Opens/creates the checkpoint journal file for reading/writing.
 * This function will set the global journal file descriptor as well.
 *
 * NOTE: This function only does something if MCP.UseCPJournal == TRUE!
 *
 * @param JournalFileName (I)
 */

int MCPJournalOpen(

  char *JournalFileName)

  {
  if (MCP.UseCPJournal == FALSE)
    {
    return(SUCCESS);
    }

  if (JournalFileName == NULL)
    {
    return(FAILURE);
    }

  if (MCP.JournalFP != NULL)
    {
    fclose(MCP.JournalFP);
    }
  else
    {
    if (MSched.Iteration == 0)
      {
      long JFileSize = -1;

      /* on boot, decide if we are recovering from a crash or not */

      if ((MFUGetInfo(JournalFileName,NULL,&JFileSize,NULL,NULL) == FAILURE) ||
          (JFileSize == 0))
        {
        /* journal file doesn't exist or is size 0, so we can assume
         * that we are not recovering from a crash, since if we shutdown cleanly, the
         * journal file is cleared/truncated */

        MSched.HadCleanStart = TRUE;
        }
      }
    }

  MFUCreate(JournalFileName,NULL,NULL,0,0600,-1,-1,TRUE,NULL);
 
  if ((MCP.JournalFP = fopen(JournalFileName,"r+")) == NULL)
    { 
    MDB(1,fCKPT) MLog("WARNING:  cannot open checkpoint journal file '%s'. errno: %d (%s)\n",
      JournalFileName,
      errno,
      strerror(errno));
 
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCPJournalOpen() */




/**
 * Closes the checkpoint journal file.
 */

int MCPJournalClose()

  {
  if (MCP.JournalFP != NULL)
    {
    fclose(MCP.JournalFP);
    }

  return(SUCCESS);
  }  /* END MCPJournalClose() */





/**
 * Creates an entry in the checkpoint journal indicating that we
 * are changing the given object's attribute.
 *
 * @param OType (I) Type of object that we are creating entry for.
 * @param OID (I) The name/ID of the object.
 * @param AName (I) The name of the attribute we are creating the entry for.
 * @param AVal (I) The value of the attribute.
 * @param Mode (I) How we are changing the attribute (mSet,mAdd,etc.)
 */

int MCPJournalAdd(

  enum MXMLOTypeEnum OType,
  char         *OID,
  char         *AName,
  char         *AVal,  /* support non-string values like MJobSetAttr? */
  enum MObjectSetModeEnum Mode)

  {
  char tmpBuf[MMAX_BUFFER];

  tmpBuf[0] = '\0';

  if ((MCP.UseCPJournal == FALSE) ||
      (MCP.JournalFP == NULL))
    {
    /* journaling not enabled */

    return(SUCCESS);
    }

  if ((OID == NULL) ||
      (AName == NULL) ||
      (AVal == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT: <OBJECT TYPE>:<OBJECT ID>:<MODE>:<ATTR NAME>:<ATTR VALUE> */

  snprintf(tmpBuf,sizeof(tmpBuf),"%s:%s:%s:%s:%s",
    MXO[OType],
    OID,
    MObjOpType[Mode],
    AName,
    AVal);

  fprintf(MCP.JournalFP,"%ld %s\n",
      MSched.Time,
      tmpBuf);

  fflush(MCP.JournalFP);  /* data MUST be removed from user space in case of a crash */

  return(SUCCESS);
  }  /* END MCPJournalAdd() */




/**
 * Reads in the CP journal system one line at a time and processes
 * the recorded event.
 */

int MCPJournalLoad()

  {
  int rc;
  mbuffd_t BufFD;
  char tmpLine[MMAX_LINE * 4];
  char *ptr;
  char *TokPtr;

  enum MXMLOTypeEnum OType;
  char *OTypeTxt;
  char *OID;
  enum MObjectSetModeEnum Mode;
  char *ModeTxt;
  char *AName;
  char *AValue;

  if ((MCP.UseCPJournal == FALSE) ||
      (MCP.JournalFP == NULL))
    {
    return(SUCCESS);
    }

  MUBufferedFDInit(&BufFD,fileno(MCP.JournalFP));

  /* disable journaling temporarily so we don't have any
   * circular loops when calling MJobSetAttr() as this routine
   * may write out to the journal file as we change job attributes. This
   * would be bad as we would eventually read them in from the journal file and 
   * get stuck in an infinite loop! */

  MCP.UseCPJournal = FALSE;

  while (1)
    {
    rc = MUBufferedFDReadLine(&BufFD,tmpLine,sizeof(tmpLine));

    if (rc == 0)
      {
      /* EOF--done loading file */

      MCP.UseCPJournal = TRUE;

      return(SUCCESS);
      }
    else if (rc < 0)
      {
      /* error occurred */

      MCP.UseCPJournal = TRUE;

      return(FAILURE);
      }
    else if ((unsigned int)rc == sizeof(tmpLine))
      {
      /* buffer overflow...read next line */

      continue;
      }

    /* remove timestamp */

    ptr = MUStrTok(tmpLine," ",&TokPtr);

    if (ptr == NULL)
      {
      /* malformed line */

      continue;
      }

    /* FORMAT: <OBJECT TYPE>:<OBJECT ID>:<MODE>:<ATTR NAME>:<ATTR VALUE> */

    ptr = MUStrTok(NULL,":\n",&TokPtr);
    OTypeTxt = ptr;

    ptr = MUStrTok(NULL,":\n",&TokPtr);
    OID = ptr;

    ptr = MUStrTok(NULL,":\n",&TokPtr);
    ModeTxt = ptr;

    ptr = MUStrTok(NULL,":\n",&TokPtr);
    AName = ptr;

    ptr = MUStrTok(NULL,":\n",&TokPtr);
    AValue = ptr;

    OType = (enum MXMLOTypeEnum)MUGetIndex(OTypeTxt,MXO,FALSE,mxoNONE);
    Mode = (enum MObjectSetModeEnum)MUGetIndex(ModeTxt,MObjOpType,FALSE,mNONE2);

    switch (OType)
      {
      case mxoJob:

        {
        mjob_t *J;
        enum MJobAttrEnum AttrIndex;

        if (MJobFind(OID,&J,mjsmBasic) == SUCCESS)
          {
          AttrIndex = (enum MJobAttrEnum)MUGetIndex(AName,MJobAttr,FALSE,mjaNONE);

          MJobSetAttr(J,AttrIndex,(void **)AValue,mdfString,Mode);
          }
        }  /* END BLOCK */

        break;

      case mxoRM:

        {
        mrm_t  *R;
        enum MRMAttrEnum AttrIndex;

        if (MRMFind(OID,&R) == SUCCESS)
          {
          AttrIndex = (enum MRMAttrEnum)MUGetIndex(AName,MRMAttr,FALSE,mjaNONE);

          MRMSetAttr(R,AttrIndex,(void **)AValue,mdfString,Mode);
          }
        }  /* END BLOCK */

        break;

      case mxoSched:

          {
          enum MSchedAttrEnum AttrIndex;

          AttrIndex = (enum MSchedAttrEnum)MUGetIndex(AName,MSchedAttr,FALSE,msaNONE);

          MSchedSetAttr(&MSched,AttrIndex,(void **)AValue,mdfString,Mode);
          }  /* END BLOCK */

          break;

      case mxoTrig:

        {
        mtrig_t *T;
        enum MTrigAttrEnum AttrIndex;

        if (MTrigFind(OID,&T) == SUCCESS)
          {
          AttrIndex = (enum MTrigAttrEnum)MUGetIndex(AName,MTrigAttr,FALSE,mtaNONE);

          MTrigSetAttr(T,AttrIndex,(void **)AValue,mdfString,Mode);
          }
        }  /* END BLOCK */

        break;

      default:

        /* not supported */

        break;
      }  /* END switch(OType) */
    }  /* END while(1) */

  /* re-enable the writing out of journal entries */

  MCP.UseCPJournal = TRUE;
   
  return(SUCCESS);
  }  /* END MCPJournalLoad() */



/**
 * Opens/creates the msub journal file for reading/writing.
 * This function will set a global msub journal file descriptor as well.
 *
 * NOTE: This function only does something if MCP.UseCPJournal == TRUE!
 *
 * @param JournalFileName (I)
 */

int MCPSubmitJournalOpen(

  char *JournalFileName)

  {
  long tmpSize;

  if (MCP.UseCPJournal == FALSE)
    {
    return(SUCCESS);
    }

  if (JournalFileName == NULL)
    {
    return(FAILURE);
    }

  if (MSubJournal.FP != NULL)
    {
    fclose(MSubJournal.FP);
    }

  MFUCreate(JournalFileName,NULL,NULL,0,0600,-1,-1,TRUE,NULL);
  MFUGetInfo(JournalFileName,NULL,&tmpSize,NULL,NULL);
 
  if ((MSubJournal.FP = fopen(JournalFileName,"r+")) == NULL)
    { 
    MDB(1,fCKPT) MLog("WARNING:  cannot open msub journal file '%s'. errno: %d (%s)\n",
      JournalFileName,
      errno,
      strerror(errno));
 
    return(FAILURE);
    }

  MSubJournal.JournalSize = tmpSize;
  MSubJournal.OutstandingEntries = 0;
  MSubJournal.LastClearTime = 0;

  MUMutexInit(&MSubJournal.Lock);

  return(SUCCESS);
  }  /* END MCPSubmitJournalOpen() */


/**
 * Adds data to the submit journal so that the
 * given socket can be reconsituted and processed if there
 * is a crash.
 *
 * @param Submit (I)
 * @param J      (I)
 */

int MCPSubmitJournalAddEntry(

  mstring_t     *Submit,
  mjob_submit_t *J)

  {
  int rc;

  if (MCP.UseCPJournal == FALSE)
    {
    return(SUCCESS);
    }

  if ((Submit == NULL) || 
      (J == NULL) ||
      (MSubJournal.FP == NULL))
    {
    return(FAILURE);
    }

  MUMutexLock(&MSubJournal.Lock);

  J->JournalID = ftell(MSubJournal.FP);

  /* FORMAT: <USERNAME> <ID> <SOCKETDATA> */

  rc = fprintf(MSubJournal.FP,"%s %d %s\n",
      J->User,
      J->ID,
      Submit->c_str());

  MSubJournal.JournalSize += rc;
  MSubJournal.OutstandingEntries++;

  fflush(MSubJournal.FP);  /* data MUST be removed from user space in case of a crash */

  MUMutexUnlock(&MSubJournal.Lock);

  return(SUCCESS);
  }  /* END MCPSubmitJournalAddEntry() */

#define MSUBMITJOURNAL_CLEARTIME 120
/**
 *  This function will notify the submit journal
 *  logic that the given entry has been processed, checkpointed,
 *  and can be safely removed from the submit journal.
 *
 *  NOTE: For now Moab will determine if it can clear the
 *  file based simply on a timestamp/file size and the
 *  outstanding records counter. This should be enhanced
 *  later on to be more fine-grained so that duplicate submits
 *  are not possible during a crash recovery.
 *
 *  @param EntryID (I)
 */

int MCPSubmitJournalRemoveEntry(

  mulong EntryID)  /* not yet used */

  {
  if (MCP.UseCPJournal == FALSE)
    {
    return(SUCCESS);
    }

  MUMutexLock(&MSubJournal.Lock);
  MSubJournal.OutstandingEntries--;

  /* clear journal file if we have no outstanding entries AND we have either
   * a megabyte of journal file or the clear time has passed */

  if ((MSubJournal.OutstandingEntries == 0) &&
      ((MSubJournal.JournalSize >= MDEF_MBYTE) ||
       (MSched.Time - MSubJournal.LastClearTime >= MSUBMITJOURNAL_CLEARTIME)))
    {
    /* we can safely truncate the journal file
     * as there are no more pending records to
     * process */

    MCPSubmitJournalClear();
    }
  else if (MSubJournal.OutstandingEntries < 0)
    {
    /* this should never happen! */

    MDB(0,fCKPT) MLog("ALERT:  submit journal count '%d' is negative!\n",
      MSubJournal.OutstandingEntries);
    }

  MUMutexUnlock(&MSubJournal.Lock);

  return(SUCCESS);
  }  /* END MCPSubmitJournalRemoveEntry() */


/**
 * If the submit journal file is already open (see MSubJournal), it successfully
 * clears the journal file (truncates to size 0) and resets the file pointer to
 * the beginning of the file.
 *
 * WARNING: If two threads are trying to access the submit journal at the
 * same time, this function should be protected with a lock.
 */

int MCPSubmitJournalClear()

  {
  if (MCP.UseCPJournal == FALSE)
    {
    return(SUCCESS);
    }

  if (MSubJournal.FP == NULL)
    {
    return(FAILURE);  /* no journal file is open */
    }

  /* truncates file to size 0 */

  if (ftruncate(fileno(MSubJournal.FP),0) != 0)
    {
    /* error occurred */

    MDB(0,fCKPT) MLog("ALERT:  cannot clear submit journal.  errno: %d (%s)\n",
      errno,
      strerror(errno));

    return(FAILURE);
    }

  rewind(MSubJournal.FP);  /* set FD pointer to beginning of file */
  MSubJournal.JournalSize = 0;
  MSubJournal.LastClearTime = MSched.Time;

  return(SUCCESS);
  }  /* END MCPSubmitJournalClear() */



/**
 * Closes the submit journal file. Will also clear
 * out the file if there are no outstanding entries.
 */

int MCPSubmitJournalClose()

  {
  if (MSubJournal.OutstandingEntries == 0)
    {
    MCPSubmitJournalClear();
    } 

  if (MSubJournal.FP != NULL)
    {
    fclose(MSubJournal.FP);
    }

  return(SUCCESS);
  }  /* END MCPSubmitJournalClose() */



/**
 * Reads in the CP journal system one line at a time and processes
 * the recorded event.
 */

int MCPSubmitJournalLoad()

  {
  int rc;
  mbuffd_t BufFD;
  char *TokPtr = NULL;

  mjob_submit_t *J;


  char *User;
  char *ID;
  char *XML;
  char *tmpLine = NULL;

  mxml_t *tmpJE;

  if ((MCP.UseCPJournal == FALSE) ||
      (MCP.JournalFP == NULL))
    {
    return(SUCCESS);
    }

  MUBufferedFDInit(&BufFD,fileno(MSubJournal.FP));

  /* disable journaling temporarily so we don't have any
   * circular loops when calling MJobSetAttr() as this routine
   * may write out to the journal file as we change job attributes. This
   * would be bad as we would eventually read them in from the journal file and 
   * get stuck in an infinite loop! */

  MCP.UseCPJournal = FALSE;

  mstring_t Line(MMAX_LINE);

  while (1)
    {
    rc = MUBufferedFDMStringReadLine(&BufFD,&Line);

    if ((rc == 0) || (Line.empty()))
      {
      /* EOF--done loading file */

      MCP.UseCPJournal = TRUE;

      MS3ProcessSubmitQueue();

      return(SUCCESS);
      }
    else if (rc < 0)
      {
      /* error occurred */

      MS3ProcessSubmitQueue();

      MCP.UseCPJournal = TRUE;

      return(FAILURE);
      }

    /* FORMAT: USER<space>ID<space>XML */

    MUStrDup(&tmpLine,Line.c_str());

    User = MUStrTok(tmpLine," ",&TokPtr);

    ID   = MUStrTok(NULL," ",&TokPtr);

    XML  = TokPtr;

    if ((MUStrIsEmpty(User)) ||
        (MUStrIsEmpty(ID)) ||
        (MUStrIsEmpty(XML)) ||
        (XML[0] != '<'))
      {
      MUFree(&tmpLine);

      Line.clear();   /* Reset the string on every iteration */

      continue;
      }

    MJobSubmitAllocate(&J);

    MUStrCpy(J->User,User,MMAX_NAME);

    J->ID = strtol(ID,NULL,10);

    MXMLFromString(&J->JE,XML,NULL,NULL);

    if (MXMLGetChildCI(J->JE,(char *)MXO[mxoJob],NULL,&tmpJE) == SUCCESS)
      {
      mxml_t *SubmitTimeE;

      MXMLCreateE(&SubmitTimeE,(char *)MS3JobAttr[ms3jaSubmitTime]);
      MXMLSetVal(SubmitTimeE,(void *)&MSched.Time,mdfLong);
      MXMLAddE(tmpJE,SubmitTimeE);
      }

    MSysAddJobSubmitToQueue(J,&J->ID,FALSE);
    
    MUFree(&tmpLine);
    }  /* END while(1) */

  MS3ProcessSubmitQueue();

  /* re-enable the writing out of journal entries */

  MCP.UseCPJournal = TRUE;
   
  return(SUCCESS);
  }  /* END MCPSubmitJournalLoad() */






/**
 * This function will take the given job and checkpoint
 * data and create a checkpoint file for the job in the spool directory.
 *
 * Note that this function can also use a temporary file and the rename()
 * function to decrease the likelihood that the CP file will be lost
 * if Moab is killed by a signal. The UseTmpFile param should be TRUE
 * if this is desired.
 *
 * @param J (I)
 * @param CPData (I)
 * @param UseTmpFile (I)
 */

int MCPPopulateJobCPFile(

  const mjob_t *J,
  const char   *CPData,
  mbool_t       UseTmpFile)

  {
  char FileName[MMAX_PATH_LEN];
  char TempFileName[MMAX_PATH_LEN];
  char *ptr;
  FILE *CPFile;

  if ((J == NULL) ||
      (CPData == NULL))
    {
    return(FAILURE);
    }

  TempFileName[0] = '\0';

  /* store file in spool/$jobid.cp */

  MCPGetCPFileName(J,FileName,sizeof(FileName));

  MDB(-1,fCKPT) MLog("storing checkpoint for job '%s' in file '%s'\n",  /*JOSH:remove*/
    J->Name,
    FileName);

  if (UseTmpFile == TRUE)
    {
    snprintf(TempFileName,sizeof(TempFileName),"%s.tmp",FileName);

    ptr = TempFileName;
    }
  else
    {
    ptr = FileName;
    }

  CPFile = fopen(ptr,"w");

  /* Job CP is in the spool directory, set permissions to 0600 */

  if (chmod(FileName,0600) == -1)
    {
    MDB(1,fCORE) MLog("WARNING:  cannot chmod file '%s' to 0600.  errno: %d (%s)\n",
      FileName,
      errno,
      strerror(errno));
    }

  if (CPFile != NULL)
    {
    fprintf(CPFile,"%-9s %20s %9ld %s\n",
      MCPType[mcpJob],
      J->Name,
      MSched.Time,
      CPData);

    fclose(CPFile);

    if (UseTmpFile == TRUE)
      rename(TempFileName,FileName);
    }

  return(SUCCESS);
  }  /* END MCPPopulateJobCPFile() */




int MCPStoreTransactions(

  mckpt_t            *CP)      /* I */

  {
  mtrans_t *Trans = NULL;

  int tindex;

  mstring_t String(MMAX_LINE);

  mxml_t *TE = NULL;
 
  for (tindex = 1;tindex < MMAX_TRANS;tindex++)
    {
    Trans = NULL;

    if (MTransFind(
        tindex,
        NULL,
        &Trans) == FAILURE)
      {
      continue;
      }

    if ((tindex <= MSched.LastTransCommitted) &&
        (Trans->RsvID == NULL))
      {
      /* don't checkpoint invalid TIDs */

      continue;
      }

    TE = NULL;

    MTransToXML(Trans,&TE);

    /* Clear the mstring */
    String.clear();

    MXMLToMString(TE,&String,NULL,TRUE);

    MCPStoreObj(CP->fp,mcpTrans,Trans->Val[mtransaName],String.c_str());

    MXMLDestroyE(&TE);
    }

  return(SUCCESS);
  }  /* END MCPStoreTransactions() */



/**
 * Checkpoints all VCs.
 *
 * @param CP (I) [utilized] - the checkpoint file
 */

int MCPStoreVCs(

  mckpt_t *CP)  /* I (utilized) */

  {
  mhashiter_t Iter;
  char       *VCName;
  mvc_t      *VC;
  mstring_t   tmpS(MMAX_LINE);
  mxml_t     *E;

  const char *FName = "MCPStoreVCs";

  MDB(3,fCKPT) MLog("%s(CP)\n",
    FName);

  if (CP == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&Iter);

  while (MUHTIterate(&MVC,&VCName,(void **)&VC,NULL,&Iter) == SUCCESS)
    {
    if (VC == NULL)
      continue;

    E = NULL;

    MXMLCreateE(&E,(char *)MXO[mxoxVC]);

    MVCToXML(VC,E,NULL,FALSE);

    tmpS = "";
    MXMLToMString(E,&tmpS,NULL,FALSE);

    /* BufSize is not used in MCPStoreObj, not reason to waste time with a sizeof */

    MCPStoreObj(CP->fp,mcpVC,VC->Name,tmpS.c_str());

    MXMLDestroyE(&E);
    }

  return(SUCCESS);
  }  /* END MCPStoreVCs() */



/**
 * Load in the VC from the given checkpoint line.
 *
 * Because VCs can connect to other VCs, this function will instantiate all of the VCs
 *  and then store the info in a VC transition object.  After all of the VCs are loaded,
 *  there is a second pass, where all of the connections between VCs and objects will be
 *  made.
 *
 * @param Line [I] - The VC checkpoint info
 */

int MCPLoadVC(

  char *Line)

  {
  mvc_t *VC;

  char *ptr;
  char  tmp[MMAX_LINE + 1];
  char  tmpName[MMAX_LINE + 1];
  long CkTime;

  const char *FName = "MCPLoadVC";

  MDB(4,fCKPT) MLog("%s(%s)\n",
    FName,
    Line);

  sscanf(Line,"%64s %64s %ld",
    tmp,
    tmpName,
    &CkTime);

  if (MVCAllocate(&VC,NULL,tmpName) == FAILURE)
    {
    /* Already have a VC by this name.  This shouldn't happen. */

    return(FAILURE);
    }

  if ((MSched.Time - CkTime) > MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  if ((ptr = strchr(Line,'<')) == NULL)
    {
    return(FAILURE);
    }

  /* Now info is stored as just a string.  Actual VCs will be populated
      in MCPSecondPass(), after all objects have been read in */

  MDB(8,fCKPT) MLog("INFO:     saving checkpoint line \"%s\" on VC '%s'\n",
    ptr,
    VC->Name);

  MUStrDup(&VC->CheckpointDesc,ptr);

  return(SUCCESS);
  }  /* END MCPLoadVC() */


/**
 * Does a second-pass of checkpoint info, setting up connections, etc.
 *  Should be called after the general checkpoint load so that all objects
 *  are in memory.
 */

int MCPSecondPass()

  {
  mhashiter_t Iter;
  char       *VCName;
  mvc_t      *VC;

  const char *FName = "MCPSecondPass";

  MDB(7,fSTRUCT) MLog("%s()\n",
    FName);

  /* Hook up VCs */

  MUHTIterInit(&Iter);

  while (MUHTIterate(&MVC,&VCName,(void **)&VC,NULL,&Iter) == SUCCESS)
    {
    if (VC == NULL)
      continue;

    if (MVCFromString(VC,VC->CheckpointDesc) == SUCCESS)
      {
      MUFree(&VC->CheckpointDesc);
      }
    }

  return(SUCCESS);
  }  /* END MCPSecondPass() */

/* MCP.c */
