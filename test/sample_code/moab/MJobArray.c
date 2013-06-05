/* HEADER */

/**
 * @file MJobArray.c
 *
 * Contains JobArray releated functions
 *
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Returns True if job is an array master job. 
 *
 * @param J Job to check.
 */

mbool_t MJobIsArrayMaster(

  mjob_t *J)

  {
  assert(J != NULL);

  return (bmisset(&J->Flags,mjfArrayMaster));
  } /* END mbool_t MJobIsArrayMaster() */


/**
 * Returns True if job array is complete (all jobs are finished)
 *
 * @param J
 *
 */

mbool_t MJobArrayIsFinished(

  mjob_t *J)

  {
  if ((J == NULL) ||
      (!bmisset(&J->Flags,mjfArrayMaster)) ||
      (J->Array == NULL))
    {
    return(FALSE);
    }

  if (J->Array->Complete != J->Array->Count)
    return(FALSE);

  return(TRUE);
  }  /* END MJobArrayIsFinished() */


/**
 * Allocate core job array structures.
 * 
 * @see MJobArrayAddStep() 
 *
 * @param J (I) [modified,attributes allocated] 
 */

int MJobArrayAlloc(

  mjob_t *J)    /* I (modified) */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  /* Alloc and check the Array container/control object */
  J->Array = (mjarray_t *)MUCalloc(1,sizeof(mjarray_t));

  if (J->Array == NULL)
    {
    return(FAILURE);
    }

  J->Array->Members = (int *)MUCalloc(1,sizeof(int) * MMAX_JOBARRAYSIZE);

  /* Alloc and check the Members array. If no memory, free Array and return */
  if (J->Array->Members == NULL)
    {
    MUFree((char **)&J->Array);
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobArrayAlloc() */





int MJobArrayAddStep(

  mjob_t *J,        /* I (modified) */
  mjob_t *JStep,    /* I (optional) */
  char   *JStepID,  /* I (optional) */
  int     JCount,   /* I */
  int    *JStepTok) /* I (optional) */

  {
  int          rc;
  int          sindex;
  mjarray_t   *JA;

  if ((J == NULL) || 
     ((JStep == NULL) && (JStepID == NULL)))
    {
    return(FAILURE);
    }

  /* have we initialized the array/group yet? */
  if (J->JGroup == NULL)
    {
    rc = MJGroupInit(J,J->Name,-1,J);
    
    if (rc != SUCCESS)
      return(rc);

    /* get the JobArray control structure */
    JA = (mjarray_t *)MUCalloc(1,sizeof(mjarray_t));

    if (NULL == JA)
      goto error_group;

    /* Save the pointer to the JobArray control structure */
    J->Array = JA;

    JA->Size    = JCount;

    JA->Members = (int *)MUCalloc(1,sizeof(int) * (JCount + 1));
    if (NULL == JA->Members)
      goto error_array;
 
    JA->JPtrs  = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * (JCount + 1));
    if (NULL == JA->JPtrs)
      goto error_array;
 
    JA->JName   = (char **)MUCalloc(1,sizeof(char *) * (JCount + 1));
    if (NULL == JA->JName)
      goto error_array;
 
    JA->JState  = (enum MJobStateEnum *)MUCalloc(1,sizeof(enum MJobStateEnum) * (JCount + 1));
    if (NULL == JA->JState)
      goto error_array;
 

    bmset(&JA->Flags,mjafDynamicArray);

    /* add self as first job in array */

    MUStrDup(&JA->JName[0],J->Name);

    JA->JState[0]  = J->State;
    JA->Members[0] = 0;
    JA->JPtrs[0]   = J;
    }
  else
    {
    /* Retrive the JobArray control structure pointer */
    JA = J->Array;
    }  /* END if (J->Group == NULL) */

  /* Determine initial index */
  if (JStepTok != NULL)
    sindex = *JStepTok;
  else
    sindex = 1;     /* NOTE:  self is at array index 0 */

  /* Initialize the list of subjobs with their proper information */
  for (;sindex < JA->Size;sindex++)
    {
    if (JA->JPtrs[sindex] == NULL)
      continue;

    if (JStep != NULL)
      {
      mln_t *tmpL;

      MUStrDup(&JA->JName[sindex],JStep->Name);

      JA->JState[sindex]  = JStep->State;

      JA->Members[sindex] = sindex;
      JA->Members[sindex + 1] = -1;

      JA->JPtrs[sindex]  = JStep;

      MJGroupInit(JStep,J->Name,-1,J);

      bmset(&JStep->Flags,mjfArrayJob);
      bmset(&JStep->SpecFlags,mjfArrayJob);

      bmset(&JStep->IFlags,mjifIsArrayJob);

      /* add JStep to J's job group */

      MULLAdd(&J->JGroup->SubJobs,JStep->Name,NULL,&tmpL,NULL);

      tmpL->Ptr = JStep;
      }
    else
      {
      MUStrDup(&JA->JName[sindex],JStepID);
      }

    break;
    }  /* END for (sindex) */

  if (JStepTok != NULL)
    *JStepTok = sindex + 1;

  return(SUCCESS);


  /* Error Clean up stack */
error_array:
  MUFree((char**)&J->Array->JName); 
  MUFree((char**)&J->Array->JPtrs); 
  MUFree((char**)&J->Array->Members); 
  MUFree((char**)&J->Array);

error_group:
  MUFree((char**)&J->JGroup); 
  return(FAILURE);
  }  /* END MJobArrayAddStep() */



/** 
 * Updates individual job information on the master array job.
 *
 * @see MJobArrayAdd() - peer
 *
 * @param J (I)
 */

int MJobArrayUpdate(

  mjob_t *J) /* I */
  
  {
  int jindex;
 
  mjob_t *tmpJ;

  mjarray_t *JA;

  if ((J == NULL) || (J->Array == NULL))
    {
    return(FAILURE);
    }

  JA = J->Array;

  JA->Idle     = 0;
  JA->Active   = 0;
  JA->Complete = 0;
  JA->Migrated = 0;

  for (jindex = 0;jindex < MMAX_JOBARRAYSIZE;jindex++)
    {
    if (JA->Members[jindex] == -1)
      break;

    if (JA->JName[jindex] == NULL)
      break;

    if (JA->JPtrs[jindex] == NULL)
      {
      MJobFind(JA->JName[jindex],&JA->JPtrs[jindex],mjsmBasic);
      }

    if (JA->JPtrs[jindex] != NULL)
      {
      if (MJobFind(JA->JName[jindex],&tmpJ,mjsmBasic) == FAILURE)
        {
        continue;
        }

      if (MJOBISACTIVE(tmpJ) == TRUE)
        {
        JA->JState[jindex] = mjsRunning;
        JA->Active++;
        }
      else if (tmpJ->State == mjsSuspended)
        {
        JA->JState[jindex] = mjsSuspended;
        JA->Active++;
        }
      else if (tmpJ->SubState == mjsstMigrated)
        {
        JA->JState[jindex] = mjsIdle;
        JA->Migrated++;
        }
      else
        {
        JA->JState[jindex] = mjsIdle;
        JA->Idle++;
        }

      continue;
      }
    else if ((JA->JState[jindex] == mjsCompleted) ||
             (JA->JState[jindex] == mjsRemoved))
      {
      JA->Complete++;
      }
    else
      {
      MDB(1,fSCHED) MLog("ERROR:    cannot find array job at index %d for job '%s'\n",
        JA->Members[jindex],
        J->Name);
      }
    }

  return(SUCCESS);
  }  /* END MJobArrayUpdate() */


/**
 * Return count of array sub jobs in a job array
 *
 * @param    JA     (I)
 * @param    countP (O)
 */

int MJobArrayGetArrayCount(
  
  mjarray_t *JA,
  int        *countP)

  {
  int count_subjobs;

  if (countP == NULL)
    {
    return(FAILURE);
    }

  if (JA == NULL)
    {
    *countP = 0;
    return(SUCCESS);
    }

  /* Need to determine the ArraySubJob length and use
   * that length against possible Class limit of maximum number
   * of ArraySubJobs that may be submitted per Array submission
   */
  for (count_subjobs = 0;count_subjobs < MMAX_JOBARRAYSIZE;count_subjobs++)
    {
    /* Short circuit at the end of the job array */
    if (JA->Members[count_subjobs] == -1)
      break;
    }

  *countP = count_subjobs;
  return(SUCCESS);
  } /* END MJobArrayGetArrayCount */


/**
 * Generate a subarray job name a new one
 *
 *  @param   J             (I)
 *  @param   jid           (I)
 *  @param   jindex        (I)
 *  @param   SJobID        (O)
 *  @param   SJobIDSize    (I)
 */

void __MJobArrayGenerateSubJobID(

    mjob_t      *J,
    int          jid,
    int          jindex,
    char        *SJobID,
    int          SJobIDSize)

  {
  mjarray_t *JA;              /* Job array structure */

  JA = J->Array;

  if (MSched.InternalRM->JobIDFormatIsInteger == TRUE)
    {
    snprintf(SJobID,SJobIDSize,"%d[%d]",
      jid,
      JA->Members[jindex]);
    }
  else
    {
    snprintf(SJobID,SJobIDSize,"%s.%d[%d]",
        MSched.Name,
        jid,
        JA->Members[jindex]);
    }
  } /* END __MJobArrayGenerateSubJobID */


/**
 * Generate one subjob array entry
 *
 * @param   J       master job pointer          (I)
 * @param   newJ    current tmp Job pointer     (O)
 * @param   jindex  current subjob array index  (I)
 * @param   SJobID  Subjob ID                   (I)
 * @param   R       Resource Manager            (I)
 */

void  __MJobArrayGenerateOneSubJob(
    
  mjob_t    *J,
  mjob_t    *newJ,
  int        jindex,
  char      *SJobID,
  mrm_t     *R)

  {
  char       ccount[MMAX_NAME];   /* Count of jobs */
  char       jobindex[MMAX_NAME]; /* index of job */
  mjarray_t *JA;                  /* Job array structure */

  JA = J->Array;

  /* job index */
  JA->JPtrs[jindex] = newJ;

  MUStrDup(&JA->JName[jindex],newJ->Name);

  MRMJobPreLoad(newJ,SJobID,J->SubmitRM);

  /* set job flags and spec job flags */
  bmset(&newJ->Flags,mjfArrayJob);
  bmset(&newJ->SpecFlags,mjfArrayJob);

  MJobDuplicate(newJ,J,TRUE);

  MUStrDup(&newJ->Env.Output,J->Env.Output);
  MUStrDup(&newJ->Env.Error,J->Env.Error);

  if ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtNative))
    MUStrDup(&newJ->DRMJID,newJ->Name);

  /* reset Master Array flag on sub job */
  bmunset(&newJ->Flags, mjfArrayMaster);
  bmunset(&newJ->SpecFlags, mjfArrayMaster);

  MUStrDup(&newJ->SRMJID,newJ->Name);

  MJGroupInit(newJ,J->Name,jindex,J);
  
  /* set internal flags - recopied in MJobDuplicate in MUIJobSubmit */
  bmclear(&newJ->IFlags);
  bmcopy(&newJ->IFlags,&J->IFlags);
  bmset(&newJ->IFlags,mjifIsArrayJob);

  MJobSetAttr(newJ,mjaSID,(void **)MSched.Name,mdfString,mSet);
  MJobSetAttr(newJ,mjaGJID,(void **)newJ->Name,mdfString,mSet);

  /* export job array variables to job environment */

  /* export JOBARRAYRANGE */
  snprintf(ccount,sizeof(ccount),"%d",JA->Count);
  MJobAddEnvVar(newJ,(char *)MJEnvVarAttr[mjenvJobArrayRange],ccount,ENVRS_ENCODED_CHAR);

  /* export JOBARRAYINDEX */
  snprintf(jobindex,sizeof(jobindex),"%d",JA->Members[jindex]);
  MJobAddEnvVar(newJ,(char *)MJEnvVarAttr[mjenvJobArrayIndex],jobindex,ENVRS_ENCODED_CHAR);

  /* In a grid, the master job isn't migrated to the slave so the sub job
   * doesn't know its place in the array. Whether in a grid or not, the array
   * variables are placed in the environment here and will get sucked into
   * the job by specifying that the job requested a full environment. */
  bmset(&newJ->IFlags,mjifEnvSpecified);
    
  /* copy depend structure here;
   * if Master is read in from spool directory before all subjobs are
   * restored, the dependency will be lost on those subjobs unless we
   * copy the dependency here.
   */

  MJobDependCopy(J->Depend,&newJ->Depend);

  MS3AddLocalJob(R,newJ->Name); 

  /* don't set to internal if job is active because the partition has already been set*/
  if ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtNative))
    {
    MUStrDup(&newJ->DRMJID,newJ->Name);
    newJ->Req[0]->RMIndex = J->Req[0]->RMIndex;
    }
  else
    {
    newJ->Req[0]->RMIndex = R->Index;
    }

  MUStrDup(&newJ->RMSubmitString,J->RMSubmitString);


  /* set initial hold state to state of the master in case it was started with a hold */
  newJ->Hold = J->Hold;
  } /* END __MJobArrayGenerateOneSubJob */


/**
 * process job input, output, error, and args 
 *
 * @param   J       master job pointer          (I)
 * @param   newJ    current tmp Job pointer     (O)
 * @param   jindex  current subjob array index  (I)
 */

void __MJobArrayDoIOStreamsSubstitution(

    mjob_t    *J,
    mjob_t    *newJ,
    int        jindex)

  {
  char      *ptr;
  char       JIndex[MMAX_NAME];   /* Job Index */
  int        BufSize;
  mjarray_t *JA;                  /* ptr to Job Array */

  JA = J->Array;

  /* map the Input stream to actual value */
  if (newJ->Env.Input != NULL)
    {
    BufSize = strlen(newJ->Env.Input) + 1;

    if ((ptr = strstr(newJ->Env.Input,"%I")) != NULL)
      {
      snprintf(JIndex,sizeof(JIndex),"%d",JA->Members[jindex]);

      MUBufReplaceString(&newJ->Env.Input,&BufSize,ptr - newJ->Env.Input,2,JIndex,TRUE);
      }

    if ((ptr = strstr(newJ->Env.Input,"%J")) != NULL)
      {
      MUBufReplaceString(&newJ->Env.Input,&BufSize,ptr - newJ->Env.Input,2,JA->Name,TRUE);
      }
    }

  /* map the Output stream to actual value */
  if (newJ->Env.Output != NULL)
    {
    BufSize = strlen(newJ->Env.Output) + 1;

    if ((ptr = strstr(newJ->Env.Output,"%I")) != NULL)
      {
      snprintf(JIndex,sizeof(JIndex),"%d",JA->Members[jindex]);

      MUBufReplaceString(&newJ->Env.Output,&BufSize,ptr - newJ->Env.Output,2,JIndex,TRUE);
      }

    if ((ptr = strstr(newJ->Env.Output,"%J")) != NULL)
      {
      MUBufReplaceString(&newJ->Env.Output,&BufSize,ptr - newJ->Env.Output,2,J->Name,TRUE);
      }
    }

  /* map the Error stream to actual value */
  if (newJ->Env.Error != NULL)
    {
    BufSize = strlen(newJ->Env.Error) + 1;

    if ((ptr = strstr(newJ->Env.Error,"%I")) != NULL)
      {
      snprintf(JIndex,sizeof(JIndex),"%d",JA->Members[jindex]);

      MUBufReplaceString(&newJ->Env.Error,&BufSize,ptr - newJ->Env.Error,2,JIndex,TRUE);
      }

    if ((ptr = strstr(newJ->Env.Error,"%J")) != NULL)
      {
      MUBufReplaceString(&newJ->Env.Error,&BufSize,ptr - newJ->Env.Error,2,J->Name,TRUE);
      }
    }
  }   /* END __MJobArrayDoIOStreamsSubstitution */

/** 
 * generate one subjob of the job array from the master job
 *
 * @param  J         Master array job.
 * @param  jid       job id
 * @param  jindex    New Job index to be copied to:  J->Array[jindex] 
 * @param  R         RM
 * @param  done
 * @param  EMsg
 */

int __MJobLoadArraySubJobConstructor(
    
  mjob_t  *J,
  int      jid,
  int      jindex,
  mrm_t   *R,
  mbool_t *done,
  char    *EMsg)

  {
  char     SJobID[MMAX_NAME];   /* new Job Name for sub job */
  mjob_t  *newJ;                /* temporary job structure */

  *done = FALSE;

  /* Generate the Array SubJob ID/Name */
  __MJobArrayGenerateSubJobID(J,jid,jindex,SJobID,sizeof(SJobID));

  /* See if the job already exists or is done */
  newJ = NULL;

  if ((MJobFind(SJobID,&newJ,mjsmBasic) == SUCCESS) ||
      (MJobCFind(SJobID,&newJ,mjsmBasic) == SUCCESS))
    {
    /* job already setup, indicate STOP processing */

    *done = TRUE;
    return(SUCCESS);
    }
  else if (MJobCreate(SJobID,TRUE,&newJ,NULL) == FAILURE)
    {
    MDB(1,fS3) MLog("ERROR:    job buffer is full  (ignoring job '%s')\n",
        SJobID);

    strcpy(EMsg, "Job buffer full. Try increasing MAXJOB parameter.\n");
    return(FAILURE);
    }

  /* go generate an individual subjob's entry */
  __MJobArrayGenerateOneSubJob(J,newJ,jindex,SJobID,R);

  /* process job input, output, error, and args %x values */
  __MJobArrayDoIOStreamsSubstitution(J,newJ,jindex);

  /* FIXME: this should be calling MRMJobPostLoad() */
  MJobTransition(newJ,FALSE,FALSE); 

  return (SUCCESS);
  } /* END __MJobLoadArraySubJobConstructor */


/** 
 * Creates jobs for the job array.
 *
 * @param J    (I) Master array job.
 * @param EMsg (O)
 */

int MJobLoadArray(

  mjob_t  *J,
  char    *EMsg)

  {
  int  count_subjobs;
  int  jindex;
  int  jid;
  int  rc;

  char *ptr;

  mjarray_t *JA;            /* Job array structure */
  mrm_t     *R;             /* resource manager structure */ 
  mbool_t    done;          /* bool to see if we return quickly */

  /* Sanity check arguments */
  if ((J == NULL) || (J->Array == NULL))
    {
    strcpy(EMsg,"[Internal] Bad arguments\n");
    return(FAILURE);
    }

  if ((J->SubmitRM != NULL) && (J->SubmitRM->Type != mrmtS3))
    {
    strcpy(EMsg,"[Internal] Non Native RM\n");
    return(FAILURE);
    }

  MRMFind("internal",&R);   /* Get reference to internal RM */

  /* Find the J ID value from the Job Name */
  ptr = strrchr(J->Name,'.');

  if ((ptr != NULL) && (ptr[1] != '\0'))
    {
    jid = strtol(&ptr[1],NULL,10);
    }
  else /* may be just a number */
    {
    jid = atoi(J->Name);
    }

  /* set flag to indicate this job is the master of the job array */
  bmset(&J->Flags,mjfArrayMaster);
  bmset(&J->SpecFlags,mjfArrayMaster);

  JA = J->Array;

  rc = MJobArrayGetArrayCount(JA,&count_subjobs);

  if (rc == FAILURE)
    {
    strcpy(EMsg, "[Internal] Failure to get JobArray Element Count\n");
    return(FAILURE);
    }

  /* Check our count_subjobs against the maximum arraysubjobs for this class:
   *   Only valid if there is a Class limit in place AND MaxArraySubJobs
   *   is greater than zero
   */
  if ((J->Credential.C != NULL) &&
      (J->Credential.C->MaxArraySubJobs > 0) &&
      (count_subjobs > J->Credential.C->MaxArraySubJobs))
    {
    /* If our request has a count of sub-jobs that is greater
     * than the Class MaxArraySubJobs limit, then we fail the attempt
     */
    snprintf(EMsg, MMAX_LINE, "Class MAX.ARRAYSUBJOBS limit exceeded. (ignoring job '%s')\n", J->Name);
    return(FAILURE);
    }

  /* upto 'count_subjobs' to iterate over
   * copy job for each index
   */
  for (jindex = 0;jindex < count_subjobs;jindex++)
    {
    /* Construct one subjob for each index */
    rc = __MJobLoadArraySubJobConstructor(J,jid,jindex,R,&done,EMsg);

    /* Any Non-SUCCESS return code terminates this function */
    if (rc != SUCCESS)
      {
      return(rc);
      }
    else
      {
      /* Need to determine if finished processing completely OR
       * if just this entry
       */
      if (done == TRUE)
        {
        /* We are completely done now */
        return(rc);
        }
      }
    }  /* END for (jindex) */

  return(SUCCESS);
  }  /* END MJobLoadArray() */


#if 0    /* NOT YET WORKING - called by MJobAllocMNL() */
/**
 * Wrapper to MJobAllocMNL() and MJobStart() for job arrays.
 *
 * @param J (I) job requesting resources
 * @param SFMNL (I) specified feasible nodes
 * @param NodeMap (I)
 * @param AMNL (O) [optional, if NULL, modify J->Req[X]->NodeList]
 * @param NAPolicy (I) node allocation policy
 * @param StartTime (I) time job must start
 * @param SC (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobArrayAllocMNL(
  
  mjob_t      *J,              /* I job requesting resources */
  mnl_t      **SFMNL,          /* I specified feasible nodes */
  char        *NodeMap,        /* I */
  mnl_t      **AMNL,           /* O allocated nodes (optional, if NULL, modify J->Req[X]->NodeList) */
  enum MNodeAllocPolicyEnum NAPolicy,  /* I node allocation policy */
  long         StartTime,      /* I time job must start      */
  int         *SC,             /* O (optional) */
  char        *EMsg)           /* O (optional,minsize=MMAX_LINE) */

  {
  int jindex;
  int nindex;
  int rqindex;

  mjob_t    *tmpJ;
  mjarray_t *JA;

  mbool_t  RsvCountRej;

  mnode_t *N;

  /* If AMNL is not NULL, then the caller is MJobAllocNML.
   * Since this function, MJobArrayAllocMNL, in turns
   * recursively calls MJobAllocMNL, but with AMNL == NULL,
   * thus AMNL is a stop recursive flag - of sorts
   */
  if ((J == NULL) || (J->Array == NULL) || (AMNL != NULL))
    {
    return(FAILURE);
    }

  JA = J->Array;

  for (jindex = 0;jindex < MMAX_JOBARRAYSIZE;jindex++)
    {
    if (JA->Members[jindex] == -1)
      break;

    if (JA->JIndex[jindex] <= 0)
      {
      /* Jobs should be allocated in MJobProcessArray() */

      continue;
      }

    /* Look for an idle job */
    if (JA->JState[jindex] != mjsIdle)
      continue;

    /* We found an idle MJob entry, so note its ptr */
    tmpJ = MJob[JA->JIndex[jindex]];

    /* If Policy limit is less than 0, then skip */
    if (JA->PLimit <= 0)
      break;
    else
      JA->PLimit--;

    if (J->Array != NULL)
      MSET(J->Array->Flags,mjafIgnoreArray);

    if (MJobAllocMNL(
          tmpJ,
          SFMNL,
          NodeMap,
          NULL,
          NAPolicy,
          StartTime,
          SC,
          EMsg) == FAILURE)
      {
      break;
      }

    if (MJobStart(tmpJ,NULL,NULL,"job array started") == FAILURE)
      {
      continue;
      }

    if (J->Array != NULL)
      MUNSET(J->Array->Flags,mjafIgnoreArray);

    /* need to MNLRemoveNode() */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (MNLIsEmpty(SFMNL[rqindex]))
        break;

      /* NOTE: no modifications needed for nodemap */

      for (nindex = 0;MNLGetNodeAtIndex(&tmpJ->Req[rqindex]->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        MNLRemoveNode( 
          SFMNL[rqindex],
          N,
          MNLGetTCAtIndex(&tmpJ->Req[rqindex]->NodeList,nindex));
        }
      }
    }  /* END for (jindex) */

  if (JA->PLimit < 0)
    return(SUCCESS);

  /* try creating priority reservations */

  for (;jindex < MMAX_JOBARRAYSIZE;jindex++)
    {
    if (JA->Members[jindex] == -1)
      break;

    if (JA->JIndex[jindex] <= 0)
      {
      /* Jobs should be allocated in MJobProcessArray() */

      continue;
      }

    tmpJ = MJob[JA->JIndex[jindex]];

    if (JA->PLimit < 0)
      break;
    else
      JA->PLimit--;

    if ((MJobPReserve(tmpJ,NULL,FALSE,0,&RsvCountRej) == SUCCESS) ||
        (RsvCountRej == FALSE))
      {
      continue;
      }

    break;
    }

  return(SUCCESS);
  }  /* END MJobArrayAllocMNL() */
#endif 


/**
 * Loops through the job array's JMember list and updates JIndex
 * with members' index into MJob[].
 *
 * Array master jobs are checkpointed with it's members indecies but the
 * indecies can't be relied on being the same after restarting so they
 * must be updated after a restart.
 *
 * @param MasterJ Job array master.
 */

void MJobArrayUpdateJobIndicies(
  
  mjob_t *MasterJ)

  {
  int memberIndex;
  mjob_t    *tmpJ   = NULL;
  mjarray_t *jArray = NULL;

  assert(MasterJ != NULL);
  assert(MasterJ->Array != NULL);
  assert(MasterJ->Array->JName != NULL);

  jArray = MasterJ->Array;

  for (memberIndex = 0;memberIndex < jArray->Count;memberIndex++)
    {
    if (MJobFind(jArray->JName[memberIndex],&tmpJ,mjsmBasic) == SUCCESS)
      {
      jArray->JPtrs[memberIndex] = tmpJ;
      }
    else if (MJobCFind(jArray->JName[memberIndex],&tmpJ,mjsmBasic) == SUCCESS)
      {
      jArray->JPtrs[memberIndex] = NULL; /* completed */
      }
    }

  } /* END void MJobArrayUpdateJobIndicies() */



/**
 * Updates any array master's JIndex table to correct indices.
 *
 * Because a job's index can change in the MJob table after restart, an array
 * master's JIndex list needs to be updated.
 */

void MJobUpdateJobArrayIndices()

  {
  job_iter JTI;

  mjob_t *tmpJ;

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&tmpJ) == SUCCESS)
    {
    if (tmpJ == NULL)
      break;

    if (MJobIsArrayMaster(tmpJ) != SUCCESS)
      continue;

    MJobArrayUpdateJobIndicies(tmpJ);
    }
  } /* void MJobUpdateJobArrayIndices() */


/* 
 * Frees J->Array and all alloc'ed structures within J->Array.
 *
 * @param mjob_t   *J
 *
 */

int MJobArrayFree(

  mjob_t *J)

  {
  int jindex;

  if ((J == NULL) || (J->Array == NULL))
    {
    return(SUCCESS);
    }

  for (jindex = 0;jindex < MMAX_JOBARRAYSIZE;jindex++)
    {
    if (J->Array->Members[jindex] == -1)
      break;

    MUFree((char **)&J->Array->JName[jindex]);
    }

  MUFree((char **)&J->Array->Members);
  MUFree((char **)&J->Array->JPtrs);
  MUFree((char **)&J->Array->JState);
  MUFree((char **)&J->Array->JName);

  MNLMultiFree(J->Array->FNL);

  MUFree((char **)&J->Array->FNL);
  MUFree((char **)&J->Array);

  return(SUCCESS);
  }  /* END MJobArrayFree() */




/**
 * Update individual job information for the job array.
 *
 * @see MJobArrayUpdate() - peer
 *
 * @param J        (I)
 * @param NewState (I)
 */

int MJobArrayUpdateJobInfo(

  mjob_t             *J,
  enum MJobStateEnum  NewState)

  {
  int     jindex;

  mjob_t *JA;

  if ((J == NULL) || (J->JGroup == NULL) || !bmisset(&J->Flags,mjfArrayJob))
    {
    return(FAILURE);
    }

  if ((MJobFind(J->JGroup->Name,&JA,mjsmBasic) == FAILURE) ||     
      (JA->Array == NULL))
    {
    MDB(1,fSCHED) MLog("ERROR:    cannot find master job (%s) for job '%s', job array slot limits may not be enforced\n",
      J->JGroup->Name,
      J->Name);

    return(FAILURE);
    }

  /* locate job in master job's table */

  if ((J->JGroup->ArrayIndex >= 0) && 
      (J->JGroup->ArrayIndex < MMAX_JOBARRAYSIZE) &&
      (JA->Array->Members[J->JGroup->ArrayIndex] != -1) &&
      (JA->Array->JPtrs[J->JGroup->ArrayIndex] == J))
    {
    /* we can use ArrayIndex and skip the search */

    jindex = J->JGroup->ArrayIndex;
    }
  else
    {
    /* couldn't use ArrayIndex, search the array instead */

    for (jindex = 0;jindex < MMAX_JOBARRAYSIZE;jindex++)
      {
      if (JA->Array->Members[jindex] == -1)
        {
        break;
        }

      if (JA->Array->JPtrs[jindex] != J)
        continue;

      break;
      }  

    J->JGroup->ArrayIndex = jindex;
    }  /* END else */

  switch (NewState)
    {
    case mjsRunning:
   
      JA->Array->Active++;

      if ((J->State == mjsIdle) && (JA->Array->Idle > 0))
        JA->Array->Idle--;

      break;

    case mjsCompleted:
    case mjsRemoved:

      JA->Array->Complete++;

      if (MJOBISACTIVE(J) == TRUE)
        JA->Array->Active--;
      else if (J->State == mjsIdle && JA->Array->Idle > 0)
        JA->Array->Idle--;

      JA->Array->JPtrs[jindex] = NULL;

      if (MJobArrayCancelIfMeetsPolicy(JA,J,jindex) == SUCCESS)
        {
        return(SUCCESS);
        }

    default:

      /* NO-OP */

      break;
    }  /* END switch (NewState) */

  JA->Array->JState[jindex] = NewState;

  /* if all jobs have completed, allow master to finish */
  if (JA->Array->Complete == JA->Array->Count)
    {
    JA->CompletionTime = MSched.Time;

    MJobProcessCompleted(&JA);
    }

  return(SUCCESS);
  }  /* END MJobArrayUpdateJobInfo() */




/**
 * Cancel an array job if it meets a certain policy.  Returns 
 * success if policy matched and job is cancelled. 
 *
 * @param JA   (I) [modified]  the master array job
 * @param J    (I)             the job to check
 * @param jindex (I)
 */

int MJobArrayCancelIfMeetsPolicy(

  mjob_t    *JA,
  mjob_t    *J,
  int       jindex)

  {
  char msg[MMAX_LINE];

  if ((JA == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

  if (JA->Array->Members[jindex] == 1)
    {
    if (((bmisset(&JA->Flags,mjfCancelOnFirstFailure)) &&
        (J->CompletionCode != 0)) ||
        ((bmisset(&JA->Flags,mjfCancelOnFirstSuccess)) &&
        (J->CompletionCode == 0)))
      {
      
      sprintf(msg,"Sub-job %s exit code %d cancelled job array %s with policy %s",
        J->Name,
        J->CompletionCode,
        JA->Name,
        (bmisset(&JA->Flags,mjfCancelOnFirstFailure)) ? "CancelOnFirstFailure" : "CancelOnFirstSuccess");

      MDB(1,fSCHED) MLog("INFO:     %s\n",
         msg);

      MMBAdd(&JA->MessageBuffer,msg,NULL,mmbtNONE,0,0,NULL);
    
      /* cancel the array job */
      
      MJobCancel(JA,NULL,FALSE,NULL,NULL);
    
      return(SUCCESS);
      }
    else if ((bmisset(&JA->Flags,mjfCancelOnFirstFailure)) ||
             (bmisset(&JA->Flags,mjfCancelOnFirstSuccess)))
      {
      /* if CancelOnFirstFailure or mjfCancelOnFirstSuccess is specified but the job didn't fail either previous
         condition, just return and don't check the following conditions.  This is because when CancelOnFirstFailure or
         mjfCancelOnFirstSuccess are used in conjunction with other flags, the other flags are meant to check the
         rest of the jobs, not the job at index 1 */
  
        return(FAILURE);
      }
    }

  /* after checking CancelOnFirstFailure or CancelOnFirstSuccess, check the following conditions as well.
     There can be different combinations (i.e. flags=CancelOnFirstFailure:CancelOnExitCode=1 ) */

  if (((bmisset(&JA->Flags,mjfCancelOnAnyFailure)) &&
      (J->CompletionCode != 0)) ||
      ((bmisset(&JA->Flags,mjfCancelOnAnySuccess)) &&
      (J->CompletionCode == 0)))
    {
    sprintf(msg,"Sub-job %s exit code %d cancelled job array %s with policy %s",
      J->Name,
      J->CompletionCode,
      JA->Name,
      (bmisset(&JA->Flags,mjfCancelOnAnyFailure)) ? "CancelOnAnyFailure" : "CancelOnAnySuccess");

    MDB(1,fSCHED) MLog("INFO:     %s\n",
       msg);

    MMBAdd(&JA->MessageBuffer,msg,NULL,mmbtNONE,0,0,NULL);

    /* cancel the array job */
    
    MJobCancel(JA,NULL,FALSE,NULL,NULL);
  
    return(SUCCESS);
    }
  else if ((bmisset(&JA->Flags,mjfCancelOnExitCode)))
    {
    char *exitCodes = NULL;
    char *ptr;
    char *TokPtr;

    /* if the job array flag is set to CancelOnExitCode and any job exits with a given
       code, cancel the array job */

    if (MJobGetDynamicAttr(JA,mjaCancelExitCodes,(void **)&exitCodes,mdfString) == FAILURE)
      return(FAILURE);

    ptr = MUStrTok(exitCodes,"+",&TokPtr);

    while(ptr != NULL)
      {
      /* check to see if the value is a single number or a range */
      if (strchr(ptr,'-'))
        {
        char *highValueStr;
        char *lowValueStr;
  
        lowValueStr = MUStrTok(ptr,"-",&highValueStr);

        /* check to see if the exit code is within the give range */
        if ((J->CompletionCode >= strtol(lowValueStr,NULL,0)) && 
            (J->CompletionCode <= strtol(highValueStr,NULL,0)))
          {
          sprintf(msg,"Sub-job %s exit code %d cancelled job array %s with policy CancelOnExitCode",
            J->Name,
            J->CompletionCode,
            JA->Name);

          MDB(1,fSCHED) MLog("INFO:     %s\n",
             msg);
    
          MMBAdd(&JA->MessageBuffer,msg,NULL,mmbtNONE,0,0,NULL);
        
          /* cancel the array job */
          
          MJobCancel(JA,NULL,FALSE,NULL,NULL);
        
          MUFree(&exitCodes);
          return(SUCCESS);
          }
        }
      else
        {
        /* compare the job's exit code to the code given to see if they're the same */
        if (J->CompletionCode == strtol(ptr,NULL,0))
          {
          sprintf(msg,"Sub-job %s exit code %d cancelled job array %s with policy CancelOnExitCode. ",
            J->Name,
            J->CompletionCode,
            JA->Name);

          MDB(1,fSCHED) MLog("INFO:     %s\n",
             msg);
    
          MMBAdd(&JA->MessageBuffer,msg,NULL,mmbtNONE,0,0,NULL);
     
          /* cancel the array job */
          
          MJobCancel(JA,NULL,FALSE,NULL,NULL);
        
          MUFree(&exitCodes);
          return(SUCCESS);
          }
        }

      ptr = MUStrTok(NULL,"+",&TokPtr);
      }
    MUFree(&exitCodes);
    }

  return(FAILURE);
  }





/**
 * Copy the given job name into the job array's list of member jobs.
 *
 * @param JA      (I) [modified]  the job array
 * @param Name    (I)             the job name to insert
 * @param JIndex  (I)             the index in the JName array to put it in
 */

int MJobArrayInsertName(

  mjarray_t  *JA,     /* (I) [modified] */
  char       *Name,   /* (I) */
  int         JIndex) /* (I) */

  {
  if ((JA == NULL) || (MUStrIsEmpty(Name)))
    {
    return(FAILURE);
    }

  MUStrDup(&JA->JName[JIndex],Name);

  return(SUCCESS);
  }


/**
 * Locks a job array to the specified partition.
 *
 * Locks the job into a specific partition by setting the job's SpecPAL.
 *
 * @param J The job used to lock the rest of the array to the specific partition.
 * @param LockP The partition to lock the array to.
 */

int MJobArrayLockToPartition(

  mjob_t *J,
  mpar_t *LockP)

  {
  mjob_t    *masterJob;
  mjob_t    *arrayJob;
  mjarray_t *jArray;
  int arrayIndex;

  if ((J == NULL) || (LockP == NULL))
    return(FAILURE);

  /* JGroup will be null is running on a slave. The arrayjob flag is sent
   * but has no idea of it's group. */
  if (J->JGroup == NULL)
    return(SUCCESS);

  /* Get master job in order to lock all members to partition. */
  if ((MJobFind(J->JGroup->Name,&masterJob,mjsmExtended) == FAILURE) ||
      (masterJob == NULL) || (masterJob->Array == NULL) ||
      (MJobIsArrayMaster(masterJob) == FALSE))
    {
    return(FAILURE);
    }

  jArray = masterJob->Array;

  if (bmisset(&jArray->Flags,mjafPartitionLocked))
    {
    return(SUCCESS);
    }

  /* Lock all array members to job */
  for (arrayIndex = 0;arrayIndex < jArray->Count;arrayIndex++)
    {
    arrayJob = jArray->JPtrs[arrayIndex];

    /* could we assume that if a job is completed that all the other array
     * members were already locked into a partition and return? */

    if (arrayJob == NULL) /* job has completed */
      continue;
    
    bmclear(&arrayJob->SpecPAL);
    bmset(&arrayJob->SpecPAL,LockP->Index);

    MJobGetPAL(arrayJob,&arrayJob->SpecPAL,&arrayJob->SysPAL,&arrayJob->PAL);
    }

  bmset(&jArray->Flags,mjafPartitionLocked);

  return(SUCCESS);
  } /*END int MJobArrayLockToPartition() */



/**
 * Get the processor count of sub jobs belonging to the given array master
 * job in the given list
 *
 * @param MJ      (I) the array master job
 * @param JList   (I) the list of jobs we want a count for (active,
 *                    idle-reserved, or idle from MCacheReadJobsForShowQ)
 * @param PC      (O) pointer to return the proc count
 *                
 * NOTE:  this does NOT zero PC first, the caller must set PC to zero before
 *        calling if needed.
 */

int MJobTransitionGetArrayProcCount(

  mtransjob_t  *MJ,     /* I */
  marray_t     *JList,  /* I */
  int          *PC)     /* O */

  {
  int sjindex;
  mtransjob_t *SubJob = NULL;

  if ((MJ == NULL) || (JList == NULL) || (PC == NULL))
    {
    return(FAILURE);
    }

  for (sjindex = 0; sjindex < JList->NumItems; sjindex++)
    {
    SubJob = (mtransjob_t *)MUArrayListGetPtr(JList,sjindex);

    if ((bmisset(&SubJob->TFlags,mtransfArraySubJob)) &&
        (SubJob->ArrayMasterID == MJ->ArrayMasterID))
      {
      *PC += SubJob->MaxProcessorCount;
      }
    } /* END for (sjindex... ) */

  return(SUCCESS);
  } /* END MJobTransitionGetArrayProcCount() */




/**
 * Get the number of array sub-jobs belonging to the given array master job
 * in the given list
 *
 * @param MJ      (I) the array master job
 * @param JList   (I) the list of jobs we want a count for
 * @param JC      (O) pointer to return the job count
 *                
 * NOTE:  this does NOT zero JC first, the caller must set JC to zero before
 *        calling if needed.
 */

int MJobTransitionGetArrayJobCount(

  mtransjob_t  *MJ,     /* I */
  marray_t     *JList,  /* I */
  int          *JC)     /* O */

  {
  int sjindex;
  mtransjob_t *SubJob = NULL;

  if ((MJ == NULL) || (JList == NULL) || (JC == NULL))
    {
    return(FAILURE);
    }

  for (sjindex = 0; sjindex < JList->NumItems; sjindex++)
    {
    SubJob = (mtransjob_t *)MUArrayListGetPtr(JList,sjindex);

    if ((bmisset(&SubJob->TFlags,mtransfArraySubJob)) &&
        (SubJob->ArrayMasterID == MJ->ArrayMasterID))
      {
      *JC += 1;
      }
    } /* END for (sjindex...) */

  return(SUCCESS);
  } /* END MJobTransitionGetArrayJobCount() */




/**
 * Get the start time of the last started, currently running array subjob
 *
 * @param MJ      (I) the array master job
 * @param JList   (I) the list of jobs we want to check start times for
 * @param STime   (O)
 */

int MJobTransitionGetArrayRunningStartTime(

  mtransjob_t  *MJ,
  marray_t     *JList,
  mulong       *STime)

  {
  int           sjindex;
  mtransjob_t  *SubJob = NULL;

  if ((MJ == NULL) || (JList == NULL) || (STime == NULL))
    {
    return(FAILURE);
    }

  for (sjindex = 0; sjindex < JList->NumItems; sjindex++)
    {
    SubJob = (mtransjob_t *)MUArrayListGetPtr(JList,sjindex);

    if ((bmisset(&SubJob->TFlags,mtransfArraySubJob)) &&
        (SubJob->ArrayMasterID == MJ->ArrayMasterID) &&
        ((mulong)(SubJob->StartTime) > *STime))
      {
      *STime = SubJob->StartTime;
      }
    } /* END for (sjindex...) */

  return(SUCCESS);
  } /* END MJobTransitionGetArrayRunningStartTime() */




/**
 * Get the unique integer ID of the job array master for the given job
 *
 * @param J     (I) the job we want the array master ID for (may be a master
 *                  or sub-job)
 */

int MJobGetArrayMasterTransitionID(

  mjob_t *J)    /* I */

  {
  int MasterID = -1;
  char *ptr = NULL;

  /* J->JGroup is the name of the master */

  if (bmisset(&J->Flags,mjfArrayMaster))
    {
    ptr = strstr(J->Name,".");
    }
  else if (bmisset(&J->Flags,mjfArrayJob))
    {
    ptr = strstr(J->JGroup->Name,".");
    }

  if (ptr != NULL)
    {
    ptr += 1;
    }
  else
    {
    ptr = (bmisset(&J->Flags,mjfArrayMaster)) ? J->Name : J->JGroup->Name;
    }

  MasterID = strtol(ptr,NULL,10);

  return(MasterID);
  } /* END MJobGetArrayMasterTransitionID() */


