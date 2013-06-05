/* HEADER */

      
/**
 * @file MRM_CP.c
 *
 * Contains: RM Checkpoint functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

 
 
/**
 * Load check-pointed RM attributes.
 *
 * @see MRMInitializeLocalQueue() - parent
 * @see MRMFromXML() - child
 *
 * @param RS (I) [optional]
 * @param Buf (I)
 */

int MRMLoadCP(

  mrm_t *RS,    /* I (optional) */
  const char  *Buf)   /* I */

  {
  char   tmpName[MMAX_NAME + 1];
  char   RMID[MMAX_NAME + 1];

  mrm_t *R;

  mrm_t  tmpR;

  mxml_t *RME;

  /* If you get a seg fault here, increase stacksize with ulimit -s unlimited */
  long    CkTime = 0;

  const char   *ptr;

  const char *FName = "MRMLoadCP";

  MDB(4,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (RS != NULL) ? "RS" : "NULL",
    (Buf != NULL) ? Buf : "NULL");

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  /* FORMAT:  <OTYPE> <OID> <CKTIME> <ODATA> */

  /* determine rm ID */

  sscanf(Buf,"%64s %64s %ld",
    tmpName,
    RMID,
    &CkTime);

  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    MDB(7,fSTRUCT) MLog("INFO:     checkpoint info expired for RM %s\n",
      RMID);

    return(SUCCESS);
    }

  if (RS == NULL)
    {
    if (MRMFind(RMID,&R) == SUCCESS)
      {
      /* ignore existing RM's if not explicitly specified */

      return(SUCCESS);
      }

    /* rm may be dynamic, load into temp buffer */

    memset(&tmpR,0,sizeof(tmpR));

    R = &tmpR;
    }
  else
    {
    R = RS;
    }

  if ((ptr = strchr(Buf,'<')) == NULL)
    {
    MDB(4,fSTRUCT) MLog("WARNING:  malformed XML data for RM %s\n",
      RMID);

    return(FAILURE);
    }

  RME = NULL;

  if ((MXMLFromString(&RME,ptr,NULL,NULL) == FAILURE) ||
      (MRMFromXML(R,RME) == FAILURE))
    {
    MXMLDestroyE(&RME);

    /* cannot load rm */

    MDB(4,fSTRUCT) MLog("WARNING:  XML cannot be processed for RM %s\n",
      RMID);

    return(FAILURE);
    }

  MXMLDestroyE(&RME);

  return(SUCCESS);
  }  /* END MRMLoadCP() */





/**
 * Attempts to load the given job, using the given RM as the "owner" or "discover-er" of the job.
 * This function will determine if the job is in the .moab.ck file or in a individual checkpoint
 * file in the spool directory.
 *
 * @param R (I)
 * @param J (I) [modified]
 * @param EMsg (I) [minsize=MMAX_LINE]
 *
 * @see MCPRestore() - child
 * @see MRMJobPostLoad() - parent
 *
 * @return SUCCESS if there were no errors with the job's checkpoint data. SUCCESS is also returned if the
 * job is not found in the checkpoint file.
 */

int MRMLoadJobCP(

  mrm_t   *R,       /* I */
  mjob_t  *J,       /* I (modified) */
  char    *EMsg)    /* O (optional,minsize=MMAX_LINE) */

  {
  mbool_t IsFound = FALSE;

  mdb_t *MDBInfo;

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((R == NULL) ||
      (J == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';
    }

  if (bmisset(&R->Flags,mrmfFullCP) || 
      bmisset(&R->IFlags,mrmifLocalQueue) || 
     (R->Type == mrmtMoab))
    {
    IsFound = TRUE;

    if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
      {
      char *CPFile=NULL;  /* NULL it: returned pointer to read buffer */

      /* Load full job record from spool file */
   
      char FileName[MMAX_PATH_LEN];

      MCPGetCPFileName(J,FileName,sizeof(FileName));

      /* note - MFULoadAllocReadBuffer does the MUCalloc on CPFile.
                This routine must do the MUFree on CPFile 
                ON SUCCESS ONLY. */

      if (MFULoadAllocReadBuffer(FileName,&CPFile,EMsg,NULL) == FAILURE)
        {
        IsFound = FALSE;
   
        if (J->Req[0] == NULL)
          {
          /* job is invalid */
   
          MDB(1,fRM) MLog("ERROR:    cannot load job checkpoint file for job %s - %s\n",
            J->Name,
            EMsg);
   
          return(FAILURE);
          }
        }
      else if (MJobLoadCPNew(J,CPFile) == FAILURE)
        {
        IsFound = FALSE;
   
        if (bmisset(&J->SpecFlags,mjfNoRMStart))
          {
          if (EMsg != NULL)
            strcpy(EMsg,"cannot load job checkpoint info");
   
          MDB(1,fRM) MLog("ERROR:    cannot load job checkpoint info for job %s\n",
            J->Name);
   
          /* MFULoadAllocReadBuffer was successful, so free its read buffer it passed back */
          MUFree(&CPFile);

          return(FAILURE);
          }    /* END if (bmisset(&J->SpecFlags,mjfNoRMStart)) */
        }      /* END else if (MJobLoadCPNew(J,CPFile) == FAILURE) */

      /* Free the read buffer which was allocated */
      MUFree(&CPFile);
      }        /* END if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE)) */
    else
      {
      mstring_t MString(MMAX_BUFFER);
      int queryRC;
      int loadRC = FAILURE;

      /* check database for job, create new entry if job does not exist in database */

      queryRC = MDBQueryCP(MDBInfo,(char *)MCPType[mcpJob],J->Name,NULL,1,&MString,NULL);

      if (queryRC == SUCCESS)
        loadRC = MJobLoadCPNew(J,MString.c_str());

      if ((queryRC == SUCCESS) && (loadRC == FAILURE))
        {
        IsFound = FALSE;
   
        if (bmisset(&J->SpecFlags,mjfNoRMStart))
          {
          if (EMsg != NULL)
            strcpy(EMsg,"cannot load job checkpoint info");
   
          MDB(1,fRM) MLog("ERROR:    cannot load job checkpoint info for job %s\n",
            J->Name);
   
          return(FAILURE);
          }
        }
      else
        {
        IsFound = FALSE;

        /* MOCheckpoint(mxoJob,(void *)J,TRUE,NULL); -- why are we doing this?
         * this function is called in MRMJobPostLoad and we checkpoint in there as well */
        }
      }
    }        /* END if (bmisset(&R->Flags,mrmfFullCP) || ... */
  else if ((MSched.Iteration == 0) ||
           (MSched.EnableHighThroughput == FALSE) ||
           bmisset(&R->IFlags,mrmifLocalQueue))
    {
    /* in high throughput mode, only look in checkpoint record on iteration 0 */

    char tmpName[MMAX_NAME];

    if (J->SRMJID != NULL)
      {
      /* use source RM job id over job name */

      MJobGetName(NULL,J->SRMJID,R,tmpName,sizeof(tmpName),mjnShortName);
      }
    else
      {
      MUStrCpy(tmpName,J->Name,sizeof(tmpName));
      }

    MCPRestore(mcpJob,tmpName,(void *)J,&IsFound);
     
    /* Looking up the job name by SRMJID in the checkpoint file may not be correct.
     * The job may have been stored with J->Name instead of SRMJID.
     * If so, the check above could fail to load the job from the checkpoint, so we
     * would lose the information that was checkpointed.
     * Thus we try again if MCPRestore fails.
     */

    if ((IsFound == FALSE) && 
        (J->SRMJID != NULL) && 
        (strcmp(J->SRMJID,tmpName)))
      {
      MUStrCpy(tmpName,J->Name,sizeof(tmpName));

      MCPRestore(mcpJob,tmpName,(void *)J,&IsFound);
      }

    if (IsFound == FALSE)
      {
      MDB(4,fRM) MLog("INFO:     job checkpoint info for job %s (%s) not found\n",
        J->Name,
        (J->SRMJID == NULL) ? "" : J->SRMJID);
      } 
    }    /* END else (bmisset(&R->Flags,mrmfFullCP) || ...) */

  /* need to process RM ext. string from newly checkpointed jobs */

  if (IsFound == TRUE)
    {
#if 0
    char *FirstPartition;
    char *LastPartition;
    char *SearchString = "partition:";
#endif

    /* mark this job as being loaded from the checkpoint file so
     * routines know to handle some data differently */
    
    bmset(&J->IFlags,mjifWasLoadedFromCP);

#if 0
    /* WARNING - this code is a temporary bug fix for the llnl 5.2 branch only
       Do not port this code to other branches
       The fix is for ticket 5097 - if we discover how the ALL partition list (including internal)
       is being written to the checkpoint file we can remove this code. */

    /* Due to a bug in the client software we could have the default submit partition and a partition specified in the command
       script show up in the RMXString as follows...
       
       "x=partition:g02;partition:g04"
       
       Since llnl does not want to update all of their client systems we are putting this temporary fix in the moab code
       that reads in the checkpoint records to remove the first partition entry if multiple partition entries are found.
       This is a legal syntax, but we are assuming that we have encoutnered it due to the client bug.

       Note that if a user specifies multiple partitions that the RMXString has the format "x=partition:g02:g04" */
 
    if ((J->RMXString != NULL) &&
        ((FirstPartition = MUStrStr(J->RMXString,SearchString,0,TRUE,FALSE)) != NULL))
      {
      /* See if there is another partition:xyz in the RMXString */

      if ((LastPartition = MUStrStr(FirstPartition + strlen(SearchString),SearchString,0,TRUE,TRUE)) != NULL)
        {
        /* we assume that all the partitions are grouped together - x=partition:g02;partition:g03;partition:g04; */

        while (*LastPartition != '\0')
          {
          *FirstPartition++ = *LastPartition++;
          }
        *FirstPartition = '\0';
        }
      }
    /* END WARNING */
#endif

    MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,NULL);
    }

  return(SUCCESS);
  }  /* END MRMLoadJobCP() */






/**
 * Store dynamic resource manager attributes in checkpoint datastore.
 *
 * @param R (I)
 * @param Buf (O)
 * @param BufSize (I)
 */

int MRMStoreCP(

  mrm_t   *R,       /* I */
  char    *Buf,     /* O */
  int      BufSize) /* I */

  {
  const enum MRMAttrEnum DeltaRMAList[] = {
    mrmaJobCounter,
    mrmaSQLData,
    mrmaNONE };

  mxml_t *RME = NULL;

  if (Buf != NULL)
    Buf[0] = '\0';

  if ((R == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MRMToXML(
    R,
    &RME,
    (enum MRMAttrEnum *)DeltaRMAList);

  if (MXMLToString(RME,Buf,BufSize,NULL,TRUE) == FAILURE)
    {
    MXMLDestroyE(&RME);

    return(FAILURE);
    }

  MXMLDestroyE(&RME);

  return(SUCCESS);
  }  /* END MRMStoreCP() */
/* END MRM_CP.c */
