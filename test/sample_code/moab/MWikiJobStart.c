/* HEADER */

      
/**
 * @file MWikiJobStart.c
 *
 * Contains: MWikiJobStart
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"

/**
 * This routine builds the Wiki TASKLIST and sends the job start command to 
 * the RM.
 *
 * @see MRMJobStart() - parent
 * @see MWikiDoCommand() - child
 *
 * @param J (I)
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiJobStart(

  mjob_t               *J,    /* I */
  mrm_t                *R,    /* I */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  int     tindex;
  int     tcount = 0;

  int     tmpSC;
  char    tEMsg[MMAX_LINE];

  char   *FSID = NULL;

  char    Comment[MMAX_LINE];

  mnode_t *N;

  char   *Response = NULL;

  char    tmpLine[MMAX_LINE];
  char   *MPtr;

  const char *FName = "MWikiJobStart";

  MDB(2,fWIKI) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  Comment[0] = '\0';

  if ((J == NULL) || (R == NULL) || (J->TaskMap == NULL))
    {
    sprintf(MPtr,"start request is corrupt");

    return(FAILURE);
    }

  mstring_t TaskBuf(MMAX_BUFFER);

  /* If we are doing compressed output then build our TASKLIST in the form of odev[01-02]*3;odev[03]*1 */

  if (bmisset(&R->Wiki.Flags,mslfCompressOutput))
    {
    mnl_t *tmpNodeList = &J->NodeList; 

    MDB(7,fWIKI) MLog("INFO:     job %s uses compressed tasklists\n",
      J->Name);

    /* tmpNodeList may be changed to point to a dynamically allocated buffer 
       below */

    /* The task counts for each node in the NodeList were determined in 
       MJobDistributeTasks() using J->Request.TC

       If the user specified a total task count (J->TTC > 0) then we must 
       increment the task counts on some or all of the nodes to make sure that 
       we honor the total task count request (which may be greater than 
       J->Request.TC */

    /* NOTE: In the case of overcommit (i.e., "msub -l nodes=2,ttc=5 -O") we 
             will not want to change the TASKLIST so skip over this for an 
             overcommit. (NYI) */

    if ((J->TotalTaskCount != 0) && (!MNLIsEmpty(&J->NodeList)))
      {
      mnode_t *tmpN;

      int CurrentTaskCount = 0; /* number of procs previously allocated in the nodelist */
      int ConfiguredTaskCount = 0; /* total number of procs available from the nodes in the nodelist */
      int nindex;

      /* Find out how many tasks/processors we have already accounted for */

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&tmpN) == SUCCESS; nindex++) 
        {
        CurrentTaskCount += MNLGetTCAtIndex(&J->NodeList,nindex);
        ConfiguredTaskCount += tmpN->CRes.Procs;
        }
      
      MDB(7,fWIKI) MLog("INFO:     job %s has %d nodes, ttc %d, current task count %d, configured task count %d\n",
        J->Name,
        nindex,
        J->TotalTaskCount,
        CurrentTaskCount,
        ConfiguredTaskCount);

      /* Make sure that the nodes have enough procs to accomodate the TTC request */

      if (J->TotalTaskCount > ConfiguredTaskCount)
        {
        /* This should have been an overcommit request */

        MDB(5,fSTRUCT) MLog("ALERT:    job %s ttc (%d) exceeds total processors in nodelist (%d)\n",
          J->Name,
          J->TotalTaskCount,
          ConfiguredTaskCount);
        }
      else
        {
        /* malloc a copy of the nodelist which we can modify and pass into the 
           routine that builds the range string */

        if ((tmpNodeList = (mnl_t *)MUCalloc(1,sizeof(mnl_t))) == NULL)
          {
          MDB(1,fSTRUCT) MLog("ALERT:    cannot allocate memory for nodelist copy %s, errno: %d (%s)\n",
            J->Name,
            errno,
            strerror(errno));

          return(FAILURE);
          }

        MNLInit(tmpNodeList);

        MNLCopy(tmpNodeList,&J->NodeList);
       
        /* Start at the beginning of the nodelist and "Top off" nodes until we bring the total task count
           up to the TTC value */  

        for (nindex = 0;(MNLGetNodeAtIndex(tmpNodeList,nindex,&tmpN) == SUCCESS) && (CurrentTaskCount < J->TotalTaskCount);nindex++) 
          {
          while ((CurrentTaskCount < J->TotalTaskCount) && 
                 (MNLGetTCAtIndex(tmpNodeList,nindex) < tmpN->CRes.Procs))
            {
            MNLAddTCAtIndex(tmpNodeList,nindex,1);
            CurrentTaskCount++;
            } /* END while ((CurrentTaskCount < J->TTC) && ...) */
          }   /* END for (nindex) */

        MDB(7,fWIKI) MLog("INFO:     job %s ttc %d, current task count %d after topping off nodes\n",
          J->Name,
          J->TotalTaskCount,
          CurrentTaskCount);

        for (nindex = 0;MNLGetNodeAtIndex(tmpNodeList,nindex,&tmpN) == SUCCESS;nindex++)
          {
          MDB(7,fWIKI) MLog("INFO:     job %s node (%s), node TC %d, node CRes procs %d \n",
            J->Name,
            tmpN->Name,
            MNLGetTCAtIndex(tmpNodeList,nindex),
            tmpN->CRes.Procs);
          }   /* END for (nindex) */
        }     /* END else (J->TTC > ConfiguredTaskCount */
      }       /* END if (J->TTC != 0) && (J->NodeList != NULL) */

    /* Build the tasklist from the nodelist */

    MUNLToRangeString(
      tmpNodeList,
      NULL,
      -1,
      TRUE,
      TRUE,
      &TaskBuf);

    /* If we allocated a working nodelist then free the memory */

    if (tmpNodeList != &J->NodeList)
      {
      MNLFree(tmpNodeList);
      MUFree((char **)&tmpNodeList);
      }
    }   /* END if bmisset(&R->U.Wiki.Flags,mslfCompressOutput) */
  else
    {
    MDB(7,fWIKI) MLog("INFO:     job %s uses non-compressed tasklists\n",
      J->Name);

    for (tindex = 0;tindex < MSched.JobMaxTaskCount;tindex++)
      {
      if (J->TaskMap[tindex] == -1)
        break;

      N = MNode[J->TaskMap[tindex]];

      if (N->RM == R)
        {
        if (tcount > 0)
          MStringAppend(&TaskBuf,":");

        MStringAppendF(&TaskBuf,"%s",N->Name);

        tcount++;

        if ((N->FSys != NULL) && (N->FSys->BestFS >= 0))
          {
          FSID = N->FSys->Name[N->FSys->BestFS];
          }
        }
      }   /* END for (tindex = 0) */

    if (tcount == 0)
      {
      MDB(3,fWIKI) MLog("ALERT:    no tasks found for job '%s' on RM %d\n",
        J->Name,
        R->Index);

      return(FAILURE);
      }
    }     /* END else (bmisset(&R->U.Wiki,mslfCompressOutput)) */

  {
  int RetCode;

  mstring_t CommandBuf(MMAX_BUFFER);

  MStringSetF(&CommandBuf,"%s%s %s%s TASKLIST=%s",
    MCKeyword[mckCommand],
    MWMCommandList[mwcmdStartJob],
    MCKeyword[mckArgs],
    (J->DRMJID != NULL) ? J->DRMJID : J->Name,
    TaskBuf.c_str());

  if (FSID != NULL)
    {
    /* add filesystem info */

    MStringAppendF(&CommandBuf," FSID=%s",FSID);
    }

  if (Comment[0] != '\0')
    {
    /* add job start comment */

    MStringAppendF(&CommandBuf," COMMENT=%s",Comment);
    }

  RetCode = MWikiDoCommand(
        R,
        mrmJobStart,
        &R->P[R->ActingMaster],
        FALSE,
        R->AuthType,
        CommandBuf.c_str(),
        &Response,  /* O (alloc,populated on SUCCESS) */
        NULL,
        FALSE,
        tEMsg,      /* O (populated on FAILURE) */
        &tmpSC);

  if (RetCode == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot start job '%s' on %s RM on %d procs (command failure - (%d) '%s')\n",
      J->Name,
      MRMType[R->Type],
      tcount,
      tmpSC,
      (tEMsg != NULL) ? tEMsg : "N/A");

    if (tmpSC < 0)
      tmpSC *= -1;

    switch (tmpSC)
      {
      case 0:
      case 100:

        /* success */

        break;

      /* job specific/transient failures */

      case 200:
      case 300:
      case 730: /* resource unavailable */

        if (EMsg != NULL)
          MUStrCpy(EMsg,tEMsg,MMAX_LINE);

        MMBAdd(&J->MessageBuffer,tEMsg,NULL,mmbtNONE,0,0,NULL);

        return(FAILURE);

        /*NOTREACHED*/

        break;

      case 913: /* Resources (temporarily unavailable) */

        if (MPar[0].JobRetryTime <= 0)
          {
          MJobReject(
            &J,
            mhBatch,
            300,
            mhrRMReject,
            "job cannot be started, RM resources temporarily unavailable");
          }

        if (J != NULL)
          MMBAdd(&J->MessageBuffer,tEMsg,NULL,mmbtNONE,0,0,NULL);

        return(FAILURE);

        /*NOTREACHED*/

        break;

      /* widespread service failures */

      case 400: 
      case 700:
      case 710: 
      case 720: /* server internal error */
      case 800:
      case 900: /* general miscellaneous error */
      case 912: /* Dependency */
      case 914: /* PartitionNodeLimit */
      case 915: /* PartitionTimeLimit */
      case 916: /* PartitionDown */
      default:

        R->FailIteration = MSched.Iteration;

        if (EMsg != NULL)
          MUStrCpy(EMsg,tEMsg,MMAX_LINE);

        /* NOTE:  defer added for LLNL to prevent jobs which cannot run immediately
                  due to policies from blocking other jobs */

        MJobSetHold(J,mhDefer,300,mhrRMReject,"job cannot start due to RM policy violation");

        MMBAdd(&J->MessageBuffer,tEMsg,NULL,mmbtNONE,0,0,NULL);

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (tmpSC) */

    if (SC != NULL)
      *SC = mscRemoteFailure;

    R->FailIteration = MSched.Iteration;

    return(FAILURE);
    }  /* END if (MWikiDoCommand() == FAILURE) */
  }    /* END case */

  if (tmpSC < 0)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot start job '%s' on %s RM on %d procs (rm failure)\n",
      J->Name,
      MRMType[R->Type],
      tcount);

    R->FailIteration = MSched.Iteration;

    if (EMsg != NULL)
      MUStrCpy(EMsg,Response,MMAX_LINE);

    if (Response != NULL)
      MUFree(&Response);

    return(FAILURE);
    }

  MDB(2,fWIKI) MLog("INFO:     job '%s' started through %s RM on %d procs\n",
    J->Name,
    MRMType[R->Type],
    tcount);

  MUFree(&Response);

  return(SUCCESS);
  }  /* END MWikiJobStart() */

/* END MWikiJobStart.c */
